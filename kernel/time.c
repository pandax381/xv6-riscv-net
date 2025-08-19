#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "time.h"

#define TZ_OFFSET 9 //JST

time_t
time(time_t *t)
{
  time_t _t;
  if (!t)
    t = &_t;
  *t = rtcread() / 1000000000;
  return *t;
}

int
gettimeofday(struct timeval *tv, void *tz)
{
  (void)tz;
  uint64 rtc = rtcread();
  tv->tv_sec = rtc / 1000000000;
  tv->tv_usec = (rtc % 1000000000) / 1000;
  return 0;
}

static int
isleapyear(int y)
{
  return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

static int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static int
ndays(int y, int m)
{
  int n = days[m];

  if (m == 1 && isleapyear(y)) {
    n++;
  }
  return n;
}

time_t
mktime(struct tm *tm)
{
  const int epoch = 1970;
  time_t result = 0;

  for (int y = epoch; y < tm->tm_year + 1900; y++) {
    result += (isleapyear(y) ? 366 : 365) * 24 * 3600;
  }
  for (int m = 0; m < tm->tm_mon; m++) {
    result += ndays(tm->tm_year + 1900, m) * 24 * 3600;
  }
  result += (tm->tm_mday - 1) * 24 * 3600;
  result += tm->tm_hour * 3600;
  result += tm->tm_min * 60;
  result += tm->tm_sec;
  result -= TZ_OFFSET * 3600;
  return result;
}

struct tm *
localtime_r(const time_t *timep, struct tm *result)
{
  time_t local_time;

  local_time = *timep + (TZ_OFFSET * 3600);
  result->tm_sec = local_time % 60;
  local_time /= 60;
  result->tm_min = local_time % 60;
  local_time /= 60;
  result->tm_hour = local_time % 24;
  local_time /= 24;

  int days = local_time;
  result->tm_wday = (days + 4) % 7;

  int y = 1970;
  while (1) {
    int n = isleapyear(y) ? 366 : 365;
    if (days < n) {
      break;
    }
    days -= n;
    y++;
  }
  result->tm_year = y - 1900;
  result->tm_yday = days;

  int m = 0;
  while (1) {
    int n = ndays(y, m);
    if (days < n) {
      break;
    }
    days -= n;
    m++;
  }
  result->tm_mon = m;
  result->tm_mday = days + 1;
  result->tm_isdst = 0;
  return result;
}
