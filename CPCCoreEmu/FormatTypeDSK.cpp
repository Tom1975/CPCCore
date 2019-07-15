#include "stdafx.h"
#include "FormatTypeDSK.h"
#include "simple_stdio.h"


#if defined (__unix) || (RASPPI) || (__APPLE__)
#define fopen_s(pFile,filename,mode) ((*(pFile))=fopen((filename),(mode)))==NULL
#endif

FormatTypeDSK::FormatTypeDSK()
{
}


FormatTypeDSK::~FormatTypeDSK()
{
}

bool FormatTypeDSK::CanLoad(const char* file_path)
{
   // Check MV - CPC version 
   FILE* file;
   bool can_load = false;

   if (fopen_s(&file, file_path, "rb") == 0)
   {
      // Check for type of file from header
      unsigned char header[0x8] = {0};
      // check the 22 first char
      fread(header, 1, 0x8, file);
      if (memcmp(header, "MV - CPC", 8) == 0)
      {
         can_load = true;
      }
      fclose(file);
   }
   return can_load;
}

int FormatTypeDSK::LoadDisk(const char* file_path, IDisk*& created_disk, ILoadingProgress* loading_progress)
{
   int return_value = -1;
   FILE* file;

   if (fopen_s(&file, file_path, "rb") == 0)
   {
      // Read whole file
      fseek(file, 0, SEEK_END);
      unsigned int size = ftell(file);
      rewind(file);
      unsigned char* buffer = new unsigned char[size];
      if (fread(buffer, 1, size, file) == size)
      {
         return_value = LoadDisk(buffer, size, created_disk, loading_progress);
      }
      else
      {
         return_value = -1;
      }
      delete[]buffer;

      fclose(file);
   }
   else
   {
      // Erreur : File not found
      return_value = -1;
   }
   return return_value;
}

int FormatTypeDSK::LoadDisk(const unsigned char* buffer, size_t size, IDisk*& created_disk,
                            ILoadingProgress* loading_progress)
{
   if (memcmp(buffer, "MV - CPC", 8) == 0)
   {
      IDisk* new_disk = new IDisk();

      // Name of creator : don't read
      // Number of tracks, and sides
      int nb_tracks = buffer[0x30];
      new_disk->nb_sides_ = buffer[0x31];

      if (new_disk->nb_sides_ > 2)
      {
         // ERROR ! Handle or correct ?
         new_disk->nb_sides_ = 2;
      }

      // Size of tracks
      int size_of_track = (buffer[0x32] + ((buffer[0x33]) << 8)) - 0x100;
      //m_NbSide = m_NbSide;

      // Format disk on memory
      int i, j;
      unsigned int k;
      for (i = 0; i < new_disk->nb_sides_; i++)
      {
         side_[i].nb_tracks = nb_tracks;
         side_[i].tracks = new Track[side_[i].nb_tracks];
         memset(side_[i].tracks, 0, sizeof(Track) * side_[i].nb_tracks);
      }


      // Read the tracks
      unsigned char* track_block = new unsigned char[size_of_track];

      int index_buffer = 0x100;
      for (j = 0; j < nb_tracks * new_disk->nb_sides_; j++)
      {
         const unsigned char* header = &buffer[index_buffer];
         index_buffer += 0x100;

         //fread(header, 0x100, 1, file);
         // Side number
         i = header[0x11];

         //fread(TrackBlock, m_SizeOfTrack, 1, file);
         memcpy(track_block, &buffer[index_buffer], size_of_track);
         index_buffer += size_of_track;
         // Track number
         unsigned int tn = header[0x10];
         if (tn >= side_[i].nb_tracks)
         {
            // bug...
            tn = side_[i].nb_tracks - 1;
         }
         unsigned char sector_size = side_[i].tracks[tn].sz = header[0x14];
         //Sz = sectorSize_L;
         side_[i].tracks[tn].sector_size = (0x80 << sector_size);
         // Number of sector
         side_[i].tracks[tn].nb_sector = header[0x15];
         side_[i].tracks[tn].gap3 = header[0x16];
         side_[i].tracks[tn].gap3_filter = 0x4E; // header [0x17];
         side_[i].tracks[tn].sectors = new Sector[side_[i].tracks[tn].nb_sector];
         // Sector info
         for (k = 0; k < side_[i].tracks[tn].nb_sector; k++)
         {
            side_[i].tracks[tn].sectors[k].track = header[0x18 + k * 8 + 00];
            side_[i].tracks[tn].sectors[k].side = header[0x18 + k * 8 + 01];
            side_[i].tracks[tn].sectors[k].sector_id = header[0x18 + k * 8 + 02];
            side_[i].tracks[tn].sectors[k].sector_size = header[0x18 + k * 8 + 03];
            side_[i].tracks[tn].sectors[k].fdc_status_1 = header[0x18 + k * 8 + 04];
            side_[i].tracks[tn].sectors[k].fdc_status_2 = header[0x18 + k * 8 + 05];

            side_[i].tracks[tn].sectors[k].actual_size = side_[i].tracks[tn].sector_size;
         }

         // Data
         unsigned int offset = 0;
         for (k = 0; k < side_[i].tracks[tn].nb_sector; k++)
         {
            side_[i].tracks[tn].sectors[k].data = new unsigned char[side_[i].tracks[tn].sector_size];
            memcpy(side_[i].tracks[tn].sectors[k].data, &track_block[offset], side_[i].tracks[tn].sector_size);
            offset += side_[i].tracks[tn].sector_size;
         }
      }
      delete[]track_block;

      // Fill the MFM structure
      for (int side = 0; side < new_disk->nb_sides_; side++)
      {
         new_disk->side_[side].tracks = new IDisk::MFMTrack[side_[side].nb_tracks < 42 ? 42 : side_[side].nb_tracks];
         new_disk->side_[side].nb_tracks = side_[side].nb_tracks;

         for (unsigned int track = 0; track < new_disk->side_[side].nb_tracks; track++)
         {
            // Init space
            new_disk->side_[side].tracks[track].nb_revolutions = 1; // Only one for DSK files
            new_disk->side_[side].tracks[track].revolution = new IDisk::Revolution[1];

            new_disk->side_[side].tracks[track].revolution[0].size = FillTrack(NULL, side, track);
            if (new_disk->side_[side].tracks[track].revolution[0].size < DEFAULT_TRACK_SIZE * 16)
            {
               new_disk->side_[side].tracks[track].revolution[0].bitfield = new unsigned char[DEFAULT_TRACK_SIZE * 16];
            }
            else
            {
               new_disk->side_[side].tracks[track].revolution[0].bitfield = new unsigned char[new_disk->side_[side].
                  tracks[track].revolution[0].size];
            }
            memset(new_disk->side_[side].tracks[track].revolution[0].bitfield, 0,
                   new_disk->side_[side].tracks[track].revolution[0].size);

            FillTrack(new_disk->side_[side].tracks[track].revolution[0].bitfield, side, track);
         }
         if (new_disk->side_[side].nb_tracks < 42)
         {
            for (unsigned int track = new_disk->side_[side].nb_tracks; track < 42; track++)
            {
               new_disk->side_[side].tracks[track].nb_revolutions = 1; // Only one for DSK files
               new_disk->side_[side].tracks[track].revolution = new IDisk::Revolution[1];

               new_disk->side_[side].tracks[track].revolution[0].size = DEFAULT_TRACK_SIZE * 16;
               new_disk->side_[side].tracks[track].revolution[0].bitfield = new unsigned char[new_disk->side_[side].
                  tracks[track].revolution[0].size];
               memset(new_disk->side_[side].tracks[track].revolution[0].bitfield, 0, DEFAULT_TRACK_SIZE * 16);
            }
         }
      }
      new_disk->CreateTrackFromMultiRevolutions();

      created_disk = new_disk;

      return OK;
   }
   return FILE_ERROR;
}

