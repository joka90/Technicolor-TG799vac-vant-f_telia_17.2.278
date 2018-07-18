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

/*
    Shared code of doing switch basic initialization, shared by
    CFE and Ethernet driver.
    By default, CFE does not do network initialization except 
    entering CFE prompt.
    Ethernet driver calls internal switch initialization here
*/

#ifdef _CFE_                                                

/* 
    Ethernet driver has full set of doing external switch initialization,
    so only CFE needs this so if CFE does network initialization.
*/
#define _EXT_SWITCH_INIT_

#include "bcm_map.h"
#include "lib_types.h"
#include "lib_malloc.h"
#include "lib_string.h"
#include "lib_printf.h"
#include "cfe_timer.h"
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

#ifdef _EXT_SWITCH_INIT_
static int ext_switch_init(void);
static int ethsw_spi_ss_id(int *bus_num);
static void ethsw_spi_select(int page);
static void ethsw_spi_rreg(int page, int reg, uint8 *data, int len);
static void ethsw_spi_wreg(int page, int reg, uint8 *data, int len);
#endif

#include "shared_utils.h"
#include "mii_shared.h"
#include "bcmSpiRes.h"
#include "robosw_reg.h"
#if defined(_BCM963381_) || defined(CONFIG_BCM963381)
#include "pmc_switch.h"
#endif

static void phy_advertise_caps(unsigned int phy_id);
static void phy_apply_init_bp(int port);
void ethsw_rreg_ext(int access_type, int page, int reg, uint8 *data, int len);
void ethsw_wreg_ext(int access_type, int page, int reg, uint8 *data, int len);
uint32 mii_read_ext(uint32 uPhyAddr, uint32 uRegAddr, uint32 external);
void mii_write_ext(uint32 uPhyAddr, uint32 uRegAddr, uint32 data, uint32 external);
uint32 extsw_phyport_rreg(int access_type, int port, int reg);
void extsw_phyport_wreg(int access_type, int port, int reg, uint16 data);


static const ETHERNET_MAC_INFO* EnetInfo;
static uint16 PortLinkState[BP_MAX_SWITCH_PORTS];

/* read a value from the MII */
uint32 mii_read_ext(uint32 uPhyAddr, uint32 uRegAddr, uint32 external) 
{
    SWITCH->MdioCtrl = 0x0;
    SWITCH->MdioCtrl = MdioCtrl_Read | external |
        ((uPhyAddr << MdioCtrl_ID_Shift) & MdioCtrl_ID_Mask) | 
        (uRegAddr << MdioCtrl_Addr_Shift);
    udelay(100);
    return SWITCH->MdioData;
}

/* write a value to the MII */
void mii_write_ext(uint32 uPhyAddr, uint32 uRegAddr, uint32 data, uint32 external)
{
    SWITCH->MdioCtrl = 0x0;
    SWITCH->MdioCtrl = MdioCtrl_Write | external |
        ((uPhyAddr << MdioCtrl_ID_Shift) & MdioCtrl_ID_Mask) | 
        (uRegAddr << MdioCtrl_Addr_Shift) | data;
    udelay(100);
}

uint32 mii_read(uint32 uPhyAddr, uint32 uRegAddr) 
{
    return mii_read_ext(uPhyAddr, uRegAddr, (IsExtPhyId(uPhyAddr) ? MdioCtrl_Ext : 0));
}

/* write a value to the MII */
void mii_write(uint32 uPhyAddr, uint32 uRegAddr, uint32 data)
{
    mii_write_ext(uPhyAddr, uRegAddr, data, (IsExtPhyId(uPhyAddr) ? MdioCtrl_Ext : 0));
}

#ifdef _EXT_SWITCH_INIT_
static int clkHz = 781000;
static int ethsw_spi_ss_id(int *bus_num)
{
    int slave_select;

    *bus_num = LEG_SPI_BUS_NUM;
    switch(EnetInfo[1].usConfigType) {
        case BP_ENET_CONFIG_SPI_SSB_0:
            slave_select = 0;
            break;
        case BP_ENET_CONFIG_SPI_SSB_1:
            slave_select = 1;
            break;
        case BP_ENET_CONFIG_SPI_SSB_2:
            slave_select = 2;
            break;
        case BP_ENET_CONFIG_SPI_SSB_3:
            slave_select = 3;
            break;

        case BP_ENET_CONFIG_HS_SPI_SSB_0:
        case BP_ENET_CONFIG_HS_SPI_SSB_1:
        case BP_ENET_CONFIG_HS_SPI_SSB_2:
        case BP_ENET_CONFIG_HS_SPI_SSB_3:
        case BP_ENET_CONFIG_HS_SPI_SSB_4:
        case BP_ENET_CONFIG_HS_SPI_SSB_5:
        case BP_ENET_CONFIG_HS_SPI_SSB_6:
        case BP_ENET_CONFIG_HS_SPI_SSB_7:
            *bus_num = HS_SPI_BUS_NUM;
            slave_select = EnetInfo[1].usConfigType - BP_ENET_CONFIG_HS_SPI_SSB_0;
            break;

        default:
            slave_select = 1;
            printk("Error: Invalid SPI_SS in usConfigType, Assuming 1\n");
            break;
    }

#if defined(_BCM96328_) || defined(CONFIG_BCM96328) || defined(_BCM96318_) || defined(CONFIG_BCM96318)
    if (*bus_num != HS_SPI_BUS_NUM) {
        printk("Error: Invalid Board Config. 6328 supports only HS SPI. Assuming HS SPI and continuing...\n");
        *bus_num = HS_SPI_BUS_NUM;
    }
#endif

    return slave_select;
}

static void ethsw_spi_select(int page)
{
    unsigned char buf[3];
    int spi_ss, cid = 0, bus_num;
    int tryCount = 0;
    static int spiRdyErrCnt = 0;

    spi_ss = ethsw_spi_ss_id(&bus_num);

    /* SPIF status bit must be clear */
    while(1)
    {
       buf[0] = BCM5325_SPI_CMD_NORMAL | BCM5325_SPI_CMD_READ |
           ((cid & BCM5325_SPI_CHIPID_MASK) << BCM5325_SPI_CHIPID_SHIFT);
       buf[1] = (unsigned char)BCM5325_SPI_STS;
       BcmSpi_Read(buf, BCM5325_SPI_PREPENDCNT, 1, bus_num, spi_ss, clkHz);
       if (buf[0] & BCM5325_SPI_CMD_SPIF)
       {
          if ( spiRdyErrCnt < 10 )
          {
             spiRdyErrCnt++;
             printk("ethsw_spi_select: SPIF set, not ready\n");
          }
          else if ( 10 == spiRdyErrCnt )
          {
             spiRdyErrCnt++;
             printk("ethsw_spi_select: SPIF set, not ready - suppressing prints\n");
          }
          tryCount++;
          if (tryCount > 10)
          {
             return;
          }
       }
       else
       {
          break;
       }
    }

    /* Select new chip */
    buf[0] = BCM5325_SPI_CMD_NORMAL | BCM5325_SPI_CMD_WRITE |
        ((cid & BCM5325_SPI_CHIPID_MASK) << BCM5325_SPI_CHIPID_SHIFT);

    /* Select new page */
    buf[1] = PAGE_SELECT;
    buf[2] = (char)page;
    BcmSpi_Write(buf, sizeof(buf), bus_num, spi_ss, clkHz);
}

static void ethsw_spi_rreg(int page, int reg, uint8 *data, int len)
{
    unsigned char buf[64];
    int rc;
    int i;
    int max_check_spi_sts;
    int prependCnt = BCM5325_SPI_PREPENDCNT, spi_ss, cid = 0, bus_num;

    spi_ss = ethsw_spi_ss_id(&bus_num);

    ethsw_spi_select(page);

    /* write command byte and register address */
    buf[0] = BCM5325_SPI_CMD_NORMAL | BCM5325_SPI_CMD_READ |
        ((cid & BCM5325_SPI_CHIPID_MASK) << BCM5325_SPI_CHIPID_SHIFT);
    buf[1] = (unsigned char)reg;
    rc = BcmSpi_Read(buf, prependCnt, 1, bus_num, spi_ss, clkHz);

    if (rc == SPI_STATUS_OK) {
        max_check_spi_sts = 0;
        do {
            /* write command byte and read spi_sts address */
            buf[0] = BCM5325_SPI_CMD_NORMAL | BCM5325_SPI_CMD_READ |
                ((cid & BCM5325_SPI_CHIPID_MASK) << BCM5325_SPI_CHIPID_SHIFT);
            buf[1] = (unsigned char)BCM5325_SPI_STS;
            rc = BcmSpi_Read(buf, prependCnt, 1, bus_num, spi_ss, clkHz);
            if (rc == SPI_STATUS_OK) {
                /* check the bit 0 RACK bit is set */
                if (buf[0] & BCM5325_SPI_CMD_RACK) {
                    break;
                }
                udelay(10000);
            } else {
                break;
            }
        } while (max_check_spi_sts++ < 10);

        if (rc == SPI_STATUS_OK) {
            buf[0] = BCM5325_SPI_CMD_NORMAL | BCM5325_SPI_CMD_READ |
               ((cid & BCM5325_SPI_CHIPID_MASK) << BCM5325_SPI_CHIPID_SHIFT);
            buf[1] = (unsigned char)0xf0;
            rc = BcmSpi_Read(buf, prependCnt, len, bus_num, spi_ss, clkHz);
            if (rc == SPI_STATUS_OK) {
                for (i = 0; i < len; i++)
                    *(data + (len - i - 1)) = buf[i];
            }
        }
    }
}

