#include "gtest/gtest.h"

#include "TestUtils.h"

#include <cstdlib>
#include <vector>

// The tape input model (CTape::InputModel) applied to the ideal pulses of a CDT.
//
// The CPC reads the tape through an AC coupled amplifier : after a long level, the
// signal has drooped towards 0 and the next edge crosses the comparator sooner than
// after a short one. The pulse that follows a long one therefore starts earlier, and
// lasts longer, than written in the CDT. The tape keeps its total length.

namespace
{

// TZX with one "pulse sequence" block (0x13) : two long pulses, then short ones
std::vector<unsigned char> Tzx(const std::vector<unsigned short>& pulses)
{
   std::vector<unsigned char> tzx = { 'Z', 'X', 'T', 'a', 'p', 'e', '!', 0x1A, 1, 20 };
   tzx.push_back(0x13);
   tzx.push_back(static_cast<unsigned char>(pulses.size()));
   for (unsigned short p : pulses) { tzx.push_back(p & 0xFF); tzx.push_back(p >> 8); }
   return tzx;
}

// Lengths of the inversions, in ticks (4 MHz), once the tape is loaded
std::vector<unsigned long long> Load(EmulatorEngine* machine, std::vector<unsigned char> tzx, bool model)
{
   CTape::InputModel m = CTape::GetInputModel();
   const CTape::InputModel saved = m;
   m.enabled = model;
   CTape::SetInputModel(m);
   CTape* tape = machine->GetTape();
   tape->InsertTape(tzx.data(), static_cast<unsigned int>(tzx.size()));
   CTape::SetInputModel(saved);

   std::vector<unsigned long long> lengths;
   CTape::DebugFlux f;
   for (unsigned int i = 0; tape->GetFlux(i, f); ++i) lengths.push_back(f.length);
   return lengths;
}

}  // namespace

TEST(TapeInputModel, EdgeAfterALongPulseComesEarlier)
{
   DirectoriesImp dirImp; CDisplay display; Log log;
   SoundFactory soundFactory; ConfigurationManager conf_manager;
   EmulatorEngine* machine = new EmulatorEngine();
   display.Init(false);
   display.Show(false);
   machine->SetDirectories(&dirImp);
   machine->SetLog(&log);
   machine->SetConfigurationManager(&conf_manager);
   machine->Init(&display, &soundFactory);

   // Speedlock sync : 2 x 3150T, then 1130T pulses
   const std::vector<unsigned short> pulses = { 1130, 1130, 1130, 1130, 3150, 3150, 1130, 1130, 1130, 1130, 1130, 1130 };
   const std::vector<unsigned long long> ideal = Load(machine, Tzx(pulses), false);
   const std::vector<unsigned long long> read = Load(machine, Tzx(pulses), true);
   ASSERT_EQ(ideal.size(), read.size());

   // Find the first short pulse after the long ones, in the ideal tape
   size_t after_sync = 0;
   for (size_t i = 1; i < ideal.size(); ++i)
      if (ideal[i - 1] == 3600 && ideal[i] == 1291) { after_sync = i; break; }
   ASSERT_NE(0u, after_sync) << "sync not found in the ideal tape";

   // The ideal tape is the CDT : 1130T = 1291 ticks
   EXPECT_EQ(1291u, ideal[after_sync + 1]);

   // Read through the input : the pulse after the sync starts sooner, so lasts longer than
   // the following ones, which are all delayed alike
   EXPECT_GT(read[after_sync], read[after_sync + 2] + 16)
      << "the edge after the long pulse should come at least 4 us earlier";

   // No edge is lost, and each one stays close to its ideal place (a delay through the
   // filters, never a drift along the tape)
   long long ideal_edge = 0, read_edge = 0;
   for (size_t i = 1; i < ideal.size(); ++i)
   {
      EXPECT_GT(read[i], 0u) << "edge " << i << " lost";
      ideal_edge += static_cast<long long>(ideal[i]);
      read_edge += static_cast<long long>(read[i]);
      EXPECT_LT(std::llabs(read_edge - ideal_edge), 120) << "edge " << i << " moved more than 30 us";
   }

   delete machine;
}

TEST(TapeInputModel, TurboBlocksArePlayedAsTimed)
{
   DirectoriesImp dirImp; CDisplay display; Log log;
   SoundFactory soundFactory; ConfigurationManager conf_manager;
   EmulatorEngine* machine = new EmulatorEngine();
   display.Init(false);
   display.Show(false);
   machine->SetDirectories(&dirImp);
   machine->SetLog(&log);
   machine->SetConfigurationManager(&conf_manager);
   machine->Init(&display, &soundFactory);

   // Turbo block (0x11) : pilot 2254T x 100, sync 1032T/397T, zero 590T, one 1181T, 2 bytes
   std::vector<unsigned char> tzx = { 'Z', 'X', 'T', 'a', 'p', 'e', '!', 0x1A, 1, 20, 0x11 };
   const unsigned short words[] = { 2254, 1032, 397, 590, 1181, 100 };
   for (unsigned short w : words) { tzx.push_back(w & 0xFF); tzx.push_back(w >> 8); }
   tzx.insert(tzx.end(), { 8, 0, 0, 2, 0, 0, 0xFF, 0x00 });

   // The read signal already shaped these timings : the model leaves them
   EXPECT_EQ(Load(machine, tzx, false), Load(machine, tzx, true));

   delete machine;
}
