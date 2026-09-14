#include "stdafx.h"
#include "MachineState.h"
#include "Machine.h"
#include "Motherboard.h"
#include "DiskGen.h"
#include "IDisk.h"
#include "Tape.h"
#include "PPI.h"
#include "DMA.h"
#include "PlayCity.h"
#include "Memoire.h"

#include <cstring>

namespace
{

const unsigned char kMagic[4] = { 'S', 'B', 'X', 'S' };
const size_t kHeaderSize = 12;

// Scheduler: the cycle debt each component carries across a time slice, which
// is what a .SNA has no room for and what makes a restored machine drift.
const unsigned int kChunkScheduler = 0x53434844;  // "SCHD"

// The CPU's execution engine: where it is inside the current instruction, and
// the latches it carries between machine cycles. A .SNA only records the
// registers an instruction boundary can see.
const unsigned int kChunkZ80 = 0x5A383058;  // "Z80X"

// The gate array's own counters. The .SNA has fields for the interrupt counter
// and the vsync delay, but none for the hsync counter that drives them, so a
// restored machine raises its next interrupt at a different moment.
const unsigned int kChunkGateArray = 0x47415258;  // "GARX"

// The sound chip's derived state. A .SNA carries the sixteen registers and the
// load side replays them through the bus, which rebuilds most of this -- but
// replaying r13 restarts the envelope and replaying r14 clears the flag that
// selects the keyboard rather than the latch, so the machine reads different
// keys after a restore and takes a different path.
const unsigned int kChunkPsg = 0x50534758;  // "PSGX"

// The disk controller: the command in flight with its parameters and results,
// the head position, and the MFM decoder's place in the track. A .SNA has room
// for the motor flag and the current track and nothing else, which describes a
// stopped drive and no more.
//
// scan_func_, disk_to_load_ and delayed_load_filepath_ are deliberately left
// out. The first is a function pointer and the others belong to the deferred
// disk-load request rather than to the controller, and writing a host address
// into a state that another process will read back is a bug, not an omission.
const unsigned int kChunkFdc = 0x46444358;  // "FDCX"

// The drives: where each head is, not what is under it.
//
// A disk image is media, not machine state. It can be hundreds of kilobytes,
// it is the same before and after, and the user may legitimately have swapped
// it. So this carries the position -- track, side, rotational angle, and the
// MFM decoder's place in the bit stream -- and refuses to load if the geometry
// of the disk now in the drive does not match the one the state was taken with.
// Restoring a head position into a different disk would seek into nothing.
const unsigned int kChunkDrives = 0x44525653;  // "DRVS"

// The CRTC. The .SNA carries eighteen of the thirty-two registers and a handful
// of the counters; the rest keeps whatever the machine happened to boot with,
// which is invisible while both machines booted the same way and shows up the
// moment a state crosses into a process that did not.
const unsigned int kChunkCrtc = 0x43525443;  // "CRTC"

// The tape. The .SNA has nothing for it at all -- not even the equivalent of
// the disk's motor flag and current track -- so everything here is new.
//
// Same split as the drives: the flux array is media and stays out, the position
// within it is state, and the size of the array is recorded so that a state can
// be refused rather than applied to a different tape.
const unsigned int kChunkTape = 0x54415045;  // "TAPE"

// The PPI's tape input level. The .SNA carries the three ports and the control
// word, but not the signal the tape is presenting to port B, which is the bit
// the firmware's loading loop samples. Leave it behind and a restored machine
// reads the wrong bit at the wrong moment, and the loop takes a different path
// while every other component stays in step -- as confusing to diagnose as it
// sounds, and only visible when the state is loaded into a machine whose own
// tape level happened to differ.
const unsigned int kChunkPpi = 0x50504958;  // "PPIX"

// The Plus ASIC's three sound DMA channels. The .SNA's CPC+ chunk carries three
// of the nine fields each channel has -- the repeat counter, the repeat address
// and the pause counter -- and not the state machine's current phase, the
// instruction being executed, or the prescaler. A machine restored mid DMA
// program therefore resumes somewhere else in it, which is most Plus software
// with music.
const unsigned int kChunkDma = 0x444D4158;  // "DMAX"

// PlayCity: two YMZ294 sound chips and a Z84C30 CTC, none of which the .SNA has
// ever heard of. It is a user-selectable option in the core, so a state taken
// with it enabled has to carry it or savestates break for everyone who turns it
// on.
const unsigned int kChunkPlayCity = 0x50434954;  // "PCIT"

// Expansion RAM beyond the first 64 KB page.
//
// The .SNA writer records the dump size as 64 or 128 and emits at most the base
// bank plus one expansion page, so a 576 KB machine -- SymbOS, OrgaMS, X-MEM,
// the FutureOS 512 KB configurations -- silently loses 448 KB. Carrying the rest
// here rather than widening the .SNA keeps the interchange format exactly as it
// is: other emulators read what they always read, and none of the existing
// snapshot tests change behaviour.
//
// Page 0 is deliberately absent: the .SNA already carries it, and writing it
// twice would be a second source of truth.
const unsigned int kChunkExtendedRam = 0x5852414D;  // "XRAM"

// Identity of the inserted cartridge, and nothing else.
//
// A cartridge is media, like a disc or a tape: its half-megabyte of ROM stays
// out of the state. What goes in is enough to refuse a state taken with a
// different cartridge -- the CRC32 Memory computed once when the .cpr was
// loaded, and how many banks are plugged. Nothing here is restored; the chunk
// exists to be checked.
const unsigned int kChunkCartridge = 0x43415254;  // "CART"


void PutU16(std::vector<unsigned char>& out, unsigned short v)
{
   out.push_back(v & 0xFF);
   out.push_back((v >> 8) & 0xFF);
}

void PutU32(std::vector<unsigned char>& out, unsigned int v)
{
   out.push_back(v & 0xFF);
   out.push_back((v >> 8) & 0xFF);
   out.push_back((v >> 16) & 0xFF);
   out.push_back((v >> 24) & 0xFF);
}

unsigned short GetU16(const unsigned char* p) { return p[0] | (p[1] << 8); }

unsigned int GetU32(const unsigned char* p)
{
   return p[0] | (p[1] << 8) | (p[2] << 16) | ((unsigned int)p[3] << 24);
}
void PutF64(std::vector<unsigned char>& out, double v)
{
   unsigned long long bits;
   memcpy(&bits, &v, 8);
   PutU32(out, (unsigned int)(bits & 0xFFFFFFFFu));
   PutU32(out, (unsigned int)(bits >> 32));
}

double GetF64(const unsigned char* p)
{
   const unsigned long long bits =
      (unsigned long long)GetU32(p) | ((unsigned long long)GetU32(p + 4) << 32);
   double v;
   memcpy(&v, &bits, 8);
   return v;
}


}  // namespace

void MachineState::WriteScheduler(Motherboard* board, std::vector<unsigned char>& out)
{
   const size_t length_at = out.size() + 4;
   PutU32(out, kChunkScheduler);
   PutU32(out, 0);                       // patched once the payload is known

   const size_t payload_at = out.size();

   PutU32(out, board->counter_);
   PutU32(out, (unsigned int)board->nb_components_);

   const int nb_elapsed = (int)(sizeof(board->component_elapsed_time_)
                              / sizeof(board->component_elapsed_time_[0]));
   PutU32(out, (unsigned int)nb_elapsed);
   for (int i = 0; i < nb_elapsed; ++i)
      PutU32(out, board->component_elapsed_time_[i]);

   // Each component keeps its own copy of where it is within the slice.
   for (int i = 0; i < board->nb_components_; ++i)
   {
      IComponent* c = board->component_list_[i];
      PutU32(out, c ? (unsigned int)c->elapsed_time_ : 0);
      PutU32(out, c ? (unsigned int)c->this_tick_time_ : 0);
   }

   const unsigned int payload_size = (unsigned int)(out.size() - payload_at);
   out[length_at + 0] = payload_size & 0xFF;
   out[length_at + 1] = (payload_size >> 8) & 0xFF;
   out[length_at + 2] = (payload_size >> 16) & 0xFF;
   out[length_at + 3] = (payload_size >> 24) & 0xFF;
}

