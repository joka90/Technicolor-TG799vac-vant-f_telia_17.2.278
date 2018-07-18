
/*
 Copyright 2004-2010 Broadcom Corp. All Rights Reserved.

 <:label-BRCM:2011:DUAL/GPL:standard    
 
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
#ifndef _BCMMII_XTN_H_
#define _BCMMII_XTN_H_

#define SF2_SWITCH_CONTROL_REG              0x40000UL
#define SF2_MDIO_MASTER                     0x01
#define SF2_IMP_2_5G_EN                     0x20 /* Only supported in 63138B0 */

/* These are needed for register accesses of length >4 bytes */

#define SF2_DIRECT_DATA_WRITE_REG           0x40008UL 
#define SF2_DIRECT_DATA_READ_REG            0x4000cUL
enum {
    SF2_P5  = 5,
    SF2_P7  = 7,
    SF2_P11 = 11,
    SF2_P12 = 12,
    SF2_INEXISTANT_PORT = 6,
};
//  SF2 P5 RGMII Control Register
#define SF2_P5_RGMII_CTRL_REGS              0x40070UL
#define SF2_P5_RGMII_RX_CLK_DELAY_CTRL      0x40078UL
//  SF2 P7 RGMII Control Register
#define SF2_P7_RGMII_CTRL_REGS              0x4007cUL
#define SF2_P7_RGMII_RX_CLK_DELAY_CTRL      0x40084UL
//  SF2 P11 RGMII Control Register
#define SF2_P11_RGMII_CTRL_REGS             0x400c8UL
#define SF2_P11_RGMII_RX_CLK_DELAY_CTRL     0x400d0UL
//  SF2 P12 RGMII Control Register
#define SF2_P12_RGMII_CTRL_REGS             0x400d4UL
#define SF2_P12_RGMII_RX_CLK_DELAY_CTRL     0x400dcUL
#define SF2_QUAD_PHY_BASE_REG               0x40024UL  // default quad phy adr base = 1 
#define SF2_QUAD_PHY_SYSTEM_RESET           0x100
#define SF2_QUAD_PHY_PHYAD_SHIFT            12

#define SF2_ENABLE_PORT_RGMII_INTF          0x01
#define SF2_TX_ID_DIS                       0x02
#define SF2_RGMII_PORT_MODE_M               0x1C
#define SF2_RGMII_PORT_MODE_S               0x2
  #define SF2_RGMII_PORT_MODE_INT_EPHY_MII      0x0 /* Internal EPHY (MII) */
  #define SF2_RGMII_PORT_MODE_INT_GPHY_GMII     0x1 /* Internal GPHY (GMII/MII) */
  #define SF2_RGMII_PORT_MODE_EXT_EPHY_MII      0x2 /* External EPHY (MII) */
  #define SF2_RGMII_PORT_MODE_EXT_GPHY_RGMII    0x3 /* External GPHY (RGMII) */
  #define SF2_RGMII_PORT_MODE_EXT_RvMII         0x4 /* External RvMII */
#define SF2_RX_ID_BYPASS                    0x20
#define SF2_MISC_MII_PAD_CTL                (MISC_BASE + 0x28)

// All Crossbar
#define CROSSBAR_SWITCH_REG                 0x400acUL

