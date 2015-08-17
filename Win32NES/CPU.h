#ifndef CPU_H
#define CPU_H

#pragma once

#include <stdint.h>
#include "PPU.h"
#include "Controller.h"
#include "Cartridge.h"

#define DEBUG 1
#define DEBUG_LOG 1

void __cdecl odprintf(const char *format, ...);

class CPU
{
	friend class PPU;
	typedef void(CPU::*instructionPointer)(void);
	typedef void(CPU::*addressingModePointer)(void);	//Not necessary, here for clarity

public:

	uint32_t cycleCount;
	/* Interrupt */
	Interrupts interrupt;

	CPU(PPU *ppu, Controller *controller, Cartridge *cartridge);
	~CPU();

#pragma region CPU Ops

	void execute();
	short fetch();
	void reset();

#pragma endregion

protected:
	int stall;

#pragma region Memory Ops

	uint8_t read_memory(uint16_t address);
	void store_memory(uint16_t address, uint8_t word);
	uint16_t read_address(uint16_t address);
	uint16_t read_address_bug(uint16_t address);
	uint16_t relative_address(uint16_t address, uint8_t offset);

#pragma endregion

private:

#pragma region Vars

	/* Devices */
	PPU *ppu;
	Controller *controller;
	Cartridge *cartridge;

	/*Function Tables*/
	instructionPointer cpuTable[0x100];
	addressingModePointer addressingModeTable[0x100];
	static const uint8_t instructionCycles[0x100];
	static const uint8_t instructionPageCycles[0x100];

	/*Registers*/
	uint16_t pc;     // program counter, 16 bits
	uint8_t sp;       // stack pointer
	uint8_t p;       // status flags
	uint8_t a;       // accumulator
	uint8_t x;       // register x
	uint8_t y;       // register y

	/*Memory*/
	uint8_t *cartridge_expansion_rom;
	uint8_t *prg_rom;
	uint8_t *cpu_ram;
	uint8_t *sram;

	/*Misc*/
	uint8_t opcode;
	uint16_t address;
	uint8_t src;
	uint16_t temp;
	bool pageCrossed;

#pragma endregion

#pragma region Init

	 void initAddressingModeTable();
	 void initCPUTable();

#pragma endregion

#pragma region Interrupts

	 void NMI();
	 void IRQ();

#pragma endregion

#pragma region Get/Set Bits
	 void set_bit(uint8_t bit, bool on);
	 bool get_bit(uint8_t bit);

	 void set_carry(uint8_t value);
	 bool get_carry();

	 void set_zero(uint8_t value);
	 bool get_zero();

	 void set_irq_disable(uint8_t value);
	 bool get_irq_disable();

	 void set_decimal(uint8_t value);
	 bool get_decimal();

	 void set_break(uint8_t value);
	 bool get_break();

	 void set_overflow(uint8_t value);
	 bool get_overflow();

	 void set_negative(uint8_t value);
	 bool get_negative();

	 void cmp_bit_helper(uint8_t reg, uint8_t mem);
#pragma endregion

#pragma region Misc

	 bool pageDiff(uint16_t src, uint16_t dst);
	 void addBranchCycles(uint16_t src, uint16_t dst);

#pragma endregion

#pragma region Stack Ops

	 void push(uint8_t value);
	 uint8_t pop();

#pragma endregion

#pragma region Addressing Modes

	/* Addressing Mode*/
	 void Absolute();
	 void AbsoluteX();
	 void AbsoluteY();
	 void Accumulator();
	 void Immediate();
	 void Implicit();
	 void Indirect();
	 void IndirectX();
	 void IndirectY();
	 void Relative();
	 void ZeroPage();
	 void ZeroPageX();
	 void ZeroPageY();

#pragma endregion
	 
#pragma region Instruction Set

	/* Instruction Set*/
	 void cpuNULL();
	 void ADC();
	 void AND();
	 void ASL();
	 void BCC();
	 void BCS();
	 void BEQ();
	 void BIT();
	 void BRK();
	 void BMI();
	 void BNE();
	 void BPL();
	 void BVC();
	 void BVS();
	 void CLC();
	 void CLD();
	 void CLI();
	 void CLV();
	 void CMP();
	 void CPX();
	 void CPY();
	 void DEC();
	 void DEX();
	 void DEY();
	 void EOR();
	 void INC();
	 void INX();
	 void INY();
	 void JMP();
	 void JSR();
	 void LDA();
	 void LDX();
	 void LDY();
	 void LSR();
	 void NOP();
	 void ORA();
	 void PHA();
	 void PHP();
	 void PLA();
	 void PLP();
	 void ROL();
	 void ROR();
	 void RTI();
	 void RTS();
	 void SBC();
	 void SEC();
	 void SED();
	 void SEI();
	 void STA();
	 void STX();
	 void STY();
	 void TAX();
	 void TAY();
	 void TSX();
	 void TXA();
	 void TXS();
	 void TYA();

#pragma endregion

};

#endif

