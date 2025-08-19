#include "kernel/types.h"
#include "user/user.h"
#include "kernel/net/socket.h"

int
main (int argc, char *argv[])
{
    int soc, acc, peerlen, ret;
    struct sockaddr_in self, peer;
    unsigned char *addr;
    char buf[2048];

    printf("Starting TCP Echo Server\n");
    soc = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (soc == 1) {
        printf("socket: failure\n");
        exit(1);
    }
    printf("socket: success, soc=%d\n", soc);
    self.sin_family = AF_INET;
    self.sin_addr.s_addr = INADDR_ANY;
    self.sin_port = htons(7);
    if (bind(soc, (struct sockaddr *)&self, sizeof(self)) == -1) {
        printf("bind: failure\n");
        close(soc);
        exit(1);
    }
    addr = (unsigned char *)&self.sin_addr;
    printf("bind: success, self=%d.%d.%d.%d:%d\n", addr[0], addr[1], addr[2], addr[3], ntohs(self.sin_port));
    listen(soc, 100);
    printf("waiting for connection...\n");
    peerlen = sizeof(peer);
    acc = accept(soc, (struct sockaddr *)&peer, &peerlen);
    if (acc == -1) {
        printf("accept: failure\n");
        close(soc);
        exit(1);
    }
    addr = (unsigned char *)&peer.sin_addr;
    printf("accept: success, peer=%d.%d.%d.%d:%d\n", addr[0], addr[1], addr[2], addr[3], ntohs(peer.sin_port));
    while (1) {
        ret = recv(acc, buf, sizeof(buf));
        if (ret <= 0) {
            printf("EOF\n");
            break;
        }
        printf("recv: %d bytes data received\n", ret);
        send(acc, buf, ret);
    }
    close(soc);  
    exit(0);
}
