int signalfd_prepare(int epoll_fd) {
    printf("... by signal: /bin/kill -USR1 -q 3 %d \n", getpid());
    return -1;
}

void signalfd_handle(int epoll_fd, int signal_fd, int events) {
}
