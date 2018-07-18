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
** Programmer(s) : Joris Gorinsek (email : joris.gorinsek@technicolor.com)  **
**                                                                          **
******************************************************************************/


#include "rip2.h"
#include "ripdrv.h"

static int  rip2_data_end  = 0;
static int  rip2_idx_end   = 0;
static int  rip2_size      = 0;

#ifdef CONFIG_RIPDRV_CRYPTO_SUPPORT
  #include "crypto_api.h"
  #include "rip2_crypto.h"
#endif

#include "rip2_common.h"

#ifdef BUILDTYPE_uboot_bootloader
#include <linux/string.h>
#include <malloc.h>
#include <common.h>
#endif

#define FLASH_READ(from, len, retlen, buf)      rip2_flash_read((from), (len), ((size_t *)retlen), ((unsigned char *)buf))
#define FLASH_WRITE(to, len, retlen, buf)       rip2_flash_write((to), (len), ((size_t *)retlen), ((unsigned char *)buf))
#define FLASH_CLEAR(to, len, retlen)            rip2_flash_clear((to), (len), ((size_t *)retlen))

/*
 * Add a rip2 field: adds an index item and copies the data to the
 * right location.
 */
static int rip2_add_item(uint8_t        *ripPtr,
                         uint8_t        *data,
                         size_t	 	len,
                         T_RIP2_ID      id,
                         uint32_t       attrHi,
                         uint32_t       attrLo);

/*
 * Sets all but the last index items of a given ID to invalid
 */
static int rip2_make_single_idx_valid(uint8_t   *ripStart,
                                      T_RIP2_ID id);

/**
 * Iterates over all index items which match the flags passed as an argument.
 * In case a NULL iterator was passed, the iterator will be restarted from
 * the RIP start address.
 * In case a non-NULL iterator was passed, the iterator continues from the last
 * entry it returned.
 *
 * RETURNS: RIP2_ERR_INV_RIP if an invalid start address was passed,
 *          RIP2_ERR_NOELEM if no matching index found
 *          RIP2_SUCCESS otherwise
 */
int rip2_get_next(T_RIP2_ITEM     **from,
                  const uint32_t  flags,
                  T_RIP2_ITEM     *item)
{
    T_RIP2_ITEM buf;
    size_t ret = 0;            //not used, needed for linux/BL compatibility

    if (*from == NULL) {
        /* Reinitialize the iterator */
        /* Some strange logic because we have to count backwards from the
         * end of the RIPv2 sector */
        *from = (T_RIP2_ITEM *)(RIP2_START + RIP2_OFFSET +
                                (rip2_size - sizeof(T_RIP2_HDR))
                                - sizeof(T_RIP2_ITEM));
    }

    FLASH_READ((size_t)*from, sizeof(T_RIP2_ITEM), &ret, &buf);

    /* iterate over index items, check flags, return if match */
    while (BETOH16(buf.ID) != 0xFFFF) {
        if ((BETOH32(buf.attr[ATTR_HI]) & RIP2_ATTR_VALID) &&
            ((flags == RIP2_ATTR_ANY) ||
             (BETOH32(buf.attr[ATTR_HI]) & flags))) {
            (*from)--;  //make sure we start from the next one next time
                        //we have found our item
            memcpy(item, &buf, sizeof(buf));
            return RIP2_SUCCESS;
        }
        (*from)--;
        FLASH_READ((size_t)*from, sizeof(T_RIP2_ITEM), &ret, &buf);
    }

    return RIP2_ERR_NOELEM;
}

/*
 * Sets all but the last index items of a given ID to invalid.
 * Beware: callers of this function MUST hold the RIP2_BIG_LOCK, otherwise the
 * correct funtion of make_single_idx_valid is not guaranteed!
 *
 * RETURNS: RIP2_SUCCESS if successful, RIP2_ERR_NOELEM if the ID was not
 * found
 */
