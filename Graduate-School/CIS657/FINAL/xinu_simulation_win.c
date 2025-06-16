/* xinu_simulation_win.c - Helper functions for Windows Simulation
 * Generated on: 2025-06-16 06:19:36 UTC by mol
 * Version: 5.30.9
 */
#include "xinu_sim_declarations.h" 
#include <stdio.h>   
#include <stdlib.h>  
#include <string.h>  
#include <stdarg.h>  
#include <ctype.h>   
#include <time.h>    
// #include <windows.h> 

// Stdio implementations... (same as 5.30.8)
int xinu_printf_sim_redirect(const char *format, ...) {
    va_list args; int ret; va_start(args, format); ret = vprintf(format, args); va_end(args); fflush(stdout); return ret;
}
int xinu_fprintf_sim_redirect(void *stream, const char *format, ...) {
    va_list args; int ret; va_start(args, format); ret = vfprintf((FILE*)stream, format, args); va_end(args); if (stream) fflush((FILE*)stream); return ret;
}
int xinu_sprintf_sim_redirect(char *buffer, const char *format, ...) {
    va_list args; int ret; va_start(args, format); ret = vsprintf(buffer, format, args); va_end(args); return ret;
}
int xinu_scanf_sim_redirect(const char *format, ...) {
    va_list args; int ret; va_start(args, format); ret = vscanf(format, args); va_end(args); return ret;
}
int xinu_fscanf_sim_redirect(void *stream, const char *format, ...) {
    va_list args; int ret; va_start(args, format); ret = vfscanf((FILE*)stream, format, args); va_end(args); return ret;
}
int xinu_sscanf_sim_redirect(const char *buffer, const char *format, ...) {
    va_list args; int ret; va_start(args, format); ret = vsscanf(buffer, format, args); va_end(args); return ret;
}
int xinu_getchar_sim_redirect(void) { return getchar(); }
int xinu_putchar_sim_redirect(int c) { return putchar(c); }
int xinu_fgetc_sim_redirect(void *stream) { return fgetc((FILE*)stream); }
char* xinu_fgets_sim_redirect(char *str, int num, void *stream) { return fgets(str, num, (FILE*)stream); }
int xinu_fputc_sim_redirect(int c, void *stream) { return fputc(c, (FILE*)stream); }
int xinu_fputs_sim_redirect(const char *str, void *stream) { return fputs(str, (FILE*)stream); }

int xinu_doprnt_sim_redirect(char *fmt, va_list ap, xinu_putc_func_t putc_func, int putc_arg) {
    char buffer[4096]; 
    int ret = vsprintf(buffer, fmt, ap);
    if (putc_func) { for (int i = 0; i < ret; ++i) { putc_func(buffer[i], putc_arg); } }
    return ret;
}
int xinu_doscan_sim_redirect(char *fmt, va_list ap, xinu_getc_func_t getc_func, xinu_ungetc_func_t ungetc_func, int getc_arg, int ungetc_arg) {
    if (fmt) {} if (ap) {} if (getc_func) {} if (ungetc_func) {} if (getc_arg) {} if (ungetc_arg) {}
    return 0; 
}
// Stdlib implementations... (same as 5.30.8)
int xinu_abs_sim_redirect(int n) { return abs(n); }
long xinu_labs_sim_redirect(long n) { return labs(n); }
int xinu_atoi_sim_redirect(const char *str) { return atoi(str); }
long xinu_atol_sim_redirect(const char *str) { return atol(str); }
int xinu_rand_sim_redirect(void) { return rand(); }
void xinu_srand_sim_redirect(unsigned int seed) { srand(seed); }
void xinu_qsort_sim_redirect(void *base, size_t num, size_t size, xinu_qsort_cmp_t compare) {
    qsort(base, num, size, compare);
}
// String implementations... (same as 5.30.8)
char* xinu_strcpy_sim_redirect(char *dest, const char *src) { return strcpy(dest, src); }
char* xinu_strncpy_sim_redirect(char *dest, const char *src, size_t n) { return strncpy(dest, src, n); }
char* xinu_strcat_sim_redirect(char *dest, const char *src) { return strcat(dest, src); }
char* xinu_strncat_sim_redirect(char *dest, const char *src, size_t n) { return strncat(dest, src, n); }
int xinu_strcmp_sim_redirect(const char *s1, const char *s2) { return strcmp(s1, s2); }
int xinu_strncmp_sim_redirect(const char *s1, const char *s2, size_t n) { return strncmp(s1, s2, n); }
size_t xinu_strlen_sim_redirect(const char *s) { return strlen(s); }
size_t xinu_strnlen_sim_redirect(const char *s, size_t maxlen) { 
#if defined(_WIN32) && defined(__STDC_LIB_EXT1__) && __STDC_WANT_LIB_EXT1__ && defined(strnlen_s)
    return strnlen_s(s, maxlen); 
#else 
    size_t i = 0; while (i < maxlen && s[i]) { ++i; } return i;
#endif
}
char* xinu_strchr_sim_redirect(const char *s, int c) { return (char*)strchr(s, c); } 
char* xinu_strrchr_sim_redirect(const char *s, int c) { return (char*)strrchr(s, c); } 
char* xinu_strstr_sim_redirect(const char *haystack, const char *needle) { return (char*)strstr(haystack, needle); } 
void* xinu_memcpy_sim_redirect(void *dest, const void *src, size_t n) { return memcpy(dest, src, n); }
void* xinu_memmove_sim_redirect(void *dest, const void *src, size_t n) { return memmove(dest, src, n); }
int xinu_memcmp_sim_redirect(const void *s1, const void *s2, size_t n) { return memcmp(s1, s2, n); }
void* xinu_memset_sim_redirect(void *s, int c, size_t n) { return memset(s, c, n); }

// Simulation specific helpers
void xinu_simulation_yield(void) { }
void xinu_trigger_clock_interrupt(void) { }
