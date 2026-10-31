#define _GNU_SOURCE

#include "syscall_engine.h"

#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/uio.h>
#include <sys/syscall.h>

#include "log.h"
#include "utils.h"

#include "syscall/newstatat.h"

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

                size_t n = getbuf(buf,sizeof(buf),rdx,pid);

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

        case SYS_newfstatat:
            char user_path[4096];

            size_t n = getbuf(user_path,sizeof(user_path),rsi,pid);

            return sys_newfstatat(
                (int)rdi,
                user_path,
                NULL,
                (int)r10
            );

        default:
            return -ENOSYS;
    }
}
