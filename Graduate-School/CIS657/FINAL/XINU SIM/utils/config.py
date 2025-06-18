# config.py - Simplified configuration for XINU builder
import os
import platform
from utils.logger import log

class XinuConfig:
    # Configuration and path management for XINU builder
    
    def __init__(self, project_dir="."):
        self.project_dir = os.path.abspath(project_dir)
        
        # Find XINU OS directory
        self.xinu_os_dir = os.path.abspath(os.path.join(self.project_dir, "..", "XINU OS"))
        if not os.path.exists(self.xinu_os_dir):
            self.xinu_os_dir = self.project_dir  # Fall back if no "XINU OS" dir
            
        # Directory paths
        self.include_dir = os.path.join(self.xinu_os_dir, "include")
        self.compile_dir = os.path.join(self.xinu_os_dir, "compile")
            
        # Output paths - Simplified
        self.output_dir = os.path.join(self.project_dir, "output")
        self.obj_dir = os.path.join(self.output_dir, "obj")
        
        # Generated files - Only essential ones
        self.includes_h = os.path.join(self.output_dir, "xinu_includes.h")
        
        # Executable output
        self.xinu_core_output = os.path.join(self.output_dir, "xinu_core")
        if platform.system() == "Windows":
            self.xinu_core_output += ".exe"
            
        # Create necessary directories
        self._ensure_directories()
        
    def _ensure_directories(self):
        # Ensure necessary directories exist
        dirs = [
            self.output_dir,
            self.obj_dir
        ]
        
        for directory in dirs:
            os.makedirs(directory, exist_ok=True)
            
    def clean_build_files(self):
        # Clean generated and build files
        import glob
        
        # Clean output directory but keep the directory itself
        if os.path.exists(self.output_dir):
            # Remove files in output directory
            for item in os.listdir(self.output_dir):
                item_path = os.path.join(self.output_dir, item)
                if os.path.isfile(item_path):
                    os.remove(item_path)
                    log(f"Removed {item_path}")
                elif os.path.isdir(item_path):
                    if item == "obj":  # Handle the obj directory separately
                        for obj_file in glob.glob(os.path.join(item_path, "*.*")):
                            os.remove(obj_file)
                        log(f"Removed object files from {item_path}")
                    else:
                        import shutil
                        shutil.rmtree(item_path)
                        log(f"Removed {item_path}")
                
        # Recreate obj directory
        os.makedirs(self.obj_dir, exist_ok=True)
        
    def is_windows(self):
        # Check if running on Windows
        return platform.system() == "Windows"
        
    def run_simulation(self):
        # Run the XINU simulation
        import subprocess
        
        if not os.path.exists(self.xinu_core_output):
            log(f"Error: Cannot run simulation. {self.xinu_core_output} not found.")
            return False
            
        if not os.access(self.xinu_core_output, os.X_OK) and not self.is_windows():
            log(f"Error: {self.xinu_core_output} is not executable.")
            return False
            
        log(f"Running XINU simulation: {self.xinu_core_output}")
        
        try:
            # Use Popen but connect directly to the terminal I/O
            process = subprocess.Popen(
                self.xinu_core_output,
                shell=False,
                # Keep standard I/O connected to the terminal
                stdin=None,
                stdout=None,
                stderr=None
            )
            # Wait for the process to complete
            return process.wait() == 0
        except Exception as e:
            log(f"Error running simulation: {str(e)}")
            return False