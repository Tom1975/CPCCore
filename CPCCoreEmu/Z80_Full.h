#pragma once

#include "IComponent.h"
#include "Memoire.h"
#include "Sig.h"

#define LOG_TAPE

///////////////////////////////////////////////////////
// MACRO
///////////////////////////////////////////////////////

                            // Next instruction is ready
#define SYNC_Z80 current_opcode_ = 0;

#define SET_NMI machine_cycle_=M_M1_NMI;t_ = 1;SYNC_Z80;return 1;
#define SET_INT machine_cycle_=M_M1_INT;t_ = 1;SYNC_Z80;return 1;

#define SET_NOINT if (!stop_on_fetch_){INC_R;machine_cycle_=M_FETCH;t_ = 4;rw_opcode_=false;\
            current_opcode_ = memory_->Get ( pc_++);nextcycle = 4 - ((counter_+1) & 0x3)+1;counter_+=nextcycle+1;return nextcycle+2;}\
            else {machine_cycle_=M_FETCH;t_ = 1;rw_opcode_=false;\
            current_opcode_ = 0;return 1;}

#define NEXT_INSTR         current_function_ = &fetch_func;if (!sig_->nmi_){if ((!sig_->int_) || !iff1_) {SET_NOINT;}else{SET_INT;}}else {SET_NMI;}

#define NEXT_INSTR_RES(reset_ptr)\
   /*if(reset_ptr)*/current_function_ = &fetch_func;\
   if (!sig_->nmi_ && ((!sig_->int_) || !iff1_)){\
         SET_NOINT;}\
      if ((sig_->int_) && iff1_) {\
      {SET_INT;}\
   }else {SET_NMI;}

#define NEXT_INSTR_LDAIR   current_function_ = &fetch_func;if (!sig_->nmi_){if ((!sig_->int_) || !iff1_) {carry_set_ = false;SET_NOINT;}else{carry_set_ = true;SET_INT;}}else{carry_set_ = true;SET_NMI;}
#define NEXT_INSTR_EI      current_function_ = &fetch_func;if (!sig_->nmi_)SET_NOINT;{SET_NMI;}

// INC R
#define INC_R           ir_.b.l = ( ( (ir_.b.l+1) & 0x7F) | (ir_.b.l & 0x80));
#define ZERO_FLAG(r) if (r==0)q_ |= ZF;else q_ &= ~(ZF);
#define SIGN_FLAG(r) if ((r&0x80)==0x80)q_ |= SF;else q_ &= ~(SF);
#define PARITY_FLAG(i) if (P[i])q_ |= PF;else q_ &= ~(PF);

// ADC
#define ADD_FLAG_CARRY(r) \
   res = af_.b.h+r+(af_.b.l&CF);\
   q_ = (((res&0xff)==0)?ZF:0) | (res >>8) | (res&0x80) | ((af_.b.h^res^r)&HF)|(res&0x28);\
   if ( (((af_.b.h&0x80)^(r&0x80)) == 0) && (((r&0x80)^(res&0x80))!=0) ) q_|= PF;/*else af_.b.l &= ~(PF);*/ /*P/V : Set if sign is same betwen r & A(before), and not the same as result*/\
   af_.b.l=q_;\
   af_.b.h=res;\

// Compare
#define CP_FLAGS(c) \
res = af_.b.h - c;\
q_ = NF|(((res&0xff)==0)?ZF:0)|(res&0x80)|((res >> 8) & CF)|((af_.b.h ^ res ^ c) & HF);\
if ( (((af_.b.h&0x80)^(c&0x80)) != 0) && (((af_.b.h&0x80)^(res&0x80))!=0) ) q_|= PF;/*else af_.b.l &= ~(PF); *//*P/V : Set if sign is same betwen r & A(before), and not the same as result*/\
q_ |=(c&0x28);\
   af_.b.l = q_;

#define CPD_FLAGS(c) \
res = af_.b.h - c;\
q_ = (af_.b.l&CF)| NF|(Sz[res&0xFF]&(~(XF|YF)))|((af_.b.h ^ res ^ c) & HF);\
if ( (bc_.w-1) != 0 ) q_|= PF;\
if (q_&HF) res--;\
if (res&0x02) q_ |= YF;\
if (res&0x08) q_ |= XF;\
af_.b.l = q_;

