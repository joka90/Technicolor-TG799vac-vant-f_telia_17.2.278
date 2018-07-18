/*
   Copyright 2007-2010 Broadcom Corp. All Rights Reserved.

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
#define _BCMENET_LOCAL_
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/mii.h>
#include <linux/stddef.h>
#include <linux/ctype.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <board.h>
#include "boardparms.h"
#include <bcm_map_part.h>
#include "bcm_intr.h"
#include "bcmenet.h"
#include "bcmmii.h"
#include "ethswdefs.h"
#include "ethsw.h"
#include "ethsw_phy.h"
#include "eth_pwrmngt.h"
#include "bcmsw.h"
#include "bcmSpiRes.h"
#include "bcmswaccess.h"
#include "bcmswshared.h"
#include "pktCmf_public.h"
#if !defined(CONFIG_BCM963138) && !defined(CONFIG_BCM963148)
#include "bcmPktDma.h"
#endif
#if defined(_CONFIG_BCM_FAP)
#include "fap_packet.h"
#endif
#if defined(CONFIG_BCM_GMAC)
#include "bcmgmac.h"
#endif
#if defined(CONFIG_BCM963381)
#include "pmc_switch.h"
#endif

#if defined(CONFIG_BCM96828) && !defined(CONFIG_EPON_HGU)
int uni_to_uni_enabled = 0;
#endif

static void str_to_num(char *in, char *out, int len);
static int proc_get_sw_param(char *page, char **start, off_t off, int cnt, int *eof, void *data);
static int proc_set_sw_param(struct file *f, const char *buf, unsigned long cnt, void *data);

static int proc_get_mii_param(char *page, char **start, off_t off, int cnt, int *eof, void *data);
static int proc_set_mii_param(struct file *f, const char *buf, unsigned long cnt, void *data);


#if defined(ENET_GPON_CONFIG)
extern struct net_device *gponifid_to_dev[MAX_GEM_IDS];
#endif
extern struct semaphore bcm_ethlock_switch_config;
extern uint8_t port_in_loopback_mode[TOTAL_SWITCH_PORTS];
extern int vport_cnt;  /* number of vports: bitcount of Enetinfo.sw.port_map */

extern BcmEnet_devctrl *pVnetDev0_g;

static uint16_t dis_learning = 0x0180;
static uint8_t  port_fwd_ctrl = 0xC1;
static uint16_t pbvlan_map[TOTAL_SWITCH_PORTS];

PHY_STAT ethsw_phyid_stat(int phyId)
{
    PHY_STAT ps;
    uint16 v16;
    uint16 ctrl;
    uint16 mii_esr = 0;
    uint16 mii_stat = 0, mii_adv = 0, mii_lpa = 0;
    uint16 mii_gb_ctrl = 0, mii_gb_stat = 0;

    ps.lnk = 0;
    ps.fdx = 0;
    ps.spd1000 = 0;
    ps.spd100 = 0;
    if ( !IsPhyConnected(phyId) )
    {
        // 0xff PHY ID means no PHY on this port.
        ps.lnk = 1;
        ps.fdx = 1;
#if defined(CONFIG_BCM96318)
        if (IsMII(phyId))
        {
            ps.spd100 = 1;
        }
        else
#endif
        {
            ps.spd1000 = 1;
        }
        return ps;
    }

    down(&bcm_ethlock_switch_config);

    ethsw_phy_rreg(phyId, MII_INTERRUPT, &v16);
    ethsw_phy_rreg(phyId, MII_ASR, &v16);
    BCM_ENET_DEBUG("%s mii_asr (reg 25) 0x%x\n", __FUNCTION__, v16);


    if (!MII_ASR_LINK(v16)) {
        up(&bcm_ethlock_switch_config);
        return ps;
    }

    ps.lnk = 1;

    ethsw_phy_rreg(phyId, MII_BMCR, &ctrl);

    if (!MII_ASR_DONE(v16)) {
        ethsw_phy_rreg(phyId, MII_BMCR, &ctrl);
        if (ctrl & BMCR_ANENABLE) {
            up(&bcm_ethlock_switch_config);
            return ps;
        }
        // auto-negotiation disabled
        ps.fdx = (ctrl & BMCR_FULLDPLX) ? 1 : 0;
        if((ctrl & BMCR_SPEED100) && !(ctrl & BMCR_SPEED1000))
            ps.spd100 = 1;
        else if(!(ctrl & BMCR_SPEED100) && (ctrl & BMCR_SPEED1000))
            ps.spd1000 = 1;

        up(&bcm_ethlock_switch_config);
        return ps;
    }

#ifdef CONFIG_BCM96368
    if ((!IsExtPhyId(phyId)) && MII_ASR_NOHCD(v16)) {
        ethsw_phy_rreg(phyId, MII_AENGSR, &ctrl);
        if (ctrl & MII_AENGSR_SPD) {
            ps.spd100 = 1;
        }
        if (ctrl & MII_AENGSR_DPLX) {
            ps.fdx = 1;
        }
        return ps;
    }
#endif

    //Auto neg enabled (this end) cases
    ethsw_phy_rreg(phyId, MII_ADVERTISE, &mii_adv);
    ethsw_phy_rreg(phyId, MII_LPA, &mii_lpa);
    ethsw_phy_rreg(phyId, MII_BMSR, &mii_stat);

    BCM_ENET_DEBUG("%s mii_adv 0x%x mii_lpa 0x%x mii_stat 0x%x mii_ctrl 0x%x \n", __FUNCTION__,
            mii_adv, mii_lpa, mii_stat, v16);
    // read 1000mb Phy  registers if supported
    if (mii_stat & BMSR_ESTATEN) { 

        ethsw_phy_rreg(phyId, MII_ESTATUS, &mii_esr);
        if (mii_esr & (1 << 15 | 1 << 14 |
                    ESTATUS_1000_TFULL | ESTATUS_1000_THALF))
            ethsw_phy_rreg(phyId, MII_CTRL1000, &mii_gb_ctrl);
        ethsw_phy_rreg(phyId, MII_STAT1000, &mii_gb_stat);
    }

    mii_adv &= mii_lpa;

    if ((mii_gb_ctrl & ADVERTISE_1000FULL) &&  // 1000mb Adv
            (mii_gb_stat & LPA_1000FULL))
    {
        ps.spd1000 = 1;
        ps.fdx = 1;
    } else if ((mii_gb_ctrl & ADVERTISE_1000HALF) && 
            (mii_gb_stat & LPA_1000HALF))
    {
        ps.spd1000 = 1;
        ps.fdx = 0;
    } else if (mii_adv & ADVERTISE_100FULL) {  // 100mb adv
        ps.spd100 = 1;
        ps.fdx = 1;
    } else if (mii_adv & ADVERTISE_100BASE4) {
        ps.spd100 = 1;
        ps.fdx = 0;
    } else if (mii_adv & ADVERTISE_100HALF) {
        ps.spd100 = 1;
        ps.fdx = 0;
    } else if (mii_adv & ADVERTISE_10FULL) {
        ps.fdx = 1;
    }

    up(&bcm_ethlock_switch_config);

    return ps;
}

