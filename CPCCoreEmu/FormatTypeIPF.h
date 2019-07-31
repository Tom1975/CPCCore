#pragma once

#include "IDisk.h"

#include "DiskBuilder.h"

#if defined (__unix) || (RASPPI) || (__APPLE__)
#define fopen_s(pFile,filename,mode) ((*(pFile))=fopen((filename),(mode)))==NULL
#endif

class FormatTypeIPF : public FormatType
{
public:
   FormatTypeIPF();
   virtual ~FormatTypeIPF();

   virtual const char* GetFormatName()
   {
      return "IPF";
   }

   virtual const char* GetFormatDescriptor()
   {
      return "IPF";
   }

   virtual const char* GetFormatExt()
   {
      return "ipf";
   }

   virtual bool CanLoad() const
   {
      return true;
   }

   virtual bool CanSave() const
   {
      return true;
   }

   //////////////////////////////////////////////////////////
   // Buffer versions of loader
   // todo

   //////////////////////////////////////////////////////////
   // File versions of loader
   virtual bool CanLoad(const char* file_path);
   virtual int LoadDisk(const char* file_path, IDisk*& created_disk, ILoadingProgress* loading_progress = nullptr);
   virtual int LoadDisk(std::vector<TrackItem> file_list, IDisk*& created_disk,
                        ILoadingProgress* loading_progress = nullptr);
   virtual int LoadDisk(const unsigned char* buffer, size_t size, IDisk*& created_disk,
                        ILoadingProgress* loading_progress = nullptr);
   virtual int SaveDisk(const char* file_path, IDisk* disk, ILoadingProgress* loading_progress = nullptr) const;
};
