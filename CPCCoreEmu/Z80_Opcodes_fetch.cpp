#include "stdafx.h"

#include "Z80_Full.h"
#include "Sig.h"
#include "Memoire.h"


void Z80::InitOpcodeShortcuts()
{
   unsigned int j;

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

   FillStructOpcode<None>(0xCB, &Z80::Opcode_CB, 1, "%CB");
   FillStructOpcode<None>(0xED, &Z80::Opcode_ED, 1, "%ED");
   FillStructOpcode<None>(0xDD, &Z80::Opcode_DD, 1, "%DD");
   FillStructOpcode<None>(0xFD, &Z80::Opcode_FD, 1, "%FD");


   // Opcodes standards
   ///////////////////////////////////////////////////////////////////////////////////////
   /////////////                           FUNCTION       SIZE   DISASSMBLY
   FillStructOpcode<None>(0x00, &Z80::Opcode_NOP, 1, "NOP");
   FillStructOpcode<None>(0x01, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 3, "LD BC, %nn__");
   FillStructOpcode<None>(0x02, &Z80::Opcode_Memory_Write_Addr_Reg<ADDR_BC, R_A>, 1, "LD (BC), A");
   FillStructOpcode<None>(0x03, &Z80::Opcode_Inc_RegW<ADDR_BC, false>, 1, "INC BC");
   FillStructOpcode<None>(0x04, &Z80::Opcode_Inc_Reg<R_B, false>, 1, "INC B");
   FillStructOpcode<None>(0x05, &Z80::Opcode_Dec_Reg<R_B, false>, 1, "DEC B");
   FillStructOpcode<None>(0x06, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 2, "LD B, %n");
   FillStructOpcode<None>(0x07, &Z80::Opcode_RLCA, 1, "RLCA");
   FillStructOpcode<None>(0x08, &Z80::Opcode_EX<ADDR_AF, ADDR_AFP>, 1, "EX AF AF'");
   FillStructOpcode<None>(0x09, &Z80::Opcode_ADD_REGW<ADDR_HL, ADDR_BC>, 1, "ADD HL, BC");
   FillStructOpcode<None>(0x0A, &Z80::Opcode_Memory_Read_REGW<ADDR_BC>, 1, "LD A, (BC)");
   FillStructOpcode<None>(0x0B, &Z80::Opcode_Dec_RegW<ADDR_BC>, 1, "DEC BC");
   FillStructOpcode<None>(0x0C, &Z80::Opcode_Inc_Reg<R_C, false>, 1, "INC C");
   FillStructOpcode<None>(0x0D, &Z80::Opcode_Dec_Reg<R_C, false>, 1, "DEC C");
   FillStructOpcode<None>(0x0E, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 2, "LD C,  %n");
   FillStructOpcode<None>(0x0F, &Z80::Opcode_RRCA, 1, "RRCA");
   FillStructOpcode<None>(0x10, &Z80::Opcode_Memory_Read_Delayed<ADDR_PC, 5>, 2, "DJNZ %j__");
   FillStructOpcode<None>(0x11, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 3, "LD DE, %nn__");
   FillStructOpcode<None>(0x12, &Z80::Opcode_Memory_Write_Addr_Reg<ADDR_DE, R_A>, 1, "LD (DE), A");
   FillStructOpcode<None>(0x13, &Z80::Opcode_Inc_RegW<ADDR_DE, false>, 1, "INC DE");
   FillStructOpcode<None>(0x14, &Z80::Opcode_Inc_Reg<R_D, false>, 1, "INC D");
   FillStructOpcode<None>(0x15, &Z80::Opcode_Dec_Reg<R_D, false>, 1, "DEC D");
   FillStructOpcode<None>(0x16, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 2, "LD D,  %n");
   FillStructOpcode<None>(0x17, &Z80::Opcode_RLA, 1, "RLA");
   FillStructOpcode<None>(0x18, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 2, "JR %j__");
   FillStructOpcode<None>(0x19, &Z80::Opcode_ADD_REGW<ADDR_HL, ADDR_DE>, 1, "ADD HL, DE");
   FillStructOpcode<None>(0x1A, &Z80::Opcode_Memory_Read_REGW<ADDR_DE>, 1, "LD A, (DE)");
   FillStructOpcode<None>(0x1B, &Z80::Opcode_Dec_RegW<ADDR_DE>, 1, "DEC DE");
   FillStructOpcode<None>(0x1C, &Z80::Opcode_Inc_Reg<R_E, false>, 1, "INC E");
   FillStructOpcode<None>(0x1D, &Z80::Opcode_Dec_Reg<R_E, false>, 1, "DEC E");
   FillStructOpcode<None>(0x1E, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 2, "LD E,  %n");
   FillStructOpcode<None>(0x1F, &Z80::Opcode_RRA, 1, "RRA");
   FillStructOpcode<None>(0x20, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 2, "JR NZ  %j__");
   FillStructOpcode<None>(0x21, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 3, "LD HL, %nn__");
   FillStructOpcode<None>(0x22, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 3, "LD (%nn__), HL");
   FillStructOpcode<None>(0x23, &Z80::Opcode_Inc_RegW<ADDR_HL, false>, 1, "INC HL");
   FillStructOpcode<None>(0x24, &Z80::Opcode_Inc_Reg<R_H, false>, 1, "INC H");
   FillStructOpcode<None>(0x25, &Z80::Opcode_Dec_Reg<R_H, false>, 1, "DEC H");
   FillStructOpcode<None>(0x26, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 2, "LD H,  %n");
   FillStructOpcode<None>(0x27, &Z80::Opcode_DAA, 1, "DAA");
   FillStructOpcode<None>(0x28, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 2, "JR Z  %j__");
   FillStructOpcode<None>(0x29, &Z80::Opcode_ADD_REGW<ADDR_HL, ADDR_HL>, 1, "ADD HL, HL");
   FillStructOpcode<None>(0x2A, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 3, "LD HL, (%nn__)");
   FillStructOpcode<None>(0x2B, &Z80::Opcode_Dec_RegW<ADDR_HL>, 1, "DEC HL");
   FillStructOpcode<None>(0x2C, &Z80::Opcode_Inc_Reg<R_L, false>, 1, "INC L");
   FillStructOpcode<None>(0x2D, &Z80::Opcode_Dec_Reg<R_L, false>, 1, "DEC L");
   FillStructOpcode<None>(0x2E, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 2, "LD L,  %n");
   FillStructOpcode<None>(0x2F, &Z80::Opcode_CPL, 1, "CPL");
   FillStructOpcode<None>(0x30, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 2, "JR NC  %j__");
   FillStructOpcode<None>(0x31, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 3, "LD SP, %nn__");
   FillStructOpcode<None>(0x32, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 3, "LD (%nn__), A");
   FillStructOpcode<None>(0x33, &Z80::Opcode_Inc_RegW<ADDR_SP, false>, 1, "INC SP");
   FillStructOpcode<None>(0x34, &Z80::Opcode_Memory_Read_REGW<ADDR_HL>, 1, "INC (HL)");
   FillStructOpcode<None>(0x35, &Z80::Opcode_Memory_Read_REGW<ADDR_HL>, 1, "DEC (HL)");
   FillStructOpcode<None>(0x36, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 2, "LD (HL), %n");
   FillStructOpcode<None>(0x37, &Z80::Opcode_SCF, 1, "SCF");
   FillStructOpcode<None>(0x38, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 2, "JR C, %j__");
   FillStructOpcode<None>(0x39, &Z80::Opcode_ADD_REGW<ADDR_HL, ADDR_SP>, 1, "ADD HL, SP");
   FillStructOpcode<None>(0x3A, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 3, "LD A, (%nn__)");
   FillStructOpcode<None>(0x3B, &Z80::Opcode_Dec_RegW<ADDR_SP>, 1, "DEC SP");
   FillStructOpcode<None>(0x3C, &Z80::Opcode_Inc_Reg<R_A, false>, 1, "INC A");
   FillStructOpcode<None>(0x3D, &Z80::Opcode_Dec_Reg<R_A, false>, 1, "DEC A");
   FillStructOpcode<None>(0x3E, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 2, "LD A, %n");
   FillStructOpcode<None>(0x3F, &Z80::Opcode_CCF, 1, "CCF");
   FillStructOpcode<None>(0x40, &Z80::Opcode_Ld_Reg<R_B, R_B>, 1, "LD B, B");
   FillStructOpcode<None>(0x41, &Z80::Opcode_Ld_Reg<R_B, R_C>, 1, "LD B, C");
   FillStructOpcode<None>(0x42, &Z80::Opcode_Ld_Reg<R_B, R_D>, 1, "LD B, D");
   FillStructOpcode<None>(0x43, &Z80::Opcode_Ld_Reg<R_B, R_E>, 1, "LD B, E");
   FillStructOpcode<None>(0x44, &Z80::Opcode_Ld_Reg<R_B, R_H>, 1, "LD B, H");
   FillStructOpcode<None>(0x45, &Z80::Opcode_Ld_Reg<R_B, R_L>, 1, "LD B, L");
   FillStructOpcode<None>(0x46, &Z80::Opcode_Memory_Read_REGW<ADDR_HL>, 1, "LD B, (HL)");
   FillStructOpcode<None>(0x47, &Z80::Opcode_Ld_Reg<R_B, R_A>, 1, "LD B, A");
   FillStructOpcode<None>(0x48, &Z80::Opcode_Ld_Reg<R_C, R_B>, 1, "LD C, B");
   FillStructOpcode<None>(0x49, &Z80::Opcode_Ld_Reg<R_C, R_C>, 1, "LD C, C");
   FillStructOpcode<None>(0x4A, &Z80::Opcode_Ld_Reg<R_C, R_D>, 1, "LD C, D");
   FillStructOpcode<None>(0x4B, &Z80::Opcode_Ld_Reg<R_C, R_E>, 1, "LD C, E");
   FillStructOpcode<None>(0x4C, &Z80::Opcode_Ld_Reg<R_C, R_H>, 1, "LD C, H");
   FillStructOpcode<None>(0x4D, &Z80::Opcode_Ld_Reg<R_C, R_L>, 1, "LD C, L");
   FillStructOpcode<None>(0x4E, &Z80::Opcode_Memory_Read_REGW<ADDR_HL>, 1, "LD C, (HL)");
   FillStructOpcode<None>(0x4F, &Z80::Opcode_Ld_Reg<R_C, R_A>, 1, "LD C, A");
   FillStructOpcode<None>(0x50, &Z80::Opcode_Ld_Reg<R_D, R_B>, 1, "LD D, B");
   FillStructOpcode<None>(0x51, &Z80::Opcode_Ld_Reg<R_D, R_C>, 1, "LD D, C");
   FillStructOpcode<None>(0x52, &Z80::Opcode_Ld_Reg<R_D, R_D>, 1, "LD D, D");
   FillStructOpcode<None>(0x53, &Z80::Opcode_Ld_Reg<R_D, R_E>, 1, "LD D, E");
   FillStructOpcode<None>(0x54, &Z80::Opcode_Ld_Reg<R_D, R_H>, 1, "LD D, H");
   FillStructOpcode<None>(0x55, &Z80::Opcode_Ld_Reg<R_D, R_L>, 1, "LD D, L");
   FillStructOpcode<None>(0x56, &Z80::Opcode_Memory_Read_REGW<ADDR_HL>, 1, "LD D, (HL)");
   FillStructOpcode<None>(0x57, &Z80::Opcode_Ld_Reg<R_D, R_A>, 1, "LD D, A");
   FillStructOpcode<None>(0x58, &Z80::Opcode_Ld_Reg<R_E, R_B>, 1, "LD E, B");
   FillStructOpcode<None>(0x59, &Z80::Opcode_Ld_Reg<R_E, R_C>, 1, "LD E, C");
   FillStructOpcode<None>(0x5A, &Z80::Opcode_Ld_Reg<R_E, R_D>, 1, "LD E, D");
   FillStructOpcode<None>(0x5B, &Z80::Opcode_Ld_Reg<R_E, R_E>, 1, "LD E, E");
   FillStructOpcode<None>(0x5C, &Z80::Opcode_Ld_Reg<R_E, R_H>, 1, "LD E, H");
   FillStructOpcode<None>(0x5D, &Z80::Opcode_Ld_Reg<R_E, R_L>, 1, "LD E, L");
   FillStructOpcode<None>(0x5E, &Z80::Opcode_Memory_Read_REGW<ADDR_HL>, 1, "LD E, (HL)");
   FillStructOpcode<None>(0x5F, &Z80::Opcode_Ld_Reg<R_E, R_A>, 1, "LD E, A");
   FillStructOpcode<None>(0x60, &Z80::Opcode_Ld_Reg<R_H, R_B>, 1, "LD H, B");
   FillStructOpcode<None>(0x61, &Z80::Opcode_Ld_Reg<R_H, R_C>, 1, "LD H, C");
   FillStructOpcode<None>(0x62, &Z80::Opcode_Ld_Reg<R_H, R_D>, 1, "LD H, D");
   FillStructOpcode<None>(0x63, &Z80::Opcode_Ld_Reg<R_H, R_E>, 1, "LD H, E");
   FillStructOpcode<None>(0x64, &Z80::Opcode_Ld_Reg<R_H, R_H>, 1, "LD H, H");
   FillStructOpcode<None>(0x65, &Z80::Opcode_Ld_Reg<R_H, R_L>, 1, "LD H, L");
   FillStructOpcode<None>(0x66, &Z80::Opcode_Memory_Read_REGW<ADDR_HL>, 1, "LD H, (HL)");
   FillStructOpcode<None>(0x67, &Z80::Opcode_Ld_Reg<R_H, R_A>, 1, "LD H, A");
   FillStructOpcode<None>(0x68, &Z80::Opcode_Ld_Reg<R_L, R_B>, 1, "LD L, B");
   FillStructOpcode<None>(0x69, &Z80::Opcode_Ld_Reg<R_L, R_C>, 1, "LD L, C");
   FillStructOpcode<None>(0x6A, &Z80::Opcode_Ld_Reg<R_L, R_D>, 1, "LD L, D");
   FillStructOpcode<None>(0x6B, &Z80::Opcode_Ld_Reg<R_L, R_E>, 1, "LD L, E");
   FillStructOpcode<None>(0x6C, &Z80::Opcode_Ld_Reg<R_L, R_H>, 1, "LD L, H");
   FillStructOpcode<None>(0x6D, &Z80::Opcode_Ld_Reg<R_L, R_L>, 1, "LD L, L");
   FillStructOpcode<None>(0x6E, &Z80::Opcode_Memory_Read_REGW<ADDR_HL>, 1, "LD L, (HL)");
   FillStructOpcode<None>(0x6F, &Z80::Opcode_Ld_Reg<R_L, R_A>, 1, "LD L, A");
   FillStructOpcode<None>(0x70, &Z80::Opcode_Memory_Write_Addr_Reg<ADDR_HL, R_B>, 1, "LD (HL), B");
   FillStructOpcode<None>(0x71, &Z80::Opcode_Memory_Write_Addr_Reg<ADDR_HL, R_C>, 1, "LD (HL), C");
   FillStructOpcode<None>(0x72, &Z80::Opcode_Memory_Write_Addr_Reg<ADDR_HL, R_D>, 1, "LD (HL), D");
   FillStructOpcode<None>(0x73, &Z80::Opcode_Memory_Write_Addr_Reg<ADDR_HL, R_E>, 1, "LD (HL), E");
   FillStructOpcode<None>(0x74, &Z80::Opcode_Memory_Write_Addr_Reg<ADDR_HL, R_H>, 1, "LD (HL), H");
   FillStructOpcode<None>(0x75, &Z80::Opcode_Memory_Write_Addr_Reg<ADDR_HL, R_L>, 1, "LD (HL), L");
   FillStructOpcode<None>(0x76, &Z80::Opcode_HALT, 1, "HALT");
   FillStructOpcode<None>(0x77, &Z80::Opcode_Memory_Write_Addr_Reg<ADDR_HL, R_A>, 1, "LD (HL), A");
   FillStructOpcode<None>(0x78, &Z80::Opcode_Ld_Reg<R_A, R_B>, 1, "LD A, B");
   FillStructOpcode<None>(0x79, &Z80::Opcode_Ld_Reg<R_A, R_C>, 1, "LD A, C");
   FillStructOpcode<None>(0x7A, &Z80::Opcode_Ld_Reg<R_A, R_D>, 1, "LD A, D");
   FillStructOpcode<None>(0x7B, &Z80::Opcode_Ld_Reg<R_A, R_E>, 1, "LD A, E");
   FillStructOpcode<None>(0x7C, &Z80::Opcode_Ld_Reg<R_A, R_H>, 1, "LD A, H");
   FillStructOpcode<None>(0x7D, &Z80::Opcode_Ld_Reg<R_A, R_L>, 1, "LD A, L");
   FillStructOpcode<None>(0x7E, &Z80::Opcode_Memory_Read_REGW<ADDR_HL>, 1, "LD A, (HL)");
   FillStructOpcode<None>(0x7F, &Z80::Opcode_Ld_Reg<R_A, R_A>, 1, "LD A, A");
   FillStructOpcode<None>(0x80, &Z80::Opcode_Add_Reg<R_B, false>, 1, "ADD A, B");
   FillStructOpcode<None>(0x81, &Z80::Opcode_Add_Reg<R_C, false>, 1, "ADD A, C");
   FillStructOpcode<None>(0x82, &Z80::Opcode_Add_Reg<R_D, false>, 1, "ADD A, D");
   FillStructOpcode<None>(0x83, &Z80::Opcode_Add_Reg<R_E, false>, 1, "ADD A, E");
   FillStructOpcode<None>(0x84, &Z80::Opcode_Add_Reg<R_H, false>, 1, "ADD A, H");
   FillStructOpcode<None>(0x85, &Z80::Opcode_Add_Reg<R_L, false>, 1, "ADD A, L");
   FillStructOpcode<None>(0x86, &Z80::Opcode_Memory_Read_REGW<ADDR_HL>, 1, "ADD A, (HL)");
   FillStructOpcode<None>(0x87, &Z80::Opcode_Add_Reg<R_A, false>, 1, "ADD A, A");
   FillStructOpcode<None>(0x88, &Z80::Opcode_Add_Reg<R_B, true>, 1, "ADC A, B");
   FillStructOpcode<None>(0x89, &Z80::Opcode_Add_Reg<R_C, true>, 1, "ADC A, C");
   FillStructOpcode<None>(0x8A, &Z80::Opcode_Add_Reg<R_D, true>, 1, "ADC A, D");
   FillStructOpcode<None>(0x8B, &Z80::Opcode_Add_Reg<R_E, true>, 1, "ADC A, E");
   FillStructOpcode<None>(0x8C, &Z80::Opcode_Add_Reg<R_H, true>, 1, "ADC A, H");
   FillStructOpcode<None>(0x8D, &Z80::Opcode_Add_Reg<R_L, true>, 1, "ADC A, L");
   FillStructOpcode<None>(0x8E, &Z80::Opcode_Memory_Read_REGW<ADDR_HL>, 1, "ADC A, (HL)");
   FillStructOpcode<None>(0x8F, &Z80::Opcode_Add_Reg<R_A, true>, 1, "ADC A, A");
   FillStructOpcode<None>(0x90, &Z80::Opcode_Sub_Reg<R_B, false>, 1, "SUB A, B");
   FillStructOpcode<None>(0x91, &Z80::Opcode_Sub_Reg<R_C, false>, 1, "SUB A, C");
   FillStructOpcode<None>(0x92, &Z80::Opcode_Sub_Reg<R_D, false>, 1, "SUB A, D");
   FillStructOpcode<None>(0x93, &Z80::Opcode_Sub_Reg<R_E, false>, 1, "SUB A, E");
   FillStructOpcode<None>(0x94, &Z80::Opcode_Sub_Reg<R_H, false>, 1, "SUB A, H");
   FillStructOpcode<None>(0x95, &Z80::Opcode_Sub_Reg<R_L, false>, 1, "SUB A, L");
   FillStructOpcode<None>(0x96, &Z80::Opcode_Memory_Read_REGW<ADDR_HL>, 1, "SUB A, (HL)");
   FillStructOpcode<None>(0x97, &Z80::Opcode_Sub_Reg<R_A, false>, 1, "SUB A, A");
   FillStructOpcode<None>(0x98, &Z80::Opcode_Sub_Reg<R_B, true>, 1, "SBC A, B");
   FillStructOpcode<None>(0x99, &Z80::Opcode_Sub_Reg<R_C, true>, 1, "SBC A, C");
   FillStructOpcode<None>(0x9A, &Z80::Opcode_Sub_Reg<R_D, true>, 1, "SBC A, D");
   FillStructOpcode<None>(0x9B, &Z80::Opcode_Sub_Reg<R_E, true>, 1, "SBC A, E");
   FillStructOpcode<None>(0x9C, &Z80::Opcode_Sub_Reg<R_H, true>, 1, "SBC A, H");
   FillStructOpcode<None>(0x9D, &Z80::Opcode_Sub_Reg<R_L, true>, 1, "SBC A, L");
   FillStructOpcode<None>(0x9E, &Z80::Opcode_Memory_Read_REGW<ADDR_HL>, 1, "SBC A, (HL)");
   FillStructOpcode<None>(0x9F, &Z80::Opcode_Sub_Reg<R_A, true>, 1, "SBC A, A");
   FillStructOpcode<None>(0xA0, &Z80::Opcode_BOOL_Reg<R_B, AND>, 1, "AND B");
   FillStructOpcode<None>(0xA1, &Z80::Opcode_BOOL_Reg<R_C, AND>, 1, "AND C");
   FillStructOpcode<None>(0xA2, &Z80::Opcode_BOOL_Reg<R_D, AND>, 1, "AND D");
   FillStructOpcode<None>(0xA3, &Z80::Opcode_BOOL_Reg<R_E, AND>, 1, "AND E");
   FillStructOpcode<None>(0xA4, &Z80::Opcode_BOOL_Reg<R_H, AND>, 1, "AND H");
   FillStructOpcode<None>(0xA5, &Z80::Opcode_BOOL_Reg<R_L, AND>, 1, "AND L");
   FillStructOpcode<None>(0xA6, &Z80::Opcode_Memory_Read_REGW<ADDR_HL>, 1, "AND (HL)");
   FillStructOpcode<None>(0xA7, &Z80::Opcode_BOOL_Reg<R_A, AND>, 1, "AND A");
   FillStructOpcode<None>(0xA8, &Z80::Opcode_BOOL_Reg<R_B, XOR>, 1, "XOR B");
   FillStructOpcode<None>(0xA9, &Z80::Opcode_BOOL_Reg<R_C, XOR>, 1, "XOR C");
   FillStructOpcode<None>(0xAA, &Z80::Opcode_BOOL_Reg<R_D, XOR>, 1, "XOR D");
   FillStructOpcode<None>(0xAB, &Z80::Opcode_BOOL_Reg<R_E, XOR>, 1, "XOR E");
   FillStructOpcode<None>(0xAC, &Z80::Opcode_BOOL_Reg<R_H, XOR>, 1, "XOR H");
   FillStructOpcode<None>(0xAD, &Z80::Opcode_BOOL_Reg<R_L, XOR>, 1, "XOR L");
   FillStructOpcode<None>(0xAE, &Z80::Opcode_Memory_Read_REGW<ADDR_HL>, 1, "XOR (HL)");
   FillStructOpcode<None>(0xAF, &Z80::Opcode_BOOL_Reg<R_A, XOR>, 1, "XOR A");
   FillStructOpcode<None>(0xB0, &Z80::Opcode_BOOL_Reg<R_B, OR>, 1, "OR B");
   FillStructOpcode<None>(0xB1, &Z80::Opcode_BOOL_Reg<R_C, OR>, 1, "OR C");
   FillStructOpcode<None>(0xB2, &Z80::Opcode_BOOL_Reg<R_D, OR>, 1, "OR D");
   FillStructOpcode<None>(0xB3, &Z80::Opcode_BOOL_Reg<R_E, OR>, 1, "OR E");
   FillStructOpcode<None>(0xB4, &Z80::Opcode_BOOL_Reg<R_H, OR>, 1, "OR H");
   FillStructOpcode<None>(0xB5, &Z80::Opcode_BOOL_Reg<R_L, OR>, 1, "OR L");
   FillStructOpcode<None>(0xB6, &Z80::Opcode_Memory_Read_REGW<ADDR_HL>, 1, "OR (HL)");
   FillStructOpcode<None>(0xB7, &Z80::Opcode_BOOL_Reg<R_A, OR>, 1, "OR A");
   FillStructOpcode<None>(0xB8, &Z80::Opcode_CP_Reg<R_B>, 1, "CP B");
   FillStructOpcode<None>(0xB9, &Z80::Opcode_CP_Reg<R_C>, 1, "CP C");
   FillStructOpcode<None>(0xBA, &Z80::Opcode_CP_Reg<R_D>, 1, "CP D");
   FillStructOpcode<None>(0xBB, &Z80::Opcode_CP_Reg<R_E>, 1, "CP E");
   FillStructOpcode<None>(0xBC, &Z80::Opcode_CP_Reg<R_H>, 1, "CP H");
   FillStructOpcode<None>(0xBD, &Z80::Opcode_CP_Reg<R_L>, 1, "CP L");
   FillStructOpcode<None>(0xBE, &Z80::Opcode_Memory_Read_REGW<ADDR_HL>, 1, "CP (HL)");
   FillStructOpcode<None>(0xBF, &Z80::Opcode_CP_Reg<R_A>, 1, "CP A");
   FillStructOpcode<None>(0xC0, &Z80::Opcode_Ret_Cond< ZF, false>, 1, "RET NZ");
   FillStructOpcode<None>(0xC1, &Z80::Opcode_MemoryFromStack, 1, "POP BC");
   FillStructOpcode<None>(0xC2, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 3, "JP NZ %nn__");
   FillStructOpcode<None>(0xC3, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 3, "JP %nn__");
   FillStructOpcode<None>(0xC4, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 3, "CALL NZ %nn__");
   FillStructOpcode<None>(0xC5, &Z80::Opcode_Push<ADDR_BC>, 1, "PUSH BC");
   FillStructOpcode<None>(0xC6, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 2, "ADD A, %n");
   FillStructOpcode<None>(0xC7, &Z80::DefaultFetch, 1, "RST 0");
   FillStructOpcode<None>(0xC8, &Z80::Opcode_Ret_Cond< ZF, true>, 1, "RET Z");
   FillStructOpcode<None>(0xC9, &Z80::Opcode_MemoryFromStack, 1, "RET");
   FillStructOpcode<None>(0xCA, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 3, "JP Z %nn__");

   FillStructOpcode<None>(0xCC, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 3, "CALL Z %nn__");
   FillStructOpcode<None>(0xCD, &Z80::DefaultFetch, 3, "CALL %nn__");
   FillStructOpcode<None>(0xCE, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 2, "ADC A, %n");
   FillStructOpcode<None>(0xCF, &Z80::DefaultFetch, 1, "RST 08H");
   FillStructOpcode<None>(0xD0, &Z80::Opcode_Ret_Cond< CF, false>, 1, "RET NC");
   FillStructOpcode<None>(0xD1, &Z80::Opcode_MemoryFromStack, 1, "POP DE");
   FillStructOpcode<None>(0xD2, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 3, "JP NC %nn__");
   FillStructOpcode<None>(0xD3, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 2, "OUT (%n), A");
   FillStructOpcode<None>(0xD4, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 3, "CALL NC %nn__");
   FillStructOpcode<None>(0xD5, &Z80::Opcode_Push<ADDR_DE>, 1, "PUSH DE");
   FillStructOpcode<None>(0xD6, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 2, "SUB A, %n");
   FillStructOpcode<None>(0xD7, &Z80::DefaultFetch, 1, "RST 10H");
   FillStructOpcode<None>(0xD8, &Z80::Opcode_Ret_Cond< CF, true>, 1, "RET C");
   FillStructOpcode<None>(0xD9, &Z80::DefaultFetch, 1, "EXX");
   FillStructOpcode<None>(0xDA, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 3, "JP C %nn__");
   FillStructOpcode<None>(0xDB, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 2, "IN A, (%n)");
   FillStructOpcode<None>(0xDC, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 3, "CALL C %nn__");
   FillStructOpcode<None>(0xDE, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 2, "SBC A, %n");
   FillStructOpcode<None>(0xDF, &Z80::DefaultFetch, 1, "RST 18H");
   FillStructOpcode<None>(0xE0, &Z80::Opcode_Ret_Cond< PF, false>, 1, "RET PO");
   FillStructOpcode<None>(0xE1, &Z80::Opcode_MemoryFromStack, 1, "POP HL");
   FillStructOpcode<None>(0xE2, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 3, "JP PO %nn__");
   FillStructOpcode<None>(0xE3, &Z80::DefaultFetch, 1, "EX (SP), HL");
   FillStructOpcode<None>(0xE4, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 3, "CALL PO %nn__");
   FillStructOpcode<None>(0xE5, &Z80::Opcode_Push<ADDR_HL>, 1, "PUSH HL");
   FillStructOpcode<None>(0xE6, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 2, "AND %n");
   FillStructOpcode<None>(0xE7, &Z80::DefaultFetch, 1, "RST 20H");
   FillStructOpcode<None>(0xE8, &Z80::Opcode_Ret_Cond< PF, true>, 1, "RET PE");
   FillStructOpcode<None>(0xE9, &Z80::DefaultFetch, 1, "JP (HL)");
   FillStructOpcode<None>(0xEA, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 3, "JP PE %nn__");
   FillStructOpcode<None>(0xEB, &Z80::Opcode_EX<ADDR_DE, ADDR_HL>, 1, "EX DE,HL");
   FillStructOpcode<None>(0xEC, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 3, "CALL PE %nn__");
   FillStructOpcode<None>(0xEE, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 2, "XOR %n");
   FillStructOpcode<None>(0xEF, &Z80::DefaultFetch, 1, "RST 28H");
   FillStructOpcode<None>(0xF0, &Z80::Opcode_Ret_Cond< SF, false>, 1, "RET P");
   FillStructOpcode<None>(0xF1, &Z80::Opcode_MemoryFromStack, 1, "POP AF");
   FillStructOpcode<None>(0xF2, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 3, "JP P %nn__");
   FillStructOpcode<None>(0xF3, &Z80::DefaultFetch, 1, "DI");
   FillStructOpcode<None>(0xF4, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 3, "CALL P %nn__");
   FillStructOpcode<None>(0xF5, &Z80::Opcode_Push<ADDR_AF>, 1, "PUSH AF");
   FillStructOpcode<None>(0xF6, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 2, "OR %n");
   FillStructOpcode<None>(0xF7, &Z80::DefaultFetch, 1, "RST 30H");
   FillStructOpcode<None>(0xF8, &Z80::Opcode_Ret_Cond< SF, true>, 1, "RET M");
   FillStructOpcode<None>(0xF9, &Z80::DefaultFetch, 1, "LD SP, HL");
   FillStructOpcode<None>(0xFA, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 3, "JP M %nn__");
   FillStructOpcode<None>(0xFB, &Z80::DefaultFetch, 1, "EI");
   FillStructOpcode<None>(0xFC, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 3, "CALL M %nn__");
   FillStructOpcode<None>(0xFE, &Z80::Opcode_Memory_Read_REGW<ADDR_PC>, 2, "CP %n");
   FillStructOpcode<None>(0xFF, &Z80::DefaultFetch, 1, "RST 38H");
#if 0
   // Opcode a multiple byte
   // CB
   for (j = 0x00; j <= 0x07; j++) liste_opcodes_cb_[j] = FillStructOpcode<CB>(nullptr, 1, "RLC %r");
   for (j = 0x08; j <= 0x0F; j++) liste_opcodes_cb_[j] = FillStructOpcode<CB>(nullptr, 1, "RRC %r");
   for (j = 0x10; j <= 0x17; j++) liste_opcodes_cb_[j] = FillStructOpcode<CB>(nullptr, 1, "RL %r");
   for (j = 0x18; j <= 0x1F; j++) liste_opcodes_cb_[j] = FillStructOpcode<CB>(nullptr, 1, "RR %r");
   for (j = 0x20; j <= 0x27; j++) liste_opcodes_cb_[j] = FillStructOpcode<CB>(nullptr, 1, "SLA %r");
   for (j = 0x28; j <= 0x2F; j++) liste_opcodes_cb_[j] = FillStructOpcode<CB>(nullptr, 1, "SRA %r");
   for (j = 0x30; j <= 0x37; j++) liste_opcodes_cb_[j] = FillStructOpcode<CB>(nullptr, 1, "SLL %r");
   for (j = 0x38; j <= 0x3F; j++) liste_opcodes_cb_[j] = FillStructOpcode<CB>(nullptr, 1, "SRL %r");
   DEF_OP_BIT(A); DEF_OP_BIT(B); DEF_OP_BIT(C); DEF_OP_BIT(D); DEF_OP_BIT(E); DEF_OP_BIT(H); DEF_OP_BIT(L); DEF_OP_BIT(HL);

   for (j = 0x80; j <= 0xBF; j++) liste_opcodes_cb_[j] = FillStructOpcode<CB>(nullptr, 1, "RES %b,%r");
   for (j = 0xC0; j <= 0xFF; j++) liste_opcodes_cb_[j] = FillStructOpcode<CB>(nullptr, 1, "SET %b,%r");

   // ED
   for (j = 0x00; j <= 0xFF; j++) liste_opcodes_ed_[j] = FillStructOpcode<ED>(nullptr, 1, "Default ED");

   liste_opcodes_ed_[0x40] = FillStructOpcode<ED>(nullptr, 1, "IN B, (C)");
   liste_opcodes_ed_[0x41] = FillStructOpcode<ED>(nullptr, 1, "OUT (C), B");
   liste_opcodes_ed_[0x42] = FillStructOpcode<ED>(nullptr, 1, "SBL HL, BC");
   liste_opcodes_ed_[0x43] = FillStructOpcode<ED>(nullptr, 3, "LD (%nn__), BC");
   liste_opcodes_ed_[0x44] = FillStructOpcode<ED>(nullptr, 1, "NEG");
   liste_opcodes_ed_[0x45] = FillStructOpcode<ED>(nullptr, 1, "RETN");
   liste_opcodes_ed_[0x46] = FillStructOpcode<ED>(nullptr, 1, "IM 0");
   liste_opcodes_ed_[0x47] = FillStructOpcode<ED>(nullptr, 1, "LD I, A");
   liste_opcodes_ed_[0x48] = FillStructOpcode<ED>(nullptr, 1, "IN C, (C)");
   liste_opcodes_ed_[0x49] = FillStructOpcode<ED>(nullptr, 1, "OUT (C), C");
   liste_opcodes_ed_[0x4A] = FillStructOpcode<ED>(nullptr, 1, "ADC HL, BC");
   liste_opcodes_ed_[0x4B] = FillStructOpcode<ED>(nullptr, 3, "LD BC, (%nn__)");
   liste_opcodes_ed_[0x4C] = FillStructOpcode<ED>(nullptr, 1, "NEG");
   liste_opcodes_ed_[0x4D] = FillStructOpcode<ED>(nullptr, 1, "RETI");
   liste_opcodes_ed_[0x4E] = FillStructOpcode<ED>(nullptr, 1, "IM 0");
   liste_opcodes_ed_[0x4F] = FillStructOpcode<ED>(nullptr, 1, "LD R, A");
   liste_opcodes_ed_[0x50] = FillStructOpcode<ED>(nullptr, 1, "IN D, (C)");
   liste_opcodes_ed_[0x51] = FillStructOpcode<ED>(nullptr, 1, "OUT (C), D");
   liste_opcodes_ed_[0x52] = FillStructOpcode<ED>(nullptr, 1, "SBL HL, DE");
   liste_opcodes_ed_[0x53] = FillStructOpcode<ED>(nullptr, 3, "LD (%nn__), DE");
   liste_opcodes_ed_[0x54] = FillStructOpcode<ED>(nullptr, 1, "NEG");
   liste_opcodes_ed_[0x55] = FillStructOpcode<ED>(nullptr, 1, "RETN");
   liste_opcodes_ed_[0x56] = FillStructOpcode<ED>(nullptr, 1, "IM 1");
   liste_opcodes_ed_[0x57] = FillStructOpcode<ED>(nullptr, 1, "LD A, I");
   liste_opcodes_ed_[0x58] = FillStructOpcode<ED>(nullptr, 1, "IN E, (C)");
   liste_opcodes_ed_[0x59] = FillStructOpcode<ED>(nullptr, 1, "OUT (C), E");
   liste_opcodes_ed_[0x5A] = FillStructOpcode<ED>(nullptr, 1, "ADC HL, DE");
   liste_opcodes_ed_[0x5B] = FillStructOpcode<ED>(nullptr, 3, "LD DE, (%nn__)");
   liste_opcodes_ed_[0x5C] = FillStructOpcode<ED>(nullptr, 1, "NEG");
   liste_opcodes_ed_[0x5D] = FillStructOpcode<ED>(nullptr, 1, "RETN");
   liste_opcodes_ed_[0x5E] = FillStructOpcode<ED>(nullptr, 1, "IM2");
   liste_opcodes_ed_[0x5F] = FillStructOpcode<ED>(nullptr, 1, "LD A, R");
   liste_opcodes_ed_[0x60] = FillStructOpcode<ED>(nullptr, 1, "IN H, (C)");
   liste_opcodes_ed_[0x61] = FillStructOpcode<ED>(nullptr, 1, "OUT (C), H");
   liste_opcodes_ed_[0x62] = FillStructOpcode<ED>(nullptr, 1, "SBC HL, HL");
   liste_opcodes_ed_[0x63] = FillStructOpcode<ED>(nullptr, 3, "LN (%nn__), HL");
   liste_opcodes_ed_[0x64] = FillStructOpcode<ED>(nullptr, 1, "NEG");
   liste_opcodes_ed_[0x65] = FillStructOpcode<ED>(nullptr, 1, "RETN");
   liste_opcodes_ed_[0x66] = FillStructOpcode<ED>(nullptr, 1, "IM 0");
   liste_opcodes_ed_[0x67] = FillStructOpcode<ED>(nullptr, 1, "RRD");
   liste_opcodes_ed_[0x68] = FillStructOpcode<ED>(nullptr, 1, "IN L, (C)");
   liste_opcodes_ed_[0x69] = FillStructOpcode<ED>(nullptr, 1, "OUT (C), L");
   liste_opcodes_ed_[0x6A] = FillStructOpcode<ED>(nullptr, 1, "ADC HL, HL");
   liste_opcodes_ed_[0x6B] = FillStructOpcode<ED>(nullptr, 3, "LN HL, (%nn__)");
   liste_opcodes_ed_[0x6C] = FillStructOpcode<ED>(nullptr, 1, "NEG");
   liste_opcodes_ed_[0x6D] = FillStructOpcode<ED>(nullptr, 1, "RETN");
   liste_opcodes_ed_[0x6E] = FillStructOpcode<ED>(nullptr, 1, "IM 0");
   liste_opcodes_ed_[0x6F] = FillStructOpcode<ED>(nullptr, 1, "RLD");
   liste_opcodes_ed_[0x70] = FillStructOpcode<ED>(nullptr, 1, "IN (C)");
   liste_opcodes_ed_[0x71] = FillStructOpcode<ED>(nullptr, 1, "OUT (C), 0");
   liste_opcodes_ed_[0x72] = FillStructOpcode<ED>(nullptr, 1, "SBC HL, SP");
   liste_opcodes_ed_[0x73] = FillStructOpcode<ED>(nullptr, 3, "LD (%nn__), SP");
   liste_opcodes_ed_[0x74] = FillStructOpcode<ED>(nullptr, 1, "NEG");
   liste_opcodes_ed_[0x75] = FillStructOpcode<ED>(nullptr, 1, "RETN");
   liste_opcodes_ed_[0x76] = FillStructOpcode<ED>(nullptr, 1, "IM 1");
   liste_opcodes_ed_[0x77] = FillStructOpcode<ED>(nullptr, 1, "NOP");
   liste_opcodes_ed_[0x78] = FillStructOpcode<ED>(nullptr, 1, "IN A, (C)");
   liste_opcodes_ed_[0x79] = FillStructOpcode<ED>(nullptr, 1, "OUT (C), A");
   liste_opcodes_ed_[0x7A] = FillStructOpcode<ED>(nullptr, 1, "ADC HL, SP");
   liste_opcodes_ed_[0x7B] = FillStructOpcode<ED>(nullptr, 3, "LD SP, (%nn__)");
   liste_opcodes_ed_[0x7C] = FillStructOpcode<ED>(nullptr, 1, "NEG");
   liste_opcodes_ed_[0x7D] = FillStructOpcode<ED>(nullptr, 1, "RETN");
   liste_opcodes_ed_[0x7E] = FillStructOpcode<ED>(nullptr, 1, "IM 2");
   liste_opcodes_ed_[0x7F] = FillStructOpcode<ED>(nullptr, 1, "NOP");
   liste_opcodes_ed_[0xA0] = FillStructOpcode<ED>(nullptr, 1, "LDI");
   liste_opcodes_ed_[0xA1] = FillStructOpcode<ED>(nullptr, 1, "CPI");
   liste_opcodes_ed_[0xA2] = FillStructOpcode<ED>(nullptr, 1, "INI");
   liste_opcodes_ed_[0xA3] = FillStructOpcode<ED>(nullptr, 1, "OUTI");
   liste_opcodes_ed_[0xA8] = FillStructOpcode<ED>(nullptr, 1, "LDD");
   liste_opcodes_ed_[0xA9] = FillStructOpcode<ED>(nullptr, 1, "CPD");
   liste_opcodes_ed_[0xAA] = FillStructOpcode<ED>(nullptr, 1, "IND");
   liste_opcodes_ed_[0xAB] = FillStructOpcode<ED>(nullptr, 1, "OUTD");
   liste_opcodes_ed_[0xB0] = FillStructOpcode<ED>(nullptr, 1, "LDIR");
   liste_opcodes_ed_[0xB1] = FillStructOpcode<ED>(nullptr, 1, "CPIR");
   liste_opcodes_ed_[0xB2] = FillStructOpcode<ED>(nullptr, 1, "INIR");
   liste_opcodes_ed_[0xB3] = FillStructOpcode<ED>(nullptr, 1, "OTIR");
   liste_opcodes_ed_[0xB8] = FillStructOpcode<ED>(nullptr, 1, "LDDR");
   liste_opcodes_ed_[0xB9] = FillStructOpcode<ED>(nullptr, 1, "CPDR");
   liste_opcodes_ed_[0xBA] = FillStructOpcode<ED>(nullptr, 1, "INDR");
   liste_opcodes_ed_[0xBB] = FillStructOpcode<ED>(nullptr, 1, "OTDR");
   liste_opcodes_ed_[0xED] = FillStructOpcode<ED>(nullptr, 1, "%ED");
#endif
   // Par defaut, tout le monde pointe sur NOP
   // DD
   // Act like normal opcode, except that HL is replaced by IX.
   for (j = 0x00; j <= 0xff; j++)
   {
      liste_opcodes_dd_[j] = liste_opcodes_[j];
   }

   FillStructOpcode<DD>(0x09, &Z80::DefaultFetch, 1, "ADD IX, BC");
   FillStructOpcode<DD>(0x19, &Z80::DefaultFetch, 1, "ADD IX, DE");
   FillStructOpcode<DD>(0x21, &Z80::DefaultFetch, 3, "LD IX, %nn__");
   FillStructOpcode<DD>(0x22, &Z80::DefaultFetch, 3, "LD (%nn__), IX");
   FillStructOpcode<DD>(0x23, &Z80::DefaultFetch, 1, "INC IX");
   FillStructOpcode<DD>(0x24, &Z80::DefaultFetch, 1, "INC IXh");
   FillStructOpcode<DD>(0x25, &Z80::DefaultFetch, 1, "DEC IXh");
   FillStructOpcode<DD>(0x26, &Z80::DefaultFetch, 2, "LD IXh, %n");
   FillStructOpcode<DD>(0x29, &Z80::DefaultFetch, 1, "ADD IX, IX");
   FillStructOpcode<DD>(0x2A, &Z80::DefaultFetch, 3, "LD IX, (%nn__)");
   FillStructOpcode<DD>(0x2B, &Z80::DefaultFetch, 1, "DEC IX");
   FillStructOpcode<DD>(0x2C, &Z80::DefaultFetch, 1, "INC IXl");
   FillStructOpcode<DD>(0x2D, &Z80::DefaultFetch, 1, "DEC IXl");
   FillStructOpcode<DD>(0x2E, &Z80::DefaultFetch, 2, "LD IXl, %n");
   FillStructOpcode<DD>(0x34, &Z80::DefaultFetch, 1, "INC (IX+%n)");
   FillStructOpcode<DD>(0x35, &Z80::DefaultFetch, 1, "DEC (IX+%n)");
   FillStructOpcode<DD>(0x36, &Z80::DefaultFetch, 3, "LD (IX+%n), %n");
   FillStructOpcode<DD>(0x39, &Z80::DefaultFetch, 1, "ADD IX, SP");
   FillStructOpcode<DD>(0x44, &Z80::DefaultFetch, 2, "LD B, IXh");
   FillStructOpcode<DD>(0x45, &Z80::DefaultFetch, 2, "LD B, IXl");
   FillStructOpcode<DD>(0x46, &Z80::DefaultFetch, 2, "LD B, (IX+%n)");
   FillStructOpcode<DD>(0x4C, &Z80::DefaultFetch, 2, "LD C, IXh");
   FillStructOpcode<DD>(0x4D, &Z80::DefaultFetch, 2, "LD C, IXl");
   FillStructOpcode<DD>(0x4E, &Z80::DefaultFetch, 2, "LD C, (IX+%n)");
   FillStructOpcode<DD>(0x54, &Z80::DefaultFetch, 2, "LD D, IXh");
   FillStructOpcode<DD>(0x55, &Z80::DefaultFetch, 2, "LD D, IXl");
   FillStructOpcode<DD>(0x56, &Z80::DefaultFetch, 2, "LD D, (IX+%n)");
   FillStructOpcode<DD>(0x5C, &Z80::DefaultFetch, 2, "LD E, IXh");
   FillStructOpcode<DD>(0x5D, &Z80::DefaultFetch, 2, "LD E, IXl");
   FillStructOpcode<DD>(0x5E, &Z80::DefaultFetch, 2, "LD E, (IX+%n)");
   FillStructOpcode<DD>(0x60, &Z80::DefaultFetch, 2, "LD IXh, B");
   FillStructOpcode<DD>(0x61, &Z80::DefaultFetch, 2, "LD IXh, C");
   FillStructOpcode<DD>(0x62, &Z80::DefaultFetch, 2, "LD IXh, D");
   FillStructOpcode<DD>(0x63, &Z80::DefaultFetch, 2, "LD IXh, E");
   FillStructOpcode<DD>(0x64, &Z80::DefaultFetch, 2, "LD IXh, IXh");
   FillStructOpcode<DD>(0x65, &Z80::DefaultFetch, 2, "LD IXh, IXl");
   FillStructOpcode<DD>(0x66, &Z80::DefaultFetch, 2, "LD H, (IX+%n)");
   FillStructOpcode<DD>(0x67, &Z80::DefaultFetch, 2, "LD IXh, A");
   FillStructOpcode<DD>(0x68, &Z80::DefaultFetch, 2, "LD IXl, B");
   FillStructOpcode<DD>(0x69, &Z80::DefaultFetch, 2, "LD IXl, C");
   FillStructOpcode<DD>(0x6A, &Z80::DefaultFetch, 2, "LD IXl, D");
   FillStructOpcode<DD>(0x6B, &Z80::DefaultFetch, 2, "LD IXl, E");
   FillStructOpcode<DD>(0x6C, &Z80::DefaultFetch, 2, "LD IXl, IXh");
   FillStructOpcode<DD>(0x6D, &Z80::DefaultFetch, 2, "LD IXl, IXl");
   FillStructOpcode<DD>(0x6E, &Z80::DefaultFetch, 2, "LD L, (IX+%n)");
   FillStructOpcode<DD>(0x6F, &Z80::DefaultFetch, 2, "LD IXh, A");
   FillStructOpcode<DD>(0x70, &Z80::DefaultFetch, 2, "LD (IX+%n), B");
   FillStructOpcode<DD>(0x71, &Z80::DefaultFetch, 2, "LD (IX+%n), C");
   FillStructOpcode<DD>(0x72, &Z80::DefaultFetch, 2, "LD (IX+%n), D");
   FillStructOpcode<DD>(0x73, &Z80::DefaultFetch, 2, "LD (IX+%n), E");
   FillStructOpcode<DD>(0x74, &Z80::DefaultFetch, 2, "LD (IX+%n), H");
   FillStructOpcode<DD>(0x75, &Z80::DefaultFetch, 2, "LD (IX+%n), L");
   FillStructOpcode<DD>(0x77, &Z80::DefaultFetch, 2, "LD (IX+%n), A");
   FillStructOpcode<DD>(0x7C, &Z80::DefaultFetch, 2, "LD A, IXh");
   FillStructOpcode<DD>(0x7D, &Z80::DefaultFetch, 1, "LD A, IXl");
   FillStructOpcode<DD>(0x7E, &Z80::DefaultFetch, 2, "LD A, (IX+%n)");
   FillStructOpcode<DD>(0x84, &Z80::DefaultFetch, 2, "ADD A, IXh");
   FillStructOpcode<DD>(0x85, &Z80::DefaultFetch, 2, "ADD A, IXl");
   FillStructOpcode<DD>(0x86, &Z80::DefaultFetch, 2, "ADD A, (IX+%n)");
   FillStructOpcode<DD>(0x8C, &Z80::DefaultFetch, 2, "ADC A, IXh");
   FillStructOpcode<DD>(0x8D, &Z80::DefaultFetch, 2, "ADC A, IXl");
   FillStructOpcode<DD>(0x8E, &Z80::DefaultFetch, 2, "ADC A, (IX+%n)");
   FillStructOpcode<DD>(0x94, &Z80::DefaultFetch, 2, "SUB A, IXh");
   FillStructOpcode<DD>(0x95, &Z80::DefaultFetch, 2, "SUB A, IXl");
   FillStructOpcode<DD>(0x96, &Z80::DefaultFetch, 2, "SUB A, (IX+%n)");
   FillStructOpcode<DD>(0x9C, &Z80::DefaultFetch, 2, "SBC A, IXh");
   FillStructOpcode<DD>(0x9D, &Z80::DefaultFetch, 2, "SBC A, IXl");
   FillStructOpcode<DD>(0x9E, &Z80::DefaultFetch, 2, "SBC A, (IX+%n)");
   FillStructOpcode<DD>(0xA4, &Z80::DefaultFetch, 2, "AND A, IXh");
   FillStructOpcode<DD>(0xA5, &Z80::DefaultFetch, 2, "AND A, IXl");
   FillStructOpcode<DD>(0xA6, &Z80::DefaultFetch, 2, "AND (IX+%n)");
   FillStructOpcode<DD>(0xAC, &Z80::DefaultFetch, 2, "XOR A, IXh");
   FillStructOpcode<DD>(0xAD, &Z80::DefaultFetch, 2, "XOR A, IXl");
   FillStructOpcode<DD>(0xAE, &Z80::DefaultFetch, 2, "XOR (IX+%n)");
   FillStructOpcode<DD>(0xB4, &Z80::DefaultFetch, 2, "OR A, IXh");
   FillStructOpcode<DD>(0xB5, &Z80::DefaultFetch, 2, "OR A, IXl");
   FillStructOpcode<DD>(0xB6, &Z80::DefaultFetch, 2, "OR (IX+%n)");
   FillStructOpcode<DD>(0xBC, &Z80::DefaultFetch, 2, "CP IXh");
   FillStructOpcode<DD>(0xBD, &Z80::DefaultFetch, 2, "CP (IXl");
   FillStructOpcode<DD>(0xBE, &Z80::DefaultFetch, 2, "CP (IX+%n)");
   FillStructOpcode<DD>(0xCB, &Z80::DefaultFetch, 4, "RES b, (IX+%n)");
   FillStructOpcode<DD>(0xE1, &Z80::DefaultFetch, 1, "POP IX");
   FillStructOpcode<DD>(0xE3, &Z80::DefaultFetch, 1, "EX SP, IX");
   FillStructOpcode<DD>(0xE5, &Z80::DefaultFetch, 1, "PUSH IX");
   FillStructOpcode<DD>(0xE9, &Z80::DefaultFetch, 1, "JP (IX)");
   FillStructOpcode<DD>(0xF9, &Z80::DefaultFetch, 1, "LD SP, IX");

   // FD
   // Act like normal opcode, except that HL is replaced by IY.
   for (j = 0x00; j <= 0xff; j++)
   {
      liste_opcodes_fd_[j] = liste_opcodes_[j];
   }

   FillStructOpcode<FD>(0x09, &Z80::DefaultFetch, 1, "ADD IY, BC");
   FillStructOpcode<FD>(0x19, &Z80::DefaultFetch, 1, "ADD IY, DE");
   FillStructOpcode<FD>(0x21, &Z80::DefaultFetch, 3, "LD IY, %nn__");
   FillStructOpcode<FD>(0x22, &Z80::DefaultFetch, 3, "LD (%nn__), IY");
   FillStructOpcode<FD>(0x23, &Z80::DefaultFetch, 1, "INC IY");
   FillStructOpcode<FD>(0x24, &Z80::DefaultFetch, 1, "INC IYh");
   FillStructOpcode<FD>(0x25, &Z80::DefaultFetch, 1, "DEC IYh");
   FillStructOpcode<FD>(0x26, &Z80::DefaultFetch, 2, "LD IYh, %n");
   FillStructOpcode<FD>(0x29, &Z80::DefaultFetch, 1, "ADD IY, IY");
   FillStructOpcode<FD>(0x2A, &Z80::DefaultFetch, 3, "LD IY, (%nn__)");
   FillStructOpcode<FD>(0x2B, &Z80::DefaultFetch, 1, "DEC IY");
   FillStructOpcode<FD>(0x2C, &Z80::DefaultFetch, 1, "INC IYl");
   FillStructOpcode<FD>(0x2D, &Z80::DefaultFetch, 1, "DEC IYl");
   FillStructOpcode<FD>(0x2E, &Z80::DefaultFetch, 2, "LD IXl, %n");
   FillStructOpcode<FD>(0x34, &Z80::DefaultFetch, 2, "INC (IY+%n)");
   FillStructOpcode<FD>(0x35, &Z80::DefaultFetch, 2, "DEC (IY+%n)");
   FillStructOpcode<FD>(0x36, &Z80::DefaultFetch, 3, "LD (IY+%n), %n");
   FillStructOpcode<FD>(0x39, &Z80::DefaultFetch, 1, "ADD IY, SP");
   FillStructOpcode<FD>(0x44, &Z80::DefaultFetch, 2, "LD B, IYh");
   FillStructOpcode<FD>(0x45, &Z80::DefaultFetch, 2, "LD B, IYl");
   FillStructOpcode<FD>(0x46, &Z80::DefaultFetch, 2, "LD B, (IY+%n)");
   FillStructOpcode<FD>(0x4C, &Z80::DefaultFetch, 2, "LD C, IYh");
   FillStructOpcode<FD>(0x4D, &Z80::DefaultFetch, 2, "LD C, IYl");
   FillStructOpcode<FD>(0x4E, &Z80::DefaultFetch, 2, "LD C, (IY+%n)");
   FillStructOpcode<FD>(0x54, &Z80::DefaultFetch, 2, "LD D, IYh");
   FillStructOpcode<FD>(0x55, &Z80::DefaultFetch, 2, "LD D, IYl");
   FillStructOpcode<FD>(0x56, &Z80::DefaultFetch, 2, "LD D, (IY+%n)");
   FillStructOpcode<FD>(0x5C, &Z80::DefaultFetch, 2, "LD E, IYh");
   FillStructOpcode<FD>(0x5D, &Z80::DefaultFetch, 2, "LD E, IYl");
   FillStructOpcode<FD>(0x5E, &Z80::DefaultFetch, 2, "LD E, (IY+%n)");
   FillStructOpcode<FD>(0x60, &Z80::DefaultFetch, 2, "LD IYh, B");
   FillStructOpcode<FD>(0x61, &Z80::DefaultFetch, 2, "LD IYh, C");
   FillStructOpcode<FD>(0x62, &Z80::DefaultFetch, 2, "LD IYh, D");
   FillStructOpcode<FD>(0x63, &Z80::DefaultFetch, 2, "LD IYh, E");
   FillStructOpcode<FD>(0x64, &Z80::DefaultFetch, 2, "LD IYh, IYh");
   FillStructOpcode<FD>(0x65, &Z80::DefaultFetch, 2, "LD IYh, IYl");
   FillStructOpcode<FD>(0x66, &Z80::DefaultFetch, 2, "LD H, (IY+%n)");
   FillStructOpcode<FD>(0x67, &Z80::DefaultFetch, 2, "LD IYh, A");
   FillStructOpcode<FD>(0x68, &Z80::DefaultFetch, 2, "LD IYh, B");
   FillStructOpcode<FD>(0x69, &Z80::DefaultFetch, 2, "LD IYh, C");
   FillStructOpcode<FD>(0x6A, &Z80::DefaultFetch, 2, "LD IYh, D");
   FillStructOpcode<FD>(0x6B, &Z80::DefaultFetch, 2, "LD IYh, E");
   FillStructOpcode<FD>(0x6C, &Z80::DefaultFetch, 2, "LD IYh, IYh");
   FillStructOpcode<FD>(0x6D, &Z80::DefaultFetch, 2, "LD IYh, IYl");
   FillStructOpcode<FD>(0x6E, &Z80::DefaultFetch, 2, "LD L, (IY+%n)");
   FillStructOpcode<FD>(0x6F, &Z80::DefaultFetch, 2, "LD IYh, A");
   FillStructOpcode<FD>(0x70, &Z80::DefaultFetch, 2, "LD (IY+%n), B");
   FillStructOpcode<FD>(0x71, &Z80::DefaultFetch, 2, "LD (IY+%n), C");
   FillStructOpcode<FD>(0x72, &Z80::DefaultFetch, 2, "LD (IY+%n), D");
   FillStructOpcode<FD>(0x73, &Z80::DefaultFetch, 2, "LD (IY+%n), E");
   FillStructOpcode<FD>(0x74, &Z80::DefaultFetch, 2, "LD (IY+%n), H");
   FillStructOpcode<FD>(0x75, &Z80::DefaultFetch, 2, "LD (IY+%n), L");
   FillStructOpcode<FD>(0x77, &Z80::DefaultFetch, 2, "LD (IY+%n), A");
   FillStructOpcode<FD>(0x7C, &Z80::DefaultFetch, 2, "LD A, IYh");
   FillStructOpcode<FD>(0x7D, &Z80::DefaultFetch, 1, "LD A, IYl");
   FillStructOpcode<FD>(0x7E, &Z80::DefaultFetch, 2, "LD A, (IY+%n)");
   FillStructOpcode<FD>(0x84, &Z80::DefaultFetch, 2, "ADD A, IYh");
   FillStructOpcode<FD>(0x85, &Z80::DefaultFetch, 2, "ADD A, IYl");
   FillStructOpcode<FD>(0x86, &Z80::DefaultFetch, 2, "ADD A, (IY+%n)");
   FillStructOpcode<FD>(0x8C, &Z80::DefaultFetch, 2, "ADC A, IYh");
   FillStructOpcode<FD>(0x8D, &Z80::DefaultFetch, 2, "ADC A, IYl");
   FillStructOpcode<FD>(0x8E, &Z80::DefaultFetch, 2, "ADC A, (IY+%n)");
   FillStructOpcode<FD>(0x94, &Z80::DefaultFetch, 2, "SUB A, IYh");
   FillStructOpcode<FD>(0x95, &Z80::DefaultFetch, 2, "SUB A, IYl");
   FillStructOpcode<FD>(0x96, &Z80::DefaultFetch, 2, "SUB A, (IY+%n)");
   FillStructOpcode<FD>(0x9C, &Z80::DefaultFetch, 2, "SBC A, IYh");
   FillStructOpcode<FD>(0x9D, &Z80::DefaultFetch, 2, "SBC A, IYl");
   FillStructOpcode<FD>(0x9E, &Z80::DefaultFetch, 2, "SBC A, (IY+%n)");
   FillStructOpcode<FD>(0xA4, &Z80::DefaultFetch, 2, "AND A, IYh");
   FillStructOpcode<FD>(0xA5, &Z80::DefaultFetch, 2, "AND A, IYl");
   FillStructOpcode<FD>(0xA6, &Z80::DefaultFetch, 2, "AND (IY+%n)");
   FillStructOpcode<FD>(0xAC, &Z80::DefaultFetch, 2, "XOR A, IYh");
   FillStructOpcode<FD>(0xAD, &Z80::DefaultFetch, 2, "XOR A, IYl");
   FillStructOpcode<FD>(0xAE, &Z80::DefaultFetch, 2, "XOR (IY+%n)");
   FillStructOpcode<FD>(0xB4, &Z80::DefaultFetch, 2, "OR A, IYh");
   FillStructOpcode<FD>(0xB5, &Z80::DefaultFetch, 2, "OR A, IYl");
   FillStructOpcode<FD>(0xB6, &Z80::DefaultFetch, 2, "OR (IY+%n)");
   FillStructOpcode<FD>(0xBC, &Z80::DefaultFetch, 2, "CP IYh");
   FillStructOpcode<FD>(0xBD, &Z80::DefaultFetch, 2, "CP (IYl");
   FillStructOpcode<FD>(0xBE, &Z80::DefaultFetch, 2, "CP (IY+%n)");
   FillStructOpcode<FD>(0xCB, &Z80::DefaultFetch, 3, "RES %b, (IY+%n)");
   FillStructOpcode<FD>(0xE1, &Z80::DefaultFetch, 1, "POP_IY");
   FillStructOpcode<FD>(0xE3, &Z80::DefaultFetch, 1, "EX (SP), IY");
   FillStructOpcode<FD>(0xE5, &Z80::DefaultFetch, 1, "PUSH_IY");
   FillStructOpcode<FD>(0xE9, &Z80::DefaultFetch, 1, "JP (IY)");
   FillStructOpcode<FD>(0xF9, &Z80::DefaultFetch, 1, "LD SP, IY");
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
   NEXT_INSTR
}

