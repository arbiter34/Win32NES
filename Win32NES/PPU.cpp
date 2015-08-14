#include "stdafx.h"
#include "PPU.h"


PPU::PPU()
{
	reset();
	screen = front;
}


PPU::~PPU()
{
}

void PPU::setCPU(CPU *cpu) {
	this->cpu = cpu;
}

void PPU::reset() {
	cycle = 340;
	scanline = 240;
	frame = 0;
	writeControl(0);
	writeMask(0);
	writeOAMAddress(0);
}


uint8_t PPU::readPalette(uint16_t address) {
	if (address >= 16 && address % 4 == 0) {
		address -= 16;
	}
	return paletteData[address];
}

void PPU::writePalette(uint16_t address, uint8_t word) {
	if (address >= 16 && address % 4 == 0) {
		address -= 16;
	}
	paletteData[address] = word;
}

uint8_t PPU::readRegister(uint16_t address) {
	switch (address) {
		case 0x2002:
			return readStatus();
		case 0x2004:
			return readOAMData();
		case 0x2007:
			return readData();
	}
	return 0;
}

void PPU::writeRegister(uint16_t address, uint8_t word) {
	reg = word;
	switch (address) {
		case 0x2000:
			writeControl(word);
			break;
		case 0x2001:
			writeMask(word);
			break;
		case 0x2003:
			writeOAMAddress(word);
			break;
		case 0x2004:
			writeOAMData(word);
			break;
		case 0x2005:
			writeScroll(word);
			break;
		case 0x2006:
			writeAddress(word);
			break;
		case 0x2007:
			writeData(word);
			break;
		case 0x4014:
			writeDMA(word);
			break;
	}
}

void PPU::writeControl(uint8_t word) {
	PPUCTRL = word;
	nmiChange();
	t = (t & 0xF3FF) | ((((uint16_t)word) & 0x03) << 10);
}

void PPU::writeMask(uint8_t word) {
	PPUMASK = word;
}

uint8_t PPU::readStatus() {
	uint8_t result = (PPUSTATUS & 0xE0) | (reg & 0x1F);
	if (nmiOccurred) {
		result |= 1 << 7;
	}
	nmiOccurred = false;
	nmiChange();
	w = 0;
	return result;
}

void PPU::writeOAMAddress(uint8_t word) {
	OAMADDR = word;
}

uint8_t PPU::readOAMData() {
	return oamData[OAMADDR];
}

void PPU::writeOAMData(uint8_t word) {
	oamData[OAMADDR++] = word;
}

void PPU::writeScroll(uint8_t word) {
	if (w == 0) {
		t = (t & 0xFFE0) | (((uint16_t)word) >> 3);
		x = word & 0x07;
		w = 1;
	}
	else {
		t = (t & 0x8FFF) | ((((uint16_t)word) & 0x07) << 12);
		t = (t & 0xFC1F) | ((((uint16_t)word) & 0xF8) << 2);
		w = 0;
	}
}

void PPU::writeAddress(uint8_t word) {
	if (w == 0) {
		t = (t & 0x80FF) | ((((uint16_t)word) & 0x3F) << 8);
		w = 1;
	}
	else {
		t = (t & 0xFF00) | ((uint16_t)word);
		v = t;
		w = 0;
	}
}

uint8_t PPU::readData() {
	uint16_t word = cpu->read_memory(v);

	// emulate buffered reads
	if ((v % 0x4000) < 0x3F00) {
		uint8_t temp = PPUDATA;
		PPUDATA = word;
		word = temp;
	}
	else {
		PPUDATA = cpu->read_memory(v - 0x1000);
	}

	// increment address
	if ((PPUCTRL & INCREMENT) != 0) {
		v += 1;
	}
	else {
		v += 32;
	}
	return word;
}

void PPU::writeData(uint8_t word) {
	cpu->store_memory(v, word);
	if ((PPUCTRL & INCREMENT) != 0) {
		v++;
	}
	else {
		v += 32;
	}
}

