/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
** Copyright (c) 2013-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
*************************************************************************/

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/err.h>

#include "bankmgr.h"
#include "bankmgr_p.h"

/* ----------------------------------------------------------------------------
 * 'Public' bank table operations
 * --------------------------------------------------------------------------*/

/*
 * bankmgr_get_num_banks -- get the total number of banks
 * This value can be used to iterate over all banks
 */
int bankmgr_get_num_banks(void)
{
	struct bank_table *bt;
	int num_banks = -1;

	bt = load_btab();
	if (bt) {
		num_banks = ntohs(bt->num_banks);
		free_btab(bt);
	}

	return num_banks;
}

/*
 * bankmgr_get_bank -- returns the bank structure of a specific bank
 * The caller should pass a pointer to an allocated bank structure.
 */
int bankmgr_get_bank(short bank_id, struct bank *bank)
{
	struct bank_table *bt;
	int ret = BANKMGR_ERR;

	if (bank_id < 0)
		return ret;

	bt = load_btab();
	if (bt) {
		if (bank_id < ntohs(bt->num_banks)) {
			memcpy(bank, &bt->banks[bank_id], sizeof(*bank));
			ret = BANKMGR_NO_ERR;
		}
		free_btab(bt);
	}

	return ret;
}

short bankmgr_get_bank_id_by_name(char * bankname) {
	short banks, bankid;
	banks = bankmgr_get_num_banks();
	for (bankid = 0; bankid < banks; bankid++) {
		struct bank cur_bank;
		if (bankmgr_get_bank(bankid, &cur_bank) == BANKMGR_ERR) continue;
		if (strncmp(cur_bank.name, bankname, sizeof(cur_bank.name)) == 0) return bankid;
	}
	return -1;
}

short __bankmgr_get_active_bank_id(struct bank_table *bt, enum bank_type type)
{
	short i, bank_id = -1;

	for (i = 0; i < ntohs(bt->num_banks); i++) {
		if (ntohl(bt->banks[i].type) == type
		    && ntohs(bt->banks[i].FVP) == 0x0000
		    && ntohs(bt->banks[i].flags) & BANK_FLAGS_ACTIVE_MASK) {
			bank_id = i;
			break;
		}
	}

	return bank_id;
}

/*
 * bankmgr_get_active_bank_id -- returns the bank id of the active bank, or -1 on failure
 */
short bankmgr_get_active_bank_id(enum bank_type type)
{
	struct bank_table *bt;
	short bank_id;

	if ((bt = load_btab()) == NULL)
		return BANKMGR_ERR;

	bank_id = __bankmgr_get_active_bank_id(bt, type);

	free_btab(bt);

	return bank_id;
}

/*
 * bankmgr_get_active_bank -- gets the bank structure of the active bank
 */
int bankmgr_get_active_bank(enum bank_type type, struct bank *bank)
{
	struct bank_table *bt;
	short bank_id;
	int ret = BANKMGR_ERR;

	if ((bt = load_btab()) == NULL)
		return ret;

	bank_id = __bankmgr_get_active_bank_id(bt, type);

	if (bank_id >= 0) {
		memcpy(bank, &bt->banks[bank_id], sizeof(*bank));
		ret = BANKMGR_NO_ERR;
	}

	free_btab(bt);

	return ret;
}

static short get_other_bank_id(struct bank_table *bt, enum bank_type type, short exclude_id)
{
	int i;
	short bank_id = -1;

	for (i = 0; i < bankmgr_get_num_banks(); i++) {
		if (ntohl(bt->banks[i].type) == type && i != exclude_id) {
			bank_id = i;
			break;
		}
	}

	return bank_id;
}

short bankmgr_get_bank_id(enum bank_type type, enum criterion criterion)
{
	struct bank_table *bt;
	short id = -1;

	if ((bt = load_btab()) == NULL)
		return BANKMGR_ERR;

	switch (criterion) {
	case BANK_ACTIVE:
		id = bankmgr_get_active_bank_id(type);
		break;
	case BANK_INACTIVE:
		id = get_other_bank_id(bt, type, bankmgr_get_active_bank_id(type));
		break;
	case BANK_BOOTED:
		id = bankmgr_get_booted_bank_id();
		break;
	case BANK_NOTBOOTED:
		id = get_other_bank_id(bt, type, bankmgr_get_booted_bank_id());
		break;
	default:
		break;
	}

	free_btab(bt);

	return id;
}

/*
 * bankmgr_get_bank_type -- returns the type of a bank or a value smaller
 *                          than 0 in case of failure.
 */
enum bank_type bankmgr_get_bank_type(short bank_id)
{
	struct bank_table *bt;
	enum bank_type type;

	if (bank_id < 0)
		return BANKMGR_ERR;

	if ((bt = load_btab()) == NULL)
		return BANKMGR_ERR;

	type = ntohl(bt->banks[bank_id].type);

	free_btab(bt);

	return type;
}

/*
 * bankmgr_get_bank_flags -- returns the flags of a bank or a value smaller
 *                          than 0 in case of failure.
 */
short bankmgr_get_bank_flags(short bank_id)
{
	struct bank_table *bt;
	short flags = -1;

	if (bank_id < 0)
		return BANKMGR_ERR;

	if ((bt = load_btab()) != NULL) {
		flags = ntohs(bt->banks[bank_id].flags);
		free_btab(bt);
	}

	return flags;
}

/*
 * bankmgr_get_bank_FVP -- returns the FVP of a bank or a value smaller
 *                          than 0 in case of failure.
 */
short bankmgr_get_bank_FVP(short bank_id)
{
	struct bank_table *bt;
	short FVP = -1;

	if (bank_id < 0)
		return BANKMGR_ERR;

	if ((bt = load_btab()) != NULL) {
		FVP = ntohs(bt->banks[bank_id].FVP);
		free_btab(bt);
	}

	return FVP;
}

