/************** COPYRIGHT AND CONFIDENTIALITY INFORMATION *********************
 **                                                                          **
 ** Copyright (c) 2010 Technicolor                                           **
 ** All Rights Reserved                                                      **
 **                                                                          **
 ** This program contains proprietary information which is a trade           **
 ** secret of TECHNICOLOR and/or its affiliates and also is protected as     **
 ** an unpublished work under applicable Copyright laws. Recipient is        **
 ** to retain this program in confidence and is not permitted to use or      **
 ** make copies thereof other than as permitted in a written agreement       **
 ** with TECHNICOLOR, UNLESS OTHERWISE EXPRESSLY ALLOWED BY APPLICABLE LAWS. **
 **                                                                          **
 ** Programmer(s) : Joris Gorinsek (email : joris.gorinsek@technicolor.com)  **
 **                                                                          **
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <malloc.h>
#include <inttypes.h>
#include <arpa/inet.h> //for endianess conversion routines
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <getopt.h>


#include "crc.h"
#include "rip2.h"

#define TP_STR      "ASCII"
#define TP_HEX      "HEX"
#define TP_FILE     "FILE"
#define TP_CMD      "EXEC"

#if (_BIG_ENDIAN)
#define HTOBE16(x)    (x)
#define HTOBE32(x)    (x)
#define BETOH16(x)    (x)
#define BETOH32(x)    (x)
#else
#define HTOBE16(x)    (htons(x))
#define HTOBE32(x)    (htonl(x))
#define BETOH16(x)    (ntohs(x))
#define BETOH32(x)    (ntohl(x))
#endif

typedef struct {
	uint32_t RIP2_idtag;
	uint32_t imagesize;
	uint32_t headersize;
	uint32_t crc;
}T_RIP2_FILEHEADER;

typedef struct {
	uint16_t id;
	uint16_t size;
}T_RIP1_ELEM;

#ifndef RIP2TAG
#define RIP2TAG            0xaacd2b85
#endif

//#define DEBUG

#ifdef DEBUG
#define DPRINTF(x...)	printf(x)
#else
#define DPRINTF(x...)
#endif


static T_RIP1_ELEM rip_size[] = {
	{0x0000, 0x0002}, //"RIP CheckSum"
	{0x0002, 0x0002}, //"Custum Pattern"
	{0x0002, 0x0002}, //"Custum Pattern"
	{0x0004, 0x000c}, //"PBA Code"
	{0x0010, 0x0002}, //"PBA ICS"
	{0x0012, 0x0010}, //"PBA Serial nr"
	{0x0022, 0x0006}, //"Product Date"
	{0x0028, 0x0002}, //"FIA code"
	{0x002A, 0x0002}, //"FIM code"
	{0x002C, 0x0006}, //"Repair Date"
	{0x0032, 0x0006}, //"Ethernet MAC"
	{0x0038, 0x0004}, //"Company ID"
	{0x003C, 0x0004}, //"Factory ID"
	{0x0040, 0x0008}, //"Board Name"
	{0x0048, 0x0004}, //"Memory Conf"
	{0x004C, 0x0006}, //"USB MAC"
	{0x0052, 0x0001}, //"Registered
	{0x0053, 0x0030}, //"VPVC Table"
	{0x0083, 0x0005}, //"Access Code"
	{0x0088, 0x0005}, //"Remote Mgr Pwd"
	{0x008D, 0x0006}  //"WiFi MAC"
};
static int  Do_Padding = 0; //determines if we use padding or not

/*
   uint32_t htonl(uint32_t hostlong);
   uint16_t htons(uint16_t hostshort);
   uint32_t ntohl(uint32_t netlong);
   uint16_t ntohs(uint16_t netshort);
 */

/* Helper function to find the amount of bytes to pad to */
static uint16_t get_padding(uint16_t id)
{
	int i = 0;
	int nbelem =  sizeof(rip_size)/sizeof(T_RIP1_ELEM);

	if (Do_Padding == 1) {
		for (i = 0; i < nbelem ; i++) {
			if (rip_size[i].id == id)
				return rip_size[i].size;
		}
	}
	return 0xFFFF; //signal error
}

/* Help text */
const char exRipBuilderVersion[] = {"RIPv2 Builder v0.8\n"};
const char exRipBuilderInfo[] =
{
		"Syntax: [-e|-h|-p|-v] [-s size] <input file> <output file>\n"
		"Option:\n"
		"  -e, --help        : Print these lines and exit\n"
		"  -h, --header      : Add bootloader header for TFTP binary image\n"
		"  -p, --padding     : Pad the old RIPv1 values to their maximum size, regardless of input\n"
		"  -s, --size <size> : Set eRIP size to <size> (default is 131072 bytes)\n"
		"  -v, --version     : Print version information and exit\n"
};

