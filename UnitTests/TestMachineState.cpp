#include "gtest/gtest.h"

#include "TestUtils.h"
#include "MachineState.h"

#include <cstdio>
#include <cstdlib>
#ifndef _WIN32
#include <unistd.h>
#else
#include <io.h>
#include <process.h>
#define getpid _getpid
#endif
#include <string>
#include <vector>

// Does a full engine state put the machine back where it was, well enough that
// it then runs the same way?
//
// StateDeterminism asks that of a .SNA and answers no: every fingerprinted
// field diverges once the machine runs on, because the scheduler's per-component
// cycle debt has no field in the container. This asks the same question of
// MachineState, which adds that debt on top of the .SNA, and asserts the answer
// is yes.
//
// The two tests share their method deliberately. The .SNA one is the control:
// if it ever stops diverging, this one has stopped proving anything.

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
   f.Add("fdc.main_status", machine->GetFDC()->GetMainStatus());

   // Memory paging: which banks and which upper ROM the CPU currently sees.
   f.Add("mem.selected_rom", machine->GetMem()->GetSelectedRom());
   f.AddBlock("mem.bank0", machine->GetMem()->GetRamBuffer(), 0x4000);

   // Gate array: what actually drives the interrupt the CPU will take next.
   GateArray* ga = machine->GetVGA();
   f.Add("ga.pen", ga->pen_r_);
   f.Add("ga.screen_mode", ga->buffered_screen_mode_);

   // PSG: the audible state, and the flag that selects keyboard vs latch on
   // port A -- get that wrong and the firmware reads different keys.
   Ay8912* psg = machine->GetPSG();
   f.Add("psg.selected", psg->GetRegisterAdress());

   return f;
}

EmulatorEngine* NewBootedMachine(DirectoriesImp& dirImp, CDisplay& display, Log& log,
                                 SoundFactory& soundFactory, ConfigurationManager& conf_manager,
                                 const char* section)
{
   EmulatorEngine* machine = new EmulatorEngine();

   display.Init(false);
   display.Show(false);

   machine->SetDirectories(&dirImp);
   machine->SetLog(&log);
   machine->SetConfigurationManager(&conf_manager);
   machine->Init(&display, &soundFactory);
   machine->GetMem()->Initialisation();
   machine->LoadConfiguration(section, "./TestConf.ini");
   machine->Reinit();
   machine->SetFixedSpeed(true);
   machine->SetSpeedLimit(EmulatorEngine::E_FULL);

   return machine;
}

}  // namespace

