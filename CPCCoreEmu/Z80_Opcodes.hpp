
#define REGW(reg) \
   ((reg==ADDR_PC)?pc_:(reg==ADDR_AF)?af_.w:(reg==ADDR_BC)?bc_.w:(reg==ADDR_DE)?de_.w:(reg==ADDR_HL)?hl_.w:(reg==ADDR_IX)?ix_.w:(reg==ADDR_IY)?iy_.w:(reg==ADDR_SP)?sp_:(reg==ADDR_AFP)?af_p_.w:(reg==ADDR_BCP)?bc_p_.w:(reg==ADDR_DEP)?de_p_.w:(reg==ADDR_HLP)?hl_p_.w:hl_p_.w)
#define REG(reg) ((reg==R_A)?af_.b.h:(reg==R_F)?af_.b.l:(reg==R_B)?bc_.b.h:(reg==R_C)?bc_.b.l:(reg==R_D)?de_.b.h:(reg==R_E)?de_.b.l:(reg==R_H)?hl_.b.h:hl_.b.l)
#define REG_First(reg) \
   (reg==ADDR_AF)?af_.b.h:(reg==ADDR_BC)?bc_.b.h:(reg==ADDR_DE)?de_.b.h:(reg==ADDR_HL)?hl_.b.h:(reg==ADDR_IX)?ix_.b.h:iy_.b.h

#define INT_TO_REG(i) ((i==0)?bc_.b.h:(i==1)?bc_.b.l:(i==2)?de_.b.h:(i==3)?de_.b.l:(i==4)?hl_.b.h:(i==5)?hl_.b.l:af_.b.h)

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

   NEXT_INSTR_RES((current_opcode_ & 0xFFFF00))
}

