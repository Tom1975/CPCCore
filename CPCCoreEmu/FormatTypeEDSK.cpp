#include "stdafx.h"
#include "FormatTypeEDSK.h"
#include "simple_stdio.h"


// Data pattern : 0 0 0 0 0 0 0 0 0 0 A1 A1 A1
unsigned char MFMDataPattern [15 * 16];

// Data pattern : 0 0 0 0 0 0 0 0 0 0 C2 C2 C2 FC
static unsigned char MFMStartOfTrackPattern [16 * 16];

int CopyMem(unsigned char* dest, unsigned char* src, int size, int idxdest, int maxsizedest)
{
   int sizeToCopy = (idxdest + size > maxsizedest) ? maxsizedest - idxdest : size;
   if (sizeToCopy <= 0)
      return 0;

   // Check first bit !
   if (idxdest > 0)
   {
      if (dest[idxdest - 1] == 1 && src[0] == 1)
      {
         src[0] &= ~1;
      }
   }
   else
   {
      // Otherwise : First should be at 0
      src[0] &= ~1;
   }

   memcpy(&dest[idxdest], src, sizeToCopy);
   return sizeToCopy;
}

FormatTypeEDSK::FormatTypeEDSK()
{
   // TODO : This should be performed only once !
   // Init : The sector patern
   int index_bit = 0;
   int j;
   for (j = 0; j < 12; j++) index_bit = IDisk::AddByteToTrack(MFMDataPattern, index_bit, 0x00);

   // A1A1A1
   index_bit = IDisk::AddSyncByteToTrack(MFMDataPattern, index_bit, 0xA1);
   index_bit = IDisk::AddSyncByteToTrack(MFMDataPattern, index_bit, 0xA1);
   index_bit = IDisk::AddSyncByteToTrack(MFMDataPattern, index_bit, 0xA1);

   index_bit = 0;
   for (j = 0; j < 12; j++) index_bit = IDisk::AddByteToTrack(MFMStartOfTrackPattern, index_bit, 0x00);

   // A1A1A1
   index_bit = IDisk::AddSyncByteToTrack(MFMStartOfTrackPattern, index_bit, 0xC2);
   index_bit = IDisk::AddSyncByteToTrack(MFMStartOfTrackPattern, index_bit, 0xC2);
   index_bit = IDisk::AddSyncByteToTrack(MFMStartOfTrackPattern, index_bit, 0xC2);

   IDisk::AddByteToTrack(MFMStartOfTrackPattern, index_bit, 0xFC);
}

FormatTypeEDSK::~FormatTypeEDSK()
{
}

bool FormatTypeEDSK::CanLoad(const char* file_path)
{
   // Check EXTENDED version 
   FILE* file;
   bool can_load = false;

   if (fopen_s(&file, file_path, "rb") == 0)
   {
      // Check for type of file from header
      unsigned char header[0x8] = {0};
      // check the 22 first char
      fread(header, 1, 0x8, file);
      if (memcmp(header, "EXTENDED", 8) == 0)
      {
         can_load = true;
      }
      fclose(file);
   }
   return can_load;
}

int FormatTypeEDSK::LoadDisk(const char* file_path, IDisk*& created_disk, ILoadingProgress* loading_progress)
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