namespace
{

// Paste() queues keystrokes on EmulatorEngine, outside the machine, so a save
// state neither does nor should carry them. Anything still queued when the state
// is taken would be typed into the straight-through run and not into the
// restored one, which looks exactly like a state that failed to carry something.
void DrainTypist(EmulatorEngine* machine)
{
   for (int i = 0; i < 2000 && !machine->PasteBufferIsEmpty(); ++i)
      machine->RunTimeSlice(false);
   ASSERT_TRUE(machine->PasteBufferIsEmpty())
      << "keystrokes were still queued, so the two runs would not see the same input";
}

// Long enough that a run which only looks identical cannot stay that way: the
// firmware sweeps the keyboard, the gate array raises interrupts and the CRTC
// completes frames many times over.
const int kSlicesAfterSave = 400;

void CheckOneMachine(const char* section, int slices_before_save,
                    const char* disk = nullptr, const char* format = "",
                    const char* tape = nullptr, int tolerated = 0,
                    const char* run_command = nullptr)
{
   DirectoriesImp dirImp; CDisplay display; Log log;
   SoundFactory soundFactory; ConfigurationManager conf_manager;
   EmulatorEngine* machine =
      NewBootedMachine(dirImp, display, log, soundFactory, conf_manager, section);

   // A tape only carries state worth restoring while it is running past the
   // head. Inserted and stopped, it is as inert as an empty drive.
   if (tape != nullptr)
   {
      ASSERT_EQ(0, machine->LoadTape(tape))
         << "the tape fixture did not load, so this case proves nothing: " << tape;

      // The sequence the tape dump tests use: the firmware, not the test, is
      // what starts the motor.
      for (int i = 0; i < 100; ++i)
         machine->RunTimeSlice();
      machine->Paste("RUN\"\r");
      for (int i = 0; i < 20; ++i)
         machine->RunTimeSlice(false);
      machine->Paste(" ");

      unsigned int position = 0;
      for (int i = 0; i < 3000 && position == 0; ++i)
      {
         machine->RunTimeSlice(false);
         position = machine->GetTape()->GetTapePosition();
      }
      ASSERT_GT(position, 0u)
         << "the tape never advanced past the head, so this case is still idle";
      DrainTypist(machine);
   }

   // With a disk inserted and the drive seeking, the FDC and the disk image
   // carry state that an idle BASIC prompt never exercises.
   if (disk != nullptr)
   {
      ASSERT_EQ(0, machine->LoadDisk(disk, 0, false))
         << "the disk fixture did not load, so this case proves nothing: " << disk;

      machine->Paste(run_command != nullptr ? run_command : "cat\r");

      // Run until the drive is actually turning and stop there. The motor spins
      // down again once the access finishes, so waiting a fixed number of slices
      // and then looking is a coin toss.
      bool spinning = false;
      for (int i = 0; i < 2000 && !spinning; ++i)
      {
         machine->RunTimeSlice(false);
         spinning = machine->GetFDC()->IsMotorOn();
      }
      ASSERT_TRUE(spinning)
         << "the drive never started, so the FDC state under test is still idle";
      DrainTypist(machine);

      // A loaded program needs time to start doing whatever it does; CAT does
      // not, which is why the plain disk cases stop at the motor.
      if (run_command != nullptr)
         for (int i = 0; i < 600; ++i)
            machine->RunTimeSlice(false);
   }

   const int kSlicesBeforeSave = slices_before_save;

   for (int i = 0; i < kSlicesBeforeSave; ++i)
      machine->RunTimeSlice();

   int ram_in_use = 0;
   const unsigned char* ram = machine->GetMem()->GetRamBuffer();
   for (int i = 0; i < 0x10000; ++i) if (ram[i] != 0) ++ram_in_use;
   ASSERT_GT(ram_in_use, 0)
      << "the machine never executed an instruction -- no ROMs found. This test "
         "needs CONF/, ROM/ and res/ from UnitTests/ and Keyboards/ from "
         "CPCCoreEmu/ reachable from the working directory.";

   std::vector<unsigned char> state;
   ASSERT_TRUE(MachineState::Save(machine, state));
   ASSERT_FALSE(state.empty());
   const Fingerprint at_save = Capture(machine);

   // rand() is global to the process, not part of the machine, and the disk
   // layer uses it for weak sectors and drive-speed jitter. Both runs have to
   // start from the same point or they diverge for a reason no save state could
   // ever fix.
   srand(0x5AFE5EED);
   for (int i = 0; i < kSlicesAfterSave; ++i)
      machine->RunTimeSlice();
   const Fingerprint straight_through = Capture(machine);

   ASSERT_TRUE(MachineState::Load(machine, &state[0], state.size()));
   const Fingerprint at_restore = Capture(machine);
   srand(0x5AFE5EED);

   // Split the two failure modes: a field that is already wrong the instant the
   // state comes back was not carried, as opposed to one that only drifts once
   // the machine runs on.
   for (size_t i = 0; i < at_save.fields.size(); ++i)
   {
      if (at_save.fields[i].second != at_restore.fields[i].second)
         fprintf(stderr, "  NOT CARRIED BY RESTORE: %s\n", at_save.fields[i].first.c_str());
   }

   for (int i = 0; i < kSlicesAfterSave; ++i)
      machine->RunTimeSlice();
   const Fingerprint after_restore = Capture(machine);

   delete machine;

   ASSERT_EQ(straight_through.fields.size(), after_restore.fields.size());

   int divergent = 0;
   for (size_t i = 0; i < straight_through.fields.size(); ++i)
   {
      if (straight_through.fields[i].second == after_restore.fields[i].second)
         continue;
      ++divergent;
      fprintf(stderr, "  STILL DIVERGES: %s\n", straight_through.fields[i].first.c_str());
   }

   fprintf(stderr, "MACHINE STATE %-9s save@%-5d %-6s %d of %zu fields diverged\n",
      section, slices_before_save, disk ? format : (tape ? "tape" : "idle"),
      divergent, straight_through.fields.size());

   EXPECT_LE(divergent, tolerated)
      << section << " save@" << slices_before_save
      << ": the state did not carry everything the machine needs to resume";
}

}  // namespace

