/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2014  -  Technicolor Delivery Technologies, SAS        **
** - All Rights Reserved                                                **
**                                                                      **
** Technicolor hereby informs you that certain portions                 **
** of this software module and/or Work are owned by Technicolor         **
** and/or its software providers.                                       **
**                                                                      **
** Distribution copying and modification of all such work are reserved  **
** to Technicolor and/or its affiliates, and are not permitted without  **
** express written authorization from Technicolor.                      **
**                                                                      **
** Technicolor is registered trademark and trade name of Technicolor,   **
** and shall not be used in any manner without express written          **
** authorization from Technicolor                                       **
**                                                                      **
*************************************************************************/

#include "rip2.h"
#include "rip_ids.h"
#include "otp_api.h"

#include "crypto_api.h"

#include "efu_privileged.h"
#include "rip2_common.h"

#if defined(BUILDTYPE_bootloader)
#define efu_printf    xprintf
#else
#define efu_printf printk
#endif

//#define ENG_FEAT_UNLOCKER_DEBUG
#ifdef ENG_FEAT_UNLOCKER_DEBUG
#define efu_dbg_printf    efu_printf
#else
#define efu_dbg_printf(...)
#endif /* ENG_FEAT_UNLOCKER_DEBUG */

#define SIGNATURE_SIZE 256
#define HASH_SIZE 32

#if defined(__BIG_ENDIAN)
#define BETOH64(x)    (x)
#elif defined(__LITTLE_ENDIAN)
// Swap all 8 bytes!
#define ntohll(x)	(\
    (((x)>>56) & 0x00000000000000ff) | \
    (((x)>>40) & 0x000000000000ff00) | \
    (((x)>>24) & 0x0000000000ff0000) | \
    (((x)>>8 ) & 0x00000000ff000000) | \
    (((x)<<8 ) & 0x000000ff00000000) | \
    (((x)<<24) & 0x0000ff0000000000) | \
    (((x)<<40) & 0x00ff000000000000) | \
    (((x)<<56) & 0xff00000000000000) \
  )
#define BETOH64(x)    (ntohll(x))
#else
#error Please define the correct endianness
#endif

/*
 * Prototypes local functions
 */

/* WARNING: When EFU_RET_SUCCESS is returned, *data_p_p needs to be freed after use!! */
static unsigned int rip2_malloc_and_read(unsigned short id, unsigned char** data_p_p, unsigned long * data_size_p);
static int isSignatureValid(unsigned char* pSignature, const char* serialNumber, EFU_CHIPID_TYPE chipId, unsigned char * data_a, unsigned long data_length);
static inline unsigned int fetchBitmask(unsigned char* unlockTag_a, unsigned long unlockTag_size, EFU_BITMASK_TYPE *bitmask_p);
static unsigned int verifyTag(unsigned char* unlockTag_a, unsigned long unlockTag_size);

/*
 * Global variables
 */

unsigned char* temporal_tag = NULL;
unsigned long temporal_tag_size = 0;

