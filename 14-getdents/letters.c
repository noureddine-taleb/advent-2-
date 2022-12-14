#define _GNU_SOURCE
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>


#define die(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

// To filter out files that were already present and read for last
// Christmas, we have to know the timestamp of Christmas. We derive
// that timestamp from the current time.
time_t christmas_day(int delta_year) {
    // FIXME: calculate timestamp for Christmas.
    // delta_years = 0  => this Christmas
    // delta_years = -1 => last Christmas
    return time(NULL);
}

// For us, a letter consists of a timestamp and a pointer to a
// filename.
struct letter {
    time_t timestamp;
    char  *filename;
};




int main(int argc, char *argv[]) {
    // FIXME: Open the directory with open(2) and O_DIRECTORY
    // FIXME: Read directory entries with getdents64(2)
    // FIXME: For each entry, use statx(2) to extract the birthtime
    // FIXME: Sort by birthtime with qsort(3)
    // FIXME: Output letters that were created after the last christmas
    //        and before the next christmas in the following format
    //        (151 until the next Christmas).

    // $ ./letters
    // 151 days: Makefile
    // 151 days: letters
}
