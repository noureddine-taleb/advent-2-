#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#define PAGE_SIZE 0x1000

#ifndef PERSISTENCE_SECTION
#define PERSISTENCE_SECTION "persistent"
#endif

#ifndef PERSISTENCE_FILE
#define PERSISTENCE_FILE "mmap.persistent"
#endif

#define persistent __attribute__((section(PERSISTENCE_SECTION)))
#define page_aligned __attribute__((aligned(PAGE_SIZE)))

#define PERSISTENT_START page_aligned persistent char __per_start__;
#define PERSISTENT_END page_aligned persistent char __per_end__;

#define PAGE_ALIGN_DOWN(addr) ((unsigned long)addr & ~0xfff)
#define PAGE_ALIGN_UP(addr) (((unsigned long)addr + 0xfff) & ~0xfff)

PERSISTENT_START

persistent int i;
persistent int j;

PERSISTENT_END

extern char __start_persistent[];
extern char __stop_persistent[];

void setup_persistent(char *file)
{
	unsigned long	segment_start;
	unsigned long	segment_end;
	unsigned long	segment_len;
    int 			fd;

	segment_start = PAGE_ALIGN_DOWN(__start_persistent);
	segment_end = PAGE_ALIGN_DOWN(__stop_persistent);
	segment_len = segment_end - segment_start;

    fd = open(file, O_CREAT | O_RDWR);
    if (fd < 0)
		perror("open"), exit(1);
	ftruncate(fd, segment_len);
	if (munmap((char *)segment_start, segment_len) < 0)
		perror("munmap"), exit(1);
	if (mmap((char *)segment_start, segment_len, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fd, 0) == MAP_FAILED)
		perror("mmap"), exit(1);
}

int main()
{
	setup_persistent(PERSISTENCE_FILE);
	printf("i = %d, j = %d\n", i, j);
	i++;
	j++;
	exit(0);
}
