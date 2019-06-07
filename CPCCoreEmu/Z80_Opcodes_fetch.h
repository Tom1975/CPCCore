

Begin:
switch (current_opcode_){
case 0x01: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD BC, nn
case 0x02: machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = bc_.w; current_data_ = af_.b.h;read_count_ = 0;break; // LD (BC), A
case 0x03: if ( t_ == 6){++bc_.w;NEXT_INSTR;}else{++t_;} break;                                                // INC BC
case 0x04: INC_FLAGS (bc_.b.h);NEXT_INSTR; break;                                                                     // INC B
case 0x05: DEC_FLAGS (bc_.b.h);NEXT_INSTR; break;                                                                     // DEC
case 0x06: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD B, n
case 0x07: q_ = af_.b.l& ~(NF|HF|CF|0x28);if(af_.b.h&0x80)q_ |=CF;btmp = af_.b.h>>7;af_.b.h = (af_.b.h<<1) + btmp;q_ |= (af_.b.h&0x28);af_.b.l = q_;NEXT_INSTR;break; // RLCA
case 0x08: EXCHANGE(af_.w, af_p_.w );NEXT_INSTR;break;                                                           // EX AF, AF'
case 0x09: ADD_FLAG_W(hl_.w, bc_.w); t_ = 4 + 3; machine_cycle_ = M_Z80_WORK; break;// ADD HL, BC
case 0x0A: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = bc_.w; current_data_ = 0;read_count_ = 0;break; // LD A, (BC)
case 0x0B: machine_cycle_ = M_Z80_WORK; t_ = 2; --bc_.w;break;                                                  // DEC BC
case 0x0C: INC_FLAGS (bc_.b.l);NEXT_INSTR; break;                                                                     // INC C
case 0x0D: DEC_FLAGS (bc_.b.l);NEXT_INSTR; break;                                                                     // DEC C
case 0x0E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD C, n
case 0x0F: q_ = af_.b.l; q_ &= ~(NF | HF | CF | 0x28); if (af_.b.h & 0x1)q_ |= CF; btmp = af_.b.h & 0x1; af_.b.h = (af_.b.h >> 1) + btmp * 0x80;q_ |= (af_.b.h & 0x28); af_.b.l = q_; NEXT_INSTR; break;     // RRCA
case 0x10: if ( t_ == 5) {machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;}else{++t_;}break; // DJNZ e
case 0x11: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD DE, nn
case 0x12: machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = de_.w; current_data_ = af_.b.h;read_count_ = 0;break; // LD (DE), A
case 0x13: if ( t_ == 6){++de_.w;NEXT_INSTR;}else{++t_;} break;                                                // INC DE
case 0x14: INC_FLAGS (de_.b.h);NEXT_INSTR; break;                                                                     // INC D
case 0x15: DEC_FLAGS (de_.b.h);NEXT_INSTR; break;                                                                     // DEC D
case 0x16: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD D, n
case 0x17: btmp = af_.b.l&CF;q_ = af_.b.l;q_ &= ~(NF|HF|CF|0x28);if(af_.b.h&0x80)q_ |=CF;af_.b.h=af_.b.h<<1;af_.b.h+=btmp;q_ |= (af_.b.h&0x28);af_.b.l = q_;NEXT_INSTR;break;  // RLA
case 0x18: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // JR e
case 0x19: ADD_FLAG_W(hl_.w, de_.w); t_ = 4 + 3; machine_cycle_ = M_Z80_WORK; break;// ADD HL, BC
case 0x1A: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = de_.w; current_data_ = 0;read_count_ = 0;break; // LD A, (DE)
case 0x1B: machine_cycle_ = M_Z80_WORK; t_ = 2; --de_.w;break;                                                  // DEC DE
case 0x1C: INC_FLAGS (de_.b.l);NEXT_INSTR; break;                                                                     // INC E
case 0x1D: DEC_FLAGS (de_.b.l);NEXT_INSTR; break;                                                                     // DEC E
case 0x1E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD E, n
case 0x1F: q_ = af_.b.l& ~(NF|HF|CF|0x28);if(af_.b.h&0x1)q_|=CF;af_.b.h=af_.b.h>>1;af_.b.h+=(af_.b.l&CF)*0x80;q_ |= (af_.b.h&0x28);af_.b.l = q_;NEXT_INSTR;break; // RRA
case 0x20: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // JR NZ, e
case 0x21: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD HL, nn
case 0x22: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD (nn), HL
case 0x23: if ( t_ == 6){++hl_.w;NEXT_INSTR;}else{++t_;} break;                                                // INC HL
case 0x24: INC_FLAGS (hl_.b.h);NEXT_INSTR; break;                                                                     // INC H
case 0x25: DEC_FLAGS (hl_.b.h);NEXT_INSTR; break;                                                                     // DEC H
case 0x26: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD H, n
case 0x27: btmp = af_.b.h;if (af_.b.l & NF) { if ((af_.b.l&HF) | ((af_.b.h&0xf)>9)) btmp-=6;if ((af_.b.l&CF) | (af_.b.h>0x99)) btmp-=0x60;}else {if ((af_.b.l&HF) | ((af_.b.h&0xf)>9)) btmp+=6;if ((af_.b.l&CF) | (af_.b.h>0x99)) btmp+=0x60;}q_ = (af_.b.l&(CF|NF)) | (af_.b.h>0x99) | ((af_.b.h^btmp)&HF) | Szp[btmp];af_.b.l = q_;af_.b.h = btmp;NEXT_INSTR;break;   // DAA
case 0x28: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // JR Z, e
case 0x29: ADD_FLAG_W(hl_.w, hl_.w); t_ = 4 + 3; machine_cycle_ = M_Z80_WORK; break;// ADD HL, BC
case 0x2A: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break;   // LD HL, (nn)
case 0x2B: machine_cycle_ = M_Z80_WORK; t_ = 2; --hl_.w;break;                                                  // DEC HL
case 0x2C: INC_FLAGS (hl_.b.l);NEXT_INSTR; break;                                                                     // INC L
case 0x2D: DEC_FLAGS (hl_.b.l);NEXT_INSTR; break;                                                                     // DEC L
case 0x2E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD L, n
case 0x2F: af_.b.h=~af_.b.h;q_ = af_.b.l & (~0x28);q_ |= (af_.b.h&0x28)|(NF|HF);af_.b.l = q_;NEXT_INSTR;break;                     // CPL
case 0x30: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // JR NC, e
case 0x31: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD SP, nn
case 0x32: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD (nn), A
case 0x33: if ( t_ == 6){++sp_;NEXT_INSTR;}else{++t_;} break;                                                  // INC SP
case 0x34: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break;// INC (HL)
case 0x35: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break;// DEC (HL)
case 0x36: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD (HL), n
case 0x37: q_ = (CF | (((q_^af_.b.l)|af_.b.h)&0x28)) | (af_.b.l&(PF|SF|ZF));af_.b.l = q_;NEXT_INSTR;break;                     //SCF
case 0x38: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // JR C, e
case 0x39: ADD_FLAG_W(hl_.w, sp_); t_ = 4 + 3; machine_cycle_ = M_Z80_WORK; break;// ADD HL, BC
case 0x3A: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD A, (nn)
case 0x3B: machine_cycle_ = M_Z80_WORK; t_ = 2; --sp_;break;                                                    // DEC SP
case 0x3C: INC_FLAGS (af_.b.h);NEXT_INSTR; break;                                                                     // INC A
case 0x3D: DEC_FLAGS (af_.b.h);NEXT_INSTR; break;                                                                     // DEC A
case 0x3E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD A, n
case 0x3F: q_ = ((((q_^af_.b.l) | af_.b.h) & 0x28)) | (af_.b.l&(PF | SF | ZF));q_ |= (af_.b.l&CF)?HF:CF;af_.b.l = q_;NEXT_INSTR;break; // CCF
case 0x40: bc_.b.h = bc_.b.h;NEXT_INSTR;break;// LD B, B
case 0x41: bc_.b.h = bc_.b.l;NEXT_INSTR;break;// LD B, C
case 0x42: bc_.b.h = de_.b.h;NEXT_INSTR;break;// LD B, D
case 0x43: bc_.b.h = de_.b.l;NEXT_INSTR;break;// LD B, E
case 0x44: bc_.b.h = hl_.b.h;NEXT_INSTR;break;// LD B, H
case 0x45: bc_.b.h = hl_.b.l;NEXT_INSTR;break;// LD B, L
case 0x46: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break; // LD B, (HL)
case 0x47: bc_.b.h = af_.b.h;NEXT_INSTR;break;// LD B, A
case 0x48: bc_.b.l = bc_.b.h;NEXT_INSTR;break;// LD C, B
case 0x49: bc_.b.l = bc_.b.l;NEXT_INSTR;break;// LD C, C
case 0x4A: bc_.b.l = de_.b.h;NEXT_INSTR;break;// LD C, D
case 0x4B: bc_.b.l = de_.b.l;NEXT_INSTR;break;// LD C, E
case 0x4C: bc_.b.l = hl_.b.h;NEXT_INSTR;break;// LD C, H
case 0x4D: bc_.b.l = hl_.b.l;NEXT_INSTR;break;// LD C, L
case 0x4E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break; // LD C, (HL)
case 0x4F: bc_.b.l = af_.b.h;NEXT_INSTR;break;// LD C, A
case 0x50: de_.b.h = bc_.b.h;NEXT_INSTR;break;// LD D, B
case 0x51: de_.b.h = bc_.b.l;NEXT_INSTR;break;// LD D, C
case 0x52: de_.b.h = de_.b.h;NEXT_INSTR;break;// LD D, D
case 0x53: de_.b.h = de_.b.l;NEXT_INSTR;break;// LD D, E
case 0x54: de_.b.h = hl_.b.h;NEXT_INSTR;break;// LD D, H
case 0x55: de_.b.h = hl_.b.l;NEXT_INSTR;break;// LD D, L
case 0x56: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break; // LD D, (HL)
case 0x57: de_.b.h = af_.b.h;NEXT_INSTR;break;// LD D, A
case 0x58: de_.b.l = bc_.b.h;NEXT_INSTR;break;// LD E, B
case 0x59: de_.b.l = bc_.b.l;NEXT_INSTR;break;// LD E, C
case 0x5A: de_.b.l = de_.b.h;NEXT_INSTR;break;// LD E, D
case 0x5B: de_.b.l = de_.b.l;NEXT_INSTR;break;// LD E, E
case 0x5C: de_.b.l = hl_.b.h;NEXT_INSTR;break;// LD E, H
case 0x5D: de_.b.l = hl_.b.l;NEXT_INSTR;break;// LD E, L
case 0x5E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break; // LD E, (HL)
case 0x5F: de_.b.l = af_.b.h;NEXT_INSTR;break;// LD E, A
case 0x60: hl_.b.h = bc_.b.h;NEXT_INSTR;break;// LD H, B
case 0x61: hl_.b.h = bc_.b.l;NEXT_INSTR;break;// LD H, C
case 0x62: hl_.b.h = de_.b.h;NEXT_INSTR;break;// LD H, D
case 0x63: hl_.b.h = de_.b.l;NEXT_INSTR;break;// LD H, E
case 0x64: hl_.b.h = hl_.b.h;NEXT_INSTR;break;// LD H, H
case 0x65: hl_.b.h = hl_.b.l;NEXT_INSTR;break;// LD H, L
case 0x66: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break; // LD H, (HL)
case 0x67: hl_.b.h = af_.b.h;NEXT_INSTR;break;// LD H, A
case 0x68: hl_.b.l = bc_.b.h;NEXT_INSTR;break;// LD L, B
case 0x69: hl_.b.l = bc_.b.l;NEXT_INSTR;break;// LD L, C
case 0x6A: hl_.b.l = de_.b.h;NEXT_INSTR;break;// LD L, D
case 0x6B: hl_.b.l = de_.b.l;NEXT_INSTR;break;// LD L, E
case 0x6C: hl_.b.l = hl_.b.h;NEXT_INSTR;break;// LD L, H
case 0x6D: hl_.b.l = hl_.b.l;NEXT_INSTR;break;// LD L, L
case 0x6E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break; // LD L, (HL)
case 0x6F: hl_.b.l = af_.b.h;NEXT_INSTR;break;// LD L, A
case 0x70: machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = hl_.w; current_data_ = bc_.b.h;read_count_ = 0;break; // LD (HL), B
case 0x71: machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = hl_.w; current_data_ = bc_.b.l;read_count_ = 0;break; // LD (HL), C
case 0x72: machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = hl_.w; current_data_ = de_.b.h;read_count_ = 0;break; // LD (HL), D
case 0x73: machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = hl_.w; current_data_ = de_.b.l;read_count_ = 0;break; // LD (HL), E
case 0x74: machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = hl_.w; current_data_ = hl_.b.h;read_count_ = 0;break; // LD (HL), H
case 0x75: machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = hl_.w; current_data_ = hl_.b.l;read_count_ = 0;break; // LD (HL), L
case 0x76: if (sig_->nmi_){ SET_NMI;}else if (sig_->int_ && iff1_) {SET_INT;}else{--pc_;SET_NOINT;}t_ = 1;SYNC_Z80;break;
case 0x77: machine_cycle_ = M_MEMORY_W; TraceTape(pc_, af_.b.h); t_ = 1; current_address_ = hl_.w; current_data_ = af_.b.h; read_count_ = 0; break; // LD (HL), A
case 0x78: af_.b.h = bc_.b.h;NEXT_INSTR;break;// LD A, B
case 0x79: af_.b.h = bc_.b.l;NEXT_INSTR;break;// LD A, C
case 0x7A: af_.b.h = de_.b.h;NEXT_INSTR;break;// LD A, D
case 0x7B: af_.b.h = de_.b.l;NEXT_INSTR;break;// LD A, E
case 0x7C: af_.b.h = hl_.b.h;NEXT_INSTR;break;// LD A, H
case 0x7D: af_.b.h = hl_.b.l;NEXT_INSTR;break;// LD A, L
case 0x7E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break; // LD A, (HL)
case 0x7F: af_.b.h = af_.b.h;NEXT_INSTR;break;// LD A, A
case 0x80: ADD_FLAG (bc_.b.h);NEXT_INSTR;break;     // ADD A, B
case 0x81: ADD_FLAG (bc_.b.l);NEXT_INSTR;break;     // ADD A, C
case 0x82: ADD_FLAG (de_.b.h);NEXT_INSTR;break;     // ADD A, D
case 0x83: ADD_FLAG (de_.b.l);NEXT_INSTR;break;     // ADD A, E
case 0x84: ADD_FLAG (hl_.b.h);NEXT_INSTR;break;     // ADD A, H
case 0x85: ADD_FLAG (hl_.b.l);NEXT_INSTR;break;     // ADD A, L
case 0x86: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break; //ADD A, (HL)
case 0x87: ADD_FLAG (af_.b.h);NEXT_INSTR;break;     // ADD A, A
case 0x88: ADD_FLAG_CARRY(bc_.b.h);NEXT_INSTR;break;// ADC A, B
case 0x89: ADD_FLAG_CARRY(bc_.b.l);NEXT_INSTR;break;// ADC A, C
case 0x8A: ADD_FLAG_CARRY(de_.b.h);NEXT_INSTR;break;// ADC A, D
case 0x8B: ADD_FLAG_CARRY(de_.b.l);NEXT_INSTR;break;// ADC A, E
case 0x8C: ADD_FLAG_CARRY(hl_.b.h);NEXT_INSTR;break;// ADC A, L
case 0x8D: ADD_FLAG_CARRY(hl_.b.l);NEXT_INSTR;break;// ADC A, H
case 0x8E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break; // ADC A, (HL)
case 0x8F: ADD_FLAG_CARRY(af_.b.h);NEXT_INSTR;break;// ADC A, A
case 0x90: SUB_FLAG(bc_.b.h);NEXT_INSTR;;break;// SUB B
case 0x91: SUB_FLAG(bc_.b.l);NEXT_INSTR;;break;// SUB C
case 0x92: SUB_FLAG(de_.b.h);NEXT_INSTR;;break;// SUB D
case 0x93: SUB_FLAG(de_.b.l);NEXT_INSTR;;break;// SUB E
case 0x94: SUB_FLAG(hl_.b.h);NEXT_INSTR;;break;// SUB H
case 0x95: SUB_FLAG(hl_.b.l);NEXT_INSTR;;break;// SUB L
case 0x96: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break;// SUB (HL)
case 0x97: SUB_FLAG(af_.b.h);NEXT_INSTR;;break;// SUB A
case 0x98: SUB_FLAG_CARRY(bc_.b.h);NEXT_INSTR;;break;// SBC B
case 0x99: SUB_FLAG_CARRY(bc_.b.l);NEXT_INSTR;;break;// SBC C
case 0x9A: SUB_FLAG_CARRY(de_.b.h);NEXT_INSTR;;break;// SBC D
case 0x9B: SUB_FLAG_CARRY(de_.b.l);NEXT_INSTR;;break;// SBC E
case 0x9C: SUB_FLAG_CARRY(hl_.b.h);NEXT_INSTR;;break;// SBC H
case 0x9D: SUB_FLAG_CARRY(hl_.b.l);NEXT_INSTR;;break;// SBC L
case 0x9E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break;// SBC (HL)
case 0x9F: SUB_FLAG_CARRY(af_.b.h);NEXT_INSTR;;break;// SBC A
case 0xA0: AND_FLAGS(bc_.b.h);NEXT_INSTR;break;  // AND B
case 0xA1: AND_FLAGS(bc_.b.l);NEXT_INSTR;break;  // AND C
case 0xA2: AND_FLAGS(de_.b.h);NEXT_INSTR;break;  // AND D
case 0xA3: AND_FLAGS(de_.b.l);NEXT_INSTR;break;  // AND E
case 0xA4: AND_FLAGS(hl_.b.h);NEXT_INSTR;break;  // AND H
case 0xA5: AND_FLAGS(hl_.b.l);NEXT_INSTR;break;  // AND L
case 0xA6: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break;  // AND (HL)
case 0xA7: AND_FLAGS(af_.b.h);NEXT_INSTR;break;  // AND A
case 0xA8: XOR_FLAGS(bc_.b.h);NEXT_INSTR;break;  // XOR B
case 0xA9: XOR_FLAGS(bc_.b.l);NEXT_INSTR;break;  // XOR C
case 0xAA: XOR_FLAGS(de_.b.h);NEXT_INSTR;break;  // XOR D
case 0xAB: XOR_FLAGS(de_.b.l);NEXT_INSTR;break;  // XOR E
case 0xAC: XOR_FLAGS(hl_.b.h);NEXT_INSTR;break;  // XOR H
case 0xAD: XOR_FLAGS(hl_.b.l);NEXT_INSTR;break;  // XOR L
case 0xAE: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break;  // XOR(HL)
case 0xAF: XOR_FLAGS(af_.b.h);NEXT_INSTR;break;  // XOR A
case 0xB0: OR_FLAGS(bc_.b.h); NEXT_INSTR;break;// OR B
case 0xB1: OR_FLAGS(bc_.b.l); NEXT_INSTR;break;// OR C
case 0xB2: OR_FLAGS(de_.b.h); NEXT_INSTR;break;// OR D
case 0xB3: OR_FLAGS(de_.b.l); NEXT_INSTR;break;// OR E
case 0xB4: OR_FLAGS(hl_.b.h); NEXT_INSTR;break;// OR H
case 0xB5: OR_FLAGS(hl_.b.l); NEXT_INSTR;break;// OR L
case 0xB6: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break;  // OR (HL)
case 0xB7: OR_FLAGS(af_.b.h); NEXT_INSTR;break;// OR A
case 0xB8: CP_FLAGS(bc_.b.h); NEXT_INSTR;break;// CP B
case 0xB9: CP_FLAGS(bc_.b.l); NEXT_INSTR;break;// CP C
case 0xBA: CP_FLAGS(de_.b.h); NEXT_INSTR;break;// CP D
case 0xBB: CP_FLAGS(de_.b.l); NEXT_INSTR;break;// CP E
case 0xBC: CP_FLAGS(hl_.b.h); NEXT_INSTR;break;// CP H
case 0xBD: CP_FLAGS(hl_.b.l); NEXT_INSTR;break;// CP L
case 0xBE: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break; // CP (HL)
case 0xBF: CP_FLAGS(af_.b.h); NEXT_INSTR;break;// CP A
case 0xC0: if (t_ == 5) { TSTN(ZF){machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_++; current_data_ = 0; read_count_ = 0; } else { NEXT_INSTR } }
           else { ++t_; }break;// RET NZ
case 0xC1: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_++; current_data_ = 0;read_count_ = 0;break;// POP BC
case 0xC2: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // JP NZ nn
case 0xC3: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // JP nn
case 0xC4: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // CALL NZ nn
case 0xC5: if ( t_ == 5) {machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = --sp_; current_data_ = bc_.b.h;read_count_ = 0;}else{++t_;}break; // PUSH BC
case 0xC6: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break;     // ADD A, n
case 0xC7: if ( t_ == 5) {machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = --sp_; current_data_ = pc_>>8;read_count_ = 0;}else{++t_;}break; // RST 00
case 0xC8: if ( t_ == 5) { TST(ZF){machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_++;current_data_ = 0;read_count_ = 0;}else{NEXT_INSTR}}else{++t_;}break; // RET Z
case 0xC9:
      TraceTape(pc_, af_.b.h);
      machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_++;current_data_ = 0;read_count_ = 0;break; // RET
case 0xCA: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // JP Z nn
case 0xCC: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // CALL Z nn
case 0xCD: machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = --sp_; current_data_ = (pc_+2)>>8;read_count_ = 0;break; // CALL nn
case 0xCE: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // ADC_A, N
case 0xCF: if ( t_ == 5) {machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = --sp_; current_data_ = pc_>>8;read_count_ = 0;}else{++t_;}break; // RST 08
case 0xD0: if ( t_ == 5) { TSTN(CF){machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_++;current_data_ = 0;read_count_ = 0;}else{NEXT_INSTR}}else{++t_;}break;// RET CF
case 0xD1: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_++; current_data_ = 0;read_count_ = 0;break;// POP DE
case 0xD2: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // JP NC nn
case 0xD3: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // OUT n, A
case 0xD4: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // CALL NC nn
case 0xD5: if ( t_ == 5) {machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = --sp_; current_data_ = de_.b.h;read_count_ = 0;}else{++t_;}break;   // PUSH DE
case 0xD6: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break;// SUB n
case 0xD7: if ( t_ == 5)
            {
               TraceTape(pc_, af_.b.h);
               machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = --sp_; current_data_ = pc_>>8;read_count_ = 0;
            }else{
               ++t_;
            }break; // RST 10
case 0xD8: if ( t_ == 5) { TST(CF){machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_++;current_data_ = 0;read_count_ = 0;}else{NEXT_INSTR}}else{++t_;}break; // RET C
case 0xD9: EXCHANGE(bc_.w, bc_p_.w );EXCHANGE(de_.w, de_p_.w );EXCHANGE(hl_.w, hl_p_.w );NEXT_INSTR;break;           // EXX
case 0xDA: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // JP C nn
case 0xDB: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // IN A, n
case 0xDC: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // CALL C nn
case 0xDE: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break;// SBC n
case 0xDF: if ( t_ == 5) {machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = --sp_; current_data_ = pc_>>8;read_count_ = 0;}else{++t_;}break; // RST 18
case 0xE0: if ( t_ == 5) { TSTN(PF){machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_++;current_data_ = 0;read_count_ = 0;}else{NEXT_INSTR}}else{++t_;}break;// RET PO
case 0xE1: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_++; current_data_ = 0;read_count_ = 0;break;// POP HL
case 0xE2: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // JP PO nn
case 0xE3: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_; current_data_ = 0;read_count_ = 0;break;   // EX (SP), HL
case 0xE4: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // CALL PO nn
case 0xE5: if ( t_ == 5) {machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = --sp_; current_data_ = hl_.b.h;read_count_ = 0;}else{++t_;}break; // PUSH HL
case 0xE6: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break;  // AND n
case 0xE7: if ( t_ == 5) {machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = --sp_; current_data_ = pc_>>8;read_count_ = 0;}else{++t_;}break; // RST 20
case 0xE8: if ( t_ == 5) { TST(PF){machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_++;current_data_ = 0;read_count_ = 0;}else{NEXT_INSTR}}else{++t_;}break; // RET PE
case 0xE9: pc_ = hl_.w;NEXT_INSTR;break;   // JP (HL)
case 0xEA: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // JP PE nn
case 0xEB: EXCHANGE(de_.w, hl_.w); NEXT_INSTR; break;//EX DE, HL
case 0xEC: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // CALL PE nn
case 0xEE: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break;// XOR n
case 0xEF: if ( t_ == 5) {machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = --sp_; current_data_ = pc_>>8;read_count_ = 0;}else{++t_;}break; // RST 28
case 0xF0: if ( t_ == 5) { TSTN(SF){machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_++;current_data_ = 0;read_count_ = 0;}else{NEXT_INSTR}}else{++t_;}break;// RET M
case 0xF1: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_++; current_data_ = 0;read_count_ = 0;break;// POP AF
case 0xF2: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // JP P nn
case 0xF3: iff1_ = false;iff2_ = false;NEXT_INSTR; break;                      // DI
case 0xF4: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // CALL P nn
case 0xF5: if ( t_ == 5) {machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = --sp_; current_data_ = af_.b.h;read_count_ = 0;}else{++t_;}break; // PUSH AF
case 0xF6: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break;// OR n
case 0xF7: if ( t_ == 5) {machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = --sp_; current_data_ = pc_>>8;read_count_ = 0;}else{++t_;}break; // RST 30
case 0xF8: if ( t_ == 5) { TST(SF){machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_++;current_data_ = 0;read_count_ = 0;}else{NEXT_INSTR}}else{++t_;}break; // RET P
case 0xF9: if ( t_ == 6) { sp_ = hl_.w; NEXT_INSTR;}else{++t_;}break;// LD SP, HL
case 0xFA: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // JP M nn
case 0xFB: iff1_ = true;iff2_ = true;NEXT_INSTR_EI; break;                        // EI
case 0xFC: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // CALL M nn
case 0xFE: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break;// CP n;
case 0xFF: if ( t_ == 5) {machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = --sp_; current_data_ = pc_>>8;read_count_ = 0;}else{++t_;}break; // RST 38
   //////////////////////////////////////////////////////////
   // CB
/// RLC
case 0xCB00: RLC(bc_.b.h);NEXT_INSTR;break; // RLC B
case 0xCB01: RLC(bc_.b.l);NEXT_INSTR;break; // RLC C
case 0xCB02: RLC(de_.b.h);NEXT_INSTR;break; // RLC D
case 0xCB03: RLC(de_.b.l);NEXT_INSTR;break; // RLC E
case 0xCB04: RLC(hl_.b.h);NEXT_INSTR;break; // RLC H
case 0xCB05: RLC(hl_.b.l);NEXT_INSTR;break; // RLC L
case 0xCB06: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break;  // RLC (HL)
case 0xCB07: RLC(af_.b.h);NEXT_INSTR;break; // RLC A
/// RRC
case 0xCB08: RRC(bc_.b.h);NEXT_INSTR;break; // RRC B
case 0xCB09: RRC(bc_.b.l);NEXT_INSTR;break; // RRC C
case 0xCB0A: RRC(de_.b.h);NEXT_INSTR;break; // RRC D
case 0xCB0B: RRC(de_.b.l);NEXT_INSTR;break; // RRC E
case 0xCB0C: RRC(hl_.b.h);NEXT_INSTR;break; // RRC H
case 0xCB0D: RRC(hl_.b.l);NEXT_INSTR;break; // RRC L
case 0xCB0E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break;  // RRC (HL)
case 0xCB0F: RRC(af_.b.h);NEXT_INSTR;break; // RRC A
/// RL
case 0xCB10:RL(bc_.b.h);NEXT_INSTR;break; // RL B
case 0xCB11:RL(bc_.b.l);NEXT_INSTR;break; // RL C
case 0xCB12:RL(de_.b.h);NEXT_INSTR;break; // RL D
case 0xCB13:RL(de_.b.l);NEXT_INSTR;break; // RL E
case 0xCB14:RL(hl_.b.h);NEXT_INSTR;break; // RL H
case 0xCB15:RL(hl_.b.l);NEXT_INSTR;break; // RL L
case 0xCB16: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break;  // RL (HL)
case 0xCB17:RL(af_.b.h);NEXT_INSTR;break; // RL A
/// RR
case 0xCB18:RR(bc_.b.h);NEXT_INSTR;break; // RR B
case 0xCB19:RR(bc_.b.l);NEXT_INSTR;break; // RR C
case 0xCB1A:RR(de_.b.h);NEXT_INSTR;break; // RR D
case 0xCB1B:RR(de_.b.l);NEXT_INSTR;break; // RR E
case 0xCB1C:RR(hl_.b.h);NEXT_INSTR;break; // RR H
case 0xCB1D:RR(hl_.b.l);NEXT_INSTR;break; // RR L
case 0xCB1E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break;  // RR (HL)
case 0xCB1F:RR(af_.b.h);NEXT_INSTR;break; // RR A
/// SLA
case 0xCB20:SLA(bc_.b.h);NEXT_INSTR;break; // SLA B
case 0xCB21:SLA(bc_.b.l);NEXT_INSTR;break; // SLA C
case 0xCB22:SLA(de_.b.h);NEXT_INSTR;break; // SLA D
case 0xCB23:SLA(de_.b.l);NEXT_INSTR;break; // SLA E
case 0xCB24:SLA(hl_.b.h);NEXT_INSTR;break; // SLA H
case 0xCB25:SLA(hl_.b.l);NEXT_INSTR;break; // SLA L
case 0xCB26: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break;  // SLA (HL)
case 0xCB27:SLA(af_.b.h);NEXT_INSTR;break; // SLA A
/// SRA
case 0xCB28:SRA(bc_.b.h);NEXT_INSTR;break; // SRA B
case 0xCB29:SRA(bc_.b.l);NEXT_INSTR;break; // SRA C
case 0xCB2A:SRA(de_.b.h);NEXT_INSTR;break; // SRA D
case 0xCB2B:SRA(de_.b.l);NEXT_INSTR;break; // SRA E
case 0xCB2C:SRA(hl_.b.h);NEXT_INSTR;break; // SRA H
case 0xCB2D:SRA(hl_.b.l);NEXT_INSTR;break; // SRA L
case 0xCB2E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break;  // SRA (HL)
case 0xCB2F:SRA(af_.b.h);NEXT_INSTR;break; // SRA A
/// SLL
case 0xCB30:SLL(bc_.b.h);NEXT_INSTR;break; // SLL B
case 0xCB31:SLL(bc_.b.l);NEXT_INSTR;break; // SLL C
case 0xCB32:SLL(de_.b.h);NEXT_INSTR;break; // SLL D
case 0xCB33:SLL(de_.b.l);NEXT_INSTR;break; // SLL E
case 0xCB34:SLL(hl_.b.h);NEXT_INSTR;break; // SLL H
case 0xCB35:SLL(hl_.b.l);NEXT_INSTR;break; // SLL L
case 0xCB36: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break;  // SLL (HL)
case 0xCB37:SLL(af_.b.h);NEXT_INSTR;break; // SLL A
/// SRL
case 0xCB38:SRL(bc_.b.h);NEXT_INSTR;break; // SRL B
case 0xCB39:SRL(bc_.b.l);NEXT_INSTR;break; // SRL C
case 0xCB3A:SRL(de_.b.h);NEXT_INSTR;break; // SRL D
case 0xCB3B:SRL(de_.b.l);NEXT_INSTR;break; // SRL E
case 0xCB3C:SRL(hl_.b.h);NEXT_INSTR;break; // SRL H
case 0xCB3D:SRL(hl_.b.l);NEXT_INSTR;break; // SRL L
case 0xCB3E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break;  // SRL (HL)
case 0xCB3F:SRL(af_.b.h);NEXT_INSTR;break; // SRL A
/// BIT
case 0xCB40: BIT_B_R(0,bc_.b.h);NEXT_INSTR;break; // BIT 0, B
case 0xCB41: BIT_B_R(0,bc_.b.l);NEXT_INSTR;break; // BIT 0, C
case 0xCB42: BIT_B_R(0,de_.b.h);NEXT_INSTR;break; // BIT 0, D
case 0xCB43: BIT_B_R(0,de_.b.l);NEXT_INSTR;break; // BIT 0, E
case 0xCB44: BIT_B_R(0,hl_.b.h);NEXT_INSTR;break; // BIT 0, H
case 0xCB45: BIT_B_R(0,hl_.b.l);NEXT_INSTR;break; // BIT 0, L
case 0xCB46: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break;// BIT 0, (HL)
case 0xCB47: BIT_B_R(0,af_.b.h);NEXT_INSTR;break; // BIT 1, A
case 0xCB48: BIT_B_R(1,bc_.b.h);NEXT_INSTR;break; // BIT 1, B
case 0xCB49: BIT_B_R(1,bc_.b.l);NEXT_INSTR;break; // BIT 1, C
case 0xCB4A: BIT_B_R(1,de_.b.h);NEXT_INSTR;break; // BIT 1, D
case 0xCB4B: BIT_B_R(1,de_.b.l);NEXT_INSTR;break; // BIT 1, E
case 0xCB4C: BIT_B_R(1,hl_.b.h);NEXT_INSTR;break; // BIT 1, H
case 0xCB4D: BIT_B_R(1,hl_.b.l);NEXT_INSTR;break; // BIT 1, L
case 0xCB4E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break;// BIT 1, (HL)
case 0xCB4F: BIT_B_R(1,af_.b.h);NEXT_INSTR;break; // BIT 1, A
case 0xCB50: BIT_B_R(2,bc_.b.h);NEXT_INSTR;break; // BIT 2, B
case 0xCB51: BIT_B_R(2,bc_.b.l);NEXT_INSTR;break; // BIT 2, C
case 0xCB52: BIT_B_R(2,de_.b.h);NEXT_INSTR;break; // BIT 2, D
case 0xCB53: BIT_B_R(2,de_.b.l);NEXT_INSTR;break; // BIT 2, E
case 0xCB54: BIT_B_R(2,hl_.b.h);NEXT_INSTR;break; // BIT 2, H
case 0xCB55: BIT_B_R(2,hl_.b.l);NEXT_INSTR;break; // BIT 2, L
case 0xCB56: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break;// BIT 2, (HL)
case 0xCB57: BIT_B_R(2,af_.b.h);NEXT_INSTR;break; // BIT 2, A
case 0xCB58: BIT_B_R(3,bc_.b.h);NEXT_INSTR;break; // BIT 3, B
case 0xCB59: BIT_B_R(3,bc_.b.l);NEXT_INSTR;break; // BIT 3, C
case 0xCB5A: BIT_B_R(3,de_.b.h);NEXT_INSTR;break; // BIT 3, D
case 0xCB5B: BIT_B_R(3,de_.b.l);NEXT_INSTR;break; // BIT 3, E
case 0xCB5C: BIT_B_R(3,hl_.b.h);NEXT_INSTR;break; // BIT 3, H
case 0xCB5D: BIT_B_R(3,hl_.b.l);NEXT_INSTR;break; // BIT 3, L
case 0xCB5E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break;// BIT 3, (HL)
case 0xCB5F: BIT_B_R(3,af_.b.h);NEXT_INSTR;break; // BIT 3, A
case 0xCB60: BIT_B_R(4,bc_.b.h);NEXT_INSTR;break; // BIT 4, B
case 0xCB61: BIT_B_R(4,bc_.b.l);NEXT_INSTR;break; // BIT 4, C
case 0xCB62: BIT_B_R(4,de_.b.h);NEXT_INSTR;break; // BIT 4, D
case 0xCB63: BIT_B_R(4,de_.b.l);NEXT_INSTR;break; // BIT 4, E
case 0xCB64: BIT_B_R(4,hl_.b.h);NEXT_INSTR;break; // BIT 4, H
case 0xCB65: BIT_B_R(4,hl_.b.l);NEXT_INSTR;break; // BIT 4, L
case 0xCB66:  machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break; // BIT 4, (HL)
case 0xCB67: BIT_B_R(4,af_.b.h);NEXT_INSTR;break; // BIT 4, A
case 0xCB68: BIT_B_R(5,bc_.b.h);NEXT_INSTR;break; // BIT 5, B
case 0xCB69: BIT_B_R(5,bc_.b.l);NEXT_INSTR;break; // BIT 5, C
case 0xCB6A: BIT_B_R(5,de_.b.h);NEXT_INSTR;break; // BIT 5, D
case 0xCB6B: BIT_B_R(5,de_.b.l);NEXT_INSTR;break; // BIT 5, E
case 0xCB6C: BIT_B_R(5,hl_.b.h);NEXT_INSTR;break; // BIT 5, H
case 0xCB6D: BIT_B_R(5,hl_.b.l);NEXT_INSTR;break; // BIT 5, L
case 0xCB6E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break; // BIT 5, (HL)
case 0xCB6F: BIT_B_R(5,af_.b.h);NEXT_INSTR;break; // BIT 5, A
case 0xCB70: BIT_B_R(6,bc_.b.h);NEXT_INSTR;break; // BIT 6, B
case 0xCB71: BIT_B_R(6,bc_.b.l);NEXT_INSTR;break; // BIT 6, C
case 0xCB72: BIT_B_R(6,de_.b.h);NEXT_INSTR;break; // BIT 6, D
case 0xCB73: BIT_B_R(6,de_.b.l);NEXT_INSTR;break; // BIT 6, E
case 0xCB74: BIT_B_R(6,hl_.b.h);NEXT_INSTR;break; // BIT 6, H
case 0xCB75: BIT_B_R(6,hl_.b.l);NEXT_INSTR;break; // BIT 6, L
case 0xCB76: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break; // BIT 6, (HL)
case 0xCB77: BIT_B_R(6,af_.b.h);NEXT_INSTR;break; // BIT 6, A
case 0xCB78: BIT_B_R(7,bc_.b.h);NEXT_INSTR;break; // BIT 7, B
case 0xCB79: BIT_B_R(7,bc_.b.l);NEXT_INSTR;break; // BIT 7, C
case 0xCB7A: BIT_B_R(7,de_.b.h);NEXT_INSTR;break; // BIT 7, D
case 0xCB7B: BIT_B_R(7,de_.b.l);NEXT_INSTR;break; // BIT 7, E
case 0xCB7C: BIT_B_R(7,hl_.b.h);NEXT_INSTR;break; // BIT 7, H
case 0xCB7D: BIT_B_R(7,hl_.b.l);NEXT_INSTR;break; // BIT 7, L
case 0xCB7E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break; // BIT 7, (HL)
case 0xCB7F: BIT_B_R(7,af_.b.h);NEXT_INSTR;break; // BIT 7, A
/// RES
case 0xCB80: bc_.b.h &=~0x1;NEXT_INSTR;break;// Set
case 0xCB81: bc_.b.l &=~0x1;NEXT_INSTR;break;// Set
case 0xCB82: de_.b.h &=~0x1;NEXT_INSTR;break;// Set
case 0xCB83: de_.b.l &=~0x1;NEXT_INSTR;break;// Set
case 0xCB84: hl_.b.h &=~0x1;NEXT_INSTR;break;// Set
case 0xCB85: hl_.b.l &=~0x1;NEXT_INSTR;break;// Set
case 0xCB86: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break; // Res 0,(HL)
case 0xCB87: af_.b.h &=~0x1;NEXT_INSTR;break;// Set
case 0xCB88: bc_.b.h &=~0x2;NEXT_INSTR;break;// Set
case 0xCB89: bc_.b.l &=~0x2;NEXT_INSTR;break;// Set
case 0xCB8A: de_.b.h &=~0x2;NEXT_INSTR;break;// Set
case 0xCB8B: de_.b.l &=~0x2;NEXT_INSTR;break;// Set
case 0xCB8C: hl_.b.h &=~0x2;NEXT_INSTR;break;// Set
case 0xCB8D: hl_.b.l &=~0x2;NEXT_INSTR;break;// Set
case 0xCB8E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break; // Res 1,(HL)
case 0xCB8F: af_.b.h &=~0x2;NEXT_INSTR;break;// Set
case 0xCB90: bc_.b.h &=~0x4;NEXT_INSTR;break;// Set
case 0xCB91: bc_.b.l &=~0x4;NEXT_INSTR;break;// Set
case 0xCB92: de_.b.h &=~0x4;NEXT_INSTR;break;// Set
case 0xCB93: de_.b.l &=~0x4;NEXT_INSTR;break;// Set
case 0xCB94: hl_.b.h &=~0x4;NEXT_INSTR;break;// Set
case 0xCB95: hl_.b.l &=~0x4;NEXT_INSTR;break;// Set
case 0xCB96: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break; // Res 2,(HL)
case 0xCB97: af_.b.h &=~0x4;NEXT_INSTR;break;// Set
case 0xCB98: bc_.b.h &=~0x8;NEXT_INSTR;break;// Set
case 0xCB99: bc_.b.l &=~0x8;NEXT_INSTR;break;// Set
case 0xCB9A: de_.b.h &=~0x8;NEXT_INSTR;break;// Set
case 0xCB9B: de_.b.l &=~0x8;NEXT_INSTR;break;// Set
case 0xCB9C: hl_.b.h &=~0x8;NEXT_INSTR;break;// Set
case 0xCB9D: hl_.b.l &=~0x8;NEXT_INSTR;break;// Set
case 0xCB9E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break; // Res 3,(HL)
case 0xCB9F: af_.b.h &=~0x8;NEXT_INSTR;break;// Set
case 0xCBA0: bc_.b.h &=~0x10;NEXT_INSTR;break;// Set
case 0xCBA1: bc_.b.l &=~0x10;NEXT_INSTR;break;// Set
case 0xCBA2: de_.b.h &=~0x10;NEXT_INSTR;break;// Set
case 0xCBA3: de_.b.l &=~0x10;NEXT_INSTR;break;// Set
case 0xCBA4: hl_.b.h &=~0x10;NEXT_INSTR;break;// Set
case 0xCBA5: hl_.b.l &=~0x10;NEXT_INSTR;break;// Set
case 0xCBA6: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break; // Res 4,(HL)
case 0xCBA7: af_.b.h &=~0x10;NEXT_INSTR;break;// Set
case 0xCBA8: bc_.b.h &=~0x20;NEXT_INSTR;break;// Set
case 0xCBA9: bc_.b.l &=~0x20;NEXT_INSTR;break;// Set
case 0xCBAA: de_.b.h &=~0x20;NEXT_INSTR;break;// Set
case 0xCBAB: de_.b.l &=~0x20;NEXT_INSTR;break;// Set
case 0xCBAC: hl_.b.h &=~0x20;NEXT_INSTR;break;// Set
case 0xCBAD: hl_.b.l &=~0x20;NEXT_INSTR;break;// Set
case 0xCBAE: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break; // Res 5,(HL)
case 0xCBAF: af_.b.h &=~0x20;NEXT_INSTR;break;// Set
case 0xCBB0: bc_.b.h &=~0x40;NEXT_INSTR;break;// Set
case 0xCBB1: bc_.b.l &=~0x40;NEXT_INSTR;break;// Set
case 0xCBB2: de_.b.h &=~0x40;NEXT_INSTR;break;// Set
case 0xCBB3: de_.b.l &=~0x40;NEXT_INSTR;break;// Set
case 0xCBB4: hl_.b.h &=~0x40;NEXT_INSTR;break;// Set
case 0xCBB5: hl_.b.l &=~0x40;NEXT_INSTR;break;// Set
case 0xCBB6: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break; // Res 6,(HL)
case 0xCBB7: af_.b.h &=~0x40;NEXT_INSTR;break;// Set
case 0xCBB8: bc_.b.h &=~0x80;NEXT_INSTR;break;// Set
case 0xCBB9: bc_.b.l &=~0x80;NEXT_INSTR;break;// Set
case 0xCBBA: de_.b.h &=~0x80;NEXT_INSTR;break;// Set
case 0xCBBB: de_.b.l &=~0x80;NEXT_INSTR;break;// Set
case 0xCBBC: hl_.b.h &=~0x80;NEXT_INSTR;break;// Set
case 0xCBBD: hl_.b.l &=~0x80;NEXT_INSTR;break;// Set
case 0xCBBE: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break; // Res 7,(HL)
case 0xCBBF: af_.b.h &=~0x80;NEXT_INSTR;break;// Set
/// SET
case 0xCBC0: bc_.b.h |= 0x1;NEXT_INSTR;break;// Set
case 0xCBC1: bc_.b.l |= 0x1;NEXT_INSTR;break;// Set
case 0xCBC2: de_.b.h |= 0x1;NEXT_INSTR;break;// Set
case 0xCBC3: de_.b.l |= 0x1;NEXT_INSTR;break;// Set
case 0xCBC4: hl_.b.h |= 0x1;NEXT_INSTR;break;// Set
case 0xCBC5: hl_.b.l |= 0x1;NEXT_INSTR;break;// Set
case 0xCBC6: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break; // Set 0,(HL)
case 0xCBC7: af_.b.h |= 0x1;NEXT_INSTR;break;// Set
case 0xCBC8: bc_.b.h |= 0x2;NEXT_INSTR;break;// Set
case 0xCBC9: bc_.b.l |= 0x2;NEXT_INSTR;break;// Set
case 0xCBCA: de_.b.h |= 0x2;NEXT_INSTR;break;// Set
case 0xCBCB: de_.b.l |= 0x2;NEXT_INSTR;break;// Set
case 0xCBCC: hl_.b.h |= 0x2;NEXT_INSTR;break;// Set
case 0xCBCD: hl_.b.l |= 0x2;NEXT_INSTR;break;// Set
case 0xCBCE: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break; // Set 1,(HL)
case 0xCBCF: af_.b.h |= 0x2;NEXT_INSTR;break;// Set
case 0xCBD0: bc_.b.h |= 0x4;NEXT_INSTR;break;// Set
case 0xCBD1: bc_.b.l |= 0x4;NEXT_INSTR;break;// Set
case 0xCBD2: de_.b.h |= 0x4;NEXT_INSTR;break;// Set
case 0xCBD3: de_.b.l |= 0x4;NEXT_INSTR;break;// Set
case 0xCBD4: hl_.b.h |= 0x4;NEXT_INSTR;break;// Set
case 0xCBD5: hl_.b.l |= 0x4;NEXT_INSTR;break;// Set
case 0xCBD6: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break; // Set 2,(HL)
case 0xCBD7: af_.b.h |= 0x4;NEXT_INSTR;break;// Set
case 0xCBD8: bc_.b.h |= 0x8;NEXT_INSTR;break;// Set
case 0xCBD9: bc_.b.l |= 0x8;NEXT_INSTR;break;// Set
case 0xCBDA: de_.b.h |= 0x8;NEXT_INSTR;break;// Set
case 0xCBDB: de_.b.l |= 0x8;NEXT_INSTR;break;// Set
case 0xCBDC: hl_.b.h |= 0x8;NEXT_INSTR;break;// Set
case 0xCBDD: hl_.b.l |= 0x8;NEXT_INSTR;break;// Set
case 0xCBDE: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break; // Set 3,(HL)
case 0xCBDF: af_.b.h |= 0x8;NEXT_INSTR;break;// Set
case 0xCBE0: bc_.b.h |= 0x10;NEXT_INSTR;break;// Set
case 0xCBE1: bc_.b.l |= 0x10;NEXT_INSTR;break;// Set
case 0xCBE2: de_.b.h |= 0x10;NEXT_INSTR;break;// Set
case 0xCBE3: de_.b.l |= 0x10;NEXT_INSTR;break;// Set
case 0xCBE4: hl_.b.h |= 0x10;NEXT_INSTR;break;// Set
case 0xCBE5: hl_.b.l |= 0x10;NEXT_INSTR;break;// Set
case 0xCBE6: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break; // Set 4,(HL)
case 0xCBE7: af_.b.h |= 0x10;NEXT_INSTR;break;// Set
case 0xCBE8: bc_.b.h |= 0x20;NEXT_INSTR;break;// Set
case 0xCBE9: bc_.b.l |= 0x20;NEXT_INSTR;break;// Set
case 0xCBEA: de_.b.h |= 0x20;NEXT_INSTR;break;// Set
case 0xCBEB: de_.b.l |= 0x20;NEXT_INSTR;break;// Set
case 0xCBEC: hl_.b.h |= 0x20;NEXT_INSTR;break;// Set
case 0xCBED: hl_.b.l |= 0x20;NEXT_INSTR;break;// Set
case 0xCBEE: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break; // Set 5,(HL)
case 0xCBEF: af_.b.h |= 0x20;NEXT_INSTR;break;// Set
case 0xCBF0: bc_.b.h |= 0x40;NEXT_INSTR;break;// Set
case 0xCBF1: bc_.b.l |= 0x40;NEXT_INSTR;break;// Set
case 0xCBF2: de_.b.h |= 0x40;NEXT_INSTR;break;// Set
case 0xCBF3: de_.b.l |= 0x40;NEXT_INSTR;break;// Set
case 0xCBF4: hl_.b.h |= 0x40;NEXT_INSTR;break;// Set
case 0xCBF5: hl_.b.l |= 0x40;NEXT_INSTR;break;// Set
case 0xCBF6: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break; // Set 6,(HL)
case 0xCBF7: af_.b.h |= 0x40;NEXT_INSTR;break;// Set
case 0xCBF8: bc_.b.h |= 0x80;NEXT_INSTR;break;// Set
case 0xCBF9: bc_.b.l |= 0x80;NEXT_INSTR;break;// Set
case 0xCBFA: de_.b.h |= 0x80;NEXT_INSTR;break;// Set
case 0xCBFB: de_.b.l |= 0x80;NEXT_INSTR;break;// Set
case 0xCBFC: hl_.b.h |= 0x80;NEXT_INSTR;break;// Set
case 0xCBFD: hl_.b.l |= 0x80;NEXT_INSTR;break;// Set
case 0xCBFE: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break; // Set 7,(HL)
case 0xCBFF: af_.b.h |= 0x80;NEXT_INSTR;break;// Set

   // DD
case 0xDD09: ADD_FLAG_W(ix_.w, bc_.w); t_ = 4 + 3; machine_cycle_ = M_Z80_WORK; break; // ADD IX, BC
case 0xDD19: ADD_FLAG_W(ix_.w, de_.w); t_ = 4 + 3; machine_cycle_ = M_Z80_WORK; break; // ADD IX, DE
case 0xDD21: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD_IX, nn
case 0xDD22: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break;// LD (nn), IX
case 0xDD23: if (t_ == 6) {++ix_.w; NEXT_INSTR;}else{++t_;};break;                     // INC_IX
case 0xDD24: INC_FLAGS(ix_.b.h);NEXT_INSTR; break;                                                           // INC IXh
case 0xDD25: DEC_FLAGS(ix_.b.h);NEXT_INSTR; break;                                                           // DEC IXh
case 0xDD26: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break;   // LD IXh, n
case 0xDD29: ADD_FLAG_W(ix_.w, ix_.w); t_ = 4 + 3; machine_cycle_ = M_Z80_WORK; break;                             // ADD IX, IX
case 0xDD2A: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break;   // LD IX, (nn)
case 0xDD2B: if (t_ == 6) {--ix_.w; NEXT_INSTR;}else{++t_;};break;                                                 // DEC_IX
case 0xDD2C: INC_FLAGS(ix_.b.l);NEXT_INSTR; break;                                                           // INC IXl
case 0xDD2D: DEC_FLAGS(ix_.b.l);NEXT_INSTR; break;                                                           // DEC IXl
case 0xDD2E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break;   // LD IXl, n
case 0xDD34: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // INC (IX+d)
case 0xDD35: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // DEC (IX+d)
case 0xDD36: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD (IX+d), n
case 0xDD39: ADD_FLAG_W(ix_.w, sp_); t_ = 4 + 3; machine_cycle_ = M_Z80_WORK; break;                               // ADD IX, SP
case 0xDD44: bc_.b.h = ix_.b.h;NEXT_INSTR;break;                                                                         // LD B, IXh
case 0xDD45: bc_.b.h = ix_.b.l;NEXT_INSTR;break;                                                                         // LD B, IXl
case 0xDD46: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD B, (IX+n)
case 0xDD4C: bc_.b.l = ix_.b.h;NEXT_INSTR;break;                                                                         // LD C, IXh
case 0xDD4D: bc_.b.l = ix_.b.l;NEXT_INSTR;break;                                                                         // LD C, IXl
case 0xDD4E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD C, (IX+n)
case 0xDD54: de_.b.h = ix_.b.h;NEXT_INSTR;break;                                                                         // LD D, IXh
case 0xDD55: de_.b.h = ix_.b.l;NEXT_INSTR;break;                                                                         // LD D, IXl
case 0xDD56: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD D, (IX+n)
case 0xDD5C: de_.b.l = ix_.b.h;NEXT_INSTR;break;                                                                         // LD E, IXh
case 0xDD5D: de_.b.l = ix_.b.l;NEXT_INSTR;break;                                                                         // LD E, IXl
case 0xDD5E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD E, (IX+n)
case 0xDD60: ix_.b.h = bc_.b.h ;NEXT_INSTR;break;                                                                        // LD IXh, B
case 0xDD61: ix_.b.h = bc_.b.l ;NEXT_INSTR;break;                                                                        // LD IXh, C
case 0xDD62: ix_.b.h = de_.b.h ;NEXT_INSTR;break;                                                                        // LD IXh, D
case 0xDD63: ix_.b.h = de_.b.l ;NEXT_INSTR;break;                                                                      // LD IXh, E
case 0xDD64: /*ix_.b.h = ix_.b.h*/ ;NEXT_INSTR;break;                                                                 // LD IXh, IXh
case 0xDD65: ix_.b.h = ix_.b.l ;NEXT_INSTR;break;                                                                 // LD IXh, IXl
case 0xDD66: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD H, (IX+n)
case 0xDD67: ix_.b.h = af_.b.h ;NEXT_INSTR;break;                                                                      // LD IXh, A
case 0xDD68: ix_.b.l = bc_.b.h ;NEXT_INSTR;break;                                                                      // LD IXl, B
case 0xDD69: ix_.b.l = bc_.b.l ;NEXT_INSTR;break;                                                                      // LD IXl, C
case 0xDD6A: ix_.b.l = de_.b.h ;NEXT_INSTR;break;                                                                      // LD IXl, D
case 0xDD6B: ix_.b.l = de_.b.l ;NEXT_INSTR;break;                                                                      // LD IXl, E
case 0xDD6C: ix_.b.l = ix_.b.h ;NEXT_INSTR;break;                                                                 // LD IXl, IXh
case 0xDD6D: /*ix_.b.l = ix_.b.l ;*/NEXT_INSTR;break;                                                                 // LD IXl, IXl
case 0xDD6E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD L, (IX+n)
case 0xDD6F: ix_.b.l = af_.b.h ;NEXT_INSTR;break;                                                                      // LD IXl, A
case 0xDD70: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD (IX+d), B
case 0xDD71: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD (IX+d), C
case 0xDD72: TraceTape(pc_, de_.b.h); machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD (IX+d), D
case 0xDD73: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD (IX+d), E
case 0xDD74: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD (IX+d), H
case 0xDD75:
   TraceTape(pc_, hl_.b.l);

   machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD (IX+d), L

case 0xDD77:
   TraceTape (pc_, af_.b.h);
   machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD (IX+d), A
case 0xDD7C: af_.b.h = ix_.b.h;NEXT_INSTR;break;                                                                       // LD A,IXh
case 0xDD7D: af_.b.h = ix_.b.l;NEXT_INSTR;break;                                                                       // LD A,IXl
case 0xDD7E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD A, (IX+n)
case 0xDD84: ADD_FLAG(ix_.b.h);NEXT_INSTR;break;                                                                   // ADD A,IXh
case 0xDD85: ADD_FLAG(ix_.b.l);NEXT_INSTR;break;                                                                   // ADD A,IXl
case 0xDD86: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; //ADD A, (IX+d)
case 0xDD8C: ADD_FLAG_CARRY(ix_.b.h);NEXT_INSTR;break;                                                             // ADC A,IXh
case 0xDD8D: ADD_FLAG_CARRY(ix_.b.l);NEXT_INSTR;break;                                                             // ADC A,IXl
case 0xDD8E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; //ADC A, (IX+d)
case 0xDD94: SUB_FLAG(ix_.b.h);NEXT_INSTR;break;                                                                   // SUB A,IXh
case 0xDD95: SUB_FLAG(ix_.b.l);NEXT_INSTR;break;                                                                   // SUB A,IXl
case 0xDD96: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // SUB A, (IX+d)
case 0xDD9C: SUB_FLAG_CARRY(ix_.b.h);NEXT_INSTR;break;                                                             // SBC A,IXh
case 0xDD9D: SUB_FLAG_CARRY(ix_.b.l);NEXT_INSTR;break;                                                             // SBC A,IXl
case 0xDD9E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // SBC A, (IX+d)
case 0xDDA4: AND_FLAGS(ix_.b.h);NEXT_INSTR;break;                                                                     // AND IXh
case 0xDDA5: AND_FLAGS(ix_.b.l);NEXT_INSTR;break;                                                                     // AND IXl
case 0xDDA6: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break;  // AND (IX+d)
case 0xDDAC: XOR_FLAGS(ix_.b.h);NEXT_INSTR;break;                                                                     // XOR IXh
case 0xDDAD: XOR_FLAGS(ix_.b.l);NEXT_INSTR;break;                                                                     // XOR IXl
case 0xDDAE: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // XOR (IX+d)
case 0xDDB4: OR_FLAGS(ix_.b.h);NEXT_INSTR;break;                                                                     // OR IXh
case 0xDDB5: OR_FLAGS(ix_.b.l);NEXT_INSTR;break;                                                                     // OR IXl
case 0xDDB6: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // OR (IX+n)
case 0xDDBC: CP_FLAGS(ix_.b.h);NEXT_INSTR;break;                                                                     // CP IXh
case 0xDDBD: CP_FLAGS(ix_.b.l);NEXT_INSTR;break;                                                                     // CP IXl
case 0xDDBE: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // CP(IX+d)
case 0xDDCB : machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break;// DD CB - Lecture de d
case 0xDDE1: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_++; current_data_ = 0; read_count_ = 0; break;// POP IX
case 0xDDE3: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_; current_data_ = 0;read_count_ = 0;break;    // EX (SP), IX
case 0xDDE5: if (t_ == 5) { machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = --sp_; current_data_ = ix_.w >>8; read_count_ = 0; }else {++t_;} break;// PUSH IX
case 0xDDE9: pc_ = ix_.w;NEXT_INSTR;break;                                                                           // JP (IX)
case 0xDDF9: if (t_ == 6) { sp_ = ix_.w; NEXT_INSTR;}else{++t_;}break;// LD SP, IX
   //////////////////////////////////////////////////////////
   // ED
case 0xED40: machine_cycle_ = M_IO_R; t_ = 1; current_address_ = bc_.w; break; // IN B, (C)
case 0xED41: machine_cycle_ = M_IO_W; t_ = 1; current_address_ = bc_.w; current_data_ = bc_.b.h;q_ = af_.b.l| ((bc_.b.h & 0x80) ? NF : 0); af_.b.l = q_;break; // OUT (C), B
case 0xED42: mem_ptr_.w = hl_.w+1;SUB_FLAG_CARRY_W (hl_.w, bc_.w);machine_cycle_ = M_Z80_WORK; t_ = 4+3;break;// SBC HL,BC
case 0xED43: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break;   // LD (nn), BC
case 0xED44: NEG;NEXT_INSTR;break;// NEG
case 0xED45: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_++; current_data_ = 0; read_count_ = 0; break;   // RETN
case 0xED46: interrupt_mode_ = 0;NEXT_INSTR;break;// IM 0
case 0xED47: if (t_ == 5) { ir_.b.h = af_.b.h;NEXT_INSTR;}else {++t_;} break;    // LD I, A
case 0xED48: machine_cycle_ = M_IO_R; t_ = 1; current_address_ = bc_.w; break; // IN C, (C)
case 0xED49: machine_cycle_ = M_IO_W; t_ = 1; current_address_ = bc_.w; current_data_ = bc_.b.l;q_ = af_.b.l| ((bc_.b.l & 0x80) ? NF : 0); af_.b.l = q_;break; // OUT (C), C
case 0xED4A: mem_ptr_.w = hl_.w+1;ADD_FLAG_CARRY_W (hl_.w, bc_.w);machine_cycle_ = M_Z80_WORK; t_ = 4+3;break; // ADC HL,BC
case 0xED4B: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break;// LD BC, (nn)
case 0xED4C: NEG;NEXT_INSTR;break;// NEG
case 0xED4D: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_++; current_data_ = 0; read_count_ = 0; break;   // RETI
case 0xED4E: interrupt_mode_ = 0;NEXT_INSTR;break;// IM 0
case 0xED4F: if (t_ == 5) { ir_.b.l = af_.b.h;NEXT_INSTR;}else {++t_;} break;    // LD R, A
case 0xED50: machine_cycle_ = M_IO_R; t_ = 1; current_address_ = bc_.w; break; // IN D, (C)
case 0xED51: machine_cycle_ = M_IO_W; t_ = 1; current_address_ = bc_.w; current_data_ = de_.b.h;q_ = af_.b.l| ((de_.b.h & 0x80) ? NF : 0); af_.b.l = q_;break; // OUT (C), D
case 0xED52: mem_ptr_.w = hl_.w+1;SUB_FLAG_CARRY_W (hl_.w, de_.w);machine_cycle_ = M_Z80_WORK; t_ = 4+3;break;// SBC HL,DE
case 0xED53:machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break;   // LD (nn), DE
case 0xED54: NEG;NEXT_INSTR;break;// NEG
case 0xED55: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_++; current_data_ = 0; read_count_ = 0; break;   // RETN
case 0xED56: interrupt_mode_ = 1;NEXT_INSTR;break;// IM 1
case 0xED57: if (t_ == 5) {af_.b.h = ir_.b.h;q_ = af_.b.l;q_ &= ~(HF | NF | 0x28);ZERO_FLAG(af_.b.h);SIGN_FLAG(af_.b.h);if (iff2_)
      q_ |= PF;
   else
      q_ &= ~(PF);
   q_ |= (af_.b.h & 0x28);
   af_.b.l = q_;
   carry_set_ = true;
   NEXT_INSTR_LDAIR }else {++t_;}; break;// LD A, I
case 0xED58: machine_cycle_ = M_IO_R; t_ = 1; current_address_ = bc_.w; break; // IN E, (C)
case 0xED59: machine_cycle_ = M_IO_W; t_ = 1; current_address_ = bc_.w; current_data_ = de_.b.l;q_ = af_.b.l| ((de_.b.l & 0x80) ? NF : 0); af_.b.l = q_;break; // OUT (C), E
case 0xED5A: mem_ptr_.w = hl_.w+1;ADD_FLAG_CARRY_W (hl_.w, de_.w);machine_cycle_ = M_Z80_WORK; t_ = 4+3;break; // ADC HL,DE
case 0xED5B: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD DE, (nn)
case 0xED5C: NEG;NEXT_INSTR;break;// NEG
case 0xED5D: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_++; current_data_ = 0; read_count_ = 0; break;   // RETN
case 0xED5E: interrupt_mode_ = 2;NEXT_INSTR;break;// IM 2
case 0xED5F: if (t_ == 5)
{
   af_.b.h = ir_.b.l;q_ = af_.b.l;q_ &= ~(HF | NF | 0x28);ZERO_FLAG(af_.b.h);SIGN_FLAG(af_.b.h);
   if (iff2_)
      q_ |= PF;
   else
      q_ &= ~(PF);
   q_ |= (af_.b.h & 0x28);
   af_.b.l = q_;
   NEXT_INSTR_LDAIR
}
             else { ++t_; return 1; }; break;// LD A, R
case 0xED60: machine_cycle_ = M_IO_R; t_ = 1; current_address_ = bc_.w; break; // IN H, (C)
case 0xED61: machine_cycle_ = M_IO_W; t_ = 1; current_address_ = bc_.w; current_data_ = hl_.b.h;q_ = af_.b.l| ((hl_.b.h & 0x80) ? NF : 0); af_.b.l = q_;break; // OUT (C), H
case 0xED62: mem_ptr_.w = hl_.w+1;SUB_FLAG_CARRY_W (hl_.w, hl_.w);machine_cycle_ = M_Z80_WORK; t_ = 4+3;break;// SBC HL,HL
case 0xED63:machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break;   // LD (nn), HL
case 0xED64: NEG;NEXT_INSTR;break;// NEG
case 0xED65: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_++; current_data_ = 0; read_count_ = 0; break;   // RETN
case 0xED66: interrupt_mode_ = 0;NEXT_INSTR;break;// IM 0
case 0xED67: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break;    //RRD
case 0xED68: machine_cycle_ = M_IO_R; t_ = 1; current_address_ = bc_.w; break; // IN L, (C)
case 0xED69: machine_cycle_ = M_IO_W; t_ = 1; current_address_ = bc_.w; current_data_ = hl_.b.l;q_ = af_.b.l| ((hl_.b.l & 0x80) ? NF : 0); af_.b.l = q_;break; // OUT (C), L
case 0xED6A: mem_ptr_.w = hl_.w+1;ADD_FLAG_CARRY_W (hl_.w, hl_.w);machine_cycle_ = M_Z80_WORK; t_ = 4+3;break; // ADC HL,HL
case 0xED6B: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break;// LD HL, (nn)
case 0xED6C: NEG;NEXT_INSTR;break;// NEG
case 0xED6D: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_++; current_data_ = 0; read_count_ = 0; break;   // RETN
case 0xED6E: interrupt_mode_ = 0;NEXT_INSTR;break;// IM 0
case 0xED6F: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break; // RLD
case 0xED70: machine_cycle_ = M_IO_R; t_ = 1; current_address_ = bc_.w; break; // IN (C)
case 0xED71: machine_cycle_ = M_IO_W; t_ = 1; current_address_ = bc_.w; current_data_ = 0; q_ = af_.b.l; q_ |= (0 & 0x80) ? NF : 0; af_.b.l = q_; break; // OUT (C), 0
case 0xED72: mem_ptr_.w = hl_.w+1;SUB_FLAG_CARRY_W (hl_.w, sp_);machine_cycle_ = M_Z80_WORK; t_ = 4+3;break;// SBC HL,SP
case 0xED73: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break;   // LD (nn), SP
case 0xED74: NEG;NEXT_INSTR;break;// NEG
case 0xED75: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_++; current_data_ = 0; read_count_ = 0; break;   // RETN
case 0xED76: interrupt_mode_ = 1;NEXT_INSTR;break;// IM 1
case 0xED77: NEXT_INSTR; break;                                                                                        // NOP
case 0xED78: machine_cycle_ = M_IO_R; t_ = 1; current_address_ = bc_.w; break; // IN A, (C)
case 0xED79: machine_cycle_ = M_IO_W; t_ = 1; current_address_ = bc_.w; current_data_ = af_.b.h;q_ = af_.b.l| ((af_.b.h & 0x80) ? NF : 0); mem_ptr_.w = bc_.w + 1; af_.b.l = q_;break; // OUT (C), A
case 0xED7A: mem_ptr_.w = hl_.w+1;ADD_FLAG_CARRY_W (hl_.w, sp_);machine_cycle_ = M_Z80_WORK; t_ = 4+3;break; // ADC HL,SP
case 0xED7B: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD SP, (nn)
case 0xED7C: NEG;NEXT_INSTR;break;// NEG
case 0xED7D: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_++; current_data_ = 0; read_count_ = 0; break;   // RETN
case 0xED7E: interrupt_mode_ = 2;NEXT_INSTR;break;// IM 2
case 0xED7F : NEXT_INSTR; break;                                                                                        // NOP
case 0xEDA0: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break;   // LDI
case 0xEDA1: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break; // CPI
case 0xEDA2: if (t_ == 5) { machine_cycle_ = M_IO_R; t_ = 1; current_address_ = bc_.w; }else { ++t_; }; break;       // INI
case 0xEDA3: if (t_ == 5) { machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; }else {++t_;} break; // OUTI
case 0xEDA8: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break; // LDD
case 0xEDA9: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break; // CPD
case 0xEDAA: if (t_ == 5) { machine_cycle_ = M_IO_R; t_ = 1; current_address_ = bc_.w; }else { ++t_; }; break;       // IND
case 0xEDAB: if (t_ == 5) { machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; }
             else { ++t_; } break;// OUTD
case 0xEDB0: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break;   // LDIR
case 0xEDB1: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break; // CPIR
case 0xEDB2: if (t_ == 5) { machine_cycle_ = M_IO_R; t_ = 1; current_address_ = bc_.w; }else { ++t_; }; break;       // INIR
case 0xEDB3: if (t_ == 5) { machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0;}
             else { ++t_; };  break; // OTIR
case 0xEDB8: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0;read_count_ = 0;break;   // LDDR
case 0xEDB9: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break; // CPDR
case 0xEDBA: if (t_ == 5) { machine_cycle_ = M_IO_R; t_ = 1; current_address_ = bc_.w; }else { ++t_; }; break;       // INDR
case 0xEDBB: if (t_ == 5) { machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; }
            else { ++t_; };  break; // OTDR
case 0xEDED: NEXT_INSTR; break;                                                                  // ED Extension opcode
   //////////////////////////////////////////////////////////
   // FD
case 0xFD09: ADD_FLAG_W(iy_.w, bc_.w); t_ = 4 + 3; machine_cycle_ = M_Z80_WORK; break; // ADD IY, BC
case 0xFD19: ADD_FLAG_W(iy_.w, de_.w); t_ = 4 + 3; machine_cycle_ = M_Z80_WORK; break; // ADD IY, DE
case 0xFD21: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break;// LD IY, nn
case 0xFD22: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break;// LD (nn), IY
case 0xFD23: if (t_ == 6) {++iy_.w; NEXT_INSTR;}else{++t_;};break;                     // INC_IY
case 0xFD24: INC_FLAGS(iy_.b.h);NEXT_INSTR; break;                                                           // INC IYh
case 0xFD25: DEC_FLAGS(iy_.b.h);NEXT_INSTR; break;                                                           // DEC IYh
case 0xFD26: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break;   // LD IYh, n
case 0xFD29: ADD_FLAG_W(iy_.w, iy_.w); t_ = 4 + 3; machine_cycle_ = M_Z80_WORK; break;                             // ADD IY, IY
case 0xFD2A: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break;   // LD IY, (nn)
case 0xFD2B: if (t_ == 6) {--iy_.w; NEXT_INSTR;}else{++t_;};break;                                                 // DEC_IY
case 0xFD2C: INC_FLAGS(iy_.b.l);NEXT_INSTR; break;                                                           // INC IYl
case 0xFD2D: DEC_FLAGS(iy_.b.l);NEXT_INSTR; break;                                                           // DEC IYl
case 0xFD2E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break;   // LD IYl, n
case 0xFD34: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // INC (IY+d)
case 0xFD35: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // DEC (IY+d)
case 0xFD36: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD (IY+d), n
case 0xFD39: ADD_FLAG_W(iy_.w, sp_); t_ = 4 + 3; machine_cycle_ = M_Z80_WORK; break;                               // ADD IY, SP
case 0xFD44: bc_.b.h = iy_.b.h;NEXT_INSTR;break;                                                                         // LD B, IYh
case 0xFD45: bc_.b.h = iy_.b.l;NEXT_INSTR;break;                                                                         // LD B, IYl
case 0xFD46: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD B, (IY+n)
case 0xFD4C: bc_.b.l = iy_.b.h;NEXT_INSTR;break;                                                                         // LD C, IYh
case 0xFD4D: bc_.b.l = iy_.b.l;NEXT_INSTR;break;                                                                         // LD C, IYl
case 0xFD4E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD C, (IY+n)
case 0xFD54: de_.b.h = iy_.b.h;NEXT_INSTR;break;                                                                       // LD D, IYh
case 0xFD55: de_.b.h = iy_.b.l;NEXT_INSTR;break;                                                                       // LD D, IYl
case 0xFD56: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD D, (IY+n)
case 0xFD5C: de_.b.l = iy_.b.h;NEXT_INSTR;break;                                                                       // LD E, IYh
case 0xFD5D: de_.b.l = iy_.b.l;NEXT_INSTR;break;                                                                       // LD E, IYl
case 0xFD5E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD E, (IY+n)
case 0xFD60: iy_.b.h = bc_.b.h ;NEXT_INSTR;break;                                                                      // LD IYh, B
case 0xFD61: iy_.b.h = bc_.b.l ;NEXT_INSTR;break;                                                                      // LD IYh, C
case 0xFD62: iy_.b.h = de_.b.h ;NEXT_INSTR;break;                                                                      // LD IYh, D
case 0xFD63: iy_.b.h = de_.b.l ;NEXT_INSTR;break;                                                                      // LD IYh, E
case 0xFD64: /*iy_.b.h = iy_.b.h */;NEXT_INSTR;break;                                                                 // LD IYh, IYh
case 0xFD65: iy_.b.h = iy_.b.l ;NEXT_INSTR;break;                                                                 // LD IYh, IYl
case 0xFD66: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD H, (IY+n)
case 0xFD67: iy_.b.h = af_.b.h ;NEXT_INSTR;break;                                                                      // LD IYh, A
case 0xFD68: iy_.b.l = bc_.b.h ;NEXT_INSTR;break;                                                                      // LD IYl, B
case 0xFD69: iy_.b.l = bc_.b.l ;NEXT_INSTR;break;                                                                      // LD IYl, C
case 0xFD6A: iy_.b.l = de_.b.h ;NEXT_INSTR;break;                                                                      // LD IYl, D
case 0xFD6B: iy_.b.l = de_.b.l ;NEXT_INSTR;break;                                                                      // LD IYl, E
case 0xFD6C: iy_.b.l = iy_.b.h ;NEXT_INSTR;break;                                                                 // LD IYl, IYh
case 0xFD6D: /*iy_.b.l = iy_.b.l */;NEXT_INSTR;break;                                                                 // LD IYl, IYl
case 0xFD6E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD L, (IY+n)
case 0xFD6F: iy_.b.l = af_.b.h ;NEXT_INSTR;break;                                                                      // LD IYl, A
case 0xFD70: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD (IY+d), B
case 0xFD71: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD (IY+d), C
case 0xFD72: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD (IY+d), D
case 0xFD73: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD (IY+d), E
case 0xFD74: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD (IY+d), H
case 0xFD75: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD (IY+d), L
case 0xFD77: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD (IY+d), A
case 0xFD7C: af_.b.h = iy_.b.h;NEXT_INSTR;break;                                                                       // LD A,IYh
case 0xFD7D: af_.b.h = iy_.b.l;NEXT_INSTR;break;                                                                       // LD A,IYl
case 0xFD7E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // LD A, (IY+n)
case 0xFD84: ADD_FLAG(iy_.b.h);NEXT_INSTR;break;                                                                   // ADD A,IYh
case 0xFD85: ADD_FLAG(iy_.b.l);NEXT_INSTR;break;                                                                   // ADD A,IYl
case 0xFD86: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; //ADD A, (IY+d)
case 0xFD8C: ADD_FLAG_CARRY(iy_.b.h);NEXT_INSTR;break;                                                             // ADC A,IYh
case 0xFD8D: ADD_FLAG_CARRY(iy_.b.l);NEXT_INSTR;break;                                                             // ADC A,IYl
case 0xFD8E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // ADC A, (IY+d)
case 0xFD94: SUB_FLAG(iy_.b.h);NEXT_INSTR;break;                                                                   // SUB A,IYh
case 0xFD95: SUB_FLAG(iy_.b.l);NEXT_INSTR;break;                                                                   // SUB A,IYl
case 0xFD96: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // SUB A, (IY+d)
case 0xFD9C: SUB_FLAG_CARRY(iy_.b.h);NEXT_INSTR;break;                                                             // SBC A,IYh
case 0xFD9D: SUB_FLAG_CARRY(iy_.b.l);NEXT_INSTR;break;                                                             // SBC A,IYl
case 0xFD9E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0;read_count_ = 0;break; // SBC A, (IY+d)
case 0xFDA4: AND_FLAGS(iy_.b.h);NEXT_INSTR;break;                                                                     // AND IYh
case 0xFDA5: AND_FLAGS(iy_.b.l);NEXT_INSTR;break;                                                                     // AND IYl
case 0xFDA6: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break;  // AND (IY+d)
case 0xFDAC: XOR_FLAGS(iy_.b.h);NEXT_INSTR;break;                                                                     // XOR IYh
case 0xFDAD: XOR_FLAGS(iy_.b.l);NEXT_INSTR;break;                                                                     // XOR IYl
case 0xFDAE: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // XOR (IY+d)
case 0xFDB4: OR_FLAGS(iy_.b.h);NEXT_INSTR;break;                                                                     // OR IYh
case 0xFDB5: OR_FLAGS(iy_.b.l);NEXT_INSTR;break;                                                                     // OR IYl
case 0xFDB6: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // OR (IY+n)
case 0xFDBC: CP_FLAGS(iy_.b.h);NEXT_INSTR;break;                                                                     // CP IYh
case 0xFDBD: CP_FLAGS(iy_.b.l);NEXT_INSTR;break;                                                                     // CP IYl
case 0xFDBE: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // CP(IY+d)
case 0xFDCB : machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break;// FD CB - Lecture de d
case 0xFDE1: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_++; current_data_ = 0; read_count_ = 0; break;// POP IY
case 0xFDE3: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_; current_data_ = 0;read_count_ = 0;break;    // EX (SP), IY
case 0xFDE5: if (t_ == 5) { machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = --sp_; current_data_ = iy_.w >> 8; read_count_ = 0; }else { ++t_; } break;// PUSH IY
case 0xFDE9: pc_ = iy_.w;NEXT_INSTR;break;                                                                           // JP (IY)
case 0xFDF9: if (t_ == 6) { sp_ = iy_.w; NEXT_INSTR;}else{++t_;}break;// LD SP, IY
default:
   if (((current_opcode_ & 0xFF00) == 0xFD00)
      || ((current_opcode_ & 0xFF00) == 0xDD00))
   {
      current_opcode_ &= 0xFF;
      goto Begin;
   }
   else
   {
      if ((current_opcode_ & 0xFF00) != 0xED00)
        {
        // Assert !
        }
/*#ifdef _WIN32
         _CrtDbgBreak();
#endif*/
   }

   NEXT_INSTR;
   break;
}
