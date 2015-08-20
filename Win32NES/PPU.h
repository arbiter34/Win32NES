#pragma once
#include <stdint.h>
#include "defines.h"
#include "Palette.h"
#include "CPU.h"

//PPUCTRL Bitmasks
#define NAME_TABLE 0x3
#define INCREMENT 0x4
#define SPRITE_TABLE 0x8
#define BACKGROUND_TABLE 0x10
#define SPRITE_SIZE 0x20
#define MASTER_SLAVE 0x40
#define NMI_OUTPUT 0x80

//PPUMASK Bitmasks
#define GRAYSCALE 0x01
#define SHOW_LEFT_BACKGROUND 0x02
#define SHOW_LEFT_SPRITES 0x04
#define SHOW_BACKGROUND 0x08
#define SHOW_SPRITES 0x10
#define RED_TINT 0x20
#define GREEN_TINT 0x40
#define BLUE_TINT 0x80

//PPUSTATUS Bitmasks
#define SPRITE_OVERFLOW 0x20
#define SPRITE_ZERO_HIT 0x40
#define VERTICAL_BLANK 0x80

class CPU;
class Cartridge;

class PPU
{	
public:

	PPU();
	~PPU();

	color_t *screen;

	void setVar(Cartridge *cartridge, CPU *cpu);
	void reset();


	uint32_t cycle;
	uint32_t scanline;
	uint64_t frame;


	void tick();
	void Step();

	uint8_t read(uint16_t address);
	void write(uint16_t address, uint8_t word);
	uint8_t readPalette(uint16_t address);
	void writePalette(uint16_t address, uint8_t word);
	uint8_t readRegister(uint16_t address);
	void writeRegister(uint16_t address, uint8_t word);
	void writeControl(uint8_t word);
	void writeMask(uint8_t word);
	uint8_t readStatus();
	void writeOAMAddress(uint8_t word);
	uint8_t readOAMData();
	void writeOAMData(uint8_t word);
	void writeScroll(uint8_t word);
	void writeAddress(uint8_t word);
	uint8_t readData();
	void writeData(uint8_t word);
	void writeDMA(uint8_t word);

private:

#pragma region Vars
	Cartridge *cartridge;
	CPU *cpu;

	color_t *front;
	color_t *back;

	uint8_t paletteData[32];
	uint8_t nameTableData[2048];
	uint8_t oamData[256];
	uint8_t secondaryOAM[32];

	uint16_t v;		//current vram address
	uint16_t t;		//temp vram address
	uint8_t x;		//fine x scroll
	uint8_t w;		//write toggle
	uint8_t f;		//even/odd frame flag

	uint8_t reg;

	//NMI Flags
	bool nmiOccurred;
	bool nmiPrevious;
	uint8_t nmiDelay;

	// background temporary variables
	uint8_t nameTableByte;
	uint8_t attributeTableByte;
	uint8_t lowTileByte;
	uint8_t highTileByte;
	uint64_t tileData;

	// sprite temporary variables
	uint32_t spriteCount;
	uint32_t spritePatterns[8];
	uint8_t spritePositions[8];
	uint8_t spritePriorities[8];
	uint8_t spriteIndexes[8];

	uint8_t PPUCTRL;
	uint8_t PPUMASK;
	uint8_t PPUSTATUS;
	uint8_t OAMADDR;
	uint8_t PPUDATA;

#pragma endregion

	void drawPixel(uint32_t x, uint32_t y, color_t color);

	void incrementX();
	void incrementY();
	void copyX();
	void copyY();
	void nmiChange();
	void setVerticalBlank();
	void clearVerticalBlank();
	void fetchNameTableByte();
	void fetchAttributeTableByte();
	void fetchLowTileByte();
	void fetchHighTileByte();
	void storeTileData();
	uint32_t fetchTileData();
	uint8_t backgroundPixel();
	uint16_t spritePixel();
	void renderPixel();
	uint32_t fetchSpritePattern(int spriteNum);
	void evaluateSprites();
};

