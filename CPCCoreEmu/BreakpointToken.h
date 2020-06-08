
#pragma once
#include <deque>
#include <functional>

#include "Breakpoint.h"

class EmulatorEngine;
class Memory;

#define NB_BP_MAX    100


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
   Token(TokenType, EmulatorEngine* emulator);

   TokenType GetType() { return token_type_;  }
   std::deque<Token>* GetGroup();

   IBreakpointItem* CreateBreakpoint();

   virtual bool IsEqual() { return false; }

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
   EmulatorEngine* emulator_;
};

class TokenValue : public Token
{
public:
   TokenValue(EmulatorEngine* emulator) : Token(VALUE, emulator) {};
   virtual ~TokenValue() {};

   virtual int GetValue() = 0;
};

class TokenImmediateValue : public TokenValue
{
public:
   TokenImmediateValue(unsigned int value, EmulatorEngine* emulator) : TokenValue(emulator), value_(value) {}

   static Token* StringToToken(std::string, EmulatorEngine* emulator, int& pos_of_token, int& size_of_token);
   virtual int GetValue() { return value_; }

protected:
   unsigned int value_;
}; 

class TokenRegisterValue : public TokenValue
{
public:
   TokenRegisterValue(EmulatorEngine* emulator);

   static Token* StringToToken(std::string, EmulatorEngine* emulator, int& pos_of_token, int& size_of_token);
   virtual int GetValue() { return 0; }

protected:
   
};

class TokenConditionOperation : public Token
{
public:
   TokenConditionOperation(EmulatorEngine* emulator) :Token(CONDITION, emulator) {}
   virtual bool IsEqual(TokenValue* value_left, TokenValue* value_right) = 0;

};

class TokenConditionOperationEquality : public TokenConditionOperation
{
   public:
      TokenConditionOperationEquality(EmulatorEngine* emulator) :TokenConditionOperation(emulator) {}
      virtual bool IsEqual(TokenValue* value_left, TokenValue* value_right);

      static Token* StringToToken(std::string, EmulatorEngine* emulator, int& pos_of_token, int& size_of_token);
};

class TokenCondition : public Token
{
public:
   TokenCondition(TokenValue *value_left, TokenConditionOperation *operation, TokenValue *value_right, EmulatorEngine* emulator);
   
   virtual bool IsEqual();

protected:
   TokenValue *value_left_;
   TokenConditionOperation* operation_;
   TokenValue *value_right_;
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

class TokenBuilder
{
public : 
   static Token* StringToToken(std::string, EmulatorEngine* emulator, std::deque<Token*>& token_list, int& pos_of_token, int& size_of_token);

   static std::function< Token* (std::string, EmulatorEngine* emulator, int& pos_of_token, int& size_of_token)> token_type_list [];
};
