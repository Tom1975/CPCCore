#include "stdafx.h"
#include "ClockLine.h"


CClockLine::CClockLine(void)
{
   nb_components_ = 0;
   component_array_ = NULL;
   size_of_component_array_ = 0;
}


CClockLine::~CClockLine(void)
{
   delete[]component_array_;
}

void CClockLine::InitLine ( int nbComponents )
{
   if ( component_array_ != NULL)
   {
      delete []component_array_;
   }

   nb_components_ = 0;
   if (nbComponents == 0)
      size_of_component_array_  = 1;
   else
      size_of_component_array_ = nbComponents;

   component_array_ = new IClockable*[size_of_component_array_];
}

void CClockLine::AddComponent (IClockable* new_component)
{
   if ( nb_components_ >= size_of_component_array_)
   {
      IClockable** tmp_clockable_list = new IClockable*[(size_of_component_array_!=0)?(size_of_component_array_*2):1];

      if ( component_array_ != NULL)
      {
         memcpy (tmp_clockable_list, component_array_, sizeof (IClockable*)*size_of_component_array_);
         delete []component_array_;
      }

      component_array_ = tmp_clockable_list;
      if (size_of_component_array_!=0)
         size_of_component_array_ *= 2;
      else
         size_of_component_array_ = 1;
   }

   component_array_[nb_components_++] = new_component;
}

unsigned int CClockLine::Tick ()
{
   signal_up_ = !signal_up_;

   // What is attach to this line ?
   for (unsigned int i = 0; i < nb_components_; i++)
   {
      component_array_[i]->Tick ();
   }
   return 1;
}
