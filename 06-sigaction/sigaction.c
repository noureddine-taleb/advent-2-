#undef _GNU_SOURCE
#define _GNU_SOURCE
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/mman.h>
#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/ucontext.h>

int PAGE_SIZE;

extern int main(void);

/* See Day 2: clone.
 *
 * Example: syscall_write("foobar = ", 23);
 */
int syscall_write(char *msg, int64_t number, char base) {
    write(1, msg, strlen(msg));

    char buffer[sizeof(number) * 8];
    char *p = &buffer[sizeof(number) * 8];
    int len = 1;
    *(--p) = '\n';
    if (number < 0) {
        write(1, "-", 1);
        number *= -1;
    }
    do {
        *(--p) =  "0123456789abcdef"[number % base];
        number /= base;
        len ++;
    } while (number != 0);
    write(1, p, len);
    return 0;
}


/* We have three different fault handlers to make our program
 * nearly "immortal":
 *
 * 1. sa_sigint:  Is invoked on Control-C.
 * 2. sa_sigsegv: Handle segmentation faults
 * 3. sa_sigill:  Jump over illegal instructions
*/

volatile bool do_exit = false;

int main(void) {
    // We get the actual page-size for this system. On x86, this
    // always return 4096, as this is the size of regular pages on
    // this architecture. We need this in the SIGSEGV handler.
    PAGE_SIZE = sysconf(_SC_PAGESIZE);


    // We generate an invalid pointer that points _somewhere_! This is
    // undefined behavior, and we only hope for the best here. Perhaps
    // we should install a signal handler for SIGSEGV beforehand....
    uint32_t * addr = (uint32_t*)0xdeadbeef;

    // This will provoke a SIGSEGV
    *addr = 23;

    // Two ud2 instructions are exactly 4 bytes long
#define INVALID_OPCODE_32_BIT() asm("ud2; ud2;")

    // This will provoke a SIGILL
    INVALID_OPCODE_32_BIT();

    // Happy faulting, until someone sets the do_exit variable.
    // Perhaps the SIGINT handler?
    while(!do_exit) {
        sleep(1);
        addr += 22559;
        *addr = 42;
        INVALID_OPCODE_32_BIT();
    }

    { // Like in the mmap exercise, we use pmap to show our own memory
      // map, before exiting.
        char cmd[256];
        snprintf(cmd, 256, "pmap %d", getpid());
        printf("---- system(\"%s\"):\n", cmd);
        system(cmd);
    }

    return 0;
}
