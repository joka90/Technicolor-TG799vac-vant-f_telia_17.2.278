#ifndef RIP2_CONFIG_H
#define RIP2_CONFIG_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <byteswap.h>
#include <sys/types.h>
#include <errno.h>


extern pthread_mutex_t rip2_biglock;

#define ALLOC(size)           malloc((size))
#define FREE(ptr)             free((ptr))

#define LOCK()                pthread_mutex_lock(&rip2_biglock)
#define UNLOCK()              pthread_mutex_unlock(&rip2_biglock)

#define ERR(fmt, ...)         fprintf(stderr, fmt, ## __VA_ARGS__)
#define INFO(fmt, ...)        printf(fmt, ## __VA_ARGS__)
#if defined(RIP_DEBUG)
  #define DBG(fmt, ...)       printf(fmt, ## __VA_ARGS__)
#else
  #define DBG(fmt, ...)
#endif

#define PAGE_SIZE    (4 * 1024)

#endif /* RIP2_CONFIG_H */