int FormatTypeEDSK::LoadDisk(const unsigned char* buffer, size_t size, IDisk*& created_disk,
                             ILoadingProgress* loading_progress)
{
   if (memcmp(buffer, "EXTENDED", 8) == 0)
   {
      IDisk* new_disk = new IDisk();

      // Extended
      // Name of creator : don't read
      // Number of tracks, and sides
      int m_NbTracks = buffer[0x30];
      new_disk->nb_sides_ = buffer[0x31];

      if (new_disk->nb_sides_ > 2)
      {
         // ERROR ! - TODO
         new_disk->nb_sides_ = 2;
      }

      // Track size table
      // Format disk on memory
      int i, j;
      unsigned int k;
      for (i = 0; i < new_disk->nb_sides_; i++)
      {
         unsigned int offset = 0x34 + i;
         side_[i].nb_tracks = m_NbTracks;
         new_disk->side_[i].nb_tracks = m_NbTracks;
         side_[i].tracks = new Track[m_NbTracks];
         new_disk->side_[i].tracks = new IDisk::MFMTrack[m_NbTracks < 42 ? 42 : m_NbTracks];
         memset(new_disk->side_[i].tracks, 0, (sizeof(IDisk::MFMTrack)) * m_NbTracks);
         memset(side_[i].tracks, 0, (sizeof(Track)) * m_NbTracks);
         for (j = 0; j < m_NbTracks; j++)
         {
            side_[i].tracks[j].track_size = (buffer[offset] << 8);
            //m_Side [i].Tracks[j].Formatted = ( header[offset] != 0);
            offset += new_disk->nb_sides_;
         }
      }
      unsigned int index_header = 0x100;

      for (j = 0; j < m_NbTracks; j++)
      {
         for (i = 0; i < new_disk->nb_sides_; i++)
         {
            // Formatted ?
            const unsigned char* header = buffer;

            if (side_[i].tracks[j].track_size > 0x100)
            {
               side_[i].tracks[j].formatted = true;

               header = &buffer[index_header];
               index_header += 0x100;
               //fread(header, 0x100, 1, file);

               if (memcmp(header, "Track-Info", 10) != 0)
               {
                  delete new_disk;
                  new_disk = nullptr;
                  return -2;
               }

               unsigned int size_to_remove_for_sector = 0x100;
               unsigned int offset_from_start = 0x18;
               //unsigned char* track_block = new unsigned char [m_Side[i].Tracks [j].TrackSize];
               //fread ( track_block, m_Side[i].Tracks [j].TrackSize-0x100, 1, file );
               // Track number
               unsigned int tn = header[0x10];
               if (tn != j)
               {
                  // bug...
                  //tn = m_NbTracks-1;
                  tn = j;
               }
               // Side number
               //i = header[0x11]; - Check this !
               if (i != header[0x11])
               {
               }
               unsigned char sectorSize_L = side_[i].tracks[tn].sz = header[0x14];
               side_[i].tracks[tn].sector_size = (0x80 << sectorSize_L);
               // Number of sector
               side_[i].tracks[tn].nb_sector = header[0x15];
               side_[i].tracks[tn].gap3 = header[0x16];
               side_[i].tracks[tn].gap3_filler = 0x4E; // header [0x17];
               side_[i].tracks[tn].sectors = new Sector[side_[i].tracks[tn].nb_sector];
               // Sector info
               unsigned int count = 0;
               unsigned int offset = 0;
               unsigned int total_size = 0;

               for (k = 0; k < side_[i].tracks[tn].nb_sector; k++)
               {
                  memset(&side_[i].tracks[tn].sectors[k], 0x00, sizeof(Sector));

                  // Get next 0x100 bytes ?
                  if ((count + 1) * 8 > 0x100 - offset_from_start)
                  {
                     // Get next 0x100 bytes
                     //memcpy ( &header[0x18], track_block, 0x100-0x18 );
                     //fread(header, 0x100, 1, file);
                     header = &buffer[index_header];
                     index_header += 0x100;

                     size_to_remove_for_sector += 0x100;
                     offset_from_start = 0;
                     count = 0;
                  }
                  side_[i].tracks[tn].sectors[k].track = header[offset_from_start + count * 8 + 00];
                  side_[i].tracks[tn].sectors[k].side = header[offset_from_start + count * 8 + 01];
                  side_[i].tracks[tn].sectors[k].sector_id = header[offset_from_start + count * 8 + 02];
                  side_[i].tracks[tn].sectors[k].sector_size = header[offset_from_start + count * 8 + 03];
                  side_[i].tracks[tn].sectors[k].fdc_status_1 = header[offset_from_start + count * 8 + 04];
                  side_[i].tracks[tn].sectors[k].fdc_status_2 = header[offset_from_start + count * 8 + 05];
                  side_[i].tracks[tn].sectors[k].actual_size = header[offset_from_start + count * 8 + 06];
                  side_[i].tracks[tn].sectors[k].actual_size += header[offset_from_start + count * 8 + 07] * 256;
                  total_size += side_[i].tracks[tn].sectors[k].actual_size;
                  count++;
               }

               unsigned char* track_block = NULL;

               if (total_size > side_[i].tracks[j].track_size - size_to_remove_for_sector && total_size > 0xFF00)
               {
                  // "total_size" should be at last a multiple of 0x100
                  if ((total_size & 0xFF) != 0)
                  {
                     total_size += (0x100 - (total_size & 0xFF));
                  }
                  // Set the totalsize as a
                  side_[i].tracks[j].track_size = total_size;
                  track_block = new unsigned char[side_[i].tracks[j].track_size];
                  memcpy(track_block, &buffer[index_header], side_[i].tracks[j].track_size);
                  index_header += side_[i].tracks[j].track_size;
               }
               else
               {
                  track_block = new unsigned char[side_[i].tracks[j].track_size - size_to_remove_for_sector];

                  memcpy(track_block, &buffer[index_header], side_[i].tracks[j].track_size - size_to_remove_for_sector);
                  index_header += (side_[i].tracks[j].track_size - size_to_remove_for_sector);
               }

               // Data
               if (tn == 0x13)
               {
                  int dbg = 1;
               }
               for (k = 0; k < side_[i].tracks[tn].nb_sector; k++)
               {
                  if (side_[i].tracks[tn].sectors[k].actual_size == 0)
                  {
                     side_[i].tracks[tn].sectors[k].data = NULL;
                  }
                  else
                  {
                     side_[i].tracks[tn].sectors[k].data = new unsigned char[side_[i].tracks[tn].sectors[k].actual_size
                     ];
                     memcpy(side_[i].tracks[tn].sectors[k].data, &track_block[offset],
                            side_[i].tracks[tn].sectors[k].actual_size);
                     offset += side_[i].tracks[tn].sectors[k].actual_size;
                  }
               }
               delete[]track_block;
            }
            else
            {
               header = &buffer[index_header];
               index_header += side_[i].tracks[j].track_size;
               side_[i].tracks[j].formatted = false;
            }
         }
      }

      // Extension ? Offset-Info ?
      if (index_header > 0 && index_header + 0x0F < size)
      {
         unsigned char sector_index_buffer[2];
         //if (memcmp(header, "Offset-Info\r\n", 0x0E) == 0)
         if (memcmp(&buffer[index_header], "Offset-Info\r\n", 0x0E) == 0)
         {
            index_header += 0x0F;
            extended_offset_ = true;
            // OFFSET EXT.
            // For each track
            for (int side = 0; side < new_disk->nb_sides_; side++)
            {
               for (i = 0; i < m_NbTracks; i++)
               {
                  // Read track length
                  //fread(sector_index_buffer, 1, 2, file);
                  memcpy(sector_index_buffer, &buffer[index_header], 2);
                  index_header += 2;
                  // For each sector
                  for (k = 0; k < side_[side].tracks[i].nb_sector; k++)
                  {
                     // Read offset
                     //fread(sector_index_buffer, 1, 2, file);
                     memcpy(sector_index_buffer, &buffer[index_header], 2);
                     index_header += 2;

                     side_[side].tracks[i].sectors[k].sector_index = sector_index_buffer[0] + (sector_index_buffer[1] << 8);
                  }
               }
            }
         }
         else
         {
            //fseek(file, -0x0F, SEEK_CUR);
         }
      }

      for (int side = 0; side < new_disk->nb_sides_; side++)
      {
         for (unsigned int track = 0; track < new_disk->side_[side].nb_tracks; track++)
         {
            // Fill tracks
            unsigned int nb_revolutions = GetNbRevolutions(side, track);
            //unsigned int size = FillTrack (  side, track, NULL, 1 );
            new_disk->side_[side].tracks[track].nb_revolutions = nb_revolutions;
            new_disk->side_[side].tracks[track].revolution = new IDisk::Revolution[nb_revolutions];

            for (unsigned int rev = 0; rev < new_disk->side_[side].tracks[track].nb_revolutions; rev++)
            {
               FillTrackMfm(new_disk, side, track, rev);

               IDisk::Track trk;
               new_disk->GetTrackInfoForRev(side, track, &trk);
            }
         }
      }

      // Complete to 42 tracks
      for (int side = 0; side < new_disk->nb_sides_; side++)
      {
         for (unsigned int track = new_disk->side_[side].nb_tracks; track < 42; track++)
         {
            new_disk->side_[side].tracks[track].nb_revolutions = 1; // Only one for DSK files
            new_disk->side_[side].tracks[track].revolution = new IDisk::Revolution[1];

            new_disk->side_[side].tracks[track].revolution[0].size = DEFAULT_TRACK_SIZE * 16;
            new_disk->side_[side].tracks[track].revolution[0].bitfield = new unsigned char[DEFAULT_TRACK_SIZE * 16];
            memset(new_disk->side_[side].tracks[track].revolution[0].bitfield, 0, DEFAULT_TRACK_SIZE * 16);
         }
         if (new_disk->side_[side].nb_tracks < 42)
            new_disk->side_[side].nb_tracks = 42;
      }

      new_disk->CreateTrackFromMultiRevolutions();

      created_disk = new_disk;

      return OK;
   }
   else
   {
      return FILE_ERROR;
   }
}

