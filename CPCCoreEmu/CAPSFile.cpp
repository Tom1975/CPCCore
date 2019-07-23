#include "stdafx.h"
#include "CAPSFile.h"

#include "CRC.h"
#include "FDC765.h"


unsigned int SwapInt(unsigned int dword)
{
   unsigned int tmp = (dword >> 24)
      + (((dword >> 16) & 0xFF) << 8)
      + (((dword >> 8) & 0xFF) << 16)
      + (((dword) & 0xFF) << 24);
   return tmp;
}

int Swap(unsigned char* buffer, unsigned int size)
{
   // Swap every unsigned dword in this buffer
   unsigned char buf[4];
   for (unsigned int i = 0; i < size; i += 4)
   {
      memcpy(buf, &buffer[i], 4);
      buffer[i] = buf[3];
      buffer[i + 1] = buf[2];
      buffer[i + 2] = buf[1];
      buffer[i + 3] = buf[0];
   }
   return 0;
}

CAPSFile::CAPSFile(): encoder_type_(0)
{
}


CAPSFile::~CAPSFile()
{
   CAPSFile::Clear();
}

void CAPSFile::Clear()
{
   for (std::vector<DataChunks>::iterator it = list_data_.begin(); it != list_data_.end(); it++)
   {
      delete []it->buffer;
      it->buffer = NULL;
   }
   list_data_.clear();
   list_imge_.clear();
   list_tracks_.clear();
}

unsigned short ExtractShort(const unsigned char* in)
{
   unsigned short return_value;
#ifdef BIG_ENDIAN
   return_value = in[1]
      + (in[0] << 8);
#else
   return_value = in[0]
      + (in[1] << 8);

#endif

   return return_value;
}

unsigned int ExtractInt(const unsigned char* in)
{
   unsigned int return_value;
#ifdef BIG_ENDIAN
   return_value = in[3]
      + (in[2] << 8)
      + (in[1] << 16)
      + (in[0] << 24);
#else
   return_value = in[0]
      + (in[1] << 8)
      + (in[2] << 16)
      + (in[3] << 24);

#endif

   return return_value;
}


bool CAPSFile::ReadRecordHeader(const unsigned char* buffer, RecordHeader* header_out, size_t size)
{
   memcpy(header_out->type, &buffer[0], 4);
   header_out->length = ExtractInt(&buffer[4]);
   header_out->crc = ExtractInt(&buffer[8]);

   // Copy buffer to check
   if (header_out->length > size) return false;

   unsigned char* buffer_to_check = new unsigned char[header_out->length];
   memcpy(buffer_to_check, buffer, header_out->length);
   // Check the CRC
   memset(&buffer_to_check[8], 0, 4);

   unsigned int crc = CRC::ComputeCrc32(0xEDB88320, buffer_to_check, header_out->length);
   delete[] buffer_to_check;

   return (crc == header_out->crc);
}

bool CAPSFile::GetStreamGapItem(CAPSFile::StreamItem* item, unsigned char* buffer, unsigned int& offset,
                                unsigned int size_of_buffer)
{
   // Next stream item
   unsigned char b = buffer[offset++];

   if (b == 0) return false;

   memset(item, 0, sizeof(StreamItem));
   while (b != 0 && offset < size_of_buffer)
   {
      int size = b >> 5;
      int type = b & 0x1F;
      int gap_size = 0;
      for (int j = 0; j < size; j++)
      {
         gap_size += (buffer[offset++] << (8 * (size - (j + 1))));
      }

      if (type == 1)
      {
         item->size = gap_size;
      }
      else if (type == 2)
      {
         item->sample = buffer[offset++];
         item->sample_size = gap_size;
      }
      else return false;

      b = buffer[offset++];
   }
   return true;
}

