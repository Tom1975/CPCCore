#pragma once

class ILog
{
public :

   virtual void WriteLog ( const char* log ) = 0;
   virtual void WriteLogByte ( unsigned char number ) = 0;
   virtual void WriteLogShort ( unsigned short number ) = 0;
   virtual void WriteLog ( unsigned int number ) = 0;
   virtual void EndOfLine () = 0;

};
