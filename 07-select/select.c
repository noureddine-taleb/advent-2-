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
#include <sys/wait.h>

#define die(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

/* For each filter process, we will generate a proc object */
struct proc {
    char *cmd;  // command line
    pid_t pid;  // process id of running process. 0 if exited
    int stdin;  // stdin file descriptor of process (pipe)
    int stdout; // stdout file descriptor of process

    // For the output, we save the last char that was printed by this
    // process. We use this to prefix all lines with a banner a la
    // "[CMD]".
    char last_char;
};

static int nprocs;         // Number of started filter processes
static struct proc *procs; // Dynamically-allocated array of procs

// This function starts the filter (proc->cmd) as a new child process
// and connects its stdin and stdout via pipes (proc->{stdin,stdout})
// to the parent process.
//
// We also start the process wrapped by stdbuf(1) to force
// line-buffered stdio for a more interactive experience on the terminal
static int start_proc(struct proc *proc) {
    // We build an array for execv that uses the shell to execute the
    // given command. Furthermore, we use the stdbuf tool to start the
    // filter with line-buffered output.

    //
    // HINT: We use the glibc asprintf(3), as I'm too lazy doing the
    //       strlen()+malloc()+snprintf() myself.
    //       Therefore we have to define _GNU_SOURCE at the top.
    char *stdbuf_cmd;
    asprintf(&stdbuf_cmd, "stdbuf -oL %s", proc->cmd);
    char *argv[] = {"sh", "-c", stdbuf_cmd, 0 };

    // We create two pipe pairs, where [0] is the reading end
    // and [1] the writing end of the pair. We also set the O_CLOEXEC
    // flag to close both descriptors when the child process is exec'ed.
    int stdin[2], stdout[2];
    if (pipe2(stdin,  O_CLOEXEC)) return -1;
    if (pipe2(stdout, O_CLOEXEC)) return -1;

    // For starting the filter, we use posix_spawn, which gives us an
    // interface around fork+exec to perform standard process
    // spawning. We use a filter action to copy our pipe descriptors to
    // the stdin (0) and stdout (1) handles within the child.
    // Internally, posix_spawn will do a dup2(2). For example,
    //
    //     dup2(stdin[0], STDIN_FILENO);
    posix_spawn_file_actions_t fa;
    posix_spawn_file_actions_init(&fa);
    posix_spawn_file_actions_adddup2(&fa, stdin[0],  STDIN_FILENO);
    posix_spawn_file_actions_adddup2(&fa, stdout[1], STDOUT_FILENO);

    // "magic variable": array of pointers to environment variables.
    // This symbol is described in environ(2)
    extern char **environ;

    // We spawn the filter process.
    int e;
    if (!(e = posix_spawn(&proc->pid, "/bin/sh", &fa, 0, argv,  environ))) {
        // On success, we free the allocated memory.
        posix_spawn_file_actions_destroy(&fa);
        free(stdbuf_cmd);

        // We are within the parent process. Therefore, we copy our
        // pipe ends to the proc object and close the ends that are
        // also used within the child (to save file descriptors)

        // stdin of filter
        proc->stdin = stdin[1]; // write end
        close(stdin[0]);        // read end

        // stdout of filter
        proc->stdout = stdout[0]; // read end
        close(stdout[1]);         // write end

        return 0;
	} else {
        // posix_spawn failed.
        errno = e;
        free(stdbuf_cmd);
        return -1;
    }
}



int main(int argc, char *argv[]) {
    if (argc <= 1) {
        fprintf(stderr, "usage: %s [CMD-1] (<CMD-2> <CMD-3> ...)", argv[0]);
        return -1;
    }

    // We allocate an array of proc objects
    nprocs = argc - 1;
    procs   = malloc(nprocs * sizeof(struct proc));
    if (!procs) die("malloc");

    // Initialize proc objects and start the filter
    for (int i = 0; i < nprocs; i++) {
        procs[i].cmd  = argv[i+1];
        procs[i].last_char = '\n';
        int rc = start_proc(&procs[i]);
        if (rc < 0) die("start_filter");

        fprintf(stderr, "[%s] Started filter as pid %d\n", procs[i].cmd, procs[i].pid);
    }

    // FIXME: Read from stdin and push data to the proc[*].stdin
    // FIXME: Use select(2)
    // FIXME: Read from proc[*].stdout and push data to stdout
}
