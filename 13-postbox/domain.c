int domain_prepare(int epoll_fd) {
    int fd;
    unlink("socket");
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        die("socket");
    struct sockaddr_un addr = {
        .sun_family = AF_UNIX,
        .sun_path = "socket",
    };
    if (bind(fd, &addr, offsetof(struct sockaddr_un, sun_path) + strlen(addr.sun_path) + 1))
        die("socket");
    if (listen(fd, 100))
        die("listen");
    printf("... by socket: echo 2 | nc -U socket\n");
    epoll_add(epoll_fd, fd, EPOLLIN);
    return fd;
}


void domain_accept(int epoll_fd, int sock_fd, int events) {
    if (events & EPOLLHUP)
        return;
    socklen_t len;
    struct sockaddr_un addr;
    int new_fd;
    if ((new_fd = accept(sock_fd, &addr, &len)) < 0)
        die("accept");
    epoll_add(epoll_fd, new_fd, EPOLLIN);
}


void domain_recv(int epoll_fd, int sock_fd, int events) {
    if (events & EPOLLHUP) {
        epoll_del(epoll_fd, sock_fd);
        return;
    }

    char buf[256];
    int  ret;
    
    struct ucred ucred;
    socklen_t len = sizeof(struct ucred);
    int rc = getsockopt(sock_fd, SOL_SOCKET, SO_PEERCRED, &ucred, &len);
    if  (rc < 0)
        die("getsockopt");

    char *banner;
    asprintf(&banner, "socket%x[pid=%d, uid=%d, gid=%d]:\n", events, ucred.pid, ucred.uid, ucred.gid);
    write(1, banner, strlen(banner));

    ret = read(sock_fd, buf, sizeof buf);
    write(1, buf, ret);
}