static int rip2_make_single_idx_valid(uint8_t   *ripStart,
                                      T_RIP2_ID id)
{
    T_RIP2_ITEM buf;
    size_t ret = 0;

    static T_RIP2_ITEM  *from;
    int                 found_last = 0;
    int			data_len;

    from = (T_RIP2_ITEM *)(ripStart + RIP2_OFFSET + rip2_idx_end);
    FLASH_READ((size_t)from, sizeof(T_RIP2_ITEM), &ret, &buf);

    /* iterate over index items, check flags, return if match */
    while (from < (T_RIP2_ITEM *)(ripStart + RIP2_OFFSET + rip2_size - sizeof(T_RIP2_HDR))) {
        if ((BETOH32(buf.attr[ATTR_HI]) & RIP2_ATTR_VALID) &&
            (BETOH16(buf.ID) == id)) {
            if (found_last) { /* we found already the most recent, set remaining to invalid */
                uint32_t toWrite = HTOBE32(BETOH32(buf.attr[ATTR_HI]) & ~RIP2_ATTR_VALID);
                FLASH_WRITE((size_t)&(from->attr[ATTR_HI]), sizeof(toWrite), &ret, &toWrite);

                /* clear data part */
                data_len = BETOH32(buf.length);
                FLASH_CLEAR((size_t)ripStart + BETOH32(buf.addr), data_len + data_len % 2 + CRC_SZ, &ret);
            }
            else { /* preserve the most recent */
                found_last = 1;
            }
        }
        from++;
        FLASH_READ((size_t)from, sizeof(T_RIP2_ITEM), &ret, &buf);
    }

    return found_last ? RIP2_SUCCESS : RIP2_ERR_NOELEM;
}

/*
 * Add a rip2 data item in empty RIP2.
 * Beware: callers of this function MUST hold the RIP2_BIG_LOCK, otherwise the
 * correctness of rip2_get_next is not guaranteed!
 *
 * RETURNS: RIP2_ERR_NOMEM if no item could be added
 *          RIP2_SUCCESS otherwise
 */
static int rip2_add_data(uint8_t        *ripStart,
                         unsigned long  len,
                         T_RIP2_ID      id,
                         uint32_t       attrHi,
                         uint32_t       attrLo,
                         uint8_t        *data)
{
    unsigned long totlen = len + CRC_SZ;
    uint32_t crc    = 0xffffffff;
    size_t ret = 0;

    /* First, check if there is enough free space */
    if (totlen > (rip2_idx_end - rip2_data_end)) {
        ERR("Error: not enough free space in rip to store data for ID %04x!\r\n", id);
        return RIP2_ERR_NOMEM;
    }

    DBG("Adding block of size %ld at offset %x\r\n", len, rip2_data_end);

    /* add a crc at the end */
    crc  = rip2_crc32(data, len);
    crc  = HTOBE32(crc);

    /* Copy the data */
    if ((size_t)data % 2 || len % 2) {
        /* address of data is not 16-bit aligned - required for the FLASH driver
           So perform a realignment of both address and length */
        uint8_t *buf = ALLOC(len + len % 2 + CRC_SZ);
        if (buf == NULL) {
            ERR("Unable to allocate buffers\r\n");
            return RIP2_ERR_NOMEM;
        }
        if (len % 2) {
            buf[len + CRC_SZ] = 0xFF; //padding byte comes after the CRC
        }

        memcpy(buf, data, len);
        memcpy(buf + len, &crc, CRC_SZ);
        FLASH_WRITE(((size_t)ripStart + RIP2_OFFSET + rip2_data_end), (len + CRC_SZ + len % 2), &ret, buf);
        FREE(buf);
        rip2_data_end += len + CRC_SZ + len % 2;
    }
    else {
        FLASH_WRITE(((size_t)ripStart + RIP2_OFFSET + rip2_data_end), len, &ret, data);
        rip2_data_end += len;
        FLASH_WRITE(((size_t)ripStart + RIP2_OFFSET + rip2_data_end), sizeof(crc), &ret, &crc);
        rip2_data_end += CRC_SZ;
    }

    return RIP2_SUCCESS;
}

/*
 * Returns the data pointer for the first index item with the given RIP2 ID
 * and the valid flag set.  Returns length if length!=NULL.
 * If raw != 0 then data is returned as they are present in rip2.
 *
 * RETURNS: RIP2_ERR_NOELEM if no matching index found
 *          RIP2_ERR_BADCRC if a CRC error was detected
 *          RIP2_SUCCESS otherwise
 */
