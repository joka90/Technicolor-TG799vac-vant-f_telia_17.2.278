/*
 * Broadcom mapping driver for thomson speedtouch boards
 * based on drivers of phillipe deswert / ronald vanschoren
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>

#include <asm/io.h>

#include "../drivers/mtd/mtdcore.h"

#ifdef CONFIG_MTD_SPEEDTOUCH_MAP_RAW_FLASH
/* In case the raw flash must be mapped we're in Anvil, which is able to rewrite
 * any part of the flash.
 * So we must be able to reload the partition info at runtime.
 * So the functions used for that cannot be unloaded after the init phase as
 * we will be needing them later on.
 */
#undef __init
#undef __initdata
#define __init
#define __initdata
#endif

static int __init load_partitions(void *);
static int __init guess_partitions(void);


static map_word physmap_read16(struct map_info *map, unsigned long ofs)
{
        map_word val;

        val.x[0] = __raw_readw((void __iomem *)map->phys + ofs);

        return val;
}

static void physmap_write16(struct map_info *map, map_word data, unsigned long ofs)
{
	__raw_writew(data.x[0], (void __iomem *)(map->phys + ofs));
}

/* --dimm
 *  
 * FIXME:
 *
 * there seems to be a problem with lwl/lwr instructions being
 * used for Flash accesses (speculative -- the real cause is not
 * yet clear [1], maybe it's not really due to lwl/lwr per-se).
 *
 * jffs2 -> mtd -> memcpy_fromio() -> memcpy(dst, src, len) ->
 *
 * [ e.g. when 'dst' (a pointer to a buffer in RAM) is not aligned (*) ]
 *
 * -> dst_unaligned -> lwl/lwr for 'src'
 *
 * ('src' is a Flash-mapped address in our case).
 *
 * The problem is that such kind of reads (*) don't seem to work reliably
 * (the issue has been seen for file-name reads from Flash in jffs2).
 * 
 * e.g. "passwd" may look like "sssswd", "group" like "ououp" and "12345678"
 * like "34345678" or even "34347878".
 *
 * One may see that there is actually a pattern here.
 *
 * The use of a simple byte-by-byte copying seems to solve (hide?) the issue.
 * btw., Broadcom does use this approach in their Flash driver. Do they know of
 * some kind of limitation of is it just a coincidence?
 *
 * The workaround: if (*) then resort to byte-by-byte copying.
 * 
 * [1] maybe hw related (limitation), hw configuration, ...
 */
static void *local_memcpy(void *dest, const void *src, size_t count)
{
	char *tmp = dest;

	while (count--) {
		*tmp++ = __raw_readb(src);
		src++;
	}
	return dest;
}

#define FLASH_LWLLWR_WORKAROUND

static void physmap_copy_from(struct map_info *map, void *to, unsigned long from, ssize_t len)
{
#ifdef FLASH_LWLLWR_WORKAROUND
	unsigned long src = map->phys + from;

	if (((unsigned long)to & (sizeof(unsigned long) - 1))
	    || (src & (sizeof(src) - 1)))
		local_memcpy(to, (void __iomem *)src, len);
	else
#endif
		memcpy_fromio(to, (void __iomem *)map->phys + from, len);
}

/* 
 * MAP DRIVER STUFF
 */


struct map_info speedtouch_map={
	.name = "flash",
	.size = CONFIG_SPEEDTOUCH_PHYS_SIZE,
	.phys = CONFIG_SPEEDTOUCH_FLASH_START,
	.bankwidth = 2,
	.read = physmap_read16,
	.write = physmap_write16,
	.copy_from = physmap_copy_from,
};

/*
 * MTD 'PARTITIONING' STUFF 
*/


#if defined(CONFIG_MTD_SPEEDTOUCH_DYNAMIC)

/* Partitions are dynamically detected */
static const char *part_probes[] __initdata = {"BTHub", NULL};

#else

#define BTAB_ID_NULL    0xffffffff

/* the offset of the current bank table and the ID of the current bank are
 * passed in from the bootloader on the kernel cmd line.
 */
static unsigned btab_start;
static unsigned btab_bank = BTAB_ID_NULL;

