#ifndef SYSCALL_ENGINE_H
#define SYSCALL_ENGINE_H

#include <stdint.h>
#include <unistd.h>
#include <sys/syscall.h>

long syscall_engine(
    pid_t pid,
    long orig_rax,
    unsigned long rdi,
    unsigned long rsi,
    unsigned long rdx,
    unsigned long r10,
    unsigned long r8,
    unsigned long r9
);

#endif
