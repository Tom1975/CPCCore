#include "stdafx.h"
#include "IDisk.h"
#include "FDC765.h"

#include "simple_filesystem.h"
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#ifdef __MORPHOS__
#include <proto/dos.h>
#endif

#include "macro.h"
#if _MSC_VER < 1900
#ifndef __GNUC__
int lround(double d)
{
  return (int)std::floor(d + 0.5);
}
#endif
#endif

static unsigned char VendorTrack[9] = {0x41, 0x46, 0x42, 0x47, 0x43, 0x48, 0x44, 0x49, 0x45};
static unsigned char DataTrack[9] = {0xC1, 0xC6, 0xC2, 0xC7, 0xC3, 0xC8, 0xC4, 0xC9, 0xC5};

// Data pattern : 0 0 C2 C2 C2 FC -> 00 + 00 + 0x5224 0x5224 0x5224 + FC
// Note : First bit of first 00 should not be used (it should be 10 )
static unsigned char MFMStartOfTrackPattern [16 * 6];

// Data pattern 2 : 0 0 A1 A1 A1 FE -> 00 + 00 + 0x4489 0x4489 0x4489 + FE
static unsigned char MFMStartOfTrackPattern2 [16 * 6];


size_t InsertShort(FILE* f, unsigned short data)
{
   unsigned char buf[2];
   buf[1] = (data & 0xFF);
   buf[0] = ((data >> 8) & 0xFF);

   const size_t size = fwrite(buf, 1, 2, f);
   return size;
}

size_t InsertInt(FILE* f, unsigned int data)
{
   unsigned char buf[4];
   buf[3] = (data & 0xFF);
   buf[2] = ((data >> 8) & 0xFF);
   buf[1] = ((data >> 16) & 0xFF);
   buf[0] = ((data >> 24) & 0xFF);
   return fwrite(buf, 1, 2, f);
}


IDisk::IDisk(): nb_sides_(1)
{
   load_progress_ = -1; // No progress by default
   log_ = nullptr;
   fixed_speed_ = false;

   head_position_ = 0;
   disk_modified_ = false;

   current_disk_path_ = "";

   side_[0].nb_tracks = 0;
   side_[0].tracks = nullptr;
   side_[1].nb_tracks = 0;
   side_[1].tracks = nullptr;

   int index_bit = 0;
   index_bit = AddByteToTrack(MFMStartOfTrackPattern, index_bit, 0x00);
   index_bit = AddByteToTrack(MFMStartOfTrackPattern, index_bit, 0x00);
   index_bit = AddSyncByteToTrack(MFMStartOfTrackPattern, index_bit, 0xC2);
   index_bit = AddSyncByteToTrack(MFMStartOfTrackPattern, index_bit, 0xC2);
   index_bit = AddSyncByteToTrack(MFMStartOfTrackPattern, index_bit, 0xC2);
   index_bit = AddByteToTrack(MFMStartOfTrackPattern, index_bit, 0xFC);

   index_bit = 0;
   index_bit = AddByteToTrack(MFMStartOfTrackPattern2, index_bit, 0x00);
   index_bit = AddByteToTrack(MFMStartOfTrackPattern2, index_bit, 0x00);
   index_bit = AddSyncByteToTrack(MFMStartOfTrackPattern2, index_bit, 0xA1);
   index_bit = AddSyncByteToTrack(MFMStartOfTrackPattern2, index_bit, 0xA1);
   index_bit = AddSyncByteToTrack(MFMStartOfTrackPattern2, index_bit, 0xA1);
   index_bit = AddByteToTrack(MFMStartOfTrackPattern2, index_bit, 0xFE);
}

IDisk::IDisk(IDisk::DiskType diskType) : IDisk()
{
   // BLANK DISK :
   // One side
   nb_sides_ = 1;

   // 41 tracks
   side_[0].nb_tracks = 42;
   side_[0].tracks = new MFMTrack[side_[0].nb_tracks];
   for (unsigned int track = 0; track < side_[0].nb_tracks; track++)
   {
      // One revolution
      side_[0].tracks[track].nb_revolutions = 0;
      side_[0].tracks[track].revolution = nullptr; //new tRevolution[1];

      side_[0].tracks[track].size = 6250 * 16;
      side_[0].tracks[track].bitfield = new unsigned char[side_[0].tracks[track].size];

      // Fill each track
      FillTrack(side_[0].tracks[track].bitfield, track, diskType, side_[0].tracks[track].size);
   }
}

IDisk::~IDisk()
{
   // todo ?
}


IDisk::FaceSelection IDisk::FilterSide(IDisk::FaceSelection faceSelection)
{
   FaceSelection ret = NO_FACE;

   switch (faceSelection)
   {
   case FACE_1:
      // Clean second side if necessary
      if (nb_sides_ > 1)
      {
         // delete fac e2
         CleanSide(1);
         nb_sides_ = 1;
      }
      ret = FACE_1;
      break;
   case FACE_2:
      // Copy face 2 to 1, then clean second face (if possible)
      CleanSide(0);
      side_[0] = side_[1];
      side_[1].nb_tracks = 0;
      side_[1].tracks = nullptr;
      nb_sides_ = 1;
      ret = FACE_2;
      break;
   case FACE_BOTH:
      // Check if a second face exists
      if (nb_sides_ == 1) ret = FACE_1;
      else ret = FACE_BOTH;
      break;
   case NO_FACE:
      break;
   default:
      break;
   }

   return ret;
}

void IDisk::SetWrite(int side, unsigned int track)
{
   // Set only one revolution here... And 6500 byte for this track.
   if (track >= side_[side].nb_tracks)
   {
      // Add a track !
      MFMTrack* tmp = side_[side].tracks;

      // Copy existing tracks
      side_[side].tracks = new MFMTrack[track + 1];
      memset(side_[side].tracks, 0, sizeof(MFMTrack) * track + 1);
      for (unsigned int i = 0; i < side_[side].nb_tracks; i++)
      {
         memcpy(&side_[side].tracks[i], &tmp[i], sizeof(MFMTrack));
      }

      // Delete old
      delete[]tmp;

      // Replace
      side_[side].nb_tracks = track + 1;

      disk_modified_ = true;
      side_[side].tracks[track].bitfield = new unsigned char [DEFAULT_TRACK_SIZE * 16];
      side_[side].tracks[track].size = DEFAULT_TRACK_SIZE * 16;
      memset(side_[side].tracks[track].bitfield, 0, DEFAULT_TRACK_SIZE * 16);

      head_position_ = 0;
   }
}


int IDisk::AddByteWithoutCrc(unsigned char* track, int index, unsigned char b, bool bSync)
{ 
   return bSync ? AddSyncByteToTrack(track, index, b) : AddByteToTrack(track, index, b);
}


unsigned char IDisk::ConvertToByte(const unsigned char pattern[8])
{
   unsigned char b = 0;
   int dec = 7;
   for (int i = 0; i < 8; i ++)
   {
      b |= ((pattern[i] & 1) << dec);
      dec--;
   }
   return b;
}

bool IDisk::MFMCorrect(const unsigned char pattern[16], unsigned char previousBit)
{
   for (int i = 1; i < 16; i += 2)
   {
      if (pattern[i - 1] == 1 && pattern[i] == 1) return false;
      if (i >= 2)
      {
         if (
            (pattern[i - 1] == 1 && pattern[i - 2] == 1)
            || (pattern[i - 1] == 0 && pattern[i - 2] == 0 && pattern[i] == 0)
         )
         {
            return false;
         }
      }
      else
      {
         if (
            (pattern[i - 1] == 1 && previousBit == 1)
            || (pattern[i - 1] == 0 && previousBit == 0 && pattern[i] == 0)
         )
         {
            return false;
         }
      }
   }
   return true;
}

bool IDisk::NoWeakBits(unsigned char* buffer, int offset, int size)
{
   for (int i = offset; i < offset + size; i++)
   {
      if ((buffer[i] & (BIT_WEAK | BIT_OPTIONAL)) != 0)
         return false;
   }
   return true;
}

unsigned char IDisk::ConvertMFMToByte(const unsigned char pattern[16])
{
   unsigned char b = 0;
   auto dec = 7;
   for (auto i = 1; i < 16; i += 2)
   {
      if (i > 2 &&
         (
            (pattern[i - 1] == 1 && pattern[i - 2] == 1)
            || (pattern[i - 1] == 1 && pattern[i] == 1)
            || (pattern[i - 1] == 0 && pattern[i - 2] == 0 && pattern[i] == 0)
         )
      )
      {
      }
      b |= ((pattern[i] & 1) << dec);
      dec--;
   }
   return b;
}

int IDisk::AddByteWithCrc(unsigned char* track, int index, unsigned char b, CRC& crc, bool bSync)
{
   const int new_index = AddByteWithoutCrc(track, index, b, bSync);
   crc.AddByteToCrc(b);
   return new_index;
}

void IDisk::ChangeTrack(int side, unsigned int newTrack)
{
   // Adjust head
   if ( newTrack >= side_[side].nb_tracks)
   {
      head_position_ = 0;
      
   }
   else if (head_position_ >= side_[side].tracks[newTrack].size)
   {
      if (side_[side].tracks[newTrack].size == 0)
      {
         head_position_ = 0;
      }
      else
      {
         head_position_ %= side_[side].tracks[newTrack].size;
      }
   }
}

void IDisk::CleanSide(int side)
{
	if (side < nb_sides_)
	{
		for (unsigned int track = 0; track < side_[side].nb_tracks; track++)
		{
			delete[]side_[side].tracks[track].bitfield;
		}
		delete[]side_[side].tracks;
	}
}

void IDisk::CleanDisk()
{
   //
   for (int side = 0; side < nb_sides_; side ++)
   {
      CleanSide(side);
   }
   disk_modified_ = false;

   side_[0].nb_tracks = 0;
   side_[0].tracks = nullptr;
   side_[1].nb_tracks = 0;
   side_[1].tracks = nullptr;
   nb_sides_ = 0;
}


void IDisk::WriteBit(int side, int track, unsigned char bit)
{
   if (head_position_ == 0)
   {
      bit |= BIT_INDEX;
   }

   if (head_position_ < side_[side].tracks[track].size)
      side_[side].tracks[track].bitfield[head_position_] = bit;

   disk_modified_ = true;

   // Weak bit : TO DO

   // Get next bit
   ++head_position_;
   if (head_position_ >= side_[side].tracks[track].size)
   {
      head_position_ = 0;
   }
}

unsigned char IDisk::GetNextBit(int side, int track)
{
   // Sanity check
   if (side_[side].tracks[track].bitfield == nullptr)
   {
      return (rand() & 0x1);
   }

   unsigned char value = side_[side].tracks[track].bitfield[head_position_];

   if (value & 0xF2)
   {
      // SOMETHING STRANGE ! - Lots of error if this is not removed !!!
      LOG(head_position_);
      LOG(value);
      value &= 0x0D;
      LOG(value);
   }
   if (head_position_ < 2)
      value |= BIT_INDEX;

   // Return current bit (0 = index)
   if ((value & BIT_OPTIONAL) == BIT_OPTIONAL)
   {
      bool one_more;
      
      {
         one_more = rand() & 0x1;
      }
      
      if (one_more)
      {
         ++head_position_;
         if (head_position_ >= side_[side].tracks[track].size)
         {
            head_position_ = 0;
         }
         value = side_[side].tracks[track].bitfield[head_position_] | (value & BIT_INDEX);
      }
   }

   if ((value & BIT_WEAK) == BIT_WEAK)
   {
      value = (rand() & 0x1) | (value & BIT_INDEX);
   }

   // Get next bit
   ++head_position_;
   if (head_position_ >= side_[side].tracks[track].size)
   {
      head_position_ = 0;
   }

   return (value & 0x3);
}


int IDisk::AddSyncByteToTrack(unsigned char* track, unsigned int index, unsigned char byte)
{
   // 0x4489; -> A1
   // 0x5224; -> C2
   if (track == nullptr)
      return index + 16;


   unsigned short seq_sync = 0;
   if (byte == 0xA1)
      seq_sync = 0x4489;
   if (byte == 0xC2)
      seq_sync = 0x5224;
   else; // ERROR !

   for (int i = 0; i < 16; i++)
   {
      track[index++] = ((seq_sync >> (15 - i)) & 0x01);
   }
   return index;
}

