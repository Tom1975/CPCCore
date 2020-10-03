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

void CSig::InterruptRaster()
{
   req_int_ = 1;
   int_is_raster_ = true;
}

void CSig::AcqInt ()
{
   req_int_ = false;
   int_ = 0;
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

void CSig::Out (unsigned short Addr_P, unsigned char Data_P/*, int delay*/ )
{

   address_ = Addr_P;
   data_ = Data_P;
   address_bus_->SetBus (address_);
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
      asic_->Out(Addr_P, Data_P);
   }
   else
   {
      if (((address_ & 0x4300) == 0x0000)
         || ((address_ & 0x4300) == 0x0100)
         )
      {
         crtc_->Out(address_, data_);
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

void CSig::PlugExpansionModule(IExpansion* exp)
{
   
}

void CSig::UnplugExapnsionModule(IExpansion* exp)
{
   
}
