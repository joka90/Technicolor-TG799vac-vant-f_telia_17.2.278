/************** COPYRIGHT AND CONFIDENTIALITY INFORMATION *********************
**                                                                          **
** Copyright (c) 2012 Technicolor                                           **
** All Rights Reserved                                                      **
**                                                                          **
** This program contains proprietary information which is a trade           **
** secret of TECHNICOLOR and/or its affiliates and also is protected as     **
** an unpublished work under applicable Copyright laws. Recipient is        **
** to retain this program in confidence and is not permitted to use or      **
** make copies thereof other than as permitted in a written agreement       **
** with TECHNICOLOR, UNLESS OTHERWISE EXPRESSLY ALLOWED BY APPLICABLE LAWS. **
**                                                                          **
******************************************************************************/

/** @file
 *
 * File containing functions to access the Remote Inventory Parameters (RIP).
 */

#ifndef __RIP_H__
#define __RIP_H__



/**
 * The maximum length of a single RIP item is limited by the size of the RIP 
 * partition (128kB) - the length of the RIP header (32 bit).
 */
#define RIP_MAX_LEN		(0x20000 - 4)

/**
 * Definition of the attribute fields.
 */

#define RIP_ATTR_ANY           0xFFFFFFFF
/** 
 *
 */
#define RIP_ATTR_CRC_MSK		0x80000000
#define RIP_ATTR_CHK_CRC		(~RIP_ATTR_CRC_MSK)
#define RIP_ATTR_DONT_CHK_CRC	(RIP_ATTR_CRC_MSK)
#define RIP_ATTR_WRITABLE      0x40000000
#define RIP_ATTR_VALID         0x20000000
#define RIP_ATTR_N_EIK_SIGN    0x10000000 /* 60 */
#define RIP_ATTR_N_ECK_ENCR    0x08000000 /* 59 */
#define RIP_ATTR_N_MCV_SIGN    0x04000000 /* 58 */
#define RIP_ATTR_N_BEK_ENCR    0x02000000 /* 57 */
#define ATTR_HI                 1
#define ATTR_LO                 0

#define RIP_ATTR_HI_MASK		0xE0000000
/* default value is: check crc|!writable|valid */
#define RIP_ATTR_DEFAULT       ((RIP_ATTR_ANY & ~RIP_ATTR_HI_MASK) | RIP_ATTR_VALID) /* Default is 0x3FFFFFFF */

#define RIP_ATTR_CRYPTO		(RIP_ATTR_N_EIK_SIGN | RIP_ATTR_N_ECK_ENCR | RIP_ATTR_N_MCV_SIGN | RIP_ATTR_N_BEK_ENCR)


/**
 * Reads the specified parameter from the RIP.
 * As a result the data corresponding to the parameter identified by id will be 
 * copied into the data buffer. The length field will be updated to reflect
 * the amount of bytes actually read from the RIP
 *
 * @param[in,out]  length  Pointer to an unsigned long containing the length of
 *                         the data buffer passed. After a successful read the
 *                         length field will contain the number of bytes read
 *                         from the RIP.
 * @param[in]  id          The RIP id of the parameters to be read.
 * @param[out] data        A previously allocated piece of memory. After a successful
 *                         read it contains the RIP data for id.
 * @return 0 in case of success or a negative number in case of failure.
 */
int rip_read (unsigned long *length, unsigned short id, unsigned char *data);

/**
 * Writes the specified parameter to the RIP.
 * As a result the data in the buffer will be written to the parameter 
 * identified by id. If the id does not exist yet, a new entry will be
 * created. If the id exists, the write operation will only be performed
 * if the WRITEABLE bit is set in the RIP parameter's attribute flags.
 *
 * @param[in]  length  The length of the data buffer to be written.
 * @param[in]  id      The RIP id of the parameters to be written.
 * @param[in]  data    A pointer to the data buffer containing the data to be
 *                     written. 
 * @param[in]  attrHi  The upper 32 bits of the attribute flags for the RIP
 *                     parameter.
 * @param[in]  attrLo  The lower 32 bits of the attribute flags for the RIP
 *                     parameter.
 * @return 0 in case of success or a negative number in case of failure.
 */
int rip_write (unsigned long length, unsigned short id, unsigned char *data, unsigned long attrHi, unsigned long attrLo);

/**
 * Get the attribute flags for the specified id in the RIP.
 * As a result the attribute flags will be read and copied into the
 * attrHi and attrLo parameters.
 *
 * @param[in]  id      The RIP id of the parameters that is the target of this
 *                     command.
 * @param[in]  attrHi  A pointer to an unsigned 32 bit value. The upper 32 
 *					   bits of the attribute flags for the RIP parameter will
 *					   be copied the unsigned 32 bit value.
 * @param[in]  attrLo  A pointer to an unsigned 32 bit value. The lower 32 
 *					   bits of the attribute flags for the RIP parameter will
 *					   be copied the unsigned 32 bit value.
 * @return 0 in case of success or a negative number in case of failure.
 */
int rip_get_flags (unsigned short id, unsigned long *attrHi, unsigned long *attrLo);


/**
 * Disables write access to the specified id in the RIP.
 * As a result the WRITEABLE bit in the attribute flags for the RIP parameter
 * identified by id will be unset.
 *
 * @param[in]  id      The RIP id of the parameter to be locked.
 * @return 0 in case of success or a negative number in case of failure.
 */
int rip_lock_item (unsigned short id);

#endif // __RIP_H__