int IDisk::AddMFMByteToAllTrack(int side, int track, unsigned int index, unsigned char byte, int datasize)
{
   for (int d = 0; d < datasize; d++)
   {
      if (index >= side_[side].tracks[track].size)
      {
         index = 0;
      }
      side_[side].tracks[track].bitfield[index++] = ((byte >> (datasize - 1 - d)) & 0x01);
   }
   return index;
}

int IDisk::AddMFMByteToSpecificTrack(int side, int track, int rev, unsigned int index, unsigned char byte, int datasize)
{
   for (int d = 0; d < datasize; d++)
   {
      if (index >= side_[side].tracks[track].revolution[rev].size)
      {
         index = 0;
      }
      side_[side].tracks[track].revolution[rev].bitfield[index++] = ((byte >> (datasize - 1 - d)) & 0x01);
   }
   return index;
}


int AddByteToTrackInBits(unsigned char* track, unsigned int index, unsigned char byte, int size, int totalSize)
{
   if (track == nullptr)
      return index + (size);

   unsigned char previous_bit;
   if (index > 0)
   {
      previous_bit = track[index - 1];
   }
   else
   {
      previous_bit = track[totalSize - 1];
   }

   for (int i = 0; i < size; i += 2)
   {
      // Get bit
      unsigned char c = ((byte >> (7 - (i / 2))) & 0x01);

      if (c == 1)
      {
         track[index++] = 0;
         if (i + 1 < size)
            track[index] = 1;
      }
      else
      {
         track[index++] = (previous_bit == 0) ? 1 : 0;
         if (i + 1 < size)
            track[index] = 0;
      }
      if (i + 1 < size)
         previous_bit = track[index++];
   }
   return index;
}

int IDisk::AddByteToSpecificTrack(int side, unsigned int track, unsigned int index, unsigned char byte, int size)
{
   if (index + size >= side_[side].tracks[track].size)
   {
      unsigned char buffer[16];

      const int nb_byte_to_last_track = side_[side].tracks[track].size - index;

      if (nb_byte_to_last_track > 0)
      {
         // Index should be far more than 0 ! Otherwiser, track is about 16 bit long ??
         if (index > 0)
         {
            AddByteToTrack(buffer, 0, byte, 8, side_[side].tracks[track].bitfield[index - 1]);
         }
         else
         {
            AddByteToTrack(buffer, 0, byte, 8, 0);
         }
         memcpy(&side_[side].tracks[track].bitfield[index], buffer, nb_byte_to_last_track);

         // Copy remaining to begining of the track ?
         memcpy(&side_[side].tracks[track].bitfield[0], &buffer[nb_byte_to_last_track], size - nb_byte_to_last_track);
         return (size) - nb_byte_to_last_track;
      }
      // TODO !
      return 0;
   }
   return AddByteToTrackInBits(side_[side].tracks[track].bitfield, index, byte, size, side_[side].tracks[track].size);
}

int IDisk::AddByteToTrack(unsigned char* track, unsigned int index, unsigned char byte, int size,
                          unsigned char previous_bit)
{
   if (track == nullptr)
      return index + (size * 2);

   if (index > 0)
   {
      previous_bit = track[index - 1];
   }

   for (int i = 0; i < size; i++)
   {
      // Get bit
      unsigned char c = ((byte >> (7 - i)) & 0x01);

      if (c == 1)
      {
         track[index++] = 0;
         track[index] = 1;
      }
      else
      {
         track[index++] = (previous_bit == 0) ? 1 : 0;
         track[index] = 0;
      }

      previous_bit = track[index++];
   }
   return index;
}

unsigned char IDisk::GetNextByte(MFMTrack* track, unsigned int offset)
{
   unsigned char return_value = 0;

   // Move to DATA bit
   offset++;
   for (int i = 0; i < 16; i += 2)
   {
      return_value <<= 1;
      if (offset >= track->size)
         offset = offset - track->size;
      unsigned char b = track->bitfield[offset];
      return_value |= b & 0x1;
      offset += 2;
   }

   return return_value;
}

unsigned char IDisk::GetNextByte(int side, unsigned int track, unsigned int offset) const
{
   unsigned char return_value = 0;

   //
   offset++;
   for (int i = 0; i < 16; i += 2)
   {
      return_value <<= 1;
      if (offset >= side_[side].tracks[track].size)
         offset = offset % side_[side].tracks[track].size;

      const unsigned char b = side_[side].tracks[track].bitfield[offset];
      return_value |= b & 0x1;
      offset += 2;
   }

   return return_value;
}

unsigned char IDisk::GetNextByteForRev(int side, unsigned int track, unsigned int offset, int rev)
{
   unsigned char return_value = 0;

   //
   offset++;
   for (int i = 0; i < 16; i += 2)
   {
      return_value <<= 1;
      if (offset >= side_[side].tracks[track].revolution[rev].size)
         offset = offset - side_[side].tracks[track].revolution[rev].size;

      unsigned char b = side_[side].tracks[track].revolution[rev].bitfield[offset];

      return_value |= b & 0x1;
      offset += 2;
   }

   return return_value;
}

void IDisk::DumpTrack(int side, unsigned int track, int rev)
{
   unsigned int index = 0;
   int shift = 0;
   while (index < side_[side].tracks[track].size)
   {
      for (int i = 0; i < 2; i++)
      {
         unsigned char byte = 0;
         for (int j = 0; j < 8; j++)
         {
            byte <<= 1;
            unsigned char b = (index + i * 8 + j < side_[side].tracks[track].size)
                                 ? side_[side].tracks[track].bitfield[index + i * 8 + (j * 2)]
                                 : 0;
            byte |= b & 0x1;

            if (index + i * 8 + j >= side_[side].tracks[track]./*Revolution[rev].*/size)
               shift++;
         }
         LOGB(byte)
      }
      index += 16;
   }
   LOGEOL
   LOGB(shift)
}

void IDisk::DumpTrackRev(int side, unsigned int track, int rev)
{
   unsigned int index = 0;
   int shift = 0;
   while (index < side_[side].tracks[track].revolution[rev].size)
   {
      for (int i = 0; i < 2; i++)
      {
         unsigned char byte = 0;
         for (int j = 0; j < 8; j++)
         {
            byte <<= 1;
            unsigned char b = (index + i * 8 + j < side_[side].tracks[track].revolution[rev].size)
                                 ? side_[side].tracks[track].revolution[rev].bitfield[index + i * 8 + (j * 2)]
                                 : 0;
            byte |= b & 0x1;

            if (index + i * 8 + j >= side_[side].tracks[track].revolution[rev].size)
               shift++;
         }
         LOGB(byte)
      }
      index += 16;
   }
   LOGEOL
   LOGB(shift)
}


unsigned short IDisk::GetTrackInfo(int side, int track, Track* track_buffer)
{
   track_buffer->Clear();
   if (side < nb_sides_)
      return GetTrackInfo(&side_[side].tracks[track], track_buffer);
   return 0;
}


unsigned int IDisk::GetTrackInfo(MFMTrack* mfm_track, Track* track_buffer)
{
   unsigned int size;
   CRC crc_local;
   // Compute size of track :
   // Header
   size = 256;

   unsigned int index = 0;
   // States :
   // 0 - waiting for next sector id
   // 1 - waiting for next sector data
   // 2 - waiting for end of datas

   // A1 = 4489, FE =
   const char pattern_fe[] = {
      0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1,
      0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0
   };
   const char pattern_f[] = {
      0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1,
      0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1
   };

   if (mfm_track->size < (int)sizeof(pattern_fe))
   {
      return 0;
   }
   while (index < mfm_track->size - (int)sizeof(pattern_fe))
   {
      // Test the pervious 8 bytes (should be '00')
      bool sync_ok = (index > SYNC_BIT_NUMBER * 2);

      if (sync_ok)
      {
         sync_ok = (memcmp(pattern_fe, &mfm_track->bitfield[index], sizeof(pattern_fe)) == 0);
         if (sync_ok)
         {
            for (int i = 0; i < SYNC_BIT_NUMBER && sync_ok; i++)
            {
               if (mfm_track->bitfield[index - SYNC_BIT_NUMBER * 2 + i * 2] != 1
                  || mfm_track->bitfield[index - SYNC_BIT_NUMBER * 2 + i * 2 + 1] != 0)
               {
                  sync_ok = false;
               }
            }
         }
      }

      if (sync_ok)
      {
         // Found ! New sector
         crc_local.Reset();
         crc_local.AddByteToCrc(0xA1);
         crc_local.AddByteToCrc(0xA1);
         crc_local.AddByteToCrc(0xA1);
         crc_local.AddByteToCrc(0xFE);

         Sector sector;
         memset(&sector, 0, sizeof(sector));
         // Keep the offset position !
         sector.idam_offset = index;
         sector.idam_sync_offset = index - SYNC_BIT_NUMBER * 2;

         // Adding CHRN to the next sector
         index += sizeof(pattern_fe);

         sector.c = GetNextByte(mfm_track, index);
         crc_local.AddByteToCrc(sector.c);
         index += 16;
         sector.h = GetNextByte(mfm_track, index);
         crc_local.AddByteToCrc(sector.h);
         index += 16;
         sector.r = GetNextByte(mfm_track, index);
         crc_local.AddByteToCrc(sector.r);
         index += 16;
         sector.n = GetNextByte(mfm_track, index);
         crc_local.AddByteToCrc(sector.n);
         index += 16;

         LOG("CHRN Found");
         LOGB(sector.c );
         LOGB(sector.h );
         LOGB(sector.r );
         LOGB(sector.n );

         // Do we have CRC error ?
         unsigned short crc = GetNextByte(mfm_track, index);
         crc <<= 8;
         index += 16;
         crc |= GetNextByte(mfm_track, index);
         index += 16;

         // Set status 1
         if (crc != crc_local.GetCRC())
         {
            sector.status1 |= 0x20;
            LOG("HEADER CRC ERROR !");
         }
         LOGEOL
         // Now, waiting for A1A1A1F8/B
         bool data_found = false;
         while ((index < mfm_track->size - (int)sizeof(pattern_f)) && (!data_found))
         {
            if (memcmp(pattern_f, &mfm_track->bitfield[index], sizeof(pattern_f)) == 0)
            {
               data_found = true;
               // Keep the offset position
               sector.dam_offset = index;
               sector.dam_sync_offset = index - SYNC_BIT_NUMBER * 2; // todo : Add verify

               // Yeah
               index += sizeof(pattern_f) - 8;

               // Set the Status according to the F8/FB
               crc_local.Reset();
               crc_local.AddByteToCrc(0xA1);
               crc_local.AddByteToCrc(0xA1);
               crc_local.AddByteToCrc(0xA1);
               unsigned char byte = GetNextByte(mfm_track, index);
               crc_local.AddByteToCrc(byte);
               index += 16;

               if (byte == 0xF8) // Erased
               {
                  sector.status2 |= 0x40;
               }

               // Now, gather datas...
               int sectorsize = (0x80 << (std::min(sector.n, (unsigned char)0x8)));
               for (int data_count = 0;
                    data_count < sectorsize; data_count++)
               {
                  byte = GetNextByte(mfm_track, index);
                  crc_local.AddByteToCrc(byte);
                  index += 16;
                  if (index >= mfm_track->size)
                  {
                     index = index - mfm_track->size;
                  }

                  sector.real_size++;
               }
               if (index < mfm_track->size - 32)
               {
                  // CRC Check
                  crc = GetNextByte(mfm_track, index);
                  crc <<= 8;
                  index += 16;
                  crc |= GetNextByte(mfm_track, index);
                  index += 16;
               }
               else
               {
               }
               // Set status 1
               if (crc != crc_local.GetCRC())
               {
                  sector.status1 |= 0x20;
                  sector.status2 |= 0x20;
                  LOG("DATA CRC ERROR !");
               }
               LOGEOL
               // Keep this to compute GAP3 size
               sector.index_end = index;

               track_buffer->list_sector_.push_back(sector);

               size += sector.real_size;

               // Adaptation: rewind to sector.DAMOffset to avoid forgetting interlaced sectors
               index = sector.dam_offset + 64 + 1;
            }
            else
            {
               ++index;
            }
         }

         // Next sector
      }
      else
      {
         ++index;
      }
   }

   // Avegare GAP3 size :
   int gap_3_total = 0;
   unsigned int offset_end = 0;
   unsigned int max_index = 0;
   bool whole_track_recorded = false;
   unsigned int finalsize = 256;

   if (track_buffer->list_sector_.size() > 0)
   {
      for (unsigned int i = 0; i < track_buffer->list_sector_.size(); i++)
      {
         if (i > 0)
         {
            gap_3_total += track_buffer->list_sector_[i].idam_offset - 24 - offset_end;
         }

         // check size :
         //if ( track_buffer->listSector.size() > 1 && i < track_buffer->listSector.size()-1 )
         {
            // Interlaced sector :
            // Add full sector if :
            // - MaxIndex < endIndex (AND whole track is not recorded)
            // - totalsize + RealSize < 0x10000 (otherwise = problem with dsk header)
            if (max_index < track_buffer->list_sector_[i].dam_offset + 0x24 + track_buffer->list_sector_[i].real_size * 16
               && !whole_track_recorded
               && (finalsize + track_buffer->list_sector_[i].real_size) < 0x10000)
            {
               if (track_buffer->list_sector_[i].dam_offset + 0x24 + track_buffer->list_sector_[i].real_size * 16 > mfm_track->size)
               {
                  // Skip to first IDAM found
                  track_buffer->list_sector_[i].real_size = static_cast<unsigned short>((mfm_track->size - (track_buffer->list_sector_[i
                  ].dam_offset + 0x24) + track_buffer->list_sector_[0].idam_offset + 64) / 16);
                  whole_track_recorded = true;
               }
               max_index = track_buffer->list_sector_[i].index_end;
            }
               // Otherwise :
               // Add until IDAMOffset + 160 bye ( 4 for idam, 4 for chrn, 4 for CRC)
            else
            {
               if (track_buffer->list_sector_.size() > 1 && i < track_buffer->list_sector_.size() - 1)
               {
                  if (track_buffer->list_sector_[i].dam_offset + 0x24 + track_buffer->list_sector_[i].real_size * 16 > track_buffer->
                     list_sector_[i + 1].idam_offset + 160)
                  {
                     //track_buffer->listSector[i].RealSize -= (track_buffer->listSector[i].indexEnd - (track_buffer->listSector[i+1].IDAMOffset + 160))/16;
                     track_buffer->list_sector_[i].index_end = track_buffer->list_sector_[i + 1].idam_offset + 160;
                     track_buffer->list_sector_[i].real_size = (track_buffer->list_sector_[i].index_end - (track_buffer->list_sector_[i].
                        dam_offset + (3 * 16))) / 16;
                  }
               }
               else
               {
                  // Last track : go until begining of first sector
                  track_buffer->list_sector_[i].real_size = static_cast<unsigned short>((mfm_track->size - track_buffer->list_sector_[i]
                     .dam_offset + 0x24 + track_buffer->list_sector_[0].idam_offset + 64) / 16);
               }
            }
            if (track_buffer->list_sector_[i].dam_offset + 0x24 + track_buffer->list_sector_[i].real_size * 16 > mfm_track->size)
            {
               whole_track_recorded = true;
            }
         }
         finalsize += track_buffer->list_sector_[i].real_size;

         offset_end = track_buffer->list_sector_[i].index_end;
      }
      //gap_3_total += trackIn->Revolution[0].Size - offset_end;

      track_buffer->gap3_size_ = gap_3_total / 16 / track_buffer->list_sector_.size();

      if ((finalsize & 0xFFFFFF00) < finalsize)
      {
         finalsize += 0x100;
      }
      track_buffer->full_size_ = finalsize;
   }
   else
   {
      track_buffer->full_size_ = 0;
   }


   // TODO : Compute real size of track
   return finalsize;
}


