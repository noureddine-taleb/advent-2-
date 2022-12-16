PROG = iouring

${PROG}: ${PROG}.c
	gcc $< -o  $@ -Wall -g

run: ${PROG}
	./${PROG} 32 test.jpg

strace: ${PROG}
	strace ./${PROG} 32 test.jpg

clean:
	rm -f ./${PROG}
