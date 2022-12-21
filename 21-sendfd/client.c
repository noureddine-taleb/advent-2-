#include <sys/socket.h>
#include <sys/signalfd.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>


#define die(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

// Receive an message with an attached file descriptor from a
// connected UNIX domain socket. The message is stored in the buffer,
// and the file descriptor in *fd. The function returns the length of
// the message.
//
// sock_fd:      Connected UNIX domain socket
// buf, bufsize: Buffer for the received message
// fd:           Where to store the file descriptor
int recvfd(int sock_fd, char *buf, size_t bufsize, int *fd) {
    return 0;
}

int main(int argc, char *argv[]) {
    // FIXME: Create an socket(2) with AF_UNIX, SOCK_SEQPACKET
    // FIXME: Connect to the domain socket "./socket"
    // FIXME: Receive the file descriptor with recvfd()
    // FIXME: Add a read/write loop that ships data from your stdin to the received file descriptor (like cat)
}
