// ********************************************************************************************************
// This program creates the contents for a MC27C160 EEPROM so that it can function as an ALU/flag device.
// ********************************************************************************************************

// ********************************************************************************************************
// Includes
// ********************************************************************************************************

#include <cstdarg>

#include <filesystem>
using std::filesystem::path;  using std::filesystem::absolute;

#include <iostream>
using std::cout;  using std::endl;

#include <fstream>
using std::ofstream;

#include <string>
using std::string;

#include <vector>
using std::vector;

#define DumpVar(var) #var ": " << var
#define DumpVarSep(var) ", " #var ": " << var

// ******************************************************************************************************
// This function implements a string printf function.  It's name and functionality matches the
// function implemented in the C++20 include <format>.
// ******************************************************************************************************

string
format(const string Format, ...)
{
  va_list ap;

  // First get number of characters needed (less terminating null).
  va_start(ap, Format);
  auto nChars = vsnprintf(nullptr, 0u, Format.c_str(), ap);
  va_end(ap);

  // Allocate buffer, including room for null.
  vector<char> Buffer(nChars + 1);

  // Print for real this time.
  va_start(ap, Format);
  nChars = vsnprintf(Buffer.data(), Buffer.size(), Format.c_str(), ap);
  va_end(ap);

  // Return resulting string.
  return string(Buffer.data());
}

// ********************************************************************************************************
// Macros
// ********************************************************************************************************

#define Dump(var) #var ": " << var
#define DumpSep(var) ", " #var ": " << var

// ********************************************************************************************************
// Enums
// ********************************************************************************************************

enum eOpCodes : uint32_t
{
  OpCodeSKIP = 0b0000u,
  OpCodeLDA  = 0b0001u,
  OpCodeSTA  = 0b0010u,
  OpCodeADD  = 0b0011u,
  OpCodeSUB  = 0b0100u,
  OpCodeADC  = 0b0101u,
  OpCodeSBC  = 0b0110u,
  OpCodeOUT  = 0b0111u,
  OpCodeJMP  = 0b1000u,
  OpCode1001 = 0b1001u,
  OpCode1010 = 0b1010u,
  OpCode1011 = 0b1011u,
  OpCode1100 = 0b1100u,
  OpCode1101 = 0b1101u,
  OpCode1110 = 0b1110u,
  OpCode1111 = 0b1111u,
};
typedef eOpCodes tOpCodes;

enum eInstructionStates : uint32_t
{
  T0 = 0,
  T1 = 1,
  T2 = 2,
  T3 = 3,
  T4 = 4,
  T5 = 5,
  T6 = 6,
  T7 = 7,
  T8 = 8,
};
typedef eInstructionStates tInstructionStates;

// Skip instruction inspired by Intel 8080 flags and IBM System/360 jump encoding strategy.
enum eSkipTests : uint32_t
{
  SkipNOP     = 0b0000, // NOP is never skip.
  SkipHALT    = 0b0001, // HALT instruction encoded into skip to save opcode.
  OutA        = 0b0010, // Display contents of A-register.

  // The rest of the encoding is as follows:

//Opcode        Code          Flag  Description
//------        ------        ----  ----------------------------------------
  SkipZ       = 0b0011,   //  Z     Skip if Z flag set.
  SkipNZ      = 0b0100,   //  Z     Skip if Z flag not set.
  SkipC       = 0b0101,   //  C     Skip if C flag set.
  SkipNC      = 0b0110,   //  C     Skip if C flag not set.
  SkipV       = 0b0111,   //  V     Skip if V (signed overflow) flag set.
  SkipNV      = 0b1000,   //  V     Skip if V flag not set.
  SkipLT      = 0b1001,   //  N     Skip if N flag set.
  SkipLE      = 0b1010,   //  N,Z   Skip if N or Z flags set.
  SkipGT      = 0b1011,   //  N,Z   Skip if neither N nor Z flags set.
  SkipGE      = 0b1100,   //  N     Skip if N flag not set.
  Skip1101    = 0b1101,   //        No encoding.
  Skip1110    = 0b1110,   //        No encoding.
  Skip1111    = 0b1111,   //        No encoding.
};

// ********************************************************************************************************
// Types
// ********************************************************************************************************

struct sAddressLines
{
  uint32_t  VF         : 1;
  uint32_t  NF         : 1;
  uint32_t  ZF         : 1;
  uint32_t  CF         : 1;
  uint32_t  Operand    : 4;
  uint32_t  InstState  : 8;
  uint32_t  OpCode     : 4;
  uint32_t  Filler     : 12;
} __attribute__((packed));

