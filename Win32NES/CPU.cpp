#include "stdafx.h"
#include "CPU.h"

CPU::CPU(PPU *ppu, Controller *controller, Cartridge *cartridge)
{
	this->ppu = ppu;
	this->controller = controller;
	this->cartridge = cartridge;
	prg_rom = NULL;
	cpu_ram = new uint8_t[CPU_RAM_SIZE];
	sram = new uint8_t[SRAM_SIZE];
	cartridge_expansion_rom = new uint8_t[0x6000 - 0x4020];

	for (int i = 0; i < CPU_RAM_SIZE; i++) {
		cpu_ram[i] = 0;
	}
	
	for (int i = 0; i < SRAM_SIZE; i++) {
		sram[i] = 0;
	}

	initAddressingModeTable();
	initCPUTable();
}


CPU::~CPU()
{
}

#pragma region CPU Ops

void CPU::reset() {
	pc = read_address(0xFFFC);    // address to jump to after reset
	cycleCount = 0;
	sp = 0xFD;
	a = 0;
	p = 0x34;
	x = 0;
	y = 0;
}

void CPU::execute()
{
	if (pc == 0xE2F9) {		//BNE @wrong
		pc = pc;
	}
	if (pc == 0xE2D4) {
		pc = pc;
	}
	if (pc == 0xE2D7) {
		pc = pc;
	}
	if (stall > 0) {
		stall--;
		return;
	}

	if (interrupt == _NMI) {
		NMI();
	}
	if (interrupt == _IRQ) {
		IRQ();
	}
	interrupt = _None;

	//Fetch
	opcode = fetch();

	//Addressing Mode Decode
	(this->*addressingModeTable[(opcode)])();

	//Instruction Decode
	(this->*cpuTable[(opcode)])();
	cycleCount += instructionCycles[opcode];
	if (pageCrossed) {
		cycleCount += instructionPageCycles[opcode];
	}
}


short CPU::fetch()
{
	return read_memory(pc);
}

/* NOP */
void CPU::cpuNULL() {

}

#pragma endregion

#pragma region Interrupts

void CPU::NMI() {
	push(pc >> 8);
	push(pc);
	PHP();
	pc = read_address(0xFFFA);
	set_irq_disable(1);
	cycleCount += 7;
}

void CPU::IRQ() {
	push(pc >> 8);
	push(pc);
	PHP();
	pc = read_address(0xFFFE);
	set_irq_disable(1);
	cycleCount += 7;
}

#pragma endregion

#pragma region Get/Set Bits

void CPU::set_bit(uint8_t bit, bool on){
	on ? p |= bit : p &= ~bit;
}

bool CPU::get_bit(uint8_t bit){
	return (p & bit) != 0;
}

void CPU::set_carry(uint8_t value){
	set_bit(1 << 0, value);
}

bool CPU::get_carry() {
	return get_bit(1 << 0);
}

void CPU::set_zero(uint8_t value){
	set_bit(1 << 1, value == 0);
}

bool CPU::get_zero() {
	return get_bit((1 << 1));
}

void CPU::set_irq_disable(uint8_t value){
	set_bit(1 << 2, value);
}

bool CPU::get_irq_disable() {
	return get_bit(1 << 2);
}

void CPU::set_decimal(uint8_t value){
	set_bit(1 << 3, value);
}

bool CPU::get_decimal() {
	return get_bit(1 << 3);
}

void CPU::set_break(uint8_t value){
	set_bit(1 << 4, value);
}

bool CPU::get_break() {
	return get_bit(1 << 4);
}

void CPU::set_overflow(uint8_t value){
	set_bit(1 << 6, value);
}

bool CPU::get_overflow() {
	return get_bit(1 << 6);
}

void CPU::set_negative(uint8_t value){
	set_bit(1 << 7, value & 0x80);
}

bool CPU::get_negative() {
	return get_bit(1 << 7);
}

void CPU::cmp_bit_helper(uint8_t reg, uint8_t mem) {
	set_zero(reg - mem);
	if (reg >= mem) {
		set_carry(1);
	}
	else {
		set_carry(0);
	}
	set_negative(reg - mem);
}


#pragma endregion

#pragma region Debug Print

void printAddress(uint16_t pc, uint8_t opcode, uint16_t address, const char *mode, bool implied = false) {
#if defined(DEBUG) || defined(DEBUG_LOG)
	char buff[255];
	if ((address & 0xFF00) != 0) {
		sprintf(buff, "\r\n%-6.4X%-3.2X%-3.2X%-4.2X%-4s%s", pc, opcode, (address & 0xFF), ((address & 0xFF00) >> 8), CPU::opcode_names[opcode], mode);
	}
	else if (!implied) {
		sprintf(buff, "\r\n%-6.4X%-3.2X%-7.2X%-4s%s", pc, opcode, address, CPU::opcode_names[opcode], mode);
	}
	else {
		sprintf(buff, "\r\n%-6.4X%-3.2X%-7.0X%-4s%s", pc, opcode, address, CPU::opcode_names[opcode], mode);
	}
#endif

#ifdef DEBUG
	odprintf("%s", buff);
#endif

#ifdef DEBUG_LOG
	FILE *fp = fopen("output.log", "ab");
	if (fp != NULL) {
		fputs(buff, fp);
		fclose(fp);
	}
#endif
}

void printOpcode(const char* opcode, uint16_t address, uint8_t a, uint8_t x, uint8_t y, uint8_t p, uint8_t sp, uint16_t cycleCount) {
#if defined(DEBUG) || defined(DEBUG_LOG)
	char buff[255];
	sprintf(buff, "A:%-3.2XX:%-3.2XY:%-3.2XP:%-3.2XSP:%-2.2X", a, x, y, p, sp);
#endif

#ifdef DEBUG
	odprintf("%s", buff);
#endif

#ifdef DEBUG_LOG
	FILE *fp = fopen("output.log", "ab");
	if (fp != NULL) {
		fputs(buff, fp);
		fclose(fp);
	}
#endif
}

void __cdecl odprintf(const char *format, ...)
{
	char    buf[4096], *p = buf;
	va_list args;
	int     n;

	va_start(args, format);
	n = _vsnprintf(p, sizeof buf - 3, format, args); // buf-3 is room for CR/LF/NUL
	va_end(args);

	p += (n < 0) ? sizeof buf - 3 : n;

	//while (p > buf  &&  isspace(p[-1]))
	//	*--p = '\0';

	//*p++ = '\r';
	//*p++ = '\n';
	*p = '\0';

	OutputDebugStringA(buf);
}

#pragma endregion

#pragma region Memory Ops

uint8_t CPU::read_memory(uint16_t address){
	uint8_t word = 0;;
	/* 2KB of Working RAM*/
	if (address < 0x2000) {
		word = cpu_ram[address & 0x7FF];
	}
	/* PPU Ctrl Registers - Mirrored 1023 Times*/
	else if (address >= 0x2000 && address < 0x4000) {
		ppu->readRegister(0x2000 + (address & 0x07));
	}
	/* Registers (Mostly APU) */
	else if (address >= 0x4000 && address < 0x4020) {
		switch (address) {
			case 0x4014:
				return ppu->readRegister(address);
			case 0x4015:
				return 0;
			case 0x4016:
				return controller->read_controller_state(1);
			case 0x4017:
				return controller->read_controller_state(2);
			default:
				return 0;
		}
	}
	/* Cartridge Expansion ROM */
	else if (address >= 0x4020 && address < 0x6000) {
	
	}
	/* SRAM */
	else if (address >= 0x6000 && address < 0x8000) {
		word = sram[address - 0x6000];
	}
	/* PRG-ROM */
	else if (address >= 0x8000 && address < 0xFFFF) {
		word = cartridge->read(address - 0x8000);
	}
	return word;
}

