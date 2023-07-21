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
   DECLARE_REFERENCE_LINE(line_busak_);            // BUSAK    - Out
   DECLARE_REFERENCE_LINE(line_halt_);             // HALT     - Out
   DECLARE_REFERENCE_LINE(line_mreq_);             // NMREQ    - Out
   DECLARE_REFERENCE_LINE(line_m1_);               // NM1      - Out
   DECLARE_REFERENCE_LINE(line_rfsh_);             // RFSH     - Out
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

   /////////////////////////////////////
   // Inner helper attributes
   unsigned int counter_;

   // Externel pins
   typedef unsigned int (Z80::* Func)();
   typedef Func ListFunction[0x100];

   typedef enum {
      M_FETCH = 0x10,
      M_MEMORY_R = 0x20,
      M_MEMORY_W = 0x30,
      M_IO_R = 0x40,
      M_IO_W = 0x50,
      M_Z80_WORK = 0x60,
      M_M1_INT = 0x70,
      M_M1_NMI = 0x80,
      M_IO_R_INT = 0x90,
      M_Z80_WAIT = 0xA0,
   } MachineCycle;

   // Current state : T state and Machine Cycle
   int t_;
   MachineCycle machine_cycle_;

   Func tick_functions_[0xAF];
   ListFunction fetch_func;
   ListFunction fetch_func_cb_;
   ListFunction fetch_func_ed_;
   ListFunction fetch_func_dd_;
   ListFunction fetch_func_fd_;
   ListFunction memr_func_;
   ListFunction memr_func_cb_;
   ListFunction memr_func_ed_;
   ListFunction memr_func_dd_;
   ListFunction memr_func_fd_;

   void InitOpcodeShortcuts();
   void InitTickFunctions();

   typedef enum
   {
      None,
      CB,
      ED,
      DD,
      FD,
   } OpcodeType;

   template<OpcodeType type>
   void FillStructOpcode(unsigned char opcode, unsigned int(Z80::* func)(), unsigned char Size, const char* disassembly);

   /////////////////////////////////////
   // Opcodes
   unsigned int DefaultFetch();
   unsigned int Opcode_DefaultToSimple();
   unsigned int Opcode_NOP();
   unsigned int Opcode_CB();
   unsigned int Opcode_ED();
   unsigned int Opcode_DD();
   unsigned int Opcode_FD();

};

