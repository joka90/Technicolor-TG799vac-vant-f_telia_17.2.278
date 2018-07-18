/*
    Copyright 2000-2011 Broadcom Corporation

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

#ifndef __BCM6818_MAP_PART_H
#define __BCM6818_MAP_PART_H

#ifdef __cplusplus
extern "C" {
#endif

#define CHIP_FAMILY_ID_HEX  0x6818

#ifndef __BCM6818_MAP_H
#define __BCM6818_MAP_H

#include "bcmtypes.h"

#define DBL_DESC 1

#define PERF_BASE                   0xb0000000
#define TIMR_BASE                   0xb0000040
#define NAND_INTR_BASE              0xb0000070
#define GPIO_BASE                   0xb0000080
#define UART_BASE                   0xb0000100
#define I2C_BASE             	    0xb0000180
#define UART1_BASE                  0xb0000120
#define I2C_BASE                    0xb0000180
#define OTP_BASE                    0xb0000400
#define UBUS_STAT_BASE              0xb0000500
#define SPI_BASE                    0xb0000800
#define HSSPIM_BASE                 0xb0001000
#define MISC_BASE                   0xb0001800
#define NAND_REG_BASE               0xb0002000
#define NAND_CACHE_BASE             0xb0002200
#define MPI_BASE                    0xb0002400
#define USB_CTL_BASE                0xb0002500
#define USB_EHCI_BASE               0x10002600
#define USB_OHCI_BASE               0x10002700
#define USBH_CFG_BASE               0xb0002800
#define IPSEC_BASE                  0xb0002900
#define MEMC_BASE                   0xb0003000
#define GPON_BASE                   0xb0004000
#define GPON_SERDES_BASE            0xb0004800
#define USB_DMA_BASE                0xb000c000
#define GPON_DMA_BASE               0xb000c800
#define IPSEC_DMA_BASE              0xb000d000
#define SWITCH_DMA_BASE             0xb000d800
#define SWITCH_DMA_CONFIG           0xb000da00
#define SWITCH_DMA_STATE            0xb000dc00
#define GMAC_BASE                   0xb000e000
#define GMAC_DMA_BASE               0xb000e800
#define PCIE_MEM1M_BASE             0xb0100000
#define APM_BASE                    0xb0200000
#define PCM_BASE                    0xb0200200
#define APM_HVG_BASE                0xb0200300
#define APM_HVG_BASE_REG_15         0xb0200408
#define APM_HVG_BASE_REG_19         0xb0200488
#define APM_IUDMA_BASE              0xb0200800
#define BMU_BASE                    0xb0201000
#define APM_PICO_IMEM_BASE          0xb0210000
#define PCIE_BASE                   0xb06e0000
#define SWITCH_BASE                 0xb0700000

#define FAP0_BASE                   0xb0800000
#define FAP0_QSM_UBUS_BASE          0xb0804000
#define FAP0_QSM_SMI_BASE           0xb0c04000
#define FAP0_PSM_BASE               0xb0820000

#define FAP1_BASE                   0xb0a00000
#define FAP1_QSM_UBUS_BASE          0xb0a04000
#define FAP1_QSM_SMI_BASE           0xb0e04000
#define FAP1_PSM_BASE               0xb0a20000

#endif

typedef struct EthSwRegs{
    byte port_traffic_ctrl[9]; /* 0x00 - 0x08 */
    byte reserved1[2]; /* 0x09 - 0x0a */
    byte switch_mode; /* 0x0b */
    unsigned short pause_quanta; /*0x0c */
    byte imp_port_state; /*0x0e */
    byte led_refresh; /* 0x0f */
    unsigned short led_function[2]; /* 0x10 */
    unsigned short led_function_map; /* 0x14 */
    unsigned short led_enable_map; /* 0x16 */
    unsigned short led_mode_map0; /* 0x18 */
    unsigned short led_function_map1; /* 0x1a */
    byte reserved2[5]; /* 0x1b - 0x20 */
    byte port_forward_ctrl; /* 0x21 */
    byte reserved3; /* 0x22 */
    byte port_enable;  /* 0x23 */
#define SWITCH_PORT_ENABLE_MASK		0xff
#define SWITCH_PORT_ENABLE_SHIFT	0
    unsigned short protected_port_selection; /* 0x24 */
    unsigned short wan_port_select; /* 0x26 */
    unsigned int pause_capability; /* 0x28 */
    byte reserved4[3]; /* 0x2c - 0x2e */
    byte reserved_multicast_control; /* 0x2f */
    byte reserved5; /* 0x30 */
    byte txq_flush_mode_control; /* 0x31 */
    unsigned short ulf_forward_map; /* 0x32 */
    unsigned short mlf_forward_map; /* 0x34 */
    unsigned short mlf_impc_forward_map; /* 0x36 */
    unsigned short pause_pass_through_for_rx; /* 0x38 */
    unsigned short pause_pass_through_for_tx; /* 0x3a */
    unsigned short disable_learning; /* 0x3c */
    byte reserved6[6]; /* 0x3e - 0x43 */
    unsigned short mii_packet_size; /* 0x44 */
    unsigned short gmii_packet_size; /* 0x46 */
    byte reserved7[16]; /* 0x48 - 0x57 */
    byte port_state_override[8]; /* 0x58 - 0x5f */
    byte reserved8[4]; /* 0x60 - 0x63 */
    byte imp_rgmii_ctrl_p2; /* 0x64 */
    byte imp_rgmii_ctrl_p3; /* 0x65 */
    byte reserved9[1]; /* 0x66 */
    byte imp_rgmii_ctrl_p5; /* 0x67 */
    byte reserved10[6]; /* 0x66 - 0x6b */
    byte rgmii_timing_delay_p2; /* 0x6c */
    byte gmii_timing_delay_p3; /* 0x6d */
    byte reserved11[11]; /* 0x6e - 0x78 */
    byte software_reset; /* 0x79 */
    byte reserved12[6]; /* 0x7a - 0x7f */
    byte pause_frame_detection; /* 0x80 */
    byte reserved13[7]; /* 0x81 - 0x87 */
    byte fast_aging_ctrl; /* 0x88 */
    byte fast_aging_port; /* 0x89 */
    byte fast_aging_vid; /* 0x8a */
    byte reserved14[21]; /* 0x8b - 0x9f */
    unsigned int swpkt_ctrl_usb; /*0xa0 */
    unsigned int swpkt_ctrl_gpon; /*0xa4 */
    unsigned int iudma_ctrl; /*0xa8 */
    unsigned int iudma_queue_ctrl; /*0xac */
    unsigned int mdio_ctrl; /*0xb0 */
    unsigned int mdio_data; /*0xb4 */
    byte reserved15[42]; /* 0xb6 - 0xdf */
    unsigned int sw_mem_test; /*0xe0 */
} EthSwRegs;

#define ETHSWREG ((volatile EthSwRegs * const) SWITCH_BASE)



typedef struct HvgMiscRegisterChannelA {
   uint32        mask;
#define   K_PROP                          0x0000000f
#define   K_INTEG                         0x000000f0
#define   SER_TST_OUTPUT_SEL              0x00000700
#define   CONT_OR_BLOCK                   0x00000800
#define   HVG_MODE                        0x00003000
#define   HVG_MODE_OFFHOOK_TRACKING       0x00001000
#define   HVG_MODE_ONHOOK_FIXED           0x00002000
#define   HVG_SOFT_INIT_0                 0x00004000
#define   HVG_RR_SINGLE                   0x00008000
} HvgMiscRegisterChannelA;

#define HVG_MISC_REG_CHANNEL_A ((volatile HvgMiscRegisterChannelA * const) APM_HVG_BASE_REG_15)

typedef struct HvgMiscRegisterChannelB {
   uint32        mask;
} HvgMiscRegisterChannelB;

#define HVG_MISC_REG_CHANNEL_B ((volatile HvgMiscRegisterChannelB * const) APM_HVG_BASE_REG_19)


/*
** NAND Controller Registers
*/
typedef struct NandCtrlRegs {
    uint32 NandRevision;            /* NAND Revision */
    uint32 NandCmdStart;            /* Nand Flash Command Start */
#define NCMD_MASK           0x1f000000
#define NCMD_LOW_LEVEL_OP   0x10000000
#define NCMD_PARAM_CHG_COL  0x0f000000
#define NCMD_PARAM_READ     0x0e000000
#define NCMD_BLK_LOCK_STS   0x0d000000
#define NCMD_BLK_UNLOCK     0x0c000000
#define NCMD_BLK_LOCK_DOWN  0x0b000000
#define NCMD_BLK_LOCK       0x0a000000
#define NCMD_FLASH_RESET    0x09000000
#define NCMD_BLOCK_ERASE    0x08000000
#define NCMD_DEV_ID_READ    0x07000000
#define NCMD_COPY_BACK      0x06000000
#define NCMD_PROGRAM_SPARE  0x05000000
#define NCMD_PROGRAM_PAGE   0x04000000
#define NCMD_STS_READ       0x03000000
#define NCMD_SPARE_READ     0x02000000
#define NCMD_PAGE_READ      0x01000000

    uint32 NandCmdExtAddr;          /* Nand Flash Command Extended Address */
    uint32 NandCmdAddr;             /* Nand Flash Command Address */
    uint32 NandCmdEndAddr;          /* Nand Flash Command End Address */
    uint32 NandNandBootConfig;      /* Nand Flash Boot Config */
#define NBC_CS_LOCK         0x80000000
#define NBC_AUTO_DEV_ID_CFG 0x40000000
#define NBC_WR_PROT_BLK0    0x10000000
#define NBC_EBI_CS7_USES_NAND (1<<15)
#define NBC_EBI_CS6_USES_NAND (1<<14)
#define NBC_EBI_CS5_USES_NAND (1<<13)
#define NBC_EBI_CS4_USES_NAND (1<<12)
#define NBC_EBI_CS3_USES_NAND (1<<11)
#define NBC_EBI_CS2_USES_NAND (1<<10)
#define NBC_EBI_CS1_USES_NAND (1<< 9)
#define NBC_EBI_CS0_USES_NAND (1<< 8)
#define NBC_EBC_CS7_SEL       (1<< 7)
#define NBC_EBC_CS6_SEL       (1<< 6)
#define NBC_EBC_CS5_SEL       (1<< 5)
#define NBC_EBC_CS4_SEL       (1<< 4)
#define NBC_EBC_CS3_SEL       (1<< 3)
#define NBC_EBC_CS2_SEL       (1<< 2)
#define NBC_EBC_CS1_SEL       (1<< 1)
#define NBC_EBC_CS0_SEL       (1<< 0)

    uint32 NandCsNandXor;           /* Nand Flash EBI CS Address XOR with */
                                    /*   1FC0 Control */
    uint32 NandReserved1;
    uint32 NandSpareAreaReadOfs0;   /* Nand Flash Spare Area Read Bytes 0-3 */
    uint32 NandSpareAreaReadOfs4;   /* Nand Flash Spare Area Read Bytes 4-7 */
    uint32 NandSpareAreaReadOfs8;   /* Nand Flash Spare Area Read Bytes 8-11 */
    uint32 NandSpareAreaReadOfsC;   /* Nand Flash Spare Area Read Bytes 12-15*/
    uint32 NandSpareAreaWriteOfs0;  /* Nand Flash Spare Area Write Bytes 0-3 */
    uint32 NandSpareAreaWriteOfs4;  /* Nand Flash Spare Area Write Bytes 4-7 */
    uint32 NandSpareAreaWriteOfs8;  /* Nand Flash Spare Area Write Bytes 8-11*/
    uint32 NandSpareAreaWriteOfsC;  /* Nand Flash Spare Area Write Bytes12-15*/
    uint32 NandAccControl;          /* Nand Flash Access Control */
#define NAC_RD_ECC_EN       0x80000000
#define NAC_WR_ECC_EN       0x40000000
#define NAC_RD_ECC_BLK0_EN  0x20000000
#define NAC_FAST_PGM_RDIN   0x10000000
#define NAC_RD_ERASED_ECC_EN 0x08000000
#define NAC_PARTIAL_PAGE_EN 0x04000000
#define NAC_WR_PREEMPT_EN   0x02000000
#define NAC_PAGE_HIT_EN     0x01000000
#define NAC_ECC_LVL_0_SHIFT 20
#define NAC_ECC_LVL_0_MASK  0x00f00000
#define NAC_ECC_LVL_SHIFT   16
#define NAC_ECC_LVL_MASK    0x000f0000
#define NAC_ECC_LVL_DISABLE 0
#define NAC_ECC_LVL_BCH_1   1
#define NAC_ECC_LVL_BCH_2   2
#define NAC_ECC_LVL_BCH_3   3
#define NAC_ECC_LVL_BCH_4   4
#define NAC_ECC_LVL_BCH_5   5
#define NAC_ECC_LVL_BCH_6   6
#define NAC_ECC_LVL_BCH_7   7
#define NAC_ECC_LVL_BCH_8   8
#define NAC_ECC_LVL_BCH_9   9
#define NAC_ECC_LVL_BCH_10  10
#define NAC_ECC_LVL_BCH_11  11
#define NAC_ECC_LVL_BCH_12  12
#define NAC_ECC_LVL_RESVD_1 13
#define NAC_ECC_LVL_RESVD_2 14
#define NAC_ECC_LVL_HAMMING 15
#define NAC_SPARE_SZ_0_SHIFT 8
#define NAC_SPARE_SZ_0_MASK 0x00003f00
#define NAC_SPARE_SZ_SHIFT  0
#define NAC_SPARE_SZ_MASK   0x0000003f
    uint32 NandReserved2;
    uint32 NandConfig;              /* Nand Flash Config */
#define NC_CONFIG_LOCK      0x80000000
#define NC_BLK_SIZE_MASK    0x70000000
#define NC_BLK_SIZE_2048K   0x60000000
#define NC_BLK_SIZE_1024K   0x50000000
#define NC_BLK_SIZE_512K    0x30000000
#define NC_BLK_SIZE_128K    0x10000000
#define NC_BLK_SIZE_16K     0x00000000
#define NC_BLK_SIZE_8K      0x20000000
#define NC_BLK_SIZE_256K    0x40000000
#define NC_DEV_SIZE_MASK    0x0f000000
#define NC_DEV_SIZE_SHIFT   24
#define NC_DEV_WIDTH_MASK   0x00800000
#define NC_DEV_WIDTH_16     0x00800000
#define NC_DEV_WIDTH_8      0x00000000
#define NC_PG_SIZE_MASK     0x00300000
#define NC_PG_SIZE_8K       0x00300000
#define NC_PG_SIZE_4K       0x00200000
#define NC_PG_SIZE_2K       0x00100000
#define NC_PG_SIZE_512B     0x00000000
#define NC_FUL_ADDR_MASK    0x00070000
#define NC_FUL_ADDR_SHIFT   16
#define NC_BLK_ADDR_MASK    0x00000700
#define NC_BLK_ADDR_SHIFT   8

    uint32 NandReserved3;
    uint32 NandTiming1;             /* Nand Flash Timing Parameters 1 */
    uint32 NandTiming2;             /* Nand Flash Timing Parameters 2 */
    uint32 NandSemaphore;           /* Semaphore */
    uint32 NandReserved4;
    uint32 NandFlashDeviceId;       /* Nand Flash Device ID */
    uint32 NandFlashDeviceIdExt;    /* Nand Flash Extended Device ID */
    uint32 NandBlockLockStatus;     /* Nand Flash Block Lock Status */
    uint32 NandIntfcStatus;         /* Nand Flash Interface Status */
#define NIS_CTLR_READY      0x80000000
#define NIS_FLASH_READY     0x40000000
#define NIS_CACHE_VALID     0x20000000
#define NIS_SPARE_VALID     0x10000000
#define NIS_FLASH_STS_MASK  0x000000ff
#define NIS_WRITE_PROTECT   0x00000080
#define NIS_DEV_READY       0x00000040
#define NIS_PGM_ERASE_ERROR 0x00000001

    uint32 NandEccCorrExtAddr;      /* ECC Correctable Error Extended Address*/
    uint32 NandEccCorrAddr;         /* ECC Correctable Error Address */
    uint32 NandEccUncExtAddr;       /* ECC Uncorrectable Error Extended Addr */
    uint32 NandEccUncAddr;          /* ECC Uncorrectable Error Address */
    uint32 NandReadErrorCount;      /* Read Error Count */
    uint32 NandCorrStatThreshold;   /* Correctable Error Reporting Threshold */
    uint32 NandOnfiStatus;          /* ONFI Status */
    uint32 NandOnfiDebugData;       /* ONFI Debug Data */
    uint32 NandFlashReadExtAddr;    /* Flash Read Data Extended Address */
    uint32 NandFlashReadAddr;       /* Flash Read Data Address */
    uint32 NandProgramPageExtAddr;  /* Page Program Extended Address */
    uint32 NandProgramPageAddr;     /* Page Program Address */
    uint32 NandCopyBackExtAddr;     /* Copy Back Extended Address */
    uint32 NandCopyBackAddr;        /* Copy Back Address */
    uint32 NandBlockEraseExtAddr;   /* Block Erase Extended Address */
    uint32 NandBlockEraseAddr;      /* Block Erase Address */
    uint32 NandInvReadExtAddr;      /* Flash Invalid Data Extended Address */
    uint32 NandInvReadAddr;         /* Flash Invalid Data Address */
    uint32 NandReserved5[2];
    uint32 NandBlkWrProtect;        /* Block Write Protect Enable and Size */
                                    /*   for EBI_CS0b */
    uint32 NandReserved6[3];
    uint32 NandAccControlCs1;       /* Nand Flash Access Control */
    uint32 NandConfigCs1;           /* Nand Flash Config */
    uint32 NandTiming1Cs1;          /* Nand Flash Timing Parameters 1 */
    uint32 NandTiming2Cs1;          /* Nand Flash Timing Parameters 2 */
    uint32 NandAccControlCs2;       /* Nand Flash Access Control */
    uint32 NandConfigCs2;           /* Nand Flash Config */
    uint32 NandTiming1Cs2;          /* Nand Flash Timing Parameters 1 */
    uint32 NandTiming2Cs2;          /* Nand Flash Timing Parameters 2 */
    uint32 NandReserved7[16];
    uint32 NandSpareAreaReadOfs10;  /* Nand Flash Spare Area Read Bytes 16-19 */
    uint32 NandSpareAreaReadOfs14;  /* Nand Flash Spare Area Read Bytes 20-23 */
    uint32 NandSpareAreaReadOfs18;  /* Nand Flash Spare Area Read Bytes 24-27 */
    uint32 NandSpareAreaReadOfs1C;  /* Nand Flash Spare Area Read Bytes 28-31 */
    uint32 NandSpareAreaWriteOfs10; /* Nand Flash Spare Area Write Bytes 16-19 */
    uint32 NandSpareAreaWriteOfs14; /* Nand Flash Spare Area Write Bytes 20-23 */
    uint32 NandSpareAreaWriteOfs18; /* Nand Flash Spare Area Write Bytes 24-27 */
    uint32 NandSpareAreaWriteOfs1C; /* Nand Flash Spare Area Write Bytes 28-31 */
    uint32 NandReserved8[10];
    uint32 NandLlOpNand;            /* Flash Low Level Operation */
    uint32 NandLlRdData;            /* Nand Flash Low Level Read Data */
} NandCtrlRegs;

