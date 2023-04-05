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
    // delta_years = 0  => this Christmas
    // delta_years = -1 => last Christmas
    time_t t;
    time_t year_time = 60 * 60 * 24 * 365;
    time(&t);
    t += year_time * delta_year;
    t = t / year_time * year_time + year_time - 1;
    return t;
}
struct linux_dirent64 {
    ino64_t        d_ino;    /* 64-bit inode number */
    off64_t        d_off;    /* 64-bit offset to next structure */
    unsigned short d_reclen; /* Size of this dirent */
    unsigned char  d_type;   /* File type */
    char           d_name[]; /* Filename (null-terminated) */
};

// For us, a letter consists of a timestamp and a pointer to a
// filename.
struct letter {
    time_t timestamp;
    char  *filename;
};

void insert_letter(struct letter **letters, int *len, char *name, time_t ts) {
    *letters = realloc(*letters, (*len + 1) * sizeof (struct letter));
    struct letter *letter = &(*letters)[*len];
    letter->filename = name;
    letter->timestamp = ts;

    (*len)++;
}

int cmp_letter(const void *l1, const void *l2) {
    return ((struct letter *)l1)->timestamp - ((struct letter *)l2)->timestamp;
}

time_t sec_to_days(time_t time) {
    return time / 60 / 60 / 24;
}

void print_time(time_t t) {
    struct tm * ptm = localtime(&t);
    char buffer[32];
    // Format: Mo, 15.06.2009 20:20:00
    strftime(buffer, 32, "%a, %d.%m.%Y %H:%M:%S\n", ptm);
    printf(buffer); 
}

int main(int argc, char *argv[]) {
    int dir = open(".", O_RDONLY|O_DIRECTORY);
    if (dir < 0)
        die("open");
    char buf[1024];
    int ret;
    struct statx sbuf;
    time_t last_christmas = christmas_day(-1);
    time_t next_christmas = christmas_day(0);
    // printf ("last chris ="); print_time(last_christmas);
    // printf ("next chris ="); print_time(next_christmas);
    struct letter *letters = NULL;
    int letters_len = 0;
    struct linux_dirent64 *dent;
    while ((ret = getdents64(dir, buf, sizeof buf)) > 0) {
        for (
            unsigned long off = 0;
            off < ret && (dent = (struct linux_dirent64 *)(buf + off));
            off += dent->d_reclen
        ) {
            if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))
                continue;
            if (statx(dir, dent->d_name, 0, STATX_BTIME | STATX_ATIME, &sbuf) < 0)
                die("statx");
            if (sbuf.stx_btime.tv_sec > last_christmas)
                insert_letter(&letters, &letters_len, strdup(dent->d_name), sbuf.stx_btime.tv_sec);
        }
    }
    qsort(letters, letters_len, sizeof(struct letter), cmp_letter);
    for (int i=0; i < letters_len; i++) {
        printf("%ld days:  %s\n", sec_to_days(next_christmas - letters[i].timestamp), letters[i].filename);
    }
}