#if defined(CONFIG_BCM963268) ||  defined(CONFIG_BCM96828)
void ethsw_phy_advertise_all(uint32 phy_id)
{
    uint16 v16;
    /* Advertise all speed & duplex combinations */
    /* Advertise 100BaseTX FD/HD and 10BaseT FD/HD */
    ethsw_phy_rreg(phy_id, MII_ADVERTISE, &v16);
    v16 |= AN_ADV_ALL;
    ethsw_phy_wreg(phy_id, MII_ADVERTISE, &v16);
    /* Advertise 1000BaseT FD/HD */
    ethsw_phy_rreg(phy_id, MII_CTRL1000, &v16);
    v16 |= AN_1000BASET_CTRL_ADV_ALL;
    ethsw_phy_wreg(phy_id, MII_CTRL1000, &v16);
}
#endif

/* apply phy init board parameters for internal switch*/
void ethsw_phy_apply_init_bp(void)
{
    BcmEnet_devctrl *pVnetDev0 = (BcmEnet_devctrl *) netdev_priv(vnet_dev[0]);
    unsigned int portmap, i, phy_id;
    bp_mdio_init_t* phyinit;
    uint16 data;

    portmap = pVnetDev0->EnetInfo[0].sw.port_map;
    for (i = 0; i < (TOTAL_SWITCH_PORTS - 1); i++) {
        if ((portmap & (1U<<i)) != 0) {
            phy_id = pVnetDev0->EnetInfo[0].sw.phy_id[i];
            phyinit = pVnetDev0->EnetInfo[0].sw.phyinit[i];
            if( phyinit == 0 )
                continue;

            while(phyinit->u.op.op != BP_MDIO_INIT_OP_NULL)
            {
                if(phyinit->u.op.op == BP_MDIO_INIT_OP_WRITE)
                    ethsw_phy_wreg(phy_id, phyinit->u.write.reg, (uint16*)(&phyinit->u.write.data));
                else if(phyinit->u.op.op == BP_MDIO_INIT_OP_UPDATE)
                {
                    ethsw_phy_rreg(phy_id, phyinit->u.update.reg, &data);
                    data &= ~phyinit->u.update.mask;
                    data |= phyinit->u.update.data;
                    ethsw_phy_wreg(phy_id, phyinit->u.update.reg, &data);
                }
                phyinit++;
            }
        }
    }

}
/* Code to handle exceptions and chip specific cases */
void ethsw_phy_handle_exception_cases (void)
{
    /* In some chips, the GPhys do not advertise all capabilities. So, fix it first */ 
#if defined(CONFIG_BCM963268)
    ethsw_phy_advertise_all(GPHY_PORT_PHY_ID);
#endif

#if defined(CONFIG_BCM96828)
    ethsw_phy_advertise_all(GPHY1_PORT_PHY_ID);
    ethsw_phy_advertise_all(GPHY2_PORT_PHY_ID);
#endif
}

#if defined(CONFIG_BCM96828)
int ethsw_setup_phys(void)
{
    BcmEnet_devctrl *pVnetDev0 = (BcmEnet_devctrl *) netdev_priv(vnet_dev[0]);
    unsigned int phy_id, portmap;
    uint16 v16;


    /* Get the portmap */
    portmap = pVnetDev0->EnetInfo[0].sw.port_map;

    /* Reset the RC calibration of the internal G-Phys to improve the return loss in 10BT.
       The calibration for both the internal G-Phys is shared and configurable through the first Phy.
       The reset is recommended by HW Team : JIRA#SWBCACPE-10270 */
    if ((portmap & (1U<<GPHY1_PORT_ID)) != 0) {
        phy_id = pVnetDev0->EnetInfo[0].sw.phy_id[GPHY1_PORT_ID];
        v16 = 0x0FB0; /* Expansion register 0xB0 */
        ethsw_phy_wreg(phy_id, MII_DSP_COEFF_ADDR, &v16);
        /* Read the current value */
        ethsw_phy_rreg(phy_id, MII_DSP_COEFF_RW_PORT, &v16);
        v16 |= 0x04; /* Set Reset Bit[2] */
        ethsw_phy_wreg(phy_id, MII_DSP_COEFF_RW_PORT, &v16);
    }
    return 0;
}
#else
int ethsw_setup_phys(void)
{
    ethsw_shutdown_unused_phys();
    return 0;
}
#endif



void bcmeapi_ethsw_init_config(void)
{
    int i;

    /* Save the state that is restored in enable_hw_switching */
    for(i = 0; i < TOTAL_SWITCH_PORTS; i++)  {
        ethsw_rreg(PAGE_PORT_BASED_VLAN, REG_VLAN_CTRL_P0 + (i * 2),
                (uint8 *)&pbvlan_map[i], 2);
    }
    ethsw_rreg(PAGE_CONTROL, REG_DISABLE_LEARNING, (uint8 *)&dis_learning, 2);
    ethsw_rreg(PAGE_CONTROL, REG_PORT_FORWARD, (uint8 *)&port_fwd_ctrl, 1);

#if defined(CONFIG_BCM963268) || defined(CONFIG_BCM96828) || defined(CONFIG_BCM963381)
    {
        /* Disable tags for internal switch ports */
        uint32 tmp;
        ethsw_rreg(PAGE_CONTROL, REG_IUDMA_CTRL, (uint8_t *)&tmp, 4);
        tmp |= REG_IUDMA_CTRL_TX_MII_TAG_DISABLE;
        ethsw_wreg(PAGE_CONTROL, REG_IUDMA_CTRL, (uint8_t *)&tmp, 4); 
    }
#endif

}

