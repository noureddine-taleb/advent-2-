#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/xattr.h>
#include <stdint.h>

#define die(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

// An helper function that maps a file to memory and returns its
// length in *len and an open file descriptor in *fd.
// On error, the function should return the null pointer;
char * map_file(char *fn, ssize_t *len, int *fd) {
    // FIXME: Map file to memory
    // FIXME: Set *len = ...
    // FIXME: Set *fd = ...
    return NULL;
}

// A (very) simple checksum function that calculates and additive checksum over a memory range.
uint64_t calc_checksum(void *data, size_t len) {
    uint64_t checksum = 0;

    // First sum as many bytes as uint64_t as possible
    uint64_t *ptr = (uint64_t*)data;
    while ((void *)ptr < (data + len)) {
        checksum += *ptr++;
    }

    // The rest (0-7 bytes) are added byte wise.
    char *cptr = (char*)ptr;
    while ((void*)cptr < (data+len)) {
        checksum += *cptr;
    }

    return checksum;
}

int main(int argc, char *argv[]) {
    // The name of the extended attribute where we store our checksum
    const char *xattr = "user.checksum";

    // Argument filename
    char *fn;

    // Should we reset the checksum?
    bool reset_checksum;

    // Argument parsing
    if (argc == 3 && strcmp(argv[1], "-r") == 0) {
        reset_checksum = true;
        fn = argv[2];
    } else if (argc == 2) {
        reset_checksum = true;
        fn = argv[1];
    } else {
        fprintf(stderr, "usage: %s [-r] <FILE>\n", argv[0]);
    }
    // Avoid compiler warnings
    (void) fn; (void) reset_checksum; (void) xattr;
    // FIXME: map the file
    // FIXME: reset the checksum if requested
    // FIXME: calculate the checksum
    // FIXME: get the old checksum and perform checking
    // FIXME: set the new checksum
}
