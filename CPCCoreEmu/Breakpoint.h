
#pragma once

class EmulatorEngine;
class Memory;

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

class BreakpointHandler : public IBreakpoint
{
// Construction
public:
	BreakpointHandler();
	virtual ~BreakpointHandler();

   virtual void Init ( EmulatorEngine* machine ) {machine_ = machine;};
   virtual bool IsBreak ();
   IBreakpointItem* CreateBreakpoint  (char* breakpoint_string);
   void RemoveBreakpoint (IBreakpointItem* breakpoint);
   bool IsThereBreakOnAdress ( unsigned short addr );
   virtual void ToggleBreakpoint ( unsigned short addr );
   virtual int GetBreakpointNumber() { return breakpoint_number_; };
   void AddBreakpoint(IBreakpointItem* breakpoint);
   // Implémentation

protected:


   // Breakpoints data
   EmulatorEngine* machine_;

   IBreakpointItem** breakpoint_list_;
   int breakpoint_number_;
   int breakpoint_list_size_;
};