void CPU::store_memory(uint16_t address, uint8_t word){
	if (address == 0) {
		address = address;
	}
	/* 2KB of Working RAM*/
	if (address < 0x2000) {
		cpu_ram[address & 0x7FF] = word;
	}
	/* PPU Ctrl Registers - Mirrored 1023 Times*/
	else if (address >= 0x2000 && address < 0x4000) {
		ppu->writeRegister(0x2000 + (address & 0x07), word);
	}
	/* Registers (Mostly APU) */
	else if (address >= 0x4000 && address < 0x4020) {
		switch (address) {
			case 0x4014:
				ppu->writeRegister(address, word);
				break;
			case 0x4016:
				controller->write_value(word);
				break;
			default:
				return;
		}
	}
	/* Cartridge Expansion ROM */
	else if (address >= 0x4020 && address < 0x6000) {
		cartridge_expansion_rom[address - 0x4020] = word;
	}
	/* SRAM */
	else if (address >= 0x6000 && address < 0x8000) {
		sram[address - 0x6000] = word;
	}
	/* PRG-ROM */
	else if (address >= 0x8000 && address < 0xFFFF) {
		cartridge->write(address - 0x8000, word);
	}
}

uint16_t CPU::read_address(uint16_t address){
	return (uint16_t)(read_memory(address + 1) << 8) + read_memory(address);
}

uint16_t CPU::read_address_bug(uint16_t address) {
	uint16_t first_byte = address;
	uint16_t second_byte = (address & 0xFF00) | (uint8_t)(first_byte + 1);
	return (uint16_t)(read_memory(second_byte) << 8) + read_memory(first_byte);
}

uint16_t CPU::relative_address(uint16_t address, uint8_t offset) {
	if ((offset & 0x80) != 0) {
		return address - (uint8_t)(~offset + 1);
	}
	else {
		return address + offset;
	}
}

#pragma endregion

#pragma region Misc

bool CPU::pageDiff(uint16_t src, uint16_t dst) {
	return (src & 0xFF00) != (dst & 0xFF00);
}
void CPU::addBranchCycles(uint16_t src, uint16_t dst) {
	cycleCount++;
	if (pageDiff(src, dst)) {
		cycleCount++;
	}
}

#pragma endregion

#pragma region Stack Ops

void CPU::push(uint8_t value){
	cpu_ram[0x100|(uint16_t)sp] = value;
	sp--;
}

uint8_t CPU::pop(){
	sp++;
	return cpu_ram[0x100 | (uint16_t)sp];
}

#pragma endregion

#pragma region Addressing Modes

void CPU::Absolute(){
	address = read_address(pc + 1);
	char buff[100];
	sprintf(buff, "$%-27.4X", address);
	printAddress(pc, opcode, address, buff, false);
	printOpcode("", address, a, x, y, p, sp, cycleCount);
	pc += 3;
}
void CPU::AbsoluteX(){
	uint16_t temp_address = read_address(pc + 1);
	address = temp_address + x;
	char buff[100];
	sprintf(buff, "$%-4.4X,X%21s", temp_address, "");
	printAddress(pc, opcode, temp_address, buff, false);
	printOpcode("", address, a, x, y, p, sp, cycleCount);
	pageCrossed = pageDiff(address - (uint16_t)x, address);
	pc += 3;
}
void CPU::AbsoluteY(){
	uint16_t temp_address = read_address(pc + 1);
	address = temp_address + y;
	char buff[100];
	sprintf(buff, "$%-4.4X,Y%21s", temp_address, "");
	printAddress(pc, opcode, temp_address, buff, false);
	printOpcode("", address, a, x, y, p, sp, cycleCount);
	pageCrossed = pageDiff(address - (uint16_t)y, address);
	pc += 3;
}
void CPU::Accumulator(){
	address = 0;
	src = a;
	char buff[100];
	sprintf(buff, "A%27s", "");
	printAddress(pc, opcode, 0, buff, true);
	printOpcode("", address, a, x, y, p, sp, cycleCount);
	pc += 1;
}
void CPU::Immediate(){
	address = pc + 1;
	char buff[100];
	sprintf(buff, "#$%-26.2X", read_memory(address));
	printAddress(pc, opcode, read_memory(address), buff, false);
	printOpcode("", address, a, x, y, p, sp, cycleCount);
	pc += 2;
}
void CPU::Implicit() {
	char buff[100];
	sprintf(buff, "%28s", "");
	printAddress(pc, opcode, 0, buff, true);
	printOpcode("", address, a, x, y, p, sp, cycleCount);
	pc += opcode == 0x00 ? 2 : 1;
}
void CPU::Indirect(){
	uint16_t jmp_address = read_address(pc + 1);
	if ((jmp_address & 0xFF) == 0xFF) {
		address = ((uint16_t)read_memory(jmp_address & 0xFF00) << 8) + read_memory(jmp_address);
	}
	else {
		address = read_address(jmp_address);
	}
	char buff[100];
	sprintf(buff, "($%-4.4X)%21s", jmp_address, "");
	printAddress(pc, opcode, jmp_address, buff, false);
	printOpcode("", address, a, x, y, p, sp, cycleCount);
	pc += 3;
}
void CPU::IndirectX(){
	address = read_address_bug((read_memory(pc + 1) + x) & 0xFF);
	char buff[100];
	sprintf(buff, "($%-2.2X,X)%21s", read_memory(pc + 1), "");
	printAddress(pc, opcode, read_memory(pc + 1), buff, false);
	printOpcode("", address, a, x, y, p, sp, cycleCount);
	pc += 2;
}
void CPU::IndirectY(){
	address = read_address_bug(read_memory(pc + 1)) + y;
	pageCrossed = pageDiff(address - (uint16_t)x, address);
	char buff[100];
	sprintf(buff, "($%-2.2X),Y%21s", read_memory(pc + 1), "");
	printAddress(pc, opcode, read_memory(pc + 1), buff, false);
	printOpcode("", address, a, x, y, p, sp, cycleCount);
	pc += 2;
}
void CPU::Relative() {
	address = pc + 1;
	char buff[100];
	sprintf(buff, "$%-27.2X", relative_address(pc, read_memory(address)) + 2);
	printAddress(pc, opcode, read_memory(address), buff, false);
	printOpcode("", address, a, x, y, p, sp, cycleCount);
	pc += 2;
}
void CPU::ZeroPage(){
	address = read_memory(pc + 1);
	char buff[100];
	sprintf(buff, "$%-27.2X", address);
	printAddress(pc, opcode, address, buff, false);
	printOpcode("", address, a, x, y, p, sp, cycleCount);
	pc += 2;
}
void CPU::ZeroPageX(){
	uint16_t temp_address = read_memory(pc + 1);
	address = (temp_address + x) & 0xFF;
	char buff[100];
	sprintf(buff, "$%-2.2X,X%23s", temp_address, "");
	printAddress(pc, opcode, temp_address, buff, false);
	printOpcode("", address, a, x, y, p, sp, cycleCount);
	pc += 2;
}
void CPU::ZeroPageY(){
	uint16_t temp_address = read_memory(pc + 1);
	address = (temp_address + y) & 0xFF;
	char buff[100];
	sprintf(buff, "$%-2.2X,Y%23s", temp_address, "");
	printAddress(pc, opcode, temp_address, buff, false);
	printOpcode("", address, a, x, y, p, sp, cycleCount);
	pc += 2;
}

