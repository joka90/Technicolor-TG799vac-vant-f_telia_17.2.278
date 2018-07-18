#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>

#include "storage_core.h"
#include "bankmgr_proc.h"
#include "bankmgr_p.h"
#include "legacy_upgrade_info.h"

static Image_ptr upgrade_info = NULL;
struct proc_dir_entry *legacy_upgrade_proc_dir = NULL;
struct proc_dir_entry *legacy_upgrade_erase = NULL;

static Image_item upgrade_url = UPGRADE_URL;
static Image_item upgrade_usr = UPGRADE_USER;
static Image_item upgrade_passwd = UPGRADE_PASSWORD;
static Image_item upgrade_key = UPGRADE_KEY;
static Image_item upgrade_file = UPGRADE_FILENAME;
static Image_item upgrade_flags = UPGRADE_FLAGS;

static int bankmgr_upgrade_info_proc_show(struct seq_file *m, void *v)
{
	Image_item *type = (Image_item *)(m->private);

	if(upgrade_info == NULL)
		return -1;

	switch (*type)
	{
		case UPGRADE_URL:
			seq_printf(m, "%s", upgrade_info->url);
			break;
		case UPGRADE_USER:
			seq_printf(m, "%s", upgrade_info->username);
			break;
		case UPGRADE_PASSWORD:
			seq_printf(m, "%s", upgrade_info->password);
			break;
		case UPGRADE_KEY:
			seq_printf(m, "%s", upgrade_info->key);
			break;
#ifdef SFS_DUAL_BANK_SUPPORT
		case UPGRADE_FILENAME:
			seq_printf(m, "%s", upgrade_info->downloadFileName);
			break;
#endif
		case UPGRADE_FLAGS:
			seq_printf(m, "%u", upgrade_info->flags);
			break;
		default:
			break;
	}

	return 0;
}

static void unpublish_info(struct proc_dir_entry *pdir)
{
	if (legacy_upgrade_proc_dir == NULL)
		return;
	remove_proc_entry(PROC_LEGACY_UPGRADE_URL, legacy_upgrade_proc_dir);
	remove_proc_entry(PROC_LEGACY_UPGRADE_USR, legacy_upgrade_proc_dir);
	remove_proc_entry(PROC_LEGACY_UPGRADE_PASSWD, legacy_upgrade_proc_dir);
	remove_proc_entry(PROC_LEGACY_UPGRADE_KEY, legacy_upgrade_proc_dir);
	remove_proc_entry(PROC_LEGACY_UPGRADE_FILE, legacy_upgrade_proc_dir);
	remove_proc_entry(PROC_LEGACY_UPGRADE_FLAGS, legacy_upgrade_proc_dir);
	if(pdir != NULL)
		remove_proc_entry(PROC_LEGACY_UPGRADE, pdir);
	legacy_upgrade_proc_dir = NULL;
	free_legacy_upgrade_info();
	return;
}


static int bankmgr_legacy_info_proc_show(struct seq_file *file, void *v)
{
	char value;
	if (upgrade_info == NULL)
		value = '1';
	else
		value = '0';

	seq_putc(file, value);
	return 0;
}

static ssize_t bankmgr_legacy_info_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	struct proc_dir_entry *pdir = (struct proc_dir_entry *)bankmgr_proc_data(file);
	struct storage_context *storage_ctx = NULL;
	unsigned char invalid_value[UPGRADED_STEP_VALUE_SIZE];
	int num = UPGRADED_STEP_VALUE_SIZE;

	if (count > 0)
	{
		if ((storage_ctx = get_storage_ctx()) == NULL )
			return 0;
#if RAWSTORAGE_VERSION > 1
		memset(invalid_value, 0xFF, UPGRADED_STEP_VALUE_SIZE);
		num = storage_set_param(storage_ctx, RS_ID_UPGRADE_STEP, invalid_value, UPGRADED_STEP_VALUE_SIZE);
#endif
		if(num != UPGRADED_STEP_VALUE_SIZE)
			return 0;
		unpublish_info(pdir);
	}

	return count;
}

