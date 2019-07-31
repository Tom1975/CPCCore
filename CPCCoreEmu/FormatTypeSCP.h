#pragma once
#include "IDisk.h"
#include "DiskBuilder.h"

class FormatTypeSCP : public FormatType
{
public:
   FormatTypeSCP();
   virtual ~FormatTypeSCP();

   virtual const char* GetFormatName()
   {
      return "SCP";
   }

   virtual const char* GetFormatDescriptor()
   {
      return "Supercard Pro";
   }

   virtual const char* GetFormatExt()
   {
      return "scp";
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

private:
   unsigned int ComputeTrack(unsigned char* bitfield, const unsigned char* cellbuffer, int size_of_buffer,
                             int duration);

   typedef struct
   {
      unsigned int offset;
   } TrackDataHeader;

   TrackDataHeader tracks_header_[166];
   double rpm_correction_;
};
