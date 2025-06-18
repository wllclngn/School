# windows_compat.py - Comprehensive Windows compatibility module for XINU OS
# Handles path translation, C code compatibility, and build process adaptation
import os
import sys
import re
import subprocess
import datetime
import ctypes
from ctypes import wintypes
from pathlib import Path
import shutil
import tempfile

# Import logger from project
from utils.logger import log

#############################################################################
#                        CONSTANTS & CONFIGURATION                          #
#############################################################################

# Windows path constants
MAX_PATH = 260
INVALID_FILE_ATTRIBUTES = 0xFFFFFFFF
FILE_ATTRIBUTE_NORMAL = 0x00000080

# MinGW location guesses
MINGW_PATHS = [
    "C:\\MinGW\\bin",
    "C:\\msys64\\mingw64\\bin", 
    "C:\\msys64\\mingw32\\bin",
    "C:\\Program Files\\mingw-w64\\x86_64-8.1.0-posix-seh-rt_v6-rev0\\mingw64\\bin"
]

# Header files that need special handling
PROBLEM_HEADERS = [
    "conf.h", 
    "kernel.h",
    "prototypes.h",
    "stdio.h", 
    "xinu.h"
]

# Compiler flags that need modification for Windows
INCOMPATIBLE_FLAGS = {
    "-m": "elf_i386",  # Linker flag not supported on Windows
    "-march=i586": "-march=i586",  # Keep this flag, it works on MinGW
    "-fno-builtin": "-fno-builtin",  # Keep this one
    "-fno-stack-protector": "-fno-stack-protector",  # Keep this one
    "-nostdlib": "-nostdlib",  # Keep this one
}

# Version information
VERSION = "1.4.0"
MODULE_NAME = "XINU Windows Compatibility Module"

#############################################################################
#                             PATH HANDLING                                 #
#############################################################################

def initialize_path_handling():
    # Initialize Windows path handling support and check system configuration
    if not os.name == 'nt':
        return False
        
    # Get kernel32.dll functions for direct Windows API access
    kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)
    
    # Configure function argument types
    kernel32.SetFileAttributesW.argtypes = (wintypes.LPCWSTR, wintypes.DWORD)
    kernel32.CreateDirectoryW.argtypes = (wintypes.LPCWSTR, wintypes.LPVOID)
    kernel32.GetFileAttributesW.argtypes = (wintypes.LPCWSTR,)
    
    # Check registry for long path support
    long_paths_enabled = check_long_path_registry()
    
    # Return the kernel32 instance and settings
    return {
        'kernel32': kernel32,
        'long_paths_enabled': long_paths_enabled
    }

def check_long_path_registry():
    # Check if long path support is enabled in the Windows registry
    try:
        import winreg
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r"SYSTEM\CurrentControlSet\Control\FileSystem") as key:
            long_paths_enabled, _ = winreg.QueryValueEx(key, "LongPathsEnabled")
            if not long_paths_enabled:
                log("WARNING: Long paths support is not enabled in Windows registry.", level="WARNING")
                log("To enable: Set HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem\\LongPathsEnabled to 1")
            return long_paths_enabled
    except Exception as e:
        log(f"Could not verify Windows long path registry setting: {e}", level="WARNING")
        return False

def handle_long_path(path_str):
    # Handle Windows long paths with robust approach using Path objects
    if not path_str:
        return path_str
        
    # First convert to Path object for consistent handling
    path_obj = Path(path_str)
    
    try:
        # Resolve to absolute path with proper handling of symlinks and junctions
        abs_path = path_obj.resolve()
        
        # Convert to string representation
        path_str = str(abs_path)
        
        # Apply long path prefix if needed
        if len(path_str) >= 260 and not path_str.startswith("\\\\?\\"):
            return f"\\\\?\\{path_str}"
        
        return path_str
    except Exception as e:
        log(f"Warning: Path resolution failed for '{path_str}': {e}", level="WARNING")
        # Fall back to original path if resolution fails
        return path_str

