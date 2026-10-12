#include "gtest/gtest.h"

#include "TestWorkspace.h"

#include "TestUtils.h"

// Real crash found while wiring experimental tape-record support in
// SugarLibRetro (github.com/rtissera/SugarLibRetro): recording onto a
// genuinely blank tape (InsertBlankTape()) segfaults inside CTape::Tick(),
// once the tape's initial silent span runs out and Record()'s deferred
// start-of-recording code (the `start_record_` block) actually engages.
// `nb_inversions_`/`tape_position_` are unsigned, and the array-growth
// `memmove` calls compute their length as
//   nb_inversions_ - (tape_position_ + N)
// which wraps to a huge value whenever the tape doesn't yet have N more
// entries beyond the current position -- exactly the case right as
// recording starts on fresh blank media.
//
// The real-hardware repro needs the emulated tape to actually run out its
// (normally 20 real-minute) blank span, which took ~100 seconds of real
// emulated CPC time chasing this by hand in RetroArch. InsertBlankTape()
// now takes an optional duration so a test can shrink that to a handful of
// Tick() calls instead.
TEST(TapeRecording, RecordOntoFreshBlankTapeDoesNotCrash)
{
   DirectoriesImp dirImp;
   CDisplay display;
   Log log;
   SoundFactory soundFactory;
   ConfigurationManager conf_manager;
   // Heap-allocated, not a stack local: sizeof(EmulatorEngine) is ~5MB,
   // which overflows Windows' default 1MB thread stack.
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

   srand(0xE7123456);
   machine->SetFixedSpeed(true);

   CTape* tape = machine->GetTape();
   PPI8255* ppi = machine->GetPPI();
   ASSERT_NE(nullptr, tape);
   ASSERT_NE(nullptr, ppi);

   // Short blank span (1000 ticks instead of 20 real minutes) so it runs
   // out almost immediately -- this is what actually reaches the buggy
   // code path quickly instead of needing ~100 seconds of real emulated
   // time like the original bug report.
   tape->InsertBlankTape(1000);
   tape->Rewind();
   tape->SetMotorOn(true);
   tape->Record();

   // Simulate the guest toggling the cassette write line and cycling the
   // motor on/off, the way a real game's own tape-loading ROM routine does
   // while searching for a header that will never come on blank media --
   // the original bug report needed ~1.5 million inversions of exactly
   // this kind of incidental activity before it crashed, not a single
   // clean transition.
   unsigned int rng = 0x12345678u;
   bool level = false;
   for (int i = 0; i < 500000; ++i)
   {
      rng = rng * 1664525u + 1013904223u; // classic LCG, deterministic
      if ((rng & 0x7) == 0) level = !level;
      ppi->tape_write_data_level_ = level;
      if ((rng & 0x3FF) == 0)
      {
         tape->SetMotorOn(false);
         tape->Tick();
         tape->SetMotorOn(true);
      }
      tape->Tick();
   }

   // Reaching here at all (rather than SIGSEGV) is the actual assertion --
   // the crash this test targets kills the whole test binary, not just
   // this one case, so there is nothing more specific to ASSERT on.
   SUCCEED();
   delete machine;
}

