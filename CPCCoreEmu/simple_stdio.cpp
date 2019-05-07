#pragma once

#ifdef MINIMUM_DEPENDENCIES

#include "circle/fs/fat/fatfs.h"

static CFATFileSystem		file_system;

errno_t fopen_s(
   FILE**      _Stream,
   char const* _FileName,
   char const* _Mode
)
{
   *_Stream = static_cast<unsigned int*>(file_system.FileOpen(_FileName));
   if (*_Stream == 0)
      return -1;
   else
      return 0;
}

#else

#include <stdio.h>

#endif