int CAPSFile::ReadBuffer(const unsigned char* buffer, unsigned int size_of_buffer, IDisk* disk,
                         ILoadingProgress* loading_progress)
{
   // While there is something to read
   RecordHeader rec_header;
   const unsigned char* block;
   int block_size;
   unsigned int gen_index = 0;

   list_imge_.clear();

   while (gen_index < size_of_buffer)
   {
      if (!ReadRecordHeader(&buffer[gen_index], &rec_header, (size_of_buffer - gen_index)))
      {
         // CRC ERROR !
         Clear();
         return -1;
      }
      gen_index += 12;

      // Block size
      block_size = rec_header.length - 12;;
      block = &buffer[gen_index];
      gen_index += block_size;

      // Next record header
      if (memcmp(rec_header.type, "CAPS", 4) == 0)
      {
         // Nothing interesting here
      }
      else if (memcmp(rec_header.type, "DUMP", 4) == 0)
      {
      }
      else if (memcmp(rec_header.type, "PACK", 4) == 0)
      {
         int offset = 0;
         unsigned int csize = ExtractInt(&block[offset]);; // compressed size in bytes
         offset += 4;
         unsigned int ccrc = ExtractInt(&block[offset]);; // CRC on compressed data
         offset += 4;
         unsigned int hcrc = ExtractInt(&block[offset]);; // CRC on header calculated as hcrc=0
         offset += 4;

         // Read compressed size ?
         if (csize + 8 != rec_header.length)
         {
            Clear();
            return -1;
         }
      }
      else if (memcmp(rec_header.type, "TRCK", 4) == 0)
      {
         //
         Trackchunk item;
         int offset = 0;
         item.ctype = ExtractInt(&block[offset]);; // type of track
         offset += 4;
         item.cyl = ExtractInt(&block[offset]);; // cylinder
         offset += 4;
         item.head = ExtractInt(&block[offset]);; // head
         offset += 4;
         item.did = ExtractInt(&block[offset]);; // id
         offset += 4;

         list_tracks_.push_back(item);
      }
      else if (memcmp(rec_header.type, "DATA", 4) == 0)
      {
         int offset = 0;
         DataChunks data_item;

         // Length
         data_item.size = ExtractInt(&block[offset]);
         offset += 4;
         // Bitsize
         data_item.bit_size = ExtractInt(&block[offset]);
         offset += 4;
         // CRC
         data_item.data_crc = ExtractInt(&block[offset]);
         offset += 4;
         // datakey
         data_item.data_id = ExtractInt(&block[offset]);
         offset += 4;

         data_item.buffer = new unsigned char[data_item.size];
         memcpy(data_item.buffer, &block[offset], data_item.size);

         gen_index += data_item.size;
         // Save it
         list_data_.push_back(data_item);

         if (list_imge_.size() > 0)
         {
            unsigned int crc = CRC::ComputeCrc32(0xEDB88320, &block[offset], data_item.size);
            if (data_item.data_crc != crc)
            {
               // CRC ERROR : TODO
               Clear();

               return -1;
            }


            DecodeData(data_item, disk);
         }
      }
      else if (memcmp(rec_header.type, "INFO", 4) == 0)
      {
         unsigned int side_min, side_max;
         unsigned int track_min, track_max;
         // check size !
         if (block_size != 84) return -1;

         int offset = 0;
         // Decode Info header :
         // Media type
         ExtractInt(&block[offset]);
         offset += 4;
         // Encoder type
         encoder_type_ = ExtractInt(&block[offset]);
         offset += 4;
         // Encoder rev
         ExtractInt(&block[offset]);
         offset += 4;
         // File key
         ExtractInt(&block[offset]);
         offset += 4;
         // File rev
         ExtractInt(&block[offset]);
         offset += 4;
         // origin CRC32
         ExtractInt(&block[offset]);
         offset += 4;
         // Min track
         track_min = ExtractInt(&block[offset]);
         offset += 4;
         // Max track
         track_max = ExtractInt(&block[offset]);
         offset += 4;
         // Min side
         side_min = ExtractInt(&block[offset]);
         offset += 4;
         // Max side
         side_max = ExtractInt(&block[offset]);
         offset += 4;
         // Creation date
         ExtractInt(&block[offset]);
         offset += 4;
         // Creation time
         ExtractInt(&block[offset]);
         offset += 4;
         // Platform
         ExtractInt(&block[offset]);
         offset += 4;
         ExtractInt(&block[offset]);
         offset += 4;
         ExtractInt(&block[offset]);
         offset += 4;
         ExtractInt(&block[offset]);
         offset += 4;

         // Disk number
         ExtractInt(&block[offset]);
         offset += 4;
         // creator ID
         ExtractInt(&block[offset]);
         offset += 4;
         // reserved -
         offset += 12;

         disk->nb_sides_ = side_max - side_min + 1;
         for (int side = 0; side < disk->nb_sides_; side++)
         {
            disk->side_[side].nb_tracks = track_max - track_min + 1;
            disk->side_[side].tracks = new IDisk::MFMTrack[disk->side_[side].nb_tracks];
            for (unsigned int tr = 0; tr < disk->side_[side].nb_tracks; tr++)
            {
               disk->side_[side].tracks[tr].size = 0;
               disk->side_[side].tracks[tr].bitfield = nullptr;
               disk->side_[side].tracks[tr].nb_revolutions = 0;
               disk->side_[side].tracks[tr].revolution = nullptr;
            }
         }
      }
      else if (memcmp(rec_header.type, "IMGE", 4) == 0)
      {
         int offset = 0;
         ImgeItem item;

         // Track number
         item.track_number = ExtractInt(&block[offset]);
         offset += 4;
         item.side = ExtractInt(&block[offset]);
         offset += 4;

         if (item.side >= disk->nb_sides_ || item.track_number >= static_cast<int>(disk->side_[item.side].nb_tracks))
         {
            Clear();
            return -1;
         }

         // density : Not used for amstrad
         item.density = ExtractInt(&block[offset]);
         offset += 4;
         // signal Type : Should be 2us cell (1)
         item.signal_type = ExtractInt(&block[offset]);
         offset += 4;
         // trackBytes
         item.track_byte = ExtractInt(&block[offset]);
         offset += 4;

         // startbytepos : useless
         offset += 4;
         // startbitpos
         item.start_bit_pos = ExtractInt(&block[offset]);
         offset += 4;
         // Databits
         item.data_bits = ExtractInt(&block[offset]);
         offset += 4;
         // gapbits
         item.gap_bits = ExtractInt(&block[offset]);
         offset += 4;
         //trackbits
         item.track_bits = ExtractInt(&block[offset]);
         offset += 4;
         // blockcount
         item.block_count = ExtractInt(&block[offset]);
         offset += 4;
         // encoderprocess
         item.encoder_process = ExtractInt(&block[offset]);
         offset += 4;
         // trackflags
         item.track_flags = ExtractInt(&block[offset]);
         offset += 4;
         // data keys
         item.data_key = ExtractInt(&block[offset]);
         offset += 4;
         // reserved
         offset += 12;

         disk->side_[item.side].tracks[item.track_number].nb_revolutions = 0;
         disk->side_[item.side].tracks[item.track_number].revolution = 0;

         disk->side_[item.side].tracks[item.track_number].bitfield = new unsigned char[item.track_bits];
         disk->side_[item.side].tracks[item.track_number].size = item.track_bits;
         memset(disk->side_[item.side].tracks[item.track_number].bitfield, 0xFF,
                disk->side_[item.side].tracks[item.track_number].size);

         // Add IMGE bloc
         list_imge_.push_back(item);
      }
      else
      {
         // Not handled
         Clear();
         return -1;
      }
   }

   // CT-RAW ?
   if (list_tracks_.size() > 0)
   {
      // Count number of tracks and heads
      unsigned int max_side = 0;
      unsigned int max_track = 0;
      for (std::vector<Trackchunk>::iterator trackit = list_tracks_.begin(); trackit != list_tracks_.end(); trackit++)
      {
         if (trackit->cyl > max_track)
            max_track = trackit->cyl;
         if (trackit->head > max_side)
            max_side = trackit->head;
      }

      disk->nb_sides_ = max_side + 1;
      for (int side = 0; side < disk->nb_sides_; side++)
      {
         disk->side_[side].nb_tracks = max_track + 1;
         disk->side_[side].tracks = new IDisk::MFMTrack[disk->side_[side].nb_tracks];

         for (unsigned int tr = 0; tr < disk->side_[side].nb_tracks; tr++)
         {
            disk->side_[side].tracks[tr].nb_revolutions = 1;
            disk->side_[side].tracks[tr].revolution = new IDisk::Revolution[1];

            // Each track : 9 sector
            disk->side_[side].tracks[tr].revolution[0].size = 6250 * 16;
            disk->side_[side].tracks[tr].revolution[0].bitfield = new unsigned char[disk->side_[side].tracks[tr].
               revolution[0].size];
            memset(disk->side_[side].tracks[tr].revolution[0].bitfield, BIT_WEAK,
                   disk->side_[side].tracks[tr].revolution[0].size);
         }
      }

      int total = 0;
      for (std::vector<Trackchunk>::iterator trackit = list_tracks_.begin(); trackit != list_tracks_.end(); trackit++)
      {
         // Decompress data !
         // Search associated datas to decode
         bool found = false;
         DataChunks image_item;
         for (std::vector<DataChunks>::iterator it = list_data_.begin(); it != list_data_.end() && (!found); it++)
         {
            if (it->data_id == trackit->did)
            {
               // Found !
               image_item = *it;
               found = true;
            }
         }
         if (found)
         {
            // Do something !
            DecompressData(disk, trackit->ctype, trackit->cyl, trackit->head, image_item);
            disk->CreateSingleTrackFromMultiRevolutions(trackit->head, trackit->cyl);

            IDisk::Track track_info;
            disk->GetTrackInfo(trackit->head, trackit->cyl, &track_info);
         }
         if (loading_progress != nullptr)
         {
            total++;
            loading_progress->SetProgress(total * 100 / list_tracks_.size());
         }
      }
   }

   // Clear side (if side 2 is empty, remove it)
   if (disk->nb_sides_ == 2)
   {
      bool do_not_remove = false;
      for (unsigned int tr = 0; tr < disk->side_[1].nb_tracks && (!do_not_remove); tr++)
      {
         // CTR ?
         if ((disk->side_[1].tracks[tr].revolution != NULL && disk->side_[1].tracks[tr].revolution[0].size != 0)
            || (disk->side_[1].tracks[tr].revolution == NULL && disk->side_[1].tracks[tr].size != 0)
         )
         {
            do_not_remove = true;
         }
      }

      if (!do_not_remove)
      {
         for (unsigned int tr = 0; tr < disk->side_[1].nb_tracks; tr++)
         {
            if (disk->side_[1].tracks[tr].revolution != NULL)
               delete[]disk->side_[1].tracks[tr].revolution;
         }
         delete[]disk->side_[1].tracks;
         disk->nb_sides_ = 1;
      }
   }

   // Clear all
   Clear();

   return 0;
}

