/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
** Copyright (c) 2013-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
*************************************************************************/

#ifndef __BANKMGR_H__
#define __BANKMGR_H__

enum bank_type {
	BOOTABLE,
	USERFS,
	CUSTOM,
	FEMTO_AP,
	
	/* extendable for future use */
};

enum criterion {
	BANK_ACTIVE,
	BANK_INACTIVE,
	BANK_BOOTED,
	BANK_NOTBOOTED,
};

enum upgraded_err_type {
  BANKMGR_RDOWNGRADE_ERR             = -4,
  BANKMGR_HEAD_VERIFY_ERR            = -3,
  BANKMGR_IMAGE_VERIFY_ERR           = -2,   
  BANKMGR_ERR                        = -1,           
  BANKMGR_NO_ERR                     = 0,         
};

#define BANK_NAME_MAXLEN	16

struct bank {
	enum bank_type type;
	unsigned long size;
	unsigned long bank_offset;
	unsigned long boot_offset;
	short FVP; /* indicates correctly Flashed image */
	short flags;
#define BANK_FLAGS_ACTIVE_MASK (1<<0) /* 1=active, 0=inactive */

	char name[BANK_NAME_MAXLEN];
	int bank_specific[5];
};


/*
 * Bank table operations
 */
int bankmgr_get_num_banks(void);
int bankmgr_init(void);
void bankmgr_fini(void);


short bankmgr_get_active_bank_id(enum bank_type type);
short bankmgr_get_booted_bank_id(void);
short bankmgr_get_bank_id(enum bank_type type, enum criterion criterion);
short bankmgr_get_bank_id_by_name(char * bankname);
int bankmgr_set_active_bank_id(enum bank_type type, short bank_id);

int bankmgr_get_bank(short bank_id, struct bank *bank);
int bankmgr_get_active_bank(enum bank_type type, struct bank *bank);

short bankmgr_get_bank_flags(short bank_id);
short bankmgr_get_bank_FVP(short bank_id);
enum bank_type bankmgr_get_bank_type(short bank_id);

int bankmgr_set_bank_specific(short bank_id, void *data, int size);



#endif /* __BANKMGR_H__ */

