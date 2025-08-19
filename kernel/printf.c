//
// formatted console output -- printf, panic.
//

#include <stdarg.h>

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

volatile int panicked = 0;

// lock to avoid interleaving concurrent printf's.
static struct {
  struct spinlock lock;
  int locking;
} pr;

static void
putch(int ch, int *cnt)
{
  consputc(ch);
  (void)*cnt++;
}

int
vcprintf(const char *fmt, va_list ap)
{
  int cnt = 0;

  vprintfmt((void*)putch, &cnt, fmt, ap);
  return cnt;
}

int
printf(char *fmt, ...)
{
  int locking, cnt;
  va_list ap;

  locking = pr.locking;
  if(locking)
    acquire(&pr.lock);

  va_start(ap, fmt);
  cnt = vcprintf(fmt, ap);
  va_end(ap);

  if(locking)
    release(&pr.lock);

  return cnt;
}

void
panic(char *s)
{
  pr.locking = 0;
  printf("panic: ");
  printf("%s\n", s);
  panicked = 1; // freeze uart output from other CPUs
  for(;;)
    ;
}

void
printfinit(void)
{
  initlock(&pr.lock, "pr");
  pr.locking = 1;
}
