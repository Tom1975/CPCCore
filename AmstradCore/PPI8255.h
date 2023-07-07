#pragma once

#include "BusLine.h"

class PPI8255 
{
public:
   PPI8255();
   virtual ~PPI8255();

   void Create();
   void Reset();

   /////////////////////////////////////
   // Various lines, in order of the ship (begining top, clockwork)
   DECLARE_REFERENCE_BUS_DATA(bus_data_ppi_ay_);   // A0-7 (Out)

   DECLARE_REFERENCE_LINE(line_bdir_);             // C7
   DECLARE_REFERENCE_LINE(line_bc1_);              // C6
   DECLARE_REFERENCE_LINE(line_wr_data_);          // C5 : WR DATA (out)
   DECLARE_REFERENCE_LINE(line_motor_);       // C4 - Motor 
   // C3 - D ( keyboard connector)
   // C2 - C
   // C1 - B
   // C0 - A

   DECLARE_REFERENCE_LINE(line_rd_data_);          // B7 - RD DATA
   DECLARE_REFERENCE_LINE(line_busy_);             // B6 - BUSY
   DECLARE_REFERENCE_LINE(line_exp_);              // B5 - EXP
   DECLARE_REFERENCE_LINE(line_lk1_);              // B4 - LK4
   DECLARE_REFERENCE_LINE(line_lk2_);              // B3 - LK3
   DECLARE_REFERENCE_LINE(line_lk3_);              // B2 - LK2
   DECLARE_REFERENCE_LINE(line_lk4_);              // B1 - LK1
   DECLARE_REFERENCE_LINE(line_vsync_);            // B0 - VSYNC IN

   DECLARE_REFERENCE_LINE(line_reset_);            // RESET

   DECLARE_REFERENCE_LINE(line_iowr_);            // WR - IOWR
   DECLARE_REFERENCE_LINE(line_iord_);            // RD - IORD
   // A8 - A8
   // A9 - A9 
   // CS - A11
   DECLARE_REFERENCE_BUS_DATA(bus_data_);          // Data (D0-D7)
   // GND
   // VCC


protected:


};

