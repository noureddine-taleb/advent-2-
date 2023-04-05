#define _GNU_SOURCE
#include <stdio.h>
#include <spawn.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>

#define die(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

// This function starts a program within a PTY by setting the process'
// stdin, stdout, and stderr to the same pseudo terminal file
// descriptor. Please note that pty_fd is the open file descriptor of
// the child (e.g. /dev/pts/7)
static pid_t exec_in_pty(char *argv[], int pty_fd) {
    // We use posix_spawnp(3) to perform the process creation. For the
    // redirection of std{in,out,err}, we use a file action which
    // performs a dup(2) *after* the fork. For details, please look at
    // the man page of posix_spawnp.
    //
    //     dup2(pty_fd, STDIN_FILENO);
    posix_spawn_file_actions_t fa;
    posix_spawn_file_actions_init(&fa);
    posix_spawn_file_actions_adddup2(&fa, pty_fd, STDIN_FILENO);
    posix_spawn_file_actions_adddup2(&fa, pty_fd, STDOUT_FILENO);
    posix_spawn_file_actions_adddup2(&fa, pty_fd, STDERR_FILENO);


    // In order to allow shells within our pseudo terminal, we have to
    // spawn the new process as a new session leader. While this is an
    // interesting topic on its own, we won't cover it today.
    posix_spawnattr_t attr;
    posix_spawnattr_init(&attr);
    posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSID);

    // "magic variable": array of pointers to environment variantes.
    // This symbol is described in environ(2)
    extern char **environ;

    // We spawn the process with posix_spawnp(3). Please note the
    // spawnp searches the PATH variable and allows us to write:
    //
    // ./scribble OUT IN bash
    //
    // instead of
    //
    // ./scribble OUT IN /bin/bash
    pid_t pid;
    if (posix_spawnp(&pid, argv[0], &fa, &attr, argv,  environ) != 0)
        die("posix_spawn");

    // Cleanup everything
    posix_spawn_file_actions_destroy(&fa);
    posix_spawnattr_destroy(&attr);

    return pid;
}

// For new interactive processes to work correctly within our pty, we
// have to set a few options. As this is also quite cumbersome to find
// out, we will only look at the result here.
static struct termios orig_termios; // <- We store the old configuration

// Restore the original termios setting
void restore_terminal() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

// Configure our terminal
void configure_terminal() {
    // Credits go to:
    // https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html
    // see also termios(3)

    //  Get the original termios setting
    tcgetattr(STDIN_FILENO, &orig_termios);
    // Register an atexit(3) handler to surely restore it when the process exits
    atexit(restore_terminal);

    // We delete the flags and configure the terminal
    // ~ECHO:   Disable direct echoing of input by the terminal 
    // ~ICANON: Disable canonical mode (line buffered, line editing, etc..)
    // ~ISIG:   Disable C-c to send a signal
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG );
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

struct transfer {
    int src;
    int dst;
    int log;
};

void *transfer_data(void *transfer) {
    struct transfer *data = transfer;
    char buf[256];
    int ret;

    while (1) {
        if ((ret = read(data->src, buf, sizeof buf)) < 0)
            die("read");
        if (write(data->dst, buf, ret) < 0)
            die("write");
        if (write(data->log, buf, ret) < 0)
            die("write");
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "usage: %s OUT IN CMD [ARG ...]", argv[0]);
        return -1;
    }
    char *OUT    = argv[1];
    char *IN     = argv[2];
    char **CMD   = &argv[3];

    int in = open(IN, O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC, S_IWUSR | S_IRUSR);
    if (in < 0)
        die("open");
    int out = open(OUT, O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC, S_IWUSR | S_IRUSR);
    if (out < 0)
        die("open");
    int ptm;
    if ((ptm = posix_openpt(O_RDWR|O_CLOEXEC|O_NOCTTY)) < 0)
        die("posix_openpt");
    if (grantpt(ptm))
        die("grantpt");
    if (unlockpt(ptm))
        die("unlockpt");
    char *ptsn = ptsname(ptm);
    if (!ptsn)
        die("ptsname");
    int pts;
    if ((pts = open(ptsn, O_RDWR)) < 0)
        die("open");
    configure_terminal();

    pid_t child;
    child = exec_in_pty(CMD, pts);

    pthread_t reader, writer;
    struct transfer rtransfer = {
        .dst = 1,
        .src = ptm,
        .log = out,
    };
    struct transfer wtransfer = {
        .dst = ptm,
        .src = 1,
        .log = in,
    };
    if (pthread_create(&reader, NULL, transfer_data, &rtransfer) < 0)
        die("pthread_create");
    if (pthread_create(&writer, NULL, transfer_data, &wtransfer) < 0)
        die("pthread_create");
    if (waitpid(child, NULL, 0) < 0)
        die("waitpid");
    exit(0);
}
