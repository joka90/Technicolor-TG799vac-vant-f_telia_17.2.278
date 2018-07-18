/* eraseall.c -- erase the whole of a MTD device

   Copyright (C) 2000 Arcom Control System Ltd

    Renamed to flash_eraseall.c

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA

   $Id: flash_eraseall.c,v 1.24 2005/11/07 11:15:10 gleixner Exp $
*/
#include <sys/types.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <libgen.h>
#include <ctype.h>
#include <time.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <linux/sched.h>
#include "crc32.h"

#include <mtd/mtd-user.h>
#include <linux/version.h>
#include "bcm_local_kernel_include/linux/jffs2.h"
#include "jffs2-user.h"

#define PROGRAM "flash_eraseall"
#define VERSION "$Revision: 1.24 $"

static const char *exe_name;
static const char *mtd_device;
static int quiet;		/* true -- don't output progress */
static int jffs2;		// format for jffs2 usage

static void process_options (int argc, char *argv[]);
static void display_help (void);
static void display_version (void);
static struct jffs2_raw_ebh ebh;
int target_endian = __BYTE_ORDER;

int main (int argc, char *argv[])
{
	mtd_info_t meminfo;
	int fd, ebhpos = 0, ebhlen = 0;
	erase_info_t erase;
	int isNAND, bbtest = 1;
	uint32_t pages_per_eraseblock, available_oob_space;

//add it 
     struct sched_param param = { .sched_priority = 5 /*BRCM_SOFTIRQD_RTPRIO*/ };

     sched_setscheduler(getpid(), SCHED_RR, &param);
//end

	process_options(argc, argv);


	if ((fd = open(mtd_device, O_RDWR)) < 0) {
		fprintf(stderr, "%s: %s: %s\n", exe_name, mtd_device, strerror(errno));
		exit(1);
	}


	if (ioctl(fd, MEMGETINFO, &meminfo) != 0) {
		fprintf(stderr, "%s: %s: unable to get MTD device info\n", exe_name, mtd_device);
		exit(1);
	}

	erase.length = meminfo.erasesize;

	isNAND = meminfo.type == MTD_NANDFLASH ? 1 : 0;

	fprintf(stderr, "%s: %s: FLASH type is :%d\n", exe_name, mtd_device,isNAND);
	printf("flash type is:%d\n",isNAND);

	if (jffs2) {
		ebh.magic = cpu_to_je16 (JFFS2_MAGIC_BITMASK);
		ebh.nodetype = cpu_to_je16 (JFFS2_NODETYPE_ERASEBLOCK_HEADER);
		ebh.totlen = cpu_to_je32(sizeof(struct jffs2_raw_ebh));
		ebh.hdr_crc = cpu_to_je32 (crc32 (0, &ebh,  sizeof (struct jffs2_unknown_node) - 4));
		ebh.reserved = 0;
		ebh.compat_fset = JFFS2_EBH_COMPAT_FSET;
		ebh.incompat_fset = JFFS2_EBH_INCOMPAT_FSET;
		ebh.rocompat_fset = JFFS2_EBH_ROCOMPAT_FSET;
		ebh.erase_count = cpu_to_je32(0);
		ebh.node_crc = cpu_to_je32(crc32(0, (unsigned char *)&ebh + sizeof(struct jffs2_unknown_node) + 4,
					 sizeof(struct jffs2_raw_ebh) - sizeof(struct jffs2_unknown_node) - 4));

		if (isNAND) {
			struct nand_oobinfo oobinfo;

			if (ioctl(fd, MEMGETOOBSEL, &oobinfo) != 0) {
				fprintf(stderr, "%s: %s: unable to get NAND oobinfo\n", exe_name, mtd_device);
				exit(1);
			}

			/* Check for autoplacement */
			if (oobinfo.useecc == MTD_NANDECC_AUTOPLACE) {
				/* Get the position of the free bytes */
				if (!oobinfo.oobfree[0][1]) {
					fprintf (stderr, " Eeep. Autoplacement selected and no empty space in oob\n");
					exit(1);
				}
				ebhpos = oobinfo.oobfree[0][0];
				ebhlen = oobinfo.oobfree[0][1];
			} else {
				/* Legacy mode */
				switch (meminfo.oobsize) {
				case 8:
					ebhpos = 6;
					ebhlen = 2;
					break;
				case 16:
					ebhpos = 8;
					ebhlen = 8;
					break;
				case 64:
					ebhpos = 16;
					ebhlen = 8;
					break;
				}
			}
			pages_per_eraseblock = meminfo.erasesize/meminfo.writesize;
			available_oob_space = ebhlen * pages_per_eraseblock;
			if (available_oob_space < sizeof(struct jffs2_raw_ebh)) {
				fprintf(stderr, "The OOB area(%d) is not big enough to hold eraseblock_header(%d)", available_oob_space, sizeof(struct jffs2_raw_ebh));
				exit(1);
			}
		}
	}

	for (erase.start = 0; erase.start < meminfo.size; erase.start += meminfo.erasesize) {
		if (bbtest) {
			loff_t offset = erase.start;
			int ret = ioctl(fd, MEMGETBADBLOCK, &offset);
			if (ret > 0) {
				if (!quiet)
					printf ("\nSkipping bad block at 0x%08x\n", erase.start);
				continue;
			} else if (ret < 0) {
				if (errno == EOPNOTSUPP) {
					bbtest = 0;
					if (isNAND) {
						fprintf(stderr, "%s: %s: Bad block check not available\n", exe_name, mtd_device);
						exit(1);
					}
				} else {
					fprintf(stderr, "\n%s: %s: MTD get bad block failed: %s\n", exe_name, mtd_device, strerror(errno));
					exit(1);
				}
			}
		}

		if (!quiet) {
			printf
                           ("\rErasing %d Kibyte @ %x -- %2llu %% complete.",
			     meminfo.erasesize / 1024, erase.start,
			     (unsigned long long)
			     erase.start * 100 / meminfo.size);
		}
		fflush(stdout);

		if (ioctl(fd, MEMERASE, &erase) != 0) {
			fprintf(stderr, "\n%s: %s: MTD Erase failure: %s\n", exe_name, mtd_device, strerror(errno));
			continue;
		}

		/* format for JFFS2 ? */
		if (!jffs2)
			continue;

		/* write cleanmarker */
		if (isNAND) {
			struct mtd_oob_buf oob;
			uint32_t i = 0, written = 0;

			while (written < sizeof(struct jffs2_raw_ebh)) {
				oob.ptr = (unsigned char *) &ebh + written;
				oob.start = erase.start + meminfo.writesize*i + ebhpos;
				oob.length = (sizeof(struct jffs2_raw_ebh) - written) < ebhlen ? (sizeof(struct jffs2_raw_ebh) - written) : ebhlen;
				if (ioctl (fd, MEMWRITEOOB, &oob) != 0) {
					fprintf(stderr, "\n%s: %s: MTD writeoob failure: %s\n", exe_name, mtd_device, strerror(errno));
					break;
				}
				i++;
				written += oob.length;
			}
			if (written < sizeof(struct jffs2_raw_ebh)) {
				continue;
			}
		} else {
			if (lseek (fd, erase.start, SEEK_SET) < 0) {
				fprintf(stderr, "\n%s: %s: MTD lseek failure: %s\n", exe_name, mtd_device, strerror(errno));
				continue;
			}
			if (write (fd , &ebh, sizeof (ebh)) != sizeof (ebh)) {
				fprintf(stderr, "\n%s: %s: MTD write failure: %s\n", exe_name, mtd_device, strerror(errno));
				continue;
			}
		}
		if (!quiet)
			printf (" Cleanmarker written at %x.", erase.start);
	}
	if (!quiet)
		printf("\n");

	return 0;
}


