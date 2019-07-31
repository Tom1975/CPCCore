#pragma once

#include "IClockable.h"

class IComponent;

class IMachine
{
public:

   virtual void ForceTick (IComponent*, int ticks = 0) = 0;
};

class IComponent : public IClockable
{
public:

   IComponent () :machine_(0),elapsed_time_(0) { };
   // Tick
   virtual void PreciseTick ( ) {};

   // Breaker (in case of an interrupted tick)
   virtual void SetTickForcer (IMachine* machine){machine_ = machine;};

   // Inner data
public:
   IMachine* machine_;

   int elapsed_time_;
   int this_tick_time_;

};
