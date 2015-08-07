#include "stdafx.h"
#include "PPU.h"


PPU::PPU()
{
	control_1 = 0;
	control_2 = 0;
	status = 0;

	first_write = true;

	for (int i = 0; i < VRAM_SIZE; i++) {
		vram[i] = 0;
	}

	color_t black{ 0, 0, 0 };
	for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
		screen[i] = black;
	}
}


PPU::~PPU()
{
}

bool  PPU::render_scanline(int scanline){
	if (scanline == 0) {
		reset_sprite_0_flag();
	}

	if (scanline == 20 & is_screen_enabled()) {
		update_scroll_counters_from_registers();
	}

	if (scanline >= 21 && scanline <= 260 && is_screen_enabled()) {
		render_scanline_display(scanline - 21);
	}

	cntH = regH;
	cntHT = regHT;

	if (scanline == 261) {
		status |= PPU_STATUS_VBLANK_MASK;
		return control_1 & VBLANK_INTERRUPT_ENABLE_MASK;
	}
}

void  PPU::set_chr_rom(uint8_t *chr_rom){
	memcpy(vram, chr_rom, PATTERN_TABLE_SIZE);
}

void  PPU::write_control1(uint8_t word){
	control_1 = word;

	regH = (word & NAME_TABLE_X_SCROLL_MASK) >> NAME_TABLE_X_SCROLL_BIT;
	regV = (word & NAME_TABLE_Y_SCROLL_MASK) >> NAME_TABLE_Y_SCROLL_BIT;
	regS = (word & BACKGROUND_PATTERN_TABLE_ADDRESS_MASK) >> BACKGROUND_PATTERN_TABLE_ADDRESS_BIT;
}

void  PPU::write_control2(uint8_t word){
	control_2 = word;
}

void  PPU::set_sprite_memory_address(uint8_t word){
	sprite_mem_address = word;
}

void  PPU::write_sprite_data(uint8_t word){
	spr_ram[sprite_mem_address++] = word;
}

void  PPU::write_scroll_register(uint8_t word){
	if (first_write) {
		regFH = word & 0x07;
		regHT = (word & 0xF8) >> 3;
	}
	else {
		regFV = word & 0x07;
		regVT = (word & 0xF8) >> 3;
	}

	first_write = !first_write;
}

void  PPU::write_vram_address(uint8_t word){
	if (first_write) {
		regVT = (regVT & 0x07) | ((word & 0x03) << 3);
		regH = (word & 0x04) >> 2;
		regV = (word & 0x08) >> 3;
		regFV = (word & 0x30) >> 4;
	}
	else {
		regHT = (word & 0x1F);
		regVT = (regVT & 0x18) | ((word & 0xE0) >> 5);

		update_scroll_counters_from_registers();
	}

	first_write = !first_write;
}

void  PPU::write_vram_data(uint8_t word){
	store_memory(vram_address(), word);
	increment_scroll_counters();
}

void  PPU::write_spr_ram(char* start){
	memcpy(spr_ram, start, SPR_RAM_SIZE);
}

uint8_t  PPU::read_control1(){
	return control_1;
}

uint8_t  PPU::read_control2(){
	return control_2;
}

uint8_t  PPU::read_status(){
	uint8_t temp = status;
	reset_vblank_flag();
	first_write = true;
	return temp;
}

uint8_t  PPU::read_sprite_data(){
	return spr_ram[sprite_mem_address++];
}

uint8_t  PPU::read_vram_data(){
	uint8_t result;

	if (vram_address() >= PALETTE_TABLE_START) {
		result = read_memory(vram_address());
	}
	else {
		result = read_buffer;
		read_buffer = read_memory(vram_address());
	}

	increment_scroll_counters();
	return result;
}

uint16_t  PPU::calculate_effective_address(uint16_t address){
	address &= 0x3FFF;
	if (address >= 0x2000 && address < 0x3000) {
		address &= 0x27FF; // TODO: vertical/horiz mirroring
	}
	else if (address >= 0x3000 && address < 0x3F00) {
		address -= 0x1000;
	}
	else if (address >= 0x3F00 && address < 0x4000) {
		address &= 0x3F1F;

		if ((address & 0x03) == 0) {
			address &= 0x3F0F;
		}
	}

	return address;
}

uint8_t  PPU::read_memory(uint16_t address){
	return vram[calculate_effective_address(address)];
}

