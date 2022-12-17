PROG = seccomp

${PROG}: ${PROG}.c
	gcc $< -o  $@ -Wall -g -Wno-unused-function

run: ${PROG}
	./${PROG} 

strace: ${PROG}
	strace -f ./${PROG}

clean:
	rm -f ./${PROG}