unsigned short IDisk::GetTrackInfoForRev(int side, int track, Track* track_buffer, int rev)
{
   unsigned short size;
   CRC crc_local;
   // Compute size of track :
   // Header
   size = 256;

   unsigned int index = 0;
   // States :
   // 0 - waiting for next sector id
   // 1 - waiting for next sector data
   // 2 - waiting for end of datas

   // A1 = 4489, FE =
   const char pattern_fe[] = {
      0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1,
      0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0
   };
   const char pattern_f[] = {
      0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1,
      0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1
   };

   if (side_[side].tracks[track].revolution[rev].size < (int)sizeof(pattern_fe))
   {
      return 0;
   }

   while (index < side_[side].tracks[track].revolution[rev].size - (int)sizeof(pattern_fe))
   {
      // Test the pervious 8 bytes (should be '00')
      bool sync_ok = (index > 8 * 16);

      if (sync_ok)
      {
         sync_ok = (memcmp(pattern_fe, &side_[side].tracks[track].revolution[rev].bitfield[index],
                           sizeof(pattern_fe)) == 0);
         if (sync_ok)
         {
            for (int i = 0; i < SYNC_BIT_NUMBER && sync_ok; i++)
            {
               if (side_[side].tracks[track].revolution[rev].bitfield[index - SYNC_BIT_NUMBER * 2 + i * 2] != 1
                  || side_[side].tracks[track].revolution[rev].bitfield[index - SYNC_BIT_NUMBER * 2 + i * 2 + 1] != 0)
               {
                  sync_ok = false;
               }
            }
         }
      }
      if (sync_ok)
      {
         // Found ! New sector
         crc_local.Reset();
         crc_local.AddByteToCrc(0xA1);
         crc_local.AddByteToCrc(0xA1);
         crc_local.AddByteToCrc(0xA1);
         crc_local.AddByteToCrc(0xFE);

         Sector sector;
         memset(&sector, 0, sizeof(sector));
         // Keep the offset position !
         sector.idam_offset = index;
         sector.idam_sync_offset = index - SYNC_BIT_NUMBER * 2;

         // Adding CHRN to the next sector
         index += sizeof(pattern_fe);

         sector.c = GetNextByteForRev(side, track, index, rev);
         crc_local.AddByteToCrc(sector.c);
         index += 16;
         sector.h = GetNextByteForRev(side, track, index, rev);
         crc_local.AddByteToCrc(sector.h);
         index += 16;
         sector.r = GetNextByteForRev(side, track, index, rev);
         crc_local.AddByteToCrc(sector.r);
         index += 16;
         sector.n = GetNextByteForRev(side, track, index, rev);
         crc_local.AddByteToCrc(sector.n);
         index += 16;

         LOG("Track : ");
         LOGB(track);
         LOG(" - REV :");
         LOGB(rev);
         LOG(" -> CHRN Found");
         LOGB(sector.c);
         LOGB(sector.h);
         LOGB(sector.r);
         LOGB(sector.n);

         // Do we have CRC error ?
         unsigned short crc = GetNextByteForRev(side, track, index, rev);
         crc <<= 8;
         index += 16;
         crc |= GetNextByteForRev(side, track, index, rev);
         index += 16;

         // Set status 1
         if (crc != crc_local.GetCRC())
         {
            sector.chrn_crc_ok = false;
            sector.status1 |= 0x20;
            LOG("HEADER CRC ERROR !");
         }
         else
         {
            sector.chrn_crc_ok = true;
         }
         LOGEOL
         // Now, waiting for A1A1A1F8/B
         bool data_found = false;
         while ((index < side_[side].tracks[track].revolution[rev].size - (int)sizeof(pattern_f)) && (!data_found))
         {
            bool data_ok = (memcmp(pattern_fe, &side_[side].tracks[track].revolution[rev].bitfield[index],
                                   sizeof(pattern_f)) == 0);

            if (data_ok)
            {
               for (int i = 0; i < SYNC_BIT_NUMBER && sync_ok; i++)
               {
                  if (side_[side].tracks[track].revolution[rev].bitfield[index - SYNC_BIT_NUMBER * 2 + i * 2] != 1
                     || side_[side].tracks[track].revolution[rev].bitfield[index - SYNC_BIT_NUMBER * 2 + i * 2 + 1] != 0
                  )
                  {
                     data_ok = false;
                  }
               }
            }

            if (data_ok)
            {
               data_found = true;
               // Keep the offset position
               sector.dam_offset = index;
               sector.dam_sync_offset = index - SYNC_BIT_NUMBER * 2;

               // Yeah
               index += sizeof(pattern_f) - 8;

               // Set the Status according to the F8/FB
               crc_local.Reset();
               crc_local.AddByteToCrc(0xA1);
               crc_local.AddByteToCrc(0xA1);
               crc_local.AddByteToCrc(0xA1);
               unsigned char byte = GetNextByteForRev(side, track, index, rev);
               crc_local.AddByteToCrc(byte);
               LOG(" DATA FOUND : A1A1A1-");
               LOGB(byte);
               LOGEOL

               index += 16;

               if (byte == 0xF8) // Erased
               {
                  sector.status2 |= 0x40;
               }

               // Now, gather datas...
               int sizeMaxSector = (sector.n & 0x7);
               for (int data_count = 0; data_count < (0x80 << sizeMaxSector); data_count++)
               {
                  if (index + 16 < side_[side].tracks[track].revolution[rev].size)
                  {
                     byte = GetNextByteForRev(side, track, index, rev);
                     crc_local.AddByteToCrc(byte);
                     sector.real_size++;
                     index += 16;
                  }
                  else
                  {
                     // Go through the index hole
                     unsigned char return_value = 0;

                     for (int i = 0; i < 16; i += 2)
                     {
                        int readoffset;
                        return_value <<= 1;
                        if (index >= side_[side].tracks[track].revolution[rev].size)
                        {
                           readoffset = index % side_[side].tracks[track].revolution[rev].size;
                        }
                        else
                        {
                           readoffset = index;
                        }
                        unsigned char b = side_[side].tracks[track].revolution[rev].bitfield[readoffset];
                        index += 2;
                        return_value |= b & 0x1;
                     }
                     crc_local.AddByteToCrc(return_value);
                     sector.real_size++;
                     //index = readoffset;
                  }
               }
               if (index < side_[side].tracks[track].revolution[rev].size - 32)
               {
                  // CRC Check
                  crc = GetNextByteForRev(side, track, index, rev);
                  crc <<= 8;
                  index += 16;
                  crc |= GetNextByteForRev(side, track, index, rev);
                  index += 16;
               }
               else
               {
                  crc = 0;

                  for (int i = 0; i < 32; i += 2)
                  {
                     int readoffset;
                     crc <<= 1;
                     if (index >= side_[side].tracks[track].revolution[rev].size)
                     {
                        readoffset = index % side_[side].tracks[track].revolution[rev].size;
                     }
                     else
                     {
                        readoffset = index;
                     }
                     unsigned char b = side_[side].tracks[track].revolution[rev].bitfield[readoffset];
                     index += 2;
                     crc |= b & 0x1;
                  }

                  //
                  LOG("Track size is too small ?!!");
               }
               // Set status 1
               if (crc != crc_local.GetCRC())
               {
                  sector.data_crc_ok = false;
                  sector.status1 |= 0x20;
                  sector.status2 |= 0x20;
                  LOG("DATA CRC ERROR !");
               }
               else
               {
                  sector.data_crc_ok = true;
               }
               LOGEOL
               // Keep this to compute GAP3 size
               sector.index_end = index;

               if (index < side_[side].tracks[track].revolution[rev].size - 16)
               {
                  // End of sector. Look at the GAP until Sync is found - Only get it if different from 0x4E
                  byte = GetNextByteForRev(side, track, index, rev);
                  if (byte != 0x4E && byte != 0x4B)
                  {
                     sector.real_size += 2; // Add CRC + first gap

                     while ((memcmp(pattern_fe, &side_[side].tracks[track].revolution[rev].bitfield[index],
                                    sizeof(pattern_f)) != 0)
                        && (index < side_[side].tracks[track].revolution[rev].size - (int)sizeof(pattern_f)))
                     {
                        index += 16;
                        ++sector.real_size;
                        /*byte = GetNextByteForRev (side, track, index );
                        index += 16;*/
                     }
                  }
               }

               // NOTE : It can be a bit misalign, so sync bytes are watched on a bit basis
               // TODO

               track_buffer->list_sector_.push_back(sector);

               size += sector.real_size;

               // Adaptation: rewind to sector.DAMOffset to avoid forgetting interlaced sectors
               //index = sector.DAMOffset + 64 + 1;
               index = sector.idam_offset + 64 + 1;
            }
            else
            {
               ++index;
            }
         }

         // Next sector
      }
      else
      {
         ++index;
      }
   }

   // Avegare GAP3 size :
   int gap_3_total = 0;
   int offset_end = 0;
   if (track_buffer->list_sector_.size() > 0)
   {
      for (unsigned int i = 0; i < track_buffer->list_sector_.size(); i++)
      {
         if (i > 0)
         {
            gap_3_total += track_buffer->list_sector_[i].idam_offset - 24 - offset_end;
         }
         offset_end = track_buffer->list_sector_[i].index_end;
      }
      //gap_3_total += side_[side].Tracks[track].Revolution[0].Size - offset_end;

      track_buffer->gap3_size_ = gap_3_total / 16 / track_buffer->list_sector_.size();

      if ((size & 0xFFFFFF00) < size)
      {
         size += 0x100;
      }
      track_buffer->full_size_ = size;
   }
   else
   {
      track_buffer->full_size_ = 0;
   }


   // TODO : Compute real size of track
   return size;
}


