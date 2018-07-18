/*
    gpon_i2c.c - I2C client driver for GPON transceiver
    Copyright (C) 2008 Broadcom Corp.
 
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

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>  /* kzalloc() */
#include <linux/types.h>
#include <linux/bcm_log.h>
#include "gpon_i2c.h"
#include "pmd.h"
#include "6838_map_part.h" /* for GPIO */

#include "opif.h"

#ifdef PROCFS_HOOKS
#include <asm/uaccess.h> /*copy_from_user*/
#include <linux/proc_fs.h>
#define PROC_DIR_NAME "i2c_gpon"
#define PROC_ENTRY_NAME1 "gponPhy_eeprom0"
#define PROC_ENTRY_NAME2 "gponPhy_eeprom1"
#ifdef GPON_I2C_TEST
#define PROC_ENTRY_NAME3 "gponPhyTest"
#endif
#endif


/* I2C client chip addresses */
/* Note that these addresses are 7-bit addresses without the LSB bit
   which indicates R/W operation */
#ifdef CONFIG_BCM_GPON_BOSA
#define GPON_PHY_I2C_ADDR1 0x4E
#define GPON_PHY_I2C_ADDR2 0x4F   
#else
#define GPON_PHY_I2C_ADDR1 0x50
#define GPON_PHY_I2C_ADDR2 0x51
#endif

/* Addresses to scan */
static unsigned short normal_i2c[] = {GPON_PHY_I2C_ADDR1, 
                                      GPON_PHY_I2C_ADDR2, I2C_CLIENT_END};

/* file system */
enum fs_enum {PROC_FS, SYS_FS};

/* Size of client in bytes */
#define DATA_SIZE             256
#define DWORD_ALIGN           4
#define WORD_ALIGN            2

/* Client0 for Address 0x50 and Client1 for Address 0x51 */
#define client0               0
#define client1               1

#ifndef GPON_I2C_IOCTL
#define GPON_I2C_IOCTL 1
#endif

#define GPIO_PIN(x)                     (1 << (x % 32))
#define GPIO_12V_DISABLE                GPIO_PIN(10)

/* Each client has this additional data */
struct gponPhy_data {
    struct i2c_client client;
};

/* Assumption: The i2c modules will be built-in to the kernel and will not be 
   unloaded; otherwise, it is possible for caller modules to call the exported
   functions even when the i2c modules are not loaded unless some registration
   mechanism is built-in to this module.  */
static struct gponPhy_data *pclient1_data; 
static struct gponPhy_data *pclient2_data; 

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 2, 0)
/* Insmod parameters */
I2C_CLIENT_INSMOD_1(gponPhy);

static int gponPhy_attach_adapter(struct i2c_adapter *adapter);
static int gponPhy_detect(struct i2c_adapter *adapter, int address, int kind);
static int gponPhy_detach_client(struct i2c_client *client);

#else /* !LINUX_VERSION_CODE < KERNEL_VERSION(3, 2, 0) */
static int gponPhy_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int gponPhy_remove(struct i2c_client *client);
static int gponPhy_detect(struct i2c_client *, struct i2c_board_info *);
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(3, 2, 0) */

/* Check if given offset is valid or not */
static inline int check_offset(u8 offset, u8 alignment)
{
    if (offset % alignment) {
        BCM_LOG_ERROR(BCM_LOG_ID_I2C, "Invalid offset %d. The offset should be"
                      " %d byte alligned \n", offset, alignment);
        return -1;
    }
    return 0;
}

static void gponPhy_Dump_Buf(u8 * buf, u32 count, u32 offset)
{
	int i = 0; 

	printk("buf[%d..%d]=[", offset, offset+count-1);
  
	for (i=0; i<count; i++) 
		printk("%02x ", buf[i]);

	printk("]\n");
}

static int get_client(u8 client_num, struct i2c_client **client)
{
    switch (client_num)
    {
    case 0:
    	*client = &pclient1_data->client;
    	break;
    case 1:
        *client = &pclient2_data->client;
        break;
    default:
    	BCM_LOG_ERROR(BCM_LOG_ID_I2C, "Invalid client number \n");
        return -1;
    }
    return 0;
}

/****************************************************************************/
/* generic_i2c_access: Provides a way to use BCM6816 algorithm driver to    */
/*  access any I2C device on the bus                                        */
/* Inputs: i2c_addr = 7-bit I2C address; offset = 8-bit offset; length =    */
/*  length (limited to 4); val = value to be written; set = indicates write */
/* Returns: None                                                            */
/****************************************************************************/
static void generic_i2c_access(u8 i2c_addr, u8 offset, u8 length, 
                               int val, u8 set)
{
    struct i2c_msg msg[2];
    char buf[5];

    if (length > 4)
    {
    	BCM_LOG_NOTICE(BCM_LOG_ID_I2C, "Read limited to 4 bytes\n");
    	return;
    }

    /* First write the offset  */
    msg[0].addr = msg[1].addr = i2c_addr;
    msg[0].flags = msg[1].flags = 0;

    /* if set = 1, do i2c write; otheriwse do i2c read */
    if (set) {
        msg[0].len = length + 1;
        buf[0] = offset;
        /* On the I2C bus, LS Byte should go first */
        val = htonl(val);
        memcpy(&buf[1], (char*)&val, length);
        msg[0].buf = buf;
        if(i2c_transfer(pclient1_data->client.adapter, msg, 1) != 1) {
            BCM_LOG_NOTICE(BCM_LOG_ID_I2C, "I2C Write Failed \n");
        }
    } else {
        /* write message */
        msg[0].len = 1;
        buf[0] = offset;
        msg[0].buf = buf;
        /* read message */
        msg[1].flags |= I2C_M_RD;
        msg[1].len = length;
        msg[1].buf = buf;

        /* On I2C bus, we receive LS byte first. So swap bytes as necessary */
        if(i2c_transfer(pclient1_data->client.adapter, msg, 2) == 2)
        {
            gponPhy_Dump_Buf(buf, length, 0);
        } else {
            BCM_LOG_NOTICE(BCM_LOG_ID_I2C, "I2C Read Failed \n");
        }
    }
}

/****************************************************************************/
/* Write gponPhy: Writes count number of bytes from buf on to the I2C bus   */
/* Returns:                                                                 */
/*   number of bytes written on success, negative value on failure.         */
/* Notes: 1. The LS byte should follow the offset                           */
/* Design Notes: The gponPhy takes the first byte after the chip address    */
/*  as offset. The BCM6816 can only send/receive upto 8 or 32 bytes         */
/*  depending on I2C_CTLHI_REG.DATA_REG_SIZE configuration in one           */
/*  transaction without using the I2C_IIC_ENABLE NO_STOP functionality.     */
/*  The 6816 algorithm driver currently splits a given transaction larger   */
/*  than DATA_REG_SIZE into multiple transactions. This function is         */   
/*  expected to be used very rarely and hence a simple approach is          */
/*  taken whereby this function limits the count to 32 (Note that the 6816  */
/*  I2C_CTLHI_REG.DATA_REG_SIZE is hard coded in 6816 algorithm driver for  */
/*  32B. This means, we can only write upto 31 bytes using this function.   */
/****************************************************************************/
ssize_t gponPhy_write(u8 client_num, char *buf, size_t count)
{
    struct i2c_client *client = NULL;
    BCM_LOG_DEBUG(BCM_LOG_ID_I2C, "Entering the function %s \n", __FUNCTION__);

    if(count > MAX_TRANSACTION_SIZE)
    {
        BCM_LOG_NOTICE(BCM_LOG_ID_I2C, "count > %d is not yet supported \n", MAX_TRANSACTION_SIZE);
        return -1;
    }
  
    if (client_num)
        client = &pclient2_data->client;
    else
        client = &pclient1_data->client;

    return i2c_master_send(client, buf, count);
}
EXPORT_SYMBOL(gponPhy_write);