// Second, distinct bug candidate found auditing the same "start_record_"
// area for a sibling of the fix above: the "Shorten now the following one"
// block right after the insert/extend logic (still inside `if (record_)`,
// but OUTSIDE the same/different-level branch -- it runs on every tick,
// not just on a transition) does:
//   tape_array_[tape_position_+1].length -= this_tick_time_;
// unconditionally, on a uint64_t field, without checking that length is
// actually >= this_tick_time_ first. Recording only ever holds
// this_tick_time_ at a fixed 4 T-states, so for this to underflow, an
// EXISTING entry immediately ahead of the recording position needs to be
// driven below 4 -- which happens for free if the guest holds the write
// line at a constant level for many consecutive ticks while overdubbing
// onto already-loaded (not blank) tape content: `tape_position_` does not
// move during a same-level run, so the same subsequent entry gets -=4'd
// on every single tick until it wraps. This targets a real loaded tape,
// not a blank one -- the "recording onto an already-populated tape did
// not crash" note from the original bug report was from a short test,
// not proof this path is safe.
TEST(TapeRecording, OverdubOntoLoadedTapeDoesNotUnderflowNextEntryLength)
{
   DirectoriesImp dirImp;
   CDisplay display;
   Log log;
   SoundFactory soundFactory;
   ConfigurationManager conf_manager;
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

   srand(0xE7123456);
   machine->SetFixedSpeed(true);

   // Real commercial tape dump already used by Test_Dumps_tape.cpp -- real
   // pulse-length data, not synthetic, so the entries ahead of position 0
   // have genuine, varied lengths to overdub onto.
   machine->LoadTape(TestWorkspace::Fixture("./res/Tape/Lemmings (UK) (1991) (01. Level 01 FUN - JUST DIG!) (Version Split) [Original] [TAPE].cdt").c_str());
   for (int i = 0; i < 100; ++i)
      machine->RunTimeSlice();

   CTape* tape = machine->GetTape();
   PPI8255* ppi = machine->GetPPI();
   ASSERT_NE(nullptr, tape);
   ASSERT_NE(nullptr, ppi);
   ASSERT_GT(tape->GetNbInversions(), 10u) << "tape did not actually load, test would prove nothing";

   tape->Rewind();
   tape->SetMotorOn(true);
   tape->Record();

   // SetMotorOn() doesn't take effect immediately either (it arms a
   // MOTOR_DELAY countdown), so neither the motor nor start_record_'s
   // transition are guaranteed to have happened after just one tick.
   // Tick until recording is actually live before trusting
   // GetTapePosition() -- otherwise "the entry the shorten logic targets"
   // gets computed from a stale, pre-transition position (this was a real
   // mistake in an earlier version of this test: it watched the position
   // BEFORE start_record_'s own increment, which is index tape_position_
   // itself post-transition -- the entry actively being recorded into,
   // which is SUPPOSED to grow while a level is held constant -- not the
   // shorten target one further ahead).
   ppi->tape_write_data_level_ = false;
   int settle_ticks = 0;
   while (!tape->IsRecordOn() && settle_ticks < 10000)
   {
      tape->Tick();
      ++settle_ticks;
   }
   ASSERT_TRUE(tape->IsRecordOn()) << "recording never actually started";

   unsigned int watch_idx = tape->GetTapePosition() + 1;
   CTape::DebugFlux before;
   ASSERT_TRUE(tape->GetFlux(watch_idx, before));

   // Hold the write line at a constant level: no more transitions, so
   // tape_position_ stays put and the "shorten the following one" block
   // hits the SAME next entry on every remaining tick. The bug does not
   // necessarily crash the process -- it corrupts a length field into a
   // huge value that only blows up later (export, or a subsequent
   // playback pass) -- so check the actual value, don't just hope for a
   // SIGSEGV.
   for (int i = 0; i < 199999; ++i)
      tape->Tick();

   CTape::DebugFlux after;
   ASSERT_TRUE(tape->GetFlux(watch_idx, after));
   // A real (non-wrapped) shortening only ever decreases length towards
   // zero. Wrapping past zero on a uint64_t makes it enormous instead.
   EXPECT_LT(after.length, before.length + 1)
      << "watched entry length grew from " << before.length << " to "
      << after.length << " -- classic uint64_t underflow signature";
   delete machine;
}

