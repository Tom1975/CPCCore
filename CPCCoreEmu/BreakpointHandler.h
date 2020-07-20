
#pragma once
#include <deque>
#include <functional>

#include "Breakpoint.h"
#include "BreakpointToken.h"

class EmulatorEngine;
class Memory;

#define NB_BP_MAX    100

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

   void ClearBreakpoints();
   void CreateBreakpoint(int indice, std::deque<std::string> param);
   void EnableBreakpoints();
   void DisableBreakpoints();
   void EnableBreakpoint(int bp_number);
   void DisableBreakpoint(int bp_number);


protected:

   // String parsing
   unsigned int ParseToken(std::string str, std::deque<Token>& token_list);
   IBreakpointItem* ConditionHandling(std::deque<Token*> &token_list);
   int BuildExpression(std::deque<Token*>& token_list);
   TokenValue* CreateVariable(std::deque<Token*> token_list);
   TokenConditionOperation* CreateOperation(Token* token);
   
   // Breakpoints data
   EmulatorEngine* machine_;

   IBreakpointItem** breakpoint_list_;
   int breakpoint_number_;
   int breakpoint_list_size_;

   bool gloal_breakpoints_enabled_;
   bool breakpoints_enabled_[NB_BP_MAX];

};