unsigned int Z80::Opcode_DAA()
{
   unsigned char btmp;
   int nextcycle;
   btmp = af_.b.h; 
   if (af_.b.l & NF)
   {
      if ((af_.b.l&HF) | ((af_.b.h & 0xf) > 9)) btmp -= 6; 
      if ((af_.b.l&CF) | (af_.b.h > 0x99)) btmp -= 0x60;
   }
   else
   {
      if ((af_.b.l&HF) | ((af_.b.h & 0xf) > 9)) btmp += 6; 
      if ((af_.b.l&CF) | (af_.b.h > 0x99)) btmp += 0x60;
   }
   q_ = (af_.b.l&(CF | NF)) | (af_.b.h > 0x99) | ((af_.b.h^btmp)&HF) | Szp[btmp]; 
   af_.b.l = q_; 
   af_.b.h = btmp; 
   NEXT_INSTR;
}

unsigned int Z80::Opcode_RLCA()
{
   unsigned char btmp;
   int nextcycle;

   q_ = af_.b.l& ~(NF | HF | CF | 0x28);
   btmp = af_.b.h >> 7;
   af_.b.h = (af_.b.h << 1) | btmp;
   q_ |= (af_.b.h & 0x28) | btmp;

   af_.b.l = q_;
   NEXT_INSTR_RES(current_opcode_ & 0xFFFF00);
}