int rip2_get_data_ex(uint8_t    *ripStart,
                     T_RIP2_ID  id,
                     uint8_t    *data,
                     uint32_t   *length,
                     int        raw)
{
    T_RIP2_ITEM item;
    uint32_t    addr;

    size_t ret = 0;
    uint32_t  rip2len, len;
    uint8_t   *buf = data;

    if (RIP2_SUCCESS != rip2_get_idx(id, &item)) {
        return RIP2_ERR_NOELEM;
    }
    LOCK();

    rip2len = BETOH32(item.length);
    addr    = BETOH32(item.addr);

    if (rip2len == 0xFFFFFFFF) {
        ERR("Error getting RIP2 data for ID %04x: bad length\r\n", id);
        UNLOCK();
        return RIP2_ERR_NOELEM;
    }

    if (~BETOH32(item.attr[ATTR_HI]) & RIP2_ATTR_CRYPTO || (data == NULL)) {
        /* Encrypted or signed entries are larger than the caller expects, so allocate a buffer big enough.
           This function can also be called to obtain the length, when data=0. */
        buf = ALLOC(rip2len);
        if (!buf) {
            UNLOCK();
            return RIP2_ERR_NOMEM;
        }
    }

    FLASH_READ(((size_t)ripStart + addr), rip2len, &ret, buf);

    if (!(BETOH32(item.attr[ATTR_HI]) & RIP2_ATTR_DONT_CHK_CRC)) {
        /* Check CRC */
        uint32_t  toverify = 0;
        uint32_t  crc;

        FLASH_READ(((size_t)ripStart + addr + rip2len), sizeof(toverify), &ret, &toverify);
        crc = rip2_crc32(buf, rip2len);

        if (crc != BETOH32(toverify)) {
            ERR("Error getting RIP2 data for ID %04x: bad CRC\r\n", id);
            UNLOCK();
            if (data != buf) {
                FREE(buf);
            }
            return RIP2_ERR_BADCRC;     //signal difference
        }
    }

    len = rip2len;
#ifdef CONFIG_RIPDRV_CRYPTO_SUPPORT
    if (~BETOH32(item.attr[ATTR_HI]) & RIP2_ATTR_CRYPTO) {
        DBG("RIP2 crypto active on ID %04x\r\n", id);
        if (rip2_crypto_process(buf, &len, ~(BETOH32(item.attr[ATTR_HI]) & RIP2_ATTR_CRYPTO), id) != RIP2_SUCCESS) {
            ERR("Error getting RIP2 data for ID %04x: bad crypto\r\n", id);
            UNLOCK();
            if (data != buf) {
                FREE(buf);
            }

            return RIP2_ERR_BADCRYPTO;     //signal problem
        }
    }
#endif

    /* We allocated a buffer because the caller did not necessarily supply a
       big enough buffer (or just 0).  Now the in-clear length is known, copy
       back. */
    if (data != buf) {
        if (data) {
            if (raw) {
                FLASH_READ(((size_t)ripStart + addr), rip2len, &ret, data);
            } else {
                memcpy(data, buf, len);
            }
        }
        FREE(buf);
    }

    if (length) {
        *length = (raw)? rip2len : len;
    }

    UNLOCK();
    return RIP2_SUCCESS;
}

/*
 * Add a rip2 index item.
 * Beware: callers of this function MUST hold the RIP2_BIG_LOCK, otherwise the
 * correctness of rip2_add_idx is not guaranteed!
 *
 * RETURNS: RIP2_ERR_NOMEM if no free space available
 *          RIP2_SUCCESS otherwise
 */
static int rip2_add_idx(uint8_t       *ripStart,
                        unsigned long len,
                        T_RIP2_ID     id,
                        uint32_t      attrHi,
                        uint32_t      attrLo)
{
    T_RIP2_ITEM item;
    size_t ret = 0;

    /* First, check if there is enough free space */
    if (sizeof(T_RIP2_ITEM) > (rip2_idx_end - rip2_data_end)) {
        ERR("Error: not enough free space in rip to store index for ID %04x!\r\n", id);
        return RIP2_ERR_NOMEM;
    }

    /* Fill in item specifics */
    item.ID            = HTOBE16(id);
    item.length        = HTOBE32(len);
    item.attr[ATTR_LO] = HTOBE32(attrLo);
    item.attr[ATTR_HI] = HTOBE32(attrHi);
    /* in the index table we use the offset from start of flash, this way we
     * can also add values that might be located outside of the RIP2 sector */
    item.addr = HTOBE32(rip2_data_end + RIP2_OFFSET);

    /* Add to the index */
    /* first, move the end pointer to make some room for this entry */
    rip2_idx_end -= sizeof(T_RIP2_ITEM);
    DBG("Adding index item at location %x\r\n", rip2_idx_end);
    DBG("ID: %04x\r\nlen: %ld\r\naddr: %x\r\nattrHi: 0x%08x\r\n",
        id, len, BETOH32(item.addr), attrHi);

    FLASH_WRITE(((size_t)ripStart + RIP2_OFFSET + (rip2_idx_end)), sizeof(T_RIP2_ITEM), &ret, &item);

    return RIP2_SUCCESS;
}

/*
 * Invalidate a rip2 index item.
 *
 * RETURNS: RIP2_ERR_NOELEM if no matching entry was found
 *          RIP2_SUCCESS otherwise
 */
