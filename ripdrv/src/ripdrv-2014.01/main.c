#include "rip2.h"
#include "rip_proc.h"
#include "rip_efu_proc.h"
#include "rip_ids.h"
#include <linux/module.h>
#include <linux/init.h>

#include "rip2.h"
#if defined(CONFIG_MIPS_BCM963XX) || defined(CONFIG_ARCH_BCM63XX)
#include "boardparms.h"
#endif

#define NVRAM_MAC_ADDRESS_LEN           6

#ifdef SET_GPON_SERIAL_NUM
#define BCM_PLOAM_SERIAL_NUMBER_SIZE_BYTES 8
#endif

#ifdef SET_MAC_ADDRESS
extern void kerSysSetLanMacAddress( unsigned char *pucaMacAddr );
extern void kerSysSetWlanMacAddress( unsigned char *pucaMacAddr );
#endif
#ifdef SET_GPON_SERIAL_NUM
extern void kerSysSetGponSerialNumber( unsigned char *pGponSerialNumber );
#endif

#ifdef SET_GPON_SERIAL_NUM
/* convert a binary data array to it's hex string representation */
static int hex_to_str(unsigned char *source,
		      unsigned int length, /* in bytes */
		      char          *out)
{
	const unsigned char *hexChars = "0123456789ABCDEF";
	int byteCount = 0;
	int index     = 0;

	if ((NULL == source) || (NULL == out) || (0 == length))
		return 0;

	for (; byteCount < length; byteCount++) {
		out[index++] = hexChars[(source[byteCount] & 0xF0) >> 4];
		out[index++] = hexChars[source[byteCount] & 0x0F];
	}
	out[index] = 0; /* terminate the string */

	return 1;
}
#endif

static void populate_Dsl_boardparams(void)
{
#if defined(CONFIG_MIPS_BCM963XX) || defined(CONFIG_ARCH_BCM63XX)
    int ret;
    unsigned long length;
    unsigned short custom_pattern, dsl_pattern;

    length = sizeof(custom_pattern);
    ret = rip2_drv_read(&length, RIP_ID_UNPROT_FREE1, (void *) &custom_pattern);
    if (ret == RIP2_SUCCESS) {
        char boardID[BP_BOARD_ID_LEN];
        char *boardExt = '\0';
        char *boardSfpExt = '\0';

        custom_pattern = ntohs(custom_pattern);
        dsl_pattern = custom_pattern;

        ret = BpGetBoardId(boardID);
        if (ret != BP_SUCCESS)
            printk("\n*** Board is not initialized properly ***\n\n");
        else {
            if ((boardID[4] == '-') && (boardID[6] == '\0')) {
                dsl_pattern &= RIP_CP_DSL_MASK;
                if (dsl_pattern == RIP_CP_DSL_POTS) {
                    printk("Dsl Annex A board\n");
                    boardExt = "";
                } else if (dsl_pattern == RIP_CP_DSL_ISDN) {
                    printk("Dsl Annex B board\n");
                    boardExt = "_ISDN";
                } else if (dsl_pattern == RIP_CP_DSL_M) {
                    printk("Dsl Annex M board\n");
                    boardExt = "_M";
                } else if (dsl_pattern == RIP_CP_DSL_BJ) {
                    printk("Dsl Annex B/J board\n");
                    boardExt = "_BJ";
                } else {
                    printk(KERN_ERR "Not supported DSL HW configuration\n");
                    return;
                }

                length= strlen(boardExt);
                strncpy(&boardID[6], boardExt, length+1);

                if ((custom_pattern & RIP_CP_SFP) != 0) {
                    printk("SFP board\n");
                    boardSfpExt = "_SFP";
                    length= strlen(boardSfpExt);
                    strncpy(&boardID[6+strlen(boardExt)], boardSfpExt, length+1);
                }

                printk("Set board (%s)\n", boardID);

                ret = BpSetBoardId(boardID);
                if (ret != BP_SUCCESS) {
                    printk("\n*** Board is not initialized properly ***\n\n");
                }
            } else {
                printk(KERN_ERR "Invalid TCH boardname (%s)\n", boardID);
            }
        }
    } else
        printk(KERN_ERR "Failed to read custom pattern from RIP\n");
#endif
}

static void populate_boardparams(void)
{
#if defined(SET_MAC_ADDRESS) || defined(SET_GPON_SERIAL_NUM)
	int ret;
	unsigned long length;
#endif

#ifdef SET_MAC_ADDRESS
	unsigned char mac_address[NVRAM_MAC_ADDRESS_LEN];
#endif

#ifdef SET_GPON_SERIAL_NUM
	unsigned char ontSerialNumber[4];  /* Used with companyId to construct ONT Unique Serial Number */
#endif

#ifdef SET_MAC_ADDRESS
	length = sizeof(mac_address);
	ret = rip2_drv_read(&length, RIP_ID_LAN_ADDR, mac_address);

	if (ret == RIP2_SUCCESS)
		kerSysSetLanMacAddress(mac_address);
	else
		printk(KERN_ERR "Failed to read LAN MAC address from RIP\n");

	length = sizeof(mac_address);
	ret = rip2_drv_read(&length, RIP_ID_WLAN_LAN_ADDR, mac_address);

	if (ret == RIP2_SUCCESS)
		kerSysSetWlanMacAddress(mac_address);
	else
		printk(KERN_ERR "Failed to read WLAN MAC address from RIP\n");
#endif

#ifdef SET_GPON_SERIAL_NUM
	length = sizeof(ontSerialNumber);
	ret = rip2_drv_read(&length, RIP_ID_SERIAL_NBR_BYTES, ontSerialNumber);
	if (ret == RIP2_SUCCESS) {
		unsigned char tSerialNumber[BCM_PLOAM_SERIAL_NUMBER_SIZE_BYTES];
		unsigned char ontSerialNumberBytes[9];
		unsigned char companyId[4];
		unsigned long companyId_len = 0;

		companyId_len = sizeof(companyId);
		ret = rip2_drv_read(&companyId_len, RIP_ID_COMPANY_ID, companyId);
		if (ret == RIP2_SUCCESS) {
			memcpy(tSerialNumber, companyId, sizeof(companyId));
			hex_to_str(ontSerialNumber, length, ontSerialNumberBytes);
			memcpy(tSerialNumber + sizeof(companyId), ontSerialNumberBytes, sizeof(ontSerialNumberBytes));

			kerSysSetGponSerialNumber(tSerialNumber);
		}
	}
#endif

  populate_Dsl_boardparams();
}

static int __init mymodule_init(void)
{
	int ripSize;

	if (rip2_flash_init(NULL, 0) != 0)
		return -1;

	ripSize = rip2_flash_get_size();

	if( ripSize < 0 ) {
		printk(KERN_ERR "Failed to get eRIP flash size\n");
		return -1;
	}

	rip2_init(0, true, ripSize);

	rip_proc_init();
	rip_efu_proc_init();

	populate_boardparams();

	return 0;
}

static void __exit mymodule_exit(void)
{
	rip_efu_proc_cleanup();
	rip_proc_cleanup();
	rip2_flash_release();
	return;
}

module_init(mymodule_init);
module_exit(mymodule_exit);

MODULE_LICENSE("GPL");