typedef sAddressLines tAddressLines;

struct sDataLines
{
  // Low order 16-bits
  uint32_t NextInstState  : 8;
  uint32_t Filler         : 7;
  uint32_t EX             : 1;

  // High order 16-bits
  uint32_t FI             : 1;
  uint32_t J              : 1;
  uint32_t CO             : 1;
  uint32_t CE             : 1;
  uint32_t OI             : 1;
  uint32_t BI             : 1;
  uint32_t SU             : 1;
  uint32_t EO             : 1;
  uint32_t AO             : 1;
  uint32_t AI             : 1;
  uint32_t II             : 1;
  uint32_t IO             : 1;
  uint32_t RO             : 1;
  uint32_t RI             : 1;
  uint32_t MI             : 1;
  uint32_t HLT            : 1;
} __attribute__((packed));

// Initial
//uint32_t HLT            : 1;
//uint32_t MI             : 1;
//uint32_t RI             : 1;
//uint32_t RO             : 1;
//uint32_t IO             : 1;
//uint32_t II             : 1;
//uint32_t AI             : 1;
//uint32_t AO             : 1;
//uint32_t EO             : 1;
//uint32_t SU             : 1;
//uint32_t BI             : 1;
//uint32_t DI             : 1;
//uint32_t CE             : 1;
//uint32_t CO             : 1;
//uint32_t J              : 1;
//uint32_t FI             : 1;
//uint32_t EX             : 1;
//uint32_t Filler         : 7;
//uint32_t T              : 8;


typedef sDataLines tDataLines;

// ********************************************************************************************************
// Globals
// ********************************************************************************************************

tAddressLines AddressLines;
uint32_t &Address { *((uint32_t *) &AddressLines) };

tDataLines DataLines;
uint32_t &Data { *((uint32_t *) &DataLines) };

vector<uint32_t> ROMData;

// ********************************************************************************************************
// Functions
// ********************************************************************************************************

inline void InstFetchT0(void)
{
  // Auto inc PC.
  DataLines.CE = 1u;

  // PC to Bus.
  DataLines.CO = 1u;

  // Bus to MAR.
  DataLines.MI = 1u;

  return;
}

inline void InstFetchT1(void)
{
  // Memory to Bus
  DataLines.RO = 1u;

  // Load IR.
  DataLines.II = 1u;

  return;
}

inline void SetInstructionFetchNextState(void)
{
  DataLines.NextInstState = 0u;

  return;
}

