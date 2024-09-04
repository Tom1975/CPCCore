#pragma once

#include <vector>
#include <map>
#include <string>

#include "IConfiguration.h"

class MultiLanguage
{
public:
   MultiLanguage(IConfiguration* manager);
   virtual ~MultiLanguage();

   void Init(const char* label_file);

   // Language list
   unsigned int GetLanguageNumber();
   const char* GetLanguage(unsigned int index);
   void ChangeLanguage(unsigned int index);

   const char* GetString(const char* id);

protected:
   IConfiguration* manager_;

   // Languages list
   std::vector<std::string> language_list_;

   // Language ids
   std::map<std::string, std::string> current_language_;
};

