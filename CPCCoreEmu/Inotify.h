#pragma once

class IFdcNotify
{
public:

   virtual void ItemLoaded (const char* disk_path,int load_ok, int drive_number) = 0;
   virtual void DiskEject() = 0;
   virtual void DiskRunning(bool on) = 0;
   virtual void TrackChanged(int nb_tracks) = 0;

};
