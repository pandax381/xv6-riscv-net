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

#define INTR_IRQ_SOFTIRQ 0
#define INTR_IRQ_EVENT 0

static inline int
intr_raise_irq(unsigned int irq)
{
    return 0;
}

static inline int
intr_init(void)
{
    return 0;
}

static inline int
intr_run(void)
{
    return 0;
}

static inline void
intr_shutdown(void)
{
    return;
}

/*
 * Scheduler
 */



#endif
