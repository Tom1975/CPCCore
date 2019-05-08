#pragma once

#ifdef MINIMUM_DEPENDENCIES

#include "circle/util.h"
#include "circle/string.h"
#include "circle/stdarg.h"

namespace std
{
   class string : public CString
   {
   public:
      string(void);
      string(const char* str);
      virtual ~string(void);

      const char* c_str(void) const;
   protected:
      CString inner_string_;
   };
}
int sprintf(char* buf, const char* fmt, ...);
   
#else

#include <string>

#endif