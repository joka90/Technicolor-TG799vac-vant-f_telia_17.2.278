/* ------------------------------------------------------------------------- */
/* i2c-algo-bcm.c i2c driver algorithms for BCM68570 Embedded I2C adapter     */
/* ------------------------------------------------------------------------- */
/*   Copyright (C) 2008 Pratapa Reddy, Vaka <pvaka@broadcom.com>

     <:label-BRCM:2012:DUAL/GPL:standard

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
/* ------------------------------------------------------------------------- */

/* Referenced and Reused the code form i2c-algo-bcm.c file */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/bcm_log.h>
#include "bcm_map_part.h"
#include <bcmtypes.h>
#include "bl_lilac_i2c.h"

BL_LILAC_I2C_CONF i2c_params;

#define GPON_PHY_EEPROM_SIZE 256
#define MAX_LILAC_I2C_BYTES_NUM 4
#define MAX_TRANSACTION_SIZE  32

void Laser_Dump_Buf(u8* buf, u32 count, u32 offset)
{
	int i = 0; 

	printk("buf[%d..%d]=[", offset, offset+count-1);
  
	for (i=0; i<count; i++) 
		printk("%02x ", buf[i]);

	printk("]\n");
}

/****************************************************************************/
/* Read M02010: Reads count number of bytes from M02100                     */
/* Returns:                                                                 */
/*   number of bytes read on success, negative value on failure.            */
/****************************************************************************/
static int Laser_Dev_Read(struct i2c_msg *msgs)
{
	BL_LILAC_I2C_STATUS i2cStatus;
	int i=0, readTimes=0, remainder = 0;
	struct i2c_msg *p = &msgs[0], *q = &msgs[1];
	u8 slave_id = ((p->addr& 0x7f) << 1);
	u32 offset = (u32)*((u8 *)p->buf);
	u8* buf = (u8 *)q->buf;
	u32 count = q->len;


	if (buf == NULL)
	{
		return -EINVAL;
	}

	if(count > MAX_TRANSACTION_SIZE)
	{
		printk("count > %d is not yet supported.\n", MAX_TRANSACTION_SIZE);
		return -EINVAL;
	}

	if (offset + count > GPON_PHY_EEPROM_SIZE)
	{
		return -EINVAL; //for hexdump
	}

	readTimes = count / MAX_LILAC_I2C_BYTES_NUM;
	remainder = count % MAX_LILAC_I2C_BYTES_NUM;

	for (i=0; i<readTimes; i++)
	{
		i2cStatus = bl_lilac_i2c_read (&i2c_params,
					       slave_id,
					       offset + (i * MAX_LILAC_I2C_BYTES_NUM),
					       I2C_REG_WIDTH_8,
					       buf + (i * MAX_LILAC_I2C_BYTES_NUM),
					       MAX_LILAC_I2C_BYTES_NUM);
		if (i2cStatus != I2C_OK)
		{
			printk("---I---read i2c failed [%d],offset=[%d],count=[%d]\n", i2cStatus, offset + (i * MAX_LILAC_I2C_BYTES_NUM),  MAX_LILAC_I2C_BYTES_NUM);        
			return -EIO;
		}
	}

	if (remainder > 0)
	{
		i2cStatus = bl_lilac_i2c_read (&i2c_params,
					       slave_id,
					       offset + (readTimes * MAX_LILAC_I2C_BYTES_NUM),
					       I2C_REG_WIDTH_8,
					       buf + (readTimes * MAX_LILAC_I2C_BYTES_NUM),
					       remainder);
		if (i2cStatus != I2C_OK)
		{
			printk("---II---read i2c failed [%d],offset=[%d],count=[%d]\n", i2cStatus, offset + (i * MAX_LILAC_I2C_BYTES_NUM),  remainder);
			return -EIO;
		}
	}

//	Laser_Dump_Buf(buf, count, offset);  

	return 0;
}