int ethsw_setup_led(void)
{
    BcmEnet_devctrl *pVnetDev0 = (BcmEnet_devctrl *) netdev_priv(vnet_dev[0]);
    unsigned int phy_id, i;
    uint16 v16;

    /* For each port that has an internal or external PHY, configure it
       as per the required initial configuration */
    for (i = 0; i < (TOTAL_SWITCH_PORTS - 1); i++) {
        /* Check if the port is in the portmap or not */
        if ((pVnetDev0->EnetInfo[0].sw.port_map & (1U<<i)) != 0) {
            /* Check if the port is connected to a PHY or not */
            phy_id = pVnetDev0->EnetInfo[0].sw.phy_id[i];
            /* If a Phy is connected, set it up with initial config */
            /* TBD: Maintain the config for each Phy */
            if(IsPhyConnected(phy_id) && !IsExtPhyId(phy_id)) {
#if defined(CONFIG_BCM96368)
                v16 = 1 << 2;
                ethsw_phy_wreg(phy_id, MII_TPISTATUS, &v16);
#elif defined(CONFIG_BCM96328) || defined(CONFIG_BCM96362) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM96828) || defined(CONFIG_BCM96318) || defined(CONFIG_BCM963381)
                v16 = 0xa410;
                // Enable Shadow register 2
                ethsw_phy_rreg(phy_id, MII_BRCM_TEST, &v16);
                v16 |= MII_BRCM_TEST_SHADOW2_ENABLE;
                ethsw_phy_wreg(phy_id, MII_BRCM_TEST, &v16);

#if defined(CONFIG_BCM963268) || defined(CONFIG_BCM96828)
#if defined(CONFIG_BCM963268)
                if (i != GPHY_PORT_ID) 
#else
                    if ((i != GPHY1_PORT_ID) && (i != GPHY2_PORT_ID))
#endif
                    {
                        // Set LED1 to speed. Set LED0 to blinky link
                        v16 = 0x08;
                    }
#else
                // Set LED0 to speed. Set LED1 to blinky link
                v16 = 0x71;
#endif
                ethsw_phy_wreg(phy_id, 0x15, &v16);
                // Disable Shadow register 2
                ethsw_phy_rreg(phy_id, MII_BRCM_TEST, &v16);
                v16 &= ~MII_BRCM_TEST_SHADOW2_ENABLE;
                ethsw_phy_wreg(phy_id, MII_BRCM_TEST, &v16);
#endif
            }
            if (IsExtPhyId(phy_id)) {
                /* Configure LED for link/activity */
                v16 = MII_1C_SHADOW_LED_CONTROL << MII_1C_SHADOW_REG_SEL_S;
                ethsw_phy_wreg(phy_id, MII_REGISTER_1C, &v16);
                ethsw_phy_rreg(phy_id, MII_REGISTER_1C, &v16);
                v16 |= ACT_LINK_LED_ENABLE;
                v16 |= MII_1C_WRITE_ENABLE;
                v16 &= ~(MII_1C_SHADOW_REG_SEL_M << MII_1C_SHADOW_REG_SEL_S);
                v16 |= (MII_1C_SHADOW_LED_CONTROL << MII_1C_SHADOW_REG_SEL_S);
                ethsw_phy_wreg(phy_id, MII_REGISTER_1C, &v16);

                ethsw_phy_rreg(phy_id, MII_PHYSID2, &v16);
                if ((v16 & BCM_PHYID_M) == (BCM54610_PHYID2 & BCM_PHYID_M)) {
                    /* Configure LOM LED Mode */
                    v16 = MII_1C_EXTERNAL_CONTROL_1 << MII_1C_SHADOW_REG_SEL_S;
                    ethsw_phy_wreg(phy_id, MII_REGISTER_1C, &v16);
                    ethsw_phy_rreg(phy_id, MII_REGISTER_1C, &v16);
                    v16 |= LOM_LED_MODE;
                    v16 |= MII_1C_WRITE_ENABLE;
                    v16 &= ~(MII_1C_SHADOW_REG_SEL_M << MII_1C_SHADOW_REG_SEL_S);
                    v16 |= (MII_1C_EXTERNAL_CONTROL_1 << MII_1C_SHADOW_REG_SEL_S);
                    ethsw_phy_wreg(phy_id, MII_REGISTER_1C, &v16);
                }
            }
        }
    }
    return 0;
}

int ethsw_reset_ports(struct net_device *dev)
{
    BcmEnet_devctrl *pDevCtrl = netdev_priv(dev);
    int map, cnt, i;
    uint16 v16, phy_identifier;
    int phyid;
    uint8 v8;
    unsigned long port_flags;
    unsigned short rxRgmiiClockDelayAtMac = 0;

    map = pDevCtrl->EnetInfo[0].sw.port_map;
    bitcount(cnt, map);

    if (cnt <= 0)
        return 0;

	/* check if disable rgmii skew is set, if not, set it to default value */
    if(BpGetRxRgmiiClockDelayAtMac_tch((unsigned short*)&rxRgmiiClockDelayAtMac) != BP_SUCCESS )
    {
        rxRgmiiClockDelayAtMac = 0;
    }

#if defined(CONFIG_BCM963268)
    if (map & (1 << (RGMII_PORT_ID + 1))) {
        GPIO->RoboswSwitchCtrl |= (RSW_MII_2_IFC_EN | (RSW_MII_SEL_2P5V << RSW_MII_2_SEL_SHIFT));
    }
#endif

    for (i = 0; i < NUM_RGMII_PORTS; i++) {
#if defined(CONFIG_BCM96318)
        if (map & (1 << (RGMII_PORT_ID + i)))
#endif
        {
            phyid = pDevCtrl->EnetInfo[0].sw.phy_id[RGMII_PORT_ID + i];
            ethsw_phy_rreg(phyid, MII_PHYSID2, &phy_identifier);

            ethsw_rreg(PAGE_CONTROL, REG_RGMII_CTRL_P4 + i, &v8, 1);
#if defined(CONFIG_BCM963268) || defined(CONFIG_BCM96828)
            v8 |= REG_RGMII_CTRL_ENABLE_RGMII_OVERRIDE;
            v8 &= ~REG_RGMII_CTRL_MODE;
            if (IsRGMII(phyid)) {
                v8 |= REG_RGMII_CTRL_MODE_RGMII;
            } else if (IsRvMII(phyid)) {
                v8 |= REG_RGMII_CTRL_MODE_RvMII;
            } else if (IsGMII(phyid)) {
                v8 |= REG_RGMII_CTRL_MODE_GMII;
            } else {
                v8 |= REG_RGMII_CTRL_MODE_MII;
            }
#endif
            
#if defined(CONFIG_BCM963268)
            if ((pDevCtrl->chipRev == 0xA0) || (pDevCtrl->chipRev == 0xB0)) {
                /* RGMII timing workaround */
                v8 &= ~REG_RGMII_CTRL_TIMING_SEL;
            }
            else
#endif    
            {

                v8 |= REG_RGMII_CTRL_TIMING_SEL;
            }
            /* Enable Clock delay in RX */
            port_flags = enet_get_port_flags(0, RGMII_PORT_ID + i);
            if (IsPortRxInternalDelay(port_flags) || rxRgmiiClockDelayAtMac) {
                v8 |= REG_RGMII_CTRL_DLL_RXC_BYPASS;
            }
            else if ((phy_identifier & BCM_PHYID_M) == (BCM54616_PHYID2 & BCM_PHYID_M)) {
                v8 |= REG_RGMII_CTRL_DLL_RXC_BYPASS;
            }

            ethsw_wreg(PAGE_CONTROL, REG_RGMII_CTRL_P4 + i, &v8, 1);

#if defined(CONFIG_BCM963268)
            if ((pDevCtrl->chipRev == 0xA0) || (pDevCtrl->chipRev == 0xB0)) {
                /* RGMII timing workaround */
                v8 = 0xAB;
                ethsw_wreg(PAGE_CONTROL, REG_RGMII_TIMING_P4 + i, &v8, 1);
            }
#endif

            /* No need to check the PhyID if the board params is set correctly for RGMII. However, keeping
             *   the phy id check to make it work even when customer does not set the RGMII flag in the phy_id
             *   in board params
             */
            if ((IsRGMII(phyid) && IsPhyConnected(phyid)) ||
                    ((phy_identifier & BCM_PHYID_M) == (BCM54610_PHYID2 & BCM_PHYID_M)) ||
                    ((phy_identifier & BCM_PHYID_M) == (BCM50612_PHYID2 & BCM_PHYID_M))) {

                v16 = MII_1C_SHADOW_CLK_ALIGN_CTRL << MII_1C_SHADOW_REG_SEL_S;
                ethsw_phy_wreg(phyid, MII_REGISTER_1C, &v16);
                ethsw_phy_rreg(phyid, MII_REGISTER_1C, &v16);
#if defined(CONFIG_BCM963268)
                /* Temporary work-around for MII2 port RGMII delay programming */
                if (i == 1 && ((pDevCtrl->chipRev == 0xA0) || (pDevCtrl->chipRev == 0xB0)) )
                    v16 |= GTXCLK_DELAY_BYPASS_DISABLE;
                else
#endif
                    v16 &= (~GTXCLK_DELAY_BYPASS_DISABLE);
                v16 |= MII_1C_WRITE_ENABLE;
                v16 &= ~(MII_1C_SHADOW_REG_SEL_M << MII_1C_SHADOW_REG_SEL_S);
                v16 |= (MII_1C_SHADOW_CLK_ALIGN_CTRL << MII_1C_SHADOW_REG_SEL_S);
                ethsw_phy_wreg(phyid, MII_REGISTER_1C, &v16);
                if ((phy_identifier & BCM_PHYID_M) == (BCM54616_PHYID2 & BCM_PHYID_M) || rxRgmiiClockDelayAtMac) {
                v16 = MII_REG_18_SEL(0x7);
                    ethsw_phy_wreg(phyid, MII_REGISTER_18, &v16);
                    ethsw_phy_rreg(phyid, MII_REGISTER_18, &v16);
                    /* Disable Skew */
                    v16 &= (~RGMII_RXD_TO_RXC_SKEW);
                    v16 = MII_REG_18_WR(0x7,v16);
                    ethsw_phy_wreg(phyid, MII_REGISTER_18, &v16);
                }
            }
        }
    }
#if defined(CONFIG_BCM96828)
    ethsw_rreg(PAGE_CONTROL, REG_RGMII_CTRL_P7, &v8, 1);
    v8 |= REG_RGMII_CTRL_ENABLE_GMII;
    ethsw_wreg(PAGE_CONTROL, REG_RGMII_CTRL_P7, &v8, 1);
#endif

    /*Remaining port reset functionality is moved into ethsw_init_hw*/

    return 0;
}

