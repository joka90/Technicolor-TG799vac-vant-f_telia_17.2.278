/*
 * Provide a MTD wrapper towards Broadcom's opensource SPI flash driver
 *
 * */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include <asm/byteorder.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>

#define MTDDEVNAME "technicolor-spi"

/** Broadcom functions in spiflash.c */
typedef void flash_device_info_t;
int spi_flash_init(flash_device_info_t **flash_info);
int spi_flash_get_sector_size(unsigned short sector);
int spi_flash_get_total_size(void);
int spi_flash_read_buf(unsigned short sector, int offset,
    unsigned char *buffer, int nbytes);
int spi_flash_write_buf(unsigned short sector, int offset,
    const unsigned char *buffer, int nbytes);
int spi_flash_sector_erase_int(unsigned short sector);


/** Interfaces towards MTD */

static int spimap_read (struct mtd_info *, loff_t, size_t, size_t *, u_char *);
static int spimap_write (struct mtd_info *, loff_t, size_t, size_t *, const u_char *);
static int spimap_erase (struct mtd_info *, struct erase_info *instr); 
static void spimap_nop (struct mtd_info *);
static struct mtd_info *spi_map_probe(struct map_info *map);

static struct mtd_chip_driver spimap_chipdrv = {
	.probe	= spi_map_probe,
	.name	= MTDDEVNAME,
	.module	= THIS_MODULE
};

static struct mtd_info *spi_map_probe(struct map_info *map)
{
	struct mtd_info *mtd;

	mtd = kzalloc(sizeof(*mtd), GFP_KERNEL);
	if (!mtd)
		return NULL;

	map->fldrv = &spimap_chipdrv;
	mtd->priv = map;
	mtd->name = MTDDEVNAME;
	mtd->type = MTD_NORFLASH;
	mtd->size = spi_flash_get_total_size();
	mtd->_read = spimap_read;
	mtd->_write = spimap_write;
	mtd->_sync = spimap_nop;
	mtd->_erase = spimap_erase;
	mtd->flags = MTD_CAP_NORFLASH;
	mtd->erasesize = spi_flash_get_sector_size(0);
	mtd->writesize = 1;

	__module_get(THIS_MODULE);
	return mtd;
}


static int spimap_read (struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf)
{
	struct map_info *map = mtd->priv;
	
	spi_flash_read_buf(0, (unsigned int)map->phys + (unsigned int)from, buf, len);

	*retlen = len;
	return 0;
}

static void spimap_nop(struct mtd_info *mtd)
{
	/* Nothing to see here */
}

static int spimap_write (struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char *buf)
{
	struct map_info *map = mtd->priv;
	
	*retlen = spi_flash_write_buf(0, (unsigned int) map->phys + (unsigned int)to, buf, len);

	return 0;	
}

static int spimap_erase (struct mtd_info *mtd, struct erase_info *instr)
{
	unsigned long i;

	for (i=0; i < instr->len; i += mtd->erasesize)
	{
        unsigned long sectoraddr = (unsigned long)(instr->addr) + (unsigned long)i;
	if ( sectoraddr & (mtd->erasesize-1)) {
            BUG_ON("Erase not sector aligned.");
	}
	else if (sectoraddr < (mtd->size))
            spi_flash_sector_erase_int(sectoraddr / mtd->erasesize);
        else
            BUG_ON("Trying to erase non-existing sector!");
	}

	instr->state = MTD_ERASE_DONE;

	mtd_erase_callback(instr);

	return 0;
}
static int __init spi_map_init(void)
{
	flash_device_info_t *fdi;

	spi_flash_init(&fdi);
	register_mtd_chip_driver(&spimap_chipdrv);
	return 0;
}

static void __exit spi_map_exit(void)
{
	unregister_mtd_chip_driver(&spimap_chipdrv);
}

module_init(spi_map_init);
module_exit(spi_map_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Broadcom SPI Flash MTD map");
