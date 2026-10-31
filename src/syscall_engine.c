#define _GNU_SOURCE

#include "syscall_engine.h"

#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/uio.h>
#include <sys/syscall.h>

#include "log.h"

long syscall_engine(
    pid_t pid,
    long orig_rax,
    unsigned long rdi,
    unsigned long rsi,
    unsigned long rdx,
    unsigned long r10,
    unsigned long r8,
    unsigned long r9
) {

    switch (orig_rax) {
        case SYS_write:
            if(rdi == 1) {
                char buf[4096];

                size_t len = rdx < sizeof(buf) - 1
                           ? (size_t)rdx
                           : sizeof(buf) - 1;

                struct iovec local = {
                    .iov_base = buf,
                    .iov_len = len
                };

                struct iovec remote = {
                    .iov_base = (void *)(uintptr_t)rsi,
                    .iov_len = len
                };

                ssize_t n = process_vm_readv(
                    pid, &local, 1, &remote, 1, 0
                );

                //if (n < 0) {
                //    perror("process_vm_readv");
                //    return -errno;
                //}

                // Nicht voraussetzen, dass der Puffer nullterminiert ist.
                fwrite(buf, 1, (size_t)n, stdout);
                //fflush(stdout);
            }
            return rdx;

        case SYS_getpid:
            return 1234;

        case SYS_getuid:
            return 1000;

        case SYS_geteuid:
            return 1000;

        case SYS_getgid:
            return 1000;

        case SYS_getegid:
            return 1000;

        case SYS_fstat:
        case SYS_newfstatat:
            // Cannot safely report success without filling
            // the tracee's struct stat.
            return -ENOSYS;

        case SYS_read:
        case SYS_open:
        case SYS_openat:
        case SYS_close:
        case SYS_mmap:
        case SYS_mprotect:
        case SYS_munmap:
        case SYS_brk:
        case SYS_ioctl:
        case SYS_futex:
        case SYS_getrandom:
        case SYS_execve:
        case SYS_exit:
        case SYS_exit_group:
            // Not implemented by this emulation engine.
            return -ENOSYS;

        default:
            // Every unknown syscall is handled here as well.
            return -ENOSYS;
    }
}
