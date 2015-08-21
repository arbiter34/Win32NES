#include "stdafx.h"
#include "MapperFactory.h"
#include "Mapper1.h"
#include "Mapper2.h"


MapperFactory::MapperFactory()
{
}


MapperFactory::~MapperFactory()
{
}


Mapper* MapperFactory::getMapper(iNesRom *rom)
{
	uint8_t mapper = (rom->header.flags7 & 0xF0) | ((rom->header.flags6 & 0xF0) >> 4);
	switch (mapper) {
		case 0:
			return new Mapper1(rom);
		case 2:
			return new Mapper1(rom);
		default:
			return NULL;
	}
	return NULL;
}
