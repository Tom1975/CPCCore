
#ifdef MINIMUM_DEPENDENCIES

#include "simple_stdio.h"

static CFATFileSystem		file_system;

errno_t fopen_s(
   FILE**      _Stream,
   char const* _FileName,
   char const* _Mode
)
{
   *_Stream = (unsigned int*)file_system.FileOpen(_FileName);
   if (*_Stream == 0)
      return -1;
   else
      return 0;
}

#else

#include <stdio.h>

#endif