/*
 
 Copyright (c) 2008 Broadcom Corporation 
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
/***********************************************************************/
/*                                                                     */
/*   MODULE   gpon_i2c.h                                               */
/*   DATE:    07/23/08                                                 */
/*   PURPOSE: GPON Transceiver reg access API                          */
/*                                                                     */
/***********************************************************************/
#ifndef __GPON_I2C_H
#define __GPON_I2C_H

#if defined(CONFIG_BCM_PMD_MODULE) ||  defined(CONFIG_BCM_PMD)
    #define MAX_TRANSACTION_SIZE  64000
#else
    #define MAX_TRANSACTION_SIZE  32
#endif
typedef unsigned char  u8;
typedef unsigned short u16;


/* Global Comments */
/* client_num = 0 selects the first EEPROM at I2C address 0xA0 and non-zero */
/* client_num selects the second EEPROM at I2C address 0xA2.                */

/****************************************************************************/
/* Write gponPhy: Writes count number of bytes from buf on to the I2C bus   */
/* Returns:                                                                 */
/*   number of bytes written on success, negative value on failure.         */
/* Notes: 1. The count > 32 is not yet supported                            */
/*        2. The buf[0] should be the offset where write starts             */
/****************************************************************************/
ssize_t gponPhy_write(u8 client_num, char *buf, size_t count);

/****************************************************************************/
/* Read gponPhy: Reads count number of bytes from gponPhy                   */
/* Returns:                                                                 */
/*   number of bytes read on success, negative value on failure.            */
/* Notes: 1. The count > 32 is not yet supported                            */
/*        2. The buf[0] should be the offset where read starts              */
/****************************************************************************/
ssize_t gponPhy_read(u8 client_num, char *buf, size_t count);

/****************************************************************************/
/* Write Register: Writes the val into gponPhy register                     */
/* Returns:                                                                 */
/*   0 on success, negative value on failure.                               */
/* Notes: 1. The offset should be DWORD aligned                             */
/****************************************************************************/
int gponPhy_write_reg(u8 client_num, u8 offset, int val);

/****************************************************************************/
/* Read Register: Read the gponPhy register at given offset                 */
/* Returns:                                                                 */
/*   value on success, negative value on failure.                           */
/* Notes: 1. The offset should be DWORD aligned                             */
/****************************************************************************/
int gponPhy_read_reg(u8 client_num, u8 offset);

/****************************************************************************/
/* Write Word: Writes the val into LSB 2 bytes of Register                  */ 
/* Returns:                                                                 */
/*   0 on success, negative value on failure.                               */
/* Notes: 1. The offset should be WORD aligned                              */
/****************************************************************************/
int gponPhy_write_word(u8 client_num, u8 offset, u16 val);

/****************************************************************************/
/* Read Word: Reads the LSB 2 bytes of Register                             */ 
/* Returns:                                                                 */
/*   value on success, negative value on failure.                           */
/* Notes: 1. The offset should be WORD aligned                              */
/****************************************************************************/
u16 gponPhy_read_word(u8 client_num, u8 offset);

/****************************************************************************/
/* Write Byte: Writes the byte val into offset                              */ 
/* Returns:                                                                 */
/*   0 on success, negative value on failure.                               */
/****************************************************************************/
int gponPhy_write_byte(u8 client_num, u8 offset, u8 val);

/****************************************************************************/
/* Read Byte: Reads a byte from offset                                      */ 
/* Returns:                                                                 */
/*   value on success, negative value on failure.                           */
/****************************************************************************/
u8 gponPhy_read_byte(u8 client_num, u8 offset);

/****************************************************************************/
/*                                                                          */
/*               TRANSCEIVER                                                */
/*                                                                          */
/****************************************************************************/
#include <linux/netlink.h>

/* Client0 for Address 0x50 and Client1 for Address 0x51 */
#define client0               0
#define client1               1

enum
{
  GPON_I2C_IOC_SET,
  GPON_I2C_IOC_GET,
  GPON_I2C_IOC_SET_TASK_ID,
  GPON_I2C_IOC_SET_HYSTERESIS,
  GPON_I2C_IOC_SET_12V_VIDEO_POWER,
  GPON_I2C_IOC_GET_TRANSCEIVER_TYPE,
  GPON_I2C_IOC_SET_SIMULATOR,  
  GPON_I2C_IOC_A2_NUM,
};

typedef struct 
{
  unsigned char bank;
  unsigned char offset;
  unsigned char len;
  unsigned char buf[MAX_TRANSACTION_SIZE];
}transceiver_ioctl_st;

struct transceiver_event_info
{
  unsigned int type;
  unsigned int isrValue;
};

struct u_transceiver_packet_info
{
  struct nlmsghdr hdr;
  struct transceiver_event_info event;
};

typedef enum
{
  GPON_I2C_EVENT_ALARM,
  GPON_I2C_EVENT_WARN,
  GPON_I2C_EVENT_RF_LED,
  GPON_I2C_EVENT_NUM,
}GPON_I2C_EVENT_TYPE;


#endif
