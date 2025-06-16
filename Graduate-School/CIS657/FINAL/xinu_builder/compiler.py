"""
compiler.py - Compile and link XINU source files
"""

import os
import re
import glob
import subprocess
from xinu_builder.utils.logger import log

class XinuCompiler:
    """Compiler for XINU simulation"""
    
    def __init__(self, config):
        self.config = config
        self.error_limit = 20
        self.warnings = {}  # Track unique warnings
        
        # Functions to exclude from compilation
        self.exclude_funcs = [
            "printf", "fprintf", "sprintf", "scanf", "fscanf", "sscanf",
            "getchar", "putchar", "fgetc", "fgets", "fputc", "fputs",
            "_doprnt", "_doscan", "abs", "labs", "atoi", "atol", 
            "rand", "srand", "qsort", "strcpy", "strncpy", "strcat", 
            "strncat", "strcmp", "strncmp", "strlen", "strnlen", 
            "strchr", "strrchr", "strstr", "memcpy", "memmove", 
            "memcmp", "memset"
        ]
        
    def build(self):
        """Build XINU simulation"""
        # Ensure output directories exist
        os.makedirs(self.config.obj_dir, exist_ok=True)
        
        # Collect source files
        srcfiles = self.collect_source_files()
        if not srcfiles:
            log("No source files found to compile.")
            return False
        
        # Compile each file
        obj_files = self.compile_files(srcfiles)
        if not obj_files:
            log("No object files generated. Compilation failed.")
            return False
        
        # Link object files
        success = self.link_files(obj_files)
        if not success:
            log("Linking failed.")
            return False
            
        # Final summary
        num_warnings = len(self.warnings)
        log(f"SUCCESS: Build completed with 0 errors and {num_warnings} unique warning(s).")
        log(f"XINU core executable: {self.config.xinu_core_output}")
        return True
        
    def collect_source_files(self):
        """Collect XINU source files for compilation"""
        log("Collecting XINU C source files...")
        
        srcfiles = []
        
        # Try to parse Makefile first
        makefile_path = os.path.join(self.config.project_dir, "compile/Makefile")
        if os.path.exists(makefile_path):
            srcfiles = self._parse_makefile_for_sources(makefile_path)
            
        # If Makefile parsing failed or found no files, scan directories
        if not srcfiles:
            log("No files found in Makefile, falling back to directory scan.")
            srcfiles = self._scan_directories_for_sources()
        
        # Always include these files if they exist
        core_files = [
            self.config.xinu_core_c,
            self.config.sim_helper_c
        ]
        
        for file_path in core_files:
            if os.path.exists(file_path) and file_path not in srcfiles:
                srcfiles.append(file_path)
                
        # Filter out library functions that will be shimmed
        filtered_srcfiles = []
        for src_path in srcfiles:
            basename = os.path.basename(src_path)
            basename_no_ext = os.path.splitext(basename)[0]
            
            if basename_no_ext in self.exclude_funcs and os.path.dirname(src_path) == self.config.libxc_dir:
                log(f"Excluding libxc source for shimmed function: {src_path}")
            else:
                filtered_srcfiles.append(src_path)
                
        log(f"Total XINU C source files for compilation: {len(filtered_srcfiles)}")
        return filtered_srcfiles
        
    def _parse_makefile_for_sources(self, makefile_path):
        """Parse Makefile for source file lists"""
        srcfiles = []
        
        try:
            with open(makefile_path, 'r') as f:
                makefile_content = f.read()
                
            # Define source file variables to look for
            var_patterns = {
                "SYSTEM_CFILES": self.config.system_dir,
                "TTY_CFILES": os.path.join(self.config.project_dir, "device/tty"),
                "SHELL_CFILES": self.config.shell_dir,
                "LIBXCCFILES": self.config.libxc_dir
            }
            
            for var_name, prefix in var_patterns.items():
                # Extract variable value using regex
                pattern = fr'{var_name}\s*=\s*(.*?)(?=\n[A-Z0-9_]+=|\Z)'
                match = re.search(pattern, makefile_content, re.DOTALL)
                
                if match:
                    value_str = match.group(1).strip()
                    # Handle line continuations and comments
                    value_str = re.sub(r'\\[\r\n]+\s*', ' ', value_str)
                    value_str = re.sub(r'#.*$', '', value_str, flags=re.MULTILINE)
                    
                    # Extract .c file names
                    files = re.findall(r'\S+\.c', value_str)
                    
                    for file_name in files:
                        file_path = os.path.join(prefix, file_name.strip())
                        if os.path.exists(file_path):
                            srcfiles.append(file_path)
                        else:
                            log(f"Warning: Source file '{file_path}' listed in Makefile not found.")
                            
        except Exception as e:
            log(f"Error parsing Makefile: {str(e)}")
            
        return srcfiles
        
    def _scan_directories_for_sources(self):
        """Scan directories for source files"""
        srcfiles = []
        
        # Define directories to scan
        dirs_to_scan = [
            self.config.system_dir,
            os.path.join(self.config.project_dir, "device/tty"),
            self.config.shell_dir,
            self.config.libxc_dir
        ]
        
        # Find all .c files
        for directory in dirs_to_scan:
            if os.path.exists(directory):
                for root, _, files in os.walk(directory):
                    for file in files:
                        if file.endswith('.c'):
                            srcfiles.append(os.path.join(root, file))
                            
        return srcfiles
        
    def compile_files(self, srcfiles):
        """Compile all source files"""
        log("Building XINU Core Process...")
        obj_files = []
        error_count = 0
        
        # Setup compiler options
        base_includes = f"-I{self.config.include_dir} -I{self.config.project_dir}"
        gcc_force_includes = f"-include {self.config.includes_h}"
        
        for src in srcfiles:
            obj = os.path.join(self.config.obj_dir, os.path.basename(src).replace('.c', '.o'))
            
            compile_options = f"{base_includes} -Wall -Wextra -g -O0"
            if src == self.config.sim_helper_c:
                cmd = f"gcc -c {src} {compile_options} -o {obj}"
            else:
                cmd = f"gcc -c {src} {gcc_force_includes} {compile_options} -o {obj}"
                
            print(f"Compiling {src} -> {obj}")
            
            # Run the compilation
            try:
                process = subprocess.Popen(
                    cmd, 
                    shell=True, 
                    stdout=subprocess.PIPE, 
                    stderr=subprocess.PIPE,
                    universal_newlines=True
                )
                stdout, stderr = process.communicate()
                
                # Process the output for errors and warnings
                errors_in_file = 0
                
                for line in stderr.splitlines():
                    if "error:" in line.lower():
                        print(line)
                        log(line)
                        errors_in_file += 1
                    elif "warning:" in line.lower():
                        warning_key = line.strip()
                        if warning_key not in self.warnings:
                            print(line)
                            log(line)
                            self.warnings[warning_key] = True
                
                # Check if compilation succeeded
                if process.returncode == 0:
                    obj_files.append(obj)
                
                error_count += errors_in_file
                if error_count >= self.error_limit:
                    log(f"Compilation aborted after {error_count} errors (limit: {self.error_limit}).")
                    return []
            except Exception as e:
                log(f"Error compiling {src}: {str(e)}")
                error_count += 1
                
        return obj_files
        
    def link_files(self, obj_files):
        """Link object files into executable"""
        if not obj_files:
            log("No object files to link.")
            return False
            
        log(f"Linking {len(obj_files)} objects to {self.config.xinu_core_output}")
        
        # Construct the link command
        link_cmd = "gcc " + " ".join(obj_files) + f" -o {self.config.xinu_core_output} -lm"
        
        try:
            process = subprocess.Popen(
                link_cmd,
                shell=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                universal_newlines=True
            )
            stdout, stderr = process.communicate()
            
            # Process the output
            link_errors = 0
            
            for line in stderr.splitlines():
                if "error:" in line.lower():
                    print(line)
                    log(line)
                    link_errors += 1
                elif "warning:" in line.lower():
                    warning_key = line.strip()
                    if warning_key not in self.warnings:
                        print(line)
                        log(line)
                        self.warnings[warning_key] = True
            
            if link_errors > 0:
                log(f"Linking failed with {link_errors} errors.")
                return False
                
            # Make the output executable (Unix-like systems)
            if not self.config.is_windows() and os.path.exists(self.config.xinu_core_output):
                os.chmod(self.config.xinu_core_output, 0o755)
                
            return process.returncode == 0
        except Exception as e:
            log(f"Error during linking: {str(e)}")
            return False
