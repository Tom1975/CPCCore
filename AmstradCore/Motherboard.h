#pragma once

#include <string>
#include "simple_vector.hpp"


#include "IComponent.h"
#include "BusLine.h"
#include "GateArray.h"
#include "Z80.h"
#include "CRTC_0.h"
#include "PPI8255.h"
#include "AY8912.h"

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
   // Component access
   GateArray* GetGateArray()
   {
      return &gate_array_;
   }

   ///////////////////////////////////////
   // Inner components lines
   Bus<unsigned short> bus_address_;
   Bus<unsigned char> bus_data_;

   Bus<unsigned char> bus_data_ppi_ay_;

   // Line are alphabetically sorted
   BusLine line_16_mhz_;
   BusLine line_4_mhz_;
   BusLine line_bc1_;
   BusLine line_bc2_;
   BusLine line_bdir_;
   BusLine line_busak_;
   BusLine line_busrq_;
   BusLine line_busy_;
   BusLine line_casad;
   BusLine line_CCLK_mhz_;
   BusLine line_CPU_ADDR_mhz_;
   BusLine line_cursor_;
   BusLine line_dispen_;
   BusLine line_exp_;
   BusLine line_halt_;
   BusLine line_hsync_;
   BusLine line_int_;
   BusLine line_iorq_;
   BusLine line_iord_;
   BusLine line_iowr_;
   BusLine line_lk1_;
   BusLine line_lk2_;
   BusLine line_lk3_;
   BusLine line_lk4_;
   BusLine line_lpen_;
   BusLine line_m1_;
   BusLine line_mreq_;
   BusLine line_motor_;
   BusLine line_n24en_;
   BusLine line_ncas_;
   BusLine line_nm1_;
   BusLine line_ramrd_;
   BusLine line_ras_;
   BusLine line_rd_;
   BusLine line_rd_data_;
   BusLine line_ready_;
   BusLine line_rfsh_;
   BusLine line_reset_;
   BusLine line_romen_;
   BusLine line_sound_;
   BusLine line_vsync_;
   BusLine line_we_;
   BusLine line_wr_;
   BusLine line_wr_data_;


protected:

   ///////////////////////////////////////
   /// Components
   Z80 z80_;
   GateArray gate_array_;
   PPI8255 ppi8255_;
   CRTC_0 crtc0_;
   AY8912 ay8912_;
};
