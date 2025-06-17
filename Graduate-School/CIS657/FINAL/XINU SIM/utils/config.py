# config.py - Configuration management for XINU builder

import os
import glob
import subprocess
import platform
from xinu_sim.utils.logger import log

class XinuConfig:
    """Configuration and path management for XINU builder"""
    
    def __init__(self, project_dir="."):
        self.project_dir = os.path.abspath(project_dir)
        
        # Check for a structure where XINU OS is in a subdirectory
        self.xinu_os_dir = os.path.join(self.project_dir, "XINU OS")
        if not os.path.exists(self.xinu_os_dir):
            self.xinu_os_dir = self.project_dir  # Fall back if no "XINU OS" dir
            
        # Directory paths
        self.include_dir = os.path.join(self.project_dir, "include")
        if not os.path.exists(self.include_dir):
            self.include_dir = os.path.join(self.xinu_os_dir, "include")
            
        self.system_dir = os.path.join(self.xinu_os_dir, "system")
        self.device_dir = os.path.join(self.xinu_os_dir, "device")
        self.shell_dir = os.path.join(self.xinu_os_dir, "shell")
        self.libxc_dir = os.path.join(self.xinu_os_dir, "lib/libxc")
        self.compile_dir = os.path.join(self.xinu_os_dir, "compile")
            
        # Output paths
        self.builder_dir = os.path.join(self.project_dir, "xinu_sim")
        self.output_dir = os.path.join(self.builder_dir, "output")
        self.obj_dir = os.path.join(self.output_dir, "obj")
        
        # Generated files - ALL in output_dir
        self.stddefs_h = os.path.join(self.output_dir, "xinu_stddefs.h")
        self.includes_h = os.path.join(self.output_dir, "xinu_includes.h")
        self.sim_decls_h = os.path.join(self.output_dir, "xinu_sim_declarations.h")
        self.sim_helper_c = os.path.join(self.output_dir, "xinu_simulation.c")
        self.xinu_core_c = os.path.join(self.output_dir, "xinu_core.c")
        
        # Compilation log - ensure it's in output_dir
        self.compilation_log = os.path.join(self.output_dir, "compilation.txt")
        
        # Default xinu.h location
        self.xinu_h = os.path.join(self.include_dir, "xinu.h")
        
        # Check if xinu_sim has its own include dir
        self.builder_include_dir = os.path.join(self.builder_dir, "include")
        if os.path.exists(self.builder_include_dir):
            # If builder has its own include directory, use it for our generated xinu.h
            self.xinu_h = os.path.join(self.builder_include_dir, "xinu.h")
        
        # Executable output
        self.xinu_core_output = os.path.join(self.output_dir, "xinu_core")
        if platform.system() == "Windows":
            self.xinu_core_output += ".exe"
            
        # Create necessary directories
        self._ensure_directories()
        
    def _ensure_directories(self):
        """Ensure necessary directories exist"""
        dirs = [
            self.output_dir,
            self.obj_dir,
            os.path.dirname(self.xinu_h)  # Make sure directory for xinu.h exists
        ]
        
        for directory in dirs:
            os.makedirs(directory, exist_ok=True)
            
    def clean_build_files(self):
        """Clean generated and build files"""
        # Remove generated files
        files_to_clean = [
            self.stddefs_h,
            self.includes_h, 
            self.sim_decls_h,
            self.sim_helper_c,
            self.xinu_core_c,
            self.compilation_log,
        ]
        
        for file_path in files_to_clean:
            if os.path.exists(file_path):
                os.remove(file_path)
                log(f"Removed {file_path}")
                
        # Clean output directory but keep the directory itself
        if os.path.exists(self.output_dir):
            for item in os.listdir(self.output_dir):
                item_path = os.path.join(self.output_dir, item)
                if os.path.isfile(item_path) and item_path not in files_to_clean:
                    os.remove(item_path)
                    log(f"Removed {item_path}")
                elif os.path.isdir(item_path):
                    if item == "obj":  # Handle the obj directory separately
                        for obj_file in glob.glob(os.path.join(item_path, "*.o")):
                            os.remove(obj_file)
                        log(f"Removed object files from {item_path}")
                    else:
                        import shutil
                        shutil.rmtree(item_path)
                        log(f"Removed {item_path}")
                
        # Recreate obj directory
        os.makedirs(self.obj_dir, exist_ok=True)
        
    def is_windows(self):
        """Check if running on Windows"""
        return platform.system() == "Windows"
        
    def run_simulation(self, args=[]):
        """Run the XINU simulation"""
        if not os.path.exists(self.xinu_core_output):
            log(f"Error: Cannot run simulation. {self.xinu_core_output} not found.")
            return 1
            
        if not os.access(self.xinu_core_output, os.X_OK) and not self.is_windows():
            log(f"Error: {self.xinu_core_output} is not executable.")
            return 1
            
        log(f"Running XINU simulation: {self.xinu_core_output} {' '.join(args)}")
        
        try:
            cmd = [self.xinu_core_output] + args
            process = subprocess.Popen(cmd)
            return process.wait()
        except Exception as e:
            log(f"Error running simulation: {str(e)}")
            return 1
