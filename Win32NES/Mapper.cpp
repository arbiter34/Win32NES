#include "stdafx.h"
#include "Mapper.h"


Mapper::Mapper(iNesRom *rom)
{
	this->rom = rom;
	prg_banks = rom->header.prg_rom_pages;
	chr_banks = rom->header.chr_rom_pages;
}


Mapper::~Mapper()
{
}


uint8_t Mapper::read(uint16_t address)
{
	if (address < 0x2000) {
		return rom->chr_rom[address];
	}
	else {
		return rom->prg_rom[address & ((0x4000 * prg_banks) - 1)];
	}
}


void Mapper::write(uint16_t address, uint8_t word)
{
	if (address < 0x2000) {
		rom->chr_rom[address] = word;
	}
	else {
		rom->prg_rom[address & (0x7FFF / prg_banks)] = word;
	}
}
