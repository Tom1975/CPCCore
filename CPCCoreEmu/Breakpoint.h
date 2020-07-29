
#pragma once
#include <deque>
#include <functional>

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

template<typename  t>
class BreakpointCondition: public IBreakpointItem
{
public:
   BreakpointCondition(std::function<bool(t, t)> op, std::function<t ()> left, std::function<t()> right) : operator_(op), left_(left), right_(right)
   {

   }
   virtual ~BreakpointCondition(){};

   bool Break()
   {
      return operator_(left_(), right_());
   }

   virtual std::string GetBreakpointFormat()
   {
      // TODO
      return "";
   };

protected:
   std::function<bool(t, t)> operator_;
   std::function<t()> left_;
   std::function<t()> right_;
};