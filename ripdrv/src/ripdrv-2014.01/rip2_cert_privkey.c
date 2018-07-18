/************** COPYRIGHT AND CONFIDENTIALITY INFORMATION *********************
**                                                                          **
** Copyright (c) 2017 Technicolor                                           **
** All Rights Reserved                                                      **
**                                                                          **
** This program contains proprietary information which is a trade           **
** secret of TECHNICOLOR and/or its affiliates and also is protected as     **
** an unpublished work under applicable Copyright laws. Recipient is        **
** to retain this program in confidence and is not permitted to use or      **
** make copies thereof other than as permitted in a written agreement       **
** with TECHNICOLOR, UNLESS OTHERWISE EXPRESSLY ALLOWED BY APPLICABLE LAWS. **
**                                                                          **
******************************************************************************/

#include <linux/version.h>
#include <linux/string.h>
#include <linux/slab.h>
#include "rip2_cert_privkey.h"

// Defines for checking on the begin and end of private key
#define PEM_PKCS1_PRIVKEY_BEGIN "-----BEGIN RSA PRIVATE KEY-----\n"
#define PEM_PKCS1_PRIVKEY_END "-----END RSA PRIVATE KEY-----\n"
#define PEM_PKCS8_PRIVKEY_BEGIN "-----BEGIN PRIVATE KEY-----\n"
#define PEM_PKCS8_PRIVKEY_END "-----END PRIVATE KEY-----\n"
#define PEM_PKCS8ENC_PRIVKEY_BEGIN "-----BEGIN ENCRYPTED PRIVATE KEY-----\n"
#define PEM_PKCS8ENC_PRIVKEY_END "-----END ENCRYPTED PRIVATE KEY-----\n"

// Defines for checking on the begin and end of certificate
#define CERT_BEGIN "-----BEGIN CERTIFICATE-----\n"
#define CERT_END "-----END CERTIFICATE-----\n"

/* Get the posistion and length of the certificate/private key inside a buffer
   param raw_data: The pointer to the buffer which contains the whole data (certificate and private key)
   param raw_data_len: The length of the buffer
   param data_type: The type of data we want to get, either CERT or PRIVKEY
   param offset: The pointer where the start position of the CERT/PRIVKEY would be stored after the function returns
   param len: The pointer where the length of CERT/PRIVKEY would be stored after the function returns
   return: -1 CERT/PRIVKEY NOT found
            0 CERT/PRIVKEY found
*/
int rip2_get_cert_privkey_position(char *raw_data, uint32_t raw_data_len, CERT_PRIVKEY_TYPE data_type, uint32_t *offset, uint32_t *len)
{
	char *begin = NULL, *end = NULL, *itr = NULL;

	char *tmp = kmalloc(raw_data_len + 1, GFP_KERNEL);

	if (tmp == NULL)
		return -1;
	memcpy(tmp, raw_data, raw_data_len);
	tmp[raw_data_len] = '\0'; //Add terminating \0 to ensure there is no overflow

	switch(data_type){
	case CERT:
		begin = strstr(tmp, CERT_BEGIN);
		if (begin == NULL)
		{
			kfree(tmp);
			return -1;
		}

		end = begin;
		while( end < tmp + raw_data_len)
		{
			itr = strstr(end, CERT_END);
			if(itr != NULL)
				end = itr + strlen(CERT_END);
			else
				break;
		}
		break;
	case PRIVKEY:
		begin = strstr(tmp, PEM_PKCS8_PRIVKEY_BEGIN);
		end = strstr(tmp, PEM_PKCS8_PRIVKEY_END);
		if(begin != NULL && end != NULL)
		{
			end += strlen(PEM_PKCS8_PRIVKEY_END);
			break;
		}

		begin = strstr(tmp, PEM_PKCS1_PRIVKEY_BEGIN);
		end = strstr(tmp, PEM_PKCS1_PRIVKEY_END);
		if(begin != NULL && end != NULL)
		{
			end += strlen(PEM_PKCS1_PRIVKEY_END);
			break;
		}

		begin = strstr(tmp, PEM_PKCS8ENC_PRIVKEY_BEGIN);
		end = strstr(tmp, PEM_PKCS8ENC_PRIVKEY_END);
		if(begin != NULL && end != NULL)
		{
			end += strlen(PEM_PKCS8ENC_PRIVKEY_END);
			break;
		}
	default:
		kfree(tmp);
		return -1;
	}

	if (end <= begin)
	{
		kfree(tmp);
		return -1;
	}

	*offset = begin - tmp;
	*len = end - begin;
	kfree(tmp);
	return 0;
}

