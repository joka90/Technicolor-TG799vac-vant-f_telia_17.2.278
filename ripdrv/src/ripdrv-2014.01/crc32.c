/* crc32.c -- Performs 32-bit CRC checksums on blocks of data. */

#ifdef BUILDTYPE_uboot_bootloader
#include "rip/include/crc.h"
#else
#include "crc.h"
#endif


static unsigned long crc32_table [256];		/* 32-bit CRC lookup table. */


/* mk_crc32_table -- Create a fast CRC lookup table for either a CCITT CRC-32 or
		     a PKZIP/ARJ CRC-32.

   To create the table for CCITT CRC-32:	mk_crc32_table (CRC32, crc32_hw);
   To create the table for PKZIP/ARJ CRC-32:	mk_crc32_table (CRC32_REV, crc32_rev_hw); */


void rip2_mk_crc32_table (unsigned long poly, CRC32FN crcfn)
{
	int i;

	for (i = 0; i < 256; i++)
		crc32_table [i] = (*crcfn)(i, poly, 0);
}

/* crc32_hw -- Simulates a normal 32-bit CRC hardware circuit. */

unsigned long rip2_crc32_hw (unsigned long data, unsigned long poly, unsigned long crc)
{
       int i;

       data <<= 24;    /* Data to MSB. */
       for (i = 8; i > 0; i--) {
               if ((data ^ crc) & 0x80000000l)
                       crc = (crc << 1) ^ poly;
               else
                       crc <<= 1;
               data <<= 1;
       }
       return (crc);
}

/* buf_crc32 -- Calculate a CRC-32 in the normal order. */

static unsigned long buf_crc32 (unsigned char *data, unsigned count, unsigned long crc)
{
	while (count-- != 0)
		crc = (crc << 8) ^ crc32_table [((crc >> 24) ^ *data++) & 0xff];
	return (crc);
}
/* crc32 -- Calculate a normal CRC-32 for a linear data block. */

unsigned long rip2_crc32 (unsigned char *data, unsigned count)
{
	unsigned long crc;

	crc = buf_crc32 (data, count, 0xffffffffl);
	return (~crc);
}