#if defined(CONFIG_5x3_CROSSBAR_SUPPORT) /* 63138B0 onwards 5x3 crossbar */
#define CB_PHY_PORT_MASK                    0x7
#define CB_PHY_PORT_SHIFT                   0x3
#define CB_WAN_LNK_STATUS_SHIFT             9
#define CB_WAN_LNK_STATUS_MASK              (1<<CB_WAN_LNK_STATUS_SHIFT)
#define CB_WAN_LNK_STATUS_SRC_SHIFT         10
#define CB_WAN_LNK_STATUS_SRC_MASK          (1<<CB_WAN_LNK_STATUS_SRC_SHIFT)
#else /* 4x2_CROSSBAR_SUPPORT */
#define CB_PHY_PORT_MASK                    0x3
#define CB_PHY_PORT_SHIFT                   0x2
#endif
#define SF2_MDIO_COMMAND_REG                0x403c0UL
#define SF2_MDIO_CONFIG_REG                 0x403c4UL
#define SF2_MDIO_BUSY                       (1 << 29)
#define SF2_MDIO_FAIL                       (1 << 28)
#define SF2_MDIO_CMD_M                      3
#define SF2_MDIO_CMD_S                      26
#define SF2_MDIO_CMD_C22_READ               2
#define SF2_MDIO_CMD_C22_WRITE              1
#define SF2_MDIO_C22_PHY_ADDR_M             0x1f
#define SF2_MDIO_C22_PHY_ADDR_S             21
#define SF2_MDIO_C22_PHY_REG_M              0x1f
#define SF2_MDIO_C22_PHY_REG_S              16
#define SF2_MDIO_PHY_DATA_M                 0xffff

#define SF2_IMP0_PORT                       8
#define SF2_WAN_IMP1_PORT                   5

#define SF2_ACB_CONTROL_REG                 0x40600UL
    #define SF2_ACB_EN                      1
#if defined(ACB_ALGORITHM2)
    #define SF2_ACB_ALGORITHM_S             1
    #define SF2_ACB_ALGORITHM_M             0x1
    #define SF2_ACB_FLUSH_S                 2
    #define SF2_ACB_FLUSH_M                 0x7
    #define SF2_ACB_EOP_DELAY_S             5
    #define SF2_ACB_EOP_DELAY_M             0xff
#endif
#define SF2_ACB_XON_THRESH_REG              0x40604UL
    #define SF2_ACB_BUFS_THRESH_M           0x7FF
    #define SF2_ACB_TOTAL_XON_BUFS_S        11
    #define SF2_ACB_XON_BUFS_S          0
#define SF2_ACB_QUE0_CONF_REG               0x40608UL
    #define SF2_ACB_QUE_PESSIMISTIC_M       1
    #define SF2_ACB_QUE_PESSIMISTIC_S       31
    #define SF2_ACB_QUE_PKT_LEN_M           0x3F
    #define SF2_ACB_QUE_PKT_LEN_S           25
    #define SF2_ACB_QUE_TOTAL_XON_M         1
    #define SF2_ACB_QUE_TOTAL_XON_S         24
    #define SF2_ACB_QUE_TOTAL_XOFF_M        1
    #define SF2_ACB_QUE_TOTAL_XOFF_S        23
    #define SF2_ACB_QUE_XON_M               1
    #define SF2_ACB_QUE_XON_S               11
    #define SF2_ACB_QUE_TOTOAL_XOFF_BUFS_S   12
    #define SF2_ACB_QUE_XOFF_BUFS_S         0
    #define SF2_ACB_QUE_MAX                 63
#if defined(ACB_ALGORITHM2)
#define SF2_ACB_QUE0_PKTS_IN_FLIGHT         0x40708UL
    #define SF2_ACB_QUE_PKTS_IN_FLIGHT_M    0x7ff
#endif

/****************************************************************************
   Control_Page : Page (0x0)
****************************************************************************/
//#define REG_FAST_AGING_CTRL                           0x88
    #define FAST_AGE_MCAST                              0x20

//#define REG_CONTROL_MII1_PORT_STATE_OVERRIDE          0x0e
#define  IMP_PORT_SPEED_UP_2G                           0xc0
/****************************************************************************
   Flow Control: Page (0x0A)
****************************************************************************/
#define PAGE_FLOW_CTRL_XTN                                    0x0A

#define REG_FC_CTRL_MODE                                      0x2
    #define FC_CTRL_MODE_PORT                                 0x1
