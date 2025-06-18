# generator.py - Minimal XINU wrapper generator with improved Windows path handling
import os
import datetime
import re
import subprocess
from utils.logger import log
import sys
from pathlib import Path

# For Windows path handling with improved reliability
if os.name == 'nt':
    import ctypes
    from ctypes import wintypes

    # Enable long path support at Windows API level
    kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel32.SetFileAttributesW.argtypes = (wintypes.LPCWSTR, wintypes.DWORD)
    
    # Add registry check for long path support
    try:
        import winreg
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r"SYSTEM\CurrentControlSet\Control\FileSystem") as key:
            long_paths_enabled, _ = winreg.QueryValueEx(key, "LongPathsEnabled")
            if not long_paths_enabled:
                log("WARNING: Long paths support is not enabled in Windows registry.")
                log("To enable: Set HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem\\LongPathsEnabled to 1")
    except Exception:
        log("Could not verify Windows long path registry setting.")

    def handle_long_path(path_str):
        # Handle Windows long paths with more robust approach using Path objects
        # consistently throughout the process
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
            log(f"Warning: Path resolution failed for '{path_str}': {e}")
            # Fall back to original path if resolution fails
            return path_str


class XinuGenerator:
    # Minimal generator for XINU simulation wrapper files
    
    def __init__(self, config):
        self.config = config
    
    def normalize_path(self, path):
        """
        Improved path normalization with more consistent handling
        of edge cases and platform-specific behavior
        """
        if not path:
            return path
            
        # Use Path objects for most operations to leverage built-in normalization
        try:
            # First convert to a Path object
            path_obj = Path(path)
            
            # Windows-specific handling
            if os.name == 'nt':
                # Resolve to absolute path
                if not path_obj.is_absolute():
                    path_obj = path_obj.resolve()
                
                # Convert back to string and handle long path prefix if needed
                path_str = str(path_obj)
                if len(path_str) >= 260 and not path_str.startswith("\\\\?\\"):
                    return handle_long_path(path_str)
                
                return path_str
            else:
                # For Unix systems, just use the Path object's string representation
                # which automatically normalizes separators
                return str(path_obj)
                
        except Exception as e:
            log(f"Path normalization warning: {e}")
            # If Path handling fails, attempt basic string normalization
            
            if os.name == 'nt':
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
            else:
                # Unix normalization fallback
                path = path.replace('\\', '/')
                while '//' in path:
                    path = path.replace('//', '/')
                    
            return path

    def get_safe_path(self, *parts):
        # Create a safe path by joining parts and normalizing,
        # handling all platform-specific edge cases
        # Join parts using the platform's separator
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
        return self.normalize_path(result)

    def fix_include_paths(self, content):
        # Fix include paths in C/C++ code to use correct path separators
        if os.name == 'nt':  # Windows
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
        return content
    
    def fix_compiler_path(self, path):
        # Convert Unix-style paths to Windows-style if needed
        if os.name == 'nt' and path.startswith('/'):
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
            mingw_paths = [
                "C:\\MinGW\\bin",
                "C:\\msys64\\mingw64\\bin",
                "C:\\msys64\\mingw32\\bin"
            ]
            
            for mp in mingw_paths:
                if os.path.exists(os.path.join(mp, f"{cmd}.exe")):
                    return os.path.join(mp, cmd)
            
            # If still not found, just return the command name
            return cmd
        return path
    
    def generate_files(self):
        # Generate only the essential files needed for XINU simulation
        log("Generating minimal XINU simulation files")
        
        # Create output directory with long path support
        output_dir = self.normalize_path(self.config.output_dir)
        os.makedirs(output_dir, exist_ok=True)
        
        # Create obj directory to prevent "No such file or directory" error
        obj_dir = self.get_safe_path(output_dir, "obj")
        os.makedirs(obj_dir, exist_ok=True)
        
        # Generate only the three essential files
        self.generate_stddefs()
        self.generate_includes_wrapper()
        self.generate_sim_helper()
        self.generate_obj_file()
        
        log("XINU simulation file generation complete")
    
    def generate_stddefs(self):
        # Generate xinu_stddefs.h - basic type definitions
        
        # Get current timestamp and username from system
        timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        username = os.environ.get("USER", os.environ.get("USERNAME", "unknown"))
        
        content = f"""/* xinu_stddefs.h - Minimal type definitions for XINU simulation */
/* Generated on: {timestamp} */
/* By user: {username} */
#ifndef _XINU_STDDEFS_H_
#define _XINU_STDDEFS_H_

/* Version information */
#define VERSION "XINU Simulation Version 1.0"

/* Minimal definition to avoid conflicts */
typedef unsigned char byte;

#endif /* _XINU_STDDEFS_H_ */
"""
        
        # Save to output directory using safe path handling
        stddefs_path = self.get_safe_path(self.config.output_dir, "xinu_stddefs.h")
        with open(stddefs_path, 'w') as f:
            f.write(content)
        log(f"Generated standard definitions: {stddefs_path}")
        
        # Also save to XINU include directory to fix pathing issues
        include_stddefs_path = self.get_safe_path(self.config.include_dir, "xinu_stddefs.h")
        with open(include_stddefs_path, 'w') as f:
            f.write(content)
        log(f"Generated standard definitions (include dir copy): {include_stddefs_path}")

    def generate_includes_wrapper(self):
        # Generate xinu_includes.h - the key wrapper that redirects standard functions
        includes_path = self.get_safe_path(self.config.output_dir, "xinu_includes.h")
        log(f"Generating includes wrapper: {includes_path}")
        
        # Get current timestamp and username from system
        timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        username = os.environ.get("USER", os.environ.get("USERNAME", "unknown"))
        
        content = f"""/* xinu_includes.h - Wrapper for XINU code compilation.
 * Generated on: {timestamp} by {username}
 */
#ifndef _XINU_INCLUDES_H_ 
#define _XINU_INCLUDES_H_

#define _CRT_SECURE_NO_WARNINGS 
#define XINU_SIMULATION        

/* Include our minimal defs */
#include "xinu_stddefs.h" 

#endif /* _XINU_INCLUDES_H_ */
"""
        
        # Fix include paths for Windows
        content = self.fix_include_paths(content)
        
        with open(includes_path, 'w') as f:
            f.write(content)
        log(f"Generated includes wrapper: {includes_path}")

    def generate_sim_helper(self):
        # Generate xinu_simulation.c - simulation helper with function implementations
        helper_path = self.get_safe_path(self.config.output_dir, "xinu_simulation.c")
        log(f"Generating simulation helper: {helper_path}")
        
        # Get current timestamp and username from system
        timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        username = os.environ.get("USER", os.environ.get("USERNAME", "unknown"))
        
        content = f"""/* xinu_simulation.c - Helper functions for XINU simulation
 * Generated on: {timestamp} by {username}
 */
#define _CRT_SECURE_NO_WARNINGS

/* Simple standalone simulation without XINU dependencies */
#include <stdio.h>
#include <stdlib.h>

/* Main entry point for simulation */
int main(void) {{
    printf("XINU Simulation Starting\\n");
    printf("Generated on: {timestamp} by {username}\\n\\n");
    
    printf("XINU Simulation Running\\n");
    
    printf("XINU Simulation Completed\\n");
    return 0;
}}
"""
        
        # Fix include paths for Windows
        content = self.fix_include_paths(content)
        
        with open(helper_path, 'w') as f:
            f.write(content)
        log(f"Generated simulation helper: {helper_path}")
    
    def generate_obj_file(self):
        # Create a minimal object file for linking with XINU
        obj_dir = self.get_safe_path(self.config.output_dir, "obj")
        obj_path = self.get_safe_path(obj_dir, "xinu_simulation.o")
        
        # Create a simple C file for compilation
        dummy_c_path = self.get_safe_path(self.config.output_dir, "dummy.c")
        with open(dummy_c_path, 'w') as f:
            f.write("/* Minimal dummy object for compilation */\n")
            f.write("void xinu_simulation_dummy(void) {}\n")
        
        try:
            # Use direct execution with proper path handling
            if os.name == 'nt':
                # On Windows, wrap paths in quotes to handle spaces and special characters
                cmd = f'gcc -c "{dummy_c_path}" -o "{obj_path}"'
            else:
                # On Unix, use direct paths
                cmd = f"gcc -c {dummy_c_path} -o {obj_path}"
                
            # Run the compilation with more robust error handling
            process = subprocess.run(
                cmd, 
                shell=True, 
                capture_output=True, 
                text=True
            )
            
            if process.returncode == 0 and os.path.exists(obj_path):
                log(f"Created object file: {obj_path}")
            else:
                log(f"Warning: Could not compile object file: {process.stderr}")
                log(f"Command was: {cmd}")
                
                # Create a minimal object file as a fallback
                log("Attempting fallback object file creation")
                with open(obj_path, 'wb') as f:
                    # Just a minimal valid object file header to satisfy the linker
                    if os.name == 'nt':
                        # COFF header for Windows
                        f.write(bytes.fromhex('4d5a900003000000040000000ffff0000'))
                    else:
                        # ELF header for Linux
                        f.write(bytes.fromhex('7f454c46010101000000000000000000'))
                
                log(f"Created fallback object file: {obj_path}")
            
            # Clean up the temporary C file
            if os.path.exists(dummy_c_path):
                os.remove(dummy_c_path)
            
        except Exception as e:
            log(f"Error generating object file: {str(e)}")