static void ethsw_spi_wreg(int page, int reg, uint8 *data, int len)
{
    unsigned char buf[64];
    int i;
    int spi_ss, cid = 0, bus_num;

    ethsw_spi_select(page);

    spi_ss = ethsw_spi_ss_id(&bus_num);
    buf[0] = BCM5325_SPI_CMD_NORMAL | BCM5325_SPI_CMD_WRITE |
        ((cid & BCM5325_SPI_CHIPID_MASK) << BCM5325_SPI_CHIPID_SHIFT);

    buf[1] = (char)reg;
    for (i = 0; i < len; i++) {
        /* Write the data out in LE format to the switch */
        buf[BCM5325_SPI_PREPENDCNT+i] = *(data + (len - i - 1));
    }
    BcmSpi_Write(buf, len+BCM5325_SPI_PREPENDCNT, bus_num, spi_ss, clkHz);
}

/* External switch register access through MDC/MDIO */
static void ethsw_mdio_rreg(int page, int reg, uint8 *data, int len)
{
    uint32 cmd, res, ret;
    int max_retry = 0;

    cmd = (page << REG_PPM_REG16_SWITCH_PAGE_NUMBER_SHIFT) | REG_PPM_REG16_MDIO_ENABLE;
    mii_write(PSEUDO_PHY_ADDR, REG_PSEUDO_PHY_MII_REG16, cmd);

    cmd = (reg << REG_PPM_REG17_REG_NUMBER_SHIFT) | REG_PPM_REG17_OP_READ;
    mii_write(PSEUDO_PHY_ADDR, REG_PSEUDO_PHY_MII_REG17, cmd);

    do {
        res = mii_read(PSEUDO_PHY_ADDR, REG_PSEUDO_PHY_MII_REG17);
        udelay(10);
    } while (((res & (REG_PPM_REG17_OP_WRITE|REG_PPM_REG17_OP_READ)) != REG_PPM_REG17_OP_DONE) &&
              (max_retry++ < 5));

    ret = 0;
    ret |= mii_read(PSEUDO_PHY_ADDR, REG_PSEUDO_PHY_MII_REG24) << 0;
    ret |= mii_read(PSEUDO_PHY_ADDR, REG_PSEUDO_PHY_MII_REG25) << 16;
    switch (len) {
        case 1:
            *data = (uint8)ret;
            break;
        case 2:
            *(uint16 *)data = (uint16)ret;
            break;
        case 4:
            *(uint32 *)data = ret;
            break;
    }
}

static void ethsw_mdio_wreg(int page, int reg, uint8 *data, int len)
{
    uint32 cmd, res;
    uint32 val = 0;
    int max_retry = 0;

    switch (len) {
        case 1:
            val = *data;
            break;
        case 2:
            val = *(uint16 *)data;
            break;
        case 4:
            val = *(uint32 *)data;
            break;
    }
    cmd = (page << REG_PPM_REG16_SWITCH_PAGE_NUMBER_SHIFT) | REG_PPM_REG16_MDIO_ENABLE;
    mii_write(PSEUDO_PHY_ADDR, REG_PSEUDO_PHY_MII_REG16, cmd);

    cmd = val>>0 & 0xffff;
    mii_write(PSEUDO_PHY_ADDR, REG_PSEUDO_PHY_MII_REG24, cmd);
    cmd = val>>16 & 0xffff;
    mii_write(PSEUDO_PHY_ADDR, REG_PSEUDO_PHY_MII_REG25, cmd);
    cmd = 0;
    mii_write(PSEUDO_PHY_ADDR, REG_PSEUDO_PHY_MII_REG26, cmd);
    cmd = 0;
    mii_write(PSEUDO_PHY_ADDR, REG_PSEUDO_PHY_MII_REG27, cmd);

    cmd = (reg << REG_PPM_REG17_REG_NUMBER_SHIFT) | REG_PPM_REG17_OP_WRITE;
    mii_write(PSEUDO_PHY_ADDR, REG_PSEUDO_PHY_MII_REG17, cmd);

    do {
        res = mii_read(PSEUDO_PHY_ADDR, REG_PSEUDO_PHY_MII_REG17);
        udelay(10);
    } while (((res & (REG_PPM_REG17_OP_WRITE|REG_PPM_REG17_OP_READ)) != REG_PPM_REG17_OP_DONE) &&
                (max_retry++ < 5));
}

void ethsw_rreg_ext(int access_type, int page, int reg, uint8 *data, int len)
{
    if (access_type == MDIO_BUS) {
        ethsw_mdio_rreg(page, reg, data, len);

    } else {
        ethsw_spi_rreg(page, reg, data, len);
    }
}

void ethsw_wreg_ext(int access_type, int page, int reg, uint8 *data, int len)
{
    if (access_type == MDIO_BUS) {
        ethsw_mdio_wreg(page, reg, data, len);
    } else {
        ethsw_spi_wreg(page, reg, data, len);
    }
}

uint32 extsw_phyport_rreg(int access_type, int port, int reg)
{
    int phy_id;
    uint8 val[2];
    uint32 res;

    if (access_type == MDIO_BUS) {
        phy_id = EnetInfo[1].sw.phy_id[port];            
        res = mii_read_ext(phy_id, reg, MdioCtrl_Ext);
    } else {
        ethsw_spi_rreg(0x10+port, reg*2, val, 2);
        res = ((val[0] << 8) | val[1]);
    }
    return res;
}

void extsw_phyport_wreg(int access_type, int port, int reg, uint16 data)
{
    int phy_id;
    uint8 val[2];

    if (access_type == MDIO_BUS) {
        phy_id = EnetInfo[1].sw.phy_id[port];            
        mii_write_ext(phy_id, reg, data, MdioCtrl_Ext);
    } else {
        val[0] = (data&0xFF00)>>8;
        val[1] = data&0x00FF;
        ethsw_spi_wreg(0x10+port, reg*2, val, 2);
    }
}

#define BCM53115 0x53115
#define BCM53125 0x53125
#define BCM53128 0x53128
#define BCM53125_REG_IMP_RGMII_DELAY 0x60
#define BCM53125_RGMII_TXCLK_DELAY   1

static inline uint8
bitmap_to_number(uint8 bitmap)
{
    uint8 i;

    for(i=0; i<8 && !(bitmap &(1<<i)); i++)
        ;
    return i;
}

