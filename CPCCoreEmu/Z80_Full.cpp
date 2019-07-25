#include "stdafx.h"

#include "Z80_Full.h"
#include "Sig.h"
#include "Memoire.h"

static unsigned char GetBitOpcode(unsigned char b, const char* r)
{
   unsigned char op = 0;
   switch (*r)
   {
   case 'B': op = 0; break;
   case 'C': op = 1; break;
   case 'D': op = 2; break;
   case 'E': op = 3; break;
   case 'L': op = 5; break;
   case 'A': op = 7; break;
   case 'H':
   {
      if (r[1] == 0)
      {
         op = 4;
      }
      else
      {
         op = 6;// HL !
      }
      break;
   };
   }
   op += (b << 3);
   return op;
}


#define DEF_OP_BIT_CPLT(o,b,r)\
{char *Buffer_Tmp = new char[64];sprintf(Buffer_Tmp, "BIT %i, %s", b,  #r);   \
   liste_opcodes_cb_[o] = FillStructOpcode(nullptr,  1, Buffer_Tmp);delete []Buffer_Tmp;\
   }

#define DEF_OP_BIT(r)\
   {\
      unsigned opcode = GetBitOpcode (0, #r);DEF_OP_BIT_CPLT( opcode|0x40, 0, r);\
      opcode = GetBitOpcode (1, #r);DEF_OP_BIT_CPLT( opcode|0x40, 1, r);\
      opcode = GetBitOpcode (2, #r);DEF_OP_BIT_CPLT( opcode|0x40, 2, r);\
      opcode = GetBitOpcode (3, #r);DEF_OP_BIT_CPLT( opcode|0x40, 3, r);\
      opcode = GetBitOpcode (4, #r);DEF_OP_BIT_CPLT( opcode|0x40, 4, r);\
      opcode = GetBitOpcode (5, #r);DEF_OP_BIT_CPLT( opcode|0x40, 5, r);\
      opcode = GetBitOpcode (6, #r);DEF_OP_BIT_CPLT( opcode|0x40, 6, r);\
      opcode = GetBitOpcode (7, #r);DEF_OP_BIT_CPLT( opcode|0x40, 7, r);\
   }


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

IZ80* Z80::CopyMe()
{
   IZ80* new_z80 = new Z80();
   *new_z80 = *this;

   return new_z80;
}

void Z80::DeleteCopy(IZ80* z80)
{
   delete z80;
}

bool Z80::CompareToCopy(IZ80* z80)
{
   if (IZ80::CompareToCopy(z80) == false) return false;

   Z80* other = (Z80*)z80;

   if (machine_cycle_ != other->machine_cycle_) return false;
   if (counter_ != other->counter_) return false;
   if (carry_set_ != other->carry_set_) return false;
   if (current_opcode_ != other->current_opcode_) return false;

   return true;
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
   memset(&system_ctrl_, 0, sizeof(system_ctrl_));

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
   cpu_ctrl_.halt = 0;
   //m_CurrentListOpcodes = ListeOpcodes;

   interrupt_mode_ = 0;

   counter_ = 2;
   current_opcode_ = 0;
}


//#define WAIT_TEST if ( !cpu_ctrl_.WAIT)
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
   // TODO : This should be handled by the GA
   //counter_ = (++counter_) & 0x3;
   //cpu_ctrl_.WAIT = counter_ != 0;
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
      if (current_opcode_ != 0x37 && current_opcode_ != 0x3F)
         //if ((current_opcode_ & 0xF7) != 0xF7)
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
      int val = (this->*(*current_function_)[current_opcode_ & 0xFF])();
      if (pc_ == 0x61D || pc_ == 0x628)
      {
      }
      return val;
   }
   /////////////////////////////////////////////////
   // MEMORY IO
   case M_MEMORY_R + 1:
   {
      rw_opcode_ = true;
      address_ = current_address_;
      //system_ctrl_.MREQ = 1;
      //system_ctrl_.RD = 1;
      //t_++;
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
      //#include "Z80_Opcodes_memr.h"
      return OpcodeMEMR();
   }

   case M_MEMORY_W + 1:
   {
      rw_opcode_ = true;
      address_ = current_address_;
      data_ = current_data_ & 0xFF;// >> (read_count_ * 8);
      //system_ctrl_.MREQ = 1;

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
      //system_ctrl_.WR = 1;
      }
      break;
   }

   case M_MEMORY_W + 3:
      memory_->Set(address_, data_);
      // WRITE the data
      //data_ = current_data_;
      //system_ctrl_.MREQ = 0;
      //system_ctrl_.WR = 0;
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
      //
      //system_ctrl_.RD = 1;
      //system_ctrl_.IORQ = 1;
      sig_->In(&data_, (address_ >> 8) & 0xFF, (address_) & 0xFF);
      //t_++;
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

int Z80::OpcodeIOR()
{
   unsigned char btmp;
   int nextcycle;

#include "Z80_Opcodes_ior.h"
   if (machine_cycle_ == M_Z80_WORK)
   {
      int ret = t_;
      counter_ += (ret - 1);
      t_ = 1;
      return ret;
   }
   return 1;
}

int Z80::OpcodeIOW()
{
   unsigned short utmp;
   int nextcycle;

#include "Z80_Opcodes_iow.h"
   if (machine_cycle_ == M_Z80_WORK)
   {
      int ret = t_;
      counter_ += (ret - 1);
      t_ = 1;
      return ret;
   }
   return 1;
}

int Z80::OpcodeMEMR()
{
   unsigned int res; unsigned char btmp;
   int nextcycle;

#include "Z80_Opcodes_memr.h"
   if (machine_cycle_ == M_Z80_WORK)
   {
      int ret = t_;
      counter_ += (ret - 1);
      t_ = 1;
      return ret;
   }
   return 1;
}

int Z80::OpcodeMEMW()
{
   unsigned char btmp; unsigned short utmp;
   int nextcycle;

#include "Z80_Opcodes_memw.h"
   if (machine_cycle_ == M_Z80_WORK)
   {
      int ret = t_;
      counter_ += (ret - 1);
      t_ = 1;
      return ret;
   }
   return 1;
}

int Z80::OpcodeWAIT()
{
   unsigned char btmp;
   int nextcycle;

#include "Z80_Opcodes_z80wait.h"
   if (machine_cycle_ == M_Z80_WORK)
   {
      int ret = t_;
      counter_ += (ret - 1);
      t_ = 1;
      return ret;
   }
   return 1;
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
