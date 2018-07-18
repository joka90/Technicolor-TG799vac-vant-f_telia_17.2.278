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
/* This file contains the implementation for 6838 Common Phys Driver          */
/*                                                                            */
/******************************************************************************/

/*****************************************************************************/
/*                                                                           */
/* Include files                                                             */
/*                                                                           */
/*****************************************************************************/
#include "phys_common_drv.h"
#include "egphy_drv.h"
#include "extphy_drv.h"
#include "mdio_drv.h"
#include "boardparms.h"

typedef struct S_PhyCache
{
	uint32_t	phyId;
	uint32_t	ifFlags;
	uint32_t	mdio;
	uint32_t	phyvalid;
}S_PhyCache;

static inline void fillBpPhyCache(void);
static int32_t PhyGetLinkState(uint32_t phyID,MDIO_TYPE mdioType);


static S_PhyCache 	phyAddrCache[BP_MAX_SWITCH_PORTS] = {[0 ... (BP_MAX_SWITCH_PORTS-1)] = {0,0,0,0} };
static uint32_t 	PhyCacheInitialized = 0;
static const ETHERNET_MAC_INFO*    pE;


static int32_t PhyGetLinkState(uint32_t phyID,MDIO_TYPE mdioType)
{
	uint16_t statReg;
	uint32_t retCode;

	if (mdio_read_c22_register(mdioType, phyID, MII_BMSR, &statReg)
			== MDIO_ERROR)
	{
		/*put some log error*/
		return -1;
	}
	/*check the link stat bit*/
	if (statReg & BMSR_LSTATUS)
	{
		retCode = PHY_LINK_ON;
	}
	else
	{
		retCode = PHY_LINK_OFF;
	}
	return retCode;
}

