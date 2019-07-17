#include "stdafx.h"
#include "FormatTypeHFEv3.h"
#include "simple_stdio.h"

#define IBMPC_DD_FLOPPYMODE 0x00
#define IBMPC_HD_FLOPPYMODE 0x01
#define ATARIST_DD_FLOPPYMODE 0x02
#define ATARIST_HD_FLOPPYMODE 0x03
#define AMIGA_DD_FLOPPYMODE 0x04
#define AMIGA_HD_FLOPPYMODE 0x05
#define CPC_DD_FLOPPYMODE 0x06
#define GENERIC_SHUGGART_DD_FLOPPYMODE 0x07
#define IBMPC_ED_FLOPPYMODE 0x08
#define MSX2_DD_FLOPPYMODE 0x09
#define C64_DD_FLOPPYMODE 0x0A
#define EMU_SHUGART_FLOPPYMODE 0x0B
#define S950_DD_FLOPPYMODE 0x0C
#define S950_HD_FLOPPYMODE 0x0D
#define DISABLE_FLOPPYMODE 0xFE

#define ISOIBM_MFM_ENCODING 0x00
#define AMIGA_MFM_ENCODING 0x01
#define ISOIBM_FM_ENCODING 0x02
#define EMU_FM_ENCODING 0x03
#define UNKNOWN_ENCODING 0xFF

#define OPCODE_MASK        0x0F

#define NOP_OPCODE         0x0F
#define SETINDEX_OPCODE    0x8F
#define SETBITRATE_OPCODE  0x4F
#define SKIPBITS_OPCODE    0xCF
#define RAND_OPCODE        0x2F

typedef struct Picfileformatheader
{
   unsigned char header_signature[8]; // “HXCPICFE”
   unsigned char formatrevision; // Revision 0
   unsigned char number_of_track; // Number of track in the file
   unsigned char number_of_side; // Number of valid side (Not used by the emulator)
   unsigned char track_encoding; // Track Encoding mode
   // (Used for the write support - Please see the list above)
   unsigned short bit_rate; // Bitrate in Kbit/s. Ex : 250=250000bits/s
   // Max value : 500
   unsigned short floppy_rpm; // Rotation per minute (Not used by the emulator)
   unsigned char floppyinterfacemode; // Floppy interface mode. (Please see the list above.)
   unsigned char dnu; // Free
   unsigned short track_list_offset; // Offset of the track list LUT in block of 512 bytes
   // (Ex: 1=0x200)
   unsigned char write_allowed; // The Floppy image is write protected ?
   unsigned char single_step; // 0xFF : Single Step – 0x00 Double Step mode
   unsigned char track0_s0_altencoding; // 0x00 : Use an alternate track_encoding for track 0 Side 0
   unsigned char track0_s0_encoding; // alternate track_encoding for track 0 Side 0
   unsigned char track0_s1_altencoding; // 0x00 : Use an alternate track_encoding for track 0 Side 1
   unsigned char track0_s1_encoding; // alternate track_encoding for track 0 Side 1
} Picfileformatheader;

typedef struct 
{
   unsigned short offset; // Offset of the track data in block of 512 bytes (Ex: 2=0x400)
   unsigned short track_len; // Length of the track data in byte.
} Pictrack;


FormatTypeHFEv3::FormatTypeHFEv3()
{
}

FormatTypeHFEv3::~FormatTypeHFEv3()
{
}

const char* FormatTypeHFEv3::GetFormatName()
{
   return "HFEv3";
}

const char* FormatTypeHFEv3::GetFormatDescriptor()
{
   return "HFE v3";
}

const char* FormatTypeHFEv3::GetFormatExt()
{
   return "hfe";
}

bool FormatTypeHFEv3::CanLoad() const
{
   return true;
}

bool FormatTypeHFEv3::CanSave() const
{
   return true;
}

bool FormatTypeHFEv3::CanLoad(const char* file_path)
{
   // Check HXCPICFE version 
   FILE* file;
   bool can_load = false;

   if (fopen_s(&file, file_path, "rb") == 0)
   {
      // Check for type of file from header
      unsigned char header[0x8] = {0};
      // check the 22 first char
      fread(header, 1, 0x8, file);
      if (memcmp(header, "HXCHFEV3", 8) == 0)
      {
         can_load = true;
      }
      fclose(file);
   }
   return can_load;
}

int FormatTypeHFEv3::LoadDisk(const char* file_path, IDisk*& created_disk, ILoadingProgress* loading_progress)
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
         return_value = FILE_ERROR;
      }
      delete[]buffer;

      fclose(file);
   }
   else
   {
      // Erreur : File not found
      return_value = FILE_ERROR;
   }
   return return_value;
}

