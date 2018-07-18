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
