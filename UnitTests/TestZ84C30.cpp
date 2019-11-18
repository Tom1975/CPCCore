
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <iostream>

#include "tchar.h"
#include "windows.h"
#include "gtest/gtest.h"
#include "Z84C30.h"

/////////////////////////////////////////////////////////////
/// Helper functions

/////////////////////////////////////////////////////////////
//
class Interrupt : public IClockable
{
public:
   Interrupt() { interrupted_ = false; }
   virtual unsigned int Tick() { interrupted_ = true; return 1; };
   bool IsInterruted() { return interrupted_; }

private:
   bool interrupted_;
};

// TEST : Reset
TEST(Z84C30, soft_reset)
{
   Z84C30 z84c30;
   Interrupt interrupt;

   // Callback functions
   z84c30.Init(Z84C30::CHANNEL_0, nullptr, &interrupt);
   // control word init : EnableInterrupt, Timer, x16, Rising edge, Auto, time constant following, soft reset,  Control word
   z84c30.Out(0, 0xC5);
   // Time constant : 16
   z84c30.Out(0, 16);
   // Soft reset
   z84c30.Out(0, 0xC5);

   // Count : Nothing should happen
   // Trigger after : 16*16
   for (int i = 0; i < 16 * 16; i++)
   {
      ASSERT_EQ(interrupt.IsInterruted(), false);

      z84c30.Tick();

   }
   ASSERT_EQ(interrupt.IsInterruted(), false);

}

TEST(Z84C30, hard_reset)
{
   Z84C30 z84c30;
   Interrupt interrupt;

   // Callback functions
   z84c30.Init(Z84C30::CHANNEL_0, nullptr, &interrupt);
   // control word init : EnableInterrupt, Timer, x16, Rising edge, Auto, time constant following, soft reset,  Control word
   z84c30.Out(0, 0xC5);
   // Time constant : 16
   z84c30.Out(0, 16);
   // Hard reset
   z84c30.HardReset();

   // Count : Nothing should happen
   // Trigger after : 16*16
   for (int i = 0; i < 16 * 16; i++)
   {
      ASSERT_EQ(interrupt.IsInterruted(), false);

      z84c30.Tick();

   }
   // No interrupt should occurs
   ASSERT_EQ(interrupt.IsInterruted(), false);
}

// TEST : Check that a correct count trigger an interrupt
TEST(Z84C30, count_to_interrupt)
{
   Z84C30 z84c30;
   Interrupt interrupt;

   // Callback functions
   z84c30.Init(Z84C30::CHANNEL_0, nullptr, &interrupt);

   // control word init : EnableInterrupt, Timer, x16, Rising edge, Auto, time constant following, soft reset,  Control word
   z84c30.Out( 0, 0x85 );
   // Time constant : 16
   z84c30.Out(0, 16);

   // Tick to enable the loading constant
   z84c30.Tick();

   // Trigger after : 16*16
   for (int i = 0; i < 16 * 16; i++)
   {
      ASSERT_EQ(interrupt.IsInterruted(), false);

      z84c30.Tick();
      
   }
   ASSERT_EQ(interrupt.IsInterruted(), true);

}
