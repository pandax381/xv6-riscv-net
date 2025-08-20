#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "proc.h"

#include "socket.h"

int
sys_socket(void)
{
  int fd, domain, type, protocol;
  struct file *f;

  argint(0, &domain);
  argint(1, &type);
  argint(2, &protocol);
  if ((f = socket_alloc(domain, type, protocol)) == 0 || (fd = fdalloc(f)) < 0){
    if (f)
      fileclose(f);
    return -1;
  }
  return fd;
}

int
sys_bind(void)
{
  struct file *f;
  uint64 addr_p;
  struct sockaddr_in addr;
  int addrlen;
  struct proc *p = myproc();

  if (argfd(0, 0, &f) < 0 || f->type != FD_SOCKET){
    return -1;
  }
  argaddr(1, &addr_p);
  argint(2, &addrlen);
  if (!addr_p || addrlen < 0) {
    return -1;
  }
  if (copyin(p->pagetable, (char *)&addr, addr_p, addrlen) < 0) {
    return -1;
  }
  return socket_bind(f->socket, (struct sockaddr *)&addr, addrlen);
}

int
sys_recvfrom(void)
{
  struct file *f;
  uint64 buf_p;
  char buf[2048];
  int buflen;
  uint64 addr_p;
  struct sockaddr_in addr;
  uint64 addrlen_p;
  int addrlen = 0;
  int ret;
  struct proc *p = myproc();

  if (argfd(0, 0, &f) < 0 || f->type != FD_SOCKET){
    return -1;
  }
  argaddr(1, &buf_p);
  argint(2, &buflen);
  if (!buf_p || buflen < 0 || buflen > sizeof(buf)) {
    return -1;
  }
  argaddr(3, (void*)&addr_p);
  argaddr(4, (void*)&addrlen_p);
  if (addrlen_p) {
    if (copyin(p->pagetable, (char *)&addrlen, addrlen_p, sizeof(addrlen)) < 0) {
      return -1;
    }
    if (addrlen && addrlen != sizeof(addr)) {
      return -1;
    }
  }
  if (addrlen && !addr_p) {
    return -1;
  }
  ret = socket_recvfrom(f->socket, buf, buflen, (struct sockaddr *)&addr, &addrlen);
  if (copyout(p->pagetable, buf_p, buf, buflen) < 0) {
    return -1;
  }
  if (addr_p) {
    if (copyout(p->pagetable, addr_p, (char *)&addr, addrlen) < 0) {
      return -1;
    }
    if (copyout(p->pagetable, addrlen_p, (char *)&addrlen, sizeof(addrlen)) < 0) {
      return -1;
    }
  }
  return ret;
}

int
sys_sendto(void)
{
  struct file *f;
  uint64 buf_p;
  char buf[2048];
  int buflen;
  uint64 addr_p;
  struct sockaddr_in addr;
  int addrlen;
  struct proc *p = myproc();

  if (argfd(0, 0, &f) < 0 || f->type != FD_SOCKET){
    return -1;
  }
  argaddr(1, &buf_p);
  argint(2, &buflen);
  if (!buf_p || buflen < 0 || buflen > sizeof(buf)) {
    return -1;
  }
  if (copyin(p->pagetable, buf, buf_p, buflen) < 0) {
    return -1;
  }
  argaddr(3, &addr_p);
  argint(4, &addrlen);
  if (!addr_p || addrlen < 0) {
    return -1;
  }
  if (copyin(p->pagetable, (char *)&addr, addr_p, addrlen) < 0) {
    return -1;
  }
  return socket_sendto(f->socket, buf, buflen, (struct sockaddr *)&addr, addrlen);
}

int
sys_connect(void)
{
  struct file *f;
  uint64 addr_p;
  struct sockaddr_in addr;
  int addrlen;
  struct proc *p = myproc();

  if (argfd(0, 0, &f) < 0 || f->type != FD_SOCKET){
    return -1;
  }
  argaddr(1, &addr_p);
  argint(2, &addrlen);
  if (!addr_p || addrlen < 0) {
    return -1;
  }
  if (copyin(p->pagetable, (char *)&addr, addr_p, addrlen) < 0) {
    return -1;
  }
  return socket_connect(f->socket, (struct sockaddr *)&addr, addrlen);
}