bool MachineState::ReadScheduler(Motherboard* board, const unsigned char* p, size_t size)
{
   if (size < 12) return false;
   size_t at = 0;

   board->counter_ = GetU32(&p[at]); at += 4;

   const int nb_components = (int)GetU32(&p[at]); at += 4;
   const int nb_elapsed = (int)GetU32(&p[at]); at += 4;

   const int capacity_elapsed = (int)(sizeof(board->component_elapsed_time_)
                                    / sizeof(board->component_elapsed_time_[0]));
   const int capacity_components = (int)(sizeof(board->component_list_)
                                       / sizeof(board->component_list_[0]));
   if (nb_elapsed != capacity_elapsed) return false;
   if (nb_components < 0 || nb_components > capacity_components) return false;
   if (size < at + (size_t)nb_elapsed * 4 + (size_t)nb_components * 8) return false;

   // nb_components_ itself is set by InitStartOptimized() from the machine's
   // configuration, not by the state: a state must not be able to describe a
   // component list that does not match the machine it is loaded into.
   if (nb_components != board->nb_components_) return false;

   for (int i = 0; i < nb_elapsed; ++i)
   {
      board->component_elapsed_time_[i] = GetU32(&p[at]); at += 4;
   }

   for (int i = 0; i < nb_components; ++i)
   {
      IComponent* c = board->component_list_[i];
      const unsigned int elapsed = GetU32(&p[at]); at += 4;
      const unsigned int tick = GetU32(&p[at]); at += 4;
      if (c)
      {
         c->elapsed_time_ = (int)elapsed;
         c->this_tick_time_ = (int)tick;
      }
   }

   return true;
}

void MachineState::WriteZ80(Motherboard* board, std::vector<unsigned char>& out)
{
   Z80* z80 = board->GetProc();

   const size_t length_at = out.size() + 4;
   PutU32(out, kChunkZ80);
   PutU32(out, 0);                       // patched once the payload is known
   const size_t payload_at = out.size();

   PutU32(out, z80->current_opcode_);
   PutU32(out, (unsigned int)z80->t_);
   PutU32(out, (unsigned int)z80->machine_cycle_);
   PutU32(out, z80->counter_);
   PutU32(out, z80->read_count_);
   PutU16(out, z80->address_);
   PutU16(out, z80->current_address_);
   PutU16(out, z80->current_data_);
   PutU16(out, z80->mem_ptr_.w);
   out.push_back(z80->data_);
   out.push_back(z80->q_);
   out.push_back(z80->new_instruction_ ? 1 : 0);
   out.push_back(z80->rw_opcode_ ? 1 : 0);
   out.push_back(z80->carry_set_ ? 1 : 0);
   out.push_back(z80->break_ ? 1 : 0);

   const unsigned int payload_size = (unsigned int)(out.size() - payload_at);
   out[length_at + 0] = payload_size & 0xFF;
   out[length_at + 1] = (payload_size >> 8) & 0xFF;
   out[length_at + 2] = (payload_size >> 16) & 0xFF;
   out[length_at + 3] = (payload_size >> 24) & 0xFF;
}

bool MachineState::ReadZ80(Motherboard* board, const unsigned char* p, size_t size)
{
   if (size < 34) return false;
   Z80* z80 = board->GetProc();
   size_t at = 0;

   // current_function_ and next_function_ are pointers into the CPU's own
   // dispatch tables, so they cannot be written to a buffer and read back in
   // another process. They do not have to be: a state is only ever taken on an
   // instruction boundary, where the pending function is always the fetch, so
   // put the CPU back in that canonical state and let the fields below say
   // where inside the fetch it was. pc_ has already been restored from the
   // .SNA at this point, so this preserves it.
   z80->PrepareForFetch(z80->pc_);

   z80->current_opcode_ = GetU32(&p[at]); at += 4;
   z80->t_ = (int)GetU32(&p[at]); at += 4;
   z80->machine_cycle_ = (Z80::MachineCycle)GetU32(&p[at]); at += 4;
   z80->counter_ = GetU32(&p[at]); at += 4;
   z80->read_count_ = GetU32(&p[at]); at += 4;
   z80->address_ = GetU16(&p[at]); at += 2;
   z80->current_address_ = GetU16(&p[at]); at += 2;
   z80->current_data_ = GetU16(&p[at]); at += 2;
   z80->mem_ptr_.w = GetU16(&p[at]); at += 2;
   z80->data_ = p[at++];
   z80->q_ = p[at++];
   z80->new_instruction_ = p[at++] != 0;
   z80->rw_opcode_ = p[at++] != 0;
   z80->carry_set_ = p[at++] != 0;
   z80->break_ = p[at++] != 0;

   return true;
}

void MachineState::WriteGateArray(Motherboard* board, std::vector<unsigned char>& out)
{
   GateArray* ga = board->GetVGA();

   const size_t length_at = out.size() + 4;
   PutU32(out, kChunkGateArray);
   PutU32(out, 0);
   const size_t payload_at = out.size();

   out.push_back(ga->hsync_counter_);
   out.push_back(ga->vsync_counter_);
   out.push_back(ga->interrupt_counter_);
   out.push_back(ga->wait_for_hsync_);
   out.push_back(ga->interrupt_raised_ ? 1 : 0);
   out.push_back(ga->hsync_ ? 1 : 0);
   out.push_back(ga->vsync_ ? 1 : 0);
   out.push_back(ga->h_old_sync_ ? 1 : 0);
   out.push_back(ga->v_old_sync_ ? 1 : 0);
   // A palette write can be pending when the state is taken.
   out.push_back(ga->buffered_ink_available_ ? 1 : 0);
   PutU32(out, ga->buffered_ink_);

   const unsigned int payload_size = (unsigned int)(out.size() - payload_at);
   out[length_at + 0] = payload_size & 0xFF;
   out[length_at + 1] = (payload_size >> 8) & 0xFF;
   out[length_at + 2] = (payload_size >> 16) & 0xFF;
   out[length_at + 3] = (payload_size >> 24) & 0xFF;
}

bool MachineState::ReadGateArray(Motherboard* board, const unsigned char* p, size_t size)
{
   if (size < 14) return false;
   GateArray* ga = board->GetVGA();
   size_t at = 0;

   ga->hsync_counter_ = p[at++];
   ga->vsync_counter_ = p[at++];
   ga->interrupt_counter_ = p[at++];
   ga->wait_for_hsync_ = p[at++];
   ga->interrupt_raised_ = p[at++] != 0;
   ga->hsync_ = p[at++] != 0;
   ga->vsync_ = p[at++] != 0;
   ga->h_old_sync_ = p[at++] != 0;
   ga->v_old_sync_ = p[at++] != 0;
   ga->buffered_ink_available_ = p[at++] != 0;
   ga->buffered_ink_ = GetU32(&p[at]); at += 4;

   return true;
}

void MachineState::WritePsg(Motherboard* board, std::vector<unsigned char>& out)
{
   Ay8912* psg = board->GetPSG();

   const size_t length_at = out.size() + 4;
   PutU32(out, kChunkPsg);
   PutU32(out, 0);
   const size_t payload_at = out.size();

   out.insert(out.end(), psg->register_, psg->register_ + 16);
   out.push_back(psg->register_address_);

   PutU32(out, psg->channel_a_freq_);   PutU32(out, psg->channel_a_freq_counter_);
   PutU32(out, psg->channel_b_freq_);   PutU32(out, psg->channel_b_freq_counter_);
   PutU32(out, psg->channel_c_freq_);   PutU32(out, psg->channel_c_freq_counter_);
   PutU32(out, psg->noise_frequency_);

   out.push_back(psg->mixer_control_register_);
   out.push_back(psg->channel_a_volume_);
   out.push_back(psg->channel_b_volume_);
   out.push_back(psg->channel_c_volume_);
   out.push_back(psg->enveloppe_volume_);
   PutU32(out, psg->volume_enveloppe_frequency_);
   out.push_back(psg->volume_enveloppe_shape_);
   out.push_back(psg->external_data_register_b_);
   out.push_back(psg->enveloppe_up_ ? 1 : 0);
   out.push_back(psg->enveloppe_stop_ ? 1 : 0);
   out.push_back(psg->register_replaced_ ? 1 : 0);
   out.push_back(psg->register_14_);

   PutU32(out, psg->counter_a_);
   PutU32(out, psg->counter_b_);
   PutU32(out, psg->counter_c_);
   PutU32(out, psg->counter_noise_);
   PutU32(out, psg->counter_env_);
   PutU32(out, psg->counter_state_env_);
   out.push_back(psg->chan_a_high_);
   out.push_back(psg->chan_b_high_);
   out.push_back(psg->chan_c_high_);
   out.push_back(psg->chan_noise_high_);
   PutU32(out, psg->noise_shift_register_);

   const unsigned int payload_size = (unsigned int)(out.size() - payload_at);
   out[length_at + 0] = payload_size & 0xFF;
   out[length_at + 1] = (payload_size >> 8) & 0xFF;
   out[length_at + 2] = (payload_size >> 16) & 0xFF;
   out[length_at + 3] = (payload_size >> 24) & 0xFF;
}

