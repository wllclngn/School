// ********************************************************************************************************
// This program creates the contents for a MC27C160 EEPROM so that it can function as an ALU/flag device.
// ********************************************************************************************************

// ********************************************************************************************************
// Includes
// ********************************************************************************************************

#include <iostream>
using std::cout;  using std::endl;

#include <fstream>
using std::ofstream;

#include <vector>
using std::vector;

// ********************************************************************************************************
// Macros
// ********************************************************************************************************

#define Dump(var) #var ": " << var
#define DumpSep(var) ", " #var ": " << var

// ********************************************************************************************************
// Types
// ********************************************************************************************************

struct sAddressLines
{
  uint32_t  Ain      : 8;
  uint32_t  Bin      : 8;
  uint32_t  Extended : 1;
  uint32_t  CarryIn  : 1;
  uint32_t  Sub      : 1;
  uint32_t  ZeroIn   : 1;
  uint32_t  Filler1  : 4;
  uint32_t  Filler2  : 8;
} __attribute__((packed));

typedef sAddressLines tAddressLines;

struct sDataLines
{
  uint32_t Sum            : 8;
  uint32_t CarryOut       : 1;
  uint32_t ZeroOut        : 1;
  uint32_t SignedOverflow : 1;
  uint32_t Negative       : 1;
  uint32_t Filler         : 4;
} __attribute__((packed));

typedef sDataLines tDataLines;

// ********************************************************************************************************
// Globals
// ********************************************************************************************************

tAddressLines AddressLines;
uint32_t &Address { *((uint32_t *) &AddressLines) };

tDataLines DataLines;
uint16_t &Data { *((uint16_t *) &DataLines) };

vector<uint16_t> ROMData;

const auto ROMSize { 1u << 20u };

// ********************************************************************************************************
// Functions
// ********************************************************************************************************

void ShowPackedDetails(void)
{
  cout << Dump(sizeof(sAddressLines)) << endl;

  Address = 0x00000001u;
  while (Address)
  {
    cout << Dump(AddressLines.Ain) << DumpSep(AddressLines.Bin) << DumpSep(AddressLines.CarryIn) << DumpSep(AddressLines.Sub) << DumpSep(AddressLines.ZeroIn) << DumpSep(AddressLines.Extended) << DumpSep(AddressLines.Filler1) << DumpSep(AddressLines.Filler2) << endl;
    Address <<= 1u;
  }

  return;
}

void DumpROM(uint32_t n)
{
  for (auto Addr = 0u; Addr < n; ++Addr)
  {
    Address = Addr;
    Data = ROMData[Addr];
    cout << Dump(AddressLines.Ain) << DumpSep(AddressLines.Bin) << DumpSep(AddressLines.CarryIn) << DumpSep(AddressLines.Sub) << DumpSep(AddressLines.ZeroIn) << DumpSep(AddressLines.Extended) <<
        ":\n   " Dump(DataLines.Sum) << DumpSep(DataLines.CarryOut) << DumpSep(DataLines.ZeroOut) << DumpSep(DataLines.SignedOverflow) << DumpSep(DataLines.Negative) << endl;
    Address <<= 1u;
  }

  return;
}

// ********************************************************************************************************
// Main program;
// ********************************************************************************************************

int main(int argc, char *argv[])
{
  // ShowPackedDetails();

  if (argc != 2)
  {
    cout << "Usage:  <Prog> <ROMFile>" << endl;
    exit(__LINE__);
  }

  // Reserve space for ROM contents.
  ROMData.resize(ROMSize);

  // Cycle through all 2^20 address combinations.
  for (auto Addr = 0u; Addr < ROMSize; ++Addr)
  {
    // Pick up current Addr and move to Address for decomposition;
    Address = Addr;

    // ********************************************************************************************************
    // Handle extended/normal add/subtract.  This also sets carry out.
    // ********************************************************************************************************

    if (AddressLines.Sub)
    {
      if (AddressLines.Extended)
      {
        Data = AddressLines.Ain + ~AddressLines.Bin + AddressLines.CarryIn;
      }
      else
      {
        Data = AddressLines.Ain + ~AddressLines.Bin + 1u;
      }
    }
    else
    {
      if (AddressLines.Extended)
      {
        Data = AddressLines.Ain + AddressLines.Bin + AddressLines.CarryIn;
      }
      else
      {
        Data = AddressLines.Ain + AddressLines.Bin;
      }
    }

    // Clear out all but carry out and sum
    Data &= 0x01ffu;

    // ********************************************************************************************************
    // Zero out flag
    // ********************************************************************************************************

    if (AddressLines.Extended)
    {
      DataLines.ZeroOut = ((DataLines.Sum == 0u) && (AddressLines.ZeroIn == 1u)) ? 1u : 0u;
    }
    else
    {
      DataLines.ZeroOut = (DataLines.Sum == 0u) ? 1u : 0u;
    }

    // ********************************************************************************************************
    // Negative flag
    // ********************************************************************************************************

    DataLines.Negative = (DataLines.Sum & 0x80u) ? 1u : 0u;

    // ********************************************************************************************************
    // Signed overflow flag
    // Signed overflow occurs when carry in to the high order stage differs from carry out of the high order stage.
    // ********************************************************************************************************

    // Get sum of high order bits ignoring carry in.
    auto SumHighOrderBiIgnoringCarryIn = ((AddressLines.Ain ^ AddressLines.Bin) & 0x80u) ? 1u : 0u;

    // Get sum of high order bits with carry in.
    auto SumHighOrderBitWithCarryIn = (DataLines.Sum & 0x80u) ? 1u : 0u;

    // If the above two are different there must have been a carry into high order bit.
    auto CarryInToHighOrderBit = (SumHighOrderBiIgnoringCarryIn != SumHighOrderBitWithCarryIn) ? 1u : 0u;

    // Signed overflow occurs when carry into high order bit differs from carry out of high order bit.
    DataLines.SignedOverflow = (CarryInToHighOrderBit != DataLines.CarryOut) ? 1u : 0u;

    // Remember this entry.
    ROMData[Addr] = Data;
  }

  // ********************************************************************************************************
  // ROM contents defined.  Write the contents to disk.
  // ********************************************************************************************************

  ofstream ofs(argv[1]);
  if (!ofs)
  {
    cout << "Could not open file \"" << argv[1] <<"\" for output." << endl;
    exit(__LINE__);
  }

  // Write the ROM contents.
  ofs.write((const char *) ROMData.data(), ROMData.size() * sizeof(uint16_t));

  ofs.close();

  DumpROM(512u);

	return 0;
}
