#pragma once


class IConfiguration
{
public:
   virtual void OpenFile(const char* config_file) = 0;
   virtual void CloseFile() = 0;

   virtual void SetConfiguration(const char* section, const char* key, const char* value, const char* file) = 0;
   virtual void SetConfiguration(const char* section, const char* key, const char* value) = 0;

   virtual unsigned int GetConfiguration(const char* section, const char* cle, const char* default_value, char* out_buffer, unsigned int buffer_size, const char* file) = 0;
   virtual unsigned int GetConfiguration(const char* section, const char* cle, const char* default_value, char* out_buffer, unsigned int buffer_size) = 0;

   virtual unsigned int GetConfigurationInt(const char* section, const char* cle, unsigned int default_value, const char* file) = 0;
   virtual unsigned int GetConfigurationInt(const char* section, const char* cle, unsigned int default_value) = 0;

   // Section number
   virtual const char* GetFirstSection() = 0;
   virtual const char* GetNextSection() = 0;

   // Key
   virtual const char* GetFirstKey(const char* section) = 0;
   virtual const char* GetNextKey() = 0;

};