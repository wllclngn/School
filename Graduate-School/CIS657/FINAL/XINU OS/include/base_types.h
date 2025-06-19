/* base_types.h - Standard type definitions for XINU OS
 * Generated on: 2025-06-19 11:39:06
 */
#ifndef _BASE_TYPES_H_
#define _BASE_TYPES_H_

/* Basic type definitions */
typedef char                int8;
typedef short               int16;
typedef int                 int32;
typedef long                int64;
typedef unsigned char       uint8;
typedef unsigned short      uint16;
typedef unsigned int        uint32;
typedef unsigned long long  uint64;

/* XINU process and device types */
typedef int32               pid32;
typedef int16               pri16;
typedef int16               qid16;     /* Queue ID type - must match kernel.h */
typedef int32               sid32;
typedef int32               did32;
typedef int32               ibid32;
typedef int32               dbid32;

#endif /* _BASE_TYPES_H_ */
