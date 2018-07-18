/*
 * Driver for /dev/ncr device (aka NCR)
 *
 * Copyright (c) 2010 Nikos Mavrogiannopoulos <nmav@gnutls.org>
 * Portions Copyright (c) 2010 Michael Weiser
 * Portions Copyright (c) 2010 Phil Sutter
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

#include <linux/version.h>
#include <linux/crypto.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/ioctl.h>
#include <linux/random.h>
#include <linux/scatterlist.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <crypto/algapi.h>
#include <crypto/hash.h>
#include "cryptodev_int.h"

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0))
#define reinit_completion(x) INIT_COMPLETION(*(x))
#endif

struct cryptodev_result {
	struct completion completion;
	int err;
};

static void cryptodev_complete(struct crypto_async_request *req, int err)
{
	struct cryptodev_result *res = req->data;

	if (err == -EINPROGRESS)
		return;

	res->err = err;
	complete(&res->completion);
}

int cryptodev_cipher_init(struct cipher_data *out, const char *alg_name,
			  uint8_t * keyp, size_t keylen)
{

	struct ablkcipher_alg *alg;
	int ret;

	memset(out, 0, sizeof(*out));

	out->async.s = crypto_alloc_ablkcipher(alg_name, 0, 0);
	if (unlikely(IS_ERR(out->async.s))) {
		dprintk(1, KERN_DEBUG, "%s: Failed to load cipher %s\n",
			__func__, alg_name);
		return -EINVAL;
	}

	alg = crypto_ablkcipher_alg(out->async.s);

	if (alg != NULL) {
		/* Was correct key length supplied? */
		if (alg->max_keysize > 0
		    && unlikely((keylen < alg->min_keysize)
				|| (keylen > alg->max_keysize))) {
			dprintk(1, KERN_DEBUG,
				"Wrong keylen '%zu' for algorithm '%s'. Use %u to %u.\n",
				keylen, alg_name, alg->min_keysize,
				alg->max_keysize);
			ret = -EINVAL;
			goto error;
		}
	}

	ret = crypto_ablkcipher_setkey(out->async.s, keyp, keylen);
	if (unlikely(ret)) {
		dprintk(1, KERN_DEBUG, "Setting key failed for %s-%zu.\n",
			alg_name, keylen * 8);
		ret = -EINVAL;
		goto error;
	}

	out->blocksize = crypto_ablkcipher_blocksize(out->async.s);
	out->ivsize = crypto_ablkcipher_ivsize(out->async.s);

	out->async.result = kmalloc(sizeof(*out->async.result), GFP_KERNEL);
	if (unlikely(!out->async.result)) {
		ret = -ENOMEM;
		goto error;
	}

	memset(out->async.result, 0, sizeof(*out->async.result));
	init_completion(&out->async.result->completion);

	out->async.request = ablkcipher_request_alloc(out->async.s, GFP_KERNEL);
	if (unlikely(!out->async.request)) {
		dprintk(1, KERN_ERR, "error allocating async crypto request\n");
		ret = -ENOMEM;
		goto error;
	}

	ablkcipher_request_set_callback(out->async.request,
					CRYPTO_TFM_REQ_MAY_BACKLOG,
					cryptodev_complete, out->async.result);

	out->init = 1;
	return 0;
error:
	if (out->async.request)
		ablkcipher_request_free(out->async.request);
	kfree(out->async.result);
	if (out->async.s)
		crypto_free_ablkcipher(out->async.s);

	return ret;
}

void cryptodev_cipher_deinit(struct cipher_data *cdata)
{
	if (cdata->init) {
		if (cdata->async.request)
			ablkcipher_request_free(cdata->async.request);
		kfree(cdata->async.result);
		if (cdata->async.s)
			crypto_free_ablkcipher(cdata->async.s);

		cdata->init = 0;
	}
}

void cryptodev_cipher_set_iv(struct cipher_data *cdata, void *iv,
			     size_t iv_size)
{
	memcpy(cdata->async.iv, iv, min(iv_size, sizeof(cdata->async.iv)));
}

