#define _GNU_SOURCE
#include <stdarg.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <sys/fcntl.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <linux/audit.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdbool.h>

#define die(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

// For seccomp(2) and close_range(2), we require own system call
// wrappers as glibc does not provide them.

// close_range(2) - close all file descriptors between minfd and maxfd
#ifndef __NR_close_range
#define __NR_close_range 436
#endif
static int sys_close_range(unsigned int minfd, unsigned int maxfd, int flags) {
    return syscall(__NR_close_range, minfd, maxfd, 0);
}
// seccomp(2) - manipulate the seccomp filter of the calling process
static int sys_seccomp(unsigned int operation, unsigned int flags, void* args) {
    return syscall(__NR_seccomp, operation, flags, args);
}

// For call_secure, a process consists of a pid (as we spawn
// the function in another process and pipe connection.
typedef struct {
    pid_t pid; // pid of the child process
    int pipe;  // read end of a pipe that we share with the child process
} secure_func_t ;

// Spawn a function within a seccomp-restricted child process
secure_func_t spawn_secure(void (*func)(void*, int), void* arg) {
    // FIXME: Create a pipe pair with pipe2(2)
    int p[2];
    if (pipe2(p, 0))
        die("pipe2");
    int ret = fork();
    if (ret < 0)
        die("exit");
    if (ret > 0) {
        secure_func_t r = {
            .pid = ret,
            .pipe = p[0],
        };
        close(p[1]);
        return r;
    }
    dup2(p[1], 0);
    sys_close_range(1, INT32_MAX, 0);
    sys_seccomp(SECCOMP_SET_MODE_STRICT, 0, NULL);
    func(arg, 0);
    syscall(__NR_exit, 0);
    __builtin_unreachable();
}

// Complete a previously spawned function and read at most bulen bytes
// into buf. The function returns -1 on error or the number of
// actually read bytes.
int complete_secure(secure_func_t f, char *buf, size_t buflen) {
    int ret;
    ret = read(f.pipe, buf, buflen);
    int status;
    if (waitpid(f.pid, &status, 0) < 0)
        die("waitpid");
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
        return  ret;
    return WIFEXITED(status) ? -WEXITSTATUS(status) : -WTERMSIG(status);
}

// Test function that is valid
void ok(void *arg, int fd) {
    write(fd, "Halo", 5);
}

// test function that is quite bad
void fail(void *arg, int fd) {
    int fd2 = open("/etc/passwd", O_RDWR);
    close(fd2);
}

int main(int argc, char*argv[]) {
    char buf[128];
    int len;
    secure_func_t p1 = spawn_secure(ok, NULL);
    if ((len = complete_secure(p1, buf, sizeof(buf))) >= 0) {
        buf[len] = 0;
        printf("ok: %s\n", buf);
    } else {
        printf("ok failed: %d\n", len);
    }

    secure_func_t p2 = spawn_secure(fail, NULL);
    if ((len = complete_secure(p2, buf, sizeof(buf))) >= 0) {
        buf[len] = 0;
        printf("fail: %s\n", buf);
    } else {
        printf("fail failed: %d\n", len);
    }

}

