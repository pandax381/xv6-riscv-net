#include "platform.h"

#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "socket.h"
#include "if.h"

#include "udp.h"
#include "tcp.h"

struct socket {
    int type;
    int desc;
};

struct file*
socket_alloc(int domain, int type, int protocol)
{
    struct file *f;
    struct socket *s;

    if (domain != AF_INET || protocol != 0) {
        return NULL;
    }
    f = filealloc();
    if (!f) {
        return NULL;
    }
    s = (struct socket *)kalloc();
    if (!s) {
        fileclose(f);
        return NULL;
    }
    s->type = type;
    switch(type) {
    case SOCK_DGRAM:
        s->desc = udp_open();
        break;
    case SOCK_STREAM:
        s->desc = tcp_open();
        break;
    default:
        fileclose(f);
        memory_free(s);
        return NULL;
    }
    f->type = FD_SOCKET;
    f->readable = 1;
    f->writable = 1;
    f->socket = s;
    return f;
}

int
socket_close(struct socket *s)
{
    switch (s->type) {
    case SOCK_DGRAM:
        udp_close(s->desc);
        break;
    case SOCK_STREAM:
        tcp_close(s->desc);
        break;
    default:
        return -1;
    }
    memory_free(s);
    return 0;
}

int
socket_bind(struct socket *s, struct sockaddr *addr, int addrlen)
{
    struct ip_endpoint local;

    local.addr = ((struct sockaddr_in *)addr)->sin_addr.s_addr;
    local.port = ((struct sockaddr_in *)addr)->sin_port;
    switch (s->type) {
    case SOCK_DGRAM:
        return udp_bind(s->desc, &local);
    case SOCK_STREAM:
        return tcp_bind(s->desc, &local);
    default:
        return -1;
    }
}

int
socket_recvfrom(struct socket *s, char *buf, int n, struct sockaddr *addr, int *addrlen)
{
    struct ip_endpoint foreign;
    int ret;

    if (s->type != SOCK_DGRAM) {
        return -1;
    }
    ret = udp_recvfrom(s->desc, (uint8_t *)buf, n, &foreign);
    if (addr) {
        ((struct sockaddr_in *)addr)->sin_family = AF_INET;
        ((struct sockaddr_in *)addr)->sin_addr.s_addr = foreign.addr;
        ((struct sockaddr_in *)addr)->sin_port = foreign.port;
    }
    return ret;
}

int
socket_sendto(struct socket *s, char *buf, int n, struct sockaddr *addr, int addrlen)
{
    struct ip_endpoint foreign;

    if (s->type != SOCK_DGRAM) {
        return -1;
    }
    foreign.addr = ((struct sockaddr_in *)addr)->sin_addr.s_addr;
    foreign.port = ((struct sockaddr_in *)addr)->sin_port;
    return udp_sendto(s->desc, (uint8_t *)buf, n, &foreign);
}

int
socket_connect(struct socket *s, struct sockaddr *addr, int addrlen)
{
    struct ip_endpoint foreign;

    if (s->type != SOCK_STREAM) {
      return -1;
    }
    foreign.addr = ((struct sockaddr_in *)addr)->sin_addr.s_addr;
    foreign.port = ((struct sockaddr_in *)addr)->sin_port;
    return tcp_connect(s->desc, &foreign);
}

int
socket_listen(struct socket *s, int backlog)
{
    if (s->type != SOCK_STREAM) {
        return -1;
    }
    return tcp_listen(s->desc, backlog);
}

struct file *
socket_accept(struct socket *s, struct sockaddr *addr, int *addrlen)
{
    int adesc;
    struct file *f;
    struct socket *as;
    struct ip_endpoint foreign;

    if (s->type != SOCK_STREAM) {
        return NULL;
    }
    f = filealloc();
    if (!f) {
        return NULL;
    }
    as = (struct socket *)kalloc();
    if (!as) {
        fileclose(f);
        return NULL;
    }
    adesc = tcp_accept(s->desc, &foreign);
    if (adesc == -1) {
        fileclose(f);
        kfree((void*)as);
        return NULL;
    }
    ((struct sockaddr_in *)addr)->sin_family = AF_INET;
    ((struct sockaddr_in *)addr)->sin_addr.s_addr = foreign.addr;
    ((struct sockaddr_in *)addr)->sin_port = foreign.port;
    as->type = s->type;
    as->desc = adesc;
    f->type = FD_SOCKET;
    f->readable = 1;
    f->writable = 1;
    f->socket = as;
    return f;
}

int
socket_read(struct socket *s, char *buf, int n)
{
    if (s->type != SOCK_STREAM) {
        return -1;
    }
    return tcp_receive(s->desc, (uint8_t *)buf, n);
}

int
socket_write(struct socket *s, char *buf, int n)
{
    if (s->type != SOCK_STREAM) {
        return -1;
    }
    return tcp_send(s->desc, (uint8_t *)buf, n);
}