#define REG_FC_CTRL_PORT_SEL                                  0x3
    #define REG_FC_CTRL_PORT_P0                               0x0
    #define REG_FC_CTRL_PORT_P1                               0x1
    #define REG_FC_CTRL_PORT_P2                               0x2
    #define REG_FC_CTRL_PORT_P3                               0x3
    #define REG_FC_CTRL_PORT_P4                               0x4
    #define REG_FC_CTRL_PORT_P8                               0x8
    #define REG_FC_CTRL_PORT_P7                               0x7
    #define REG_FC_CTRL_PORT_P5                               0x5
#define REG_FC_OOB_EN                                         0x4  // 16 bit
    #define FC_CTRL_OOB_EN_PORT_P5                            0x10
    #define FC_CTRL_OOB_EN_PORT_P7                            0x80
    #define FC_CTRL_OOB_EN_PORT_P8                            0x100
#define REG_FC_OOB_EN                                         0x4  // 16 bit
#define REG_FC_PAUSE_TIME_MAX                                 0x10  // 16 bit
#define REG_FC_PAUSE_TIME_MIN                                 0x12  // 16 bit
#define REG_FC_PAUSE_TIME_RESET_THD                           0x14  // 16 bit
#define REG_FC_PAUSE_TIME_DEFAULT                             0x18  // 16 bit
#define REG_FC_PAUSE_DROP_CTRL                                0x1c  // 16 bit
    #define FC_QUEUE_BASED_PAUSE_EN                           0x1000
    #define FC_TX_IMP0_TOTAL_PAUSE_EN                         0x800
    #define FC_TX_IMP0_TXQ_PAUSE_EN                           0x400
    #define FC_TX_IMP1_TOTAL_PAUSE_EN                         0x200
    #define FC_TX_IMP1_TXQ_PAUSE_EN                           0x100
    #define FC_TX_TOTAL_PAUSE_EN                              0x80
    #define FC_TX_TXQ_PAUSE_EN                                0x40
    #define FC_RX_DROP_EN                                     0x20
    #define FC_TX_TOTAL_DROP_EN                               0x10
    #define FC_TX_TXQ_DROP_EN                                 0x8
    #define FC_RX_BASED_CTRL_EN                               0x4
    #define FC_TX_QUANTUM_CTRL_EN                             0x2
    #define FC_TX_BASED_CTRL_EN                               0x1

    #define FC_LAN_TXQ_QUEUES                                 8
    #define FC_LAN_TXQ_BUF_RSV_IMP                            24
    #define FC_LAN_TXQ_BUF_RSV_LAN                            16
    #define SF2_SWITCH_BUFS_MAX                               1535
    #define SF2_LAN_BUFS_TXQ_TOT_PAUSE                        656
    #define SF2_LAN_BUFS_TXQ_TOT_HYST                         328
    #define SF2_IMP_BUFS_TXQ_TOT_PAUSE                        304
    #define SF2_IMP_BUFS_TXQ_TOT_HYST                         152
   
/****************************************************************************
   Flow Control: Page (0x0B)
****************************************************************************/
#define PAGE_FC_LAN_TXQ                                       0x0B

    #define REG_FC_LAN_TXQ_THD_RSV_QN0                0x0   // 16 bits x 8 queues
    #define REG_FC_LAN_TXQ_THD_HYST_QN0               0x10  // 16 bits x 8 queues
    #define REG_FC_LAN_TXQ_THD_PAUSE_QN0              0x20  // 16 bits x 8 queues
    #define REG_FC_LAN_TXQ_THD_DROP_QN0               0x30  // 16 bits x 8 queues
    
    #define REG_FC_LAN_TOTAL_THD_HYST_QN0             0x40  // 16 bits x 8 queues
    #define REG_FC_LAN_TOTAL_THD_PAUSE_QN0            0x50  // 16 bits x 8 queues
    #define REG_FC_LAN_TOTAL_THD_DROP_QN0             0x60  // 16 bits x 8 queues
