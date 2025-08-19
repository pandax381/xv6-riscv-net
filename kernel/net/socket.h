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
