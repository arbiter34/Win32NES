#pragma once
#include "Mapper.h"
#include "Cartridge.h"
class Mapper1 :
	public Mapper
{
public:
	Mapper1(iNesRom *rom);
	~Mapper1();

	uint8_t read(uint16_t address);
	void write(uint16_t address, uint8_t word);
};

