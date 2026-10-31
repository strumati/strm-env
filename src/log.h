#ifndef LOG_H
#define LOG_H

#ifdef ENABLE_LOG
    #define env_log(...) fprintf(stderr, __VA_ARGS__)
#else
    #define env_log(...) ((void)0)
#endif

#endif