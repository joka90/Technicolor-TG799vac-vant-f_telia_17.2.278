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

#include "bcm_OS_Deps.h"
#include <linux/bcm_log.h>
#include <rdpa_api.h>
#include <rdpa_epon.h>
#include <autogen/rdpa_ag_epon.h>
#include <linux/blog_rule.h>
#include "boardparms.h"
#include "board.h"
#include "clk_rst.h"
#include "wan_drv.h"

/* init system params */
#define EPON_SPEED_NORMAL "Normal"
#define EPON_SPEED_TURBO  "Turbo"
#define BP_NO_EXT_SW 30
extern int sysinit;
static int emac_map = 0;
static int emacs_num = 0;
static int ext_sw_pid = BP_NO_EXT_SW;
#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
    char *wan_default_type = "DSL";
#elif defined(WAN_GBE)
    char *wan_default_type = "GBE";
#else
    char *wan_default_type = "GPON";
#endif
#if !defined(CONFIG_BCM963138) && !defined(CONFIG_BCM963148)
static char *wan_oe_default_emac = "EMAC0";
static char wan_type_buf[PSP_BUFLEN_16]={0};
static char gbe_emac_buf[PSP_BUFLEN_16]={0};
static char epon_speed_buf[PSP_BUFLEN_16]={0};
char *epon_default_speed = EPON_SPEED_NORMAL;
#endif


#define base(x) ((x >= '0' && x <= '9') ? '0' : \
    (x >= 'a' && x <= 'f') ? 'a' - 10 : \
    (x >= 'A' && x <= 'F') ? 'A' - 10 : \
    '\255')

#define TOHEX(x) (x - base(x))
#define TM_BASE_ADDR_STR    "tm"
#define TM_MC_BASE_ADDR_STR "mc"

#if !defined(CONFIG_BCM963138) && !defined(CONFIG_BCM963148)
static rdpa_system_init_cfg_t init_cfg = {};
static rdpa_system_cfg_t sys_cfg = {};
static bdmf_object_handle system_obj = NULL;
#endif

static int rdpa_get_init_system_bp_params(void)
{
    int rc = 0;
    int i, cnt;
    const ETHERNET_MAC_INFO* EnetInfos;
    EnetInfos = BpGetEthernetMacInfoArrayPtr();
    if (EnetInfos == NULL)
        return rc;

    emac_map = EnetInfos[0].sw.port_map & 0xFF;
    for (i = 0; i < BP_MAX_SWITCH_PORTS; ++i)
    {
        if (((1<<i) & emac_map) && ((int)EnetInfos[0].sw.phy_id[i] & EXTSW_CONNECTED))
            ext_sw_pid = i;
    }
    if (EnetInfos[0].ucPhyType != BP_ENET_NO_PHY)
    {
        bitcount(cnt, EnetInfos[0].sw.port_map);
        emacs_num += cnt;
    }

    return rc;
}

#if !defined(CONFIG_BCM963138) && !defined(CONFIG_BCM963148)
static int rdpa_get_init_wan_type_spad_param(void)
{
#if defined(CONFIG_TECHNICOLOR_GPON_PATCH)
    strcpy(wan_type_buf,wan_default_type);

	return 0;
#else
    int rc = 0;
    int count;

    count = kerSysScratchPadGet((char*)RDPA_WAN_TYPE_PSP_KEY, (char*)wan_type_buf, (int)sizeof(wan_type_buf));
    if (count == 0)
    {
        rc = kerSysScratchPadSet(RDPA_WAN_TYPE_PSP_KEY, wan_default_type, strlen(wan_default_type));
        if (rc)
        {
            printk("Could not set PSP %s to %s, rc=%d",
                    RDPA_WAN_TYPE_PSP_KEY, wan_default_type, rc);
            return rc;
        }
        strcpy(wan_type_buf,wan_default_type);
    }
    else
    {
        printk("kerSysScratchPadGet wan type - %s \n", wan_type_buf);
    }

    return rc;
#endif
}

