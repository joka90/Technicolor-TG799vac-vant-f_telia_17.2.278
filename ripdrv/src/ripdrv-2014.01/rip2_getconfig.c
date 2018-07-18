#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <inttypes.h>
#include <arpa/inet.h> //for endianess conversion routines
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#include "crc.h"
#include "rip2.h"

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

#define RIP_FILENAME_LENGTH         500		// arbitrary filename length
#define RIP_ITEM_CONFIG_LENGTH      RIP_FILENAME_LENGTH+32
#define RIP_ID_MAX                  0x10000 // eRIP ID are coded in 2 bytes, hence this maximum

void usage( char *name ){
	printf( "%s: Get RIPv2 configuration and data\n", basename(name));
	printf( "The tool will create a configuration file (<filename>.config) listing all parameters of the file given in arguments.\n");
	printf( "It will also create 1 file per parameter (<filename>.<parameter ID>) holding the content of the parameter.\n\n");
	printf( "Syntax: %s [option] <filename>\n", basename(name));
	printf( "	<filename>\t\t\teRIPv2 binary\n");
	printf( "\nOptions:\n" );
	printf( "\t-o, --output <directory>\tOutput files to given directory\n");
	printf( "\t-h, --help\t\t\tThis help\n");
	printf( "\n");
}

int open_and_map_eRIP_file( char *ripfilename, uint8_t **map ){
	int infd;
	struct stat inst;
	int rv=-1;
	int ripSize;

	infd = open(ripfilename, O_RDONLY);
	if (infd == -1) {
		fprintf(stderr, "Error: Opening eRIPv2 binary %s (%s)\n", ripfilename, strerror(errno));
	} else {
		fstat(infd, &inst);
		ripSize = inst.st_size;
		if (ripSize == 0) {
			fprintf(stderr, "Error: empty file, aborting!\n");
		} else {
			/* mmap the output file so we can treat it as memory */
			*map = mmap(NULL, ripSize, PROT_READ, MAP_PRIVATE, infd, 0);
			if (MAP_FAILED == *map) {
				fprintf(stderr,"Mapping to input file (%d: %s)\n", errno, strerror(errno));
			} else {
				rv = ripSize;
			}
		}
		close(infd);
	}
	return rv;
}


