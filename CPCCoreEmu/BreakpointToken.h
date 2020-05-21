
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
   Token(TokenType);

   TokenType GetType() { return token_type_;  }
   std::deque<Token>* GetGroup();

   IBreakpointItem* CreateBreakpoint();

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
