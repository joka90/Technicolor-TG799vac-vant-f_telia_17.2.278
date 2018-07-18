#include <generated/autoconf.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/err.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/brcmnand.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/flashchip.h>
#include <linux/crc32.h>
#include <linux/proc_fs.h>
#include "nandflash_tbbt.h"


//#define NAND_TRACE_DEBUG
//#define CONFIG_MTD_NAND_TL_TEST

#define BOOTLOADER_OFFSET 0x20000
#define MAX_RETRIES 10


#define NAND_ERROR(format, args...) printk("[NAND] ERROR (%s): "format"\n\r", __FUNCTION__, ##args)
#define NAND_TRACE(format, args...) printk("[NAND] : "format"\n\r", ##args)

#ifdef NAND_TRACE_DEBUG
#define NAND_DEBUG(format, args...) printk("[NAND] DBG (%s): "format"\n\r", __FUNCTION__, ##args)
#else
#define NAND_DEBUG(format, args...)
#endif

#define TO_OOB_SIZE(x) ((x * info_tl->mtd_tl->ecclayout->oobavail) / info_tl->mtd_tl->writesize)
#define TO_OOB_ADDR(x) (((x & ~(info_tl->mtd_tl->writesize - 1)) * info_tl->mtd_tl->ecclayout->oobavail) / info_tl->mtd_tl->writesize)

void __exit nand_tl_exit(void);

static loff_t nand_tl_trans_addr(loff_t addr);
static int nand_tl_isbad_bbt (struct mtd_info *mtd, loff_t ofs, int allowbbt);
static int nand_tl_block_isbad(struct mtd_info *mtd, loff_t ofs);
static int nand_tl_erase(struct mtd_info *mtd, struct erase_info *instr);
static int nand_tl_erase_bbt(struct erase_info *instr, int allow_bbt);
static int nand_tl_read(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, uint8_t *buf);
static int nand_tl_read_nolock(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, uint8_t *buf, int handle_ecc_errors);
static int nand_tl_read_oob(struct mtd_info *mtd, loff_t from, struct mtd_oob_ops *ops);
static int nand_tl_write(struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const uint8_t *buf);
static int nand_tl_write_bbt(loff_t to, size_t len, size_t *retlen, const uint8_t *buf, int allow_bbt);
static int nand_tl_write_blocklimited(loff_t to, struct mtd_oob_ops *ops, int isbbt);
static int nand_tl_copy_block(loff_t src, struct mtd_oob_ops *ops, loff_t dest);
static int nand_tl_check_empty_page(struct mtd_oob_ops *ops);
static int nand_tl_write_oob(struct mtd_info *mtd, loff_t to, struct mtd_oob_ops *ops);
static void nand_tl_sync(struct mtd_info *mtd);
static int nand_tl_suspend(struct mtd_info *mtd);
static void nand_tl_resume(struct mtd_info *mtd);
static int nand_tl_block_markbad(struct mtd_info *mtd, loff_t ofs);


static int nand_tl_bbt_check_crc(int unload, t_tbbt *tbbt);
static void nand_tl_bbt_update_crc(t_tbbt *tbbt);
static int nand_tl_bbt_print(void);
static int nand_tl_bbt_load(loff_t phys_addr, t_tbbt **tbbt_load_addr);
static int nand_tl_bbt_check_copyblocks(void);
static int nand_tl_bbt_get_spareblock(unsigned char **addr);
static int nand_tl_bbt_add_translation(unsigned char *addr, unsigned char *spare_addr);
static int nand_tl_bbt_add_block(unsigned char *addr, int *pos);
static int nand_tl_bbt_del_block(unsigned char *addr);
static int nand_tl_bbt_save(void);
static int nand_tl_bbt_mark_block(unsigned char *addr);
static int nand_tl_bbt_add_spareblock(unsigned char *addr, int *pos);
static int nand_tl_bbt_free_spareblock(unsigned char *addr);
static void nand_tl_mark_block_bad_and_relocate(unsigned char *phy_addr);

static int nand_tl_get_device(int new_state);
static int ReadProc(char *buf, char **start, off_t offset, int count, int *eof, void *data);
static void nand_tl_release_device(void);
static struct mtd_info *nand_tl_probe(struct map_info *map);
static void nand_tl_destroy(struct mtd_info *mtd_tl);

#ifdef CONFIG_MTD_NAND_TL_TEST
void t_NAND_TEST(int seed);
int max_amount_of_bad_blocks;
int amount_of_bad_blocks;
int code_cov[2];
EXPORT_SYMBOL_GPL(amount_of_bad_blocks);
EXPORT_SYMBOL_GPL(max_amount_of_bad_blocks);
#define NAND_CODE_COV(x) if (!(code_cov[x>>5] & (1<<(x&31)))) {(code_cov[x>>5] |= (1<<(x&31))); NAND_DEBUG("--> LINE DBG 0x%08X 0x%08X <--\n\r", code_cov[0], code_cov[1]);}
#define NAND_CODE_COV_ADDR_SHIFT 			0	/* 1 */
#define NAND_CODE_COV_ADDR_TRANSL 			1	/* 1 */
#define NAND_CODE_COV_BLOCK_CHECKBAD 			2	/* 1 */
#define NAND_CODE_COV_ERASE_BLOCK_BAD 			3	/* 1 */

#define NAND_CODE_COV_ERASE_FAILS 			4	/* 1 */
#define NAND_CODE_COV_ALIGNED_WRITE_BLOCK_BAD 		5	/* 1 */
#define NAND_CODE_COV_BBT_SAVE_RECALL			6	/* 1 */
#define NAND_CODE_COV_BLOCKLIMITED_WRITE_FAILS 		7	/* 1 */

#define NAND_CODE_COV_WRITE_OOB_PARTS 			8	/* 1 */
#define NAND_CODE_COV_READ_OOB_PARTS			9	/* 1 */
#define NAND_CODE_COV_READ_PARTS			10	/* 1 */
#define NAND_CODE_COV_WRITE_PARTS			11	/* 1 */

#define NAND_CODE_COV_WRITE_UNALIN			12	/* 1 */
#define NAND_CODE_COV_COPY_BLOCK			13	/* 1 */
#define NAND_CODE_COV_PAGE_EMPTY			14	/* 1 */
#define NAND_CODE_COV_LOAD_TBBT_NOADDR			15	/* 1 */

#define NAND_CODE_COV_LOAD_TBBT_ADDR			16	/* 1 */
#define NAND_CODE_COV_LOAD_TBBT_NOLOADADDR		17	/* 1 */
#define NAND_CODE_COV_LOAD_TBBT_LOADADDR		18	/* 1 */
#define NAND_CODE_COV_COPYBLOCK_ERASE_FAIL		19	/* 1 */

#define NAND_CODE_COV_COPYBLOCK				20	/* 1 */
#define NAND_CODE_COV_COPYBLOCK_SPARE_ERASE_FAIL	21	/* 1 */
#define NAND_CODE_COV_SAVE_TBBT_PRELIM_DONE		22	/* 1 */

#define NAND_CODE_COV_RECC						23 /* 1 */
#define NAND_CODE_COV_RECC_IN_ABBT				24 /* 0 */
#define NAND_CODE_COV_RECC_IN_MBBT				25 /* 0 */
#define NAND_CODE_COV_RUECC						26 /* 1 */

#else
#define NAND_CODE_COV(x)
#endif

struct nand_tl_info
{
	struct mtd_info *mtd_tl;
	struct mtd_info *mtd_dev;
	t_tbbt *tbbt;
	char bbt_valid;
	int pages_per_block;
	spinlock_t lock;
	wait_queue_head_t wq;
	flstate_t state;
};

static struct nand_tl_info *info_tl;

static loff_t nand_tl_trans_addr(loff_t addr)
{
	int i;
	
	loff_t tmp_addr;
	
	if (info_tl->bbt_valid)
	{
		if (addr > (unsigned int)info_tl->tbbt->logic_flash_start)
			addr -= (unsigned int)info_tl->tbbt->logic_flash_start;

		tmp_addr = (addr & ~(info_tl->mtd_tl->erasesize - 1));
		
		for (i = 0; i < info_tl->tbbt->block_count; i++)
			if (((unsigned int)info_tl->tbbt->block_info[i].phys_addr <= tmp_addr) && (info_tl->tbbt->block_info[i].isreassigned == 0))
			{
				tmp_addr += info_tl->mtd_tl->erasesize;
				NAND_CODE_COV(NAND_CODE_COV_ADDR_SHIFT);
			}
			else if ((unsigned int)info_tl->tbbt->block_info[i].phys_addr == tmp_addr)
			{
				if (info_tl->tbbt->block_info[i].isreassigned == 1)
				{
					tmp_addr = (unsigned int)info_tl->tbbt->block_info[i].phys_translated_addr;
					NAND_CODE_COV(NAND_CODE_COV_ADDR_TRANSL);
				}
				break;
			}
		
		nand_tl_bbt_check_crc(1, info_tl->tbbt);

		return (tmp_addr & ~(info_tl->mtd_tl->erasesize - 1)) | (addr & (info_tl->mtd_tl->erasesize - 1));
	}
	else
		return addr;
}

static int nand_tl_block_isbad(struct mtd_info *mtd, loff_t ofs)
{
	if (!info_tl->bbt_valid)
		return 1; /* say everything is bad : we always need a bbt!!! */

	return 0; /* we hide all bad blocks in the TL */
}

static int nand_tl_isbad_bbt (struct mtd_info *mtd, loff_t ofs, int allowbbt)
{
	int i;
	loff_t addr = ofs & ~(info_tl->mtd_tl->erasesize - 1);

	NAND_CODE_COV(NAND_CODE_COV_BLOCK_CHECKBAD);

	if (!info_tl->bbt_valid)
		return 1; /* say everything is bad : we always need a bbt!!! */
		
	for (i = 0; i < info_tl->tbbt->block_count; i++)
	{
		if ((unsigned int)info_tl->tbbt->block_info[i].phys_addr == addr)
			return info_tl->tbbt->block_info[i].isbad;
	}
	for (i = 0; i < info_tl->tbbt->spareblock_count; i++)
	{
		if ((unsigned int)info_tl->tbbt->spareblock_info[i].phys_addr == addr)
			return info_tl->tbbt->spareblock_info[i].isbad;
	}
	return 0;
}

static int nand_tl_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	int ret;
	if (!info_tl->bbt_valid)
		return -EIO;
	nand_tl_get_device(FL_ERASING);
	NAND_DEBUG("mtd: erasing 0x%08x, len %d", (unsigned int)instr->addr, (unsigned int)instr->len);
	ret =  nand_tl_erase_bbt(instr, 0);
	nand_tl_release_device();
	return ret;
}