/* Global flags */

/*
 * Get rid of remaining special characters such as \" \r \n
 *
 */
static char *sanitize(char *input)
{
	int i, j, quotes = 0;

	j = 0; //will be used to modify the original string

	for (i = 0; i < strlen(input); i++) {
		// replace \n by string terminator
		switch (input[i]) {
			case '\r':
			case '\n':
				input[j] = '\0';
				j++;
				break;

			case ' ':
				/* if we have seen an uneven amount of quotes its ok to remove
				 * these chars, otherwise keep them */
				if (quotes %2) {
					input[j] = input[i];
					j++;
				}
				break;

			case '\"':
				quotes++;
				break;

			default:
				input[j] = input[i];
				j++;
				break;
		}
	}
	return input;
}


static uint8_t *do_hex_input(uint8_t        *data,
		char           *arg,
		unsigned long  *len,
		unsigned int   available)
{
	int   cnt, i, items_read = 0;
	char  *infile  = NULL;
	char  hex[3]   = "\0\0\0";
	char  b        = 0;

	/* Sanitize input */
	infile = sanitize(arg);

	cnt = 0;
	/* We only accept HEX input if it is a multiple of 2 nibbles */
	if (strlen(infile) % 2) {
		return NULL;
	}

	if (strlen(infile) / 2 > available) {
		fprintf(stderr, "Input data is too big (%d) to fit in the remaining eRIP space (%d), aborting!\n", (unsigned int) (strlen(infile) / 2), available);
		return NULL;
	}

	for (i = 0; i < strlen(infile) - 1; i += 2) {
		hex[0] = infile[i];
		hex[1] = infile[i + 1];

		items_read = sscanf(hex, "%02hhx", &b);
		if (items_read == 1) {
			DPRINTF("Got hex value 0x%02hhx\n", b);
			data[cnt] = b;
			cnt++;
		}
	}

	*len = cnt;
	return data;
}

/* Converts hex flag (32bit) to number, with length check, assume it already
   is formatted in big endian */
static int process_flag(char *input, uint32_t  *output, unsigned int available)
{
	unsigned long len = 0;

	if (strlen(input) != 8)
		return -1;

	if (!do_hex_input((uint8_t *) output, input, &len, available))
		return -1;

	*output = BETOH32(*output);

	if (len != 4)
		return -1;

	return 0;
}

/*
 * process input file and return the contents as a data buffer
 * RETURNS: pointer to the data buffer containing the file content
 *
 *
 */
