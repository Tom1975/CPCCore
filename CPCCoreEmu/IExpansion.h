#pragma once

class IExpansion
{

public:

   // Tick
   virtual unsigned int Tick () = 0;

   // Out
   virtual void Out (unsigned short addr, unsigned char data ) = 0;

   // In
   virtual void In (unsigned char* data, unsigned short address) = 0;

   // Write
   virtual void Set ( unsigned short addr, unsigned char data ) { };
   virtual bool CanWrite( unsigned short addr) {return false;}
   // M1
   virtual void M1 (){};
   virtual void Reset() {};

   virtual unsigned char*  GetROM() { return nullptr;};
};
