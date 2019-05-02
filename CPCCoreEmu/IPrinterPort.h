#pragma once

class IPrinterPort 
{
public :
      virtual void Out ( unsigned char ) = 0;
      virtual bool Busy () = 0;
};
