#pragma once

#include <string>
#include <vector>

#include "IComponent.h"
#include "BusLine.h"

class GateArray : public IComponent
{
public:
   GateArray();
   virtual ~GateArray();

   void Create();

   void Reset();
   void TickUp() override;
   void TickDown() override;

   // Various lines
   DECLARE_REFERENCE_LINE(line_4_mhz_);
   DECLARE_REFERENCE_LINE(line_CCLK_mhz_);
   DECLARE_REFERENCE_LINE(line_CPU_ADDR_mhz_);

   DECLARE_REFERENCE_LINE(line_ready_);
   DECLARE_REFERENCE_LINE(line_reset_);
   DECLARE_REFERENCE_LINE(line_int_);
   DECLARE_REFERENCE_LINE(line_hsync_);
   DECLARE_REFERENCE_LINE(line_vsync_);
   DECLARE_REFERENCE_LINE(line_dispen_);
   DECLARE_REFERENCE_LINE(line_cpu_addr_);
   DECLARE_REFERENCE_LINE(line_m1_);
   DECLARE_REFERENCE_LINE(line_iorq_);
   DECLARE_REFERENCE_LINE(line_rd_);

   DECLARE_REFERENCE_BUS_ADDRESS(bus_address_);
   DECLARE_REFERENCE_BUS_DATA(bus_data_);

protected:

   void Sequencer();
   void SequencerDecode();

   unsigned char s_;
   unsigned int counter;

   unsigned char u312_, u313_;




};

