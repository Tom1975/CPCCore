#include "stdafx.h"
#include "MediaManager.h"

#include "simple_regex.h"
#include "simple_stdio.h"


IDisk* Disk::CreateDisk(ILoadingProgress* loading_progress, ILog* log)
{
   IDisk* disk = nullptr;

   std::vector<SingleElements> list_elements = contained_element_->GetInnerElements(3);
   std::vector<FormatType::TrackItem> list_item;
   for (auto& list_element : list_elements)
   {
      FormatType::TrackItem item;
      item.buffer = list_element.buffer_;
      item.size = list_element.size_;
      item.path = list_element.filename_;
      list_item.push_back(item);
   }


   if (disk_builder_.LoadDisk(list_item, disk, loading_progress) == 0)
   {
      disk->SetName(list_elements[0].filename_);
   }
   return disk;
}

MediaManager::MediaManager(DataContainer* container): container_(container)
{
   Init();
}

MediaManager::~MediaManager()
{
   ClearList();
}

void MediaManager::ClearList()
{
   for (auto& it : media_list_)
   {
      delete []it;
   }
   media_list_.clear();
}

void MediaManager::Init()
{
   ClearList();

   std::vector<IContainedElement*> list_of_elements = container_->GetFileList();

   // Check buffer :
   for (auto& list_of_element : list_of_elements)
   {
      const int type = list_of_element->GetType();

      // Add a media type
      switch (type)
      {
      case MEDIA_DISK:
         media_list_.push_back(new Disk(type, list_of_element));
         break;
      case MEDIA_TAPE:
         media_list_.push_back(new Tape(type));
         break;
      case MEDIA_BIN:
         media_list_.push_back(new Binary(type));
         break;
      case MEDIA_SNA:
         media_list_.push_back(new Snapshot(type));
         break;
      case MEDIA_SNR:
         media_list_.push_back(new SnapshotReplay(type));
         break;
      case MEDIA_CPR:
         media_list_.push_back(new CPR(type));
         break;
      case MEDIA_XPR:
         media_list_.push_back(new XPR(type));
         break;
      default: // unhandled type
         break;
      }
   }
}

int MediaManager::GetType(std::vector<MediaType> wanted_types)
{
   int result = MEDIA_UNDEF;
   int nb_media = 0;
   for (auto& it : media_list_)
   {
      // Maybe a use of Strategy pattern would do the job
      // RULES :
      // - Do not keep unwanted types
      int local_result = MEDIA_UNDEF;
      for (auto& wanted_type : wanted_types)
      {
         if (wanted_type == it->GetType())
         {
            local_result = wanted_type;
            break;
         }
      }
      // - No mixed types
      if (result != MEDIA_UNDEF && local_result != MEDIA_UNDEF && local_result != result)
      {
         return MEDIA_UNDEF;
      }
      // - No more than 2 DISK
      if (local_result == MEDIA_DISK && nb_media >= 2)
      {
         return MEDIA_UNDEF;
      }
      else
      {
         // - Unlimited number of tracks or ROM
         // - No more than 1 of any other
      }
      result = local_result;
      nb_media++;
   }
   return result;
}

std::vector<IDisk*> MediaManager::GetDisk(ILoadingProgress* loading_progress, ILog* log)
{
   std::vector<IDisk*> disk_list;
   // Return every disk found
   for (auto& it : media_list_)
   {
      if (it->GetType() == MEDIA_DISK)
      {
         // Add it : From buffer,
         Disk* disk = (Disk*)(it);
         IDisk* disk_image = disk->CreateDisk(loading_progress, log);
         if (disk_image != nullptr)
            disk_list.push_back(disk_image);
      }
   }
   return disk_list;
}

int MediaManager::GetTypeFromBuffer(unsigned char* buffer, int size)
{
   if (memcmp(buffer, "MV - SNA", 8) == 0)
      return MEDIA_SNA;
   else if (memcmp(buffer, "RW - SNR", 8) == 0)
      return MEDIA_SNR;
   else if ((memcmp(buffer, "MV - CPC", 8) == 0)
      || (memcmp(buffer, "EXTENDED", 8) == 0)
      || (memcmp(buffer, "SCP", 3) == 0)
      || (memcmp(buffer, "CAPS", 4) == 0)
      || (memcmp(buffer, "HXCPICFE", 8) == 0)
      || (memcmp(buffer, "HXCHFEV3", 8) == 0)
      
#ifdef SUPPORT_SFWR
      || (memcmp(buffer, "FORM", 4) == 0)
#endif
   )
      // SNA, ROM or DSK ?
      return MEDIA_DISK;
      // Tape : CDT, TAP
   else if ((memcmp(buffer, "ZXTape!", 7) == 0)
      || (size >= 12 && (memcmp(buffer, "RIFF", 4) == 0) && (memcmp(&buffer[8], "WAVE", 4) == 0))
      || (memcmp(buffer, "Compressed Square Wave", 22) == 0)
      || (memcmp(buffer, "Creative Voice File", 19) == 0)
   )
      return MEDIA_TAPE;
   else if (size >= 12 && (memcmp(&buffer[8], "AMS!", 4) == 0) && (memcmp(&buffer[0], "RIFF", 4) == 0))
   {
      return MEDIA_CPR;
   }
   else if (size >= 12 && (memcmp(&buffer[8], "CXME", 4) == 0) && (memcmp(&buffer[0], "RIFF", 4) == 0))
   {
      return MEDIA_XPR;
   }
   else return MEDIA_UNDEF;
}

int MediaManager::GetTypeFromFile(const char*  str)
{
   int return_type = MEDIA_UNDEF;
   // Check buffer
   FILE* file;

   if (fopen_s(&file, str, "rb") == 0)
   {
      unsigned char buffer [0x100] = {0};
      // check the 22 first char
      fread(buffer, 1, 0x100, file);
      fclose(file);

      return_type = GetTypeFromBuffer(buffer, 0x100);
   }

   if (return_type == MEDIA_UNDEF)
   {
      // Check file extension
      if (IsExtensionMatch(str, "rom"))
      {
         return_type = MEDIA_ROM;
      }
      else if (IsExtensionMatch(str, "raw"))
      {
         return_type = MEDIA_DISK;
      }
      else if (IsExtensionMatch(str,"tap"))
      {
         return_type = MEDIA_TAPE;
      }
      else if (IsExtensionMatch(str,"bin"))
      {
         return_type = MEDIA_BIN;
      }
      else if (IsExtensionMatch(str,"cpr"))
      {
         return_type = MEDIA_CPR;
      }
      else if (IsExtensionMatch(str, "xpr"))
      {
         return_type = MEDIA_XPR;
      }
   }

   return return_type;
}

int MediaManager::GetTypeFromMultipleTypes(int* type_list, int nb_types)
{
   return MEDIA_UNDEF;
}
