
#include <iostream>
#include <sstream>

#include "Z80Disassembler.h"

///////////////////////////////////////
// Z80Disassembler
//
Z80Disassembler::Z80Disassembler() 
{
   // Par defaut, tout le monde pointe sur NOP
   for (unsigned int i = 0; i < 256; i++)
   {
      liste_opcodes_[i] = FillStructOpcode(1, "UNKNOWN");
      liste_opcodes_cb_[i] = FillStructOpcode(1, "UNKNOWN");
      liste_opcodes_ed_[i] = FillStructOpcode(1, "UNKNOWN");
      liste_opcodes_dd_[i] = FillStructOpcode(1, "UNKNOWN");
      liste_opcodes_fd_[i] = FillStructOpcode(1, "UNKNOWN");
   }

   liste_opcodes_[0xCB] = FillStructOpcode( 1, "%CB");
   liste_opcodes_[0xED] = FillStructOpcode( 1, "%ED");
   liste_opcodes_[0xDD] = FillStructOpcode( 1, "%DD");
   liste_opcodes_[0xFD] = FillStructOpcode( 1, "%FD");

   // Opcodes standards
   ///////////////////////////////////////////////////////////////////////////////////////
   /////////////                           FUNCTION       SIZE   DISASSMBLY
   liste_opcodes_[0x00] = FillStructOpcode(1, "NOP");
   liste_opcodes_[0x01] = FillStructOpcode(3, "LD BC, %nn__");
   liste_opcodes_[0x02] = FillStructOpcode(1, "LD (BC), A");
   liste_opcodes_[0x03] = FillStructOpcode(1, "INC BC");
   liste_opcodes_[0x04] = FillStructOpcode(1, "INC B");
   liste_opcodes_[0x05] = FillStructOpcode(1, "DEC B");
   liste_opcodes_[0x06] = FillStructOpcode(2, "LD B, %n");
   liste_opcodes_[0x07] = FillStructOpcode(1, "RLCA");
   liste_opcodes_[0x08] = FillStructOpcode(1, "EX AF AF'");
   liste_opcodes_[0x09] = FillStructOpcode(1, "ADD HL, BC");
   liste_opcodes_[0x0A] = FillStructOpcode(1, "LD A, (BC)");
   liste_opcodes_[0x0B] = FillStructOpcode(1, "DEC BC");
   liste_opcodes_[0x0C] = FillStructOpcode(1, "INC C");
   liste_opcodes_[0x0D] = FillStructOpcode(1, "DEC C");
   liste_opcodes_[0x0E] = FillStructOpcode(2, "LD C,  %n");
   liste_opcodes_[0x0F] = FillStructOpcode(1, "RRCA");
   liste_opcodes_[0x10] = FillStructOpcode(2, "DJNZ %j__");
   liste_opcodes_[0x11] = FillStructOpcode(3, "LD DE, %nn__");
   liste_opcodes_[0x12] = FillStructOpcode(1, "LD (DE), A");
   liste_opcodes_[0x13] = FillStructOpcode(1, "INC DE");
   liste_opcodes_[0x14] = FillStructOpcode(1, "INC D");
   liste_opcodes_[0x15] = FillStructOpcode(1, "DEC D");
   liste_opcodes_[0x16] = FillStructOpcode(2, "LD D,  %n");
   liste_opcodes_[0x17] = FillStructOpcode(1, "RLA");
   liste_opcodes_[0x18] = FillStructOpcode(2, "JR %j__");
   liste_opcodes_[0x19] = FillStructOpcode(1, "ADD HL, DE");
   liste_opcodes_[0x1A] = FillStructOpcode(1, "LD A, (DE)");
   liste_opcodes_[0x1B] = FillStructOpcode(1, "DEC DE");
   liste_opcodes_[0x1C] = FillStructOpcode(1, "INC E");
   liste_opcodes_[0x1D] = FillStructOpcode(1, "DEC E");
   liste_opcodes_[0x1E] = FillStructOpcode(2, "LD E,  %n");
   liste_opcodes_[0x1F] = FillStructOpcode(1, "RRA");
   liste_opcodes_[0x20] = FillStructOpcode(2, "JR NZ  %j__");
   liste_opcodes_[0x21] = FillStructOpcode(3, "LD HL, %nn__");
   liste_opcodes_[0x22] = FillStructOpcode(3, "LD (%nn__), HL");
   liste_opcodes_[0x23] = FillStructOpcode(1, "INC HL");
   liste_opcodes_[0x24] = FillStructOpcode(1, "INC H");
   liste_opcodes_[0x25] = FillStructOpcode(1, "DEC H");
   liste_opcodes_[0x26] = FillStructOpcode(2, "LD H,  %n");
   liste_opcodes_[0x27] = FillStructOpcode(1, "DAA");
   liste_opcodes_[0x28] = FillStructOpcode(2, "JR Z  %j__");
   liste_opcodes_[0x29] = FillStructOpcode(1, "ADD HL, HL");
   liste_opcodes_[0x2A] = FillStructOpcode(3, "LD HL, (%nn__)");
   liste_opcodes_[0x2B] = FillStructOpcode(1, "DEC HL");
   liste_opcodes_[0x2C] = FillStructOpcode(1, "INC L");
   liste_opcodes_[0x2D] = FillStructOpcode(1, "DEC L");
   liste_opcodes_[0x2E] = FillStructOpcode(2, "LD L,  %n");
   liste_opcodes_[0x2F] = FillStructOpcode(1, "CPL");
   liste_opcodes_[0x30] = FillStructOpcode(2, "JR NC  %j__");
   liste_opcodes_[0x31] = FillStructOpcode(3, "LD SP, %nn__");
   liste_opcodes_[0x32] = FillStructOpcode(3, "LD (%nn__), A");
   liste_opcodes_[0x33] = FillStructOpcode(1, "INC SP");
   liste_opcodes_[0x34] = FillStructOpcode(1, "INC (HL)");
   liste_opcodes_[0x35] = FillStructOpcode(1, "DEC (HL)");
   liste_opcodes_[0x36] = FillStructOpcode(2, "LD (HL), %n");
   liste_opcodes_[0x37] = FillStructOpcode(1, "SCF");
   liste_opcodes_[0x38] = FillStructOpcode(2, "JR C, %j__");
   liste_opcodes_[0x39] = FillStructOpcode(1, "ADD HL, SP");
   liste_opcodes_[0x3A] = FillStructOpcode(3, "LD A, (%nn__)");
   liste_opcodes_[0x3B] = FillStructOpcode(1, "DEC SP");
   liste_opcodes_[0x3C] = FillStructOpcode(1, "INC A");
   liste_opcodes_[0x3D] = FillStructOpcode(1, "DEC A");
   liste_opcodes_[0x3E] = FillStructOpcode(2, "LD A, %n");
   liste_opcodes_[0x3F] = FillStructOpcode(1, "CCF");
   liste_opcodes_[0x40] = FillStructOpcode(1, "LD B, B");
   liste_opcodes_[0x41] = FillStructOpcode(1, "LD B, C");
   liste_opcodes_[0x42] = FillStructOpcode(1, "LD B, D");
   liste_opcodes_[0x43] = FillStructOpcode(1, "LD B, E");
   liste_opcodes_[0x44] = FillStructOpcode(1, "LD B, H");
   liste_opcodes_[0x45] = FillStructOpcode(1, "LD B, L");
   liste_opcodes_[0x46] = FillStructOpcode(1, "LD B, (HL)");
   liste_opcodes_[0x47] = FillStructOpcode(1, "LD B, A");
   liste_opcodes_[0x48] = FillStructOpcode(1, "LD C, B");
   liste_opcodes_[0x49] = FillStructOpcode(1, "LD C, C");
   liste_opcodes_[0x4A] = FillStructOpcode(1, "LD C, D");
   liste_opcodes_[0x4B] = FillStructOpcode(1, "LD C, E");
   liste_opcodes_[0x4C] = FillStructOpcode(1, "LD C, H");
   liste_opcodes_[0x4D] = FillStructOpcode(1, "LD C, L");
   liste_opcodes_[0x4E] = FillStructOpcode(1, "LD C, (HL)");
   liste_opcodes_[0x4F] = FillStructOpcode(1, "LD C, A");
   liste_opcodes_[0x50] = FillStructOpcode(1, "LD D, B");
   liste_opcodes_[0x51] = FillStructOpcode(1, "LD D, C");
   liste_opcodes_[0x52] = FillStructOpcode(1, "LD D, D");
   liste_opcodes_[0x53] = FillStructOpcode(1, "LD D, E");
   liste_opcodes_[0x54] = FillStructOpcode(1, "LD D, H");
   liste_opcodes_[0x55] = FillStructOpcode(1, "LD D, L");
   liste_opcodes_[0x56] = FillStructOpcode(1, "LD D, (HL)");
   liste_opcodes_[0x57] = FillStructOpcode(1, "LD D, A");
   liste_opcodes_[0x58] = FillStructOpcode(1, "LD E, B");
   liste_opcodes_[0x59] = FillStructOpcode(1, "LD E, C");
   liste_opcodes_[0x5A] = FillStructOpcode(1, "LD E, D");
   liste_opcodes_[0x5B] = FillStructOpcode(1, "LD E, E");
   liste_opcodes_[0x5C] = FillStructOpcode(1, "LD E, H");
   liste_opcodes_[0x5D] = FillStructOpcode(1, "LD E, L");
   liste_opcodes_[0x5E] = FillStructOpcode(1, "LD E, (HL)");
   liste_opcodes_[0x5F] = FillStructOpcode(1, "LD E, A");
   liste_opcodes_[0x60] = FillStructOpcode(1, "LD H, B");
   liste_opcodes_[0x61] = FillStructOpcode(1, "LD H, C");
   liste_opcodes_[0x62] = FillStructOpcode(1, "LD H, D");
   liste_opcodes_[0x63] = FillStructOpcode(1, "LD H, E");
   liste_opcodes_[0x64] = FillStructOpcode(1, "LD H, H");
   liste_opcodes_[0x65] = FillStructOpcode(1, "LD H, L");
   liste_opcodes_[0x66] = FillStructOpcode(1, "LD H, (HL)");
   liste_opcodes_[0x67] = FillStructOpcode(1, "LD H, A");
   liste_opcodes_[0x68] = FillStructOpcode(1, "LD L, B");
   liste_opcodes_[0x69] = FillStructOpcode(1, "LD L, C");
   liste_opcodes_[0x6A] = FillStructOpcode(1, "LD L, D");
   liste_opcodes_[0x6B] = FillStructOpcode(1, "LD L, E");
   liste_opcodes_[0x6C] = FillStructOpcode(1, "LD L, H");
   liste_opcodes_[0x6D] = FillStructOpcode(1, "LD L, L");
   liste_opcodes_[0x6E] = FillStructOpcode(1, "LD L, (HL)");
   liste_opcodes_[0x6F] = FillStructOpcode(1, "LD L, A");
   liste_opcodes_[0x70] = FillStructOpcode(1, "LD (HL), B");
   liste_opcodes_[0x71] = FillStructOpcode(1, "LD (HL), C");
   liste_opcodes_[0x72] = FillStructOpcode(1, "LD (HL), D");
   liste_opcodes_[0x73] = FillStructOpcode(1, "LD (HL), E");
   liste_opcodes_[0x74] = FillStructOpcode(1, "LD (HL), H");
   liste_opcodes_[0x75] = FillStructOpcode(1, "LD (HL), L");
   liste_opcodes_[0x76] = FillStructOpcode(1, "HALT");
   liste_opcodes_[0x77] = FillStructOpcode(1, "LD (HL), A");
   liste_opcodes_[0x78] = FillStructOpcode(1, "LD A, B");
   liste_opcodes_[0x79] = FillStructOpcode(1, "LD A, C");
   liste_opcodes_[0x7A] = FillStructOpcode(1, "LD A, D");
   liste_opcodes_[0x7B] = FillStructOpcode(1, "LD A, E");
   liste_opcodes_[0x7C] = FillStructOpcode(1, "LD A, H");
   liste_opcodes_[0x7D] = FillStructOpcode(1, "LD A, L");
   liste_opcodes_[0x7E] = FillStructOpcode(1, "LD A, (HL)");
   liste_opcodes_[0x7F] = FillStructOpcode(1, "LD A, A");
   liste_opcodes_[0x80] = FillStructOpcode(1, "ADD A, B");
   liste_opcodes_[0x81] = FillStructOpcode(1, "ADD A, C");
   liste_opcodes_[0x82] = FillStructOpcode(1, "ADD A, D");
   liste_opcodes_[0x83] = FillStructOpcode(1, "ADD A, E");
   liste_opcodes_[0x84] = FillStructOpcode(1, "ADD A, H");
   liste_opcodes_[0x85] = FillStructOpcode(1, "ADD A, L");
   liste_opcodes_[0x86] = FillStructOpcode(1, "ADD A, (HL)");
   liste_opcodes_[0x87] = FillStructOpcode(1, "ADD A, A");
   liste_opcodes_[0x88] = FillStructOpcode(1, "ADC A, B");
   liste_opcodes_[0x89] = FillStructOpcode(1, "ADC A, C");
   liste_opcodes_[0x8A] = FillStructOpcode(1, "ADC A, D");
   liste_opcodes_[0x8B] = FillStructOpcode(1, "ADC A, E");
   liste_opcodes_[0x8C] = FillStructOpcode(1, "ADC A, H");
   liste_opcodes_[0x8D] = FillStructOpcode(1, "ADC A, L");
   liste_opcodes_[0x8E] = FillStructOpcode(1, "ADC A, (HL)");
   liste_opcodes_[0x8F] = FillStructOpcode(1, "ADC A, A");
   liste_opcodes_[0x90] = FillStructOpcode(1, "SUB A, B");
   liste_opcodes_[0x91] = FillStructOpcode(1, "SUB A, C");
   liste_opcodes_[0x92] = FillStructOpcode(1, "SUB A, D");
   liste_opcodes_[0x93] = FillStructOpcode(1, "SUB A, E");
   liste_opcodes_[0x94] = FillStructOpcode(1, "SUB A, H");
   liste_opcodes_[0x95] = FillStructOpcode(1, "SUB A, L");
   liste_opcodes_[0x96] = FillStructOpcode(1, "SUB A, (HL)");
   liste_opcodes_[0x97] = FillStructOpcode(1, "SUB A, A");
   liste_opcodes_[0x98] = FillStructOpcode(1, "SBC A, B");
   liste_opcodes_[0x99] = FillStructOpcode(1, "SBC A, C");
   liste_opcodes_[0x9A] = FillStructOpcode(1, "SBC A, D");
   liste_opcodes_[0x9B] = FillStructOpcode(1, "SBC A, E");
   liste_opcodes_[0x9C] = FillStructOpcode(1, "SBC A, H");
   liste_opcodes_[0x9D] = FillStructOpcode(1, "SBC A, L");
   liste_opcodes_[0x9E] = FillStructOpcode(1, "SBC A, (HL)");
   liste_opcodes_[0x9F] = FillStructOpcode(1, "SBC A, A");
   liste_opcodes_[0xA0] = FillStructOpcode(1, "AND B");
   liste_opcodes_[0xA1] = FillStructOpcode(1, "AND C");
   liste_opcodes_[0xA2] = FillStructOpcode(1, "AND D");
   liste_opcodes_[0xA3] = FillStructOpcode(1, "AND E");
   liste_opcodes_[0xA4] = FillStructOpcode(1, "AND H");
   liste_opcodes_[0xA5] = FillStructOpcode(1, "AND L");
   liste_opcodes_[0xA6] = FillStructOpcode(1, "AND (HL)");
   liste_opcodes_[0xA7] = FillStructOpcode(1, "AND A");
   liste_opcodes_[0xA8] = FillStructOpcode(1, "XOR B");
   liste_opcodes_[0xA9] = FillStructOpcode(1, "XOR C");
   liste_opcodes_[0xAA] = FillStructOpcode(1, "XOR D");
   liste_opcodes_[0xAB] = FillStructOpcode(1, "XOR E");
   liste_opcodes_[0xAC] = FillStructOpcode(1, "XOR H");
   liste_opcodes_[0xAD] = FillStructOpcode(1, "XOR L");
   liste_opcodes_[0xAE] = FillStructOpcode(1, "XOR (HL)");
   liste_opcodes_[0xAF] = FillStructOpcode(1, "XOR A");
   liste_opcodes_[0xB0] = FillStructOpcode(1, "OR B");
   liste_opcodes_[0xB1] = FillStructOpcode(1, "OR C");
   liste_opcodes_[0xB2] = FillStructOpcode(1, "OR D");
   liste_opcodes_[0xB3] = FillStructOpcode(1, "OR E");
   liste_opcodes_[0xB4] = FillStructOpcode(1, "OR H");
   liste_opcodes_[0xB5] = FillStructOpcode(1, "OR L");
   liste_opcodes_[0xB6] = FillStructOpcode(1, "OR (HL)");
   liste_opcodes_[0xB7] = FillStructOpcode(1, "OR A");
   liste_opcodes_[0xB8] = FillStructOpcode(1, "CP B");
   liste_opcodes_[0xB9] = FillStructOpcode(1, "CP C");
   liste_opcodes_[0xBA] = FillStructOpcode(1, "CP D");
   liste_opcodes_[0xBB] = FillStructOpcode(1, "CP E");
   liste_opcodes_[0xBC] = FillStructOpcode(1, "CP H");
   liste_opcodes_[0xBD] = FillStructOpcode(1, "CP L");
   liste_opcodes_[0xBE] = FillStructOpcode(1, "CP (HL)");
   liste_opcodes_[0xBF] = FillStructOpcode(1, "CP A");
   liste_opcodes_[0xC0] = FillStructOpcode(1, "RET NZ");
   liste_opcodes_[0xC1] = FillStructOpcode(1, "POP BC");
   liste_opcodes_[0xC2] = FillStructOpcode(3, "JP NZ %nn__");
   liste_opcodes_[0xC3] = FillStructOpcode(3, "JP %nn__");
   liste_opcodes_[0xC4] = FillStructOpcode(3, "CALL NZ %nn__");
   liste_opcodes_[0xC5] = FillStructOpcode(1, "PUSH BC");
   liste_opcodes_[0xC6] = FillStructOpcode(2, "ADD A, %n");
   liste_opcodes_[0xC7] = FillStructOpcode(1, "RST 0");
   liste_opcodes_[0xC8] = FillStructOpcode(1, "RET Z");
   liste_opcodes_[0xC9] = FillStructOpcode(1, "RET");
   liste_opcodes_[0xCA] = FillStructOpcode(3, "JP Z %nn__");
   liste_opcodes_[0xCC] = FillStructOpcode(3, "CALL Z %nn__");
   liste_opcodes_[0xCD] = FillStructOpcode(3, "CALL %nn__");
   liste_opcodes_[0xCE] = FillStructOpcode(2, "ADC A, %n");
   liste_opcodes_[0xCF] = FillStructOpcode(1, "RST 08H");
   liste_opcodes_[0xD0] = FillStructOpcode(1, "RET NC");
   liste_opcodes_[0xD1] = FillStructOpcode(1, "POP DE");
   liste_opcodes_[0xD2] = FillStructOpcode(3, "JP NC %nn__");
   liste_opcodes_[0xD3] = FillStructOpcode(2, "OUT (%n), A");
   liste_opcodes_[0xD4] = FillStructOpcode(3, "CALL NC %nn__");
   liste_opcodes_[0xD5] = FillStructOpcode(1, "PUSH DE");
   liste_opcodes_[0xD6] = FillStructOpcode(2, "SUB A, %n");
   liste_opcodes_[0xD7] = FillStructOpcode(1, "RST 10H");
   liste_opcodes_[0xD8] = FillStructOpcode(1, "RET C");
   liste_opcodes_[0xD9] = FillStructOpcode(1, "EXX");
   liste_opcodes_[0xDA] = FillStructOpcode(3, "JP C %nn__");
   liste_opcodes_[0xDB] = FillStructOpcode(2, "IN A, (%n)");
   liste_opcodes_[0xDC] = FillStructOpcode(3, "CALL C %nn__");
   liste_opcodes_[0xDE] = FillStructOpcode(2, "SBC A, %n");
   liste_opcodes_[0xDF] = FillStructOpcode(1, "RST 18H");
   liste_opcodes_[0xE0] = FillStructOpcode(1, "RET PO");
   liste_opcodes_[0xE1] = FillStructOpcode(1, "POP HL");
   liste_opcodes_[0xE2] = FillStructOpcode(3, "JP PO %nn__");
   liste_opcodes_[0xE3] = FillStructOpcode(1, "EX (SP), HL");
   liste_opcodes_[0xE4] = FillStructOpcode(3, "CALL PO %nn__");
   liste_opcodes_[0xE5] = FillStructOpcode(1, "PUSH HL");
   liste_opcodes_[0xE6] = FillStructOpcode(2, "AND %n");
   liste_opcodes_[0xE7] = FillStructOpcode(1, "RST 20H");
   liste_opcodes_[0xE8] = FillStructOpcode(1, "RET PE");
   liste_opcodes_[0xE9] = FillStructOpcode(1, "JP (HL)");
   liste_opcodes_[0xEA] = FillStructOpcode(3, "JP PE %nn__");
   liste_opcodes_[0xEB] = FillStructOpcode(1, "EX DE,HL");
   liste_opcodes_[0xEC] = FillStructOpcode(3, "CALL PE %nn__");
   liste_opcodes_[0xEE] = FillStructOpcode(2, "XOR %n");
   liste_opcodes_[0xEF] = FillStructOpcode(1, "RST 28H");
   liste_opcodes_[0xF0] = FillStructOpcode(1, "RET P");
   liste_opcodes_[0xF1] = FillStructOpcode(1, "POP AF");
   liste_opcodes_[0xF2] = FillStructOpcode(3, "JP P %nn__");
   liste_opcodes_[0xF3] = FillStructOpcode(1, "DI");
   liste_opcodes_[0xF4] = FillStructOpcode(3, "CALL P %nn__");
   liste_opcodes_[0xF5] = FillStructOpcode(1, "PUSH AF");
   liste_opcodes_[0xF6] = FillStructOpcode(2, "OR %n");
   liste_opcodes_[0xF7] = FillStructOpcode(1, "RST 30H");
   liste_opcodes_[0xF8] = FillStructOpcode(1, "RET M");
   liste_opcodes_[0xF9] = FillStructOpcode(1, "LD SP, HL");
   liste_opcodes_[0xFA] = FillStructOpcode(3, "JP M %nn__");
   liste_opcodes_[0xFB] = FillStructOpcode(1, "EI");
   liste_opcodes_[0xFC] = FillStructOpcode(3, "CALL M %nn__");
   liste_opcodes_[0xFE] = FillStructOpcode(2, "CP %n");
   liste_opcodes_[0xFF] = FillStructOpcode(1, "RST 38H");

   //////////////////
   // CB

#define FILL_CB_FUNC(base,asm)\
   liste_opcodes_cb_[base+0] = FillStructOpcode(1, asm);\
   liste_opcodes_cb_[base+1] = FillStructOpcode(1, asm);\
   liste_opcodes_cb_[base+2] = FillStructOpcode(1, asm);\
   liste_opcodes_cb_[base+3] = FillStructOpcode(1, asm);\
   liste_opcodes_cb_[base+4] = FillStructOpcode(1, asm);\
   liste_opcodes_cb_[base+5] = FillStructOpcode(1, asm);\
   liste_opcodes_cb_[base+6] = FillStructOpcode(1, asm);\
   liste_opcodes_cb_[base+7] = FillStructOpcode(1, asm);\

   char Buffer_Tmp[64];
#define FILL_CB_FUNC_GENERIC(str_asm, bit_num, reg, base)\
   sprintf(Buffer_Tmp, #str_asm " %i, %s", bit_num, REG_TO_STR(reg)); liste_opcodes_cb_[base+ (8 * bit_num)] =  FillStructOpcode( 1, Buffer_Tmp);\

#define FILL_CB_FUNC_BIT(str_asm, bit_num, base)\
   FILL_CB_FUNC_GENERIC(str_asm, bit_num, R_B, base) \
   FILL_CB_FUNC_GENERIC(str_asm, bit_num, R_C, base+1) \
   FILL_CB_FUNC_GENERIC(str_asm, bit_num, R_D, base+2) \
   FILL_CB_FUNC_GENERIC(str_asm, bit_num, R_E, base+3) \
   FILL_CB_FUNC_GENERIC(str_asm, bit_num, R_H, base+4) \
   FILL_CB_FUNC_GENERIC(str_asm, bit_num, R_L, base+5) \
   FILL_CB_FUNC_GENERIC(str_asm, bit_num, R_A, base+7) \
   sprintf(Buffer_Tmp, #str_asm " %i, %s", bit_num, "(HL)"); liste_opcodes_cb_[base+6 + (8 * bit_num) ] = FillStructOpcode(1, Buffer_Tmp);

#define FILL_CB_FUNC_GEN(str_asm, base)\
   FILL_CB_FUNC_BIT(str_asm,0,base)\
   FILL_CB_FUNC_BIT(str_asm,1,base)\
   FILL_CB_FUNC_BIT(str_asm,2,base)\
   FILL_CB_FUNC_BIT(str_asm,3,base)\
   FILL_CB_FUNC_BIT(str_asm,4,base)\
   FILL_CB_FUNC_BIT(str_asm,5,base)\
   FILL_CB_FUNC_BIT(str_asm,6,base)\
   FILL_CB_FUNC_BIT(str_asm,7,base)

   FILL_CB_FUNC(0x00, "RLC %r");
   FILL_CB_FUNC(0x08, "RRC %r");
   FILL_CB_FUNC(0x10, "RL %r");
   FILL_CB_FUNC(0x18, "RR %r");
   FILL_CB_FUNC(0x20, "SLA %r");
   FILL_CB_FUNC(0x28, "SRA %r");
   FILL_CB_FUNC(0x30, "SLL %r");
   FILL_CB_FUNC(0x38, "SRL %r");

   FILL_CB_FUNC_GEN("BIT", 0x40);
   FILL_CB_FUNC_GEN("RES", 0x80);
   FILL_CB_FUNC_GEN("SES", 0xC0);

   //////////////////
   // ED  
   liste_opcodes_ed_[0x40] = FillStructOpcode(1, "IN B, (C)");
   liste_opcodes_ed_[0x41] = FillStructOpcode(1, "OUT (C), B");
   liste_opcodes_ed_[0x42] = FillStructOpcode(1, "SBC HL, BC");
   liste_opcodes_ed_[0x43] = FillStructOpcode(3, "LD (%nn__), BC");
   liste_opcodes_ed_[0x44] = FillStructOpcode(1, "NEG");
   liste_opcodes_ed_[0x45] = FillStructOpcode(1, "RETN");
   liste_opcodes_ed_[0x46] = FillStructOpcode(1, "IM 0");
   liste_opcodes_ed_[0x47] = FillStructOpcode(1, "LD I, A");
   liste_opcodes_ed_[0x48] = FillStructOpcode(1, "IN C, (C)");
   liste_opcodes_ed_[0x49] = FillStructOpcode(1, "OUT (C), C");
   liste_opcodes_ed_[0x4A] = FillStructOpcode(1, "ADC HL, BC");
   liste_opcodes_ed_[0x4B] = FillStructOpcode(3, "LD BC, (%nn__)");
   liste_opcodes_ed_[0x4C] = FillStructOpcode(1, "NEG");
   liste_opcodes_ed_[0x4D] = FillStructOpcode(1, "RETI");
   liste_opcodes_ed_[0x4E] = FillStructOpcode(1, "IM 0");
   liste_opcodes_ed_[0x4F] = FillStructOpcode(1, "LD R, A");
   liste_opcodes_ed_[0x50] = FillStructOpcode(1, "IN D, (C)");
   liste_opcodes_ed_[0x51] = FillStructOpcode(1, "OUT (C), D");
   liste_opcodes_ed_[0x52] = FillStructOpcode(1, "SBC HL, DE");
   liste_opcodes_ed_[0x53] = FillStructOpcode(3, "LD (%nn__), DE");
   liste_opcodes_ed_[0x54] = FillStructOpcode(1, "NEG");
   liste_opcodes_ed_[0x55] = FillStructOpcode(1, "RETN");
   liste_opcodes_ed_[0x56] = FillStructOpcode(1, "IM 1");
   liste_opcodes_ed_[0x57] = FillStructOpcode(1, "LD A, I");
   liste_opcodes_ed_[0x58] = FillStructOpcode(1, "IN E, (C)");
   liste_opcodes_ed_[0x59] = FillStructOpcode(1, "OUT (C), E");
   liste_opcodes_ed_[0x5A] = FillStructOpcode(1, "ADC HL, DE");
   liste_opcodes_ed_[0x5B] = FillStructOpcode(3, "LD DE, (%nn__)");
   liste_opcodes_ed_[0x5C] = FillStructOpcode(1, "NEG");
   liste_opcodes_ed_[0x5D] = FillStructOpcode(1, "RETN");
   liste_opcodes_ed_[0x5E] = FillStructOpcode(1, "IM2");
   liste_opcodes_ed_[0x5F] = FillStructOpcode(1, "LD A, R");
   liste_opcodes_ed_[0x60] = FillStructOpcode(1, "IN H, (C)");
   liste_opcodes_ed_[0x61] = FillStructOpcode(1, "OUT (C), H");
   liste_opcodes_ed_[0x62] = FillStructOpcode(1, "SBC HL, HL");
   liste_opcodes_ed_[0x63] = FillStructOpcode(3, "LN (%nn__), HL");
   liste_opcodes_ed_[0x64] = FillStructOpcode(1, "NEG");
   liste_opcodes_ed_[0x65] = FillStructOpcode(1, "RETN");
   liste_opcodes_ed_[0x66] = FillStructOpcode(1, "IM 0");
   liste_opcodes_ed_[0x67] = FillStructOpcode(1, "RRD");
   liste_opcodes_ed_[0x68] = FillStructOpcode(1, "IN L, (C)");
   liste_opcodes_ed_[0x69] = FillStructOpcode(1, "OUT (C), L");
   liste_opcodes_ed_[0x6A] = FillStructOpcode(1, "ADC HL, HL");
   liste_opcodes_ed_[0x6B] = FillStructOpcode(3, "LN HL, (%nn__)");
   liste_opcodes_ed_[0x6C] = FillStructOpcode(1, "NEG");
   liste_opcodes_ed_[0x6D] = FillStructOpcode(1, "RETN");
   liste_opcodes_ed_[0x6E] = FillStructOpcode(1, "IM 0");
   liste_opcodes_ed_[0x6F] = FillStructOpcode(1, "RLD");
   liste_opcodes_ed_[0x70] = FillStructOpcode(1, "IN (C)");
   liste_opcodes_ed_[0x71] = FillStructOpcode(1, "OUT (C), 0");
   liste_opcodes_ed_[0x72] = FillStructOpcode(1, "SBC HL, SP");
   liste_opcodes_ed_[0x73] = FillStructOpcode(3, "LD (%nn__), SP");
   liste_opcodes_ed_[0x74] = FillStructOpcode(1, "NEG");
   liste_opcodes_ed_[0x75] = FillStructOpcode(1, "RETN");
   liste_opcodes_ed_[0x76] = FillStructOpcode(1, "IM 1");
   liste_opcodes_ed_[0x77] = FillStructOpcode(1, "NOP");
   liste_opcodes_ed_[0x78] = FillStructOpcode(1, "IN A, (C)");
   liste_opcodes_ed_[0x79] = FillStructOpcode(1, "OUT (C), A");
   liste_opcodes_ed_[0x7A] = FillStructOpcode(1, "ADC HL, SP");
   liste_opcodes_ed_[0x7B] = FillStructOpcode(3, "LD SP, (%nn__)");
   liste_opcodes_ed_[0x7C] = FillStructOpcode(1, "NEG");
   liste_opcodes_ed_[0x7D] = FillStructOpcode(1, "RETN");
   liste_opcodes_ed_[0x7E] = FillStructOpcode(1, "IM 2");
   liste_opcodes_ed_[0x7F] = FillStructOpcode(1, "NOP");
   liste_opcodes_ed_[0xA0] = FillStructOpcode(1, "LDI");
   liste_opcodes_ed_[0xA1] = FillStructOpcode(1, "CPI");
   liste_opcodes_ed_[0xA2] = FillStructOpcode(1, "INI");
   liste_opcodes_ed_[0xA3] = FillStructOpcode(1, "OUTI");
   liste_opcodes_ed_[0xA8] = FillStructOpcode(1, "LDD");
   liste_opcodes_ed_[0xA9] = FillStructOpcode(1, "CPD");
   liste_opcodes_ed_[0xAA] = FillStructOpcode(1, "IND");
   liste_opcodes_ed_[0xAB] = FillStructOpcode(1, "OUTD");
   liste_opcodes_ed_[0xB0] = FillStructOpcode(1, "LDIR");
   liste_opcodes_ed_[0xB1] = FillStructOpcode(1, "CPIR");
   liste_opcodes_ed_[0xB2] = FillStructOpcode(1, "INIR");
   liste_opcodes_ed_[0xB3] = FillStructOpcode(1, "OTIR");
   liste_opcodes_ed_[0xB8] = FillStructOpcode(1, "LDDR");
   liste_opcodes_ed_[0xB9] = FillStructOpcode(1, "CPDR");
   liste_opcodes_ed_[0xBA] = FillStructOpcode(1, "INDR");
   liste_opcodes_ed_[0xBB] = FillStructOpcode(1, "OTDR");
   liste_opcodes_ed_[0xED] = FillStructOpcode(1, "%ED");

   //////////////////
   // DD
   // Act like normal opcode, except that HL is replaced by IX.
   for (unsigned int j = 0x00; j <= 0xff; j++)
   {
      liste_opcodes_dd_[j] = liste_opcodes_[j];
   }
   liste_opcodes_dd_[0x09] = FillStructOpcode(1, "ADD IX, BC");
   liste_opcodes_dd_[0x19] = FillStructOpcode(1, "ADD IX, DE");
   liste_opcodes_dd_[0x21] = FillStructOpcode(3, "LD IX, %nn__");
   liste_opcodes_dd_[0x22] = FillStructOpcode(3, "LD (%nn__), IX");
   liste_opcodes_dd_[0x23] = FillStructOpcode(1, "INC IX");
   liste_opcodes_dd_[0x24] = FillStructOpcode(1, "INC IXh");
   liste_opcodes_dd_[0x25] = FillStructOpcode(1, "DEC IXh");
   liste_opcodes_dd_[0x26] = FillStructOpcode(2, "LD IXh, %n");
   liste_opcodes_dd_[0x29] = FillStructOpcode(1, "ADD IX, IX");
   liste_opcodes_dd_[0x2A] = FillStructOpcode(3, "LD IX, (%nn__)");
   liste_opcodes_dd_[0x2B] = FillStructOpcode(1, "DEC IX");
   liste_opcodes_dd_[0x2C] = FillStructOpcode(1, "INC IXl");
   liste_opcodes_dd_[0x2D] = FillStructOpcode(1, "DEC IXl");
   liste_opcodes_dd_[0x2E] = FillStructOpcode(2, "LD IXl, %n");
   liste_opcodes_dd_[0x34] = FillStructOpcode(1, "INC (IX+%n)");
   liste_opcodes_dd_[0x35] = FillStructOpcode(1, "DEC (IX+%n)");
   liste_opcodes_dd_[0x36] = FillStructOpcode(3, "LD (IX+%n), %n");
   liste_opcodes_dd_[0x39] = FillStructOpcode(1, "ADD IX, SP");
   liste_opcodes_dd_[0x44] = FillStructOpcode(2, "LD B, IXh");
   liste_opcodes_dd_[0x45] = FillStructOpcode(2, "LD B, IXl");
   liste_opcodes_dd_[0x46] = FillStructOpcode(2, "LD B, (IX+%n)");
   liste_opcodes_dd_[0x4C] = FillStructOpcode(2, "LD C, IXh");
   liste_opcodes_dd_[0x4D] = FillStructOpcode(2, "LD C, IXl");
   liste_opcodes_dd_[0x4E] = FillStructOpcode(2, "LD C, (IX+%n)");
   liste_opcodes_dd_[0x54] = FillStructOpcode(2, "LD D, IXh");
   liste_opcodes_dd_[0x55] = FillStructOpcode(2, "LD D, IXl");
   liste_opcodes_dd_[0x56] = FillStructOpcode(2, "LD D, (IX+%n)");
   liste_opcodes_dd_[0x5C] = FillStructOpcode(2, "LD E, IXh");
   liste_opcodes_dd_[0x5D] = FillStructOpcode(2, "LD E, IXl");
   liste_opcodes_dd_[0x5E] = FillStructOpcode(2, "LD E, (IX+%n)");
   liste_opcodes_dd_[0x60] = FillStructOpcode(2, "LD IXh, B");
   liste_opcodes_dd_[0x61] = FillStructOpcode(2, "LD IXh, C");
   liste_opcodes_dd_[0x62] = FillStructOpcode(2, "LD IXh, D");
   liste_opcodes_dd_[0x63] = FillStructOpcode(2, "LD IXh, E");
   liste_opcodes_dd_[0x64] = FillStructOpcode(2, "LD IXh, IXh");
   liste_opcodes_dd_[0x65] = FillStructOpcode(2, "LD IXh, IXl");
   liste_opcodes_dd_[0x66] = FillStructOpcode(2, "LD H, (IX+%n)");
   liste_opcodes_dd_[0x67] = FillStructOpcode(2, "LD IXh, A");
   liste_opcodes_dd_[0x68] = FillStructOpcode(2, "LD IXl, B");
   liste_opcodes_dd_[0x69] = FillStructOpcode(2, "LD IXl, C");
   liste_opcodes_dd_[0x6A] = FillStructOpcode(2, "LD IXl, D");
   liste_opcodes_dd_[0x6B] = FillStructOpcode(2, "LD IXl, E");
   liste_opcodes_dd_[0x6C] = FillStructOpcode(2, "LD IXl, IXh");
   liste_opcodes_dd_[0x6D] = FillStructOpcode(2, "LD IXl, IXl");
   liste_opcodes_dd_[0x6E] = FillStructOpcode(2, "LD L, (IX+%n)");
   liste_opcodes_dd_[0x6F] = FillStructOpcode(2, "LD IXl, A");
   liste_opcodes_dd_[0x70] = FillStructOpcode(2, "LD (IX+%n), B");
   liste_opcodes_dd_[0x71] = FillStructOpcode(2, "LD (IX+%n), C");
   liste_opcodes_dd_[0x72] = FillStructOpcode(2, "LD (IX+%n), D");
   liste_opcodes_dd_[0x73] = FillStructOpcode(2, "LD (IX+%n), E");
   liste_opcodes_dd_[0x74] = FillStructOpcode(2, "LD (IX+%n), H");
   liste_opcodes_dd_[0x75] = FillStructOpcode(2, "LD (IX+%n), L");
   liste_opcodes_dd_[0x77] = FillStructOpcode(2, "LD (IX+%n), A");
   liste_opcodes_dd_[0x7C] = FillStructOpcode(2, "LD A, IXh");
   liste_opcodes_dd_[0x7D] = FillStructOpcode(1, "LD A, IXl");
   liste_opcodes_dd_[0x7E] = FillStructOpcode(2, "LD A, (IX+%n)");
   liste_opcodes_dd_[0x84] = FillStructOpcode(2, "ADD A, IXh");
   liste_opcodes_dd_[0x85] = FillStructOpcode(2, "ADD A, IXl");
   liste_opcodes_dd_[0x86] = FillStructOpcode(2, "ADD A, (IX+%n)");
   liste_opcodes_dd_[0x8C] = FillStructOpcode(2, "ADC A, IXh");
   liste_opcodes_dd_[0x8D] = FillStructOpcode(2, "ADC A, IXl");
   liste_opcodes_dd_[0x8E] = FillStructOpcode(2, "ADC A, (IX+%n)");
   liste_opcodes_dd_[0x94] = FillStructOpcode(2, "SUB A, IXh");
   liste_opcodes_dd_[0x95] = FillStructOpcode(2, "SUB A, IXl");
   liste_opcodes_dd_[0x96] = FillStructOpcode(2, "SUB A, (IX+%n)");
   liste_opcodes_dd_[0x9C] = FillStructOpcode(2, "SBC A, IXh");
   liste_opcodes_dd_[0x9D] = FillStructOpcode(2, "SBC A, IXl");
   liste_opcodes_dd_[0x9E] = FillStructOpcode(2, "SBC A, (IX+%n)");
   liste_opcodes_dd_[0xA4] = FillStructOpcode(2, "AND A, IXh");
   liste_opcodes_dd_[0xA5] = FillStructOpcode(2, "AND A, IXl");
   liste_opcodes_dd_[0xA6] = FillStructOpcode(2, "AND (IX+%n)");
   liste_opcodes_dd_[0xAC] = FillStructOpcode(2, "XOR A, IXh");
   liste_opcodes_dd_[0xAD] = FillStructOpcode(2, "XOR A, IXl");
   liste_opcodes_dd_[0xAE] = FillStructOpcode(2, "XOR (IX+%n)");
   liste_opcodes_dd_[0xB4] = FillStructOpcode(2, "OR A, IXh");
   liste_opcodes_dd_[0xB5] = FillStructOpcode(2, "OR A, IXl");
   liste_opcodes_dd_[0xB6] = FillStructOpcode(2, "OR (IX+%n)");
   liste_opcodes_dd_[0xBC] = FillStructOpcode(2, "CP IXh");
   liste_opcodes_dd_[0xBD] = FillStructOpcode(2, "CP (IXl");
   liste_opcodes_dd_[0xBE] = FillStructOpcode(2, "CP (IX+%n)");
   liste_opcodes_dd_[0xCB] = FillStructOpcode(4, "RES b, (IX+%n)");
   liste_opcodes_dd_[0xE1] = FillStructOpcode(1, "POP IX");
   liste_opcodes_dd_[0xE3] = FillStructOpcode(1, "EX SP, IX");
   liste_opcodes_dd_[0xE5] = FillStructOpcode(1, "PUSH IX");
   liste_opcodes_dd_[0xE9] = FillStructOpcode(1, "JP (IX)");
   liste_opcodes_dd_[0xF9] = FillStructOpcode(1, "LD SP, IX");

   //////////////////
   // FD
   // Act like normal opcode, except that HL is replaced by IY.
   for (unsigned j = 0x00; j <= 0xff; j++)
   {
      liste_opcodes_fd_[j] = liste_opcodes_[j];
   }
   liste_opcodes_fd_[0x09] = FillStructOpcode(1, "ADD IY, BC");
   liste_opcodes_fd_[0x19] = FillStructOpcode(1, "ADD IY, DE");
   liste_opcodes_fd_[0x21] = FillStructOpcode(3, "LD IY, %nn__");
   liste_opcodes_fd_[0x22] = FillStructOpcode(3, "LD (%nn__), IY");
   liste_opcodes_fd_[0x23] = FillStructOpcode(1, "INC IY");
   liste_opcodes_fd_[0x24] = FillStructOpcode(1, "INC IYh");
   liste_opcodes_fd_[0x25] = FillStructOpcode(1, "DEC IYh");
   liste_opcodes_fd_[0x26] = FillStructOpcode(2, "LD IYh, %n");
   liste_opcodes_fd_[0x29] = FillStructOpcode(1, "ADD IY, IY");
   liste_opcodes_fd_[0x2A] = FillStructOpcode(3, "LD IY, (%nn__)");
   liste_opcodes_fd_[0x2B] = FillStructOpcode(1, "DEC IY");
   liste_opcodes_fd_[0x2C] = FillStructOpcode(1, "INC IYl");
   liste_opcodes_fd_[0x2D] = FillStructOpcode(1, "DEC IYl");
   liste_opcodes_fd_[0x2E] = FillStructOpcode(2, "LD IXl, %n");
   liste_opcodes_fd_[0x34] = FillStructOpcode(2, "INC (IY+%n)");
   liste_opcodes_fd_[0x35] = FillStructOpcode(2, "DEC (IY+%n)");
   liste_opcodes_fd_[0x36] = FillStructOpcode(3, "LD (IY+%n), %n");
   liste_opcodes_fd_[0x39] = FillStructOpcode(1, "ADD IY, SP");
   liste_opcodes_fd_[0x44] = FillStructOpcode(2, "LD B, IYh");
   liste_opcodes_fd_[0x45] = FillStructOpcode(2, "LD B, IYl");
   liste_opcodes_fd_[0x46] = FillStructOpcode(2, "LD B, (IY+%n)");
   liste_opcodes_fd_[0x4C] = FillStructOpcode(2, "LD C, IYh");
   liste_opcodes_fd_[0x4D] = FillStructOpcode(2, "LD C, IYl");
   liste_opcodes_fd_[0x4E] = FillStructOpcode(2, "LD C, (IY+%n)");
   liste_opcodes_fd_[0x54] = FillStructOpcode(2, "LD D, IYh");
   liste_opcodes_fd_[0x55] = FillStructOpcode(2, "LD D, IYl");
   liste_opcodes_fd_[0x56] = FillStructOpcode(2, "LD D, (IY+%n)");
   liste_opcodes_fd_[0x5C] = FillStructOpcode(2, "LD E, IYh");
   liste_opcodes_fd_[0x5D] = FillStructOpcode(2, "LD E, IYl");
   liste_opcodes_fd_[0x5E] = FillStructOpcode(2, "LD E, (IY+%n)");
   liste_opcodes_fd_[0x60] = FillStructOpcode(2, "LD IYh, B");
   liste_opcodes_fd_[0x61] = FillStructOpcode(2, "LD IYh, C");
   liste_opcodes_fd_[0x62] = FillStructOpcode(2, "LD IYh, D");
   liste_opcodes_fd_[0x63] = FillStructOpcode(2, "LD IYh, E");
   liste_opcodes_fd_[0x64] = FillStructOpcode(2, "LD IYh, IYh");
   liste_opcodes_fd_[0x65] = FillStructOpcode(2, "LD IYh, IYl");
   liste_opcodes_fd_[0x66] = FillStructOpcode(2, "LD H, (IY+%n)");
   liste_opcodes_fd_[0x67] = FillStructOpcode(2, "LD IYh, A");
   liste_opcodes_fd_[0x68] = FillStructOpcode(2, "LD IYh, B");
   liste_opcodes_fd_[0x69] = FillStructOpcode(2, "LD IYh, C");
   liste_opcodes_fd_[0x6A] = FillStructOpcode(2, "LD IYh, D");
   liste_opcodes_fd_[0x6B] = FillStructOpcode(2, "LD IYh, E");
   liste_opcodes_fd_[0x6C] = FillStructOpcode(2, "LD IYh, IYh");
   liste_opcodes_fd_[0x6D] = FillStructOpcode(2, "LD IYh, IYl");
   liste_opcodes_fd_[0x6E] = FillStructOpcode(2, "LD L, (IY+%n)");
   liste_opcodes_fd_[0x6F] = FillStructOpcode(2, "LD IYl, A");
   liste_opcodes_fd_[0x70] = FillStructOpcode(2, "LD (IY+%n), B");
   liste_opcodes_fd_[0x71] = FillStructOpcode(2, "LD (IY+%n), C");
   liste_opcodes_fd_[0x72] = FillStructOpcode(2, "LD (IY+%n), D");
   liste_opcodes_fd_[0x73] = FillStructOpcode(2, "LD (IY+%n), E");
   liste_opcodes_fd_[0x74] = FillStructOpcode(2, "LD (IY+%n), H");
   liste_opcodes_fd_[0x75] = FillStructOpcode(2, "LD (IY+%n), L");
   liste_opcodes_fd_[0x77] = FillStructOpcode(2, "LD (IY+%n), A");
   liste_opcodes_fd_[0x7C] = FillStructOpcode(2, "LD A, IYh");
   liste_opcodes_fd_[0x7D] = FillStructOpcode(1, "LD A, IYl");
   liste_opcodes_fd_[0x7E] = FillStructOpcode(2, "LD A, (IY+%n)");
   liste_opcodes_fd_[0x84] = FillStructOpcode(2, "ADD A, IYh");
   liste_opcodes_fd_[0x85] = FillStructOpcode(2, "ADD A, IYl");
   liste_opcodes_fd_[0x86] = FillStructOpcode(2, "ADD A, (IY+%n)");
   liste_opcodes_fd_[0x8C] = FillStructOpcode(2, "ADC A, IYh");
   liste_opcodes_fd_[0x8D] = FillStructOpcode(2, "ADC A, IYl");
   liste_opcodes_fd_[0x8E] = FillStructOpcode(2, "ADC A, (IY+%n)");
   liste_opcodes_fd_[0x94] = FillStructOpcode(2, "SUB A, IYh");
   liste_opcodes_fd_[0x95] = FillStructOpcode(2, "SUB A, IYl");
   liste_opcodes_fd_[0x96] = FillStructOpcode(2, "SUB A, (IY+%n)");
   liste_opcodes_fd_[0x9C] = FillStructOpcode(2, "SBC A, IYh");
   liste_opcodes_fd_[0x9D] = FillStructOpcode(2, "SBC A, IYl");
   liste_opcodes_fd_[0x9E] = FillStructOpcode(2, "SBC A, (IY+%n)");
   liste_opcodes_fd_[0xA4] = FillStructOpcode(2, "AND A, IYh");
   liste_opcodes_fd_[0xA5] = FillStructOpcode(2, "AND A, IYl");
   liste_opcodes_fd_[0xA6] = FillStructOpcode(2, "AND (IY+%n)");
   liste_opcodes_fd_[0xAC] = FillStructOpcode(2, "XOR A, IYh");
   liste_opcodes_fd_[0xAD] = FillStructOpcode(2, "XOR A, IYl");
   liste_opcodes_fd_[0xAE] = FillStructOpcode(2, "XOR (IY+%n)");
   liste_opcodes_fd_[0xB4] = FillStructOpcode(2, "OR A, IYh");
   liste_opcodes_fd_[0xB5] = FillStructOpcode(2, "OR A, IYl");
   liste_opcodes_fd_[0xB6] = FillStructOpcode(2, "OR (IY+%n)");
   liste_opcodes_fd_[0xBC] = FillStructOpcode(2, "CP IYh");
   liste_opcodes_fd_[0xBD] = FillStructOpcode(2, "CP (IYl");
   liste_opcodes_fd_[0xBE] = FillStructOpcode(2, "CP (IY+%n)");
   liste_opcodes_fd_[0xCB] = FillStructOpcode(3, "RES %b, (IY+%n)");
   liste_opcodes_fd_[0xE1] = FillStructOpcode(1, "POP_IY");
   liste_opcodes_fd_[0xE3] = FillStructOpcode(1, "EX (SP), IY");
   liste_opcodes_fd_[0xE5] = FillStructOpcode(1, "PUSH_IY");
   liste_opcodes_fd_[0xE9] = FillStructOpcode(1, "JP (IY)");
   liste_opcodes_fd_[0xF9] = FillStructOpcode(1, "LD SP, IY");
}
Z80Disassembler::~Z80Disassembler()
{
   
}


unsigned char Z80Disassembler::GetOpcodeSize(unsigned char* buffer)
{
   unsigned char nextInstr_L = *buffer;
   unsigned char size = liste_opcodes_[nextInstr_L].size;
   if (liste_opcodes_[nextInstr_L].disassembly == "%CB")
   {
      size += liste_opcodes_cb_[nextInstr_L].size;
   }
   else if (liste_opcodes_[nextInstr_L].disassembly == "%ED")
   {
      // ED ?
      size += liste_opcodes_ed_[nextInstr_L].size;
   }
   else if (liste_opcodes_[nextInstr_L].disassembly == "%DD")
   {
      // DD
      size += liste_opcodes_dd_[nextInstr_L].size;
   }
   else if (liste_opcodes_[nextInstr_L].disassembly == "%FD")
   {
      // FD
      size += liste_opcodes_fd_[nextInstr_L].size;
   }
   return size;
}

const int Z80Disassembler::DasmMnemonic(unsigned short Addr, unsigned char* buffer, char pMnemonic[16], char pArgument[16]) const
{
   // Disassemble the memory from Addr, to Addr+size
   unsigned short currentAddr = 0;

   memset(pMnemonic, 0, 16);
   memset(pArgument, 0, 16);
   
   unsigned char nextInstr_L = buffer[currentAddr];
   unsigned char size = liste_opcodes_[nextInstr_L].size;

   // disassembly
   std::string Opcode_L;
   if (strcmp(liste_opcodes_[nextInstr_L].disassembly.c_str(), "%CB") == 0)
   {
      // CB ?
      nextInstr_L = buffer[currentAddr + 1];
      Opcode_L = liste_opcodes_cb_[nextInstr_L].disassembly;
      size += liste_opcodes_cb_[nextInstr_L].size;
   }
   else if (strcmp(liste_opcodes_[nextInstr_L].disassembly.c_str(), "%ED") == 0)
   {
      // ED ?
      nextInstr_L = buffer[currentAddr + 1];
      Opcode_L = liste_opcodes_ed_[nextInstr_L].disassembly;
      size += liste_opcodes_ed_[nextInstr_L].size;
   }
   else if (strcmp(liste_opcodes_[nextInstr_L].disassembly.c_str(), "%DD") == 0)
   {
      // DD
      nextInstr_L = buffer[currentAddr + 1];
      Opcode_L = liste_opcodes_dd_[nextInstr_L].disassembly;
      size += liste_opcodes_dd_[nextInstr_L].size;
   }
   else if (strcmp(liste_opcodes_[nextInstr_L].disassembly.c_str(), "%FD") == 0)
   {
      // FD
      nextInstr_L = buffer[currentAddr + 1];
      Opcode_L = liste_opcodes_fd_[nextInstr_L].disassembly;
      size += liste_opcodes_fd_[nextInstr_L].size;
   }
   else
   {
      Opcode_L = liste_opcodes_[nextInstr_L].disassembly;
   }

   // First word of disassembly is opcode
   size_t pEndOfMnemonic = Opcode_L.find(' ');

   if (pEndOfMnemonic != std::string::npos)
   {

      std::string mnemonic = Opcode_L.substr(0, pEndOfMnemonic);
      memcpy(pMnemonic, mnemonic.c_str(), mnemonic.size() * sizeof(char));

      char* pAddr = pArgument;
      strcpy(pAddr, Opcode_L.substr(pEndOfMnemonic).c_str());

      //  Next
      char* pReplace_L = strchr(pAddr, '%');
      while (pReplace_L != NULL)
      {
         // Replace %nn__ by adress
         if (pReplace_L[1] == 'n' && pReplace_L[2] == 'n')
         {
            char minibuf[6];
            std::snprintf(minibuf, 6, "$%2.2X%2.2X", buffer[currentAddr + size - 1], buffer[currentAddr + size - 2]);
            // 2 then 1
            memcpy(pReplace_L, minibuf, 5 * sizeof(char));
         }
         // Replace %n2 by value
         else if (pReplace_L[1] == 'n' && pReplace_L[2] == '2')
         {
            char minibuf[4];
            std::snprintf(minibuf, 4, "%2.2X ", buffer[currentAddr + size - 2]);
            // 2 then 1
            memcpy(pReplace_L, minibuf, 3 * sizeof(char));
         }
         // Replace  %n" by value
         else if (pReplace_L[1] == 'n')
         {
            char minibuf[3];
            std::snprintf(minibuf, 3, "%2.2X", buffer[currentAddr + size - 1]);
            // 2 then 1
            memcpy(pReplace_L, minibuf, 2 * sizeof(char));
         }
         // Replace %j__ by relative jump
         else if (pReplace_L[1] == 'j')
         {
            char minibuf[5];
            char dec_L = buffer[currentAddr + size - 1];
            unsigned short relative_adress = Addr + dec_L + size;
            std::snprintf(minibuf, 5, "%4.4X", relative_adress);
            // 2 then 1
            memcpy(pReplace_L, minibuf, 4 * sizeof(char));

         }
         // Replace %r by register
         else if (pReplace_L[1] == 'r')
         {
            // get previous value
            int indexReg = currentAddr + size - 1;
            unsigned char reg = buffer[indexReg];
            // mask the 3 first bits
            switch (reg & 0x7)
            {
               // convert to register
            case 00: memcpy(pReplace_L, "B ", 2 * sizeof(char)); break;
            case 01: memcpy(pReplace_L, "C ", 2 * sizeof(char)); break;
            case 02: memcpy(pReplace_L, "D ", 2 * sizeof(char)); break;
            case 03: memcpy(pReplace_L, "E ", 2 * sizeof(char)); break;
            case 04: memcpy(pReplace_L, "H ", 2 * sizeof(char)); break;
            case 05: memcpy(pReplace_L, "L ", 2 * sizeof(char)); break;
            case 06: memcpy(pReplace_L, "HL", 2 * sizeof(char)); break;
            case 07: memcpy(pReplace_L, "A ", 2 * sizeof(char)); break;
            }

         }
         // Replace %r by bit
         else if (pReplace_L[1] == 'b')
         {
            // get previous value
            int indexReg = currentAddr + size - 1;
            unsigned char bit = buffer[indexReg];
            bit = (bit >> 3) & 0x7;
            ((char*)pReplace_L)[0] = '0' + bit;
            ((char*)pReplace_L)[1] = ' ';
         }
         pReplace_L = strchr(pAddr, '%');
      }
   }
   else
   {
      strncpy(pMnemonic, Opcode_L.c_str(), 15);
   }
   //pAddr = _tcscat(pAddr, "\n ");

   return size;
}
