#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

char buf[4096];
int main(int argc, char *argv[]) {
    // For cat, we have to iterate over all command-line arguments of
    // our process. Thereby, argv[0] is our program binary itself ("./cat").
    int idx, ret, fd;
    for (idx = 1; idx < argc; idx++) {
        fd = open(argv[idx], O_RDONLY);
        if (fd < 0) {
            fprintf(stderr, "no such file: %s\n", argv[idx]);
            continue;
        }
        while ((ret = read(fd, buf, sizeof buf)) > 0) {
            write(1, buf, ret);
        }
        close(fd);
    }

    return 0;
}
