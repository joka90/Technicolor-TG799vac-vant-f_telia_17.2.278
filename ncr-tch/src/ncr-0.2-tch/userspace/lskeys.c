/*
 * Demo on how to list the keys stored in NCR.
 * This can be used to do some clean up when NCR is compiled in with the
 * PERSITENCE option:
 * # lskeys
 * # rmkey 1234
 *
 * Placed under public domain.
 *
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include "../ncr.h"
#include <stdlib.h>

static int display_key_info(int cfd, ncr_key_t key)
{
	NCR_STRUCT(ncr_key_get_info) kinfo;
	struct nlattr *nla;
	uint16_t *attr_p;
	uint32_t algo, flags, type;
	char id[MAX_KEY_ID_SIZE];
	algo = flags = type = 0;
	memset(id, 0, MAX_KEY_ID_SIZE);

	nla = NCR_INIT(kinfo);
	kinfo.f.output_size = sizeof(kinfo);
	kinfo.f.key = key;
	attr_p = ncr_reserve(&nla, NCR_ATTR_WANTED_ATTRS, 4 * sizeof(*attr_p));
	*attr_p++ = NCR_ATTR_ALGORITHM;
	*attr_p++ = NCR_ATTR_KEY_FLAGS;
	*attr_p++ = NCR_ATTR_KEY_TYPE;
	*attr_p++ = NCR_ATTR_KEY_ID;
	NCR_FINISH(kinfo, nla);

	if (ioctl(cfd, NCRIO_KEY_GET_INFO, &kinfo)) {
		printf("\t%-8d | INVALID KEY\n", key);
		return 0;
	}

	if (kinfo.f.output_size < sizeof (kinfo.f)) {
		fprintf(stderr, "No nlattr returned\n");
		return 1;
	}
	nla = (struct nlattr *)(&kinfo.f + 1);
	for (;;) {
		void *data;

		if (nla->nla_len >
		    kinfo.f.output_size - ((char *)nla - (char *)&kinfo)) {
			fprintf(stderr, "Attributes overflow\n");
			return 1;
		}
		data = (char *)nla + NLA_HDRLEN;
		switch (nla->nla_type) {
		case NCR_ATTR_ALGORITHM:
			if (nla->nla_len < NLA_HDRLEN + sizeof(uint32_t)) {
				fprintf(stderr, "Attribute too small\n");
				return 1;
			}
			algo = *(uint32_t *) data;
			break;
		case NCR_ATTR_KEY_FLAGS:
			if (nla->nla_len < NLA_HDRLEN + sizeof(uint32_t)) {
				fprintf(stderr, "Attribute too small\n");
				return 1;
			}
			flags = *(uint32_t *) data;
			break;
		case NCR_ATTR_KEY_TYPE:
			if (nla->nla_len < NLA_HDRLEN + sizeof(uint32_t)) {
				fprintf(stderr, "Attribute too small\n");
				return 1;
			}
			type = *(uint32_t *) data;;
			break;
		case NCR_ATTR_KEY_ID:
			if (nla->nla_len > NLA_HDRLEN + MAX_KEY_ID_SIZE) {
				fprintf(stderr, "Attribute too long\n");
				return 1;
			}
			strncpy(id, data, nla->nla_len - NLA_HDRLEN);
			id[nla->nla_len - NLA_HDRLEN] = '\0';
			break;
		}

		if (NLA_ALIGN(nla->nla_len) + NLA_HDRLEN >
		    kinfo.f.output_size - ((char *)nla - (char *)&kinfo))
			break;
		nla = (struct nlattr *)((char *)nla + NLA_ALIGN(nla->nla_len));
	}
	printf("\t%-8d | %s-%u-%u-%u\n", key, id, algo, flags, type);

	return 0;
}

static int list_keys(int cfd)
{
	ncr_key_t keys[MAX_KEY_LIST];
	int i = -1, num = -1;

	printf("NCR Keys:\n"
			"\tdesc     | id-algo-flags-type\n"
			"\t---------+-----------------------\n");

	do {
		num = ioctl(cfd, NCRIO_KEY_LIST, &keys);
		if (num < 0)  {
			fprintf(stderr, "Error: %s:%d\n", __func__, __LINE__);
			perror("ioctl(NCRIO_KEY_LIST)");
			return 1;
		}

		while(++i < num && keys[i] != -1)
		{
			display_key_info(cfd, keys[i]);
		}
	}
	while (num == MAX_KEY_LIST);

	return 0;
}

int main()
{
	int fd = -1;

	/* Open the crypto device */
	fd = open("/dev/ncr", O_RDWR, 0);
	if (fd < 0) {
		perror("open(/dev/ncr)");
		return EXIT_FAILURE;
	}

	if (list_keys(fd))
		return EXIT_SUCCESS;

	return EXIT_SUCCESS;
}