TEST(MachineStateTest, RestoringAStateReproducesTheSameRun)
{
   const char* sections[] = { "6128", "464", "6128PLUS" };
   const int save_points[] = { 20, 137, 400 };

   for (size_t s = 0; s < sizeof(sections) / sizeof(sections[0]); ++s)
      for (size_t p = 0; p < sizeof(save_points) / sizeof(save_points[0]); ++p)
         CheckOneMachine(sections[s], save_points[p]);
}

// The FDC and the disk only carry state worth restoring while the drive is
// actually turning, which a BASIC prompt never makes happen.
//
// Once per container format, because they do not decode alike: a plain DSK has
// one fixed-size image of each track, an EDSK varies the size per track, and
// IPF, HFE and SCP describe the flux itself and can hold several revolutions of
// the same track. The head position that has to survive a round trip means
// something different in each.
TEST(MachineStateTest, RestoringAStateReproducesTheSameRunWithADiskSpinning)
{
   struct Image { const char* path; const char* format; };
   const Image images[] = {
      { "./res/FDC/fdctest.dsk",                                            "DSK"  },
      { "./res/DSK/Ace Of Aces (UK) (1985) [Original] (Weak Sectors).dsk",  "EDSK" },
      { "./res/After Burner (UK) (1988) [Activision SEGA] (Pre-release).ipf","IPF"  },
      { "./res/30YMD double sides 1 and 2.hfe",                             "HFE"  },
      { "./res/The Demo [A].scp",                                           "SCP"  },
   };

   for (size_t i = 0; i < sizeof(images) / sizeof(images[0]); ++i)
   {
      SCOPED_TRACE(images[i].format);
      CheckOneMachine("6128", 20, images[i].path, images[i].format);
      CheckOneMachine("6128", 137, images[i].path, images[i].format);
   }
}

TEST(MachineStateTest, RejectsSomethingThatIsNotAState)
{
   DirectoriesImp dirImp; CDisplay display; Log log;
   SoundFactory soundFactory; ConfigurationManager conf_manager;
   EmulatorEngine* machine =
      NewBootedMachine(dirImp, display, log, soundFactory, conf_manager, "6128");

   for (int i = 0; i < 20; ++i)
      machine->RunTimeSlice();

   std::vector<unsigned char> state;
   ASSERT_TRUE(MachineState::Save(machine, state));

   std::vector<unsigned char> bad = state;
   bad[0] = 'X';
   EXPECT_FALSE(MachineState::Load(machine, &bad[0], bad.size()))
      << "a buffer without the magic must be refused";

   bad = state;
   bad[4] = 0xFE; bad[5] = 0xFF;
   EXPECT_FALSE(MachineState::Load(machine, &bad[0], bad.size()))
      << "an unknown version must be refused rather than misread";

   EXPECT_FALSE(MachineState::Load(machine, &state[0], state.size() / 2))
      << "a truncated state must be refused";

   // A whole .SNA is not a MachineState, however valid it is on its own.
   std::vector<unsigned char> sna;
   ASSERT_TRUE(machine->SaveSnapshotNow(sna));
   EXPECT_FALSE(MachineState::Load(machine, &sna[0], sna.size()));

   delete machine;
}

