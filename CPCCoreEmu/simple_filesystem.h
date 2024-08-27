#pragma once

#if defined (MINIMUM_DEPENDENCIES) ||defined (TEST_VECTOR)
#include "simple_string.h"

namespace std::filesystem
{
   class path
   {
   public:
      path(const char* path);
      path(const std::string& path);

      path& operator/=(const char*ext);

      unsigned int operator==(const std::filesystem::path&);

      path filename() ;
      std::string string() const;

   protected:

      std::string path_;

   };
}

#else
#include <filesystem>

#endif