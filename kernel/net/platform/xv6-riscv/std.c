#include "platform.h"

int errno;

/*
 * STDIO
 */

FILE *stderr;

void
flockfile(FILE *fp)
{
    /* dummy */
}

void
funlockfile(FILE *fp)
{
    /* dummy */
}

/*
 * Time
 */

size_t
strftime(char *s, size_t max, const char *format, const struct tm *tm)
{
    (void)format; /* force HH:MM:SS */
    return snprintf(s, max, "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
}

void
timersub(struct timeval *a, struct timeval *b, struct timeval *res)
{
    res->tv_sec = a->tv_sec - b->tv_sec;
    res->tv_usec = a->tv_usec - b->tv_usec;
    if (res->tv_usec < 0) {
        --res->tv_sec;
        res->tv_usec += 1000*1000;
    }
    return;
}

void
timerclear(struct timeval *tv)
{
    tv->tv_sec = 0;
    tv->tv_usec = 0;
}

/*
 * Random
 */

static unsigned int seed = 1;

void
srand(unsigned int newseed)
{
    seed = newseed;
}

long
random(void)
{
    /* Linear Congruential Generator (LCG) */
    seed = (seed * 1103515245 + 12345) % 0x7fffffff;
    return seed;
}

/*
 * String
 */

long
strtol(const char *s, char **endptr, int base)
{
    int neg = 0;
    long val = 0;

    // gobble initial whitespace
    while (*s == ' ' || *s == '\t')
        s++;

    // plus/minus sign
    if (*s == '+')
        s++;
    else if (*s == '-')
        s++, neg = 1;

    // hex or octal base prefix
    if ((base == 0 || base == 16) && (s[0] == '0' && s[1] == 'x'))
        s += 2, base = 16;
    else if (base == 0 && s[0] == '0')
        s++, base = 8;
    else if (base == 0)
        base = 10;

    // digits
    while (1) {
        int dig;

        if (*s >= '0' && *s <= '9')
            dig = *s - '0';
        else if (*s >= 'a' && *s <= 'z')
            dig = *s - 'a' + 10;
        else if (*s >= 'A' && *s <= 'Z')
            dig = *s - 'A' + 10;
        else
            break;
        if (dig >= base)
            break;
        s++, val = (val * base) + dig;
        // we don't properly detect overflow!
    }

    if (endptr)
        *endptr = (char *) s;
    return (neg ? -val : val);
}

char *
strrchr(const char *cp, int ch)
{
    char *save;
    char c;

    for (save = (char *) 0; (c = *cp); cp++) {
        if (c == ch) {
            save = (char *) cp;
        }
    }
    return save;
}

int
strcmp(const char *p, const char *q)
{
  while(*p && *p == *q)
    p++, q++;
  return (uchar)*p - (uchar)*q;
}
