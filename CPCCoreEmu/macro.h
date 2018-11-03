#pragma once


#ifndef TXT_OUT_PROF
#define TXT_OUT_PROF "Duration"
#endif

#define PROF

#ifdef PROF

#ifdef _WIN32
static __int64 S1, S2, Freq;
#else
static int64_t s1, s2, freq;
#endif

static DWORD T;
static char S [1024];

#define START_CHRONO  QueryPerformanceFrequency((LARGE_INTEGER*)&Freq);;QueryPerformanceCounter ((LARGE_INTEGER*)&S1);
#define STOP_CHRONO   QueryPerformanceCounter ((LARGE_INTEGER*)&S2);t=(DWORD)(((S2 - S1) * 1000000) / Freq);
#define PROF_DISPLAY _stprintf_s(S, 1024, "%s: %d us\n",TXT_OUT_PROF, T);OutputDebugString (S);

#else
   #define START_CHRONO
   #define STOP_CHRONO
   #define PROF_DISPLAY
#endif

#ifdef LOGFDC
   #define LOG(str)  if (log_) log_->WriteLog (str);
   #define LOGEOL if (log_) log_->EndOfLine ();
   #define LOGB(str)  if (log_) log_->WriteLogByte (str);
#else
   #define LOG(str)
   #define LOGB(str)
   #define LOGEOL
#endif
