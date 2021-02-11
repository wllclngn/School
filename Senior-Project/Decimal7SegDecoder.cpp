// ********************************************************************************************************
// This program creates the contents for a MC27C160 EEPROM so that it can function as an ALU/flag device.
// ********************************************************************************************************

// ********************************************************************************************************
// Includes
// ********************************************************************************************************

#include <filesystem>
using std::filesystem::path;  using std::filesystem::absolute;

#include <iostream>
using std::cout;  using std::endl;

#include <fstream>
using std::ofstream;

#include <unordered_map>
using std::unordered_map;

#include <vector>
using std::vector;

// ********************************************************************************************************
// Macros
// ********************************************************************************************************

#define Dump(var) #var ": " << var
#define DumpSep(var) ", " #var ": " << var

// ********************************************************************************************************
// Enums
// ********************************************************************************************************

//     A
//  +-----+
//  |     |
//  |F    |B
//  |  G  |
//  +-----+
//  |     |
//  |E    |C
//  |     |
//  +-----+  DP
//     D
enum eSegmentBits : uint32_t
{
  SegA  = 0b1000'0000,
  SegB  = 0b0100'0000,
  SegC  = 0b0010'0000,
  SegD  = 0b0001'0000,
  SegE  = 0b0000'1000,
  SegF  = 0b0000'0100,
  SegG  = 0b0000'0010,
  SegDP = 0b0000'0001,
};
typedef eSegmentBits tSegmentBits;

// Digit to segments lookup table.
unordered_map<char, uint8_t> DigitToSegment
{
  { ' ', 0u },
  { '-', SegG },
  { '0', SegA | SegB | SegC | SegD | SegE | SegF },
  { '1', SegB | SegC },
  { '2', SegA | SegB | SegD | SegE | SegG },
  { '3', SegA | SegB | SegC | SegD | SegG },
  { '4', SegB | SegC | SegF | SegG },
  { '5', SegA | SegC | SegD | SegF | SegG },
  { '6', SegA | SegC | SegD | SegE | SegF | SegG },
  { '7', SegA | SegB | SegC },
  { '8', SegA | SegB | SegC | SegD | SegE | SegF | SegG },
  { '9', SegA | SegB | SegC | SegD | SegF | SegG },
  { 'a', SegA | SegB | SegC | SegE | SegF | SegG },
  { 'A', SegA | SegB | SegC | SegE | SegF | SegG },
  { 'b', SegC | SegD | SegE | SegF | SegG },
  { 'B', SegC | SegD | SegE | SegF | SegG },
  { 'c', SegA | SegD | SegE | SegF },
  { 'C', SegA | SegD | SegE | SegF },
  { 'd', SegB | SegC | SegD | SegE | SegG },
  { 'D', SegB | SegC | SegD | SegE | SegG },
  { 'e', SegA | SegD | SegE | SegF | SegG },
  { 'E', SegA | SegD | SegE | SegF | SegG },
  { 'f', SegA | SegE | SegF | SegG },
  { 'F', SegA | SegE | SegF | SegG },
};

// ********************************************************************************************************
// Types
// ********************************************************************************************************

struct sAddressLines
{
  uint8_t   Byte;
  uint32_t  Digit      : 2;
  uint32_t  Sign       : 1;
  uint32_t  Filler     : 21;
} __attribute__((packed));

typedef sAddressLines tAddressLines;

// ********************************************************************************************************
// Globals
// ********************************************************************************************************

tAddressLines AddressLines;
uint32_t &Address { *((uint32_t *) &AddressLines) };

const auto nBits { 11u };
const auto nBytes { 1u << nBits };

char DisplayString[5];

vector<uint8_t> ROMData;

// ********************************************************************************************************
// Functions
// ********************************************************************************************************

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
  ROMData.resize(nBytes);

  // Cycle through all 2^20 address combinations.
  for (auto Addr = 0u; Addr < nBytes; ++Addr)
  {
    // Pick up current Addr and move to Address for decomposition;
    Address = Addr;

    if (AddressLines.Sign == 1u)
    {
      snprintf(DisplayString, sizeof(DisplayString), "%4d", (int8_t) AddressLines.Byte);
    }
    else
    {
      snprintf(DisplayString, sizeof(DisplayString), "%4u", (uint8_t) AddressLines.Byte);
    }

    auto Char { DisplayString[3 - AddressLines.Digit] };
    if (DigitToSegment.count(Char) == 0u)
    {
      cout << "Mapping failure." << endl;
      exit(__LINE__);
    }

    // Remember this entry.
    ROMData[Addr] = DigitToSegment[Char];
  }

  // ********************************************************************************************************
  // ROM contents defined.  Write the contents to disk.
  // ********************************************************************************************************

  // Write the low order ROM contents.
  ofstream ofs(absolute(argv[1]));
  if (!ofs)
  {
    cout << "Could not open file \"" << argv[1] <<"\" for output." << endl;
    exit(__LINE__);
  }

  ofs.write((const char *) ROMData.data(), ROMData.size() * sizeof(uint8_t));

  ofs.close();

  cout << "Done." << endl;

	return 0;
}
