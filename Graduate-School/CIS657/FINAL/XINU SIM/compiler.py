# compiler.py - Direct compilation of XINU OS source files
import os
import sys
import subprocess
import platform
import datetime
import re
import tempfile
from utils.logger import log
from makefile_parser import MakefileParser
from generator import XinuGenerator

class XinuCompiler:
    # XINU compiler that directly uses source files from the Makefile
    
    def __init__(self, config):
        self.config = config
        self.detected_compile_errors = []
        self.structure_adaptations = {}
        self.include_paths = []
        self.system_info = self._get_system_info()
        self.makefile_parser = MakefileParser(config)
        self.xinu_source_files = []
        self._setup_environment()
        
    def _print(self, message):
        # Print directly to terminal for real-time feedback
        sys.stdout.write(message + "\n")
        sys.stdout.flush()
        
    def _get_system_info(self):
        # Get detailed system information for compilation parameters
        system = platform.system().lower()
        info = {
            'system': system,
            'compiler': 'g++',
            'c_compiler': 'gcc',
            'timestamp': datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
            'user': os.environ.get("USER", os.environ.get("USERNAME", "unknown"))
        }
        
        # Check for compiler availability
        try:
            process = subprocess.Popen(['g++', '--version'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            stdout, stderr = process.communicate()
            if process.returncode == 0:
                version = stdout.decode('utf-8', errors='ignore').split('\n')[0]
                info['compiler_version'] = version
                log(f"Found g++ compiler: {version}")
                self._print(f"Found g++ compiler: {version}")
                
                # Check if this is MinGW
                is_mingw = "mingw" in version.lower()
            else:
                log("Warning: g++ compiler not found")
                self._print("Warning: g++ compiler not found")
                info['compiler_version'] = 'unknown'
                is_mingw = False
        except Exception:
            log("Warning: Could not determine g++ compiler version")
            info['compiler_version'] = 'unknown'
            is_mingw = False
        
        # Set OS-specific configuration
        if system == 'darwin':
            info['os'] = 'macos'
            info['c_flags'] = '-Wall -Wextra'
            info['cpp_flags'] = '-Wall -Wextra -std=c++11'
            info['obj_ext'] = '.o'
            info['exe_ext'] = ''
        elif system == 'windows' and not is_mingw:
            info['os'] = 'windows'
            info['c_flags'] = '/W4'
            info['cpp_flags'] = '/W4 /EHsc'
            info['obj_ext'] = '.obj'
            info['exe_ext'] = '.exe'
        else:  # Linux and others, including MinGW on Windows
            info['os'] = 'linux' if system != 'windows' else 'mingw'
            info['c_flags'] = '-Wall -Wextra'
            info['cpp_flags'] = '-Wall -Wextra -std=c++11'
            info['obj_ext'] = '.o'
            info['exe_ext'] = '.exe' if system == 'windows' else ''
            
        return info
    
    def _setup_environment(self):
        # Setup the compilation environment
        self.include_paths = []
        
        # Add standard include paths
        if hasattr(self.config, 'include_dir') and os.path.exists(self.config.include_dir):
            self.include_paths.append(self.config.include_dir)
        
        # Add XINU OS include directory
        xinu_include_dir = os.path.join(self.config.xinu_os_dir, 'include')
        if os.path.exists(xinu_include_dir):
            self.include_paths.append(xinu_include_dir)
        
        # Add output directory for generated headers
        output_include_dir = os.path.join(self.config.output_dir)
        if os.path.exists(output_include_dir):
            self.include_paths.append(output_include_dir)
            
        # Create output directories
        os.makedirs(self.config.output_dir, exist_ok=True)
        os.makedirs(self.config.obj_dir, exist_ok=True)
        
        log(f"Setup environment with include paths: {self.include_paths}")
        self._print(f"Setup environment with include paths: {self.include_paths}")
    
    def compile(self):
        # Standard compile method (use compile_with_retry for enhanced version)
        log("Compiling XINU Simulation")
        
        # Parse Makefile to get original source files
        self._parse_makefile()
        
        # Compile the simulation helper
        sim_helper_path = os.path.join(self.config.output_dir, "xinu_simulation.c")
        sim_helper_obj = self._get_object_path(sim_helper_path)
        self._compile_file(sim_helper_path, sim_helper_obj)
        
        # Compile XINU OS source files from Makefile
        xinu_objects = []
        for source_file in self.xinu_source_files:
            obj_file = self._get_object_path(source_file)
            if self._compile_file(source_file, obj_file, force_include=self.config.includes_h):
                xinu_objects.append(obj_file)
        
        # Link everything together
        all_objects = [sim_helper_obj] + xinu_objects
        result = self._link_executable(all_objects)
        
        if result:
            self._print("\nXINU Simulation compilation successful!")
            return True
        else:
            self._print("\nXINU Simulation compilation failed!")
            return False
            
    def compile_with_retry(self):
        # Enhanced compile method with retry support
        self._print("\n##### Compiling XINU Simulation #####")
        
        # Parse Makefile to get original source files
        self._parse_makefile()
        
        # First attempt at compilation
        success, compile_output = self._compile_attempt()
        
        # If it failed, check for missing headers
        if not success:
            # Create generator for analyzing errors and creating stubs
            generator = XinuGenerator(self.config)
            
            # Analyze compilation errors
            log("Analyzing compilation errors for missing headers")
            missing_headers, undefined_types = generator.analyze_compile_errors(compile_output)
            
            # If we found missing headers, generate them and retry
            if missing_headers:
                log(f"Detected {len(missing_headers)} missing headers, generating stubs")
                self._print(f"\nDetected {len(missing_headers)} missing headers, generating stubs and retrying...")
                
                # Create stub headers
                generated_stubs = generator.create_missing_header_stubs(missing_headers, undefined_types)
                
                # Track what we generated
                generator.track_generated_stubs(generated_stubs)
                
                # Retry compilation
                self._print("\n##### Retrying Compilation with Generated Stubs #####")
                success, compile_output = self._compile_attempt()
        
        # Report final result
        if success:
            self._print("\nXINU Simulation compilation successful!")
            return True
        else:
            self._print("\nXINU Simulation compilation failed!")
            return False

    def _compile_attempt(self):
        # Attempt to compile all source files
        compile_output = ""
        success = True
        
        # Compile the simulation helper
        sim_helper_path = os.path.join(self.config.output_dir, "xinu_simulation.c")
        sim_helper_obj = self._get_object_path(sim_helper_path)
        result, output = self._capture_compile_output(sim_helper_path, sim_helper_obj)
        compile_output += output
        success = success and result
        
        # Compile XINU OS source files from Makefile
        xinu_objects = []
        for source_file in self.xinu_source_files:
            obj_file = self._get_object_path(source_file)
            result, output = self._capture_compile_output(source_file, obj_file, 
                                                force_include=self.config.includes_h)
            compile_output += output
            if result:
                xinu_objects.append(obj_file)
            else:
                success = False
        
        # Only link if compilation succeeded
        if success and xinu_objects:
            # Link everything together
            all_objects = [sim_helper_obj] + xinu_objects
            link_result, link_output = self._capture_link_output(all_objects)
            compile_output += link_output
            success = success and link_result
        
        return success, compile_output

    def _capture_compile_output(self, source_path, obj_path, force_include=None):
        # Compile a file and capture the output
        self._print(f"Compiling {os.path.basename(source_path)}...")
        
        # Check if the source file exists
        if not os.path.exists(source_path):
            log(f"Error: Source file not found: {source_path}")
            self._print(f"Error: Source file not found: {source_path}")
            return False, f"Error: Source file not found: {source_path}\n"
        
        # Build include paths
        include_opts = " ".join(f'-I"{p}"' for p in self.include_paths if os.path.exists(p))
        
        # Build command based on source type
        compiler = self.system_info['c_compiler']
        flags = self.system_info['c_flags']
        
        # Add force include if specified (for XINU OS sources)
        force_include_opt = ""
        if force_include:
            if self.system_info['os'] == 'windows' and not 'mingw' in self.system_info['os']:
                force_include_opt = f' /FI"{force_include}"'
            else:
                force_include_opt = f' -include "{force_include}"'
        
        cmd = f'{compiler} {flags} {include_opts}{force_include_opt} -c "{source_path}" -o "{obj_path}"'
        
        try:
            self._print(f"Running: {cmd}")
            process = subprocess.Popen(
                cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                universal_newlines=True
            )
            stdout, stderr = process.communicate()
            output = f"OUTPUT {source_path}:\n{stdout}\n{stderr}\n"
            
            if process.returncode != 0:
                log(f"Compilation error in {source_path}:\n{stderr}")
                self._print(f"Error compiling {os.path.basename(source_path)}")
                self._print(f"Error message: {stderr}")
                return False, output
            
            self._print(f"Successfully compiled {os.path.basename(source_path)}")
            return True, output
        except Exception as e:
            error_msg = f"Error during compilation of {source_path}: {str(e)}"
            log(error_msg)
            self._print(f"Error during compilation of {os.path.basename(source_path)}: {str(e)}")
            return False, error_msg

    def _capture_link_output(self, object_files):
        # Link object files and capture the output
        if not object_files:
            log("Error: No object files to link")
            self._print("Error: No object files to link")
            return False, "Error: No object files to link\n"
        
        # Create the executable path
        exe_name = "xinu_core" + self.system_info['exe_ext']
        exe_path = os.path.join(self.config.output_dir, exe_name)
        
        # Store the path in the config for other modules to access
        self.config.xinu_core_output = exe_path
        
        # Build object files list
        obj_list = " ".join(f'"{obj}"' for obj in object_files)
        
        # Get additional linker flags from Makefile if available
        additional_flags = ""
        if hasattr(self.makefile_parser, "get_linker_flags"):
            makefile_flags = self.makefile_parser.get_linker_flags()
            if makefile_flags:
                additional_flags = " ".join(makefile_flags)
        
        # Add math library in Unix systems
        if self.system_info['os'] in ('linux', 'macos'):
            additional_flags += " -lm"
        
        # Link command
        cmd = f'{self.system_info["compiler"]} {obj_list} -o "{exe_path}" {additional_flags}'
        
        self._print(f"Linking executable: {exe_path}")
        self._print(f"Running: {cmd}")
        
        try:
            process = subprocess.Popen(
                cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                universal_newlines=True
            )
            stdout, stderr = process.communicate()
            output = f"LINK OUTPUT:\n{stdout}\n{stderr}\n"
            
            if process.returncode != 0:
                log(f"Linking error:\n{stderr}")
                self._print(f"Error linking executable: {exe_path}")
                self._print(f"Error message: {stderr}")
                return False, output
            
            self._print(f"Successfully created executable: {exe_path}")
            
            # Make the file executable on Unix-like systems
            if self.system_info['os'] in ('linux', 'macos'):
                os.chmod(exe_path, 0o755)
            
            return True, output
        except Exception as e:
            error_msg = f"Error during linking: {str(e)}"
            log(error_msg)
            self._print(f"Error during linking: {str(e)}")
            return False, error_msg
    
    def _parse_makefile(self):
        # Parse XINU OS Makefile to get source files
        self._print("\n##### Parsing XINU OS Makefile #####")
        
        if not self.makefile_parser.parse_makefile():
            self._print("Warning: Failed to parse Makefile, falling back to source scanning")
            self._scan_for_source_files()
            return
        
        # Get resolved source files from Makefile
        self.xinu_source_files = self.makefile_parser.get_resolved_source_files()
        
        # Update include paths with Makefile-defined includes
        makefile_includes = self.makefile_parser.get_resolved_include_dirs()
        for inc_dir in makefile_includes:
            if inc_dir not in self.include_paths:
                self.include_paths.append(inc_dir)
        
        self._print(f"Found {len(self.xinu_source_files)} source files from Makefile")
        log(f"Source files from Makefile: {self.xinu_source_files}")
    
    def _scan_for_source_files(self):
        # Fallback: scan for source files in XINU OS directory structure
        self._print("Scanning XINU OS directory for source files")
        
        self.xinu_source_files = []
        
        # Check standard XINU directories
        dirs_to_scan = [
            os.path.join(self.config.xinu_os_dir, "system"),
            os.path.join(self.config.xinu_os_dir, "device"),
            os.path.join(self.config.xinu_os_dir, "shell"),
            os.path.join(self.config.xinu_os_dir, "lib", "libxc")
        ]
        
        for dir_path in dirs_to_scan:
            if os.path.exists(dir_path):
                for root, _, files in os.walk(dir_path):
                    for file in files:
                        if file.endswith(".c"):
                            self.xinu_source_files.append(os.path.join(root, file))
        
        self._print(f"Found {len(self.xinu_source_files)} source files from directory scan")
    
    def _get_object_path(self, source_path):
        # Get the path for an object file based on source file
        base_name = os.path.basename(source_path)
        name_without_ext = os.path.splitext(base_name)[0]
        obj_ext = self.system_info['obj_ext']
        return os.path.join(self.config.obj_dir, f"{name_without_ext}{obj_ext}")
    
    def _compile_file(self, source_path, obj_path, force_include=None):
        # Compile a source file to object file
        self._print(f"Compiling {os.path.basename(source_path)}...")
        
        # Check if the source file exists
        if not os.path.exists(source_path):
            log(f"Error: Source file not found: {source_path}")
            self._print(f"Error: Source file not found: {source_path}")
            return False
        
        # Build include paths
        include_opts = " ".join(f'-I"{p}"' for p in self.include_paths if os.path.exists(p))
        
        # Build command based on source type
        compiler = self.system_info['c_compiler']
        flags = self.system_info['c_flags']
        
        # Add force include if specified (for XINU OS sources)
        force_include_opt = ""
        if force_include:
            if self.system_info['os'] == 'windows' and not 'mingw' in self.system_info['os']:
                force_include_opt = f' /FI"{force_include}"'
            else:
                force_include_opt = f' -include "{force_include}"'
        
        cmd = f'{compiler} {flags} {include_opts}{force_include_opt} -c "{source_path}" -o "{obj_path}"'
        
        try:
            self._print(f"Running: {cmd}")
            process = subprocess.Popen(
                cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                universal_newlines=True
            )
            stdout, stderr = process.communicate()
            
            if process.returncode != 0:
                log(f"Compilation error in {source_path}:\n{stderr}")
                self._print(f"Error compiling {os.path.basename(source_path)}")
                self._print(f"Error message: {stderr}")
                return False
            
            self._print(f"Successfully compiled {os.path.basename(source_path)}")
            return True
        except Exception as e:
            log(f"Error during compilation of {source_path}: {str(e)}")
            self._print(f"Error during compilation of {os.path.basename(source_path)}: {str(e)}")
            return False
    
    def _link_executable(self, object_files):
        # Link object files into executable
        if not object_files:
            log("Error: No object files to link")
            self._print("Error: No object files to link")
            return False
        
        # Create the executable path
        exe_name = "xinu_core" + self.system_info['exe_ext']
        exe_path = os.path.join(self.config.output_dir, exe_name)
        
        # Store the path in the config for other modules to access
        self.config.xinu_core_output = exe_path
        
        # Build object files list
        obj_list = " ".join(f'"{obj}"' for obj in object_files)
        
        # Get additional linker flags from Makefile if available
        additional_flags = ""
        if hasattr(self.makefile_parser, "get_linker_flags"):
            makefile_flags = self.makefile_parser.get_linker_flags()
            if makefile_flags:
                additional_flags = " ".join(makefile_flags)
        
        # Add math library in Unix systems
        if self.system_info['os'] in ('linux', 'macos'):
            additional_flags += " -lm"
        
        # Link command
        cmd = f'{self.system_info["compiler"]} {obj_list} -o "{exe_path}" {additional_flags}'
        
        self._print(f"Linking executable: {exe_path}")
        self._print(f"Running: {cmd}")
        
        try:
            process = subprocess.Popen(
                cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                universal_newlines=True
            )
            stdout, stderr = process.communicate()
            
            if process.returncode != 0:
                log(f"Linking error:\n{stderr}")
                self._print(f"Error linking executable: {exe_path}")
                self._print(f"Error message: {stderr}")
                return False
            
            self._print(f"Successfully created executable: {exe_path}")
            
            # Make the file executable on Unix-like systems
            if self.system_info['os'] in ('linux', 'macos'):
                os.chmod(exe_path, 0o755)
            
            return True
        except Exception as e:
            log(f"Error during linking: {str(e)}")
            self._print(f"Error during linking: {str(e)}")
            return False
    
    def get_compile_errors(self):
        # Get list of compilation errors detected
        return self.detected_compile_errors
