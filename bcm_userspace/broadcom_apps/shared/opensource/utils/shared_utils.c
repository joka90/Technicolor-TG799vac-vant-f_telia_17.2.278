
/*
 * <:copyright-BRCM:2012:DUAL/GPL:standard
 * 
 *    Copyright (c) 2012 Broadcom Corporation
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
#include "shared_utils.h"

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

#if defined (CONFIG_BCM96838) || defined(_BCM96838_) 
#ifndef _CFE_             
#include <linux/spinlock.h>                                   
static spinlock_t brcm_sharedUtilslock;
#endif
#endif

unsigned int UtilGetChipRev(void)
{
    unsigned int revId;
#if defined(CONFIG_BCM960333) || defined(_BCM960333_)
    revId = (PERF->ChipID & CHIP_VERSION_MASK) >> CHIP_VERSION_SHIFT;
#else
    revId = PERF->RevID & REV_ID_MASK;
#endif

    return  revId;
}


#if !defined(_CFE_)
EXPORT_SYMBOL(UtilGetChipRev);
#endif

char *UtilGetChipName(char *buf, int len) {
#if defined (CONFIG_BCM960333) || defined (_BCM960333_)
    /* DUNA TODO: Complete implementation for 60333 (chipId and revId) */
    char *mktname = "60333";
    unsigned int chipId = (PERF->ChipID & CHIP_PART_NUMBER_MASK)
                            >> CHIP_PART_NUMBER_SHIFT;
    unsigned int revId = (PERF->ChipID & CHIP_VERSION_MASK)
                            >> CHIP_VERSION_SHIFT;
#else
    unsigned int chipId = (PERF->RevID & CHIP_ID_MASK) >> CHIP_ID_SHIFT;
    unsigned int revId;
    char *mktname = NULL;
    revId = (int) (PERF->RevID & REV_ID_MASK);
#endif

#if  defined (_BCM96818_) || defined(CONFIG_BCM96818)
   unsigned int var = (BRCM_VARIANT_REG & BRCM_VARIANT_REG_MASK) >> BRCM_VARIANT_REG_SHIFT;

    switch ((chipId << 8) | var) {
	case(0x681100):
		mktname = "6812B";
		break;
	case(0x681101):
		mktname = "6812R";
		break;
	case(0x681503):
		mktname = "6812GU";
		break;
	case(0x681500):
		mktname = "6818SR";
		break;
	case(0x681700):
		mktname = "6818G";
		break;
	case(0x681701):
		mktname = "6818GS";
		break;
	case(0x681501):
		mktname = "6818GR";
		break;
	case(0x681502):
		mktname = "6820IAD";
		break;
	default:
		mktname = NULL;
    }

#elif  defined (_BCM96828_) || defined(CONFIG_BCM96828)
#if defined(CHIP_VAR_MASK)
        unsigned int var = (PERF->RevID & CHIP_VAR_MASK) >> CHIP_VAR_SHIFT;
#endif
    switch ((chipId << 8) | var) {
	case(0x682100):
		mktname = "6821F";
		break;
	case(0x682101):
		mktname = "6821G";
		break;
	case(0x682200):
		mktname = "6822F";
		break;
	case(0x682201):
		mktname = "6822G";
		break;
	case(0x682800):
		mktname = "6828F";
		break;
	case(0x682801):
		mktname = "6828G";
		break;
	default:
		mktname = NULL;
		break;
    }
#elif  defined (_BCM96318_) || defined(CONFIG_BCM96318)
#if defined(CHIP_VAR_MASK)
        unsigned int var = (PERF->RevID & CHIP_VAR_MASK) >> CHIP_VAR_SHIFT;
#endif
    switch ((chipId << 8) | var) {
	case(0x63180b):
		mktname = "6318B";
		break;
	default:
		mktname = NULL;
		break;
    }
#elif  defined (_BCM96838_) || defined(CONFIG_BCM96838)
    switch (chipId) {
    case(1):
        mktname = "68380";
        break;
    case(3):
        mktname = "68380F";
        break;
    case(4):
        mktname = "68385";
        break;
    case(5):
        mktname = "68381";
        break;
    case(6):
        mktname = "68380M";
        break;
    case(7):
        mktname = "68389";
        break;
    default:
        mktname = NULL;
    }	
#endif

    if (mktname == NULL) {
	sprintf(buf,"%X%X",chipId,revId);
    } else {
#if  defined (_BCM96838_) || defined(CONFIG_BCM96838)
		sprintf(buf,"%s_", mktname);
		switch(revId)
		{
		case 0:
            strcat(buf,"A0");
			break;
		case 1:
            strcat(buf,"A1");
			break;
		case 4:
            strcat(buf,"B0");
			break;
		case 5:
            strcat(buf,"B1");
			break;
		}
#else
        sprintf(buf,"%s_%X",mktname,revId);
#endif
    }
    return(buf);
}

