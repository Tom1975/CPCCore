#include "stdafx.h"

#include "Z80_Full.h"
#include "Sig.h"
#include "Memoire.h"


Z80::Z80(void) : 
   stop_on_fetch_(false), 
   rw_opcode_(false),
   log_(nullptr), 
   count_(0)
{

   InitOpcodeShortcuts();
   int i, p;
   for (i = 0; i < 256; i++)
   {
      p = 0;
      if (i & 0x01) ++p;
      if (i & 0x02) ++p;
      if (i & 0x04) ++p;
      if (i & 0x08) ++p;
      if (i & 0x10) ++p;
      if (i & 0x20) ++p;
      if (i & 0x40) ++p;
      if (i & 0x80) ++p;
      Sz[i] = i ? i & SF : ZF;
      Sz[i] |= (i & (YF | XF));       /* undocumented flag bits 5+3 */
      SzBit[i] = i ? i & SF : ZF | PF;
      SzBit[i] |= (i & (YF | XF));   /* undocumented flag bits 5+3 */
      Szp[i] = Sz[i] | ((p & 1) ? 0 : PF);
      P[i] = ((p & 1) ? 0 : PF);
      SzhvInc[i] = Sz[i];
      if (i == 0x80) SzhvInc[i] |= VF;
      if ((i & 0x0f) == 0x00) SzhvInc[i] |= HF;
      SzhvDec[i] = Sz[i] | NF;
      if (i == 0x7f) SzhvDec[i] |= VF;
      if ((i & 0x0f) == 0x0f) SzhvDec[i] |= HF;

      FlagAnd[i] = (((i & 0xff) == 0) ? ZF : 0) | HF | (i & 0x80) | ((p & 1) ? 0 : PF) | (i & 0x28);
      FlagOr[i] = (((i & 0xff) == 0) ? ZF : 0) | (i & 0x80) | ((p & 1) ? 0 : PF) | (i & 0x28);
      FlagXor[i] = (((i & 0xff) == 0) ? ZF : 0) | (i & 0x80) | ((p & 1) ? 0 : PF) | (i & 0x28);
   }

   bc_.w = 0;
   de_.w = 0;
   hl_.w = 0;
   af_p_.w = 0;
   bc_p_.w = 0;
   de_p_.w = 0;
   hl_p_.w = 0;
   iy_.w = 0;
   ix_.w = 0;
   ir_.w = 0;
   q_ = 0;

   sp_ = 0xFFFF;

   carry_set_ = false;

   //InitOpcodes ();
   Reset();
}


Z80::~Z80(void)
{
}

void Z80::ReinitProc()
{

   t_ = 1;
   mem_ptr_.w = 0;
   q_ = 0;
   machine_cycle_ = Z80::M_FETCH;
   counter_ = 2;
   SYNC_Z80

}

void Z80::Reset()
{

   rw_opcode_ = false;

   machine_cycle_ = M_FETCH;
   pc_ = 0x0000;

   // interrupt_mode_ = 0; - todo !
   iff1_ = false;
   iff2_ = false;

   //m_bInterrupt = false;

   //iy_.w = 0;
   //ix_.w = 0;

   af_.w = 0xFFFF;
   mem_ptr_.w = 0;
   //ir_.w = 0;

   t_ = 1;

   interrupt_mode_ = 0;

   counter_ = 2;
   current_opcode_ = 0;
}


#define WAIT_TEST if ((counter_&0x3) == 0)

void Z80::InterruptInit()
{
   INC_R;
   if (carry_set_)
   {
      af_.b.l &= ~(PF);
   }

   q_ = 0;
   iff1_ = false;
   iff2_ = false;

   sig_->AcqInt();

   // Machine cycle : M1 interrupted
   //machine_cycle_ = M_M1_INT;
   //t_ = 1;
   //m_bInterrupt = false;
}

unsigned short Z80::GetPC()
{
   //
   if (machine_cycle_ == M_M1_INT || machine_cycle_ == M_M1_NMI)
   {
      return pc_;
   }
   else
   {
      return pc_ - 1;
   }
}

void Z80::PreciseTick()
{
}