#define NAND ((volatile NandCtrlRegs * const) NAND_REG_BASE)


typedef struct DDRPhyControl {
    uint32 REVISION;               /* 0x00 */
    uint32 CLK_PM_CTRL;            /* 0x04 */
    uint32 unused0[2];             /* 0x08-0x10 */
    uint32 PLL_STATUS;             /* 0x10 */
    uint32 PLL_CONFIG;             /* 0x14 */
    uint32 PLL_PRE_DIVIDER;        /* 0x18 */
    uint32 PLL_DIVIDER;            /* 0x1c */
    uint32 PLL_CONTROL1;           /* 0x20 */
    uint32 PLL_CONTROL2;           /* 0x24 */
    uint32 PLL_SS_EN;              /* 0x28 */
    uint32 PLL_SS_CFG;             /* 0x2c */
    uint32 STATIC_VDL_OVERRIDE;    /* 0x30 */
    uint32 DYNAMIC_VDL_OVERRIDE;   /* 0x34 */
    uint32 IDLE_PAD_CONTROL;       /* 0x38 */
    uint32 ZQ_PVT_COMP_CTL;        /* 0x3c */
    uint32 DRIVE_PAD_CTL;          /* 0x40 */
    uint32 CLOCK_REG_CONTROL;      /* 0x44 */
    uint32 unused1[46];
} DDRPhyControl;

typedef struct DDRPhyByteLaneControl {
    uint32 REVISION;                /* 0x00 */
    uint32 VDL_CALIBRATE;           /* 0x04 */
    uint32 VDL_STATUS;              /* 0x08 */
#define VDL_STATUS_CALIB_FSM_IDLE_SHIFT      0
#define VDL_STATUS_CALIB_FSM_IDLE_MASK       (1<<VDL_STATUS_CALIB_FSM_IDLE_SHIFT)
#define VDL_STATUS_CALIB_FSM_IDLE_		     1
#define VDL_STATUS_CALIB_FSM_NOT_IDLE	     0
#define VDL_STATUS_CALIB_TOTAL_STEP_SHIFT    8
#define VDL_STATUS_CALIB_TOTAL_STEP_MASK     (0x1f<<VDL_STATUS_CALIB_TOTAL_STEP_SHIFT)
    uint32 unused;                  /* 0x0c */
    uint32 VDL_OVERRIDE_0;          /* 0x10 */
    uint32 VDL_OVERRIDE_1;          /* 0x14 */
    uint32 VDL_OVERRIDE_2;          /* 0x18 */
    uint32 VDL_OVERRIDE_3;          /* 0x1c */
    uint32 VDL_OVERRIDE_4;          /* 0x20 */
    uint32 VDL_OVERRIDE_5;          /* 0x24 */
    uint32 VDL_OVERRIDE_6;          /* 0x28 */
    uint32 VDL_OVERRIDE_7;          /* 0x2c */
    uint32 READ_CONTROL;            /* 0x30 */
    uint32 READ_FIFO_STATUS;        /* 0x34 */
    uint32 READ_FIFO_CLEAR;         /* 0x38 */
    uint32 IDLE_PAD_CONTROL;        /* 0x3c */
    uint32 DRIVE_PAD_CTL;           /* 0x40 */
    uint32 CLOCK_PAD_DISABLE;       /* 0x44 */
    uint32 WR_PREAMBLE_MODE;        /* 0x48 */
    uint32 CLOCK_REG_CONTROL;       /* 0x4C */
    uint32 unused0[44];
} DDRPhyByteLaneControl;

/*
 * I2C Controller.
 */

typedef struct I2CControl {
  uint32        ChipAddress;            /* 0x0 */
#define I2C_CHIP_ADDRESS_MASK           0x000000f7
#define I2C_CHIP_ADDRESS_SHIFT          0x1
  uint32        DataIn0;                /* 0x4 */
  uint32        DataIn1;                /* 0x8 */
  uint32        DataIn2;                /* 0xc */
  uint32        DataIn3;                /* 0x10 */
  uint32        DataIn4;                /* 0x14 */
  uint32        DataIn5;                /* 0x18 */
  uint32        DataIn6;                /* 0x1c */
  uint32        DataIn7;                /* 0x20 */
  uint32        CntReg;                 /* 0x24 */
#define I2C_CNT_REG1_SHIFT              0x0
#define I2C_CNT_REG2_SHIFT              0x6
  uint32        CtlReg;                 /* 0x28 */
#define I2C_CTL_REG_DTF_MASK            0x00000003
#define I2C_CTL_REG_DTF_WRITE           0x0
#define I2C_CTL_REG_DTF_READ            0x1
#define I2C_CTL_REG_DTF_READ_AND_WRITE  0x2
#define I2C_CTL_REG_DTF_WRITE_AND_READ  0x3
#define I2C_CTL_REG_DEGLITCH_DISABLE    0x00000004
#define I2C_CTL_REG_DELAY_DISABLE       0x00000008
#define I2C_CTL_REG_SCL_SEL_MASK        0x00000030
#define I2C_CTL_REG_SCL_CLK_375KHZ      0x00000000
#define I2C_CTL_REG_SCL_CLK_390KHZ      0x00000010
#define I2C_CTL_REG_SCL_CLK_187_5KHZ    0x00000020
#define I2C_CTL_REG_SCL_CLK_200KHZ      0x00000030
#define I2C_CTL_REG_INT_ENABLE          0x00000040
#define I2C_CTL_REG_DIV_CLK             0x00000080
  uint32        IICEnable;              /* 0x2c */
#define I2C_IIC_ENABLE                  0x00000001
#define I2C_IIC_INTRP                   0x00000002
#define I2C_IIC_NO_ACK                  0x00000004
#define I2C_IIC_NO_STOP                 0x00000010
#define I2C_IIC_NO_START                0x00000020
  uint32        DataOut0;               /* 0x30 */
  uint32        DataOut1;               /* 0x34 */
  uint32        DataOut2;               /* 0x38 */
  uint32        DataOut3;               /* 0x3c */
  uint32        DataOut4;               /* 0x40 */
  uint32        DataOut5;               /* 0x44 */
  uint32        DataOut6;               /* 0x48 */
  uint32        DataOut7;               /* 0x4c */
  uint32        CtlHiReg;               /* 0x50 */
#define I2C_CTLHI_REG_WAIT_DISABLE      0x00000001
#define I2C_CTLHI_REG_IGNORE_ACK        0x00000002
#define I2C_CTLHI_REG_DATA_REG_SIZE     0x00000040
  uint32        SclParam;               /* 0x54 */
} I2CControl;

#define I2C ((volatile I2CControl * const) I2C_BASE)

#define BRCM_VARIANT_REG_BASE	    0xb0000434
#define BRCM_VARIANT_REG (*((volatile uint32 * const)BRCM_VARIANT_REG_BASE))
#define BRCM_VARIANT_REG_SHIFT 		(18)
#define BRCM_VARIANT_REG_MASK		(0x3c0000)

typedef struct MEMCControl {
    uint32 CNFG;                            /* 0x000 */
    uint32 CSST;                            /* 0x004 */
    uint32 CSEND;                           /* 0x008 */
    uint32 unused;                          /* 0x00c */
    uint32 ROW00_0;                         /* 0x010 */
    uint32 ROW00_1;                         /* 0x014 */
    uint32 ROW01_0;                         /* 0x018 */
    uint32 ROW01_1;                         /* 0x01c */
    uint32 unused0[4];
    uint32 ROW20_0;                         /* 0x030 */
    uint32 ROW20_1;                         /* 0x034 */
    uint32 ROW21_0;                         /* 0x038 */
    uint32 ROW21_1;                         /* 0x03c */
    uint32 unused1[4];
    uint32 COL00_0;                         /* 0x050 */
    uint32 COL00_1;                         /* 0x054 */
    uint32 COL01_0;                         /* 0x058 */
    uint32 COL01_1;                         /* 0x05c */
    uint32 unused2[4];
    uint32 COL20_0;                         /* 0x070 */
    uint32 COL20_1;                         /* 0x074 */
    uint32 COL21_0;                         /* 0x078 */
    uint32 COL21_1;                         /* 0x07c */
    uint32 unused3[4];
    uint32 BNK10;                           /* 0x090 */
    uint32 BNK32;                           /* 0x094 */
    uint32 unused4[26];
    uint32 DCMD;                            /* 0x100 */
#define DCMD_CS1          (1 << 5)
#define DCMD_CS0          (1 << 4)
#define DCMD_SET_SREF     4
    uint32 DMODE_0;                         /* 0x104 */
    uint32 DMODE_2;                         /* 0x108 */
    uint32 CLKS;                            /* 0x10c */
    uint32 ODT;                             /* 0x110 */
    uint32 TIM1_0;                          /* 0x114 */
    uint32 TIM1_1;                          /* 0x118 */
    uint32 TIM2;                            /* 0x11c */
    uint32 CTL_CRC;                         /* 0x120 */
    uint32 DOUT_CRC;                        /* 0x124 */
    uint32 DIN_CRC;                         /* 0x128 */
    uint32 unused5[2];
    uint32 DRAM_CFG;                        /* 0x134 */
#define CFG_DRAMSLEEP (1 << 11)
    uint32 CTL_STAT;                        /* 0x138 */
    uint32 unused6[49];

    DDRPhyControl           PhyControl;             /* 0x200 */
    DDRPhyByteLaneControl   PhyByteLane0Control;    /* 0x300 */
    DDRPhyByteLaneControl   PhyByteLane1Control;    /* 0x400 */
    
    uint32 unused7[192];                            /* 0x500 */

    uint32 GCFG;                            /* 0x800 */
    uint32 VERS;                            /* 0x804 */
    uint32 unused8;                         /* 0x808 */
    uint32 ARB;                             /* 0x80c */
    uint32 PI_GCF;                          /* 0x810 */
    uint32 PI_UBUS_CTL;                     /* 0x814 */
    uint32 PI_MIPS_CTL;                     /* 0x818 */
    uint32 PI_DSL_MIPS_CTL;                 /* 0x81c */
    uint32 PI_DSL_PHY_CTL;                  /* 0x820 */
    uint32 PI_UBUS_ST;                      /* 0x824 */
    uint32 PI_MIPS_ST;                      /* 0x828 */
    uint32 PI_DSL_MIPS_ST;                  /* 0x82c */
    uint32 PI_DSL_PHY_ST;                   /* 0x830 */
    uint32 PI_UBUS_SMPL;                    /* 0x834 */
    uint32 TESTMODE;                        /* 0x838 */
    uint32 TEST_CFG1;                       /* 0x83c */
    uint32 TEST_PAT;                        /* 0x840 */
    uint32 TEST_COUNT;                      /* 0x844 */
    uint32 TEST_CURR_COUNT;                 /* 0x848 */
    uint32 TEST_ADDR_UPDT;                  /* 0x84c */
    uint32 TEST_ADDR;                       /* 0x850 */
    uint32 TEST_DATA0_0;                    /* 0x854 */
    uint32 TEST_DATA0_1;                    /* 0x858 */
    uint32 TEST_DATA0_2;                    /* 0x85c */
    uint32 TEST_DATA0_3;                    /* 0x860 */
    uint32 TEST_DATA1_0;                    /* 0x864 */
    uint32 TEST_DATA1_1;                    /* 0x868 */
    uint32 TEST_DATA1_2;                    /* 0x86c */
    uint32 TEST_DATA1_3;                    /* 0x870 */
    uint32 REPLY_DATA0;                     /* 0x874 */
    uint32 REPLY_DATA1;                     /* 0x878 */
    uint32 REPLY_DATA2;                     /* 0x87c */
    uint32 REPLY_DATA3;                     /* 0x880 */
    uint32 REPLY_STAT;                      /* 0x884 */
    uint32 LBIST_CFG;                       /* 0x888 */
    uint32 LBIST_SEED;                      /* 0x88c */
    uint32 PI_MIPS_SMPL;                    /* 0x890 */
} MEMCControl;

#define MEMC ((volatile MEMCControl * const) MEMC_BASE)


/*
** Peripheral Controller
*/

#define IRQ_BITS 64
typedef struct  {
    uint64         IrqMask;
    uint64         IrqStatus;
} IrqControl_t;

typedef struct  {
    uint32         ExtIrqMask32;
    uint32         IrqMask32;
    uint32         ExtIrqStatus32;
    uint32         IrqStatus32;
} IrqControl_32_t;


typedef struct PerfControl {
    uint32        RevID;             /* (00) word 0 */
#define CHIP_ID_SHIFT   16
#define CHIP_ID_MASK    (0xffff << CHIP_ID_SHIFT)
#define CHIP_VAR_SHIFT   8
#define CHIP_VAR_MASK    (0xff << CHIP_VAR_SHIFT)
#define REV_ID_MASK     0xff

    uint32        blkEnables;        /* (04) word 1 */
#define APM_SPI_SCLK_EN         (1 << 31)    
#define GMAC_UBUS_CLK_EN        (1 << 30)
#define APM_AUDIO_UBUS_CLKEN    (1 << 29)
#define MIPS_CLK_EN             (1 << 28)
#define FAP1_CLK_EN             (1 << 27)
#define TESTBUS_CLK_EN          (1 << 26)
#define APM_AUDIO_A_CLKEN       (1 << 25)
#define APM_AUDIO_B_CLKEN       (1 << 24)
#define NTP_CLK_EN              (1 << 23)
#define APM_PCM_CLKEN           (1 << 22)
#define APM_BMU_CLKEN           (1 << 21)
#define PCIE_CLK_EN             (1 << 20)
#define GPON_SER_CLK_EN         (1 << 19)
#define IPSEC_CLK_EN            (1 << 18)
#define NAND_CLK_EN             (1 << 17)
#define DISABLE_GLESS           (1 << 16)
#define ROBOSW_DMA_CLK_EN       (1 << 15)
#define APM_CLK_EN              (1 << 14)
#define FAP0_CLK_EN             (1 << 13)
#define ROBOSW_CLK_EN           (1 << 12)
#define HS_SPI_CLK_EN           (1 << 11)
#define USBH_CLK_EN             (1 << 10) /* Only one bit for USB CLK */
#define USBD_CLK_EN             (1 << 10)
#define SPI_CLK_EN              (1 << 9)
#define SWPKT_GPON_CLK_EN       (1 << 8)
#define SWPKT_USB_CLK_EN        (1 << 7)
#define GPON_CLK_EN             (1 << 6)

#define APM_AUDIO_COMMON_CLKEN  0 // TBD007 this is from 6828, where is in 6818?

    uint32        pll_control;       /* (08) word 2 */
#define SOFT_RESET              0x00000001      // 0

    uint32        deviceTimeoutEn;   /* (0c) word 3 */
    uint32        softResetB;        /* (10) word 4 */
#define SOFT_RST_GPON_UBUS2     (1 << 24)
#define SOFT_RST_SERDES_DIG     (1 << 23)
#define SOFT_RST_SERDES         (1 << 22)
#define SOFT_RST_SERDES_MDIO    (1 << 21)
#define SOFT_RST_SERDES_PLL     (1 << 20)
#define SOFT_RST_SERDES_HW      (1 << 19)
#define SOFT_RST_GPON           (1 << 18)
#define SOFT_RST_BMU            (1 << 17)
#define SOFT_RST_HVG            (1 << 16)
#define SOFT_RST_APM            (1 << 15)
#define SOFT_RST_ACP            (1 << 14)
#define SOFT_RST_PCM            (1 << 13)
#define SOFT_RST_USBH           (1 << 12)
#define SOFT_RST_USBD           (1 << 11)
#define SOFT_RST_SWITCH         (1 << 10)
#define SOFT_RST_FAP1           (1 << 9)
#define SOFT_RST_PCIE_HARD      (1 << 8)
#define SOFT_RST_FAP0           (1 << 7)
#define SOFT_RST_EPHY           (1 << 6)
#define SOFT_RST_PCIE           (1 << 5)
#define SOFT_RST_IPSEC          (1 << 4)
#define SOFT_RST_MPI            (1 << 3)
#define SOFT_RST_PCIE_EXT       (1 << 2)
#define SOFT_RST_PCIE_CORE      (1 << 1)
#define SOFT_RST_SPI            (1 << 0)

    uint32        diagControl;        /* (14) word 5 */
    uint32        ExtIrqCfg;          /* (18) word 6*/
    uint32        ExtIrqCfg1;         /* (1c) word 7 */
#define EI_SENSE_SHFT   0
#define EI_STATUS_SHFT  4
#define EI_CLEAR_SHFT   8
#define EI_MASK_SHFT    12
#define EI_INSENS_SHFT  16
#define EI_LEVEL_SHFT   20

     union {
          IrqControl_t     IrqControl[2];    /* (20) (40) */
          IrqControl_32_t  IrqControl32[2];  /* (20) (40)  */
     };
} PerfControl;

