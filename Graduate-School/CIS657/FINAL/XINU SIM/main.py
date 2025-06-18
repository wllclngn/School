# main.py - XINU SIM main entry point
# Direct compilation approach based on PowerShell script methodology

import os
import sys
import argparse
import datetime
import platform
from pathlib import Path

# Add current directory to path for local imports
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

# Import local modules
from utils.logger import log, setup_logger
from utils.config import XinuConfig
from generator import XinuGenerator
from compiler import XinuCompiler

def setup_logging(level='INFO'):
    # Setup logging with specified level
    verbose = level.lower() == 'debug'
    return setup_logger(verbose=verbose)

class XinuSimMain:
    # Main class for XINU simulation
    
    def __init__(self):
        self.config = XinuConfig()
        self.generator = None
        self.compiler = None
        
    def _print(self, message):
        # Print directly to terminal for real-time feedback
        sys.stdout.write(message + "\n")
        sys.stdout.flush()
        
    def setup_args(self):
        # Parse command-line arguments
        parser = argparse.ArgumentParser(
            description='XINU OS Simulation System',
            epilog='Streamlined compilation for XINU OS.'
        )
        
        parser.add_argument('-x', '--xinu-dir', dest='xinu_dir',
                           help='Path to XINU OS directory')
        parser.add_argument('-o', '--output-dir', dest='output_dir',
                           help='Path to output directory')
        parser.add_argument('-r', '--rebuild-all', dest='rebuild_all',
                           action='store_true',
                           help='Force rebuild of all components')
        parser.add_argument('-c', '--clean', dest='rebuild_all',
                           action='store_true',
                           help='Clean and rebuild (alias for --rebuild-all)')
        parser.add_argument('-v', '--verbose', dest='verbose',
                           action='store_true',
                           help='Enable verbose output')
        parser.add_argument('-d', '--debug', dest='debug',
                           action='store_true',
                           help='Enable debug mode')
        parser.add_argument('--run', dest='run_sim',
                           action='store_true',
                           help='Run the simulation after building')
        
        args = parser.parse_args()
        return args
    
    def initialize(self):
        # Initialize the XINU simulation system
        # Parse command-line arguments
        args = self.setup_args()
        
        # Setup logging
        log_level = 'DEBUG' if getattr(args, 'debug', False) else 'INFO'
        setup_logging(log_level)
        
        # Update config from args
        if getattr(args, 'xinu_dir', None):
            self.config.xinu_os_dir = os.path.abspath(args.xinu_dir)
            self.config.include_dir = os.path.join(self.config.xinu_os_dir, "include")
            self.config.compile_dir = os.path.join(self.config.xinu_os_dir, "compile")
            
        if getattr(args, 'output_dir', None):
            self.config.output_dir = os.path.abspath(args.output_dir)
            self.config.obj_dir = os.path.join(self.config.output_dir, "obj")
            self.config.includes_h = os.path.join(self.config.output_dir, "xinu_includes.h")
            
        # Clean if requested
        if getattr(args, 'rebuild_all', False):
            self.config.clean_build_files()
        
        # Display welcome message
        current_time = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        username = os.environ.get("USER", os.environ.get("USERNAME", "unknown"))
        
        self._print("\n##############################################")
        self._print("#  XINU Simulation System - Streamlined Build")
        self._print("##############################################")
        self._print(f"Date/Time: {current_time}")
        self._print(f"User: {username}")
        self._print(f"System: {platform.system()} {platform.release()}")
        self._print("##############################################\n")
        
        # Check XINU OS directory
        if not os.path.exists(self.config.xinu_os_dir):
            self._print(f"ERROR: XINU OS directory not found at: {self.config.xinu_os_dir}")
            return False
            
        # Setup objects
        self.generator = XinuGenerator(self.config)
        self.compiler = XinuCompiler(self.config)
        
        # Store run flag for later
        self.run_after_build = getattr(args, 'run_sim', False)
        
        return True
    
    def run(self):
        # Run the XINU simulation build process
        # Generate minimal wrapper files
        self._print("\n### Generating XINU Simulation Wrapper Files ###")
        self.generator.generate_files()
        
        # Compile simulation using direct Makefile sources
        self._print("\n### Compiling XINU Simulation ###")
        compile_result = self.compiler.compile()
        
        # Report result
        if compile_result:
            self._print("\n### Build Successful ###")
            self._print(f"\nExecutable created: {self.config.xinu_core_output}")
            
            # Run if requested
            if self.run_after_build:
                self._print("\nRunning simulation...")
                self.config.run_simulation()
            else:
                self._print("\nRun the simulation with:")
                self._print(f"  {self.config.xinu_core_output}")
            
            return True
        else:
            self._print("\n### Build Failed ###")
            self._print("\nPlease check the error messages above and fix any issues.")
            return False


# Main entry point
if __name__ == "__main__":
    xinu_sim = XinuSimMain()
    if xinu_sim.initialize():
        success = xinu_sim.run()
        sys.exit(0 if success else 1)
    else:
        sys.exit(1)