#include "gtest/gtest.h"

#include "Machine.h"
#include "KeyboardHandler.h"

#include <cstring>

// The GX4000 is a console: most of the keyboard matrix is simply not wired.
// Per MAME's gx4000 input ports (src/mame/amstrad/amstrad.cpp, "The GX4000 is a
// console, so no keyboard access other than the joysticks"), only three lines
// carry anything: line 3 bit 3 is the Pause button (the P key position on a
// CPC), line 6 is joystick 2 and line 9 joystick 1, both with bits 6-7 unused.
// Everything else reads as no key pressed, so a host that keeps feeding a full
// keyboard into the matrix cannot type into a GX4000 game.
//
// CPCWiki's GX4000 page says the unconnected lines are "undefined, possibly
// high"; 0xFF follows MAME rather than a measurement on the real console.

namespace
{

const int kPauseLine = 3;
const unsigned char kPauseBit = 0x08;
const int kJoystick2Line = 6;
const int kJoystick1Line = 9;

void PressEverything(KeyboardHandler& keyboard)
{
   unsigned char all_pressed[10];
   memset(all_pressed, 0x00, sizeof(all_pressed));  // bit = 0 means pressed
   keyboard.ForceKeyboardState(all_pressed);
}

}  // namespace

TEST(Gx4000Keyboard, ConsoleWiringOnlyKeepsPauseAndTheTwoJoysticks)
{
   KeyboardHandler keyboard;
   bool register_replaced = false;
   keyboard.Init(&register_replaced);
   keyboard.SetConsoleWiring(true);
   PressEverything(keyboard);

   for (int line = 0; line < 10; ++line)
   {
      if (line == kPauseLine || line == kJoystick1Line || line == kJoystick2Line)
         continue;
      EXPECT_EQ(0xFF, keyboard.GetKeyboardMap(line)) << "line " << line << " is not wired";
   }

   EXPECT_EQ(static_cast<unsigned char>(~kPauseBit), keyboard.GetKeyboardMap(kPauseLine))
      << "line 3 carries the Pause button and nothing else";
   EXPECT_EQ(0xC0, keyboard.GetKeyboardMap(kJoystick2Line)) << "joystick 2, bits 6-7 unused";
   EXPECT_EQ(0xC0, keyboard.GetKeyboardMap(kJoystick1Line)) << "joystick 1, bits 6-7 unused";
}

TEST(Gx4000Keyboard, AComputerKeepsTheWholeMatrix)
{
   KeyboardHandler keyboard;
   bool register_replaced = false;
   keyboard.Init(&register_replaced);
   keyboard.SetConsoleWiring(false);
   PressEverything(keyboard);

   for (int line = 0; line < 10; ++line)
      EXPECT_EQ(0x00, keyboard.GetKeyboardMap(line)) << "line " << line;
}

// The wiring follows the machine, not the frontend: selecting GX400 is what
// unplugs the keyboard, and selecting a computer plugs it back in.
TEST(Gx4000Keyboard, TheMachineTypeSelectsTheWiring)
{
   EmulatorEngine machine;
   KeyboardHandler* keyboard = machine.GetKeyboardHandler();

   machine.SetMachineType(MachineSettings::GX400);
   PressEverything(*keyboard);
   EXPECT_EQ(0xFF, keyboard->GetKeyboardMap(0)) << "a GX4000 has no keyboard line 0";

   machine.SetMachineType(MachineSettings::OLD_6128);
   PressEverything(*keyboard);
   EXPECT_EQ(0x00, keyboard->GetKeyboardMap(0)) << "a 6128 has one";
}