int main( int argc, char *argv[]){

	uint8_t *ripbuf;
	int ret;
	char *default_directory=".";
	char *ripv2_filename=NULL;
	int ripSize;

	static struct option long_options[] = {
		{"help", no_argument, 0,'h'},
		{"output", required_argument, 0, 'o'},
		{0, 0, 0, 0}
	};
	int option_index = 0;
	int c;

	if (argc < 2) exit(0);

	while ((c = getopt_long(argc, argv, "ho:", long_options, &option_index)) != -1) {
		switch (c) {
			case 'h':
				usage(argv[0]);
				exit(0);
				break;
			case 'o':
				default_directory = optarg;
				break;
			default:
				exit(1);
		}
	}

	if (argv[optind]) {
		ripv2_filename = argv[optind];
	} else {
		fprintf(stderr, "Error: Need eRIPv2 file\n");
		exit(1);
	}

	ripSize = open_and_map_eRIP_file( ripv2_filename, &ripbuf );
	if( ripSize < 0 ) {
		exit(1);
	}

	rip2_flash_init(ripbuf, ripSize);

	if (rip2_init(0, 1, ripSize) < 0) {
		fprintf(stderr, "Error intializing RIP2 library\n");
		munmap(ripbuf, ripSize);
		exit(1);
	}

	/* Create an empty eRIPv2 configuration file */
	char ripconfig_filename[RIP_FILENAME_LENGTH];
	memset(ripconfig_filename, 0, sizeof(ripconfig_filename));
	snprintf(ripconfig_filename, RIP_FILENAME_LENGTH, "%s/%s.config", default_directory, basename(ripv2_filename));
	FILE *ripconfig_fd = fopen(ripconfig_filename, "w");
	if (ripconfig_fd == NULL) {
		fprintf(stderr,"Error: Opening output file %s (%s)\n", ripconfig_filename, strerror(errno));
		munmap(ripbuf, ripSize);
		exit(1);
	}

	/* Reset the table holding the number of times a certain ID was seen */
	uint8_t ripid_count[RIP_ID_MAX];
	memset(ripid_count, 0, sizeof(ripid_count));

	/* Loop through all eRIPv2 parameters */
	T_RIP2_ITEM *current_item = (T_RIP2_ITEM *) NULL;
	T_RIP2_ITEM eripv2_item;
	ret = rip2_get_next( &current_item, RIP2_ATTR_ANY, &eripv2_item );
	while( ret == RIP2_SUCCESS ){
		/* Keep track of the number of times a certain ID was seen */
		ripid_count[BETOH16(eripv2_item.ID)]++;

		char ripitem_filename[RIP_FILENAME_LENGTH];
		memset(ripitem_filename, 0, sizeof(ripitem_filename));
		/* Check if the ID is not the first one, add a counter to the ID */
		if( ripid_count[BETOH16(eripv2_item.ID)] == 1){
			snprintf( ripitem_filename, RIP_FILENAME_LENGTH, "%s/%s.%4.4X", default_directory, basename(ripv2_filename), BETOH16(eripv2_item.ID));
		} else {
			snprintf( ripitem_filename, RIP_FILENAME_LENGTH, "%s/%s.%4.4X.%d", default_directory, basename(ripv2_filename), BETOH16(eripv2_item.ID), ripid_count[BETOH16(eripv2_item.ID)]);
			fprintf( stderr, "Warning: eRIPv2 item %4.4X has %d duplicates\n",  BETOH16(eripv2_item.ID), ripid_count[BETOH16(eripv2_item.ID)]);
		}

		/* Add parameter to configuration file */
		char ripconfig_line[RIP_ITEM_CONFIG_LENGTH];
		snprintf(ripconfig_line, RIP_ITEM_CONFIG_LENGTH, "%4.4X:%8.8X:%8.8X:FILE = \"%s\"\n",
				BETOH16(eripv2_item.ID),
				BETOH32(eripv2_item.attr[ATTR_HI]),
				BETOH32(eripv2_item.attr[ATTR_LO]),
				ripitem_filename
		);
		if( fputs(ripconfig_line, ripconfig_fd) == EOF) {
			fprintf(stderr,"Error: Writing to file %s failed\n", ripconfig_filename);
			fclose(ripconfig_fd);
			munmap(ripbuf, ripSize);
			exit(1);
		}

		/* Create parameter file*/
		FILE *ripitem_fd = fopen(ripitem_filename, "w");
		if (ripitem_fd == NULL) {
			fprintf(stderr,"Error: Opening output file %s\n", ripitem_filename);
			fclose(ripconfig_fd);
			munmap(ripbuf, ripSize);
			exit(1);
		}

		/* Check if the attribute is wrong */
		if( BETOH32(eripv2_item.length) == 0xFFFFFFFF ){
			fprintf(stderr, "Error: Getting RIP2 data for ID %04x: bad length\n", BETOH16(eripv2_item.ID));
			fclose(ripitem_fd);
			fclose(ripconfig_fd);
			munmap(ripbuf, ripSize);
			exit(1);
		}

		/* Write parameter data into parameter file */
		for(int i=0; i<BETOH32(eripv2_item.length); i++){
			fputc(*(ripbuf + BETOH32(eripv2_item.addr) - RIP2_OFFSET + i), ripitem_fd);
		}

		/* Close parameter file */
		fclose(ripitem_fd);

		/* Get next eRIPv2 parameter */
		ret = rip2_get_next( &current_item, RIP2_ATTR_ANY, &eripv2_item );
	}

	fclose(ripconfig_fd);
	munmap(ripbuf, ripSize);

	exit(0);
}
