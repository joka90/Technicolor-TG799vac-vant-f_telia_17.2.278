#ifndef RIP2_CONFIG_H
#define RIP2_CONFIG_H
#include <linux/types.h>
//#include <linux/kernel.h>
#include <asm/byteorder.h>

extern struct mutex rip2_biglock;

#define ALLOC(size)       malloc((size))
#define FREE(ptr)         free((ptr))

#define LOCK()            
#define UNLOCK()          

#define ERR(fmt, ...)     printf(fmt, ## __VA_ARGS__)
#define INFO(fmt, ...)    printf(fmt, ## __VA_ARGS__)
#if defined(RIP_DEBUG)
  #define DBG(fmt, ...)   printf(fmt, ## __VA_ARGS__)
#else
  #define DBG(fmt, ...)
#endif

#ifndef bswap_16
#define bswap_16(x)	__cpu_to_be16(x)
#endif

#ifndef bswap_32
#define bswap_32(x)	__cpu_to_be32(x)
#endif

#endif /* RIP2_CONFIG */
