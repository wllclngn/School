# XINU SIM Builder

This project is a cross-platform build system for a XINU Operating System simulation environment. It handles the generation of necessary files, compilation of the XINU C code, and running the simulation.

## Project Overview

The XINU SIM Builder (`xinu_sim`) provides a suite of Python scripts to manage the lifecycle of a XINU OS simulation. It is designed to:
- Generate necessary header files and C source files for the simulation.
- Compile the XINU OS source code along with the simulation-specific files using GCC.
- Provide a clean build process.
- Run the compiled XINU simulation.
- Log compilation and build activities.

## Features

- **File Generation**: Automatically generates standard XINU definitions, includes, simulation declarations, and core C files.
- **Compilation**: Compiles XINU C source files from various directories (`system`, `device/tty`, `shell`, `lib/libxc`) and links them into an executable.
- **Build Management**: Supports cleaning previous build artifacts.
- **Simulation Execution**: Can run the compiled XINU simulation, with support for arguments (e.g., for starvation tests).
- **Logging**: Detailed logging of the build process is saved to `compilation.txt` in the output directory, with a summary appended.
- **Cross-Platform**: Designed to work on different operating systems (Windows, Linux, macOS) with Python 3 and GCC.

## Project Structure

The project expects a certain directory structure, typically with the XINU OS source code either in the project root or in a subdirectory named `XINU OS`.

Key directories and files involved:
- `XINU SIM/` (or your project root): Contains the builder scripts.
    - `main.py`: Main entry point for the builder.
    - `generator.py`: Handles generation of XINU simulation files.
    - `compiler.py`: Manages compilation and linking of XINU source.
    - `utils/`: Contains utility modules like `config.py` and `logger.py`.
    - `templates/`: Contains templates for generated files.
    - `include/`: May contain a custom `xinu.h` or be used for XINU OS headers.
    - `/output/` (default, can be changed via `-o`):
        - `obj/`: Directory for compiled object files.
        - `xinu_stddefs.h`, `xinu_includes.h`, etc.: Generated files.
        - `compilation.txt`: Log file for the build process.
        - `xinu_core` (or `xinu_core.exe` on Windows): The compiled simulation executable.
- `XINU OS/` (optional, if XINU source is nested):
    - `include/`: XINU OS header files.
    - `system/`: XINU OS system files.
    - `device/`: XINU OS device driver files.
    - `shell/`: XINU OS shell files.
    - `lib/libxc/`: XINU OS library files.

## Prerequisites

- Python 3
- GCC (GNU Compiler Collection) installed and accessible in the system PATH.

## How to Use

Navigate to the directory containing `main.py` (e.g., `Graduate-School/CIS657/FINAL/XINU SIM/`).

### 1. Setup
Ensure your XINU OS source files are located correctly, either in the project directory or in a subdirectory named `XINU OS`.

### 2. Building the Simulation

To generate files and compile the XINU simulation:
```bash
python main.py
```

### 3. Running the Simulation

To run the simulation after building:
```bash
python main.py --run
```
Once compiled, XINU's live environment will run automatically just like as if launched by VirtualBox within the terminal that the Python script was compiled in.

### 4. Cleaning the Build

To remove all generated files and build artifacts:
```bash
python main.py --clean
```

### Command-Line Options

The `main.py` script accepts several command-line arguments:

- `--clean`: Clean the build directory before any other operation.
- `--run`: Run the XINU simulation after a successful build.
- `--starvation <value>`: Pass an argument for a starvation test to the simulation (used with `--run`).
- `--no-compile`: Generate necessary files but skip the compilation step.
- `-d <directory>`, `--directory <directory>`: Specify the project directory. Defaults to the current directory (`.`).
- `-v`, `--verbose`: Enable verbose output during the build process.
- `-o <output_directory>`, `--output-dir <output_directory>`: Specify a custom output directory for generated files, object files, and the executable. Defaults to `xinu_sim/output/`.

**Example combining options:**
Clean, build, and run with verbose output, specifying a project directory:
```bash
python main.py -d "/path/to/your/xinu_project" --clean --run -v
```

## Logging

The build process generates two log files:
- `compilation.txt`: Located in the specified output directory (e.g., `XINU SIM/output/compilation.txt`). Contains detailed logs of the file generation and compilation process.
