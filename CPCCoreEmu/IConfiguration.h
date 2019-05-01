#pragma once


class IConfiguration
{
public:
   virtual void SetConfiguration ( const char* section, const char* cle, const char* valeur, const char* file) = 0;
   virtual size_t GetConfiguration (const char* section, const char* cle, const char* default_value, char* out_buffer, size_t buffer_size, const char* file) = 0;
   virtual unsigned int GetConfigurationInt(const char* section, const char* cle, unsigned int default_value, const char* file) = 0;

};