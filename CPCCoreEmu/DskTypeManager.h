#pragma once

#include "ILog.h"
#include "DiskContainer.h"



class CDskTypeManager : public ITypeManager
{
public:

   CDskTypeManager(void);
   virtual ~CDskTypeManager(void);

   //////////////////////////////////////////////////////////
   // Implementation of ITypeManager
   virtual int GetTypeFromFile(std::string str);
   virtual int GetTypeFromBuffer (unsigned char* buffer, int size);
   virtual int GetTypeFromMultipleTypes ( int * type_list, int nb_types );


};

