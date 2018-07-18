
#include "lib_types.h"
#include "lib_string.h"

#include "crypto_api.h"

int aes_create_crypto(unsigned char *iv, unsigned char *key, int key_len_bits, void *crypto_ctx, int size_of_ctx)
{
  capi_aes_ctx *ctx = (capi_aes_ctx *)crypto_ctx;

  if ((key_len_bits != 256 && key_len_bits != 128)
      || size_of_ctx < sizeof(*ctx))
    return -1;

  memcpy(ctx->iv, iv, IV_LENGTH);
  memcpy(ctx->key, key, key_len_bits / 8);
  ctx->key_bits = key_len_bits;
  return 0;
}

int aes_decrypt_blocks(void *crypto_ctx, unsigned char *idata, unsigned int numBytes, unsigned char *odata)
{
  capi_aes_ctx *ctx = (capi_aes_ctx *)crypto_ctx;
  unsigned long rk[RK256_LEN];

  switch (ctx->key_bits) {
      case 256:
        aes256InitDecrypt(rk, ctx->key);
        decryptWithEkCbc256(odata, idata, rk, numBytes, ctx->iv);
        break;
      case 128:
        decryptWithEk(odata, idata, ctx->key, numBytes, ctx->iv);
        break;
      default:
        return -1;
  }
  return 0;
}

int aes_destroy_crypto(void *crypto_ctx, int size_of_ctx)
{
  memset(crypto_ctx, 0, size_of_ctx);
  return 0;
}

int rsa_create_crypto(unsigned char *key,
                      int           keyformat,
                      void          *crypto_ctx,
                      int           size_of_ctx)
{
  capi_rsa_ctx *ctx = (capi_rsa_ctx *)crypto_ctx;

  if (size_of_ctx < sizeof(*ctx))
    return -1;

  switch (keyformat) {
      case RSA_KEYFORMAT_2048_PUBLICMODULUSONLY:
        ctx->mod_bits = 2048;
        break;
      case RSA_KEYFORMAT_1024_PUBLICMODULUSONLY:
        ctx->mod_bits = 1024;
        break;
      default:
        return -1;
  }

  memcpy(ctx->pubkey, key, ctx->mod_bits/8);
  return 0;
}

int rsa_delete_crypto(void *crypto_ctx)
{
  capi_rsa_ctx *ctx = (capi_rsa_ctx *)crypto_ctx;

  memset(ctx, 0, sizeof(*ctx));

  return 0;
}

int rsa_verifysig_pss_sha256(void           *crypto_ctx,
                             unsigned char  *sig,
                             int            siglen,
                             unsigned char  *data,
                             int            datalen)
{
  capi_rsa_ctx *ctx = (capi_rsa_ctx *)crypto_ctx;
  /* signature needs to be 4-bytes aligned */
  unsigned char sig_buf[SEC_S_MODULUS];

  if (siglen > sizeof(sig_buf))
    return 0;
  memcpy(sig_buf, sig, siglen);

  SecStatus rv_sec = Sec_verify((uint32_t *)sig_buf, siglen, (uint32_t *)ctx->pubkey, ctx->mod_bits/8, data, datalen);

  return (rv_sec == SEC_S_SUCCESS)? 1 : 0;
}

