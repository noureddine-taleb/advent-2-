PROG = scribble

${PROG}: ${PROG}.c
	gcc $< -o  $@ -Wall -g -lpthread

run: ${PROG}
	./${PROG} OUT IN bash

strace: ${PROG}
	strace -f ./${PROG} OUT IN  bash

clean:
	rm -f ./${PROG}