/* Technicolor OSIK for platforms that don't have this programmed in their eRIPv2 (DANT-U,DANT-O...) */
const unsigned char fallback_key[] = {
	0xd9, 0xa2, 0xae, 0xcb, 0xea, 0x13, 0xad, 0x7b, 0xff, 0x2b, 0x6b, 0x9c, 0xce, 0xd2, 0x20, 0xfd,
	0xc6, 0x4a, 0x6f, 0xa3, 0x44, 0x4b, 0x54, 0xce, 0x45, 0x39, 0xf7, 0xe8, 0x39, 0x98, 0xee, 0xf2,
	0x53, 0x7c, 0x9b, 0x00, 0x6c, 0x29, 0x0c, 0x02, 0x70, 0x74, 0x21, 0x1a, 0xf9, 0xd9, 0x57, 0x98,
	0x3b, 0x7e, 0x5a, 0x58, 0x0a, 0x60, 0x8e, 0xb1, 0xc1, 0xd9, 0xfb, 0xad, 0x62, 0x0f, 0x06, 0x8e,
	0x4d, 0xaa, 0x9e, 0x1b, 0xc4, 0x11, 0x01, 0xa6, 0x45, 0xc4, 0x43, 0xe8, 0x90, 0xd3, 0xc8, 0xb4,
	0xc6, 0x18, 0x99, 0xe5, 0x65, 0x71, 0x28, 0x60, 0x56, 0xba, 0x27, 0x42, 0xb8, 0xf5, 0x2a, 0x04,
	0x40, 0xcc, 0x95, 0xa9, 0x4e, 0xa6, 0xf0, 0x47, 0x55, 0x5b, 0x66, 0xc8, 0x33, 0xc2, 0x0e, 0xad,
	0x73, 0x22, 0xc6, 0xb4, 0x22, 0xe2, 0x89, 0xd6, 0x25, 0x19, 0x4c, 0x84, 0xa5, 0x42, 0xa0, 0x2a,
	0x41, 0xf7, 0x15, 0xd3, 0x26, 0xc8, 0x31, 0x63, 0xf1, 0x1d, 0x28, 0x05, 0x80, 0x86, 0xbb, 0xfd,
	0x6d, 0xf6, 0x57, 0xa0, 0xa8, 0x9b, 0x51, 0xfd, 0x4d, 0x20, 0x42, 0x5a, 0x15, 0x87, 0xc9, 0x96,
	0x5a, 0x1e, 0xc0, 0xad, 0x06, 0x2a, 0xcd, 0xa3, 0xe2, 0xe4, 0x7f, 0x97, 0x3c, 0x05, 0xe6, 0x7a,
	0x32, 0xd5, 0xec, 0x20, 0xe5, 0xe3, 0x3b, 0x54, 0x99, 0x4e, 0x15, 0x7e, 0xf5, 0xd1, 0xbd, 0xfc,
	0x59, 0x88, 0x72, 0x16, 0xae, 0x92, 0xfe, 0xf4, 0x44, 0x9c, 0x78, 0x1e, 0x32, 0xc0, 0x3b, 0x73,
	0x9a, 0xf8, 0x1c, 0x2d, 0xc5, 0x90, 0x60, 0x3f, 0xa9, 0xa5, 0x26, 0x11, 0x63, 0x1c, 0xa6, 0xf7,
	0xbe, 0x71, 0xba, 0x8c, 0xc9, 0xf7, 0xd8, 0xf6, 0xa0, 0x9b, 0x4e, 0x16, 0x2e, 0x06, 0x94, 0xbd,
	0x96, 0x4a, 0x44, 0x05, 0xc5, 0xdf, 0xf0, 0x44, 0x54, 0x93, 0x1a, 0xe5, 0x63, 0x75, 0x7c, 0xb5
};


/*
 * LOCAL FUNCTIONS: IMPLEMENTATION
 */

/* WARNING: When EFU_RET_SUCCESS is returned, *data_p_p needs to be freed after use!! */
static unsigned int rip2_malloc_and_read(unsigned short id, unsigned char** data_p_p, unsigned long * data_size_p) {
	unsigned int ret = EFU_RET_SUCCESS;
	unsigned char *data_p = NULL;
    unsigned long data_size = 0;
    // REF: Function entered

    if (rip2_drv_read_length(&data_size, id) != RIP2_SUCCESS) {
        ret = EFU_RET_RIPERR;
        goto rip2_malloc_and_read_afterFunctionEntered;
    }
    data_p = ALLOC(data_size);
    if (data_p == NULL) {
        ret = EFU_RET_NOMEM;
        goto rip2_malloc_and_read_afterFunctionEntered;
    }
    // REF: Datamem Allocated

    if (rip2_drv_read(&data_size, id, data_p) != RIP2_SUCCESS) {
        ret = EFU_RET_RIPERR;
        FREE(data_p);
    }

rip2_malloc_and_read_afterFunctionEntered:
    if (ret == EFU_RET_SUCCESS) {
        *data_p_p = data_p;
        *data_size_p = data_size;
    }
    else {
        *data_p_p = NULL;
        *data_size_p = 0;
    }
    return ret;
}

