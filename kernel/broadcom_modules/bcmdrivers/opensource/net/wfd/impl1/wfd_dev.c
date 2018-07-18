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

/****************************************************************************/
/**                                                                        **/
/** Software unit Wlan accelerator dev                                     **/
/**                                                                        **/
/** Title:                                                                 **/
/**                                                                        **/
/**   Wlan accelerator interface.                                          **/
/**                                                                        **/
/** Abstract:                                                              **/
/**                                                                        **/
/**   Mediation layer between the wifi / PCI interface and the Accelerator **/
/**  (Runner/FAP)                                                          **/
/**                                                                        **/
/** Allocated requirements:                                                **/
/**                                                                        **/
/** Allocated resources:                                                   **/
/**                                                                        **/
/**   A thread.                                                            **/
/**   An interrupt.                                                        **/
/**                                                                        **/
/**                                                                        **/
/****************************************************************************/


/****************************************************************************/
/******************** Operating system include files ************************/
/****************************************************************************/
#include <linux/types.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <net/route.h>
#include <linux/moduleparam.h>
#include <linux/netdevice.h>
#include <linux/if_ether.h>
#include <linux/etherdevice.h>
#include <linux/kthread.h>
#include "wfd_dev.h"

#if defined(PKTC) && defined(CONFIG_BCM_WFD_CHAIN_SUPPORT)
#include <linux_osl_dslcpe_pktc.h>
#include <linux/bcm_skb_defines.h>
#endif

/****************************************************************************/
/***************************** Module Version *******************************/
/****************************************************************************/
static const char *version = "Wifi Forwarding Driver";

#define WFD_MAX_OBJECTS   3
#define WFD_QUEUE_TO_WFD_IDX_MASK 0x1
#define WIFI_MW_MAX_NUM_IF    ( 16 )
#define WIFI_IF_NAME_STR_LEN  ( IFNAMSIZ )
#define WLAN_CHAINID_OFFSET 8
#define WFD_INTERRUPT_COALESCING_TIMEOUT_US 500
#define WFD_INTERRUPT_COALESCING_MAX_PKT_CNT 32
#define WFD_MCAST_OBJECT_IDX 2
#define WFD_MCAST_QUEUE_IDX 2

/****************************************************************************/
/*********************** Multiple SSID FUNCTIONALITY ************************/
/****************************************************************************/
static struct net_device __read_mostly *wifi_net_devices[WIFI_MW_MAX_NUM_IF]={NULL, } ;

static struct proc_dir_entry *proc_directory, *proc_file_conf;

extern void replace_upper_layer_packet_destination( void * cb, void * napi_cb );
extern void unreplace_upper_layer_packet_destination( void );

static int wfd_tasklet_handler(void  *context);

/****************************************************************************/
/***************************** Module parameters*****************************/
/****************************************************************************/
/* Initial maximum queue size */
static int packet_threshold = 0;
module_param (packet_threshold, int, 0);
/* Number of packets to read in each tasklet iteration */
#define NUM_PACKETS_TO_READ_MAX 128
static int num_packets_to_read = NUM_PACKETS_TO_READ_MAX;
module_param (num_packets_to_read, int, 0);
/* Initial number of configured PCI queues */
static int number_of_queues = 1;
module_param (number_of_queues, int, 0);
/* first Cpu ring queue - Currently pci CPU ring queues must be sequentioal */
static int first_pci_queue = 8;
module_param (first_pci_queue,int,0);
/* wifi Broadcom prefix */
static char wifi_prefix [WIFI_IF_NAME_STR_LEN] = "wl";
module_param_string (wifi_prefix, wifi_prefix, WIFI_IF_NAME_STR_LEN, 0);

/* FRV: Direct release of buffers from wl to wfd */
#ifdef CONFIG_TCH_KF_WLAN_PERF
RecycleFuncP bcm63xx_wfd_recycle_hook;
EXPORT_SYMBOL(bcm63xx_wfd_recycle_hook);
static struct kmem_cache *wfd_skbuff_head_cache __read_mostly;
#endif /* CONFIG_TCH_KF_WLAN_PERF */
/* FRV: Direct release of buffers from wl to wfd */

