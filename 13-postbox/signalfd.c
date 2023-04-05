int signalfd_prepare(int epoll_fd) {
    sigset_t mask;
    int fd;

    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    sigprocmask(SIG_BLOCK, &mask, NULL);
    if ((fd = signalfd(-1, &mask, 0)) < 0)
        die("signalfd");
    epoll_add(epoll_fd, fd, EPOLLIN);
    printf("... by signal: /bin/kill -USR1 -q 3 %d \n", getpid());
    return fd;
}

void signalfd_handle(int epoll_fd, int signal_fd, int events) {
    if (events & EPOLLHUP)
        return;
        
    struct signalfd_siginfo buf;
    int  ret;
    char banner[] = "signalfd: ";
    write(1, banner, sizeof banner);
    if ((ret = read(signal_fd, &buf, sizeof buf)) < 0)
        die("signalfd: read");
    printf("[uid=%d,pid=%d] signal=%d, data=%d\n", buf.ssi_uid, buf.ssi_pid, buf.ssi_signo, buf.ssi_int);
}
