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
