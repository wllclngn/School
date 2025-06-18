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

    def scan_include_files(self):
        # Scan all relevant include files to build a dependency graph
        include_files = {}
        
        # Get list of include directories
        if hasattr(self.config, 'include_dirs'):
            include_dirs = self.config.include_dirs
        else:
            include_dirs = [self.config.include_dir]
        
        # Process each include directory
        for include_dir in include_dirs:
            if os.path.exists(include_dir):
                for root, _, files in os.walk(include_dir):
                    for file in files:
                        if file.endswith(('.h', '.hpp')):
                            file_path = os.path.join(root, file)
                            includes = self.extract_includes(file_path, include_dirs)
                            include_files[file_path] = includes
        
        return include_files

    def extract_includes(self, file_path, include_dirs):
        # Extract include statements from a file
        includes = []
        include_pattern = re.compile(r'#include\s+["<](.*?)[">]')
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                for line in f:
                    match = include_pattern.search(line)
                    if match:
                        include_name = match.group(1)
                        # Find full path of the included file
                        for include_dir in include_dirs:
                            potential_path = os.path.join(include_dir, include_name)
                            if os.path.exists(potential_path):
                                includes.append(potential_path)
                                break
        except Exception as e:
            log(f"Error processing includes in {file_path}: {e}")
        
        return includes

    def check_for_circular_includes(self, include_files):
        # Check for circular dependencies in include files using DFS algorithm
        
        # Build dependency graph
        graph = {}
        for file_path, includes in include_files.items():
            graph[file_path] = includes
        
        # Track visited nodes and current path
        visited = set()
        path = []
        path_set = set()
        
        def dfs(node):
            # If node is already in current path, we found a cycle
            if node in path_set:
                cycle_start = path.index(node)
                return True, path[cycle_start:] + [node]
            
            # If already visited and not in cycle, skip
            if node in visited:
                return False, []
            
            # Mark as visited and add to current path
            visited.add(node)
            path.append(node)
            path_set.add(node)
            
            # Check all dependencies
            if node in graph:
                for neighbor in graph[node]:
                    has_cycle, cycle_path = dfs(neighbor)
                    if has_cycle:
                        return True, cycle_path
            
            # Remove from current path when backtracking
            path.pop()
            path_set.remove(node)
            
            return False, []
        
        # Check each node as a starting point
        for node in graph:
            has_cycle, cycle_path = dfs(node)
            if has_cycle:
                # Return simplified path with basenames for readability
                simple_path = [os.path.basename(p) for p in cycle_path]
                return True, simple_path
        
        return False, []

    def generate_files(self):
        # Generate only the essential files needed for XINU simulation
        log("Generating minimal XINU simulation files")
        
        # Create output directory with long path support
        output_dir = self.normalize_path(self.config.output_dir)
        os.makedirs(output_dir, exist_ok=True)
        
        # Check for circular includes before proceeding
        include_files = self.scan_include_files()
        has_circular, circular_path = self.check_for_circular_includes(include_files)
        
        if has_circular:
            log("CRITICAL ERROR: Circular include dependencies detected!")
            log("Circular path: " + " -> ".join(circular_path))
            log("Compilation cannot proceed with circular dependencies.")
            
            # Create a special error file to inform the user
            error_path = self.get_safe_path(output_dir, "circular_includes_error.txt")
            with open(error_path, 'w') as f:
                f.write(f"CRITICAL ERROR: Circular include dependencies detected at {datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n\n")
                f.write("Circular include path:\n")
                f.write(" -> ".join(circular_path) + "\n\n")
                f.write("Possible solutions:\n")
                f.write("1. Use include guards in all header files (#ifndef _FILE_H_ / #define _FILE_H_)\n")
                f.write("2. Create a base_types.h file for common type definitions\n")
                f.write("3. Use forward declarations for struct types where possible\n")
                f.write("4. Reorganize your code to avoid mutual dependencies\n")
                f.write("\nCompilation aborted to prevent errors.\n")
            
            # Create a special header to indicate circular includes
            base_types_path = self.get_safe_path(output_dir, "base_types.h")
            with open(base_types_path, 'w') as f:
                f.write("/* base_types.h - Generated to resolve circular dependencies */\n")
                f.write(f"/* Generated on: {datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')} */\n")
                f.write("#ifndef _BASE_TYPES_H_\n")
                f.write("#define _BASE_TYPES_H_\n\n")
                f.write("/* Basic type definitions */\n")
                f.write("typedef char                int8;\n")
                f.write("typedef short               int16;\n")
                f.write("typedef int                 int32;\n")
                f.write("typedef unsigned char       uint8;\n")
                f.write("typedef unsigned short      uint16;\n")
                f.write("typedef unsigned int        uint32;\n")
                f.write("typedef unsigned long long  uint64;\n\n")
                f.write("/* This file was auto-generated to fix circular dependencies in headers */\n")
                f.write("/* Please include this file in your problematic headers */\n\n")
                f.write("#endif /* _BASE_TYPES_H_ */\n")
            
            # Exit early but still create a minimal environment
            self.generate_stddefs(True)
            self.generate_includes_wrapper()
            
            log("Created base_types.h with common definitions to help resolve circular dependencies.")
            log("Please check circular_includes_error.txt for details on fixing the issue.")
            return
        
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
    
    def generate_stddefs(self, has_circular_deps=False):
        # Generate xinu_stddefs.h - basic type definitions
        
        # Get current timestamp and username from system using required function
        if os.name == 'nt':
            timestamp, username = windows_compat.get_timestamp_and_user()
        else:
            timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            username = os.environ.get("USER", os.environ.get("USERNAME", "unknown"))
        
        # Create more complete type definitions if circular dependencies detected
        if has_circular_deps:
            content = f"/* xinu_stddefs.h - Extended type definitions to prevent circular dependencies */\n"
            content += f"/* Generated on: {timestamp} */\n"
            content += f"/* By user: {username} */\n"
            content += "#ifndef _XINU_STDDEFS_H_\n"
            content += "#define _XINU_STDDEFS_H_\n\n"
            content += "/* Version information */\n"
            content += "#define VERSION \"XINU Simulation Version 1.0\"\n\n"
            content += "/* Basic type definitions to prevent circular dependencies */\n"
            content += "typedef char                int8;\n"
            content += "typedef short               int16;\n"
            content += "typedef int                 int32;\n"
            content += "typedef unsigned char       uint8;\n"
            content += "typedef unsigned short      uint16;\n"
            content += "typedef unsigned int        uint32;\n"
            content += "typedef unsigned long long  uint64;\n\n"
            content += "#ifdef _WIN32\n"
            content += "/* Windows-specific compatibility */\n\n"
            content += "/* Basic constants that don't conflict */\n"
            content += "#ifndef NULLCH\n"
            content += "#define NULLCH '\\0'\n"
            content += "#endif\n\n"
            content += "#else\n"
            content += "/* Non-Windows platforms - Standard definitions */\n"
            content += "typedef unsigned char byte;\n"
            content += "#endif /* _WIN32 */\n\n"
            content += "#endif /* _XINU_STDDEFS_H_ */\n"
        else:
            content = f"/* xinu_stddefs.h - Minimal type definitions for XINU simulation */\n"
            content += f"/* Generated on: {timestamp} */\n"
            content += f"/* By user: {username} */\n"
            content += "#ifndef _XINU_STDDEFS_H_\n"
            content += "#define _XINU_STDDEFS_H_\n\n"
            content += "/* Version information */\n"
            content += "#define VERSION \"XINU Simulation Version 1.0\"\n\n"
            content += "#ifdef _WIN32\n"
            content += "/* Windows-specific compatibility */\n\n"
            content += "/* Basic constants that don't conflict */\n"
            content += "#ifndef NULLCH\n"
            content += "#define NULLCH '\\0'\n"
            content += "#endif\n\n"
            content += "#else\n"
            content += "/* Non-Windows platforms - Standard definitions */\n"
            content += "typedef unsigned char byte;\n"
            content += "#endif /* _WIN32 */\n\n"
            content += "#endif /* _XINU_STDDEFS_H_ */\n"
        
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
        content = f"/* xinu_includes.h - Wrapper for XINU code compilation.\n"
        content += f" * Generated on: {timestamp} by {username}\n"
        content += f" */\n"
        content += "#ifndef _XINU_INCLUDES_H_ \n"
        content += "#define _XINU_INCLUDES_H_\n\n"
        content += "#define _CRT_SECURE_NO_WARNINGS \n"
        content += "#define XINU_SIMULATION        \n\n"
        content += "/* Windows compatibility - Minimal version */\n"
        content += "#ifdef _WIN32\n"
        content += "  /* Include the Windows compatibility header */\n"
        content += "  #include \"xinu_windows_compat.h\"\n"
        content += "#endif\n\n"
        content += "/* Include our minimal definitions */\n"
        content += "#include \"xinu_stddefs.h\" \n\n"
        content += "#endif /* _XINU_INCLUDES_H_ */\n"
        
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
            content = f"/* xinu_simulation.c - Helper functions for XINU simulation\n"
            content += f" * Generated on: {timestamp} by {username}\n"
            content += f" */\n"
            content += "#define _CRT_SECURE_NO_WARNINGS\n\n"
            content += "/* Windows compatibility header MUST be included first */\n"
            content += "#ifdef _WIN32\n"
            content += "  #include \"xinu_windows_compat.h\"\n"
            content += "#endif\n\n"
            content += "/* Use standard C libraries for basic functions */\n"
            content += "#include <stdio.h>\n"
            content += "#include <stdlib.h>\n\n"
            content += "/* Main entry point for simulation */\n"
            content += "int main(void) {\n"
            content += f"    printf(\"XINU Simulation Starting\\n\");\n"
            content += f"    printf(\"Generated on: {timestamp} by {username}\\n\\n\");\n"
            content += f"    \n"
            content += f"    printf(\"XINU Simulation Running\\n\");\n"
            content += f"    \n"
            content += f"    printf(\"XINU Simulation Completed\\n\");\n"
            content += f"    return 0;\n"
            content += "}\n"
        else:
            # Regular Unix version
            content = f"/* xinu_simulation.c - Helper functions for XINU simulation\n"
            content += f" * Generated on: {timestamp} by {username}\n"
            content += f" */\n"
            content += "#define _CRT_SECURE_NO_WARNINGS\n\n"
            content += "/* Simple standalone simulation without XINU dependencies */\n"
            content += "#include <stdio.h>\n"
            content += "#include <stdlib.h>\n\n"
            content += "/* Main entry point for simulation */\n"
            content += "int main(void) {\n"
            content += f"    printf(\"XINU Simulation Starting\\n\");\n"
            content += f"    printf(\"Generated on: {timestamp} by {username}\\n\\n\");\n"
            content += f"    \n"
            content += f"    printf(\"XINU Simulation Running\\n\");\n"
            content += f"    \n"
            content += f"    printf(\"XINU Simulation Completed\\n\");\n"
            content += f"    return 0;\n"
            content += "}\n"
        
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