/*
 * <:copyright-BRCM:2013:DUAL/GPL:standard
 *
 *    Copyright (c) 2013 Broadcom Corporation
 *    All Rights Reserved
 *
 * Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed
 * to you under the terms of the GNU General Public License version 2
 * (the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
 * with the following added to such license:
 *
 *    As a special exception, the copyright holders of this software give
 *    you permission to link this software with independent modules, and
 *    to copy and distribute the resulting executable under terms of your
 *    choice, provided that you also meet, for each linked independent
 *    module, the terms and conditions of the license of that module.
 *    An independent module is a module which is not derived from this
 *    software.  The special exception does not apply to any modifications
 *    of the software.
 *
 * Not withstanding the above, under no circumstances may you combine
 * this software in any way with any other Broadcom software provided
 * under a license other than the GPL, without Broadcom's express prior
 * written consent.
 *
 * :>
 */

// BCMFORMAT: notabs reindent:uncrustify:bcm_minimal_i4.cfg

#include "boardparms.h"

#ifdef _CFE_
#include "lib_types.h"
#include "lib_printf.h"
#include "lib_string.h"
#include "bcm_map.h"
#define printk  printf
#else // Linux
#include <linux/kernel.h>
#include <linux/module.h>
#include <bcm_map_part.h>
#include <linux/string.h>
#endif
static unsigned int read_ssbm_reg(unsigned int addr) __attribute__((unused));
static void write_ssbm_reg(unsigned int addr, unsigned int data, int force) __attribute__((unused));

/* map SWITCH/CROSSBAR port numbers to the order of xMII pas control
   registers in the device
 */

static int port2xmii[] = { -1, -1, -1, -1, -1, 1, -1, 2, -1, -1, -1, 3, 0 };
static void bcm_misc_hw_xmii_pads_init(void)
{
    const ETHERNET_MAC_INFO *Enet;
    int i,j;
    int u;
    int n;
    // Check for network ports requiring MAC interfaces to be active
    if ( (Enet = BpGetEthernetMacInfoArrayPtr()) != NULL)
    {
        for (i = 0 ; i < BP_MAX_ENET_MACS ; i++) {
            for (j = 0; j < BP_MAX_SWITCH_PORTS ; j++) {
                if ((Enet[i].sw.port_map >> j) & 0x01)  {
                    u = j;
                    n = port2xmii[u];
                    if (n >= 0) {
                        switch (Enet[i].sw.phy_id[u] & MAC_IFACE)
                        {
                        case MAC_IF_RGMII_1P8V :
                            MISC->miscxMIIPadCtrl[n] = MISC->miscxMIIPadCtrl[n] & ~MISC_XMII_PAD_MODEHV;
                            break;
                        case MAC_IF_RGMII_2P5V :
                            MISC->miscxMIIPadCtrl[n] = (MISC->miscxMIIPadCtrl[n] | MISC_XMII_PAD_MODEHV) & ~MISC_XMII_PAD_SEL_GMII;
                            break;
                        case MAC_IF_RGMII_3P3V :
                            MISC->miscxMIIPadCtrl[n] = MISC->miscxMIIPadCtrl[n] | MISC_XMII_PAD_MODEHV | MISC_XMII_PAD_SEL_GMII;
                            break;
                        }
                    }
                }
            }
            for (j = 0; j < BP_MAX_CROSSBAR_EXT_PORTS ; j++) {
                u = BP_CROSSBAR_PORT_TO_PHY_PORT(j);
                if (Enet[i].sw.crossbar[j].switch_port != BP_CROSSBAR_NOT_DEFINED)
                {
                    n = port2xmii[u];
                    if (n >= 0) {
                        switch (Enet[i].sw.crossbar[j].phy_id & MAC_IFACE)
                        {
                        case MAC_IF_RGMII_1P8V :
                            MISC->miscxMIIPadCtrl[n] = MISC->miscxMIIPadCtrl[n] & ~MISC_XMII_PAD_MODEHV;
                            break;
                        case MAC_IF_RGMII_2P5V :
                            MISC->miscxMIIPadCtrl[n] = (MISC->miscxMIIPadCtrl[n] | MISC_XMII_PAD_MODEHV) & ~MISC_XMII_PAD_SEL_GMII;
                            break;
                        case MAC_IF_RGMII_3P3V :
                            MISC->miscxMIIPadCtrl[n] = MISC->miscxMIIPadCtrl[n] | MISC_XMII_PAD_MODEHV | MISC_XMII_PAD_SEL_GMII;
                            break;
                        }
                    }
                }
            }
        }
    }
}