int IDisk::CompareForSync(unsigned char* src, unsigned char* src2, int size)
{
   int found = 0;

   // Src is from unsafe source : It can be :
   // - Shiffted
   // - incomplete (only one bit every 2 MFM bit is stored - It can be data or clock bit)
   for (int i = 1; i < size; i += 2)
   {
      // Add this : 10 == 00
      // Only check data bit (no clock bit)
      if (src[i] != src2[i])
      {
         found = -1;
         break;
      }
   }

   if (found == 0)
      return found;

   found = 1;
   for (int i = 1; i < size; i += 2)
   {
      // Now, check clock bit (maybe it's the one that is stored)
      if ((src[i] != src2[i - 1]) && (i > 1 || src2[i] == 1))
      {
         found = -1;
         break;
      }
   }
   return found;
}

unsigned int IDisk::AdjustSideOfTrack(unsigned char* bit_field, unsigned int nb_bit_in_flux)
{
   // Check if begining of track can be found at the end of track.
   // Count the occurence of matching bits.
   int count = 8;
   bool found_ok = true;
   while (found_ok && (nb_bit_in_flux - 1 - count) > (6000 * 16))
   {
      if (memcmp(&bit_field[nb_bit_in_flux - 1 - count], bit_field, count) != 0)
      {
         count ++;
      }
      else
      {
         found_ok = false;
      }
   }

   if (!found_ok)
      nb_bit_in_flux -= (count + 1);
   return nb_bit_in_flux;
}

unsigned int GetNextFuzzyBits(unsigned char* buffer, unsigned int offset_begin, unsigned int offset_end,
                              bool bSearchForFuzzy)
{
   //
   while (offset_begin < offset_end)
   {
      if (bSearchForFuzzy)
      {
         if (buffer[offset_begin] & BIT_WEAK)
         {
            // Found !
            return offset_begin;
         }
      }
      else
      {
         if ((buffer[offset_begin] & BIT_WEAK) == 0)
         {
            // Found !
            return offset_begin;
         }
      }

      offset_begin++;
   }
   return -1;
}

unsigned int FindNextSector(unsigned char* buffer, unsigned int size, unsigned int offset, int size_to_search)
{
   // Search for 0xA1A1A1FE ( 10xx for last quartet)
   // { 0x44, 0x89, 0x44, 0x89, 0x44, 0x89, 0x55, 0x4x };
   // Convert pattern to bit array
   const unsigned char pattern_in_bits [] = {
      0, 1, 0, 0, 0, 1, 0, 0, // 44
      1, 0, 0, 0, 1, 0, 0, 1, // 89
      0, 1, 0, 0, 0, 1, 0, 0, // 44
      1, 0, 0, 0, 1, 0, 0, 1, // 89
      0, 1, 0, 0, 0, 1, 0, 0, // 44
      1, 0, 0, 0, 1, 0, 0, 1, // 89
      0, 1, 0, 1, 0, 1, 0, 1, // 55
      0, 1, 0, 1, 0, 1, 0, 0 // 54
   };
   // Find this array
   while ( (offset + size_to_search * 8) < size && memcmp(&buffer[offset], pattern_in_bits, size_to_search * 8) != 0)
   {
      ++offset;
   }

   if ((offset + size_to_search * 8) >= size) return -1;
   return offset;
}

unsigned int FindNextPattern(unsigned char* buffer, unsigned int size, unsigned int offset,
                             const unsigned char* pattern, unsigned int size_of_pattern)
{
   // Convert pattern to bit array
   unsigned char* pattern_in_bits = new unsigned char[size_of_pattern * 8];
   int index = 0;
   for (unsigned int i = 0; i < size_of_pattern; i++)
   {
      for (int j = 0; j < 8; j++)
      {
         unsigned char c = ((pattern[i] >> (7 - j)) & 0x01);
         pattern_in_bits[index++] = c;
      }
   }

   // Find this array

   while (offset < size && memcmp(&buffer[offset], pattern_in_bits, size_of_pattern * 8) != 0)
   {
      ++offset;
   }

   delete[]pattern_in_bits;

   if (offset == size) return -1;
   return offset;
}

void IDisk::CreateSingleTrackFromMultiRevolutions(int side, int track)
{
   // Only one revolution : Nothing else to add...
   if (side_[side].tracks[track].nb_revolutions == 1)
   {
      side_[side].tracks[track].bitfield = side_[side].tracks[track].revolution[0].bitfield;
      side_[side].tracks[track].size = side_[side].tracks[track].revolution[0].size;

      delete[]side_[side].tracks[track].revolution;
      side_[side].tracks[track].nb_revolutions = 0;
      side_[side].tracks[track].revolution = nullptr;
   }
   else
   {
      // Otherwise
      Track track_buffer;

      if (!CreateTrackFromRevolution(side, track))
      {
         for (unsigned int rev = 0; rev < side_[side].tracks[track].nb_revolutions; rev++)
         {
            delete[]side_[side].tracks[track].revolution[rev].bitfield;
         }
         delete[]side_[side].tracks[track].revolution;
         side_[side].tracks[track].revolution = nullptr;
         side_[side].tracks[track].nb_revolutions = 0;
         side_[side].tracks[track].bitfield = nullptr;
         side_[side].tracks[track].size = 0;
      }
      else
      {
         for (unsigned int rev = 0; rev < side_[side].tracks[track].nb_revolutions; rev++)
         {
            delete[]side_[side].tracks[track].revolution[rev].bitfield;
         }
         delete[]side_[side].tracks[track].revolution;
         side_[side].tracks[track].nb_revolutions = 0;
         side_[side].tracks[track].revolution = nullptr;
      }
   }
   // Now, just clear the Revolution structure.
   if (side_[side].tracks[track].bitfield == nullptr)
   {
      // Set a default (unformatted) track
      side_[side].tracks[track].size = DEFAULT_TRACK_SIZE * 16;
      side_[side].tracks[track].bitfield = new unsigned char[DEFAULT_TRACK_SIZE * 16];
      memset(side_[side].tracks[track].bitfield, BIT_WEAK, DEFAULT_TRACK_SIZE * 16);
   }
}

void IDisk::CreateTrackFromMultiRevolutions()
{
   //   START_CHRONO

   // Create one single track with WEAK informations from
   for (int side = 0; side < nb_sides_; side ++)
   {
      for (unsigned int track = 0; track < side_[side].nb_tracks; track++)
      {
         CreateSingleTrackFromMultiRevolutions(side, track);
      }
   }
   //   STOP_CHRONO
   //   PROF_DISPLAY
}

// TODO :
// - Si index retourne a 0... Gere ca d'une certaine maniere ... (comment ?)
// - Reflechir sur taille de piste !!!

#define INC_INDEXEX  for (unsigned int rev = 0; rev < side_[side].tracks[track].nb_revolutions ; rev++){if (index_buffer[rev] != -1) index_buffer[rev]++;if (index_buffer[rev]>=side_[side].tracks[track].revolution[rev].size) index_buffer[rev]=-1;}
#define RECOVER_SIZE 14
#define MOVING_AREA 64
//#define _OldSearch

// Find next correct area : This area begin with an incorrect area (indexed by current_search_revolution)
// Input : pNextCorrectIndex are filled with the end of the area to search
// Return : pNextCorrectIndex with index of correct area. Return number of correct bits
int IDisk::FindEndOfWeakArea(int side, int track, unsigned int* current_search_revolution, unsigned int* next_correct_index,
                             unsigned int nb_used_revolutions, int* rev_to_use, bool revolution_wrecked[16])
{
   // Init : The ending is, by default, pNextCorrectIndex
   unsigned int ending_search[16];
   unsigned int best_count = 0;
   int tmp_buffer[16];

   int refrev = -1;

   for (unsigned int rev = 0; rev < nb_used_revolutions; rev++)
   {
      if (!revolution_wrecked[rev])
      {
         ending_search[rev] = next_correct_index[rev];
         if (refrev == -1)
            refrev = rev;
      }
   }

   if (refrev == -1) return 0;

   // Rev0 is the reference.
   unsigned int maxRev0 = ending_search[refrev];

   for (unsigned int index = 0; index < maxRev0 - current_search_revolution[refrev] - RECOVER_SIZE
        && index < (ending_search[refrev] - current_search_revolution[refrev])
        && best_count < maxRev0 - current_search_revolution[refrev] - index
        && (maxRev0 >= (current_search_revolution[refrev] + RECOVER_SIZE)); index++)
   {
      tmp_buffer[refrev] = index + current_search_revolution[refrev];
      for (unsigned int i = 0; i < nb_used_revolutions; i++) { if (refrev != i) tmp_buffer[i] = -1; }
      for (unsigned int rev = 0; rev < nb_used_revolutions; rev++)
      {
         if (refrev != rev && !revolution_wrecked[rev])
         {
            // Walk every reference until we find something that is same (at least 14 bits correct)
            unsigned int min = (index < MOVING_AREA)
                                  ? (current_search_revolution[rev])
                                  : (current_search_revolution[rev] + index - MOVING_AREA);
            unsigned int max = current_search_revolution[rev] + index + MOVING_AREA;
            if (max > ending_search[rev] - RECOVER_SIZE - 1) max = ending_search[rev] - RECOVER_SIZE - 1;
            bool found = false;
            for (unsigned int i = min; i < max && !found; i++)
            {
               if (side_[side].tracks[track].revolution[refrev].size <= current_search_revolution[refrev] + index + RECOVER_SIZE
                  || side_[side].tracks[track].revolution[rev].size <= i + RECOVER_SIZE)
               {
                  found = false;
               }
               else
               {
                  if (memcmp(&side_[side].tracks[track].revolution[refrev].bitfield[current_search_revolution[refrev] + index],
                             &side_[side].tracks[track].revolution[rev].bitfield[i], RECOVER_SIZE) == 0)
                  {
                     tmp_buffer[rev] = i;
                     found = true;
                  }
                     // Half (one on 2) is the same - For all those dump with half datas (clock or data)
                  else
                  {
                     bool ok = true;
                     for (int i2 = 0; i2 < RECOVER_SIZE; i2 += 2)
                     {
                        if (side_[side].tracks[track].revolution[refrev].bitfield[current_search_revolution[refrev] + index + i2
                           ] != side_[side].tracks[track].revolution[rev].bitfield[i + i2]
                        )
                        {
                           ok = false;
                        }
                     }
                     if (ok)
                     {
                        tmp_buffer[rev] = i;
                        found = true;
                        // Only half is correct
                     }
                  }
               }
            }
         }
      }

      // All Tmp != 0 ?
      bool ok = true;
      for (unsigned int rev = 1; rev < nb_used_revolutions; rev++)
      {
         if (!revolution_wrecked[rev] && (tmp_buffer[rev] == -1))
         {
            ok = false;
         }
      }
      // Found ! Count how many bits are correct.
      if (ok)
      {
         unsigned int count = 0;
         int revolution_count = -1;
         int bit_corrects[16][16];
         for (unsigned int i = 0; i < nb_used_revolutions; i++)
         {
            for (unsigned int j = i + 1; j < nb_used_revolutions; j++)
            {
               if (!revolution_wrecked[i] && !revolution_wrecked[j])
               {
                  // Comparaison des revolutions i et j
                  bool revolution_comp_ok = true;
                  bit_corrects[i][j] = 0;
                  unsigned int localcount = 0;
                  // Max comp : pTmp -> pNextCorrectIndex
                  unsigned int max_comp_1 = next_correct_index[i] - tmp_buffer[i];
                  unsigned int max_comp_2 = next_correct_index[j] - tmp_buffer[j];
                  unsigned int min_comp = std::min(max_comp_1, max_comp_2);

                  while (revolution_comp_ok && localcount <= min_comp)
                  {
                     // Compare
                     if (side_[side].tracks[track].revolution[i].bitfield[tmp_buffer[i] + localcount] != side_[side].tracks[
                        track].revolution[j].bitfield[tmp_buffer[j] + localcount])
                     {
                        revolution_comp_ok = false;
                     }
                     else
                     {
                        localcount += 2;
                     }
                  }
                  if (localcount > min_comp) localcount = min_comp;
                  bit_corrects[i][j] = localcount;
                  if (count < localcount)
                  {
                     count = localcount;
                     revolution_count = i;
                  }
               }
            }
         }

         // Better than the previous one ?
         if (count > best_count)
         {
            // Yes : set it as the best candidate
            best_count = count;
            *rev_to_use = revolution_count;
            // In all case, set the end of the search to the end of this set
            for (unsigned int rev = 0; rev < nb_used_revolutions; rev++)
            {
               if (!revolution_wrecked[rev])
               {
                  ending_search[rev] = tmp_buffer[rev] + count;
                  if (ending_search[rev] > side_[side].tracks[track].revolution[rev].size)
                  {
                     if (rev == nb_used_revolutions - 1)
                        revolution_wrecked[rev] = true;
                     else
                        ending_search[rev] = side_[side].tracks[track].revolution[rev].size;
                  }
               }
            }
         }
      }
   }

   if (best_count > 0)
   {
      for (unsigned int rev = 0; rev < nb_used_revolutions; rev++)
      {
         if (!revolution_wrecked[rev])
            next_correct_index[rev] = ending_search[rev];
      }

      // Check recursively for inner area search, from initial current_search_revolution to pEndingSearch-bestCount
      unsigned int inner_search_end[16];
      for (unsigned int rev = 0; rev < nb_used_revolutions; rev++)
      {
         if (!revolution_wrecked[rev])
            inner_search_end[rev] = ending_search[rev] - best_count;
      }

      int inner_revolution_to_use = -1;
      int inner_best_count = FindEndOfWeakArea(side, track, current_search_revolution, inner_search_end, nb_used_revolutions,
                                             &inner_revolution_to_use, revolution_wrecked);
      if (inner_best_count != 0)
      {
         // Return these datas
         for (unsigned int rev = 0; rev < nb_used_revolutions; rev++)
         {
            if (!revolution_wrecked[rev])
               next_correct_index[rev] = inner_search_end[rev];
         }
         *rev_to_use = inner_revolution_to_use;

         return inner_best_count;
      }
      // Nothing found : Return our datas
      return best_count;
   }
   return best_count;
}

