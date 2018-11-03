#pragma once

#include <vector>

class CRC
{
public:
   CRC(void);
   virtual ~CRC(void);

   void Reset();
   void AddByteToCrc(unsigned char b);
   unsigned short GetCRC() { return computed_crc_; };

   // Other CRC utilities
   static unsigned int ComputeCrc32(unsigned int polynomial, const unsigned char* buffer, unsigned int size);


protected:
   unsigned short computed_crc_;
   int count_;


   std::vector<unsigned short> crc_stack_;
};
