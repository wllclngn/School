# main.py - XINU SIM main entry point with g++ integration
# NOTE: ALWAYS USE SYSTEM INFORMATION FOR USER AND TIMESTAMP

import os
import sys
import argparse
import datetime
import platform
import subprocess
from pathlib import Path

# Add current directory to path for local imports
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

# Import local modules
from utils.logger import log, setup_logger

# Define setup_logging to maintain compatibility with existing code
def setup_logging(level='INFO'):
    """Wrapper for setup_logger to maintain API compatibility"""
    verbose = level.lower() == 'debug'
    return setup_logger(verbose=verbose)

from generator import XinuGenerator
from compiler import XinuCompiler

class XinuConfig:
    """Configuration class for XINU simulation."""
    
    def __init__(self):
        # Initialize with default values
        self.project_dir = os.path.abspath(os.path.dirname(__file__))
        # Point to the correct XINU OS directory which is one level up
        self.xinu_os_dir = os.path.abspath(os.path.join(self.project_dir, "..", "XINU OS"))
        self.output_dir = os.path.join(self.project_dir, "output")
        self.obj_dir = os.path.join(self.output_dir, "obj")
        self.bin_dir = os.path.join(self.output_dir, "bin")
        self.include_dir = os.path.join(self.xinu_os_dir, "include")
        self.builder_dir = os.path.join(self.project_dir, "xinu_sim")
        
        # Output file paths
        self.stddefs_h = os.path.join(self.output_dir, "xinu_stddefs.h")
        self.xinu_h = os.path.join(self.output_dir, "xinu.h")
        self.includes_h = os.path.join(self.output_dir, "xinu_includes.h")
        self.sim_decls_h = os.path.join(self.output_dir, "xinu_sim_declarations.h")
        self.sim_helper_c = os.path.join(self.output_dir, "xinu_simulation.c")
        self.xinu_core_c = os.path.join(self.output_dir, "xinu_core.c")
        
        # Configuration options
        self.rebuild_all = False
        self.verbose = False
        self.use_gcc_preprocess = True  # Default to using g++ preprocessing
        self.debug = False
        
        # Additional source directories
        self.source_dirs = []
        
    def check_environment(self):
        """Check for required environment components."""
        # Check if g++ is available
        try:
            result = subprocess.run(['g++', '--version'], 
                                   stdout=subprocess.PIPE, 
                                   stderr=subprocess.PIPE, 
                                   text=True)
            if result.returncode == 0:
                log(f"Found g++ compiler: {result.stdout.split('\n')[0]}")
                return True
            else:
                log("Warning: g++ compiler not found")
                self.use_gcc_preprocess = False
        except Exception:
            log("Warning: Could not detect g++ compiler")
            self.use_gcc_preprocess = False
            
        return True  # Continue anyway, fallback mechanisms will handle it
    
    def check_xinu_os(self):
        """Check if XINU OS directory is valid."""
        # Check if the XINU OS directory exists
        if not os.path.exists(self.xinu_os_dir):
            log(f"Warning: XINU OS directory not found at: {self.xinu_os_dir}")
            return False
            
        # Check for include directory
        if not os.path.exists(self.include_dir):
            # Try to find the include directory
            potential_include = os.path.join(self.xinu_os_dir, "include")
            if os.path.exists(potential_include):
                self.include_dir = potential_include
                log(f"Found include directory at: {self.include_dir}")
            else:
                # Search for include directory in the project
                for root, dirs, files in os.walk(self.xinu_os_dir):
                    if "include" in dirs:
                        self.include_dir = os.path.join(root, "include")
                        log(f"Found include directory at: {self.include_dir}")
                        break
                else:
                    log(f"Warning: Could not find include directory in XINU OS")
                    return False
        
        return True
    
    def setup_directories(self):
        """Set up required directories for the build."""
        # Create output directories
        os.makedirs(self.output_dir, exist_ok=True)
        os.makedirs(self.obj_dir, exist_ok=True)
        os.makedirs(self.bin_dir, exist_ok=True)
        
        # Check if we need to clean directories
        if self.rebuild_all:
            # Clean output directory
            for file in os.listdir(self.output_dir):
                file_path = os.path.join(self.output_dir, file)
                if os.path.isfile(file_path):
                    os.remove(file_path)
            
            # Clean obj directory
            for file in os.listdir(self.obj_dir):
                file_path = os.path.join(self.obj_dir, file)
                if os.path.isfile(file_path):
                    os.remove(file_path)
            
            # Clean bin directory
            for file in os.listdir(self.bin_dir):
                file_path = os.path.join(self.bin_dir, file)
                if os.path.isfile(file_path):
                    os.remove(file_path)
            
            log("Cleaned output directories for complete rebuild")
            
    def load_from_args(self, args):
        """Load configuration from command-line arguments."""
        if args.xinu_dir:
            self.xinu_os_dir = os.path.abspath(args.xinu_dir)
            self.include_dir = os.path.join(self.xinu_os_dir, "include")
        
        if args.output_dir:
            self.output_dir = os.path.abspath(args.output_dir)
            self.obj_dir = os.path.join(self.output_dir, "obj")
            self.bin_dir = os.path.join(self.output_dir, "bin")
            
            # Update output file paths
            self.stddefs_h = os.path.join(self.output_dir, "xinu_stddefs.h")
            self.xinu_h = os.path.join(self.output_dir, "xinu.h")
            self.includes_h = os.path.join(self.output_dir, "xinu_includes.h")
            self.sim_decls_h = os.path.join(self.output_dir, "xinu_sim_declarations.h")
            self.sim_helper_c = os.path.join(self.output_dir, "xinu_simulation.c")
            self.xinu_core_c = os.path.join(self.output_dir, "xinu_core.c")
        
        if args.source_dir:
            for src_dir in args.source_dir:
                if os.path.exists(src_dir):
                    self.source_dirs.append(os.path.abspath(src_dir))
        
        self.rebuild_all = args.rebuild_all
        self.verbose = args.verbose
        self.debug = args.debug
        
        # Honor explicit disablement of g++ preprocessing
        if args.no_gcc_preprocess:
            self.use_gcc_preprocess = False
        
        # Update include directory if specified explicitly
        if args.include_dir:
            self.include_dir = os.path.abspath(args.include_dir)
            
        log(f"Configuration loaded from command-line arguments")


