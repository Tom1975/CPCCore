#include "gtest/gtest.h"

#include "TestUtils.h"
#include "Motherboard.h"

// Colours of the ASIC palette.
//
// The Gate Array table (ListeColors, 0x00RRGGBB) goes through IDisplay::ConvertRGB, which
// puts the colour in the format of the display. The ASIC palette (pens, border, sprite
// inks) must go the same way: with a display whose format is not 0x00RRGGBB (the one of
// the tests is 0xAABBGGRR), red and blue were swapped for the ASIC colours only.

namespace
{

EmulatorEngine* BootPlus(DirectoriesImp& dirImp, CDisplay& display, Log& log,
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
   machine->LoadConfiguration("CONF/CPC6128PLUSFR.cfg");
   machine->Reinit();
   machine->SetFixedSpeed(true);
   machine->SetSpeedLimit(EmulatorEngine::E_FULL);
   for (int i = 0; i < 10; ++i)
      machine->RunTimeSlice(true);
   // RMR2 : ASIC registers in 4000-7FFF
   machine->GetMem()->SetRmr2(0x18);
   return machine;
}

// Palette register : byte 0 = red (bits 7-4) and blue (bits 3-0), byte 1 = green (bits 3-0)
void WriteAsicColour(EmulatorEngine* machine, unsigned short address, unsigned char r, unsigned char g, unsigned char b)
{
   machine->GetMem()->Set(address, (unsigned char)((r << 4) | b));
   machine->GetMem()->Set(address + 1, g);
}

}

TEST(AsicPalette, PensBorderAndSpriteInksAreInTheFormatOfTheDisplay)
{
   DirectoriesImp dirImp; CDisplay display; Log log;
   SoundFactory soundFactory; ConfigurationManager conf_manager;
   EmulatorEngine* machine = BootPlus(dirImp, display, log, soundFactory, conf_manager);

   const unsigned int red  = display.ConvertRGB(0xF00000) | 0xFF000000;
   const unsigned int blue = display.ConvertRGB(0x0000F0) | 0xFF000000;
   ASSERT_NE(red, 0xFFF00000u) << "the display of the tests must not be 0x00RRGGBB, or this test proves nothing";

   WriteAsicColour(machine, 0x6400, 15, 0, 0);    // pen 0 : red
   WriteAsicColour(machine, 0x6402, 0, 0, 15);    // pen 1 : blue
   WriteAsicColour(machine, 0x6420, 15, 0, 0);    // border : red
   WriteAsicColour(machine, 0x6422, 0, 0, 15);    // sprite ink 1 : blue

   EXPECT_EQ(machine->GetVGA()->ink_list_[0], red);
   EXPECT_EQ(machine->GetVGA()->ink_list_[1], blue);
   EXPECT_EQ(machine->GetVGA()->video_border_[0], red);
   EXPECT_EQ(machine->GetVGA()->sprite_ink_list_[1], blue);

   // An ink written to the Gate Array of a CPC+ goes to the ASIC palette : the same format
   EXPECT_EQ(machine->GetMem()->UpdateAsicPalette(2, 0x0C, false), red);   // hardware colour &4C : bright red

   delete machine;
}