EXPORT_SYMBOL(btab_bank);

static int __init btab_start_init(char *str)
{
    get_option(&str, &btab_start);
    return 1;
}

static int __init btab_bank_init(char *str)
{
    get_option(&str, &btab_bank);
    return 1;
}

__setup("btab=", btab_start_init);
__setup("btab_bootid=", btab_bank_init);

#endif

#ifdef CONFIG_MTD_SPEEDTOUCH_BTAB

/* Partitions inside our bank, e.g. rootfs */
#define SPEEDTOUCH_NR_PARTS	1
static struct mtd_partition speedtouch_partitions[SPEEDTOUCH_NR_PARTS] __initdata = {
	{
		.name = CONFIG_SPEEDTOUCH_PART2_NAME,
		.size = CONFIG_SPEEDTOUCH_PART2_SIZE,
		.offset = CONFIG_SPEEDTOUCH_PART2_OFFSET
	}
};

#else /* all partitions are configured via kernel config */
#define BUILTIN_PARTS    1

#ifdef CONFIG_MTD_SPEEDTOUCH_EEPROM
#ifdef CONFIG_MTD_SPEEDTOUCH_DUAL_BANK_MANUAL
#define SPEEDTOUCH_NR_PARTS     6
#else
#define SPEEDTOUCH_NR_PARTS	4
#endif
#else
#define SPEEDTOUCH_NR_PARTS	3
#endif
static struct mtd_partition speedtouch_partitions[SPEEDTOUCH_NR_PARTS] = {
	{	
		.name = CONFIG_SPEEDTOUCH_PART1_NAME,
		.size = CONFIG_SPEEDTOUCH_PART1_SIZE,
		.offset = CONFIG_SPEEDTOUCH_PART1_OFFSET
	},
	{
		.name = CONFIG_SPEEDTOUCH_PART2_NAME,
		.size = CONFIG_SPEEDTOUCH_PART2_SIZE,
		.offset = CONFIG_SPEEDTOUCH_PART2_OFFSET
	},
	{
		.name = CONFIG_SPEEDTOUCH_PART3_NAME,
		.size =  CONFIG_SPEEDTOUCH_PART3_SIZE,
		.offset = CONFIG_SPEEDTOUCH_PART3_OFFSET
	}
#ifdef CONFIG_MTD_SPEEDTOUCH_EEPROM
	,{
		.name = CONFIG_SPEEDTOUCH_PART4_NAME,
		.size =  CONFIG_SPEEDTOUCH_PART4_SIZE,
		.offset = CONFIG_SPEEDTOUCH_PART4_OFFSET
	}
#endif
#ifdef CONFIG_MTD_SPEEDTOUCH_DUAL_BANK_MANUAL
        ,{
                .name = CONFIG_SPEEDTOUCH_PART5_NAME,
                .size =  CONFIG_SPEEDTOUCH_PART5_SIZE,
                .offset = CONFIG_SPEEDTOUCH_PART5_OFFSET
        },
        {
                .name = CONFIG_SPEEDTOUCH_PART6_NAME,
                .size =  CONFIG_SPEEDTOUCH_PART6_SIZE,
                .offset = CONFIG_SPEEDTOUCH_PART6_OFFSET
        }
#endif
};
#endif /* CONFIG_MTD_SPEEDTOUCH_DYNAMIC */

#ifdef CONFIG_SPEEDTOUCH_BLVERSION
static struct mtd_partition blversion_partition __initdata = {
	.name = "blversion",
	.size = 3,
	.offset = CONFIG_SPEEDTOUCH_BLVERSION_OFFSET,
	.mask_flags = MTD_WRITEABLE
};
#endif

/*
 * This is the master MTD device for which all the others are just
 * auto-relocating aliases.
 */
static struct mtd_info *mymtd;
int bcm_flash_read(loff_t from, size_t len, size_t *retlen, unsigned char *buf)
{
	int ret;

	if (!mymtd) {
		printk(KERN_ERR "bcm_flash_read: MTD layer is not yet up!\n");
		return -1;
	}

	ret = mtd_read(mymtd, from, len, retlen, buf);

#if defined(CONFIG_MTD_NAND_TL)
	if (ret == -EUCLEAN || ret == -EBADMSG)
		/* the nand driver can return this when ECC errors occur */
		ret = 0;

#endif
	return ret;
}

