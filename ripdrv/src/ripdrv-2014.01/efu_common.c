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
#include "efu_privileged.h"

int EFU_isEngineeringFeatureUnlocked(EFU_BITMASK_TYPE efu_feature_bit) {
    EFU_BITMASK_TYPE unlockedfeatures_bitmask;

    if (!(efu_feature_bit & EFU_SUPPORTEDFEATURES_BITMASK)) {
        /* This feature is not supported in this release */
        return false;
    }

    if (EFU_getBitmask(&unlockedfeatures_bitmask, EFU_DEFAULT_TAG) == EFU_RET_SUCCESS) {
        // Bitmask found and valid, now check field
        if (unlockedfeatures_bitmask & efu_feature_bit) {
            return true;
        }
        else {
            // bitmask valid, but bit not set
            return false;
        }
    }
    else {
        // Bitmask not found or not valid
        return false;
    }
}