/****************************************************************************
   Flow Control IMP0: Page (0x0D)
****************************************************************************/
#define PAGE_FC_IMP0_TXQ                                       0x0D

    #define REG_FC_IMP0_TXQ_THD_RSV_QN0                0x0   // 16 bits x 8 queues
    #define REG_FC_IMP0_TXQ_THD_HYST_QN0               0x10  // 16 bits x 8 queues
    #define REG_FC_IMP0_TXQ_THD_PAUSE_QN0              0x20  // 16 bits x 8 queues
    #define REG_FC_IMP0_TXQ_THD_DROP_QN0               0x30  // 16 bits x 8 queues
    
    #define REG_FC_IMP0_TOTAL_THD_HYST_QN0             0x40  // 16 bits x 8 queues
    #define REG_FC_IMP0_TOTAL_THD_PAUSE_QN0            0x50  // 16 bits x 8 queues
    #define REG_FC_IMP0_TOTAL_THD_DROP_QN0             0x60  // 16 bits x 8 queues
/****************************************************************************
   Flow Control IMP1: Page (0x0E)
****************************************************************************/
#define PAGE_FC_IMP1_TXQ                                       0x0E

    #define REG_FC_IMP1_TXQ_THD_RSV_QN0                0x0   // 16 bits x 8 queues
    #define REG_FC_IMP1_TXQ_THD_HYST_QN0               0x10  // 16 bits x 8 queues
    #define REG_FC_IMP1_TXQ_THD_PAUSE_QN0              0x20  // 16 bits x 8 queues
    #define REG_FC_IMP1_TXQ_THD_DROP_QN0               0x30  // 16 bits x 8 queues
    
    #define REG_FC_IMP1_TOTAL_THD_HYST_QN0             0x40  // 16 bits x 8 queues
    #define REG_FC_IMP1_TOTAL_THD_PAUSE_QN0            0x50  // 16 bits x 8 queues
    #define REG_FC_IMP1_TOTAL_THD_DROP_QN0             0x60  // 16 bits x 8 queues
// use the following enums to retun GET result of sf2_pause_drop_ctrl()
enum {
    LAN,
    IMP0,
    IMP1,
};
/****************************************************************************
   MIB Counters: Page (0x20 to 0x28)
****************************************************************************/

//#define PAGE_MIB_P0                                       0x20

