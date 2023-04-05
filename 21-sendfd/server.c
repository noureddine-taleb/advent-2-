#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <sys/un.h>
#include <sys/socket.h>

#define die(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

// Send a file descriptor over an connected UNIX domain socket. The
// file descriptor is send as an auxiliary data attached to a regular
// buffer. With Linux, we have to send at least one byte to transfer a
// file descriptor.
//
// sockfd:      connected UNIX domain socket
// buf, buflen: Message to send, arbitary data
// fd:          file descriptor to transfer
void sendfd(int sockfd, void *buf, size_t buflen, int fd) {
    struct msghdr msg = { 0 };
    struct cmsghdr *cmsg;
    struct iovec io = {
        .iov_base = buf,
        .iov_len = buflen,
    };
    union {         /* Ancillary data buffer, wrapped in a union
                        in order to ensure it is suitably aligned */
        char buf[CMSG_SPACE(sizeof(fd))];
        struct cmsghdr __align;
    } u;

    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = u.buf;
    msg.msg_controllen = sizeof(u.buf);
    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(fd));
    *(int *)CMSG_DATA(cmsg) = fd;
    if (sendmsg(sockfd, &msg, 0) < 0)
        die("sendmsg");
}

int main() {
    int fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (fd < 0)
        die("socket");
    unlink("socket");
    struct sockaddr_un addr = {
        .sun_family = AF_UNIX,
        .sun_path = "socket",
    };
    int ret = bind(fd, &addr, offsetof(struct sockaddr_un, sun_path) + strlen(addr.sun_path) + 1);
    if (ret < 0)
        die("bind");
    if (listen(fd, 10) < 0)
        die("listen");
    
    socklen_t _len;
    struct sockaddr_un _addr;

    int client = accept(fd, &_addr, &_len);
    if (client < 0)
        die("accept");

    char iobuf[1];
    sendfd(client, iobuf, sizeof iobuf, 1);
}