static int isSignatureValid(unsigned char* pSignature, const char* serialNumber, EFU_CHIPID_TYPE chipId,
                            unsigned char* data_a, unsigned long data_length) {
    unsigned char pubkey[256];
    unsigned long pubkey_length;
    unsigned char *data_to_sign_a = NULL;
    unsigned int data_to_sign_s;
    int isSignatureValid_b = 0;
    int rsa_ok;
    uint32_t chipIdBigEndian = HTOBE32(chipId);
    char *rsa_ctx = NULL;
    // REF: Function entered

#ifdef ENG_FEAT_UNLOCKER_DEBUG
    int pos = 0;
    efu_dbg_printf("SN:  %s\r\n", serialNumber);
    efu_dbg_printf("CID: 0x%08X\r\n", chipId);
    efu_dbg_printf("SIG:\r\n");
    while (pos < SIGNATURE_SIZE) {
        efu_dbg_printf("%02X.", pSignature[pos]);
        pos++;
        if (pos%15 == 0) {
            efu_dbg_printf("\r\n");
        }
    }
    efu_dbg_printf("\r\n");
#endif

    /* Fetch OSIK public key */
    pubkey_length = sizeof(pubkey);
    EFU_getOSIK(pubkey, pubkey_length);

    rsa_ctx = ALLOC(RSA_CTX_SIZE);
    if(rsa_ctx == NULL) {
        goto isSignatureValid_afterFunctionEntered;
    }

    if (rsa_create_crypto(pubkey, RSA_KEYFORMAT_2048_PUBLICMODULUSONLY, rsa_ctx, RSA_CTX_SIZE)) {
        goto isSignatureValid_afterRsaCTXAllocated;
    }

    /* Prepare data for signature verification */
    data_to_sign_s = strlen(serialNumber) + sizeof(chipId) + data_length;
    data_to_sign_a = ALLOC(data_to_sign_s);
    if (data_to_sign_a == NULL) {
        goto isSignatureValid_afterFunctionEntered;
    }
    // REF: DataToSign Allocated
    memcpy(data_to_sign_a, serialNumber, strlen(serialNumber));
    memcpy(data_to_sign_a + strlen(serialNumber), &chipIdBigEndian, sizeof(chipIdBigEndian));
    memcpy(data_to_sign_a + strlen(serialNumber) + sizeof(chipIdBigEndian), data_a, data_length);

    /* Compare hash in signature with calculated hash */
    rsa_ok = rsa_verifysig_pss_sha256(rsa_ctx, pSignature, SIGNATURE_SIZE, data_to_sign_a, data_to_sign_s);
    rsa_delete_crypto(rsa_ctx);
    if (rsa_ok) {
        isSignatureValid_b = true;
        efu_dbg_printf("== SIG VALID ==\r\n");
        goto isSignatureValid_afterDataToSignAllocated;
    }
    else {
        isSignatureValid_b = false;
        efu_dbg_printf("== ! SIG INV ! ==\r\n");
        goto isSignatureValid_afterDataToSignAllocated;
    }
isSignatureValid_afterRsaCTXAllocated:
    FREE(rsa_ctx);
isSignatureValid_afterDataToSignAllocated:
    FREE(data_to_sign_a);
isSignatureValid_afterFunctionEntered:
    return (isSignatureValid_b);
}

/* WARNING: No input parameter checking!! */
static inline unsigned int fetchBitmask(unsigned char* unlockTag_a, unsigned long unlockTag_size, EFU_BITMASK_TYPE *bitmask_p) {
    EFU_BITMASK_TYPE bitmask;

    // Copy bitmask in final struct, in the RIP everything is stored in Big Endian
    memcpy(&bitmask, unlockTag_a + SIGNATURE_SIZE, EFU_BITMASK_SIZE);
    bitmask = BETOH64(bitmask);
    *bitmask_p = bitmask;

    return EFU_RET_SUCCESS;
}

