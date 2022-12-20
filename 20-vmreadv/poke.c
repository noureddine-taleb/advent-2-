#define _GNU_SOURCE
#include <sys/uio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <Python.h>

#define die(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

// Read memory from pid's memory, starting at ptr and len bytes.
// Returns an heap-allocated buffer
void *peek(pid_t pid, const void *ptr, size_t len) {
    // FIXME: process_vm_readv(2)
    return NULL;
}

// Copy memory from this process to the remote process (pid).
// Remote Address: ptr
// Local  Address: buffer
// Length: len
void poke(pid_t pid, void *ptr, void *buffer, size_t len) {
    // FIXME: process_vm_writev(2)
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s PID ADDR\n", argv[0]);
        return -1;
    }
    pid_t pid = atoi(argv[1]); (void)pid;
    void *ptr;
    int n = sscanf(argv[2], "0x%p", &ptr);
    if (n != 1) die("sscanf");

    // FIXME: Read the PyObject obj in remote process (pid, ptr)
    // FIXME: Extract the type name (Py_TYPE(obj)->tp_name
    // FIXME: If "float" PyFloatObject: square ob_fvalue
    // FIXME: If "int"   PyLongObject:  set the lowest 2 bytes to 0xabba

    // Example output of make run (two invocations of poke):
    // pointers ['0x7f6050b5da90', '0x7f60507cd410', '0x7f60507cd410']
    // objects ['0xdeadbeef', 23.0, 23.0]
    //    PyObject @ 0x7f6050b5da90: refcount=3, type=int
    //    PyLongObject: ob_digit[0] = 0x1eadbeef
    //    PyObject @ 0x7f60507cd410: refcount=4, type=float
    //    PyFloatObject: ob_fval=23.000000
    // object ['0xdeadabba', 529.0, 529.0]
}
