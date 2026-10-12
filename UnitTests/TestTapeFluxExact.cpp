#include "gtest/gtest.h"

#include "TestWorkspace.h"

#include "TestUtils.h"

#include <vector>
#include <algorithm>

// Null experiment: load a real commercial tape, export it unchanged via
// SaveAsCdtCSW(), reload into a separate machine, and check whether the
// flux array survives the CSW round trip exactly. This decides whether
// "byte exact" can mean literal length/high equality per entry, or
// whether CSW's own sample-rate quantization makes that unreachable even
// with zero bugs involved.
static std::vector<CTape::DebugFlux> SnapshotFlux(CTape* tape)
{
   std::vector<CTape::DebugFlux> out;
   unsigned int n = tape->GetNbInversions();
   out.reserve(n);
   for (unsigned int i = 0; i < n; ++i)
   {
      CTape::DebugFlux f;
      if (tape->GetFlux(i, f)) out.push_back(f);
   }
   return out;
}

TEST(TapeFluxExact, ExportReloadOfUnmodifiedTapeIsLosslessAtFluxLevel)
{
   const char* kExportPath = "./tape_null_experiment.cdt";
   const std::string source_tape_path = TestWorkspace::Fixture("./res/Tape/Lemmings (UK) (1991) (01. Level 01 FUN - JUST DIG!) (Version Split) [Original] [TAPE].cdt");
   const char* kSourceTape = source_tape_path.c_str();

   std::vector<CTape::DebugFlux> before;

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
      machine->SetFixedSpeed(true);
      machine->SetSpeedLimit(EmulatorEngine::E_FULL);

      machine->LoadTape(kSourceTape);
      for (int i = 0; i < 100; ++i)
         machine->RunTimeSlice();

      CTape* tape = machine->GetTape();
      ASSERT_NE(nullptr, tape);
      ASSERT_GT(tape->GetNbInversions(), 100u) << "source tape did not actually load, test would prove nothing";

      before = SnapshotFlux(tape);
      tape->SaveAsCdtCSW(kExportPath);
      delete machine;
   }

   std::vector<CTape::DebugFlux> after;
   {
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
      machine2->SetSpeedLimit(EmulatorEngine::E_FULL);

      machine2->LoadTape(kExportPath);
      for (int i = 0; i < 100; ++i)
         machine2->RunTimeSlice();

      CTape* tape2 = machine2->GetTape();
      ASSERT_NE(nullptr, tape2);
      after = SnapshotFlux(tape2);
      delete machine2;
   }

   remove(kExportPath);

   fprintf(stderr, "DIAG: before.size()=%zu after.size()=%zu\n", before.size(), after.size());
   unsigned int report_n = (unsigned int)std::min(before.size(), after.size());
   unsigned int first_mismatch = report_n;
   long long max_abs_delta = 0;
   for (unsigned int i = 0; i < report_n; ++i)
   {
      long long delta = (long long)after[i].length - (long long)before[i].length;
      if (delta < 0) delta = -delta;
      if (delta > max_abs_delta) max_abs_delta = delta;
      if (first_mismatch == report_n && (after[i].length != before[i].length || after[i].high != before[i].high))
         first_mismatch = i;
   }
   fprintf(stderr, "DIAG: first_mismatch idx=%u max_abs_length_delta=%lld\n", first_mismatch, max_abs_delta);
   if (first_mismatch < report_n)
   {
      fprintf(stderr, "DIAG: at idx %u: before.length=%llu before.high=%d after.length=%llu after.high=%d\n",
         first_mismatch,
         (unsigned long long)before[first_mismatch].length, (int)before[first_mismatch].high,
         (unsigned long long)after[first_mismatch].length, (int)after[first_mismatch].high);
   }

   SUCCEED();
}
