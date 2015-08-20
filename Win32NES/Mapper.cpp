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
		if (address < rom->header.chr_rom_pages * CHR_ROM_PAGE_SIZE) {
			return rom->chr_rom[address];
		}
		else {
			return 0;
		}
	}
	else {
		return rom->prg_rom[address & ((0x4000 * prg_banks) - 1)];
	}
}


void Mapper::write(uint16_t address, uint8_t word)
{
	if (address < 0x2000) {
		if (address < rom->header.chr_rom_pages * CHR_ROM_PAGE_SIZE) {
			rom->chr_rom[address] = word;
		}
	}
	else {
		rom->prg_rom[address & (0x7FFF / prg_banks)] = word;
	}
}
