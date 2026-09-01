
#pragma once
#include <string>

class EmulatorEngine;
class Memory;

#define NB_BP_MAX    100

class IBreakpoint
{
public:
   virtual bool IsBreak() = 0;
};

class IBreakpointItem
{
public :
   virtual bool Break () = 0;
   virtual bool IsThereBreakOnAdress (unsigned short addr) { return false;}
   virtual std::string GetBreakpointFormat() { return ""; };
};

class BreakpointPC : public IBreakpointItem
{
public :
   BreakpointPC (EmulatorEngine* machine, unsigned short addr);
   virtual bool Break ();
   virtual bool IsThereBreakOnAdress (unsigned short addr) { return break_address_==addr;}


protected:
   unsigned short break_address_;
   EmulatorEngine* machine_;
};

class BreakpointMemoryAccess : public IBreakpointItem
{
public :
   BreakpointMemoryAccess(EmulatorEngine* machine, unsigned short addr, bool read);
   virtual bool Break ();

protected:
   unsigned short memory_address_;
   bool access_read_;
   Memory* memory_;
};

class BreakpointMemory : public IBreakpointItem
{
public :
   BreakpointMemory(EmulatorEngine* machine, unsigned short addr, unsigned char value);
   virtual bool Break ();

protected:
   unsigned short memory_address_;
   unsigned char memory_value_;
   Memory* memory_;
};


class BreakpointRegisterValue : public IBreakpointItem
{
public :
   BreakpointRegisterValue(EmulatorEngine* machine, unsigned short* reg, unsigned short value);
   virtual bool Break ();

protected:
   unsigned short* register_;
   unsigned short register_value_;
   Memory* memory_;
};

class BreakPointComplex : public IBreakpointItem
{
public:
   BreakPointComplex();
   virtual ~BreakPointComplex();

   virtual bool Break();

protected:

};

class IValue
{
public:
   virtual int GetValue() = 0;
   virtual std::string GetFormat() = 0;

};

class IOperator
{
public:
   virtual bool IsTrue() = 0;
   virtual std::string GetFormat() = 0;

};

//template<typename  T>
class BreakpointCondition: public IBreakpointItem
{
public:
   //BreakpointCondition(std::function<bool(T, T)> op, std::function<T ()> left, std::function<T()> right) : operator_(op), left_(left), right_(right)
   //BreakpointCondition(std::function<bool(T, T)> op, IValue* left, IValue* right) : operator_(op), left_(left), right_(right)
   BreakpointCondition(IOperator* op/*, IValue* left, IValue* right*/) : operator_(op)//, left_(left), right_(right)
   {

   }
   virtual ~BreakpointCondition(){};

   bool Break()
   {
      return operator_->IsTrue();
   }

   virtual std::string GetBreakpointFormat()
   {
      // TODO
      //std::string str = left_->GetFormat() + "=" + right_->GetFormat();
      return operator_->GetFormat();
   };

protected:
   //std::function<bool(T, T)> operator_;
   IOperator* operator_;
   //IValue* left_;
   //IValue* right_;
   //std::function<T()> left_;
   //std::function<T()> right_;
};