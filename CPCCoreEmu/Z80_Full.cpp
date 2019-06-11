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

   for ( i = 0; i < 0x100; i++)
   {
      fetch_func[i] = &Z80::DefaultFetch;
      fetch_func_CB_[i] = &Z80::DefaultFetch;
      fetch_func_ED_[i] = &Z80::DefaultFetch;
      fetch_func_DD_[i] = &Z80::DefaultFetch;
      fetch_func_FD_[i] = &Z80::DefaultFetch;
   }
   fetch_func[0] = &Z80::Opcode_NOP;
   fetch_func[0xCB] = &Z80::Opcode_CB;
   fetch_func[0xFD] = &Z80::Opcode_FD;

   fetch_func[0x01] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0x06] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0x0E] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0x11] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0x16] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0x18] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0x1E] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0x20] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0x20] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0x21] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0x22] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0x26] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0x28] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0x2A] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0x2E] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0x30] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0x31] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0x32] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0x36] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0x38] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0x3A] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0x3E] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0xC2] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0xC3] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0xC4] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0xCA] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0xCC] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0xD2] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0xD3] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0xD4] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0xD6] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0xDA] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0xDB] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0xDC] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0xDD] = &Z80::Opcode_DD;
   fetch_func[0xDE] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0xE2] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0xE4] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0xE6] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0xEA] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0xEC] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0xED] = &Z80::Opcode_ED;
   fetch_func[0xEE] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0xF2] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0xF4] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0xFA] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0xFC] = &Z80::Opcode_Memory_Read_PC;
   fetch_func[0xFE] = &Z80::Opcode_Memory_Read_PC;

   current_function_ = &fetch_func;

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
      //#include "Z80_Opcodes_fetch.h"
      int val = OpcodeFetch();
      //int val = (this->*(*current_function_)[current_opcode_ & 0xFF])();
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


int Z80::OpcodeFetch()
{
   return (this->*(*current_function_)[current_opcode_ & 0xFF])();
}

