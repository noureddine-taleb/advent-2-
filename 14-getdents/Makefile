PROG = letters

${PROG}: ${PROG}.c
	gcc $< -o  $@ -Wall -g

run: ${PROG}
	./${PROG}

strace: ${PROG}
	strace ./${PROG}

clean:
	rm -f ./${PROG}