// ADD
#define ADD_FLAG(r) \
   res = af_.b.h+r;\
   q_ = (((res&0xff)==0)?ZF:0) | (res >>8) | (res&0x80) | ((af_.b.h^res^r)&HF)|(res&0x28);\
   if ( (((af_.b.h&0x80)^(r&0x80)) == 0) && (((r&0x80)^(res&0x80))!=0) ) q_|= PF;/*else af_.b.l &= ~(PF);*/ /*P/V : Set if sign is same betwen r & A(before), and not the same as result*/\
   af_.b.h=res;\
   af_.b.l = q_;

#define SUB_FLAG(r) \
   res = af_.b.h-r;\
   q_ = NF | (((res&0xff)==0)?ZF:0) | ((res >>8)&CF) | (res&0x80) | ((af_.b.h^res^r)&HF);\
   if ( (((af_.b.h&0x80)^(r&0x80)) != 0) && (((af_.b.h&0x80)^(res&0x80))!=0) ) q_|= PF;/*else af_.b.l &= ~(PF);*/\
   af_.b.h=res;q_ |=(af_.b.h&0x28);af_.b.l = q_;\


#define SUB_FLAG_CARRY(r) \
   res = af_.b.h-(r+(af_.b.l&CF));\
   q_ = NF | (((res&0xff)==0)?ZF:0) | ((res >>8)&CF) | (res&0x80) | ((af_.b.h^res^r)&HF);\
   if ( (((af_.b.h&0x80)^(r&0x80)) != 0) && (((af_.b.h&0x80)^(res&0x80))!=0) ) q_|= PF;/*else af_.b.l &= ~(PF);*/\
   af_.b.h=res;q_ |=(res&0x28);af_.b.l = q_;\

// Increment
#define INC_FLAGS(dest) \
dest ++;\
q_ = (af_.b.l&CF)|((dest==0)?ZF:0) | (dest & 0x80 ) | ((dest == 0x80)?PF:0)|(((dest&0x0F)==0x00)?HF:0);q_ |= (dest&0x28);\
af_.b.l = q_;

// Decrement
#define DEC_FLAGS(dest) \
--dest;\
q_ = (af_.b.l&CF)|NF|((dest==0)?ZF:0) | (dest&0x80) |((dest==0x7F)?PF:0)| (((dest&0x0F)==0x0F)?HF:0);q_ |= (dest&0x28);af_.b.l = q_;\

#define IN_FLAGS(r)  r=current_data_ &0xFF;q_ = af_.b.l&CF;ZERO_FLAG(r);SIGN_FLAG(r);/*q_ |= (r&0x80)?NF:0;*/PARITY_FLAG(r);q_ |= (r & 0x28);/*TODO : ADD HERE XY FLAGS !*/;af_.b.l = q_;

// Boolean operators
#define AND_FLAGS(r)\
res = (af_.b.h & r); q_ = (((res&0xff)==0)?ZF:0)|HF|(res&0x80);/*unsigned int res = af_.b.h & r; af_.b.l = (((res&0xff)==0)?ZF:0)|HF|(res&0x80);*/\
PARITY_FLAG(res);q_ |= (res&0x28);af_.b.l = q_;\
af_.b.h = res;

#define OR_FLAGS(r) \
res = (af_.b.h | r); q_ = (((res&0xff)==0)?ZF:0)|(res&0x80);\
PARITY_FLAG(res);q_ |= (res&0x28);af_.b.l = q_;\
af_.b.h = res;

#define XOR_FLAGS(r)  \
af_.b.h= (af_.b.h^r); q_ = (((af_.b.h&0xff)==0)?ZF:0)|(af_.b.h&0x80)|((af_.b.h&0x1)<<2);PARITY_FLAG(af_.b.h);q_ |= (af_.b.h&0x28);af_.b.l=q_;


// Flag tests
#define TST(c)  if ((af_.b.l & c) == c)
#define TSTN(c)  if ((af_.b.l & c) == 0)