/****************************************************************************/
/* Read BCM3450: Reads count number of bytes from BCM3450                   */
/* Returns:                                                                 */
/*   number of bytes read on success, negative value on failure.            */
/* Notes: 1. The offset should be provided in buf[0]                        */
/*        2. The count is limited to 32.                                    */
/*        3. The gponPhy with the serial EEPROM protocol requires the offset*/
/*        be written before reading the data on every I2C transaction       */
/****************************************************************************/
ssize_t gponPhy_read(u8 client_num, char *buf, size_t count)
{
    struct i2c_msg msg[2];
    struct i2c_client *client = NULL;
    BCM_LOG_DEBUG(BCM_LOG_ID_I2C, "Entering the function %s \n", __FUNCTION__);

    if(count > MAX_TRANSACTION_SIZE)
    {
        BCM_LOG_NOTICE(BCM_LOG_ID_I2C, "count > %d is not yet supported \n", MAX_TRANSACTION_SIZE);
        return -1;
    }

    if(get_client(client_num, &client))
    {
        return -1;
    }

    /* First write the offset  */
    msg[0].addr = msg[1].addr = client->addr;
    msg[0].flags = msg[1].flags = client->flags & I2C_M_TEN;

#if defined(CONFIG_BCM_PMD_MODULE) ||  defined(CONFIG_BCM_PMD)
    msg[0].len = PMD_I2C_HEADER;
#else
    msg[0].len = 1;
#endif
    msg[0].buf = buf;

    /* Now read the data */
    msg[1].flags |= I2C_M_RD;
    msg[1].len = count;
    msg[1].buf = buf;

    /* On I2C bus, we receive LS byte first. So swap bytes as necessary */
    if(i2c_transfer(client->adapter, msg, 2) == 2)
    {
        return count;
    }

    return -1;
}
EXPORT_SYMBOL(gponPhy_read);

/****************************************************************************/
/* Write Register: Writes the val into gponPhy register                     */
/* Returns:                                                                 */
/*   0 on success, negative value on failure.                               */
/* Notes: 1. The offset should be DWORD aligned                             */
/****************************************************************************/
int gponPhy_write_reg(u8 client_num, u8 offset, int val)
{
    char buf[5];
    struct i2c_client *client = NULL;

    BCM_LOG_DEBUG(BCM_LOG_ID_I2C, "Entering the function %s \n", __FUNCTION__);

    if(check_offset(offset, DWORD_ALIGN))
    {
        return -1;
    }

    if(get_client(client_num, &client))
    {
        return -1;
    }

    /* Set the buf[0] to be the offset for write operation */
    buf[0] = offset;

    /* On the I2C bus, LS Byte should go first */
    val = htonl(val);

    memcpy(&buf[1], (char*)&val, 4);
    if (i2c_master_send(client, buf, 5) == 5)
    {
        return 0;
    }
    return -1;
}
EXPORT_SYMBOL(gponPhy_write_reg);

/****************************************************************************/
/* Read Register: Read the gponPhy register                                 */
/* Returns:                                                                 */
/*   value on success, negative value on failure.                           */
/* Notes: 1. The offset should be DWORD aligned                             */
/****************************************************************************/
int gponPhy_read_reg(u8 client_num, u8 offset)
{
    struct i2c_msg msg[2];
    int val;
    struct i2c_client *client = NULL;

    BCM_LOG_DEBUG(BCM_LOG_ID_I2C, "Entering the function %s \n", __FUNCTION__);

    if(check_offset(offset, DWORD_ALIGN))
    {
        return -1;
    }

    if(get_client(client_num, &client))
    {
            return -1;
    }

    msg[0].addr = msg[1].addr = client->addr;
    msg[0].flags = msg[1].flags = client->flags & I2C_M_TEN;

    msg[0].len = 1;
    msg[0].buf = (char *)&offset;

    msg[1].flags |= I2C_M_RD;
    msg[1].len = 4;
    msg[1].buf = (char *)&val;

    /* On I2C bus, we receive LS byte first. So swap bytes as necessary */
    if(i2c_transfer(client->adapter, msg, 2) == 2)
    {
        return ntohl(val);
    }

    return -1;
}
EXPORT_SYMBOL(gponPhy_read_reg);

/****************************************************************************/
/* Write Word: Writes the val into the word offset                          */ 
/* Returns:                                                                 */
/*   0 on success, negative value on failure.                               */
/* Notes: 1. The offset should be WORD aligned                              */
/****************************************************************************/
int gponPhy_write_word(u8 client_num, u8 offset, u16 val)
{
    char buf[3];
    struct i2c_client *client = NULL;

    BCM_LOG_DEBUG(BCM_LOG_ID_I2C, "Entering the function %s \n", __FUNCTION__);

    if(check_offset(offset, WORD_ALIGN))
    {
        return -1;
    }

    if(get_client(client_num, &client))
    {
            return -1;
    }

    /* The offset to be written should be the first byte in the I2C write */
    buf[0] = offset;
    buf[1] = (char)(val&0xFF);
    buf[2] = (char)(val>>8);
    if (i2c_master_send(client, buf, 3) == 3)
    {
        return 0;
    }
    return -1;
}              
EXPORT_SYMBOL(gponPhy_write_word);

/****************************************************************************/
/* Read Word: Reads the LSB 2 bytes of Register                             */ 
/* Returns:                                                                 */
/*   value on success, negative value on failure.                           */
/* Notes: 1. The offset should be WORD aligned                              */
/****************************************************************************/
u16 gponPhy_read_word(u8 client_num, u8 offset)
{
    struct i2c_msg msg[2];
    u16 val;
    struct i2c_client *client = NULL;

    BCM_LOG_DEBUG(BCM_LOG_ID_I2C, "Entering the function %s \n", __FUNCTION__);

    if(check_offset(offset, WORD_ALIGN))
    {
        return -1;
    }

    if(get_client(client_num, &client))
    {
        return -1;
    }

    msg[0].addr = msg[1].addr = client->addr;
    msg[0].flags = msg[1].flags = client->flags & I2C_M_TEN;

    msg[0].len = 1;
    msg[0].buf = (char *)&offset;

    msg[1].flags |= I2C_M_RD;
    msg[1].len = 2;
    msg[1].buf = (char *)&val;

    if(i2c_transfer(client->adapter, msg, 2) == 2)
    {
        return ntohs(val);
    }

    return -1;
}
EXPORT_SYMBOL(gponPhy_read_word);

