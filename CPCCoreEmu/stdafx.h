#pragma once

// Definition of endianess.
#ifdef __BIG_ENDIAN__
#define BIG_ENDIAN
#endif

// Windows Header Files:
#ifdef _WIN32
// Big endian on windows.
#define BIG_ENDIAN
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#define _CRTDBG_MAP_ALLOC

#include <filesystem>
namespace fs = std::experimental::filesystem;

#elif __MORPHOS__
// fs does not exist there
#else

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#define stricmp strcasecmp
#define strnicmp strncasecmp

#endif

#include <stdlib.h>
#include <stdio.h>


#if defined (__unix) || (__MORPHOS__)
#define fopen_s(pFile,filename,mode) (((*(pFile))=fopen((filename), (mode))) == NULL)
#include <sys/stat.h>
#endif


#if 1

#define OutputDebugString(x)

#define DWORD unsigned int

#ifdef __MORPHOS__
#define MAX_PATH 1024 // There should be no max...
#else
#define MAX_PATH  260
#endif

typedef void* HINSTANCE;
typedef void* HWND;
#else
#include <windows.h>
#include <tchar.h>
#endif

// C RunTime Header Files
#include <malloc.h>
#include <memory.h>

#ifdef _DEBUG
#define Trace //_tprintf
#else
#define Trace //
#endif