// BIT operations
#define RL(c) btmp = af_.b.l&CF;q_ = (c&0x80)?CF:0;c=c<<1;c+=btmp;q_ |= ((((c&0xff)==0)?ZF:0)|(c&0x80));q_ |= (c&0x28);PARITY_FLAG(c);af_.b.l = q_;
#define RR(c) btmp = af_.b.l&CF;q_ = (c&0x1)?CF:0;c=c>>1;c+=btmp*0x80;q_ |= ((((c&0xff)==0)?ZF:0)|(c&0x80));q_ |= (c&0x28);PARITY_FLAG(c);af_.b.l = q_;
#define RLC(c) btmp = c>>7;c = (c<<1) + btmp;q_ = btmp | ((((c&0xff)==0)?ZF:0)|(c&0x80));q_ |= (c&0x28);PARITY_FLAG(c);af_.b.l = q_;
#define RRC(c) q_ =(c&0x1)?CF:0;btmp = c&0x1;c = (c>>1) + btmp*0x80;q_ |= ((((c&0xff)==0)?ZF:0)|(c&0x80));q_ |= (c&0x28);PARITY_FLAG(c);af_.b.l = q_;
#define SLA(c) {if (( c & 0x80) == 0x80)q_ |= (CF);c = c << 1;if (c == 0x00) q_ |= (ZF);q_ |= ((c&SF)|(c&0x28));PARITY_FLAG(c);af_.b.l = q_;}
#define SRA(c) {if (( c & 0x01) == 0x01)q_ |= (CF);btmp = c&0x80;c = (c >> 1)|btmp;if (c == 0x00) q_ |= (ZF);q_ |= ((c&SF)|(c&0x28));PARITY_FLAG(c);af_.b.l = q_;}
#define SLL(c) {if (( c & 0x80) == 0x80)q_ |= (CF);c = (c << 1)+1;if (c == 0x00) q_ |= (ZF);q_ |= ((c&SF)|(c&0x28));PARITY_FLAG(c);af_.b.l = q_;}
#define SRL(c) {if (( c & 0x01) == 0x01)q_ |= (CF);c = c >> 1;if (c == 0x00) q_ |= (ZF);q_ |= ((c&SF)|(c&0x28));PARITY_FLAG(c);af_.b.l = q_;}
#define BIT_B_R(bitnum,reg)   ((reg >>bitnum)&0x1);q_ = (af_.b.l & CF) | HF | (SzBit[reg & (1<<bitnum)]& ~(YF|XF)) | ((reg &(YF|XF)));af_.b.l = q_;
#define BIT_B_R_DD(bitnum,r) btmp = ((r>>bitnum)&0x1);q_ = (HF) | (af_.b.l&CF);q_ &= ~(NF|0x28);if (bitnum == 7 && btmp ==0x1) q_ |= (SF);if (btmp  == 0x00) q_ |= (ZF|PF);q_ |= (((mem_ptr_.w)>>8)&0x28);af_.b.l = q_;
#define SET(r,b) r|=(0x01<<b);
#define RES(r,b) r&=~(0x01<<b);

//////////////////
// FLAGS
#define CF        0x1
#define NF        0x2
#define PF        0x4   // Also parity
#define VF        PF
#define XF        0x8
#define HF        0x10
#define YF        0x20
#define ZF       0x40
#define SF       0x80

#define MAX_DISASSEMBLY_SIZE 32

///////////////////////////////////////////////////////////////////
// Z80 full implementation
///////////////////////////////////////////////////////////////////

class Z80 : public IComponent
{
public:
   Z80(void);
   virtual ~Z80(void);

   void Init(Memory * memory, CSig* sig, ILog* log) {
      memory_ = memory; sig_ = sig; log_ = log;
   }

   void InitOpcodeShortcuts();
   unsigned char GetOpcodeSize(unsigned short address);
   bool IsCallInstruction(unsigned short address);


   unsigned int Tick( /*unsigned int nbTicks = 1*/); /*{ return 1;};*/
   void PreciseTick ( );
   void Reset ();
   void InterruptInit ();

   void ReinitProc ();
   unsigned int GetCurrentOpcode () { return current_opcode_;};
   unsigned short GetPC();

   int OpcodeIOR();
   int OpcodeIOW();
   unsigned int OpcodeMEMR();
   int OpcodeMEMW();
   int OpcodeWAIT();

   //////////////////////////////////////
   // Externel pins
   typedef unsigned int (Z80::*Func)();
   typedef Func ListFunction[0x100];


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
   
   bool stop_on_fetch_;

   unsigned int current_opcode_ ;
   ListFunction * current_function_;
   Func next_function_;

   unsigned short current_address_;
   unsigned short current_data_;

   // Current state : T state and Machine Cycle
   int t_;

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
   MachineCycle machine_cycle_;

   // Inner helper attributes
   unsigned int counter_;
   unsigned int read_count_;

   // Optimize functions
   bool carry_set_;
   Memory * memory_;
   CSig* sig_;

   // Vectorized Interrupt Bug : Detect opcodes that perform read/write memory
   bool rw_opcode_;

   typedef struct
   {
      unsigned char size;
      char disassembly[MAX_DISASSEMBLY_SIZE];
   }Opcode;

