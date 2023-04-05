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
typedef struct {
    uint64_t count;
    struct {
        uint64_t value;
        perf_event_id id; // PERF_FORMAT_ID
    } list[3];
} perf_event_val_t;

// Structure to hold a perf group
struct perf_handle {
    int group_fd;   // First perf_event fd that we create
    int nevents;    // Number of registered events
    perf_event_val_t events_buf; // should be staticallly allocated
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
    if (p->nevents >= ARRAY_SIZE(p->events_buf.list))
        die("surpassing the number of allowed events");
    // FIXME: Create event with perf_event_open
    struct perf_event_attr attr = {
        .type = type,
        .config = config,
        .size = sizeof(attr),
        .disabled = 1,
        .exclude_kernel = 1,
        .exclude_hv = 1,
        .read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID,
    };
    int fd = sys_perf_event_open(&attr, 0, -1, p->group_fd, 0);
    if (fd < 0)
        die("perf_event_open");
    if (p->group_fd < 0)
        p->group_fd = fd;
    // FIXME: Get perf_event_id with PERF_EVENT_IOC_ID
    __u64 id;
    if (ioctl(fd, PERF_EVENT_IOC_ID, &id) < 0)
        die("ioctl");
    p->nevents++;
    return id;
}

// Resets and starts the perf measurement
void perf_event_start(struct perf_handle *p) {
    // FIXME: PERF_EVENT_IOC_{RESET, ENABLE}
    if (ioctl(p->group_fd, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP) < 0)
        die("ioctl");
    if (ioctl(p->group_fd, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP) < 0)
        die("ioctl");
}

// Stops the perf measurement and reads out the event
void perf_event_stop(struct perf_handle *p) {
    // FIXME: PERF_EVENT_IOC_DISABLE
    // FIXME: Read event from the group_fd into an allocated buffer
    if (ioctl(p->group_fd, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP) < 0)
        die("ioctl");
    if (read(p->group_fd, &p->events_buf, sizeof(p->events_buf)) < 0)
        die("read");
    if (p->nevents < p->events_buf.count)
        die("read/overflow");
}


// After the measurement, this helper extracts the event counter for
// the given perf_event_id (which was returned by perf_event_add)
uint64_t perf_event_get(struct perf_handle *p, perf_event_id id) {
    for (unsigned i = 0; i < p->events_buf.count; i++) {
        if (p->events_buf.list[i].id == id) {
            return p->events_buf.list[i].value;
        }
    }
    die("no event");
    return -1;
}

void print_events(char *msg, uint64_t instr, uint64_t cycles, uint64_t cmisses) {
    printf("%s    %ldM instr, %ldM cycles  %.2f ipc,  %ldM cache misses\n",
        msg,
        instr / (1000 * 1000),
        cycles  / (1000 * 1000),
        instr/((double)cycles),
        cmisses / (1000 * 1000)
    );
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
    struct perf_handle ph_drepper = {
        .group_fd = -1,
    };
    struct perf_handle ph_naive = {
        .group_fd = -1,
    };

    perf_event_id inst = perf_event_add(&ph_drepper, PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS);
    perf_event_id cyc = perf_event_add(&ph_drepper, PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES);
    perf_event_id miss = perf_event_add(&ph_drepper, PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES);
    perf_event_start(&ph_drepper);
    matrixmul_drepper(dim, A, B, C0);
    perf_event_stop(&ph_drepper);
    inst = perf_event_get(&ph_drepper, inst);
    cyc = perf_event_get(&ph_drepper, cyc);
    miss = perf_event_get(&ph_drepper, miss);
    print_events("drepper", 
        inst,
        cyc,
        miss
    );

    inst = perf_event_add(&ph_naive, PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS);
    cyc = perf_event_add(&ph_naive, PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES);
    miss = perf_event_add(&ph_naive, PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES);
    perf_event_start(&ph_naive);
    matrixmul_naive(dim, A, B, C1);
    perf_event_stop(&ph_naive);
    inst = perf_event_get(&ph_naive, inst);
    cyc = perf_event_get(&ph_naive, cyc);
    miss = perf_event_get(&ph_naive, miss);
    print_events("naive", 
        inst,
        cyc,
        miss
    );


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