unsigned int Z80::Tick()
{
   int nextcycle;
   ++counter_;

   switch (machine_cycle_ | t_)
   {
   case M_M1_NMI + 1:
   {
      INC_R;

      q_ = 0;
      iff2_ = iff1_;
      iff1_ = 0;
      sig_->nmi_ = false;
      sig_->M1();
      ++t_; return 1;
   }
   case M_M1_NMI + 2:
   case M_M1_NMI + 3:
   case M_M1_NMI + 4:
      ++t_; break;
   case M_M1_NMI + 5:
   {
      machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = --sp_; current_data_ = pc_ >> 8; read_count_ = 0;
      current_opcode_ = 0xFF12; // NMI
      break;
   }
   /////////////////////////////////////////////////
   // M1 Interrupt
   case M_M1_INT + 1:
   {
      switch (interrupt_mode_)
      {
      case 0:
      {
         InterruptInit();
         nextcycle = 4 - ((counter_ + 3) & 0x3);
         t_ = 6;
         counter_ += nextcycle + 3;
         return nextcycle + 4;
      }
      case 1:
      {
         InterruptInit();

         nextcycle = 4 - ((counter_ + 2) & 0x3);
         t_ = 7;
         counter_ += nextcycle + 4;
         return nextcycle + 5;

         //++t_;
      }
      case 2:
      {
         InterruptInit();

         /*int nextcycle = 4 - ((counter_ + 2) & 0x3);
         t_ = 7;
         counter_ += nextcycle + 4;
         return nextcycle + 5;*/
         t_++;
         return 1;
      }
      }
   }
   case M_M1_INT + 2:
   case M_M1_INT + 3:
      ++t_; return 1;
   case M_M1_INT + 4:
   {
      WAIT_TEST
         t_++;
      return 1;
   }

   case M_M1_INT + 5:
   case M_M1_INT + 6:
   {
      if (interrupt_mode_ == 2)
      {
         ++t_; return 1;
      }
      else if (interrupt_mode_ == 0)
      {
         // Read from databus
         sig_->In(&data_, (address_ >> 8) & 0xFF, (address_) & 0xFF, true);
         t_++;
         return 1;
      }
   }

   case M_M1_INT + 7:
   {
      if (interrupt_mode_ == 0)
      {
         //INC_R
         machine_cycle_ = M_FETCH; t_ = 4; current_opcode_ = data_;
         return 1;
      }
      else if (interrupt_mode_ == 1)
      {
         machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = --sp_; current_data_ = pc_ >> 8; read_count_ = 0;
         current_opcode_ = 0xFF; // Opcode is RST &38
         return 1;
      }
      else if (interrupt_mode_ == 2)
      {
         // Depends :
         // On OLD = 0xFF
         // On PLUS : If ASIC is not enabled = 0
         // Otherwise, interrupt_io_data_

         // The Vectored Interrupt Bug

         // This bug occurs when :
         // PREVENT SHMUP TO WORK !!!
         if (((pc_ & 0x2000) == 0) && rw_opcode_)
         {
            // A13 of PC = 0
            // last opcode perform a Read/Write memory
            // DMA interrupt are not used
            if (((sig_->interrupt_io_data_ & 0x6) == 0x6)
               || (sig_->interrupt_io_data_ == 0))
            {
               sig_->interrupt_io_data_ &= ~0x06;
               sig_->interrupt_io_data_ |= 0x04;
            }

            // Happens for Raster interrupt, or DMA with autoclear enabled
         }

         // End of vectorized Interrupt Bug
         current_data_ = data_ = sig_->interrupt_io_data_;

         machine_cycle_ = M_IO_R; t_ = 5;
         current_opcode_ = 0xFF02; // Interrupt Mode 2
         return 5;

      }
   }

   /////////////////////////////////////////////////
   // FETCH
   case M_FETCH + 1:
   {
      INC_R

         // Set PC to address bus
         address_ = pc_++;

      data_ = memory_->Get(address_);

      // Compute to avoid waiting cycles :
      // t_++;
      current_opcode_ <<= 8;
      current_opcode_ |= data_;
      nextcycle = 4 - (counter_ & 0x3);

      t_ = 4;
      counter_ += nextcycle + 1;
      return nextcycle + 2;
   }
   case M_FETCH + 2:
      WAIT_TEST
         t_++;
      return 1;
   case M_FETCH + 3:
      t_++;
      return 1;
   case M_FETCH + 4:
   {
      //if (current_opcode_ != 0x37 && current_opcode_ != 0x3F)
      if ((current_opcode_ & 0xF7) != 0x37)
         q_ = 0;
   }
   case M_FETCH + 5:
   case M_FETCH + 6:
   case M_FETCH + 7:
   case M_FETCH + 8:
   case M_FETCH + 9:
   case M_FETCH + 10:
   case M_FETCH + 11:
   case M_FETCH + 12:
   {
      return (this->*(*current_function_)[current_opcode_ & 0xFF])();
   }
   /////////////////////////////////////////////////
   // MEMORY IO
   case M_MEMORY_R + 1:
   {
      rw_opcode_ = true;
      address_ = current_address_;
      data_ = memory_->Get(address_);

      nextcycle = 3 - (counter_ & 0x3);

      t_ = 3;
      counter_ += nextcycle + 1;
      return nextcycle + 2;
   }
   case M_MEMORY_R + 2:
   {
      WAIT_TEST
         t_++;
      return 1;
   }

   case M_MEMORY_R + 3:
      current_data_ |= (data_ << (read_count_ * 8));
   case M_MEMORY_R + 4:
   case M_MEMORY_R + 5:
   case M_MEMORY_R + 6:
   case M_MEMORY_R + 7:
   case M_MEMORY_R + 8:
   {
      //return OpcodeMEMR();
      if (current_opcode_ < 0x100)
         return (this->*(memr_func_)[current_opcode_ & 0xFF])();
      else
         return OpcodeMEMR();
   }

   case M_MEMORY_W + 1:
   {
      rw_opcode_ = true;
      address_ = current_address_;
      data_ = current_data_ & 0xFF;// >> (read_count_ * 8);

      nextcycle = 3 - (counter_ & 0x3);

      t_ = 3;
      counter_ += nextcycle + 1;
      return nextcycle + 2;
   }
   case M_MEMORY_W + 2:
   {
      // Sample WAIT
      WAIT_TEST
      {
         t_++;
      }
      break;
   }

   case M_MEMORY_W + 3:
      memory_->Set(address_, data_);
      // WRITE the data
   case M_MEMORY_W + 4:
   case M_MEMORY_W + 5:
   case M_MEMORY_W + 6:
   case M_MEMORY_W + 7:
   case M_MEMORY_W + 8:
      // Execute next of the instruction
      return OpcodeMEMW();

      /////////////////////////////////////////////////
   // I/O
   case M_IO_R + 1:
      address_ = current_address_;
      t_++;
      return 1;

   case M_IO_R + 2:
   {
      sig_->In(&data_, (address_ >> 8) & 0xFF, (address_) & 0xFF);
      nextcycle = 3 - (counter_ & 0x3);
      t_ = 4;
      counter_ += nextcycle + 1;
      return nextcycle + 2;

   }
   case M_IO_R + 4:
      current_data_ = data_;
   case M_IO_R + 5:
   case M_IO_R + 6:
   case M_IO_R + 7:
   case M_IO_R + 8:
      return OpcodeIOR();

   case M_IO_W + 1:
   {
      address_ = current_address_;
      data_ = current_data_ & 0xFF;
      t_++;
      break;
   case M_IO_W + 2:
   {
      sig_->Out(address_, data_);
      nextcycle = 3 - (counter_ & 0x3);

      t_ = 4;
      counter_ += nextcycle + 1;
      return nextcycle + 2;

   }
   case M_IO_W + 4:
   case M_IO_W + 5:
   case M_IO_W + 6:
   case M_IO_W + 7:
   case M_IO_W + 8:
      return OpcodeIOW();
   }
   case M_Z80_WORK:
   case M_Z80_WORK + 1:
      return OpcodeWAIT();
   case M_Z80_WORK + 2:
   case M_Z80_WORK + 3:
   case M_Z80_WORK + 4:
   case M_Z80_WORK + 5:
      //if (t_>=4) if (sig_->ctrl_int != 1) rw_opcode_ = false;
      --t_;
      return 1;
   case M_Z80_WAIT:
   case M_Z80_WAIT + 1:
      NEXT_INSTR;
      break;
   default:
      --t_;
      return 1;
   }
   return 1;
}