int FormatTypeHFEv3::LoadDisk(const unsigned char* buffer, size_t size, IDisk*& created_disk,
                            ILoadingProgress* loading_progress)
{
   if (memcmp(buffer, "HXCHFEV3", 8) == 0)
   {
      IDisk* new_disk = new IDisk();

      // Header
      int index_buffer = 0;

      Picfileformatheader header;
      //fread(&header, 1, sizeof(header), file);
      memcpy(&header, buffer, sizeof(header));
      index_buffer += sizeof(header);

      // Number of sides
      new_disk->nb_sides_ = header.number_of_side;

      for (int side = 0; side < new_disk->nb_sides_; side++)
      {
         new_disk->side_[side].nb_tracks = header.number_of_track;
         new_disk->side_[side].tracks = new IDisk::MFMTrack[new_disk->side_[side].nb_tracks];
         memset(new_disk->side_[side].tracks, 0, sizeof(IDisk::MFMTrack) * new_disk->side_[side].nb_tracks);
      }

      //fseek(file, 0x200, SEEK_SET);
      index_buffer = 0x200;

      Pictrack* pck_trk = new Pictrack[header.number_of_track];
      //fread(pck_trk, 1, sizeof(pck_trk) * header.number_of_track, file);
      memcpy(pck_trk, &buffer[index_buffer], sizeof(Pictrack) * header.number_of_track);
      index_buffer += sizeof(pck_trk) * header.number_of_track;

      // Second part : (up to 1024 bytes) : Track offset LUT
      unsigned char* buffer1;
      unsigned char* buffer2;
      for (int track = 0; track < header.number_of_track; track++)
      {
         // Track data
         //fseek(file, 0x200 * pck_trk[track].offset, SEEK_SET);
         index_buffer = 0x200 * pck_trk[track].offset;

         // Read bytes
         buffer1 = new unsigned char[pck_trk[track].track_len / 2];
         buffer2 = new unsigned char[pck_trk[track].track_len / 2];

         int byte_to_read = pck_trk[track].track_len / 2;
         unsigned char buf[0x200];
         int index = 0;
         while (byte_to_read > 0)
         {
            //fread(buf, 1, 0x200, file);
            memcpy(buf, &buffer[index_buffer], 0x200);
            index_buffer += 0x200;

            unsigned int cnt = (byte_to_read < 0x100) ? byte_to_read : 0x100;

            memcpy(&buffer1[index], buf, cnt);
            memcpy(&buffer2[index], &buf[0x100], cnt);

            byte_to_read -= cnt;
            index += cnt;
         }


         for (int side = 0; side < new_disk->nb_sides_; side++)
         {
            new_disk->side_[side].tracks[track].size = pck_trk[track].track_len * 4;
            new_disk->side_[side].tracks[track].bitfield = new unsigned char[new_disk->side_[side].
               tracks[track].size];

            // TODO : Multiple side
            unsigned char* buffer_in = ((side == 0) ? buffer1 : buffer2);
            unsigned int buffer_in_counter = 0;
            unsigned int buffer_out_counter = 0;

            while(   buffer_out_counter < new_disk->side_[side].tracks[track].size 
                  && buffer_in_counter < pck_trk[track].track_len/2)
            {
               // Read next byte
               // Check tag ?
               //unsigned char* tag_buffer = &(((side == 0) ? buffer1 : buffer2)[i >> 3]);

               if ((buffer_in[buffer_in_counter] & OPCODE_MASK) == OPCODE_MASK)
               {
                  switch (buffer_in[buffer_in_counter])
                  {
                     case NOP_OPCODE:        // Nothing to do : just don't use this special opcode
                        buffer_in_counter++;
                        break;
                     case SETINDEX_OPCODE:   // Index is here !
                        new_disk->side_[side].tracks[track].bitfield[buffer_out_counter] |= BIT_INDEX;
                        buffer_in_counter++;
                        break;
                     case SETBITRATE_OPCODE: // to implement ?
                        buffer_in_counter+=2;
                        break;
                     case SKIPBITS_OPCODE:
                        {
                        unsigned int nb_bit_to_skip = buffer_in[buffer_in_counter+1];
                        for (int i = nb_bit_to_skip; i < 8; i++)
                           new_disk->side_[side].tracks[track].bitfield[buffer_out_counter++] = buffer_in[buffer_in_counter+2] >> ((i & 7)) & 1;
                        buffer_in_counter += 3;
                        break;
                        
                        }
                     case RAND_OPCODE:
                        for (int i = 0; i < 8; i++)
                           new_disk->side_[side].tracks[track].bitfield[buffer_out_counter++] |= BIT_WEAK;
                        buffer_in_counter ++;

                        break;
                     default:
                     {
                        int dbg = 1;
                        break;
                     }
                  }

               }
               else
               {
                  for (int i = 0; i < 8; i++)
                     new_disk->side_[side].tracks[track].bitfield[buffer_out_counter++] = buffer_in[buffer_in_counter] >> ((i & 7)) & 1;
                  buffer_in_counter++;
               }
            }
         }
         delete[]buffer1;
         delete[]buffer2;
      }

      delete[]pck_trk;

      created_disk = new_disk;
      return OK;
   }
   else
   {
      return FILE_ERROR;
   }
}

