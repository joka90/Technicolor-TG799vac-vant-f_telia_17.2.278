/*
   <:copyright-BRCM:2012:DUAL/GPL:standard
   
      Copyright (c) 2012 Broadcom Corporation
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

/** Includes. **/
#include "lib_types.h"
#include "lib_printf.h"
#include "lib_string.h"

#include "bcm_map_part.h"  

#include "bcmtypes.h"
#include "bcm_hwdefs.h"
#include "flash_api.h"
#include "bcmSpiRes.h"

#if defined(CFG_RAMAPP)
#include "lib_malloc.h"
#include "jffs2.h"
#endif

//#undef DEBUG_NAND
//#define DEBUG_NAND
#if defined(DEBUG_NAND) && defined(CFG_RAMAPP)
#define DBG_PRINTF printf
#else
#define DBG_PRINTF(...)
#endif

/* Command codes for the flash_command routine */
#define FLASH_PROG          0x02    /* program load data to cache */
#define FLASH_READ          0x03    /* read data from cache */
#define FLASH_WRDI          0x04    /* reset write enable latch */
#define FLASH_WREN          0x06    /* set write enable latch */
#define FLASH_READ_FAST     0x0B    /* read data from cache */
#define FLASH_GFEAT         0x0F    /* get feature option */
#define FLASH_PEXEC         0x10    /* program cache data to memory array*/
#define FLASH_PREAD         0x13    /* read from memory array to cache */
#define FLASH_SFEAT         0x1F    /* set feature option */
#define FLASH_PROG_RAN      0x84    /* program load data to cache at offset*/
#define FLASH_BERASE        0xD8    /* erase one block in memory array */
#define FLASH_RDID          0x9F    /* read manufacturer and product id */
#define FLASH_RESET         0xFF    /* reset flash */

#define FEATURE_PROT_ADDR   0xA0
#define FEATURE_FEAT_ADDR   0xB0
#define FEATURE_STAT_ADDR   0xC0
#define FEATURE_STAT_AUX    0xF0

/* Feature protectin bit defintion */
#define PROT_BRWD           0x80
#define PROT_BP_MASK        0x38
#define PROT_BP_SHIFT       0x3
#define PROT_BP_ALL         0x7
#define PROT_BP_NONE        0x0
/* giga device only */
#define PROT_INV            0x04
#define PROT_CMP            0x02

/* Feature feature bit defintion */
#define FEAT_OPT_EN         0x40
#define FEAT_ECC_EN         0x10
#define FEAT_DISABLE        0x0
/* giga device only */
#define FEAT_BBI            0x04
#define FEAT_QE             0x01

/* Feature status bit definition */
#define STAT_ECC_MASK       0x30
#define STAT_ECC_GOOD       0x00
#define STAT_ECC_CORR       0x10  /* correctable error*/
#define STAT_ECC_UNCORR     0x20  /* uncorrectable error*/
#define STAT_PFAIL          0x8   /* program fail*/
#define STAT_EFAIL          0x4   /* erase fail*/
#define STAT_WEL            0x2   /* write enable latch*/
#define STAT_OIP            0x1   /* operation in progress*/

/* Return codes from flash_status */
#define STATUS_READY        0       /* ready for action */
#define STATUS_BUSY         1       /* operation in progress */
#define STATUS_TIMEOUT      2       /* operation timed out */
#define STATUS_ERROR        3       /* unclassified but unhappy status */

/* Micron manufacturer ID */
#define MICRONPART          0x2C
#define ID_MT29F1G01        0x12
#define ID_MT29F2G01        0x22
#define ID_MT29F4G01        0x32
 
/* Giga Device manufacturer ID */
#define GIGADEVPART         0xC8
#define ID_GD5F1GQ4UA       0xF1
#define ID_GD5F1GQ4UB       0xD1
#define ID_GD5F2GQ4UB       0xD2
#define ID_GD5F4GQ4UB       0xD4

#define SPI_MAKE_ID(A,B)    \
    (((unsigned short) (A) << 8) | ((unsigned short) B & 0xff))

#define SPI_MANU_ID(devId)    \
    ((unsigned char)((devId>>8)&0xff))

#define SPI_NAND_DEVICES                                                \
    {{SPI_MAKE_ID(GIGADEVPART, ID_GD5F1GQ4UA), "GigaDevice GD5F1GQ4UA"},   \
     {SPI_MAKE_ID(GIGADEVPART, ID_GD5F1GQ4UB), "GigaDevice GD5F1GQ4UB"},  \
     {SPI_MAKE_ID(GIGADEVPART, ID_GD5F2GQ4UB), "GigaDevice GD5F2GQ4UB"},  \
     {SPI_MAKE_ID(GIGADEVPART, ID_GD5F4GQ4UB), "GigaDevice GD5F4GQ4UB"},  \
     {SPI_MAKE_ID(MICRONPART, ID_MT29F1G01),  "Micron MT29F1G01"},      \
     {SPI_MAKE_ID(MICRONPART, ID_MT29F2G01),  "Micron MT29F2G01"},      \
     {SPI_MAKE_ID(MICRONPART, ID_MT29F4G01),  "Micron MT29F4G01"},      \
     {0,""}                                                             \
    }

#define SPI_NAND_MANUFACTURERS           \
  {{MICRONPART, "Micron"},               \
   {GIGADEVPART, "GigaDevice"},          \
   {0,""}                                \
  }
/*
typedef struct SpareLayout
{
    unsigned char sl_bi_ofs[2];
    unsigned char sl_spare_mask[];
} SPARE_LAYOUT, *PSPARE_LAYOUT;
*/
typedef struct CfeSpiNandChip
{
//    char *chip_name;
    unsigned int chip_device_id;
    unsigned long chip_total_size;
    unsigned int chip_num_blocks;
    unsigned int chip_block_size;
    unsigned int chip_page_size;
#if defined(CFG_RAMAPP)
    unsigned int chip_spare_size;
    struct nand_ecclayout *ecclayout;
    unsigned int chip_ecc_offset;
    char chip_ecc_corr; // threshold to fix correctable bits
#endif
    unsigned short chip_block_shift;
    unsigned short chip_page_shift;
    unsigned short chip_num_planes;
    unsigned int chip_pages_per_block;
} CFE_SPI_NAND_CHIP, *PCFE_SPI_NAND_CHIP;

CFE_SPI_NAND_CHIP g_spinand_chip;


#if defined(CFG_RAMAPP)
static int spi_nand_write_buf(unsigned short blk, int offset, unsigned char *buffer, int len);
static int spi_nand_write_page(unsigned long page_addr, unsigned int page_offset, unsigned char *buffer, int len);
static int spi_nand_write_enable(void);
static int spi_nand_is_blk_bad(PCFE_SPI_NAND_CHIP pchip, unsigned short blk);
static void spi_nand_mark_bad_blk(PCFE_SPI_NAND_CHIP pchip, unsigned long block_addr);
static int spi_nand_write_enable(void);
static int spi_nand_write_disable(void);
static void spi_nand_row_addr(PCFE_SPI_NAND_CHIP pchip, unsigned int page_addr, unsigned char* buf);
static void spi_nand_col_addr(PCFE_SPI_NAND_CHIP pchip, unsigned int page_addr, unsigned int page_offset, unsigned char* buf);
static int spi_nand_sector_erase_blk(unsigned short blk);
static int spi_nand_get_blk(int addr);
static int spi_nand_wel(void);
static unsigned char *spi_nand_get_memptr(unsigned short blk);
static int spi_nand_get_total_size(void);
static int spi_nand_dev_specific_cmd(unsigned int command, void * inBuf, void * outBuf);

int spi_nand_is_cleanmarker(unsigned long addr, int write_if_not);
static void spi_nand_place_jffs2_clean_marker(unsigned char * buf);

static struct flash_name_from_id fnfi[] = SPI_NAND_DEVICES;