// The other half of keeping media out of the state: a head position only means
// something against the disk it was recorded on. Swap the disk and the state
// must be refused, not applied to whatever happens to be in the drive.
TEST(MachineStateTest, RefusesAStateTakenWithADifferentDisk)
{
   DirectoriesImp dirImp; CDisplay display; Log log;
   SoundFactory soundFactory; ConfigurationManager conf_manager;
   EmulatorEngine* machine =
      NewBootedMachine(dirImp, display, log, soundFactory, conf_manager, "6128");

   ASSERT_EQ(0, machine->LoadDisk("./res/FDC/fdctest.dsk", 0, false));
   for (int i = 0; i < 100; ++i)
      machine->RunTimeSlice(false);

   std::vector<unsigned char> state;
   ASSERT_TRUE(MachineState::Save(machine, state));

   // Same drive, different geometry.
   ASSERT_EQ(0, machine->LoadDisk(
      "./res/After Burner (UK) (1988) [Activision SEGA] (Pre-release).ipf", 0, false));

   const bool loaded = MachineState::Load(machine, &state[0], state.size());

   const unsigned int tracks_now =
      machine->GetFDC()->GetCurrentTrack(0) >= 0 ? 1u : 0u;
   (void)tracks_now;

   delete machine;

   EXPECT_FALSE(loaded)
      << "a state recorded against another disk was applied instead of refused";
}

// An empty drive is a geometry too: a state taken with no disk must not be
// loaded into a machine that now has one.
// A cartridge is media too, and until now it was the one kind a state would
// accept blindly: the ROM stays out of the state, so nothing distinguished a
// state taken with one cartridge from another. The CART chunk carries the
// CRC32 Memory computed when the .cpr was read, and a load checks it.
TEST(MachineStateTest, RefusesAStateTakenWithADifferentCartridge)
{
   DirectoriesImp dirImp; CDisplay display; Log log;
   SoundFactory soundFactory; ConfigurationManager conf_manager;
   EmulatorEngine* machine =
      NewBootedMachine(dirImp, display, log, soundFactory, conf_manager, "GX4000");

   ASSERT_EQ(0, machine->LoadCpr("./res/CART/Eerie_Forest_(Logon_System_2017).cpr"));
   machine->Reinit();
   for (int i = 0; i < 200; ++i)
      machine->RunTimeSlice();

   std::vector<unsigned char> state;
   ASSERT_TRUE(MachineState::Save(machine, state));

   // The state must come back into the machine it was taken from...
   ASSERT_TRUE(MachineState::Load(machine, &state[0], state.size()))
      << "the guard refuses the very cartridge the state was taken with";

   // ...and must not come back into a machine holding a different one.
   ASSERT_EQ(0, machine->LoadCpr("./res/plus/sscrtest.cpr"));
   machine->Reinit();
   const bool loaded = MachineState::Load(machine, &state[0], state.size());

   delete machine;

   EXPECT_FALSE(loaded)
      << "a state recorded against another cartridge was applied instead of refused";
}

TEST(MachineStateTest, RefusesAStateTakenWithNoDiskWhenOneIsInserted)
{
   DirectoriesImp dirImp; CDisplay display; Log log;
   SoundFactory soundFactory; ConfigurationManager conf_manager;
   EmulatorEngine* machine =
      NewBootedMachine(dirImp, display, log, soundFactory, conf_manager, "6128");

   for (int i = 0; i < 100; ++i)
      machine->RunTimeSlice(false);

   std::vector<unsigned char> state;
   ASSERT_TRUE(MachineState::Save(machine, state));

   ASSERT_EQ(0, machine->LoadDisk("./res/FDC/fdctest.dsk", 0, false));

   const bool loaded = MachineState::Load(machine, &state[0], state.size());
   delete machine;

   EXPECT_FALSE(loaded)
      << "a state taken with an empty drive was applied to a loaded one";
}

