#ifndef __NANDFLASH_TBBT_H__
#define __NANDFLASH_TBBT_H__

typedef struct
{
	unsigned char *phys_addr;
	unsigned char *phys_translated_addr;
	unsigned char isbad;
	unsigned char isreassigned;
} t_tbbt_blockinfo;

typedef struct
{
	unsigned char *phys_addr;
	unsigned char isbad;
	unsigned char isused;
} t_tbbt_spareblockinfo;

typedef struct
{
	char magic[4];
	unsigned long crc32;
	unsigned char version;
	unsigned char *logic_flash_start;
	unsigned char *logic_flash_end;
	unsigned char *logic_bbt_addr;
	unsigned char *logic_bbt_mirror_addr;
	unsigned char *phys_spare_start;
	unsigned long max_badblock_count;
	unsigned long block_count;
	unsigned long spareblock_count;
	t_tbbt_blockinfo *block_info;
	t_tbbt_spareblockinfo *spareblock_info; /*LAST!*/
} t_tbbt;

static const char tbbt_magic[]="tBBT";

#define TBBT_SIZE sizeof(t_tbbt)
#define TBBT_BLOCK_SIZE sizeof(t_tbbt_blockinfo)
#define TBBT_SPAREBLOCK_SIZE sizeof(t_tbbt_spareblockinfo)

#endif
