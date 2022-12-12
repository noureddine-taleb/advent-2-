int domain_prepare(int epoll_fd) {
    printf("... by socket: echo 2 | nc -U socket\n");
    return -1;
}


void domain_accept(int epoll_fd, int sock_fd, int events) {
}


void domain_recv(int epoll_fd, int sock_fd, int events) {
}
