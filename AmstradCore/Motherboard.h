#pragma once

#include <string>
#include <vector>

#include "IComponent.h"
#include "BusLine.h"
#include "GateArray.h"
#include "Z80.h"

#include "SingleLineSample.h"


class Motherboard
{
public:

   ///////////////////////////////////////
   // CTor / DTor
   Motherboard();
   virtual ~Motherboard();

   ///////////////////////////////////////
   // Creation
   void Create();

   ///////////////////////////////////////
   // Reset
   void Reset();

   ///////////////////////////////////////
   // Run
   void Tick();

   ///////////////////////////////////////
   // Sample
   void StartSample();
   std::string StopSample();

   ///////////////////////////////////////
   // Component access
   GateArray* GetGateArray()
   {
      return &gate_array_;
   }

protected:
   ///////////////////////////////////////
   // Sample
   bool sample_{};

   std::vector<SingleLineSample> samples_;

   ///////////////////////////////////////
   // Inner components lines
   Bus<unsigned short> bus_address_;
   Bus<unsigned char> bus_data_;
   
   BusLine line_16_mhz_;
   BusLine line_4_mhz_;
   BusLine line_CCLK_mhz_;
   BusLine line_CPU_ADDR_mhz_;
   BusLine line_ready_;
   BusLine line_int_;
   BusLine line_reset_;

   BusLine line_busrq_;
   BusLine line_nm1_;
   BusLine line_busak_;
   BusLine line_halt_;
   BusLine line_mreq_;
   BusLine line_m1_;
   BusLine line_rfsh_;
   BusLine line_rd_;
   BusLine line_wr_;
   BusLine line_iorq_;

   BusLine line_exp_;
   BusLine line_lk1_;
   BusLine line_lk2_;
   BusLine line_lk3_;
   BusLine line_lk4_;
   BusLine line_busy_;

   BusLine line_bc1_;
   BusLine line_bc2_;
   BusLine line_bdir_;

   BusLine line_hsync_;
   BusLine line_vsync_;
   BusLine line_dispen_;

   // Drive output
   BusLine line_motor_;
   BusLine line_sound_;
   BusLine line_rd_data_;
   BusLine line_wr_data_;
   
   ///////////////////////////////////////
   /// Components
   Z80 z80_;
   GateArray gate_array_;
};
