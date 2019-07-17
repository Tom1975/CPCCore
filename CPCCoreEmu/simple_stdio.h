#pragma once


#ifdef MINIMUM_DEPENDENCIES

#include "circle/fs/fat/fatfs.h"
#include <stdio.h>

#define SEEK_CUR    1
#define SEEK_END    2
#define SEEK_SET    0

typedef int          errno_t;
//typedef unsigned int FILE;

int fclose(FILE* _Stream );

errno_t fopen_s(
   FILE**      _Stream,
   char const* _FileName,
   char const* _Mode
);

//int fseek(FILE* _Stream, long  _Offset,int   _Origin);

//void rewind(FILE* _Stream);
//long ftell(FILE* _Stream);
/*
size_t fread(
   void* _Buffer,
   size_t _ElementSize,
   size_t _ElementCount,
   FILE* _Stream
);

unsigned int fwrite(
   void const* _Buffer,
   unsigned int _ElementSize,
   unsigned int _ElementCount,
   FILE* _Stream
);*/

#else

#include <stdio.h>

#endif