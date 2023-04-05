#include <sys/types.h>
#include <stdatomic.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <linux/futex.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <stdbool.h>

////////////////////////////////////////////////////////////////
// Layer 0: Futex System Call Helpers
////////////////////////////////////////////////////////////////

/* We provide you with a few wrapper function to invoke the described
   futex system call as it is not directly exposed by the GNU C
   library.

   We use the atomic_int type here as it is 32-bit wide on all
   platforms of interest.

*/
int futex(atomic_int *addr, int op, uint32_t val,
          struct timespec *ts, uint32_t *uaddr2, uint32_t val3) {
    return syscall(SYS_futex, addr, op, val, ts, uaddr2, val3);
}

int futex_wake(atomic_int *addr, int nr) {
    return futex(addr, FUTEX_WAKE, nr, NULL, NULL, 0);
}

int futex_wait(atomic_int *addr, int val) {
    return futex(addr, FUTEX_WAIT, val, NULL, NULL, 0);
}

////////////////////////////////////////////////////////////////
// Layer 1: Semaphore Abstraction
////////////////////////////////////////////////////////////////

/* Initialize the semaphore. This boils down to setting the referenced
   32-bit word with the given initval */
void sem_init(atomic_int *sem, unsigned initval) {
    atomic_init(sem, initval);
}

/* The semaphores decrement operation tries to decrement the given
   semaphore. If the semaphore counter is larger than zero, we just
   decrement it. If it is already zero, we sleep until the value
   becomes larger than zero and try decrementing it again. */
void sem_down(atomic_int *sem) {
    while (1) {
        atomic_int exp = 1;
        if (atomic_compare_exchange_strong(sem, &exp, 0))
            return ;
        futex_wait(sem, 0);
    }
}

/* The semaphore increment operation increments the counter and wakes
   up one waiting thread, if there is the possibility of waiting
   threads. */
void sem_up(atomic_int *sem) {
    atomic_store(sem, 1);
    futex_wake(sem, 1);
}

////////////////////////////////////////////////////////////////
// Layer 2: Semaphore-Synchronized Bounded Buffer
////////////////////////////////////////////////////////////////

// Calculate the number of elements in a statically allocated array.
#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(*(arr)))

struct bounded_buffer {
    // We use two semaphores to count the number of empty slots and
    // the number of valid elements in our bounded buffer. Initialize
    // these values accordingly.
    atomic_int slots;
    atomic_int elements;

    // We use another semaphore as a binary semaphore to synchronize
    // the access to the data and meta-data of our bounded buffer
    atomic_int lock;
    unsigned int read_idx;  // Next slot to read
    unsigned int write_idx; // Next slot to write

    // We have place for three pointers in our bounded buffer.
    void * data[3];
};

void bb_init(struct bounded_buffer *bb) {
    memset(bb, 0, sizeof(*bb));
    bb->lock = 1;
    bb->slots = ARRAY_SIZE(bb->data);
}

void * bb_get(struct bounded_buffer *bb) {
    futex_wait(&bb->elements, 0);
    sem_down(&bb->lock);
    void *ret = bb->data[bb->read_idx++];
    bb->elements--;
    bb->slots++;
    bb->read_idx = bb->read_idx % 3;
    futex_wake(&bb->slots, 1);
    sem_up(&bb->lock);
    return ret;
}

void bb_put(struct bounded_buffer *bb, void *data) {
    futex_wait(&bb->slots, 0);
    sem_down(&bb->lock);
    bb->data[bb->write_idx++] = data;
    bb->elements++;
    bb->slots--;
    bb->write_idx = bb->write_idx % 3;
    futex_wake(&bb->elements, 1);
    sem_up(&bb->lock);
}


int main() {
    // First, we use mmap to establish a piece of memory that is
    // shared between the parent and the child process. The mapping is
    // 4096 bytes large a resides at the same address in the parent and the child process.
    char *shared_memory = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                               MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    if (shared_memory == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // We place a semaphore and a bounded buffer instance in our shared memory.
    atomic_int *semaphore     = (void *) &shared_memory[0];
    struct bounded_buffer *bb = (void *) &shared_memory[sizeof(atomic_int)];

    // We use this semaphore as a condition variable. The parent
    // process uses sem_down(), which will initially result in
    // sleeping, until the child has initialized the bounded buffer
    // and signals this by sem_up(semaphore).
    sem_init(semaphore, 0);

    pid_t child = fork();
    if (child == -1) {
        perror("fork");
        return -1;
    }

    if (child != 0) {
        ////////////////////////////////////////////////////////////////
        // Parent

        // Wait until the child has initialized the bounded buffer
        sem_down(semaphore);

        printf("Child has initialized the bounded buffer\n");

        while (1) {
            char *data = bb_get(bb);
            printf("%s\n", data);
        }
    } else {
        ////////////////////////////////////////////////////////////////
        // Child
        char *data[] = {
            "Hello", "World", "!", "How", "are", "you", "?"
        };
        bb_init(bb);
        sem_up(semaphore);
        for (int i=0; i < ARRAY_SIZE(data); i++) {
            bb_put(bb, data[i]);
        }
    }

    return 0;
}