   typedef enum
   {
      None,
      CB, 
      ED,
      DD,
      FD,
   } OpcodeType;

   template<OpcodeType type>
   void FillStructOpcode(unsigned char opcode, unsigned int(Z80::* func)(), unsigned char Size, const char* disassembly)
   {
      Opcode *op;
      ListFunction* fetch;
      switch( type )
      {
      case None:
         op = &liste_opcodes_[opcode];
         fetch = &fetch_func;
         break;
      case CB:
         op = &liste_opcodes_cb_[opcode];
         fetch = &fetch_func_cb_;
         break;
      case ED:
         op = &liste_opcodes_ed_[opcode];
         fetch = &fetch_func_ed_;
         break;
      case DD:
         op = &liste_opcodes_dd_[opcode];
         fetch = &fetch_func_dd_;
         break;
      case FD:
         op = &liste_opcodes_fd_[opcode];
         fetch = &fetch_func_fd_;
         break;
      }
      op->size = Size;
      strcpy(op->disassembly, disassembly);
      (*fetch)[opcode] = func;
   };

   template<OpcodeType type>
   void FillStructOpcodeMemr(unsigned char opcode, unsigned int(Z80::* func)())
   {
      ListFunction* memr;
      switch (type)
      {
      case None:
         memr = &memr_func_;
         break;
      case CB:
         memr = &memr_func_cb_;
         break;
      case ED:
         memr = &memr_func_ed_;
         break;
      case DD:
         memr = &memr_func_dd_;
         break;
      case FD:
         memr = &memr_func_fd_;
         break;
      }
      (*memr)[opcode] = func;
   }

   Opcode liste_opcodes_[256];
   Opcode liste_opcodes_cb_[256];
   Opcode liste_opcodes_ed_[256];
   Opcode liste_opcodes_dd_[256];
   Opcode liste_opcodes_fd_[256];


   void TraceTape(unsigned short pc, unsigned char value);
   ILog* log_;
   int count_;

   unsigned char Sz[256];       /* zero and sign flags */
   unsigned char SzBit[256];   /* zero, sign and parity/overflow (=zero) flags for BIT opcode */
   unsigned char Szp[256];      /* zero, sign and parity flags */
   unsigned char SzhvInc[256]; /* zero, sign, half btmp and overflow flags INC r8 */
   unsigned char SzhvDec[256]; /* zero, sign, half btmp and overflow flags DEC r8 */
   unsigned char P[256];        /* Parity flag */
   unsigned char FlagAnd[256];        /* AND flags */
   unsigned char FlagOr[256];        /* AND flags */
   unsigned char FlagXor[256];        /* AND flags */

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

   unsigned int DefaultFetch();
   unsigned int Opcode_DefaultToSimple();
   unsigned int Opcode_NOP();
   unsigned int Opcode_CB();
   unsigned int Opcode_ED();
   unsigned int Opcode_DD();
   unsigned int Opcode_FD();

   typedef enum
   {
      ADDR_PC,
      ADDR_AF,
      ADDR_BC,
      ADDR_DE,
      ADDR_HL,
      ADDR_IX,
      ADDR_IY,
      ADDR_SP,
      ADDR_AFP,
      ADDR_BCP,
      ADDR_DEP,
      ADDR_HLP
   } AddressRegisters;

   typedef enum
   {
      R_0,
      R_A,
      R_F,
      R_B,
      R_C,
      R_D,
      R_E,
      R_H,
      R_L,
      R_IXh,
      R_IXl,
      R_IYh,
      R_IYl,
      R_I,
      R_R
   } Registers;

   const char* REG_TO_STR(Registers reg)
   {
      return ((reg == R_A) ? "A" : (reg == R_F) ? "F" : (reg == R_B) ? "B" : (reg == R_C) ? "C" : (reg == R_D) ? "D" : (reg == R_E) ? "E" : (reg == R_H) ? "H" : "L");
   }

   typedef enum
   {
      AND,
      XOR,
      OR,
   } OperationType;

   typedef enum
   {
      MEMORY_OP,
      IO_OP
   } OperationSource;

   template<MachineCycle operation_source, AddressRegisters reg>
   unsigned int Opcode_Read_REGW();

   template<Z80::MachineCycle operation_source, Z80::AddressRegisters reg>
   unsigned int Opcode_Delayed_Read_REG();

   template<MachineCycle operation_source, AddressRegisters addr, Registers reg>
   unsigned int Opcode_Write_Addr_Reg();

