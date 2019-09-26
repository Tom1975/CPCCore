#include "Z80_Full.h"

unsigned int Z80::OpcodeMEMR()
{
   unsigned int res; unsigned char btmp;
   int nextcycle;

   switch (current_opcode_)
   {
   case 0xBE: CP_FLAGS(data_); NEXT_INSTR; break;// CP (HL)
   case 0xC0: if (read_count_ == 0) { t_ = 1; current_address_ = sp_++; ++read_count_; }
              else { pc_ = current_data_ & 0xFFFF; mem_ptr_.w = pc_; NEXT_INSTR }; break;  // RET NZ
   case 0xC1: if (read_count_ == 0) { t_ = 1; current_address_ = sp_++; ++read_count_; }
              else { bc_.w = current_data_ & 0xFFFF; NEXT_INSTR }; break;  // POP BC
   case 0xC2: ++pc_; t_ = 1; if (read_count_ == 0) { current_address_ = pc_; ++read_count_; }
              else { mem_ptr_.w = current_data_; TSTN(ZF) { pc_ = current_data_; }NEXT_INSTR }break;// JP NZ nn
   case 0xC3: ++pc_; t_ = 1; if (read_count_ == 0) { current_address_ = pc_; ++read_count_; }
              else { pc_ = current_data_; mem_ptr_.w = pc_; NEXT_INSTR }break;// JP nn
   case 0xC4: ++pc_; t_ = 1; if (read_count_ == 0) { current_address_ = pc_; ++read_count_; }
              else { mem_ptr_.w = current_data_; TSTN(ZF) { t_ = 1; machine_cycle_ = M_Z80_WORK; } else { NEXT_INSTR } }break;// CALL NZ
   case 0xC6: ++pc_; ADD_FLAG(current_data_); NEXT_INSTR; break;                                                                                    // ADD A,n
   case 0xC8: if (read_count_ == 0) { t_ = 1; current_address_ = sp_++; ++read_count_; }
              else { pc_ = current_data_ & 0xFFFF; mem_ptr_.w = pc_; NEXT_INSTR }; break;  // RET Z
   case 0xC9: if (read_count_ == 0) { t_ = 1; current_address_ = sp_++; ++read_count_; }
              else { pc_ = current_data_ & 0xFFFF; mem_ptr_.w = pc_; NEXT_INSTR }; break;  // RET
   case 0xCA: ++pc_; t_ = 1; if (read_count_ == 0) { current_address_ = pc_; ++read_count_; }
              else { mem_ptr_.w = current_data_; TST(ZF) { pc_ = current_data_; }NEXT_INSTR }break; // JP Z nn
   case 0xCC: ++pc_; t_ = 1; if (read_count_ == 0) { current_address_ = pc_; ++read_count_; }
              else { mem_ptr_.w = current_data_; TST(ZF) { t_ = 1; machine_cycle_ = M_Z80_WORK; } else { NEXT_INSTR } }break;// CALL Z nn
   case 0xCD: ++pc_; t_ = 1; if (read_count_ == 0) { current_address_ = pc_; ++read_count_; }
              else
              {
                 TraceTape(pc_, hl_.b.l);
                 pc_ = current_data_; mem_ptr_.w = pc_; NEXT_INSTR
              }break;   // CALL nn
   case 0xCE: ++pc_; ADD_FLAG_CARRY(current_data_); NEXT_INSTR; break;                                                                                    // ADC A,n
   case 0xD0: if (read_count_ == 0) { t_ = 1; current_address_ = sp_++; ++read_count_; }
              else { pc_ = current_data_ & 0xFFFF; mem_ptr_.w = pc_; NEXT_INSTR }; break;  // RET NC
   case 0xD1: if (read_count_ == 0) { t_ = 1; current_address_ = sp_++; ++read_count_; }
              else { de_.w = current_data_ & 0xFFFF; NEXT_INSTR }; break;  // POP DE
   case 0xD2: ++pc_; t_ = 1; if (read_count_ == 0) { current_address_ = pc_; ++read_count_; }
              else { mem_ptr_.w = current_data_; TSTN(CF) { pc_ = current_data_; }NEXT_INSTR }break;// JP NC nn
   case 0xD3: ++pc_; q_ = af_.b.l; q_ |= (data_ & 0x80) ? NF : 0; af_.b.l = q_; mem_ptr_.b.l = data_; mem_ptr_.b.h = af_.b.h; machine_cycle_ = M_IO_W; t_ = 1; current_address_ = mem_ptr_.w;mem_ptr_.b.l++; current_data_ = af_.b.h; break; // OUT (n), A
   case 0xD4: ++pc_; t_ = 1; if (read_count_ == 0) { current_address_ = pc_; ++read_count_; }
              else { mem_ptr_.w = current_data_; TSTN(CF) { t_ = 1; machine_cycle_ = M_Z80_WORK; } else { NEXT_INSTR } }break;// CALL NC nn
   case 0xD6: ++pc_; SUB_FLAG(data_); NEXT_INSTR; break; // SUB n
   case 0xD8: if (read_count_ == 0) { t_ = 1; current_address_ = sp_++; ++read_count_; }
              else { pc_ = current_data_ & 0xFFFF; mem_ptr_.w = pc_; NEXT_INSTR }; break;  // RET C
   case 0xDA: ++pc_; t_ = 1; if (read_count_ == 0) { current_address_ = pc_; ++read_count_; }
              else { mem_ptr_.w = current_data_; TST(CF) { pc_ = current_data_; }NEXT_INSTR }break;// JP C nn
   case 0xDB: ++pc_; mem_ptr_.b.l = data_; mem_ptr_.b.h = af_.b.h; machine_cycle_ = M_IO_R; t_ = 1; current_address_ = mem_ptr_.w++; current_data_ = 0; break; // IN (n), A
   case 0xDC: ++pc_; t_ = 1; if (read_count_ == 0) { current_address_ = pc_; ++read_count_; }
              else { mem_ptr_.w = current_data_; TST(CF) { t_ = 1; machine_cycle_ = M_Z80_WORK; } else { NEXT_INSTR } }break;// CALL C nn
   case 0xDE: ++pc_; SUB_FLAG_CARRY(data_); NEXT_INSTR; break; // SBC n
   case 0xE0: if (read_count_ == 0) { t_ = 1; current_address_ = sp_++; ++read_count_; }
              else { pc_ = current_data_ & 0xFFFF; mem_ptr_.w = pc_; NEXT_INSTR }; break;  // RET PO
   case 0xE1: if (read_count_ == 0) { t_ = 1; current_address_ = sp_++; ++read_count_; }
              else { hl_.w = current_data_ & 0xFFFF; NEXT_INSTR }; break;  // POP HL
   case 0xE2: ++pc_; t_ = 1; if (read_count_ == 0) { current_address_ = pc_; ++read_count_; }
              else { mem_ptr_.w = current_data_; TSTN(PF) { pc_ = current_data_; }NEXT_INSTR }break;// JP PO nn
   case 0xE3: if (read_count_ == 0) { t_ = 1; current_address_ = sp_ + 1; ++read_count_; }
              else { if (t_ == 4) { mem_ptr_.w = current_data_ & 0xFFFF; machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = sp_+1; current_data_ = hl_.b.h; read_count_ = 0; } else { ++t_; } }; break;// EX (SP), HL
   case 0xE4: ++pc_; t_ = 1; if (read_count_ == 0) { current_address_ = pc_; ++read_count_; }
              else { mem_ptr_.w = current_data_; TSTN(PF) { t_ = 1; machine_cycle_ = M_Z80_WORK; } else { NEXT_INSTR } }break;// CALL PO nn
   case 0xE6: {++pc_; t_ = 1; AND_FLAGS(current_data_ & 0xFF); NEXT_INSTR; break; } // AND n
   case 0xE8: if (read_count_ == 0) { t_ = 1; current_address_ = sp_++; ++read_count_; }
              else { pc_ = current_data_ & 0xFFFF; mem_ptr_.w = pc_; NEXT_INSTR }; break;  // RET PE
   case 0xEA: ++pc_; t_ = 1; if (read_count_ == 0) { current_address_ = pc_; ++read_count_; }
              else { mem_ptr_.w = current_data_; TST(PF) { pc_ = current_data_; }NEXT_INSTR }break;// JP PE nn
   case 0xEC: ++pc_; t_ = 1; if (read_count_ == 0) { current_address_ = pc_; ++read_count_; }
              else { mem_ptr_.w = current_data_; TST(PF) { t_ = 1; machine_cycle_ = M_Z80_WORK; } else { NEXT_INSTR } }break;// CALL PE nn
   case 0xEE: {++pc_; t_ = 1; XOR_FLAGS(current_data_ & 0xFF); NEXT_INSTR; break; } // AND n
   case 0xF0: if (read_count_ == 0) { t_ = 1; current_address_ = sp_++; ++read_count_; }
              else { pc_ = current_data_ & 0xFFFF; mem_ptr_.w = pc_; NEXT_INSTR }; break;  // RET P
   case 0xF1: if (read_count_ == 0) { t_ = 1; current_address_ = sp_++; ++read_count_; }
              else { af_.w = current_data_ & 0xFFFF; NEXT_INSTR }; break;  // POP DEAF
   case 0xF2: ++pc_; t_ = 1; if (read_count_ == 0) { current_address_ = pc_; ++read_count_; }
              else { mem_ptr_.w = current_data_; TSTN(SF) { pc_ = current_data_; }NEXT_INSTR }break;// JP P nn
   case 0xF4: ++pc_; t_ = 1; if (read_count_ == 0) { current_address_ = pc_; ++read_count_; }
              else { mem_ptr_.w = current_data_; TSTN(SF) { t_ = 1; machine_cycle_ = M_Z80_WORK; } else { NEXT_INSTR } }break;// CALL P nn
   case 0xF6: ++pc_; OR_FLAGS(current_data_); NEXT_INSTR; break;                                                                                    // OR n
   case 0xF8: if (read_count_ == 0) { t_ = 1; current_address_ = sp_++; ++read_count_; }
              else { pc_ = current_data_ & 0xFFFF; mem_ptr_.w = pc_; NEXT_INSTR }; break;  // RET M
   case 0xFA: ++pc_; t_ = 1; if (read_count_ == 0) { current_address_ = pc_; ++read_count_; }
              else { mem_ptr_.w = current_data_; TST(SF) { pc_ = current_data_; }NEXT_INSTR }break; // JP M nn
   case 0xFC: ++pc_; t_ = 1; if (read_count_ == 0) { current_address_ = pc_; ++read_count_; }
              else { mem_ptr_.w = current_data_; TST(SF) { t_ = 1; machine_cycle_ = M_Z80_WORK; } else { NEXT_INSTR } }break;// CALL M
   case 0xFE: ++pc_; CP_FLAGS(data_); NEXT_INSTR; break;// break;

   case 0xCB06: if (t_ == 4) { RLC(data_); current_data_ = data_; read_count_ = 0; machine_cycle_ = M_MEMORY_W; t_ = 1; }
                else { ++t_; }; break; // RLC (HL)
   case 0xCB0E: if (t_ == 4) { RRC(data_); current_data_ = data_; read_count_ = 0; machine_cycle_ = M_MEMORY_W; t_ = 1; }
                else { ++t_; }; break; // RRC (HL)
   case 0xCB16: if (t_ == 4) { RL(data_); current_data_ = data_; read_count_ = 0; machine_cycle_ = M_MEMORY_W; t_ = 1; }
                else { ++t_; }; break; // RL (HL)
   case 0xCB1E: if (t_ == 4) { RR(data_); current_data_ = data_; read_count_ = 0; machine_cycle_ = M_MEMORY_W; t_ = 1; }
                else { ++t_; }; break; // RR (HL)
   case 0xCB26: if (t_ == 4) { SLA(data_); current_data_ = data_; read_count_ = 0; machine_cycle_ = M_MEMORY_W; t_ = 1; }
                else { ++t_; }; break; // SLA (HL)
   case 0xCB2E: if (t_ == 4) { SRA(data_); current_data_ = data_; read_count_ = 0; machine_cycle_ = M_MEMORY_W; t_ = 1; }
                else { ++t_; }; break; // SRA (HL)
   case 0xCB36: if (t_ == 4) { SLL(data_); current_data_ = data_; read_count_ = 0; machine_cycle_ = M_MEMORY_W; t_ = 1; }
                else { ++t_; }; break; // SLL (HL)
   case 0xCB3E: if (t_ == 4) { SRL(data_); current_data_ = data_; read_count_ = 0; machine_cycle_ = M_MEMORY_W; t_ = 1; }
                else { ++t_; }; break; // SRL (HL)

   case 0xCB46: if (t_ == 4) { ; q_ = (af_.b.l & CF) | HF | (SzBit[data_ & (1 << 0)] & ~(YF | XF)) | (mem_ptr_.b.h & (YF | XF)); af_.b.l = q_; NEXT_INSTR; }
                else { ++t_; }; break; // BIT 0, (HL)
   case 0xCB4E: if (t_ == 4) { ; q_ = (af_.b.l & CF) | HF | (SzBit[data_ & (1 << 1)] & ~(YF | XF)) | (mem_ptr_.b.h & (YF | XF)); af_.b.l = q_; NEXT_INSTR; }
                else { ++t_; }; break; // BIT 1, (HL)
   case 0xCB56: if (t_ == 4) { ; q_ = (af_.b.l & CF) | HF | (SzBit[data_ & (1 << 2)] & ~(YF | XF)) | (mem_ptr_.b.h & (YF | XF)); af_.b.l = q_; NEXT_INSTR; }
                else { ++t_; }; break; // BIT 2, (HL)
   case 0xCB5E: if (t_ == 4) { ; q_ = (af_.b.l & CF) | HF | (SzBit[data_ & (1 << 3)] & ~(YF | XF)) | (mem_ptr_.b.h & (YF | XF)); af_.b.l = q_; NEXT_INSTR; }
                else { ++t_; }; break; // BIT 3, (HL)
   case 0xCB66: if (t_ == 4) { ; q_ = (af_.b.l & CF) | HF | (SzBit[data_ & (1 << 4)] & ~(YF | XF)) | (mem_ptr_.b.h & (YF | XF)); af_.b.l = q_; NEXT_INSTR; }
                else { ++t_; }; break; // BIT 4, (HL)
   case 0xCB6E: if (t_ == 4) { ; q_ = (af_.b.l & CF) | HF | (SzBit[data_ & (1 << 5)] & ~(YF | XF)) | (mem_ptr_.b.h & (YF | XF)); af_.b.l = q_; NEXT_INSTR; }
                else { ++t_; }; break; // BIT 5, (HL)
   case 0xCB76: if (t_ == 4) { ; q_ = (af_.b.l & CF) | HF | (SzBit[data_ & (1 << 6)] & ~(YF | XF)) | (mem_ptr_.b.h & (YF | XF)); af_.b.l = q_; NEXT_INSTR; }
                else { ++t_; }; break; // BIT 6, (HL)
   case 0xCB7E: if (t_ == 4) { ; q_ = (af_.b.l & CF) | HF | (SzBit[data_ & (1 << 7)] & ~(YF | XF)) | (mem_ptr_.b.h & (YF | XF)); af_.b.l = q_; NEXT_INSTR; }
                else { ++t_; }; break; // BIT 7, (HL)

   case 0xCB86: if (t_ == 4) { current_data_ &= ~0x01; read_count_ = 0; machine_cycle_ = M_MEMORY_W; t_ = 1; }
                else { ++t_; }; break; // RES 0, (HL)
   case 0xCB8E: if (t_ == 4) { current_data_ &= ~0x02; read_count_ = 0; machine_cycle_ = M_MEMORY_W; t_ = 1; }
                else { ++t_; }; break; // RES 1, (HL)
   case 0xCB96: if (t_ == 4) { current_data_ &= ~0x04; read_count_ = 0; machine_cycle_ = M_MEMORY_W; t_ = 1; }
                else { ++t_; }; break; // RES 2, (HL)
   case 0xCB9E: if (t_ == 4) { current_data_ &= ~0x08; read_count_ = 0; machine_cycle_ = M_MEMORY_W; t_ = 1; }
                else { ++t_; }; break; // RES 3, (HL)
   case 0xCBA6: if (t_ == 4) { current_data_ &= ~0x10; read_count_ = 0; machine_cycle_ = M_MEMORY_W; t_ = 1; }
                else { ++t_; }; break; // RES 4, (HL)
   case 0xCBAE: if (t_ == 4) { current_data_ &= ~0x20; read_count_ = 0; machine_cycle_ = M_MEMORY_W; t_ = 1; }
                else { ++t_; }; break; // RES 5, (HL)
   case 0xCBB6: if (t_ == 4) { current_data_ &= ~0x40; read_count_ = 0; machine_cycle_ = M_MEMORY_W; t_ = 1; }
                else { ++t_; }; break; // RES 6, (HL)
   case 0xCBBE: if (t_ == 4) { current_data_ &= ~0x80; read_count_ = 0; machine_cycle_ = M_MEMORY_W; t_ = 1; }
                else { ++t_; }; break; // RES 7, (HL)

   case 0xCBC6: if (t_ == 4) { current_data_ |= 0x01; read_count_ = 0; machine_cycle_ = M_MEMORY_W; t_ = 1; }
                else { ++t_; }; break; // SET 0, (HL)
   case 0xCBCE: if (t_ == 4) { current_data_ |= 0x02; read_count_ = 0; machine_cycle_ = M_MEMORY_W; t_ = 1; }
                else { ++t_; }; break; // SET 1, (HL)
   case 0xCBD6: if (t_ == 4) { current_data_ |= 0x04; read_count_ = 0; machine_cycle_ = M_MEMORY_W; t_ = 1; }
                else { ++t_; }; break; // SET 2, (HL)
   case 0xCBDE: if (t_ == 4) { current_data_ |= 0x08; read_count_ = 0; machine_cycle_ = M_MEMORY_W; t_ = 1; }
                else { ++t_; }; break; // SET 3, (HL)
   case 0xCBE6: if (t_ == 4) { current_data_ |= 0x10; read_count_ = 0; machine_cycle_ = M_MEMORY_W; t_ = 1; }
                else { ++t_; }; break; // SET 4, (HL)
   case 0xCBEE: if (t_ == 4) { current_data_ |= 0x20; read_count_ = 0; machine_cycle_ = M_MEMORY_W; t_ = 1; }
                else { ++t_; }; break; // SET 5, (HL)
   case 0xCBF6: if (t_ == 4) { current_data_ |= 0x40; read_count_ = 0; machine_cycle_ = M_MEMORY_W; t_ = 1; }
                else { ++t_; }; break; // SET 6, (HL)
   case 0xCBFE: if (t_ == 4) { current_data_ |= 0x80; read_count_ = 0; machine_cycle_ = M_MEMORY_W; t_ = 1; }
                else { ++t_; }; break; // SET 7, (HL)

   case 0xDD21: ++pc_; t_ = 1; if (read_count_ == 0) { current_address_ = pc_; ++read_count_; }
                else { ix_.w = current_data_ & 0xFFFF; NEXT_INSTR }break;// LD_IX, nn
   case 0xDD22: ++pc_; t_ = 1; if (read_count_ == 0) { current_address_ = pc_; ++read_count_; }
                else { mem_ptr_.w = current_data_ & 0xFFFF; machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = mem_ptr_.w; current_data_ = ix_.w & 0xFF; read_count_ = 0; }break;// LD (nn), IX
   case 0xDD26: ++pc_; ix_.b.h = data_; NEXT_INSTR; break;                                                                      // LD IXh, n
   case 0xDD2A: t_ = 1; switch (read_count_) {
   case 0: { ++pc_; current_address_ = pc_; ++read_count_; break; }
   case 1: { ++pc_; current_address_ = current_data_ & 0xFFFF; ++read_count_; break; }
   case 2: { ix_.b.l = data_; ++read_count_; mem_ptr_.w = ++current_address_; break; }
   case 3: { ix_.b.h = data_; NEXT_INSTR; }
   }; break; //  LD IX, (nn)
   case 0xDD2E: ++pc_; ix_.b.l = data_; NEXT_INSTR; break;                                                                      // LD IXl, n
   case 0xDD34: if (read_count_ == 0) { ++pc_; mem_ptr_.w = ix_.w + (char)data_; t_ = 5; machine_cycle_ = M_Z80_WORK; }
                else { INC_FLAGS(data_); current_data_ = data_; machine_cycle_ = M_MEMORY_W; t_ = 1; read_count_ = 0; }break; // INC (IX+d)
   case 0xDD35: if (read_count_ == 0) { ++pc_; mem_ptr_.w = ix_.w + (char)data_; t_ = 5; machine_cycle_ = M_Z80_WORK; }
                else { DEC_FLAGS(data_); current_data_ = data_; machine_cycle_ = M_MEMORY_W; t_ = 1; read_count_ = 0; }break; // INC (IX+d)
   case 0xDD36: if (read_count_ == 0) { ++pc_; ++read_count_;  mem_ptr_.w = ix_.w + (char)data_; t_ = 1; current_address_ = pc_; }
                else { if (t_ == 5) { ++pc_; t_ = 1; current_address_ = mem_ptr_.w; current_data_ = data_; read_count_ = 0; machine_cycle_ = M_MEMORY_W; } else { ++t_; } }break; // LD (IX+d), n
   case 0xDD46: if (read_count_ == 0) { ++pc_; mem_ptr_.w = ix_.w + (char)data_; t_ = 5; machine_cycle_ = M_Z80_WORK; }
                else { bc_.b.h = data_; NEXT_INSTR; }break; // LD B, (IX+d)
   case 0xDD4E: if (read_count_ == 0) { ++pc_; mem_ptr_.w = ix_.w + (char)data_; t_ = 5; machine_cycle_ = M_Z80_WORK; }
                else { bc_.b.l = data_; NEXT_INSTR; }break; // LD C, (IX+d)
   case 0xDD56: if (read_count_ == 0) { ++pc_; mem_ptr_.w = ix_.w + (char)data_; t_ = 5; machine_cycle_ = M_Z80_WORK; }
                else { de_.b.h = data_; NEXT_INSTR; }break; // LD D, (IX+d)
   case 0xDD5E: if (read_count_ == 0) { ++pc_; mem_ptr_.w = ix_.w + (char)data_; t_ = 5; machine_cycle_ = M_Z80_WORK; }
                else { de_.b.l = data_; NEXT_INSTR; }break; // LD E, (IX+d)
   case 0xDD66: if (read_count_ == 0) { ++pc_; mem_ptr_.w = ix_.w + (char)data_; t_ = 5; machine_cycle_ = M_Z80_WORK; }
                else { hl_.b.h = data_; NEXT_INSTR; }break; // LD H, (IX+d)
   case 0xDD6E: if (read_count_ == 0) { ++pc_; mem_ptr_.w = ix_.w + (char)data_; t_ = 5; machine_cycle_ = M_Z80_WORK; }
                else { hl_.b.l = data_; NEXT_INSTR; }break; // LD L, (IX+d)
   case 0xDD70: ++pc_; mem_ptr_.w = ix_.w + (char)data_; t_ = 5; machine_cycle_ = M_Z80_WORK; break; // LD (IX+d), B
   case 0xDD71: ++pc_; mem_ptr_.w = ix_.w + (char)data_; t_ = 5; machine_cycle_ = M_Z80_WORK; break; // LD (IX+d), C
   case 0xDD72: ++pc_; mem_ptr_.w = ix_.w + (char)data_; t_ = 5; machine_cycle_ = M_Z80_WORK; break; // LD (IX+d), D
   case 0xDD73: ++pc_; mem_ptr_.w = ix_.w + (char)data_; t_ = 5; machine_cycle_ = M_Z80_WORK; break; // LD (IX+d), E
   case 0xDD74: ++pc_; mem_ptr_.w = ix_.w + (char)data_; t_ = 5; machine_cycle_ = M_Z80_WORK; break; // LD (IX+d), H
   case 0xDD75: ++pc_; mem_ptr_.w = ix_.w + (char)data_; t_ = 5; machine_cycle_ = M_Z80_WORK; break; // LD (IX+d), L
   case 0xDD77: ++pc_; mem_ptr_.w = ix_.w + (char)data_; t_ = 5; machine_cycle_ = M_Z80_WORK; break; // LD (IX+d), A
   case 0xDD7E: if (read_count_ == 0) { ++pc_; mem_ptr_.w = ix_.w + (char)data_; t_ = 5; machine_cycle_ = M_Z80_WORK; }
                else { af_.b.h = data_; NEXT_INSTR; }break; // LD A, (IX+d)
   case 0xDD86: if (read_count_ == 0) { ++pc_; mem_ptr_.w = ix_.w + (char)data_; t_ = 5; machine_cycle_ = M_Z80_WORK; }
                else { ADD_FLAG(data_); NEXT_INSTR; }break; // ADD A, (IX+d)
   case 0xDD8E: if (read_count_ == 0) { ++pc_; mem_ptr_.w = ix_.w + (char)data_; t_ = 5; machine_cycle_ = M_Z80_WORK; }
                else { ADD_FLAG_CARRY(data_); NEXT_INSTR; }break; // ACD A, (IX+d)
   case 0xDD96: if (read_count_ == 0) { ++pc_; mem_ptr_.w = ix_.w + (char)data_; t_ = 5; machine_cycle_ = M_Z80_WORK; }
                else { SUB_FLAG(data_); NEXT_INSTR; }break;         // SUB A, (IX+d)
   case 0xDD9E: if (read_count_ == 0) { ++pc_; mem_ptr_.w = ix_.w + (char)data_; t_ = 5; machine_cycle_ = M_Z80_WORK; }
                else { SUB_FLAG_CARRY(data_); NEXT_INSTR; }break;   // SBC A, (IX+d)
   case 0xDDA6: if (read_count_ == 0) { ++pc_; mem_ptr_.w = ix_.w + (char)data_; t_ = 5; machine_cycle_ = M_Z80_WORK; }
                else { AND_FLAGS(data_); NEXT_INSTR; }break;// AND (IX+d)
   case 0xDDAE: if (read_count_ == 0) { ++pc_; mem_ptr_.w = ix_.w + (char)data_; t_ = 5; machine_cycle_ = M_Z80_WORK; }
                else { XOR_FLAGS(data_); NEXT_INSTR; }break;// XOR (IX+d)
   case 0xDDB6: if (read_count_ == 0) { ++pc_; mem_ptr_.w = ix_.w + (char)data_; t_ = 5; machine_cycle_ = M_Z80_WORK; }
                else { OR_FLAGS(data_); NEXT_INSTR; }break; // OR (IX+n)
   case 0xDDBE: if (read_count_ == 0) { ++pc_; mem_ptr_.w = ix_.w + (char)data_; t_ = 5; machine_cycle_ = M_Z80_WORK; }
                else { CP_FLAGS(data_); NEXT_INSTR; }break; // CP (IX+n)

   case 0xDDCB: if (read_count_ == 0)
   {
      ++pc_; mem_ptr_.w = ix_.w + (char)data_; t_ = 1; ++read_count_; ++current_address_;
   }
                else
                { /* Opcode to handle is i*/
                   if (t_ == 5)
                   {
                      ++pc_;
                      current_address_ = mem_ptr_.w;
                      t_ = 1; read_count_ = 0;
                      current_opcode_ <<= 8;
                      current_opcode_ |= data_;
                   }
                   else { ++t_; }
                }
                break;// DDCB : Read +d, then opcode

   case 0xDDE1: if (read_count_ == 0) { t_ = 1; current_address_ = sp_++; ++read_count_; }
                else { ix_.w = current_data_ & 0xFFFF; NEXT_INSTR }; break;// POP IX
   case 0xDDE3: if (read_count_ == 0) { t_ = 1; current_address_ = sp_ + 1; ++read_count_; }
                else { if (t_ == 4) { mem_ptr_.w = current_data_ & 0xFFFF; machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = sp_+1; current_data_ = ix_.b.h; read_count_ = 0; } else { ++t_; } }; break;// EX (SP), IX

   case 0xED4B: switch (read_count_) {
   case 0:++read_count_; ++pc_; t_ = 1; current_address_ = pc_; break;
   case 1:++read_count_; ++pc_; t_ = 1; current_address_ = current_data_; mem_ptr_.w = current_address_ + 1; break;
   case 2:++read_count_; t_ = 1; bc_.b.l = data_; ++current_address_; break;
   case 3: bc_.b.h = data_; NEXT_INSTR; break;
   }break;// LD BC, (nn)

   case 0xED5B: switch (read_count_) {
   case 0:++read_count_; ++pc_; t_ = 1; current_address_ = pc_; break;
   case 1:++read_count_; ++pc_; t_ = 1; current_address_ = current_data_; mem_ptr_.w = current_address_ + 1; break;
   case 2:++read_count_; t_ = 1; de_.b.l = data_; ++current_address_; break;
   case 3: de_.b.h = data_; NEXT_INSTR; break;
   }break;// LD DE, (nn)
   case 0xED43: ++pc_; t_ = 1; if (read_count_ == 0) { current_address_ = pc_; ++read_count_; }
                else { machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = current_data_; current_data_ = bc_.b.l; read_count_ = 0; }break;// LD (NN), BC
   case 0xED45: if (read_count_ == 0) { t_ = 1; current_address_ = sp_++; ++read_count_; }
                else { pc_ = current_data_ & 0xFFFF; mem_ptr_.w = pc_; iff1_ = iff2_;  NEXT_INSTR }; break;
   case 0xED4D: if (read_count_ == 0) { t_ = 1; current_address_ = sp_++; ++read_count_; }
                else { pc_ = current_data_ & 0xFFFF; mem_ptr_.w = pc_; NEXT_INSTR }; break;  // RET
   case 0xED53: ++pc_; t_ = 1; if (read_count_ == 0) { current_address_ = pc_; ++read_count_; }
                else { machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = current_data_; current_data_ = de_.b.l; read_count_ = 0; }break;// LD (NN), DE
   case 0xED55: if (read_count_ == 0) { t_ = 1; current_address_ = sp_++; ++read_count_; }
                else { pc_ = current_data_ & 0xFFFF; mem_ptr_.w = pc_; iff1_ = iff2_;  NEXT_INSTR }; break;
   case 0xED5D: if (read_count_ == 0) { t_ = 1; current_address_ = sp_++; ++read_count_; }
                else { pc_ = current_data_ & 0xFFFF; mem_ptr_.w = pc_; iff1_ = iff2_;  NEXT_INSTR }; break;
   case 0xED63: ++pc_; t_ = 1; if (read_count_ == 0) { current_address_ = pc_; ++read_count_; }
                else { machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = current_data_; current_data_ = hl_.b.l; read_count_ = 0; }break;// LD (NN), HL
   case 0xED65: if (read_count_ == 0) { t_ = 1; current_address_ = sp_++; ++read_count_; }
                else { pc_ = current_data_ & 0xFFFF; mem_ptr_.w = pc_; iff1_ = iff2_;  NEXT_INSTR }; break;
   case 0xED67: t_ = 5; machine_cycle_ = M_Z80_WORK; break; // RRD;
   case 0xED6B: switch (read_count_) {
   case 0:++read_count_; ++pc_; t_ = 1; current_address_ = pc_; break;
   case 1:++read_count_; ++pc_; t_ = 1; current_address_ = current_data_; mem_ptr_.w = current_address_ + 1; break;
   case 2:++read_count_; t_ = 1; hl_.b.l = data_; ++current_address_; break;
   case 3: hl_.b.h = data_; NEXT_INSTR; break;
   }break;// LD HL, (nn)
   case 0xED6D: if (read_count_ == 0) { t_ = 1; current_address_ = sp_++; ++read_count_; }
                else { pc_ = current_data_ & 0xFFFF; mem_ptr_.w = pc_; iff1_ = iff2_;  NEXT_INSTR }; break;
   case 0xED6F: t_ = 5; machine_cycle_ = M_Z80_WORK; break; // RLD;
   case 0xED73: ++pc_; t_ = 1; if (read_count_ == 0) { current_address_ = pc_; ++read_count_; }
                else { machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = current_data_; current_data_ = sp_ & 0xFF; read_count_ = 0; }break;// LD (NN), SP
   case 0xED75: if (read_count_ == 0) { t_ = 1; current_address_ = sp_++; ++read_count_; }
                else { pc_ = current_data_ & 0xFFFF; mem_ptr_.w = pc_; iff1_ = iff2_;  NEXT_INSTR }; break;
   case 0xED7B: switch (read_count_) {
   case 0:++read_count_; ++pc_; t_ = 1; current_address_ = pc_; break;
   case 1:++read_count_; ++pc_; t_ = 1; current_address_ = current_data_; mem_ptr_.w = current_address_ + 1; break;
   case 2:++read_count_; t_ = 1; sp_ = data_; ++current_address_; break;
   case 3: sp_ |= (data_ << 8); NEXT_INSTR; break;
   }break;// LD SP, (nn)
   case 0xED7D: if (read_count_ == 0) { t_ = 1; current_address_ = sp_++; ++read_count_; }
                else { pc_ = current_data_ & 0xFFFF; mem_ptr_.w = pc_; iff1_ = iff2_;  NEXT_INSTR }; break;

   case 0xEDA0: machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = de_.w; current_data_; read_count_ = 0; break; // LDI
   case 0xEDA1: CPD_FLAGS(data_); ++hl_.w; --bc_.w; ++mem_ptr_.w; machine_cycle_ = M_Z80_WORK; t_ = 5; break;     // CPI
   case 0xEDA3:  --bc_.b.h; mem_ptr_.w = bc_.w+1; machine_cycle_ = M_IO_W; t_ = 1; current_address_ = bc_.w; break;// OUTI
   case 0xEDA8: machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = de_.w; current_data_; read_count_ = 0; break; // LDD

   case 0xEDA9: CPD_FLAGS(data_); --hl_.w; --bc_.w; --mem_ptr_.w; machine_cycle_ = M_Z80_WORK; t_ = 5; break;      // CPD
   case 0xEDAB: mem_ptr_.w = bc_.w; --bc_.b.h; machine_cycle_ = M_IO_W; t_ = 1; current_address_ = bc_.w; break;// OUTD

   case 0xEDB0: machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = de_.w; current_data_;read_count_ = 0;break;   // LDIR
   case 0xEDB1: CPD_FLAGS(data_); ++hl_.w; --bc_.w; if ((af_.b.h == data_) || (bc_.w == 0)) { ++mem_ptr_.w; machine_cycle_ = M_Z80_WORK; t_ = 5; }
                else { pc_ -= 2; mem_ptr_.w = pc_ + 1; machine_cycle_ = M_Z80_WORK; t_ = 5 + 5; }break;// CPIR
   case 0xEDB3: --bc_.b.h; mem_ptr_.w = bc_.w+1; machine_cycle_ = M_IO_W; t_ = 1; current_address_ = bc_.w; current_data_ = data_; break;// OTIR

   case 0xEDB8: machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = de_.w; current_data_;read_count_ = 0;break;   // LDDR
   case 0xEDB9: CPD_FLAGS(data_);--hl_.w;--bc_.w; if ((af_.b.h == data_) || (bc_.w == 0)) { --mem_ptr_.w; machine_cycle_ = M_Z80_WORK; t_ = 5; }
                else { pc_ -= 2; mem_ptr_.w = pc_ + 1; machine_cycle_ = M_Z80_WORK; t_ = 5 + 5; }break;// CPDR
   case 0xEDBB: mem_ptr_.w = bc_.w; --bc_.b.h; machine_cycle_ = M_IO_W; t_ = 1; current_address_ = bc_.w; current_data_ = data_; break;// OTDR
   case 0xFD21: ++pc_; t_ = 1; if (read_count_ == 0) { current_address_ = pc_; ++read_count_; }else { iy_.w = current_data_ & 0xFFFF; NEXT_INSTR }break;// LD_IY, nn
   case 0xFD22: ++pc_; t_ = 1; if (read_count_ == 0) { current_address_ = pc_; ++read_count_; }else { mem_ptr_.w = current_data_ & 0xFFFF;machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = mem_ptr_.w; current_data_=iy_.w&0xFF;read_count_ = 0; }break;// LD (nn), IY
   case 0xFD26: ++pc_; iy_.b.h = data_ ;NEXT_INSTR;break;                                                                      // LD IYh, n
   case 0xFD2A: t_=1;switch (read_count_){
      case 0: { ++pc_;current_address_ = pc_;++read_count_;break;}
      case 1: { ++pc_;current_address_ =current_data_&0xFFFF;++read_count_;break;}
      case 2: { iy_.b.l = data_;++read_count_;mem_ptr_.w= ++current_address_;break;}
      case 3: { iy_.b.h = data_;NEXT_INSTR;}};break; //  LD IY, (nn)
   case 0xFD2E: ++pc_; iy_.b.l = data_ ;NEXT_INSTR;break;                                                                      // LD IYl, n
   case 0xFD34: if (read_count_ == 0){++pc_; t_=5;machine_cycle_=M_Z80_WORK;}else{ INC_FLAGS(data_);current_data_=data_;machine_cycle_ = M_MEMORY_W; t_ = 1;read_count_=0;}break; // INC (IY+d)
   case 0xFD35: if (read_count_ == 0){++pc_; t_=5;machine_cycle_=M_Z80_WORK;}else{ DEC_FLAGS(data_);current_data_=data_;machine_cycle_ = M_MEMORY_W; t_ = 1;read_count_=0;}break; // DEC (IY+d)
   case 0xFD36: if (read_count_ == 0) { ++pc_; ++read_count_;  mem_ptr_.w = iy_.w + (char)data_; t_ = 1; current_address_ = pc_; }else { if (t_ == 5) { ++pc_; t_ = 1; current_address_ = mem_ptr_.w; current_data_ = data_; read_count_ = 0; machine_cycle_ = M_MEMORY_W; } else { ++t_; } }break; // LD (IY+d), n
   case 0xFD46: if (read_count_ == 0){++pc_; mem_ptr_.w = iy_.w+(char)data_;t_=5;machine_cycle_=M_Z80_WORK;}else{ bc_.b.h = data_;NEXT_INSTR;}break; // LD B, (IY+d)
   case 0xFD4E: if (read_count_ == 0){++pc_; mem_ptr_.w = iy_.w+(char)data_;t_=5;machine_cycle_=M_Z80_WORK;}else{ bc_.b.l = data_;NEXT_INSTR;}break; // LD C, (IY+d)
   case 0xFD56: if (read_count_ == 0){++pc_; mem_ptr_.w = iy_.w+(char)data_;t_=5;machine_cycle_=M_Z80_WORK;}else{ de_.b.h = data_;NEXT_INSTR;}break; // LD D, (IY+d)
   case 0xFD5E: if (read_count_ == 0){++pc_; mem_ptr_.w = iy_.w+(char)data_;t_=5;machine_cycle_=M_Z80_WORK;}else{ de_.b.l = data_;NEXT_INSTR;}break; // LD E, (IY+d)
   case 0xFD66: if (read_count_ == 0){++pc_; mem_ptr_.w = iy_.w+(char)data_;t_=5;machine_cycle_=M_Z80_WORK;}else{ hl_.b.h = data_;NEXT_INSTR;}break; // LD H, (IY+d)
   case 0xFD6E: if (read_count_ == 0){++pc_; mem_ptr_.w = iy_.w+(char)data_;t_=5;machine_cycle_=M_Z80_WORK;}else{ hl_.b.l = data_;NEXT_INSTR;}break; // LD L, (IY+d)
   case 0xFD70: ++pc_; mem_ptr_.w = iy_.w+(char)data_;t_=5;machine_cycle_=M_Z80_WORK;break; // LD (IY+d), B
   case 0xFD71: ++pc_; mem_ptr_.w = iy_.w+(char)data_;t_=5;machine_cycle_=M_Z80_WORK;break; // LD (IY+d), C
   case 0xFD72: ++pc_; mem_ptr_.w = iy_.w+(char)data_;t_=5;machine_cycle_=M_Z80_WORK;break; // LD (IY+d), D
   case 0xFD73: ++pc_; mem_ptr_.w = iy_.w+(char)data_;t_=5;machine_cycle_=M_Z80_WORK;break; // LD (IY+d), E
   case 0xFD74: ++pc_; mem_ptr_.w = iy_.w+(char)data_;t_=5;machine_cycle_=M_Z80_WORK;break; // LD (IY+d), H
   case 0xFD75: ++pc_; mem_ptr_.w = iy_.w+(char)data_;t_=5;machine_cycle_=M_Z80_WORK;break; // LD (IY+d), L
   case 0xFD77: ++pc_; mem_ptr_.w = iy_.w+(char)data_;t_=5;machine_cycle_=M_Z80_WORK;break; // LD (IY+d), A
   case 0xFD7E: if (read_count_ == 0){++pc_; mem_ptr_.w = iy_.w+(char)data_;t_=5;machine_cycle_=M_Z80_WORK;}else{ af_.b.h = data_;NEXT_INSTR;}break; // LD A, (IY+d)
   case 0xFD86: if (read_count_ == 0){++pc_; mem_ptr_.w = iy_.w+(char)data_;t_=5;machine_cycle_=M_Z80_WORK;}else{ ADD_FLAG(data_);NEXT_INSTR;}break; // ADD A, (IY+d)
   case 0xFD8E: if (read_count_ == 0){++pc_; mem_ptr_.w = iy_.w+(char)data_;t_=5;machine_cycle_=M_Z80_WORK;}else{ ADD_FLAG_CARRY(data_);NEXT_INSTR;}break; // ADC A, (IY+d)
   case 0xFD96: if (read_count_ == 0){++pc_; mem_ptr_.w = iy_.w+(char)data_;t_=5;machine_cycle_=M_Z80_WORK;}else{ SUB_FLAG(data_);NEXT_INSTR;}break; // SUB A, (IY+d)
   case 0xFD9E: if (read_count_ == 0){++pc_; mem_ptr_.w = iy_.w+(char)data_;t_=5;machine_cycle_=M_Z80_WORK;}else{ SUB_FLAG_CARRY(data_);NEXT_INSTR;}break; // SBC A, (IY+d)
   case 0xFDA6: if (read_count_ == 0) { ++pc_; mem_ptr_.w = iy_.w + (char)data_; t_ = 5; machine_cycle_ = M_Z80_WORK; }else { AND_FLAGS(data_); NEXT_INSTR; }break;// AND (IY+d)
   case 0xFDAE: if (read_count_ == 0) { ++pc_; mem_ptr_.w = iy_.w + (char)data_; t_ = 5; machine_cycle_ = M_Z80_WORK; }else { XOR_FLAGS(data_); NEXT_INSTR; }break;// XOR (IY+d)
   case 0xFDB6: if (read_count_ == 0) { ++pc_; mem_ptr_.w = iy_.w + (char)data_; t_ = 5; machine_cycle_ = M_Z80_WORK; }else { OR_FLAGS(data_); NEXT_INSTR; }break; // OR (IX+n)
   case 0xFDBE: if (read_count_ == 0) { ++pc_; mem_ptr_.w = iy_.w + (char)data_; t_ = 5; machine_cycle_ = M_Z80_WORK; }else { CP_FLAGS(data_); NEXT_INSTR; }break; // CP (IYX+n)
   case 0xFDCB: if (read_count_ == 0)
                {
                   ++pc_; mem_ptr_.w = iy_.w + (char)data_; t_ = 1;++read_count_;++current_address_;
                }else
                { /* Opcode to handle is i*/
                   if ( t_ == 5)
                   {
                      ++pc_;
                      current_address_ = mem_ptr_.w;
                      t_ = 1;read_count_=0;
                      current_opcode_<<=8;
                      current_opcode_|=data_;
                      current_data_ = 0;
                   }
                   else{++t_;}
                }
                break;// FDCB : Read +d, then opcode
   case 0xFDE1: if (read_count_ == 0) { t_ = 1; current_address_ = sp_++; ++read_count_; }else { iy_.w = current_data_ & 0xFFFF; NEXT_INSTR }; break;// POP IY
   case 0xFDE3: if (read_count_ == 0){t_ = 1; current_address_ = sp_+1; ++read_count_;} else { if ( t_ == 4){mem_ptr_.w=current_data_&0xFFFF;machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = sp_+1; current_data_ = iy_.b.h;read_count_ = 0;}else{++t_;}};break;// EX (SP), IY


   #define DDCB_MACRO(macro,r) macro(data_);r=data_;t_ = 1;current_data_ = data_;machine_cycle_ = M_MEMORY_W;
   #define DDCB_MACRO_(macro) macro(data_);t_ = 1;current_data_ = data_;machine_cycle_ = M_MEMORY_W;
   #define DDCB_MACRO_BIT(macro,b) macro(b, data_);

   #define DDCB_MACRO_RS_(macro,b) macro(data_,b);t_ = 1;current_data_ = data_;machine_cycle_ = M_MEMORY_W;
   #define DDCB_MACRO_RS(macro,b,r) macro(data_,b);r=data_;t_ = 1;current_data_ = data_;machine_cycle_ = M_MEMORY_W;

   #define DDCB_MACRO_LIST_BIT(base,macro,bit)\
   case base+0:if ( t_ == 4){DDCB_MACRO_BIT(macro,bit);NEXT_INSTR;}else{++t_;}break;\
   case base+1:if ( t_ == 4){DDCB_MACRO_BIT(macro,bit);NEXT_INSTR;}else{++t_;}break;\
   case base+2:if ( t_ == 4){DDCB_MACRO_BIT(macro,bit);NEXT_INSTR;}else{++t_;}break;\
   case base+3:if ( t_ == 4){DDCB_MACRO_BIT(macro,bit);NEXT_INSTR;}else{++t_;}break;\
   case base+4:if ( t_ == 4){DDCB_MACRO_BIT(macro,bit);NEXT_INSTR;}else{++t_;}break;\
   case base+5:if ( t_ == 4){DDCB_MACRO_BIT(macro,bit);NEXT_INSTR;}else{++t_;}break;\
   case base+6:if ( t_ == 4){DDCB_MACRO_BIT(macro,bit);NEXT_INSTR;}else{++t_;}break;\
   case base+7:if ( t_ == 4){DDCB_MACRO_BIT(macro,bit);NEXT_INSTR;}else{++t_;}break;

   #define DDCB_MACRO_LIST(base,macro)\
   case base+0:if ( t_ == 4){DDCB_MACRO(macro,bc_.b.h);}else{++t_;}break;\
   case base+1:if ( t_ == 4){DDCB_MACRO(macro,bc_.b.l);}else{++t_;}break;\
   case base+2:if ( t_ == 4){DDCB_MACRO(macro,de_.b.h);}else{++t_;}break;\
   case base+3:if ( t_ == 4){DDCB_MACRO(macro,de_.b.l);}else{++t_;}break;\
   case base+4:if ( t_ == 4){DDCB_MACRO(macro,hl_.b.h);}else{++t_;}break;\
   case base+5:if ( t_ == 4){DDCB_MACRO(macro,hl_.b.l);}else{++t_;}break;\
   case base+6:if ( t_ == 4){DDCB_MACRO_(macro)   ;}else{++t_;}break;\
   case base+7:if ( t_ == 4){DDCB_MACRO(macro,af_.b.h);}else{++t_;}break;

   #define DDCB_MACRO_LIST_RS(base,macro,bit)\
   case base+0:if ( t_ == 4){DDCB_MACRO_RS(macro,bit,bc_.b.h);}else{++t_;}break;\
   case base+1:if ( t_ == 4){DDCB_MACRO_RS(macro,bit,bc_.b.l);}else{++t_;}break;\
   case base+2:if ( t_ == 4){DDCB_MACRO_RS(macro,bit,de_.b.h);}else{++t_;}break;\
   case base+3:if ( t_ == 4){DDCB_MACRO_RS(macro,bit,de_.b.l);}else{++t_;}break;\
   case base+4:if ( t_ == 4){DDCB_MACRO_RS(macro,bit,hl_.b.h);}else{++t_;}break;\
   case base+5:if ( t_ == 4){DDCB_MACRO_RS(macro,bit,hl_.b.l);}else{++t_;}break;\
   case base+6:if ( t_ == 4){DDCB_MACRO_RS_(macro,bit);}else{++t_;}break;\
   case base+7:if ( t_ == 4){DDCB_MACRO_RS(macro,bit,af_.b.h);}else{++t_;}break;


   DDCB_MACRO_LIST(0xDDCB00, RLC)
   DDCB_MACRO_LIST(0xDDCB08, RRC)
   DDCB_MACRO_LIST(0xDDCB10, RL)
   DDCB_MACRO_LIST(0xDDCB18, RR)
   DDCB_MACRO_LIST(0xDDCB20, SLA)
   DDCB_MACRO_LIST(0xDDCB28, SRA)
   DDCB_MACRO_LIST(0xDDCB30, SLL)
   DDCB_MACRO_LIST(0xDDCB38, SRL)
   // BIT
   DDCB_MACRO_LIST_BIT(0xDDCB40, BIT_B_R_DD, 0)
   DDCB_MACRO_LIST_BIT(0xDDCB48, BIT_B_R_DD, 1)
   DDCB_MACRO_LIST_BIT(0xDDCB50, BIT_B_R_DD, 2)
   DDCB_MACRO_LIST_BIT(0xDDCB58, BIT_B_R_DD, 3)
   DDCB_MACRO_LIST_BIT(0xDDCB60, BIT_B_R_DD, 4)
   DDCB_MACRO_LIST_BIT(0xDDCB68, BIT_B_R_DD, 5)
   DDCB_MACRO_LIST_BIT(0xDDCB70, BIT_B_R_DD, 6)
   DDCB_MACRO_LIST_BIT(0xDDCB78, BIT_B_R_DD, 7)

   // RES
   DDCB_MACRO_LIST_RS(0xDDCB80,RES,0)
   DDCB_MACRO_LIST_RS(0xDDCB88,RES,1)
   DDCB_MACRO_LIST_RS(0xDDCB90,RES,2)
   DDCB_MACRO_LIST_RS(0xDDCB98,RES,3)
   DDCB_MACRO_LIST_RS(0xDDCBA0,RES,4)
   DDCB_MACRO_LIST_RS(0xDDCBA8,RES,5)
   DDCB_MACRO_LIST_RS(0xDDCBB0,RES,6)
   DDCB_MACRO_LIST_RS(0xDDCBB8,RES,7)

   // SET
   DDCB_MACRO_LIST_RS(0xDDCBC0,SET,0)
   DDCB_MACRO_LIST_RS(0xDDCBC8,SET,1)
   DDCB_MACRO_LIST_RS(0xDDCBD0,SET,2)
   DDCB_MACRO_LIST_RS(0xDDCBD8,SET,3)
   DDCB_MACRO_LIST_RS(0xDDCBE0,SET,4)
   DDCB_MACRO_LIST_RS(0xDDCBE8,SET,5)
   DDCB_MACRO_LIST_RS(0xDDCBF0,SET,6)
   DDCB_MACRO_LIST_RS(0xDDCBF8,SET,7)

   // FDCB
   DDCB_MACRO_LIST(0xFDCB00, RLC)
   DDCB_MACRO_LIST(0xFDCB08, RRC)
   DDCB_MACRO_LIST(0xFDCB10, RL)
   DDCB_MACRO_LIST(0xFDCB18, RR)
   DDCB_MACRO_LIST(0xFDCB20, SLA)
   DDCB_MACRO_LIST(0xFDCB28, SRA)
   DDCB_MACRO_LIST(0xFDCB30, SLL)
   DDCB_MACRO_LIST(0xFDCB38, SRL)
   // BIT
   DDCB_MACRO_LIST_BIT(0xFDCB40, BIT_B_R_DD, 0)
   DDCB_MACRO_LIST_BIT(0xFDCB48, BIT_B_R_DD, 1)
   DDCB_MACRO_LIST_BIT(0xFDCB50, BIT_B_R_DD, 2)
   DDCB_MACRO_LIST_BIT(0xFDCB58, BIT_B_R_DD, 3)
   DDCB_MACRO_LIST_BIT(0xFDCB60, BIT_B_R_DD, 4)
   DDCB_MACRO_LIST_BIT(0xFDCB68, BIT_B_R_DD, 5)
   DDCB_MACRO_LIST_BIT(0xFDCB70, BIT_B_R_DD, 6)
   DDCB_MACRO_LIST_BIT(0xFDCB78, BIT_B_R_DD, 7)

   // RES
   DDCB_MACRO_LIST_RS(0xFDCB80,RES,0)
   DDCB_MACRO_LIST_RS(0xFDCB88,RES,1)
   DDCB_MACRO_LIST_RS(0xFDCB90,RES,2)
   DDCB_MACRO_LIST_RS(0xFDCB98,RES,3)
   DDCB_MACRO_LIST_RS(0xFDCBA0,RES,4)
   DDCB_MACRO_LIST_RS(0xFDCBA8,RES,5)
   DDCB_MACRO_LIST_RS(0xFDCBB0,RES,6)
   DDCB_MACRO_LIST_RS(0xFDCBB8,RES,7)

   // SET
   DDCB_MACRO_LIST_RS(0xFDCBC0,SET,0)
   DDCB_MACRO_LIST_RS(0xFDCBC8,SET,1)
   DDCB_MACRO_LIST_RS(0xFDCBD0,SET,2)
   DDCB_MACRO_LIST_RS(0xFDCBD8,SET,3)
   DDCB_MACRO_LIST_RS(0xFDCBE0,SET,4)
   DDCB_MACRO_LIST_RS(0xFDCBE8,SET,5)
   DDCB_MACRO_LIST_RS(0xFDCBF0,SET,6)
   DDCB_MACRO_LIST_RS(0xFDCBF8,SET,7)

   case 0xFF02: t_ = 1; if (read_count_ == 0) { current_address_++; ++read_count_; }
                else { pc_ = current_data_; NEXT_INSTR }break;

   default:
   #ifdef _WIN32
      _CrtDbgBreak();
   #endif
      NEXT_INSTR;
      break;

   }
   if (machine_cycle_ == M_Z80_WORK || machine_cycle_ == M_Z80_WAIT)
   {
      int ret = t_;
      counter_ += (ret - 1);
      t_ = 1;
      return ret;
   }
   return 1;
}