bool MachineState::ReadPsg(Motherboard* board, const unsigned char* p, size_t size)
{
   if (size < 85) return false;
   Ay8912* psg = board->GetPSG();
   size_t at = 0;

   memcpy(psg->register_, &p[at], 16); at += 16;
   psg->register_address_ = p[at++];

   psg->channel_a_freq_ = GetU32(&p[at]); at += 4;
   psg->channel_a_freq_counter_ = GetU32(&p[at]); at += 4;
   psg->channel_b_freq_ = GetU32(&p[at]); at += 4;
   psg->channel_b_freq_counter_ = GetU32(&p[at]); at += 4;
   psg->channel_c_freq_ = GetU32(&p[at]); at += 4;
   psg->channel_c_freq_counter_ = GetU32(&p[at]); at += 4;
   psg->noise_frequency_ = GetU32(&p[at]); at += 4;

   psg->mixer_control_register_ = p[at++];
   psg->channel_a_volume_ = p[at++];
   psg->channel_b_volume_ = p[at++];
   psg->channel_c_volume_ = p[at++];
   psg->enveloppe_volume_ = p[at++];
   psg->volume_enveloppe_frequency_ = GetU32(&p[at]); at += 4;
   psg->volume_enveloppe_shape_ = p[at++];
   psg->external_data_register_b_ = p[at++];
   psg->enveloppe_up_ = p[at++] != 0;
   psg->enveloppe_stop_ = p[at++] != 0;
   psg->register_replaced_ = p[at++] != 0;
   psg->register_14_ = p[at++];

   psg->counter_a_ = GetU32(&p[at]); at += 4;
   psg->counter_b_ = GetU32(&p[at]); at += 4;
   psg->counter_c_ = GetU32(&p[at]); at += 4;
   psg->counter_noise_ = GetU32(&p[at]); at += 4;
   psg->counter_env_ = GetU32(&p[at]); at += 4;
   psg->counter_state_env_ = GetU32(&p[at]); at += 4;
   psg->chan_a_high_ = p[at++];
   psg->chan_b_high_ = p[at++];
   psg->chan_c_high_ = p[at++];
   psg->chan_noise_high_ = p[at++];
   psg->noise_shift_register_ = GetU32(&p[at]); at += 4;

   return true;
}

void MachineState::WriteFdc(Motherboard* board, std::vector<unsigned char>& out)
{
   FDC* f = board->GetFDC();

   const size_t length_at = out.size() + 4;
   PutU32(out, kChunkFdc);
   PutU32(out, 0);
   const size_t payload_at = out.size();

   PutU32(out, (unsigned int)f->state_);
   out.push_back(f->instruction_);
   out.push_back(f->mt_ ? 1 : 0);
   out.push_back(f->mf_ ? 1 : 0);
   out.push_back(f->sk_ ? 1 : 0);
   out.push_back(f->motor_on_ ? 1 : 0);
   out.push_back(f->first_sector_found_ ? 1 : 0);
   PutU32(out, (unsigned int)f->sector_count_);
   PutU32(out, (unsigned int)f->nb_data_in_buffer_);
   PutU32(out, (unsigned int)f->nb_data_offset_);
   out.insert(out.end(), f->data_buffer_, f->data_buffer_ + 1);
   out.push_back(f->recalibrate_ ? 1 : 0);
   out.insert(out.end(), f->parameters_, f->parameters_ + 9);
   out.insert(out.end(), f->results_, f->results_ + 7);
   out.push_back(f->parameters_count_);
   out.push_back(f->results_count_);
   PutU32(out, (unsigned int)f->index_fdc_command_);
   PutU32(out, (unsigned int)f->step_rate_);
   out.push_back(f->track_seeded_);
   out.push_back(f->step_count_);
   PutU32(out, (unsigned int)f->move_size_);
   out.push_back(f->dma_disable_ ? 1 : 0);
   out.push_back(f->ma_ ? 1 : 0);
   out.push_back(f->nw_ ? 1 : 0);
   out.push_back(f->nd_ ? 1 : 0);
   out.push_back(f->or_ ? 1 : 0);
   out.push_back(f->de_ ? 1 : 0);
   out.push_back(f->en_ ? 1 : 0);
   out.push_back(f->status_0_);
   out.push_back(f->status_1_);
   out.push_back(f->status_2_);
   out.push_back(f->status_3_);
   out.push_back(f->hu_);
   out.push_back(f->tp_);
   out.push_back(f->tr_);
   out.push_back(f->hd_);
   out.push_back(f->sc_);
   out.push_back(f->init_r_);
   out.push_back(f->sz_);
   out.push_back(f->ls_);
   out.push_back(f->gp_);
   out.push_back(f->sl_);
   out.push_back(f->fb_);
   out.push_back(f->nm_);
   out.push_back(f->seek_cmd_ ? 1 : 0);
   out.push_back(f->interrupt_code_);
   out.push_back(f->seek_end_ ? 1 : 0);
   out.push_back(f->force_nd_ ? 1 : 0);
   out.push_back(f->last_interrupt_result_);
   out.push_back(f->interrupt_occured_ ? 1 : 0);
   out.push_back(f->ready_line_changed_ ? 1 : 0);
   out.push_back(f->busy_);
   out.push_back(f->main_status_);
   out.push_back(f->data_register_);
   out.push_back(f->cb_ ? 1 : 0);
   out.push_back(f->rqm_ ? 1 : 0);
   out.push_back(f->dio_ ? 1 : 0);
   out.push_back(f->exm_ ? 1 : 0);
   PutU32(out, (unsigned int)f->delay_for_instruction_);
   PutU32(out, (unsigned int)(f->time_ & 0xFFFFFFFFu)); PutU32(out, (unsigned int)(f->time_ >> 32));
   PutU32(out, (unsigned int)(f->time_for_bad_instruction_ & 0xFFFFFFFFu)); PutU32(out, (unsigned int)(f->time_for_bad_instruction_ >> 32));
   out.push_back(f->rw_command_ ? 1 : 0);
   PutU32(out, (unsigned int)f->cnt_exec_);
   PutU32(out, (unsigned int)f->old_state_);
   PutU32(out, (unsigned int)f->current_command_);
   PutU32(out, (unsigned int)f->index_hole_encountered_);
   out.push_back(f->read_deleted_ ? 1 : 0);
   out.push_back(f->write_deleted_ ? 1 : 0);
   out.push_back(f->tc_ ? 1 : 0);
   PutU16(out, (unsigned short)f->read_crc_);
   out.push_back(f->seek_track_ ? 1 : 0);
   out.push_back(f->stp_);
   PutU32(out, (unsigned int)f->format_phase_);
   PutU32(out, (unsigned int)f->format_sector_count_);
   PutU32(out, (unsigned int)f->seek_count_);
   out.insert(out.end(), f->sc_array_, f->sc_array_ + 2);
   PutU32(out, (unsigned int)f->current_drive_);
   PutU32(out, (unsigned int)f->current_command_phase_);
   out.push_back(f->previous_bit_);
   out.push_back(f->sync_count_);
   out.push_back(f->current_data_byte_);
   out.push_back(f->bit_count_);
   PutU32(out, (unsigned int)f->out_index_);
   PutU32(out, (unsigned int)f->data_to_read_);
   PutU32(out, (unsigned int)f->data_to_return_);
   out.push_back(f->first_non_0_ ? 1 : 0);
   PutU32(out, (unsigned int)f->read_sector_state_);
   out.push_back(f->read_track_ ? 1 : 0);
   PutU32(out, (unsigned int)f->write_byte_counter_);
   out.push_back(f->byte_to_write_);
   out.push_back(f->byte_is_sync_ ? 1 : 0);
   PutU32(out, (unsigned int)f->next_phase_);
   out.push_back(f->crc_on_ ? 1 : 0);
   PutU32(out, (unsigned int)f->chrn_byte_count_);
   PutU32(out, (unsigned int)f->crc_cyte_);
   out.push_back(f->delayed_load_ ? 1 : 0);
   PutU32(out, (unsigned int)f->delayed_load_count_);
   PutU32(out, (unsigned int)f->delayed_load_drive_);
   out.push_back(f->delayed_load_container_ ? 1 : 0);
   out.push_back(f->on_index_ ? 1 : 0);
   out.push_back(f->read_data_done_ ? 1 : 0);

   const unsigned int payload_size = (unsigned int)(out.size() - payload_at);
   out[length_at + 0] = payload_size & 0xFF;
   out[length_at + 1] = (payload_size >> 8) & 0xFF;
   out[length_at + 2] = (payload_size >> 16) & 0xFF;
   out[length_at + 3] = (payload_size >> 24) & 0xFF;
}