/* Counters */
static unsigned int gs_count_rx_queue_packets [WIFI_MW_MAX_NUM_IF] = {0, } ;
static unsigned int gs_count_tx_packets [WIFI_MW_MAX_NUM_IF] = {0, } ;
static unsigned int gs_count_no_buffers [WFD_MAX_OBJECTS] = { } ;
static unsigned int gs_count_rx_pkt [WFD_MAX_OBJECTS] = { } ;
static unsigned int gs_count_rx_invalid_ssid_vector[WFD_MAX_OBJECTS] = {};
static unsigned int gs_count_rx_no_wifi_interface[WFD_MAX_OBJECTS] = {} ;
typedef uint32_t (*wfd_bulk_get_t)(unsigned long qid, unsigned long budget, void *wfd_p, void **rx_pkts);

typedef struct
{
    struct net_device *wl_dev_p;
    enumWFD_WlFwdHookType eFwdHookType; 
    bool isTxChainingReqd;
    wfd_bulk_get_t wfd_bulk_get;
    HOOK4PARM wfd_fwdHook;
    HOOK32 wfd_completeHook;
    HOOK2PARM wfd_rxLoopBackHook;
	unsigned int wl_chained_packets;
	unsigned int wl_unchained_packets;
	void * wfd_acc_info_p;
	unsigned int wfd_idx;
	unsigned int wfd_rx_work_avail;
	wait_queue_head_t   wfd_rx_thread_wqh;
	struct task_struct *wfd_rx_thread;
	int wfd_queue_mask;
/* FRV: Direct release of buffers from wl to wfd */
#ifdef CONFIG_TCH_KF_WLAN_PERF
	struct sk_buff *release_head;
#endif /* CONFIG_TCH_KF_WLAN_PERF */
/* FRV: Direct release of buffers from wl to wfd */
} wfd_object_t;

static wfd_object_t wfd_objects[WFD_MAX_OBJECTS];
static int wfd_idx=0;
static spinlock_t wfd_irq_lock[WFD_MAX_OBJECTS];
static int mcast_instance_idx = -1;

#define WFD_IRQ_LOCK(wfdLockIdx, flags) spin_lock_irqsave(&wfd_irq_lock[wfdLockIdx], flags)
#define WFD_IRQ_UNLOCK(wfdLockIdx, flags) spin_unlock_irqrestore(&wfd_irq_lock[wfdLockIdx], flags)

#define WFD_WAKEUP_RXWORKER(wfdIdx) do { \
            wake_up_interruptible(&wfd_objects[wfdIdx].wfd_rx_thread_wqh); \
          } while (0)

int (*send_packet_to_upper_layer)(struct sk_buff *skb) = netif_rx;
EXPORT_SYMBOL(send_packet_to_upper_layer); 
int (*send_packet_to_upper_layer_napi)(struct sk_buff *skb) = netif_receive_skb;
EXPORT_SYMBOL(send_packet_to_upper_layer_napi);
int inject_to_fastpath = 0;
EXPORT_SYMBOL(inject_to_fastpath);

static void wfd_dump(void);

/****************************************************************************/
/******************* Other software units include files *********************/
/****************************************************************************/
#if (defined(CONFIG_BCM_RDPA)||defined(CONFIG_BCM_RDPA_MODULE)) 
#include "runner_wfd_inline.h"
void (*wfd_dump_fn)(void) = 0;
#elif (defined(CONFIG_BCM_FAP) || defined(CONFIG_BCM_FAP_MODULE))
#include "fap_wfd_inline.h"
extern void (*wfd_dump_fn)(void);
#endif

