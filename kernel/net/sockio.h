#pragma once

#include "ioccom.h"

#define SIOCGIFINDEX   _IOWR('i',  0, struct ifreq)
#define SIOCGIFNAME    _IOWR('i',  1, struct ifreq)
#define SIOCSIFNAME     _IOW('i',  2, struct ifreq)
#define SIOCGIFHWADDR  _IOWR('i',  3, struct ifreq)
#define SIOCSIFHWADDR   _IOW('i',  4, struct ifreq)
#define SIOCGIFFLAGS   _IOWR('i',  5, struct ifreq)
#define SIOCSIFFLAGS    _IOW('i',  6, struct ifreq)
#define SIOCGIFADDR    _IOWR('i',  7, struct ifreq)
#define SIOCSIFADDR     _IOW('i',  8, struct ifreq)
#define SIOCGIFNETMASK _IOWR('i',  9, struct ifreq)
#define SIOCSIFNETMASK  _IOW('i', 10, struct ifreq)
#define SIOCGIFBRDADDR _IOWR('i', 11, struct ifreq)
#define SIOCSIFBRDADDR  _IOW('i', 12, struct ifreq)
#define SIOCGIFMTU     _IOWR('i', 13, struct ifreq)
#define SIOCSIFMTU      _IOW('i', 14, struct ifreq)
