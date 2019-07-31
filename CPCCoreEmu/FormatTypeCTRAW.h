#pragma once
#include "IDisk.h"
#include "DiskBuilder.h"

class FormatTypeCTRAW : public FormatType
{
public:
   FormatTypeCTRAW();
   virtual ~FormatTypeCTRAW();

   virtual const char* GetFormatName()
   {
      return "CTRAW";
   }

   virtual const char* GetFormatDescriptor()
   {
      return "CT-RAW";
   }

   virtual const char* GetFormatExt()
   {
      return "raw";
   }

   virtual bool CanLoad() const
   {
      return true;
   }

   virtual bool CanSave() const
   {
      return false;
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