/****************************************************************************/
/**                                                                        **/
/** Name:                                                                  **/
/**                                                                        **/
/**   wfd_tasklet_handler.                                                 **/
/**                                                                        **/
/** Title:                                                                 **/
/**                                                                        **/
/**   wlan accelerator - tasklet handler                                   **/
/**                                                                        **/
/** Abstract:                                                              **/
/**                                                                        **/
/**   Reads all the packets from the Rx queue and send it to the wifi      **/
/**   interface.                                                           **/
/**                                                                        **/
/** Input:                                                                 **/
/**                                                                        **/
/** Output:                                                                **/
/**                                                                        **/
/**                                                                        **/
/****************************************************************************/
static int wfd_tasklet_handler(void *context)
{
    int wfdIdx = (int)context;
    int rx_pktcnt = 0;
    int qid, qidx = 0;
	wfd_object_t * wfd_p = &wfd_objects[wfdIdx];
    unsigned long flags;
    void *rx_pkts[NUM_PACKETS_TO_READ_MAX];
	uint32_t qMask = 0;

	printk("Instantiating WFD %d thread\n", wfdIdx);
    while (1)
    {
/* FRV: Direct release of buffers from wl to wfd */
#ifdef CONFIG_TCH_KF_WLAN_PERF
        wait_event_interruptible(wfd_p->wfd_rx_thread_wqh, wfd_p->wfd_rx_work_avail || wfd_p->release_head || kthread_should_stop());
#else
        wait_event_interruptible(wfd_p->wfd_rx_thread_wqh, wfd_p->wfd_rx_work_avail || kthread_should_stop());
#endif /* CONFIG_TCH_KF_WLAN_PERF */
/* FRV: Direct release of buffers from wl to wfd */

        if (kthread_should_stop())
        {
            printk(KERN_INFO "kthread_should_stop detected in wfd\n");
            break;
        }

/* FRV: Direct release of buffers from wl to wfd */
#ifdef CONFIG_TCH_KF_WLAN_PERF
	{
		struct sk_buff *head;
		struct sk_buff *next;
		
		WFD_IRQ_LOCK(wfdIdx, flags);
		head = wfd_p->release_head;
		wfd_p->release_head = NULL;
		WFD_IRQ_UNLOCK(wfdIdx, flags);

		while(head) {
			next = head->next;			
			head->recycle_hook(head, head->recycle_context, SKB_DATA_RECYCLE);
			head->recycle_hook(head, head->recycle_context, SKB_RECYCLE);
			head = next;
		}
	}
#endif /* CONFIG_TCH_KF_WLAN_PERF */
/* FRV: Direct release of buffers from wl to wfd */

		qMask = wfd_p->wfd_queue_mask;
		qidx = 0;
		while (qMask)
        {
		    if ((qMask & (1 << qidx)) == 0)
		    {
			   qidx++;
			   continue;
		    }

            qid = wfd_get_qid(qidx);

            local_bh_disable();

            rx_pktcnt = wfd_p->wfd_bulk_get(qid, num_packets_to_read, wfd_p, rx_pkts);

            gs_count_rx_pkt[qidx] += rx_pktcnt;

            local_bh_enable();

            if (wfd_queue_not_empty(qid, qidx))
            {
/* FRV BEGIN: Use same preemption method as in other RT threads (wlx-thread, bcmswrx, skbFreeTask) */
#if 1
                /* If this thread is running with Real Time priority, be nice
                 * to other threads by yielding the CPU after each batch of packets.
                 */
                if (current->policy == SCHED_FIFO || current->policy == SCHED_RR)
                        yield();
#else
                schedule();
#endif
/* FRV END: Use same preemption method as in other RT threads (wlx-thread, bcmswrx, skbFreeTask) */
            }
            else
            {
                /* Queue is empty: no more packets, enable interrupts */
                WFD_IRQ_LOCK(wfdIdx, flags);
                wfd_p->wfd_rx_work_avail &= (~(1 << qidx));
                WFD_IRQ_UNLOCK(wfdIdx, flags);
                wfd_int_enable(qid, qidx);
            }

			qMask &= ~(1 << qidx);
			qidx++;
        } /*for pci queue*/

#if 0
        WFD_IRQ_LOCK(flags);
        if (wfd_rx_work_avail == 0)
        {
            for (qidx = 0; qidx < number_of_queues; qidx++)
            {
               qid = wfd_get_qid(qidx);

               //Enable interrupts
               wfd_int_enable(qid);
            }
        }
#endif
    }

    return 0;
}

