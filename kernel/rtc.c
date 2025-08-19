#include "types.h"
#include "riscv.h"
#include "memlayout.h"
#include "defs.h"

#define RTC_TIME_LOW 0x00
#define RTC_TIME_HIGH 0x04

uint64
rtcread(void)
{
  return *(volatile uint64 *)(RTC + RTC_TIME_LOW);
}
