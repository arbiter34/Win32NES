#pragma once
#include "Mapper.h"
#include "Cartridge.h"

class Mapper;
class MapperFactory
{
public:
	MapperFactory();
	~MapperFactory();
	static Mapper* getMapper(iNesRom *rom);
};