int FormatTypeHFEv3::SaveDisk(const char* file_path, IDisk* disk, ILoadingProgress* loading_progress) const
{
   int ret = 1;

   FILE* file;
   int res = disk->SmartOpen(&file, file_path, ".HFE");

   if (res == 0)
   {
      // First part : Header
      unsigned char buffer[0x200];
      memset(buffer, 0xFF, sizeof(buffer));

      Picfileformatheader header = {0xFF};
      memcpy(header.header_signature, "HXCHFEV3", sizeof(header.header_signature));
      header.formatrevision = 0;
      header.number_of_track = disk->side_[0].nb_tracks;
      header.number_of_side = disk->nb_sides_;
      header.track_encoding = ISOIBM_MFM_ENCODING;
      header.bit_rate = 250;
      header.floppy_rpm = 300;
      header.dnu = 1;
      header.floppyinterfacemode = CPC_DD_FLOPPYMODE;
      header.track_list_offset = 1; // Offset of the track list = 512
      header.write_allowed = 0xFF;
      header.single_step = 0xFF; // Single step ? TODO : To check !!
      header.track0_s0_altencoding = 0xFF;
      header.track0_s0_encoding = 0xFF;
      header.track0_s1_altencoding = 0xFF;
      header.track0_s1_encoding = 0xFF;

      memcpy(buffer, &header, sizeof(header));
      fwrite(&buffer, 1, sizeof(buffer), file);

      // Second part : Track offset LUT
      Pictrack* pck_trk = new Pictrack[header.number_of_track];
      memset(pck_trk, 0, sizeof(Pictrack) * header.number_of_track);

      memset(buffer, 0xFF, sizeof(buffer));

      fwrite(buffer, 0x200, 1, file);

      // Third part : Track data
      for (unsigned int track = 0; track < header.number_of_track; track++)
      {
         // Where are we ? Set the track offset
         const int post_current = ftell(file);
         pck_trk[track].offset = static_cast<unsigned short>(post_current / 0x200);
         int byte_to_write = (disk->side_[0].tracks[track].size + 7) / 8;
         if (disk->side_[1].nb_tracks > track)
         {
            if (disk->side_[1].tracks[track].size > disk->side_[0].tracks[track].size)
               byte_to_write = (disk->side_[1].tracks[track].size + 7) / 8;
         }
         byte_to_write *= 2;

         pck_trk[track].track_len = byte_to_write;

         unsigned int index = 0;
         while (byte_to_write > 0)
         {
            memset(buffer, 0, 0x200);

            // First track
            unsigned int cnt = (byte_to_write < 0x100) ? byte_to_write : 0x100;

            unsigned char b = 0;
            unsigned char numb = 0;

            int tmpindex[2];
            tmpindex[0] = tmpindex[1] = index;
            for (int s = 0; s < 2; s++)
            {
               index = tmpindex[s];
               int numbtowr = 0;
               if (s < disk->nb_sides_ && track < disk->side_[s].nb_tracks)
               {
                  for (unsigned int i = index; i < index + 0x800 && i < disk->side_[s].tracks[track].size; i++)
                  {
                     // Index : TODO
                     if (disk->side_[s].tracks[track].bitfield[i] & BIT_INDEX)
                     {
                        // TODO ?
                     }

                     if (disk->side_[s].tracks[track].bitfield[i] & BIT_WEAK)
                     {
                        
                        // TODO ?
                     }
                     else
                     {
                        b |= ((disk->side_[s].tracks[track].bitfield[i]) << numb++);
                     }

                     if (numb == 8)
                     {
                        buffer[(0x100 * s) + numbtowr++] = b;
                        numb = 0;
                        b = 0;
                     }
                  }
               }
               tmpindex[s] += 0x800;
            }
            index = (tmpindex[0] > tmpindex[1] ? tmpindex[0] : tmpindex[1]);

            // Write 0x200 bytes
            fwrite(buffer, 1, 0x200, file);

            byte_to_write -= cnt;
         }
      }

      // Write now the LUT
      fseek(file, 0x200, SEEK_SET);
      fwrite(pck_trk, sizeof(pck_trk) * header.number_of_track, 1, file);
      delete[]pck_trk;
      ret = 0;

      fclose(file);
   }
   return ret;
}

int FormatTypeHFEv3::LoadDisk(std::vector<TrackItem> file_list, IDisk*& created_disk, ILoadingProgress* loading_progress)
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
