
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

class Token
{
public:

   ///////////////////////////////////
   // Token - 
   typedef enum {
      PARENTHESIS_OPEN,
      PARENTHESIS_CLOSE,
      CONDITION,
      GROUPED_CONDITION,
      BOOL_CONDITION,
      VARIABLE,
      OPERATION,
      VALUE,
      GROUP
   }TokenType;

   ///////////////////////////////////
   Token(TokenType);

   TokenType GetType() { return token_type_;  }
   std::deque<Token>* GetGroup();

   ///////////////////////////////////
   // Token definition
   typedef struct
   {
      TokenType token_type;
      std::function< size_t( std::string, int& size_of_token)> find_;
   }TokenDefinitions;



   ///////////////////////////////////
   // Helper functions
   static size_t FindRegister(std::string base_string, int& token_length);
   static size_t FindValue(std::string base_string, int& token_length);
   static size_t FindString ( std::string base_string, std::string token, int& token_length)
   {
      token_length = token.size();
      return base_string.find(token);
   };
   static unsigned int ParseToken(std::string str, std::deque<Token>& token_list);

   
   
   
protected:
   TokenType token_type_;
   std::deque<Token>* group_token_list_;
};

class TokenTree
{
public:
   TokenTree(std::deque<Token> token_list);
   virtual ~TokenTree();

protected:
   // Tree is either  :
   // - leaf
   // - Operation between 2 leafs
   enum
   {
      None,
      Comparison,
      Bool,
      Math
   }OperationType;
   TokenTree *left_;
   TokenTree *right_;
   
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

   void ClearBreakpoints();
   void CreateBreakpoint(int indice, std::deque<std::string> param);
   void EnableBreakpoints();
   void DisableBreakpoints();
   void EnableBreakpoint(int bp_number);
   void DisableBreakpoint(int bp_number);


protected:

   // String parsing
   unsigned int ParseToken(std::string str, std::deque<Token>& token_list);

   // Breakpoints data
   EmulatorEngine* machine_;

   IBreakpointItem** breakpoint_list_;
   int breakpoint_number_;
   int breakpoint_list_size_;

   bool gloal_breakpoints_enabled_;
   bool breakpoints_enabled_[NB_BP_MAX];

};


