#pragma once
#include "IPrinterPort.h"
#include "IDirectories.h"

class CPrinterDefault :
   public IPrinterPort
{
public:
   CPrinterDefault(void);
   virtual ~CPrinterDefault(void);

   virtual void SetDirectories (IDirectories * diretories){diretories_ = diretories;};
   virtual void Out ( unsigned char );
   virtual bool Busy () ;

protected:
   bool GetNewPrinterFile (char * buffer, unsigned int size);
   IDirectories * diretories_;
   bool busy_;

   FILE* file_ptr_;
};

