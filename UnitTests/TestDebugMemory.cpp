#include "gtest/gtest.h"

#include "TestUtils.h"

#include <vector>

// Debug reads of the RAM banks (memory dialog, debug server) : "lower 64 KB" and each
// expansion page are 64 KB, made of four 16 KB banks. Only the first bank was read,
// four times over.
TEST(DebugMemory, ReadOfRamBanksCoversTheirFourBlocks)
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
   machine->GetMem()->Initialisation();
   machine->LoadConfiguration("CONF/CPC6128FR.cfg");
   machine->Reinit();
   Memory* mem = machine->GetMem();

   // Base RAM, configuration 0 : 0000-FFFF is banks 0-3
   mem->ConnectBank(0, 0, 0);
   for (int b = 0; b < 4; ++b) mem->Set((unsigned short)(b * 0x4000 + 0x123), (unsigned char)(0x10 + b));
   // Expansion page 0, configuration 4..7 maps its bank n at 4000-7FFF
   for (int b = 0; b < 4; ++b)
   {
      mem->ConnectBank(0, 1, (unsigned char)b);
      mem->Set(0x4123, (unsigned char)(0x20 + b));
   }
   mem->ConnectBank(0, 0, 0);

   std::vector<unsigned char> lower(0x10000), page(0x10000);
   mem->GetDebugValue(lower.data(), 0, 0x10000, Memory::MEM_RAM_LOWER_BANK, 0);
   mem->GetDebugValue(page.data(), 0, 0x10000, Memory::MEM_RAM_BANK, 0);
   for (int b = 0; b < 4; ++b)
   {
      EXPECT_EQ(0x10 + b, lower[b * 0x4000 + 0x123]) << "lower RAM, bank " << b;
      EXPECT_EQ(0x20 + b, page[b * 0x4000 + 0x123]) << "expansion page 0, bank " << b;
   }

   delete machine;
}
