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

   /////////////////////////////////////
   // Various lines, in order of the ship (begining top, clockwork)
   DECLARE_REFERENCE_LINE(line_reset_);            // RESET 
   DECLARE_REFERENCE_LINE(line_busrq_);            // BUSRQ    - In
   DECLARE_REFERENCE_LINE(line_int_);              // INT      - In
   DECLARE_REFERENCE_LINE(line_nm1_);              // NM1      - In
   DECLARE_REFERENCE_LINE(line_ready_);            // READY    - In
   DECLARE_REFERENCE_LINE(line_busak_);           // BUSAK    - Out
   DECLARE_REFERENCE_LINE(line_halt_);             // HALT     - Out
   DECLARE_REFERENCE_LINE(line_mreq_);             // NMREQ    - Out
   DECLARE_REFERENCE_LINE(line_m1_);               // NM1      - Out
   DECLARE_REFERENCE_LINE(line_rfsh_);          // RFSH     - Out
   DECLARE_REFERENCE_LINE(line_4_mhz_);            // PHI      - In
   DECLARE_REFERENCE_LINE(line_iorq_);             // NIORQ    - Out
   DECLARE_REFERENCE_LINE(line_wr_);               // WR       - Out
   DECLARE_REFERENCE_LINE(line_rd_);               // RD       - Out
   // GND, VCC
   DECLARE_REFERENCE_BUS_DATA(bus_data_);          // Data (D0-D7)
   DECLARE_REFERENCE_BUS_ADDRESS(bus_address_);    // Address (A0-A15)
   

protected:

   /////////////////////////////////////
   // Registres
   union Register
   {
      struct {
         unsigned char l;
         unsigned char h;
      } b;
      unsigned short w;
   };

   Register af_;
   Register bc_;
   Register de_;
   Register hl_;

   Register af_p_;
   Register bc_p_;
   Register de_p_;
   Register hl_p_;

   Register ix_;    // IX
   Register iy_;    // IY

   Register ir_;

   unsigned short sp_;
   unsigned short pc_;

   bool iff1_;
   bool iff2_;

   unsigned char q_;
   Register mem_ptr_;

   // Mode d'interruption
   char interrupt_mode_;


   unsigned short address_;
   unsigned char data_;


};