static int nand_tl_erase_bbt(struct erase_info *instr, int allow_bbt)
{
	size_t len;
	struct erase_info instr_tmp;
	unsigned char *spare_block;
	int ret;
	
	if ((instr->addr & (info_tl->mtd_tl->erasesize - 1)) || (instr->len & (info_tl->mtd_tl->erasesize - 1)))
	{
		NAND_ERROR("Nand flash TL: unsupported erase request.");
		return -EINVAL;
	}
	if (!allow_bbt)
	{
		if ((instr->addr == (unsigned int)info_tl->tbbt->logic_bbt_addr) || (instr->addr == (unsigned int)info_tl->tbbt->logic_bbt_mirror_addr))
		{
			NAND_ERROR("Nand flash TL: It's not allowed to erase the BBT's");
			return -EPERM;
		}
	}
	
	len = instr->len;
	memcpy(&instr_tmp, instr, sizeof(struct erase_info));
	instr_tmp.len = info_tl->mtd_tl->erasesize;
	instr_tmp.callback = NULL;
	instr_tmp.mtd = info_tl->mtd_dev;
	while(len)
	{
		instr_tmp.fail_addr = ~0;
		instr_tmp.addr = nand_tl_trans_addr(instr->addr);
		ret = mtd_erase(info_tl->mtd_dev, &instr_tmp);
		
		if (ret == -EIO || instr_tmp.state != MTD_ERASE_DONE)
		{
			NAND_DEBUG("erase failed for addr %x. getting spare block...", instr_tmp.fail_addr);
			NAND_CODE_COV(NAND_CODE_COV_ERASE_BLOCK_BAD);
			if ((ret = nand_tl_bbt_get_spareblock(&spare_block)))
			{
				goto bbt_err_out;
			}
			if ((ret = nand_tl_bbt_add_translation((unsigned char *)((unsigned int)instr_tmp.addr), spare_block)))
			{
				goto bbt_err_out;
			}
			/* ASSUMES BLOCK IS ALREADY MARKED BAD IN OOB BY DRIVER */
			if ((ret = nand_tl_bbt_mark_block((unsigned char *)((unsigned int)instr_tmp.addr))))
			{
				goto bbt_err_out;
			}
			if ((ret = nand_tl_bbt_save()))
			{
				goto bbt_err_out;
			}
		}
		else if (ret)
		{
			goto err_out;
		}
		
		len -= info_tl->mtd_tl->erasesize;
		instr->addr += info_tl->mtd_tl->erasesize;
	}
	
	instr->state = MTD_ERASE_DONE;
	mtd_erase_callback(instr);
	return 0; /* should ALWAYS succeed! */
bbt_err_out:
	NAND_ERROR("Nand flash: Unexpected erase error: %d!! Reloading BBT.", ret);
	NAND_CODE_COV(NAND_CODE_COV_ERASE_FAILS);
	nand_tl_bbt_load(0,0);
	return ret;
err_out:
	NAND_ERROR("Nand flash: Unexpected erase error: %d!!", ret);
	return ret;
}

static int nand_tl_read(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, uint8_t *buf)
{
	int ret;
	nand_tl_get_device(FL_READING);
	NAND_DEBUG("mtd: reading from 0x%08x, len %d", (unsigned int)from, (unsigned int)len);
	ret = nand_tl_read_nolock(mtd, from, len, retlen, buf, 1);
	nand_tl_release_device();
	return ret;
}


/* handle a correctable ECC */
static void nandtl_handle_correctable_ecc(loff_t phys_addr, int mark_bad_anyway)
{
	struct mtd_oob_ops ops_donothing, ops_reread;
	unsigned char *spare_addr = 0;
	int copy_back = 0, i = 0;
	struct erase_info instr;
	unsigned char *oob_data = NULL;
	unsigned char *block_data = NULL;
	const size_t oob_block_len = TO_OOB_SIZE(info_tl->mtd_tl->erasesize);
			
	ops_donothing.mode = MTD_OPS_AUTO_OOB;
	ops_donothing.len = ops_donothing.ooblen = ops_donothing.ooboffs = ops_donothing.retlen = ops_donothing.oobretlen = 0;
	ops_donothing.datbuf = ops_donothing.oobbuf = NULL;
	
	instr.mtd = info_tl->mtd_dev;
	instr.len = info_tl->mtd_tl->erasesize;
	instr.callback = NULL;
	instr.fail_addr = ~0;
	
	/* Do not start handling ECC errors if BBT is not loaded yet */
	if (!info_tl->bbt_valid)
	{
		return;
	}
		
	
	/* align to block size */
	phys_addr = phys_addr & ~(info_tl->mtd_tl->erasesize - 1);

	
	NAND_TRACE("Refreshing block with recoverable ECC");
	if ((phys_addr == nand_tl_trans_addr((unsigned int)info_tl->tbbt->logic_bbt_addr)) || (phys_addr == nand_tl_trans_addr((unsigned int)info_tl->tbbt->logic_bbt_mirror_addr)))
	{
		/* Should not get here anyway as BBT errors are handled by the BBT load routine */
		NAND_ERROR("Do not handle ECC errors in BBT");
		return;
	}
	
	if ((unsigned long)phys_addr < BOOTLOADER_OFFSET) 
	{
		/* do not handle these errors; these could be handled by the inflater which is not aware of the BBT */
		NAND_ERROR("Do not handle ECC errors in the bootloader");
		return;
	}
	
	/* get a spare block */
	if (nand_tl_bbt_get_spareblock(&spare_addr))
	{
		goto err_out;
	}

	/* add the translation to the bbt */
	if (nand_tl_bbt_add_translation((unsigned char *)((unsigned int)phys_addr), spare_addr)) 
	{

		goto err_out;
	}

	/* copy the block to the spare block */
	if (nand_tl_copy_block(phys_addr, &ops_donothing, (loff_t)(unsigned int)spare_addr)) 
	{
		goto err_out;
	}

	/* save the bbt to flash */
	if (nand_tl_bbt_save()) 
	{
		goto err_out;
	}

	/* Detect final spare_addr.  This might be different from previous nandflash_bbt_get_spareblock call,
	   since the write to the old spare_addr might have had errors */
	spare_addr = 0;

	for (i = 0; i < info_tl->tbbt->block_count; i++)
	{
		if ((unsigned int)info_tl->tbbt->block_info[i].phys_addr == phys_addr)
		{
			if (info_tl->tbbt->block_info[i].isreassigned == 1)
			{
				spare_addr = (unsigned char *)info_tl->tbbt->block_info[i].phys_translated_addr;
			}
			break;
		}
	}
	
	instr.addr = (unsigned int)phys_addr;
	
	oob_data = kmalloc(oob_block_len, GFP_KERNEL);
	if (!oob_data)
	{
		NAND_ERROR("Could not allocate memory");
		goto err_out;
	}
	
	block_data = kmalloc(info_tl->mtd_tl->erasesize, GFP_KERNEL);
	if (!block_data)
	{
		kfree(oob_data);
		NAND_ERROR("Could not allocate memory");
		goto err_out;
	}

	ops_reread.mode = MTD_OPS_AUTO_OOB;
	ops_reread.ooblen = oob_block_len;
	ops_reread.oobbuf = oob_data;
	ops_reread.len = info_tl->mtd_tl->erasesize;
	ops_reread.datbuf = block_data;
	ops_reread.ooboffs = ops_reread.retlen = ops_reread.oobretlen = 0;
	
	/* Only copy back if the original address was not a spare, and the BBT still has a reassigned entry */
	copy_back = ((unsigned int)phys_addr < (unsigned int)info_tl->tbbt->phys_spare_start) && spare_addr;
	/* write back block and check whether it is now without ECC errors */
	if (mark_bad_anyway ||
		(copy_back && mtd_erase(info_tl->mtd_dev, &instr)) ||
		(copy_back && (nand_tl_copy_block((loff_t)(unsigned int)spare_addr, &ops_donothing, phys_addr))) ||
		mtd_read_oob(info_tl->mtd_dev, phys_addr, &ops_reread))
	{
		kfree(oob_data);
		kfree(block_data);
		NAND_CODE_COV(NAND_CODE_COV_RUECC);
		NAND_ERROR("R/UECC detected; mark block as bad");
		mtd_block_markbad(info_tl->mtd_dev, phys_addr);
		nand_tl_bbt_mark_block((unsigned char *)(unsigned int)phys_addr);
		if (nand_tl_bbt_save())
		{
			goto err_out;
		}
		return;
	}
	kfree(oob_data);
	kfree(block_data);
		
	
	if (copy_back)
	{
		/* point back to the original block */
		if (nand_tl_bbt_del_block((unsigned char *)((unsigned int)phys_addr)))
		{
			goto err_out;
		}
		if (nand_tl_bbt_save())
		{
			goto err_out;
		}
	}
	NAND_TRACE("ECC error handled successfully");
	return;
	
err_out:
	NAND_ERROR("Problem during ECC refresh");
	nand_tl_bbt_load(0,0);

}
			
static void nand_tl_mark_block_bad_and_relocate(unsigned char *phy_addr)
{
	unsigned char *spare_addr = 0;
	phy_addr = (unsigned char *)((unsigned long)phy_addr & ~(info_tl->mtd_tl->erasesize - 1));	
	

	if (nand_tl_bbt_get_spareblock(&spare_addr)) {
		goto err_out;
	}
	/* add the translation to the bbt */
	if (nand_tl_bbt_add_translation(phy_addr, spare_addr)) {
		goto err_out;
	}
	
	/* if this fails, we will still record the problem in the BBT, so don't check return value */
	mtd_block_markbad(info_tl->mtd_dev, (loff_t)(unsigned int)phy_addr);
	
	if (nand_tl_bbt_mark_block(phy_addr))
	{
		goto err_out;
	}
	if (nand_tl_bbt_save())
	{
		goto err_out;
	}
	return;
			
err_out:
	NAND_ERROR("Block not marked bad");
	nand_tl_bbt_load(0, 0);
}

static int nand_tl_read_nolock(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, uint8_t *buf, int handle_ecc_errors)
{
	loff_t block_addr, block_offset;
	size_t size, retlen_tmp;
	unsigned long index = 0;
	int ret = 0, total_ret = 0;
	int retries = 0;
#ifdef NAND_TRACE_DEBUG
	int print_tl = 0;
#endif
	block_addr = from & ~(info_tl->mtd_tl->erasesize - 1);
	block_offset = from - block_addr;
	size = info_tl->mtd_tl->erasesize - block_offset;
	
	*retlen = 0;
	retlen_tmp = 0;
	
	if(size > len)
		size = len;

	do
	{
		retries = 0;
		ret = mtd_read(info_tl->mtd_dev, nand_tl_trans_addr(from), size, &retlen_tmp, &buf[index]);
		if (handle_ecc_errors)
		{
			if (ret == -EUCLEAN)
			{
				NAND_CODE_COV(NAND_CODE_COV_RECC);
				nandtl_handle_correctable_ecc(nand_tl_trans_addr(from), 0);
			}
			else if (ret == -EBADMSG)
			{
				while (ret == -EBADMSG && retries < MAX_RETRIES)
				{
					ret = mtd_read(info_tl->mtd_dev, nand_tl_trans_addr(from), size, &retlen_tmp, &buf[index]);
					retries++;
				}
				
				if (ret == -EBADMSG)
				{
					nand_tl_mark_block_bad_and_relocate((unsigned char *)(unsigned int)nand_tl_trans_addr(from));
					panic("Unrecoverable ECC error in NAND");
				}
				else
				{
					nandtl_handle_correctable_ecc(nand_tl_trans_addr(from), 1);
				}
				
			}
		}
		
		/* do not pass RECC errors further */
		if (ret == -EUCLEAN)
		{
			ret = 0;
		}
		
		*retlen += retlen_tmp;
		
		if (ret != -EBADMSG && ret != 0)
		{
			total_ret = ret;
			NAND_DEBUG("getting error %d, aborting", ret);
			break;
		}
		else if (total_ret != -EBADMSG)
		{
			total_ret = ret;
		}
		
		if (retlen_tmp != size)
		{
			NAND_DEBUG("got unexpected length (got %zd, expected %zd). aborting", retlen_tmp, size);
			break;
		}
		
		len -= size;
#ifdef NAND_TRACE_DEBUG
		if (len || print_tl)
		{
			NAND_DEBUG("[%d] splitted read in block parts: logic addr %llx, len %zd ", print_tl, from, size);
			print_tl++;
			NAND_CODE_COV(NAND_CODE_COV_READ_PARTS);
		}
#endif
		from += size;
		index += size;
		if(len > info_tl->mtd_tl->erasesize)
			size = info_tl->mtd_tl->erasesize;
		else
			size = len;
	} while (len);
	
#ifdef NAND_TRACE_DEBUG
	if (total_ret)
		NAND_DEBUG("warning: returning %d", total_ret);
#endif
	return total_ret;
}

