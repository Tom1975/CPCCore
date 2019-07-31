#include "stdafx.h"
#include "Breakpoint.h"

#include "Machine.h"


/////////////////////////////////////////////////////////////////////////////
// CBreakPoint

BreakpointHandler::BreakpointHandler()
{
   machine_ = nullptr;
   breakpoint_list_size_ = 10;
   breakpoint_list_ = new IBreakpointItem*[breakpoint_list_size_];
   breakpoint_number_ = 0;
}

BreakpointHandler::~BreakpointHandler()
{
   delete[]breakpoint_list_;
}


bool BreakpointHandler::IsBreak()
{
   // Parcours des breakpoints
   //for (std::vector<IBreakpointItem*>::iterator it = m_BreakpointList.begin(); it != m_BreakpointList.end(); it++)
   for (auto i = 0; i < breakpoint_number_; i++)
   {
      // Si on break, on break !
      if (breakpoint_list_[i]->Break())
         return true;
   }
   return false;
}

void BreakpointHandler::RemoveBreakpoint(IBreakpointItem* breakpoint)
{
   //for (std::vector<IBreakpointItem*>::iterator it = m_BreakpointList.begin(); it != m_BreakpointList.end(); it++)
   for (auto i = 0; i < breakpoint_number_; i++)
   {
      if (breakpoint_list_[i] == breakpoint)
      {
         for (auto j = i + 1; j < breakpoint_number_; j++)
         {
            breakpoint_list_[j - 1] = breakpoint_list_[j];
         }
         delete breakpoint;
         --breakpoint_number_;
         return;
      }
   }
}

bool BreakpointHandler::IsThereBreakOnAdress(unsigned short addr)
{
   for (auto j = 0; j < breakpoint_number_; j++)
   {
      if (breakpoint_list_[j]->IsThereBreakOnAdress(addr))
         return true;
   }
   return false;
}

void BreakpointHandler::ToggleBreakpoint(unsigned short addr)
{
   for (auto j = 0; j < breakpoint_number_; j++)
   {
      if (breakpoint_list_[j]->IsThereBreakOnAdress(addr))
      {
         // Remove it
         RemoveBreakpoint(breakpoint_list_[j]);
         return;
      }
   }
   // Create it
   IBreakpointItem* breakpoint = new BreakpointPC(machine_, addr);
   AddBreakpoint(breakpoint);
}


void BreakpointHandler::AddBreakpoint(IBreakpointItem* breakpoint)
{
   if (breakpoint_number_ + 1 == breakpoint_list_size_)
   {
      auto old_brk_size = breakpoint_list_size_;
      breakpoint_list_size_ *= 2;
      IBreakpointItem** tmp = new IBreakpointItem*[breakpoint_list_size_];
      memcpy(tmp, breakpoint_list_, old_brk_size * sizeof(IBreakpointItem*));
      delete[]breakpoint_list_;
      breakpoint_list_ = tmp;
   }

   breakpoint_list_[breakpoint_number_++] = breakpoint;

}


