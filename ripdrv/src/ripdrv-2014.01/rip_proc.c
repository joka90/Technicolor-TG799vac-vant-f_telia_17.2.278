/************** COPYRIGHT AND CONFIDENTIALITY INFORMATION *********************
**                                                                          **
** Copyright (c) 2012 Technicolor                                           **
** All Rights Reserved                                                      **
**                                                                          **
** This program contains proprietary information which is a trade           **
** secret of TECHNICOLOR and/or its affiliates and also is protected as     **
** an unpublished work under applicable Copyright laws. Recipient is        **
** to retain this program in confidence and is not permitted to use or      **
** make copies thereof other than as permitted in a written agreement       **
** with TECHNICOLOR, UNLESS OTHERWISE EXPRESSLY ALLOWED BY APPLICABLE LAWS. **
**                                                                          **
******************************************************************************/

/* parts of this file are wrapped in a ifndef USERSPACE
 * This is done to exclude those parts from the compilation of the unit tests.
 * This allows us to write test for some generic parts in this file.
 */
#include <linux/version.h>
#include <linux/string.h>
#include "rip2.h"
#include "rip2_common.h"
#include "rip2_cert_privkey.h"
#include "rip_ids.h"
#include "rip_proc.h"

struct proc_dir_entry   *rip_proc_dir;
struct rip_proc_list    rip_proc_entries;

/**
 * This struct is used to cache parameters which are too big to fit
 * into the buffer passed from userspace.
 */
struct rip_cache read_cache;

#define PROC_BLOCK_SIZE    (PAGE_SIZE - 1024) //taken from fs/proc/generic.c

static void rip_proc_set_size(struct proc_dir_entry *entry, size_t len)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
	entry->size = len;
#else
	proc_set_size(entry, (loff_t)len);
#endif
}

static void *rip_proc_data(struct file *file)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
	return PDE(file->f_path.dentry->d_inode)->data;
#else
	return PDE_DATA(file_inode(file));
#endif
}

#define _RIP_SEQ_FOPS(fops_name, _show, _read, _write)                \
static int fops_name##_open(struct inode *inode, struct file *file)     \
{                                                                       \
	return single_open(file, _show, rip_proc_data(file));           \
}                                                                       \
static struct file_operations fops_name = {                             \
	.owner   = THIS_MODULE,                                         \
	.open    = fops_name##_open,                                    \
	.read    = _read,						\
	.write   = _write,                                              \
	.release = single_release,                                      \
}

#define RIP_SEQ_FOPS(fops_name, _show, _write) _RIP_SEQ_FOPS(fops_name, _show, seq_read, _write)
#define RIP_SEQ_FOPS_WO(fops_name, _write) _RIP_SEQ_FOPS(fops_name, NULL, NULL, _write)

/**
 * Convert an input string with a hex formatted number to an uint16
 * \param input a string containing the hex formatted number
 * \return [0x0-0xFFFE] The converted uint16 if successful
 * \return 0xFFFF In case of error
 */
static uint16_t str_to_hex(char *input)
{
	uint16_t result;

	if (sscanf(input, "%hx", &result) == 1) {
		return result;
	}
	return 0xFFFF;
}

/**
 * Add a new entry to the RIP proc filesystem and add it to the linked list
 * that keeps track of all RIP proc entries.
 * Note that this function does not modify the RIP. The RIP will be modified
 * once a write operation on the newly create proc fs entry is performed
 * \param name The name of the new node, needs to be string representation of
 * a hex number between 0x0 and 0xFFFE
 * \param mode Integer representation of unix file permissions
 * \param fops File operations for the proc - read/write/etc
 * \param set_data Integer to decide whether to save the id in the proc data
 * \return pointer to a rip_proc_list entry containing all info on the newly
 * created proc entry
 * \return NULL in case of error
 */
