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
** Programmer(s) : Gino Peeters (email : gino.peeters@technicolor.com)      **
**                                                                          **
******************************************************************************/

#ifndef CONFIG_RIPDRV_CRYPTO_SUPPORT
#error
#endif

#ifdef BUILDTYPE_cfe_bootloader
#include "lib_types.h"
#endif

#include "crypto_api.h"
#include "rip2.h"
#include "ripdrv.h"
#include "rip2_common.h"
#include "rip_ids.h"

#include "rip2_crypto.h"

#ifndef CONFIG_RIPDRV_INTEGRITY_ONLY
crypt_key    bek;
crypt_key    eck;
#endif
sign_key     eik;
sign_key     mcv;

static unsigned char  rip_bl_crypto_integrkey[INTEGRKEYSIZE];
#ifndef CONFIG_RIPDRV_INTEGRITY_ONLY
static unsigned char  rip_bl_crypto_confidkey[CONFIDKEYSIZE];
#endif
int rip2_crypto_import_eik(unsigned char *data, int length)
{
    if (length != 256) {
        ERR("Inv EIK");
        return RIP2_ERR_BADCRYPTO;
    }
    eik.length = length;
    eik.key    = rip_bl_crypto_integrkey;
    memcpy(rip_bl_crypto_integrkey, data, length);
    return RIP2_SUCCESS;
}

#ifndef CONFIG_RIPDRV_INTEGRITY_ONLY
int rip2_crypto_import_eck(unsigned char *data, int length)
{
    if (length != 16) {
        ERR("Inv ECK");
        return RIP2_ERR_BADCRYPTO;
    }
    eck.length = length;
    eck.key    = rip_bl_crypto_confidkey;
    eck.iv     = 0;
    memcpy(rip_bl_crypto_confidkey, data, length);
    return RIP2_SUCCESS;
}

static int rip_crypto_decrypt(unsigned char  *data,
                       unsigned char  *result,
                       uint32_t       *length,
                       crypt_key      *key)
{
    unsigned char crypto_ctx_exram[CAPI_MAX_ENC_CTX_SIZE];
    unsigned char *crypto_ctx = crypto_ctx_exram;
    unsigned char padding;

    unsigned char iv[IV_LENGTH];
    DBG("decr...");

    if (!key || !key->key) {
        return RIP2_ERR_BADCRYPTO;
    }

    if (key->iv) { /* General IV */
        memcpy(iv, key->iv, 16);
    }
    else { /* IV in data */
        memcpy(iv, data, 16);
        data += 16;
        *length -= 16;
    }
    aes_create_crypto(iv, key->key, key->length * 8, crypto_ctx, CAPI_MAX_ENC_CTX_SIZE);
    aes_decrypt_blocks(crypto_ctx, data, *length, result);
    aes_destroy_crypto(crypto_ctx, CAPI_MAX_ENC_CTX_SIZE);

    memset(iv, 0, 16);
    padding = result[*length - 1];
    if (padding > 16) {
        DBG("err ");
        return RIP2_ERR_BADCRYPTO;
    }
    *length -= padding;
    DBG("ok  ");
    return RIP2_SUCCESS;
}
#endif

static int rip_crypto_checksign(unsigned char  *data,
                         uint32_t       *length,
                         sign_key       *key,
                         T_RIP2_ID      id)
{
    int ok = 0;
    unsigned char * signature_inrip = NULL;
    unsigned char * data_inrip = NULL;
    int signeddata_length;
    unsigned char * signeddata = NULL;
    uint16_t d = BETOH16(id);
    char *rsa_ctx = NULL;

    DBG("auth...");
    if (!key || !key->key || *length <= key->length) {
        return RIP2_ERR_BADCRYPTO;
    }
    /* Format in RIP: <data><signature> with length including data + signature */

    /* Decode format in RIP */
    *length -= key->length;
    signature_inrip = data + *length;
    data_inrip = data;

    /* Signed data: <ID><data> */
    signeddata_length = *length + sizeof(T_RIP2_ID);
    signeddata = ALLOC(signeddata_length);

    if (!signeddata) {
        return RIP2_ERR_NOMEM;
    }

    memcpy(signeddata, (uint8_t* )&d, sizeof(T_RIP2_ID));
    memcpy(signeddata + sizeof(T_RIP2_ID), data_inrip, *length);

    if (key->key != SKIP_SIGNATURE_CHECK) { /* Sometimes we want to skip signature check (currently only MCV signed EIK) */
        rsa_ctx = ALLOC(RSA_CTX_SIZE);
        if(rsa_ctx == NULL) {
            FREE(signeddata);
            return RIP2_ERR_BADCRYPTO;
        }

        if (rsa_create_crypto(key->key, (key->length == 256) ? RSA_KEYFORMAT_2048_PUBLICMODULUSONLY : RSA_KEYFORMAT_1024_PUBLICMODULUSONLY, rsa_ctx, RSA_CTX_SIZE)) {
            FREE(signeddata);
            FREE(rsa_ctx);
            return RIP2_ERR_BADCRYPTO;
        }

        ok = rsa_verifysig_pss_sha256(rsa_ctx, signature_inrip, key->length, signeddata, signeddata_length);
        rsa_delete_crypto(rsa_ctx);
        FREE(rsa_ctx);
    }
    else {
        ok = 1;
    }

    DBG(ok ? "ok  " : " fail");
    FREE(signeddata);

    return ok ? RIP2_SUCCESS : RIP2_ERR_BADCRYPTO;
}

