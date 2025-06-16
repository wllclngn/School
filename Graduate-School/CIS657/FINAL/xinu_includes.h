/* xinu_includes.h - Wrapper for XINU code compilation.
 * Generated on: 2025-06-16 06:19:36 UTC by mol
 * Version: 5.30.9 (Shims first, then xinu.h. NO HOST HEADERS HERE)
 */
#ifndef _XINU_INCLUDES_H_ 
#define _XINU_INCLUDES_H_

#define _CRT_SECURE_NO_WARNINGS 
#define XINU_SIMULATION        
#ifndef _MSC_VER
  #define _MSC_VER 1930 
#endif

/* --- STAGE 1: AGGRESSIVE SHIMS FIRST --- */
#ifdef getchar 
#undef getchar
#endif
#define getchar() xinu_getchar_sim_redirect()
#ifdef putchar
#undef putchar
#endif
#define putchar(c) xinu_putchar_sim_redirect(c)

#define printf xinu_printf_sim_redirect
#define fprintf xinu_fprintf_sim_redirect
#define sprintf xinu_sprintf_sim_redirect
#define scanf xinu_scanf_sim_redirect
#define fscanf xinu_fscanf_sim_redirect
#define sscanf xinu_sscanf_sim_redirect
#define fgetc xinu_fgetc_sim_redirect
#define fgets xinu_fgets_sim_redirect
#define fputc xinu_fputc_sim_redirect
#define fputs xinu_fputs_sim_redirect

#define _doprnt xinu_doprnt_sim_redirect 
#define _doscan xinu_doscan_sim_redirect 

#ifdef abs
#undef abs
#endif
#define abs(n) xinu_abs_sim_redirect(n)
#ifdef labs
#undef labs
#endif
#define labs(n) xinu_labs_sim_redirect(n)

#define atoi(s) xinu_atoi_sim_redirect(s)
#define atol(s) xinu_atol_sim_redirect(s)

#ifdef rand
#undef rand
#endif
#define rand() xinu_rand_sim_redirect()
#ifdef srand
#undef srand
#endif
#define srand(s) xinu_srand_sim_redirect(s)

#ifdef qsort
#undef qsort
#endif
#define qsort(b,n,s,c) xinu_qsort_sim_redirect(b,n,s,c)

#define strcpy xinu_strcpy_sim_redirect
#define strncpy xinu_strncpy_sim_redirect
#define strcat xinu_strcat_sim_redirect
#define strncat xinu_strncat_sim_redirect
#define strcmp xinu_strcmp_sim_redirect
#define strncmp xinu_strncmp_sim_redirect
#define strlen xinu_strlen_sim_redirect
#define strnlen xinu_strnlen_sim_redirect 
#define strchr xinu_strchr_sim_redirect
#define strrchr xinu_strrchr_sim_redirect
#define strstr xinu_strstr_sim_redirect
#define memcpy xinu_memcpy_sim_redirect
#define memmove xinu_memmove_sim_redirect
#define memcmp xinu_memcmp_sim_redirect
#define memset xinu_memset_sim_redirect 

/* --- STAGE 2: Full XINU Environment --- */
#include "xinu.h" 

/* NO HOST SYSTEM HEADERS IN THIS FILE. */
#endif /* _XINU_INCLUDES_H_ */