def normalize_windows_path(path):
    # Improved path normalization with consistent handling of edge cases
    if not path:
        return path
        
    # Use Path objects for most operations to leverage built-in normalization
    try:
        # First convert to a Path object
        path_obj = Path(path)
        
        # Resolve to absolute path
        if not path_obj.is_absolute():
            path_obj = path_obj.resolve()
        
        # Convert back to string and handle long path prefix if needed
        path_str = str(path_obj)
        if len(path_str) >= 260 and not path_str.startswith("\\\\?\\"):
            return handle_long_path(path_str)
        
        return path_str
                
    except Exception as e:
        log(f"Path normalization warning: {e}", level="WARNING")
        # If Path handling fails, attempt basic string normalization
        
        # Windows normalization fallback
        path = path.replace('/', '\\')
        # De-duplicate backslashes, preserving network shares and long path prefixes
        if path.startswith("\\\\?\\"):
            prefix, rest = "\\\\?\\", path[4:]
            # Clean up the rest of the path
            while "\\\\" in rest:
                rest = rest.replace("\\\\", "\\")
            path = prefix + rest
        elif path.startswith("\\\\"):
            # Handle UNC paths (network shares)
            parts = path[2:].split('\\')
            prefix = "\\\\"
            rest = '\\'.join(parts)
            # Clean up the rest
            while "\\\\" in rest:
                rest = rest.replace("\\\\", "\\")
            path = prefix + rest
        else:
            # Normal local paths
            while "\\\\" in path:
                path = path.replace("\\\\", "\\")
                
        return path

def get_safe_path(*parts):
    # Create a safe path by joining parts and normalizing
    if not parts:
        return ""
        
    # Start with the first part
    result = str(parts[0]) if parts[0] else ""
    
    # Add remaining parts using os.path.join for proper separator handling
    for part in parts[1:]:
        if part:
            if result and result[-1] not in (os.path.sep, os.path.altsep or os.path.sep):
                result = os.path.join(result, str(part))
            else:
                result += str(part)
    
    # Normalize the final path
    return normalize_windows_path(result)

def ensure_directory_exists(directory_path):
    # Create directory with support for long paths
    path_str = handle_long_path(directory_path)
    
    if not os.path.exists(path_str):
        try:
            # Use regular mkdir for paths within MAX_PATH
            os.makedirs(path_str, exist_ok=True)
            return True
        except Exception as e:
            # For very long paths, try Windows API directly
            if len(path_str) > MAX_PATH:
                try:
                    # Load kernel32 for direct Windows API calls
                    kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)
                    kernel32.CreateDirectoryW.argtypes = (wintypes.LPCWSTR, wintypes.LPVOID)
                    
                    # Split path and create directories one by one
                    parts = Path(path_str.replace("\\\\?\\", "")).parts
                    current = "\\\\?\\" + parts[0] + "\\"
                    for part in parts[1:]:
                        current = os.path.join(current, part)
                        if not os.path.exists(current):
                            # Use CreateDirectoryW from kernel32
                            if not kernel32.CreateDirectoryW(current, None):
                                error = ctypes.get_last_error()
                                if error != 183:  # ERROR_ALREADY_EXISTS
                                    log(f"Error creating directory: {current}, error code: {error}", level="ERROR")
                                    return False
                    return True
                except Exception as nested_e:
                    log(f"Failed to create directory using Windows API: {nested_e}", level="ERROR")
                    return False
            else:
                log(f"Failed to create directory: {e}", level="ERROR")
                return False
    return True
    
def fix_include_paths(content):
    # Fix include paths in C/C++ code to use correct path separators
    # Find #include statements with forward slashes and replace them
    pattern = r'#include\s+["<](.*?)[">]'
    
    def replace_path(match):
        path = match.group(1)
        fixed_path = path.replace('/', '\\')
        if match.group(0).startswith('#include "'):
            return f'#include "{fixed_path}"'
        else:
            return f'#include <{fixed_path}>'
    
    return re.sub(pattern, replace_path, content)

