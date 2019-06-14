
#define REGW(reg) ((reg==ADDR_BC)?bc_.w:(reg==ADDR_DE)?de_.w:(reg==ADDR_HL)?hl_.w:(reg==ADDR_IX)?ix_.w:iy_.w)
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
   const unsigned char lookup_ZF[2] = { 0,ZF };
   const unsigned char lookup_PF[2] = { 0,PF };
   const unsigned char lookup_HF[2] = { 0,HF };
   int nextcycle;
   REG(reg)++;
   q_ = (af_.b.l&CF) | SzhvInc[REG(reg)];
   //q_ = (af_.b.l&CF) | (lookup_ZF[REG(reg) == 0]) | (REG(reg) & 0x80) | (lookup_PF[REG(reg) == 0x80]) | (lookup_HF[((REG(reg) & 0x0F) == 0x00)]) | (REG(reg) & 0x28);
   af_.b.l = q_;

   NEXT_INSTR_RES(reset_ptr)

}