#pragma endregion

#pragma region Instructions

void CPU::ADC() {
	src = read_memory(address);

	temp = (uint16_t)(src + a + (get_carry() ? 1 : 0));

	set_carry(temp > 0xFF);
	set_zero(temp & 0xFF);
	set_negative(temp);
	set_overflow(!((a ^ src) & 0x80) && ((a ^ temp) & 0x80));

	a = (uint8_t)temp;
}
void CPU::AND() {
	src = read_memory(address);
	a &= src;
	set_negative(a);
	set_zero(a);
}
void CPU::ASL() {
	if (opcode != 0x0A) {
		src = read_memory(address);
	}

	set_carry(src & 0x80);
	src = (uint8_t)src << 1;
	set_zero(src);
	set_negative(src);

	if (opcode == 0x0A) {
		a = src;
	}
	else {
		store_memory(address, src);
	}
}
void CPU::BCC() {
	if (!get_carry()) {
		temp = pc;
		pc = relative_address(pc, read_memory(address));
		addBranchCycles(temp, pc);
	}
}
void CPU::BCS() {
	if (get_carry()) {
		temp = pc;
		pc = relative_address(pc, read_memory(address));
		addBranchCycles(temp, pc);
	}
}
void CPU::BEQ() {
	if (get_zero()) {
		temp = pc;
		pc = relative_address(pc, read_memory(address));
		addBranchCycles(temp, pc);
	}
}
void CPU::BIT() {
	src = read_memory(address);
	set_negative(src);
	set_overflow(((1 << 6) & src) != 0);
	set_zero(src & a);
}
void CPU::BMI() {
	if (get_negative()) {
		temp = pc;
		pc = relative_address(pc, read_memory(address));
		addBranchCycles(temp, pc);
	}
}
void CPU::BNE() {
	if (!get_zero()) {
		temp = pc;
		pc = relative_address(pc, read_memory(address));
		addBranchCycles(temp, pc);
	}
}
void CPU::BPL() {
	if (!get_negative()) {
		temp = pc;
		pc = relative_address(pc, read_memory(address));
		addBranchCycles(temp, pc);
	}
}
void CPU::BRK() {
	push(pc >> 8);
	push(pc);
	set_break(1);
	push(p | (1 << 4));
	set_irq_disable(1);
	pc = read_address(0xFFFE);
}
void CPU::BVC() {
	if (!get_overflow()) {
		pc = relative_address(pc, read_memory(address));
	}
}
void CPU::BVS() {
	if (get_overflow()) {
		pc = relative_address(pc, read_memory(address));
	}
}
void CPU::CLC() {
	set_carry(0);
}
void CPU::CLD() {
	set_decimal(0);
}
void CPU::CLI() {
	set_irq_disable(0);
}
void CPU::CLV() {
	set_overflow(0);
}
void CPU::CMP() {
	src = read_memory(address);
	cmp_bit_helper(a, src);
}
void CPU::CPX() {
	src = read_memory(address);
	cmp_bit_helper(x, src);
}
void CPU::CPY() {
	src = read_memory(address);
	cmp_bit_helper(y, src);
}
void CPU::DEC() {
	src = read_memory(address);
	src -= 1;
	set_negative(src);
	set_zero(src);
	store_memory(address, src);
}
void CPU::DEX() {
	x -= 1;
	set_negative(x);
	set_zero(x);
}
void CPU::DEY() {
	y -= 1;
	set_negative(y);
	set_zero(y);
}
void CPU::EOR() {
	src = read_memory(address);
	a ^= src;
	set_negative(a);
	set_zero(a);	
}
void CPU::INC() {
	src = read_memory(address);
	src += 1;
	set_negative(src);
	set_zero(src);
	store_memory(address, src);
}
void CPU::INX() {
	x += 1;
	set_negative(x);
	set_zero(x);
}
void CPU::INY() {
	y += 1;
	set_negative(y);
	set_zero(y);
}
void CPU::JMP() {
	pc = address;
	}
void CPU::JSR() {
	pc--;
	push(pc >> 8);
	push(pc);
	pc = address;
}
void CPU::LDA() {
	a = read_memory(address);
	set_zero(a);
	set_negative(a);
}
void CPU::LDX() {
	x = read_memory(address);
	set_zero(x);
	set_negative(x);
}
void CPU::LDY() {
	y = read_memory(address);
	set_zero(y);
	set_negative(y);
}
void CPU::LSR() {
	if (opcode != 0x4A) {
		src = read_memory(address);
}

	set_carry(src & 0x01);
	src >>= 1;
	set_negative(src);
	set_zero(src);

	if (opcode == 0x4A) {
		a = src;
	}
	else {
		store_memory(address, src);
	}
}
void CPU::NOP() {
	}
void CPU::ORA() {
	src = read_memory(address);
	a |= src;
	set_zero(a);
	set_negative(a);
}
void CPU::PHA() {
	push(a);
}
void CPU::PHP() {
	push((p | 0x10));
}
void CPU::PLA() {
	a = pop();
	set_zero(a);
	set_negative(a);
}
void CPU::PLP() {
	p = pop() & 0xEF | 0x20;
}
void CPU::ROL() {
	if (opcode != 0x2A) {
		src = read_memory(address);
	}
	uint8_t c = get_carry();
	set_carry(src & 0x80);
	src <<= 1;
	src |= c;
	set_negative(src);
	set_zero(src);
	if (opcode == 0x2A) {
		a = src;
	}
	else {
		store_memory(address, src);
	}
}
void CPU::ROR() {
	if (opcode != 0x6A) {
		src = read_memory(address);
	}
	uint8_t c = get_carry();
	set_carry(src & 0x01);
	src >>= 1;
	src |= (c << 7);
	set_negative(src);
	set_zero(src);
	if (opcode == 0x6A) {
		a = src;
	}
	else {
		store_memory(address, src);
	}
}
void CPU::RTI() {
	p = pop() & 0xEF | 0x20;
	pc = pop();
	pc |= (uint16_t)pop() << 8;
}
void CPU::RTS() {
	pc = pop();
	pc |= (uint16_t)pop() << 8;
	pc++;
}
void CPU::SBC() {
	src = read_memory(address);

	temp = (uint16_t)(a - src - (get_carry() ? 0 : 1));
	set_negative((uint8_t)temp);
	set_zero((uint8_t)temp);
	if (a >= (src + (get_carry() ? 0 : 1))) {
		set_carry(1);
	}
	else {
		set_carry(0);
	}
	if (((a ^ src) & 0x80) != 0 && ((a ^ temp) & 0x80) != 0) {
		set_overflow(1);
	}
	else {
		set_overflow(0);
	}

	a = (uint8_t)temp;
}
void CPU::SEC() {
	set_carry(1);
}
void CPU::SED() {
	set_decimal(1);
}
void CPU::SEI() {
	set_irq_disable(1);
}
void CPU::STA() {
	store_memory(address, a);
}
void CPU::STX() {
	if (address & 0x7FF == 0x7FF) {
		address = address;
	}
	store_memory(address, x);
}
void CPU::STY() {
	store_memory(address, y);
}
void CPU::TAX() {
	x = a;
	set_negative(x);
	set_zero(x);
}
void CPU::TAY() {
	y = a;
	set_negative(y);
	set_zero(y);
}
void CPU::TSX() {
	x = sp;
	set_zero(x);
	set_negative(x);
}
void CPU::TXA() {
	a = x;
	set_negative(a);
	set_zero(a);
}
void CPU::TXS() {
	sp = x;
}
void CPU::TYA() {
	a = y;
	set_zero(a);
	set_negative(a);
}

