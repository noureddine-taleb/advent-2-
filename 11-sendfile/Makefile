PROG = sendfile

${PROG}: ${PROG}.c
	gcc $< -o  $@ -Wall -g

run: ${PROG}
	ROUNDS=100 ./${PROG} test.jpg

strace: ${PROG}
	ROUNDS=1 strace ./${PROG} ${PROG}

clean:
	rm -f ./${PROG}
