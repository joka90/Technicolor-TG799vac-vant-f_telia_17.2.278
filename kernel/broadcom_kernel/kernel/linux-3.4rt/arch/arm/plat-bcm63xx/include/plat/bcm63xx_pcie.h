/*
<:copyright-BRCM:2013:DUAL/GPL:standard

   Copyright (c) 2013 Broadcom Corporation
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

#ifndef __BCM63XX_PCIE_H
#define __BCM63XX_PCIE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/export.h>
#include <bcm_map_part.h>
#include <bcm_intr.h>
#include <board.h>
#include <pmc_pcie.h>
#include <pmc_drv.h>
#include <shared_utils.h>

#if 0
#define DPRINT(x...)                printk(x)
#define TRACE()                     DPRINT("%s\n",__FUNCTION__)
#define TRACE_READ(x...)            printk(x)
#define TRACE_WRITE(x...)           printk(x)
#else
#undef  DPRINT
#define DPRINT(x...)
#define TRACE()
#define TRACE_READ(x...)
#define TRACE_WRITE(x...)
#endif

/*PCI-E */
#define BCM_BUS_PCIE_ROOT           0
#if defined(PCIEH) && defined(PCIEH_1)
#define NUM_CORE                    2
#else
#define NUM_CORE                    1
#endif


/*
 * Per port control structure
 */
struct bcm63xx_pcie_port {
    unsigned char * __iomem regs;
    struct resource *owin_res;
    unsigned int irq;
    struct hw_pci hw_pci;

    bool enabled;                   // link-up
    bool link;                      // link-up
    bool saved;                     // pci-state saved
};

#ifdef __cplusplus
}
#endif

#endif /* __BCM63XX_PCIE_H */