static inline int waitfor(struct cryptodev_result *cr, ssize_t ret)
{
	switch (ret) {
	case 0:
		break;
	case -EINPROGRESS:
	case -EBUSY:
		wait_for_completion(&cr->completion);
		/* At this point we known for sure the request has finished,
		 * because wait_for_completion above was not interruptible.
		 * This is important because otherwise hardware or driver
		 * might try to access memory which will be freed or reused for
		 * another request. */

		if (unlikely(cr->err)) {
			dprintk(0, KERN_ERR, "error from async request: %d \n",
				cr->err);
			return cr->err;
		}

		break;
	default:
		return ret;
	}

	return 0;
}

int _cryptodev_cipher_encrypt(struct cipher_data *cdata, const void *plaintext,
			      size_t plaintext_size, void *ciphertext,
			      size_t ciphertext_size)
{
	struct scatterlist sg, sg2;

	sg_init_one(&sg, plaintext, plaintext_size);
	sg_init_one(&sg2, ciphertext, ciphertext_size);

	return cryptodev_cipher_encrypt(cdata, &sg, &sg2, plaintext_size);
}

int _cryptodev_cipher_decrypt(struct cipher_data *cdata, const void *ciphertext,
			      size_t ciphertext_size, void *plaintext,
			      size_t plaintext_size)
{
	struct scatterlist sg, sg2;

	sg_init_one(&sg, ciphertext, ciphertext_size);
	sg_init_one(&sg2, plaintext, plaintext_size);

	return cryptodev_cipher_decrypt(cdata, &sg, &sg2, ciphertext_size);
}

ssize_t cryptodev_cipher_encrypt(struct cipher_data * cdata,
				 const struct scatterlist * sg1,
				 struct scatterlist * sg2, size_t len)
{
	int ret;

	reinit_completion(&cdata->async.result->completion);
	ablkcipher_request_set_crypt(cdata->async.request,
				     (struct scatterlist *)sg1, sg2, len,
				     cdata->async.iv);
	ret = crypto_ablkcipher_encrypt(cdata->async.request);

	return waitfor(cdata->async.result, ret);
}

ssize_t cryptodev_cipher_decrypt(struct cipher_data * cdata,
				 const struct scatterlist * sg1,
				 struct scatterlist * sg2, size_t len)
{
	int ret;

	reinit_completion(&cdata->async.result->completion);
	ablkcipher_request_set_crypt(cdata->async.request,
				     (struct scatterlist *)sg1, sg2, len,
				     cdata->async.iv);
	ret = crypto_ablkcipher_decrypt(cdata->async.request);

	return waitfor(cdata->async.result, ret);
}

/* Hash functions */

int cryptodev_hash_init(struct hash_data *hdata, const char *alg_name,
			const void *mackey, size_t mackeylen)
{
	int ret;

	hdata->async.s = crypto_alloc_ahash(alg_name, 0, 0);
	if (unlikely(IS_ERR(hdata->async.s))) {
		dprintk(1, KERN_DEBUG, "%s: Failed to load transform for %s\n",
			__func__, alg_name);
		return -EINVAL;
	}

	/* Copy the key from user and set to TFM. */
	if (mackey != NULL) {

		ret = crypto_ahash_setkey(hdata->async.s, mackey, mackeylen);
		if (unlikely(ret)) {
			dprintk(1, KERN_DEBUG,
				"Setting hmac key failed for %s-%zu.\n",
				alg_name, mackeylen * 8);
			ret = -EINVAL;
			goto error;
		}
	}

	hdata->digestsize = crypto_ahash_digestsize(hdata->async.s);

	hdata->async.result = kmalloc(sizeof(*hdata->async.result), GFP_KERNEL);
	if (unlikely(!hdata->async.result)) {
		ret = -ENOMEM;
		goto error;
	}

	memset(hdata->async.result, 0, sizeof(*hdata->async.result));
	init_completion(&hdata->async.result->completion);

	hdata->async.request = ahash_request_alloc(hdata->async.s, GFP_KERNEL);
	if (unlikely(!hdata->async.request)) {
		dprintk(0, KERN_ERR, "error allocating async crypto request\n");
		ret = -ENOMEM;
		goto error;
	}

	ahash_request_set_callback(hdata->async.request,
				   CRYPTO_TFM_REQ_MAY_BACKLOG,
				   cryptodev_complete, hdata->async.result);

	ret = crypto_ahash_init(hdata->async.request);
	if (unlikely(ret)) {
		dprintk(0, KERN_ERR, "error in crypto_hash_init()\n");
		goto error_request;
	}

	hdata->init = 1;
	return 0;

error_request:
	ahash_request_free(hdata->async.request);
error:
	kfree(hdata->async.result);
	crypto_free_ahash(hdata->async.s);
	return ret;
}