BANKMGR_SEQ_FOPS(bankmgr_upgrade_info_proc_fops, bankmgr_upgrade_info_proc_show, NULL);
BANKMGR_SEQ_FOPS(bankmgr_legacy_info_proc_fops, bankmgr_legacy_info_proc_show, bankmgr_legacy_info_proc_write);

int load_legacy_upgrade_info(void)
{
	struct storage_context *storage_ctx = NULL;
	unsigned char *step_info;
	unsigned char invalid_value[UPGRADED_STEP_VALUE_SIZE];
	int size = 0;

	if (upgrade_info != NULL)
	{
		return 0;
	}

	if ((storage_ctx = get_storage_ctx()) == NULL )
		return -1;

	size = storage_get_param(storage_ctx, RS_ID_UPGRADE_STEP, NULL, 0);
	if (size != 0)
	{
		if (size != UPGRADED_STEP_VALUE_SIZE)
			return -1;
		memset(invalid_value, 0xFF, UPGRADED_STEP_VALUE_SIZE);
		step_info = kmalloc(size, GFP_KERNEL);
		if (!step_info)
			return -1;
		if (storage_get_param(storage_ctx, RS_ID_UPGRADE_STEP, step_info, size) != size || memcmp(step_info, invalid_value, size) == 0)
		{
			kfree(step_info);
			return -1;
		}
	}

	size = storage_get_param(storage_ctx, RS_ID_IMAGE_STRUCT, NULL, 0);
	if (size == 0 || size < sizeof(Image))
		return -1;

	upgrade_info = kmalloc(size, GFP_KERNEL);
	if (upgrade_info)
	{
		if (storage_get_param(storage_ctx, RS_ID_IMAGE_STRUCT, upgrade_info, size) != size)
		{
			kfree(upgrade_info);
			upgrade_info = NULL;
			return -1;
		}
	}
	else
		return -1;

	return 0;
}

void free_legacy_upgrade_info(void)
{
	if (upgrade_info)
	{
		kfree(upgrade_info);
		upgrade_info = NULL;
	}
	return;
}

void publish_legacy_upgrade_info(void)
{
	struct proc_dir_entry *entry = NULL;
	struct proc_dir_entry *pdir = bankmgr_proc_get_proc_root_dir();

	if (upgrade_info == NULL || pdir == NULL)
		return;

	legacy_upgrade_proc_dir = proc_mkdir(PROC_LEGACY_UPGRADE, pdir);
	if(legacy_upgrade_proc_dir == NULL)
	{
		free_legacy_upgrade_info();
		return;
	}

	entry = proc_create_data(PROC_LEGACY_UPGRADE_URL, 0444, legacy_upgrade_proc_dir, &bankmgr_upgrade_info_proc_fops, &upgrade_url);
	entry = proc_create_data(PROC_LEGACY_UPGRADE_USR, 0444, legacy_upgrade_proc_dir, &bankmgr_upgrade_info_proc_fops, &upgrade_usr);
	entry = proc_create_data(PROC_LEGACY_UPGRADE_PASSWD, 0444, legacy_upgrade_proc_dir, &bankmgr_upgrade_info_proc_fops, &upgrade_passwd);
	entry = proc_create_data(PROC_LEGACY_UPGRADE_KEY, 0444, legacy_upgrade_proc_dir, &bankmgr_upgrade_info_proc_fops, &upgrade_key);
	entry = proc_create_data(PROC_LEGACY_UPGRADE_FILE, 0444, legacy_upgrade_proc_dir, &bankmgr_upgrade_info_proc_fops, &upgrade_file);
	entry = proc_create_data(PROC_LEGACY_UPGRADE_FLAGS, 0444, legacy_upgrade_proc_dir, &bankmgr_upgrade_info_proc_fops, &upgrade_flags);

	legacy_upgrade_erase = proc_create_data(PROC_LEGACY_UPGRADE_ERASE, 0644, pdir, &bankmgr_legacy_info_proc_fops, pdir);

	return;
}

void unpublish_legacy_upgrade_info(void)
{
	struct proc_dir_entry *pdir = bankmgr_proc_get_proc_root_dir();
	unpublish_info(pdir);
	if(pdir != NULL && legacy_upgrade_erase != NULL)
		remove_proc_entry(PROC_LEGACY_UPGRADE_ERASE, pdir);
	return;
}
