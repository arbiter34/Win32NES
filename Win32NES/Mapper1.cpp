#include "stdafx.h"
#include "Mapper1.h"


Mapper1::Mapper1(iNesRom *rom) : Mapper(rom)
{

}


Mapper1::~Mapper1()
{
}

uint8_t Mapper1::read(uint16_t address)
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


void Mapper1::write(uint16_t address, uint8_t word)
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