static int nand_tl_read_oob(struct mtd_info *mtd, loff_t from, struct mtd_oob_ops *ops)
{
	int oobavail = info_tl->mtd_dev->ecclayout->oobavail;
	struct mtd_oob_ops ops_tmp;
	loff_t block_addr, block_offset;
	int ret = 0, total_ret = 0;
	size_t len = ops->ooblen;
#ifdef NAND_TRACE_DEBUG
	int print_tl = 0;
#endif
	
	if (ops->mode != MTD_OPS_AUTO_OOB || ops->ooboffs != 0 || ops->datbuf != NULL)
	{
		NAND_ERROR("Nand flash TL: unsupported read OOB request.");
		return -EINVAL;
	}

	nand_tl_get_device(FL_READING);
	ops->oobretlen = 0;
	memcpy(&ops_tmp, ops, sizeof(struct mtd_oob_ops));

	{
		from &= ~(info_tl->mtd_tl->writesize - 1);
		block_addr = from & ~(info_tl->mtd_tl->erasesize - 1);
		block_offset = info_tl->mtd_tl->erasesize - (from - block_addr);
		ops_tmp.ooblen = ((unsigned int)block_offset / info_tl->mtd_tl->writesize) * oobavail;

		if(ops_tmp.ooblen > len)
			ops_tmp.ooblen = len;
		
		do
		{
			ret = mtd_read_oob(info_tl->mtd_dev, nand_tl_trans_addr(from), &ops_tmp);
			ops->oobretlen += ops_tmp.oobretlen;

			if (ret != -EUCLEAN && ret != -EBADMSG && ret != 0)
			{
				total_ret = ret;
				NAND_DEBUG("getting error %d, aborting", ret);
				break;
			}
			else if (total_ret != -EBADMSG)
			{
				total_ret = ret;
			}
			
			if (ops_tmp.oobretlen != ops_tmp.ooblen)
			{
				NAND_DEBUG("got unexpected length (got %zd, expected %zd). aborting", ops_tmp.oobretlen, ops_tmp.ooblen);
				break;
			}
			
			len -= ops_tmp.oobretlen;
#ifdef NAND_TRACE_DEBUG
			if (len || print_tl)
			{
				NAND_DEBUG("[%d] splitted read_OOB in block parts: logic addr %llx, len %zd, %d bytes per page", print_tl, from, ops_tmp.ooblen, oobavail);
				print_tl++;
				NAND_CODE_COV(NAND_CODE_COV_READ_OOB_PARTS);
			}
#endif
			from += block_offset;
			block_offset = info_tl->mtd_tl->erasesize;
			ops_tmp.oobbuf += ops_tmp.oobretlen;
			if(len > (info_tl->pages_per_block * oobavail))
				ops_tmp.ooblen = (info_tl->pages_per_block * oobavail);
			else
				ops_tmp.ooblen = len;
		} while (len);
	}
#ifdef NAND_TRACE_DEBUG
	if (total_ret)
		NAND_DEBUG("warning: returning %d", total_ret);
#endif
	nand_tl_release_device();
	return total_ret;
}

static int nand_tl_write(struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const uint8_t *buf)
{
	int ret;
	if (!info_tl->bbt_valid)
		return -EIO;
	nand_tl_get_device(FL_WRITING);
	NAND_DEBUG("mtd: writing to 0x%08x, len %d", (unsigned int)to, (unsigned int)len);
	ret = nand_tl_write_bbt(to, len, retlen, buf, 0);
	nand_tl_release_device();
	return ret;
}

static int nand_tl_write_bbt(loff_t to, size_t len, size_t *retlen, const uint8_t *buf, int allow_bbt)
{
	loff_t block_addr, block_offset;
	size_t size;
	unsigned long index = 0;
	int ret = 0;
	struct mtd_oob_ops ops;
#ifdef NAND_TRACE_DEBUG
	int print_tl = 0;
#endif
	ops.mode = MTD_OPS_AUTO_OOB;
	ops.oobbuf = NULL;
	ops.ooblen = ops.ooboffs = ops.retlen = ops.oobretlen = 0;
	
	block_addr = to & ~(info_tl->mtd_tl->erasesize - 1);
	block_offset = to - block_addr;
	size = info_tl->mtd_tl->erasesize - block_offset;
	
	*retlen = 0;
	
	if(size > len)
		size = len;

	do
	{
		if (!allow_bbt && (((to & ~(info_tl->mtd_tl->erasesize - 1)) == (unsigned int)info_tl->tbbt->logic_bbt_addr) 
		                || ((to & ~(info_tl->mtd_tl->erasesize - 1)) == (unsigned int)info_tl->tbbt->logic_bbt_mirror_addr)))
		{
			NAND_ERROR("Nand flash: can't allow to write to bbt blocks!");
			return -EINVAL;
		}

		ops.len = size;
		ops.datbuf = (uint8_t *)&buf[index];
		
		ret = nand_tl_write_blocklimited(nand_tl_trans_addr(to), &ops, allow_bbt);
		
		if (ret)
			return -EIO;
			
		*retlen += ops.retlen;
		if (ops.retlen != size)
		{
			NAND_DEBUG("got unexpected length (got %zd, expected %zd). aborting", ops.retlen, size);
			return -EIO;
		}
		
		len -= size;
#ifdef NAND_TRACE_DEBUG
		if (len || print_tl)
		{
			NAND_DEBUG("[%d] splitted write in block parts: logic addr %llx, len %zd ", print_tl, to, size);
			print_tl++;
			NAND_CODE_COV(NAND_CODE_COV_WRITE_PARTS);
		}
#endif
		to += size;
		index += size;
		if(len > info_tl->mtd_tl->erasesize)
			size = info_tl->mtd_tl->erasesize;
		else
			size = len;
	} while (len);
	
	return 0;
}

static int nand_tl_write_blocklimited(loff_t to, struct mtd_oob_ops *ops, int isbbt)
{
	unsigned char *spare_addr;
	size_t size, oobsize;
	struct mtd_oob_ops ops_tmp;
	unsigned long index, oobindex;
	int ret;
	size_t len = ops->len, ooblen = ops->ooblen;

	if (ops->mode != MTD_OPS_AUTO_OOB || ops->ooboffs != 0)
	{
		NAND_ERROR("Nand flash TL: unsupported write OOB request.");
		return -EINVAL;
	}
	if (ops->len && ((to & ~(info_tl->mtd_tl->erasesize - 1)) != ((to + ops->len - 1) & ~(info_tl->mtd_tl->erasesize - 1))))
	{
		NAND_DEBUG("requested write length exceeds block (address = %llx, length = %d bytes", to, ops->len);
		return -EINVAL;
	}
	if (ops->ooblen && ((to & ~(info_tl->mtd_tl->erasesize - 1)) != ((to + ((ops->ooblen * info_tl->mtd_tl->writesize)/info_tl->mtd_tl->ecclayout->oobavail)-1) & ~(info_tl->mtd_tl->erasesize - 1))))
	{
		NAND_DEBUG("requested write oob length exceeds block (address = %llx, length = %d bytes", to, ops->ooblen);
		return -EINVAL;
	}

	ops->retlen = ops->oobretlen = 0;
	
	ops_tmp.mode = MTD_OPS_AUTO_OOB;
	ops_tmp.ooboffs = 0;

	if (ops->len && ((to & (info_tl->mtd_tl->writesize - 1)) || ((to + ops->len) & (info_tl->mtd_tl->writesize - 1))))
	{
		NAND_CODE_COV(NAND_CODE_COV_WRITE_UNALIN);
		NAND_DEBUG("nand write: not aligned, using copyblocks.");
		if (isbbt)
			goto bbt_err_out;
		if ((ret = nand_tl_bbt_get_spareblock(&spare_addr)))
			goto bbt_err_out;
		if ((ret = nand_tl_bbt_add_translation((unsigned char *)(unsigned int)(to & ~(info_tl->mtd_tl->erasesize - 1)), spare_addr)))
			goto bbt_err_out;
		if ((ret = nand_tl_copy_block(to, ops, (loff_t)(unsigned int)spare_addr)))
			goto bbt_err_out;
		if ((ret = nand_tl_bbt_save()))
			goto bbt_err_out;
		nand_tl_bbt_check_copyblocks();
	}
	else
	{
		size = info_tl->mtd_tl->writesize;
		oobsize = info_tl->mtd_tl->ecclayout->oobavail;

		index = 0;
		oobindex = 0;
		
		if(size > len)
			size = len;
		if(oobsize > ooblen)
			oobsize = ooblen;

		do
		{
			ops_tmp.len = size;
			ops_tmp.datbuf = &ops->datbuf[index];
			ops_tmp.ooblen = oobsize;
			ops_tmp.oobbuf = &ops->oobbuf[oobindex];
			ops_tmp.retlen = ops_tmp.oobretlen = 0;
			
			if (!nand_tl_check_empty_page(&ops_tmp))
			{
				ret = mtd_write_oob(info_tl->mtd_dev, to, &ops_tmp);
				
				if (ret || ops_tmp.retlen != size || ops_tmp.oobretlen != oobsize)
				{
					nand_tl_bbt_mark_block((unsigned char *)(unsigned int)to);
					if ((ret = nand_tl_bbt_get_spareblock(&spare_addr)))
						goto bbt_err_out;
					if ((ret = nand_tl_bbt_add_translation((unsigned char *)(unsigned int)(to & ~(info_tl->mtd_tl->erasesize - 1)), spare_addr)))
						goto bbt_err_out;
					if (!isbbt)
					{
						NAND_CODE_COV(NAND_CODE_COV_ALIGNED_WRITE_BLOCK_BAD);
						/* only copy the block if it's not a bbt block */
						ops_tmp.datbuf = &ops->datbuf[index];
						ops_tmp.len = len;
						ops_tmp.oobbuf = &ops->oobbuf[oobindex];
						ops_tmp.ooblen = ooblen;
						ops_tmp.retlen = ops_tmp.oobretlen = 0;
						if ((ret = nand_tl_copy_block(to, &ops_tmp, (loff_t)(unsigned int)spare_addr)))
							goto bbt_err_out;
						ops->retlen += ops_tmp.retlen;
						ops->oobretlen += ops_tmp.oobretlen;
						if ((ret = nand_tl_bbt_save()))
							goto bbt_err_out;
						return 0;
					}
					else
					{
						/* restart saving bbt because it changed due to new translation */
						NAND_DEBUG("Saving bbt over again");
						if (!info_tl->bbt_valid)
							return -EPERM;
						
						nand_tl_bbt_check_crc(1, info_tl->tbbt);
						to = (loff_t)(unsigned int)spare_addr;
						len = ops->len;
						ooblen = ops->ooblen;
						size = info_tl->mtd_tl->writesize;
						oobsize = info_tl->mtd_tl->ecclayout->oobavail;
						index = 0;
						oobindex = 0;
						ops->retlen = ops->oobretlen = 0;

						if(size > len)
							size = len;
						if(oobsize > ooblen)
							oobsize = ooblen;
							
						NAND_CODE_COV(NAND_CODE_COV_BBT_SAVE_RECALL);
						continue;
					}
				}
				ops->retlen += ops_tmp.retlen;
				ops->oobretlen += ops_tmp.oobretlen;

			}
			else
			{
				ops->retlen += size;
				ops->oobretlen += oobsize;
			}
	
			to += info_tl->mtd_tl->writesize;
			index += size;
			oobindex += oobsize;
			len -= size;
			ooblen -= oobsize;
			if(len > info_tl->mtd_tl->writesize)
				size = info_tl->mtd_tl->writesize;
			else
				size = len;
			if(ooblen > info_tl->mtd_tl->ecclayout->oobavail)
				oobsize = info_tl->mtd_tl->ecclayout->oobavail;
			else
				oobsize = ooblen;
		} while(len || ooblen);
	}

	return 0;
bbt_err_out:
	NAND_ERROR("Nand flash: error writing. Reloading BBT");
	NAND_CODE_COV(NAND_CODE_COV_BLOCKLIMITED_WRITE_FAILS);
	nand_tl_bbt_load(0,0);
	return -EIO;
}

