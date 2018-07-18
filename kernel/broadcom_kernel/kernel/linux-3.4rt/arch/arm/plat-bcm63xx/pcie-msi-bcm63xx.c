/*
<:copyright-BRCM:2013:DUAL/GPL:standard

   Copyright (c) 2013 Broadcom Corporation
   All Rights Reserved

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
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <asm/mach/irq.h>
#include <asm/irq.h>
#include <linux/msi.h>
#include "plat/bcm63xx_pcie.h"
#include <linux/workqueue.h>

/*
*    Static definitions
*/
#define MSI_MAP_SIZE                            PCIE_MSI_IDS_PER_DOMAIN
#define PCIE_MISC_MSI_DATA_CONFIG_MATCH_MAGIC   0x0000BCA0

/*
*    Static structures
*/
struct msi_map_entry {
    bool    used;               /* flag: in use or not */
    u8      index;              /* index into the vector */
    int     irq;                /* Virtual IRQ */
};

struct pcie_rc_msi_info {
    /* flag to specify  msi is anbled for the domain or not */
    bool    msi_enable;

    /* MSI IRQ map entries */
    struct  msi_map_entry     msi_map[MSI_MAP_SIZE];
};


/*
*    Static function declerations
*/

/*
*    Static variables
*/
/* pci msi local structures */
static struct pcie_rc_msi_info    pciercmsiinfo[NUM_CORE] = {
    {
        .msi_enable = 0,
    },
#if defined(PCIEH_1)
    {
        .msi_enable = 0,
    },
#endif
};

/*
*/
static struct irq_chip bcm63xx_irq_chip_msi_pcie[NUM_CORE] = {
    {
        .name = "PCIe0-MSI",
        .irq_mask = mask_msi_irq,
        .irq_unmask = unmask_msi_irq,
        .irq_enable = unmask_msi_irq,
        .irq_disable = mask_msi_irq,
    },
#if defined(PCIEH_1)
    {
        .name = "PCIe1-MSI",
        .irq_mask = mask_msi_irq,
        .irq_unmask = unmask_msi_irq,
        .irq_enable = unmask_msi_irq,
        .irq_disable = mask_msi_irq,
    },
#endif
};

/*
* Initializes all msi irq map entries
*
* return - None
*/
static void msi_map_init(u8 domain)
{
    int i;
    struct msi_map_entry* msi_map = pciercmsiinfo[domain].msi_map;

    for (i = 0; i < MSI_MAP_SIZE; i++) {
        msi_map[i].used = false;
        msi_map[i].index = i;
        msi_map[i].irq = 0;
    }
}

/*
* returns an unused msi irq map
*
* return - pointer to map entry on success else NULL
*/
static struct msi_map_entry *msi_map_get(u8 domain)
{
    struct msi_map_entry* msi_map = pciercmsiinfo[domain].msi_map;
    struct msi_map_entry *retval = NULL;
    int i;

    for (i = 0; i < MSI_MAP_SIZE; i++) {
        if (!msi_map[i].used) {
            retval = msi_map + i;
            retval->irq = INTERRUPT_ID_PCIE_MSI_FIRST + i + domain*MSI_MAP_SIZE;
            retval->used = true;
            break;
        }
    }

    return retval;
}

/*
* Release MSI Irq map
*
* return - None
*/
static void msi_map_release(struct msi_map_entry *entry)
{
    if (entry) {
        entry->used = false;
        entry->irq = 0;
    }
}

