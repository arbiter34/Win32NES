#include "stdafx.h"
#include "PPU.h"


PPU::PPU()
{
	reset();
	front = new color_t[SCREEN_WIDTH * SCREEN_HEIGHT];
	back = new color_t[SCREEN_WIDTH * SCREEN_HEIGHT];
	screen = front;
}


PPU::~PPU()
{
}

void PPU::setVar(Cartridge *cartridge, CPU *cpu) {
	this->cartridge = cartridge;
	this->cpu = cpu;
}

void PPU::reset() {
	cycle = 340;
	scanline = 240;
	frame = 0;
	writeControl(0);
	writeMask(0);
	writeOAMAddress(0);
	spriteCount = 0;
}

uint16_t MirrorLookup[5][4] = {
	{ 0, 0, 1, 1 },
	{ 0, 1, 0, 1 },
	{ 0, 0, 0, 0 },
	{ 1, 1, 1, 1 },
	{ 0, 1, 2, 3 }
};

uint16_t MirrorAddress(uint8_t mode, uint16_t address) {
	address = (address - 0x2000) % 0x1000;
	uint8_t table = address / 0x0400;
	uint16_t offset = address % 0x0400;
	return 0x2000 + MirrorLookup[mode][table] * 0x0400 + offset;
}

uint8_t PPU::read(uint16_t address) {
	address = address & 0x3FFF;
	if (address < 0x2000) {
		return cartridge->read(address);
	}
	else if (address < 0x3F00) {
		uint8_t mode = cartridge->mirror;
		return nameTableData[MirrorAddress(mode, address) & 0x7FF];
	}
	else if (address < 0x4000) {
		return readPalette(address & 0x1F);
	}
	else {
		odprintf("FUCK");
	}
}

void PPU::write(uint16_t address, uint8_t word) {
	address = address & 0x3FFF;
	if (address < 0x2000) {
		cartridge->write(address, word);
	}
	else if (address < 0x3F00) {
		uint8_t mode = cartridge->mirror;
		nameTableData[MirrorAddress(mode, address) & 0x7FF] = word;
	}
	else if (address < 0x4000) {
		writePalette(address & 0x1F, word);
	}
	else {
		odprintf("FUCK");
	}
}


uint8_t PPU::readPalette(uint16_t address) {
	if (address >= 16 && (address % 4) == 0) {
		address -= 16;
	}
	return paletteData[address];
}

