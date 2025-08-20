#include "kernel/types.h"
#include "user/user.h"
#include "kernel/net/socket.h"
#include "kernel/net/if.h"

static void
display(const char *name)
{
    struct ifreq ifr;
    int fd;
    const char **s, *str[] = {
        "UP",
        "BROADCAST",
        "DEBUG",
        "LOOPBACK",
        "POINTOPOINT",
        "NOTRAILERS",
        "RUNNING",
        "NEEDARP",
        "PROMISC",
        "ALLMULTI",
        "MASTER",
        "SLAVE",
        "MULTICAST",
        "PORTSEL",
        "AUTOMEDIA",
        "DYNAMIC",
        NULL
    };
    unsigned short mask = 1;
    int any = 0;
    uint8_t *p; 
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1)
        return;
    // name
    strcpy(ifr.ifr_name, name);
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) == -1) {
        close(fd);
        printf("ifconfig: interface %s does not exist\n", name);
        return;
    }
    printf("%s: ", ifr.ifr_name);
    // flags
    printf("flags=%x<", ifr.ifr_flags);
    for (s = str; *s; s++) {
        if (ifr.ifr_flags & mask) {
            if (any)
                printf("|");
            any = 1;
            printf("%s", *s);
        }
        mask <<= 1;
    }
    printf(">");
    // mtu
    ifr.ifr_mtu = 1500;
    if (ioctl(fd, SIOCGIFMTU, &ifr) == -1)
        ;//ifr.ifr_mtu = 0;
    printf(" mtu %d\n", ifr.ifr_mtu);
    // hwaddr
    if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0) {
        p = (uint8_t *)ifr.ifr_hwaddr.sa_data;
        printf("  ether %x:%x:%x:%x:%x:%x\n", p[0], p[1], p[2], p[3], p[4], p[5]);
    }
    do {
        // addr
        ifr.ifr_addr.sa_family = AF_INET;
        if (ioctl(fd, SIOCGIFADDR, &ifr) == -1)
            break;
        p = (uint8_t *)&((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
        printf("  inet %d.%d.%d.%d", p[0], p[1], p[2], p[3]);
        // netmask
        ifr.ifr_netmask.sa_family = AF_INET;
        if (ioctl(fd, SIOCGIFNETMASK, &ifr) == -1)
            break;
        p = (uint8_t *)&((struct sockaddr_in *)&ifr.ifr_netmask)->sin_addr;
        printf(" netmask %d.%d.%d.%d", p[0], p[1], p[2], p[3]);
        // broadcast
        ifr.ifr_broadaddr.sa_family = AF_INET;
        if (ioctl(fd, SIOCGIFBRDADDR, &ifr) == -1)
            break;
        p = (uint8_t *)&((struct sockaddr_in *)&ifr.ifr_broadaddr)->sin_addr;
        printf(" broadcast %d.%d.%d.%d\n", p[0], p[1], p[2], p[3]);
    } while(0);
    close(fd);
}

static void
display_all(void)
{
    int fd;
    struct ifreq ifr = {.ifr_ifindex = 0};

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        exit(-1);
    }
    while (1) {
        if (ioctl(fd, SIOCGIFNAME, &ifr) == -1)
            break;
        display(ifr.ifr_name);
        ifr.ifr_ifindex++;
    }
    close(fd);
}

static void
ifup(const char *name)
{
    int fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1)
        return;
    strcpy(ifr.ifr_name, name);
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) == -1) {
        close(fd);
        printf("ifconfig: interface %s does not exist\n", name);
        return;
    }
    ifr.ifr_flags |= IFF_UP;
    if (ioctl(fd, SIOCSIFFLAGS, &ifr) == -1) {
        close(fd);
        printf("ifconfig: ioctl(SIOCSIFFLAGS) failure, interface=%s\n", name);
        return;
    }
    close(fd);
}

static void
ifdown(const char *name)
{
    int fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1)
        return;
    strcpy(ifr.ifr_name, name);
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) == -1) {
        close(fd);
        printf("ifconfig: interface %s does not exist\n", name);
        return;
    }
    ifr.ifr_flags &= ~IFF_UP;
    if (ioctl(fd, SIOCSIFFLAGS, &ifr) == -1) {
        close(fd);
        printf("ifconfig: ioctl(SIOCSIFFLAGS) failure, interface=%s\n", name);
        return;
    }
    close(fd);
}

static void
ifset(const char *name, uint32_t addr, uint32_t netmask)
{
    int fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1)
        return;
    strcpy(ifr.ifr_name, name); 
    ifr.ifr_addr.sa_family = AF_INET;
    ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr = addr;
    if (ioctl(fd, SIOCSIFADDR, &ifr) == -1) {
        close(fd);
        printf("ifconfig: ioctl(SIOCSIFADDR) failure, interface=%s\n", name);
        return;
    }
    ifr.ifr_netmask.sa_family = AF_INET;
    ((struct sockaddr_in *)&ifr.ifr_netmask)->sin_addr.s_addr = netmask;
    if (ioctl(fd, SIOCSIFNETMASK, &ifr) == -1) {
        close(fd);
        printf("ifconfig: ioctl(SIOCSIFNETMASK) failure, interface=%s\n", name);
        return;
    }
    close(fd);
}

static void
usage(void)
{
    printf("usage: ifconfig interface [command|address]\n");
    printf("           - command: up | down\n");
    printf("           - address: ADDRESS/PREFIX | ADDRESS netmask NETMASK\n");
    printf("       ifconfig [-a]\n");
    exit(-1);
}

int
main(int argc, char *argv[])
{
    char *s;
    uint32_t addr, netmask;
    int prefix = 0;

    if (argc == 1) {
        display_all();
        exit(0);
    }
    if (argc == 2) {
        if (strcmp(argv[1], "-a") == 0)
            display_all();
        else
            display(argv[1]);
        exit(0);
    }
    if (argc == 3) {
        if (strcmp(argv[2], "up") == 0) {
            ifup(argv[1]);
            exit(0);
        }
        if (strcmp(argv[2], "down") == 0) {
            ifdown(argv[1]);
            exit(0);
        }
        s = strchr(argv[2], '/');
        if (!s)
            usage();
        *s++ = 0;
        if (inet_pton(AF_INET, argv[2], (struct in_addr *)&addr) == -1)
            usage();
        prefix = atoi(s);
        if (prefix < 0 || prefix > 32)
            usage();
        netmask = htonl(0xffffffff << (32 - prefix));
        ifset(argv[1], addr, netmask);
        exit(0);
    }
    if (argc == 5) {
        if (inet_pton(AF_INET, argv[2], (struct in_addr *)&addr) == -1)
            usage();
        if (strcmp(argv[3], "netmask") != 0)
            usage();
        if (inet_pton(AF_INET, argv[4], (struct in_addr *)&netmask) == -1)
            usage();
        ifset(argv[1], addr, netmask);
        exit(0);
    }
    usage();
}
