PROG = postbox

DEPS = postbox.c fifo.c domain.c
DEPS += mq_send signalfd.c mqueue.c

${PROG}: ${DEPS}
	gcc ${PROG}.c -o  $@ -Wall -g -lrt

run: ${PROG}
	./${PROG}

mq_send: mq_send.c
	gcc $< -o  $@ -Wall -g -lrt


strace: ${PROG}
	strace ./${PROG}

clean:
	rm -f ./${PROG}