static int rdpa_set_init_wanoe_emac_spad_param(char *emacid)
{
#if defined(CONFIG_TECHNICOLOR_GPON_PATCH)
    strcpy(gbe_emac_buf,emacid);

    return 0;
#else
    int rc = 0;

    rc = kerSysScratchPadSet(RDPA_WAN_OEMAC_PSP_KEY, emacid, strlen(emacid));
    if (rc)
    {
        printk("Could not set PSP %s to %s, rc=%d (reboot canceled)",
                RDPA_WAN_OEMAC_PSP_KEY, emacid, rc);
        return rc;
    }
    strcpy(gbe_emac_buf,emacid);

    return rc;
#endif
}

static int rdpa_get_init_wanoe_emac_spad_param(void)
{
#if defined(CONFIG_TECHNICOLOR_GPON_PATCH)
    strcpy(gbe_emac_buf,wan_oe_default_emac);

	return 0;
#else
    int rc = 0;
    int count;

    count = kerSysScratchPadGet(RDPA_WAN_OEMAC_PSP_KEY, gbe_emac_buf, sizeof(gbe_emac_buf));
    if (count == 0)
    {
        rc = kerSysScratchPadSet(RDPA_WAN_OEMAC_PSP_KEY, wan_oe_default_emac, strlen(wan_oe_default_emac));
        if (rc)
        {
            printk("Could not set PSP %s to %s, rc=%d (reboot canceled)",
                    RDPA_WAN_OEMAC_PSP_KEY, wan_oe_default_emac, rc);
            return rc;
        }
        strcpy(gbe_emac_buf,wan_oe_default_emac);
    }
    else
    {
        printk("kerSysScratchPadGet wan emac - %s \n", gbe_emac_buf);
    }

    return rc;
#endif
}

static int rdpa_get_init_epon_speed_spad_param(void)
{
#if defined(CONFIG_TECHNICOLOR_GPON_PATCH)
    strcpy(epon_speed_buf,epon_default_speed);

    return 0;
#else
    int rc = 0;
    int count;

    count = kerSysScratchPadGet((char*)RDPA_EPON_SPEED_PSP_KEY, (char*)epon_speed_buf, (int)sizeof(epon_speed_buf));
    if (count == 0)
    {
        rc = kerSysScratchPadSet(RDPA_EPON_SPEED_PSP_KEY, epon_default_speed, strlen(epon_default_speed));
        if (rc)
        {
            printk("Could not set PSP %s to %s, rc=%d",
                    RDPA_EPON_SPEED_PSP_KEY, epon_default_speed, rc);
            return rc;
        }
        strcpy(epon_speed_buf,epon_default_speed);
    }
    else
    {
        printk("kerSysScratchPadGet epon speed - %s \n", epon_speed_buf);
    }

    return rc;
#endif
}

static int rdpa_gpon_car_mode_cfg(void)
{
    int rc = 0;
    rc = rdpa_system_get(&system_obj);
    if(rc)
    {
        printk("%s %s Failed to get RDPA System object rc(%d)\n", __FILE__, __FUNCTION__, rc);
        goto exit;
    }
    else
    {
        rc = rdpa_system_cfg_get(system_obj, &sys_cfg);
        if(rc)
        {
            printk("Failed to getting RDPA System cfg\n");
            goto exit;
        }
    }

    sys_cfg.car_mode = 1;
    rc = rdpa_system_cfg_set(system_obj, &sys_cfg);
    if(rc)
    {
        printk("%s %s Failed to set RDPA System car mode rc(%d)\n", __FILE__, __FUNCTION__, rc);
        goto exit;
    }
    bdmf_put(system_obj);

exit:
    if (rc && system_obj)
        bdmf_put(system_obj);
    return rc;
}

