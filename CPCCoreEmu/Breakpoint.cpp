#include "stdafx.h"
#include "Breakpoint.h"

#include "Machine.h"


/////////////////////////////////////////////////////////////////////////////
//
// Specific breakpoints
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// PC
BreakpointPC::BreakpointPC(EmulatorEngine* machine, unsigned short addr)
{
   break_address_ = addr;
   machine_ = machine;
}

bool BreakpointPC::Break()
{
   return (machine_->GetProc()->GetPC() == break_address_);
}

/////////////////////////////////////////////////////////////////////////////
// Memory
BreakpointMemory::BreakpointMemory(EmulatorEngine* machine, unsigned short addr, unsigned char value)
{
   memory_address_ = addr; memory_ = machine->GetMem(); memory_value_ = value;
}

bool BreakpointMemory::Break()
{
   return (memory_->Get(memory_address_) == memory_value_);
}

/////////////////////////////////////////////////////////////////////////////
// Memory access
BreakpointMemoryAccess::BreakpointMemoryAccess(EmulatorEngine* machine, unsigned short addr, bool read)
{
   memory_address_ = addr; memory_ = machine->GetMem(); access_read_ = read;
}

bool BreakpointMemoryAccess::Break()
{
   // Access ? - TODO
   // Read
   if (access_read_)
   {
      for (unsigned int i = 0; i < memory_->index_addr_read_; i++)
      {
         if (memory_->last_address_read_[i] == memory_address_)
         {
            return true;
         }
      }
   }
   else
   {
      for (unsigned int i = 0; i < memory_->index_addr_write_; i++)
      {
         if (memory_->last_address_write_[i] == memory_address_)
         {
            return true;
         }
      }

   }
   return false;
}

/////////////////////////////////////////////////////////////////////////////
// Reg value
BreakpointRegisterValue::BreakpointRegisterValue(EmulatorEngine* machine, unsigned short* reg, unsigned short value)
{
   register_ = reg; memory_ = machine->GetMem(); register_value_ = value;
}

bool BreakpointRegisterValue::Break()
{
   return (*register_ == register_value_);
}

/////////////////////////////////////////////////////////////////////////////
// Complex breakpoints
BreakPointComplex::BreakPointComplex()
{
   
}

BreakPointComplex::~BreakPointComplex()
{

}

bool BreakPointComplex::Break()
{
   return false;
}
