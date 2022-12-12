PROG = postbox

DEPS = postbox.c fifo.c domain.c

${PROG}: ${DEPS}
	gcc ${PROG}.c -o  $@ -Wall -g -lrt

run: ${PROG}
	./${PROG}



strace: ${PROG}
	strace ./${PROG}

clean:
	rm -f ./${PROG}