def fix_compiler_path(path):
    # Convert Unix-style paths to Windows-style if needed
    if path.startswith('/'):
        # Convert /usr/bin/ style paths to Windows format
        if path.endswith('/'):
            cmd = path.split('/')[-2]  # Get command from path
        else:
            cmd = path.split('/')[-1]
        
        # Try to find the command in PATH
        from shutil import which
        cmd_path = which(cmd)
        if cmd_path:
            return cmd_path
        
        # If not found, try MinGW locations
        for mp in MINGW_PATHS:
            if os.path.exists(os.path.join(mp, f"{cmd}.exe")):
                return os.path.join(mp, cmd)
        
        # If still not found, just return the command name
        return cmd
    return path

def get_timestamp_and_user():
    # Get current timestamp in consistent format and username
    # Get current timestamp in consistent format
    timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    
    # Get username from system environment
    username = os.environ.get("USER", os.environ.get("USERNAME", "unknown"))
    
    return timestamp, username

#############################################################################
#                        C CODE COMPATIBILITY                               #
#############################################################################

def create_compatibility_header(output_dir):
    # Create a compatibility header that resolves XINU vs Windows conflicts
    timestamp, username = get_timestamp_and_user()
    
    header_content = f"""/* xinu_windows_compat.h - Compatibility layer for XINU on Windows
 * Generated on: {timestamp} by {username}
 * AUTOMATICALLY GENERATED - DO NOT EDIT DIRECTLY
 */
#ifndef _XINU_WINDOWS_COMPAT_H_
#define _XINU_WINDOWS_COMPAT_H_

/* This compatibility header provides Windows-specific functions
 * without conflicting with XINU types.
 */

#ifdef _WIN32

/* Forward-declare needed types without redefining XINU's types */
struct dentry;  /* XINU device entry structure */

/* Forward-declare Windows compatibility functions */
int xinu_putc(int dev, char c);
int xinu_getc(int dev);
int xinu_open(int dev, char *name, char *mode);
int xinu_close(int dev);

/* Remap commonly conflicting functions */
#define putc xinu_putc
#define getc xinu_getc
#define open xinu_open
#define close xinu_close

/* Forward declarations for common Windows structs - NO WINDOWS.H */
struct HWND__;
struct HDC__;
typedef struct HWND__ *HWND;
typedef struct HDC__ *HDC;
typedef unsigned long DWORD;
typedef int BOOL;

#endif /* _WIN32 */

#endif /* _XINU_WINDOWS_COMPAT_H_ */
"""
    
    # Save the compatibility header
    compat_header_path = os.path.join(output_dir, "xinu_windows_compat.h")
    with open(compat_header_path, 'w') as f:
        f.write(header_content)
    
    return compat_header_path

def create_helpers_file(output_dir):
    # Create a C file with Windows-specific helper functions
    timestamp, username = get_timestamp_and_user()
    
    # Get full path for helpers file
    helpers_path = os.path.join(output_dir, "xinu_win_helpers.c")
    
    helpers_content = f"""/* xinu_win_helpers.c - Windows helper functions for XINU
 * Generated on: {timestamp} by {username}
 * AUTOMATICALLY GENERATED - DO NOT EDIT DIRECTLY
 */
#ifdef _WIN32

/* Standard headers */
#include <stdio.h>
#include <stdlib.h>

/* Include our compatibility header */
#include "xinu_windows_compat.h"

/* Implementation of Windows compatibility functions */

/* Replacement for XINU's putc */
int xinu_putc(int dev, char c) {{
    /* Simple implementation that writes to stdout */
    putchar(c);
    return 0;  /* Return success */
}}

/* Replacement for XINU's getc */
int xinu_getc(int dev) {{
    /* Simplified implementation that reads from stdin */
    return getchar();
}}

/* Simplified replacement for XINU's open 
   Note: Signature matches XINU's open(did32, char *, char *); */
int xinu_open(int dev, char *name, char *mode) {{
    /* Simplified implementation */
    return 0;  /* Return a fake file descriptor */
}}

/* Simplified replacement for XINU's close */
int xinu_close(int dev) {{
    /* Simplified implementation */
    return 0;  /* Return success */
}}

/* Simplified replacement for XINU's getpid */
int getpid(void) {{
    /* Return a fake process ID */
    return 1;
}}

#endif /* _WIN32 */
"""
    
    # Save the helpers file
    with open(helpers_path, 'w') as f:
        f.write(helpers_content)
    
    log(f"Created Windows helper functions: {helpers_path}")
    return helpers_path