static uint8_t *do_file_input(uint8_t       *data,
		char          *arg,
		unsigned long *len,
		unsigned int  available)
{
	int         fd;
	uint8_t     *inbuf;
	struct stat inst;

	if ((fd = open(arg, O_RDONLY)) == -1) {
		fprintf(stderr,"Opening input file %s\n", arg);
		return NULL;
	}

	fstat(fd, &inst);

	if (inst.st_size > available) {
		fprintf(stderr, "Input file (%s) is too big (%d) to fit in the remaining eRIP space (%d), aborting!\n", arg, (unsigned int) inst.st_size, available);
		goto err;
	}

	/* mmap the input file */
	inbuf = mmap(0, inst.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

	if (MAP_FAILED == inbuf) {
		fprintf(stderr,"Mapping the input file\n");
		goto err;
	}

	memcpy(data, inbuf, inst.st_size);

	*len = inst.st_size;

	close(fd);
	munmap(inbuf, inst.st_size);
	return data;

err:
	close(fd);
	return NULL;
}

/*
 * Does whatever is needed to get input data into a buffer.
 * Allocates the buffer and returns it.
 *
 * IN: type: a string representing the type of action that will need to be
 *            performed to acquired the data.
 *     arg: the command | string | filename that will provide the data
 *     available: remaining space left on the erip
 *
 * OUT: len: the length of the data buffer that is returned.
 *
 * RETURNS: pointer to the data buffer in case of success; this memory
 *          should be freed by the caller!
 *          NULL in case of failure.
 */
static uint8_t *get_input_data(unsigned long  *len,
		const char     *type,
		char           *arg,
		unsigned int   available)
{
	uint8_t *data = NULL;

	if ((len == NULL) || (type == NULL) || (arg == NULL) || (available <= 0)) {
		return NULL;
	}

	data = malloc(available);
	if (data == NULL) {
		fprintf(stderr,"Allocating internal buffer\n");
		return NULL;
	}

	/* string input */
	if (0 == strncasecmp(type, TP_STR, strlen(TP_STR))) {
		char * str = sanitize(arg);
		memset(data, ' ', available); //strings need to be padded with spaces
		if (strlen(str) > available) {
			fprintf(stderr, "Input string is too big (%d) to fit in the remaining eRIP space (%d), aborting!\n", (unsigned int) strlen(str), available);
		}
		strncpy((char*)data, str, strlen(str));
		*len = strlen(str);
		return data;
	}

	/* hex input */
	if (0 == strncasecmp(type, TP_HEX, strlen(TP_HEX))) {
		memset(data, 0x0, available); // binary stuff needs to be padded with 0
		data = do_hex_input(data, arg, len, available);
		if (data == NULL) {
			goto err;
		}
		return data;
	}

	/* file input */
	if (0 == strncasecmp(type, TP_FILE, strlen(TP_FILE))) {
		char * infile = sanitize(arg);
		memset(data, 0x0, available); // binary stuff needs to be padded with 0

		data = do_file_input(data, infile, len, available);
		if (data == NULL) {
			goto err;
		}

		return data;
	}

	/* cmd input */
	if (0 == strncasecmp(type, TP_CMD, strlen(TP_CMD))) {
		int ret = 0;
		char *cmd = sanitize(arg);

		DPRINTF("command input: \"%s\"\n", cmd);
		ret = system(cmd);

		if (0 > ret) {
			fprintf(stderr, "Error: executing %s returned %d\n", arg, ret);
			goto err;
		}
		memset(data, 0x0, available); // binary stuff needs to be padded with 0
		data = do_file_input(data, "temp.exrip", len, available);

		return data;
	}

err:
	free(data);
	return NULL;
}

/*
 * Performs the real heavy lifting: processes the input file line by line:
 * parses each line, adds the corresponding data and index item to the RIP2
 * buffer.
 * IN: infile: pointer to the memory mapped input file
 *     outfile: pointer to the memory mapped output file
 *
 * RETURNS: the space left in the eRIP  upon success, a negative value upon failure
 */
int process_inputfile(FILE    *infile,
		uint8_t *outfile,
		unsigned int ripSize)
{
	int           items_read, i=0;
	char          line[LINE_MAX];
	T_RIP2_ID     ID;
	char          *sAttrHi, *sAttrLo, *type, *arg, *tmp;
	const char    *delim = "=:";
	uint32_t      attrHi = 0, attrLo = 0;
	uint16_t	  pad    = 0;
	unsigned long len    = 0;
	uint8_t       *data  = NULL;
	unsigned int  space_left = ripSize - sizeof(T_RIP2_HDR);


	/* Process the input file line by line */
	while (fgets(line, LINE_MAX, infile) != NULL) {

		i++;
		if ((line[0] == '#') ||
				(line[0] == ' ') ||
				(line[0] == '\n') ||
				(line[0] == '\r')){
			/* skip this line */
			continue;
		}

		/* Try to parse the line */
		items_read = 0;

		/* Chop the string up in substrings */
		tmp = strtok(line, delim);
		if (tmp != NULL) {
			items_read = sscanf(tmp, "%04hx:", &ID);
		}

		if (items_read == 1) {
			sAttrHi  = strtok(NULL, delim);
			sAttrLo  = strtok(NULL, delim);
			type   = strtok(NULL, delim);
			arg    = strtok(NULL, delim);
			DPRINTF("ID: %04x, AttrHi: %s, AttrLo: %s, type: %s, arg: %s, space:%d\n", ID, sAttrHi, sAttrLo, type, arg, space_left);

			data = get_input_data(&len, type, arg, space_left);
			if (data == NULL) {
				goto err;
			}

			if (0 > process_flag(sAttrHi, &attrHi, space_left)) {
				goto err;
			}

			if (0 > process_flag(sAttrLo, &attrLo, space_left)) {
				goto err;
			}

			/* Check if input needs padding */
			pad = get_padding(ID);
			if ((pad != 0xFFFF) && (len < pad)) {
				DPRINTF("going to add %d padding bytes\r\n", pad - (unsigned int) len);
				len = pad;
			}

			/* Now add it to the RIP2*/
			if (0 > rip2_drv_write(data, len, ID, attrHi, attrLo)) {
				fprintf(stderr, "Error while adding item with index %x\n", ID);
				goto err;
			}
			space_left -= len + len%2 + sizeof(T_RIP2_ITEM) + 4; /* data + 1 byte padding if odd data length + table entry + CRC32 */
			free(data);

		} else {
			fprintf(stderr, "Syntax error while processing line %d\n", i);
			return -1;
		}
	}

	return space_left;

err:
	if(data) free(data);
	return -1;
}

/*
 * Process an input file and generate a corresponding RIPv2 sector
 */
int generate_rip2(char *config, char *rip_bin, unsigned int Set_ERIP_Header, unsigned int ripSize)
{
	int     outfd;
	FILE    *infd;
	uint8_t *ripbuf;
	int     space_left;

	/* Generate a CRC table */
	rip2_mk_crc32_table(CRC32, rip2_crc32_hw);

	/********* Prepare input and output files for business *********/
	infd = fopen(config, "r");
	if (infd == NULL) {
		fprintf(stderr,"Error opening input file %s\n", config);
		goto out_err;
	}

	outfd = open(rip_bin, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (outfd == -1) {
		fprintf(stderr,"Error opening output file %s\n", rip_bin);
		fclose(infd);
		goto out_err;
	}

	if (ripSize == 0)
		ripSize = RIP2_SZ;

	ripbuf = malloc(ripSize);

	if (ripbuf == 0) {
		fprintf(stderr,"Error allocating memory\n");
		goto out_err_close;
	}
	memset(ripbuf, 0xFF, ripSize);
	rip2_flash_init(ripbuf, ripSize);

	if (0 > rip2_init(ripbuf, 0, ripSize)) {
		fprintf(stderr, "Error intializing RIP2 library\n");
		goto out_err_close;
	}

	/********** Start processing and generate the RIP2 ***********/
	space_left = process_inputfile(infd, ripbuf, ripSize);
	if (space_left < 0) {
		goto out_err_unmap;
	}

	/* Prepend header for MBH upload */
	if (Set_ERIP_Header) {
		unsigned int crcsize = ripSize + sizeof(T_RIP2_FILEHEADER);
		uint8_t * tempimg  = malloc(crcsize);
		if (!tempimg) {
			fprintf(stderr, "Error allocating buffer\n");
			goto out_err_unmap;
		}
		memcpy(tempimg + sizeof(T_RIP2_FILEHEADER), ripbuf,ripSize);

		T_RIP2_FILEHEADER * header = (T_RIP2_FILEHEADER *)tempimg;
		header->imagesize = HTOBE32(ripSize);
		header->RIP2_idtag = HTOBE32(RIP2TAG);
		header->headersize = HTOBE32(sizeof(T_RIP2_FILEHEADER));
		header->crc = 0;
		header->crc = HTOBE32(rip2_crc32((unsigned char *)header,crcsize));
		if (write(outfd, header,sizeof(T_RIP2_FILEHEADER)) != sizeof(T_RIP2_FILEHEADER)) {
			fprintf(stderr, "Error writing RIP2 header to file\n");
			free(tempimg);
			goto out_err_unmap;
		}
		free(tempimg);
	}

	if (write(outfd, ripbuf,ripSize) != ripSize) {
		fprintf(stderr, "Error writing RIP2 data to file.\n");
		goto out_err_unmap;
	}

	/* done successful */
	printf("%s eRIPv2 created (space left=%d)\n", rip_bin, space_left);

	free(ripbuf);
	fclose(infd);
	close(outfd);
	return 0;

out_err_unmap:
out_err_close:
	free(ripbuf);
	fclose(infd);
	close(outfd);

out_err:
	printf("RIPv2 creation failed!\n");

	return -1;
}

int main(int argc, char *argv[])
{
	unsigned int  Set_ERIP_Header = 0;
	char          *config = NULL, *rip_bin = NULL;
	unsigned int  ripSize = 0;

	static struct option long_options[] = {
		{"help", no_argument, 0,'e'},
		{"header", no_argument, 0,'h'},
		{"padding", no_argument, 0, 'p'},
		{"size", required_argument, 0, 's'},
		{"version", no_argument, 0, 'v'},
		{0, 0, 0, 0}
	};

	if (argc < 3) {
		fprintf(stderr, "Error: Expecting more arguments\n");
		fprintf(stderr, "%s", exRipBuilderInfo);
		return 1;
	}

	int option_index = 0;
	int c;
	while ((c = getopt_long(argc, argv, "hvps:", long_options, &option_index)) != -1) {
		switch (c) {
			case 'e':
				printf("%s", exRipBuilderVersion);
				printf("%s", exRipBuilderInfo);
				exit(0);
				break;
			case 'h':
				Set_ERIP_Header = 1;
				break;
			case 'p':
				Do_Padding = 1;
				break;
			case 's':
				ripSize = (unsigned int) strtod(optarg, NULL);
				break;
			case 'v':
				printf("%s", exRipBuilderVersion);
				exit(0);
				break;
			default:
				exit(1);
		}
	}

	if (argv[optind]) {
		config = argv[optind];
	} else {
		fprintf(stderr, "Error: Need eRIPv2 configuration filename\n");
		exit(1);
	}

	if (argv[++optind]) {
		rip_bin = argv[optind];
	} else {
		fprintf(stderr, "Error: Need a filename where the eRIP will be written to\n");
		exit(1);
	}

	if (0 > generate_rip2(config, rip_bin, Set_ERIP_Header, ripSize)) {
		return 1;
	}

	return 0;
}