void PPU::writeDMA(uint8_t word) {
	uint16_t address = ((uint16_t)word) << 8;
	for (int i = 0; i < 256; i++) {
		oamData[OAMADDR] = cpu->read_memory(address);
		OAMADDR++;
		address++;
	}
	cpu->stall += 513;
	if ((cpu->cycleCount % 2) == 1) {
		cpu->stall++;
	}
}

void PPU::incrementX() {
	if ((v & 0x001F) == 31) {
		v &= 0xFFE0;
		v ^= 0x0400;
	}
	else {
		v++;
	}
}

void PPU::incrementY() {
	if (v & 0x7000 != 0x7000) {
		// increment fine Y
		v += 0x1000;
	}
	else {
		// fine Y = 0
		v &= 0x8FFF;
			// let y = coarse Y
		uint16_t y = (v & 0x03E0) >> 5;
		if (y == 29) {
			// coarse Y = 0
			y = 0;
				// switch vertical nametable
			v ^= 0x0800;
		}
		else if (y == 31) {
			// coarse Y = 0, nametable not switched
			y = 0;
		}
		else {
			// increment coarse Y
			y++;
		}
		// put coarse Y back into v
		v = (v & 0xFC1F) | (y << 5);
	}
}

void PPU::copyX() {
	v = (v & 0xFBE0) | (t & 0x041F);
}

void PPU::copyY() {
	v = (v & 0x841F) | (t & 0x7BE0);
}

void PPU::nmiChange() {
	bool nmi = nmiOutput && nmiOccurred;
	if (nmi && !nmiPrevious) {
		 // TODO: this fixes some games but the delay shouldn't have to be so
		 // long, so the timings are off somewhere
		nmiDelay = 15;
	}
	nmiPrevious = nmi;
}

void PPU::setVerticalBlank() {
	if (screen == front) {
		screen = back;
	}
	else {
		screen = front;
	}
	nmiOccurred = true;
	nmiChange();
}

void PPU::clearVerticalBlank() {
	nmiOccurred = false;
	nmiChange();
}

void PPU::fetchNameTableByte() {
	uint16_t temp_v = this->v;
	uint16_t address = 0x2000 | (v & 0x0FFF);
	nameTableByte = cpu->read_memory(address);
}

void PPU::fetchAttributeTableByte() {
	uint16_t temp_v = this->v;
	uint16_t address = 0x23C0 | (temp_v & 0x0C00) | ((temp_v >> 4) & 0x38) | ((temp_v >> 2) & 0x07);
	uint16_t shift = ((temp_v >> 4) & 4) | (temp_v & 2);
	attributeTableByte = ((cpu->read_memory(address) >> shift) & 3) << 2;
}

void PPU::fetchLowTileByte() {
	uint16_t fineY = (v >> 12) & 7;
	uint8_t table = PPUCTRL & BACKGROUND_TABLE;
	uint8_t tile = nameTableByte;
	uint16_t address = 0x1000 * ((uint16_t)table) + ((uint16_t)tile) * 16 + fineY;
	lowTileByte = cpu->read_memory(address);
}

void PPU::fetchHighTileByte() {
	uint16_t fineY = (v >> 12) & 7;
	uint8_t table = PPUCTRL & BACKGROUND_TABLE;
	uint8_t tile = nameTableByte;
	uint16_t address = 0x1000 * ((uint16_t)table) + ((uint16_t)tile) * 16 + fineY;
	highTileByte = cpu->read_memory(address + 8);
}

void PPU::storeTileData() {
	uint32_t data = 0;
	for (int i = 0; i < 8; i++) {
		uint8_t a = attributeTableByte;
		uint8_t p1 = (lowTileByte & 0x80) >> 7;
		uint8_t p2 = (highTileByte & 0x80) >> 6;
		lowTileByte <<= 1;
		highTileByte <<= 1;
		data <<= 4;
		data |= ((uint32_t)(a | p1 | p2));
	}
	tileData |= ((uint64_t)data);
}

