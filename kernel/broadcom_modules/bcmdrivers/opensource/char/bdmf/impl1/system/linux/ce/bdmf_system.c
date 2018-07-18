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
#include "bcm_intr.h"
#if defined(CONFIG_BCM96838) || defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
#include "board.h"
#include "bcm_mm.h"
extern int BcmMemReserveGetByName(char *name, void **addr, uint32_t *size);
#endif
#if defined(CONFIG_BCM_PKTRUNNER_GSO)
#include "kmap_skb.h"
#include "bcm_mm.h"
#endif


/*
 * System buffer abstraction
 */

/** Recycle the data buffer
 * \param[in]   data   Data buffer
 * \param[in]   context -unused,for future use
 */
void bdmf_sysb_databuf_recycle(void *datap, uint32_t context)
{
    __bdmf_sysb_databuf_recycle(datap, context);
}

/** Recycle the system buffer and associated data buffer
 * \param[in]   data   Data buffer
 * \param[in]   context - unused,for future use
 * \param[in]   flags   - indicates what to recyle
 */
void bdmf_sysb_recycle(bdmf_sysb sysb, uint32_t context, uint32_t flags)
{
    if ( IS_FKBUFF_PTR(sysb) )
    {
        __bdmf_sysb_databuf_recycle(PNBUFF_2_FKBUFF(sysb), context);
    }
    else /* skb */
    {
        struct sk_buff *skb = PNBUFF_2_SKBUFF(sysb);

        if (flags & SKB_DATA_RECYCLE)
        {
            void *data_endp;
            void *data_startp = skb->head + BCM_PKT_HEADROOM;

#if defined(CC_NBUFF_FLUSH_OPTIMIZATION)
            void *dirty_p = skb_shinfo(skb)->dirty_p;
            void *shinfoBegin = (uint8_t *)skb_shinfo(skb);
            void *shinfoEnd;
            if (skb_shinfo(skb)->nr_frags == 0)
            {
                // no frags was used on this skb, so can shorten amount of data
                // flushed on the skb_shared_info structure
                shinfoEnd = shinfoBegin + offsetof(struct skb_shared_info, frags);
            }
            else
            {
                shinfoEnd = shinfoBegin + sizeof(struct skb_shared_info);
            }
            //cache_flush_region(shinfoBegin, shinfoEnd);
            cache_invalidate_region(shinfoBegin, shinfoEnd);

            // If driver returned this buffer to us with a valid dirty_p,
            // then we can shorten the flush length.
            if (dirty_p)
            {
                if ((dirty_p < (void *)skb->head) || (dirty_p > shinfoBegin))
                {
                    printk("invalid dirty_p detected: %p valid=[%p %p]\n",
                           dirty_p, skb->head, shinfoBegin);
                    data_endp = shinfoBegin;
                }
                else
                {
                    data_endp = (dirty_p < data_startp) ? data_startp : dirty_p;
                }
            }
            else
            {
                data_endp = shinfoBegin;
            }
#else
            /*its ok not to invalidate skb->head to skb->data as this area is
             * not used by runner or DMA
             */
            data_endp = (void *)(skb_shinfo(skb)) + sizeof(struct skb_shared_info);
#endif
            cache_invalidate_region(data_startp, data_endp);

            /* free the buffer to global BPM pool */
            __bdmf_sysb_databuf_recycle(PDATA_TO_PFKBUFF(data_startp, BCM_PKT_HEADROOM), context);
        }
        else
        {
            printk("%s: Error only DATA recycle is supported\n", __FUNCTION__);
        }
    }
}

#if defined(CONFIG_BCM_PKTRUNNER_GSO)

static DEFINE_SPINLOCK(bdmf_runner_gso_desc_pool_lock);

#define BDMF_RUNNER_GSO_DESC_POOL_LOCK(flags)    spin_lock_irqsave(&bdmf_runner_gso_desc_pool_lock, flags)
#define BDMF_RUNNER_GSO_DESC_POOL_UNLOCK(flags)  spin_unlock_irqrestore(&bdmf_runner_gso_desc_pool_lock, flags)

