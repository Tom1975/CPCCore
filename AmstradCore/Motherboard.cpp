#include "Motherboard.h"

#include <chrono>
#include <iostream>
#include <sstream>

///////////////////////////////////////
// Bus line
//

///////////////////////////////////////
// Motherboard
//
Motherboard::Motherboard()
{
   line_16_mhz_.AddComponent(&gate_array_);
}

Motherboard::~Motherboard()
= default;

void Motherboard::Create()
{

   gate_array_.CreateGateArray(&line_4_mhz_, &line_CCLK_mhz_, &line_wait_  );

   // Sample process creation
   samples_.emplace_back("16MHz",  & line_16_mhz_);
   samples_.emplace_back("4MHz",  &line_4_mhz_);
   samples_.emplace_back("CCLK" , &line_CCLK_mhz_);
   samples_.emplace_back("Wait", &line_wait_);
   line_wait_.ForceLevel(true);
}

void Motherboard::Reset()
{
   // Set the Reset line.
   // todo
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
   // First sample
   for (auto& it : samples_)
   {
      it.samples_.push_back(it.line_->GetLevel());
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

   result += "],foot:{tock:1}}";

   return result;
}

int main(int argc, char* argv[])
{
   Motherboard mb;
   mb.Create();
   mb.StartSample();

   // Generate 128 ticks (16 us) for timing
   for (int i = 0; i < 128; i++)
   {
      mb.Tick();
   }
   const auto sp = mb.StopSample();

   // Check speed of the simulation
   const auto start = std::chrono::high_resolution_clock::now();

   for (int i = 0; i < 1000000*16; i++)
   {
      // Run 1 second
      mb.Tick();
   }
   const auto elapsed = std::chrono::high_resolution_clock::now() - start;

   const long long microseconds = std::chrono::duration_cast<std::chrono::microseconds>(
      elapsed).count();

   std::cout << "Elapsed time for 1000000 us : " << microseconds << " - Speed is " << ((1000000.0/ static_cast<double>(microseconds))*100.0)<< "%"<<std::endl;

   std::cout << sp << std::endl;

   return 0;
}