// Real end-to-end round trip, not just "the internal array survives":
// record a known square-wave pattern onto a blank tape, export it with
// CTape's own SaveAsCdtCSW(), load that exported file into a SEPARATE
// fresh machine, play it back, and confirm the played-back signal
// actually reflects what was recorded (right ballpark of transitions,
// not silence, not garbage). Neither of the two bugs fixed above would
// have been caught by this alone (they are about internal bookkeeping,
// not output correctness), which is exactly why this test exists
// separately -- "doesn't crash/corrupt" and "records the right thing"
// are different claims.
TEST(TapeRecording, RecordedTapeReloadsAndReplaysTheWrittenSignal)
{
   const char* kExportPath = "./tape_roundtrip_test.cdt";

   unsigned int written_transitions = 0;
   unsigned long long written_total_ticks = 0;

   // --- Machine A: record a known pattern ---
   {
      DirectoriesImp dirImp;
      CDisplay display;
      Log log;
      SoundFactory soundFactory;
      ConfigurationManager conf_manager;
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

      srand(0xE7123456);
      machine->SetFixedSpeed(true);

      CTape* tape = machine->GetTape();
      PPI8255* ppi = machine->GetPPI();
      ASSERT_NE(nullptr, tape);
      ASSERT_NE(nullptr, ppi);

      tape->InsertBlankTape(2000000); // long enough not to run out mid-test
      tape->Rewind();
      tape->SetMotorOn(true);
      tape->Record();

      int settle_ticks = 0;
      while (!tape->IsRecordOn() && settle_ticks < 10000)
      {
         tape->Tick();
         ++settle_ticks;
      }
      ASSERT_TRUE(tape->IsRecordOn());

      // A plain square wave: flip every 50 ticks, 60 flips -- a real,
      // recognizable, non-trivial signal, not one long silence.
      const int kPeriodTicks = 50;
      const int kFlips = 60;
      bool level = ppi->tape_write_data_level_;
      for (int flip = 0; flip < kFlips; ++flip)
      {
         level = !level;
         ppi->tape_write_data_level_ = level;
         ++written_transitions;
         for (int t = 0; t < kPeriodTicks; ++t)
            tape->Tick();
         written_total_ticks += (unsigned long long)kPeriodTicks * 4; // this_tick_time_ is always 4 while recording
      }

      ASSERT_GT(tape->GetNbInversions(), written_transitions)
         << "recording produced far fewer inversions than the transitions we actually wrote";

      tape->SaveAsCdtCSW(kExportPath);
      delete machine;
   }

   // The export must actually exist and look like a real CDT (TZX magic),
   // not an empty or garbage file.
   FILE* f = fopen(kExportPath, "rb");
   ASSERT_NE(nullptr, f) << "SaveAsCdtCSW did not produce a file";
   char magic[8] = {0};
   size_t magic_read = fread(magic, 1, 7, f);
   fseek(f, 0, SEEK_END);
   long file_size = ftell(f);
   fclose(f);
   ASSERT_EQ(7u, magic_read);
   EXPECT_STREQ("ZXTape!", magic) << "exported file does not start with the real CDT/TZX magic";
   EXPECT_GT(file_size, 20) << "exported file is suspiciously tiny -- likely empty of real data";

   // --- Machine B: reload the exported file into a completely separate
   // machine and play it back for real ---
   DirectoriesImp dirImp2;
   CDisplay display2;
   Log log2;
   SoundFactory soundFactory2;
   ConfigurationManager conf_manager2;
   EmulatorEngine* machine2 = new EmulatorEngine();

   display2.Init(false);
   display2.Show(false);

   machine2->SetDirectories(&dirImp2);
   machine2->SetLog(&log2);
   machine2->SetConfigurationManager(&conf_manager2);
   machine2->Init(&display2, &soundFactory2);
   machine2->GetMem()->Initialisation();
   machine2->LoadConfiguration("./TestConf.ini", "./TestConf_0.ini");
   machine2->Reinit();
   machine2->SetFixedSpeed(true);

   machine2->LoadTape(kExportPath);
   for (int i = 0; i < 200; ++i)
      machine2->RunTimeSlice();

   CTape* tape2 = machine2->GetTape();
   PPI8255* ppi2 = machine2->GetPPI();
   ASSERT_NE(nullptr, tape2);
   ASSERT_NE(nullptr, ppi2);
   ASSERT_GT(tape2->GetNbInversions(), 0u) << "re-exported tape did not actually load anything";

   tape2->Rewind();
   tape2->SetMotorOn(true);
   tape2->Play();

   // Sample the played-back cassette-read level every tick and count real
   // transitions -- proves the exported file actually carries a real,
   // varying signal, not silence or a single stuck level.
   unsigned int replay_transitions = 0;
   bool prev_level = (ppi2->tape_level_ != 0);
   const int kReplayTicks = 400000; // generous -- real CSW pause/leader framing costs some
   for (int i = 0; i < kReplayTicks; ++i)
   {
      tape2->Tick();
      bool cur_level = (ppi2->tape_level_ != 0);
      if (cur_level != prev_level)
      {
         ++replay_transitions;
         prev_level = cur_level;
      }
   }

   remove(kExportPath);

   EXPECT_GT(replay_transitions, 0u)
      << "played-back signal never changed level at all -- exported file is effectively silent";
   // Real ballpark check, not exact equality: CSW/TZX framing (pause
   // blocks, leader tone, rounding to 1/44100s samples) means the exact
   // transition count will not match bit-for-bit, but it should be the
   // same order of magnitude as what was actually written, not off by
   // 10x/100x (which would mean the export/reload lost or fabricated most
   // of the signal).
   EXPECT_GT(replay_transitions, written_transitions / 4)
      << "replayed only " << replay_transitions << " transitions vs "
      << written_transitions << " written -- exported signal looks mostly lost";
   EXPECT_LT(replay_transitions, written_transitions * 10)
      << "replayed " << replay_transitions << " transitions vs only "
      << written_transitions << " written -- exported signal looks fabricated/noisy";
   delete machine2;
}