bool IDisk::CreateTrackFromRevolution(int side, int track)
{
   for (unsigned int rev = 0; rev < side_[side].tracks[track].nb_revolutions; rev++)
   {
      GetTrackInfoForRev(side, track, &side_[side].tracks[track].revolution[rev].track_info, rev);
   }

   int size = CreateTrackFromRevolutionWithSize(side, track, 0);

   if (size != 0)
   {
      CreateTrackFromRevolutionWithSize(side, track, size);
   }

   return (size != 0);
}

// Ok then...
// Take the MAX size, from first IDAM to first IDAM in next revolution
int IDisk::CreateUnformattedTrack(int side, int track)
{
   // todo
   int size = 100000;
   if (size != 0)
   {
      side_[side].tracks[track].size = size;
      side_[side].tracks[track].bitfield = new unsigned char[side_[side].tracks[track].size + 32];

      for (int i = 0; i < size; i++)
      {
         side_[side].tracks[track].bitfield[i] = BIT_WEAK;
      }
   }

   return size;
}

int IDisk::CreateTrackFromRevolutionWithSize(int side, int track, int sizeOfTrack)
{
   if (side_[side].tracks[track].nb_revolutions == 0) return 0;

   int index_write = 0;
   std::vector<unsigned int>* list_sizes = new std::vector<unsigned int>[side_[side].tracks[track].nb_revolutions];

   int* first_sync_of_track = new int[side_[side].tracks[track].nb_revolutions];
   memset(first_sync_of_track, 0, sizeof(int) * side_[side].tracks[track].nb_revolutions);
   int* size_after = new int[side_[side].tracks[track].nb_revolutions];
   memset(size_after, 0, sizeof(int) * side_[side].tracks[track].nb_revolutions);
   bool is_bad_track = false;

   int size_buffer[16] = {0};
   unsigned char* tmp_buffer[16] = {nullptr};

   bool revolution_wrecked[16] = {0};

   //if ( sizeOfTrack == 0)
   {
      for (unsigned int rev = 0; rev < side_[side].tracks[track].nb_revolutions; rev++)
      {
         //GetTrackInfoForRev(side, track, &side_[side].Tracks[track].Revolution[rev].TrackInfo, rev);
         size_buffer[rev] = side_[side].tracks[track].revolution[rev].size;
         tmp_buffer[rev] = new unsigned char [size_buffer[rev] ];
         memcpy(tmp_buffer[rev], side_[side].tracks[track].revolution[rev].bitfield, size_buffer[rev]);
      }
   }

   // Check the value computed
   // Here, assume that the first SYNC is identical. If it's not the case, the dump's revolutions should be corrected
   for (unsigned int rev = 0; rev < side_[side].tracks[track].nb_revolutions && !is_bad_track; rev++)
   {
      unsigned char* track_field = side_[side].tracks[track].revolution[rev].bitfield;
      unsigned int track_size = side_[side].tracks[track].revolution[rev].size;
      unsigned int search_offset = FindNextPattern(track_field, track_size, 0, PATTERN_C2, 6);

      if (search_offset != -1)
      {
         int tmp = search_offset;
         if (search_offset > 16 * 12)
            search_offset -= 16 * 12;

         first_sync_of_track[rev] = search_offset;
         search_offset = tmp;
      }
      else
      {
         search_offset = FindNextSector(track_field, track_size, search_offset + 1, 7);
         if (search_offset != -1)
         {
            // 0x00 sync bytes
            int tmp = search_offset;
            if (search_offset > 16 * 12)
               search_offset -= 16 * 12;

            first_sync_of_track[rev] = search_offset;
            search_offset = tmp;
         }
         else
         {
            is_bad_track = true;
         }
      }
      list_sizes[rev].push_back(first_sync_of_track[rev]);

      // Next sector
      unsigned int search_next_offset;
      search_next_offset = FindNextSector(track_field, track_size, search_offset + 1, 7);
      while (search_next_offset != -1)
      {
         // 0x00 sync bytes
         search_offset = search_next_offset;
         if (search_offset > 16 * 12)
            search_offset -= 16 * 12;

         list_sizes[rev].push_back(search_offset);

         search_offset = search_next_offset;
         search_next_offset = FindNextSector(track_field, track_size, search_offset + 1, 7);
      }
      int lastData = track_size;
      list_sizes[rev].push_back(lastData);
   }
   // All sectors header positions are now known in pListSizes.

   // Now, compute total max size !
   if (!is_bad_track)
   {
      // Recompute tracks from first sync to sync in next revolution, with
      // - for 3+ rev, in rev-1
      // - for 2 rev, in 2 rev (what else)

      // keep an "average" offset from begining, then
      // Begining is at first sync byte, until end of track, and add to this begining of next track until first sync byte.
      // for 2 only rev, add first rev to end of 2nd one
      unsigned int nb_used_revolutions;
      if (side_[side].tracks[track].nb_revolutions == 2)
      {
         nb_used_revolutions = 2;

         unsigned char* buf = new unsigned char[side_[side].tracks[track].revolution[0].size + first_sync_of_track[1] -
            first_sync_of_track[0]];
         memcpy(buf, &side_[side].tracks[track].revolution[0].bitfield[first_sync_of_track[0]],
                side_[side].tracks[track].revolution[0].size - first_sync_of_track[0]);
         memcpy(&buf[side_[side].tracks[track].revolution[0].size - first_sync_of_track[0]],
                side_[side].tracks[track].revolution[1].bitfield, first_sync_of_track[1]);

         delete[]side_[side].tracks[track].revolution[0].bitfield;
         side_[side].tracks[track].revolution[0].bitfield = buf;
         side_[side].tracks[track].revolution[0].size += first_sync_of_track[1] - first_sync_of_track[0];


         buf = new unsigned char[side_[side].tracks[track].revolution[1].size + first_sync_of_track[0] -
            first_sync_of_track[1]];
         memcpy(buf, &side_[side].tracks[track].revolution[1].bitfield[first_sync_of_track[1]],
                side_[side].tracks[track].revolution[1].size - first_sync_of_track[1]);
         memcpy(&buf[side_[side].tracks[track].revolution[1].size - first_sync_of_track[1]],
                side_[side].tracks[track].revolution[0].bitfield, first_sync_of_track[0]);

         delete[]side_[side].tracks[track].revolution[1].bitfield;
         side_[side].tracks[track].revolution[1].bitfield = buf;
         side_[side].tracks[track].revolution[1].size += first_sync_of_track[0] - first_sync_of_track[1];
      }
      else
      {
         nb_used_revolutions = side_[side].tracks[track].nb_revolutions /*- 1*/;
         for (unsigned int rev = 0; rev < nb_used_revolutions; rev++)
         {
            unsigned char* buf;
            if (rev < side_[side].tracks[track].nb_revolutions - 1)
               buf = new unsigned char[side_[side].tracks[track].revolution[rev].size + first_sync_of_track[rev + 1] -
                  first_sync_of_track[rev]];
            else
               buf = new unsigned char[side_[side].tracks[track].revolution[rev].size - first_sync_of_track[rev]];

            memcpy(buf, &side_[side].tracks[track].revolution[rev].bitfield[first_sync_of_track[rev]],
                   side_[side].tracks[track].revolution[rev].size - first_sync_of_track[rev]);
            if (rev < side_[side].tracks[track].nb_revolutions - 1)
               memcpy(&buf[side_[side].tracks[track].revolution[rev].size - first_sync_of_track[rev]],
                      side_[side].tracks[track].revolution[rev + 1].bitfield, first_sync_of_track[rev + 1]);

            //if (sizeOfTrack != 0)
            {
               delete[]side_[side].tracks[track].revolution[rev].bitfield;
            }
            side_[side].tracks[track].revolution[rev].bitfield = buf;

            if (rev < side_[side].tracks[track].nb_revolutions - 1)
               side_[side].tracks[track].revolution[rev].size += first_sync_of_track[rev + 1] - first_sync_of_track[rev];
            else
               side_[side].tracks[track].revolution[rev].size -= first_sync_of_track[rev];
         }
      }

      // Adjust positions
      for (unsigned int rev = 0; rev < nb_used_revolutions; rev++)
      {
         for (std::vector<unsigned int>::iterator it = list_sizes[rev].begin(); it != list_sizes[rev].end(); ++it)
         {
            (*it) -= first_sync_of_track[rev];
         }
         // last one : add next
         if (nb_used_revolutions < side_[side].tracks[track].nb_revolutions || rev != nb_used_revolutions - 1)
            list_sizes[rev].back() += first_sync_of_track[rev + 1];
         /*else
            pListSizes[rev].back() += pFirstSyncOfTrack[0];*/
      }

      // TODO : Adjust track. Remove track if, for exemple, only one of 4 is bad in size


      unsigned int max_size = 0;

      unsigned int index = 1;
      bool not_finished = true;
      unsigned int keep_rev_end = -1;

      while (not_finished)
      {
         not_finished = false;
         unsigned int max_sector_size = 0;
         for (unsigned int rev = 0; rev < nb_used_revolutions - 1 && !is_bad_track; rev++)
         {
            if (index < list_sizes[rev].size())
            {
               unsigned int sector_size = list_sizes[rev].at(index) - list_sizes[rev].at(index - 1);
               if (index == list_sizes[rev].size() - 1)
               {
                  if (nb_used_revolutions < side_[side].tracks[track].nb_revolutions || rev == nb_used_revolutions - 1)
                     sector_size += first_sync_of_track[rev + 1];
                  /*else
                     sectorSize += pFirstSyncOfTrack[0];*/
               }
               if (max_sector_size < sector_size)
               {
                  max_sector_size = sector_size;
                  not_finished = true;
               }
            }
         }
         index++;
         max_size += max_sector_size;
      }

      // Max track is now known !
      // Note : This can be wrong : It can be more than that. In fact, we should compute this from all weak areas....
      // Let's say we can have no more thant 256 bits "floating". This is 64 bytes, wich should be enough.
      if (sizeOfTrack != 0)
      {
         side_[side].tracks[track].size = sizeOfTrack;
         side_[side].tracks[track].bitfield = new unsigned char[side_[side].tracks[track].size + 32];
      }
      else
      {
         side_[side].tracks[track].size = max_size;
      }

      // Now, report data to bitfield, with weak / optional / index data
      // Index -> First sync data. Use 2nd revolution

      // Now, check each "sectorized" bloc
      unsigned int* index_buffer = new unsigned int[side_[side].tracks[track].nb_revolutions];
      memset(index_buffer, 0, sizeof(unsigned int) * side_[side].tracks[track].nb_revolutions);
      // Set index to first sync byte

      unsigned int index_for_next_sector = 0;

      unsigned int normal_sector_count = 0;
      for (unsigned int rev = 0; rev < nb_used_revolutions; rev++)
      {
         if (normal_sector_count < list_sizes[rev].size())
            normal_sector_count = list_sizes[rev].size();
      }

      for (unsigned int rev = 0; rev < nb_used_revolutions; rev++)
      {
         index_buffer[rev] = list_sizes[rev].at(index_for_next_sector);
         if (normal_sector_count > list_sizes[rev].size())
         {
            index_buffer[rev] = -1;
            revolution_wrecked[rev] = true;
         }
         else
         {
            revolution_wrecked[rev] = false;
         }
      }

      // Until the end of the track...
      bool end = false;

      int revolution_to_keep = -1;
      keep_rev_end = -1;

      while (!end)
      {
         // Count only on a byte !
         // Process next byte.
         bool ok = true;

         // resync :
         unsigned int index_of_resync = -1;
         for (unsigned int rev = 0; rev < nb_used_revolutions && index_of_resync == -1; rev++)
         {
            while ((index_for_next_sector < list_sizes[rev].size()) && (list_sizes[rev].at(index_for_next_sector) < index_buffer[rev]))
            {
               index_for_next_sector++;
            }
            if ((index_for_next_sector < list_sizes[rev].size()) && (list_sizes[rev].at(index_for_next_sector) == index_buffer[rev]))
            {
               index_of_resync = index_for_next_sector;
            }
         }
         if (index_of_resync != -1)
         {
            for (unsigned int rev = 0; rev < nb_used_revolutions; rev++)
            {
               if (!revolution_wrecked[rev])
               {
                  index_buffer[rev] = list_sizes[rev].at(index_of_resync);
                  if (index_buffer[rev] == side_[side].tracks[track].revolution[rev].size)
                     index_buffer[rev] = -1;
               }

               else
               {
                  // No area here ??
                  index_buffer[rev] = -1;
               }
               if (nb_used_revolutions == side_[side].tracks[track].nb_revolutions && rev == nb_used_revolutions - 1)
               {
                  if (!revolution_wrecked[nb_used_revolutions - 1] && index_buffer[nb_used_revolutions - 1] != -1)
                  {
                     if (index_buffer[nb_used_revolutions - 1] >= list_sizes[nb_used_revolutions - 1].at(
                        list_sizes[nb_used_revolutions - 1].size() - 2))
                     {
                        revolution_wrecked[nb_used_revolutions - 1] = true;
                     }
                  }
               }
            }
         }

         int first_revolution_available = -1;
         for (unsigned int rev = 0; rev < nb_used_revolutions && first_revolution_available == -1; rev++)
         {
            if (index_buffer[rev] != -1 && !revolution_wrecked[rev])
               first_revolution_available = rev;
         }


         unsigned char val = 0;
         if (first_revolution_available == -1)
         {
            ok = false;
            //bEnd = true;
         }
         else
         {
            //side_[side].Tracks[track].Revolution[firstRevAvailable].BitField[pIndex[firstRevAvailable]];
            val = side_[side].tracks[track].revolution[first_revolution_available].bitfield[index_buffer[first_revolution_available]];

            for (unsigned int rev = 0; rev < nb_used_revolutions; rev++)
            {
               if (first_revolution_available != rev && !revolution_wrecked[rev])
               {
                  if (index_buffer[rev] == -1)
                  {
                     ok = false;
                  }
                  else
                  {
                     if (val != side_[side].tracks[track].revolution[rev].bitfield[index_buffer[rev]])
                     {
                        // Different
                        ok = false;
                     }
                  }
               }
            }
         }

         if (!ok)
         {
            // Use the CRC mode : If any rev has a good CRC, use it
            int correct_revolution = -1;

            // TODO :
            // revToKeep should be reset when a certain point is meet
            if (revolution_to_keep != -1 && index_buffer[revolution_to_keep] != -1 && index_buffer[revolution_to_keep] <= keep_rev_end)
            {
               val = side_[side].tracks[track].revolution[revolution_to_keep].bitfield[index_buffer[revolution_to_keep]];

               ok = true;

               if (index_buffer[revolution_to_keep] == keep_rev_end)
               {
                  // Set all rev to the same index !
                  unsigned char c, h, r, n;
                  bool is_header;
                  if (side_[side].tracks[track].revolution[revolution_to_keep].track_info.GetRFromIndex(
                     index_buffer[revolution_to_keep] - 1 + first_sync_of_track[
                        revolution_to_keep], &c, &h, &r, &n, &is_header))
                  {
                     for (unsigned int rev = 0; rev < nb_used_revolutions; rev++)
                     {
                        if (rev != revolution_to_keep && !revolution_wrecked[rev])
                        {
                           int end_index;
                           if (side_[side].tracks[track].revolution[revolution_to_keep].track_info.GetEndIndexOfSector(
                              c, h, r, n, is_header, &end_index))
                           {
                              index_buffer[rev] = end_index - first_sync_of_track[rev] + 1;
                           }
                        }
                     }
                  }
                  revolution_to_keep = -1;
               }
            }
            else
            {
               for (unsigned int rev = 0; rev < nb_used_revolutions && correct_revolution == -1; rev++)
               {
                  unsigned int end_of_rev;
                  if (!revolution_wrecked[rev] && index_buffer[rev] != -1 && side_[side].tracks[track].revolution[rev]
                                                                       .track_info.IsBitInACorrectSector(
                                                                          index_buffer[rev] + first_sync_of_track[rev],
                                                                          &end_of_rev))
                  {
                     // Found a correct revolution -> Just think that it's the right one, until next pIndexNextSync
                     correct_revolution = rev;
                     revolution_to_keep = rev;

                     keep_rev_end = end_of_rev - first_sync_of_track[rev];
                  }
               }

               if (correct_revolution != -1)
               {
                  val = side_[side].tracks[track].revolution[correct_revolution].bitfield[index_buffer[correct_revolution]];
                  ok = true;
               }
            }
         }

         // Same ? It's ok then
         //if (false/*ok*/)
         if (ok)
         {
            // Copy this bit
            if (sizeOfTrack != 0)
            {
               side_[side].tracks[track].bitfield[index_write] = val;
            }
            index_write++;
            INC_INDEXEX
         }
         else

            // WEAK SPOTTED §
            // What to do :
            // - Check when we're sure the weak sector ended (next sync - check bits back !)
            // - Now we have a section which begin and end with weak bits......
            // Check for 8 bits that are correct in all revolutions

         {
            // Not the same : We may expect end of weak sector.
            // Otherwise...It should be shitty until the next synchronisation (IE next SYNC bytes)
            // From next sync to current place, compare byte
            unsigned int index_next_sync [16];
            memset(index_next_sync, 0, sizeof(index_next_sync));

            // Prepare end byte
            for (unsigned int rev = 0; rev < nb_used_revolutions; rev++)
            {
               if (!revolution_wrecked[rev])
               {
                  index_next_sync[rev] = 0; // pFirstSyncOfTrack[rev];
                  unsigned int index_next_sector = 0;
                  while (index_next_sector < list_sizes[rev].size() && list_sizes[rev].at(index_next_sector) <= index_buffer[rev])
                  {
                     //pIndexNextSync[rev] = pListSizes[rev].at(indexNextSector);
                     index_next_sector++;
                  }
                  if (index_next_sector < list_sizes[rev].size())
                  {
                     index_next_sync[rev] = list_sizes[rev].at(index_next_sector);
                  }
                  else
                  {
                     // wtf ?? - Nothing valid here
                     if (rev < nb_used_revolutions - 1)
                        index_next_sync[rev] = side_[side].tracks[track].revolution[rev].size + first_sync_of_track[rev + 1
                        ] - first_sync_of_track[rev];
                     else
                        index_next_sync[rev] = side_[side].tracks[track].revolution[rev].size - first_sync_of_track[rev];
                  }

                  if (index_next_sync[rev] > side_[side].tracks[track].revolution[rev].size)
                     index_next_sync[rev] = side_[side].tracks[track].revolution[rev].size;
               }
            }

            // Check size. All same ? Just compare bit to bit !
            bool is_same_size = true;
            unsigned int weak_max_size = 0;
            unsigned int weak_min_size = side_[side].tracks[track].size - index_write;
            for (unsigned int rev = 0; rev < nb_used_revolutions; rev++)
            {
               if (!revolution_wrecked[rev])
               {
                  if (rev > 0 && index_buffer[rev - 1] != -1 && index_buffer[rev] != -1)
                  {
                     if (index_next_sync[rev - 1] - index_buffer[rev - 1] != index_next_sync[rev] - index_buffer[rev])
                        is_same_size = false;
                  }
                  if (index_buffer[rev] != -1)
                  {
                     if (index_next_sync[rev] - index_buffer[rev] > weak_max_size)
                     {
                        weak_max_size = index_next_sync[rev] - index_buffer[rev];
                     }
                     if (index_next_sync[rev] - index_buffer[rev] < weak_min_size)
                     {
                        weak_min_size = index_next_sync[rev] - index_buffer[rev];
                     }
                  }
               }
            }
            {
               // What to do :
               // - Check when we're sure the weak sector ended (next sync - check bits back !)
               bool reverse_ok;
               unsigned int nb_bits_ok = 0;

               int ok_count = 0;
               first_revolution_available = -1;
               for (unsigned int rev = 0; rev < nb_used_revolutions; rev++)
               {
                  if (!revolution_wrecked[rev])
                  {
                     ok_count++;
                     if (first_revolution_available == -1)
                        first_revolution_available = rev;
                  }
               }
               reverse_ok = (ok_count > 1);

               while (reverse_ok)
               {
                  val = side_[side].tracks[track].revolution[first_revolution_available].bitfield[index_next_sync[
                     first_revolution_available] - nb_bits_ok - 1];

                  for (unsigned int rev = 0; rev < nb_used_revolutions && reverse_ok; rev++)
                  {
                     if (rev != first_revolution_available && !revolution_wrecked[rev])
                     {
                        if (((index_next_sync[rev] - nb_bits_ok - 1) <= index_buffer[rev])
                           || (val != side_[side].tracks[track].revolution[rev].bitfield[index_next_sync[rev] - nb_bits_ok
                              - 1]))
                        {
                           // Different
                           reverse_ok = false;
                        }
                     }
                  }

                  if (reverse_ok)
                  {
                     nb_bits_ok++;
                  }
               }

               // - Now we have a section which begin and end with weak bits......
               // This is from pIndex[x] -> pIndexNextSync[x] - nbBitsOk.
               // Handle this : now : pIndex[x] -> pIndexNextSync[x] - nbBitsOk.
               // At the end of the section, every revolution  should be on the pIndexNextSync[x] - nbBitsOk index - So next control will return correct bits.
               /*if (nbBitsOk + indexWrite > side_[side].Tracks[track].Size)
               {
                  // This is strange... TODO : check this, as it shouldn't appear
                  nbBitsOk = side_[side].Tracks[track].Size - indexWrite;
               }*/

               if (nb_bits_ok <= weak_max_size)
               {
                  // if all sizes are not the same, there are "optionnal" bits, equal to maxsize - min size
                  if (nb_bits_ok > weak_min_size)
                  {
                     weak_min_size = nb_bits_ok;
                  }

                  // Check for 8 bits that are correct in all revolutions
                  unsigned int current_search_revolution[16];
                  unsigned int next_correct_index[16];
                  //#define RECOVER_SIZE 16

                  // Loop until
                  bool is_finished = false;

                  // Set index : Search must begin with this index.
                  for (unsigned int rev = 0; rev < nb_used_revolutions; rev++)
                  {
                     if (!revolution_wrecked[rev])
                     {
                        current_search_revolution[rev] = index_buffer[rev];
                        next_correct_index[rev] = index_next_sync[rev];
                        if (current_search_revolution[rev] == -1)
                           // Nothing to add
                           is_finished = true;
                     }
                  }

                  while (!is_finished)
                  {
                     // Find next correct state :
                     int correct_revolution = 0;
                     const int nb_correct_bits = FindEndOfWeakArea(side, track, current_search_revolution, next_correct_index,
                                                           nb_used_revolutions, &correct_revolution, revolution_wrecked);


                     // Compute number of weak bits : Min between pNextCorrectIndex-current_search_revolution,

                     unsigned int nb_weak_bits = next_correct_index[correct_revolution] - current_search_revolution[correct_revolution];
                     unsigned int nb_max_weak_bits = next_correct_index[correct_revolution] - current_search_revolution[correct_revolution];
                     for (unsigned int rev = 0; rev < nb_used_revolutions; rev++)
                     {
                        if (!revolution_wrecked[rev] && rev != correct_revolution)
                        {
                           // Beware ! If on last rev, and nbWeakBits = 0 : This means that last rev has reach end of his track : Consider it as wrecked.
                           if (next_correct_index[rev] - current_search_revolution[rev] == 0 && (rev == nb_used_revolutions - 1))
                           {
                              revolution_wrecked[rev] = true;
                           }
                           else
                           {
                              if (next_correct_index[rev] - current_search_revolution[rev] < nb_weak_bits)
                              {
                                 nb_weak_bits = next_correct_index[rev] - current_search_revolution[rev];
                              }
                              if (next_correct_index[rev] - current_search_revolution[rev] > nb_max_weak_bits)
                              {
                                 nb_max_weak_bits = next_correct_index[rev] - current_search_revolution[rev];
                              }
                           }
                        }
                     }
                     // Weak bits : nbWeakBits - nbCorrectBits
                     int nb_min_weak_bits = nb_weak_bits - nb_correct_bits;
                     if (nb_min_weak_bits < 0)
                     {
                        nb_min_weak_bits = 0;
                        LOG("nbCorrectBits > nbWeakBits - track :  ")
                        LOG(track)
                        LOGEOL
                     }

                     for (int c = 0; c < nb_min_weak_bits; c++)
                     {
                        if (sizeOfTrack != 0) side_[side].tracks[track].bitfield[index_write] = BIT_WEAK;
                        index_write++;
                     }
                     // Facultative bits : nbMaxWeakBits - nbWeakBits - nbCorrectBits
                     int nbFacBits = nb_max_weak_bits - nb_min_weak_bits - nb_correct_bits;
                     if (nbFacBits < 0)
                     {
                        LOG("nbCorrectBits > nbFacBits track :  ")
                        LOG(track)
                        LOGEOL
                     }

                     for (int c = 0; c < nbFacBits; c++)
                     {
                        if (sizeOfTrack != 0) side_[side].tracks[track].bitfield[index_write] = BIT_OPTIONAL | BIT_WEAK;
                        index_write ++;
                     }

                     // Then, copy correct bits.
                     for (int c = 0; c < nb_correct_bits; c++)
                     {
                        if (sizeOfTrack != 0)
                           side_[side].tracks[track].bitfield[index_write] = side_[side].tracks[
                              track].revolution[correct_revolution].bitfield[next_correct_index[correct_revolution] - nb_correct_bits + c];

                        index_write++;
                     }
                     // New values are from any pNextCorrectIndex.
                     for (unsigned int rev = 0; rev < nb_used_revolutions; rev++)
                     {
                        if (!revolution_wrecked[rev])
                        {
                           current_search_revolution[rev] = next_correct_index[rev];
                           next_correct_index[rev] = index_next_sync[rev];
                           if (current_search_revolution[rev] >= next_correct_index[rev])
                           {
                              current_search_revolution[rev] = next_correct_index[rev];
                              is_finished = true;
                           }
                        }
                     }
                     // Then, loop until all is found
                  }
               }

               // Now, all index are on the pIndexNextSync[rev] index
               for (unsigned int rev = 0; rev < nb_used_revolutions; rev++)
               {
                  if (!revolution_wrecked[rev] && index_buffer[rev] != -1)
                  {
                     index_buffer[rev] = index_next_sync[rev];
                     if (index_buffer[rev] >= side_[side].tracks[track].revolution[rev].size)
                     {
                        if (rev == nb_used_revolutions - 1)
                           revolution_wrecked[rev] = true;

                        index_buffer[rev] = -1;
                     }
                  }
               }
            }
         }
         end = true;
         // Finish it !
         for (unsigned int rev = 0; rev < nb_used_revolutions; rev++)
         {
            if (!revolution_wrecked[rev])
            {
               if (index_buffer[rev] != -1)
               {
                  end = false;
               }
            }
         }
      }

      // Now, move the bitfield a bit : Set the xxx bits from the end to the begining.
      if (sizeOfTrack != 0)
      {
         side_[side].tracks[track].size = index_write;

         if (sizeOfTrack != index_write)
         {
            //MessageBox(NULL, _T("Erreur Weak area detection "), _T("indexWrite != sizeOfTrack"), MB_OK);
            LOG("Erreur Weak area detection - track :  ")
            LOG(track)
            LOG("indexWrite:")
            LOG(index_write);
            LOG(" - sizeOfTrack:")
            LOG(sizeOfTrack);
            LOGEOL
         }


         // Replace now track on the correct place
         unsigned char* new_track_buffer = new unsigned char[side_[side].tracks[track].size ];
         memcpy(&new_track_buffer[first_sync_of_track[0]], side_[side].tracks[track].bitfield,
                side_[side].tracks[track].size - first_sync_of_track[0]);
         memcpy(new_track_buffer, &side_[side].tracks[track].bitfield[side_[side].tracks[track].size - first_sync_of_track[0]],
                first_sync_of_track[0]);
         delete[]side_[side].tracks[track].bitfield;
         side_[side].tracks[track].bitfield = new_track_buffer;

         // CHECK
         for (unsigned int i = 0; i < side_[side].tracks[track].size; i++)
         {
            if ((side_[side].tracks[track].bitfield[i] & (~0xD)) != 0)
            {
               LOG("Erreur Data type - track :  ")
               LOG(track)
               LOG(" - Problem with data : Addr ")
               LOG(i)
               LOG(" - Data = ")
               LOG(side_[side].tracks[track].bitfield[i] );
               LOGEOL
            }
         }
         for (unsigned int rev = 0; rev < side_[side].tracks[track].nb_revolutions; rev++)
         {
            delete [] tmp_buffer[rev];
         }
      }
      else
      {
         for (unsigned int rev = 0; rev < side_[side].tracks[track].nb_revolutions; rev++)
         {
            delete[]side_[side].tracks[track].revolution[rev].bitfield;
            side_[side].tracks[track].revolution[rev].bitfield = tmp_buffer[rev];
            side_[side].tracks[track].revolution[rev].size = size_buffer[rev];
         }
      }

      delete[]index_buffer;
   }
   else
   {
      for (unsigned int rev = 0; rev < side_[side].tracks[track].nb_revolutions; rev++)
      {
         delete [] tmp_buffer[rev];
      }
   }


   delete[]list_sizes;
   delete[]first_sync_of_track;
   delete[]size_after;
   return index_write;
}


