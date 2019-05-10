#include "stdafx.h"
#include "DskTypeManager.h"
#include "simple_regex.h"



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

int DskTypeManager::GetTypeFromFile(const char* str)
{
   if (IsExtensionMatch(str, "rom"))
   {
      return 2;
   }
   else if (IsExtensionMatch(str, "raw"))
   {
      return 3;
   }
   else if (IsExtensionMatch(str, "tap"))
   {
      return 4;
   }
   else if (IsExtensionMatch(str, "bin"))
   {
      return 6;
   }
   else if (IsExtensionMatch(str, "cpr"))
   {
      return 7;
   }
   return 0;
}

int DskTypeManager::GetTypeFromMultipleTypes ( int * type_list, int nb_types )
{
   return 0;
}

