#ifndef TCP_H
#define TCP_H

#include "platform.h"

#include "ip.h"

extern int
tcp_init(void);

extern int
tcp_open_rfc793(struct ip_endpoint *local, struct ip_endpoint *foreign, int active);
extern int
tcp_close(int id);
extern int
tcp_open(void);
extern int
tcp_bind(int id, struct ip_endpoint *local);
extern int
tcp_connect(int id, struct ip_endpoint *foreign);
extern int
tcp_listen(int id, int backlog);
extern int
tcp_accept(int id, struct ip_endpoint *foreign);
extern ssize_t
tcp_send(int id, uint8_t *data, size_t len);
extern ssize_t
tcp_receive(int id, uint8_t *buf, size_t size);

#endif
