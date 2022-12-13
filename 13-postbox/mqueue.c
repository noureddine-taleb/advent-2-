int mqueue_prepare(int epoll_fd) {
    printf("... by mq_send: ./mq_send 4 (see also `cat /dev/mqueue/postbox`)\n");
    return -1;
}

void mqueue_handle(int epoll_fd, int msg_fd, int events) {
}

