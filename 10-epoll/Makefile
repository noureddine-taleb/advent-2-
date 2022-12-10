PROG = epoll

${PROG}: ${PROG}.c
	gcc $< -o  $@ -Wall -g

run: ${PROG}
	yes | ./${PROG}  cat cat cat cat > /dev/null

strace: ${PROG}
	echo 123 | strace  ./${PROG} cat

clean:
	rm -f ./${PROG}