EXPORT_SYMBOL(bcm_flash_read);

int bcm_flash_write(loff_t to, size_t len, size_t *retlen, const unsigned char *buf)
{
	int ret;

	if (!mymtd) {
		printk(KERN_ERR "bcm_flash_write: MTD layer is not yet up!\n");
		return -1;
	}

	ret = mtd_write(mymtd, to, len, retlen, buf);

#if defined(CONFIG_MTD_NAND_TL)
	if (ret == -EUCLEAN || ret == -EBADMSG)
		/* the nand driver can return this when ECC errors occur */
		ret = 0;

#endif
	return ret;
}

EXPORT_SYMBOL(bcm_flash_write);

#ifdef CONFIG_MTD_SPEEDTOUCH_MAP_RAW_FLASH

static uint64_t erip_base;

static struct mtd_partition *__init part_by_name(struct mtd_partition *map, int num_parts, const char *name)
{
    int i;

    for (i = 0; i < num_parts; i++) {
        if (strcmp(map[i].name, name) == 0) {
            return map + i;
        }
    }
    return NULL;
}

static int __init map_raw_flash(struct mtd_partition *map, int num_parts)
{
    struct mtd_partition anvil_parts[4]; /* BL, user_flash, rawstorage (if needed) */
    struct mtd_partition *part;
    int nparts = 0; /* number of parts to create */

    /* lookup the eRIP partition info */
    part = part_by_name(map, num_parts, "eripv2");
    if (!part) {
        /* the eRIP partition info could not be found. This is a serious error
         * as we cannot define the anvil partitions without it
         */
        return -1;
    }
    erip_base = part->offset;

    memset(anvil_parts, 0, sizeof(anvil_parts));

    /*full flash size partition  used for flashing the flash image(BL+OS) via olympus*/
    anvil_parts[nparts].offset = 0;
    anvil_parts[nparts].size   = mymtd->size;
    anvil_parts[nparts].name   = "anvil_rawflash";
    nparts++; 

    /* the bootloader is at offset 0 and always immediatly followed by the eRIP
     * so its size can be determined by the offset of the eRIP
     */
    anvil_parts[nparts].offset = 0;
    anvil_parts[nparts].size   = part->offset;
    anvil_parts[nparts].name   = "anvil_BL";
    nparts++;

    /* map the user flash. It starts right after the eRIP and extends to the end
     * of the flash. If the size is unknown use the end of bank_2 (for dual bank
     * platforms) or the end of bank_1 for single bank platforms.
     */
    anvil_parts[nparts].name   = "raw_flash";
    anvil_parts[nparts].offset = part->offset + part->size;
    if (mymtd->size ) {
        /* the size of the flash is known for sure */
        anvil_parts[nparts].size = mymtd->size - anvil_parts[nparts].offset;
        nparts++;
    }
    else {
        /* depend on the bank info to define the size of the raw flash */
        part = part_by_name(map, num_parts, "bank_2");
        if (!part) {
            /* not a dual bank */
            part = part_by_name(map, num_parts, "bank_1");
            if (part) {
                anvil_parts[nparts].size = (part->offset+part->size) - anvil_parts[nparts].offset;
                nparts++;
            }
        }
    }

#if defined(CONFIG_MTD_SPEEDTOUCH_BTAB)
    /* make sure a rawstorage partition is present to make the bankmgr happy */
    part = part_by_name(map, num_parts, "rawstorage");
    if( !part ) {
        anvil_parts[nparts].name = "rawstorage";
        anvil_parts[nparts].size = 0x00080000; /* large enough but not too large */
        nparts++;
    }
#endif

    return add_mtd_partitions(mymtd, anvil_parts, nparts);
}

#define ERIP_SIZE 0x00020000
struct rip_index_entry
{
    /* only define the fields we check, not the real flash structure */
    uint16_t id;
    uint32_t offset;
};