def patch_xinu_header(header_path, system_include_dir):
    # Create a patched version of a XINU header file for Windows compatibility
    if not os.path.exists(header_path):
        log(f"Header file not found: {header_path}", level="ERROR")
        return None
        
    # Read the original header
    try:
        with open(header_path, 'r') as f:
            content = f.read()
    except Exception as e:
        log(f"Could not read header file {header_path}: {e}", level="ERROR")
        return None
    
    # Add Windows compatibility modifications
    basename = os.path.basename(header_path)
    if basename in PROBLEM_HEADERS:
        # Add our compatibility header first for all problem headers
        content = """#ifdef _WIN32
#include "xinu_windows_compat.h"
#endif

""" + content

        # Additional file-specific modifications
        if basename == "conf.h":
            # Need devcall defined before this header
            pass
            
        elif basename == "kernel.h":
            # Prevent duplicate NULL and NULLCH
            content = content.replace("#define NULL 0  /* null pointer for linked lists */",
                                      "#ifndef NULL\n#define NULL 0  /* null pointer for linked lists */\n#endif")
            content = content.replace("#define NULLCH '\\0'  /* null character   */",
                                      "#ifndef NULLCH\n#define NULLCH '\\0'  /* null character   */\n#endif")
            
        elif basename == "prototypes.h":
            # Handle function prototypes that conflict with Windows
            # No need to redefine - we're just using macros to remap now
            pass
            
        elif basename == "stdio.h":
            # Handle stdio conflicts with conditional compilation
            pass
            
        elif basename == "xinu.h":
            # Prevent stdint.h conflicts
            if os.name == 'nt':
                content = content.replace("#include <stdint.h>", 
                                          "#ifndef _WIN32\n#include <stdint.h>\n#else\n/* Windows uses its own int types */\n#endif")
    
    # Create a path for the wrapper header
    output_dir = os.path.join(system_include_dir, "windows_compat")
    ensure_directory_exists(output_dir)
    
    # Save to the wrapper directory
    wrapper_path = os.path.join(output_dir, os.path.basename(header_path))
    with open(wrapper_path, 'w') as f:
        f.write(content)
    
    return wrapper_path

def create_xinu_windows_layer(include_dirs, output_dir):
    # Create a complete Windows compatibility layer for XINU
    # Create the compatibility header
    compat_header_path = create_compatibility_header(output_dir)
    
    # Create helpers file with function implementations
    helpers_path = create_helpers_file(output_dir)
    
    # Create a special directory for our patched headers
    compat_dir = os.path.join(output_dir, "windows_compat")
    ensure_directory_exists(compat_dir)
    
    # Process each include directory
    patched_headers = {}
    for include_dir in include_dirs:
        if os.path.isdir(include_dir):
            # Process all .h files
            for root, _, files in os.walk(include_dir):
                for file in files:
                    if file.endswith(".h") and file in PROBLEM_HEADERS:
                        header_path = os.path.join(root, file)
                        patched_path = patch_xinu_header(header_path, output_dir)
                        if patched_path:
                            patched_headers[file] = patched_path
    
    # Create an include wrapper script to modify GCC's include path
    wrapper_script = f"""@echo off
REM XINU Windows compatibility wrapper for GCC
REM Generated on {datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")}

REM Save all arguments
set ARGS=

:parse
if "%1"=="" goto execute
set ARGS=%ARGS% %1
shift
goto parse

:execute
REM Call GCC with our compatibility directory first in include path
gcc -I"{output_dir}" -I"{compat_dir}" %ARGS%
exit /b %ERRORLEVEL%
"""

    wrapper_path = os.path.join(output_dir, "xinu-gcc.bat") 
    with open(wrapper_path, 'w') as f:
        f.write(wrapper_script)
    
    log(f"Created Windows compatibility layer in {output_dir}")
    log(f"Created patched headers: {', '.join(patched_headers.keys())}")
    log(f"Created compiler wrapper: {wrapper_path}")
    
    return {
        "compat_header": compat_header_path,
        "helpers_file": helpers_path,
        "compat_dir": compat_dir,
        "patched_headers": patched_headers,
        "wrapper_script": wrapper_path
    }

