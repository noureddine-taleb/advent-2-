#define _GNU_SOURCE
#include <pthread.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <stdatomic.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <stdbool.h>
#include <malloc.h>

#define die(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

// With modern glibc versions (>2.35), the glibc already registers
// a rseq area for each thread that is started with
// pthread_create. In that case, we derive this pointer from the
// thread_pointer(). For details, see
// https://www.gnu.org/software/libc/manual/html_node/Restartable-Sequences.html
#if __has_include ("sys/rseq.h")
#include <sys/rseq.h>

#if RSEQ_SIG != 0x53053053
#error "glibc defined RSEQ_SIG differently"
#endif

static struct rseq * rseq_register() {
    return __builtin_thread_pointer() + __rseq_offset;
}

#else
#include <linux/rseq.h>
#define RSEQ_SIG   0x53053053

// The rseq(2) syscall has no glibc wrapper. Therefore, we define our
// own. Please run `make man` to see the manpage rseq(2).
int sys_rseq(struct rseq * rseq, uint32_t rseq_len, int flags, uint32_t sig) {
    return syscall(SYS_rseq, rseq, rseq_len, flags, RSEQ_SIG);
}

struct rseq *rseq_register() {
    struct rseq *ret = memalign(sizeof(struct rseq), sizeof(struct rseq));
    memset(ret, 0, sizeof(struct rseq));
    ret->cpu_id_start = -1;
    ret->cpu_id       = -1;
    if (sys_rseq(ret, sizeof(struct rseq), 0, 0) < 0)
        die("rseq");
    return ret;
}

#endif



// This data structure is exactly one cache-line wide (assuming that a
// cache line is 64 bytes). Thereby, we can allocate an array of
// cpu-local counters, where each CPU only operates on a single cache
// line. Thereby, we can avoid most side effects of cache-line
// transfers.
struct cacheline {
    union {
        char data[64];
        struct {
            uint64_t        counter;
            pthread_mutex_t mutex;   // Used in the lock variant
        };
    };
};

// We will define multiple operation_t functions that all implement
// the same behavior: They increment a cpu-local counter by 1.
typedef int (*operation_t)(struct rseq *_, struct cacheline *);

// The simplest variant of a cpu-local counter is to get the cpuid
// with getcpu() and increment the counter. However, due to the
// read-update-write cycle, this variant is racy and will produce
// incorrect results.
int operation_regular(struct rseq*_, struct cacheline *counters) {
    unsigned int cpu_id;

    getcpu(&cpu_id, NULL);
    counters[cpu_id].counter += 1;

    return 0;
}

// A correct, but slow variant uses the cache-line--local pthread
// mutex to lock the counter for the time of the operation.
int operation_lock(struct rseq*_, struct cacheline *counters) {
    unsigned int cpu_id;

    getcpu(&cpu_id, NULL);

    pthread_mutex_lock(&counters[cpu_id].mutex);
    counters[cpu_id].counter += 1;
    pthread_mutex_unlock(&counters[cpu_id].mutex);

    return 0;
}

// Variant that uses getcpu() + atomic_fetch_add
int operation_atomic(struct rseq* _, struct cacheline *counters) {
    // FIXME: Implement variant

    return 0;
}

// Variant without getcpu: Like operation_atomic, but uses the
// restartable sequence to retrieve the cpu id.
// Please look at /usr/include/linux/rseq.h for the documentation of struct rseq
int operation_rseq_atomic(struct rseq* rs, struct cacheline *counters) {
    // FIXME: Implement variant

    return 0;
}


// Variant that uses no atomic operations and fully relies on rseq
// This variant is implemented in assembler (see rseq.S)
extern int operation_rseq(struct rseq *, struct cacheline*);
// FIXME: Implement counter_rseq in rseq.S


////////////////////////////////////////////////////////////////
// The Benchmarking code
//
// We start NTHREADS threads and each thread executes
// ROUNDS_PER_THREAD cpu-local increments

int  ROUNDS_PER_THREAD = 50000000;

