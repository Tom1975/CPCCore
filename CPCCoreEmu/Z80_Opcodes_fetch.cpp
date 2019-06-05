#include "stdafx.h"

#include "Z80_Full.h"
#include "Sig.h"
#include "Memoire.h"

// Create and add opcode to correct array


unsigned int Z80::Opcode_NOP()
{
   unsigned int res;
   unsigned char btmp;
   int nextcycle;

Begin:
   switch (current_opcode_) {
   case 0x00: NEXT_INSTR; break;// NOP
//////////////////////////////////////////////////////////
// CB
/// RLC
   case 0xCB00: RLC(bc_.b.h); NEXT_INSTR; break; // RLC B
   default:
      if (((current_opcode_ & 0xFF00) == 0xFD00)
         || ((current_opcode_ & 0xFF00) == 0xDD00))
      {
         current_opcode_ &= 0xFF;
         goto Begin;
      }
      else
      {
         if ((current_opcode_ & 0xFF00) != 0xED00)
         {
            // Assert !
         }
         /*#ifdef _WIN32
                  _CrtDbgBreak();
         #endif*/
      }

      NEXT_INSTR;
      break;
   }
   if (machine_cycle_ == M_Z80_WORK)
   {
      int ret = t_;
      counter_ += (ret - 1);
      t_ = 1;
      return ret;
   }
   return 1;
}
