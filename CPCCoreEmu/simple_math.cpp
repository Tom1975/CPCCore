
//#include "simple_math.h"
#include <stdlib.h>

#ifdef MINIMUM_DEPENDENCIES


double Round(double d)
{
   if (d < 0.0)
      return (int)(d - 0.5);
   else
      return (int)(d + 0.5);
}


float sqrt(float number)
{
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

double Round(double d)
{
   return floor(d + 0.5);
}

#endif