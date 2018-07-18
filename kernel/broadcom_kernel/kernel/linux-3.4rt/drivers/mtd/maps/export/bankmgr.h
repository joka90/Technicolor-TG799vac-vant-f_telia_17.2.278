
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
  BANKMGR_HEAD_VERIFY_ERR            = -3,
  BANKMGR_IMAGE_VERIFY_ERR           = -2,   
  BANKMGR_ERR                        = -1,           
  BANKMGR_NO_ERR                     = 0,         
};

enum bank_status {
	BANK_NOTEMPTY,  
	BANK_EMPTY,      
	BANK_CORRUPTED,
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

struct bankmgr_stream_ctx {
	void *opaque;
};

#ifndef BOOL
#define BOOL    int
#endif

#define FIRMWARE_FILENAME_PATH    "/userfs/common/filename-bank-" 

/*
 * Bank table operations
 */
int bankmgr_get_num_banks(void);

short bankmgr_get_active_bank_id(enum bank_type type);
short bankmgr_get_booted_bank_id(void);
short bankmgr_get_bank_id(enum bank_type type, enum criterion criterion);
int bankmgr_set_active_bank_id(enum bank_type type, short bank_id);

int bankmgr_get_bank(short bank_id, struct bank *bank);
int bankmgr_get_active_bank(enum bank_type type, struct bank *bank);

/* 
  returns -1 on error, status values:	
  BANK_NOTEMPTY,  The bank is not empty.
	BANK_EMPTY,     The bank is empty.
	BANK_CORRUPTED, The bank is corrupted.
*/
int bankmgr_get_bank_status(short bank_id); 
short bankmgr_get_bank_flags(short bank_id);
short bankmgr_get_bank_FVP(short bank_id);
enum bank_type bankmgr_get_bank_type(short bank_id);

int bankmgr_copy(short dest_bank_id, short src_bank_id);

int bankmgr_set_bank_specific(short bank_id, void *data, int size);

/*
 * Verify whether a given bank contains a valid image:
 * when it does, 1 is returned; 0 -- otherwise and
 * (-1) indicates a failure to retrive the status.
 */
int bankmgr_is_populated(short bank_id);

/*
 * Flash operations
 */
int bankmgr_erase(short bank_id, unsigned long off, int len); /* if len == 0, erase from off to the end of the bank */

/* non-streamed writing */
int bankmgr_write(short bank_id, unsigned long off, void *image, int len); /* returns the number of (uncompressed) bytes written. */

/* streamed writing */
int bankmgr_stream_open(short bank_id, unsigned long off, struct bankmgr_stream_ctx *ctx);
int bankmgr_stream_write(struct bankmgr_stream_ctx *ctx, void *data, int len);
int bankmgr_stream_close(struct bankmgr_stream_ctx *ctx);
char *bankmgr_sw_version(short bank_id, char *bank_version, int len);

/* Others */
char * bankmgr_getSwFilename(BOOL activeBank_b, char * fileName_o);

#endif /* __BANKMGR_H__ */