IBreakpointItem* BreakpointHandler::CreateBreakpoint(char* breakpoint_string)
{
   unsigned int component_found = 0;
   unsigned int address_found = 0;
   char component_string[64];
   char address_string[64];
   auto lg = strlen(breakpoint_string);
   char* working_buffer = new char[lg + 1];
   memset(working_buffer, 0,  lg + 1);
   // Trim and parse the string
   unsigned int cpt = 0;
   for (unsigned int i = 0; i < lg; i++)
   {
      //
      if (breakpoint_string[i] != ' ')
      {
         //
         if (breakpoint_string[i] == ':')
         {
            // Component
            component_found = 1;
            working_buffer[cpt] = '\0';
            strcpy(component_string, working_buffer);
            component_string[cpt] = '\0';

            cpt = 0;
            memset(working_buffer, 0, lg + 1);
         }
         else if (breakpoint_string[i] == '=')
         {
            // Address
            address_found = 1;
            working_buffer[cpt] = '\0';
            strcpy(address_string, working_buffer);
            address_string[cpt] = '\0';

            cpt = 0;
            memset(working_buffer, 0, lg + 1);
         }
         else
         {
            working_buffer[cpt++] = breakpoint_string[i];
         }
      }
   }

   IBreakpointItem* breakpoint = NULL;
   if (component_found == 0)
   {
      // Parse address
      unsigned short addr = (unsigned short)strtoul(working_buffer, NULL, 16);

      // Create PC breakpoint
      breakpoint = new BreakpointPC(machine_, addr);
   }
   else
   {
      unsigned short addr;
      unsigned char value;
      if (strcmp(component_string, "CRTC") == 0)
      {
         // Create CRTC breakpoint
      }
      else if (strcmp(component_string, "MEM") == 0)
      {
         if (address_found == 1)
         {
            addr = (unsigned short)strtoul(address_string, NULL, 16);
            value = (unsigned char)strtoul(working_buffer, NULL, 16);
            breakpoint = new BreakpointMemory(machine_, addr, value);
         }
      }
      else if (strcmp(component_string, "MEMR") == 0)
      {
         if (address_found == 1)
            addr = (unsigned short)strtoul(address_string, NULL, 16);
         else
            addr = (unsigned short)strtoul(working_buffer, NULL, 16);
         breakpoint = new BreakpointMemoryAccess(machine_, addr, true);
      }
      else if (strcmp(component_string, "MEMW") == 0)
      {
         if (address_found == 1)
            addr = (unsigned short)strtoul(address_string, NULL, 16);
         else
            addr = (unsigned short)strtoul(working_buffer, NULL, 16);
         breakpoint = new BreakpointMemoryAccess(machine_, addr, false);
      }
      else if (strcmp(component_string, "REG") == 0)
      {
         // Which one ?
         addr = (unsigned short)strtoul(working_buffer, NULL, 16);
         if (strcmp(address_string, "AF") == 0)
         {
            breakpoint = new BreakpointRegisterValue(machine_, &(machine_->GetProc()->af_.w), addr);
         }
         else if (strcmp(address_string, "BC") == 0)
         {
            breakpoint = new BreakpointRegisterValue(machine_, &(machine_->GetProc()->bc_.w), addr);
         }
         else if (strcmp(address_string, "DE") == 0)
         {
            breakpoint = new BreakpointRegisterValue(machine_, &(machine_->GetProc()->de_.w), addr);
         }
         else if (strcmp(address_string, "HL") == 0)
         {
            breakpoint = new BreakpointRegisterValue(machine_, &(machine_->GetProc()->hl_.w), addr);
         }
         else if (strcmp(address_string, "IX") == 0)
         {
            breakpoint = new BreakpointRegisterValue(machine_, &(machine_->GetProc()->ix_.w), addr);
         }
         else if (strcmp(address_string, "IY") == 0)
         {
            breakpoint = new BreakpointRegisterValue(machine_, &(machine_->GetProc()->iy_.w), addr);
         }
      }
   }
   if (breakpoint != NULL)
   {
      AddBreakpoint(breakpoint);
   }

   // ...
   return breakpoint;
}

/////////////////////////////////////////////////////////////////////////////
// Specific breakpoints
BreakpointPC::BreakpointPC(EmulatorEngine* machine, unsigned short addr)
{
   break_address_ = addr;
   machine_ = machine;
}

bool BreakpointPC::Break()
{
   return (machine_->GetProc()->GetPC() == break_address_);
}

BreakpointMemory::BreakpointMemory(EmulatorEngine* machine, unsigned short addr, unsigned char value)
{
   memory_address_ = addr; memory_ = machine->GetMem(); memory_value_ = value;
}

bool BreakpointMemory::Break()
{
   return (memory_->Get(memory_address_) == memory_value_);
}

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

BreakpointRegisterValue::BreakpointRegisterValue(EmulatorEngine* machine, unsigned short* reg, unsigned short value)
{
   register_ = reg; memory_ = machine->GetMem(); register_value_ = value;
}

bool BreakpointRegisterValue::Break()
{
   return (*register_ == register_value_);
}
