#define _GNU_SOURCE
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <string.h>
#include <linux/io_uring.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdatomic.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <assert.h>


#define die(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

#define CANARY 0xdeadbeef

////////////////////////////////////////////////////////////////
// Buffer Allocator

struct buffer {
    union {
        struct {
            // If a buffer is in our free list, we use the buffer
            // memory (through the union) to have a single-linked list
            // of available buffers
            struct buffer * next;
            // In order to have a little safety guard, we place a
            // canary value here
            unsigned canary;
        };
        // The data for the buffer
        char data[4096];
    };
};

static_assert(sizeof(struct buffer) == 4096);

// We have a stack of empty buffers. At process start, the
// free-buffer stack is empty
static struct buffer *free_buffers = NULL;

// Allocate a buffer
struct buffer* alloc_buffer() {
    struct buffer *ret;
    // If the stack is empty, we allocate a new buffer and give it to
    // the caller. We use posix_memalign(3) for this, as the
    // destination buffer for read operations on an O_DIRECT file
    // descriptor have to be aligned.
    if (free_buffers == NULL) {
        if (posix_memalign((void**)&ret, 512, sizeof(struct buffer)) < 0)
            die("posix_memalign");
        return ret;
    }

    // Our free stack contains a buffer. Pop it; check the canary; and
    // return it to the user.
    ret = free_buffers;
    free_buffers = free_buffers->next; // Pop
    assert(ret->canary == CANARY); // Test the canary
    ret->canary = 0;
    return ret;
}

// Free a buffer to the free-buffer stack and set the canary value,
// which we use to detect memory corruptions. For example, if the
// kernel overwrites a buffer, while it is in the free stack (both
// free and allocated), the canary will detect that.
void free_buffer(struct buffer *b) {
    b->canary = CANARY;
    b->next   = free_buffers;
    free_buffers = b; // Push
}

////////////////////////////////////////////////////////////////
// Helpers for using I/O urings

// We have our own system call wrappers for io_uring_setup(2) and
// io_uring_enter(2) as those functions are also provided by the
// liburing. To minimize confusion, we prefixed the helpers with `sys_'
int sys_io_uring_setup(unsigned entries, struct io_uring_params *p) {
    return (int) syscall(__NR_io_uring_setup, entries, p);
}

int sys_io_uring_enter(int ring_fd, unsigned int to_submit,
                   unsigned int min_complete, unsigned int flags) {
    return (int) syscall(__NR_io_uring_enter, ring_fd, to_submit, min_complete,
                         flags, NULL, 0);
}

// Loads and stores of the head and tail pointers of an io_uring require
// memory barriers to be fully synchronized with the kernel.
//
// For a very good and detailed discussion of load-acquire and
// store-release, we strongly recommend the LWN article series on
// lockless patterns: https://lwn.net/Articles/844224/
//
// If you have not yet understood that memory is a distributed system
// that has a happens-before relation, than read(!) that series.
#define store_release(p, v)                                    \
    atomic_store_explicit((_Atomic typeof(*(p)) *)(p), (v), \
                          memory_order_release)
#define load_aquire(p)                    \
    atomic_load_explicit((_Atomic typeof(*(p)) *)(p),   \
                         memory_order_acquire)


struct ring {
    // The file descriptor to the created uring.
    int ring_fd;

    // The params that we have received from the kernel
    struct io_uring_params params;

    // We perform our own book keeping how many requests are currently
    // in flight.
    unsigned in_flight;

    // The Submission Ring (mapping 1)
    unsigned *sring;       // An array of length params.sq_entries
    unsigned *sring_tail;  // Pointer to the tail index
    unsigned  sring_mask;  // Apply this mask to (*sring_tail) to get the next free sring entry

    // The SQE array (mapping 2)
    //
    // An SQE is like a system call that you prepare in this array and
    // then push its index into the submission ring (please search for
    // the term "indirection array" in io_uring(7)
    struct io_uring_sqe *sqes;