// Every test above restores into the machine that saved, so a chunk holding a
// raw pointer would still work: the object it points at is exactly where it was.
// A savestate has to survive being loaded into a machine built from scratch,
// whose components sit at different addresses. This is the cheap half of that
// (same process, fresh instance); the expensive half is another process
// entirely, which only differs by also reshuffling the address space.
TEST(MachineStateTest, LoadsIntoAMachineThatDidNotSaveIt)
{
   const int kSlicesAfter = 400;

   std::vector<unsigned char> state;
   Fingerprint expected;

   {
      DirectoriesImp dirImp; CDisplay display; Log log;
      SoundFactory soundFactory; ConfigurationManager conf_manager;
      EmulatorEngine* saver =
         NewBootedMachine(dirImp, display, log, soundFactory, conf_manager, "6128");

      for (int i = 0; i < 137; ++i)
         saver->RunTimeSlice();

      ASSERT_TRUE(MachineState::Save(saver, state));

      for (int i = 0; i < kSlicesAfter; ++i)
         saver->RunTimeSlice();
      expected = Capture(saver);

      delete saver;
   }

   // A second machine, deliberately run a different distance so that nothing it
   // already holds could make the comparison pass by accident.
   DirectoriesImp dirImp; CDisplay display; Log log;
   SoundFactory soundFactory; ConfigurationManager conf_manager;
   EmulatorEngine* loader =
      NewBootedMachine(dirImp, display, log, soundFactory, conf_manager, "6128");

   for (int i = 0; i < 61; ++i)
      loader->RunTimeSlice();

   const Fingerprint before_load = Capture(loader);
   ASSERT_TRUE(MachineState::Load(loader, &state[0], state.size()));

   for (int i = 0; i < kSlicesAfter; ++i)
      loader->RunTimeSlice();
   const Fingerprint actual = Capture(loader);

   delete loader;

   ASSERT_EQ(expected.fields.size(), actual.fields.size());

   bool differed_before = false;
   for (size_t i = 0; i < before_load.fields.size(); ++i)
      if (before_load.fields[i].second != expected.fields[i].second)
         differed_before = true;
   ASSERT_TRUE(differed_before)
      << "the second machine already matched before loading, so this proves nothing";

   int divergent = 0;
   for (size_t i = 0; i < expected.fields.size(); ++i)
   {
      if (expected.fields[i].second == actual.fields[i].second) continue;
      ++divergent;
      fprintf(stderr, "  DIFFERS IN A FRESH MACHINE: %s\n", expected.fields[i].first.c_str());
   }
   fprintf(stderr, "CROSS INSTANCE: %d of %zu fields diverged\n",
      divergent, expected.fields.size());

   EXPECT_EQ(0, divergent)
      << "the state depends on the machine instance that wrote it";
}

// ---------------------------------------------------------------------------
// A state has to survive the process that wrote it.
//
// Everything above runs in one process, where a chunk that accidentally held a
// host address would still work: the address is still valid. A savestate is
// written, the emulator is closed, and the file is opened again days later in a
// process whose heap and code sit somewhere else entirely. That is the case
// that matters, and the only way to test it is to actually be a second process.
//
// So the test re-runs this binary. The child is the disabled test below, told
// where to write by an argument the parent adds; it saves a state and the
// fingerprint of the run that follows it. The parent then loads that state cold
// and checks it reaches the same fingerprint.
// ---------------------------------------------------------------------------

namespace
{

const char kStateOutFlag[] = "--machine-state-out=";

// A path no other process can be using.
//
// A fixed name in the temp directory is shared by every test process on the
// machine, and a name built from the process id still collides with a stale file
// left by a crashed run that happened to have the same id. Let the OS pick,
// atomically, and delete it when done.
std::string CreateUniqueStatePath()
{
   std::string pattern = testing::TempDir() + "cpccore_state_XXXXXX";
   std::vector<char> buffer(pattern.begin(), pattern.end());
   buffer.push_back('\0');

#ifndef _WIN32
   const int fd = mkstemp(&buffer[0]);
   if (fd < 0) return std::string();
   close(fd);
#else
   if (_mktemp_s(&buffer[0], buffer.size()) != 0) return std::string();
#endif
   return std::string(&buffer[0]);
}

std::string ArgumentValue(const char* prefix)
{
   const std::vector<std::string> argv = testing::internal::GetArgvs();
   const size_t n = strlen(prefix);
   for (size_t i = 0; i < argv.size(); ++i)
      if (argv[i].compare(0, n, prefix) == 0)
         return argv[i].substr(n);
   return std::string();
}

bool WriteWholeFile(const std::string& path, const std::vector<unsigned char>& data)
{
   FILE* f = fopen(path.c_str(), "wb");
   if (f == nullptr) return false;
   const bool ok = data.empty() || fwrite(&data[0], 1, data.size(), f) == data.size();
   fclose(f);
   return ok;
}

bool ReadWholeFile(const std::string& path, std::vector<unsigned char>& data)
{
   FILE* f = fopen(path.c_str(), "rb");
   if (f == nullptr) return false;
   fseek(f, 0, SEEK_END);
   const long size = ftell(f);
   fseek(f, 0, SEEK_SET);
   bool ok = size > 0;
   if (ok)
   {
      data.resize((size_t)size);
      ok = fread(&data[0], 1, (size_t)size, f) == (size_t)size;
   }
   fclose(f);
   return ok;
}

void WriteFingerprint(const std::string& path, const Fingerprint& f)
{
   FILE* out = fopen(path.c_str(), "wb");
   ASSERT_NE(nullptr, out);
   // First line identifies the writer, so the parent can tell its own child's
   // output from a file left by some other test process.
   fprintf(out, "pid %ld\n", (long)getpid());
   for (size_t i = 0; i < f.fields.size(); ++i)
      fprintf(out, "%s %llu\n", f.fields[i].first.c_str(),
              (unsigned long long)f.fields[i].second);
   fclose(out);
}

// The distances the two processes agree on. Kept here so the child and the
// parent cannot drift apart.
const int kCrossProcessSlicesBeforeSave = 137;
const int kCrossProcessSlicesAfterSave = 400;

}  // namespace