runner_gso_desc_t *runner_gso_desc_pool=NULL;


/* bdmf_kmap_skb_frag bdmf_kunmap_skb_frag  functions are copied
 * from linux. Here we are not using local_bh_disable/local_bh_enable
 * when HIGHMEM is enabled. This is because currently these functions
 * are called with irq's disabled (f_rdd_lock_irq), kernel will generate
 * a warning if local_bh_enable is called when irq's are disabled
 */

/*TODO: optimize bdmf_kmap_skb_frag bdmf_kunmap_skb_frag */

/** map a page to virtual address
 * \param[in]   skb_frag_t
 * return : virtual address
 */
static inline void *bdmf_kmap_skb_frag(const skb_frag_t *frag)
{
#ifdef CONFIG_HIGHMEM
    BUG_ON(in_irq());

    //local_bh_disable();
#endif
    return kmap_atomic(skb_frag_page(frag));
}

/** Free virtual address mapping
 * \param[in]   vaddr virtual address
 */
static inline void bdmf_kunmap_skb_frag(void *vaddr)
{
    kunmap_atomic(vaddr);
#ifdef CONFIG_HIGHMEM
    //local_bh_enable();
#endif
}

//#define CONFIG_BCM_PKTRUNNER_GSO_DEBUG 1

/** Recycle a GSO Descriptor
 * \param[in]   gso_desc_p   pointer to GSO Descriptor
 */
void bdmf_runner_gso_desc_free(runner_gso_desc_t *gso_desc_p)
{
    if(gso_desc_p)
    {
        unsigned long flags;

        BDMF_RUNNER_GSO_DESC_POOL_LOCK(flags);
        gso_desc_p->isAllocated = 0;
        cache_flush_len(gso_desc_p, sizeof(runner_gso_desc_t));
        BDMF_RUNNER_GSO_DESC_POOL_UNLOCK(flags);
    }
}

/** Allocate a GSO Descriptor
 * \returns pointer to allocated GSO Descriptor
 */
static inline runner_gso_desc_t *bdmf_runner_gso_desc_alloc(void)
{
    static int alloc_index = 0;
    int i=0;
    unsigned long flags;
    runner_gso_desc_t *gso_desc_p;

    BDMF_RUNNER_GSO_DESC_POOL_LOCK(flags);

    while(i < RUNNER_MAX_GSO_DESC)
    {
        if(alloc_index == RUNNER_MAX_GSO_DESC)
            alloc_index = 0;

        gso_desc_p = &runner_gso_desc_pool[alloc_index];
        cache_invalidate_len(gso_desc_p, sizeof(runner_gso_desc_t));
        if(!gso_desc_p->isAllocated)
        {
    #if 1
            memset(gso_desc_p, 0, sizeof(runner_gso_desc_t));
    #else
            gso_desc_p->nr_frags = 0;
            gso_desc_p->flags = 0;
            gso_desc_p->mss = 0;
    #endif

            gso_desc_p->isAllocated = 1;
            alloc_index++;
            BDMF_RUNNER_GSO_DESC_POOL_UNLOCK(flags);
            return gso_desc_p;
        }
        alloc_index++;
        i++;
    }
    BDMF_RUNNER_GSO_DESC_POOL_UNLOCK(flags);
    return NULL;
}

#if defined(CONFIG_BCM_PKTRUNNER_GSO_DEBUG)
static int bdmf_gso_desc_dump_enabled=0;

/**  Print information in GSO Descriptor
 * \param[in]   gso_desc_p   pointer to GSO Descriptor
 */
