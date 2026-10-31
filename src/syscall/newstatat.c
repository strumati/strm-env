#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>

// file system check
long sys_newfstatat(int dirfd,const char *user_path,struct stat *user_stat,int flags)
{
    printf("dirfd: %d\n",dirfd);
    printf("user_path: %s\n",user_path);

    printf("flags: %d\n",flags);

    return -2;
}