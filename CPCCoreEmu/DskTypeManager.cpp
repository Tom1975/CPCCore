#include "stdafx.h"
#include "DskTypeManager.h"
#include "FileAccess.h"
#include <regex>



DskTypeManager::DskTypeManager(void)
{
}


DskTypeManager::~DskTypeManager(void)
{
}


int DskTypeManager::GetTypeFromBuffer (unsigned char* buffer, int size)
{
   if (buffer == nullptr) return 0;

   if (memcmp( buffer, "MV - SNA", 8) == 0)
      return 1;
   else if (memcmp( buffer, "RW - SNR", 8) == 0)
      return 5;
   else if (  (memcmp( buffer, "MV - CPC", 8) == 0)
         || (memcmp( buffer, "EXTENDED", 8) == 0)
         || ( memcmp ( buffer, "SCP", 3) == 0)
         || ( memcmp ( buffer, "CAPS", 4) == 0)
         || ( memcmp ( buffer, "HXCPICFE", 8) == 0)
#ifdef SUPPORT_SFWR
         || ( memcmp ( pBuffer, "FORM", 4) == 0)
#endif
         )
      // SNA, ROM or DSK ?
      return 3;
   // Tape : CDT, TAP
   else if ( (memcmp( buffer, "ZXTape!", 7) == 0)
            || (size >= 12 && (memcmp(buffer, "RIFF", 4) == 0) && (memcmp(&buffer[8], "WAVE", 4) == 0))
            || (memcmp( buffer, "Compressed Square Wave", 22) == 0)
            || (memcmp ( buffer, "Creative Voice File", 19) == 0)
            )
      return 4;
   else if (size >= 12 && (memcmp(&buffer[8], "AMS!", 4) == 0) && (memcmp(&buffer[0], "RIFF", 4) == 0))
   {
      return 8; // kCPR
   }

   return 0;
}

int DskTypeManager::GetTypeFromFile(std::string str)
{
   //if (PathMatchSpec(str.c_str(), _T("*.rom")) == TRUE)
   if ( MatchTextWithWildcards (str, "*.rom"))
   //if (regex_match(str.c_str(), std::regex("*\\.rom")))
   {
      return 2;
   }
   else //if (PathMatchSpec(str.c_str(), _T("*.raw")) == TRUE)
      //if (regex_match(str.c_str(), std::regex("*\\.raw")))
      if (MatchTextWithWildcards(str, "*.raw"))
   {
      return 3;
   }
   else //if (PathMatchSpec(str.c_str(), _T("*.tap")) == TRUE)
      //if (regex_match(str.c_str(), std::regex("*\\.tap")))
      if (MatchTextWithWildcards(str, "*.tap"))
   {
      return 4;
   }
   else //if (PathMatchSpec(str.c_str(), _T("*.bin")) == TRUE)
      if (MatchTextWithWildcards(str, "*.bin"))
      //if (regex_match(str.c_str(), std::regex("*\\.bin")))
   {
      return 6;
   }
   else //if (PathMatchSpec(str.c_str(), _T("*.cpr")) == TRUE)
      if (MatchTextWithWildcards(str, "*.cpr"))
      //if (regex_match(str.c_str(), std::regex("*\\.cpr")))
   {
      return 7;
   }
   return 0;
}

int DskTypeManager::GetTypeFromMultipleTypes ( int * type_list, int nb_types )
{
   return 0;
}