/*
* ISR routine for MSI interrupt
*
*  - Clear MSI interrupt status
*  - Call corresponding MSI virtual interrupt
*
* return - always returns IRQ_HANDLED
*/
static irqreturn_t bcm63xx_pcie_msi_isr(int irq, void *arg)
{
    int index;
    u32 reg_val;
    struct bcm63xx_pcie_port* port = (struct bcm63xx_pcie_port*)arg;
    int domain = port->hw_pci.domain;
    struct msi_map_entry* msi_map = pciercmsiinfo[domain].msi_map;


    /* Get the MSI interrupt status */
    reg_val =__raw_readl(port->regs+PCIEH_L2_INTR_CTRL_REGS+offsetof(PcieL2IntrControl,Intr2CpuStatus));
    reg_val &= (PCIE_L2_INTR_CTRL_MSI_CPU_INTR_MASK);

    /* clear the interrupts, as this is an edge triggered interrupt */
    __raw_writel(reg_val, port->regs+PCIEH_L2_INTR_CTRL_REGS+offsetof(PcieL2IntrControl,Intr2CpuClear));

    /* Process all the available MSI interrupts */
    index = 0;

    while (reg_val != 0x00000000) {
        if ( reg_val & ( 1ul << (index+PCIE_L2_INTR_CTRL_MSI_CPU_INTR_SHIFT))) {
            if (index < MSI_MAP_SIZE) {
                if (msi_map[index].used)
                    /* Call the corresponding virtual interrupt */
                    generic_handle_irq(msi_map[index].irq);
                else
                    printk(KERN_INFO "unexpected MSI (1)\n");
            } else {
                /* that's weird who triggered this?*/
                /* just clear it*/
                printk(KERN_INFO "unexpected MSI (2)\n");
            }
            reg_val &= (~( 1ul << (index+PCIE_L2_INTR_CTRL_MSI_CPU_INTR_SHIFT)));
        }
        index++;
    }

    if (index) return IRQ_HANDLED;
    else return IRQ_NONE;
}

/*
* Enable MSI interrupt on the root complex
*
*  - Setup MSI isr, program MSI matching address and data pattern
*  - Enable MSI interrupt vectors at L2
*  - Enable L2 interrupts at L1 and disable INTA-D interrupts at L1
*
* return - true: on msi enable, false: failure to enable msi
*/
static bool bcm63xx_pcie_enable_msi(struct bcm63xx_pcie_port *port)
{
    bool    retval    = false;
    u8        domain = port->hw_pci.domain;
    u32        reg_val;

    /* Initialize only once */
    if (pciercmsiinfo[domain].msi_enable) {
        retval = true;
        goto exit;
    }

    /* Initialize the local map structure */
    msi_map_init(domain);

    /* Register MSI interrupt with OS */
    if (request_irq(port->irq, bcm63xx_pcie_msi_isr, IRQF_SHARED,
                    bcm63xx_irq_chip_msi_pcie[domain].name, port)) {
        pr_err("%s: Cannot register IRQ %u\n", __func__, port->irq);
        goto exit;
    }

    printk(KERN_INFO "Using irq=%d for PCIE-MSI interrupts\r\n",port->irq);

    /* Program the Root Complex Registers for matching address hi and low */
    /* The address should be unique with in the down stream/up stream BAR mapping */
    __raw_writel((PCIE_MISC_MSI_BAR_CONFIG_LO_MATCH_ADDR_MASK|PCIE_MISC_MSI_BAR_CONFIG_LO_ENABLE_MASK),
                port->regs+PCIEH_MISC_REGS+offsetof(PcieMiscRegs,msi_bar_config_lo));
    __raw_writel(0,port->regs+PCIEH_MISC_REGS+offsetof(PcieMiscRegs,msi_bar_config_hi));

    /* Program the RC registers for matching data pattern */
    reg_val = PCIE_MISC_MSI_DATA_CONFIG_MATCH_MASK;
    reg_val &= ((~(MSI_MAP_SIZE-1))<<PCIE_MISC_MSI_DATA_CONFIG_MATCH_SHIFT);
    reg_val |= PCIE_MISC_MSI_DATA_CONFIG_MATCH_MAGIC;
    __raw_writel(reg_val, port->regs+PCIEH_MISC_REGS+offsetof(PcieMiscRegs,msi_data_config));


    /* Clear all MSI interrupts initially */
    __raw_writel(PCIE_L2_INTR_CTRL_MSI_CPU_INTR_MASK, port->regs+PCIEH_L2_INTR_CTRL_REGS+offsetof(PcieL2IntrControl,Intr2CpuClear));


    /* enable all available MSI vectors */
    __raw_writel(PCIE_L2_INTR_CTRL_MSI_CPU_INTR_MASK, port->regs+PCIEH_L2_INTR_CTRL_REGS+offsetof(PcieL2IntrControl,Intr2CpuMask_clear));

    /* Enable L2 Intr2 controller interrupt */
    __raw_writel(PCIE_CPU_INTR1_PCIE_INTR_CPU_INTR, port->regs+PCIEH_CPU_INTR1_REGS+offsetof(PcieCpuL1Intr1Regs,maskClear));

    set_irq_flags(port->irq, IRQF_VALID);

    /* Set the flag to specify MSI is enabled */
    pciercmsiinfo[domain].msi_enable = true;

    retval = true;

exit:

    return retval;
}


