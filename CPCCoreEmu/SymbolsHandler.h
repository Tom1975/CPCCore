// SymbolLoader.hpp - Parser generique de fichiers symboles Z80 (C++17)
// Supporte :
//   - .sym WinAPE/RASM/Caprice : "LABEL  &8000" ou "8000 LABEL"
//   - Hex : &8000, 8000h, 0x8000, 8000
//   - .map (z88dk/SDCC basiques) : lignes "  8000  _main"
//   - .noi (no$*) : "8000h  START"
//   - listings simples : lignes avec adresse en tête puis mnémonique/label
//
// Licence : MIT (faites-en ce que vous voulez)

#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <optional>

class SymbolsHandler
{
public:

   struct Symbol 
   {
      uint16_t address{};
      unsigned char rom_number;
      std::string label;
      std::string sourceLine; // pour debug/traces
   };

   struct Alias
   {
      std::string label;
      uint32_t value;
   };

   SymbolsHandler();
   virtual ~SymbolsHandler();

private:
   class SymbolTable
   {
   public:
      void clear() 
      {
         aliases_.clear(); aliases_by_addr_.clear(); aliases_by_label_.clear();
         symbols_.clear(); symbols_by_addr_.clear(); symbols_by_label_.clear();
      }

      // Aliases
      std::vector<Alias> aliases_;
      std::unordered_multimap<uint32_t, std::string> aliases_by_addr_;
      std::unordered_map<std::string, uint16_t> aliases_by_label_;


      // Symbols
      std::vector<Symbol> symbols_;
      std::unordered_multimap<uint16_t, std::string> symbols_by_addr_;
      std::unordered_map<std::string, uint16_t> symbols_by_label_;
   };

   static int load_symbols_rasm(const std::string& filename, SymbolTable *symbol_table);

public:
   int load_symbols(const std::string& filename, bool search);
   

   // --- Helpers d'utilisation ---------------------------------------------------

   std::optional<uint16_t> find_address(const std::string& label);
   std::vector<std::string> find_labels(uint16_t address);

private:
   SymbolTable symbol_table_;

};