int FormatTypeEDSK::SaveDisk(const char* file_path, IDisk* disk, ILoadingProgress* loading_progress) const
{
   FILE* file;
   int res = disk->SmartOpen(&file, file_path, ".DSK");

   if (res == 0)
   {
      // DISK INFORMATION BLOCK
      unsigned int nb_sectors = 0;
      unsigned char dib[0x100] = {0};
      memcpy(dib, "EXTENDED CPC DSK File\r\nDisk-Info\r\n", 34);
      memcpy(&dib[0x22], "SugarBox\r\n", 10);

      dib[0x30] = disk->side_[0].nb_tracks;
      dib[0x31] = disk->nb_sides_;
      //memcpy ( &dib[0x30], &disk_[0].NbTracks , 1 );
      //memcpy ( &dib[0x31], &m_NbSide , 1 );

      // Track size table
      // For each track :
      IDisk::Track* track = new IDisk::Track[disk->side_[0].nb_tracks * disk->nb_sides_];

      int offset = 0x34;
      for (unsigned int j = 0; j < disk->side_[0].nb_tracks; j++)
      {
         for (int i = 0; i < disk->nb_sides_; i++)
         {
            // Get size
            unsigned short size = disk->GetTrackInfo(i, j, &track[j * disk->nb_sides_ + i]);

            if (size != 0)
            {
               if (track[j * disk->nb_sides_ + i].full_size_ == 0)
               {
                  size = 0;
               }
               else if ((size & 0xFFFFFF00) < size)
               {
                  size += 0x100;
               }
            }
            dib[offset++] = (unsigned char)(size >> 8);
         }
      }
      fwrite(dib, 0x100, 1, file);

      // For each track :
      for (unsigned int j = 0; j < disk->side_[0].nb_tracks; j++)
      {
         for (int i = 0; i < disk->nb_sides_; i++)
         {
            IDisk::Track* info_track = &track[j * disk->nb_sides_ + i];

            if (info_track->list_sector_.size() > 0)
            {
               // Allocate proper size
               unsigned short size = (dib[0x34 + i + (j * disk->nb_sides_)] << 8);

               unsigned char* track_data = new unsigned char[size];
               memset(track_data, 0, size);
               // Generate track data
               memcpy(track_data, "Track-Info\r\n", 12);
               track_data[0x10] = j;
               track_data[0x11] = i;

               // Sz = First N...
               track_data[0x14] = info_track->list_sector_[0].n;
               track_data[0x15] = static_cast<unsigned char>(info_track->list_sector_.size());
               track_data[0x16] = info_track->gap3_size_;
               track_data[0x17] = 0xE5; // ?

               // Sector information list
               offset = 0x18;
               int offset_data = 0x100;
               //for (std::vector<tSector>::iterator it = info_track->listSector.begin(); it != info_track->listSector.end(); it++)
               for (unsigned int sector_index = 0; sector_index < info_track->list_sector_.size(); sector_index++)
               {
                  ++nb_sectors;
                  track_data[offset++] = info_track->list_sector_[sector_index].c; // it->C;
                  track_data[offset++] = info_track->list_sector_[sector_index].h; // it->H;
                  track_data[offset++] = info_track->list_sector_[sector_index].r; // it->R;
                  track_data[offset++] = info_track->list_sector_[sector_index].n; // it->N;
                  track_data[offset++] = info_track->list_sector_[sector_index].status1; // it->Status1;
                  track_data[offset++] = info_track->list_sector_[sector_index].status2; // ->Status2;
                  track_data[offset++] = info_track->list_sector_[sector_index].real_size & 0xFF;
                  // (it->RealSize & 0xFF);
                  track_data[offset++] = info_track->list_sector_[sector_index].real_size >> 8; // (it->RealSize >> 8);

                  // Data
                  int index = info_track->list_sector_[sector_index].dam_offset + 64;
                  // it->DAMOffset + 64; // Jump the A1A1A1F8/B 4 bytes of IDAM.

                  for (int cpt = 0; cpt < info_track->list_sector_[sector_index].real_size/*  it->RealSize*/; cpt++)
                  {
                     track_data[offset_data++] = disk->GetNextByte(i, j, index);
                     if (offset_data > size)
                     {
                        int dbg = 1;
                     }
                     index += 16;
                  }
               }
               // Write it to the disk
               fwrite(track_data, size, 1, file);

               // Free tracks
               delete[]track_data;
            }
         }
      }

      // Free ressources
      fclose(file);
      delete[]track;

      disk->disk_modified_ = false;
   }
   return 0;
}

// Get size and revolution number for this track
int FormatTypeEDSK::GetNbRevolutions(int side, int track)
{
   unsigned int nb_computed_revolution = 1;
   for (unsigned int s = 0; s < side_[side].tracks[track].nb_sector; s++)
   {
      // - n datas of sector (512 by default)
      unsigned int size_of_track = side_[side].tracks[track].sectors[s].actual_size;
      unsigned int theorical_size = (0x80 << (side_[side].tracks[track].sectors[s].sector_size));

      // Do we have multiple read ?
      if (theorical_size == 0) theorical_size = size_of_track;

      unsigned int nb_read = (size_of_track / theorical_size);

      if (size_of_track % theorical_size != 0)
         nb_read = 1;

      side_[side].tracks[track].sectors[s].nb_recorded_revolutions = nb_read;
      if (nb_read > nb_computed_revolution) nb_computed_revolution = nb_read;
   }
   return nb_computed_revolution;
}

