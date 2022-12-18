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
#include <string.h>

#define die(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

// A table of system call names and argument counts
// This is only valid on x64 (AMD64)
#include "table.c"

// Print information about a system call (as strace also does it),
// which we got from PTRACE_GET_SYSCALL_INFO. This function also uses
// the system call table to print pretty syscall names.
void print_syscall(struct ptrace_syscall_info *info) {
    printf("%s\n", names[__NR_write].name);
    // FIXME: Handle info->op == PTRACE_SYSCALL_INFO_ENTRY
    // FIXME: Handle info->op == PTRACE_SYSCALL_INFO_EXIT
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        fprintf(stderr, "usage: %s CMD [ARGS...]\n", argv[0]);
        return -1;
    }
    // FIXME/child: Issue PTRACE_TRACEME
    // FIXME/child: execvp(argv[1], &argv[1])
    // FIXME/parent: Wait for the first SIGTRAP
    // FIXME/parent: set PTRACE_O_TRACESYSGOOD option
    // FIXME/parent: PTRACE_SYSCALL to the next syscall
    // FIXME/parent: Use PTRACE_GET_SYSCALL_INFO to get details
}
