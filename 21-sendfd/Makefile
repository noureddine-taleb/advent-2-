PROG=server

all: ${PROG} client

%: %.c
	gcc $< -o  $@ -Wall -g

run: ${PROG}
	./${PROG}

strace: ${PROG}
	strace ./${PROG}

clean:
	rm -f ./${PROG} client
