#pragma once


class IConfiguration
{
public:
   virtual void SetConfiguration ( const char* section, const char* cle, const char* valeur, const char* file) = 0;
   virtual unsigned int GetConfiguration (const char* section, const char* cle, const char* default_value, char* out_buffer, unsigned int buffer_size, const char* file) = 0;
   virtual unsigned int GetConfigurationInt(const char* section, const char* cle, unsigned int default_value, const char* file) = 0;

};