bool MachineState::ReadFdc(Motherboard* board, const unsigned char* p, size_t size)
{
   FDC* f = board->GetFDC();
   size_t at = 0;

   f->state_ = (FDC::Phase)GetU32(&p[at]); at += 4;
   f->instruction_ = p[at++];
   f->mt_ = p[at++] != 0;
   f->mf_ = p[at++] != 0;
   f->sk_ = p[at++] != 0;
   f->motor_on_ = p[at++] != 0;
   f->first_sector_found_ = p[at++] != 0;
   f->sector_count_ = GetU32(&p[at]); at += 4;
   f->nb_data_in_buffer_ = GetU32(&p[at]); at += 4;
   f->nb_data_offset_ = GetU32(&p[at]); at += 4;
   memcpy(f->data_buffer_, &p[at], 1); at += 1;
   f->recalibrate_ = p[at++] != 0;
   memcpy(f->parameters_, &p[at], 9); at += 9;
   memcpy(f->results_, &p[at], 7); at += 7;
   f->parameters_count_ = p[at++];
   f->results_count_ = p[at++];
   f->index_fdc_command_ = GetU32(&p[at]); at += 4;
   f->step_rate_ = GetU32(&p[at]); at += 4;
   f->track_seeded_ = p[at++];
   f->step_count_ = p[at++];
   f->move_size_ = GetU32(&p[at]); at += 4;
   f->dma_disable_ = p[at++] != 0;
   f->ma_ = p[at++] != 0;
   f->nw_ = p[at++] != 0;
   f->nd_ = p[at++] != 0;
   f->or_ = p[at++] != 0;
   f->de_ = p[at++] != 0;
   f->en_ = p[at++] != 0;
   f->status_0_ = p[at++];
   f->status_1_ = p[at++];
   f->status_2_ = p[at++];
   f->status_3_ = p[at++];
   f->hu_ = p[at++];
   f->tp_ = p[at++];
   f->tr_ = p[at++];
   f->hd_ = p[at++];
   f->sc_ = p[at++];
   f->init_r_ = p[at++];
   f->sz_ = p[at++];
   f->ls_ = p[at++];
   f->gp_ = p[at++];
   f->sl_ = p[at++];
   f->fb_ = p[at++];
   f->nm_ = p[at++];
   f->seek_cmd_ = p[at++] != 0;
   f->interrupt_code_ = p[at++];
   f->seek_end_ = p[at++] != 0;
   f->force_nd_ = p[at++] != 0;
   f->last_interrupt_result_ = p[at++];
   f->interrupt_occured_ = p[at++] != 0;
   f->ready_line_changed_ = p[at++] != 0;
   f->busy_ = p[at++];
   f->main_status_ = p[at++];
   f->data_register_ = p[at++];
   f->cb_ = p[at++] != 0;
   f->rqm_ = p[at++] != 0;
   f->dio_ = p[at++] != 0;
   f->exm_ = p[at++] != 0;
   f->delay_for_instruction_ = GetU32(&p[at]); at += 4;
   f->time_ = GetU32(&p[at]) | ((uint64_t)GetU32(&p[at+4]) << 32); at += 8;
   f->time_for_bad_instruction_ = GetU32(&p[at]) | ((uint64_t)GetU32(&p[at+4]) << 32); at += 8;
   f->rw_command_ = p[at++] != 0;
   f->cnt_exec_ = GetU32(&p[at]); at += 4;
   f->old_state_ = (FDC::Phase)GetU32(&p[at]); at += 4;
   f->current_command_ = (FDC::Commands)GetU32(&p[at]); at += 4;
   f->index_hole_encountered_ = GetU32(&p[at]); at += 4;
   f->read_deleted_ = p[at++] != 0;
   f->write_deleted_ = p[at++] != 0;
   f->tc_ = p[at++] != 0;
   f->read_crc_ = GetU16(&p[at]); at += 2;
   f->seek_track_ = p[at++] != 0;
   f->stp_ = p[at++];
   f->format_phase_ = (FDC::MFMPhase)GetU32(&p[at]); at += 4;
   f->format_sector_count_ = GetU32(&p[at]); at += 4;
   f->seek_count_ = GetU32(&p[at]); at += 4;
   memcpy(f->sc_array_, &p[at], 2); at += 2;
   f->current_drive_ = GetU32(&p[at]); at += 4;
   f->current_command_phase_ = (FDC::SectorPhase)GetU32(&p[at]); at += 4;
   f->previous_bit_ = p[at++];
   f->sync_count_ = p[at++];
   f->current_data_byte_ = p[at++];
   f->bit_count_ = p[at++];
   f->out_index_ = GetU32(&p[at]); at += 4;
   f->data_to_read_ = GetU32(&p[at]); at += 4;
   f->data_to_return_ = GetU32(&p[at]); at += 4;
   f->first_non_0_ = p[at++] != 0;
   f->read_sector_state_ = GetU32(&p[at]); at += 4;
   f->read_track_ = p[at++] != 0;
   f->write_byte_counter_ = GetU32(&p[at]); at += 4;
   f->byte_to_write_ = p[at++];
   f->byte_is_sync_ = p[at++] != 0;
   f->next_phase_ = (FDC::MFMPhase)GetU32(&p[at]); at += 4;
   f->crc_on_ = p[at++] != 0;
   f->chrn_byte_count_ = GetU32(&p[at]); at += 4;
   f->crc_cyte_ = GetU32(&p[at]); at += 4;
   f->delayed_load_ = p[at++] != 0;
   f->delayed_load_count_ = GetU32(&p[at]); at += 4;
   f->delayed_load_drive_ = GetU32(&p[at]); at += 4;
   f->delayed_load_container_ = p[at++] != 0;
   f->on_index_ = p[at++] != 0;
   f->read_data_done_ = p[at++] != 0;

   // Writer and reader must agree byte for byte; if they ever drift apart this
   // catches it here rather than in a machine that quietly runs wrong.
   return (at == size);
}

void MachineState::WriteDrives(Motherboard* board, std::vector<unsigned char>& out)
{
   FDC* fdc = board->GetFDC();

   const size_t length_at = out.size() + 4;
   PutU32(out, kChunkDrives);
   PutU32(out, 0);
   const size_t payload_at = out.size();

   const unsigned int nb_drives = (unsigned int)(sizeof(fdc->disk_) / sizeof(fdc->disk_[0]));
   PutU32(out, nb_drives);

   for (unsigned int d = 0; d < nb_drives; ++d)
   {
      DiskGen* g = &fdc->disk_[d];

      // Identity of the media, so the reader can refuse a mismatch.
      IDisk* disk = g->disk_;
      out.push_back(g->disk_present_ ? 1 : 0);
      out.push_back(disk ? 1 : 0);
      PutU32(out, disk ? (unsigned int)disk->GetNumberOfSide() : 0);
      PutU32(out, disk ? disk->GetNumberOfTracks() : 0);

      // Position within that media.
      PutU32(out, disk ? disk->head_position_ : 0);

      PutU32(out, (unsigned int)g->encode_scheme_);
      PutU32(out, (unsigned int)g->current_side_);
      PutU32(out, g->current_track_);
      PutU32(out, g->side_0_number_);
      PutU32(out, (unsigned int)g->write_bit_);
      PutU32(out, (unsigned int)g->current_speed_);
      PutU32(out, (unsigned int)g->final_speed_);
      PutU32(out, (unsigned int)g->speed_change_counter_);
      PutU16(out, g->byte_to_write_);
      PutU16(out, g->current_mfm_byte_);
      out.push_back(g->on_index_hole_ ? 1 : 0);
      out.push_back(g->write_protection_on_ ? 1 : 0);
      out.push_back(g->fixed_speed_ ? 1 : 0);
      out.push_back(g->read_ ? 1 : 0);
      out.push_back(g->sync_write_ ? 1 : 0);
      out.push_back(g->sync_found_ ? 1 : 0);
      out.push_back(g->byte_ready_ ? 1 : 0);
      out.push_back(g->data_bit_ ? 1 : 0);
      out.push_back(g->motor_on_ ? 1 : 0);
      out.push_back(g->new_bit_available_ ? 1 : 0);

      // Floats go out as their bits: a decimal round trip would not come back
      // to the same value, and the timing depends on the exact one.
      unsigned int bits;
      memcpy(&bits, &g->time_for_one_bit_, 4); PutU32(out, bits);
      memcpy(&bits, &g->timer_count_, 4);      PutU32(out, bits);
   }

   const unsigned int payload_size = (unsigned int)(out.size() - payload_at);
   out[length_at + 0] = payload_size & 0xFF;
   out[length_at + 1] = (payload_size >> 8) & 0xFF;
   out[length_at + 2] = (payload_size >> 16) & 0xFF;
   out[length_at + 3] = (payload_size >> 24) & 0xFF;
}

