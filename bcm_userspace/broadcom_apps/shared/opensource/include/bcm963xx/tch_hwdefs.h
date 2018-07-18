#ifndef _TCH_HWDEFS_H
#define _TCH_HWDEFS_H

#define PROZONE_RESERVED_MEM            ( 128 * 1024)
#define RIP2_CRYPTO_RESERVED_MEM        (   4 * 1024)

#define TCH_RESERVED_TOPMEM             ( 256 * 1024)

#if (TCH_RESERVED_TOPMEM < PROZONE_RESERVED_MEM + RIP2_CRYPTO_RESERVED_MEM)
#error "Invalid reserved memory configuration"
#endif

#endif /* _TCH_HWDEFS_H */
