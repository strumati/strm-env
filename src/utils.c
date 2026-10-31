#define _GNU_SOURCE

#include <stdint.h>
#include <sys/uio.h>

ssize_t getbuf(char *buf, size_t buf_size, unsigned long r, pid_t pid)
{
    struct iovec local = {
        .iov_base = buf,
        .iov_len = buf_size
    };

    struct iovec remote = {
        .iov_base = (void *)(uintptr_t)r,
        .iov_len = buf_size
    };

    return process_vm_readv(
        pid, &local, 1, &remote, 1, 0
    );
}