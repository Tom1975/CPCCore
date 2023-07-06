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

   void VSync(bool set);
   void HSync(bool set);

   /////////////////////////////////////
   // Various lines, in order of the ship (begining top, clockwork)
   DECLARE_REFERENCE_LINE(line_vsync_);
   DECLARE_REFERENCE_LINE(line_hsync_);
   DECLARE_REFERENCE_LINE(line_dispen_);

   // R, G, B : No dedicated lines for color output to VDU
   // Sync either
   // VDD1-2 and VSS1,2,3 : not connected

   DECLARE_REFERENCE_LINE(line_romen_);            // NROMEN
   DECLARE_REFERENCE_LINE(line_int_);              // NINTERRUPT
   DECLARE_REFERENCE_LINE(line_iorq_);             // NIORQ
   DECLARE_REFERENCE_LINE(line_rd_);               // NRD
   DECLARE_REFERENCE_LINE(line_m1_);               // NM1
   DECLARE_REFERENCE_LINE(line_mreq_);             // NMREQ
   DECLARE_REFERENCE_LINE(line_4_mhz_);            // NPHI
   DECLARE_REFERENCE_LINE(line_reset_);            // NRESET
   DECLARE_REFERENCE_LINE(line_ready_);            // READY
   DECLARE_REFERENCE_BUS_ADDRESS(bus_address_);    // Address
   DECLARE_REFERENCE_LINE(line_ramrd_);            // NRAMRD
   DECLARE_REFERENCE_LINE(line_16_mhz_);           // CK16 
   DECLARE_REFERENCE_BUS_DATA(bus_data_);          // Data
   DECLARE_REFERENCE_LINE(line_n24en_);            // N24EN
   DECLARE_REFERENCE_LINE(line_ras_);              // NRAS
   DECLARE_REFERENCE_LINE(line_ncas_);             // NCAS
   DECLARE_REFERENCE_LINE(line_we_)                // NMWE
   DECLARE_REFERENCE_LINE(line_CCLK_mhz_);         // CCLK
   DECLARE_REFERENCE_LINE(line_casad);             // NCASAD 
   DECLARE_REFERENCE_LINE(line_CPU_ADDR_mhz_);     // NCPU

   //////////////////////////
   // Test : Convenient methods to access inner data
   unsigned char GetS() { return s_; }


protected:

   void Sequencer();
   void SequencerDecode();
   void SequencerDecodeDown();
   void RegisterDecode();
   void InkRegister();
   void RomMapping();
   void CasGeneration();

   unsigned char s_;
   unsigned int counter;

   unsigned char u312_, u313_;
   unsigned char u815, u821, u826, u830, u832, u835, u812, u808, u813, u818, u824;
   unsigned char u809, u814, u820, u825, u829, u803, u836;
   unsigned char u816, u817, u831, u806, u828;

   unsigned char border_;
   unsigned char mode_;
   unsigned short inkr_[5];
   unsigned char inkre_;
   unsigned char inksel_;
   bool irq_reset_;
   bool hromen_;
   bool lromen_;

   bool romen_;
   bool ramrd_;
   bool mode_sync;
   bool nsync_;
   bool hcntlt28_;

};

