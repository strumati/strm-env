#define _GNU_SOURCE
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "log.h"
#include "syscall_engine.h"

#ifndef __x86_64__
#error "This example requires Linux x86-64."
#endif

#define SYSCALL_PASSTHROUGH INT64_MIN

static int entering = 1;
static int emulated = 0;
static long pending_result = 0;

static const char *syscall_name(long n) {
    switch (n) {
        case 0:   return "read";
        case 1:   return "write";
        case 2:   return "open";
        case 3:   return "close";
        case 9:   return "mmap";
        case 10:  return "mprotect";
        case 11:  return "munmap";
        case 12:  return "brk";
        case 16:  return "ioctl";
        case 39:  return "getpid";
        case 57:  return "fork";
        case 59:  return "execve";
        case 60:  return "exit";
        case 157: return "prctl";
        case 202: return "futex";
        case 231: return "exit_group";
        case 257: return "openat";
        case 262: return "newfstatat";
        case 318: return "getrandom";
        default:  return "unknown";
    }
}

static void show_call(struct user_regs_struct *r) {
    long n = (long)r->orig_rax;

    env_log("\n[SYSCALL] %s (#%ld)\n", syscall_name(n), n);
    env_log("  args: rdi=%#llx rsi=%#llx rdx=%#llx\n",
           (unsigned long long)r->rdi,
           (unsigned long long)r->rsi,
           (unsigned long long)r->rdx);
    env_log("        r10=%#llx r8=%#llx r9=%#llx\n",
           (unsigned long long)r->r10,
           (unsigned long long)r->r8,
           (unsigned long long)r->r9);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s ./runner [args...]\n", argv[0]);
        return 2;
    }

    pid_t child = fork();
    if (child == -1) {
        perror("fork");
        return 1;
    }

    if (child == 0) {
        if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) == -1) {
            perror("PTRACE_TRACEME");
            _exit(127);
        }

        execvp(argv[1], &argv[1]);
        perror("execvp");
        _exit(127);
    }

    int status;
    if (waitpid(child, &status, 0) == -1 || !WIFSTOPPED(status)) {
        perror("waitpid initial stop");
        return 1;
    }

    if (ptrace(PTRACE_SETOPTIONS, child, 0,
               (void *)(uintptr_t)PTRACE_O_TRACESYSGOOD) == -1) {
        perror("PTRACE_SETOPTIONS");
        kill(child, SIGKILL);
        waitpid(child, NULL, 0);
        return 1;
    }

    for (;;) {
        if (ptrace(PTRACE_SYSCALL, child, 0, 0) == -1) {
            if (errno == ESRCH) break;
            perror("PTRACE_SYSCALL");
            break;
        }

        if (waitpid(child, &status, 0) == -1) {
            if (errno == EINTR) continue;
            perror("waitpid");
            break;
        }

        if (WIFEXITED(status)) {
            printf("\n[EXIT] code=%d\n", WEXITSTATUS(status));
            break;
        }

        if (WIFSIGNALED(status)) {
            printf("\n[SIGNAL EXIT] signal=%d\n", WTERMSIG(status));
            break;
        }

        if (!WIFSTOPPED(status))
            continue;

        int sig = WSTOPSIG(status);

        if (sig == (SIGTRAP | 0x80)) {
            struct user_regs_struct regs;

            if (ptrace(PTRACE_GETREGS, child, 0, &regs) == -1) {
                perror("PTRACE_GETREGS");
                break;
            }

            if (entering) {
                
long nr = (long)regs.orig_rax;

show_call(&regs);

long result = syscall_engine(
    child,
    nr,
    regs.rdi,
    regs.rsi,
    regs.rdx,
    regs.r10,
    regs.r8,
    regs.r9
);

regs.orig_rax = (unsigned long long)-1;

if (ptrace(PTRACE_SETREGS, child, 0, &regs) == -1) {
    perror("PTRACE_SETREGS");
    kill(child, SIGKILL);
    waitpid(child, NULL, 0);
    return 1;
}

pending_result = result;
emulated = 1;

env_log("  -> ENGINE (result=%ld)\n", result);

            } else {
                if (emulated) {
                    regs.rax = (unsigned long long)pending_result;

                    if (ptrace(PTRACE_SETREGS, child, 0, &regs) == -1) {
                        perror("PTRACE_SETREGS");
                        kill(child, SIGKILL);
                        waitpid(child, NULL, 0);
                        return 1;
                    }

                    emulated = 0;
                }

                env_log("  [RETURN] rax=%lld (0x%llx)\n",
                       (long long)regs.rax,
                       (unsigned long long)regs.rax);
            }

            entering = !entering;
            continue;
        }

        int deliver = sig == SIGTRAP ? 0 : sig;
        if (ptrace(PTRACE_SYSCALL, child, 0,
                   (void *)(intptr_t)deliver) == -1) {
            if (errno != ESRCH) perror("PTRACE_SYSCALL signal");
            break;
        }

        if (waitpid(child, &status, 0) == -1) {
            perror("waitpid");
            break;
        }

        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            if (WIFEXITED(status))
                env_log("\n[EXIT] code=%d\n", WEXITSTATUS(status));
            else
                env_log("\n[SIGNAL EXIT] signal=%d\n", WTERMSIG(status));
            break;
        }

        if (WIFSTOPPED(status)) {
            int next_sig = WSTOPSIG(status);
            if (next_sig == (SIGTRAP | 0x80)) {
                fprintf(stderr,
                    "Signal/syscall stop interleaving; use a robust "
                    "PTRACE_GET_SYSCALL_INFO loop for production.\n");
                kill(child, SIGKILL);
                waitpid(child, NULL, 0);
                break;
            }
        }
    }

    return 0;
}
