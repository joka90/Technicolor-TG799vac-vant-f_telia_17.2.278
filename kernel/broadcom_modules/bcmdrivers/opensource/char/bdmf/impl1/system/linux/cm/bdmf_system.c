/*
* <:copyright-BRCM:2013:GPL/GPL:standard
*
*    Copyright (c) 2013 Broadcom Corporation
*    All Rights Reserved
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License, version 2, as published by
* the Free Software Foundation (the "GPL").
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*
* A copy of the GPL is available at http://www.broadcom.com/licenses/GPLv2.php, or by
* writing to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
* Boston, MA 02111-1307, USA.
*
* :>
*/

#include "bdmf_system.h"
#include "bdmf_system_common.h"
/*
 * System buffer abstraction
 */

/** Recycle the data buffer
 * \param[in]   data   Data buffer
 * \param[in]   context -unused,for future use
 */
void bdmf_sysb_databuf_recycle(void *datap, uint32_t context)
{
   return;
}

/** Recycle the system buffer and associated data buffer
 * \param[in]   data   Data buffer
 * \param[in]   context - unused,for future use
 * \param[in]   flags   - indicates what to recyle
 */
void bdmf_sysb_recycle(bdmf_sysb sysb, uint32_t context, uint32_t flags)
{
    return;
}

/*
 * Platform buffer support
 */
static inline void *bdmf_get_tm_ddr_base(void)
{
    return NULL;
}

/** Initialize platform buffer support
 * \param[in]   size    buffer size
 * \param[in]   offset  min offset
 */
void bdmf_pbuf_init(uint32_t size, uint32_t offset)
{

}

/** Allocate pbuf and fill with data
 * The function allocates platform buffer and copies data into it
 * \param[in]   data        data pointer
 * \param[in]   length      data length
 * \param[in]   source      source port as defined by RDD
 * \param[out]  pbuf        Platform buffer
 * \return 0 if OK or error < 0
 */
int bdmf_pbuf_alloc(void *data, uint32_t length, uint16_t source, bdmf_pbuf_t *pbuf)
{
    return 0;
}

/** Add data to sysb
 *
 * The function will is similar to skb_put()
 *
 * \param[in]   sysb        System buffer
 * \param[in]   bytes       Bytes to add
 * \returns added block pointer
 */
static inline void *bdmf_sysb_put(const bdmf_sysb sysb, uint32_t bytes)
{
   return NULL;
}

/** Convert sysb to platform buffer
 * \param[in]   sysb        System buffer. Released regardless on the outcome
 * \param[in]   pbuf_source BPM source port as defined by RDD
 * \param[out]  pbuf        Platform buffer
 * \return 0=OK, <0-error (sysb doesn't contain pbuf info)
 */
int bdmf_pbuf_from_sysb(const bdmf_sysb sysb, uint16_t pbuf_source, bdmf_pbuf_t *pbuf)
{
    return 0;
}

/** Convert platform buffer to sysb
 * \param[in]   sysb_type   System buffer type
 * \param[in]   pbuf        Platform buffer. Released regardless on the outcome or becomes "owned" by sysb
 * \return sysb pointer or NULL
 */
bdmf_sysb bdmf_pbuf_to_sysb(bdmf_sysb_type sysb_type, bdmf_pbuf_t *pbuf)
{
    return NULL;
}

int bdmf_int_connect(int irq, int cpu, int flags,
    int (*isr)(int irq, void *priv), const char *name, void *priv)
{
	/* if needed, put CM code to register for an interrupt */
	return 0;
}

/** Unmask IRQ
 * \param[in]   irq IRQ
 */
void bdmf_int_enable(int irq)
{
    /*
     * in case of Oren the BcmHalMapInterrupt already enables the interrupt.
     */
}

/** Mask IRQ
 * \param[in]   irq IRQ
 */
void bdmf_int_disable(int irq)
{
    /* Supposingly should work for all BCM platforms.
     * If it is not the case - mode ifdefs can be added later.
     */
}