static struct rip_proc_list *rip_proc_add_entry(char                    *name,
                                                int                     mode,
                                                struct file_operations  *fops,
                                                int                     set_data)
{
	struct rip_proc_list *tmp = NULL;

	tmp = (struct rip_proc_list *)kmalloc(sizeof(struct rip_proc_list), GFP_KERNEL);
	if (!tmp) {
		printk(KERN_ERR "Error: Could not allocate /proc/rip/...\n");
		return NULL;
	}
	snprintf(tmp->name, RIP_NAME_SZ, "%s", name);

	/* create the new proc entry */
	tmp->entry = proc_create_data(tmp->name, mode, rip_proc_dir, fops, set_data ? &tmp->id : NULL);
	if (!tmp->entry) {
		printk(KERN_ERR "Error: Could not initialize /proc/rip/%s\n", tmp->name);
		kfree(tmp);
		return NULL;
	}

	/* add it to our linked list */
	list_add(&tmp->list, &rip_proc_entries.list);
	return tmp;
}

/**
 * Remove an entry to the RIP proc filesystem and delete it from the linked list
 * that keeps track of all RIP proc entries.
 * \param name The name of the node to be deleted
 * \return 1 in case of success
 * \return 0 in case of error
 */
static int rip_proc_delete_entry(char *name)
{
	struct list_head        *pos, *q;
	struct rip_proc_list    *tmp;

	list_for_each_safe(pos, q, &rip_proc_entries.list)
	{
		tmp = list_entry(pos, struct rip_proc_list, list);
		if (strncmp(tmp->name, name, RIP_NAME_SZ) == 0) {
			remove_proc_entry(tmp->name, rip_proc_dir);
			list_del(pos);
			kfree(tmp);
			return 1;
		}
	}
	return 0;
}

/**
 * Find an entry in the linked list of proc entries based on the RIP ID it
 * represents.
 * \param name The name of the node to be found
 * \return rip_proc_list entry in case of success
 * \return NULL in case of error
 */
static struct rip_proc_list *rip_proc_find_entry(char *name)
{
	struct list_head        *pos, *q;
	struct rip_proc_list    *tmp;

	list_for_each_safe(pos, q, &rip_proc_entries.list)
	{
		tmp = list_entry(pos, struct rip_proc_list, list);
		if (strncmp(tmp->name, name, RIP_NAME_SZ) == 0) {
			return tmp;
		}
	}
	return NULL;
}

static int rip_proc_show(struct seq_file *m, void *v)
{
	unsigned long length;
	T_RIP2_ID     id = *(T_RIP2_ID *)m->private;
	char          *buffer = NULL;
	int           ret;

	/* Get entry size for allocation */
	if (rip2_drv_read_length(&length, id) != RIP2_SUCCESS) {
		return -EFAULT;
	}

	buffer = kmalloc(length, GFP_KERNEL);
	if (buffer == NULL) {
		return -ENOMEM;
	}

	/* Get the RIP parameter */
	if (rip2_drv_read(&length, id, buffer) != RIP2_SUCCESS) {
		kfree(buffer);
		return -EFAULT;
	}

	ret = seq_write(m, buffer, (size_t)length);

#ifndef CONFIG_RIPDRV_ANVIL
	/* append a \n
	 * For Anvil it complicates things a lot, so we do not do it.
	 */
	if (!ret) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,3,0)
		ret = seq_putc(m, '\n');
#else
		seq_putc(m, '\n');
#endif
	}
#endif

	kfree(buffer);
	return ret;
}

/**
 * Handler for write system call on regular RIP proc filesystem entries.
 * \param filp The file pointer corresponding to the proc fs entry the write
 * was performed on
 * \param buff A pointer to a buffer in userspace containig the data to write
 * \param len The length of the buffer in userspace
 * \param ppos Pointer position for the write function, mostly unused
 * \return length of the data that was written to the RIP item
 * \return -EFAULT if the write operation failed
 */
ssize_t rip_proc_write(struct file          *filp,
                       const char __user    *buff,
                       size_t               len,
                       loff_t               *ppos)
{
	char *buffer = NULL;
	char name[RIP_NAME_SZ];
	T_RIP2_ID   id = *(T_RIP2_ID *)rip_proc_data(filp);
	int         ret;

	/* sanity check, the actual RIP write will also check if there is enough free
	   space available */
	if (len > RIP2_SZ - 1) {
		return -EFAULT;
	}
	buffer = kmalloc(len, GFP_KERNEL);
	if (buffer == NULL) {
		return -EFAULT;
	}
	if (0 != copy_from_user(buffer, buff, len)) {
		kfree(buffer);
		return -EFAULT;
	}

	snprintf(name, RIP_NAME_SZ, "%4.4x", id);
	ret = rip2_drv_write(buffer,
	                     len,
	                     id,
	                     RIP2_ATTR_DEFAULT | RIP2_ATTR_WRITABLE,
	                     RIP2_ATTR_ANY);
	if (ret == RIP2_SUCCESS) {
		struct rip_proc_list *tmp = rip_proc_find_entry(name);
		if (tmp == NULL) {
			goto out_err;
		}
		rip_proc_set_size(tmp->entry, len);
		tmp->len = len;
		kfree(buffer);
		return len;
	}
out_err:
	kfree(buffer);
	return -EFAULT;
}

