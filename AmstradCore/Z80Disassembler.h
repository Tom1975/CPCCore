#pragma once

#include <string>
#include <vector>

#include "IComponent.h"
#include "BusLine.h"

class Z80Disassembler : public IComponent
{
public:
   Z80Disassembler();
   virtual ~Z80Disassembler();

   unsigned char GetOpcodeSize(unsigned char* buffer);
   const int DasmMnemonic(unsigned short Addr, unsigned char* buffer, char pMnemonic[16], char pArgument[16]) const;

protected:
   typedef enum
   {
      R_0,
      R_A,
      R_F,
      R_B,
      R_C,
      R_D,
      R_E,
      R_H,
      R_L,
      R_IXh,
      R_IXl,
      R_IYh,
      R_IYl,
      R_I,
      R_R
   } Registers;

   const char* REG_TO_STR(Registers reg)
   {
      return ((reg == R_A) ? "A" : (reg == R_F) ? "F" : (reg == R_B) ? "B" : (reg == R_C) ? "C" : (reg == R_D) ? "D" : (reg == R_E) ? "E" : (reg == R_H) ? "H" : "L");
   }

   /////////////////////////////////////
   // Disassembly
   typedef struct
   {
      unsigned char size;
      std::string disassembly;
   }OpcodeDisassembly;

   OpcodeDisassembly FillStructOpcode(unsigned char Size, const char* disassembly)
   {
      OpcodeDisassembly op;
      op.size = Size;
      op.disassembly =  disassembly;
      return op;

   }

   OpcodeDisassembly liste_opcodes_[256];
   OpcodeDisassembly liste_opcodes_cb_[256];
   OpcodeDisassembly liste_opcodes_ed_[256];
   OpcodeDisassembly liste_opcodes_dd_[256];
   OpcodeDisassembly liste_opcodes_fd_[256];
};

