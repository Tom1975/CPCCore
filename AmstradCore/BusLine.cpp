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

unsigned char BusLine::GetLevel()
{
   return up_;
}

void BusLine::Tick()
{
   up_ = !up_;
   for (auto& it : component_list_)
   {
      up_?(it->TickUp()): (it->TickDown());
   }
}

void BusLine::ForceLevel(unsigned char level)
{
   up_ = level;
}