int ext_switch_init(void)
{
    uint8 data8;
    uint32 data32, access_type;
    int phy_id;
    int spi_ss, bus_num;

    switch (EnetInfo[1].usConfigType) {
        case BP_ENET_CONFIG_SPI_SSB_0:
        case BP_ENET_CONFIG_SPI_SSB_1:
        case BP_ENET_CONFIG_SPI_SSB_2:
        case BP_ENET_CONFIG_SPI_SSB_3:
        case BP_ENET_CONFIG_HS_SPI_SSB_0:
        case BP_ENET_CONFIG_HS_SPI_SSB_1:
        case BP_ENET_CONFIG_HS_SPI_SSB_2:
        case BP_ENET_CONFIG_HS_SPI_SSB_3:
        case BP_ENET_CONFIG_HS_SPI_SSB_4:
        case BP_ENET_CONFIG_HS_SPI_SSB_5:
        case BP_ENET_CONFIG_HS_SPI_SSB_6:
        case BP_ENET_CONFIG_HS_SPI_SSB_7:
            spi_ss = ethsw_spi_ss_id(&bus_num);
            BcmSpi_SetCtrlState(bus_num, spi_ss, SPI_MODE_3, 
                                SPI_CONTROLLER_STATE_GATE_CLK_SSOFF);
            access_type = SPI_BUS;
            break;

        case BP_ENET_CONFIG_MDIO:
        case BP_ENET_CONFIG_MDIO_PSEUDO_PHY:
            access_type = MDIO_BUS;
            break;
            
        default:
            printk("Unknown PHY configuration type\n");
            return -1;
    }

    ethsw_rreg_ext(access_type, PAGE_MANAGEMENT, REG_DEVICE_ID, (uint8 *)&data32, sizeof(data32));
    printk("External switch id = %x \n", (int)data32);

    if ((data32 == BCM53115) || (data32 == BCM53125) || (data32 == BCM53128))
    {
        int i, port=5;

	if (data32 == BCM53128) {
           port=8;
        }
        
        /* setup Switch MII1 port state override */
        data8 = (REG_CONTROL_MPSO_MII_SW_OVERRIDE |
            REG_CONTROL_MPSO_SPEED1000 |
            REG_CONTROL_MPSO_FDX |
            REG_CONTROL_MPSO_LINKPASS);
        ethsw_wreg_ext(access_type, PAGE_CONTROL, 
            REG_CONTROL_MII1_PORT_STATE_OVERRIDE, &data8, sizeof(data8));
        /* management mode, enable forwarding */
        ethsw_rreg_ext(access_type, PAGE_CONTROL, REG_SWITCH_MODE, &data8, sizeof(data8));
        data8 |= REG_SWITCH_MODE_FRAME_MANAGE_MODE | REG_SWITCH_MODE_SW_FWDG_EN;
        ethsw_wreg_ext(access_type, PAGE_CONTROL, 
            REG_SWITCH_MODE, &data8, sizeof(data8));
        /* Enable IMP Port */
        data8 = ENABLE_MII_PORT;
        ethsw_wreg_ext(access_type, PAGE_MANAGEMENT, 
            REG_GLOBAL_CONFIG, &data8, sizeof(data8));
        /* Disable BRCM Tag for IMP */
        data8 = 0; //~REG_BRCM_HDR_ENABLE;
        ethsw_wreg_ext(access_type, PAGE_MANAGEMENT, 
            REG_BRCM_HDR_CTRL, &data8, sizeof(data8));
        /* enable rx bcast, ucast and mcast of imp port */
        data8 = (REG_MII_PORT_CONTROL_RX_UCST_EN |
            REG_MII_PORT_CONTROL_RX_MCST_EN | 
            REG_MII_PORT_CONTROL_RX_BCST_EN);
        ethsw_wreg_ext(access_type, PAGE_CONTROL, REG_MII_PORT_CONTROL, &data8, sizeof(data8));

        for (i = 4; i < EPHY_PORTS; i++) {      
            phy_id = EnetInfo[0].sw.phy_id[i];            
                   
            if ((EnetInfo[0].sw.port_map & (1 << i)) && ((data32 == BCM53125) || (data32 == BCM53128)) && IsRGMII(phy_id)) {            
              ethsw_rreg_ext(access_type, PAGE_CONTROL, BCM53125_REG_IMP_RGMII_DELAY, &data8, sizeof(data8));
              data8 |= BCM53125_RGMII_TXCLK_DELAY;
              ethsw_wreg_ext(access_type, PAGE_CONTROL, BCM53125_REG_IMP_RGMII_DELAY, &data8, sizeof(data8));
            }
        }

        /* Enable APD compatibility bit on all ports for the 53125 */
        if ((data32 == BCM53125) || (data32 == BCM53128)) {
           for (i = 0; i < port; i++) {      
              if ((EnetInfo[1].sw.port_map & (1 << i)) != 0) {
                 extsw_phyport_wreg(access_type, i, MII_REGISTER_1C, 0xa921);
              }
           }
        }
	
	/* Init STP state to forwarding */
	for(i=0;i<port;i++)
	{
		data8 = 0xa0;
		ethsw_wreg_ext(access_type, PAGE_CONTROL, i, &data8, sizeof(data8));
	}	
    }

    return 0;
}
#endif /* _EXT_SWITCH_INIT_ */

#if defined(_BCM963381_) || defined(CONFIG_BCM963381)
static void ethsw_ephy_write_bank2_reg(int phy_id, uint16 reg, uint16 data)
{
    mii_write(phy_id, 0xe, reg);
    mii_write(phy_id, 0xf, data);
}

static void ephy_adjust_afe(unsigned int phy_id)
{
    uint32 chipRev = UtilGetChipRev();

    /* based on JIRA http://jira.broadcom.com/browse/HW63381-104 */
    /* these EPHY AFE tuning only apply to A0 and A1 */
    if ( (chipRev&0xf0) == 0xa0 ) {
        // Shadow Bank 2 enable
        mii_write(phy_id, 0x1f, 0xf);
        //Zero out internal and external current trim values
        mii_write(phy_id, 0x1a, 0x3b80);
        //Write analog control registers
        //AFE_RXCONFIG_0
        ethsw_ephy_write_bank2_reg(phy_id, 0xc0, 0xeb15);
        //AFE_RXCONFIG_1
        ethsw_ephy_write_bank2_reg(phy_id, 0xc1, 0x9a07);
        //AFE_TX_CONFIG
        ethsw_ephy_write_bank2_reg(phy_id, 0xc4, 0x0800);
        //AFE_RX_LP_COUNTER
        ethsw_ephy_write_bank2_reg(phy_id, 0xc3, 0x7fc4);
        //AFE_HPF_TRIM_OTHERS
        ethsw_ephy_write_bank2_reg(phy_id, 0xc8, 0x000b);
        //AFE PLLCTRL_4
        ethsw_ephy_write_bank2_reg(phy_id, 0x8c, 0x0e20);
        //Reset RC_CAL, R_CAL Engine
        ethsw_ephy_write_bank2_reg(phy_id, 0x23, 0x0006);
        //Disable Reset RC_CAL, R_CAL Engine
        ethsw_ephy_write_bank2_reg(phy_id, 0x23, 0x0000);
        //APD Workaround
        ethsw_ephy_write_bank2_reg(phy_id, 0x21, 0x0080);
        // Shadow Bank 2 disable
        mii_write(phy_id, 0x1f, 0xb);
    } else {
        // Shadow Bank 2 enable
        mii_write(phy_id, 0x1f, 0xf);
        // Set internal and external current trim values INT_trim = -2, Ext_trim =0
        mii_write(phy_id, 0x1a, 0x3be0);
        // Cal reset
        mii_write(phy_id, 0xe, 0x23);
        mii_write(phy_id, 0xf, 0x6);
        // Cal reset disable
        mii_write(phy_id, 0xe, 0x23);
        mii_write(phy_id, 0xf, 0x0);
        //AFE_RX_CONFIG_2 NTLT = +1 code HT = +3 code
        mii_write(phy_id, 0xe, 0xc2);
        mii_write(phy_id, 0xf, 0xd003);
        //AFE_TX_CONFIG, Set 1000BT Cfeed=110 for all ports
        mii_write(phy_id, 0xe, 0xc4);
        mii_write(phy_id, 0xf, 0x61);
        //AFE_VDAC_OTHERS_0, Set 1000BT Cidac=010 for all ports
        mii_write(phy_id, 0xe, 0xc7);
        mii_write(phy_id, 0xf, 0xa020);
        //AFE_HPF_TRIM_OTHERS Amplitude -3.2% and HT = +3 code
        mii_write(phy_id, 0xe, 0xc8);
        mii_write(phy_id, 0xf, 0xb3);
        // Shadow Bank 2 disable
        mii_write(phy_id, 0x1f, 0xb);
    }
}
#endif

