#pragma once

#ifdef MINIMUM_DEPENDENCIES

float sqrt(float number) {
   long i;
   float x2, y;
   const float threehalfs = 1.5F;

   x2 = number * 0.5F;
   y = number;
   i = *(long*)& y;                     // floating point bit level hacking [sic]
   i = 0x5f3759df - (i >> 1);             // Newton's approximation
   y = *(float*)& i;
   y = y * (threehalfs - (x2 * y * y)); // 1st iteration
   y = y * (threehalfs - (x2 * y * y)); // 2nd iteration
   y = y * (threehalfs - (x2 * y * y)); // 3rd iteration

   return 1 / y;
}

int abs(int _Number)
{
   return (_Number < 0) ? _Number * -1 : _Number;
}

#else

#include <cmath>

#endif