bool IDisk::TrackFormatted(IDisk::MFMTrack* track_buffer)
{
   // If the pointer is null, it is not
   if (track_buffer == nullptr) return false;
   // If the size is 0, it is not
   if (track_buffer->size == 0) return false;

   // Search the track for any non-weak value
   Track track_info;
   GetTrackInfo(track_buffer, &track_info);
   return (track_info.list_sector_.size() > 0);
}

int IDisk::CompareTracks(IDisk::MFMTrack* track1, IDisk::MFMTrack* track2)
{
   int ret = 0;

   // Cas degradé :
   bool bT1Formatted = TrackFormatted(track1);
   bool bT2Formatted = TrackFormatted(track2);
   unsigned int index1 = 0, index2 = 0;

   if (bT1Formatted && bT2Formatted)
   {
      // Compare !
      bool is_previous_weak = (track1->bitfield[track1->size - 1] & BIT_WEAK) == BIT_WEAK;

      while (index1 < track1->size && index2 < track2->size && ret == 0)
      {
         unsigned char b1 = (index1 < track1->size) ? track1->bitfield[index1] : BIT_WEAK;
         unsigned char b2 = (index2 < track2->size) ? track2->bitfield[index2] : BIT_WEAK;

         // Weak bits ?
         if (((b1 | b2) & BIT_WEAK) == BIT_WEAK)
         {
            // count for total weak : weak + optionnal
            int count1 = 0;
            int count2 = 0;
            int opt1 = 0;
            int opt2 = 0;
            unsigned int index_search_weak = index1;
            while (index_search_weak < track1->size && ((track1->bitfield[index_search_weak] & (BIT_WEAK | BIT_OPTIONAL)) !=
               0))
            {
               if ((track1->bitfield[index_search_weak] & BIT_OPTIONAL) == BIT_OPTIONAL) opt1++;
               else if ((track1->bitfield[index_search_weak] & BIT_WEAK) == BIT_WEAK) count1++;
               index_search_weak++;
            }

            index_search_weak = index2;
            while (index_search_weak < track2->size && ((track2->bitfield[index_search_weak] & (BIT_WEAK | BIT_OPTIONAL)) !=
               0))
            {
               if ((track2->bitfield[index_search_weak] & BIT_OPTIONAL) == BIT_OPTIONAL) opt2++;
               else if ((track2->bitfield[index_search_weak] & BIT_WEAK) == BIT_WEAK) count2++;
               index_search_weak++;
            }

            // Compare if it's correct :
            // count1 should be between count2 and count2+opt2
            // count2 should be between count1 and count1+opt1
            if ((count1 + opt1 >= count2 && count1 <= count2 + opt2)
               && (count2 + opt2 >= count1 && count2 <= count1 + opt1)
            )
            {
               // yes ? Set the indexes on the next non-weak pattern
               index1 += count1 + opt1;
               index2 += count2 + opt2;
            }
            else
            {
               // no : Error !
#ifndef __circle__
               char str[1024];
               sprintf(str, "ERROR : Inde1 = %x; Index2 = %x", index1, index2);

               //MessageBox (NULL, str, _T("Compare error"), MB_OK);

               sprintf(str, "COMPARE TRACK 1, addr 7FF : Orig = %x %x %x ; Convert = %x %x %x",
                       track1->bitfield[0x7FE], track1->bitfield[0x7FF], track1->bitfield[0x800],
                       track2->bitfield[0x7FE], track2->bitfield[0x7FF], track2->bitfield[0x800]
               );
#endif

               return -1;
            }
            is_previous_weak = true;
         }
            // Otherwise
         else
         {
            if (b1 != b2)
            {
               // Special case ! Do not apply this if the previous bit is weak !
               if (!is_previous_weak)
               {
#ifndef __circle__
                  char str[1024];
                  sprintf(str, "ERROR : Inde1 = %x; Index2 = %x", index1, index2);
                  //MessageBox (NULL, str, _T("Compare error"), MB_OK);

                  sprintf(str, "COMPARE TRACK 1, addr 7FF : Orig = %x %x %x ; Convert = %x %x %x",
                          track1->bitfield[0x7FE], track1->bitfield[0x7FF], track1->bitfield[0x800],
                          track2->bitfield[0x7FE], track2->bitfield[0x7FF], track2->bitfield[0x800]
                  );

                  //MessageBox (NULL, str, _T("Compare error"), MB_OK);
#endif
                  return -1;
               }
            }
            index1++;
            index2++;
            is_previous_weak = false;
         }
      }
   }
   else
   {
      ret = (bT1Formatted == false && bT2Formatted == false) ? 0 : -1;
   }

   return ret;
}