RIP_SEQ_FOPS(rip_proc_fops, rip_proc_show, rip_proc_write);
RIP_SEQ_FOPS(rip_proc_read_fops, rip_proc_show, NULL);

/**
 * Handler for write system call on the "new" entry in the RIP proc filesystem.
 * Writing a valid rip2 id will add a new entry to the RIP.
 * \param filp The file pointer corresponding to the proc fs entry the write
 * was performed on
 * \param buff A pointer to a buffer in userspace containig the data to write.
 * In this case the ID of the new RIP entry to create.
 * \param len The length of the buffer in userspace
 * \param ppos Pointer position for the write function, mostly unused
 * \return length of the buffer in userspace
 * \return -EFAULT or -ENOMEM if the operation failed
 */
ssize_t rip_proc_new(struct file        *filp,
                     const char __user  *buff,
                     size_t             len,
                     loff_t             *ppos)
{
	char                    buffer[RIP_BUF_SZ];
	struct rip_proc_list    *tmp = NULL;
	T_RIP2_ID               id;

	if ((len > RIP_BUF_SZ - 1) || (0 != copy_from_user(buffer, buff, len))) {
		return -EFAULT;
	}

	buffer[RIP_NAME_SZ] = '\0'; //strip off trailing chars e.g. "\n"
	id = str_to_hex(buffer);
	if (id == 0xFFFF) {
		return -EFAULT;
	}
	snprintf(buffer, RIP_NAME_SZ, "%4.4x", id);

#if !defined(CONFIG_RIPDRV_ANVIL)
	// Do not expose the client certificate which includes the private key.
	if (id == RIP_ID_CLIENT_CERTIFICATE) {
		return -EACCES;
	}
#if defined(CONFIG_RIPDRV_CRYPTO_SUPPORT)
	// Do not expose OSCK.
	if (id == RIP_ID_OSCK) {
		return -EACCES;
	}
#endif
#endif
	// check to see if index already exists!
	if (NULL != rip_proc_find_entry(buffer)) {
		/* Adding an existing ID should not result in error, so return len */
		return len;
	}

	tmp = rip_proc_add_entry(buffer, 0644, &rip_proc_fops, 1);
	if (tmp == NULL) {
		return -ENOMEM;
	}
	tmp->id = (T_RIP2_ID)str_to_hex(tmp->name);

	if (tmp->id == 0xFFFF) {
		remove_proc_entry(tmp->name, rip_proc_dir);
		kfree(tmp);
		return -EFAULT;
	}

	return len;
}

/**
 * Handler for write system call on the "lock" entry in the RIP proc filesystem.
 * Writing a valid rip2 id that corresponds to and existing entry in the RIP
 * with writeable flag set will unset the writable flag and update the proc
 * filesystem entry.
 * \param filp The file pointer corresponding to the proc fs entry the write
 * was performed on
 * \param buff A pointer to a buffer in userspace containig the data to write.
 * In this case the ID of the RIP entry to lock.
 * \param len The length of the buffer in userspace
 * \param ppos Pointer position for the write function, mostly unused
 * \return length of the data that was written to the RIP item
 * \return -EFAULT if the operation failed
 */