void bdmf_gso_desc_dump(runner_gso_desc_t *gso_desc_p)
{
    int i;

    if(bdmf_gso_desc_dump_enabled)
    {
        printk("******* Dumping gso_desc_p =%p  runner phys-addr =%x*********\n", gso_desc_p, swap4bytes(VIRT_TO_PHYS(gso_desc_p)));

        printk("gso_desc_p ->len:        host(%d ) runner(%d) \n", swap2bytes(gso_desc_p->len), gso_desc_p->len);
        printk("gso_desc_p ->linear_len: host(%d ) runner(%d) \n", swap2bytes(gso_desc_p->linear_len), gso_desc_p->linear_len);
        printk("gso_desc_p ->mss:        host(%d ) runner(%d)\n", swap2bytes(gso_desc_p->mss), gso_desc_p->mss);
        printk("gso_desc_p ->nr_frags:   %d \n", gso_desc_p->nr_frags);

        printk("gso_desc_p ->data:       host(%x ) runner(%x)\n", swap4bytes(gso_desc_p->data), gso_desc_p->data);
        for (i=0; i < gso_desc_p ->nr_frags; i++)
        {
            printk("Hostfrag_data[%d]=%x, frag_len=%d \n", i, swap4bytes(gso_desc_p->frag_data[i]), swap2bytes(gso_desc_p->frag_len[i]));
            printk("frag_data[%d]=%x, frag_len=%d \n", i, gso_desc_p->frag_data[i], gso_desc_p->frag_len[i]);
        }
        cache_invalidate_len(gso_desc_p, sizeof(runner_gso_desc_t));
    }
}
#endif

/** Create a pool of GSO Descriptors
 * \param[in]   num_desc  number of descriptors to be created
 * \returns 0=Success -1=failure
 */
int bdmf_gso_desc_pool_create( uint32_t num_desc)
{
    uint32_t mem_size = num_desc * sizeof(runner_gso_desc_t);

    runner_gso_desc_pool = CACHED_MALLOC(mem_size);
    if(!runner_gso_desc_pool)
    {
        printk(KERN_ERR "###### ERROR:Failed to allocate runner_gso_desc_pool\n");
        return -1;
    }
    memset(runner_gso_desc_pool, 0, mem_size);
    cache_flush_len(runner_gso_desc_pool,  mem_size);

    printk("++++Runner gso_desc_pool created successfully\n");
#if defined(CONFIG_BCM_PKTRUNNER_GSO_DEBUG)
    printk("&bdmf_gso_desc_dump_enabled=%p bdmf_gso_desc_dump_enabled=%d\n",
            &bdmf_gso_desc_dump_enabled, bdmf_gso_desc_dump_enabled);
#endif

    return 0;
}
EXPORT_SYMBOL(bdmf_gso_desc_pool_create);

/** Free a pool of GSO Descriptors
 */
void bdmf_gso_desc_pool_destroy(void)
{
    CACHED_FREE(runner_gso_desc_pool);
}
EXPORT_SYMBOL(bdmf_gso_desc_pool_destroy);


/** Checks if a packet needs GSO processing and convert skb to GSO Descriptor
 * \param[in]   sysb  system buffer
 * \param[out]  is_gso_pkt_p indicates to caller if sysb is a GSO packet
 * \returns for Non-GSO: sysb->data GSO: GSO Desciptor or NULL
 */
