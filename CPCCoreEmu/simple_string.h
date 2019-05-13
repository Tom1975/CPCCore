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

      unsigned int size() const noexcept;
      void clear();
      const char* c_str(void) const;
   protected:
      CString *inner_string_;
   };
}
int sprintf(char* buf, const char* fmt, ...);

int stricmp(char const* _String1, char const* _String2);
int strnicmp(char const* _String1, char const* _String2, unsigned int _MaxCount);


#else

#include <string>

#endif