#define PERF ((volatile PerfControl * const) PERF_BASE)

/*
** Timer
*/
typedef struct Timer {
    uint16        unused0;
    byte          TimerMask;
#define TIMER0EN        0x01
#define TIMER1EN        0x02
#define TIMER2EN        0x04
    byte          TimerInts;
#define TIMER0          0x01
#define TIMER1          0x02
#define TIMER2          0x04
#define WATCHDOG        0x08
    uint32        TimerCtl0;
    uint32        TimerCtl1;
    uint32        TimerCtl2;
#define TIMERENABLE     0x80000000
#define RSTCNTCLR       0x40000000
    uint32        TimerCnt0;
    uint32        TimerCnt1;
    uint32        TimerCnt2;
    uint32        WatchDogDefCount;

    /* Write 0xff00 0x00ff to Start timer
     * Write 0xee00 0x00ee to Stop and re-load default count
     * Read from this register returns current watch dog count
     */
    uint32        WatchDogCtl;

    /* Number of 50-MHz ticks for WD Reset pulse to last */
    uint32        WDResetCount;

    uint32        EnSwPLL;
    uint32        WakeOnCtrl;
} Timer;

#define TIMER ((volatile Timer * const) TIMR_BASE)

/*
** UART
*/
typedef struct UartChannel {
    byte          unused0;
    byte          control;
#define BRGEN           0x80    /* Control register bit defs */
#define TXEN            0x40
#define RXEN            0x20
#define LOOPBK          0x10
#define TXPARITYEN      0x08
#define TXPARITYEVEN    0x04
#define RXPARITYEN      0x02
#define RXPARITYEVEN    0x01

    byte          config;
#define XMITBREAK       0x40
#define BITS5SYM        0x00
#define BITS6SYM        0x10
#define BITS7SYM        0x20
#define BITS8SYM        0x30
#define ONESTOP         0x07
#define TWOSTOP         0x0f
    /* 4-LSBS represent STOP bits/char
     * in 1/8 bit-time intervals.  Zero
     * represents 1/8 stop bit interval.
     * Fifteen represents 2 stop bits.
     */
    byte          fifoctl;
#define RSTTXFIFOS      0x80
#define RSTRXFIFOS      0x40
    /* 5-bit TimeoutCnt is in low bits of this register.
     *  This count represents the number of characters
     *  idle times before setting receive Irq when below threshold
     */
    uint32        baudword;
    /* When divide SysClk/2/(1+baudword) we should get 32*bit-rate
     */

    byte          txf_levl;       /* Read-only fifo depth */
    byte          rxf_levl;       /* Read-only fifo depth */
    byte          fifocfg;        /* Upper 4-bits are TxThresh, Lower are
                                   *      RxThreshold.  Irq can be asserted
                                   *      when rx fifo> thresh, txfifo<thresh
                                   */
    byte          prog_out;       /* Set value of DTR (Bit0), RTS (Bit1)
                                   *  if these bits are also enabled to GPIO_o
                                   */
#define DTREN   0x01
#define RTSEN   0x02

    byte          unused1;
    byte          DeltaIPEdgeNoSense;     /* Low 4-bits, set corr bit to 1 to
                                           * detect irq on rising AND falling
                                           * edges for corresponding GPIO_i
                                           * if enabled (edge insensitive)
                                           */
    byte          DeltaIPConfig_Mask;     /* Upper 4 bits: 1 for posedge sense
                                           *      0 for negedge sense if
                                           *      not configured for edge
                                           *      insensitive (see above)
                                           * Lower 4 bits: Mask to enable change
                                           *  detection IRQ for corresponding
                                           *  GPIO_i
                                           */
    byte          DeltaIP_SyncIP;         /* Upper 4 bits show which bits
                                           *  have changed (may set IRQ).
                                           *  read automatically clears bit
                                           * Lower 4 bits are actual status
                                           */

    uint16        intMask;                /* Same Bit defs for Mask and status */
    uint16        intStatus;
#define DELTAIP         0x0001
#define TXUNDERR        0x0002
#define TXOVFERR        0x0004
#define TXFIFOTHOLD     0x0008
#define TXREADLATCH     0x0010
#define TXFIFOEMT       0x0020
#define RXUNDERR        0x0040
#define RXOVFERR        0x0080
#define RXTIMEOUT       0x0100
#define RXFIFOFULL      0x0200
#define RXFIFOTHOLD     0x0400
#define RXFIFONE        0x0800
#define RXFRAMERR       0x1000
#define RXPARERR        0x2000
#define RXBRK           0x4000

    uint16        unused2;
    uint16        Data;                   /* Write to TX, Read from RX */
                                          /* bits 11:8 are BRK,PAR,FRM errors */

    uint32        unused3;
    uint32        unused4;
} Uart;

#define UART ((volatile Uart * const) UART_BASE)

/*
** Gpio Controller
*/

typedef struct GpioControl {
    uint64      GPIODir;                    /* 0 */

// TBD. Does this apply to the BCM6818?
// Used in 6818 MII over GPIO function
#define         GPIO_02_MII_RX_DV    (1LLU <<2)
#define         GPIO_09_MII_RXD_0    (1LLU <<9)
#define         GPIO_10_MII_RXD_1    (1LLU <<10)
#define         GPIO_11_MII_RXD_2    (1LLU <<11)
#define         GPIO_12_MII_RXD_3    (1LLU <<12)
#define         GPIO_13_MII_RX_CLK   (1LLU <<13)                
#define         GPIO_30_MII_TXD_0    (1LLU <<30)
#define         GPIO_31_MII_TXD_1    (1LLU <<31)
#define         GPIO_32_MII_TXD_2    (1LLU <<32)
#define         GPIO_33_MII_TXD_3    (1LLU <<33)
#define         GPIO_37_MII_TX_CLK   (1LLU <<37)
#define         GPIO_38_MII_RX_ER    (1LLU <<38)
#define         GPIO_39_MII_TX_EN    (1LLU <<39)

#define         GPIO_MII_OVER_GPIO_OUTPUTS     (GPIO_30_MII_TXD_0 | GPIO_31_MII_TXD_1 | GPIO_32_MII_TXD_2 | GPIO_33_MII_TXD_3 | GPIO_39_MII_TX_EN)  
#define         GPIO_MII_OVER_GPIO_INPUTS      (GPIO_02_MII_RX_DV | GPIO_09_MII_RXD_0 | GPIO_10_MII_RXD_1 | GPIO_11_MII_RXD_2 | GPIO_12_MII_RXD_3 | GPIO_13_MII_RX_CLK | GPIO_37_MII_TX_CLK | GPIO_38_MII_RX_ER)  

#define         GPIO_31_UART2_SDOUT  (1LLU <<31)
#define         GPIO_27_UART2_SDOUT  (1LLU <<27)
#define         GPIO_3_UART2_SDOUT   (1LLU <<3)
#define         GPIO_30_UART2_SIN    (1LLU <<30)

    uint64      GPIOio;                     /* 8 */
    uint32      LEDCtrl;
#define LED_ALL_STROBE          0x0f000000
#define LED3_STROBE             0x08000000
#define LED2_STROBE             0x04000000
#define LED1_STROBE             0x02000000
#define LED0_STROBE             0x01000000
#define LED_TEST                0x00010000
#define DISABLE_LINK_ACT_ALL    0x0000f000
#define DISABLE_LINK_ACT_3      0x00008000
#define DISABLE_LINK_ACT_2      0x00004000
#define DISABLE_LINK_ACT_1      0x00002000
#define DISABLE_LINK_ACT_0      0x00001000
#define LED_INTERVAL_SET_MASK   0x00000f00
#define LED_INTERVAL_SET_1280MS 0x00000600
#define LED_INTERVAL_SET_320MS  0x00000500
#define LED_INTERVAL_SET_160MS  0x00000400
#define LED_INTERVAL_SET_80MS   0x00000300
#define LED_INTERVAL_SET_40MS   0x00000200
#define LED_INTERVAL_SET_20MS   0x00000100
#define LED_ON_ALL              0x000000f0
#define LED_ON_3                0x00000080
#define LED_ON_2                0x00000040
#define LED_ON_1                0x00000020
#define LED_ON_0                0x00000010
    uint32      SpiSlaveCfg;                /* 14 */
    uint32      GPIOMode;                   /* 18 */
#define GPIO_MODE_SPI_SSN5          (1<<31)
#define GPIO_MODE_SPI_SSN4          (1<<30)
#define GPIO_MODE_SPI_SSN3          (1<<29)
#define GPIO_MODE_SPI_SSN2          (1<<28)
#define GPIO_MODE_SDOUT             (1<<27)
#define GPIO_MODE_GPN_SUP_FRAME     (1<<26)
#define GPIO_MODE_APM_PCM_CLK       (1<<25)
#define GPIO_MODE_APM_PCM_SDIN      (1<<24)
#define GPIO_MODE_APM_PCM_SDOUT     (1<<23)
#define GPIO_MODE_APM_PCM_FS        (1<<22)
#define GPIO_MODE_APM_DOUT_3        (1<<21)
#define GPIO_MODE_APM_DOUT_2        (1<<20)
#define GPIO_MODE_APM_DOUT_1        (1<<19)
#define GPIO_MODE_APM_DOUT_0_B      (1<<18)
#define GPIO_MODE_APM_DOUT_0_A      (1<<17)
#define GPIO_MODE_NTR_PULSE         (1<<15)
#define GPIO_MODE_USBD_LED          (1<<14)
#define GPIO_MODE_ROBOSW_LED1       (1<<13)
#define GPIO_MODE_ROBOSW_LED0       (1<<12)
#define GPIO_MODE_ROBOSW_LED_CLK    (1<<11)
#define GPIO_MODE_ROBOSW_LED_DATA   (1<<10)
#define GPIO_MODE_GPON_LED          (1<<8)
#define GPIO_MODE_GPHY1_LED         (1<<7)
#define GPIO_MODE_GPHY0_LED         (1<<6)
#define GPIO_MODE_APM_HVG_MAX_PWM   (1<<5)
#define GPIO_MODE_SERIAL_LED_CLK    (1<<4)
#define GPIO_MODE_SERIAL_LED_DATA   (1<<3)
#define GPIO_MODE_SYS_IRQ           (1<<2)
#define GPIO_MODE_GPON_TX_APC_FAIL  (1<<1) 
#define GPIO_MODE_GPON_TX_EN_L      (1<<0)
/* GPIO mode definition for 6818 pin compatible devices */
#define COMPAT_GPIO_MODE_APM_DOUT_3         (1<<29)
#define COMPAT_GPIO_MODE_SERIAL_LED_DATA    (1<<27)
#define COMPAT_GPIO_MODE_SPI_SSN3           (1<<21)
#define COMPAT_GPIO_MODE_GPON_TX_EN_L       (1<<4)
#define COMPAT_GPIO_MODE_SDOUT              (1<<3)
#define COMPAT_GPIO_MODE_SERIAL_LED_CLK     (1<<0)

    uint32      reserved;                   /* 1C */

    uint32      AuxLedInterval;             /* 20 */
#define AUX_LED_IN_7            0x80000000
#define AUX_LED_IN_6            0x40000000
#define AUX_LED_IN_5            0x20000000
#define AUX_LED_IN_4            0x10000000
#define AUX_LED_IN_MASK         0xf0000000
#define LED_IN_3                0x08000000
#define LED_IN_2                0x04000000
#define LED_IN_1                0x02000000
#define LED_IN_0                0x01000000
#define AUX_LED_TEST            0x00400000
#define USE_NEW_INTV            0x00200000
#define LED7_LNK_ORAND          0x00100000
#define LED7_LNK_MASK           0x000f0000
#define LED7_LNK_MASK_SHFT      16
#define LED7_ACT_MASK           0x0000f000
#define LED7_ACT_MASK_SHFT      12
#define AUX_FLASH_INTV          0x00000fc0
#define AUX_FLASH_INTV_100MS    0x00000140
#define AUX_FLASH_INTV_SHFT     6
#define AUX_BLINK_INTV          0x0000003f
#define AUX_BLINK_INTV_60MS     0x00000003
    uint32      AuxLedCtrl;                 /* 24 */
#define AUX_HW_DISAB_7          0x80000000
#define AUX_STROBE_7            0x40000000
#define AUX_MODE_7              0x30000000
#define AUX_MODE_SHFT_7         28
#define AUX_HW_DISAB_6          0x08000000
#define AUX_STROBE_6            0x04000000
#define AUX_MODE_6              0x03000000
#define AUX_MODE_SHFT_6         24
#define AUX_HW_DISAB_5          0x00800000
#define AUX_STROBE_5            0x00400000
#define AUX_MODE_5              0x00300000
#define AUX_MODE_SHFT_5         20
#define AUX_HW_DISAB_4          0x00080000
#define AUX_STROBE_4            0x00040000
#define AUX_MODE_4              0x00030000
#define AUX_MODE_SHFT_4         16
#define AUX_HW_DISAB_3          0x00008000
#define AUX_STROBE_3            0x00004000
#define AUX_MODE_3              0x00003000
#define AUX_MODE_SHFT_3         12
#define AUX_HW_DISAB_2          0x00000800
#define AUX_STROBE_2            0x00000400
#define AUX_MODE_2              0x00000300
#define AUX_MODE_SHFT_2         8
#define AUX_HW_DISAB_1          0x00000080
#define AUX_STROBE_1            0x00000040
#define AUX_MODE_1              0x00000030
#define AUX_MODE_SHFT_1         4
#define AUX_HW_DISAB_0          0x00000008
#define AUX_STROBE_0            0x00000004
#define AUX_MODE_0              0x00000003
#define AUX_MODE_SHFT_0         0

#define LED_STEADY_OFF          0x0
#define LED_FLASH               0x1
#define LED_BLINK               0x2
#define LED_STEADY_ON           0x3

    uint32      TestControl;                /* 28 */

    uint32      OscControl;                 /* 2C */
    uint32      RoboSWLEDControl;           /* 30 */
    uint32      RoboSWLEDLSR;               /* 34 */
    uint32      GPIOBaseMode;               /* 38 */
#define EN_SEL_GMAC_VS_GPON     (1<<22)    
#define EN_6828_REMAP           (1<<21)    
#define EN_GMII3                (1<<18)
#define EN_GMII2                (1<<17)
#define EN_GMII1                (1<<16)
#define EN_MII_OVER_GPIO        (6<<00)
#define EN_UART2_OVER_GPIO      (1<<00)
#define EN_ZARLINK_SLIC_IF_LE9540 (1<<3)

    uint32      RoboswEphyCtrl;             /* 3C */
#define RSW_BC_SUPP_EN          (1<<26)
#define RSW_HW_FWDG_EN          (1<<19)
#define RSW_MII_DUMB_FWDG_EN    (1<<16)
#define GPHY_PWR_DOWN_SD_1      (1<<4)
#define GPHY_PWR_DOWN_SD_0      (1<<3)
#define GPHY_PWR_DOWN_1         (1<<2)
#define GPHY_PWR_DOWN_0         (1<<1)
#define GPHY_PWR_DOWN_BIAS      (1<<0)

    uint32      MiiPadCtrl;                 /* 40 */
/* Silicon bug. when read, the mii_over_gpio 4 bits are shift by 1 bit to the right, write is ok */
#define MII_OVER_GPIO_SLEW_MASK (1<<31)
#define MII_OVER_GPIO_SEL_MASK  (0x7<<28)
#define MII_OVER_GPIO_MASK      (0xf<<28)
#define MII_OVER_GPIO_RD_MASK   (0xf<<27)

#define RGMII_3_AMP_EN          (1<<8)
#define GMII_2_AMP_EN           (1<<5)
#define GMII_1_AMP_EN           (1<<2)

#define RGMII_3_SEL_MASK        (0x3<<6)
#define RGMII_3_SEL_SHIFT       6
#define RGMII_3_SEL_3P3V        (0x0<<RGMII_3_SEL_SHIFT)
#define RGMII_3_SEL_2P5V        (0x1<<RGMII_3_SEL_SHIFT)
#define RGMII_3_SEL_1P5V        (0x2<<RGMII_3_SEL_SHIFT)

#define GMII_2_SEL_MASK         (0x3<<3)
#define GMII_2_SEL_SHIFT        3
#define GMII_2_SEL_3P3V         (0x0<<GMII_2_SEL_SHIFT)
#define GMII_2_SEL_2P5V         (0x1<<GMII_2_SEL_SHIFT)
#define GMII_2_SEL_1P5V         (0x2<<GMII_2_SEL_SHIFT)

#define GMII_1_SEL_MASK         (0x3)
#define GMII_1_SEL_SHIFT        0
#define GMII_1_SEL_3P3V         (0x0<<GMII_1_SEL_SHIFT)
#define GMII_1_SEL_2P5V         (0x1<<GMII_1_SEL_SHIFT)
#define GMII_1_SEL_1P5V         (0x2<<GMII_1_SEL_SHIFT)

    uint32      unused1[1];                 /* 44 */

    uint32      RingOscCtrl0;               /* 48 */
#define RING_OSC_256_CYCLES        8
#define RING_OSC_512_CYCLES        9
#define RING_OSC_1024_CYCLES       10

    uint32      RingOscCtrl1;               /* 4C */
#define RING_OSC_ENABLE_MASK       (0xf7<<24)
#define RING_OSC_ENABLE_SHIFT      24
#define RING_OSC_MAX               8
#define RING_OSC_COUNT_RESET       (0x1<<23)
#define RING_OSC_SELECT_MASK       (0x7<<20)
#define RING_OSC_SELECT_SHIFT      20
#define RING_OSC_IRQ               (0x1<<18)
#define RING_OSC_COUNTER_OVERFLOW  (0x1<<17)
#define RING_OSC_COUNTER_BUSY      (0x1<<16)
#define RING_OSC_COUNT_MASK        0x0000ffff

    uint32      SerialLed;                  /* 50 */
    uint32      SerialLedCtrl;              /* 54 */
#define SER_LED_BUSY            (1<<3)
#define SER_LED_POLARITY        (1<<2)
#define SER_LED_DIV_1           0
#define SER_LED_DIV_2           1
#define SER_LED_DIV_4           2
#define SER_LED_DIV_8           3
#define SER_LED_DIV_MASK        0x3
#define SER_LED_DIV_SHIFT       0
    uint32    SerialLedBlink;               /* 58 */
    uint32    SerdesCtl;                    /* 5c */
#define SERDES_PCIE_EXD_ENABLE                  (1<<15)
#define SERDES_PCIE_ENABLE  0x00000001
    uint32    SerdesStatus;                 /* 60 */
    uint32    SataPhyPwrdwn;                /* 64 */
    uint32    DieRevID;                     /* 68 */
    uint32    unused2;                      /* 6c */
    //uint32    DiagMemStatus;              /* 6c */
    uint32    DiagSelControl;               /* 70 */
    uint32    DiagReadBack;                 /* 74 */
    uint32    DiagReadBackHi;               /* 78 */
    uint32    DiagMiscControl;              /* 7c */
} GpioControl;