static unsigned int verifyTag(unsigned char* unlockTag_a, unsigned long unlockTag_size) {
    unsigned char *signature_a = NULL;
    char *serialNumber_a = NULL;
    EFU_BITMASK_TYPE bitmask, expected_hashes_bitmask;
    unsigned int ret = EFU_RET_SUCCESS;
    unsigned int expected_hashes_count;

    /* Here we are going to verify the signature, but also wether all required hashes are present! */

    // At least the signature and bitmask should be present!!
    unsigned int minimumSize = SIGNATURE_SIZE + EFU_BITMASK_SIZE;
    if (unlockTag_size < minimumSize) {
        ret = EFU_RET_BADTAG;
        goto verifyTag_EXIT;
    }

    signature_a = ALLOC(SIGNATURE_SIZE);
    if (signature_a == NULL) {
        ret = EFU_RET_NOMEM;
        goto verifyTag_EXIT;
    }

    // Copy signature in final struct
    memcpy(signature_a, unlockTag_a, SIGNATURE_SIZE);

    // Copy bitmask in final struct, in the RIP everything is stored in Big Endian
    memcpy(&bitmask, unlockTag_a + SIGNATURE_SIZE, EFU_BITMASK_SIZE);
    bitmask = BETOH64(bitmask);

    // Also, for each supported feature that requires a tag AND is activated in the bitmask,
    // we expect the hash to be present
    expected_hashes_bitmask = bitmask & EFU_SUPPORTEDFEATURES_BITMASK & EFU_REQUIREDHASHES_BITMASK;
    expected_hashes_count = 0;
    while (expected_hashes_bitmask > 0) {
        if (expected_hashes_bitmask & 0x0000000000000001) {
            expected_hashes_count++;
        }
        expected_hashes_bitmask = expected_hashes_bitmask>>1;
    }
    minimumSize += expected_hashes_count * HASH_SIZE;
    // Also all required hashes should be present!!
    if (unlockTag_size < minimumSize) {
        ret = EFU_RET_BADTAG;
        goto verifyTag_EXIT;
    }

    // Warning: serialNumber_a needs to be freed after use!!
    serialNumber_a = EFU_getSerialNumber();
    if (serialNumber_a == NULL) {
        ret = EFU_RET_NOMEM;
        goto verifyTag_EXIT;
    }

    if (isSignatureValid(signature_a, serialNumber_a, EFU_getChipid(), unlockTag_a + SIGNATURE_SIZE, unlockTag_size - SIGNATURE_SIZE)) {
        ret = EFU_RET_SUCCESS;
    }
    else {
        ret = EFU_RET_BADSIG;
        goto verifyTag_EXIT;
    }

verifyTag_EXIT:
    FREE(serialNumber_a);
    FREE(signature_a);
    return ret;
}

/*
 * ALL PUBLIC FUNCTIONS: IMPLEMENTATION
 */

unsigned int EFU_getBitmask(EFU_BITMASK_TYPE *bitmask_p, int tag_location) {
    unsigned int ret = EFU_RET_SUCCESS;
    EFU_BITMASK_TYPE bitmask;

    unsigned char *unlockTag_a = NULL;
    unsigned long unlockTag_size;

    // The temporal tag has precedence, except for specific request
    if (temporal_tag && tag_location != EFU_PERMANENT_TAG) {
        unlockTag_a = ALLOC(temporal_tag_size);
        if (unlockTag_a == NULL) {
            ret = EFU_RET_NOMEM;
            goto EFU_getBitmask_EXIT;
        }

        memcpy(unlockTag_a, temporal_tag, temporal_tag_size);
        unlockTag_size = temporal_tag_size;
    }

    // Fallback: read the bootloader unlock tag from RIP
    if (!unlockTag_a && tag_location != EFU_TEMPORAL_TAG) {
        ret = rip2_malloc_and_read(RIP_ID_UNLOCK_TAG, &unlockTag_a, &unlockTag_size);
        if (ret != EFU_RET_SUCCESS) {
            goto EFU_getBitmask_EXIT;
        }
    }

    if (!unlockTag_a) {
        ret = EFU_RET_NOTFOUND;
        goto EFU_getBitmask_EXIT;
    }

    // Verify the unlock tag
    if (verifyTag(unlockTag_a, unlockTag_size) != EFU_RET_SUCCESS) {
        ret = EFU_RET_BADTAG;
        goto EFU_getBitmask_EXIT;
    }

    // Fetch bitmask
    if (fetchBitmask(unlockTag_a, unlockTag_size, &bitmask) != EFU_RET_SUCCESS) {
        ret = EFU_RET_PARSEERROR;
        goto EFU_getBitmask_EXIT;
    }

    *bitmask_p = bitmask;
    efu_dbg_printf("EFU BITMASK: %08x%08x\r\n", (uint32_t)(bitmask >> 32), (uint32_t)bitmask);

EFU_getBitmask_EXIT:
    FREE(unlockTag_a);
    return ret;
}

EFU_CHIPID_TYPE EFU_getChipid() {
    return otp_chipid_read();
}