/****************************************************************************/
/**                                                                        **/
/** Name:                                                                  **/
/**                                                                        **/
/**   wifi_mw_proc_read_func_conf                                          **/
/**                                                                        **/
/** Title:                                                                 **/
/**                                                                        **/
/**   wifi mw - proc read                                                  **/
/**                                                                        **/
/** Abstract:                                                              **/
/**                                                                        **/
/**   Procfs callback method.                                              **/
/**      Called when someone reads proc command                            **/
/**   using: cat /proc/wifi_mw                                             **/
/**                                                                        **/
/** Input:                                                                 **/
/**                                                                        **/
/**   page  -  Buffer where we should write                                **/
/**   start -  Never used in the kernel.                                   **/
/**   off   -  Where we should start to write                              **/
/**   count -  How many character we could write.                          **/
/**   eof   -  Used to signal the end of file.                             **/
/**   data  -  Only used if we have defined our own buffer                 **/
/**                                                                        **/
/** Output:                                                                **/
/**                                                                        **/
/**                                                                        **/
/****************************************************************************/
static int wifi_mw_proc_read_func_conf(char* page, char** start, off_t off, int count, int* eof, void* data)
{
    int wifi_index ;
    unsigned int count_rx_queue_packets_total=0 ;
    unsigned int count_tx_bridge_packets_total=0 ;
    int len = 0 ;

    page+=off;
    page[0]=0;

    for(wifi_index=0;wifi_index < WIFI_MW_MAX_NUM_IF;wifi_index++)
    {
        if( wifi_net_devices[wifi_index] != NULL )
        {
            len += sprintf((page+len),"WFD Registered Interface %d:%s\n",
                            wifi_index,wifi_net_devices[wifi_index]->name);
        }
    }

    /*RX-MW from WiFi queues*/
    for (wifi_index=0; wifi_index<WIFI_MW_MAX_NUM_IF; wifi_index++)
    {
        if (gs_count_rx_queue_packets[wifi_index]!=0)
        {
            count_rx_queue_packets_total += gs_count_rx_queue_packets[wifi_index] ;
            if (wifi_index==0)
            {
                len += sprintf((page+len), "RX-MW from WiFi queues      [WiFi %d] = %d\n", 
                    wifi_index, gs_count_rx_queue_packets[wifi_index]) ;
            }
            else
            {
                len += sprintf((page +len), "                            [WiFi %d] = %d\n", 
                    wifi_index, gs_count_rx_queue_packets[wifi_index]) ;
            }
        }
    }

    /*TX-MW to bridge*/
    for (wifi_index=0; wifi_index<WIFI_MW_MAX_NUM_IF; wifi_index++)
    {
        if ( gs_count_tx_packets[wifi_index]!=0)
        {
            count_tx_bridge_packets_total += gs_count_tx_packets[wifi_index] ;
            if (wifi_index == 0)
            {
                len += sprintf((page+len), "TX-MW to bridge             [WiFi %d] = %d\n", 
                    wifi_index, gs_count_tx_packets[wifi_index]) ;
            }
            else
            {
                len += sprintf((page+len ), "                            [WiFi %d] = %d\n",      
                    wifi_index, gs_count_tx_packets[wifi_index]) ;
            }
        }
    }

    for (wifi_index = 0 ; wifi_index < WFD_MAX_OBJECTS ;wifi_index++ )
    {
    	if (wfd_objects[wifi_index].wl_dev_p)
    	{
    		len += sprintf((page+len),"\nWFD Object %d",wifi_index);
    		len += sprintf((page+len), "\nwl_chained_counters       =%d", wfd_objects[wifi_index].wl_chained_packets) ;
    		len += sprintf((page+len), "\nwl_unchained_counters     =%d",
                wfd_objects[wifi_index].wl_unchained_packets) ;
    		len += sprintf((page+len), "\ncount_rx_pkt              =%d", gs_count_rx_pkt[wifi_index]) ;
    		len += sprintf((page+len), "\nno_bpm_buffers_error      =%d", gs_count_no_buffers[wifi_index]) ;
            len += sprintf((page+len), "\nInvalid SSID vector       =%d",
                gs_count_rx_invalid_ssid_vector[wifi_index]);
            len += sprintf((page+len), "\nNo WIFI interface         =%d", gs_count_rx_no_wifi_interface[wifi_index]);
#if (defined(CONFIG_BCM_RDPA)||defined(CONFIG_BCM_RDPA_MODULE))
    		len += sprintf((page+len), "\nRing Information:\n");
    		len += sprintf((page+len), "\tRing Base address = 0x%pK\n",wfd_rings[wifi_index].base );
    		len += sprintf((page+len), "\tRing Head address = 0x%pK\n",wfd_rings[wifi_index].head );
    		len += sprintf((page+len), "\tRing Head position = %d\n",wfd_rings[wifi_index].head - wfd_rings[wifi_index].base);
#endif
            wfd_objects[wifi_index].wl_chained_packets = 0;
            wfd_objects[wifi_index].wl_unchained_packets = 0;
    	}


    }

    len += sprintf((page+len), "\nRX-MW from WiFi queues      [SUM] = %d\n", count_rx_queue_packets_total) ;
    len += sprintf((page+len), "TX-MW to bridge             [SUM] = %d\n", count_tx_bridge_packets_total) ;

    memset(gs_count_rx_queue_packets, 0, sizeof(gs_count_rx_queue_packets));
    memset(gs_count_tx_packets, 0, sizeof(gs_count_tx_packets));
    memset(gs_count_no_buffers, 0, sizeof(gs_count_no_buffers));
    memset(gs_count_rx_pkt, 0, sizeof(gs_count_rx_pkt));
    memset(gs_count_rx_invalid_ssid_vector, 0, sizeof(gs_count_rx_invalid_ssid_vector));
    memset(gs_count_rx_no_wifi_interface, 0, sizeof(gs_count_rx_no_wifi_interface));

    *eof = 1;
    return len;
}


