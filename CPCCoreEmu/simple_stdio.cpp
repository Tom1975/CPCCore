
#include "simple_stdio.h"

#ifdef __circle__

#include <circle/addon/fatfs/ff.h>
#include <circle/logger.h>

class File
{
public:
   File()
   {
      handle = new FIL;
   }
   virtual ~File()
   {
      delete handle;
   }

   FIL* handle;
};

BYTE GetMode(const char* mode)
{
   BYTE mode_byte = FA_READ | FA_WRITE | FA_CREATE_ALWAYS | FA_CREATE_NEW | FA_OPEN_ALWAYS | FA_OPEN_APPEND ;
   return mode_byte;
}

errno_t fopen_s(
   FILE**      _Stream,
   char const* _FileName,
   char const* _Mode
)
{
   File* f = new File ();
   
   CLogger::Get ()->Write ("FILE", LogNotice, "f_open %s - mode %s", _FileName, _Mode);

   if (f_open ( (f->handle), _FileName, FA_READ | FA_OPEN_EXISTING) != FR_OK)
   {
      CLogger::Get ()->Write ("FILE", LogNotice, "f_open failed");
      delete f;
      return -1;
   }
   CLogger::Get ()->Write ("FILE", LogNotice, "f_open Ok");
   *_Stream = (void*)f;
   return 0;
}

int fclose(FILE* _Stream)
{
   
   FRESULT res = f_close ( ((File*)_Stream)->handle);
   CLogger::Get ()->Write ("FILE", LogNotice, "f_close : %i", res);
   return res == FR_OK;
}

int fseek(FILE* _Stream, long  _Offset, int   _Origin)
{
   CLogger::Get ()->Write ("FILE", LogNotice, "f_seek : offset = %i; origin = %i", _Offset, _Origin);
   switch (_Origin)
   {
      case SEEK_SET:
         f_lseek ( ((File*)_Stream)->handle, _Offset);
         break;
      case SEEK_END:
         f_lseek ( ((File*)_Stream)->handle, f_size(((File*)_Stream)->handle));
         break;
      case SEEK_CUR:
         f_lseek ( ((File*)_Stream)->handle, f_tell (((File*)_Stream)->handle)+ _Offset);
         break;
         break;
   }
   return 0;
}

void rewind( FILE* _Stream) 
{
   f_rewind ( ((File*)_Stream)->handle);
}

long ftell(FILE* _Stream)
{
   return f_tell (((File*)_Stream)->handle);
}

unsigned int fread(
   void* _Buffer,
   unsigned int _ElementSize,
   unsigned int _ElementCount,
   FILE* _Stream
)
{
   CLogger::Get ()->Write ("FILE", LogNotice, "f_read size = %i; element_count = %i", _ElementSize, _ElementCount);
   unsigned int byte_read;
   if ( f_read( ((File*)_Stream)->handle, _Buffer, _ElementSize*_ElementCount, &byte_read) == FR_OK)
   {
      CLogger::Get ()->Write ("FILE", LogNotice, "f_read ok");
      return byte_read / _ElementSize;
   }
   else
   {
      CLogger::Get ()->Write ("FILE", LogNotice, "*** f_read error");
      return 0;
   }

}

unsigned int fwrite(
   void const* _Buffer,
   unsigned int _ElementSize,
   unsigned int _ElementCount,
   FILE* _Stream
)
{
   
   unsigned int byte_written;
   if (f_write (((File*)_Stream)->handle, _Buffer, _ElementSize*_ElementCount, &byte_written) == FR_OK)
   {
      CLogger::Get ()->Write ("FILE", LogNotice, "fwrite ok");
      return byte_written / _ElementSize;
   }
   else
   {
      CLogger::Get ()->Write ("FILE", LogNotice, "*** fwrite errror");
      return 0;
   }
}

#endif