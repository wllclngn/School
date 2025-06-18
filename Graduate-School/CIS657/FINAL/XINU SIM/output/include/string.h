#ifndef _STRING_H_
#define _STRING_H_
#include <stddef.h>

#include <xinu.h>

/* string.h */

extern	char	*strncpy(char *, const char *, int32);
extern	char	*strncat(char *, const char *, int32);
extern	int32	strncmp(const char *, const char *, int32);
extern	char	*strchr(const char *, int32);
extern	char	*strrchr(const char *, int32);
extern	char	*strstr(const char *, const char *);
extern	int32	strnlen(const char *, uint32);

#endif /* _STRING_H_ */
