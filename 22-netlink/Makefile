PROG = cn_proc

${PROG}: ${PROG}.c
	gcc $< -o  $@ -Wall -g -Wno-unused-function

run: ${PROG}
	sudo ./${PROG} 

strace: ${PROG}
	sudo strace ./${PROG}

clean:
	rm -f ./${PROG}