int
socket_ioctl(struct socket *s, int req, void *arg)
{
    struct ifreq *ifreq;
    struct net_device *dev;
    struct ip_iface *iface;
    char addr[IP_ADDR_STR_LEN];

    ifreq = (struct ifreq *)arg;
    switch ((unsigned int)req) {
    case SIOCGIFINDEX:
        dev = net_device_by_name(ifreq->ifr_name);
        if (!dev) {
            return -1;
        }
        ifreq->ifr_ifindex = dev->index;
        break;
    case SIOCGIFNAME:
        dev = net_device_by_index(ifreq->ifr_ifindex);
        if (!dev) {
            return -1;
        }
        strncpy(ifreq->ifr_name, dev->name, sizeof(ifreq->ifr_name));
        break;
    case SIOCSIFNAME:
        // TODO
        break;
    case SIOCGIFHWADDR:
        dev = net_device_by_name(ifreq->ifr_name);
        if (!dev) {
            return -1;
        }
        // TODO: HW type check
        memcpy(ifreq->ifr_hwaddr.sa_data, dev->addr, dev->alen);
        break;
    case SIOCSIFHWADDR:
        // TODO
        break;
    case SIOCGIFFLAGS:
        dev = net_device_by_name(ifreq->ifr_name);
        if (!dev) {
            return -1;
        }
        ifreq->ifr_flags = dev->flags;
        break;
    case SIOCSIFFLAGS:
        dev = net_device_by_name(ifreq->ifr_name);
        if (!dev) {
            return -1;
        }
        if ((dev->flags & IFF_UP) != (ifreq->ifr_flags & IFF_UP)) {
            if (ifreq->ifr_flags & IFF_UP) {
                net_device_open(dev);
            } else {
                net_device_close(dev);
            }
        }
        break;
    case SIOCGIFADDR:
        dev = net_device_by_name(ifreq->ifr_name);
        if (!dev) {
            return -1;
        }
        switch (ifreq->ifr_addr.sa_family) {
        case AF_INET:
            iface = (struct ip_iface *)net_device_get_iface(dev, NET_IFACE_FAMILY_IP);
            if (!iface) {
                return -1;
            }
            ((struct sockaddr_in *)&ifreq->ifr_addr)->sin_addr.s_addr = iface->unicast;
            break;
        default:
            return -1;
        }
        break;
    case SIOCSIFADDR:
        dev = net_device_by_name(ifreq->ifr_name);
        if (!dev) {
            return -1;
        }
        switch (ifreq->ifr_addr.sa_family) {
        case AF_INET:
            iface = (struct ip_iface *)net_device_get_iface(dev, NET_IFACE_FAMILY_IP);
            if (iface) {
                if (ip_iface_reconfigure(iface, ((struct sockaddr_in *)&ifreq->ifr_addr)->sin_addr.s_addr, iface->netmask) == -1) {
                    return -1;
                }
            } else {
                iface = ip_iface_alloc(ip_addr_ntop(((struct sockaddr_in *)&ifreq->ifr_addr)->sin_addr.s_addr, addr, sizeof(addr)), "255.255.255.255");
                if (!iface) {
                    return -1;
                }
                ip_iface_register(dev, iface);
            }
            break;
        default:
            return -1;
        }
        break;
    case SIOCGIFNETMASK:
        dev = net_device_by_name(ifreq->ifr_name);
        if (!dev) {
            return -1;
        }
        switch (ifreq->ifr_addr.sa_family) {
        case AF_INET:
            iface = (struct ip_iface *)net_device_get_iface(dev, NET_IFACE_FAMILY_IP);
            if (!iface) {
                return -1;
            }
            ((struct sockaddr_in *)&ifreq->ifr_netmask)->sin_addr.s_addr = iface->netmask;
            break;
        default:
            return -1;
        }
        break;
    case SIOCSIFNETMASK:
        dev = net_device_by_name(ifreq->ifr_name);
        if (!dev) {
            return -1;
        }
        switch (ifreq->ifr_addr.sa_family) {
        case AF_INET:
            iface = (struct ip_iface *)net_device_get_iface(dev, NET_IFACE_FAMILY_IP);
            if (!iface) {
                return -1;
            }
            if (ip_iface_reconfigure(iface, iface->unicast, ((struct sockaddr_in *)&ifreq->ifr_addr)->sin_addr.s_addr) == -1) {
                return -1;
            }
            break;
        default:
            return -1;
        }
        break;
    case SIOCGIFBRDADDR:
        dev = net_device_by_name(ifreq->ifr_name);
        if (!dev) {
            return -1;
        }
        switch (ifreq->ifr_addr.sa_family) {
        case AF_INET:
            iface = (struct ip_iface *)net_device_get_iface(dev, NET_IFACE_FAMILY_IP);
            if (!iface) {
                return -1;
            }
            ((struct sockaddr_in *)&ifreq->ifr_broadaddr)->sin_addr.s_addr = iface->broadcast;
            break;
        default:
            return -1;
        }
        break;
    case SIOCSIFBRDADDR:
        // TODO
        break;
    case SIOCGIFMTU:
        dev = net_device_by_name(ifreq->ifr_name);
        if (!dev) {
            return -1;
        }
        ifreq->ifr_mtu = dev->mtu;
        break;
    case SIOCSIFMTU:
        break;
    default:
        return -1;
    }
    return 0;
}
