#include "../benchmark/include/benchmark/benchmark.h"
#include <set>
#include <vector>
#include "TestUtils.h"

void BenchmarkOpcode (Motherboard *motherboard_emulation, unsigned char opcode)
{
   motherboard_emulation->SetPlus(true);
   motherboard_emulation->OnOff();
   motherboard_emulation->GetMem()->InitMemory();
   motherboard_emulation->GetMem()->SetRam(1);
   motherboard_emulation->GetCRTC()->DefinirTypeCRTC(CRTC::AMS40226);
   motherboard_emulation->GetVGA()->SetPAL(true);

   // Set Memory (0x04 = INC B)
   for (int i = 0; i < 4; i++)
   {
      memset(motherboard_emulation->GetMem()->GetRamRead(i), opcode, sizeof(Memory::RamBank));
   }

   // Empty cartridge
   for (int i = 0; i < 32; i++)
   {
      unsigned char* rom = motherboard_emulation->GetCartridge(i);
      memset(rom, opcode, sizeof(Memory::RamBank));
   }


   motherboard_emulation->GetPSG()->Reset();
   motherboard_emulation->GetSig()->Reset();


   motherboard_emulation->InitStartOptimizedPlus();

   motherboard_emulation->OnOff();
}

// Main Sugarbox benchmark
static void BM_z80full(benchmark::State &state) {

   for (auto _ : state) {
      InitBinary("6128", ".\\TestConf.ini", ".\\res\\z80\\z80full.bin", 0x807A, 0x8065);
   }
}

BENCHMARK(BM_z80full)->Unit(benchmark::kMillisecond);
static void BM_NOP(benchmark::State &state)
{
   SoundMixer        sound_mixer;
   Motherboard       *motherboard_emulation;
   CDisplay          display;
   KeyboardForTest   keyboard;

   
#ifdef _DEBUG
   display.Init(true);
   display.Show(true);
#else
   display.Init(false);
   display.Show(false);
#endif
   motherboard_emulation = new Motherboard(&sound_mixer, &keyboard);
   motherboard_emulation->InitMotherbard(nullptr, nullptr, &display, nullptr, nullptr, nullptr);
   BenchmarkOpcode(motherboard_emulation, 0);

   for (auto _ : state)
   {
      motherboard_emulation->StartOptimizedPlus<false, false, false>(4000 * 1000 * 20);
      //motherboard_emulation->Start(0, 4000 * 50 * 20);
   }
   delete motherboard_emulation;
}

BENCHMARK(BM_NOP)->Unit(benchmark::kMillisecond);

static void BM_INC_B(benchmark::State &state)
{
   SoundMixer        sound_mixer;
   Motherboard       *motherboard_emulation;
   CDisplay          display;
   KeyboardForTest   keyboard;

#ifdef _DEBUG
   display.Init(true);
   display.Show(true);
#else
   display.Init(false);
   display.Show(false);
#endif

   motherboard_emulation = new Motherboard(&sound_mixer, &keyboard);
   motherboard_emulation->InitMotherbard(nullptr, nullptr, &display, nullptr, nullptr, nullptr);

   BenchmarkOpcode(motherboard_emulation, 0x4);

   for (auto _ : state)
   {
      //motherboard_emulation->Start(0, 4000 * 50 * 20);
      motherboard_emulation->StartOptimizedPlus<false, false, false>(4000 * 50 * 20);
   }
   delete motherboard_emulation;
}

BENCHMARK(BM_INC_B)->Unit(benchmark::kMillisecond);

static void BM_INC_BC(benchmark::State &state)
{
   SoundMixer        sound_mixer;
   Motherboard       *motherboard_emulation;
   CDisplay          display;
   KeyboardForTest   keyboard;

#ifdef _DEBUG
   display.Init(true);
   display.Show(true);
#else
   display.Init(false);
   display.Show(false);
#endif

   motherboard_emulation = new Motherboard(&sound_mixer, &keyboard);
   motherboard_emulation->InitMotherbard(nullptr, nullptr, &display, nullptr, nullptr, nullptr);

   BenchmarkOpcode(motherboard_emulation, 0x3);

   for (auto _ : state)
   {
      motherboard_emulation->StartOptimizedPlus<false, false, false>( 4000 * 50 * 20);
   }
   delete motherboard_emulation;
}

BENCHMARK(BM_INC_BC)->Unit(benchmark::kMillisecond);


static void BM_RLCA(benchmark::State &state)
{
   SoundMixer        sound_mixer;
   Motherboard       *motherboard_emulation;
   CDisplay          display;
   KeyboardForTest   keyboard;

#ifdef _DEBUG
   display.Init(true);
   display.Show(true);
#else
   display.Init(false);
   display.Show(false);
#endif

   motherboard_emulation = new Motherboard(&sound_mixer, &keyboard);
   motherboard_emulation->InitMotherbard(nullptr, nullptr, &display, nullptr, nullptr, nullptr);

   BenchmarkOpcode(motherboard_emulation, 0x7);

   for (auto _ : state)
   {

      motherboard_emulation->StartOptimizedPlus<false, false, false>(4000 * 50 * 20);
   }
   delete motherboard_emulation;
}

BENCHMARK(BM_RLCA)->Unit(benchmark::kMillisecond);

// Eerie forest GX4000 benchmark
static void BM_EerieForest(benchmark::State &state) {

   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(15000));
  
   for (auto _ : state) {
      test_dump.TestCardridge("GX4000", ".\\TestConf.ini", ".\\res\\CART\\Eerie_Forest_(Logon_System_2017).cpr", &cmd_list);
   }
}

BENCHMARK(BM_EerieForest)->Unit(benchmark::kMillisecond);

// Mod 2 PLUS benchmark
static void BM_Mode2plus(benchmark::State &state) {

   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(100));
   cmd_list.AddCommand(new CommandKeyboard("mode 2"));
   cmd_list.AddCommand(new CommandRunCycles(80));
   cmd_list.AddCommand(new CommandKeyboard("\r"));
   cmd_list.AddCommand(new CommandRunCycles(15000));

   for (auto _ : state) {
      test_dump.TestCardridge("GX4000", ".\\TestConf.ini", ".\\res\\CART\\AmstradPlus.f4 (Parados 1.0) (unofficial).cpr", &cmd_list);
   }
}

BENCHMARK(BM_Mode2plus)->Unit(benchmark::kMillisecond);


// Eerie forest GX4000 benchmark
static void BM_CRTC3(benchmark::State &state) {

   TestDump test_dump;
   CommandList cmd_list;
   cmd_list.AddCommand(new CommandRunCycles(15000));

   for (auto _ : state) {
      test_dump.TestCardridge("GX4000", ".\\TestConf.ini", ".\\res\\CART\\crtc3_projo.cpr", &cmd_list);
   }
}

BENCHMARK(BM_CRTC3)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
