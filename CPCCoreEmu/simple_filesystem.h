#pragma once

#ifdef MINIMUM_DEPENDENCIES
#include "simple_string.h"

namespace std::filesystem
{
   class path
   {
   public:
      path(const char* path);

      path& operator/=(const char*ext);
      std::string filename() const;
      std::string string() const;

   protected:

      std::string path_;

   };
}

#else

#endif