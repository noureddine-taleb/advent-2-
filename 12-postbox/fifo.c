
int fifo_prepare(int epoll_fd) {
    unlink("fifo");
    mknod("fifo", S_IRUSR | S_IWUSR | S_IFIFO, 0);
    int fd  = open("fifo", O_RDONLY|O_NONBLOCK);
    epoll_add(epoll_fd, fd, EPOLLIN);
    printf("... by fifo:   echo 1 > fifo\n");
    return fd;
}

void fifo_handle(int epoll_fd, int fifo_fd, int events) {
    if (events & EPOLLHUP)
        return;
        
    char buf[256];
    int  ret;
    char banner[] = "fifo: ";
    write(1, banner, sizeof banner);
    ret = read(fifo_fd, buf, sizeof buf);
    write(1, buf, ret);
}
