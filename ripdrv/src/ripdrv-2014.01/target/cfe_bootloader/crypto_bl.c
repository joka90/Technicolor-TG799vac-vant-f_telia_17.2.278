
#include "bcm_common.h"
#include "util.h"

#include "crypto_api.h"
#include "rip2.h"
#include "ripdrv.h"
#include "rip2_common.h"
#include "rip_ids.h"
#include "efu_privileged.h"

#include "rip2_crypto.h"

typedef enum{
    RIP2_REQ_REQUIRED,
    RIP2_REQ_OPTIONAL,
    RIP2_REQ_REQUIRED_UNLESS_EFU,

} T_RIP2_REQ;

static int rip2_crypto_check_item(T_RIP2_ID id,
                                  uint32_t  crypt_attr,
                                  T_RIP2_REQ requirements)
{
    T_RIP2_ITEM item;

    if (rip2_get_idx(id, &item) != RIP2_SUCCESS) {
        switch (requirements) {
        case RIP2_REQ_REQUIRED_UNLESS_EFU:
            if (EFU_verifyStoredTag() == EFU_RET_SUCCESS) {
                return RIP2_SUCCESS;
            }
            else {
                return RIP2_ERR_NOELEM;
            }
        case RIP2_REQ_OPTIONAL:
            return RIP2_SUCCESS;
        default:
            return RIP2_ERR_NOELEM;
        }
    }
    if ((~BETOH32(item.attr[ATTR_HI]) & RIP2_ATTR_CRYPTO) != crypt_attr) {
        return RIP2_ERR_BADCRYPTO;
    }

    return RIP2_SUCCESS;
}

int rip2_crypto_check(uint8_t *ripStart)
{
    T_RIP2_ITEM item, *it = NULL;

    /* Check whether signature of all EIK signed items are OK */
    while(rip2_get_next(&it, RIP2_ATTR_ANY, &item) == RIP2_SUCCESS) {
        if (BETOH32(item.attr[ATTR_HI]) & RIP2_ATTR_N_EIK_SIGN) {
            continue;
        }
        if (rip2_get_data(ripStart, BETOH16(item.ID), NULL) != RIP2_SUCCESS) {
            return RIP2_ERR_BADCRYPTO;
        }
    }

    if (rip2_crypto_check_item(RIP_ID_EIK, RIP2_ATTR_N_MCV_SIGN, RIP2_REQ_REQUIRED) != RIP2_SUCCESS) {
        return RIP2_ERR_BADCRYPTO;
    }

    if (rip2_crypto_check_item(RIP_ID_ECK, RIP2_ATTR_N_MCV_SIGN | RIP2_ATTR_N_BEK_ENCR, RIP2_REQ_REQUIRED) != RIP2_SUCCESS) {
        return RIP2_ERR_BADCRYPTO;
    }

    if (rip2_crypto_check_item(RIP_ID_OSCK, RIP2_ATTR_N_EIK_SIGN | RIP2_ATTR_N_ECK_ENCR, RIP2_REQ_REQUIRED) != RIP2_SUCCESS) {
        return RIP2_ERR_BADCRYPTO;
    }

    if (rip2_crypto_check_item(RIP_ID_RANDOM_KEY_A, RIP2_ATTR_N_EIK_SIGN | RIP2_ATTR_N_ECK_ENCR, RIP2_REQ_REQUIRED) != RIP2_SUCCESS) {
        return RIP2_ERR_BADCRYPTO;
    }

    if (rip2_crypto_check_item(RIP_ID_RANDOM_KEY_B, RIP2_ATTR_N_EIK_SIGN | RIP2_ATTR_N_ECK_ENCR, RIP2_REQ_REQUIRED) != RIP2_SUCCESS) {
        return RIP2_ERR_BADCRYPTO;
    }

    if (rip2_crypto_check_item(RIP_ID_OSIK, RIP2_ATTR_N_EIK_SIGN, RIP2_REQ_REQUIRED) != RIP2_SUCCESS) {
        return RIP2_ERR_BADCRYPTO;
    }

    if (rip2_crypto_check_item(RIP_ID_CLIENT_CERTIFICATE, RIP2_ATTR_N_EIK_SIGN | RIP2_ATTR_N_ECK_ENCR, RIP2_REQ_REQUIRED_UNLESS_EFU) != RIP2_SUCCESS) {
        return RIP2_ERR_BADCRYPTO;
    }

    if (rip2_crypto_check_item(RIP_ID_OLYMPUS_IK, RIP2_ATTR_N_EIK_SIGN, RIP2_REQ_OPTIONAL) != RIP2_SUCCESS) {
        return RIP2_ERR_BADCRYPTO;
    }

    if (rip2_crypto_check_item(RIP_ID_OLYMPUS_CK, RIP2_ATTR_N_EIK_SIGN | RIP2_ATTR_N_ECK_ENCR, RIP2_REQ_OPTIONAL) != RIP2_SUCCESS) {
        return RIP2_ERR_BADCRYPTO;
    }

    return RIP2_SUCCESS;
}