int
sys_listen(void)
{
  struct file *f;
  int backlog;

  if (argfd(0, 0, &f) < 0 || f->type != FD_SOCKET){
    return -1;
  }
  argint(1, &backlog);
  return socket_listen(f->socket, backlog);
}

int
sys_accept(void)
{
  struct file *f, *newf;
  uint64 addr_p;
  struct sockaddr_in addr;
  uint64 addrlen_p;
  int addrlen = 0;
  int newfd;
  struct proc *p = myproc();

  if (argfd(0, 0, &f) < 0 || f->type != FD_SOCKET){
    return -1;
  }
  argaddr(1, (void*)&addr_p);
  argaddr(2, (void*)&addrlen_p);
  if (addrlen_p) {
    if (copyin(p->pagetable, (char *)&addrlen, addrlen_p, sizeof(addrlen)) < 0) {
      return -1;
    }
    if (addrlen && addrlen != sizeof(addr)) {
      return -1;
    }
  }
  if (addrlen && !addr_p) {
    return -1;
  }
  if ((newf = socket_accept(f->socket, (struct sockaddr *)&addr, &addrlen)) == 0 || (newfd = fdalloc(newf)) < 0){
    if (newf)
      fileclose(newf);
    return -1;
  }
  if (addr_p) {
    if (copyout(p->pagetable, addr_p, (char *)&addr, addrlen) < 0) {
      return -1;
    }
    if (copyout(p->pagetable, addrlen_p, (char *)&addrlen, sizeof(addrlen)) < 0) {
      return -1;
    }
  }
  return newfd;
}

int
sys_recv(void)
{
  struct file *f;
  uint64 buf_p;
  char buf[2048];
  int buflen;
  int ret;
  struct proc *p = myproc();

  if (argfd(0, 0, &f) < 0 || f->type != FD_SOCKET){
    return -1;
  }
  argaddr(1, &buf_p);
  argint(2, &buflen);
  if (!buf_p || buflen < 0 || buflen > sizeof(buf)) {
    return -1;
  }
  ret = socket_read(f->socket, buf, buflen);
  if (copyout(p->pagetable, buf_p, buf, buflen) < 0) {
    return -1;
  }
  return ret;
}

int
sys_send(void)
{
  struct file *f;
  uint64 buf_p;
  char buf[2048];
  int buflen;
  struct proc *p = myproc();

  if (argfd(0, 0, &f) < 0 || f->type != FD_SOCKET){
    return -1;
  }
  argaddr(1, &buf_p);
  argint(2, &buflen);
  if (!buf_p || buflen < 0 || buflen > sizeof(buf)) {
    return -1;
  }
  if (copyin(p->pagetable, buf, buf_p, buflen) < 0) {
    return -1;
  }
  return socket_write(f->socket, buf, buflen);
}

int
sys_ioctl(void)
{
  struct file *f;
  int req;
  uint64 ifr_p;
  struct ifreq ifr;
  struct proc *p = myproc();
  int ret;

  if (argfd(0, 0, &f) < 0 || f->type != FD_SOCKET){
    return -1;
  }
  argint(1, &req);
  if (req & IOC_IN) {
    argaddr(2, &ifr_p);
    if (!ifr_p) {
      return -1;
    }
    if (copyin(p->pagetable, (char *)&ifr, ifr_p, sizeof(ifr)) < 0) {
      return -1;
    }
  }
  ret = socket_ioctl(f->socket, req, &ifr);
  if (req & IOC_OUT) {
    if (copyout(p->pagetable, ifr_p, (char *)&ifr, sizeof(ifr)) < 0) {
      return -1;
    }
  }
  return ret;
}
