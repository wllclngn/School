# makefile_parser.py - Parse XINU Makefile for build instructions
import os
import re
from xinu_sim.utils.logger import log

class MakefileParser:
    """Parser for XINU Makefile to extract build instructions and dependencies"""
    
    def __init__(self, config):
        self.config = config
        self.makefile_path = os.path.join(self.config.compile_dir, "Makefile")
        self.sources = []
        self.include_dirs = []
        self.cflags = []
        self.ldflags = []
        self.definitions = {}
        
    def parse_makefile(self):
        """Parse the XINU Makefile to extract build information"""
        if not os.path.exists(self.makefile_path):
            log(f"Warning: Makefile not found at {self.makefile_path}")
            return False
            
        log(f"Parsing Makefile: {self.makefile_path}")
        
        try:
            with open(self.makefile_path, 'r') as f:
                content = f.read()
                
            # Extract variable definitions
            self._extract_variables(content)
            
            # Extract source files
            self._extract_sources(content)
            
            # Extract compiler flags
            self._extract_compiler_flags(content)
            
            # Extract include directories
            self._extract_include_dirs(content)
            
            # Expand variables in extracted values
            self._expand_variables()
            
            log(f"Successfully parsed Makefile: found {len(self.sources)} source files and {len(self.include_dirs)} include directories")
            return True
        except Exception as e:
            log(f"Error parsing Makefile: {str(e)}")
            return False
    
    def _extract_variables(self, content):
        """Extract variable definitions from Makefile"""
        # Match variable definitions (e.g., NAME = value)
        pattern = r'^\s*([A-Za-z0-9_]+)\s*=\s*(.*)$'
        for line in content.split('\n'):
            match = re.match(pattern, line)
            if match:
                name = match.group(1)
                value = match.group(2).strip()
                self.definitions[name] = value
                log(f"Found definition: {name} = {value}", verbose_only=True)
    
    def _extract_sources(self, content):
        """Extract source file definitions from Makefile"""
        # Look for source file lists (e.g., SRC_FILES = file1.c file2.c)
        source_patterns = [
            r'^\s*SRC\s*=\s*(.*)$',
            r'^\s*C_FILES\s*=\s*(.*)$',
            r'^\s*SYSTEM_SRC\s*=\s*(.*)$',
            r'^\s*LIB_SRC\s*=\s*(.*)$'
        ]
        
        # Track all source file declarations
        all_sources = []
        
        for pattern in source_patterns:
            for line in content.split('\n'):
                match = re.search(pattern, line)
                if match:
                    sources = match.group(1).strip().split()
                    all_sources.extend(sources)
                    log(f"Found source files: {sources}", verbose_only=True)
        
        # Also look for .c files in explicit compilation rules
        compile_pattern = r'^\s*[\w/\.\$\{\}]+\.o\s*:\s*([\w/\.\$\{\}]+\.c)'
        for line in content.split('\n'):
            match = re.search(compile_pattern, line)
            if match:
                source = match.group(1).strip()
                if source not in all_sources:
                    all_sources.append(source)
                    log(f"Found compiled source: {source}", verbose_only=True)
                    
        self.sources = all_sources
    
    def _extract_compiler_flags(self, content):
        """Extract compiler flags from Makefile"""
        # Look for CFLAGS definition
        cflags_pattern = r'^\s*CFLAGS\s*=\s*(.*)$'
        for line in content.split('\n'):
            match = re.search(cflags_pattern, line)
            if match:
                flags = match.group(1).strip().split()
                self.cflags.extend(flags)
                log(f"Found CFLAGS: {flags}", verbose_only=True)
        
        # Look for LDFLAGS definition
        ldflags_pattern = r'^\s*LDFLAGS\s*=\s*(.*)$'
        for line in content.split('\n'):
            match = re.search(ldflags_pattern, line)
            if match:
                flags = match.group(1).strip().split()
                self.ldflags.extend(flags)
                log(f"Found LDFLAGS: {flags}", verbose_only=True)
    
    def _extract_include_dirs(self, content):
        """Extract include directories from Makefile"""
        # Look for -I flags in CFLAGS
        include_pattern = r'-I\s*([^\s]+)'
        for flag in self.cflags:
            match = re.search(include_pattern, flag)
            if match:
                include_dir = match.group(1)
                if include_dir not in self.include_dirs:
                    self.include_dirs.append(include_dir)
                    log(f"Found include directory: {include_dir}", verbose_only=True)
        
        # Also look for INCLUDE definitions
        include_def_pattern = r'^\s*INCLUDE\s*=\s*(.*)$'
        for line in content.split('\n'):
            match = re.search(include_def_pattern, line)
            if match:
                dirs = match.group(1).strip().split()
                for dir in dirs:
                    # Check if it's a path or flag
                    if dir.startswith('-I'):
                        include_dir = dir[2:]
                    else:
                        include_dir = dir
                    
                    if include_dir not in self.include_dirs:
                        self.include_dirs.append(include_dir)
                        log(f"Found include directory from INCLUDE: {include_dir}", verbose_only=True)
    
    def _expand_variables(self):
        """Expand Makefile variables in extracted values"""
        # First expand all definitions recursively
        expanded_defs = {}
        for name, value in self.definitions.items():
            expanded_defs[name] = self._expand_value(value, expanded_defs)
        
        # Then update our definitions
        self.definitions = expanded_defs
        
        # Now expand variables in sources, includes, and flags
        self.sources = [self._expand_value(s, self.definitions) for s in self.sources]
        self.include_dirs = [self._expand_value(d, self.definitions) for d in self.include_dirs]
        self.cflags = [self._expand_value(f, self.definitions) for f in self.cflags]
        self.ldflags = [self._expand_value(f, self.definitions) for f in self.ldflags]
    
    def _expand_value(self, value, definitions, max_depth=10):
        """Recursively expand variables in a value"""
        if max_depth <= 0:
            log(f"Warning: Maximum expansion depth reached for '{value}'", verbose_only=True)
            return value
            
        # Match $(VAR) or ${VAR} style variables
        var_pattern = r'\$[\(\{]([A-Za-z0-9_]+)[\)\}]'
        
        # Find all variables in the value
        matches = re.findall(var_pattern, value)
        
        # If no variables, return as is
        if not matches:
            return value
            
        # Replace each variable with its definition
        result = value
        for var in matches:
            if var in definitions:
                var_value = self._expand_value(definitions[var], definitions, max_depth - 1)
                result = re.sub(r'\$[\(\{]' + var + r'[\)\}]', var_value, result)
            else:
                # If variable not defined, keep it as is
                pass
                
        return result
    
    def get_resolved_source_files(self):
        """Get list of source files with resolved paths"""
        resolved_sources = []
        
        for src in self.sources:
            # Handle absolute paths
            if os.path.isabs(src):
                resolved_src = src
            else:
                # Check multiple locations relative to compile dir
                candidates = [
                    os.path.join(self.config.compile_dir, src),
                    os.path.join(self.config.xinu_os_dir, src),
                    os.path.join(os.path.dirname(self.config.compile_dir), src)
                ]
                
                resolved_src = None
                for candidate in candidates:
                    if os.path.exists(candidate):
                        resolved_src = candidate
                        break
                
                # If still not found, use the compile dir path as fallback
                if not resolved_src:
                    resolved_src = os.path.join(self.config.compile_dir, src)
            
            # Only add C source files
            if resolved_src.endswith('.c') and resolved_src not in resolved_sources:
                resolved_sources.append(resolved_src)
                
        return resolved_sources
    
    def get_resolved_include_dirs(self):
        """Get list of include directories with resolved paths"""
        resolved_includes = []
        
        # Always include the standard includes
        standard_includes = [
            self.config.include_dir,
            self.config.output_dir,
            os.path.dirname(self.config.xinu_h)
        ]
        
        for inc in standard_includes:
            if os.path.exists(inc) and inc not in resolved_includes:
                resolved_includes.append(inc)
        
        # Add Makefile-defined includes
        for inc in self.include_dirs:
            # Handle absolute paths
            if os.path.isabs(inc):
                resolved_inc = inc
            else:
                # Check multiple locations relative to compile dir
                candidates = [
                    os.path.join(self.config.compile_dir, inc),
                    os.path.join(self.config.xinu_os_dir, inc),
                    os.path.join(os.path.dirname(self.config.compile_dir), inc)
                ]
                
                resolved_inc = None
                for candidate in candidates:
                    if os.path.exists(candidate):
                        resolved_inc = candidate
                        break
                
                # If still not found, use the compile dir path as fallback
                if not resolved_inc:
                    resolved_inc = os.path.join(self.config.compile_dir, inc)
            
            # Add if it exists and not already in the list
            if os.path.exists(resolved_inc) and resolved_inc not in resolved_includes:
                resolved_includes.append(resolved_inc)
                
        return resolved_includes
    
    def get_compiler_flags(self):
        """Get compiler flags from Makefile suitable for cross-platform use"""
        safe_flags = []
        
        # Filter flags to remove platform-specific or unsafe options
        for flag in self.cflags:
            # Skip include flags (we handle them separately)
            if flag.startswith('-I'):
                continue
                
            # Skip unsafe flags for cross-platform builds
            unsafe_prefixes = ['-m', '-f', '-W', '--target=']
            if any(flag.startswith(prefix) for prefix in unsafe_prefixes):
                continue
                
            safe_flags.append(flag)
            
        # Add our own common flags for cross-platform builds
        common_flags = [
            "-Wall",
            "-g",
            "-O0",
            "-fno-builtin"  # Important for XINU as it redefines standard functions
        ]
        
        for flag in common_flags:
            if flag not in safe_flags:
                safe_flags.append(flag)
                
        return safe_flags
        
    def get_linker_flags(self):
        """Get linker flags from Makefile suitable for cross-platform use"""
        safe_flags = []
        
        # Filter flags to remove platform-specific options
        for flag in self.ldflags:
            # Skip unsafe flags for cross-platform linking
            unsafe_prefixes = ['-m', '-z', '--target=']
            if any(flag.startswith(prefix) for prefix in unsafe_prefixes):
                continue
                
            safe_flags.append(flag)
            
        # Always include the math library
        if '-lm' not in safe_flags:
            safe_flags.append('-lm')
            
        return safe_flags