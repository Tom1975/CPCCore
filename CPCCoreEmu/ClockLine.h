#pragma once

///////////////////////////////////////////////////////////////////
// Clock Line implementation
///////////////////////////////////////////////////////////////////


#include "IComponent.h"
#include "IClockable.h"

///////
class CClockLine : public IClockable
{
public:
   CClockLine(void);
   virtual ~CClockLine(void);

   // Set components
   void InitLine ( int nb_components = 1 );
   void AddComponent (IClockable* new_component);

   // Tick
   virtual unsigned int Tick (); // Inversion of the clock !

protected:
   // Signal state
   bool signal_up_;

   // Component connected to this line
   IClockable** component_array_;
   unsigned int nb_components_;
   unsigned int size_of_component_array_;

};