int FormatTypeEDSK::FillTrackMfm(IDisk* new_disk, int side, int track, unsigned int indexRevolution)
{
   unsigned char track_byte[DEFAULT_TRACK_SIZE * 16];
   memset(track_byte, 0x00, sizeof(track_byte));
   unsigned int index_bit = 0;
   int search_base_index = 0;
   unsigned int max_search_index = 0;
   int j;
   bool add_end_of_track = true;

   if (!side_[side].tracks[track].formatted)
   {
      new_disk->side_[side].tracks[track].revolution[indexRevolution].size = 100000; // 100000 bits by default
      new_disk->side_[side].tracks[track].revolution[indexRevolution].bitfield = new unsigned char[new_disk->side_[side]
         .tracks[track].revolution[indexRevolution].size];
      memset(new_disk->side_[side].tracks[track].revolution[indexRevolution].bitfield, BIT_WEAK,
             new_disk->side_[side].tracks[track].revolution[indexRevolution].size);

      return new_disk->side_[side].tracks[track].revolution[indexRevolution].size;
   }

   // --- First of all : Seek in the existing sectors for a begining pattern
   int index_iam = 0x80;
   bool begining_found = false;
   unsigned int exact_size = 0;
   for (unsigned int s = 0; s < side_[side].tracks[track].nb_sector; s++)
   {
      //////////
      // MFM Conversion
      // CHRN
      ConvertChrntoMfm(&side_[side].tracks[track].sectors[s]);
      // DATA
      ConvertDatatoMfm(&side_[side].tracks[track].sectors[s], indexRevolution);

      // Look for the right pattern

      side_[side].tracks[track].sectors[s].whole_size_recorded = 0;


      if ((s == side_[side].tracks[track].nb_sector - 1 || (side_[side].tracks[track].sectors[s].mfm_data_size >= 100000
      )) && (begining_found == false))
         for (int j = 0; (begining_found == false) && j < side_[side].tracks[track].sectors[s].mfm_data_size; j++)
         {
            int off = IDisk::CompareForSync(&side_[side].tracks[track].sectors[s].mfm_data[j], MFMStartOfTrackPattern,
                                            sizeof(MFMStartOfTrackPattern));
            if (off != -1)
            {
               int indexIAMFound = j + off;

               // Begining of track exists !
               // Go backward, finding 0x4E until a rupture OR until something smart happens
               unsigned char pattern[16];
               IDisk::AddByteToTrack(pattern, 0, 0x4E);

               // Search for an hypotetic 4E : The 00 can be a bit more than 12 bytes !
               int search_index = j + off - 1;
               off = IDisk::CompareForSync(&side_[side].tracks[track].sectors[s].mfm_data[search_index], pattern,
                                           sizeof(pattern));
               while (search_index > 0 && off == -1)
               {
                  search_index--;
                  off = IDisk::CompareForSync(&side_[side].tracks[track].sectors[s].mfm_data[search_index], pattern,
                                              sizeof(pattern));
               }

               int EndOfGAP = search_index - off;
               search_index -= off;

               // Now, search for the rupture
               search_index -= 16;

               int off2 = IDisk::CompareForSync(&side_[side].tracks[track].sectors[s].mfm_data[search_index], pattern,
                                                sizeof(pattern));
               while (search_index >= 0 && off2 == 0)
               {
                  search_index -= 16;
                  off2 = IDisk::CompareForSync(&side_[side].tracks[track].sectors[s].mfm_data[search_index], pattern,
                                               sizeof(pattern));
               }

               if (search_index >= 0)
               {
                  // look for number of correct bits in the last byte
                  int correctbits = 0;
                  for (int count = 15; count >= 0; count--)
                  {
                     if (side_[side].tracks[track].sectors[s].mfm_data[search_index + count] == pattern[count])
                     {
                        correctbits++;
                     }
                  }
                  int start_of_track_index = search_index + (16 - correctbits);

                  if ((EndOfGAP - start_of_track_index) > (80 + 10) * 16)
                  {
                     // Compute the begining of the track "by hand", 80 "4E" before IAM
                     start_of_track_index = EndOfGAP - (80 * 16);
                  }

                  add_end_of_track = false;
                  begining_found = true;
                  // Set the index for the first IAM

                  // Now, try to find another MFMStartOfTrackPattern, at about 100000 bits from the first one.
                  for (int k = indexIAMFound + 90000; k < indexIAMFound + 110000 && k < side_[side].tracks[track].
                       sectors[s].mfm_data_size; k++)
                  {
                     int fnd = IDisk::CompareForSync(&side_[side].tracks[track].sectors[s].mfm_data[k],
                                                     MFMStartOfTrackPattern, sizeof(MFMStartOfTrackPattern));
                     if (fnd != -1)
                     {
                        // Ok ! Now we know the exact size of the track
                        exact_size = k + fnd - indexIAMFound;
                     }
                  }
                  if (side_[side].tracks[track].sectors[s].mfm_data_size - start_of_track_index > DEFAULT_TRACK_SIZE * 16)
                  {
                     memcpy(track_byte, &side_[side].tracks[track].sectors[s].mfm_data[start_of_track_index],
                            DEFAULT_TRACK_SIZE * 16);
                  }
                  else
                  {
                     memcpy(track_byte, &side_[side].tracks[track].sectors[s].mfm_data[start_of_track_index],
                            side_[side].tracks[track].sectors[s].mfm_data_size - start_of_track_index);
                  }

                  // Sync : 12 byte with value 0x00 + C2C2C2 FC
                  memcpy(&track_byte[indexIAMFound - start_of_track_index], MFMStartOfTrackPattern,
                         sizeof(MFMStartOfTrackPattern));

                  // Set search_base_index & max_search_index
                  search_base_index = 0;
                  max_search_index = side_[side].tracks[track].sectors[s].mfm_data_size - (start_of_track_index);

                  side_[side].tracks[track].sectors[s].mfm_data_size = (start_of_track_index /*-16*/);

                  side_[side].tracks[track].sectors[s].whole_size_recorded = start_of_track_index;

                  index_bit = max_search_index;
                  if (exact_size != 0)
                  {
                     max_search_index = index_bit = exact_size;
                  }
               }
            }
         }
   }

   // ---  Set the begining of the track
   // GAP 4a : 80 byte with value 0x4E
   if (!begining_found)
   {
      for (j = 0; j < index_iam; j++)
      {
         index_bit = IDisk::AddByteToTrack(track_byte, index_bit, 0x4E);
      }

      // Sync : 12 byte with value 0x00
      for (j = 0; j < 12; j++)
      {
         index_bit = IDisk::AddByteToTrack(track_byte, index_bit, 0x00);
      }

      // IAM : 3 byte with 0xC2, then one byte with 0xFC
      index_bit = IDisk::AddSyncByteToTrack(track_byte, index_bit, 0xC2);
      index_bit = IDisk::AddSyncByteToTrack(track_byte, index_bit, 0xC2);
      index_bit = IDisk::AddSyncByteToTrack(track_byte, index_bit, 0xC2);
      index_bit = IDisk::AddByteToTrack(track_byte, index_bit, 0xFC);


      // GAP 1 : 50 byte with 0x4E value
      for (j = 0; j < 50; j++)
      {
         index_bit = IDisk::AddByteToTrack(track_byte, index_bit, 0x4E);
      }

      // It's not necessary to search for sector before this place

      search_base_index = index_bit;
      max_search_index = index_bit;
   }


   unsigned int offset_sector = 0;
   unsigned int offset_first_sync_before_idam = 0;
   unsigned int last_end_of_sector = 0;


   if (track == 40)
   {
      int stop = 1;
   }

   // ---  Each sectors :
   for (unsigned int s = 0; s < side_[side].tracks[track].nb_sector; s++)
   {
      // Looking for interlaced sector : Are we finding something that match in the previous section ? (which means CHRN section, then sync immediately followed by data sections)
      // Search for interlaced offset
      unsigned int offset = LookforInterlaced(track_byte, search_base_index, max_search_index,
                                              &side_[side].tracks[track].sectors[s], &offset_first_sync_before_idam,
                                              s == 0);

      bool add_gap3 = false;
      // Yes : Check for CHRN and Data phase. If everything's fine, add the remaining (if any)
      if (offset != 0)
      {
         offset_sector = offset;

         side_[side].tracks[track].sectors[s].index_data = offset;
         if (exact_size != 0 && side_[side].tracks[track].sectors[s].mfm_data_size + offset > exact_size)
         {
            side_[side].tracks[track].sectors[s].mfm_data_size = exact_size - offset;
         }

         int sizeToCopy = CopyMem(track_byte, side_[side].tracks[track].sectors[s].mfm_data,
                                  side_[side].tracks[track].sectors[s].mfm_data_size, offset, DEFAULT_TRACK_SIZE * 16);

         if (side_[side].tracks[track].sectors[s].whole_size_recorded != 0 && exact_size == 0)
         {
            exact_size = side_[side].tracks[track].sectors[s].whole_size_recorded + offset;
         }

         if (index_bit < offset + sizeToCopy && (exact_size == 0))
         {
            index_bit = offset + sizeToCopy;
            if (max_search_index < index_bit)
               max_search_index = index_bit; // Remove CRC from search

            if (side_[side].tracks[track].sectors[s].whole_size_recorded == 0)
               add_gap3 = true;
         }

         // Into the search index ?
         bool add_crc = true; // (begining_found == false);
         if (max_search_index >= index_bit + 32)
         {
            if (IDisk::CompareForSync(side_[side].tracks[track].sectors[s].mfm_crc, &track_byte[offset + sizeToCopy],
                                      32) != 0)
            {
               // Dont add crc !
               add_crc = false;
            }
         }

         int size_to_copy_crc = 0;
         if (add_crc)
            size_to_copy_crc = CopyMem(track_byte, side_[side].tracks[track].sectors[s].mfm_crc, 32, offset + sizeToCopy,
                                    DEFAULT_TRACK_SIZE * 16);

         if (index_bit < offset + sizeToCopy + size_to_copy_crc /*&& (!begining_found)*/)
         {
            index_bit = offset + sizeToCopy + size_to_copy_crc;
            if (side_[side].tracks[track].sectors[s].whole_size_recorded == 0)
               add_gap3 = true;

            add_gap3 = true;
         }

         // search_base_index only increased by the CHRN sync datas
         search_base_index = offset + MFMCHRNSIZE;
         last_end_of_sector = offset + sizeToCopy + size_to_copy_crc;
      }
      else
      {
         bool filler_bytes = true;
         // Is there an index ?
         if (extended_offset_ && s > 0 && (side_[side].tracks[track].sectors[s].sector_index - side_[side].tracks[track].
            sectors[s - 1].sector_index > 0))
         {
            // Yes : Set it
            // Size between the previous and current data segments
            int sectorsize = (side_[side].tracks[track].sectors[s].sector_index - side_[side].tracks[track].sectors[s -
               1].sector_index) * 16; // Add SYNC datas
            int sizeGap = index_bit - offset_sector;
            //   Size between begining of previous Data segment (first 0x00 sync byte) and real position

            int offset = sectorsize - (sizeGap + 44 * 16);

            // Offset < 2 are ignored : it's just placement
#define IGNORE_LEVEL 48
            if (offset >= -IGNORE_LEVEL && offset <= IGNORE_LEVEL)
               offset = 0;

            if (offset > 0)
            {
               // Add some informations :
               // IF actual size < CHRN size : Maybe we have a CRC, and some other things
               // TODO
               // Otherwise, ...

               // Add GAP3 infos
               for (int gapcounter = 0; gapcounter < (offset / 16); gapcounter++)
               {
                  if (index_bit + 16 < DEFAULT_TRACK_SIZE * 16)
                     index_bit = IDisk::AddByteToTrack(track_byte, index_bit, side_[side].tracks[track].gap3_filler);
               }
            }
            else if (offset < 0)
            {
               // GAP3 removal :
               // If it is possible : GAP3 are enough
               int nb_gap = 0;
               if (s > 0)
               {
                  nb_gap = side_[side].tracks[track].gap3 * 16;

                  offset = -offset;
                  if (offset <= /*nb_gap*/(static_cast<int>(index_bit) - static_cast<int>(max_search_index)))
                  {
                     index_bit -= offset;
                  }
                  else
                  {
                     // Theorical size :
                     int theoricalsize = ((0x80 << side_[side].tracks[track].sectors[s - 1].sector_size) + 12 + 4 + 2) *
                        16;

                     if (theoricalsize < side_[side].tracks[track].sectors[s - 1].mfm_data_size /*+ 32 */)
                        // add crc to mfmsize !
                     {
                        // actual size should have a crc and some gap, or more...
                        index_bit -= offset;
                     }
                     else
                     {
                        // Go back BEFORE theorical end of track...
                        // Check with max_search_index (actual value registered)
                        if (max_search_index < index_bit - offset)
                        {
                           index_bit -= offset;
                        }
                        else
                        {
                           // The 'offset' bits should be the begining of the track
                           if (memcmp(side_[side].tracks[track].sectors[s].mfm_chrn,
                                      (unsigned char*)&track_byte[index_bit - offset], offset) == 0)
                           {
                              index_bit -= offset;
                           }
                           else
                           {
                              index_bit = max_search_index + 32;
                           }
                        }
                     }

                     // DO NOTHING : TO TEST !
                     // All these case should be handled by LookforInterlaced
                     // So... Here is just a case where offset-info is wrong !

                     /*
                     // We're somewhere after the GAP. Like....
                     int offsetAfterGap = offset - nb_gap;
                     index_bit -= offset; //offsetAfterGap;

                     // Nothing fit, as it would been returned by LookforInterlaced
                     int dbg = 1;
                     */
                  }
               }
            }
         }
         //else
         {
            // No : Add it to the end -
            // Checkj if size > expected size : if yes, dont add CRC / GAP
            if (side_[side].tracks[track].sectors[s].actual_size != (0x80 << (side_[side].tracks[track].sectors[s].
               sector_size)))
            {
               // Check that size is greater. If it's less,
               if (side_[side].tracks[track].sectors[s].actual_size > (0x80 << (side_[side].tracks[track].sectors[s].
                  sector_size)))
               {
                  filler_bytes = false;
               }
               else
               {
                  // if actual size 'like' a proper one, add checksum
                  if (side_[side].tracks[track].sectors[s].actual_size % 0x80 != 0)
                  {
                     filler_bytes = false;
                  }
                  else
                  {
                     filler_bytes = true;
                  }
               }
            }
         }

         // Set the search_base_index to the current index
         // CHRN Data
         /*int size_to_copy = (index_bit+MFMCHRNSIZE>DEFAULT_TRACK_SIZE*16)?DEFAULT_TRACK_SIZE*16-index_bit:MFMCHRNSIZE;
         memcpy ( &track_byte[index_bit], m_Side [side].Tracks[track].Sectors[s].MfmCHRN, size_to_copy );
         */
         int size_to_copy = CopyMem(track_byte, side_[side].tracks[track].sectors[s].mfm_chrn, MFMCHRNSIZE, index_bit,
                                  DEFAULT_TRACK_SIZE * 16);
         side_[side].tracks[track].sectors[s].index_header = index_bit;

         index_bit += size_to_copy;
         search_base_index = index_bit;

         // 0x4E as GAP (22 here )
         for (j = 0; j < 22; j++)
         {
            if (index_bit + 16 < DEFAULT_TRACK_SIZE * 16)
               index_bit = IDisk::AddByteToTrack(track_byte, index_bit, 0x4E);
         }
         offset_first_sync_before_idam = index_bit;
         // Track Data
         /*size_to_copy = (index_bit+m_Side [side].Tracks[track].Sectors[s].MfmDataSize>DEFAULT_TRACK_SIZE*16)?DEFAULT_TRACK_SIZE*16-index_bit:m_Side [side].Tracks[track].Sectors[s].MfmDataSize;
         memcpy ( &track_byte[index_bit], m_Side [side].Tracks[track].Sectors[s].MfmData, size_to_copy );
         */
         side_[side].tracks[track].sectors[s].index_data = index_bit;
         size_to_copy = CopyMem(track_byte, side_[side].tracks[track].sectors[s].mfm_data,
                              side_[side].tracks[track].sectors[s].mfm_data_size, index_bit, DEFAULT_TRACK_SIZE * 16);

         offset_sector = index_bit;
         index_bit += size_to_copy;

         max_search_index = index_bit;

         // Add CRC
         // TOCHECK : Check if this filler_bytes is a good idea or not.
         if (filler_bytes)
         {
            size_to_copy = CopyMem(track_byte, side_[side].tracks[track].sectors[s].mfm_crc, 32, index_bit,
                                 DEFAULT_TRACK_SIZE * 16);
            index_bit += size_to_copy;
            //max_search_index += size_to_copy;

            add_gap3 = true;
         }
         last_end_of_sector = index_bit;
      }

      // GAP 3 ? Add GAP3 without changing max_search_index....
      if (add_gap3)
      {
         // - GAP #3 (x bytes)
         for (j = 0; j < side_[side].tracks[track].gap3 && index_bit + 16 < DEFAULT_TRACK_SIZE * 16; j++)
         {
            index_bit = IDisk::AddByteToTrack(track_byte, index_bit, 0x4E);
         }
      }
   }
   ////////////////////////////////////////
   // Also, check if it's an overlapping track. If yes (by looking for A1A1A1FC sync Pattern), adjust total size to keep shifting as describe here.
   int index_last_gap = 0;
   // Check of IAM (we can remove it if necessary)

   if (begining_found)
   {
      if (exact_size == 0)
         index_bit = last_end_of_sector;
      else
         index_bit = exact_size;
   }
   else if (add_end_of_track)
   {
      // Exception : 10 sector track (or BIG size sectors, or....) can lead to an overrun of the IAM by the end GAP.
      // Ex : Fighter bomber.
      // Rules : Compute all sector sizes (data+crc+gap+headers).
      // That shall give us the remaining GAP, divided at the end / begining of the track.
      // If less than 150 bytes are left, just don't add the IAM, we assume it's overwrittent at any time
      // Total gap 
      // Remove some data from the IAM sector - Envenly share GAP between end and begin of the track
      unsigned int final_gap = 0;
      index_last_gap = index_bit;
      if (index_bit < 6250 * 16)
      {
         final_gap = (6250 * 16 - index_last_gap);
      }

      unsigned int final_index_begin = index_iam + 16 + 50;
      unsigned int total_gap = final_index_begin * 16 + final_gap;

      if (final_gap < 150 * 16)
      {
         // Adjust : We need to have the final gap at least 100 bytes 
         unsigned int adjust = 150 * 16 - final_gap;

         unsigned int move_begin = (index_iam + 16 + 50) * 16;
         unsigned int move_end = (index_iam + 16 + 50) * 16 - adjust;
         // Move the whole track 
         memmove( &track_byte[move_end], &track_byte[move_begin], adjust);
         index_bit -= adjust;
      }

      // Add missing gap
      while (index_bit + 16 < 6250 * 16)
      {
         index_bit = IDisk::AddByteToTrack(track_byte, index_bit, 0x4E);
      }
   }


   // 
   // Copy these data to bitfield
   new_disk->side_[side].tracks[track].revolution[indexRevolution].size = index_bit;
   new_disk->side_[side].tracks[track].revolution[indexRevolution].bitfield = new unsigned char[index_bit];

   memcpy(new_disk->side_[side].tracks[track].revolution[indexRevolution].bitfield, track_byte, index_bit);

   // Clear allocated datas
   for (unsigned int s = 0; s < side_[side].tracks[track].nb_sector; s++)
   {
      if (side_[side].tracks[track].sectors[s].mfm_data != NULL)
      {
         delete[]side_[side].tracks[track].sectors[s].mfm_data;
         side_[side].tracks[track].sectors[s].mfm_data = NULL;
      }
   }

   if (exact_size != 0)
   {
      return exact_size;
   }

   return index_bit;
}