/* interpret a number of bytes as a big endian unsigned integer */
static uint32_t mkint_be(unsigned char *m, int num_bytes)
{
    uint32_t r = 0;
    int i;
    for(i=0 ; i<num_bytes ; i++) {
        r = (r<<8) | m[i];
    }
    return r;
}

#define INDEX_ENTRY_SIZE 18
static int read_rip_index_entry(uint64_t off, struct rip_index_entry *entry)
{
    int ret;
    int retlen;
    unsigned char raw_entry[INDEX_ENTRY_SIZE];

    ret = bcm_flash_read(off, sizeof(raw_entry), &retlen, raw_entry);
    if (ret || (retlen != sizeof(raw_entry))) {
        return 0;
    }

    entry->id = (uint16_t)mkint_be(raw_entry, 2);
    entry->offset = mkint_be(raw_entry+2, 4);

    return 1;
}
static int is_erip_v2(uint64_t base)
{
    int retlen;
    int ret;
    uint64_t off = base + ERIP_SIZE - 4; //base of header

    /* these id's need to be present for a valid ERIP */
    int wanted_ids[] = {
        0x0012, /* serial number */
        0x0028, /* FIA */
        0x0032, /* Ethernet MAC Address */
        0x0040, /* Board name */
        0xFFFF  /* sentinel to mark the end of the list */
    };

    unsigned char hdr;

    printk(KERN_WARNING "Looking for ERIP at %08X\n", (unsigned int)base);

    ret = bcm_flash_read(off, sizeof(hdr), &retlen, &hdr);
    if (ret || (retlen != sizeof(hdr))) {
        printk(KERN_ERR "Failed to read header at %08X\n", (unsigned int)off);
        return 0;
    }
    if (hdr != 0x02 ) {
        printk(KERN_WARNING "Header byte was not 02 but %02X\n", (int)hdr);
        return 0;
    }


    while (off>base+INDEX_ENTRY_SIZE) {
        struct rip_index_entry entry;

        off -= INDEX_ENTRY_SIZE; /* the index extends backwards from the end */
        if (!read_rip_index_entry(off, &entry)) {
            printk(KERN_WARNING "Failed to read index entry at %08X\n", (unsigned)off);
            break;
        }
        if (entry.id != 0xFFFF ) {
            /* remove ID from wanted list (if it's in) */
            int *wanted = wanted_ids;
            while( (*wanted!=0xFFFF) && (*wanted!=entry.id)) wanted++;
            while( *wanted != 0xFFFF ) {
                wanted[0] = wanted[1];
                wanted++;
            }
        }
        else {
            /* all index entries read */
            printk(KERN_WARNING "Not all ID's found\n");
            break;
        }

        /* The offset used to be the absolute offset from the beginning of the
         * flash. This is true as long as the RIP is found at offset 0x20000 in
         * the flash. If it moves to 0x40000 or even further then this is no
         * longer true.
         * So now we can consider the offset as the offset from the start of
         * the RIP plus 0x20000.
         * So here we adjust for that.
         */
        entry.offset = entry.offset - 0x20000 + base;
        if ((entry.offset<base) || (off<entry.offset)) {
            /* offset outside of range */
            printk(KERN_ERR "Offset for index out of range: %08X\n", (unsigned)entry.offset);
            break;
        }

        if( wanted_ids[0]==0xFFFF) {
            /* the wanted_id list is empty, all were present */
            printk(KERN_WARNING "All wanted ERIP ID's were found\n");
            return 1;
        }
    }
    return 0;
}
static uint64_t __init locate_erip_v2(void)
{
    uint64_t base;

    /* check for an ERIP at some possible offsets */
    for( base=0x00020000 ; base<0x00220000 ; base+=ERIP_SIZE) {
        if (is_erip_v2(base)) {
            return base;
        }
    }
    /* not found */
    return 0;
}

