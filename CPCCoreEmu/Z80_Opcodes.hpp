
#define REGW(reg) \
   ((reg==ADDR_PC)?pc_:(reg==ADDR_AF)?af_.w:(reg==ADDR_BC)?bc_.w:(reg==ADDR_DE)?de_.w:(reg==ADDR_HL)?hl_.w:(reg==ADDR_IX)?ix_.w:(reg==ADDR_IY)?iy_.w:(reg==ADDR_SP)?sp_:(reg==ADDR_AFP)?af_p_.w:(reg==ADDR_BCP)?bc_p_.w:(reg==ADDR_DEP)?de_p_.w:(reg==ADDR_HLP)?hl_p_.w:hl_p_.w)
#define REG(reg) ((reg==R_A)?af_.b.h:(reg==R_F)?af_.b.l:(reg==R_B)?bc_.b.h:(reg==R_C)?bc_.b.l:(reg==R_D)?de_.b.h:(reg==R_E)?de_.b.l:(reg==R_H)?hl_.b.h:hl_.b.l)


template<Z80::AddressRegisters addr, Z80::Registers reg>
unsigned int Z80::Opcode_Memory_Write_Addr_Reg()
{
   machine_cycle_ = M_MEMORY_W;
   t_ = 1;
   current_address_ = REGW(addr);
   current_data_ = REG(reg);
   read_count_ = 0;
   return 1;
}

template<Z80::Registers reg, bool reset_ptr>
unsigned int Z80::Opcode_Inc_Reg()
{
   int nextcycle;
   REG(reg)++;
   q_ = (af_.b.l&CF) | SzhvInc[REG(reg)];
   af_.b.l = q_;

   NEXT_INSTR_RES((current_opcode_ & 0xFFFF00) && reset_ptr)
}

template<Z80::Registers reg, bool reset_ptr>
unsigned int Z80::Opcode_Dec_Reg()
{
   int nextcycle;
   REG(reg)--;
   q_ = (af_.b.l&CF) | SzhvDec[REG(reg)];
   af_.b.l = q_;

   NEXT_INSTR_RES((current_opcode_ & 0xFFFF00) && reset_ptr)
}

template<Z80::AddressRegisters reg, bool reset_ptr>
unsigned int Z80::Opcode_Inc_RegW()
{
   if (t_ != 6)
   {
      //t_ = 6;return 2;
      t_++; return 1;
   }
   else
   {
      int nextcycle;
      REGW(reg)++;
      NEXT_INSTR_RES((current_opcode_ & 0xFFFF00) && reset_ptr)
   }
}

template<Z80::AddressRegisters reg>
unsigned int Z80::Opcode_Dec_RegW()
{
   machine_cycle_ = M_Z80_WORK;
   REGW(reg)--;
   counter_ += (2 - 1);
   t_ = 1;
   return 2;
}

template<Z80::AddressRegisters reg1, Z80::AddressRegisters reg2>
unsigned int Z80::Opcode_EX()
{
   int nextcycle;
   unsigned short t = REGW(reg1); 
   REGW(reg1) = REGW(reg2);
   REGW(reg2) = t;
   NEXT_INSTR((current_opcode_ & 0xFFFF00));
}

template<Z80::AddressRegisters reg1, Z80::AddressRegisters reg2>
unsigned int Z80::Opcode_ADD_REGW()
{
   unsigned int res = REGW(reg1)+ REGW(reg2);  
   mem_ptr_.w = REGW(reg1) + 1;
   q_ = (af_.b.l&(SF | ZF | PF)) | ((res >> 8) & 0x28) | (((REGW(reg1) ^res^REGW(reg2)) >> 8)&HF) | ((res >> 16)&CF);
   af_.b.l = q_;
   REGW(reg1) = res;

   machine_cycle_ = M_Z80_WORK; 
   counter_ += 6;
   t_ = 1;
   return 7;
}

template<Z80::AddressRegisters reg>
unsigned int Z80::Opcode_Memory_Read_REGW()
{
   machine_cycle_ = M_MEMORY_R;
   t_ = 1;
   current_address_ = REGW(reg);
   current_data_ = 0;
   read_count_ = 0;
   return 1;
}


template<Z80::AddressRegisters reg, int t_val>
unsigned int Z80::Opcode_Memory_Read_Delayed()
{
   /*
   if (t_ == 5)
   {
      machine_cycle_ = M_MEMORY_R; 
      t_ = 1; 
      current_address_ = REGW(reg);
      current_data_ = 0; 
      read_count_ = 0;
   }
   else
   {
      ++t_;
   }
   return 1;
   */
   // This one give wrong timings
   if (t_ == t_val)
   {
      machine_cycle_ = M_MEMORY_R;
      t_ = 1;
      current_address_ = REGW(reg);
      current_data_ = 0; 
      read_count_ = 0;
      return 1;
   }
   else
   {
      int t_tmp = t_val - t_;
      t_ = t_val;
      return t_tmp;
   }
   /*
   machine_cycle_ = M_MEMORY_R;
   t_ = 1;
   current_address_ = REGW(reg);
   current_data_ = 0;
   read_count_ = 0;
   return t_val - 3;*/

}