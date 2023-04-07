inotify: inotify.c
	gcc inotify.c -o inotify -Wall

run: ./inotify
	./inotify

strace: ./inotify
	strace -ff ./inotify thread

clean:
	rm -f ./inotify
