/*
    Copyright 2000-2010 Broadcom Corporation

    Unless you and Broadcom execute a separate written software license
    agreement governing use of this software, this software is licensed
    to you under the terms of the GNU General Public License version 2
    (the “GPL”), available at http://www.broadcom.com/licenses/GPLv2.php,
    with the following added to such license:

        As a special exception, the copyright holders of this software give
        you permission to link this software with independent modules, and to
        copy and distribute the resulting executable under terms of your
        choice, provided that you also meet, for each linked independent
        module, the terms and conditions of the license of that module. 
        An independent module is a module which is not derived from this
        software.  The special exception does not apply to any modifications
        of the software.

    Notwithstanding the above, under no circumstances may you combine this
    software in any way with any other Broadcom software provided under a
    license other than the GPL, without Broadcom's express prior written
    consent.
*/                       

#ifndef __6828_INTR_H
#define __6828_INTR_H

#ifdef __cplusplus
    extern "C" {
#endif

#define INTERRUPT_ID_SOFTWARE_0           0
#define INTERRUPT_ID_SOFTWARE_1           1

/*=====================================================================*/
/* BCM6828 Timer Interrupt Level Assignments                          */
/*=====================================================================*/
#define MIPS_TIMER_INT                  7

/*=====================================================================*/
/* Peripheral ISR Table Offset                                              */
/*=====================================================================*/
#define INTERNAL_ISR_TABLE_OFFSET       8
#define INTERNAL_HIGH_ISR_TABLE_OFFSET  (INTERNAL_ISR_TABLE_OFFSET + 32)

/*=====================================================================*/
/* Logical Peripheral Interrupt IDs                                    */
/*=====================================================================*/
#define INTERRUPT_ID_TIMER               (INTERNAL_ISR_TABLE_OFFSET + 0)
#define INTERRUPT_ID_ENETSW_RX_DMA_0     (INTERNAL_ISR_TABLE_OFFSET + 1)
#define INTERRUPT_ID_ENETSW_RX_DMA_1     (INTERNAL_ISR_TABLE_OFFSET + 2)
#define INTERRUPT_ID_ENETSW_RX_DMA_2     (INTERNAL_ISR_TABLE_OFFSET + 3)
#define INTERRUPT_ID_ENETSW_RX_DMA_3     (INTERNAL_ISR_TABLE_OFFSET + 4)
#define INTERRUPT_ID_UART                (INTERNAL_ISR_TABLE_OFFSET + 5)
#define INTERRUPT_ID_HS_SPIM             (INTERNAL_ISR_TABLE_OFFSET + 6)
#define INTERRUPT_ID_GPHY0               (INTERNAL_ISR_TABLE_OFFSET + 7)
#define INTERRUPT_ID_GPHY1               (INTERNAL_ISR_TABLE_OFFSET + 8)
#define INTERRUPT_ID_USBH                (INTERNAL_ISR_TABLE_OFFSET + 9)
#define INTERRUPT_ID_USBH20              (INTERNAL_ISR_TABLE_OFFSET + 10)
#define INTERRUPT_ID_USBS                (INTERNAL_ISR_TABLE_OFFSET + 11)
#define INTERRUPT_ID_APM                 (INTERNAL_ISR_TABLE_OFFSET + 12)
#define INTERRUPT_ID_EPHY                (INTERNAL_ISR_TABLE_OFFSET + 13)
#define INTERRUPT_ID_EPHY_ENERGY_0       (INTERNAL_ISR_TABLE_OFFSET + 14)
#define INTERRUPT_ID_EPHY_ENERGY_1       (INTERNAL_ISR_TABLE_OFFSET + 15)
#define INTERRUPT_ID_EPON                (INTERNAL_ISR_TABLE_OFFSET + 16)
#define INTERRUPT_ID_GPHY_ENERGY_0       (INTERNAL_ISR_TABLE_OFFSET + 17)
#define INTERRUPT_ID_GPHY_ENERGY_1       (INTERNAL_ISR_TABLE_OFFSET + 18)
#define INTERRUPT_ID_USB_CNTL_RX_DMA     (INTERNAL_ISR_TABLE_OFFSET + 19)
#define INTERRUPT_ID_USB_BULK_RX_DMA     (INTERNAL_ISR_TABLE_OFFSET + 20)
#define INTERRUPT_ID_USB_ISO_RX_DMA      (INTERNAL_ISR_TABLE_OFFSET + 21)
#define INTERRUPT_ID_FAP_0               (INTERNAL_ISR_TABLE_OFFSET + 22)
#define INTERRUPT_ID_FAP_1               (INTERNAL_ISR_TABLE_OFFSET + 23)
#define INTERRUPT_ID_APM_DMA0            (INTERNAL_ISR_TABLE_OFFSET + 24)
#define INTERRUPT_ID_APM_DMA2            (INTERNAL_ISR_TABLE_OFFSET + 25)
#define INTERRUPT_ID_APM_DMA4            (INTERNAL_ISR_TABLE_OFFSET + 26)
#define INTERRUPT_ID_WAKE_ON_IRQ         (INTERNAL_ISR_TABLE_OFFSET + 27)
#define INTERRUPT_ID_DG                  (INTERNAL_ISR_TABLE_OFFSET + 28)
#define INTERRUPT_ID_USB_SOFT_SHUTDOWN   (INTERNAL_ISR_TABLE_OFFSET + 29)


#define INTERRUPT_ID_APM_DMA1            (INTERNAL_HIGH_ISR_TABLE_OFFSET + 0)
#define INTERRUPT_ID_APM_DMA3            (INTERNAL_HIGH_ISR_TABLE_OFFSET + 1)
#define INTERRUPT_ID_APM_DMA5            (INTERNAL_HIGH_ISR_TABLE_OFFSET + 2)
#define INTERRUPT_ID_UART1               (INTERNAL_HIGH_ISR_TABLE_OFFSET + 3)
#define INTERRUPT_ID_USB_CNTL_TX_DMA     (INTERNAL_HIGH_ISR_TABLE_OFFSET + 4)
#define INTERRUPT_ID_USB_BULK_TX_DMA     (INTERNAL_HIGH_ISR_TABLE_OFFSET + 5)
#define INTERRUPT_ID_USB_ISO_TX_DMA      (INTERNAL_HIGH_ISR_TABLE_OFFSET + 6)
#define INTERRUPT_ID_PCIE_RC             (INTERNAL_HIGH_ISR_TABLE_OFFSET + 7)
#define INTERRUPT_ID_PCIE_EP             (INTERNAL_HIGH_ISR_TABLE_OFFSET + 8)
#define INTERRUPT_ID_PER_MBOX0           (INTERNAL_HIGH_ISR_TABLE_OFFSET + 9)
#define INTERRUPT_ID_PER_MBOX1           (INTERNAL_HIGH_ISR_TABLE_OFFSET + 10)
#define INTERRUPT_ID_PER_MBOX2           (INTERNAL_HIGH_ISR_TABLE_OFFSET + 11)
#define INTERRUPT_ID_PER_MBOX3           (INTERNAL_HIGH_ISR_TABLE_OFFSET + 12)
#define INTERRUPT_ID_EXTERNAL_0          (INTERNAL_HIGH_ISR_TABLE_OFFSET + 13)
#define INTERRUPT_ID_EXTERNAL_1          (INTERNAL_HIGH_ISR_TABLE_OFFSET + 14)
#define INTERRUPT_ID_EXTERNAL_2          (INTERNAL_HIGH_ISR_TABLE_OFFSET + 15)
#define INTERRUPT_ID_EXTERNAL_3          (INTERNAL_HIGH_ISR_TABLE_OFFSET + 16)
#define INTERRUPT_ID_ENETSW_SYS          (INTERNAL_HIGH_ISR_TABLE_OFFSET + 17)
#define INTERRUPT_ID_NAND_FLASH          (INTERNAL_HIGH_ISR_TABLE_OFFSET + 18)
#define INTERRUPT_ID_LS_SPIM             (INTERNAL_HIGH_ISR_TABLE_OFFSET + 19)
#define INTERRUPT_ID_RING_OSC            (INTERNAL_HIGH_ISR_TABLE_OFFSET + 20)
#define INTERRUPT_ID_USB_CONNECT         (INTERNAL_HIGH_ISR_TABLE_OFFSET + 21)
#define INTERRUPT_ID_USB_DISCONNECT      (INTERNAL_HIGH_ISR_TABLE_OFFSET + 22)
#define INTERRUPT_ID_ENETSW_TX_DMA_0     (INTERNAL_HIGH_ISR_TABLE_OFFSET + 23)
#define INTERRUPT_ID_ENETSW_TX_DMA_1     (INTERNAL_HIGH_ISR_TABLE_OFFSET + 24)
#define INTERRUPT_ID_ENETSW_TX_DMA_2     (INTERNAL_HIGH_ISR_TABLE_OFFSET + 25)
#define INTERRUPT_ID_ENETSW_TX_DMA_3     (INTERNAL_HIGH_ISR_TABLE_OFFSET + 26)


#define INTERRUPT_ID_LAST                INTERRUPT_ID_ENETSW_TX_DMA_3



#ifdef __cplusplus
    }
#endif                    

#endif  /* __BCM6828_H */