/*
* Clear the previous setup virtual MSI interrupt
*
* return - None
*/
void arch_teardown_msi_irq(unsigned int irq)
{
    int i, d;

    /* find existance of msi irq in all domains */
    for (d = 0; d < NUM_CORE; d++) {
        if (pciercmsiinfo[d].msi_enable == true) {
            for (i = 0; i < MSI_MAP_SIZE; i++) {
                if ((pciercmsiinfo[d].msi_map[i].used) && (pciercmsiinfo[d].msi_map[i].irq == irq)) {
                    /* Free the resources */
                    irq_free_desc(irq);
                    msi_map_release(pciercmsiinfo[d].msi_map + i);
                    break;
                }
            }
        }
    }
}

/*
* setup architecture specific initialization for msi interrupt
* - enable msi in the root complex,
* - get a virtual irq,
* - setup msi address & data on the EP
*
* return 0: on success, <0: on failure
*/
int arch_setup_msi_irq(struct pci_dev *pdev, struct msi_desc *desc)
{

    int retval = -EINVAL;
    struct msi_msg msg;
    struct msi_map_entry *map_entry = NULL;
    struct bcm63xx_pcie_port *port = NULL;

    port = (struct bcm63xx_pcie_port*)((struct pci_sys_data*)pdev->bus->sysdata)->private_data;

    /* Enable MSI at RC */
    if (!bcm63xx_pcie_enable_msi(port))
        goto exit;

    /*
     * Get an unused IRQ map entry and set the irq descriptors
     */
    map_entry = msi_map_get(port->hw_pci.domain);
    if (map_entry == NULL)
        goto exit;

    retval = irq_alloc_desc(map_entry->irq);
    if (retval < 0)
        goto exit;

    irq_set_chip_and_handler(map_entry->irq,
                            &bcm63xx_irq_chip_msi_pcie[port->hw_pci.domain],
                            handle_simple_irq);

    retval = irq_set_msi_desc(map_entry->irq, desc);
    if (retval < 0)
        goto exit;

    set_irq_flags(map_entry->irq, IRQF_VALID);


    /*
     * Program the msi matching address and data pattern on the EP
    */
    /* Get the address from RC and mask the enable bit */
    /* 32 bit address only */
    msg.address_lo = __raw_readl(port->regs+PCIEH_MISC_REGS+offsetof(PcieMiscRegs,msi_bar_config_lo));
    msg.address_lo &= PCIE_MISC_MSI_BAR_CONFIG_LO_MATCH_ADDR_MASK;
    msg.address_hi = 0;
    msg.data = (PCIE_MISC_MSI_DATA_CONFIG_MATCH_MAGIC | map_entry->index);
    write_msi_msg(map_entry->irq, &msg);

    retval = 0;

exit:
    if (retval != 0) {
        pr_err(" arch_setup_msi_irq returned error %d\r\n",retval);
        if (map_entry) {
            irq_free_desc(map_entry->irq);
            msi_map_release(map_entry);
        }
    }

    return retval;
}
