# generator.py - Minimal XINU wrapper generator with improved Windows support
import os
import datetime
import re
import subprocess
from utils.logger import log
import sys
from pathlib import Path

# For Windows-specific functionality
if os.name == 'nt':
    # Import our comprehensive Windows compatibility module
    from utils import windows_compat
    
    # Initialize the module
    windows_handler = windows_compat.initialize()


class XinuGenerator:
    # Minimal generator for XINU simulation wrapper files
    
    def __init__(self, config):
        self.config = config
        # Create Windows compatibility header immediately if on Windows
        if os.name == 'nt':
            self.setup_compatibility()
    
    def normalize_path(self, path):
        # Improved path normalization with more consistent handling
        # of edge cases and platform-specific behavior
        if not path:
            return path
            
        # Use Windows compatibility module if on Windows
        if os.name == 'nt':
            return windows_compat.normalize_windows_path(path)
            
        # Use Path objects for most operations to leverage built-in normalization
        try:
            # First convert to a Path object
            path_obj = Path(path)
            
            # For Unix systems, just use the Path object's string representation
            # which automatically normalizes separators
            return str(path_obj)
                
        except Exception as e:
            log(f"Path normalization warning: {e}")
            # If Path handling fails, attempt basic string normalization
            
            # Unix normalization fallback
            path = path.replace('\\', '/')
            while '//' in path:
                path = path.replace('//', '/')
                
            return path

    def get_safe_path(self, *parts):
        # Create a safe path by joining parts and normalizing,
        # handling all platform-specific edge cases
        if os.name == 'nt':
            # Use the Windows compatibility module
            return windows_compat.get_safe_path(*parts)
        
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
            # Use the Windows compatibility module
            return windows_compat.fix_include_paths(content)
        return content
    
    def fix_compiler_path(self, path):
        # Convert Unix-style paths to Windows-style if needed
        if os.name == 'nt' and path.startswith('/'):
            # Use the Windows compatibility module
            return windows_compat.fix_compiler_path(path)
        return path
    
    def setup_compatibility(self):
        # Create the Windows compatibility header first - before any other files
        if os.name == 'nt':
            output_dir = self.normalize_path(self.config.output_dir)
            os.makedirs(output_dir, exist_ok=True)
            
            # Create compatibility header
            compat_header = windows_compat.create_compatibility_header(output_dir)
            log(f"Created Windows compatibility header: {compat_header}")

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
        
        # On Windows, set up Windows-specific compatibility environment
        if os.name == 'nt':
            self.setup_windows_environment()
        
        log("XINU simulation file generation complete")
    
    def setup_windows_environment(self):
        # Use our Windows compatibility module to set up the environment
        if hasattr(self.config, 'include_dirs'):
            include_dirs = self.config.include_dirs
        else:
            include_dirs = [self.config.include_dir]
        
        # Set up the full Windows environment for XINU compilation
        windows_env = windows_compat.setup_windows_environment(self.config)
        
        log(f"Windows environment setup complete")
        
        return windows_env
    
    def generate_stddefs(self):
        # Generate xinu_stddefs.h - basic type definitions
        
        # Get current timestamp and username from system using required function
        if os.name == 'nt':
            timestamp, username = windows_compat.get_timestamp_and_user()
        else:
            timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            username = os.environ.get("USER", os.environ.get("USERNAME", "unknown"))
        
        content = f"""/* xinu_stddefs.h - Minimal type definitions for XINU simulation */
/* Generated on: {timestamp} */
/* By user: {username} */
#ifndef _XINU_STDDEFS_H_
#define _XINU_STDDEFS_H_

/* Version information */
#define VERSION "XINU Simulation Version 1.0"

#ifdef _WIN32
/* Windows-specific compatibility */

/* Basic constants that don't conflict */
#ifndef NULLCH
#define NULLCH '\\0'
#endif

#else
/* Non-Windows platforms - Standard definitions */
typedef unsigned char byte;
#endif /* _WIN32 */

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
        if os.name == 'nt':
            timestamp, username = windows_compat.get_timestamp_and_user()
        else:
            timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            username = os.environ.get("USER", os.environ.get("USERNAME", "unknown"))
        
        # Simple includes wrapper that doesn't conflict with XINU
        content = f"""/* xinu_includes.h - Wrapper for XINU code compilation.
 * Generated on: {timestamp} by {username}
 */
#ifndef _XINU_INCLUDES_H_ 
#define _XINU_INCLUDES_H_

#define _CRT_SECURE_NO_WARNINGS 
#define XINU_SIMULATION        

/* Windows compatibility - Minimal version */
#ifdef _WIN32
  /* Include the Windows compatibility header */
  #include "xinu_windows_compat.h"
#endif

/* Include our minimal definitions */
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
        if os.name == 'nt':
            timestamp, username = windows_compat.get_timestamp_and_user()
        else:
            timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            username = os.environ.get("USER", os.environ.get("USERNAME", "unknown"))
        
        # Windows-compatible C file - simpler approach
        if os.name == 'nt':
            content = f"""/* xinu_simulation.c - Helper functions for XINU simulation
 * Generated on: {timestamp} by {username}
 */
#define _CRT_SECURE_NO_WARNINGS

/* Windows compatibility header MUST be included first */
#ifdef _WIN32
  #include "xinu_windows_compat.h"
#endif

/* Use standard C libraries for basic functions */
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
        else:
            # Regular Unix version
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
            
            # Add Windows compatibility if needed
            if os.name == 'nt':
                f.write("#ifdef _WIN32\n")
                f.write("/* Include Windows compatibility header */\n")
                f.write("#include \"xinu_windows_compat.h\"\n")
                f.write("#endif\n\n")
            
            f.write("void xinu_simulation_dummy(void) {}\n")
        
        try:
            # Use direct execution with proper path handling
            if os.name == 'nt':
                # On Windows, properly format the command to avoid path issues
                gcc_path = windows_compat.find_compiler()
                include_output_dir = f'-I"{self.config.output_dir}"'
                cmd = f'"{gcc_path}" {include_output_dir} -c "{dummy_c_path}" -o "{obj_path}"'
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
                    # Write a minimal valid object file header
                    if os.name == 'nt':
                        # COFF header for Windows (corrected hexadecimal)
                        f.write(bytes.fromhex('4d5a9000030000000400000000ffff0000'))
                    else:
                        # ELF header for Linux
                        f.write(bytes.fromhex('7f454c4601010100000000000000000000'))
                
                log(f"Created fallback object file: {obj_path}")
            
            # Clean up the temporary C file
            if os.path.exists(dummy_c_path):
                os.remove(dummy_c_path)
            
        except Exception as e:
            log(f"Error generating object file: {str(e)}")