#pragma once
#include <stdint.h>
#include "defines.h"
#include "Cartridge.h"
class Cartridge;
typedef struct iNesRom;
class Mapper
{
public:
	Mapper(iNesRom *rom);
	~Mapper();

	virtual uint8_t read(uint16_t address) = 0;
	virtual void write(uint16_t address, uint8_t word) = 0;

protected:
	iNesRom *rom;
	uint8_t prg_banks;
	uint8_t chr_banks;
};

