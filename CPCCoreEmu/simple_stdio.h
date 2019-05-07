#pragma once

#ifdef MINIMUM_DEPENDENCIES

#include "circle/fs/fat/fatfs.h"

typedef int          errno_t;
typedef unsigned int FILE;

errno_t fopen_s(
   FILE**      _Stream,
   char const* _FileName,
   char const* _Mode
);

#else

#include <stdio.h>

#endif