#pragma endregion

#pragma region initAdressingModeTable()

void CPU::initAddressingModeTable() {
	this->addressingModeTable[0] = &CPU::Implicit;
	this->addressingModeTable[1] = &CPU::IndirectX;
	this->addressingModeTable[2] = &CPU::cpuNULL;
	this->addressingModeTable[3] = &CPU::cpuNULL;
	this->addressingModeTable[4] = &CPU::cpuNULL;
	this->addressingModeTable[5] = &CPU::ZeroPage;
	this->addressingModeTable[6] = &CPU::ZeroPage;
	this->addressingModeTable[7] = &CPU::cpuNULL;
	this->addressingModeTable[8] = &CPU::Implicit;
	this->addressingModeTable[9] = &CPU::Immediate;
	this->addressingModeTable[10] = &CPU::Accumulator;
	this->addressingModeTable[11] = &CPU::cpuNULL;
	this->addressingModeTable[12] = &CPU::cpuNULL;
	this->addressingModeTable[13] = &CPU::Absolute;
	this->addressingModeTable[14] = &CPU::Absolute;
	this->addressingModeTable[15] = &CPU::cpuNULL;
	this->addressingModeTable[16] = &CPU::Relative;
	this->addressingModeTable[17] = &CPU::IndirectY;
	this->addressingModeTable[18] = &CPU::cpuNULL;
	this->addressingModeTable[19] = &CPU::cpuNULL;
	this->addressingModeTable[20] = &CPU::cpuNULL;
	this->addressingModeTable[21] = &CPU::ZeroPageX;
	this->addressingModeTable[22] = &CPU::ZeroPageX;
	this->addressingModeTable[23] = &CPU::cpuNULL;
	this->addressingModeTable[24] = &CPU::Implicit;
	this->addressingModeTable[25] = &CPU::AbsoluteY;
	this->addressingModeTable[26] = &CPU::cpuNULL;
	this->addressingModeTable[27] = &CPU::cpuNULL;
	this->addressingModeTable[28] = &CPU::cpuNULL;
	this->addressingModeTable[29] = &CPU::AbsoluteX;
	this->addressingModeTable[30] = &CPU::AbsoluteX;
	this->addressingModeTable[31] = &CPU::cpuNULL;
	this->addressingModeTable[32] = &CPU::Absolute;
	this->addressingModeTable[33] = &CPU::IndirectX;
	this->addressingModeTable[34] = &CPU::cpuNULL;
	this->addressingModeTable[35] = &CPU::cpuNULL;
	this->addressingModeTable[36] = &CPU::ZeroPage;
	this->addressingModeTable[37] = &CPU::ZeroPage;
	this->addressingModeTable[38] = &CPU::ZeroPage;
	this->addressingModeTable[39] = &CPU::cpuNULL;
	this->addressingModeTable[40] = &CPU::Implicit;
	this->addressingModeTable[41] = &CPU::Immediate;
	this->addressingModeTable[42] = &CPU::Accumulator;
	this->addressingModeTable[43] = &CPU::cpuNULL;
	this->addressingModeTable[44] = &CPU::Absolute;
	this->addressingModeTable[45] = &CPU::Absolute;
	this->addressingModeTable[46] = &CPU::Absolute;
	this->addressingModeTable[47] = &CPU::cpuNULL;
	this->addressingModeTable[48] = &CPU::Relative;
	this->addressingModeTable[49] = &CPU::IndirectY;
	this->addressingModeTable[50] = &CPU::cpuNULL;
	this->addressingModeTable[51] = &CPU::cpuNULL;
	this->addressingModeTable[52] = &CPU::cpuNULL;
	this->addressingModeTable[53] = &CPU::ZeroPageX;
	this->addressingModeTable[54] = &CPU::ZeroPageX;
	this->addressingModeTable[55] = &CPU::cpuNULL;
	this->addressingModeTable[56] = &CPU::Implicit;
	this->addressingModeTable[57] = &CPU::AbsoluteY;
	this->addressingModeTable[58] = &CPU::cpuNULL;
	this->addressingModeTable[59] = &CPU::cpuNULL;
	this->addressingModeTable[60] = &CPU::cpuNULL;
	this->addressingModeTable[61] = &CPU::AbsoluteX;
	this->addressingModeTable[62] = &CPU::AbsoluteX;
	this->addressingModeTable[63] = &CPU::cpuNULL;
	this->addressingModeTable[64] = &CPU::Implicit;
	this->addressingModeTable[65] = &CPU::IndirectX;
	this->addressingModeTable[66] = &CPU::cpuNULL;
	this->addressingModeTable[67] = &CPU::cpuNULL;
	this->addressingModeTable[68] = &CPU::cpuNULL;
	this->addressingModeTable[69] = &CPU::ZeroPage;
	this->addressingModeTable[70] = &CPU::ZeroPage;
	this->addressingModeTable[71] = &CPU::cpuNULL;
	this->addressingModeTable[72] = &CPU::Implicit;
	this->addressingModeTable[73] = &CPU::Immediate;
	this->addressingModeTable[74] = &CPU::Accumulator;
	this->addressingModeTable[75] = &CPU::cpuNULL;
	this->addressingModeTable[76] = &CPU::Absolute;
	this->addressingModeTable[77] = &CPU::Absolute;
	this->addressingModeTable[78] = &CPU::Absolute;
	this->addressingModeTable[79] = &CPU::cpuNULL;
	this->addressingModeTable[80] = &CPU::Relative;
	this->addressingModeTable[81] = &CPU::IndirectY;
	this->addressingModeTable[82] = &CPU::cpuNULL;
	this->addressingModeTable[83] = &CPU::cpuNULL;
	this->addressingModeTable[84] = &CPU::cpuNULL;
	this->addressingModeTable[85] = &CPU::ZeroPageX;
	this->addressingModeTable[86] = &CPU::ZeroPageX;
	this->addressingModeTable[87] = &CPU::cpuNULL;
	this->addressingModeTable[88] = &CPU::Implicit;
	this->addressingModeTable[89] = &CPU::AbsoluteY;
	this->addressingModeTable[90] = &CPU::cpuNULL;
	this->addressingModeTable[91] = &CPU::cpuNULL;
	this->addressingModeTable[92] = &CPU::cpuNULL;
	this->addressingModeTable[93] = &CPU::AbsoluteX;
	this->addressingModeTable[94] = &CPU::AbsoluteX;
	this->addressingModeTable[95] = &CPU::cpuNULL;
	this->addressingModeTable[96] = &CPU::Implicit;
	this->addressingModeTable[97] = &CPU::IndirectX;
	this->addressingModeTable[98] = &CPU::cpuNULL;
	this->addressingModeTable[99] = &CPU::cpuNULL;
	this->addressingModeTable[100] = &CPU::cpuNULL;
	this->addressingModeTable[101] = &CPU::ZeroPage;
	this->addressingModeTable[102] = &CPU::ZeroPage;
	this->addressingModeTable[103] = &CPU::cpuNULL;
	this->addressingModeTable[104] = &CPU::Implicit;
	this->addressingModeTable[105] = &CPU::Immediate;
	this->addressingModeTable[106] = &CPU::Accumulator;
	this->addressingModeTable[107] = &CPU::cpuNULL;
	this->addressingModeTable[108] = &CPU::Indirect;
	this->addressingModeTable[109] = &CPU::Absolute;
	this->addressingModeTable[110] = &CPU::Absolute;
	this->addressingModeTable[111] = &CPU::cpuNULL;
	this->addressingModeTable[112] = &CPU::Relative;
	this->addressingModeTable[113] = &CPU::IndirectY;
	this->addressingModeTable[114] = &CPU::cpuNULL;
	this->addressingModeTable[115] = &CPU::cpuNULL;
	this->addressingModeTable[116] = &CPU::cpuNULL;
	this->addressingModeTable[117] = &CPU::ZeroPageX;
	this->addressingModeTable[118] = &CPU::ZeroPageX;
	this->addressingModeTable[119] = &CPU::cpuNULL;
	this->addressingModeTable[120] = &CPU::Implicit;
	this->addressingModeTable[121] = &CPU::AbsoluteY;
	this->addressingModeTable[122] = &CPU::cpuNULL;
	this->addressingModeTable[123] = &CPU::cpuNULL;
	this->addressingModeTable[124] = &CPU::cpuNULL;
	this->addressingModeTable[125] = &CPU::AbsoluteX;
	this->addressingModeTable[126] = &CPU::AbsoluteX;
	this->addressingModeTable[127] = &CPU::cpuNULL;
	this->addressingModeTable[128] = &CPU::cpuNULL;
	this->addressingModeTable[129] = &CPU::IndirectX;
	this->addressingModeTable[130] = &CPU::cpuNULL;
	this->addressingModeTable[131] = &CPU::cpuNULL;
	this->addressingModeTable[132] = &CPU::ZeroPage;
	this->addressingModeTable[133] = &CPU::ZeroPage;
	this->addressingModeTable[134] = &CPU::ZeroPage;
	this->addressingModeTable[135] = &CPU::cpuNULL;
	this->addressingModeTable[136] = &CPU::Implicit;
	this->addressingModeTable[137] = &CPU::cpuNULL;
	this->addressingModeTable[138] = &CPU::Implicit;
	this->addressingModeTable[139] = &CPU::cpuNULL;
	this->addressingModeTable[140] = &CPU::Absolute;
	this->addressingModeTable[141] = &CPU::Absolute;
	this->addressingModeTable[142] = &CPU::Absolute;
	this->addressingModeTable[143] = &CPU::cpuNULL;
	this->addressingModeTable[144] = &CPU::Relative;
	this->addressingModeTable[145] = &CPU::IndirectY;
	this->addressingModeTable[146] = &CPU::cpuNULL;
	this->addressingModeTable[147] = &CPU::cpuNULL;
	this->addressingModeTable[148] = &CPU::ZeroPageX;
	this->addressingModeTable[149] = &CPU::ZeroPageX;
	this->addressingModeTable[150] = &CPU::ZeroPageY;
	this->addressingModeTable[151] = &CPU::cpuNULL;
	this->addressingModeTable[152] = &CPU::Implicit;
	this->addressingModeTable[153] = &CPU::AbsoluteY;
	this->addressingModeTable[154] = &CPU::Implicit;
	this->addressingModeTable[155] = &CPU::cpuNULL;
	this->addressingModeTable[156] = &CPU::cpuNULL;
	this->addressingModeTable[157] = &CPU::AbsoluteX;
	this->addressingModeTable[158] = &CPU::cpuNULL;
	this->addressingModeTable[159] = &CPU::cpuNULL;
	this->addressingModeTable[160] = &CPU::Immediate;
	this->addressingModeTable[161] = &CPU::IndirectX;
	this->addressingModeTable[162] = &CPU::Immediate;
	this->addressingModeTable[163] = &CPU::cpuNULL;
	this->addressingModeTable[164] = &CPU::ZeroPage;
	this->addressingModeTable[165] = &CPU::ZeroPage;
	this->addressingModeTable[166] = &CPU::ZeroPage;
	this->addressingModeTable[167] = &CPU::cpuNULL;
	this->addressingModeTable[168] = &CPU::Implicit;
	this->addressingModeTable[169] = &CPU::Immediate;
	this->addressingModeTable[170] = &CPU::Implicit;
	this->addressingModeTable[171] = &CPU::cpuNULL;
	this->addressingModeTable[172] = &CPU::Absolute;
	this->addressingModeTable[173] = &CPU::Absolute;
	this->addressingModeTable[174] = &CPU::Absolute;
	this->addressingModeTable[175] = &CPU::cpuNULL;
	this->addressingModeTable[176] = &CPU::Relative;
	this->addressingModeTable[177] = &CPU::IndirectY;
	this->addressingModeTable[178] = &CPU::cpuNULL;
	this->addressingModeTable[179] = &CPU::cpuNULL;
	this->addressingModeTable[180] = &CPU::ZeroPageX;
	this->addressingModeTable[181] = &CPU::ZeroPageX;
	this->addressingModeTable[182] = &CPU::ZeroPageY;
	this->addressingModeTable[183] = &CPU::cpuNULL;
	this->addressingModeTable[184] = &CPU::Implicit;
	this->addressingModeTable[185] = &CPU::AbsoluteY;
	this->addressingModeTable[186] = &CPU::Implicit;
	this->addressingModeTable[187] = &CPU::cpuNULL;
	this->addressingModeTable[188] = &CPU::AbsoluteX;
	this->addressingModeTable[189] = &CPU::AbsoluteX;
	this->addressingModeTable[190] = &CPU::AbsoluteY;
	this->addressingModeTable[191] = &CPU::cpuNULL;
	this->addressingModeTable[192] = &CPU::Immediate;
	this->addressingModeTable[193] = &CPU::IndirectX;
	this->addressingModeTable[194] = &CPU::cpuNULL;
	this->addressingModeTable[195] = &CPU::cpuNULL;
	this->addressingModeTable[196] = &CPU::ZeroPage;
	this->addressingModeTable[197] = &CPU::ZeroPage;
	this->addressingModeTable[198] = &CPU::ZeroPage;
	this->addressingModeTable[199] = &CPU::cpuNULL;
	this->addressingModeTable[200] = &CPU::Implicit;
	this->addressingModeTable[201] = &CPU::Immediate;
	this->addressingModeTable[202] = &CPU::Implicit;
	this->addressingModeTable[203] = &CPU::cpuNULL;
	this->addressingModeTable[204] = &CPU::Absolute;
	this->addressingModeTable[205] = &CPU::Absolute;
	this->addressingModeTable[206] = &CPU::Absolute;
	this->addressingModeTable[207] = &CPU::cpuNULL;
	this->addressingModeTable[208] = &CPU::Relative;
	this->addressingModeTable[209] = &CPU::IndirectY;
	this->addressingModeTable[210] = &CPU::cpuNULL;
	this->addressingModeTable[211] = &CPU::cpuNULL;
	this->addressingModeTable[212] = &CPU::cpuNULL;
	this->addressingModeTable[213] = &CPU::ZeroPageX;
	this->addressingModeTable[214] = &CPU::ZeroPageX;
	this->addressingModeTable[215] = &CPU::cpuNULL;
	this->addressingModeTable[216] = &CPU::Implicit;
	this->addressingModeTable[217] = &CPU::AbsoluteY;
	this->addressingModeTable[218] = &CPU::cpuNULL;
	this->addressingModeTable[219] = &CPU::cpuNULL;
	this->addressingModeTable[220] = &CPU::cpuNULL;
	this->addressingModeTable[221] = &CPU::AbsoluteX;
	this->addressingModeTable[222] = &CPU::AbsoluteX;
	this->addressingModeTable[223] = &CPU::cpuNULL;
	this->addressingModeTable[224] = &CPU::Immediate;
	this->addressingModeTable[225] = &CPU::IndirectX;
	this->addressingModeTable[226] = &CPU::cpuNULL;
	this->addressingModeTable[227] = &CPU::cpuNULL;
	this->addressingModeTable[228] = &CPU::ZeroPage;
	this->addressingModeTable[229] = &CPU::ZeroPage;
	this->addressingModeTable[230] = &CPU::ZeroPage;
	this->addressingModeTable[231] = &CPU::cpuNULL;
	this->addressingModeTable[232] = &CPU::Implicit;
	this->addressingModeTable[233] = &CPU::Immediate;
	this->addressingModeTable[234] = &CPU::Implicit;
	this->addressingModeTable[235] = &CPU::cpuNULL;
	this->addressingModeTable[236] = &CPU::Absolute;
	this->addressingModeTable[237] = &CPU::Absolute;
	this->addressingModeTable[238] = &CPU::Absolute;
	this->addressingModeTable[239] = &CPU::cpuNULL;
	this->addressingModeTable[240] = &CPU::Relative;
	this->addressingModeTable[241] = &CPU::IndirectY;
	this->addressingModeTable[242] = &CPU::cpuNULL;
	this->addressingModeTable[243] = &CPU::cpuNULL;
	this->addressingModeTable[244] = &CPU::cpuNULL;
	this->addressingModeTable[245] = &CPU::ZeroPageX;
	this->addressingModeTable[246] = &CPU::ZeroPageX;
	this->addressingModeTable[247] = &CPU::cpuNULL;
	this->addressingModeTable[248] = &CPU::Implicit;
	this->addressingModeTable[249] = &CPU::AbsoluteY;
	this->addressingModeTable[250] = &CPU::cpuNULL;
	this->addressingModeTable[251] = &CPU::cpuNULL;
	this->addressingModeTable[252] = &CPU::cpuNULL;
	this->addressingModeTable[253] = &CPU::AbsoluteX;
	this->addressingModeTable[254] = &CPU::AbsoluteX;
	this->addressingModeTable[255] = &CPU::cpuNULL;

}

