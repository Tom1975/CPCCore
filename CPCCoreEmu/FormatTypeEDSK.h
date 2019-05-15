#pragma once
#include "IDisk.h"
#include "DiskBuilder.h"


// 12x0 - A1A1A1FE - CHRN - CRC = 22 byte = 22*16 MFM bits  - 22*16 = 352
#define MFMCHRNSIZE 352


class FormatTypeEDSK : public FormatType
{
public:
   FormatTypeEDSK();
   virtual ~FormatTypeEDSK();

   virtual const char* GetFormatName()
   {
      return "EDSK";
   }

   virtual const char* GetFormatDescriptor()
   {
      return "Extended Dsk";
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
      return true;
   };

   //////////////////////////////////////////////////////////
   // Buffer versions of loader
   // todo

   //////////////////////////////////////////////////////////
   // File versions of loader
   virtual bool CanLoad(const char*);
   virtual int LoadDisk(const char* file_path, IDisk*& created_disk, ILoadingProgress* loading_progress = nullptr);
   virtual int LoadDisk(std::vector<TrackItem> file_list, IDisk*& created_disk,
                        ILoadingProgress* loading_progress = nullptr);
   virtual int LoadDisk(const unsigned char* buffer, size_t size, IDisk*& created_disk,
                        ILoadingProgress* loading_progress = nullptr);
   virtual int SaveDisk(const char* file_path, IDisk* disk, ILoadingProgress* loading_progress = nullptr) const;


private:


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
      unsigned char mfm_chrn[MFMCHRNSIZE];
      unsigned char* mfm_data;
      unsigned char mfm_crc[2 * 16];
      bool crc_present;
      unsigned char* mfm_data_for_comparison;
      int mfm_data_size;
      int mfm_offset_idam;
      int mfm_offset_dam;
      int nb_recorded_revolutions;
      int whole_size_recorded;
      int index_header;
      int index_data;
   } Sector;

   typedef struct
   {
      bool formatted;
      unsigned int sector_size;
      unsigned char sz;
      unsigned int nb_sector;
      unsigned char gap3;
      unsigned char gap3_filler;
      Sector* sectors;
      int track_size;

      unsigned int mfm_bit_size;
   } Track;

   typedef struct
   {
      unsigned char nb_tracks;
      Track* tracks;
   } Side;

   int GetNbRevolutions(int side, int track);
   int FillTrackMfm(IDisk* new_disk, int side, int track, unsigned int index_revolution);
   void ConvertChrntoMfm(Sector* sector);
   void ConvertDatatoMfm(Sector* sector, unsigned int index_revolution);
   unsigned int LookforInterlaced(unsigned char* track_byte, int search_base_index, int index_bit, Sector* sector,
                                  unsigned int* returned_offset_first_sync_before_idam, bool first_sector);

   int nb_tracks_;
   Side side_[2]; // Max two side

   bool extended_offset_;
};
