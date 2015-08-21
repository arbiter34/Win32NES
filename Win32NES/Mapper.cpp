#include "stdafx.h"
#include "Mapper.h"


Mapper::Mapper(iNesRom *rom)
{
	this->rom = rom;
	prg_banks = rom->header.prg_rom_pages / 4;
	chr_banks = rom->header.chr_rom_pages;
}


Mapper::~Mapper()
{
}

