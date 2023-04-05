PROG = epoll

${PROG}: ${PROG}.c
	gcc $< -o  $@ -Wall -Wextra -g

run: ${PROG}
	seq 1 10000 | ./${PROG} "grep ^1[1-4]$$" "wc -l" | cat > /dev/null

strace: ${PROG}
	echo 123 | strace  ./${PROG} cat

clean:
	rm -f ./${PROG}
