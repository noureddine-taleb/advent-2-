
int fifo_prepare(int epoll_fd) {
    printf("... by fifo:   echo 1 > fifo\n");
    return -1;
}

void fifo_handle(int epoll_fd, int fifo_fd, int events) {
}
