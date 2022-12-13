#include <mqueue.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#define die(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s DATA\n", argv[0]);
        return -1;
    }
    int msg_fd = mq_open("/postbox", O_WRONLY);
    if (msg_fd < 0) die("mq_open");

    int rc = mq_send(msg_fd, argv[1], strlen(argv[1]), 0);
    if (rc < 0) die("mq_send");
}