void  PPU::store_memory(uint16_t address, uint8_t word){
	vram[calculate_effective_address(address)] = word;
}

void  PPU::reset_vblank_flag(){
	status &= ~PPU_STATUS_VBLANK_MASK;
}

void  PPU::reset_sprite_0_flag(){
	status &= ~PPU_STATUS_SPRITE_0_HIT_MASK;
}

void  PPU::set_sprite_0_flag(){
	status |= PPU_STATUS_SPRITE_0_HIT_MASK;
}

void PPU::reset_more_than_8_sprites_flag() {
	status &= ~PPU_STATUS_MORE_THAN_8_SPRITES_HIT_MASK;
}

void PPU::set_more_than_8_sprites_flag() {
	status |= PPU_STATUS_MORE_THAN_8_SPRITES_HIT_MASK;
}

uint16_t  PPU::vram_address(){
	uint16_t address = (uint16_t)cntHT;
	address |= ((uint16_t)cntVT) << 5;
	address |= ((uint16_t)cntH) << 10;
	address |= ((uint16_t)cntV) << 11;
	address |= ((uint16_t)cntFV & 0x03) << 12;
	return address;
}

uint16_t  PPU::nametable_address(){
	uint16_t address = 0x2000;
	address |= (uint16_t)cntHT;
	address |= (uint16_t)cntVT << 5;
	address |= (uint16_t)cntH << 10;
	address |= (uint16_t)cntV << 11;
	return address;
}

uint16_t  PPU::attributetable_address(){
	int attr_byte = read_memory(attributetable_address());

	// Figure out which two bits to take
	int bits_offset = ((cntVT & 0x02) << 1) | (cntHT & 0x02);

	// Mask and shift
	return (attr_byte & (0x03 << (bits_offset))) >> bits_offset;
}

uint16_t  PPU::patterntable_address(){
	uint8_t pattern_index = read_memory(nametable_address());

	uint16_t address = (uint16_t)regS << 12;
	address |= (uint16_t)pattern_index << 4;
	address |= cntFV;
	return address;
}

uint8_t  PPU::palette_select_bits(){
	int attr_byte = read_memory(attributetable_address());

	// Figure out which two bits to take
	int bits_offset = ((cntVT & 0x02) << 1) | (cntHT & 0x02);

	// Mask and shift
	return (attr_byte & (0x03 << (bits_offset))) >> bits_offset;
}

void  PPU::increment_scroll_counters(){
	bool vertical_write = ((control_1 & VERTICAL_WRITE_MASK) == VERTICAL_WRITE_ON);

	if (!vertical_write) {
		cntHT++;
		if (cntHT == 0x20) {
			cntHT = 0;
			cntVT++;
		}
	}
	else {
		cntVT++;
	}

	if (cntVT == 0x20) {
		cntVT = 0;

		if (++cntH == 0x02) {
			cntH = 0;

			if (++cntV == 0x02) {
				cntV = 0;

				if (++cntFV == 0x08) {
					cntFV = 0;
				}
			}
		}
	}
}

void  PPU::increment_horizontal_scroll_counter(){
	if (++cntHT == 0x20) {
		cntHT = 0;

		if (++cntH == 0x02) {
			cntH = 0;
		}
	}
}

void  PPU::increment_vertical_scroll_counter(){
	if (++cntFV == 0x08) {
		cntFV = 0;

		if (++cntVT == 30) { // Is this correct??
			cntVT = 0;

			if (++cntV == 0x02) {
				cntV = 0;
			}
		}
	}
}

void  PPU::update_scroll_counters_from_registers(){
	cntFV = regFV;
	cntV = regV;
	cntH = regH;
	cntVT = regVT;
	cntHT = regHT;
}

bool  PPU::is_screen_enabled(){
	return (control_2 & BACKGROUND_ENABLE_MASK) == BACKGROUND_ENABLE ||
		(control_2 & SPRITES_ENABLE_MASK) == SPRITES_ENABLE;
}


color_t PPU::get_pixel(int x, int y) {
	return screen[y*SCREEN_HEIGHT + x];
}

void PPU::draw_pixel(int x, int y, color_t color) {
	screen[y*SCREEN_WIDTH + x] = color;
}