int bcmeapi_ethsw_init(void)
{
    robosw_init();

    return 0;
}

void bcmeapi_ethsw_init_ports()
{
    robosw_configure_ports();
}

static uint8 swdata[16];
static uint8 miidata[16];

int ethsw_add_proc_files(struct net_device *dev)
{
    struct proc_dir_entry *p;

    p = create_proc_entry("switch", 0644, NULL);

    if (p == NULL)
        return -1;

    memset(swdata, 0, sizeof(swdata));

    p->data        = dev;
    p->read_proc   = proc_get_sw_param;
    p->write_proc  = proc_set_sw_param;

    p = create_proc_entry("mii", 0644, NULL);

    if (p == NULL)
        return -1;

    memset(miidata, 0, sizeof(miidata));

    p->data       = dev;
    p->read_proc  = proc_get_mii_param;
    p->write_proc = proc_set_mii_param;

    return 0;
}

int ethsw_del_proc_files(void)
{
    remove_proc_entry("switch", NULL);

    remove_proc_entry("mii", NULL);
    return 0;
}

static void str_to_num(char* in, char* out, int len)
{
    int i;
    memset(out, 0, len);

    for (i = 0; i < len * 2; i ++)
    {
        if ((*in >= '0') && (*in <= '9'))
            *out += (*in - '0');
        else if ((*in >= 'a') && (*in <= 'f'))
            *out += (*in - 'a') + 10;
        else if ((*in >= 'A') && (*in <= 'F'))
            *out += (*in - 'A') + 10;
        else
            *out += 0;

        if ((i % 2) == 0)
            *out *= 16;
        else
            out ++;

        in ++;
    }
    return;
}

static int proc_get_sw_param(char *page, char **start, off_t off, int cnt, int *eof, void *data)
{
    int reg_page  = swdata[0];
    int reg_addr  = swdata[1];
    int reg_len   = swdata[2];
    int i = 0;
    int r = 0;

    *eof = 1;

    if (reg_len == 0)
        return 0;

    down(&bcm_ethlock_switch_config);
    ethsw_rreg(reg_page, reg_addr, swdata + 3, reg_len);
    up(&bcm_ethlock_switch_config);

    r += sprintf(page + r, "[%02x:%02x] = ", swdata[0], swdata[1]);

    for (i = 0; i < reg_len; i ++)
        r += sprintf(page + r, "%02x ", swdata[3 + i]);

    r += sprintf(page + r, "\n");
    return (r < cnt)? r: 0;
}

static int proc_set_sw_param(struct file *f, const char *buf, unsigned long cnt, void *data)
{
    char input[32];
    int i;
    int r;
    int num_of_octets;

    int reg_page;
    int reg_addr;
    int reg_len;

    if (cnt > 32)
        cnt = 32;

    if (copy_from_user(input, buf, cnt) != 0)
        return -EFAULT;

    r = cnt;

    for (i = 0; i < r; i ++)
    {
        if (!isxdigit(input[i]))
        {
            memmove(&input[i], &input[i + 1], r - i - 1);
            r --;
            i --;
        }
    }

    num_of_octets = r / 2;

    if (num_of_octets < 3) // page, addr, len
        return -EFAULT;

    str_to_num(input, swdata, num_of_octets);

    reg_page  = swdata[0];
    reg_addr  = swdata[1];
    reg_len   = swdata[2];

    if (((reg_len != 1) && (reg_len % 2) != 0) || reg_len > 8)
    {
        memset(swdata, 0, sizeof(swdata));
        return -EFAULT;
    }

    if ((num_of_octets > 3) && (num_of_octets != reg_len + 3))
    {
        memset(swdata, 0, sizeof(swdata));
        return -EFAULT;
    }

    if (num_of_octets > 3) {
        down(&bcm_ethlock_switch_config);
        ethsw_wreg(reg_page, reg_addr, swdata + 3, reg_len);
        up(&bcm_ethlock_switch_config);
    }
    return cnt;
}

