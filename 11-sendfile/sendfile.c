#define _GNU_SOURCE
#include <stdio.h>
#include <time.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <assert.h>
#include <limits.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define die(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)
ssize_t in_size;

ssize_t copy_write(int fd_in, int fd_out, int *syscalls) {
    *syscalls = 0;
    size_t copied = 0;
    char buf[128 * 1024];
    int ret;
    while (copied < in_size) {
        ret = read(fd_in, buf, sizeof buf);
        ret = write(fd_out, buf, ret);
        copied += ret;
        *syscalls += 2;
    }
    return copied;
}

ssize_t copy_sendfile(int fd_in, int fd_out, int *syscalls) {
    *syscalls = 0;
    size_t copied = 0;
    while (copied < in_size) {
        copied += sendfile(fd_out, fd_in, NULL, in_size);
        *syscalls += 1;
    }
    return copied;
}

// This function measures the given copy implementation.
// fd_in:  file descriptor to copy from
// fd_out: file descriptor to copy to
// banner: Just a nice string to print the output
// copy:   The copy implementation
// returns the number of bytes per second
double measure(int fd_in, int fd_out, char *banner, ssize_t (*copy)(int, int, int*)) {
    // First, we reset our file descriptors.
    // fd_in:  seek to position zero
    // fd_out: truncate the file to zero bytes.
    if (lseek(fd_in, 0, SEEK_SET) < 0) die("lseek");
    if (ftruncate(fd_out, 0) < 0)      die("ftruncate");
    
    // Measure the start time.
    struct timespec start, end;
    if (clock_gettime(CLOCK_REALTIME, &start) < 0)
        die("clock_gettime");

    // Perform the actual copy. We give the copy function also a
    // pointer to syscalls, where the implementation should count the
    // number of issued system calls.
    int syscalls;
    ssize_t bytes = copy(fd_in, fd_out, &syscalls);


    // Measure the end time
    if (clock_gettime(CLOCK_REALTIME, &end) < 0)
        die("clock_gettime");

    // Calculate the time delta between both points in time.
    double delta = end.tv_sec - start.tv_sec;
    delta += (end.tv_nsec - start.tv_nsec) / 1e9;

    double speed = (bytes /delta) / 1024.0 / 1024.0;
    // Print out some nicely formatted message
    printf("[%10s] copied with %.2f MiB/s (in %.2f s, %d syscalls)\n",
           banner, speed, delta, syscalls);

    return speed;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s FILE\n", argv[0]);
        return -1;
    }
    // As input file, we open the user-specified file from the command
    // line as a read only file.
    int fd_in = open(argv[1], O_RDONLY);
    if (fd_in < 0) die("open");

    // As an output, we create an anonymous in-memory file. By using
    // such an in-memory file, do not measure the influence of on-disk
    // file systems.
    int fd_out = memfd_create("target", 0);

    // We will run for ten rounds, unless the user specified something
    // else in the ROUNDS environment variable.
    char *ROUNDS = getenv("ROUNDS");
    int rounds = atoi(ROUNDS ? ROUNDS : "10");

    // We run the copy_write algorithm once to warm up the buffer
    // cache. With this, the input file should now, if small enough,
    // reside in in the buffer cache.
    int dummy;
    in_size = lseek(fd_in, 0, SEEK_END);
    lseek(fd_in, 0, SEEK_SET);
    copy_write(fd_in, fd_out, &dummy);

    // The actual measurement
    double sendfile = 0, write = 0;
    for (int i = 0; i < rounds; i++) {
        sendfile += measure(fd_in, fd_out, "sendfile", copy_sendfile);
        write    += measure(fd_in, fd_out, "read/write", copy_write);
    }

    // Print the average MiB/s for both copy algorithms
    sendfile /= rounds;
    write /= rounds;
    printf("sendfile: %.2f MiB/s(📈%.2f%%), read/write: %.2f MiB/s\n",
           sendfile,
           (sendfile - write) / write * 100,
           write
        );
}
