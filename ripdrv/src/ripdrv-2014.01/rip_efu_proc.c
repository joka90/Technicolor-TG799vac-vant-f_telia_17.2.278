/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2017 - Technicolor Delivery Technologies, SAS          **
** - All Rights Reserved                                                **
**                                                                      **
** Technicolor hereby informs you that certain portions                 **
** of this software module and/or Work are owned by Technicolor         **
** and/or its software providers.                                       **
** Distribution copying and modification of all such work are reserved  **
** to Technicolor and/or its affiliates, and are not permitted without  **
** express written authorization from Technicolor.                      **
** Technicolor is registered trademark and trade name of Technicolor,   **
** and shall not be used in any manner without express written          **
** authorization from Technicolor                                       **
**                                                                      **
*************************************************************************/

#define RIP_EFU_DIR_NAME "efu"
#define RIP_EFU_ALLOWED_NAME "allowed"
#define RIP_EFU_ALLOWED_BOOTLOADER_NAME "allowed_bootloader"
#define RIP_EFU_TEMPORAL_TAG_NAME "temporal_tag"

#define RIP_EFU_INFO_NAME "info"

#include <linux/version.h>
#include "rip2.h"
#include "rip2_common.h"
#include "rip_ids.h"
#include "rip_efu_proc.h"
#include "efu_privileged.h"

/**
 * Populate /proc/rip/info
 */
static void rip_efu_proc_print_tag_state(struct seq_file *m, unsigned int state) {
    switch (state) {
        case EFU_RET_NOTFOUND:
        case EFU_RET_RIPERR:
            seq_printf(m, "not present");
            break;
        case EFU_RET_SUCCESS:
            seq_printf(m, "valid");
            break;
        default:
            seq_printf(m, "invalid");
            break;
    }
}

static int rip_efu_proc_show(struct seq_file *m, void *v) {
  char * serial;
  EFU_CHIPID_TYPE chipid;
  EFU_BITMASK_TYPE mask;
  unsigned char osik[256];

  serial = EFU_getSerialNumber();
  if (serial) {
    seq_printf(m, "Serial: %s\n", serial);
    FREE(serial);
  }

  chipid = EFU_getChipid();
  seq_printf(m, "ChipID: 0x%X\n", chipid);

  EFU_getOSIK(osik, sizeof(osik));
  seq_printf(m, "Key   : %08X...\n", BETOH32(*(uint32_t *)osik));

  seq_printf(m, "\nPermanent tag: ");
  rip_efu_proc_print_tag_state(m, EFU_getBitmask(&mask, EFU_PERMANENT_TAG));

  seq_printf(m, "\nTemporal tag: ");
  rip_efu_proc_print_tag_state(m, EFU_getBitmask(&mask, EFU_TEMPORAL_TAG));
  seq_printf(m, "\n");

  return 0;
}

static void *rip_proc_data(struct file *file)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
	return PDE(file->f_path.dentry->d_inode)->data;
#else
	return PDE_DATA(file_inode(file));
#endif
}

static int rip_efu_info_proc_open(struct inode *inode, struct  file *file) {
  return single_open(file, rip_efu_proc_show, NULL);
}

static const struct file_operations rip_efu_proc_fops = {
  .owner = THIS_MODULE,
  .open = rip_efu_info_proc_open,
  .read = seq_read,
  .llseek = seq_lseek,
  .release = single_release,
};


typedef EFU_BITMASK_TYPE (*tag_retrieval_t)(void);

/**
 * Populate /proc/rip/temporal_tag
 */

/**
 * Handler for write system call on /proc/rip/temporal_tag
 * \param filp The file pointer corresponding to the proc fs entry the write
 * was performed on
 * \param buff A pointer to a buffer in userspace containig the data to write
 * \param len The length of the buffer in userspace
 * \param ppos Pointer position for the write function, mostly unused
 * \return length of the data that was written to the RIP item
 * \return -EFAULT if the write operation failed
 */
ssize_t rip_efu_temporal_tag_proc_write(struct file          *filp,
                       const char __user    *buff,
                       size_t               len,
                       loff_t               *ppos)
{
    char *buffer = NULL;

    int         ret;

    buffer = kmalloc(len, GFP_KERNEL);
    if (buffer == NULL) {
        return -EFAULT;
    }
    if (0 != copy_from_user(buffer, buff, len)) {
        kfree(buffer);
        return -EFAULT;
    }

    ret = EFU_storeTemporalTag(buffer, len);

    if (ret == EFU_RET_SUCCESS) {
        kfree(buffer);
        return len;
    }
    return -EFAULT;
}