static int rdpa_epon_object_init(void)
{
    BDMF_MATTR(rdpa_epon_attrs, rdpa_epon_drv());
    bdmf_object_handle rdpa_epon_obj;
    int rc = 0;

    rc = rdpa_system_get(&system_obj);
    if(rc)
    {
        printk("%s %s Failed to get RDPA System object rc(%d)\n", __FILE__, __FUNCTION__, rc);
        goto exit;
    }
    else
    {
        rc = rdpa_system_cfg_get(system_obj, &sys_cfg);
        if(rc)
        {
            printk("Failed to getting RDPA System cfg\n");
            goto exit;
        }
        rc = rdpa_system_init_cfg_get(system_obj, &init_cfg);
        if(rc)
        {
            printk("Failed to getting RDPA System init cfg\n");
            goto exit;
        }
    }

    if (init_cfg.ip_class_method == rdpa_method_fc)
    {
        sys_cfg.car_mode = 1;

    } else
    {
        sys_cfg.car_mode = 0;
    }

    rc = rdpa_system_cfg_set(system_obj, &sys_cfg);
    if(rc)
    {
        printk("Failed to set RDPA System car mode \n");
        goto exit;
    }
    bdmf_put(system_obj);

    rc = bdmf_new_and_set(rdpa_epon_drv(), NULL, rdpa_epon_attrs, &rdpa_epon_obj);
    if (rc)
        printk("%s %s Failed to create RDPA epon object rc(%d)\n", __FILE__, __FUNCTION__, rc);

exit:
    if (rc && system_obj)
        bdmf_put(system_obj);
    if (rc && rdpa_epon_obj)
        bdmf_put(rdpa_epon_obj);
    return rc;
}

static int rdpa_iptv_object_init(void)
{
    BDMF_MATTR(rdpa_iptv_attrs, rdpa_iptv_drv());
    bdmf_object_handle rdpa_iptv_obj;
    int rc = 0;

    rc = rdpa_system_get(&system_obj);
    if(!rc)
    {
        rc = rdpa_system_cfg_get(system_obj, &sys_cfg);
        if(rc)
        {
            printk("Failed to getting RDPA System cfg\n");
            goto exit;
        }
    }
    sys_cfg.headroom_size = 0;
    rc = rdpa_system_cfg_set(system_obj, &sys_cfg);
    if(rc)
    {
        printk("Failed to set RDPA System hdr size 0\n");
        goto exit;
    }

    bdmf_put(system_obj);

    rdpa_iptv_lookup_method_set(rdpa_iptv_attrs, iptv_lookup_method_group_ip_src_ip);
    rc = bdmf_new_and_set(rdpa_iptv_drv(), NULL, rdpa_iptv_attrs, &rdpa_iptv_obj);

exit:
    if (rc && system_obj)
        bdmf_put(system_obj);
    if (rc && rdpa_iptv_obj)
        bdmf_put(rdpa_iptv_obj);
    return rc;
}
#endif

