
#define REGW(reg) ((reg==ADDR_BC)?bc_.w:(reg==ADDR_DE)?de_.w:(reg==ADDR_HL)?hl_.w:(reg==ADDR_IX)?ix_.w:(reg==ADDR_IY)?iy_.w:sp_)
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
      NEXT_INSTR_RES((current_opcode_&0xFFFF00)&& reset_ptr)
   }
}