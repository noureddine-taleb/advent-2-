#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/ptrace.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/eventfd.h>
#include <assert.h>
#include <signal.h>
#include <string.h>

#define die(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

// A table of system call names and argument counts
// This is only valid on x64 (AMD64)
#include "table.c"

// Print information about a system call (as strace also does it),
// which we got from PTRACE_GET_SYSCALL_INFO. This function also uses
// the system call table to print pretty syscall names.
void print_syscall(struct ptrace_syscall_info *info) {
    if (info->op == PTRACE_SYSCALL_INFO_EXIT)
        return;

    struct syscall_name *sc = &names[info->entry.nr];
    printf("%s(", sc->name);
    for (int i=0; i < sc->argc; i++)
        printf("%#llx, ", info->entry.args[i]);
    printf(")\n");
}

pid_t pid;

void start_proc(char **cmd) {
    pid = fork();
    
    if (pid < 0)
        die("fork");

    if (pid > 0) {
        // parent
        int wstatus;
        if (waitpid(pid, &wstatus, 0) < 0)
            die("waitpid");

        if (WIFEXITED(wstatus))
            exit(WEXITSTATUS(wstatus));

        ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_TRACESYSGOOD);
        return;
    }

    if (ptrace(PTRACE_TRACEME) < 0)
        die("ptrace");

    execvp(*cmd, cmd);
    die("execvp");
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        fprintf(stderr, "usage: %s CMD [ARGS...]\n", argv[0]);
        return -1;
    }
    start_proc(&argv[1]);
    while(1) {
        ptrace(PTRACE_SYSCALL, pid, 0, 0);
        int wstatus;
        if (waitpid(pid, &wstatus, 0) < 0)
            die("waitpid");
        
        if (WIFEXITED(wstatus))
            exit(WEXITSTATUS(wstatus));
        struct ptrace_syscall_info i;
        ptrace(PTRACE_GET_SYSCALL_INFO, pid, sizeof i, &i);
        print_syscall(&i);
    }
}
