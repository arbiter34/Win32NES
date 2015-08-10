#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#pragma once
#include <bitset>
#include <fstream>
#include <iostream>
#include "defines.h"
#include <stdint.h>
#include "Mapper.h"

class Mapper;

typedef struct iNesHeader {
	uint8_t magicBytes[4];
	uint8_t prg_rom_pages;
	uint8_t chr_rom_pages;
	uint8_t flags6;
	uint8_t flags7;
	uint8_t prg_ram_pages;
	uint8_t flags9;
	uint8_t flags10;
	uint8_t fill[5];
};

typedef struct iNesRom {
	iNesHeader header;
	uint8_t *prg_rom;
	uint8_t *chr_rom;
	uint8_t *prg_ram;
};

class Cartridge
{
public:
	Cartridge();
	~Cartridge();

	iNesRom *rom;

	bool loadRom(PWSTR filePath);

	uint8_t read(uint16_t address);
	void write(uint16_t address, uint8_t word);

private:
	Mapper *mapper;

	bool mirror_prg_rom;
};

#endif

