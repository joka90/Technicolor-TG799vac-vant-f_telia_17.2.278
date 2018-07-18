#ifndef RIP2_CONFIG_H
#define RIP2_CONFIG_H
#include <byteswap.h>
#include <asm/byteorder.h>

/* Include files from the bootloader tree, passed via CMake
   EXTRA_INCLUDES variable */
#include "lib_types.h"
#include "lib_malloc.h"
#include "lib_string.h"

/* Define needed otherwise the print function is stubbed */
#define CFG_RAMAPP 1
#include "lib_printf.h"

typedef unsigned long loff_t;

#define ALLOC(size)           KMALLOC((size), 0)
#define FREE(ptr)             KFREE((ptr))

#define LOCK()
#define UNLOCK()

#define ERR(fmt, ...)         printf(fmt, ## __VA_ARGS__)
#define INFO(fmt, ...)        printf(fmt, ## __VA_ARGS__)
#if defined(RIP_DEBUG)
  #define DBG(fmt, ...)       printf(fmt, ## __VA_ARGS__)
#else
  #define DBG(fmt, ...)
#endif

#define PAGE_SIZE    (4 * 1024)

#endif /* RIP2_CONFIG_H */

