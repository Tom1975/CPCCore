#include "stdafx.h"
#include "MediaContainer.h"
#include <stdio.h>
#include "zlib.h"


/////////////////////////////////////////////////////////////////
// MediaIterator Implementation
/////////////////////////////////////////////////////////////////
MediaContainer::MediaIterator::MediaIterator(MediaContainer* container) :container_(container)
{

}

MediaContainer::MediaIterator::~MediaIterator()
{

}

/////////////////////////////////////////////////////////////////
// MediaContainer Implementation
/////////////////////////////////////////////////////////////////
MediaContainer::MediaContainer()
{
}

MediaContainer::~MediaContainer()
{
   // Clear Media list
}

void MediaContainer::AddSourceFile (std::string path)
{
   // Create associated media This is a media factory
   // Is this a file or a directory ?
   //DWORD attrib = GetFileAttributes(path.c_str());

   // Directory
//   if ((attrib & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
   struct stat s;

   if (stat(path.c_str(), &s) == 0)
   {
      // Directory
      if (s.st_mode & S_IFDIR)
      {
         BuildFromDirectory(path);
      }
      else
      {
         // Is this a compressed file ?
         BuildFromFile(path);
      }
   }
}


void MediaContainer::BuildFromDirectory(std::string /*path*/)
{
   // Check directory content.
   //IContainedElement *newElement = new DirectoryFile(type_manager_, m_CurrentPath.c_str());
   //media_list_.push_back(newElement);

}

void MediaContainer::BuildFromFile(std::string path)
{
   if (BuildFromZippedFile(path) == false)
   {
      //media_list_.push_back(newElement);
   }

}

bool  MediaContainer::BuildFromZippedFile(std::string path)
{
   bool bRet = false;
   // Open zipped content, then check it
   FILE * f;
   if (fopen_s(&f, path.c_str(), "rb") == 0)
   {
      unsigned char zipped_buffer[4];
      const bool read_ok = (fread(zipped_buffer, 4, 1, f) == 1);
      fclose(f);

      if (read_ok && *(unsigned int*)zipped_buffer == 0x04034b50)
      {
         //IContainedElement *newElement = new ZippedFile(type_manager_, m_CurrentPath);
         //media_list_.push_back(newElement);
      }

   }
   return bRet;
}
