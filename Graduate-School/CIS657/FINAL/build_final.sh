#!/bin/bash
# build_final.sh - Build the final solution with fixes

# Ensure we have the sscanf fix header
mkdir -p include
cat > include/sscanf_fix.h << 'EOF'
/* sscanf_fix.h - Fix for conflicting sscanf definitions */
#ifndef _SSCANF_FIX_H_
#define _SSCANF_FIX_H_

/* 
 * This file is included by compile.sh after the standard headers
 * to override problematic function signatures.
 */

/* Undefine the problematic macro */
#ifdef sscanf
#undef sscanf
#endif

/* Define our own version that will match what your XINU system expects */
#define sscanf xinu_sscanf_sim_redirect

#endif /* _SSCANF_FIX_H_ */
EOF

# Run the go.sh script to generate the base files
./go.sh --clean

# Apply fixes to generated files
sed -i 's/int xinu_sscanf_sim_redirect(const char \*buffer, const char \*format, ...);/int32 xinu_sscanf_sim_redirect(char \*buffer, char \*format, int32 args);/' xinu_sim_declarations.h
sed -i 's/int xinu_sscanf_sim_redirect(const char \*buffer, const char \*format, ...) {/int32 xinu_sscanf_sim_redirect(char \*buffer, char \*format, int32 args) {/' xinu_simulation.c
sed -i 's/va_list args; int ret; va_start(args, format); ret = vsscanf(buffer, format, args); va_end(args); return ret;/return sscanf(buffer, format, args);/' xinu_simulation.c

# Now build again with the fixes
gcc -c xinu_core.c -I. -I./include -o sim_output/obj/xinu_core.o
gcc -c xinu_simulation.c -I. -I./include -o sim_output/obj/xinu_simulation.o
gcc -c system/pstarv.c -I. -I./include -o sim_output/obj/pstarv.o
gcc -c system/resched.c -I. -I./include -o sim_output/obj/resched.o

# Link everything
gcc -o sim_output/xinu_core sim_output/obj/*.o 

echo "Build completed!"
