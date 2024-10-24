
#define REGW(reg) \
  ((reg==ADDR_PC)?pc_:\
   (reg==ADDR_AF)?af_.w:\
   (reg==ADDR_BC)?bc_.w:\
   (reg==ADDR_DE)?de_.w:\
   (reg==ADDR_HL)?hl_.w:\
   (reg==ADDR_IX)?ix_.w:\
   (reg==ADDR_IY)?iy_.w:\
   (reg==ADDR_SP)?sp_:\
   (reg==ADDR_AFP)?af_p_.w:\
   (reg==ADDR_BCP)?bc_p_.w:\
   (reg==ADDR_DEP)?de_p_.w:\
   (reg==ADDR_HLP)?hl_p_.w:\
   hl_p_.w)

#define REG(reg) \
   ((reg==R_A)?af_.b.h:\
   (reg==R_F)?af_.b.l:\
   (reg==R_B)?bc_.b.h:\
   (reg==R_C)?bc_.b.l:\
   (reg==R_D)?de_.b.h:\
   (reg==R_E)?de_.b.l:\
   (reg==R_H)?hl_.b.h:\
   (reg==R_L)?hl_.b.l:\
   (reg==R_I)?ir_.b.h:\
   (reg==R_IXh)?ix_.b.h:\
   (reg==R_IXl)?ix_.b.l:\
   (reg==R_IYh)?iy_.b.h:\
   (reg==R_IYl)?iy_.b.l:\
   ir_.b.l)

#define REG_First(reg) \
   (reg==ADDR_AF)?af_.b.h:(reg==ADDR_BC)?bc_.b.h:(reg==ADDR_DE)?de_.b.h:(reg==ADDR_HL)?hl_.b.h:(reg==ADDR_IX)?ix_.b.h:iy_.b.h

#define INT_TO_REG(i) ((i==0)?bc_.b.h:(i==1)?bc_.b.l:(i==2)?de_.b.h:(i==3)?de_.b.l:(i==4)?hl_.b.h:(i==5)?hl_.b.l:af_.b.h)

template<Z80::MachineCycle operation_source, Z80::AddressRegisters addr, Z80::Registers reg>
unsigned int Z80::Opcode_Write_Addr_Reg()
{
   machine_cycle_ = operation_source;
   t_ = 1;
   current_address_ = REGW(addr);
   if (reg == R_0)
   {
      current_data_ = 0;
   }
   else
   {
      current_data_ = REG(reg);
   }
   
   if (machine_cycle_ == M_MEMORY_W)
   {
      read_count_ = 0;
   }
   else if (machine_cycle_ == M_IO_W)
   {
      q_ = af_.b.l | ((REG(reg) & 0x80) ? NF : 0);
      af_.b.l = q_;
      if (reg == R_A)
      {
         mem_ptr_.w = bc_.w + 1;
      }
   }

      
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
   machine_cycle_ = M_Z80_WAIT;
   REGW(reg)--;
   counter_ += (2 - 1);
   t_ = 1;
   return 2;
}

template<Z80::AddressRegisters reg>
unsigned int Z80::Opcode_Dec_RegWI()
{
   if (t_ == 6)
   {
      int nextcycle;
      --REGW(reg); 
      NEXT_INSTR; 
   }
   else { ++t_; };
   return 1;
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

   machine_cycle_ = M_Z80_WAIT;
   counter_ += 6;
   t_ = 1;
   return 7;
}

template<Z80::MachineCycle operation_source, Z80::AddressRegisters reg>
unsigned int Z80::Opcode_Read_REGW()
{
   //machine_cycle_ = M_MEMORY_R;
   machine_cycle_ = operation_source;
   t_ = 1;
   current_address_ = REGW(reg);
   if (machine_cycle_ == M_MEMORY_R)
   {
      current_data_ = 0;
      read_count_ = 0;
   }
   return 1;
}


