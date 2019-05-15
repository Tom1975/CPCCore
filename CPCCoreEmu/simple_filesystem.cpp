
#include "simple_filesystem.h"

#ifdef MINIMUM_DEPENDENCIES

#define PATH_SLASH '/'
#define PATH_SLASH_STRING "/"

namespace std::filesystem
{

   path::path(const char* path)
   {
      path_ = path;
   }



   std::string path::filename() const
   {
      int lg = path_.length();
      if (lg == 0) return "";
      lg--;
      for (lg; lg >= 0; lg--)
      {
         if (path_[lg] == PATH_SLASH)
         {
            return std::string(&path_[lg + 1]);
         }
      }
   }

   path& path::operator /=(const char*ext)
   {
      path_.Append(PATH_SLASH_STRING);
      path_.Append(ext);
      return *this;
   }

   std::string path::string() const
   { // return path as basic_string<char> native
      return path_;
   }

}

#else

#endif