static int rip2_invalidate_idx(uint8_t    *ripStart,
                               int        rip2_invalid_idx,
                               T_RIP2_ID  id)
{
    T_RIP2_ITEM item;
    size_t ret = 0;

    FLASH_READ((size_t)rip2_invalid_idx, sizeof(T_RIP2_ITEM), &ret, &item);
    if (BETOH16(item.ID) == id) {
        /* Get the item at that location and check if it's the one we need to
         * invalidate */
        DBG("Invalidating index item with ID 0x%04x at location %x\r\n", id, rip2_invalid_idx);
        //clear the valid bit on this item
        item.attr[ATTR_HI] = HTOBE32( BETOH32(item.attr[ATTR_HI]) & ~RIP2_ATTR_VALID);
        FLASH_WRITE(((size_t)ripStart + RIP2_OFFSET + (rip2_invalid_idx)), sizeof(T_RIP2_ITEM), &ret, &item);
        return RIP2_SUCCESS;
    }

    return RIP2_ERR_NOELEM;
}

/*
 * Add a rip2 field: adds an index item and copies the data to the
 * right location.
 * Beware: callers of this function MUST hold the RIP2_BIG_LOCK, otherwise the
 * correctness of rip2_add_item is not guaranteed!
 *
 * RETURNS: RIP2_ERR_NOMEM if no free space available
 *          RIP2_SUCCESS otherwise
 */
static int rip2_add_item(uint8_t        *ripStart,
                         uint8_t        *data,
                         size_t		len,
                         T_RIP2_ID      id,
                         uint32_t       attrHi,
                         uint32_t       attrLo)
{
    DBG("%s: entered for id 0x%x\n", __FUNCTION__, id);

    //for writeable data we should also take into account the CRC on each
    //piece of data!!!
    if (sizeof(T_RIP2_ITEM) + len + CRC_SZ > (rip2_idx_end - rip2_data_end)) {
        ERR("Error: not enough free space in rip to store ID %04x!\r\n", id);
        return RIP2_ERR_NOMEM;
    }

    // add an index entry for this item
    if (RIP2_SUCCESS != rip2_add_idx(ripStart, len, id, attrHi, attrLo)) {
        DBG("%s: rip2_add_idx failed\n", __FUNCTION__);
        return RIP2_ERR_NOMEM;
    }

    // add the data field
    if (RIP2_SUCCESS != rip2_add_data(ripStart, len, id, attrHi, attrLo, data)) {
        DBG("%s: rip2_add_data failed\n", __FUNCTION__);
        //clean up invalid index item
        DBG("Adding data item failed, need to clean up dangling index item\r\n");
        rip2_invalidate_idx(ripStart, rip2_idx_end + sizeof(T_RIP2_ITEM), id);
        return RIP2_ERR_NOMEM;
    }

    // if not 16bit aligned, move the data end pointer to make sure we are!
    if (rip2_data_end % 2) {
        DBG("Added padding byte at offset %x\r\n", rip2_data_end);
        rip2_data_end++;
    }

    return RIP2_SUCCESS;
}

/*
 * Verifies the CRC of all elements in the RIP container.
 *
 * RETURNS: RIP2_SUCCESS if CRC is valid
 *          RIP2_ERR_BADCRC if CRC is invalid
 */
int rip2_verify_crc(uint8_t *ripStart)
{
    uint32_t    crc = 0;
    T_RIP2_ITEM item, *it = NULL;
    uint8_t     *data = ALLOC(rip2_size);
    size_t      ret = 0;

    if (data == NULL) {
        return RIP2_ERR_INV_RIP;
    }

    LOCK();

    //iterate over all items have their own crc and check each one
    while (rip2_get_next(&it, RIP2_ATTR_CHK_CRC, &item) == RIP2_SUCCESS) {
        uint32_t  toverify = 0;
        uint32_t  addr     = BETOH32(item.addr);
        uint32_t  len      = BETOH32(item.length);

        if (len + CRC_SZ > rip2_size) {
            UNLOCK();
            FREE(data);
            return RIP2_ERR_BADCRC; // invalid length
        }

        FLASH_READ(((size_t)ripStart + addr), len + CRC_SZ, &ret, data);
        memcpy(&toverify, data + len, sizeof(toverify));

        crc = rip2_crc32(data, len);

        if (crc != BETOH32(toverify)) {
            UNLOCK();
            FREE(data);
            return RIP2_ERR_BADCRC; //signal difference
        }
    }

    UNLOCK();
    FREE(data);
    return RIP2_SUCCESS; //all ok
}

/*
 * Performs some checks to determine whether the RIP2 content is ok:
 * - A CRC check on all elements
 * - Verify required crypto settings
 *
 * RETURNS: RIP2_SUCCESS if all ok
 *          RIP2_ERR_BADCRC if CRC is invalid
 *          RIP2_ERR_BADCRYPTO if some crypto requirement is not met
 */
