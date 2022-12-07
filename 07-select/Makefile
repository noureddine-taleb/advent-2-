PROG = select

${PROG}: ${PROG}.c
	gcc $< -o $@ -Wall -lpthread

run: ${PROG}
	seq 1 100 | ./select "grep 1[1-3]" "grep [1-3]2"

strace: ${PROG}
	echo -e "A\nB\nAB" | strace ./${PROG} "grep A" "grep B"

clean:
	rm -f ./${PROG}