bool CAPSFile::DecodeData(DataChunks item, IDisk* disk)
{
   unsigned int offset = 0;
   unsigned char* block = item.buffer;
   unsigned char* data_block = &block[offset];

   int total_total = 0;
   // Now, extra data blocks ?
   if (item.size != 0)
   {
      // Get block count from IMGE
      ImgeItem imge_item;
      bool found = false;
      for (std::vector<ImgeItem>::iterator it = list_imge_.begin(); it != list_imge_.end() && (!found); it++)
      {
         if (it->data_key == item.data_id)
         {
            // Found !
            imge_item = *it;
            found = true;
         }
      }
      if (!found || (imge_item.block_count * 32 > item.size))
      {
         // Error !
         return false;
         // TODO : Save this chunk for later use ?
         //break;
      }
      else
      {
         int total_data = 0;

         unsigned int index_track = imge_item.start_bit_pos;
         for (DWORD i = 0; i < imge_item.block_count; i++)
         {
            // Read descriptors
            int data_bits = ExtractInt(&block[offset]);
            offset += 4;
            unsigned int gap_bits = ExtractInt(&block[offset]);
            offset += 4;
            int data_bytes = 0;
            int gap_bytes = 0;
            unsigned int gap_offset = 0;
            int cell_type = 0;
            if (encoder_type_ == 1)
            {
               data_bytes = ExtractInt(&block[offset]);
               offset += 4;
               gap_bytes = ExtractInt(&block[offset]);
               offset += 4;
            }
            else
            {
               gap_offset = ExtractInt(&block[offset]);
               offset += 4;
               cell_type = ExtractInt(&block[offset]);
               offset += 4;
            }
            int encoder_type = ExtractInt(&block[offset]);
            offset += 4;
            int block_flags = ExtractInt(&block[offset]);
            offset += 4;
            int gap_default = ExtractInt(&block[offset]);
            offset += 4;
            int data_offset = ExtractInt(&block[offset]);
            offset += 4;

            // Now, remaining datas
            unsigned char* current_block_gap = &data_block[gap_offset];
            unsigned char* current_block_data = &data_block[data_offset];
            data_offset = 0;
            gap_offset = 0;
            // Do we have backward gap ?
            // Read Forward / Backward streams
            StreamItem backward_stream = {0};
            StreamItem forward_stream = {0};

            unsigned int total_gap = 0;
            bool forward_gap = false;
            bool backward_gap = false;
            if ((block_flags & 0x1) == 0x1)
            {
               // Ok, Forward stream
               forward_gap = GetStreamGapItem(&forward_stream, current_block_gap, gap_offset, item.size);
            }
            if ((block_flags & 0x2) == 0x2)
            {
               // Ok, backward stream
               backward_gap = GetStreamGapItem(&backward_stream, current_block_gap, gap_offset, item.size);
            }

            // data
            unsigned char b = current_block_data[data_offset++];
            unsigned int offset_before = index_track;
            while (b != 0)
            {
               int size_width = b >> 5;
               int data_type = b & 0x1F;
               int data_size = 0;
               for (int j = 0; j < size_width; j++)
               {
                  data_size += (current_block_data[data_offset++] << (8 * (size_width - (j + 1))));
               }
               if ((block_flags & 0x4) == 0x4)
               {
                  // in bits
                  while (data_size != 0)
                  {
                     if (data_type == 5)
                     {
                        for (int c = 0; c < data_size; c++)
                        {
                           disk->side_[imge_item.side].tracks[imge_item.track_number].bitfield[index_track++] =
                              BIT_WEAK;
                           if (index_track >= disk->side_[imge_item.side].tracks[imge_item.track_number].size)
                              index_track = 0;
                        }
                        data_size = 0;
                     }
                     else
                     {
                        unsigned char bitfield = current_block_data[data_offset++];
                        if (data_type == 1)
                        {
                           if (data_size > 8)
                           {
                              int maxbits = 8;
                              for (int d = 0; d < 8; d++)
                              {
                                 maxbits--;
                                 int val = (bitfield >> maxbits) & 0x1;
                                 disk->side_[imge_item.side].tracks[imge_item.track_number].bitfield[index_track++] =
                                    val;
                                 if (index_track >= disk->side_[imge_item.side].tracks[imge_item.track_number].size)
                                    index_track = 0;
                              }
                              data_size -= 8;
                           }
                           else
                           {
                              int maxbits = 8;
                              for (int d = 0; d < data_size; d++)
                              {
                                 maxbits--;
                                 int val = (bitfield >> maxbits) & 0x1;
                                 disk->side_[imge_item.side].tracks[imge_item.track_number].bitfield[index_track++] =
                                    val;
                                 if (index_track >= disk->side_[imge_item.side].tracks[imge_item.track_number].size)
                                    index_track = 0;
                              }
                              data_size = 0;
                           }
                        }
                           // in bytes
                        else
                        {
                           // Convert to MFM
                           if (data_size >= 8)
                           {
                              index_track = disk->AddByteToSpecificTrack(
                                 imge_item.side, imge_item.track_number, index_track, bitfield);
                              data_size -= 8;
                           }
                           else
                           {
                              // Convert to MFM
                              index_track = disk->AddByteToSpecificTrack(
                                 imge_item.side, imge_item.track_number, index_track, bitfield, data_size * 2);
                              data_size = 0;
                           }
                        }
                     }
                  }
               }
               else
               {
                  // in bytes
                  if (data_type == 5)
                  {
                     // weak bit ??
                     unsigned char byte = 0xE5;

                     for (int c = 0; c < data_size * 16; c++)
                     {
                        if (c > 256 * 16 || data_size != 512)
                        {
                           disk->side_[imge_item.side].tracks[imge_item.track_number].bitfield[index_track++] =
                              BIT_WEAK;
                           if (index_track >= disk->side_[imge_item.side].tracks[imge_item.track_number].size)
                              index_track = 0;
                        }
                        else
                        {
                           index_track = disk->AddByteToSpecificTrack(imge_item.side, imge_item.track_number,
                                                                      index_track, byte);
                           c += 15;
                        }
                     }
                     //dataoffset++;
                  }
                  else
                  {
                     if (data_type == 3)
                     {
                        //totalGap += dataSize*16;
                     }
                     for (int c = 0; c < data_size; c++)
                     {
                        unsigned char byte = 0;
                        {
                           byte = current_block_data[data_offset++];

                           if (data_type == 1)
                           {
                              index_track = disk->AddMFMByteToAllTrack(imge_item.side, imge_item.track_number,
                                                                       index_track, byte, 8);
                           }
                           else
                           {
                              index_track = disk->AddByteToSpecificTrack(
                                 imge_item.side, imge_item.track_number, index_track, byte);
                           }
                        }
                     }
                  }
               }
               b = current_block_data[data_offset++];
            }
            if (offset_before > index_track)
            {
               offset_before = offset_before - disk->side_[imge_item.side].tracks[imge_item.track_number].size;
            }
            total_total += data_bits; //indexTrack - offsetBefore;
            total_data += data_bits;
            if ((data_bits != index_track - offset_before)
               && ((index_track != offset_before)
                  || (data_bits != disk->side_[imge_item.side].tracks[imge_item.track_number].size)
               )
            )
            {
               //
               int dbg = 1;
            }


            if (encoder_type_ == 1)
            {
               // GAP via CAPS encoding
               for (unsigned int c = 0; c < gap_bits; c += 16)
               {
                  if (gap_bits - c < 16)
                  {
                     // Add what we need to
                     index_track = disk->AddByteToSpecificTrack(imge_item.side, imge_item.track_number, index_track,
                                                                gap_default, (gap_bits - c));

                     // then quit
                     total_gap += gap_bits - c;
                     c = gap_bits;
                  }
                  else
                  {
                     {
                        index_track = disk->AddByteToSpecificTrack(imge_item.side, imge_item.track_number, index_track,
                                                                   gap_default);

                        if (index_track > imge_item.start_bit_pos && index_track < imge_item.start_bit_pos + 16)
                        {

                        }

                        total_gap += 16;
                     }
                  }
               }
            }
            else
            {
               if (forward_stream.size * 2 + backward_stream.size * 2 > gap_bits)
               {
                  // Remove from forward
                  forward_stream.size = (gap_bits - backward_stream.size * 2) / 2;
               }
               else
               {
                  if (forward_stream.size * 2 + backward_stream.size * 2 < gap_bits)
                  {
                     // Adjust :
                     //
                     if (forward_gap && forward_stream.size == 0)
                     {
                        if (backward_gap && backward_stream.size == 0)
                        {
                           forward_stream.size = gap_bits / 4;
                           backward_stream.size = (gap_bits - forward_stream.size * 2) / 2;
                        }
                        else
                        {
                           forward_stream.size = (gap_bits - backward_stream.size * 2) / 2;
                        }
                     }
                     else
                     {
                        if (backward_gap && backward_stream.size == 0)
                        {
                           backward_stream.size = (gap_bits - forward_stream.size * 2) / 2;
                        }
                     }
                  }
               }
               if (forward_gap)
               {
                  if (forward_stream.size == 0)
                  {
                     if (backward_stream.size != 0)
                     {
                        forward_stream.size = gap_bits / 2 - backward_stream.size;
                     }
                     else
                     {
                        forward_stream.size = backward_stream.size = gap_bits / 4;
                        backward_stream.sample = gap_default;
                     }
                     //forwardStream.Sample = gapDefault;
                  }
                  for (unsigned int c = 0; c < forward_stream.size * 2; c += 16)
                  {
                     if (forward_stream.size * 2 - c < 16)
                     {
                        // Add what we need to
                        index_track = disk->AddByteToSpecificTrack(imge_item.side, imge_item.track_number, index_track,
                                                                   forward_stream.sample,
                                                                   (forward_stream.size * 2 - c));

                        // then quit
                        total_gap += forward_stream.size * 2 - c;
                        c = forward_stream.size * 2;
                     }
                     else
                     {
                        if (forward_stream.sample_size == 8)
                        {
                           index_track = disk->AddByteToSpecificTrack(imge_item.side, imge_item.track_number,
                                                                      index_track, forward_stream.sample);

                           if (index_track > imge_item.start_bit_pos && index_track < imge_item.start_bit_pos + 16)
                           {
                           }

                           total_gap += 16;
                        }
                        else
                        {
                        }
                     }
                  }
               }
               // Between these two :
               {
                  if (forward_stream.size * 2 + backward_stream.size * 2 < gap_bits)
                  {
                     int nb_bytes = (gap_bits - (forward_stream.size * 2 + backward_stream.size * 2));
                     for (int gp = 0; gp < nb_bytes; gp += 16)
                     {
                        if (gp + 16 > nb_bytes)
                        {
                           // Add what we need to
                           index_track = disk->AddByteToSpecificTrack(imge_item.side, imge_item.track_number,
                                                                      index_track, 0x4E, (nb_bytes - gp));

                           // then quit
                           total_gap += nb_bytes - gp;
                        }
                        else
                        {
                           index_track = disk->AddByteToSpecificTrack(imge_item.side, imge_item.track_number,
                                                                      index_track, 0x4E);
                           total_gap += 16;
                        }

                        if (index_track > imge_item.start_bit_pos && index_track < imge_item.start_bit_pos + 16)
                        {
                        }
                     }
                  }
               }

               // Backward stream ?
               if (backward_gap)
               {
                  // First byte (or any part of it)
                  unsigned int c = 0;
                  if (backward_stream.size % 8 != 0)
                  {
                     // TODO
                     c = backward_stream.size % 8;
                     index_track = disk->AddByteToSpecificTrack(imge_item.side, imge_item.track_number, index_track,
                                                                backward_stream.sample, c);
                  }
                  for (; c < backward_stream.size * 2; c += 16)
                  {
                     if (backward_stream.size * 2 - c < 16)
                     {
                        // Add what we need to
                        index_track = disk->AddByteToSpecificTrack(imge_item.side, imge_item.track_number, index_track,
                                                                   backward_stream.sample,
                                                                   (backward_stream.size * 2 - c));

                        // then quit
                        total_gap += backward_stream.size * 2 - c;
                        c = backward_stream.size * 2;
                     }
                     else
                     {
                        if (backward_stream.sample_size == 8)
                        {
                           index_track = disk->AddByteToSpecificTrack(imge_item.side, imge_item.track_number,
                                                                      index_track, backward_stream.sample);

                           if (index_track > imge_item.start_bit_pos && index_track < imge_item.start_bit_pos + 16)
                           {
                           }

                           total_gap += 16;
                        }
                        else
                        {
                        }
                     }
                  }
               }
            }
            if (total_gap != gap_bits)
            {
            }
            //else
            total_total += total_gap;
         }

         // Check index / size
         if (index_track != imge_item.start_bit_pos || total_total != disk->side_[imge_item.side].tracks[imge_item.
            track_number].size)
         {
         }
      }
   }

   return true;
}

