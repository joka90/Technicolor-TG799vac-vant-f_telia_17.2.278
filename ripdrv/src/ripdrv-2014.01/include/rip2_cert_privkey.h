#ifndef __T_RIP2_CERT_PRIVKEY_H__
#define __T_RIP2_CERT_PRIVKEY_H__

typedef enum
{
	UNKOWN,
	CERT,
	PRIVKEY
} CERT_PRIVKEY_TYPE;

int rip2_get_cert_privkey_position(char *raw_data, uint32_t raw_data_len, CERT_PRIVKEY_TYPE data_type, uint32_t *offset, uint32_t *len);

#endif /* __T_RIP2_CERT_PRIVKEY_H__ */
