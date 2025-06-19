# XINU SIM Builder

A robust, cross-platform Python build system for compiling and running XINU Operating System simulations. The XINU SIM Builder offers a fully automated toolchain for generating, building, and executing XINU OS simulations with strict, transparent logging and no misleading or simulated actions.

## Project Purpose

The XINU SIM Builder (`xinu_sim`) is designed for developers and students working with the XINU Operating System. It automates the build process, manages code generation from templates, handles compilation and linking, and (optionally) runs the final simulation. The system is designed to be platform-agnostic, supporting Linux, macOS, and Windows, and always adheres to strict operational transparency—logging only what is actually done and never printing misleading or suggestive messages.

## What This Project Does

- **Automated Code Generation**: Dynamically creates required C headers, type definitions, and simulation-specific source files using templates.
- **Source Discovery & Makefile Parsing**: Scans XINU OS directories for source files and parses XINU Makefiles to extract build instructions.
- **Intelligent Compilation**: Compiles all required XINU OS sources with correct include and object management, using platform-appropriate compilers and flags.
- **Artifact Management**: Organizes build outputs, object files, and logs in a dedicated structure.
- **Error Handling**: Provides robust, actionable error detection and logging.
- **Strict Logging**: Console and file logs report only real actions—no suggestions, no stubs, and no simulated operations.
- **Simulation Execution**: Optionally runs the compiled XINU simulation if and only if explicitly requested (with `--run`).

## Features

### Build System

- **Template-Based Generation**: Generates all required headers and simulation scaffolding from templates in the `templates/` directory.
- **Modular Architecture**: Each key function is in its own module (`generator.py`, `compiler.py`, `makefile_parser.py`, `utils/`).
- **Cross-Platform**: Runs identically on Windows, Linux, and macOS.
- **No Stubs/Facades**: All actions are real; the system never prints or logs actions it hasn't performed.

### Compilation

- **Automatic Source File Discovery**: Scans standard XINU OS directories for sources.
- **Makefile Integration**: Parses and obeys Makefile-defined build logic.
- **Object File Organization**: All object files are organized under `output/obj/`.
- **Platform-Aware Linking**: Handles system-specific compiler/linker flags and library requirements.

### Execution

- **Explicit Control**: Simulation is only run if invoked with `--run`.
- **No Suggestive Output**: The system never prints messages like "Run the simulation with..." unless it's actually performing the action.

### Logging

- **Accurate Build Logs**: All actions, errors, and outputs are logged to `output/compilation.txt`.
- **Verbose Option**: Use `-v` or `--verbose` for detailed logging.
- **No Banners or Decorative Output**: Logging is modern and strictly functional.

## Architecture

```
Graduate-School/CIS657/FINAL/
├── XINU SIM/                    # Build system
│   ├── main.py                  # Orchestrator
│   ├── generator.py             # Template generation
│   ├── compiler.py              # Compilation/linking
│   ├── makefile_parser.py       # Makefile parsing
│   ├── templates/               # Code generation templates
│   ├── utils/                   # Config and logger
│   ├── include/                 # Generated headers
│   ├── output/                  # Build outputs
│   │   ├── obj/                 # Object files
│   │   ├── xinu_core[.exe]      # Executable (platform-specific)
│   │   └── compilation.txt      # Build log
│   └── system/                  # Additional files
└── XINU OS/                     # XINU source tree
    ├── include/
    ├── system/
    ├── device/
    ├── shell/
    ├── lib/libxc/
    ├── compile/
    └── config/
```

## Requirements

- **Python 3.7+**
- **GCC or compatible compiler** (MinGW-w64 for Windows)
- **XINU OS Source Tree**

### Platform Setup

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

- Install MinGW-w64 or similar
- Ensure `gcc.exe` is in system PATH
- Install Python 3.7+ from python.org

## Quick Start

1. **Clone or download** the project.
2. **Verify your XINU OS source** is present in the `XINU OS/` directory.
3. **Test your compiler**:

   ```bash
   gcc --version
   python3 --version
   ```

4. **Navigate to the build system directory**:

   ```bash
   cd Graduate-School/CIS657/FINAL/XINU\ SIM/
   ```

### Build Commands

| Command                                 | What it does                                                 |
|------------------------------------------|--------------------------------------------------------------|
| `python3 main.py`                       | Generate, compile, and link XINU OS simulation               |
| `python3 main.py --clean`                | Remove all build artifacts and output files                   |
| `python3 main.py --run`                  | Build, then run the simulation executable                    |
| `python3 main.py --verbose`              | Build with detailed output/logging                            |
| `python3 main.py -o /path/to/output`     | Use a custom output directory                                 |
| `python3 main.py -d /path/to/project`    | Build from an alternate XINU OS source location               |
| `python3 main.py --no-compile`           | Only generate files, do not compile                           |
| `python3 main.py --run --starvation 5`   | Build, then run simulation with starvation testing parameter  |

> **Note:** No instructional messages are output—actions only reflect what is performed.

## Logging and Debugging

- All logs are in `output/compilation.txt` (with timestamps).
- Use `--verbose` for more detail.
- No banners or decorative output; logs are strictly functional.

## Troubleshooting

- **Build Fails:** Check prerequisites, permissions, and that all templates are present.
- **Executable Not Found or Not Running:** Ensure build completed successfully and that you have execution permissions (on Unix, use `chmod +x`).
- **No Suggestive Output:** If you expected a message about how to run the simulation, see the documentation here; the build system won’t print such messages unless actually running the simulation.

## Contributing

- Please test all changes across platforms.
- Provide detailed logs and environment information for issues.
- Adhere to the project's strict operational and logging standards.

## License

This project is developed for educational use in CIS657 (Graduate Operating Systems).