// decompress CT Raw dump format
int CAPSFile::DecompressData(IDisk* disk, DWORD ctype, DWORD cyl, DWORD head, DataChunks imageItem)
{
   // Get the DATA buffer :

   // First PACK size
   int offset = 0;
   int density_buf_size = ExtractInt(&imageItem.buffer[offset]);
   offset += 4;
   // Second PACK size
   int track_buf_size = ExtractInt(&imageItem.buffer[offset]);
   offset += 4;

   // First pack is density information
   ExtractDensity(&imageItem.buffer[offset], density_buf_size, ctype, cyl, head);
   offset += density_buf_size;

   // Second one is track informations
   ExtractTrack(disk, &imageItem.buffer[offset], track_buf_size, ctype, cyl, head);

   return 0;
}

unsigned int* CAPSFile::ExtractDensity(unsigned char* buffer, int density_buf_size, DWORD ctype, DWORD cyl, DWORD head)
{
   unsigned int usize = ExtractInt(&buffer[4]);
   unsigned int ucrc = ExtractInt(&buffer[8]);; // CRC on uncompressed data
   unsigned int csize = ExtractInt(&buffer[12]);; // compressed size in bytes
   unsigned int ccrc = ExtractInt(&buffer[16]);; // CRC on compressed data
   unsigned int hcrc = ExtractInt(&buffer[20]);; // CRC on header calculated as hcrc=0

   // density
   unsigned int* buf = new unsigned int[usize >> 2];

   // Ptr to end of buffer
   unsigned int* mem = &buf[usize >> 2];
   unsigned int offset_source = density_buf_size;

   // Decode data :
   while (mem > buf)
   {
      int size, ofs = 0;
      unsigned char cb0 = buffer[--offset_source];

      switch (cb0 & 0x03)
      {
      case 0x0: // data block
         // 14 or 4 bit size
         if (cb0 & 0x8)
         {
            size = ((cb0 << 4) & 0xF00) | (buffer[--offset_source]);
         }
         else
         {
            size = (cb0 >> 4) + 1;
         }
         if (cb0 & 0x4)
         {
            // 4 byte per value
            while (size--)
            {
               unsigned int data;
               data = buffer[--offset_source];
               data = (data << 8) | buffer[--offset_source];
               data = (data << 8) | buffer[--offset_source];
               data = (data << 8) | buffer[--offset_source];
               *--mem = data;
            }
         }
         else
         {
            while (size--)
            {
               *--mem = buffer[--offset_source];
            }
         }
         continue;

      case 0x1: // copy block, 8bit offset, 6 bit size
         size = (cb0 >> 2) + 1;
         ofs = buffer[--offset_source] + 1;
         break;
      case 0x2: // copy block, 16 bit offset, 6 bit size
         size = (cb0 >> 2) + 1;
         ofs = buffer[--offset_source];
         ofs = (ofs << 8) | buffer[--offset_source];
         break;
      case 0x3: // Copy block, 16 bit offset, 14 bit size
         size = ((cb0 << 6) & 0x3F00) | buffer[--offset_source];
         ofs = buffer[--offset_source];
         ofs = (ofs << 8) | buffer[--offset_source];
         break;
      }

      while (size--)
      {
         mem--;
         *mem = mem[ofs];
      }
   }

   return buf;
}

////////////////////////////
// Extract CTRAW track
int CAPSFile::ExtractTrack(IDisk* disk, unsigned char* buffer, int track_buf_size, DWORD ctype, DWORD cyl, DWORD head)
{
   // PACK HEADER
   unsigned int usize = ExtractInt(&buffer[4]);
   unsigned int ucrc = ExtractInt(&buffer[8]);; // CRC on uncompressed data
   unsigned int csize = ExtractInt(&buffer[12]);; // compressed size in bytes
   unsigned int ccrc = ExtractInt(&buffer[16]);; // CRC on compressed data
   unsigned int hcrc = ExtractInt(&buffer[20]);; // CRC on header calculated as hcrc=0


   delete[]disk->side_[head].tracks[cyl].revolution[0].bitfield;
   delete[]disk->side_[head].tracks[cyl].revolution;


   disk->side_[head].tracks[cyl].nb_revolutions = buffer[24];
   disk->side_[head].tracks[cyl].revolution = new IDisk::Revolution[disk->side_[head].tracks[cyl].nb_revolutions];

   int offset = 25;
   for (unsigned int rev = 0; rev < disk->side_[head].tracks[cyl].nb_revolutions; rev++)
   {
      disk->side_[head].tracks[cyl].revolution[rev].size = ExtractShort(&buffer[offset]) * 8;
      disk->side_[head].tracks[cyl].revolution[rev].bitfield = new unsigned char[disk->side_[head].tracks[cyl].
         revolution[rev].size];
      offset += 2;
   }

   // First track
   unsigned int track_offset = 0;
   if (disk->side_[head].tracks[cyl].nb_revolutions > 0)
   {
      for (unsigned int i = 0; i < disk->side_[head].tracks[cyl].revolution[0].size / 8; i++)
      {
         track_offset = disk->AddMFMByteToSpecificTrack(head, cyl, 0, track_offset, buffer[offset++], 8);
      }
   }

   for (unsigned int rev = 1; rev < disk->side_[head].tracks[cyl].nb_revolutions; rev++)
   {
      track_offset = 0;
      while (track_offset < disk->side_[head].tracks[cyl].revolution[rev].size && offset < track_buf_size)
      {
         // Lecture de la taille
         unsigned int size = buffer[offset++];
         if (size & 0x80)
         {
            // Copy block
            // 3 bit shift
            int shift = ((size >> 4) & 7);

            // 12 bits size
            size = ((size & 0xF) << 8) | buffer[offset++];

            // 16 bits offset
            unsigned int ofs = ((buffer[offset] << 8) | (buffer[offset + 1]));
            offset += 2;
            ofs *= 8;
            unsigned char* buf = &disk->side_[head].tracks[cyl].revolution[0].bitfield[ofs];

            if (shift)
            {
               // Shift; "shift" bit :
               size *= 8;
               buf += (8 - shift);
               if (track_offset + size > disk->side_[head].tracks[cyl].revolution[rev].size)
               {
               }
               if (ofs + (8 - shift) + size > disk->side_[head].tracks[cyl].revolution[0].size)
               {
               }
               memcpy(&disk->side_[head].tracks[cyl].revolution[rev].bitfield[track_offset], buf, size);
               track_offset += size;
            }
            else
            {
               size *= 8;
               if (track_offset + size > disk->side_[head].tracks[cyl].revolution[rev].size)
               {
                  // todo ?
               }
               if (ofs + (8 - shift) + size > disk->side_[head].tracks[cyl].revolution[0].size)
               {
                  // todo ?
               }


               memcpy(&disk->side_[head].tracks[cyl].revolution[rev].bitfield[track_offset], buf, size);
               track_offset += size;
            }
         }
         else
         {
            // Data block, 15 bit size
            size = (size << 8) | buffer[offset++];

            for (size; size > 0; size--)
            {
               track_offset = disk->AddMFMByteToSpecificTrack(head, cyl, rev, track_offset, buffer[offset++], 8);
            }
         }
      }
   }

   return 0;
}