static int nand_tl_copy_block(loff_t src, struct mtd_oob_ops *ops, loff_t dest)
{
	int ret = 0;
	unsigned char *block_data = NULL;
	unsigned char *oob_data = NULL;
	struct mtd_oob_ops ops_tmp;
	const size_t oob_block_len = TO_OOB_SIZE(info_tl->mtd_tl->erasesize);
	const unsigned int src_offset = src & (info_tl->mtd_tl->erasesize - 1);
	const unsigned int src_block = src & ~(info_tl->mtd_tl->erasesize - 1);

	NAND_DEBUG("copy block from 0x%08x to 0x%08x", (unsigned)src, (unsigned)dest);
	NAND_CODE_COV(NAND_CODE_COV_COPY_BLOCK);

	dest = dest & ~(info_tl->mtd_tl->erasesize - 1);

	block_data = kmalloc(info_tl->mtd_tl->erasesize, GFP_KERNEL);
	if (!block_data)
	{
		ret = -ENOMEM;
		goto err_out;
	}

	oob_data = kmalloc(oob_block_len, GFP_KERNEL);
	if (!oob_data)
	{
		ret = -ENOMEM;
		goto err_out;
	}

	ops_tmp.mode = MTD_OPS_AUTO_OOB;
	ops_tmp.ooblen = oob_block_len;
	ops_tmp.oobbuf = oob_data;
	ops_tmp.len = info_tl->mtd_tl->erasesize;
	ops_tmp.datbuf = block_data;
	ops_tmp.ooboffs = ops_tmp.retlen = ops_tmp.oobretlen = 0;
	
	mtd_read_oob(info_tl->mtd_dev, src_block, &ops_tmp);

	if (ops_tmp.retlen != info_tl->mtd_tl->erasesize || ops_tmp.oobretlen != oob_block_len)
	{
		if (!ret)
			ret = -EIO;
		goto err_out;
	}

	memcpy((void *)&block_data[src_offset], (void *)ops->datbuf, ops->len);
	memcpy((void *)&oob_data[TO_OOB_ADDR(src_offset)], (void *)ops->oobbuf, ops->ooblen);

	ops_tmp.mode = MTD_OPS_AUTO_OOB;
	ops_tmp.ooblen = oob_block_len;
	ops_tmp.oobbuf = oob_data;
	ops_tmp.len = info_tl->mtd_tl->erasesize;
	ops_tmp.ooboffs = ops_tmp.retlen = ops_tmp.oobretlen = 0;
	ops_tmp.datbuf = block_data;
	
	ret = nand_tl_write_blocklimited(dest, &ops_tmp, 0);
	
	if (ops_tmp.retlen != info_tl->mtd_tl->erasesize || ops_tmp.oobretlen != oob_block_len || ret)
	{
		if (!ret)
			ret = -EIO;
		goto err_out;
	}
	
	ops->retlen += ops->len;
	ops->oobretlen += ops->ooblen;
	
	kfree(block_data);
	kfree(oob_data);
	return 0;

err_out:
	NAND_ERROR("Nand flash copy block error %d", ret);
	if (block_data)
		kfree(block_data);
	if (oob_data)
		kfree(oob_data);
	return ret;
}

static int nand_tl_check_empty_page(struct mtd_oob_ops *ops)
{
	int i;
	char *buf;

	for (i = 0, buf = (char *)(ops->datbuf); i < ops->len; i++, buf++)
	{
		if (*buf - (char)0xFF)
		{
			return 0;
		}
	}
	for (i = 0, buf = (char *)(ops->oobbuf); i < ops->ooblen; i++, buf++)
	{
		if (*buf - (char)0xFF)
		{
			return 0;
		}
	}
	NAND_CODE_COV(NAND_CODE_COV_PAGE_EMPTY);
	return 1;
}

static int nand_tl_write_oob(struct mtd_info *mtd, loff_t to, struct mtd_oob_ops *ops)
{
	int oobavail = info_tl->mtd_dev->ecclayout->oobavail;
	struct mtd_oob_ops ops_tmp;
	loff_t block_addr, block_offset;
	int ret = 0, total_ret = 0;
	size_t len = ops->ooblen;
#ifdef NAND_TRACE_DEBUG
	int print_tl = 0;
#endif

	if (ops->mode != MTD_OPS_AUTO_OOB || ops->ooboffs != 0 || ops->datbuf != NULL)
	{
		NAND_ERROR("Nand flash TL: unsupported write OOB request.");
		return -EINVAL;
	}

	if (!info_tl->bbt_valid)
		return -EIO;

	nand_tl_get_device(FL_WRITING);
	ops->oobretlen = 0;
	memcpy(&ops_tmp, ops, sizeof(struct mtd_oob_ops));

	{
		to &= ~(info_tl->mtd_tl->writesize - 1);
		block_addr = to & ~(info_tl->mtd_tl->erasesize - 1);
		block_offset = info_tl->mtd_tl->erasesize - (to - block_addr);
		ops_tmp.ooblen = ((unsigned int)block_offset / info_tl->mtd_tl->writesize) * oobavail;

		if(ops_tmp.ooblen > len)
			ops_tmp.ooblen = len;
		
		do
		{
			ret = nand_tl_write_blocklimited(nand_tl_trans_addr(to), &ops_tmp, 0);
			ops->oobretlen += ops_tmp.oobretlen;

			total_ret = ret;
			if (total_ret)
			{
				NAND_DEBUG("Writing to oob failed. Aborting\r");
				break;
			}
			
			if (ops_tmp.oobretlen != ops_tmp.ooblen)
			{
				NAND_DEBUG("got unexpected length (got %zd, expected %zd). aborting", ops_tmp.oobretlen, ops_tmp.ooblen);
				break;
			}
			
			len -= ops_tmp.oobretlen;
#ifdef NAND_TRACE_DEBUG
			if (len || print_tl)
			{
				NAND_DEBUG("[%d] splitted write_OOB in block parts: logic addr %llx, len %zd, %d bytes per page", print_tl, to, ops_tmp.ooblen, oobavail);
				print_tl++;
				NAND_CODE_COV(NAND_CODE_COV_WRITE_OOB_PARTS);
			}
#endif
			to += block_offset;
			block_offset = info_tl->mtd_tl->erasesize;
			ops_tmp.oobbuf += ops_tmp.oobretlen;
			if(len > (info_tl->pages_per_block * oobavail))
				ops_tmp.ooblen = (info_tl->pages_per_block * oobavail);
			else
				ops_tmp.ooblen = len;
		} while (len);
	}
#ifdef NAND_TRACE_DEBUG
	if (total_ret)
		NAND_DEBUG("warning: returning %d", total_ret);
#endif
	nand_tl_release_device();
	return total_ret;
}

static void nand_tl_sync(struct mtd_info *mtd)
{
	nand_tl_get_device(FL_SYNCING);
	nand_tl_release_device();
	mtd_sync(info_tl->mtd_dev);
	return;
}

static int nand_tl_suspend(struct mtd_info *mtd)
{
	return mtd_suspend(info_tl->mtd_dev);
}

static void nand_tl_resume(struct mtd_info *mtd)
{
	mtd_resume(info_tl->mtd_dev);
}

static int nand_tl_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	NAND_ERROR("Nand flash: unexpected bad block marking!");
	return -EINVAL;
}

/* **************************************************************************** */
/* **************************************************************************** */

/*                                     BBT                                      */

/* **************************************************************************** */
/* **************************************************************************** */

static int nand_tl_bbt_print(void)
{
	int i;
	
	NAND_TRACE(" ---------------------------------------------------------------------------------");
	NAND_TRACE("|                                                                                 |");
	NAND_TRACE("|                           Technicolor NAND flash BBT                            |");
	NAND_TRACE("|                                                                                 |");
	NAND_TRACE(" ---------------------------------------------------------------------------------");
	NAND_TRACE("|                                                                                 |");
	NAND_TRACE("| version | bbt address | bbt mirror address | logic flash end | phys spare start |");
	NAND_TRACE(" ---------------------------------------------------------------------------------");
	NAND_TRACE("| %7d | 0x%08X  | 0x%08X         | 0x%08X      | 0x%08X       |", info_tl->tbbt->version, 
	                                                                             (unsigned int)info_tl->tbbt->logic_bbt_addr, 
	                                                                             (unsigned int)info_tl->tbbt->logic_bbt_mirror_addr, 
	                                                                             (unsigned int)info_tl->tbbt->logic_flash_end, 
	                                                                             (unsigned int)info_tl->tbbt->phys_spare_start);
	NAND_TRACE(" ---------------------------------------------------------------------------------");
	NAND_TRACE("|                                                                                 |");
	NAND_TRACE("| max_badblock_count | block count | spareblock count                             |");  
	NAND_TRACE(" ---------------------------------------------------------------------------------");
	NAND_TRACE("| %18ld | %11ld | %16ld                             |", info_tl->tbbt->max_badblock_count, 
	                                                                  info_tl->tbbt->block_count, 
	                                                                  info_tl->tbbt->spareblock_count);
	NAND_TRACE(" ---------------------------------------------------------------------------------");
	NAND_TRACE("|                                                                                 |");
	NAND_TRACE("|                                   block info                                    |");
	NAND_TRACE("|                                                                                 |");
	NAND_TRACE(" ---------------------------------------------------------------------------------");
	NAND_TRACE("| phys addr  | is bad | is reassigned | phys translated addr                      |");
	NAND_TRACE(" ---------------------------------------------------------------------------------");
	NAND_TRACE("|                                                                                 |");
	for (i = 0; i < info_tl->tbbt->block_count; i++)
	{
		NAND_TRACE("| 0x%08X | %6d | %13d | 0x%08X                                |", (unsigned int)info_tl->tbbt->block_info[i].phys_addr, 
		                                                                            info_tl->tbbt->block_info[i].isbad, 
		                                                                            info_tl->tbbt->block_info[i].isreassigned, 
		                                                                            (unsigned int)info_tl->tbbt->block_info[i].phys_translated_addr);
	}
	NAND_TRACE("|                                                                                 |");
	NAND_TRACE(" ---------------------------------------------------------------------------------");
	NAND_TRACE("|                                                                                 |");
	NAND_TRACE("|                                   spare info                                    |");
	NAND_TRACE("|                                                                                 |");
	NAND_TRACE(" ---------------------------------------------------------------------------------");
	NAND_TRACE("| phys addr  | is bad | is used                                                   |");
	NAND_TRACE(" ---------------------------------------------------------------------------------");
	NAND_TRACE("|                                                                                 |");
	for (i = 0; i < info_tl->tbbt->spareblock_count; i++)
	{
		NAND_TRACE("| 0x%08X | %6d | %7d                                                   |", (unsigned int)info_tl->tbbt->spareblock_info[i].phys_addr,
		                                                                                     info_tl->tbbt->spareblock_info[i].isbad,
		                                                                                     info_tl->tbbt->spareblock_info[i].isused);
	}
	NAND_TRACE("|                                                                                 |");
	NAND_TRACE(" ---------------------------------------------------------------------------------");
	return 0;
}

