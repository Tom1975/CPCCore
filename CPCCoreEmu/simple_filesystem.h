#pragma once

#ifdef MINIMUM_DEPENDENCIES
#include "simple_string.h"

namespace std::filesystem
{
   class path
   {
   public:
      path(const char* path)
      {
         path_ = path;
      }

      path& operator/=(const char*ext) {
         path_.Append("//");
         path_.Append(ext);
         return *this;
      }

      std::string string() const { // return path as basic_string<char> native
         return path_;
      }

   protected:
      std::string path_;

   };
}

#else

#endif