uint32_t PPU::fetchTileData() {
	return ((uint32_t)(tileData >> 32));
}

uint8_t PPU::backgroundPixel() {
	if ((PPUMASK & SHOW_BACKGROUND) == 0) {
		return 0;
	}
	uint32_t data = fetchTileData() >> ((7 - x) * 4);
	return (uint8_t)(data & 0x0F);
}

uint16_t PPU::spritePixel() {
	if ((PPUMASK & SHOW_SPRITES) == 0) {
		return (uint16_t)0;
	}
	for (int i = 0; i < spriteCount; i++) {
		uint32_t offset = (cycle - 1) - (uint32_t)spritePositions[i];
		if (offset < 0 || offset > 7) {
			continue;
		}
		offset = 7 - offset;
		uint8_t color = (uint8_t)(spritePatterns[i] >> ((uint8_t)offset * 4) & 0x0F);
		if (color % 4 == 0) {
			continue;
		}
		return (i << 8) | color;
	}
	return (uint16_t)0;
}

void PPU::renderPixel() {
	uint32_t x = cycle - 1;
	uint32_t y = scanline;
	uint8_t background = backgroundPixel();
	uint16_t temp = spritePixel();
	uint8_t i = (temp & 0xF0) >> 8;
	uint8_t sprite = temp & 0x0F;
	if (x < 8 && PPUMASK & SHOW_LEFT_BACKGROUND) {
		background = 0;
	}
	if (x < 8 && PPUMASK & SHOW_LEFT_SPRITES) {
		sprite = 0;
	}
	bool b = background % 4 != 0;
	bool s = sprite % 4 != 0;
	uint8_t color;
	if (!b && !s) {
		color = 0;
	}
	else if (!b && s) {
		color = sprite | 0x10;
	}
	else if (b && !s) {
		color = background;
	}
	else {
		if (spriteIndexes[i] == 0 && x < 255) {
			PPUSTATUS |= SPRITE_ZERO_HIT;
		}
		if (spritePriorities[i] == 0) {
			color = sprite | 0x10;
		}
		else {
			color = background;
		}
	}
	color_t c = NES_PALETTE[readPalette((uint16_t)color) % 64];
	drawPixel(x, y, c);
}

uint32_t PPU::fetchSpritePattern(int col, int row) {
	uint8_t tile = oamData[col * 4 + 1];
	uint8_t attributes = oamData[col * 4 + 2];
	uint16_t address;
	uint8_t table;
	if (PPUCTRL & SPRITE_SIZE == 0) {
		if (attributes & 0x80 != 0) {
			row = 7 - row;
		}
		table = PPUCTRL & SPRITE_TABLE != 0;
		address = 0x1000 * ((uint16_t)table) + ((uint16_t)tile) * 16 + ((uint16_t)row);
	}
	else {
		if (attributes & 0x80 != 0) {
			row = 15 - row;
		}
		table = tile & 0x01;
		tile &= 0xFE;
		if (row > 7) {
			tile++;
			row -= 8;
		}
		address = 0x1000 * ((uint16_t)table) + ((uint16_t)tile) * 16 + ((uint16_t)row);
	}
	uint8_t a = (attributes & 0x03) << 2;
	lowTileByte = cpu->read_memory(address);
	highTileByte = cpu->read_memory(address + 8);
	uint32_t data = 0;
	for (int i = 0; i < 8; i++) {
		uint8_t p1, p2;
		if (attributes & 0x40 == 0x40) {
			p1 = (lowTileByte & 0x01) << 0;
			p2 = (highTileByte & 0x01) << 1;
			lowTileByte >>= 1;
			highTileByte >>= 1;
		}
		else {
			p1 = (lowTileByte & 0x80) >> 7;
			p2 = (highTileByte & 0x80) >> 6;
			lowTileByte <<= 1;
			highTileByte <<= 1;
		}
		data <<= 4;
		data |= (uint32_t)(a | p1 | p2);
	}
	return data;
}

