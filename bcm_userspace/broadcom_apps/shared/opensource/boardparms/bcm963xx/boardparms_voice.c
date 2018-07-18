/*
    Copyright 2000-2010 Broadcom Corporation

    <:label-BRCM:2011:DUAL/GPL:standard

    Unless you and Broadcom execute a separate written software license
    agreement governing use of this software, this software is licensed
    to you under the terms of the GNU General Public License version 2
    (the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
    with the following added to such license:

       As a special exception, the copyright holders of this software give
       you permission to link this software with independent modules, and
       to copy and distribute the resulting executable under terms of your
       choice, provided that you also meet, for each linked independent
       module, the terms and conditions of the license of that module.
       An independent module is a module which is not derived from this
       software.  The special exception does not apply to any modifications
       of the software.

    Not withstanding the above, under no circumstances may you combine
    this software in any way with any other Broadcom software provided
    under a license other than the GPL, without Broadcom's express prior
    written consent.

:>
*/

/**************************************************************************
* File Name  : boardparms_voice.c
*
* Description: This file contains the implementation for the BCM63xx board
*              parameter voice access functions.
*
***************************************************************************/

/* ---- Include Files ---------------------------------------------------- */

#include "boardparms_voice.h"
#include "bp_defs.h"
#include <bcm_map_part.h>

#if !defined(_CFE_)
#include <linux/kernel.h>
#endif /* !defined(_CFE_) */

/* ---- Public Variables ------------------------------------------------- */
/* ---- Private Constants and Types -------------------------------------- */
#define VOICE_PINMUX_RETRIEVAL 1

#if VOICE_PINMUX_RETRIEVAL
#define BP_VOICE_ADD_INTERFACE_PINMUX( pElem, intfEnableFlag )  { pElem->id = bp_ulInterfaceEnable; pElem->u.ul = intfEnableFlag; pElem++; }
#define BP_VOICE_ADD_SIGNAL_PINMUX( pElem, itemId, usVal ) { pElem->id = itemId; pElem->u.us = usVal; pElem++; }
#define FILTERED_BP_MAX_SIZE   20
#endif /*VOICE_PINMUX_RETRIEVAL*/

/* ---- Private Variables ------------------------------------------------ */
static char voiceCurrentDgtrCardCfgId[BP_BOARD_ID_LEN] = VOICE_BOARD_ID_DEFAULT;
static int g_BpDectPopulated = 1;
#if VOICE_PINMUX_RETRIEVAL
static bp_elem_t g_voice_filteredBp[FILTERED_BP_MAX_SIZE];
#endif /*VOICE_PINMUX_RETRIEVAL*/

/* ---- Public Functions -------------------------------------------------- */
bp_elem_t * BpGetVoicePmuxBp( bp_elem_t * pCurrentDataBp );

/* ---- External Functions ------------------------------------------------ */
extern bp_elem_t * BpGetElem(enum bp_id id, bp_elem_t **pstartElem, enum bp_id stopAtId);
extern char *BpGetSubCp(enum bp_id id, bp_elem_t *pstartElem, enum bp_id stopAtId );
extern void *BpGetSubPtr(enum bp_id id, bp_elem_t *pstartElem, enum bp_id stopAtId );
extern unsigned char BpGetSubUc(enum bp_id id, bp_elem_t *pstartElem, enum bp_id stopAtId );
extern unsigned short BpGetSubUs(enum bp_id id, bp_elem_t *pstartElem, enum bp_id stopAtId );
extern unsigned long BpGetSubUl(enum bp_id id, bp_elem_t *pstartElem, enum bp_id stopAtId );

/* ---- Private Functions ------------------------------------------------ */
static void bpmemcpy( void* dstptr, const void* srcptr, int size );
static char * bpstrcpy( char* dest, const char* src );
static bp_elem_t * BpGetVoiceBoardStartElemPtr( char * pszBaseBoardId );
#if VOICE_PINMUX_RETRIEVAL
static int BpIsIntfEnabled( unsigned int interfaceFlag, bp_elem_t * pBoardParms );
static int BpElemExists( bp_elem_t * pBoardParms, enum bp_id  id );
#endif /*VOICE_PINMUX_RETRIEVAL*/

#if !defined(_CFE_)
static int bpstrlen( char * src );
static enum bp_id mapDcRstPinToBpType( BP_RESET_PIN rstPin );
static enum bp_id mapDcSpiDevIdToBpType( BP_SPI_SIGNAL spiId );
static unsigned int BpGetZSISpiDevID( void );
static unsigned short BpGetSlaveSelectGpioNum( BP_SPI_PORT ssNum);
#endif /* !defined(_CFE_) */

/*
 * -------------------------- Voice Daughter Board Configs ------------------------------
 */

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_LE88506 =
{
   VOICECFG_LE88506_STR,     /* szBoardId */
   2,             /* numFxsLines */
   0,             /* numFxoLines */
   {
      /* voiceDevice0 parameters */
      {
         /* Device type */
         BP_VD_ZARLINK_88506,
         BP_SPI_SS_B1,  /* Device uses SPI_SS_B1 pin. Pin on base board depends on base board parameters. */
         BP_RESET_FXS1, /* Device uses FXS1 reset pin. Pin on base board depends on base board parameters. */

         /* Channel description */
         {
            /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               0,
               0
            },
            /* Channel 1 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               1,
               1
            }
         }
      },
      /* Always end the device list with BP_NULL_DEVICE */
      BP_NULL_DEVICE_MACRO_NEW,
   },
   /* SLIC Device Profile */
   BP_VD_FLYBACK,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE  )
};


VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_SI32267 =
{
   VOICECFG_SI32267_STR,   /*Daughter board ID */
   2,   /*Number of FXS Lines */
   0,   /*Number of FXO Lines */
   {
      {
         /* Device Type */
         BP_VD_SILABS_32267,
         BP_SPI_SS_NOT_REQUIRED,   /* ISI SPI CS handled internally. It is mapped by the zsiChipMap list. */
         BP_RESET_FXS1,
         /* Channel description */
         {
            /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               0,
               0
            },
            /* Channel 1 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               1,
               1
            }
         }
      },

      BP_NULL_DEVICE_MACRO_NEW,
   },
   /* SLIC Device Profile */
   BP_VD_FB_TSS_ISO,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_ISI_SUPPORT | BP_FLAG_DSP_PCMHAL_ENABLE  )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_SI3217x =
{
   VOICECFG_SI3217X_STR,   /*Daughter Card ID */
   2,   /*Number of FXS Lines */
   1,   /*Number of FXO Lines */
   {
      {
         /* Device Type */
         BP_VD_SILABS_32176,
         BP_SPI_SS_B1,  /* Device uses SPI_SS_B1 pin. Pin on base board depends on base board parameters. */
         BP_RESET_FXS1, /* Device uses FXS1 reset pin. Pin on base board depends on base board parameters. */
         {
            /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               0,
               0
            },
            {
               BP_VOICE_CHANNEL_INACTIVE,
               BP_VCTYPE_NONE,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               BP_TIMESLOT_INVALID,
               BP_TIMESLOT_INVALID
            }
         }
      },
      {
         /* Device Type 2 */
         BP_VD_SILABS_32178,
         BP_SPI_SS_B2,  /* Device uses SPI_SS_B2 pin. Pin on base board depends on base board parameters. */
         BP_RESET_FXS2, /* Device uses FXS2 reset pin. Pin on base board depends on base board parameters. */
         {
            /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               1,
               1
            },
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_DAA,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               2,
               2
            }
         }
      },

      BP_NULL_DEVICE_MACRO_NEW,
   },
   /* SLIC Device Profile */
   BP_VD_FLYBACK,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE  )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_VE890_INVBOOST =
{
   VOICECFG_VE890_INVBOOST_STR,   /* daughter card ID */
   2,   /* FXS number is 2 */
   1,   /* FXO number is 1 */
   {   /* voiceDevice0 parameters */
      {
         /* Device Type */
         BP_VD_ZARLINK_89116,
         BP_SPI_SS_B1,  /* Device uses SPI_SS_B1 pin. Pin on base board depends on base board parameters. */
         BP_RESET_FXS1, /* Device uses FXS1 reset pin. Pin on base board depends on base board parameters. */
         {
            /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               0,
               0
            },
            {
               BP_VOICE_CHANNEL_INACTIVE,
               BP_VCTYPE_NONE,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               BP_TIMESLOT_INVALID,
               BP_TIMESLOT_INVALID
            }
         }
      },
      {
         /* Device Type 2 */
         BP_VD_ZARLINK_89316,
         BP_SPI_SS_B2,  /* Device uses SPI_SS_B2 pin. Pin on base board depends on base board parameters. */
         BP_RESET_FXS2, /* Device uses FXS2 reset pin. Pin on base board depends on base board parameters. */
         {
            /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               1,
               1
            },
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_DAA,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               2,
               2
            }
         }
      },

      /* Always end device list with this macro. */
      BP_NULL_DEVICE_MACRO_NEW,
   },
   /* SLIC Device Profile */
   BP_VD_INVBOOST,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE  )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_SI32260x2 =
{
   VOICECFG_SI32260x2_STR,   /*Daughter Card ID*/
   4,   /*Number of FXS Lines*/
   0,   /*Number of FXO Lines*/
   {   /* voiceDevice0 Parameters */
      {
         /* Device Type */
         BP_VD_SILABS_32261,
         BP_SPI_SS_B1,  /* Device uses SPI_SS_B1 pin. Pin on base board depends on base board parameters. */
         BP_RESET_FXS1, /* Device uses FXS1 reset pin. Pin on base board depends on base board parameters. */
         /* Channel description */
         {
            /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               0,
               0
            },
            /* Channel 1 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               1,
               1
            }
         }
      },
      {
         /* Device Type 2*/
         BP_VD_SILABS_32261,
         BP_SPI_SS_B2,  /* Device uses SPI_SS_B2 pin. Pin on base board depends on base board parameters. */
         BP_RESET_FXS2, /* Device uses FXS2 reset pin. Pin on base board depends on base board parameters. */
         /* Channel description */
         {
            /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               2,
               2
            },
            /* Channel 1 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               3,
               3
            }
         }
      },

      /* Always end device list with this macro. */
      BP_NULL_DEVICE_MACRO_NEW,
   },
   /* SLIC Device Profile */
   BP_VD_FB_TSS,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_SI32260 =
{
   VOICECFG_SI32260_STR,   /*Daughter Card ID*/
   2,   /*Number of FXS Lines*/
   0,   /*Number of FXO Lines*/
   {   /* voiceDevice0 Parameters */
      {
         /* Device Type */
         BP_VD_SILABS_32261,
         BP_SPI_SS_B1,  /* Device uses SPI_SS_B1 pin. Pin on base board depends on base board parameters. */
         BP_RESET_FXS1, /* Device uses FXS1 reset pin. Pin on base board depends on base board parameters. */
         /* Channel description */
         {
            /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               0,
               0
            },
            /* Channel 1 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               1,
               1
            }
         }
      },

      /* Always end device list with this macro. */
      BP_NULL_DEVICE_MACRO_NEW,
   },
   /* SLIC Device Profile */
   BP_VD_PMOS_BUCK_BOOST,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_SI32260_SI3050 =
{
   VOICECFG_SI32260_SI3050_STR,   /*Daughter card ID */
   2,   /* Number of FXS Lines */
   1,   /* Number of FXO Lines */
   {
      {
         /* Device Type */
         BP_VD_SILABS_32261,
         BP_SPI_SS_B1,  /* Device uses SPI_SS_B1 pin. Pin on base board depends on base board parameters. */
         BP_RESET_FXS1, /* Device uses FXS1 reset pin. Pin on base board depends on base board parameters. */
         {
            /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               0,
               0
            },
            /* Channel 1 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               1,
               1
            }
         }
      },
      {
         /* Device Type 2 */
         BP_VD_SILABS_3050,
         BP_SPI_SS_B2,  /* Device uses SPI_SS_B2 pin. Pin on base board depends on base board parameters. */
         BP_RESET_FXS2, /* Device uses FXS2 reset pin. Pin on base board depends on base board parameters. */
         {
            /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_DAA,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               2,
               2
            },
            /* There is no second channel on Si3050 so mark it as inactive */
            {
               BP_VOICE_CHANNEL_INACTIVE,
               BP_VCTYPE_NONE,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               BP_TIMESLOT_INVALID,
               BP_TIMESLOT_INVALID
            },
         }
      },

      /* Always end device list with this macro. */
      BP_NULL_DEVICE_MACRO_NEW,
   },
   /* SLIC Device Profile */
   BP_VD_PMOS_BUCK_BOOST,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_SI32260_SI3050_QC =
{
   VOICECFG_SI32260_SI3050_QC_STR,   /*Daughter card ID */
   2,   /* Number of FXS Lines */
   1,   /* Number of FXO Lines */
   {
      {
         /* Device Type */
         BP_VD_SILABS_32261,
         BP_SPI_SS_B1,  /* Device uses SPI_SS_B1 pin. Pin on base board depends on base board parameters. */
         BP_RESET_FXS1, /* Device uses FXS1 reset pin. Pin on base board depends on base board parameters. */
         {
            /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               0,
               0
            },
            /* Channel 1 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               1,
               1
            }
         }
      },
      {
         /* Device Type 2 */
         BP_VD_SILABS_3050,
         BP_SPI_SS_B2,  /* Device uses SPI_SS_B2 pin. Pin on base board depends on base board parameters. */
         BP_RESET_FXS2, /* Device uses FXS2 reset pin. Pin on base board depends on base board parameters. */
         {
            /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_DAA,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               2,
               2
            },
            /* There is no second channel on Si3050 so mark it as inactive */
            {
               BP_VOICE_CHANNEL_INACTIVE,
               BP_VCTYPE_NONE,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               BP_TIMESLOT_INVALID,
               BP_TIMESLOT_INVALID
            },
         }
      },

      /* Always end device list with this macro. */
      BP_NULL_DEVICE_MACRO_NEW,
   },
   /* SLIC Device Profile */
   BP_VD_QCUK,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_SI32260_LCQC =
{
   VOICECFG_SI32260_LCQC_STR,   /*Daughter card ID */
   2,   /* Number of FXS Lines */
   0,   /* Number of FXO Lines */
   {
      {
         /* Device Type */
         BP_VD_SILABS_32261,
         BP_SPI_SS_B1,  /* Device uses SPI_SS_B1 pin. Pin on base board depends on base board parameters. */
         BP_RESET_FXS1, /* Device uses FXS1 reset pin. Pin on base board depends on base board parameters. */
         {
            /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               0,
               0
            },
            /* Channel 1 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               1,
               1
            }
         }
      },
      /* Always end device list with this macro. */
      BP_NULL_DEVICE_MACRO_NEW,
   },
   /* SLIC Device Profile */
   BP_VD_LCQCUK,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_LE88536_ZSI =
{
   VOICECFG_LE88536_ZSI_STR,   /* Daughter Board ID */
   2,   /* Number of FXS Lines */
   0,   /*Number of FXO Lines */
   {   /* Voice Device 0 Parameters */
      {
         BP_VD_ZARLINK_88536,   /* Device Type */
         BP_SPI_SS_NOT_REQUIRED,   /* ZSI SPI CS handled internally. It is mapped using the zsiMapList. */
         BP_RESET_FXS1,
         /* Channel description */
         {
            /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               2,
               2
            },
            /* Channel 1 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               3,
               3
            },
         }
      },

      /* Always end device list with this macro. */
      BP_NULL_DEVICE_MACRO_NEW,
   },
   /* SLIC Device Profile */
   BP_VD_INVBOOST,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_ZSI_SUPPORT | BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_ZL88601 =
{
   VOICECFG_ZL88601_STR,   /* szBoardId */
   2,   /* Number of FXS Lines */
   0,   /* Number of FXO Lines */
   {
      {
         /* Device Type */
         BP_VD_ZARLINK_88601,
         BP_SPI_SS_B1,  /* Device uses SPI_SS_B1 pin. Pin on base board depends on base board parameters. */
         BP_RESET_FXS1, /* Device uses FXS1 reset pin. Pin on base board depends on base board parameters. */
         /* Channel Description */
         {
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               0,
               0
            },
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               1,
               1
            }
         }
      },

      /* Always end device list with this macro. */
      BP_NULL_DEVICE_MACRO_NEW,
   },
   /* SLIC Device Profile */
   BP_VD_FLYBACK,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_ZL88701 =
{
   VOICECFG_ZL88701_STR,   /* szBoardId */
   2,   /* Number of FXS Lines */
   1,   /* Number of FXO Lines */
   {
      {
         /* Device Type */
         BP_VD_ZARLINK_88701,
         BP_SPI_SS_B1,  /* Device uses SPI_SS_B1 pin. Pin on base board depends on base board parameters. */
         BP_RESET_FXS1, /* Device uses FXS1 reset pin. Pin on base board depends on base board parameters. */
         /* Channel Description */
         {
            /* Channel 0 on device 0 */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               0,
               0
            },
            /* Channel 1 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               1,
               1
            }
         }
      },
      {
         /* Device Type 2 */
         BP_VD_ZARLINK_89010,
         BP_SPI_SS_B2,  /* Device uses SPI_SS_B2 pin. Pin on base board depends on base board parameters. */
         BP_RESET_NOT_REQUIRED,  /* FXO reset pin tied with FXS on this board.*/
         /* Channel description */
         {
            /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_DAA,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               2,
               2
            },
            /* There is no second channel on Le89010 so mark it as inactive */
            {
               BP_VOICE_CHANNEL_INACTIVE,
               BP_VCTYPE_NONE,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               BP_TIMESLOT_INVALID,
               BP_TIMESLOT_INVALID
            },
         }
      },

      /* Always end device list with this macro. */
      BP_NULL_DEVICE_MACRO_NEW,
   },
   /* SLIC Device Profile */
   BP_VD_INVBOOST,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_ZL88702_ZSI =
{
   VOICECFG_ZL88702_ZSI_STR,   /* szBoardId */
   2,   /* Number of FXS Lines */
   0,   /* Number of FXO Lines */
   {
      {
         /* First 2 lines */
         /* Device Type */
         BP_VD_ZARLINK_88702_ZSI,
         BP_SPI_SS_NOT_REQUIRED,  /* Device uses SPI_SS_B1 pin. Pin on base board depends on base board parameters. */
         BP_RESET_FXS1, /* Device uses FXS1 reset pin. Pin on base board depends on base board parameters. */
         /* Channel Description */
         {
            /* Channel 0 on device 0 */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               2,
               2
            },
            /* Channel 1 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               3,
               3
            }
         }
      },

      /* Always end device list with this macro. */
      BP_NULL_DEVICE_MACRO_NEW,
   },
   /* SLIC Device Profile */
   BP_VD_FLYBACK,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_ZSI_SUPPORT | BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_LE9672_ZSI = /* for ZLRR96741H Rev A0 daughter card */
{
   VOICECFG_LE9672_ZSI_STR,   /* szBoardId */
   2,   /* Number of FXS Lines */
   0,   /* Number of FXO Lines */
   {
      {
         /* First 2 lines */
         /* Device Type */
         BP_VD_ZARLINK_9672_ZSI,
         BP_SPI_SS_NOT_REQUIRED,  /* Device uses SPI_SS_B1 pin. Pin on base board depends on base board parameters. */
         BP_RESET_FXS1, /* Device uses FXS1 reset pin. Pin on base board depends on base board parameters. */
         /* Channel Description */
         {
            /* Channel 0 on device 0 */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               2,
               2
            },
            /* Channel 1 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               3,
               3
            }
         }
      },

      /* Always end device list with this macro. */
      BP_NULL_DEVICE_MACRO_NEW
   },
   /* SLIC Device Profile */
   BP_VD_INVBOOST,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_ZSI_SUPPORT | BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_LE9661_ZSI =
{
   VOICECFG_LE9661_ZSI_STR,   /* szBoardId */
   1,   /* Number of FXS Lines */
   0,   /* Number of FXO Lines */
   {
      {
         /* Device Type */
         BP_VD_ZARLINK_9661,
         BP_SPI_SS_NOT_REQUIRED,  /* ZSI SPI CS handled internally. It is mapped using the zsiMapList */
         BP_RESET_FXS1, /* Device uses FXS1 reset pin. Pin on base board depends on base board parameters. */
         /* Channel Description */
         {
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               2,
               2
            },
            {
               BP_VOICE_CHANNEL_INACTIVE,
               BP_VCTYPE_NONE,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               BP_TIMESLOT_INVALID,
               BP_TIMESLOT_INVALID
            }
         }
      },

      /* Always end device list with this macro. */
      BP_NULL_DEVICE_MACRO_NEW
   },
   /* SLIC Device Profile */
   BP_VD_FLYBACK,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_ZSI_SUPPORT | BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_ZL88801_89010_BB = /* for Microsemi ZLR88842L REV A0 DC */
{
   VOICECFG_ZL88801_89010_BB_STR,   /* szBoardId */
   2,   /* Number of FXS Lines */
   1,   /* Number of FXO Lines */
   {
      {
         /* First 2 lines */
         /* Device Type */
         BP_VD_ZARLINK_88801,
         BP_SPI_SS_B1,  /* Device uses SPI_SS_B1 pin. Pin on base board depends on base board parameters. */
         BP_RESET_FXS1, /* Device uses FXS1 reset pin. Pin on base board depends on base board parameters. */
         /* Channel Description */
         {
            /* Channel 0 on device 0 */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               0,
               0
            },
            /* Channel 1 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               1,
               1
            }
         }
      },
      {
         /* Device Type 2 */
         BP_VD_ZARLINK_89010,
         BP_SPI_SS_B2,  /* Device uses SPI_SS_B2 pin. Pin on base board depends on base board parameters. */
         BP_RESET_NOT_REQUIRED,  /* FXO reset pin tied with FXS on this board.*/
         /* Channel description */
         {
            /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_DAA,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               2,
               2
            },
            /* There is no second channel on Le89010 so mark it as inactive */
            {
               BP_VOICE_CHANNEL_INACTIVE,
               BP_VCTYPE_NONE,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               BP_TIMESLOT_INVALID,
               BP_TIMESLOT_INVALID
            },
         }
      },

      /* Always end device list with this macro. */
      BP_NULL_DEVICE_MACRO_NEW
   },
   /* SLIC Device Profile */
   BP_VD_BUCKBOOST,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_LE9662_ZSI =
{
   VOICECFG_LE9662_ZSI_STR,   /* szBoardId */
   2,   /* Number of FXS Lines */
   0,   /* Number of FXO Lines */
   {
      {
         /* Device Type */
         BP_VD_ZARLINK_9662,
         BP_SPI_SS_NOT_REQUIRED,  /* ZSI SPI CS handled internally. It is mapped using the zsiMapList */
         BP_RESET_FXS1, /* Device uses FXS1 reset pin. Pin on base board depends on base board parameters. */
         /* Channel Description */
         {
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               2,
               2
            },
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               3,
               3
            }
         }
      },

      /* Always end device list with this macro. */
      BP_NULL_DEVICE_MACRO_NEW,
   },
   /* SLIC Device Profile */
   BP_VD_FLYBACK,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_ZSI_SUPPORT | BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_LE9662_ZSI_BB =
{
   VOICECFG_LE9662_ZSI_BB_STR,   /* szBoardId */
   2,   /* Number of FXS Lines */
   0,   /* Number of FXO Lines */
   {
      {
         /* Device Type */
         BP_VD_ZARLINK_9662,
         BP_SPI_SS_NOT_REQUIRED,  /* ZSI SPI CS handled internally. It is mapped using the zsiMapList */
         BP_RESET_FXS1, /* Device uses FXS1 reset pin. Pin on base board depends on base board parameters. */
         /* Channel Description */
         {
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               2,
               2
            },
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               3,
               3
            }
         }
      },

      /* Always end device list with this macro. */
      BP_NULL_DEVICE_MACRO_NEW,
   },
   /* SLIC Device Profile */
   BP_VD_BUCKBOOST,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_ZSI_SUPPORT | BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_LE9642_ZSI_BB =
{
   VOICECFG_LE9642_ZSI_BB_STR,   /* szBoardId */
   2,   /* Number of FXS Lines */
   0,   /* Number of FXO Lines */
   {
      {
         /* Device Type */
         BP_VD_ZARLINK_9642_ZSI,
         BP_SPI_SS_NOT_REQUIRED,  /* ZSI SPI CS handled internally. It is mapped using the zsiMapList */
         BP_RESET_FXS1, /* Device uses FXS1 reset pin. Pin on base board depends on base board parameters. */
         /* Channel Description */
         {
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               2,
               2
            },
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               3,
               3
            }
         }
      },

      /* Always end device list with this macro. */
      BP_NULL_DEVICE_MACRO_NEW,
   },
   /* SLIC Device Profile */
   BP_VD_BUCKBOOST,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_ZSI_SUPPORT | BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_LE89116 =
{
   VOICECFG_LE89116_STR,   /* Daughter Card ID */
   1,   /*Number of FXS Lines */
   0,   /*Number of FXO Lines */
   {
      {
         /* Device Type */
         BP_VD_ZARLINK_89116,
         BP_SPI_SS_B1,  /* Device uses SPI_SS_B1 pin. Pin on base board depends on base board parameters. */
         BP_RESET_FXS1, /* Device uses FXS1 reset pin. Pin on base board depends on base board parameters. */
         /* Channel description */
         {
            /* Channel 0 on device 0 */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               0,
               0
            },
            /* There is no second channel on 89116 so mark it as inactive */
            {
               BP_VOICE_CHANNEL_INACTIVE,
               BP_VCTYPE_NONE,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               BP_TIMESLOT_INVALID,
               BP_TIMESLOT_INVALID},
            }
      },

      /* Always end device list with this macro. */
      BP_NULL_DEVICE_MACRO_NEW,
   },
   /* SLIC Device Profile */
   BP_VD_INVBOOST,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_SI32176 =
{
   VOICECFG_SI32176_STR,   /* Daughter Board ID */
   1,   /* Number of FXS Lines */
   0,   /* Number of FXO Lines */
   {
      {
         /* Device Type */
         BP_VD_SILABS_32176,
         BP_SPI_SS_B1,  /* Device uses SPI_SS_B1 pin. Pin on base board depends on base board parameters. */
         BP_RESET_FXS2, /* Device uses FXS2 reset pin. Pin on base board depends on base board parameters. */
         /* Channel description */
         {
            /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               0,
               0
            },
            /* There is no second channel on 89116 so mark it as inactive */
            {
               BP_VOICE_CHANNEL_INACTIVE,
               BP_VCTYPE_NONE,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               BP_TIMESLOT_INVALID,
               BP_TIMESLOT_INVALID
            },
         }
      },

      /* Always end device list with this macro. */
      BP_NULL_DEVICE_MACRO_NEW,
   },
   /* SLIC Device Profile */
   BP_VD_FLYBACK,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_VE890HV =
{
   VOICECFG_VE890HV_STR,   /*Daughter Card ID */
   2,   /*Number of FXS Lines */
   1,   /*Number of FXO Lines */
   {
      {
         /* Device type */
         BP_VD_ZARLINK_89136,
         BP_SPI_SS_B1,  /* Device uses SPI_SS_B1 pin. Pin on base board depends on base board parameters. */
         BP_RESET_FXS1, /* Device uses FXS1 reset pin. Pin on base board depends on base board parameters. */
         /* Channel description */
         {
            /* Channel 0 on device 0 */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               0,
               0
            },
            /* There is no second channel on 89116 so mark it as inactive */
            {
               BP_VOICE_CHANNEL_INACTIVE,
               BP_VCTYPE_NONE,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               BP_TIMESLOT_INVALID,
               BP_TIMESLOT_INVALID
            },
         }
      },
      {
         /* Device type 2 */
         BP_VD_ZARLINK_89336,
         BP_SPI_SS_B2,  /* Device uses SPI_SS_B2 pin. Pin on base board depends on base board parameters. */
         BP_RESET_FXS2, /* Device uses FXS2 reset pin. Pin on base board depends on base board parameters. */
         /* Channel description */
         {
            /* Channel 0 on device 0 */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               1,
               1
            },
            /* Channel 1 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_DAA,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               2,
               2
            },
         }
      },

      /* Always end the device list with this macro. */
      BP_NULL_DEVICE_MACRO_NEW,
   },
   /* SLIC Device Profile */
   BP_VD_INVBOOST,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_LE89316 =
{
   VOICECFG_LE89316_STR,   /* Daughter Card ID */
   1,   /*Number of FXS Lines */
   1,   /*Number of FXO Lines */
   {
      {
         /* Device Type */
         BP_VD_ZARLINK_89316,
         BP_SPI_SS_B2,  /* Device uses SPI_SS_B2 pin. Pin on base board depends on base board parameters. */
         BP_RESET_FXS2, /* Device uses FXS2 reset pin. Pin on base board depends on base board parameters. */
         /* Channel description */
         {
            /* Channel 0 on device 0 */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               0,
               0
            },
            /* Channel 1 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_DAA,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               1,
               1
            }
         }
      },

      /* Always end device list with this macro. */
      BP_NULL_DEVICE_MACRO_NEW,
   },
   /* SLIC Device Profile */
   BP_VD_INVBOOST,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )

};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_SI32178 =
{
   VOICECFG_SI32178_STR,   /* Daughter Board ID */
   1,   /* Number of FXS Lines */
   1,   /* Number of FXO Lines */
   {
      {
         /* Device Type */
         BP_VD_SILABS_32178,
         BP_SPI_SS_B2,  /* Device uses SPI_SS_B2 pin. Pin on base board depends on base board parameters. */
         BP_RESET_FXS2, /* Device uses FXS2 reset pin. Pin on base board depends on base board parameters. */
         /* Channel description */
         {
            /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               0,
               0
            },
            /* Channel 1 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_DAA,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               1,
               1
            }
         }
      },

      /* Always end device list with this macro. */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
    BP_VD_FLYBACK,
    BP_VOICE_NO_DETECTION,
    /* General-purpose flags */
    ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_NOSLIC =
{
   VOICECFG_NOSLIC_STR, /*Daughter Board ID */
   0,   /*Number of FXS Lines */
   0,   /*Number of FXO Lines */
   {
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_NOT_DEFINED,
   /* General-purpose flags */
   ( 0 )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_SI32267_NTR =
{
   VOICECFG_SI32267_NTR_STR,   /* Daughter Board ID */
   2,   /*Number of FXS Lines */
   0,   /*Number of FXO Lines */
   {
      {
         /* Device Type */
         BP_VD_SILABS_32267,
         BP_SPI_SS_NOT_REQUIRED, /* ISI SPI CS handled internally. It is mapped by the zsiChipMap list. */
         BP_RESET_FXS1,          /* Device uses FXS1 reset pin. Pin on base board depends on base board parameters. */
         /* Channel description */
         {
            /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               0,
               0
            },
            /* Channel 1 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               1,
               1
            },
         }
      },

      /* Always end the device list with this macro. */
      BP_NULL_DEVICE_MACRO_NEW,
   },
   /* SLIC Device Profile */
   BP_VD_FB_TSS_ISO,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_ISI_SUPPORT | BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_SI32260x2_SI3050 =
{
   VOICECFG_SI32260x2_SI3050_STR,   /*Daughter card ID */
   4,   /* Number of FXS Lines */
   1,   /* Number of FXO Lines */
   {
      {
         /* Device Type */
         BP_VD_SILABS_32261,
         BP_SPI_SS_B1,  /* Device uses SPI_SS_B1 pin. Pin on base board depends on base board parameters. */
         BP_RESET_FXS1, /* Device uses FXS1 reset pin. Pin on base board depends on base board parameters. */
         {
            /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               0,
               0
            },
            /* Channel 1 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               1,
               1
            }
         }
      },
      {
         /* Device Type 2 */
         BP_VD_SILABS_32261,
         BP_SPI_SS_B2,  /* Device uses SPI_SS_B2 pin. Pin on base board depends on base board parameters. */
         BP_RESET_FXS2, /* Device uses FXS2 reset pin. Pin on base board depends on base board parameters. */
         {
            /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               2,
               2
            },
            /* Channel 1 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               3,
               3
            },
         }
      },
      {
         /* Device Type 3 */
         BP_VD_SILABS_3050,
         BP_SPI_SS_B3,  /* Device uses SPI_SS_B3 pin. Pin on base board depends on base board parameters. */
         BP_RESET_FXO,  /* Device uses FXO reset pin. Pin on base board depends on base board parameters. */
         {
            /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_DAA,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               4,
               4
            },
            /* There is no second channel on Si3050 so mark it as inactive */
            {
               BP_VOICE_CHANNEL_INACTIVE,
               BP_VCTYPE_NONE,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               BP_TIMESLOT_INVALID,
               BP_TIMESLOT_INVALID
            },
         }
      },

      /* Always end device list with this macro. */
      BP_NULL_DEVICE_MACRO_NEW,
   },
   /* SLIC Device Profile */
   BP_VD_FB_TSS,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_LE9530 =
{
   VOICECFG_LE9530_STR,   /* daughter card ID */
   2,   /* FXS number is 2 */
   0,   /* FXO number is 0 */
   {  /* voiceDevice0 parameters */
      {
         /* Device Type */
         BP_VD_ZARLINK_9530,
         BP_SPI_SS_B1,           /* Device uses SPI_SS_B1 pin. Pin on base board depends on base board parameters. */
         BP_RESET_NOT_REQUIRED,  /* Device does not require a reset pin. */
         {
            /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               0,
               0
            },
            /* Channel 1 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               1,
               1
            }
         }
      },

      /* Always end device list with this macro. */
      BP_NULL_DEVICE_MACRO_NEW,
   },
   /* SLIC Device Profile */
   BP_NOT_DEFINED,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_APMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_LE9530_WB =
{
   VOICECFG_LE9530_WB_STR,	/* daughter card ID */
   2,   /* FXS number is 2 */
   0,   /* FXO number is 0 */
   {   /* voiceDevice0 parameters */
      {
         /* Device Type */
         BP_VD_ZARLINK_9530,
         BP_SPI_SS_B1,           /* Device uses SPI_SS_B1 pin. Pin on base board depends on base board parameters. */
         BP_RESET_NOT_REQUIRED,  /* Device does not require a reset pin. */
         {
            /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_WIDEBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               BP_TIMESLOT_INVALID,
               BP_TIMESLOT_INVALID
            },
            /* Channel 1 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_WIDEBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               BP_TIMESLOT_INVALID,
               BP_TIMESLOT_INVALID
            }
         }
      },

      /* Always end device list with this macro. */
      BP_NULL_DEVICE_MACRO_NEW,
   },
   /* SLIC Device Profile */
   BP_NOT_DEFINED,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_APMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_LE9540 =
{
   VOICECFG_LE9540_STR,   /* daughter card ID */
   2,   /* FXS number is 2 */
   0,   /* FXO number is 0 */
   {  /* voiceDevice0 parameters */
      {
         /* Device Type */
         BP_VD_ZARLINK_9540,
         BP_SPI_SS_B1,           /* Device uses SPI_SS_B1 pin. Pin on base board depends on base board parameters. */
         BP_RESET_NOT_REQUIRED,  /* Device does not require a reset pin. */
         {
            /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               0,
               0
            },
            /* Channel 1 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               1,
               1
            }
         }
      },

      /* Always end device list with this macro. */
      BP_NULL_DEVICE_MACRO_NEW,
   },
   /* SLIC Device Profile */
   BP_NOT_DEFINED,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_APMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_LE9540_WB =
{
   VOICECFG_LE9540_WB_STR,	/* daughter card ID */
   2,   /* FXS number is 2 */
   0,   /* FXO number is 0 */
   {   /* voiceDevice0 parameters */
      {
         /* Device Type */
         BP_VD_ZARLINK_9540,
         BP_SPI_SS_B1,           /* Device uses SPI_SS_B1 pin. Pin on base board depends on base board parameters. */
         BP_RESET_NOT_REQUIRED,  /* Device does not require a reset pin. */
         {
            /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_WIDEBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               BP_TIMESLOT_INVALID,
               BP_TIMESLOT_INVALID
            },
            /* Channel 1 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_WIDEBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               BP_TIMESLOT_INVALID,
               BP_TIMESLOT_INVALID
            }
         }
      },

      /* Always end device list with this macro. */
      BP_NULL_DEVICE_MACRO_NEW,
   },
   /* SLIC Device Profile */
   BP_NOT_DEFINED,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_APMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_LE9541 =
{
   VOICECFG_LE9541_STR,   /* daughter card ID */
   1,   /* FXS number is 1 */
   0,   /* FXO number is 0 */
   {  /* voiceDevice0 parameters */
      {
         /* Device Type */
         BP_VD_ZARLINK_9541,
         BP_SPI_SS_B1,           /* Device uses SPI_SS_B1 pin. Pin on base board depends on base board parameters. */
         BP_RESET_NOT_REQUIRED,  /* Device does not require a reset pin. */
         {
            /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               0,
               0
            },
            /* Channel 1 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               1,
               1
            }
         }
      },

      /* Always end device list with this macro. */
      BP_NULL_DEVICE_MACRO_NEW,
   },
   /* SLIC Device Profile */
   BP_NOT_DEFINED,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_APMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_LE9541_WB =
{
   VOICECFG_LE9541_WB_STR,	/* daughter card ID */
   1,   /* FXS number is 1 */
   0,   /* FXO number is 0 */
   {   /* voiceDevice0 parameters */
      {
         /* Device Type */
         BP_VD_ZARLINK_9541,
         BP_SPI_SS_B1,           /* Device uses SPI_SS_B1 pin. Pin on base board depends on base board parameters. */
         BP_RESET_NOT_REQUIRED,  /* Device does not require a reset pin. */
         {
            /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_WIDEBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               BP_TIMESLOT_INVALID,
               BP_TIMESLOT_INVALID
            },
            /* Channel 1 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_WIDEBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               BP_TIMESLOT_INVALID,
               BP_TIMESLOT_INVALID
            }
         }
      },

      /* Always end device list with this macro. */
      BP_NULL_DEVICE_MACRO_NEW,
   },
   /* SLIC Device Profile */
   BP_NOT_DEFINED,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_APMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_SI3239 =
{
   VOICECFG_SI3239_STR,   /* daughter card ID */
   2,   /* FXS number is 2 */
   0,   /* FXO number is 0 */
   {   /* voiceDevice0 parameters */
      {
         /* Device type */
         BP_VD_SILABS_3239,
         BP_SPI_SS_B1,           /* Device uses SPI_SS_B1 pin. Pin on base board depends on base board parameters. */
         BP_RESET_NOT_REQUIRED,  /* Device does not require a reset pin. */
         {
            /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               0,
               0
            },
            /* Channel 1 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               1,
               1
            }
         }
      },

      /* Always end device list with this macro. */
      BP_NULL_DEVICE_MACRO_NEW,
   },
   /* SLIC Device Profile */
   BP_NOT_DEFINED,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_APMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_SI32392 =
{
   VOICECFG_SI32392_STR,   /* daughter card ID */
   2,   /* FXS number is 2 */
   0,   /* FXO number is 0 */
   {   /* voiceDevice0 parameters */
      {
         /* Device type */
         BP_VD_SILABS_32392,
         BP_SPI_SS_NOT_REQUIRED,  /* Device doesn't use SPI_SS_Bx pin. Hard coded in board HAL. */
         BP_RESET_NOT_REQUIRED,  /* Device does not require a reset pin. */
         {
            /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               0,
               0
            },
            /* Channel 1 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               1,
               1
            }
         }
      },

      /* Always end device list with this macro. */
      BP_NULL_DEVICE_MACRO_NEW,
   },
   /* SLIC Device Profile */
   BP_NOT_DEFINED,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_APMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_LE9530_LE88506 = {
   VOICECFG_LE9530_LE88506_STR,   /* Daughter board ID*/
   4,   /*Num FXS Lines */
   0,   /*Num FXO Lines */
   {   /*voiceDevice0 Params */
      {
         /* Device Type */
         BP_VD_ZARLINK_88506,
         BP_SPI_SS_B1,  /* Device uses SPI_SS_B1 pin. Pin on base board depends on base board parameters. */
         BP_RESET_FXS1, /* Device uses FXS1 reset pin. Pin on base board depends on base board parameters. */
         /* Channel description */
            {
               /* Channel 0 on device */
               {
                  BP_VOICE_CHANNEL_ACTIVE,
                  BP_VCTYPE_SLIC,
                  BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
                  BP_VOICE_CHANNEL_NARROWBAND,
                  BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                  BP_VOICE_CHANNEL_ENDIAN_BIG,
                  0,
                  0
               },
               /* Test a single channel on 88506 */
               {
                  BP_VOICE_CHANNEL_ACTIVE,
                  BP_VCTYPE_SLIC,
                  BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
                  BP_VOICE_CHANNEL_NARROWBAND,
                  BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                  BP_VOICE_CHANNEL_ENDIAN_BIG,
                  1,
                  1
               }
         }
      },
      {
         /* Device Type 2 */
         BP_VD_ZARLINK_9530,
         BP_SPI_SS_B1,           /*The 9530 chips are actually internal, device ID is always 0. */
         BP_RESET_NOT_REQUIRED,  /* Device does not require a reset pin. */
         /* Channel description */
            {
               /* Channel 0 on device */
               {
                  BP_VOICE_CHANNEL_ACTIVE,
                  BP_VCTYPE_SLIC,
                  BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
                  BP_VOICE_CHANNEL_NARROWBAND,
                  BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                  BP_VOICE_CHANNEL_ENDIAN_BIG,
                  BP_TIMESLOT_INVALID,
                  BP_TIMESLOT_INVALID
               },
               /* Channel 1 on device */
               {
                  BP_VOICE_CHANNEL_ACTIVE,
                  BP_VCTYPE_SLIC,
                  BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
                  BP_VOICE_CHANNEL_NARROWBAND,
                  BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                  BP_VOICE_CHANNEL_ENDIAN_BIG,
                  BP_TIMESLOT_INVALID,
                  BP_TIMESLOT_INVALID
               }
            }
      },

     /* Always end device list with this macro. */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_VD_FLYBACK,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_APMHAL_ENABLE )
};


VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_vanth =
{
    VOICECFG_TCH_VANTH_STR,   /*Daughter Card ID */
    2,             /* numFxsLines */
    0,             /* numFxoLines */
    {
      /* voiceDevice0 parameters */
      {
          /* Device type */
          BP_VD_SILABS_32261,
          BP_SPI_SS_B1,
          BP_RESET_FXS1,
          /* Channel description */
          {
              /* Channel 0 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 0,
                 0,
              },
              /* Channel 1 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 1,
                 1,
              }
          }
      },
       /* Always end the device list with BP_NULL_DEVICE_MACRO */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_VD_QCUK_TCH_EXT_ATTN,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_vantw =
{
    VOICECFG_TCH_VANTW_STR,   /*Daughter Card ID */
    2,             /* numFxsLines */
    0,             /* numFxoLines */
    {
      /* voiceDevice0 parameters */
      {
          /* Device type */
          BP_VD_SILABS_32261,
          BP_SPI_SS_B1,
          BP_RESET_FXS1,
          /* Channel description */
          {
              /* Channel 0 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 0,
                 0,
              },
              /* Channel 1 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 1,
                 1,
              }
          }
      },
       /* Always end the device list with BP_NULL_DEVICE_MACRO */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_VD_QCUK_TCH_EXT_ATTN,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_vantv =
{
    VOICECFG_TCH_VANTV_STR,   /*Daughter Card ID */
    2,             /* numFxsLines */
    0,             /* numFxoLines */
    {
      /* voiceDevice0 parameters */
      {
          /* Device type */
          BP_VD_SILABS_32261,
          BP_SPI_SS_B1,
          BP_RESET_FXS1,
          /* Channel description */
          {
              /* Channel 0 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 0,
                 0,
              },
              /* Channel 1 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 1,
                 1,
              }
          }
      },
       /* Always end the device list with BP_NULL_DEVICE_MACRO */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_VD_QCUK_TCH_EXT_ATTN,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_vbntf =
{
    VOICECFG_TCH_VBNTF_STR,   /*Daughter Card ID */
    2,             /* numFxsLines */
    0,             /* numFxoLines */
    {
      /* voiceDevice0 parameters */
      {
          /* Device type */
          BP_VD_SILABS_32261,
          BP_SPI_SS_B1,
          BP_RESET_FXS1,
          /* Channel description */
          {
              /* Channel 0 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 0,
                 0,
              },
              /* Channel 1 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 1,
                 1,
              }
          }
      },
       /* Always end the device list with BP_NULL_DEVICE_MACRO */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_VD_QCUK_TCH_EXT_ATTN,
   /* General-purpose flags */
   BP_VOICE_NO_DETECTION,
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_vbnth =
{
    VOICECFG_TCH_VBNTH_STR,   /*Daughter Card ID */
    2,             /* numFxsLines */
    0,             /* numFxoLines */
    {
      /* voiceDevice0 parameters */
      {
          /* Device type */
          BP_VD_SILABS_32261,
          BP_SPI_SS_B1,
          BP_RESET_FXS1,
          /* Channel description */
          {
              /* Channel 0 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 0,
                 0,
              },
              /* Channel 1 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 1,
                 1,
              }
          }
      },
       /* Always end the device list with BP_NULL_DEVICE_MACRO */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_VD_QCUK_TCH_EXT_ATTN,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_vbntk =
{
    VOICECFG_TCH_VBNTK_STR,   /*Daughter Card ID */
    2,             /* numFxsLines */
    0,             /* numFxoLines */
    {
      /* voiceDevice0 parameters */
      {
          /* Device type */
          BP_VD_SILABS_32261,
          BP_SPI_SS_B1,
          BP_RESET_FXS1,
          /* Channel description */
          {
              /* Channel 0 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 0,
                 0,
              },
              /* Channel 1 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 1,
                 1,
              }
          }
      },
       /* Always end the device list with BP_NULL_DEVICE_MACRO */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_VD_QCUK_TCH_EXT_ATTN,
   /* General-purpose flags */
   BP_VOICE_NO_DETECTION,
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

BP_VOICE_CHANNEL g_iDectStdCfg[BP_MAX_CHANNELS_PER_DEVICE*BP_MAX_DECT_DEVICE+1] = {						
   { 														
      BP_VOICE_CHANNEL_ACTIVE,					
      BP_VCTYPE_DECT,								
      BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,		
      BP_VOICE_CHANNEL_WIDEBAND,					
      BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,	
      BP_VOICE_CHANNEL_ENDIAN_BIG,				
      BP_TIMESLOT_INVALID,							
      BP_TIMESLOT_INVALID							
   },														
   {  													
      BP_VOICE_CHANNEL_ACTIVE,					
      BP_VCTYPE_DECT,								
      BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,		
      BP_VOICE_CHANNEL_WIDEBAND,					
      BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,	
      BP_VOICE_CHANNEL_ENDIAN_BIG,				
      BP_TIMESLOT_INVALID,							
      BP_TIMESLOT_INVALID							
   },														
   { 														
      BP_VOICE_CHANNEL_ACTIVE,					
      BP_VCTYPE_DECT,								
      BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,		
      BP_VOICE_CHANNEL_WIDEBAND,					
      BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,	
      BP_VOICE_CHANNEL_ENDIAN_BIG,				
      BP_TIMESLOT_INVALID,							
      BP_TIMESLOT_INVALID							
   },														
   {  													
      BP_VOICE_CHANNEL_ACTIVE,					
      BP_VCTYPE_DECT,								
      BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,		
      BP_VOICE_CHANNEL_WIDEBAND,					
      BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,	
      BP_VOICE_CHANNEL_ENDIAN_BIG,				
      BP_TIMESLOT_INVALID,							
      BP_TIMESLOT_INVALID							
   },														
			   											
   BP_NULL_CHANNEL_DESCRIPTION_MACRO			
};


/* Technicolor added voice daughter boards */

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_dantu =
{
    VOICECFG_TCH_DANTU_STR,   /*Daughter Card ID */
    2,             /* numFxsLines */
    0,             /* numFxoLines */
    {
      /* voiceDevice0 parameters */
      {
          /* Device type */
          BP_VD_SILABS_32176,
          BP_SPI_SS_B1,
          BP_RESET_FXS1,
         /* Channel description */
         {
             /* Channel 0 on device */
             {  BP_VOICE_CHANNEL_ACTIVE,
                BP_VCTYPE_SLIC,
                BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                BP_VOICE_CHANNEL_NARROWBAND,
                BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                BP_VOICE_CHANNEL_ENDIAN_BIG,
                0,
                0,
             },
             BP_NULL_CHANNEL_DESCRIPTION_MACRO,
         },
      },
      {
          /* Device type */
          BP_VD_SILABS_32176,
          BP_SPI_SS_B2,
          BP_RESET_NOT_REQUIRED,

          /* Channel description */
          {
              /* Channel 0 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 1,
                 1,
              },
              /* There is no second channel on 89116 so mark it as inactive */
              BP_NULL_CHANNEL_DESCRIPTION_MACRO,
          }
      },
       /* Always end the device list with BP_NULL_DEVICE_MACRO */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_VD_BUCKBOOST,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_danto =
{
    VOICECFG_TCH_DANTO_STR,   /*Daughter Card ID */
    1,             /* numFxsLines */
    1,             /* numFxoLines */
    {
      /* voiceDevice0 parameters */
      {
          /* Device type */
          BP_VD_SILABS_32178,
          BP_SPI_SS_B1,
          BP_RESET_FXS1,
         /* Channel description */
         {
             /* Channel 0 on device */
             {  BP_VOICE_CHANNEL_ACTIVE,
                BP_VCTYPE_SLIC,
                BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                BP_VOICE_CHANNEL_NARROWBAND,
                BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                BP_VOICE_CHANNEL_ENDIAN_BIG,
                0,
                0,
             },
             /* Channel 1 on device */
             {  BP_VOICE_CHANNEL_ACTIVE,
                BP_VCTYPE_DAA,
                BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                BP_VOICE_CHANNEL_NARROWBAND,
                BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                BP_VOICE_CHANNEL_ENDIAN_BIG,
                1,
                1,
             },
         },
      },
       /* Always end the device list with BP_NULL_DEVICE_MACRO */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_VD_BUCKBOOST,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_vdntw =
{
    VOICECFG_TCH_VDNTW_STR,   /*Daughter Card ID */
    2,             /* numFxsLines */
    0,             /* numFxoLines */
    {
      /* voiceDevice0 parameters */
      {
          /* Device type */
          BP_VD_SILABS_32261,
          BP_SPI_SS_B1,
          BP_RESET_FXS1,
          /* Channel description */
          {
              /* Channel 0 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 0,
                 0,
              },
              /* Channel 1 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 1,
                 1,
              }
          }
      },
       /* Always end the device list with BP_NULL_DEVICE_MACRO */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_VD_PMOS_BUCK_BOOST,
   /* General-purpose flags */
   BP_VOICE_NO_DETECTION,
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};


VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_vdnto =
{
    VOICECFG_TCH_VDNTO_STR,   /*Daughter Card ID */
    2,             /* numFxsLines */
    0,             /* numFxoLines */
    {
      /* voiceDevice0 parameters */
      {
          /* Device type */
          BP_VD_SILABS_32176,
          BP_SPI_SS_B2,
          BP_RESET_FXS1,
          
          /* Channel description */
          {
              /* Channel 0 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 0,
                 0,
              },
              /* There is no second channel on 89116 so mark it as inactive */
              BP_NULL_CHANNEL_DESCRIPTION_MACRO
          }
      },
      {
          /* Device type */
          BP_VD_SILABS_32178,
          BP_SPI_SS_B3,
          BP_RESET_FXS2,
         /* Channel description */
         {
             /* Channel 0 on device */
             {  BP_VOICE_CHANNEL_ACTIVE,
                BP_VCTYPE_SLIC,
                BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                BP_VOICE_CHANNEL_NARROWBAND,
                BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                BP_VOICE_CHANNEL_ENDIAN_BIG,
                1,
                1,
             },
             /* Channel 1 on device */
             {  BP_VOICE_CHANNEL_ACTIVE,
                BP_VCTYPE_DAA,
                BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                BP_VOICE_CHANNEL_NARROWBAND,
                BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                BP_VOICE_CHANNEL_ENDIAN_BIG,
                2,
                2,
            },
         },
      },
       /* Always end the device list with BP_NULL_DEVICE_MACRO */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_VD_FLYBACK,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_vdnt4 =
{
    VOICECFG_TCH_VDNT4_STR,   /*Daughter Card ID */
    2,             /* numFxsLines */
    0,             /* numFxoLines */
    {
      /* voiceDevice0 parameters */
      {
          /* Device type */
          BP_VD_SILABS_32261,
          BP_SPI_SS_B1,
          BP_RESET_FXS1,

          /* Channel description */
          {
              /* Channel 0 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 0,
                 0,
              },
              /* Channel 1 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 1,
                 1,
              }
          }
      },
       /* Always end the device list with BP_NULL_DEVICE_MACRO */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_VD_PMOS_BUCK_BOOST,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_vdnt6 =
{
    VOICECFG_TCH_VDNT6_STR,   /*Daughter Card ID */
    2,             /* numFxsLines */
    0,             /* numFxoLines */
    {
      /* voiceDevice0 parameters */
      {
          /* Device type */
          BP_VD_SILABS_32176,
          BP_SPI_SS_B2,
          BP_RESET_FXS1,
          
          /* Channel description */
          {
              /* Channel 0 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 0,
                 0,
              },
              /* There is no second channel on 89116 so mark it as inactive */
              BP_NULL_CHANNEL_DESCRIPTION_MACRO
          }
      },
      {
          /* Device type */
          BP_VD_SILABS_32178,
          BP_SPI_SS_B3,
          BP_RESET_FXS2,
         /* Channel description */
         {
             /* Channel 0 on device */
             {  BP_VOICE_CHANNEL_ACTIVE,
                BP_VCTYPE_SLIC,
                BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                BP_VOICE_CHANNEL_NARROWBAND,
                BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                BP_VOICE_CHANNEL_ENDIAN_BIG,
                1,
                1,
             },
             /* Channel 1 on device */
             {  BP_VOICE_CHANNEL_ACTIVE,
                BP_VCTYPE_DAA,
                BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                BP_VOICE_CHANNEL_NARROWBAND,
                BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                BP_VOICE_CHANNEL_ENDIAN_BIG,
                2,
                2,
            },
         },
      },
       /* Always end the device list with BP_NULL_DEVICE_MACRO */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_VD_FLYBACK,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_vdnt8 =
{
    VOICECFG_TCH_VDNT8_STR,   /*Daughter Card ID */
    2,             /* numFxsLines */
    0,             /* numFxoLines */
    {
      /* voiceDevice0 parameters */
      {
          /* Device type */
          BP_VD_SILABS_32176,
          BP_SPI_SS_B2,
          BP_RESET_FXS1,
          
          /* Channel description */
          {
              /* Channel 0 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 0,
                 0,
              },
              /* There is no second channel on 89116 so mark it as inactive */
              BP_NULL_CHANNEL_DESCRIPTION_MACRO
          }
      },
      {
          /* Device type */
          BP_VD_SILABS_32178,
          BP_SPI_SS_B3,
          BP_RESET_FXS2,
         /* Channel description */
         {
             /* Channel 0 on device */
             {  BP_VOICE_CHANNEL_ACTIVE,
                BP_VCTYPE_SLIC,
                BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                BP_VOICE_CHANNEL_NARROWBAND,
                BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                BP_VOICE_CHANNEL_ENDIAN_BIG,
                1,
                1,
             },
             /* Channel 1 on device */
             {  BP_VOICE_CHANNEL_ACTIVE,
                BP_VCTYPE_DAA,
                BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                BP_VOICE_CHANNEL_NARROWBAND,
                BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                BP_VOICE_CHANNEL_ENDIAN_BIG,
                2,
                2,
            },
         },
      },
       /* Always end the device list with BP_NULL_DEVICE_MACRO */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_VD_FLYBACK,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_vantf =
{
    VOICECFG_TCH_VANTF_STR,   /*Daughter Card ID */
    2,             /* numFxsLines */
    1,             /* numFxoLines */
    {
      /* voiceDevice0 parameters */
      {
          /* Device type */
          BP_VD_SILABS_32261,
          BP_SPI_SS_B2,
          BP_RESET_FXS1,
          
          /* Channel description */
          {
              /* Channel 0 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 0,
                 0,
              },
              /* Channel 1 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 1,
                 1,
              }
          }
      },
      {
          /* Device type */
          BP_VD_SILABS_3050,
          BP_SPI_SS_B3,
          BP_RESET_FXO,
         /* Channel description */
         {
             /* Channel 0 on device */
             {  BP_VOICE_CHANNEL_ACTIVE,
                BP_VCTYPE_DAA,
                BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                BP_VOICE_CHANNEL_NARROWBAND,
                BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                BP_VOICE_CHANNEL_ENDIAN_BIG,
                2,
                2,
             },
             /* There is no second channel on 3050 so mark it as inactive */
              BP_NULL_CHANNEL_DESCRIPTION_MACRO
         },
      },
       /* Always end the device list with BP_NULL_DEVICE_MACRO */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_VD_PMOS_BUCK_BOOST,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_vantg =
{
    VOICECFG_TCH_VANTG_STR,   /*Daughter Card ID */
    2,             /* numFxsLines */
    0,             /* numFxoLines */
    {
      /* voiceDevice0 parameters */
      {
          /* Device type */
          BP_VD_SILABS_32261,
          BP_SPI_SS_B1,
          BP_RESET_FXS1,
          /* Channel description */
          {
              /* Channel 0 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 0,
                 0,
              },
              /* Channel 1 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 1,
                 1,
              }
          }
      },
       /* Always end the device list with BP_NULL_DEVICE_MACRO */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_VD_PMOS_BUCK_BOOST,
   BP_VOICE_NEEDS_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_vant9 =
{
    VOICECFG_TCH_VANT9_STR,   /*Daughter Card ID */
    2,             /* numFxsLines */
    0,             /* numFxoLines */
    {
      /* voiceDevice0 parameters */
      {
          /* Device type */
          BP_VD_SILABS_32261,
          BP_SPI_SS_B1,
          BP_RESET_FXS1,
          /* Channel description */
          {
              /* Channel 0 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 0,
                 0,
              },
              /* Channel 1 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 1,
                 1,
              }
          }
      },
       /* Always end the device list with BP_NULL_DEVICE_MACRO */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_VD_QCUK_TCH_NO_EXT_ATTN,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_vant5 =
{
    VOICECFG_TCH_VANT5_STR,   /*Daughter Card ID */
    1,             /* numFxsLines */
    1,             /* numFxoLines */
    {
      /* voiceDevice0 parameters */
      {
          /* Device type */
          BP_VD_SILABS_32261,
          BP_SPI_SS_B2,
          BP_RESET_FXS1,

          /* Channel description */
          {
              /* Channel 0 on device */
              {  BP_VOICE_CHANNEL_INACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 0,
                 0,
              },
              /* Channel 1 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 1,
                 1,
              }
          }
      },
      {
          /* Device type */
          BP_VD_SILABS_3050,
          BP_SPI_SS_B3,
          BP_RESET_FXO,
         /* Channel description */
         {
             /* Channel 0 on device */
             {  BP_VOICE_CHANNEL_ACTIVE,
                BP_VCTYPE_DAA,
                BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                BP_VOICE_CHANNEL_NARROWBAND,
                BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                BP_VOICE_CHANNEL_ENDIAN_BIG,
                2,
                2,
             },
             /* There is no second channel on 3050 so mark it as inactive */
              BP_NULL_CHANNEL_DESCRIPTION_MACRO
         },
      },
       /* Always end the device list with BP_NULL_DEVICE_MACRO */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_VD_PMOS_BUCK_BOOST,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_vant6 =
{
    VOICECFG_TCH_VANT6_STR,   /*Daughter Card ID */
    2,             /* numFxsLines */
    0,             /* numFxoLines */
    {
      /* voiceDevice0 parameters */
      {
          /* Device type */
          BP_VD_SILABS_32261,
          BP_SPI_SS_B2,
          BP_RESET_FXS1,

          /* Channel description */
          {
              /* Channel 0 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 0,
                 0,
              },
              /* Channel 1 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 1,
                 1,
              }
          }
      },
       /* Always end the device list with BP_NULL_DEVICE_MACRO */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_VD_QCUK_TCH_NO_EXT_ATTN,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_vant7 =
{
    VOICECFG_TCH_VANT7_STR,   /*Daughter Card ID */
    2,             /* numFxsLines */
    0,             /* numFxoLines */
    {
      /* voiceDevice0 parameters */
      {
          /* Device type */
          BP_VD_SILABS_32261,
          BP_SPI_SS_B2,
          BP_RESET_FXS1,

          /* Channel description */
          {
              /* Channel 0 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 0,
                 0,
              },
              /* Channel 1 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 1,
                 1,
              }
          }
      },
       /* Always end the device list with BP_NULL_DEVICE_MACRO */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_VD_QCUK_TCH_NO_EXT_ATTN,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_vantr =
{
    VOICECFG_TCH_VANTR_STR,   /*Daughter Card ID */
    2,             /* numFxsLines */
    0,             /* numFxoLines */
    {
      /* voiceDevice0 parameters */
      {
          /* Device type */
          BP_VD_SILABS_32261,
          BP_SPI_SS_B2,
          BP_RESET_FXS1,
          
          /* Channel description */
          {
              /* Channel 0 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 0,
                 0,
              },
              /* Channel 1 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 1,
                 1,
              }
          }
      },
       /* Always end the device list with BP_NULL_DEVICE_MACRO */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_VD_PMOS_BUCK_BOOST,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_gbntg =
{
    VOICECFG_TCH_GBNTG_STR,   /*Daughter Card ID */
    2,             /* numFxsLines */
    0,             /* numFxoLines */
    {
      /* voiceDevice0 parameters */
      {
          /* Device type */
          BP_VD_SILABS_32261,
          BP_SPI_SS_B2,
          BP_RESET_FXS1,

          /* Channel description */
          {
              /* Channel 0 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 0,
                 0,
              },
              /* Channel 1 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 1,
                 1,
              }
          }
      },
       /* Always end the device list with BP_NULL_DEVICE_MACRO */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_VD_PMOS_BUCK_BOOST,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_vantd =
{
    VOICECFG_TCH_VANTD_STR,   /*Daughter Card ID */
    2,             /* numFxsLines */
    0,             /* numFxoLines */
    {
      /* voiceDevice0 parameters */
      {
          /* Device type */
          BP_VD_SILABS_32261,
          BP_SPI_SS_B2,
          BP_RESET_FXS1,
          
          /* Channel description */
          {
              /* Channel 0 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 0,
                 0,
              },
              /* Channel 1 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 1,
                 1,
              }
          }
      },
       /* Always end the device list with BP_NULL_DEVICE_MACRO */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_VD_PMOS_BUCK_BOOST,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_vant4 =
{
    VOICECFG_TCH_VANT4_STR,   /*Daughter Card ID */
    2,             /* numFxsLines */
    0,             /* numFxoLines */
    {
      /* voiceDevice0 parameters */
      {
          /* Device type */
          BP_VD_SILABS_32261,
          BP_SPI_SS_B2,
          BP_RESET_FXS1,
          
          /* Channel description */
          {
              /* Channel 0 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 0,
                 0,
              },
              /* Channel 1 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 1,
                 1,
              }
          }
      },
       /* Always end the device list with BP_NULL_DEVICE_MACRO */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_VD_PMOS_BUCK_BOOST,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_vbnta =
{
    VOICECFG_TCH_VBNTA_STR,   /*Daughter Card ID */
    1,             /* numFxsLines */
    1,             /* numFxoLines */
    {
      /* voiceDevice0 parameters */
      {
          /* Device type */
          BP_VD_SILABS_32261,
          BP_SPI_SS_B2,
          BP_RESET_FXS1,

          /* Channel description */
          {
              /* Channel 0 on device */
              {  BP_VOICE_CHANNEL_INACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 0,
                 0,
              },
              /* Channel 1 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 1,
                 1,
              }
          }
      },
      {
          /* Device type */
          BP_VD_SILABS_3050,
          BP_SPI_SS_B3,
          BP_RESET_FXO,
         /* Channel description */
         {
             /* Channel 0 on device */
             {  BP_VOICE_CHANNEL_ACTIVE,
                BP_VCTYPE_DAA,
                BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                BP_VOICE_CHANNEL_NARROWBAND,
                BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                BP_VOICE_CHANNEL_ENDIAN_BIG,
                2,
                2,
             },
             /* There is no second channel on 3050 so mark it as inactive */
              BP_NULL_CHANNEL_DESCRIPTION_MACRO
         },
      },
       /* Always end the device list with BP_NULL_DEVICE_MACRO */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_VD_PMOS_BUCK_BOOST,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_vbntb =
{
    VOICECFG_TCH_VBNTB_STR,   /*Daughter Card ID */
    1,             /* numFxsLines */
    1,             /* numFxoLines */
    {
      /* voiceDevice0 parameters */
      {
          /* Device type */
          BP_VD_SILABS_32178,
          BP_SPI_SS_B2,
          BP_RESET_FXS1,
         /* Channel description */
         {
             /* Channel 0 on device */
             {  BP_VOICE_CHANNEL_ACTIVE,
                BP_VCTYPE_SLIC,
                BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                BP_VOICE_CHANNEL_NARROWBAND,
                BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                BP_VOICE_CHANNEL_ENDIAN_BIG,
                0,
                0,
             },
             /* Channel 1 on device */
             {  BP_VOICE_CHANNEL_ACTIVE,
                BP_VCTYPE_DAA,
                BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                BP_VOICE_CHANNEL_NARROWBAND,
                BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                BP_VOICE_CHANNEL_ENDIAN_BIG,
                1,
                1,
             },
         },
      },
       /* Always end the device list with BP_NULL_DEVICE_MACRO */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   /*BP_VD_LCQCUK,*/       /*LCQCUK DC-DC convert mode need patch for si32179 */
   BP_VD_FLYBACK,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_vbntl =
{
    VOICECFG_TCH_VBNTL_STR,   /*Daughter Card ID */
    2,             /* numFxsLines */
    0,             /* numFxoLines */
    {
      /* voiceDevice0 parameters */
      {
          /* Device type */
          BP_VD_SILABS_32261,
          BP_SPI_SS_B2,
          BP_RESET_FXS1,

          /* Channel description */
          {
              /* Channel 0 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 0,
                 0,
              },
              /* Channel 1 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 1,
                 1,
              }
          }
      },
       /* Always end the device list with BP_NULL_DEVICE_MACRO */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_VD_QCUK_TCH_EXT_ATTN,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_gant1 =
{
    VOICECFG_TCH_GANT1_STR,   /*Daughter Card ID */
    2,             /* numFxsLines */
    0,             /* numFxoLines */
    {
      /* voiceDevice0 parameters */
      {
          /* Device type */
          BP_VD_SILABS_32261,
          BP_SPI_SS_B2,
          BP_RESET_FXS1,

          /* Channel description */
          {
              /* Channel 0 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 0,
                 0,
              },
              /* Channel 1 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 1,
                 1,
              }
          }
      },
       /* Always end the device list with BP_NULL_DEVICE_MACRO */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_VD_QCUK,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};
VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_gant2 =
{
    VOICECFG_TCH_GANT2_STR,   /*Daughter Card ID */
    2,             /* numFxsLines */
    0,             /* numFxoLines */
    {
      /* voiceDevice0 parameters */
      {
          /* Device type */
          BP_VD_SILABS_32261,
          BP_SPI_SS_B2,
          BP_RESET_FXS1,

          /* Channel description */
          {
              /* Channel 0 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 0,
                 0,
              },
              /* Channel 1 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 1,
                 1,
              }
          }
      },
      /* Always end the device list with BP_NULL_DEVICE_MACRO */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_VD_QCUK,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

/** TECHNICOLOR START [MAM] Adding new profile for 63148 NAND boards **/
VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_GANT6 =
{
    VOICECFG_TCH_GANT6_STR, /*Daughter Card ID*/
    2, /*Number of FXS Lines*/
    0, /*Number of FXO Lines*/
    /* voiceDevice0 Parameters */
    {
        {
            /* Device Type */
            BP_VD_SILABS_32261,
            BP_SPI_SS_B1, /* Device uses SPI_SS_B1 pin. Pin on base board depends on base board parameters. */
            BP_RESET_FXS1, /* Device uses FXS1 reset pin. Pin on base board depends on base board parameters. */
            /* Channel description */
            {
                /* Channel 0 on device */
                {
                    BP_VOICE_CHANNEL_ACTIVE,
                    BP_VCTYPE_SLIC,
                    BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
                    BP_VOICE_CHANNEL_NARROWBAND,
                    BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                    BP_VOICE_CHANNEL_ENDIAN_BIG,
                    0,
                    0
                },
                /* Channel 1 on device */
                {
                    BP_VOICE_CHANNEL_ACTIVE,
                    BP_VCTYPE_SLIC,
                    BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
                    BP_VOICE_CHANNEL_NARROWBAND,
                    BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                    BP_VOICE_CHANNEL_ENDIAN_BIG,
                    1,
                    1
                }
            }
        },
        /* Always end device list with this macro. */
        BP_NULL_DEVICE_MACRO_NEW,
    },
    /* SLIC Device Profile */
    BP_VD_QCUK,
    BP_VOICE_NO_DETECTION,
    /* General-purpose flags */
    ( 0 )
};
/** TECHNICOLOR END   [MAM] Adding new profile for 63148 NAND boards **/


// Technicolor: 12364_02792_JCC: START - Add C2100 PEM2
VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_vantt =
{
   VOICECFG_TCH_VANTT_STR,   /* Daughter Card ID */
   2,             /* numFxsLines */
   0,             /* numFxoLines */
   {
      /* voiceDevice0 parameters */
      {
         /* Device type */
         BP_VD_SILABS_32261,
         BP_SPI_SS_B2,
         BP_RESET_FXS1,

         /* Channel description */
         {
            /* Channel 0 on device 0 */
            {  BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               0,
               0
            },
            /* Channel 1 on device */
            {  BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               1,
               1
            },
         }
      },

      /* Always end the device list with BP_NULL_DEVICE_MACRO */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_VD_PMOS_BUCK_BOOST,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )

};
// Technicolor: 12364_02792_JCC: END   - Add C2100 PEM2

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_vantz =
{
    VOICECFG_TCH_VANTZ_STR,   /*Daughter Card ID */
    2,             /* numFxsLines */
    0,             /* numFxoLines */
    {
      /* voiceDevice0 parameters */
      {
          /* Device type */
          BP_VD_SILABS_32261,
          BP_SPI_SS_B1,
          BP_RESET_FXS1,

          /* Channel description */
          {
              /* Channel 0 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 0,
                 0,
              },
              /* Channel 1 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 1,
                 1,
              }
          }
      },
       /* Always end the device list with BP_NULL_DEVICE_MACRO */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_VD_QCUK,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_vant1 =
{
    VOICECFG_TCH_VANT1_STR,   /*Daughter Card ID */
    2,             /* numFxsLines */
    0,             /* numFxoLines */
    {
      /* voiceDevice0 parameters */
      {
          /* Device type */
          BP_VD_SILABS_32261,
          BP_SPI_SS_B1,
          BP_RESET_FXS1,

          /* Channel description */
          {
              /* Channel 0 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 0,
                 0,
              },
              /* Channel 1 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 1,
                 1,
              }
          }
      },
       /* Always end the device list with BP_NULL_DEVICE_MACRO */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_VD_QCUK,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_vdnty =
{
    VOICECFG_TCH_VDNTY_STR,   /*Daughter Card ID */
    2,             /* numFxsLines */
    0,             /* numFxoLines */
    {
      /* voiceDevice0 parameters */
      {
          /* Device type */
          BP_VD_SILABS_32261,
          BP_SPI_SS_B1,
          BP_RESET_FXS1,
          /* Channel description */
          {
              /* Channel 0 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 0,
                 0,
              },
              /* Channel 1 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 1,
                 1,
              }
          }
      },
       /* Always end the device list with BP_NULL_DEVICE_MACRO */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_VD_PMOS_BUCK_BOOST,
   BP_VOICE_NEEDS_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_gantk =
{
    VOICECFG_TCH_GANTK_STR,   /*Daughter Card ID */
    2,             /* numFxsLines */
    0,             /* numFxoLines */
    {
      /* voiceDevice0 parameters */
      {
          /* Device type */
          BP_VD_SILABS_32176,
          BP_SPI_SS_B2,
          BP_RESET_FXS1,

          /* Channel description */
          {
              /* Channel 0 on device */
              {  BP_VOICE_CHANNEL_ACTIVE,
                 BP_VCTYPE_SLIC,
                 BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                 BP_VOICE_CHANNEL_NARROWBAND,
                 BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                 BP_VOICE_CHANNEL_ENDIAN_BIG,
                 0,
                 0,
              },
              BP_NULL_CHANNEL_DESCRIPTION_MACRO
          }
      },
      {
          /* Device type */
          BP_VD_SILABS_32176,
          BP_SPI_SS_B3,
          BP_RESET_FXS2,
         /* Channel description */
         {
             /* Channel 0 on device */
             {  BP_VOICE_CHANNEL_ACTIVE,
                BP_VCTYPE_SLIC,
                BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
                BP_VOICE_CHANNEL_NARROWBAND,
                BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
                BP_VOICE_CHANNEL_ENDIAN_BIG,
                1,
                1,
             },
             BP_NULL_CHANNEL_DESCRIPTION_MACRO
         },
      },
       /* Always end the device list with BP_NULL_DEVICE_MACRO */
      BP_NULL_DEVICE_MACRO_NEW,
   },

   /* SLIC Device Profile */
   BP_VD_FLYBACK,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_vbnti =
{
   VOICECFG_TCH_VBNTI_STR,   /*Daughter Card ID */
   1,   /* Number of FXS Lines */
   0,   /* Number of FXO Lines */
   {
      {
         /* Device Type */
         BP_VD_SILABS_32176,
         BP_SPI_SS_B1,  /* Device uses SPI_SS_B1 pin. Pin on base board depends on base board parameters. */
         BP_RESET_FXS1, /* Device uses FXS1 reset pin. Pin on base board depends on base board parameters. */
         /* Channel description */
         {
            /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_ALAW,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               0,
               0
            },
            BP_NULL_CHANNEL_DESCRIPTION_MACRO
         },
      },

      /* Always end device list with this macro. */
      BP_NULL_DEVICE_MACRO_NEW,
   },
   /* SLIC Device Profile */
   BP_VD_LCQCUK,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_PCMHAL_ENABLE )
};

/* End of Daughter Card Definitions */


/*
 * -------------------------- Voice Mother Board Configs ------------------------------
 */
#if defined(_BCM96328_) || defined(CONFIG_BCM96328)

static VOICE_DAUGHTER_BOARD_PARMS * g_96328avngr_dCardList[] = {
   &g_voiceBoard_NOSLIC,
   &g_voiceBoard_SI32176,
   &g_voiceBoard_SI3217x,
   &g_voiceBoard_LE89116,
   &g_voiceBoard_LE88506,
   &g_voiceBoard_SI32267,
   &g_voiceBoard_ZL88601,
   &g_voiceBoard_SI32260,
   &g_voiceBoard_SI32260_SI3050,
   0
};

static bp_elem_t g_voice_bcm96328avngr[] = {                 
   {bp_cpBoardId,               .u.cp = "96328avngr"},           
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_1},             
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_2},             
   {bp_usFxsFxo3SpiSSNum,       .u.us = SPI_DEV_6},             
   {bp_usGpioFxsFxoRst1,        .u.us = BP_GPIO_29_AL},          
   {bp_daughterCardList,        .u.ptr = g_96328avngr_dCardList},
   {bp_last}                                                    
};                                                              
                                                                

static bp_elem_t * g_VoiceBoardParms[]=
{
   g_voice_bcm96328avngr,
   0
};

#endif


#if defined(_BCM96362_) || defined(CONFIG_BCM96362)


static VOICE_DAUGHTER_BOARD_PARMS * g_96362advngr2_dCardList[] = {
   &g_voiceBoard_NOSLIC,
   &g_voiceBoard_SI3217x,
   &g_voiceBoard_SI32267,   
   &g_voiceBoard_LE88506,
   &g_voiceBoard_VE890_INVBOOST,
   &g_voiceBoard_SI32260,
   &g_voiceBoard_SI32260_SI3050,
   &g_voiceBoard_SI32260_SI3050_QC,
   &g_voiceBoard_ZL88601,
   0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_dantu_dCardList[] = {
   &g_voiceBoard_tch_dantu, 0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_danto_dCardList[] = {
   &g_voiceBoard_tch_danto, 0
};



static bp_elem_t g_voice_tch_danto[] = {
   {bp_cpBoardId,               .u.cp = "DANT-O"},
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_0},
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_1},
   {bp_usFxsFxo3SpiSSNum,       .u.us = SPI_DEV_2},
   {bp_usGpioFxsFxoRst1,            .u.us = BP_GPIO_10_AL},
   {bp_usGpioDectRst,               .u.us = BP_GPIO_34_AL},
   {bp_usGpioVoipRelayCtrl1,        .u.us = BP_GPIO_0_AL},
   {bp_iDectCfg,                .u.ptr = g_iDectStdCfg},
   {bp_daughterCardList,        .u.ptr = g_danto_dCardList},
   {bp_last}
};


static bp_elem_t g_voice_bcm96362advngr2[] = {                 
   {bp_cpBoardId,               .u.cp = "96362ADVNgr2"},           
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_0},             
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_1},             
   {bp_usFxsFxo3SpiSSNum,       .u.us = SPI_DEV_2},             
   {bp_usGpioFxsFxoRst1,        .u.us = BP_GPIO_28_AL},          
   {bp_usGpioFxsFxoRst2,        .u.us = BP_GPIO_31_AL},          
   {bp_usGpioDectRst,           .u.us = BP_GPIO_29_AL},          
   {bp_usGpioVoipRelayCtrl1,    .u.us = BP_GPIO_44_AH},          
   {bp_iDectCfg,                .u.ptr = g_iDectStdCfg},        
   {bp_daughterCardList,        .u.ptr = g_96362advngr2_dCardList},
   {bp_last}                                                    
};                                                              

static bp_elem_t g_voice_bcm96362ravngr2[] = {                 
   {bp_cpBoardId,               .u.cp = "96362RAVNGR2"},           
   {bp_usGpioVoipRelayCtrl1,    .u.us = BP_GPIO_35_AH},          
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm96362advngr2},
   {bp_last}                                                    
};                                                                                                                                                                                             

static bp_elem_t g_voice_bcm96362rpvt[] = {                 
   {bp_cpBoardId,               .u.cp = "96362RPVT"},           
   {bp_usGpioFxsFxoRst3,        .u.us = BP_GPIO_26_AL},          
   {bp_usGpioVoipRelayCtrl1,    .u.us = BP_GPIO_35_AH},          
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm96362advngr2},
   {bp_last}                                                    
};                                                              

static bp_elem_t g_voice_bcm96362rpvt_2u[] = {                 
   {bp_cpBoardId,               .u.cp = "96362RPVT_2U"},           
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm96362rpvt},
   {bp_last}                                                    
};                                                              
                                                                
static bp_elem_t g_voice_bcm96362advn2xh[] = {                 
   {bp_cpBoardId,               .u.cp = "96362ADVN2xh"},           
   {bp_usGpioFxsFxoRst3,        .u.us = BP_GPIO_26_AL},          
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm96362advngr2},
   {bp_last}                                                    
};                                                              
  
static bp_elem_t g_voice_bcm96362radvn2xh[] = {                 
   {bp_cpBoardId,               .u.cp = "96362RADVN2XH"},           
   {bp_usGpioFxsFxoRst3,        .u.us = BP_GPIO_26_AL},          
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm96362advngr2},
   {bp_last}                                                    
};                                                              
                                                              

static bp_elem_t g_voice_tch_dantu[] = {
   {bp_cpBoardId,               .u.cp = "DANT-U"},
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_0},
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_1},
   {bp_usFxsFxo3SpiSSNum,       .u.us = SPI_DEV_2},
   {bp_usGpioFxsFxoRst1,            .u.us = BP_GPIO_33_AL},
   {bp_daughterCardList,        .u.ptr = g_dantu_dCardList},
   {bp_last}
};

static bp_elem_t * g_VoiceBoardParms[]=
{
   g_voice_bcm96362rpvt,
   g_voice_bcm96362rpvt_2u,
   g_voice_bcm96362advn2xh,
   g_voice_bcm96362ravngr2,
   g_voice_bcm96362advngr2,
   g_voice_bcm96362radvn2xh,
   g_voice_tch_dantu,
   g_voice_tch_danto,
   0
};

#endif


#if defined(_BCM963268_) || defined(CONFIG_BCM963268)

static VOICE_DAUGHTER_BOARD_PARMS * g_963168_dCardList_full[] = {
   &g_voiceBoard_NOSLIC,
   &g_voiceBoard_VE890_INVBOOST,  
   &g_voiceBoard_LE88506,         
   &g_voiceBoard_LE89116,         
   &g_voiceBoard_SI3217x,         
   &g_voiceBoard_SI32267,         
   &g_voiceBoard_SI32260x2,       
   &g_voiceBoard_SI32260x2_SI3050,
   &g_voiceBoard_SI32260,         
   &g_voiceBoard_SI32260_SI3050,  
   &g_voiceBoard_SI32260_SI3050_QC,
   &g_voiceBoard_ZL88601,         
   &g_voiceBoard_ZL88701,         
   &g_voiceBoard_ZL88702_ZSI,     
   &g_voiceBoard_LE9662_ZSI,      
   &g_voiceBoard_LE9662_ZSI_BB,
   &g_voiceBoard_LE9642_ZSI_BB,
   &g_voiceBoard_LE88536_ZSI,
   &g_voiceBoard_LE9661_ZSI,
   &g_voiceBoard_ZL88801_89010_BB,     
   &g_voiceBoard_LE9672_ZSI,
   0                              
};

static VOICE_DAUGHTER_BOARD_PARMS * g_963168_dCardList_noFxoRst[] = {
   &g_voiceBoard_NOSLIC,
   &g_voiceBoard_VE890_INVBOOST,  
   &g_voiceBoard_LE88506,         
   &g_voiceBoard_LE89116,         
   &g_voiceBoard_SI3217x,         
   &g_voiceBoard_SI32267,         
   &g_voiceBoard_SI32260x2,       
   &g_voiceBoard_SI32260,         
   &g_voiceBoard_SI32260_SI3050,  
   &g_voiceBoard_SI32260_SI3050_QC,
   &g_voiceBoard_ZL88601,         
   &g_voiceBoard_ZL88702_ZSI,     
   &g_voiceBoard_LE9662_ZSI,      
   &g_voiceBoard_LE9662_ZSI_BB, 
   &g_voiceBoard_LE9642_ZSI_BB,
   &g_voiceBoard_LE88536_ZSI,     
   &g_voiceBoard_LE9661_ZSI,
   &g_voiceBoard_LE9672_ZSI,
   0                              
};

static VOICE_DAUGHTER_BOARD_PARMS * g_vdntw_dCardList[] = {
   &g_voiceBoard_tch_vdntw, 0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_vdnto_dCardList[] = {
   &g_voiceBoard_tch_vdnto, 0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_vdnt4_dCardList[] = {
   &g_voiceBoard_tch_vdnt4, 0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_vdnt6_dCardList[] = {
   &g_voiceBoard_tch_vdnt6, 0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_vdnt8_dCardList[] = {
   &g_voiceBoard_tch_vdnt8, 0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_gant1_dCardList[] = {
   &g_voiceBoard_tch_gant1, 0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_gant2_dCardList[] = {
   &g_voiceBoard_tch_gant2, 0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_vantf_dCardList[] = {
   &g_voiceBoard_tch_vantf, 0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_vantg_dCardList[] = {
   &g_voiceBoard_tch_vantg, 0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_vant9_dCardList[] = {
   &g_voiceBoard_tch_vant9, 0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_vant5_dCardList[] = {
   &g_voiceBoard_tch_vant5, 0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_vant6_dCardList[] = {
   &g_voiceBoard_tch_vant6, 0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_vant7_dCardList[] = {
   &g_voiceBoard_tch_vant7, 0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_vantr_dCardList[] = {
   &g_voiceBoard_tch_vantr, 0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_gbntg_dCardList[] = {
   &g_voiceBoard_tch_gbntg, 0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_vantd_dCardList[] = {
   &g_voiceBoard_tch_vantd, 0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_vant4_dCardList[] = {
   &g_voiceBoard_tch_vant4, 0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_vantt_dCardList[] = {
   &g_voiceBoard_tch_vantt, 0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_vbnta_dCardList[] = {
   &g_voiceBoard_tch_vbnta, 0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_vbntb_dCardList[] = {
   &g_voiceBoard_tch_vbntb, 0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_vbntl_dCardList[] = {
   &g_voiceBoard_tch_vbntl, 0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_vdnty_dCardList[] = {
   &g_voiceBoard_tch_vdnty, 0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_gantk_dCardList[] = {
   &g_voiceBoard_tch_gantk, 0
};

static bp_elem_t g_voice_bcm963168mbv_17a[] = {                                                         
   {bp_cpBoardId,               .u.cp = "963168MBV_17A"},           
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_1},             
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_4},             
   {bp_usFxsFxo3SpiSSNum,       .u.us = SPI_DEV_7},             
   {bp_usGpioFxsFxoRst1,        .u.us = BP_GPIO_14_AL},          
   {bp_usGpioFxsFxoRst2,        .u.us = BP_GPIO_15_AL},          
   {bp_usGpioFxsFxoRst3,        .u.us = BP_GPIO_23_AL},          
   {bp_usGpioDectRst,           .u.us = BP_GPIO_35_AL},          
   {bp_usGpioVoipRelayCtrl1,    .u.us = BP_GPIO_8_AH},          
   {bp_iDectCfg,                .u.ptr = g_iDectStdCfg},        
   {bp_daughterCardList,        .u.ptr = g_963168_dCardList_full},
   {bp_last}                                                    
};                                                              

static bp_elem_t g_voice_bcm963168mbv_30a[] = {                 
   {bp_cpBoardId,               .u.cp = "963168MBV_30A"},           
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm963168mbv_17a},
   {bp_last}                                                    
};                                                              

static bp_elem_t g_voice_bcm963168mbv17a302[] = {                 
   {bp_cpBoardId,               .u.cp = "963168MBV17A302"},           
   {bp_usGpioFxsFxoRst3,        .u.us = BP_GPIO_10_AL},          
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm963168mbv_17a},
   {bp_last}                                                    
};                                                              

static bp_elem_t g_voice_bcm963168mbv30a302[] = {                 
   {bp_cpBoardId,               .u.cp = "963168MBV30A302"},           
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm963168mbv17a302},
   {bp_last}                                                    
};                                                              

static bp_elem_t g_voice_bcm963168xh[] = {                 
   {bp_cpBoardId,               .u.cp = "963168XH"},           
   {bp_usGpioFxsFxoRst3,        .u.us = BP_GPIO_21_AL},          
   {bp_usGpioVoipRelayCtrl1,    .u.us = BP_GPIO_39_AH},          
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm963168mbv_17a},
   {bp_last}                                                    
};                                                              
                                                                
static bp_elem_t g_voice_bcm963168xh5[] = {                 
   {bp_cpBoardId,               .u.cp = "963168XH5"},           
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm963168xh},
   {bp_last}                                                    
};                                                              
         
static bp_elem_t g_voice_bcm963168mp[] = {                 
   {bp_cpBoardId,               .u.cp = "963168MP"},           
   {bp_usGpioFxsFxoRst3,        .u.us = BP_NOT_DEFINED},          
   {bp_usGpioDectRst,           .u.us = BP_GPIO_19_AL},          
   {bp_usGpioVoipRelayCtrl1,    .u.us = BP_GPIO_39_AH},   
   {bp_daughterCardList,        .u.ptr = g_963168_dCardList_noFxoRst},          
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm963168mbv_17a},
   {bp_last}                                                    
};                                                              
                                                       
static bp_elem_t g_voice_bcm963168mbv3[] = {                 
   {bp_cpBoardId,               .u.cp = "963168MBV3"},           
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_1},             
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_4},             
   {bp_usFxsFxo3SpiSSNum,       .u.us = SPI_DEV_5},             
   {bp_usGpioFxsFxoRst1,        .u.us = BP_GPIO_14_AL},          
   {bp_usGpioFxsFxoRst2,        .u.us = BP_GPIO_15_AL},          
   {bp_usGpioFxsFxoRst3,        .u.us = BP_GPIO_23_AL},          
   {bp_usGpioDectRst,           .u.us = BP_GPIO_35_AL},          
   {bp_usGpioVoipRelayCtrl1,    .u.us = BP_GPIO_12_AH},          
   {bp_iDectCfg,                .u.ptr = g_iDectStdCfg},        
   {bp_daughterCardList,        .u.ptr = g_963168_dCardList_full},
   {bp_last}                                                    
};                                                              

static bp_elem_t g_voice_bcm963168plc[] = {                 
   {bp_cpBoardId,               .u.cp = "963168PLC"},           
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_1},             
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_4},             
   {bp_usFxsFxo3SpiSSNum,       .u.us = SPI_DEV_5},             
   {bp_usGpioFxsFxoRst1,        .u.us = BP_GPIO_14_AL},          
   {bp_usGpioFxsFxoRst2,        .u.us = BP_GPIO_15_AL},          
   {bp_usGpioFxsFxoRst3,        .u.us = BP_GPIO_23_AL},          
   {bp_usGpioVoipRelayCtrl1,    .u.us = BP_GPIO_12_AH},          
   {bp_daughterCardList,        .u.ptr = g_963168_dCardList_full},
   {bp_last}                                                    
};                                                              

static bp_elem_t g_voice_bcm963168plc_mocawan[] = {                 
   {bp_cpBoardId,               .u.cp = "963168PLC_MOCAWAN"},           
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm963168plc},
   {bp_last}                                                    
};                                                              

static bp_elem_t g_voice_bcm963268v30a[] = {                 
   {bp_cpBoardId,               .u.cp = "963268V30A"},           
   {bp_usGpioFxsFxoRst3,        .u.us = BP_GPIO_50_AL},          
   {bp_usGpioDectRst,           .u.us = BP_GPIO_51_AL},          
   {bp_usGpioVoipRelayCtrl1,    .u.us = BP_GPIO_39_AH},          
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm963168mbv3},
   {bp_last}                                                    
};                       
                                       
static bp_elem_t g_voice_bcm963268bu[] = {                 
   {bp_cpBoardId,               .u.cp = "963268BU"},           
   {bp_usGpioFxsFxoRst3,        .u.us = BP_GPIO_18_AL},          
   {bp_usGpioDectRst,           .u.us = BP_GPIO_19_AL},          
   {bp_usGpioVoipRelayCtrl1,    .u.us = BP_GPIO_39_AH},          
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm963168mbv3},
   {bp_last}                                                    
};                                                              

static bp_elem_t g_voice_bcm963268bu_p300[] = {                 
   {bp_cpBoardId,               .u.cp = "963268BU_P300"},           
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm963268bu},
   {bp_last}                                                    
};                                                                                                                                                                                             

static bp_elem_t g_voice_bcm963168vx[] = {                 
   {bp_cpBoardId,               .u.cp = "963168VX"},           
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_1},             
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_4},             
   {bp_usFxsFxo3SpiSSNum,       .u.us = SPI_DEV_5},             
   {bp_usGpioFxsFxoRst1,        .u.us = BP_GPIO_14_AL},          
   {bp_usGpioFxsFxoRst2,        .u.us = BP_GPIO_15_AL},          
   {bp_usGpioFxsFxoRst3,        .u.us = BP_GPIO_9_AL},          
   {bp_usGpioVoipRelayCtrl1,    .u.us = BP_GPIO_8_AH},          
   {bp_daughterCardList,        .u.ptr = g_963168_dCardList_full},
   {bp_last}                                                    
};                                                              
                                                                
static bp_elem_t g_voice_bcm963168vx_p400[] = {                 
   {bp_cpBoardId,               .u.cp = "963168VX_P400"},           
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm963168vx},
   {bp_last}                                                    
};                                                              
                                                                                                   
static bp_elem_t g_voice_bcm963168xn5[] = {                 
   {bp_cpBoardId,               .u.cp = "963168XN5"},           
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_1},             
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_4},             
   {bp_usFxsFxo3SpiSSNum,       .u.us = SPI_DEV_7},             
   {bp_usGpioFxsFxoRst1,        .u.us = BP_GPIO_14_AL},          
   {bp_usGpioFxsFxoRst2,        .u.us = BP_GPIO_18_AL},          
   {bp_usGpioFxsFxoRst3,        .u.us = BP_GPIO_8_AL},          
   {bp_usGpioVoipRelayCtrl1,    .u.us = BP_GPIO_39_AH},          
   {bp_daughterCardList,        .u.ptr = g_963168_dCardList_full},
   {bp_last}                                                    
};                                                                                                                              

static bp_elem_t g_voice_bcm963168wfar[] = {                 
   {bp_cpBoardId,               .u.cp = "963168WFAR"},           
   {bp_usGpioFxsFxoRst2,        .u.us = BP_GPIO_15_AL},          
   {bp_usGpioFxsFxoRst3,        .u.us = BP_GPIO_10_AL},          
   {bp_usGpioVoipRelayCtrl1,    .u.us = BP_NOT_DEFINED},          
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm963168xn5},
   {bp_last}                                                    
};                                                              
                                                                
static bp_elem_t g_voice_bcm963168ac5[] = {                 
   {bp_cpBoardId,               .u.cp = "963168AC5"},           
   {bp_usGpioFxsFxoRst2,        .u.us = BP_GPIO_15_AL},          
   {bp_usGpioFxsFxoRst3,        .u.us = BP_GPIO_21_AL},          
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm963168xn5},
   {bp_last}                                                    
}; 

static bp_elem_t g_voice_bcm963268sv1[] = {                 
   {bp_cpBoardId,               .u.cp = "963268SV1"},           
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_1},             
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_4},             
   {bp_usFxsFxo3SpiSSNum,       .u.us = SPI_DEV_3},             
   {bp_usGpioFxsFxoRst1,        .u.us = BP_GPIO_14_AL},          
   {bp_usGpioFxsFxoRst2,        .u.us = BP_GPIO_15_AL},          
   {bp_usGpioDectRst,           .u.us = BP_GPIO_35_AL},          
   {bp_usGpioVoipRelayCtrl1,    .u.us = BP_GPIO_8_AH},          
   {bp_iDectCfg,                .u.ptr = g_iDectStdCfg},        
   {bp_daughterCardList,        .u.ptr = g_963168_dCardList_noFxoRst},
   {bp_last}                                                    
};                                                                                                                                                                    

static bp_elem_t g_voice_tch_vdntw[] = {
   {bp_cpBoardId,               .u.cp = "VDNT-W"},
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_1},
   {bp_usGpioFxsFxoRst1,        .u.us = BP_GPIO_39_AL},
   {bp_daughterCardList,        .u.ptr = g_vdntw_dCardList},
   {bp_last}
};

static bp_elem_t g_voice_tch_vdnto[] = {
   {bp_cpBoardId,               .u.cp = "VDNT-O"},
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_0},
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_1},
   {bp_usFxsFxo3SpiSSNum,       .u.us = SPI_DEV_4},
   {bp_usGpioFxsFxoRst1,            .u.us = BP_GPIO_13_AL},
   {bp_usGpioFxsFxoRst2,            .u.us = BP_GPIO_14_AL},
   {bp_usGpioDectRst,               .u.us = BP_GPIO_18_AL},
   {bp_usGpioVoipRelayCtrl1,        .u.us = BP_GPIO_8_AL},
   {bp_iDectCfg,                .u.ptr = g_iDectStdCfg},
   {bp_daughterCardList,        .u.ptr = g_vdnto_dCardList},
   {bp_last}
};

static bp_elem_t g_voice_tch_vdnt4[] = {
   {bp_cpBoardId,               .u.cp = "VDNT-4"},
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_1},
   {bp_usGpioFxsFxoRst1,        .u.us = BP_GPIO_14_AL},
   {bp_daughterCardList,        .u.ptr = g_vdnt4_dCardList},
   {bp_last}
};

static bp_elem_t g_voice_tch_vdnt6[] = {
   {bp_cpBoardId,               .u.cp = "VDNT-6"},
   {bp_daughterCardList,        .u.ptr = g_vdnt6_dCardList},
   {bp_elemTemplate,            .u.bp_elemp = g_voice_tch_vdnto},
   {bp_last}
};

static bp_elem_t g_voice_tch_vdnt8[] = {
   {bp_cpBoardId,               .u.cp = "VDNT-8"},
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_0},
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_1},
   {bp_usFxsFxo3SpiSSNum,       .u.us = SPI_DEV_4},
   {bp_usGpioFxsFxoRst1,            .u.us = BP_GPIO_13_AL},
   {bp_usGpioFxsFxoRst2,            .u.us = BP_GPIO_14_AL},
   {bp_usGpioVoipRelayCtrl1,        .u.us = BP_GPIO_8_AH},
   {bp_daughterCardList,        .u.ptr = g_vdnt8_dCardList},

   {bp_last}
};

static bp_elem_t g_voice_tch_gant1[] = {
   {bp_cpBoardId,               .u.cp = "GANT-1"},
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_0},
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_1},
   {bp_usFxsFxo3SpiSSNum,       .u.us = SPI_DEV_4},
   {bp_usGpioFxsFxoRst1,            .u.us = BP_GPIO_20_AL},
   {bp_daughterCardList,        .u.ptr = g_gant1_dCardList},
   {bp_last}
};

static bp_elem_t g_voice_tch_gant2[] = {
   {bp_cpBoardId,               .u.cp = "GANT-2"},
   {bp_daughterCardList,        .u.ptr = g_gant2_dCardList},
   {bp_elemTemplate,            .u.bp_elemp = g_voice_tch_gant1},
   {bp_last}
};


static bp_elem_t g_voice_tch_vantf[] = {
   {bp_cpBoardId,               .u.cp = "VANT-F"},
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_0},
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_1},
   {bp_usFxsFxo3SpiSSNum,       .u.us = SPI_DEV_4},
   {bp_usGpioFxsFxoRst1,            .u.us = BP_GPIO_15_AL},
   {bp_usGpioFxsFxoRst2,            .u.us = BP_GPIO_15_AL},
   {bp_usGpioFxsFxoRst3,            .u.us = BP_GPIO_14_AL},
   {bp_usGpioDectRst,               .u.us = BP_GPIO_8_AL},
   {bp_usGpioVoipRelayCtrl1,        .u.us = BP_GPIO_13_AL},
   {bp_usGpioVoipRelayCtrl2,        .u.us = BP_GPIO_21_AL},
   {bp_iDectCfg,                .u.ptr = g_iDectStdCfg},
   {bp_daughterCardList,        .u.ptr = g_vantf_dCardList},
   {bp_last}
};

static bp_elem_t g_voice_tch_vantg[] = {
   {bp_cpBoardId,               .u.cp = "VANT-G"},
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_1},
   {bp_usGpioFxsFxoRst1,            .u.us = BP_GPIO_20_AL},
   {bp_usGpioVoipRelayCtrl1,        .u.us = BP_NOT_DEFINED},
   {bp_daughterCardList,        .u.ptr = g_vantg_dCardList},
   {bp_last}
};

static bp_elem_t g_voice_tch_vant9[] = {
   {bp_cpBoardId,               .u.cp = "VANT-9"},
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_1},
   {bp_usGpioFxsFxoRst1,            .u.us = BP_GPIO_20_AL},
   {bp_daughterCardList,        .u.ptr = g_vant9_dCardList},
   {bp_last}
};

static bp_elem_t g_voice_tch_vant5[] = {
   {bp_cpBoardId,               .u.cp = "VANT-5"},
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_0},
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_1},
   {bp_usFxsFxo3SpiSSNum,       .u.us = SPI_DEV_4},
   {bp_usGpioFxsFxoRst1,            .u.us = BP_GPIO_15_AL},
   {bp_usGpioFxsFxoRst2,            .u.us = BP_GPIO_15_AL},
   {bp_usGpioFxsFxoRst3,            .u.us = BP_GPIO_14_AL},
   {bp_usGpioVoipRelayCtrl1,        .u.us = BP_GPIO_NONE},
   {bp_usGpioVoipRelayCtrl2,        .u.us = BP_GPIO_21_AL},
   {bp_daughterCardList,        .u.ptr = g_vant5_dCardList},
   {bp_last}
};

static bp_elem_t g_voice_tch_vant6[] = {
   {bp_cpBoardId,               .u.cp = "VANT-6"},
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_1},
   {bp_usGpioFxsFxoRst1,            .u.us = BP_GPIO_15_AL},
   {bp_daughterCardList,        .u.ptr = g_vant6_dCardList},
   {bp_last}
};

static bp_elem_t g_voice_tch_vant7[] = {
   {bp_cpBoardId,               .u.cp = "VANT-7"},
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_1},
   {bp_usGpioFxsFxoRst1,            .u.us = BP_GPIO_15_AL},
   {bp_daughterCardList,        .u.ptr = g_vant7_dCardList},
   {bp_last}
};

static bp_elem_t g_voice_tch_vantr[] = {
   {bp_cpBoardId,               .u.cp = "VANT-R"},
   {bp_daughterCardList,        .u.ptr = g_vantr_dCardList},
   {bp_elemTemplate,            .u.bp_elemp = g_voice_tch_vantf},
   {bp_last}
};

static bp_elem_t g_voice_tch_gbntg[] = {
   {bp_cpBoardId,               .u.cp = "GBNT-G"},
   {bp_daughterCardList,        .u.ptr = g_gbntg_dCardList},
   {bp_elemTemplate,            .u.bp_elemp = g_voice_tch_vantf},
   {bp_last}
};

static bp_elem_t g_voice_tch_vantd[] = {
   {bp_cpBoardId,               .u.cp = "VANT-D"},
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_0},
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_1},
   {bp_usFxsFxo3SpiSSNum,       .u.us = SPI_DEV_4},
   {bp_usGpioFxsFxoRst1,            .u.us = BP_GPIO_15_AL},
   {bp_usGpioFxsFxoRst2,            .u.us = BP_GPIO_15_AL},
   {bp_usGpioVoipRelayCtrl1,        .u.us = BP_GPIO_13_AL},
   {bp_usGpioVoipRelayCtrl2,        .u.us = BP_GPIO_21_AL},
   {bp_daughterCardList,        .u.ptr = g_vantd_dCardList},
   {bp_last}
};

static bp_elem_t g_voice_tch_vant4[] = {
   {bp_cpBoardId,               .u.cp = "VANT-4"},
   {bp_daughterCardList,        .u.ptr = g_vant4_dCardList},
   {bp_elemTemplate,            .u.bp_elemp = g_voice_tch_vantd},
   {bp_last}
};

static bp_elem_t g_voice_tch_vantt[] = {
   {bp_cpBoardId,               .u.cp = "VANT-T"},
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_0},
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_1},
   {bp_usFxsFxo3SpiSSNum,       .u.us = SPI_DEV_4},
   {bp_usGpioFxsFxoRst1,            .u.us = BP_GPIO_14_AL},
   {bp_usGpioFxsFxoRst2,            .u.us = BP_GPIO_14_AL},
   {bp_daughterCardList,        .u.ptr = g_vantt_dCardList},
   {bp_last}
};

static bp_elem_t g_voice_tch_vbnta[] = {
   {bp_cpBoardId,               .u.cp = "VBNT-A"},
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_1},
   {bp_usFxsFxo3SpiSSNum,       .u.us = SPI_DEV_4},
   {bp_usGpioFxsFxoRst1,            .u.us = BP_GPIO_15_AL},
   {bp_usGpioFxsFxoRst3,            .u.us = BP_GPIO_14_AL},
   {bp_usGpioVoipRelayCtrl1,        .u.us = BP_GPIO_NONE},
   {bp_usGpioVoipRelayCtrl2,        .u.us = BP_GPIO_21_AL},
   {bp_daughterCardList,        .u.ptr = g_vbnta_dCardList},
   {bp_last}
};

static bp_elem_t g_voice_tch_vbntb[] = {
   {bp_cpBoardId,               .u.cp = "VBNT-B"},
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_1},
   {bp_usGpioFxsFxoRst1,            .u.us = BP_GPIO_34_AL},
   {bp_usGpioDectRst,               .u.us = BP_GPIO_8_AL},
   {bp_usGpioVoipRelayCtrl1,        .u.us = BP_GPIO_13_AL},
   {bp_iDectCfg,                .u.ptr = g_iDectStdCfg},
   {bp_daughterCardList,        .u.ptr = g_vbntb_dCardList},
   {bp_last}
};

static bp_elem_t g_voice_tch_vbntl[] = {
   {bp_cpBoardId,               .u.cp = "VBNT-L"},
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_1},
   {bp_usGpioFxsFxoRst1,            .u.us = BP_GPIO_15_AL},
   {bp_daughterCardList,        .u.ptr = g_vbntl_dCardList},
   {bp_last}
};

static bp_elem_t g_voice_tch_vdnty[] = {
   {bp_cpBoardId,               .u.cp = "VDNT-Y"},
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_1},
   {bp_usGpioFxsFxoRst1,        .u.us = BP_GPIO_15_AL},
   {bp_daughterCardList,        .u.ptr = g_vdnty_dCardList},
   {bp_last}
};

static bp_elem_t g_voice_tch_gantk[] = {
   {bp_cpBoardId,               .u.cp = "GANT-K"},
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_0},
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_1},
   {bp_usFxsFxo3SpiSSNum,       .u.us = SPI_DEV_4},
   {bp_usGpioFxsFxoRst1,            .u.us = BP_GPIO_13_AL},
   {bp_usGpioFxsFxoRst2,            .u.us = BP_GPIO_14_AL},
   {bp_daughterCardList,        .u.ptr = g_gantk_dCardList},
   {bp_last}
};


static bp_elem_t * g_VoiceBoardParms[]=
{
   g_voice_bcm963168mbv_17a,
   g_voice_bcm963168mbv_30a,
   g_voice_bcm963168mbv17a302,
   g_voice_bcm963168mbv30a302,
   g_voice_bcm963168xh,
   g_voice_bcm963168xh5,
   g_voice_bcm963168mp,
   g_voice_bcm963168mbv3,
   g_voice_bcm963268v30a,
   g_voice_bcm963268bu,
   g_voice_bcm963268bu_p300,
   g_voice_bcm963168vx,
   g_voice_bcm963168vx_p400,
   g_voice_bcm963168xn5,
   g_voice_bcm963168wfar,
   g_voice_bcm963168ac5,
   g_voice_bcm963268sv1,
   g_voice_bcm963168plc,
   g_voice_bcm963168plc_mocawan,
   g_voice_tch_vdnto,
   g_voice_tch_vdnt4,
   g_voice_tch_vdntw,
   g_voice_tch_vdnt6,
   g_voice_tch_vdnt8,
   g_voice_tch_gant1,
   g_voice_tch_gant2,
   g_voice_tch_vantf,
   g_voice_tch_vantg,
   g_voice_tch_vant9,
   g_voice_tch_vant5,
   g_voice_tch_vant6,
   g_voice_tch_vant7,
   g_voice_tch_vantr,
   g_voice_tch_gbntg,
   g_voice_tch_vantd,
   g_voice_tch_vant4,
   g_voice_tch_vantt,
   g_voice_tch_vbnta,
   g_voice_tch_vbntl,
   g_voice_tch_vdnty,
   g_voice_tch_gantk,
   g_voice_tch_vbntb,
   0
};

#endif


#if defined(_BCM96838_) || defined(CONFIG_BCM96838)

static VOICE_DAUGHTER_BOARD_PARMS * g_968380_dCardList_Le9540[] = {
   &g_voiceBoard_NOSLIC,
   &g_voiceBoard_LE9540,
   0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_968380_dCardList_Si32392[] = {
   &g_voiceBoard_NOSLIC,
   &g_voiceBoard_SI32392,
   0
};

static bp_elem_t g_voice_bcm968380fhgu[] = {                 
   {bp_cpBoardId,               .u.cp = "968380FHGU"},           
   {bp_usZarIfEnable,           .u.us = BP_GPIO_0_AH}, 
   {bp_usZarIfSdin,             .u.us = BP_GPIO_1_AH}, 
   {bp_usZarIfSdout,            .u.us = BP_GPIO_2_AH}, 
   {bp_usZarIfSclk,             .u.us = BP_GPIO_3_AH}, 
   {bp_usVoipApmChSwap,         .u.us = BP_APMCH_SWAP_ON},
   {bp_daughterCardList,        .u.ptr = g_968380_dCardList_Le9540},
   {bp_last}                                                    
};                                                              
                                                                
static bp_elem_t g_voice_bcm968380fehg[] = {                 
   {bp_cpBoardId,               .u.cp = "968380FEHG"},           
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm968380fhgu},
   {bp_last}                                                    
};                                                              
                                                                
static bp_elem_t g_voice_bcm968380fggu[] = {                 
   {bp_cpBoardId,               .u.cp = "968380FGGU"},           
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm968380fhgu},
   {bp_last}                                                    
};                                                              
                                                                
static bp_elem_t g_voice_bcm968380fegu[] = {                 
   {bp_cpBoardId,               .u.cp = "968380FEGU"},           
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm968380fhgu},
   {bp_last}                                                    
};            
                                                  
static bp_elem_t g_voice_bcm968380gerg[] = {                 
   {bp_cpBoardId,               .u.cp = "968380GERG"},           
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm968380fhgu},
   {bp_last}                                                    
};     

static bp_elem_t g_voice_bcm968380gwan[] = {                 
   {bp_cpBoardId,               .u.cp = "968380GWAN"},           
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm968380fhgu},
   {bp_last}                                                    
};    

static bp_elem_t g_voice_bcm968380lte[] = {                 
   {bp_cpBoardId,               .u.cp = "968380LTE"},           
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm968380fhgu},
   {bp_last}                                                    
};                                                            

static bp_elem_t g_voice_bcm968380eprg[] = {                 
   {bp_cpBoardId,               .u.cp = "968380EPRG"},           
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm968380fhgu},
   {bp_last}                                                    
};                                                              
                                                                
static bp_elem_t g_voice_bcm968380ffhg[] = {                 
   {bp_cpBoardId,               .u.cp = "968380FFHG"},           
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm968380fhgu},
   {bp_last}                                                    
};                                                              
                                                                
static bp_elem_t g_voice_bcm968380fesfu[] = {                 
   {bp_cpBoardId,               .u.cp = "968380FESFU"},           
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm968380fhgu},
   {bp_last}                                                    
};                                                              
                                                                    
static bp_elem_t g_voice_bcm968385sfu[] = {                 
   {bp_cpBoardId,               .u.cp = "968385SFU"},           
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm968380fhgu},
   {bp_last}                                                    
};                                                              
                                                                
static bp_elem_t g_voice_bcm968385esfu[] = {                 
   {bp_cpBoardId,               .u.cp = "968385ESFU"},           
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm968380fhgu},
   {bp_last}                                                    
};                                                              

static bp_elem_t g_voice_bcm968380fhgu_si[] = {                 
   {bp_cpBoardId,               .u.cp = "968380FHGU_SI"},           
   {bp_usHvgMaxPwm,             .u.us = BP_GPIO_0_AH}, 
   {bp_usSi32392SpiSSNum,       .u.us = SPI_DEV_1},  
   {bp_usVoipApmChSwap,         .u.us = BP_APMCH_SWAP_ON}, 
   {bp_daughterCardList,        .u.ptr = g_968380_dCardList_Si32392},
   {bp_last}                                                    
};                                                              
                                                                                                                                                                                            
static VOICE_DAUGHTER_BOARD_PARMS * g_968380fsv_g_dCardList[] = {
   &g_voiceBoard_NOSLIC,
   &g_voiceBoard_LE9540,
   &g_voiceBoard_SI32392,
   &g_voiceBoard_LE88506,
   &g_voiceBoard_VE890_INVBOOST,
   &g_voiceBoard_LE88536_ZSI,
   &g_voiceBoard_SI32267,
   &g_voiceBoard_ZL88601,
   &g_voiceBoard_ZL88702_ZSI,
   &g_voiceBoard_LE9672_ZSI,
   0
};

static bp_elem_t g_voice_bcm968380fsv_g[] = {                 
   {bp_cpBoardId,               .u.cp = "968380FSV_G"},           
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_4},             
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_6},             
   {bp_usFxsFxo3SpiSSNum,       .u.us = SPI_DEV_7},             
   {bp_usGpioFxsFxoRst1,        .u.us = BP_GPIO_5_AL},          
   {bp_usGpioFxsFxoRst2,        .u.us = BP_GPIO_6_AL},          
   {bp_usGpioFxsFxoRst3,        .u.us = BP_GPIO_7_AL},          
   {bp_usZarIfEnable,           .u.us = BP_GPIO_40_AH}, 
   {bp_usZarIfSdin,             .u.us = BP_GPIO_6_AH}, 
   {bp_usZarIfSdout,            .u.us = BP_GPIO_42_AH}, 
   {bp_usZarIfSclk,             .u.us = BP_GPIO_41_AH}, 
   {bp_usGpioLe9540Reset,       .u.us = BP_GPIO_4_AL}, 
   {bp_usHvgMaxPwm,             .u.us = BP_GPIO_33_AH}, 
   {bp_usSi32392SpiSSNum,       .u.us = SPI_DEV_7},  
   {bp_usVoipApmChSwap,         .u.us = BP_APMCH_SWAP_ON},
   {bp_daughterCardList,        .u.ptr = g_968380fsv_g_dCardList},
   {bp_last}                                                    
};                                                              
                                                                
static bp_elem_t g_voice_bcm968380fsv_e[] = {                 
   {bp_cpBoardId,               .u.cp = "968380FSV_E"},           
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm968380fsv_g},
   {bp_last}                                                    
};                                                              
                                                                
static bp_elem_t g_voice_bcm968380fsv_ge[] = {                 
   {bp_cpBoardId,               .u.cp = "968380FSV_GE"},           
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm968380fsv_g},
   {bp_last}                                                    
};                                                              
                                                                
                                                                
static VOICE_DAUGHTER_BOARD_PARMS * g_968380gerg_si_dCardList[] = {
   &g_voiceBoard_NOSLIC,
   &g_voiceBoard_SI32392,
   &g_voiceBoard_LE88506,
   &g_voiceBoard_VE890_INVBOOST,
   &g_voiceBoard_SI32267,
   &g_voiceBoard_ZL88601,
   &g_voiceBoard_SI32260,
   &g_voiceBoard_SI32260_SI3050,
   &g_voiceBoard_SI32260_SI3050_QC,
   &g_voiceBoard_ZL88701,
   &g_voiceBoard_LE9661_ZSI,
   0
};


static VOICE_DAUGHTER_BOARD_PARMS * g_968381gerg_si_dCardList[] = {
   &g_voiceBoard_NOSLIC,
   &g_voiceBoard_LE88506,
   &g_voiceBoard_VE890_INVBOOST,
   &g_voiceBoard_SI32267,
   &g_voiceBoard_ZL88601,
   &g_voiceBoard_SI32260,
   &g_voiceBoard_SI32260_SI3050,
   &g_voiceBoard_SI32260_SI3050_QC,
   &g_voiceBoard_ZL88701,
   &g_voiceBoard_LE9661_ZSI,
   0
};


static bp_elem_t g_voice_bcm968380gerg_si[] = {                 
   {bp_cpBoardId,               .u.cp = "968380GERG_SI"},           
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_0},             
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_1},             
   {bp_usFxsFxo3SpiSSNum,       .u.us = SPI_DEV_2},             
   {bp_usGpioFxsFxoRst1,        .u.us = BP_GPIO_36_AL},          
   {bp_usGpioFxsFxoRst2,        .u.us = BP_GPIO_40_AL},          
   {bp_usGpioFxsFxoRst3,        .u.us = BP_GPIO_38_AL},          
   {bp_usHvgMaxPwm,             .u.us = BP_GPIO_0_AH}, 
   {bp_usSi32392SpiSSNum,       .u.us = SPI_DEV_1},  
   {bp_usVoipApmChSwap,         .u.us = BP_APMCH_SWAP_ON},
   {bp_daughterCardList,        .u.ptr = g_968380gerg_si_dCardList},
   {bp_last}                                                    
};                                                              
                                                                
static bp_elem_t g_voice_bcm968380sv_g[] = {                 
   {bp_cpBoardId,               .u.cp = "968380SV_G"},           
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_4},             
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_6},             
   {bp_usFxsFxo3SpiSSNum,       .u.us = SPI_DEV_7},             
   {bp_usGpioFxsFxoRst1,        .u.us = BP_GPIO_5_AL},          
   {bp_usGpioFxsFxoRst2,        .u.us = BP_GPIO_38_AL},          
   {bp_usGpioFxsFxoRst3,        .u.us = BP_GPIO_7_AL},          
   {bp_usZarIfEnable,           .u.us = BP_GPIO_40_AH}, 
   {bp_usZarIfSdin,             .u.us = BP_GPIO_6_AH}, 
   {bp_usZarIfSdout,            .u.us = BP_GPIO_42_AH}, 
   {bp_usZarIfSclk,             .u.us = BP_GPIO_41_AH}, 
   {bp_usVoipApmChSwap,         .u.us = BP_APMCH_SWAP_ON},
   {bp_daughterCardList,        .u.ptr = g_968380_dCardList_Le9540},
   {bp_last}                                                    
};                                                              
                                                                
static bp_elem_t g_voice_bcm968380sv_e[] = {                 
   {bp_cpBoardId,               .u.cp = "968380SV_E"},           
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm968380sv_g},
   {bp_last}                                                    
};                                                              
                                                                

static bp_elem_t g_voice_bcm968381sv_g[] = {                 
   {bp_cpBoardId,               .u.cp = "968381SV_G"},           
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_4},             
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_6},             
   {bp_usFxsFxo3SpiSSNum,       .u.us = SPI_DEV_6},      // SPI3 and SPI2 are tied with SPI_6       
   {bp_usGpioFxsFxoRst1,        .u.us = BP_GPIO_5_AL},          
   {bp_usGpioFxsFxoRst2,        .u.us = BP_GPIO_6_AL},          
   {bp_usGpioFxsFxoRst3,        .u.us = BP_GPIO_7_AL},          
   {bp_daughterCardList,        .u.ptr = g_968381gerg_si_dCardList},
   {bp_last}                                                    
};  

static bp_elem_t g_voice_bcm968385sfu_si[] = {                 
   {bp_cpBoardId,               .u.cp = "968385SFU_SI"},           
   {bp_usHvgMaxPwm,             .u.us = BP_GPIO_0_AH}, 
   {bp_usSi32392SpiSSNum,       .u.us = SPI_DEV_1},  
   {bp_daughterCardList,        .u.ptr = g_968380_dCardList_Si32392},
   {bp_last}                                                    
};                                                              
                                                                
static VOICE_DAUGHTER_BOARD_PARMS * g_968385sv_g_dCardList[] = {
   &g_voiceBoard_NOSLIC,
   &g_voiceBoard_LE9540,
   &g_voiceBoard_SI32260,
   &g_voiceBoard_SI32392,
   &g_voiceBoard_LE88506,
   &g_voiceBoard_VE890_INVBOOST,
   &g_voiceBoard_LE88536_ZSI,
   &g_voiceBoard_SI32267,
   &g_voiceBoard_ZL88601,
   &g_voiceBoard_ZL88701,
   &g_voiceBoard_ZL88702_ZSI,
   &g_voiceBoard_LE9662_ZSI_BB,
   &g_voiceBoard_LE9642_ZSI_BB,
   &g_voiceBoard_LE9661_ZSI,
   &g_voiceBoard_LE9672_ZSI,
   0
};

static bp_elem_t g_voice_bcm968385sv_g[] = {                 
   {bp_cpBoardId,               .u.cp = "968385SV_G"},           
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_4},             
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_6},             
   {bp_usFxsFxo3SpiSSNum,       .u.us = SPI_DEV_6},             
   {bp_usGpioFxsFxoRst1,        .u.us = BP_GPIO_5_AL},          
   {bp_usGpioFxsFxoRst2,        .u.us = BP_GPIO_6_AL},          
   {bp_usGpioFxsFxoRst3,        .u.us = BP_GPIO_7_AL},          
   {bp_usZarIfEnable,           .u.us = BP_GPIO_40_AH}, 
   {bp_usZarIfSdin,             .u.us = BP_GPIO_6_AH}, 
   {bp_usZarIfSdout,            .u.us = BP_GPIO_42_AH}, 
   {bp_usZarIfSclk,             .u.us = BP_GPIO_41_AH}, 
   {bp_daughterCardList,        .u.ptr = g_968385sv_g_dCardList},
   {bp_last}                                                    
};                                                              

static bp_elem_t g_voice_bcm968385sv_e[] = {                 
   {bp_cpBoardId,               .u.cp = "968385SV_E"},           
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm968385sv_g},
   {bp_last}                                                    
};                                                              
VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_gantu =
{
   VOICECFG_TCH_GANTU_STR,   /* daughter card ID */
   2,   /* FXS number is 2 */
   0,   /* FXO number is 0 */
   {  /* voiceDevice0 parameters */
      {
         /* Device Type */
         BP_VD_SILABS_32392,
         BP_SPI_SS_B1,   /* Device uses SPI_SS_B1 pin. Pin on base board depends on base board parameters. */
         BP_RESET_FXS1,  /* Device uses FXS1 reset pin. Pin on base board depends on base board parameters. */
         {
         /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               0,
               0
            },
         /* Channel 1 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               1,
               1
            }
         }
      },

      BP_NULL_DEVICE_MACRO_NEW
   },
   /* SLIC Device Profile */
   BP_VD_PMOS_BUCK_BOOST, // BP_VD_BUCKBOOST,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_APMHAL_ENABLE )
};
VOICE_DAUGHTER_BOARD_PARMS g_voiceBoard_tch_ganth =
{
   VOICECFG_TCH_GANTH_STR,   /* daughter card ID */
   2,   /* FXS number is 2 */
   0,   /* FXO number is 0 */
   {  /* voiceDevice0 parameters */
      {
         /* Device Type */
         BP_VD_SILABS_32392,
         BP_SPI_SS_B1,   /* Device uses SPI_SS_B1 pin. Pin on base board depends on base board parameters. */
         BP_RESET_FXS1,  /* Device uses FXS1 reset pin. Pin on base board depends on base board parameters. */
         {
         /* Channel 0 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               0,
               0
            },
         /* Channel 1 on device */
            {
               BP_VOICE_CHANNEL_ACTIVE,
               BP_VCTYPE_SLIC,
               BP_VOICE_CHANNEL_PCMCOMP_MODE_NONE,
               BP_VOICE_CHANNEL_NARROWBAND,
               BP_VOICE_CHANNEL_SAMPLE_SIZE_16BITS,
               BP_VOICE_CHANNEL_ENDIAN_BIG,
               1,
               1
            }
         }
      },

      BP_NULL_DEVICE_MACRO_NEW
   },
   /* SLIC Device Profile */
   BP_VD_PMOS_BUCK_BOOST, // BP_VD_BUCKBOOST,
   BP_VOICE_NO_DETECTION,
   /* General-purpose flags */
   ( BP_FLAG_DSP_APMHAL_ENABLE )
};

static VOICE_DAUGHTER_BOARD_PARMS * g_tg1700ac_g_dCardList[] = {
   &g_voiceBoard_NOSLIC,
   &g_voiceBoard_tch_gantu,
   &g_voiceBoard_tch_ganth,
   0
};

static bp_elem_t g_voice_tch_gantu[] = {                 
   {bp_cpBoardId,               .u.cp = "GANT-U"},           
   {bp_ulInterfaceEnable,       .u.ul = BP_PINMUX_FNTYPE_HS_SPI},
   {bp_ulInterfaceEnable,       .u.ul = BP_PINMUX_FNTYPE_PCM},
   {bp_ulInterfaceEnable,       .u.ul = BP_PINMUX_FNTYPE_APM},
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_1},             
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_0},             
   {bp_usSi32392SpiSSNum,       .u.us = SPI_DEV_1},  
   {bp_usVoipApmChSwap,         .u.us = BP_APMCH_SWAP_ON},
   {bp_daughterCardList,        .u.ptr = g_tg1700ac_g_dCardList},
   {bp_last}                                                    
};  

static bp_elem_t g_voice_tch_ganth[] = {                 
   {bp_cpBoardId,               .u.cp = "GANT-H"},           
   {bp_elemTemplate,            .u.bp_elemp = g_voice_tch_gantu},
   {bp_last}                                                    
};  
                                                              
static bp_elem_t * g_VoiceBoardParms[]=
{
   g_voice_bcm968380fhgu,
   g_voice_bcm968380fhgu_si,
   g_voice_bcm968380fehg,
   g_voice_bcm968380fggu,
   g_voice_bcm968380fegu,
   g_voice_bcm968380fsv_g,
   g_voice_bcm968380fsv_e,
   g_voice_bcm968380fsv_ge,
   g_voice_bcm968380gerg,
   g_voice_bcm968380gwan,
   g_voice_bcm968380lte,
   g_voice_bcm968380gerg_si,
   g_voice_bcm968380eprg,
   g_voice_bcm968380ffhg,
   g_voice_bcm968380fesfu,
   g_voice_bcm968380sv_g,
   g_voice_bcm968380sv_e,
   g_voice_bcm968385sfu,
   g_voice_bcm968385sfu_si,
   g_voice_bcm968385esfu,
   g_voice_bcm968385sv_g,
   g_voice_bcm968385sv_e,
   g_voice_bcm968381sv_g,
   &g_voice_tch_gantu,
   &g_voice_tch_ganth,
   0
};

#endif


#if defined(_BCM963138_) || defined(CONFIG_BCM963138)

static VOICE_DAUGHTER_BOARD_PARMS * g_963138_dCardListFull[] = {
   &g_voiceBoard_NOSLIC,
   &g_voiceBoard_SI3217x,
   &g_voiceBoard_VE890_INVBOOST,
   &g_voiceBoard_LE88506,
   &g_voiceBoard_SI32267,
   &g_voiceBoard_LE88536_ZSI,
   &g_voiceBoard_SI32260,
   &g_voiceBoard_SI32260_SI3050,
   &g_voiceBoard_ZL88601,
   &g_voiceBoard_ZL88701,
   &g_voiceBoard_ZL88702_ZSI,
   &g_voiceBoard_SI32260x2,
   &g_voiceBoard_LE9662_ZSI,
   &g_voiceBoard_LE9662_ZSI_BB,
   &g_voiceBoard_LE9642_ZSI_BB,
   &g_voiceBoard_LE9661_ZSI,
   &g_voiceBoard_ZL88801_89010_BB,
   &g_voiceBoard_SI32260x2_SI3050,   
   &g_voiceBoard_LE9672_ZSI,
   0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_vanth_dCardList[] = {
   &g_voiceBoard_tch_vanth, 0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_vantw_dCardList[] = {
   &g_voiceBoard_tch_vantw, 0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_vantv_dCardList[] = {
   &g_voiceBoard_tch_vantv, 0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_vbntf_dCardList[] = {
   &g_voiceBoard_tch_vbntf, 0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_vbnth_dCardList[] = {
   &g_voiceBoard_tch_vbnth, 0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_vbntk_dCardList[] = {
   &g_voiceBoard_tch_vbntk, 0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_963138_dCardListNoFxoRst[] = {
   &g_voiceBoard_NOSLIC,
   &g_voiceBoard_SI3217x,
   &g_voiceBoard_VE890_INVBOOST,
   &g_voiceBoard_LE88506,
   &g_voiceBoard_SI32267,
   &g_voiceBoard_LE88536_ZSI,
   &g_voiceBoard_SI32260,
   &g_voiceBoard_SI32260_SI3050,
   &g_voiceBoard_ZL88601,
   &g_voiceBoard_ZL88701,
   &g_voiceBoard_ZL88702_ZSI,
   &g_voiceBoard_SI32260x2,
   &g_voiceBoard_LE9662_ZSI,
   &g_voiceBoard_LE9662_ZSI_BB,
   &g_voiceBoard_LE9642_ZSI_BB,
   &g_voiceBoard_LE9661_ZSI,
   &g_voiceBoard_ZL88801_89010_BB,
   &g_voiceBoard_LE9672_ZSI,
   0
};


static bp_elem_t g_voice_bcm963138dvt[] = {                 
   {bp_cpBoardId,               .u.cp = "963138DVT"},           
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_1},             
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_2},             
   {bp_usFxsFxo3SpiSSNum,       .u.us = SPI_DEV_4},             
   {bp_usGpioFxsFxoRst1,        .u.us = BP_GPIO_7_AL},          
   {bp_usGpioFxsFxoRst2,        .u.us = BP_GPIO_19_AL},          
   {bp_usGpioFxsFxoRst3,        .u.us = BP_GPIO_36_AL},          
   {bp_usGpioDectRst,           .u.us = BP_GPIO_37_AL},          
   {bp_usGpioVoipRelayCtrl1,    .u.us = BP_GPIO_6_AH},          
   {bp_iDectCfg,                .u.ptr = g_iDectStdCfg},        
   {bp_daughterCardList,        .u.ptr = g_963138_dCardListFull},
   {bp_last}                                                    
};

static bp_elem_t g_voice_bcm963138dvt_p300[] = {                 
   {bp_cpBoardId,               .u.cp = "963138DVT_P300"},           
   {bp_usGpioFxsFxoRst3,        .u.us = BP_GPIO_11_AL},          
   {bp_usGpioDectRst,           .u.us = BP_GPIO_12_AL},          
   {bp_usGpioVoipRelayCtrl1,    .u.us = BP_GPIO_26_AH},    
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm963138dvt},      
   {bp_last}                                                    
};                                                                
                                                                
static bp_elem_t g_voice_bcm963138ref_bmu[] = {                 
   {bp_cpBoardId,               .u.cp = "963138REF_BMU"},           
   {bp_usGpioFxsFxoRst1,        .u.us = BP_GPIO_60_AL},          
   {bp_usGpioFxsFxoRst2,        .u.us = BP_GPIO_61_AL},          
   {bp_usGpioFxsFxoRst3,        .u.us = BP_NOT_DEFINED},          
   {bp_usGpioDectRst,           .u.us = BP_GPIO_62_AL},          
   {bp_usGpioVoipRelayCtrl1,    .u.us = BP_NOT_DEFINED},          
   {bp_daughterCardList,        .u.ptr = g_963138_dCardListNoFxoRst},
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm963138dvt},
   {bp_last}                                                    
};                                                              

static bp_elem_t g_voice_bcm963138bmu_p202[] = {                 
   {bp_cpBoardId,               .u.cp = "963138BMU_P202"},           
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm963138ref_bmu},
   {bp_last}                                                    
};                                                               

static bp_elem_t g_voice_bcm963138ref[] = {                 
   {bp_cpBoardId,               .u.cp = "963138REF"},           
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_1},             
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_2},             
   {bp_usFxsFxo3SpiSSNum,       .u.us = SPI_DEV_4},             
   {bp_usGpioFxsFxoRst1,        .u.us = BP_GPIO_7_AL},          
   {bp_usGpioFxsFxoRst2,        .u.us = BP_GPIO_117_AL},          
   {bp_usGpioFxsFxoRst3,        .u.us = BP_GPIO_116_AL},          
   {bp_usGpioVoipRelayCtrl1,    .u.us = BP_GPIO_6_AH},          
   {bp_daughterCardList,        .u.ptr = g_963138_dCardListFull},
   {bp_last}                                                    
};                                                              
                                                                

static bp_elem_t g_voice_bcm963138ref_p402[] = {                 
   {bp_cpBoardId,               .u.cp = "963138REF_P402"},           
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_1},             
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_4},             
   {bp_usFxsFxo3SpiSSNum,       .u.us = SPI_DEV_5},             
   {bp_usGpioFxsFxoRst1,        .u.us = BP_GPIO_4_AL},          
   {bp_usGpioFxsFxoRst2,        .u.us = BP_GPIO_5_AL},          
   {bp_usGpioFxsFxoRst3,        .u.us = BP_GPIO_6_AL},          
   {bp_usGpioVoipRelayCtrl1,    .u.us = BP_GPIO_3_AH},          
   {bp_daughterCardList,        .u.ptr = g_963138_dCardListFull},
   {bp_last}                                                    
};                                                              

static bp_elem_t g_voice_bcm963138ref_p502[] = {                 
   {bp_cpBoardId,               .u.cp = "963138REF_P502"},           
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm963138ref_p402},
   {bp_last}                                                    
};                                                              

static bp_elem_t g_voice_bcm963138ref_gfast_p40x[] = {
   {bp_cpBoardId,               .u.cp = "963138_GFSTP40X"},
   {bp_usGpioVoipRelayCtrl1,    .u.us = BP_GPIO_NONE},
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm963138ref_p402},
   {bp_last}                                                    
};                                                              
                                                                                                                               
static VOICE_DAUGHTER_BOARD_PARMS * g_963138ref_lte_dCardList[] = {
   &g_voiceBoard_NOSLIC,
   &g_voiceBoard_LE88506,
   &g_voiceBoard_SI32267,
   &g_voiceBoard_SI32260,
   &g_voiceBoard_SI32260_LCQC,
   &g_voiceBoard_LE88536_ZSI,
   &g_voiceBoard_ZL88601,
   &g_voiceBoard_ZL88702_ZSI,
   &g_voiceBoard_LE9662_ZSI,
   &g_voiceBoard_LE9662_ZSI_BB,
   &g_voiceBoard_LE9642_ZSI_BB,
   &g_voiceBoard_LE9661_ZSI,
   &g_voiceBoard_LE89116,
   &g_voiceBoard_LE9672_ZSI,
   0
};

static bp_elem_t g_voice_bcm963138ref_lte[] = {                 
   {bp_cpBoardId,               .u.cp = "963138REF_LTE"},           
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_1},             
   {bp_usGpioFxsFxoRst1,        .u.us = BP_GPIO_34_AL},          
   {bp_daughterCardList,        .u.ptr = g_963138ref_lte_dCardList},
   {bp_last}                                                    
};                                                              
                                                                
static bp_elem_t g_voice_bcm963138lte_p302[] = {                 
   {bp_cpBoardId,               .u.cp = "963138LTE_P302"},           
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm963138ref_lte},
   {bp_last}                                                    
};                                                              

static bp_elem_t g_voice_bcm963138ref_rnc[] = {                 
   {bp_cpBoardId,               .u.cp = "963138REF_RNC"},
   {bp_usGpioFxsFxoRst1,        .u.us = BP_GPIO_7_AL},       
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm963138ref_lte},
   {bp_last}                                                    
};                                                              

static bp_elem_t g_voice_tch_vanth[] = {
   {bp_cpBoardId,               .u.cp = "VANT-H"},
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_1},
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_2},
   {bp_usFxsFxo3SpiSSNum,       .u.us = SPI_DEV_4},
   {bp_usGpioFxsFxoRst1,            .u.us = BP_GPIO_7_AL},
   {bp_usGpioFxsFxoRst2,            .u.us = BP_GPIO_7_AL},
   {bp_usGpioFxsFxoRst3,            .u.us = BP_GPIO_27_AL},
   {bp_usGpioDectRst,               .u.us = BP_GPIO_3_AL},
   {bp_usGpioVoipRelayCtrl1,        .u.us = BP_GPIO_25_AL},
   {bp_usGpioVoipRelayCtrl2,        .u.us = BP_GPIO_24_AL},
   {bp_iDectCfg,                .u.ptr = g_iDectStdCfg},
   {bp_daughterCardList,        .u.ptr = g_vanth_dCardList},
   {bp_last}
};

static bp_elem_t g_voice_tch_vantw[] = {
   {bp_cpBoardId,               .u.cp = "VANT-W"},
   {bp_daughterCardList,        .u.ptr = g_vantw_dCardList},
   {bp_elemTemplate,            .u.bp_elemp = g_voice_tch_vanth},
   {bp_last}
};

static bp_elem_t g_voice_tch_vantv[] = {
   {bp_cpBoardId,               .u.cp = "VANT-V"},
   {bp_daughterCardList,        .u.ptr = g_vantv_dCardList},
   {bp_elemTemplate,            .u.bp_elemp = g_voice_tch_vanth},
   {bp_last}
};

static bp_elem_t g_voice_tch_vbntf[] = {
   {bp_cpBoardId,               .u.cp = "VBNT-F"},
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_1},
   {bp_usGpioFxsFxoRst1,        .u.us = BP_GPIO_7_AL},
   {bp_daughterCardList,        .u.ptr = g_vbntf_dCardList},
   {bp_last}
};

static bp_elem_t g_voice_tch_vbnth[] = {
   {bp_cpBoardId,               .u.cp = "VBNT-H"},
   {bp_daughterCardList,        .u.ptr = g_vbnth_dCardList},
   {bp_elemTemplate,            .u.bp_elemp = g_voice_tch_vanth},
   {bp_last}
};

static bp_elem_t g_voice_tch_vbntk[] = {
   {bp_cpBoardId,               .u.cp = "VBNT-K"},
   {bp_daughterCardList,        .u.ptr = g_vbntk_dCardList},
   {bp_elemTemplate,            .u.bp_elemp = g_voice_tch_vbntf},
   {bp_last}
};

static bp_elem_t g_voice_tch_vbntk_sfp[] = {
   {bp_cpBoardId,               .u.cp = "VBNT-K_SFP"},
   {bp_daughterCardList,        .u.ptr = g_vbntk_dCardList},
   {bp_elemTemplate,            .u.bp_elemp = g_voice_tch_vbntk},
   {bp_last}
};

static bp_elem_t * g_VoiceBoardParms[]=
{
   g_voice_bcm963138dvt,
   g_voice_bcm963138dvt_p300,
   g_voice_bcm963138ref_bmu,
   g_voice_bcm963138bmu_p202,
   g_voice_bcm963138ref,
   g_voice_bcm963138ref_p402,
   g_voice_bcm963138ref_p502,
   g_voice_bcm963138ref_lte,
   g_voice_bcm963138lte_p302,
   g_voice_bcm963138ref_rnc,
   g_voice_bcm963138ref_gfast_p40x,
   g_voice_tch_vanth,
   g_voice_tch_vantw,
   g_voice_tch_vantv,
   g_voice_tch_vbntf,
   g_voice_tch_vbnth,
   g_voice_tch_vbntk,
   g_voice_tch_vbntk_sfp,
   0
};

#endif


#if defined(_BCM963148_) || defined(CONFIG_BCM963148)

static VOICE_DAUGHTER_BOARD_PARMS * g_963148_dCardListFull[] = {
   &g_voiceBoard_NOSLIC,
   &g_voiceBoard_SI3217x,
   &g_voiceBoard_VE890_INVBOOST,
   &g_voiceBoard_LE88506,
   &g_voiceBoard_SI32267,
   &g_voiceBoard_LE88536_ZSI,
   &g_voiceBoard_SI32260,
   &g_voiceBoard_SI32260_SI3050,
   &g_voiceBoard_ZL88601,
   &g_voiceBoard_ZL88701,
   &g_voiceBoard_ZL88702_ZSI,
   &g_voiceBoard_ZL88801_89010_BB,
   &g_voiceBoard_LE9672_ZSI,
   0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_963148_dCardListFullNoFxoRst[] = {
   &g_voiceBoard_NOSLIC,
   &g_voiceBoard_SI3217x,
   &g_voiceBoard_VE890_INVBOOST,
   &g_voiceBoard_LE88506,
   &g_voiceBoard_SI32267,
   &g_voiceBoard_LE88536_ZSI,
   &g_voiceBoard_SI32260,
   &g_voiceBoard_SI32260_SI3050,
   &g_voiceBoard_SI32260_SI3050_QC,
   &g_voiceBoard_ZL88601,
   &g_voiceBoard_ZL88702_ZSI,
   &g_voiceBoard_LE9672_ZSI,
   0
};

static bp_elem_t g_voice_bcm963148dvt[] = {                 
   {bp_cpBoardId,               .u.cp = "963148DVT"},           
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_1},             
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_2},             
   {bp_usFxsFxo3SpiSSNum,       .u.us = SPI_DEV_4},             
   {bp_usGpioFxsFxoRst1,        .u.us = BP_GPIO_7_AL},          
   {bp_usGpioFxsFxoRst2,        .u.us = BP_GPIO_19_AL},          
   {bp_usGpioFxsFxoRst3,        .u.us = BP_GPIO_36_AL},          
   {bp_usGpioDectRst,           .u.us = BP_GPIO_37_AL},          
   {bp_usGpioVoipRelayCtrl1,    .u.us = BP_GPIO_6_AH},          
   {bp_iDectCfg,                .u.ptr = g_iDectStdCfg},        
   {bp_daughterCardList,        .u.ptr = g_963148_dCardListFull},
   {bp_last}                                                    
};                                                              

static bp_elem_t g_voice_bcm963148dvt_p300[] = {                 
   {bp_cpBoardId,               .u.cp = "963148DVT_P300"},           
   {bp_usGpioFxsFxoRst3,        .u.us = BP_GPIO_11_AL},          
   {bp_usGpioDectRst,           .u.us = BP_GPIO_12_AL},          
   {bp_usGpioVoipRelayCtrl1,    .u.us = BP_GPIO_26_AH}, 
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm963148dvt},              
   {bp_last}                                                    
};                                                                   

static bp_elem_t g_voice_bcm963148sv[] = {                 
   {bp_cpBoardId,               .u.cp = "963148SV"},           
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm963148dvt},
   {bp_last}                                                    
};                                                              
                                                                
static bp_elem_t g_voice_bcm963148ref[] = {                 
   {bp_cpBoardId,               .u.cp = "963148REF"},           
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_1},             
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_4},             
   {bp_usFxsFxo3SpiSSNum,       .u.us = SPI_DEV_5},             
   {bp_usGpioFxsFxoRst1,        .u.us = BP_GPIO_4_AL},          
   {bp_usGpioFxsFxoRst2,        .u.us = BP_GPIO_5_AL},          
   {bp_usGpioFxsFxoRst3,        .u.us = BP_GPIO_6_AL},          
   {bp_usGpioVoipRelayCtrl1,    .u.us = BP_GPIO_3_AH},          
   {bp_daughterCardList,        .u.ptr = g_963148_dCardListFull},
   {bp_last}                                                    
};                                                              

static bp_elem_t g_voice_bcm963148ref_bmu[] = {                 
   {bp_cpBoardId,               .u.cp = "963148REF_BMU"},           
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_1},             
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_2},             
   {bp_usFxsFxo3SpiSSNum,       .u.us = SPI_DEV_4},             
   {bp_usGpioFxsFxoRst1,        .u.us = BP_GPIO_60_AL},          
   {bp_usGpioFxsFxoRst2,        .u.us = BP_GPIO_61_AL},          
   {bp_usGpioDectRst,           .u.us = BP_GPIO_62_AL},          
   {bp_iDectCfg,                .u.ptr = g_iDectStdCfg},        
   {bp_daughterCardList,        .u.ptr = g_963148_dCardListFullNoFxoRst},
   {bp_last}                                                    
};                                                              
                                                                
/** TECHNICOLOR END   [MAM] Adding new profile for 63148 NAND boards **/
static VOICE_DAUGHTER_BOARD_PARMS * g_HG1X_dCardList[] = {
   &g_voiceBoard_NOSLIC,
   &g_voiceBoard_tch_GANT6,
   0
};

static bp_elem_t g_voice_tch_gant6[] = {
    {bp_cpBoardId,             .u.cp  = "GANT-6"},
    {bp_ulInterfaceEnable,     .u.ul  = BP_PINMUX_FNTYPE_HS_SPI},
    {bp_ulInterfaceEnable,     .u.ul  = BP_PINMUX_FNTYPE_PCM},
    {bp_usFxsFxo1SpiSSNum,     .u.us  = SPI_DEV_1},
    {bp_usFxsFxoRst1,          .u.us  = BP_GPIO_15_AL},
    {bp_daughterCardList,      .u.ptr = g_HG1X_dCardList},
    {bp_last}
};
/** TECHNICOLOR END   [MAM] Adding new profile for 63148 NAND boards **/

static bp_elem_t * g_VoiceBoardParms[]=
{
   g_voice_bcm963148dvt,
   g_voice_bcm963148dvt_p300,
   g_voice_bcm963148sv,
   g_voice_bcm963148ref,
   g_voice_bcm963148ref_bmu,
   g_voice_tch_gant6,
   0
};

#endif


#if defined(_BCM963381_) || defined(CONFIG_BCM963381)

static VOICE_DAUGHTER_BOARD_PARMS * g_963381ref1_dCardList[] = {
   &g_voiceBoard_NOSLIC,
   &g_voiceBoard_LE9662_ZSI_BB,
   &g_voiceBoard_LE9642_ZSI_BB,
   &g_voiceBoard_LE9661_ZSI,
   0
};

static bp_elem_t g_voice_bcm963381ref1[] = {                 
   {bp_cpBoardId,               .u.cp = "963381REF1"},           
   {bp_usGpioFxsFxoRst1,        .u.us = BP_GPIO_22_AL},          
   {bp_daughterCardList,        .u.ptr = g_963381ref1_dCardList},
   {bp_last}                                                    
};                                                              
                                                                
static bp_elem_t g_voice_bcm963381ref1_a0[] = {                 
   {bp_cpBoardId,               .u.cp = "963381REF1_A0"},           
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm963381ref1},
   {bp_last}                                                    
};                                                              
                                                                
static VOICE_DAUGHTER_BOARD_PARMS * g_963381dvt_dCardList[] = {
   &g_voiceBoard_NOSLIC,
   &g_voiceBoard_LE88506,
   &g_voiceBoard_SI32267,
   &g_voiceBoard_LE88536_ZSI,
   &g_voiceBoard_SI32260,
   &g_voiceBoard_ZL88601,
   &g_voiceBoard_LE9662_ZSI_BB,
   &g_voiceBoard_LE9642_ZSI_BB,
   &g_voiceBoard_LE9661_ZSI,
   &g_voiceBoard_ZL88702_ZSI,
   &g_voiceBoard_LE9672_ZSI,
   0
};

static bp_elem_t g_voice_bcm963381dvt[] = {                 
   {bp_cpBoardId,               .u.cp = "963381DVT"},           
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_1},             
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_3},    // This is a workaround for the NC SPI_SS2 in REV03 DVT boards. Must             
//    {bp_usFxsFxo3SpiSSNum,       .u.us = SPI_DEV_3}, // short SPI_SS2/3 on motherboard to get any 2 chip cards to work.  
   {bp_usGpioFxsFxoRst1,        .u.us = BP_GPIO_22_AL},          
   {bp_usGpioFxsFxoRst3,        .u.us = BP_GPIO_14_AL},          
   {bp_usGpioVoipRelayCtrl1,    .u.us = BP_GPIO_23_AH},          
   {bp_daughterCardList,        .u.ptr = g_963381dvt_dCardList},
   {bp_last}                                                    
};                                                              
                                                                
static VOICE_DAUGHTER_BOARD_PARMS * g_963381sv_dCardList[] = {
   &g_voiceBoard_NOSLIC,
   &g_voiceBoard_SI3217x,
   &g_voiceBoard_VE890_INVBOOST,
   &g_voiceBoard_LE88506,
   &g_voiceBoard_SI32267,
   &g_voiceBoard_LE88536_ZSI,
   &g_voiceBoard_SI32260,
   &g_voiceBoard_SI32260_SI3050,
   &g_voiceBoard_ZL88601,
   &g_voiceBoard_ZL88702_ZSI,
   &g_voiceBoard_LE9662_ZSI_BB,
   &g_voiceBoard_LE9642_ZSI_BB,
   &g_voiceBoard_LE9661_ZSI,
   &g_voiceBoard_LE9672_ZSI,
   0
};

static bp_elem_t g_voice_bcm963381sv[] = {                 
   {bp_cpBoardId,               .u.cp = "963381SV"},           
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_1},             
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_2},             
   {bp_usGpioFxsFxoRst1,        .u.us = BP_GPIO_22_AL},          
   {bp_usGpioFxsFxoRst2,        .u.us = BP_GPIO_23_AL},          
   {bp_daughterCardList,        .u.ptr = g_963381sv_dCardList},
   {bp_last}                                                    
};                                                              
                                                                

static VOICE_DAUGHTER_BOARD_PARMS * g_963381a_ref1_dCardList[] = {
   &g_voiceBoard_NOSLIC,
   &g_voiceBoard_SI32260_LCQC,
   0
};

static bp_elem_t g_voice_bcm963381a_ref1[] = {                 
   {bp_cpBoardId,               .u.cp = "963381A_REF1"},           
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_1},             
   {bp_usGpioFxsFxoRst1,        .u.us = BP_GPIO_22_AL},          
   {bp_daughterCardList,        .u.ptr = g_963381a_ref1_dCardList},
   {bp_last}                                                    
};                                                              
  
static bp_elem_t g_voice_bcm963381ref2[] = {                 
   {bp_cpBoardId,               .u.cp = "963381REF2"},           
   {bp_elemTemplate,            .u.bp_elemp = g_voice_bcm963381a_ref1},
   {bp_last}                                                    
};                                                              

static VOICE_DAUGHTER_BOARD_PARMS * g_963381ref3_dCardList[] = {
   &g_voiceBoard_NOSLIC,
   &g_voiceBoard_SI3217x,
   &g_voiceBoard_VE890_INVBOOST,
   &g_voiceBoard_LE88506,
   &g_voiceBoard_SI32267,
   &g_voiceBoard_LE88536_ZSI,
   &g_voiceBoard_SI32260,
   &g_voiceBoard_SI32260_SI3050,
   &g_voiceBoard_ZL88601,
   &g_voiceBoard_ZL88702_ZSI,
   &g_voiceBoard_LE9662_ZSI_BB,
   &g_voiceBoard_LE9642_ZSI_BB,
   &g_voiceBoard_LE9661_ZSI,
   &g_voiceBoard_LE9672_ZSI,
   0
};
                                                              
static bp_elem_t g_voice_bcm963381ref3[] = {                 
   {bp_cpBoardId,               .u.cp = "963381REF3"},           
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_1},             
   {bp_usFxsFxo2SpiSSNum,       .u.us = SPI_DEV_3},             
   {bp_usGpioFxsFxoRst1,        .u.us = BP_GPIO_22_AL},          
   {bp_daughterCardList,        .u.ptr = g_963381ref3_dCardList},
   {bp_last}                                                    
};                                                              

static VOICE_DAUGHTER_BOARD_PARMS * g_vantz_dCardList[] = {
   &g_voiceBoard_tch_vantz, 0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_vant1_dCardList[] = {
   &g_voiceBoard_tch_vant1, 0
};

static VOICE_DAUGHTER_BOARD_PARMS * g_vbnti_dCardList[] = {
   &g_voiceBoard_tch_vbnti, 0
};

static bp_elem_t g_voice_tch_vantz[] = {
   {bp_cpBoardId,               .u.cp = "VANT-Z"},
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_1},
   {bp_usGpioFxsFxoRst1,        .u.us = BP_GPIO_22_AL},
   {bp_daughterCardList,        .u.ptr = g_vantz_dCardList},
   {bp_last}
};

static bp_elem_t g_voice_tch_vant1[] = {
   {bp_cpBoardId,               .u.cp = "VANT-1"},
   {bp_daughterCardList,        .u.ptr = g_vant1_dCardList},
   {bp_elemTemplate,            .u.bp_elemp = g_voice_tch_vantz},
   {bp_last}
};

static bp_elem_t g_voice_tch_vbnti[] = {
   {bp_cpBoardId,               .u.cp = "VBNT-I"},
   {bp_usFxsFxo1SpiSSNum,       .u.us = SPI_DEV_1},
   {bp_usGpioFxsFxoRst1,        .u.us = BP_GPIO_22_AL},
   {bp_daughterCardList,        .u.ptr = g_vbnti_dCardList},
   {bp_last}
};

static bp_elem_t * g_VoiceBoardParms[]=
{
   g_voice_bcm963381ref1,
   g_voice_bcm963381ref1_a0,
   g_voice_bcm963381dvt,
   g_voice_bcm963381sv,
   g_voice_bcm963381a_ref1,
   g_voice_bcm963381ref2,
   g_voice_bcm963381ref3,
   g_voice_tch_vantz,
   g_voice_tch_vant1,
   g_voice_tch_vbnti,
   0
};

#endif

#if !defined(_BCM963268_) && !defined(CONFIG_BCM963268) && !defined(_BCM96362_) && !defined(CONFIG_BCM96362) && !defined(_BCM96328_) && !defined(CONFIG_BCM96328) && !defined(_BCM96838_) && !defined(CONFIG_BCM96838) && !defined(_BCM963138_) && !defined(CONFIG_BCM963138) && !defined(_BCM963381_) && !defined(CONFIG_BCM963381) && !defined(_BCM963148_) && !defined(CONFIG_BCM963148)

static bp_elem_t * g_VoiceBoardParms[]=
{
   0
};

#endif
/* Voice Boardparams End */


/*****************************************************************************
*
*  String and memory manipulation private functions
*
******************************************************************************/
static void bpmemcpy( void* dstptr, const void* srcptr, int size )
{
   char* dstp = dstptr;
   const char* srcp = srcptr;
   int i;
   for( i=0; i < size; i++ )
   {
      *dstp++ = *srcp++;
   }
}

static char * bpstrcpy( char* dest, const char* src)
{
   while(*src)
   {
      *dest++ = *src++;
   }

   *dest = '\0';

   return dest;
}

#if !defined(_CFE_)
static int bpstrlen( char * src )
{
   char *s;

	for(s = src; (s != 0) && (*s != 0); ++s);
	
	return(s - src);
}
#endif /* !defined(_CFE_) */
   
#if !defined(_CFE_)
/*****************************************************************************
*
*  voice Daughtercard type to BoardParam Type mapping functions
*
******************************************************************************/
static enum bp_id mapDcRstPinToBpType( BP_RESET_PIN rstPin )
{
   return( bp_usGpioFxsFxoRst1 + (enum bp_id)( rstPin - BP_RESET_FXS1 ) );
}

static enum bp_id mapDcSpiDevIdToBpType( BP_SPI_SIGNAL spiId )
{
   return( bp_usFxsFxo1SpiSSNum + (enum bp_id)( spiId - BP_SPI_SS_B1 ) );
}
#endif /* !defined(_CFE_) */

#if !defined(_CFE_)
/*****************************************************************************
 * Name:          BpGetZSISpiDevID()
 *
 * Description:     This function returns the SPI Device ID for the ZSI daughter
*                   boards based on the current chip.
 *
 * Parameters:    Nothing
 *
 * Returns:       SPI Dev ID for ZSI Daughter Boards
 *
 *****************************************************************************/
static unsigned int BpGetZSISpiDevID( void )
{
#ifdef ZSI_SPI_DEV_ID
   return ZSI_SPI_DEV_ID;
#else
   return BP_NOT_DEFINED;
#endif
}
#endif /* !defined(_CFE_) */


/*****************************************************************************
 * Name:          BpSetDectPopulatedData()
 *
 * Description:     This function sets the g_BpDectPopulated variable. It is
 *                used for the user to specify in the board parameters if the
 *                board DECT is populated or not (1 for populated, 0 for not).
 *
 * Parameters:    int BpData - The data that g_BpDectPopulated will be set to.
 *
 * Returns:       Nothing
 *
 *****************************************************************************/
void BpSetDectPopulatedData( int BpData )
{
   g_BpDectPopulated = BpData;
}

/*****************************************************************************
 * Name: 	      BpDectPopulated()
 *
 * Description:	  This function is used to determine if DECT is populated on
 * 				  the board.
 *
 * Parameters:    None
 *
 * Returns:       BP_DECT_POPULATED if DECT is populated, otherwise it will
 *                return BP_DECT_NOT_POPULATED.
 *
 *****************************************************************************/
int BpDectPopulated( void )
{
   return (g_BpDectPopulated ? BP_DECT_POPULATED:BP_DECT_NOT_POPULATED);
}

#if !defined(_CFE_)
/*****************************************************************************
 * Name:          BpGetVoiceParms()
 *
 * Description:     Finds the voice parameters based on the daughter board and
 *                base board IDs and fills the old parameters structure with
 *                information.
 *
 * Parameters:    pszVoiceDaughterCardId - The daughter board ID that is being used.
 *                voiceParms - The old voice parameters structure that must be
 *                             filled with data from the new structure.
 *                pszBaseBoardId - The base board ID that is being used.
 *
 * Returns:       If the board is not found, returns BP_BOARD_ID_NOT_FOUND.
 *                If everything goes properly, returns BP_SUCCESS.
 *
 *****************************************************************************/
int BpGetVoiceParms( char* pszVoiceDaughterCardId, VOICE_BOARD_PARMS* voiceParms, char* pszBaseBoardId )
{
   int nRet = BP_BOARD_ID_NOT_FOUND;
   int i = 0;
   int nDeviceCount = 0;
   bp_elem_t * pBpStartElem;
   PVOICE_DAUGHTER_BOARD_PARMS *ppDc;
   BP_VOICE_CHANNEL * dectChanCfg = 0;   
   VOICE_BOARD_PARMS currentVoiceParms;
   
   /* Get start element of voice board params structure */
   if( !(pBpStartElem = BpGetVoiceBoardStartElemPtr(pszBaseBoardId)) )
   {
      /* No matching base board found */
      return nRet;     
   }

   /* Get dectcfg pointer */
   if( BpDectPopulated() == BP_DECT_POPULATED )
   {
      dectChanCfg = (BP_VOICE_CHANNEL *)BpGetSubPtr(bp_iDectCfg, pBpStartElem, bp_last); 
   }
      
   /* Get daughtercard list pointer */
   ppDc = (PVOICE_DAUGHTER_BOARD_PARMS *)BpGetSubPtr(bp_daughterCardList, pBpStartElem, bp_last);     
   if( !ppDc ) 
   {
      /* No matching daughtercard list was found */
      return nRet;      
   }
   
   /* Iterate through daughter card list */
   for(; *ppDc; ppDc++)
   {
      if( (0 == bpstrcmp((*ppDc)->szBoardId, pszVoiceDaughterCardId)))
      {
         nRet = BP_SUCCESS;
         
         /* Succesfully found base board + daughter card combination
          * Must now fill the currentVoiceParms structure with data and copy to voiceParms
          * First set base board and daughter board strings */
         bpmemcpy(currentVoiceParms.szBoardId, pszVoiceDaughterCardId, bpstrlen(pszVoiceDaughterCardId));
         bpmemcpy(currentVoiceParms.szBaseBoardId, pszBaseBoardId, bpstrlen(pszBaseBoardId));

         currentVoiceParms.apmChannelSwap = BpGetSubUs(bp_usVoipApmChSwap, pBpStartElem, bp_last);

         /* Set the FXS and FXO line numbers. */
         currentVoiceParms.numFxsLines = (*ppDc)->numFxsLines;
         currentVoiceParms.numFxoLines = (*ppDc)->numFxoLines;

         /* Set the number of DECT Lines. */  
         currentVoiceParms.numDectLines = 0;
         if( dectChanCfg )
         {  
            for( ; dectChanCfg[currentVoiceParms.numDectLines].status != BP_VOICE_CHANNEL_NONE; 
                 currentVoiceParms.numDectLines++)
            { }                            
         }
         
         /*This prevents the total number of channels from being greater than 7. */
         if(currentVoiceParms.numFxsLines + currentVoiceParms.numFxoLines + currentVoiceParms.numDectLines > 7)
         {
            if(currentVoiceParms.numDectLines == 4)
            {
               /* If there are four DECT lines and it is exceeding limit, can
                * cut two of the DECT lines for board/daughter card combinations
                * with 4 FXS lines such as 963268V30A with Si32260x2.*/
               currentVoiceParms.numDectLines = 2;
            }
            else
            {
               return BP_MAX_CHANNELS_EXCEEDED; /* Return a failure. */
            }
         }

         /*Set the relay GPIO pins*/
         currentVoiceParms.pstnRelayCtrl.relayGpio[0] = BpGetSubUs(bp_usGpioVoipRelayCtrl1, pBpStartElem, bp_last);
         currentVoiceParms.pstnRelayCtrl.relayGpio[1] = BpGetSubUs(bp_usGpioVoipRelayCtrl2, pBpStartElem, bp_last);
         for( i = 0, currentVoiceParms.numFailoverRelayPins = 0; i < BP_MAX_RELAY_PINS; i++ )             
         {
            if(currentVoiceParms.pstnRelayCtrl.relayGpio[i] != BP_NOT_DEFINED)
            {
               currentVoiceParms.numFailoverRelayPins++;
            }
         }

         /*Set DECT UART to Not Defined always for now. */
         currentVoiceParms.dectUartControl.dectUartGpioTx = BP_NOT_DEFINED;
         currentVoiceParms.dectUartControl.dectUartGpioRx = BP_NOT_DEFINED;

         /* Set the device profile */
         currentVoiceParms.deviceProfile = (*ppDc)->deviceProfile;
#ifdef CONFIG_MMPBX_API_PATCH
          /*Set the flag to start slic detection*/
         currentVoiceParms.detectSlic = (*ppDc)->detectSlic;
#endif
         /*Set the flags*/
         currentVoiceParms.flags = (*ppDc)->flags;

         /*Set DECT*/
         if(currentVoiceParms.numDectLines)
         {
            currentVoiceParms.voiceDevice[nDeviceCount].voiceDeviceType = 
                  currentVoiceParms.voiceDevice[nDeviceCount+1].voiceDeviceType = BP_VD_IDECT1;
            currentVoiceParms.voiceDevice[nDeviceCount].spiCtrl.spiDevId = 
                  currentVoiceParms.voiceDevice[nDeviceCount+1].spiCtrl.spiDevId = 0;
            currentVoiceParms.voiceDevice[nDeviceCount].spiCtrl.spiGpio = 
                  currentVoiceParms.voiceDevice[nDeviceCount+1].spiCtrl.spiGpio = BP_NOT_DEFINED;
            currentVoiceParms.voiceDevice[nDeviceCount].requiresReset = 
                  currentVoiceParms.voiceDevice[nDeviceCount+1].requiresReset = 1;
            currentVoiceParms.voiceDevice[nDeviceCount].resetGpio = 
                  currentVoiceParms.voiceDevice[nDeviceCount+1].resetGpio = BpGetSubUs(bp_usGpioDectRst, pBpStartElem, bp_last);

            switch(currentVoiceParms.numDectLines)
            {
               case 1:
               case 2:
               {
                  bpmemcpy(&currentVoiceParms.voiceDevice[nDeviceCount].channel[nDeviceCount],
                           &dectChanCfg[0],
                           sizeof(BP_VOICE_CHANNEL)*currentVoiceParms.numDectLines);
                  nDeviceCount++;
               }
               break;

               case 3:
               case 4:
               {
                  bpmemcpy(&currentVoiceParms.voiceDevice[nDeviceCount].channel[nDeviceCount], 
                           &dectChanCfg[0], sizeof(BP_VOICE_CHANNEL)*2);
                  bpmemcpy(&currentVoiceParms.voiceDevice[nDeviceCount+1].channel[nDeviceCount],
                           &dectChanCfg[2],sizeof(BP_VOICE_CHANNEL)*2);
                  nDeviceCount+=2;
               }
               break;
               default:
               {
                  /* Return a failure */
                  return BP_MAX_CHANNELS_EXCEEDED;
               }
               break;
            }
         }

         for( i = 0; (i < BP_MAX_VOICE_DEVICES && (*ppDc)->voiceDevice[i].nDeviceType != BP_VD_NONE); i++ )
         {
            /* Loop through the voice devices and copy to currentVoiceParms */

            currentVoiceParms.voiceDevice[nDeviceCount].voiceDeviceType = (*ppDc)->voiceDevice[i].nDeviceType;

            if( (*ppDc)->voiceDevice[i].nRstPin == BP_RESET_NOT_REQUIRED )                
            {
               currentVoiceParms.voiceDevice[nDeviceCount].requiresReset = 0;
               currentVoiceParms.voiceDevice[nDeviceCount].resetGpio = BP_NOT_DEFINED;
            }
            else
            {
               currentVoiceParms.voiceDevice[nDeviceCount].requiresReset = 1;
               
               /* Retrieve the Reset GPIO */
               if( BpGetSubUs( mapDcRstPinToBpType((*ppDc)->voiceDevice[i].nRstPin), pBpStartElem, bp_last ) != BP_NOT_DEFINED )
               {
                  currentVoiceParms.voiceDevice[nDeviceCount].resetGpio = 
                                          BpGetSubUs( mapDcRstPinToBpType((*ppDc)->voiceDevice[i].nRstPin), pBpStartElem, bp_last );
               }
               else
               {
                  currentVoiceParms.voiceDevice[nDeviceCount].resetGpio = BP_NOT_DEFINED;
               }
            }

            /* Handle the ZSI/ISI devices */
            if((*ppDc)->voiceDevice[i].nSPI_SS_Bx == BP_SPI_SS_NOT_REQUIRED)
            {
               /* Current device is a ZSI/ISI device. */
               currentVoiceParms.voiceDevice[nDeviceCount].spiCtrl.spiDevId = BpGetZSISpiDevID();

               if(currentVoiceParms.voiceDevice[nDeviceCount].spiCtrl.spiDevId == BP_NOT_DEFINED)
               {
                  /* Failure - Tried to use a ZSI/ISI chip on a board which does not support it*/
                  return BP_NO_ZSI_ON_BOARD_ERR;
               }

               currentVoiceParms.voiceDevice[nDeviceCount].spiCtrl.spiGpio = BP_NOT_DEFINED;
            }
            else
            {
               /* Retrieve the system SPI device ID */
               if( BpGetSubUs(mapDcSpiDevIdToBpType((*ppDc)->voiceDevice[i].nSPI_SS_Bx), pBpStartElem, bp_last) != BP_NOT_DEFINED )
               {
                  /* Assign system SPI device ID */
                  currentVoiceParms.voiceDevice[nDeviceCount].spiCtrl.spiDevId = BpGetSubUs(mapDcSpiDevIdToBpType((*ppDc)->voiceDevice[i].nSPI_SS_Bx),
                                                                                 pBpStartElem, bp_last);                                                                                 
               }
               else
               {
                  /* SPI ID not defined */
                  currentVoiceParms.voiceDevice[nDeviceCount].spiCtrl.spiDevId = BP_NOT_DEFINED;                                                                                
               }
               
               /* Assign SPI associated GPIO pin */
               currentVoiceParms.voiceDevice[nDeviceCount].spiCtrl.spiGpio  = BpGetSlaveSelectGpioNum( currentVoiceParms.voiceDevice[nDeviceCount].spiCtrl.spiDevId );    
                                                                              
                                                                                         
            }

            /* Handle Le9530 and Si3239, which are internal devices */
            switch(currentVoiceParms.voiceDevice[nDeviceCount].voiceDeviceType)
            {
               case BP_VD_ZARLINK_9530:
               case BP_VD_ZARLINK_9540:
               case BP_VD_ZARLINK_9541:
               {
                  currentVoiceParms.voiceDevice[nDeviceCount].spiCtrl.spiDevId = 0;
                  currentVoiceParms.voiceDevice[nDeviceCount].spiCtrl.spiGpio = BP_NOT_DEFINED;
               }
               break;
               case BP_VD_SILABS_3239:
               case BP_VD_SILABS_32392:
               {
                  /* FIXME - Add SPI retrieval function for SI3239X in boardhal code */
                  currentVoiceParms.voiceDevice[nDeviceCount].spiCtrl.spiDevId = BpGetSubUs(bp_usSi32392SpiSSNum, pBpStartElem, bp_last);
                  currentVoiceParms.voiceDevice[nDeviceCount].spiCtrl.spiGpio = BpGetSlaveSelectGpioNum( currentVoiceParms.voiceDevice[nDeviceCount].spiCtrl.spiDevId );
               }
               default:
               break;
            }

            bpmemcpy(&currentVoiceParms.voiceDevice[nDeviceCount].channel[0], &((*ppDc)->voiceDevice[i].channel[0]), sizeof(BP_VOICE_CHANNEL)*2);

            nDeviceCount++;
         }

         /*Add a NULL Device*/
         currentVoiceParms.voiceDevice[nDeviceCount].voiceDeviceType=BP_VD_NONE;

         /* Copy over params */
         bpmemcpy( voiceParms, &currentVoiceParms, sizeof(VOICE_BOARD_PARMS) );
         nRet = BP_SUCCESS;

         break;
      }
   }

   return( nRet );
}
#endif /* !defined(_CFE_) */

/**************************************************************************
* Name       : BpSetVoiceBoardId
*
* Description: This function find the BOARD_PARAMETERS structure for the
*              specified board id string and assigns it to a global, static
*              variable.
*
* Parameters : [IN] pszVoiceDaughterCardId - Board id string that is saved into NVRAM.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_FOUND - Error, board id input string does not
*                  have a board parameters configuration record.
***************************************************************************/
int BpSetVoiceBoardId( char *pszVoiceDaughterCardId )
{
   bp_elem_t * pBpStartElem;
   int nRet = BP_BOARD_ID_NOT_FOUND;
   PVOICE_DAUGHTER_BOARD_PARMS *ppDc;
  
   /* Get start element of voice board params structure - Pass 0 to retrieve base board id in utility function */
   if( !(pBpStartElem = BpGetVoiceBoardStartElemPtr(0)) )
   {
      /* No matching base board found */
      return nRet;     
   }
      
   /* Get daughtercard list pointer */
   ppDc = (PVOICE_DAUGHTER_BOARD_PARMS *)BpGetSubPtr(bp_daughterCardList, pBpStartElem, bp_last);     
   if( !ppDc ) 
   {
      /* No matching daughtercard list was found */
      return nRet;      
   }
   
   /* Iterate through daughter card list */
   for(; *ppDc; ppDc++)
   {
      if( (0 == bpstrcmp((*ppDc)->szBoardId, pszVoiceDaughterCardId)))
      {
         bpmemcpy(voiceCurrentDgtrCardCfgId,pszVoiceDaughterCardId,BP_BOARD_ID_LEN);
         nRet = BP_SUCCESS;   
         return nRet;
      }
   }
   
   return( nRet );
} /* BpSetVoiceBoardId */


/**************************************************************************
* Name       : BpGetVoiceBoardId
*
* Description: This function returns the current board id strings.
*
* Parameters : [OUT] pszVoiceDaughterCardId - Address of a buffer that the board id
*                  string is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
***************************************************************************/
int BpGetVoiceBoardId( char *pszVoiceDaughterCardId )
{
   int i;

   if (0 == bpstrcmp(voiceCurrentDgtrCardCfgId, VOICE_BOARD_ID_DEFAULT))
   {
      return -1;
   }

   for (i = 0; i < BP_BOARD_ID_LEN; i++)
   {
      pszVoiceDaughterCardId[i] = voiceCurrentDgtrCardCfgId[i];
   }

   return 0;
}


/**************************************************************************
* Name       : BpGetVoiceBoardIds
*
* Description: This function returns all of the supported voice board id strings.
*
* Parameters : [OUT] pszVoiceDaughterCardIds - Address of a buffer that the board id
*                  strings are returned in.  Each id starts at BP_BOARD_ID_LEN
*                  boundary.
*              [IN] nBoardIdsSize - Number of BP_BOARD_ID_LEN elements that
*                  were allocated in pszBoardIds.
*              [IN] pszBaseBoardId - Name of base Board ID to associate Voice
*                  Board ID with.
*
* Returns    : Number of board id strings returned.
***************************************************************************/
int BpGetVoiceBoardIds( char *pszVoiceDaughterCardIds, int nBoardIdsSize, char *pszBaseBoardId )
{
   int count = 0;
   bp_elem_t * pBpStartElem;
   PVOICE_DAUGHTER_BOARD_PARMS *ppDc;
  
   /* Get start element of voice board params structure */
   if( !(pBpStartElem = BpGetVoiceBoardStartElemPtr(pszBaseBoardId)) )
   {
      /* No matching base board found */
      return 0;     
   }
      
   /* Get daughtercard list pointer */
   ppDc = (PVOICE_DAUGHTER_BOARD_PARMS *)BpGetSubPtr(bp_daughterCardList, pBpStartElem, bp_last);     
   if( !ppDc ) 
   {
      /* No matching daughtercard list was found */
      return count;      
   }
   
   /* Iterate through daughter card list */
   for(; (*ppDc != 0) && (*ppDc != 0) && (nBoardIdsSize != 0); ppDc++)
   {
      /* Copy over daughtercard Ids */
      bpstrcpy(pszVoiceDaughterCardIds, (*ppDc)->szBoardId);
      pszVoiceDaughterCardIds += BP_BOARD_ID_LEN;
      nBoardIdsSize--;
      count++;
   }
   
   return( count );
} /* BpGetVoiceBoardIds */


/**************************************************************************
* Name       : BpGetVoiceDectType
*
* Description: This function returns whether or not Dect is supported on a given board.
*
* Parameters : [IN] pszBaseBoardId - Name of the base Board ID
*
* Returns    : Status indicating if the base board supports dect.
***************************************************************************/
int BpGetVoiceDectType( char *pszBaseBoardId )
{
   bp_elem_t * pBpStartElem;
   BP_VOICE_CHANNEL * dectChanCfg = 0;   
   int dectStatus = BP_VOICE_NO_DECT;
  
   /* Get start element of voice board params structure */
   if( !(pBpStartElem = BpGetVoiceBoardStartElemPtr(pszBaseBoardId)) )
   {
      /* No matching base board found */
      return dectStatus;     
   }
   
   /* Get IDECT Cfg */
   dectChanCfg = (BP_VOICE_CHANNEL *)BpGetSubPtr(bp_iDectCfg, pBpStartElem, bp_last);    
   if( dectChanCfg )
   {
      dectStatus = BP_VOICE_INT_DECT;
   }

   return dectStatus;
}

/**************************************************************************
* Name       : BpGetVoiceBoardStartElemPtr
*
* Description: This function returns the start element of a voice board params stucture
*              when given a baseboard id. If base boardid is not specified then the 
*              currently active base board id is retreived and used
*
* Parameters : [IN] pszBaseBoardId - Name of the base Board ID
*
* Returns    : Start element of matching voice boardparams structure.
***************************************************************************/
static bp_elem_t * BpGetVoiceBoardStartElemPtr( char * pszBaseBoardId )
{
   int bpPtrIndex;
   char * baseBoardId;
   char boardIdStr[BP_BOARD_ID_LEN];
  
   /* Get Base board Id if not specified*/
   if( !pszBaseBoardId )
   {
      if ( BpGetBoardId(boardIdStr) != BP_SUCCESS )
      {
         /* No matching base board found */
         return 0;      
      }
   }
   else
   {
      /* Copy over specified base board id */
      bpstrcpy(boardIdStr, pszBaseBoardId);
   }

   /* Iterate through list of voice board params to find matching structure to base board*/
   for( bpPtrIndex=0; g_VoiceBoardParms[bpPtrIndex]; bpPtrIndex++ )
   {
      baseBoardId = BpGetSubCp(bp_cpBoardId, g_VoiceBoardParms[bpPtrIndex], bp_last);
      if( baseBoardId && (0 == bpstrcmp(baseBoardId, boardIdStr)) )
      {
         /* Found the matching board */
         break;
      }      
   }   
   
   return g_VoiceBoardParms[bpPtrIndex];      
}

#if !defined(_CFE_)
/**************************************************************************
* Name       : BpGetSlaveSelectGpioNum
*
* Description: This function returns the gpio number associated with a particular
*              SPI slave select
*
* Parameters : ssNum - Slave select ID
*
* Returns    : Start element of matching voice boardparams structure.
***************************************************************************/
static unsigned short BpGetSlaveSelectGpioNum( BP_SPI_PORT ssNum)
{
   bp_elem_t * pElem;
   
   for( pElem = g_pCurrentBp; pElem && (pElem->id != bp_last); pElem++ ) 
   {
      /* check for spi slave select definition.bp_usSpiSlaveSelectNum must be follwed by bp_usSpiSlaveSelectGpioNum */ 
      if( pElem->id == bp_usSpiSlaveSelectNum )
      {
         /* If ssNum matches, retrieve gpio num */
         if( pElem->u.us == (unsigned short)ssNum )
         {
            pElem++;
            if( (pElem->id != bp_last) && (pElem->id == bp_usSpiSlaveSelectGpioNum) )
            {
               /* Return active low for compatibility with legacy code */
               return (pElem->u.us|BP_ACTIVE_LOW);
            }
         }
      }
      
      /* Assign parent bp if pointer present */
      if( pElem->id == bp_elemTemplate )
      {
         pElem = pElem->u.bp_elemp;
      }                         
   }
   return BP_NOT_DEFINED;           
}
#endif /* !defined(_CFE_) */

/**************************************************************************
* Name       : BpGetVoicePmuxBp
*
* Description: This function returns a filtered version of the voice board params
*              based on the daughter card that is configured. The last element
*              in the filtered boardparams struct is a pointer to the passed-in 
*              data side board params struct
*
* Parameters : pCurrentDataBp - pointer to current data boardparams
*
* Returns    : filtered voice boardparams.
***************************************************************************/
bp_elem_t * BpGetVoicePmuxBp( bp_elem_t * pCurrentDataBp )
{
#if VOICE_PINMUX_RETRIEVAL
   int i = 0;
   int bSi3239x = 0;
   int bLe954x = 0; 
   int bHspi = 0;  
   bp_elem_t * pBpStartElem;
   bp_elem_t * pElem;
   PVOICE_DAUGHTER_BOARD_PARMS *ppDc;
   bp_elem_t * pNewElem;
   BP_VOICE_CHANNEL * dectChanCfg = 0;   
   
   if( !pCurrentDataBp )
   {
      //printk("Failed %d\n", __LINE__ );
      return 0;
   }
   
   /* Get base board ID */
   if( pCurrentDataBp[0].id != bp_cpBoardId )
   {      
      //printk("Failed %d\n", __LINE__ );
      return 0;
   }

   /* Get start element of voice board params structure */
   if( pCurrentDataBp[0].u.cp && !(pBpStartElem = BpGetVoiceBoardStartElemPtr(pCurrentDataBp[0].u.cp)) )
   {
      /* No matching base board found */
      //printk("Failed %d\n", __LINE__ );
      return 0;     
   }
   
   /* Get dectcfg pointer */
   if( BpDectPopulated() == BP_DECT_POPULATED )
   {
      dectChanCfg = (BP_VOICE_CHANNEL *)BpGetSubPtr(bp_iDectCfg, pBpStartElem, bp_last); 
   }
      
   /* Get daughtercard list pointer */
   ppDc = (PVOICE_DAUGHTER_BOARD_PARMS *)BpGetSubPtr(bp_daughterCardList, pBpStartElem, bp_last);     
   if( !ppDc ) 
   {
      /* No matching daughtercard list was found */
      //printk("Failed %d\n", __LINE__ );
      return 0;      
   }   
   
   /* Iterate through daughter card list */
   for(; *ppDc; ppDc++)
   {
      /* If matching voice board is found, break out */
      if( (0 == bpstrcmp((*ppDc)->szBoardId, &voiceCurrentDgtrCardCfgId[0])))
      {
         break;
      }
   }

   /* Return if no dect && ( no dc match || dc match with zero lines ) */
   if( !dectChanCfg && ( !(*ppDc) || ( (*ppDc) && !( (*ppDc)->numFxsLines + (*ppDc)->numFxoLines ) ) ) )
   {
      /* No voice lines configured */
      return 0;
   }

   /* Initialize filtered list and assign to pointer */
   for( i=0; i< FILTERED_BP_MAX_SIZE; i++ )
   {
      g_voice_filteredBp[i].id = bp_last;   
   }
   pNewElem = &g_voice_filteredBp[0];

   /* 1 - Configure FXS/FXO related interface enables */
   if( (*ppDc) && ( (*ppDc)->numFxsLines + (*ppDc)->numFxoLines ) )
   {
      if( (*ppDc)->flags & BP_FLAG_DSP_APMHAL_ENABLE )
      {
         /* Enable APM interface */
         BP_VOICE_ADD_INTERFACE_PINMUX( pNewElem, BP_PINMUX_FNTYPE_APM );
      }
 
      if( (*ppDc)->flags & BP_FLAG_DSP_PCMHAL_ENABLE )
      {
         /* Enable PCM interface */
         BP_VOICE_ADD_INTERFACE_PINMUX( pNewElem, BP_PINMUX_FNTYPE_PCM );
      }
                    
      /* Iterate through devices to determine device type */
      for( i=0; (i < BP_MAX_VOICE_DEVICES) && ((*ppDc)->voiceDevice[i].nDeviceType != BP_VD_NONE); i++ )
      {
         switch( (*ppDc)->voiceDevice[i].nDeviceType )
         {
            case BP_VD_SILABS_3239:
            case BP_VD_SILABS_32392:
            {
               bSi3239x = 1;
               
               if( !bHspi )
               {
                  /* Enable HS_SPI interface */
                  BP_VOICE_ADD_INTERFACE_PINMUX( pNewElem, BP_PINMUX_FNTYPE_HS_SPI );
                  bHspi = 1;
               }
            }
            break;
            
            case BP_VD_ZARLINK_9540:
            case BP_VD_ZARLINK_9541:
            {
               bLe954x = 1;
            }
            break;            
            
            default:
            {
               /* PCM SLACs */
               if( (*ppDc)->voiceDevice[i].nSPI_SS_Bx != BP_SPI_SS_NOT_REQUIRED )                                             
               {         
                  if( !bHspi )
                  {
                     /* Enable SPI for PCM SLACS only if no ISI/ZSI && no APM SLICs configured */
                     BP_VOICE_ADD_INTERFACE_PINMUX( pNewElem, BP_PINMUX_FNTYPE_HS_SPI );
                     bHspi = 1;
                  }
               }                              
            }
            break;
         }                                         
      }
   }
   
   /* 2 - Configure DECT related interface enable */
   if( dectChanCfg )
   {
      /* Enable DECT interface */
      BP_VOICE_ADD_INTERFACE_PINMUX( pNewElem, BP_PINMUX_FNTYPE_DECT );
   }

   /* 3 - Start adding signals based on interface enables and dc selection */
   pElem = pBpStartElem;
   while( pElem->id != bp_last )
   {
      switch ( pElem->id )
      {
         case bp_usGpioFxsFxoRst1: 
         case bp_usGpioFxsFxoRst2:         
         case bp_usGpioFxsFxoRst3:  
         {
            /* Check if PCM interface is enabled */
            if( BpIsIntfEnabled(BP_PINMUX_FNTYPE_PCM, &g_voice_filteredBp[0]) )
            {
               /* Only copy over elem if it doesnt already exist in filtered *
                * list - This accounts for sibling board overrides          */
               if( !BpElemExists( &g_voice_filteredBp[0], pElem->id  ) )            
               {
                  BP_VOICE_ADD_SIGNAL_PINMUX( pNewElem, pElem->id, pElem->u.us );
               }
            }
         }
         break;

         case bp_usGpioVoipRelayCtrl1:    
         case bp_usGpioVoipRelayCtrl2:    
         {
            /* Only copy over elem if it doesnt already exist in filtered *
             * list - This accounts for sibling board overrides          */
            if( !BpElemExists( &g_voice_filteredBp[0], pElem->id  ) )            
            {
               /* TODO: Maybe only add this for FXO enabled devices */
               BP_VOICE_ADD_SIGNAL_PINMUX( pNewElem, pElem->id, pElem->u.us );
            }
         }
         break;

         case bp_usGpioDectRst:           
         {
            /* Check if DECT interface is enabled */
            if( BpIsIntfEnabled(BP_PINMUX_FNTYPE_DECT, &g_voice_filteredBp[0]) )
            {
               /* Only copy over elem if it doesnt already exist in filtered *
                * list - This accounts for sibling board overrides          */
               if( !BpElemExists( &g_voice_filteredBp[0], pElem->id  ) )            
               {
                  BP_VOICE_ADD_SIGNAL_PINMUX( pNewElem, pElem->id, pElem->u.us );
               }
            }
         }
         break;

         case bp_usZarIfSclk: 
         case bp_usZarIfSdout: 
         case bp_usZarIfSdin: 
         case bp_usZarIfEnable: 
         case bp_usGpioLe9540Reset: 
         {
            /* Check if APM and Le954x are enabled */
            if( BpIsIntfEnabled(BP_PINMUX_FNTYPE_APM, &g_voice_filteredBp[0]) && bLe954x )
            {                           
               /* Only copy over elem if it doesnt already exist in filtered *
                * list - This accounts for sibling board overrides          */
               {
                  BP_VOICE_ADD_SIGNAL_PINMUX( pNewElem, pElem->id, pElem->u.us );
               }
            }
         }
         break;

         case bp_usHvgMaxPwm:
         case bp_usSi32392SpiSSNum:
         {
            /* Check if APM and Si3239x are enabled */
            if( BpIsIntfEnabled(BP_PINMUX_FNTYPE_APM, &g_voice_filteredBp[0]) && bSi3239x )
            {         
               /* Only copy over elem if it doesnt already exist in filtered *
                * list - This accounts for sibling board overrides          */
               if( !BpElemExists( &g_voice_filteredBp[0], pElem->id  ) )            
               {
                  BP_VOICE_ADD_SIGNAL_PINMUX( pNewElem, pElem->id, pElem->u.us );
               }
            }
         }
         break;

         default:
         {
         }
         break;
      }

      pElem++;
      
      /* Assign parent bp if pointer present */
      if( pElem->id == bp_elemTemplate )
      {
         pElem = pElem->u.bp_elemp;
      }
   }

   /* 4 - Add pointer to data bp at the end of voice bp */
   pNewElem->id = bp_elemTemplate;
   pNewElem++->u.bp_elemp = pCurrentDataBp;
   pNewElem->id = bp_last;
   return ( &g_voice_filteredBp[0] );
#else
   return 0;   
#endif /* VOICE_PINMUX_RETRIEVAL */
}
    
#if VOICE_PINMUX_RETRIEVAL
/**************************************************************************
* Name       : BpIsIntfEnabled
*
* Description: Checks if a particular interface is enabled in boardparms
*
* Parameters : interfaceFlag - Flag representing interface
*              pBoardParms   - pointer to boardparams
*
* Returns    : 1 if found, 0 otherwise
***************************************************************************/  
static int BpIsIntfEnabled( unsigned int interfaceFlag, bp_elem_t * pBoardParms )
{
   int retVal = 0;
   
   if( pBoardParms )
   {
      while( pBoardParms->id != bp_last )
      {
         if( (pBoardParms->id == bp_ulInterfaceEnable) && (pBoardParms->u.ul == interfaceFlag) )
         {
            retVal = 1;
            break;
         }
         pBoardParms++;
      }
   }
   
   //printk("%s: flag:0x%x enabled:%d\n", __FUNCTION__, interfaceFlag, retVal );
   return retVal;                        
}

/**************************************************************************
* Name       : BpElemExists
*
* Description: Checks if element with specific id already exists in bp
*
* Parameters : pBoardParms - pointer to boardparams
*              id   - element id to look for
*
* Returns    : 1 if found, 0 otherwise
***************************************************************************/  
static int BpElemExists( bp_elem_t * pBoardParms, enum bp_id  id )
{
   int retVal=0;
   bp_elem_t * tempPtr = pBoardParms;
   bp_elem_t * pFoundElem;
   
   pFoundElem = BpGetElem(id, &tempPtr, bp_last);   
   if (id == pFoundElem->id) 
   {      
      retVal = 1;
   }
   
   //printk("%s: id:%d exists:%d\n", __FUNCTION__, id, retVal );
   return ( retVal );
}

#endif /*VOICE_PINMUX_RETRIEVAL*/
