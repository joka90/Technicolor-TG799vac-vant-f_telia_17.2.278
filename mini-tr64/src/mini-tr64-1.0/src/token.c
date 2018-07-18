/************** COPYRIGHT AND CONFIDENTIALITY INFORMATION ********************
**                                                                          **
** Copyright (c) 2014 Technicolor                                           **
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

/**
  * @file token.c
  * @brief Token generation functions implmentation
  */

#include "token.h"

#include <stdio.h>
#include <unistd.h>

#define TMP_TOKEN_FILE "/tmp/tr064_token"


/**
  * Generate random string token
  */
int generate_token_string( char *buffer, int bufsize )
{
	int i = 0;

	if( access( TMP_TOKEN_FILE , F_OK ) != -1 ) {
	   FILE *fd = fopen( TMP_TOKEN_FILE, "r" );
	   fread( buffer, bufsize, 1, fd );
	   buffer[ bufsize - 1 ] = '\0';
	   fclose( fd );

	} else {
	    // file doesn't exist


	srandom( time(0) );
	for ( i = 0; i < bufsize -1; i++ )
	{
		char rv = (  (double)random() / (double)RAND_MAX ) * ( 'Z' - 'A' )  + 'A' ;
		if ( random() > RAND_MAX / 2 )
		{
			rv = rv | 0x20; // Trick to change the bit for upper/lower case letters in ASCII
		}

		buffer[i] = rv;
	}

	buffer[bufsize - 1] = '\0';

	FILE *fd = fopen( TMP_TOKEN_FILE, "w+" );
	fwrite( buffer, bufsize, 1, fd );
	fclose( fd );

	}


	return 0;
}

