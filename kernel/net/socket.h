#include "sockio.h"

#define PF_INET 1

#define AF_INET PF_INET

#define SOCK_DGRAM 1
#define SOCK_STREAM 2

#define IPPROTO_UDP 0
#define IPPROTO_TCP 0

#define INADDR_ANY ((uint32_t)0)

struct in_addr {
    uint32_t s_addr;
};

struct sockaddr {
    unsigned short sa_family;
    char sa_data[14];
};

struct sockaddr_in {
    unsigned short sin_family;
    uint16_t sin_port;
    struct in_addr sin_addr;
};

#define IFNAMSIZ 16

struct ifreq {
    char ifr_name[IFNAMSIZ]; /* Interface name */
    union {
        struct sockaddr ifr_addr;
        struct sockaddr ifr_dstaddr;
        struct sockaddr ifr_broadaddr;
        struct sockaddr ifr_netmask;
        struct sockaddr ifr_hwaddr;
        short           ifr_flags;
        int             ifr_ifindex;
        int             ifr_metric;
        int             ifr_mtu;
//      struct ifmap    ifr_map;
        char            ifr_slave[IFNAMSIZ];
        char            ifr_newname[IFNAMSIZ];
        char           *ifr_data;
    };
};
