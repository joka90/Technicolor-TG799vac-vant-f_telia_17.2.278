/*
    Copyright 2004-2013 Broadcom Corporation

    <:label-BRCM:2013:DUAL/GPL:standard
    
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

#ifndef _ETHSW_PWRMNGT_H_
#define _ETHSW_PWRMNGT_H_

int    ethsw_shutdown_unused_phys(void);
void   ethsw_setup_hw_apd(unsigned int enable);
int    ethsw_phy_pll_up(int ephy_and_gphy);
uint32 ethsw_ephy_auto_power_down_wakeup(void);
uint32 ethsw_ephy_auto_power_down_sleep(void);
void   ethsw_switch_manage_port_power_mode(int portnumber, int power_mode);
int    ethsw_switch_get_port_power_mode(int portnumber);

void   ethsw_eee_enable_phy(int log_port);
void   ethsw_eee_init(void);
void   ethsw_eee_port_enable(int port, int enable, int linkstate);
void   ethsw_eee_process_delayed_enable_requests(void);

void   extsw_apd_set_compatibility_mode(void);
void   ethsw_eee_init_hw(void);
void   extsw_eee_init(void);

int BcmPwrMngtGetEthAutoPwrDwn(void);
void BcmPwrMngtSetEthAutoPwrDwn(unsigned int enable);
void BcmPwrMngtSetEnergyEfficientEthernetEn(unsigned int enable);
int BcmPwrMngtGetEnergyEfficientEthernetEn(void);

void BcmPwrMngtSetAutoGreeEn(unsigned int enable);
int BcmPwrMngtGetAutoGreeEn(void);

#endif