void FormatTypeEDSK::ConvertChrntoMfm(Sector* sector)
{
   // Sync 0x00
   int j;
   int index_bit = 0;
   CRC crc;
   for (j = 0; j < 12; j++)
   {
      index_bit = IDisk::AddByteToTrack(sector->mfm_chrn, index_bit, 0x00);
   }

   crc.Reset();

   // A1A1A1
   index_bit = IDisk::AddSyncByteToTrack(sector->mfm_chrn, index_bit, 0xA1);
   crc.AddByteToCrc(0xA1);
   index_bit = IDisk::AddSyncByteToTrack(sector->mfm_chrn, index_bit, 0xA1);
   crc.AddByteToCrc(0xA1);
   index_bit = IDisk::AddSyncByteToTrack(sector->mfm_chrn, index_bit, 0xA1);
   crc.AddByteToCrc(0xA1);
   // FE
   index_bit = IDisk::AddByteToTrack(sector->mfm_chrn, index_bit, 0xFE);
   crc.AddByteToCrc(0xFE);

   // CHRN
   index_bit = IDisk::AddByteToTrack(sector->mfm_chrn, index_bit, sector->track);
   crc.AddByteToCrc(sector->track);
   index_bit = IDisk::AddByteToTrack(sector->mfm_chrn, index_bit, sector->side);
   crc.AddByteToCrc(sector->side);
   index_bit = IDisk::AddByteToTrack(sector->mfm_chrn, index_bit, sector->sector_id);
   crc.AddByteToCrc(sector->sector_id);
   index_bit = IDisk::AddByteToTrack(sector->mfm_chrn, index_bit, sector->sector_size);
   crc.AddByteToCrc(sector->sector_size);

   unsigned short computed_crc = crc.GetCRC();

   if ((sector->fdc_status_1 & 0x20) == 0x20
      && (sector->fdc_status_2 & 0x20) == 0
   )
   {
      // Error in CRC..
      computed_crc += 1;
   }

   index_bit = IDisk::AddByteToTrack(sector->mfm_chrn, index_bit, (computed_crc >> 8));
   index_bit = IDisk::AddByteToTrack(sector->mfm_chrn, index_bit, (computed_crc & 0xFF));
}