int cryptodev_hash_clone(struct hash_data *hdata, struct hash_data *old_data,
			 const void *mackey, size_t mackeylen)
{
	const char *algo;
	void *state;
	int ret;

	/* We want exactly the same driver. */
	algo = crypto_tfm_alg_driver_name(crypto_ahash_tfm(old_data->async.s));
	ret = cryptodev_hash_init(hdata, algo, mackey, mackeylen);
	if (unlikely(ret != 0))
		return ret;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31)
	state = kmalloc(crypto_ahash_digestsize(hdata->async.s), GFP_KERNEL);
#else
	state = kmalloc(crypto_ahash_statesize(hdata->async.s), GFP_KERNEL);
#endif
	if (unlikely(state == NULL)) {
		ret = -ENOMEM;
		goto err;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31)
	crypto_ahash_export(old_data->async.request, state);
#else
	ret = crypto_ahash_export(old_data->async.request, state);
	if (unlikely(ret != 0)) {
		dprintk(0, KERN_ERR, "error exporting hash state\n");
		goto err;
	}
#endif

	ret = crypto_ahash_import(hdata->async.request, state);
	if (unlikely(ret != 0)) {
		dprintk(0, KERN_ERR, "error in crypto_hash_init()\n");
		goto err;
	}

	kfree(state);

	hdata->init = 1;
	return 0;

err:
	kfree(state);
	cryptodev_hash_deinit(hdata);
	return ret;
}

void cryptodev_hash_deinit(struct hash_data *hdata)
{
	if (hdata->init) {
		if (hdata->async.request)
			ahash_request_free(hdata->async.request);
		kfree(hdata->async.result);
		if (hdata->async.s)
			crypto_free_ahash(hdata->async.s);
		hdata->init = 0;
	}
}

int cryptodev_hash_reset(struct hash_data *hdata)
{
	int ret;
	ret = crypto_ahash_init(hdata->async.request);
	if (unlikely(ret)) {
		dprintk(0, KERN_ERR, "error in crypto_hash_init()\n");
		return ret;
	}

	return 0;
}

ssize_t cryptodev_hash_update(struct hash_data * hdata, struct scatterlist * sg,
			      size_t len)
{
	int ret;

	reinit_completion(&hdata->async.result->completion);
	ahash_request_set_crypt(hdata->async.request, sg, NULL, len);

	ret = crypto_ahash_update(hdata->async.request);

	return waitfor(hdata->async.result, ret);
}

ssize_t _cryptodev_hash_update(struct hash_data * hdata, const void *data,
			       size_t len)
{
	struct scatterlist sg;

	sg_init_one(&sg, data, len);

	return cryptodev_hash_update(hdata, &sg, len);
}

int cryptodev_hash_final(struct hash_data *hdata, void *output)
{
	int ret;

	reinit_completion(&hdata->async.result->completion);
	ahash_request_set_crypt(hdata->async.request, NULL, output, 0);

	ret = crypto_ahash_final(hdata->async.request);

	return waitfor(hdata->async.result, ret);
}