// Runs only in the child, which the parent starts with the output path appended.
TEST(MachineStateTest, DISABLED_CrossProcessProducer)
{
   const std::string state_path = ArgumentValue(kStateOutFlag);
   ASSERT_FALSE(state_path.empty())
      << "this test is the child half of CrossProcess and is not meant to be run "
         "on its own";

   DirectoriesImp dirImp; CDisplay display; Log log;
   SoundFactory soundFactory; ConfigurationManager conf_manager;
   EmulatorEngine* machine =
      NewBootedMachine(dirImp, display, log, soundFactory, conf_manager, "6128");

   for (int i = 0; i < kCrossProcessSlicesBeforeSave; ++i)
      machine->RunTimeSlice();

   // Assert in the child, where the message is unambiguous: a state written by a
   // machine that never executed would load fine in the parent and fail the
   // comparison for a reason that has nothing to do with the state format.
   int ram_in_use = 0;
   const unsigned char* ram = machine->GetMem()->GetRamBuffer();
   for (int i = 0; i < 0x10000; ++i) if (ram[i] != 0) ++ram_in_use;
   ASSERT_GT(ram_in_use, 0) << "the child machine never executed; no ROMs found";

   std::vector<unsigned char> state;
   ASSERT_TRUE(MachineState::Save(machine, state));
   ASSERT_TRUE(WriteWholeFile(state_path, state));

   for (int i = 0; i < kCrossProcessSlicesAfterSave; ++i)
      machine->RunTimeSlice();
   WriteFingerprint(state_path + ".fingerprint", Capture(machine));

   delete machine;
}

