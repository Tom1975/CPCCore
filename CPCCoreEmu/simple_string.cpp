
#ifdef MINIMUM_DEPENDENCIES

#include "simple_string.h"

std::string::string() : inner_string_()
{

}

std::string::string(const char* str) : inner_string_(str)
{

}
std::string::~string(void)
{ };

const char* std::string::c_str(void) const
{
   return inner_string_;
}

int sprintf(char* buf, const char* fmt, ...)
{
   va_list var;
   va_start(var, fmt);

   CString Msg;
   Msg.FormatV(fmt, var);

   va_end(var);

   strcpy(buf, (const char*)Msg);

   return Msg.GetLength();
}

#else

#include <string>

#endif