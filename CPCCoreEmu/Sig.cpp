#include "stdafx.h"
#include "Sig.h"
#include "CRTC.h"
#include "VGA.h"
#include "Z80_Full.h"
#include "Asic.h"


#define LOG(str) \
   if (log_) log_->WriteLog (str);
#define LOGEOL if (log_) log_->EndOfLine ();
#define LOGB(str) \
   if (log_) log_->WriteLogByte (str);


CSig::CSig(void) : plus_(false), interrupt_io_data_(0xFF)
{
   log_ = NULL;
   h_sync_ = false;
   v_sync_ = false;
   dispen_ = false;
   req_int_ = req_nmi_ = false;
   int_delay_ = 0;
   irq_ack_hold_ = 0;
   irq_ack_raster_ = false;
   int_ = 0;
   nmi_ = 0;
   hsync_raise_ = false;
   hsync_fall_ = false;

   nb_expansion_ = 0;
   memset (exp_list_, 0, sizeof (exp_list_));

   printer_port_ = NULL;
   int_is_raster_ = false;
   int_is_gate_array_ = false;
}


CSig::~CSig(void)
{
}

void CSig::Reset ()
{
   h_sync_on_begining_of_line_ = false;
   pri_changed_ = false;
   h_sync_ = h_sync_wr_ = false;
   v_sync_ = v_sync_wr_ = false;
   dispen_ = false;
   req_int_ = req_nmi_ = false;
   int_delay_ = 0;
   irq_ack_hold_ = 0;
   irq_ack_raster_ = false;
   int_ = 0;
   nmi_ = 0;
   hsync_raise_ = false;
   hsync_fall_ = hsync_fall_wr_ = false;;

   z80_->Reset();

   fdc_->Reset ();
   vga_->Reset ();
   crtc_->Reset ();
   ppi_->Reset ();

}

// This method propagate signal, so they can be used by other components
void CSig::Propagate()
{

   int_ |= req_int_;
   req_int_ = false;
   if (int_delay_ != 0 && --int_delay_ == 0)
      int_ = true;
   if (irq_ack_hold_ != 0)
      --irq_ack_hold_;

   nmi_ |= req_nmi_;
   req_nmi_ = false;

   // ctrl_int
   /*if (req_int_)
   {
      int_ = true;
      req_int_ = false;
   }
   // NMI
   if (req_nmi_)
   {
      nmi_ = true;
      req_nmi_ = false;
   }*/

}

void CSig::M1()
{
   // Is there something on the expansion port ?
   for (int i = 0; i < nb_expansion_; i++)
   {
      exp_list_[i]->M1 ();
   }
}

// INT from the GATE ARRAY, seen by the Z80A from the T-state 'from_t_state' of this microsecond (1 : the next one,
// as req_int_). Propagate runs after the Z80A on each T-state.
void CSig::InterruptRaster(unsigned char from_t_state)
{
   if (from_t_state > 1)
      int_delay_ = from_t_state;
   else
      req_int_ = 1;
   int_is_raster_ = true;
}

void CSig::AcqInt ()
{
   req_int_ = false;
   int_ = 0;
   irq_ack_raster_ = int_is_raster_;
   // DCSR : Reset to 0

   if (plus_)
   {
      // if last was from dcsr, ack it
      unsigned char dcsr = memory_->GetDCSR();
      if (int_is_raster_)
      {
         dcsr |= 0x80;
         int_is_raster_ = false;
         vga_->interrupt_counter_ &= 0x1F;
      }
      else
      {
         dcsr &= ~0x80;
      }

      // Auto ack
      unsigned char ivr = memory_->GetIVR();
      if ((ivr & 1) == 0)
      {
         dcsr &= ~0x70;
      }
      memory_->SetDCSR(dcsr);

   }
   else
   {
      if (int_is_raster_)
         vga_->interrupt_counter_ &= 0x1F;
   }

}

