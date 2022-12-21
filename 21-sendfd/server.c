#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <limits.h>
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
    // FIXME: Perpare an struct msghdr with an msg_control buffer
    // FIXME: Attach the file descriptor as cmsg (see cmsg(3), unix(7))
    // FIXME: Use sendmsg(2) to send the file descriptor and the message to the other process.
}

int main() {
    // FIXME: Create an socket with AF_UNIX and SOCK_SEQPACKET
    // FIXME: Bind it to a filename and listen
    // FIXME: Accept clients, send your STDOUT, and directly close the connection again
}