#define GPIO ((volatile GpioControl * const) GPIO_BASE)

/* Number to mask conversion macro used for GPIODir and GPIOio */
#define GPIO_NUM_MAX                    40
#define GPIO_NUM_TO_MASK(X)             ( (((X) & BP_GPIO_NUM_MASK) < GPIO_NUM_MAX) ? ((uint64)1 << ((X) & BP_GPIO_NUM_MASK)) : (0) )

/*
** Spi Controller
*/

typedef struct SpiControl {
  uint16        spiMsgCtl;              /* (0x0) control byte */
#define FULL_DUPLEX_RW                  0
#define HALF_DUPLEX_W                   1
#define HALF_DUPLEX_R                   2
#define SPI_MSG_TYPE_SHIFT              14
#define SPI_BYTE_CNT_SHIFT              0
  byte          spiMsgData[0x21e];      /* (0x02 - 0x21f) msg data */
  byte          unused0[0x1e0];
  byte          spiRxDataFifo[0x220];   /* (0x400 - 0x61f) rx data */
  byte          unused1[0xe0];

  uint16        spiCmd;                 /* (0x700): SPI command */
#define SPI_CMD_NOOP                    0
#define SPI_CMD_SOFT_RESET              1
#define SPI_CMD_HARD_RESET              2
#define SPI_CMD_START_IMMEDIATE         3

#define SPI_CMD_COMMAND_SHIFT           0
#define SPI_CMD_COMMAND_MASK            0x000f

#define SPI_CMD_DEVICE_ID_SHIFT         4
#define SPI_CMD_PREPEND_BYTE_CNT_SHIFT  8
#define SPI_CMD_ONE_BYTE_SHIFT          11
#define SPI_CMD_ONE_WIRE_SHIFT          12
#define SPI_DEV_ID_0                    0
#define SPI_DEV_ID_1                    1
#define SPI_DEV_ID_2                    2
#define SPI_DEV_ID_3                    3
#define ZSI_SPI_DEV_ID                  7

  byte          spiIntStatus;           /* (0x702): SPI interrupt status */
  byte          spiMaskIntStatus;       /* (0x703): SPI masked interrupt status */

  byte          spiIntMask;             /* (0x704): SPI interrupt mask */
#define SPI_INTR_CMD_DONE               0x01
#define SPI_INTR_RX_OVERFLOW            0x02
#define SPI_INTR_INTR_TX_UNDERFLOW      0x04
#define SPI_INTR_TX_OVERFLOW            0x08
#define SPI_INTR_RX_UNDERFLOW           0x10
#define SPI_INTR_CLEAR_ALL              0x1f

  byte          spiStatus;              /* (0x705): SPI status */
#define SPI_RX_EMPTY                    0x02
#define SPI_CMD_BUSY                    0x04
#define SPI_SERIAL_BUSY                 0x08

  byte          spiClkCfg;              /* (0x706): SPI clock configuration */
#define SPI_CLK_0_391MHZ                1
#define SPI_CLK_0_781MHZ                2 /* default */
#define SPI_CLK_1_563MHZ                3
#define SPI_CLK_3_125MHZ                4
#define SPI_CLK_6_250MHZ                5
#define SPI_CLK_12_50MHZ                6
#define SPI_CLK_MASK                    0x07
#define SPI_SSOFFTIME_MASK              0x38
#define SPI_SSOFFTIME_SHIFT             3
#define SPI_BYTE_SWAP                   0x80

  byte          spiFillByte;            /* (0x707): SPI fill byte */
  byte          unused2;
  byte          spiMsgTail;             /* (0x709): msgtail */
  byte          unused3;
  byte          spiRxTail;              /* (0x70B): rxtail */
} SpiControl;


#define SPI ((volatile SpiControl * const) SPI_BASE)


/*
** High-Speed SPI Controller
*/

#define	__mask(end, start)		(((1 << ((end - start) + 1)) - 1) << start)
typedef struct HsSpiControl {

  uint32	hs_spiGlobalCtrl;	// 0x0000
#define	HS_SPI_MOSI_IDLE		(1 << 18)
#define	HS_SPI_CLK_POLARITY		(1 << 17)
#define	HS_SPI_CLK_GATE_SSOFF		(1 << 16)
#define	HS_SPI_PLL_CLK_CTRL		(8)
#define	HS_SPI_PLL_CLK_CTRL_MASK	__mask(15, HS_SPI_PLL_CLK_CTRL)
#define	HS_SPI_SS_POLARITY		(0)
#define	HS_SPI_SS_POLARITY_MASK		__mask(7, HS_SPI_SS_POLARITY)

  uint32	hs_spiExtTrigCtrl;	// 0x0004
#define	HS_SPI_TRIG_RAW_STATE		(24)
#define	HS_SPI_TRIG_RAW_STATE_MASK	__mask(31, HS_SPI_TRIG_RAW_STATE)
#define	HS_SPI_TRIG_LATCHED		(16)
#define	HS_SPI_TRIG_LATCHED_MASK	__mask(23, HS_SPI_TRIG_LATCHED)
#define	HS_SPI_TRIG_SENSE		(8)
#define	HS_SPI_TRIG_SENSE_MASK		__mask(15, HS_SPI_TRIG_SENSE)
#define	HS_SPI_TRIG_TYPE		(0)
#define	HS_SPI_TRIG_TYPE_MASK		__mask(7, HS_SPI_TRIG_TYPE)
#define	HS_SPI_TRIG_TYPE_EDGE		(0)
#define	HS_SPI_TRIG_TYPE_LEVEL		(1)

  uint32	hs_spiIntStatus;	// 0x0008
#define	HS_SPI_IRQ_PING1_USER		(28)
#define	HS_SPI_IRQ_PING1_USER_MASK	__mask(31, HS_SPI_IRQ_PING1_USER)
#define	HS_SPI_IRQ_PING0_USER		(24)
#define	HS_SPI_IRQ_PING0_USER_MASK	__mask(27, HS_SPI_IRQ_PING0_USER)

#define	HS_SPI_IRQ_PING1_CTRL_INV	(1 << 12)
#define	HS_SPI_IRQ_PING1_POLL_TOUT	(1 << 11)
#define	HS_SPI_IRQ_PING1_TX_UNDER	(1 << 10)
#define	HS_SPI_IRQ_PING1_RX_OVER	(1 << 9)
#define	HS_SPI_IRQ_PING1_CMD_DONE	(1 << 8)

#define	HS_SPI_IRQ_PING0_CTRL_INV	(1 << 4)
#define	HS_SPI_IRQ_PING0_POLL_TOUT	(1 << 3)
#define	HS_SPI_IRQ_PING0_TX_UNDER	(1 << 2)
#define	HS_SPI_IRQ_PING0_RX_OVER	(1 << 1)
#define	HS_SPI_IRQ_PING0_CMD_DONE	(1 << 0)

  uint32	hs_spiIntStatusMasked;	// 0x000C
#define	HS_SPI_IRQSM__PING1_USER	(28)
#define	HS_SPI_IRQSM__PING1_USER_MASK	__mask(31, HS_SPI_IRQSM__PING1_USER)
#define	HS_SPI_IRQSM__PING0_USER	(24)
#define	HS_SPI_IRQSM__PING0_USER_MASK	__mask(27, HS_SPI_IRQSM__PING0_USER)

#define	HS_SPI_IRQSM__PING1_CTRL_INV	(1 << 12)
#define	HS_SPI_IRQSM__PING1_POLL_TOUT	(1 << 11)
#define	HS_SPI_IRQSM__PING1_TX_UNDER	(1 << 10)
#define	HS_SPI_IRQSM__PING1_RX_OVER	(1 << 9)
#define	HS_SPI_IRQSM__PING1_CMD_DONE	(1 << 8)

#define	HS_SPI_IRQSM__PING0_CTRL_INV	(1 << 4)
#define	HS_SPI_IRQSM__PING0_POLL_TOUT	(1 << 3)
#define	HS_SPI_IRQSM__PING0_TX_UNDER	(1 << 2)
#define	HS_SPI_IRQSM__PING0_RX_OVER	(1 << 1)
#define	HS_SPI_IRQSM__PING0_CMD_DONE	(1 << 0)

  uint32	hs_spiIntMask;		// 0x0010
#define	HS_SPI_IRQM_PING1_USER		(28)
#define	HS_SPI_IRQM_PING1_USER_MASK	__mask(31, HS_SPI_IRQM_PING1_USER)
#define	HS_SPI_IRQM_PING0_USER		(24)
#define	HS_SPI_IRQM_PING0_USER_MASK	__mask(27, HS_SPI_IRQM_PING0_USER)

#define	HS_SPI_IRQM_PING1_CTRL_INV	(1 << 12)
#define	HS_SPI_IRQM_PING1_POLL_TOUT	(1 << 11)
#define	HS_SPI_IRQM_PING1_TX_UNDER	(1 << 10)
#define	HS_SPI_IRQM_PING1_RX_OVER	(1 << 9)
#define	HS_SPI_IRQM_PING1_CMD_DONE	(1 << 8)

#define	HS_SPI_IRQM_PING0_CTRL_INV	(1 << 4)
#define	HS_SPI_IRQM_PING0_POLL_TOUT	(1 << 3)
#define	HS_SPI_IRQM_PING0_TX_UNDER	(1 << 2)
#define	HS_SPI_IRQM_PING0_RX_OVER	(1 << 1)
#define	HS_SPI_IRQM_PING0_CMD_DONE	(1 << 0)

#define HS_SPI_INTR_CLEAR_ALL       (0xFF001F1F)

  uint32	hs_spiFlashCtrl;	// 0x0014
#define	HS_SPI_FCTRL_MB_ENABLE		(23)
#define	HS_SPI_FCTRL_SS_NUM			(20)
#define	HS_SPI_FCTRL_SS_NUM_MASK	__mask(22, HS_SPI_FCTRL_SS_NUM)
#define	HS_SPI_FCTRL_PROFILE_NUM	(16)
#define	HS_SPI_FCTRL_PROFILE_NUM_MASK	__mask(18, HS_SPI_FCTRL_PROFILE_NUM)
#define	HS_SPI_FCTRL_DUMMY_BYTES	(10)
#define	HS_SPI_FCTRL_DUMMY_BYTES_MASK	__mask(11, HS_SPI_FCTRL_DUMMY_BYTES)
#define	HS_SPI_FCTRL_ADDR_BYTES		(8)
#define	HS_SPI_FCTRL_ADDR_BYTES_MASK	__mask(9, HS_SPI_FCTRL_ADDR_BYTES)
#define	HS_SPI_FCTRL_ADDR_BYTES_2	(0)
#define	HS_SPI_FCTRL_ADDR_BYTES_3	(1)
#define	HS_SPI_FCTRL_ADDR_BYTES_4	(2)
#define	HS_SPI_FCTRL_READ_OPCODE	(0)
#define	HS_SPI_FCTRL_READ_OPCODE_MASK	__mask(7, HS_SPI_FCTRL_READ_OPCODE)

  uint32	hs_spiFlashAddrBase;	// 0x0018

  char		fill0[0x80 - 0x18];

  uint32	hs_spiPP_0_Cmd;		// 0x0080
#define	HS_SPI_PP_SS_NUM		(12)
#define	HS_SPI_PP_SS_NUM_MASK		__mask(14, HS_SPI_PP_SS_NUM)
#define	HS_SPI_PP_PROFILE_NUM		(8)
#define	HS_SPI_PP_PROFILE_NUM_MASK	__mask(10, HS_SPI_PP_PROFILE_NUM)

} HsSpiControl;

typedef struct HsSpiPingPong {

	uint32 command;
#define HS_SPI_SS_NUM (12)
#define HS_SPI_PROFILE_NUM (8)
#define HS_SPI_TRIGGER_NUM (4)
#define HS_SPI_COMMAND_VALUE (0)
    #define HS_SPI_COMMAND_NOOP (0)
    #define HS_SPI_COMMAND_START_NOW (1)
    #define HS_SPI_COMMAND_START_TRIGGER (2)
    #define HS_SPI_COMMAND_HALT (3)
    #define HS_SPI_COMMAND_FLUSH (4)

	uint32 status;
#define HS_SPI_ERROR_BYTE_OFFSET (16)
#define HS_SPI_WAIT_FOR_TRIGGER (2)
#define HS_SPI_SOURCE_BUSY (1)
#define HS_SPI_SOURCE_GNT (0)

	uint32 fifo_status;
	uint32 control;

} HsSpiPingPong;

typedef struct HsSpiProfile {

	uint32 clk_ctrl;
#define HS_SPI_ACCUM_RST_ON_LOOP (15)
#define HS_SPI_SPI_CLK_2X_SEL (14)
#define HS_SPI_FREQ_CTRL_WORD (0)
	
    uint32 signal_ctrl;
#define   HS_SPI_ASYNC_INPUT_PATH (1 << 16)
#define   HS_SPI_LAUNCH_RISING    (1 << 13)
#define   HS_SPI_LATCH_RISING     (1 << 12)

	uint32 mode_ctrl;
#define HS_SPI_PREPENDBYTE_CNT (24)
#define HS_SPI_MODE_ONE_WIRE (20)
#define HS_SPI_MULTIDATA_WR_SIZE (18)
#define HS_SPI_MULTIDATA_RD_SIZE (16)
#define HS_SPI_MULTIDATA_WR_STRT (12)
#define HS_SPI_MULTIDATA_RD_STRT (8)
#define HS_SPI_FILLBYTE (0)

	uint32 polling_config;
	uint32 polling_and_mask;
	uint32 polling_compare;
	uint32 polling_timeout;
	uint32 reserved;

} HsSpiProfile;