static int proc_get_mii_param(char *page, char **start, off_t off, int cnt, int *eof, void *data)
{
    int mii_port  = miidata[0];
    int mii_addr  = miidata[1];
    int r = 0;

    *eof = 1;

    down(&bcm_ethlock_switch_config);
    ethsw_phy_rreg(mii_port, mii_addr, (uint16 *)(miidata + 2));
    up(&bcm_ethlock_switch_config);

    r += sprintf(
            page + r,
            "[%02x:%02x] = %02x %02x\n",
            miidata[0], miidata[1], miidata[2], miidata[3]
            );

    return (r < cnt)? r: 0;
}

static int proc_set_mii_param(struct file *f, const char *buf, unsigned long cnt, void *data)
{
    char input[32];
    int i;
    int r;
    int num_of_octets;

    int mii_port;
    int mii_addr;

    if (cnt > 32)
        cnt = 32;

    if (copy_from_user(input, buf, cnt) != 0)
        return -EFAULT;

    r = cnt;

    for (i = 0; i < r; i ++)
    {
        if (!isxdigit(input[i]))
        {
            memmove(&input[i], &input[i + 1], r - i - 1);
            r --;
            i --;
        }
    }

    num_of_octets = r / 2;

    if ((num_of_octets!= 2) && (num_of_octets != 4))
    {
        memset(miidata, 0, sizeof(miidata));
        return -EFAULT;
    }

    str_to_num(input, miidata, num_of_octets);
    mii_port  = miidata[0];
    mii_addr  = miidata[1];

    down(&bcm_ethlock_switch_config);

    if (num_of_octets > 2)
        ethsw_phy_wreg(mii_port, mii_addr, (uint16 *)(miidata + 2));

    up(&bcm_ethlock_switch_config);
    return cnt;
}

#if defined(CONFIG_BCM96368)
/*
 *------------------------------------------------------------------------------
 * Function   : ethsw_enable_sar_port
 * Description: Setup the SAR port of a 6368 Switch as follows:
 *              - Learning is disabled.
 *              - Full duplex, 1000Mbs, Link Up
 *              - Rx Enabled, Tx Disabled.
 *
 * Design Note: Invoked by SAR Packet CMF when SAR XTM driver is operational,
 *              via CMF Hook pktCmfSarPortEnable.
 *
 *              This function expects to be called from a softirq context.
 *------------------------------------------------------------------------------
 */
int ethsw_enable_sar_port(void)
{
    uint8  val8;
    uint16 val16;

    ethsw_rreg(PAGE_CONTROL, REG_DISABLE_LEARNING,
            (uint8 *)&val16, sizeof(val16) );
    val16 |= ( 1 << SAR_PORT_ID );  /* add sar_port_id to disabled ports */
    ethsw_wreg(PAGE_CONTROL, REG_DISABLE_LEARNING,
            (uint8 *)&val16, sizeof(val16) );

    val8 = ( REG_PORT_STATE_OVERRIDE    /* software override Phy values */
            | REG_PORT_STATE_1000        /* Speed of 1000 Mbps */
            | REG_PORT_STATE_FDX         /* Full duplex mode */
            | REG_PORT_STATE_LNK );      /* Link State Up */
    ethsw_wreg(PAGE_CONTROL, REG_PORT_STATE + SAR_PORT_ID,
            &val8, sizeof(val8) );

    val8 = REG_PORT_TX_DISABLE;
    ethsw_wreg(PAGE_CONTROL, REG_PORT_CTRL + SAR_PORT_ID,
            &val8, sizeof(val8) );

    return 0;
}

/*
 *------------------------------------------------------------------------------
 * Function   : ethsw_disable_sar_port
 * Description: Setup the SAR port of a 6368 Switch as follows:
 *              - Link Sown
 *              - Rx Disabled, Tx Disabled.
 *
 * Design Note: Invoked via CMF Hook pktCmfSarPortDisable.
 *
 *              This function expects to be called from a softirq context.
 *------------------------------------------------------------------------------
 */
int ethsw_disable_sar_port(void)
{
    uint8  val8;

    val8 = REG_PORT_STATE_OVERRIDE;
    ethsw_wreg(PAGE_CONTROL, REG_PORT_STATE + SAR_PORT_ID,
            &val8, sizeof(val8) );

    val8 = (REG_PORT_TX_DISABLE | REG_PORT_RX_DISABLE);
    ethsw_wreg(PAGE_CONTROL, REG_PORT_CTRL + SAR_PORT_ID,
            &val8, sizeof(val8) );

    return 0;
}
#endif

int ethsw_enable_hw_switching(void)
{
    u8 i;

    /* restore pbvlan config */
    for(i = 0; i < TOTAL_SWITCH_PORTS; i++)
    {
        ethsw_wreg(PAGE_PORT_BASED_VLAN, REG_VLAN_CTRL_P0 + (i * 2),
                (uint8 *)&pbvlan_map[i], 2);
    }

    /* restore disable learning register */
    ethsw_wreg(PAGE_CONTROL, REG_DISABLE_LEARNING, (uint8 *)&dis_learning, 2);

    /* restore port forward control register */
    ethsw_wreg(PAGE_CONTROL, REG_PORT_FORWARD, (uint8 *)&port_fwd_ctrl, 1);

    i = 0;
    while (vnet_dev[i])
    {
        if (LOGICAL_PORT_TO_UNIT_NUMBER(VPORT_TO_LOGICAL_PORT(i)) != 0) /* Not Internal switch port */
        {
            i++;  /* Go to next port */
            continue;
        }
        /* When hardware switching is enabled, enable the Linux bridge to
           not to forward the bcast packets on hardware ports */
        vnet_dev[i++]->priv_flags |= IFF_HW_SWITCH;
    }
#if defined(ENET_GPON_CONFIG)
    for (i = 0; i < MAX_GEM_IDS; i++)
    {
        if (gponifid_to_dev[i])
        {
            /* When hardware switching is enabled, enable the Linux bridge to
               not to forward the bcast packets on hardware ports */
            gponifid_to_dev[i]->priv_flags |= IFF_HW_SWITCH;
        }
    }
#endif

    return 0;
}

