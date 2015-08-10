#include "stdafx.h"
#include "Cartridge.h"
#include <iostream>
#include <string>


Cartridge::Cartridge()
{
}


Cartridge::~Cartridge()
{
}

bool Cartridge::loadRom(PWSTR filePath) {
	rom = new iNesRom;

	FILE *in;
	long size;
	size_t res;

	res = _wfopen_s(&in, filePath, L"rb");
	if (res != 0) {
		return false;
	}

	fseek(in, 0, SEEK_END);
	size = ftell(in);
	rewind(in);

	res = fread(&(rom->header), 1, sizeof(rom->header), in);
	if (res != sizeof(rom->header)) {
		return false;
	}

	int prg_rom_size = rom->header.prg_rom_pages * PRG_ROM_PAGE_SIZE;
	rom->prg_rom = new uint8_t[prg_rom_size];
	printf("0x%04\n", rom->prg_rom[0]);
	res = fread(rom->prg_rom, 1, prg_rom_size, in);
	if (res != prg_rom_size) {
		return false;
	}

	int chr_rom_size = rom->header.chr_rom_pages * CHR_ROM_PAGE_SIZE;
	rom->chr_rom = new uint8_t[chr_rom_size];
	res = fread(rom->chr_rom, 1, chr_rom_size, in);
	if (res != chr_rom_size) {
		return false;
	}

	int prg_ram_size = rom->header.prg_ram_pages * PRG_RAM_PAGE_SIZE;
	rom->prg_ram = new uint8_t[prg_ram_size];
	fread(rom->prg_ram, 1, prg_ram_size, in);

	printf("0x%04\n", rom->prg_rom[0]);
	//TO-DO: Pick correct mapper
	mapper = new Mapper(rom);

	fclose(in);

	return true;
}

uint8_t Cartridge::read(uint16_t address) {
	return mapper->read(address);
}

void Cartridge::write(uint16_t address, uint8_t word) {
	mapper->write(address, word);
}