template<Z80::Registers reg, bool reset_ptr>
unsigned int Z80::Opcode_Dec_Reg()
{
   int nextcycle;
   REG(reg)--;
   q_ = (af_.b.l&CF) | SzhvDec[REG(reg)];
   af_.b.l = q_;

   NEXT_INSTR_RES((current_opcode_ & 0xFFFF00))
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
      NEXT_INSTR_RES((current_opcode_ & 0xFFFF00))
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

template<Z80::Registers reg1, Z80::Registers reg2>
unsigned int Z80::Opcode_Ld_Reg()
{
   int nextcycle;
   REG(reg1) = REG(reg2);
   NEXT_INSTR;
}

template<Z80::Registers reg, bool Carry>
unsigned int Z80::Opcode_Add_Reg()
{
   int nextcycle;
   unsigned int res = af_.b.h + REG(reg) + ((Carry)? (af_.b.l&CF):0);

   q_ = (((res & 0xff) == 0) ? ZF : 0) | (res >> 8) | (res & 0x80) | ((af_.b.h^res^REG(reg))&HF) | (res & 0x28);
   if ((((af_.b.h & 0x80) ^ (REG(reg) & 0x80)) == 0) && (((REG(reg) & 0x80) ^ (res & 0x80)) != 0))
      q_ |= PF;
   af_.b.h = res;
   af_.b.l = q_;
   
   NEXT_INSTR
}

template<Z80::Registers reg, bool Carry>
unsigned int Z80::Opcode_Sub_Reg()
{
   int nextcycle;
   unsigned int res = af_.b.h - (REG(reg) + (Carry? (af_.b.l&CF):0) );
   q_ = NF | (((res & 0xff) == 0) ? ZF : 0) | ((res >> 8)&CF) | (res & 0x80) | ((af_.b.h^res^REG(reg))&HF);
   if ((((af_.b.h & 0x80) ^ (REG(reg) & 0x80)) != 0) && (((af_.b.h & 0x80) ^ (res & 0x80)) != 0)) q_ |= PF;/*else af_.b.l &= ~(PF);*/
   af_.b.h = res; 
   q_ |= (af_.b.h & 0x28);   
   af_.b.l = q_; \

   NEXT_INSTR
}

template<Z80::Registers reg, Z80::OperationType op>
unsigned int Z80::Opcode_BOOL_Reg()
{
   int nextcycle;
   if (op == AND)
   {
      af_.b.h &= REG(reg);
      q_ = FlagAnd[af_.b.h];
   }
   else if (op == OR)
   {
      af_.b.h |= REG(reg);
      q_ = FlagOr[af_.b.h];
   }
   else if (op == XOR)
   {
      af_.b.h ^= REG(reg);
      q_ = FlagXor[af_.b.h];
   }
   af_.b.l = q_;
   NEXT_INSTR; 
}

template<Z80::Registers reg>
unsigned int Z80::Opcode_CP_Reg()
{
   int nextcycle;
   unsigned int res = af_.b.h - REG(reg);
   q_ = NF | (((res & 0xff) == 0) ? ZF : 0) | (res & 0x80) | ((res >> 8) & CF) | ((af_.b.h ^ res ^ REG(reg)) & HF);
   if ((((af_.b.h & 0x80) ^ (REG(reg) & 0x80)) != 0) && (((af_.b.h & 0x80) ^ (res & 0x80)) != 0)) q_ |= PF;
      q_ |= (REG(reg) & 0x28);
   af_.b.l = q_;
   NEXT_INSTR;
}

template<unsigned char cond, bool present>
unsigned int Z80::Opcode_Ret_Cond()
{
   int nextcycle;
   if (t_ == 5)
   {
      if ((af_.b.l & cond) == (present? cond:0))
      {
         machine_cycle_ = M_MEMORY_R;
         t_ = 1; 
         current_address_ = sp_++; 
         current_data_ = 0; 
         read_count_ = 0;
      }
      else
      {
         NEXT_INSTR
      }
   }
   else
   {
      ++t_;
      /*
      int t_tmp = 5 - t_;
      t_ = 5;
      return t_tmp;*/
   }
   return 1;
}

template<Z80::AddressRegisters reg>
unsigned int Z80::Opcode_Push()
{
   if (t_ == 5)
   {
      machine_cycle_ = M_MEMORY_W;
      t_ = 1;
      current_address_ = --sp_; 
      current_data_ = REG_First(reg);
      read_count_ = 0;
   }
   else
   {
      ++t_;
   }
   return 1;
}

template<int b> unsigned int Z80::Opcode_RLC()
{
   int nextcycle;
   unsigned char btmp = INT_TO_REG(b) >> 7;
   INT_TO_REG(b) = (INT_TO_REG(b) << 1) + btmp; 
   q_ = btmp | ((((INT_TO_REG(b) & 0xff) == 0) ? ZF : 0) | (INT_TO_REG(b) & 0x80));
   q_ |= (INT_TO_REG(b) & 0x28); 
   PARITY_FLAG(INT_TO_REG(b)); 
   af_.b.l = q_;
   NEXT_INSTR;
}

template<int b> unsigned int Z80::Opcode_RRC()
{
   int nextcycle;
   unsigned char btmp;
   q_ = (INT_TO_REG(b) & 0x1) ? CF : 0; 
   btmp = INT_TO_REG(b) & 0x1; 
   INT_TO_REG(b) = (INT_TO_REG(b) >> 1) + btmp * 0x80; 
   q_ |= ((((INT_TO_REG(b) & 0xff) == 0) ? ZF : 0) | (INT_TO_REG(b) & 0x80)); 
   q_ |= (INT_TO_REG(b) & 0x28); 
   PARITY_FLAG(INT_TO_REG(b)); 
   af_.b.l = q_;
   NEXT_INSTR;
}

template<int b> unsigned int Z80::Opcode_RL()
{
   int nextcycle;
   unsigned char btmp;
   btmp = af_.b.l&CF; 
   q_ = (INT_TO_REG(b) & 0x80) ? CF : 0; 
   INT_TO_REG(b) = INT_TO_REG(b) << 1; 
   INT_TO_REG(b) += btmp; 
   q_ |= ((((INT_TO_REG(b) & 0xff) == 0) ? ZF : 0) | (INT_TO_REG(b) & 0x80)); 
   q_ |= (INT_TO_REG(b) & 0x28); 
   PARITY_FLAG(INT_TO_REG(b)); 
   af_.b.l = q_;
   NEXT_INSTR;
}

template<int b> unsigned int Z80::Opcode_RR()
{
   int nextcycle;
   unsigned char btmp;
   btmp = af_.b.l&CF;
   q_ = (INT_TO_REG(b) & 0x1) ? CF : 0; 
   INT_TO_REG(b) = INT_TO_REG(b) >> 1; 
   INT_TO_REG(b) += btmp * 0x80; 
   q_ |= ((((INT_TO_REG(b) & 0xff) == 0) ? ZF : 0) | (INT_TO_REG(b) & 0x80)); 
   q_ |= (INT_TO_REG(b) & 0x28); 
   PARITY_FLAG(INT_TO_REG(b)); 
   af_.b.l = q_;
   NEXT_INSTR;
}

template<int b> unsigned int Z80::Opcode_SLA()
{
   int nextcycle;
   unsigned char btmp;
   if ((INT_TO_REG(b) & 0x80) == 0x80)
      q_ |= (CF); 
   INT_TO_REG(b) = INT_TO_REG(b) << 1; 
   if (INT_TO_REG(b) == 0x00) 
      q_ |= (ZF); 
   q_ |= ((INT_TO_REG(b)&SF) | (INT_TO_REG(b) & 0x28)); 
   PARITY_FLAG(INT_TO_REG(b)); 
   af_.b.l = q_; 

   NEXT_INSTR;
}

template<int b> unsigned int Z80::Opcode_SRA()
{
   int nextcycle;
   unsigned char btmp;
   if ((INT_TO_REG(b) & 0x01) == 0x01)
      q_ |= (CF); 
   btmp = INT_TO_REG(b) & 0x80;
   INT_TO_REG(b) = (INT_TO_REG(b) >> 1) | btmp; 
   if (INT_TO_REG(b) == 0x00)
      q_ |= (ZF);
   q_ |= ((INT_TO_REG(b)&SF) | (INT_TO_REG(b) & 0x28));
   PARITY_FLAG(INT_TO_REG(b)); 
   af_.b.l = q_;

   NEXT_INSTR;
}

template<int b> unsigned int Z80::Opcode_SLL()
{
   int nextcycle;
   unsigned char btmp;
   if ((INT_TO_REG(b) & 0x80) == 0x80)
      q_ |= (CF);
   INT_TO_REG(b) = (INT_TO_REG(b) << 1) + 1; 
   if (INT_TO_REG(b) == 0x00) q_ |= (ZF); 
   q_ |= ((INT_TO_REG(b)&SF) | (INT_TO_REG(b) & 0x28));
   PARITY_FLAG(INT_TO_REG(b)); 
   af_.b.l = q_;
   NEXT_INSTR;
}

template<int b> unsigned int Z80::Opcode_SRL()
{
   int nextcycle;
   unsigned char btmp;
   if ((INT_TO_REG(b) & 0x01) == 0x01)
      q_ |= (CF);
   INT_TO_REG(b) = INT_TO_REG(b) >> 1;
   if (INT_TO_REG(b) == 0x00) q_ |= (ZF);
   q_ |= ((INT_TO_REG(b)&SF) | (INT_TO_REG(b) & 0x28));
   PARITY_FLAG(INT_TO_REG(b));
   af_.b.l = q_;
   NEXT_INSTR;
}


