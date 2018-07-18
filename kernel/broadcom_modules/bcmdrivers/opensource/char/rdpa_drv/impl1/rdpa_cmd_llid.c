
/*
* <:copyright-BRCM:2013:DUAL/GPL:standard
*
*    Copyright (c) 2013 Broadcom Corporation
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

/*
 *******************************************************************************
 * File Name  : rdpa_cmd_llid.c
 *
 * Description: 
 *
 *******************************************************************************
 */

#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/bcm_log.h>
#include "bcmenet.h"
#include "bcmtypes.h"
#include "bcmnet.h"
#include "rdpa_types.h"
#include "rdpa_api.h"
#include "rdpa_ag_port.h"
#include "rdpa_drv.h"
#include "rdpa_cmd_llid.h"

#define __BDMF_LOG__

#define CMD_LLID_LOG_ID_RDPA_CMD_DRV BCM_LOG_ID_RDPA_CMD_DRV

#if defined(__BDMF_LOG__)
#define CMD_LLID_LOG_ERROR(fmt, args...)                                             \
    do {                                                                           \
        if (bdmf_global_trace_level >= bdmf_trace_level_error)                     \
            bdmf_trace("ERR: %s#%d: " fmt "\n", __FUNCTION__, __LINE__, ## args);  \
    } while(0)
#define CMD_LLID_LOG_INFO(fmt, args...)                                              \
    do {                                                                           \
        if (bdmf_global_trace_level >= bdmf_trace_level_info)                      \
            bdmf_trace("INF: %s#%d: " fmt "\n", __FUNCTION__, __LINE__, ## args);  \
    } while(0)
#define CMD_LLID_LOG_DEBUG(fmt, args...)                                             \
    do {                                                                           \
        if (bdmf_global_trace_level >= bdmf_trace_level_debug)                     \
            bdmf_trace("DBG: %s#%d: " fmt "\n", __FUNCTION__, __LINE__, ## args);  \
    } while(0)
#else
#define CMD_LLID_LOG_ERROR(fmt, arg...) BCM_LOG_ERROR(fmt, arg...)
#define CMD_LLID_LOG_INFO(fmt, arg...)  BCM_LOG_INFO(fmt, arg...)
#define CMD_LLID_LOG_DEBUG(fmt, arg...) BCM_LOG_DEBUG(fmt, arg...)
#endif

/*******************************************************************************/
/* static routines Functions                                                   */
/*******************************************************************************/



/*******************************************************************************/
/* global routines                                                             */
/*******************************************************************************/

/*******************************************************************************
 *
 * Function: rdpa_cmd_llid_ioctl
 *
 * IOCTL interface to the RDPA LLID API.
 *
 *******************************************************************************/
int rdpa_cmd_llid_ioctl(unsigned long arg)
{
    rdpa_drv_ioctl_llid_t *userLlid_p = (rdpa_drv_ioctl_llid_t *)arg;
    rdpa_drv_ioctl_llid_t llid;
    bdmf_error_t rc = BDMF_ERR_OK;
    int ret = 0;

    copy_from_user(&llid, userLlid_p, sizeof(rdpa_drv_ioctl_llid_t));

    CMD_LLID_LOG_DEBUG("RDPA LLID CMD(%d)", llid.cmd);

    bdmf_lock();

    switch(llid.cmd)
    {
        case RDPA_IOCTL_LLID_CMD_NEW: 
        {
            rdpa_tm_queue_cfg_t queue_cfg = {};
            bdmf_object_handle tmp_llid = NULL;
            bdmf_object_handle egress_tm = NULL;
    
            BDMF_MATTR(tm_mattrs, rdpa_egress_tm_drv());
            BDMF_MATTR(link_attrs, rdpa_llid_drv());

            rdpa_llid_index_set(link_attrs, llid.llid_index);
            if ((rc = bdmf_new_and_set(rdpa_llid_drv(), NULL, link_attrs, &tmp_llid))) 
            {
                CMD_LLID_LOG_ERROR("bdmf_new_and_set() llid failed: llid(%u) rc(%d)", llid.llid_index, rc);
                ret = RDPA_DRV_NEW_LLID_ALLOC;
                goto ioctl_exit;
            }
        
            rdpa_egress_tm_dir_set(tm_mattrs, rdpa_dir_us);
            rdpa_egress_tm_level_set(tm_mattrs, rdpa_tm_level_queue); 
            rdpa_egress_tm_mode_set(tm_mattrs, rdpa_tm_sched_sp);
            if ((rc = bdmf_new_and_set(rdpa_egress_tm_drv(), NULL, tm_mattrs, &egress_tm)))
            {
                CMD_LLID_LOG_ERROR("bdmf_new_and_set() tm failed: link(%u) rc(%d)", llid.llid_index, rc);
                ret = RDPA_DRV_NEW_TM_ALLOC;
                goto ioctl_exit;
            }
            
            memset(&queue_cfg, 0, sizeof(queue_cfg));
            queue_cfg.queue_id           = 101 + llid.llid_index;
            queue_cfg.drop_alg           = rdpa_tm_drop_alg_dt;
            queue_cfg.drop_threshold     = 256;
            queue_cfg.weight             = 0;
            queue_cfg.red_high_threshold = 0; 
            queue_cfg.red_low_threshold  = 0;

            if ((rc = rdpa_egress_tm_queue_cfg_set(egress_tm, 0, &queue_cfg)))
            {
                CMD_LLID_LOG_ERROR("rdpa_egress_tm_queue_cfg_set() failed: link(%u) rc(%d)", llid.llid_index, rc);
                ret = RDPA_DRV_Q_CFG_SET;
                goto ioctl_exit;
            } 
            if ((rc = rdpa_llid_control_egress_tm_set(tmp_llid, egress_tm)))
            {
                CMD_LLID_LOG_ERROR("rdpa_llid_control_egress_tm_set() failed: link(%u) rc(%d)", llid.llid_index, rc);
                ret = RDPA_DRV_LLID_CTRL_TM_SET;
                goto ioctl_exit;
            }    
            
            if((rc = rdpa_llid_control_enable_set(tmp_llid, 1)))
            {
                CMD_LLID_LOG_ERROR("rdpa_llid_control_enable_set() failed: link(%u) rc(%d)", llid.llid_index, rc);
                ret = RDPA_DRV_LLID_CTRL_EN_SET;
                goto ioctl_exit;
            }
            
            break;
        }
        
        default:
        {
            CMD_LLID_LOG_ERROR("Invalid IOCTL cmd %d", llid.cmd);
            ret = RDPA_DRV_ERROR;
            break;
        }
    }

ioctl_exit:

    bdmf_unlock();

    copy_to_user(userLlid_p, &llid, sizeof(rdpa_drv_ioctl_llid_t));

    return ret;
}

/*******************************************************************************
 *
 * Function: rdpa_cmd_llid_init
 *
 * Initializes the RDPA LLID API.
 *
 *******************************************************************************/
void rdpa_cmd_llid_init(void)
{
    CMD_LLID_LOG_DEBUG("RDPA LLID INIT");
}

EXPORT_SYMBOL(rdpa_cmd_llid_ioctl);
EXPORT_SYMBOL(rdpa_cmd_llid_init);