ssize_t rip_proc_lock(struct file       *filp,
                      const char __user *buff,
                      size_t             len,
                      loff_t             *ppos)
{
	char                    buffer[RIP_BUF_SZ];
	T_RIP2_ID               id = 0;
	struct rip_proc_list    *tmp;
	int                     ret;

	if ((len > RIP_BUF_SZ - 1) || (0 != copy_from_user(buffer, buff, len))) {
		return -EFAULT;
	}
	buffer[RIP_NAME_SZ] = '\0'; //strip off trailing chars e.g. "\n"
	id = str_to_hex(buffer);
	if (id == 0xFFFF) {
		return -EFAULT;
	}
	snprintf(buffer, RIP_NAME_SZ, "%4.4x", id);

	ret = rip2_lock(id);
	if (RIP2_SUCCESS == ret) {
		T_RIP2_ITEM item;

		if (RIP2_SUCCESS != rip2_get_idx(id, &item)) {
			return -EFAULT; // should never happen, rip2_lock would have failed
		}

		rip_proc_delete_entry(buffer);

		tmp = rip_proc_add_entry(buffer, 0444, &rip_proc_read_fops, 1);
		if (tmp == NULL) {
			return -EFAULT;
		}
		rip_proc_set_size(tmp->entry, BETOH32(item.length));
		tmp->len = BETOH32(item.length);
		tmp->id = id;
	}
	return len;
}

#if defined(CONFIG_RIPDRV_CRYPTO_SUPPORT)

/**
 * Handler for write system call on the "signed" entry in the RIP proc filesystem.
 * Writing a valid rip2 id that corresponds to an existing item in the RIP
 * will check the signature in the content using EIK and set the EIK_SIGN flag.
 *
 * \param filp The file pointer corresponding to the 'signed' entry in proc fs.
 * \param buff A pointer to a buffer in userspace, containing the rip2 ID.
 * \param len The length of the buffer in userspace.
 * \param ppos Pointer position for the write function, mostly unused
 * \return length of the buffer in userspace
 * \return -EFAULT if the operation failed
 */
static ssize_t rip_proc_signed(struct file          *filp,
                               const char __user    *buff,
                               size_t               len,
                               loff_t               *ppos)
{
	char        buffer[RIP_BUF_SZ];
	T_RIP2_ID   id;

	if ((len > RIP_BUF_SZ - 1) || (0 != copy_from_user(buffer, buff, len))) {
		return -EFAULT;
	}
	buffer[RIP_NAME_SZ] = '\0'; //strip off trailing chars e.g. "\n"
	id = str_to_hex(buffer);
	if (id == 0xFFFF) {
		return -EFAULT;
	}
	snprintf(buffer, RIP_NAME_SZ, "%4.4x", id);

	if (rip2_set_signed(id) != RIP2_SUCCESS) {
		return -EFAULT;
	}

	DBG("%s: success\n", __func__);
	return len;
}

#if !defined(CONFIG_RIPDRV_INTEGRITY_ONLY)
/**
 * Handler for write system call on the "encrypt" entry in the RIP proc filesystem.
 * Writing a valid rip2 id that corresponds to an existing item in the RIP
 * will encrypt the content using ECK, and set the ECK_ENCR flag.
 * For encryption be allowed, the rip2 item should already have EIK_SIGN flag set.
 *
 * \param filp The file pointer corresponding to the 'encrypt' entry in proc fs.
 * \param buff A pointer to a buffer in userspace, containing the rip2 ID.
 * \param len The length of the buffer in userspace.
 * \param ppos Pointer position for the write function, mostly unused
 * \return length of the buffer in userspace
 * \return -EFAULT if the operation failed
 */
static ssize_t rip_proc_encrypt(struct file         *filp,
                                const char __user   *buff,
                                size_t              len,
                                loff_t              *ppos)
{
	char                    buffer[RIP_BUF_SZ];
	T_RIP2_ID               id;
	T_RIP2_ITEM             item;
	struct rip_proc_list    *tmp;

	if ((len > RIP_BUF_SZ - 1) || (0 != copy_from_user(buffer, buff, len))) {
		return -EFAULT;
	}
	buffer[RIP_NAME_SZ] = '\0'; //strip off trailing chars e.g. "\n"
	id = str_to_hex(buffer);
	if (id == 0xFFFF) {
		return -EFAULT;
	}
	snprintf(buffer, RIP_NAME_SZ, "%4.4x", id);

	if ((rip2_encrypt(id) != RIP2_SUCCESS) ||
	    (rip2_get_idx(id, &item) != RIP2_SUCCESS) ||
	    ((tmp = rip_proc_find_entry(buffer)) == NULL)) {
		return -EFAULT;
	}
	rip_proc_set_size(tmp->entry, BETOH32(item.length));
	tmp->len = BETOH32(item.length);

	DBG("%s: success\n", __func__);
	return len;
}

