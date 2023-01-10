TARGET := mmap
SRCS := mmap.c
OBJS := ${SRCS:.c=.o}
PERSISTENCE_FILE := mmap.persistent
PERSISTENCE_SECTION := persistent

CFLAGS := -DPERSISTENCE_FILE=\"$(PERSISTENCE_FILE)\" -DPERSISTENCE_SECTION=\"$(PERSISTENCE_SECTION)\"

all: $(TARGET) $(PERSISTENCE_FILE)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

$(PERSISTENCE_FILE): $(TARGET)
	cat /dev/zero | head --bytes=$(shell size -A -d $(TARGET) | grep $(PERSISTENCE_SECTION) | awk '{ print $$2 }') > $@

clean:
	$(RM) $(TARGET) $(PERSISTENCE_FILE)

re: clean all