/****************************************************************************/
/* Write M02010: Writes count number of bytes from buf on to the I2C bus    */
/* Returns:                                                                 */
/*   number of bytes written on success, negative value on failure.         */
/****************************************************************************/
static int Laser_Dev_Write(struct i2c_msg *msgs)
{
	BL_LILAC_I2C_STATUS i2cStatus;
	int i=0, writeTimes=0, remainder = 0;
	struct i2c_msg *p = &msgs[0];
	u8 slave_id = ((p->addr& 0x7f) << 1);
	u8 *buf = (u8 *)p->buf + 1;
	u32 offset = (u32)((u8 *)p->buf)[0];
	u32 count = p->len;


	if (buf == NULL)
	{
		return -EINVAL;
	}

	if(count > MAX_TRANSACTION_SIZE)
	{
		printk("count > %d is not yet supported.\n", MAX_TRANSACTION_SIZE);
		return -EINVAL;
	}

	if (offset + count + 1> GPON_PHY_EEPROM_SIZE)
	{
		printk("write beyond edge.\n");
		return -EINVAL;
	}

	writeTimes = count / MAX_LILAC_I2C_BYTES_NUM;
	remainder = count % MAX_LILAC_I2C_BYTES_NUM;

	for (i=0; i<writeTimes; i++)
	{
		i2cStatus = bl_lilac_i2c_write (&i2c_params,
						slave_id,
						offset + (i * MAX_LILAC_I2C_BYTES_NUM),
						I2C_REG_WIDTH_8,
						buf + (i * MAX_LILAC_I2C_BYTES_NUM),
						MAX_LILAC_I2C_BYTES_NUM);
		if (i2cStatus != I2C_OK)
		{
			printk("---III---write i2c failed [%d],offset=[%d],count=[%d]\n", i2cStatus, offset + (i * MAX_LILAC_I2C_BYTES_NUM),  MAX_LILAC_I2C_BYTES_NUM);     
			return -EIO;
		}
	}

	if (remainder > 0)
	{
		i2cStatus = bl_lilac_i2c_write (&i2c_params,
						slave_id,
						offset + (writeTimes * MAX_LILAC_I2C_BYTES_NUM),
						I2C_REG_WIDTH_8,
						buf + (writeTimes * MAX_LILAC_I2C_BYTES_NUM),
						remainder);
		if (i2cStatus != I2C_OK)
		{
			printk("---IV---write i2c failed [%d], offset=[%d],count=[%d]\n", i2cStatus, offset + (i * MAX_LILAC_I2C_BYTES_NUM),  remainder);
			return -EIO;
		}
	}

	return 0;
}


static int bcm68570_xfer(struct i2c_adapter *adap, 
                        struct i2c_msg *msgs, int num)
{
	struct i2c_msg *p, *q;
	int i, err = 0;

	BCM_LOG_DEBUG(BCM_LOG_ID_I2C, "Entering the function %s \n", __FUNCTION__);
	if (num == 2)
	{
		p = &msgs[0]; q = &msgs[1];
	
		if ((!(p->flags & I2C_M_RD)) && (q->flags == I2C_M_RD))
		{
			err = Laser_Dev_Read(msgs);
			return (err < 0) ? err : 2;
		}
	}

	for (i = 0; !err && i < num; i++)
	{
		p = &msgs[i];
		if (!(p->flags & I2C_M_RD))
			err = Laser_Dev_Write(p);
	}

	return (err < 0) ? err : i;
    
}

static u32 bcm68570_func(struct i2c_adapter *adap)
{
	BCM_LOG_DEBUG(BCM_LOG_ID_I2C, "Entering the function %s \n", __FUNCTION__);
	/*The SMBus commands are emulated on the I2C bus using i2c_xfer function*/
	/*TBD:For correct SMBus Emulation, need to use the NOSTOP b/w given 
	  messges to the i2c_xfer */
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
/*  TBD: The HW supports 10 bit addressing and some protocol magling */
/*	I2C_FUNC_10BIT_ADDR | I2C_FUNC_PROTOCOL_MAGLING; */
}

static const struct i2c_algorithm bcm68570_algo = {
	.master_xfer	= bcm68570_xfer,
	.functionality	= bcm68570_func,
};

static struct i2c_adapter bcm68570_i2c_adapter = {
	.owner		= THIS_MODULE,
	.class		= I2C_CLASS_HWMON,
	.algo		= &bcm68570_algo,
	.name		= "BCM68570 I2C driver",
};


static int __init bcm68570_i2c_init(void)
{
	BL_LILAC_SOC_STATUS socStatus = 0;
	int scl     = -1;
	int sda_out = -1;
	int rate    = 100;
	int sda_in  = -1;
    
	/* Init i2c driver */
	i2c_params.rate = rate;
	i2c_params.scl_pin = scl;
	i2c_params.sda_in_pin = sda_in;
	i2c_params.sda_out_pin = sda_out;

        
	socStatus = bl_lilac_i2c_init(&i2c_params);
       
	if (socStatus != BL_LILAC_SOC_OK )
	{
		printk(KERN_ERR "Error initializing i2c: Error=%d \n", (int)socStatus);
	}

	/* Register the Adapter(Bus & Algo drivers combined in this file) Driver*/
	return i2c_add_adapter(&bcm68570_i2c_adapter);
}

static void __exit bcm68570_i2c_exit(void)
{
	i2c_del_adapter(&bcm68570_i2c_adapter);
}

module_init(bcm68570_i2c_init);
module_exit(bcm68570_i2c_exit);

MODULE_AUTHOR("Pratapa Reddy <pratapas@yahoo.com>");
MODULE_DESCRIPTION("I2C-Bus BCM68570 algorithm");
MODULE_LICENSE("GPL");

