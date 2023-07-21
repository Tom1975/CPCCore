
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

namespace TEST_Z80
{
   /////////////////////////////////////////////////////////////
   // Check 
   TEST(Z80, Clocks)
   {
      Motherboard mb;
      Sampler sampler;
      mb.Create();

      // Check fetch sample
      sampler.AddLineToSample("4MHz", &mb.line_4_mhz_);
      sampler.AddLineToSample("Wait", &mb.line_ready_);


   }
}

