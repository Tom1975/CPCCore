#pragma once

#ifdef MINIMUM_DEPENDENCIES

#ifdef __cplusplus
extern "C" {
#endif

double Round(double d);
float sqrt(float number);
//int abs(int _Number);

   #ifdef __cplusplus
}
#endif
#include <stdlib.h>

#else

#include <cmath>

double Round(double d);

#endif