static inline void fillBpPhyCache(void)
{
	uint32_t			iter;

	/*already initialized*/
	if(PhyCacheInitialized)
		return;

	if ( (pE = BpGetEthernetMacInfoArrayPtr()) == NULL )
	{
		printk("ERROR:BoardID not Set in BoardParams\n");
		return ;
	}

	for( iter = 0 ; iter < BP_MAX_SWITCH_PORTS; iter++)
	{
	    /*is port enabled?*/
	    if(pE[0].sw.port_map & (1<<iter))
	    {
            phyAddrCache[iter].phyId 	= pE[0].sw.phy_id[iter] & BCM_PHY_ID_M;
            phyAddrCache[iter].ifFlags	= pE[0].sw.phy_id[iter] & MAC_IFACE;
            if(pE[0].sw.phy_id[iter] & PHY_EXTERNAL)
                phyAddrCache[iter].mdio = MDIO_EXT;
            else if (phyAddrCache[iter].ifFlags == MAC_IF_SERDES)
            {
                phyAddrCache[iter].mdio = MDIO_AE;
                /* for case of working mac to mac PHYCFG_VALID will not be configured in BP */
                if(pE[0].sw.phy_id[iter] & PHYCFG_VALID)
                    phyAddrCache[iter].phyvalid = 1;
            }                
            else
                phyAddrCache[iter].mdio = MDIO_EGPHY;
	    }
	}

	PhyCacheInitialized  = 1;
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
void	PhyReset(uint32_t port_map)
{
	/*here you do some initialization needed before configuring the phy*/
	egPhyReset(port_map);
}
EXPORT_SYMBOL(PhyReset);
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
void  PhyAutoEnable(uint32_t macId)
{
	uint16_t anAdvBits;
	uint16_t ctrlReg;
	uint32_t	phyId;
	MDIO_TYPE	mdioType;
	uint32_t	mode;
	uint32_t    phyvalid;
	uint32_t mdio_data = 0x0;


	fillBpPhyCache();
	if(!( pE[0].sw.port_map & (1<<macId) ))
	{
		printk("trying to access to phy of wrong Mac = %d\n",macId);
		return;
	}
	phyId = phyAddrCache[macId].phyId;
	mdioType = phyAddrCache[macId].mdio;
	mode   = phyAddrCache[macId].ifFlags;
    phyvalid = phyAddrCache[macId].phyvalid;

    /* configure serdes based phy */
	if(mode == MAC_IF_SERDES)
	{
		//Set block address to zero
		if (mdio_write_c22_register(MDIO_AE ,phyId, MII_PTEST1_RDBRW, mdio_data))
			printk("failed MDIO_AE write reg=0x%x data=0x%x\n ",MII_PTEST1_RDBRW,mdio_data);
		/* In case of working serdess mac to mac phyvalid will be 0 */
		if (phyvalid)
		{
			mdio_data = 0x1140;
			if (mdio_write_c22_register(MDIO_AE ,phyId, MII_BMCR, mdio_data))
				printk("failed MDIO_AE write reg=0x%x data=0x%x\n ",MII_BMCR,mdio_data);
			mdio_data = 0x01a0;
			if (mdio_write_c22_register(MDIO_AE ,phyId, MII_ADVERTISE, mdio_data))
				printk("failed MDIO_AE write reg=0x%x data=0x%x\n ",MII_ADVERTISE,mdio_data);
		}            
		else /* Disable MII autoNeg and forced to 1000M ,full duplex */
		{
			mdio_data = 0x0140;
			if (mdio_write_c22_register(MDIO_AE ,phyId, MII_BMCR, mdio_data))
				printk("failed MDIO_AE write reg=0x%x data=0x%x\n ",MII_BMCR,mdio_data);
		}
		mdio_data = 0x4101;
		if (mdio_write_c22_register(MDIO_AE ,phyId, MII_XCTL, mdio_data))
			printk("failed MDIO_AE write reg=0x%x data=0x%x\n ",MII_XCTL,mdio_data);
        if (mdioType == MDIO_AE)
            return;
	}

	/*reset phy*/
	if (mdio_write_c22_register(mdioType, phyId, MII_BMCR, BMCR_RESET)
			== MDIO_ERROR)
	{
		/*put some log error*/
		return;
	}

	udelay(300);
	/*write the autoneg bits*/
	anAdvBits = ADVERTISE_CSMA | ADVERTISE_10HALF | ADVERTISE_10FULL
	    | ADVERTISE_100HALF | ADVERTISE_100FULL | ADVERTISE_PAUSE_CAP;
	if (mdio_write_c22_register(mdioType, phyId, MII_ADVERTISE, anAdvBits)
			== MDIO_ERROR)
	{
		/*put some log error*/
		return;
	}
	mdio_read_c22_register(mdioType, phyId, MII_CTRL1000, &anAdvBits);
        /* Favor clock master for better compatibility when in EEE */
	anAdvBits = ADVERTISE_REPEATER;
	if ( mode == MAC_IF_GMII || mode == MAC_IF_SERDES)
	{
		/*if 1000 is supported also advertise 1000 capability*/
		anAdvBits |= ADVERTISE_1000FULL ;
	}
	if (mdio_write_c22_register(mdioType, phyId, MII_CTRL1000, anAdvBits)
					== MDIO_ERROR)
	{
		/*put some log error*/
		return;
	}
	/*initiate autonegotiation*/
	if (mdio_read_c22_register(mdioType, phyId, MII_BMCR, &ctrlReg)
			== MDIO_ERROR)
	{
		/*put some log error*/
		return;
	}
	ctrlReg |= BMCR_ANENABLE | BMCR_ANRESTART;
	if (mdio_write_c22_register(mdioType, phyId, MII_BMCR, ctrlReg)
			== MDIO_ERROR)
	{
		/*put some log error*/
		return;
	}

	/*now do some extra configuration according to the phy type*/
	switch(mdioType)
	{
		case MDIO_EGPHY:
			egPhyAutoEnableExtraConfig(phyId,mode);
			break;
		case MDIO_EXT:
			extPhyAutoEnableExtraConfig(mode);
			break;
		default:
			/*nothing*/
			break;
	}
}
EXPORT_SYMBOL(PhyAutoEnable);
/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   PhyShutDown								                              */
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
void PhyShutDown(uint32_t macId)
{
	MDIO_TYPE	mdioType;
	uint32_t 	phyId ;


	fillBpPhyCache();
	if(!( pE[0].sw.port_map & (1<<macId) ))
	{
		printk("trying to access to phy of wrong Mac = %d\n",macId);
		return;
	}
	phyId	=  phyAddrCache[macId].phyId;
	mdioType = phyAddrCache[macId].mdio;

	if (mdio_write_c22_register(mdioType, phyId  , MII_BMCR, BMCR_PDOWN)
				== MDIO_ERROR)
	{
		/*put some log error*/
		return;
	}

}
EXPORT_SYMBOL(PhyShutDown);
/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   PhyGetLinkState							                              */
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


/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   egPhyGetLineRateAndDuplex							                      */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*  function to retrieve the current status of the phy		                  */
/*                                                                            */
/* Abstract:                                                                  */
/*   get the current status of the phy in manners of rate and duplex          */
/*                                                                            */
/* Input:                                                                     */
/*     phyID	-	if of phy in MDIO bus                                     */
/* Output:																	  */
/*																			  */
/* Return:                                                                    */
/* 	  GPHY_RATE - enumation                                                   */
/******************************************************************************/
int32_t PhyGetLineRateAndDuplex(uint32_t macId)
{
	uint32_t	phyId;
	MDIO_TYPE	mdioType;
	uint16_t	miiCtrl;
	uint16_t	ctrl1000;
	uint16_t	stat1000;
	uint16_t	lpa;
	uint16_t    adv;
	uint32_t 	result = PHY_RATE_LINK_DOWN;



	fillBpPhyCache();
	if(!( pE[0].sw.port_map & (1<<macId) ))
    {
    	printk("trying to access to phy of wrong Mac = %d\n",macId);
    	return PHY_RATE_ERR;
    }
	phyId 	 = phyAddrCache[macId].phyId;
	mdioType = phyAddrCache[macId].mdio;

	if (PhyGetLinkState(phyId,mdioType) != PHY_LINK_ON)
	{
		return PHY_RATE_LINK_DOWN;
	}
	/*check if autoneg is enabled*/
	if (mdio_read_c22_register(mdioType, phyId, MII_BMCR, &miiCtrl)
			== MDIO_ERROR)
	{
		/*put some log error*/
		return PHY_RATE_ERR;
	}
	/*if autoneg enabled*/
	if (miiCtrl & BMCR_ANENABLE)
	{
		/*read the status register*/
		if (mdio_read_c22_register(mdioType, phyId, MII_BMSR, &miiCtrl)
				== MDIO_ERROR)
		{
			/*put some log error*/
			return PHY_RATE_ERR;
		}
		/*if autoneg didn't complete leave the function*/
		if (!(miiCtrl & BMSR_ANEGCOMPLETE))
		{
			return PHY_RATE_LINK_DOWN;
		}
		/*first read the 1000 ctrl reg*/
		if (mdio_read_c22_register(mdioType, phyId, MII_CTRL1000, &ctrl1000)
				== MDIO_ERROR)
		{
			/*put some log error*/
			return PHY_RATE_ERR;
		}

		/* read the 1000 stat reg*/
		if (mdio_read_c22_register(mdioType, phyId, MII_STAT1000, &stat1000)
				== MDIO_ERROR)
		{
			/*put some log error*/
			return PHY_RATE_ERR;
		}
		/*aligne ctrl1000 with stat1000*/
		stat1000 &= ctrl1000 << 2;

		/* read the read partner reg*/
		if (mdio_read_c22_register(mdioType, phyId, MII_LPA, &lpa)
				== MDIO_ERROR)
		{
			/*put some log error*/
			return PHY_RATE_ERR;
		}
		/* read the read advertise reg*/
		if (mdio_read_c22_register(mdioType, phyId, MII_ADVERTISE, &adv)
				== MDIO_ERROR)
		{
			/*put some log error*/
			return PHY_RATE_ERR;
		}
		/*match lpa with adv*/
		lpa &= adv;

		/*check speeds*/
		if (stat1000 & (LPA_1000FULL | LPA_1000HALF))
		{
			if (stat1000 & LPA_1000FULL)
			{
				result = PHY_RATE_1000_FULL;
			}
			else
			{
				result = PHY_RATE_1000_HALF;
			}
		}
		else if (lpa & (LPA_100FULL | LPA_100HALF))
		{
			if (lpa & LPA_100FULL)
			{
				result = PHY_RATE_100_FULL;
			}
			else
			{
				result = PHY_RATE_100_HALF;
			}
		}
		else if (lpa & (LPA_10FULL | LPA_10HALF))
		{
			if (lpa & LPA_10FULL)
			{
				result = PHY_RATE_10_FULL;
			}
			else
			{
				result = PHY_RATE_10_HALF;
			}
		}

	}
	else // no autoneg
	{
		/*read the status register*/
		if (mdio_read_c22_register(mdioType, phyId, MII_BMCR, &miiCtrl)
				== MDIO_ERROR)
		{
			/*put some log error*/
			return PHY_RATE_ERR;
		}
		if (miiCtrl & BMCR_SPEED1000)
		{
			result =
					(miiCtrl & BMCR_FULLDPLX) ?
							PHY_RATE_1000_FULL : PHY_RATE_1000_HALF;
		}
		else if (miiCtrl & BMCR_SPEED100)
		{
			result =
					(miiCtrl & BMCR_FULLDPLX) ?
							PHY_RATE_100_FULL : PHY_RATE_100_HALF;
		}
		else
		{
			result =
					(miiCtrl & BMCR_FULLDPLX) ?
							PHY_RATE_10_FULL : PHY_RATE_10_HALF;
		}
	}
	return result;
}
EXPORT_SYMBOL(PhyGetLineRateAndDuplex);

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
uint16_t PhyReadRegister(uint32_t phyId,uint32_t regOffset)
{

	if ( phyId & PHY_EXTERNAL)
	{
		return extPhyReadRegister( phyId & BCM_PHY_ID_M ,regOffset );
	}
	else
	{
		return egPhyReadRegister( phyId & BCM_PHY_ID_M ,regOffset );
	}
}
EXPORT_SYMBOL(PhyReadRegister);

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
int32_t PhyWriteRegister(uint32_t phyId,uint32_t regOffset,uint16_t regValue)
{

	if ( phyId & PHY_EXTERNAL)
	{
		extPhyWriteRegister(phyId & BCM_PHY_ID_M ,regOffset,regValue );
	}
	else
	{
		return egPhyWriteRegister( phyId & BCM_PHY_ID_M ,regOffset,regValue );
	}
	return 0;
}
EXPORT_SYMBOL(PhyWriteRegister);

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   PhyLoopback                                                              */
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
int32_t PhyLoopback(uint32_t macId, phy_loopback_t loopback)
{
    uint32_t    phyId;

    fillBpPhyCache();
    if(!( pE[0].sw.port_map & (1<<macId) ))
    {
        printk("trying to access to phy of wrong Mac = %d\n",macId);
        return -1;
    }
    phyId = phyAddrCache[macId].phyId;

    if (phyId & PHY_EXTERNAL)
        extPhyLoopback(phyId,loopback);
    else
        egPhyLoopback(phyId,loopback);

    return 0;
}
EXPORT_SYMBOL(PhyLoopback);
