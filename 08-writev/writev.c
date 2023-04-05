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

int cmp(const struct iovec *a, const struct iovec *b) {
    return strcmp(a->iov_base, b->iov_base);
}

#define IOV_BUF(iov) ((char *)iov.iov_base)
int main() {
    struct iovec iov[512];
    int i;
    ssize_t len;
    for (
        i = 0;
            i < 512 
            && (len = getline((char **)&(iov[i].iov_base), &iov[i].iov_len, stdin)) > 0 
            && (IOV_BUF(iov[i])[len] = 0, 1);
        i++
    );
    qsort(iov, i, sizeof iov[0], (comparison_fn_t)cmp);
    writev(1, iov, i);
}
