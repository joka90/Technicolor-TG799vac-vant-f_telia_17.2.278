
#include "rip_ids.h"

#include "rip2_crypto.h"

#ifndef CONFIG_RIPDRV_INTEGRITY_ONLY
#ifdef CONFIG_ARM
#include "mach/memory.h"

static inline void * rip2_crypto_phys_to_virt (void *p)
{
    return (void*)(phys_to_virt((unsigned long)p));
}
#else
#define rip2_crypto_phys_to_virt
#endif

static int rip2_crypto_get_passed_data(Rip2Secrets    *rip2secrets,
                                       T_RIP2_ID      id,
                                       unsigned char  *buf,
                                       uint32_t       *length)
{
    Rip2SecretsItem *nextFreeItem = rip2secrets->items;

    while (nextFreeItem->id != 0 && nextFreeItem->id != id) {
        ++nextFreeItem;
    }
    if (nextFreeItem->id != id) {
        return RIP2_ERR_NOELEM;
    }

    *length = nextFreeItem->length;
    memcpy(buf, rip2_crypto_phys_to_virt(nextFreeItem->data), *length);
    return RIP2_SUCCESS;
}
#endif

int rip2_crypto_init(uint8_t *ripStart)
{
#ifndef CONFIG_RIPDRV_INTEGRITY_ONLY
    extern unsigned char *r2secr;
    Rip2Secrets *r2secr_struct = (Rip2Secrets *)(rip2_crypto_phys_to_virt(r2secr));
#endif
    uint32_t  length;
    char      tmp[256 + 256 + 16]; /* Allocate for data + cryptopadding + signature */

    static int init_done = 0;

    if (init_done) {
        return RIP2_SUCCESS;
    }
    init_done = 1;

    DBG("\r\neRIPv2 crypto module init\r\n");

	mcv.key  = 0;
#ifndef CONFIG_RIPDRV_INTEGRITY_ONLY
    bek.key  = 0;
    eck.key  = 0;
#endif
    eik.key  = 0;

    mcv.length = 256;
    /* The EIK is MCV signed; we do not have this key in OS anymore, but bootloader had checked it */
    mcv.key = SKIP_SIGNATURE_CHECK;

    if (rip2_get_data_ex(ripStart, RIP_ID_EIK, tmp, &length, 0) == RIP2_SUCCESS) {
        rip2_crypto_import_eik(tmp, length);
    }

#if !defined(CONFIG_RIPDRV_ANVIL)
    /* Now disable anything which is MCV signed.
     * This is not done for Anvil builds as Anvil needs userspace access to
     * the values signed with MCV. The EIK in particular, so it can check the
     * signature of a signed item before storing it in the eRIP.
     */
    mcv.key = 0;
#endif


#ifndef CONFIG_RIPDRV_INTEGRITY_ONLY
    /* The ECK is passed from the bootloader */

    DBG("eRIPv2 secrets passed to Linux: %lx\r\n", (unsigned long)r2secr);

    if (r2secr == NULL || (r2secr_struct->magic != RIP2SECRETSMAGIC)) {
        return RIP2_ERR_BADCRYPTO;
    }

    if (rip2_crypto_get_passed_data(r2secr_struct, RIP_ID_ECK, tmp, &length)) {
        rip2_crypto_import_eck(tmp, length);
        INFO("eRIPv2 secrets passed correctly to Linux\n");
    }
#endif
    return RIP2_SUCCESS;
}


