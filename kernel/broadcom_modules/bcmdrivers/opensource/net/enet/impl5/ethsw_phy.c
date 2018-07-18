/*
 <:copyright-BRCM:2011:DUAL/GPL:standard

    Copyright (c) 2011 Broadcom Corporation
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
#define _BCMENET_LOCAL_
#include "bcm_map_part.h"
#include <board.h>
#include "bcm_OS_Deps.h"
#include "bcmmii.h"
#include "bcmsw.h"
#include "boardparms.h"
#include "bcmenet.h"
#include "bcmswaccess.h"
#include "ethsw_phy.h"

#if defined(SWITCH_REG_SINGLE_SERDES_CNTRL)
#include "pmc/pmc_drv.h"

#include <linux/proc_fs.h>
#define SFP_STATUS_PROC_NAME    "sfp_status"
#define CROSSBAR_STATUS_PROC_NAME    "crossbar_status"

static u16 serdesSet1GFiber [] =
{
    0x8000, 0x0c2f,
    0x8182, 0x4000,     /* This and next lines are for yield rate improvement */
    0x8186, 0x003c,
    0x8300, 0x015d,     /* Force Auto Detect */
    0x8301, 0x7,
    0x0,    0x1140,
    0x8000, 0x2c2f
};

static u16 serdesSet100MFiber [] =
{
    0x8000, 0x0c2f,
    0x8182, 0x4000,     /* This and next lines are for yield rate improvement */
    0x8186, 0x003c,
    0x8300, 0xd,
    0x8400, 0x14b,
    0x8402, 0x880,
    0x8000, 0x2c2f,
};

static u16 serdesSetSGMII [] =
{
    0x8000, 0x0c2f,
    0x8182, 0x4000,     /* This and next lines are for yield rate improvement */
    0x8186, 0x003c,
    0x8300, 0x0100,
    0x0,    0x1140,
    0x8000, 0x2c2f
};

static void config_serdes(int phyId, u16 seq[], int seqSize)
{
    int i;

    seqSize /= sizeof(seq[0]);
    for(i=0; i<seqSize; i+=2)
    {
        ethsw_phy_exp_wreg(phyId, seq[i], &seq[i+1]);
    }
}

static void ethsw_phy_autong_restart(int phyId)
{
    u16 val16;
    ethsw_phy_rreg(phyId, MII_CONTROL, &val16);
    val16 |= MII_CONTROL_RESTART_AUTONEG;
    ethsw_phy_wreg(phyId, MII_CONTROL, &val16);
}

void ethsw_phy_reset(int phyId)
{
    u16 v16;

    /* Reset PHY to clear status first */
    v16= MII_CONTROL_RESET;
    ethsw_phy_exp_wreg(phyId, MII_CONTROL, &v16);
    for(ethsw_phy_exp_rreg(phyId, MII_CONTROL, &v16); v16 & MII_CONTROL_RESET;
            ethsw_phy_exp_rreg(phyId, MII_CONTROL, &v16));
}

void ethsw_config_serdes_1kx(int phyId)
{
    ethsw_phy_reset(phyId);
    config_serdes(phyId, serdesSet1GFiber, sizeof(serdesSet1GFiber));
    ethsw_phy_autong_restart(phyId);
    msleep(50);
}

void ethsw_config_serdes_100fx(int phyId)
{
    ethsw_phy_reset(phyId);
    config_serdes(phyId, serdesSet100MFiber, sizeof(serdesSet100MFiber));
    msleep(80);
}

void ethsw_config_serdes_sgmii(int phyId)
{
    config_serdes(phyId, serdesSetSGMII, sizeof(serdesSetSGMII));
    ethsw_phy_autong_restart(phyId);
    msleep(50);
}


static void sgmiiResCal(int phyId)
{
    int val;
    u16 v16;

    v16 = RX_AFE_CTRL2_DIV4 | RX_AFE_CTRL2_DIV10;
    ethsw_phy_exp_wreg(phyId, RX_AFE_CTRL2, &v16);

    if(GetRCalSetting(RCAL_1UM_VERT, &val) != kPMC_NO_ERROR)
    {
        printk("AVS is not turned on, leave SGMII termination resistor values as current default\n");
        ethsw_phy_exp_rreg(phyId, PLL_AFE_CTRL1, &v16);
        printk("    PLL_PON: 0x%04x; ", (v16 & PLL_AFE_PLL_PON_MASK) >> PLL_AFE_PLL_PON_SHIFT);
        ethsw_phy_exp_rreg(phyId, TX_AFE_CTRL2, &v16);
        printk("TX_PON: 0x%04x; ", (v16 & TX_AFE_TX_PON_MASK) >> TX_AFE_TX_PON_SHIFT);
        ethsw_phy_exp_rreg(phyId, RX_AFE_CTRL0, &v16);
        printk("RX_PON: 0x%04x\n", (v16 & RX_AFE_RX_PON_MASK) >> RX_AFE_RX_PON_SHIFT);
        return;
    }

    val &= 0xf;
    printk("Setting SGMII Calibration value to 0x%x\n", val);

    ethsw_phy_exp_rreg(phyId, PLL_AFE_CTRL1, &v16);
    v16 = (v16 & (~PLL_AFE_PLL_PON_MASK)) | (val << PLL_AFE_PLL_PON_SHIFT);
    ethsw_phy_exp_wreg(phyId, PLL_AFE_CTRL1, &v16);

    ethsw_phy_exp_rreg(phyId, TX_AFE_CTRL2, &v16);
    v16 = (v16 & (~TX_AFE_TX_PON_MASK)) | (val << TX_AFE_TX_PON_SHIFT);
    ethsw_phy_exp_wreg(phyId, TX_AFE_CTRL2, &v16);

    ethsw_phy_exp_rreg(phyId, RX_AFE_CTRL0, &v16);
    v16 = (v16 & (~RX_AFE_RX_PON_MASK)) | (val << RX_AFE_RX_PON_SHIFT);
    ethsw_phy_exp_wreg(phyId, RX_AFE_CTRL0, &v16);
}