#pragma endregion

#pragma region initCpuTable()

void CPU::initCPUTable() {
	this->cpuTable[0] = &CPU::BRK;
	this->cpuTable[1] = &CPU::ORA;
	this->cpuTable[2] = &CPU::cpuNULL;
	this->cpuTable[3] = &CPU::cpuNULL;
	this->cpuTable[4] = &CPU::cpuNULL;
	this->cpuTable[5] = &CPU::ORA;
	this->cpuTable[6] = &CPU::ASL;
	this->cpuTable[7] = &CPU::cpuNULL;
	this->cpuTable[8] = &CPU::PHP;
	this->cpuTable[9] = &CPU::ORA;
	this->cpuTable[10] = &CPU::ASL;
	this->cpuTable[11] = &CPU::cpuNULL;
	this->cpuTable[12] = &CPU::cpuNULL;
	this->cpuTable[13] = &CPU::ORA;
	this->cpuTable[14] = &CPU::ASL;
	this->cpuTable[15] = &CPU::cpuNULL;
	this->cpuTable[16] = &CPU::BPL;
	this->cpuTable[17] = &CPU::ORA;
	this->cpuTable[18] = &CPU::cpuNULL;
	this->cpuTable[19] = &CPU::cpuNULL;
	this->cpuTable[20] = &CPU::cpuNULL;
	this->cpuTable[21] = &CPU::ORA;
	this->cpuTable[22] = &CPU::ASL;
	this->cpuTable[23] = &CPU::cpuNULL;
	this->cpuTable[24] = &CPU::CLC;
	this->cpuTable[25] = &CPU::ORA;
	this->cpuTable[26] = &CPU::cpuNULL;
	this->cpuTable[27] = &CPU::cpuNULL;
	this->cpuTable[28] = &CPU::cpuNULL;
	this->cpuTable[29] = &CPU::ORA;
	this->cpuTable[30] = &CPU::ASL;
	this->cpuTable[31] = &CPU::cpuNULL;
	this->cpuTable[32] = &CPU::JSR;
	this->cpuTable[33] = &CPU::AND;
	this->cpuTable[34] = &CPU::cpuNULL;
	this->cpuTable[35] = &CPU::cpuNULL;
	this->cpuTable[36] = &CPU::BIT;
	this->cpuTable[37] = &CPU::AND;
	this->cpuTable[38] = &CPU::ROL;
	this->cpuTable[39] = &CPU::cpuNULL;
	this->cpuTable[40] = &CPU::PLP;
	this->cpuTable[41] = &CPU::AND;
	this->cpuTable[42] = &CPU::ROL;
	this->cpuTable[43] = &CPU::cpuNULL;
	this->cpuTable[44] = &CPU::BIT;
	this->cpuTable[45] = &CPU::AND;
	this->cpuTable[46] = &CPU::ROL;
	this->cpuTable[47] = &CPU::cpuNULL;
	this->cpuTable[48] = &CPU::BMI;
	this->cpuTable[49] = &CPU::AND;
	this->cpuTable[50] = &CPU::cpuNULL;
	this->cpuTable[51] = &CPU::cpuNULL;
	this->cpuTable[52] = &CPU::cpuNULL;
	this->cpuTable[53] = &CPU::AND;
	this->cpuTable[54] = &CPU::ROL;
	this->cpuTable[55] = &CPU::cpuNULL;
	this->cpuTable[56] = &CPU::SEC;
	this->cpuTable[57] = &CPU::AND;
	this->cpuTable[58] = &CPU::cpuNULL;
	this->cpuTable[59] = &CPU::cpuNULL;
	this->cpuTable[60] = &CPU::cpuNULL;
	this->cpuTable[61] = &CPU::AND;
	this->cpuTable[62] = &CPU::ROL;
	this->cpuTable[63] = &CPU::cpuNULL;
	this->cpuTable[64] = &CPU::RTI;
	this->cpuTable[65] = &CPU::EOR;
	this->cpuTable[66] = &CPU::cpuNULL;
	this->cpuTable[67] = &CPU::cpuNULL;
	this->cpuTable[68] = &CPU::cpuNULL;
	this->cpuTable[69] = &CPU::EOR;
	this->cpuTable[70] = &CPU::LSR;
	this->cpuTable[71] = &CPU::cpuNULL;
	this->cpuTable[72] = &CPU::PHA;
	this->cpuTable[73] = &CPU::EOR;
	this->cpuTable[74] = &CPU::LSR;
	this->cpuTable[75] = &CPU::cpuNULL;
	this->cpuTable[76] = &CPU::JMP;
	this->cpuTable[77] = &CPU::EOR;
	this->cpuTable[78] = &CPU::LSR;
	this->cpuTable[79] = &CPU::cpuNULL;
	this->cpuTable[80] = &CPU::BVC;
	this->cpuTable[81] = &CPU::EOR;
	this->cpuTable[82] = &CPU::cpuNULL;
	this->cpuTable[83] = &CPU::cpuNULL;
	this->cpuTable[84] = &CPU::cpuNULL;
	this->cpuTable[85] = &CPU::EOR;
	this->cpuTable[86] = &CPU::LSR;
	this->cpuTable[87] = &CPU::cpuNULL;
	this->cpuTable[88] = &CPU::CLI;
	this->cpuTable[89] = &CPU::EOR;
	this->cpuTable[90] = &CPU::cpuNULL;
	this->cpuTable[91] = &CPU::cpuNULL;
	this->cpuTable[92] = &CPU::cpuNULL;
	this->cpuTable[93] = &CPU::EOR;
	this->cpuTable[94] = &CPU::LSR;
	this->cpuTable[95] = &CPU::cpuNULL;
	this->cpuTable[96] = &CPU::RTS;
	this->cpuTable[97] = &CPU::ADC;
	this->cpuTable[98] = &CPU::cpuNULL;
	this->cpuTable[99] = &CPU::cpuNULL;
	this->cpuTable[100] = &CPU::cpuNULL;
	this->cpuTable[101] = &CPU::ADC;
	this->cpuTable[102] = &CPU::ROR;
	this->cpuTable[103] = &CPU::cpuNULL;
	this->cpuTable[104] = &CPU::PLA;
	this->cpuTable[105] = &CPU::ADC;
	this->cpuTable[106] = &CPU::ROR;
	this->cpuTable[107] = &CPU::cpuNULL;
	this->cpuTable[108] = &CPU::JMP;
	this->cpuTable[109] = &CPU::ADC;
	this->cpuTable[110] = &CPU::ROR;
	this->cpuTable[111] = &CPU::cpuNULL;
	this->cpuTable[112] = &CPU::BVS;
	this->cpuTable[113] = &CPU::ADC;
	this->cpuTable[114] = &CPU::cpuNULL;
	this->cpuTable[115] = &CPU::cpuNULL;
	this->cpuTable[116] = &CPU::cpuNULL;
	this->cpuTable[117] = &CPU::ADC;
	this->cpuTable[118] = &CPU::ROR;
	this->cpuTable[119] = &CPU::cpuNULL;
	this->cpuTable[120] = &CPU::SEI;
	this->cpuTable[121] = &CPU::ADC;
	this->cpuTable[122] = &CPU::cpuNULL;
	this->cpuTable[123] = &CPU::cpuNULL;
	this->cpuTable[124] = &CPU::cpuNULL;
	this->cpuTable[125] = &CPU::ADC;
	this->cpuTable[126] = &CPU::ROR;
	this->cpuTable[127] = &CPU::cpuNULL;
	this->cpuTable[128] = &CPU::cpuNULL;
	this->cpuTable[129] = &CPU::STA;
	this->cpuTable[130] = &CPU::cpuNULL;
	this->cpuTable[131] = &CPU::cpuNULL;
	this->cpuTable[132] = &CPU::STY;
	this->cpuTable[133] = &CPU::STA;
	this->cpuTable[134] = &CPU::STX;
	this->cpuTable[135] = &CPU::cpuNULL;
	this->cpuTable[136] = &CPU::DEY;
	this->cpuTable[137] = &CPU::cpuNULL;
	this->cpuTable[138] = &CPU::TXA;
	this->cpuTable[139] = &CPU::cpuNULL;
	this->cpuTable[140] = &CPU::STY;
	this->cpuTable[141] = &CPU::STA;
	this->cpuTable[142] = &CPU::STX;
	this->cpuTable[143] = &CPU::cpuNULL;
	this->cpuTable[144] = &CPU::BCC;
	this->cpuTable[145] = &CPU::STA;
	this->cpuTable[146] = &CPU::cpuNULL;
	this->cpuTable[147] = &CPU::cpuNULL;
	this->cpuTable[148] = &CPU::STY;
	this->cpuTable[149] = &CPU::STA;
	this->cpuTable[150] = &CPU::STX;
	this->cpuTable[151] = &CPU::cpuNULL;
	this->cpuTable[152] = &CPU::TYA;
	this->cpuTable[153] = &CPU::STA;
	this->cpuTable[154] = &CPU::TXS;
	this->cpuTable[155] = &CPU::cpuNULL;
	this->cpuTable[156] = &CPU::cpuNULL;
	this->cpuTable[157] = &CPU::STA;
	this->cpuTable[158] = &CPU::cpuNULL;
	this->cpuTable[159] = &CPU::cpuNULL;
	this->cpuTable[160] = &CPU::LDY;
	this->cpuTable[161] = &CPU::LDA;
	this->cpuTable[162] = &CPU::LDX;
	this->cpuTable[163] = &CPU::cpuNULL;
	this->cpuTable[164] = &CPU::LDY;
	this->cpuTable[165] = &CPU::LDA;
	this->cpuTable[166] = &CPU::LDX;
	this->cpuTable[167] = &CPU::cpuNULL;
	this->cpuTable[168] = &CPU::TAY;
	this->cpuTable[169] = &CPU::LDA;
	this->cpuTable[170] = &CPU::TAX;
	this->cpuTable[171] = &CPU::cpuNULL;
	this->cpuTable[172] = &CPU::LDY;
	this->cpuTable[173] = &CPU::LDA;
	this->cpuTable[174] = &CPU::LDX;
	this->cpuTable[175] = &CPU::cpuNULL;
	this->cpuTable[176] = &CPU::BCS;
	this->cpuTable[177] = &CPU::LDA;
	this->cpuTable[178] = &CPU::cpuNULL;
	this->cpuTable[179] = &CPU::cpuNULL;
	this->cpuTable[180] = &CPU::LDY;
	this->cpuTable[181] = &CPU::LDA;
	this->cpuTable[182] = &CPU::LDX;
	this->cpuTable[183] = &CPU::cpuNULL;
	this->cpuTable[184] = &CPU::CLV;
	this->cpuTable[185] = &CPU::LDA;
	this->cpuTable[186] = &CPU::TSX;
	this->cpuTable[187] = &CPU::cpuNULL;
	this->cpuTable[188] = &CPU::LDY;
	this->cpuTable[189] = &CPU::LDA;
	this->cpuTable[190] = &CPU::LDX;
	this->cpuTable[191] = &CPU::cpuNULL;
	this->cpuTable[192] = &CPU::CPY;
	this->cpuTable[193] = &CPU::CMP;
	this->cpuTable[194] = &CPU::cpuNULL;
	this->cpuTable[195] = &CPU::cpuNULL;
	this->cpuTable[196] = &CPU::CPY;
	this->cpuTable[197] = &CPU::CMP;
	this->cpuTable[198] = &CPU::DEC;
	this->cpuTable[199] = &CPU::cpuNULL;
	this->cpuTable[200] = &CPU::INY;
	this->cpuTable[201] = &CPU::CMP;
	this->cpuTable[202] = &CPU::DEX;
	this->cpuTable[203] = &CPU::cpuNULL;
	this->cpuTable[204] = &CPU::CPY;
	this->cpuTable[205] = &CPU::CMP;
	this->cpuTable[206] = &CPU::DEC;
	this->cpuTable[207] = &CPU::cpuNULL;
	this->cpuTable[208] = &CPU::BNE;
	this->cpuTable[209] = &CPU::CMP;
	this->cpuTable[210] = &CPU::cpuNULL;
	this->cpuTable[211] = &CPU::cpuNULL;
	this->cpuTable[212] = &CPU::cpuNULL;
	this->cpuTable[213] = &CPU::CMP;
	this->cpuTable[214] = &CPU::DEC;
	this->cpuTable[215] = &CPU::cpuNULL;
	this->cpuTable[216] = &CPU::CLD;
	this->cpuTable[217] = &CPU::CMP;
	this->cpuTable[218] = &CPU::cpuNULL;
	this->cpuTable[219] = &CPU::cpuNULL;
	this->cpuTable[220] = &CPU::cpuNULL;
	this->cpuTable[221] = &CPU::CMP;
	this->cpuTable[222] = &CPU::DEC;
	this->cpuTable[223] = &CPU::cpuNULL;
	this->cpuTable[224] = &CPU::CPX;
	this->cpuTable[225] = &CPU::SBC;
	this->cpuTable[226] = &CPU::cpuNULL;
	this->cpuTable[227] = &CPU::cpuNULL;
	this->cpuTable[228] = &CPU::CPX;
	this->cpuTable[229] = &CPU::SBC;
	this->cpuTable[230] = &CPU::INC;
	this->cpuTable[231] = &CPU::cpuNULL;
	this->cpuTable[232] = &CPU::INX;
	this->cpuTable[233] = &CPU::SBC;
	this->cpuTable[234] = &CPU::NOP;
	this->cpuTable[235] = &CPU::cpuNULL;
	this->cpuTable[236] = &CPU::CPX;
	this->cpuTable[237] = &CPU::SBC;
	this->cpuTable[238] = &CPU::INC;
	this->cpuTable[239] = &CPU::cpuNULL;
	this->cpuTable[240] = &CPU::BEQ;
	this->cpuTable[241] = &CPU::SBC;
	this->cpuTable[242] = &CPU::cpuNULL;
	this->cpuTable[243] = &CPU::cpuNULL;
	this->cpuTable[244] = &CPU::cpuNULL;
	this->cpuTable[245] = &CPU::SBC;
	this->cpuTable[246] = &CPU::INC;
	this->cpuTable[247] = &CPU::cpuNULL;
	this->cpuTable[248] = &CPU::SED;
	this->cpuTable[249] = &CPU::SBC;
	this->cpuTable[250] = &CPU::cpuNULL;
	this->cpuTable[251] = &CPU::cpuNULL;
	this->cpuTable[252] = &CPU::cpuNULL;
	this->cpuTable[253] = &CPU::SBC;
	this->cpuTable[254] = &CPU::INC;
	this->cpuTable[255] = &CPU::cpuNULL;
}

