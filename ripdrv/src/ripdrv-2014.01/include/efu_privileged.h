/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2014  -  Technicolor Delivery Technologies, SAS        **
** - All Rights Reserved                                                **
**                                                                      **
** Technicolor hereby informs you that certain portions                 **
** of this software module and/or Work are owned by Technicolor         **
** and/or its software providers.                                       **
**                                                                      **
** Distribution copying and modification of all such work are reserved  **
** to Technicolor and/or its affiliates, and are not permitted without  **
** express written authorization from Technicolor.                      **
**                                                                      **
** Technicolor is registered trademark and trade name of Technicolor,   **
** and shall not be used in any manner without express written          **
** authorization from Technicolor                                       **
**                                                                      **
*************************************************************************/

#include "efu_common.h"

/* Store a new tag */
unsigned int    EFU_verifyStoredTag(void);
unsigned int    EFU_storeTemporalTag(unsigned char * unlockTag_a, unsigned long unlockTag_size);

/* Get information */
EFU_CHIPID_TYPE EFU_getChipid(void);
void            EFU_getOSIK(unsigned char * pubkey, unsigned long pubkey_length);
char*           EFU_getSerialNumber(void); // WARNING: returns string that needs to be freed after use!!
unsigned int    EFU_getBitmask(EFU_BITMASK_TYPE *bitmask_p, int tag_location);