int rip2_crypto_init(uint8_t *ripStart)
{
    static int init_done = 0;

    if (init_done) {
        return RIP2_SUCCESS;
    }
    DBG("\r\neRIPv2 crypto module init\r\n");

    bek.key  = 0;
    mcv.key  = 0;
    eck.key  = 0;
    eik.key  = 0;

    Booter1Args *bootArgs = btrm_get_bootargs();
    if (bootArgs != NULL) {
      bek.length = 16;
      bek.key    = bootArgs->encrArgs.bek;
      bek.iv     = 0;

      mcv.length = 256;
      mcv.key    = bootArgs->authArgs.manu;
    }

    unsigned char tmp[256 + 256 + 16]; /* Allocate for data + cryptopadding + signature */
    uint32_t length = 0;
    int allok = 1;

    /* Retrieve EIK, which should be a 2048 bit RSA public key modulus */
    if (rip2_get_data_ex(ripStart, RIP_ID_EIK, tmp, &length, 0) == RIP2_SUCCESS) {
        rip2_crypto_import_eik(tmp, length);
    }
    else {
        allok = 0;
    }

    /* Retrieve ECK, which should be a 128 bit AES key */
    if (rip2_get_data_ex(ripStart, RIP_ID_ECK, tmp, &length, 0) == RIP2_SUCCESS) {
        rip2_crypto_import_eck(tmp, length);
    }
    else {
        allok = 0;
    }

    /* All secure boot secrets are no longer needed.  In case of the production scenario, we need the keys */
#if 0
    if (allok || !check_MBH_Enabled()) {
        secureboot_gopublic(0);
    }
    else {
        init_done = 0;
    }
#endif
    init_done = allok;

    return RIP2_SUCCESS;
}

unsigned char *rip2_crypto_pass_linux(void)
{
    unsigned char *buf_ptr;

    Rip2Secrets *rip2secrets = tch_rip2_crypto_base();

    if (rip2secrets != NULL) {
      rip2secrets->magic   = RIP2SECRETSMAGIC;
      rip2secrets->version = 1;

      Rip2SecretsItem *nextFreeItem = rip2secrets->items;
      /* Construct [header][data1][data2]...: a bit overkill for passing just 1 item,
         but other things might be added later. */

      /* Add one or more items to be passed */
      nextFreeItem->id     = RIP_ID_ECK;
      nextFreeItem->length = eck.length; /* key length*/
      nextFreeItem->data   = eck.key;
      nextFreeItem++;

      /* Last entry */
      nextFreeItem->id = 0;

      /* Now copy data to follow the header, and adapt .data pointers */
      buf_ptr          = (unsigned char *)nextFreeItem + sizeof(Rip2SecretsItem);

      nextFreeItem = rip2secrets->items;
      while (nextFreeItem->id != 0) {
          memcpy(buf_ptr, nextFreeItem->data, nextFreeItem->length);
          nextFreeItem->data = buf_ptr;
          buf_ptr           += nextFreeItem->length;
          ++nextFreeItem;
      }
    }

    return (unsigned char *)rip2secrets;
}

