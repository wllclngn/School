# logger.py - Logging functionality for XINU builder

import os
import sys
from datetime import datetime

LOG_FILE = "compilation.txt"
SUMMARY_FILE = "compilation_summary.txt"
VERBOSE = False

# ANSI color codes for better terminal output
class Colors:
    HEADER = '\033[95m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

def setup_logger(verbose=False):
    # Configure the logger
    global VERBOSE, LOG_FILE, SUMMARY_FILE
    VERBOSE = verbose
    
    user_login = os.environ.get("USER", os.environ.get("USERNAME", "unknown"))
    now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    
    # Update file paths to include the output directory
    output_dir = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "output")
    os.makedirs(output_dir, exist_ok=True)
    
    LOG_FILE = os.path.join(output_dir, "compilation.txt")
    SUMMARY_FILE = os.path.join(output_dir, "compilation_summary.txt")
    
    # Create the log file with header
    with open(LOG_FILE, 'w') as f:
        f.write(f"XINU Compilation Log\nStarted: {user_login} at {now}\n")
    
    # Create summary file (will be merged at the end)
    with open(SUMMARY_FILE, 'w') as f:
        f.write("")
    
def log(message, verbose_only=False, summary=False, terminal_only=False, level="INFO"):
    # Write log message to file and console
    if verbose_only and not VERBOSE:
        return
    
    # Format the message for terminal display
    timestamp = datetime.now().strftime("%H:%M:%S")
    
    # Apply colors based on message level
    if level == "INFO":
        color = Colors.BLUE
        prefix = "INFO"
    elif level == "WARNING":
        color = Colors.YELLOW
        prefix = "WARNING"
    elif level == "ERROR":
        color = Colors.RED
        prefix = "ERROR"
    elif level == "SUCCESS":
        color = Colors.GREEN
        prefix = "SUCCESS"
    else:
        color = Colors.ENDC
        prefix = "DEBUG"
    
    # Print to terminal with color
    print(f"{color}[{timestamp}] {prefix}: {message}{Colors.ENDC}")
    
    # Don't write to log file if terminal_only is True
    if not terminal_only:
        # Format for log file (no colors)
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        formatted = f"{timestamp} - {message}"
        
        with open(LOG_FILE, 'a') as f:
            f.write(f"{formatted}\n")
        
        if summary:
            with open(SUMMARY_FILE, 'a') as f:
                f.write(f"{formatted}\n")

def finalize_log():
    # Merge summary into main log and clean up
    try:
        if os.path.exists(SUMMARY_FILE):
            with open(SUMMARY_FILE, 'r') as f:
                summary_content = f.read()
                
            with open(LOG_FILE, 'a') as f:
                f.write("\n--- Build Summary ---\n")
                f.write(summary_content)
                
            os.remove(SUMMARY_FILE)
            
            print(f"{Colors.GREEN}[COMPLETE] Build process finished. See {LOG_FILE} for details.{Colors.ENDC}")
    except Exception as e:
        print(f"{Colors.RED}[ERROR] Error finalizing log: {str(e)}{Colors.ENDC}")
        log(f"Error finalizing log: {str(e)}", level="ERROR")

# Auto-initialize the logger when the module is imported
# This ensures that the log file is created even if setup_logger is not explicitly called
setup_logger()
