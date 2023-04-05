#include <sys/socket.h>
#include <sys/signalfd.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <stddef.h>


#define die(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

// Receive an message with an attached file descriptor from a
// connected UNIX domain socket. The message is stored in the buffer,
// and the file descriptor in *fd. The function returns the length of
// the message.
//
// sock_fd:      Connected UNIX domain socket
// buf, bufsize: Buffer for the received message
// fd:           Where to store the file descriptor
void recvfd(int sock_fd, char *buf, size_t bufsize, int *fd) {
    struct iovec io = {
        .iov_base = buf,
        .iov_len = bufsize,
    };
    
    char buffer[1024];
    struct msghdr msgh = {
        .msg_iovlen = 1,
        .msg_iov = &io,
        .msg_control = buffer,
        .msg_controllen = sizeof buffer,
    };
    struct cmsghdr *cmsg = msgh.msg_control;

    /* Receive auxiliary data in msgh */
    if (recvmsg(sock_fd, &msgh, 0) < 0)
        die("recvmsg");
    *fd = *(int *)CMSG_DATA(cmsg);
}

int main(int argc, char *argv[]) {
    int fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (fd < 0)
        die("socket");
    struct sockaddr_un addr = {
        .sun_family = AF_UNIX,
        .sun_path = "socket",
    };
    int ret;
    ret = connect(fd, &addr, offsetof(struct sockaddr_un, sun_path) + strlen(addr.sun_path) + 1);
    if (ret < 0)
        die("connect");

    char buf[1024];
    int outfd = -1;
    recvfd(fd, buf, sizeof buf, &outfd);
    printf("outfd = %d\n", outfd);
    while ((ret = read(0, buf, sizeof buf)) > 0) {
        write(outfd, buf, ret);
    }
}