TEST(MachineStateTest, LoadsAStateWrittenByAnotherProcess)
{
   const std::vector<std::string> argv = testing::internal::GetArgvs();
   ASSERT_FALSE(argv.empty());

   const std::string state_path = CreateUniqueStatePath();
   ASSERT_FALSE(state_path.empty()) << "could not create a temporary file";
   const std::string fingerprint_path = state_path + ".fingerprint";

   std::string command = "\"" + argv[0] + "\""
      + " --gtest_also_run_disabled_tests"
      + " --gtest_filter=MachineStateTest.DISABLED_CrossProcessProducer"
      + " " + kStateOutFlag + "\"" + state_path + "\"";
#ifdef _WIN32
   // cmd.exe drops the outer pair of quotes from a command line that begins
   // with one, which mangles a line that has more than one quoted part -- here
   // the executable and the output path. Wrapping the whole thing in another
   // pair is the documented way round it. Without this the child never runs and
   // system() reports "The filename, directory name, or volume label syntax is
   // incorrect", which looks exactly like an intermittent failure because it
   // only happens on Windows.
   command = "\"" + command + "\"";
#endif
   ASSERT_EQ(0, system(command.c_str()))
      << "the child process failed; command was: " << command;

   std::vector<unsigned char> state;
   ASSERT_TRUE(ReadWholeFile(state_path, state))
      << "the child wrote no state at " << state_path;

   FILE* fp = fopen(fingerprint_path.c_str(), "rb");
   ASSERT_NE(nullptr, fp);
   std::vector<std::pair<std::string, unsigned long long> > expected;
   char name[128];
   unsigned long long value;
   long writer_pid = 0;
   if (fscanf(fp, "pid %ld\n", &writer_pid) != 1)
      writer_pid = 0;
   while (fscanf(fp, "%127s %llu", name, &value) == 2)
      expected.push_back(std::make_pair(std::string(name), value));
   fclose(fp);
   ASSERT_FALSE(expected.empty());
   ASSERT_NE(0, writer_pid)
      << "the fingerprint carries no writer id, so it cannot be attributed to "
         "this test's own child";

   // Cold load, in this process, into a machine that has never seen that state.
   DirectoriesImp dirImp; CDisplay display; Log log;
   SoundFactory soundFactory; ConfigurationManager conf_manager;
   EmulatorEngine* machine =
      NewBootedMachine(dirImp, display, log, soundFactory, conf_manager, "6128");

   // This refused once during development and was never reproduced, in over a
   // hundred targeted runs. Rather than guess at a fix, describe the state fully
   // when it happens: header, and every chunk with its length, so the next
   // occurrence names the culprit instead of restarting the investigation.
   ASSERT_GE(state.size(), 12u);
   if (!MachineState::Load(machine, &state[0], state.size()))
   {
      std::string report;
      char line[256];
      snprintf(line, sizeof(line), "%zu bytes, magic %c%c%c%c, version %u (build writes %u)",
               state.size(), state[0], state[1], state[2], state[3],
               (unsigned)(state[4] | (state[5] << 8)), (unsigned)MachineState::kVersion);
      report = line;

      const unsigned int sna_len = state[8] | (state[9] << 8)
                                 | (state[10] << 16) | ((unsigned int)state[11] << 24);
      snprintf(line, sizeof(line), "\n  .SNA %u bytes", sna_len);
      report += line;

      size_t at = 12 + sna_len;
      while (at + 8 <= state.size())
      {
         const unsigned int id = state[at] | (state[at+1] << 8)
                               | (state[at+2] << 16) | ((unsigned int)state[at+3] << 24);
         const unsigned int len = state[at+4] | (state[at+5] << 8)
                                | (state[at+6] << 16) | ((unsigned int)state[at+7] << 24);
         snprintf(line, sizeof(line), "\n  chunk %c%c%c%c %u bytes",
                  (char)((id >> 24) & 0xFF), (char)((id >> 16) & 0xFF),
                  (char)((id >> 8) & 0xFF), (char)(id & 0xFF), len);
         report += line;
         at += 8 + len;
      }
      if (at != state.size())
      {
         snprintf(line, sizeof(line), "\n  TRAILING: chunk walk ended at %zu of %zu",
                  at, state.size());
         report += line;
      }

      FAIL() << "a state written by another process was refused.\n  " << report
             << "\n  from " << state_path;
   }

   for (int i = 0; i < kCrossProcessSlicesAfterSave; ++i)
      machine->RunTimeSlice();
   const Fingerprint actual = Capture(machine);

   delete machine;
   remove(state_path.c_str());
   remove(fingerprint_path.c_str());

   ASSERT_EQ(expected.size(), actual.fields.size());

   int divergent = 0;
   for (size_t i = 0; i < expected.size(); ++i)
   {
      ASSERT_EQ(expected[i].first, actual.fields[i].first);
      if (expected[i].second == actual.fields[i].second) continue;
      ++divergent;
      fprintf(stderr, "  DIFFERS ACROSS PROCESSES: %s\n", expected[i].first.c_str());
   }
   fprintf(stderr, "CROSS PROCESS: %d of %zu fields diverged\n",
      divergent, expected.size());

   EXPECT_EQ(0, divergent)
      << "the state did not survive being written by another process";
}

// The tape is the other moving medium, and the one the .SNA has nothing at all
// for -- not even the equivalent of the FDC's motor flag and current track.
TEST(MachineStateTest, RestoringAStateReproducesTheSameRunWithATapeRunning)
{
   const char* kTape =
      "./res/Basil The Great Mouse Detective (UK) (1987) [Original] [TAPE].cdt";
   CheckOneMachine("464", 20, nullptr, "", kTape);

   // Saving in the middle of a transfer used to leave three fields apart. The
   // cause was the PPI's tape input level: the bit the firmware's loading loop
   // samples, which the .SNA has no field for, so a restored machine read the
   // wrong bit at the wrong moment and the loop took a different path while
   // every other component stayed in step.
   CheckOneMachine("464", 137, nullptr, "", kTape);
}

