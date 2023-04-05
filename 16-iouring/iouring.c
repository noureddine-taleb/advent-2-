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

    int size;
    // A per-second statistic about the performed I/O
    unsigned read_blocks; // Number of read blocks
    ssize_t  read_bytes; // How many bytes where read

    // The params that we have received from the kernel
    struct io_uring_params params;

    // We perform our own book keeping how many requests are currently
    // in flight.
    unsigned in_flight;

    // The Submission Ring (mapping 1)
    unsigned *sring;       // An array of length params.sq_entries
    unsigned *sring_tail;  // Pointer to the tail index
    unsigned  sring_mask;  // Apply this mask to (*sring_tail) to get the next free sring entry
    unsigned *sring_array;
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
} ring = {0};

// Submit up to count random-read SQEs into the given file with a
// _single_ system call. The function returns the number of actually
// submitted random reads.
unsigned submit_random_read(int fd, ssize_t fsize, unsigned count) {
    if (count > ring.size)
        count = ring.size;
    
    int tail = load_aquire(ring.sring_tail);
    for (int i=0; i < count; i++) {
        int index = tail & ring.sring_mask;
        struct io_uring_sqe *sqe = &ring.sqes[index];
        memset(sqe, 0, sizeof(*sqe));
        sqe->opcode = IORING_OP_READ;
        sqe->fd = fd;
        struct buffer *buf = alloc_buffer(); 
        sqe->user_data = (unsigned long)buf;
        sqe->addr = (unsigned long)buf->data;
        sqe->len = sizeof (free_buffers->data);
        sqe->off = rand() % fsize;

        ring.sring_array[index] = index;
        tail++;
    }

    store_release(ring.sring_tail, tail);
    if (sys_io_uring_enter(ring.ring_fd, count, 0, 0) != count)
        die("io_uring_enter: submit");
    ring.in_flight += count;
    return count;
}

// Reap one CQE from the completion ring and copy the CQE to *cqe. If
// no CQEs are available (*cring_head == *cring_tail), this function
// returns 0.
int reap_cqe(struct io_uring_cqe *cqe) {
    int _head = load_aquire(ring.cring_head);
    int _tail = load_aquire(ring.cring_tail);
    int head = _head & ring.cring_mask, tail = _tail & ring.cring_mask;
    if (head == tail)
        return 0;
    memmove(cqe, &ring.cqes[head], sizeof *cqe);
    _head++;
    store_release(ring.cring_head, _head);
    if (cqe->res < 0)
        die("iouring read");
    // ((struct buffer *)cqe->user_data)->data[cqe->res] = 0;
    return 1;
}

// This function uses reap_cqe() to extract a filled buffer from the
// uring. If wait is true, we wait for an CQE with
// io_uring_enter(min_completions=1, IORING_ENTER_GETEVENTS) if
// necessary.
struct buffer * receive_random_read(bool wait) {
    struct io_uring_cqe cqe;
try:
    if (reap_cqe(&cqe))
        goto success;
    else if (wait) {
        if (sys_io_uring_enter(ring.ring_fd, 0, 1, IORING_ENTER_GETEVENTS) < 0)
            die("sys_io_uring_enter: wait");
        wait = 0;
        goto try;
    }
    return NULL;
success:
    ring.read_bytes += cqe.res;
    ring.read_blocks++;
    ring.in_flight--;
    return  (struct buffer *)cqe.user_data;
}

void setup_uring() {
    char *sq_ptr, *cq_ptr;
    struct io_uring_params *p = &ring.params;

    /* See io_uring_setup(2) for io_uring_params.flags you can set */
    memset(p, 0, sizeof(ring.params));
    ring.ring_fd = sys_io_uring_setup(ring.size, p);
    if (ring.ring_fd < 0)
        die("io_uring_setup");

    /*
    * io_uring communication happens via 2 shared kernel-user space ring
    * buffers, which can be jointly mapped with a single mmap() call in
    * kernels >= 5.4.
    */

    int sring_sz = p->sq_off.array + p->sq_entries * sizeof(unsigned);
    int cring_sz = p->cq_off.cqes + p->cq_entries * sizeof(struct io_uring_cqe);

    /* Rather than check for kernel version, the recommended way is to
    * check the features field of the io_uring_params structure, which is a
    * bitmask. If IORING_FEAT_SINGLE_MMAP is not set, we can do away with the
    * second mmap() call to map in the completion ring separately.
    */
    if (p->features & IORING_FEAT_SINGLE_MMAP) {
        if (cring_sz > sring_sz)
            sring_sz = cring_sz;
        cring_sz = sring_sz;
    }

    /* Map in the submission and completion queue ring buffers.
    *  Kernels < 5.4 only map in the submission queue, though.
    */
    sq_ptr = mmap(0, sring_sz, PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_POPULATE,
                    ring.ring_fd, IORING_OFF_SQ_RING);
    if (sq_ptr == MAP_FAILED)
        die("mmap");

    if (p->features & IORING_FEAT_SINGLE_MMAP) {
        cq_ptr = sq_ptr;
    } else {
        /* Map in the completion queue ring buffer in older kernels separately */
        cq_ptr = mmap(0, cring_sz, PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_POPULATE,
                        ring.ring_fd, IORING_OFF_CQ_RING);
        if (cq_ptr == MAP_FAILED)
            die("mmap");
    }
    /* Save useful fields for later easy reference */
    ring.sring_tail = (void *)(sq_ptr + p->sq_off.tail);
    ring.sring_mask = *(unsigned int *)(sq_ptr + p->sq_off.ring_mask);
    ring.sring_array = (void *)(sq_ptr + p->sq_off.array);

    /* Map in the submission queue entries array */
    ring.sqes = mmap(0, p->sq_entries * sizeof(struct io_uring_sqe),
                    PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE,
                    ring.ring_fd, IORING_OFF_SQES);
    if (ring.sqes == MAP_FAILED)
        die("mmap");
    /* Save useful fields for later easy reference */
    ring.cring_head = (void *)(cq_ptr + p->cq_off.head);
    ring.cring_tail = (void *)(cq_ptr + p->cq_off.tail);
    ring.cring_mask = *(unsigned int *)(cq_ptr + p->cq_off.ring_mask);
    ring.cqes = (void *)(cq_ptr + p->cq_off.cqes);
}

int main(int argc, char *argv[]) {
    // Argument parsing
    if (argc < 3) {
        fprintf(stderr, "usage: %s SQ_SIZE FILE\n", argv[0]);
        return -1;
    }

    ring.size  = atoi(argv[1]);
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

    setup_uring();
    
    struct buffer *buf;
    while(1) {
        submit_random_read(fd, fsize, ring.size - ring.in_flight);
        int wait = true;
        while((buf = receive_random_read(wait))) {
            wait = false;
            free_buffer(buf);
        }
        // Every second, we output a statistic ouptu
        struct timeval now2;
        gettimeofday(&now2, NULL);
        if (now.tv_sec < now2.tv_sec) {
            time_t diff = now2.tv_sec - now.tv_sec;
            printf("in_flight: %d, read_blocks/s: %ldK, read_bytes: %ld MiB/s\n",
                   ring.in_flight, ring.read_blocks/1000 / diff, ring.read_bytes / (1024 * 1024) / diff);
            ring.read_blocks = 0;
            ring.read_bytes = 0;
        }
        now = now2;
    }
}
