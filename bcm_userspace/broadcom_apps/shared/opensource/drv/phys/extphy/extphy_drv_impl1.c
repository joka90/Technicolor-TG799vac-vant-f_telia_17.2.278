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

/*
 * This file contains the implementation for external RGMII Phy
 * In 6838 SV Boards there is B50612E 10/100/1000 Gigabit Ethernet Transceiver
 */

/*****************************************************************************/
/*                                                                           */
/* Include files                                                             */
/*                                                                           */
/*****************************************************************************/
#include "extphy_drv.h"
#include "mdio_drv.h"
#include "boardparms.h"


/*
 * Register Defines
 */


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
void extPhyReset(void)
{
	/*here you can put pre-initialization*/
}


/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   egPhyAutoEnableExtraConfig					                              */
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
void extPhyAutoEnableExtraConfig(int mode)
{
	/*add your extra configuration code for external phy when it becomes enable*/
}

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   extPhyReadRegister							    		                  */
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
uint16_t extPhyReadRegister(uint32_t phyID,uint32_t regOffset)
{
	uint16_t value = 0;

	if (mdio_read_c22_register(MDIO_EXT,phyID,regOffset,&value) == MDIO_ERROR)
	{
		printk("failed to read register phy %ul regOffset %ul\n",phyID,regOffset);
		value = 0;
	}

	return value;
}
EXPORT_SYMBOL(extPhyReadRegister);
/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   extPhyWriteRegister							    		                  */
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
int32_t extPhyWriteRegister(uint32_t phyID,uint32_t regOffset,uint16_t regValue)
{
	int32_t ret;


	ret=mdio_write_c22_register(MDIO_EXT,phyID, regOffset,regValue);
	if (ret== MDIO_ERROR)
	{
			printk("failed to write register phy %ul regOffset %ul\n",phyID,regOffset);
			ret=-1;
	}

	return ret;
}
EXPORT_SYMBOL(extPhyWriteRegister);

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   extPhyLoopback                                                           */
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
int32_t extPhyLoopback(uint32_t phyID, phy_loopback_t loopback)
{
    /* here customer implement external phy loopback */
    return 0;
}
EXPORT_SYMBOL(extPhyLoopback);
