/*
<:copyright-BRCM:2010:DUAL/GPL:standard

   Copyright (c) 2010 Broadcom Corporation
   All Rights Reserved

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


#ifndef __PKTCMF_6816_PUBLIC_H_INCLUDED__
#define __PKTCMF_6816_PUBLIC_H_INCLUDED__

#include "pktHdr.h"


/*
 *------------------------------------------------------------------------------
 * Common defines for Packet CMF layers.
 *------------------------------------------------------------------------------
 */
#define CMF_DECL(x)                 #x, /* for string declaration in C file */
#undef CMF_DECL
#define CMF_DECL(x)                 x,  /* for enum declaration in H file */

/* Offsets and sizes in CMF are specified as half words */
#define CMFUNIT                     (sizeof(uint16_t))

#define CMP1BYTE                    (sizeof(uint8_t))
#define CMP2BYTES                   (sizeof(uint16_t))
#define CMP4BYTES                   (sizeof(uint32_t))

/* Creation of CMF NIBBLE MASK, b0=[0..3] b1=[4..7] b2=[8..11] b3=[12..15] */
#define NBLMSK1BYTE                 0x3
#define NBLMSK3NBLS                 0x7
#define NBLMSK2BYTES                0xF

/* Redefined from unexported header: ./broadcom/net/enet/impl2/bcmenet.h */
#define BRCM_TAG_ETH_TYPE           0x8874  /* BRCM_TYPE */
#define BRCM_TAG_LENGTH             6

#define PKTCMF_ALL_ENTRIES          (-1)


/*
 * Interface: Single callback entry point into Packet CMF subsystem
 * from CMF Control utility or other kernel modules.
 */
typedef enum {
    CMF_DECL(PKTCMF_IF_STATUS)
    CMF_DECL(PKTCMF_IF_RESET)
    CMF_DECL(PKTCMF_IF_INIT)
    CMF_DECL(PKTCMF_IF_ENABLE)
    CMF_DECL(PKTCMF_IF_DISABLE)
    CMF_DECL(PKTCMF_IF_PRE_SYSTEMRESET)
    CMF_DECL(PKTCMF_IF_POST_SYSTEMRESET)
    CMF_DECL(PKTCMF_IF_FLUSH)
    CMF_DECL(PKTCMF_IF_DEBUG)
    CMF_DECL(PKTCMF_IF_PRINT)
    CMF_DECL(PKTCMF_IF_UNITTEST)
    CMF_DECL(PKTCMF_IF_CONFIG)
    CMF_DECL(PKTCMF_IF_TRAFFIC)
    CMF_DECL(PKTCMF_IF_SET_ASPF)
    CMF_DECL(PKTCMF_IF_GET_ASPF)
    CMF_DECL(PKTCMF_IF_SET_TPID)
    CMF_DECL(PKTCMF_IF_GET_TPID)
    CMF_DECL(PKTCMF_IF_CMF_CFG_MISS)
    CMF_DECL(PKTCMF_IF_LABFLOW)
    CMF_DECL(PKTCMF_IF_FCBCTRL)
    CMF_DECL(PKTCMF_IF_SET_PADLEN)
    CMF_DECL(PKTCMF_IF_GET_PADLEN)
    CMF_DECL(PKTCMF_IF_SET_GBL_STATUS_RL)
    CMF_DECL(PKTCMF_IF_GET_GBL_STATUS_RL)
    CMF_DECL(PKTCMF_IF_SET_TCICFG_RL)
    CMF_DECL(PKTCMF_IF_GET_TCICFG_RL)
    CMF_DECL(PKTCMF_IF_CFG_RL)
    CMF_DECL(PKTCMF_IF_GET_RL)
} pktCmf_if_t;

extern int pktCmf_isEnabled(void);

extern int pktCmfIf( pktCmf_if_t callback, int *parg1, int *parg2 );


/*
 *------------------------------------------------------------------------------
 *                  Enet Switch Driver Hooks
 *  Ethernet switch driver will register the appropriate handlers to
 *  manage the Switch ports. CMF will invoke these handlers when CMF is
 *  initialized.
 *------------------------------------------------------------------------------
 */
extern HOOKV pktCmfSaveSwitchPortState;      /* Binding with Switch ENET */
extern HOOKV pktCmfRestoreSwitchPortState;   /* Binding with Switch ENET */
extern HOOKV pktCmfResetSwitch;              /* LAB trigger of reset_switch() */

#define  PKTCMF_MAX_VPORTS  8

#endif /* __PKTCMF_6816_PUBLIC_H_INCLUDED__ */
