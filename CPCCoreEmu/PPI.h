#pragma once

#include "Bus.h"
#include "Memoire.h"
#include "ILog.h"
#include "ITapeOut.h"


/*#ifdef CPCCOREEMU_EXPORTS
#define CPCCOREEMU_API __declspec(dllexport)
#else
#define CPCCOREEMU_API __declspec(dllimport)
#endif
*/
#define CPCCOREEMU_API 

class CSig;
class Ay8912;
class CTape;
class CSnapshot;

class CPCCOREEMU_API PPI8255 : public ITapeOut
{
public:
   PPI8255( );
   virtual ~PPI8255();

   void SetPlus(bool plus) {plus_ = plus;}
   void SetSig ( CSig* sig) {sig_ = sig;};
   void SetTape(CTape* tape) { tape_ = tape; };
   void SetLog(ILog* log) { log_ = log; };


   void SetDataRecorderR ( bool set );
   virtual bool GetCassetteWriteData () {return tape_write_data_level_; };

   void SetPrinterBusy ( bool set );
   void SetExpSignal ( bool set );
   void SetScreenState  ( bool set );
   void SetVbl ( bool set );
   void SetAmstradType( unsigned char type );

   void SetPsg( Ay8912 * psg) {psg_ = psg;};
   

   void DataRead  (unsigned char* data, unsigned char address );
   void DataWrite (unsigned char* data, unsigned char address);
   void Reset();

   //bool m_bVbl;
   unsigned char port_a_;
   unsigned char port_b_;
   unsigned char port_c_;

//protected:
   void TraitePortC();
   void TraitePortCAsCtrl();
   void UpdateSignalForPortB ();
   
   // Control for mode 1/2
   union
   {
      unsigned char byte;
      struct {
         union InnerControlB
         {
            struct{
               unsigned char stb:1;
               unsigned char ibf:1;
               unsigned char intr:1;
            }input;
            struct{
               unsigned char obf:1;
               unsigned char ack:1;
               unsigned char intr:1;
            }output;
         }control_b;
         struct{
            unsigned char intr:1;
            unsigned char stb:1;
            unsigned char ibf:1;
            unsigned char ack:1;
            unsigned char obf:1;
         } control_a;
      } inner_control;
   }inner_control_;

   bool tape_write_data_level_;

   ILog* log_;
   CTape* tape_;

   // Signaux;
   CSig* sig_;

   // Connected to PSG
   Ay8912 * psg_;
   unsigned char tape_level_;

   union ControlWord {
      unsigned char byte;
      struct {
         // 0 = Output / 1 = Input
         unsigned char io_low_c : 1;
         unsigned char io_b : 1;
         unsigned char mode_b : 1; // 0 = Mode 0 / 1 = Mode 1
         unsigned char io_high_c : 1;
         unsigned char io_a : 1;
         unsigned char mode_a : 2; // 00 = Mode 0 / 01 = Mode 1 / 1x = Mode 2
         unsigned char b7 : 1;
      } control_word;
   }control_word_;

   bool plus_;
};
