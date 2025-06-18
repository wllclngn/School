# XINU SIM Builder

A comprehensive cross-platform build system for compiling and running XINU Operating System simulations. The XINU SIM Builder provides an integrated Python-based toolchain that handles the complete lifecycle of XINU OS development, from source code generation to executable creation and simulation execution.

## Project Overview

The XINU SIM Builder (`xinu_sim`) is a sophisticated build automation system designed specifically for XINU Operating System development and simulation. It bridges the gap between traditional XINU OS source code and modern development environments by providing:

- **Automated File Generation**: Creates necessary header files, type definitions, and simulation-specific C source files
- **Intelligent Compilation**: Compiles XINU OS source code with proper include path resolution and cross-platform compatibility
- **Makefile Integration**: Parses existing XINU Makefiles to extract build instructions and dependencies
- **Simulation Execution**: Runs compiled XINU simulations with interactive terminal I/O
- **Comprehensive Logging**: Detailed build process logging with error reporting and debugging information

## Features

### Core Build System
- **Template-Based Code Generation**: Automatically generates XINU standard definitions, includes, simulation declarations, and core implementation files
- **Modular Architecture**: Separate modules for generation (`generator.py`), compilation (`compiler.py`), and Makefile parsing (`makefile_parser.py`)
- **Error Handling**: Robust error detection and reporting with compilation limits and detailed logging
- **Cross-Platform Support**: Works seamlessly across Windows, Linux, and macOS environments

### Compilation Management
- **Source File Discovery**: Automatically scans XINU OS directories (`system`, `device/tty`, `shell`, `lib/libxc`) for source files
- **Include Path Resolution**: Intelligent include directory detection and path resolution
- **Object File Management**: Organized compilation with separate object file directory structure
- **Symbol Shimming**: Handles platform-specific function implementations and library conflicts

### Build Integration
- **Makefile Parsing**: Extracts source files, compiler flags, include directories, and linker options from existing XINU Makefiles
- **Dependency Tracking**: Analyzes build dependencies and compilation order
- **Flag Management**: Cross-platform compiler and linker flag optimization
- **Build Verification**: Post-build validation and executable integrity checking

## Architecture

### Project Structure
```
Graduate-School/CIS657/FINAL/
├── XINU SIM/                    # Build system directory
│   ├── main.py                  # Main entry point and build orchestration
│   ├── generator.py             # Template-based file generation
│   ├── compiler.py              # Source compilation and linking
│   ├── makefile_parser.py       # Makefile analysis and parsing
│   ├── templates/               # Template files for code generation
│   │   ├── xinu_stddefs_h.tmpl  # XINU type definitions
│   │   ├── xinu_h.tmpl          # Main XINU header template
│   │   ├── xinu_includes_h.tmpl # Include wrapper template
│   │   ├── xinu_simulation_c.tmpl # Simulation helper template
│   │   ├── xinu_core_c.tmpl     # Core implementation template
│   │   └── ...                  # Additional templates
│   ├── utils/                   # Utility modules
│   │   ├── config.py            # Configuration management
│   │   └── logger.py            # Logging functionality
│   ├── include/                 # Generated header files
│   ├── output/                  # Build artifacts and executables
│   │   ├── obj/                 # Compiled object files
│   │   ├── xinu_core            # Main executable (Unix/Linux)
│   │   ├── xinu_core.exe        # Main executable (Windows)
│   │   └── compilation.txt      # Detailed build log
│   └── system/                  # Additional system files
└── XINU OS/                     # XINU Operating System source code
    ├── include/                 # XINU OS header files
    ├── system/                  # Core system implementation
    ├── device/                  # Device drivers and I/O
    ├── shell/                   # XINU shell implementation
    ├── lib/libxc/              # XINU C library
    ├── compile/                 # Build configuration and Makefiles
    └── config/                  # System configuration files
```

### Key Components

#### XinuBuilder (main.py)
The central orchestrator that manages the complete build process:
- Project directory detection and setup
- Build artifact cleanup and management
- Template loading and file generation coordination
- Compilation and linking process management
- Simulation execution with interactive I/O

#### XinuGenerator (generator.py)
Handles template-based code generation:
- XINU standard definitions and type declarations
- Platform-specific include file generation
- Simulation helper function implementation
- Core executable template processing

#### XinuCompiler (compiler.py)
Manages the compilation and linking process:
- Source file discovery and filtering
- Cross-platform compiler flag management
- Object file generation and organization
- Executable linking with library resolution

#### MakefileParser (makefile_parser.py)
Extracts build information from existing XINU Makefiles:
- Variable expansion and definition extraction
- Source file list compilation
- Include directory resolution
- Compiler and linker flag parsing

## Prerequisites

- **Python 3.7+**: Required for the build system
- **GCC (GNU Compiler Collection)**: Must be installed and accessible in system PATH
- **XINU OS Source Code**: Complete XINU operating system source tree

### Platform-Specific Requirements

#### Linux/Unix
```bash
sudo apt-get install build-essential gcc python3
```

#### macOS
```bash
xcode-select --install
brew install gcc python3
```

#### Windows
- Install MinGW-w64 or similar GCC distribution
- Ensure gcc.exe is in system PATH
- Python 3.7+ from python.org

