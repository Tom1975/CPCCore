#include "stdafx.h"
#include "FormatTypeIPF.h"
#include "CAPSFile.h"
#include <stdio.h>

FormatTypeIPF::FormatTypeIPF()
{
}

FormatTypeIPF::~FormatTypeIPF()
{
}

bool FormatTypeIPF::CanLoad(const char* file_path)
{
   // Check HXCPICFE version 
   FILE* file;
   bool can_load = false;

   if (fopen_s(&file, file_path, "rb") == 0)
   {
      // Check for type of file from header
      unsigned char header[0x3] = {0};
      // check the 22 first char
      fread(header, 1, 0x3, file);
      if (memcmp(header, "CAPS", 3) == 0)
      {
         can_load = true;
      }
      fclose(file);
   }
   return can_load;
}

int FormatTypeIPF::LoadDisk(const char* file_path, IDisk*& created_disk, ILoadingProgress* loading_progress)
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

int FormatTypeIPF::LoadDisk(const unsigned char* buffer, size_t size, IDisk*& created_disk,
                            ILoadingProgress* loading_progress)
{
   CAPSFile caps_file;
   created_disk = new IDisk();
   if (caps_file.ReadBuffer(buffer, size, created_disk, loading_progress) == 0)
   {
      return OK;
   }
   else
   {
      delete created_disk;
      created_disk = nullptr;
      return FILE_ERROR;
   }
}

int FormatTypeIPF::SaveDisk(const char* file_path, IDisk* disk, ILoadingProgress* loading_progress) const
{
   FILE* file;
   int res = disk->SmartOpen(&file, file_path, ".IPF");

   if (res == 0)
   {
      CAPSFile caps_file;

      // Create IPF structures
      caps_file.CreateIpf(disk);

      // Create buffer
      int size = caps_file.ComputeSize();
      unsigned char* buffer = new unsigned char[size];

      caps_file.WriteToBuffer(buffer, size);

      fwrite(buffer, 1, size, file);
      delete[] buffer;

      fclose(file);

      return OK;
   }
   return FILE_ERROR;
}

int FormatTypeIPF::LoadDisk(std::vector<TrackItem> file_list, IDisk*& created_disk, ILoadingProgress* loading_progress)
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
