#include "stdafx.h"

#include "Z80_Full.h"
#include "Sig.h"
#include "Memoire.h"


unsigned int Z80::Opcode_CB()
{
   current_function_ = &fetch_func_CB_;
   machine_cycle_ = M_FETCH; t_ = 1;
   return 1;
}

unsigned int Z80::Opcode_ED()
{
   current_function_ = &fetch_func_ED_;
   machine_cycle_ = M_FETCH; t_ = 1;
   return 1;
}
unsigned int Z80::Opcode_DD()
{
   current_function_ = &fetch_func_DD_;
   machine_cycle_ = M_FETCH; t_ = 1;
   return 1;
}

unsigned int Z80::Opcode_FD()
{
   current_function_ = &fetch_func_FD_;
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

