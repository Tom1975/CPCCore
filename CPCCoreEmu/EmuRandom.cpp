#include "EmuRandom.h"

#include <cstdint>

namespace
{
const int kDegree = 31;
const int kSeparation = 3;

struct State
{
   uint32_t table[kDegree];
   int front;    // index of r[i - 3]
   int rear;     // index of r[i - 31]

   explicit State(unsigned int seed) { Init(seed); }

   void Init(unsigned int seed)
   {
      if (seed == 0) seed = 1;
      int32_t r = (int32_t)seed;
      table[0] = (uint32_t)r;
      for (int i = 1; i < kDegree; ++i)
      {
         // 16807 * r % (2^31 - 1), without overflow (Schrage)
         const int32_t hi = r / 127773;
         const int32_t lo = r % 127773;
         r = 16807 * lo - 2836 * hi;
         if (r < 0) r += 2147483647;
         table[i] = (uint32_t)r;
      }
      front = kSeparation;
      rear = 0;
      // The first 310 values are discarded
      for (int i = 0; i < 10 * kDegree; ++i) Step();
   }

   uint32_t Step()
   {
      table[front] += table[rear];
      const uint32_t result = table[front];
      if (++front >= kDegree) front = 0;
      if (++rear >= kDegree) rear = 0;
      return result;
   }
};

State& Generator()
{
   static State state(1);   // as the C library without srand()
   return state;
}
}

namespace EmuRandom
{
void Seed(unsigned int seed)
{
   Generator().Init(seed);
}

int Next()
{
   return (int)(Generator().Step() >> 1);
}
}
