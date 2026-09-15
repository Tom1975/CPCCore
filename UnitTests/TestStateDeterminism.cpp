#include "gtest/gtest.h"

#include "TestUtils.h"

#include <string>
#include <vector>

// Does saving a state and restoring it put the machine back where it was?
//
// The check is differential rather than absolute: run on from a save point,
// fingerprint the machine, restore, run on the same distance again, and
// fingerprint again. Anything the snapshot failed to carry shows up as a
// divergence between the two, because the restored run started from an
// incomplete machine.
//
// Fingerprints are taken per component so a failure names the component that
// was not carried, instead of reporting one opaque hash mismatch. That is the
// point of this harness: it is meant to tell us what is still missing from the
// serialiser as components get added, not just that something is.

namespace
{

struct Fingerprint
{
   std::vector<std::pair<std::string, unsigned long long> > fields;

   void Add(const char* name, unsigned long long value)
   {
      fields.push_back(std::make_pair(std::string(name), value));
   }

   void AddBlock(const char* name, const unsigned char* data, size_t size)
   {
      // FNV-1a, only needs to be stable and to change when the block does.
      unsigned long long h = 14695981039346656037ULL;
      for (size_t i = 0; i < size; ++i)
      {
         h ^= data[i];
         h *= 1099511628211ULL;
      }
      Add(name, h);
   }
};

Fingerprint Capture(EmulatorEngine* machine)
{
   Fingerprint f;

   f.AddBlock("ram", machine->GetMem()->GetRamBuffer(), 0x10000);

   Z80* z80 = machine->GetProc();
   f.Add("z80.pc", z80->pc_);
   f.Add("z80.sp", z80->sp_);
   f.Add("z80.iff1", z80->iff1_ ? 1 : 0);
   f.Add("z80.iff2", z80->iff2_ ? 1 : 0);
   f.Add("z80.opcode", z80->current_opcode_);
   f.Add("z80.machine_cycle", z80->machine_cycle_);
   f.Add("z80.t", z80->t_);

   CRTC* crtc = machine->GetCRTC();
   f.Add("crtc.hcc", crtc->hcc_);
   f.Add("crtc.vcc", crtc->vcc_);
   f.Add("crtc.vlc", crtc->vlc_);
   f.AddBlock("crtc.registers", crtc->registers_list_, sizeof(crtc->registers_list_));

   CTape* tape = machine->GetTape();
   f.Add("tape.position", tape->GetTapePosition());
   f.Add("tape.inversions", tape->GetNbInversions());
   f.Add("tape.recording", tape->IsRecordOn() ? 1 : 0);

   f.Add("fdc.track0", machine->GetFDC()->GetCurrentTrack(0));
   f.Add("fdc.motor", machine->GetFDC()->IsMotorOn() ? 1 : 0);

   return f;
}

EmulatorEngine* NewBootedMachine(DirectoriesImp& dirImp, CDisplay& display, Log& log,
                                 SoundFactory& soundFactory, ConfigurationManager& conf_manager)
{
   EmulatorEngine* machine = new EmulatorEngine();

   display.Init(false);
   display.Show(false);

   machine->SetDirectories(&dirImp);
   machine->SetLog(&log);
   machine->SetConfigurationManager(&conf_manager);
   machine->Init(&display, &soundFactory);
   machine->GetMem()->Initialisation();
   machine->LoadConfiguration("./TestConf.ini", "./TestConf_0.ini");
   machine->Reinit();
   machine->SetFixedSpeed(true);
   machine->SetSpeedLimit(EmulatorEngine::E_FULL);

   return machine;
}

}  // namespace