void FormatTypeEDSK::ConvertDatatoMfm(Sector* sector, unsigned int index_revolution)
{
   // Check size
   // - n datas of sector (512 by default)
   unsigned int size_of_track = sector->actual_size;
   unsigned int theorical_size = (0x80 << (sector->sector_size));
   // Do we have multiple read ?
   if (theorical_size == 0) theorical_size = size_of_track;
   unsigned int nbRead = (size_of_track / theorical_size);
   bool bMultipleRead = ((nbRead > 1) && (size_of_track % theorical_size == 0));
   // Recompute real size to read in case of multipleread
   if (bMultipleRead)
   {
      size_of_track = theorical_size;
   }
   else
   {
      index_revolution = 0;
   }

   int size = (12 + 4 + 2 + size_of_track) * 16; // Sync + IDAM + track size + CRC

   sector->mfm_data = new unsigned char[size];
   unsigned char* track_byte = sector->mfm_data;

   // Sync 0x00
   int j;
   int index_bit = 0;
   CRC crc;
   for (j = 0; j < 12; j++)
   {
      index_bit = IDisk::AddByteToTrack(track_byte, index_bit, 0x00);
   }

   crc.Reset();

   // A1A1A1
   index_bit = IDisk::AddSyncByteToTrack(track_byte, index_bit, 0xA1);
   crc.AddByteToCrc(0xA1);
   index_bit = IDisk::AddSyncByteToTrack(track_byte, index_bit, 0xA1);
   crc.AddByteToCrc(0xA1);
   index_bit = IDisk::AddSyncByteToTrack(track_byte, index_bit, 0xA1);
   crc.AddByteToCrc(0xA1);

   // FE
   unsigned char idam_byte;
   if (sector->fdc_status_2 & 0x01)
   {
      // Missing adress mark in data field
      idam_byte = 0xFF;
   }
   else
   {
      if ((sector->fdc_status_2 & 0x40) == 0x40)
         idam_byte = DAAM_ERASED;
      else
         idam_byte = DAAM_OFF;
   }

   index_bit = IDisk::AddByteToTrack(track_byte, index_bit, idam_byte);
   crc.AddByteToCrc(idam_byte);

   // Datas
   // TODO : Create all revolutions registered.
   // Then, compare them to find identical pattern and weak area

   if (nbRead <= index_revolution)
   {
      index_revolution = 0;
   }

   unsigned int k;
   for (k = 0; k < size_of_track; k++)
   {
      unsigned char b = sector->data[k + index_revolution * theorical_size];
      index_bit = IDisk::AddByteToTrack(track_byte, index_bit, b);
      crc.AddByteToCrc(b);
   }

   sector->mfm_data_size = index_bit;

   bool crc_needed = (size_of_track == theorical_size);
   // If less
   if (size_of_track < theorical_size)
   {
      // How can we know ?
      // Compare size of gap with index values.
      // if there is a diff of "2", add a crc to make it correct
      // TODO this
      int dbg = 1;
      crc_needed = true;
   }
   if ((sector->fdc_status_1 & 0x20) == 0x20
      && (sector->fdc_status_2 & 0x20) == 0x20
      && (size_of_track == theorical_size)
   )
      crc_needed = false;


   sector->crc_present = true;

   //if ( crc_needed )
   {
      unsigned short computed_crc = crc.GetCRC();

      if ((sector->fdc_status_1 & 0x20) == 0x20
         && (sector->fdc_status_2 & 0x20) == 0x20
         && (size_of_track == theorical_size)
            //&& (size_of_track == (0x80<<(sector->sector_size)))
      )
      {
         // Error in CRC..
         computed_crc += 1;
      }

      int crc_bit = 0;
      crc_bit = IDisk::AddByteToTrack(sector->mfm_crc, crc_bit, (computed_crc >> 8));
      crc_bit = IDisk::AddByteToTrack(sector->mfm_crc, crc_bit, (computed_crc & 0xFF));
   }
}

