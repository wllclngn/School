# generator.py - XINU file generator using g++ preprocessing

import os
import re
import subprocess
import tempfile
import datetime
import sys
from utils.logger import log

class XinuGenerator:
    # Generator for XINU simulation files using g++ preprocessing.
    
    def __init__(self, config):
        self.config = config
        self.templates = {}
        self._load_templates()
        self.preprocessed_content = None
        self.extracted_symbols = {}
        self.missing_types = []
        
    def _print(self, message):
        sys.stdout.write(message + "\n")
        sys.stdout.flush()
        
    def _load_templates(self):
        # Try to find templates in the project directory first
        templates_dir = os.path.join(self.config.project_dir, "templates")
        if not os.path.exists(templates_dir):
            # Create templates directory if it doesn't exist
            try:
                os.makedirs(templates_dir, exist_ok=True)
                log(f"Created templates directory: {templates_dir}")
            except Exception as e:
                log(f"Error creating templates directory: {str(e)}")
            
        # Define template files mapping
        template_files = {
            "XINU_STDDEFS_H": os.path.join(templates_dir, "xinu_stdefs_h.tmpl"),
            "XINU_H": os.path.join(templates_dir, "xinu_h.tmpl"),
            "XINU_INCLUDES_H": os.path.join(templates_dir, "xinu_includes_h.tmpl"),
            "XINU_SIM_DECLARATIONS_H": os.path.join(templates_dir, "xinu_sim_declarations_h.tmpl"),
            "XINU_SIMULATION_C": os.path.join(templates_dir, "xinu_simulation_c.tmpl"),
            "XINU_CORE_C": os.path.join(templates_dir, "xinu_core_c.tmpl")
        }
        
        for template_name, template_path in template_files.items():
            try:
                with open(template_path, 'r') as f:
                    self.templates[template_name] = f.read()
                log(f"Loaded template: {template_name}")
                self._print(f"Loaded template: {template_name}")
            except FileNotFoundError:
                log(f"WARNING: Template file not found: {template_path}")
                self._print(f"WARNING: Template file not found: {template_path}")
        
        if not self.templates:
            log("Using fallback hardcoded templates")
            self._print("Using fallback hardcoded templates")
            self._setup_fallback_templates()
    
    def _setup_fallback_templates(self):
        # Setup basic fallback templates when files are not found
        self.templates["XINU_STDDEFS_H"] = """/* xinu_stddefs.h - Generated {{ timestamp }} by {{ user }} */
#ifndef _XINU_STDDEFS_H_
#define _XINU_STDDEFS_H_

/* Basic type definitions for XINU simulation */
typedef int bool;
typedef int pid32;
typedef int sid32;
typedef int qid16;
typedef int int32;
typedef unsigned int uint32;
typedef int intmask;
typedef int status;
typedef int message;
typedef int did32;
typedef void (*interrupt_handler_t)();
typedef int syscall;
typedef void (*exchandler)();
typedef unsigned int memblk;

/* Constants */
#define TRUE        1
#define FALSE       0
#define SYSERR     (-1)
#define OK          1
#define READY       1
#define SUSPENDED   2
#define WAITING     3

#endif /* _XINU_STDDEFS_H_ */
"""

        self.templates["XINU_H"] = """/* xinu.h - Generated {{ timestamp }} by {{ user }} */
#ifndef _XINU_H_
#define _XINU_H_

#include "xinu_stddefs.h"

/* Core XINU structure and function declarations */
struct procent {
    void    *prstkptr;      /* Saved stack pointer                */
    int     prio;           /* Process priority                   */
    int     prstate;        /* Process state: READY or SUSPENDED  */
    char    prname[16];     /* Process name                       */
    int     prsem;          /* Semaphore on which process waits   */
    pid32   prparent;       /* ID of the creating process         */
    int     prnxtkin;       /* Next-of-kin notified of death      */
};

#endif /* _XINU_H_ */
"""

        self.templates["XINU_INCLUDES_H"] = """/* xinu_includes.h - Generated {{ timestamp }} by {{ user }} */
#ifndef _XINU_INCLUDES_H_
#define _XINU_INCLUDES_H_

#include "xinu_stddefs.h"
#include "xinu.h"

/* Additional includes will be added dynamically */

#endif /* _XINU_INCLUDES_H_ */
"""

        # Add other fallback templates as needed
        log("Setup fallback templates completed")
        
    def add_missing_types(self, type_list):
        # Add missing types to be included in next generation
        for type_name in type_list:
            if type_name not in self.missing_types:
                self.missing_types.append(type_name)
                log(f"Added type {type_name} to missing types list")
        
    def preprocess_headers(self):
        # Use g++ to preprocess XINU headers and extract definitions.
        self._print("\n##### Preprocessing XINU Headers with g++ #####")
        
        # Find main XINU header
        xinu_h_path = os.path.join(self.config.include_dir, "xinu.h")
        if not os.path.exists(xinu_h_path):
            # Try other common locations
            xinu_include_dir = os.path.join(self.config.xinu_os_dir, "include")
            xinu_h_path = os.path.join(xinu_include_dir, "xinu.h")
        
        if not os.path.exists(xinu_h_path):
            log(f"ERROR: Cannot find xinu.h in standard locations")
            self._print(f"ERROR: Cannot find xinu.h in standard locations")
            return False
        
        # Create a temporary file with inclusion of xinu.h
        with tempfile.NamedTemporaryFile(suffix=".c", delete=False) as temp_file:
            temp_path = temp_file.name
            temp_file.write(f"#include \"{xinu_h_path}\"\n".encode('utf-8'))
        
        # Build include paths with proper quoting to handle spaces
        include_paths = [
            self.config.include_dir,
            os.path.join(self.config.xinu_os_dir, "include"),
            os.path.dirname(xinu_h_path)
        ]
        include_opts = " ".join(f'-I"{p}"' for p in include_paths if os.path.exists(p))
        
        # Run g++ preprocessor with properly quoted paths
        try:
            cmd = f'g++ -E {include_opts} "{temp_path}"'
            self._print(f"Running: {cmd}")
            process = subprocess.Popen(
                cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                universal_newlines=True
            )
            stdout, stderr = process.communicate()
            
            if process.returncode != 0:
                log(f"ERROR: g++ preprocessing failed: {stderr}")
                self._print(f"ERROR: g++ preprocessing failed: {stderr}")
                return False
            
            # Store preprocessed output
            self.preprocessed_content = stdout
            self._print(f"Successfully preprocessed XINU headers ({len(stdout)} bytes)")
            
            # Clean up temp file
            os.unlink(temp_path)
            return True
        except Exception as e:
            log(f"ERROR: Failed to run g++ preprocessor: {str(e)}")
            self._print(f"ERROR: Failed to run g++ preprocessor: {str(e)}")
            return False
    
    def extract_symbols(self):
        # Extract symbols from preprocessed content.
        if not self.preprocessed_content:
            return False
        
        # Extract typedefs
        typedefs = re.findall(r'typedef\s+(.*?)\s+(\w+)\s*;', self.preprocessed_content, re.DOTALL)
        self.extracted_symbols['typedefs'] = {name: type_def for type_def, name in typedefs}
        
        # Extract defines
        defines = re.findall(r'#define\s+(\w+)\s+(.*?)$', self.preprocessed_content, re.MULTILINE)
        self.extracted_symbols['defines'] = {name: value for name, value in defines}
        
        # Extract structs
        structs = re.findall(r'struct\s+(\w+)\s*{(.*?)};', self.preprocessed_content, re.DOTALL)
        self.extracted_symbols['structs'] = {name: body for name, body in structs}
        
        # Extract function prototypes
        funcs = re.findall(r'(\w+)\s+(\w+)\s*\((.*?)\)\s*;', self.preprocessed_content, re.DOTALL)
        self.extracted_symbols['functions'] = {
            name: {'return_type': ret, 'params': params} 
            for ret, name, params in funcs 
            if ret not in ('struct', 'class', 'enum', 'union')
        }
        
        self._print(f"Extracted {len(self.extracted_symbols['typedefs'])} typedefs")
        self._print(f"Extracted {len(self.extracted_symbols['defines'])} defines")
        self._print(f"Extracted {len(self.extracted_symbols['structs'])} structs")
        self._print(f"Extracted {len(self.extracted_symbols['functions'])} functions")
        
        # Add essential symbols that might be missing
        self._add_missing_symbols()
        return True
    
    def _add_missing_symbols(self):
        # Add essential symbols if they're missing from extraction.
        # Essential typedefs to ensure are defined
        essential_types = {
            'bool8': 'int', 'shellcmd': 'int', 'intmask': 'int',
            'exchandler': 'void', 'message': 'int', 'syscall': 'int',
            'process': 'int', 'int32': 'int', 'uint32': 'unsigned int',
            'bool32': 'int', 'did32': 'int', 'pid32': 'int', 'status': 'int'
        }
        
        # Add any missing essential types
        for name, type_def in essential_types.items():
            if name not in self.extracted_symbols.get('typedefs', {}):
                if 'typedefs' not in self.extracted_symbols:
                    self.extracted_symbols['typedefs'] = {}
                self.extracted_symbols['typedefs'][name] = type_def
                log(f"Added missing type: {name}")
        
        # Add any user-requested missing types
        for type_name in self.missing_types:
            if type_name not in self.extracted_symbols.get('typedefs', {}):
                self.extracted_symbols['typedefs'][type_name] = 'int'
                log(f"Added user-requested type: {type_name}")
    
    def generate_files(self):
        # Generate all XINU simulation files.
        # Use g++ preprocessing to extract symbols
        if not self.preprocess_headers():
            # Fall back to template-based generation if preprocessing fails
            log("WARNING: Falling back to template-based file generation")
            self._print("WARNING: Falling back to template-based file generation")
        else:
            # Extract symbols from preprocessed content
            self.extract_symbols()
        
        self._print("\n##### Generating XINU Simulation Files #####")
        self.generate_stddefs()
        self.generate_xinu_h()
        self.generate_includes_h()
        self.generate_sim_declarations()
        self.generate_sim_helper()
        self.generate_xinu_core()
    
    def generate_stddefs(self):
        # Generate xinu_stddefs.h with type definitions.
        # Generate content from extracted symbols if available
        if self.extracted_symbols:
            content = self._generate_stddefs_from_symbols()
        else:
            # Fall back to template rendering
            content = self._render_template("XINU_STDDEFS_H")
        
        # Make sure output directory exists
        os.makedirs(os.path.dirname(self.config.stddefs_h), exist_ok=True)
        
        with open(self.config.stddefs_h, 'w') as f:
            f.write(content)
        
        log(f"Generated XINU stddefs: {self.config.stddefs_h}")
        self._print(f"Generated XINU stddefs: {self.config.stddefs_h}")
    
    def _generate_stddefs_from_symbols(self):
        # Generate stddefs content from extracted symbols.
        # Build content from extracted symbols
        timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        username = os.environ.get("USER", os.environ.get("USERNAME", "unknown"))
        
        content = f"/* xinu_stddefs.h - Generated by g++ preprocessing */\n"
        content += f"/* Generated on: {timestamp} */\n"
        content += f"/* By user: {username} */\n"
        content += "#ifndef _XINU_STDDEFS_H_\n"
        content += "#define _XINU_STDDEFS_H_\n\n"
        
        # Add typedefs
        content += "/* Basic type definitions */\n"
        for name, type_def in self.extracted_symbols.get('typedefs', {}).items():
            content += f"typedef {type_def} {name};\n"
        
        # Add defines
        content += "\n/* Constant definitions */\n"
        for name, value in self.extracted_symbols.get('defines', {}).items():
            content += f"#define {name} {value}\n"
        
        content += "\n#endif /* _XINU_STDDEFS_H_ */\n"
        return content
    
    def generate_xinu_h(self):
        # Generate xinu.h with basic XINU declarations
        if self.extracted_symbols and 'structs' in self.extracted_symbols:
            content = self._generate_xinu_h_from_symbols()
        else:
            content = self._render_template("XINU_H")
            
        with open(self.config.xinu_h, 'w') as f:
            f.write(content)
            
        log(f"Generated XINU header: {self.config.xinu_h}")
        self._print(f"Generated XINU header: {self.config.xinu_h}")
    
    def _generate_xinu_h_from_symbols(self):
        # Generate xinu.h content from extracted symbols
        timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        username = os.environ.get("USER", os.environ.get("USERNAME", "unknown"))
        
        content = f"/* xinu.h - Generated from XINU headers */\n"
        content += f"/* Generated on: {timestamp} */\n"
        content += f"/* By user: {username} */\n"
        content += "#ifndef _XINU_H_\n"
        content += "#define _XINU_H_\n\n"
        content += "#include \"xinu_stddefs.h\"\n\n"
        
        # Add key structure definitions
        content += "/* Structure definitions */\n"
        if 'procent' in self.extracted_symbols['structs']:
            content += f"struct procent {{\n{self.extracted_symbols['structs']['procent']}\n}};\n\n"
        else:
            # Add a default procent structure
            content += """struct procent {
    void    *prstkptr;      /* Saved stack pointer                */
    int     prio;           /* Process priority                   */
    int     prstate;        /* Process state: READY or SUSPENDED  */
    char    prname[16];     /* Process name                       */
};\n\n"""
        
        # Add more structures as needed
        
        content += "#endif /* _XINU_H_ */\n"
        return content
    
    def generate_includes_h(self):
        # Generate includes.h with all necessary includes
        dynamic_includes = self._generate_dynamic_includes()
        context = {'dynamic_includes': dynamic_includes}
        
        content = self._render_template("XINU_INCLUDES_H", context)
        
        with open(self.config.includes_h, 'w') as f:
            f.write(content)
            
        log(f"Generated XINU includes: {self.config.includes_h}")
        self._print(f"Generated XINU includes: {self.config.includes_h}")
    
    def _generate_dynamic_includes(self):
        # Generate include statements for headers.
        include_statements = ""
        if hasattr(self.config, 'include_dir') and os.path.exists(self.config.include_dir):
            for filename in os.listdir(self.config.include_dir):
                if filename.endswith(".h") and filename != "xinu_stddefs.h":
                    include_statements += f"#include \"{filename}\"\n"
        return include_statements
    
    def generate_sim_declarations(self):
        # Generate simulation declarations header
        content = self._render_template("XINU_SIM_DECLARATIONS_H")
        
        with open(self.config.sim_decls_h, 'w') as f:
            f.write(content)
            
        log(f"Generated simulation declarations: {self.config.sim_decls_h}")
        self._print(f"Generated simulation declarations: {self.config.sim_decls_h}")
    
    def generate_sim_helper(self):
        # Generate simulation helper C file
        content = self._render_template("XINU_SIMULATION_C")
        
        with open(self.config.sim_helper_c, 'w') as f:
            f.write(content)
            
        log(f"Generated simulation helper: {self.config.sim_helper_c}")
        self._print(f"Generated simulation helper: {self.config.sim_helper_c}")
    
    def generate_xinu_core(self):
        # Generate XINU core C file
        context = {'extracted_functions': self._get_extracted_function_list()}
        content = self._render_template("XINU_CORE_C", context)
        
        with open(self.config.xinu_core_c, 'w') as f:
            f.write(content)
            
        log(f"Generated XINU core: {self.config.xinu_core_c}")
        self._print(f"Generated XINU core: {self.config.xinu_core_c}")
    
    def _get_extracted_function_list(self):
        # Get list of extracted functions for template rendering
        if not self.extracted_symbols or 'functions' not in self.extracted_symbols:
            return "/* No functions extracted */"
            
        result = "/* Extracted function prototypes */\n"
        for name, func_info in self.extracted_symbols['functions'].items():
            result += f"{func_info['return_type']} {name}({func_info['params']});\n"
            
        return result
    
    def _render_template(self, template_name, context=None):
        # Render a template with variable replacements.
        if context is None:
            context = {}
            
        if template_name not in self.templates:
            log(f"ERROR: Template '{template_name}' not found")
            self._print(f"ERROR: Template '{template_name}' not found")
            return f"/* Error: Template {template_name} not found */"
            
        template = self.templates[template_name]
        
        # Get current timestamp and username from system
        timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        full_context = {
            "generator": "xinu_generator.py",
            "timestamp": timestamp,
            "user": os.environ.get("USER", os.environ.get("USERNAME", "unknown"))
        }
        full_context.update(context)
        
        result = template
        for key, value in full_context.items():
            result = result.replace(f"{{{{ {key} }}}}", str(value))
            
        return result