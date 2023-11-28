#include "SingleLineSample.h"

#include <iostream>
#include <sstream>

///////////////////////////////////////
// SingleLineSample
SingleLineSample::SingleLineSample(std::string label, BusLine* line) :label_(label), line_(line)
{

}
void SingleLineSample::Clear()
{
   samples_.clear();
}

std::string SingleLineSample::GetSample()
{
   // Format : Wavedrom
   // {name: '28Mhz', wave: '01010101010101010101010101010101'}
   std::ostringstream buffer;
   
   buffer << "{name: '" << label_ << "', wave: '";
   bool first = true, old = true;
   for (auto it: samples_)
   {
      if (first || (old != it))
      {
         buffer << (it ? 'h' : 'l');
      }
      else
      {
         buffer << '.';
      }
      old = it;
      first = false;
   }
   buffer << "'}";

   return buffer.str();
}
