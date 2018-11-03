#include "stdafx.h"
#include "DiskBuilder.h"

#include "FormatTypeCTRAW.h"
#include "FormatTypeEDSK.h"
#include "FormatTypeDSK.h"
#include "FormatTypeHFE.h"
#include "FormatTypeRAW.h"
#include "FormatTypeSCP.h"
#include "FormatTypeIPF.h"

DiskBuilder::DiskBuilder(void)
{
   // Fill the format list.

   format_list_.push_back(new FormatTypeCTRAW()); // CTRAW
   format_list_.push_back(new FormatTypeDSK()); // DSK
   format_list_.push_back(new FormatTypeEDSK()); // EDSK
   format_list_.push_back(new FormatTypeHFE()); // HFE
   format_list_.push_back(new FormatTypeIPF()); // IPF
   format_list_.push_back(new FormatTypeSCP()); // SCP
   format_list_.push_back(new FormatTypeRAW()); // RAW - Last one : No real means to know if a file can be loaded ...
}

DiskBuilder::~DiskBuilder(void)
{
   // Clear the format list
   for (auto it = format_list_.begin(); it != format_list_.end(); it++)
   {
      delete(*it);
   }
}

/////////////////////////////////////////////////////////////////////
// Supported formats
std::vector<FormatType*> DiskBuilder::GetFormatsList(DiskBuilder::FormatAction action)
{
   std::vector<FormatType*> format_list;

   for (auto it = format_list_.begin(); it != format_list_.end(); it++)
   {
      bool add = false;
      switch (action)
      {
      case READ:
         add = (*it)->CanLoad();
         break;
      case WRITE:
         add = (*it)->CanSave();
         break;
      }
      if (add)
      {
         format_list.push_back(*it);
      }
   }
   return format_list;
}

/////////////////////////////////////////////////////////////////////
// Load disk from file

bool DiskBuilder::CanLoad(const char* file_path, FormatType*& format)
{
   for (auto it = format_list_.begin(); it != format_list_.end(); it++)
   {
      if ((*it)->CanLoad(file_path))
      {
         format = *it;
         return true;
      }
   }
   return false;
}

int DiskBuilder::LoadDisk(const char* file_path, IDisk*& created_disk, ILoadingProgress* loading_progress)
{
   // Select correct builder
   for (auto it = format_list_.begin(); it != format_list_.end(); it++)
   {
      if ((*it)->CanLoad(file_path))
      {
         return (*it)->LoadDisk(file_path, created_disk, loading_progress);
      }
   }

   // Nobody can load this file
   return -1;
}

int DiskBuilder::LoadDisk(const unsigned char* buffer, size_t size, IDisk*& created_disk,
                          ILoadingProgress* loading_progress)
{
   for (auto it = format_list_.begin(); it != format_list_.end(); it++)
   {
      if ((*it)->LoadDisk(buffer, size, created_disk, loading_progress) == 0)
         return 0;;
   }
   return -1;
}

int DiskBuilder::LoadDisk(std::vector<FormatType::TrackItem> file_list, IDisk*& created_disk,
                          ILoadingProgress* loading_progress)
{
   for (auto it = format_list_.begin(); it != format_list_.end(); it++)
   {
      if ((*it)->LoadDisk(file_list, created_disk, loading_progress) == 0)
         return 0;;
   }
   return -1;
}

int DiskBuilder::SaveDisk(const char* file_path, IDisk* disk, const char* format, ILoadingProgress* loading_progress)
{
   for (auto it = format_list_.begin(); it != format_list_.end(); it++)
   {
      if (strcmp((*it)->GetFormatName(), format) == 0)
      {
         return (*it)->SaveDisk(file_path, disk, loading_progress);;
      }
   }
   return -1;
}

int DiskBuilder::SaveDisk(const char* file_path, IDisk* disk, const FormatType* format,
                          ILoadingProgress* loading_progress)
{
   // Use correct format
   if (format->CanSave())
   {
      return format->SaveDisk(file_path, disk, loading_progress);
   }

   // No save can be done !
   return -1;
}
