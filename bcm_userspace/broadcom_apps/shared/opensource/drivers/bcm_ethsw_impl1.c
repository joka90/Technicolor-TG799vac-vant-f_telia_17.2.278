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

#include "boardparms.h"

#ifdef _CFE_                                                
#include "lib_types.h"
#include "lib_printf.h"
#include "lib_string.h"
#include "cfe_timer.h"
#include "bcm_map.h"
#define printk  printf
#define udelay cfe_usleep
#else // Linux
#include <linux/kernel.h>
#include <linux/module.h>
#include <bcm_map_part.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/sched.h>
#endif

#include "mii_shared.h"
#include "pmc_drv.h"
#include "pmc_switch.h"
#include "bcm_ethsw.h"
#include "shared_utils.h"

#define K1CTL_REPEATER_DTE 0x400
#if !defined(K1CTL_1000BT_FDX)
#define K1CTL_1000BT_FDX 	0x200
#endif
#if !defined(K1CTL_1000BT_HDX)
#define K1CTL_1000BT_HDX 	0x100
#endif


/*
  This file implement the switch and phy related init and low level function that can be shared between
  cfe and linux.These are functions called from CFE or from the Linux ethernet driver.

  The Linux ethernet driver handles any necessary locking so these functions should not be called
  directly from elsewhere.
*/

/* SF2 switch phy register access function */
uint16_t bcm_ethsw_phy_read_reg(int phy_id, int reg)
{
    int reg_value = 0;
    int i = 0;

    phy_id &= BCM_PHY_ID_M;

    reg_value = ((phy_id << ETHSW_MDIO_C22_PHY_ADDR_SHIFT)&ETHSW_MDIO_C22_PHY_ADDR_MASK) |
                ((reg << ETHSW_MDIO_C22_PHY_REG_SHIFT)&ETHSW_MDIO_C22_PHY_REG_MASK) |
                (ETHSW_MDIO_CMD_C22_READ << ETHSW_MDIO_CMD_SHIFT);
    ETHSW_MDIO->mdio_cmd = reg_value | ETHSW_MDIO_BUSY;
    do {
         if (++i >= 10)  {
             printk("%s MDIO No Response! phy 0x%x reg 0x%x \n", __FUNCTION__, phy_id, reg);
             return 0;
         }
         udelay(60);
         reg_value = ETHSW_MDIO->mdio_cmd;
    } while (reg_value & ETHSW_MDIO_BUSY);

    /* Read a second time to ensure it is reliable */
    ETHSW_MDIO->mdio_cmd = reg_value | ETHSW_MDIO_BUSY;
    do {
         if (++i >= 10)  {
             printk("%s MDIO No Response! phy 0x%x reg 0x%x \n", __FUNCTION__, phy_id, reg);
             return 0;
         }
         udelay(60);
         reg_value = ETHSW_MDIO->mdio_cmd;
    } while (reg_value & ETHSW_MDIO_BUSY);


    return (uint16_t)reg_value;
}

void bcm_ethsw_phy_write_reg(int phy_id, int reg, uint16 data)
{
    int reg_value = 0;
    int i = 0;

    phy_id &= BCM_PHY_ID_M;
 
    reg_value = ((phy_id << ETHSW_MDIO_C22_PHY_ADDR_SHIFT)&ETHSW_MDIO_C22_PHY_ADDR_MASK) |
                ((reg << ETHSW_MDIO_C22_PHY_REG_SHIFT)& ETHSW_MDIO_C22_PHY_REG_MASK) |
                (ETHSW_MDIO_CMD_C22_WRITE << ETHSW_MDIO_CMD_SHIFT) | (data&ETHSW_MDIO_PHY_DATA_MASK);
    ETHSW_MDIO->mdio_cmd = reg_value | ETHSW_MDIO_BUSY;

    do {
         if (++i >= 10)  {
             printk("%s MDIO No Response! phy 0x%x reg 0x%x\n", __FUNCTION__, phy_id, reg);
             return;
         }
         udelay(60);
         reg_value = ETHSW_MDIO->mdio_cmd;
    } while (reg_value & ETHSW_MDIO_BUSY);

    return;
}

