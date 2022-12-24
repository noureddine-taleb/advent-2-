#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <sys/ucontext.h>
#include <sys/uio.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <stdbool.h>
#include <dlfcn.h>
#include <ctype.h>
#include <stdint.h>



#define die(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

// A ROT13 implementation
#define rot13(c) (isalpha(c)?(c&96)+1+(c-(c&96)+12)%26:c)

// Function prototype
void usyscall_init(void *offset, ssize_t length);
void usyscall_signal(int signum, siginfo_t *info, void *context);
void usyscall_enable(bool enable);

// This flag indicates to the kernel whether system calls are
// currently blocked or allowed. We start with allowed.
volatile char usyscall_flag = SYSCALL_DISPATCH_FILTER_ALLOW;

// Enable userspace systemcall dispatching, but exclude the region
// from [offset, offset+length].
void usyscall_init(void *offset, ssize_t length) {
    // FIXME: install SIGSYS signal handler with SA_SIGINFO
    // FIXME: prctl(2) and PR_SET_SYSCALL_USER_DISPATCH
}

// Just a wrapper function to enable the usyscall mechanism
void usyscall_enable(bool enable) {
    usyscall_flag = enable
        ? SYSCALL_DISPATCH_FILTER_BLOCK
        : SYSCALL_DISPATCH_FILTER_ALLOW;
}

void usyscall_signal(int signum, siginfo_t *info, void *context) {
    usyscall_enable(false);

    ucontext_t *ctx = (ucontext_t *)context;
    uint64_t args[6] = {
        ctx->uc_mcontext.gregs[REG_RDI],
        ctx->uc_mcontext.gregs[REG_RSI],
        ctx->uc_mcontext.gregs[REG_RDX],
        ctx->uc_mcontext.gregs[REG_R10],
        ctx->uc_mcontext.gregs[REG_R9],
        ctx->uc_mcontext.gregs[REG_R8]
    }; (void) args;

    // HINT: Return address can be obtained with:
    //       __builtin_extract_return_addr(__builtin_return_address (0))
    // FIXME: call usyscall_init(return_address, 20) a second time
    // FIXME: Interpret some system calls (e.g., __NR_write)

    // A return calls the rt_sigreturn system call. This has to be
    // allowed here as the (offset+length) of prctl. length=20 bytes
    // is enough for glibc.
}

int main(int argc, char **argv) {
    usyscall_init(NULL, 0);

    write(1, "Hallo Welt\n", 12);

    usyscall_enable(true);

    write(1, "Hallo Welt\n", 12);

    syscall(512, 0xdeadbeef);

    usyscall_enable(false);

    write(1, "Hallo Welt\n", 12);

    return 0;
}
