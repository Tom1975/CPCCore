
Begin:

switch (current_opcode_) {
//////////////////////////////////////////////////////////
// CB
/// RLC
case 0xCB00: RLC(bc_.b.h); NEXT_INSTR; break; // RLC B
case 0xCB01: RLC(bc_.b.l); NEXT_INSTR; break; // RLC C
case 0xCB02: RLC(de_.b.h); NEXT_INSTR; break; // RLC D
case 0xCB03: RLC(de_.b.l); NEXT_INSTR; break; // RLC E
case 0xCB04: RLC(hl_.b.h); NEXT_INSTR; break; // RLC H
case 0xCB05: RLC(hl_.b.l); NEXT_INSTR; break; // RLC L
case 0xCB06: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break;  // RLC (HL)
case 0xCB07: RLC(af_.b.h); NEXT_INSTR; break; // RLC A
/// RRC
case 0xCB08: RRC(bc_.b.h); NEXT_INSTR; break; // RRC B
case 0xCB09: RRC(bc_.b.l); NEXT_INSTR; break; // RRC C
case 0xCB0A: RRC(de_.b.h); NEXT_INSTR; break; // RRC D
case 0xCB0B: RRC(de_.b.l); NEXT_INSTR; break; // RRC E
case 0xCB0C: RRC(hl_.b.h); NEXT_INSTR; break; // RRC H
case 0xCB0D: RRC(hl_.b.l); NEXT_INSTR; break; // RRC L
case 0xCB0E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break;  // RRC (HL)
case 0xCB0F: RRC(af_.b.h); NEXT_INSTR; break; // RRC A
/// RL
case 0xCB10:RL(bc_.b.h); NEXT_INSTR; break; // RL B
case 0xCB11:RL(bc_.b.l); NEXT_INSTR; break; // RL C
case 0xCB12:RL(de_.b.h); NEXT_INSTR; break; // RL D
case 0xCB13:RL(de_.b.l); NEXT_INSTR; break; // RL E
case 0xCB14:RL(hl_.b.h); NEXT_INSTR; break; // RL H
case 0xCB15:RL(hl_.b.l); NEXT_INSTR; break; // RL L
case 0xCB16: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break;  // RL (HL)
case 0xCB17:RL(af_.b.h); NEXT_INSTR; break; // RL A
/// RR
case 0xCB18:RR(bc_.b.h); NEXT_INSTR; break; // RR B
case 0xCB19:RR(bc_.b.l); NEXT_INSTR; break; // RR C
case 0xCB1A:RR(de_.b.h); NEXT_INSTR; break; // RR D
case 0xCB1B:RR(de_.b.l); NEXT_INSTR; break; // RR E
case 0xCB1C:RR(hl_.b.h); NEXT_INSTR; break; // RR H
case 0xCB1D:RR(hl_.b.l); NEXT_INSTR; break; // RR L
case 0xCB1E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break;  // RR (HL)
case 0xCB1F:RR(af_.b.h); NEXT_INSTR; break; // RR A
/// SLA
case 0xCB20:SLA(bc_.b.h); NEXT_INSTR; break; // SLA B
case 0xCB21:SLA(bc_.b.l); NEXT_INSTR; break; // SLA C
case 0xCB22:SLA(de_.b.h); NEXT_INSTR; break; // SLA D
case 0xCB23:SLA(de_.b.l); NEXT_INSTR; break; // SLA E
case 0xCB24:SLA(hl_.b.h); NEXT_INSTR; break; // SLA H
case 0xCB25:SLA(hl_.b.l); NEXT_INSTR; break; // SLA L
case 0xCB26: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break;  // SLA (HL)
case 0xCB27:SLA(af_.b.h); NEXT_INSTR; break; // SLA A
/// SRA
case 0xCB28:SRA(bc_.b.h); NEXT_INSTR; break; // SRA B
case 0xCB29:SRA(bc_.b.l); NEXT_INSTR; break; // SRA C
case 0xCB2A:SRA(de_.b.h); NEXT_INSTR; break; // SRA D
case 0xCB2B:SRA(de_.b.l); NEXT_INSTR; break; // SRA E
case 0xCB2C:SRA(hl_.b.h); NEXT_INSTR; break; // SRA H
case 0xCB2D:SRA(hl_.b.l); NEXT_INSTR; break; // SRA L
case 0xCB2E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break;  // SRA (HL)
case 0xCB2F:SRA(af_.b.h); NEXT_INSTR; break; // SRA A
/// SLL
case 0xCB30:SLL(bc_.b.h); NEXT_INSTR; break; // SLL B
case 0xCB31:SLL(bc_.b.l); NEXT_INSTR; break; // SLL C
case 0xCB32:SLL(de_.b.h); NEXT_INSTR; break; // SLL D
case 0xCB33:SLL(de_.b.l); NEXT_INSTR; break; // SLL E
case 0xCB34:SLL(hl_.b.h); NEXT_INSTR; break; // SLL H
case 0xCB35:SLL(hl_.b.l); NEXT_INSTR; break; // SLL L
case 0xCB36: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break;  // SLL (HL)
case 0xCB37:SLL(af_.b.h); NEXT_INSTR; break; // SLL A
/// SRL
case 0xCB38:SRL(bc_.b.h); NEXT_INSTR; break; // SRL B
case 0xCB39:SRL(bc_.b.l); NEXT_INSTR; break; // SRL C
case 0xCB3A:SRL(de_.b.h); NEXT_INSTR; break; // SRL D
case 0xCB3B:SRL(de_.b.l); NEXT_INSTR; break; // SRL E
case 0xCB3C:SRL(hl_.b.h); NEXT_INSTR; break; // SRL H
case 0xCB3D:SRL(hl_.b.l); NEXT_INSTR; break; // SRL L
case 0xCB3E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break;  // SRL (HL)
case 0xCB3F:SRL(af_.b.h); NEXT_INSTR; break; // SRL A
/// BIT
case 0xCB40: BIT_B_R(0, bc_.b.h); NEXT_INSTR; break; // BIT 0, B
case 0xCB41: BIT_B_R(0, bc_.b.l); NEXT_INSTR; break; // BIT 0, C
case 0xCB42: BIT_B_R(0, de_.b.h); NEXT_INSTR; break; // BIT 0, D
case 0xCB43: BIT_B_R(0, de_.b.l); NEXT_INSTR; break; // BIT 0, E
case 0xCB44: BIT_B_R(0, hl_.b.h); NEXT_INSTR; break; // BIT 0, H
case 0xCB45: BIT_B_R(0, hl_.b.l); NEXT_INSTR; break; // BIT 0, L
case 0xCB46: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break;// BIT 0, (HL)
case 0xCB47: BIT_B_R(0, af_.b.h); NEXT_INSTR; break; // BIT 1, A
case 0xCB48: BIT_B_R(1, bc_.b.h); NEXT_INSTR; break; // BIT 1, B
case 0xCB49: BIT_B_R(1, bc_.b.l); NEXT_INSTR; break; // BIT 1, C
case 0xCB4A: BIT_B_R(1, de_.b.h); NEXT_INSTR; break; // BIT 1, D
case 0xCB4B: BIT_B_R(1, de_.b.l); NEXT_INSTR; break; // BIT 1, E
case 0xCB4C: BIT_B_R(1, hl_.b.h); NEXT_INSTR; break; // BIT 1, H
case 0xCB4D: BIT_B_R(1, hl_.b.l); NEXT_INSTR; break; // BIT 1, L
case 0xCB4E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break;// BIT 1, (HL)
case 0xCB4F: BIT_B_R(1, af_.b.h); NEXT_INSTR; break; // BIT 1, A
case 0xCB50: BIT_B_R(2, bc_.b.h); NEXT_INSTR; break; // BIT 2, B
case 0xCB51: BIT_B_R(2, bc_.b.l); NEXT_INSTR; break; // BIT 2, C
case 0xCB52: BIT_B_R(2, de_.b.h); NEXT_INSTR; break; // BIT 2, D
case 0xCB53: BIT_B_R(2, de_.b.l); NEXT_INSTR; break; // BIT 2, E
case 0xCB54: BIT_B_R(2, hl_.b.h); NEXT_INSTR; break; // BIT 2, H
case 0xCB55: BIT_B_R(2, hl_.b.l); NEXT_INSTR; break; // BIT 2, L
case 0xCB56: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break;// BIT 2, (HL)
case 0xCB57: BIT_B_R(2, af_.b.h); NEXT_INSTR; break; // BIT 2, A
case 0xCB58: BIT_B_R(3, bc_.b.h); NEXT_INSTR; break; // BIT 3, B
case 0xCB59: BIT_B_R(3, bc_.b.l); NEXT_INSTR; break; // BIT 3, C
case 0xCB5A: BIT_B_R(3, de_.b.h); NEXT_INSTR; break; // BIT 3, D
case 0xCB5B: BIT_B_R(3, de_.b.l); NEXT_INSTR; break; // BIT 3, E
case 0xCB5C: BIT_B_R(3, hl_.b.h); NEXT_INSTR; break; // BIT 3, H
case 0xCB5D: BIT_B_R(3, hl_.b.l); NEXT_INSTR; break; // BIT 3, L
case 0xCB5E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break;// BIT 3, (HL)
case 0xCB5F: BIT_B_R(3, af_.b.h); NEXT_INSTR; break; // BIT 3, A
case 0xCB60: BIT_B_R(4, bc_.b.h); NEXT_INSTR; break; // BIT 4, B
case 0xCB61: BIT_B_R(4, bc_.b.l); NEXT_INSTR; break; // BIT 4, C
case 0xCB62: BIT_B_R(4, de_.b.h); NEXT_INSTR; break; // BIT 4, D
case 0xCB63: BIT_B_R(4, de_.b.l); NEXT_INSTR; break; // BIT 4, E
case 0xCB64: BIT_B_R(4, hl_.b.h); NEXT_INSTR; break; // BIT 4, H
case 0xCB65: BIT_B_R(4, hl_.b.l); NEXT_INSTR; break; // BIT 4, L
case 0xCB66:  machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break; // BIT 4, (HL)
case 0xCB67: BIT_B_R(4, af_.b.h); NEXT_INSTR; break; // BIT 4, A
case 0xCB68: BIT_B_R(5, bc_.b.h); NEXT_INSTR; break; // BIT 5, B
case 0xCB69: BIT_B_R(5, bc_.b.l); NEXT_INSTR; break; // BIT 5, C
case 0xCB6A: BIT_B_R(5, de_.b.h); NEXT_INSTR; break; // BIT 5, D
case 0xCB6B: BIT_B_R(5, de_.b.l); NEXT_INSTR; break; // BIT 5, E
case 0xCB6C: BIT_B_R(5, hl_.b.h); NEXT_INSTR; break; // BIT 5, H
case 0xCB6D: BIT_B_R(5, hl_.b.l); NEXT_INSTR; break; // BIT 5, L
case 0xCB6E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break; // BIT 5, (HL)
case 0xCB6F: BIT_B_R(5, af_.b.h); NEXT_INSTR; break; // BIT 5, A
case 0xCB70: BIT_B_R(6, bc_.b.h); NEXT_INSTR; break; // BIT 6, B
case 0xCB71: BIT_B_R(6, bc_.b.l); NEXT_INSTR; break; // BIT 6, C
case 0xCB72: BIT_B_R(6, de_.b.h); NEXT_INSTR; break; // BIT 6, D
case 0xCB73: BIT_B_R(6, de_.b.l); NEXT_INSTR; break; // BIT 6, E
case 0xCB74: BIT_B_R(6, hl_.b.h); NEXT_INSTR; break; // BIT 6, H
case 0xCB75: BIT_B_R(6, hl_.b.l); NEXT_INSTR; break; // BIT 6, L
case 0xCB76: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break; // BIT 6, (HL)
case 0xCB77: BIT_B_R(6, af_.b.h); NEXT_INSTR; break; // BIT 6, A
case 0xCB78: BIT_B_R(7, bc_.b.h); NEXT_INSTR; break; // BIT 7, B
case 0xCB79: BIT_B_R(7, bc_.b.l); NEXT_INSTR; break; // BIT 7, C
case 0xCB7A: BIT_B_R(7, de_.b.h); NEXT_INSTR; break; // BIT 7, D
case 0xCB7B: BIT_B_R(7, de_.b.l); NEXT_INSTR; break; // BIT 7, E
case 0xCB7C: BIT_B_R(7, hl_.b.h); NEXT_INSTR; break; // BIT 7, H
case 0xCB7D: BIT_B_R(7, hl_.b.l); NEXT_INSTR; break; // BIT 7, L
case 0xCB7E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break; // BIT 7, (HL)
case 0xCB7F: BIT_B_R(7, af_.b.h); NEXT_INSTR; break; // BIT 7, A
/// RES
case 0xCB80: bc_.b.h &= ~0x1; NEXT_INSTR; break;// Set
case 0xCB81: bc_.b.l &= ~0x1; NEXT_INSTR; break;// Set
case 0xCB82: de_.b.h &= ~0x1; NEXT_INSTR; break;// Set
case 0xCB83: de_.b.l &= ~0x1; NEXT_INSTR; break;// Set
case 0xCB84: hl_.b.h &= ~0x1; NEXT_INSTR; break;// Set
case 0xCB85: hl_.b.l &= ~0x1; NEXT_INSTR; break;// Set
case 0xCB86: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break; // Res 0,(HL)
case 0xCB87: af_.b.h &= ~0x1; NEXT_INSTR; break;// Set
case 0xCB88: bc_.b.h &= ~0x2; NEXT_INSTR; break;// Set
case 0xCB89: bc_.b.l &= ~0x2; NEXT_INSTR; break;// Set
case 0xCB8A: de_.b.h &= ~0x2; NEXT_INSTR; break;// Set
case 0xCB8B: de_.b.l &= ~0x2; NEXT_INSTR; break;// Set
case 0xCB8C: hl_.b.h &= ~0x2; NEXT_INSTR; break;// Set
case 0xCB8D: hl_.b.l &= ~0x2; NEXT_INSTR; break;// Set
case 0xCB8E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break; // Res 1,(HL)
case 0xCB8F: af_.b.h &= ~0x2; NEXT_INSTR; break;// Set
case 0xCB90: bc_.b.h &= ~0x4; NEXT_INSTR; break;// Set
case 0xCB91: bc_.b.l &= ~0x4; NEXT_INSTR; break;// Set
case 0xCB92: de_.b.h &= ~0x4; NEXT_INSTR; break;// Set
case 0xCB93: de_.b.l &= ~0x4; NEXT_INSTR; break;// Set
case 0xCB94: hl_.b.h &= ~0x4; NEXT_INSTR; break;// Set
case 0xCB95: hl_.b.l &= ~0x4; NEXT_INSTR; break;// Set
case 0xCB96: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break; // Res 2,(HL)
case 0xCB97: af_.b.h &= ~0x4; NEXT_INSTR; break;// Set
case 0xCB98: bc_.b.h &= ~0x8; NEXT_INSTR; break;// Set
case 0xCB99: bc_.b.l &= ~0x8; NEXT_INSTR; break;// Set
case 0xCB9A: de_.b.h &= ~0x8; NEXT_INSTR; break;// Set
case 0xCB9B: de_.b.l &= ~0x8; NEXT_INSTR; break;// Set
case 0xCB9C: hl_.b.h &= ~0x8; NEXT_INSTR; break;// Set
case 0xCB9D: hl_.b.l &= ~0x8; NEXT_INSTR; break;// Set
case 0xCB9E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break; // Res 3,(HL)
case 0xCB9F: af_.b.h &= ~0x8; NEXT_INSTR; break;// Set
case 0xCBA0: bc_.b.h &= ~0x10; NEXT_INSTR; break;// Set
case 0xCBA1: bc_.b.l &= ~0x10; NEXT_INSTR; break;// Set
case 0xCBA2: de_.b.h &= ~0x10; NEXT_INSTR; break;// Set
case 0xCBA3: de_.b.l &= ~0x10; NEXT_INSTR; break;// Set
case 0xCBA4: hl_.b.h &= ~0x10; NEXT_INSTR; break;// Set
case 0xCBA5: hl_.b.l &= ~0x10; NEXT_INSTR; break;// Set
case 0xCBA6: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break; // Res 4,(HL)
case 0xCBA7: af_.b.h &= ~0x10; NEXT_INSTR; break;// Set
case 0xCBA8: bc_.b.h &= ~0x20; NEXT_INSTR; break;// Set
case 0xCBA9: bc_.b.l &= ~0x20; NEXT_INSTR; break;// Set
case 0xCBAA: de_.b.h &= ~0x20; NEXT_INSTR; break;// Set
case 0xCBAB: de_.b.l &= ~0x20; NEXT_INSTR; break;// Set
case 0xCBAC: hl_.b.h &= ~0x20; NEXT_INSTR; break;// Set
case 0xCBAD: hl_.b.l &= ~0x20; NEXT_INSTR; break;// Set
case 0xCBAE: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break; // Res 5,(HL)
case 0xCBAF: af_.b.h &= ~0x20; NEXT_INSTR; break;// Set
case 0xCBB0: bc_.b.h &= ~0x40; NEXT_INSTR; break;// Set
case 0xCBB1: bc_.b.l &= ~0x40; NEXT_INSTR; break;// Set
case 0xCBB2: de_.b.h &= ~0x40; NEXT_INSTR; break;// Set
case 0xCBB3: de_.b.l &= ~0x40; NEXT_INSTR; break;// Set
case 0xCBB4: hl_.b.h &= ~0x40; NEXT_INSTR; break;// Set
case 0xCBB5: hl_.b.l &= ~0x40; NEXT_INSTR; break;// Set
case 0xCBB6: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break; // Res 6,(HL)
case 0xCBB7: af_.b.h &= ~0x40; NEXT_INSTR; break;// Set
case 0xCBB8: bc_.b.h &= ~0x80; NEXT_INSTR; break;// Set
case 0xCBB9: bc_.b.l &= ~0x80; NEXT_INSTR; break;// Set
case 0xCBBA: de_.b.h &= ~0x80; NEXT_INSTR; break;// Set
case 0xCBBB: de_.b.l &= ~0x80; NEXT_INSTR; break;// Set
case 0xCBBC: hl_.b.h &= ~0x80; NEXT_INSTR; break;// Set
case 0xCBBD: hl_.b.l &= ~0x80; NEXT_INSTR; break;// Set
case 0xCBBE: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break; // Res 7,(HL)
case 0xCBBF: af_.b.h &= ~0x80; NEXT_INSTR; break;// Set
/// SET
case 0xCBC0: bc_.b.h |= 0x1; NEXT_INSTR; break;// Set
case 0xCBC1: bc_.b.l |= 0x1; NEXT_INSTR; break;// Set
case 0xCBC2: de_.b.h |= 0x1; NEXT_INSTR; break;// Set
case 0xCBC3: de_.b.l |= 0x1; NEXT_INSTR; break;// Set
case 0xCBC4: hl_.b.h |= 0x1; NEXT_INSTR; break;// Set
case 0xCBC5: hl_.b.l |= 0x1; NEXT_INSTR; break;// Set
case 0xCBC6: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break; // Set 0,(HL)
case 0xCBC7: af_.b.h |= 0x1; NEXT_INSTR; break;// Set
case 0xCBC8: bc_.b.h |= 0x2; NEXT_INSTR; break;// Set
case 0xCBC9: bc_.b.l |= 0x2; NEXT_INSTR; break;// Set
case 0xCBCA: de_.b.h |= 0x2; NEXT_INSTR; break;// Set
case 0xCBCB: de_.b.l |= 0x2; NEXT_INSTR; break;// Set
case 0xCBCC: hl_.b.h |= 0x2; NEXT_INSTR; break;// Set
case 0xCBCD: hl_.b.l |= 0x2; NEXT_INSTR; break;// Set
case 0xCBCE: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break; // Set 1,(HL)
case 0xCBCF: af_.b.h |= 0x2; NEXT_INSTR; break;// Set
case 0xCBD0: bc_.b.h |= 0x4; NEXT_INSTR; break;// Set
case 0xCBD1: bc_.b.l |= 0x4; NEXT_INSTR; break;// Set
case 0xCBD2: de_.b.h |= 0x4; NEXT_INSTR; break;// Set
case 0xCBD3: de_.b.l |= 0x4; NEXT_INSTR; break;// Set
case 0xCBD4: hl_.b.h |= 0x4; NEXT_INSTR; break;// Set
case 0xCBD5: hl_.b.l |= 0x4; NEXT_INSTR; break;// Set
case 0xCBD6: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break; // Set 2,(HL)
case 0xCBD7: af_.b.h |= 0x4; NEXT_INSTR; break;// Set
case 0xCBD8: bc_.b.h |= 0x8; NEXT_INSTR; break;// Set
case 0xCBD9: bc_.b.l |= 0x8; NEXT_INSTR; break;// Set
case 0xCBDA: de_.b.h |= 0x8; NEXT_INSTR; break;// Set
case 0xCBDB: de_.b.l |= 0x8; NEXT_INSTR; break;// Set
case 0xCBDC: hl_.b.h |= 0x8; NEXT_INSTR; break;// Set
case 0xCBDD: hl_.b.l |= 0x8; NEXT_INSTR; break;// Set
case 0xCBDE: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break; // Set 3,(HL)
case 0xCBDF: af_.b.h |= 0x8; NEXT_INSTR; break;// Set
case 0xCBE0: bc_.b.h |= 0x10; NEXT_INSTR; break;// Set
case 0xCBE1: bc_.b.l |= 0x10; NEXT_INSTR; break;// Set
case 0xCBE2: de_.b.h |= 0x10; NEXT_INSTR; break;// Set
case 0xCBE3: de_.b.l |= 0x10; NEXT_INSTR; break;// Set
case 0xCBE4: hl_.b.h |= 0x10; NEXT_INSTR; break;// Set
case 0xCBE5: hl_.b.l |= 0x10; NEXT_INSTR; break;// Set
case 0xCBE6: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break; // Set 4,(HL)
case 0xCBE7: af_.b.h |= 0x10; NEXT_INSTR; break;// Set
case 0xCBE8: bc_.b.h |= 0x20; NEXT_INSTR; break;// Set
case 0xCBE9: bc_.b.l |= 0x20; NEXT_INSTR; break;// Set
case 0xCBEA: de_.b.h |= 0x20; NEXT_INSTR; break;// Set
case 0xCBEB: de_.b.l |= 0x20; NEXT_INSTR; break;// Set
case 0xCBEC: hl_.b.h |= 0x20; NEXT_INSTR; break;// Set
case 0xCBED: hl_.b.l |= 0x20; NEXT_INSTR; break;// Set
case 0xCBEE: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break; // Set 5,(HL)
case 0xCBEF: af_.b.h |= 0x20; NEXT_INSTR; break;// Set
case 0xCBF0: bc_.b.h |= 0x40; NEXT_INSTR; break;// Set
case 0xCBF1: bc_.b.l |= 0x40; NEXT_INSTR; break;// Set
case 0xCBF2: de_.b.h |= 0x40; NEXT_INSTR; break;// Set
case 0xCBF3: de_.b.l |= 0x40; NEXT_INSTR; break;// Set
case 0xCBF4: hl_.b.h |= 0x40; NEXT_INSTR; break;// Set
case 0xCBF5: hl_.b.l |= 0x40; NEXT_INSTR; break;// Set
case 0xCBF6: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break; // Set 6,(HL)
case 0xCBF7: af_.b.h |= 0x40; NEXT_INSTR; break;// Set
case 0xCBF8: bc_.b.h |= 0x80; NEXT_INSTR; break;// Set
case 0xCBF9: bc_.b.l |= 0x80; NEXT_INSTR; break;// Set
case 0xCBFA: de_.b.h |= 0x80; NEXT_INSTR; break;// Set
case 0xCBFB: de_.b.l |= 0x80; NEXT_INSTR; break;// Set
case 0xCBFC: hl_.b.h |= 0x80; NEXT_INSTR; break;// Set
case 0xCBFD: hl_.b.l |= 0x80; NEXT_INSTR; break;// Set
case 0xCBFE: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break; // Set 7,(HL)
case 0xCBFF: af_.b.h |= 0x80; NEXT_INSTR; break;// Set

   // DD
case 0xDD09: ADD_FLAG_W(ix_.w, bc_.w); t_ = 4 + 3; machine_cycle_ = M_Z80_WORK; break; // ADD IX, BC
case 0xDD19: ADD_FLAG_W(ix_.w, de_.w); t_ = 4 + 3; machine_cycle_ = M_Z80_WORK; break; // ADD IX, DE
case 0xDD21: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD_IX, nn
case 0xDD22: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break;// LD (nn), IX
case 0xDD23: if (t_ == 6) { ++ix_.w; NEXT_INSTR; }
             else { ++t_; }; break;                     // INC_IX
case 0xDD24: INC_FLAGS(ix_.b.h); NEXT_INSTR; break;                                                           // INC IXh
case 0xDD25: DEC_FLAGS(ix_.b.h); NEXT_INSTR; break;                                                           // DEC IXh
case 0xDD26: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break;   // LD IXh, n
case 0xDD29: ADD_FLAG_W(ix_.w, ix_.w); t_ = 4 + 3; machine_cycle_ = M_Z80_WORK; break;                             // ADD IX, IX
case 0xDD2A: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break;   // LD IX, (nn)
case 0xDD2B: if (t_ == 6) { --ix_.w; NEXT_INSTR; }
             else { ++t_; }; break;                                                 // DEC_IX
case 0xDD2C: INC_FLAGS(ix_.b.l); NEXT_INSTR; break;                                                           // INC IXl
case 0xDD2D: DEC_FLAGS(ix_.b.l); NEXT_INSTR; break;                                                           // DEC IXl
case 0xDD2E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break;   // LD IXl, n
case 0xDD34: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // INC (IX+d)
case 0xDD35: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // DEC (IX+d)
case 0xDD36: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD (IX+d), n
case 0xDD39: ADD_FLAG_W(ix_.w, sp_); t_ = 4 + 3; machine_cycle_ = M_Z80_WORK; break;                               // ADD IX, SP
case 0xDD44: bc_.b.h = ix_.b.h; NEXT_INSTR; break;                                                                         // LD B, IXh
case 0xDD45: bc_.b.h = ix_.b.l; NEXT_INSTR; break;                                                                         // LD B, IXl
case 0xDD46: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD B, (IX+n)
case 0xDD4C: bc_.b.l = ix_.b.h; NEXT_INSTR; break;                                                                         // LD C, IXh
case 0xDD4D: bc_.b.l = ix_.b.l; NEXT_INSTR; break;                                                                         // LD C, IXl
case 0xDD4E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD C, (IX+n)
case 0xDD54: de_.b.h = ix_.b.h; NEXT_INSTR; break;                                                                         // LD D, IXh
case 0xDD55: de_.b.h = ix_.b.l; NEXT_INSTR; break;                                                                         // LD D, IXl
case 0xDD56: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD D, (IX+n)
case 0xDD5C: de_.b.l = ix_.b.h; NEXT_INSTR; break;                                                                         // LD E, IXh
case 0xDD5D: de_.b.l = ix_.b.l; NEXT_INSTR; break;                                                                         // LD E, IXl
case 0xDD5E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD E, (IX+n)
case 0xDD60: ix_.b.h = bc_.b.h; NEXT_INSTR; break;                                                                        // LD IXh, B
case 0xDD61: ix_.b.h = bc_.b.l; NEXT_INSTR; break;                                                                        // LD IXh, C
case 0xDD62: ix_.b.h = de_.b.h; NEXT_INSTR; break;                                                                        // LD IXh, D
case 0xDD63: ix_.b.h = de_.b.l; NEXT_INSTR; break;                                                                      // LD IXh, E
case 0xDD64: /*ix_.b.h = ix_.b.h*/; NEXT_INSTR; break;                                                                 // LD IXh, IXh
case 0xDD65: ix_.b.h = ix_.b.l; NEXT_INSTR; break;                                                                 // LD IXh, IXl
case 0xDD66: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD H, (IX+n)
case 0xDD67: ix_.b.h = af_.b.h; NEXT_INSTR; break;                                                                      // LD IXh, A
case 0xDD68: ix_.b.l = bc_.b.h; NEXT_INSTR; break;                                                                      // LD IXl, B
case 0xDD69: ix_.b.l = bc_.b.l; NEXT_INSTR; break;                                                                      // LD IXl, C
case 0xDD6A: ix_.b.l = de_.b.h; NEXT_INSTR; break;                                                                      // LD IXl, D
case 0xDD6B: ix_.b.l = de_.b.l; NEXT_INSTR; break;                                                                      // LD IXl, E
case 0xDD6C: ix_.b.l = ix_.b.h; NEXT_INSTR; break;                                                                 // LD IXl, IXh
case 0xDD6D: /*ix_.b.l = ix_.b.l ;*/NEXT_INSTR; break;                                                                 // LD IXl, IXl
case 0xDD6E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD L, (IX+n)
case 0xDD6F: ix_.b.l = af_.b.h; NEXT_INSTR; break;                                                                      // LD IXl, A
case 0xDD70: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD (IX+d), B
case 0xDD71: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD (IX+d), C
case 0xDD72: TraceTape(pc_, de_.b.h); machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD (IX+d), D
case 0xDD73: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD (IX+d), E
case 0xDD74: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD (IX+d), H
case 0xDD75:
   TraceTape(pc_, hl_.b.l);

   machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD (IX+d), L

case 0xDD77:
   TraceTape(pc_, af_.b.h);
   machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD (IX+d), A
case 0xDD7C: af_.b.h = ix_.b.h; NEXT_INSTR; break;                                                                       // LD A,IXh
case 0xDD7D: af_.b.h = ix_.b.l; NEXT_INSTR; break;                                                                       // LD A,IXl
case 0xDD7E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD A, (IX+n)
case 0xDD84: ADD_FLAG(ix_.b.h); NEXT_INSTR; break;                                                                   // ADD A,IXh
case 0xDD85: ADD_FLAG(ix_.b.l); NEXT_INSTR; break;                                                                   // ADD A,IXl
case 0xDD86: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; //ADD A, (IX+d)
case 0xDD8C: ADD_FLAG_CARRY(ix_.b.h); NEXT_INSTR; break;                                                             // ADC A,IXh
case 0xDD8D: ADD_FLAG_CARRY(ix_.b.l); NEXT_INSTR; break;                                                             // ADC A,IXl
case 0xDD8E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; //ADC A, (IX+d)
case 0xDD94: SUB_FLAG(ix_.b.h); NEXT_INSTR; break;                                                                   // SUB A,IXh
case 0xDD95: SUB_FLAG(ix_.b.l); NEXT_INSTR; break;                                                                   // SUB A,IXl
case 0xDD96: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // SUB A, (IX+d)
case 0xDD9C: SUB_FLAG_CARRY(ix_.b.h); NEXT_INSTR; break;                                                             // SBC A,IXh
case 0xDD9D: SUB_FLAG_CARRY(ix_.b.l); NEXT_INSTR; break;                                                             // SBC A,IXl
case 0xDD9E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // SBC A, (IX+d)
case 0xDDA4: AND_FLAGS(ix_.b.h); NEXT_INSTR; break;                                                                     // AND IXh
case 0xDDA5: AND_FLAGS(ix_.b.l); NEXT_INSTR; break;                                                                     // AND IXl
case 0xDDA6: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break;  // AND (IX+d)
case 0xDDAC: XOR_FLAGS(ix_.b.h); NEXT_INSTR; break;                                                                     // XOR IXh
case 0xDDAD: XOR_FLAGS(ix_.b.l); NEXT_INSTR; break;                                                                     // XOR IXl
case 0xDDAE: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // XOR (IX+d)
case 0xDDB4: OR_FLAGS(ix_.b.h); NEXT_INSTR; break;                                                                     // OR IXh
case 0xDDB5: OR_FLAGS(ix_.b.l); NEXT_INSTR; break;                                                                     // OR IXl
case 0xDDB6: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // OR (IX+n)
case 0xDDBC: CP_FLAGS(ix_.b.h); NEXT_INSTR; break;                                                                     // CP IXh
case 0xDDBD: CP_FLAGS(ix_.b.l); NEXT_INSTR; break;                                                                     // CP IXl
case 0xDDBE: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // CP(IX+d)
case 0xDDCB: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break;// DD CB - Lecture de d
case 0xDDE1: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_++; current_data_ = 0; read_count_ = 0; break;// POP IX
case 0xDDE3: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_; current_data_ = 0; read_count_ = 0; break;    // EX (SP), IX
case 0xDDE5: if (t_ == 5) { machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = --sp_; current_data_ = ix_.w >> 8; read_count_ = 0; }
             else { ++t_; } break;// PUSH IX
case 0xDDE9: pc_ = ix_.w; NEXT_INSTR; break;                                                                           // JP (IX)
case 0xDDF9: if (t_ == 6) { sp_ = ix_.w; NEXT_INSTR; }
             else { ++t_; }break;// LD SP, IX
//////////////////////////////////////////////////////////
// ED
case 0xED40: machine_cycle_ = M_IO_R; t_ = 1; current_address_ = bc_.w; break; // IN B, (C)
case 0xED41: machine_cycle_ = M_IO_W; t_ = 1; current_address_ = bc_.w; current_data_ = bc_.b.h; q_ = af_.b.l | ((bc_.b.h & 0x80) ? NF : 0); af_.b.l = q_; break; // OUT (C), B
case 0xED42: mem_ptr_.w = hl_.w + 1; SUB_FLAG_CARRY_W(hl_.w, bc_.w); machine_cycle_ = M_Z80_WORK; t_ = 4 + 3; break;// SBC HL,BC
case 0xED43: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break;   // LD (nn), BC
case 0xED44: NEG; NEXT_INSTR; break;// NEG
case 0xED45: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_++; current_data_ = 0; read_count_ = 0; break;   // RETN
case 0xED46: interrupt_mode_ = 0; NEXT_INSTR; break;// IM 0
case 0xED47: if (t_ == 5) { ir_.b.h = af_.b.h; NEXT_INSTR; }
             else { ++t_; } break;    // LD I, A
case 0xED48: machine_cycle_ = M_IO_R; t_ = 1; current_address_ = bc_.w; break; // IN C, (C)
case 0xED49: machine_cycle_ = M_IO_W; t_ = 1; current_address_ = bc_.w; current_data_ = bc_.b.l; q_ = af_.b.l | ((bc_.b.l & 0x80) ? NF : 0); af_.b.l = q_; break; // OUT (C), C
case 0xED4A: mem_ptr_.w = hl_.w + 1; ADD_FLAG_CARRY_W(hl_.w, bc_.w); machine_cycle_ = M_Z80_WORK; t_ = 4 + 3; break; // ADC HL,BC
case 0xED4B: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break;// LD BC, (nn)
case 0xED4C: NEG; NEXT_INSTR; break;// NEG
case 0xED4D: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_++; current_data_ = 0; read_count_ = 0; break;   // RETI
case 0xED4E: interrupt_mode_ = 0; NEXT_INSTR; break;// IM 0
case 0xED4F: if (t_ == 5) { ir_.b.l = af_.b.h; NEXT_INSTR; }
             else { ++t_; } break;    // LD R, A
case 0xED50: machine_cycle_ = M_IO_R; t_ = 1; current_address_ = bc_.w; break; // IN D, (C)
case 0xED51: machine_cycle_ = M_IO_W; t_ = 1; current_address_ = bc_.w; current_data_ = de_.b.h; q_ = af_.b.l | ((de_.b.h & 0x80) ? NF : 0); af_.b.l = q_; break; // OUT (C), D
case 0xED52: mem_ptr_.w = hl_.w + 1; SUB_FLAG_CARRY_W(hl_.w, de_.w); machine_cycle_ = M_Z80_WORK; t_ = 4 + 3; break;// SBC HL,DE
case 0xED53:machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break;   // LD (nn), DE
case 0xED54: NEG; NEXT_INSTR; break;// NEG
case 0xED55: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_++; current_data_ = 0; read_count_ = 0; break;   // RETN
case 0xED56: interrupt_mode_ = 1; NEXT_INSTR; break;// IM 1
case 0xED57: if (t_ == 5) {
   af_.b.h = ir_.b.h; q_ = af_.b.l; q_ &= ~(HF | NF | 0x28); ZERO_FLAG(af_.b.h); SIGN_FLAG(af_.b.h); if (iff2_)
      q_ |= PF;
   else
      q_ &= ~(PF);
   q_ |= (af_.b.h & 0x28);
   af_.b.l = q_;
   carry_set_ = true;
   NEXT_INSTR_LDAIR
}
             else { ++t_; }; break;// LD A, I
case 0xED58: machine_cycle_ = M_IO_R; t_ = 1; current_address_ = bc_.w; break; // IN E, (C)
case 0xED59: machine_cycle_ = M_IO_W; t_ = 1; current_address_ = bc_.w; current_data_ = de_.b.l; q_ = af_.b.l | ((de_.b.l & 0x80) ? NF : 0); af_.b.l = q_; break; // OUT (C), E
case 0xED5A: mem_ptr_.w = hl_.w + 1; ADD_FLAG_CARRY_W(hl_.w, de_.w); machine_cycle_ = M_Z80_WORK; t_ = 4 + 3; break; // ADC HL,DE
case 0xED5B: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD DE, (nn)
case 0xED5C: NEG; NEXT_INSTR; break;// NEG
case 0xED5D: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_++; current_data_ = 0; read_count_ = 0; break;   // RETN
case 0xED5E: interrupt_mode_ = 2; NEXT_INSTR; break;// IM 2
case 0xED5F: if (t_ == 5)
{
   af_.b.h = ir_.b.l; q_ = af_.b.l; q_ &= ~(HF | NF | 0x28); ZERO_FLAG(af_.b.h); SIGN_FLAG(af_.b.h);
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
case 0xED61: machine_cycle_ = M_IO_W; t_ = 1; current_address_ = bc_.w; current_data_ = hl_.b.h; q_ = af_.b.l | ((hl_.b.h & 0x80) ? NF : 0); af_.b.l = q_; break; // OUT (C), H
case 0xED62: mem_ptr_.w = hl_.w + 1; SUB_FLAG_CARRY_W(hl_.w, hl_.w); machine_cycle_ = M_Z80_WORK; t_ = 4 + 3; break;// SBC HL,HL
case 0xED63:machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break;   // LD (nn), HL
case 0xED64: NEG; NEXT_INSTR; break;// NEG
case 0xED65: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_++; current_data_ = 0; read_count_ = 0; break;   // RETN
case 0xED66: interrupt_mode_ = 0; NEXT_INSTR; break;// IM 0
case 0xED67: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break;    //RRD
case 0xED68: machine_cycle_ = M_IO_R; t_ = 1; current_address_ = bc_.w; break; // IN L, (C)
case 0xED69: machine_cycle_ = M_IO_W; t_ = 1; current_address_ = bc_.w; current_data_ = hl_.b.l; q_ = af_.b.l | ((hl_.b.l & 0x80) ? NF : 0); af_.b.l = q_; break; // OUT (C), L
case 0xED6A: mem_ptr_.w = hl_.w + 1; ADD_FLAG_CARRY_W(hl_.w, hl_.w); machine_cycle_ = M_Z80_WORK; t_ = 4 + 3; break; // ADC HL,HL
case 0xED6B: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break;// LD HL, (nn)
case 0xED6C: NEG; NEXT_INSTR; break;// NEG
case 0xED6D: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_++; current_data_ = 0; read_count_ = 0; break;   // RETN
case 0xED6E: interrupt_mode_ = 0; NEXT_INSTR; break;// IM 0
case 0xED6F: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break; // RLD
case 0xED70: machine_cycle_ = M_IO_R; t_ = 1; current_address_ = bc_.w; break; // IN (C)
case 0xED71: machine_cycle_ = M_IO_W; t_ = 1; current_address_ = bc_.w; current_data_ = 0; q_ = af_.b.l; q_ |= (0 & 0x80) ? NF : 0; af_.b.l = q_; break; // OUT (C), 0
case 0xED72: mem_ptr_.w = hl_.w + 1; SUB_FLAG_CARRY_W(hl_.w, sp_); machine_cycle_ = M_Z80_WORK; t_ = 4 + 3; break;// SBC HL,SP
case 0xED73: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break;   // LD (nn), SP
case 0xED74: NEG; NEXT_INSTR; break;// NEG
case 0xED75: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_++; current_data_ = 0; read_count_ = 0; break;   // RETN
case 0xED76: interrupt_mode_ = 1; NEXT_INSTR; break;// IM 1
case 0xED77: NEXT_INSTR; break;                                                                                        // NOP
case 0xED78: machine_cycle_ = M_IO_R; t_ = 1; current_address_ = bc_.w; break; // IN A, (C)
case 0xED79: machine_cycle_ = M_IO_W; t_ = 1; current_address_ = bc_.w; current_data_ = af_.b.h; q_ = af_.b.l | ((af_.b.h & 0x80) ? NF : 0); mem_ptr_.w = bc_.w + 1; af_.b.l = q_; break; // OUT (C), A
case 0xED7A: mem_ptr_.w = hl_.w + 1; ADD_FLAG_CARRY_W(hl_.w, sp_); machine_cycle_ = M_Z80_WORK; t_ = 4 + 3; break; // ADC HL,SP
case 0xED7B: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD SP, (nn)
case 0xED7C: NEG; NEXT_INSTR; break;// NEG
case 0xED7D: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_++; current_data_ = 0; read_count_ = 0; break;   // RETN
case 0xED7E: interrupt_mode_ = 2; NEXT_INSTR; break;// IM 2
case 0xED7F: NEXT_INSTR; break;                                                                                        // NOP
case 0xEDA0: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break;   // LDI
case 0xEDA1: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break; // CPI
case 0xEDA2: if (t_ == 5) { machine_cycle_ = M_IO_R; t_ = 1; current_address_ = bc_.w; }
             else { ++t_; }; break;       // INI
case 0xEDA3: if (t_ == 5) { machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; }
             else { ++t_; } break; // OUTI
case 0xEDA8: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break; // LDD
case 0xEDA9: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break; // CPD
case 0xEDAA: if (t_ == 5) { machine_cycle_ = M_IO_R; t_ = 1; current_address_ = bc_.w; }
             else { ++t_; }; break;       // IND
case 0xEDAB: if (t_ == 5) { machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; }
             else { ++t_; } break;// OUTD
case 0xEDB0: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break;   // LDIR
case 0xEDB1: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break; // CPIR
case 0xEDB2: if (t_ == 5) { machine_cycle_ = M_IO_R; t_ = 1; current_address_ = bc_.w; }
             else { ++t_; }; break;       // INIR
case 0xEDB3: if (t_ == 5) { machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; }
             else { ++t_; };  break; // OTIR
case 0xEDB8: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break;   // LDDR
case 0xEDB9: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; break; // CPDR
case 0xEDBA: if (t_ == 5) { machine_cycle_ = M_IO_R; t_ = 1; current_address_ = bc_.w; }
             else { ++t_; }; break;       // INDR
case 0xEDBB: if (t_ == 5) { machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = hl_.w; current_data_ = 0; read_count_ = 0; }
             else { ++t_; };  break; // OTDR
case 0xEDED: NEXT_INSTR; break;                                                                  // ED Extension opcode
   //////////////////////////////////////////////////////////
   // FD
case 0xFD09: ADD_FLAG_W(iy_.w, bc_.w); t_ = 4 + 3; machine_cycle_ = M_Z80_WORK; break; // ADD IY, BC
case 0xFD19: ADD_FLAG_W(iy_.w, de_.w); t_ = 4 + 3; machine_cycle_ = M_Z80_WORK; break; // ADD IY, DE
case 0xFD21: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break;// LD IY, nn
case 0xFD22: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break;// LD (nn), IY
case 0xFD23: if (t_ == 6) { ++iy_.w; NEXT_INSTR; }
             else { ++t_; }; break;                     // INC_IY
case 0xFD24: INC_FLAGS(iy_.b.h); NEXT_INSTR; break;                                                           // INC IYh
case 0xFD25: DEC_FLAGS(iy_.b.h); NEXT_INSTR; break;                                                           // DEC IYh
case 0xFD26: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break;   // LD IYh, n
case 0xFD29: ADD_FLAG_W(iy_.w, iy_.w); t_ = 4 + 3; machine_cycle_ = M_Z80_WORK; break;                             // ADD IY, IY
case 0xFD2A: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break;   // LD IY, (nn)
case 0xFD2B: if (t_ == 6) { --iy_.w; NEXT_INSTR; }
             else { ++t_; }; break;                                                 // DEC_IY
case 0xFD2C: INC_FLAGS(iy_.b.l); NEXT_INSTR; break;                                                           // INC IYl
case 0xFD2D: DEC_FLAGS(iy_.b.l); NEXT_INSTR; break;                                                           // DEC IYl
case 0xFD2E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break;   // LD IYl, n
case 0xFD34: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // INC (IY+d)
case 0xFD35: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // DEC (IY+d)
case 0xFD36: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD (IY+d), n
case 0xFD39: ADD_FLAG_W(iy_.w, sp_); t_ = 4 + 3; machine_cycle_ = M_Z80_WORK; break;                               // ADD IY, SP
case 0xFD44: bc_.b.h = iy_.b.h; NEXT_INSTR; break;                                                                         // LD B, IYh
case 0xFD45: bc_.b.h = iy_.b.l; NEXT_INSTR; break;                                                                         // LD B, IYl
case 0xFD46: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD B, (IY+n)
case 0xFD4C: bc_.b.l = iy_.b.h; NEXT_INSTR; break;                                                                         // LD C, IYh
case 0xFD4D: bc_.b.l = iy_.b.l; NEXT_INSTR; break;                                                                         // LD C, IYl
case 0xFD4E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD C, (IY+n)
case 0xFD54: de_.b.h = iy_.b.h; NEXT_INSTR; break;                                                                       // LD D, IYh
case 0xFD55: de_.b.h = iy_.b.l; NEXT_INSTR; break;                                                                       // LD D, IYl
case 0xFD56: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD D, (IY+n)
case 0xFD5C: de_.b.l = iy_.b.h; NEXT_INSTR; break;                                                                       // LD E, IYh
case 0xFD5D: de_.b.l = iy_.b.l; NEXT_INSTR; break;                                                                       // LD E, IYl
case 0xFD5E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD E, (IY+n)
case 0xFD60: iy_.b.h = bc_.b.h; NEXT_INSTR; break;                                                                      // LD IYh, B
case 0xFD61: iy_.b.h = bc_.b.l; NEXT_INSTR; break;                                                                      // LD IYh, C
case 0xFD62: iy_.b.h = de_.b.h; NEXT_INSTR; break;                                                                      // LD IYh, D
case 0xFD63: iy_.b.h = de_.b.l; NEXT_INSTR; break;                                                                      // LD IYh, E
case 0xFD64: /*iy_.b.h = iy_.b.h */; NEXT_INSTR; break;                                                                 // LD IYh, IYh
case 0xFD65: iy_.b.h = iy_.b.l; NEXT_INSTR; break;                                                                 // LD IYh, IYl
case 0xFD66: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD H, (IY+n)
case 0xFD67: iy_.b.h = af_.b.h; NEXT_INSTR; break;                                                                      // LD IYh, A
case 0xFD68: iy_.b.l = bc_.b.h; NEXT_INSTR; break;                                                                      // LD IYl, B
case 0xFD69: iy_.b.l = bc_.b.l; NEXT_INSTR; break;                                                                      // LD IYl, C
case 0xFD6A: iy_.b.l = de_.b.h; NEXT_INSTR; break;                                                                      // LD IYl, D
case 0xFD6B: iy_.b.l = de_.b.l; NEXT_INSTR; break;                                                                      // LD IYl, E
case 0xFD6C: iy_.b.l = iy_.b.h; NEXT_INSTR; break;                                                                 // LD IYl, IYh
case 0xFD6D: /*iy_.b.l = iy_.b.l */; NEXT_INSTR; break;                                                                 // LD IYl, IYl
case 0xFD6E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD L, (IY+n)
case 0xFD6F: iy_.b.l = af_.b.h; NEXT_INSTR; break;                                                                      // LD IYl, A
case 0xFD70: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD (IY+d), B
case 0xFD71: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD (IY+d), C
case 0xFD72: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD (IY+d), D
case 0xFD73: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD (IY+d), E
case 0xFD74: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD (IY+d), H
case 0xFD75: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD (IY+d), L
case 0xFD77: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD (IY+d), A
case 0xFD7C: af_.b.h = iy_.b.h; NEXT_INSTR; break;                                                                       // LD A,IYh
case 0xFD7D: af_.b.h = iy_.b.l; NEXT_INSTR; break;                                                                       // LD A,IYl
case 0xFD7E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // LD A, (IY+n)
case 0xFD84: ADD_FLAG(iy_.b.h); NEXT_INSTR; break;                                                                   // ADD A,IYh
case 0xFD85: ADD_FLAG(iy_.b.l); NEXT_INSTR; break;                                                                   // ADD A,IYl
case 0xFD86: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; //ADD A, (IY+d)
case 0xFD8C: ADD_FLAG_CARRY(iy_.b.h); NEXT_INSTR; break;                                                             // ADC A,IYh
case 0xFD8D: ADD_FLAG_CARRY(iy_.b.l); NEXT_INSTR; break;                                                             // ADC A,IYl
case 0xFD8E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // ADC A, (IY+d)
case 0xFD94: SUB_FLAG(iy_.b.h); NEXT_INSTR; break;                                                                   // SUB A,IYh
case 0xFD95: SUB_FLAG(iy_.b.l); NEXT_INSTR; break;                                                                   // SUB A,IYl
case 0xFD96: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // SUB A, (IY+d)
case 0xFD9C: SUB_FLAG_CARRY(iy_.b.h); NEXT_INSTR; break;                                                             // SBC A,IYh
case 0xFD9D: SUB_FLAG_CARRY(iy_.b.l); NEXT_INSTR; break;                                                             // SBC A,IYl
case 0xFD9E: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // SBC A, (IY+d)
case 0xFDA4: AND_FLAGS(iy_.b.h); NEXT_INSTR; break;                                                                     // AND IYh
case 0xFDA5: AND_FLAGS(iy_.b.l); NEXT_INSTR; break;                                                                     // AND IYl
case 0xFDA6: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break;  // AND (IY+d)
case 0xFDAC: XOR_FLAGS(iy_.b.h); NEXT_INSTR; break;                                                                     // XOR IYh
case 0xFDAD: XOR_FLAGS(iy_.b.l); NEXT_INSTR; break;                                                                     // XOR IYl
case 0xFDAE: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // XOR (IY+d)
case 0xFDB4: OR_FLAGS(iy_.b.h); NEXT_INSTR; break;                                                                     // OR IYh
case 0xFDB5: OR_FLAGS(iy_.b.l); NEXT_INSTR; break;                                                                     // OR IYl
case 0xFDB6: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // OR (IY+n)
case 0xFDBC: CP_FLAGS(iy_.b.h); NEXT_INSTR; break;                                                                     // CP IYh
case 0xFDBD: CP_FLAGS(iy_.b.l); NEXT_INSTR; break;                                                                     // CP IYl
case 0xFDBE: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break; // CP(IY+d)
case 0xFDCB: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = pc_; current_data_ = 0; read_count_ = 0; break;// FD CB - Lecture de d
case 0xFDE1: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_++; current_data_ = 0; read_count_ = 0; break;// POP IY
case 0xFDE3: machine_cycle_ = M_MEMORY_R; t_ = 1; current_address_ = sp_; current_data_ = 0; read_count_ = 0; break;    // EX (SP), IY
case 0xFDE5: if (t_ == 5) { machine_cycle_ = M_MEMORY_W; t_ = 1; current_address_ = --sp_; current_data_ = iy_.w >> 8; read_count_ = 0; }
             else { ++t_; } break;// PUSH IY
case 0xFDE9: pc_ = iy_.w; NEXT_INSTR; break;                                                                           // JP (IY)
case 0xFDF9: if (t_ == 6) { sp_ = iy_.w; NEXT_INSTR; }
             else { ++t_; }break;// LD SP, IY
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
