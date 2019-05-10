#pragma once

#ifdef MINIMUM_DEPENDENCIES
#include "simple_string.h"

std::filesystem::path::path(const char* path)
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