void* bdmf_sysb_data_to_gsodesc(const bdmf_sysb sysb, uint32_t *is_gso_pkt_p)
{
#if 0
    if(IS_FKBUFF_PTR(sysb))
    {
        printk("GSO not supported with fkbs's \n");
        return NULL;
    }
    else
#endif
    {
        const struct sk_buff *skb = PNBUFF_2_SKBUFF(sysb);
        uint32 linear_len, total_frag_len;
        uint16 nr_frags, i;
        runner_gso_desc_t *gso_desc_p;
        skb_frag_t *frag;
        uint8 *vaddr;

        if((skb->ip_summed == CHECKSUM_PARTIAL) || skb_is_gso(skb) || skb_shinfo(skb)->nr_frags)
        {

#if 0
            /*TODO remove this check after verfiying with 8K page sizes */
            if( skb->len > 64*1024)
            {
                printk("%s: ERROR: skb->len %d  >64K\n",__FUNCTION__, skb->len);
            }
#endif

            /*TODO when FRAGLIST is supported add nr_frags of all skb's in fraglist */
            nr_frags = skb_shinfo(skb)->nr_frags;

            if(nr_frags >  RUNNER_MAX_GSO_FRAGS)
            {
                printk("%s: ERROR: nr_frags %d exceed max\n",__FUNCTION__, nr_frags);
                return NULL;
            }

            gso_desc_p = bdmf_runner_gso_desc_alloc();
            if(!gso_desc_p)
            {
                printk("%s: ERROR: failed to allocate gso_desc_p\n",__FUNCTION__);
                return NULL;
            }

            if(nr_frags)
            {

                total_frag_len = 0;

                for(i=0; i < nr_frags; i++ )
                {
                    frag = &skb_shinfo(skb)->frags[i];
                    vaddr = bdmf_kmap_skb_frag(frag);
                    cache_flush_len((vaddr + frag->page_offset), frag->size);
                    gso_desc_p->frag_data[i]= swap4bytes(VIRT_TO_PHYS(vaddr + frag->page_offset));
                    gso_desc_p->frag_len[i] = swap2bytes((uint16_t)(frag->size));
                    total_frag_len += frag->size;
                    bdmf_kunmap_skb_frag(vaddr);
                }

                linear_len = skb->len - total_frag_len;

            }
            else
            {
                linear_len = skb->len;
            }
            /*We expect skb->data to be flushed already in RDPA */
            //cache_flush_len(skb->data, linear_len);


            gso_desc_p->data = swap4bytes(VIRT_TO_PHYS(skb->data));
            gso_desc_p->linear_len = swap2bytes(linear_len);
            gso_desc_p->len = swap2bytes(skb->len);
            gso_desc_p->nr_frags = nr_frags;
            gso_desc_p->mss = swap2bytes(skb_shinfo(skb)->gso_size);
            *is_gso_pkt_p = 1;
            cache_flush_len(gso_desc_p, sizeof(runner_gso_desc_t));

#if defined(CONFIG_BCM_PKTRUNNER_GSO_DEBUG)
            bdmf_gso_desc_dump(gso_desc_p);
#endif

            return gso_desc_p;
        }
        else
        {
            *is_gso_pkt_p = 0;
            return skb->data;
        }
    }
}
EXPORT_SYMBOL(bdmf_sysb_data_to_gsodesc);
#endif

/*
 * Platform buffer support
 */
static void *tm_ddr_base;
static void *tm_ddr_end;
static uint32_t tm_pbuf_size;
static uint32_t tm_pbuf_offset;

static inline void *bdmf_get_tm_ddr_base(void)
{
    uint32_t tm_ddr_size = 0;
    int rc = 0;

    if (tm_ddr_base)
        return tm_ddr_base;
//TODO:replace the config by something else
    rc = BcmMemReserveGetByName("tm", &tm_ddr_base, &tm_ddr_size);
    if (rc == -1 || ((uint32_t)tm_ddr_base % 0x200000) != 0)
         printk( "Failed to get valid DDR TM address, rc = %d  validate_ddr_address: ddr_tm_base_address=%x\n", rc,
                  (int) tm_ddr_base);

    tm_ddr_end = tm_ddr_base + tm_ddr_size - 1;
    return tm_ddr_base;
}

/** Initialize platform buffer support
 * \param[in]   size    buffer size
 * \param[in]   offset  min offset
 */
void bdmf_pbuf_init(uint32_t size, uint32_t offset)
{
    bdmf_get_tm_ddr_base();
    tm_pbuf_size = size;
    tm_pbuf_offset = offset;
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
    uint32_t bn;
    void *buffer_ptr;

    BUG_ON(!tm_pbuf_size);
    if (tm_pbuf_offset + length > tm_pbuf_size)
        return BDMF_ERR_OVERFLOW;
    if (fi_bl_drv_bpm_req_buffer(source, &bn))
        return BDMF_ERR_NOMEM;
    buffer_ptr = bdmf_get_tm_ddr_base() + bn * tm_pbuf_size + tm_pbuf_offset;
    /* ToDo: copy via cache */
    memcpy(buffer_ptr, data, length);
    pbuf->bpm_bn = bn;
    pbuf->length = length;
    pbuf->source = source;
    pbuf->data = buffer_ptr;
    pbuf->offset = tm_pbuf_offset;
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
    return nbuff_put((struct sk_buff *)sysb, bytes);
}

