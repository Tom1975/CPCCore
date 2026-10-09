#pragma once

// Full machine harness for the tests written from "The Amstrad CPC CRTC Compendium" v1.11
// (Logon System, CC BY-NC-ND 4.0) : a 6128 run one 4 MHz cycle at a time (Motherboard::DebugNew),
// for the behaviours that involve several chips (Z80A, GATE ARRAY, CRTC, PPI).
//
// The lower ROM is disconnected by the test programs (RMR), so the RAM at &0000-&3FFF holds the
// IM 1 vector (&38). Programs are loaded at kProgram.

#include "Machine.h"
#include "Display.h"
#include "TestUtils.h"

#include <functional>
#include <memory>
#include <vector>

namespace compendium
{
const unsigned short kProgram = 0x4000;

class Machine
{
public:
   explicit Machine(CRTC::TypeCRTC type, const char* config = "6128")
   {
      display_.Init(false);
      display_.Show(false);
      engine_.reset(new EmulatorEngine());
      engine_->SetDirectories(&directories_);
      engine_->SetConfigurationManager(&configuration_);
      engine_->SetLog(NULL);
      engine_->Init(&display_, &sound_);
      engine_->GetMem()->Initialisation();
      engine_->SetFixedSpeed(true);
      engine_->LoadConfiguration(config, "./TestConf.ini");
      engine_->Reinit();
      for (int i = 0; i < 50; i++) engine_->RunTimeSlice();
      // The CRTC type is changed after the boot : R3 is written again so that the sync widths the
      // boot derived for the configured type (VSYNC length from R3h) follow the new one.
      CRTC& crtc = *engine_->GetCRTC();
      if (crtc.type_crtc_ != type)
      {
         crtc.DefinirTypeCRTC(type);
         const unsigned char selected = crtc.adddress_register_;
         crtc.Out(0xBC00, 3);
         crtc.Out(0xBD00, crtc.registers_list_[3]);
         crtc.Out(0xBC00, selected);
      }
   }

   EmulatorEngine& Engine() { return *engine_; }
   CRTC& Crtc() { return *engine_->GetCRTC(); }
   GateArray& Ga() { return *engine_->GetVGA(); }
   Z80& Cpu() { return *engine_->GetProc(); }
   CSig& Sig() { return *engine_->GetSig(); }
   unsigned char* Ram() { return engine_->GetMotherboard()->GetRamBuffer(); }

   void Load(unsigned short address, const std::vector<unsigned char>& bytes)
   {
      for (size_t i = 0; i < bytes.size(); i++) Ram()[address + i] = bytes[i];
   }

   // Loads 'program' at kProgram and makes the Z80 fetch it next.
   void Run(const std::vector<unsigned char>& program)
   {
      Load(kProgram, program);
      Cpu().PrepareForFetch(kProgram);
   }

   // One 4 MHz cycle. Also tracks the microsecond boundaries (the CRTC moves C0 on the last cycle of
   // each microsecond : phase 3) and, when enabled, the 16 pixel blocks drawn by the GATE ARRAY.
   void Cycle()
   {
      const unsigned char hcc = Crtc().hcc_;
      engine_->GetMotherboard()->DebugNew(1);
      ++cycles_;
      if (Crtc().hcc_ != hcc) c0_change_cycle_ = cycles_;
      if (Phase() == 0)
      {
         c0_of_microsecond_ = Crtc().hcc_;
         line_of_microsecond_ = Crtc().vcc_ * (Crtc().registers_list_[9] + 1) + Crtc().vlc_;
      }
      if (record_blocks_ && Ga().last_block_ != block_pending_.pixels_at)
      {
         // The previous block is final (FinalizeBlock applied the HSYNC / VSYNC black).
         if (block_pending_.pixels_at != nullptr)
         {
            Block b = block_pending_;
            for (int i = 0; i < 16; ++i) b.pixels[i] = b.pixels_at[i];
            b.pixels_at = nullptr;
            blocks_.push_back(b);
         }
         // Drawn on the last cycle of a microsecond, with the CRTC moving to the next C0 : the
         // block is displayed during the microsecond of the new C0.
         block_pending_.pixels_at = Ga().last_block_;
         block_pending_.line = Crtc().vcc_ * (Crtc().registers_list_[9] + 1) + Crtc().vlc_;
         block_pending_.c0 = Crtc().hcc_;
      }
   }

   // Phase of the last cycle in its microsecond (0 to 3, 3 : the CRTC moved C0).
   int Phase() const { return (int)((cycles_ - c0_change_cycle_ + 3) % 4); }