int rip2_crypto_process(uint8_t       *data,
                        uint32_t      *length,
                        uint32_t      crypto_attr,
                        T_RIP2_ID     id)
{
    unsigned char *buffer = 0;
#ifndef CONFIG_RIPDRV_INTEGRITY_ONLY
    crypt_key   *crypt_key   = 0;
#endif
    sign_key    *sign_key    = 0;

    if (crypto_attr & RIP2_ATTR_N_EIK_SIGN) {
        DBG(" EIK signed ");
        sign_key = &eik;
    }
    else if (crypto_attr & RIP2_ATTR_N_MCV_SIGN) {
        DBG(" MCV signed ");
        sign_key = &mcv;
    }
#ifndef CONFIG_RIPDRV_INTEGRITY_ONLY
    if (crypto_attr & RIP2_ATTR_N_ECK_ENCR) {
        DBG(" ECK encrypted ");
        crypt_key = &eck;
    }
    else if (crypto_attr & RIP2_ATTR_N_BEK_ENCR) {
        DBG(" BEK encrypted ");
        crypt_key = &bek;
    }

    /* Do not decrypt entries which are not signed */
    if (crypt_key && !sign_key) {
        return RIP2_ERR_BADCRYPTO;
    }
#endif

    buffer = ALLOC(*length);
    if (!buffer) {
        return RIP2_ERR_NOMEM;
    }

#ifndef CONFIG_RIPDRV_INTEGRITY_ONLY
    if (crypt_key) {
        if (rip_crypto_decrypt(data, buffer, length, crypt_key) != RIP2_SUCCESS) {
            FREE(buffer);
            return RIP2_ERR_BADCRYPTO;
        }
    }
    else
#endif
	{
        memcpy(buffer, data, *length);
    }

    if (sign_key) {
        if (rip_crypto_checksign(buffer, length, sign_key, id) != RIP2_SUCCESS) {
            memset(buffer, 0, *length);
            FREE(buffer);
            return RIP2_ERR_BADCRYPTO;
        }
    }

    DBG("done\r\n");

    memcpy(data, buffer, *length);

    FREE(buffer);
    return RIP2_SUCCESS;
}

#if !defined(BUILDTYPE_cfe_bootloader) && !defined(CONFIG_RIPDRV_INTEGRITY_ONLY)
static int rip_crypto_encrypt(uint8_t     *data,
                              uint8_t     *result,
                              uint32_t    *length,
                              crypt_key   *key)
{
    int iv_len = 0;
    unsigned char iv[IV_LENGTH];

    unsigned char crypto_ctx_exram[CAPI_MAX_ENC_CTX_SIZE];
    unsigned char *crypto_ctx = crypto_ctx_exram;
    unsigned char padding;

    DBG("encr...");

    if (!key || !key->key) {
        return RIP2_ERR_BADCRYPTO;
    }

    if (key->iv) { /* General IV */
        memcpy(iv, key->iv, IV_LENGTH);
    }
    else { /* prepend data with random IV */
        random_get_data(iv, IV_LENGTH);
        memcpy(result, iv, IV_LENGTH);
        iv_len = IV_LENGTH;
    }

    /* padding */
    padding = 16 - (*length % 16);
    memset(data + *length, padding, padding);

    if (aes_create_crypto(iv, key->key, key->length * 8, crypto_ctx, CAPI_MAX_ENC_CTX_SIZE)
        || aes_encrypt_blocks(crypto_ctx, data, *length + padding, result + iv_len)) {
      return RIP2_ERR_BADCRYPTO;
    }

    *length += iv_len + padding;
    DBG("ok\n");
    return RIP2_SUCCESS;
}

int rip2_crypto_encrypt_with_ECK (uint8_t *data, uint32_t *length)
{
    uint8_t *data_cp;
    int ret;

    data_cp = ALLOC(*length + CONFIDKEYSIZE);
    if (data_cp == NULL) {
        return RIP2_ERR_NOMEM;
    }

    memcpy(data_cp, data, *length);
    ret = rip_crypto_encrypt(data_cp, data, length, &eck);
    FREE(data_cp);

    return ret;
}

#endif /* BUILDTYPE_cfe_bootloader && CONFIG_RIPDRV_INTEGRITY_ONLY*/