/****************************************************************************/
/**                                                                        **/
/** Name:                                                                  **/
/**                                                                        **/
/**   wifi_proc_init.                                                      **/
/**                                                                        **/
/** Title:                                                                 **/
/**                                                                        **/
/**   wifi mw - proc init                                                  **/
/**                                                                        **/
/** Abstract:                                                              **/
/**                                                                        **/
/**   The function initialize the proc entry                               **/
/**                                                                        **/
/** Input:                                                                 **/
/**                                                                        **/
/** Output:                                                                **/
/**                                                                        **/
/**                                                                        **/
/****************************************************************************/
static int wifi_proc_init(void)
{
    /* make a directory in /proc */
    proc_directory = proc_mkdir("wfd", NULL) ;

    if (proc_directory==NULL) goto fail_dir ;

    /* make conf file */
    proc_file_conf = create_proc_entry("stats", 0444, proc_directory) ;

    if (proc_file_conf==NULL ) goto fail_entry ;

    /* set callback function on file */
    proc_file_conf->read_proc = wifi_mw_proc_read_func_conf ;

    return (0) ;

fail_entry:
    printk("%s %s: Failed to create proc entry in wifi_mw\n", __FILE__, __FUNCTION__);
    remove_proc_entry("wfd" ,NULL); /* remove already registered directory */

fail_dir:
    printk("%s %s: Failed to create directory wifi_mw\n", __FILE__, __FUNCTION__) ;
    return (-1) ;
}



/****************************************************************************/
/**                                                                        **/
/** Name:                                                                  **/
/**                                                                        **/
/**   release_wfd_os_resources                                             **/
/**                                                                        **/
/** Title:                                                                 **/
/**                                                                        **/
/** Abstract:                                                              **/
/**                                                                        **/
/**   The function releases the OS resources                               **/
/**                                                                        **/
/** Input:                                                                 **/
/**                                                                        **/
/** Output:                                                                **/
/**                                                                        **/
/**   bool - Error code:                                                   **/
/**             true - No error                                            **/
/**             false - Error                                              **/
/**                                                                        **/
/****************************************************************************/
static int release_wfd_os_resources(void)
{
    int idx;
    /* stop kernel thread */
    for (idx = 0; idx < WFD_MAX_OBJECTS; idx++)
    {
        if (wfd_objects[idx].wfd_rx_thread)
            kthread_stop(wfd_objects[idx].wfd_rx_thread);
    }

#if defined(CONFIG_BCM_RDPA) || defined(CONFIG_BCM_RDPA_MODULE)
    bdmf_destroy(rdpa_cpu_obj);
#endif
    return (0) ;
}



/****************************************************************************/
/**                                                                        **/
/** Name:                                                                  **/
/**                                                                        **/
/**   wfd_dev_close                                                        **/
/**                                                                        **/
/** Title:                                                                 **/
/**                                                                        **/
/**   wifi accelerator - close                                             **/
/**                                                                        **/
/** Abstract:                                                              **/
/**                                                                        **/
/**   The function closes all the driver resources.                        **/
/**                                                                        **/
/** Input:                                                                 **/
/**                                                                        **/
/** Output:                                                                **/
/**                                                                        **/
/**                                                                        **/
/****************************************************************************/
static void wfd_dev_close(void)
{
    int qid, qidx;

    if (mcast_instance_idx != -1)
    {
        wfd_unbind(mcast_instance_idx, WFD_WL_FWD_HOOKTYPE_SKB);
        mcast_instance_idx = -1;
    }

    /* Disable the interrupt */
    for (qidx = 0; qidx < number_of_queues; qidx++)
    {
        qid = wfd_get_qid(qidx);

        /* Deconfigure PCI RX queue */
        wfd_config_rx_queue(qid, 0, WFD_WL_FWD_HOOKTYPE_SKB, 0, 0);
      
	    /* interrupt mask only */
        wfd_int_disable (qid, qidx); 
    }
     
    /* Release the OS driver resources */
    release_wfd_os_resources();

    remove_proc_entry("stats", proc_directory);

    remove_proc_entry("wfd", NULL);

#if defined(CONFIG_BCM96838)
    /*Free PCI resources*/
    release_wfd_interfaces();
#endif
}