int ethsw_disable_hw_switching(void)
{
    u8 i, byte_value;
    u16 reg_value;


    /* set the port-based vlan control reg of each port with fwding mask of
       only that port and MIPS. For MIPS port, set the forwarding mask of
       all the ports */
    for(i = 0; i < TOTAL_SWITCH_PORTS; i++)
    {
        ethsw_rreg(PAGE_PORT_BASED_VLAN, REG_VLAN_CTRL_P0 + (i * 2),
                (uint8 *)&pbvlan_map[i], 2);
        if (i == MIPS_PORT_ID)
        {
            reg_value = PBMAP_ALL;
        }
        else
        {
            reg_value = PBMAP_MIPS;
        }
        ethsw_wreg(PAGE_PORT_BASED_VLAN, REG_VLAN_CTRL_P0 + (i * 2),
                (uint8 *)&reg_value, 2);
    }

    /* Save disable_learning_reg setting */
    ethsw_rreg(PAGE_CONTROL, REG_DISABLE_LEARNING, (uint8 *)&dis_learning, 2);
    /* disable learning on all ports */
    reg_value = PBMAP_ALL;
    ethsw_wreg(PAGE_CONTROL, REG_DISABLE_LEARNING, (uint8 *)&reg_value, 2);

    /* Save port forward control setting */
    ethsw_rreg(PAGE_CONTROL, REG_PORT_FORWARD, (uint8 *)&port_fwd_ctrl, 1);
    /* flood unlearned packets */
    byte_value = 0x00;
    ethsw_wreg(PAGE_CONTROL, REG_PORT_FORWARD, (uint8 *)&byte_value, 1);

    i = 0;
    while (vnet_dev[i])
    {
        if (LOGICAL_PORT_TO_UNIT_NUMBER(VPORT_TO_LOGICAL_PORT(i)) != 0) /* Not Internal switch port */
        {
            i++;  /* Go to next port */
            continue;
        }
        /* When hardware switching is disabled, enable the Linux bridge to
           forward the bcast on hardware ports as well */
        vnet_dev[i++]->priv_flags &= ~IFF_HW_SWITCH;
    }

#if defined(ENET_GPON_CONFIG)
    for (i = 0; i < MAX_GEM_IDS; i++)
    {
        if (gponifid_to_dev[i])
        {
            /* When hardware switching is enabled, enable the Linux bridge to
               not to forward the bcast on hardware ports */
            gponifid_to_dev[i]->priv_flags &= ~IFF_HW_SWITCH;
        }
    }
#endif

    /* Flush arl table dynamic entries */
    fast_age_all(0);
    return 0;
}


int ethsw_switch_manage_ports_leds(int led_mode)
{
#define AUX_MODE_REG 0x1d
#define LNK_LED_DIS  4 // Bit4

    uint16 v16, i;

    down(&bcm_ethlock_switch_config);

    for (i=0; i<4; i++) {
        ethsw_phy_rreg(enet_sw_port_to_phyid(0, i), AUX_MODE_REG, &v16);

        if(led_mode)
            v16 &= ~(1 << LNK_LED_DIS);
        else
            v16 |= (1 << LNK_LED_DIS);

        ethsw_phy_wreg(enet_sw_port_to_phyid(0, i), AUX_MODE_REG, &v16);
    }

    up(&bcm_ethlock_switch_config);
    return 0;
}
EXPORT_SYMBOL(ethsw_switch_manage_ports_leds);

#if defined(CONFIG_BCM96828) && !defined(CONFIG_EPON_HGU)

static uint8 g_rx_port_to_iudma_init_cfg[MAX_SWITCH_PORTS] =
{
    PKTDMA_ETH_US_IUDMA  /* alls ports default to the US iuDMA channel */
};

void saveEthPortToRxIudmaConfig(uint8 port, uint8 iudma)
{
    if((port < MAX_SWITCH_PORTS) && (iudma < ENET_RX_CHANNELS_MAX))
    {
        g_rx_port_to_iudma_init_cfg[port] = iudma;
    }
    else
    {
        printk("%s : Invalid Argument: port <%d>, channel <%d>\n",
                __FUNCTION__, port, iudma);
    }
}

int restoreEthPortToRxIudmaConfig(uint8 port)
{
    if (port < LOGICAL_PORT_START || port > LOGICAL_PORT_END) {
        printk("%s : Invalid Argument: port <%d>\n", __FUNCTION__, port);
        return PKTDMA_ETH_US_IUDMA;
    }

    if (IsExternalSwitchPort(port))
    {
        port = BpGetPortConnectedToExtSwitch();
    } 
    else
    {
        port = LOGICAL_PORT_TO_PHYSICAL_PORT(port);
    }

    return g_rx_port_to_iudma_init_cfg[port];
}


void ethsw_set_port_to_fap_map(unsigned int portMap, int split_upstream)
{
    struct ethswctl_data e2;
    int i, j, swap = 0;

    for(i = 0; i < BP_MAX_SWITCH_PORTS; i++)
    {
        if (portMap & (1 << i)) {
            for(j = 0; j <= MAX_PRIORITY_VALUE; j++)
            {
                e2.type = TYPE_SET;
                e2.port = i;
                e2.priority = j;

                if (i == EPON_PORT_ID) {
                    e2.queue = PKTDMA_ETH_DS_IUDMA;
                }
                else {
                    if (split_upstream) {
                        if (swap) {
                            e2.queue = PKTDMA_ETH_DS_IUDMA;
                        } else {
                            e2.queue = PKTDMA_ETH_US_IUDMA;
                        }
                    } else {
                        e2.queue = PKTDMA_ETH_US_IUDMA;
                    }
                }
                saveEthPortToRxIudmaConfig(e2.port, e2.queue);
                mapEthPortToRxIudma(e2.port, e2.queue);
                bcmeapi_ioctl_ethsw_cosq_port_mapping(&e2);
            }
            if (swap) {
                swap = 0;
            } else {
                swap = 1;
            }
        }
    }
}