void ethsw_powerup_serdes(int powerLevel, int phyId)
{
    static int curPwrLvl=SERDES_POWER_DOWN;
    u32 val32;
    u16 val16;

    if (powerLevel == curPwrLvl)
        return;

    val32 = *(u32 *)SWITCH_REG_SINGLE_SERDES_CNTRL;
    switch(powerLevel)
    {
        case SERDES_POWER_ON:
            val32 |= SWITCH_REG_SERDES_RESETPLL|SWITCH_REG_SERDES_RESETMDIO|SWITCH_REG_SERDES_RESET;
            val32 &= ~(SWITCH_REG_SERDES_IQQD|SWITCH_REG_SERDES_PWRDWN);
            *(u32 *)SWITCH_REG_SINGLE_SERDES_CNTRL = val32;
            msleep(1);
            val32 &= ~(SWITCH_REG_SERDES_RESETPLL|SWITCH_REG_SERDES_RESETMDIO|SWITCH_REG_SERDES_RESET);
            *(u32 *)SWITCH_REG_SINGLE_SERDES_CNTRL = val32;

            /* Do dummy MDIO read to work around ASIC problem */
            ethsw_phy_rreg(phyId, 0, &val16);
            break;
        case SERDES_POWER_STANDBY:
            if (val32 & SWITCH_REG_SERDES_IQQD) {
                val32 |= SWITCH_REG_SERDES_PWRDWN;
                val32 &= ~SWITCH_REG_SERDES_IQQD;
                *(u32 *)SWITCH_REG_SINGLE_SERDES_CNTRL = val32;
                msleep(1);
            }
        case SERDES_POWER_DOWN:
            val32 |= SWITCH_REG_SERDES_PWRDWN|SWITCH_REG_SERDES_RESETPLL|
                    SWITCH_REG_SERDES_RESETMDIO|SWITCH_REG_SERDES_RESET;
            *(u32 *)SWITCH_REG_SINGLE_SERDES_CNTRL = val32;
            break;
        default:
            printk("Wrong power level request to Serdes module\n");
            return;
    }
    curPwrLvl = powerLevel;
}

static int proc_get_sfp_status(char *page, char **start, off_t off, int cnt, int *eof, void *data)
{
    int r;

    *eof = 1;
    if(sfp_status == SFP_MODULE_OUT)
        r = sprintf(page, "SFP Module Status:unplug\n");
    else if(sfp_status == SFP_MODULE_IN)
        r = sprintf(page, "SFP Module Status:plug in\n");
    else if(sfp_status == SFP_LINK_UP)
        r = sprintf(page, "SFP Module Status:link up\n");
    else
        r = sprintf(page, "SFP Module Status:unkown\n");

    return r < cnt ? r : 0;
}

#if defined(CONFIG_4x2_CROSSBAR_SUPPORT) || defined(CONFIG_5x3_CROSSBAR_SUPPORT)
static int proc_get_crossbar_status(char *page, char **start, off_t off, int cnt, int *eof, void *data)
{
    volatile u32 *cb_mux_reg = (void *)(SWITCH_BASE + CROSSBAR_SWITCH_REG);
#if defined(CONFIG_5x3_CROSSBAR_SUPPORT)
    char *intPortName[] = {"Switch Port 3", "Switch Port 4", "WAN Port"};
    char *extPortName[]= {"AE", "GPHY4", "RGMII3", "xMII", "GPHY3"};
#else /* CONFIG_4x2_CROSSBAR_SUPPORT */
    char *intPortName[] = {"Switch Port 4", "WAN Port"};
    char *extPortName[]= {"AE", "GPHY4", "RGMII3", "xMII"};
#endif
    int r = 0, extPort, intPort;

    for(intPort = 0; intPort < BCMENET_CROSSBAR_MAX_INT_PORTS; intPort++)
    {
        extPort = (*cb_mux_reg & (CB_PHY_PORT_MASK << (intPort * CB_PHY_PORT_SHIFT))) >> (intPort * CB_PHY_PORT_SHIFT);
        if(extPort >= BCMENET_CROSSBAR_MAX_EXT_PORTS)
            extPort = BCMENET_CROSSBAR_MAX_EXT_PORTS-1;
        r += sprintf(page + r, "%s is connected to: %s at Port %d\n", intPortName[intPort], extPortName[extPort], extPort+BP_CROSSBAR_PORT_BASE);
    }

    *eof = 1;
    return r < cnt ? r : 0;
}
#endif /* CROSSBAR_SUPPORT */

