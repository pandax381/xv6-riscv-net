#ifndef STD_H
#define STD_H

#include <stdarg.h>

#include "time.h"

#define UINT16_MAX 65535

#define isascii(x) ((x >= 0x00) && (x <= 0x7f))
#define isprint(x) ((x >= 0x20) && (x <= 0x7e))

#define EINTR 1

extern int errno;

/*
 * STDIO
 */

typedef struct {
    /* dummy */
} FILE;

extern FILE *stderr;

#define fprintf(fp, ...) printf(__VA_ARGS__)
#define vfprintf(fp, ...) vcprintf(__VA_ARGS__)

extern void
flockfile(FILE *fp);
extern void
funlockfile(FILE *fp);
extern int
vfprintf(FILE *fp, const char *fmt, va_list ap);

/*
 * Time
 */

struct timespec {
    /* dummy */
};

extern size_t
strftime(char *s, size_t max, const char *format, const struct tm *tm);
extern void
timersub(struct timeval *a, struct timeval *b, struct timeval *res);
extern void
timerclear(struct timeval *tv);

#define timercmp(a, b, cmp) \
    ((a)->tv_sec == (b)->tv_sec ? (a)->tv_usec cmp (b)->tv_usec : (a)->tv_sec cmp (b)->tv_sec)

/*
 * Random
 */

extern void
srand(unsigned int newseed);
extern long
random(void);

/*
 * String
 */

extern void *
memcpy(void *dst, const void *src, uint n);
extern long
strtol(const char *s, char **endptr, int base);
extern char *
strrchr(const char *cp, int ch);
extern int
strcmp(const char *p, const char *q);

#endif
