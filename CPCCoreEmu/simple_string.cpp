
#ifdef MINIMUM_DEPENDENCIES

#include <circle/logger.h>

#include "simple_string.h"
#include <stdlib.h>

std::string::string()
{
   inner_string_ = new CString();
}

std::string::string (const string& str)
{
   inner_string_ = new CString(str.c_str());
}

std::string::string(const char* str)
{
   inner_string_ = new CString(str);
}
std::string::~string(void)
{
   delete inner_string_;
};

std::string::operator const char *(void) const
{
	return *inner_string_;
}

const char *std::string::operator = (const char *pString)
{
   *inner_string_ = pString;
   return *this;
}

const std::string &std::string::operator = (const std::string &rString)
{
   *inner_string_ = rString;
   return *this;
}


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

std::string& std::string::append (const char* s)
{
   inner_string_->Append(s);
   return *this;
}

std::string std::string::substr (size_t pos, size_t len ) const
{
   
   size_t size_substr = 0;
   if ( len != npos )
   {
      size_substr = len + 1;
   }
   else
   {
      size_substr = length() - pos + 1;
   }
   char* sub_str = new char[size_substr];
   const char* ptr = *inner_string_;
   memset (sub_str, 0, size_substr);
   strncpy ( sub_str, &ptr[pos], size_substr-1);
   std::string substring (sub_str);
   delete []sub_str;
   return substring;
}

std::string&  std::string::erase (size_t pos, size_t len)
{
   size_t lenght_substring = 0;
   if ( pos != std::string::npos && pos < inner_string_->GetLength())
      lenght_substring = len<(inner_string_->GetLength()-pos)?len:(inner_string_->GetLength()-pos);

   char* buffer = new char[lenght_substring+1];
   memset (buffer, 0, lenght_substring);
   const char* ptr = *inner_string_;
   strncpy ( buffer, &ptr[pos], lenght_substring);
   *inner_string_ = buffer;
   return *this;
}

int std::string::compare (const char* s) const
{
   return strcmp (s, *inner_string_);
}

size_t std::string::find (char c, size_t pos ) const
{
   if ( inner_string_ == nullptr || pos >= inner_string_->GetLength())
      return npos;

   const char* ptr = *inner_string_;
   size_t val = npos;
   int count = pos;
   while (ptr[count] != '\0' && val == npos)
   {
      if ( ptr[count] == c)
         val = count;
      count ++;
   }
   return val;
}

size_t std::string::find_last_not_of (const char* s, size_t pos ) const
{
   size_t val = npos;
   if ( inner_string_ == nullptr )
      return npos;

   if ( pos >= inner_string_->GetLength() || pos == npos)
      pos = inner_string_->GetLength();

   const char* ptr = *inner_string_;
   int count = pos - 1;
   while (count >= 0 && val == npos)
   {
      bool car_found = false;
      for (size_t i = 0; i < strlen(s) && (car_found == false); i++)
      {
         if ( ptr[count] == s[i])
            car_found = true;
      }
      if (!car_found)
      {
         if ( count < pos-1)
            val = count;
         else
            val = npos;

         return val;
      }
      count --;
   }
   return val;   
   
}

size_t std::string::find_first_not_of (const char* s, size_t pos ) const
{
   size_t val = npos;
   if ( inner_string_ == nullptr || pos >= inner_string_->GetLength())
      return npos;

   const char* ptr = *inner_string_;
   int count = pos;
   while (ptr[count] != '\0' && val == npos)
   {
      bool car_found = false;
      for (size_t i = 0; i < strlen(s) && (car_found == false); i++)
      {
         if ( ptr[count] == s[i])
            car_found = true;
      }
      if (!car_found)
      {
         val = count ;
      }
      count ++;
   }
   return val;
}

/*
int sprintf(char* buf, const char* fmt, ...)
{
   va_list var;
   va_start(var, fmt);

   CString Msg;
   Msg.FormatV(fmt, var);

   va_end(var);

   strcpy(buf, (const char*)Msg);

   return Msg.GetLength();
}*/

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