#include "Z80_Full.h"

int Z80::OpcodeIOR()
{
   unsigned char btmp;
   int nextcycle;

   switch (current_opcode_)
   {
   case 0x00DB: af_.b.h = data_; NEXT_INSTR; break;// IN (n), A
   case 0xED40: IN_FLAGS(bc_.b.h); NEXT_INSTR; break; // IN B, (C)
   case 0xED48: IN_FLAGS(bc_.b.l); NEXT_INSTR; break; // IN C, (C)
   case 0xED50: IN_FLAGS(de_.b.h); NEXT_INSTR; break; // IN D, (C)
   case 0xED58: IN_FLAGS(de_.b.l); NEXT_INSTR; break; // IN E, (C)
   case 0xED60: IN_FLAGS(hl_.b.h); NEXT_INSTR; break; // IN H, (C)
   case 0xED68: IN_FLAGS(hl_.b.l); NEXT_INSTR; break; // IN L, (C)
   case 0xED70: IN_FLAGS(btmp); NEXT_INSTR; break; // IN (C)
   //case 0xED78: IN_FLAGS(A); mem_ptr_.w = bc_.w + 1; NEXT_INSTR; break; // IN A, (C)
   case 0xED78:
      af_.b.h = current_data_ & 0xFF;
      q_ = af_.b.l&CF;
      ZERO_FLAG(af_.b.h);
      SIGN_FLAG(af_.b.h);
      //q_ |= (A & 0x80) ? NF : 0;
      PARITY_FLAG(af_.b.h);
      q_ |= (af_.b.h & 0x28);/*TODO : ADD HERE XY FLAGS !*/
      af_.b.l = q_;
      mem_ptr_.w = bc_.w + 1; NEXT_INSTR; break; // IN A, (C)

   case 0xEDA2: machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = hl_.w; ; read_count_ = 0; break; // INI
   case 0xEDAA: machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = hl_.w; ; read_count_ = 0; break; // IND
   case 0xEDB2: machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = hl_.w; ; read_count_ = 0; break; // INIR
   case 0xEDBA: machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = hl_.w; ; read_count_ = 0; break; // INDR

   case 0xFF02: mem_ptr_.w = ((ir_.b.h << 8) | (data_)); machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = --sp_; current_data_ = pc_ >> 8; ; read_count_ = 0; break; // Int MODE2

   default:
#ifdef _WIN32
      _CrtDbgBreak();
#endif
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