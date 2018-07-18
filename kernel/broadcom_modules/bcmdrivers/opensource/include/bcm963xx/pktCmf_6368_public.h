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


#ifndef __PKTCMF_6368_PUBLIC_H_INCLUDED__
#define __PKTCMF_6368_PUBLIC_H_INCLUDED__

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


/* Explicit wrappers to callback */
extern int pktCmfSwcConfig(void);
extern int pktCmfSarConfig(int ulHwFwdTxChannel, unsigned int ulTrafficType);
extern int pktCmfSarAbort(void);


/*
 *------------------------------------------------------------------------------
 *                  Enet Switch Driver Hooks
 *  Ethernet switch driver will register the appropriate handlers to
 *  configure the SAR Port of the Switch. CMF will invoke these handlers when
 *  the SAR runtime driver becomes operational and wishes to use CMF for
 *  downstream hardware accelerated forwarding via the SAR port of the Switch.
 *------------------------------------------------------------------------------
 */
extern HOOKV pktCmfSarPortEnable;   /* Binding with Switch ENET */
extern HOOKV pktCmfSarPortDisable;  /* Binding with Switch ENET */

#define  PKTCMF_MAX_VPORTS  16

#endif /* __PKTCMF_6368_PUBLIC_H_INCLUDED__ */
