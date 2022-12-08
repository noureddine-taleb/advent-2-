#define _GNU_SOURCE
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include <stdio.h>
#include <sys/uio.h>
#include <stdio.h>
#include <memory.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>

#define die(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)


int main() {
    // FIXME: Read in lines (HINT: getline(3))
    // FIXME: Shuffle lines (HINT: random(3))
    // FIXME: Dump lines with writev(2)
}
