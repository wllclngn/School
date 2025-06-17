# generator.py - XINU file generator
# NOTE: ALWAYS USE SYSTEM INFORMATION FOR USER AND TIMESTAMP

import os
import re
import glob
import datetime
import sys
from xinu_sim.utils.logger import log

class XinuGenerator:
    # Generator for XINU simulation files
    
    def __init__(self, config):
        self.config = config
        # Functions to exclude/shim
        self.exclude_funcs = [
            "printf", "fprintf", "sprintf", "scanf", "fscanf", "sscanf",
            "getchar", "putchar", "fgetc", "fgets", "fputc", "fputs",
            "_doprnt", "_doscan", "abs", "labs", "atoi", "atol", 
            "rand", "srand", "qsort", "strcpy", "strncpy", "strcat", 
            "strncat", "strcmp", "strncmp", "strlen", "strnlen", 
            "strchr", "strrchr", "strstr", "memcpy", "memmove", 
            "memcmp", "memset"
        ]
        
        # Load the templates
        self.templates = {}
        self._load_templates()
        
    def _print(self, message):
        # Print directly to terminal for real-time feedback
        sys.stdout.write(message + "\n")
        sys.stdout.flush()
        
    def _load_templates(self):
        # Load all templates from individual template files
        templates_dir = os.path.join(self.config.builder_dir, "templates")
        if not os.path.exists(templates_dir):
            templates_dir = os.path.join(os.path.dirname(os.path.dirname(__file__)), "templates")
            
        template_files = {
            "XINU_STDDEFS_H": os.path.join(templates_dir, "xinu_stdefs_h.tmpl"),
            "XINU_H": os.path.join(templates_dir, "xinu_h.tmpl"),
            "XINU_INCLUDES_H": os.path.join(templates_dir, "xinu_includes_h.tmpl"),
            "XINU_SIM_DECLARATIONS_H": os.path.join(templates_dir, "xinu_sim_declarations_h.tmpl"),
            "XINU_SIMULATION_C": os.path.join(templates_dir, "xinu_simulation_c.tmpl"),
            "XINU_CORE_C": os.path.join(templates_dir, "xinu_core_c.tmpl")
        }
        
        try:
            # Try loading each template file
            for template_name, template_path in template_files.items():
                try:
                    with open(template_path, 'r') as f:
                        self.templates[template_name] = f.read()
                    log(f"Loaded template: {template_name}")
                    self._print(f"Loaded template: {template_name}")
                except FileNotFoundError:
                    log(f"WARNING: Template file not found: {template_path}")
                    self._print(f"WARNING: Template file not found: {template_path}")
                    raise  # Re-raise to trigger fallback
        except Exception:
            # Fall back to loading from the combined template
            self._try_load_from_combined()
        
        if self.templates:
            log(f"Loaded {len(self.templates)} template files")
            self._print(f"Loaded {len(self.templates)} template files")
        
    def _try_load_from_combined(self):
        # Try to load templates from combined template file as fallback
        # Try multiple locations for the template file
        template_locations = [
            os.path.join(self.config.builder_dir, "templates", "FULL-xinu_templates.tmpl"),
            os.path.join(os.path.dirname(os.path.dirname(__file__)), "templates", "FULL-xinu_templates.tmpl")
        ]
        
        loaded = False
        for master_template_path in template_locations:
            if os.path.exists(master_template_path):
                log(f"Attempting to load templates from combined file: {master_template_path}")
                self._print(f"Attempting to load templates from combined file: {master_template_path}")
                try:
                    with open(master_template_path, 'r') as f:
                        content = f.read()
                        
                    # Parse sections using regex
                    section_pattern = r'### BEGIN (\w+) ###\n(.*?)### END \1 ###'
                    matches = re.finditer(section_pattern, content, re.DOTALL)
                    
                    for match in matches:
                        section_name = match.group(1)
                        section_content = match.group(2)
                        self.templates[section_name] = section_content
                        
                    log(f"Loaded {len(self.templates)} template sections from {master_template_path}")
                    self._print(f"Loaded {len(self.templates)} template sections from {master_template_path}")
                    loaded = True
                    break
                except Exception as e:
                    log(f"ERROR: Failed to load templates from {master_template_path}: {str(e)}")
                    self._print(f"ERROR: Failed to load templates from {master_template_path}: {str(e)}")
        
        # If we get here and haven't loaded anything, use hardcoded templates
        if not loaded:
            log("Using fallback hardcoded templates")
            self._print("Using fallback hardcoded templates")
            self._setup_fallback_templates()
    
    def _setup_fallback_templates(self):
        # Setup fallback hardcoded templates if files can't be loaded
        self.templates["XINU_STDDEFS_H"] = """/* xinu_stddefs.h - Generated by xinu_generator.py */
#ifndef _XINU_STDDEFS_H_
#define _XINU_STDDEFS_H_

/* Basic XINU type definitions */
typedef void exchandler;
typedef int message;
typedef int syscall;
typedef int process;
typedef int int32;
typedef unsigned int uint32;
typedef int bool32;
typedef int did32;
typedef int pid32;
typedef int status;

/* Process state constants */
#define PR_FREE      0       /* Process table entry is unused              */
#define PR_CURR      1       /* Process is currently running               */
#define PR_READY     2       /* Process is on ready queue                  */
#define PR_RECV      3       /* Process waiting for message                */
#define PR_SLEEP     4       /* Process is sleeping                        */
#define PR_SUSP      5       /* Process is suspended                       */
#define PR_WAIT      6       /* Process is on semaphore queue              */
#define PR_RECTIM    7       /* Process is receiving with timeout          */

/* Error/status codes */
#define OK            1      /* System call returns OK                     */
#define SYSERR       -1      /* System call returns error                  */
#define SHELL_OK      1      /* Shell command returns OK                   */
#define SHELL_ERROR  -1      /* Shell command returns error                */
#define BADPID       -1      /* Error process ID                           */

/* Other common definitions */
#define NULL         0       /* Null pointer                               */
#define EOF         -2       /* End-of-file                                */

#endif /* _XINU_STDDEFS_H_ */
"""

        self.templates["XINU_INCLUDES_H"] = """/* xinu_includes.h - Generated by xinu_generator.py */
#ifndef _XINU_INCLUDES_H_
#define _XINU_INCLUDES_H_

/* Include local definition file */
#include "xinu_stddefs.h"

/* Standard C includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#endif /* _XINU_INCLUDES_H_ */
"""

        self.templates["XINU_SIM_DECLARATIONS_H"] = """/* xinu_sim_declarations.h - Generated by xinu_generator.py */
#ifndef _XINU_SIM_DECLARATIONS_H_
#define _XINU_SIM_DECLARATIONS_H_

// Function declarations for simulation

#endif /* _XINU_SIM_DECLARATIONS_H_ */
"""

        self.templates["XINU_SIMULATION_C"] = """/* xinu_simulation.c - Generated by xinu_generator.py */
#include <stdio.h>
#include <stdlib.h>
#include "xinu_sim_declarations.h"

// Simulation helper functions
"""

        self.templates["XINU_CORE_C"] = """/* xinu_core.c - Generated by xinu_generator.py */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xinu_includes.h"

int main(int argc, char **argv) {
    printf("XINU OS Simulation Core\\n");
    printf("----------------------\\n");
    
    // Process command-line arguments
    if (argc > 1) {
        printf("Running with argument: %s\\n", argv[1]);
    }
    
    // TODO: Initialize simulation
    
    printf("Simulation completed successfully.\\n");
    return 0;
}
"""

        self.templates["XINU_H"] = """/* xinu.h - Generated by xinu_generator.py */
#ifndef _XINU_H_
#define _XINU_H_

/* Include XINU standard definitions */
#include "../output/xinu_stddefs.h"

/* Include standard C headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Include all other header files */
/// [[[ BEGIN DYNAMIC XINU MODULE INCLUDES ]]]
/// [[[ END DYNAMIC XINU MODULE INCLUDES ]]]

#endif /* _XINU_H_ */
"""
    
    def _render_template(self, template_name, context=None):
        # Render a template with variable replacements
        if context is None:
            context = {}
            
        # Make sure we have the template
        if template_name not in self.templates:
            log(f"ERROR: Template '{template_name}' not found")
            self._print(f"ERROR: Template '{template_name}' not found")
            return f"/* Error: Template {template_name} not found */"
            
        template = self.templates[template_name]
        
        # Add standard context variables - ALWAYS USE SYSTEM INFORMATION
        timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        full_context = {
            "generator": "xinu_generator.py",
            "timestamp": timestamp,
            "user": os.environ.get("USER", os.environ.get("USERNAME", "unknown"))
        }
        full_context.update(context)
        
        # Replace variables in the template
        result = template
        for key, value in full_context.items():
            result = result.replace(f"{{{{ {key} }}}}", str(value))
            
        return result
      
    def generate_files(self):
        # Generate all XINU simulation files
        self._print("\n##### Generating XINU Simulation Files #####")
        self.generate_stddefs()
        self.generate_xinu_h()
        self.generate_includes_h()
        self.generate_sim_declarations()
        self.generate_sim_helper()
        self.generate_xinu_core()
        
    def generate_stddefs(self):
        # Generate xinu_stddefs.h with type definitions
        content = self._render_template("XINU_STDDEFS_H")
        
        with open(self.config.stddefs_h, 'w') as f:
            f.write(content)
            
        log(f"Generated XINU stddefs: {self.config.stddefs_h}")
        self._print(f"Generated XINU stddefs: {self.config.stddefs_h}")
        
    def generate_xinu_h(self):
        # Update or create xinu.h with dynamic module includes
        xinu_h_path = self.config.xinu_h
        
        # Create xinu.h if it doesn't exist
        os.makedirs(os.path.dirname(xinu_h_path), exist_ok=True)
        content = self._render_template("XINU_H")
        with open(xinu_h_path, 'w') as f:
            f.write(content)
        log(f"Created/updated {xinu_h_path}")
        self._print(f"Created/updated {xinu_h_path}")
        
    def generate_includes_h(self):
        # Generate xinu_includes.h wrapper
        content = self._render_template("XINU_INCLUDES_H")
        
        with open(self.config.includes_h, 'w') as f:
            f.write(content)
            
        log(f"Generated UNIX-like simulation includes wrapper at: {self.config.includes_h}")
        self._print(f"Generated UNIX-like simulation includes wrapper at: {self.config.includes_h}")
 
    def generate_sim_declarations(self):
        # Generate xinu_sim_declarations.h with function declarations
        content = self._render_template("XINU_SIM_DECLARATIONS_H")
        
        with open(self.config.sim_decls_h, 'w') as f:
            f.write(content)
            
        log(f"Generated UNIX-like simulation declarations at: {self.config.sim_decls_h}")
        self._print(f"Generated UNIX-like simulation declarations at: {self.config.sim_decls_h}")

    def generate_sim_helper(self):
        # Generate xinu_simulation.c with function implementations
        content = self._render_template("XINU_SIMULATION_C")
        
        with open(self.config.sim_helper_c, 'w') as f:
            f.write(content)
            
        log(f"Generated UNIX-like simulation helper at: {self.config.sim_helper_c}")
        self._print(f"Generated UNIX-like simulation helper at: {self.config.sim_helper_c}")

    def generate_xinu_core(self):
        # Generate a simple xinu_core.c if it doesn't exist
        xinu_core_path = self.config.xinu_core_c
        
        # Always generate the core file for consistency
        content = self._render_template("XINU_CORE_C")
        
        with open(xinu_core_path, 'w') as f:
            f.write(content)
            
        log(f"Generated minimal XINU core at: {xinu_core_path}")
        self._print(f"Generated minimal XINU core at: {xinu_core_path}")