RIP_SEQ_FOPS_WO(rip_proc_encrypt_fops, rip_proc_encrypt);
#endif /* CONFIG_RIPDRV_INTEGRITY_ONLY */
RIP_SEQ_FOPS_WO(rip_proc_signed_fops, rip_proc_signed);
#endif /* CONFIG_RIPDRV_CRYPTO_SUPPORT */

RIP_SEQ_FOPS_WO(rip_proc_new_fops, rip_proc_new);
RIP_SEQ_FOPS_WO(rip_proc_lock_fops, rip_proc_lock);


/* This function take the proc file name as the input parameter,
   use it to get the corresponding RIP ID,
   and then read out the corresponding RIP content inside FLASH
   store the reference pointer in read_cache.data
*/
// Important: caller of this function should take the read_cache.lock
static int cache_rip_data(char *proc_entry_name)
{
	unsigned long   length;
	T_RIP2_ID id;
	struct rip_proc_list *tmp = rip_proc_find_entry(proc_entry_name);

	if (tmp == NULL) {
		return -1;
	}
	id = tmp->id;

	/* Clean up read cache from previous iteration */
	if (read_cache.data != NULL) {
		kfree(read_cache.data);
		read_cache.id   = 0;
		read_cache.data = NULL;
	}
	read_cache.data = kmalloc(tmp->len, GFP_KERNEL);
	if (read_cache.data == NULL) {
		return -1;
	}
	/* Get the RIP parameter in the read cache */
	length = tmp->len;
	if (rip2_drv_read(&length, id, read_cache.data) == RIP2_SUCCESS) {
		read_cache.id   = HTOBE16(id);
		read_cache.len  = length;
	}
	else {
		kfree(read_cache.data);
		read_cache.id   = 0;
		read_cache.data = NULL;
		return -1;
	}
	return 0;
}

// rip_seq_start/next/stop/show are seq_file callback functions for generic rip proc files

static void *rip_seq_start(struct seq_file  *s,
                           loff_t           *pos)
{
	T_RIP2_ID     id = *(T_RIP2_ID *)s->private;
	char          name[RIP_NAME_SZ];

	snprintf(name, RIP_NAME_SZ, "%4.4x", id);
	mutex_lock(&read_cache.lock);
	if (cache_rip_data(name))
		return NULL;

	if (*pos >= read_cache.len) {
		/* Cache is empty */
#ifndef CONFIG_RIPDRV_ANVIL
		/* append a \n
		* For Anvil it complicates things a lot, so we do not do it.
		*/
		seq_putc(s, '\n');
#endif
		return NULL;
	}
	return pos;
}

static void *rip_seq_next(struct seq_file   *s,
                          void              *v,
                          loff_t            *pos)
{
	(*pos)++;
	if (*pos >= read_cache.len) {
		return NULL;
	}
	return pos;
}

static void rip_seq_stop(struct seq_file    *s,
                         void               *v)
{
	/* The sequence has ended, release the mutex and free the cache */
	if (read_cache.data != NULL) {
		kfree(read_cache.data);
		read_cache.id   = 0;
		read_cache.data = NULL;
	}
	mutex_unlock(&read_cache.lock);
}

static int rip_seq_show(struct seq_file *s,
                        void            *v)
{
	unsigned int i = *(loff_t *) v;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,3,0)
	return seq_putc(s, read_cache.data[i]);
#else
	seq_putc(s, read_cache.data[i]);
	return 0;
#endif
}

static struct seq_operations rip_seq_ops =
{
	.start  = rip_seq_start,
	.next   = rip_seq_next,
	.stop   = rip_seq_stop,
	.show   = rip_seq_show
};

/**
 * This function is called when the large proc file is opened.
 */
static int rip_seq_open(struct inode    *inode,
                        struct file     *file)
{
	int             ret;
        struct seq_file *s;

	ret = seq_open(file, &rip_seq_ops);

	/* file->private_data was initialised by seq_open */
	s = (struct seq_file *)file->private_data;
	s->private = rip_proc_data(file);