/* NOTE : TBD ; Almost all (except Q6/7 and TX size based octet count) of these stats
 * are applicable to other External switches as well; should remove duplicity  */

    #define SF2_REG_MIB_P0_TXOCTETS                           0x00
    #define SF2_REG_MIB_P0_TXDROPS                            0x08
    #define SF2_REG_MIB_P0_TXQ0PKT                            0x0C  
    #define SF2_REG_MIB_P0_TXBPKTS                            0x10
    #define SF2_REG_MIB_P0_TXMPKTS                            0x14
    #define SF2_REG_MIB_P0_TXUPKTS                            0x18  
    #define SF2_REG_MIB_P0_TXCOL                              0x1C 
    #define SF2_REG_MIB_P0_TXSINGLECOL                        0x20 
    #define SF2_REG_MIB_P0_TXMULTICOL                         0x24
    #define SF2_REG_MIB_P0_TXDEFERREDTX                       0x28
    #define SF2_REG_MIB_P0_TXLATECOL                          0x2C
    #define SF2_REG_MIB_P0_TXEXCESSCOL                        0x30
    #define SF2_REG_MIB_P0_TXFRAMEINDISC                      0x34
    #define SF2_REG_MIB_P0_TXPAUSEPKTS                        0x38
    // SF2 Enhancements
    #define SF2_REG_MIB_P0_TXQ1PKT                            0x3c
    #define SF2_REG_MIB_P0_TXQ2PKT                            0x40
    #define SF2_REG_MIB_P0_TXQ3PKT                            0x44
    #define SF2_REG_MIB_P0_TXQ4PKT                            0x48
    #define SF2_REG_MIB_P0_TXQ5PKT                            0x4c
    // SF2 Done
    #define SF2_REG_MIB_P0_RXOCTETS                           0x50  
    #define SF2_REG_MIB_P0_RXUNDERSIZEPKTS                    0x58
    #define SF2_REG_MIB_P0_RXPAUSEPKTS                        0x5c  
    #define SF2_REG_MIB_P0_RX64OCTPKTS                        0x60  
    #define SF2_REG_MIB_P0_RX127OCTPKTS                       0x64  
    #define SF2_REG_MIB_P0_RX255OCTPKTS                       0x68  
    #define SF2_REG_MIB_P0_RX511OCTPKTS                       0x6c  
    #define SF2_REG_MIB_P0_RX1023OCTPKTS                      0x70 
    #define SF2_REG_MIB_P0_RXMAXOCTPKTS                       0x74 
    #define SF2_REG_MIB_P0_RXOVERSIZE                         0x78 
    #define SF2_REG_MIB_P0_RXJABBERS                          0x7c
    #define SF2_REG_MIB_P0_RXALIGNERRORS                      0x80
    #define SF2_REG_MIB_P0_RXFCSERRORS                        0x84
    #define SF2_REG_MIB_P0_RXGOODOCT                          0x88
    #define SF2_REG_MIB_P0_RXDROPS                            0x90
    #define SF2_REG_MIB_P0_RXUPKTS                            0x94
    #define SF2_REG_MIB_P0_RXMPKTS                            0x98
    #define SF2_REG_MIB_P0_RXBPKTS                            0x9c
    #define SF2_REG_MIB_P0_RXSACHANGES                        0xa0
    #define SF2_REG_MIB_P0_RXFRAGMENTS                        0xa4
    #define SF2_REG_MIB_P0_RXJUMBOPKT                         0xa8 
    #define SF2_REG_MIB_P0_RXSYMBOLERRORS                     0xAc
    #define SF2_REG_MIB_P0_RXINRANGEERR                       0xB0
    #define SF2_REG_MIB_P0_RXOUTRANGEERR                      0xB4
    #define SF2_REG_MIB_P0_EEELPIEVEVT                        0xB8
    #define SF2_REG_MIB_P0_EEELPIDURATION                     0xBc
    #define SF2_REG_MIB_P0_RXDISCARD                          0xC0
    #define SF2_REG_MIB_P0_TXQ6PKT                            0xC8
    #define SF2_REG_MIB_P0_TXQ7PKT                            0xCC
    #define SF2_REG_MIB_P0_TX64OCTPKTS                        0xD0  
    #define SF2_REG_MIB_P0_TX127OCTPKTS                       0xD4  
    #define SF2_REG_MIB_P0_TX255OCTPKTS                       0xD8  
    #define SF2_REG_MIB_P0_TX511OCTPKTS                       0xDC  
    #define SF2_REG_MIB_P0_TX1023OCTPKTS                      0xE0  
    #define SF2_REG_MIB_P0_TXMAXOCTPKTS                       0xE4  

/****************************************************************************
   QOS : Page (0x30)
****************************************************************************/

    #define SF2_REG_PORT_ID_PRIO_MAP                          0x48
    #define SF2_REG_PORTN_TC_SELECT_TABLE                     0x50
        #define SF2_QOS_TC_SRC_SEL_PKT_TYPE_MASK              7
        #define SF2_QOS_TC_SRC_SEL_VAL_MASK                   3
    #define SF2_REG_PORTN_TC_TO_COS                           0x70
        #define SF2_QOS_TC_SRC_SEL_PKT_TYPE_ALL               0x8
        #define SF2_QOS_TC_MAX                                0x7
        #define SF2_QOS_COS_MASK                              0x7
        #define SF2_QOS_COS_SHIFT                             0x3
    #define SF2_REG_QOS_PCP_P7                                0x28
    #define SF2_REG_QOS_PCP_IMP0                              0x2c
/****************************************************************************
   QOS Scheduler : Page (0x46)
****************************************************************************/

#define PAGE_QOS_SCHEDULER                                     0x46

