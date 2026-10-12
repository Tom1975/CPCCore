#include "gtest/gtest.h"

#include "Machine.h"
#include "PPI.h"
#include "Sig.h"

// PPI port B carries the cassette read line on bit 7 and the printer "ready"
// signal on bit 6 ("1" = not ready, see https://cpctech.cpcwiki.de/docs/8255cpc.html).
// A GX4000 has neither a cassette deck nor a printer port, so neither line is
// wired and what the emulated tape or printer does must not show through.
//
// With no printer connected the line is not driven, which reads as not ready;
// the three places that built this byte disagreed about that case.

namespace
{

const unsigned char kPortB = 1;
const unsigned char kControl = 3;
const unsigned char kCassetteBit = 0x80;
const unsigned char kPrinterBit = 0x40;

// Mode 0, port B input: the setting the firmware uses.
void SetUpPortBAsInput(PPI8255& ppi)
{
   unsigned char control = 0x82;
   ppi.DataWrite(&control, kControl);
}

unsigned char ReadPortB(PPI8255& ppi)
{
   unsigned char data = 0;
   ppi.DataRead(&data, kPortB);
   return data;
}

}  // namespace

TEST(Gx4000PortB, NeitherTheCassetteNorThePrinterIsWired)
{
   CSig sig;
   PPI8255 ppi;
   ppi.SetSig(&sig);
   ppi.Reset();
   SetUpPortBAsInput(ppi);
   ppi.SetConsoleWiring(true);

   ppi.tape_level_ = kCassetteBit;  // a tape signal the console cannot have

   const unsigned char port_b = ReadPortB(ppi);
   EXPECT_EQ(0, port_b & kCassetteBit) << "no cassette circuit on a GX4000";
   EXPECT_EQ(kPrinterBit, port_b & kPrinterBit) << "no printer port: the line is not driven";
}

TEST(Gx4000PortB, AComputerStillReadsItsCassette)
{
   CSig sig;
   PPI8255 ppi;
   ppi.SetSig(&sig);
   ppi.Reset();
   SetUpPortBAsInput(ppi);
   ppi.SetConsoleWiring(false);

   ppi.tape_level_ = kCassetteBit;

   EXPECT_EQ(kCassetteBit, ReadPortB(ppi) & kCassetteBit);
}

// Reading the port and writing the control word both rebuild the byte; with no
// printer they used to disagree, one reporting ready and the other not ready.
TEST(Gx4000PortB, WithNoPrinterEveryPathReportsNotReady)
{
   CSig sig;
   PPI8255 ppi;
   ppi.SetSig(&sig);
   ppi.Reset();
   SetUpPortBAsInput(ppi);

   EXPECT_EQ(kPrinterBit, ReadPortB(ppi) & kPrinterBit) << "read";
   EXPECT_EQ(kPrinterBit, ppi.port_b_ & kPrinterBit) << "rebuilt by the control word write";
}

TEST(Gx4000PortB, TheMachineTypeSelectsTheWiring)
{
   // Heap-allocated: sizeof(EmulatorEngine) is megabytes, more than a default
   // thread stack holds (see TestTapeRecording). The machine is not booted
   // here, so hand its PPI the signal bus a boot would have wired.
   EmulatorEngine* machine = new EmulatorEngine();
   CSig sig;
   PPI8255* ppi = machine->GetPPI();
   ppi->SetSig(&sig);

   machine->SetMachineType(MachineSettings::GX400);
   SetUpPortBAsInput(*ppi);
   ppi->tape_level_ = kCassetteBit;
   EXPECT_EQ(0, ReadPortB(*ppi) & kCassetteBit) << "a GX4000 has no cassette";

   machine->SetMachineType(MachineSettings::OLD_6128);
   SetUpPortBAsInput(*ppi);
   ppi->tape_level_ = kCassetteBit;
   EXPECT_EQ(kCassetteBit, ReadPortB(*ppi) & kCassetteBit) << "a 6128 has one";

   delete machine;
}