unsigned int Z80::Opcode_RRCA()
{
   unsigned char btmp;
   int nextcycle;

   btmp = af_.b.h & CF;
   q_ = af_.b.l & ~(NF | HF | CF | 0x28);
   q_ |= btmp;
   af_.b.h = (af_.b.h >> 1) + (btmp << 7);
   q_ |= (af_.b.h & 0x28);
   af_.b.l = q_;
   NEXT_INSTR;
}

unsigned int Z80::Opcode_RLA()
{
   unsigned char btmp;
   int nextcycle;
   btmp = af_.b.l&CF; 
   q_ = af_.b.l & ~(NF | HF | CF | 0x28);

   if (af_.b.h & 0x80) q_ |= CF; 
   af_.b.h = af_.b.h << 1; 
   af_.b.h |= btmp; 
   q_ |= (af_.b.h & 0x28); 
   af_.b.l = q_; 
   NEXT_INSTR;
}

unsigned int Z80::Opcode_RRA()
{
   unsigned char btmp;
   int nextcycle;
   q_ = af_.b.l& ~(NF | HF | CF | 0x28); 
   q_ |= (af_.b.h & CF);
   af_.b.h = af_.b.h >> 1;
   af_.b.h |= (af_.b.l&CF) * 0x80; 
   q_ |= (af_.b.h & 0x28); 
   af_.b.l = q_; 
   NEXT_INSTR; 
}

