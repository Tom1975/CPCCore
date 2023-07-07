#pragma once

#include "IComponent.h"
#include "BusLine.h"

class AY8912 : public IComponent
{
public:
   AY8912();
   virtual ~AY8912();

   void Create();

   void Reset();
   void TickUp() override;
   void TickDown() override;

   /////////////////////////////////////
   // Various lines, in order of the ship (begining top, clockwork)
   // VCC, GND, A8- => Fixed

   DECLARE_REFERENCE_LINE(line_CPU_ADDR_mhz_);     // CLOCK
   DECLARE_REFERENCE_LINE(line_reset_);            // RESET
   // A, B, C : Sound output

   // IO0 - IO7 : Keyboard
   // Test : not plugged
   DECLARE_REFERENCE_LINE(line_bc1_);              // BC1
   DECLARE_REFERENCE_LINE(line_bdir_);             // BDIR
   // BC2 - +5V
   DECLARE_REFERENCE_BUS_DATA(bus_data_ppi_ay_);   // D0-7


protected:



};