/* this function performs any customization to the voltage regulator setting */
static void bcm_misc_hw_vr_init(void)
{
    unsigned short bmuen;

    /* set vr drive strength for bmu board */
    if ((BpGetBatteryEnable(&bmuen) == BP_SUCCESS) && (bmuen != 0)) {
        write_ssbm_reg(0x00+7,0x12d9,1);
        write_ssbm_reg(0x40+7,0x12d9,1);
        write_ssbm_reg(0x20+7,0x12d9,1);
    }

    /* Set 1.0V, 1.5V and 1.8V  digital voltage switching regulator's gain setting  based on 
       JIRA SWBCACPE-18708 and SWBCACPE-18709 */
#if defined(_BCM963148_) || defined(CONFIG_BCM963148)
    write_ssbm_reg(0x0, 0x800, 1);
    write_ssbm_reg(0x20, 0x830, 1);
    write_ssbm_reg(0x40, 0x800, 1);
#endif

#if defined(_BCM963138_) || defined(CONFIG_BCM963138)
    write_ssbm_reg(0x0, 0x800, 1);
    write_ssbm_reg(0x20, 0x830, 1);
    write_ssbm_reg(0x40, 0x800, 1);
#endif
    return;
}

static unsigned int read_ssbm_reg(unsigned int addr) 
{
    PROCMON->SSBMaster.control = PMC_SSBM_CONTROL_SSB_ADPRE | 
                                (PMC_SSBM_CONTROL_SSB_CMD_READ << PMC_SSBM_CONTROL_SSB_CMD_SHIFT ) 
                                | (PMC_SSBM_CONTROL_SSB_ADDR_MASK & 
                                   (addr << PMC_SSBM_CONTROL_SSB_ADDR_SHIFT) );
    PROCMON->SSBMaster.control |= PMC_SSBM_CONTROL_SSB_EN;
    PROCMON->SSBMaster.control |= PMC_SSBM_CONTROL_SSB_START;
    while (PROCMON->SSBMaster.control & PMC_SSBM_CONTROL_SSB_START);
    return(PROCMON->SSBMaster.rd_data);
}

static void write_ssbm_reg(unsigned int addr, unsigned int data, int force) 
{
    PROCMON->SSBMaster.wr_data = data;
    PROCMON->SSBMaster.control = (PMC_SSBM_CONTROL_SSB_CMD_WRITE << PMC_SSBM_CONTROL_SSB_CMD_SHIFT ) 
                                | (PMC_SSBM_CONTROL_SSB_ADDR_MASK & 
                                   (addr << PMC_SSBM_CONTROL_SSB_ADDR_SHIFT) );
    PROCMON->SSBMaster.control |= PMC_SSBM_CONTROL_SSB_EN;
    PROCMON->SSBMaster.control |= PMC_SSBM_CONTROL_SSB_START;
    while (PROCMON->SSBMaster.control & PMC_SSBM_CONTROL_SSB_START);
    if (force != 0) {
        unsigned int reg0;
        reg0 = read_ssbm_reg(addr&0xfff0);
        write_ssbm_reg(addr&0xfff0, reg0|2, 0);
        write_ssbm_reg(addr&0xfff0, reg0&(~2), 0);
    }
    return;
}


void bcm_misc_hw_init(void)
{
    bcm_misc_hw_vr_init();
    bcm_misc_hw_xmii_pads_init();
}



#ifndef _CFE_
arch_initcall(bcm_misc_hw_init);

#endif