/** Convert sysb to platform buffer
 * \param[in]   sysb        System buffer. Released regardless on the outcome
 * \param[in]   pbuf_source BPM source port as defined by RDD
 * \param[out]  pbuf        Platform buffer
 * \return 0=OK, <0-error (sysb doesn't contain pbuf info)
 */
int bdmf_pbuf_from_sysb(const bdmf_sysb sysb, uint16_t pbuf_source, bdmf_pbuf_t *pbuf)
{
    /* ToDo: for now do copy.
     * When adding zero copy not to forget cache flush
     */
    uint32_t length;
    void *data;
    int rc;

    length = bdmf_sysb_length(sysb);
    data = bdmf_sysb_data(sysb);
    rc = bdmf_pbuf_alloc(data, length, pbuf_source, pbuf);
    bdmf_sysb_free(sysb);
    return rc;
}

/** Convert platform buffer to sysb
 * \param[in]   sysb_type   System buffer type
 * \param[in]   pbuf        Platform buffer. Released regardless on the outcome or becomes "owned" by sysb
 * \return sysb pointer or NULL
 */
bdmf_sysb bdmf_pbuf_to_sysb(bdmf_sysb_type sysb_type, bdmf_pbuf_t *pbuf)
{
    /* ToDo: currently do a copy. Add zero-copy next.
     * remember about cache invalidation!
     */
    bdmf_sysb sysb;
    uint32_t data_bufp;
    uint32_t context=0;
    void *data = (pbuf->bpm_bn == INVALID_BPM_BUFFER) ? bdmf_sysb_data((bdmf_sysb)pbuf->data) : pbuf->data;

    if(!bdmf_sysb_databuf_alloc(&data_bufp, 1 , context))
    {
        bdmf_pbuf_free(pbuf);
        return NULL;
    }
    /* ToDo: copy via cache */
    memcpy((void*)data_bufp, data, pbuf->length);

    sysb = bdmf_sysb_header_alloc(sysb_type, (void*)data_bufp, pbuf->length, context);
    if (!sysb)
    {
        bdmf_sysb_databuf_free((void*)data_bufp, context);
    }
    bdmf_pbuf_free(pbuf);
    return sysb;
}

int bdmf_int_connect(int irq, int cpu, int flags,
    int (*isr)(int irq, void *priv), const char *name, void *priv)
{
    int rc;
    /* Supposingly should work for all BCM platforms.
     * If it is not the case - mode ifdefs can be added later.
     * We might also switch to
     * BcmHalMapInterruptEx in order to handle affinity.
     */
    rc = BcmHalMapInterrupt((FN_HANDLER)isr, (unsigned int)priv, irq);
    return rc ? BDMF_ERR_INTERNAL : 0;
}

/** Unmask IRQ
 * \param[in]   irq IRQ
 */
void bdmf_int_enable(int irq)
{
    /*
     * in case of Oren the BcmHalMapInterrupt already enables the interrupt.
     */
#if defined(CONFIG_BCM96838)
    BcmHalInterruptEnable(irq);
#endif
    /* Supposingly should work for all BCM platforms.
     * If it is not the case - mode ifdefs can be added later.
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
    BcmHalInterruptDisable(irq);
}
EXPORT_SYMBOL(bdmf_sysb_headroom_size_set);
EXPORT_SYMBOL(bdmf_sysb_databuf_recycle);
EXPORT_SYMBOL(bdmf_sysb_recycle);
EXPORT_SYMBOL(bdmf_int_connect);
EXPORT_SYMBOL(bdmf_int_enable);
EXPORT_SYMBOL(bdmf_int_disable);
EXPORT_SYMBOL(bdmf_pbuf_init);
EXPORT_SYMBOL(bdmf_pbuf_alloc);
EXPORT_SYMBOL(bdmf_pbuf_free);
EXPORT_SYMBOL(bdmf_pbuf_from_sysb);
EXPORT_SYMBOL(bdmf_pbuf_to_sysb);
