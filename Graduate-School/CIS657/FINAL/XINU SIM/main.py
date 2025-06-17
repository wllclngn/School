#!/usr/bin/env python3
"""
main.py - Main entry point for XINU builder
"""

import os
import sys
import argparse
import platform
import datetime
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
    parser.add_argument("-o", "--output-dir", help="Output directory (defaults to 'output' under xinu_builder dir)")
    args = parser.parse_args()
    
    # Setup configuration
    project_dir = os.path.abspath(args.directory)
    config = XinuConfig(project_dir)
    
    # Override output directory if specified
    if args.output_dir:
        output_dir = os.path.abspath(args.output_dir)
        config.output_dir = output_dir
        config.obj_dir = os.path.join(output_dir, "obj")
        config.xinu_core_output = os.path.join(output_dir, "xinu_core")
        if config.is_windows():
            config.xinu_core_output += ".exe"
        
        # Update generated file paths
        config.stddefs_h = os.path.join(output_dir, "xinu_stddefs.h")
        config.includes_h = os.path.join(output_dir, "xinu_includes.h")
        config.sim_decls_h = os.path.join(output_dir, "xinu_sim_declarations.h")
        config.sim_helper_c = os.path.join(output_dir, "xinu_simulation.c")
        config.xinu_core_c = os.path.join(output_dir, "xinu_core.c")
        config.compilation_log = os.path.join(output_dir, "compilation.txt")
        
        # Ensure directories exist
        os.makedirs(config.obj_dir, exist_ok=True)
    
    # Remove any old compilation_summary.txt file in the project directory
    compilation_summary = os.path.join(project_dir, "compilation_summary.txt")
    if os.path.exists(compilation_summary):
        try:
            os.remove(compilation_summary)
            log(f"Removed old compilation_summary.txt")
        except Exception as e:
            log(f"Warning: Could not remove old compilation_summary.txt: {str(e)}")
    
    # Setup logging
    setup_logger(verbose=args.verbose)
    
    # Output basic information
    timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    username = os.environ.get('USER', os.environ.get('USERNAME', 'unknown'))
    system_info = platform.system() + " " + platform.release()
    
    log("=======================================")
    log(f"XINU Builder v1.0 - {timestamp}")
    log(f"User: {username} on {system_info}")
    log("=======================================")
    log("Starting XINU Build Process...")
    log(f"Project directory: {config.project_dir}")
    log(f"Output directory: {config.output_dir}")
    
    # Clean if requested
    if args.clean:
        log("Cleaning previous build...")
        config.clean_build_files()
        
        # Also clean any stray root directory compilation files
        root_compilation = os.path.join(project_dir, "compilation.txt")
        if os.path.exists(root_compilation):
            try:
                os.remove(root_compilation)
                log(f"Removed stray compilation.txt from project root")
            except Exception as e:
                log(f"Warning: Could not remove stray compilation.txt: {str(e)}")
    
    # Generate files
    log("Generating XINU simulation files...")
    generator = XinuGenerator(config)
    generator.generate_files()
    
    # Compile if requested
    if not args.no_compile:
        compiler = XinuCompiler(config)
        success = compiler.build()
        if not success:
            log(f"Build failed. See log at {config.compilation_log} for details.")
            return 1
        
        # Run simulation if requested
        if args.run:
            starvation_args = [args.starvation] if args.starvation else []
            log(f"Running XINU simulation with args: {starvation_args}")
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