void robosw_init(void)
{
    int port;
#if defined(_BCM96818_) || defined(CONFIG_BCM96818)
    uint32 miipadctl;
#endif

    EnetInfo = BpGetEthernetMacInfoArrayPtr();

#if !defined(_BCM96818_) && !defined(CONFIG_BCM96818)
#if defined(EPHY_RST_MASK)
#if defined(_BCM96368_) || defined(CONFIG_BCM96368)
	// Power up and reset EPHYs on 6368
        GPIO->RoboswEphyCtrl = 0;
#else
	// Just reset EPHYs 
        GPIO->RoboswEphyCtrl &= ~(EPHY_RST_MASK);
#endif
        udelay(1000);

        // Take EPHYs out of reset
        GPIO->RoboswEphyCtrl |= (EPHY_RST_MASK);
        udelay(1000);
#endif
#endif

#if defined(GPHY_EEE_1000BASE_T_DEF)
    // Enable EEE on internal GPHY
    GPIO->RoboswGphyCtrl |= ( GPHY_LPI_FEATURE_EN_DEF_MASK |
                              GPHY_EEE_1000BASE_T_DEF | GPHY_EEE_100BASE_TX_DEF |
                              GPHY_EEE_PCS_1000BASE_T_DEF | GPHY_EEE_PCS_100BASE_TX_DEF );
#endif

#if defined(_BCM963268_) || defined(CONFIG_BCM963268)
    // Take GPHY out of low pwer and disable IDDQ
    GPIO->RoboswGphyCtrl &= ~( GPHY_IDDQ_BIAS | GPHY_LOW_PWR );
    udelay(1000);

    // Bring internal GigPhy out of reset
    PERF->softResetB &= ~SOFT_RST_GPHY;
    udelay(1000);
    PERF->softResetB |= SOFT_RST_GPHY;
    udelay(1000);
#endif

#if defined(_BCM96818_) || defined(CONFIG_BCM96818)
    // Change default value of unused feature (MII HSTL mode) from enabled to disabled
    // workaround hw issue. when read, the mii_over_gpio upper 4 bits are shift by 1 bit to the right.
    miipadctl = GPIO->MiiPadCtrl;
    miipadctl = ((miipadctl&MII_OVER_GPIO_RD_MASK)<<1) | (miipadctl&~MII_OVER_GPIO_RD_MASK);
    miipadctl &= ~(RGMII_3_AMP_EN | GMII_2_AMP_EN | GMII_1_AMP_EN);
    GPIO->MiiPadCtrl = miipadctl;

    // Bring internal ethernet Phy out of reset
    PERF->softResetB &= ~SOFT_RST_EPHY;
    udelay(1000);
    // Power-up GPHY
    MISC->miscIddqCtrl &= ~MISC_IDDQ_CTRL_GPHY;
    udelay(1000);
    PERF->softResetB |= SOFT_RST_EPHY;
    udelay(1000);
#endif


#if defined(_BCM96828_) || defined(CONFIG_BCM96828)
    // Take GPHY out of low power and disable IDDQ
    GPIO->RoboswGphyCtrl &= ~( GPHY_IDDQ_BIAS | GPHY2_LOW_PWR_SD | GPHY2_LOW_PWR | GPHY1_LOW_PWR_SD | GPHY1_LOW_PWR);
    udelay(1000);

    // Bring internal GigPhy out of reset
    PERF->softResetB &= ~(SOFT_RST_GPHY_CAL | SOFT_RST_GPHY1 | SOFT_RST_GPHY0);
    udelay(1000);
    PERF->softResetB |= (SOFT_RST_GPHY_CAL | SOFT_RST_GPHY1 | SOFT_RST_GPHY0);
    udelay(1000);
#endif

#if defined(_BCM96328_) || defined(CONFIG_BCM96328) || \
    defined(_BCM96362_) || defined(CONFIG_BCM96362) || \
    defined(_BCM963268_) || defined(CONFIG_BCM963268) || \
    defined(_BCM96828_) || defined(CONFIG_BCM96828) || \
    defined(_BCM96318_) || defined(CONFIG_BCM96318)
    GPIO->RoboswSwitchCtrl |= (RSW_MII_DUMB_FWDG_EN | RSW_HW_FWDG_EN);
#endif
#if defined(_BCM96368_) || defined(CONFIG_BCM96368) || defined(_BCM96818_) || defined(CONFIG_BCM96818)
    GPIO->RoboswEphyCtrl |= (RSW_MII_DUMB_FWDG_EN | RSW_HW_FWDG_EN);
#endif

#if defined(_BCM96318_) || defined(CONFIG_BCM96318)
    PERF->blkEnables |= ROBOSW025_CLK_EN;
#elif defined(_BCM963381_) || defined(CONFIG_BCM963381)
    /* some 63381 ref board can't use RGMII strap, use software override if bp says RGMII but strap is MII */
    if (IsRGMII(EnetInfo[0].sw.phy_id[4]) && (MISC->miscStrapBus&MISC_STRAP_BUS_ROBO_SW_RGMII_N_MII) )
    {
        MISC_REG->RoboswCtrl |= RSW_RGMII_PADS_ENABLE;
        MISC->miscStrapBus &= ~MISC_STRAP_BUS_ROBO_SW_RGMII_N_MII;
        MISC->miscStrapOverride |= 0x1;
    }
    MISC_REG->RoboswCtrl |= (RSW_MII_DUMB_FWDG_EN | RSW_HW_FWDG_EN);
    pmc_switch_power_up();

    if( MISC->miscStrapOverride&0x1 )
        MISC->miscStrapOverride &= ~0x1;

    /* EPHY power up */
    MISC_REG->EphyResetCtrl |= EPHY_RESET_N;
    udelay(100);
    MISC_REG->EphyTestCtrl = EPHY_TEST_DLLBYP_PIN_F;
    udelay(100);
    MISC_REG->EphyResetCtrl &= ~EPHY_RESET_N;
    udelay(100);
    MISC_REG->EphyTestCtrl = EPHY_TEST_DLLBYP_PIN_F;
    udelay(100);
    MISC_REG->EphyTestCtrl = EPHY_TEST_DLLBYP_PIN_F;
    udelay(100);
    MISC_REG->EphyResetCtrl |= (EPHY_RESET_N|EPHY_RESET_N_PHY_3|EPHY_RESET_N_PHY_2|EPHY_RESET_N_PHY_1|EPHY_RESET_N_PHY_0);
    udelay(100);
    MISC_REG->EphyResetCtrl = 0x0;
    udelay(1000);
    MISC_REG->EphyPwrMgnt = (EPHY_PWR_DOWN_RD_0|EPHY_PWR_DOWN_0|EPHY_PWR_DOWN_RD_1|EPHY_PWR_DOWN_1|
                             EPHY_PWR_DOWN_RD_2|EPHY_PWR_DOWN_2|EPHY_PWR_DOWN_RD_3|EPHY_PWR_DOWN_3);
    udelay(500);
    MISC_REG->EphyPwrMgnt = 0x0;
    udelay(500);
    MISC_REG->EphyResetCtrl |= EPHY_RESET_N;
    udelay(100);
    MISC_REG->EphyResetCtrl = 0x0;
    udelay(1000);
#else
    // Enable Switch clock
    PERF->blkEnables |= ROBOSW_CLK_EN;
#endif

#if defined(_BCM96368_) || defined(CONFIG_BCM96368)
    PERF->blkEnables |= SWPKT_SAR_CLK_EN | SWPKT_USB_CLK_EN;
#endif
#if defined(_BCM96818_) || defined(CONFIG_BCM96818)
    PERF->blkEnables |= SWPKT_GPON_CLK_EN; /* 6818 do not need USB_SWPKT by default */
    PERF->blkEnables &= (~SWPKT_USB_CLK_EN);
#endif

    udelay(1000);
#if !defined(_BCM963381_) && !defined(CONFIG_BCM963381)
    PERF->softResetB &= ~SOFT_RST_SWITCH;
    udelay(1000);
    PERF->softResetB |= SOFT_RST_SWITCH;
    udelay(1000);
#endif

#if defined(_BCM96818_) || defined(CONFIG_BCM96818)
    /* After reset, we can disable unused switch clocks
       Only disabling the Switch GPON CLK really saves power
       so no need to bother for the other clocks on other chips */
    {        
        
        /* Check if this is BHR */
        unsigned char portInfo6829;
        if ( (BP_SUCCESS == BpGet6829PortInfo(&portInfo6829)) &&
             (0 != portInfo6829))
        {
            /* BHR does not need GPON and USB Switch ports */
            PERF->blkEnables &= ~(SWPKT_GPON_CLK_EN | SWPKT_USB_CLK_EN);
        }

        // determine we need to setup MII over GPIO for 6818
        {
        unsigned long
            Flag = 0;

            BpGetMiiOverGpioFlag(&Flag);

            if (TRUE == Flag)
            {
                // set basemode for MII to robosw
                GPIO->GPIOBaseMode |= EN_MII_OVER_GPIO;

                //set pins for output direction 
                GPIO->GPIODir |= GPIO_MII_OVER_GPIO_OUTPUTS;

                //clear pins for input direction 
                GPIO->GPIODir &= ~(GPIO_MII_OVER_GPIO_INPUTS); 
            }
        }

    }
       
#endif

    /* Disable Rx and Tx on all Ethernet Ports */
    for (port = 0; port < EPHY_PORTS; port++) {
        SWITCH->PortCtrl[port] = 0x03;
    }

#ifdef _EXT_SWITCH_INIT_
    if (EnetInfo[1].ucPhyType == BP_ENET_EXTERNAL_SWITCH) {
        ext_switch_init();
    }
#endif

}

static void phy_advertise_caps(unsigned int phy_id)
{
    /* control advertising if boardparms says so */
    if(IsPhyConnected(phy_id) && IsPhyAdvCapConfigValid(phy_id))
    {
        uint16 cap_mask = 0;
        cap_mask = mii_read(phy_id, MII_ANAR);
        cap_mask &= ~(ANAR_TXFD | ANAR_TXHD | ANAR_10FD | ANAR_10HD);
        if (phy_id & ADVERTISE_10HD)
            cap_mask |= ANAR_10HD;
        if (phy_id & ADVERTISE_10FD)
            cap_mask |= ANAR_10FD;
        if (phy_id & ADVERTISE_100HD)
            cap_mask |= ANAR_TXHD;
        if (phy_id & ADVERTISE_100FD)
            cap_mask |= ANAR_TXFD;
        mii_write(phy_id, MII_ANAR, cap_mask);

        cap_mask = mii_read(phy_id, MII_K1CTL);
        cap_mask &= (~(K1CTL_1000BT_FDX | K1CTL_1000BT_HDX));
        if (phy_id & ADVERTISE_1000HD)
            cap_mask |= K1CTL_1000BT_HDX;
        if (phy_id & ADVERTISE_1000FD)
            cap_mask |= K1CTL_1000BT_FDX;
        mii_write(phy_id, MII_K1CTL, cap_mask);
    }
}