   template<Z80::Registers reg, bool reset_ptr>
   unsigned int Opcode_Inc_Reg();

   template<Z80::Registers reg, bool reset_ptr>
   unsigned int Opcode_Dec_Reg();

   template<Z80::AddressRegisters reg, bool reset_ptr>
   unsigned int Opcode_Inc_RegW();

   template<Z80::AddressRegisters reg>
   unsigned int Opcode_Dec_RegW();

   template<Z80::AddressRegisters reg>
   unsigned int Opcode_Dec_RegWI();

   template<Z80::AddressRegisters reg1, Z80::AddressRegisters reg2>
   unsigned int Opcode_EX();

   template<Z80::AddressRegisters reg1, Z80::AddressRegisters reg2>
   unsigned int Opcode_ADD_REGW();

   template<Z80::AddressRegisters reg, int t_val>
   unsigned int Opcode_Memory_Read_Delayed();

   template<Z80::Registers reg1, Z80::Registers reg2>
   unsigned int Opcode_Ld_Reg();

   template<Z80::Registers reg1, Z80::Registers reg2>
   unsigned int Opcode_Ld_Delayed_Reg();

   template<Z80::Registers reg, bool Carry>
   unsigned int Opcode_Add_Reg();

   template<Z80::AddressRegisters reg>
   unsigned int Opcode_Add_Reg();

   template<Z80::Registers reg, bool Carry>
   unsigned int Opcode_Sub_Reg();
   
   template<Z80::AddressRegisters reg>
   unsigned int Opcode_Sub_Reg();

   template<Z80::Registers reg, Z80::OperationType op>
   unsigned int Opcode_BOOL_Reg();
      
   template<Z80::Registers reg>
   unsigned int Opcode_CP_Reg();

   template<unsigned char cond, bool present>
   unsigned int Opcode_Ret_Cond();

   template<Z80::AddressRegisters reg>
   unsigned int Opcode_Push();

   template<int b> unsigned int Opcode_RLC();
   template<int b> unsigned int Opcode_RRC();
   template<int b> unsigned int Opcode_RL();
   template<int b> unsigned int Opcode_RR();
   template<int b> unsigned int Opcode_SLA();
   template<int b> unsigned int Opcode_SRA();
   template<int b> unsigned int Opcode_SLL();
   template<int b> unsigned int Opcode_SRL();
   template<int b, Z80::Registers reg> unsigned int Opcode_BIT();
   template<int b, Z80::Registers reg> unsigned int Opcode_RES();
   template<int b, Z80::Registers reg> unsigned int Opcode_SET();
   
   template<int mode> unsigned int Opcode_IM();

   unsigned int Opcode_MemoryFromStack();
   unsigned int Opcode_Push_delayed();
   unsigned int Opcode_Call_fetch();

   unsigned int Opcode_CPL();
   unsigned int Opcode_CCF();
   unsigned int Opcode_SCF();

   unsigned int Opcode_NEG();

   unsigned int Opcode_DAA();
   unsigned int Opcode_RLA();
   unsigned int Opcode_RLCA();
   unsigned int Opcode_RRA();
   unsigned int Opcode_RRCA();

   unsigned int Opcode_HALT();
   unsigned int Opcode_Exx();
   unsigned int Opcode_DI();
   unsigned int Opcode_EI();

   unsigned int MEMR_DJNZ();
   unsigned int MEMR_JR();

   unsigned int MEMR_Inc_REGW();

   template<AddressRegisters reg>unsigned int Opcode_JP_REGW();

   template<AddressRegisters reg>unsigned int Opcode_LD_SP_REGW();

   template<Z80::AddressRegisters reg> unsigned int MEMR_Read_REGW_();
   template<Z80::Registers reg> unsigned int MEMR_Read_REG_();
   template<Z80::Registers reg, Z80::AddressRegisters regw> unsigned int MEMR_Read_REG_REGW();
   template<bool positive, int cond> unsigned int MEMR_JR_Cond();
   template<Z80::AddressRegisters regw> unsigned int MEMR_Read_NN_HL();

   template<Z80::AddressRegisters regw> unsigned int MEMR_HL_NN_0();
   template<Z80::AddressRegisters regw> unsigned int MEMR_HL_NN_1();
   template<Z80::AddressRegisters regw> unsigned int MEMR_HL_NN_2();
   template<Z80::AddressRegisters regw> unsigned int MEMR_HL_NN_3();
   template<Z80::Registers reg> unsigned int MEMR_Read_REG_NN();
   
};

#include "Z80_Opcodes.hpp"