int rip2_is_valid(uint8_t *ripStart)
{
    if (rip2_verify_crc(ripStart) != RIP2_SUCCESS) {
        return RIP2_ERR_BADCRC;
    }
#if defined(BUILDTYPE_cfe_bootloader) && defined(CONFIG_RIPDRV_CRYPTO_SUPPORT)
    if (rip2_crypto_check(ripStart) != RIP2_SUCCESS) {
        return RIP2_ERR_BADCRYPTO;
    }
#endif
    return RIP2_SUCCESS;
}

/*
 * Returns the data pointer for the first index item with the given RIP2 ID
 * and the valid flag set
 *
 * RETURNS: RIP2_ERR_NOELEM if no matching index found
 *          RIP2_ERR_BADCRC if a CRC error was detected
 *          RIP2_SUCCESS otherwise
 */
int rip2_get_data(uint8_t   *ripStart,
                  T_RIP2_ID id,
                  uint8_t   *data)
{
    return rip2_get_data_ex(ripStart, id, data, NULL, 0);
}

/*
 * Locks the data corresponding to RIP2 ID id by disabling the
 * writable flag (if set).
 *
 * RETURNS: RIP2_ERR_NOELEM if no matching id found
 *          RIP2_SUCCESS otherwise
 */
int rip2_lock(T_RIP2_ID id)
{
    T_RIP2_ITEM *from = NULL;
    T_RIP2_ITEM buf;
    uint32_t    ret = RIP2_SUCCESS;

    /* Some strange logic because we have to count backwards from the end of
     * the RIPv2 sector */
    from = (T_RIP2_ITEM *)(RIP2_START + RIP2_OFFSET +
                           (rip2_size - sizeof(T_RIP2_HDR))
                           - sizeof(T_RIP2_ITEM));

    FLASH_READ((size_t)from, sizeof(T_RIP2_ITEM), &ret, &buf);

    /* iterate over all items, check ID: if match, check valid flag */
    while (BETOH16(buf.ID) != 0xFFFF) {
        if ((BETOH16(buf.ID) == id) &&
            (BETOH32(buf.attr[ATTR_HI]) & RIP2_ATTR_VALID)) {
            /* If writable, disable it */
            if ((BETOH32(buf.attr[ATTR_HI]) & RIP2_ATTR_WRITABLE)) {
                buf.attr[ATTR_HI] = HTOBE32( BETOH32(buf.attr[ATTR_HI]) & ~RIP2_ATTR_WRITABLE);
                FLASH_WRITE((size_t)(from), sizeof(T_RIP2_ITEM), &ret, &buf);
            }
            return (ret == 0) ? RIP2_SUCCESS : ret;
        }
        from--;
        FLASH_READ((size_t)from, sizeof(T_RIP2_ITEM), &ret, &buf);
    }

    return RIP2_ERR_NOELEM;
}

#ifdef CONFIG_RIPDRV_CRYPTO_SUPPORT

/*
 * Checks if data contains valid signature using EIK and sets flag.
 *
 * RETURNS: RIP2_ERR_NOELEM if no matching id found
 *          RIP2_ERR_PERM if the flags don't match or there are no write
 *          permissions
 *          RIP2_SUCCESS otherwise
 */
int rip2_set_signed(T_RIP2_ID id)
{
  T_RIP2_ITEM old_item;
  uint32_t rip2len, len;
  uint32_t attrHi;
  uint8_t *buf;
  int ret;

  if (rip2_get_idx(id, &old_item) != RIP2_SUCCESS) {
    return RIP2_ERR_NOELEM;
  }

#if !defined(CONFIG_RIPDRV_ANVIL)
  // Do not overwrite READ-ONLY
  if (!(BETOH32(old_item.attr[ATTR_HI]) & RIP2_ATTR_WRITABLE)) {
    DBG("%s: read only bit is set\n", __FUNCTION__);
    return RIP2_ERR_PERM;
  }
#endif

  // Fail if already crypted or signed
  if (~BETOH32(old_item.attr[ATTR_HI]) & RIP2_ATTR_CRYPTO) {
    DBG("%s: encryption or signature conflict\n", __FUNCTION__);
    return RIP2_ERR_PERM;
  }

  rip2len = BETOH32(old_item.length);
  buf = ALLOC(rip2len);
  if (buf == NULL) {
    return RIP2_ERR_NOMEM;
  }

  ret = rip2_get_data((uint8_t *)(RIP2_START), id, buf);
  if (ret != RIP2_SUCCESS) {
    goto out;
  }

  /* check if valid signature is present */
  len = rip2len;
  if (rip2_crypto_process(buf, &len, RIP2_ATTR_N_EIK_SIGN, id) != RIP2_SUCCESS) {
    DBG("%s: signature check failed, ret=%d (rip2len=%d, len=%d)\n", __func__, ret, rip2len, len);
    ret = RIP2_ERR_BADCRYPTO;
    goto out;
  }

  /* write to flash (args are in host order!!)*/
  attrHi = BETOH32(old_item.attr[ATTR_HI]) & ~RIP2_ATTR_N_EIK_SIGN;
  ret = rip2_drv_write(buf, rip2len, id, attrHi, BETOH32(old_item.attr[ATTR_LO]));
  if (ret != RIP2_SUCCESS) {
    DBG("%s: write failed, ret=%d (rip2len=%d, len=%d)\n", __func__, ret, rip2len, len);
    goto out;
  }

  DBG("%s: success (rip2len=%d, len=%d)\n", __func__, rip2len, len);
out:
  FREE(buf);
  return ret;
}

