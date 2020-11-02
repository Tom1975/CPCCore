#pragma once

#ifdef MINIMUM_DEPENDENCIES

#include "circle/util.h"
#include "circle/string.h"
#include "circle/stdarg.h"
#include <stdlib.h>

namespace std
{
   
   class string //: public CString
   {
   public:
      typedef size_t size_type ;

      string(void);
      string(const char* str);
      string (const string& str);
      virtual ~string(void);

      operator const char *(void) const;
      const char *operator = (const char *pString);
      const string &operator = (const string &rString);

      string& append (const char* s);
      unsigned int length() const noexcept;
      unsigned int size() const noexcept;
      void clear();
      const char* c_str(void) const;
      char& operator [](const unsigned int);
      int compare (const char* s) const;
      size_t find ( char c, size_t pos = 0) const;
      size_t find_first_of (const char* s, size_t pos = 0) const;
      size_t find_first_not_of (const char* s, size_t pos = 0) const;
      size_t find_last_not_of (const char* s, size_t pos = 0) const;

      string& erase (size_t pos = 0, size_t len = npos);
      string substr (size_t pos = 0, size_t len = npos) const;

      static const size_t npos = -1;   

   protected:
      CString *inner_string_;
   };
}
#ifdef __cplusplus
extern "C" {
#endif

int sprintf(char* buf, const char* fmt, ...);

int stricmp(char const* _String1, char const* _String2);
int strnicmp(char const* _String1, char const* _String2, unsigned int _MaxCount);

#ifdef __cplusplus
}
#endif

#else

#include <string>

#endif