int spi_nand_init(flash_device_info_t **flash_info);
int spi_nand_read_page(unsigned long page_addr, unsigned int page_offset, unsigned char *buffer, int len);
#else
int spi_nand_init(void);
static int spi_nand_read_page(unsigned long page_addr, unsigned int page_offset, unsigned char *buffer, int len);
#endif

static unsigned short spi_nand_get_device_id(void);

static int spiRead( struct spi_transfer *xfer );
static int spiWrite( unsigned char *msg_buf, int nbytes );
static int spi_nand_read_cfg(PCFE_SPI_NAND_CHIP pchip);

static int spi_nand_status(void);
static int spi_nand_reset(void);
static int spi_nand_ready(void);
static int spi_nand_ecc(void);
int spi_nand_read_buf(unsigned short blk, int offset, unsigned char *buffer, int len);

static int spi_nand_get_feat(unsigned char feat_addr);
static void spi_nand_set_feat(unsigned char feat_addr, unsigned char feat_val);

int spi_nand_get_sector_size(unsigned short blk);
int spi_nand_get_numsectors(void);

extern void cfe_usleep(int usec);

#if defined(CFG_RAMAPP)
/** Variables. **/
static flash_device_info_t flash_spi_nand_dev =
    {
        0xffff,
        FLASH_IFC_SPINAND,
        "",
        spi_nand_sector_erase_blk, // fn_flash_sector_erase_int in flash_api.c
        spi_nand_read_buf,         // fn_flash_read_buf in flash_api.c
        spi_nand_write_buf,        // fn_flash_write_buf in flash_api.c
        spi_nand_get_numsectors,   // fn_flash_get_numsectors in flash_api.c
        spi_nand_get_sector_size,  // fn_flash_get_sector_size in flash_api.c
        spi_nand_get_memptr,       // fn_flash_get_memptr in flash_api.c
        spi_nand_get_blk,          // fn_flash_get_blk in flash_api.c
        spi_nand_get_total_size,   // fn_flash_get_total_size in flash_api.c
        spi_nand_dev_specific_cmd  // fn_flash_dev_specific_cmd in flash_api.c
    };

// persistant flags for spi_nand_dev_specific_cmd function
static int g_no_ecc = 0;
static int g_force_erase = 0;

struct nand_ecclayout
{
    int eccbytes;
    int oobavail;
    int eccpos[];
//    int oobfree[];
};

static struct nand_ecclayout spinand_oob_gigadevice =
    {
    .eccbytes = 17,
    .eccpos = { // for ease of use, call the bad block marker an ECC byte as well
        0, 12, 13, 14, 15, // these must be in numerical order
        28, 29, 30, 31,
        44, 45, 46, 47,
        60, 61, 62, 63
    },
//    .oobavail = 47,
    .oobavail = 12, // supposed to be per 512 bytes? JFFS2 multiplies by 4 to find available OOB size
/*
    .oobfree =
    {
        {.offset = 1,
         .length = 11},
        {.offset = 16,
         .length = 12},
        {.offset = 24,
         .length = 12},
        {.offset = 48,
         .length = 12}
    }
*/
};

static struct nand_ecclayout spinand_oob_micron =
{
    .eccbytes = 33,
    .eccpos = { // for ease of use, call the bad block marker an ECC byte as well
        0, 8, 9, 10, 11, 12, 13, 14, 15, // these must be in numerical order
        24, 25, 26, 27, 28, 29, 30, 31,
        40, 41, 42, 43, 44, 45, 46, 47,
        56, 57, 58, 59, 60, 61, 62, 63
    },
//    .oobavail = 8, // per 512 bytes? JFFS2 multiplies by 4 to find available OOB size
/*
    .oobfree = {
        {.offset = 1,
         .length = 7},
        {.offset = 16,
         .length = 8},
        {.offset = 24,
         .length = 8},
        {.offset = 48,
         .length = 8}
    }
*/
};
#endif


/* the controller will handle operations that are greater than the FIFO size
   code that relies on READ_BUF_LEN_MAX, READ_BUF_LEN_MIN or spi_max_op_len
   could be changed */
#define READ_BUF_LEN_MAX   544    /* largest of the maximum transaction sizes for SPI */
#define READ_BUF_LEN_MIN   60     /* smallest of the maximum transaction sizes for SPI */
#define SPI_BUF_LEN        512    /* largest of the maximum transaction sizes for SPI */
/* this is the slave ID of the SPI flash for use with the SPI controller */
#define SPI_FLASH_SLAVE_DEV_ID    0
/* clock defines for the flash */
#define SPI_FLASH_DEF_CLOCK       781000
#define SPARE_MAX_SIZE          (27 * 16)
#define CTRLR_CACHE_SIZE        512
#define ECC_MASK_BIT(ECCMSK, OFS)   (ECCMSK[OFS / 8] & (1 << (OFS % 8)))

/* default to smallest transaction size - updated later */
static unsigned int spi_max_op_len = READ_BUF_LEN_MIN; 
static int spi_dummy_bytes         = 0;

/* default to legacy controller - updated later */
static int spi_flash_clock  = SPI_FLASH_DEF_CLOCK;
static int spi_flash_busnum = LEG_SPI_BUS_NUM;

#if defined(CFG_RAMAPP)
#if defined(__ARMEL__)
static const unsigned short jffs2_clean_marker[] = {JFFS2_MAGIC_BITMASK, JFFS2_NODETYPE_CLEANMARKER, 0x0008, 0x0000};
#else
static const unsigned short jffs2_clean_marker[] = {JFFS2_MAGIC_BITMASK, JFFS2_NODETYPE_CLEANMARKER, 0x0000, 0x0008};
#endif
#endif

static int spiRead(struct spi_transfer *xfer)
{
    return(BcmSpi_MultibitRead(xfer, spi_flash_busnum, SPI_FLASH_SLAVE_DEV_ID));
}

static int spiWrite(unsigned char *msg_buf, int nbytes)
{
    return(BcmSpi_Write(msg_buf, nbytes, spi_flash_busnum, SPI_FLASH_SLAVE_DEV_ID, spi_flash_clock));
}


