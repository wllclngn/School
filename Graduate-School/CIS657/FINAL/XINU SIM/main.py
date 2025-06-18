#!/usr/bin/env python3
# XINU SIM - A build system for compiling and running XINU OS simulations

import os
import sys
import shutil
import subprocess
import datetime
import getpass
import argparse
import platform
import glob

# Force unbuffered output for all print statements
print("##### XINU SIM Build System Start #####", flush=True)
sys.stdout.reconfigure(line_buffering=True)

# Import and setup the logger before anything else
try:
    sys.path.insert(0, os.path.abspath(os.path.dirname(__file__)))
    from utils.logger import setup_logger, log, finalize_log, LOG_FILE
    logger_imported = True
except ImportError as e:
    print(f"Warning: Could not import logger module: {str(e)}", flush=True)
    logger_imported = False
    
    # Simple log function as fallback
    def log(message, **kwargs):
        print(message, flush=True)
    
    def setup_logger(**kwargs):
        pass
    
    def finalize_log():
        pass
    
    LOG_FILE = None

class XinuBuilder:
    def __init__(self, verbose=False):
        self.verbose = verbose
        self.timestamp = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        self.username = getpass.getuser()
        self.system = platform.system()
        
        # Print directly to terminal
        print(f"##### XINU Builder v1.0 #####", flush=True)
        print(f"User: {self.username} on {self.system}", flush=True)
        print(f"Time: {self.timestamp}", flush=True)
        print("-" * 40, flush=True)
        
        # Project directories
        self.project_dir = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
        self.xinu_os_dir = os.path.join(self.project_dir, "XINU OS")
        self.output_dir = os.path.join(self.project_dir, "XINU SIM", "output")
        self.template_dir = os.path.join(self.project_dir, "XINU SIM", "templates")
        self.include_dir = os.path.join(self.project_dir, "XINU SIM", "include")
        self.obj_dir = os.path.join(self.output_dir, "obj")
        
        # Create directories if they don't exist
        for directory in [self.output_dir, self.obj_dir, self.include_dir]:
            os.makedirs(directory, exist_ok=True)
        
        # Initialize logger with proper output directory
        if logger_imported:
            setup_logger(verbose=verbose)
        
        # Template files
        self.templates = {
            "XINU_STDDEFS_H": os.path.join(self.template_dir, "xinu_stddefs_h.tmpl"),
            "XINU_H": os.path.join(self.template_dir, "xinu_h.tmpl"),
            "XINU_INCLUDES_H": os.path.join(self.template_dir, "xinu_includes_h.tmpl"),
            "XINU_SIM_DECLARATIONS_H": os.path.join(self.template_dir, "xinu_sim_declarations_h.tmpl"),
            "XINU_SIMULATION_C": os.path.join(self.template_dir, "xinu_simulation_c.tmpl"),
            "XINU_CORE_C": os.path.join(self.template_dir, "xinu_core_c.tmpl"),
        }
        
        # Output files
        self.output_files = {
            "stddefs": os.path.join(self.output_dir, "xinu_stddefs.h"),
            "xinu_h": os.path.join(self.include_dir, "xinu.h"),
            "includes": os.path.join(self.output_dir, "xinu_includes.h"),
            "declarations": os.path.join(self.output_dir, "xinu_sim_declarations.h"),
            "simulation": os.path.join(self.output_dir, "xinu_simulation.c"),
            "core": os.path.join(self.output_dir, "xinu_core.c"),
            "pre": os.path.join(self.output_dir, "xinu_pre.h"),
            "executable": os.path.join(self.output_dir, "xinu_core"),
            "compilation_log": os.path.join(self.output_dir, "compilation.txt"),
        }
    
    def log(self, message):
        """Print log message with timestamp if verbose mode is enabled"""
        if logger_imported:
            # Use the imported logger if available
            log(message, verbose_only=not self.verbose)
        elif self.verbose:
            current_time = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
            print(f"{current_time}  - {message}", flush=True)
        else:
            print(message, flush=True)
    
    def clean(self):
        """Clean previous build artifacts"""
        self.log("Cleaning previous build...")
        
        # Remove output files
        for file_key, file_path in self.output_files.items():
            if os.path.exists(file_path):
                os.remove(file_path)
                self.log(f"Removed {file_path}")
        
        # Remove object files
        obj_files = glob.glob(os.path.join(self.obj_dir, "*.o"))
        for obj_file in obj_files:
            os.remove(obj_file)
            self.log(f"Removed object file: {obj_file}")
        
        # Remove any stray logs
        for log_file in ["compilation_summary.txt", "compilation.txt"]:
            log_path = os.path.join(self.project_dir, log_file)
            if os.path.exists(log_path):
                os.remove(log_path)
                self.log(f"Removed {log_path}")
    
    def load_templates(self):
        """Load all template files"""
        templates = {}
        for name, path in self.templates.items():
            try:
                with open(path, "r") as f:
                    templates[name] = f.read()
                    self.log(f"Loaded template: {name}")
            except FileNotFoundError:
                self.log(f"Template not found: {path}")
                sys.exit(1)
        
        self.log(f"Loaded {len(templates)} template files")
        return templates
    
    def find_xinu_source_files(self):
        """Find XINU OS source files needed for compilation"""
        source_files = []
        
        # Add shell files
        for root, dirs, files in os.walk(os.path.join(self.xinu_os_dir, "shell")):
            for file in files:
                if file.endswith(".c"):
                    source_files.append(os.path.join(root, file))
                    self.log(f"Found shell source: {os.path.join(root, file)}")
                    
        # Add system files
        for root, dirs, files in os.walk(os.path.join(self.xinu_os_dir, "system")):
            for file in files:
                if file.endswith(".c"):
                    source_files.append(os.path.join(root, file))
                    self.log(f"Found system source: {os.path.join(root, file)}")
        
        return source_files
    
    def generate_files(self):
        """Generate XINU simulation files"""
        self.log("##### Generating XINU simulation files #####")
        
        # Load templates
        templates = self.load_templates()
        
        # Add our simulation core source files first
        sources = []
        
        # Generate xinu_stddefs.h
        with open(self.output_files["stddefs"], "w") as f:
            content = templates["XINU_STDDEFS_H"]
            content = content.replace("{{DATE}}", self.timestamp)
            content = content.replace("{{USER}}", self.username)
            f.write(content)
        self.log(f"Generated XINU stddefs: {self.output_files['stddefs']}")
        
        # Generate xinu.h
        with open(self.output_files["xinu_h"], "w") as f:
            content = templates["XINU_H"]
            content = content.replace("{{DATE}}", self.timestamp)
            content = content.replace("{{USER}}", self.username)
            f.write(content)
        self.log(f"Created/updated {self.output_files['xinu_h']}")
        
        # Generate xinu_includes.h
        with open(self.output_files["includes"], "w") as f:
            content = templates["XINU_INCLUDES_H"]
            content = content.replace("{{DATE}}", self.timestamp)
            content = content.replace("{{USER}}", self.username)
            f.write(content)
        self.log(f"Generated UNIX-like simulation includes wrapper at: {self.output_files['includes']}")
        
        # Generate xinu_sim_declarations.h
        with open(self.output_files["declarations"], "w") as f:
            content = templates["XINU_SIM_DECLARATIONS_H"]
            content = content.replace("{{DATE}}", self.timestamp)
            content = content.replace("{{USER}}", self.username)
            f.write(content)
        self.log(f"Generated UNIX-like simulation declarations at: {self.output_files['declarations']}")
        
        # Generate xinu_simulation.c
        with open(self.output_files["simulation"], "w") as f:
            content = templates["XINU_SIMULATION_C"]
            content = content.replace("{{DATE}}", self.timestamp)
            content = content.replace("{{USER}}", self.username)
            f.write(content)
        self.log(f"Generated UNIX-like simulation helper at: {self.output_files['simulation']}")
        sources.append(self.output_files["simulation"])
        
        # Find XINU source files
        xinu_sources = self.find_xinu_source_files()
        
        # Generate xinu_core.c
        with open(self.output_files["core"], "w") as f:
            content = templates["XINU_CORE_C"]
            content = content.replace("{{DATE}}", self.timestamp)
            content = content.replace("{{USER}}", self.username)
            f.write(content)
        self.log(f"Generated minimal XINU core at: {self.output_files['core']}")
        sources.append(self.output_files["core"])
        
        # Add all sources for compilation
        sources.extend(xinu_sources)
        
        self.log(f"Using total of {len(sources)} source files for simulation")
        return sources
    
    def compile(self, sources):
        """Compile XINU simulation"""
        self.log("##### Building XINU Core Process #####")
        
        # Prepare compilation log
        compilation_log = f"##### XINU Compilation Log #####\n"
        compilation_log += f"Started: {self.username} at {self.timestamp}\n"
        compilation_log += f"Project directory: {self.project_dir}\n"
        compilation_log += f"System directory: {self.xinu_os_dir}/system\n"
        compilation_log += f"Output directory: {self.output_dir}\n"
        
        # Check for Makefile first
        makefile_path = os.path.join(self.xinu_os_dir, "compile", "Makefile")
        use_makefile = os.path.exists(makefile_path)
        
        if use_makefile:
            self.log(f"Found Makefile at {makefile_path}, parsing for build instructions")
            compilation_log += f"Using build instructions from Makefile: {makefile_path}\n"
            
            # Import the Makefile parser
            sys.path.append(os.path.join(self.project_dir, "XINU SIM"))
            try:
                from makefile_parser import MakefileParser
                
                # Setup config object
                class Config:
                    def __init__(self):
                        self.compile_dir = os.path.join(self.xinu_os_dir, "compile")
                        self.xinu_os_dir = self.xinu_os_dir
                        self.include_dir = os.path.join(self.xinu_os_dir, "include")
                        self.output_dir = self.output_dir
                        self.xinu_h = os.path.join(self.include_dir, "xinu.h")
                
                config = Config()
                config.compile_dir = os.path.dirname(makefile_path)
                config.xinu_os_dir = self.xinu_os_dir
                config.include_dir = self.include_dir
                config.output_dir = self.output_dir
                config.xinu_h = os.path.join(self.include_dir, "xinu.h")
                
                parser = MakefileParser(config)
                if parser.parse_makefile():
                    # Get source files from Makefile
                    makefile_sources = parser.get_resolved_source_files()
                    if makefile_sources:
                        self.log(f"Found {len(makefile_sources)} source files in Makefile")
                        # Add our generated files
                        makefile_sources.append(self.output_files["simulation"])
                        makefile_sources.append(self.output_files["core"])
                        sources = makefile_sources
                        
                        # Get include directories
                        include_dirs = parser.get_resolved_include_dirs()
                        
                        # Get compiler flags
                        compiler_flags = parser.get_compiler_flags()
                        
                        # Get linker flags
                        linker_flags = parser.get_linker_flags()
                        
                        compilation_log += f"Using {len(sources)} source files from Makefile\n"
                        compilation_log += f"Using {len(include_dirs)} include directories from Makefile\n"
                        compilation_log += f"Using compiler flags: {' '.join(compiler_flags)}\n"
                        compilation_log += f"Using linker flags: {' '.join(linker_flags)}\n\n"
            except ImportError:
                self.log("Makefile parser module not found, falling back to standard compilation")
                use_makefile = False
        
        # Add source files to log
        for src_file in sources:
            compilation_log += f"Added core file: {src_file}\n"
        compilation_log += f"Using total of {len(sources)} source files for simulation\n\n"
        
        compilation_log += "##### Compilation #####\n\n"
        
        # Create object files
        object_files = []
        warning_count = 0
        error_count = 0
        
        # Include paths for compilation
        if use_makefile and 'include_dirs' in locals():
            include_paths = [f"-I{d}" for d in include_dirs]
        else:
            include_paths = [
                f"-I{self.output_dir}",
                f"-I{self.include_dir}",
                f"-I{os.path.join(self.xinu_os_dir, 'include')}",
                f"-I{os.path.join(self.xinu_os_dir, 'system', 'include')}",
                f"-I{os.path.join(self.xinu_os_dir, 'shell')}"
            ]
        
        # Compiler flags
        if use_makefile and 'compiler_flags' in locals():
            compile_flags = compiler_flags
        else:
            compile_flags = ["-Wall", "-g", "-O0", "-fno-builtin"]
        
        # Compile each source file
        for i, src_file in enumerate(sources):
            base_name = os.path.basename(src_file)
            obj_file = os.path.join(self.obj_dir, f"{os.path.splitext(base_name)[0]}.o")
            object_files.append(obj_file)
            
            # Build compilation command
            compile_cmd = ["gcc", "-c", src_file] + include_paths + compile_flags + ["-o", obj_file]
            
            # Log the compile command
            compilation_log += f"Compiling {src_file} -> {obj_file}\n"
            compilation_log += f"Command: {' '.join(compile_cmd)}\n"
            
            # Print progress to terminal
            print(f"Compiling {i+1}/{len(sources)}: {os.path.basename(src_file)}", flush=True)
            
            # Execute compilation
            try:
                result = subprocess.run(compile_cmd, 
                                      stdout=subprocess.PIPE, 
                                      stderr=subprocess.PIPE,
                                      text=True)
                
                if result.returncode != 0:
                    compilation_log += f"Error compiling {src_file}:\n{result.stderr}\n"
                    self.log(f"Error compiling {src_file}")
                    self.log(result.stderr)
                    error_count += 1
                else:
                    compilation_log += f"Successfully compiled {src_file}\n\n"
                    self.log(f"Successfully compiled {src_file}")
                    
                    # Count warnings
                    for line in result.stderr.split('\n'):
                        if 'warning:' in line:
                            warning_count += 1
                            compilation_log += f"WARNING: {line}\n"
            
            except Exception as e:
                compilation_log += f"Exception during compilation: {str(e)}\n"
                self.log(f"Exception during compilation: {str(e)}")
                error_count += 1
        
        # If compilation failed, write log and exit
        if error_count > 0:
            # Save to both our local compilation log and the global logger's file
            with open(self.output_files["compilation_log"], "w") as f:
                f.write(compilation_log)
                
            self.log(f"Compilation failed with {error_count} errors. See log for details.")
            return False
        
        # Link object files
        compilation_log += "##### Linking #####\n"
        self.log("##### Linking object files #####")
        
        # Use linker flags from Makefile if available
        if use_makefile and 'linker_flags' in locals():
            link_flags = linker_flags
            link_cmd = ["gcc"] + object_files + ["-o", self.output_files["executable"]] + link_flags
        else:
            link_cmd = ["gcc"] + object_files + ["-o", self.output_files["executable"], "-lm"]
        
        # Log the link command
        compilation_log += f"Linking {len(object_files)} objects to {self.output_files['executable']}\n"
        compilation_log += f"Command: {' '.join(link_cmd)}\n"
        self.log(f"Linking {len(object_files)} objects to {self.output_files['executable']}")
        
        # Execute linking
        try:
            result = subprocess.run(link_cmd, 
                              stdout=subprocess.PIPE, 
                              stderr=subprocess.PIPE,
                              text=True)
            
            if result.returncode != 0:
                compilation_log += f"Error linking:\n{result.stderr}\n"
                self.log(f"Error linking")
                self.log(result.stderr)
                
                # Save to both logs on error
                with open(self.output_files["compilation_log"], "w") as f:
                    f.write(compilation_log)
                    
                return False
            else:
                # Make executable
                os.chmod(self.output_files["executable"], 0o755)
                compilation_log += f"Made {self.output_files['executable']} executable\n"
                self.log(f"Made {self.output_files['executable']} executable")
        except Exception as e:
            compilation_log += f"Exception during linking: {str(e)}\n"
            self.log(f"Exception during linking: {str(e)}")
            
            # Save to both logs on error
            with open(self.output_files["compilation_log"], "w") as f:
                f.write(compilation_log)
                
            return False
        
        # Finalize compilation log
        compilation_log += f"SUCCESS: Build completed with {error_count} errors and {warning_count} unique warning(s).\n"
        compilation_log += f"XINU core executable: {self.output_files['executable']}\n"
        
        # Always write to our compilation log
        with open(self.output_files["compilation_log"], "w") as f:
            f.write(compilation_log)
        
        # Log success in global logger as well
        self.log(f"##### SUCCESS: Build completed with {error_count} errors and {warning_count} unique warning(s). #####")
        self.log(f"Compilation log saved to {self.output_files['compilation_log']}")
        
        if logger_imported:
            # Finalize the logger's log files
            try:
                finalize_log()
            except:
                pass
        
        return True
    
    def run_simulation(self):
        """Run the XINU simulation with interactive I/O"""
        if not os.path.exists(self.output_files["executable"]):
            self.log(f"ERROR: Executable {self.output_files['executable']} not found.")
            return False
        
        self.log(f"##### Running XINU simulation: {self.output_files['executable']} #####")
        
        try:
            # Use Popen but connect directly to the terminal I/O
            process = subprocess.Popen(
                self.output_files["executable"],
                shell=False,
                # Keep standard I/O connected to the terminal
                stdin=None,
                stdout=None,
                stderr=None
            )
            # Wait for the process to complete
            return process.wait() == 0
        except Exception as e:
            self.log(f"Error running simulation: {str(e)}")
            return False
    
    def build_and_run(self, clean=False, run=False):
        """Complete build and run process"""
        self.log("##### Starting XINU Build Process #####")
        self.log(f"Project directory: {self.project_dir}")
        self.log(f"Output directory: {self.output_dir}")
        
        if clean:
            self.clean()
        
        # Generate files and get source list
        sources = self.generate_files()
        
        # Compile
        success = self.compile(sources)
        
        # Run if requested and compilation succeeded
        if success and run:
            # Run with no arguments - let user interact with XINU shell
            if not self.run_simulation():
                self.log("Failed to run simulation.")
                return False
        
        self.log("##### XINU Build Process Completed Successfully #####")
        return success


def main():
    """Main entry point"""
    print("Starting XINU Builder...", flush=True)
    parser = argparse.ArgumentParser(description='XINU Builder - Build and run XINU OS simulations')
    parser.add_argument('--clean', action='store_true', help='Clean previous build artifacts')
    parser.add_argument('--verbose', action='store_true', help='Enable verbose output')
    parser.add_argument('--run', action='store_true', help='Run the simulation after building')
    
    args = parser.parse_args()
    
    print(f"Command line arguments: clean={args.clean}, verbose={args.verbose}, run={args.run}", flush=True)
    
    # Initialize logger with command line arguments
    if logger_imported:
        setup_logger(verbose=args.verbose)
    
    # Create builder instance
    builder = XinuBuilder(verbose=args.verbose)
    
    # Build and optionally run
    success = builder.build_and_run(
        clean=args.clean,
        run=args.run
    )
    
    # Finalize logs before exiting
    if logger_imported:
        finalize_log()
    
    # Exit with appropriate code
    print("XINU Builder exiting with status: " + ("SUCCESS" if success else "FAILURE"), flush=True)
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    print("Python script starting", flush=True)
    main()