int IDisk::CompareToDisk(IDisk* other_disk, bool exact)
{
   int idem = 0;

   int max_side = std::max(nb_sides_, other_disk->nb_sides_);

   for (int side = 0; side < max_side && idem == 0; side ++)
   {
      // For each side :
      if (exact && (side_[side].nb_tracks != other_disk->side_[side].nb_tracks))
      {
         return -1;
      }

      unsigned int maxTrack = std::max(side_[side].nb_tracks, other_disk->side_[side].nb_tracks);
      for (unsigned int track = 0; track < maxTrack && idem == 0; track++)
      {
         // For each track :
         // EXACT comparison ?
         if (exact)
         {
            if (side_[side].tracks[track].size != other_disk->side_[side].tracks[track].size)
            {
               return - 1;
            }
            if (side_[side].tracks[track].bitfield != nullptr && other_disk->side_[side].tracks[track].bitfield !=
               nullptr)
            {
               if (memcmp(side_[side].tracks[track].bitfield, other_disk->side_[side].tracks[track].bitfield,
                          side_[side].tracks[track].size) != 0)
               {
                  idem = -1;
               }
            }
            else
            {
               // ok if both are null
               return (side_[side].tracks[track].bitfield == other_disk->side_[side].tracks[track].bitfield) ? 0 : -1;
            }
         }
         else
         {
            // Otherwise...
            idem = CompareTracks((side_[side].nb_tracks > track) ? (&side_[side].tracks[track]) : nullptr,
                                 (other_disk->side_[side].nb_tracks > track)
                                    ? (&other_disk->side_[side].tracks[track])
                                    : nullptr);
            if (idem != 0)
            {
            }
         }
      }
   }

   return idem;
}