#if defined(_BCM963138_) || defined(CONFIG_BCM963138) || defined(_BCM963148_) || defined(CONFIG_BCM963148)
/* EGPHY28 phy misc and expansion register indirect access function. Hard coded MII register
number, define them in bcmmii.h. Should consider move the bcmmii.h to shared folder as well */
uint16 bcm_ethsw_phy_read_exp_reg(int phy_id, int reg)
{
    bcm_ethsw_phy_write_reg(phy_id, 0x17, reg|0xf00);
    return bcm_ethsw_phy_read_reg(phy_id, 0x15);
}


void bcm_ethsw_phy_write_exp_reg(int phy_id, int reg, uint16 data)
{
    bcm_ethsw_phy_write_reg(phy_id, 0x17, reg|0xf00);
    bcm_ethsw_phy_write_reg(phy_id, 0x15, data);

    return;
}

uint16 bcm_ethsw_phy_read_misc_reg(int phy_id, int reg, int chn)
{
    uint16 temp;
    bcm_ethsw_phy_write_reg(phy_id, 0x18, 0x7);
    temp = bcm_ethsw_phy_read_reg(phy_id, 0x18);
    temp |= 0x800;
    bcm_ethsw_phy_write_reg(phy_id, 0x18, temp);

    temp = (chn << 13)|reg;
    bcm_ethsw_phy_write_reg(phy_id, 0x17, temp);
    return  bcm_ethsw_phy_read_reg(phy_id, 0x15);
}

void bcm_ethsw_phy_write_misc_reg(int phy_id, int reg, int chn, uint16 data)
{
    uint16 temp;
    bcm_ethsw_phy_write_reg(phy_id, 0x18, 0x7);
    temp = bcm_ethsw_phy_read_reg(phy_id, 0x18);
    temp |= 0x800;
    bcm_ethsw_phy_write_reg(phy_id, 0x18, temp);

    temp = (chn << 13)|reg;
    bcm_ethsw_phy_write_reg(phy_id, 0x17, temp);
    bcm_ethsw_phy_write_reg(phy_id, 0x15, data);
    
    return;
}
#endif

/* FIXME - same code exists in robosw_reg.c */
static void phy_advertise_caps(unsigned int phy_id)
{
    uint16 cap_mask = 0;

    /* control advertising if boardparms says so */
    if(IsPhyConnected(phy_id) && IsPhyAdvCapConfigValid(phy_id))
    {
        cap_mask = bcm_ethsw_phy_read_reg(phy_id, MII_ANAR);
        cap_mask &= ~(ANAR_TXFD | ANAR_TXHD | ANAR_10FD | ANAR_10HD);
        if (phy_id & ADVERTISE_10HD)
            cap_mask |= ANAR_10HD;
        if (phy_id & ADVERTISE_10FD)
            cap_mask |= ANAR_10FD;
        if (phy_id & ADVERTISE_100HD)
            cap_mask |= ANAR_TXHD;
        if (phy_id & ADVERTISE_100FD)
            cap_mask |= ANAR_TXFD;
        bcm_ethsw_phy_write_reg(phy_id, MII_ANAR, cap_mask);

        cap_mask = bcm_ethsw_phy_read_reg(phy_id, MII_K1CTL);
        cap_mask &= (~(K1CTL_1000BT_FDX | K1CTL_1000BT_HDX));
        if (phy_id & ADVERTISE_1000HD)
            cap_mask |= K1CTL_1000BT_HDX;
        if (phy_id & ADVERTISE_1000FD)
            cap_mask |= K1CTL_1000BT_FDX;
        bcm_ethsw_phy_write_reg(phy_id, MII_K1CTL, cap_mask);
    }

    /* Always enable repeater mode */
    cap_mask = bcm_ethsw_phy_read_reg(phy_id, MII_K1CTL);
    cap_mask |= K1CTL_REPEATER_DTE;
    bcm_ethsw_phy_write_reg(phy_id, MII_K1CTL, cap_mask);
}

