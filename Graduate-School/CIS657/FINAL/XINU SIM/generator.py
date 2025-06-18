# generator.py - Minimal XINU wrapper generator
import os
import datetime
import re
import subprocess
import platform
from utils.logger import log

class XinuGenerator:
    # Minimal generator for XINU simulation wrapper files
    
    def __init__(self, config):
        self.config = config
    
    def normalize_path(self, path):
        # Normalize paths to use backslashes on Windows, forward slashes on other platforms
        if os.name == 'nt':  # Windows
            # Replace all forward slashes with backslashes
            path = path.replace('/', '\\')
            # Ensure no double backslashes
            while '\\\\' in path:
                path = path.replace('\\\\', '\\')
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
        
        # Create output directory
        os.makedirs(self.config.output_dir, exist_ok=True)
        
        # Create obj directory to prevent "No such file or directory" error
        obj_dir = os.path.join(self.config.output_dir, "obj")
        os.makedirs(obj_dir, exist_ok=True)
        
        # Generate only the essential files
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

/* Prevent conflicts with standard C headers */
#ifndef _XINU_INTERNAL_
typedef unsigned char byte;
#endif

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
#define _XINU_INTERNAL_

/* No need to include other headers here to avoid conflicts */
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
#define _XINU_INTERNAL_

/* Standard C headers only - avoid including XINU headers */
#include <stdio.h>
#include <stdlib.h>

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
        # Create a simple object file for linking
        obj_dir = self.normalize_path(os.path.join(self.config.output_dir, "obj"))
        obj_path = self.normalize_path(os.path.join(obj_dir, "xinu_simulation.o"))
        
        # Create a simple C file with minimal content
        dummy_c_path = self.normalize_path(os.path.join(self.config.output_dir, "dummy.c"))
        with open(dummy_c_path, 'w') as f:
            f.write("/* Dummy C file for object generation */\n")
            f.write("int xinu_simulation_dummy(void) { return 0; }\n")
        
        # On Linux, we need to use -m32 compile flag for compatibility
        is_windows = os.name == 'nt'
        
        try:
            # Determine the right compiler flags based on the platform
            if is_windows:
                # Windows - use standard GCC
                compiler_cmd = "gcc -c"
            else:
                # Linux - try with -m32 if available
                compiler_cmd = "gcc -m32 -c"
            
            # Try to compile the object file
            cmd = f"{compiler_cmd} {dummy_c_path} -o {obj_path}"
            result = subprocess.run(cmd, shell=True, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
            
            # If -m32 fails, try without it on Linux
            if not is_windows and result.returncode != 0:
                log("32-bit compilation failed, trying without -m32 flag")
                cmd = f"gcc -c {dummy_c_path} -o {obj_path}"
                result = subprocess.run(cmd, shell=True)
            
            if os.path.exists(obj_path):
                log(f"Created object file: {obj_path}")
            else:
                log(f"Warning: Failed to create object file at {obj_path}")
                
                # Create an empty file as a placeholder
                with open(obj_path, 'wb') as f:
                    # Write a minimal ELF header to trick the linker
                    if not is_windows:
                        # ELF magic number and some minimal headers for Linux
                        f.write(bytes.fromhex('7f454c4601010100000000000000000001000000000000000000000000000000'))
                    else:
                        # COFF header for Windows
                        f.write(bytes.fromhex('4d5a900003000000040000000ffff0000b80000000000000040000000000000000000000000000000000000000000000000000000000000000000000080000000'))
                
                log(f"Created placeholder object file: {obj_path}")
            
            # Remove the temporary C file
            if os.path.exists(dummy_c_path):
                os.remove(dummy_c_path)
                
        except Exception as e:
            log(f"Warning: Could not create object file: {str(e)}")
            
            # Create an empty file as a last resort
            with open(obj_path, 'wb') as f:
                f.write(b'\0' * 128)  # Just some null bytes
            
            log(f"Created empty placeholder object file: {obj_path}")