bool MachineState::ReadDrives(Motherboard* board, const unsigned char* p, size_t size)
{
   FDC* fdc = board->GetFDC();
   size_t at = 0;
   if (size < 4) return false;

   const unsigned int nb_drives = GetU32(&p[at]); at += 4;
   if (nb_drives != (unsigned int)(sizeof(fdc->disk_) / sizeof(fdc->disk_[0]))) return false;

   for (unsigned int d = 0; d < nb_drives; ++d)
   {
      if (at + 68 > size) return false;
      DiskGen* g = &fdc->disk_[d];
      IDisk* disk = g->disk_;

      const bool was_present = p[at++] != 0;
      const bool had_disk = p[at++] != 0;
      const unsigned int sides = GetU32(&p[at]); at += 4;
      const unsigned int tracks = GetU32(&p[at]); at += 4;
      const unsigned int head_position = GetU32(&p[at]); at += 4;

      // Refuse rather than seek into a disk that is not the one described.
      if (was_present != g->disk_present_) return false;
      if (had_disk != (disk != nullptr)) return false;
      if (disk != nullptr)
      {
         if (sides != (unsigned int)disk->GetNumberOfSide()) return false;
         if (tracks != disk->GetNumberOfTracks()) return false;
         disk->head_position_ = head_position;
      }

      g->encode_scheme_ = (DiskGen::CodageMfm)GetU32(&p[at]); at += 4;
      g->current_side_ = (int)GetU32(&p[at]); at += 4;
      g->current_track_ = GetU32(&p[at]); at += 4;
      g->side_0_number_ = GetU32(&p[at]); at += 4;
      g->write_bit_ = (int)GetU32(&p[at]); at += 4;
      g->current_speed_ = (int)GetU32(&p[at]); at += 4;
      g->final_speed_ = (int)GetU32(&p[at]); at += 4;
      g->speed_change_counter_ = (int)GetU32(&p[at]); at += 4;
      g->byte_to_write_ = GetU16(&p[at]); at += 2;
      g->current_mfm_byte_ = GetU16(&p[at]); at += 2;
      g->on_index_hole_ = p[at++] != 0;
      g->write_protection_on_ = p[at++] != 0;
      g->fixed_speed_ = p[at++] != 0;
      g->read_ = p[at++] != 0;
      g->sync_write_ = p[at++] != 0;
      g->sync_found_ = p[at++] != 0;
      g->byte_ready_ = p[at++] != 0;
      g->data_bit_ = p[at++] != 0;
      g->motor_on_ = p[at++] != 0;
      g->new_bit_available_ = p[at++] != 0;

      unsigned int bits;
      bits = GetU32(&p[at]); at += 4; memcpy(&g->time_for_one_bit_, &bits, 4);
      bits = GetU32(&p[at]); at += 4; memcpy(&g->timer_count_, &bits, 4);
   }

   return (at == size);
}

void MachineState::WriteCrtc(Motherboard* board, std::vector<unsigned char>& out)
{
   CRTC* c = board->GetCRTC();

   const size_t length_at = out.size() + 4;
   PutU32(out, kChunkCrtc);
   PutU32(out, 0);
   const size_t payload_at = out.size();

   out.insert(out.end(), c->registers_list_, c->registers_list_ + 32);
   out.insert(out.end(), c->registers_mask_, c->registers_mask_ + 32);
   PutU16(out, c->ma_);
   PutU16(out, c->bu_);
   PutU32(out, (unsigned int)c->sscr_bit_8_);
   out.push_back(c->vlc_);
   out.push_back(c->adddress_register_);
   out.push_back(c->status_register_);
   out.push_back(c->status1_);
   out.push_back(c->status2_);
   out.push_back(c->hcc_);
   out.push_back(c->horinzontal_pulse_);
   out.push_back(c->vcc_);
   out.push_back(c->scanline_vbl_);
   out.push_back(c->vertical_sync_width_);
   out.push_back(c->horizontal_sync_width_);
   out.push_back(c->vertical_adjust_counter_);
   out.push_back(c->r4_reached_ ? 1 : 0);
   out.push_back(c->ff1_ ? 1 : 0);
   out.push_back(c->ff3_ ? 1 : 0);
   out.push_back(c->ff4_ ? 1 : 0);
   out.push_back(c->mux_ ? 1 : 0);
   out.push_back(c->mux_set_ ? 1 : 0);
   out.push_back(c->mux_reset_ ? 1 : 0);
   out.push_back(c->lightpen_input_ ? 1 : 0);
   out.push_back(c->r9_triggered_ ? 1 : 0);
   out.push_back(c->r4_triggered_ ? 1 : 0);
   out.push_back(c->even_field_ ? 1 : 0);

   const unsigned int payload_size = (unsigned int)(out.size() - payload_at);
   out[length_at + 0] = payload_size & 0xFF;
   out[length_at + 1] = (payload_size >> 8) & 0xFF;
   out[length_at + 2] = (payload_size >> 16) & 0xFF;
   out[length_at + 3] = (payload_size >> 24) & 0xFF;
}

bool MachineState::ReadCrtc(Motherboard* board, const unsigned char* p, size_t size)
{
   CRTC* c = board->GetCRTC();
   size_t at = 0;
   // 32 + 32 registers, ma_, bu_, sscr_bit_8_, then twelve bytes and eleven flags.
   if (size < 95) return false;

   memcpy(c->registers_list_, &p[at], 32); at += 32;
   memcpy(c->registers_mask_, &p[at], 32); at += 32;
   c->ma_ = GetU16(&p[at]); at += 2;
   c->bu_ = GetU16(&p[at]); at += 2;
   c->sscr_bit_8_ = (int)GetU32(&p[at]); at += 4;
   c->vlc_ = p[at++];
   c->adddress_register_ = p[at++];
   c->status_register_ = p[at++];
   c->status1_ = p[at++];
   c->status2_ = p[at++];
   c->hcc_ = p[at++];
   c->horinzontal_pulse_ = p[at++];
   c->vcc_ = p[at++];
   c->scanline_vbl_ = p[at++];
   c->vertical_sync_width_ = p[at++];
   c->horizontal_sync_width_ = p[at++];
   c->vertical_adjust_counter_ = p[at++];
   c->r4_reached_ = p[at++] != 0;
   c->ff1_ = p[at++] != 0;
   c->ff3_ = p[at++] != 0;
   c->ff4_ = p[at++] != 0;
   c->mux_ = p[at++] != 0;
   c->mux_set_ = p[at++] != 0;
   c->mux_reset_ = p[at++] != 0;
   c->lightpen_input_ = p[at++] != 0;
   c->r9_triggered_ = p[at++] != 0;
   c->r4_triggered_ = p[at++] != 0;
   c->even_field_ = p[at++] != 0;

   return (at == size);
}

void MachineState::WriteTape(Motherboard* board, std::vector<unsigned char>& out)
{
   CTape* t = board->GetTape();

   const size_t length_at = out.size() + 4;
   PutU32(out, kChunkTape);
   PutU32(out, 0);
   const size_t payload_at = out.size();

   // Identity of the tape in the deck.
   out.push_back(t->is_tape_inserted_ ? 1 : 0);
   out.push_back(t->tape_array_ != nullptr ? 1 : 0);
   PutU32(out, t->nb_inversions_);
   PutU32(out, t->array_size_);
   PutU32(out, t->nb_blocks_);

   // Where it is.
   PutU32(out, t->tape_position_);
   PutU32(out, (unsigned int)(t->remaining_reversal_flux_ & 0xFFFFFFFFu));
   PutU32(out, (unsigned int)(t->remaining_reversal_flux_ >> 32));
   PutU32(out, (unsigned int)(t->counter_us_ & 0xFFFFFFFFu));
   PutU32(out, (unsigned int)(t->counter_us_ >> 32));
   PutU32(out, (unsigned int)t->counter_sec_);
   PutU32(out, t->tape_length_);

   out.push_back(t->motor_on_ ? 1 : 0);
   out.push_back(t->previous_motor_on_ ? 1 : 0);
   out.push_back(t->next_motor_state_ ? 1 : 0);
   PutU32(out, (unsigned int)t->time_to_change_motor_state_);
   out.push_back(t->play_ ? 1 : 0);
   out.push_back(t->record_ ? 1 : 0);
   out.push_back(t->start_record_ ? 1 : 0);
   out.push_back(t->tape_changed_ ? 1 : 0);
   out.push_back(t->pending_tape_ ? 1 : 0);
   out.push_back(t->current_level_ ? 1 : 0);
   out.push_back(t->polarity_inversion_ ? 1 : 0);

   PutU32(out, (unsigned int)t->frequency_);
   PutU16(out, t->pilot_pulse_);
   PutU16(out, t->pilot_length_);
   PutU16(out, t->zero_);
   PutU16(out, t->one_);
   PutU32(out, (unsigned int)t->current_block_type_);
   PutU32(out, (unsigned int)t->current_block_);
   PutU32(out, t->nb_samples_);
   PutU32(out, (unsigned int)t->nb_sample_to_read_);
   PutU32(out, (unsigned int)t->carry_);
   PutU32(out, (unsigned int)t->oldcarry_);
   PutF64(out, t->sample_rate_);
   PutF64(out, t->last_time_);

   const unsigned int payload_size = (unsigned int)(out.size() - payload_at);
   out[length_at + 0] = payload_size & 0xFF;
   out[length_at + 1] = (payload_size >> 8) & 0xFF;
   out[length_at + 2] = (payload_size >> 16) & 0xFF;
   out[length_at + 3] = (payload_size >> 24) & 0xFF;
}

