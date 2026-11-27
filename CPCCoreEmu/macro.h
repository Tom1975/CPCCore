#pragma once

#ifdef LOGFDC
   #define LOG(str)  if (log_) log_->WriteLog (str);
   #define LOGEOL if (log_) log_->EndOfLine ();
   #define LOGB(str)  if (log_) log_->WriteLogByte (str);
#else
   #define LOG(str)
   #define LOGB(str)
   #define LOGEOL
#endif