void process_options (int argc, char *argv[])
{
	int error = 0;

	exe_name = argv[0];

	for (;;) {
		int option_index = 0;
		static const char *short_options = "jq";
		static const struct option long_options[] = {
			{"help", no_argument, 0, 0},
			{"version", no_argument, 0, 0},
			{"jffs2", no_argument, 0, 'j'},
			{"quiet", no_argument, 0, 'q'},
			{"silent", no_argument, 0, 'q'},

			{0, 0, 0, 0},
		};

		int c = getopt_long(argc, argv, short_options,
				    long_options, &option_index);
		if (c == EOF) {
			break;
		}

		switch (c) {
		case 0:
			switch (option_index) {
			case 0:
				display_help();
				break;
			case 1:
				display_version();
				break;
			}
			break;
		case 'q':
			quiet = 1;
			break;
		case 'j':
			jffs2 = 1;
			break;
		case '?':
			error = 1;
			break;
		}
	}
	if (optind == argc) {
		fprintf(stderr, "%s: no MTD device specified\n", exe_name);
		error = 1;
	}
	if (error) {
		fprintf(stderr, "Try `%s --help' for more information.\n",
			exe_name);
		exit(1);
	}

	mtd_device = argv[optind];
}


void display_help (void)
{
	printf("Usage: %s [OPTION] MTD_DEVICE\n"
	       "Erases all of the specified MTD device.\n"
	       "\n"
	       "  -j, --jffs2    format the device for jffs2\n"
	       "  -q, --quiet    don't display progress messages\n"
	       "      --silent   same as --quiet\n"
	       "      --help     display this help and exit\n"
	       "      --version  output version information and exit\n",
	       exe_name);
	exit(0);
}


void display_version (void)
{
	printf(PROGRAM " " VERSION "\n"
	       "\n"
	       "Copyright (C) 2000 Arcom Control Systems Ltd\n"
	       "\n"
	       PROGRAM " comes with NO WARRANTY\n"
	       "to the extent permitted by law.\n"
	       "\n"
	       "You may redistribute copies of " PROGRAM "\n"
	       "under the terms of the GNU General Public Licence.\n"
	       "See the file `COPYING' for more information.\n");
	exit(0);
}
