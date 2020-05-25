#include "stdafx.h"
#include "BreakpointToken.h"

#include "Machine.h"


/////////////////////////////////////////////////////////////////////////////
// CBreakPoint

 Token::TokenDefinitions token_definitions_ [] =
 {
    { Token::PARENTHESIS_OPEN, std::bind(&Token::FindString, std::placeholders::_1, "(", std::placeholders::_2) },
    { Token::PARENTHESIS_CLOSE, std::bind(&Token::FindString, std::placeholders::_1, ")", std::placeholders::_2) },
    { Token::CONDITION, std::bind(&Token::FindString, std::placeholders::_1, "=", std::placeholders::_2) },
    { Token::OPERATION, std::bind(&Token::FindString, std::placeholders::_1, "*", std::placeholders::_2) },
    { Token::OPERATION, std::bind(&Token::FindString, std::placeholders::_1, "/", std::placeholders::_2) },
    { Token::OPERATION, std::bind(&Token::FindString, std::placeholders::_1, "+", std::placeholders::_2) },
    { Token::OPERATION, std::bind(&Token::FindString, std::placeholders::_1, "-", std::placeholders::_2) },
    { Token::VARIABLE, std::bind(&Token::FindRegister, std::placeholders::_1, std::placeholders::_2) },
    { Token::VARIABLE, std::bind(&Token::FindValue, std::placeholders::_1, std::placeholders::_2) },
 };

Token::Token(TokenType token_type):token_type_(token_type), group_token_list_(nullptr)
{
   if (token_type_==Token::GROUP)
   {
      group_token_list_ = new std::deque<Token>;
   }
}

std::deque<Token>* Token::GetGroup()
{
   return group_token_list_;
}

size_t Token::FindValue(std::string str, int& token_length)
{
   // convert to lower
   std::transform(str.begin(), str.end(), str.begin(), ::toupper);
   // find register
   unsigned int value;
   if (sscanf (str.c_str(), "%xH", &value) == 1)
   {
      token_length = str.size();
      return 0;
   }
   return std::string::npos;
}

size_t Token::FindRegister(std::string str, int& token_length)
{
   // convert to lower
   std::transform(str.begin(), str.end(), str.begin(), ::toupper);
   // find register
   if (str.compare("PC") == 0)
   {
      token_length = 2;
      return 0;
   }
   return std::string::npos;
}

unsigned int Token::ParseToken(std::string str, std::deque<Token>& token_list)
{
   // look at parenthesis
   for (auto &it: token_definitions_)
   {
      int size;
      size_t indice = it.find_(str, size);
      if (indice != std::string::npos)
      {
         // Parse before
         if (indice > 0)
         {
            ParseToken(str.substr(0, indice), token_list);
         }
         // Add token to list
         token_list.push_back(it.token_type);

         // parse after
         if (indice + 1 + size < str.size())
            ParseToken(str.substr(indice + size + 1), token_list);
         return 1;
      }
   }
   
   return 0;
}

IBreakpointItem* Token::CreateBreakpoint()
{
   // Depending on the token :
   return nullptr;
}

TokenTree::~TokenTree()
{
   
}

bool TokenConditionOperationEquality::IsEqual(TokenValue* value_left, TokenValue* value_right)
{
   return (value_left->GetValue() == value_right->GetValue());
}

TokenCondition::TokenCondition(TokenValue* value_left, TokenConditionOperation *operation, TokenValue* value_right):Token(CONDITION),
   value_left_(value_left), operation_(operation), value_right_(value_right)
{

}

bool TokenCondition::IsEqual()
{
   return operation_->IsEqual(value_left_, value_right_);
}