#define HS_SPI_OP_CODE 13
    #define HS_SPI_OP_SLEEP (0)
    #define HS_SPI_OP_READ_WRITE (1)
    #define HS_SPI_OP_WRITE (2)
    #define HS_SPI_OP_READ (3)
    #define HS_SPI_OP_SETIRQ (4)

#define	HS_SPI ((volatile HsSpiControl * const) HSSPIM_BASE)
#define	HS_SPI_PINGPONG0 ((volatile HsSpiPingPong * const) (HSSPIM_BASE+0x80))
#define	HS_SPI_PINGPONG1 ((volatile HsSpiPingPong * const) (HSSPIM_BASE+0xc0))
#define	HS_SPI_PROFILES ((volatile HsSpiProfile * const) (HSSPIM_BASE+0x100))
#define	HS_SPI_FIFO0 ((volatile uint8 * const) (HSSPIM_BASE+0x200))
#define	HS_SPI_FIFO1 ((volatile uint8 * const) (HSSPIM_BASE+0x400))


/*
** Periph - Misc Register Set Definitions.
*/

typedef struct Misc {
  uint32        miscPllCtrl ;              /* (0x00) */

  uint32        miscVregCtrl0;             /* (0x04) */
#define MISC_VREG_CONTROL0_REG_RESET_B         (1<<31)
#define MISC_VREG_CONTROL0_POWER_DOWN_2        (1<<30)
#define MISC_VREG_CONTROL0_POWER_DOWN_1        (1<<29)

  uint32        miscVregCtrl1;             /* (0x08) */
#define VREG_VSEL1P2_SHIFT                         0
#define VREG_VSEL1P2_MASK                      0x1ff
#define MISC_VREG_CONTROL1_VSEL1P2_DEFAULT      0x7d

  uint32        miscVregCtrl2;             /* (0x0C) */
#define MISC_VREG_CONTROL2_SWITCH_CLKEN        (1<<7)

  uint32        miscMemcControl ;          /* (0x10) MEMC Control     */
#define MISC_MEMC_CONTROL_DDR_SLEEP_MODE                           0x00000010
#define MISC_MEMC_CONTROL_MC_UBUS_ASYNC_MODE                       (1<<3)
#define MISC_MEMC_CONTROL_MC_LMB_ASYNC_MODE                        (1<<2)
#define MISC_MEMC_CONTROL_DDR_TEST_DONE                            0x00000002
#define MISC_MEMC_CONTROL_DDR_TEST_DISABLE                         0x00000001
  uint32        miscStrapBus ;             /* (0x14) Strap Register   */
#define MISC_STRAP_BUS_MIPS_PLL_FVCO_SHIFT                         27
#define MISC_STRAP_BUS_MIPS_PLL_FVCO_MASK                          (0x1F<<MISC_STRAP_BUS_MIPS_PLL_FVCO_SHIFT)
#define MISC_STRAP_BUS_HRD_RST_DELAY                               0x04000000
#define MISC_STRAP_BUS_ALT_BFC_EN                                  0x02000000
#define MISC_STRAP_BUS_DDR2_DDR3_SELECT                            0x00800000
#define MISC_STRAP_BUS_IXTAL_ADJ_MASK                              0x00600000
#define MISC_STRAP_BUS_IXTAL_ADJ_SHIFT                             21
#define MISC_STRAP_BUS_BYPASS_XTAL                                 0x00100000
//#define MISC_STRAP_BUS_TS                                          0x00080000
#define MISC_STRAP_BUS_APM_PICO_BOOT_ROM                           0x00040000
#define MISC_STRAP_BUS_TA                                          0x00020000
#define MISC_STRAP_BUS_ROBOSW_2_MODE_MASK                          0x00018000
#define MISC_STRAP_BUS_ROBOSW_2_MODE_SHIFT                         15
#define MISC_STRAP_BUS_ROBOSW_1_MODE_MASK                          0x00006000
#define MISC_STRAP_BUS_ROBOSW_1_MODE_SHIFT                         13
#define MISC_STRAP_BUS_ROBOSW_MODE_RGMII                           0
#define MISC_STRAP_BUS_ROBOSW_MODE_MII                             1
#define MISC_STRAP_BUS_ROBOSW_MODE_GMII                            3
#define MISC_STRAP_BUS_BIST_CLRMEM_N                               0x00001000
#define MISC_STRAP_BUS_PLL_MIPS_WAIT_FAST_N                        0x00000200
#define MISC_STRAP_BUS_PLL_USE_LOCK                                0x00000100
#define MISC_STRAP_BUS_PCIE_ROOT_COMPLEX                           0x00000080
#define MISC_STRAP_BUS_LS_SPIM_ENABLED                             0x00000040
#define MISC_STRAP_BUS_USE_SPI_MASTER                              0x00000020
#define MISC_STRAP_BUS_SPI_CLK_FAST                                0x00000010
#define MISC_STRAP_BUS_SPI_BOOT_DELAY                              0x00000008
#define MISC_STRAP_BUS_MIPS_BOOT16                                 0x00000004
#define MISC_STRAP_BUS_BOOT_SEL_MASK                               0x00000003
#define MISC_STRAP_BUS_BOOT_SEL_SHIFT                              0
#define MISC_STRAP_BUS_BOOT_PARALLEL                               0x03
#define MISC_STRAP_BUS_BOOT_SERIAL                                 0x01
#define MISC_STRAP_BUS_BOOT_NAND                                   0x02
  uint32        miscStrapOverride ;        /* (0x18) Strap Override Reg */
#define MISC_STRAP_OVERRIDE_NAND_FLASH_ECC_SHIFT                   1
#define MISC_STRAP_OVERRIDE_NAND_FLASH_ECC_MASK                    (0x7<<MISC_STRAP_OVERRIDE_NAND_FLASH_ECC_SHIFT)
#define MISC_STRAP_OVERRIDE_NAND_FLASH_ECC_DISABLE                 0x7
#define MISC_STRAP_OVERRIDE_NAND_FLASH_ECC_4_BIT_16B               0x6
#define MISC_STRAP_OVERRIDE_NAND_FLASH_ECC_8_BIT_16B               0x5
#define MISC_STRAP_OVERRIDE_NAND_FLASH_ECC_8_BIT_27B               0x4
#define MISC_STRAP_OVERRIDE_NAND_FLASH_ECC_12_BIT_27B              0x3
#define MISC_STRAP_OVERRIDE_NAND_FLASH_ECC_24_BIT_27B              0x2
#define MISC_STRAP_OVERRIDE_NAND_FLASH_ECC_30_BIT_27B              0x1
#define MISC_STRAP_OVERRIDE_NAND_FLASH_ECC_1_BIT_16B               0x0
#define MISC_STRAP_OVERRIDE_STRAP_OVERRIDE                         0x00000001
  uint32        miscReserved[2];           /* (0x1C) */
  uint32        miscPllMdivCtrl;           /* (0x24) */
  uint32        miscReserved2[1];          /* (0x28) */
  uint32        miscDdrPllOutEnCh ;        /* (0x2C) DDR PLL Out En Ch Number */
#define MISC_DDR_PLL_OUTEN_CH_DDR_PLL_OUTEN_CH_MASK                0x000001FF
#define MISC_DDR_PLL_OUTEN_CH_DDR_PLL_OUTEN_CH_SHIFT               0

  uint32        miscGpioDiagOverlay ;      /* (0x30) GPIO Diag Overlay Control Reg */
#define MISC_GPIO_DIAG_OVERLAY_DIAG_OVERLAY_EN                     0x00000020
#define MISC_GPIO_DIAG_OVERLAY_DIAG_OVERLAY_MASK                   0x0000001F
#define MISC_GPIO_DIAG_OVERLAY_DIAG_OVERLAY_PORT_39_32             0x00000010
#define MISC_GPIO_DIAG_OVERLAY_DIAG_OVERLAY_PORT_31_24             0x00000008
#define MISC_GPIO_DIAG_OVERLAY_DIAG_OVERLAY_PORT_23_16             0x00000004
#define MISC_GPIO_DIAG_OVERLAY_DIAG_OVERLAY_PORT_15_8              0x00000002
#define MISC_GPIO_DIAG_OVERLAY_DIAG_OVERLAY_PORT_7_0               0x00000001
  uint32        miscGpioModeCtrlHi ;       /* (0x34) GPIO Pin Mode Control Reg Hi */
#define GPIO_MODE_APM_HVG_MAX_PWM_38   (1<<6)
  uint32        miscIddqCtrl;              /* (0x38) */
#define MISC_IDDQ_CTRL_GMAC		(1<<16)
#define MISC_IDDQ_CTRL_USBH		(1<<15)
#define MISC_IDDQ_CTRL_APMH		(1<<14)
#define MISC_IDDQ_CTRL_FAP 		(1<<13)
#define MISC_IDDQ_CTRL_GPHY 		(1<<12)
#define MISC_IDDQ_CTRL_ROBOSW 		(1<<11)
#define MISC_IDDQ_CTRL_USBD 		(1<<10)
#define MISC_IDDQ_CTRL_AEPHY 		(1<<9)
#define MISC_IDDQ_CTRL_SATA_PHY		(1<<8)
#define MISC_IDDQ_CTRL_GPON		(1<<7)
#define MISC_IDDQ_CTRL_PCIE		(1<<6)
#define MISC_IDDQ_CTRL_MPI		(1<<5)
#define MISC_IDDQ_CTRL_DOSC		(1<<4)
#define MISC_IDDQ_CTRL_MEMC		(1<<3)
#define MISC_IDDQ_CTRL_MIPS		(1<<2)
#define MISC_IDDQ_CTRL_BRIDGE		(1<<1)
#define MISC_IDDQ_CTRL_IPSEC		(1<<0)

  uint32        miscFapIrqMask;            /* (0x3c) */
#define FAP_IRQ_USBD_DMA_IRQ           (0x3F<<26)
#define FAP_IRQ_EXTERNAL_IRQ_5         (1<<25)
#define FAP_IRQ_EXTERNAL_IRQ_4         (1<<24)
#define FAP_IRQ_EXTERNAL_IRQ_3         (1<<23)
#define FAP_IRQ_EXTERNAL_IRQ_2         (1<<22)
#define FAP_IRQ_EXTERNAL_IRQ_1         (1<<21)
#define FAP_IRQ_EXTERNAL_IRQ_0         (1<<20)
#define FAP_IRQ_FAP1_IRQ               (1<<19)
#define FAP_IRQ_FAP0_IRQ               (1<<18)
#define FAP_IRQ_USB_SOFT_SHUTDOWN_IRQ  (1<<17)
#define FAP_IRQ_PCIE_RC_IRQ            (1<<16)
#define FAP_IRQ_PCIE_EP_IRQ            (1<<15)
#define FAP_IRQ_GPHY_IRQ               (1<<14)
#define FAP_IRQ_GMAC_IRQ               (1<<13)
#define FAP_IRQ_APM_IRQ                (1<<12)
#define FAP_IRQ_GPON_IRQ               (1<<11)
#define FAP_IRQ_NAND_FLASH_IRQ         (1<<10)
#define FAP_IRQ_RING_OSC_IRQ           (1<<9)
#define FAP_IRQ_USBD_IRQ               (1<<8)
#define FAP_IRQ_USBH_IRQ               (1<<7)
#define FAP_IRQ_IPSEC_IRQ              (1<<6)
#define FAP_IRQ_OHCI_IRQ               (1<<5)
#define FAP_IRQ_DYING_GASP_IRQ         (1<<4)
#define FAP_IRQ_UART1_IRQ              (1<<3)
#define FAP_IRQ_UART0_IRQ              (1<<2)
#define FAP_IRQ_SPI_IRQ                (1<<1)
#define FAP_IRQ_TIMR_IRQ               (1<<0)
  uint32        miscFapExtraIrqMask;       /* (0x40) */
#define FAP_EXT_IRQ_I2C_IRQ                (1<<31)
#define FAP_EXT_IRQ_IPSEC_DMA_IRQ1         (1<<29)
#define FAP_EXT_IRQ_IPSEC_DMA_IRQ0         (1<<28)
#define FAP_EXT_IRQ_ROBO_FFE_MBOX_IRQ      (1<<23)
#define FAP_EXT_IRQ_ROBO_SYS_IRQ           (1<<22)
#define FAP_EXT_IRQ_APM_DMA_IRQ            (0x3F<<16)
#define FAP_EXT_IRQ_GPON_DMA_IRQ           (0x3<<14)
#define FAP_EXT_IRQ_GPHY_ENGY_IRQ          (0x3<<12)
#define FAP_EXT_IRQ_FAP1_IRQ               (1<<9)
#define FAP_EXT_IRQ_FAP0_IRQ               (1<<8)
#define FAP_EXT_IRQ_SWITCH_TX_DMA_IRQ_3    (1<<7)
#define FAP_EXT_IRQ_SWITCH_TX_DMA_IRQ_2    (1<<6)
#define FAP_EXT_IRQ_SWITCH_TX_DMA_IRQ_1    (1<<5)
#define FAP_EXT_IRQ_SWITCH_TX_DMA_IRQ_0    (1<<4)
#define FAP_EXT_IRQ_SWITCH_RX_DMA_IRQ_3    (1<<3)
#define FAP_EXT_IRQ_SWITCH_RX_DMA_IRQ_2    (1<<2)
#define FAP_EXT_IRQ_SWITCH_RX_DMA_IRQ_1    (1<<1)
#define FAP_EXT_IRQ_SWITCH_RX_DMA_IRQ_0    (1<<0)

  uint32        miscFap2IrqMask;           /* (0x44) */
  uint32        miscFap2ExtraIrqMask;      /* (0x48) */
} Misc ;

#define MISC ((volatile Misc * const) MISC_BASE)



#define IUDMA_MAX_CHANNELS          8

/*
** DMA Channel Configuration (1 .. 8)
*/
typedef struct DmaChannelCfg {
  uint32        cfg;                    /* (00) assorted configuration */
#define         DMA_ENABLE      0x00000001  /* set to enable channel */
#define         DMA_PKT_HALT    0x00000002  /* idle after an EOP flag is detected */
#define         DMA_BURST_HALT  0x00000004  /* idle after finish current memory burst */
  uint32        intStat;                /* (04) interrupts control and status */
  uint32        intMask;                /* (08) interrupts mask */
#define         DMA_BUFF_DONE   0x00000001  /* buffer done */
#define         DMA_DONE        0x00000002  /* packet xfer complete */
#define         DMA_NO_DESC     0x00000004  /* no valid descriptors */
  uint32        maxBurst;               /* (0C) max burst length permitted */
#define         DMA_DESCSIZE_SEL 0x00040000  /* DMA Descriptor Size Selection */
} DmaChannelCfg;

/*
** DMA State RAM (1 .. 16)
*/
typedef struct DmaStateRam {
  uint32        baseDescPtr;            /* (00) descriptor ring start address */
  uint32        state_data;             /* (04) state/bytes done/ring offset */
  uint32        desc_len_status;        /* (08) buffer descriptor status and len */
  uint32        desc_base_bufptr;       /* (0C) buffer descrpitor current processing */
} DmaStateRam;


/*
** DMA Registers
*/
typedef struct DmaRegs {
    uint32 controller_cfg;              /* (00) controller configuration */
#define DMA_MASTER_EN           0x00000001
#define DMA_FLOWC_CH1_EN        0x00000002
#define DMA_FLOWC_CH3_EN        0x00000004

    // Flow control Ch1
    uint32 flowctl_ch1_thresh_lo;           /* 004 */
    uint32 flowctl_ch1_thresh_hi;           /* 008 */
    uint32 flowctl_ch1_alloc;               /* 00c */
#define DMA_BUF_ALLOC_FORCE     0x80000000

    // Flow control Ch3
    uint32 flowctl_ch3_thresh_lo;           /* 010 */
    uint32 flowctl_ch3_thresh_hi;           /* 014 */
    uint32 flowctl_ch3_alloc;               /* 018 */

    // Flow control Ch5
    uint32 flowctl_ch5_thresh_lo;           /* 01C */
    uint32 flowctl_ch5_thresh_hi;           /* 020 */
    uint32 flowctl_ch5_alloc;               /* 024 */

    // Flow control Ch7
    uint32 flowctl_ch7_thresh_lo;           /* 028 */
    uint32 flowctl_ch7_thresh_hi;           /* 02C */
    uint32 flowctl_ch7_alloc;               /* 030 */

    uint32 ctrl_channel_reset;              /* 034 */
    uint32 ctrl_channel_debug;              /* 038 */
    uint32 reserved1;                       /* 03C */
    uint32 ctrl_global_interrupt_status;    /* 040 */
    uint32 ctrl_global_interrupt_mask;      /* 044 */

    // Unused words
    uint8 reserved2[0x200-0x48];

    // Per channel registers/state ram
    DmaChannelCfg chcfg[IUDMA_MAX_CHANNELS];/* (200-3FF) Channel configuration */

    //Unused
    uint8 reserved3[0xDC00-0xDA7C-4];
    
    union {
        DmaStateRam     s[IUDMA_MAX_CHANNELS];
        uint32          u32[4 * IUDMA_MAX_CHANNELS];
    } stram;                                /* (400-5FF) state ram */
} DmaRegs;

