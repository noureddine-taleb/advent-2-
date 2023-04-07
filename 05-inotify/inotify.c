#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <stdbool.h>
#include <unistd.h>

#define die(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

/* With each inotify event, the kernel supplies us with a bit mask
 * that indicates the cause of the event. With the following table,
 * with one flag per line, you can decode the mask of events.

 If you wonder about the wild syntax of the definition here, a word of
 explanation: Each definition has the form of

    TYPE VARNAME (= INITIALIZER);

 Usually, we use named typed for TYPE (e.g., int, struct bar, long).
 However, C also allows to use unnamed types that are declared in
 place. So in the following

   TYPE        = 'struct { .... } []'
   VARNAME     = inotify_event_flags
   INITIALIZER = { { IN_ACCESS, ...}, ...}
 */
struct {
    int mask;
    char *name;
} inotify_event_flags[] = {
    {IN_ACCESS, "access"},
    {IN_ATTRIB, "attrib"},
    {IN_CLOSE_WRITE, "close_write"},
    {IN_CLOSE_NOWRITE, "close_nowrite"},
    {IN_CREATE, "create"},
    {IN_DELETE, "delete"},
    {IN_DELETE_SELF, "delete_self"},
    {IN_MODIFY, "modify"},
    {IN_MOVE_SELF, "move_self"},
    {IN_MOVED_FROM, "move_from"},
    {IN_MOVED_TO, "moved_to"},
    {IN_OPEN, "open"},
    {IN_MOVE, "move"},
    {IN_CLOSE, "close"},
    {IN_MASK_ADD, "mask_add"},
    {IN_IGNORED, "ignored"},
    {IN_ISDIR, "directory"},
    {IN_UNMOUNT, "unmount"},
};

// We already know this macro from yesterday.
#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(*(arr)))

char *get_event_name(int mask) {
    for  (int i=0; i < ARRAY_SIZE(inotify_event_flags); i++) {
        if (inotify_event_flags[i].mask & mask)
            return inotify_event_flags[i].name;
    }
    return NULL;
}

int main(void) {
    // We allocate a buffer to hold the inotify events, which are
    // variable in size.
    int fd;
    void *buffer = malloc(4096);
    if (!buffer) return -1;

    if ((fd = inotify_init()) < 0)
        return perror("inotify_init"), -1;

    if (inotify_add_watch(fd, ".", IN_ALL_EVENTS) < 0)
        return perror("inotify_add_watch"), -1;

    while (1) {
        int ret = read(fd, buffer, 4096);
        if (ret < 0)
            die("read");
        int off = 0;
        for (
            struct inotify_event *event = buffer;
            off < sizeof buffer
                && off < ret
            ;
            off += sizeof(*event) + event->len,
            event = buffer + off
        ) {
            printf("%s (%x)%s\n", event->name, event->mask, get_event_name(event->mask));
        }
    }
    // As we are nice, we free the buffer again.
    free(buffer);
    return 0;
}