static int nand_tl_bbt_check_crc(int unload, t_tbbt *tbbt)
{
	unsigned long crc32_old = tbbt->crc32;
	size_t bbt_size = TBBT_SIZE + (tbbt->max_badblock_count * (TBBT_BLOCK_SIZE + TBBT_SPAREBLOCK_SIZE));
	tbbt->crc32 = ~0;
	tbbt->crc32 = crc32_be(~0, (char *)tbbt, bbt_size);
	if ((tbbt->crc32 != crc32_old) && unload)
	{
		NAND_ERROR("tBBT corruption! Danger! Unregistering MTD device.");
		info_tl->bbt_valid = 0;
		nand_tl_exit();
	}
	return (tbbt->crc32 == crc32_old);
}

static void nand_tl_bbt_update_crc(t_tbbt *tbbt)
{
	size_t bbt_size = TBBT_SIZE + (tbbt->max_badblock_count * (TBBT_BLOCK_SIZE + TBBT_SPAREBLOCK_SIZE));
	tbbt->crc32 = ~0;
	tbbt->crc32 = crc32_be(~0, (char *)tbbt, bbt_size);
}

static int nand_tl_bbt_load(loff_t phys_addr, t_tbbt **tbbt_load_addr) /* the load addr is only used for test purposes */
{
	t_tbbt bbt_tmp;
	int ret, retlen;
	loff_t ofs, bbt_addr;
	loff_t phys_addr1, phys_addr2;
	unsigned char version;
	size_t bbt_size;
	int i;
	int prev_errors;

	int active_bbt_has_ecc_errors = 0;
	int mirror_bbt_has_ecc_errors = 0;
	
	bbt_addr = -1;
	ret = 0;
	version = 0;
	
	if (!phys_addr)
	{
		if (!info_tl->bbt_valid)
		{
			NAND_ERROR("Nand flash TL: Cant determine TBBT position!");
			goto err_out;
		}
		else
		{
			phys_addr1 = nand_tl_trans_addr((unsigned int)info_tl->tbbt->logic_bbt_addr);
			phys_addr2 = nand_tl_trans_addr((unsigned int)info_tl->tbbt->logic_bbt_mirror_addr);
			NAND_CODE_COV(NAND_CODE_COV_LOAD_TBBT_NOADDR);
		}
	}
	else
	{
		phys_addr1 = phys_addr;
		phys_addr2 = 0;
		NAND_CODE_COV(NAND_CODE_COV_LOAD_TBBT_ADDR);
	}
	
	if (!tbbt_load_addr)
	{
		NAND_CODE_COV(NAND_CODE_COV_LOAD_TBBT_NOLOADADDR);
		tbbt_load_addr = &info_tl->tbbt;
		info_tl->bbt_valid = 0;
		if (info_tl->tbbt)
			kfree(info_tl->tbbt);
	}
	else
	{
		NAND_CODE_COV(NAND_CODE_COV_LOAD_TBBT_LOADADDR);
	}
	
	for (i=0; i < 2; i++)
	{
		if (i==0)
			ofs = phys_addr1;
		else
			ofs = phys_addr2;
		
		if (!ofs)
			break;
		
		retlen = 0;
		
		prev_errors = info_tl->mtd_dev->ecc_stats.corrected;
		if (!phys_addr)
			ret = nand_tl_read_nolock(info_tl->mtd_tl, ofs, TBBT_SIZE, &retlen, (uint8_t *)&bbt_tmp, 0);
		else
			ret = mtd_read(info_tl->mtd_tl, ofs, TBBT_SIZE, &retlen, (uint8_t *)&bbt_tmp);

		if (ret || retlen < TBBT_SIZE)
			goto err_out;
		
		if (strncmp(bbt_tmp.magic, tbbt_magic, 4))
			continue;
			
		bbt_size = TBBT_SIZE + (bbt_tmp.max_badblock_count * (TBBT_BLOCK_SIZE + TBBT_SPAREBLOCK_SIZE));
		if (bbt_size > info_tl->mtd_tl->erasesize)
			continue;
		
		*tbbt_load_addr = kmalloc(bbt_size, GFP_KERNEL);

		if (!(*tbbt_load_addr))
		{
			ret = -ENOMEM;
			goto err_out;
		}
		retlen = 0;
		
		if (!phys_addr)
			ret = nand_tl_read_nolock(info_tl->mtd_tl, ofs, bbt_size, &retlen, (uint8_t *)(*tbbt_load_addr), 0);
		else
			ret = mtd_read(info_tl->mtd_tl, ofs, bbt_size, &retlen, (uint8_t *)(*tbbt_load_addr));
		
		if (!nand_tl_bbt_check_crc(0, *tbbt_load_addr))
		{
			kfree(*tbbt_load_addr);
			continue;
		}

		kfree(*tbbt_load_addr);
			
		*tbbt_load_addr = NULL;
		
		if (bbt_addr == -1)
		{
			bbt_addr = ofs;
			version = bbt_tmp.version;
			NAND_DEBUG("bbt with version %d found at %llx", bbt_tmp.version, bbt_addr);
			active_bbt_has_ecc_errors = (prev_errors != info_tl->mtd_dev->ecc_stats.corrected);
		}
		else
		{
			if (((bbt_tmp.version == 1) && (version == 0)) ||
			    ((bbt_tmp.version == 2) && (version == 1)) ||
			    ((bbt_tmp.version == 0) && (version == 2)))
			{
				bbt_addr = ofs;
				NAND_DEBUG("newer bbt with version %d found at %llx", bbt_tmp.version, bbt_addr);
				mirror_bbt_has_ecc_errors = active_bbt_has_ecc_errors;
				active_bbt_has_ecc_errors = (prev_errors != info_tl->mtd_dev->ecc_stats.corrected);
			}
			else
			{
				mirror_bbt_has_ecc_errors = (prev_errors != info_tl->mtd_dev->ecc_stats.corrected);
			}
		}
	}
	
	if (bbt_addr != -1)
	{
		retlen = 0;
		
		if (!phys_addr)
			ret = nand_tl_read_nolock(info_tl->mtd_tl, bbt_addr, TBBT_SIZE, &retlen, (uint8_t *)&bbt_tmp, 0);
		else
			ret = mtd_read(info_tl->mtd_tl, bbt_addr, TBBT_SIZE, &retlen, (uint8_t *)&bbt_tmp);
			
		bbt_size = TBBT_SIZE + (bbt_tmp.max_badblock_count * (TBBT_BLOCK_SIZE + TBBT_SPAREBLOCK_SIZE));
		
		*tbbt_load_addr = kmalloc(bbt_size, GFP_KERNEL);
		if (!(*tbbt_load_addr))
		{
			ret = -ENOMEM;
			goto err_out;
		}
		
		retlen = 0;
		if (!phys_addr)
			ret = nand_tl_read_nolock(info_tl->mtd_tl, bbt_addr, bbt_size, &retlen, (uint8_t *)(*tbbt_load_addr), 0);
		else
			ret = mtd_read(info_tl->mtd_tl, bbt_addr, bbt_size, &retlen, (uint8_t *)(*tbbt_load_addr));
			
		if (ret || retlen < bbt_size)
		{
			goto err_out;
		}
		
		(*tbbt_load_addr)->block_info = (t_tbbt_blockinfo *)((unsigned int)&(*tbbt_load_addr)->spareblock_info + sizeof((*tbbt_load_addr)->spareblock_info));
		(*tbbt_load_addr)->spareblock_info  = (t_tbbt_spareblockinfo *)((unsigned int)(*tbbt_load_addr)->block_info + ((*tbbt_load_addr)->max_badblock_count * TBBT_BLOCK_SIZE));
		nand_tl_bbt_update_crc(*tbbt_load_addr);
		
		if (*tbbt_load_addr == info_tl->tbbt)
		{
			info_tl->bbt_valid = 1;
			NAND_TRACE("tBBT loaded");
#ifdef NAND_TRACE_DEBUG
			nand_tl_bbt_print();
#endif
			if (mirror_bbt_has_ecc_errors && !active_bbt_has_ecc_errors)
			{
				NAND_CODE_COV(NAND_CODE_COV_RECC_IN_MBBT);
				NAND_TRACE("RECC in mBBT: save BBT once");
				nand_tl_bbt_save();
			}
			if (active_bbt_has_ecc_errors)
			{
				NAND_CODE_COV(NAND_CODE_COV_RECC_IN_ABBT);
				NAND_TRACE("nand: RECC in aBBT: save BBT twice");
				nand_tl_bbt_save();
				nand_tl_bbt_save();
			}
		}
	}
	else
	{
		NAND_ERROR("Nand flash TL: no BBT found");
		if (*tbbt_load_addr)
			kfree(*tbbt_load_addr);
		ret = -ENOENT;
	}
	return ret;
	
err_out:
	NAND_ERROR("Nand flash TL: Error scanning for BBT");
	if (*tbbt_load_addr)
		kfree(*tbbt_load_addr);
	return ret;
}

static int nand_tl_bbt_check_copyblocks(void)
{
	int i, ret;
	struct erase_info instr;
	struct mtd_oob_ops ops;
	
	for (i = 0; i < info_tl->tbbt->block_count; i++)
	{
		if ((info_tl->tbbt->block_info[i].isbad == 0) && (info_tl->tbbt->block_info[i].isreassigned == 1))
		{
			NAND_DEBUG("found copyblock");
			
			instr.mtd = info_tl->mtd_dev;
			instr.addr = (unsigned int)info_tl->tbbt->block_info[i].phys_addr;
			instr.len = info_tl->mtd_tl->erasesize;
			instr.callback = NULL;
			instr.fail_addr = ~0;
			
			ret = mtd_erase(info_tl->mtd_dev, &instr);
			
			if (ret == -EIO || instr.state != MTD_ERASE_DONE)
			{
				NAND_CODE_COV(NAND_CODE_COV_COPYBLOCK_ERASE_FAIL);
				/* ASSUMES BLOCK IS ALREADY MARKED BAD IN OOB BY DRIVER */
				if ((ret = nand_tl_bbt_mark_block(info_tl->tbbt->block_info[i].phys_addr)))
				{
					goto err_out;
				}
			}
			else
			{
				NAND_CODE_COV(NAND_CODE_COV_COPYBLOCK);
				ops.mode = MTD_OPS_AUTO_OOB;
				ops.len = ops.ooblen = ops.ooboffs = ops.retlen = ops.oobretlen = 0;
				ops.datbuf = ops.oobbuf = NULL;
				if ((ret = nand_tl_copy_block((loff_t)(unsigned int)info_tl->tbbt->block_info[i].phys_translated_addr, &ops, (loff_t)(unsigned int)info_tl->tbbt->block_info[i].phys_addr)))
					goto err_out;
				if ((ret = nand_tl_bbt_del_block(info_tl->tbbt->block_info[i].phys_addr)))
					goto err_out;
			}
			if ((ret = nand_tl_bbt_save()))
				goto err_out;
			NAND_DEBUG("copyblock removed");
		}
	}
	return 0;
err_out:
	NAND_ERROR("Nand flash copyblock recovery error");
	return ret;
}

