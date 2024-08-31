
#include "simple_regex.h"

#include <regex>

static size_t ReplaceAll(std::string &str, const std::string &from, const std::string &to)
{
   size_t count = 0;

   size_t pos = 0;
   while ((pos = str.find(from, pos)) != std::string::npos)
   {
      str.replace(pos, from.length(), to);
      pos += to.length();
      ++count;
   }

   return count;
}

static void EscapeRegex(std::string &regex)
{
   ReplaceAll(regex, "\\", "\\\\");
   ReplaceAll(regex, "^", "\\^");
   ReplaceAll(regex, ".", "\\.");
   ReplaceAll(regex, "$", "\\$");
   ReplaceAll(regex, "|", "\\|");
   ReplaceAll(regex, "(", "\\(");
   ReplaceAll(regex, ")", "\\)");
   ReplaceAll(regex, "[", "\\[");
   ReplaceAll(regex, "]", "\\]");
   ReplaceAll(regex, "*", "\\*");
   ReplaceAll(regex, "+", "\\+");
   ReplaceAll(regex, "?", "\\?");
   ReplaceAll(regex, "/", "\\/");
}

static bool MatchTextWithWildcards(const std::string &text, std::string wildcardPattern /*, bool caseSensitive = true*/)
{
   // Escape all regex special chars
   EscapeRegex(wildcardPattern);

   // Convert chars '*?' back to their regex equivalents
   ReplaceAll(wildcardPattern, "\\?", ".");
   ReplaceAll(wildcardPattern, "\\*", ".*");

   std::regex pattern(wildcardPattern /*, caseSensitive ? std::regex::normal : std::regex::icase*/);

   return std::regex_match(text, pattern);
}

bool IsExtensionMatch(const char* str, const char* ext)
{
   std::string extension = "*.";
   extension += ext;
   return MatchTextWithWildcards(str, extension);
}

//#endif