#define SW_DMA ((volatile DmaRegs * const) SWITCH_DMA_BASE)

/*
** DMA Buffer
*/
typedef struct DmaDesc {
  union {
    struct {
        uint16        length;                   /* in bytes of data in buffer */
#define          DMA_DESC_USEFPM    0x8000
#define          DMA_DESC_MULTICAST 0x4000
#define          DMA_DESC_BUFLENGTH 0x0fff
        uint16        status;                   /* buffer status */
#define          DMA_OWN                0x8000  /* cleared by DMA, set by SW */
#define          DMA_EOP                0x4000  /* last buffer in packet */
#define          DMA_SOP                0x2000  /* first buffer in packet */
#define          DMA_WRAP               0x1000  /* */
#define          DMA_PRIO               0x0C00  /* Prio for Tx */
#define          DMA_APPEND_BRCM_TAG    0x0200
#define          DMA_APPEND_CRC         0x0100
#define          USB_ZERO_PKT           (1<< 0) // Set to send zero length packet
    };
    uint32      word0;
  };

  uint32        address;                /* address of data */
} DmaDesc;

/*
** 16 Byte DMA Buffer
*/
typedef union {
    struct {
        uint32 rsvrd2    : 12; /* 31:20 */
        uint32 pktColor  : 2;  /* 19:18 */
        uint32 gemPid    : 12; /* 17: 6 */
        uint32 rsvrd1    : 1;  /*  5: 5 */
        uint32 usQueue   : 5;  /*  4: 0 */
    };
    uint32 word;
}DmaDesc16Ctrl;

typedef struct {
  union {
    struct {
        uint16        length;                   /* in bytes of data in buffer */
#define          DMA_DESC_USEFPM        0x8000
#define          DMA_DESC_MULTICAST     0x4000
#define          DMA_DESC_BUFLENGTH     0x0fff
        uint16        status;                   /* buffer status */
#define          DMA_OWN                0x8000  /* cleared by DMA, set by SW */
#define          DMA_EOP                0x4000  /* last buffer in packet */
#define          DMA_SOP                0x2000  /* first buffer in packet */
#define          DMA_WRAP               0x1000  /* */
#define          DMA_PRIO               0x0C00  /* Prio for Tx */
#define          DMA_APPEND_BRCM_TAG    0x0200
#define          DMA_APPEND_CRC         0x0100
#define          USB_ZERO_PKT           (1<< 0) // Set to send zero length packet
    };
    uint32      word0;
  };

  uint32        address;                 /* address of data */
  union {
      DmaDesc16Ctrl ctrl;
      uint32        control;
        #define         US_QUEUE_ID_MASK             0x0000001F /*  4: 0 */
        #define         RESERVED_1_MASK              0x00000020 /*  5: 5 */
        #define         GEM_PORT_ID_MASK             0x0003FFC0 /* 17: 6 */
        #define         PKT_COLOR_MASK               0x000C0000 /* 19:18 */
        #define         RESERVED_2_MASK              0xFFF00000 /* 31:20 */
  };
  uint32        reserved;
} DmaDesc16;

/*
** External Bus Interface
*/
typedef struct EbiChipSelect {
    uint32    base;                         /* base address in upper 24 bits */
#define EBI_SIZE_8K         0
#define EBI_SIZE_16K        1
#define EBI_SIZE_32K        2
#define EBI_SIZE_64K        3
#define EBI_SIZE_128K       4
#define EBI_SIZE_256K       5
#define EBI_SIZE_512K       6
#define EBI_SIZE_1M         7
#define EBI_SIZE_2M         8
#define EBI_SIZE_4M         9
#define EBI_SIZE_8M         10
#define EBI_SIZE_16M        11
#define EBI_SIZE_32M        12
#define EBI_SIZE_64M        13
#define EBI_SIZE_128M       14
#define EBI_SIZE_256M       15
    uint32    config;
#define EBI_ENABLE          0x00000001      /* .. enable this range */
#define EBI_WAIT_STATES     0x0000000e      /* .. mask for wait states */
#define EBI_WTST_SHIFT      1               /* .. for shifting wait states */
#define EBI_WORD_WIDE       0x00000010      /* .. 16-bit peripheral, else 8 */
//#define EBI_WREN            0x00000020      /* enable posted writes */
//#define EBI_POLARITY        0x00000040      /* .. set to invert something,
//                                        **    don't know what yet */
#define EBI_TS_TA_MODE      0x00000080      /* .. use TS/TA mode */
#define EBI_TS_SEL          0x00000100      /* .. drive tsize, not bs_b */
//#define EBI_FIFO            0x00000200      /* .. use fifo */
#define EBI_RE              0x00000400      /* .. Reverse Endian */
#define EBI_SETUP_SHIFT     16
#define EBI_HOLD_SHIFT      20
#define EBI_SETUP_STATES    0x0f0000
#define EBI_HOLD_STATES     0xf00000
} EbiChipSelect;

typedef struct MpiRegisters {
  EbiChipSelect cs[5];                  /* size chip select configuration */
#define EBI_CS0_BASE            0
#define EBI_CS1_BASE            1
#define EBI_CS2_BASE            2
#define EBI_CS3_BASE            3
#define EBI_CS4_BASE            4
#define EBI_CS5_BASE            5
#define EBI_CS6_BASE            6
#define EBI_CS7_BASE            7
  uint8        unused0[0x44-0x24-4];    /* reserved */
  uint32        ebi_control;            /* ebi control */
#define EBI_ACCESS_TIMEOUT      0x000007FF

} MpiRegisters;

#define MPI ((volatile MpiRegisters * const) MPI_BASE)

typedef struct USBControl {
    uint32 BrtControl1;
    uint32 BrtControl2;
    uint32 BrtStatus1;
    uint32 BrtStatus2;
    uint32 UTMIControl1;
    uint32 TestPortControl;
    uint32 PllControl1;
#define PLLC_REFCLKSEL_MASK     0x00000003
#define PLLC_REFCLKSEL_SHIFT    0
#define PLLC_CLKSEL_MASK        0x0000000c
#define PLLC_CLKSEL_SHIFT       2
#define PLLC_XTAL_PWRDWNB       0x00000010
#define PLLC_PLL_PWRDWNB        0x00000020
#define PLLC_PLL_CALEN          0x00000040
#define PLLC_PHYPLL_BYP         0x00000080
#define PLLC_PLL_RESET          0x00000100
#define PLLC_PLL_IDDQ_PWRDN     0x00000200
#define PLLC_PLL_PWRDN_DELAY    0x00000400
    uint32 SwapControl;
#define USB_DEVICE_SEL          (1<<6)
//#define EHCI_LOGICAL_ADDRESS_EN (1<<5)
#define EHCI_ENDIAN_SWAP        (1<<4)
#define EHCI_DATA_SWAP          (1<<3)
//#define OHCI_LOGICAL_ADDRESS_EN (1<<2)
#define OHCI_ENDIAN_SWAP        (1<<1)
#define OHCI_DATA_SWAP          (1<<0)
    uint32 GenericControl;
#define GC_PLL_SUSPEND_EN       (1<<1)
    uint32 FrameAdjustValue;
    uint32 Setup;
#define USBH_IOC                (1<<4)
    uint32 MDIO;
    uint32 MDIO32;
    uint32 USBSimControl;
#define USBH_OHCI_MEM_REQ_DIS   (1<<1)
} USBControl;

#define USBH ((volatile USBControl * const) USBH_CFG_BASE)

/*
** GPON SERDES Registers
*/
typedef struct GponSerdesRegs {
  uint32 topCfg;
  uint32 swReset;
  uint32 aeCfg;
  uint32 aeStatus;
  uint32 phyCfg;
  uint32 phyStatus;
  uint32 mdioWr;
  uint32 mdioRd;
  uint32 reserved[2];
  uint32 fifoCfg;
  uint32 fifoStatus;
  uint32 patternCfg[4];
  uint32 patternStatus[2];
  uint32 laserCfg;
#define GPON_SERDES_LASERMODE_MASK (3<<30)
#define GPON_SERDES_LASERMODE_NORMAL (0<<30)
#define GPON_SERDES_LASERMODE_FORCE_OFF (1<<30)
#define GPON_SERDES_LASERMODE_FORCE_ON (2<<30)
  uint32 laserStatus;
  uint32 miscCfg;
  uint32 miscStatus;
  uint32 phDet[5];
} GponSerdesRegs;

#define GPON_SERDES ((volatile GponSerdesRegs * const) GPON_SERDES_BASE)


/*
** PCI-E
*/
typedef struct PcieRegs{
  uint32 devVenID;
  uint16 command;
  uint16 status;
  uint32 revIdClassCode;
  uint32 headerTypeLatCacheLineSize;
  uint32 bar1;
  uint32 bar2;
  uint32 priSecBusNo;
#define PCIE_CFG_TYPE1_PRI_SEC_BUS_NO_SUB_BUS_NO_MASK              0x00ff0000
#define PCIE_CFG_TYPE1_PRI_SEC_BUS_NO_SUB_BUS_NO_SHIFT             16
#define PCIE_CFG_TYPE1_PRI_SEC_BUS_NO_SEC_BUS_NO_MASK              0x0000ff00
#define PCIE_CFG_TYPE1_PRI_SEC_BUS_NO_SEC_BUS_NO_SHIFT             8
#define PCIE_CFG_TYPE1_PRI_SEC_BUS_NO_PRI_BUS_NO_MASK              0x000000ff
} PcieRegs;

typedef struct PcieBlk404Regs{
  uint32 unused;          /* 0x404 */
  uint32 config2;         /* 0x408 */
#define PCIE_IP_BLK404_CONFIG_2_BAR1_SIZE_MASK         0x0000000f
#define PCIE_IP_BLK404_CONFIG_2_BAR1_DISABLE           0
  uint32 config3;         /* 0x40c */
  uint32 pmDataA;         /* 0x410 */
  uint32 pmDataB;         /* 0x414 */
} PcieBlk404Regs;

typedef struct PcieBlk428Regs{
   uint32 vpdIntf;        /* 0x428 */
   uint16 unused_g;       /* 0x42c */
   uint16 vpdAddrFlag;    /* 0x42e */
   uint32 vpdData;        /* 0x430 */
   uint32 idVal1;         /* 0x434 */
   uint32 idVal2;         /* 0x438 */
   uint32 idVal3;         /* 0x43c */
#define PCIE_IP_BLK428_ID_VAL3_REVISION_ID_MASK                    0xff000000
#define PCIE_IP_BLK428_ID_VAL3_REVISION_ID_SHIFT                   24
#define PCIE_IP_BLK428_ID_VAL3_CLASS_CODE_MASK                     0x00ffffff
#define PCIE_IP_BLK428_ID_VAL3_CLASS_CODE_SHIFT                    16
#define PCIE_IP_BLK428_ID_VAL3_SUB_CLASS_CODE_SHIFT                 8
}PcieBlk428Regs;


typedef struct PcieBridgeRegs{
   uint32 bar1Remap;       /* 0x0818*/
#define PCIE_BRIDGE_BAR1_REMAP_addr_MASK                    0xffff0000
#define PCIE_BRIDGE_BAR1_REMAP_addr_MASK_SHIFT              16
#define PCIE_BRIDGE_BAR1_REMAP_remap_enable                 (1<<1)
#define PCIE_BRIDGE_BAR1_REMAP_swap_enable                  1

   uint32 bar2Remap;       /* 0x081c*/
#define PCIE_BRIDGE_BAR2_REMAP_addr_MASK                    0xffff0000
#define PCIE_BRIDGE_BAR2_REMAP_addr_MASK_SHIFT              16
#define PCIE_BRIDGE_BAR2_REMAP_remap_enable                 (1<<1)
#define PCIE_BRIDGE_BAR2_REMAP_swap_enable                  1

   uint32 bridgeOptReg1;   /* 0x0820*/
#define PCIE_BRIDGE_OPT_REG1_en_l1_int_status_mask_polarity  (1<<12)
#define PCIE_BRIDGE_OPT_REG1_en_pcie_bridge_hole_detection   (1<<11)
#define PCIE_BRIDGE_OPT_REG1_en_rd_reply_be_fix              (1<<9)
#define PCIE_BRIDGE_OPT_REG1_enable_rd_be_opt                (1<<7)

   uint32 bridgeOptReg2;    /* 0x0824*/
#define PCIE_BRIDGE_OPT_REG2_cfg_type1_func_no_MASK    0xe0000000
#define PCIE_BRIDGE_OPT_REG2_cfg_type1_func_no_SHIFT   29
#define PCIE_BRIDGE_OPT_REG2_cfg_type1_dev_no_MASK     0x1f000000
#define PCIE_BRIDGE_OPT_REG2_cfg_type1_dev_no_SHIFT    24
#define PCIE_BRIDGE_OPT_REG2_cfg_type1_bus_no_MASK     0x00ff0000
#define PCIE_BRIDGE_OPT_REG2_cfg_type1_bus_no_SHIFT    16
#define PCIE_BRIDGE_OPT_REG2_cfg_type1_bd_sel_MASK     0x00000080
#define PCIE_BRIDGE_OPT_REG2_cfg_type1_bd_sel_SHIFT    7
#define PCIE_BRIDGE_OPT_REG2_dis_pcie_timeout_MASK     0x00000040
#define PCIE_BRIDGE_OPT_REG2_dis_pcie_timeout_SHIFT    6
#define PCIE_BRIDGE_OPT_REG2_dis_pcie_abort_MASK       0x00000020
#define PCIE_BRIDGE_OPT_REG2_dis_pcie_abort_SHIFT      5
#define PCIE_BRIDGE_OPT_REG2_enable_tx_crd_chk_MASK    0x00000010
#define PCIE_BRIDGE_OPT_REG2_enable_tx_crd_chk_SHIFT   4
#define PCIE_BRIDGE_OPT_REG2_dis_ubus_ur_decode_MASK   0x00000004
#define PCIE_BRIDGE_OPT_REG2_dis_ubus_ur_decode_SHIFT  2
#define PCIE_BRIDGE_OPT_REG2_cfg_reserved_MASK         0x0000ff0b

   uint32 Ubus2PcieBar0BaseMask; /* 0x0828 */
#define PCIE_BRIDGE_BAR0_BASE_base_MASK                     0xfff00000
#define PCIE_BRIDGE_BAR0_BASE_base_MASK_SHIFT               20
#define PCIE_BRIDGE_BAR0_BASE_mask_MASK                     0x0000fff0
#define PCIE_BRIDGE_BAR0_BASE_mask_MASK_SHIFT               4
#define PCIE_BRIDGE_BAR0_BASE_swap_enable                   (1<<1)
#define PCIE_BRIDGE_BAR0_BASE_remap_enable                  1

   uint32 Ubus2PcieBar0RemapAdd; /* 0x082c */
#define PCIE_BRIDGE_BAR0_REMAP_ADDR_addr_MASK               0xfff00000
#define PCIE_BRIDGE_BAR0_REMAP_ADDR_addr_SHIFT              20

   uint32 Ubus2PcieBar1BaseMask; /* 0x0830 */
#define PCIE_BRIDGE_BAR1_BASE_base_MASK                     0xfff00000
#define PCIE_BRIDGE_BAR1_BASE_base_MASK_SHIFT               20
#define PCIE_BRIDGE_BAR1_BASE_mask_MASK                     0x0000fff0
#define PCIE_BRIDGE_BAR1_BASE_mask_MASK_SHIFT               4
#define PCIE_BRIDGE_BAR1_BASE_swap_enable                   (1<<1)
#define PCIE_BRIDGE_BAR1_BASE_remap_enable                  1

   uint32 Ubus2PcieBar1RemapAdd; /* 0x0834 */
#define PCIE_BRIDGE_BAR1_REMAP_ADDR_addr_MASK               0xfff00000
#define PCIE_BRIDGE_BAR1_REMAP_ADDR_addr_SHIFT              20

   uint32 bridgeErrStatus;       /* 0x0838 */
   uint32 bridgeErrMask;         /* 0x083c */
   uint32 coreErrStatus2;        /* 0x0840 */
   uint32 coreErrMask2;          /* 0x0844 */
   uint32 coreErrStatus1;        /* 0x0848 */
   uint32 coreErrMask1;          /* 0x084c */
   uint32 rcInterruptStatus;     /* 0x0850 */
   uint32 rcInterruptMask;       /* 0x0854 */
#define PCIE_BRIDGE_INTERRUPT_MASK_int_a_MASK   (1<<0)
#define PCIE_BRIDGE_INTERRUPT_MASK_int_b_MASK   (1<<1)
#define PCIE_BRIDGE_INTERRUPT_MASK_int_c_MASK   (1<<2)
#define PCIE_BRIDGE_INTERRUPT_MASK_int_d_MASK   (1<<3)

   uint32 ubusToParam;           /* 0x0858 */
   uint32 pcieToParam;           /* 0x085c */
   uint32 toTickParam;           /* 0x0860 */
   uint32 epL1IntMask;           /* 0x0864 */
   uint32 epL1IntStatus;         /* 0x0868 */
   uint32 unused2[4];
   uint32 epL2IntMask;           /* 0x087c */
   uint32 epL2IntSetStatus;      /* 0x0880 */
   uint32 epL2IntStatus;         /* 0x0884 */
   uint32 epCoreLinkRstStatus;   /* 0x0888 */
   uint32 epCoreLinkRstMask;     /* 0x088c */
   uint32 pcieControl;           /* 0x0890 */
#define PCIE_BRIDGE_CLKREQ_ENABLE               (1<<2)

   uint32 pcieStatus;            /* 0x0894 */

} PcieBridgeRegs;