int epon_uni_to_uni_ctrl(unsigned int portMap, int val)
{
    int i = 0;
    uint8_t v8;
    uint16_t v16;

    if (val) {
        /* UNI to UNI communication is needed. So, DS also goes through FAP */
        v16 = 0x1FF;
        ethsw_wreg(PAGE_CONTROL, REG_DISABLE_LEARNING, (uint8 *)&v16, 2);

        v16 = 0x100;
        for (i=0; i < MAX_SWITCH_PORTS; i++) {
            ethsw_wreg(PAGE_PORT_BASED_VLAN, (i * 2), (uint8*)&v16, 2);
        }

        v8 = (REG_PORT_FORWARD_MCST | REG_PORT_FORWARD_UCST | REG_PORT_FORWARD_IP_MCST);
        ethsw_wreg(PAGE_CONTROL, REG_PORT_FORWARD, (uint8 *)&v8, sizeof(v8));
        v16 = PBMAP_MIPS;
        ethsw_wreg(PAGE_CONTROL, REG_UCST_LOOKUP_FAIL, (uint8 *)&v16, 2);
        ethsw_wreg(PAGE_CONTROL, REG_IPMC_LOOKUP_FAIL, (uint8 *)&v16, 2);
        ethsw_wreg(PAGE_CONTROL, REG_MCST_LOOKUP_FAIL, (uint8 *)&v16, 2);

        ethsw_rreg(PAGE_CONTROL, REG_SWITCH_MODE, (uint8 *)&v8, 1);
        v8 &= (~BROADCAST_TO_ONLY_MIPS);
        ethsw_wreg(PAGE_CONTROL, REG_SWITCH_MODE, (uint8 *)&v8, 1);

        ethsw_set_port_to_fap_map(portMap, 0);

        ethsw_rreg(PAGE_8021Q_VLAN, REG_VLAN_GLOBAL_8021Q, (uint8 *)&v8, 1);
        v8 &=  ~(1 << VLAN_EN_8021Q_S);
        ethsw_wreg(PAGE_8021Q_VLAN, REG_VLAN_GLOBAL_8021Q, (uint8 *)&v8, 1);

        fapL2flow_defaultVlanTagConfig(0);

        for (i=0; i < MAX_SWITCH_PORTS; i++) {
            if ((portMap & (1 << i)) && (i != EPON_PORT_ID))
                fapPkt_setFloodingMask(i, PBMAP_UNIS & (~(1 << i)), 0);
        }

        fapPkt_hwArlConfig(0, 0);

        uni_to_uni_enabled = 1;
    } else {
        /* UNI to UNI communication is NOT needed. So, resolved DS goes through hardware */
        v16 = 0x1FF;
        ethsw_wreg(PAGE_CONTROL, REG_DISABLE_LEARNING, (uint8 *)&v16, 2);

        v16 = PBMAP_MIPS;
        for (i=0; i < 7; i++) {
            ethsw_wreg(PAGE_PORT_BASED_VLAN, (i * 2), (uint8*)&v16, 2);
        }

        v16 = PBMAP_ALL;
        ethsw_wreg(PAGE_PORT_BASED_VLAN, (EPON_PORT_ID * 2), (uint8*)&v16, 2);

        v8 = (REG_PORT_FORWARD_MCST | REG_PORT_FORWARD_UCST | REG_PORT_FORWARD_IP_MCST);
        ethsw_wreg(PAGE_CONTROL, REG_PORT_FORWARD, (uint8 *)&v8, sizeof(v8));
        v16 = PBMAP_MIPS;
        ethsw_wreg(PAGE_CONTROL, REG_UCST_LOOKUP_FAIL, (uint8 *)&v16, 2);
        ethsw_wreg(PAGE_CONTROL, REG_IPMC_LOOKUP_FAIL, (uint8 *)&v16, 2);
        ethsw_wreg(PAGE_CONTROL, REG_MCST_LOOKUP_FAIL, (uint8 *)&v16, 2);

        ethsw_rreg(PAGE_CONTROL, REG_SWITCH_MODE, (uint8 *)&v8, 1);
        v8 |= BROADCAST_TO_ONLY_MIPS;
        ethsw_wreg(PAGE_CONTROL, REG_SWITCH_MODE, (uint8 *)&v8, 1);

        v8 = (1 << VID_FFF_ENABLE_S);
        ethsw_wreg(PAGE_8021Q_VLAN, REG_VLAN_GLOBAL_CTRL5, (uint8 *)&v8, 1);

        for (i = 0; i < MAX_SWITCH_PORTS; i++) {
            ethsw_rreg(PAGE_8021Q_VLAN, REG_VLAN_DEFAULT_TAG, (uint8 *)&v16, 2);
            v16 &= (~DEFAULT_TAG_VID_M);
            v16 |= (0xFFF << DEFAULT_TAG_VID_S);
            ethsw_wreg(PAGE_8021Q_VLAN, REG_VLAN_DEFAULT_TAG + (i*2), (uint8 *)&v16, 2);
        }

        ethsw_rreg(PAGE_8021Q_VLAN, REG_VLAN_GLOBAL_8021Q, (uint8 *)&v8, 1);
        v8 |=  (1 << VLAN_EN_8021Q_S);
        v8 |= (VLAN_IVL_SVL_M << VLAN_IVL_SVL_S);
        ethsw_wreg(PAGE_8021Q_VLAN, REG_VLAN_GLOBAL_8021Q, (uint8 *)&v8, 1);

        for (i = 0; i < 4095; i++) {
            write_vlan_table(i, 0x1ff);
        }
        write_vlan_table(0xfff, 0x1ff | (0x1ff<<9));

        fapL2flow_defaultVlanTagConfig(1);

        for (i=0; i < MAX_SWITCH_PORTS; i++) {
            if ((portMap & (1 << i)) && (i != EPON_PORT_ID))
                fapPkt_setFloodingMask(i, PBMAP_EPON, 0);
        }

        ethsw_set_port_to_fap_map(portMap, 1);

        fapPkt_hwArlConfig(1, PBMAP_UNIS);

        uni_to_uni_enabled = 0;
    }

    fapPkt_mcastSetMissBehavior(1);

    return BCM_E_NONE;
}

int enet_learning_ctrl(uint32_t portMask, uint8_t enable)
{
    uint16_t v16;

    BCM_ENET_DEBUG("Given portMask: %02d \n ", portMask);
    if (portMask > PBMAP_ALL) {
        BCM_ENET_DEBUG("Invalid portMask: %02d \n ", portMask);
        return -1;
    }

    ethsw_rreg(PAGE_CONTROL, REG_DISABLE_LEARNING, (uint8 *)&v16, 2);
    if (enable) {
        v16 &= ~(portMask);
    } else {
        v16 |= portMask;
    }
    ethsw_wreg(PAGE_CONTROL, REG_DISABLE_LEARNING, (uint8 *)&v16, 2);

    return BCM_E_NONE;
}

int enet_ioctl_ethsw_port_traffic_control(struct ethswctl_data *e)
{
    uint32_t val32;
    uint8_t v8;

    BCM_ENET_DEBUG("Given port: %02d \n ", e->port);
    if (e->type == TYPE_GET) {
        ethsw_rreg(PAGE_CONTROL, REG_PORT_CTRL + e->port, &v8, 1);
        /* Get the enable/disable status */
        val32 = v8 & (REG_PORT_CTRL_DISABLE);
        if (copy_to_user((void*)(&e->ret_val), (void*)&val32, sizeof(int))) {
            return -EFAULT;
        }
        BCM_ENET_DEBUG("The port ctrl status: %02d \n ", e->ret_val);
    } else {
        BCM_ENET_DEBUG("Given port control: %02x \n ", e->val);
        ethsw_rreg(PAGE_CONTROL, REG_PORT_CTRL + e->port, &v8, 1);
        v8 &= (~REG_PORT_CTRL_DISABLE);
        v8 |= (e->val & REG_PORT_CTRL_DISABLE);
        BCM_ENET_DEBUG("Writing = 0x%x to REG_PORT_CTRL", v8);
        ethsw_wreg(PAGE_CONTROL, REG_PORT_CTRL + e->port, &v8, 1);
        /* 
         * Fix for 6816, link not coming up after removing REG_PORT_RX_DISABLE
         * SWBCACPE-9012 - scenario:
         * ethswctl -c portctrl -p <> -v 3 
         * ethswctl -c test -t 3 // reset switch
         * ethswctl -c portctrl -p <> -v 0 
         * The fix is to: in port state override register mark link up if phy says so
         * 
         */
        if (!(e->val & REG_PORT_CTRL_DISABLE))
        {
            /* update link status bit in port state override sw register */
            uint8 port_ctrl;
            uint16 v16;
            int phy_id;
            phy_id = ETHSW_PHY_GET_PHYID(e->port); // internal sw ports only
            ethsw_phy_rreg(phy_id, MII_BMSR, &v16);
            if (!(v16 & BMSR_LSTATUS)) return BCM_E_NONE;
            ethsw_rreg(PAGE_CONTROL, REG_PORT_STATE + e->port, &port_ctrl, 1);
            port_ctrl |= REG_PORT_STATE_LNK;
            ethsw_wreg(PAGE_CONTROL, REG_PORT_STATE + e->port, &port_ctrl, 1);
        }
    }

    return BCM_E_NONE;
}

