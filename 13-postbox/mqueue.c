int mqueue_prepare(int epoll_fd) {
    struct mq_attr attr = {
        .mq_maxmsg = 10,
        .mq_msgsize = 512,
    };
    int fd = mq_open("/postbox", O_CREAT|O_RDWR, S_IRWXU, &attr);
    if (fd < 0)
        die("mq_open");
    epoll_add(epoll_fd, fd, EPOLLIN);
    printf("... by mq_send: ./mq_send 4 (see also `cat /dev/mqueue/postbox`)\n");
    return fd;
}

void mqueue_handle(int epoll_fd, int msg_fd, int events) {
    if (events & EPOLLHUP)
        return;
        
    char buf[512];
    int  ret;
    ret = mq_receive(msg_fd, buf, sizeof buf, NULL);
    if (ret < 0)
        die("mq_receive");
    buf[ret] = 0;
    printf("mq: %s\n", buf);
}

