#pragma once

#include "simple_vector.hpp"
#include <string>

#include "Media.h"

#define CPCCOREEMU_API 

/////////////////////////////////////////////////
// Container class : contains from 1 to n file. 
// Source cas be :
// - File
// - Directory
// - Zip file

class CPCCOREEMU_API MediaContainer
{
public:

   MediaContainer();
   virtual ~MediaContainer(void);

   // Set the container file : Either single file, zip, or directory
   virtual void AddSourceFile ( std::string path );

   ////////////////
   // Iterator
   class MediaIterator
   {
   public:
      MediaIterator(MediaContainer* container);
      virtual ~MediaIterator();

      virtual Media* GetFirst();
      virtual Media* GetNext();
      virtual Media* GetCurrent();
      virtual bool IsDone();

   private:
      MediaContainer* container_;
      std::vector<Media>::iterator it_;
   };

   MediaIterator* GetIterator() {return new MediaIterator(this);};
   void ReleaseIterator(MediaIterator* it) {delete it;}

protected:

   bool BuildFromZippedFile(std::string path);
   void BuildFromDirectory(std::string path);
   void BuildFromFile(std::string path);



   std::vector<Media> media_list_;
};