#if !defined(BUILDTYPE_cfe_bootloader) && !defined(CONFIG_RIPDRV_INTEGRITY_ONLY)


/*
 * Encrypts the data corresponding to RIP2 ID id using ECK.
 * First checks if item has RIP2_ATTR_N_EIK_SIGN flag set.
 *
 * RETURNS: RIP2_ERR_NOELEM if no matching id found
 *          RIP2_ERR_PERM if the flags don't match or there are no write
 *          permissions
 *          RIP2_SUCCESS otherwise
 */
int rip2_encrypt(T_RIP2_ID id)
{
  T_RIP2_ITEM old_item;
  uint32_t rip2len, encr_len;
  uint32_t attrHi;
  uint8_t *buf;
  int ret;

  if (rip2_get_idx(id, &old_item) != RIP2_SUCCESS) {
    return RIP2_ERR_NOELEM;
  }

#if !defined(CONFIG_RIPDRV_ANVIL)
  // Do not overwrite READ-ONLY
  if (!(BETOH32(old_item.attr[ATTR_HI]) & RIP2_ATTR_WRITABLE)) {
    DBG("%s: read only bit is set\n", __FUNCTION__);
    return RIP2_ERR_PERM;
  }
#endif

  // Fail if already crypted or not signed using EIK
  if (~(BETOH32(old_item.attr[ATTR_HI]) ^ RIP2_ATTR_N_EIK_SIGN) & RIP2_ATTR_CRYPTO) {
    DBG("%s: encryption or signature conflict\n", __FUNCTION__);
    return RIP2_ERR_PERM;
  }

  rip2len = BETOH32(old_item.length);
  buf = ALLOC(rip2len + IV_LENGTH + CONFIDKEYSIZE);   /* encryption needs extra bytes for IV and padding */
  if (buf == NULL) {
    return RIP2_ERR_NOMEM;
  }

  ret = rip2_get_data_ex((uint8_t *)(RIP2_START), id, buf, NULL, 1);
  if (ret != RIP2_SUCCESS) {
    goto out;
  }

  /* encrypt data */
  encr_len = rip2len;
  if (rip2_crypto_encrypt_with_ECK(buf, &encr_len) != RIP2_SUCCESS) {
    DBG("%s: encryption failed (rip2len=%d, encr_len=%d)\n", __func__, rip2len, encr_len);
    ret = RIP2_ERR_BADCRYPTO;
    goto out;
  }

  /* write to flash (args are in host order!!)*/
  attrHi = BETOH32(old_item.attr[ATTR_HI]) & ~RIP2_ATTR_N_ECK_ENCR;
  ret = rip2_drv_write(buf, encr_len, id, attrHi, BETOH32(old_item.attr[ATTR_LO]));
  if (ret != RIP2_SUCCESS) {
    DBG("%s: write failed, ret=%d (rip2len=%d, encr_len=%d)\n", __func__, ret, rip2len, encr_len);
    goto out;
  }

  DBG("%s: success (rip2len=%d, encr_len=%d)\n", __func__, rip2len, encr_len);
out:
  FREE(buf);
  return ret;
}

#endif /* BUILDTYPE_cfe_bootloader && CONFIG_RIPDRV_INTEGRITY_ONLY */

#endif /* CONFIG_RIPDRV_CRYPTO_SUPPORT */

/*
 * Updates a rip2 field; if it does not yet exist, this performs rip2_add_item
 *
 * RETURNS: RIP2_SUCCESS if the item was successfully written
 *          RIP2_ERR_PERM if the flags don't match or there are no write
 *          permissions
 *          RIP2_ERR_NOELEM if no matching element was found (should not
 *          happen)
 */