void CSig::Out (unsigned short Addr_P, unsigned char Data_P, unsigned int t_state)
{

   address_ = Addr_P;
   data_ = Data_P;
   address_bus_->SetBus (address_);
   // The CRTC is not wired to RD / WR : A14 = 0 selects it and A9 is its R/W line (5.1). An OUT on &BExx / &BFxx (A9 = 1)
   // is a read for it : it drives the data bus with the status or the register while the Z80A drives the byte written.
   // Measured on a real CRTC 1 (disquettes de test, PALA14 / PALA14B, 2026-11-19) : OUT (C),&C4 on &3Fxx with R15 selected
   // gives the RAM configuration C0 for R15 = &00, C4 for R15 = &44, &C4, &C7, &FF, C4 for &C5 with R15 = &C6 : the bits
   // 0 to 5 of the bus are the AND of both drivers (the low level wins), the bits 6, 7 stay those of the Z80A. The CPC
   // with CRTC 0 or 2 of the same series take the byte of the Z80A. PROVISIONAL : why only the bits 0 to 5 and only the
   // CRTC 1 is not known (RealCpc/README.md, part 1 bis, P10). It is also why the compendium (5.1, note *) finds the PAL
   // selected by A14 = 0 "on some CPC only" : an RAM configuration written on &3Fxx of a CRTC 1 is spoiled.
   if (!plus_ && crtc_->type_crtc_ == CRTC::UM6845R && (address_ & 0x4200) == 0x0200)
   {
      const unsigned char driven = crtc_->In(address_);
      data_ = data_ & (driven | 0xC0);
   }
   data_bus_->SetBus (data_);

   if ( address_ == 256)
   {
      int dbg = 1;
   }

   // Is it a PPI call ?
   if ( ((address_ &0x0800) ==0x0000 )
      )   {
      ppi_->DataWrite (&data_, (address_>>8)&0x3);
   }
   if ( fdc_present_ &&
         ( ((address_ &0x0581) ==0x0000 )
         ||((address_ &0x0580) ==0x0000 )
         ||((address_ &0x0581) ==0x0101 )
         ||((address_ &0x0581) ==0x0100 ))
      )
   {
      fdc_->Out (address_, data_);
   }


   if (plus_)
   {
      asic_->Out(Addr_P, Data_P, t_state);
   }
   else
   {
      if (((address_ & 0x4300) == 0x0000)
         || ((address_ & 0x4300) == 0x0100)
         )
      {
         crtc_->Out(address_, data_, t_state);
      }

      if (((address_ & 0xC000) == 0x4000)
         || ((address_ & 0x8000) == 0x0000)
         || ((address_ & 0x2000) == 0)
         )
      {
         vga_->TickIO();
      }
   }

   // Magnum Light Phaser
   if (address_ == 0xFBFE)
   {
      int dbg = 1;
   }

   // Is there something on the expansion port ?
   for (int i = 0; i < nb_expansion_; i++)
   {
      exp_list_[i]->Out (Addr_P, Data_P);
   }

   // Printer port ?
   if (( address_ & 0x1000) == 0x0000)
   {
      // Send this to printer port
      if ( printer_port_ != NULL )
      {
         printer_port_->Out ( Data_P );
      }

   }
   //IORW = false;
}


void CSig::In (unsigned char* a, unsigned char b, unsigned char c, bool int_mode_0)
{
   unsigned short address_ = b<<8|c;
   bool found = false;

   *a = 0xFF;

   // Est-ce un composant actif ?
   if ( ((b &0x08) ==0x00 )
      && (int_mode_0 == false)
      )
   {
      // Non, Action
      *a = data_;
      ppi_->DataRead ( a, b&0x3);

      found = true;
   }
   if (fdc_present_ &&
         ( ((address_ &0x0581) ==0x0000 )
         ||((address_ &0x0581) ==0x0100 )
         ||((address_ &0x0581) ==0x0101 )
         )
      && (int_mode_0 == false)
      )
   {
      *a = fdc_->In ( address_ );
      found = true;
   }
   // CRTC
   if ((b &0x40) ==0x00 && (int_mode_0 == false))
   {
      *a = crtc_->In ( address_ );
      found = true;
   }
   if (plus_ && (int_mode_0 == false))
   {
       data_bus_->SetBus( (unsigned char)memory_->GetLastValueRead()  ); // 0x76 on 464 / 0x79 on 6128 todo
      asic_->In(a, address_);
   }
   else if (int_mode_0 == false && (address_ & 0xC000) == 0x4000 && (*a & 0xC0) != 0xC0)
   {
      // GATE ARRAY 40010 : its registers are strobed by A15 = 0, A14 = 1, IORQ low and M1 high, neither RD nor WR
      // (40010-simplified, sheet Registers, U401) : an IN that selects it gives it the byte on the data bus, as a
      // write. Measured on a real CPC (UnitTests/RealCpc/README.md, T1, 2026-11-18) : IN A,(C) with BC = &7400 reads
      // the PSG register 0 (#4C) through the PPI port A, and the border turns bright red. Compendium 4.4 says the
      // opposite ("une lecture sur ce circuit n'est pas prise en compte"). The RAM configuration (#C0 to #FF) is
      // decoded by the PAL of the 6128, not by the 40010 : not known, a read leaves it (and an IN that reads nothing,
      // the bus at #FF, then has no effect).
      address_bus_->SetBus(address_);
      data_bus_->SetBus(*a);
      vga_->TickIO();
   }

   if ( (int_mode_0 == false)) // TODO !!!!
      for (int i = 0; i < nb_expansion_; i++)
      {
         exp_list_[i]->In (a, address_ );

      }

   // Traces
   if (!found)
   {
      LOG ( "IN : ");
      LOGB( b);
      LOGB( c);
      LOG ( " -> Ret : ");
      LOGB( *a);
      LOGEOL
   }

}
