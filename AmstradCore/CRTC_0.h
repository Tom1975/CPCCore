#pragma once

#include "IComponent.h"
#include "BusLine.h"

class CRTC_0 : public IComponent
{
public:
   CRTC_0();
   virtual ~CRTC_0();

   void Create();

   void Reset();
   void TickUp() override;
   void TickDown() override;

   /////////////////////////////////////
   // Various lines, in order of the ship (begining top, clockwork)
   // VCC, GND
   
   // CS - A14
   // R/W - A9
   // RS - A8
   DECLARE_REFERENCE_BUS_DATA(bus_data_);       // D0-7
   DECLARE_REFERENCE_LINE(line_reset_);         // RESET
   DECLARE_REFERENCE_LINE(line_lpen_);          // LPEN
   DECLARE_REFERENCE_LINE(line_cursor_);        // CUREN
   // EN - ??
   DECLARE_REFERENCE_LINE(line_dispen_);        // DISPEN
   DECLARE_REFERENCE_LINE(line_hsync_);         // HSYNC
   DECLARE_REFERENCE_LINE(line_vsync_);         // VSYNC
   DECLARE_REFERENCE_LINE(line_CCLK_mhz_);      // CCLK
   // MA0-13 / RA0-4

protected:



};