int rip2_drv_write(uint8_t       *data,
                   size_t	 len,
                   T_RIP2_ID     id,
                   uint32_t      attrHi,
                   uint32_t      attrLo)
{
    T_RIP2_ITEM old_item;
    int         ret = RIP2_ERR_NOELEM;

    if (RIP2_SUCCESS != rip2_get_idx(id, &old_item)) {
        LOCK();
        // item does not exist, so create (no check on flags)
        DBG("%s: item 0x%x does not exist yet, add it\n", __FUNCTION__, id);
        ret = rip2_add_item(RIP2_START, data, len, id, attrHi, attrLo);
        DBG("%s: rip2_add_item returned %d\n", __FUNCTION__, ret);
    }
    else {
        // item exists, so check old flags & update.
        // procedure: make current index item invalid & add a new index item

#if !defined(CONFIG_RIPDRV_ANVIL)
        // Do not overwrite READ-ONLY
        if (!(BETOH32(old_item.attr[ATTR_HI]) & RIP2_ATTR_WRITABLE)) {
            DBG("%s: read only bit is set\n", __FUNCTION__);
            return RIP2_ERR_PERM;
        }

        // Only allow clearing of the writable bit in the flags
        if ((BETOH32(old_item.attr[ATTR_LO]) != attrLo)
            || (((BETOH32(old_item.attr[ATTR_HI]) ^ attrHi) & ~RIP2_ATTR_WRITABLE))) {
            DBG("%s: attributes don't match\n", __FUNCTION__);
            return RIP2_ERR_PERM;
        }
#endif /* CONFIG_RIPDRV_ANVIL */

        LOCK();
        /* First add a new data item, then set the old item's valid flag to 0.
         * In case of a power failure between the add_item action and the
         * invalidation of the old item, the get_idx and get_data functions
         * will still return the old values.
         */
        ret = rip2_add_item(RIP2_START, data, len, id, attrHi | RIP2_ATTR_VALID, attrLo);
        if (ret == RIP2_SUCCESS) {
            DBG("%s: rip2_add_item successful\n", __FUNCTION__);
            ret = rip2_make_single_idx_valid(RIP2_START, id);
        } else {
            DBG("%s: rip2_add_item failed with ret %d\n", __FUNCTION__, ret);
        }

    }
    UNLOCK();
    return ret;
}

/*
 * Finds the first index item with the given RIP2 ID and the valid flag set.
 *
 * RETURNS: RIP2_ERR_NOELEM if no matching index found,
 *          RIP2_SUCCESS otherwise
 */
int rip2_get_idx(T_RIP2_ID    id,
                 T_RIP2_ITEM  *item)
{
    T_RIP2_ITEM *from = NULL;
    T_RIP2_ITEM buf;
    size_t ret = 0;

    DBG("%s: called for id 0x%x\n", __FUNCTION__, id);

    LOCK();

    /* Some strange logic because we have to count backwards from the end of
     * the RIPv2 sector */
    from = (T_RIP2_ITEM *)(RIP2_START + RIP2_OFFSET +
                           (rip2_size - sizeof(T_RIP2_HDR))
                           - sizeof(T_RIP2_ITEM));

    FLASH_READ((size_t)from, sizeof(T_RIP2_ITEM), &ret, &buf);

    /* iterate over all items, check ID: if match, check valid flag */
    while (BETOH16(buf.ID) != 0xFFFF) {
        if ((BETOH16(buf.ID) == id) &&
            (BETOH32(buf.attr[ATTR_HI]) & RIP2_ATTR_VALID)) {
            memcpy(item, &buf, sizeof(buf));
            UNLOCK();
            DBG("%s: found index item\n", __FUNCTION__);
            return RIP2_SUCCESS;
        }
        from--;
        FLASH_READ((size_t)from, sizeof(T_RIP2_ITEM), &ret, &buf);
    }
    DBG("%s: no index item for id 0x%x found\n", __FUNCTION__, id);

    UNLOCK();
    return RIP2_ERR_NOELEM;
}

int rip2_get_item(T_RIP2_ID id,
                  T_RIP2_ITEM *item)
{
	int rv = rip2_get_idx(id, item);
	if( rv == RIP2_SUCCESS ) {
		item->ID = BETOH16(item->ID);
		item->length = BETOH32(item->length);
		item->addr = BETOH32(item->addr);
		item->attr[0] = BETOH32(item->attr[0]);
		item->attr[1] = BETOH32(item->attr[1]);
	}
	return rv;
}

