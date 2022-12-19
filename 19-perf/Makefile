PROG = perf

${PROG}: ${PROG}.c matrix.c
	gcc $< -o  $@ -Wall -g -O3

run: ${PROG}
	./${PROG} 512

strace: ${PROG}
	strace ./${PROG} 512

clean:
	rm -f ./${PROG}