int IDisk::SmartOpen(FILE** file, const char* file_path, const char* file_ext)
{
   int res;

#ifdef _WIN32
   if (file_ext != nullptr && stricmp(file_path + strlen(file_path) - strlen(file_ext), file_ext) != 0)
#else
   if (file_ext != NULL && strcmp(file_path + strlen(file_path) - strlen(file_ext), file_ext) != 0)
#endif
   {
      // Add extension
      char full_path[MAX_PATH];
      strcpy(full_path, file_path);
      strcat(full_path, file_ext);
      res = fopen_s(file, full_path, "wb");
      if (res == 0)
      {
         current_disk_path_ = full_path;
      }
   }
   else
   {
      res = fopen_s(file, file_path, "wb");
      if (res == 0)
      {
         current_disk_path_ = file_path;
      }
   }
   return res;
}

void IDisk::SetName(const char* new_filepath)
{
#ifdef __MORPHOS__
   if(PathPart(new_filepath) == new_filepath)
   {
      char curDirName[MAX_PATH];

      if(GetCurrentDirName(curDirName, sizeof(curDirName)))
      {
         current_disk_path_  = curDirName;
         current_disk_path_ += '/';
         current_disk_path_ += new_filepath;
      }
   }
#else 
#if !defined(RASPPI) && !defined(TEST_VECTOR)
   fs::path path(new_filepath);

   if (!path.has_root_path())
   {
      path = fs::current_path() / path;
      current_disk_path_ = path.filename().string();
   }
#else
   if (false)
   {
      // no implementation for PI
   }
#endif
#endif
   else
   {
      current_disk_path_ = new_filepath;
   }
}

void IDisk::FillTrack(unsigned char* track_buffer, const unsigned char cylinder, IDisk::DiskType type,
                      unsigned int sizeOftrack)
{
   int j;
   int load_disk_index = 0;

   unsigned char* chrns;
   switch (type)
   {
   case VENDOR:
      chrns = VendorTrack;
      break;
   case DATA:
   default:
      chrns = DataTrack;
      break;
   }

   // Fill the track :
   //  GAP 4a : 80 octets � #4E
   for (j = 0; j < 80; j++) load_disk_index = AddByteToTrack(track_buffer, load_disk_index, 0x4E);

   //  Sync : 12 octets � #00
   for (j = 0; j < 12; j++) load_disk_index = AddByteToTrack(track_buffer, load_disk_index, 0x00);

   //  IAM : 3 octets � #C2 + 1 octet � #FC
   load_disk_index = AddSyncByteToTrack(track_buffer, load_disk_index, 0xC2);
   load_disk_index = AddSyncByteToTrack(track_buffer, load_disk_index, 0xC2);
   load_disk_index = AddSyncByteToTrack(track_buffer, load_disk_index, 0xC2);
   load_disk_index = AddByteToTrack(track_buffer, load_disk_index, 0xFC);

   // GAP 1 : 50 byte with 0x4E value
   for (j = 0; j < 50; j++) load_disk_index = AddByteToTrack(track_buffer, load_disk_index, 0x4E);

   // Then, each sectors :
   for (unsigned int s = 0; s < 9; s++)
   {
      CRC crc;
      // - Sync : 12 byte, value #00
      for (j = 0; j < 12; j++) load_disk_index = IDisk::AddByteWithCrc(track_buffer, load_disk_index, 0x00, crc, false);

      crc.Reset();
      // - IDAM : 3 byte #A1 + 1 byte #FE
      load_disk_index = IDisk::AddByteWithCrc(track_buffer, load_disk_index, 0xA1, crc, true);
      load_disk_index = IDisk::AddByteWithCrc(track_buffer, load_disk_index, 0xA1, crc, true);
      load_disk_index = IDisk::AddByteWithCrc(track_buffer, load_disk_index, 0xA1, crc, true);
      load_disk_index = IDisk::AddByteWithCrc(track_buffer, load_disk_index, 0xFE, crc);

      // Index
      // - C (identification secteur 'C' = 1 byte)
      load_disk_index = IDisk::AddByteWithCrc(track_buffer, load_disk_index, cylinder, crc);
      // - H (identification secteur 'H' = 1 byte)
      load_disk_index = IDisk::AddByteWithCrc(track_buffer, load_disk_index, 0, crc);
      // - R (identification secteur 'R' = 1 byte)
      load_disk_index = IDisk::AddByteWithCrc(track_buffer, load_disk_index, chrns[s], crc);
      // - N (identification secteur 'N' = 1 byte)
      load_disk_index = IDisk::AddByteWithCrc(track_buffer, load_disk_index, 2, crc);

      unsigned short computed_crc = crc.GetCRC();
      load_disk_index = IDisk::AddByteWithCrc(track_buffer, load_disk_index, (computed_crc >> 8), crc);
      load_disk_index = IDisk::AddByteWithCrc(track_buffer, load_disk_index, (computed_crc & 0xFF), crc);

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
      load_disk_index = IDisk::AddByteWithCrc(track_buffer, load_disk_index, DAAM_OFF, crc);

      // With extension, CRC and GAP#3 are in the floppy
      for (unsigned int k = 0; k < 0x200; k++)
      {
         load_disk_index = IDisk::AddByteWithCrc(track_buffer, load_disk_index, 0xE5, crc);
      }

      // - Crc (2 bytes) - If needed....
      computed_crc = crc.GetCRC();
      load_disk_index = IDisk::AddByteWithoutCrc(track_buffer, load_disk_index, (computed_crc >> 8));
      load_disk_index = IDisk::AddByteWithoutCrc(track_buffer, load_disk_index, (computed_crc & 0xFF));

      // - GAP #3 (x bytes)
      for (j = 0; j < 78; j++)
      {
         load_disk_index = IDisk::AddByteWithoutCrc(track_buffer, load_disk_index, 0x4E);
      }
   }
   // End of track
   for (; load_disk_index < 6250 * 16 - 15;)
   {
      load_disk_index = IDisk::AddByteWithoutCrc(track_buffer, load_disk_index, 0x4E);
   }

   for (; load_disk_index < static_cast<int>(sizeOftrack); load_disk_index++)
   {
      track_buffer[load_disk_index] = BIT_WEAK;
   }
}

void IDisk::CombineWithDisk(IDisk* other_disk)
{
   CleanSide(1);

   nb_sides_ = 2;
   side_[1] = other_disk->side_[0];
   other_disk->side_[0].nb_tracks = 0;
   other_disk->side_[0].tracks = nullptr;
}


bool ValidName(std::string str)
{
   if (str.length() == 0) return false;

   bool valid = true;
   // no not displayable or lower case character
   if (str.length() >= 9) str[8] &= 0x7F;
   if (str.length() >= 10) str[9] &= 0x7F;
   for (unsigned int i = 0; i < str.length() && valid; i++)
   {
      if (str[i] > 0x60 || str[i] < 0x20) valid = false;
   }
   return valid;
}


//IDisk::AutorunType DiskGen::GetAutorun(char* buffer, unsigned int size_of_buffer)
std::vector<std::string>  IDisk::GetCat(IDisk::AutorunType& autorun_type, int user)
{
   std::vector<std::string> filename_vector;
   autorun_type = IDisk::AUTO_UNKNOWN;

   // Get info
   IDisk::Track track_info;
   GetTrackInfo(0, 0, &track_info);

   if (track_info.list_sector_.size() == 0)
   {
      autorun_type = IDisk::AUTO_UNKNOWN;
      return filename_vector;
   }

   // CPM ?
   int index_cat = 0;
   unsigned char first_sector = 00;
   // A |CPM can be run if a"0x41" sector is found
   for (auto it = track_info.list_sector_.begin(); it != track_info.list_sector_.end(); ++it)
   {
      if (it->r == 0x41)
      {
         index_cat = 2;
      }
   }

   // SYSTEM / VENDOR
   if ((track_info.list_sector_.at(0).r & 0xC0) == 0x40)
   {
      first_sector = 0x41;
      autorun_type = IDisk::AUTO_CPM;
   }
   // DATA
   else if ((track_info.list_sector_.at(0).r & 0xC0) == 0xC0)
   {
      index_cat = 0;
      first_sector = 0xC1;
   }
   // IBM
   else if ((track_info.list_sector_.at(0).r & 0xC0) == 0x00
      || (track_info.list_sector_.at(0).r & 0xC0) == 0x80)
   {
      index_cat = 1;
      first_sector = 0x01;
   }

   if (index_cat != 0)
   {
      track_info.list_sector_.clear();
      GetTrackInfo(0, index_cat, &track_info);
   }

#define NB_SECTOR_FOR_CAT 4

   int total_size = NB_SECTOR_FOR_CAT * 512;
   /*for (int i = 0; i < NB_SECTOR_FOR_CAT && i < trackInfo.listSector.size(); i++)
   {
      totalSize += trackInfo.listSector.at(i).RealSize;
   }*/

   // No, try to fins appropriate file to run
   unsigned char* track_data = new unsigned char[total_size];
   memset(track_data, 0, total_size);
   int offset_data = 0;


   // It's in first track, NB_SECTOR_FOR_CAT firsts sectors, from first_sector to first_sector + NB_SECTOR_FOR_CAT
   // todo : read the proper sectors !
   int nb_sector_read = 0;

   //for (int i = 0; i < NB_SECTOR_FOR_CAT && i < trackInfo.listSector.size(); i++)
   unsigned int index_sector = 0;
   int nb_read_sector = 0;
   while (index_sector < track_info.list_sector_.size() && nb_read_sector < NB_SECTOR_FOR_CAT && offset_data < total_size)
   {
      // Read first xx sectors
      if (track_info.list_sector_.at(index_sector).r >= first_sector)
      {
         if (track_info.list_sector_.at(index_sector).r - first_sector < NB_SECTOR_FOR_CAT)
         {
            int index = track_info.list_sector_.at(index_sector).dam_offset + 64;

            offset_data = (track_info.list_sector_.at(index_sector).r - first_sector) * 512;
            nb_read_sector++;
            for (int cpt = 0; cpt < track_info.list_sector_.at(index_sector).real_size && offset_data < total_size; cpt++)
            {
               // Walk all of them - remove the 0x80 bits
               unsigned char b = GetNextByte(0, index_cat, index);
               track_data[offset_data++] = b;
               index += 16;
            }
         }
      }
      index_sector++;
   }

   // Dump TrackData
   int line_count = 0;
   for (int hex = 0; hex < offset_data; hex++)
   {
      LOGB(track_data[hex]);
      LOG(" ");
      if (++line_count == 16)
      {
         LOGEOL
            line_count = 0;
      }
   }

   int offset = 1;
   bool end = false;

   while (offset < total_size && !end)
   {
      if (user == -1 || track_data[offset - 1] == user)
      {
         // Add to the name vector
         std::string str = (const char*)(&track_data[offset]);
         if (ValidName(str))
         {
            filename_vector.push_back(str);
         }
      }
      offset += 32;
   }
   delete[]track_data;

   return filename_vector;
}
