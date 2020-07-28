
#pragma once
#include <deque>
#include <functional>
#include <string>
#include "Breakpoint.h"
#include <map>

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

   virtual IBreakpointItem* CreateBreakpoint();

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

class TokenPCValue: public TokenValue
{
public:
   TokenPCValue(EmulatorEngine* emulator) : TokenValue(emulator){}
   int GetValue();

};


template <typename T>
class TokenRegisterValue : public TokenValue
{
public:
   typedef enum
   {
      REG_A,
      REG_F,
      REG_B,
      REG_C,
      REG_D,
      REG_E,
      REG_H,
      REG_L,
      REG_PC,
      REG_AF,
      REG_BC,
      REG_DE,
      REG_HL,

   } RegisterType;

   TokenRegisterValue(T* reg_ptr, EmulatorEngine* emulator) : TokenValue(emulator)
   {
      value_ = reg_ptr;
   }

   int GetValue() { return (int)*value_; }

   static Token* StringToToken(std::string, EmulatorEngine* emulator, int& pos_of_token, int& size_of_token);
   static T* GetRegister(RegisterType, EmulatorEngine* emulator);

protected:
   static std::map<std::string, RegisterType > register_token_list_;

   T* value_;
};

class TokenConditionOperation : public Token
{
public:
   TokenConditionOperation(EmulatorEngine* emulator) :Token(CONDITION, emulator), value_left_(nullptr), value_right_(nullptr) {}

   void SetOperationMembers(TokenValue* value_left, TokenValue* value_right);
   virtual bool IsEqual(TokenValue* value_left, TokenValue* value_right) = 0;

protected:
   TokenValue* value_left_;
   TokenValue* value_right_;
};

class TokenConditionOperationEquality : public TokenConditionOperation
{
   public:
      TokenConditionOperationEquality(EmulatorEngine* emulator) :TokenConditionOperation(emulator) {}
      virtual bool IsEqual(TokenValue* value_left, TokenValue* value_right);
      IBreakpointItem* CreateBreakpoint();


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
