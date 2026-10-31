#ifndef NEWSTATAT_H
#define NEWSTATAT_H

#include <sys/stat.h>

long sys_newfstatat(
    int dirfd,
    const char *user_path,
    struct stat *user_stat,
    int flags
);

#endif