unsigned int Z80::Opcode_CPL()
{
   int nextcycle;
   af_.b.h = ~af_.b.h;
   q_ = af_.b.l & (~0x28); 
   q_ |= (af_.b.h & 0x28) | (NF | HF); 
   af_.b.l = q_; 
   NEXT_INSTR;
}

unsigned int Z80::Opcode_SCF()
{
   int nextcycle;
   q_ = (CF | (((q_^af_.b.l) | af_.b.h) & 0x28)) | (af_.b.l&(PF | SF | ZF)); 
   af_.b.l = q_; 
   NEXT_INSTR
}

unsigned int Z80::Opcode_CCF()
{
   int nextcycle;
   q_ = ((((q_^af_.b.l) | af_.b.h) & 0x28)) | (af_.b.l&(PF | SF | ZF));
   q_ |= (af_.b.l&CF) ? HF : CF; 
   af_.b.l = q_; 
   NEXT_INSTR;
}


unsigned int Z80::Opcode_HALT()
{
   int nextcycle;
   if (sig_->nmi_) { SET_NMI; }
   else {
      if (sig_->int_ && iff1_) { SET_INT; }
      else { --pc_; SET_NOINT; }
   }

   current_opcode_ = 0;
   int ret = t_;
   counter_ += (ret - 1);
   t_ = 1;
   return ret;
}

unsigned int Z80::Opcode_MemoryFromStack()
{
   machine_cycle_ = M_MEMORY_R; 
   t_ = 1; 
   current_address_ = sp_++; 
   current_data_ = 0; 
   read_count_ = 0;
   return 1;
}
