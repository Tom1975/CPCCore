#include "Z80_Full.h"


int Z80::OpcodeIOW()
{
   unsigned short utmp;
   int nextcycle;

   switch (current_opcode_)
   {
   case 0x00D3:   // OUT A, (n)
   case 0xED41:   // OUT (C), B
   case 0xED49:   // OUT (C), C
   case 0xED51:   // OUT (C), D
   case 0xED59:   // OUT (C), E
   case 0xED61:   // OUT (C), H
   case 0xED69:   // OUT (C), L
   case 0xED79:   // OUT (C), A
   case 0xED71:   // OUT (C), 0
      // Nothing to do
      NEXT_INSTR
         break;
   case 0xEDA3: {++hl_.w; q_ = af_.b.l; utmp = hl_.b.l + data_; if (utmp > 255) q_ |= (CF | HF); utmp &= 7; utmp ^= bc_.b.h; PARITY_FLAG(utmp);
      if (data_ & 0x80) q_ |= NF; else q_ &= ~NF; if (bc_.b.h == 0) q_ |= ZF; else q_ &= ~(ZF);
      af_.b.l = q_; NEXT_INSTR;  break; }// OUTI
   case 0xEDAB: {--hl_.w; q_ = af_.b.l; utmp = hl_.b.l + data_; if (utmp > 255) q_ |= (CF | HF); utmp &= 7; utmp ^= bc_.b.h; PARITY_FLAG(utmp);
      if (data_ & 0x80) q_ |= NF; else q_ &= ~NF; if (bc_.b.h == 0) q_ |= ZF; else q_ &= ~(ZF);
      af_.b.l = q_; NEXT_INSTR;  break; }// OUTD
   case 0xEDB3: {++hl_.w; q_ = af_.b.l; utmp = hl_.b.l + data_; if (utmp > 255) q_ |= (CF | HF); utmp &= 7; utmp ^= bc_.b.h; PARITY_FLAG(utmp);
      if (data_ & 0x80) q_ |= NF; else q_ &= ~NF; if (bc_.b.h == 0) {
         q_ |= ZF; af_.b.l = q_; NEXT_INSTR;
      }
      else {
         q_ &= ~(ZF); af_.b.l = q_; pc_ -= 2; machine_cycle_ = M_Z80_WORK; t_ = 5;
      }
   }
                break;// OUTI
   case 0xEDBB: {--hl_.w; q_ = af_.b.l; utmp = hl_.b.l + data_; if (utmp > 255) q_ |= (CF | HF); utmp &= 7; utmp ^= bc_.b.h; PARITY_FLAG(utmp);
      if (data_ & 0x80) q_ |= NF; else q_ &= ~NF; if (bc_.b.h == 0) {
         q_ |= ZF; af_.b.l = q_; NEXT_INSTR;
      }
      else {
         q_ &= ~(ZF); af_.b.l = q_; pc_ -= 2; machine_cycle_ = M_Z80_WORK; t_ = 5;
      }
   }
                break;// OUTI
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