        return ret;
}

/**
 * This structure gathers functions that manage the large proc files
 */
static struct file_operations seq_file_ops =
{
	.owner      = THIS_MODULE,
	.open       = rip_seq_open,
	.read       = seq_read,
	.llseek     = seq_lseek,
	.release    = seq_release
};


/* When CONFIG_RIPDRV_SEPARATE_CERT_PRIVKEY is defined, the client certificate and RSA private key inside RIP 0x11a
   are separated and published in two different files /proc/rip/011a.cert and /proc/rip/011a.privkey (if CONFIG_RIPDRV_EXPOSE_PRIVATES is defined).
   Otherwise, they will be published together inside a single file /proc/rip/011a (if CONFIG_RIPDRV_EXPOSE_PRIVATES is defined).
*/
#if defined(CONFIG_RIPDRV_SEPARATE_CERT_PRIVKEY)

//rip_cert_privkey_seq_start, rip_seq_next/show/stop are seq_file callbacks for 011a.cert and 011a.privkey proc files

static void *rip_cert_privkey_seq_start(struct seq_file  *s,
                           loff_t           *pos)
{
	char name[RIP_NAME_SZ];
	CERT_PRIVKEY_TYPE data_type = (CERT_PRIVKEY_TYPE)s->private;
	char *tmp = NULL;
	uint32_t offset = 0, len = 0;

	mutex_lock(&read_cache.lock);

	switch(data_type){
	case CERT:
		snprintf(name, RIP_NAME_SZ, "%4.4x.cert", RIP_ID_CLIENT_CERTIFICATE);
		break;
#if defined(CONFIG_RIPDRV_EXPOSE_PRIVATES)
	case PRIVKEY:
		snprintf(name, RIP_NAME_SZ, "%4.4x.privkey", RIP_ID_CLIENT_CERTIFICATE);
		break;
#endif
	default:
		return NULL;
	}
	if (cache_rip_data(name))
		return NULL;

	// Manipulate cache to get separated CERT or PRIVKEY
	if (rip2_get_cert_privkey_position(read_cache.data, read_cache.len, data_type, &offset, &len))
		return NULL;
	else
	{
		tmp = kmalloc(len, GFP_KERNEL);
		if (tmp == NULL)
			return NULL;
		memcpy(tmp, read_cache.data + offset, len);
		kfree(read_cache.data);
		read_cache.data = tmp;
		read_cache.len = len;
	}

	if (*pos >= read_cache.len) {
		/* Cache is empty */
#ifndef CONFIG_RIPDRV_ANVIL
		/* append a \n
		* For Anvil it complicates things a lot, so we do not do it.
		*/
		seq_putc(s, '\n');
#endif
		return NULL;
	}
	return pos;
}

static struct seq_operations rip_cert_privkey_seq_ops =
{
	.start  = rip_cert_privkey_seq_start,
	.next   = rip_seq_next,
	.stop   = rip_seq_stop,
	.show   = rip_seq_show
};

static int rip_cert_privkey_seq_open(struct inode    *inode,
                        struct file     *file)
{
	int             ret;
	struct seq_file *s;

	ret = seq_open(file, &rip_cert_privkey_seq_ops);

	/* file->private_data was initialised by seq_open */
	s = (struct seq_file *) file->private_data;
	s->private = rip_proc_data(file);

        return ret;
}

static struct file_operations seq_fops_read_cert_privkey =
{
	.owner      = THIS_MODULE,
	.open       = rip_cert_privkey_seq_open,
	.read       = seq_read,
	.llseek     = seq_lseek,
	.release    = seq_release
};

/* Create proc file entries for certificate or RSA private key inside 0x11a rip
   param data_type: The data_type inside 0x11a to be published, either CERT or PRIVKEY
   return:          return NULL if failed, otherwise, a pointer to rip_proc_list entry
*/
static struct rip_proc_list *rip_proc_add_entry_separate_cert_privkey(CERT_PRIVKEY_TYPE data_type)
{
	char name[RIP_NAME_SZ];
	struct rip_proc_list *tmp = NULL;

