
#ifdef __circle__

#include "simple_stdio.h"

static CFATFileSystem		file_system;

class File
{
public:
   File()
   {

   }
   virtual ~File()
   {

   }
protected:
   unsigned int handle_;
};

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

int fclose(FILE* _Stream)
{
   file_system.FileClose(*_Stream);
   delete _Stream;
   return 0;
}

int fseek(FILE* _Stream, long  _Offset, int   _Origin)
{
   switch (_Origin)
   {
   default:
      break;
   }
}
void rewind( FILE* _Stream) 
{
}

long ftell(FILE* _Stream)
{
   return 0;
}

size_t fread(
   void* _Buffer,
   size_t _ElementSize,
   size_t _ElementCount,
   FILE* _Stream
)
{
   unsigned int handle = *_Stream;
   return (file_system.FileRead(handle, _Buffer, _ElementSize * _ElementCount) / _ElementSize);
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