bool MachineState::ReadTape(Motherboard* board, const unsigned char* p, size_t size)
{
   CTape* t = board->GetTape();
   size_t at = 0;
   if (size < 100) return false;

   const bool was_inserted = p[at++] != 0;
   const bool had_flux = p[at++] != 0;
   const unsigned int nb_inversions = GetU32(&p[at]); at += 4;
   const unsigned int array_size = GetU32(&p[at]); at += 4;
   const unsigned int nb_blocks = GetU32(&p[at]); at += 4;

   // Refuse rather than seek into a tape that is not the one described.
   if (was_inserted != t->is_tape_inserted_) return false;
   if (had_flux != (t->tape_array_ != nullptr)) return false;
   if (nb_inversions != t->nb_inversions_) return false;
   if (array_size != t->array_size_) return false;
   if (nb_blocks != t->nb_blocks_) return false;

   t->tape_position_ = GetU32(&p[at]); at += 4;
   t->remaining_reversal_flux_ =
      (unsigned long long)GetU32(&p[at]) | ((unsigned long long)GetU32(&p[at+4]) << 32); at += 8;
   t->counter_us_ =
      (unsigned long long)GetU32(&p[at]) | ((unsigned long long)GetU32(&p[at+4]) << 32); at += 8;
   t->counter_sec_ = (int)GetU32(&p[at]); at += 4;
   t->tape_length_ = GetU32(&p[at]); at += 4;

   t->motor_on_ = p[at++] != 0;
   t->previous_motor_on_ = p[at++] != 0;
   t->next_motor_state_ = p[at++] != 0;
   t->time_to_change_motor_state_ = (int)GetU32(&p[at]); at += 4;
   t->play_ = p[at++] != 0;
   t->record_ = p[at++] != 0;
   t->start_record_ = p[at++] != 0;
   t->tape_changed_ = p[at++] != 0;
   t->pending_tape_ = p[at++] != 0;
   t->current_level_ = p[at++] != 0;
   t->polarity_inversion_ = p[at++] != 0;

   t->frequency_ = (int)GetU32(&p[at]); at += 4;
   t->pilot_pulse_ = GetU16(&p[at]); at += 2;
   t->pilot_length_ = GetU16(&p[at]); at += 2;
   t->zero_ = GetU16(&p[at]); at += 2;
   t->one_ = GetU16(&p[at]); at += 2;
   t->current_block_type_ = (int)GetU32(&p[at]); at += 4;
   t->current_block_ = (int)GetU32(&p[at]); at += 4;
   t->nb_samples_ = GetU32(&p[at]); at += 4;
   t->nb_sample_to_read_ = (int)GetU32(&p[at]); at += 4;
   t->carry_ = (int)GetU32(&p[at]); at += 4;
   t->oldcarry_ = (int)GetU32(&p[at]); at += 4;
   t->sample_rate_ = GetF64(&p[at]); at += 8;
   t->last_time_ = GetF64(&p[at]); at += 8;

   return (at == size);
}

void MachineState::WritePpi(Motherboard* board, std::vector<unsigned char>& out)
{
   PPI8255* ppi = board->GetPPI();

   const size_t length_at = out.size() + 4;
   PutU32(out, kChunkPpi);
   PutU32(out, 0);
   const size_t payload_at = out.size();

   out.push_back(ppi->tape_level_);
   out.push_back(ppi->tape_write_data_level_ ? 1 : 0);

   const unsigned int payload_size = (unsigned int)(out.size() - payload_at);
   out[length_at + 0] = payload_size & 0xFF;
   out[length_at + 1] = (payload_size >> 8) & 0xFF;
   out[length_at + 2] = (payload_size >> 16) & 0xFF;
   out[length_at + 3] = (payload_size >> 24) & 0xFF;
}

bool MachineState::ReadPpi(Motherboard* board, const unsigned char* p, size_t size)
{
   PPI8255* ppi = board->GetPPI();
   if (size != 2) return false;
   ppi->tape_level_ = p[0];
   ppi->tape_write_data_level_ = p[1] != 0;
   return true;
}

void MachineState::WriteDma(Motherboard* board, std::vector<unsigned char>& out)
{
   const size_t length_at = out.size() + 4;
   PutU32(out, kChunkDma);
   PutU32(out, 0);
   const size_t payload_at = out.size();

   PutU32(out, 3);
   for (int i = 0; i < 3; ++i)
   {
      DMA* d = board->GetDMA(i);
      PutU32(out, (unsigned int)d->dma_cycle_);
      PutU32(out, (unsigned int)d->pause_counter_);
      PutU32(out, (unsigned int)d->repeat_counter_);
      PutU16(out, d->repeat_addr_);
      PutU16(out, d->curent_instr_);
      out.push_back(d->enable_next_ ? 1 : 0);
      out.push_back(d->ppr_);
      out.push_back(d->interrupt_on_ ? 1 : 0);
      out.push_back(d->prescalar_);
      out.push_back(d->prescalar_counter_);
   }

   const unsigned int payload_size = (unsigned int)(out.size() - payload_at);
   out[length_at + 0] = payload_size & 0xFF;
   out[length_at + 1] = (payload_size >> 8) & 0xFF;
   out[length_at + 2] = (payload_size >> 16) & 0xFF;
   out[length_at + 3] = (payload_size >> 24) & 0xFF;
}

bool MachineState::ReadDma(Motherboard* board, const unsigned char* p, size_t size)
{
   size_t at = 0;
   if (size < 4) return false;
   if (GetU32(&p[at]) != 3) return false;
   at += 4;
   if (size != at + 3 * 21) return false;

   for (int i = 0; i < 3; ++i)
   {
      DMA* d = board->GetDMA(i);
      d->dma_cycle_ = (decltype(d->dma_cycle_))GetU32(&p[at]); at += 4;
      d->pause_counter_ = (int)GetU32(&p[at]); at += 4;
      d->repeat_counter_ = (int)GetU32(&p[at]); at += 4;
      d->repeat_addr_ = GetU16(&p[at]); at += 2;
      d->curent_instr_ = GetU16(&p[at]); at += 2;
      d->enable_next_ = p[at++] != 0;
      d->ppr_ = p[at++];
      d->interrupt_on_ = p[at++] != 0;
      d->prescalar_ = p[at++];
      d->prescalar_counter_ = p[at++];
   }
   return true;
}

void MachineState::WriteYmz(YMZ294* y, std::vector<unsigned char>& out)
{
   out.insert(out.end(), y->register_, y->register_ + 16);
   out.push_back(y->register_address_);
   PutU32(out, y->channel_a_freq_);   PutU32(out, y->channel_a_freq_counter_);
   PutU32(out, y->channel_b_freq_);   PutU32(out, y->channel_b_freq_counter_);
   PutU32(out, y->channel_c_freq_);   PutU32(out, y->channel_c_freq_counter_);
   PutU32(out, y->noise_frequency_);
   out.push_back(y->mixer_control_register_);
   out.push_back(y->channel_a_volume_);
   out.push_back(y->channel_b_volume_);
   out.push_back(y->channel_c_volume_);
   out.push_back(y->envelope_volume_);
   PutU32(out, y->volume_enveloppe_frequency_);
   out.push_back(y->volume_enveloppe_shape_);
   out.push_back(y->up_ ? 1 : 0);
   out.push_back(y->stop_ ? 1 : 0);
   PutU32(out, y->counter_a_); PutU32(out, y->counter_b_); PutU32(out, y->counter_c_);
   PutU32(out, y->counter_noise_); PutU32(out, y->counter_env_); PutU32(out, y->counter_state_env_);
   out.push_back(y->channel_a_high_); out.push_back(y->channel_b_high_);
   out.push_back(y->channel_c_high_); out.push_back(y->channel_noise_high_);
   PutU32(out, y->noise_shift_register_);
}

