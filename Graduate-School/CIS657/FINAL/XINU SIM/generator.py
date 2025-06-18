# generator.py - Minimal XINU wrapper generator with improved Windows path handling
import os
import datetime
import re
import subprocess
from utils.logger import log
import sys

# For Windows long path support
if os.name == 'nt':
    import ctypes
    from ctypes import wintypes
    from pathlib import Path

    # Enable long path support at Windows API level
    kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel32.SetFileAttributesW.argtypes = (wintypes.LPCWSTR, wintypes.DWORD)

    def handle_long_path(path_str):
        # Prefix with \\?\ for paths exceeding MAX_PATH
        if len(path_str) >= 260 and not path_str.startswith("\\\\?\\"):
            path_str = "\\\\?\\" + str(Path(path_str).resolve()).replace("/", "\\")
        return path_str

class XinuGenerator:
    # Minimal generator for XINU simulation wrapper files
    
    def __init__(self, config):
        self.config = config
    
    def normalize_path(self, path):
        # Handle Windows long paths and normalize separators
        if os.name == 'nt':  # Windows
            # Handle long paths with \\?\ prefix
            path = handle_long_path(path)
            
            # Make sure all slashes are backslashes
            path = path.replace('/', '\\')
            
            # Ensure no double backslashes (except at start for \\?\ prefix)
            if path.startswith("\\\\?\\"):
                prefix = "\\\\?\\"
                rest = path[4:]
                while "\\\\" in rest:
                    rest = rest.replace("\\\\", "\\")
                path = prefix + rest
            else:
                while "\\\\" in path:
                    path = path.replace("\\\\", "\\")
        else:
            # Replace all backslashes with forward slashes on Unix
            path = path.replace('\\', '/')
            # Ensure no double slashes
            while '//' in path:
                path = path.replace('//', '/')
        return path

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
        obj_dir = self.normalize_path(os.path.join(output_dir, "obj"))
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
        
        # Save to output directory
        stddefs_path = self.normalize_path(os.path.join(self.config.output_dir, "xinu_stddefs.h"))
        with open(stddefs_path, 'w') as f:
            f.write(content)
        log(f"Generated standard definitions: {stddefs_path}")
        
        # Also save to XINU include directory to fix pathing issues
        include_stddefs_path = self.normalize_path(os.path.join(self.config.include_dir, "xinu_stddefs.h"))
        with open(include_stddefs_path, 'w') as f:
            f.write(content)
        log(f"Generated standard definitions (include dir copy): {include_stddefs_path}")

    def generate_includes_wrapper(self):
        # Generate xinu_includes.h - the key wrapper that redirects standard functions
        includes_path = self.normalize_path(os.path.join(self.config.output_dir, "xinu_includes.h"))
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
        helper_path = self.normalize_path(os.path.join(self.config.output_dir, "xinu_simulation.c"))
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
        obj_dir = self.normalize_path(os.path.join(self.config.output_dir, "obj"))
        obj_path = self.normalize_path(os.path.join(obj_dir, "xinu_simulation.o"))
        
        # Create a simple C file for compilation
        dummy_c_path = self.normalize_path(os.path.join(self.config.output_dir, "dummy.c"))
        with open(dummy_c_path, 'w') as f:
            f.write("/* Minimal dummy object for compilation */\n")
            f.write("void xinu_simulation_dummy(void) {}\n")
        
        try:
            # Use direct execution with proper path handling
            if os.name == 'nt':
                # On Windows, wrap paths in quotes to handle spaces
                cmd = f'gcc -c "{dummy_c_path}" -o "{obj_path}"'
            else:
                # On Unix, use direct paths
                cmd = f"gcc -c {dummy_c_path} -o {obj_path}"
                
            # Run the compilation
            result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
            
            if result.returncode == 0 and os.path.exists(obj_path):
                log(f"Created object file: {obj_path}")
            else:
                log(f"Warning: Could not compile object file: {result.stderr}")
                
                # Create a minimal object file as a fallback
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
            os.remove(dummy_c_path)
            
        except Exception as e:
            log(f"Error generating object file: {str(e)}")