static int __init guess_partitions(void)
{
    /* here we guess were the eRIP is located. If we guess wrong, the RIP
     * cannot be accessed and Anvil will not work, but no harm is done.
     */
    struct mtd_partition gparts[2];

    memset(&gparts, 0, sizeof(gparts));
    gparts[0].name  = "eripv2";
    gparts[0].size  = ERIP_SIZE;
    gparts[0].offset = locate_erip_v2();
    if (gparts[0].offset==0) {
        printk(KERN_WARNING "No ERIP found\n");
        return -1;
    }
    printk(KERN_WARNING "Guessed location of eRIP at 0x%08X", (int)gparts[0].offset);

    /* make bankmgr happy. bankmgr was updated to not format the space if no
     * banktable is found. (only for Anvil)
     * If it were to format the space and our guess was wrong, it could render
     * the board unusable.
     */
    gparts[1].name    = "rawstorage";
    gparts[1].offset  = gparts[0].offset + gparts[0].size;
    gparts[1].size    = 0x00080000; /* large enough but not too large */

    if (add_mtd_partitions(mymtd, gparts, 2) == 0) {
        return map_raw_flash(gparts, 2);
    }
    return -1;
}

#else
static inline int map_raw_flash(struct mtd_partition *map, int num_parts)
{
    return 0;
}

static inline int __init guess_partitions(void)
{
    return -1;
}

#endif

#ifdef CONFIG_MTD_SPEEDTOUCH_BTAB

#include "export/bankmgr.h"
#include "bankmgr_p.h"

static int __init create_btab_partitions(
    struct mtd_partition  *parts,
    int                   count)
{
    int ret = add_mtd_partitions(mymtd, parts, count);

    if (ret == 0) {
        ret = map_raw_flash(parts, count);
    }

    return ret;
}

#ifdef CONFIG_MTD_SPEEDTOUCH_MAP_RAW_FLASH
static int __init use_btab(struct bank_table *btab)
{
    int ret = -1;
    struct mtd_partition *btab_parts = NULL;
    unsigned btab_parts_size = 0;
    int i;

    /* Note that the banktable loaded by the bankmgr has all numbers stored in
     * Big Endian (network) order. Bankmgr will convert them when needed so we
     * will need to do the same.
     */
    int num_banks = (int)ntohs(btab->num_banks);

    /* allocate memory for the part defs */
    btab_parts_size = sizeof(struct mtd_partition) * num_banks;
    printk(KERN_WARNING "using provided bank table, num_banks=%d\n", num_banks);
    btab_parts = kmalloc(btab_parts_size, GFP_KERNEL);
    if (!btab_parts) {
        printk(KERN_ERR "Failed to allocate memory\n");
        goto out;
    }
    memset(btab_parts, 0, btab_parts_size);

    /* fill the part info */
    for (i = 0; i < num_banks; i++) {
        btab_parts[i].offset  = ntohl(btab->banks[i].bank_offset);
        btab_parts[i].size    = ntohl(btab->banks[i].size);
        btab_parts[i].name    = btab->banks[i].name;
        printk(KERN_DEBUG "bank #%d: %x, %x (%s)\n",
               i,
               (unsigned)btab_parts[i].offset,
               (unsigned)btab_parts[i].size,
               btab_parts[i].name);
    }

    ret = create_btab_partitions(btab_parts, num_banks);
out:
    if (btab_parts) {
        kfree(btab_parts);
    }
    return ret;
}

#endif

