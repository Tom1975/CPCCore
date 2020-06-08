#include "stdafx.h"
#include "BreakpointToken.h"

#include "Machine.h"


/////////////////////////////////////////////////////////////////////////////
// CBreakPoint

std::function< Token* (std::string, EmulatorEngine* emulator, int& pos_of_token, int& size_of_token)> TokenBuilder::token_type_list []=
{
   &TokenImmediateValue::StringToToken,
   &TokenRegisterValue::StringToToken,
   &TokenConditionOperationEquality::StringToToken,
};

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

Token::Token(TokenType token_type, EmulatorEngine* emulator) :token_type_(token_type), group_token_list_(nullptr), emulator_(emulator)
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

Token* TokenBuilder::StringToToken(std::string str, EmulatorEngine* emulator, std::deque<Token*>& token_list, int& pos_of_token, int& size_of_token)
{
   Token* token = nullptr;

   for (auto &it : token_type_list)
   {
      int pos, size;

      Token* token = it(str, emulator, pos, size);
      if (token != nullptr)
      {
         // Parse before
         if (pos > 0)
         {
            int first_pos, first_size;
            if (StringToToken(str.substr(0, pos), emulator, token_list, first_pos, first_size) == nullptr)
            {
               // Error
               return nullptr;
            }
         }

         token_list.push_back(token);

         if (pos + 1 + size < str.size())
         {
            if (StringToToken(str.substr(pos + size + 1), emulator, token_list, pos, size) == nullptr)
            {
               return nullptr;
            }
         }
         return token;
      }
      
   }
   return nullptr;
}

unsigned int Token::ParseToken(std::string str, std::deque<Token>& token_list)
{
   /*
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
   */
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


TokenCondition::TokenCondition(TokenValue* value_left, TokenConditionOperation *operation, TokenValue* value_right, EmulatorEngine* emulator) :Token(CONDITION, emulator),
value_left_(value_left), operation_(operation), value_right_(value_right)
{

}

bool TokenCondition::IsEqual()
{
   return operation_->IsEqual(value_left_, value_right_);
}

bool TokenConditionOperationEquality::IsEqual(TokenValue* value_left, TokenValue* value_right)
{
   return (value_left->GetValue() == value_right->GetValue());
}

Token* TokenConditionOperationEquality::StringToToken(std::string str, EmulatorEngine* emulator, int& pos_of_token, int& size_of_token)
{
   // convert to lower
   std::transform(str.begin(), str.end(), str.begin(), ::toupper);
   // find register
   int token_length;
   size_t indice = FindString (str, "=", token_length);
   if (indice != std::string::npos) 
   {
      // Set PC as register
      pos_of_token = indice; 
      size_of_token = token_length;
      return new TokenConditionOperationEquality(emulator);
   }
   return nullptr;
}

TokenRegisterValue::TokenRegisterValue(EmulatorEngine* emulator): TokenValue( emulator)
{

}

Token* TokenRegisterValue::StringToToken(std::string str, EmulatorEngine* emulator, int& pos_of_token, int& size_of_token)
{
   // convert to lower
   std::transform(str.begin(), str.end(), str.begin(), ::toupper);
   // find register
   if (str.compare("PC") == 0)
   {
      // Set PC as register
      pos_of_token = 0;
      size_of_token = str.size();
      return new TokenRegisterValue( emulator);
   }
   return nullptr;
}

Token* TokenImmediateValue::StringToToken(std::string str, EmulatorEngine* emulator, int& pos_of_token, int& size_of_token)
{
   Token* token = nullptr;
   // Parse a string, to get an immediate value.
   // convert to lower
   std::transform(str.begin(), str.end(), str.begin(), ::toupper);
   // find value
   unsigned int value;
   if (sscanf(str.c_str(), "%xH", &value) == 1)
   {
      pos_of_token = 0;
      size_of_token = str.size();
      token = new TokenImmediateValue(value, emulator);
   }
   return token;
}