unsigned int Z80::DefaultFetch()
{
   int nextcycle;
   NEXT_INSTR;
}

bool Z80::IsCallInstruction(unsigned short Addr_P)
{
   unsigned char nextInstr_L = memory_->Get(Addr_P);
   return  (nextInstr_L == 0xc4
      || nextInstr_L == 0xcc
      || nextInstr_L == 0xcd
      || nextInstr_L == 0xd4
      || nextInstr_L == 0xdc
      || nextInstr_L == 0xe4
      || nextInstr_L == 0xec
      || nextInstr_L == 0xf4
      || nextInstr_L == 0xfc
      );

}


unsigned char Z80::GetOpcodeSize(unsigned short Addr_P)
{
   unsigned char nextInstr_L = memory_->Get(Addr_P);
   unsigned char size = liste_opcodes_[nextInstr_L].size;
   if (strcmp(liste_opcodes_[nextInstr_L].disassembly, "%CB") == 0)
   {
      size += liste_opcodes_cb_[nextInstr_L].size;
   }
   else if (strcmp(liste_opcodes_[nextInstr_L].disassembly, "%ED") == 0)
   {
      // ED ?
      size += liste_opcodes_ed_[nextInstr_L].size;
   }
   else if (strcmp(liste_opcodes_[nextInstr_L].disassembly, "%DD") == 0)
   {
      // DD
      size += liste_opcodes_dd_[nextInstr_L].size;
   }
   else if (strcmp(liste_opcodes_[nextInstr_L].disassembly, "%FD") == 0)
   {
      // FD
      size += liste_opcodes_fd_[nextInstr_L].size;
   }
   return size;
}


