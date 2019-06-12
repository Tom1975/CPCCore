#define AFFECT_ADDRESS_REG(var,addr)\
   switch (addr){\
   case ADDR_BC:var = bc_.w;break;\
   case ADDR_DE:var = de_.w;break;\
   case ADDR_HL:var = hl_.w;break;\
   case ADDR_IX:var = ix_.w;break;\
   case ADDR_IY:var = iy_.w;break;\
   }

#define AFFECT_REG(var,reg)\
   switch (reg){\
   case R_A:var = af_.b.h;break;\
   case R_F:var = af_.b.l;break;\
   case R_B:var = bc_.b.h;break;\
   case R_C:var = bc_.b.l;break;\
   case R_D:var = de_.b.h;break;\
   case R_E:var = de_.b.l;break;\
   }

#define INC_REG(reg)\
   switch (reg){\
   case R_A:af_.b.h++;break;\
   case R_F:af_.b.l++;break;\
   case R_B:bc_.b.h++;break;\
   case R_C:bc_.b.l++;break;\
   case R_D:de_.b.h++;break;\
   case R_E:de_.b.l++;break;\
   }


#define REG(reg) ((reg==R_A)?af_.b.h:(reg==R_F)?af_.b.l:(reg==R_B)?bc_.b.h:(reg==R_C)?bc_.b.l:(reg==R_D)?de_.b.h:de_.b.l)
   

template<Z80::AddressRegisters addr, Z80::Registers reg>
unsigned int Z80::Opcode_Memory_Write_Addr_Reg()
{
   machine_cycle_ = M_MEMORY_W; 
   t_ = 1;
   AFFECT_ADDRESS_REG(current_address_, addr);
   AFFECT_REG(current_data_, reg);
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
   //q_ = (af_.b.l&CF)|((REG(reg) ==0)?ZF:0) | (REG(reg) & 0x80 ) | ((REG(reg) == 0x80)?PF:0)|(((REG(reg) &0x0F)==0x00)?HF:0) | (REG(reg) & 0x28);
   q_ = (af_.b.l&CF) | SzhvInc[REG(reg)];
   //q_ = (af_.b.l&CF) | (lookup_ZF[REG(reg) == 0]) | (REG(reg) & 0x80) | (lookup_PF[REG(reg) == 0x80]) | (lookup_HF[((REG(reg) & 0x0F) == 0x00)]) | (REG(reg) & 0x28);
   af_.b.l = q_;

   NEXT_INSTR_RES(reset_ptr)
}