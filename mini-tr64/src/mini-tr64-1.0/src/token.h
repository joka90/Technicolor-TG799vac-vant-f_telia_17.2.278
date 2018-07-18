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
  * @file token.h
  * @brief Simple helper functions to create a token string for adding to the url in CSRF protection mode
  * @author Jeroen Z
  */

#ifndef __TOKEN_H_
#define __TOKEN_H_

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

/**
  * Function to generate a random token string
  */
int generate_token_string( char *buffer, int bufsize );

#endif // __TOKEN_H_
