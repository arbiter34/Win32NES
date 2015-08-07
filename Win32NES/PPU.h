#pragma once
#include <stdint.h>
#include "defines.h"
#include "Palette.h"

class PPU
{	
	// 16 bytes per pattern
	static const int cPATTERN_SIZE = 16;
public:

	color_t screen[SCREEN_WIDTH * SCREEN_HEIGHT];

	PPU();
	~PPU();

	bool render_scanline(int scanline);
	void set_chr_rom(uint8_t *chr_rom);

	void write_control1(uint8_t word);
	void write_control2(uint8_t word);
	void set_sprite_memory_address(uint8_t word);
	void write_sprite_data(uint8_t word);
	void write_scroll_register(uint8_t word);
	void write_vram_address(uint8_t word);
	void write_vram_data(uint8_t word);
	void write_spr_ram(char* start);
	uint8_t read_control1();            
	uint8_t read_control2();			
	uint8_t read_status();			
	uint8_t read_sprite_data();			
	uint8_t read_vram_data();

private:

	uint8_t vram[VRAM_SIZE];
	uint8_t spr_ram[SPR_RAM_SIZE];
	
	uint8_t control_1;
	uint8_t control_2;
	uint8_t status;
	uint8_t sprite_mem_address;

	uint8_t read_buffer;

	// PPU Scroll Registers
	uint8_t regFV; // Fine vertical scroll latch
	uint8_t regV;  // Vertical name table selection latch
	uint8_t regH;  // Horizontal name table selection latch
	uint8_t regVT; // Vertical tile index latch
	uint8_t regHT; // Horizontal tile index latch
	uint8_t regFH; // Fine horizontal scroll latch
	uint8_t regS;  // Playfield pattern selection table latch

	// PPU Scroll counters
	uint8_t cntFV;
	uint8_t cntV;
	uint8_t cntH;
	uint8_t cntVT;
	uint8_t cntHT;

	bool first_write;

	color_t get_pixel(int x, int y);
	void draw_pixel(int x, int y, color_t color);
	uint8_t color_index_for_pattern_bit(int x, uint16_t pattern_start, int palette_select, bool sprite);
	void print_pattern(int pattern_num);
	void render_scanline_display(int scanline);

	uint16_t calculate_effective_address(uint16_t address);
	uint8_t read_memory(uint16_t address);
	void store_memory(uint16_t address, uint8_t word);

	void reset_vblank_flag();
	void reset_sprite_0_flag();
	void set_sprite_0_flag();

	void reset_more_than_8_sprites_flag();
	void set_more_than_8_sprites_flag();

	uint16_t vram_address();
	uint16_t nametable_address();
	uint16_t attributetable_address();
	uint16_t patterntable_address();

	uint8_t palette_select_bits();

	void increment_scroll_counters();
	void increment_horizontal_scroll_counter();
	void increment_vertical_scroll_counter();
	void update_scroll_counters_from_registers();

	bool is_screen_enabled();
};