void MachineState::ReadYmz(YMZ294* y, const unsigned char* p, size_t& at)
{
   memcpy(y->register_, &p[at], 16); at += 16;
   y->register_address_ = p[at++];
   y->channel_a_freq_ = GetU32(&p[at]); at += 4; y->channel_a_freq_counter_ = GetU32(&p[at]); at += 4;
   y->channel_b_freq_ = GetU32(&p[at]); at += 4; y->channel_b_freq_counter_ = GetU32(&p[at]); at += 4;
   y->channel_c_freq_ = GetU32(&p[at]); at += 4; y->channel_c_freq_counter_ = GetU32(&p[at]); at += 4;
   y->noise_frequency_ = GetU32(&p[at]); at += 4;
   y->mixer_control_register_ = p[at++];
   y->channel_a_volume_ = p[at++]; y->channel_b_volume_ = p[at++]; y->channel_c_volume_ = p[at++];
   y->envelope_volume_ = p[at++];
   y->volume_enveloppe_frequency_ = GetU32(&p[at]); at += 4;
   y->volume_enveloppe_shape_ = p[at++];
   y->up_ = p[at++] != 0; y->stop_ = p[at++] != 0;
   y->counter_a_ = GetU32(&p[at]); at += 4; y->counter_b_ = GetU32(&p[at]); at += 4;
   y->counter_c_ = GetU32(&p[at]); at += 4; y->counter_noise_ = GetU32(&p[at]); at += 4;
   y->counter_env_ = GetU32(&p[at]); at += 4; y->counter_state_env_ = GetU32(&p[at]); at += 4;
   y->channel_a_high_ = p[at++]; y->channel_b_high_ = p[at++];
   y->channel_c_high_ = p[at++]; y->channel_noise_high_ = p[at++];
   y->noise_shift_register_ = GetU32(&p[at]); at += 4;
}

void MachineState::WritePlayCity(Motherboard* board, std::vector<unsigned char>& out)
{
   PlayCity* pc = board->GetPlayCity();

   const size_t length_at = out.size() + 4;
   PutU32(out, kChunkPlayCity);
   PutU32(out, 0);
   const size_t payload_at = out.size();

   out.push_back(pc->z84c30_.interrupt_vector_);
   for (int i = 0; i < 4; ++i)
   {
      Z84C30::CTCCounter* ch = &pc->z84c30_.channel_[i];
      out.push_back(ch->enabled_ ? 1 : 0);
      out.push_back(ch->control_);
      out.push_back(ch->time_constant_);
      out.push_back(ch->down_counter_);
      out.push_back(ch->prescaler_);
      out.push_back(ch->decrement_ ? 1 : 0);
      out.push_back(ch->wait_for_time_constraint_ ? 1 : 0);
      out.push_back(ch->count_enabled_ ? 1 : 0);
      out.push_back(ch->reload_ ? 1 : 0);
   }
   WriteYmz(&pc->ymz294_1_, out);
   WriteYmz(&pc->ymz294_2_, out);

   // PlayCity's own timing state, which is not in either chip. next_call_ymz_
   // in particular is the field whose uninitialised value once made the whole
   // expansion silent: leaving it out of a state would bring that back on every
   // restore.
   out.push_back(pc->trg0_update_ ? 1 : 0);
   out.push_back(pc->drop_next_tick_ ? 1 : 0);
   PutU32(out, (unsigned int)pc->next_call_ymz_);
   out.push_back(pc->inner_line_.signal_up_ ? 1 : 0);
   out.push_back(pc->inner_line_channel_23_.signal_up_ ? 1 : 0);

   const unsigned int payload_size = (unsigned int)(out.size() - payload_at);
   out[length_at + 0] = payload_size & 0xFF;
   out[length_at + 1] = (payload_size >> 8) & 0xFF;
   out[length_at + 2] = (payload_size >> 16) & 0xFF;
   out[length_at + 3] = (payload_size >> 24) & 0xFF;
}

bool MachineState::ReadPlayCity(Motherboard* board, const unsigned char* p, size_t size)
{
   PlayCity* pc = board->GetPlayCity();
   size_t at = 0;
   if (size < 1 + 4 * 9) return false;

   pc->z84c30_.interrupt_vector_ = p[at++];
   for (int i = 0; i < 4; ++i)
   {
      Z84C30::CTCCounter* ch = &pc->z84c30_.channel_[i];
      ch->enabled_ = p[at++] != 0;
      ch->control_ = p[at++];
      ch->time_constant_ = p[at++];
      ch->down_counter_ = p[at++];
      ch->prescaler_ = p[at++];
      ch->decrement_ = p[at++] != 0;
      ch->wait_for_time_constraint_ = p[at++] != 0;
      ch->count_enabled_ = p[at++] != 0;
      ch->reload_ = p[at++] != 0;
   }
   ReadYmz(&pc->ymz294_1_, p, at);
   ReadYmz(&pc->ymz294_2_, p, at);

   pc->trg0_update_ = p[at++] != 0;
   pc->drop_next_tick_ = p[at++] != 0;
   pc->next_call_ymz_ = (int)GetU32(&p[at]); at += 4;
   pc->inner_line_.signal_up_ = p[at++] != 0;
   pc->inner_line_channel_23_.signal_up_ = p[at++] != 0;

   return (at == size);
}

void MachineState::WriteExtendedRam(Motherboard* board, std::vector<unsigned char>& out)
{
   Memory* mem = board->GetMem();

   const size_t length_at = out.size() + 4;
   PutU32(out, kChunkExtendedRam);
   PutU32(out, 0);
   const size_t payload_at = out.size();

   // Which pages this machine has, so a state cannot be applied to a machine
   // configured with less memory than it describes.
   unsigned int available = 0;
   for (int page = 0; page < 8; ++page)
      if (mem->extended_ram_available_[page]) available |= (1u << page);
   PutU32(out, available);

   for (int page = 1; page < 8; ++page)
   {
      if (!mem->extended_ram_available_[page]) continue;
      for (int bank = 0; bank < 4; ++bank)
         out.insert(out.end(), mem->extended_ram_buffer_[page][bank],
                               mem->extended_ram_buffer_[page][bank] + 0x4000);
   }

   const unsigned int payload_size = (unsigned int)(out.size() - payload_at);
   out[length_at + 0] = payload_size & 0xFF;
   out[length_at + 1] = (payload_size >> 8) & 0xFF;
   out[length_at + 2] = (payload_size >> 16) & 0xFF;
   out[length_at + 3] = (payload_size >> 24) & 0xFF;
}

bool MachineState::ReadExtendedRam(Motherboard* board, const unsigned char* p, size_t size)
{
   Memory* mem = board->GetMem();
   if (size < 4) return false;

   const unsigned int available = GetU32(&p[0]);
   size_t at = 4;

   unsigned int machine_available = 0;
   for (int page = 0; page < 8; ++page)
      if (mem->extended_ram_available_[page]) machine_available |= (1u << page);

   // Refuse rather than half-fill a machine that has different memory: writing
   // a 576 KB state into a 128 KB machine would drop 448 KB on the floor, and
   // the reverse would leave stale pages behind.
   if (available != machine_available) return false;

   size_t expected = 4;
   for (int page = 1; page < 8; ++page)
      if (available & (1u << page)) expected += 4 * 0x4000;
   if (size != expected) return false;

   for (int page = 1; page < 8; ++page)
   {
      if (!(available & (1u << page))) continue;
      for (int bank = 0; bank < 4; ++bank)
      {
         memcpy(mem->extended_ram_buffer_[page][bank], &p[at], 0x4000);
         at += 0x4000;
      }
   }

   return (at == size);
}

