/* xinu_sim_declarations.h - Declarations for XINU simulation shim functions.
 * Generated on: 2025-06-16 06:19:36 UTC by mol
 * Version: 5.30.9
 */
#ifndef _XINU_SIM_DECLARATIONS_H_
#define _XINU_SIM_DECLARATIONS_H_

#include <stdarg.h> 
#include <stddef.h> 

// Stdio
int xinu_printf_sim_redirect(const char *format, ...);
int xinu_fprintf_sim_redirect(void *stream, const char *format, ...); 
int xinu_sprintf_sim_redirect(char *buffer, const char *format, ...);
int xinu_scanf_sim_redirect(const char *format, ...);
int xinu_fscanf_sim_redirect(void *stream, const char *format, ...); 
int xinu_sscanf_sim_redirect(const char *buffer, const char *format, ...);
int xinu_getchar_sim_redirect(void);
int xinu_putchar_sim_redirect(int c);
int xinu_fgetc_sim_redirect(void *stream); 
char* xinu_fgets_sim_redirect(char *str, int num, void *stream); 
int xinu_fputc_sim_redirect(int c, void *stream); 
int xinu_fputs_sim_redirect(const char *str, void *stream); 

typedef int ((*xinu_putc_func_t)(int, int)); 
typedef int ((*xinu_getc_func_t)(int));      
typedef int ((*xinu_ungetc_func_t)(int,int)); 

int xinu_doprnt_sim_redirect(char *fmt, va_list ap, xinu_putc_func_t putc_func, int putc_arg);
int xinu_doscan_sim_redirect(char *fmt, va_list ap, xinu_getc_func_t getc_func, xinu_ungetc_func_t ungetc_func, int getc_arg, int ungetc_arg);

// Stdlib
int xinu_abs_sim_redirect(int n);
long xinu_labs_sim_redirect(long n);
int xinu_atoi_sim_redirect(const char *str);
long xinu_atol_sim_redirect(const char *str);
int xinu_rand_sim_redirect(void);
void xinu_srand_sim_redirect(unsigned int seed);
typedef int (*xinu_qsort_cmp_t)(const void *, const void *);
void xinu_qsort_sim_redirect(void *base, size_t num, size_t size, xinu_qsort_cmp_t compare);

// String
char* xinu_strcpy_sim_redirect(char *dest, const char *src);
char* xinu_strncpy_sim_redirect(char *dest, const char *src, size_t n);
char* xinu_strcat_sim_redirect(char *dest, const char *src);
char* xinu_strncat_sim_redirect(char *dest, const char *src, size_t n);
int xinu_strcmp_sim_redirect(const char *s1, const char *s2);
int xinu_strncmp_sim_redirect(const char *s1, const char *s2, size_t n);
size_t xinu_strlen_sim_redirect(const char *s);
size_t xinu_strnlen_sim_redirect(const char *s, size_t maxlen);
char* xinu_strchr_sim_redirect(const char *s, int c);
char* xinu_strrchr_sim_redirect(const char *s, int c);
char* xinu_strstr_sim_redirect(const char *haystack, const char *needle);
void* xinu_memcpy_sim_redirect(void *dest, const void *src, size_t n);
void* xinu_memmove_sim_redirect(void *dest, const void *src, size_t n);
int xinu_memcmp_sim_redirect(const void *s1, const void *s2, size_t n);
void* xinu_memset_sim_redirect(void *s, int c, size_t n);

// Simulation specific helpers
void xinu_simulation_yield(void);
void xinu_trigger_clock_interrupt(void);

#endif /* _XINU_SIM_DECLARATIONS_H_ */
