#pragma once
#include "IExpansion.h"
#include "Memoire.h"

class CSig;
class MultifaceII : public IExpansion
{
public:
   MultifaceII(void);
   virtual ~MultifaceII(void);

   void Init (CSig* sig);

   // Tick
   virtual unsigned int Tick ();

   // Out
   virtual void Out (unsigned short address, unsigned char data ) ;

   // In
   virtual void In (unsigned char* data, unsigned short address);

   virtual void Set ( unsigned short address, unsigned char data) { if (address >= 0x2000 && address < 0x4000) rom_ram_[address] = data;};
   virtual bool CanWrite( unsigned short address) {return (address >= 0x2000) ;}
   virtual void M1 ();

   // Button actions
   void Visible (bool bVisible){visible_ = bVisible;}
   bool IsVisible (){return visible_;}
   void Reset ();
   void Stop ();

   virtual unsigned char*  GetROM();

protected:
   CSig* sig_;
   Memory::RamBank rom_ram_;
   unsigned char* ram_zone_;

   bool nmi_enabled_;
   bool visible_;
};

