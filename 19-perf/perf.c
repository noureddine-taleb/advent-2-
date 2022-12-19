#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <asm/unistd.h>
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>
#include <emmintrin.h>
#include <limits.h>


#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(*(arr)))
#define die(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

// Matrix Multiplication Functions
#include "matrix.c"

// When reading from the perf descriptor, the kernel returns an
// event record in the following format (if PERF_FORMAT_GROUP |
// PERF_FORMAT_ID are enabled).
// Example (with id0=100, id1=200): {.nr = 2, .values = {{41433, 200}, {42342314, 100}}}
typedef uint64_t perf_event_id; // For readability only
struct read_format {
    uint64_t nr;
    struct {
        uint64_t value;
        perf_event_id id; // PERF_FORMAT_ID
    } values[];
};

// Structure to hold a perf group
struct perf_handle {
    int group_fd;   // First perf_event fd that we create
    int nevents;    // Number of registered events
    size_t rf_size; // How large is the read_format buffer (derived from nevents)
    struct read_format *rf; // heap-allocated buffer for the read event
};

// Syscall wrapper for perf_event_open(2), as glibc does not have one
int sys_perf_event_open(struct perf_event_attr *attr,
                    pid_t pid, int cpu, int group_fd,
                    unsigned long flags) {
    return syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags);
}

// Add a perf event of given (type, config) with default config. If p
// is not yet initialized (p->group_fd <=0), the perf_event becomes
// the group leader. The function returns an id that can be used in
// combination with perf_event_get.
perf_event_id perf_event_add(struct perf_handle *p, int type, int config) {
    // FIXME: Create event with perf_event_open
    // FIXME: Get perf_event_id with PERF_EVENT_IOC_ID
    return -1;
}

// Resets and starts the perf measurement
void perf_event_start(struct perf_handle *p) {
    // FIXME: PERF_EVENT_IOC_{RESET, ENABLE}
}

// Stops the perf measurement and reads out the event
void perf_event_stop(struct perf_handle *p) {
    // FIXME: PERF_EVENT_IOC_DISABLE
    // FIXME: Read event from the group_fd into an allocated buffer
}


// After the measurement, this helper extracts the event counter for
// the given perf_event_id (which was returned by perf_event_add)
uint64_t perf_event_get(struct perf_handle *p, perf_event_id id) {
    return -1;
}

int main(int argc, char* argv[]) {
    unsigned dim = 32;
    if (argc > 1) {
        dim = atoi(argv[1]);
    }
    if ((dim & (dim - 1)) != 0) {
        fprintf(stderr, "Given dimension must be a power of two\n");
        exit(EXIT_FAILURE);
    }

    // Create some matrices
    double *A  = create_random_matrix(dim);
    double *B  = create_random_matrix(dim);
    double *C0 = create_matrix(dim);
    double *C1 = create_matrix(dim);

    size_t msize = sizeof(double) * dim * dim;
    printf("matrix_size: %.2f MiB\n", msize / (1024.0 * 1024.0));

    // We provide you with two matrix multiply implementations (see
    // matrix.c). The optimized variant was created by Ulrich Drepper
    // and is optimized for cache usage. For a detailed discussion,
    // we refer you to
    //
    // "What Every Programmer Should Know About Memory", Ulrich Drepper, 2007,
    // https://www.akkadia.org/drepper/cpumemory.pdf

    matrixmul_drepper(dim, A, B, C0);
    matrixmul_naive(dim, A, B, C1);

    // Sanity Checking: are both result matrices equal (with a margin of 0.1%) ?
    for (unsigned i = 0; i < (dim*dim); i++) {
        double delta = 1.0 - C1[i]/C0[i];
        if (delta > 0.001 || delta < -0.001) {
            fprintf(stderr, "mismatch at %d: %f%%\n", i, delta * 100);
            return -1;
        }
    }
    return 0;
}