uint8_t PPU::color_index_for_pattern_bit(int x, uint16_t pattern_start, int palette_select, bool sprite) {
	uint8_t lower_byte = read_memory(pattern_start);
	uint8_t higher_byte = read_memory(pattern_start + 8);

	int pattern_bit = 7 - x; // x is ascending left to right; that's H -> L in bit order

	uint8_t palette_entry = (palette_select << 2) |
		((lower_byte & (1 << pattern_bit)) >> pattern_bit) |
		(pattern_bit == 0 ? ((higher_byte & (1 << pattern_bit)) << 1) :
		((higher_byte & (1 << pattern_bit)) >> (pattern_bit - 1)));

	uint16_t palette_address = PALETTE_TABLE_START;

	if ((palette_entry & 0x03) != 0) {
		palette_address += (sprite ? PALETTE_TABLE_SPRITE_OFFSET : 0) + palette_entry;
	}

	return read_memory(palette_address);
}

void PPU::print_pattern(int pattern_num) {

}

void PPU::render_scanline_display(int scanline) {

	uint8_t control_1 = read_control1();
	uint8_t control_2 = read_control2();

	reset_more_than_8_sprites_flag();

	// TODO: Render Sprites & Background at the same time

	///////////////////////////
	// RENDER THE BACKGROUND //
	///////////////////////////
	if ((control_2 & BACKGROUND_ENABLE_MASK) == BACKGROUND_ENABLE) {
		int x = 0;
		int tile_column = (x + regFH) / 8;

		while (x < SCREEN_WIDTH) {
			draw_pixel(
				x,
				scanline,
				NES_PALETTE[
					color_index_for_pattern_bit(
						(x + regFH) % 8, // bit offset within that tile
						patterntable_address(),
						palette_select_bits(),
						false
						)
				]
				);

					// roll over to the next tile?
					if ((++x + regFH) % 8 == 0) {
						increment_horizontal_scroll_counter();
						tile_column++;
					}
		}
	}

	////////////////////////
	// RENDER THE SPRITES //
	////////////////////////
	if ((control_2 & SPRITES_ENABLE_MASK) == SPRITES_ENABLE) {
		if ((control_2 & SPRITE_SIZE_MASK) == SPRITE_SIZE_8x16) {
			throw "Unimplemented sprite size 8x16!";
		}

		uint8_t transparency_color_index = read_memory(PALETTE_TABLE_START);
		color_t transparency_color = NES_PALETTE[transparency_color_index];

		int sprites_drawn = 0;

		// Lowest number sprites are highest priority to draw
		for (int i = 63; i >= 0; i--) {
			int ypos = spr_ram[i * 4] + 1;

			if (!(ypos <= scanline && ypos + 7 >= scanline)) {
				// Does this sprite intersect with this scanline?
				continue;
			}

			sprites_drawn += 1;

			if (sprites_drawn == 9) {
				set_more_than_8_sprites_flag();
				break;
			}

			int pattern_num = spr_ram[i * 4 + 1];
			uint8_t color_attr = spr_ram[i * 4 + 2];
			uint8_t xpos = spr_ram[i * 4 + 3];

			if ((control_1 & SPRITE_PATTERN_TABLE_ADDRESS_MASK) == SPRITE_PATTERN_TABLE_ADDRESS_1000) {
				pattern_num += 256;
			}

			int upper_color_bits = color_attr & 0x03;
			bool flip_horizontal = color_attr & 0x40;
			bool flip_vertical = color_attr & 0x80;

			int y = scanline - ypos;
			for (int x = 0; x < 8; x++) {
				uint16_t pattern_start = pattern_num * cPATTERN_SIZE + (flip_vertical ? 7 - y : y);
				uint8_t color_index =
					color_index_for_pattern_bit((flip_horizontal ? 7 - x : x), pattern_start, upper_color_bits, true);

				if (color_index != transparency_color_index) {
					color_t current_pixel = get_pixel(xpos + x, scanline);

					// Sprite 0 hit flag - TODO: Is this correct??
					if (i == 0 && current_pixel != transparency_color) {
						set_sprite_0_flag();
					}

					// if color_attr & 0x20 == 0x20, sprite is drawn behind background (but not transparent color)
					if ((color_attr & 0x20) == 0x00 || current_pixel == transparency_color) {
						draw_pixel(xpos + x, scanline, NES_PALETTE[color_index]);
					}
				}
			}
		}
	}

	if (scanline == 239) {
		//TODO: Flip
	}

	increment_vertical_scroll_counter();
}