#pragma endregion

#pragma region Static Init

const uint8_t CPU::instructionCycles[0x100] = {
	7, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6,
	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
	6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6,
	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
	6, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6,
	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
	6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6,
	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
	2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
	2, 6, 2, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5,
	2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
	2, 5, 2, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4,
	2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
	2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 3
};

const uint8_t CPU::instructionPageCycles[0x100] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0
};

const char* CPU::opcode_names[0x100] = {
	"BRK", "ORA", "KIL", "SLO", "NOP", "ORA", "ASL", "SLO",
	"PHP", "ORA", "ASL", "ANC", "NOP", "ORA", "ASL", "SLO",
	"BPL", "ORA", "KIL", "SLO", "NOP", "ORA", "ASL", "SLO",
	"CLC", "ORA", "NOP", "SLO", "NOP", "ORA", "ASL", "SLO",
	"JSR", "AND", "KIL", "RLA", "BIT", "AND", "ROL", "RLA",
	"PLP", "AND", "ROL", "ANC", "BIT", "AND", "ROL", "RLA",
	"BMI", "AND", "KIL", "RLA", "NOP", "AND", "ROL", "RLA",
	"SEC", "AND", "NOP", "RLA", "NOP", "AND", "ROL", "RLA",
	"RTI", "EOR", "KIL", "SRE", "NOP", "EOR", "LSR", "SRE",
	"PHA", "EOR", "LSR", "ALR", "JMP", "EOR", "LSR", "SRE",
	"BVC", "EOR", "KIL", "SRE", "NOP", "EOR", "LSR", "SRE",
	"CLI", "EOR", "NOP", "SRE", "NOP", "EOR", "LSR", "SRE",
	"RTS", "ADC", "KIL", "RRA", "NOP", "ADC", "ROR", "RRA",
	"PLA", "ADC", "ROR", "ARR", "JMP", "ADC", "ROR", "RRA",
	"BVS", "ADC", "KIL", "RRA", "NOP", "ADC", "ROR", "RRA",
	"SEI", "ADC", "NOP", "RRA", "NOP", "ADC", "ROR", "RRA",
	"NOP", "STA", "NOP", "SAX", "STY", "STA", "STX", "SAX",
	"DEY", "NOP", "TXA", "XAA", "STY", "STA", "STX", "SAX",
	"BCC", "STA", "KIL", "AHX", "STY", "STA", "STX", "SAX",
	"TYA", "STA", "TXS", "TAS", "SHY", "STA", "SHX", "AHX",
	"LDY", "LDA", "LDX", "LAX", "LDY", "LDA", "LDX", "LAX",
	"TAY", "LDA", "TAX", "LAX", "LDY", "LDA", "LDX", "LAX",
	"BCS", "LDA", "KIL", "LAX", "LDY", "LDA", "LDX", "LAX",
	"CLV", "LDA", "TSX", "LAS", "LDY", "LDA", "LDX", "LAX",
	"CPY", "CMP", "NOP", "DCP", "CPY", "CMP", "DEC", "DCP",
	"INY", "CMP", "DEX", "AXS", "CPY", "CMP", "DEC", "DCP",
	"BNE", "CMP", "KIL", "DCP", "NOP", "CMP", "DEC", "DCP",
	"CLD", "CMP", "NOP", "DCP", "NOP", "CMP", "DEC", "DCP",
	"CPX", "SBC", "NOP", "ISC", "CPX", "SBC", "INC", "ISC",
	"INX", "SBC", "NOP", "SBC", "CPX", "SBC", "INC", "ISC",
	"BEQ", "SBC", "KIL", "ISC", "NOP", "SBC", "INC", "ISC",
	"SED", "SBC", "NOP", "ISC", "NOP", "SBC", "INC", "ISC",
};

#pragma endregion
