#pragma once
#include "Mapper.h"
#include "Cartridge.h"
class Mapper2 :
	public Mapper
{
public:
	Mapper2(iNesRom *rom);
	~Mapper2();

	uint8_t read(uint16_t address);
	void write(uint16_t address, uint8_t word);
};