int bcm_fun_enet_drv_handler(void *ptr)
{
    BCM_EnetHandle_t *pParam = (BCM_EnetHandle_t *)ptr;

    switch (pParam->type) {
        case BCM_ENET_FUN_TYPE_LEARN_CTRL:
            enet_learning_ctrl(pParam->portMask, pParam->enable);
            break;

        case BCM_ENET_FUN_TYPE_ARL_WRITE:
            enet_arl_write(pParam->arl_entry.mac, pParam->arl_entry.vid, pParam->arl_entry.val);
            break;

        case BCM_ENET_FUN_TYPE_AGE_PORT:
            fast_age_port(pParam->port, 0);
            break;

        case BCM_ENET_FUN_TYPE_PORT_RX_CTRL:
            {
                struct ethswctl_data e;
                e.type = TYPE_SET;
                e.port = pParam->port;
                e.val = (pParam->enable)?0:1;
                enet_ioctl_ethsw_port_traffic_control(&e);
            }
            break;

        case BCM_ENET_FUN_TYPE_UNI_UNI_CTRL:
            {
                epon_uni_to_uni_ctrl(pVnetDev0_g->EnetInfo[0].sw.port_map, pParam->enable);
            }
            break;

        case BCM_ENET_FUN_TYPE_GET_VPORT_CNT:
            pParam->uniport_cnt = vport_cnt - 1;
            break;

        case BCM_ENET_FUN_TYPE_GET_IF_NAME_OF_VPORT:
            strcpy(pParam->name, vnet_dev[pParam->port]->name);
            break;

        case BCM_ENET_FUN_TYPE_GET_UNIPORT_MASK:
            pParam->portMask  = pVnetDev0_g->EnetInfo[0].sw.port_map & (~(1 << EPON_PORT_ID));
            break;

        default:
            BCM_ENET_DEBUG("%s: Invalid type \n", __FUNCTION__);
            break;
    }

    return 0;
}
#endif

/* port = physical port */
int ethsw_phy_intr_ctrl(int port, int on)
{
    uint16 v16;
    int phyId = enet_sw_port_to_phyid(0, port);

    down(&bcm_ethlock_switch_config);

#if defined(CONFIG_BCM96328) || defined(CONFIG_BCM96362) || defined(CONFIG_BCM96368) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM96828) || defined(CONFIG_BCM96818) || defined(CONFIG_BCM96318) || defined(CONFIG_BCM963138) | defined(CONFIG_BCM963381) || defined(CONFIG_BCM963148)
    if (on != 0)
        v16 = MII_INTR_ENABLE | MII_INTR_FDX | MII_INTR_SPD | MII_INTR_LNK;
    else
        v16 = 0;

    ethsw_phy_wreg(phyId, MII_INTERRUPT, &v16);
#endif

#if defined(CONFIG_BCM963268) ||  defined(CONFIG_BCM96828)
#if defined(CONFIG_BCM963268)
    if (port == GPHY_PORT_ID)
#elif defined(CONFIG_BCM96828)
        if ((port == GPHY1_PORT_ID) || (port == GPHY2_PORT_ID))
#endif
        {
            if (on != 0)
                v16 = ~(MII_INTR_FDX | MII_INTR_SPD | MII_INTR_LNK);
            else
                v16 = 0xFFFF;

            ethsw_phy_wreg(phyId, MII_INTERRUPT_MASK, &v16);
        }
#endif

    up(&bcm_ethlock_switch_config);

    return 0;
}

void ethsw_port_mirror_get(int *enable, int *mirror_port, unsigned int *ing_pmap,
                           unsigned int *eg_pmap, unsigned int *blk_no_mrr,
                           int *tx_port, int *rx_port)
{
    uint16 v16;
    ethsw_rreg(PAGE_MANAGEMENT, REG_MIRROR_CAPTURE_CTRL,  (uint8*)&v16, sizeof(v16));
    if (v16 & REG_MIRROR_ENABLE)
    {
        *enable = 1;
        *mirror_port = v16 & REG_CAPTURE_PORT_M;
        *blk_no_mrr = v16 & REG_BLK_NOT_MIRROR;
        ethsw_rreg(PAGE_MANAGEMENT, REG_MIRROR_INGRESS_CTRL, (uint8*)&v16, sizeof(v16));
        *ing_pmap = v16 & REG_INGRESS_MIRROR_M;
        ethsw_rreg(PAGE_MANAGEMENT, REG_MIRROR_EGRESS_CTRL, (uint8*)&v16, sizeof(v16));
        *eg_pmap = v16 & REG_EGRESS_MIRROR_M;
    }
    else
    {
        *enable = 0;
    }
}
void ethsw_port_mirror_set(int enable, int mirror_port, unsigned int ing_pmap, 
                           unsigned int eg_pmap, unsigned int blk_no_mrr, 
                           int tx_port, int rx_port)
{
    uint16 v16;
    if (enable)
    {
        v16 = REG_MIRROR_ENABLE;
        v16 |= (mirror_port & REG_CAPTURE_PORT_M);
        v16 |= blk_no_mrr?REG_BLK_NOT_MIRROR:0;

        ethsw_wreg(PAGE_MANAGEMENT, REG_MIRROR_CAPTURE_CTRL, (uint8*)&v16, sizeof(v16));
        v16 = ing_pmap & REG_INGRESS_MIRROR_M;
        ethsw_wreg(PAGE_MANAGEMENT, REG_MIRROR_INGRESS_CTRL, (uint8*)&v16, sizeof(v16));
        v16 = eg_pmap & REG_INGRESS_MIRROR_M;
        ethsw_wreg(PAGE_MANAGEMENT, REG_MIRROR_EGRESS_CTRL, (uint8*)&v16, sizeof(v16));
    }
    else
    {
        v16  = 0;
        ethsw_wreg(PAGE_MANAGEMENT, REG_MIRROR_CAPTURE_CTRL, (uint8*)&v16, sizeof(v16));
    }
}

MODULE_LICENSE("GPL");