#############################################################################
#                        BUILD PROCESS ADAPTATION                           #
#############################################################################

def find_compiler():
    # Find the GCC compiler on Windows systems
    # Try using 'where' command to locate gcc
    try:
        result = subprocess.run(['where', 'gcc'], 
                               capture_output=True, 
                               text=True, 
                               check=False)
        
        if result.returncode == 0 and result.stdout.strip():
            gcc_path = result.stdout.strip().split('\n')[0]
            return gcc_path
    except Exception:
        pass
    
    # If 'where' failed, look in common MinGW locations
    for path in MINGW_PATHS:
        gcc_path = os.path.join(path, "gcc.exe")
        if os.path.exists(gcc_path):
            return gcc_path
    
    # If still not found, try other names
    for cmd in ["mingw32-gcc.exe", "x86_64-w64-mingw32-gcc.exe"]:
        for path in MINGW_PATHS:
            gcc_path = os.path.join(path, cmd)
            if os.path.exists(gcc_path):
                return gcc_path
    
    return "gcc"  # Default fallback

def adjust_makefile(makefile_path, output_dir):
    # Create a Windows-compatible version of the XINU Makefile
    if not os.path.exists(makefile_path):
        log(f"Makefile not found: {makefile_path}", level="ERROR")
        return None
    
    # Read the original Makefile
    with open(makefile_path, 'r') as f:
        content = f.read()
    
    # Find the compiler path
    compiler_path = find_compiler()
    compiler_dir = os.path.dirname(compiler_path)
    
    # Adjust paths to Windows format
    content = content.replace("/usr/bin/", compiler_dir.replace('\\', '/') + "/")
    
    # Replace incompatible flags
    content = content.replace("-m elf_i386", "")  # Remove unsupported linker flag
    
    # Add Windows detection and compatibility fixes
    windows_detect = f"""
# Windows compatibility - Added by XINU Windows compatibility layer
ifeq ($(OS),Windows_NT)
    # Replace Unix commands with Windows equivalents
    RM = del /Q
    CP = copy
    MKDIR = mkdir
    # Use Windows path to GCC
    COMPILER_ROOT = {compiler_dir.replace('\\', '/')}/ 
    
    # Add Windows compatibility headers to include path
    INCLUDE += -I{output_dir.replace('\\', '/')}
    
    # Fix linker flags for Windows
    LDFLAGS = 
    
    # Add Windows helper files to compilation
    SRC_FILES += {os.path.join(output_dir, 'xinu_win_helpers.c').replace('\\', '/')}
endif

"""
    
    # Insert Windows detection after first comment block
    first_comment_end = content.find("# ") + content[content.find("# "):].find("\n\n")
    if first_comment_end > 0:
        content = content[:first_comment_end+2] + windows_detect + content[first_comment_end+2:]
    else:
        content = windows_detect + content
    
    # Save the modified Makefile to the output directory
    windows_makefile_path = os.path.join(output_dir, "Makefile.windows")
    with open(windows_makefile_path, 'w') as f:
        f.write(content)
    
    log(f"Created Windows-compatible Makefile: {windows_makefile_path}")
    return windows_makefile_path