static int nand_tl_bbt_get_spareblock(unsigned char **addr)
{
	int i, ret = -ENOENT;
	struct erase_info instr;
	
	if (!info_tl->bbt_valid)
		return -EINVAL;

	for (i = 0; i < info_tl->tbbt->spareblock_count; i++)
	{
		if ((info_tl->tbbt->spareblock_info[i].isbad == 0) && (info_tl->tbbt->spareblock_info[i].isused == 0))
		{
			info_tl->tbbt->spareblock_info[i].isused = 1;
			nand_tl_bbt_update_crc(info_tl->tbbt);
			*addr = info_tl->tbbt->spareblock_info[i].phys_addr;
			NAND_DEBUG("getting spare block at addr %x", (unsigned int)*addr);

			instr.mtd = info_tl->mtd_dev;
			instr.addr = (unsigned int)*addr;
			instr.len = info_tl->mtd_tl->erasesize;
			instr.callback = NULL;
			instr.fail_addr = ~0;
				
			ret = mtd_erase(info_tl->mtd_dev, &instr);
				
			if (ret == -EIO || instr.state != MTD_ERASE_DONE)
			{
				NAND_CODE_COV(NAND_CODE_COV_COPYBLOCK_SPARE_ERASE_FAIL);
				/* ASSUMES BLOCK IS ALREADY MARKED BAD IN OOB BY DRIVER */
				if ((ret = nand_tl_bbt_mark_block(*addr)))
				{
					goto err_out;
				}
				return nand_tl_bbt_get_spareblock(addr);
			}
			else if (ret)
				goto err_out;
			return 0;
		}
	}
	
err_out:
	NAND_ERROR("Nand flash: Unexpected error: %d", ret);
	return ret;;
}

static int nand_tl_bbt_add_translation(unsigned char *addr, unsigned char *spare_addr)
{
	int i, pos, ret = 0;
	
	addr = (unsigned char *)((unsigned int)addr & ~(info_tl->mtd_tl->erasesize - 1));
	spare_addr = (unsigned char *)((unsigned int)spare_addr & ~(info_tl->mtd_tl->erasesize - 1));

	NAND_DEBUG("Adding translation from %x to %x", (unsigned int)addr, (unsigned int)spare_addr);
	
	if (addr >= info_tl->tbbt->phys_spare_start)
	{
		ret = -ENOENT;
		for (i = 0; i < info_tl->tbbt->block_count; i++)
		{
			if (info_tl->tbbt->block_info[i].phys_translated_addr == addr)
			{
				NAND_DEBUG("Translation was already present, changing %x to %x", (unsigned int)addr, (unsigned int)info_tl->tbbt->block_info[i].phys_addr);
				addr = info_tl->tbbt->block_info[i].phys_addr;
				ret = 0;
				break;
			}
		}
		if (ret)
		{
			NAND_DEBUG("Cant find original block address for spare %x", (unsigned int)addr);
			return ret;
		}
	}

	if ((ret = nand_tl_bbt_add_block(addr, &pos)))
	{
		return ret;
	}

	NAND_DEBUG("Check is reassigned: %x", info_tl->tbbt->block_info[pos].isreassigned);
	if (info_tl->tbbt->block_info[pos].isreassigned)
	{
		NAND_DEBUG("Free %x", (unsigned int)info_tl->tbbt->block_info[pos].phys_translated_addr);
		nand_tl_bbt_free_spareblock(info_tl->tbbt->block_info[pos].phys_translated_addr);
		NAND_DEBUG("ok");
	}
	info_tl->tbbt->block_info[pos].isreassigned = 1;
	info_tl->tbbt->block_info[pos].phys_translated_addr = spare_addr;
	nand_tl_bbt_update_crc(info_tl->tbbt);
	NAND_DEBUG("done");
	return ret;
}

static int nand_tl_bbt_add_block(unsigned char *addr, int *pos)
{
	int i;

	addr = (unsigned char *)((unsigned int)addr & ~(info_tl->mtd_tl->erasesize - 1));
	
	for (i = 0; i < info_tl->tbbt->block_count; i++)
	{
		if (info_tl->tbbt->block_info[i].phys_addr == addr)
		{
			*pos = i;
			return 0;
		}
	}
	
	if (info_tl->tbbt->block_count > 0)
	{
		i = info_tl->tbbt->block_count;
		while (info_tl->tbbt->block_info[i-1].phys_addr > addr)
		{
			info_tl->tbbt->block_info[i] = info_tl->tbbt->block_info[i-1];
			i--;
		}
	}
	else 
		i = 0;
	
	info_tl->tbbt->block_info[i].phys_addr = addr;
	info_tl->tbbt->block_info[i].isbad = 0;
	info_tl->tbbt->block_info[i].isreassigned = 0;
	info_tl->tbbt->block_info[i].phys_translated_addr = (unsigned char *)-1;
	info_tl->tbbt->block_count++;
	nand_tl_bbt_update_crc(info_tl->tbbt);

	NAND_DEBUG("added block %x in bbt", (unsigned int)addr);
	*pos = i;
	return 0;
}

static int nand_tl_bbt_del_block(unsigned char *addr)
{
	int j, pos, ret = 0;
	
	if ((ret = nand_tl_bbt_add_block(addr, &pos)))
	{
		return ret;
	}

	if (info_tl->tbbt->block_info[pos].isbad == 1)
	{
		NAND_DEBUG("block %x is bad. cant remore from bbt", (unsigned int)addr);
		ret = -EINVAL;
		return ret;
	}
	
	nand_tl_bbt_free_spareblock(info_tl->tbbt->block_info[pos].phys_translated_addr);

	info_tl->tbbt->block_count--;
	
	for (j = pos;  j < info_tl->tbbt->block_count; j++)
	{
		info_tl->tbbt->block_info[j] = info_tl->tbbt->block_info[j+1];
	}
	nand_tl_bbt_update_crc(info_tl->tbbt);

	NAND_DEBUG("block %x is removed from bbt", (unsigned int)addr);
	return ret;
}

static int nand_tl_bbt_save(void)
{
	loff_t addr;
	struct erase_info instr;
	int bbt_size, ret, retlen;
	unsigned long crc32;
	
	if (!info_tl->bbt_valid)
		return -EPERM;
		
	nand_tl_bbt_check_crc(1, info_tl->tbbt);

	NAND_DEBUG("starting saving bbt");
	crc32 = info_tl->tbbt->crc32;

	instr.mtd = info_tl->mtd_tl;
	instr.addr = (loff_t)(unsigned int)info_tl->tbbt->logic_bbt_mirror_addr;
	instr.len = info_tl->mtd_tl->erasesize;
	instr.callback = NULL;
	if ((ret = nand_tl_erase_bbt(&instr, 1)))
		goto err_out;
		
	if (crc32 != info_tl->tbbt->crc32)
	{
		NAND_CODE_COV(NAND_CODE_COV_SAVE_TBBT_PRELIM_DONE);
		NAND_DEBUG("no need to continue saving bbt");
		return 0;
	}

	addr = (unsigned int)info_tl->tbbt->logic_bbt_mirror_addr;
	info_tl->tbbt->logic_bbt_mirror_addr = info_tl->tbbt->logic_bbt_addr;
	info_tl->tbbt->logic_bbt_addr = (unsigned char *)(unsigned int)addr;

	info_tl->tbbt->version++;
	if (info_tl->tbbt->version > 2)
		info_tl->tbbt->version = 0;

	nand_tl_bbt_update_crc(info_tl->tbbt);
	
	bbt_size = TBBT_SIZE + (info_tl->tbbt->max_badblock_count * (TBBT_BLOCK_SIZE + TBBT_SPAREBLOCK_SIZE));
	bbt_size += info_tl->mtd_tl->writesize - (bbt_size % info_tl->mtd_tl->writesize);

	NAND_DEBUG("saving bbt version %d at %x", info_tl->tbbt->version, (unsigned int)addr);
	retlen = 0;
	ret = nand_tl_write_bbt(addr, bbt_size, &retlen, (uint8_t *)info_tl->tbbt, 1);
	
	if (ret || retlen != bbt_size)
		goto err_out;
	
	NAND_DEBUG("done saving bbt");
	return 0;
err_out:
	NAND_ERROR("Unable to save BBT: %d", ret);
	return ret;
}

static int nand_tl_bbt_mark_block(unsigned char *addr)
{
	int pos, ret = -ENOENT;

#ifdef CONFIG_MTD_NAND_TL_TEST
	amount_of_bad_blocks++;
#endif
	if (addr < info_tl->tbbt->phys_spare_start)
	{
		if ((ret = nand_tl_bbt_add_block(addr, &pos)))
			goto err_out;

		NAND_DEBUG("marking block at %x as bad in bbt", (unsigned int)addr);
		info_tl->tbbt->block_info[pos].isbad = 1;
		nand_tl_bbt_update_crc(info_tl->tbbt);
		return 0;
	}
	else
	{
		if ((ret = nand_tl_bbt_add_spareblock(addr, &pos)))
			goto err_out;
	
		NAND_DEBUG("marking spare block at %x as bad in bbt", (unsigned int)addr);
		info_tl->tbbt->spareblock_info[pos].isbad = 1;
		info_tl->tbbt->spareblock_info[pos].isused = 0;
		nand_tl_bbt_update_crc(info_tl->tbbt);
		return 0;
	}

err_out:
	NAND_ERROR("Nand flash: block not found: %d", ret);
	return ret;
}

static int nand_tl_bbt_add_spareblock(unsigned char *addr, int *pos)
{
	int i, ret;

	addr = (unsigned char *)((unsigned int)addr & ~(info_tl->mtd_tl->erasesize - 1));
	
	for (i = 0; i < info_tl->tbbt->spareblock_count; i++)
	{
		if (info_tl->tbbt->spareblock_info[i].phys_addr == addr)
		{
			*pos = i;
			return 0;
		}
	}
	
	if (info_tl->tbbt->spareblock_count == info_tl->tbbt->max_badblock_count)
	{
		ret = -ENOSPC;
		goto err_out;
	}
	info_tl->tbbt->spareblock_info[info_tl->tbbt->spareblock_count].phys_addr = addr;
	info_tl->tbbt->spareblock_info[info_tl->tbbt->spareblock_count].isused = 0;
	info_tl->tbbt->spareblock_info[info_tl->tbbt->spareblock_count].isbad = 0;
	*pos = info_tl->tbbt->spareblock_count;
	info_tl->tbbt->spareblock_count++;
	nand_tl_bbt_update_crc(info_tl->tbbt);

	NAND_DEBUG("added spare block %x in bbt", (unsigned int)addr);
	return 0;
err_out:
	NAND_ERROR("Nand flash: can't add spare block: %d", ret);
	return ret;
}

static int nand_tl_bbt_free_spareblock(unsigned char *addr)
{
	int pos, ret;
	t_tbbt_spareblockinfo tmp;
	
	if ((ret = nand_tl_bbt_add_spareblock(addr, &pos)))
		return ret;
		
	info_tl->tbbt->spareblock_info[pos].isused = 0;
	NAND_DEBUG("freeing spare block %x", (unsigned int)addr);

	tmp = info_tl->tbbt->spareblock_info[info_tl->tbbt->spareblock_count - 1];
	for (pos = info_tl->tbbt->spareblock_count - 1; pos > 0; pos--)
	{
		info_tl->tbbt->spareblock_info[pos] = info_tl->tbbt->spareblock_info[pos-1];
	}
	info_tl->tbbt->spareblock_info[0] = tmp;
	nand_tl_bbt_update_crc(info_tl->tbbt);

	return 0;
}