#if !defined(CONFIG_BCM963138) && !defined(CONFIG_BCM963148)
int rdpa_init_system(void)
{
    BDMF_MATTR(rdpa_system_attrs, rdpa_system_drv());
    bdmf_object_handle rdpa_system_obj = NULL;
    bdmf_object_handle rdpa_filter_obj = NULL;
    BDMF_MATTR(rdpa_filter_attrs, rdpa_filter_drv());
#if defined(G9991) 
    uint32_t fttdp_addr, fttdp_val;
#endif
    int rc;
    rdpa_system_init_cfg_t sys_init_cfg = {};
    rdpa_epon_speed_mode epon_speed = rdpa_epon_speed_1g1g;

    rc = rdpa_get_init_system_bp_params();
    if (rc)
        goto exit;

    rc = rdpa_get_init_wan_type_spad_param();
    if (rc)
        goto exit;
    rc = rdpa_get_init_epon_speed_spad_param();
    if (rc)
        goto exit;
    if (!strcmp(wan_type_buf ,"GBE"))
    {
        sys_init_cfg.wan_type = rdpa_wan_gbe;
        rc = rdpa_get_init_wanoe_emac_spad_param();

        if (rc)
            goto exit;
        if (!strncmp(gbe_emac_buf ,"EMAC",4) && (strlen(gbe_emac_buf) == strlen("EMACX")))
            sys_init_cfg.gbe_wan_emac = (rdpa_emac)(TOHEX(gbe_emac_buf[4]));
        else
        {
            printk("%s %s Wrong EMAC string in ScrachPad - ###(%s)###\n", __FILE__, __FUNCTION__, gbe_emac_buf);
            rc = -1;
            goto exit;
        }
    }
    /* saved for backward compatibility */
    else if (!strcmp(wan_type_buf ,"AE"))
    {
        sys_init_cfg.wan_type = rdpa_wan_gbe;
        rc = rdpa_set_init_wanoe_emac_spad_param("EMAC5");
        if (rc)
            goto exit;
        sys_init_cfg.gbe_wan_emac = rdpa_emac5;
    }
    else if (!strcmp(wan_type_buf ,"GPON"))
        sys_init_cfg.wan_type = rdpa_wan_gpon;
    else if (!strcmp(wan_type_buf ,"EPON"))
        sys_init_cfg.wan_type = rdpa_wan_epon;

    if (sys_init_cfg.wan_type == rdpa_wan_epon)
    {
        if (!strcmp(epon_speed_buf, EPON_SPEED_TURBO))
        {
            epon_speed = rdpa_epon_speed_2g1g;
        }
        else if (!strcmp(epon_speed_buf, EPON_SPEED_NORMAL))
        {
            epon_speed = rdpa_epon_speed_1g1g;
        }
    }
    
    sys_init_cfg.enabled_emac = emac_map;

    /*disable emac5 from portmap in case we are not in GBE_ON_EMAC5/AE mode*/
    if (!(sys_init_cfg.wan_type == rdpa_wan_gbe && sys_init_cfg.gbe_wan_emac == rdpa_emac5))
    {
        sys_init_cfg.enabled_emac &= ~(1<<rdpa_emac5);
    }
    sys_init_cfg.runner_ext_sw_cfg.emac_id = (ext_sw_pid == BP_NO_EXT_SW) ? rdpa_emac_none:(rdpa_emac)ext_sw_pid;
    sys_init_cfg.runner_ext_sw_cfg.enabled = (ext_sw_pid == BP_NO_EXT_SW) ? 0 : 1;
#if defined(G9991)
    sys_init_cfg.runner_ext_sw_cfg.enabled = 1;
    sys_init_cfg.runner_ext_sw_cfg.type = rdpa_brcm_fttdp;
#else
    sys_init_cfg.runner_ext_sw_cfg.type = rdpa_brcm_hdr_opcode_0;
#endif

    /* when system wan type is known we shall configure the WAN serdes
     * before creating system object
     */
    /*first we call the basic data path initialization*/
    switch (sys_init_cfg.wan_type)
    {
    case rdpa_wan_gpon:
        ConfigWanSerdes(SERDES_WAN_TYPE_GPON, epon_speed);
        break;
    case rdpa_wan_epon:
        ConfigWanSerdes(SERDES_WAN_TYPE_EPON, epon_speed);
        break;
    case rdpa_wan_gbe:
        if (sys_init_cfg.gbe_wan_emac == rdpa_emac5)
        {
            ConfigWanSerdes(SERDES_WAN_TYPE_AE, epon_speed);
        }
        break;
    default:
        break;
    }

    sys_init_cfg.switching_mode = rdpa_switching_none;
#if defined(CONFIG_EPON_SFU)
    if (sys_init_cfg.wan_type == rdpa_wan_epon)
        sys_init_cfg.switching_mode = rdpa_mac_based_switching;
#endif

#if defined(CONFIG_GPON_SFU) || defined(CONFIG_EPON_SFU)
    sys_init_cfg.ip_class_method = rdpa_method_none;
#else
    sys_init_cfg.ip_class_method = rdpa_method_fc;
#endif

    rdpa_system_init_cfg_set(rdpa_system_attrs, &sys_init_cfg);
    rc = bdmf_new_and_set(rdpa_system_drv(), NULL, rdpa_system_attrs, &rdpa_system_obj);
    if (rc)
    {
        printk("%s %s Failed to create rdpa system object rc(%d)\n", __FILE__, __FUNCTION__, rc);
        goto exit;
    }

    if (sys_init_cfg.wan_type == rdpa_wan_gpon)
    {
        rc = rdpa_gpon_car_mode_cfg();
        if (rc)
        {
            printk("%s %s Failed to configure rdpa gpon car mode rc(%d)\n", __FILE__, __FUNCTION__, rc);
            goto exit;
        }
    }

    if (sys_init_cfg.wan_type == rdpa_wan_epon)
    {
        rc = rdpa_epon_object_init();
        if (rc)
        {
            printk("%s %s Failed to create rdpa epon object rc(%d)\n", __FILE__, __FUNCTION__, rc);
            goto exit;
        }
    }
    if (sys_init_cfg.wan_type == rdpa_wan_gpon && sys_init_cfg.ip_class_method == rdpa_method_none)
    {
        rc = rdpa_iptv_object_init();
        if (rc)
        {
            printk("%s %s Failed to create rdpa iptv object rc(%d)\n", __FILE__, __FUNCTION__, rc);
            goto exit;
        }
    }

    /*Create Filter object for non-DSL platforms*/
    rc = bdmf_new_and_set(rdpa_filter_drv(), NULL, rdpa_filter_attrs, &rdpa_filter_obj);
    if (rc)
    {
        printk("%s %s Failed to create rdpa filter object rc(%d)\n", __FILE__, __FUNCTION__, rc);
        goto exit;
    }

exit:
    if (rc && rdpa_system_obj)
        bdmf_destroy(rdpa_system_obj);
    if (rc && rdpa_filter_obj)
        bdmf_destroy(rdpa_filter_obj);

//TODO:Remove the FTTD runner code when ready
#if defined(G9991)
    fttdp_addr = 0xb30d1818;
    fttdp_val = 0x19070019;
    WRITE_32(fttdp_addr, fttdp_val);
    fttdp_addr = 0xb30e1018;
    fttdp_val = 0x00002c2c;
    WRITE_32(fttdp_addr, fttdp_val);
    fttdp_addr = 0xb30e101c;
    fttdp_val = 0x00001013;
    WRITE_32(fttdp_addr, fttdp_val);
    fttdp_addr = 0xb30e1020;
    fttdp_val = 0x00001919;
    WRITE_32(fttdp_addr, fttdp_val);
#endif
    return rc;
}