unsigned int Z80::DefaultFetch()
{
   unsigned int res;
   unsigned char btmp;
   int nextcycle;

#include "Z80_Opcodes_fetch.h"
   if (machine_cycle_ == M_Z80_WORK)
   {
      int ret = t_;
      counter_ += (ret - 1);
      t_ = 1;
      return ret;
   }
   return 1;
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


void Z80::InitOpcodeShortcuts()
{
   // Par defaut, tout le monde pointe sur NOP
   for (unsigned int i = 0; i < 256; i++)
   {
      liste_opcodes_[i] = FillStructOpcode(nullptr, 1, "UNKNOWN");
      liste_opcodes_cb_[i] = FillStructOpcode(nullptr, 1, "UNKNOWN");
      liste_opcodes_ed_[i] = FillStructOpcode(nullptr, 1, "UNKNOWN");
      liste_opcodes_dd_[i] = FillStructOpcode(nullptr, 1, "UNKNOWN");
      liste_opcodes_fd_[i] = FillStructOpcode(nullptr, 1, "UNKNOWN");
   }

   // Opcodes standards
   ///////////////////////////////////////////////////////////////////////////////////////
   /////////////                           FUNCTION       SIZE   DISASSMBLY
   liste_opcodes_[0x00] = FillStructOpcode(nullptr, 1, "NOP");
   liste_opcodes_[0x01] = FillStructOpcode(nullptr, 3, "LD BC, %nn__");
   liste_opcodes_[0x02] = FillStructOpcode(nullptr, 1, "LD (BC), A");
   liste_opcodes_[0x03] = FillStructOpcode(nullptr, 1, "INC BC");
   liste_opcodes_[0x04] = FillStructOpcode(nullptr, 1, "INC B");
   liste_opcodes_[0x05] = FillStructOpcode(nullptr, 1, "DEC B");
   liste_opcodes_[0x06] = FillStructOpcode(nullptr, 2, "LD B, %n");
   liste_opcodes_[0x07] = FillStructOpcode(nullptr, 1, "RLCA");
   liste_opcodes_[0x08] = FillStructOpcode(nullptr, 1, "EX AF AF'");
   liste_opcodes_[0x09] = FillStructOpcode(nullptr, 1, "ADD HL, BC");
   liste_opcodes_[0x0A] = FillStructOpcode(nullptr, 1, "LD A, (BC)");
   liste_opcodes_[0x0B] = FillStructOpcode(nullptr, 1, "DEC BC");
   liste_opcodes_[0x0C] = FillStructOpcode(nullptr, 1, "INC C");
   liste_opcodes_[0x0D] = FillStructOpcode(nullptr, 1, "DEC C");
   liste_opcodes_[0x0E] = FillStructOpcode(nullptr, 2, "LD C,  %n");
   liste_opcodes_[0x0F] = FillStructOpcode(nullptr, 1, "RRCA");
   liste_opcodes_[0x10] = FillStructOpcode(nullptr, 2, "DJNZ %j__");
   liste_opcodes_[0x11] = FillStructOpcode(nullptr, 3, "LD DE, %nn__");
   liste_opcodes_[0x12] = FillStructOpcode(nullptr, 1, "LD (DE), A");
   liste_opcodes_[0x13] = FillStructOpcode(nullptr, 1, "INC DE");
   liste_opcodes_[0x14] = FillStructOpcode(nullptr, 1, "INC D");
   liste_opcodes_[0x15] = FillStructOpcode(nullptr, 1, "DEC D");
   liste_opcodes_[0x16] = FillStructOpcode(nullptr, 2, "LD D,  %n");
   liste_opcodes_[0x17] = FillStructOpcode(nullptr, 1, "RLA");
   liste_opcodes_[0x18] = FillStructOpcode(nullptr, 2, "JR %j__");
   liste_opcodes_[0x19] = FillStructOpcode(nullptr, 1, "ADD HL, DE");
   liste_opcodes_[0x1A] = FillStructOpcode(nullptr, 1, "LD A, (DE)");
   liste_opcodes_[0x1B] = FillStructOpcode(nullptr, 1, "DEC DE");
   liste_opcodes_[0x1C] = FillStructOpcode(nullptr, 1, "INC E");
   liste_opcodes_[0x1D] = FillStructOpcode(nullptr, 1, "DEC E");
   liste_opcodes_[0x1E] = FillStructOpcode(nullptr, 2, "LD E,  %n");
   liste_opcodes_[0x1F] = FillStructOpcode(nullptr, 1, "RRA");
   liste_opcodes_[0x20] = FillStructOpcode(nullptr, 2, "JR NZ  %j__");
   liste_opcodes_[0x21] = FillStructOpcode(nullptr, 3, "LD HL, %nn__");
   liste_opcodes_[0x22] = FillStructOpcode(nullptr, 3, "LD (%nn__), HL");
   liste_opcodes_[0x23] = FillStructOpcode(nullptr, 1, "INC HL");
   liste_opcodes_[0x24] = FillStructOpcode(nullptr, 1, "INC H");
   liste_opcodes_[0x25] = FillStructOpcode(nullptr, 1, "DEC H");
   liste_opcodes_[0x26] = FillStructOpcode(nullptr, 2, "LD H,  %n");
   liste_opcodes_[0x27] = FillStructOpcode(nullptr, 1, "DAA");
   liste_opcodes_[0x28] = FillStructOpcode(nullptr, 2, "JR Z  %j__");
   liste_opcodes_[0x29] = FillStructOpcode(nullptr, 1, "ADD HL, HL");
   liste_opcodes_[0x2A] = FillStructOpcode(nullptr, 3, "LD HL, (%nn__)");
   liste_opcodes_[0x2B] = FillStructOpcode(nullptr, 1, "DEC HL");
   liste_opcodes_[0x2C] = FillStructOpcode(nullptr, 1, "INC L");
   liste_opcodes_[0x2D] = FillStructOpcode(nullptr, 1, "DEC L");
   liste_opcodes_[0x2E] = FillStructOpcode(nullptr, 2, "LD L,  %n");
   liste_opcodes_[0x2F] = FillStructOpcode(nullptr, 1, "CPL");
   liste_opcodes_[0x30] = FillStructOpcode(nullptr, 2, "JR NC  %j__");
   liste_opcodes_[0x31] = FillStructOpcode(nullptr, 3, "LD SP, %nn__");
   liste_opcodes_[0x32] = FillStructOpcode(nullptr, 3, "LD (%nn__), A");
   liste_opcodes_[0x33] = FillStructOpcode(nullptr, 1, "INC SP");
   liste_opcodes_[0x34] = FillStructOpcode(nullptr, 1, "INC (HL)");
   liste_opcodes_[0x35] = FillStructOpcode(nullptr, 1, "DEC (HL)");
   liste_opcodes_[0x36] = FillStructOpcode(nullptr, 2, "LD (HL), %n");
   liste_opcodes_[0x37] = FillStructOpcode(nullptr, 1, "SCF");
   liste_opcodes_[0x38] = FillStructOpcode(nullptr, 2, "JR C, %j__");
   liste_opcodes_[0x39] = FillStructOpcode(nullptr, 1, "ADD HL, SP");
   liste_opcodes_[0x3A] = FillStructOpcode(nullptr, 3, "LD A, (%nn__)");
   liste_opcodes_[0x3B] = FillStructOpcode(nullptr, 1, "DEC SP");
   liste_opcodes_[0x3C] = FillStructOpcode(nullptr, 1, "INC A");
   liste_opcodes_[0x3D] = FillStructOpcode(nullptr, 1, "DEC A");
   liste_opcodes_[0x3E] = FillStructOpcode(nullptr, 2, "LD A, %n");
   liste_opcodes_[0x3F] = FillStructOpcode(nullptr, 1, "CCF");
   liste_opcodes_[0x40] = FillStructOpcode(nullptr, 1, "LD B, B");
   liste_opcodes_[0x41] = FillStructOpcode(nullptr, 1, "LD B, C");
   liste_opcodes_[0x42] = FillStructOpcode(nullptr, 1, "LD B, D");
   liste_opcodes_[0x43] = FillStructOpcode(nullptr, 1, "LD B, E");
   liste_opcodes_[0x44] = FillStructOpcode(nullptr, 1, "LD B, H");
   liste_opcodes_[0x45] = FillStructOpcode(nullptr, 1, "LD B, L");
   liste_opcodes_[0x46] = FillStructOpcode(nullptr, 1, "LD B, (HL)");
   liste_opcodes_[0x47] = FillStructOpcode(nullptr, 1, "LD B, A");
   liste_opcodes_[0x48] = FillStructOpcode(nullptr, 1, "LD C, B");
   liste_opcodes_[0x49] = FillStructOpcode(nullptr, 1, "LD C, C");
   liste_opcodes_[0x4A] = FillStructOpcode(nullptr, 1, "LD C, D");
   liste_opcodes_[0x4B] = FillStructOpcode(nullptr, 1, "LD C, E");
   liste_opcodes_[0x4C] = FillStructOpcode(nullptr, 1, "LD C, H");
   liste_opcodes_[0x4D] = FillStructOpcode(nullptr, 1, "LD C, L");
   liste_opcodes_[0x4E] = FillStructOpcode(nullptr, 1, "LD C, (HL)");
   liste_opcodes_[0x4F] = FillStructOpcode(nullptr, 1, "LD C, A");
   liste_opcodes_[0x50] = FillStructOpcode(nullptr, 1, "LD D, B");
   liste_opcodes_[0x51] = FillStructOpcode(nullptr, 1, "LD D, C");
   liste_opcodes_[0x52] = FillStructOpcode(nullptr, 1, "LD D, D");
   liste_opcodes_[0x53] = FillStructOpcode(nullptr, 1, "LD D, E");
   liste_opcodes_[0x54] = FillStructOpcode(nullptr, 1, "LD D, H");
   liste_opcodes_[0x55] = FillStructOpcode(nullptr, 1, "LD D, L");
   liste_opcodes_[0x56] = FillStructOpcode(nullptr, 1, "LD D, (HL)");
   liste_opcodes_[0x57] = FillStructOpcode(nullptr, 1, "LD D, A");
   liste_opcodes_[0x58] = FillStructOpcode(nullptr, 1, "LD E, B");
   liste_opcodes_[0x59] = FillStructOpcode(nullptr, 1, "LD E, C");
   liste_opcodes_[0x5A] = FillStructOpcode(nullptr, 1, "LD E, D");
   liste_opcodes_[0x5B] = FillStructOpcode(nullptr, 1, "LD E, E");
   liste_opcodes_[0x5C] = FillStructOpcode(nullptr, 1, "LD E, H");
   liste_opcodes_[0x5D] = FillStructOpcode(nullptr, 1, "LD E, L");
   liste_opcodes_[0x5E] = FillStructOpcode(nullptr, 1, "LD E, (HL)");
   liste_opcodes_[0x5F] = FillStructOpcode(nullptr, 1, "LD E, A");
   liste_opcodes_[0x60] = FillStructOpcode(nullptr, 1, "LD H, B");
   liste_opcodes_[0x61] = FillStructOpcode(nullptr, 1, "LD H, C");
   liste_opcodes_[0x62] = FillStructOpcode(nullptr, 1, "LD H, D");
   liste_opcodes_[0x63] = FillStructOpcode(nullptr, 1, "LD H, E");
   liste_opcodes_[0x64] = FillStructOpcode(nullptr, 1, "LD H, H");
   liste_opcodes_[0x65] = FillStructOpcode(nullptr, 1, "LD H, L");
   liste_opcodes_[0x66] = FillStructOpcode(nullptr, 1, "LD H, (HL)");
   liste_opcodes_[0x67] = FillStructOpcode(nullptr, 1, "LD H, A");
   liste_opcodes_[0x68] = FillStructOpcode(nullptr, 1, "LD L, B");
   liste_opcodes_[0x69] = FillStructOpcode(nullptr, 1, "LD L, C");
   liste_opcodes_[0x6A] = FillStructOpcode(nullptr, 1, "LD L, D");
   liste_opcodes_[0x6B] = FillStructOpcode(nullptr, 1, "LD L, E");
   liste_opcodes_[0x6C] = FillStructOpcode(nullptr, 1, "LD L, H");
   liste_opcodes_[0x6D] = FillStructOpcode(nullptr, 1, "LD L, L");
   liste_opcodes_[0x6E] = FillStructOpcode(nullptr, 1, "LD L, (HL)");
   liste_opcodes_[0x6F] = FillStructOpcode(nullptr, 1, "LD L, A");
   liste_opcodes_[0x70] = FillStructOpcode(nullptr, 1, "LD (HL), B");
   liste_opcodes_[0x71] = FillStructOpcode(nullptr, 1, "LD (HL), C");
   liste_opcodes_[0x72] = FillStructOpcode(nullptr, 1, "LD (HL), D");
   liste_opcodes_[0x73] = FillStructOpcode(nullptr, 1, "LD (HL), E");
   liste_opcodes_[0x74] = FillStructOpcode(nullptr, 1, "LD (HL), H");
   liste_opcodes_[0x75] = FillStructOpcode(nullptr, 1, "LD (HL), L");
   liste_opcodes_[0x76] = FillStructOpcode(nullptr, 1, "HALT");
   liste_opcodes_[0x77] = FillStructOpcode(nullptr, 1, "LD (HL), A");
   liste_opcodes_[0x78] = FillStructOpcode(nullptr, 1, "LD A, B");
   liste_opcodes_[0x79] = FillStructOpcode(nullptr, 1, "LD A, C");
   liste_opcodes_[0x7A] = FillStructOpcode(nullptr, 1, "LD A, D");
   liste_opcodes_[0x7B] = FillStructOpcode(nullptr, 1, "LD A, E");
   liste_opcodes_[0x7C] = FillStructOpcode(nullptr, 1, "LD A, H");
   liste_opcodes_[0x7D] = FillStructOpcode(nullptr, 1, "LD A, L");
   liste_opcodes_[0x7E] = FillStructOpcode(nullptr, 1, "LD A, (HL)");
   liste_opcodes_[0x7F] = FillStructOpcode(nullptr, 1, "LD A, A");
   liste_opcodes_[0x80] = FillStructOpcode(nullptr, 1, "ADD A, B");
   liste_opcodes_[0x81] = FillStructOpcode(nullptr, 1, "ADD A, C");
   liste_opcodes_[0x82] = FillStructOpcode(nullptr, 1, "ADD A, D");
   liste_opcodes_[0x83] = FillStructOpcode(nullptr, 1, "ADD A, E");
   liste_opcodes_[0x84] = FillStructOpcode(nullptr, 1, "ADD A, H");
   liste_opcodes_[0x85] = FillStructOpcode(nullptr, 1, "ADD A, L");
   liste_opcodes_[0x86] = FillStructOpcode(nullptr, 1, "ADD A, (HL)");
   liste_opcodes_[0x87] = FillStructOpcode(nullptr, 1, "ADD A, A");
   liste_opcodes_[0x88] = FillStructOpcode(nullptr, 1, "ADC A, B");
   liste_opcodes_[0x89] = FillStructOpcode(nullptr, 1, "ADC A, C");
   liste_opcodes_[0x8A] = FillStructOpcode(nullptr, 1, "ADC A, D");
   liste_opcodes_[0x8B] = FillStructOpcode(nullptr, 1, "ADC A, E");
   liste_opcodes_[0x8C] = FillStructOpcode(nullptr, 1, "ADC A, H");
   liste_opcodes_[0x8D] = FillStructOpcode(nullptr, 1, "ADC A, L");
   liste_opcodes_[0x8E] = FillStructOpcode(nullptr, 1, "ADC A, (HL)");
   liste_opcodes_[0x8F] = FillStructOpcode(nullptr, 1, "ADC A, A");
   liste_opcodes_[0x90] = FillStructOpcode(nullptr, 1, "SUB A, B");
   liste_opcodes_[0x91] = FillStructOpcode(nullptr, 1, "SUB A, C");
   liste_opcodes_[0x92] = FillStructOpcode(nullptr, 1, "SUB A, D");
   liste_opcodes_[0x93] = FillStructOpcode(nullptr, 1, "SUB A, E");
   liste_opcodes_[0x94] = FillStructOpcode(nullptr, 1, "SUB A, H");
   liste_opcodes_[0x95] = FillStructOpcode(nullptr, 1, "SUB A, L");
   liste_opcodes_[0x96] = FillStructOpcode(nullptr, 1, "SUB A, (HL)");
   liste_opcodes_[0x97] = FillStructOpcode(nullptr, 1, "SUB A, A");
   liste_opcodes_[0x98] = FillStructOpcode(nullptr, 1, "SBC A, B");
   liste_opcodes_[0x99] = FillStructOpcode(nullptr, 1, "SBC A, C");
   liste_opcodes_[0x9A] = FillStructOpcode(nullptr, 1, "SBC A, D");
   liste_opcodes_[0x9B] = FillStructOpcode(nullptr, 1, "SBC A, E");
   liste_opcodes_[0x9C] = FillStructOpcode(nullptr, 1, "SBC A, H");
   liste_opcodes_[0x9D] = FillStructOpcode(nullptr, 1, "SBC A, L");
   liste_opcodes_[0x9E] = FillStructOpcode(nullptr, 1, "SBC A, (HL)");
   liste_opcodes_[0x9F] = FillStructOpcode(nullptr, 1, "SBC A, A");
   liste_opcodes_[0xA0] = FillStructOpcode(nullptr, 1, "AND B");
   liste_opcodes_[0xA1] = FillStructOpcode(nullptr, 1, "AND C");
   liste_opcodes_[0xA2] = FillStructOpcode(nullptr, 1, "AND D");
   liste_opcodes_[0xA3] = FillStructOpcode(nullptr, 1, "AND E");
   liste_opcodes_[0xA4] = FillStructOpcode(nullptr, 1, "AND H");
   liste_opcodes_[0xA5] = FillStructOpcode(nullptr, 1, "AND L");
   liste_opcodes_[0xA6] = FillStructOpcode(nullptr, 1, "AND (HL)");
   liste_opcodes_[0xA7] = FillStructOpcode(nullptr, 1, "AND A");
   liste_opcodes_[0xA8] = FillStructOpcode(nullptr, 1, "XOR B");
   liste_opcodes_[0xA9] = FillStructOpcode(nullptr, 1, "XOR C");
   liste_opcodes_[0xAA] = FillStructOpcode(nullptr, 1, "XOR D");
   liste_opcodes_[0xAB] = FillStructOpcode(nullptr, 1, "XOR E");
   liste_opcodes_[0xAC] = FillStructOpcode(nullptr, 1, "XOR H");
   liste_opcodes_[0xAD] = FillStructOpcode(nullptr, 1, "XOR L");
   liste_opcodes_[0xAE] = FillStructOpcode(nullptr, 1, "XOR (HL)");
   liste_opcodes_[0xAF] = FillStructOpcode(nullptr, 1, "XOR A");
   liste_opcodes_[0xB0] = FillStructOpcode(nullptr, 1, "OR B");
   liste_opcodes_[0xB1] = FillStructOpcode(nullptr, 1, "OR C");
   liste_opcodes_[0xB2] = FillStructOpcode(nullptr, 1, "OR D");
   liste_opcodes_[0xB3] = FillStructOpcode(nullptr, 1, "OR E");
   liste_opcodes_[0xB4] = FillStructOpcode(nullptr, 1, "OR H");
   liste_opcodes_[0xB5] = FillStructOpcode(nullptr, 1, "OR L");
   liste_opcodes_[0xB6] = FillStructOpcode(nullptr, 1, "OR (HL)");
   liste_opcodes_[0xB7] = FillStructOpcode(nullptr, 1, "OR A");
   liste_opcodes_[0xB8] = FillStructOpcode(nullptr, 1, "CP B");
   liste_opcodes_[0xB9] = FillStructOpcode(nullptr, 1, "CP C");
   liste_opcodes_[0xBA] = FillStructOpcode(nullptr, 1, "CP D");
   liste_opcodes_[0xBB] = FillStructOpcode(nullptr, 1, "CP E");
   liste_opcodes_[0xBC] = FillStructOpcode(nullptr, 1, "CP H");
   liste_opcodes_[0xBD] = FillStructOpcode(nullptr, 1, "CP L");
   liste_opcodes_[0xBE] = FillStructOpcode(nullptr, 1, "CP (HL)");
   liste_opcodes_[0xBF] = FillStructOpcode(nullptr, 1, "CP A");
   liste_opcodes_[0xC0] = FillStructOpcode(nullptr, 1, "RET NZ");
   liste_opcodes_[0xC1] = FillStructOpcode(nullptr, 1, "POP BC");
   liste_opcodes_[0xC2] = FillStructOpcode(nullptr, 3, "JP NZ %nn__");
   liste_opcodes_[0xC3] = FillStructOpcode(nullptr, 3, "JP %nn__");
   liste_opcodes_[0xC4] = FillStructOpcode(nullptr, 3, "CALL NZ %nn__");
   liste_opcodes_[0xC5] = FillStructOpcode(nullptr, 1, "PUSH BC");
   liste_opcodes_[0xC6] = FillStructOpcode(nullptr, 2, "ADD A, %n");
   liste_opcodes_[0xC7] = FillStructOpcode(nullptr, 1, "RST 0");
   liste_opcodes_[0xC8] = FillStructOpcode(nullptr, 1, "RET Z");
   liste_opcodes_[0xC9] = FillStructOpcode(nullptr, 1, "RET");
   liste_opcodes_[0xCA] = FillStructOpcode(nullptr, 3, "JP Z %nn__");
   liste_opcodes_[0xCB] = FillStructOpcode(nullptr, 1, "%CB");
   liste_opcodes_[0xCC] = FillStructOpcode(nullptr, 3, "CALL Z %nn__");
   liste_opcodes_[0xCD] = FillStructOpcode(nullptr, 3, "CALL %nn__");
   liste_opcodes_[0xCE] = FillStructOpcode(nullptr, 2, "ADC A, %n");
   liste_opcodes_[0xCF] = FillStructOpcode(nullptr, 1, "RST 08H");
   liste_opcodes_[0xD0] = FillStructOpcode(nullptr, 1, "RET NC");
   liste_opcodes_[0xD1] = FillStructOpcode(nullptr, 1, "POP DE");
   liste_opcodes_[0xD2] = FillStructOpcode(nullptr, 3, "JP NC %nn__");
   liste_opcodes_[0xD3] = FillStructOpcode(nullptr, 2, "OUT (%n), A");
   liste_opcodes_[0xD4] = FillStructOpcode(nullptr, 3, "CALL NC %nn__");
   liste_opcodes_[0xD5] = FillStructOpcode(nullptr, 1, "PUSH DE");
   liste_opcodes_[0xD6] = FillStructOpcode(nullptr, 2, "SUB A, %n");
   liste_opcodes_[0xD7] = FillStructOpcode(nullptr, 1, "RST 10H");
   liste_opcodes_[0xD8] = FillStructOpcode(nullptr, 1, "RET C");
   liste_opcodes_[0xD9] = FillStructOpcode(nullptr, 1, "EXX");
   liste_opcodes_[0xDA] = FillStructOpcode(nullptr, 3, "JP C %nn__");
   liste_opcodes_[0xDB] = FillStructOpcode(nullptr, 2, "IN A, (%n)");
   liste_opcodes_[0xDC] = FillStructOpcode(nullptr, 3, "CALL C %nn__");
   liste_opcodes_[0xDD] = FillStructOpcode(nullptr, 1, "%DD");
   liste_opcodes_[0xDE] = FillStructOpcode(nullptr, 2, "SBC A, %n");
   liste_opcodes_[0xDF] = FillStructOpcode(nullptr, 1, "RST 18H");
   liste_opcodes_[0xE0] = FillStructOpcode(nullptr, 1, "RET PO");
   liste_opcodes_[0xE1] = FillStructOpcode(nullptr, 1, "POP HL");
   liste_opcodes_[0xE2] = FillStructOpcode(nullptr, 3, "JP PO %nn__");
   liste_opcodes_[0xE3] = FillStructOpcode(nullptr, 1, "EX (SP), HL");
   liste_opcodes_[0xE4] = FillStructOpcode(nullptr, 3, "CALL PO %nn__");
   liste_opcodes_[0xE5] = FillStructOpcode(nullptr, 1, "PUSH HL");
   liste_opcodes_[0xE6] = FillStructOpcode(nullptr, 2, "AND %n");
   liste_opcodes_[0xE7] = FillStructOpcode(nullptr, 1, "RST 20H");
   liste_opcodes_[0xE8] = FillStructOpcode(nullptr, 1, "RET PE");
   liste_opcodes_[0xE9] = FillStructOpcode(nullptr, 1, "JP (HL)");
   liste_opcodes_[0xEA] = FillStructOpcode(nullptr, 3, "JP PE %nn__");
   liste_opcodes_[0xEB] = FillStructOpcode(nullptr, 1, "EX DE,HL");
   liste_opcodes_[0xEC] = FillStructOpcode(nullptr, 3, "CALL PE %nn__");
   liste_opcodes_[0xED] = FillStructOpcode(nullptr, 1, "%ED");
   liste_opcodes_[0xEE] = FillStructOpcode(nullptr, 2, "XOR %n");
   liste_opcodes_[0xEF] = FillStructOpcode(nullptr, 1, "RST 28H");
   liste_opcodes_[0xF0] = FillStructOpcode(nullptr, 1, "RET P");
   liste_opcodes_[0xF1] = FillStructOpcode(nullptr, 1, "POP AF");
   liste_opcodes_[0xF2] = FillStructOpcode(nullptr, 3, "JP P %nn__");
   liste_opcodes_[0xF3] = FillStructOpcode(nullptr, 1, "DI");
   liste_opcodes_[0xF4] = FillStructOpcode(nullptr, 3, "CALL P %nn__");
   liste_opcodes_[0xF5] = FillStructOpcode(nullptr, 1, "PUSH AF");
   liste_opcodes_[0xF6] = FillStructOpcode(nullptr, 2, "OR %n");
   liste_opcodes_[0xF7] = FillStructOpcode(nullptr, 1, "RST 30H");
   liste_opcodes_[0xF8] = FillStructOpcode(nullptr, 1, "RET M");
   liste_opcodes_[0xF9] = FillStructOpcode(nullptr, 1, "LD SP, HL");
   liste_opcodes_[0xFA] = FillStructOpcode(nullptr, 3, "JP M %nn__");
   liste_opcodes_[0xFB] = FillStructOpcode(nullptr, 1, "EI");
   liste_opcodes_[0xFC] = FillStructOpcode(nullptr, 3, "CALL M %nn__");
   liste_opcodes_[0xFD] = FillStructOpcode(nullptr, 1, "%FD");
   liste_opcodes_[0xFE] = FillStructOpcode(nullptr, 2, "CP %n");
   liste_opcodes_[0xFF] = FillStructOpcode(nullptr, 1, "RST 38H");

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
