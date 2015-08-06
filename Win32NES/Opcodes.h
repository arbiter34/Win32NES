#pragma once
class Opcodes
{
public:
	Opcodes();
	~Opcodes();

	enum AddressMode{
		Immediate,
		ZeroPage,
		Absolute,
		Implicit,
		Accumulator,
		Indexed,
		ZeroPageIndexed,
		Indirect,
		PreIndexedIndirect,
		PostIndexedIndirect,
	};

	static void cpuNULL(unsigned char opcode);
	static void BRK(unsigned char opcode);
	static void ORAIndirectX(unsigned char opcode);
	static void ORAZeroPage(unsigned char opcode);
	static void ASLZeroPage(unsigned char opcode);
	static void PHP(unsigned char opcode);
	static void ORAImmediate(unsigned char opcode);
	static void ASLAccumulator(unsigned char opcode);
	static void ORAAbsolute(unsigned char opcode);
	static void ASLAbsolute(unsigned char opcode);
	static void BPL(unsigned char opcode);
	static void ORAIndirectY(unsigned char opcode);
	static void ORAZeroPageX(unsigned char opcode);
	static void ASLZeroPageX(unsigned char opcode);
	static void CLC(unsigned char opcode);
	static void ORAAbsoluteY(unsigned char opcode);
	static void ORAAbsoluteX(unsigned char opcode);
	static void ASLAbsoluteX(unsigned char opcode);
	static void JSR(unsigned char opcode);
	static void ANDIndirectX(unsigned char opcode);
	static void BITZeroPage(unsigned char opcode);
	static void ANDZeroPage(unsigned char opcode);
	static void ROLZeroPage(unsigned char opcode);
	static void PLP(unsigned char opcode);
	static void ANDImmediate(unsigned char opcode);
	static void ROLAccumulator(unsigned char opcode);
	static void BITAbsolute(unsigned char opcode);
	static void ANDAbsolute(unsigned char opcode);
	static void ROLAbsolute(unsigned char opcode);
	static void BMI(unsigned char opcode);
	static void ANDIndirectY(unsigned char opcode);
	static void ANDZeroPageX(unsigned char opcode);
	static void ROLZeroPageX(unsigned char opcode);
	static void SEC(unsigned char opcode);
	static void ANDAbsoluteY(unsigned char opcode);
	static void ANDAbsoluteX(unsigned char opcode);
	static void ROLAbsoluteX(unsigned char opcode);
	static void RTI(unsigned char opcode);
	static void EORIndirectX(unsigned char opcode);
	static void EORZeroPage(unsigned char opcode);
	static void LSRZeroPage(unsigned char opcode);
	static void PHA(unsigned char opcode);
	static void EORImmediate(unsigned char opcode);
	static void LSRAccumulator(unsigned char opcode);
	static void JMPAbsolute(unsigned char opcode);
	static void EORAbsolute(unsigned char opcode);
	static void LSRAbsolute(unsigned char opcode);
	static void BVC(unsigned char opcode);
	static void EORIndirectY(unsigned char opcode);
	static void EORZeroPageX(unsigned char opcode);
	static void LSRZeroPageX(unsigned char opcode);
	static void CLI(unsigned char opcode);
	static void EORAbsoluteY(unsigned char opcode);
	static void EORAbsoluteX(unsigned char opcode);
	static void LSRAbsoluteX(unsigned char opcode);
	static void RTS(unsigned char opcode);
	static void ADCIndirectX(unsigned char opcode);
	static void ADCZeroPage(unsigned char opcode);
	static void RORZeroPage(unsigned char opcode);
	static void PLA(unsigned char opcode);
	static void ADCImmediate(unsigned char opcode);
	static void RORAccumulator(unsigned char opcode);
	static void JMPIndirect(unsigned char opcode);
	static void ADCAbsolute(unsigned char opcode);
	static void RORAbsolute(unsigned char opcode);
	static void BVS(unsigned char opcode);
	static void ADCIndirectY(unsigned char opcode);
	static void ADCZeroPageX(unsigned char opcode);
	static void RORZeroPageX(unsigned char opcode);
	static void SEI(unsigned char opcode);
	static void ADCAbsoluteY(unsigned char opcode);
	static void ADCAbsoluteX(unsigned char opcode);
	static void RORAbsoluteX(unsigned char opcode);
	static void STAIndirectX(unsigned char opcode);
	static void STYZeroPage(unsigned char opcode);
	static void STAZeroPage(unsigned char opcode);
	static void STXZeroPage(unsigned char opcode);
	static void DEY(unsigned char opcode);
	static void TXA(unsigned char opcode);
	static void STYAbsolute(unsigned char opcode);
	static void STAAbsolute(unsigned char opcode);
	static void STXAbsoulte(unsigned char opcode);
	static void BCC(unsigned char opcode);
	static void STAIndirectY(unsigned char opcode);
	static void STYZeroPageX(unsigned char opcode);
	static void STAZeroPageX(unsigned char opcode);
	static void STXZeroPageY(unsigned char opcode);
	static void TYA(unsigned char opcode);
	static void STAAbsoluteY(unsigned char opcode);
	static void TXS(unsigned char opcode);
	static void STAAbsoluteX(unsigned char opcode);
	static void LDYImmediate(unsigned char opcode);
	static void LDAIndirectX(unsigned char opcode);
	static void LDXImmediate(unsigned char opcode);
	static void LDYZeroPage(unsigned char opcode);
	static void LDAZeroPage(unsigned char opcode);
	static void LDXZeroPage(unsigned char opcode);
	static void TAY(unsigned char opcode);
	static void LDAImmediate(unsigned char opcode);
	static void TAX(unsigned char opcode);
	static void LDYAbsolute(unsigned char opcode);
	static void LDAAbsolute(unsigned char opcode);
	static void LDXAbsolute(unsigned char opcode);
	static void BCS(unsigned char opcode);
	static void LDAIndirectY(unsigned char opcode);
	static void LDYZeroPageX(unsigned char opcode);
	static void LDAZeroPageX(unsigned char opcode);
	static void LDXZeroPageY(unsigned char opcode);
	static void CLV(unsigned char opcode);
	static void LDAAbsoluteY(unsigned char opcode);
	static void TSX(unsigned char opcode);
	static void LDYAbsoluteX(unsigned char opcode);
	static void LDAAbsoluteX(unsigned char opcode);
	static void LDXAbsoluteY(unsigned char opcode);
	static void CPYImmediate(unsigned char opcode);
	static void CMPIndirectX(unsigned char opcode);
	static void CPYZeroPage(unsigned char opcode);
	static void CMPZeroPage(unsigned char opcode);
	static void DECZeroPage(unsigned char opcode);
	static void INY(unsigned char opcode);
	static void CMPImmediate(unsigned char opcode);
	static void DEX(unsigned char opcode);
	static void CPYAbsolute(unsigned char opcode);
	static void CMPAbsolute(unsigned char opcode);
	static void DECAbsolute(unsigned char opcode);
	static void BNE(unsigned char opcode);
	static void CMPIndirectY(unsigned char opcode);
	static void CMPZeroPageX(unsigned char opcode);
	static void DECZeroPageX(unsigned char opcode);
	static void CLD(unsigned char opcode);
	static void CMPAbsoluteY(unsigned char opcode);
	static void CMPAbsoluteX(unsigned char opcode);
	static void DECAbsoluteX(unsigned char opcode);
	static void CPXImmediate(unsigned char opcode);
	static void SBCIndirectX(unsigned char opcode);
	static void CPXZeroPage(unsigned char opcode);
	static void SBCZeroPage(unsigned char opcode);
	static void INCZeroPage(unsigned char opcode);
	static void INX(unsigned char opcode);
	static void SBCImmediate(unsigned char opcode);
	static void NOP(unsigned char opcode);
	static void CPXAbsolute(unsigned char opcode);
	static void SBCAbsolute(unsigned char opcode);
	static void INCAbsolute(unsigned char opcode);
	static void BEQ(unsigned char opcode);
	static void SBCIndirectY(unsigned char opcode);
	static void SBCZeroPageX(unsigned char opcode);
	static void INCZeroPageX(unsigned char opcode);
	static void SED(unsigned char opcode);
	static void SBCAbsoluteY(unsigned char opcode);
	static void SBCAbsoluteX(unsigned char opcode);
	static void INCAbsoluteX(unsigned char opcode);

};

