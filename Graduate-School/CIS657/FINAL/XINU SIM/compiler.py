# compiler.py - Compile and link XINU source files

import os
import re
import glob
import subprocess
import datetime
from xinu_sim.utils.logger import log

class XinuCompiler:
    """Compiler for XINU simulation"""
    
    def __init__(self, config):
        self.config = config
        self.error_limit = 20
        self.warnings = {}  # Track unique warnings
        self.compilation_log = []  # For storing compilation messages
        
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
        
        # Start compilation log
        timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        self._log(f"=== XINU Compilation Log ===")
        self._log(f"Started: {os.environ.get('USER', 'unknown')} at {timestamp}")
        self._log(f"Project directory: {self.config.project_dir}")
        self._log(f"System directory: {self.config.system_dir}")
        self._log(f"Output directory: {self.config.output_dir}")
        
        # Try parsing the Makefile for build instructions
        from xinu_sim.makefile_parser import MakefileParser
        makefile_parser = MakefileParser(self.config)
        if makefile_parser.parse_makefile():
            log("Successfully parsed Makefile for build instructions")
            self._log("Using build instructions from Makefile")
            
            # Get source files from Makefile
            makefile_sources = makefile_parser.get_resolved_source_files()
            if makefile_sources:
                log(f"Found {len(makefile_sources)} source files defined in Makefile")
                self._log(f"Found {len(makefile_sources)} source files defined in Makefile")
                
                # Add our core files
                core_files = [
                    self.config.xinu_core_c,
                    self.config.sim_helper_c
                ]
                
                for file_path in core_files:
                    if os.path.exists(file_path) and file_path not in makefile_sources:
                        makefile_sources.append(file_path)
                        self._log(f"Added core file: {file_path}")
                
                # Use Makefile sources
                srcfiles = makefile_sources
            else:
                # Fall back to collecting sources normally
                log("No source files found in Makefile, falling back to source scanning")
                self._log("No source files found in Makefile, falling back to source scanning")
                srcfiles = self.collect_source_files()
        else:
            # Fall back to collecting sources normally
            log("Makefile not found or couldn't be parsed, falling back to source scanning")
            self._log("Makefile not found or couldn't be parsed, falling back to source scanning")
            srcfiles = self.collect_source_files()
        
        if not srcfiles:
            log("No source files found to compile.")
            self._log("ERROR: No source files found to compile.")
            self._save_compilation_log()
            return False
        
        # Compile each file
        obj_files = self.compile_files(srcfiles, makefile_parser if makefile_parser.parse_makefile() else None)
        if not obj_files:
            log("No object files generated. Compilation failed.")
            self._log("ERROR: No object files generated. Compilation failed.")
            self._save_compilation_log()
            return False
        
        # Link object files
        success = self.link_files(obj_files, makefile_parser if makefile_parser.parse_makefile() else None)
        if not success:
            log("Linking failed.")
            self._log("ERROR: Linking failed.")
            self._save_compilation_log()
            return False
            
        # Final summary
        num_warnings = len(self.warnings)
        success_msg = f"SUCCESS: Build completed with 0 errors and {num_warnings} unique warning(s)."
        log(success_msg)
        self._log(success_msg)
        self._log(f"XINU core executable: {self.config.xinu_core_output}")
        
        # Save compilation log
        self._save_compilation_log()
        return True
        
    def _log(self, message):
        """Add a message to the compilation log"""
        self.compilation_log.append(message)
        
    def _save_compilation_log(self):
        """Save compilation log to file"""
        log_file = self.config.compilation_log
        try:
            with open(log_file, 'w') as f:
                f.write('\n'.join(self.compilation_log))
            log(f"Compilation log saved to {log_file}")
        except Exception as e:
            log(f"Error saving compilation log: {str(e)}")
        
    def _collect_minimal_source_files(self):
        """Collect only the minimal source files needed for a basic simulation"""
        srcfiles = []
        
        # Only include the generated files
        core_files = [
            self.config.xinu_core_c,
            self.config.sim_helper_c
        ]
        
        for file_path in core_files:
            if os.path.exists(file_path):
                srcfiles.append(file_path)
                self._log(f"Added core file: {file_path}")
        
        log(f"Using minimal set of {len(srcfiles)} source files for simulation")
        self._log(f"Using minimal set of {len(srcfiles)} source files for simulation")
        return srcfiles
        
    def collect_source_files(self):
        """Collect XINU C source files for full compilation"""
        log("Collecting XINU C source files...")
        self._log("\n=== Source Files ===")
        
        srcfiles = []
        
        # Use XINU OS directories directly
        self._log("Scanning directories for source files...")
        srcfiles = self._scan_directories_for_sources()
        
        # Always include these files if they exist
        core_files = [
            self.config.xinu_core_c,
            self.config.sim_helper_c
        ]
        
        for file_path in core_files:
            if os.path.exists(file_path) and file_path not in srcfiles:
                srcfiles.append(file_path)
                self._log(f"Added core file: {file_path}")
                
        # Filter out assembly files and library functions that will be shimmed
        filtered_srcfiles = []
        for src_path in srcfiles:
            basename = os.path.basename(src_path)
            basename_no_ext = os.path.splitext(basename)[0]
            
            if basename.endswith(('.S', '.s', '.asm')):
                log(f"Skipping assembly file: {src_path}")
                self._log(f"Skipping assembly file: {src_path}")
                continue
                
            if basename_no_ext in self.exclude_funcs and os.path.dirname(src_path) == self.config.libxc_dir:
                log(f"Excluding libxc source for shimmed function: {src_path}")
                self._log(f"Excluding libxc source for shimmed function: {src_path}")
            else:
                filtered_srcfiles.append(src_path)
                self._log(f"Adding source file: {src_path}")
                
        log(f"Total XINU C source files for compilation: {len(filtered_srcfiles)}")
        self._log(f"Total XINU C source files for compilation: {len(filtered_srcfiles)}")
        return filtered_srcfiles
        
    def _scan_directories_for_sources(self):
        """Scan directories for source files"""
        srcfiles = []
        
        # Define directories to scan
        dirs_to_scan = [
            self.config.system_dir,
            os.path.join(self.config.device_dir, "tty") if os.path.exists(self.config.device_dir) else None,
            self.config.shell_dir,
            self.config.libxc_dir
        ]
        
        # Filter out None values (directories that don't exist)
        dirs_to_scan = [d for d in dirs_to_scan if d]
        
        # Find all .c files
        for directory in dirs_to_scan:
            if os.path.exists(directory):
                self._log(f"Scanning directory: {directory}")
                count = 0
                for root, _, files in os.walk(directory):
                    for file in files:
                        if file.endswith('.c'):
                            srcfiles.append(os.path.join(root, file))
                            count += 1
                self._log(f"  Found {count} .c files")
            else:
                self._log(f"Directory does not exist: {directory}")
                            
        return srcfiles
        
    def compile_files(self, srcfiles, makefile_parser=None):
        """Compile all source files"""
        log("Building XINU Core Process...")
        self._log("\n=== Compilation ===")
        obj_files = []
        error_count = 0
        
        # Setup compiler options
        include_dirs = []
        compiler_flags = []
        
        # If we have Makefile info, use it
        if makefile_parser:
            include_dirs = makefile_parser.get_resolved_include_dirs()
            compiler_flags = makefile_parser.get_compiler_flags()
        
        # If no Makefile info or it's incomplete, add our standard include dirs
        if not include_dirs:
            include_dirs = [
                self.config.output_dir,                      # For generated files
                os.path.dirname(self.config.xinu_h),         # For xinu.h location
                self.config.include_dir.replace(" ", "\\ ")  # For XINU OS include files
            ]
        
        # If no Makefile compiler flags, use our defaults
        if not compiler_flags:
            compiler_flags = ["-Wall", "-Wextra", "-g", "-O0", "-w", "-fno-builtin"]
        
        # Build include options
        include_opts = " ".join(f"-I{d}" for d in include_dirs if os.path.exists(d))
        compiler_opts = " ".join(compiler_flags)
                
        pre_header_path = os.path.join(self.config.output_dir, "xinu_pre.h")
        # Create a simple pre-header if it doesn't exist
        with open(pre_header_path, 'w') as f:
            f.write("/* Auto-generated xinu_pre.h */\n")
            f.write("#define NULL 0\n")
            f.write("#define OK 1\n")
            f.write("#define SYSERR -1\n")
            f.write("typedef int process;\n")
            f.write("typedef int syscall;\n")
            f.write("typedef int pid32;\n")
        
        # Compile each source file
        for src in srcfiles:
            obj = os.path.join(self.config.obj_dir, os.path.basename(src).replace('.c', '.o'))
            self._log(f"\nCompiling {src} -> {obj}")
            
            compile_options = f"{include_opts} {compiler_opts}"
            cmd = f"gcc -c {src} {compile_options} -o {obj}"
            
            self._log(f"Command: {cmd}")
            
            try:
                process = subprocess.Popen(
                    cmd, 
                    shell=True, 
                    stdout=subprocess.PIPE, 
                    stderr=subprocess.PIPE,
                    universal_newlines=True
                )
                stdout, stderr = process.communicate()
                
                # Process output for errors and warnings
                errors_in_file = 0
                for line in stderr.splitlines():
                    if "error:" in line.lower():
                        print(line)
                        log(line)
                        self._log(f"ERROR: {line}")
                        errors_in_file += 1
                    elif "warning:" in line.lower():
                        warning_key = line.strip()
                        if warning_key not in self.warnings:
                            print(line)
                            log(line)
                            self._log(f"WARNING: {line}")
                            self.warnings[warning_key] = True
                
                # Check if compilation succeeded
                if process.returncode == 0:
                    obj_files.append(obj)
                    self._log(f"Successfully compiled {src}")
                else:
                    self._log(f"Failed to compile {src}")
                    
                error_count += errors_in_file
                if error_count >= self.error_limit:
                    log(f"Compilation aborted after {error_count} errors (limit: {self.error_limit}).")
                    self._log(f"Compilation aborted after {error_count} errors (limit: {self.error_limit}).")
                    return []
            except Exception as e:
                log(f"Error compiling {src}: {str(e)}")
                self._log(f"Error compiling {src}: {str(e)}")
                error_count += 1
        
        return obj_files
        
    def link_files(self, obj_files, makefile_parser=None):
        """Link object files into executable"""
        if not obj_files:
            log("No object files to link.")
            self._log("No object files to link.")
            return False
            
        log(f"Linking {len(obj_files)} objects to {self.config.xinu_core_output}")
        self._log(f"\n=== Linking ===")
        self._log(f"Linking {len(obj_files)} objects to {self.config.xinu_core_output}")
        
        # Get linker flags from Makefile if available
        linker_flags = ""
        if makefile_parser:
            linker_flags = " ".join(makefile_parser.get_linker_flags())
        else:
            linker_flags = "-lm"  # Default to just math library if no Makefile
        
        # Construct the link command
        link_cmd = f"gcc {' '.join(obj_files)} -o {self.config.xinu_core_output} {linker_flags}"
        self._log(f"Command: {link_cmd}")
        
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
                    self._log(f"ERROR: {line}")
                    link_errors += 1
                elif "warning:" in line.lower():
                    warning_key = line.strip()
                    if warning_key not in self.warnings:
                        print(line)
                        log(line)
                        self._log(f"WARNING: {line}")
                        self.warnings[warning_key] = True
            
            if link_errors > 0:
                log(f"Linking failed with {link_errors} errors.")
                self._log(f"Linking failed with {link_errors} errors.")
                return False
                
            # Make the output executable (Unix-like systems)
            if not self.config.is_windows() and os.path.exists(self.config.xinu_core_output):
                os.chmod(self.config.xinu_core_output, 0o755)
                self._log(f"Made {self.config.xinu_core_output} executable")
                
            return process.returncode == 0
        except Exception as e:
            log(f"Error during linking: {str(e)}")
            self._log(f"Error during linking: {str(e)}")
            return False