typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int  uint32;
typedef unsigned long uint64;

typedef uint64 pde_t;

typedef long time_t;

#if defined(_STDIO_H)
#define MKFS
#endif

#if !defined(MKFS)

#define NULL ((void*)0)

typedef char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long int64_t;
typedef unsigned long uint64_t;

typedef int64_t ssize_t;
typedef uint64_t size_t;

typedef int64_t intptr_t;
typedef uint64_t uintptr_t;

#endif
