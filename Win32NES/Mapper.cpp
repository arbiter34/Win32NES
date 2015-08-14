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
	return rom->prg_rom[address & ((0x4000 * prg_banks) - 1)];
}


void Mapper::write(uint16_t address, uint8_t word)
{
	rom->prg_rom[address & (0x7FFF / prg_banks)] = word;
}
