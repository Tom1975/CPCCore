#pragma once

// Pseudo random numbers of the emulation (weak / optional bits of a disk, speed of a drive...).
// The same suite on every platform : the C library rand() differs (glibc, MSVC), and a test seeded the
// same way gave other draws on Windows. The algorithm is the one of the glibc rand() (additive feedback,
// degree 31, separation 3, seeded by the Park-Miller generator), so the draws do not change on Linux.
namespace EmuRandom
{
   void Seed(unsigned int seed);
   // 0 .. 2^31 - 1
   int Next();
}