int CAPSFile::CreateIpf(IDisk* disk)
{
   // Clear values
   memset(&caps_header_, 0, sizeof(caps_header_));
   memset(&caps_info_, 0, sizeof(caps_header_));
   Clear();

   // Now create it !
   ////////////////////
   // CAPS
   memcpy(caps_header_.type, "CAPS", 4);
   caps_header_.length = 12;
   caps_header_.crc = 0;
   Swap((unsigned char*)&caps_header_.length, caps_header_.length - 4);
   unsigned int crc = CRC::ComputeCrc32(0xEDB88320, (unsigned char*)&caps_header_, 12);
   caps_header_.crc = SwapInt(crc);

   ///////////////////////
   // INFO Chunk
   memset(&caps_info_, 0, sizeof(caps_info_));
   memcpy(caps_info_.header.type, "INFO", 4);
   caps_info_.header.length = 96;
   caps_info_.header.crc = 0;
   caps_info_.media_type = 1;
   caps_info_.encoder_type = 2;
   caps_info_.encoder_rev = 1;
   caps_info_.file_key = 0;
   caps_info_.file_rev = 1;
   caps_info_.origin_crc32 = 0; // TODO ??
   caps_info_.track_min = 0;
   caps_info_.side_min = 0;
   caps_info_.side_max = 1; // disk->m_NbSide - 1;
   caps_info_.creation_date = 0; // TODO
   caps_info_.creation_time = 0; // TODO
   caps_info_.platform_1 = 4;
   caps_info_.disk_number = 0;
   caps_info_.creator_id = 0; // TODO

   // Unset unformatted tracks at the end

   // Now, fill the tracks ! - Force 2 sides
   int datakey = 1;
   int maxtrack = disk->side_[0].nb_tracks > disk->side_[1].nb_tracks
                     ? disk->side_[0].nb_tracks
                     : disk->side_[1].nb_tracks;

   for (int tr = 0; tr < maxtrack; tr++)
   {
      for (int side = 0; side < 2/*disk->m_NbSide*/; side++)
      {
         CreateData(disk, side, tr, datakey++);
      }
   }

   caps_info_.track_max = disk->side_[0].nb_tracks - 1;
   Swap((unsigned char*)&caps_info_.header.length, caps_info_.header.length - 4);
   crc = CRC::ComputeCrc32(0xEDB88320, (unsigned char*)&caps_info_, sizeof(caps_info_));
   caps_info_.header.crc = SwapInt(crc);


   return 0;
}


int CAPSFile::AddDataRecordInBits(StreamRecording* record, unsigned char type, unsigned int nb_bits_to_add,
                                  unsigned char* buffer)
{
   unsigned char size = 0;
   int tmp = nb_bits_to_add;
   while (tmp > 0)
   {
      tmp >>= 8;
      size++;
   }
   while (record->size - record->offset <= (size + 2 + nb_bits_to_add))
   {
      unsigned char* tmp_buffer = record->buffer;
      record->buffer = new unsigned char[record->size * 2];
      memcpy(record->buffer, tmp_buffer, record->size);
      record->size = record->size * 2;
      delete[]tmp_buffer;
   }

   if (size > 0)
   {
      record->buffer[record->offset++] = (size << 5) | type;
      for (int c = 0; c < size; c++)
      {
         record->buffer[record->offset++] = (nb_bits_to_add >> (8 * (size - (c + 1))));
      }


      unsigned short b = 0;
      int dec = 15;
      for (unsigned int i = 0; i < nb_bits_to_add; i++)
      {
         b |= ((buffer[i] & 1) << dec);
         dec--;
      }

      // Sample now
      record->buffer[record->offset++] = ((b >> 8) & 0xFF); // Sample
      if (nb_bits_to_add > 8)
      {
         record->buffer[record->offset++] = ((b) & 0xFF); // Sample
      }
   }
   return 0;
}

int CAPSFile::AddDataRecord(StreamRecording* record, unsigned char type, int nb_byte_to_add, unsigned char* buffer,
                            bool byte_size)
{
   unsigned int size = 0;
   int tmp = nb_byte_to_add;
   int nbByte = nb_byte_to_add;
   while (tmp > 0)
   {
      tmp >>= 8;
      size++;
   }

   while (record->size - record->offset <= (size + 2 + nb_byte_to_add))
   {
      unsigned char* tmp_buffer= record->buffer;
      record->buffer = new unsigned char[record->size * 2];
      memcpy(record->buffer, tmp_buffer, record->size);
      record->size = record->size * 2;
      delete[]tmp_buffer;
   }
   if (size > 0)
   {
      record->buffer[record->offset++] = (size << 5) | type;
      for (unsigned int c = 0; c < size; c++)
      {
         record->buffer[record->offset++] = (nbByte >> (8 * (size - (c + 1))));
      }

      // Sample now
      if (type == 5)
      {
         // Nothing to add !
         // TODO : Check
      }
      else
      {
         if (byte_size)
         {
            for (int c = 0; c < nb_byte_to_add; c++)
            {
               record->buffer[record->offset++] = buffer[c]; // Sample
            }
         }
         else
         {
            int c = 0;
            while (nb_byte_to_add > 0)
            {
               if (nb_byte_to_add > 8)
               {
                  record->buffer[record->offset++] = buffer[c++]; // Sample
                  nb_byte_to_add -= 8;
               }
               else
               {
                  record->buffer[record->offset++] = buffer[c++]; // Sample
                  nb_byte_to_add = 0;
               }
            }
         }
      }
   }
   return 0;
}

int CAPSFile::AddShortStreamRecord(StreamRecording* record, unsigned char* buffer, unsigned int offset_begin,
                                   unsigned int offset_end)
{
   // Record less than 16 bits - So MFM bits !
   AddDataRecordInBits(record, 1, (offset_end - offset_begin), &buffer[offset_begin]);
   record->buffer[record->offset++] = 0x00; // End of data
   return 0;
}

