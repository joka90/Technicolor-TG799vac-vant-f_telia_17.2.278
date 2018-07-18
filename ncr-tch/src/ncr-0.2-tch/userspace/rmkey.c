/*
 * Demo on how to remove a key stored in NCR.
 * This can be used to do some clean up when NCR is compiled in with the
 * PERSITENCE option:
 * # lskeys
 * # rmkey 1234
 *
 * Placed under public domain.
 *
 */
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include "../ncr.h"

int usage(char *prog)
{
	printf("USAGE: %s <key_desc>\n", prog);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	int fd = -1;
	char *endptr, *str;
	int val;

	if (argc < 2)
		usage(argv[0]);

	/* Open the crypto device */
	fd = open("/dev/ncr", O_RDWR, 0);
	if (fd < 0) {
		perror("open(/dev/ncr)");
		return 1;
	}

	str = argv[1];
	val = strtol(str, &endptr, 10);

	/* Check for various possible errors */

	if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN))
			|| (errno != 0 && val == 0)) {
		perror("strtol");
		usage(argv[0]);
	}

	if (endptr == str) {
		usage(argv[0]);
	}

	if (ioctl(fd, NCRIO_KEY_DEINIT, &val))
	{
		fprintf(stderr, "Error: %s:%d - %d\n", __func__, __LINE__, val);
		perror("ioctl(NCRIO_KEY_DEINIT)");
		return 1;
	}

	return 0;
}
