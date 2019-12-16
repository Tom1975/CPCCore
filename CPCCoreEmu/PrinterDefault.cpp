#include "stdafx.h"
#include "PrinterDefault.h"
//#include "shlwapi.h"

#define SCR_PATH "PRINT"

PrinterDefault::PrinterDefault(void)
{
   busy_ = false;
   diretories_ = NULL;
   file_ptr_ = NULL;
}


PrinterDefault::~PrinterDefault(void)
{
#ifndef MINIMUM_DEPENDENCIES
   if (file_ptr_ != NULL)
   {
      fclose ( file_ptr_);
   }
#endif
}

void PrinterDefault::Out ( unsigned char c)
{
#ifndef MINIMUM_DEPENDENCIES
   char buff[8] = { 0 };
   sprintf ( buff, "%c", c&0x7F );
   if (!busy_ && c & 0x80 )
   {
      // Strobe : Write char to default
      if ( file_ptr_ == NULL)
      {
         // Get a new name
         char file_name [MAX_PATH];
         if ( GetNewPrinterFile (file_name, MAX_PATH))
         {
            if (fopen_s(&file_ptr_, file_name, "w") != 0)
               file_ptr_ = NULL;;
         }
      }

      if ( file_ptr_ != NULL )
      {
         fwrite ( buff, 1, 1, file_ptr_ );
      }

      busy_ = true;
   }
#endif
}

bool PrinterDefault::Busy ()
{
   bool bRet = busy_ ;
   busy_  = false;
   return bRet ;
}


bool PrinterDefault::GetNewPrinterFile (char * buffer, unsigned int size)
{
#ifndef MINIMUM_DEPENDENCIES
   if (diretories_ == NULL)
      return false;

   fs::path exe_path(diretories_->GetBaseDirectory());
   exe_path /= SCR_PATH;
   const char* file_path = exe_path.string().c_str();

   // Create new sound file
   bool name_ok = false;
   unsigned int inc = 0;

   while (!name_ok && inc<= 9999)
   {
      sprintf ( buffer, "%sPRN%4.4i.TXT", file_path, inc);

      // Exists ?  inc and retry
      fs::path print_path (buffer);

      if (fs::is_regular_file( fs::status ( print_path)))

      /*WIN32_FIND_DATA FindFileData;
      HANDLE handle = FindFirstFile(buffer, &FindFileData) ;

      if (handle != INVALID_HANDLE_VALUE)*/
      {
         inc ++;
      }
      else
      {
         name_ok = true;
      }
   }
   return name_ok;
#else
   return false;
#endif
}
