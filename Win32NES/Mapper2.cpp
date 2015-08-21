#include "stdafx.h"
#include "Mapper2.h"


Mapper2::Mapper2(iNesRom *rom) : Mapper(rom)
{
}


Mapper2::~Mapper2()
{
}

uint8_t Mapper2::read(uint16_t address) {
	return 0;
}
void Mapper2::write(uint16_t address, uint8_t word) {

}