# compiler.py - XINU compilation system with g++ integration
# NOTE: ALWAYS USE SYSTEM INFORMATION FOR USER AND TIMESTAMP

import os
import sys
import subprocess
import shlex
import glob
import platform
import datetime
import re
import tempfile
from utils.logger import log

class XinuCompiler:
    # Enhanced XINU compiler with g++ integration and dynamic adaptation.
    
    def __init__(self, config):
        self.config = config
        self.detected_compile_errors = []
        self.structure_adaptations = {}
        self.include_paths = []
        self.system_info = self._get_system_info()
        self._setup_environment()
        
    def _print(self, message):
        # Print directly to terminal for real-time feedback
        sys.stdout.write(message + "\n")
        sys.stdout.flush()
        
    def _get_system_info(self):
        # Get detailed system information for compilation parameters.
        system = platform.system().lower()
        info = {
            'system': system,
            'compiler': 'g++',
            'c_compiler': 'gcc',
            'timestamp': datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
            'user': os.environ.get("USER", os.environ.get("USERNAME", "unknown"))
        }
        
        # Check for compiler availability and detect MinGW specifically
        try:
            process = subprocess.Popen(['g++', '--version'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            stdout, stderr = process.communicate()
            if process.returncode == 0:
                version = stdout.decode('utf-8', errors='ignore').split('\n')[0]
                info['compiler_version'] = version
                log(f"Found g++ compiler: {version}")
                self._print(f"Found g++ compiler: {version}")
                
                # Check if this is MinGW (will contain "MinGW" in version string)
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
        
        # Set OS-specific configuration - use GCC style flags for MinGW
        if system == 'darwin':
            info['os'] = 'macos'
            info['c_flags'] = '-Wall -Wextra'
            info['cpp_flags'] = '-Wall -Wextra -std=c++11'
            info['obj_ext'] = '.o'
            info['exe_ext'] = ''
        elif system == 'windows' and not is_mingw:
            # Regular Windows MSVC compiler
            info['os'] = 'windows'
            info['c_flags'] = '/W4'
            info['cpp_flags'] = '/W4 /EHsc'
            info['obj_ext'] = '.obj'
            info['exe_ext'] = '.exe'
        else:  # Linux and others, including MinGW on Windows
            # MinGW on Windows or GCC on Linux
            info['os'] = 'linux' if system != 'windows' else 'mingw'
            info['c_flags'] = '-Wall -Wextra'
            info['cpp_flags'] = '-Wall -Wextra -std=c++11'
            info['obj_ext'] = '.o'
            info['exe_ext'] = '.exe' if system == 'windows' else ''
            
            # For Linux, add -fPIC flag
            if system == 'linux':
                info['c_flags'] += ' -fPIC'
                info['cpp_flags'] += ' -fPIC'
        
        return info
    
    def _setup_environment(self):
        # Setup the compilation environment.
        # Initialize include paths
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
            
        # Add templates directory for fallback includes
        templates_dir = os.path.join(self.config.project_dir, "templates")
        if os.path.exists(templates_dir):
            self.include_paths.append(templates_dir)
            
        # Create output directories if they don't exist
        os.makedirs(self.config.output_dir, exist_ok=True)
        os.makedirs(self.config.obj_dir, exist_ok=True)
        os.makedirs(self.config.bin_dir, exist_ok=True)
        
        log(f"Setup environment with include paths: {self.include_paths}")
        self._print(f"Setup environment with include paths: {self.include_paths}")
    
    def compile(self):
        # Compile the XINU simulation project with g++ integration.
        self._print("\n##### Compiling XINU Simulation #####")
        
        # Analyze the XINU headers to detect structure names and adaptations needed
        self._analyze_for_adaptations()
        
        # Compile core files
        core_files = [self.config.xinu_core_c, self.config.sim_helper_c]
        core_objects = self._compile_sources(core_files)
        
        # Find and compile additional C files
        additional_sources = self._find_additional_sources()
        additional_objects = self._compile_sources(additional_sources)
        
        # Link everything together
        result = self._link_executable(core_objects + additional_objects)
        
        if result:
            self._print("\nXINU Simulation compilation successful!")
            return True
        else:
            self._print("\nXINU Simulation compilation failed!")
            return False
    
    def _analyze_for_adaptations(self):
        # Use g++ preprocessing to analyze headers for structure adaptations.
        self._print("\n##### Analyzing Headers for Structure Adaptations #####")
        
        # Create a temporary file for preprocessing
        with tempfile.NamedTemporaryFile(suffix=".c", delete=False) as temp_file:
            temp_path = temp_file.name
            temp_file.write(f"#include \"{self.config.includes_h}\"\n".encode('utf-8'))
        
        # Build include paths for preprocessing - properly quote paths to handle spaces
        include_opts = " ".join(f'-I"{p}"' for p in self.include_paths if os.path.exists(p))
        
        # Run g++ preprocessor with properly quoted paths
        try:
            cmd = f'g++ -E {include_opts} "{temp_path}"'
            self._print(f"Running preprocessor: {cmd}")
            process = subprocess.Popen(
                cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                universal_newlines=True
            )
            stdout, stderr = process.communicate()
            
            if process.returncode != 0:
                log(f"Warning: Failed to preprocess headers for adaptation: {stderr}")
                self._print("Warning: Failed to preprocess headers for adaptation")
            else:
                # Scan for structure definitions
                self._scan_for_struct_variations(stdout)
            
            # Clean up temp file
            os.unlink(temp_path)
        except Exception as e:
            log(f"Warning: Error during header analysis: {str(e)}")
            self._print(f"Warning: Error during header analysis")
    
    def _scan_for_struct_variations(self, preprocessed_content):
        # Scan preprocessed content for structure variations.
        # Find process structure
        proc_structs = re.findall(r'struct\s+(\w+)\s*{(.*?)};', preprocessed_content, re.DOTALL)
        
        # Look for key structures like procent
        for name, body in proc_structs:
            if "prstate" in body or "prio" in body or "prprio" in body:
                self._print(f"Found process structure: {name}")
                
                # Detect key field variations
                if "prio" in body and "prprio" not in body:
                    self.structure_adaptations["prprio"] = "prio"
                    log("Detected field variation: prprio -> prio")
                    self._print("Detected field variation: prprio -> prio")
                elif "prprio" in body and "prio" not in body:
                    self.structure_adaptations["prio"] = "prprio"
                    log("Detected field variation: prio -> prprio")
                    self._print("Detected field variation: prio -> prprio")
        
        log(f"Structure adaptations: {self.structure_adaptations}")
    
    def _find_additional_sources(self):
        # Find additional source files to compile.
        sources = []
        
        # Check for any C files in the XINU SIM directory
        for root, dirs, files in os.walk(self.config.project_dir):
            for file in files:
                if file.endswith(".c") and not file.startswith("xinu_"):
                    source_path = os.path.join(root, file)
                    sources.append(source_path)
        
        # Look in specific directories if defined
        if hasattr(self.config, 'source_dirs'):
            for source_dir in self.config.source_dirs:
                if os.path.exists(source_dir):
                    for file in os.listdir(source_dir):
                        if file.endswith(".c"):
                            source_path = os.path.join(source_dir, file)
                            if source_path not in sources:
                                sources.append(source_path)
        
        if sources:
            self._print(f"Found {len(sources)} additional source files to compile")
            log(f"Additional sources: {sources}")
        
        return sources
    
    def _compile_sources(self, sources):
        # Compile source files to object files.
        objects = []
        
        for source in sources:
            obj_file = self._get_object_path(source)
            if self._compile_file(source, obj_file):
                objects.append(obj_file)
        
        return objects
    
    def _get_object_path(self, source_path):
        # Get the path for an object file based on source file.
        base_name = os.path.basename(source_path)
        name_without_ext = os.path.splitext(base_name)[0]
        obj_ext = self.system_info['obj_ext']
        return os.path.join(self.config.obj_dir, f"{name_without_ext}{obj_ext}")
    
    def _compile_file(self, source_path, obj_path):
        # Compile a single source file with adaptations.
        self._print(f"Compiling {os.path.basename(source_path)}...")
        
        # Check if the source file exists
        if not os.path.exists(source_path):
            log(f"Error: Source file not found: {source_path}")
            self._print(f"Error: Source file not found: {source_path}")
            return False
        
        # Apply structure adaptations if needed
        adapted_source = None
        if self.structure_adaptations:
            adapted_source = self._adapt_source(source_path)
        
        # Build include paths with proper quoting to handle spaces in paths
        include_opts = " ".join(f'-I"{p}"' for p in self.include_paths if os.path.exists(p))
        
        # Build command based on source type
        compiler = self.system_info['c_compiler'] if source_path.endswith(".c") else self.system_info['compiler']
        flags = self.system_info['c_flags'] if source_path.endswith(".c") else self.system_info['cpp_flags']
        
        cmd = f'{compiler} {flags} {include_opts} -c'
        
        # If we have an adapted source, compile that instead of the original
        if adapted_source:
            cmd += f' "{adapted_source}" -o "{obj_path}"'
            source_to_compile = adapted_source
        else:
            cmd += f' "{source_path}" -o "{obj_path}"'
            source_to_compile = source_path
        
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
                
                # Try to parse errors for easier diagnosis
                self._parse_compilation_errors(stderr)
                
                # If using an adapted source, clean it up
                if adapted_source:
                    try:
                        os.unlink(adapted_source)
                    except:
                        pass
                
                return False
            
            self._print(f"Successfully compiled {os.path.basename(source_path)}")
            
            # Clean up adapted source if used
            if adapted_source:
                try:
                    os.unlink(adapted_source)
                except:
                    pass
            
            return True
        except Exception as e:
            log(f"Error during compilation of {source_path}: {str(e)}")
            self._print(f"Error during compilation of {os.path.basename(source_path)}: {str(e)}")
            return False
    
    def _adapt_source(self, source_path):
        # Create an adapted version of the source with structure adaptations.
        try:
            # Create a temporary file for the adapted source
            fd, adapted_path = tempfile.mkstemp(suffix=".c", prefix="xinu_adapted_")
            os.close(fd)
            
            with open(source_path, 'r', encoding='utf-8', errors='ignore') as src:
                content = src.read()
            
            # Apply structure adaptations
            for from_field, to_field in self.structure_adaptations.items():
                # Replace field references in structure access (e.g., proc->prio to proc->prprio)
                content = re.sub(r'(\w+)->(' + from_field + r')\b', r'\1->' + to_field, content)
                
                # Replace array references (e.g., proctab[i].prio to proctab[i].prprio)
                content = re.sub(r'(\w+\[\w+\])\.(' + from_field + r')\b', r'\1.' + to_field, content)
            
            with open(adapted_path, 'w', encoding='utf-8') as dest:
                dest.write(content)
            
            log(f"Created adapted source: {adapted_path}")
            return adapted_path
        except Exception as e:
            log(f"Error adapting source {source_path}: {str(e)}")
            self._print(f"Error adapting source {os.path.basename(source_path)}")
            return None
    
    def _parse_compilation_errors(self, error_output):
        # Parse compilation errors for diagnosis.
        # Look for common error patterns
        missing_types = re.findall(r"'(\w+)' was not declared in this scope", error_output)
        missing_fields = re.findall(r"no member named '(\w+)' in", error_output)
        
        # Record the errors
        for type_name in missing_types:
            self.detected_compile_errors.append(f"Missing type: {type_name}")
            self._print(f"Detected missing type: {type_name}")
        
        for field_name in missing_fields:
            self.detected_compile_errors.append(f"Missing structure field: {field_name}")
            self._print(f"Detected missing structure field: {field_name}")
    
    def _link_executable(self, object_files):
        # Link object files into executable.
        if not object_files:
            log("Error: No object files to link")
            self._print("Error: No object files to link")
            return False
        
        # Create the executable path
        exe_name = "xinu_sim" + self.system_info['exe_ext']
        exe_path = os.path.join(self.config.bin_dir, exe_name)
        
        # Build object files list with proper quoting to handle spaces in paths
        obj_list = " ".join(f'"{obj}"' for obj in object_files)
        
        # Link command with properly quoted paths
        cmd = f'{self.system_info["compiler"]} {obj_list} -o "{exe_path}"'
        
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
        # Get list of compilation errors detected.
        return self.detected_compile_errors