static int __init parse_btab(void)
{
    loff_t off;
    int ret = -1, i, retlen;
    unsigned long boot_bank_offset;
    struct mtd_partition *btab_parts = NULL;
    struct bank *banks = NULL;
    struct bank_table btab;

    if (btab_start == 0) {
        printk(KERN_ERR "ERROR: uninitialised 'btab_start'\n");
        goto out;
    }

    if (btab_bank == BTAB_ID_NULL) {
        printk(KERN_WARNING "WARNING: uninitialised 'btab_bank'\n");
    }

    off = btab_start - CONFIG_SPEEDTOUCH_FLASH_START;

    ret = bcm_flash_read(off, sizeof(btab), &retlen, (unsigned char *)&btab);
    if (ret || (retlen != sizeof(btab))) {
        printk(KERN_ERR "failed to read a 'btab' header\n");
        goto out;
    }
#if CONFIG_RAWSTORAGE_VERSION < 2
    if (strncmp(btab.magic, "BTAB", 4)) {
        printk(KERN_ERR "Failed to locate a 'BTAB' record\n");
        goto out;
    }
#else /* RAWSTORAGE V2 */
    if (strncmp(btab.magic, "BTA2", 4)) {
        printk(KERN_ERR "Failed to locate a 'BTA2' record\n");
        goto out;
    }
#endif

    btab.checksum = ntohl(btab.checksum);
    btab.version = ntohs(btab.version);
    btab.num_banks = ntohs(btab.num_banks);

    printk("parse_btab: num_banks (%d)\n", btab.num_banks);

    banks = kmalloc(sizeof(struct bank) * btab.num_banks, GFP_KERNEL);
    if (!banks) {
        goto out;
    }
    btab_parts = kmalloc(sizeof(struct mtd_partition) * btab.num_banks, GFP_KERNEL);
    if (!btab_parts) {
        goto out;
    }
    memset(btab_parts, 0, sizeof(struct mtd_partition) * btab.num_banks);

    if ((btab_bank != BTAB_ID_NULL) && (btab_bank >= btab.num_banks)) {
        printk("parse_btab: wrong value for the active bank (%d)\n", btab_bank);
        goto out;
    }

    off += sizeof(btab);

    ret = bcm_flash_read(off, sizeof(struct bank) * btab.num_banks,
                         &retlen, (unsigned char *)banks);
    if (ret || (retlen != sizeof(struct bank) * btab.num_banks)) {
        printk("failed to read banks\n");
        goto out;
    }

    for (i = 0; i < btab.num_banks; i++) {
        btab_parts[i].offset = ntohl(banks[i].bank_offset);
        btab_parts[i].size = ntohl(banks[i].size);
        btab_parts[i].name = banks[i].name;
    }

    if (btab_bank != BTAB_ID_NULL) {
        /* In practice, this will create the rootfs partition.  If not provided by the bootloader,
         * like when loading from RAM, we do not create this partition as this does not make sense */

        boot_bank_offset = btab_parts[btab_bank].offset;

        /*
         * Adjust offsets of partitions which have been specified relatively
         * to their bank's start:
         */
        for (i = 0; i < SPEEDTOUCH_NR_PARTS; i++) {
            speedtouch_partitions[i].offset += boot_bank_offset;
        }

        /*
         * First add partitions specified via a kernel config
         * so users may rely on their ordering (e.g. a device number
         * for rootfs as specified in "root=" to mount it)
         * and then partitions being configured via BTAB.
         */
        ret = add_mtd_partitions(mymtd, speedtouch_partitions, SPEEDTOUCH_NR_PARTS);
    }

    ret = create_btab_partitions(btab_parts, btab.num_banks);
out:
    if (ret != 0) {
        printk(KERN_ERR "parse_btab: failed to create MTD partitions\n");
    }
    if (banks) {
        kfree(banks);
    }
    if (btab_parts) {
        kfree(btab_parts);
    }

    return ret;
}

static int __init load_partitions(void *btab)
{
#ifdef CONFIG_MTD_SPEEDTOUCH_MAP_RAW_FLASH
    if (btab) {
        return use_btab((struct bank_table *)btab);
    }
#endif
    (void)btab;
    return parse_btab();
}

#endif /* CONFIG_MTD_SPEEDTOUCH_BTAB */

#ifdef CONFIG_MTD_SPEEDTOUCH_DYNAMIC
static int __init load_partitons()
{
    struct mtd_partition  *mtd_parts = NULL;
    int                   mtd_parts_nb;

    mtd_parts_nb  = parse_mtd_partitions(mymtd, part_probes, &mtd_parts, 0);
    ret           = add_mtd_partitions(mymtd, mtd_parts, mtd_parts_nb);
    if (ret == 0) {
        ret = map_raw_flash(mtd_parts, mtd_parts_nb);
    }
    if (mtd_parts) {
        kfree(mtd_parts);
    }
}

#endif

