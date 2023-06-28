#pragma once

#include <string>
#include <vector>

#include "IComponent.h"
#include "BusLine.h"

class Z80 : public IComponent
{
public:
   Z80();
   virtual ~Z80();

   void Create();

   void Reset();
   void TickUp() override;
   void TickDown() override;

   // Various lines
   DECLARE_REFERENCE_LINE(line_4_mhz_);
   DECLARE_REFERENCE_LINE(line_ready_);
   DECLARE_REFERENCE_LINE(line_reset_);
   DECLARE_REFERENCE_LINE(line_int_);

   DECLARE_REFERENCE_BUS_ADDRESS(bus_address_);
   DECLARE_REFERENCE_BUS_DATA(bus_data_);

protected:
   unsigned int counter;
};