unsigned int FormatTypeEDSK::LookforInterlaced(unsigned char* track_byte, int searchBaseIndex, int index_bit,
                                               Sector* sector, unsigned int* returned_offset_first_sync_before_idam,
                                               bool first_sector)
{
   // Look for the first data part
   for (int i = searchBaseIndex; i < index_bit - MFMCHRNSIZE; i++)
   {
      int off = IDisk::CompareForSync(&track_byte[i], sector->mfm_chrn, MFMCHRNSIZE);

      if (off != -1)
      {
         // Found ? Then look for the second one, begining with 0x00 (x12) + A1A1A1 as sync bytes
         int offsetCHRN = i + off;
         for (int j = i + MFMCHRNSIZE; j < index_bit - 16; j++)
         {
            //if ( memcmp ( &track_byte[j], mfm_data_pattern, sizeof(mfm_data_pattern)) == 0 )
            int size_of_data_pattern = (sizeof(MFMDataPattern) < (sector->mfm_data_size))
                                       ? sizeof(MFMDataPattern)
                                       : sector->mfm_data_size;
            size_of_data_pattern = (size_of_data_pattern < (index_bit - j)) ? size_of_data_pattern : (index_bit - j);

            off = IDisk::CompareForSync(&track_byte[j], MFMDataPattern, size_of_data_pattern);
            if (off != -1)
            {
               //j += off;
               // Now compare withe begining of data sections
               if (index_bit - (j) >= sizeof(MFMDataPattern))
               {
                  // Compare between last sync bit and what we've got
                  // So : begining at size_of_data_pattern, until index_bit / end of Mfmdata

                  int compare_size = (sector->mfm_data_size - size_of_data_pattern) < (index_bit - (j + size_of_data_pattern))
                                       ? (sector->mfm_data_size - size_of_data_pattern)
                                       : (index_bit - (j + size_of_data_pattern));

                  if (compare_size > 256) compare_size = 256;

                  // Ok ? We got our offset !
                  int off2 = IDisk::CompareForSync(&track_byte[j + size_of_data_pattern],
                                                   &sector->mfm_data[size_of_data_pattern], compare_size);
                  if (off2 != -1)
                     //if ( memcmp ( &track_byte[j+sizeof(mfm_data_pattern)], &sector->MfmData[sizeof(mfm_data_pattern)], compare_size) == 0 )
                  {
                     // Before returning offset, patch the CHRN data for sync bytes
                     sector->index_header = offsetCHRN;
                     memcpy(&track_byte[offsetCHRN], sector->mfm_chrn, MFMCHRNSIZE);

                     // Also, return the offset_first_sync_before_idam

                     return j + off2;
                  }
               }
               else
               {
                  // No data check, so be sure we are alligned with data....
                  if (off == 0)
                  {
                     memcpy(&track_byte[offsetCHRN], sector->mfm_chrn, MFMCHRNSIZE);
                     sector->index_header = offsetCHRN;
                     return j + off;
                  }
               }
            }
         }
      }
   }
   // Not found : Last try - Let's check if we have a SYNC zone at the very end of the data
   // This would let us play the ALLGAP dsk;
   // Sync zone is at least 11 byte of '00' (because of shifting, the 12th can be incomplete)
#define TO_IMPROVE
#ifdef TO_IMPROVE

   // Remove : SYNC at the end can be used in lots of cases
   if (first_sector || (sector->actual_size > (0x80 << (sector->sector_size))))
   {
      if (searchBaseIndex <= index_bit - (12 * 16) && (16 * 22) + index_bit - (12 * 16) + MFMCHRNSIZE + sizeof(
         MFMDataPattern) < DEFAULT_TRACK_SIZE * 16)
      {
         int count = 1;
         unsigned char curr_b = track_byte[index_bit - count++];
         bool correct = true;
         while (count <= 12 * 16 && correct)
         {
            if (track_byte[index_bit - count] != curr_b)
               curr_b = track_byte[index_bit - count++];
            else
            {
               correct = false;
               count--;
            }
         }

         //int off = IDisk::CompareForSync(&track_byte[index_bit - (12 * 16)-1], sector->MfmCHRN, (12 * 16));
         //if (off != -1)
         if (count >= 11 * 16)
         {
            if (count > 12 * 16) count = 12 * 16;

            // adjust have first data bit = 0
            if (track_byte[index_bit - count + 1] != 0) count--;

            // It seems to be ok here
            unsigned char pattern[16];
            IDisk::AddByteToTrack(pattern, 0, 0x4E);

            // Now, just check that before this, we have something like "4e"
            /*int off2 = IDisk::CompareForSync(&track_byte[index_bit - count - 16], pattern, sizeof(pattern));
            if (off2 != -1)
            {*/
            // Ok, then just fill everything and let's go !
            int index = index_bit - count;
            memcpy(&track_byte[index], sector->mfm_chrn, MFMCHRNSIZE);
            index += MFMCHRNSIZE;

            // GAP
            for (int i = 0; i < 22; i++)
            {
               memcpy(&track_byte[index], pattern, 16);
               index += 16;
            }

            // Pattern
            /*
            memcpy(&track_byte[index], mfm_data_pattern, sizeof(mfm_data_pattern));
            index += sizeof(mfm_data_pattern);*/
            return index;
            //}
         }
      }
   }
#endif // TO_IMPROVE

   return 0;
}

int FormatTypeEDSK::LoadDisk(std::vector<TrackItem> file_list, IDisk*& created_disk,
                             ILoadingProgress* loading_progress)
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
