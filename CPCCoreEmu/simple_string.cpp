
#ifdef MINIMUM_DEPENDENCIES

//#include "simple_string.h"
#include <stdlib.h>

std::string::string() : inner_string_(nullptr)
{

}

std::string::string(const char* str)
{
   inner_string_ = new CString(str);
}
std::string::~string(void)
{
   delete inner_string_;
};

const char* std::string::c_str(void) const
{
   return *inner_string_;
}

unsigned int std::string::size() const noexcept { // return length of sequence
   return inner_string_==0?0:inner_string_->GetLength();
}

unsigned int std::string::length() const noexcept { // return length of sequence
   return inner_string_ == 0 ? 0 : inner_string_->GetLength();
}

void std::string::clear()
{
   delete inner_string_;
   inner_string_ = new CString();
}


char& std::string::operator [](unsigned int idx)
{
   const char* ptr = *inner_string_;

   return ((char*)ptr)[idx];
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

int stricmp(char const* _String1, char const* _String2)
{
   return strcasecmp(_String1, _String2);
}

int strnicmp(char const* _String1, char const* _String2, unsigned int _MaxCount)
{
   char* str1 = new char[_MaxCount+1];
   char* str2 = new char[_MaxCount + 1];

   strncpy(str1, _String1, _MaxCount);
   strncpy(str2, _String2, _MaxCount);
   str1[_MaxCount] = 0;
   str2[_MaxCount] = 0;
   int result = strcasecmp(str1, str2);

   delete []str1;
   delete []str2;

   return result;

}

#else

#include <string>

#endif