// default is Strict Priority on all q's of all ports.
#define REG_PN_QOS_PRI_CTL_PORT_0                              0x0   
    #define REG_PN_QOS_PRI_CTL_SZ                              0x1   // 1 Byte per port
    #define  PN_QOS_SCHED_SEL_M                                0x7
    #define  PN_QOS_SCHED_SEL_S                                0

    #define SF2_ALL_Q_SP                                       0
    #define SF2_Q7_SP                                          1
    #define SF2_Q7_Q6_SP                                       2
    #define SF2_Q7_Q5_SP                                       3
    #define SF2_Q7_Q4_SP                                       4
    #define SF2_ALL_Q_WRR                                      5
  /* Granularity is 1 packet or 256 bytes */
    #define  PN_QOS_WDRR_GRAN_M                                0x1
    #define  PN_QOS_WDRR_GRAN_S                                3

    #define SF2_WRR_PKT                                        1
    #define SF2_WDRR_PKT                                       0

#define REG_PN_QOS_WEIGHT_PORT_0                               0x10 
                                             // 8q's x 1Byte per port
                                             // [q7.. q0]
#define REG_PN_QOS_WEIGHTS                                     0x8 

/****************************************************************************
   Egress Shaper control : Page (0x47)
****************************************************************************/

#define PAGE_PORT_EGRESS_SHAPER                                0x47

    #define SF2_REG_PN_SHAPER_RATE_BYTE                        0x0
    #define SF2_REG_P7_SHAPER_RATE_BYTE                        0x1C
    #define SF2_REG_P8_SHAPER_RATE_BYTE                        0x20

    #define SF2_REG_PN_SHAPER_BURST_SZ_BYTE                    0x30
    #define SF2_REG_P7_SHAPER_BURST_SZ_BYTE                    0x4C
    #define SF2_REG_P8_SHAPER_BURST_SZ_BYTE                    0x50

    #define SF2_REG_PN_SHAPER_STAT                             0x60
    #define SF2_REG_P7_SHAPER_STAT                             0x7C
    #define SF2_REG_P8_SHAPER_STAT                             0x80

    #define SF2_REG_PN_SHAPER_RATE_PKT                         0x90
    #define SF2_REG_P7_SHAPER_RATE_PKT                         0xAC
    #define SF2_REG_P8_SHAPER_RATE_PKT                         0xB0

    #define SF2_REG_PN_SHAPER_BURST_SZ_PKT                     0xC0
    #define SF2_REG_P7_SHAPER_BURST_SZ_PKT                     0xDC
    #define SF2_REG_P8_SHAPER_BURST_SZ_PKT                     0xE0

        #define SHAPER_RATE_BURST_VAL_MASK                     0x3FFFFU
        #define SHAPER_PACKET_MODE                             1
        #define SHAPER_STAT_COUNT_MASK                         0xFFFFFFFU
        #define SHAPER_STAT_OVF_MASK                           0x10000000U  // Overflow mask
        #define SHAPER_STAT_INPF_MASK                          0x10000000U  // In Profile mask

    #define SF2_REG_SHAPER_ENB_AVB                             0xE4
    #define SF2_REG_SHAPER_ENB                                 0xE6
    #define SF2_REG_SHAPER_ENB_PKT_BASED                       0xE8
    #define SF2_REG_SHAPER_BLK_CTRL_ENB                        0xEA
    #define SF2_REG_SHAPER_INC_IFG_CTRL                        0xEC   // On port shaper only

/****************************************************************************
   Egress Per QUEUE Shaper control : Page (0x48)
****************************************************************************/

#define PAGE_Q0_EGRESS_SHAPER                                0x48
#define PAGE_Q1_EGRESS_SHAPER                                0x49
#define PAGE_Q2_EGRESS_SHAPER                                0x4A
#define PAGE_Q3_EGRESS_SHAPER                                0x4B
#define PAGE_Q4_EGRESS_SHAPER                                0x4C
#define PAGE_Q5_EGRESS_SHAPER                                0x4D
#define PAGE_Q6_EGRESS_SHAPER                                0x4E
#define PAGE_Q7_EGRESS_SHAPER                                0x4F



#endif /* _BCMMII_XTN_H_ */