/* WARNING: returns string that needs to be freed after use!! */
char* EFU_getSerialNumber() {
    char *serialNumber_a = NULL;
    unsigned long serialNumber_size = 0;

    unsigned char *factoryId_a = NULL;
    unsigned long factoryId_size = 0;

    unsigned char *pbaSerialNumber_a = NULL;
    unsigned long pbaSerialNumber_size = 0;

    unsigned int ret = EFU_RET_SUCCESS;

    // REF: Function entered

    /* Get Factory ID */
    ret = rip2_malloc_and_read(RIP_ID_FACTORY_ID, &factoryId_a, &factoryId_size);
    if (ret != EFU_RET_SUCCESS) {
        goto EngFeatUnlocker_getSerialNumber_EXIT;
    }

    /* Get PBA Serial */
    ret = rip2_malloc_and_read(RIP_ID_BOARD_SERIAL_NBR, &pbaSerialNumber_a, &pbaSerialNumber_size);
    if (ret != EFU_RET_SUCCESS) {
        goto EngFeatUnlocker_getSerialNumber_EXIT;
    }

    /* Now concatenate both to form Serial Number */
    serialNumber_size = factoryId_size + pbaSerialNumber_size + 1 /* String termination */;
    serialNumber_a = ALLOC(serialNumber_size);
    if (serialNumber_a == NULL) {
        ret = EFU_RET_NOMEM;
        goto EngFeatUnlocker_getSerialNumber_EXIT;
    }
    // REF: Sermem Allocated

    memcpy(serialNumber_a, factoryId_a, factoryId_size);
    memcpy(&(serialNumber_a[factoryId_size]), pbaSerialNumber_a, pbaSerialNumber_size);
    serialNumber_a[serialNumber_size-1] = '\0';

//EngFeatUnlcoker_getSerialNumber_afterSerialmemAllocated:
    if (ret != EFU_RET_SUCCESS) {
        FREE(serialNumber_a);
        serialNumber_a = NULL;
    }
EngFeatUnlocker_getSerialNumber_EXIT:
    FREE(pbaSerialNumber_a);
    FREE(factoryId_a);
    return serialNumber_a;
}

unsigned int EFU_verifyStoredTag() {
    unsigned int ret = EFU_RET_SUCCESS;
    unsigned char *unlockTag_a = NULL;
    unsigned long unlockTag_size = 0;

    // Read the bootloader unlock tag from RIP
    ret = rip2_malloc_and_read(RIP_ID_UNLOCK_TAG, &unlockTag_a, &unlockTag_size);
    if (ret != EFU_RET_SUCCESS) {
        goto EFU_verifyStoredTag_EXIT;
    }

    // Verify the unlock tag
    if (verifyTag(unlockTag_a, unlockTag_size) != EFU_RET_SUCCESS) {
        ret = EFU_RET_BADTAG;
        goto EFU_verifyStoredTag_EXIT;
    }

EFU_verifyStoredTag_EXIT:
    FREE(unlockTag_a);
    return ret;
}

unsigned int EFU_storeTemporalTag(unsigned char * unlockTag_a, unsigned long unlockTag_size) {
    unsigned int ret = EFU_RET_SUCCESS;

    // Verify the unlock tag
    if (verifyTag(unlockTag_a, unlockTag_size) != EFU_RET_SUCCESS) {
        ret = EFU_RET_BADTAG;
        goto EFU_storeTemporalTag_EXIT;
    }

    FREE(temporal_tag);
    temporal_tag_size = 0;

    temporal_tag = ALLOC(unlockTag_size);
    if (temporal_tag == NULL) {
        ret = EFU_RET_NOMEM;
        goto EFU_storeTemporalTag_EXIT;
    }

    temporal_tag_size = unlockTag_size;
    memcpy(temporal_tag, unlockTag_a, unlockTag_size);

EFU_storeTemporalTag_EXIT:
    return ret;

}

void EFU_getOSIK(unsigned char * pubkey, unsigned long pubkey_length) {
    int ret;
    ret = rip2_drv_read(&pubkey_length, RIP_ID_OSIK, pubkey);

    if (ret != RIP2_SUCCESS)
    {
        memcpy(pubkey, fallback_key, pubkey_length);
    }
}