int FormatTypeDSK::SaveDisk(const char* file_path, IDisk* disk, ILoadingProgress* loading_progress) const
{
   return NOT_IMPLEMENTED;
}

int FormatTypeDSK::FillTrack(unsigned char* track_buffer, int side, int track)
{
   int j, N;

   int load_disk_index = 0;

   // GAP 4a : 80 byte with value 0x4E
   for (j = 0; j < 80; j++) load_disk_index = IDisk::AddByteToTrack(track_buffer, load_disk_index, 0x4E);

   // Sync : 12 byte with value 0x00
   for (j = 0; j < 12; j++) load_disk_index = IDisk::AddByteToTrack(track_buffer, load_disk_index, 0x00);

   // IAM : 3 byte with 0xC2, then one byte with 0xFC
   load_disk_index = IDisk::AddSyncByteToTrack(track_buffer, load_disk_index, 0xC2);
   load_disk_index = IDisk::AddSyncByteToTrack(track_buffer, load_disk_index, 0xC2);
   load_disk_index = IDisk::AddSyncByteToTrack(track_buffer, load_disk_index, 0xC2);
   load_disk_index = IDisk::AddByteToTrack(track_buffer, load_disk_index, 0xFC);

   // GAP 1 : 50 byte with 0x4E value
   for (j = 0; j < 50; j++) load_disk_index = IDisk::AddByteToTrack(track_buffer, load_disk_index, 0x4E);

   unsigned int last_valid_data = 0;

   // Then, each sectors :
   for (unsigned int s = 0; s < side_[side].tracks[track].nb_sector; s++)
   {
      CRC crc;
      // - Sync : 12 byte, value #00
      crc.Reset();
      for (j = 0; j < 12; j++) load_disk_index = IDisk::AddByteWithCrc(track_buffer, load_disk_index, 0x00, crc, false);

      crc.Reset();
      // - IDAM : 3 byte #A1 + 1 byte #FE
      load_disk_index = IDisk::AddByteWithCrc(track_buffer, load_disk_index, 0xA1, crc, true);
      load_disk_index = IDisk::AddByteWithCrc(track_buffer, load_disk_index, 0xA1, crc, true);
      load_disk_index = IDisk::AddByteWithCrc(track_buffer, load_disk_index, 0xA1, crc, true);
      load_disk_index = IDisk::AddByteWithCrc(track_buffer, load_disk_index, 0xFE, crc);

      // Index
      // - C (identification secteur 'C' = 1 byte)
      load_disk_index = IDisk::AddByteWithCrc(track_buffer, load_disk_index, side_[side].tracks[track].sectors[s].track,
                                            crc);
      // - H (identification secteur 'H' = 1 byte)
      load_disk_index = IDisk::AddByteWithCrc(track_buffer, load_disk_index, side_[side].tracks[track].sectors[s].side,
                                            crc);
      // - R (identification secteur 'R' = 1 byte)
      load_disk_index = IDisk::AddByteWithCrc(track_buffer, load_disk_index, side_[side].tracks[track].sectors[s].sector_id,
                                            crc);
      // - N (identification secteur 'N' = 1 byte)
      N = side_[side].tracks[track].sectors[s].sector_size;
      load_disk_index = IDisk::AddByteWithCrc(track_buffer, load_disk_index, N, crc);

      // - Crc (2 bytes) - TODO ???
      //if (NsizeOfTrack_L == sizeOfTrack_L)
      //   if (false)
      {
         unsigned short computed_crc = crc.GetCRC();
         load_disk_index = IDisk::AddByteWithCrc(track_buffer, load_disk_index, (computed_crc >> 8), crc);
         load_disk_index = IDisk::AddByteWithCrc(track_buffer, load_disk_index, (computed_crc & 0xFF), crc);
      }

      crc.Reset();

      // - GAP 2 (22 bytes #4E)
      for (j = 0; j < 22; j++) load_disk_index = IDisk::AddByteWithCrc(track_buffer, load_disk_index, 0x4E, crc);

      // - Sync : 12 bytes #00
      for (j = 0; j < 12; j++) load_disk_index = IDisk::AddByteWithCrc(track_buffer, load_disk_index, 0x00, crc);

      crc.Reset();
      // - DATA AM : 3 bytes #A1 + 1 bytes #FB ou #F8
      load_disk_index = IDisk::AddByteWithCrc(track_buffer, load_disk_index, 0xA1, crc, true);
      load_disk_index = IDisk::AddByteWithCrc(track_buffer, load_disk_index, 0xA1, crc, true);
      load_disk_index = IDisk::AddByteWithCrc(track_buffer, load_disk_index, 0xA1, crc, true);

      if ((side_[side].tracks[track].sectors[s].fdc_status_2 & 0x40) == 0x40)
         load_disk_index = IDisk::AddByteWithCrc(track_buffer, load_disk_index, DAAM_ERASED, crc);
      else
         load_disk_index = IDisk::AddByteWithCrc(track_buffer, load_disk_index, DAAM_OFF, crc);

      // - n datas of sector (512 by default)
      unsigned int size_of_track = side_[side].tracks[track].sectors[s].actual_size;

      // With extension, CRC and GAP#3 are in the floppy
      for (unsigned int k = 0; k < size_of_track; k++)
      {
         load_disk_index = IDisk::AddByteWithCrc(track_buffer, load_disk_index,
                                               side_[side].tracks[track].sectors[s].data[k], crc);
      }

      // - Crc (2 bytes) - If needed....
      unsigned short computed_crc = crc.GetCRC();
      load_disk_index = IDisk::AddByteWithoutCrc(track_buffer, load_disk_index, (computed_crc >> 8));
      load_disk_index = IDisk::AddByteWithoutCrc(track_buffer, load_disk_index, (computed_crc & 0xFF));

      crc.Reset();

      // - GAP #3 (x bytes)
      for (j = 0; j < side_[side].tracks[track].gap3; j++)
      {
         load_disk_index = IDisk::AddByteWithoutCrc(track_buffer, load_disk_index, side_[side].tracks[track].gap3_filter);
      }
   }

   // End of track : GAP 4B until DEFAULT_TRACK_SIZE bytes.
   for (; load_disk_index < DEFAULT_TRACK_SIZE; load_disk_index++)
   {
      load_disk_index = IDisk::AddByteWithoutCrc(track_buffer, load_disk_index, 0x4E);
   }

   return load_disk_index;
}

int FormatTypeDSK::LoadDisk(std::vector<TrackItem> file_list, IDisk*& created_disk, ILoadingProgress* loading_progress)
{
   if (file_list.size() == 1)
   {
      return LoadDisk(file_list[0].buffer, file_list[0].size, created_disk, loading_progress);
   }
   else
   {
      return NOT_IMPLEMENTED;
   }
}
