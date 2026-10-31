#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <sys/types.h>

ssize_t getbuf(void *buf, size_t size, unsigned long addr, pid_t pid);

#endif