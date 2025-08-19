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

typedef struct spinlock mutex_t;

#define MUTEX_INITIALIZER {0}

static inline int
mutex_init(mutex_t *mutex)
{
    initlock(mutex, "");
    return 0;
}

static inline int
mutex_lock(mutex_t *mutex)
{
    acquire(mutex);
    return 0;
}

static inline int
mutex_unlock(mutex_t *mutex)
{
    release(mutex);
    return 0;
}

/*
 * Interrupt
 */

#include "softirq.h"

#define INTR_IRQ_SOFTIRQ SOFT_IRQ_NET_RX
#define INTR_IRQ_EVENT SOFT_IRQ_NET_EVENT

static inline int
intr_raise_irq(unsigned int irq)
{
    acquire(&pendinglock);
    pending |= irq;
    release(&pendinglock);
    w_sip(r_sip() | SIP_SSIP);
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

struct sched_ctx {
    int interrupted;
    int wc; /* wait count */
};

#define SCHED_CTX_INITIALIZER {0, 0}

static inline int
sched_ctx_init(struct sched_ctx *ctx)
{
    ctx->interrupted = 0;
    ctx->wc = 0;
    return 0;
}

static inline int
sched_ctx_destroy(struct sched_ctx *ctx)
{
    if (ctx->wc) {
        return -1;
    }
    return 0;
}

static inline int
sched_sleep(struct sched_ctx *ctx, mutex_t *mutex, const struct timespec *abstime)
{
    (void)abstime;
    if (ctx->interrupted) {
        errno = EINTR;
        return -1;
    }
    ctx->wc++;
    sleep(ctx, mutex);
    ctx->wc--;
    if (ctx->interrupted) {
        if (!ctx->wc) {
            ctx->interrupted = 0;
        }
        errno = EINTR;
        return -1;
    }
    return 0;
}

static inline int
sched_wakeup(struct sched_ctx *ctx)
{
    wakeup(ctx);
    return 0;
}

static inline int
sched_interrupt(struct sched_ctx *ctx)
{
    ctx->interrupted = 1;
    wakeup(ctx);
    return 0;
}

#endif
