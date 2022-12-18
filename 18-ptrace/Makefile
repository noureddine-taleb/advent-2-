PROG = systrace

${PROG}: ${PROG}.c
	gcc $< -o  $@ -Wall -g

run: ${PROG}
	./${PROG} ls -l

strace: ${PROG}
	strace ./${PROG} ls -l

clean:
	rm -f ./${PROG}