#if defined(_BCM963138_) || defined(CONFIG_BCM963138)
static void phy_adjust_afe(unsigned int phy_id)
{
    int rev = UtilGetChipRev();

    //reset phy
    bcm_ethsw_phy_write_reg(phy_id, 0x0, 0x9140);
    udelay(100);
    //Write analog control registers
    if( rev == 0xa0 )
    {
        //AFE_RXCONFIG_0
        bcm_ethsw_phy_write_misc_reg(phy_id, 0x38, 0x0, 0xeb15);
        //AFE_RXCONFIG_1
        bcm_ethsw_phy_write_misc_reg(phy_id, 0x38, 0x1, 0x9a3f);
        //AFE_TX_CONFIG
        bcm_ethsw_phy_write_misc_reg(phy_id, 0x39, 0x0, 0x0800);
        //AFE_RX_LP_COUNTER
        bcm_ethsw_phy_write_misc_reg(phy_id, 0x38, 0x3, 0x7fc0);
        //AFE_HPF_TRIM_OTHERS
        bcm_ethsw_phy_write_misc_reg(phy_id, 0x3a, 0x0, 0x000b);
        //CORE_EXPFF (EEE LPI)
        bcm_ethsw_phy_write_exp_reg(phy_id, 0xFF, 0x2000);
    }
    else
    {
        /* B0 need different adjustment. See JIRA HW63138-132 */
        //AFE_RXCONFIG_1 Provide more margin for INL/DNL measurement on ATE  
        bcm_ethsw_phy_write_misc_reg(phy_id, 0x38, 0x1, 0x9b2f);
        //AFE_VDAC_ICTRL_0 Set Iq=1101 instead of 0111 for improving AB symmetry 
        bcm_ethsw_phy_write_misc_reg(phy_id, 0x39, 0x1, 0xa7da);
        //AFE_HPF_TRIM_OTHERS Set 100Tx/10BT to -4.5% swing & Set rCal offset for HT=0 code
        bcm_ethsw_phy_write_misc_reg(phy_id, 0x3a, 0x0, 0x00e3);
    }
}

static void phy_adjust_misc(unsigned int phy_id)
{
    int rev = UtilGetChipRev();

    if( rev == 0xa0 )
    {
       //AFE_PLLCTRL_1, Change to 'H0048 increase VCO range to prevent unlocking problem of PLL at low temp
        bcm_ethsw_phy_write_misc_reg(phy_id, 0x32, 0x1, 0x0048);

        //AFE_PLLCTRL_2, Change Ki to 011
        bcm_ethsw_phy_write_misc_reg(phy_id, 0x32, 0x2, 0x021b);

        //AFE_PLLCTRL_4,'Disable loading of TVCO buffer to bandgap 'Set Bandgap trim to 111
        bcm_ethsw_phy_write_misc_reg(phy_id, 0x33, 0x0, 0x0e20);

        //Adjust bias current trim by -3 'DSP_TAP10
        bcm_ethsw_phy_write_misc_reg(phy_id, 0xa, 0x0, 0x690b);

        //CORE_BASE1E
        bcm_ethsw_phy_write_reg(phy_id, 0x1e, 0xd);
    }
    else
    {
        /* B0 need different adjustment. See JIRA HW63138-132 */
        //CORE_BASE1E Force trim overwrite and set I_ext trim to 0000
        bcm_ethsw_phy_write_reg(phy_id, 0x1e, 0x10);

        //Adjust bias current trim by +4% swing, +2 tick 'DSP_TAP10
        bcm_ethsw_phy_write_misc_reg(phy_id, 0xa, 0x0, 0x011b);
    }

    //Reset R_CAL/RC_CAL Engine 'CORE_EXPB0
    bcm_ethsw_phy_write_exp_reg(phy_id, 0xb0, 0x10);
    //Disable Reset R_CAL/RC_CAL Engine 'CORE_EXPB0
    bcm_ethsw_phy_write_exp_reg(phy_id, 0xb0, 0x0);
 
}