void PPU::evaluateSprites() {
	uint32_t h;
	if (PPUCTRL & SPRITE_SIZE == 0) {
		h = 8;
	}
	else {
		h = 16;
	}
	uint32_t count = 0;
	for (int i = 0; i < 64; i++) {
		uint8_t y = oamData[i * 4 + 0];
		uint8_t a = oamData[i * 4 + 1];
		uint8_t x = oamData[i * 4 + 2];
		int row = scanline - (uint32_t)y;
		if (row < 0 || row >= h) {
			continue;
		}
		if (count < 8) {
			spritePatterns[count] = fetchSpritePattern(i, row);
			spritePositions[count] = x;
			spritePriorities[count] = (a >> 5) & 1;
			spriteIndexes[count] = (uint8_t)i;
		}
		count++;
	}
	if (count > 8) {
		count = 8;
		PPUSTATUS |= SPRITE_OVERFLOW;
	}
	spriteCount = count;
}




void PPU::tick() {
	if (nmiDelay > 0) {
		nmiDelay--;
		if (nmiDelay == 0 && nmiOutput && nmiOccurred) {
			cpu->interrupt = _NMI;
		}
	}

	if ((PPUMASK & SHOW_BACKGROUND != 0) || (PPUMASK & SHOW_SPRITES != 0)) {
		if (f == 1 && scanline == 261 && cycle == 339) {
			cycle = 0;
			scanline = 0;
			frame++;
			f ^= 1;
			return;
		}
	}
	cycle++;
	if (cycle > 340) {
		cycle = 0;
		scanline++;
		if (scanline > 261) {
			scanline = 0;
			frame++;
			f ^= 1;
		}
	}
}

void PPU::Step() {
	tick();

	bool renderingEnabled = (PPUMASK & SHOW_BACKGROUND != 0) || (PPUMASK & SHOW_SPRITES != 0);
	bool preLine = scanline == 261;
	bool visibleLine = scanline < 240;
	bool renderLine = preLine || visibleLine;
	bool preFetchCycle = (cycle >= 321 && cycle <= 336);
	bool visibleCycle = cycle >= 1 && cycle <= 256;
	bool fetchCycle = preFetchCycle || visibleCycle;

	if (renderingEnabled) {
		if (visibleLine && visibleCycle) {
			renderPixel();
		}
		if (renderLine && fetchCycle) {
			tileData <<= 4;
			switch (cycle % 8) {
			case 1:
				fetchNameTableByte();
				break;
			case 3:
				fetchAttributeTableByte();
				break;
			case 5:
				fetchLowTileByte();
				break;
			case 7:
				fetchHighTileByte();
				break;
			case 0:
				storeTileData();
				break;
			}
		}
		if (preLine && cycle >= 280 && cycle <= 304) {
			copyY();
		}
		if (renderLine) {
			if (fetchCycle && (cycle % 8 == 0)) {
				incrementX();
			}
			if (cycle == 256) {
				incrementY();
			}
			if (cycle == 257) {
				copyX();
			}
		}
	}

	if (renderingEnabled) {
		if (cycle == 257) {
			if (visibleLine) {
				evaluateSprites();
			}
			else {
				spriteCount = 0;
			}
		}
	}

	if (scanline == 241 && cycle == 1) {
		setVerticalBlank();
	}

	if (preLine && cycle == 1) {
		clearVerticalBlank();
		PPUSTATUS &= ~SPRITE_ZERO_HIT;
		PPUSTATUS &= ~SPRITE_OVERFLOW;
	}
}


void PPU::drawPixel(uint32_t x, uint32_t y, color_t color) {
	back[y*SCREEN_WIDTH + x] = color;
}