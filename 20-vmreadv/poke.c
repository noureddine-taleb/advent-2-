#define _GNU_SOURCE
#include <sys/uio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <Python.h>

#define die(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

// Read memory from pid's memory, starting at ptr and len bytes.
// Returns an heap-allocated buffer
void *peek(pid_t pid, const void *ptr, size_t len) {
    char *buf = malloc(len);
    const struct iovec lv = {
        .iov_base = buf,
        .iov_len = len,
    };
    const struct iovec rv = {
        .iov_base = (void *)ptr,
        .iov_len = len,
    };
    if (process_vm_readv(pid, &lv, 1, &rv, 1, 0) != len)
        die("process_vm_read");
    return buf;
}

// Copy memory from this process to the remote process (pid).
// Remote Address: ptr
// Local  Address: buffer
// Length: len
void poke(pid_t pid, void *ptr, void *buffer, size_t len) {
    const struct iovec lv = {
        .iov_base = buffer,
        .iov_len = len,
    };
    const struct iovec rv = {
        .iov_base = (void *)ptr,
        .iov_len = len,
    };
    if (process_vm_writev(pid, &lv, 1, &rv, 1, 0) != len)
        die("process_vm_writev");
}

PyObject *mutate_pyobj(void *ptr, int pid) {
    PyObject *obj       = peek(pid, ptr, sizeof(PyObject));
    PyTypeObject *type  = peek(pid, Py_TYPE(obj), sizeof(PyTypeObject));
    char *tp_name = peek(pid, type->tp_name, 1024);
    
    printf(">>  PyObject @ %p: type=%s, refcount=%ld\n", ptr, tp_name, Py_REFCNT(obj));

    if (!strcmp(tp_name, "int")) {
        free(obj);
        obj = peek(pid, ptr, sizeof(PyLongObject));
        printf(">>  PyLongObject: ob_digit[0] = %#x\n", ((PyLongObject *)obj)->ob_digit[0]);
        typeof(((PyLongObject *)obj)->ob_digit[0]) new_val = 0xcafebabe;
        poke(pid, ptr + offsetof(PyLongObject, ob_digit), &new_val, sizeof(new_val));
    } else if (!strcmp(tp_name, "float")) {
        free(obj);
        obj = peek(pid, ptr, sizeof(PyFloatObject));
        printf(">>  PyFloatObject: ob_fval = %f\n", ((PyFloatObject *)obj)->ob_fval);
        typeof(((PyFloatObject *)obj)->ob_fval) new_val = 42;
        poke(pid, ptr + offsetof(PyFloatObject, ob_fval), &new_val, sizeof(new_val));
    } else {
        die(tp_name);
    }

    Py_TYPE(obj) = type;
    Py_TYPE(obj)->tp_name = tp_name;
    return obj;
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

    mutate_pyobj(ptr, pid);    
}