static void phy_fixup(void)
{
    int phy_id, i;

    /* On 63138 internal QUAD PHY and Single PHY require some addition fixup on the PHY AFE */

    //
    // Quad PHY
    //
    phy_id = (ETHSW_REG->qphy_ctrl&ETHSW_QPHY_CTRL_PHYAD_BASE_MASK)>>ETHSW_QPHY_CTRL_PHYAD_BASE_SHIFT;

    // Adjust individual PHY AFE
    for (i = phy_id; i < phy_id + 4; i++) {
        phy_adjust_afe(i);
    }
    /* quad phy share the same pll for all four ports using the first port */
    phy_adjust_misc(phy_id);

    //
    // Single PHY
    //
    phy_id = (ETHSW_REG->sphy_ctrl&ETHSW_SPHY_CTRL_PHYAD_MASK)>>ETHSW_SPHY_CTRL_PHYAD_SHIFT;

    // Adjust individual PHY AFE
    phy_adjust_afe(phy_id);
    phy_adjust_misc(phy_id);

}
#endif

#if defined(_BCM963148_) || defined(CONFIG_BCM963148)
static void phy_adjust_afe(unsigned int phy_id)
{
    //reset phy
    bcm_ethsw_phy_write_reg(phy_id, 0x0, 0x9140);
    udelay(100);
    //Write analog control registers
    //AFE_RXCONFIG_0
    bcm_ethsw_phy_write_misc_reg(phy_id, 0x38, 0x0, 0xeb15);
    //AFE_RXCONFIG_1. Replacing the previously suggested 0x9AAF for SS part. See JIRA HW63148-31
    bcm_ethsw_phy_write_misc_reg(phy_id, 0x38, 0x1, 0x9b2f);
    //AFE_RXCONFIG_2
    bcm_ethsw_phy_write_misc_reg(phy_id, 0x38, 0x2, 0x2003);
    //AFE_RX_LP_COUNTER
    bcm_ethsw_phy_write_misc_reg(phy_id, 0x38, 0x3, 0x7fc0);
    //AFE_TX_CONFIG
    bcm_ethsw_phy_write_misc_reg(phy_id, 0x39, 0x0, 0x0060);
    //AFE_VDAC_ICTRL_0
    bcm_ethsw_phy_write_misc_reg(phy_id, 0x39, 0x1, 0xa7da);
    //AFE_VDAC_OTHERS_0
    bcm_ethsw_phy_write_misc_reg(phy_id, 0x39, 0x3, 0xa020);
    //AFE_HPF_TRIM_OTHERS
    bcm_ethsw_phy_write_misc_reg(phy_id, 0x3a, 0x0, 0x00a3);
}

static void phy_adjust_misc(unsigned int phy_id)
{
    //CORE_BASE1E Force trim overwrite and set I_ext trim to 0000
    bcm_ethsw_phy_write_reg(phy_id, 0x1e, 0x0010);

    //Adjust bias current trim by +4% swing, +2 tick, increase PLL BW in GPHY link start up training 'DSP_TAP10
    bcm_ethsw_phy_write_misc_reg(phy_id, 0xa, 0x0, 0x111b);

    //Reset R_CAL/RC_CAL Engine 'CORE_EXPB0
    bcm_ethsw_phy_write_exp_reg(phy_id, 0xb0, 0x10);
    //Disable Reset R_CAL/RC_CAL Engine 'CORE_EXPB0
    bcm_ethsw_phy_write_exp_reg(phy_id, 0xb0, 0x0);
}

static void phy_fixup(void)
{
    int phy_id, i;

    /* On 63148 internal QUAD PHY, it requires some additional fixup on the PHY AFE */

    //
    // Quad PHY
    //
    phy_id = (ETHSW_REG->qphy_ctrl&ETHSW_QPHY_CTRL_PHYAD_BASE_MASK)>>ETHSW_QPHY_CTRL_PHYAD_BASE_SHIFT;

    // Adjust individual PHY AFE
    for (i = phy_id; i < phy_id + 4; i++) {
        phy_adjust_afe(i);
    }
    /* quad phy share the same pll for all four ports using the first port */
    phy_adjust_misc(phy_id);
  
    //
    // Single PHY
    //
    phy_id = (ETHSW_REG->sphy_ctrl&ETHSW_SPHY_CTRL_PHYAD_MASK)>>ETHSW_SPHY_CTRL_PHYAD_SHIFT;

    // Adjust individual PHY AFE
    phy_adjust_afe(phy_id);
    phy_adjust_misc(phy_id);
}
#endif

