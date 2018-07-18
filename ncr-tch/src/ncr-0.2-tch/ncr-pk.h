#ifndef NCR_PK_H
# define NCR_PK_H

#include <tomcrypt.h>

struct nlattr;
struct key_item_st;

#ifdef CONFIG_CRYPTO_USERSPACE_ASYMMETRIC
struct ncr_pk_ctx {
	const struct algo_properties_st *algorithm;	/* algorithm */

	const struct algo_properties_st *sign_hash;	/* for verification */

	const struct algo_properties_st *oaep_hash;
	int salt_len;		/* for RSA-PSS signatures */

	int type;		/* libtomcrypt type */
	int init;		/* non zero if initialized */

	struct key_item_st *key;
};

/* PK */
void ncr_pk_clear(struct key_item_st *key);
int ncr_pk_generate(const struct algo_properties_st *algo, struct nlattr *tb[],
		    struct key_item_st *private, struct key_item_st *public);
int ncr_pk_pack(const struct key_item_st *key, uint8_t * packed,
		uint32_t * packed_size);
int ncr_pk_unpack(struct key_item_st *key, const void *packed,
		  size_t packed_size);

/* encryption/decryption */
int ncr_pk_cipher_init(const struct algo_properties_st *algo,
		       struct ncr_pk_ctx *ctx, struct nlattr *tb[],
		       struct key_item_st *key,
		       const struct algo_properties_st *sign_hash);
void ncr_pk_cipher_deinit(struct ncr_pk_ctx *ctx);

int ncr_pk_cipher_encrypt(const struct ncr_pk_ctx *ctx,
			  const struct scatterlist *isg, unsigned int isg_cnt,
			  size_t isg_size, struct scatterlist *osg,
			  unsigned int osg_cnt, size_t * osg_size);

int ncr_pk_cipher_decrypt(const struct ncr_pk_ctx *ctx,
			  const struct scatterlist *isg, unsigned int isg_cnt,
			  size_t isg_size, struct scatterlist *osg,
			  unsigned int osg_cnt, size_t * osg_size);

int ncr_pk_cipher_sign(const struct ncr_pk_ctx *ctx, const void *hash,
		       size_t hash_size, void *sig, size_t * sig_size);

int ncr_pk_cipher_verify(const struct ncr_pk_ctx *ctx, const void *sig,
			 size_t sig_size, const void *hash, size_t hash_size);

int _ncr_tomerr(int err);

int ncr_pk_derive(struct key_item_st *newkey, struct key_item_st *oldkey,
		  struct nlattr *tb[]);

int ncr_pk_get_rsa_size(rsa_key * key);
int ncr_pk_get_dsa_size(dsa_key * key);

// Defines for checking on the begin and end of private key
#define PEM_PKCS1_PRIVKEY_BEGIN "-----BEGIN RSA PRIVATE KEY-----\n"
#define PEM_PKCS1_PRIVKEY_END "-----END RSA PRIVATE KEY-----\n"
#define PEM_PKCS8_PRIVKEY_BEGIN "-----BEGIN PRIVATE KEY-----\n"
#define PEM_PKCS8_PRIVKEY_END "-----END PRIVATE KEY-----\n"
#define PEM_PKCS8ENC_PRIVKEY_BEGIN "-----BEGIN ENCRYPTED PRIVATE KEY-----\n"
#define PEM_PKCS8ENC_PRIVKEY_END "-----END ENCRYPTED PRIVATE KEY-----\n"

typedef enum
{
	UNKNOWN,
	PKCS1,
	PKCS8,
	PKCS8ENC,
} pkey_format;

pkey_format ncr_pk_get_pemkey_info(char* key, unsigned int size, unsigned int *offset, unsigned int *length);

int ncr_pk_pem_decode_to_der(char *pem_data, unsigned int pem_len, char *out, unsigned long *out_len);

int ncr_pk_privkey_pkcs8_to_pkcs1(char *key, unsigned long *key_size);

#else /* !CONFIG_CRYPTO_USERSPACE_ASYMMETRIC */
struct ncr_pk_ctx {};
#define ncr_pk_clear(key) ((void)0)
#define ncr_pk_pack(key, packed, packed_size) (-EOPNOTSUPP)
#define ncr_pk_unpack(key, packed, packed_size) (-EOPNOTSUPP)
#define ncr_pk_cipher_init(algo, ctx, tb, key, sign_hash) (-EOPNOTSUPP)
#define ncr_pk_cipher_deinit(ctx) ((void)0)
#define ncr_pk_cipher_encrypt(ctx, i, icnt, isize, o, ocnt, osize) (-EOPNOTSUPP)
#define ncr_pk_cipher_decrypt(ctx, i, icnt, isize, o, ocnt, osize) (-EOPNOTSUPP)
#define ncr_pk_cipher_sign(ctx, hash, hash_size, sig, sig_size) (-EOPNOTSUPP)
#define ncr_pk_cipher_verify(ctx, sig, sig_size, hash, hash_size) (-EOPNOTSUPP)
#define ncr_pk_derive(newkey, oldkey, tb) (-EOPNOTSUPP)
#endif /* !CONFIG_CRYPTO_USERSPACE_ASYMMETRIC */

#endif