void ethsw_init_serdes(void)
{
    ETHERNET_MAC_INFO *info = EnetGetEthernetMacInfo();
    int muxExtPort, phyId;
    u16 val16;
    struct proc_dir_entry *p = NULL;

    p = create_proc_entry(SFP_STATUS_PROC_NAME, 444, NULL);
    if(p != NULL) {
        p->read_proc = proc_get_sfp_status;
    }
#if defined(CONFIG_4x2_CROSSBAR_SUPPORT) || defined(CONFIG_5x3_CROSSBAR_SUPPORT)
    p = create_proc_entry(CROSSBAR_STATUS_PROC_NAME, 444, NULL);
    if(p != NULL) {
        p->read_proc = proc_get_crossbar_status;
    }
#endif

    for (muxExtPort = 0; muxExtPort < BCMENET_CROSSBAR_MAX_EXT_PORTS; muxExtPort++) {
        phyId = info->sw.crossbar[muxExtPort].phy_id;
        if((phyId & MAC_IFACE) != MAC_IF_SERDES)
        {
            continue;
        }

        ETHSW_POWERUP_SERDES(phyId);
        ethsw_sfp_restore_from_power_saving(phyId);

        /* Enable GPIO 36 Input */
        if (BpGetSgmiiGpios(&val16) != BP_SUCCESS)
        {
            printk("Error: GPIO pin for Serdes not defined\n");
            return;
        }

        if(val16 != 28 && val16 != 36)
        {
            printk("Error: GPIO Pin %d for Serdes is not supported, correct boardparams.c definition.\n", val16);
            return;
        }

        if (val16 == 28)
        {
            MISC->miscSGMIIFiberDetect = 0;
            printk("GPIO Pin 28 is assigned to Serdes Fiber Signal Detection.\n");
        }
        else
        {
            MISC->miscSGMIIFiberDetect = MISC_SGMII_FIBER_GPIO36;
            printk("GPIO 36 is assigned to Serdes Fiber Signal Detection.\n");
        }

        /* Enable AN mode */
        #if 0
        ethsw_phy_rreg(phyId, MII_CONTROL, &val16);
        val16 |= MII_CONTROL_AN_ENABLE|MII_CONTROL_RESTART_AUTONEG;
        ethsw_phy_wreg(phyId, MII_CONTROL, &val16);
        #endif

        /* read back for testing */
        ethsw_phy_rreg(phyId, MII_CONTROL, &val16);

        /* Calibrate SGMII termination resistors */
        sgmiiResCal(phyId);

        ethsw_sfp_init(phyId);
    }
}
#endif

/* Broadcom MII Extended Register Access Driver */
DEFINE_MUTEX(bcm_phy_exp_mutex);
void ethsw_phy_exp_rw_reg_flag(int phy_id, int reg, uint16 *data, int ext, int write)
{
    u16 bank, v16, offset;

    /* If the register falls within standard MII address */
    if ((reg & BRCM_MIIEXT_ADDR_RANGE) == 0)
    {
        if(!write)
            ethsw_phy_read_reg(phy_id, reg, data, ext);
        else
            ethsw_phy_write_reg(phy_id, reg, data, ext);
        return;
    }

    bank = reg & BRCM_MIIEXT_BANK_MASK;
    offset = (reg & BRCM_MIIEXT_OFF_MASK) + BRCM_MIIEXT_OFFSET;
    mutex_lock(&bcm_phy_exp_mutex);
    /* Set Bank Address */
    ethsw_phy_wreg(phy_id, BRCM_MIIEXT_BANK, &bank);

    /* Set offset address */
    if (!write)
        ethsw_phy_read_reg(phy_id, offset, data, ext);
    else
        ethsw_phy_write_reg(phy_id, offset, data, ext);

    /* Set Bank back to default for standard access */
    if(bank != BRCM_MIIEXT_DEF_BANK || offset == BRCM_MIIEXT_OFFSET)
    {
        v16 = BRCM_MIIEXT_DEF_BANK;
        ethsw_phy_wreg(phy_id, BRCM_MIIEXT_BANK, &v16);
    }
    mutex_unlock(&bcm_phy_exp_mutex);
}
MODULE_LICENSE("GPL");

