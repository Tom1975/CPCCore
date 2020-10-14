#pragma once

// Definition of endianess.
#ifdef __BIG_ENDIAN__
#define BIG_ENDIAN
#endif

#ifdef _MACOS_
#define NOFILTER
#define NOZLIB
#define MINIMUM_DEPENDENCIES
#define NO_MULTITHREAD
#define NO_RAW_FORMAT
#define LOG_MIXER 
#define LOGFDC
#endif

// Windows Header Files:
#ifdef _WIN32
// Big endian on windows.
#define BIG_ENDIAN
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#define _CRTDBG_MAP_ALLOC

#include <filesystem>
namespace fs = std::filesystem;

#elif __MORPHOS__
// fs does not exist there

#elif __circle__
#include "simple_filesystem.h"
// fs does not exist there
namespace fs = std::filesystem;
#else

// fs does not exist there
#include <filesystem>
// Only for C++17 !
namespace fs = std::filesystem;

#define stricmp strcasecmp
#define strnicmp strncasecmp

#endif

#ifdef __circle__
typedef unsigned int FILE;
//#include <stdio.h>

#else
#include <stdlib.h>
#include <stdio.h>
#endif

/*#if defined (__unix) || (__MORPHOS__) || (__APPLE__) || (RASPPI)
#ifdef MINIMUM_DEPENDENCIES
#else
#define fopen_s(pFile,filename,mode) (((*(pFile))=fopen((filename), (mode))) == NULL)
#include <sys/stat.h>
#define fopen_s(pFile,filename,mode) ((*(pFile))=fopen((filename),(mode)))==NULL
#endif
#endif*/


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
#include <memory.h>

#ifdef _DEBUG
#define Trace //_tprintf
#else
#define Trace //
#endif
