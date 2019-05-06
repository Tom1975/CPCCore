#pragma once

#ifdef MINIMUM_DEPENDENCIES

#include "circle/string.h"

namespace std
{
   class string : public CString
   {
   public:
      string(void) : inner_string_()
      {

      }

      string(const char* str) : inner_string_(str)
      {

      }
      virtual ~string(void) 
      { };

      const char* c_str(void) const
      {
         return inner_string_;
      }
   protected:
      CString inner_string_;
   };
}


#else

#include <string>

#endif