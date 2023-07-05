
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

#include "Sampler.h"
#include "Motherboard.h"
#include "gtest/gtest.h"

namespace GATE_ARRAY
{
   /////////////////////////////////////////////////////////////
   // Check the various clocks & signals (16mhz, 4mhz, cclk, cpu_addr, wait )
   TEST(GateArray, Clocks)
   {
      // Generate clocks. Compare with what's expected
      Motherboard mb;
      Sampler sampler;
      mb.Create();

      for (int i = 0; i < 128; i++)
      {
         mb.Tick();
      }
      // Create Samples


   ///////////////////////////
   // Sample process creation
      sampler.AddLineToSample("16MHz", &mb.line_16_mhz_);
      sampler.AddLineToSample("4MHz", &mb.line_4_mhz_);
      sampler.AddLineToSample("CCLK", &mb.line_CCLK_mhz_);
      sampler.AddLineToSample("CPU_ADDR", &mb.line_CPU_ADDR_mhz_);

      sampler.AddLineToSample("Wait", &mb.line_ready_);

      sampler.AddLineToSample("Int", &mb.line_int_);
      sampler.AddLineToSample("Reset", &mb.line_reset_);

      sampler.AddLineToSample("HSync", &mb.line_hsync_);
      sampler.AddLineToSample("VSync", &mb.line_vsync_);
      sampler.AddLineToSample("DispEn", &mb.line_dispen_);



      // Generate 128 ticks (16 us) for timing
      for (int i = 0; i < 128; i++)
      {
         mb.Tick();
         sampler.AddSample();
      }
      const auto sp = sampler.GetWaves();

      // Compare it with what's expected
      ASSERT_EQ(sp, "{signal: [{name: '16MHz', wave: 'hlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhlhl'},{name: '4MHz', wave: 'h...l...h...l...h...l...h...l...h...l...h...l...h...l...h...l...h...l...h...l...h...l...h...l...h...l...h...l...h...l...h...l...'},{name: 'CCLK', wave: 'l.................h.........l.....................h.........l.....................h.........l.....................h.........l...'},{name: 'CPU_ADDR', wave: 'l.....h...................l...........h...................l...........h...................l...........h...................l.....'},{name: 'Wait', wave: 'h......l......................h........l......................h........l......................h........l......................h.'},{name: 'Int', wave: 'l...............................................................................................................................'},{name: 'Reset', wave: 'l...............................................................................................................................'},{name: 'HSync', wave: 'l...............................................................................................................................'},{name: 'VSync', wave: 'l...............................................................................................................................'},{name: 'DispEn', wave: 'l...............................................................................................................................'},],foot:{tock:1}}");
   }

   /////////////////////////////////////////////////////////////
   // Check the sequencer
   TEST(Sequencer, Evolution)
   {
      Motherboard mb;
      mb.Create();
      GateArray* ga = mb.GetGateArray();

      // Initial condition :
      for (int i = 0; i < 16; i++)
      {
         ga->TickUp();
         if ( ga->GetS() == 0xFF)
         {
            break;
         }
      }

      if (ga->GetS() != 0xFF)
      {
         FAIL() << "Cannot set S to 0 !" << std::endl;
      }

      const unsigned char value_expected[] = {0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80, 0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F };
      for (unsigned char i : value_expected)
      {
         ASSERT_EQ(i, ga->GetS());
         ga->TickUp();
      }

   }

}