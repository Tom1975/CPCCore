#pragma once

#define CPCCOREEMU_API 

class CPCCOREEMU_API Bus
{
public:
   Bus( unsigned int size );
   virtual ~Bus(void);

   // Bus - Tristate !
   typedef enum {
      ZERO = 0,
      ONE = 1,
      UNDEF = 2
   } BusState;

   void SetBus ( unsigned char data );
   void SetBus ( unsigned short data );
   unsigned short GetShortBus () { return (short)data_; };
   unsigned char  GetByteBus () { return (char)data_; };

protected:
   // Attributs
   unsigned int data_;
   unsigned int defined_data_;
};