## Installation and Setup

1. **Clone or download the project** to your local development environment
2. **Verify XINU OS source structure** - ensure the `XINU OS/` directory contains the complete XINU source tree
3. **Test compiler installation**:
   ```bash
   gcc --version
   python3 --version
   ```

## Usage

Navigate to the `Graduate-School/CIS657/FINAL/XINU SIM/` directory for all operations.

### Basic Build Operations

#### Complete Build Process
Generate files, compile source code, and create executable:
```bash
python3 main.py
```

#### Clean Build
Remove all previous build artifacts and perform fresh build:
```bash
python3 main.py --clean
```

#### Build and Execute
Compile and immediately run the XINU simulation:
```bash
python3 main.py --run
```

#### Verbose Build
Enable detailed output for debugging build issues:
```bash
python3 main.py --verbose
```

### Advanced Usage

#### Custom Output Directory
Specify alternative output location:
```bash
python3 main.py -o /path/to/custom/output
```

#### Project Directory Override
Build XINU source from different location:
```bash
python3 main.py -d /path/to/xinu/project
```

#### Skip Compilation
Generate files only without compilation:
```bash
python3 main.py --no-compile
```

#### Starvation Testing
Run simulation with starvation test parameters:
```bash
python3 main.py --run --starvation 5
```

### Command-Line Reference

| Option | Description |
|--------|-------------|
| `--clean` | Remove all generated files and build artifacts before building |
| `--run` | Execute the XINU simulation after successful compilation |
| `--verbose`, `-v` | Enable detailed logging and output during build process |
| `--no-compile` | Generate necessary files but skip compilation step |
| `--starvation <value>` | Pass starvation test parameter to simulation (use with `--run`) |
| `--directory <path>`, `-d <path>` | Specify project directory (default: current directory) |
| `--output-dir <path>`, `-o <path>` | Specify custom output directory for build artifacts |

### Example Workflows

#### Development Workflow
```bash
# Clean build with verbose output for debugging
python3 main.py --clean --verbose

# Quick compilation check
python3 main.py

# Test execution
python3 main.py --run
```

#### CI/CD Integration
```bash
# Automated build verification
python3 main.py --clean --verbose
if [ $? -eq 0 ]; then
    echo "Build successful"
    python3 main.py --run --starvation 1
else
    echo "Build failed"
    exit 1
fi
```

## Logging and Debugging

### Log Files
- **`output/compilation.txt`**: Complete build log with timestamps, commands, and output
- **Console Output**: Real-time build progress and error reporting
- **Verbose Mode**: Detailed debugging information with `-v` flag

### Common Build Issues
1. **Missing Headers**: Ensure XINU OS include files are present
2. **Compiler Errors**: Check GCC installation and PATH configuration  
3. **Linking Failures**: Verify all required libraries are available
4. **Template Issues**: Confirm template files exist in `templates/` directory

### Debugging Steps
1. **Enable Verbose Mode**: Use `--verbose` for detailed output
2. **Check Log Files**: Review `compilation.txt` for specific errors
3. **Verify Paths**: Ensure all directory structures are correct
4. **Test Minimal Build**: Try `--no-compile` to isolate generation issues

## Simulation Execution

Once successfully built, the XINU simulation provides:
- **Interactive Shell**: Access to XINU command-line interface
- **Process Management**: XINU process creation and scheduling
- **System Calls**: Full XINU system call implementation
- **Device I/O**: Simulated device driver functionality

### Simulation Features
- Real-time terminal interaction
- Command history and editing
- Process monitoring and debugging
- System state inspection
- Performance testing capabilities

## Cross-Platform Compatibility

The build system is designed for maximum portability:

### Compiler Abstraction
- Automatic platform detection
- Conditional compilation flags
- Path separator handling
- Library linking adaptation

### Template System
- Platform-specific code generation
- Conditional include statements
- Runtime environment detection
- Cross-platform type definitions

## Troubleshooting

### Build Failures
1. **Verify Prerequisites**: Check Python 3.7+ and GCC installation
2. **Path Issues**: Ensure all paths use forward slashes or proper escaping
3. **Permission Problems**: Check write permissions for output directory
4. **Template Errors**: Verify template files are present and readable

### Runtime Issues
1. **Executable Not Found**: Check compilation completed successfully
2. **Permission Denied**: Run `chmod +x output/xinu_core` on Unix systems
3. **Library Errors**: Ensure all required libraries are installed
4. **Simulation Crashes**: Check for memory allocation or pointer issues

### Getting Help
1. **Enable Verbose Logging**: Use `--verbose` flag for detailed output
2. **Check Log Files**: Review `compilation.txt` for specific error messages
3. **Verify Environment**: Ensure all prerequisites are properly installed
4. **Clean Build**: Try `--clean` flag to remove potentially corrupted artifacts

## Contributing

This project is part of the CIS657 graduate coursework. For issues, improvements, or contributions:

1. Document any problems in the build process
2. Provide detailed error logs and system information
3. Test changes across multiple platforms when possible
4. Maintain compatibility with existing XINU OS source code

## License

This project is developed for educational purposes as part of graduate-level operating systems coursework.