/****************************************************************************/
/* Write Byte: Writes the val into LS Byte of Register                      */ 
/* Returns:                                                                 */
/*   0 on success, negative value on failure.                               */
/****************************************************************************/
int gponPhy_write_byte(u8 client_num, u8 offset, u8 val)
{
    char buf[2];
    struct i2c_client *client = NULL;

    BCM_LOG_DEBUG(BCM_LOG_ID_I2C, "Entering the function %s \n", __FUNCTION__);

    if(get_client(client_num, &client))
    {
        return -1;
    }

    /* BCM3450 requires the offset to be the register number */
    buf[0] = offset;
    buf[1] = val;
    if (i2c_master_send(client, buf, 2) == 2)
    {
        return 0;
    }
    return -1;
}
EXPORT_SYMBOL(gponPhy_write_byte);

/****************************************************************************/
/* Read Byte: Reads the LS Byte of Register                                 */ 
/* Returns:                                                                 */
/*   value on success, negative value on failure.                           */
/****************************************************************************/
u8 gponPhy_read_byte(u8 client_num, u8 offset)
{
    struct i2c_msg msg[2];
    char val;
    struct i2c_client *client = NULL;

    BCM_LOG_DEBUG(BCM_LOG_ID_I2C, "Entering the function %s \n", __FUNCTION__);

    if(get_client(client_num, &client))
    {
        return -1;
    }

    msg[0].addr = msg[1].addr = client->addr;
    msg[0].flags = msg[1].flags = client->flags & I2C_M_TEN;

    msg[0].len = 1;
    msg[0].buf = (char *)&offset;

    msg[1].flags |= I2C_M_RD;
    msg[1].len = 1;
    msg[1].buf = (char *)&val;

    if(i2c_transfer(client->adapter, msg, 2) == 2)
    {
        return val;
    }

    return -1;
}
EXPORT_SYMBOL(gponPhy_read_byte);

#if defined(SYSFS_HOOKS) || defined(PROCFS_HOOKS)
#ifdef GPON_I2C_TEST
static int client_num = 0;
/* Calls the appropriate function based on user command */
static int exec_command(const char *buf, size_t count, int fs_type)
{
#define MAX_ARGS 4
#define MAX_ARG_SIZE 32
    int i, argc = 0, val = 0;
    char cmd;
    u8 offset, i2c_addr, length, set = 0;
    char arg[MAX_ARGS][MAX_ARG_SIZE];
#if 0
    char temp_buf[20] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
#endif
#ifdef PROCFS_HOOKS
#define LOG_WR_KBUF_SIZE 128
    char kbuf[LOG_WR_KBUF_SIZE];

    if(fs_type == PROC_FS)
    {
        if ((count > LOG_WR_KBUF_SIZE-1) || 
            (copy_from_user(kbuf, buf, count) != 0))
            return -EFAULT;
        kbuf[count]=0;
        argc = sscanf(kbuf, "%c %s %s %s %s", &cmd, arg[0], arg[1], 
                      arg[2], arg[3]);
    }
#endif

#ifdef SYSFS_HOOKS
    if(fs_type == SYS_FS)
        argc = sscanf(buf, "%c %s %s %s %s", &cmd, arg[0], arg[1], 
                      arg[2], arg[3]);
#endif

    if (argc <= 1) {
        BCM_LOG_NOTICE(BCM_LOG_ID_I2C, "Need at-least 2 arguments \n");
        return -EFAULT;
    }

    for (i=0; i<MAX_ARGS; ++i) {
        arg[i][MAX_ARG_SIZE-1] = '\0';
    }

    offset = (u8) simple_strtoul(arg[0], NULL, 0);
    if (argc == 3)
        val = (int) simple_strtoul(arg[1], NULL, 0);

    switch (cmd) {
 
       case 'a':
        if (argc >= 4) {
            i2c_addr = (u8) simple_strtoul(arg[0], NULL, 0);
            offset = (u8) simple_strtoul(arg[1], NULL, 0);
            length = (u8) simple_strtoul(arg[2], NULL, 0);
            BCM_LOG_DEBUG(BCM_LOG_ID_I2C, "I2C Access: i2c_addr = 0x%x, offset"
                          " = 0x%x, len = %d \n", i2c_addr, offset, length);
            if (i2c_addr > 127 || length > 4) {
                BCM_LOG_NOTICE(BCM_LOG_ID_I2C, "Invalid I2C addr or len \n");
                return -EFAULT;
            }
            val = 0;
            if (argc > 4) {
                val = (int) simple_strtoul(arg[3], NULL, 0);
                set = 1;
            }
            generic_i2c_access(i2c_addr, offset, length, val, set);
        } else {
            BCM_LOG_NOTICE(BCM_LOG_ID_I2C, "Need at-least 3 arguments \n");
            return -EFAULT;
        }
        break;
    
#if 0
    case 'y':
        if (argc == 3) {
            if (val > 16) {
                BCM_LOG_INFO(BCM_LOG_ID_I2C, "Limiting byte count to 16 \n");
                val = 16;
            }
            BCM_LOG_DEBUG(BCM_LOG_ID_I2C, "Write Byte Stream: offset = 0x%x, " 
                          "count = 0x%x \n", offset, val);
            for (i=0; i< val; i++) {
                BCM_LOG_DEBUG(BCM_LOG_ID_I2C, "0x%x, ",temp_buf[i]);
            }
            temp_buf[0] = offset;
            gponPhy_write(client_num, temp_buf, val+1);
        }
        break;

    case 'z':
        if (argc == 3) {
            if (val > 16) {
                BCM_LOG_INFO(BCM_LOG_ID_I2C, "This test limits the byte"
                             "stream count to 16 \n");
                val = 16;
            }
            temp_buf[0] = offset;
            gponPhy_read(client_num, temp_buf, val);
            BCM_LOG_DEBUG(BCM_LOG_ID_I2C, "Read Byte Stream: offset =0x%x, " 
                          "count = 0x%x \n", offset, val);
            for (i=0; i< val; i++) {
                BCM_LOG_DEBUG(BCM_LOG_ID_I2C, "0x%x (%c)",(u8)temp_buf[i], 
                              (u8) temp_buf[i]);
            }
            BCM_LOG_DEBUG(BCM_LOG_ID_I2C, "\n");
        }
        break;
#endif

    case 'b':
        if (argc == 3) {
            BCM_LOG_DEBUG(BCM_LOG_ID_I2C, "Write Byte: offset = 0x%x, " 
                          "val = 0x%x \n", offset, val);
            if (gponPhy_write_byte(client_num, offset, (u8)val) < 0) {
                BCM_LOG_NOTICE(BCM_LOG_ID_I2C, "Write Failed \n"); 
            } else {
                BCM_LOG_NOTICE(BCM_LOG_ID_I2C, "Write Successful \n"); 
            }
        }
        else {
            if((val = gponPhy_read_byte(client_num, offset)) < 0) {
                BCM_LOG_NOTICE(BCM_LOG_ID_I2C, "Read Failed \n"); 
            } else {
                 BCM_LOG_NOTICE(BCM_LOG_ID_I2C, "Read Byte: offset = 0x%x, " 
                                "val = 0x%x \n", offset, val);
            }
        }
        break;

    case 'w':
        if (argc == 3) {
            BCM_LOG_DEBUG(BCM_LOG_ID_I2C, "Write Word: offset = 0x%x, " 
                              "val = 0x%x \n", offset, val);
            if (gponPhy_write_word(client_num, offset, (u16)val) < 0) {
                BCM_LOG_NOTICE(BCM_LOG_ID_I2C, "Write Failed \n"); 
            } else {
                BCM_LOG_NOTICE(BCM_LOG_ID_I2C, "Write Successful \n"); 
            }
        }
        else {
            if((val = gponPhy_read_word(client_num, offset)) < 0) {
                BCM_LOG_NOTICE(BCM_LOG_ID_I2C, "Read Failed \n"); 
            } else {
                BCM_LOG_NOTICE(BCM_LOG_ID_I2C, "Read Word: offset = 0x%x, " 
                               "val = 0x%x \n", offset, val);
            }
        }
        break;

    case 'd':    
        if (argc == 3) {
            BCM_LOG_DEBUG(BCM_LOG_ID_I2C, "Write Register: offset = 0x%x, " 
                          "val = 0x%x \n", offset, val);
            if (gponPhy_write_reg(client_num, offset, val) < 0) {
                BCM_LOG_NOTICE(BCM_LOG_ID_I2C, "Write Failed \n"); 
            } else {
                BCM_LOG_NOTICE(BCM_LOG_ID_I2C, "Write Successful \n"); 
            }
        }
        else {
            if((val = gponPhy_read_reg(client_num, offset)) < 0) {
                BCM_LOG_NOTICE(BCM_LOG_ID_I2C, "Read Failed \n"); 
            } else {
                BCM_LOG_NOTICE(BCM_LOG_ID_I2C, "Read Register: offset = 0x%x,"
                               " val = 0x%x \n", offset, val);
            }
        }
        break;

    case 'c':    
        if (offset == GPON_PHY_I2C_ADDR1)
            client_num = 0;
        else
            client_num = 1;
        break;

    default:
        BCM_LOG_NOTICE(BCM_LOG_ID_I2C, "Invalid command. \n Valid commands: \n" 
                       "  Change I2C Addr b/w 0x50 and 0x51: c addr \n" 
                       "  Write Reg:       d offset val \n" 
                       "  Read Reg:        d offset \n" 
                       "  Write Word:      w offset val \n" 
                       "  Read Word:       w offset \n" 
                       "  Write Byte:      b offset val \n" 
                       "  Read Byte:       b offset \n" 
                       "  Generic I2C access: a <i2c_addr(7-bit)>" 
                       " <offset> <length(1-4)> [value] \n" 
#if 0
                       "  Write Bytes:     y offset count \n" 
                       "  Read Bytes:      z offset count \n"
#endif
                       );
        break;
    }
    return count;
}
#endif
#endif

