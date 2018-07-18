
#ifndef __BANKMGR_P_H__
#define __BANKMGR_P_H__

#include "export/bankmgr.h"

#define BTAB_ID 0x4f4b  /* 'OK' */

struct bank_table {
	char magic[4]; /* "BTAB" or "BTA2" for new version */
	unsigned long checksum;
	short version;
	short num_banks;
	short reserved1;
	short reserved2;
	struct bank banks[0];
};

#define bank_table_size(num_banks) (sizeof(struct bank_table) + num_banks*sizeof(struct bank))

int bankmgr_checksum(struct bank_table *bt, unsigned long *checksum);

struct bank_table *load_btab(void);
int store_btab(struct bank_table *bt);
void free_btab(struct bank_table *bt);

short __bankmgr_get_active_bank_id(struct bank_table *bt, enum bank_type type);

#endif /* __BANKMGR_P_H__ */

