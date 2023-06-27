#include "BusLine.h"

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

void BusLine::ForceLevel(bool level)
{
   up_ = level;
}