struct thread_args {
    operation_t         operation;
    struct cacheline    *counters;
};


void* thread_handler(void* data) {
    struct thread_args *args = data;

    // Register rseq area or use glibc's rseq
    struct rseq *rseq = rseq_register();
    printf("rseq: %p\n", rseq);
    
    // Execute the given operation ROUNDS_PER_THREAD times and count
    // the number of aborts (only != 0 for rseq)
    uint64_t aborts = 0;
    for (uint64_t i = 0; i < ROUNDS_PER_THREAD; i++) {
        aborts += args->operation(rseq, args->counters);
    }

    // Return the number of rseq aborts
    return (void*) aborts;
}

// Print usage and exit.
static void usage(char *argv0) {
    fprintf(stderr, "usage: %s <threads> <regular|lock|getcpu-atomic|rseq-atomic|rseq> [rounds]\n", argv0);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    // Parameter Parsing. This is boring
    if (argc < 3) usage(argv[0]);
    if (argc == 4)
        ROUNDS_PER_THREAD *= atoi(argv[3]);

    int CPUS     = get_nprocs();
    int NTHREADS = atoi(argv[1]);
    char *MODE   = argv[2];

    struct thread_args args;
    if      (!strcmp(MODE, "rseq"))          args.operation = operation_rseq;
    else if (!strcmp(MODE, "getcpu-atomic")) args.operation = operation_atomic;
    else if (!strcmp(MODE, "rseq-atomic"))   args.operation = operation_rseq_atomic;
    else if (!strcmp(MODE, "regular"))       args.operation = operation_regular;
    else if (!strcmp(MODE, "lock"))          args.operation = operation_lock;
    else      usage(argv[0]);

    // Initialize the cpu-local counters. Each CPU gets an struct
    // cacheline on its own. We use aligned_alloc(3) to get
    // cacheline-aligned memory from the allocator

    args.counters = aligned_alloc(sizeof(struct cacheline), CPUS * sizeof(struct cacheline));
    if (!args.counters) die("calloc");


    // Initialize locks for the lock variant
    for (uint32_t i = 0; i < CPUS; i++) {
        pthread_mutex_init(&args.counters[i].mutex, NULL);
    }


    // The actual benchmarking code
    ////////////////////////////////////////////////////////////////
    struct timespec start, end;
    // Start Time. We use the CLOCK_PROCESS_CPUTIME_ID to get the
    // number of cpu seconds spent. 
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start) < 0)
        die("clock_gettime");

    // Create NTHREADS threads
    pthread_t threads[NTHREADS];
    for (uint32_t i = 0; i < NTHREADS; i++) {
        pthread_create(&threads[i], NULL, thread_handler, (void*)&args);
    }

    // Wait for all threads to complete and accumulate the number of aborts
    uint64_t aborts = 0;
    for (uint32_t i = 0; i < NTHREADS; i++) {
        uint64_t thread_aborts;
        pthread_join(threads[i], (void**)&thread_aborts);
        aborts += thread_aborts;
    }

    // End Time
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end) < 0)
        die("clock_gettime");

    // Calculate the time delta between both points in time.
    double delta = end.tv_sec - start.tv_sec;
    delta += (end.tv_nsec - start.tv_nsec) / 1e9;

    // Print out the cpu-local counters. With this output and a low
    // number of threads you can see the thread migration.
    uint64_t sum = 0;
    for (uint32_t i = 0; i < CPUS; i++) {
        fprintf(stderr, "counter[cpu=%d] = %ld\n", i, args.counters[i].counter);
        sum += args.counters[i].counter;
    }

    // Print out the result. We also check that the threads actually
    // counted correctly (state)
    printf("mode=%s threads=%d sum=%ld state=%s aborts=%ld cputime=%fs per_increment=%fns\n",
           MODE, NTHREADS,
           sum, (sum % ROUNDS_PER_THREAD) == 0 ? "ok" : "fail",
           aborts,
           delta,            // total cpu time that was spent
           delta * 1e9 / sum // nanoseconds per increment
        );
}