/* apply phy init board parameters for internal switch*/
void phy_apply_init_bp(int port)
{
    bp_mdio_init_t* phyinit;
    uint16 data;

    phyinit = EnetInfo[0].sw.phyinit[port];
    if( phyinit == 0 )
    	return;

    while(phyinit->u.op.op != BP_MDIO_INIT_OP_NULL)
    {
        if(phyinit->u.op.op == BP_MDIO_INIT_OP_WRITE)
            mii_write(EnetInfo[0].sw.phy_id[port], phyinit->u.write.reg, phyinit->u.write.data);
        else if(phyinit->u.op.op == BP_MDIO_INIT_OP_UPDATE)
        {
            data = mii_read(EnetInfo[0].sw.phy_id[port], phyinit->u.update.reg);
            data &= ~phyinit->u.update.mask;
            data |= phyinit->u.update.data;
            mii_write(EnetInfo[0].sw.phy_id[port], phyinit->u.update.reg, data);
        }
        phyinit++;
    }

    return;
}

void robosw_configure_ports()
{
    uint16 data;
    int i;
#if defined(_BCM963268_) || defined(CONFIG_BCM963268)
    int chipRev = (PERF->RevID & REV_ID_MASK);
#endif
#if defined(_BCM96818_) || defined(CONFIG_BCM96818)
    uint32 miipadctl;
#endif

    for (i = 0; i < EPHY_PORTS; i++) {
        if ((EnetInfo[0].sw.port_map & (1 << i)) != 0) {
            if (IsWanPort(EnetInfo[0].sw.phy_id[i])) {
                *(SWITCH_PBVLAN + i) = PBMAP_MIPS;
                SWITCH->DisableLearn |= (1 << i);
            }
#if defined(_BCM96818_) || defined(CONFIG_BCM96818)
            if (i == 2) {
                GPIO->GPIOBaseMode |= (EN_GMII1);
                // imp_rgmii_ctrl_p2 is a better name for ImpRgmiiCtrlP4, 
                // which is confusing
                ETHSWREG->imp_rgmii_ctrl_p2 |= ImpRgmiiCtrl_GMII_En; 
                ETHSWREG->imp_rgmii_ctrl_p2 |= ImpRgmiiCtrl_Timing_Sel;
            }
            if (i == 3) {
                GPIO->GPIOBaseMode |= (EN_GMII2);
                // imp_rgmii_ctrl_p3 is a better name for ImpRgmiiCtrlP5, 
                // which is confusing
                ETHSWREG->imp_rgmii_ctrl_p3 |= ImpRgmiiCtrl_GMII_En; 
                ETHSWREG->imp_rgmii_ctrl_p3 |= ImpRgmiiCtrl_Timing_Sel;
            }
            if (i == 5) {
                GPIO->GPIOBaseMode |= (EN_GMII3);
                // imp_rgmii_ctrl_p5 is a better name for ImpRgmiiCtrlP7, 
                // which is confusing
                ETHSWREG->imp_rgmii_ctrl_p5 |= ImpRgmiiCtrl_GMII_En; 
                ETHSWREG->imp_rgmii_ctrl_p5 |= ImpRgmiiCtrl_Timing_Sel;
            }
#endif
            if (!IsPhyConnected(EnetInfo[0].sw.phy_id[i])) {          
#if defined(_BCM96368_) || defined(CONFIG_BCM96368)
                if (i == 4)
                    GPIO->GPIOBaseMode |= (EN_GMII1);
                if (i == 5)
                    GPIO->GPIOBaseMode |= (EN_GMII2);
#endif
                continue;
            }

            // Reset
            mii_write(EnetInfo[0].sw.phy_id[i], MII_BMCR, BMCR_RESET);
            PortLinkState[i] = 0;

#if defined(_BCM963381_) || defined(CONFIG_BCM963381)
            ephy_adjust_afe(EnetInfo[0].sw.phy_id[i]);
#endif

            /*software workaround for the EPHY ADC issue JIRA 13141. Must be called right below phy reset*/
#if defined(_BCM96318_) || defined(CONFIG_BCM96318)
            if (!IsExtPhyId(EnetInfo[0].sw.phy_id[i])) {
                data = mii_read(EnetInfo[0].sw.phy_id[i], MII_BRCM_TEST);
                mii_write(EnetInfo[0].sw.phy_id[i], MII_BRCM_TEST, (data | MII_BRCM_TEST_SHADOW2_ENABLE));
                /* reest ADC */
                mii_write(EnetInfo[0].sw.phy_id[i], 0x14, 0x0f00);  //set iddq_clkbias
                udelay(100);
                mii_write(EnetInfo[0].sw.phy_id[i], 0x14, 0x0C00);  //reset iddq_clkbias
                udelay(100);
                /* Invert ADC clock */
                mii_write(EnetInfo[0].sw.phy_id[i], 0x13, 0x7555);  //set clock inversion 
                mii_write(EnetInfo[0].sw.phy_id[i], MII_BRCM_TEST, (data & ~MII_BRCM_TEST_SHADOW2_ENABLE));
            }
#endif

            // Enable auto-negotiation
            mii_write(EnetInfo[0].sw.phy_id[i], MII_ANAR, ANAR_TXFD | ANAR_TXHD | ANAR_10FD | ANAR_10HD | PSB_802_3);

            if (!IsExtPhyId(EnetInfo[0].sw.phy_id[i])) {
                // Configure PHY link/act LED
#if defined(_BCM96368_) || defined(CONFIG_BCM96368)
                // Enable status change notification */
                mii_write(EnetInfo[0].sw.phy_id[i], MII_INTERRUPT, MII_INTR_ENABLE);
                // Configure LEDs
                data = mii_read(EnetInfo[0].sw.phy_id[i], MII_RESERVED_1B);
                mii_write(EnetInfo[0].sw.phy_id[i], MII_RESERVED_1B, data | MII_RESERVED_1B_ACT_LED);
#elif defined(_BCM96818_) || defined(CONFIG_BCM96818)
                // Configure LEDs
                mii_write(EnetInfo[0].sw.phy_id[i], 0x1c, 0xa410);
#elif   defined(_BCM96328_) || defined(CONFIG_BCM96328) || \
        defined(_BCM96362_) || defined(CONFIG_BCM96362) || \
        defined(_BCM963268_) || defined(CONFIG_BCM963268) || \
        defined(_BCM96828_) || defined(CONFIG_BCM96828) || \
        defined(_BCM96318_) || defined(CONFIG_BCM96318) || \
        defined(_BCM963381_) || defined(CONFIG_BCM963381)
                // Configure LEDs
                // Enable Shadow register 2
                data = mii_read(EnetInfo[0].sw.phy_id[i], MII_BRCM_TEST);
                mii_write(EnetInfo[0].sw.phy_id[i], MII_BRCM_TEST, (data | MII_BRCM_TEST_SHADOW2_ENABLE));
#if defined(_BCM963268_) || defined(CONFIG_BCM963268)
                if (i != GPHY_PORT_ID ) {
                    // Set LED1 to speed. Set LED0 to blinky link
                    mii_write(EnetInfo[0].sw.phy_id[i], 0x15, 0x08);
                }
#elif defined(_BCM96828_) || defined(CONFIG_BCM96828)
                if ( (i != GPHY1_PORT_ID) && (i != GPHY2_PORT_ID) ){
                    // Set LED1 to speed. Set LED0 to blinky link
                    mii_write(EnetInfo[0].sw.phy_id[i], 0x15, 0x08);
                }
#else
                // Set LED0 to speed. Set LED1 to blinky link
                mii_write(EnetInfo[0].sw.phy_id[i], 0x15, 0x71);
#endif
                // Disable Shadow register 2
                mii_write(EnetInfo[0].sw.phy_id[i], MII_BRCM_TEST, (data & ~MII_BRCM_TEST_SHADOW2_ENABLE));
#endif

#if defined(_BCM963268_) || defined(CONFIG_BCM963268)
                if (i == GPHY_PORT_ID ) {
                    data = mii_read(EnetInfo[0].sw.phy_id[i], MII_K1CTL);
                    /* Enable Gbit Half-Duplex and Gbit Full-Duplex */
                    /* Enable repeater mode to be master of clock when partner is single-port device */
                    data |= (K1CTL_REPEATER_DTE | K1CTL_1000BT_HDX | K1CTL_1000BT_FDX);
                    mii_write(EnetInfo[0].sw.phy_id[i], MII_K1CTL, data);
                }
#elif defined(_BCM96828_) || defined(CONFIG_BCM96828)
                if ( (i == GPHY1_PORT_ID) || (i == GPHY2_PORT_ID) ){
                    data = mii_read(EnetInfo[0].sw.phy_id[i], MII_K1CTL);
                    /* Enable Gbit Half-Duplex and Gbit Full-Duplex */
                    /* Enable repeater mode to be master of clock when partner is single-port device */
                    data |= (K1CTL_REPEATER_DTE | K1CTL_1000BT_HDX | K1CTL_1000BT_FDX);
                    mii_write(EnetInfo[0].sw.phy_id[i], MII_K1CTL, data);
                }
#endif
            } else {
#if defined(_BCM96368_) || defined(CONFIG_BCM96368)
                if (i == 4)
                    GPIO->GPIOBaseMode |= (EN_GMII1);
                if (i == 5)
                    GPIO->GPIOBaseMode |= (EN_GMII2);
#endif
#if defined(_BCM96362_) || defined(CONFIG_BCM96362) || defined(_BCM963268_) || defined(CONFIG_BCM963268)
                if (i == 4)
                    GPIO->RoboswSwitchCtrl |= (RSW_MII_SEL_2P5V << RSW_MII_SEL_SHIFT);
                if (i == 5)
                    GPIO->RoboswSwitchCtrl |= (RSW_MII_2_IFC_EN | (RSW_MII_SEL_2P5V << RSW_MII_2_SEL_SHIFT));
#endif
#if defined(_BCM963268_) || defined(CONFIG_BCM963268)
                if (i == 6)
                    GPIO->RoboswSwitchCtrl |= (RSW_MII_SEL_2P5V << RSW_MII_3_SEL_SHIFT);
                if (i == 7)
                    GPIO->RoboswSwitchCtrl |= (RSW_MII_SEL_2P5V << RSW_MII_4_SEL_SHIFT);
#endif
#if defined(_BCM96828_)  || defined(CONFIG_BCM96828)
                if (i == 4)
                    GPIO->RoboswSwitchCtrl |= (RSW_MII_SEL_2P5V << RSW_MII_1_SEL_SHIFT);
                if (i == 6)
                    GPIO->RoboswSwitchCtrl |= (RSW_MII_SEL_2P5V << RSW_MII_2_SEL_SHIFT);
#endif

#if defined(_BCM96328_) || defined(CONFIG_BCM96328)
                if (i == 4)
                    MISC->miscPadCtrlHigh |= (MISC_MII_SEL_2P5V << MISC_MII_SEL_SHIFT);
#endif
#if defined(_BCM96818_) || defined(CONFIG_BCM96818)
                // workaround hw issue. when read, the mii_over_gpio upper 4 bits are shift by 1 bit to the right.
                miipadctl = GPIO->MiiPadCtrl;
                miipadctl = ((miipadctl&MII_OVER_GPIO_RD_MASK)<<1) | (miipadctl&~MII_OVER_GPIO_RD_MASK);
                if (i == 2) {
                    //default is 3.3V GMII mode
                    if (IsRGMII(EnetInfo[0].sw.phy_id[i]))
                    {
                        miipadctl &= ~GMII_1_SEL_MASK;
                        if( IsRGMII_2P5V(EnetInfo[0].sw.phy_id[i]) )
                            miipadctl |= GMII_1_SEL_2P5V;
                        else
                            miipadctl |= GMII_1_SEL_3P3V;
                    }
                }

                if (i == 3) {
                    //default is 3.3V GMII mode
                    if (IsRGMII(EnetInfo[0].sw.phy_id[i]))
                    {
                        miipadctl &= ~GMII_2_SEL_MASK;
                        if( IsRGMII_2P5V(EnetInfo[0].sw.phy_id[i]) )
                            miipadctl |= GMII_2_SEL_2P5V;
                        else
                            miipadctl |= GMII_2_SEL_3P3V;
                    }
                }

                if (i == 5) {
                    //default is 2.5V RGMII mode. This port only support RGMII
                    miipadctl &= ~RGMII_3_SEL_MASK;
                    if( IsRGMII_2P5V(EnetInfo[0].sw.phy_id[i]) )
                        miipadctl |= RGMII_3_SEL_2P5V;
                    else
                        miipadctl |= RGMII_3_SEL_3P3V;
                }
                GPIO->MiiPadCtrl = miipadctl;
#endif

#if defined(_BCM96318_) || defined(CONFIG_BCM96318)
                if (i == 4)
                {
                    //default is 3.3V GMII mode
                    if (IsRGMII(EnetInfo[0].sw.phy_id[i]))
                    {
                        MISC->miscSpecialPadControl &= ~(RGMII_PAD_SEL_MASK << RGMII_PAD_SEL_SHIFT);
                        if( IsRGMII_2P5V(EnetInfo[0].sw.phy_id[i]) )
                            MISC->miscSpecialPadControl |= (RGMII_PAD_SEL_2_5V  << RGMII_PAD_SEL_SHIFT);
                        else
                            MISC->miscSpecialPadControl |= (RGMII_PAD_SEL_3_3V  << RGMII_PAD_SEL_SHIFT);
                    }
                }
#endif

                // Reset
                mii_write(EnetInfo[0].sw.phy_id[i], MII_BMCR, BMCR_RESET);

                // Configure LED for link/activity
                data = MII_1C_SHADOW_LED_CONTROL << MII_1C_SHADOW_REG_SEL_S;
                mii_write(EnetInfo[0].sw.phy_id[i], MII_REGISTER_1C, data);
                data = mii_read(EnetInfo[0].sw.phy_id[i], MII_REGISTER_1C);
                data |= ACT_LINK_LED_ENABLE;
                data |= MII_1C_WRITE_ENABLE;
                mii_write(EnetInfo[0].sw.phy_id[i], MII_REGISTER_1C, data);

                data = mii_read(EnetInfo[0].sw.phy_id[i], MII_PHYIDR2);
                if (IsRGMII(EnetInfo[0].sw.phy_id[i]) ||
                    ((data & BCM_PHYID_M) == (BCM54610_PHYID2 & BCM_PHYID_M)) ||
                    ((data & BCM_PHYID_M) == (BCM50612_PHYID2 & BCM_PHYID_M))) {
                    // Configure RGMII timing for 54610 GPHY
                    data = MII_1C_SHADOW_CLK_ALIGN_CTRL << MII_1C_SHADOW_REG_SEL_S;
                    mii_write(EnetInfo[0].sw.phy_id[i], MII_REGISTER_1C, data);
                    data = mii_read(EnetInfo[0].sw.phy_id[i], MII_REGISTER_1C);
#if defined(_BCM963268_) || defined(CONFIG_BCM963268)
                    /* Temporary work-around for MII2 port RGMII delay programming */
                    if ((i == 5) && ((chipRev == 0xA0) || (chipRev == 0xB0)))
                        data |= GTXCLK_DELAY_BYPASS_DISABLE;
                    else
#endif
                        data &= (~GTXCLK_DELAY_BYPASS_DISABLE);
                    data |= MII_1C_WRITE_ENABLE;
                    mii_write(EnetInfo[0].sw.phy_id[i], MII_REGISTER_1C, data);

                    // Configure LOM LED Mode
                    data = MII_1C_EXTERNAL_CONTROL_1 << MII_1C_SHADOW_REG_SEL_S;
                    mii_write(EnetInfo[0].sw.phy_id[i], MII_REGISTER_1C, data);
                    data = mii_read(EnetInfo[0].sw.phy_id[i], MII_REGISTER_1C);
                    data |= LOM_LED_MODE;
                    data |= MII_1C_WRITE_ENABLE;
                    mii_write(EnetInfo[0].sw.phy_id[i], MII_REGISTER_1C, data);

                    /* Enable Gbit Half-Duplex and Gbit Full-Duplex */
                    /* Enable repeater mode to be master of clock when partner is single-port device */
                    data = mii_read(EnetInfo[0].sw.phy_id[i], MII_K1CTL);
                    data |= (K1CTL_REPEATER_DTE | K1CTL_1000BT_HDX | K1CTL_1000BT_FDX);
                    mii_write(EnetInfo[0].sw.phy_id[i], MII_K1CTL, data);

                }
            }

            // Enable auto-negotiation
            data = mii_read(EnetInfo[0].sw.phy_id[i], MII_ANAR);
            data |= (ANAR_TXFD | ANAR_TXHD | ANAR_10FD | ANAR_10HD);
            data &= (~ANAR_PSB);
            data |= PSB_802_3;
            mii_write(EnetInfo[0].sw.phy_id[i], MII_ANAR, data);

            phy_advertise_caps(EnetInfo[0].sw.phy_id[i]);

            // Restart auto-negotiation
            data = mii_read(EnetInfo[0].sw.phy_id[i], MII_BMCR);
            mii_write(EnetInfo[0].sw.phy_id[i], MII_BMCR, data | BMCR_RESTARTAN);

            /* additional phy init from board parameter, may require reset/re-negotiation depend on
             * parameters. But the phy init parameter should include reset/negotiation instructions
             *  in bp_pPhyInit list.
             */
            phy_apply_init_bp(i);
        }
#if defined(_BCM96818_) || defined(CONFIG_BCM96818)
        else {
            if (!IsExtPhyId(EnetInfo[0].sw.phy_id[i])) {
                if (i == 0)
                    GPIO->RoboswEphyCtrl |= (GPHY_PWR_DOWN_0 | GPHY_PWR_DOWN_SD_0);
                if (i == 1)
                    GPIO->RoboswEphyCtrl |= (GPHY_PWR_DOWN_1 | GPHY_PWR_DOWN_SD_1);
            }
        }
#endif
    }

#if defined(_BCM96818_) || defined(CONFIG_BCM96818)
    // Disable SERDES, MoCA, USB and GPON port
    SWITCH->PortOverride[SERDES_PORT_ID] = PortOverride_Enable;
    SWITCH->PortOverride[USB_PORT_ID] = PortOverride_Enable;
    SWITCH->PortOverride[GPON_PORT_ID] = PortOverride_Enable;
#endif

#if defined(_BCM96328_) || defined(CONFIG_BCM96328) || defined(_BCM96362_) || defined(CONFIG_BCM96362)
    // Ports 6 and 7 are not used on 6328 and 6362
    SWITCH->PortOverride[PORT_6_PORT_ID] = PortOverride_Enable;
    SWITCH->PortOverride[PORT_7_PORT_ID] = PortOverride_Enable;
#endif

#if defined(_BCM96368_) || defined(CONFIG_BCM96368)
    // Disable SAR and USB port
    SWITCH->PortOverride[USB_PORT_ID] = PortOverride_Enable;
    SWITCH->PortOverride[SAR_PORT_ID] = PortOverride_Enable;
#endif

#if defined(_BCM96318_) || defined(CONFIG_BCM96318) || defined(_BCM963381_) || defined(CONFIG_BCM963381)
    /* only turn the RMMII clock when the port is used to reduce EMI*/
    if ((EnetInfo[0].sw.port_map & (1 << 4)) != 0)
    {
        // Enable the GMII clocks.
        SWITCH->ImpRgmiiCtrlP4 |= ImpRgmiiCtrl_GMII_En | ImpRgmiiCtrl_Force_RGMII;
        /* RGMII Delay Programming. Enable ID mode */
        SWITCH->ImpRgmiiCtrlP4 |= ImpRgmiiCtrl_Timing_Sel;
        if (IsPortRxInternalDelay(EnetInfo[0].sw.port_flags[4])) {
            SWITCH->ImpRgmiiCtrlP4 |= ImpRgmiiCtrl_DLL_RXC_Bypass;
        }

        SWITCH->ImpRgmiiCtrlP4 &= (~ImpRgmiiCtrl_Mode_Mask);
        /* Force MII/RMII/RGMII based on board params */
        if (IsRGMII(EnetInfo[0].sw.phy_id[4])) {
            SWITCH->ImpRgmiiCtrlP4 |= ImpRgmiiCtrl_Mode_RGMII;
        } else if (IsRvMII(EnetInfo[0].sw.phy_id[4])) {
            SWITCH->ImpRgmiiCtrlP4 |= ImpRgmiiCtrl_Mode_RvMII;
        } else if (IsGMII(EnetInfo[0].sw.phy_id[4]))  {
            SWITCH->ImpRgmiiCtrlP4 |= ImpRgmiiCtrl_Mode_GMII;
        } else {
            SWITCH->ImpRgmiiCtrlP4 |= ImpRgmiiCtrl_Mode_MII;
            /* based on Herman,  MII is simulated through RGMII and TX clock must be off */
            SWITCH->ImpRgmiiCtrlP4 &= ~ImpRgmiiCtrl_GMII_En;
        }

#if defined(_BCM963381_) || defined(CONFIG_BCM963381)
       if (IsRGMII(EnetInfo[0].sw.phy_id[4]))
       {
           MISC_REG->RoboswCtrl |= RSW_RGMII_PADS_ENABLE;
           if ( IsRGMII_1P8V(EnetInfo[0].sw.phy_id[4]) ) {
               MISC_REG->RoboswCtrl &= ~RSW_RGMII_PAD_MODEHV_CTRL; //clear this bit for 1.8v and 1.5v rgmii
               /* adjust RGMII pad control */
               PAD_CTL_REG->PadCtrl[22] = 0x04104104;
               PAD_CTL_REG->PadCtrl[23] = 0x08208204;
               PAD_CTL_REG->PadCtrl[24] = 0x0003F208;
               PAD_CTL_REG->PadCtrl[25] = 0x09249249;
	   } 
           else
               MISC_REG->RoboswCtrl |= RSW_RGMII_PAD_MODEHV_CTRL; //set this bit for 3.3v and 2.5v rgmii
       }
#endif
    }
#else
    // Enable the GMII clocks. 
    SWITCH->ImpRgmiiCtrlP4 |= ImpRgmiiCtrl_GMII_En;
    /* RGMII Delay Programming. Enable ID mode */
    SWITCH->ImpRgmiiCtrlP4 |= ImpRgmiiCtrl_Timing_Sel;
    if (IsPortRxInternalDelay(EnetInfo[0].sw.port_flags[4])) {
        SWITCH->ImpRgmiiCtrlP4 |= ImpRgmiiCtrl_DLL_RXC_Bypass;
    }
#if defined(_BCM963268_) || defined(CONFIG_BCM963268) || defined(_BCM96828_) || defined(CONFIG_BCM96828)
    SWITCH->ImpRgmiiCtrlP4 |= ImpRgmiiCtrl_Force_RGMII;
#endif

#if ( !defined(_BCM96328_) && !defined(CONFIG_BCM96328) && !defined(_BCM96828_) && !defined(CONFIG_BCM96828))
    SWITCH->ImpRgmiiCtrlP5 |= ImpRgmiiCtrl_GMII_En;
    SWITCH->ImpRgmiiCtrlP5 |= ImpRgmiiCtrl_Timing_Sel;
    if (IsPortRxInternalDelay(EnetInfo[0].sw.port_flags[5])) {
        SWITCH->ImpRgmiiCtrlP5 |= ImpRgmiiCtrl_DLL_RXC_Bypass;
    }
#if defined(_BCM963268_) || defined(CONFIG_BCM963268)
    SWITCH->ImpRgmiiCtrlP5 |= ImpRgmiiCtrl_Force_RGMII;
#endif
#endif
#endif

#if defined(_BCM963268_) || defined(CONFIG_BCM963268)
    SWITCH->ImpRgmiiCtrlP6 |= (ImpRgmiiCtrl_GMII_En | ImpRgmiiCtrl_Force_RGMII);
    SWITCH->ImpRgmiiCtrlP6 |= ImpRgmiiCtrl_Timing_Sel;
    if (IsPortRxInternalDelay(EnetInfo[0].sw.port_flags[6])) {
        SWITCH->ImpRgmiiCtrlP6 |= ImpRgmiiCtrl_DLL_RXC_Bypass;
    }
    SWITCH->ImpRgmiiCtrlP7 |= (ImpRgmiiCtrl_GMII_En | ImpRgmiiCtrl_Force_RGMII);
    SWITCH->ImpRgmiiCtrlP7 |= ImpRgmiiCtrl_Timing_Sel;
    if (IsPortRxInternalDelay(EnetInfo[0].sw.port_flags[7])) {
        SWITCH->ImpRgmiiCtrlP7 |= ImpRgmiiCtrl_DLL_RXC_Bypass;
    }

    SWITCH->ImpRgmiiCtrlP4 &= (~ImpRgmiiCtrl_Mode_Mask);
    SWITCH->ImpRgmiiCtrlP5 &= (~ImpRgmiiCtrl_Mode_Mask);
    SWITCH->ImpRgmiiCtrlP6 &= (~ImpRgmiiCtrl_Mode_Mask);
    SWITCH->ImpRgmiiCtrlP7 &= (~ImpRgmiiCtrl_Mode_Mask);

    /* Force MII/RMII/RGMII based on board params */
    if (IsRGMII(EnetInfo[0].sw.phy_id[4])) {
        SWITCH->ImpRgmiiCtrlP4 |= ImpRgmiiCtrl_Mode_RGMII;
    } else if (IsRvMII(EnetInfo[0].sw.phy_id[4])) {
        SWITCH->ImpRgmiiCtrlP4 |= ImpRgmiiCtrl_Mode_RvMII;
    } else {
        SWITCH->ImpRgmiiCtrlP4 |= ImpRgmiiCtrl_Mode_MII;
    }

    if (IsRGMII(EnetInfo[0].sw.phy_id[5])) {
        SWITCH->ImpRgmiiCtrlP5 |= ImpRgmiiCtrl_Mode_RGMII;
    } else if (IsRvMII(EnetInfo[0].sw.phy_id[5])) {
        SWITCH->ImpRgmiiCtrlP5 |= ImpRgmiiCtrl_Mode_RvMII;
    } else {
        SWITCH->ImpRgmiiCtrlP5 |= ImpRgmiiCtrl_Mode_MII;
    }

    if (IsRGMII(EnetInfo[0].sw.phy_id[6])) {
        SWITCH->ImpRgmiiCtrlP6 |= ImpRgmiiCtrl_Mode_RGMII;
    } else if (IsRvMII(EnetInfo[0].sw.phy_id[6])) {
        SWITCH->ImpRgmiiCtrlP6 |= ImpRgmiiCtrl_Mode_RvMII;
    } else {
        SWITCH->ImpRgmiiCtrlP6 |= ImpRgmiiCtrl_Mode_MII;
    }

    if (IsRGMII(EnetInfo[0].sw.phy_id[7])) {
        SWITCH->ImpRgmiiCtrlP7 |= ImpRgmiiCtrl_Mode_RGMII;
    } else if (IsRvMII(EnetInfo[0].sw.phy_id[7])) {
        SWITCH->ImpRgmiiCtrlP7 |= ImpRgmiiCtrl_Mode_RvMII;
    } else {
        SWITCH->ImpRgmiiCtrlP7 |= ImpRgmiiCtrl_Mode_MII;
    }

    if (EnetInfo[0].sw.port_map & (1 << PORT_5_PORT_ID)) {
        GPIO->RoboswSwitchCtrl |= RSW_MII_2_IFC_EN;
        /* Temporary partial work-around for port-5 using port-6's speed and duplex. */
        SWITCH->PortOverride[PORT_5_PORT_ID] = PortOverride_Enable | PortOverride_1000Mbs | PortOverride_Fdx;
    }

    if ((chipRev == 0xA0) || (chipRev == 0xB0)) {
        /* Temporary work-around for RGMII delay programming */
        SWITCH->ImpRgmiiCtrlP4 &= (~ImpRgmiiCtrl_Timing_Sel);
        SWITCH->ImpRgmiiCtrlP5 &= (~ImpRgmiiCtrl_Timing_Sel);
        SWITCH->ImpRgmiiCtrlP6 &= (~ImpRgmiiCtrl_Timing_Sel);
        SWITCH->ImpRgmiiCtrlP7 &= (~ImpRgmiiCtrl_Timing_Sel);
    }

#endif

#if  defined(_BCM96828_) || defined(CONFIG_BCM96828)
    SWITCH->ImpRgmiiCtrlP6 |= (ImpRgmiiCtrl_GMII_En | ImpRgmiiCtrl_Force_RGMII);
    SWITCH->ImpRgmiiCtrlP6 |= ImpRgmiiCtrl_Timing_Sel;
    if (IsPortRxInternalDelay(EnetInfo[0].sw.port_flags[6])) {
        SWITCH->ImpRgmiiCtrlP6 |= ImpRgmiiCtrl_DLL_RXC_Bypass;
    }

    /* Force MII/RMII/RGMII based on board params */
    if (IsRGMII(EnetInfo[0].sw.phy_id[4])) {
        SWITCH->ImpRgmiiCtrlP4 |= ImpRgmiiCtrl_Mode_RGMII;
    } else if (IsRvMII(EnetInfo[0].sw.phy_id[4])) {
        SWITCH->ImpRgmiiCtrlP4 |= ImpRgmiiCtrl_Mode_RvMII;
    } else {
        SWITCH->ImpRgmiiCtrlP4 |= ImpRgmiiCtrl_Mode_MII;
    }

    if (IsRGMII(EnetInfo[0].sw.phy_id[6])) {
        SWITCH->ImpRgmiiCtrlP6 |= ImpRgmiiCtrl_Mode_RGMII;
    } else if (IsRvMII(EnetInfo[0].sw.phy_id[6])) {
        SWITCH->ImpRgmiiCtrlP6 |= ImpRgmiiCtrl_Mode_RvMII;
    } else {
        SWITCH->ImpRgmiiCtrlP6 |= ImpRgmiiCtrl_Mode_MII;
    }
#endif

#if  defined(_BCM96818_) || defined(CONFIG_BCM96818)
    /* for pin compatible 6818 device, there is no bootstrap pin to config the sw port mode for p2 and p3.
     * so use the RGMII Ctrl reg to force the port mode.  p5 is always RGMII.
     */
    if(UtilGetChipIsPinCompatible())
    {
        SWITCH->ImpRgmiiCtrlP4 |= (ImpRgmiiCtrl_GMII_En | ImpRgmiiCtrl_Force_RGMII);
        SWITCH->ImpRgmiiCtrlP4 |= ImpRgmiiCtrl_Timing_Sel;
        SWITCH->ImpRgmiiCtrlP5 |= (ImpRgmiiCtrl_GMII_En | ImpRgmiiCtrl_Force_RGMII);
        SWITCH->ImpRgmiiCtrlP5 |= ImpRgmiiCtrl_Timing_Sel;
        SWITCH->ImpRgmiiCtrlP4 &= (~ImpRgmiiCtrl_Mode_Mask);
        SWITCH->ImpRgmiiCtrlP5 &= (~ImpRgmiiCtrl_Mode_Mask);

        if (IsRGMII(EnetInfo[0].sw.phy_id[2])) {
            SWITCH->ImpRgmiiCtrlP4 |= ImpRgmiiCtrl_Mode_RGMII;
        } else if (IsRvMII(EnetInfo[0].sw.phy_id[2])) {
            SWITCH->ImpRgmiiCtrlP4 |= ImpRgmiiCtrl_Mode_RvMII;
        } else if (IsGMII(EnetInfo[0].sw.phy_id[2])) {
            SWITCH->ImpRgmiiCtrlP4 |= ImpRgmiiCtrl_Mode_GMII;
        } else {
            SWITCH->ImpRgmiiCtrlP4 |= ImpRgmiiCtrl_Mode_MII;
        }

        if (IsRGMII(EnetInfo[0].sw.phy_id[3])) {
            SWITCH->ImpRgmiiCtrlP5 |= ImpRgmiiCtrl_Mode_RGMII;
        } else if (IsRvMII(EnetInfo[0].sw.phy_id[3])) {
            SWITCH->ImpRgmiiCtrlP5 |= ImpRgmiiCtrl_Mode_RvMII;
        } else if (IsGMII(EnetInfo[0].sw.phy_id[3])) {
            SWITCH->ImpRgmiiCtrlP5 |= ImpRgmiiCtrl_Mode_GMII;
        } else {
            SWITCH->ImpRgmiiCtrlP5 |= ImpRgmiiCtrl_Mode_MII;
        }
    }
#endif

    // Reset MIB counters
    SWITCH->GlbMgmt = GlbMgmt_ResetMib;
    udelay(100);
    SWITCH->GlbMgmt = 0;

    SWITCH->ImpOverride |= ImpOverride_Force | ImpOverride_Linkup;

#if defined(SWITCH_PORT_ENABLE_MASK)
    {
        unsigned long ulPortMap = 0;
        // Ports are disabled at reset on 6818 ... 
        if (BpGetSwitchPortMap(&ulPortMap)) /* Failure */
        {
            /* Enable all switch ports */
            ETHSWREG->port_enable |= SWITCH_PORT_ENABLE_MASK;
        }
        else
        {
            /* Enable ports from boardparams + GPON always */
            ETHSWREG->port_enable = ulPortMap | (1<<GPON_PORT_ID);
        }
    }
#endif

}