def create_build_script(output_dir, makefile_path):
    # Create a Windows batch script to build XINU
    # Determine paths
    makefile_dir = os.path.dirname(makefile_path)
    
    # Create the build script
    script_content = f"""@echo off
REM XINU Windows Build Script
REM Generated on {datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")}

echo Starting XINU build for Windows...

REM Set up environment
set ORIGINAL_DIR=%CD%
cd /d "{makefile_dir}"

REM Set Windows flags
set OS=Windows_NT
set XINU_WINDOWS=1

REM Build using the Windows-compatible Makefile
mingw32-make -f "{os.path.basename(makefile_path)}" %*

REM Return to original directory
cd /d %ORIGINAL_DIR%

IF %ERRORLEVEL% NEQ 0 (
    echo Build failed with error %ERRORLEVEL%
    exit /b %ERRORLEVEL%
) ELSE (
    echo XINU build completed successfully!
)
"""
    
    # Save the build script
    script_path = os.path.join(output_dir, "build_xinu.bat")
    with open(script_path, 'w') as f:
        f.write(script_content)
    
    log(f"Created Windows build script: {script_path}")
    return script_path

#############################################################################
#                           COMPILER FLAG ADJUSTMENT                        #
#############################################################################

def adjust_compiler_flags(flags):
    # Modify compiler flags to be compatible with Windows/MinGW
    if not isinstance(flags, list):
        flags = flags.split()
    
    adjusted_flags = []
    skip_next = False
    
    for i, flag in enumerate(flags):
        if skip_next:
            skip_next = False
            continue
            
        # Check for flags that need special handling
        if flag == "-m" and i+1 < len(flags) and flags[i+1] == "elf_i386":
            # Skip both the -m and elf_i386
            skip_next = True
        elif flag in INCOMPATIBLE_FLAGS:
            replacement = INCOMPATIBLE_FLAGS[flag]
            if replacement:
                adjusted_flags.append(replacement)
        else:
            # Keep other flags
            adjusted_flags.append(flag)
    
    # Add Windows-specific flags as needed
    if "-m32" in flags and "-m64" not in adjusted_flags:
        adjusted_flags.append("-m32")  # Ensure 32-bit compilation
        
    return adjusted_flags

def process_cc_command(command):
    # Process a C compiler command for Windows compatibility
    parts = command.split()
    
    # Find the executable
    executable = parts[0]
    win_executable = fix_compiler_path(executable)
    
    # Find and adjust flags
    flags = []
    files = []
    output = None
    
    i = 1
    while i < len(parts):
        if parts[i] == "-o" and i+1 < len(parts):
            output = parts[i+1]
            i += 2
        elif parts[i].startswith("-"):
            flags.append(parts[i])
            i += 1
        else:
            files.append(parts[i])
            i += 1
    
    # Adjust flags for Windows
    adjusted_flags = adjust_compiler_flags(flags)
    
    # Reconstruct the command
    command_parts = [f'"{win_executable}"']  # Quote the executable path
    
    for flag in adjusted_flags:
        if ' ' in flag:  # Quote flags that contain spaces
            command_parts.append(f'"{flag}"')
        else:
            command_parts.append(flag)
    
    if output:
        command_parts.append("-o")
        if ' ' in output:  # Quote output path if it contains spaces
            command_parts.append(f'"{output}"')
        else:
            command_parts.append(output)
        
    for file in files:
        if ' ' in file:  # Quote file paths that contain spaces
            command_parts.append(f'"{file}"')
        else:
            command_parts.append(file)
    
    return " ".join(command_parts)

#############################################################################
#                               MAIN ENTRY POINTS                           #
#############################################################################

def setup_windows_environment(config):
    # Set up a complete Windows environment for XINU compilation
    log(f"Setting up Windows environment for XINU compilation")
    
    # Ensure we're on Windows
    if os.name != 'nt':
        log("This function should only be called on Windows systems", level="ERROR")
        return False
    
    # Initialize path handling
    initialize_path_handling()
    
    # Extract configuration
    output_dir = normalize_windows_path(config.output_dir)
    
    # Handle include_dirs/include_dir based on what's available
    if hasattr(config, 'include_dirs'):
        include_dirs = [normalize_windows_path(d) for d in config.include_dirs]
    elif hasattr(config, 'include_dir'):
        include_dirs = [normalize_windows_path(config.include_dir)]
    else:
        include_dirs = []
    
    # Ensure output directory exists
    ensure_directory_exists(output_dir)
    
    # Create compatibility layer
    compat_layer = create_xinu_windows_layer(include_dirs, output_dir)
    
    # Find and adjust Makefile
    if hasattr(config, 'makefile_path') and config.makefile_path:
        makefile_path = normalize_windows_path(config.makefile_path)
        win_makefile = adjust_makefile(makefile_path, output_dir)
        
        # Create build script
        if win_makefile:
            build_script = create_build_script(output_dir, win_makefile)
    
    log(f"Windows environment setup complete")
    log(f"Use the generated compatibility headers and build scripts to compile XINU")
    
    return compat_layer