/****************************************************************************/
/**                                                                        **/
/** Name:                                                                  **/
/**                                                                        **/
/**   wfd_dev_init                                                         **/
/**                                                                        **/
/** Title:                                                                 **/
/**                                                                        **/
/**   wifi accelerator - init                                              **/
/**                                                                        **/
/** Abstract:                                                              **/
/**                                                                        **/
/**   The function initialize all the driver resources.                    **/
/**                                                                        **/
/** Input:                                                                 **/
/**                                                                        **/
/** Output:                                                                **/
/**                                                                        **/
/**                                                                        **/
/****************************************************************************/
static int wfd_dev_init(void)
{
    int idx;

/* FRV: Direct release of buffers from wl to wfd */                        
#ifdef CONFIG_TCH_KF_WLAN_PERF
	bcm63xx_wfd_recycle_hook = (RecycleFuncP)wfd_buf_recycle;
	wfd_skbuff_head_cache = kmem_cache_create("wfd_skbuff_head_cache",
	                                          sizeof(struct sk_buff),
	                                          0,
	                                          SLAB_HWCACHE_ALIGN|SLAB_PANIC,
	                                          NULL);
#endif /* CONFIG_TCH_KF_WLAN_PERF */
/* FRV: Direct release of buffers from wl to wfd */ 

	if (num_packets_to_read > NUM_PACKETS_TO_READ_MAX)
	{
        printk("%s %s Invalid num_packets_to_read %d\n",__FILE__, __FUNCTION__, num_packets_to_read);    
        return -1;
	}

	for (idx = 0; idx < WFD_MAX_OBJECTS; idx++)
	{
	   spin_lock_init(&wfd_irq_lock[idx]);
	}

    /* Initialize the proc interface for debugging information */
    if (wifi_proc_init()!=0)
    {
        printk("\n%s %s: wifi_proc_init() failed\n", __FILE__, __FUNCTION__) ;
        goto proc_release;
    }

    /* Queues will be initialized during wfd_bind() */
    number_of_queues = 0;

    /* Initialize accelerator(Runner/FAP) specific data structures, Queues */
    if (wfd_accelerator_init() != 0)
    {
        printk("%s %s: wfd_platform_init() failed\n", __FILE__, __FUNCTION__) ;
        goto proc_release;    
    }

    wfd_dump_fn = wfd_dump;
#ifdef CONFIG_BCM96838
    /*Bind instance 0 for mcast support */
    mcast_instance_idx = wfd_bind(NULL, WFD_WL_FWD_HOOKTYPE_SKB, false, NULL, 0, NULL, NULL);
    if (mcast_instance_idx == -1)
    {
        printk("%s %s: wfd_bind() failed to bind mcast queue\n", __FILE__, __FUNCTION__) ;
        goto cleanup;    
    }

    {
        bdmf_object_handle tc_to_q_obj = NULL;
        BDMF_MATTR(qos_mattrs, rdpa_tc_to_queue_drv());
        rdpa_tc_to_queue_table_set(qos_mattrs, 0);
        rdpa_tc_to_queue_dir_set(qos_mattrs, rdpa_dir_ds);
        if (bdmf_new_and_set(rdpa_tc_to_queue_drv(), NULL, qos_mattrs, &tc_to_q_obj))
        {
            printk("%s %s: bdmf_new_and_set tc_to_queue obj failed\n", __FILE__, __FUNCTION__);
            goto cleanup;
        }

        if (rdpa_tc_to_queue_tc_map_set(tc_to_q_obj, 0, WFD_MCAST_QUEUE_IDX))
        {
            printk("%s %s: rdpa_tc_to_queue_tc_map_set failed, q(0)\n", __FILE__, __FUNCTION__);
            goto cleanup; 
        }
    }

#endif

    printk("\033[1m\033[34m%s is initialized!\033[0m\n", version);
        
    return 0;

#ifdef CONFIG_BCM96838
cleanup:
    wfd_dump_fn = NULL;
#endif

proc_release:
    remove_proc_entry("stats", proc_directory);
    remove_proc_entry("wfd", NULL);

    return -1;
}