void PPU::writePalette(uint16_t address, uint8_t word) {
	if (address >= 16 && (address % 4) == 0) {
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
	uint8_t result = (reg & 0x1F);
	result |= (PPUSTATUS & SPRITE_OVERFLOW);
	result |= (PPUSTATUS & SPRITE_ZERO_HIT);
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
	if (cycle > 0 && cycle <= 64) {
		return 0xFF;
	}
	return oamData[OAMADDR];
}

void PPU::writeOAMData(uint8_t word) {
	oamData[OAMADDR] = word;
	OAMADDR++;
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
	uint16_t word = read(v);

	// emulate buffered reads
	if ((v & 0x3FFF) < 0x3F00) {
		uint8_t temp = PPUDATA;
		PPUDATA = word;
		word = temp;
	}
	else {
		PPUDATA = read(v - 0x1000);
	}

	// increment address
	if ((PPUCTRL & INCREMENT) == 0) {
		v += 1;
	}
	else {
		v += 32;
	}
	return word;
}

void PPU::writeData(uint8_t word) {
	write(v, word);
	if ((PPUCTRL & INCREMENT) == 0) {
		v += 1;
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
	if ((v & 0x001F) == 0x1F) {
		v &= 0xFFE0;
		v ^= 0x0400;
	}
	else {
		v++;
	}
}

void PPU::incrementY() {
	if ((v & 0x7000) != 0x7000) {
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
	bool nmi = ((PPUCTRL & NMI_OUTPUT) != 0) && nmiOccurred;
	if (nmi && !nmiPrevious) {
		 // TODO: this fixes some games but the delay shouldn't have to be so
		 // long, so the timings are off somewhere
		nmiDelay = 1;
	}
	nmiPrevious = nmi;
}

void PPU::setVerticalBlank() {
	screen = back;
	back = front;
	front = screen;
	nmiOccurred = true;
	nmiChange();
}

void PPU::clearVerticalBlank() {
	nmiOccurred = false;
	nmiChange();
}

void PPU::fetchNameTableByte() {
	uint16_t temp_v = this->v;
	uint16_t address = 0x2000 | (temp_v & 0x0FFF);
	nameTableByte = read(address);
}

void PPU::fetchAttributeTableByte() {
	uint16_t temp_v = this->v;
	uint16_t address = 0x23C0 | (temp_v & 0x0C00) | ((temp_v >> 4) & 0x38) | ((temp_v >> 2) & 0x07);
	uint16_t shift = ((temp_v >> 4) & 4) | (temp_v & 2);
	attributeTableByte = ((read(address) >> shift) & 3) << 2;
}

void PPU::fetchLowTileByte() {
	uint16_t fineY = (v >> 12) & 7;
	uint8_t table = (PPUCTRL & BACKGROUND_TABLE) >> 4;
	uint8_t tile = nameTableByte;
	uint16_t address = 0x1000 * ((uint16_t)table) + ((uint16_t)tile) * 16 + fineY;
	lowTileByte = read(address);
}

void PPU::fetchHighTileByte() {
	uint16_t fineY = (v >> 12) & 7;
	uint8_t table = (PPUCTRL & BACKGROUND_TABLE) >> 4;
	uint8_t tile = nameTableByte;
	uint16_t address = 0x1000 * ((uint16_t)table) + ((uint16_t)tile) * 16 + fineY;
	highTileByte = read(address + 8);
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
	//check show sprites
	if ((PPUMASK & SHOW_SPRITES) == 0) {
		return (uint16_t)0;
	}
	//iterate over found sprites on current scanline
	for (int i = 0; i < spriteCount; i++) {
		if (i == 1) {
			i = i;
		}
		//check whether left of sprite is current x or in previous 7 pixels
		uint32_t offset = (cycle - 1) - (secondaryOAM[i * 4 + 3]);
		if (offset < 0 || offset >= 8) {
			continue;
		}
		//Correct offset for x in sprite
		offset = 7 - offset;
		uint8_t color = (uint8_t)((spritePatterns[i] >> (offset * 4)) & 0x0F);
		if (color % 4 == 0) {
			continue;
		}
		//passing back sprite number and color(4 bits)
		return (i << 8) | color;
	}
	return (uint16_t)0;
}

void PPU::renderPixel() {
	uint32_t x = cycle - 1;
	uint32_t y = scanline;
	uint8_t background = backgroundPixel();
	uint16_t temp = spritePixel();
	uint8_t i = (temp & 0xFF00) >> 8;
	uint8_t sprite = temp &0x00FF;
	if (x < 8 && ((PPUMASK & SHOW_LEFT_BACKGROUND) == 0)) {
		background = 0;
	}
	if (x < 8 && ((PPUMASK & SHOW_LEFT_SPRITES) == 0)) {
		sprite = 0;
	}
	bool b = (background % 4) != 0;
	bool s = (sprite % 4) != 0;
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
		if (i == 0 && x < 255) {
			PPUSTATUS |= SPRITE_ZERO_HIT;
		}
		if (((secondaryOAM[i * 4 + 2] >> 5) & 0x01) == 0) {
			color = sprite | 0x10;
		}
		else {
			color = background;
		}
	}
	color_t c = NES_PALETTE[readPalette((uint16_t)color) % 64];
	drawPixel(x, y, c);
}

uint32_t PPU::fetchSpritePattern(int spriteNum) {
	uint8_t y = secondaryOAM[spriteNum * 4 + 0];
	uint8_t tile = secondaryOAM[spriteNum * 4 + 1];
	uint8_t attributes = secondaryOAM[spriteNum * 4 + 2];
	uint16_t address;
	uint8_t table;

	int row = (int)((scanline + 1) % 261) - y;
	if ((PPUCTRL & SPRITE_SIZE) == 0) {
		//Check vertical mirror
		if ((attributes & 0x80) == 0x80) {
			row = 7 - row;
		}
		//Get sprite table
		table = (PPUCTRL & SPRITE_TABLE) >> 3;
		address = 0x1000 * ((uint16_t)table) + ((uint16_t)tile) * 16 + ((uint16_t)row);
	}
	else {
		//Check vertical mirror
		if ((attributes & 0x80) == 0x80) {
			row = 15 - row;
		}
		//Compute table
		table = tile & 0x01;
		//0 out the bank number(LSB)
		tile &= 0xFE;

		//Check whether we're in the second half of a tall sprite
		if (row > 7) {
			tile++;
			row -= 8;
		}
		address = 0x1000 * ((uint16_t)table) + ((uint16_t)tile) * 16 + ((uint16_t)row);
	}
	//palette of sprite
	uint8_t a = (attributes & 0x03) << 2;
	//get bit plane 0 of current row (bit 0 of color)
	lowTileByte = read(address);
	//get bit plane 1 of current row (bit 1 of color)
	highTileByte = read(address + 8);
	uint32_t data = 0;
	for (int i = 0; i < 8; i++) {
		uint8_t p1, p2;
		//Check mirror horizontal
		if ((attributes & 0x40) == 0x40) {
			//bit 0
			p1 = (lowTileByte & 0x01) << 0;
			//bit 1
			p2 = (highTileByte & 0x01) << 1;
			//shift tiles right
			lowTileByte >>= 1;
			highTileByte >>= 1;
		}
		//No h-mirror
		else {
			//bit 0
			p1 = (lowTileByte & 0x80) >> 7;
			//bit 1
			p2 = (highTileByte & 0x80) >> 6;
			//shift tiles left
			lowTileByte <<= 1;
			highTileByte <<= 1;
		}
		data <<= 4;
		data |= (uint32_t)(a | p1 | p2);
	}
	//return 8 pixel row 
	return data;
}

void PPU::evaluateSprites() {
	uint32_t h;
	static int n, m, value, oamIndex, tempSpriteCount;
	static bool overflow, found;

	//Secondary OAM Init
	if (cycle > 0 && cycle <= 64) {
		n = m = oamIndex = tempSpriteCount = 0;
		overflow = found = false;
	}

	if (n == 64) {
		n = 0;
		overflow = true;
	}

	//SpriteEvaluation
	if (cycle > 64 && cycle <= 256) {
		//read
		if (cycle % 2 == 1) {
			//read oamData[n][m]
			value = oamData[n * 4 + m];
			if (overflow) {
				n++;
				return;
			}
			if (!found) {
				h = (PPUCTRL & SPRITE_SIZE) != 0 ? 16 : 8;
				if (((int)((scanline + 1) % 261) - value) >= h || ((int)((scanline + 1) % 261) - value) < 0) {
					m = 0;
					n = (n + 1);
					return;
				}
				else {
					found = true;
				}
			}
		}
		//write
		else {
			if (overflow) {
				n++;
			}
			if (tempSpriteCount <= 8) {
				secondaryOAM[(tempSpriteCount) * 4 + (m)] = value;
				if (found) {
					m++;
				}
			}
			if (m == 4) {

				tempSpriteCount++;
				if (tempSpriteCount == 9) {
					PPUSTATUS |= SPRITE_OVERFLOW;
				}
				m = 0;
				n = (n+1);
				found = false;
			}
		}
	}

	//Sprite Fetching
	if (cycle == 339 && tempSpriteCount > 0) {
		for (int i = 0; i < tempSpriteCount; i++) {
			spritePatterns[i] = fetchSpritePattern(i);
		}
		spriteCount = tempSpriteCount > 8 ? 8 : tempSpriteCount;
	}
}




void PPU::tick() {
	if (cycle == 338) {
		spriteCount = 0;
	}
	if (nmiDelay > 0) {
		nmiDelay--;
		if (nmiDelay == 0 && ((PPUCTRL & NMI_OUTPUT) != 0) && nmiOccurred) {
			cpu->interrupt = _NMI;
		}
	}

	if (((PPUMASK & SHOW_BACKGROUND) != 0) || ((PPUMASK & SHOW_SPRITES) != 0)) {
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

	bool renderingEnabled = ((PPUMASK & SHOW_BACKGROUND) != 0) || ((PPUMASK & SHOW_SPRITES) != 0);
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
			if (fetchCycle && ((cycle % 8) == 0)) {
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


	if (scanline == 241 && cycle == 1) {
		setVerticalBlank();
	}

	if (preLine && cycle == 0) {
		clearVerticalBlank();
		PPUSTATUS &= ~SPRITE_ZERO_HIT;
		PPUSTATUS &= ~SPRITE_OVERFLOW;
	}

	if (renderingEnabled) {
		if (scanline < 239 || preLine) {
			evaluateSprites();
		}
	}

}


void PPU::drawPixel(uint32_t x, uint32_t y, color_t color) {
	back[y*SCREEN_WIDTH + x] = color;
}