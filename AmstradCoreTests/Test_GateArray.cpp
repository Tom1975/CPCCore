
#ifdef _WIN32

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

#if defined (__unix) || (__MORPHOS__) || (__APPLE__) || (RASPPI)
#ifdef MINIMUM_DEPENDENCIES
#else
#define fopen_s(pFile,filename,mode) (((*(pFile))=fopen((filename), (mode))) == NULL)
#include <sys/stat.h>
#define fopen_s(pFile,filename,mode) ((*(pFile))=fopen((filename),(mode)))==NULL
#endif
#endif

#include <iostream>

#include "Motherboard.h"
#include "gtest/gtest.h"

/////////////////////////////////////////////////////////////
// Gate array tests

TEST(GateArray, Clocks)
{
   // Generate clocks. Compare with what's expected
   Motherboard mb;
   mb.Create();

   for (int i = 0; i < 128; i++)
   {
      mb.Tick();
   }

   mb.StartSample();

   // Generate 128 ticks (16 us) for timing
   for (int i = 0; i < 128; i++)
   {
      mb.Tick();
   }
   const auto sp = mb.StopSample();

   // Compare it with what's expected
   ASSERT_EQ(sp, "");
}