bool CAPSFile::AddStreamRecord(StreamRecording* record, unsigned char* buffer, unsigned int offset_begin,
                               unsigned int offset_end, unsigned int size_of_track)
{
   unsigned char byte;
   // first byte is sync
   // First is always sync block
   unsigned char sync_buf[6] = {0};
   unsigned char buffer_short[2]; // FC, CRC, etc
   int offset = 0;
   int sector_size = 0;
   // Record eveything between of odfsetBegin and offset_end
   bool byte_size = true;

   // Now, compute if byte or bit
   int current_offset = offset_begin;
   bool end = false;
   while (!end && byte_size)
   {
      int next_sync = FindNextSector(buffer, offset_end, current_offset, 6);

      // Ok, get ready for bitsize everywhere !
      if (next_sync == -1)
      {
         next_sync = offset_end;
         end = true;
      }
      if ((next_sync - current_offset) % 16 != 0)
         byte_size = false;
      else
      {
         if (next_sync != current_offset)
         {
            // Check for bit/byte size : Size is not enough
            for (int i = offset_begin; i < next_sync && byte_size; i += 16)
            {
               if (i < next_sync)
               {
                  byte_size = IDisk::MFMCorrect(&buffer[i], (i > 0) ? (buffer[i - 1]) : 0);
                  if (i > 0)
                  {
                     if ((buffer[i - 16 + 15] == 0 && buffer[i] == 0 && (buffer[i + 1] == 0 || (i > 2 && buffer[i - 16 +
                           14] == 0)))
                        || (buffer[i - 16 + 15] == 1 && buffer[i] == 1))
                     {
                        byte_size = false;
                     }
                  }
                  else
                  {
                     if ((buffer[size_of_track - 1] == 0 && buffer[i] == 0 && (buffer[i + 1] == 0 || (i > 2 && buffer[
                           size_of_track - 16 + 14] == 0)))
                        || (buffer[size_of_track - 1] == 1 && buffer[i] == 1))
                     {
                        byte_size = false;
                     }
                  }
               }
               else
                  byte_size = false;
            }
         }
         else
         {
            byte_size = false;
         }
      }
      // PROPAGATE NOW !
      current_offset = next_sync + 1;
   }


   int size_to_add;
   while (offset_begin < offset_end)
   {
      int remainingBits = 0;
      if (offset_end - offset_begin < 6 * 8)
      {
         // Record what we can, then end
         for (unsigned int i = 0; i < (offset_end - offset_begin) / 8; i++)
         {
            byte = IDisk::ConvertToByte(&buffer[offset_begin + offset]);
            sync_buf[i] = byte;
            offset += 8;
         }

         size_to_add = (byte_size) ? (offset_end - offset_begin) / 8 : (offset_end - offset_begin);
         AddDataRecord(record, 1, size_to_add, sync_buf, byte_size);
         record->buffer[record->offset++] = 0x00; // End of data
         return offset_end - offset_begin;
      }
      // Record what we can, then end
      // Weak bits ?
      // TODO
      if (IDisk::NoWeakBits(buffer, offset_begin, 6 * 8))
      {
         for (int i = 0; i < 6; i++)
         {
            byte = IDisk::ConvertToByte(&buffer[offset_begin]);
            sync_buf[i] = byte;
            offset_begin += 8;
         }
         size_to_add = (byte_size) ? 6 : 48;
         AddDataRecord(record, 1, size_to_add, sync_buf, byte_size);
      }

      // Next byte
      if (offset_end > offset_begin)
      {
         // Weak bits ?
         bool byte_read = false;
         if (offset_begin > 0 && (offset_end - offset_begin >= 16) && IDisk::NoWeakBits(buffer, offset_begin, 16) &&
            IDisk::MFMCorrect(&buffer[offset_begin], (offset_begin > 0) ? (buffer[offset_begin - 1]) : 0))
         {
            buffer_short[0] = IDisk::ConvertMFMToByte(&buffer[offset_begin]);
            byte_read = true;
            offset_begin += 16;
            size_to_add = (byte_size) ? 1 : 8;
            AddDataRecord(record, 2, size_to_add, buffer_short, byte_size);
         }
         // Check the next sync byte
         unsigned int next_sync = FindNextSector(buffer, offset_end, offset_begin, 6);
         if (next_sync == -1)
            next_sync = offset_end;


         if (next_sync > offset_begin)
         {
            // Check for weak bits or incorrect MFM in CHRN area
            int sizeToCheck = (next_sync - offset_begin < 6 * 16) ? (next_sync - offset_begin) : 6 * 16;
            if (IDisk::NoWeakBits(buffer, offset_begin, sizeToCheck) == false)
               byte_read = false;
            for (int i = 0; i < sizeToCheck; i++)
            {
               if (offset_begin + i * 16 + 16 > next_sync)
                  byte_read = false;
               else
               {
                  if (IDisk::MFMCorrect(&buffer[offset_begin + i * 16],
                                        (offset_begin + i * 16 > 0) ? buffer[offset_begin + i * 16 - 1] : 0) == false)
                  {
                     byte_read = false;
                  }
               }
            }


            // between these two (or the first and the end), it's pure data
            if (byte_read && buffer_short[0] == 0xFE)
            {
               // CHRN + CRC
               // Weak bits ?
               // TODO

               if (next_sync - offset_begin < 6 * 16)
               {
                  for (unsigned int i = 0; i < (next_sync - offset_begin) / 16; i++)
                  {
                     byte = IDisk::ConvertMFMToByte(&buffer[offset_begin + offset]);
                     sync_buf[i] = byte;
                     offset += 16;
                  }
                  size_to_add = (byte_size) ? (next_sync - offset_begin) / 8 : (next_sync - offset_begin);
                  AddDataRecord(record, 1, size_to_add, sync_buf, byte_size);
                  offset_begin += offset /** 16*/;
               }
               else
               {
                  unsigned char buffer_chrn[6];
                  for (int i = 0; i < 6; i++)
                  {
                     byte = IDisk::ConvertMFMToByte(&buffer[offset_begin]);
                     buffer_chrn[i] = byte;
                     offset_begin += 16;
                  }
                  // Keep size for next run
                  sector_size = (buffer_chrn[3] << 8);

                  size_to_add = (byte_size) ? 6 : 6 * 8;
                  AddDataRecord(record, 2, size_to_add, buffer_chrn, byte_size);
               }
            }

            // DATA - sector_size in theory, with size of buffer in logic
            while (next_sync > offset_begin)
            {
               // While there is something to do !
               // Search for next weak bits
               int next_weak_bits_index = GetNextFuzzyBits(buffer, offset_begin, next_sync, true);

               // Normal Data : From offset_begin to next_weak_bits_index or next_sync if nothing found)
               int end_of_weak_range = next_sync;
               next_sync = (next_weak_bits_index == -1) ? next_sync : next_weak_bits_index;

               int lastbits = (next_sync - offset_begin) % 16;

               int max_size_mfm_test = (next_sync - offset_begin) / 16;

               bool mfm_is_correct = true;
               for (unsigned int i = offset_begin; i < next_sync && mfm_is_correct; i += 16)
               {
                  if (i < next_sync && i + 16 < next_sync)
                  {
                     mfm_is_correct = IDisk::MFMCorrect(&buffer[i], (i > 0) ? buffer[i - 1] : 0);
                     if (i > 0)
                     {
                        if ((buffer[i - 16 + 15] == 0 && buffer[i] == 0 && (buffer[i + 1] == 0 || (buffer[i - 16 + 14]
                              == 0)))
                           || (buffer[i - 16 + 15] == 1 && buffer[i] == 1)
                        )
                        {
                           mfm_is_correct = false;
                        }
                     }
                     else
                     {
                        // todo !!!
                     }
                  }
                  else
                     mfm_is_correct = false;
               }

               if ((lastbits != 0) || (!mfm_is_correct)) // todo : Check this !!
               {
                  int max_size = (next_sync - offset_begin);

                  // MFM so everything is recorded

                  unsigned char* buffer_data = new unsigned char[(next_sync - offset_begin) / 8 + 2];
                  memset(buffer_data, 0, (next_sync - offset_begin) / 8 + 2);

                  for (int i = 0; i < max_size / 8; i++)
                  {
                     buffer_data[i] = IDisk::ConvertToByte(&buffer[offset_begin]);
                     offset_begin += 8;
                  }
                  // Remaining bits
                  int dec = 7;
                  int buf_indx = max_size / 8;
                  lastbits = max_size - (max_size / 8) * 8;
                  for (int i = 0; i < lastbits; i++)
                  {
                     buffer_data[buf_indx] |= ((buffer[offset_begin++] & 1) << dec);
                     if (dec == 0)
                     {
                        dec = 8;
                        buf_indx++;
                     }
                     dec--;
                     //offset_begin ++;
                  }
                  size_to_add = max_size;
                  AddDataRecord(record, 1, size_to_add, buffer_data, false);
                  delete[]buffer_data;
               }
               else
               {
                  int maxSize = (next_sync - offset_begin) / 16;
                  unsigned char* buffer_data = new unsigned char[maxSize];

                  for (int i = 0; i < maxSize; i++)
                  {
                     buffer_data[i] = IDisk::ConvertMFMToByte(&buffer[offset_begin]);
                     offset_begin += 16;
                  }
                  // Remaining bits
                  size_to_add = (byte_size) ? maxSize : (maxSize * 8);
                  AddDataRecord(record, 2, size_to_add, buffer_data, byte_size);
                  delete[]buffer_data;
               }
               // Add weak rang (if any) : From next_sync to end_of_weak_range
               if (next_sync != end_of_weak_range)
               {
                  // Search for next "end opf weak" range
                  int next_weak_bits_index = GetNextFuzzyBits(buffer, offset_begin, end_of_weak_range, false);
                  if (next_weak_bits_index != -1)
                  {
                     AddDataRecord(record, 5, next_weak_bits_index - next_sync, 0, false);
                  }
                  else
                  {
                     AddDataRecord(record, 5, end_of_weak_range - next_sync, 0, false);
                  }


                  offset_begin = next_weak_bits_index;
                  next_sync = end_of_weak_range;
               }
            }
            // Last bits
            // TODO
         }
         offset_begin = next_sync - remainingBits;
      }

      // Check that's MFM does not need to be recorded
      // TODO
      // Check for weak bit
      // TODO
   }
   record->buffer[record->offset++] = 0x00; // End of data

   return byte_size;
}

