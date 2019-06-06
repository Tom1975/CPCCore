#include "stdafx.h"

#include "Z80_Full.h"
#include "Sig.h"
#include "Memoire.h"


unsigned int Z80::Opcode_NOP()
{
   unsigned char btmp;
   int nextcycle;

BeginNop:
   switch (current_opcode_) {
   case 0x00: NEXT_INSTR; break;// NOP
//////////////////////////////////////////////////////////
// CB
/// RLC
   case 0xCB00: 
      btmp = bc_.b.h >> 7; 
      bc_.b.h = (bc_.b.h << 1) + btmp;
      q_ = btmp | ((((bc_.b.h & 0xff) == 0) ? ZF : 0) | (bc_.b.h & 0x80)); 
      q_ |= (bc_.b.h & 0x28); 
      PARITY_FLAG(bc_.b.h); 
      af_.b.l = q_;
      
      NEXT_INSTR; break; // RLC B
   default:
      if (((current_opcode_ & 0xFF00) == 0xFD00)
         || ((current_opcode_ & 0xFF00) == 0xDD00))
      {
         current_opcode_ &= 0xFF;
         goto BeginNop;
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
