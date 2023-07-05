#include "Sampler.h"

#include <chrono>
#include <iostream>
#include <sstream>

///////////////////////////////////////
// Bus line
//

///////////////////////////////////////
// Sampler
//

Sampler::Sampler()
{
}

Sampler::~Sampler()
= default;

void Sampler::AddLineToSample(std::string name, BusLine* line)
{
   samples_.emplace_back(name, line);
}

void Sampler::Clear()
{
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

void Sampler::AddSample()
{
   // Sample the whole lines
   for (auto& it : samples_)
   {
      it.samples_.push_back(it.line_->GetLevel());
   }
}

std::string Sampler::GetWaves()
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