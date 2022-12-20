PROG = poke

${PROG}: ${PROG}.c
	gcc $< -o  $@ -Wall -g -O3 `pkg-config --cflags python3`

run: ${PROG}
	python3 test.py

strace: ${PROG}
	python3 test.py strace -e trace='process_vm_readv,process_vm_writev'

clean:
	rm -f ./${PROG}