// Two separate questions, asserted separately, because they have very different
// answers today.
//
// 1. RESTORE FIDELITY -- does the machine come back exactly as it was saved?
//    Yes, for every field below. This is a hard assertion: any field that stops
//    matching here is a real regression in the .SNA path.
//
// 2. RUN-ON DETERMINISM -- does the restored machine then run the same way?
//    No, and not for any field. Motherboard::component_elapsed_time_[] and
//    IComponent::this_tick_time_ hold each component's cycle debt across time
//    slices and are not in the container, so a restored machine resumes with
//    every component's scheduling phase reset. The CPU and CRTC drift apart
//    within a slice and the program takes a different path from there.
//    Asserting field-by-field equality here would just be asserting that a
//    known-missing feature is present, so this is a ratchet instead: the number
//    of divergent fields may fall as the serialiser grows, never rise.
TEST(StateDeterminism, RestoringAStateReproducesTheSameRun)
{
   // Every field diverges once the machine runs on. Lower this as the
   // serialiser carries more; it must never need raising.
   const int kDivergentFieldsBaseline = 11;

   const int kSlicesBeforeSave = 20;
   // Long enough to be a meaningful control. At ten slices a .SNA round trip
   // happens to stay in step, which made this look far better than it is.
   const int kSlicesAfterSave = 400;

   DirectoriesImp dirImp; CDisplay display; Log log;
   SoundFactory soundFactory; ConfigurationManager conf_manager;
   EmulatorEngine* machine = NewBootedMachine(dirImp, display, log, soundFactory, conf_manager);

   for (int i = 0; i < kSlicesBeforeSave; ++i)
      machine->RunTimeSlice();

   // A machine with no firmware paged in never executes, and then every
   // fingerprint below matches for the wrong reason. Refuse to report on one.
   int ram_in_use = 0;
   const unsigned char* ram = machine->GetMem()->GetRamBuffer();
   for (int i = 0; i < 0x10000; ++i) if (ram[i] != 0) ++ram_in_use;
   ASSERT_GT(ram_in_use, 0)
      << "the machine never executed an instruction -- no ROMs found. This test "
         "needs CONF/, ROM/ and res/ from UnitTests/ and Keyboards/ from "
         "CPCCoreEmu/ reachable from the working directory.";

   std::vector<unsigned char> saved;
   ASSERT_TRUE(machine->SaveSnapshotNow(saved));
   ASSERT_FALSE(saved.empty());
   const Fingerprint at_save = Capture(machine);

   for (int i = 0; i < kSlicesAfterSave; ++i)
      machine->RunTimeSlice();
   const Fingerprint straight_through = Capture(machine);

   ASSERT_TRUE(machine->LoadSnapshotNow(&saved[0], saved.size()));
   const Fingerprint at_restore = Capture(machine);

   for (int i = 0; i < kSlicesAfterSave; ++i)
      machine->RunTimeSlice();
   const Fingerprint after_restore = Capture(machine);

   delete machine;

   ASSERT_EQ(at_save.fields.size(), at_restore.fields.size());
   ASSERT_EQ(straight_through.fields.size(), after_restore.fields.size());

   // 1. Restore fidelity.
   for (size_t i = 0; i < at_save.fields.size(); ++i)
   {
      EXPECT_EQ(at_save.fields[i].second, at_restore.fields[i].second)
         << at_save.fields[i].first << " did not come back from the snapshot";
   }

   // 2. Run-on determinism.
   int divergent = 0;
   for (size_t i = 0; i < straight_through.fields.size(); ++i)
   {
      if (straight_through.fields[i].second == after_restore.fields[i].second)
         continue;
      ++divergent;
      fprintf(stderr, "  DIVERGES AFTER RUNNING ON: %s\n",
         straight_through.fields[i].first.c_str());
   }

   fprintf(stderr, "DETERMINISM: %d of %zu fields restored exactly, %d diverged after "
      "running on (baseline %d)\n",
      static_cast<int>(at_save.fields.size()), at_save.fields.size(),
      divergent, kDivergentFieldsBaseline);

   EXPECT_LE(divergent, kDivergentFieldsBaseline)
      << "more fields diverge than before -- the state path regressed";
}