template<Z80::MachineCycle operation_source, Z80::AddressRegisters reg>
unsigned int Z80::Opcode_Delayed_Read_REG()
{
   if (t_ == 5) 
   {
      machine_cycle_ = operation_source;
      t_ = 1; 
      current_address_ = REGW(reg);
      if (machine_cycle_ == M_MEMORY_R)
      {
         current_data_ = 0;
         read_count_ = 0;
      }
      return 1;
   }
   else 
   { 
      int t_tmp = 5 - t_;
      t_ = 5;
      return t_tmp;
   }
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

template<Z80::Registers reg1, Z80::Registers reg2>
unsigned int Z80::Opcode_Ld_Delayed_Reg()
{
   int nextcycle;

   if ( reg1 == R_A && (reg2 == R_I || reg2 == R_R) )
   {
      if (t_ == 5) {
         af_.b.h = REG(reg2);
         q_ = af_.b.l; 
         q_ &= ~(HF | NF | 0x28); 
         ZERO_FLAG(af_.b.h); 
         SIGN_FLAG(af_.b.h); 
         if (iff2_)
            q_ |= PF;
         else
            q_ &= ~(PF);
         q_ |= (af_.b.h & 0x28);
         af_.b.l = q_;

         if (reg2 == R_I)
            carry_set_ = true;
         NEXT_INSTR_LDAIR
      }
      else { ++t_; };
   }
   else
   {
      if (t_ == 5) 
      { 
         REG(reg1) = REG(reg2);
         NEXT_INSTR; 
      }
      else
      {
         int t_tmp = 5 - t_;
         t_ = 5;
         return t_tmp;
      }
   }
   return 1;
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

template<Z80::AddressRegisters reg>
unsigned int Z80::Opcode_Add_Reg()
{
   unsigned int res;
   mem_ptr_.w = hl_.w + 1; 
   
   res = hl_.w + REGW(reg) + (af_.b.l&CF);
   q_ = ((res >> 8) & 0x28);
   q_ |= (((res & 0xffff) == 0) ? ZF : 0) | ((res >> 16)&CF) | ((res & 0x8000) ? SF : 0) | (((hl_.w^res^REGW(reg)) >> 8)&HF);
   if ((((hl_.w & 0x8000) ^ (REGW(reg) & 0x8000)) == 0) && (((hl_.w & 0x8000) ^ (res & 0x8000)) != 0)) 
      q_ |= PF;
   af_.b.l = q_; 
   hl_.w = res; 

   machine_cycle_ = M_Z80_WAIT;
   t_ = 4 + 3;
   return 1;
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

template<Z80::AddressRegisters reg>
unsigned int Z80::Opcode_Sub_Reg()
{
   unsigned int res;
   mem_ptr_.w = hl_.w + 1;

   res = hl_.w - (REGW(reg) + (af_.b.l&CF));
   q_ = ((res >> 8) & 0x28);
   q_ |= NF | (((res & 0xffff) == 0) ? ZF : 0) | ((res >> 16)&CF) | ((res & 0x8000) ? SF : 0) | (((hl_.w^res^REGW(reg)) >> 8)&HF);
   if ((((hl_.w & 0x8000) ^ (REGW(reg) & 0x8000)) != 0) && (((hl_.w & 0x8000) ^ (res & 0x8000)) != 0))
      q_ |= PF;
   hl_.w = res; af_.b.l = q_;

   machine_cycle_ = M_Z80_WAIT;
   t_ = 4 + 3;
   return 1;
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
   if ((INT_TO_REG(b) & 0x01) == 0x01)
      q_ |= (CF);
   INT_TO_REG(b) = INT_TO_REG(b) >> 1;
   if (INT_TO_REG(b) == 0x00) q_ |= (ZF);
   q_ |= ((INT_TO_REG(b)&SF) | (INT_TO_REG(b) & 0x28));
   PARITY_FLAG(INT_TO_REG(b));
   af_.b.l = q_;
   NEXT_INSTR;
}

template<int b, Z80::Registers reg> unsigned int Z80::Opcode_BIT()
{
   int nextcycle;
   q_ = (af_.b.l & CF) | HF | (SzBit[REG(reg) & (1<<b)]& ~(YF|XF)) | ((REG(reg) &(YF|XF)));
   af_.b.l = q_;
   NEXT_INSTR;
}

template<int b, Z80::Registers reg> unsigned int Z80::Opcode_RES()
{
   int nextcycle;
   REG(reg) &= ~(1<<b);
   NEXT_INSTR;
}

template<int b, Z80::Registers reg> unsigned int Z80::Opcode_SET()
{
   int nextcycle;
   REG(reg) |= (1 << b);
   NEXT_INSTR;
}

template<int mode> unsigned int Z80::Opcode_IM()
{
   int nextcycle;
   interrupt_mode_ = mode;
   NEXT_INSTR;
}

template<Z80::AddressRegisters reg>
unsigned int Z80::Opcode_JP_REGW()
{
   int nextcycle;
   pc_ = REGW(reg);
   NEXT_INSTR
}

template<Z80::AddressRegisters reg>
unsigned int Z80::Opcode_LD_SP_REGW()
{
   if (t_ == 6)
   {
      int nextcycle;
      sp_ = REGW(reg);
      NEXT_INSTR;
   }
   else
   {
      ++t_;
   }
   return 1;
}

////////////////////////////////// MEMR
template<Z80::AddressRegisters reg>
unsigned int Z80::MEMR_Read_REGW_()
{
   ++pc_; 
   t_ = 1; 
   if (read_count_ == 0)
   {
      current_address_ = pc_; 
      ++read_count_; 
   }
   else 
   {
      int nextcycle;
      REGW(reg) = current_data_ & 0xFFFF;
      NEXT_INSTR 
   }
   return 1;
}

template<Z80::Registers reg>
unsigned int Z80::MEMR_Read_REG_()
{
   int nextcycle;
   ++pc_;
   REG(reg) = current_data_ & 0xFF; 
   NEXT_INSTR
}


template<Z80::Registers reg, Z80::AddressRegisters regw> 
unsigned int Z80::MEMR_Read_REG_REGW()
{
   int nextcycle;
   REG(reg) = current_data_ & 0xFF; 
   mem_ptr_.w = REGW(regw) + 1;
   NEXT_INSTR;
}

template<bool positive, int cond>
unsigned int Z80::MEMR_JR_Cond()
{
   ++pc_;
   if ( (positive && ((af_.b.l & cond) == cond))
      ||(!positive && ((af_.b.l & cond) == 0))
      )
   //TSTN(ZF)
   {
      pc_ += ((char)(current_data_ & 0xFF)); 
      mem_ptr_.w = pc_; 
      machine_cycle_ = M_Z80_WAIT; 
      counter_ += (4);
      t_ = 1;
      return 5;

   }
   else 
   {
      int nextcycle;
      NEXT_INSTR 
   }
}

template<Z80::AddressRegisters regw>
unsigned int Z80::MEMR_Read_NN_HL()
{
   ++pc_;
   t_ = 1;
   if (read_count_ == 0) 
   {
      current_address_ = pc_; 
      ++read_count_; 
   }
   else 
   {
      machine_cycle_ = M_MEMORY_W; 
      t_ = 1; 
      current_address_ = current_data_; 
      current_data_ = REGW(regw); 
      read_count_ = 0; 
   }
   return 1;
}

template<Z80::AddressRegisters regw>
unsigned int Z80::MEMR_HL_NN_0()
{
   t_ = 1;
   ++pc_; 
   current_address_ = pc_;
   ++read_count_;
   (*(&memr_func_))[current_opcode_&0xFF] = &Z80::MEMR_HL_NN_1<regw>;
   return 1;
}

template<Z80::AddressRegisters regw>
unsigned int Z80::MEMR_HL_NN_1()
{
   t_ = 1;
   ++pc_; 
   current_address_ = current_data_ & 0xFFFF;
   ++read_count_;
   (*(&memr_func_))[current_opcode_ & 0xFF] = &Z80::MEMR_HL_NN_2<regw>;
   return 1;
}

template<Z80::AddressRegisters regw>
unsigned int Z80::MEMR_HL_NN_2()
{
   t_ = 1;
   if (regw == ADDR_AF) af_.b.l = data_;
   if (regw == ADDR_BC) bc_.b.l = data_;
   if (regw == ADDR_DE) de_.b.l = data_;
   if (regw == ADDR_HL) hl_.b.l = data_;
   if (regw == ADDR_IX) ix_.b.l = data_;
   if (regw == ADDR_IY) iy_.b.l = data_;
   ++read_count_;
   mem_ptr_.w = ++current_address_;
   (*(&memr_func_))[current_opcode_ & 0xFF] = &Z80::MEMR_HL_NN_3<regw>;
   return 1;
}

template<Z80::AddressRegisters regw>
unsigned int Z80::MEMR_HL_NN_3()
{
   int nextcycle;
   if (regw == ADDR_AF) af_.b.h = data_;
   if (regw == ADDR_BC) bc_.b.h = data_;
   if (regw == ADDR_DE) de_.b.h = data_;
   if (regw == ADDR_HL) hl_.b.h = data_;
   if (regw == ADDR_IX) ix_.b.h = data_;
   if (regw == ADDR_IY) iy_.b.h = data_;

   mem_ptr_.w = ++current_address_;
   (*(&memr_func_))[current_opcode_ & 0xFF] = &Z80::MEMR_HL_NN_0<regw>;
   NEXT_INSTR;
}

template<Z80::Registers reg>
unsigned int Z80::MEMR_Read_REG_NN()
{
   ++pc_;
   t_ = 1;
   if (read_count_ == 0)
   {
      current_address_ = pc_;
      ++read_count_;
   }
   else
   {
      current_address_ = current_data_;
      current_data_ = REG(reg);
      machine_cycle_ = M_MEMORY_W;
      read_count_ = 0;
   }
   return 1;
}

template<bool inc>
unsigned int Z80::MEMR_Inc_REGW()
{
   if (t_ == 4) {
      if (inc)
      {
         INC_FLAGS(data_);
      }
      else
      {
         DEC_FLAGS(data_);
      }
         
      current_data_ = data_;
      machine_cycle_ = M_MEMORY_W;
      t_ = 1;
      read_count_ = 0;
   }
   else
   {
      ++t_;
   };
   return 1;
}

template<Z80::AddressRegisters regw>
unsigned int Z80::MEMR_REGW_N()
{
   ++pc_; 
   t_ = 1; 
   current_address_ = REGW(regw);
   machine_cycle_ = M_MEMORY_W; 
   read_count_ = 0;
   return 1;
}

template<Z80::Registers reg>
unsigned int Z80::MEMR_Ld_Reg_Regw()
{
   int nextcycle;
   REG(reg) = current_data_ & 0xFF; 
   NEXT_INSTR;
}

template<bool add, bool Carry>
unsigned int Z80::Opcode_AddSub_Reg()
{
   int nextcycle;
   unsigned int res;
   if constexpr (add)
   {
      if constexpr (Carry)
      {
         ADD_FLAG_CARRY(data_);
      }
      else
      {
         ADD_FLAG(data_);
      }
   }
   else
   {
      if constexpr (Carry)
      {
         SUB_FLAG_CARRY(data_);
      }
      else
      {
         SUB_FLAG(data_);
      }
   }

   NEXT_INSTR
}

template<Z80::OperationType op>
unsigned int Z80::Opcode_BOOL_data()
{
   int nextcycle;
   if (op == AND)
   {
      af_.b.h &= data_;
      q_ = FlagAnd[af_.b.h];
   }
   else if (op == OR)
   {
      af_.b.h |= data_;
      q_ = FlagOr[af_.b.h];
   }
   else if (op == XOR)
   {
      af_.b.h ^= data_;
      q_ = FlagXor[af_.b.h];
   }
   af_.b.l = q_;
   NEXT_INSTR;
}

template<Z80::AddressRegisters regw>
unsigned int Z80::Opcode_Pop_Regw()
{
   if (read_count_ == 0) 
   {
      t_ = 1; 
      current_address_ = sp_++; 
      ++read_count_; 
      return 1;
   }
   else 
   {
      int nextcycle;
      REGW(regw) = current_data_ & 0xFFFF;
      NEXT_INSTR 
   }
}

template<bool positive, int cond>
unsigned int Z80::MEMR_JP_Cond()
{
   ++pc_; 
   t_ = 1; 
   if (read_count_ == 0) 
   {
      current_address_ = pc_; 
      ++read_count_;
      return 1;
   }
   else 
   {
      int nextcycle;
      mem_ptr_.w = current_data_;
      if ((positive && ((af_.b.l & cond) == cond))
         || (!positive && ((af_.b.l & cond) == 0))
         )
      {
         pc_ = current_data_; 
      }
      NEXT_INSTR 
   }
}

template<bool positive, int cond>
unsigned int Z80::MEMR_Call_Cond()
{
   ++pc_; 
   
   if (read_count_ == 0)
   {
      t_ = 1; 
      current_address_ = pc_; 
      ++read_count_; 
   }
   else 
   {
      mem_ptr_.w = current_data_; 
      if ((positive && ((af_.b.l & cond) == cond))
         || (!positive && ((af_.b.l & cond) == 0))
         )
      { 
         t_ = 1; 
         machine_cycle_ = M_Z80_WORK; 
      } 
      else 
      {
         int nextcycle;
         NEXT_INSTR 
      } 
   }
   return 1;
}


template<bool add, bool Carry>
unsigned int Z80::Opcode_AddSub_n()
{
   ++pc_;
   int nextcycle;
   unsigned int res;
   if constexpr (add)
   {
      if constexpr (Carry)
      {
         ADD_FLAG_CARRY(data_);
      }
      else
      {
         ADD_FLAG(data_);
      }
   }
   else
   {
      if constexpr (Carry)
      {
         SUB_FLAG_CARRY(data_);
      }
      else
      {
         SUB_FLAG(data_);
      }
   }

   NEXT_INSTR
}


#define WAIT_TEST if ((counter_&0x3) == 0)
template <int state>
unsigned int Z80::DefaultTick()
{
   int nextcycle;
   switch (state)
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
