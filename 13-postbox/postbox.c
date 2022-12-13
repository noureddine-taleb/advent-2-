#define _GNU_SOURCE
#include <stdio.h>
#include <time.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <assert.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <mqueue.h>

#define die(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

// Adds a file descriptor to an open epoll instance's list of
// interesting file descriptors.
//// events -  For which events are we waiting (usually EPOLLIN)
//// data   -  The kernel returns this data when an event occurs 
void epoll_add(int epoll_fd, int fd, int events) {
    struct epoll_event ev;
    ev.events   = events;
    ev.data.fd  = fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        die("epoll_ctl: activate");
    }
     }

// Remove a file descriptor from the interest list.
void epoll_del(int epoll_fd, int fd) {
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
        die("epoll_ctl: reset");
    }
}

#include "fifo.c"
#include "domain.c"
#include "signalfd.c"
#include "mqueue.c"


struct postbox {
    int fd;
    int (*prepare)(int epoll_fd);
    void (*handle)(int epoll_fd, int fd, int events);
};

struct postbox boxes[] = {
    {-1, fifo_prepare,     fifo_handle},
    {-1, domain_prepare,   domain_accept},
    {-1, signalfd_prepare, signalfd_handle},
    {-1, mqueue_prepare,   mqueue_handle},
    // This must be the last entry
    {-1, NULL,             domain_recv}, // Fallback
};

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(*(arr)))

int main() {
    int epoll_fd = epoll_create1(0);
	if (epoll_fd == -1)
        die("epoll_create");

    printf("Santas Postbox is open! Send your requests ...\n");
    // Initialize all backends by calling the prepare method
    for (int i = 0; i < ARRAY_SIZE(boxes); i++) {
        if (boxes[i].prepare) {
            boxes[i].fd = boxes[i].prepare(epoll_fd);
        }
    }
    while (true) {
        // We use epoll_wait(2) to wait for at least one event, but we
        // can receive up to ten events. We do not set a timeout but
        // wait forever if necessary.
        struct epoll_event event[10];
        int nfds = epoll_wait(epoll_fd, event, 10, -1);

        for (int n = 0; n < nfds; n++) {
            int fd = event[n].data.fd;
            for (int i = 0; i < ARRAY_SIZE(boxes); i++) {
                if (boxes[i].fd == fd || boxes[i].fd == -1) {
                    boxes[i].handle(epoll_fd, fd, event[n].events);
                    break;
                }
            }
        }
    }

}
