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
#define MYSYSCALL 512

// Function prototype
void usyscall_init();
void usyscall_signal(int signum, siginfo_t *info, void *context);
void usyscall_enable(bool enable);

// This flag indicates to the kernel whether system calls are
// currently blocked or allowed. We start with allowed.
volatile char usyscall_flag = SYSCALL_DISPATCH_FILTER_ALLOW;

// Enable userspace systemcall dispatching, but exclude the region
// from [offset, offset+length].
void usyscall_init(void *offset, ssize_t length) {
    struct sigaction sa = {
        .sa_flags = SA_SIGINFO,
        .sa_sigaction = usyscall_signal,
    };
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGSYS, &sa, NULL) < 0)
        die("sigaction");
    if (prctl(PR_SET_SYSCALL_USER_DISPATCH, PR_SYS_DISPATCH_ON, offset, length, &usyscall_flag) < 0)
        die("prctl");
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
    greg_t *sc = &ctx->uc_mcontext.gregs[REG_RAX]; 
    uint64_t args[6] = {
        ctx->uc_mcontext.gregs[REG_RDI],
        ctx->uc_mcontext.gregs[REG_RSI],
        ctx->uc_mcontext.gregs[REG_RDX],
        ctx->uc_mcontext.gregs[REG_R10],
        ctx->uc_mcontext.gregs[REG_R9],
        ctx->uc_mcontext.gregs[REG_R8]
    }; (void) args;

    if (*sc == MYSYSCALL) {
        // hook here
        *sc = 0;
    } else {
        *sc = syscall(*sc, args[0], args[1], args[2], args[3], args[4], args[5]);
    }

    static int register_restorer = false;
    if (!register_restorer) {
        register_restorer = true;
        void *ret = __builtin_extract_return_addr(__builtin_return_address (0));

        usyscall_init(ret, 20);
    };
    usyscall_enable(true);
}

int main(int argc, char **argv) {
    usyscall_init(NULL, 0);

    write(1, "Hallo Welt1\n", 13);

    usyscall_enable(true);

    write(1, "Hallo Welt2\n", 13);

    syscall(MYSYSCALL, 0xdeadbeef);

    usyscall_enable(false);

    write(1, "Hallo Welt3\n", 13);

    return 0;
}