int CAPSFile::AddGapRecord(StreamRecording* record, int nb_byte_to_add, unsigned char byte)
{
   unsigned int size = 0;
   nb_byte_to_add *= 8;
   int tmp = nb_byte_to_add;
   while (tmp > 0)
   {
      tmp >>= 8;
      size++;
   }

   if (record->size - record->offset <= (size + 5))
   {
      unsigned char* tmp = record->buffer;

      record->buffer = new unsigned char[record->size * 2];
      memcpy(record->buffer, tmp, record->size);
      record->size = record->size * 2;
      delete[]tmp;
   }
   if (size > 0)
   {
      record->buffer[record->offset++] = (size << 5) | 0x01;
      for (unsigned int c = 0; c < size; c++)
      {
         record->buffer[record->offset++] = ((nb_byte_to_add >> (8 * (size - (c + 1)))) & 0xFF);
      }

      // Sample now
      record->buffer[record->offset++] = (1 << 5) | 0x02;
      record->buffer[record->offset++] = 8;
      record->buffer[record->offset++] = byte; // Sample
      record->buffer[record->offset++] = 0x00; // End of data
   }
   return 0;
}

bool CAPSFile::NoWeakBitInPattern(unsigned char* pattern, unsigned int size_of_pattern)
{
   for (unsigned int i = 0; i < size_of_pattern; i++)
   {
      if ((pattern[i] & (BIT_WEAK | BIT_OPTIONAL)) != 0)
         return false;

      // Check for correctness of MFM
      if (i > 2 &&
         (
            (pattern[i - 1] == 1 && pattern[i - 2] == 1)
            || (pattern[i - 1] == 0 && pattern[i - 2] == 0 && pattern[i] == 0)
               || (pattern[i - 1] == 1 && pattern[i] == 1)
         )
      )
      {
         return false;
      }
   }

   return true;
}

