/************** COPYRIGHT AND CONFIDENTIALITY INFORMATION ********************
**                                                                          **
** Copyright (c) 2014 Technicolor                                           **
** All Rights Reserved                                                      **
**                                                                          **
** This program contains proprietary information which is a trade           **
** secret of TECHNICOLOR and/or its affiliates and also is protected as     **
** an unpublished work under applicable Copyright laws. Recipient is        **
** to retain this program in confidence and is not permitted to use or      **
** make copies thereof other than as permitted in a written agreement       **
** with TECHNICOLOR, UNLESS OTHERWISE EXPRESSLY ALLOWED BY APPLICABLE LAWS. **
**                                                                          **
******************************************************************************/

#ifndef UPNPDESCSTRINGS_H_INCLUDED
#define UPNPDESCSTRINGS_H_INCLUDED

#include "config.h"

/* strings used in the root device xml description */
#define ROOTDEV_FRIENDLYNAME		OS_NAME " router"
#define ROOTDEV_MANUFACTURER		OS_NAME
#define ROOTDEV_MANUFACTURERURL		OS_URL
#define ROOTDEV_MODELNAME			OS_NAME " router"
#define ROOTDEV_MODELDESCRIPTION	OS_NAME " router"
#define ROOTDEV_MODELURL			OS_URL
#define ROOTDEV_UPC					"000000000000"

#define LANDEV_FRIENDLYNAME			"LANDevice"
#define LANDEV_MANUFACTURER			"MiniTR064d"
#define LANDEV_MANUFACTURERURL		"http://www.technicolor.com/"
#define LANDEV_MODELNAME			"LAN Device"
#define LANDEV_MODELDESCRIPTION		"LAN Device"
#define LANDEV_MODELNUMBER			UPNP_VERSION
#define LANDEV_MODELURL				"http://www.technicolor.com/"
#define LANDEV_UPC					"000000000000"
/* UPC is 12 digit (barcode) */

#define WANDEV_FRIENDLYNAME			"WANDevice"
#define WANDEV_MANUFACTURER			"MiniTR064d"
#define WANDEV_MANUFACTURERURL		"http://www.technicolor.com/"
#define WANDEV_MODELNAME			"WAN Device"
#define WANDEV_MODELDESCRIPTION		"WAN Device"
#define WANDEV_MODELNUMBER			UPNP_VERSION
#define WANDEV_MODELURL				"http://www.technicolor.com/"
#define WANDEV_UPC					"000000000000"
/* UPC is 12 digit (barcode) */

#define WANCDEV_FRIENDLYNAME		"WANConnectionDevice"
#define WANCDEV_MANUFACTURER		WANDEV_MANUFACTURER
#define WANCDEV_MANUFACTURERURL		WANDEV_MANUFACTURERURL
#define WANCDEV_MODELNAME			"MiniTR064d"
#define WANCDEV_MODELDESCRIPTION	"MiniUPnP daemon"
#define WANCDEV_MODELNUMBER			UPNP_VERSION
#define WANCDEV_MODELURL			"http://www.technicolor.com/"
#define WANCDEV_UPC					"000000000000"
/* UPC is 12 digit (barcode) */

#endif

