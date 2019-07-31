#pragma once
#include "IDisk.h"
#include "DiskBuilder.h"

class FormatTypeHFE : public FormatType
{
public:
   FormatTypeHFE();
   virtual ~FormatTypeHFE();

   virtual const char* GetFormatName();

   virtual const char* GetFormatDescriptor();

   virtual const char* GetFormatExt();

   virtual bool CanLoad() const;

   virtual bool CanSave() const;

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