void Z80::TraceTape(unsigned short pc, unsigned char value)
{
   if (log_ != nullptr
      && (
         pc == 0x28BE  // CAS READ
         || pc == 0x2B3D  // ?
         || pc == 0xBB5A  // ?
         || pc == 0x2B20  // ?
         || pc == 0x28BC  // Enterprise
         || pc == 0xBDF5  // Enterprise
         || pc == 0xBC2A  // Marmelade
         || pc == 0x9D7F
         || pc == 0x18B2
         || pc == 0x91CA
         || pc == 0x0277
         || pc == 0x8DC4
         || pc == 0x18B2
         || pc == 0x91CA
         || pc == 0x8DC4
         || pc == 0xA52A
         || pc == 0x2A2C
         || pc == 0x465
         || pc == 0x9D0C
         || pc == 0xAe7
         || pc == 0x0278
         || pc == 0x041D    // Gauntlet P3
         || pc == 0x9D0F
         || pc == 0xD27B
         )
      ) // Gauntlet P3)
   {
      // Write to log
      char c[4] = { 0 };

      sprintf(c, "%2.2X ", value);

      log_->WriteLog(c);
      OutputDebugString(c);
      if (++count_ == 16)
      {
         log_->EndOfLine();
         count_ = 0;
      }
   }

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

unsigned int Z80::Opcode_DefaultToSimple()
{
   current_function_ = &fetch_func;
   current_opcode_ &= 0xFF;
   return (this->*(fetch_func)[current_opcode_])();
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

unsigned int Z80::Opcode_Push_delayed()
{
   if (t_ == 5) {
      machine_cycle_ = M_MEMORY_W;
      t_ = 1;
      current_address_ = --sp_;
      current_data_ = pc_ >> 8;
      read_count_ = 0;
      return 1;
   }
   else
   {
      ++t_;
   }
   return 1;
}

unsigned int Z80::Opcode_Call_fetch()
{
   machine_cycle_ = M_MEMORY_W;
   t_ = 1;
   current_address_ = --sp_;
   current_data_ = (pc_ + 2) >> 8;
   read_count_ = 0;
   return 1;
}

unsigned int Z80::Opcode_Exx()
{
   int nextcycle;
   unsigned short t;
   t = bc_.w;
   bc_.w = bc_p_.w;
   bc_p_.w = t;
   t = de_.w;
   de_.w = de_p_.w;
   de_p_.w = t;
   t = hl_.w;
   hl_.w = hl_p_.w;
   hl_p_.w = t;
   NEXT_INSTR
}

unsigned int Z80::Opcode_DI()
{
   int nextcycle;
   iff1_ = false;
   iff2_ = false;
   NEXT_INSTR
}

unsigned int Z80::Opcode_EI()
{
   int nextcycle;
   iff1_ = true;
   iff2_ = true;
   NEXT_INSTR_EI
}

unsigned int Z80::Opcode_NEG()
{
   int nextcycle;
   unsigned int res;
   res = 0 - af_.b.h;
   q_ = NF | (((res & 0xff) == 0) ? ZF : 0) | (res & 0x80) | ((af_.b.h != 0) ? CF : 0) | ((af_.b.h == 0x80) ? PF : 0) | ((af_.b.h^res)&HF);
   q_ |= (res & 0x28);
   af_.b.l = q_;
   af_.b.h = res;
   NEXT_INSTR;
}

unsigned int Z80::MEMR_DJNZ()
{
   ++pc_;
   --bc_.b.h;
   if (bc_.b.h != 0) 
   {
      pc_ += (char)data_;
      mem_ptr_.w = pc_;
      machine_cycle_ = M_Z80_WAIT;
      counter_ += (5 - 1);
      t_ = 1;
      return 5;
   }
   else 
   { 
      int nextcycle;
      NEXT_INSTR;
   }
}

unsigned int Z80::MEMR_JR()
{
   ++pc_; 
   pc_ += (char)(current_data_ & 0xFF); 
   mem_ptr_.w = pc_; 
   machine_cycle_ = M_Z80_WAIT; 
   counter_ += (4);
   t_ = 1;
   return 5;
}