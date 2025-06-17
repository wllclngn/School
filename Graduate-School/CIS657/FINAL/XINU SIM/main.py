#!/usr/bin/env python3
"""
XINU Builder - A build system for compiling and running XINU OS simulations
"""
import os
import sys
import shutil
import subprocess
import datetime
import getpass
import argparse
import platform
import glob

class XinuBuilder:
    def __init__(self, verbose=False):
        self.verbose = verbose
        self.timestamp = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        self.username = getpass.getuser()
        self.system = platform.system()
        
        # Project directories
        self.project_dir = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
        self.xinu_os_dir = os.path.join(self.project_dir, "XINU OS")
        self.output_dir = os.path.join(self.project_dir, "XINU SIM", "output")
        self.template_dir = os.path.join(self.project_dir, "xinu_sim", "templates")
        self.include_dir = os.path.join(self.project_dir, "xinu_sim", "include")
        self.obj_dir = os.path.join(self.output_dir, "obj")
        
        # Create directories if they don't exist
        for directory in [self.output_dir, self.obj_dir, self.include_dir]:
            os.makedirs(directory, exist_ok=True)
        
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
        
        # Banner display
        banner = [
            "=======================================",
            f"XINU Builder v1.0 - {self.timestamp}",
            f"User: {self.username} on {self.system} {platform.release()}",
            "======================================="
        ]
        for line in banner:
            self.log(line)
    
    def log(self, message):
        """Print log message with timestamp if verbose mode is enabled"""
        if self.verbose:
            current_time = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
            print(f"{current_time}  - {message}")
    
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
        
        # Add starvation test files
        for root, dirs, files in os.walk(os.path.join(self.xinu_os_dir, "shell")):
            for file in files:
                if "starvation" in file.lower() and file.endswith(".c"):
                    source_files.append(os.path.join(root, file))
                    self.log(f"Found starvation test source: {os.path.join(root, file)}")
                    
        # Add shell structure files if needed for starvation tests
        shell_structure_files = [
            os.path.join(self.xinu_os_dir, "shell", "shell.c"),
            os.path.join(self.xinu_os_dir, "shell", "shellcmd.c")
        ]
        
        for file in shell_structure_files:
            if os.path.exists(file):
                source_files.append(file)
                self.log(f"Found shell structure file: {file}")
        
        return source_files
    
    def generate_files(self):
        """Generate XINU simulation files"""
        self.log("Generating XINU simulation files...")
        
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
        
        # Find XINU starvation test files
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
        self.log("Building XINU Core Process...")
        
        # Prepare compilation log
        compilation_log = f"=== XINU Compilation Log ===\n"
        compilation_log += f"Started: {self.username} at {self.timestamp}\n"
        compilation_log += f"Project directory: {self.project_dir}\n"
        compilation_log += f"System directory: {self.xinu_os_dir}/system\n"
        compilation_log += f"Output directory: {self.output_dir}\n"
        
        # Add source files to log
        for src_file in sources:
            compilation_log += f"Added core file: {src_file}\n"
        compilation_log += f"Using minimal set of {len(sources)} source files for simulation\n\n"
        
        compilation_log += "=== Compilation ===\n\n"
        
        # Create object files
        object_files = []
        warning_count = 0
        error_count = 0
        
        # Include paths for compilation
        include_paths = [
            f"-I{self.output_dir}",
            f"-I{self.include_dir}",
            f"-I{os.path.join(self.xinu_os_dir, 'include')}",
            f"-I{os.path.join(self.xinu_os_dir, 'system', 'include')}",
            f"-I{os.path.join(self.xinu_os_dir, 'shell')}"
        ]
        
        # Compile each source file
        for src_file in sources:
            base_name = os.path.basename(src_file)
            obj_file = os.path.join(self.obj_dir, f"{os.path.splitext(base_name)[0]}.o")
            object_files.append(obj_file)
            
            # Build compilation command
            compile_cmd = ["gcc", "-c", src_file] + include_paths + ["-o", obj_file]
            
            # Log the compile command
            compilation_log += f"Compiling {src_file} -> {obj_file}\n"
            compilation_log += f"Command: {' '.join(compile_cmd)}\n"
            
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
            with open(self.output_files["compilation_log"], "w") as f:
                f.write(compilation_log)
            self.log(f"Compilation failed with {error_count} errors. See log for details.")
            return False
        
        # Link object files
        compilation_log += "=== Linking ===\n"
        link_cmd = ["gcc"] + object_files + ["-o", self.output_files["executable"]]
        
        # Log the link command
        compilation_log += f"Linking {len(object_files)} objects to {self.output_files['executable']}\n"
        compilation_log += f"Command: {' '.join(link_cmd)}\n"
        
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
                with open(self.output_files["compilation_log"], "w") as f:
                    f.write(compilation_log)
                return False
            else:
                # Make executable
                os.chmod(self.output_files["executable"], 0o755)
                compilation_log += f"Made {self.output_files['executable']} executable\n"
        except Exception as e:
            compilation_log += f"Exception during linking: {str(e)}\n"
            self.log(f"Exception during linking: {str(e)}")
            with open(self.output_files["compilation_log"], "w") as f:
                f.write(compilation_log)
            return False
        
        # Finalize compilation log
        compilation_log += f"SUCCESS: Build completed with {error_count} errors and {warning_count} unique warning(s).\n"
        compilation_log += f"XINU core executable: {self.output_files['executable']}\n"
        
        with open(self.output_files["compilation_log"], "w") as f:
            f.write(compilation_log)
        
        self.log(f"SUCCESS: Build completed with {error_count} errors and {warning_count} unique warning(s).")
        self.log(f"Compilation log saved to {self.output_files['compilation_log']}")
        
        return True
    
    def run_simulation(self, args=None):
        """Run the XINU simulation"""
        if not os.path.exists(self.output_files["executable"]):
            self.log(f"ERROR: Executable {self.output_files['executable']} not found.")
            return False
        
        if args:
            self.log(f"Running XINU simulation with args: {args}")
            cmd = [self.output_files["executable"]] + args
            self.log(f"Running XINU simulation: {' '.join(cmd)}")
            subprocess.run(cmd)
        else:
            self.log(f"Running XINU simulation: {self.output_files['executable']}")
            subprocess.run([self.output_files["executable"]])
        
        return True
    
    def build_and_run(self, clean=False, run=False, starvation_test=None):
        """Complete build and run process"""
        self.log("Starting XINU Build Process...")
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
            run_args = []
            if starvation_test:
                run_args = [starvation_test]
            
            if not self.run_simulation(run_args):
                self.log("Failed to run simulation.")
                return False
        
        self.log("XINU Build Process Completed Successfully.")
        return success


def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(description='XINU Builder - Build and run XINU OS simulations')
    parser.add_argument('--clean', action='store_true', help='Clean previous build artifacts')
    parser.add_argument('--verbose', action='store_true', help='Enable verbose output')
    parser.add_argument('--run', action='store_true', help='Run the simulation after building')
    parser.add_argument('--starvation', metavar='TEST', help='Run specific starvation test (test1 or test2)')
    
    args = parser.parse_args()
    
    # Create builder instance
    builder = XinuBuilder(verbose=args.verbose)
    
    # Build and optionally run
    success = builder.build_and_run(
        clean=args.clean,
        run=args.run,
        starvation_test=args.starvation
    )
    
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