static int rip_efu_temporal_tag_proc_open(struct inode *inode, struct  file *file) {
  return single_open(file, NULL, NULL);
}

static const struct file_operations rip_efu_temporal_tag_proc_fops = {
  .owner = THIS_MODULE,
  .open = rip_efu_temporal_tag_proc_open,
  .release = single_release,
  .write = rip_efu_temporal_tag_proc_write
};


/**
 * Helper to retrieve the tag which only the bootloader can see
 */
static EFU_BITMASK_TYPE rip_efu_get_blonly_tag(void) {
  EFU_BITMASK_TYPE result;

  if (EFU_getBitmask(&result, EFU_PERMANENT_TAG) != EFU_RET_SUCCESS) {
    return 0;
  }

  return result;
}

/**
 * Helper to retrieve the default SW tag
 */
static EFU_BITMASK_TYPE rip_efu_get_tag(void) {
  EFU_BITMASK_TYPE result;

  if (EFU_getBitmask(&result, EFU_DEFAULT_TAG) != EFU_RET_SUCCESS) {
    return 0;
  }

  return result;
}


/**
 * Populate the /proc/rip/allowed* API
 */
static int rip_efu_allowed_show(struct seq_file *m, void *v) {
  EFU_BITMASK_TYPE tag;
  tag_retrieval_t get_tag = (tag_retrieval_t)m->private;

  tag = get_tag();

  if (tag & EFU_SKIPSIGNATURECHECK_BIT)   seq_printf(m, EFU_SKIPSIGNATURECHECK_NAME "\n");
  if (tag & EFU_SKIPVARIANTIDCHECK_BIT)   seq_printf(m, EFU_SKIPVARIANTIDCHECK_NAME "\n");
  if (tag & EFU_DOWNGRADERESTRICTION_BIT) seq_printf(m, EFU_DOWNGRADERESTRICTION_NAME "\n");
  return 0;
}

static int rip_efu_allowed_proc_open(struct inode *inode, struct  file *file) {
  return single_open(file, rip_efu_allowed_show, rip_proc_data(file));
}

static const struct file_operations rip_efu_allowed_proc_fops = {
  .owner = THIS_MODULE,
  .open = rip_efu_allowed_proc_open,
  .read = seq_read,
  .llseek = seq_lseek,
  .release = single_release,
};



struct proc_dir_entry   *rip_efu_proc_dir;


/**
 * Creates the RIP EFU proc filesystem
 * \return 0 in case of success
 * \return -ENOMEM otherwise
 */
int rip_efu_proc_init(void)
{
    rip_efu_proc_dir = proc_mkdir(RIP_EFU_DIR_NAME, NULL);
    if (!rip_efu_proc_dir) {
        printk(KERN_ERR "Error: Could not initialize "RIP_EFU_DIR_NAME"\n");
        return -ENOMEM;
    }

    proc_create_data(RIP_EFU_INFO_NAME, 0444, rip_efu_proc_dir, &rip_efu_proc_fops, NULL);
    proc_create_data(RIP_EFU_ALLOWED_NAME, 0444, rip_efu_proc_dir, &rip_efu_allowed_proc_fops, &rip_efu_get_tag);
    proc_create_data(RIP_EFU_ALLOWED_BOOTLOADER_NAME, 0444, rip_efu_proc_dir, &rip_efu_allowed_proc_fops, &rip_efu_get_blonly_tag);
    proc_create_data(RIP_EFU_TEMPORAL_TAG_NAME, 0644, rip_efu_proc_dir, &rip_efu_temporal_tag_proc_fops, NULL);

    return 0;
}

/**
 * Remove all proc entries for the RIP EFU driver
 * \return 0
 */
int rip_efu_proc_cleanup(void)
{
    remove_proc_entry(RIP_EFU_INFO_NAME, rip_efu_proc_dir );
    remove_proc_entry(RIP_EFU_ALLOWED_NAME, rip_efu_proc_dir );
    remove_proc_entry(RIP_EFU_ALLOWED_BOOTLOADER_NAME, rip_efu_proc_dir );
    remove_proc_entry(RIP_EFU_TEMPORAL_TAG_NAME, rip_efu_proc_dir );


    remove_proc_entry(RIP_EFU_DIR_NAME, NULL);

    return 0;
}