static int spi_nand_read_cfg(PCFE_SPI_NAND_CHIP pchip)
{
    int ret = FLASH_API_OK;

    /* default settings for the most common chips,
       over write it in the switch case if new chip has a different size */

    pchip->chip_page_size = 0x800;
    pchip->chip_page_shift = 11;
    pchip->chip_pages_per_block = 64;
    pchip->chip_num_blocks = 0x400; //1024 blocks total
    pchip->chip_block_shift = 17;
    pchip->chip_num_planes = 1;
#if defined(CFG_RAMAPP)
    pchip->chip_spare_size = 64;
    pchip->chip_ecc_offset = 0x800; // location of ECC bytes
    pchip->ecclayout = &spinand_oob_micron;
    pchip->chip_ecc_corr = 0x00; // threshold to fix correctable bits (1)
#endif
    switch(pchip->chip_device_id)
    {
    case SPI_MAKE_ID(GIGADEVPART, ID_GD5F1GQ4UA): // 1Gb, 128MB
#if defined(CFG_RAMAPP)
        pchip->chip_ecc_offset = 0x840; // location of ECC bytes
        pchip->ecclayout = &spinand_oob_gigadevice;
#endif
        break;

    case SPI_MAKE_ID(GIGADEVPART, ID_GD5F1GQ4UB): // 1Gb, 128MB
#if defined(CFG_RAMAPP)
        pchip->chip_ecc_offset = 0x840; // location of ECC bytes
        pchip->ecclayout = &spinand_oob_gigadevice;
        pchip->chip_ecc_corr = 0x10; // threshold to fix correctable bits (5)
#endif
        break;

    case SPI_MAKE_ID(GIGADEVPART, ID_GD5F2GQ4UB): // 2Gb, 256MB
        pchip->chip_num_blocks = 0x800;  // 2048 blocks total
#if defined(CFG_RAMAPP)
        pchip->chip_ecc_offset = 0x840; // location of ECC bytes
        pchip->ecclayout = &spinand_oob_gigadevice;
        pchip->chip_ecc_corr = 0x10; // threshold to fix correctable bits (5)
#endif
        break;

    case SPI_MAKE_ID(GIGADEVPART, ID_GD5F4GQ4UB): // 4Gb, 512MB
        pchip->chip_page_size = 0x1000;
        pchip->chip_page_shift = 12;
        pchip->chip_num_blocks = 0x800;  // 2048 blocks total
        pchip->chip_block_shift = 18;
#if defined(CFG_RAMAPP)
        pchip->chip_spare_size = 128;
        pchip->chip_ecc_offset = 0x1080; // location of ECC bytes
        pchip->ecclayout = &spinand_oob_gigadevice;
        pchip->chip_ecc_corr = 0x10; // threshold to fix correctable bits (5)
#endif
        break;

    case SPI_MAKE_ID(MICRONPART, ID_MT29F1G01): // 1Gb, 128MB, default part
        break;

    case SPI_MAKE_ID(MICRONPART, ID_MT29F2G01): // 2Gb, 256MB
        pchip->chip_num_blocks = 0x800;  // 2048 blocks total
        pchip->chip_num_planes  = 2;
        break;

    case SPI_MAKE_ID(MICRONPART, ID_MT29F4G01): // 4Gb, 512MB
        pchip->chip_num_blocks = 0x1000;  // 4096 blocks total
        pchip->chip_num_planes  = 2;
        break;

    default: // 1Gb, 128MB
#if defined(CFG_RAMAPP)
        printf("unsupported spi nand device id 0x%lx, use default cfg setting\n", pchip->chip_device_id);
#endif
        break;
    }

    pchip->chip_block_size = pchip->chip_pages_per_block * pchip->chip_page_size;
    pchip->chip_total_size = pchip->chip_block_size * pchip->chip_num_blocks;

    return ret;
}