int CAPSFile::CreateData(IDisk* disk, int side, unsigned int tr, int datakey)
{
   // Create IMGE and data structure
   ImgeItem imge_item;
   DataChunks data_chunk;

   bool abort = false;

   StreamRecording gap_data;
   StreamRecording data;
   data.offset = 0;
   data.buffer = new unsigned char[128];
   data.size = 128;

   gap_data.offset = 0;
   gap_data.buffer = new unsigned char[128];
   gap_data.size = 128;

   int gap_offset = 0;
   //unsigned char* pData = new unsigned char[128];
   int max_size_data = 128;

   // Block descriptor vector
   std::vector<BlockDescriptor> list_block_descriptor;


   memset(&imge_item, 0, sizeof(imge_item));
   memset(&data_chunk, 0, sizeof(data_chunk));

   // &disk->disk_[side].Tracks[tr]);

   ////////////
   // IMGE
   memcpy(imge_item.header.type, "IMGE", 4);
   imge_item.header.crc = 0;
   imge_item.header.length = 0x50;
   imge_item.track_number = tr;
   imge_item.side = side;
   imge_item.density = 2; // auto
   imge_item.signal_type = 1; // 2us cell
   imge_item.track_byte = 0; //
   imge_item.start_byte_pos = 0;
   imge_item.start_bit_pos = 0;
   imge_item.data_bits = 0;
   imge_item.gap_bits = 0;
   imge_item.track_bits = 0;
   imge_item.block_count = 0;
   imge_item.encoder_process = 0; // 0
   imge_item.track_flags = 0;
   imge_item.data_key = 0;

   // Nothing on this side / track ?
   if (side >= disk->nb_sides_ || tr >= disk->side_[side].nb_tracks)
   {
      imge_item.start_bit_pos = 0;
      imge_item.density = 1;
      abort = true;
      //goto End;//return 0;
   }

   unsigned char* track_field = nullptr;
   if (!abort)
   {
      track_field = disk->side_[side].tracks[tr].bitfield;
      // DATA
      if (track_field == NULL)
      {
         imge_item.start_bit_pos = 0;
         imge_item.density = 1;
         abort = true;
         //goto End;//return 0;
      }
   }

   unsigned int startbitpos = 0;
   // Construct DATA associated :
   // Find begining of track

   // Find the 0x5224 0x5224 0x5224  first sync
   unsigned int search_offset = 0;
   unsigned int current_recored_offset = 0;

   unsigned char pattern_byte[16];

   if (!abort)
   {
      // Only search the 3 x 0x5224 bytes
      search_offset = FindNextPattern(track_field, disk->side_[side].tracks[tr].size, search_offset, PATTERN_C2, 6);
      if (search_offset != -1)
      {
         imge_item.start_bit_pos = (search_offset != -1) ? search_offset : 0;
         imge_item.start_byte_pos = imge_item.start_bit_pos / 8;
      }
      else
      {
         search_offset = FindNextSector(track_field, disk->side_[side].tracks[tr].size, search_offset + 1, 8);
         if (search_offset != -1)
         {
            imge_item.start_bit_pos = (search_offset != -1) ? search_offset : 0;
            imge_item.start_byte_pos = imge_item.start_bit_pos / 8;
         }
         else
         {
            // Not a formatted disk : Generate random data
            imge_item.start_bit_pos = 0;
            imge_item.density = 1;
            int dbg = 1;
            //goto End;
            abort = true;
         }
      }
   }

   if (!abort)
   {
      track_field = new unsigned char[disk->side_[side].tracks[tr].size];
      memcpy(track_field, &disk->side_[side].tracks[tr].bitfield[imge_item.start_bit_pos],
             disk->side_[side].tracks[tr].size - imge_item.start_bit_pos);
      memcpy(&track_field[disk->side_[side].tracks[tr].size - imge_item.start_bit_pos],
             disk->side_[side].tracks[tr].bitfield, imge_item.start_bit_pos);

      search_offset = 0;
      int initial_backward_gap = 0;
      unsigned int offset_gap = disk->side_[side].tracks[tr].size - 16;
      while ((offset_gap > current_recored_offset && memcmp(&track_field[offset_gap], PATTERN00, 16) == 0)
         && (offset_gap < 1 || track_field[offset_gap - 1] != 1)
      )
      {
         offset_gap -= 16;
         initial_backward_gap++;
      }

      while (search_offset != -1)
      {
         int nb_backward_gap_bytes = 0;
         // Add it : Descriptor, and data
         BlockDescriptor block_descriptor;
         memset(&block_descriptor, 0, sizeof(block_descriptor));

         block_descriptor.block_flag = 0;
         block_descriptor.data_offset = data.offset;
         block_descriptor.sps_specific.gap_offset = gap_data.offset;


         unsigned int search_next_offset = FindNextSector(track_field, disk->side_[side].tracks[tr].size,
                                                          search_offset + 1, 8);
         int end_of_block = 0;
         if (search_next_offset != -1)
         {
            // Next block found : Dont use the '0x00' sync bytes before it - It's the 'backward' gap
            unsigned int offset_gap = search_next_offset - 16;
            nb_backward_gap_bytes = 0;
            while ((offset_gap > search_offset && memcmp(&track_field[offset_gap], PATTERN00, 16) == 0)
               && (offset_gap < 1 || track_field[offset_gap - 1] != 1)
            )
            {
               offset_gap -= 16;
               nb_backward_gap_bytes++;
            }
            end_of_block = offset_gap + 16;
         }
         else
         {
            // End block : until the end, then until IMGEitem.start_bit_pos
            nb_backward_gap_bytes = initial_backward_gap;
            end_of_block = disk->side_[side].tracks[tr].size - nb_backward_gap_bytes * 16;
         }

         // What to record : From the next block, until it changed, record the "forward gap"
         memcpy(pattern_byte, &track_field[end_of_block - 16], 16);
         int nb_gap_bytes = 0;
         unsigned int offset_gap = end_of_block - 16;

         // Only if no weak bits in the pattern !
         if (NoWeakBitInPattern(pattern_byte, 16))
         {
            if (pattern_byte[0] == 0
               && pattern_byte[15] == 0
               && (pattern_byte[14] & pattern_byte[1]) == 0
            )
            {
               int dbg = 1;
            }

            while ((offset_gap > search_offset && memcmp(&track_field[offset_gap], pattern_byte, 16) == 0)
               && (((track_field[offset_gap] & track_field[offset_gap - 1]) != 1) || offset_gap - 1 <= search_offset)
               && (offset_gap < 1 || ((track_field[offset_gap - 1] & track_field[offset_gap] & track_field[offset_gap +
                     1]) == 1)
                  && (offset_gap < 2 || (track_field[offset_gap - 1] & track_field[offset_gap] & track_field[offset_gap
                     - 2]) == 1)
               )
            )
            {
               offset_gap -= 16;
               nb_gap_bytes++;
            }
         }
         offset_gap += 16;
         // pPatternByte -> signel byte
         unsigned char byte = IDisk::ConvertMFMToByte(pattern_byte);
         {
            AddGapRecord(&gap_data, nb_gap_bytes, byte);
            block_descriptor.gap_bits += nb_gap_bytes * 16;
            if (nb_backward_gap_bytes > 0)
            {
               AddGapRecord(&gap_data, nb_backward_gap_bytes, 0x00);
               block_descriptor.gap_bits += nb_backward_gap_bytes * 16;
               imge_item.gap_bits += 16 * nb_backward_gap_bytes;
               imge_item.track_byte += nb_backward_gap_bytes * 2;
               block_descriptor.block_flag |= 2;
            }


            imge_item.gap_bits += 16 * nb_gap_bytes;
            imge_item.track_byte += nb_gap_bytes * 2;
            if (nb_gap_bytes > 0)
               block_descriptor.block_flag |= 1;

            // TODO : Adjust at bit value
            // data itself : Add the data (supposed to PatternC2 + 16 bits (FC most of the time)
            // From searchOffset -> offsetGap;
            bool byte_size = AddStreamRecord(&data, track_field, search_offset, offset_gap,
                                             disk->side_[side].tracks[tr].size);
            imge_item.data_bits += (offset_gap - search_offset);
            imge_item.track_byte += (offset_gap - search_offset + 7) / 8;

            // Block descriptor : End the description
            block_descriptor.block_bits = (offset_gap - search_offset);
            block_descriptor.sps_specific.cell_type = 1;
            block_descriptor.encoder_type = 1;

            if (!byte_size)
               block_descriptor.block_flag |= 4; // in byte. TODO : Use value for bits
            block_descriptor.gap_value = 0x4E;

            list_block_descriptor.push_back(block_descriptor);
            imge_item.block_count++;

            search_offset = search_next_offset;
         }

         // Next blocks
      }

      delete[]track_field;
   }
   // End of track (or unformatted ones !)
   // TODO
   //End:

   // End : Now compile every descriptor, then every datas ( ).
   // dataoffset and gapoffsets
   memcpy(data_chunk.header.type, "DATA", 4);
   data_chunk.header.length = sizeof (data_chunk) - sizeof (unsigned char*); //+ listBlockDescriptor.size() * sizeof(tBlockDescriptor);
   //dataChunk.header.Length += data.Offset + gapData.Offset;
   data_chunk.size = list_block_descriptor.size() * sizeof(BlockDescriptor);
   int final_gap_offset = data_chunk.size;
   int final_data_offset = final_gap_offset + gap_data.offset;

   data_chunk.size += gap_data.offset;
   data_chunk.size += data.offset;
   data_chunk.bit_size = data_chunk.size * 8;

   // Now compile DATA chunks into a buffer
   data_chunk.buffer = new unsigned char[data_chunk.size];
   int final_offset = 0;

   for (std::vector<BlockDescriptor>::iterator it = list_block_descriptor.begin(); it != list_block_descriptor.end(); it
        ++)
   {
      // Adjust gap offset
      it->sps_specific.gap_offset += final_gap_offset;
      // Adjust data offset
      it->data_offset += final_data_offset;

      // Copy block descriptor
      memcpy(&data_chunk.buffer[final_offset], &(*it), sizeof(BlockDescriptor));
      Swap(&data_chunk.buffer[final_offset], sizeof(BlockDescriptor));
      final_offset += sizeof(BlockDescriptor);
   }
   memcpy(&data_chunk.buffer[final_gap_offset], gap_data.buffer, gap_data.offset);
   memcpy(&data_chunk.buffer[final_data_offset], data.buffer, data.offset);

   data_chunk.data_id = imge_item.data_key = datakey;
   data_chunk.data_crc = CRC::ComputeCrc32(0xEDB88320, data_chunk.buffer, data_chunk.size);

   // Now, update IMGE
   imge_item.track_bits = imge_item.gap_bits + imge_item.data_bits;

   if (side < disk->nb_sides_ && tr < disk->side_[side].nb_tracks)
   {
      if (disk->side_[side].tracks[tr].size != imge_item.track_bits)
      {
         // error ?
         int dbg = 1;
      }
   }
   // .. To add

   // CRC
   imge_item.header.crc = SwapInt(CRC::ComputeCrc32(0xEDB88320, (unsigned char*)&imge_item, sizeof(imge_item)));

   list_imge_.push_back(imge_item);
   list_data_.push_back(data_chunk);

   delete []data.buffer;
   delete []gap_data.buffer;

   return 0;
}


unsigned int CAPSFile::ComputeSize()
{
   unsigned int size = 0;

   size += sizeof (caps_header_);
   size += sizeof (caps_info_);
   size += list_imge_.size() * sizeof(ImgeItem);

   for (std::vector<DataChunks>::iterator it = list_data_.begin(); it != list_data_.end(); it++)
   {
      size += it->header.length;
      size += it->size;
   }

   return size;
}


int CAPSFile::WriteToBuffer(unsigned char* buffer, unsigned int size_of_buffer)
{
   // Write header
   int offset = 0;

   if (size_of_buffer < ComputeSize())
      return -1;

   int size = SwapInt(caps_header_.length);
   memcpy(&buffer[offset], &caps_header_, size);
   offset += size;

   // Write INFO
   size = SwapInt(caps_info_.header.length);
   memcpy(&buffer[offset], &caps_info_, size);
   offset += size;

   // Write IMGE
   for (std::vector<ImgeItem>::iterator it = list_imge_.begin(); it != list_imge_.end(); ++it)
   {
      int base_offset = offset;
      it->header.crc = 0;
      int lg = it->header.length;
      memcpy(&buffer[offset], &(*it), lg);
      Swap(&buffer[offset + 4], lg - 4);
      unsigned int crc = SwapInt(CRC::ComputeCrc32(0xEDB88320, &buffer[base_offset], lg));
      memcpy(&buffer[offset + 8], &crc, 4);
      offset += lg;
   }

   // Write DATA
   for (std::vector<DataChunks>::iterator it = list_data_.begin(); it != list_data_.end(); ++it)
   {
      if (it->header.length >= 4)
      {
         // datablock
         int base_offset = offset;
         it->header.crc = 0;
         int lg = it->header.length;

         memcpy(&buffer[offset], &(*it), sizeof(DataChunks) - sizeof(unsigned char*));
         Swap(&buffer[offset + 4], sizeof(DataChunks) - (4+ sizeof(unsigned char*)));
         offset += sizeof(DataChunks) - sizeof(unsigned char*);

         memcpy(&buffer[offset], it->buffer, it->size);
         offset += it->size;

         unsigned int crc = SwapInt(CRC::ComputeCrc32(0xEDB88320, &buffer[base_offset], lg));
         memcpy(&buffer[base_offset + 8], &crc, 4);
      }
      delete[]it->buffer;
      it->buffer = NULL;
   }

   return 0;
}