#ifdef PROCFS_HOOKS
#ifdef GPON_I2C_TEST
/* Read Function of PROCFS attribute "gponPhyTest" */
static ssize_t gponPhy_proc_test_read(struct file *f, char *buf, size_t count, 
                               loff_t *pos) 
{
    BCM_LOG_NOTICE(BCM_LOG_ID_I2C, " Usage: echo command > "
                   " /proc/i2c-gpon/gponPhyTest \n");
    BCM_LOG_NOTICE(BCM_LOG_ID_I2C, " supported commands: \n" 
                   "  Change I2C Addr b/w 0x50 and 0x51: c addr \n" 
                   "  Write Reg:       d offset val \n" 
                   "  Read Reg:        d offset \n" 
                   "  Write Word:      w offset val \n" 
                   "  Read Word:       w offset \n" 
                   "  Write Byte:      b offset val \n" 
                   "  Read Byte:       b offset \n" 
                   "  Generic I2C access: a <i2c_addr(7-bit)>" 
                   " <offset> <length(1-4)> [value] \n" 
                   );
    return 0;
}

/* Write Function of PROCFS attribute "gponPhyTest" */
static ssize_t gponPhy_proc_test_write(struct file *f, const char *buf, 
                                       size_t count, loff_t *pos)
{
    return exec_command(buf, count, PROC_FS);
}
#endif

#define GPON_PHY_EEPROM_SIZE  256

#ifndef GPON_I2C_IOCTL      //if p->proc_fops != NULL, read_proc & write_proc will not be called     
/* Read Function of PROCFS attribute "gponPhy_eepromX" */
static ssize_t gponPhy_proc_read(char *page, char **start, off_t off, int count,
                               int *eof, void *data) 
{
    int client_num = 0, max_offset, ret_val;
    struct gponPhy_data *pclient_data; 

    BCM_LOG_DEBUG(BCM_LOG_ID_I2C, "The offset is %d; the count is %d \n", 
                  (int)off, count);

    /* Verify that max_offset is below the max_eeprom_size (256 Bytes)*/
    max_offset = (int) (off + count);
    if (max_offset > GPON_PHY_EEPROM_SIZE) {
        BCM_LOG_NOTICE(BCM_LOG_ID_I2C, "offset + count must be less than "
                       "Max EEPROM Size of 256\n");
        return -1;
    }
 
    /* Set the page[0] to eeprom offset */
    page[0] = (u8)off;

    /* Select the eeprom of the 2 eeproms inside gponPhy */
    pclient_data = (struct gponPhy_data *)data;
    if (pclient_data->client.addr == GPON_PHY_I2C_ADDR2) {
        client_num = 1;
    }

    /*   See comments in the proc_file_read for info on 3 different
    *    ways of returning data. We are following below method.
    *    Set *start = an address within the buffer.
    *    Put the data of the requested offset at *start.
    *    Return the number of bytes of data placed there.
    *    If this number is greater than zero and you
    *    didn't signal eof and the reader is prepared to
    *    take more data you will be called again with the
    *    requested offset advanced by the number of bytes
    *    absorbed. */
    ret_val = gponPhy_read(client_num, page, count);
    *start = page;
    *eof = 1;

    return ret_val;
}