def preprocess_xinu_source(source_path, output_path, include_dirs):
    # Preprocess a XINU source file for Windows compatibility
    # Read the source file
    with open(source_path, 'r') as f:
        content = f.read()
    
    # Fix include paths
    content = fix_include_paths(content)
    
    # Add Windows compatibility header if needed
    if os.name == 'nt' and source_path.endswith((".c", ".h")):
        # Check if this needs our compatibility header
        add_compat = True
        
        # Don't add to files that already have it
        if "#include \"xinu_windows_compat.h\"" in content:
            add_compat = False
            
        # Add our compatibility header at the very top if needed
        if add_compat:
            content = f"""/* Windows compatibility layer - Automatically added */
#ifdef _WIN32
#include "xinu_windows_compat.h"
#endif

{content}"""
    
    # Save the processed file
    with open(output_path, 'w') as f:
        f.write(content)
    
    return output_path

def compile_for_windows(source_files, output_dir, include_dirs=None):
    # Compile XINU source files for Windows
    if include_dirs is None:
        include_dirs = []
        
    # Ensure output directory exists
    ensure_directory_exists(output_dir)
    
    # Find the compiler
    compiler_path = find_compiler()
    log(f"Using compiler: {compiler_path}")
    
    # Process each source file
    compiled_files = []
    
    for source in source_files:
        source_path = normalize_windows_path(source)
        base_name = os.path.basename(source_path)
        processed_path = os.path.join(output_dir, f"win_{base_name}")
        
        # Preprocess the source file
        preprocess_xinu_source(source_path, processed_path, include_dirs)
        
        # Create output object file path
        obj_file = os.path.join(output_dir, f"{os.path.splitext(base_name)[0]}.o")
        
        # Build the compiler command
        include_flags = " ".join([f"-I\"{d}\"" for d in include_dirs])
        cmd = f"\"{compiler_path}\" {include_flags} -c \"{processed_path}\" -o \"{obj_file}\""
        
        # Process the command for Windows
        win_cmd = process_cc_command(cmd)
        
        # Run the compilation
        try:
            log(f"Compiling: {base_name}")
            result = subprocess.run(win_cmd, shell=True, capture_output=True, text=True)
            
            if result.returncode == 0:
                log(f"Successfully compiled: {obj_file}")
                compiled_files.append(obj_file)
            else:
                log(f"Compilation error in {base_name}:", level="ERROR")
                log(result.stderr, level="ERROR")
        except Exception as e:
            log(f"Error running compilation: {str(e)}", level="ERROR")
    
    return compiled_files

#############################################################################
#                            MODULE INITIALIZATION                          #
#############################################################################

def initialize():
    # Initialize the Windows compatibility module
    if os.name != 'nt':
        log("Windows compatibility module loaded but not on Windows - most features disabled")
        return False
    
    log(f"Initializing {MODULE_NAME} v{VERSION}")
    
    # Check for compiler
    compiler = find_compiler()
    if compiler:
        log(f"Found compiler: {compiler}")
    else:
        log("WARNING: Could not find GCC compiler. Please install MinGW.", level="WARNING")
    
    # Initialize path handling
    path_handler = initialize_path_handling()
    
    # Return initialization status
    return {
        'version': VERSION,
        'compiler': compiler,
        'path_handler': path_handler
    }

# Automatically initialize when imported directly on Windows
if __name__ != "__main__" and os.name == 'nt':
    handler_info = initialize()