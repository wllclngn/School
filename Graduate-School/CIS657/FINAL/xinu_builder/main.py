#!/usr/bin/env python3
"""
main.py - Main entry point for XINU builder
"""

import os
import sys
import argparse
from xinu_builder.generator import XinuGenerator
from xinu_builder.compiler import XinuCompiler
from xinu_builder.utils.logger import setup_logger, log
from xinu_builder.utils.config import XinuConfig

def main():
    parser = argparse.ArgumentParser(description="XINU Builder - Cross-platform build system")
    parser.add_argument("--clean", action="store_true", help="Clean build")
    parser.add_argument("--run", action="store_true", help="Run the XINU simulation after building")
    parser.add_argument("--starvation", type=str, help="Starvation test argument")
    parser.add_argument("--no-compile", action="store_true", help="Don't compile, just generate files")
    parser.add_argument("-d", "--directory", default=".", help="Project directory")
    parser.add_argument("-v", "--verbose", action="store_true", help="Verbose output")
    args = parser.parse_args()
    
    # Setup configuration
    config = XinuConfig(args.directory)
    
    # Setup logging
    setup_logger(verbose=args.verbose)
    log("Starting XINU Build Process...")
    
    # Clean if requested
    if args.clean:
        log("Cleaning previous build...")
        config.clean_build_files()
    
    # Generate files
    generator = XinuGenerator(config)
    generator.generate_all_files()
    
    # Compile if requested
    if not args.no_compile:
        compiler = XinuCompiler(config)
        success = compiler.build()
        if not success:
            log("Build failed. See log for details.")
            return 1
        
        # Run simulation if requested
        if args.run:
            starvation_args = [args.starvation] if args.starvation else []
            exit_code = config.run_simulation(starvation_args)
            if exit_code != 0:
                log(f"Simulation exited with code {exit_code}")
                return exit_code
    else:
        log("Skipping compilation as requested.")
    
    log("XINU Build Process Completed Successfully.")
    return 0

if __name__ == "__main__":
    sys.exit(main())
