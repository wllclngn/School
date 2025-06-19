# main.py - Main entry point for XINU Simulation builder
import os
import sys
import argparse
import datetime
import platform
import subprocess
from utils.config import XinuConfig
from utils.logger import log
from generator import XinuGenerator
from compiler import XinuCompiler
import shutil
import re

class XinuSimMain:
    # Main orchestrator for XINU simulation build

    def __init__(self, args, timestamp=None):
        # Initialize the XINU simulation builder
        self.args = args

        # Use provided timestamp or generate a new one
        if timestamp is None:
            self.timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        else:
            self.timestamp = timestamp

        # Set up configuration - pass the project directory instead of args
        self.config = XinuConfig(args.directory)

        # If custom output dir specified, update it
        if args.output_dir:
            self.config.output_dir = os.path.abspath(args.output_dir)
            self.config.obj_dir = os.path.join(self.config.output_dir, "obj")

        # Initialize components
        self.generator = XinuGenerator(self.config)
        self.compiler = XinuCompiler(self.config)

        # Determine if we should run after build
        self.run_after_build = args.run

    def _print(self, message):
        # Print messages with consistent formatting
        print(message)

    def _print_header(self):
        # Print concise header information for the build (user-facing only)
        system_info = f"{platform.system()} {platform.release()}"
        self._print("-" * 40)
        log(f"XINU Simulation System - Streamlined Build on {system_info}")
        self._print(f"Date/Time: {self.timestamp}")
        self._print(f"User: {os.environ.get('USER', os.environ.get('USERNAME', 'unknown'))}")
        self._print(f"System: {system_info}")
        self._print("-" * 40)

    def clean(self):
        # Clean up output artifacts
        log("Cleaning output files")

        # List of file patterns to remove
        patterns = [
            "*.o", "*.obj",                    # Object files
            "xinu_core", "xinu_core.exe",      # Executables
            "xinu_*.h", "xinu_*.c",            # Generated source files
            "base_types.h",                    # Special header
            "circular_includes_error.txt",     # Error logs
            "compilation.txt",                 # Compilation log
            "compilation_summary.txt"          # Build summary
        ]

        output_dir = self.config.output_dir
        obj_dir = self.config.obj_dir

        # Remove specific files in output directory
        for pattern in patterns:
            for f in self._find_files(output_dir, pattern):
                try:
                    os.remove(f)
                    log(f"Removed {f}")
                except Exception as e:
                    log(f"Error removing {f}: {str(e)}")

        # Clean object files separately
        for f in self._find_files(obj_dir, "*.o"):
            try:
                os.remove(f)
                log(f"Removed {f}")
            except Exception as e:
                log(f"Error removing {f}: {str(e)}")
        log(f"Removed object files from {obj_dir}")

        # Also clean XINU include directory from our generated files
        if hasattr(self.config, 'include_dir'):
            for pattern in ["xinu_*.h", "base_types.h"]:
                for f in self._find_files(self.config.include_dir, pattern):
                    try:
                        os.remove(f)
                        log(f"Removed {f}")
                    except Exception as e:
                        log(f"Error removing {f}: {str(e)}")

    def _find_files(self, directory, pattern):
        # Find files matching a pattern
        if not os.path.exists(directory):
            return []

        result = []
        for root, _, files in os.walk(directory):
            for name in files:
                if self._match_pattern(name, pattern):
                    result.append(os.path.join(root, name))

        return result

    def _match_pattern(self, filename, pattern):
        # Match filename against a pattern with wildcards
        if pattern == "*":
            return True

        # Convert glob pattern to regex pattern
        regex_pattern = "^" + pattern.replace(".", "\\.").replace("*", ".*") + "$"
        return bool(re.match(regex_pattern, filename))

    def run(self):
        # Run the XINU simulation build process
        log("Generating XINU Simulation Wrapper Files")
        self.generator.generate_files()

        log("Compiling XINU Simulation")
        compile_result = self.compiler.compile_with_retry()  # Use the new retry method

        if compile_result:
            log("Build Successful")
            log(f"Executable created: {self.config.xinu_core_output}")

            if self.run_after_build:
                log("Running simulation...")
                if hasattr(self.config, "run_simulation"):
                    self.config.run_simulation()
                else:
                    try:
                        subprocess.run([self.config.xinu_core_output])
                    except Exception as e:
                        log(f"Error running simulation: {e}")
            else:
                log(f"Run the simulation with: {self.config.xinu_core_output}")
            return True
        else:
            log("Build Failed")
            log("Please check the error messages above and fix any issues.")
            return False


def parse_args():
    # Parse command-line arguments
    parser = argparse.ArgumentParser(description="XINU Simulation Builder")

    parser.add_argument("--clean", action="store_true", help="Clean up output files before build")
    parser.add_argument("--run", action="store_true", help="Run the simulation after build")
    parser.add_argument("--verbose", "-v", action="store_true", help="Enable verbose output")
    parser.add_argument("--no-compile", action="store_true", help="Generate files only (no compilation)")
    parser.add_argument("--starvation", type=int, default=0, help="Set starvation testing parameter")
    parser.add_argument("--directory", "-d", type=str, default=".", help="Project root directory")
    parser.add_argument("--output-dir", "-o", type=str, help="Custom output directory")

    return parser.parse_args()


def main():
    # Main entry point
    args = parse_args()

    if args.verbose:
        os.environ["XINU_VERBOSE"] = "1"

    timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    log(f"XINU Builder Started at {timestamp}")
    log(f"User: {os.environ.get('USER', os.environ.get('USERNAME', 'unknown'))}")
    log(f"System: {platform.system()} {platform.release()}")

    xinu_sim = XinuSimMain(args, timestamp=timestamp)
    xinu_sim._print_header()

    if args.clean:
        xinu_sim.clean()

    if not args.no_compile:
        result = xinu_sim.run()
        sys.exit(0 if result else 1)
    else:
        log("Skipping compilation (--no-compile specified)")
        sys.exit(0)


if __name__ == "__main__":
    main()