int rip2_drv_read (unsigned long *length, T_RIP2_ID id, void *data)
{
    uint32_t length_inrip;
    int ret;

    if (length == NULL) {
        ret = RIP2_ERR_NOMEM;
        goto out_err; // length should not be NULL
    }

    ret = rip2_get_data_ex((uint8_t *)(RIP2_START), id, 0, &length_inrip, 0);
    if (RIP2_SUCCESS != ret)
        goto out_err;

    if (data == NULL) {   // caller is asking for length only
        *length = length_inrip;
        return ret;
    }

    if((length_inrip > 0) && (*length >= length_inrip)){
        *length = length_inrip; // Update length parameter
    }
    else {
        ret = RIP2_ERR_NOMEM;
        goto out_err; // Can't return anything meaningful
    }

    ret = rip2_get_data((uint8_t *)(RIP2_START), id, data);
    if (RIP2_SUCCESS == ret)
        return ret;

out_err:
    /* We don't have a requested key, return keysize 0 */
    if (length != NULL) *length = 0;
    return ret;
}

/*
 * Create an empty rip2 with just a header field and bogus CRC
 * IN: ripPtr: a pointer to a memory buffer that will be used to store the
 *             rip2.
 *     verify: determines whether we are in verification of generation mode
 *     size: the size of the RIP2 sector
 *
 * RETURNS: RIP2_SUCCESS when successful, RIP2_ERR_INV_RIP or RIP2_ERR_NOMEM
 *          upon error.
 */
int rip2_init(uint8_t       *ripStart,
              int           verify,
              unsigned int  size)
{
    T_RIP2_HDR  hdr;
    int         dataFree   = 0;
    uint32_t    hdrStart  = RIP2_START + RIP2_OFFSET + (size - sizeof(T_RIP2_HDR));
    static int  printed    = 0;
    size_t 	ret	= 0;

    LOCK();

    rip2_size      = size;
    rip2_idx_end   = size - sizeof(T_RIP2_HDR);
    rip2_data_end  = 0;

    hdr.val          = 0xFFFFFFFF;
    hdr.str.version  = RIP_VERSION_2;

    /* Just verify if a valid RIP2 exists */
    if (verify) {
        uint8_t     toverify;
        uint8_t     *rip2buf = ALLOC(size);
        T_RIP2_ITEM *item;

        if (rip2buf == NULL) {
            ERR("Unable to allocate buffers\r\n");
            UNLOCK();
#ifdef CONFIG_RIPDRV_CRYPTO_SUPPORT
            rip2_crypto_init(0);
#endif
            return RIP2_ERR_NOMEM;
        }

        FLASH_READ((size_t)ripStart + RIP2_OFFSET, size, &ret, rip2buf);

        toverify = ((T_RIP2_HDR *)(rip2buf + (size - sizeof(T_RIP2_HDR))))->str.version;

        if (hdr.str.version != toverify) {
            if (!printed) {
                ERR("\r\nError: no valid RIP2 header found!\r\n");
                printed = 1;
            }
            UNLOCK();
            FREE(rip2buf);
#ifdef CONFIG_RIPDRV_CRYPTO_SUPPORT
            rip2_crypto_init(0);
#endif
            return RIP2_ERR_INV_RIP;
        }

        // find last element, so we can determine rip2_idx_end
        // rip2_data_end is first unused location
        // we do not use rip2_get_next, as we also want to process INVALID items
        item = (T_RIP2_ITEM *)(rip2buf + (rip2_size - sizeof(T_RIP2_HDR))
                               - sizeof(T_RIP2_ITEM));
        while (BETOH16(item->ID) != 0xFFFF) {
            
            // if data is within the RIP2 sector, make sure we don't overwrite it
            if ((BETOH32(item->addr) >= RIP2_OFFSET) && ((BETOH32(item->addr) - RIP2_OFFSET + BETOH32(item->length)) <= size)) {
                int dataEndsBefore = BETOH32(item->addr) - RIP2_OFFSET + BETOH32(item->length) + CRC_SZ;
                if (dataEndsBefore > rip2_data_end) {
                    rip2_data_end = dataEndsBefore;
                }
            }
            rip2_idx_end = (uint8_t *)item - rip2buf;
            item--;
        }

        // now check whether data area is free (i.e., 0xFF), otherwise shrink available area
        dataFree = rip2_idx_end - 1;
        while ((*(rip2buf + dataFree) == 0xFF) && dataFree > rip2_data_end) {
            dataFree--;
        }

        // if data ends at an odd address, add 1 padding byte
        rip2_data_end = (dataFree % 2) ? dataFree + 1 : dataFree;
        FREE(rip2buf);
    }
    else {
        FLASH_WRITE(hdrStart, sizeof(hdr), &ret, &hdr);
    }
    UNLOCK();
#ifdef CONFIG_RIPDRV_CRYPTO_SUPPORT
    rip2_crypto_init(ripStart);
#endif
    return RIP2_SUCCESS;
}
