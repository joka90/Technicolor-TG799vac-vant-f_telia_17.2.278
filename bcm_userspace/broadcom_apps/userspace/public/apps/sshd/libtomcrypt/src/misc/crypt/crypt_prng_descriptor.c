/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtomcrypt.org
 */
#include "tomcrypt.h"

/**
  @file crypt_prng_descriptor.c
  Stores the PRNG descriptors, Tom St Denis
*/  
struct ltc_prng_descriptor prng_descriptor[TAB_SIZE] = {
{ NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

LTC_MUTEX_GLOBAL(ltc_prng_mutex);


/* $Source: /cvs/libtom/libtomcrypt/src/misc/crypt/crypt_prng_descriptor.c,v $ */
/* $Revision: 1.5 $ */
/* $Date: 2005/06/19 18:00:28 $ */