inline void SetNextState(void)
{
  DataLines.NextInstState = AddressLines.InstState + 1u;

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
  ROMData.resize(1 << 20u);

  cout << DumpVar(sizeof(tAddressLines)) << DumpVarSep(sizeof(tDataLines)) << endl;

  // Cycle through all 2^20 address combinations.
  for (auto Addr = 0u; Addr < 1u << 20u; ++Addr)
  {
  // Pick up current Addr and move to Address for decomposition;
  Address = Addr;

  // Initialize all data lines.
  Data = 0u;

  // Instruction fetch (T0 abd T1) is common to all instructions.
  switch (AddressLines.InstState)
  {
    case T0:
    {
      InstFetchT0();
      break;
    }

    case T1:
    {
      InstFetchT1();
      break;
    }
  }

  // Assume next state.  Last step for each opcode must change this first state (T0) by call SetFirstState().
  SetNextState();

  switch (AddressLines.OpCode)
  {
    case OpCodeSKIP:
    {
      // Micro instructions for HALT opcode.
      switch (AddressLines.InstState)
      {
        case T0:
        case T1:
          break;

        case T2:
        {
          switch (AddressLines.Operand)
          {
            case SkipNOP:
            {
              // Nothing to do.  Just fetch next instruction.
              SetInstructionFetchNextState();

              break;
            }

            case SkipHALT:
            {
              // Halt clock.
              DataLines.HLT = 1;

              // Freeze state.
              DataLines.NextInstState = AddressLines.InstState;

              break;
            }

            case OutA:
            {
              // Register A drives the bus.
              DataLines.AO = 1u;

              // Output register receives data.
              DataLines.OI = 1u;

              // Next state is instruction fetch.
              SetInstructionFetchNextState();

              break;
            }

//          Opcode  Code    Flag  Description
//          ------  ------  ----  ----------------------------------------
//          SkipZ   0b0010  Z     Skip if Z flag set.
//          SkipNZ  0b0011  Z     Skip if Z flag not set.
//          SkipC   0b0100  C     Skip if C flag set.
//          SkipNC  0b0101  C     Skip if C flag not set.
//          SkipV   0b0110  V     Skip if V (signed overflow) flag set.
//          SkipNV  0b0111  V     Skip if V flag not set.
//          SkipLT  0b1000  N     Skip if N flag set.
//          SkipLE  0b1001  N,Z   Skip if N or Z flags set.
//          SkipGT  0b1010  N,Z   Skip if neither N nor Z flags set.
//          SkipGE  0b1011  N     Skip if N flag not set.

            case SkipZ:
            {
              // Increment PC (i.e., skip) if Z flag set.
              DataLines.CE = (AddressLines.ZF == 1u) ? 1u : 0u;

              // Fetch next instruction.
              SetInstructionFetchNextState();

              break;
            }

            case SkipNZ:
            {
              // Increment PC (i.e., skip) if Z flag not set.
              DataLines.CE = (AddressLines.ZF == 0u) ? 1u : 0u;

              // Fetch next instruction.
              SetInstructionFetchNextState();

              break;
            }

            case SkipC:
            {
              // Increment PC (i.e., skip) if C flag set.
              DataLines.CE = (AddressLines.CF == 1u) ? 1u : 0u;

              // Fetch next instruction.
              SetInstructionFetchNextState();

              break;
            }

            case SkipNC:
            {
              // Increment PC (i.e., skip) if C flag not set.
              DataLines.CE = (AddressLines.CF == 0u) ? 1u : 0u;

              // Fetch next instruction.
              SetInstructionFetchNextState();

              break;
            }

            case SkipV:
            {
              // Increment PC (i.e., skip) if V flag set.
              DataLines.CE = (AddressLines.VF == 1u) ? 1u : 0u;

              // Fetch next instruction.
              SetInstructionFetchNextState();

              break;
            }

            case SkipNV:
            {
              // Increment PC (i.e., skip) if V flag not set.
              DataLines.CE = (AddressLines.VF == 0u) ? 1u : 0u;

              // Fetch next instruction.
              SetInstructionFetchNextState();

              break;
            }

            case SkipLT:
            {
              // Increment PC (i.e., skip) if N flag set.
              DataLines.CE = (AddressLines.NF == 1u) ? 1u : 0u;

              // Fetch next instruction.
              SetInstructionFetchNextState();

              break;
            }

            case SkipLE:
            {
              // Increment PC (i.e., skip) if N or Z flag set.
              DataLines.CE = ((AddressLines.NF == 1u) || (AddressLines.ZF == 1u)) ? 1u : 0u;

              // Fetch next instruction.
              SetInstructionFetchNextState();

              break;
            }

            case SkipGT:
            {
              // Increment PC (i.e., skip) if N or Z flag set.
              DataLines.CE = ((AddressLines.NF == 0u) && (AddressLines.ZF == 0u)) ? 1u : 0u;

              // Fetch next instruction.
              SetInstructionFetchNextState();

              break;
            }

            case SkipGE:
            {
              // Increment PC (i.e., skip) if N not flag set.
              DataLines.CE = (AddressLines.NF == 0u) ? 1u : 0u;

              // Fetch next instruction.
              SetInstructionFetchNextState();

              break;
            }

            default:
            {
              // Halt on bad encoding.
              // Halt clock.
              DataLines.HLT = 1;

              // Freeze state.
              DataLines.NextInstState = AddressLines.InstState;

              break;
            }
          }
          break;

          default:
          {
            // Halt on bad encoding.
            // Halt clock.
            DataLines.HLT = 1;

            // Freeze state.
            DataLines.NextInstState = AddressLines.InstState;

            break;
          }
        }

        break;
      }

      break;
    }

    case OpCodeLDA:
    {
      // Micro instructions for LDA opcode.
      switch (AddressLines.InstState)
      {
        case T0:
        case T1:
          break;

        case T2:
        {
          // Place IR operand on data bus.
          DataLines.IO = 1u;

          // Load MAR from data bus.
          DataLines.MI = 1u;

          break;
        }

        case T3:
        {
          // Place RAM on data bus;
          DataLines.RO = 1u;

          // Load register A from data bus.
          DataLines.AI = 1u;

          // Fetch next instruction.
          SetInstructionFetchNextState();

          break;
        }

        default:
        {
          // Halt clock.
          DataLines.HLT = 1;

          // Freeze state.
          DataLines.NextInstState = AddressLines.InstState;

          break;
        }
      }

      break;
    }

    case OpCodeSTA:
    {
      // STA  7
      switch (AddressLines.InstState)
      {
        case T0:
        case T1:
          break;

        case T2:
        {
          // Place IR operand on data bus.
          DataLines.IO = 1u;

          // Load MAR from data bus.
          DataLines.MI = 1u;

          break;
        }

        case T3:
        {
          // Place register A on data bus;
          DataLines.AO = 1u;

          // Load RAM from data bus.
          DataLines.RI = 1u;

          // Fetch next instruction.
          SetInstructionFetchNextState();

          break;
        }

        default:
        {
          // Halt clock.
          DataLines.HLT = 1;

          // Freeze state.
          DataLines.NextInstState = AddressLines.InstState;

          break;
        }
      }

      break;
    }

    case OpCodeADD:
    {
      // ADD 7  Add contenst of memory location 7 to A-Reg
      // Move operand address (low-order of IR) to MAR
      // Read from Memory store to B-reg
      // Add then store to A-reg
      switch (AddressLines.InstState)
      {
        case T0:
        case T1:
          break;

        case T2:
        {
          DataLines.IO = 1u;
          DataLines.MI = 1u;

          break;
        }

        case T3:
        {
          DataLines.RO = 1u;
          DataLines.BI =1u;
          break;
        }

        case T4:
        {
          DataLines.EO = 1u;
          DataLines.AI = 1u;
          DataLines.FI = 1u;
          DataLines.SU = 0u;
          DataLines.EX = 0u;
          SetInstructionFetchNextState();

          break;
        }

        default:
        {
          // Halt clock.
          DataLines.HLT = 1;

          // Freeze state.
          DataLines.NextInstState = AddressLines.InstState;

          break;
        }
      }
      break;
    }

    case OpCodeSUB:
    {
      switch (AddressLines.InstState)
      {
        case T0:
        case T1:
          break;

        case T2:
        {
          DataLines.IO = 1u;
          DataLines.MI = 1u;

          break;
        }

        case T3:
        {
          DataLines.RO = 1u;
          DataLines.BI =1u;
          break;
        }

        case T4:
        {
          DataLines.EO = 1u;
          DataLines.AI = 1u;
          DataLines.FI = 1u;
          DataLines.SU = 1u;
          DataLines.EX = 0u;
          SetInstructionFetchNextState();

          break;
        }

        default:
        {
          // Halt clock.
          DataLines.HLT = 1;

          // Freeze state.
          DataLines.NextInstState = AddressLines.InstState;

          break;
        }
      }
      break;
    }

    case OpCodeADC:
    {
      switch (AddressLines.InstState)
      {
        case T0:
        case T1:
          break;

        case T2:
        {
          DataLines.IO = 1u;
          DataLines.MI = 1u;

          break;
        }

        case T3:
        {
          DataLines.RO = 1u;
          DataLines.BI =1u;
          break;
        }

        case T4:
        {
          DataLines.EO = 1u;
          DataLines.AI = 1u;
          DataLines.FI = 1u;
          DataLines.SU = 0u;
          DataLines.EX = 1u;
          SetInstructionFetchNextState();

          break;
        }

        default:
        {
          // Halt clock.
          DataLines.HLT = 1;

          // Freeze state.
          DataLines.NextInstState = AddressLines.InstState;

          break;
        }
      }
      break;
    }

    case OpCodeSBC:
    {
      switch (AddressLines.InstState)
      {
        case T0:
        case T1:
          break;

        case T2:
        {
          DataLines.IO = 1u;
          DataLines.MI = 1u;

          break;
        }

        case T3:
        {
          DataLines.RO = 1u;
          DataLines.BI =1u;
          break;
        }

        case T4:
        {
          DataLines.EO = 1u;
          DataLines.AI = 1u;
          DataLines.FI = 1u;
          DataLines.SU = 1u;
          DataLines.EX = 1u;
          SetInstructionFetchNextState();

          break;
        }

        default:
        {
          // Halt clock.
          DataLines.HLT = 1;

          // Freeze state.
          DataLines.NextInstState = AddressLines.InstState;

          break;
        }
      }
      break;
    }

    case OpCodeOUT:
    {
      // Micro instructions for OUT opcode.
      switch (AddressLines.InstState)
      {
        case T0:
        case T1:
          break;

        case T2:
        {
          // Place IR operand on data bus.
          DataLines.IO = 1u;

          // Load MAR from data bus.
          DataLines.MI = 1u;

          break;
        }

        case T3:
        {
          // Place RAM on data bus;
          DataLines.RO = 1u;

          // Load output register from data bus.
          DataLines.OI = 1u;

          // Fetch next instruction.
          SetInstructionFetchNextState();

          break;
        }

        default:
        {
          // Halt clock.
          DataLines.HLT = 1;

          // Freeze state.
          DataLines.NextInstState = AddressLines.InstState;

          break;
        }
      }

      break;
    }

    case OpCodeJMP:
    {
      // Micro instructions for JMP <addr> opcode.
      switch (AddressLines.InstState)
      {
        case T0:
        case T1:
          break;

        case T2:
        {
          // Place IR on bus.  Only the operand is used.
          DataLines.IO = 1u;

          // Set PC from operand.
          DataLines.J = 1u;

          // Fetch next instruction.
          SetInstructionFetchNextState();

          break;
        }

        default:
        {
          // Halt clock.
          DataLines.HLT = 1;

          // Freeze state.
          DataLines.NextInstState = AddressLines.InstState;

          break;
        }
      }
      break;
    }

    case OpCode1001:
    {
      switch (AddressLines.InstState)
      {
        case T0:
        case T1:
          break;

        // Unimplemented op code.
        default:
        {
          // Halt clock.
          DataLines.HLT = 1;

          // Freeze state.
          DataLines.NextInstState = AddressLines.InstState;

          break;
        }

//        case T2:
//        {
//          break;
//        }
//
//        case T3:
//        {
//          break;
//        }
//
//        case T4:
//        {
//          break;
//        }
//
//        default:
//        {
//          break;
//        }
      }
      break;
    }

    case OpCode1010:
    {
      switch (AddressLines.InstState)
      {
        case T0:
        case T1:
          break;

        // Unimplemented op code.
        default:
        {
          // Halt clock.
          DataLines.HLT = 1;

          // Freeze state.
          DataLines.NextInstState = AddressLines.InstState;

          break;
        }

//        case T2:
//        {
//          break;
//        }
//
//        case T3:
//        {
//          break;
//        }
//
//        case T4:
//        {
//          break;
//        }
//
//        default:
//        {
//          break;
//        }
      }
      break;
    }

    case OpCode1011:
    {
      switch (AddressLines.InstState)
      {
        case T0:
        case T1:
          break;

        // Unimplemented op code.
        default:
        {
          // Halt clock.
          DataLines.HLT = 1;

          // Freeze state.
          DataLines.NextInstState = AddressLines.InstState;

          break;
        }

//        case T2:
//        {
//          break;
//        }
//
//        case T3:
//        {
//          break;
//        }
//
//        case T4:
//        {
//          break;
//        }
//
//        default:
//        {
//          break;
//        }
      }
      break;
    }

    case OpCode1100:
    {
      switch (AddressLines.InstState)
      {
        case T0:
        case T1:
          break;

        // Unimplemented op code.
        default:
        {
          // Halt clock.
          DataLines.HLT = 1;

          // Freeze state.
          DataLines.NextInstState = AddressLines.InstState;

          break;
        }

//        case T2:
//        {
//          break;
//        }
//
//        case T3:
//        {
//          break;
//        }
//
//        case T4:
//        {
//          break;
//        }
//
//        default:
//        {
//          break;
//        }
      }
      break;
    }

    case OpCode1101:
    {
      switch (AddressLines.InstState)
      {
        case T0:
        case T1:
          break;

        // Unimplemented op code.
        default:
        {
          // Halt clock.
          DataLines.HLT = 1;

          // Freeze state.
          DataLines.NextInstState = AddressLines.InstState;

          break;
        }

//        case T2:
//        {
//          break;
//        }
//
//        case T3:
//        {
//          break;
//        }
//
//        case T4:
//        {
//          break;
//        }
//
//        default:
//        {
//          break;
//        }
      }
      break;
    }


    case OpCode1110:
    {
      switch (AddressLines.InstState)
      {
        case T0:
        case T1:
          break;

        // Unimplemented op code.
        default:
        {
          // Halt clock.
          DataLines.HLT = 1;

          // Freeze state.
          DataLines.NextInstState = AddressLines.InstState;

          break;
        }

//        case T2:
//        {
//          break;
//        }
//
//        case T3:
//        {
//          break;
//        }
//
//        case T4:
//        {
//          break;
//        }
//
//        default:
//        {
//          break;
//        }
      }
      break;
    }

    case OpCode1111:
    {
      switch (AddressLines.InstState)
      {
        case T0:
        case T1:
          break;

        // Unimplemented op code.
        default:
        {
          // Halt clock.
          DataLines.HLT = 1;

          // Freeze state.
          DataLines.NextInstState = AddressLines.InstState;

          break;
        }

//        case T2:
//        {
//          break;
//        }
//
//        case T3:
//        {
//          break;
//        }
//
//        case T4:
//        {
//          break;
//        }
//
//        default:
//        {
//          break;
//        }
      }
      break;
    }

    default:
    {
      cout << "Invalid opcode." << endl;
      break;
    }
  }

    // Remember this entry.
    ROMData[Addr] = Data;
  }

  // ********************************************************************************************************
  // ROM contents defined.  Write the contents to disk.
  // ********************************************************************************************************

  path PathBase { absolute(argv[1]) };
  path Stem { PathBase.stem() };
  uint16_t Temp;

  cout << "Write to file name base: " << PathBase << endl;

  {
    // Write the low order ROM contents.
    ofstream ofs(PathBase.replace_filename(Stem.string() + "-LowOrder" + PathBase.extension().string()));
    if (!ofs)
    {
      cout << "Could not open file \"" << argv[1] <<"\" for output." << endl;
      exit(__LINE__);
    }

    for (const auto &Entry : ROMData)
    {
      Temp = Entry & 0xffffu;
      ofs.write((const char *) &Temp, sizeof(uint16_t));
    }

    ofs.close();
  }

  {
    // Write the high order ROM contents.
    ofstream ofs(PathBase.replace_filename(Stem.string() + "-HighOrder" + PathBase.extension().string()));
    if (!ofs)
    {
      cout << "Could not open file \"" << argv[1] <<"\" for output." << endl;
      exit(__LINE__);
    }

    for (const auto &Entry : ROMData)
    {
      Temp = (Entry >> 16u) & 0xffffu;
      ofs.write((const char *) &Temp, sizeof(uint16_t));
    }

    ofs.close();
  }

  // Write a csv file with results.
  {
    ofstream ofs(PathBase.replace_filename(Stem.string() + PathBase.extension().string() + ".csv"));
    if (!ofs)
    {
      cout << "Could not open file \"" << argv[1] <<"\" for output." << endl;
      exit(__LINE__);
    }

    ofs << "Opcode, Operand, CF, ZF, NF, VF State, HLT, MI, RI, RO, IO, II, AI, AO, EO, SU, BI, OI, CE, CO, J, FI, EX ,NextState" << endl;

    auto Addr = 0u;
    for (const auto &Entry : ROMData)
    {
      Data = Entry;
      Address = Addr++;

      if (AddressLines.InstState > 3u) continue;
      if ((AddressLines.OpCode > 0u) && (AddressLines.CF || AddressLines.ZF || AddressLines.NF || AddressLines.VF || (AddressLines.Operand > 0u))) continue;

      ofs <<
          format("%1X", AddressLines.OpCode) <<
          format(",%1X", AddressLines.Operand) <<
          format(",%u", AddressLines.CF) <<
          format(",%u", AddressLines.ZF) <<
          format(",%u", AddressLines.NF) <<
          format(",%u", AddressLines.VF) <<
          format(",%u", AddressLines.InstState) <<
          format(",%u", DataLines.HLT) <<
          format(",%u", DataLines.MI) <<
          format(",%u", DataLines.RI) <<
          format(",%u", DataLines.RO) <<
          format(",%u", DataLines.IO) <<
          format(",%u", DataLines.II) <<
          format(",%u", DataLines.AI) <<
          format(",%u", DataLines.AO) <<
          format(",%u", DataLines.EO) <<
          format(",%u", DataLines.SU) <<
          format(",%u", DataLines.BI) <<
          format(",%u", DataLines.OI) <<
          format(",%u", DataLines.CE) <<
          format(",%u", DataLines.CO) <<
          format(",%u", DataLines.J) <<
          format(",%u", DataLines.FI) <<
          format(",%u", DataLines.EX) <<
          format(",%u", DataLines.NextInstState) <<
          endl;
    }

    ofs.close();
  }

  cout << "Done." << endl;

  return 0;
}