/* Write Function of PROCFS attribute "gponPhy_eepromX" */
static ssize_t gponPhy_proc_write(struct file *file, const char __user *buffer, 
                                  unsigned long count, void *data) 
{
    int client_num = 0, max_offset, offset = (int)file->f_pos;
    struct gponPhy_data *pclient_data; 
    char *kbuf;
    int rc;

    BCM_LOG_DEBUG(BCM_LOG_ID_I2C, "The offset is %d; the count is %ld \n", 
                  offset, count);

    /* Verify that count is less than 31 bytes */
    if ((count+1) > MAX_TRANSACTION_SIZE)
    {
        BCM_LOG_NOTICE(BCM_LOG_ID_I2C, "Writing more than 31 Bytes is not"
                       "yet supported \n");
        return -1;
    }

    kbuf = kzalloc(count, GFP_KERNEL);
    if (!kbuf)
    {
        BCM_LOG_DEBUG(BCM_LOG_ID_I2C, "Couldn't allocated kbuf \n");
        return -1;
    }
 
    /* Verify that max_offset is below the max_eeprom_size (256 Bytes)*/
    max_offset = (int) (offset + count);
    if (max_offset > GPON_PHY_EEPROM_SIZE)
    {
        BCM_LOG_NOTICE(BCM_LOG_ID_I2C, "offset + count must be less than "
                       "Max EEPROM Size of 256\n");
        rc = -1;
        goto exit;
    }  
   
    /* Select the eeprom of the 2 eeproms inside gponPhy */
    pclient_data = (struct gponPhy_data *)data;
    if (pclient_data->client.addr == GPON_PHY_I2C_ADDR2)
        client_num = 1;

    kbuf[0] = (u8)offset;
    copy_from_user(&kbuf[1], buffer, count);
    /* Return the number of bytes written (exclude the address byte added
       at kbuf[0] */
    rc = gponPhy_write(client_num, kbuf, count+1) - 1;
exit:
    kfree(kbuf);
    return rc;
}
#endif

#endif

#ifdef SYSFS_HOOKS
/* Read Function of SYSFS attribute */
static ssize_t gponPhy_sys_read(struct device *dev, struct device_attribute *attr, 
                          char *buf)
{
    return snprintf(buf, PAGE_SIZE, "The gponPhy access read attribute \n");
}

/* Write Function of SYSFS attribute */
static ssize_t gponPhy_sys_write(struct device *dev, struct device_attribute *attr, 
                           const char *buf, size_t count)
{
    return exec_command(buf, count, SYS_FS);
}

static DEVICE_ATTR(gponPhy_access, S_IRWXUGO, gponPhy_sys_read, gponPhy_sys_write);

static struct attribute *gponPhy_attributes[] = {
    &dev_attr_gponPhy_access.attr,
    NULL
};

static const struct attribute_group gponPhy_attr_group = {
    .attrs = gponPhy_attributes,
};
#endif

#ifdef PROCFS_HOOKS
#ifdef GPON_I2C_TEST
static struct file_operations gponPhyTest_fops = {
    read: gponPhy_proc_test_read,
    write: gponPhy_proc_test_write
};
#endif
#endif

#ifdef GPON_I2C_IOCTL
static int gponPhy_write_check_boundary(u8 offset, u8 count)
{
	int max_offset;
    
	if(IS_BITFIELD_SIZE(count)) 
		return 0;
    
	/* Verify that count is less than 31 bytes */
	if ((count+1) > MAX_TRANSACTION_SIZE)
	{
		BCM_LOG_NOTICE(BCM_LOG_ID_I2C, "Writing more than 31 Bytes is not"
			       "yet supported \n");
		return -1;
	}
 
	/* Verify that max_offset is below the max_eeprom_size (256 Bytes)*/
	max_offset = (int) (offset + count);
	if (max_offset > GPON_PHY_EEPROM_SIZE)
	{
		BCM_LOG_NOTICE(BCM_LOG_ID_I2C, "offset + count must be less than "
			       "Max EEPROM Size of 256\n");
		return -1;
	} 
	
	return 0;
}

static int gponPhy_write_n_bytes(u8 client_num, u8 offset, u8 count, u8 *buf)
{
	int ret = 0;
    
	if(IS_BITFIELD_SIZE(count))  /* write a bit */
	{
		u8 c;
    
		c = gponPhy_read_byte(client_num, offset);
      
		if(0 == buf[0])
			c &= ~(1<<(count&0x7f));
		else
			c |= 1<<(count&0x7f);

		ret = gponPhy_write_byte(client_num, offset, c);
	}
	else   /* write n bytes, 1<=n<=31 */
	{
		char kbuf[MAX_TRANSACTION_SIZE]; 

		kbuf[0] = offset;
		memcpy(&kbuf[1], buf, count);      
		ret =  gponPhy_write(client_num, kbuf, count+1) - 1;        
	}      

	return ret;
}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33)
static long gpon_i2c_ioctl(int client_num, unsigned int cmd, unsigned long argument) 
#else
	static int gpon_i2c_ioctl(int client_num, unsigned int cmd, unsigned long argument) 
#endif
{
	int ret = 0;
	u8  len = 0, offset = 0, real_len = 0;
	transceiver_ioctl_st  arg;    

	if (cmd >= GPON_I2C_IOC_A2_NUM) {
		BCM_LOG_DEBUG(BCM_LOG_ID_I2C, "Now we only support SET and GET, don't support [%d], return!\n", cmd);
		return -EINVAL;
	}

	if (copy_from_user(&arg, (const void*)argument, sizeof(transceiver_ioctl_st)))
		return -EINVAL;


	real_len = (IS_BITFIELD_SIZE(arg.len)) ? 1 : arg.len;
	if (arg.offset + real_len >= GPON_PHY_EEPROM_SIZE) {
		return -EINVAL;
	}

	if (cmd == GPON_I2C_IOC_SET || cmd == GPON_I2C_IOC_GET)
	{
		len = arg.len;
		offset = arg.offset;
	}
        
	switch(cmd)
	{
	case GPON_I2C_IOC_SET:
	{
		if (gponPhy_write_check_boundary(offset, len) < 0)
			return -EFAULT;        
        
		ret = gponPhy_write_n_bytes(client_num, offset, len, arg.buf);
		break;
	}

	case GPON_I2C_IOC_GET:      
	{
		if(IS_BITFIELD_SIZE(len))  /* read a bit */
		{
			u8 c;    
			c = gponPhy_read_byte(client_num, offset);      
			arg.buf[0] = (c & ( 1<<(len&0x7f) )) ? 1 : 0;
			ret = 1;  //read 1 byte
		}
		else   /* read n bytes, 1<=n<=31 */
		{
			arg.buf[0] = offset;
			if ( (ret =  gponPhy_read(client_num, arg.buf, len)) < 0)
				return ret;
            
			//ret = len;  //read len bytes
		}      
		if (copy_to_user((void*)argument, &arg, sizeof(transceiver_ioctl_st)))
			return -EFAULT;
		break;
	}

	case GPON_I2C_IOC_SET_12V_VIDEO_POWER:
	{
		if (arg.buf[0])
		{
			//uncomment below 2 lines after PEM3 ready
			//GPIO->GPIODir_low |= GPIO_12V_DISABLE;
			//GPIO->GPIOData_low |= GPIO_12V_DISABLE; // set the voltage of pin17 to 12V, to enable the power supply of RF-O
			printk("[gpon_i2c] Enable 12V video power supply.\n");
		}
		else
		{
			//uncomment below 2 lines after PEM3 ready
			//GPIO->GPIODir_low |= GPIO_12V_DISABLE;
			//GPIO->GPIOData_low&= ~GPIO_12V_DISABLE; // set the voltage of pin17 to 0V, to disable the power supply of RF-O
			printk("[gpon_i2c] Disable 12V video power supply.\n");
		}
		break;
	}

	default:
		printk("KERN_EMERG invalid command, %d\n", cmd);
	};

	return ret;
}

