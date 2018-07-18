/*
   Copyright (c) 2013 Broadcom Corporation
   All Rights Reserved

    <:label-BRCM:2013:DUAL/GPL:standard

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
/******************************************************************************/
/*                                                                            */
/* File Description:                                                          */
/*                                                                            */
/* This file contains the implementation for Broadcom's QuadPhy block         */
/*                                                                            */
/******************************************************************************/

/*****************************************************************************/
/*                                                                           */
/* Include files                                                             */
/*                                                                           */
/*****************************************************************************/
#include "egphy_drv.h"
#include "mdio_drv.h"
#include "rdp_map.h"

/*
 * structure describing the EGPHY OUT register
 */
#pragma pack(push,1)
typedef struct
{
	 uint32_t	reserved1:17;//	 Reserved bits must be written with 0.  A read returns an unknown value.
	 uint32_t	PHY_TEST_EN:1;//	 Phy test enable	0x1 = Test_mode  	Reset value is 0x0.
	 uint32_t	PHYA:5;//	 phyaReset value is 0x1.
	 uint32_t	GPHY_CK25_DIS:1;//	 gphy_ck25_disable 	Reset value is 0x0.
	 uint32_t	IDDQ_BIAS:1;//	 i_mac_gphy_cfg_iddq_bias Reset value is 0x1.
	 uint32_t	DLL_EN:1;//	 i_mac_gphy_cfg_ext_force_dll_en Reset value is 0x0.
	 uint32_t	PWRDWN:4;//	 i_mac_gphy_cfg_ext_pwrdown	Reset value is 0xf.
	 uint32_t	reserved0:1;//	 Reserved bit must be written with 0.  A read returns an unknown value.
	 uint32_t	RST:1;// i_mac_gphy_cfg_reset_b	0x1 = not_rst Reset value is 0x0.
}EGPHY_GPHY_OUT;
#pragma pack(pop)


static void changeEGphyRDBAccess(uint32_t phyID,uint32_t enable)
{

	uint16_t value = 0x0f00 ;
	if (enable)
	{
		value	|= (uint16_t)EXP_RDP_ENABLE_REG;

		if( mdio_write_c22_register(MDIO_EGPHY,phyID,MII_RDB_ENABLE_OFFSET,value) == MDIO_ERROR)
		{

			return ;
		}
		if( mdio_write_c22_register(MDIO_EGPHY,phyID,MII_RDB_VALUE_OFFSET,0) == MDIO_ERROR)
		{
			/*put some log error*/
			return ;
		}


	}
	else
	{

		if( mdio_write_c22_register(MDIO_EGPHY,phyID,0x1e,0x0087) == MDIO_ERROR)
		{

			return ;
		}
		if( mdio_write_c22_register(MDIO_EGPHY,phyID,0x1f,0x8000) == MDIO_ERROR)
		{
			/*put some log error*/
			return ;
		}

	}

}
/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   egPhyReset									                              */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   reset the EGPHY IP in 			         				                  */
/*                                                                            */
/* Abstract:                                                                  */
/*   initialize the EGPHY block, must be called before any MAC				  */
/* 	 operations                                                               */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/******************************************************************************/
void egPhyReset(uint32_t port_map)
{
	/*read the egphy register*/
	EGPHY_GPHY_OUT gPhyOut;

	READ_32(EGPHY_RDP_UBUS_MISC_EGPHY_GPHY_OUT,gPhyOut);

	gPhyOut.RST			=	1;
	gPhyOut.IDDQ_BIAS	=	1;
	gPhyOut.PWRDWN		=	0xf;

	WRITE_32(EGPHY_RDP_UBUS_MISC_EGPHY_GPHY_OUT,gPhyOut);

	udelay(50);

	gPhyOut.IDDQ_BIAS	=	0;

	WRITE_32(EGPHY_RDP_UBUS_MISC_EGPHY_GPHY_OUT,gPhyOut);

	gPhyOut.PHYA		= 	1;
	gPhyOut.PWRDWN		=	~port_map & 0xf;

	WRITE_32(EGPHY_RDP_UBUS_MISC_EGPHY_GPHY_OUT,gPhyOut);

	//udelay(50);

//	/*set the value of PHYA to 1
//	 * all access to phy addresses is depended by this value, it's the offset
//	 * from the MDIO address*/
//	gPhyOut.RST			=	0;
//	WRITE_32(EGPHY_RDP_UBUS_MISC_EGPHY_GPHY_OUT,gPhyOut);
//	udelay(200);
//
//	gPhyOut.RST			=	1;
//	WRITE_32(EGPHY_RDP_UBUS_MISC_EGPHY_GPHY_OUT,gPhyOut);
//	udelay(50);

	/*phy is ready to go!*/
}


