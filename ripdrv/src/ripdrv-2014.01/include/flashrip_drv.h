#ifndef __FLASHRIP_DRV_H__
#define __FLASHRIP_DRV_H__

unsigned long rip_drv_init(void);

unsigned long rip_drv_read (unsigned long *length, unsigned short id, unsigned char *data);

unsigned long rip_drv_write (unsigned long length, unsigned short id, unsigned char *data, unsigned long attrHi, unsigned long attrLo);

unsigned long rip_drv_get_flags (unsigned short id, unsigned long *attrHi, unsigned long *attrLo);

unsigned long rip_drv_lock_item (unsigned short id);

#endif //__FLASHRIP_DRV_H__