/****************************************************************************/
/**                                                                        **/
/** Name:                                                                  **/
/**                                                                        **/
/**   wfd_bind                                                             **/
/**                                                                        **/
/** Title:                                                                 **/
/**                                                                        **/
/** Abstract:                                                              **/
/**                                                                        **/
/**   Bind the function hooks and other attributes that are needed by      **/
/**   wfd to forward packets to WLAN.                                      **/
/**                                                                        **/
/** Input:                                                                 **/
/**                                                                        **/
/** Output:                                                                **/
/**                                                                        **/
/**                                                                        **/
/****************************************************************************/
int wfd_bind(struct net_device *wl_dev_p, 
             enumWFD_WlFwdHookType eFwdHookType, 
             bool isTxChainingReqd,
             HOOK4PARM wfd_fwdHook, 
             HOOK32 wfd_completeHook,
             HOOK3PARM *wfd_rxOffloadHook,
             HOOK2PARM wfd_rxLoopBackHook)
{
    int rc=0;
    int qid;
    int numQCreated=0;
    int qidx = number_of_queues;
    char threadname[15]={0};
    int tmp_idx = wfd_idx;

    if (wfd_idx >= WFD_MAX_OBJECTS - 1)
    {
        printk("%s ERROR. WFD_MAX_OBJECTS(%d) limit reached\n", __FUNCTION__, WFD_MAX_OBJECTS);
        return rc;
    }

    if (!wl_dev_p)
    {
        /*This bind is for mcast traffic use the dummy wfd instance */
        tmp_idx = WFD_MCAST_OBJECT_IDX;
        qidx = WFD_MCAST_QUEUE_IDX;
    }

    memset(&wfd_objects[tmp_idx], 0, sizeof(wfd_objects[tmp_idx]));

    wfd_objects[tmp_idx].wl_dev_p         = wl_dev_p;
    wfd_objects[tmp_idx].eFwdHookType     = eFwdHookType;
    wfd_objects[tmp_idx].isTxChainingReqd = isTxChainingReqd;

    if (eFwdHookType == WFD_WL_FWD_HOOKTYPE_SKB)
        wfd_objects[tmp_idx].wfd_bulk_get = wfd_bulk_skb_get;
    else
        wfd_objects[tmp_idx].wfd_bulk_get = wfd_bulk_fkb_get;

    wfd_objects[tmp_idx].wfd_fwdHook      = wfd_fwdHook;
    wfd_objects[tmp_idx].wfd_completeHook = wfd_completeHook;
    wfd_objects[tmp_idx].wl_chained_packets = 0;
    wfd_objects[tmp_idx].wl_unchained_packets = 0;
    wfd_objects[tmp_idx].wfd_acc_info_p  = wfd_acc_info_get();
    wfd_objects[tmp_idx].wfd_idx  = tmp_idx;
    wfd_objects[tmp_idx].wfd_rx_work_avail  = 0;
    wfd_objects[tmp_idx].wfd_rxLoopBackHook = wfd_rxLoopBackHook;

    if(wfd_rxOffloadHook != NULL)
    {
#if defined(CONFIG_BCM_WFD_RX_ACCELERATION)
        *wfd_rxOffloadHook = (HOOK3PARM)rdpa_cpu_tx_flow_cache_offload;
#else
        *wfd_rxOffloadHook = NULL;
#endif
    }

    sprintf(threadname,"wfd%d-thrd", tmp_idx);

    packet_threshold = WFD_WLAN_QUEUE_MAX_SIZE;

    /* Configure WFD RX queue */
    if (qidx < WFD_NUM_QUEUE_SUPPORTED)
    {
        qid = wfd_get_qid(qidx);

        if ((rc = wfd_config_rx_queue(qid, 
            packet_threshold,
            eFwdHookType,
            &numQCreated,
            &wfd_objects[tmp_idx].wfd_queue_mask)) != 0)
        {
            printk("%s %s: Cannot configure WFD CPU Rx queue (%d), status (%d)\n",
                __FILE__, __FUNCTION__, qid, rc);
            return rc;
        }

        wfd_int_enable (qid, qidx); 

        if (qidx != WFD_MCAST_QUEUE_IDX)
            number_of_queues += numQCreated;

        /* Create WFD Thread */
        init_waitqueue_head(&wfd_objects[tmp_idx].wfd_rx_thread_wqh);
        wfd_objects[tmp_idx].wfd_rx_thread = kthread_create(wfd_tasklet_handler, (void *)tmp_idx, threadname);
        /* wlmngr manages the logic to bind the WFD threads to specific CPUs depending on platform
           Look at function wlmngr_setupTPs() for more details */
        //kthread_bind(wfd_objects[tmp_idx].wfd_rx_thread, tmp_idx);
        wake_up_process(wfd_objects[tmp_idx].wfd_rx_thread);

        printk("\033[1m\033[34m %s: Dev %s wfd_idx %d Type %s configured WFD thread %s "
            "RxQId (%d), status (%d) number_of_queues %d qmask 0x%x\033[0m\n",
            __FUNCTION__, wl_dev_p->name, tmp_idx, 
            ((eFwdHookType == WFD_WL_FWD_HOOKTYPE_SKB) ? "skb" : "fkb"), threadname,
            qid, rc, number_of_queues, wfd_objects[tmp_idx].wfd_queue_mask);
    }
    else
    {
        printk("%s: ERROR qidx %d numq %d maxq %d\n", __FUNCTION__, 
            (int)qidx, (int)number_of_queues, (int)WFD_NUM_QUEUE_SUPPORTED);
    }

    if (tmp_idx != WFD_MCAST_OBJECT_IDX)
        wfd_idx++;
    return (tmp_idx);
}
EXPORT_SYMBOL(wfd_bind);