#ifdef BUILTIN_PARTS
static int __init load_partitions(void *b)
{
    int ret;
#ifdef CONFIG_MTD_SPEEDTOUCH_DUAL_BANK_MANUAL
    unsigned long boot_bank_offset;
    int i;
    if (btab_bank != BTAB_ID_NULL && btab_bank >= 1 && btab_bank <= SPEEDTOUCH_NR_PARTS) {
        boot_bank_offset = speedtouch_partitions[btab_bank - 1].offset;
        for (i = 0; i < SPEEDTOUCH_NR_PARTS; i++) {
            if (0  == strcmp(speedtouch_partitions[i].name, "rootfs")) {
                speedtouch_partitions[i].offset += boot_bank_offset;
                break;
            }
        }
    }
    else {
        printk(KERN_ERR "manual_dual_bank: wrong value for the active bank (%d)\n", btab_bank);
        return -1;
    }

#endif

    (void)b;

    ret = add_mtd_partitions(mymtd, speedtouch_partitions,
                                 SPEEDTOUCH_NR_PARTS);

    if (ret == 0) {
        ret = map_raw_flash(speedtouch_partitions, SPEEDTOUCH_NR_PARTS);
    }

    return ret;
}

#endif

static int __init create_partitions(void *btab)
{
    int ret = load_partitions(btab);

#ifdef CONFIG_MTD_SPEEDTOUCH_MAP_RAW_FLASH
    if (ret == 0){
        if( (erip_base==0) || !is_erip_v2(erip_base) ) {
            /* no erip defined or it is invalid */
            del_mtd_partitions(mymtd);
            erip_base = 0;
            ret = 1; /* force partition guessing */
        }
    }
#endif

    if (ret != 0) {
        ret = guess_partitions();
    }
#if defined(CONFIG_SPEEDTOUCH_BLVERSION_OFFSET)
    if (ret == 0) {
        ret = add_mtd_partitions(mymtd, &blversion_partition, 1);
    }
#endif

    return ret;
}

int __init init_bcm(void)
{
    int ret = -ENXIO;

    printk(KERN_NOTICE "Gateway flash mapping\n");
#if defined(CONFIG_MTD_NAND_TL)
    mymtd = do_map_probe("technicolor-nand-tl", 0);
#elif defined(CONFIG_MTD_SPI)
    speedtouch_map.size = 0;
    mymtd = do_map_probe("technicolor-spi", &speedtouch_map);
    /* if mymtd size is nonzero we can be confident it was set by probing the
     * chip, and not merely by copying the value from speedtouch_map.
     */
#else
    mymtd = do_map_probe("cfi_probe", &speedtouch_map);
    /* we were not able to verify the size returned here is correct, so we will
     * take the prudent approach and pretend it is completely unknown */
     */
    mymtd->size = 0;
#endif
    printk(KERN_NOTICE "flash mapping initialized, size=%d Mb\n", (int)(mymtd->size/(1024*1024)));
    if (mymtd) {
        mymtd->owner  = THIS_MODULE;
        ret           = create_partitions(0);
    }

    return ret;
}

#ifdef CONFIG_MTD_SPEEDTOUCH_MAP_RAW_FLASH
static int partitions_invalid = 0;
int partitions_invalidate(void)
{
    int f = partitions_invalid;

    if (!partitions_invalid) {
        partitions_invalid = 1;
        printk(KERN_NOTICE "reload flag for partitions set\n");
    }
    return f;
}
EXPORT_SYMBOL(partitions_invalidate);

int partitions_are_invalid(void)
{
    return partitions_invalid;
}
EXPORT_SYMBOL(partitions_are_invalid);

int partitions_reload(void *btab)
{
    if (partitions_invalid) {
        printk(KERN_NOTICE "reloading partions\n");
        del_mtd_partitions(mymtd);
        create_partitions(btab);
        partitions_invalid = 0;
    }
    return 0;
}
EXPORT_SYMBOL(partitions_reload);
#endif

static void __exit cleanup_bcm(void)
{
    if (mymtd) {
        del_mtd_partitions(mymtd);
        map_destroy(mymtd);
    }
}

module_init(init_bcm);
module_exit(cleanup_bcm);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Joris Lijssens <Joris.Lijssens@thomson.net");
MODULE_DESCRIPTION("MTD map driver for speedtouch boards");
