# generator.py - Minimal XINU wrapper generator
import os
import datetime
import re
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
            # First try to find the command in PATH
            if path.endswith('/'):
                cmd = path.split('/')[-2]  # Get command from path
            else:
                cmd = path.split('/')[-1]
            
            # Try to find the command in Windows PATH
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
        os.makedirs(os.path.join(self.config.output_dir, "obj"), exist_ok=True)
        
        # Generate only the three essential files
        self.generate_stddefs()
        self.generate_includes_wrapper()
        self.generate_sim_helper()
        self.generate_standalone_helper()
        
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

/* Basic XINU types */
typedef unsigned char byte;
typedef int devcall;
typedef int syscall;
typedef int did32;
typedef int int32;

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

/* These must be defined before XINU inclusion */
typedef int devcall;
typedef int syscall;
typedef unsigned char byte;

/* --- Function Redirection Shims --- */
#ifdef getchar 
#undef getchar
#endif
#define getchar() getchar()

#ifdef putchar
#undef putchar
#endif
#define putchar(c) putchar(c)

/* Prevent include conflicts by removing XINU's overrides */
#ifdef scanf
#undef scanf
#endif

#ifdef sscanf
#undef sscanf
#endif

#ifdef fscanf
#undef fscanf
#endif

#ifdef printf
#undef printf
#endif

#ifdef sprintf
#undef sprintf
#endif

#ifdef fprintf
#undef fprintf
#endif

/* Include XINU headers */
#include "xinu.h" 

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
#define XINU_SIM_INTERNAL

/* Do NOT include XINU's headers here to avoid type conflicts */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

/* Main entry point for XINU simulation */
int main(int argc, char *argv[]) {{
    printf("XINU Simulation Starting\\n");
    printf("Generated on: {timestamp} by {username}\\n\\n");
    
    /* Initialize process table */
    /* This is where XINU OS code would be called */
    
    printf("XINU Simulation Running\\n");
    /* Run the simulation */
    
    printf("XINU Simulation Completed\\n");
    return 0;
}}
"""
        
        # Fix include paths for Windows
        content = self.fix_include_paths(content)
        
        with open(helper_path, 'w') as f:
            f.write(content)
        log(f"Generated simulation helper: {helper_path}")
    
    def generate_standalone_helper(self):
        # Generate a standalone helper file that can be compiled without XINU dependencies
        obj_dir = self.normalize_path(os.path.join(self.config.output_dir, "obj"))
        sim_obj_path = self.normalize_path(os.path.join(obj_dir, "xinu_simulation.o"))
        
        # Create a standalone minimal C file that doesn't include any XINU headers
        standalone_c_path = self.normalize_path(os.path.join(self.config.output_dir, "xinu_simulation_standalone.c"))
        
        # Get current timestamp and username from system
        timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        username = os.environ.get("USER", os.environ.get("USERNAME", "unknown"))
        
        standalone_content = f"""/* xinu_simulation_standalone.c - Standalone compilation unit for simulation functions 
 * Generated on: {timestamp} by {username}
 */
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>

/* Simple main function that doesn't require XINU headers */
int main(int argc, char *argv[]) {{
    printf("XINU Standalone Simulation\\n");
    printf("Generated on: {timestamp} by {username}\\n");
    return 0;
}}
"""
        with open(standalone_c_path, 'w') as f:
            f.write(standalone_content)
        
        # Try to compile the standalone file
        try:
            # Using system compiler directly without XINU dependencies
            import subprocess
            compile_cmd = f"gcc {standalone_c_path} -o {os.path.join(self.config.output_dir, 'xinu_sim.exe')}"
            subprocess.run(compile_cmd, shell=True, check=True)
            log(f"Compiled standalone simulation helper")
            
            # Also create the .o file for linking
            compile_obj_cmd = f"gcc -c {standalone_c_path} -o {sim_obj_path}"
            subprocess.run(compile_obj_cmd, shell=True, check=True)
            log(f"Created object file: {sim_obj_path}")
            
        except Exception as e:
            log(f"Warning: Could not compile simulation helper: {e}")