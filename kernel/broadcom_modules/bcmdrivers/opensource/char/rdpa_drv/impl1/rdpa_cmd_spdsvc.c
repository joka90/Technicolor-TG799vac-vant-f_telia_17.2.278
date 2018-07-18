
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
 * File Name  : rdpa_cmd_spdsvc.c
 *
 * Description: This file contains the Runner Speed Service management API
 *
 *******************************************************************************
 */

#include <linux/module.h>
#include <linux/bcm_log.h>
#include "rdpa_types.h"
#include "rdpa_api.h"
#include "rdpa_drv.h"
#include "rdpa_cmd_spdsvc.h"

#if 1
#define CMD_SPDSVC_LOG_ERROR(fmt, args...)                                         \
    do {                                                                            \
        if (bdmf_global_trace_level >= bdmf_trace_level_error)                      \
            bdmf_trace("ERR: %s#%d: " fmt "\n", __FUNCTION__, __LINE__, ## args);    \
    } while(0)
#define CMD_SPDSVC_LOG_INFO(fmt, args...)                                         \
    do {                                                                            \
        if (bdmf_global_trace_level >= bdmf_trace_level_info)                          \
            bdmf_trace("INF: %s#%d: " fmt "\n", __FUNCTION__, __LINE__, ## args);    \
    } while(0)
#define CMD_SPDSVC_LOG_DEBUG(fmt, args...)                                         \
    do {                                                                            \
        if (bdmf_global_trace_level >= bdmf_trace_level_debug)                          \
            bdmf_trace("DBG: %s#%d: " fmt "\n", __FUNCTION__, __LINE__, ## args);    \
    } while(0)
#endif

static bdmf_object_handle spdsvc_class = NULL;

/*******************************************************************************/
/* global routines                                                             */
/*******************************************************************************/

/*******************************************************************************
 *
 * Function: rdpa_cmd_spdsvc_ioctl
 *
 * IOCTL interface to the RDPA SPDSVC API.
 *
 *******************************************************************************/
int rdpa_cmd_spdsvc_ioctl(unsigned long arg)
{
    rdpa_drv_ioctl_spdsvc_t *userSpdsvc_p = (rdpa_drv_ioctl_spdsvc_t *)arg;
    rdpa_drv_ioctl_spdsvc_t spdsvc;
    int ret = 0;

    copy_from_user(&spdsvc, userSpdsvc_p, sizeof(rdpa_drv_ioctl_spdsvc_t));

    CMD_SPDSVC_LOG_DEBUG("RDPA SPDSVC CMD: %d", spdsvc.cmd);

    bdmf_lock();

    switch(spdsvc.cmd)
    {
        case RDPA_IOCTL_SPDSVC_CMD_ENABLE:
        {
            CMD_SPDSVC_LOG_INFO("Runner Speed Service: Enable");

            ret = rdpa_spdsvc_get(&spdsvc_class);
            if(ret)
            {
                BDMF_MATTR(spdsvc_attrs, rdpa_spdsvc_drv());

                rdpa_spdsvc_local_ip_addr_set(spdsvc_attrs, (bdmf_ip_t *)&spdsvc.config.local_ip_addr);
                rdpa_spdsvc_local_port_nbr_set(spdsvc_attrs, spdsvc.config.local_port_nbr);
                rdpa_spdsvc_remote_ip_addr_set(spdsvc_attrs, (bdmf_ip_t *)&spdsvc.config.remote_ip_addr);
                rdpa_spdsvc_remote_port_nbr_set(spdsvc_attrs, spdsvc.config.remote_port_nbr);
                rdpa_spdsvc_direction_set(spdsvc_attrs, spdsvc.config.dir);

                ret = bdmf_new_and_set(rdpa_spdsvc_drv(), NULL, spdsvc_attrs, &spdsvc_class);
                if(ret)
                {
                    CMD_SPDSVC_LOG_ERROR("rdpa spdsvc_class object does not exist and can't be created.");
                }

                CMD_SPDSVC_LOG_DEBUG("%s,%u: spdsvc_class %p", __FUNCTION__, __LINE__, spdsvc_class);
            }
            else
            {
                CMD_SPDSVC_LOG_ERROR("Speed Service Enable: Already enabled");
                ret = -1;
            }
        }
        break;

        case RDPA_IOCTL_SPDSVC_CMD_GET_RESULT:
        {
            CMD_SPDSVC_LOG_INFO("Runner Speed Service: Get Result");

            if(spdsvc_class)
            {
                rdpa_spdsvc_result_t result;

                ret = rdpa_spdsvc_result_get(spdsvc_class, 0, &result);
                if(!ret)
                {
                    spdsvc.result.running = result.running;
                    spdsvc.result.rx_packets = result.rx_packets;
                    spdsvc.result.rx_bytes = result.rx_bytes;
                    spdsvc.result.tx_packets = result.tx_packets;
                    spdsvc.result.tx_bytes = result.tx_bytes;

                    copy_to_user(&userSpdsvc_p->result, &spdsvc.result, sizeof(rdpactl_spdsvc_result_t));
                }
                else
                {
                    CMD_SPDSVC_LOG_ERROR("Could not rdpa_spdsvc_result_get spdsvc_class %p, ret %d", spdsvc_class, ret);
                }
            }
            else
            {
                CMD_SPDSVC_LOG_ERROR("Speed Service Result: Not enabled");
                ret = -1;
            }
        }
        break;

        case RDPA_IOCTL_SPDSVC_CMD_DISABLE:
        {
            CMD_SPDSVC_LOG_INFO("Runner Speed Service: Disable");

            if(spdsvc_class)
            {
                bdmf_destroy(spdsvc_class);

                spdsvc_class = NULL;
            }
            else
            {
                CMD_SPDSVC_LOG_ERROR("Speed Service Disable: Not enabled");
                ret = -1;
            }
        }
        break;

        default:
        {
            CMD_SPDSVC_LOG_ERROR("Invalid Command: %d", spdsvc.cmd);
            ret = -1;
        }
    }

    bdmf_unlock();

    return ret;
}
EXPORT_SYMBOL(rdpa_cmd_spdsvc_ioctl);

/*******************************************************************************
 *
 * Function: rdpa_cmd_spdsvc_init
 *
 * RDPA SPDSVC initialization.
 *
 *******************************************************************************/
void rdpa_cmd_spdsvc_init(void)
{
    printk("RDPA Speed Service Command Driver\n");
}
EXPORT_SYMBOL(rdpa_cmd_spdsvc_init);
