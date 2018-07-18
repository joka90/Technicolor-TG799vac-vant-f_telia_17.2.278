/****************************************************************************
 * <:copyright-BRCM:2013:DUAL/GPL:standard
 * 
 *    Copyright (c) 2013 Broadcom Corporation
 *    All Rights Reserved
 * 
 * Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed
 * to you under the terms of the GNU General Public License version 2
 * (the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
 * with the following added to such license:
 * 
 *    As a special exception, the copyright holders of this software give
 *    you permission to link this software with independent modules, and
 *    to copy and distribute the resulting executable under terms of your
 *    choice, provided that you also meet, for each linked independent
 *    module, the terms and conditions of the license of that module.
 *    An independent module is a module which is not derived from this
 *    software.  The special exception does not apply to any modifications
 *    of the software.
 * 
 * Not withstanding the above, under no circumstances may you combine
 * this software in any way with any other Broadcom software provided
 * under a license other than the GPL, without Broadcom's express prior
 * written consent.
 * 
 * :>
 ***************************************************************************/

#if !defined(_PCMSHIMDRV_H_)
#define _PCMSHIMDRV_H_

#if defined(__cplusplus)
extern "C" {
#endif

/* pcmshim driver is following pcm device driver major and minor number */
#define PCMSHIM_MAJOR    217
#define PCMSHIM_MINOR    0
#if defined(CONFIG_BCM96838)
#define PCMSHIM_DEVNAME  "pcmshim0"
#else
#define PCMSHIM_DEVNAME  "pcmshim"
#endif


typedef struct {
   unsigned int* bufp;
} PCMSHIMDRV_GETBUF_PARAM, *PPCMSHIMDRV_GETBUF_PARAM;


#define PCMSHIMIOCTL_GETBUF_CMD \
    _IOWR(PCMSHIM_MAJOR, 0, PCMSHIMDRV_GETBUF_PARAM)

#define MAX_PCMSHIM_IOCTL_CMDS  1


#if defined(__cplusplus)
}
#endif

#endif /* _PCMSHIMDRV_H_ */
