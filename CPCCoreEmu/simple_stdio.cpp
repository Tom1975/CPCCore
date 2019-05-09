
#ifdef MINIMUM_DEPENDENCIES

#include "simple_stdio.h"

static CFATFileSystem		file_system;

errno_t fopen_s(
   FILE**      _Stream,
   char const* _FileName,
   char const* _Mode
)
{
   unsigned int* new_handle = new unsigned int;
   *new_handle = file_system.FileOpen(_FileName);
   *_Stream = new_handle;
   if (*_Stream == 0)
      return -1;
   else
      return 0;
}
unsigned int fwrite(
   void const* _Buffer,
   unsigned int _ElementSize,
   unsigned int _ElementCount,
   FILE* _Stream
)
{
   unsigned int handle = *_Stream;
   return (file_system.FileWrite(handle, _Buffer, _ElementSize * _ElementCount)/ _ElementSize);
}

#else

#include <stdio.h>

#endif