#endif

/*   How to use this IOCTL in user space? 
 *   1.Set a field
 *      CMD: GPON_I2C_IOC_SET
 *      example: 
 *              transceiver_ioctl_st  arg;	
 *              memset(&arg, 0, sizeof(arg)); 			 
 *              //set field_id and fill out buf 
 *              arg.field_id = index;
 *              memcpy(arg.buf, pMsg->value, pCmd->nDataLen); 
 *              ioctl(i2c_socket, GPON_I2C_IOC_SET, (u32)&arg);
 *      Return value:
 *		number of bytes written on success, negative value on failure. 
 *		-EINVAL	: Invalid argument address
 *		-EACCES	: Permission denied 
 *		-EFAULT	: boundary check failure
 *		-1	       : I2C write failure  
 *
 * 
 *   2.Get a field
 *      CMD: GPON_I2C_IOC_GET
 *      example: 
 *              transceiver_ioctl_st  arg;	
 *              memset(&arg, 0, sizeof(arg)); 			 
 *              //set field_id
 *              arg.field_id = index;
 *              ioctl(i2c_socket, GPON_I2C_IOC_GET, (u32)&arg);
 *      Return value: 
 *              number of bytes read on success, negative value on failure. 
 *              on success, the bytes read are stored in arg.buf[]   
 *		-EINVAL	: Invalid argument address
 *		-EFAULT	: copy_to_user() failed
 *		-1	: I2C read failure   
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33)
static long gpon_i2c_ioctl_0(struct file *filp, unsigned int cmd, unsigned long argument) 
#else
	static int gpon_i2c_ioctl_0(struct inode *ip, struct file *filp, unsigned int cmd, unsigned long argument) 
#endif
{
	return gpon_i2c_ioctl(OP_BANK_A0, cmd, argument);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33)
static long gpon_i2c_ioctl_1(struct file *filp, unsigned int cmd, unsigned long argument) 
#else
	static int gpon_i2c_ioctl_1(struct inode *ip, struct file *filp, unsigned int cmd, unsigned long argument) 
#endif
{
	return gpon_i2c_ioctl(OP_BANK_A2, cmd, argument);
}

static ssize_t gpon_fops_read(int client_id, struct file *file, char *buf,
			      size_t count, loff_t *pos)
{
	int max_offset, ret_val;
	int off = *pos;
	char kbuf[MAX_TRANSACTION_SIZE]={0};

	BCM_LOG_DEBUG(BCM_LOG_ID_I2C, "The offset is 0x%x; the count is %d \n", (int)off, count);

	/* Verify that max_offset is below the max_eeprom_size (256 Bytes)*/
	max_offset = (int) (off + count);
	if (max_offset > GPON_PHY_EEPROM_SIZE) {
		BCM_LOG_NOTICE(BCM_LOG_ID_I2C, "offset + count must be less than "
			       "Max EEPROM Size of 256\n");
		return -1;
	}
 
	/* Set the kbuf[0] to eeprom offset */
	kbuf[0] = (u8)off;

	ret_val = gponPhy_read(client_id, kbuf, count);      

	if (ret_val > 0)
		copy_to_user(buf, kbuf, ret_val);

	*pos += ret_val;

	return ret_val;
}

static ssize_t gpon_fops0_read(struct file *file, char *buf,
			       size_t count, loff_t *pos)
{
	return gpon_fops_read(client0, file, buf, count, pos);
}

static ssize_t gpon_fops1_read(struct file *file, char *buf,
			       size_t count, loff_t *pos)
{
	return gpon_fops_read(client1, file, buf, count, pos);
}

static ssize_t gpon_fops_write(int client_id, struct file * file, const char * buf,
			       size_t count, loff_t *pos)
{
	int max_offset, offset = *pos;
	char kbuf[MAX_TRANSACTION_SIZE]={0};

	BCM_LOG_DEBUG(BCM_LOG_ID_I2C, "The offset is %d; the count is %d \n", 
		      offset, count);

	/* Verify that count is less than 31 bytes */
	if ((count+1) > MAX_TRANSACTION_SIZE)
	{
		BCM_LOG_NOTICE(BCM_LOG_ID_I2C, "Writing more than 31 Bytes is not"
			       "yet supported \n");
		return -1;
	}
 
	/* Verify that max_offset is below the max_eeprom_size (256 Bytes)*/
	max_offset = (int) (offset + count);
	if (max_offset > GPON_PHY_EEPROM_SIZE)
	{
		BCM_LOG_NOTICE(BCM_LOG_ID_I2C, "offset + count must be less than "
			       "Max EEPROM Size of 256\n");
		return -1;
	}  

	kbuf[0] = (u8)offset;
	copy_from_user(&kbuf[1], buf, count);
	/* Return the number of bytes written (exclude the address byte added
	   at kbuf[0] */    
	return (gponPhy_write(client_id, kbuf, count+1) - 1);
}

static ssize_t gpon_fops0_write(struct file *file, const char *buf,
				size_t count, loff_t *pos)
{
	return gpon_fops_write(client0, file, buf, count, pos);
}

static ssize_t gpon_fops1_write(struct file *file, const char *buf,
				size_t count, loff_t *pos)
{
	return gpon_fops_write(client1, file, buf, count, pos);
}

static struct file_operations gpon_fops0 = {
read : gpon_fops0_read,
write: gpon_fops0_write,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33)
unlocked_ioctl: gpon_i2c_ioctl_0,
#else
ioctl: gpon_i2c_ioctl_0,
#endif    
};
static struct file_operations gpon_fops1 = {
read : gpon_fops1_read,
write: gpon_fops1_write,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33)
unlocked_ioctl: gpon_i2c_ioctl_1,
#else
ioctl: gpon_i2c_ioctl_1,
#endif
};

#if 0
/* set the control bit of A2h 6E/6F */
static int set_trsv_control_bit(u8 index, char value)
{
	u8 len = 0, offset = 0, c;

	len = op_data_field[g_gpon_transceiver_type][index].size;
	offset = op_data_field[g_gpon_transceiver_type][index].start;

	c = gponPhy_read_byte(client1, offset);

	if(0 == value)
		c &= ~(1<<(len&0x7f));
	else
		c |= 1<<(len&0x7f);

	return gponPhy_write_byte(client1, offset, c);
}

/* 1=RX enabled, 0=RX disabled */
static inline int set_rxOutputEnable(char value)
{
	return set_trsv_control_bit(OP_A2_RX_OUT_EN, value);
}

/* 0=TX enabled, 1=TX disabled */
static inline int set_txDisable(char value)
{
	return set_trsv_control_bit(OP_A2_TX_DIS, value);
}

/* 1=Video Enabled, 0=Video disabled */
static inline int set_videoEnable(char value)
{
	return set_trsv_control_bit(OP_A2_V_EN, value);
}

/* 1=Squelch Enabled, 0=Squelch disabled */
static inline int set_rfSquelchEnable(char value)
{
	return set_trsv_control_bit(OP_A2_RF_SQUELCH_EN, value);
}

