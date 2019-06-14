#include "stdafx.h"

#include "Z80_Full.h"
#include "Sig.h"
#include "Memoire.h"


void Z80::InitOpcodeShortcuts()
{
   // Par defaut, tout le monde pointe sur NOP
   for (unsigned int i = 0; i < 256; i++)
   {
      FillStructOpcode<None>(i, &Z80::DefaultFetch, 1, "UNKNOWN");
      FillStructOpcode<CB>(i, &Z80::DefaultFetch, 1, "UNKNOWN");
      FillStructOpcode<ED>(i, &Z80::DefaultFetch, 1, "UNKNOWN");
      FillStructOpcode<DD>(i, &Z80::DefaultFetch, 1, "UNKNOWN");
      FillStructOpcode<FD>(i, &Z80::DefaultFetch, 1, "UNKNOWN");
   }

   current_function_ = &fetch_func;

   // Opcodes standards
   ///////////////////////////////////////////////////////////////////////////////////////
   /////////////                           FUNCTION       SIZE   DISASSMBLY
   FillStructOpcode<None>(0x00, &Z80::Opcode_NOP, 1, "NOP");
   FillStructOpcode<None>(0x01, &Z80::Opcode_Memory_Read_PC, 3, "LD BC, %nn__");
   FillStructOpcode<None>(0x02, &Z80::Opcode_Memory_Write_Addr_Reg<ADDR_BC, R_A>, 1, "LD (BC), A");
   FillStructOpcode<None>(0x03, &Z80::DefaultFetch, 1, "INC BC");
   FillStructOpcode<None>(0x04, &Z80::Opcode_Inc_Reg<R_B, false>, 1, "INC B");
   FillStructOpcode<None>(0x05, &Z80::DefaultFetch, 1, "DEC B");
   FillStructOpcode<None>(0x06, &Z80::Opcode_Memory_Read_PC, 2, "LD B, %n");
   FillStructOpcode<None>(0x07, &Z80::DefaultFetch, 1, "RLCA");
   FillStructOpcode<None>(0x08, &Z80::DefaultFetch, 1, "EX AF AF'");
   FillStructOpcode<None>(0x09, &Z80::DefaultFetch, 1, "ADD HL, BC");
   FillStructOpcode<None>(0x0A, &Z80::DefaultFetch, 1, "LD A, (BC)");
   FillStructOpcode<None>(0x0B, &Z80::DefaultFetch, 1, "DEC BC");
   FillStructOpcode<None>(0x0C, &Z80::Opcode_Inc_Reg<R_C, false>, 1, "INC C");
   FillStructOpcode<None>(0x0D, &Z80::DefaultFetch, 1, "DEC C");
   FillStructOpcode<None>(0x0E, &Z80::Opcode_Memory_Read_PC, 2, "LD C,  %n");
   FillStructOpcode<None>(0x0F, &Z80::DefaultFetch, 1, "RRCA");
   FillStructOpcode<None>(0x10, &Z80::DefaultFetch, 2, "DJNZ %j__");
   FillStructOpcode<None>(0x11, &Z80::Opcode_Memory_Read_PC, 3, "LD DE, %nn__");
   FillStructOpcode<None>(0x12, &Z80::DefaultFetch, 1, "LD (DE), A");
   FillStructOpcode<None>(0x13, &Z80::DefaultFetch, 1, "INC DE");
   FillStructOpcode<None>(0x14, &Z80::Opcode_Inc_Reg<R_D, false>, 1, "INC D");
   FillStructOpcode<None>(0x15, &Z80::DefaultFetch, 1, "DEC D");
   FillStructOpcode<None>(0x16, &Z80::Opcode_Memory_Read_PC, 2, "LD D,  %n");
   FillStructOpcode<None>(0x17, &Z80::DefaultFetch, 1, "RLA");
   FillStructOpcode<None>(0x18, &Z80::Opcode_Memory_Read_PC, 2, "JR %j__");
   FillStructOpcode<None>(0x19, &Z80::DefaultFetch, 1, "ADD HL, DE");
   FillStructOpcode<None>(0x1A, &Z80::DefaultFetch, 1, "LD A, (DE)");
   FillStructOpcode<None>(0x1B, &Z80::DefaultFetch, 1, "DEC DE");
   FillStructOpcode<None>(0x1C, &Z80::Opcode_Inc_Reg<R_E, false>, 1, "INC E");
   FillStructOpcode<None>(0x1D, &Z80::DefaultFetch, 1, "DEC E");
   FillStructOpcode<None>(0x1E, &Z80::Opcode_Memory_Read_PC, 2, "LD E,  %n");
   FillStructOpcode<None>(0x1F, &Z80::DefaultFetch, 1, "RRA");
   FillStructOpcode<None>(0x20, &Z80::Opcode_Memory_Read_PC, 2, "JR NZ  %j__");
   FillStructOpcode<None>(0x21, &Z80::Opcode_Memory_Read_PC, 3, "LD HL, %nn__");
   FillStructOpcode<None>(0x22, &Z80::Opcode_Memory_Read_PC, 3, "LD (%nn__), HL");
   FillStructOpcode<None>(0x23, &Z80::DefaultFetch, 1, "INC HL");
   FillStructOpcode<None>(0x24, &Z80::Opcode_Inc_Reg<R_H, false>, 1, "INC H");
   FillStructOpcode<None>(0x25, &Z80::DefaultFetch, 1, "DEC H");
   FillStructOpcode<None>(0x26, &Z80::Opcode_Memory_Read_PC, 2, "LD H,  %n");
   FillStructOpcode<None>(0x27, &Z80::DefaultFetch, 1, "DAA");
   FillStructOpcode<None>(0x28, &Z80::Opcode_Memory_Read_PC, 2, "JR Z  %j__");
   FillStructOpcode<None>(0x29, &Z80::DefaultFetch, 1, "ADD HL, HL");
   FillStructOpcode<None>(0x2A, &Z80::Opcode_Memory_Read_PC, 3, "LD HL, (%nn__)");
   FillStructOpcode<None>(0x2B, &Z80::DefaultFetch, 1, "DEC HL");
   FillStructOpcode<None>(0x2C, &Z80::Opcode_Inc_Reg<R_L, false>, 1, "INC L");
   FillStructOpcode<None>(0x2D, &Z80::DefaultFetch, 1, "DEC L");
   FillStructOpcode<None>(0x2E, &Z80::Opcode_Memory_Read_PC, 2, "LD L,  %n");
   FillStructOpcode<None>(0x2F, &Z80::DefaultFetch, 1, "CPL");
   FillStructOpcode<None>(0x30, &Z80::Opcode_Memory_Read_PC, 2, "JR NC  %j__");
   FillStructOpcode<None>(0x31, &Z80::Opcode_Memory_Read_PC, 3, "LD SP, %nn__");
   FillStructOpcode<None>(0x32, &Z80::Opcode_Memory_Read_PC, 3, "LD (%nn__), A");
   FillStructOpcode<None>(0x33, &Z80::DefaultFetch, 1, "INC SP");
   FillStructOpcode<None>(0x34, &Z80::DefaultFetch, 1, "INC (HL)");
   FillStructOpcode<None>(0x35, &Z80::DefaultFetch, 1, "DEC (HL)");
   FillStructOpcode<None>(0x36, &Z80::Opcode_Memory_Read_PC, 2, "LD (HL), %n");
   FillStructOpcode<None>(0x37, &Z80::DefaultFetch, 1, "SCF");
   FillStructOpcode<None>(0x38, &Z80::Opcode_Memory_Read_PC, 2, "JR C, %j__");
   FillStructOpcode<None>(0x39, &Z80::DefaultFetch, 1, "ADD HL, SP");
   FillStructOpcode<None>(0x3A, &Z80::Opcode_Memory_Read_PC, 3, "LD A, (%nn__)");
   FillStructOpcode<None>(0x3B, &Z80::DefaultFetch, 1, "DEC SP");
   FillStructOpcode<None>(0x3C, &Z80::Opcode_Inc_Reg<R_A, false>, 1, "INC A");
   FillStructOpcode<None>(0x3D, &Z80::DefaultFetch, 1, "DEC A");
   FillStructOpcode<None>(0x3E, &Z80::Opcode_Memory_Read_PC, 2, "LD A, %n");
   FillStructOpcode<None>(0x3F, &Z80::DefaultFetch, 1, "CCF");
   FillStructOpcode<None>(0x40, &Z80::DefaultFetch, 1, "LD B, B");
   FillStructOpcode<None>(0x41, &Z80::DefaultFetch, 1, "LD B, C");
   FillStructOpcode<None>(0x42, &Z80::DefaultFetch, 1, "LD B, D");
   FillStructOpcode<None>(0x43, &Z80::DefaultFetch, 1, "LD B, E");
   FillStructOpcode<None>(0x44, &Z80::DefaultFetch, 1, "LD B, H");
   FillStructOpcode<None>(0x45, &Z80::DefaultFetch, 1, "LD B, L");
   FillStructOpcode<None>(0x46, &Z80::DefaultFetch, 1, "LD B, (HL)");
   FillStructOpcode<None>(0x47, &Z80::DefaultFetch, 1, "LD B, A");
   FillStructOpcode<None>(0x48, &Z80::DefaultFetch, 1, "LD C, B");
   FillStructOpcode<None>(0x49, &Z80::DefaultFetch, 1, "LD C, C");
   FillStructOpcode<None>(0x4A, &Z80::DefaultFetch, 1, "LD C, D");
   FillStructOpcode<None>(0x4B, &Z80::DefaultFetch, 1, "LD C, E");
   FillStructOpcode<None>(0x4C, &Z80::DefaultFetch, 1, "LD C, H");
   FillStructOpcode<None>(0x4D, &Z80::DefaultFetch, 1, "LD C, L");
   FillStructOpcode<None>(0x4E, &Z80::DefaultFetch, 1, "LD C, (HL)");
   FillStructOpcode<None>(0x4F, &Z80::DefaultFetch, 1, "LD C, A");
   FillStructOpcode<None>(0x50, &Z80::DefaultFetch, 1, "LD D, B");
   FillStructOpcode<None>(0x51, &Z80::DefaultFetch, 1, "LD D, C");
   FillStructOpcode<None>(0x52, &Z80::DefaultFetch, 1, "LD D, D");
   FillStructOpcode<None>(0x53, &Z80::DefaultFetch, 1, "LD D, E");
   FillStructOpcode<None>(0x54, &Z80::DefaultFetch, 1, "LD D, H");
   FillStructOpcode<None>(0x55, &Z80::DefaultFetch, 1, "LD D, L");
   FillStructOpcode<None>(0x56, &Z80::DefaultFetch, 1, "LD D, (HL)");
   FillStructOpcode<None>(0x57, &Z80::DefaultFetch, 1, "LD D, A");
   FillStructOpcode<None>(0x58, &Z80::DefaultFetch, 1, "LD E, B");
   FillStructOpcode<None>(0x59, &Z80::DefaultFetch, 1, "LD E, C");
   FillStructOpcode<None>(0x5A, &Z80::DefaultFetch, 1, "LD E, D");
   FillStructOpcode<None>(0x5B, &Z80::DefaultFetch, 1, "LD E, E");
   FillStructOpcode<None>(0x5C, &Z80::DefaultFetch, 1, "LD E, H");
   FillStructOpcode<None>(0x5D, &Z80::DefaultFetch, 1, "LD E, L");
   FillStructOpcode<None>(0x5E, &Z80::DefaultFetch, 1, "LD E, (HL)");
   FillStructOpcode<None>(0x5F, &Z80::DefaultFetch, 1, "LD E, A");
   FillStructOpcode<None>(0x60, &Z80::DefaultFetch, 1, "LD H, B");
   FillStructOpcode<None>(0x61, &Z80::DefaultFetch, 1, "LD H, C");
   FillStructOpcode<None>(0x62, &Z80::DefaultFetch, 1, "LD H, D");
   FillStructOpcode<None>(0x63, &Z80::DefaultFetch, 1, "LD H, E");
   FillStructOpcode<None>(0x64, &Z80::DefaultFetch, 1, "LD H, H");
   FillStructOpcode<None>(0x65, &Z80::DefaultFetch, 1, "LD H, L");
   FillStructOpcode<None>(0x66, &Z80::DefaultFetch, 1, "LD H, (HL)");
   FillStructOpcode<None>(0x67, &Z80::DefaultFetch, 1, "LD H, A");
   FillStructOpcode<None>(0x68, &Z80::DefaultFetch, 1, "LD L, B");
   FillStructOpcode<None>(0x69, &Z80::DefaultFetch, 1, "LD L, C");
   FillStructOpcode<None>(0x6A, &Z80::DefaultFetch, 1, "LD L, D");
   FillStructOpcode<None>(0x6B, &Z80::DefaultFetch, 1, "LD L, E");
   FillStructOpcode<None>(0x6C, &Z80::DefaultFetch, 1, "LD L, H");
   FillStructOpcode<None>(0x6D, &Z80::DefaultFetch, 1, "LD L, L");
   FillStructOpcode<None>(0x6E, &Z80::DefaultFetch, 1, "LD L, (HL)");
   FillStructOpcode<None>(0x6F, &Z80::DefaultFetch, 1, "LD L, A");
   FillStructOpcode<None>(0x70, &Z80::DefaultFetch, 1, "LD (HL), B");
   FillStructOpcode<None>(0x71, &Z80::DefaultFetch, 1, "LD (HL), C");
   FillStructOpcode<None>(0x72, &Z80::DefaultFetch, 1, "LD (HL), D");
   FillStructOpcode<None>(0x73, &Z80::DefaultFetch, 1, "LD (HL), E");
   FillStructOpcode<None>(0x74, &Z80::DefaultFetch, 1, "LD (HL), H");
   FillStructOpcode<None>(0x75, &Z80::DefaultFetch, 1, "LD (HL), L");
   FillStructOpcode<None>(0x76, &Z80::DefaultFetch, 1, "HALT");
   FillStructOpcode<None>(0x77, &Z80::DefaultFetch, 1, "LD (HL), A");
   FillStructOpcode<None>(0x78, &Z80::DefaultFetch, 1, "LD A, B");
   FillStructOpcode<None>(0x79, &Z80::DefaultFetch, 1, "LD A, C");
   FillStructOpcode<None>(0x7A, &Z80::DefaultFetch, 1, "LD A, D");
   FillStructOpcode<None>(0x7B, &Z80::DefaultFetch, 1, "LD A, E");
   FillStructOpcode<None>(0x7C, &Z80::DefaultFetch, 1, "LD A, H");
   FillStructOpcode<None>(0x7D, &Z80::DefaultFetch, 1, "LD A, L");
   FillStructOpcode<None>(0x7E, &Z80::DefaultFetch, 1, "LD A, (HL)");
   FillStructOpcode<None>(0x7F, &Z80::DefaultFetch, 1, "LD A, A");
   FillStructOpcode<None>(0x80, &Z80::DefaultFetch, 1, "ADD A, B");
   FillStructOpcode<None>(0x81, &Z80::DefaultFetch, 1, "ADD A, C");
   FillStructOpcode<None>(0x82, &Z80::DefaultFetch, 1, "ADD A, D");
   FillStructOpcode<None>(0x83, &Z80::DefaultFetch, 1, "ADD A, E");
   FillStructOpcode<None>(0x84, &Z80::DefaultFetch, 1, "ADD A, H");
   FillStructOpcode<None>(0x85, &Z80::DefaultFetch, 1, "ADD A, L");
   FillStructOpcode<None>(0x86, &Z80::DefaultFetch, 1, "ADD A, (HL)");
   FillStructOpcode<None>(0x87, &Z80::DefaultFetch, 1, "ADD A, A");
   FillStructOpcode<None>(0x88, &Z80::DefaultFetch, 1, "ADC A, B");
   FillStructOpcode<None>(0x89, &Z80::DefaultFetch, 1, "ADC A, C");
   FillStructOpcode<None>(0x8A, &Z80::DefaultFetch, 1, "ADC A, D");
   FillStructOpcode<None>(0x8B, &Z80::DefaultFetch, 1, "ADC A, E");
   FillStructOpcode<None>(0x8C, &Z80::DefaultFetch, 1, "ADC A, H");
   FillStructOpcode<None>(0x8D, &Z80::DefaultFetch, 1, "ADC A, L");
   FillStructOpcode<None>(0x8E, &Z80::DefaultFetch, 1, "ADC A, (HL)");
   FillStructOpcode<None>(0x8F, &Z80::DefaultFetch, 1, "ADC A, A");
   FillStructOpcode<None>(0x90, &Z80::DefaultFetch, 1, "SUB A, B");
   FillStructOpcode<None>(0x91, &Z80::DefaultFetch, 1, "SUB A, C");
   FillStructOpcode<None>(0x92, &Z80::DefaultFetch, 1, "SUB A, D");
   FillStructOpcode<None>(0x93, &Z80::DefaultFetch, 1, "SUB A, E");
   FillStructOpcode<None>(0x94, &Z80::DefaultFetch, 1, "SUB A, H");
   FillStructOpcode<None>(0x95, &Z80::DefaultFetch, 1, "SUB A, L");
   FillStructOpcode<None>(0x96, &Z80::DefaultFetch, 1, "SUB A, (HL)");
   FillStructOpcode<None>(0x97, &Z80::DefaultFetch, 1, "SUB A, A");
   FillStructOpcode<None>(0x98, &Z80::DefaultFetch, 1, "SBC A, B");
   FillStructOpcode<None>(0x99, &Z80::DefaultFetch, 1, "SBC A, C");
   FillStructOpcode<None>(0x9A, &Z80::DefaultFetch, 1, "SBC A, D");
   FillStructOpcode<None>(0x9B, &Z80::DefaultFetch, 1, "SBC A, E");
   FillStructOpcode<None>(0x9C, &Z80::DefaultFetch, 1, "SBC A, H");
   FillStructOpcode<None>(0x9D, &Z80::DefaultFetch, 1, "SBC A, L");
   FillStructOpcode<None>(0x9E, &Z80::DefaultFetch, 1, "SBC A, (HL)");
   FillStructOpcode<None>(0x9F, &Z80::DefaultFetch, 1, "SBC A, A");
   FillStructOpcode<None>(0xA0, &Z80::DefaultFetch, 1, "AND B");
   FillStructOpcode<None>(0xA1, &Z80::DefaultFetch, 1, "AND C");
   FillStructOpcode<None>(0xA2, &Z80::DefaultFetch, 1, "AND D");
   FillStructOpcode<None>(0xA3, &Z80::DefaultFetch, 1, "AND E");
   FillStructOpcode<None>(0xA4, &Z80::DefaultFetch, 1, "AND H");
   FillStructOpcode<None>(0xA5, &Z80::DefaultFetch, 1, "AND L");
   FillStructOpcode<None>(0xA6, &Z80::DefaultFetch, 1, "AND (HL)");
   FillStructOpcode<None>(0xA7, &Z80::DefaultFetch, 1, "AND A");
   FillStructOpcode<None>(0xA8, &Z80::DefaultFetch, 1, "XOR B");
   FillStructOpcode<None>(0xA9, &Z80::DefaultFetch, 1, "XOR C");
   FillStructOpcode<None>(0xAA, &Z80::DefaultFetch, 1, "XOR D");
   FillStructOpcode<None>(0xAB, &Z80::DefaultFetch, 1, "XOR E");
   FillStructOpcode<None>(0xAC, &Z80::DefaultFetch, 1, "XOR H");
   FillStructOpcode<None>(0xAD, &Z80::DefaultFetch, 1, "XOR L");
   FillStructOpcode<None>(0xAE, &Z80::DefaultFetch, 1, "XOR (HL)");
   FillStructOpcode<None>(0xAF, &Z80::DefaultFetch, 1, "XOR A");
   FillStructOpcode<None>(0xB0, &Z80::DefaultFetch, 1, "OR B");
   FillStructOpcode<None>(0xB1, &Z80::DefaultFetch, 1, "OR C");
   FillStructOpcode<None>(0xB2, &Z80::DefaultFetch, 1, "OR D");
   FillStructOpcode<None>(0xB3, &Z80::DefaultFetch, 1, "OR E");
   FillStructOpcode<None>(0xB4, &Z80::DefaultFetch, 1, "OR H");
   FillStructOpcode<None>(0xB5, &Z80::DefaultFetch, 1, "OR L");
   FillStructOpcode<None>(0xB6, &Z80::DefaultFetch, 1, "OR (HL)");
   FillStructOpcode<None>(0xB7, &Z80::DefaultFetch, 1, "OR A");
   FillStructOpcode<None>(0xB8, &Z80::DefaultFetch, 1, "CP B");
   FillStructOpcode<None>(0xB9, &Z80::DefaultFetch, 1, "CP C");
   FillStructOpcode<None>(0xBA, &Z80::DefaultFetch, 1, "CP D");
   FillStructOpcode<None>(0xBB, &Z80::DefaultFetch, 1, "CP E");
   FillStructOpcode<None>(0xBC, &Z80::DefaultFetch, 1, "CP H");
   FillStructOpcode<None>(0xBD, &Z80::DefaultFetch, 1, "CP L");
   FillStructOpcode<None>(0xBE, &Z80::DefaultFetch, 1, "CP (HL)");
   FillStructOpcode<None>(0xBF, &Z80::DefaultFetch, 1, "CP A");
   FillStructOpcode<None>(0xC0, &Z80::DefaultFetch, 1, "RET NZ");
   FillStructOpcode<None>(0xC1, &Z80::DefaultFetch, 1, "POP BC");
   FillStructOpcode<None>(0xC2, &Z80::Opcode_Memory_Read_PC, 3, "JP NZ %nn__");
   FillStructOpcode<None>(0xC3, &Z80::Opcode_Memory_Read_PC, 3, "JP %nn__");
   FillStructOpcode<None>(0xC4, &Z80::Opcode_Memory_Read_PC, 3, "CALL NZ %nn__");
   FillStructOpcode<None>(0xC5, &Z80::DefaultFetch, 1, "PUSH BC");
   FillStructOpcode<None>(0xC6, &Z80::DefaultFetch, 2, "ADD A, %n");
   FillStructOpcode<None>(0xC7, &Z80::DefaultFetch, 1, "RST 0");
   FillStructOpcode<None>(0xC8, &Z80::DefaultFetch, 1, "RET Z");
   FillStructOpcode<None>(0xC9, &Z80::DefaultFetch, 1, "RET");
   FillStructOpcode<None>(0xCA, &Z80::Opcode_Memory_Read_PC, 3, "JP Z %nn__");
   FillStructOpcode<None>(0xCB, &Z80::Opcode_CB, 1, "%CB");
   FillStructOpcode<None>(0xCC, &Z80::Opcode_Memory_Read_PC, 3, "CALL Z %nn__");
   FillStructOpcode<None>(0xCD, &Z80::DefaultFetch, 3, "CALL %nn__");
   FillStructOpcode<None>(0xCE, &Z80::DefaultFetch, 2, "ADC A, %n");
   FillStructOpcode<None>(0xCF, &Z80::DefaultFetch, 1, "RST 08H");
   FillStructOpcode<None>(0xD0, &Z80::DefaultFetch, 1, "RET NC");
   FillStructOpcode<None>(0xD1, &Z80::DefaultFetch, 1, "POP DE");
   FillStructOpcode<None>(0xD2, &Z80::Opcode_Memory_Read_PC, 3, "JP NC %nn__");
   FillStructOpcode<None>(0xD3, &Z80::Opcode_Memory_Read_PC, 2, "OUT (%n), A");
   FillStructOpcode<None>(0xD4, &Z80::Opcode_Memory_Read_PC, 3, "CALL NC %nn__");
   FillStructOpcode<None>(0xD5, &Z80::DefaultFetch, 1, "PUSH DE");
   FillStructOpcode<None>(0xD6, &Z80::Opcode_Memory_Read_PC, 2, "SUB A, %n");
   FillStructOpcode<None>(0xD7, &Z80::DefaultFetch, 1, "RST 10H");
   FillStructOpcode<None>(0xD8, &Z80::DefaultFetch, 1, "RET C");
   FillStructOpcode<None>(0xD9, &Z80::DefaultFetch, 1, "EXX");
   FillStructOpcode<None>(0xDA, &Z80::Opcode_Memory_Read_PC, 3, "JP C %nn__");
   FillStructOpcode<None>(0xDB, &Z80::Opcode_Memory_Read_PC, 2, "IN A, (%n)");
   FillStructOpcode<None>(0xDC, &Z80::Opcode_Memory_Read_PC, 3, "CALL C %nn__");
   FillStructOpcode<None>(0xDD, &Z80::Opcode_DD, 1, "%DD");
   FillStructOpcode<None>(0xDE, &Z80::Opcode_Memory_Read_PC, 2, "SBC A, %n");
   FillStructOpcode<None>(0xDF, &Z80::DefaultFetch, 1, "RST 18H");
   FillStructOpcode<None>(0xE0, &Z80::DefaultFetch, 1, "RET PO");
   FillStructOpcode<None>(0xE1, &Z80::DefaultFetch, 1, "POP HL");
   FillStructOpcode<None>(0xE2, &Z80::Opcode_Memory_Read_PC, 3, "JP PO %nn__");
   FillStructOpcode<None>(0xE3, &Z80::DefaultFetch, 1, "EX (SP), HL");
   FillStructOpcode<None>(0xE4, &Z80::Opcode_Memory_Read_PC, 3, "CALL PO %nn__");
   FillStructOpcode<None>(0xE5, &Z80::DefaultFetch, 1, "PUSH HL");
   FillStructOpcode<None>(0xE6, &Z80::Opcode_Memory_Read_PC, 2, "AND %n");
   FillStructOpcode<None>(0xE7, &Z80::DefaultFetch, 1, "RST 20H");
   FillStructOpcode<None>(0xE8, &Z80::DefaultFetch, 1, "RET PE");
   FillStructOpcode<None>(0xE9, &Z80::DefaultFetch, 1, "JP (HL)");
   FillStructOpcode<None>(0xEA, &Z80::Opcode_Memory_Read_PC, 3, "JP PE %nn__");
   FillStructOpcode<None>(0xEB, &Z80::DefaultFetch, 1, "EX DE,HL");
   FillStructOpcode<None>(0xEC, &Z80::Opcode_Memory_Read_PC, 3, "CALL PE %nn__");
   FillStructOpcode<None>(0xED, &Z80::Opcode_ED, 1, "%ED");
   FillStructOpcode<None>(0xEE, &Z80::Opcode_Memory_Read_PC, 2, "XOR %n");
   FillStructOpcode<None>(0xEF, &Z80::DefaultFetch, 1, "RST 28H");
   FillStructOpcode<None>(0xF0, &Z80::DefaultFetch, 1, "RET P");
   FillStructOpcode<None>(0xF1, &Z80::DefaultFetch, 1, "POP AF");
   FillStructOpcode<None>(0xF2, &Z80::Opcode_Memory_Read_PC, 3, "JP P %nn__");
   FillStructOpcode<None>(0xF3, &Z80::DefaultFetch, 1, "DI");
   FillStructOpcode<None>(0xF4, &Z80::Opcode_Memory_Read_PC, 3, "CALL P %nn__");
   FillStructOpcode<None>(0xF5, &Z80::DefaultFetch, 1, "PUSH AF");
   FillStructOpcode<None>(0xF6, &Z80::DefaultFetch, 2, "OR %n");
   FillStructOpcode<None>(0xF7, &Z80::DefaultFetch, 1, "RST 30H");
   FillStructOpcode<None>(0xF8, &Z80::DefaultFetch, 1, "RET M");
   FillStructOpcode<None>(0xF9, &Z80::DefaultFetch, 1, "LD SP, HL");
   FillStructOpcode<None>(0xFA, &Z80::Opcode_Memory_Read_PC, 3, "JP M %nn__");
   FillStructOpcode<None>(0xFB, &Z80::DefaultFetch, 1, "EI");
   FillStructOpcode<None>(0xFC, &Z80::Opcode_Memory_Read_PC, 3, "CALL M %nn__");
   FillStructOpcode<None>(0xFD, &Z80::Opcode_FD, 1, "%FD");
   FillStructOpcode<None>(0xFE, &Z80::Opcode_Memory_Read_PC, 2, "CP %n");
   FillStructOpcode<None>(0xFF, &Z80::DefaultFetch, 1, "RST 38H");
#if 0
   // Opcode a multiple byte
   // CB
   unsigned int j;
   for (j = 0x00; j <= 0x07; j++) liste_opcodes_cb_[j] = FillStructOpcode(nullptr, 1, "RLC %r");
   for (j = 0x08; j <= 0x0F; j++) liste_opcodes_cb_[j] = FillStructOpcode(nullptr, 1, "RRC %r");
   for (j = 0x10; j <= 0x17; j++) liste_opcodes_cb_[j] = FillStructOpcode(nullptr, 1, "RL %r");
   for (j = 0x18; j <= 0x1F; j++) liste_opcodes_cb_[j] = FillStructOpcode(nullptr, 1, "RR %r");
   for (j = 0x20; j <= 0x27; j++) liste_opcodes_cb_[j] = FillStructOpcode(nullptr, 1, "SLA %r");
   for (j = 0x28; j <= 0x2F; j++) liste_opcodes_cb_[j] = FillStructOpcode(nullptr, 1, "SRA %r");
   for (j = 0x30; j <= 0x37; j++) liste_opcodes_cb_[j] = FillStructOpcode(nullptr, 1, "SLL %r");
   for (j = 0x38; j <= 0x3F; j++) liste_opcodes_cb_[j] = FillStructOpcode(nullptr, 1, "SRL %r");
   DEF_OP_BIT(A); DEF_OP_BIT(B); DEF_OP_BIT(C); DEF_OP_BIT(D); DEF_OP_BIT(E); DEF_OP_BIT(H); DEF_OP_BIT(L); DEF_OP_BIT(HL);

   for (j = 0x80; j <= 0xBF; j++) liste_opcodes_cb_[j] = FillStructOpcode(nullptr, 1, "RES %b,%r");
   for (j = 0xC0; j <= 0xFF; j++) liste_opcodes_cb_[j] = FillStructOpcode(nullptr, 1, "SET %b,%r");

   // ED
   for (j = 0x00; j <= 0xFF; j++) liste_opcodes_ed_[j] = FillStructOpcode(nullptr, 1, "Default ED");

   liste_opcodes_ed_[0x40] = FillStructOpcode(nullptr, 1, "IN B, (C)");
   liste_opcodes_ed_[0x41] = FillStructOpcode(nullptr, 1, "OUT (C), B");
   liste_opcodes_ed_[0x42] = FillStructOpcode(nullptr, 1, "SBL HL, BC");
   liste_opcodes_ed_[0x43] = FillStructOpcode(nullptr, 3, "LD (%nn__), BC");
   liste_opcodes_ed_[0x44] = FillStructOpcode(nullptr, 1, "NEG");
   liste_opcodes_ed_[0x45] = FillStructOpcode(nullptr, 1, "RETN");
   liste_opcodes_ed_[0x46] = FillStructOpcode(nullptr, 1, "IM 0");
   liste_opcodes_ed_[0x47] = FillStructOpcode(nullptr, 1, "LD I, A");
   liste_opcodes_ed_[0x48] = FillStructOpcode(nullptr, 1, "IN C, (C)");
   liste_opcodes_ed_[0x49] = FillStructOpcode(nullptr, 1, "OUT (C), C");
   liste_opcodes_ed_[0x4A] = FillStructOpcode(nullptr, 1, "ADC HL, BC");
   liste_opcodes_ed_[0x4B] = FillStructOpcode(nullptr, 3, "LD BC, (%nn__)");
   liste_opcodes_ed_[0x4C] = FillStructOpcode(nullptr, 1, "NEG");
   liste_opcodes_ed_[0x4D] = FillStructOpcode(nullptr, 1, "RETI");
   liste_opcodes_ed_[0x4E] = FillStructOpcode(nullptr, 1, "IM 0");
   liste_opcodes_ed_[0x4F] = FillStructOpcode(nullptr, 1, "LD R, A");
   liste_opcodes_ed_[0x50] = FillStructOpcode(nullptr, 1, "IN D, (C)");
   liste_opcodes_ed_[0x51] = FillStructOpcode(nullptr, 1, "OUT (C), D");
   liste_opcodes_ed_[0x52] = FillStructOpcode(nullptr, 1, "SBL HL, DE");
   liste_opcodes_ed_[0x53] = FillStructOpcode(nullptr, 3, "LD (%nn__), DE");
   liste_opcodes_ed_[0x54] = FillStructOpcode(nullptr, 1, "NEG");
   liste_opcodes_ed_[0x55] = FillStructOpcode(nullptr, 1, "RETN");
   liste_opcodes_ed_[0x56] = FillStructOpcode(nullptr, 1, "IM 1");
   liste_opcodes_ed_[0x57] = FillStructOpcode(nullptr, 1, "LD A, I");
   liste_opcodes_ed_[0x58] = FillStructOpcode(nullptr, 1, "IN E, (C)");
   liste_opcodes_ed_[0x59] = FillStructOpcode(nullptr, 1, "OUT (C), E");
   liste_opcodes_ed_[0x5A] = FillStructOpcode(nullptr, 1, "ADC HL, DE");
   liste_opcodes_ed_[0x5B] = FillStructOpcode(nullptr, 3, "LD DE, (%nn__)");
   liste_opcodes_ed_[0x5C] = FillStructOpcode(nullptr, 1, "NEG");
   liste_opcodes_ed_[0x5D] = FillStructOpcode(nullptr, 1, "RETN");
   liste_opcodes_ed_[0x5E] = FillStructOpcode(nullptr, 1, "IM2");
   liste_opcodes_ed_[0x5F] = FillStructOpcode(nullptr, 1, "LD A, R");
   liste_opcodes_ed_[0x60] = FillStructOpcode(nullptr, 1, "IN H, (C)");
   liste_opcodes_ed_[0x61] = FillStructOpcode(nullptr, 1, "OUT (C), H");
   liste_opcodes_ed_[0x62] = FillStructOpcode(nullptr, 1, "SBC HL, HL");
   liste_opcodes_ed_[0x63] = FillStructOpcode(nullptr, 3, "LN (%nn__), HL");
   liste_opcodes_ed_[0x64] = FillStructOpcode(nullptr, 1, "NEG");
   liste_opcodes_ed_[0x65] = FillStructOpcode(nullptr, 1, "RETN");
   liste_opcodes_ed_[0x66] = FillStructOpcode(nullptr, 1, "IM 0");
   liste_opcodes_ed_[0x67] = FillStructOpcode(nullptr, 1, "RRD");
   liste_opcodes_ed_[0x68] = FillStructOpcode(nullptr, 1, "IN L, (C)");
   liste_opcodes_ed_[0x69] = FillStructOpcode(nullptr, 1, "OUT (C), L");
   liste_opcodes_ed_[0x6A] = FillStructOpcode(nullptr, 1, "ADC HL, HL");
   liste_opcodes_ed_[0x6B] = FillStructOpcode(nullptr, 3, "LN HL, (%nn__)");
   liste_opcodes_ed_[0x6C] = FillStructOpcode(nullptr, 1, "NEG");
   liste_opcodes_ed_[0x6D] = FillStructOpcode(nullptr, 1, "RETN");
   liste_opcodes_ed_[0x6E] = FillStructOpcode(nullptr, 1, "IM 0");
   liste_opcodes_ed_[0x6F] = FillStructOpcode(nullptr, 1, "RLD");
   liste_opcodes_ed_[0x70] = FillStructOpcode(nullptr, 1, "IN (C)");
   liste_opcodes_ed_[0x71] = FillStructOpcode(nullptr, 1, "OUT (C), 0");
   liste_opcodes_ed_[0x72] = FillStructOpcode(nullptr, 1, "SBC HL, SP");
   liste_opcodes_ed_[0x73] = FillStructOpcode(nullptr, 3, "LD (%nn__), SP");
   liste_opcodes_ed_[0x74] = FillStructOpcode(nullptr, 1, "NEG");
   liste_opcodes_ed_[0x75] = FillStructOpcode(nullptr, 1, "RETN");
   liste_opcodes_ed_[0x76] = FillStructOpcode(nullptr, 1, "IM 1");
   liste_opcodes_ed_[0x77] = FillStructOpcode(nullptr, 1, "NOP");
   liste_opcodes_ed_[0x78] = FillStructOpcode(nullptr, 1, "IN A, (C)");
   liste_opcodes_ed_[0x79] = FillStructOpcode(nullptr, 1, "OUT (C), A");
   liste_opcodes_ed_[0x7A] = FillStructOpcode(nullptr, 1, "ADC HL, SP");
   liste_opcodes_ed_[0x7B] = FillStructOpcode(nullptr, 3, "LD SP, (%nn__)");
   liste_opcodes_ed_[0x7C] = FillStructOpcode(nullptr, 1, "NEG");
   liste_opcodes_ed_[0x7D] = FillStructOpcode(nullptr, 1, "RETN");
   liste_opcodes_ed_[0x7E] = FillStructOpcode(nullptr, 1, "IM 2");
   liste_opcodes_ed_[0x7F] = FillStructOpcode(nullptr, 1, "NOP");
   liste_opcodes_ed_[0xA0] = FillStructOpcode(nullptr, 1, "LDI");
   liste_opcodes_ed_[0xA1] = FillStructOpcode(nullptr, 1, "CPI");
   liste_opcodes_ed_[0xA2] = FillStructOpcode(nullptr, 1, "INI");
   liste_opcodes_ed_[0xA3] = FillStructOpcode(nullptr, 1, "OUTI");
   liste_opcodes_ed_[0xA8] = FillStructOpcode(nullptr, 1, "LDD");
   liste_opcodes_ed_[0xA9] = FillStructOpcode(nullptr, 1, "CPD");
   liste_opcodes_ed_[0xAA] = FillStructOpcode(nullptr, 1, "IND");
   liste_opcodes_ed_[0xAB] = FillStructOpcode(nullptr, 1, "OUTD");
   liste_opcodes_ed_[0xB0] = FillStructOpcode(nullptr, 1, "LDIR");
   liste_opcodes_ed_[0xB1] = FillStructOpcode(nullptr, 1, "CPIR");
   liste_opcodes_ed_[0xB2] = FillStructOpcode(nullptr, 1, "INIR");
   liste_opcodes_ed_[0xB3] = FillStructOpcode(nullptr, 1, "OTIR");
   liste_opcodes_ed_[0xB8] = FillStructOpcode(nullptr, 1, "LDDR");
   liste_opcodes_ed_[0xB9] = FillStructOpcode(nullptr, 1, "CPDR");
   liste_opcodes_ed_[0xBA] = FillStructOpcode(nullptr, 1, "INDR");
   liste_opcodes_ed_[0xBB] = FillStructOpcode(nullptr, 1, "OTDR");
   liste_opcodes_ed_[0xED] = FillStructOpcode(nullptr, 1, "%ED");

   // DD
   // Act like normal opcode, except that HL is replaced by IX.
   for (j = 0x00; j <= 0xff; j++)
   {
      liste_opcodes_dd_[j] = liste_opcodes_[j];
   }

   liste_opcodes_dd_[0x09] = FillStructOpcode(nullptr, 1, "ADD IX, BC");
   liste_opcodes_dd_[0x19] = FillStructOpcode(nullptr, 1, "ADD IX, DE");
   liste_opcodes_dd_[0x21] = FillStructOpcode(nullptr, 3, "LD IX, %nn__");
   liste_opcodes_dd_[0x22] = FillStructOpcode(nullptr, 3, "LD (%nn__), IX");
   liste_opcodes_dd_[0x23] = FillStructOpcode(nullptr, 1, "INC IX");
   liste_opcodes_dd_[0x24] = FillStructOpcode(nullptr, 1, "INC IXh");
   liste_opcodes_dd_[0x25] = FillStructOpcode(nullptr, 1, "DEC IXh");
   liste_opcodes_dd_[0x26] = FillStructOpcode(nullptr, 2, "LD IXh, %n");
   liste_opcodes_dd_[0x29] = FillStructOpcode(nullptr, 1, "ADD IX, IX");
   liste_opcodes_dd_[0x2A] = FillStructOpcode(nullptr, 3, "LD IX, (%nn__)");
   liste_opcodes_dd_[0x2B] = FillStructOpcode(nullptr, 1, "DEC IX");
   liste_opcodes_dd_[0x2C] = FillStructOpcode(nullptr, 1, "INC IXl");
   liste_opcodes_dd_[0x2D] = FillStructOpcode(nullptr, 1, "DEC IXl");
   liste_opcodes_dd_[0x2E] = FillStructOpcode(nullptr, 2, "LD IXl, %n");
   liste_opcodes_dd_[0x34] = FillStructOpcode(nullptr, 1, "INC (IX+%n)");
   liste_opcodes_dd_[0x35] = FillStructOpcode(nullptr, 1, "DEC (IX+%n)");
   liste_opcodes_dd_[0x36] = FillStructOpcode(nullptr, 3, "LD (IX+%n), %n");
   liste_opcodes_dd_[0x39] = FillStructOpcode(nullptr, 1, "ADD IX, SP");
   liste_opcodes_dd_[0x44] = FillStructOpcode(nullptr, 2, "LD B, IXh");
   liste_opcodes_dd_[0x45] = FillStructOpcode(nullptr, 2, "LD B, IXl");
   liste_opcodes_dd_[0x46] = FillStructOpcode(nullptr, 2, "LD B, (IX+%n)");
   liste_opcodes_dd_[0x4C] = FillStructOpcode(nullptr, 2, "LD C, IXh");
   liste_opcodes_dd_[0x4D] = FillStructOpcode(nullptr, 2, "LD C, IXl");
   liste_opcodes_dd_[0x4E] = FillStructOpcode(nullptr, 2, "LD C, (IX+%n)");
   liste_opcodes_dd_[0x54] = FillStructOpcode(nullptr, 2, "LD D, IXh");
   liste_opcodes_dd_[0x55] = FillStructOpcode(nullptr, 2, "LD D, IXl");
   liste_opcodes_dd_[0x56] = FillStructOpcode(nullptr, 2, "LD D, (IX+%n)");
   liste_opcodes_dd_[0x5C] = FillStructOpcode(nullptr, 2, "LD E, IXh");
   liste_opcodes_dd_[0x5D] = FillStructOpcode(nullptr, 2, "LD E, IXl");
   liste_opcodes_dd_[0x5E] = FillStructOpcode(nullptr, 2, "LD E, (IX+%n)");
   liste_opcodes_dd_[0x60] = FillStructOpcode(nullptr, 2, "LD IXh, B");
   liste_opcodes_dd_[0x61] = FillStructOpcode(nullptr, 2, "LD IXh, C");
   liste_opcodes_dd_[0x62] = FillStructOpcode(nullptr, 2, "LD IXh, D");
   liste_opcodes_dd_[0x63] = FillStructOpcode(nullptr, 2, "LD IXh, E");
   liste_opcodes_dd_[0x64] = FillStructOpcode(nullptr, 2, "LD IXh, IXh");
   liste_opcodes_dd_[0x65] = FillStructOpcode(nullptr, 2, "LD IXh, IXl");
   liste_opcodes_dd_[0x66] = FillStructOpcode(nullptr, 2, "LD H, (IX+%n)");
   liste_opcodes_dd_[0x67] = FillStructOpcode(nullptr, 2, "LD IXh, A");
   liste_opcodes_dd_[0x68] = FillStructOpcode(nullptr, 2, "LD IXl, B");
   liste_opcodes_dd_[0x69] = FillStructOpcode(nullptr, 2, "LD IXl, C");
   liste_opcodes_dd_[0x6A] = FillStructOpcode(nullptr, 2, "LD IXl, D");
   liste_opcodes_dd_[0x6B] = FillStructOpcode(nullptr, 2, "LD IXl, E");
   liste_opcodes_dd_[0x6C] = FillStructOpcode(nullptr, 2, "LD IXl, IXh");
   liste_opcodes_dd_[0x6D] = FillStructOpcode(nullptr, 2, "LD IXl, IXl");
   liste_opcodes_dd_[0x6E] = FillStructOpcode(nullptr, 2, "LD L, (IX+%n)");
   liste_opcodes_dd_[0x6F] = FillStructOpcode(nullptr, 2, "LD IXh, A");
   liste_opcodes_dd_[0x70] = FillStructOpcode(nullptr, 2, "LD (IX+%n), B");
   liste_opcodes_dd_[0x71] = FillStructOpcode(nullptr, 2, "LD (IX+%n), C");
   liste_opcodes_dd_[0x72] = FillStructOpcode(nullptr, 2, "LD (IX+%n), D");
   liste_opcodes_dd_[0x73] = FillStructOpcode(nullptr, 2, "LD (IX+%n), E");
   liste_opcodes_dd_[0x74] = FillStructOpcode(nullptr, 2, "LD (IX+%n), H");
   liste_opcodes_dd_[0x75] = FillStructOpcode(nullptr, 2, "LD (IX+%n), L");
   liste_opcodes_dd_[0x77] = FillStructOpcode(nullptr, 2, "LD (IX+%n), A");
   liste_opcodes_dd_[0x7C] = FillStructOpcode(nullptr, 2, "LD A, IXh");
   liste_opcodes_dd_[0x7D] = FillStructOpcode(nullptr, 1, "LD A, IXl");
   liste_opcodes_dd_[0x7E] = FillStructOpcode(nullptr, 2, "LD A, (IX+%n)");
   liste_opcodes_dd_[0x84] = FillStructOpcode(nullptr, 2, "ADD A, IXh");
   liste_opcodes_dd_[0x85] = FillStructOpcode(nullptr, 2, "ADD A, IXl");
   liste_opcodes_dd_[0x86] = FillStructOpcode(nullptr, 2, "ADD A, (IX+%n)");
   liste_opcodes_dd_[0x8C] = FillStructOpcode(nullptr, 2, "ADC A, IXh");
   liste_opcodes_dd_[0x8D] = FillStructOpcode(nullptr, 2, "ADC A, IXl");
   liste_opcodes_dd_[0x8E] = FillStructOpcode(nullptr, 2, "ADC A, (IX+%n)");
   liste_opcodes_dd_[0x94] = FillStructOpcode(nullptr, 2, "SUB A, IXh");
   liste_opcodes_dd_[0x95] = FillStructOpcode(nullptr, 2, "SUB A, IXl");
   liste_opcodes_dd_[0x96] = FillStructOpcode(nullptr, 2, "SUB A, (IX+%n)");
   liste_opcodes_dd_[0x9C] = FillStructOpcode(nullptr, 2, "SBC A, IXh");
   liste_opcodes_dd_[0x9D] = FillStructOpcode(nullptr, 2, "SBC A, IXl");
   liste_opcodes_dd_[0x9E] = FillStructOpcode(nullptr, 2, "SBC A, (IX+%n)");
   liste_opcodes_dd_[0xA4] = FillStructOpcode(nullptr, 2, "AND A, IXh");
   liste_opcodes_dd_[0xA5] = FillStructOpcode(nullptr, 2, "AND A, IXl");
   liste_opcodes_dd_[0xA6] = FillStructOpcode(nullptr, 2, "AND (IX+%n)");
   liste_opcodes_dd_[0xAC] = FillStructOpcode(nullptr, 2, "XOR A, IXh");
   liste_opcodes_dd_[0xAD] = FillStructOpcode(nullptr, 2, "XOR A, IXl");
   liste_opcodes_dd_[0xAE] = FillStructOpcode(nullptr, 2, "XOR (IX+%n)");
   liste_opcodes_dd_[0xB4] = FillStructOpcode(nullptr, 2, "OR A, IXh");
   liste_opcodes_dd_[0xB5] = FillStructOpcode(nullptr, 2, "OR A, IXl");
   liste_opcodes_dd_[0xB6] = FillStructOpcode(nullptr, 2, "OR (IX+%n)");
   liste_opcodes_dd_[0xBC] = FillStructOpcode(nullptr, 2, "CP IXh");
   liste_opcodes_dd_[0xBD] = FillStructOpcode(nullptr, 2, "CP (IXl");
   liste_opcodes_dd_[0xBE] = FillStructOpcode(nullptr, 2, "CP (IX+%n)");
   liste_opcodes_dd_[0xCB] = FillStructOpcode(nullptr, 4, "RES b, (IX+%n)");
   liste_opcodes_dd_[0xE1] = FillStructOpcode(nullptr, 1, "POP IX");
   liste_opcodes_dd_[0xE3] = FillStructOpcode(nullptr, 1, "EX SP, IX");
   liste_opcodes_dd_[0xE5] = FillStructOpcode(nullptr, 1, "PUSH IX");
   liste_opcodes_dd_[0xE9] = FillStructOpcode(nullptr, 1, "JP (IX)");
   liste_opcodes_dd_[0xF9] = FillStructOpcode(nullptr, 1, "LD SP, IX");

   // FD
   // Act like normal opcode, except that HL is replaced by IY.
   for (j = 0x00; j <= 0xff; j++)
   {
      liste_opcodes_fd_[j] = liste_opcodes_[j];
   }

   liste_opcodes_fd_[0x09] = FillStructOpcode(nullptr, 1, "ADD IY, BC");
   liste_opcodes_fd_[0x19] = FillStructOpcode(nullptr, 1, "ADD IY, DE");
   liste_opcodes_fd_[0x21] = FillStructOpcode(nullptr, 3, "LD IY, %nn__");
   liste_opcodes_fd_[0x22] = FillStructOpcode(nullptr, 3, "LD (%nn__), IY");
   liste_opcodes_fd_[0x23] = FillStructOpcode(nullptr, 1, "INC IY");
   liste_opcodes_fd_[0x24] = FillStructOpcode(nullptr, 1, "INC IYh");
   liste_opcodes_fd_[0x25] = FillStructOpcode(nullptr, 1, "DEC IYh");
   liste_opcodes_fd_[0x26] = FillStructOpcode(nullptr, 2, "LD IYh, %n");
   liste_opcodes_fd_[0x29] = FillStructOpcode(nullptr, 1, "ADD IY, IY");
   liste_opcodes_fd_[0x2A] = FillStructOpcode(nullptr, 3, "LD IY, (%nn__)");
   liste_opcodes_fd_[0x2B] = FillStructOpcode(nullptr, 1, "DEC IY");
   liste_opcodes_fd_[0x2C] = FillStructOpcode(nullptr, 1, "INC IYl");
   liste_opcodes_fd_[0x2D] = FillStructOpcode(nullptr, 1, "DEC IYl");
   liste_opcodes_fd_[0x2E] = FillStructOpcode(nullptr, 2, "LD IXl, %n");
   liste_opcodes_fd_[0x34] = FillStructOpcode(nullptr, 2, "INC (IY+%n)");
   liste_opcodes_fd_[0x35] = FillStructOpcode(nullptr, 2, "DEC (IY+%n)");
   liste_opcodes_fd_[0x36] = FillStructOpcode(nullptr, 3, "LD (IY+%n), %n");
   liste_opcodes_fd_[0x39] = FillStructOpcode(nullptr, 1, "ADD IY, SP");
   liste_opcodes_fd_[0x44] = FillStructOpcode(nullptr, 2, "LD B, IYh");
   liste_opcodes_fd_[0x45] = FillStructOpcode(nullptr, 2, "LD B, IYl");
   liste_opcodes_fd_[0x46] = FillStructOpcode(nullptr, 2, "LD B, (IY+%n)");
   liste_opcodes_fd_[0x4C] = FillStructOpcode(nullptr, 2, "LD C, IYh");
   liste_opcodes_fd_[0x4D] = FillStructOpcode(nullptr, 2, "LD C, IYl");
   liste_opcodes_fd_[0x4E] = FillStructOpcode(nullptr, 2, "LD C, (IY+%n)");
   liste_opcodes_fd_[0x54] = FillStructOpcode(nullptr, 2, "LD D, IYh");
   liste_opcodes_fd_[0x55] = FillStructOpcode(nullptr, 2, "LD D, IYl");
   liste_opcodes_fd_[0x56] = FillStructOpcode(nullptr, 2, "LD D, (IY+%n)");
   liste_opcodes_fd_[0x5C] = FillStructOpcode(nullptr, 2, "LD E, IYh");
   liste_opcodes_fd_[0x5D] = FillStructOpcode(nullptr, 2, "LD E, IYl");
   liste_opcodes_fd_[0x5E] = FillStructOpcode(nullptr, 2, "LD E, (IY+%n)");
   liste_opcodes_fd_[0x60] = FillStructOpcode(nullptr, 2, "LD IYh, B");
   liste_opcodes_fd_[0x61] = FillStructOpcode(nullptr, 2, "LD IYh, C");
   liste_opcodes_fd_[0x62] = FillStructOpcode(nullptr, 2, "LD IYh, D");
   liste_opcodes_fd_[0x63] = FillStructOpcode(nullptr, 2, "LD IYh, E");
   liste_opcodes_fd_[0x64] = FillStructOpcode(nullptr, 2, "LD IYh, IYh");
   liste_opcodes_fd_[0x65] = FillStructOpcode(nullptr, 2, "LD IYh, IYl");
   liste_opcodes_fd_[0x66] = FillStructOpcode(nullptr, 2, "LD H, (IY+%n)");
   liste_opcodes_fd_[0x67] = FillStructOpcode(nullptr, 2, "LD IYh, A");
   liste_opcodes_fd_[0x68] = FillStructOpcode(nullptr, 2, "LD IYh, B");
   liste_opcodes_fd_[0x69] = FillStructOpcode(nullptr, 2, "LD IYh, C");
   liste_opcodes_fd_[0x6A] = FillStructOpcode(nullptr, 2, "LD IYh, D");
   liste_opcodes_fd_[0x6B] = FillStructOpcode(nullptr, 2, "LD IYh, E");
   liste_opcodes_fd_[0x6C] = FillStructOpcode(nullptr, 2, "LD IYh, IYh");
   liste_opcodes_fd_[0x6D] = FillStructOpcode(nullptr, 2, "LD IYh, IYl");
   liste_opcodes_fd_[0x6E] = FillStructOpcode(nullptr, 2, "LD L, (IY+%n)");
   liste_opcodes_fd_[0x6F] = FillStructOpcode(nullptr, 2, "LD IYh, A");
   liste_opcodes_fd_[0x70] = FillStructOpcode(nullptr, 2, "LD (IY+%n), B");
   liste_opcodes_fd_[0x71] = FillStructOpcode(nullptr, 2, "LD (IY+%n), C");
   liste_opcodes_fd_[0x72] = FillStructOpcode(nullptr, 2, "LD (IY+%n), D");
   liste_opcodes_fd_[0x73] = FillStructOpcode(nullptr, 2, "LD (IY+%n), E");
   liste_opcodes_fd_[0x74] = FillStructOpcode(nullptr, 2, "LD (IY+%n), H");
   liste_opcodes_fd_[0x75] = FillStructOpcode(nullptr, 2, "LD (IY+%n), L");
   liste_opcodes_fd_[0x77] = FillStructOpcode(nullptr, 2, "LD (IY+%n), A");
   liste_opcodes_fd_[0x7C] = FillStructOpcode(nullptr, 2, "LD A, IYh");
   liste_opcodes_fd_[0x7D] = FillStructOpcode(nullptr, 1, "LD A, IYl");
   liste_opcodes_fd_[0x7E] = FillStructOpcode(nullptr, 2, "LD A, (IY+%n)");
   liste_opcodes_fd_[0x84] = FillStructOpcode(nullptr, 2, "ADD A, IYh");
   liste_opcodes_fd_[0x85] = FillStructOpcode(nullptr, 2, "ADD A, IYl");
   liste_opcodes_fd_[0x86] = FillStructOpcode(nullptr, 2, "ADD A, (IY+%n)");
   liste_opcodes_fd_[0x8C] = FillStructOpcode(nullptr, 2, "ADC A, IYh");
   liste_opcodes_fd_[0x8D] = FillStructOpcode(nullptr, 2, "ADC A, IYl");
   liste_opcodes_fd_[0x8E] = FillStructOpcode(nullptr, 2, "ADC A, (IY+%n)");
   liste_opcodes_fd_[0x94] = FillStructOpcode(nullptr, 2, "SUB A, IYh");
   liste_opcodes_fd_[0x95] = FillStructOpcode(nullptr, 2, "SUB A, IYl");
   liste_opcodes_fd_[0x96] = FillStructOpcode(nullptr, 2, "SUB A, (IY+%n)");
   liste_opcodes_fd_[0x9C] = FillStructOpcode(nullptr, 2, "SBC A, IYh");
   liste_opcodes_fd_[0x9D] = FillStructOpcode(nullptr, 2, "SBC A, IYl");
   liste_opcodes_fd_[0x9E] = FillStructOpcode(nullptr, 2, "SBC A, (IY+%n)");
   liste_opcodes_fd_[0xA4] = FillStructOpcode(nullptr, 2, "AND A, IYh");
   liste_opcodes_fd_[0xA5] = FillStructOpcode(nullptr, 2, "AND A, IYl");
   liste_opcodes_fd_[0xA6] = FillStructOpcode(nullptr, 2, "AND (IY+%n)");
   liste_opcodes_fd_[0xAC] = FillStructOpcode(nullptr, 2, "XOR A, IYh");
   liste_opcodes_fd_[0xAD] = FillStructOpcode(nullptr, 2, "XOR A, IYl");
   liste_opcodes_fd_[0xAE] = FillStructOpcode(nullptr, 2, "XOR (IY+%n)");
   liste_opcodes_fd_[0xB4] = FillStructOpcode(nullptr, 2, "OR A, IYh");
   liste_opcodes_fd_[0xB5] = FillStructOpcode(nullptr, 2, "OR A, IYl");
   liste_opcodes_fd_[0xB6] = FillStructOpcode(nullptr, 2, "OR (IY+%n)");
   liste_opcodes_fd_[0xBC] = FillStructOpcode(nullptr, 2, "CP IYh");
   liste_opcodes_fd_[0xBD] = FillStructOpcode(nullptr, 2, "CP (IYl");
   liste_opcodes_fd_[0xBE] = FillStructOpcode(nullptr, 2, "CP (IY+%n)");
   liste_opcodes_fd_[0xCB] = FillStructOpcode(nullptr, 3, "RES %b, (IY+%n)");
   liste_opcodes_fd_[0xE1] = FillStructOpcode(nullptr, 1, "POP_IY");
   liste_opcodes_fd_[0xE3] = FillStructOpcode(nullptr, 1, "EX (SP), IY");
   liste_opcodes_fd_[0xE5] = FillStructOpcode(nullptr, 1, "PUSH_IY");
   liste_opcodes_fd_[0xE9] = FillStructOpcode(nullptr, 1, "JP (IY)");
   liste_opcodes_fd_[0xF9] = FillStructOpcode(nullptr, 1, "LD SP, IY");
#endif
}


unsigned int Z80::Opcode_CB()
{
   current_function_ = &fetch_func_cb_;
   machine_cycle_ = M_FETCH; t_ = 1;
   return 1;
}

unsigned int Z80::Opcode_ED()
{
   current_function_ = &fetch_func_ed_;
   machine_cycle_ = M_FETCH; t_ = 1;
   return 1;
}
unsigned int Z80::Opcode_DD()
{
   current_function_ = &fetch_func_dd_;
   machine_cycle_ = M_FETCH; t_ = 1;
   return 1;
}

unsigned int Z80::Opcode_FD()
{
   current_function_ = &fetch_func_fd_;
   machine_cycle_ = M_FETCH; t_ = 1;
   return 1;
}

unsigned int Z80::Opcode_NOP()
{
   int nextcycle;

   if (!sig_->nmi_)
   {
      if ((!sig_->int_) || !iff1_)
      {
         SET_NOINT
      }
      else
      {
         SET_INT;
      }
   }
   SET_NMI;
}

unsigned int Z80::Opcode_Memory_Read_PC()
{
   machine_cycle_ = M_MEMORY_R;
   t_ = 1;
   current_address_ = pc_;
   current_data_ = 0;
   read_count_ = 0;  // LD BC, nn   
   return 1;
}