int UtilGetChipIsPinCompatible(void) 
{

    int ret = 0;
#if  defined (_BCM96818_) || defined(CONFIG_BCM96818)
    unsigned int chipId = (PERF->RevID & CHIP_ID_MASK) >> CHIP_ID_SHIFT;
    unsigned int var = (BRCM_VARIANT_REG & BRCM_VARIANT_REG_MASK) >> BRCM_VARIANT_REG_SHIFT;
    unsigned int sw;
    sw =  ((chipId << 8) | var);
    switch (sw) {
	case(0x681503): //  "6812GU";
	case(0x681500): //  "6818SR";
	case(0x681501): //  "6818GR";
		ret = 1;
		break;
	default:
		ret = 0;
    }
#endif

    return(ret);
}

#if defined (CONFIG_BCM96838) || defined(_BCM96838_) 
unsigned int gpio_get_dir(unsigned int gpio_num)
{
	if( gpio_num < 32 )
		return (GPIO->GPIODir_low & (1<<gpio_num));
	else if( gpio_num < 64 )
		return (GPIO->GPIODir_mid0 & (1<<(gpio_num-32)));
	else
		return (GPIO->GPIODir_mid1 & (1<<(gpio_num-64)));
}

void gpio_set_dir(unsigned int gpio_num, unsigned int dir)
{
	if(gpio_num<32)
	{ 				
		if(dir)	
			GPIO->GPIODir_low |= GPIO_NUM_TO_MASK(gpio_num);	
		else												
			GPIO->GPIODir_low &= ~GPIO_NUM_TO_MASK(gpio_num);	
	}				
	else if(gpio_num<64) 
	{				
		if(dir)	
			GPIO->GPIODir_mid0 |= GPIO_NUM_TO_MASK(gpio_num-32);	
		else													
			GPIO->GPIODir_mid0 &= ~GPIO_NUM_TO_MASK(gpio_num-32);	
	}				
	else			
	{				
		if(dir)	
			GPIO->GPIODir_mid1 |= GPIO_NUM_TO_MASK(gpio_num-64);	
		else													
			GPIO->GPIODir_mid1 &= ~GPIO_NUM_TO_MASK(gpio_num-64);	
	}
}

unsigned int gpio_get_data(unsigned int gpio_num)
{
	if( gpio_num < 32 )
		return ((GPIO->GPIOData_low & (1<<gpio_num)) >> gpio_num);
	else if( gpio_num < 64 )
		return ((GPIO->GPIOData_mid0 & (1<<(gpio_num-32))) >> (gpio_num-32));
	else
		return ((GPIO->GPIOData_mid1 & (1<<(gpio_num-64))) >> (gpio_num-64));
}

void gpio_set_data(unsigned int gpio_num, unsigned int data)
{
	if(gpio_num<32)
	{ 				
		if(data)	
			GPIO->GPIOData_low |= GPIO_NUM_TO_MASK(gpio_num);	
		else												
			GPIO->GPIOData_low &= ~GPIO_NUM_TO_MASK(gpio_num);	
	}				
	else if(gpio_num<64) 
	{				
		if(data)	
			GPIO->GPIOData_mid0 |= GPIO_NUM_TO_MASK(gpio_num-32);	
		else													
			GPIO->GPIOData_mid0 &= ~GPIO_NUM_TO_MASK(gpio_num-32);	
	}				
	else			
	{				
		if(data)	
			GPIO->GPIOData_mid1 |= GPIO_NUM_TO_MASK(gpio_num-64);	
		else													
			GPIO->GPIOData_mid1 &= ~GPIO_NUM_TO_MASK(gpio_num-64);	
	}
}

void set_pinmux(unsigned int pin_num, unsigned int mux_num)
{
   unsigned int tp_blk_data_lsb;
   
#ifndef _CFE_                                                
   int flags;
   spin_lock_irqsave(&brcm_sharedUtilslock, flags);
#endif

   tp_blk_data_lsb= 0;
   tp_blk_data_lsb |= pin_num;
   tp_blk_data_lsb |= (mux_num << PINMUX_DATA_SHIFT);
   GPIO->port_block_data1 = 0;
   GPIO->port_block_data2 = tp_blk_data_lsb;
   GPIO->port_command = LOAD_MUX_REG_CMD;

#ifndef _CFE_                                                
   spin_unlock_irqrestore(&brcm_sharedUtilslock, flags);	
#endif
}

#endif
#if defined (CONFIG_BCM96838) || defined(_BCM96838_) 
#ifndef _CFE_                                                
EXPORT_SYMBOL(set_pinmux);
EXPORT_SYMBOL(gpio_get_dir);
EXPORT_SYMBOL(gpio_set_dir);
EXPORT_SYMBOL(gpio_get_data);
EXPORT_SYMBOL(gpio_set_data);
#endif
#endif
    