// Does this state describe the machine it is about to be loaded into?
//
// A chunk that refuses halfway through leaves the machine part restored: the
// .SNA and every chunk before it have already been applied, and the caller is
// told the load failed while the emulator carries on with a machine that is
// neither where it was nor where the state wanted it. The frontend shows an
// error and the user keeps playing something broken.
//
// The refusals that happen in practice are the ones about the machine rather
// than the buffer -- a state from a 576 KB machine on a 128 KB one, from a
// different disk, a different tape -- so those are checked first, across every
// chunk, and the load is abandoned before a single byte is written. A size that
// disagrees means a corrupt buffer, which is a different problem and still
// fails partway.
void MachineState::WriteCartridge(Motherboard* board, std::vector<unsigned char>& out)
{
   Memory* mem = board->GetMem();

   // Nothing plugged: no chunk. A machine that never had a cartridge should
   // not carry one in its state, and an older state without the chunk still
   // loads.
   if (mem->cartridge_crc_ == 0) return;

   const size_t length_at = out.size() + 4;
   PutU32(out, kChunkCartridge);
   PutU32(out, 0);
   const size_t payload_at = out.size();

   PutU32(out, mem->cartridge_crc_);
   PutU32(out, (unsigned int)mem->cartridge_list_.size());

   const unsigned int payload_size = (unsigned int)(out.size() - payload_at);
   out[length_at + 0] = payload_size & 0xFF;
   out[length_at + 1] = (payload_size >> 8) & 0xFF;
   out[length_at + 2] = (payload_size >> 16) & 0xFF;
   out[length_at + 3] = (payload_size >> 24) & 0xFF;
}

bool MachineState::DescribesThisMachine(Motherboard* board,
                                        const unsigned char* buffer, size_t size,
                                        size_t first_chunk)
{
   size_t at = first_chunk;
   while (at + 8 <= size)
   {
      const unsigned int id = GetU32(&buffer[at]);
      const unsigned int length = GetU32(&buffer[at + 4]);
      at += 8;
      if (at + length > size) return false;

      const unsigned char* p = &buffer[at];

      if (id == kChunkScheduler)
      {
         if (length < 12) return false;
         if ((int)GetU32(&p[4]) != board->nb_components_) return false;
      }
      else if (id == kChunkDrives)
      {
         if (length < 4) return false;
         const unsigned int nb_drives = GetU32(&p[0]);
         if (nb_drives != (unsigned int)(sizeof(board->GetFDC()->disk_)
                                       / sizeof(board->GetFDC()->disk_[0]))) return false;
         size_t d_at = 4;
         for (unsigned int d = 0; d < nb_drives; ++d)
         {
            // Identity, then position: 14 + 32 + 4 + 10 + 8 bytes per drive.
            const size_t kPerDrive = 68;
            if (d_at + kPerDrive > length) return false;
            DiskGen* g = &board->GetFDC()->disk_[d];
            IDisk* disk = g->disk_;
            const bool was_present = p[d_at] != 0;
            const bool had_disk = p[d_at + 1] != 0;
            const unsigned int sides = GetU32(&p[d_at + 2]);
            const unsigned int tracks = GetU32(&p[d_at + 6]);
            if (was_present != g->disk_present_) return false;
            if (had_disk != (disk != nullptr)) return false;
            if (disk != nullptr)
            {
               if (sides != (unsigned int)disk->GetNumberOfSide()) return false;
               if (tracks != disk->GetNumberOfTracks()) return false;
            }
            d_at += kPerDrive;
         }
      }
      else if (id == kChunkTape)
      {
         if (length < 14) return false;
         CTape* t = board->GetTape();
         if ((p[0] != 0) != t->is_tape_inserted_) return false;
         if ((p[1] != 0) != (t->tape_array_ != nullptr)) return false;
         if (GetU32(&p[2]) != t->nb_inversions_) return false;
         if (GetU32(&p[6]) != t->array_size_) return false;
         if (GetU32(&p[10]) != t->nb_blocks_) return false;
      }
      else if (id == kChunkCartridge)
      {
         if (length < 8) return false;
         Memory* mem = board->GetMem();
         if (GetU32(&p[0]) != mem->cartridge_crc_) return false;
         if (GetU32(&p[4]) != (unsigned int)mem->cartridge_list_.size()) return false;
      }
      else if (id == kChunkExtendedRam)
      {
         if (length < 4) return false;
         unsigned int machine_available = 0;
         for (int page = 0; page < 8; ++page)
            if (board->GetMem()->extended_ram_available_[page])
               machine_available |= (1u << page);
         if (GetU32(&p[0]) != machine_available) return false;
      }

      at += length;
   }
   return true;
}

bool MachineState::Save(EmulatorEngine* machine, std::vector<unsigned char>& out)
{
   if (machine == nullptr) return false;

   std::vector<unsigned char> sna;
   if (!machine->SaveSnapshotNow(sna) || sna.empty())
      return false;

   out.clear();
   out.insert(out.end(), kMagic, kMagic + 4);
   PutU16(out, kVersion);
   PutU16(out, 0);
   PutU32(out, (unsigned int)sna.size());
   out.insert(out.end(), sna.begin(), sna.end());

   WriteScheduler(machine->GetMotherboard(), out);
   WriteZ80(machine->GetMotherboard(), out);
   WriteGateArray(machine->GetMotherboard(), out);
   WritePsg(machine->GetMotherboard(), out);
   WriteFdc(machine->GetMotherboard(), out);
   WriteDrives(machine->GetMotherboard(), out);
   WriteCrtc(machine->GetMotherboard(), out);
   WriteTape(machine->GetMotherboard(), out);
   WritePpi(machine->GetMotherboard(), out);
   WriteDma(machine->GetMotherboard(), out);
   WritePlayCity(machine->GetMotherboard(), out);
   WriteExtendedRam(machine->GetMotherboard(), out);
   WriteCartridge(machine->GetMotherboard(), out);

   return true;
}

bool MachineState::Load(EmulatorEngine* machine, const unsigned char* buffer, size_t size)
{
   if (machine == nullptr || buffer == nullptr) return false;
   if (size < kHeaderSize) return false;
   if (memcmp(buffer, kMagic, 4) != 0) return false;
   if (GetU16(&buffer[4]) != kVersion) return false;

   const unsigned int sna_size = GetU32(&buffer[8]);
   if (sna_size == 0 || kHeaderSize + sna_size > size) return false;

   // Check what the chunks say about this machine before touching it, so a
   // state meant for another machine is refused instead of half applied.
   if (!DescribesThisMachine(machine->GetMotherboard(), buffer, size,
                             kHeaderSize + sna_size))
      return false;

   // The .SNA goes in first: it resets and repopulates the components, so
   // anything the chunks restore has to be applied after it.
   if (!machine->LoadSnapshotNow(&buffer[kHeaderSize], sna_size))
      return false;

   size_t at = kHeaderSize + sna_size;
   while (at + 8 <= size)
   {
      const unsigned int id = GetU32(&buffer[at]);
      const unsigned int length = GetU32(&buffer[at + 4]);
      at += 8;
      if (at + length > size) return false;

      if (id == kChunkScheduler)
      {
         if (!ReadScheduler(machine->GetMotherboard(), &buffer[at], length))
            return false;
      }
      else if (id == kChunkZ80)
      {
         if (!ReadZ80(machine->GetMotherboard(), &buffer[at], length))
            return false;
      }
      else if (id == kChunkGateArray)
      {
         if (!ReadGateArray(machine->GetMotherboard(), &buffer[at], length))
            return false;
      }
      else if (id == kChunkPsg)
      {
         if (!ReadPsg(machine->GetMotherboard(), &buffer[at], length))
            return false;
      }
      else if (id == kChunkFdc)
      {
         if (!ReadFdc(machine->GetMotherboard(), &buffer[at], length))
            return false;
      }
      else if (id == kChunkDrives)
      {
         if (!ReadDrives(machine->GetMotherboard(), &buffer[at], length))
            return false;
      }
      else if (id == kChunkCrtc)
      {
         if (!ReadCrtc(machine->GetMotherboard(), &buffer[at], length))
            return false;
      }
      else if (id == kChunkTape)
      {
         if (!ReadTape(machine->GetMotherboard(), &buffer[at], length))
            return false;
      }
      else if (id == kChunkPpi)
      {
         if (!ReadPpi(machine->GetMotherboard(), &buffer[at], length))
            return false;
      }
      else if (id == kChunkDma)
      {
         if (!ReadDma(machine->GetMotherboard(), &buffer[at], length))
            return false;
      }
      else if (id == kChunkPlayCity)
      {
         if (!ReadPlayCity(machine->GetMotherboard(), &buffer[at], length))
            return false;
      }
      else if (id == kChunkExtendedRam)
      {
         if (!ReadExtendedRam(machine->GetMotherboard(), &buffer[at], length))
            return false;
      }
      // Unknown chunks are skipped, so a state from a newer build still loads.

      at += length;
   }

   return true;
}
