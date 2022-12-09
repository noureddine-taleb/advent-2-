PROG = checksum

${PROG}: ${PROG}.c
	gcc $< -o  $@ -Wall -g

run: ${PROG}
	./checksum -r ${PROG}.c 
	./checksum ${PROG}.c
	./checksum ${PROG}.c



strace: ${PROG}
	strace ./checksum ${PROG}.c

clean:
	rm -f ./${PROG}