/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   egPhyAutoEnable							                              */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*  configure the phy to auto negotiation mode				                  */
/*                                                                            */
/* Abstract:                                                                  */
/*   configure the phy to work in auto negotiation  with respect to the		  */
/*   requested rate limit								                      */
/*                                                                            */
/* Input:                                                                     */
/*     phyID	-	if of phy in MDIO bus                                     */
/*		mode	- rate limit to advertise ( 100,1000)						  */
/* Output:                                                                    */
/*                                                                            */
/******************************************************************************/
void egPhyAutoEnableExtraConfig(uint32_t phyID, int mode)
{
	egPhyWriteRegister(phyID,RDB_REG_ACCESS|CORE_SHD1C_0D,CORE_SHD_BICOLOR_LED0);
	egPhyWriteRegister(phyID,RDB_REG_ACCESS|CORE_SHD18_111,FORCE_AUTO_MDIX);
}
/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   egPhyReadRegister							    		                  */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*  function to retrieve a phy register content				                  */
/*                                                                            */
/* Abstract:                                                                  */
/*   get the current status of the phy in manners of rate and duplex          */
/*                                                                            */
/* Input:                                                                     */
/*     phyID	-	                                     					  */
/*     regOffset -															  */
/* Output:																	  */
/*																			  */
/* Return:                                                                    */
/* 	  read value -			                                                   */
/******************************************************************************/
uint16_t egPhyReadRegister(uint32_t phyID,uint32_t regOffset)
{
	uint16_t value = 0;
	int 	rdbEnabled = regOffset & 0x8000;
	if(rdbEnabled)
	{
		changeEGphyRDBAccess(phyID,1);
		if ( mdio_write_c22_register(MDIO_EGPHY,phyID,0x1e,regOffset) == MDIO_ERROR)
		{
				printk("failed to write register phy %ul rdb regOffset %ul\n",phyID,regOffset);
				value=-1;
		}
	}

	if (mdio_read_c22_register(MDIO_EGPHY,phyID,rdbEnabled ? 0x1f : regOffset,&value) == MDIO_ERROR)
	{
		printk("failed to read register phy %ul regOffset %ul\n",phyID,regOffset);
		value = 0;
	}

	if(rdbEnabled)
		changeEGphyRDBAccess(phyID,0);

	return value;
}
EXPORT_SYMBOL(egPhyReadRegister);

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   egPhyWriteRegister							    		                  */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*  function to set a phy register content				       		          */
/*                                                                            */
/* Abstract:                                                                  */
/*   get the current status of the phy in manners of rate and duplex          */
/*                                                                            */
/* Input:                                                                     */
/*     phyID	-	                                     					  */
/*     regOffset -															  */
/*     regValue -															  */
/* Output:																	  */
/*																			  */
/* Return:                                                                    */
/******************************************************************************/
int32_t egPhyWriteRegister(uint32_t phyID,uint32_t regOffset,uint16_t regValue)
{
	int32_t ret = 0 ;
	int 	rdbEnabled = regOffset & 0x8000;

	if(rdbEnabled)
	{
		changeEGphyRDBAccess(phyID,1);
		if (mdio_write_c22_register(MDIO_EGPHY,phyID,0x1e,regOffset)== MDIO_ERROR)
		{
				printk("failed to write register phy %ul rdb regOffset %ul\n",phyID,regOffset);
				ret=-1;
		}
	}

	ret=mdio_write_c22_register(MDIO_EGPHY,phyID,rdbEnabled ? 0x1f : regOffset,regValue);
	if (ret== MDIO_ERROR)
	{
			printk("failed to write register phy %ul regOffset %ul\n",phyID,regOffset);
			ret=-1;
	}
	if(rdbEnabled)
		changeEGphyRDBAccess(phyID,0);

	return ret;
}
EXPORT_SYMBOL(egPhyWriteRegister);

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   egPhyLoopback                                                           */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*  function to set a phy loopback                                            */
/*                                                                            */
/* Abstract:                                                                  */
/*   set local or remote loopback on a phy                                    */
/*                                                                            */
/* Input:                                                                     */
/*     phyID    -                                                             */
/*     phy_loopback_t -                                                       */
/* Output:                                                                    */
/*                                                                            */
/* Return:                                                                    */
/******************************************************************************/
int32_t egPhyLoopback(uint32_t phyID, phy_loopback_t loopback)
{
    uint16_t value;
    uint8_t  speed;

    switch (loopback)
    {
        case PHY_LOOPBACK_NONE: /* disable both local and remote loopback */
        {
            /* followed by phy reset, do nothing */
            break;
        }
       case   PHY_LOOPBACK_LOCAL:
       {
           /* first determine the current speed */
           value = egPhyReadRegister(phyID,RDB_REG_ACCESS|CORE_BASE19);
           speed = (value & CORE_BASE19_AUTONEG_HCD_MASK) >> CORE_BASE19_AUTONEG_HCD_OFFSET;

           switch(speed)
           {
               case 6:
               case 7:
               {
                   value = 0x40;
                   egPhyWriteRegister(phyID,0,value);
                   value = 0x1800;
                   egPhyWriteRegister(phyID,9,value);
                   break;
               }
               case 4:
               case 5:
               {
                   value = 0x2100;
                   egPhyWriteRegister(phyID,0,value);
                   break;
               }
               case 3:
               case 2:
               {
                  value = 0x0100;
                  egPhyWriteRegister(phyID,0,value);
                  break;
               }
               default:
                   printk("ERR:couldn't determine phy speed");
                   return -1;

           }
            /* read auxiliry control register */
            value = egPhyReadRegister(phyID,RDB_REG_ACCESS|CORE_SHD18_000);
            /* unset EXT_LPBK and SM_DSP_CLK_EN bits */
            value |= (CORE_SHD18_000_EXT_LPBK|CORE_SHD18_000_SM_DSP_CLK_EN);
            /* write back the vlue to auxiliry control register */
            egPhyWriteRegister(phyID,RDB_REG_ACCESS|CORE_SHD18_000,value);

            /* read auxiliry control register */
            value = egPhyReadRegister(phyID,RDB_REG_ACCESS|CORE_SHD18_100);
            /* unset SM_TDK_FIX_EN and SM_DSP_CLK_EN bits */
            value |= (CORE_SHD18_100_SM_TDK_FIX_EN|CORE_SHD18_100_SWAP_RXMDIX);
            /* write back the vlue to auxiliry control register */
            egPhyWriteRegister(phyID,RDB_REG_ACCESS|CORE_SHD18_100,value);
            break;
       }
       case PHY_LOOPBACK_REMOTE:
       {
            /* read auxiliry control register */
            value = egPhyReadRegister(phyID,RDB_REG_ACCESS|CORE_SHD18_100);
            /* unset SM_TDK_FIX_EN and SM_DSP_CLK_EN bits */
            value |= (CORE_SHD18_100_RMT_LPBK_EN);
            /* write back the vlue to auxiliry control register */
            egPhyWriteRegister(phyID,RDB_REG_ACCESS|CORE_SHD18_100,value);

            /* restart autoneg */
            value = egPhyReadRegister(phyID,0);
            value |= (0x20);
            egPhyWriteRegister(phyID,0,value);
            break;
       }
       default:
           /* should not reach here*/
           break;
    }
    return 0;
}
EXPORT_SYMBOL(egPhyLoopback);