/* **************************************************************************** */
/* **************************************************************************** */

/*                                 END BBT                                      */

/* **************************************************************************** */
/* **************************************************************************** */

static loff_t tbbt_addr;

static int __init tbbt_addr_init(char *str)
{
	int tmp;
	get_option(&str, &tmp);
	tbbt_addr = tmp;
	return 1;
}

__setup("tbbt_addr=", tbbt_addr_init);

static int nand_tl_get_device(int new_state)
{
	spinlock_t *lock = &info_tl->lock;
	wait_queue_head_t *wq = &info_tl->wq;
	DECLARE_WAITQUEUE(wait, current);
 retry:
	spin_lock(lock);

	if (info_tl->state == FL_READY) {
		info_tl->state = new_state;
		spin_unlock(lock);
		return 0;
	}
	
	set_current_state(TASK_UNINTERRUPTIBLE);
	add_wait_queue(wq, &wait);
	spin_unlock(lock);
	schedule();
	remove_wait_queue(wq, &wait);
	goto retry;
}

static void nand_tl_release_device(void)
{
	spin_lock(&info_tl->lock);
	info_tl->state = FL_READY;
	wake_up(&info_tl->wq);
	spin_unlock(&info_tl->lock);
}

static struct mtd_info *nand_tl_probe(struct map_info *map)
{
	struct brcmnand_chip *chip;
	struct proc_dir_entry *proc_entry;

	int err = 0;
	
	if (tbbt_addr == 0) {
		NAND_ERROR("ERROR: uninitialised 'tbbt address'");
		goto err_out;
	}

	info_tl = kmalloc(sizeof(struct nand_tl_info), GFP_KERNEL);
	if (!info_tl)
	{
		err = -ENOMEM;
		goto err_out;
	}
	memset(info_tl, 0, sizeof(struct nand_tl_info));

	info_tl->mtd_tl = kmalloc(sizeof(struct mtd_info), GFP_KERNEL);
	if (!info_tl->mtd_tl)
	{
		err = -ENOMEM;
		goto err_out;
	}
	memset(info_tl->mtd_tl, 0, sizeof(struct mtd_info));

	info_tl->mtd_dev = get_mtd_device_nm("brcmnand.0");
	if (!info_tl->mtd_dev || PTR_ERR(info_tl->mtd_dev) == -ENODEV)
	{
		{
			NAND_ERROR("No nand flash mtd layer found.");
			err = -ENXIO;
			goto err_out;
		}
	}

	chip = info_tl->mtd_dev->priv;
	chip->isbad_bbt = nand_tl_isbad_bbt;

	memcpy(info_tl->mtd_tl, info_tl->mtd_dev, sizeof(struct mtd_info));

	info_tl->mtd_tl->name = "technicolor-nand-tl";
	info_tl->mtd_tl->priv = info_tl;
	info_tl->mtd_tl->owner = THIS_MODULE;
	info_tl->mtd_tl->_erase = nand_tl_erase;
	info_tl->mtd_tl->_read = nand_tl_read;
	info_tl->mtd_tl->_write = nand_tl_write;
	info_tl->mtd_tl->_read_oob = nand_tl_read_oob;
	info_tl->mtd_tl->_write_oob = nand_tl_write_oob;
	info_tl->mtd_tl->_writev = NULL; /* also not implemented in nand_base.c. For further improvement */
	info_tl->mtd_tl->_sync = nand_tl_sync;
	info_tl->mtd_tl->_unlock = NULL;
	info_tl->mtd_tl->_suspend = nand_tl_suspend;
	info_tl->mtd_tl->_resume = nand_tl_resume;
	info_tl->mtd_tl->_block_isbad = nand_tl_block_isbad;
	info_tl->mtd_tl->_block_markbad = nand_tl_block_markbad;
	spin_lock_init(&info_tl->lock);
	init_waitqueue_head(&info_tl->wq);
	info_tl->state = FL_READY;

	if (nand_tl_bbt_load(tbbt_addr,0))
		goto err_out;

	/* determine the size of the logical flash as presented to the upper layers
	* this can be derived from the logical start and end and the space reserved
	* for the BBT.
	*/
	info_tl->mtd_tl->size = (uintptr_t)info_tl->tbbt->logic_flash_end -
	                          (uintptr_t)info_tl->tbbt->logic_flash_start -
	                          2*info_tl->mtd_tl->erasesize;
	info_tl->pages_per_block = info_tl->mtd_tl->erasesize / info_tl->mtd_tl->writesize;

	proc_entry = create_proc_read_entry("tbbt" ,0, NULL, ReadProc, NULL);
	if (!proc_entry)
		printk(KERN_NOTICE "Technicolor nand flash: unable to create /proc/tbbt!\n");

	printk(KERN_NOTICE "Technicolor nand flash translation layer initialized.\n");
#ifdef CONFIG_MTD_NAND_TL_TEST
	t_NAND_TEST(101);
	NAND_TRACE("halting");
	while(1);
#endif
	return info_tl->mtd_tl;
	
err_out:
	if (info_tl)
	{
		if (info_tl->mtd_tl)
			kfree(info_tl->mtd_tl);
		kfree(info_tl);
	}
	return 0;
}

static int ReadProc(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
	int len=0;
	nand_tl_bbt_print();
	*eof = 1;
	return len;
}

static void nand_tl_destroy(struct mtd_info *mtd_tl)
{
	if (info_tl)
	{
		if (info_tl->mtd_tl)
			kfree(info_tl->mtd_tl);
		if (info_tl->tbbt)
			kfree(info_tl->tbbt);
		kfree(info_tl);
	}
}

static struct mtd_chip_driver nand_tl_chipdriver = {
	.name		= "technicolor-nand-tl",
	.module		= THIS_MODULE,
	.probe		= nand_tl_probe,
	.destroy	= nand_tl_destroy
};

int __init nand_tl_init(void)
{
	register_mtd_chip_driver(&nand_tl_chipdriver);
	return 0;

}

void __exit nand_tl_exit(void)
{
	unregister_mtd_chip_driver(&nand_tl_chipdriver);
}

module_init(nand_tl_init);
module_exit(nand_tl_exit);


////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//                                               TEST SECTION
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//                                               TEST SECTION
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////


/* !!
   TO ENABLE THE TEST, DEFINE CONFIG_MTD_NAND_TL_TEST IN THIS FILE AND IN THE BRCM DRIVER 
   FOR ERROR INJECTION (IN BRCMNAND_BASE.C)
   !!
*/

#ifdef CONFIG_MTD_NAND_TL_TEST

/* TEST PARAMS */
#define TEST_SIZE 0x20000 /* defines the testsize on flash, mirrored in ram */
#define TEST_START_ADDR 0x10b0000 /* start address in flash */
/* END TEST PARAMS */

#include <linux/random.h>

static int test_load_bbt(void);
static int test_rnd_r_w(void);
static int test_w_bbt(void);
static int test_erase_fail(void);
static int test_w_fail(void);
static int bbt_in_flash_check(void);
static int final_test(void);

#define RND_ADDR ((unsigned char *)(random32() % TEST_SIZE))
#define RND_SIZE(addr) (random32() % (TEST_SIZE - (unsigned)addr))

#define NAND_TEST(format, args...)  printk("\t\t[NAND] TEST %s : "format"\r\n", teststr, ##args)

unsigned char *memmirror;
unsigned char *oobmirror;
char teststr[16];
void *testlist[] = {
                    &test_load_bbt,
                    &test_rnd_r_w,
                    &test_w_bbt, 
                    &test_erase_fail, 
                    &test_w_fail, 
                    &final_test, 
                    0};

static int ClearFlash(void)
{
	struct erase_info instr;

	instr.mtd = info_tl->mtd_tl;
	instr.addr = (loff_t)(unsigned int)TEST_START_ADDR;
	instr.len = TEST_SIZE;
	instr.callback = NULL;
	
	NAND_TEST("Erasing testarea");
	if (mtd_erase(info_tl->mtd_tl, &instr))
	{
		NAND_TEST("Error during flash sector erase");
		return -1;
	}

	memset(memmirror, 0xFF, TEST_SIZE);
	memset(oobmirror, 0xFF, TO_OOB_SIZE(TEST_SIZE));

	return 0;
}

static int WriteToFlashWithErase(unsigned char *addr, int size, int data_seed)
{
	int i;
	unsigned char old_data, new_data;
	struct erase_info instr;
	int retlen = 0;
	
	instr.mtd = info_tl->mtd_tl;
	instr.callback = NULL;
	instr.len = info_tl->mtd_tl->erasesize;

	NAND_TEST("Writing %d bytes to addr %x", size, (unsigned int)addr);
	srandom32(data_seed);
	
	i = (unsigned int)addr;
	while(i < (unsigned int)addr+size)
	{
		old_data = memmirror[i];
		new_data = random32() & 0xFF;
		
		if (old_data != 0xFF && (new_data != old_data))
		{
			NAND_TEST("Erasing sector for 0x%x", TEST_START_ADDR + i);
			instr.addr = TEST_START_ADDR + (i & ~(info_tl->mtd_tl->erasesize - 1));
			if (info_tl->mtd_tl->erase(info_tl->mtd_tl, &instr))
			{
				NAND_TEST("Error during flash sector erase");
				return -1;
			}
			memset(&memmirror[i & ~(info_tl->mtd_tl->erasesize - 1)], 0xFF, info_tl->mtd_tl->erasesize);
			memset(&oobmirror[TO_OOB_ADDR(i & ~(info_tl->mtd_tl->erasesize - 1))], 0xFF, TO_OOB_SIZE(info_tl->mtd_tl->erasesize));
		}
		memmirror[i] = new_data;
		i++;
	}
	if (mtd_write(info_tl->mtd_tl, (loff_t)((unsigned int)addr + TEST_START_ADDR), size, &retlen, &memmirror[(unsigned int)addr]) || retlen != size)
	{
		NAND_TEST("Error during flash write");
		return -1;
	}
	return 0;
}

static int WriteToOob(unsigned char *addr, int size, int data_seed)
{
	int i, assigned = 0;
	struct mtd_oob_ops ops;
	int oob_addr = TO_OOB_ADDR((unsigned int)addr);
	
	NAND_TEST("Writing %d bytes to oob starting in page addr %x (oob addr %x)", size, (unsigned int)addr + TEST_START_ADDR, oob_addr);
	srandom32(data_seed);
	
	i = oob_addr;
	while(i < (unsigned int)oob_addr+size)
	{
		if (oobmirror[i] == 0xFF)
		{
			oobmirror[i] = random32() & 0xFF;
			assigned = 1;
		}
		i++;
	}
	
	if (!assigned)
	{
		NAND_TEST("Haven't updated any value. refine test.");
		return -1;
	}
	
	ops.mode = MTD_OPS_AUTO_OOB;
	ops.datbuf = NULL;
	ops.len = ops.ooboffs = ops.retlen = ops.oobretlen = 0;
	ops.ooblen = size;
	ops.oobbuf = &oobmirror[oob_addr];
	
	if (mtd_write_oob(info_tl->mtd_tl, (loff_t)(unsigned int)addr + TEST_START_ADDR, &ops) || ops.oobretlen != size)
	{
		NAND_TEST("Error during oob write");
		return -1;
	}
	return 0;
}

