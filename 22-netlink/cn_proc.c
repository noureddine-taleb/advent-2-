#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <linux/connector.h>
#include <linux/netlink.h>
#include <linux/cn_proc.h>

#define die(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

// For a given PID, get the path of the executable
static char *execname(pid_t pid, char *buf, size_t bufsize) {
    char path[512];
    snprintf(path, sizeof(path)-1, "/proc/%d/exe", pid);

    // Read the linkname from the /proc file system
    int len = readlink(path, buf, bufsize-1);

    // Perhaps, the process has already exited and then thhe path in
    // proc no longer exists. Therefore, do not fail eagerly here.
    if (len < 0) return NULL;

    // Null-terminate the path and return it
    buf[len] = 0;
    return buf;
}

// For using cn_proc, we have to create an NETLINK_CONNECTOR socket
// and "connect" it. Please note that these sockets are not
// connect(2)'ed, but we bind it to the correct sockaddr_nl address.
int cn_proc_connect() {
    return -1;
}

// Without configuration, the Linux kernel does not creat
// fork/exec/exit messages on the CN_IDX_PROC multicast group.
// Therefore, we have to send an message to the kernel to enable those
// messages. This function enables/disables that multicast stream.
//
// Internally, the kernel keeps track (with a counter) how often
// PROC_CN_MCAST_{LISTEN|IGNORE} was called. Interestingly, if your
// process does not disable this mcast again, subsequent binds will
// directly produce proc_event messages.
void cn_proc_configure(int sock_fd, bool enable) {
    // For enabling, the CN_PROC mcast, we have to send an message
    // that consists of three parts:
    // +-------------------------------------------------------+
    // |  struct    | Padd  | struct       |  enum             |
    // |  nlmsghdr  | -ing  | cn_msg       |  proc_cn_mcast_op |
    // +-------------------------------------------------------+
    // 
    // Thereby, the nlmsghdr and the cn_msg have length fields of the
    // follwiing data. Think of this as encapsulated protocols. We
    // send cn_mcast_operation as an connector message over an netlink
    // transport.
    // The padding is: NLMSG_LENGTH(0) - sizeof(struct nlmsghdr)
    
    // FIXME: Initialize struct nlmsghdr, struct cn_msg, and struct proc_cn_mcast_op
    // FIXME: Create an iovec with 4 elements (don't forget the padding)
    // FIXME: You can send the message with writev(2)
}

// This function handles a single cn_proc event and prints it to the terminal
void cn_proc_handle(struct proc_event *ev) {
    // See /usr/include/linux/cn_proc.h for details

    printf("ev->what: %d\n", ev->what);
}


int cn_proc_fd;

void cn_proc_atexit() {
    cn_proc_configure(cn_proc_fd, false);
}

int main(int argc, char **argv) {
    // Unfortunately, the cn_proc interface is only available if you
    // have CAP_NET_ADMIN. As the most simple overapproximation of
    // this, we require the tool to be run as root.
    if (getuid() != 0) {
        fprintf(stderr, "must be run as root\n");
        return -1;
    }

    // FIXME: Create cn_proc socket and enable mcast
    // FIXME: recv(2) data from the socket and iterate over nl message (see netlink(7))
    // FIXME: For NLMSG_DONE messages, invoke cn_proc_handle on the (struct proc_event*)cn_hdr->data
    return 0;
}
