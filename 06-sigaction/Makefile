PROG = sigaction

${PROG}: ${PROG}.c
	gcc $< -o $@ -Wall

run: ${PROG}
	./${PROG}

strace: ${PROG}
	strace -ff ./{$PROG}

clean:
	rm -f ./${PROG}
