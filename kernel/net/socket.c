#include "platform.h"

#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "socket.h"

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
    case SOCK_STREAM:
        tcp_close(s->desc);
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
