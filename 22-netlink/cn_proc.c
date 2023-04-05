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

#define die(msg)            \
    do                      \
    {                       \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0)

int cn_proc_fd;

// For a given PID, get the path of the executable
char *execname(pid_t pid)
{
    char path[512];
    static char buffer[512];
    snprintf(path, sizeof(path) - 1, "/proc/%d/exe", pid);

    // Read the linkname from the /proc file system
    int len = readlink(path, buffer, sizeof(buffer) - 1);

    // Perhaps, the process has already exited and then thhe path in
    // proc no longer exists. Therefore, do not fail eagerly here.
    if (len < 0)
        return "";

    // Null-terminate the path and return it
    buffer[len] = 0;
    return buffer;
}

// For using cn_proc, we have to create an NETLINK_CONNECTOR socket
// and "connect" it. Please note that these sockets are not
// connect(2)'ed, but we bind it to the correct sockaddr_nl address.
int cn_proc_connect()
{
    int sock = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR);
    if (sock < 0)
        die("socket");
    struct sockaddr_nl addr = {
        .nl_family = AF_NETLINK,
        .nl_groups = CN_IDX_PROC,
        .nl_pid = getpid(),
    };
    int ret = bind(sock, &addr, sizeof(addr));
    if (ret < 0)
        die("bind");
    return sock;
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
void cn_proc_configure()
{
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
    int rc;
    struct __attribute__((aligned(NLMSG_ALIGNTO)))
    {
        struct nlmsghdr nl_hdr;
        struct __attribute__((__packed__))
        {
            struct cn_msg cn_msg;
            enum proc_cn_mcast_op cn_mcast;
        };
    } nlcn_msg;

    memset(&nlcn_msg, 0, sizeof(nlcn_msg));
    nlcn_msg.nl_hdr.nlmsg_len = sizeof(nlcn_msg);
    nlcn_msg.nl_hdr.nlmsg_pid = getpid();
    nlcn_msg.nl_hdr.nlmsg_type = NLMSG_DONE;

    nlcn_msg.cn_msg.id.idx = CN_IDX_PROC;
    nlcn_msg.cn_msg.id.val = CN_VAL_PROC;
    nlcn_msg.cn_msg.len = sizeof(enum proc_cn_mcast_op);

    nlcn_msg.cn_mcast = PROC_CN_MCAST_LISTEN;

    rc = send(cn_proc_fd, &nlcn_msg, sizeof(nlcn_msg), 0);
    if (rc == -1)
        die("netlink send");
}

// This function handles a single cn_proc event and prints it to the terminal
void cn_proc_handle_single(struct proc_event *ev)
{
    switch (ev->what)
    {
    case PROC_EVENT_FORK:
        printf("fork(%s): %dt -> %dt\n",
                execname(ev->event_data.fork.parent_tgid),
                ev->event_data.fork.parent_pid,
                ev->event_data.fork.child_pid
            );
        break;
    case PROC_EVENT_EXEC:
        printf("exec(%s): %dt\n",
                execname(ev->event_data.exec.process_tgid),
                ev->event_data.exec.process_pid
            );
        break;
    case PROC_EVENT_UID:
        printf("uid(%s): %dt: %du -> %du\n",
                execname(ev->event_data.id.process_tgid),
                ev->event_data.id.process_pid,
                ev->event_data.id.r.ruid,
                ev->event_data.id.e.euid);
        break;
    case PROC_EVENT_GID:
        printf("gid(%s): %dt: %dg -> %dg\n",
                execname(ev->event_data.id.process_tgid),
                ev->event_data.id.process_pid,
                ev->event_data.id.r.rgid,
                ev->event_data.id.e.egid);
        break;
    case PROC_EVENT_EXIT:
        printf("exit(%s) = %d: %dt\n",
                execname(ev->event_data.exit.process_tgid),
                ev->event_data.exit.exit_code,
                ev->event_data.exit.process_pid
            );
        break;
    default:
        break;
    }
}

void cn_proc_handle()
{
    int ret;
    struct __attribute__((aligned(NLMSG_ALIGNTO)))
    {
        struct nlmsghdr nl_hdr;
        struct __attribute__((__packed__))
        {
            struct cn_msg cn_msg;
            struct proc_event proc_ev;
        };
    } nlcn_msg;

    while (1)
    {
        ret = recv(cn_proc_fd, &nlcn_msg, sizeof(nlcn_msg), 0);
        if (ret <= 0)
            die("netlink recv");
        cn_proc_handle_single(&nlcn_msg.proc_ev);
    }
}

int main(int argc, char **argv)
{
    // Unfortunately, the cn_proc interface is only available if you
    // have CAP_NET_ADMIN. As the most simple overapproximation of
    // this, we require the tool to be run as root.
    cn_proc_fd = cn_proc_connect();
    cn_proc_configure();
    cn_proc_handle();
    return 0;
}