/* SF2 switch low level init */
void bcm_ethsw_init(void)
{
    const ETHERNET_MAC_INFO   *pE;
    const ETHERNET_MAC_INFO   *pMacInfo;
    unsigned int port_map, phy_ctrl;
    unsigned short phy_base;
    int i;
    unsigned long led_map;
   
    if ( ( pE = BpGetEthernetMacInfoArrayPtr()) == NULL )
    {
        printk("ERROR:BoardID not Set in BoardParams\n");
        return;
    }

    /* port map for the SF2 switch */
    pMacInfo = &pE[1];
    port_map = pMacInfo->sw.port_map;

    /* power up the switch block */
    pmc_switch_power_up();

    if( BpGetGphyBaseAddress(&phy_base) != BP_SUCCESS )
        phy_base = 1;

    /* power on and reset the quad and single phy */
    phy_ctrl = ETHSW_REG->qphy_ctrl;
    phy_ctrl &= ~(ETHSW_QPHY_CTRL_IDDQ_BIAS_MASK|ETHSW_QPHY_CTRL_EXT_PWR_DOWN_MASK|ETHSW_QPHY_CTRL_PHYAD_BASE_MASK);
    phy_ctrl |= ETHSW_QPHY_CTRL_RESET_MASK|(phy_base<<ETHSW_QPHY_CTRL_PHYAD_BASE_SHIFT);
    ETHSW_REG->qphy_ctrl = phy_ctrl;

    phy_ctrl = ETHSW_REG->sphy_ctrl;
    phy_ctrl &= ~(ETHSW_SPHY_CTRL_IDDQ_BIAS_MASK| ETHSW_SPHY_CTRL_EXT_PWR_DOWN_MASK|ETHSW_SPHY_CTRL_PHYAD_MASK);
    phy_ctrl |= ETHSW_SPHY_CTRL_RESET_MASK|((phy_base+4)<<ETHSW_SPHY_CTRL_PHYAD_SHIFT);
    ETHSW_REG->sphy_ctrl = phy_ctrl;

    udelay(1000);

    ETHSW_REG->qphy_ctrl &= ~ETHSW_QPHY_CTRL_RESET_MASK;
    ETHSW_REG->sphy_ctrl &= ~ETHSW_SPHY_CTRL_RESET_MASK;

    udelay(1000);

    phy_fixup();

    if (BpGetEthLedMap_tch(&led_map) != BP_SUCCESS)
    {
        led_map = ((ETHSW_LED_CTRL_SPD0_ON  << ETHSW_LED_CTRL_1000M_SHIFT) | (ETHSW_LED_CTRL_SPD1_OFF << ETHSW_LED_CTRL_1000M_SHIFT) |
                   (ETHSW_LED_CTRL_SPD0_OFF << ETHSW_LED_CTRL_100M_SHIFT)  | (ETHSW_LED_CTRL_SPD1_ON  << ETHSW_LED_CTRL_100M_SHIFT) |
                   (ETHSW_LED_CTRL_SPD0_OFF << ETHSW_LED_CTRL_10M_SHIFT)   | (ETHSW_LED_CTRL_SPD1_OFF << ETHSW_LED_CTRL_10M_SHIFT) |
                   (ETHSW_LED_CTRL_SPD0_OFF << ETHSW_LED_CTRL_NOLINK_SHIFT)| (ETHSW_LED_CTRL_SPD1_OFF << ETHSW_LED_CTRL_NOLINK_SHIFT));
    }

    /* set the sw internal phy LED mode. the default speed mode encoding is wrong.
       apply to all 5 internal GPHY */
    for( i = 0; i < 5; i++ )
    {
        ETHSW_REG->led_ctrl[i] &= ~ETHSW_LED_CTRL_SPEED_MASK;
        /* broadcom reference design alway use LED_SPD0 for 1g link and LED_SPD1 for 100m link */
        ETHSW_REG->led_ctrl[i] |= led_map;
    }

    if ( (CHIP_FAMILY_ID_HEX == 0x63148) || ((CHIP_FAMILY_ID_HEX == 0x63138) && (UtilGetChipRev() != 0xa0)) ) {
        /* set the sw wan port LED mode */
        ETHSW_REG->led_wan_ctrl &= ~ETHSW_LED_CTRL_SPEED_MASK;
        /* broadcom reference design alway use LED_SPD0 for 1g link and LED_SPD1 for 100m link */
        ETHSW_REG->led_wan_ctrl |= led_map;
    }

    /* Setting switch to un-managed mode. */
    for( i = 0; i < BP_MAX_SWITCH_PORTS; i++ )
    {
        if( ((1 << i) & port_map) != 0 )
            ETHSW_CORE->port_traffic_ctrl[i] = 0;
    }

    ETHSW_CORE->switch_mode = ETHSW_SM_RETRY_LIMIT_DIS | ETHSW_SM_FORWARDING_EN;
    ETHSW_CORE->brcm_hdr_ctrl = 0;
    ETHSW_CORE->switch_ctrl = ETHSW_SC_MII_DUMP_FORWARDING_EN | ETHSW_SC_MII2_VOL_SEL;

    /* TBD. Use an ETHERNET_MAC_INFO field or something else to determine override for each port.
     *      Values used here are taken from the configuration used in simulation settings.
     */
#if 0
    for( i = 4; i < 8; i++ )
    {
        if( ((1 << i) & port_map) != 0 )
            ETHSW_CORE->port_state_override[i] = ETHSW_PS_SW_OVERRIDE | ETHSW_PS_SW_TX_FLOW_CTRL_EN | ETHSW_PS_SW_RX_FLOW_CTRL_EN | 
                ETHSW_PS_SW_PORT_SPEED_1000M | ETHSW_PS_DUPLEX_MODE | ETHSW_PS_LINK_UP;
    }
#endif

#if 0 /* Temporary work-around until we have no support for NETDEVICE_INIT_EARLY=0 for these platforms */
    ETHSW_CORE->imp_port_state = ETHSW_IPS_USE_REG_CONTENTS | ETHSW_IPS_TXFLOW_PAUSE_CAPABLE | ETHSW_IPS_RXFLOW_PAUSE_CAPABLE |
        ETHSW_IPS_SW_PORT_SPEED_1000M_2000M | ETHSW_IPS_DUPLEX_MODE | ETHSW_IPS_LINK_PASS;
#else
    ETHSW_CORE->imp_port_state = ETHSW_IPS_USE_REG_CONTENTS | ETHSW_IPS_TXFLOW_PAUSE_CAPABLE | ETHSW_IPS_RXFLOW_PAUSE_CAPABLE |
        ETHSW_IPS_SW_PORT_SPEED_1000M_2000M | ETHSW_IPS_DUPLEX_MODE;
    printk("\n LINK DOWN IMP Port \n");
#endif

#if 0
    ETHSW_REG->rgmii_5_ctrl  = ETHSW_RC_EXT_GPHY | ETHSW_RC_ID_MODE_DIS;
    ETHSW_REG->rgmii_7_ctrl  = ETHSW_RC_EXT_GPHY | ETHSW_RC_ID_MODE_DIS;
    ETHSW_REG->rgmii_11_ctrl = ETHSW_RC_EXT_GPHY | ETHSW_RC_ID_MODE_DIS;
    ETHSW_REG->rgmii_12_ctrl = ETHSW_RC_EXT_GPHY | ETHSW_RC_ID_MODE_DIS;
#endif

    {/* Configure PHY capability advertisement */
        /* port map for the SF2 switch */
        pMacInfo = &pE[1];
        port_map = pMacInfo->sw.port_map;
        for( i = 0; i < BP_MAX_SWITCH_PORTS; i++ )
        {
            if( ((1 << i) & port_map) != 0 )
                phy_advertise_caps(pMacInfo->sw.phy_id[i]);
        }
    }

    return;
}



