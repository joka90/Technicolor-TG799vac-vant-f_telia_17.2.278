 #ifndef __PKT_CMF_PUBLIC_H_INCLUDED__
#define __PKT_CMF_PUBLIC_H_INCLUDED__
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
#if defined(CONFIG_BCM96368) || defined(CHIP_6368)
#include "pktCmf_6368_public.h"
#endif

#if defined(CONFIG_BCM96816) || defined(CHIP_6816)
#include "pktCmf_6816_public.h"
#endif

#if defined(CONFIG_BCM96818) || defined(CHIP_6818)
#include "pktCmf_6816_public.h"
#endif

typedef struct {
    uint32_t rxDropped;
    uint32_t txDropped;
} PktCmfVportStats_t;

typedef struct {
    uint32_t vport;
    uint32_t *rxDropped_p;
    uint32_t *txDropped_p;
} PktCmfStatsParam_t;

#if defined(CONFIG_BCM_FAP_MODULE) || defined(CONFIG_BCM_FAP) || \
    defined(CONFIG_BCM_ARL_MODULE) || defined(CONFIG_BCM_ARL)

// No need for fap.h right now.  Should be fap_public.h anyways.
//#include "fap.h"
#endif

#endif  /* defined(__PKT_CMF_PUBLIC_H_INCLUDED__) */

