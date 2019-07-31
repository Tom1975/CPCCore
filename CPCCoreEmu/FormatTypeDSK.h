#pragma once
#include "IDisk.h"
#include "DiskBuilder.h"

class FormatTypeDSK : public FormatType
{
public:
   FormatTypeDSK();
   virtual ~FormatTypeDSK();

   virtual const char* GetFormatName() { return "DSK"; }

   virtual const char* GetFormatDescriptor()
   {
      return "Dsk";
   }

   virtual const char* GetFormatExt()
   {
      return "dsk";
   }

   virtual bool CanLoad() const
   {
      return true;
   };

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

protected:
   int FillTrack(unsigned char* track_buffer, int side, int track);

   typedef struct
   {
      unsigned char track;
      unsigned char side;
      unsigned char sector_id;
      unsigned char sector_size;
      unsigned char fdc_status_1;
      unsigned char fdc_status_2;
      unsigned short actual_size;
      unsigned short sector_index;
      unsigned char* data;
   } Sector;

   typedef struct
   {
      bool formatted;
      unsigned int sector_size;
      unsigned char sz;
      unsigned int nb_sector;
      unsigned char gap3;
      unsigned char gap3_filter;
      Sector* sectors;
      unsigned int track_size;
   } Track;

   typedef struct
   {
      unsigned char nb_tracks;
      Track* tracks;
   } Side;


   Side side_[2]; // Max two side
};
