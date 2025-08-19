#ifndef PLATFORM_H
#define PLATFORM_H

#include "types.h"
#include "riscv.h"
#include "defs.h"

#include "std.h"

/*
 * Memory
 */

static inline void *
memory_alloc(size_t size)
{
    void *p;

    if (PGSIZE < size) {
        return NULL;
    }
    p = kalloc();
    if (p) {
        memset(p, 0, size);
    }
    return p;
}

static inline void
memory_free(void *ptr)
{
    kfree(ptr);
}

/*
 * Mutex
 */

#include "param.h"
#include "spinlock.h"
#include "proc.h"



/*
 * Interrupt
 */



/*
 * Scheduler
 */



#endif
