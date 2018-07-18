/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
** Copyright (c) 2013-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
*************************************************************************/

#ifndef __STORAGE_CRC32_H
#define __STORAGE_CRC32_H

int storage_crc32_sb(struct sfs_super_block *sb, unsigned long *crc32);
int storage_crc32_desc(struct sfs_desc *desc, unsigned long *crc32);
int storage_crc32_data(void *data, int size, struct sfs_desc *desc, unsigned long *crc32);

#endif
