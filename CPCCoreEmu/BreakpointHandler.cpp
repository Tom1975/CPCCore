#include "stdafx.h"
#include "BreakpointHandler.h"
#include <algorithm>

#include "Machine.h"


/////////////////////////////////////////////////////////////////////////////
// CBreakPoint

BreakpointHandler::BreakpointHandler()
{
   machine_ = nullptr;
   breakpoint_list_size_ = 10;
   breakpoint_list_ = new IBreakpointItem*[breakpoint_list_size_];
   breakpoint_number_ = 0;
   gloal_breakpoints_enabled_ = false;
   for (auto i = 0; i < NB_BP_MAX; i++)
   {
      breakpoints_enabled_[i] = false;
   }
}

BreakpointHandler::~BreakpointHandler()
{
   delete[]breakpoint_list_;
}


TokenValue* BreakpointHandler::CreateVariable(std::deque<Token*> token_list)
{
   TokenValue* value = nullptr;
   // TODO !
   return value;
}

TokenConditionOperation* BreakpointHandler::CreateOperation(Token* token)
{
   TokenConditionOperation* operation = nullptr;
   // TODO !
   return operation;
}

IBreakpointItem* BreakpointHandler::ConditionHandling(std::deque<Token*> &token_list)
{
   IBreakpointItem* item = nullptr;
   std::deque<Token> out_token_list;

   for (auto i = 0; i < token_list.size(); i++)
   {
      if ( i > 0 && i+1 < token_list.size() && token_list[i]->GetType() == Token::CONDITION)
      {
         // We should have something before AND after
         // Create right and left leaf (which should be 'variable' meta type)
         std::deque<Token*> right_list;
         std::deque<Token*> left_list;
         std::copy(token_list.begin(), token_list.begin()+i, std::back_inserter(left_list));
         std::copy(token_list.begin() + (i + 1), token_list.end(), std::back_inserter(right_list));

         TokenValue* value_left = CreateVariable(left_list);
         if (value_left == nullptr)
         {
            return nullptr;
         }
         TokenValue* value_right = CreateVariable(right_list);
         if (value_left == nullptr)
         {
            delete value_left;
            return nullptr;
         }
         TokenConditionOperation* operation = CreateOperation(token_list[i]);
         if (operation == nullptr)
         {
            delete value_left;
            delete value_right;
            return nullptr;
         }
         // Create the tree node 
         TokenCondition * token_condition = new TokenCondition ( value_left, operation, value_right, machine_);
         return token_condition->CreateBreakpoint();
      }
   }
   return 0;
}

int OperationHandling(std::deque<Token*> &token_list)
{
   /*
   std::deque<Token> out_token_list;
   for (auto i = 0; i < token_list.size(); i++)
   {
      if (i + 1 < token_list.size() && token_list[i + 1].GetType() == Token::OPERATION)
      {
         // We should have something before AND after
         if (i + 2 < token_list.size() && token_list[i].GetType() == Token::VARIABLE && token_list[i + 2].GetType() == Token::VARIABLE)
         {
            // Ok, create a condition
            
            TokenCondition* token = new TokenCondition(dynamic_cast<TokenValue*>(&token_list[i]), dynamic_cast<TokenConditionOperation*>(&token_list[i+2]), dynamic_cast<TokenValue*>(&token_list[i]));

            Token grouped_variable(Token::GROUP);
            out_token_list.push_back(grouped_variable);
            i += 2;
         }
         else
         {
            // Error : Operation is always between 2 variables
            return -1;
         }
      }
      else
      {
         // Add it to the final list
         out_token_list.push_back(token_list[i]);
      }
   }

   token_list = out_token_list;
   */
   return 0;
}

int ParenthesisHandling(std::deque<Token*> &token_list)
{
   // - Parenthesis :
   // P : ( C ) => C
   // P : ( V ) => V
   
   /*std::deque<Token*> out_token_list;
   Token * inner_list;
   std::deque<Token*>* relative_main_list = &out_token_list;
   std::deque<std::deque<Token*>*> inner_stack;

   for (auto &it : token_list)
   {
      if (it->GetType() == Token::PARENTHESIS_OPEN)
      {
         // Register a level of parenthesis; 
         inner_stack.push_back(relative_main_list);
         inner_list = new Token(Token::GROUP);
         relative_main_list = inner_list->GetGroup();
      }
      else if (it->GetType() == Token::PARENTHESIS_CLOSE)
      {
         if (inner_stack.empty()) return -1;

         std::deque<Token>* upper_main_list = inner_stack.back();
         upper_main_list->push_back(*inner_list);
         relative_main_list = upper_main_list;
      }
      else
      {
         // Building a parenthesis list ? 
         relative_main_list->push_back(it);
      }
   }
   token_list = out_token_list;
   */
   return 0;
}

int BreakpointHandler::BuildExpression(std::deque<Token*>& token_list)
{
   // Reduce tokens to tree + single tree tokens (ie : regroup parenthesis)
   //if (ParenthesisHandling(token_list) != 0) return -1; 
   // TODO

   // Apply rules :
   // Let's say C : Condition, V : Variable 
   // C : V CONDITION V
   IBreakpointItem*  breakpoint = ConditionHandling(token_list);
   //if (ConditionHandling(token_list) != 0) return -1;



   return 0;
}


bool BreakpointHandler::IsBreak()
{
   // Parcours des breakpoints
   //for (std::vector<IBreakpointItem*>::iterator it = m_BreakpointList.begin(); it != m_BreakpointList.end(); it++)
   if (gloal_breakpoints_enabled_)
   {
      for (auto i = 0; i < breakpoint_number_; i++)
      {
         // Si on break, on break !
         if (breakpoints_enabled_[i] && breakpoint_list_[i]->Break())
            return true;
      }

   }
   return false;
}

void BreakpointHandler::ClearBreakpoints()
{
   
}

void BreakpointHandler::EnableBreakpoint(int bp_number)
{
   if (bp_number > 0 && bp_number <= NB_BP_MAX)
   {
      breakpoints_enabled_[bp_number - 1] = true;
   }
}

void BreakpointHandler::EnableBreakpoints()
{
   gloal_breakpoints_enabled_ = true;
}

void BreakpointHandler::DisableBreakpoints()
{
   gloal_breakpoints_enabled_ = false;
}

void BreakpointHandler::DisableBreakpoint(int bp_number)
{
   if (bp_number > 0 && bp_number <= NB_BP_MAX)
   {
      breakpoints_enabled_[bp_number - 1] = false;
   }
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

void BreakpointHandler::CreateBreakpoint(int indice, std::deque<std::string> param)
{
   // Separate in tokens : Regroupements, Conditions, Values, Variable, Computation
   std::deque<Token> token_list;
   std::deque<Token*> subtoken_list;
   for (auto &it:param)
   {

      int pos, size;
      if ( TokenBuilder::StringToToken(it, machine_, subtoken_list, pos, size) != nullptr)
      //if ( Token::ParseToken(it, subtoken_list) > 0)
      {
         //token_list.insert(token_list.end(), subtoken_list.begin(), subtoken_list.end());
      }
      else
      {
         // error
         return;
      }
   }

   // Create binary tree
   BuildExpression(subtoken_list);

   // Now, if no error, we *should* have only one remaining item.
   if (subtoken_list.size() == 1)
   { 
      IBreakpointItem* breakpoint = subtoken_list[0]->CreateBreakpoint();
      AddBreakpoint(breakpoint);
   }
   
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
