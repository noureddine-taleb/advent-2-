PROG = writev

${PROG}: ${PROG}.c
	gcc $< -o $@ -Wall

run: ${PROG}
	./${PROG} < ${PROG}.c

strace: ${PROG}
	strace -ff ./{$PROG} < ${PROG}.c

clean:
	rm -f ./${PROG}
