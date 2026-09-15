#pragma once

#include <vector>
#include <cstddef>

class EmulatorEngine;
class Motherboard;
class YMZ294;

// A save state for the whole engine, as opposed to a .SNA.
//
// A .SNA is an interchange format: it describes the machine an Amstrad user
// could see, and nothing else. That is not enough to resume emulation exactly
// where it stopped -- the scheduler's per-component cycle debt, the CPU's
// mid-instruction state and the tape and disk positions have no field in it,
// at any version. A state saved here is ours: it may hold whatever the engine
// needs, and it is never written to a file by this class, only to a buffer the
// caller owns.
//
// Layout:
//   "SBXS"        4 bytes
//   version       2 bytes, little endian
//   reserved      2 bytes
//   sna length    4 bytes, little endian
//   sna image     <sna length> bytes
//   chunks        [4 byte id][4 byte length][payload], to the end of the buffer
//
// The .SNA image carries everything the container can express, so this class
// only has to add what it cannot. Chunks are skipped when unknown, so a state
// written by a newer build still loads what an older one understands.
class MachineState
{
public:
   // Bumped when a chunk's payload changes shape. Loading refuses a version it
   // does not know rather than misreading it.
   //
   // Not named VERSION: that is a macro in the Windows SDK headers, which
   // stdafx.h pulls in, and the expansion turns this line into a syntax error
   // on MSVC only.
   static constexpr unsigned short kVersion = 1;

   static bool Save(EmulatorEngine* machine, std::vector<unsigned char>& out);
   static bool Load(EmulatorEngine* machine, const unsigned char* buffer, size_t size);

private:
   // Members rather than free functions: Motherboard grants friendship to this
   // class, and friendship does not extend to anything outside it.
   static void WriteScheduler(Motherboard* board, std::vector<unsigned char>& out);
   static bool ReadScheduler(Motherboard* board, const unsigned char* p, size_t size);
   static void WriteZ80(Motherboard* board, std::vector<unsigned char>& out);
   static bool ReadZ80(Motherboard* board, const unsigned char* p, size_t size);
   static void WriteGateArray(Motherboard* board, std::vector<unsigned char>& out);
   static bool ReadGateArray(Motherboard* board, const unsigned char* p, size_t size);
   static void WritePsg(Motherboard* board, std::vector<unsigned char>& out);
   static bool ReadPsg(Motherboard* board, const unsigned char* p, size_t size);
   static void WriteFdc(Motherboard* board, std::vector<unsigned char>& out);
   static bool ReadFdc(Motherboard* board, const unsigned char* p, size_t size);
   static void WriteDrives(Motherboard* board, std::vector<unsigned char>& out);
   static bool ReadDrives(Motherboard* board, const unsigned char* p, size_t size);
   static void WriteCrtc(Motherboard* board, std::vector<unsigned char>& out);
   static bool ReadCrtc(Motherboard* board, const unsigned char* p, size_t size);
   static void WriteTape(Motherboard* board, std::vector<unsigned char>& out);
   static bool ReadTape(Motherboard* board, const unsigned char* p, size_t size);
   static void WritePpi(Motherboard* board, std::vector<unsigned char>& out);
   static bool ReadPpi(Motherboard* board, const unsigned char* p, size_t size);
   static void WriteYmz(YMZ294* y, std::vector<unsigned char>& out);
   static void ReadYmz(YMZ294* y, const unsigned char* p, size_t& at);
   static void WriteDma(Motherboard* board, std::vector<unsigned char>& out);
   static bool ReadDma(Motherboard* board, const unsigned char* p, size_t size);
   static void WritePlayCity(Motherboard* board, std::vector<unsigned char>& out);
   static bool ReadPlayCity(Motherboard* board, const unsigned char* p, size_t size);
   static void WriteExtendedRam(Motherboard* board, std::vector<unsigned char>& out);
   static bool ReadExtendedRam(Motherboard* board, const unsigned char* p, size_t size);
   // Identity only: written so a load can refuse a different cartridge, and
   // never read back. Nothing about a cartridge is machine state.
   static void WriteCartridge(Motherboard* board, std::vector<unsigned char>& out);

   // Everything a chunk checks about the machine it is being loaded into,
   // verified before anything is written. See the comment on the definition.
   static bool DescribesThisMachine(Motherboard* board,
                                    const unsigned char* buffer, size_t size,
                                    size_t first_chunk);
};
