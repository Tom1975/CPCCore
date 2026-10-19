#include "gtest/gtest.h"

#include "TestWorkspace.h"

#include "TestUtils.h"

#include <vector>

// Checked in-memory, against the live tape_array_, not via a CSW
// export/reload: TestTapeFluxExact.cpp's null experiment shows CSW
// re-splits entries even with no bug involved, so a file-level byte
// comparison wouldn't prove anything here.
//
// Claim: after a short, bounded overdub burst, every flux entry the burst
// did NOT touch is still EXACTLY (length, high) what it was before --
// exactly what Bug 2's "shorten now the following one" fix guarantees.
TEST(TapeOverdubByteExact, EntriesFarFromOverdubBurstAreUnchanged)
{
   DirectoriesImp dirImp;
   CDisplay display;
   Log log;
   SoundFactory soundFactory;
   ConfigurationManager conf_manager;
   // Heap-allocated, not a stack local: sizeof(EmulatorEngine) is ~5MB.
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
   machine->SetSpeedLimit(EmulatorEngine::E_FULL);

   machine->LoadTape(TestWorkspace::Fixture("./res/Tape/Lemmings (UK) (1991) (01. Level 01 FUN - JUST DIG!) (Version Split) [Original] [TAPE].cdt").c_str());
   for (int i = 0; i < 100; ++i)
      machine->RunTimeSlice();

   CTape* tape = machine->GetTape();
   PPI8255* ppi = machine->GetPPI();
   ASSERT_NE(nullptr, tape);
   ASSERT_NE(nullptr, ppi);
   ASSERT_GT(tape->GetNbInversions(), 300u) << "tape did not actually load, test would prove nothing";

   // Snapshot the whole array before touching anything.
   unsigned int nb_before = tape->GetNbInversions();
   std::vector<CTape::DebugFlux> before(nb_before);
   for (unsigned int i = 0; i < nb_before; ++i)
      ASSERT_TRUE(tape->GetFlux(i, before[i]));

   tape->Rewind();
   tape->SetMotorOn(true);
   tape->Record();

   int settle_ticks = 0;
   while (!tape->IsRecordOn() && settle_ticks < 10000)
   {
      tape->Tick();
      ++settle_ticks;
   }
   ASSERT_TRUE(tape->IsRecordOn()) << "recording never actually started";

   unsigned int overdub_start_idx = tape->GetTapePosition();

   // Hold the write line constant long enough to fully consume at least
   // one ahead-of-position entry and cross into the next -- that crossing
   // is exactly where the two stacked bugs diverge from correct behaviour.
   ppi->tape_write_data_level_ = false;
   for (int i = 0; i < 300000; ++i)
      tape->Tick();

   unsigned int nb_after = tape->GetNbInversions();

   // The one genuine level transition at the burst's start legitimately
   // splices in a new entry, shifting every later index by a fixed delta
   // -- compare before[idx] against after[idx+delta], not after[idx].
   long long delta = (long long)nb_after - (long long)nb_before;
   fprintf(stderr, "DIAG: nb_before=%u nb_after=%u delta=%lld overdub_start_idx=%u\n", nb_before, nb_after, delta, overdub_start_idx);

   // The bug's signature is a uint64_t wraparound (18446744073708754220 in
   // the original repro), not a zero -- a legitimately-consumed entry is
   // set to exactly 0. No real flux pulse gets anywhere near this bound
   // (a full 20-minute blank tape span is ~4.8e9 ticks).
   const unsigned long long kSaneMaxLength = 1000000000ULL; // 1e9 ticks (~250s of tape)
   unsigned long long max_length_seen = 0;
   unsigned int max_length_idx = 0;
   for (unsigned int idx = 0; idx < nb_after; ++idx)
   {
      CTape::DebugFlux e;
      ASSERT_TRUE(tape->GetFlux(idx, e));
      if (e.length > max_length_seen)
      {
         max_length_seen = e.length;
         max_length_idx = idx;
      }
   }
   fprintf(stderr, "DIAG: max entry length after overdub = %llu at idx %u\n", max_length_seen, max_length_idx);
   EXPECT_LT(max_length_seen, kSaneMaxLength)
      << "entry " << max_length_idx << " has length " << max_length_seen
      << " -- classic uint64_t underflow signature (a legitimately-consumed entry is set to exactly 0, never wraps)";

   // Entries well past the write region must be byte-identical to before.
   const unsigned int kSafetyMargin = 2000; // comfortably past anything a 300000-tick burst could reach
   unsigned int untouched_start = overdub_start_idx + kSafetyMargin;
   ASSERT_LT(untouched_start, nb_before) << "test tape too short to leave an untouched tail -- pick a longer fixture or shorter burst";

   unsigned int checked = 0;
   for (unsigned int idx = untouched_start; idx < nb_before; ++idx)
   {
      long long after_idx = (long long)idx + delta;
      if (after_idx < 0 || after_idx >= (long long)nb_after) break;
      CTape::DebugFlux after_entry;
      ASSERT_TRUE(tape->GetFlux((unsigned int)after_idx, after_entry));
      EXPECT_EQ(before[idx].length, after_entry.length)
         << "entry " << idx << " length changed from " << before[idx].length
         << " to " << after_entry.length << " despite being well outside the overdub burst"
         << " -- corruption bled outside the write region";
      EXPECT_EQ(before[idx].high, after_entry.high)
         << "entry " << idx << " polarity changed despite being outside the overdub burst";
      ++checked;
   }
   EXPECT_GT(checked, 0u) << "no entries were actually compared -- test would prove nothing";
   fprintf(stderr, "DIAG: compared %u untouched entries (before-idx %u..%u) byte-for-byte against after-idx+delta, all must be identical to pre-overdub content\n",
      checked, untouched_start, untouched_start + checked - 1);
   delete machine;
}