typedef struct PcieBlk1000Regs{
   uint32 undisclosed[18]; /*0x1000 - 0x1044*/
   uint32 dlStatus;        /* 0x1048 */
#define PCIE_IP_BLK1000_DL_STATUS_PHYLINKUP_MASK                   0x00002000
#define PCIE_IP_BLK1000_DL_STATUS_PHYLINKUP_SHIFT                  13
}PcieBlk1000Regs;

typedef struct PcieBlk1800Regs{
#define NUM_PCIE_BLK_1800_PHY_CTRL_REGS         4
  uint32 phyCtrl[NUM_PCIE_BLK_1800_PHY_CTRL_REGS];
#define REG_POWERDOWN_P1PLL_ENA                      (1<<12)
  uint32 phyErrorAttnVec;
  uint32 phyErrorAttnMask;
#define NUM_PCIE_BLK_1800_PHY_LTSSM_HIST_REGS   3
  uint32 phyltssmHist[NUM_PCIE_BLK_1800_PHY_LTSSM_HIST_REGS];
#define NUM_PCIE_BLK_1800_PHY_DBG_REGS          11
  uint32 phyDbg[NUM_PCIE_BLK_1800_PHY_DBG_REGS];
} PcieBlk1800Regs;

#define PCIEH_DEV_OFFSET              0x8000
#define PCIEH                         ((volatile uint32 * const) PCIE_BASE)
#define PCIEH_REGS          ((volatile PcieCfeType1Regs * const) PCIE_BASE)
#define PCIEH_BLK_404_REGS            ((volatile PcieBlk404Regs * const)(PCIE_BASE+0x404))
#define PCIEH_BLK_428_REGS            ((volatile PcieBlk428Regs * const)(PCIE_BASE+0x428))
#define PCIEH_BRIDGE_REGS             ((volatile PcieBridgeRegs * const) (PCIE_BASE+0x2818))
#define PCIEH_BLK_1000_REGS           ((volatile PcieBlk1000Regs * const) (PCIE_BASE+0x1000))
#define PCIEH_BLK_1800_REGS           ((volatile PcieBlk1800Regs * const) (PCIE_BASE+0x1800))
#define PCIEH_MEM1_BASE               0x10100000
#define PCIEH_MEM1_SIZE               0x00100000
#define PCIEH_MEM2_BASE               0xa0000000
#define PCIEH_MEM2_SIZE               0x01000000

typedef struct EthSwMIBRegs {
    unsigned int TxOctetsLo;
    unsigned int TxOctetsHi;
    unsigned int TxDropPkts;
    unsigned int TxQoSPkts;
    unsigned int TxBroadcastPkts;
    unsigned int TxMulticastPkts;
    unsigned int TxUnicastPkts;
    unsigned int TxCol;
    unsigned int TxSingleCol;
    unsigned int TxMultipleCol;
    unsigned int TxDeferredTx;
    unsigned int TxLateCol;
    unsigned int TxExcessiveCol;
    unsigned int TxFrameInDisc;
    unsigned int TxPausePkts;
    unsigned int TxQoSOctetsLo;
    unsigned int TxQoSOctetsHi;
    unsigned int RxOctetsLo;
    unsigned int RxOctetsHi;
    unsigned int RxUndersizePkts;
    unsigned int RxPausePkts;
    unsigned int Pkts64Octets;
    unsigned int Pkts65to127Octets;
    unsigned int Pkts128to255Octets;
    unsigned int Pkts256to511Octets;
    unsigned int Pkts512to1023Octets;
    unsigned int Pkts1024to1522Octets;
    unsigned int RxOversizePkts;
    unsigned int RxJabbers;
    unsigned int RxAlignErrs;
    unsigned int RxFCSErrs;
    unsigned int RxGoodOctetsLo;
    unsigned int RxGoodOctetsHi;
    unsigned int RxDropPkts;
    unsigned int RxUnicastPkts;
    unsigned int RxMulticastPkts;
    unsigned int RxBroadcastPkts;
    unsigned int RxSAChanges;
    unsigned int RxFragments;
    unsigned int RxExcessSizeDisc;
    unsigned int RxSymbolError;
    unsigned int RxQoSPkts;
    unsigned int RxQoSOctetsLo;
    unsigned int RxQoSOctetsHi;
    unsigned int Pkts1523to2047;
    unsigned int Pkts2048to4095;
    unsigned int Pkts4096to8191;
    unsigned int Pkts8192to9728;
} EthSwMIBRegs;

#define ETHSWMIBREG ((volatile EthSwMIBRegs * const) (SWITCH_BASE + 0x2000))

/*
** FAP Control Registers
*/
typedef struct CoprocCtlRegs_S
{
  uint32    irq_4ke_mask;    /* 00 */
  uint32    irq_4ke_status;  /* 04 */
            #define IRQ_FAP_4KE_TIMER                           (1 << 0)
            #define IRQ_FAP_4KE_OUT_FIFO                        (1 << 1)
            #define IRQ_FAP_4KE_IN_FIFO                         (1 << 2)
            #define IRQ_FAP_4KE_DQM                             (1 << 3)
            #define IRQ_FAP_4KE_MBOX_IN                         (1 << 4)
            #define IRQ_FAP_4KE_MBOX_OUT                        (1 << 5)
            #define IRQ_FAP_4KE_GENERAL_PURPOSE_INPUT           (1 << 6)
            #define IRQ_FAP_4KE_ERROR_EB2UBUS_TIMEOUT           (1 << 7)
            #define IRQ_FAP_4KE_ERROR_UB_SLAVE_TIMEOUT          (1 << 8)
            #define IRQ_FAP_4KE_ERROR_UB_SLAVE                  (1 << 9)
            #define IRQ_FAP_4KE_ERROR_UB_MASTER                 (1 << 10)
            #define IRQ_FAP_4KE_ERROR_EB_DQM_OVERFLOW           (1 << 11)
            #define IRQ_FAP_4KE_ERROR_UB_DQM_OVERFLOW           (1 << 12)
            #define IRQ_FAP_4KE_ERROR_DMA0_RX_FIFO_NOT_EMPTY    (1 << 13)
            #define IRQ_FAP_4KE_ERROR_DMA0_TX_FIFO_NOT_EMPTY    (1 << 14)
            #define IRQ_FAP_4KE_ERROR_DMA1_RX_FIFO_NOT_EMPTY    (1 << 15)
            #define IRQ_FAP_4KE_ERROR_DMA1_TX_FIFO_NOT_EMPTY    (1 << 16)
            #define IRQ_FAP_4KE_TIMER_0                         (1 << 17)
            #define IRQ_FAP_4KE_TIMER_1                         (1 << 18)
            #define IRQ_FAP_4KE_TIMER_2                         (1 << 19)
            #define IRQ_FAP_4KE_SMISB_ERROR                     (1 << 20)
            #define IRQ_FAP_4KE_DMA0_RESULT_FIFO_NOT_EMPTY      (1 << 23)
            #define IRQ_FAP_4KE_DMA1_RESULT_FIFO_NOT_EMPTY      (1 << 24)

  uint32    irq_mips_mask;   /* 08 */
  uint32    irq_mips_status; /* 0C */
            #define IRQ_FAP_HOST_TIMER                           (1 << 0)
            #define IRQ_FAP_HOST_DQM                             (1 << 3)
            #define IRQ_FAP_HOST_MBOX_IN                         (1 << 4)
            #define IRQ_FAP_HOST_MBOX_OUT                        (1 << 5)
            #define IRQ_FAP_HOST_GENERAL_PURPOSE_INPUT           (1 << 6)
            #define IRQ_FAP_HOST_ERROR_EB2UBUS_TIMEOUT           (1 << 7)
            #define IRQ_FAP_HOST_ERROR_UB_SLAVE_TIMEOUT          (1 << 8)
            #define IRQ_FAP_HOST_ERROR_UB_SLAVE                  (1 << 9)
            #define IRQ_FAP_HOST_ERROR_UB_MASTER                 (1 << 10)
            #define IRQ_FAP_HOST_ERROR_EB_DQM_OVERFLOW           (1 << 11)
            #define IRQ_FAP_HOST_ERROR_UB_DQM_OVERFLOW           (1 << 12)
            #define IRQ_FAP_HOST_ERROR_DMA0_RX_FIFO_NOT_EMPTY    (1 << 13)
            #define IRQ_FAP_HOST_ERROR_DMA0_TX_FIFO_NOT_EMPTY    (1 << 14)
            #define IRQ_FAP_HOST_ERROR_DMA1_RX_FIFO_NOT_EMPTY    (1 << 15)
            #define IRQ_FAP_HOST_ERROR_DMA1_TX_FIFO_NOT_EMPTY    (1 << 16)
            #define IRQ_FAP_HOST_TIMER_0                         (1 << 17)
            #define IRQ_FAP_HOST_TIMER_1                         (1 << 18)
            #define IRQ_FAP_HOST_TIMER_2                         (1 << 19)
            #define IRQ_FAP_HOST_SMISB_ERROR                     (1 << 20)
            #define IRQ_FAP_HOST_DMA0_RESULT_FIFO_NOT_EMPTY      (1 << 23)
            #define IRQ_FAP_HOST_DMA1_RESULT_FIFO_NOT_EMPTY      (1 << 24)


  uint32    gp_mask;         /* 10 */
  uint32    gp_status;       /* 14 */
            #define IRQ_FAP_GP_TIMER_0                         (1 << 0)
            #define IRQ_FAP_GP_TIMER_1                         (1 << 1)
            #define IRQ_FAP_GP_MBOX_IN                         (1 << 2)
            #define IRQ_FAP_GP_MBOX_OUT                        (1 << 3)
            #define IRQ_FAP_GP_ERROR_EB2UBUS_TIMEOUT           (1 << 4)
            #define IRQ_FAP_GP_ERROR_UB_SLAVE_TIMEOUT          (1 << 5)
            #define IRQ_FAP_GP_ERROR_UB_SLAVE                  (1 << 6)
            #define IRQ_FAP_GP_ERROR_UB_MASTER                 (1 << 7)
            #define IRQ_FAP_GP_ERROR_EB_DQM_OVERFLOW           (1 << 8)
            #define IRQ_FAP_GP_ERROR_UB_DQM_OVERFLOW           (1 << 9)
            #define IRQ_FAP_GP_ERROR_DMA0_RX_FIFO_NOT_EMPTY    (1 << 10)
            #define IRQ_FAP_GP_ERROR_DMA0_TX_FIFO_NOT_EMPTY    (1 << 11)
            #define IRQ_FAP_GP_ERROR_DMA1_RX_FIFO_NOT_EMPTY    (1 << 12)
            #define IRQ_FAP_GP_ERROR_DMA1_TX_FIFO_NOT_EMPTY    (1 << 13)
            #define IRQ_FAP_GP_TIMER_2                         (1 << 14)
            #define IRQ_FAP_GP_SMISB_ERROR                     (1 << 15)
            #define IRQ_FAP_GP_DMA0_RESULT_FIFO_NOT_EMPTY      (1 << 18)
            #define IRQ_FAP_GP_DMA1_RESULT_FIFO_NOT_EMPTY      (1 << 19)

  uint32    gp_tmr0_ctl;     /* 18 */
            #define   TIMER_ENABLE                     0x80000000
            #define   TIMER_MODE_REPEAT                0x40000000
            #define   TIMER_COUNT_MASK                 0x3fffffff
  uint32    gp_tmr0_cnt;     /* 1C */
  uint32    gp_tmr1_ctl;     /* 20 */
  uint32    gp_tmr1_cnt;     /* 24 */
  uint32    host_mbox_in;    /* 28 */
  uint32    host_mbox_out;   /* 2C */
  uint32    gp_out;          /* 30 */
  uint32    gp_in;           /* 34 */
            #define GP_IN_TAM_IRQ_MASK                 0x80000000
            #define GP_IN_SEGDMA_IRQ_MASK              0x00000002
            #define GP_IN_USPP_BUSY_FLAG               0x00000001
  uint32    gp_in_irq_mask;  /* 38 */
            #define GP_IN_BASE4_IRQ_MASK               0x80000000
            #define GP_IN_BASE4_IRQ_SHIFT              31
  uint32    gp_in_irq_status;/* 3C */
            #define GP_IN_IRQ_STATUS_MASK              0x0000FFFF
            #define GP_IN_IRQ_STATUS_SHIFT             0
  uint32    dma_control;     /* 40 */
            #define DMA_NO_WRITE_WITH_ACK              (1<<31)
  uint32    dma_status;      /* 44 */
            #define DMA_STS_DMA1_RSLT_FULL_BIT         (1<<7)
            #define DMA_STS_DMA1_RSLT_EMPTY_BIT        (1<<6)
            #define DMA_STS_DMA1_CMD_FULL_BIT          (1<<5)
            #define DMA_STS_DMA1_BUSY                  (1<<4)
            #define DMA_STS_DMA1_SHIFT                 4
            #define DMA_STS_DMA0_RSLT_FULL_BIT         (1<<3)
            #define DMA_STS_DMA0_RSLT_EMPTY_BIT        (1<<2)
            #define DMA_STS_DMA0_CMD_FULL_BIT          (1<<1)
            #define DMA_STS_DMA0_BUSY                  (1<<0)
  uint32    dma0_3_fifo_status; /* 48 */
            #define DMA_FIFO_STS_DMA1_RSLT_DEPTH_MSK        0x0000F000
            #define DMA_FIFO_STS_DMA1_RSLT_DEPTH_SHIFT      12
            #define DMA_FIFO_STS_DMA1_CMD_ROOM_MSK          0x00000F00
            #define DMA_FIFO_STS_DMA1_CMD_ROOM_SHIFT        8
            #define DMA_FIFO_STS_DMA0_RSLT_FIFO_DEPTH_MSK   0x000000F0
            #define DMA_FIFO_STS_DMA0_RSLT_DEPTH_SHIFT      4
            #define DMA_FIFO_STS_DMA0_CMD_ROOM_MSK          0x0000000F
            #define DMA_FIFO_STS_DMA0_CMD_ROOM_SHIFT        0
  uint32    unused1; /* 4c   //uint32    dma4_7_fifo_status; // 4C */
  uint32    unused2; /* 50   //uint32    dma_irq_sts;        // 50 */
  uint32    unused3; /* 54   //uint32    dma_4ke_irq_mask;   // 54 */
  uint32    unused4; /* 58   //uint32    dma_host_irq_mask;  // 58 */
  uint32    diag_cntrl;         /* 5C */
            #define DIAG_SEL_HI_HI_MSK          0xFF000000
            #define DIAG_SEL_HI_LO_MSK          0x00FF0000
            #define DIAG_SEL_LO_HI_MSK          0x0000FF00
            #define DIAG_SEL_LO_LO_MSK          0x000000FF
  uint32    diag_hi;            /* 60 */
            #define DIAG_HI_HI_MSK              0xFFFF0000
            #define DIAG_HI_LO_MSK              0x0000FFFF
  uint32    diag_lo;            /* 64 */
            #define DIAG_LO_HI_MSK              0xFFFF0000
            #define DIAG_LO_LO_MSK              0x0000FFFF
  uint32    bad_address;        /* 68 */
  uint32    addr1_mask;         /* 6C */
  uint32    addr1_base_in;      /* 70 */
  uint32    addr1_base_out;     /* 74 */
  uint32    addr2_mask;         /* 78 */
  uint32    addr2_base_in;      /* 7C */
  uint32    addr2_base_out;     /* 80 */
  uint32    scratch;            /* 84 */
  uint32    tm_ctl;             /* 88 */
            #define TM_CTL_BASE_4_MASK          0xFFFF0000
            #define TM_CTL_ICDATA_MASK          0x0000F000
            #define TM_CTL_ICTAG_MASK           0x00000F00
            #define TM_CTL_ICWS_MASK            0x000000F0
            #define TM_CTL_DSPRAM_MASK          0x0000000F
  uint32    soft_resets;        /* 8C active high */
            #define SOFT_RESET_DMA                    0x00000004
            #define SOFT_RESET_BASE4                  0x00000002
            #define SOFT_RESET_4KE                    0x00000001
  uint32    eb2ubus_timeout;    /* 90 */
            #define EB2UBUS_TIMEOUT_EN                0x80000000
            #define EB2UBUS_TIMEOUT_MASK              0x0000FFFF
            #define EB2UBUS_TIMEOUT_SHIFT             0
  uint32    m4ke_core_status;   /* 94 */
            #define M4KE_CORE_REV_ID_EXISTS           (1<<31)
            #define M4KE_CORE_EJ_DEBUGM               (1<<16)
            #define M4KE_CORE_EJ_SRSTE                (1<<15)
            #define M4KE_CORE_EJ_PRRST                (1<<14)
            #define M4KE_CORE_EJ_PERRST               (1<<13)
            #define M4KE_CORE_EJ_SLEEP                (1<<12)
            #define M4KE_CORE_EJ_SI_SWINT_MASK        0x00000C00
            #define M4KE_CORE_EJ_SI_SWINT_SHIFT       10
            #define M4KE_CORE_EJ_SI_IPL_MASK          0x000003F0
            #define M4KE_CORE_EJ_SI_IPL_SHIFT         4
            #define M4KE_CORE_EJ_SI_IACK              (1<<3)
            #define M4KE_CORE_EJ_SI_RP                (1<<2)
            #define M4KE_CORE_EJ_SI_EXL               (1<<1)
            #define M4KE_CORE_EJ_SI_ERL               (1<<0)
  uint32    gp_in_irq_sense;    /* 98 */
            #define GP_IN_SENSE_BASE4                 (1<<31)
            #define GP_IN_SENSE_MASK                  0x7FFFFFFF
            #define GP_IN_SENSE_SHIFT                 0
  uint32    ub_slave_timeout;   /* 9C */
            #define UB_SLAVE_TIMEOUT_EN               0x80000000
            #define UB_SLAVE_TIMEOUT_MASK             0x0000FFFF
            #define UB_SLAVE_TIMEOUT_SHIFT            0
  uint32    diag_en;            /* A0 */
            #define DIAG_EN_DIAG_SEL_OVERRIDE         (1<<31)
            #define DIAG_EN_EJTAG_DINT_MASK           (1<<30)
            #define DIAG_EN_SEL_HI_HI_MASK            0x000000C0
            #define DIAG_EN_SEL_HI_HI_SHIFT           6
            #define DIAG_EN_SEL_HI_LO_MASK            0x00000030
            #define DIAG_EN_SEL_HI_LO_SHIFT           4
            #define DIAG_EN_SEL_LO_HI_MASK            0x0000000C
            #define DIAG_EN_SEL_LO_HI_SHIFT           2
            #define DIAG_EN_SEL_LO_LO_MASK            0x00000003
            #define DIAG_EN_SEL_LO_LO_SHIFT           0
  uint32    dev_timeout;        /* A4 */
  uint32    ubus_error_out_mask;/* A8 */
            #define UBUS_ERROR_BASE4                  (1<<15)
            #define UBUS_ERROR_IN_MSG_FIFO            (1<<3)
            #define UBUS_ERROR_UBSLAVE_TIMEOUT        (1<<2)
            #define UBUS_ERROR_EB2UB_TIMEOUT          (1<<1)
            #define UBUS_ERROR_UB_DQM_OVERFLOW        (1<<0)
  uint32    diag_capt_stop_mask;/* AC */
            #define DIAG_ERROR_BASE4                  (1<<31)
            #define DIAG_ERROR_IN_MSG_FIFO            (1<<3)
            #define DIAG_ERROR_UBSLAVE_TIMEOUT        (1<<2)
            #define DIAG_ERROR_EB2UB_TIMEOUT          (1<<1)
            #define DIAG_ERROR_UB_DQM_OVERFLOW        (1<<0)
  uint32    rev_id;             /* B0 */
  uint32    gp_tmr2_ctl;        /* B4 */
            #define GP_TMR2_ENABLE                      (1<<31)
            #define GP_TMR2_MODE                        (1<<30)
            #define GP_TMR2_COUNT_MASK                  (0x3FFFFFFF)
            #define GP_TMR2_COUNT_SHIFT                 0
  uint32    gp_tmr2_cnt;        /* B8 */
            #define GP_TMR2_CNT_MASK                    (0x3FFFFFFF)
            #define GP_TMR2_CNT_SHIFT                   0
  uint32    legacy_mode;        /* BC */
  uint32    smisb_mon;          /* C0 */
            #define SMISB_MON_MAX_CLEAR                 (1<<31)
            #define SMISB_MON_MAX_TIME_MASK             0x007FFF00
            #define SMISB_MON_MAX_TIME_SHIFT            8
            #define SMISB_MON_TIME_OUT_MASK             0x007FFF00
            #define SMISB_MON_TIME_OUT_SHIFT            8
  uint32    diag_ctl;           /* C4 */
            #define DIAG_CTL_USE_INT_DIAG_HIGH          (1<<31)
            #define DIAG_CTL_HIGH_MODE_MASK             0x70000000
            #define DIAG_CTL_HIGH_MODE_SHIFT            28
            #define DIAG_CTL_HIGH_SEL_MASK              0x00FF0000
            #define DIAG_CTL_HIGH_SEL_SHIFT             16
            #define DIAG_CTL_USE_INT_LOW                (1<<15)
            #define DIAG_CTL_LOW_MODE_MASK              0x00007000
            #define DIAG_CTL_LOW_MODE_SHIFT             12
            #define DIAG_CTL_LOW_SEL_MASK               0x000000FF
            #define DIAG_CTL_LOW_SEL_SHIFT              0
  uint32    diag_stat;         /* C8 */
            #define DIAG_STAT_HI_VALID                  (1<<16)
            #define DIAG_STAT_LO_VALID                  (1<<0)
  uint32    diag_mask;         /* CC */
            #define DIAG_HI_MASK                        0xFFFF0000
            #define DIAG_HI_SHIFT                       16
            #define DIAG_LO_MASK                        0x0000FFFF
            #define DIAG_LO_SHIFT                       0
  uint32    diag_result;        /* D0 */
  uint32    diag_compare;       /* D4 */
  uint32    diag_capture;       /* D8 */
  uint32    daig_count;         /* DC */
  uint32    diag_edge_cnt;      /* E0 */
  uint32    hw_counter_0;       /* E4 */
  uint32    hw_counter_1;       /* E8 */
  uint32    hw_counter_2;       /* EC */
  uint32    hw_counter_3;       /* F0 */
  uint32    unused;             /* F4 */
  uint32    lfsr;               /* F8 */
            #define LFSR_MASK                         0x0000FFFF
            #define LFSR_SHIFT                        0

} CoprocCtlRegs_S;


