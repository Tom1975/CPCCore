#include "CRC.h"


CRC::CRC() : computed_crc_(0xFFFF)
{
   count_ = 0;
}


CRC::~CRC(void)
{
}

void CRC::Reset()
{
   computed_crc_ = 0xFFFF;
   count_ = 0;
}

// CRC-CCITT (0xFFFF)
void CRC::AddByteToCrc(unsigned char b)
{
   count_++;
   for (int cpt_crc = 0; cpt_crc < 8; cpt_crc++)
   {
      int bit = (b >> (7 - cpt_crc) & 0x1);

      if ((computed_crc_ ^ (bit ? 0x8000 : 0x0000)) & 0x8000)
         computed_crc_ = ((computed_crc_ << 1) ^ 0x1021);
      else
         computed_crc_ = computed_crc_ << 1;
   }
}

unsigned int CRC::ComputeCrc32(unsigned int polynomial, const unsigned char* buffer, unsigned int size)
{
   // Compute the array
   int crc_array [ 256 ];

   for (int i = 0; i < 256; i++)
   {
      unsigned int crc = i;
      for (int b = 0; b < 8; b++)
      {
         if ((crc & 1) == 1)
         {
            crc = (polynomial ^ (crc >> 1));
         }
         else
         {
            crc >>= 1;
         }
      }
      crc_array[i] = crc;
   }

   unsigned int crc = ~0;

   while (size--)
   {
      crc = crc_array[(crc ^ *buffer++) & 0xff] ^ (crc >> 8);
   }
   return crc ^ ~0;
}