class XinuSimMain:
    """Main class for XINU simulation."""
    
    def __init__(self):
        self.config = XinuConfig()
        self.generator = None
        self.compiler = None
        
    def _print(self, message):
        # Print directly to terminal for real-time feedback
        sys.stdout.write(message + "\n")
        sys.stdout.flush()
        
    def setup_args(self):
        """Parse command-line arguments."""
        parser = argparse.ArgumentParser(
            description='XINU OS Simulation System',
            epilog='Dynamically creates a simulation environment for XINU OS.'
        )
        
        parser.add_argument('-x', '--xinu-dir', dest='xinu_dir',
                           help='Path to XINU OS directory')
        parser.add_argument('-o', '--output-dir', dest='output_dir',
                           help='Path to output directory')
        parser.add_argument('-i', '--include-dir', dest='include_dir',
                           help='Path to XINU include directory')
        parser.add_argument('-s', '--source-dir', dest='source_dir', 
                           action='append',
                           help='Path(s) to additional source directories')
        parser.add_argument('-r', '--rebuild-all', dest='rebuild_all',
                           action='store_true',
                           help='Force rebuild of all components')
        parser.add_argument('-c', '--clean', dest='rebuild_all',  # Added clean alias
                           action='store_true',
                           help='Clean and rebuild (alias for --rebuild-all)')
        parser.add_argument('-v', '--verbose', dest='verbose',
                           action='store_true',
                           help='Enable verbose output')
        parser.add_argument('-d', '--debug', dest='debug',
                           action='store_true',
                           help='Enable debug mode')
        parser.add_argument('--no-gcc-preprocess', dest='no_gcc_preprocess',
                           action='store_true',
                           help='Disable g++ preprocessing (use fallback parser)')
        
        args = parser.parse_args()
        return args
    
    def initialize(self):
        """Initialize the XINU simulation system."""
        # Parse command-line arguments
        args = self.setup_args()
        self.config.load_from_args(args)
        
        # Setup logging
        log_level = 'DEBUG' if self.config.debug else 'INFO'
        setup_logging(log_level)
        
        # Display welcome message
        self._print("\n============================================")
        self._print("  XINU Simulation System with g++ Integration")
        self._print("============================================")
        self._print(f"Date/Time: {datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        self._print(f"User: {os.environ.get('USER', os.environ.get('USERNAME', 'unknown'))}")
        self._print(f"System: {platform.system()} {platform.release()}")
        self._print("--------------------------------------------\n")
        
        # Check environment
        if not self.config.check_environment():
            self._print("ERROR: Environment check failed")
            return False
            
        # Check XINU OS
        if not self.config.check_xinu_os():
            self._print("ERROR: XINU OS check failed")
            self._print("Please specify a valid XINU OS directory with --xinu-dir")
            return False
            
        # Setup directories
        self.config.setup_directories()
        
        # Create generator and compiler
        self.generator = XinuGenerator(self.config)
        self.compiler = XinuCompiler(self.config)
        
        return True
    
    def run(self):
        """Run the XINU simulation system."""
        # Generate simulation files
        self._print("\n=== Generating XINU Simulation Files ===")
        self.generator.generate_files()
        
        # Compile simulation
        self._print("\n=== Compiling XINU Simulation ===")
        compile_result = self.compiler.compile()
        
        # Check compilation result
        if not compile_result:
            self._print("\nCompilation failed. Checking for issues...")
            errors = self.compiler.get_compile_errors()
            
            if errors:
                self._print("\nDetected errors:")
                for error in errors:
                    self._print(f" - {error}")
                
                self._print("\nTrying to resolve issues and recompile...")
                if self._resolve_compilation_issues():
                    self._print("\nIssues resolved, recompiling...")
                    compile_result = self.compiler.compile()
            
        # Report final result
        if compile_result:
            self._print("\n=== Build Successful ===")
            exe_path = os.path.join(self.config.bin_dir, "xinu_sim")
            if platform.system() == "Windows":
                exe_path += ".exe"
            
            self._print(f"\nExecutable created: {exe_path}")
            self._print("\nRun the simulation with:")
            self._print(f"  {exe_path}")
            return True
        else:
            self._print("\n=== Build Failed ===")
            self._print("\nPlease check the error messages above and fix any issues.")
            return False
    
    def _resolve_compilation_issues(self):
        """Attempt to resolve compilation issues automatically."""
        # Get detected compilation errors
        errors = self.compiler.get_compile_errors()
        missing_types = [err.split(": ")[1] for err in errors if err.startswith("Missing type")]
        missing_fields = [err.split(": ")[1] for err in errors if err.startswith("Missing structure field")]
        
        # Attempt to fix missing types by regenerating stddefs
        if missing_types:
            self._print("\nAttempting to fix missing types:")
            for type_name in missing_types:
                self._print(f" - Adding missing type: {type_name}")
            
            # Regenerate with additional types
            self.generator.add_missing_types(missing_types)
            self.generator.generate_stddefs()
        
        # Attempt to fix missing fields by adding structure adaptations
        if missing_fields:
            self._print("\nAttempting to fix missing structure fields:")
            for field_name in missing_fields:
                self._print(f" - Adding structure adaptation for: {field_name}")
                
                # Try to determine field variants
                if field_name.startswith("pr") and len(field_name) > 2:
                    # Try adapting prXXX to XXX
                    non_pr_field = field_name[2:]
                    self.compiler.structure_adaptations[field_name] = non_pr_field
                    self._print(f"   - Added adaptation: {field_name} -> {non_pr_field}")
                else:
                    # Try adapting XXX to prXXX
                    pr_field = f"pr{field_name}"
                    self.compiler.structure_adaptations[field_name] = pr_field
                    self._print(f"   - Added adaptation: {field_name} -> {pr_field}")
        
        return bool(missing_types or missing_fields)


# Main entry point
if __name__ == "__main__":
    xinu_sim = XinuSimMain()
    if xinu_sim.initialize():
        success = xinu_sim.run()
        sys.exit(0 if success else 1)
    else:
        sys.exit(1)