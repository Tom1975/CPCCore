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
   DECLARE_REFERENCE_LINE(line_mreq_);

   DECLARE_REFERENCE_BUS_ADDRESS(bus_address_);
   DECLARE_REFERENCE_BUS_DATA(bus_data_);

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