void wfd_unbind(int idx, enumWFD_WlFwdHookType hook_type)
{
    int qidx, qid, n = 0;
    int numQCreated;
	int tempQMask;

    // simple reclaim iff idx of last bind
    if (idx != wfd_idx - 1)
    {
        printk("%s idx %d wfd_idx %d\n", __func__, idx, wfd_idx);
        return;
    }

    /* free the pci rx queue(s); disable the interrupt(s) */
    do {
        /* Deconfigure PCI RX queue(s) */
        qidx = --number_of_queues;
        qid = wfd_get_qid(qidx);
        wfd_config_rx_queue(qid, 0, hook_type, &numQCreated, &tempQMask);
        wfd_objects[idx].wfd_queue_mask &= ~(1 << qidx);
        wfd_int_disable(qid, qidx);
	// loop if numQCreated > 1
    } while (++n < numQCreated);

    wfd_idx = idx;
    memset(&wfd_objects[idx], 0, sizeof wfd_objects[idx]);
}
EXPORT_SYMBOL(wfd_unbind);

static void wfd_dump(void)
{
    unsigned long flags;
	int idx;
	
	for (idx = 0; idx < WFD_MAX_OBJECTS; idx++)
	{
	   WFD_IRQ_LOCK(idx, flags);
	   printk("wfd_rx_work_avail 0x%x qmask 0x%x number_of_queues %d\n", 
			  wfd_objects[idx].wfd_rx_work_avail, wfd_objects[idx].wfd_queue_mask, number_of_queues);
	   WFD_IRQ_UNLOCK(idx, flags);
	}
}

/* FRV: Direct release of buffers from wl to wfd */
#ifdef CONFIG_TCH_KF_WLAN_PERF
void wfd_release_fast_queue(int idx, struct sk_buff *skb)
{
	wfd_object_t *wfd_p = &wfd_objects[idx];
	unsigned long flags;

	WFD_IRQ_LOCK(idx, flags);
	skb->next = wfd_p->release_head;
	wfd_p->release_head = skb;
	WFD_IRQ_UNLOCK(idx, flags);
}
EXPORT_SYMBOL(wfd_release_fast_queue);

void wfd_release_fast_schedule(int idx)
{
	wfd_object_t *wfd_p = &wfd_objects[idx];

	if (wfd_p->release_head && wfd_p->wfd_rx_thread->state != TASK_RUNNING) {  
		WFD_WAKEUP_RXWORKER(idx);
	}
}
EXPORT_SYMBOL(wfd_release_fast_schedule);
#endif /* CONFIG_TCH_KF_WLAN_PERF */
/* FRV: Direct release of buffers from wl to wfd */

MODULE_DESCRIPTION("WLAN Forwarding Driver");
MODULE_AUTHOR("Broadcom");
MODULE_LICENSE("GPL");

module_init(wfd_dev_init);
module_exit(wfd_dev_close);
