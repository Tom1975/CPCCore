#include <map>
#include <functional>
#include <filesystem>

#include "SymbolsHandler.h"

SymbolsHandler::SymbolsHandler()
{

}

SymbolsHandler::~SymbolsHandler()
{

}

int SymbolsHandler::load_symbols(const std::string& filename, bool search)
{

   // Loading symbols : .sym, .map, .noi, .lst, rasm
   std::map<std::string, std::function<int(const std::string&, SymbolTable*)>> supported_extension = 
   {  /*"sym", "map", "noi", "lst",*/
      {"rasm", load_symbols_rasm }
   };

   // 
   // Shall we try to find a file with supported extension ?
   if (search)
   {
      // Try to find any file with supported extension : n
      for (auto it : supported_extension)
      {
         std::filesystem::path file_path_symbols(filename);
         file_path_symbols.replace_extension(it.first);
         if (it.second(file_path_symbols.string(), &symbol_table_) == 0)
         {
            // Found !
            return 0;
         }
      }
      return -1;
   }
   else
   {
      //return symbols_handler_.load_symbols(file_path);
   }
   return 0;
}

int SymbolsHandler::load_symbols_rasm(const std::string& filename, SymbolTable* symbol_table)
{
   std::ifstream f(filename);
   if (!f) return -1;

   symbol_table->clear();

   std::string token;
   while (std::getline(f, token, ';')) {
      if (token.empty()) continue;

      std::istringstream line(token);
      std::string type;
      line >> type;

      if (type == "romlabel") 
      {
         std::string label;
         uint32_t addr, bank;
         line >> label >> addr >> bank;

         label += ':';
         Symbol s{ addr, bank, label, line.str()};
         symbol_table->symbols_.push_back(s);
         symbol_table->symbols_by_addr_.emplace(s.address, s.label);
         symbol_table->symbols_by_label_[s.label] = s.address;
      }
      else if (type == "alias") 
      {
         std::string name;
         uint32_t value;
         line >> name >> value;
         Alias a{ name, value};
         symbol_table->aliases_.push_back(a);
         symbol_table->aliases_by_addr_.emplace(a.value, a.label);
         symbol_table->aliases_by_label_[a.label] = a.value;
      }
   }
   return 0;
}

std::optional<uint16_t> SymbolsHandler::find_address(const std::string& label) 
{
   auto it = symbol_table_.symbols_by_label_.find(label);
   if (it == symbol_table_.symbols_by_label_.end()) return std::nullopt;
   return it->second;
}

std::vector<std::string> SymbolsHandler::find_labels(uint16_t address) 
{
   std::vector<std::string> out;
   auto range = symbol_table_.symbols_by_addr_.equal_range(address);
   for (auto it = range.first; it != range.second; ++it) {
      out.push_back(it->second);
   }
   return out;
}