static int CheckFlashContent(void)
{
	int retlen = 0, i;
	unsigned char *checkmem;
	
	NAND_TEST("Checking");
	checkmem = kmalloc(TEST_SIZE, GFP_KERNEL);
	
	if (!checkmem)
	{
		NAND_TEST("Cant allocate test memory3");
		return -1;
	}
	
	if (mtd_read(info_tl->mtd_tl, (loff_t)(unsigned int)TEST_START_ADDR, TEST_SIZE, &retlen, checkmem) || retlen != TEST_SIZE)
	{
		NAND_TEST("Error during flash read");
		goto err_out;
	}
	
	if (memcmp(memmirror, checkmem, TEST_SIZE))
	{
		NAND_TEST("Contents differ");
		for (i=0;i<TEST_SIZE;i++)
		{
			if (memmirror[i] != checkmem[i])
			{
				NAND_TEST("offset 0x%x, exp %d, is now %d", i, memmirror[i], checkmem[i]);
			}
		}
		goto err_out;
	}
	if (checkmem)
		kfree(checkmem);
	return 0;
err_out:
	if (checkmem)
		kfree(checkmem);
	return -1;
}

static int CheckOobContent(void)
{
	int i;
	unsigned char *checkmem;
	struct mtd_oob_ops ops;
	int size = TO_OOB_SIZE(TEST_SIZE);
	
	NAND_TEST("Checking oob");
	checkmem = kmalloc(size, GFP_KERNEL);
	
	if (!checkmem)
	{
		NAND_TEST("Cant allocate test memory4");
		return -1;
	}
	
	ops.mode = MTD_OPS_AUTO_OOB;
	ops.ooblen = size;
	ops.oobbuf = checkmem;
	ops.len = ops.ooboffs = ops.retlen = ops.oobretlen = 0;
	ops.datbuf = NULL;
	if (mtd_read_oob(info_tl->mtd_tl, (loff_t)(unsigned int)TEST_START_ADDR, &ops) || ops.oobretlen != size)
	{
		NAND_TEST("Error during oob read");
		goto err_out;
	}
	
	if (memcmp(oobmirror, checkmem, size))
	{
		NAND_TEST("oob Contents differ");
		for (i=0;i<size;i++)
		{
			if (oobmirror[i] != checkmem[i])
			{
				NAND_TEST("offset 0x%x, exp %d, is now %d", i, oobmirror[i], checkmem[i]);
			}
		}
		goto err_out;
	}
	if (checkmem)
		kfree(checkmem);
	return 0;
err_out:
	if (checkmem)
		kfree(checkmem);
	return -1;
}

static int test_load_bbt(void)
{
	/* RANDOM WRITE - READ TEST */

	unsigned char *addr;
	unsigned long size;
	int i;
	strcpy(teststr, "load_bbt");
	NAND_TEST("Test starts.");
	
	max_amount_of_bad_blocks = info_tl->tbbt->max_badblock_count - 2 ;
	
	for (i = 0; i < 500; ++i)
	{
		nand_tl_bbt_load(0,0);
	}
	return 0;
}

static int test_rnd_r_w(void)
{
	/* RANDOM WRITE - READ TEST */

	unsigned char *addr;
	unsigned long size;
	int i;
	strcpy(teststr, "rnd_w_r");
	NAND_TEST("Test starts.");
	
	max_amount_of_bad_blocks = info_tl->tbbt->max_badblock_count - 2 ;

	for (i = 0; i < 500; i++)
	{
		addr = RND_ADDR;
		size = RND_SIZE(addr);
		
		NAND_TEST("-> %d", i);
	
		if (WriteToFlashWithErase(addr, size, random32()))
		{
			NAND_TEST("Write flash error (%d)", i);
			return -1;
		}
		
		if ((random32() % 100) < 20)
		{
			WriteToOob(addr, TO_OOB_SIZE(size), random32());
		}
		
		if (CheckFlashContent())
		{
			NAND_TEST("Read flash error (%d)", i);
			return -1;
		}
		if (CheckOobContent())
		{
			NAND_TEST("Read oob error (%d)", i);
			return -1;
		}
		if (amount_of_bad_blocks < max_amount_of_bad_blocks)
			i = 0;
		if (bbt_in_flash_check())
			return -1;

	}
	return 0;
}
static int test_w_bbt(void)
{
	/* WRITE TO BBT WHICH HAS TO BE DENIED */
	
	int i, retlen;
	unsigned char *addr = info_tl->tbbt->logic_bbt_addr;
	struct erase_info instr;

	instr.mtd = info_tl->mtd_tl;
	instr.len = info_tl->mtd_tl->erasesize;
	instr.callback = NULL;
	
	strcpy(teststr, "w_bbt");
	NAND_TEST("Test starts.");
	for (i=0;i<2;i++)
	{
		instr.addr = (loff_t)(unsigned int)addr;
		if (!mtd_erase(info_tl->mtd_tl, &instr) || 
		    !mtd_write(info_tl->mtd_tl, (loff_t)(unsigned int)addr, sizeof(struct erase_info), &retlen, (void *)&instr))
		{
			NAND_TEST("Error bbt block: access in NOT denied.");
			return -1;
		}
		addr = info_tl->tbbt->logic_bbt_mirror_addr;
	}
	return bbt_in_flash_check();
}

static int test_erase_fail(void)
{
	int i = 0;
	unsigned char *addr;
	struct erase_info instr;

	instr.mtd = info_tl->mtd_tl;
	instr.len = info_tl->mtd_tl->erasesize;
	instr.callback = NULL;
	
	/* erase will fail because no more spare blocks */
	
	strcpy(teststr, "erase_fail");
	NAND_TEST("Test starts.");

	max_amount_of_bad_blocks = info_tl->tbbt->max_badblock_count + 50 ;
	
	for(i=0;i<10000;i++)
	{
		addr = RND_ADDR;
		NAND_TEST("erasing %x", (unsigned int)addr);
		instr.addr = (loff_t)(TEST_START_ADDR + ((unsigned int)addr & ~(info_tl->mtd_tl->erasesize - 1)));
		if (mtd_erase(info_tl->mtd_tl, &instr))
		{
			NAND_TEST("erase failed as expected.");
			return bbt_in_flash_check();
		}
	}
	NAND_TEST("erase should have failed by now...");
	return -1;
}

static int test_w_fail(void)
{
	int i = 0, retlen = 0;
	unsigned char *addr;
	
	/* write will fail because no more spare blocks */

	strcpy(teststr, "w_fail");
	NAND_TEST("Test starts.");

	max_amount_of_bad_blocks = info_tl->tbbt->max_badblock_count + 1 ;
	
	for(i=0;i<1000;i++)
	{
		addr = RND_ADDR;
		NAND_TEST("writing %x", (unsigned int)addr);
		if (mtd_write(info_tl->mtd_tl, (loff_t)((unsigned int)addr + TEST_START_ADDR), 0x100, &retlen, &memmirror[(unsigned int)addr]))
		{
			NAND_TEST("write failed as expected.");
			return bbt_in_flash_check();
		}
		if (retlen != 0x100)
		{
			NAND_TEST("write failed unexpectedly");
			return -1;
		}
	}
	NAND_TEST("write should have failed by now...");
	return -1;
}

static int final_test(void)
{
	/* RELOADING AND CHECKING OF BBT */

	int version;
	unsigned char *addr;
	
	strcpy(teststr, "final");
	NAND_TEST("Tests finalizing");
	nand_tl_bbt_print();
	version = info_tl->tbbt->version;
	addr = info_tl->tbbt->logic_bbt_addr;
	nand_tl_bbt_load(0,0);
	if (version != info_tl->tbbt->version)
		return -1;
	if (addr != info_tl->tbbt->logic_bbt_addr)
		return -1;
	return 0;
}

static int bbt_in_flash_check(void)
{
	int bbt_size;
	t_tbbt *bbt;
	int exp_version;

	/* checks the sanity of the bbt's in flash: there should be 2, the bbt in mem should be the latest in flash etc... */

	NAND_TEST("bbt check...");

	bbt_size = TBBT_SIZE + (info_tl->tbbt->max_badblock_count * (TBBT_BLOCK_SIZE + TBBT_SPAREBLOCK_SIZE));
	
	NAND_TEST("Checking bbt @ 0x%08X", (unsigned int)info_tl->tbbt->logic_bbt_addr);
	if (nand_tl_bbt_load((loff_t)(unsigned int)info_tl->tbbt->logic_bbt_addr, &bbt))
	{
		goto err_out;
	}
	bbt->block_info = info_tl->tbbt->block_info;
	bbt->spareblock_info  = info_tl->tbbt->spareblock_info;
	nand_tl_bbt_update_crc(bbt);
	if (memcmp(info_tl->tbbt, bbt, bbt_size))
	{
		NAND_TEST("bbt inequality");
		goto err_out;
	}
	NAND_TEST("Checking bbt @ 0x%08X", (unsigned int)info_tl->tbbt->logic_bbt_mirror_addr);
	if (nand_tl_bbt_load((loff_t)(unsigned int)info_tl->tbbt->logic_bbt_mirror_addr, &bbt))
	{
		goto err_out;
	}
	exp_version = info_tl->tbbt->version - 1;
	if (exp_version < 0)
		exp_version = 2;
	
	if (bbt->version != exp_version)
	{
		NAND_TEST("incorrect version of mirror bbt (%d %d)", bbt->version, exp_version);
		goto err_out;
	}
	
	NAND_TEST("bbt check ok.");
	if (bbt)
		kfree(bbt);
	return 0;
err_out:
	NAND_TEST("bbt check error.");
	if (bbt)
		kfree(bbt);
	return -1;
}

static void setup_chip(void)
{
	int i;
	
	NAND_TEST("setup");
	code_cov[0] = 0;
	code_cov[1] = 0;
	amount_of_bad_blocks = 0;
	for (i=0;i<info_tl->tbbt->block_count;i++)
		amount_of_bad_blocks += info_tl->tbbt->block_info[i].isbad;
	for (i=0;i<info_tl->tbbt->spareblock_count;i++)
		amount_of_bad_blocks += info_tl->tbbt->spareblock_info[i].isbad;
	max_amount_of_bad_blocks = info_tl->tbbt->max_badblock_count - 1 ;
}

void t_NAND_TEST(int seed)
{
	int i;

	strcpy(teststr, "init");
	NAND_TEST("--> NAND TEST (seed %d) <--", seed);
	srandom32(seed);
	NAND_TEST("Test area = %d bytes, starting at 0x%08X", TEST_SIZE, TEST_START_ADDR);
	

	memmirror = kmalloc(TEST_SIZE, GFP_KERNEL);
	
	if (!memmirror)
	{
		NAND_TEST("Cant allocate test memory1");
		goto err_out;
	}
	
	oobmirror = kmalloc(TO_OOB_SIZE(TEST_SIZE), GFP_KERNEL);
	
	if (!oobmirror)
	{
		NAND_TEST("Cant allocate test memory2");
		goto err_out;
	}
	
	NAND_TEST("Clearing contents for starters...");
	if (ClearFlash())
	{
		NAND_TEST("Clear flash error");
		goto err_out;
	}

	setup_chip();
	
	if (bbt_in_flash_check())
		goto err_out;
		
	for (i=0; testlist[i]; i++)
	{
		int (*test) (void) = (int*)testlist[i];
		if (testlist[i])
			if (test())
			{
				NAND_TEST("test error");
				goto err_out;
			}
	}

	kfree(memmirror);
	NAND_TEST("NAND TEST SUCCES");
	NAND_TEST("CODE COVERAGE RESULT = 0x%08X 0x%08X", code_cov[0], code_cov[1]);
	return;

err_out:	
	if (memmirror)
		kfree(memmirror);
	nand_tl_bbt_print();
	NAND_TEST("NAND TEST FAIL");
	return;
}

#endif
