PROG = rseq

${PROG}: ${PROG}.o rseq-asm.o Makefile
	gcc ${PROG}.o rseq-asm.o -o  $@ -Wall -g -Wno-unused-function -O3

%.o: %.c
	gcc -c $< -o  $@ -Wall -g -Wno-unused-function -O3

%.o: %.S
	gcc -c $< -o  $@ 

run: ${PROG}
	./${PROG} 32 regular
	./${PROG} 32 lock

clean:
	rm -f ./${PROG} *.o

man:
	man ./rseq.2

plot:
	./plot.py data
