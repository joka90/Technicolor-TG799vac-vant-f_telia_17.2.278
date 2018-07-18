/* crc.h -- Defines the functions available for various Cyclic Redundancy Checks
 */

#ifndef CRC__H
#define CRC__H

#ifdef __cplusplus
extern "C"
{
#endif

#define CRC32		0x04c11db7l	/* CRC-32 polynomial. */
/* CRC-32 functions.
   ----------------- */

typedef unsigned long (*CRC32FN)(unsigned long data, unsigned long poly, unsigned long crc);

/* A function to create a crc from a byte. */

void rip2_mk_crc32_table (unsigned long poly, CRC32FN crcfn);
unsigned long rip2_crc32 (unsigned char *data, unsigned count);
	unsigned long rip2_crc32_hw (unsigned long data, unsigned long poly, unsigned long crc);

#ifdef __cplusplus
}
#endif

#endif	/* ! CRC__H */