// A Plus doing Plus things.
//
// Every other Plus case here sits at a BASIC prompt, where the ASIC, its three
// DMA channels and the sprite hardware are all idle -- and a component that does
// nothing during a test reads as perfectly carried whether it is or not. That
// mistake has been made four times in this file's history. These run the ASIC
// test programs off the disk the Plus.* tests use.
TEST(MachineStateTest, RestoringAStateReproducesTheSameRunOnAPlusUsingTheAsic)
{
   const char* kAsicDisk = "./res/plus/asic.dsk";

   struct Program { const char* command; const char* what; };
   const Program programs[] = {
      { "run\"dmatest\r",   "DMA sound channels" },
      { "run\"asicrast\r", "raster interrupts"  },
      { "run\"hscrl\r",     "hardware scroll"    },
      { "run\"lumasic\r",   "ASIC palette"       },
   };

   for (size_t i = 0; i < sizeof(programs) / sizeof(programs[0]); ++i)
   {
      SCOPED_TRACE(programs[i].what);
      CheckOneMachine("6128PLUSPARADOS", 137, kAsicDisk, programs[i].what,
                      nullptr, 0, programs[i].command);
   }
}

// A cartridge is media, like a disk or a tape: the state carries which bank the
// machine is looking at, never the half-megabyte of ROM behind it.
//
// This runs a real Plus demo from a .cpr rather than sitting at a prompt, for
// the same reason the ASIC cases exist -- a cartridge that is merely inserted
// exercises nothing the base machine does not.
TEST(MachineStateTest, RestoringAStateReproducesTheSameRunFromACartridge)
{
   const int kSlicesAfter = 400;
   const char* kCart = "./res/CART/Eerie_Forest_(Logon_System_2017).cpr";

   DirectoriesImp dirImp; CDisplay display; Log log;
   SoundFactory soundFactory; ConfigurationManager conf_manager;
   EmulatorEngine* machine =
      NewBootedMachine(dirImp, display, log, soundFactory, conf_manager, "GX4000");

   ASSERT_EQ(0, machine->LoadCpr(kCart))
      << "the cartridge did not load, so this case proves nothing: " << kCart;
   machine->Reinit();

   // Let the demo get going: a cartridge that has only just been inserted is as
   // idle as an empty machine.
   for (int i = 0; i < 600; ++i)
      machine->RunTimeSlice();

   int ram_in_use = 0;
   const unsigned char* ram = machine->GetMem()->GetRamBuffer();
   for (int i = 0; i < 0x10000; ++i) if (ram[i] != 0) ++ram_in_use;
   ASSERT_GT(ram_in_use, 0) << "the cartridge never ran";

   std::vector<unsigned char> state;
   ASSERT_TRUE(MachineState::Save(machine, state));

   srand(0x5AFE5EED);
   for (int i = 0; i < kSlicesAfter; ++i)
      machine->RunTimeSlice();
   const Fingerprint straight_through = Capture(machine);

   ASSERT_TRUE(MachineState::Load(machine, &state[0], state.size()));
   srand(0x5AFE5EED);
   for (int i = 0; i < kSlicesAfter; ++i)
      machine->RunTimeSlice();
   const Fingerprint after_restore = Capture(machine);

   delete machine;

   int divergent = 0;
   for (size_t i = 0; i < straight_through.fields.size(); ++i)
   {
      if (straight_through.fields[i].second == after_restore.fields[i].second) continue;
      ++divergent;
      fprintf(stderr, "  STILL DIVERGES: %s\n", straight_through.fields[i].first.c_str());
   }
   fprintf(stderr, "MACHINE STATE gx4000    cartridge  %d of %zu fields diverged\n",
      divergent, straight_through.fields.size());

   EXPECT_EQ(0, divergent) << "the state did not carry everything a cartridge run needs";
}
