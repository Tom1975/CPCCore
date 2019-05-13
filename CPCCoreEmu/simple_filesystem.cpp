#pragma once

#ifdef MINIMUM_DEPENDENCIES
#include "simple_string.h"

#define PATH_SLASH '/'

std::filesystem::path::path(const char* path)
{
   path_ = path;
}



std::filesystem::path filename() const
{
   int lg = strlen(path_);
   if (lg == 0) return "";
   lg--;
   for (lg; lg >= 0; lg--)
   {
      if (path_[lg] == PATH_SLASH)
      {
         return std::string(&path_[lg+1]);
      }
   }
}

std::filesystem::path& operator std::filesystem:: /=(const char*ext)
{
   path_.Append(PATH_SLASH);
   path_.Append(ext);
   return *this;
}

std::string std::filesystem::string() const
{ // return path as basic_string<char> native
   return path_;
}


#else

#endif