static inline int set_trsv_rf_offset(char value)
{
	u8 index = OP_A2_RF_OFFSET, len = 0, offset = 0;

	len = op_data_field[g_gpon_transceiver_type][index].size;
	offset = op_data_field[g_gpon_transceiver_type][index].start;

	return gponPhy_write_byte(client1, offset, (u8)value);
}
#endif

typedef struct 
{
	short temper_hi;
	short temper_lo;
	short rxRssiAlrm_hi;
	short rxRssiAlrm_lo;
	short rxRssiWarn_hi;
	short rxRssiWarn_lo;
	char  V_EN;
	char  RF_Squelch;
	char  RF_Offset;
}transceiver_defalut_value;


void bcm_load_trsv_dflt_config(void)
{
#if 0
	transceiver_defalut_value dflt;

	/* Disable RF-Overlay and GPON Tx before setting transceiver default value */
	printk("trsv[1]:Disable RF-Overlay and GPON Tx before setting transceiver default value.\n");
	set_txDisable(1);   /* 0=TX enabled, 1=TX disabled */
	set_videoEnable(0); /* 1=Video Enabled, 0=Video disabled */
  
	dflt.V_EN = 0;  //Video disabled
	dflt.RF_Squelch = 1; //RF Squelch Enabled
	dflt.RF_Offset = 0;

	printk("trsv[2]:Load config from trsv.dflt:V_EN[%d]RF_Squelch[%d]RF_Offset[%d]\n", dflt.V_EN, dflt.RF_Squelch, dflt.RF_Offset);
	if (dflt.V_EN != 0)
		set_videoEnable(dflt.V_EN);

	set_rfSquelchEnable(dflt.RF_Squelch);  /* 1: if 1550nm to low, enable squelch, i.e., block video */
	set_trsv_rf_offset(dflt.RF_Offset);

	/* Restore RF-Overlay and GPON Tx after setting finished */
	printk("trsv[3]:Restore GPON Tx after setting finished.\n");
	set_txDisable(0);   /* 0=TX enabled, 1=TX disabled */

	return;
#endif  
}


#ifdef PROCFS_HOOKS
static struct proc_dir_entry *q=NULL;
#endif




#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 2, 0)
/* This is the driver that will be inserted */
static struct i2c_driver gponPhy_driver = {
    .driver = {
        .name    = "gpon_i2c",
    },
    .attach_adapter   = gponPhy_attach_adapter,
    .detach_client    = gponPhy_detach_client,
};

static int gponPhy_attach_adapter(struct i2c_adapter *adapter)
{
    BCM_LOG_DEBUG(BCM_LOG_ID_I2C, "Entering the function %s \n", __FUNCTION__);
    return i2c_probe(adapter, &addr_data, gponPhy_detect);
}

/* This function is called by i2c_probe for each I2C address*/
static int gponPhy_detect(struct i2c_adapter *adapter, int address, int kind)
{
    struct i2c_client *client;
    struct gponPhy_data *pclient_data; 
    int err = 0;
#ifdef PROCFS_HOOKS
    struct proc_dir_entry *p;
#endif

    BCM_LOG_DEBUG(BCM_LOG_ID_I2C, "Entering the function %s \n", __FUNCTION__);

    if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
        goto exit;

    if (!(pclient_data = kzalloc(sizeof(struct gponPhy_data), GFP_KERNEL))) 
    {
        err = -ENOMEM;
        goto exit;
    }

    /* Setup the i2c client data */
    client = &pclient_data->client;
    i2c_set_clientdata(client, pclient_data);
    client->addr = address;
    client->adapter = adapter;
    client->driver = &gponPhy_driver;
    client->flags = 0;

    /* Tell the I2C layer a new client has arrived */
    if ((err = i2c_attach_client(client)))
        goto exit_kfree;

    if (address == GPON_PHY_I2C_ADDR1)
    {
        pclient1_data = pclient_data;
    }
    else
    {
        pclient2_data = pclient_data;
    }

#ifdef SYSFS_HOOKS
    /* Register sysfs hooks */
    err = sysfs_create_group(&client->dev.kobj, &gponPhy_attr_group);
    if (err)
        goto exit_detach;
#endif

#ifdef PROCFS_HOOKS
    if (address == GPON_PHY_I2C_ADDR1)
    {
        q = proc_mkdir(PROC_DIR_NAME, NULL);
        if (!q) {
            BCM_LOG_ERROR(BCM_LOG_ID_I2C, "bcmlog: unable to create proc entry\n");
            err = -ENOMEM;
#ifdef SYSFS_HOOKS
            sysfs_remove_group(&client->dev.kobj, &gponPhy_attr_group);
#endif
            goto exit_detach;
        }
    }

    if (address == GPON_PHY_I2C_ADDR1) {
        p = create_proc_entry(PROC_ENTRY_NAME1, 0, q);
#ifdef GPON_I2C_IOCTL
		p->proc_fops = &gpon_fops0;
#endif
	}
    else {
        p = create_proc_entry(PROC_ENTRY_NAME2, 0, q);
#ifdef GPON_I2C_IOCTL
		p->proc_fops = &gpon_fops1;
#endif
	}
    if (!p) {
        BCM_LOG_ERROR(BCM_LOG_ID_I2C, "bcmlog: unable to create proc entry\n");
        err = -EIO;
#ifdef SYSFS_HOOKS
        sysfs_remove_group(&client->dev.kobj, &gponPhy_attr_group);
#endif
        goto exit_detach;
    }
#ifndef GPON_I2C_IOCTL
    p->read_proc = gponPhy_proc_read;
    p->write_proc = gponPhy_proc_write;
    p->data = (void *)pclient_data;
#endif

#ifdef GPON_I2C_TEST
    /* Create only once */
    if (address == GPON_PHY_I2C_ADDR1)
    {
        p = create_proc_entry(PROC_ENTRY_NAME3, 0, q);
        if (p) {
            p->proc_fops = &gponPhyTest_fops;
        }
    }
#endif
#endif

    return 0;

#if defined(SYSFS_HOOKS) || defined(PROCFS_HOOKS)
exit_detach:
    i2c_detach_client(client);
#endif
exit_kfree:
    kfree(pclient_data);
exit:
    return err;
}

static int gponPhy_detach_client(struct i2c_client *client)
{
    int err = 0;

    BCM_LOG_DEBUG(BCM_LOG_ID_I2C, "Entering the function %s \n", __FUNCTION__);

#ifdef SYSFS_HOOKS
    sysfs_remove_group(&client->dev.kobj, &gponPhy_attr_group);
#endif

#ifdef PROCFS_HOOKS
        remove_proc_entry(PROC_ENTRY_NAME1, q);
        remove_proc_entry(PROC_ENTRY_NAME2, q);
#ifdef GPON_I2C_TEST
        remove_proc_entry(PROC_ENTRY_NAME3, q);
#endif
        remove_proc_entry(PROC_DIR_NAME, NULL);
#endif

    err = i2c_detach_client(client);
    if (err)
        return err;

    kfree(i2c_get_clientdata(client));

    return err;
}

static int __init gponPhy_init(void)
{
    return i2c_add_driver(&gponPhy_driver);
}

