
#pragma once

class ICfg
{
public:
   virtual void SetDefaultConfiguration () = 0;
   virtual void SaveConfiguration (const char* config_name, const char* ini_file) = 0;
   virtual bool LoadConfiguration (const char* config_name, const char* ini_file) = 0;

};
