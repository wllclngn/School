# ISSUES.md

## Compilation Issues

### Summary
The following issues were encountered during the compilation of the XINU simulation project. These issues need to be addressed to ensure a successful build.

---

### Errors

#### 1. Missing Files
- `xinu_stddefs.h`: This file is missing, causing a fatal error during preprocessing. It was expected to be included in the project root.

#### 2. Template Placeholders Not Replaced
- Several placeholders like `{{ basic_types }}`, `{{ process_states }}`, and `{{ custom_initialization }}` were not replaced in the generated files, indicating an issue with the template rendering or data injection process.

#### 3. Conflicting Type Definitions
- `int8` was defined in multiple locations (`kernel.h` and `xinu.h`), leading to conflicting type declarations.

#### 4. Undefined Identifiers
- `count1000`, `custom_initialization`, and `test_execution_switch` were not declared or defined in the codebase but were referenced in files like `xinu_core.c`.

#### 5. Missing Includes
- Standard library functions like `strcpy` were used without including `<string.h>`. This caused warnings and potential errors.

#### 6. Struct Field Mismatches
- The `struct procent` definition was missing expected fields such as `prio`, causing errors when attempting to access these fields.

---

### Warnings

#### 1. Redefinition of `NULL`
- The macro `NULL` was redefined in `kernel.h`, conflicting with the standard definition in `<stddef.h>`.

#### 2. Implicit Declarations
- Functions like `getenv` and `strcpy` were implicitly declared, which could lead to undefined behavior.

#### 3. Parameter Visibility
- Parameters like `struct ttycblk` and `struct uart_csreg` in `prototypes.h` will not be visible outside their definition scope.

---

### Steps to Resolve

1. **Fix Missing Files**
   - Ensure `xinu_stddefs.h` is generated or included properly in the project root.

2. **Template Rendering**
   - Investigate the template system to ensure placeholders are properly replaced with actual content during file generation.

3. **Resolve Conflicts**
   - Remove duplicate type definitions and standardize typedefs like `int8`.

4. **Define Missing Identifiers**
   - Add proper declarations or definitions for `count1000`, `custom_initialization`, and `test_execution_switch`.

5. **Include Headers**
   - Add missing includes like `<string.h>` for standard library functions.

6. **Update Struct Definitions**
   - Ensure `struct procent` and similar structs have all the required fields.

7. **Address Warnings**
   - Avoid redefining standard macros like `NULL`.
   - Ensure all functions and parameters are properly declared and defined.

8. **Improve Logging**
   - Capture and log detailed error messages to aid debugging.

---