	switch(data_type){
	case CERT:
		snprintf(name, RIP_NAME_SZ, "%4.4x.cert", RIP_ID_CLIENT_CERTIFICATE);
		break;
	case PRIVKEY:
		snprintf(name, RIP_NAME_SZ, "%4.4x.privkey", RIP_ID_CLIENT_CERTIFICATE);
		break;
	default:
		return NULL;
	}

	tmp = (struct rip_proc_list *)kmalloc(sizeof(struct rip_proc_list), GFP_KERNEL);
	if (!tmp) {
		printk(KERN_ERR "Error: Could not allocate /proc/rip/...\n");
		return NULL;
	}
	snprintf(tmp->name, RIP_NAME_SZ, "%s", name);

	/* create the new proc entry */
	//Right now only support read operation
	tmp->entry = proc_create_data(tmp->name, 0400, rip_proc_dir, &seq_fops_read_cert_privkey, (void *)data_type);
	if (!tmp->entry) {
		printk(KERN_ERR "Error: Could not initialize /proc/rip/%s\n", tmp->name);
		kfree(tmp);
		return NULL;
	}

	/* add it to our linked list */
	list_add(&tmp->list, &rip_proc_entries.list);
	return tmp;
}

// Read out RIP_ID_CLIENT_CERTIFICATE(0x11a), and publish the client certificate and RSA private key inside /proc/rip/
static void rip_proc_separate_cert_privkey_entry(void)
{
	T_RIP2_ITEM eripv2_item;
	struct rip_proc_list *tmp = NULL;

	if (rip2_get_item(RIP_ID_CLIENT_CERTIFICATE, &eripv2_item) != RIP2_SUCCESS)
	{
		printk(KERN_ERR "Error: Could not find CLIENT_CERT inside RIP!\n");
		return;
	}

	tmp = rip_proc_add_entry_separate_cert_privkey(CERT);
	if (tmp != NULL) {
		rip_proc_set_size(tmp->entry, eripv2_item.length);
		tmp->len = eripv2_item.length;
		tmp->id = eripv2_item.ID;
	}

#if defined(CONFIG_RIPDRV_EXPOSE_PRIVATES)
	tmp = rip_proc_add_entry_separate_cert_privkey(PRIVKEY);
	if (tmp != NULL) {
		rip_proc_set_size(tmp->entry, eripv2_item.length);
		tmp->len = eripv2_item.length;
		tmp->id = eripv2_item.ID;
	}
#endif

	return;
}
#endif   /* CONFIG_RIPDRV_SEPARATE_CERT_PRIVKEY */

/**
 * Creates the RIP proc filesystem
 * \return 0 in case of success
 * \return -ENOMEM otherwise
 */