void robosw_check_ports()
{
    uint16 data;
    uint8 PortOverride; 
    int i;

    for (i = 0; i < EPHY_PORTS; i++) {
        if ((EnetInfo[0].sw.port_map & (1 << i)) != 0) {
            if (!IsPhyConnected(EnetInfo[0].sw.phy_id[i])) {    
#if defined(_BCM96318_) || defined(CONFIG_BCM96318)
                if (IsMII(EnetInfo[0].sw.phy_id[i]))
                {
                    PortOverride = PortOverride_Enable | PortOverride_100Mbs | 
                                   PortOverride_Fdx | PortOverride_Linkup;
                }
                else
#endif
                {
                    PortOverride = PortOverride_Enable | PortOverride_1000Mbs | 
                                   PortOverride_Fdx | PortOverride_Linkup;
                }
                if ((SWITCH->PortOverride[i] & PortOverride) != PortOverride) {
                    SWITCH->PortOverride[i] = PortOverride;
                    SWITCH->PortCtrl[i] = 0;
                    PortLinkState[i] = 1;
                }
                continue;
            }        
            PortOverride = PortOverride_Enable;
            data = mii_read(EnetInfo[0].sw.phy_id[i], MII_ASR);
            if (PortLinkState[i] != MII_ASR_LINK(data)) {
                
                if (MII_ASR_LINK(data)) {
                    PortOverride |= PortOverride_Linkup;
                    printk("\r\n Port %d link UP\r\n", i);
                } else {
                    printk("\r\n Port %d link DOWN\r\n", i);
                }

                if (MII_ASR_DONE(data)) {
                    if (MII_ASR_FDX(data))
                        PortOverride |= PortOverride_Fdx;
                    if (MII_ASR_1000(data))
                        PortOverride |= PortOverride_1000Mbs;
                    else if (MII_ASR_100(data))
                        PortOverride |= PortOverride_100Mbs;
                    else
                        PortOverride |= PortOverride_10Mbs;
                }

                SWITCH->PortOverride[i] = PortOverride;

                if(PortOverride & PortOverride_Linkup) {
                    /* Enable Rx and Tx */
                    SWITCH->PortCtrl[i] = 0;
                }

                PortLinkState[i] = MII_ASR_LINK(data);
            }
        }
    }
}