/*********************************************************************/
/*  SPI NAND INIT                                                    */
/*********************************************************************/
#if defined(CFG_RAMAPP)
int spi_nand_init(flash_device_info_t **flash_info)
{
    struct flash_name_from_id *fnfi_ptr;
#else
int spi_nand_init(void)
{
#endif
    PCFE_SPI_NAND_CHIP pchip = &g_spinand_chip;  
    int spiCtrlState;
    int ret = FLASH_API_OK;
    /* micron MT29F1G01 only support up to 50MHz, update to 50Mhz if it is more than that */
#if defined(_BCM96816_) || defined(CONFIG_BCM96816) || defined(_BCM96818_) || defined(CONFIG_BCM96818)
    uint32 miscStrapBus = MISC->miscStrapBus;

    if ( miscStrapBus & MISC_STRAP_BUS_LS_SPIM_ENABLED )
    {
        spi_flash_busnum = LEG_SPI_BUS_NUM;
        if ( miscStrapBus & MISC_STRAP_BUS_SPI_CLK_FAST )
        {
            spi_flash_clock = 20000000;
        }
        else
        {
            spi_flash_clock = 781000;
        }
    }
    else
    {
        spi_flash_busnum = HS_SPI_BUS_NUM;
        if ( miscStrapBus & MISC_STRAP_BUS_SPI_CLK_FAST )
        {
            spi_flash_clock = 20000000;
        }
        else
        {
            spi_flash_clock = 50000000;
        }
    }

#elif defined(_BCM96328_) || defined(CONFIG_BCM96328)
    uint32 miscStrapBus = MISC->miscStrapBus;

    spi_flash_busnum = HS_SPI_BUS_NUM;
    if ( miscStrapBus & MISC_STRAP_BUS_HS_SPIM_FAST_B_MASK )
        spi_flash_clock = 33333334;
    else
        spi_flash_clock = 16666667;

#elif defined(_BCM96362_) || defined(CONFIG_BCM96362) || defined(_BCM963268_) || defined(CONFIG_BCM963268) || defined(_BCM96828_) || defined(CONFIG_BCM96828)
    uint32 miscStrapBus = MISC->miscStrapBus;

    spi_flash_busnum = HS_SPI_BUS_NUM;
    if ( miscStrapBus & MISC_STRAP_BUS_HS_SPIM_CLK_SLOW_N_FAST )
        spi_flash_clock = 50000000;
    else
        spi_flash_clock = 20000000;

#elif defined(_BCM96368_) || defined(CONFIG_BCM96368)
    uint32 miscStrapBus = GPIO->StrapBus;
 
    if ( miscStrapBus & MISC_STRAP_BUS_SPI_CLK_FAST )
       spi_flash_clock = 20000000;
    else
       spi_flash_clock = 781000;

#elif defined(_BCM96318_) || defined(CONFIG_BCM96318)
       spi_flash_busnum = HS_SPI_BUS_NUM;
       spi_flash_clock = 50000000; 
       //spi_flash_clock = 12500000; Default clock

#elif defined(_BCM96838_) || defined(CONFIG_BCM96838) || (BRCM_BOARD_ID==96838) // Linux
       spi_flash_busnum = HS_SPI_BUS_NUM;
       spi_flash_clock = 20000000; // reset value 

#elif defined(_BCM963381_) || defined(CONFIG_BCM963381)
       spi_flash_busnum = HS_SPI_BUS_NUM;
       //FIXME choose the right clock
       spi_flash_clock = 20000000; // reset value 
#endif

#if defined(_BCM963138_) || defined(CONFIG_BCM963138)
       spi_flash_busnum = HS_SPI_BUS_NUM;
       //FIXME choose the right clock
       spi_flash_clock = 20000000; // reset value 
#endif


#if defined(_BCM963148_) || defined(CONFIG_BCM963148)
       spi_flash_busnum = HS_SPI_BUS_NUM;
       //FIXME choose the right clock
       spi_flash_clock = 20000000; // reset value 
#endif
		     
    /* retrieve the maximum read/write transaction length from the SPI controller */
    spi_max_op_len = BcmSpi_GetMaxRWSize( spi_flash_busnum, 1 );

#if defined(CFG_RAMAPP)
//    printf("BcmSpi_GetMaxRWSize=0x%x\n", spi_max_op_len);
#endif

    /* set the controller state, spi_mode_0 */
    spiCtrlState = SPI_CONTROLLER_STATE_DEFAULT;

    if ( spi_flash_clock > SPI_CONTROLLER_MAX_SYNC_CLOCK )
       spiCtrlState |= SPI_CONTROLLER_STATE_ASYNC_CLOCK;

    BcmSpi_SetCtrlState(spi_flash_busnum, SPI_FLASH_SLAVE_DEV_ID, SPI_MODE_DEFAULT, spiCtrlState);
    BcmSpi_SetFlashCtrl(0x3, 1, spi_dummy_bytes, spi_flash_busnum, SPI_FLASH_SLAVE_DEV_ID, spi_flash_clock, 0);

    spi_nand_reset();

    pchip->chip_device_id = spi_nand_get_device_id();

#if defined(CFG_RAMAPP)
    flash_spi_nand_dev.flash_device_id = pchip->chip_device_id;
    for( fnfi_ptr = fnfi; fnfi_ptr->fnfi_id != 0; fnfi_ptr++ )
    {
        if( fnfi_ptr->fnfi_id == flash_spi_nand_dev.flash_device_id )
        {
            strcpy(flash_spi_nand_dev.flash_device_name, fnfi_ptr->fnfi_name);
            break;
        }
    }

    *flash_info = &flash_spi_nand_dev;
#endif
    ret = spi_nand_read_cfg(pchip);

    if(ret != FLASH_API_OK )
        return ret;
#if 0
    spi_nand_init_cleanmarker(pchip);
    /* If the first block's spare area is not a JFFS2 cleanmarker,
     * initialize all block's spare area to a cleanmarker.
     */
    if( !spi_nand_is_page_cleanmarker(pchip, 0, 0) )
        ret = spi_nand_initialize_spare_area(pchip, 0);
#endif

    return ret;
}

/*********************************************************************/
/*  row address is 24 bit length. so buf must be at least 3 bytes. */
/*  For gigadevcie GD5F1GQ4 part(2K page size, 64 page per block and 1024 blocks) */
/*  Row Address. RA<5:0> selects a page inside a block, and RA<15:6> selects a block and */
/*  first byte is dummy byte */
/*********************************************************************/
static void spi_nand_row_addr(PCFE_SPI_NAND_CHIP pchip, unsigned int page_addr, unsigned char* buf)
{
    buf[0] = (unsigned char)(page_addr>>(pchip->chip_page_shift+16)); //dummy byte 
    buf[1] = (unsigned char)(page_addr>>(pchip->chip_page_shift+8));
    buf[2] = (unsigned char)(page_addr>>(pchip->chip_page_shift));
     
    return;
}

/*********************************************************************/
/*  column address select the offset within the page. For gigadevcie GD5F1GQ4 part(2K page size and 2112 with spare) */
/*  is 12 bit length. so buf must be at least 2 bytes. The 12 bit address is capable of address from 0 to 4095 bytes */
/*  however only byte 0 to 2111 are valid. */
/*********************************************************************/

static void spi_nand_col_addr(PCFE_SPI_NAND_CHIP pchip,  unsigned int page_addr, unsigned int page_offset, unsigned char* buf)
{
    page_offset = page_offset&((1<<(pchip->chip_page_shift+1))-1);  /* page size + spare area size */

    /* the upper 4 bits of buf[0] is either wrap bits for gigadevice or dummy bit[3:1] + plane select bit[0] for micron
     */
    if( SPI_MANU_ID(pchip->chip_device_id) == MICRONPART )
    {
        /* setup plane bit if more than one plane. otherwise that bit is always 0 */
        if( pchip->chip_num_planes > 1 )
            buf[0] = (unsigned char)(((page_offset>>8)&0xf)|((page_addr>>pchip->chip_block_shift)&0x1)<<4); //plane bit is the first bit of the block number RowAddr[6]
        else
            buf[0] = (unsigned char)((page_offset>>8)&0xFF);
    }
    else
    {
        /* use default wrap option 0, wrap length 2112 */
        buf[0] = (unsigned char)((page_offset>>8)&0xFF);
    }
    buf[1] = (unsigned char)(page_offset&0xFF);

    return;
}

/*********************************************************************/
/* some devices such as Micron MT29F1G01 require explicit reset before */
/* access to the device.                                             */
/*********************************************************************/

static int spi_nand_reset(void)
{
    unsigned char buf[4];
#if defined(CONFIG_BRCM_IKOS)
    unsigned int i;
    for( i = 0; i < 250; i++);
#else
    cfe_usleep(300);
#endif

    buf[0]        = FLASH_RESET;
    spiWrite(buf, 1);

#if defined(CONFIG_BRCM_IKOS)
    for( i = 0; i < 3000; i++);
#else
    /* device is availabe after 5ms */ 
    cfe_usleep(5000);
#endif

    while(!spi_nand_ready()); // do we need this here??

    spi_nand_set_feat(FEATURE_PROT_ADDR, FEAT_DISABLE); // disable block locking

    return(FLASH_API_OK);
}

/***************************************************************************
 * Function Name: spi_nand_read_page
 * Description  : Reads up to a NAND block of pages into the specified buffer.
 * Returns      : FLASH_API_OK or FLASH_API_ERROR or FLASH_API_CORR
 ***************************************************************************/
#if defined(CFG_RAMAPP)
int spi_nand_read_page(unsigned long page_addr, unsigned int page_offset, unsigned char *buffer, int len)
#else
static int spi_nand_read_page(unsigned long page_addr, unsigned int page_offset, unsigned char *buffer, int len)
#endif
{
    PCFE_SPI_NAND_CHIP pchip = &g_spinand_chip;
    unsigned char buf[READ_BUF_LEN_MAX];
    int status = FLASH_API_OK;
    int maxread;
    struct spi_transfer xfer;
#if defined(CFG_RAMAPP)
    if ((page_offset + len) > (pchip->chip_ecc_offset + pchip->chip_spare_size))
    {
        printf("spi_nand_read_page(): Attempt to read past page boundary, offset 0x%x, length 0x%x, into page address 0x%x\n", page_offset, len, (unsigned int)page_addr);

        return (FLASH_API_ERROR);
    }

    if (g_no_ecc)
        spi_nand_set_feat(FEATURE_FEAT_ADDR, FEAT_DISABLE); // disable ECC, used for correctable error counting
    else
#endif
        spi_nand_set_feat(FEATURE_FEAT_ADDR, FEAT_ECC_EN); // reading from page, enable ECC, turn on ECC anyway even if there's a failure should still fill buffer

    /* The PAGE READ (13h) command transfers the data from the NAND Flash array to the
     * cache register.  The PAGE READ command requires a 24-bit address consisting of
     * 8 dummy bits followed by a 16-bit block/page address.
     */
    buf[0] = FLASH_PREAD;
    spi_nand_row_addr(pchip, page_addr, buf+1);
    DBG_PRINTF("spi_nand_read_page - spi cmd 0x%x, 0x%x, 0x%x, 0x%x\n", buf[0], buf[1], buf[2], buf[3]);
    spiWrite(buf, 4);

    /* GET FEATURES (0Fh)  command to read the status */
    while(!spi_nand_ready());

    status = spi_nand_ecc();

    if (!len)
        return(status);

    while (len > 0)
    { // break up NAND buffer read into SPI buffer sized chunks
       /* Random data read (0Bh or 03h) command to read the page data from the cache
          The RANDOM DATA READ command requires 4 dummy bits, followed by a 12-bit column
          address for the starting byte address and a dummy byte for waiting data.
          This is only for 2K page size, the format will change for other page size.
       */

        maxread = (len < spi_max_op_len) ? len : spi_max_op_len;

        buf[0] = FLASH_READ;
        spi_nand_col_addr(pchip, page_addr, page_offset, buf+1);
        buf[3] = 0; //dummy byte

        DBG_PRINTF("spi_nand_read_page - spi cmd 0x%x, 0x%x, 0x%x, 0x%x\n", buf[0],buf[1],buf[2],buf[3]);
        DBG_PRINTF("spi_nand_read_page - spi read len 0x%x, offset 0x%x, remaining 0x%x\n", maxread, page_offset, len);

        memset(&xfer, 0, sizeof(struct spi_transfer));
        xfer.tx_buf      = buf;
        xfer.rx_buf      = buffer;
        xfer.len         = maxread;
        xfer.speed_hz    = spi_flash_clock;
        xfer.prepend_cnt = 4;
        xfer.addr_len    = 3; // length of address field (max 4 bytes)
        xfer.addr_offset = 1; // offset of first addr byte in header
        xfer.hdr_len     = 4; // length of header
        xfer.unit_size   = 1; // data for each transfer will be divided into multiples of unit_size
        spiRead(&xfer);
        while (!spi_nand_ready());

        buffer += maxread;
        len -= maxread;
        page_offset += maxread;
    }

    /* check ecc from status bits if we are reading from page area */
#if defined(CFG_RAMAPP)
    if(status == FLASH_API_ERROR)
    { // check to see if ECC is set to all FF's to verify as blank page (don't check page area as there may be bad bits which would not look like a blank page)
        spi_nand_set_feat(FEATURE_FEAT_ADDR, FEAT_DISABLE); // disable ECC

        buf[0] = FLASH_READ;
        spi_nand_col_addr(pchip, page_addr, pchip->chip_ecc_offset, buf+1);
        buf[3] = 0; //dummy byte

        memset(&xfer, 0, sizeof(struct spi_transfer));
        xfer.tx_buf      = buf;
        xfer.rx_buf      = buf;
        xfer.len         = 64;
        xfer.speed_hz    = spi_flash_clock;
        xfer.prepend_cnt = 4;
        xfer.addr_len    = 3;
        xfer.addr_offset = 1;
        xfer.hdr_len     = 4;
        xfer.unit_size   = 1;
        spiRead(&xfer);
        while (!spi_nand_ready());
        { // check to see if all ECC bytes are 0xFF to verify page is empty
            int i;

            for (i = 0; i < pchip->ecclayout->eccbytes; i++)
                if (buf[pchip->ecclayout->eccpos[i]] != 0xFF)
                { // need to grab secondary ECC data for Gigadevice
                    printf("Uncorrectable ECC ERROR!! Address 0x%x, block 0x%x, page 0x%x is not empty!!\n", (unsigned int)page_addr, (unsigned int)page_addr>>pchip->chip_block_shift, (unsigned int)(page_addr%pchip->chip_block_size)>>pchip->chip_page_shift);
                    return(FLASH_API_ERROR);
                }

            memset(buffer, 0xFF, len); // fill page with 0xFF just in case there were bad bits

            return(FLASH_API_OK);
        }
    }
#endif

    return(status);
}

/*********************************************************************/
/* flash_read_buf() reads buffer of data from the specified          */
/* offset from the sector parameter.                                 */
/*********************************************************************/
int spi_nand_read_buf(unsigned short blk, int offset, unsigned char *buffer, int len)
{ // can only read one block at a time, otherwise if one block is good and another is bad then whole read is invalid
    PCFE_SPI_NAND_CHIP pchip = &g_spinand_chip;
    int ret;
    unsigned int addr;
    unsigned int page_addr;
    unsigned int page_offset;
    unsigned int page_boundary;
    unsigned int size;
#if defined(CFG_RAMAPP)
    unsigned int numBlocksInChip = pchip->chip_num_blocks;
    bool flash_features;

    flash_features = (len & FLASH_FEATURES);
    len &= ~FLASH_FEATURES;
#endif
    ret = len;

    addr = (blk * pchip->chip_block_size) + offset;
    page_addr = addr & ~(pchip->chip_page_size - 1);
    page_offset = addr - page_addr;
    page_boundary = page_addr + pchip->chip_page_size;

    size = page_boundary - addr;

    if(size > len)
        size = len;

#if defined(CFG_RAMAPP)
    DBG_PRINTF(">> spi_nand_read_buf - 1 blk=0x%8.8x, offset=%d, len=%u, size=%d\n", blk, offset, len, size);

    if (blk >= numBlocksInChip)
    {
        printf("spi_nand_read_buf(): Attempt to read block number(%d) beyond the nand max blk(%d) \n", blk, numBlocksInChip-1);

        return (FLASH_API_ERROR);
    }

    if ( ((addr & (pchip->chip_block_size-1)) + len) > pchip->chip_block_size)
    { // cannot read past block boundary, otherwise if one block is good and another is bad then whole read is invalid
        printf("spi_nand_read_buf(): Attempt to read past block boundary, blk 0x%x, address 0x%x, length 0x%x\n", blk, addr, len);

        return (FLASH_API_ERROR);
    }

    if (flash_features)
        ret = spi_nand_is_cleanmarker(page_addr, 0);
#endif

    while (len)
    {
        if (spi_nand_read_page(page_addr, page_offset, buffer, size) == FLASH_API_ERROR)
            return (FLASH_API_ERROR);

        len -= size;
        if (len)
        {
            page_addr += pchip->chip_page_size;
            page_offset = 0;
            buffer += size;
            if(len > pchip->chip_page_size)
                size = pchip->chip_page_size;
            else
                size = len;
        }
    }

#if defined(CFG_RAMAPP)
    if (spi_nand_is_blk_bad(pchip, blk))
    { /* don't check for bad block during page read/write since may be reading/writing to bad block marker,
         check for bad block after read to allow for data recovery */
        printf("spi_nand_read_buf(): Attempt to read bad nand block 0x%x\n", blk);

        return (FLASH_API_ERROR);
    }
#endif

    DBG_PRINTF(">> spi_nand_read_buf - ret=%d\n", ret);

    return(ret);
}

/*********************************************************************/
/* Flash_status return the feature status byte                       */
/*********************************************************************/
static int spi_nand_status(void)
{
    return spi_nand_get_feat(FEATURE_STAT_ADDR);
}

/* check device ready bit */
static int spi_nand_ready(void)
{
  return (spi_nand_status()&STAT_OIP) ? 0 : 1;
}

/*********************************************************************/
/*  spi_nand_get_feat return the feature byte at feat_addr            */
/*********************************************************************/
static int spi_nand_get_feat(unsigned char feat_addr)
{
    unsigned char buf[4];
    struct spi_transfer xfer;

    /* check device is ready */
    memset(&xfer, 0, sizeof(struct spi_transfer));
    buf[0]           = FLASH_GFEAT;
    buf[1]           = feat_addr;
    xfer.tx_buf      = buf;
    xfer.rx_buf      = buf;
    xfer.len         = 1;
    xfer.speed_hz    = spi_flash_clock;
    xfer.prepend_cnt = 2;
    spiRead(&xfer);
   
    DBG_PRINTF("spi_nand_get_feat at 0x%x 0x%x\n", feat_addr, buf[0]);

    return buf[0];
}

/*********************************************************************/
/*  spi_nand_set_feat set the feature byte at feat_addr              */
/*********************************************************************/
static void spi_nand_set_feat(unsigned char feat_addr, unsigned char feat_val)
{
    unsigned char buf[3];

    /* check device is ready */
    buf[0]           = FLASH_SFEAT;
    buf[1]           = feat_addr;
    buf[2]           = feat_val;
    spiWrite(buf, 3);

    while(!spi_nand_ready());

    return;
}

static int spi_nand_ecc(void)
{
    int status;

    status = spi_nand_get_feat(FEATURE_STAT_ADDR);

    status = status & STAT_ECC_MASK;
    if (status == STAT_ECC_UNCORR) // uncorrectable ECC error, however page may be empty, next layer up checks for this
        return(FLASH_API_ERROR);

#if defined(CFG_RAMAPP)
    if (status == STAT_ECC_GOOD)
        return(FLASH_API_OK);

    // correctable errors from this point on
    if (status == STAT_ECC_CORR) // check for higher granularity of correctable ECC bits to see if it falls within threshold
        if (g_spinand_chip.chip_ecc_corr && ( (spi_nand_get_feat(FEATURE_STAT_AUX) & STAT_ECC_MASK) < g_spinand_chip.chip_ecc_corr ))
            return(FLASH_API_OK);

    return(FLASH_API_CORR); // correctable errors
#else

    return(FLASH_API_OK);
#endif
 }

/*********************************************************************/
/* Useful function to return the number of blks in the device.       */
/* Can be used for functions which need to loop among all the        */
/* blks, or wish to know the number of the last blk.                 */
/*********************************************************************/
int spi_nand_get_numsectors(void)
{
    return g_spinand_chip.chip_num_blocks;  
}

/*********************************************************************/
/* flash_get_sector_size() is provided for cases in which the size   */
/* of a sector is required by a host application.  The sector size   */
/* (in bytes) is returned in the data location pointed to by the     */
/* 'size' parameter.                                                 */
/*********************************************************************/
int spi_nand_get_sector_size(unsigned short blk)
{
    return g_spinand_chip.chip_block_size;
}

#if defined(CFG_RAMAPP)
/************************************************************************/
/* The purpose of flash_get_total_size() is to return the total size of */
/* the flash                                                            */
/************************************************************************/
static int spi_nand_get_total_size(void)
{
   return(g_spinand_chip.chip_total_size);
}

/************************************************************************/
/* spi_nand_dev_specific_cmd Triggers a device specific feature,        */
/* used to access non-standard features.                                */
/************************************************************************/
static int spi_nand_dev_specific_cmd(unsigned int command, void * inBuf, void * outBuf)
{
    PCFE_SPI_NAND_CHIP pchip = &g_spinand_chip;

    switch(command)
    {
    case WRITE_WITHOUT_ECC:
    {
        flash_write_data_t *fwd = inBuf;

        g_no_ecc = 1;
        spi_nand_write_page((fwd->block * pchip->chip_block_size) + (fwd->page * pchip->chip_page_size), fwd->offset, fwd->data, fwd->amount);
        g_no_ecc = 0;
        break;
    }

    case NAND_REINIT_FLASH_BAD:
        g_force_erase = 1;

    case NAND_REINIT_FLASH:
    {
        unsigned int block;

        for (block = 0; block < pchip->chip_num_blocks; block++)
        {
            printf(".");
            flash_sector_erase_int(block);
        }
        printf("\n");

        g_force_erase = 0;
        break;
    }

    case CHECK_BAD_BLOCK:
        return(spi_nand_is_blk_bad(pchip, *(unsigned int *)inBuf));

    case MARK_BLOCK_BAD:
    {
        unsigned long page_addr = *(unsigned int *)inBuf * pchip->chip_block_size;

        spi_nand_mark_bad_blk(pchip, page_addr);
        break;
    }

    case FORCE_ERASE:
        g_force_erase = 1;
        flash_sector_erase_int(*(unsigned int *)inBuf);
        g_force_erase = 0;
        break;

    case GET_PAGE_SIZE:
        return(pchip->chip_page_size);

    case GET_SPARE_SIZE:
        return(pchip->chip_spare_size);

    case GET_ECC_OFFSET:
        return(pchip->chip_ecc_offset);

    }

    return 0;
}
#endif

/*********************************************************************/
/* flash_get_device_id() return the device id of the component.      */
/*********************************************************************/

static unsigned short spi_nand_get_device_id(void)
{
    unsigned char buf[4];
    unsigned char *pBuf = buf;
    struct spi_transfer xfer;

    memset(&xfer, 0, sizeof(struct spi_transfer));
    buf[0]           = FLASH_RDID;
    buf[1]           = 0;
    xfer.tx_buf      = buf;
    xfer.rx_buf      = buf;
    xfer.len         = 2;
    xfer.speed_hz    = spi_flash_clock;
    xfer.prepend_cnt = 2;
    spiRead(&xfer);
    while(!spi_nand_ready());

    DBG_PRINTF("spi_nand_get_device_id 0x%x 0x%x\n", buf[0], buf[1]);
    /* return manufacturer code buf[0] in msb and device code buf[1] in lsb*/
#if defined(__MIPSEL) || defined(__ARMEL__)
     buf[2] = buf[1];
     buf[1] = buf[0];
     buf[0] = buf[2];
#endif

    return( *((unsigned short *)pBuf) );
}
#if defined(CFG_RAMAPP)
/*********************************************************************/
/* Flash_sector__int() wait until the erase is completed before */
/* returning control to the calling function.  This can be used in   */
/* cases which require the program to hold until a sector is erased, */
/* without adding the wait check external to this function.          */
/*********************************************************************/
extern int dump_nand(int addr, int end);

static int spi_nand_sector_erase_blk(unsigned short blk)
{
    PCFE_SPI_NAND_CHIP pchip = &g_spinand_chip;
    unsigned char buf[11];
    unsigned int page_addr;
    int ret = FLASH_API_OK, status;

    DBG_PRINTF("spi_nand_sector_erase_blk block 0x%x\n", blk);

    if (blk >= pchip->chip_num_blocks)
    {
        printf("Attempt to erase failed due to block %d beyond the nand max blk(%d)\n", blk, pchip->chip_num_blocks-1);
        return FLASH_API_ERROR;
    }

    page_addr = (blk * pchip->chip_block_size);

    if (!g_force_erase)
    {
        if (spi_nand_is_blk_bad(pchip, blk))
        {
            printf("Attempt to erase failed due to bad block 0x%x, address 0x%x\n", blk, page_addr);
            return FLASH_API_ERROR;
        }
    }

    /* set device to write enabled */
    spi_nand_write_enable();
    buf[0] = FLASH_BERASE;
    spi_nand_row_addr(pchip, page_addr, buf+1);
    spiWrite(buf, 4);
    while(!spi_nand_ready()) ;

    status = spi_nand_status();
    if( status & STAT_EFAIL )
    {
        printf("Erase block 0x%x failed, sts 0x%x\n", blk, status);
        return(FLASH_API_ERROR);
    }

    spi_nand_write_disable();

    return ret;
}

/**********************************************************************/
/* flash_write_enable() must be called before any change to the       */
/* device such as write, erase. It also unlock the blocks if they are */
/* previouly locked.                                                  */
/**********************************************************************/
static int spi_nand_write_enable(void)
{
    unsigned char buf[4], prot;

    /* make sure it is not locked first */
    prot = spi_nand_get_feat(FEATURE_PROT_ADDR);
    if( prot != 0 )
    {
        prot = 0;
        spi_nand_set_feat(FEATURE_PROT_ADDR, prot);
    }

    /* send write enable cmd and check feature status WEL latch bit */
    buf[0] = FLASH_WREN;
    spiWrite(buf, 1);
    while(!spi_nand_ready());
    while(!spi_nand_wel()); 

    return(FLASH_API_OK);
}

static int spi_nand_write_disable(void)
{
    unsigned char buf[4];

    buf[0] = FLASH_WRDI;
    spiWrite(buf, 1);
    while(!spi_nand_ready());
    while(spi_nand_wel()); 

    return(FLASH_API_OK);
}

/***************************************************************************
 * Function Name: spi_nand_write_page
 * Description  : Writes a NAND page from the specified buffer.
 * Returns      : FLASH_API_OK or FLASH_API_ERROR
 ***************************************************************************/
static int spi_nand_write_page(unsigned long page_addr, unsigned int page_offset, unsigned char *buffer, int len)
{
    PCFE_SPI_NAND_CHIP pchip = &g_spinand_chip;
    unsigned char spi_buf[512];  /* HS_SPI_BUFFER_LEN SPI controller fifo size is current at 512 bytes*/
    unsigned char xfer_buf[pchip->chip_ecc_offset + pchip->chip_spare_size];
    int maxwrite, status;
    int verify;

    if (!len)
    {
        printf("spi_nand_write_page(): Not writing any data to page addr 0x%x, page_offset 0x%x, len 0x%x\n", (unsigned int)page_addr, page_offset, len);
        return (FLASH_API_OK);
    }

    if ((page_offset + len) > (pchip->chip_ecc_offset + pchip->chip_spare_size))
    {
        printf("spi_nand_write_page(): Attempt to write past page boundary, offset 0x%x, length 0x%x, into page address 0x%x\n", page_offset, len, (unsigned int)page_addr);
        return (FLASH_API_ERROR);
    }

    { // check if write is blank
        unsigned int i;

        DBG_PRINTF("spi_nand_write_page - page addr 0x%x, offset 0x%x, len 0x%x\n", (unsigned int)page_addr, page_offset, len);

        status = FLASH_API_OK_BLANK;
        for( i = 0; i < len; i++ )
        { // do this on a byte basis because buffer may not be aligned/whole page
            if( *(buffer+i) != 0xff )
            {
                status = FLASH_API_OK;
                break;
            }
        }
    }

    if (status == FLASH_API_OK_BLANK)
        return(FLASH_API_OK); // don't write to page if data is all FF's

    if (!g_no_ecc)
    { // turn on ECC
        spi_nand_set_feat(FEATURE_FEAT_ADDR, FEAT_ECC_EN); // enable ECC if writing to page
        verify = 1;

        if ( (page_addr & (pchip->chip_block_size - 1)) == 0 ) // can we get rid of this? JFFS2 should not care about clean marker in block with magic number, but we still need clean marker for block 0 and backward compatibility
            verify = 2; /* write JFFS2 clean marker into spare area buffer if writing into data area of first page of block */
    }
    else
    { // turn off ECC
        spi_nand_set_feat(FEATURE_FEAT_ADDR, FEAT_DISABLE); // else don't write ECC
        verify = 0;
    }

    memset(xfer_buf, 0xff, sizeof(xfer_buf));
    memcpy(xfer_buf + page_offset, buffer, len);
    if (verify == 2)
        spi_nand_place_jffs2_clean_marker(xfer_buf + pchip->chip_page_size);

    if ((page_offset + len) > (pchip->chip_page_size + pchip->chip_spare_size))
        len = pchip->chip_ecc_offset + pchip->chip_spare_size; // to access Gigadevice hidden spare area
    else
        len = pchip->chip_page_size + pchip->chip_spare_size;

    page_offset = 0;

    while (len > 0)
    {
        /* Send Program Load Random Data Command (0x84) to load data to cache register.
         * PROGRAM LOAD consists of an 8-bit Op code, followed by 4 bit dummy  and a
         * 12-bit column address, then the data bytes to be programmed. */
        spi_buf[0] = FLASH_PROG_RAN;
        spi_nand_col_addr(pchip, page_addr, page_offset, spi_buf + 1);

        maxwrite = (len < (spi_max_op_len - 3)) ? len : (spi_max_op_len - 3);

        memcpy(&spi_buf[3], xfer_buf + page_offset, maxwrite);
        DBG_PRINTF("spi_nand_write_page - spi cmd 0x%x, 0x%x, 0x%x\n", spi_buf[0], spi_buf[1], spi_buf[2]);
        DBG_PRINTF("spi_nand_write_page - spi write len 0x%x, offset 0x%x, remaining 0x%x\n", maxwrite, offset, len-maxwrite);

        spiWrite(spi_buf, maxwrite + 3);

        len -= maxwrite;
        page_offset += maxwrite;

        while(!spi_nand_ready()); // do we need this here??
    }

    /* Send Program Execute command (0x10) to write cache data to memory array
     * Send address (24bit): 8 bit dummy + 16 bit address (page/Block)
     */
    /* Send Write enable Command (0x06) */
    spi_nand_write_enable(); // should we move this further up?

    spi_buf[0] = FLASH_PEXEC;
    spi_nand_row_addr(pchip, page_addr, spi_buf + 1);
    DBG_PRINTF("spi_nand_write_page - spi cmd 0x%x, 0x%x, 0x%x, 0x%x\n", spi_buf[0], spi_buf[1], spi_buf[2], spi_buf[3]);
    spiWrite(spi_buf, 4);
    while(!spi_nand_ready());

    status = spi_nand_status();
    spi_nand_write_disable();

    if( status & STAT_PFAIL )
    {
        printf("Page program failed at 0x%x page address, sts 0x%x\n", (unsigned int)page_addr, status);
        return(FLASH_API_ERROR);
    }

    if (verify)
    {
        unsigned char buf[pchip->chip_page_size];

        status = spi_nand_read_page(page_addr, 0, buf, pchip->chip_page_size);

        if (status == FLASH_API_ERROR)
        {
            printf("Write verify failed reading back page at address 0x%lx\n", page_addr);
            return(FLASH_API_ERROR);
        }

        if (memcmp(xfer_buf, buf, pchip->chip_page_size))
        {
            printf("Write data did not match read data at address 0x%lx\n", page_addr);
            return(FLASH_API_ERROR);
        }

        if (status == FLASH_API_CORR)
        {
            printf("Write verify correctable errors at address 0x%lx\n", page_addr);
            return(FLASH_API_CORR);
        }
    }

    return (FLASH_API_OK);
}

/***************************************************************************
 * Function Name: nand_flash_write_buf
 * Description  : Writes to flash memory. Erase block must be called first. 
 * Returns      : number of bytes written or FLASH_API_ERROR
 ***************************************************************************/
static int spi_nand_write_buf(unsigned short blk, int offset, unsigned char *buf, int len)
{ // can only write one block at a time, otherwise if one block is good and another is bad then whole write is invalid

    PCFE_SPI_NAND_CHIP pchip = &g_spinand_chip;
    int ret, status, error;
    unsigned long addr;
    unsigned long page_addr;
    unsigned int page_offset;
    unsigned long page_boundary;
    unsigned long block_addr;
    unsigned int size, ofs;
    unsigned int numBlocksInChip = pchip->chip_num_blocks;

    ret = len;

    DBG_PRINTF(">> spi_nand_write_buf - 1 blk=0x%8.8lx, offset=%d, len=%d\n", blk, offset, len);

    block_addr = blk * pchip->chip_block_size;
    addr = block_addr + offset;
    page_addr = addr & ~(pchip->chip_page_size - 1);
    page_offset = addr - page_addr;
    page_boundary = page_addr + pchip->chip_page_size;

    size = page_boundary - addr;

    if(size > len)
        size = len;

    if (spi_nand_is_blk_bad(pchip, blk))
    {
        printf("spi_nand_write_buf(): Attempt to write bad nand block 0x%x\n", blk);

        return (FLASH_API_ERROR);
    }

    if (blk >= numBlocksInChip)
    {
        printf("spi_nand_write_buf(): Attempt to write block number(%d) beyond the nand max blk(%d) \n", blk, numBlocksInChip-1);
        return (FLASH_API_ERROR);
    }

    if ( ((addr & (pchip->chip_block_size-1)) + len) > pchip->chip_block_size)
    { // cannot write past block boundary, otherwise if one block is good and another is bad then whole write is invalid
        printf("spi_nand_write_buf(): Attempt to write past block boundary, blk 0x%x, address 0x%x, length 0x%x\n", blk, addr, len);

        return (FLASH_API_ERROR);
    }

    if (len)
    {
        ofs = 0;
        error = 0;

        do
        {
            if( (status = spi_nand_write_page(page_addr, page_offset, buf + ofs, size)) == FLASH_API_ERROR )
                error = 1;

            if (!error && (status == FLASH_API_CORR) && (page_addr >= pchip->chip_block_size))
            { // read/erase/write block to see if we can get rid of the bit errors, but only if not block zero
                int offset;
                unsigned char * buffer;

                buffer = KMALLOC(pchip->chip_block_size, 0);
                if (!buffer)
                { // unfortunately can't attempt to fix block in this case
                    printf("Error allocating buffer!!\n");
                    error = 1;
                }

                // read block
                for (offset = 0; !error && (offset < pchip->chip_block_size); offset += pchip->chip_page_size)
                {
                    status = spi_nand_read_page(block_addr + offset, 0, buffer + offset, pchip->chip_page_size);
                    if (status == FLASH_API_ERROR)
                        error = 1;
                }

                // erase block
                if (!error)
                {
                    status = spi_nand_sector_erase_blk(blk);
                    if (status == FLASH_API_ERROR)
                        error = 1;
                }

                // write block
                if (!error)
                {
                    for (offset = 0; offset < pchip->chip_block_size; offset += pchip->chip_page_size)
                    {
                        status = spi_nand_write_page(block_addr + offset, 0, buffer + offset, pchip->chip_page_size);
                        if (status != FLASH_API_OK)
                            error = 1; // essentially failed, but finish writing out all the data anyway to hopefully be recovered later
                    }
                }

                if (buffer)
                    KFREE(buffer);
            }

            if (error)
            { // mark block bad
                printf("SPI NAND ERROR Writing page!!\n");
                spi_nand_mark_bad_blk(pchip, block_addr);

                return(FLASH_API_ERROR);
            }

            len -= size;
            if( len )
            {
                DBG_PRINTF(">> nand_flash_write_buf- 2 blk=0x%8.8lx, len=%d\n", blk, len);

                page_addr += pchip->chip_page_size;
                page_offset = 0;
                ofs += size;
                if(len > pchip->chip_page_size)
                    size = pchip->chip_page_size;
                else
                    size = len;
            }
        } while(len);
    }

    DBG_PRINTF(">> nand_flash_write_buf - ret=%d\n", ret);

    return( ret ) ;
}

/* check device write enable latch bit */
static int spi_nand_wel(void)
{
  return (spi_nand_status()&STAT_WEL) ? 1 : 0;
}

static int spi_nand_is_blk_bad(PCFE_SPI_NAND_CHIP pchip, unsigned short blk)
{
    unsigned char buf1, buf2;
    int temp = g_no_ecc;

    if (0 == blk)
        return 0; // always return good for block 0, because if it's bad chip quite possibly system is useless

    g_no_ecc = 1; // disable ECC since bad block marker is not covered by ECC, in addition this suppresses ECC error reporting if there are issues with the first two pages of a block, however this is ok since still reports bad block
    spi_nand_read_page(pchip->chip_block_size * blk, pchip->chip_page_size, &buf1, 1);
    spi_nand_read_page((pchip->chip_block_size * blk) + pchip->chip_page_size, pchip->chip_page_size, &buf2, 1);
    g_no_ecc = temp;

    return((buf1 != 0xFF) || (buf2 != 0xFF));
}

static void spi_nand_mark_bad_blk(PCFE_SPI_NAND_CHIP pchip, unsigned long page_addr)
{
    printf("Marking block 0x%lx bad (address 0x%lx)\n", page_addr >> pchip->chip_block_shift, page_addr);

    g_no_ecc = 1;
    spi_nand_write_page(page_addr, pchip->chip_page_size, (unsigned char *) "\0", 1); // mark block bad first page
    spi_nand_write_page(page_addr + pchip->chip_page_size, pchip->chip_page_size, (unsigned char *) "\0", 1); // mark block bad second page
    g_no_ecc = 0;
}

/***************************************************************************
 * Function Name: spi_nand_get_memptr
 * Description  : Returns the base MIPS memory address for the specfied flash
 *                sector.
 * Returns      : Base MIPS memory address for the specfied flash sector.
 ***************************************************************************/

static unsigned char *spi_nand_get_memptr(unsigned short blk)
{
    return((unsigned char *) (FLASH_BASE + (blk * g_spinand_chip.chip_block_size)));
}

/*********************************************************************
 * The purpose of flash_get_blk() is to return the block number
 * for a given memory address.
 *********************************************************************/

static int spi_nand_get_blk(int addr)
{
    return((int) ((unsigned long) addr - FLASH_BASE) >> g_spinand_chip.chip_block_shift);
}


static void spi_nand_place_jffs2_clean_marker(unsigned char * buf)
{ // we need to worry about the position of useable OOB data
    PCFE_SPI_NAND_CHIP pchip = &g_spinand_chip;
    unsigned char * clean_marker = (unsigned char *)jffs2_clean_marker;

    int srcI = 0, destI = 0, eccI = 0; // indexes

    while (destI < pchip->chip_spare_size)
    { // crawl through the OOB data, avoiding the bad block/ECC bytes
        if ((eccI < pchip->ecclayout->eccbytes) && (destI == pchip->ecclayout->eccpos[eccI]))
        { // bypass ECC and bad block data
            buf[destI++] = 0xFF;
            eccI++;
        }
        else if (srcI < sizeof(jffs2_clean_marker))
        { // while there is still source data, copy that data
            buf[destI++] = clean_marker[srcI++];
        }
        else
        { // terminate with 0xFF
            buf[destI++] = 0xFF;
        }
    }
}


int spi_nand_is_cleanmarker(unsigned long addr, int write_if_not)
{
    PCFE_SPI_NAND_CHIP pchip = &g_spinand_chip;
    int blk = addr >> pchip->chip_block_shift;
    int page_addr = addr & ~(pchip->chip_page_size - 1);
    unsigned char buf[pchip->chip_spare_size];
    int status = 1;
    unsigned char * clean_marker = (unsigned char *)jffs2_clean_marker;

    if (spi_nand_read_page(page_addr, pchip->chip_page_size, buf, pchip->chip_spare_size) == FLASH_API_ERROR)
    {
        return(FLASH_API_ERROR);
    }

    { // we need to worry about the position of useable OOB data, this code assumes zero offset if writing OOB mode AUTO
        int srcI = 0, checkI = 0, eccI = 0; // indexes

        while (checkI < sizeof(jffs2_clean_marker))
        { // crawl through the OOB data, avoiding the bad block/ECC bytes
            if ((eccI < pchip->ecclayout->eccbytes) && (srcI == pchip->ecclayout->eccpos[eccI]))
            { // bypass ECC and bad block bytes
                srcI++;
                eccI++;
            }
            else
            { // if still within OOB area, verify data
                if (clean_marker[checkI++] != buf[srcI++])
                {
                    status = 0;
                    break;
                }
            }
        }
    }

    if (!status)
    { // no clean marker
        if (write_if_not)
        { // erase block/write clean marker
            if (spi_nand_sector_erase_blk(blk) != FLASH_API_OK)
            {
                return(FLASH_API_ERROR);
            }

            return(1);  // we wrote clean marker, so return true
        }
    }

    return(status);
}

int read_spi_spare_data(int blk, int offset, unsigned char *buf, int bufsize);
int read_spi_spare_data(int blk, int offset, unsigned char *buf, int bufsize)
{
    PCFE_SPI_NAND_CHIP pchip = &g_spinand_chip;
    unsigned long addr = (blk * pchip->chip_block_size) + offset;
    unsigned long page_addr = addr & pchip->chip_page_size;

    return(spi_nand_read_page(page_addr, pchip->chip_page_size, buf, bufsize));
}


void dump_spi_spare(void);
void dump_spi_spare(void)
{
    PCFE_SPI_NAND_CHIP pchip = &g_spinand_chip;
    unsigned char spare[SPARE_MAX_SIZE];
    unsigned long i;

    for( i = 0; i < pchip->chip_total_size; i += pchip->chip_block_size )
    {
        {
            printf("%8.8lx: %8.8lx %8.8lx %8.8lx %8.8lx\n", i,
                *(unsigned long *) &spare[0], *(unsigned long *) &spare[4],
                *(unsigned long *) &spare[8], *(unsigned long *) &spare[12]);
            if( pchip->chip_spare_size > 16 )
            {
                printf("%8.8lx: %8.8lx %8.8lx %8.8lx %8.8lx\n", i,
                    *(unsigned long *)&spare[16],*(unsigned long *)&spare[20],
                    *(unsigned long *)&spare[24],*(unsigned long *)&spare[28]);
                printf("%8.8lx: %8.8lx %8.8lx %8.8lx %8.8lx\n", i,
                    *(unsigned long *)&spare[32],*(unsigned long *)&spare[36],
                    *(unsigned long *)&spare[40],*(unsigned long *)&spare[44]);
                printf("%8.8lx: %8.8lx %8.8lx %8.8lx %8.8lx\n", i,
                    *(unsigned long *)&spare[48],*(unsigned long *)&spare[52],
                    *(unsigned long *)&spare[56],*(unsigned long *)&spare[60]);
            }
        }
    }
}

#else // !defined(CFG_RAMAPP)

/***************************************************************************
 * Function Name: rom_spi_nand_init
 * Description  : Initialize flash part just enough to read blocks.
 * Returns      : FLASH_API_OK or FLASH_API_ERROR
 ***************************************************************************/
void rom_spi_nand_init(void);
void rom_spi_nand_init(void)
{
    spi_nand_init();
}
#endif // defined(CFG_RAMAPP)