int rip_proc_init(void)
{
	T_RIP2_ITEM             eripv2_item, *it;
	struct rip_proc_list    *tmp;

	INIT_LIST_HEAD(&rip_proc_entries.list);

	memset(&read_cache, 0, sizeof(read_cache));
	mutex_init(&read_cache.lock);

	// Create the proc entry for the RIP parent directory.
	rip_proc_dir = proc_mkdir(RIP_DIR_NAME, NULL);
	if (!rip_proc_dir) {
		printk(KERN_ERR "Error: Could not initialize /proc/rip\n");
		return -ENOMEM;
	}

	// Create the proc entries for the RIP items.
	it = 0;
	while (rip2_get_next(&it, RIP2_ATTR_ANY, &eripv2_item) == RIP2_SUCCESS) {
		unsigned                perm;
		struct file_operations  *fops;
		int                     need_write_func = 0;
		char                    name[RIP_NAME_SZ];

#if !defined(CONFIG_RIPDRV_ANVIL)
#if defined(CONFIG_RIPDRV_CRYPTO_SUPPORT)
		// These are skipped
		if (~BETOH32(eripv2_item.attr[ATTR_HI]) & (RIP2_ATTR_N_BEK_ENCR | RIP2_ATTR_N_MCV_SIGN)) {
			continue;
		}
#else
		// If no crypto support, hide any crypted parameters
		if (~BETOH32(eripv2_item.attr[ATTR_HI]) & RIP2_ATTR_CRYPTO) {
			continue;
		}
#endif
#if !defined(CONFIG_RIPDRV_EXPOSE_PRIVATES) || defined(CONFIG_RIPDRV_SEPARATE_CERT_PRIVKEY)
		if (BETOH16(eripv2_item.ID) == RIP_ID_CLIENT_CERTIFICATE) {
			// If CONFIG_RIPDRV_EXPOSE_PRIVATES is NOT defined, do not expose the client certificate with the private key
			// If CONFIG_RIPDRV_SEPARATE_CERT_PRIVKEY defined, process RIP_ID_CLIENT_CERTIFICATE later, not using generic rip proc functions
			continue;
		}
#endif
#if defined(CONFIG_RIPDRV_CRYPTO_SUPPORT) && !defined(CONFIG_RIPDRV_INTEGRITY_ONLY)
		// Never expose OSCK; access should happen via keymanager/SPF.
		if (BETOH16(eripv2_item.ID) == RIP_ID_OSCK) {
			continue;
		}
#endif
#endif  /* CONFIG_RIPDRV_ANVIL */

		sprintf(name, "%4.4x", BETOH16(eripv2_item.ID));

		/* Crypted RIP items are readable by root only */
		perm = (~BETOH32(eripv2_item.attr[ATTR_HI]) & RIP2_ATTR_CRYPTO) ? 0400 : 0444;
		/* Writable RIP items are writable by root only */
		if (BETOH32(eripv2_item.attr[ATTR_HI]) & RIP2_ATTR_WRITABLE) {
			perm |= 0200;
		}
#if !defined(CONFIG_RIPDRV_ANVIL)
		if (BETOH32(eripv2_item.attr[ATTR_HI]) & RIP2_ATTR_WRITABLE)
#endif
		{
			need_write_func = 1;
		}

		// Special case to handle files bigger than 3072 bytes
		if (BETOH32(eripv2_item.length) > PROC_BLOCK_SIZE) {
			fops = &seq_file_ops;
		}
		else {
			if (need_write_func) {
				fops = &rip_proc_fops;
			}
			else {
				fops = &rip_proc_read_fops;
			}
		}

		tmp = rip_proc_add_entry(name, perm, fops, 1);
		if (tmp == NULL) {
			continue;
		}

		rip_proc_set_size(tmp->entry, BETOH32(eripv2_item.length));
		tmp->len = BETOH32(eripv2_item.length);
		tmp->id             = BETOH16(eripv2_item.ID);
	}

	// Add special proc entries "lock" and "new"
	tmp = rip_proc_add_entry("new", 0644, &rip_proc_new_fops, 0);
	if (tmp == NULL) {
		goto out_err;
	}

	tmp = rip_proc_add_entry("lock", 0644, &rip_proc_lock_fops, 0);
	if (tmp == NULL) {
		goto out_err;
	}

#if defined(CONFIG_RIPDRV_CRYPTO_SUPPORT)
	// Add special proc entry "signed"
	tmp = rip_proc_add_entry("signed", 0644, &rip_proc_signed_fops, 0);
	if (tmp == NULL) {
		goto out_err;
	}
#if !defined(CONFIG_RIPDRV_INTEGRITY_ONLY)
	// Add special proc entry "encrypt"
	tmp = rip_proc_add_entry("encrypt", 0644, &rip_proc_encrypt_fops, 0);
	if (tmp == NULL) {
		goto out_err;
	}
#endif /* CONFIG_RIPDRV_INTEGRITY_ONLY */
#endif /* CONFIG_RIPDRV_CRYPTO_SUPPORT */

#if defined(CONFIG_RIPDRV_SEPARATE_CERT_PRIVKEY)
	rip_proc_separate_cert_privkey_entry();
#endif

	return 0;

out_err:
	/* Cleanup the proc filesystem entries we already created */
	rip_proc_cleanup();
	return -ENOMEM;
}

/**
 * Remove all proc entries for the RIP driver
 * \return 0
 */
int rip_proc_cleanup(void)
{
	struct list_head        *pos, *q;
	struct rip_proc_list    *tmp;

	list_for_each_safe(pos, q, &rip_proc_entries.list)
	{
		tmp = list_entry(pos, struct rip_proc_list, list);
		remove_proc_entry(tmp->name, rip_proc_dir);
		list_del(pos);
		kfree(tmp);
	}

	remove_proc_entry(RIP_DIR_NAME, NULL);

	return 0;
}
