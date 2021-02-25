#pragma once


#ifdef __circle__

#define SEEK_CUR    1
#define SEEK_END    2
#define SEEK_SET    0

typedef int          errno_t;
typedef void         FILE;

int fclose(FILE* _Stream );

errno_t fopen_s(
   FILE**      _Stream,
   char const* _FileName,
   char const* _Mode
);

int fseek(FILE* _Stream, long  _Offset,int   _Origin);

void rewind(FILE* _Stream);
long ftell(FILE* _Stream);

unsigned int fread(
   void* _Buffer,
   unsigned int _ElementSize,
   unsigned int _ElementCount,
   FILE* _Stream
);

unsigned int fwrite(
   void const* _Buffer,
   unsigned int _ElementSize,
   unsigned int _ElementCount,
   FILE* _Stream
);

#else

#include <stdio.h>

#ifdef __unix
#define fopen_s(pFile,filename,mode) ((*(pFile))=fopen((filename),  (mode)))==NULL
#endif

#endif