    // The Completion Queue (mapping 3)
    unsigned *cring_head; // Pointer to the head index (written by us, read by the kernel)
    unsigned *cring_tail; // Pointer to the tail index (written by the kernel, read by us)
    unsigned  cring_mask; // Apply this mask the head index to get the next available CQE
    struct io_uring_cqe *cqes; // Array of CQEs
};

// Map io_uring into the user space. This includes:
// - Create all three mappings (submission ring, SQE array, and completion ring)
// - Derive all pointers in struct ring
struct ring ring_map(int ring_fd, struct io_uring_params p) {
    struct ring ring = {
        .ring_fd = ring_fd,
        .params = p,
        .in_flight = 0
    };
    // HINT: Use PROT_READ, PROT_WRITE, MAP_SHARED, and MAP_POPULATE
    // HINT: The three mappings use three magic mmap offsets: IORING_OFF_SQ_RING, IORING_OFF_SQES, IORING_OFF_CQ_RING
    // HINT: Dereference the {sring,cring}_mask directly
    return ring;
}

// Submit up to count random-read SQEs into the given file with a
// _single_ system call. The function returns the number of actually
// submitted random reads.
unsigned submit_random_read(struct ring *R, int fd, ssize_t fsize, unsigned count) {
    // FIXME: Prepare up to count IORING_OP_READ operations
    // HINT:  Set the sqe->user_data to the address of the used destination buffer
    // FIXME: Issue a single io_ring_enter() command to submit those SQEs
    return 0;
}

// Reap one CQE from the completion ring and copy the CQE to *cqe. If
// no CQEs are available (*cring_head == *cring_tail), this function
// returns 0.
int reap_cqe(struct ring *R, struct io_uring_cqe *cqe) {
    // FIXME: Check that the cring contains an CQE, if not return 0
    // FIXME: Extract the CQE into *cqe
    // FIXME: Forward cring_head by 1 and return 1
    return 0;
}

// This function uses reap_cqe() to extract a filled buffer from the
// uring. If wait is true, we wait for an CQE with
// io_uring_enter(min_completions=1, IORING_ENTER_GETEVENTS) if
// necessary.
struct buffer * receive_random_read(struct ring *R, bool wait) {
    // FIXME: Step 1: optimistic reap
    // FIXME: Step 2: io_uring_enter()
    // FIXME: Step 3: reap again.
    return NULL;
}

int main(int argc, char *argv[]) {
    // Argument parsing
    if (argc < 3) {
        fprintf(stderr, "usage: %s SQ_SIZE FILE\n", argv[0]);
        return -1;
    }

    int sq_size  = atoi(argv[1]);
    char *fn     = argv[2];

    // Initialize the random number generator with the current time
    // with gettimeofday(2). We use the nanoseconds within this second
    // to get some randomness as a seed for srand(3).
    struct timeval now;
    gettimeofday(&now, NULL);
    srand(now.tv_usec);

    // Open the source file and get its size
    int fd = open(fn, O_RDONLY | O_DIRECT);
    if (fd < 0) die("open");

    struct stat s;
    if (fstat(fd, &s) < 0) die("stat");
    ssize_t fsize = s.st_size;

    struct ring R = {0};
    (void) sq_size; (void) fsize;
    // FIXME: Create an io_uring with io_uring_setup(2)
    // FIXME: Map the ring with:  R = ring_map(ring_fd, params);
    
    // A per-second statistic about the performed I/O
    unsigned read_blocks = 0; // Number of read blocks
    ssize_t  read_bytes  = 0; // How many bytes where read
    while(1) {
        // FIXME: Submit SQEs up to a certain threshold (e.g. R.params.sq_entries)
        // FIXME: Reap as many CQEs as possible with waiting exactly once
        // FIXME: Keep track of the total in_flight requests (submitted - completed)

        // Every second, we output a statistic ouptu
        struct timeval now2;
        gettimeofday(&now2, NULL);
        if (now.tv_sec < now2.tv_sec) {
            printf("in_flight: %d, read_blocks/s: %.2fK, read_bytes: %.2f MiB/s\n",
                   R.in_flight, read_blocks/1000.0, read_bytes / (1024.0 * 1024.0));
            read_blocks = 0;
            read_bytes = 0;
        }
        now = now2;
    }
}
