
#include "simple_regex.h"

#ifdef MINIMUM_DEPENDENCIES
#include "simple_string.h"

bool IsExtensionMatch(const char* str, const char* ext)
{

}

#else

#include <regex>

bool IsExtensionMatch(const char* str, const char* ext)
{
   std::string extension_filter (".*\\");
   extension_filter += ext;

   const std::regex my_filter(extension_filter);

   return std::regex_match( std::string(str), my_filter);
}


#endif