static void __exit gponPhy_exit(void)
{
    i2c_del_driver(&gponPhy_driver);
}
module_init(gponPhy_init);
module_exit(gponPhy_exit);





#else /* !LINUX_VERSION_CODE < KERNEL_VERSION(3, 2, 0) */

static const struct i2c_device_id gpon_i2c_id_table[] = {
    { "gpon_i2c", 0 },
    { },
};

MODULE_DEVICE_TABLE(i2c, gpon_i2c_id_table);

static struct i2c_driver gponPhy_driver = {
    .class = ~0,
    .driver = {
        .name = "gpon_i2c",
    },
    .probe  = gponPhy_probe,
    .remove = gponPhy_remove,
    .id_table = gpon_i2c_id_table,
    .detect = gponPhy_detect,
    .address_list = normal_i2c
};

static int gponPhy_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int err = 0;
    struct gponPhy_data *pclient_data; 
#ifdef PROCFS_HOOKS
    struct proc_dir_entry *p;
#endif

    BCM_LOG_DEBUG(BCM_LOG_ID_I2C, "Entering the function %s \n", __FUNCTION__);

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
        goto exit;

    if (!(pclient_data = kzalloc(sizeof(struct gponPhy_data), GFP_KERNEL))) 
    {
        err = -ENOMEM;
        goto exit;
    }

    pclient_data->client.addr = client->addr;
    pclient_data->client.adapter = client->adapter;
    pclient_data->client.driver = client->driver; 
    pclient_data->client.flags = client->flags;

    i2c_set_clientdata(client, pclient_data);

    switch(client->addr)
    {
    case GPON_PHY_I2C_ADDR1:
    	pclient1_data = pclient_data;
    	break;
    case GPON_PHY_I2C_ADDR2:
        pclient2_data = pclient_data;
        break;
    default:
        BCM_LOG_DEBUG(BCM_LOG_ID_I2C, "%s client addr out of range \n", __FUNCTION__);
        goto exit_kfree;
    }

#ifdef SYSFS_HOOKS
    /* Register sysfs hooks */
    err = sysfs_create_group(&client->dev.kobj, &gponPhy_attr_group);
    if (err)
        goto exit_kfree;
#endif

#ifdef PROCFS_HOOKS
    if (client->addr == GPON_PHY_I2C_ADDR1)
    {
        q = proc_mkdir(PROC_DIR_NAME, NULL);
        if (!q) {
            BCM_LOG_ERROR(BCM_LOG_ID_I2C, "bcmlog: unable to create proc entry\n");
            err = -ENOMEM;
#ifdef SYSFS_HOOKS
            sysfs_remove_group(&client->dev.kobj, &gponPhy_attr_group);
#endif
            goto exit_kfree;
        }
    }

    if (client->addr == GPON_PHY_I2C_ADDR1) {
        p = create_proc_entry(PROC_ENTRY_NAME1, 0, q);
#ifdef GPON_I2C_IOCTL
		p->proc_fops = &gpon_fops0;
#endif
	}
    else {
        p = create_proc_entry(PROC_ENTRY_NAME2, 0, q);
#ifdef GPON_I2C_IOCTL
		p->proc_fops = &gpon_fops1;
#endif
	}

    if (!p) {
        BCM_LOG_ERROR(BCM_LOG_ID_I2C, "bcmlog: unable to create proc entry\n");
        err = -EIO;
#ifdef SYSFS_HOOKS
        sysfs_remove_group(&client->dev.kobj, &gponPhy_attr_group);
#endif
        goto exit_kfree;
    }
#ifndef GPON_I2C_IOCTL
    p->read_proc = gponPhy_proc_read;
    p->write_proc = gponPhy_proc_write;
    p->data = (void *)pclient_data;
#endif

#ifdef GPON_I2C_TEST
    /* Create only once */
    if (client->addr == GPON_PHY_I2C_ADDR1)
    {
        p = create_proc_entry(PROC_ENTRY_NAME3, 0, q);
        if (p) {
            p->proc_fops = &gponPhyTest_fops;
        }
    }
#endif
#endif

    return 0;

exit_kfree:
    kfree(pclient_data);
exit:
    return err;
   
}

static int gponPhy_detect(struct i2c_client *client, struct i2c_board_info *info)
{
    BCM_LOG_DEBUG(BCM_LOG_ID_I2C, "Entering the function %s \n", __FUNCTION__);
    strcpy(info->type, "gpon_i2c");
    info->flags = 0;
    return 0;
}

static int gponPhy_remove(struct i2c_client *client)
{
    int err = 0;

    BCM_LOG_DEBUG(BCM_LOG_ID_I2C, "Entering the function %s \n", __FUNCTION__);

#ifdef SYSFS_HOOKS
    sysfs_remove_group(&client->dev.kobj, &gponPhy_attr_group);
#endif

#ifdef PROCFS_HOOKS
        remove_proc_entry(PROC_ENTRY_NAME1, q);
        remove_proc_entry(PROC_ENTRY_NAME2, q);
#ifdef GPON_I2C_TEST
        remove_proc_entry(PROC_ENTRY_NAME3, q);
#endif
        remove_proc_entry(PROC_DIR_NAME, NULL);
#endif

	kfree(i2c_get_clientdata(client));

	return err;
}

//module_i2c_driver(gponPhy_driver);
static int __init gponPhy_init(void)
{
	int ret;
	struct i2c_board_info info;
	struct i2c_adapter *adapter;
	struct i2c_client *client;
	int i2c_bus = 0;

	memset(&info, 0, sizeof(struct i2c_board_info));
	info.addr = normal_i2c[0];
	strcpy(info.type, "gpon_i2c");

	adapter = i2c_get_adapter(i2c_bus);
	if (!adapter) {
		printk("can't get i2c adapter %d\n", i2c_bus);
		goto err_driver;
	}

	client = i2c_new_device(adapter, &info);
	if (!client) {
		printk("can't add i2c device at 0x%x\n",
			(unsigned int)info.addr);
		goto err_driver;
	}

	info.addr = normal_i2c[1];
	client = i2c_new_device(adapter, &info);
	if (!client) {
		printk("can't add i2c device at 0x%x\n",
			(unsigned int)info.addr);
		goto err_driver;
	}

	i2c_put_adapter(adapter);

	/* add i2c driver */
	ret = i2c_add_driver(&gponPhy_driver);
	if (ret) 
	{
		BCM_LOG_ERROR(BCM_LOG_ID_I2C,"i2c_add_driver failed");
		return ret;
	}  

	/* Load the default transceiver configuration  */
	bcm_load_trsv_dflt_config();
  
	return 0;

err_driver:
	return -ENODEV;
}

static void __exit gponPhy_exit(void)
{
	i2c_del_driver(&gponPhy_driver);
}
module_init(gponPhy_init);
module_exit(gponPhy_exit);


#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(3, 2, 0) */


MODULE_AUTHOR("Pratapa Reddy, Vaka <pvaka@broadcom.com>");
MODULE_DESCRIPTION("GPON OLT Transceiver I2C driver");
MODULE_LICENSE("GPL");
