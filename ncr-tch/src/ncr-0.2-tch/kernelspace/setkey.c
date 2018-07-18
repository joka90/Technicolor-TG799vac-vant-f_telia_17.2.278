/*
 * Driver for /dev/ncr device (aka NCR)
 *
 * Copyright (c) 2014 Marc Rivière <marc.riviere AT gmail DOT com>
 *
 * This file is part of linux cryptodev.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include <linux/module.h>
#include "../ncr-int.h"
#include "../cryptodev_int.h"
#include "../version.h"
#include "setkey.h"

/* ====== Provisioning ====== */
static void __init display_buf(void *buf, int buf_len)
{
	int i;

	for (i = 0; i < buf_len; i++)
		printk("%x", ((unsigned char *)buf)[i]);
	printk(".\n");
}


int import_key(const unsigned char *key_data,
                      const unsigned int key_size, const ncr_key_type_t key_type,
                      const unsigned int key_flags, const unsigned int key_algo,
                      char *key_id, unsigned char *key_descriptor, int *key_descriptor_length)
{
  void *ncr;
  struct key_item_st *item_import;
  ncr_key_t key_desc;
  unsigned int key_id_len;
  int rc = 0;

  ncr = ncr_get_lists();

  /* Get a descriptor on a key item */
  key_desc = ncr_key_init(ncr);
  if (key_desc < 0) {
    return key_desc;
  }

  /* Allocate memory for a locale key item
   * The NCR module will copy our structure and we have to kfree it
   * before returning to the calling function  */
  item_import = kzalloc(sizeof(struct key_item_st), GFP_KERNEL);
  if (!item_import) {
    err();
    ncr_key_deinit(ncr, key_desc);
    return -ENOMEM;
  }

  /* Fill in the key item */
  item_import->type = key_type;
  item_import->flags = key_flags;
  if(key_algo == NCR_ALG_RSA)
    item_import->algorithm = _ncr_algo_to_properties(NCR_ALG_RSA);
  else if(key_algo == NCR_ALG_AES_ECB)
    item_import->algorithm = _ncr_algo_to_properties(NCR_ALG_AES_ECB);
  else
  {
    err();
    ncr_key_deinit(ncr, key_desc);
    kfree(item_import);
    return -1;
  }

  key_id_len = strlen(key_id);
  if (key_id_len >= MAX_KEY_ID_SIZE)
  {
    key_id_len = MAX_KEY_ID_SIZE;
    key_id[key_id_len - 1] = '\0';
  }
  memcpy(item_import->key_id, key_id, key_id_len);
  item_import->key_id_size = key_id_len;
  item_import->desc = key_desc;

  /* Import key in NCR */
  rc = ncr_key_import_from_kernel(ncr, item_import, key_data, key_size);
  if (rc < 0) {
    err();
    ncr_key_deinit(ncr, key_desc);
  }

  kfree(item_import);
  //TODO right now there is no risk of out of the array boundary, better to do a size check
  sprintf(key_descriptor, "%d", key_desc);
  (*key_descriptor_length) = strlen(key_descriptor) + 1;
  return rc;

}
EXPORT_SYMBOL(import_key);
