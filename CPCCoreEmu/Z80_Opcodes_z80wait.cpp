#include "Z80_Full.h"



int Z80::OpcodeWAIT()
{
   unsigned char btmp;
   int nextcycle;

   
   switch (current_opcode_)
   {
   case 0xC4:machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = --sp_; current_data_ = pc_>>8;read_count_ = 0;break;  // CALL NZ nn
   case 0xCC:machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = --sp_; current_data_ = pc_>>8;read_count_ = 0;break;  // CALL Z nn
   case 0xD4:machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = --sp_; current_data_ = pc_>>8;read_count_ = 0;break;  // CALL NC nn
   case 0xDC:machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = --sp_; current_data_ = pc_>>8;read_count_ = 0;break;  // CALL C nn
   case 0xE4:machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = --sp_; current_data_ = pc_>>8;read_count_ = 0;break;  // CALL PO nn
   case 0xEC:machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = --sp_; current_data_ = pc_>>8;read_count_ = 0;break;  // CALL PE nn
   case 0xF4:machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = --sp_; current_data_ = pc_>>8;read_count_ = 0;break;  // CALL P nn
   case 0xFC:machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = --sp_; current_data_ = pc_>>8;read_count_ = 0;break;  // CALL M nn

   case 0xED67: {btmp = af_.b.h & 0x0F;unsigned char c = data_;unsigned char d = c & 0xF0;btmp=btmp<<4;d=d>>4;af_.b.h &=~(0xF);af_.b.h += c&0x0F;c = d + btmp;;machine_cycle_ = M_MEMORY_W;current_data_=c;read_count_=0;t_ = 1;break;} // RLD
   case 0xED6F: {btmp = af_.b.h & 0x0F;unsigned char c = data_;unsigned char d = c & 0xF0;d=d>>4;af_.b.h &=~(0xF);af_.b.h += d;c = c<<4;c+= btmp;machine_cycle_ = M_MEMORY_W;current_data_=c;read_count_=0;t_ = 1;break;} // RLD

   case 0xDD34:machine_cycle_ = M_MEMORY_R; read_count_ = 1; t_ = 1; current_address_ = mem_ptr_.w; break; // INC (IX+d)
   case 0xDD35:machine_cycle_ = M_MEMORY_R; read_count_ = 1; t_ = 1; current_address_ = mem_ptr_.w; break; // DEC (IX+d)
   case 0xDD46:mem_ptr_.w = ix_.w + (char)data_; machine_cycle_ = M_MEMORY_R; read_count_ = 1; t_ = 1; current_address_ = mem_ptr_.w; break; // LD B, (IY+d)
   case 0xDD4E:mem_ptr_.w = ix_.w + (char)data_; machine_cycle_ = M_MEMORY_R; read_count_ = 1; t_ = 1; current_address_ = mem_ptr_.w; break; // LD C, (IY+d)
   case 0xDD56:mem_ptr_.w = ix_.w + (char)data_; machine_cycle_ = M_MEMORY_R; read_count_ = 1; t_ = 1; current_address_ = mem_ptr_.w; break; // LD D, (IY+d)
   case 0xDD5E:mem_ptr_.w = ix_.w + (char)data_; machine_cycle_ = M_MEMORY_R; read_count_ = 1; t_ = 1; current_address_ = mem_ptr_.w; break; // LD E, (IY+d)
   case 0xDD66:mem_ptr_.w = ix_.w + (char)data_; machine_cycle_ = M_MEMORY_R; read_count_ = 1; t_ = 1; current_address_ = mem_ptr_.w; break; // LD H, (IY+d)
   case 0xDD6E:mem_ptr_.w = ix_.w + (char)data_; machine_cycle_ = M_MEMORY_R; read_count_ = 1; t_ = 1; current_address_ = mem_ptr_.w; break; // LD L, (IY+d)

   case 0xDD70:machine_cycle_ = M_MEMORY_W;current_data_=bc_.b.h;read_count_=0;t_ = 1; current_address_ = mem_ptr_.w;break; // LD (IX+d), B
   case 0xDD71:machine_cycle_ = M_MEMORY_W;current_data_=bc_.b.l;read_count_=0;t_ = 1; current_address_ = mem_ptr_.w;break; // LD (IX+d), C
   case 0xDD72:machine_cycle_ = M_MEMORY_W;current_data_=de_.b.h;read_count_=0;t_ = 1; current_address_ = mem_ptr_.w;break; // LD (IX+d), D
   case 0xDD73:machine_cycle_ = M_MEMORY_W;current_data_=de_.b.l;read_count_=0;t_ = 1; current_address_ = mem_ptr_.w;break; // LD (IX+d), E
   case 0xDD74:machine_cycle_ = M_MEMORY_W;current_data_=hl_.b.h;read_count_=0;t_ = 1; current_address_ = mem_ptr_.w;break; // LD (IX+d), H
   case 0xDD75:machine_cycle_ = M_MEMORY_W;current_data_=hl_.b.l;read_count_=0;t_ = 1; current_address_ = mem_ptr_.w;break; // LD (IX+d), L
   case 0xDD77:machine_cycle_ = M_MEMORY_W;current_data_=af_.b.h;read_count_=0;t_ = 1; current_address_ = mem_ptr_.w;break; // LD (IX+d), A

   case 0xDD7E:mem_ptr_.w = ix_.w + (char)data_; machine_cycle_ = M_MEMORY_R; read_count_ = 1; t_ = 1; current_address_ = mem_ptr_.w; break; // LD A, (IY+d)
   case 0xDD86:mem_ptr_.w = ix_.w+(char)data_;machine_cycle_ = M_MEMORY_R;read_count_=1;t_ = 1; current_address_ = mem_ptr_.w;break; // ADD A, (IX+d)
   case 0xDD8E:mem_ptr_.w = ix_.w+(char)data_;machine_cycle_ = M_MEMORY_R;read_count_=1;t_ = 1; current_address_ = mem_ptr_.w;break; // ADC A, (IX+d)
   case 0xDD96:machine_cycle_ = M_MEMORY_R;read_count_=1;t_ = 1; current_address_ = mem_ptr_.w;break; // SUB A, (IX+d)
   case 0xDD9E:machine_cycle_ = M_MEMORY_R;read_count_=1;t_ = 1; current_address_ = mem_ptr_.w;break; // SBC A, (IX+d)
   case 0xDDA6:machine_cycle_ = M_MEMORY_R;read_count_ = 1; t_ = 1; current_address_ = mem_ptr_.w; break;  //AND (IX+d)
   case 0xDDAE:machine_cycle_ = M_MEMORY_R; read_count_ = 1; t_ = 1; current_address_ = mem_ptr_.w; break; //XOR (IX+d)
   case 0xDDB6:machine_cycle_ = M_MEMORY_R; read_count_ = 1; t_ = 1; current_address_ = mem_ptr_.w; break; //OR (IX+n)
   case 0xDDBE:machine_cycle_ = M_MEMORY_R; read_count_ = 1; t_ = 1; current_address_ = mem_ptr_.w; break; //CP (IX+n)

   case 0xFD34:mem_ptr_.w = iy_.w + (char)data_; machine_cycle_ = M_MEMORY_R; read_count_ = 1; t_ = 1; current_address_ = mem_ptr_.w; break; // INC (IY+d)
   case 0xFD35:mem_ptr_.w = iy_.w + (char)data_; machine_cycle_ = M_MEMORY_R; read_count_ = 1; t_ = 1; current_address_ = mem_ptr_.w; break; // DEC (IY+d)
   case 0xFD46:mem_ptr_.w = iy_.w+(char)data_;machine_cycle_ = M_MEMORY_R;read_count_=1;t_ = 1; current_address_ = mem_ptr_.w;break; // LD B, (IY+d)
   case 0xFD4E:mem_ptr_.w = iy_.w+(char)data_;machine_cycle_ = M_MEMORY_R;read_count_=1;t_ = 1; current_address_ = mem_ptr_.w;break; // LD C, (IY+d)
   case 0xFD56:mem_ptr_.w = iy_.w+(char)data_;machine_cycle_ = M_MEMORY_R;read_count_=1;t_ = 1; current_address_ = mem_ptr_.w;break; // LD D, (IY+d)
   case 0xFD5E:mem_ptr_.w = iy_.w+(char)data_;machine_cycle_ = M_MEMORY_R;read_count_=1;t_ = 1; current_address_ = mem_ptr_.w;break; // LD E, (IY+d)
   case 0xFD66:mem_ptr_.w = iy_.w+(char)data_;machine_cycle_ = M_MEMORY_R;read_count_=1;t_ = 1; current_address_ = mem_ptr_.w;break; // LD H, (IY+d)
   case 0xFD6E:mem_ptr_.w = iy_.w+(char)data_;machine_cycle_ = M_MEMORY_R;read_count_=1;t_ = 1; current_address_ = mem_ptr_.w;break; // LD L, (IY+d)

   case 0xFD70:machine_cycle_ = M_MEMORY_W;current_data_=bc_.b.h;read_count_=0;t_ = 1; current_address_ = mem_ptr_.w;break; // LD (IY+d), B
   case 0xFD71:machine_cycle_ = M_MEMORY_W;current_data_=bc_.b.l;read_count_=0;t_ = 1; current_address_ = mem_ptr_.w;break; // LD (IY+d), C
   case 0xFD72:machine_cycle_ = M_MEMORY_W;current_data_=de_.b.h;read_count_=0;t_ = 1; current_address_ = mem_ptr_.w;break; // LD (IY+d), D
   case 0xFD73:machine_cycle_ = M_MEMORY_W;current_data_=de_.b.l;read_count_=0;t_ = 1; current_address_ = mem_ptr_.w;break; // LD (IY+d), E
   case 0xFD74:machine_cycle_ = M_MEMORY_W;current_data_=hl_.b.h;read_count_=0;t_ = 1; current_address_ = mem_ptr_.w;break; // LD (IY+d), H
   case 0xFD75:machine_cycle_ = M_MEMORY_W;current_data_=hl_.b.l;read_count_=0;t_ = 1; current_address_ = mem_ptr_.w;break; // LD (IY+d), L
   case 0xFD77:machine_cycle_ = M_MEMORY_W;current_data_=af_.b.h;read_count_=0;t_ = 1; current_address_ = mem_ptr_.w;break; // LD (IY+d), 
   case 0xFD7E:machine_cycle_ = M_MEMORY_R;read_count_=1;t_ = 1; current_address_ = mem_ptr_.w;break; // LD A, (IY+d)
   case 0xFD86:machine_cycle_ = M_MEMORY_R;read_count_=1;t_ = 1; current_address_ = mem_ptr_.w;break; // ADD A, (IY+d)
   case 0xFD8E:machine_cycle_ = M_MEMORY_R;read_count_=1;t_ = 1; current_address_ = mem_ptr_.w;break; // ADC A, (IY+d)
   case 0xFD96:machine_cycle_ = M_MEMORY_R;read_count_=1;t_ = 1; current_address_ = mem_ptr_.w;break; // SUB A, (IY+d)
   case 0xFD9E:machine_cycle_ = M_MEMORY_R;read_count_=1;t_ = 1; current_address_ = mem_ptr_.w;break; // SBC A, (IY+d)
   case 0xFDA6:machine_cycle_ = M_MEMORY_R;read_count_ = 1; t_ = 1; current_address_ = mem_ptr_.w; break;  //AND (IY+d)
   case 0xFDAE:machine_cycle_ = M_MEMORY_R;read_count_ = 1; t_ = 1; current_address_ = mem_ptr_.w; break;  //XOR (IY+d)
   case 0xFDB6:machine_cycle_ = M_MEMORY_R;read_count_=1;t_ = 1; current_address_ = mem_ptr_.w; break;     //OR (IY+n)
   case 0xFDBE:machine_cycle_ = M_MEMORY_R; read_count_ = 1; t_ = 1; current_address_ = mem_ptr_.w; break; //CP (IY+n)
   default:
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