   // Runs to the fetch of the instruction at 'address' and returns the C0 of the microsecond it
   // starts on : the emulated Z80 may fetch before the boundary and insert its WAIT cycles in the
   // fetch, the instruction then starts on the next microsecond. 'line' : the line of the frame.
   int InstructionStart(unsigned short address, int* line)
   {
      if (CyclesUntil([&]() { return Cpu().pc_ == address + 1; }, 4 * 20000) < 0) return -1;
      if (Phase() != 0) CyclesUntil([&]() { return Phase() == 0; }, 4);
      if (line != nullptr) *line = line_of_microsecond_;
      return c0_of_microsecond_;
   }

   // A block of 16 Mode 2 pixels drawn by the GATE ARRAY during the microsecond C0 of 'line' (it
   // displays the character C0 - 1).
   struct Block
   {
      int line = -1;
      int c0 = -1;
      int pixels[16] = {};
      int* pixels_at = nullptr;
   };
   void RecordBlocks(bool on)
   {
      if (on && !record_blocks_) { blocks_.clear(); block_pending_ = Block(); }
      record_blocks_ = on;
   }
   const std::vector<Block>& Blocks() const { return blocks_; }
   const Block* FindBlock(int line, int c0) const
   {
      for (const Block& b : blocks_)
         if (b.line == line && b.c0 == c0) return &b;
      return nullptr;
   }

   // Cycles until stop() is true ; returns the number of cycles, or -1 after 'cap' cycles.
   int CyclesUntil(const std::function<bool()>& stop, int cap)
   {
      for (int i = 0; i < cap; ++i)
      {
         Cycle();
         if (stop()) return i + 1;
      }
      return -1;
   }

   // True during the M1 cycle of an interrupt acknowledge (the interrupted code stops there).
   bool InInterruptAcknowledge() { return Cpu().machine_cycle_ == Z80::M_M1_INT; }

   unsigned long long Cycles() const { return cycles_; }

private:
   DirectoriesImp directories_;
   ConfigurationManager configuration_;
   CDisplay display_;
   SoundFactory sound_;
   std::unique_ptr<EmulatorEngine> engine_;
   unsigned long long cycles_ = 0;
   unsigned long long c0_change_cycle_ = 0;
   int c0_of_microsecond_ = -1;
   int line_of_microsecond_ = -1;
   bool record_blocks_ = false;
   Block block_pending_;
   std::vector<Block> blocks_;
};

// Z80 encodings used by the test programs.
namespace z80
{
inline void Append(std::vector<unsigned char>& to, const std::vector<unsigned char>& bytes)
{
   to.insert(to.end(), bytes.begin(), bytes.end());
}
// LD BC,&7Fxx : OUT (C),C -- GATE ARRAY write.
inline std::vector<unsigned char> OutGateArray(unsigned char value)
{
   return { 0x01, value, 0x7F, 0xED, 0x49 };
}
// CRTC register write : LD BC,&BCrr : OUT (C),C : LD BC,&BDvv : OUT (C),C.
inline std::vector<unsigned char> WriteCrtc(unsigned char reg, unsigned char value)
{
   return { 0x01, reg, 0xBC, 0xED, 0x49, 0x01, value, 0xBD, 0xED, 0x49 };
}
// LD BC,port : LD A,value : OUT (C),A.
inline std::vector<unsigned char> Out(unsigned short port, unsigned char value)
{
   return { 0x01, (unsigned char)(port & 0xFF), (unsigned char)(port >> 8), 0x3E, value, 0xED, 0x79 };
}
// LD BC,port : IN A,(C) : LD (to),A.
inline std::vector<unsigned char> In(unsigned short port, unsigned short to)
{
   return { 0x01, (unsigned char)(port & 0xFF), (unsigned char)(port >> 8), 0xED, 0x78,
            0x32, (unsigned char)(to & 0xFF), (unsigned char)(to >> 8) };
}
const unsigned char kNop = 0x00;
const unsigned char kDi = 0xF3;
const unsigned char kEi = 0xFB;
const std::vector<unsigned char> kIm1 = { 0xED, 0x56 };
const std::vector<unsigned char> kIm2 = { 0xED, 0x5E };
const std::vector<unsigned char> kJrSelf = { 0x18, 0xFE };
// RMR : mode 1, lower and upper ROM disconnected (bit 4 : R52 reset).
const unsigned char kRmrRamOnly = 0x8D;
const unsigned char kRmrRamOnlyResetR52 = 0x9D;
}
}
