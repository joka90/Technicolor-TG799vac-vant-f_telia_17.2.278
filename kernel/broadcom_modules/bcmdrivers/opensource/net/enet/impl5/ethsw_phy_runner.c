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

#include <bcm_map_part.h>
#include "bcmenet_common.h"
#include "bcmswaccess.h"
#include "ethsw_runner.h"
#include "ethsw_phy.h"

uint16_t ethSwPhyReadRegister(int phy_id, int reg, int ext);
int ethSwPhyWriteRegister(int phy_id, int reg, uint16 data, int ext);
extern spinlock_t bcm_ethlock_phy_access;

#if defined( CONFIG_BCM96838)
	#include "phys_common_drv.h"
	#define ReadPHYReg(_phy_id, _reg, ext_bit) PhyReadRegister(ext_bit ? _phy_id|PHY_EXTERNAL :_phy_id, _reg)
	#define WritePHYReg(_phy_id, _reg,_data, ext_bit) PhyWriteRegister(ext_bit ? _phy_id|PHY_EXTERNAL :_phy_id, _reg, _data)
#elif defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
	#define ReadPHYReg(_phy_id, _reg, ext_bit) ethSwPhyReadRegister(_phy_id, _reg, ext_bit)
	#define WritePHYReg(_phy_id, _reg,_data, ext_bit) ethSwPhyWriteRegister(_phy_id, _reg, _data, ext_bit)
#endif


void ethsw_phy_rw_reg(int phy_id, int reg, uint16 *data, int ext_bit, int rd)
{
    int status;

    if(rd)
    {
        status = ReadPHYReg(phy_id, reg, ext_bit);
        if (status >= 0) 
        {
            *data = (uint16)status;
        }
        else
        {
            printk("ERROR : ethsw_phy_read_reg(%d, %d, **)\n",phy_id,reg);
        }
    }
    else
    {
        status = WritePHYReg(phy_id, reg, *data, ext_bit);
        if (status < 0) 
        {
            printk("ERROR : ethsw_phy_wreg(%d, %d, 0x%04x)\n",phy_id,reg,*data);
        }
    }
}

#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
volatile uint32 *sf2_mdio_command_reg     = (void *)(SWITCH_BASE + SF2_MDIO_COMMAND_REG);
// FIXME:  You need the following to configure c45 access mode.
//volatile uint32 *sf2_mdio_config_reg      = (void *)(SWITCH_BASE + SF2_MDIO_CONFIG_REG);

uint16_t ethSwPhyReadRegister(int phy_id, int reg, int ext)
{
    int reg_value = 0;
    int i = 0;

    phy_id &= BCM_PHY_ID_M;
    spin_lock(&bcm_ethlock_phy_access);

    if(ext == ETHCTL_FLAG_ACCESS_I2C_PHY)
    {
#if defined(SWITCH_REG_SINGLE_SERDES_CNTRL) && defined(CONFIG_I2C)
        sfp_i2c_phy_read(reg, &reg_value);
#else
        printk("Error: I2C PHY is not supported\n");
#endif
        goto endSfp;
    }

    if (ext == ETHCTL_FLAG_ACCESS_SERDES_POWER_MODE)
    {
#if defined(SWITCH_REG_SINGLE_SERDES_CNTRL)
        ethsw_serdes_power_mode_get(phy_id, &reg_value);
#else
        printk("Error: Serdes is not supported in this platform\n");
#endif
        goto endSfp;
    }
        
    sf2_mdio_master_disable();
    reg_value = (phy_id & SF2_MDIO_C22_PHY_ADDR_M) << SF2_MDIO_C22_PHY_ADDR_S |
                 (reg & SF2_MDIO_C22_PHY_REG_M) << SF2_MDIO_C22_PHY_REG_S |
                        (SF2_MDIO_CMD_C22_READ << SF2_MDIO_CMD_S);
    *sf2_mdio_command_reg = reg_value | SF2_MDIO_BUSY;
    do {
         if (++i >= 3)  {
             printk("%s MDIO No Response! phy 0x%x reg 0x%x ext 0x%x  \n", __FUNCTION__, phy_id, reg, ext);
             spin_unlock(&bcm_ethlock_phy_access);
             return 0;
         }
         udelay(60);
         reg_value = *sf2_mdio_command_reg;
    } while (reg_value & SF2_MDIO_BUSY);
    /* Read a second time to ensure it is reliable */
    *sf2_mdio_command_reg = reg_value | SF2_MDIO_BUSY;
    do {
         if (++i >= 3)  {
             printk("%s MDIO No Response! phy 0x%x reg 0x%x ext 0x%x  \n", __FUNCTION__, phy_id, reg, ext);
             spin_unlock(&bcm_ethlock_phy_access);
             return 0;
         }
         udelay(60);
         reg_value = *sf2_mdio_command_reg;
    } while (reg_value & SF2_MDIO_BUSY);

    sf2_mdio_master_enable();
endSfp:
    spin_unlock(&bcm_ethlock_phy_access);
    return (u16)reg_value;
}

int ethSwPhyWriteRegister(int phy_id, int reg, uint16 data, int ext)
{
    int reg_value = 0;
    int i = 0;

    phy_id &= BCM_PHY_ID_M;

    if(ext == ETHCTL_FLAG_ACCESS_I2C_PHY)
    {
#if defined(SWITCH_REG_SINGLE_SERDES_CNTRL) && defined(CONFIG_I2C)
        sfp_i2c_phy_write(reg, data);
#else
        printk("Error: I2C PHY is not supported\n");
#endif
        return 0;
    }
        
    if (ext == ETHCTL_FLAG_ACCESS_SERDES_POWER_MODE)
    {
#if defined(SWITCH_REG_SINGLE_SERDES_CNTRL)
        ethsw_serdes_power_mode_set(phy_id, reg);
#else
        printk("Error: Serdes is not supported in this platform\n");
#endif
        return 0;
    }
        
    spin_lock(&bcm_ethlock_phy_access);
    sf2_mdio_master_disable();
    reg_value = (phy_id & SF2_MDIO_C22_PHY_ADDR_M) << SF2_MDIO_C22_PHY_ADDR_S |
                 (reg & SF2_MDIO_C22_PHY_REG_M) << SF2_MDIO_C22_PHY_REG_S |
                        (SF2_MDIO_CMD_C22_WRITE << SF2_MDIO_CMD_S) | data ;
    *sf2_mdio_command_reg = reg_value | SF2_MDIO_BUSY;
    do {
         if (++i >= 3)  {
             printk("%s MDIO No Response! phy 0x%x reg 0x%x ext 0x%x  \n", __FUNCTION__, phy_id, reg, ext);
             spin_unlock(&bcm_ethlock_phy_access);
             return 0;
         }
         udelay(60);
         reg_value = *sf2_mdio_command_reg;
    } while (reg_value & SF2_MDIO_BUSY);
    sf2_mdio_master_enable();
    spin_unlock(&bcm_ethlock_phy_access);
    return 0;
}
#endif


MODULE_LICENSE("GPL");

