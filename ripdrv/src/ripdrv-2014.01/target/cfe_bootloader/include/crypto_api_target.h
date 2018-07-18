
#include "bcm_sec.h"
#include "bcm_encr_if.h"

#define IV_LENGTH           CIPHER_IV_LEN

typedef struct capi_aes_ctx_st {
  unsigned char iv[IV_LENGTH];
  unsigned char key[CIPHER256_KEY_LEN];
  unsigned      key_bits; /* key length in bits */
  unsigned long rk[RK256_LEN];
} capi_aes_ctx;

typedef struct capi_rsa_ctx_st {
  unsigned char pubkey[SEC_S_MODULUS];
  unsigned mod_bits; /* modulus length in bits */
} capi_rsa_ctx;

#define CAPI_MAX_ENC_CTX_SIZE       (sizeof(capi_aes_ctx))

#define RSA_CTX_SIZE      (sizeof(capi_rsa_ctx))