// Record() had no counterpart, so a caller could arm recording but never end
// it. Two behaviours to prove: stopping mid-recording really does stop new
// inversions being appended, and stopping before the deferred start_record_
// has engaged cancels the arm instead of recording anyway.
TEST(TapeRecording, StopRecordActuallyStopsRecording)
{
   DirectoriesImp dirImp;
   CDisplay display;
   Log log;
   SoundFactory soundFactory;
   ConfigurationManager conf_manager;
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

   srand(0xE7123456);
   machine->SetFixedSpeed(true);

   CTape* tape = machine->GetTape();
   PPI8255* ppi = machine->GetPPI();
   ASSERT_NE(nullptr, tape);
   ASSERT_NE(nullptr, ppi);

   // --- Case 1: stop mid-recording ---
   tape->InsertBlankTape(2000000);
   tape->Rewind();
   tape->SetMotorOn(true);
   tape->Record();

   int settle_ticks = 0;
   while (!tape->IsRecordOn() && settle_ticks < 10000)
   {
      tape->Tick();
      ++settle_ticks;
   }
   ASSERT_TRUE(tape->IsRecordOn());

   bool level = ppi->tape_write_data_level_;
   for (int i = 0; i < 500; ++i)
   {
      level = !level;
      ppi->tape_write_data_level_ = level;
      for (int t = 0; t < 20; ++t)
         tape->Tick();
   }
   ASSERT_TRUE(tape->IsRecordOn());
   unsigned int nb_inversions_before_stop = tape->GetNbInversions();

   tape->StopRecord();
   EXPECT_FALSE(tape->IsRecordOn());

   // Keep toggling the write line and ticking exactly as before -- if
   // StopRecord() actually disengaged the record head, none of this
   // should create new inversions any more.
   for (int i = 0; i < 500; ++i)
   {
      level = !level;
      ppi->tape_write_data_level_ = level;
      for (int t = 0; t < 20; ++t)
         tape->Tick();
   }
   EXPECT_EQ(nb_inversions_before_stop, tape->GetNbInversions())
      << "tape kept recording new inversions after StopRecord()";

   // --- Case 2: cancel before the deferred start ever engages ---
   // (InsertBlankTape() itself calls Eject() first, resetting position/
   // record state cleanly.)
   tape->InsertBlankTape(2000000);
   tape->Rewind();
   tape->SetMotorOn(true);
   tape->Record();
   tape->StopRecord(); // cancel before any Tick() has processed start_record_
   EXPECT_FALSE(tape->IsRecordOn());

   settle_ticks = 0;
   while (settle_ticks < 10000)
   {
      tape->Tick();
      ++settle_ticks;
   }
   EXPECT_FALSE(tape->IsRecordOn())
      << "StopRecord() before the deferred start engaged did not cancel it -- recording started anyway";
   delete machine;
}
