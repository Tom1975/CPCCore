#include "Z80_Full.h"

int Z80::OpcodeMEMW()
{
   unsigned char btmp; unsigned short utmp;
   int nextcycle;
   
   switch (current_opcode_)
   {
   case 0x02: mem_ptr_.b.l = ((bc_.w+1)&0xff);mem_ptr_.b.h = (af_.b.h);NEXT_INSTR;break;  // LD (BC), A
   case 0x12: mem_ptr_.b.l = ((de_.w+1)&0xff);mem_ptr_.b.h = (af_.b.h);NEXT_INSTR;break;  // LD (DE), A
   case 0x22: if ( read_count_ == 0) {++read_count_ ;++current_address_;current_data_=hl_.b.h;t_=1;}else{mem_ptr_.w = current_address_;NEXT_INSTR};break; // LD ( nn), HL
   case 0x32: mem_ptr_.b.l = ((current_address_+1)&0xff);mem_ptr_.b.h = af_.b.h;NEXT_INSTR;break;  // LN (nn), A
   case 0x34: NEXT_INSTR;break;  // INC (HL)
   case 0x35: NEXT_INSTR;break;  // INC (HL)
   case 0x36: NEXT_INSTR;break;  // LD (HL), n 
   case 0x70: NEXT_INSTR;break;  // LD (HL), B
   case 0x71: NEXT_INSTR;break;  // LD (HL), C
   case 0x72: NEXT_INSTR;break;  // LD (HL), D
   case 0x73: NEXT_INSTR;break;  // LD (HL), E
   case 0x74: NEXT_INSTR;break;  // LD (HL), H
   case 0x75: NEXT_INSTR;break;  // LD (HL), L
   case 0x77: NEXT_INSTR;break;  // LD (HL), A
   case 0xC4: if(read_count_++ == 0){ t_ = 1; current_address_ = --sp_; current_data_ = pc_&0xFF;}else{pc_ = mem_ptr_.w;NEXT_INSTR;};break; //CALL cc, nn
   case 0xC5: if(read_count_++ == 0){ t_ = 1; current_address_ = --sp_; current_data_ = bc_.b.l;}else{NEXT_INSTR;};break; // PUSH BC
   case 0xC7: if(read_count_++ == 0){ t_ = 1; current_address_ = --sp_; current_data_ = pc_&0xFF;}else{pc_ = 0x00;mem_ptr_.w = pc_;NEXT_INSTR;};break; //RST &00
   case 0xCC: if(read_count_++ == 0){ t_ = 1; current_address_ = --sp_; current_data_ = pc_&0xFF;}else{pc_ = mem_ptr_.w;NEXT_INSTR;};break; //CALL Z nn
   case 0xCD: if(read_count_++ == 0){ t_ = 1; current_address_ = --sp_; current_data_ = ((pc_+2)&0xFF);}else{machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;};break; // CALL nn
   case 0xCF: if(read_count_++ == 0){ t_ = 1; current_address_ = --sp_; current_data_ = pc_&0xFF;}else{pc_ = 0x08;mem_ptr_.w = pc_;NEXT_INSTR;};break; //RST &08
   case 0xD4: if(read_count_++ == 0){ t_ = 1; current_address_ = --sp_; current_data_ = pc_&0xFF;}else{pc_ = mem_ptr_.w;NEXT_INSTR;};break; //CALL NC nn
   case 0xD5: if(read_count_++ == 0){ t_ = 1; current_address_ = --sp_; current_data_ = de_.b.l;}else{NEXT_INSTR;};break; // PUSH DE
   case 0xD7: if(read_count_++ == 0){ t_ = 1; current_address_ = --sp_; current_data_ = pc_&0xFF;}else{pc_ = 0x10;mem_ptr_.w = pc_;NEXT_INSTR;};break; //RST &10
   case 0xDC: if(read_count_++ == 0){ t_ = 1; current_address_ = --sp_; current_data_ = pc_&0xFF;}else{pc_ = mem_ptr_.w;NEXT_INSTR;};break; //CALL C nn
   case 0xDF: if(read_count_++ == 0){ t_ = 1; current_address_ = --sp_; current_data_ = pc_&0xFF;}else{pc_ = 0x18;mem_ptr_.w = pc_;NEXT_INSTR;};break; //RST &18
   case 0xE3: if(read_count_++ == 0){ t_ = 1; current_address_ = sp_; current_data_ = hl_.b.l;}else{if ( t_ == 5){hl_.w = mem_ptr_.w;NEXT_INSTR;}else{++t_;}};break; // EX(SP), HL
   case 0xE4: if(read_count_++ == 0){ t_ = 1; current_address_ = --sp_; current_data_ = pc_&0xFF;}else{pc_ = mem_ptr_.w;NEXT_INSTR;};break; //CALL PO nn
   case 0xE5: if(read_count_++ == 0){ t_ = 1; current_address_ = --sp_; current_data_ = hl_.b.l;}else{NEXT_INSTR;};break; // PUSH HL
   case 0xE7: if(read_count_++ == 0){ t_ = 1; current_address_ = --sp_; current_data_ = pc_&0xFF;}else{pc_ = 0x20;mem_ptr_.w = pc_;NEXT_INSTR;};break; //RST &20
   case 0xEC: if(read_count_++ == 0){ t_ = 1; current_address_ = --sp_; current_data_ = pc_&0xFF;}else{pc_ = mem_ptr_.w;NEXT_INSTR;};break; //CALL PE nn
   case 0xEF: if(read_count_++ == 0){ t_ = 1; current_address_ = --sp_; current_data_ = pc_&0xFF;}else{pc_ = 0x28;mem_ptr_.w = pc_;NEXT_INSTR;};break; //RST &28
   case 0xF4: if(read_count_++ == 0){ t_ = 1; current_address_ = --sp_; current_data_ = pc_&0xFF;}else{pc_ = mem_ptr_.w;NEXT_INSTR;};break; //CALL P nncc, nn
   case 0xF5: if(read_count_++ == 0){ t_ = 1; current_address_ = --sp_; current_data_ = af_.b.l;}else{NEXT_INSTR;};break; // PUSH AF
   case 0xF7: if(read_count_++ == 0){ t_ = 1; current_address_ = --sp_; current_data_ = pc_&0xFF;}else{pc_ = 0x30;mem_ptr_.w = pc_;NEXT_INSTR;};break; //RST &30
   case 0xFC: if(read_count_++ == 0){ t_ = 1; current_address_ = --sp_; current_data_ = pc_&0xFF;}else{pc_ = mem_ptr_.w;NEXT_INSTR;};break; //CALL M, nn
   case 0xFF: if(read_count_++ == 0){ t_ = 1; current_address_ = --sp_; current_data_ = pc_&0xFF;}else{pc_ = 0x38;mem_ptr_.w = pc_;NEXT_INSTR;};break; //RST &38


   case 0xCB06: NEXT_INSTR; break; // 
   case 0xCB0E: NEXT_INSTR; break; // 
   case 0xCB16: NEXT_INSTR; break; // 
   case 0xCB1E: NEXT_INSTR; break; // 
   case 0xCB26: NEXT_INSTR; break; // 
   case 0xCB2E: NEXT_INSTR; break; // 
   case 0xCB36: NEXT_INSTR; break; // 
   case 0xCB3E: NEXT_INSTR; break; // 

   case 0xCB86: NEXT_INSTR; break; // RES 0, (HL)
   case 0xCB8E: NEXT_INSTR; break; // RES 1, (HL)
   case 0xCB96: NEXT_INSTR; break; // RES 2, (HL)
   case 0xCB9E: NEXT_INSTR; break; // RES 3, (HL)
   case 0xCBA6: NEXT_INSTR; break; // RES 4, (HL)
   case 0xCBAE: NEXT_INSTR; break; // RES 5, (HL)
   case 0xCBB6: NEXT_INSTR; break; // RES 6, (HL)
   case 0xCBBE: NEXT_INSTR; break; // RES 7, (HL)

   case 0xCBC6: NEXT_INSTR; break; // SET 0, (HL)
   case 0xCBCE: NEXT_INSTR; break; // SET 1, (HL)
   case 0xCBD6: NEXT_INSTR; break; // SET 2, (HL)
   case 0xCBDE: NEXT_INSTR; break; // SET 3, (HL)
   case 0xCBE6: NEXT_INSTR; break; // SET 4, (HL)
   case 0xCBEE: NEXT_INSTR; break; // SET 5, (HL)
   case 0xCBF6: NEXT_INSTR; break; // SET 6, (HL)
   case 0xCBFE: NEXT_INSTR; break; // SET 7, (HL)
   case 0xDD22: if (read_count_++ == 0) { t_ = 1; current_address_ = ++mem_ptr_.w; current_data_ = ix_.w >> 8; }else{NEXT_INSTR;}break; // LD (nn), IX
   case 0xDD34: NEXT_INSTR; break; // INC (IX+d)
   case 0xDD35: NEXT_INSTR; break; // DEC (IX+d)
   case 0xDD36: NEXT_INSTR; break; // LD (IX+d), n
   case 0xDD70: NEXT_INSTR; break; // LD (IX+d), B
   case 0xDD71: NEXT_INSTR; break; // LD (IX+d), C
   case 0xDD72: NEXT_INSTR; break; // LD (IX+d), D
   case 0xDD73: NEXT_INSTR; break; // LD (IX+d), E
   case 0xDD74: NEXT_INSTR; break; // LD (IX+d), H
   case 0xDD75: NEXT_INSTR; break; // LD (IX+d), L
   case 0xDD77: NEXT_INSTR; break; // LD (IX+d), A
   case 0xDDE3: if(read_count_++ == 0){ t_ = 1; current_address_ = sp_; current_data_ = ix_.b.l;}else{if ( t_ == 5){ix_.w = mem_ptr_.w;NEXT_INSTR;}else{++t_;}};break; // EX(SP), IX
   case 0xDDE5: if (read_count_++ == 0) { t_ = 1; current_address_ = --sp_; current_data_ = ix_.w & 0xFF; }else { NEXT_INSTR; }; break; // PUSH IX


   case 0xED43:if ( read_count_ == 0) {++read_count_ ;++current_address_;current_data_=bc_.b.h;t_=1;}else{mem_ptr_.w = current_address_;NEXT_INSTR};break; // LD ( nn), BC
   case 0xED53:if ( read_count_ == 0) {++read_count_ ;++current_address_;current_data_=de_.b.h;t_=1;}else{mem_ptr_.w = current_address_;NEXT_INSTR};break; // LD ( nn), DE
   case 0xED63:if ( read_count_ == 0) {++read_count_ ;++current_address_;current_data_=hl_.b.h;t_=1;}else{mem_ptr_.w = current_address_;NEXT_INSTR};break; // LD ( nn), HL
   case 0xED67: q_ = af_.b.l&CF;if ((af_.b.h & 0x80) == 0x80) q_ |= SF; if (af_.b.h == 00) q_ |= ZF; q_ |= (af_.b.h&0x28);PARITY_FLAG(af_.b.h);af_.b.l = q_;mem_ptr_.w = hl_.w+1;NEXT_INSTR;break; // RRD
   case 0xED6F: q_ = af_.b.l&CF;if ((af_.b.h & 0x80) == 0x80) q_ |= SF; if (af_.b.h == 00) q_ |= ZF; q_ |= (af_.b.h&0x28);PARITY_FLAG(af_.b.h);af_.b.l = q_;mem_ptr_.w = hl_.w+1;NEXT_INSTR;break;    //RLD
   case 0xED73:if ( read_count_ == 0) {++read_count_ ;++current_address_;current_data_=sp_>>8;t_=1;}else{mem_ptr_.w = current_address_;NEXT_INSTR};break; // LD ( nn), SP

   case 0xEDA0:if(t_ == 5)
   {
      q_ = af_.b.l& (~(HF|NF|0x28));++de_.w;++hl_.w;--bc_.w;if (bc_.w == 0 )
      {q_ &= ~(PF);}else{q_ |= PF;}
      btmp = current_data_+af_.b.h;if ((btmp&0x2) == 0x2 ) q_ |= 0x20;
      if ((btmp&0x8) == 0x8 ) q_ |= 0x8;
      af_.b.l = q_;NEXT_INSTR;
   }else{++t_;}break; // LDI
   case 0xEDA2: {mem_ptr_.w = bc_.w + 1; --bc_.b.h; ++hl_.w; utmp = (data_ + ((bc_.b.l + 1) & 0xFF));
      q_ = af_.b.l; if (utmp > 0xFF) q_ |= (CF | HF); utmp = ((utmp & 7) ^ bc_.b.h); PARITY_FLAG(utmp);
      if (data_ & 0x80) q_ |= NF; else q_ &= ~NF;
      if (bc_.b.h == 0) q_ |= ZF; else q_ &= ~(ZF); af_.b.l = q_; NEXT_INSTR;
   }break; // INI

   case 0xEDA8:if (t_ == 5) {
      q_ = af_.b.l& (~(HF | NF | 0x28 | PF)); --de_.w; --hl_.w; --bc_.w; if (bc_.w) q_ |= PF; btmp = current_data_ + af_.b.h; if ((btmp & 0x2) == 0x2) q_ |= YF; if ((btmp & 0x8) == 0x8) q_ |= XF; af_.b.l = q_; NEXT_INSTR ;}
               else { ++t_; }break; // LDD
   case 0xEDAA: {mem_ptr_.w = bc_.w -1; --bc_.b.h; --hl_.w; utmp = (data_ + ((bc_.b.l - 1) & 0xFF));
      q_ = af_.b.l; if (utmp > 0xFF) q_ |= (CF | HF); utmp = ((utmp & 7) ^ bc_.b.h); PARITY_FLAG(utmp);
      if (data_ & 0x80) q_ |= NF; else q_ &= ~NF;
      if (bc_.b.h == 0) q_ |= ZF; else q_ &= ~(ZF); af_.b.l = q_; NEXT_INSTR;
   }break; // IND
   case 0xEDB0:if(t_ == 5){++de_.w;++hl_.w;--bc_.w;btmp = current_data_+af_.b.h;q_ = af_.b.l&~(0x28);if ((btmp&0x2) == 0x2 ) q_ |= 0x20;if ((btmp&0x8) == 0x8 ) q_ |= 0x8;
      if ( bc_.w == 0 ){q_ &= ~(HF|PF|NF);af_.b.l = q_;NEXT_INSTR} else{ af_.b.l = q_;pc_ -= 2;mem_ptr_.w = pc_+1;machine_cycle_=M_Z80_WORK;t_=5;}
               }else{++t_;};break; // LDIR
   case 0xEDB2: {mem_ptr_.w = bc_.w + 1; --bc_.b.h; ++hl_.w; utmp = (data_ + ((bc_.b.l + 1) & 0xFF));
      q_ = af_.b.l; if (utmp > 0xFF) q_ |= (CF | HF); utmp = ((utmp & 7) ^ bc_.b.h); PARITY_FLAG(utmp);
      if (data_ & 0x80) q_ |= NF; else q_ &= ~NF; q_ |= ZF; af_.b.l = q_;
      if (bc_.b.h == 0) { NEXT_INSTR }else { pc_ -= 2; machine_cycle_ = M_Z80_WORK; t_ = 5;};
   }break; // INIR

   case 0xEDB8:if(t_ == 5){--de_.w;--hl_.w;--bc_.w;btmp = data_+af_.b.h;q_ = af_.b.l;q_ &= ~(0x28);if ((btmp&0x2) == 0x2 ) q_ |= 0x20;   if ((btmp&0x8) == 0x8 ) q_ |= 0x8;
      if ( bc_.w == 0 ){q_ &= ~(HF|PF|NF);af_.b.l = q_;NEXT_INSTR} else { af_.b.l = q_;pc_ -= 2;mem_ptr_.w = pc_+1;machine_cycle_=M_Z80_WORK;t_=5;}
               }else{++t_;};break; // LDDR
   case 0xEDBA: {mem_ptr_.w = bc_.w + 1; --bc_.b.h; --hl_.w; utmp = (data_ + ((bc_.b.l - 1) & 0xFF));
      q_ = af_.b.l; if (utmp > 0xFF) q_ |= (CF | HF); utmp = ((utmp & 7) ^ bc_.b.h); PARITY_FLAG(utmp);
      if (bc_.b.h == 0) { af_.b.l = q_; NEXT_INSTR }
      else {if (data_ & 0x80) q_ |= NF; else q_ &= ~NF;q_ |= ZF;af_.b.l = q_; pc_ -= 2; machine_cycle_ = M_Z80_WORK; t_ = 5; };
   }break; // INDR
   case 0xFD22: if (read_count_++ == 0) { t_ = 1; current_address_ = ++mem_ptr_.w; current_data_ = iy_.w >> 8; }else{NEXT_INSTR;}break; // LD (nn), IY
   case 0xFD34: NEXT_INSTR; break; // INC (IY+d)
   case 0xFD35: NEXT_INSTR; break; // DEC (IY+d)
   case 0xFD36: NEXT_INSTR; break; // LD (IY+d), n
   case 0xFDE5: if (read_count_++ == 0) { t_ = 1; current_address_ = --sp_; current_data_ = iy_.w & 0xFF; }
                else { NEXT_INSTR; }; break; // PUSH IX


   case 0xFD70: NEXT_INSTR; break; // LD (IY+d), B
   case 0xFD71: NEXT_INSTR; break; // LD (IY+d), C
   case 0xFD72: NEXT_INSTR; break; // LD (IY+d), D
   case 0xFD73: NEXT_INSTR; break; // LD (IY+d), E
   case 0xFD74: NEXT_INSTR; break; // LD (IY+d), H
   case 0xFD75: NEXT_INSTR; break; // LD (IY+d), L
   case 0xFD77: NEXT_INSTR; break; // LD (IY+d), A
   case 0xFDE3: if(read_count_++ == 0){ t_ = 1; current_address_ = sp_; current_data_ = iy_.b.l;}else{if ( t_ == 5){iy_.w = mem_ptr_.w;NEXT_INSTR;}else{++t_;}};break; // EX(SP), IY

   case 0xFF02: if (read_count_++ == 0) { t_ = 1; current_address_ = --sp_; current_data_ = pc_ & 0xFF; }else 
               {
      machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = mem_ptr_.w; current_data_ = 0; read_count_ = 0;
   }; break;// Int MODE2

   case 0xFF12: if (read_count_++ == 0) { t_ = 1; current_address_ = --sp_; current_data_ = pc_ & 0xFF; }
                else
                {
                   /*current_address_ = mem_ptr_.w;*/ pc_ = 0x66; NEXT_INSTR;
                }; break;// NMI

   case 0xDDCB00: 

   default:
      //_CrtDbgBreak(); 
      NEXT_INSTR;
      break;

   }
   if (machine_cycle_ == M_Z80_WORK)
   {
      int ret = t_;
      counter_ += (ret - 1);
      t_ = 1;
      return ret;
   }
   return 1;
}