/*
** FAP Outgoing Msg Fifo
*/
typedef struct OGMsgFifoRegs_S
{
  uint32    og_msg_ctl;    /* 00 */
            #define OG_MSG_LOW_WM_MSK                       (1<<13)
            #define OG_MSG_LOW_WM_WORDS_MASK                0x0000000F
            #define OG_MSG_LOW_WM_WORDS_SHIFT               0
  uint32    og_msg_sts;    /* 04 */
            #define OG_MSG_STS_AVAIL_FIFO_SPC_MASK          0x0000000F
            #define OG_MSG_STS_AVAIL_FIFO_SPC_SHIFT         0
  uint32    resv[14];
  uint32    og_msg_data;   /* 40 */
  uint32    resv2[14];
  uint32    og_msg_data_cont;   /* 7C */
} OGMsgFifoRegs_S;


/*
** FAP Incoming Msg Fifo
*/
typedef struct INMsgFifoRegs_S     /* 200 */
{
  uint32    in_msg_ctl;
            #define   NOT_EMPTY_IRQ_STS_MASK          0x00008000
            #define   NOT_EMPTY_IRQ_STS_OFFSET        15
            #define   ERR_IRQ_STS_MASK                0x00004000
            #define   ERR_IRQ_STS_OFFSET              14
            #define   LOW_WTRMRK_IRQ_STS_MASK         0x00002000
            #define   LOW_WTRMRK_IRQ_MSK_OFFSET       13
            #define   MSG_RCVD_IRQ_STS_MASK           0x00001000
            #define   MSG_RCVD_IRQ_MSK_OFFSET         12
            #define   LOW_WATER_MARK_MASK             0x0000003F
            #define   LOW_WATER_MARK_SHIFT            0
            #define   AVAIL_FIFO_SPACE_MASK           0x0000003F
            #define   AVAIL_FIFO_SPACE_OFFSET         0
  uint32    in_msg_sts;
            #define INMSG_NOT_EMPTY_STS_BIT           0x80000000
            #define INMSG_NOT_EMPTY_STS_SHIFT         31
            #define INMSG_ERR_STS_BIT                 0x40000000
            #define INMSG_ERR_STS_SHIFT               30
            #define INMSG_LOW_WATER_STS_BIT           0x20000000
            #define INMSG_LOW_WATER_STS_SHIFT         29
            #define INMSG_MSG_RX_STS_BIT              0x10000000
            #define INMSG_MSG_RX_STS_SHIFT            28
            #define INMSG_RESERVED1_MASK              0x0fc00000
            #define INMSG_RESERVED1_SHIFT             22
            #define INMSG_NUM_MSGS_MASK               0x003F0000
            #define INMSG_NUM_MSGS_SHIFT              16
            #define INMSG_NOT_EMPTY_IRQ_STS_BIT       0x00008000
            #define INMSG_NOT_EMPTY_IRQ_STS_SHIFT     15
            #define INMSG_ERR_IRQ_STS_BIT             0x00004000
            #define INMSG_ERR_IRQ_STS_SHIFT           14
            #define INMSG_LOW_WATER_IRQ_STS_BIT       0x00002000
            #define INMSG_LOW_WATER_IRQ_STS_SHIFT     13
            #define INMSG_MSG_RX_IRQ_STS_BIT          0x00001000
            #define INMSG_MSG_RX_IRQ_STS_SHIFT        12
            #define INMSG_RESERVED2_MASK              0x00000fc0
            #define INMSG_RESERVED2_SHIFT             6
            #define INMSG_AVAIL_FIFO_SPACE_MASK       0x0000003f
            #define INMSG_AVAIL_FIFO_SPACE_SHIFT      0
  uint32    resv[13];
  uint32    in_msg_last;
  uint32    in_msg_data;
} INMsgFifoRegs_S;



/*
** FAP DMA Registers
*/
typedef struct mDma_regs_S
{
  uint32    dma_source;         /* 00 */
  uint32    dma_dest;           /* 04 */
  uint32    dma_cmd_list;       /* 08 */
            #define DMA_CMD_MEMSET                   0x08000000
            #define DMA_CMD_REPLACE_LENGTH           0x07000000
            #define DMA_CMD_INSERT_LENGTH            0x06000000
            #define DMA_CMD_CHECKSUM2                0x05000000
            #define DMA_CMD_CHECKSUM1                0x04000000
            #define DMA_CMD_DELETE                   0x03000000
            #define DMA_CMD_REPLACE                  0x02000000
            #define DMA_CMD_INSERT                   0x01000000
            #define DMA_CMD_OPCODE_MASK              0xFF000000
            #define DMA_CMD_OPCODE_SHIFT             24
            #define DMA_CMD_OFFSET_MASK              0x00FFFF00
            #define DMA_CMD_OFFSET_SHIFT             8
            #define DMA_CMD_LENGTH_MASK              0x000000FF
            #define DMA_CMD_LENGTH_SHIFT             0
  uint32    dma_len_ctl;        /* 0c */
            #define DMA_CTL_LEN_LENGTH_N_VALUE_MASK  0xFFFC0000
            #define DMA_CTL_LEN_LENGTH_N_VALUE_SHIFT 18
            #define DMA_CTL_LEN_WAIT_BIT             0x00020000
            #define DMA_CTL_LEN_EXEC_CMD_LIST_BIT    0x00010000
            #define DMA_CTL_LEN_DEST_ADDR_MASK       0x0000C000
            #define DMA_CTL_LEN_DEST_IS_TOKEN_MASK   0x0000C000
            #define DMA_CTL_LEN_DEST_IS_TOKEN_SHIFT  14
            #define DMA_CTL_LEN_SRC_IS_TOKEN_BIT     0x00002000
            #define DMA_CTL_LEN_CONTINUE_BIT         0x00001000
            #define DMA_CTL_LEN_LEN_MASK             0x00000FFF
  uint32    dma_rslt_source;    /* 10 */
  uint32    dma_rslt_dest;      /* 14 */
  uint32    dma_rslt_hcs;       /* 18 */
            #define DMA_RSLT_HCS_HCS0_MASK           0x0000FFFF
            #define DMA_RSLT_HCS_HCS0_SHIFT          0
            #define DMA_RSLT_HCS_HCS1_MASK           0xFFFF0000
            #define DMA_RSLT_HCS_HCS1_SHIFT          16
  uint32    dma_rslt_len_stat;  /* 1C */
            #define DMA_RSLT_ERROR_MASK              0x003FE000
            #define DMA_RSLT_NOT_END_CMDS            0x00200000
            #define DMA_RSLT_FLUSHED                 0x00100000
            #define DMA_RSLT_ABORTED                 0x00080000
            #define DMA_RSLT_ERR_CMD_FMT             0x00040000
            #define DMA_RSLT_ERR_DEST                0x00020000
            #define DMA_RSLT_ERR_SRC                 0x00010000
            #define DMA_RSLT_ERR_CMD_LIST            0x00008000
            #define DMA_RSLT_ERR_DEST_LEN            0x00004000
            #define DMA_RSLT_ERR_SRC_LEN             0x00002000
            #define DMA_RSLT_CONTINUE                0x00001000
            #define DMA_RSLT_DMA_LEN                 0x00000FFF
} mDma_regs_S;


typedef struct DMARegs_S
{
  mDma_regs_S    dma_ch[2];
} DMARegs_S;


/* Token Registers */
typedef struct TknIntfRegs_S
{
  uint32    tok_buf_size;    /* 00 */
  uint32    tok_buf_base;    /* 04 */
  uint32    tok_idx2ptr_idx; /* 08 */
  uint32    tok_idx2ptr_ptr; /* 0c */
} TknIntfRegs_S;

/* Performance Measurement Registers on 4ke */
typedef struct PMRegs_S
{
  uint32        DCacheHit;      /* n/a  */
  uint32        DCacheMiss;     /* n/a  */
  uint32        ICacheHit;      /* 08 */
  uint32        ICacheMiss;     /* 0c */
  uint32        InstnComplete;  /* 10 */
  uint32        WTBMerge;       /* n/a  */
  uint32        WTBNoMerge;     /* n/a  */
} PMRegs_S;

/* MessageID Registers */
typedef struct MsgIdRegs_S
{
  uint32    msg_id[64];
} MsgIdRegs_S;



/*
** FAP DQM Registers
*/

typedef struct DQMCtlRegs_S /* 1800 */
{
  uint32        cfg;                        /* 00 */
                #define DQM_CFG_TOT_MEM_SZ_MASK      0xFFFF0000
                #define DQM_CFG_TOT_MEM_SZ_SHIFT     16
                #define DQM_CFG_START_ADDR_MASK      0x0000FFFF
                #define DQM_CFG_START_ADDR_SHIFT     0
  uint32        _4ke_low_wtmk_irq_msk;      /* 04 */
  uint32        mips_low_wtmk_irq_msk;      /* 08 */
  uint32        low_wtmk_irq_sts;           /* 0c */
  uint32        _4ke_not_empty_irq_msk;     /* 10 */
  uint32        mips_not_empty_irq_msk;     /* 14 */
  uint32        not_empty_irq_sts;          /* 18 */
  uint32        queue_rst;                  /* 1c */
  uint32        not_empty_sts;              /* 20 */
  uint32        next_avail_mask;            /* 24 */
  uint32        next_avail_queue;           /* 28 */
} DQMCtlRegs_S;


/* DQM Queue Control */
typedef struct DQMQRegs_S
{
  uint32        size;   /* 00 */
                #define Q_HEAD_PTR_MASK                     0xFFFC0000
                #define Q_HEAD_PTR_SHIFT                    18
                #define Q_TAIL_PTR_MASK                     0x0003FFF0
                #define Q_TAIL_PTR_SHIFT                    4
                #define Q_TOKEN_SIZE_MASK                   0x00000003
                #define Q_TOKEN_SIZE_SHIFT                  0
  uint32        cfgA;   /* 04 */
                #define Q_SIZE_MASK                         0xffff0000
                #define Q_SIZE_SHIFT                        16
                #define Q_START_ADDR_MASK                   0x0000ffff
                #define Q_START_ADDR_SHIFT                  0
  uint32        cfgB;   /* 08 */
                #define Q_NUM_TKNS_MASK                     0x3fff0000
                #define Q_NUM_TKNS_SHIFT                    16
                #define Q_LOW_WATERMARK_MASK                0x00003fff
                #define Q_LOW_WATERMARK_SHIFT               0
  uint32        sts;    /* 0c */
                #define AVAIL_TOKEN_SPACE_MASK              0x00003FFF
} DQMQRegs_S;

typedef struct DQMQCntrlRegs_S
{
  DQMQRegs_S q[32];
} DQMQCntrlRegs_S;

/* DQM Queue Data */
typedef struct DQMQueueDataReg_S
{
  uint32        word0;   /* 00 */
  uint32        word1;   /* 04 */
  uint32        word2;   /* 08 */
  uint32        word3;   /* 0c */
} DQMQueueDataReg_S;

typedef struct DQMQDataRegs_S
{
  DQMQueueDataReg_S q[32];
} DQMQDataRegs_S;


/* DQM Queue MIB */
typedef struct DQMQMibRegs_S
{
  uint32          MIB_NumFull[32];
  uint32          MIB_NumEmpty[32];
  uint32          MIB_TokensPushed[32];

} DQMQMibRegs_S;


#ifdef __cplusplus
}
#endif

#endif