#else /* CONFIG_BCM963138 || CONFIG_BCM963148 */

int rdpa_init_system(void)
{
    BDMF_MATTR(rdpa_system_attrs, rdpa_system_drv());
    bdmf_object_handle rdpa_system_obj = NULL;
    int rc;
    rdpa_system_init_cfg_t sys_init_cfg = {};
    bdmf_object_handle rdpa_port_object = NULL;
    BDMF_MATTR(rdpa_port_attrs, rdpa_port_drv());
    rdpa_port_dp_cfg_t port_cfg = {};

    rc = rdpa_get_init_system_bp_params();
    if (rc)
        goto exit;

    /* wan_type is set to the rdpa_if_wan0 type but it is not used to identify the WAN */
    sys_init_cfg.wan_type = rdpa_wan_gbe;
    sys_init_cfg.gbe_wan_emac = rdpa_emac0;
    sys_init_cfg.enabled_emac = emac_map;
    sys_init_cfg.runner_ext_sw_cfg.emac_id = (ext_sw_pid == BP_NO_EXT_SW) ? rdpa_emac_none:(rdpa_emac)ext_sw_pid;
    sys_init_cfg.runner_ext_sw_cfg.enabled = (ext_sw_pid == BP_NO_EXT_SW) ? 0 : 1;
    sys_init_cfg.runner_ext_sw_cfg.type = rdpa_brcm_hdr_opcode_0;
    sys_init_cfg.switching_mode = rdpa_switching_none;
    sys_init_cfg.ip_class_method = rdpa_method_fc;

    rdpa_system_init_cfg_set(rdpa_system_attrs, &sys_init_cfg);

    rc = bdmf_new_and_set(rdpa_system_drv(), NULL, rdpa_system_attrs, &rdpa_system_obj);
    if (rc)
    {
        printk("%s %s Failed to create rdpa system object rc(%d)\n", __FILE__, __FUNCTION__, rc);
        goto exit;
    }

    rdpa_port_index_set(rdpa_port_attrs, rdpa_if_wan0);
    port_cfg.emac = rdpa_emac0;
    rdpa_port_cfg_set(rdpa_port_attrs, &port_cfg);
    rc = bdmf_new_and_set(rdpa_port_drv(), NULL, rdpa_port_attrs, &rdpa_port_object);
    if (rc)
    {
        printk("%s %s Failed to create rdpa wan port rc(%d)\n", __FILE__, __FUNCTION__, rc);
        goto exit;
    }

exit:
    if (rc && rdpa_system_obj)
        bdmf_destroy(rdpa_system_obj);
    return rc;
}

#endif

