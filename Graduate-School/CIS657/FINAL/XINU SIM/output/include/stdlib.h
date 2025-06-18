#ifndef _STDLIB_H_
#define _STDLIB_H_

/**
 * @file stdlib.h
 * Standard library function declarations for XINU.
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

/*
 * This header declares standard library functions.
 * It relies on types like 'int32', 'uint32', 'size_t' being defined
 * BEFORE this file is included. This is typically achieved by including
 * 'xinu.h' (which should handle system includes like <stdint.h> and <stddef.h>)
 * in the .c file prior to including this 'stdlib.h'.
 *
 * DO NOT #include <xinu.h> in this file.
 */

// If any function signatures here *directly* use size_t, and you want this
// header to be potentially usable without xinu.h already providing size_t,
// you could include <stddef.h> here. It's generally better practice
// to ensure xinu.h (which includes <stddef.h>) is processed first by the .c file.
// #include <stddef.h> // For size_t, if needed directly by declarations below

int abs(int n);

// Standard C uses 'long int' for labs.
// If XINU specifically uses 'int32' for labs, the type 'int32' must be defined
// (via previous include of xinu.h) before this line is processed.
long int labs(long int n);
/* Example if XINU defines labs using its own int32 type:
 * // int32 labs(int32 n);
 */

int atoi(char *s);

// Similar to labs regarding 'long int' vs 'int32'.
long int atol(char *s);
/* Example if XINU defines atol using its own int32 type:
 * // int32 atol(int32 s); // Note: Original Xinu atol often takes char*, returns int32
 */

// bzero is technically from <strings.h> (POSIX) or <string.h> (as an extension).
// Its second argument is typically size_t. Using 'unsigned int' is an older practice.
void bzero(void *s, unsigned int n);
/* More standard version (requires size_t to be known, e.g. from <stddef.h>):
 * // void bzero(void *s, size_t n);
 */

// For qsort, parameter types can vary slightly by standard/implementation.
// Using 'unsigned int' for counts/sizes is common in older C.
// 'size_t' is used in C89 and later for 'num' and 'size'.
// The comparison function signature is 'int (*compar)(const void *, const void *)'.
void qsort(void *base, unsigned int num, unsigned int size, int (*compar)(const void *, const void *));
/* More standard version (requires size_t to be known):
 * // void qsort(void *base, size_t num, size_t size, int (*compar)(const void *, const void *));
 */

// These should use XINU's standard integer types if defined (e.g., uint32 from xinu.h),
// or rely on 'unsigned int' being sufficient.
unsigned int rand(void);
/* Example if using XINU's uint32 type:
 * // uint32 rand(void);
 */

void srand(unsigned int seed);

// 'nbytes' is often 'size_t' in modern C.
void *malloc(unsigned int nbytes);
/* More standard version (requires size_t to be known):
 * // void *malloc(size_t nbytes);
 */

void free(void *pmem);

#endif /* _STDLIB_H_ */
