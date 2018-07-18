/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
** Copyright (c) 2013-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
*************************************************************************/

#ifndef __LEGACY_UPGRADE_INFO_H
#define __LEGACY_UPGRADE_INFO_H


typedef enum {
  RS_ID_DUMMY = 0,          /* invalid index */
  RS_ID_UPGRADE_STEP,
  RS_ID_UPGRADE_ERROR,
  RS_ID_IMAGE_STRUCT,
  RS_ID_UPGRADE_XFER_IP,
  RS_ID_UPGRADE_XFER_PORT,
  RS_ID_UPGRADE_ATTEMPT,
  RS_ID_UPGRADE_MAX_ATTEMPT,
  RS_ID_UPGRADE_TRIGGER,
  RS_ID_UPGRADE_STARTBANK,
  RS_ID_UPGRADE_NON_BOOT_BANK,
  RS_ID_UPGRADE_CONTROL_MODE,
  RS_ID_UPGRADE_MANUAL_SWITCHOVER,
#ifdef OMCI_UPGRADE
  RS_ID_UPGRADE_BANKINFO1,
  RS_ID_UPGRADE_BANKINFO2,
#endif
  RS_ID_NUM                 /* keep last */
} raw_storage_id;

#define UPGRADED_MAX_URL_LEN            (256)
#define UPGRADED_MAX_TRIGGER_LEN        (64)
#define UPGRADED_MAX_USERNAME_LEN       (64)
#define UPGRADED_MAX_PASSWORD_LEN       (64)
#define UPGRADED_MAX_ERROR_CODE_LEN     (16)
#define UPGRADED_MAX_ERROR_DESC_LEN     (128)
#define UPGRADED_MAX_ERROR_LEN          (UPGRADED_MAX_ERROR_DESC_LEN)
#define UPGRADED_MAX_KEY_LEN            (32)
#define UPGRADED_MAX_FILENAME_LEN       (256)
#define UPGRADED_MAX_VERSION_LEN        (14)

#define UPGRADED_STEP_VALUE_SIZE              (4)

#pragma pack(1)

/* upgraded image info */
typedef struct image {
  unsigned int flags;
  char url[UPGRADED_MAX_URL_LEN+1];
  char username[UPGRADED_MAX_USERNAME_LEN+1];
  char password[UPGRADED_MAX_PASSWORD_LEN+1];
  char key[UPGRADED_MAX_KEY_LEN+1];
#ifdef SFS_DUAL_BANK_SUPPORT
  /* Attention: 
     It's Obsolated parameter ! BUT should still include this parameter 
     in project after R8.4.2 for compatibility !!!
     Or else there will be error to read 'image' data from RAWStorage if upgrade from R8.4.2 -> R8.4.3*/
  char downloadFileName[UPGRADED_MAX_FILENAME_LEN+1];
#endif
} Image, *Image_ptr;

#pragma pack()

#define PROC_LEGACY_UPGRADE "legacy_upgrade"
#define PROC_LEGACY_UPGRADE_URL "url"
#define PROC_LEGACY_UPGRADE_USR "username"
#define PROC_LEGACY_UPGRADE_PASSWD "password"
#define PROC_LEGACY_UPGRADE_KEY "key"
#define PROC_LEGACY_UPGRADE_FILE "filename"
#define PROC_LEGACY_UPGRADE_FLAGS "flags"
#define PROC_LEGACY_UPGRADE_ERASE "erase_upgrade_info"

typedef enum {
  UPGRADE_URL,
  UPGRADE_USER,
  UPGRADE_PASSWORD,
  UPGRADE_KEY,
  UPGRADE_FILENAME,
  UPGRADE_FLAGS
} Image_item;

int load_legacy_upgrade_info(void);
void free_legacy_upgrade_info(void);
void publish_legacy_upgrade_info(void);
void unpublish_legacy_upgrade_info(void);

#endif /* __LEGACY_UPGRADE_INFO_H */
