#include "Motherboard.h"

#include <iostream>
#include <sstream>

///////////////////////////////////////
// Bus line
//
BusLine::BusLine() : up_(false)
{
   
}

BusLine::~BusLine()
{
   
}

void BusLine::AddComponent(IComponent* component)
{
   component_list_.push_back(component);
}

bool BusLine::GetLevel()
{
   return up_;
}

void BusLine::Tick()
{
   up_ = !up_;
   for (auto& it : component_list_)
   {
      it->Tick();
   }
}

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
   // {name: '28Mhz', wave: '01010101010101010101010101010101'}
   std::ostringstream buffer;
   
   buffer << "{name: '" << label_ << "', wave: '";
   bool first = true, old = true;
   for (auto it: samples_)
   {
      if (first || (old != it))
      {
         buffer << (it ? '1' : '0');
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

///////////////////////////////////////
// GateArray
//
GateArray::GateArray() : counter(0), line_4_mhz_(nullptr), line_1_mhz_(nullptr)
{
   
}
GateArray::~GateArray()
{
   
}

void GateArray::CreateGateArray(BusLine* line_4, BusLine* line_1)
{
   line_4_mhz_ = line_4;
   line_1_mhz_ = line_1;
}

void GateArray::Tick()
{
   ++counter;
   if ((counter & 0x3) == 0) line_4_mhz_->Tick();
   if ((counter & 0xF) == 0) line_1_mhz_->Tick();
}

///////////////////////////////////////
// Motherboard
//
Motherboard::Motherboard()
{
   line_16_mhz_.AddComponent(&gate_array_);
}

Motherboard::~Motherboard()
{

}

void Motherboard::Create()
{

   gate_array_.CreateGateArray(&line_4_mhz_, &line_1_mhz_);

   // Sample process creation
   samples_.push_back(SingleLineSample ( "16 Mhz",  & line_16_mhz_));
   samples_.push_back(SingleLineSample ("4 Mhz",  &line_4_mhz_));
   samples_.push_back(SingleLineSample ("1 Mhz" , &line_1_mhz_));
}

void Motherboard::Tick()
{
   line_16_mhz_.Tick();

   if (sample_)
   {
      // Sample the whole lines
      for (auto& it : samples_)
      {
         it.samples_.push_back(it.line_->GetLevel());
      }
   }
}

void Motherboard::StartSample()
{
   sample_ = true;
   for (auto& it : samples_)
   {
      it.Clear();
   }
}

std::string Motherboard::StopSample()
{
   sample_ = false;

   std::string result = "{signal: [";

   for (auto& it : samples_)
   {
      result += it.GetSample();
      result += ",";
   }

   result += "]}";

   return result;
}

int main(int argc, char* argv[])
{
   Motherboard mb;
   mb.Create();
   mb.StartSample();

   for (int i = 0; i < 128; i++)
   {
      mb.Tick();
   }
   auto sp = mb.StopSample();

   std::cout << sp << std::endl;

   return 0;
}