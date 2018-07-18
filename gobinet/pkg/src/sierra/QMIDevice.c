/*===========================================================================
FILE:
   QMIDevice.c

DESCRIPTION:
   Functions related to the QMI interface device

FUNCTIONS:
   Generic functions
      IsDeviceValid
      PrintHex
      GobiSetDownReason
      GobiClearDownReason
      GobiTestDownReason

   Driver level asynchronous read functions
      ResubmitIntURB
      ReadCallback
      IntCallback
      StartRead
      KillRead

   Internal read/write functions
      ReadAsync
      UpSem
      ReadSync
      WriteSync

   Internal memory management functions
      GetClientID
      ReleaseClientID
      FindClientMem
      AddToReadMemList
      PopFromReadMemList
      AddToNotifyList
      NotifyAndPopNotifyList

   Internal userspace wrapper functions
      UserspaceunlockedIOCTL

   Userspace wrappers
      UserspaceOpen
      UserspaceIOCTL
      UserspaceClose
      UserspaceRead
      UserspaceWrite
      UserspacePoll

   Initializer and destructor
      RegisterQMIDevice
      DeregisterQMIDevice

   Driver level client management
      QMIReady
      QMIWDSCallback
      SetupQMIWDSCallback
      QMIDMSGetMEID

Copyright (c) 2011, Code Aurora Forum. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Code Aurora Forum nor
      the names of its contributors may be used to endorse or promote
      products derived from this software without specific prior written
      permission.

Alternatively, provided that this notice is retained in full, this software
may be relicensed by the recipient under the terms of the GNU General Public
License version 2 ("GPL") and only version 2, in which case the provisions of
the GPL apply INSTEAD OF those given above.  If the recipient relicenses the
software under the GPL, then the identification text in the MODULE_LICENSE
macro must be changed to reflect "GPLv2" instead of "Dual BSD/GPL".  Once a
recipient changes the license terms to the GPL, subsequent recipients shall
not relicense under alternate licensing terms, including the BSD or dual
BSD/GPL terms.  In addition, the following license statement immediately
below and between the words START and END shall also then apply when this
software is relicensed under the GPL:

START

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License version 2 and only version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

END

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
===========================================================================*/

//---------------------------------------------------------------------------
// Include Files
//---------------------------------------------------------------------------
#include <asm/unaligned.h>
#include "QMIDevice.h"
#include <linux/module.h>
#include <linux/proc_fs.h> // for the proc filesystem

//-----------------------------------------------------------------------------
// Definitions
//-----------------------------------------------------------------------------

extern int debug;
extern int is9x15;
extern int interruptible;
const bool clientmemdebug = 0;

#define SEND_ENCAPSULATED_COMMAND (0)
#define GET_ENCAPSULATED_RESPONSE (1)
#define USB_WRITE_TIMEOUT 500   // must be less than AM timeout
#define USB_WRITE_RETRY (2)
#define USB_READ_TIMEOUT (500)

/* initially all zero */
int qcqmi_table[MAX_QCQMI];

#define CLIENT_READMEM_SNAPSHOT(clientID, pdev)\
   if( debug == 1 && clientmemdebug )\
   {\
      sClientMemList *pclnt;\
      sReadMemList *plist;\
      pclnt = FindClientMem((pdev), (clientID));\
      plist = pclnt->mpList;\
      if( (pdev) != NULL){\
          while(plist != NULL)\
          {\
             DBG(  "clientID 0x%x, mDataSize = %u, mpData = 0x%p, mTransactionID = %u,  \
                    mpNext = 0x%p\n", (clientID), plist->mDataSize, plist->mpData, \
                    plist->mTransactionID, plist->mpNext  ) \
             /* advance to next entry */\
             plist = plist->mpNext;\
          }\
      }\
   }

#ifdef CONFIG_PM
// Prototype to GobiNetSuspend function
int GobiNetSuspend(
   struct usb_interface *     pIntf,
   pm_message_t               powerEvent );
#endif /* CONFIG_PM */

// IOCTL to generate a client ID for this service type
#define IOCTL_QMI_GET_SERVICE_FILE 0x8BE0 + 1

// IOCTL to get the VIDPID of the device
#define IOCTL_QMI_GET_DEVICE_VIDPID 0x8BE0 + 2

// IOCTL to get the MEID of the device
#define IOCTL_QMI_GET_DEVICE_MEID 0x8BE0 + 3

#define IOCTL_QMI_RELEASE_SERVICE_FILE_IOCTL  (0x8BE0 + 4)

#define IOCTL_QMI_ADD_MAPPING 0x8BE0 + 5
#define IOCTL_QMI_DEL_MAPPING 0x8BE0 + 6
#define IOCTL_QMI_CLR_MAPPING 0x8BE0 + 7

#define IOCTL_QMI_QOS_SIMULATE 0x8BE0 + 8
#define IOCTL_QMI_GET_TX_Q_LEN 0x8BE0 + 9

#define IOCTL_QMI_EDIT_MAPPING 0x8BE0 + 10
#define IOCTL_QMI_READ_MAPPING 0x8BE0 + 11
#define IOCTL_QMI_DUMP_MAPPING 0x8BE0 + 12
#define IOCTL_QMI_GET_USBNET_STATS 0x8BE0 + 13
#define IOCTL_QMI_SET_DEVICE_MTU 0x8BE0 + 14

// CDC GET_ENCAPSULATED_RESPONSE packet
#define CDC_GET_ENCAPSULATED_RESPONSE_LE 0x01A1ll
#define CDC_GET_ENCAPSULATED_RESPONSE_BE 0xA101000000000000ll
/* The following masks filter the common part of the encapsulated response
 * packet value for Gobi and QMI devices, ie. ignore usb interface number
 */
#define CDC_RSP_MASK_BE 0xFFFFFFFF00FFFFFFll
#define CDC_RSP_MASK_LE 0xFFFFFFE0FFFFFFFFll

const int i = 1;
#define is_bigendian() ( (*(char*)&i) == 0 )
#define CDC_GET_ENCAPSULATED_RESPONSE(pcdcrsp, pmask)\
{\
   *pcdcrsp  = is_bigendian() ? CDC_GET_ENCAPSULATED_RESPONSE_BE \
                          : CDC_GET_ENCAPSULATED_RESPONSE_LE ; \
   *pmask = is_bigendian() ? CDC_RSP_MASK_BE \
                           : CDC_RSP_MASK_LE; \
}

// CDC CONNECTION_SPEED_CHANGE indication packet
#define CDC_CONNECTION_SPEED_CHANGE_LE 0x2AA1ll
#define CDC_CONNECTION_SPEED_CHANGE_BE 0xA12A000000000000ll
/* The following masks filter the common part of the connection speed change
 * packet value for Gobi and QMI devices
 */
#define CDC_CONNSPD_MASK_BE 0xFFFFFFFFFFFF7FFFll
#define CDC_CONNSPD_MASK_LE 0XFFF7FFFFFFFFFFFFll
#define CDC_GET_CONNECTION_SPEED_CHANGE(pcdccscp, pmask)\
{\
   *pcdccscp  = is_bigendian() ? CDC_CONNECTION_SPEED_CHANGE_BE \
                          : CDC_CONNECTION_SPEED_CHANGE_LE ; \
   *pmask = is_bigendian() ? CDC_CONNSPD_MASK_BE \
                           : CDC_CONNSPD_MASK_LE; \
}

#define SET_CONTROL_LINE_STATE_REQUEST_TYPE \
       (USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE)
#define SET_CONTROL_LINE_STATE_REQUEST             0x22
#define CONTROL_DTR                     0x01
#define CONTROL_RTS                     0x02

/*=========================================================================*/
// UserspaceQMIFops
//    QMI device's userspace file operations
/*=========================================================================*/
struct file_operations UserspaceQMIFops =
{
   .owner     = THIS_MODULE,
   .read      = UserspaceRead,
   .write     = UserspaceWrite,
#ifdef CONFIG_COMPAT
   .compat_ioctl = UserspaceunlockedIOCTL,
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION( 2,6,36 ))
   .unlocked_ioctl = UserspaceunlockedIOCTL,
#else
   .ioctl     = UserspaceIOCTL,
#endif
   .open      = UserspaceOpen,
   .flush     = UserspaceClose,
   .poll      = UserspacePoll,
};

/*=========================================================================*/
// Generic functions
/*=========================================================================*/
u8 QMIXactionIDGet( sGobiUSBNet *pDev)
{
   u8 transactionID;

   if( 0 == (transactionID = atomic_add_return( 1, &pDev->mQMIDev.mQMICTLTransactionID)) )
   {
      transactionID = atomic_add_return( 1, &pDev->mQMIDev.mQMICTLTransactionID );
   }
   
   return transactionID;
}

static struct usb_endpoint_descriptor *GetEndpoint(
    struct usb_interface *pintf,
    int type,
    int dir )
{
   int i;
   struct usb_host_interface *iface = pintf->cur_altsetting;
   struct usb_endpoint_descriptor *pendp;

   for( i = 0; i < iface->desc.bNumEndpoints; i++)
   {
      pendp = &iface->endpoint[i].desc;
      if( ((pendp->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == dir)
          &&
          (usb_endpoint_type(pendp) == type) )
      {
         return pendp;
      }
   }

   return NULL;
}

struct dit_data 
{ 
   int result; 
   struct task_struct *task; 
   
}; 


/*===========================================================================
METHOD:
   IsDeviceValid (Public Method)

DESCRIPTION:
   Basic test to see if device memory is valid

PARAMETERS:
   pDev     [ I ] - Device specific memory

RETURN VALUE:
   bool
===========================================================================*/
bool IsDeviceValid( sGobiUSBNet * pDev )
{
   if (pDev == NULL)
   {
      return false;
   }

   if (pDev->mbQMIValid == false)
   {
      return false;
   }

   return true;
}

/*===========================================================================
METHOD:
   PrintHex (Public Method)

DESCRIPTION:
   Print Hex data, for debug purposes

PARAMETERS:
   pBuffer       [ I ] - Data buffer
   bufSize       [ I ] - Size of data buffer

RETURN VALUE:
   None
===========================================================================*/
void PrintHex(
   void *      pBuffer,
   u16         bufSize )
{
   char * pPrintBuf;
   u16 pos;
   int status;

   if (debug != 1)
   {
       return;
   }

   pPrintBuf = kmalloc( bufSize * 3 + 1, GFP_ATOMIC );
   if (pPrintBuf == NULL)
   {
      DBG( "Unable to allocate buffer\n" );
      return;
   }
   memset( pPrintBuf, 0 , bufSize * 3 + 1 );

   for (pos = 0; pos < bufSize; pos++)
   {
      status = snprintf( (pPrintBuf + (pos * 3)),
                         4,
                         "%02X ",
                         *(u8 *)(pBuffer + pos) );
      if (status != 3)
      {
         DBG( "snprintf error %d\n", status );
         kfree( pPrintBuf );
         return;
      }
   }

   DBG( "   : %s\n", pPrintBuf );

   kfree( pPrintBuf );
   pPrintBuf = NULL;
   return;
}

/*===========================================================================
METHOD:
   GobiSetDownReason (Public Method)

DESCRIPTION:
   Sets mDownReason and turns carrier off

PARAMETERS
   pDev     [ I ] - Device specific memory
   reason   [ I ] - Reason device is down

RETURN VALUE:
   None
===========================================================================*/
void GobiSetDownReason(
   sGobiUSBNet *    pDev,
   u8                 reason )
{
   set_bit( reason, &pDev->mDownReason );

   netif_carrier_off( pDev->mpNetDev->net );
}

/*===========================================================================
METHOD:
   GobiClearDownReason (Public Method)

DESCRIPTION:
   Clear mDownReason and may turn carrier on

PARAMETERS
   pDev     [ I ] - Device specific memory
   reason   [ I ] - Reason device is no longer down

RETURN VALUE:
   None
===========================================================================*/
void GobiClearDownReason(
   sGobiUSBNet *    pDev,
   u8                 reason )
{
   clear_bit( reason, &pDev->mDownReason );

#if (LINUX_VERSION_CODE >= KERNEL_VERSION( 3,11,0 ))
    netif_carrier_on( pDev->mpNetDev->net );
#else
   if (pDev->mDownReason == 0)
   {
      netif_carrier_on( pDev->mpNetDev->net );
   }
#endif
}

/*===========================================================================
METHOD:
   GobiTestDownReason (Public Method)

DESCRIPTION:
   Test mDownReason and returns whether reason is set

PARAMETERS
   pDev     [ I ] - Device specific memory
   reason   [ I ] - Reason device is down

RETURN VALUE:
   bool
===========================================================================*/
bool GobiTestDownReason(
   sGobiUSBNet *    pDev,
   u8                 reason )
{
   return test_bit( reason, &pDev->mDownReason );
}

/*=========================================================================*/
// Driver level asynchronous read functions
/*=========================================================================*/

/*===========================================================================
METHOD:
   ResubmitIntURB (Public Method)

DESCRIPTION:
   Resubmit interrupt URB, re-using same values

PARAMETERS
   pIntURB       [ I ] - Interrupt URB

RETURN VALUE:
   int - 0 for success
         negative errno for failure
===========================================================================*/
int ResubmitIntURB( struct urb * pIntURB )
{
   int status;
   int interval;

   // Sanity test
   if ( (pIntURB == NULL)
   ||   (pIntURB->dev == NULL) )
   {
      return -EINVAL;
   }

   // Interval needs reset after every URB completion
   // QC suggestion, 4ms per poll:
   //   bInterval 6 = 2^5 = 32 frames = 4 ms per poll
   interval = (pIntURB->dev->speed == USB_SPEED_HIGH) ?
                 6 : max((int)(pIntURB->ep->desc.bInterval), 3);

   // Reschedule interrupt URB
   usb_fill_int_urb( pIntURB,
                     pIntURB->dev,
                     pIntURB->pipe,
                     pIntURB->transfer_buffer,
                     pIntURB->transfer_buffer_length,
                     pIntURB->complete,
                     pIntURB->context,
                     interval );
   status = usb_submit_urb( pIntURB, GFP_ATOMIC );
   if (status != 0)
   {
      DBG( "Error re-submitting Int URB %d\n", status );
   }

   return status;
}

/*===========================================================================
METHOD:
   ReadCallback (Public Method)

DESCRIPTION:
   Put the data in storage and notify anyone waiting for data

PARAMETERS
   pReadURB       [ I ] - URB this callback is run for

RETURN VALUE:
   None
===========================================================================*/
void ReadCallback( struct urb * pReadURB )
{
   int result;
   u16 clientID;
   sClientMemList * pClientMem;
   void * pData;
   void * pDataCopy;
   u16 dataSize;
   sGobiUSBNet * pDev;
   unsigned long flags;
   u16 transactionID;

   if (pReadURB == NULL)
   {
      DBG( "bad read URB\n" );
      return;
   }

   pDev = pReadURB->context;
   if (IsDeviceValid( pDev ) == false)
   {
      DBG( "Invalid device!\n" );
      return;
   }

   del_timer(&pDev->read_tmr);

   if (pReadURB->status != 0)
   {
      DBG( "Read status = %d\n", pReadURB->status );
      if ((pReadURB->status == -ECONNRESET) && (pReadURB->actual_length > 0))
      {
          pDev->readTimeoutCnt++;
          // Read URB unlinked after receiving data, send data to client
          DBG( "Read URB timeout/kill after recv data\n" );
          printk(KERN_WARNING "Read URB timeout/kill, recv data len (%d), cnt (%d)\n",
                  pReadURB->actual_length, pDev->readTimeoutCnt);
      }
      else
      {
          // Resubmit the interrupt URB
          if (IsDeviceValid( pDev ) == false)
          {
             DBG( "Invalid device!\n" );
             return;
          }
          ResubmitIntURB( pDev->mQMIDev.mpIntURB );
          return;
      }
   }
   DBG( "Read %d bytes\n", pReadURB->actual_length );

   pData = pReadURB->transfer_buffer;
   dataSize = pReadURB->actual_length;

   PrintHex( pData, dataSize );

   result = ParseQMUX( &clientID,
                       pData,
                       dataSize );
   if (result < 0)
   {
      DBG( "Read error parsing QMUX %d\n", result );
      if (IsDeviceValid( pDev ) == false)
      {
         DBG( "Invalid device!\n" );
         return;
      }
      // Resubmit the interrupt URB
      ResubmitIntURB( pDev->mQMIDev.mpIntURB );

      return;
   }

   // Grab transaction ID

   // Data large enough?
   if (dataSize < result + 3)
   {
      DBG( "Data buffer too small to parse\n" );
      if (IsDeviceValid( pDev ) == false)
      {
         DBG( "Invalid device!\n" );
         return;
      }
      // Resubmit the interrupt URB
      ResubmitIntURB( pDev->mQMIDev.mpIntURB );

      return;
   }

   // Transaction ID size is 1 for QMICTL, 2 for others
   if (clientID == QMICTL)
   {
      transactionID = *(u8*)(pData + result + 1);
   }
   else
   {
      transactionID = le16_to_cpu( get_unaligned((u16*)(pData + result + 1)) );
   }

   // Critical section
   spin_lock_irqsave( &pDev->mQMIDev.mClientMemLock, flags );

   // Find memory storage for this service and Client ID
   // Not using FindClientMem because it can't handle broadcasts
   pClientMem = pDev->mQMIDev.mpClientMemList;

   while (pClientMem != NULL)
   {
      if (pClientMem->mClientID == clientID
      ||  (pClientMem->mClientID | 0xff00) == clientID)
      {
         // Make copy of pData
         pDataCopy = kmalloc( dataSize, GFP_ATOMIC );
         if (pDataCopy == NULL)
         {
            DBG( "Error allocating client data memory\n" );

            // End critical section
            spin_unlock_irqrestore( &pDev->mQMIDev.mClientMemLock, flags );
            if (IsDeviceValid( pDev ) == false)
            {
             DBG( "Invalid device!\n" );
             return;
             }
            // Resubmit the interrupt URB
            ResubmitIntURB( pDev->mQMIDev.mpIntURB );

            return;             
         }

         memcpy( pDataCopy, pData, dataSize );

         if (AddToReadMemList( pDev,
                               pClientMem->mClientID,
                               transactionID,
                               pDataCopy,
                               dataSize ) == false)
         {
            DBG( "Error allocating pReadMemListEntry "
                 "read will be discarded\n" );
            kfree( pDataCopy );

            // End critical section
            spin_unlock_irqrestore( &pDev->mQMIDev.mClientMemLock, flags );
            if (IsDeviceValid( pDev ) == false)
            {
               DBG( "Invalid device!\n" );
               return;
            }
            // Resubmit the interrupt URB
            ResubmitIntURB( pDev->mQMIDev.mpIntURB );

            return;
         }

         // Success
         CLIENT_READMEM_SNAPSHOT(clientID, pDev);
         DBG( "Creating new readListEntry for client 0x%04X, TID %x\n",
              clientID,
              transactionID );

         // Notify this client data exists
         NotifyAndPopNotifyList( pDev,
                                 pClientMem->mClientID,
                                 transactionID );

         // Possibly notify poll() that data exists
         wake_up_interruptible( &pClientMem->mWaitQueue );

         // Not a broadcast
         if (clientID >> 8 != 0xff)
         {
            break;
         }
      }

      // Next element
      pClientMem = pClientMem->mpNext;
   }

   // End critical section
   spin_unlock_irqrestore( &pDev->mQMIDev.mClientMemLock, flags );
   if (IsDeviceValid( pDev ) == false)
    {
        DBG( "Invalid device!\n" );
        return;
    }
   // Resubmit the interrupt URB
   ResubmitIntURB( pDev->mQMIDev.mpIntURB );
}

void read_tmr_cb( struct urb * pReadURB )
{
  int result;

  DBG( "%s called (%ld).\n", __func__, jiffies );

  if ((pReadURB != NULL) && (pReadURB->status == -EINPROGRESS))
  {
     // Asynchronously unlink URB. On success, -EINPROGRESS will be returned, 
     // URB status will be set to -ECONNRESET, and ReadCallback() executed
     result = usb_unlink_urb( pReadURB );
     DBG( "%s called usb_unlink_urb, result = %d\n", __func__, result);
  }
}

/*===========================================================================
METHOD:
   IntCallback (Public Method)

DESCRIPTION:
   Data is available, fire off a read URB

PARAMETERS
   pIntURB       [ I ] - URB this callback is run for

RETURN VALUE:
   None
===========================================================================*/
void IntCallback( struct urb * pIntURB )
{
   int status;
   u64 CDCEncResp;
   u64 CDCEncRespMask;

   sGobiUSBNet * pDev = (sGobiUSBNet *)pIntURB->context;
   if (IsDeviceValid( pDev ) == false)
   {
      DBG( "Invalid device!\n" );
      return;
   }

   // Verify this was a normal interrupt
   if (pIntURB->status != 0)
   {
        DBG( "IntCallback: Int status = %d\n", pIntURB->status );

      // Ignore EOVERFLOW errors
      if (pIntURB->status != -EOVERFLOW)
      {
         // Read 'thread' dies here
         return;
      }
      if(pIntURB->status<0)
      {
         return;
      }
   }
   else
   {
      //TODO cast transfer_buffer to struct usb_cdc_notification
      
      // CDC GET_ENCAPSULATED_RESPONSE
      CDC_GET_ENCAPSULATED_RESPONSE(&CDCEncResp, &CDCEncRespMask)

      DBG( "IntCallback: Encapsulated Response = 0x%llx\n",
          (*(u64*)pIntURB->transfer_buffer));

      if ((pIntURB->actual_length == 8)
      &&  ((*(u64*)pIntURB->transfer_buffer & CDCEncRespMask) == CDCEncResp ) )

      {
         // Time to read
         usb_fill_control_urb( pDev->mQMIDev.mpReadURB,
                               pDev->mpNetDev->udev,
                               usb_rcvctrlpipe( pDev->mpNetDev->udev, 0 ),
                               (unsigned char *)pDev->mQMIDev.mpReadSetupPacket,
                               pDev->mQMIDev.mpReadBuffer,
                               DEFAULT_READ_URB_LENGTH,
                               ReadCallback,
                               pDev );
         setup_timer( &pDev->read_tmr, (void*)read_tmr_cb, (unsigned long)pDev->mQMIDev.mpReadURB );
         mod_timer( &pDev->read_tmr, jiffies + msecs_to_jiffies(USB_READ_TIMEOUT) );
         status = usb_submit_urb( pDev->mQMIDev.mpReadURB, GFP_ATOMIC );
         if (status != 0)
         {
            DBG( "Error submitting Read URB %d\n", status );
            if (IsDeviceValid( pDev ) == false)
            {
               DBG( "Invalid device!\n" );
               return;
            }
            // Resubmit the interrupt urb
            ResubmitIntURB( pIntURB );
            return;
         }

         // Int URB will be resubmitted during ReadCallback
         return;
      }
      // CDC CONNECTION_SPEED_CHANGE
      else if ((pIntURB->actual_length == 16)
           &&  (CDC_GET_CONNECTION_SPEED_CHANGE(&CDCEncResp, &CDCEncRespMask))
           &&  ((*(u64*)pIntURB->transfer_buffer & CDCEncRespMask) == CDCEncResp ) )
      {
         DBG( "IntCallback: Connection Speed Change = 0x%llx\n",
              (*(u64*)pIntURB->transfer_buffer));

         // if upstream or downstream is 0, stop traffic.  Otherwise resume it
         if ((*(u32*)(pIntURB->transfer_buffer + 8) == 0)
         ||  (*(u32*)(pIntURB->transfer_buffer + 12) == 0))
         {
            GobiSetDownReason( pDev, CDC_CONNECTION_SPEED );
            DBG( "traffic stopping due to CONNECTION_SPEED_CHANGE\n" );
         }
         else
         {
            GobiClearDownReason( pDev, CDC_CONNECTION_SPEED );
            DBG( "resuming traffic due to CONNECTION_SPEED_CHANGE\n" );
         }
      }
      else
      {
         DBG( "ignoring invalid interrupt in packet\n" );
         PrintHex( pIntURB->transfer_buffer, pIntURB->actual_length );
      }
   }
   if (IsDeviceValid( pDev ) == false)
   {
      DBG( "Invalid device!\n" );
      return;
   }
   // Resubmit the interrupt urb
   ResubmitIntURB( pIntURB );

   return;
}

/*===========================================================================
METHOD:
   StartRead (Public Method)

DESCRIPTION:
   Start continuous read "thread" (callback driven)

   Note: In case of error, KillRead() should be run
         to remove urbs and clean up memory.

PARAMETERS:
   pDev     [ I ] - Device specific memory

RETURN VALUE:
   int - 0 for success
         negative errno for failure
===========================================================================*/
int StartRead( sGobiUSBNet * pDev )
{
   int interval;
   struct usb_endpoint_descriptor *pendp;

   if (IsDeviceValid( pDev ) == false)
   {
      DBG( "Invalid device!\n" );
      return -ENXIO;
   }

   // Allocate URB buffers
   pDev->mQMIDev.mpReadURB = usb_alloc_urb( 0, GFP_KERNEL );
   if (pDev->mQMIDev.mpReadURB == NULL)
   {
      DBG( "Error allocating read urb\n" );
      return -ENOMEM;
   }

   pDev->mQMIDev.mpIntURB = usb_alloc_urb( 0, GFP_KERNEL );
   if (pDev->mQMIDev.mpIntURB == NULL)
   {
      DBG( "Error allocating int urb\n" );
      usb_free_urb( pDev->mQMIDev.mpReadURB );
      pDev->mQMIDev.mpReadURB = NULL;
      return -ENOMEM;
   }

   // Create data buffers
   pDev->mQMIDev.mpReadBuffer = kmalloc( DEFAULT_READ_URB_LENGTH, GFP_KERNEL );
   if (pDev->mQMIDev.mpReadBuffer == NULL)
   {
      DBG( "Error allocating read buffer\n" );
      usb_free_urb( pDev->mQMIDev.mpIntURB );
      pDev->mQMIDev.mpIntURB = NULL;
      usb_free_urb( pDev->mQMIDev.mpReadURB );
      pDev->mQMIDev.mpReadURB = NULL;
      return -ENOMEM;
   }

   pDev->mQMIDev.mpIntBuffer = kmalloc( DEFAULT_READ_URB_LENGTH, GFP_KERNEL );
   if (pDev->mQMIDev.mpIntBuffer == NULL)
   {
      DBG( "Error allocating int buffer\n" );
      kfree( pDev->mQMIDev.mpReadBuffer );
      pDev->mQMIDev.mpReadBuffer = NULL;
      usb_free_urb( pDev->mQMIDev.mpIntURB );
      pDev->mQMIDev.mpIntURB = NULL;
      usb_free_urb( pDev->mQMIDev.mpReadURB );
      pDev->mQMIDev.mpReadURB = NULL;
      return -ENOMEM;
   }

   pDev->mQMIDev.mpReadSetupPacket = kmalloc( sizeof( sURBSetupPacket ),
                                              GFP_KERNEL );
   if (pDev->mQMIDev.mpReadSetupPacket == NULL)
   {
      DBG( "Error allocating setup packet buffer\n" );
      kfree( pDev->mQMIDev.mpIntBuffer );
      pDev->mQMIDev.mpIntBuffer = NULL;
      kfree( pDev->mQMIDev.mpReadBuffer );
      pDev->mQMIDev.mpReadBuffer = NULL;
      usb_free_urb( pDev->mQMIDev.mpIntURB );
      pDev->mQMIDev.mpIntURB = NULL;
      usb_free_urb( pDev->mQMIDev.mpReadURB );
      pDev->mQMIDev.mpReadURB = NULL;
      return -ENOMEM;
   }

   // CDC Get Encapsulated Response packet
   pDev->mQMIDev.mpReadSetupPacket->mRequestType = 
       USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
   pDev->mQMIDev.mpReadSetupPacket->mRequestCode = GET_ENCAPSULATED_RESPONSE;
   pDev->mQMIDev.mpReadSetupPacket->mValue = 0;
   pDev->mQMIDev.mpReadSetupPacket->mIndex =
      cpu_to_le16(pDev->mpIntf->cur_altsetting->desc.bInterfaceNumber);  /* interface number */
   pDev->mQMIDev.mpReadSetupPacket->mLength = cpu_to_le16(DEFAULT_READ_URB_LENGTH);

   pendp = GetEndpoint(pDev->mpIntf, USB_ENDPOINT_XFER_INT, USB_DIR_IN);
   if (pendp == NULL)
   {
      DBG( "Invalid interrupt endpoint!\n" );
      kfree(pDev->mQMIDev.mpReadSetupPacket);
      pDev->mQMIDev.mpReadSetupPacket = NULL;
      kfree( pDev->mQMIDev.mpIntBuffer );
      pDev->mQMIDev.mpIntBuffer = NULL;
      kfree( pDev->mQMIDev.mpReadBuffer );
      pDev->mQMIDev.mpReadBuffer = NULL;
      usb_free_urb( pDev->mQMIDev.mpIntURB );
      pDev->mQMIDev.mpIntURB = NULL;
      usb_free_urb( pDev->mQMIDev.mpReadURB );
      pDev->mQMIDev.mpReadURB = NULL;
      return -ENXIO;
   }

   // Interval needs reset after every URB completion
   interval = (pDev->mpNetDev->udev->speed == USB_SPEED_HIGH) ?
                 6 : max((int)(pendp->bInterval), 3);

   // Schedule interrupt URB
   if (IsDeviceValid( pDev ) == false)
   {
      DBG( "Invalid device!\n" );
      return -ENXIO;
   }
   usb_fill_int_urb( pDev->mQMIDev.mpIntURB,
                     pDev->mpNetDev->udev,
                     /* QMI interrupt endpoint for the following
                      * interface configuration: DM, NMEA, MDM, NET
                      */
                     usb_rcvintpipe( pDev->mpNetDev->udev,
                                     pendp->bEndpointAddress),
                     pDev->mQMIDev.mpIntBuffer,
                     le16_to_cpu(pendp->wMaxPacketSize),
                     IntCallback,
                     pDev,
                     interval );
   if (IsDeviceValid( pDev ) == false)
   {
      DBG( "Invalid device!\n" );
      return -ENXIO;
   }
   return usb_submit_urb( pDev->mQMIDev.mpIntURB, GFP_KERNEL );
}

/*===========================================================================
METHOD:
   KillRead (Public Method)

DESCRIPTION:
   Kill continuous read "thread"

PARAMETERS:
   pDev     [ I ] - Device specific memory

RETURN VALUE:
   None
===========================================================================*/
void KillRead( sGobiUSBNet * pDev )
{

   if(pDev ==NULL)
   {
      DBG( "pDev NULL\n" );
      return ;
   }

   // Stop reading
   if (pDev->mQMIDev.mpReadURB != NULL)
   {
      DBG( "Killng read URB\n" );
      usb_kill_urb( pDev->mQMIDev.mpReadURB );
   }

   if (pDev->mQMIDev.mpIntURB != NULL)
   {
      DBG( "Killng int URB\n" );
      usb_kill_urb( pDev->mQMIDev.mpIntURB );
   }

   // Release buffers
   kfree( pDev->mQMIDev.mpReadSetupPacket );
   pDev->mQMIDev.mpReadSetupPacket = NULL;
   kfree( pDev->mQMIDev.mpReadBuffer );
   pDev->mQMIDev.mpReadBuffer = NULL;
   kfree( pDev->mQMIDev.mpIntBuffer );
   pDev->mQMIDev.mpIntBuffer = NULL;

   // Release URB's
   usb_free_urb( pDev->mQMIDev.mpReadURB );
   pDev->mQMIDev.mpReadURB = NULL;
   usb_free_urb( pDev->mQMIDev.mpIntURB );
   pDev->mQMIDev.mpIntURB = NULL;
}

/*=========================================================================*/
// Internal read/write functions
/*=========================================================================*/

/*===========================================================================
METHOD:
   ReadAsync (Public Method)

DESCRIPTION:
   Start asynchronous read
   NOTE: Reading client's data store, not device

PARAMETERS:
   pDev              [ I ] - Device specific memory
   clientID          [ I ] - Requester's client ID
   transactionID     [ I ] - Transaction ID or 0 for any
   pCallback         [ I ] - Callback to be executed when data is available
   pData             [ I ] - Data buffer that willl be passed (unmodified)
                             to callback

RETURN VALUE:
   int - 0 for success
         negative errno for failure
===========================================================================*/
int ReadAsync(
   sGobiUSBNet *    pDev,
   u16                clientID,
   u16                transactionID,
   void               (*pCallback)(sGobiUSBNet*, u16, void *),
   void *             pData )
{
   sClientMemList * pClientMem;
   sReadMemList ** ppReadMemList;

   unsigned long flags;

   if (IsDeviceValid( pDev ) == false)
   {
      DBG( "Invalid device!\n" );
      return -ENXIO;
   }

   // Critical section
   spin_lock_irqsave( &pDev->mQMIDev.mClientMemLock, flags );

   // Find memory storage for this client ID
   pClientMem = FindClientMem( pDev, clientID );
   if (pClientMem == NULL)
   {
      DBG( "Could not find matching client ID 0x%04X\n",
           clientID );

      // End critical section
      spin_unlock_irqrestore( &pDev->mQMIDev.mClientMemLock, flags );
      return -ENXIO;
   }

   ppReadMemList = &(pClientMem->mpList);

   // Does data already exist?
   while (*ppReadMemList != NULL)
   {
      // Is this element our data?
      if (transactionID == 0
      ||  transactionID == (*ppReadMemList)->mTransactionID)
      {
         // End critical section
         spin_unlock_irqrestore( &pDev->mQMIDev.mClientMemLock, flags );

         // Run our own callback
         pCallback( pDev, clientID, pData );

         return 0;
      }

      // Next
      ppReadMemList = &(*ppReadMemList)->mpNext;
   }

   // Data not found, add ourself to list of waiters
   if (AddToNotifyList( pDev,
                        clientID,
                        transactionID,
                        pCallback,
                        pData ) == false)
   {
      DBG( "Unable to register for notification\n" );
   }

   // End critical section
   spin_unlock_irqrestore( &pDev->mQMIDev.mClientMemLock, flags );

   // Success
   return 0;
}

/*===========================================================================
METHOD:
   UpSem (Public Method)

DESCRIPTION:
   Notification function for synchronous read

PARAMETERS:
   pDev              [ I ] - Device specific memory
   clientID          [ I ] - Requester's client ID
   pData             [ I ] - Buffer that holds semaphore to be up()-ed

RETURN VALUE:
   None
===========================================================================*/
void UpSem(
   sGobiUSBNet * pDev,
   u16             clientID,
   void *          pData )
{
   DBG( "0x%04X\n", clientID );

   up( (struct semaphore *)pData );
   return;
}

/*===========================================================================
METHOD:
   ReadSync (Public Method)

DESCRIPTION:
   Start synchronous read
   NOTE: Reading client's data store, not device

PARAMETERS:
   pDev              [ I ] - Device specific memory
   ppOutBuffer       [I/O] - On success, will be filled with a
                             pointer to read buffer
   clientID          [ I ] - Requester's client ID
   transactionID     [ I ] - Transaction ID or 0 for any

RETURN VALUE:
   int - size of data read for success
         negative errno for failure
===========================================================================*/
int ReadSync(
   sGobiUSBNet *    pDev,
   void **            ppOutBuffer,
   u16                clientID,
   u16                transactionID )
{
   int result;
   sClientMemList * pClientMem;
   sNotifyList ** ppNotifyList, * pDelNotifyListEntry;
   struct semaphore readSem;
   void * pData;
   unsigned long flags;
   u16 dataSize;

   if (IsDeviceValid( pDev ) == false)
   {
      DBG( "Invalid device!\n" );
      return -ENXIO;
   }

   // Critical section
   spin_lock_irqsave( &pDev->mQMIDev.mClientMemLock, flags );

   // Find memory storage for this Client ID
   pClientMem = FindClientMem( pDev, clientID );
   if (pClientMem == NULL)
   {
      DBG( "Could not find matching client ID 0x%04X\n",
           clientID );

      // End critical section
      spin_unlock_irqrestore( &pDev->mQMIDev.mClientMemLock, flags );
      return -ENXIO;
   }

   // Note: in cases where read is interrupted,
   //    this will verify client is still valid
   while (PopFromReadMemList( pDev,
                              clientID,
                              transactionID,
                              &pData,
                              &dataSize ) == false)
   {
      // Data does not yet exist, wait
      sema_init( &readSem, 0 );

      // Add ourself to list of waiters
      if (AddToNotifyList( pDev,
                           clientID,
                           transactionID,
                           UpSem,
                           &readSem ) == false)
      {
         DBG( "unable to register for notification\n" );
         spin_unlock_irqrestore( &pDev->mQMIDev.mClientMemLock, flags );
         return -EFAULT;
      }

      // End critical section while we block
      spin_unlock_irqrestore( &pDev->mQMIDev.mClientMemLock, flags );

      // Wait for notification
      result = down_timeout( &readSem,msecs_to_jiffies(5000));
      if (result != 0)
      {
         DBG( "Interrupted %d\n", result );

         // readSem will fall out of scope,
         // remove from notify list so it's not referenced
         spin_lock_irqsave( &pDev->mQMIDev.mClientMemLock, flags );
         ppNotifyList = &(pClientMem->mpReadNotifyList);
         pDelNotifyListEntry = NULL;

         // Find and delete matching entry
         while (*ppNotifyList != NULL)
         {
            if ((*ppNotifyList)->mpData == &readSem)
            {
               pDelNotifyListEntry = *ppNotifyList;
               *ppNotifyList = (*ppNotifyList)->mpNext;
               kfree( pDelNotifyListEntry );
               break;
            }

            // Next
            ppNotifyList = &(*ppNotifyList)->mpNext;
         }

         spin_unlock_irqrestore( &pDev->mQMIDev.mClientMemLock, flags );
         return -EINTR;
      }

      // Verify device is still valid
      if (IsDeviceValid( pDev ) == false)
      {
         DBG( "Invalid device!\n" );
         return -ENXIO;
      }

      // Restart critical section and continue loop
      spin_lock_irqsave( &pDev->mQMIDev.mClientMemLock, flags );
   }

   // End Critical section
   spin_unlock_irqrestore( &pDev->mQMIDev.mClientMemLock, flags );

   // Success
   *ppOutBuffer = pData;

   return dataSize;
}

/*===========================================================================
METHOD:
   WriteSync (Public Method)

DESCRIPTION:
   Start synchronous write

PARAMETERS:
   pDev                 [ I ] - Device specific memory
   pWriteBuffer         [ I ] - Data to be written
   writeBufferSize      [ I ] - Size of data to be written
   clientID             [ I ] - Client ID of requester

RETURN VALUE:
   int - write size (includes QMUX)
         negative errno for failure
===========================================================================*/
int WriteSync(
   sGobiUSBNet *          pDev,
   char *                 pWriteBuffer,
   int                    writeBufferSize,
   u16                    clientID )
{
   int i;
   int result;
   if (IsDeviceValid( pDev ) == false)
   {
      DBG( "Invalid device!\n" );
      return -ENXIO;
   }

   // Fill writeBuffer with QMUX
   result = FillQMUX( clientID, pWriteBuffer, writeBufferSize );
   if (result < 0)
   {
      return result;
   }

   // Wake device
   result = usb_autopm_get_interface( pDev->mpIntf );
   if (result < 0)
   {
      DBG( "unable to resume interface: %d\n", result );

      // Likely caused by device going from autosuspend -> full suspend
      if (result == -EPERM)
      {
#ifdef CONFIG_PM
#if (LINUX_VERSION_CODE < KERNEL_VERSION( 2,6,33 ))
         pDev->mpNetDev->udev->auto_pm = 0;
#endif
         GobiNetSuspend( pDev->mpIntf, PMSG_SUSPEND );
#endif /* CONFIG_PM */
      }
      return result;
   }

   DBG( "Actual Write:\n" );
   PrintHex( pWriteBuffer, writeBufferSize );

   // Write Control URB, protect with read semaphore to track in-flight USB control writes in case of disconnect

   for(i=0;i<USB_WRITE_RETRY;i++)
   {
       down_read(&pDev->shutdown_rwsem);
       result = usb_control_msg( pDev->mpNetDev->udev, usb_sndctrlpipe( pDev->mpNetDev->udev, 0 ),
             SEND_ENCAPSULATED_COMMAND,
             USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
             0, pDev->mpIntf->cur_altsetting->desc.bInterfaceNumber,
             (void*)pWriteBuffer, writeBufferSize,
             USB_WRITE_TIMEOUT );
       up_read(&pDev->shutdown_rwsem);
       if (result < 0)
       {
          printk(KERN_WARNING "usb_control_msg failed (%d)", result);
       }
       // Control write transfer may occasionally timeout with certain HCIs, attempt a second time before reporting an error
       if (result == -ETIMEDOUT)
       {
           pDev->writeTimeoutCnt++;
           printk(KERN_WARNING "Write URB timeout, cnt(%d)\n", pDev->writeTimeoutCnt);
       }
       else
       {
           break;
       }
   }

   // Write is done, release device
   usb_autopm_put_interface( pDev->mpIntf );


   return result;
}

/*=========================================================================*/
// Internal memory management functions
/*=========================================================================*/

/*===========================================================================
METHOD:
   GetClientID (Public Method)

DESCRIPTION:
   Request a QMI client for the input service type and initialize memory
   structure

PARAMETERS:
   pDev           [ I ] - Device specific memory
   serviceType    [ I ] - Desired QMI service type

RETURN VALUE:
   int - Client ID for success (positive)
         Negative errno for error
===========================================================================*/
int GetClientID(
   sGobiUSBNet *    pDev,
   u8                 serviceType )
{
   u16 clientID;
   sClientMemList ** ppClientMem;
   int result;
   void * pWriteBuffer;
   u16 writeBufferSize;
   void * pReadBuffer;
   u16 readBufferSize;
   unsigned long flags;
   u8 transactionID;

   if (IsDeviceValid( pDev ) == false)
   {
      DBG( "Invalid device!\n" );
      return -ENXIO;
   }

   // Run QMI request to be asigned a Client ID
   if (serviceType != 0)
   {
      writeBufferSize = QMICTLGetClientIDReqSize();
      pWriteBuffer = kmalloc( writeBufferSize, GFP_KERNEL );
      if (pWriteBuffer == NULL)
      {
         return -ENOMEM;
      }

      /* transactionID cannot be 0 */
      transactionID = atomic_add_return( 1, &pDev->mQMIDev.mQMICTLTransactionID );
      if (transactionID == 0)
      {
         transactionID = atomic_add_return( 1, &pDev->mQMIDev.mQMICTLTransactionID );
      }
      if (transactionID != 0)
      {
         result = QMICTLGetClientIDReq( pWriteBuffer,
                                        writeBufferSize,
                                        transactionID,
                                        serviceType );
         if (result < 0)
         {
            kfree( pWriteBuffer );
            return result;
         }
      }
      else
      {
         kfree( pWriteBuffer );
         DBG( "Invalid transaction ID!\n" );
         return EINVAL;
      }

      result = WriteSync( pDev,
                          pWriteBuffer,
                          writeBufferSize,
                          QMICTL );
      kfree( pWriteBuffer );

      if (result < 0)
      {
         return result;
      }

      result = ReadSync( pDev,
                         &pReadBuffer,
                         QMICTL,
                         transactionID );
      if (result < 0)
      {
         DBG( "bad read data %d\n", result );
         return result;
      }
      readBufferSize = result;

      result = QMICTLGetClientIDResp( pReadBuffer,
                                      readBufferSize,
                                      &clientID );

     /* Upon return from QMICTLGetClientIDResp, clientID
      * low address contains the Service Number (SN), and
      * clientID high address contains Client Number (CN)
      * For the ReadCallback to function correctly,we swap
      * the SN and CN on a Big Endian architecture.
      */
      clientID = le16_to_cpu(clientID);
      if(pReadBuffer)
      kfree( pReadBuffer );

      if (result < 0)
      {
         return result;
      }
   }
   else
   {
      // QMI CTL will always have client ID 0
      clientID = 0;
   }

   // Critical section
   spin_lock_irqsave( &pDev->mQMIDev.mClientMemLock, flags );

   // Verify client is not already allocated
   if (FindClientMem( pDev, clientID ) != NULL)
   {
      DBG( "Client memory already exists\n" );

      // End Critical section
      spin_unlock_irqrestore( &pDev->mQMIDev.mClientMemLock, flags );
      return -ETOOMANYREFS;
   }

   // Go to last entry in client mem list
   ppClientMem = &pDev->mQMIDev.mpClientMemList;
   while (*ppClientMem != NULL)
   {
      ppClientMem = &(*ppClientMem)->mpNext;
   }

   // Create locations for read to place data into
   *ppClientMem = kmalloc( sizeof( sClientMemList ), GFP_ATOMIC );
   if (*ppClientMem == NULL)
   {
      DBG( "Error allocating read list\n" );

      // End critical section
      spin_unlock_irqrestore( &pDev->mQMIDev.mClientMemLock, flags );
      return -ENOMEM;
   }

   (*ppClientMem)->mClientID = clientID;
   (*ppClientMem)->mpList = NULL;
   (*ppClientMem)->mpReadNotifyList = NULL;
   (*ppClientMem)->mpURBList = NULL;
   (*ppClientMem)->mpNext = NULL;

   // Initialize workqueue for poll()
   init_waitqueue_head( &(*ppClientMem)->mWaitQueue );

   // End Critical section
   spin_unlock_irqrestore( &pDev->mQMIDev.mClientMemLock, flags );

   return (int)( (*ppClientMem)->mClientID );
}

/*===========================================================================
METHOD:
   ReleaseClientID (Public Method)

DESCRIPTION:
   Release QMI client and free memory

PARAMETERS:
   pDev           [ I ] - Device specific memory
   clientID       [ I ] - Requester's client ID

RETURN VALUE:
   None
===========================================================================*/
bool ReleaseClientID(
   sGobiUSBNet *    pDev,
   u16                clientID )
{
   int result;
   sClientMemList ** ppDelClientMem;
   sClientMemList * pNextClientMem;
   void * pDelData;
   u16 dataSize;
   void * pWriteBuffer;
   u16 writeBufferSize;
   void * pReadBuffer;
   u16 readBufferSize;
   unsigned long flags;
   u8 transactionID;

   // Is device is still valid?
   if (IsDeviceValid( pDev ) == false)
   {
      DBG( "invalid device\n" );
      return false;
   }

   DBG( "releasing 0x%04X\n", clientID );

   // Check if a WriteSync() is already in progress and wait for it to complete
   down_write(&pDev->shutdown_rwsem);
   up_write(&pDev->shutdown_rwsem);

   // Run QMI ReleaseClientID if this isn't QMICTL
   if (clientID != QMICTL)
   {
      // Note: all errors are non fatal, as we always want to delete
      //    client memory in latter part of function

      writeBufferSize = QMICTLReleaseClientIDReqSize();
      pWriteBuffer = kmalloc( writeBufferSize, GFP_KERNEL );
      if (pWriteBuffer == NULL)
      {
         DBG( "memory error\n" );
         return false;
      }
      else
      {
         transactionID = atomic_add_return( 1, &pDev->mQMIDev.mQMICTLTransactionID );
         if (transactionID == 0)
         {
            transactionID = atomic_add_return( 1, &pDev->mQMIDev.mQMICTLTransactionID );
         }
         result = QMICTLReleaseClientIDReq( pWriteBuffer,
                                            writeBufferSize,
                                            transactionID,
                                            clientID );
         if (result < 0)
         {
            kfree( pWriteBuffer );
            DBG( "error %d filling req buffer\n", result );
         }
         else
         {
            result = WriteSync( pDev,
                                pWriteBuffer,
                                writeBufferSize,
                                QMICTL );
            kfree( pWriteBuffer );

            if (result < 0)
            {
               DBG( "bad write status %d\n", result );
               return false;
            }
            else
            {
               result = ReadSync( pDev,
                                  &pReadBuffer,
                                  QMICTL,
                                  transactionID );
               if (result < 0)
               {
                  DBG( "bad read status %d\n", result );
                  return false;
               }
               else
               {
                  readBufferSize = result;

                  result = QMICTLReleaseClientIDResp( pReadBuffer,
                                                      readBufferSize );
                  if(pReadBuffer)
                  kfree( pReadBuffer );

                  if (result < 0)
                  {
                     DBG( "error %d parsing response\n", result );
                     return false;
                  }
               }
            }
         }
      }
   }

   // Cleaning up client memory

   // Critical section
   spin_lock_irqsave( &pDev->mQMIDev.mClientMemLock, flags );

   // Can't use FindClientMem, I need to keep pointer of previous
   ppDelClientMem = &pDev->mQMIDev.mpClientMemList;
   while (*ppDelClientMem != NULL)
   {
      if ((*ppDelClientMem)->mClientID == clientID)
      {
         pNextClientMem = (*ppDelClientMem)->mpNext;

         // Notify all clients
         while (NotifyAndPopNotifyList( pDev,
                                        clientID,
                                        0 ) == true );

         // Free any unread data
         while (PopFromReadMemList( pDev,
                                    clientID,
                                    0,
                                    &pDelData,
                                    &dataSize ) == true )
         {
            kfree( pDelData );
         }
         DBG("Delete client Mem\r\n");
         // Delete client Mem
         kfree( *ppDelClientMem );

         // Overwrite the pointer that was to this client mem
         *ppDelClientMem = pNextClientMem;
      }
      else
      {
         // I now point to (a pointer of ((the node I was at)'s mpNext))
          if(*ppDelClientMem==NULL)
          {
              DBG("ppDelClientMem NULL %d\r\n",__LINE__);
              break;
          }
         ppDelClientMem = &(*ppDelClientMem)->mpNext;
      }
   }

   // End Critical section
   spin_unlock_irqrestore( &pDev->mQMIDev.mClientMemLock, flags );
   return true;
}

/*===========================================================================
METHOD:
   FindClientMem (Public Method)

DESCRIPTION:
   Find this client's memory

   Caller MUST have lock on mClientMemLock

PARAMETERS:
   pDev           [ I ] - Device specific memory
   clientID       [ I ] - Requester's client ID

RETURN VALUE:
   sClientMemList - Pointer to requested sClientMemList for success
                    NULL for error
===========================================================================*/
sClientMemList * FindClientMem(
   sGobiUSBNet *      pDev,
   u16              clientID )
{
   sClientMemList * pClientMem;

   if (IsDeviceValid( pDev ) == false)
   {
      DBG( "Invalid device\n" );
      return NULL;
   }

#ifdef CONFIG_SMP
   // Verify Lock
   if (spin_is_locked( &pDev->mQMIDev.mClientMemLock ) == 0)
   {
      DBG( "unlocked\n" );
      BUG();
   }
#endif

   pClientMem = pDev->mQMIDev.mpClientMemList;
   while (pClientMem != NULL)
   {
      if (pClientMem->mClientID == clientID)
      {
         // Success
         DBG("Found client's 0x%x memory\n", clientID);
         return pClientMem;
      }

      pClientMem = pClientMem->mpNext;
   }

   DBG( "Could not find client mem 0x%04X\n", clientID );
   return NULL;
}

/*===========================================================================
METHOD:
   AddToReadMemList (Public Method)

DESCRIPTION:
   Add Data to this client's ReadMem list

   Caller MUST have lock on mClientMemLock

PARAMETERS:
   pDev           [ I ] - Device specific memory
   clientID       [ I ] - Requester's client ID
   transactionID  [ I ] - Transaction ID or 0 for any
   pData          [ I ] - Data to add
   dataSize       [ I ] - Size of data to add

RETURN VALUE:
   bool
===========================================================================*/
bool AddToReadMemList(
   sGobiUSBNet *      pDev,
   u16              clientID,
   u16              transactionID,
   void *           pData,
   u16              dataSize )
{
   sClientMemList * pClientMem;
   sReadMemList ** ppThisReadMemList;

#ifdef CONFIG_SMP
   // Verify Lock
   if (spin_is_locked( &pDev->mQMIDev.mClientMemLock ) == 0)
   {
      DBG( "unlocked\n" );
      BUG();
   }
#endif

   // Get this client's memory location
   pClientMem = FindClientMem( pDev, clientID );
   if (pClientMem == NULL)
   {
      DBG( "Could not find this client's memory 0x%04X\n",
           clientID );

      return false;
   }

   // Go to last ReadMemList entry
   ppThisReadMemList = &pClientMem->mpList;
   while (*ppThisReadMemList != NULL)
   {
      ppThisReadMemList = &(*ppThisReadMemList)->mpNext;
   }

   *ppThisReadMemList = kmalloc( sizeof( sReadMemList ), GFP_ATOMIC );
   if (*ppThisReadMemList == NULL)
   {
      DBG( "Mem error\n" );

      return false;
   }

   (*ppThisReadMemList)->mpNext = NULL;
   (*ppThisReadMemList)->mpData = pData;
   (*ppThisReadMemList)->mDataSize = dataSize;
   (*ppThisReadMemList)->mTransactionID = transactionID;

   return true;
}

/*===========================================================================
METHOD:
   PopFromReadMemList (Public Method)

DESCRIPTION:
   Remove data from this client's ReadMem list if it matches
   the specified transaction ID.

   Caller MUST have lock on mClientMemLock

PARAMETERS:
   pDev              [ I ] - Device specific memory
   clientID          [ I ] - Requester's client ID
   transactionID     [ I ] - Transaction ID or 0 for any
   ppData            [I/O] - On success, will be filled with a
                             pointer to read buffer
   pDataSize         [I/O] - On succces, will be filled with the
                             read buffer's size

RETURN VALUE:
   bool
===========================================================================*/
bool PopFromReadMemList(
   sGobiUSBNet *      pDev,
   u16              clientID,
   u16              transactionID,
   void **          ppData,
   u16 *            pDataSize )
{
   sClientMemList * pClientMem;
   sReadMemList * pDelReadMemList, ** ppReadMemList;
   DBG("");
#ifdef CONFIG_SMP
   // Verify Lock
   if (spin_is_locked( &pDev->mQMIDev.mClientMemLock ) == 0)
   {
      DBG( "unlocked\n" );
      BUG();
   }
#endif

   // Get this client's memory location
   pClientMem = FindClientMem( pDev, clientID );
   if (pClientMem == NULL)
   {
      DBG( "Could not find this client's memory 0x%04X\n",
           clientID );

      return false;
   }

   ppReadMemList = &(pClientMem->mpList);
   pDelReadMemList = NULL;

   // Find first message that matches this transaction ID
   CLIENT_READMEM_SNAPSHOT(clientID, pDev);
   while (*ppReadMemList != NULL)
   {
      // Do we care about transaction ID?
      if (transactionID == 0
      ||  transactionID == (*ppReadMemList)->mTransactionID )
      {
         pDelReadMemList = *ppReadMemList;
         DBG(  "*ppReadMemList = 0x%p pDelReadMemList = 0x%p\n",
               *ppReadMemList, pDelReadMemList );
         break;
      }

      DBG( "skipping 0x%04X data TID = %x\n", clientID, (*ppReadMemList)->mTransactionID );

      // Next
      ppReadMemList = &(*ppReadMemList)->mpNext;
   }
   DBG(  "*ppReadMemList = 0x%p pDelReadMemList = 0x%p\n",
         *ppReadMemList, pDelReadMemList );
   if (pDelReadMemList != NULL)
   {
       if(*ppReadMemList==NULL)
       {
           DBG("%d\r\n",__LINE__);
           return false;
       }
      *ppReadMemList = (*ppReadMemList)->mpNext;

      // Copy to output
      *ppData = pDelReadMemList->mpData;
      *pDataSize = pDelReadMemList->mDataSize;
      DBG(  "*ppData = 0x%p pDataSize = %u\n",
            *ppData, *pDataSize );

      // Free memory
      kfree( pDelReadMemList );

      return true;
   }
   else
   {
      DBG( "No read memory to pop, Client 0x%04X, TID = %x\n",
           clientID,
           transactionID );
      return false;
   }
}

/*===========================================================================
METHOD:
   AddToNotifyList (Public Method)

DESCRIPTION:
   Add Notify entry to this client's notify List

   Caller MUST have lock on mClientMemLock

PARAMETERS:
   pDev              [ I ] - Device specific memory
   clientID          [ I ] - Requester's client ID
   transactionID     [ I ] - Transaction ID or 0 for any
   pNotifyFunct      [ I ] - Callback function to be run when data is available
   pData             [ I ] - Data buffer that willl be passed (unmodified)
                             to callback

RETURN VALUE:
   bool
===========================================================================*/
bool AddToNotifyList(
   sGobiUSBNet *      pDev,
   u16              clientID,
   u16              transactionID,
   void             (* pNotifyFunct)(sGobiUSBNet *, u16, void *),
   void *           pData )
{
   sClientMemList * pClientMem;
   sNotifyList ** ppThisNotifyList;
   DBG("");
#ifdef CONFIG_SMP
   // Verify Lock
   if (spin_is_locked( &pDev->mQMIDev.mClientMemLock ) == 0)
   {
      DBG( "unlocked\n" );
      BUG();
   }
#endif

   // Get this client's memory location
   pClientMem = FindClientMem( pDev, clientID );
   if (pClientMem == NULL)
   {
      DBG( "Could not find this client's memory 0x%04X\n", clientID );
      return false;
   }

   // Go to last URBList entry
   ppThisNotifyList = &pClientMem->mpReadNotifyList;
   while (*ppThisNotifyList != NULL)
   {
      ppThisNotifyList = &(*ppThisNotifyList)->mpNext;
   }

   *ppThisNotifyList = kmalloc( sizeof( sNotifyList ), GFP_ATOMIC );
   if (*ppThisNotifyList == NULL)
   {
      DBG( "Mem error\n" );
      return false;
   }

   (*ppThisNotifyList)->mpNext = NULL;
   (*ppThisNotifyList)->mpNotifyFunct = pNotifyFunct;
   (*ppThisNotifyList)->mpData = pData;
   (*ppThisNotifyList)->mTransactionID = transactionID;

   return true;
}

/*===========================================================================
METHOD:
   NotifyAndPopNotifyList (Public Method)

DESCRIPTION:
   Remove first Notify entry from this client's notify list
   and Run function

   Caller MUST have lock on mClientMemLock

PARAMETERS:
   pDev              [ I ] - Device specific memory
   clientID          [ I ] - Requester's client ID
   transactionID     [ I ] - Transaction ID or 0 for any

RETURN VALUE:
   bool
===========================================================================*/
bool NotifyAndPopNotifyList(
   sGobiUSBNet *      pDev,
   u16              clientID,
   u16              transactionID )
{
   sClientMemList * pClientMem;
   sNotifyList * pDelNotifyList, ** ppNotifyList;

#ifdef CONFIG_SMP
   // Verify Lock
   if (spin_is_locked( &pDev->mQMIDev.mClientMemLock ) == 0)
   {
      DBG( "unlocked\n" );
      BUG();
   }
#endif

   // Get this client's memory location
   pClientMem = FindClientMem( pDev, clientID );
   if (pClientMem == NULL)
   {
      DBG( "Could not find this client's memory 0x%04X\n", clientID );
      return false;
   }

   ppNotifyList = &(pClientMem->mpReadNotifyList);
   pDelNotifyList = NULL;

   // Remove from list
   CLIENT_READMEM_SNAPSHOT(clientID,pDev);
   while (*ppNotifyList != NULL)
   {
      // Do we care about transaction ID?
      if (transactionID == 0
      ||  (*ppNotifyList)->mTransactionID == 0
      ||  transactionID == (*ppNotifyList)->mTransactionID)
      {
         pDelNotifyList = *ppNotifyList;
         break;
      }

      DBG( "skipping data TID = %x\n", (*ppNotifyList)->mTransactionID );

      // next
      ppNotifyList = &(*ppNotifyList)->mpNext;
   }

   if (pDelNotifyList != NULL)
   {
      // Remove element
      *ppNotifyList = (*ppNotifyList)->mpNext;

      // Run notification function
      if (pDelNotifyList->mpNotifyFunct != NULL)
      {
         // Unlock for callback
         spin_unlock( &pDev->mQMIDev.mClientMemLock );

         pDelNotifyList->mpNotifyFunct( pDev,
                                        clientID,
                                        pDelNotifyList->mpData );

         // Restore lock
         spin_lock( &pDev->mQMIDev.mClientMemLock );
      }

      // Delete memory
      kfree( pDelNotifyList );

      return true;
   }
   else
   {
      DBG( "no one to notify for TID %x\n", transactionID );

      return false;
   }
}

/*=========================================================================*/
// Internal userspace wrappers
/*=========================================================================*/

/*===========================================================================
METHOD:
   UserspaceunlockedIOCTL (Public Method)

DESCRIPTION:
   Internal wrapper for Userspace IOCTL interface

PARAMETERS
   pFilp        [ I ] - userspace file descriptor
   cmd          [ I ] - IOCTL command
   arg          [ I ] - IOCTL argument

RETURN VALUE:
   long - 0 for success
          Negative errno for failure
===========================================================================*/
long UserspaceunlockedIOCTL(
   struct file *     pFilp,
   unsigned int      cmd,
   unsigned long     arg ) 
{
   int j;
   int result;
   u32 devVIDPID;

   sQMIFilpStorage * pFilpData = (sQMIFilpStorage *)pFilp->private_data;

   if (pFilpData == NULL)
   {
      DBG( "Bad file data\n" );
      return -EBADF;
   }

   if (IsDeviceValid( pFilpData->mpDev ) == false)
   {
      DBG( "Invalid device! Updating f_ops\n" );
      pFilp->f_op = pFilp->f_dentry->d_inode->i_fop;
      return -ENXIO;
   }

   switch (cmd)
   {
      case IOCTL_QMI_GET_SERVICE_FILE:
         DBG( "Setting up QMI for service %lu\n", arg );
         if ((u8)arg == 0)
         {
            DBG( "Cannot use QMICTL from userspace\n" );
            return -EINVAL;
         }

         // Connection is already setup
         if (pFilpData->mClientID != (u16)-1)
         {
            DBG( "Close the current connection before opening a new one\n" );
            return -EBADR;
         }

         result = GetClientID( pFilpData->mpDev, (u8)arg );
         if (result < 0)
         {
            return result;
         }
         pFilpData->mClientID = (u16)result;
         DBG("pFilpData->mClientID = 0x%x\n", pFilpData->mClientID );
         return 0;
         break;


      case IOCTL_QMI_GET_DEVICE_VIDPID:
         if (arg == 0)
         {
            DBG( "Bad VIDPID buffer\n" );
            return -EINVAL;
         }

         // Extra verification
         if (pFilpData->mpDev->mpNetDev == 0)
         {
            DBG( "Bad mpNetDev\n" );
            return -ENOMEM;
         }
         if (pFilpData->mpDev->mpNetDev->udev == 0)
         {
            DBG( "Bad udev\n" );
            return -ENOMEM;
         }

         devVIDPID = ((le16_to_cpu( pFilpData->mpDev->mpNetDev->udev->descriptor.idVendor ) << 16)
                     + le16_to_cpu( pFilpData->mpDev->mpNetDev->udev->descriptor.idProduct ) );

         result = copy_to_user( (unsigned int *)arg, &devVIDPID, 4 );
         if (result != 0)
         {
            DBG( "Copy to userspace failure %d\n", result );
         }

         return result;

         break;

      case IOCTL_QMI_GET_DEVICE_MEID:
         if (arg == 0)
         {
            DBG( "Bad MEID buffer\n" );
            return -EINVAL;
         }

         result = copy_to_user( (unsigned int *)arg, &pFilpData->mpDev->mMEID[0], 14 );
         if (result != 0)
         {
            DBG( "Copy to userspace failure %d\n", result );
         }

         return result;

         break;

      case IOCTL_QMI_ADD_MAPPING:
         {
             sGobiUSBNet * pDev = pFilpData->mpDev;
             sMapping *pmap = (sMapping*) arg;

             DBG( "add mapping\n" );
             if (arg == 0)
             {
                DBG( "null pointer\n" );
                return -EINVAL;
             }
             DBG( "dscp, qos_id: 0x%x, 0x%x\n", pmap->dscp, pmap->qosId );

             if ((MAX_DSCP_ID < pmap->dscp) && (UNIQUE_DSCP_ID != pmap->dscp))
             {
                 DBG( "Invalid DSCP value\n" );
                 return -EINVAL;
             }

             //check for existing map
             for (j=0;j<MAX_MAP;j++)
             {
                 if (pDev->maps.table[j].dscp == pmap->dscp)
                 {
                     DBG("mapping already exists at slot #%d\n", j);
                     return -EINVAL;
                 }
             }

             //check if this is a request to redirect all IP traffic to default bearer
             if (UNIQUE_DSCP_ID == pmap->dscp)
             {
                 DBG("set slot (%d) to indicate IP packet redirection is needed\n", MAX_MAP-1);
                 pDev->maps.table[MAX_MAP-1].dscp = UNIQUE_DSCP_ID;
                 pDev->maps.table[MAX_MAP-1].qosId = pmap->qosId;
                 pDev->maps.count++; 
                 return 0;
             }

             //find free slot to hold new mapping
             for(j=0;j<MAX_MAP-1;j++)
             {
                 if (pDev->maps.table[j].dscp == 0xff)
                 {
                     pDev->maps.table[j].dscp = pmap->dscp;
                     pDev->maps.table[j].qosId = pmap->qosId;
                     pDev->maps.count++; 
                     return 0;
                 }
             }

             DBG("no free mapping slot\n");
             return -ENOMEM;
         }
         break;

      case IOCTL_QMI_EDIT_MAPPING:
         {
             sGobiUSBNet * pDev = pFilpData->mpDev;

             sMapping *pmap = (sMapping*) arg;
             DBG( "edit mapping\n" );
             if (arg == 0)
             {
                DBG( "null pointer\n" );
                return -EINVAL;
             }
             DBG( "dscp, qos_id: 0x%x, 0x%x\n", pmap->dscp, pmap->qosId );

             if ((MAX_DSCP_ID < pmap->dscp) && (UNIQUE_DSCP_ID != pmap->dscp))
             {
                 DBG( "Invalid DSCP value\n" );
                 return -EINVAL;
             }

             for(j=0;j<MAX_MAP;j++)
             {
                 if (pDev->maps.table[j].dscp == pmap->dscp)
                 {
                     pDev->maps.table[j].qosId = pmap->qosId;
                     return 0;
                 }
             }

             DBG("no matching tos for edit mapping\n");
             return -ENOMEM;
         }
         break;

      case IOCTL_QMI_READ_MAPPING:
         {
             sGobiUSBNet * pDev = pFilpData->mpDev;

             sMapping *pmap = (sMapping*) arg;
             DBG( "read mapping\n" );
             if (arg == 0)
             {
                DBG( "null pointer\n" );
                return -EINVAL;
             }

             if ((MAX_DSCP_ID < pmap->dscp) && (UNIQUE_DSCP_ID != pmap->dscp))
             {
                 DBG( "Invalid DSCP value\n" );
                 return -EINVAL;
             }

             for(j=0;j<MAX_MAP;j++)
             {
                 if (pDev->maps.table[j].dscp == pmap->dscp)
                 {
                     pmap->qosId = pDev->maps.table[j].qosId;
                     DBG( "dscp, qos_id: 0x%x, 0x%x\n", pmap->dscp, pmap->qosId );

                     result = copy_to_user( (unsigned int *)arg, &pDev->maps.table[j], sizeof(sMapping));
                     if (result != 0)
                     {
                         DBG( "Copy to userspace failure %d\n", result );
                     }

                     return result;
                 }
             }

             DBG("no matching tos for read mapping\n");
             return -ENOMEM;
         }
         break;

      case IOCTL_QMI_DEL_MAPPING:
         {
             sGobiUSBNet * pDev = pFilpData->mpDev;
             sMapping *pmap = (sMapping*) arg;
             DBG( "Delete mapping\n" );
             if (arg == 0)
             {
                 DBG( "null pointer\n" );
                 return -EINVAL;
             }
             DBG( "DSCP 0x%x\n", pmap->dscp );

             if ((MAX_DSCP_ID < pmap->dscp) && (UNIQUE_DSCP_ID != pmap->dscp))
             {
                 DBG( "Invalid DSCP value\n" );
                 return -EINVAL;
             }

             for(j=0;j<MAX_MAP;j++)
             {
                 if (pDev->maps.table[j].dscp == pmap->dscp)
                 {
                     // delete mapping table entry
                     memset(&pDev->maps.table[j], 0xff, sizeof(pDev->maps.table[0]));
                     if (pDev->maps.count) pDev->maps.count--; 
                     return 0;
                 }
             }

             DBG("no matching mapping slot\n");
             return -ENOMEM;
         }
         break;

      case IOCTL_QMI_CLR_MAPPING:
         {
             sGobiUSBNet * pDev = pFilpData->mpDev;
             DBG( "Clear mapping\n" );
             memset(pDev->maps.table, 0xff, sizeof(pDev->maps.table));
             pDev->maps.count = 0; 
             return 0;
         }
         break;

#ifdef QOS_SIMULATE
      case IOCTL_QMI_QOS_SIMULATE:
         {
             int result;
             u8 supported = (u8)-1;
             DBG( "simulate indication\n" );
             u8 qos_support_ind[] = {
                 0x01,0x15,0x00,0x80,0x04,0xFF,0x04,0x00,0x00,
                 0x27,0x00,0x09,0x00,0x01,0x01,0x00,0x01,0x10,0x02,0x00,0x01,0x80
             };
             u8 qos_flow_activate_ind[] = {
                 0x01,0x15,0x00,0x80,0x04,0xFF,0x04,0x00,0x00,
                 0x26,0x00,0x09,0x00,0x01,0x06,0x00,0xDD,0xCC,0xBB,0xAA,0x01,0x01
             };
             u8 qos_flow_suspend_ind[] = {
                 0x01,0x15,0x00,0x80,0x04,0xFF,0x04,0x00,0x00,
                 0x26,0x00,0x09,0x00,0x01,0x06,0x00,0xDD,0xCC,0xBB,0xAA,0x02,0x02
             };
             u8 qos_flow_gone_ind[] = {
                 0x01,0x15,0x00,0x80,0x04,0xFF,0x04,0x00,0x00,
                 0x26,0x00,0x09,0x00,0x01,0x06,0x00,0xDD,0xCC,0xBB,0xAA,0x03,0x03
             };
             result = QMIQOSEventResp( qos_support_ind,
                     sizeof(qos_support_ind));
             result = QMIQOSEventResp( qos_flow_activate_ind,
                     sizeof(qos_flow_activate_ind));
             result = QMIQOSEventResp( qos_flow_suspend_ind,
                     sizeof(qos_flow_suspend_ind));
             result = QMIQOSEventResp( qos_flow_gone_ind,
                     sizeof(qos_flow_gone_ind));
             return 0;
         }
         break;
#endif

      case IOCTL_QMI_GET_TX_Q_LEN:
         {

             sGobiUSBNet * pDev = pFilpData->mpDev;

             if (arg == 0)
             {
                 DBG( "Bad Tx Queue buffer\n" );
                 return -EINVAL;
             }

             // Extra verification
             if (pFilpData->mpDev->mpNetDev == 0)
             {
                 DBG( "Bad mpNetDev\n" );
                 return -ENOMEM;
             }
             if (pFilpData->mpDev->mpNetDev->udev == 0)
             {
                 DBG( "Bad udev\n" );
                 return -ENOMEM;
             }

             result = copy_to_user( (unsigned int *)arg, &pDev->tx_qlen, sizeof(pDev->tx_qlen) );
             if (result != 0)
             {
                 DBG( "Copy to userspace failure %d\n", result );
             }

             return result;
         }

         break;

      case IOCTL_QMI_DUMP_MAPPING:
         {
             sGobiUSBNet * pDev = pFilpData->mpDev;

             DBG( "dump mapping\n" );
             if (arg == 0)
             {
                DBG( "null pointer\n" );
                return -EINVAL;
             }

             result = copy_to_user( (unsigned int *)arg, &pDev->maps.table[0], sizeof(pDev->maps.table));
             if (result != 0)
             {
                 DBG( "Copy to userspace failure %d\n", result );
             }
             return result;
         }

      case IOCTL_QMI_GET_USBNET_STATS:
         {
             sGobiUSBNet * pDev = pFilpData->mpDev;
             struct net_device_stats * pStats = &(pDev->mpNetDev->net->stats);
             sNetStats netStats;

             if (arg == 0)
             {
                 DBG( "Bad usbnet statistic buffer\n" );
                 return -EINVAL;
             }

             // Extra verification
             if (pFilpData->mpDev->mpNetDev == 0)
             {
                 DBG( "Bad mpNetDev\n" );
                 return -ENOMEM;
             }

             /* copy the value from struct net_device_stats to struct sNetStats */
             netStats.rx_packets = pStats->rx_packets;
             netStats.tx_packets = pStats->tx_packets;
             netStats.rx_bytes = pStats->rx_bytes;
             netStats.tx_bytes = pStats->tx_bytes;
             netStats.rx_errors = pStats->rx_errors;
             netStats.tx_errors = pStats->tx_errors;
             netStats.rx_overflows = pStats->rx_fifo_errors;
             netStats.tx_overflows = pStats->tx_fifo_errors;

             result = copy_to_user( (unsigned int *)arg, &netStats, sizeof(sNetStats) );
             if (result != 0)
             {
                 DBG( "Copy to userspace failure %d\n", result );
             }

             return result;
         }

         break;
         case IOCTL_QMI_SET_DEVICE_MTU:
         {
             sGobiUSBNet *pDev = pFilpData->mpDev;
             // struct usbnet * pNet = netdev_priv( pDev->mpNetDev->net );
             int iArgp = (int)arg;
             if (iArgp <= 0)
             {
                 DBG( "Bad MTU buffer\n" );
                 return -EINVAL;
             }
             DBG( "new mtu :%d ,qcqmi:%d\n",iArgp,(int)pDev->mQMIDev.qcqmi );
             pDev->mtu = iArgp;
             usbnet_change_mtu(pDev->mpNetDev->net ,pDev->mtu);
         }
         return 0;
      default:
         return -EBADRQC;
   }
}

/*=========================================================================*/
// Userspace wrappers
/*=========================================================================*/

/*===========================================================================
METHOD:
   UserspaceOpen (Public Method)

DESCRIPTION:
   Userspace open
      IOCTL must be called before reads or writes

PARAMETERS
   pInode       [ I ] - kernel file descriptor
   pFilp        [ I ] - userspace file descriptor

RETURN VALUE:
   int - 0 for success
         Negative errno for failure
===========================================================================*/
int UserspaceOpen(
   struct inode *         pInode,
   struct file *          pFilp )
{
   sQMIFilpStorage * pFilpData;

   // Optain device pointer from pInode
   sQMIDev * pQMIDev = container_of( pInode->i_cdev,
                                     sQMIDev,
                                     mCdev );
   sGobiUSBNet * pDev = container_of( pQMIDev,
                                    sGobiUSBNet,
                                    mQMIDev );

   if (IsDeviceValid( pDev ) == false)
   {
      DBG( "Invalid device\n" );
      return -ENXIO;
   }

   // Setup data in pFilp->private_data
   pFilp->private_data = kmalloc( sizeof( sQMIFilpStorage ), GFP_KERNEL );
   if (pFilp->private_data == NULL)
   {
      DBG( "Mem error\n" );
      return -ENOMEM;
   }

   pFilpData = (sQMIFilpStorage *)pFilp->private_data;
   pFilpData->mClientID = (u16)-1;
   pFilpData->mpDev = pDev;

   return 0;
}

/*===========================================================================
METHOD:
   UserspaceIOCTL (Public Method)

DESCRIPTION:
   Userspace IOCTL functions

PARAMETERS
   pUnusedInode [ I ] - (unused) kernel file descriptor
   pFilp        [ I ] - userspace file descriptor
   cmd          [ I ] - IOCTL command
   arg          [ I ] - IOCTL argument

RETURN VALUE:
   int - 0 for success
         Negative errno for failure
===========================================================================*/
int UserspaceIOCTL(
   struct inode *    pUnusedInode,
   struct file *     pFilp,
   unsigned int      cmd,
   unsigned long     arg ) 
{
   // call the internal wrapper function
   return (int)UserspaceunlockedIOCTL( pFilp, cmd, arg );  
}

/*===========================================================================
METHOD:
   UserspaceClose (Public Method)

DESCRIPTION:
   Userspace close
      Release client ID and free memory

PARAMETERS
   pFilp           [ I ] - userspace file descriptor
   unusedFileTable [ I ] - (unused) file table

RETURN VALUE:
   int - 0 for success
         Negative errno for failure
===========================================================================*/
int UserspaceClose(
   struct file *       pFilp,
   fl_owner_t          unusedFileTable )
{
   sQMIFilpStorage * pFilpData = (sQMIFilpStorage *)pFilp->private_data;
   struct task_struct * pEachTask;
   struct fdtable * pFDT;
   int count = 0;
   int used = 0;
   unsigned long flags;

   if (pFilpData == NULL)
   {
      DBG( "bad file data\n" );
      return -EBADF;
   }

   // Fallthough.  If f_count == 1 no need to do more checks
   if (atomic_long_read( &pFilp->f_count ) != 1)
   {
      rcu_read_lock();
      for_each_process( pEachTask )
      {
         if (pEachTask == NULL || pEachTask->files == NULL)
         {
            // Some tasks may not have files (e.g. Xsession)
            continue;
         }
         spin_lock_irqsave( &pEachTask->files->file_lock, flags );
         pFDT = files_fdtable( pEachTask->files );
         for (count = 0; count < pFDT->max_fds; count++)
         {
            // Before this function was called, this file was removed
            // from our task's file table so if we find it in a file
            // table then it is being used by another task
            if (pFDT->fd[count] == pFilp)
            {
               used++;
               break;
            }
         }
         spin_unlock_irqrestore( &pEachTask->files->file_lock, flags );
      }
      rcu_read_unlock();

      if (used > 0)
      {
         DBG( "not closing, as this FD is open by %d other process\n", used );
         return 0;
      }
   }

   if (IsDeviceValid( pFilpData->mpDev ) == false)
   {
      DBG( "Invalid device! Updating f_ops\n" );
      pFilp->f_op = pFilp->f_dentry->d_inode->i_fop;
      return -ENXIO;
   }

   DBG( "0x%04X\n", pFilpData->mClientID );

   // Disable pFilpData so they can't keep sending read or write
   //    should this function hang
   // Note: memory pointer is still saved in pFilpData to be deleted later
   pFilp->private_data = NULL;

   if (pFilpData->mClientID != (u16)-1)
   {
      ReleaseClientID( pFilpData->mpDev,
                       pFilpData->mClientID );
   }

   kfree( pFilpData );
   return 0;
}

/*===========================================================================
METHOD:
   UserspaceRead (Public Method)

DESCRIPTION:
   Userspace read (synchronous)

PARAMETERS
   pFilp           [ I ] - userspace file descriptor
   pBuf            [ I ] - read buffer
   size            [ I ] - size of read buffer
   pUnusedFpos     [ I ] - (unused) file position

RETURN VALUE:
   ssize_t - Number of bytes read for success
             Negative errno for failure
===========================================================================*/
ssize_t UserspaceRead(
   struct file *          pFilp,
   char __user *          pBuf,
   size_t                 size,
   loff_t *               pUnusedFpos )
{
   int result;
   void * pReadData = NULL;
   void * pSmallReadData;
   sQMIFilpStorage * pFilpData = (sQMIFilpStorage *)pFilp->private_data;

   if (pFilpData == NULL)
   {
      DBG( "Bad file data\n" );
      return -EBADF;
   }

   if (IsDeviceValid( pFilpData->mpDev ) == false)
   {
      DBG( "Invalid device! Updating f_ops\n" );
      pFilp->f_op = pFilp->f_dentry->d_inode->i_fop;
      return -ENXIO;
   }

   if (pFilpData->mClientID == (u16)-1)
   {
      DBG( "Client ID must be set before reading 0x%04X\n",
           pFilpData->mClientID );
      return -EBADR;
   }

   // Perform synchronous read
   result = ReadSync( pFilpData->mpDev,
                      &pReadData,
                      pFilpData->mClientID,
                      0 );
   if (result <= 0)
   {
      return result;
   }

   // Discard QMUX header
   result -= QMUXHeaderSize();
   pSmallReadData = pReadData + QMUXHeaderSize();

   if (result > size)
   {
      DBG( "Read data is too large for amount user has requested\n" );
      if(pReadData)
      kfree( pReadData );
      return -EOVERFLOW;
   }

   DBG(  "pBuf = 0x%p pSmallReadData = 0x%p, result = %d",
         pBuf, pSmallReadData, result );

   if (copy_to_user( pBuf, pSmallReadData, result ) != 0)
   {
      DBG( "Error copying read data to user\n" );
      result = -EFAULT;
   }

   // Reader is responsible for freeing read buffer
   kfree( pReadData );

   return result;
}

/*===========================================================================
METHOD:
   UserspaceWrite (Public Method)

DESCRIPTION:
   Userspace write (synchronous)

PARAMETERS
   pFilp           [ I ] - userspace file descriptor
   pBuf            [ I ] - write buffer
   size            [ I ] - size of write buffer
   pUnusedFpos     [ I ] - (unused) file position

RETURN VALUE:
   ssize_t - Number of bytes read for success
             Negative errno for failure
===========================================================================*/
ssize_t UserspaceWrite(
   struct file *        pFilp,
   const char __user *  pBuf,
   size_t               size,
   loff_t *             pUnusedFpos )
{
   int status;
   void * pWriteBuffer;
   sQMIFilpStorage * pFilpData = (sQMIFilpStorage *)pFilp->private_data;

   if (pFilpData == NULL)
   {
      DBG( "Bad file data\n" );
      return -EBADF;
   }

   if (IsDeviceValid( pFilpData->mpDev ) == false)
   {
      DBG( "Invalid device! Updating f_ops\n" );
      pFilp->f_op = pFilp->f_dentry->d_inode->i_fop;
      return -ENXIO;
   }

   if (pFilpData->mClientID == (u16)-1)
   {
      DBG( "Client ID must be set before writing 0x%04X\n",
           pFilpData->mClientID );
      return -EBADR;
   }

   // Copy data from user to kernel space
   pWriteBuffer = kmalloc( size + QMUXHeaderSize(), GFP_KERNEL );
   if (pWriteBuffer == NULL)
   {
      return -ENOMEM;
   }
   status = copy_from_user( pWriteBuffer + QMUXHeaderSize(), pBuf, size );
   if (status != 0)
   {
      DBG( "Unable to copy data from userspace %d\n", status );
      kfree( pWriteBuffer );
      return status;
   }

   status = WriteSync( pFilpData->mpDev,
                       pWriteBuffer,
                       size + QMUXHeaderSize(),
                       pFilpData->mClientID );

   kfree( pWriteBuffer );

   // On success, return requested size, not full QMI reqest size
   if (status == size + QMUXHeaderSize())
   {
      return size;
   }
   else
   {
      return status;
   }
}

/*===========================================================================
METHOD:
   UserspacePoll (Public Method)

DESCRIPTION:
   Used to determine if read/write operations are possible without blocking

PARAMETERS
   pFilp              [ I ] - userspace file descriptor
   pPollTable         [I/O] - Wait object to notify the kernel when data 
                              is ready

RETURN VALUE:
   unsigned int - bitmask of what operations can be done immediately
===========================================================================*/
unsigned int UserspacePoll(
   struct file *                  pFilp,
   struct poll_table_struct *     pPollTable )
{
   sQMIFilpStorage * pFilpData = (sQMIFilpStorage *)pFilp->private_data;
   sClientMemList * pClientMem;
   unsigned long flags;

   // Always ready to write
   unsigned int status = POLLOUT | POLLWRNORM;

   if (pFilpData == NULL)
   {
      DBG( "Bad file data\n" );
      return POLLERR;
   }

   if (IsDeviceValid( pFilpData->mpDev ) == false)
   {
      DBG( "Invalid device! Updating f_ops\n" );
      pFilp->f_op = pFilp->f_dentry->d_inode->i_fop;
      return POLLERR;
   }

   if (pFilpData->mClientID == (u16)-1)
   {
      DBG( "Client ID must be set before polling 0x%04X\n",
           pFilpData->mClientID );
      return POLLERR;
   }

   // Critical section
   spin_lock_irqsave( &pFilpData->mpDev->mQMIDev.mClientMemLock, flags );

   // Get this client's memory location
   pClientMem = FindClientMem( pFilpData->mpDev, 
                               pFilpData->mClientID );
   if (pClientMem == NULL)
   {
      DBG( "Could not find this client's memory 0x%04X\n",
           pFilpData->mClientID );

      spin_unlock_irqrestore( &pFilpData->mpDev->mQMIDev.mClientMemLock, 
                              flags );
      return POLLERR;
   }
   
   poll_wait( pFilp, &pClientMem->mWaitQueue, pPollTable );

   if (pClientMem->mpList != NULL)
   {
      status |= POLLIN | POLLRDNORM;
   }

   // End critical section
   spin_unlock_irqrestore( &pFilpData->mpDev->mQMIDev.mClientMemLock, flags );

   // Always ready to write 
   return (status | POLLOUT | POLLWRNORM);
}

/*=========================================================================*/
// Initializer and destructor
/*=========================================================================*/
int QMICTLSyncProc(sGobiUSBNet *pDev)
{
   void *pWriteBuffer;
   void *pReadBuffer;
   int result;
   u16 writeBufferSize;
   u8 transactionID;

   if (IsDeviceValid( pDev ) == false)
   {
      DBG( "Invalid device\n" );
      return -EFAULT;
   }

   writeBufferSize= QMICTLSyncReqSize();
   pWriteBuffer = kmalloc( writeBufferSize, GFP_KERNEL );
   if (pWriteBuffer == NULL)
   {
      return -ENOMEM;
   }

   transactionID = QMIXactionIDGet(pDev);

   /* send a QMI_CTL_SYNC_REQ (0x0027) */
   result = QMICTLSyncReq( pWriteBuffer,
                           writeBufferSize,
                           transactionID );
   if (result < 0)
   {
      kfree( pWriteBuffer );
      return result;
   }

   result = WriteSync( pDev,
                       pWriteBuffer,
                       writeBufferSize,
                       QMICTL );

   kfree( pWriteBuffer );
   if (result < 0)
   {
      return result;
   }

   // QMI CTL Sync Response
   result = ReadSync( pDev,
                      &pReadBuffer,
                      QMICTL,
                      transactionID );
   if (result < 0)
   {
      return result;
   }

   result = QMICTLSyncResp( pReadBuffer,
                            (u16)result );
   if(pReadBuffer); 
   kfree( pReadBuffer );

   if (result < 0) /* need to re-sync */
   {
      DBG( "sync response error code %d\n", result );
      /* start timer and wait for the response */
      /* process response */
      return result;
   }

   // Success
   return 0;
}

static int 
qmi_show(struct seq_file *m, void *v)
{
    sGobiUSBNet * pDev = (sGobiUSBNet*) m->private;
    seq_printf(m, "readTimeoutCnt %d\n", pDev->readTimeoutCnt);
    seq_printf(m, "writeTimeoutCnt %d\n", pDev->writeTimeoutCnt);
    return 0;
}

static int
qmi_open(struct inode *inode, struct file *file)
{
    char *data;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION( 3,10,0 ))
    data=PDE_DATA(inode);
#else
    data=PDE(inode)->data;
#endif

    return single_open(file, qmi_show, data);
}

static const struct file_operations proc_fops = {
    .owner      = THIS_MODULE,
    .open       = qmi_open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = single_release,
};

/*===========================================================================
METHOD:
   RegisterQMIDevice (Public Method)

DESCRIPTION:
   QMI Device initialization function

PARAMETERS:
   pDev     [ I ] - Device specific memory
   is9x15   [ I ]

RETURN VALUE:
   int - 0 for success
         Negative errno for failure
===========================================================================*/
int RegisterQMIDevice( sGobiUSBNet * pDev, int is9x15 )
{
   char qcqmi_dev_name[10];
   int i;
   int result;
   dev_t devno;
   pDev->mQMIDev.proc_file = NULL;
   if (pDev->mQMIDev.mbCdevIsInitialized == true)
   {
      // Should never happen, but always better to check
      DBG( "device already exists\n" );
      return -EEXIST;
   }

   pDev->mbQMIValid = true;
   pDev->readTimeoutCnt = 0;
   pDev->writeTimeoutCnt = 0;
   pDev->mtu = 0;
   init_rwsem(&pDev->shutdown_rwsem);


   // Set up for QMICTL
   //    (does not send QMI message, just sets up memory)
   result = GetClientID( pDev, QMICTL );
   if (result != 0)
   {
      pDev->mbQMIValid = false;
      return result;
   }
   atomic_set( &pDev->mQMIDev.mQMICTLTransactionID, 1 );

   // Start Async reading
   result = StartRead( pDev );
   if (result != 0)
   {
      pDev->mbQMIValid = false;
      return result;
   }

   // Send SetControlLineState request (USB_CDC)
   //   Required for Autoconnect and 9x30 to wake up
   result = usb_control_msg( pDev->mpNetDev->udev,
                             usb_sndctrlpipe( pDev->mpNetDev->udev, 0 ),
                             SET_CONTROL_LINE_STATE_REQUEST,
                             SET_CONTROL_LINE_STATE_REQUEST_TYPE,
                             CONTROL_DTR,
                             /* USB interface number to receive control message */
                             pDev->mpIntf->cur_altsetting->desc.bInterfaceNumber,
                             NULL,
                             0,
                             100 );
   if (result < 0)
   {
      DBG( "Bad SetControlLineState status %d\n", result );
      return result;
   }


   // Device is not ready for QMI connections right away
   //   Wait up to 30 seconds before failing
   if (QMIReady( pDev, 30000 ) == false)
   {
      DBG( "Device unresponsive to QMI\n" );
      return -ETIMEDOUT;
   }

   // Initiate QMI CTL Sync Procedure
   DBG( "Sending QMI CTL Sync Request\n" );
   result = QMICTLSyncProc(pDev);
   if (result != 0)
   {
      DBG( "QMI CTL Sync Procedure Error\n" );
      return result;
   }
   else
   {
      DBG( "QMI CTL Sync Procedure Successful\n" );
   }

   // Setup Data Format
   if (is9x15)
   {
       result = QMIWDASetDataFormat (pDev);
   }
   else
   {
       result = QMICTLSetDataFormat (pDev);
   }

   if (result != 0)
   {
       return result;
   }

   // Setup WDS callback
   result = SetupQMIWDSCallback( pDev );
   if (result != 0)
   {
      return result;
   }

#ifdef QOS_MODE
   // Setup QOS callback
   result = SetupQMIQOSCallback( pDev );
   if (result != 0)
   {
      return result;
   }
#endif

   if (is9x15)
   {
       // Set FCC Authentication
       result = QMIDMSSWISetFCCAuth( pDev );
       if (result != 0)
       {
          return result;
       }
   }

   // Fill MEID for device
   result = QMIDMSGetMEID( pDev );
   if (result != 0)
   {
      return result;
   }

   // allocate and fill devno with numbers
   result = alloc_chrdev_region( &devno, 0, 1, "qcqmi" );
   if (result < 0)
   {
      return result;
   }

   // Create cdev
   cdev_init( &pDev->mQMIDev.mCdev, &UserspaceQMIFops );
   pDev->mQMIDev.mCdev.owner = THIS_MODULE;
   pDev->mQMIDev.mCdev.ops = &UserspaceQMIFops;
   pDev->mQMIDev.mbCdevIsInitialized = true;

   result = cdev_add( &pDev->mQMIDev.mCdev, devno, 1 );
   if (result != 0)
   {
      DBG( "error adding cdev\n" );
      return result;
   }

   for(i=0;i<MAX_QCQMI;i++)
   {
       if (qcqmi_table[i] == 0)
           break;
   }
   
   if (i == MAX_QCQMI)
   {
       printk(KERN_WARNING "no free entry available at qcqmi_table array\n");
       return -ENOMEM;
   }
   qcqmi_table[i] = 1;
   pDev->mQMIDev.qcqmi = i;

   // Always print this output
   printk( KERN_INFO "creating qcqmi%d\n",
           pDev->mQMIDev.qcqmi );

#if (LINUX_VERSION_CODE >= KERNEL_VERSION( 2,6,27 ))
   // kernel 2.6.27 added a new fourth parameter to device_create
   //    void * drvdata : the data to be added to the device for callbacks
   device_create( pDev->mQMIDev.mpDevClass,
                  &pDev->mpIntf->dev,
                  devno,
                  NULL,
                  "qcqmi%d",
                  pDev->mQMIDev.qcqmi );
#else
   device_create( pDev->mQMIDev.mpDevClass,
                  &pDev->mpIntf->dev,
                  devno,
                  "qcqmi%d",
                  pDev->mQMIDev.qcqmi );
#endif

   pDev->mQMIDev.mDevNum = devno;

   memset(pDev->maps.table, 0xff, sizeof(pDev->maps.table));
   pDev->maps.count = 0; 

   sprintf(qcqmi_dev_name, "qcqmi%d", (int)pDev->mQMIDev.qcqmi);
   pDev->mQMIDev.proc_file = proc_create_data(qcqmi_dev_name, 0, NULL, &proc_fops, pDev);

   if (!pDev->mQMIDev.proc_file) {
       return -ENOMEM;
   }

  // Success
   return 0;
}

/*===========================================================================
METHOD:
   DeregisterQMIDevice (Public Method)

DESCRIPTION:
   QMI Device cleanup function

   NOTE: When this function is run the device is no longer valid

PARAMETERS:
   pDev     [ I ] - Device specific memory

RETURN VALUE:
   None
===========================================================================*/
void DeregisterQMIDevice( sGobiUSBNet * pDev )
{
   char qcqmi_dev_name[10];
   struct inode * pOpenInode;
   struct list_head * pInodeList;
   struct task_struct * pEachTask;
   struct fdtable * pFDT;
   struct file * pFilp;
   unsigned long flags;
   int count = 0;
   int tries;
   int result;

   // Should never happen, but check anyway
   if (IsDeviceValid( pDev ) == false)
   {
      DBG( "wrong device\n" );
      KillRead( pDev );
      // Send SetControlLineState request (USB_CDC)
      result = usb_control_msg( pDev->mpNetDev->udev,
                             usb_sndctrlpipe( pDev->mpNetDev->udev, 0 ),
                             SET_CONTROL_LINE_STATE_REQUEST,
                             SET_CONTROL_LINE_STATE_REQUEST_TYPE,
                             0, // DTR not present
                             /* USB interface number to receive control message */
                             pDev->mpIntf->cur_altsetting->desc.bInterfaceNumber,
                             NULL,
                             0,
                             100 );
      return;
   }
   if(pDev->mQMIDev.proc_file != NULL)
   {
      sprintf(qcqmi_dev_name, "qcqmi%d", (int)pDev->mQMIDev.qcqmi);
      remove_proc_entry(qcqmi_dev_name, NULL);
      pDev->mQMIDev.proc_file = NULL;
   }
   // Release all clients
   while (pDev->mQMIDev.mpClientMemList != NULL)
   {
      DBG( "release 0x%04X\n", pDev->mQMIDev.mpClientMemList->mClientID );

      if (ReleaseClientID(pDev,
                       pDev->mQMIDev.mpClientMemList->mClientID) == false)
          break;
      // NOTE: pDev->mQMIDev.mpClientMemList will
      //       be updated in ReleaseClientID()
   }

   // Stop all reads
   KillRead( pDev );

   pDev->mbQMIValid = false;

   if (pDev->mQMIDev.mbCdevIsInitialized == false)
   {
      return;
   }

   // Find each open file handle, and manually close it

   // Generally there will only be only one inode, but more are possible
   list_for_each( pInodeList, &pDev->mQMIDev.mCdev.list )
   {
      // Get the inode
      pOpenInode = container_of( pInodeList, struct inode, i_devices );
      if (pOpenInode != NULL && (IS_ERR( pOpenInode ) == false))
      {
         // Look for this inode in each task

         rcu_read_lock();
         for_each_process( pEachTask )
         {
            if (pEachTask == NULL || pEachTask->files == NULL)
            {
               // Some tasks may not have files (e.g. Xsession)
               continue;
            }
            // For each file this task has open, check if it's referencing
            // our inode.
            spin_lock_irqsave( &pEachTask->files->file_lock, flags );
            pFDT = files_fdtable( pEachTask->files );
            for (count = 0; count < pFDT->max_fds; count++)
            {
               pFilp = pFDT->fd[count];
               if (pFilp != NULL &&  pFilp->f_dentry != NULL)
               {
                  if (pFilp->f_dentry->d_inode == pOpenInode)
                  {
                     // Close this file handle
                     rcu_assign_pointer( pFDT->fd[count], NULL );
                     spin_unlock_irqrestore( &pEachTask->files->file_lock, flags );

                     DBG( "forcing close of open file handle\n" );
                     filp_close( pFilp, pEachTask->files );

                     spin_lock_irqsave( &pEachTask->files->file_lock, flags );
                  }
               }
            }
            spin_unlock_irqrestore( &pEachTask->files->file_lock, flags );
         }
         rcu_read_unlock();
      }
   }

   // Send SetControlLineState request (USB_CDC)
   result = usb_control_msg( pDev->mpNetDev->udev,
                             usb_sndctrlpipe( pDev->mpNetDev->udev, 0 ),
                             SET_CONTROL_LINE_STATE_REQUEST,
                             SET_CONTROL_LINE_STATE_REQUEST_TYPE,
                             0, // DTR not present
                             /* USB interface number to receive control message */
                             pDev->mpIntf->cur_altsetting->desc.bInterfaceNumber,
                             NULL,
                             0,
                             100 );
   if (result < 0)
   {
      DBG( "Bad SetControlLineState status %d\n", result );
   }

   // Remove device (so no more calls can be made by users)
   if (IS_ERR( pDev->mQMIDev.mpDevClass ) == false)
   {
      device_destroy( pDev->mQMIDev.mpDevClass,
                      pDev->mQMIDev.mDevNum );
   }

   qcqmi_table[pDev->mQMIDev.qcqmi] = 0;

   // Hold onto cdev memory location until everyone is through using it.
   // Timeout after 30 seconds (10 ms interval).  Timeout should never happen,
   // but exists to prevent an infinate loop just in case.
   for (tries = 0; tries < 30 * 100; tries++)
   {
      int ref = atomic_read( &pDev->mQMIDev.mCdev.kobj.kref.refcount );
      if (ref > 1)
      {
         DBG( "cdev in use by %d tasks\n", ref - 1 );
         msleep( 10 );
      }
      else
      {
         break;
      }
   }

   cdev_del( &pDev->mQMIDev.mCdev );

   unregister_chrdev_region( pDev->mQMIDev.mDevNum, 1 );

   return;
}

/*=========================================================================*/
// Driver level client management
/*=========================================================================*/

/*===========================================================================
METHOD:
   QMIReady (Public Method)

DESCRIPTION:
   Send QMI CTL GET VERSION INFO REQ and SET DATA FORMAT REQ
   Wait for response or timeout

PARAMETERS:
   pDev     [ I ] - Device specific memory
   timeout  [ I ] - Milliseconds to wait for response

RETURN VALUE:
   bool
===========================================================================*/
bool QMIReady(
   sGobiUSBNet *    pDev,
   u16                timeout )
{
   int result;
   void * pWriteBuffer;
   u16 writeBufferSize;
   void * pReadBuffer;
   u16 readBufferSize;
   struct semaphore readSem;
   u16 curTime;
   unsigned long flags;
   u8 transactionID;

   if (IsDeviceValid( pDev ) == false)
   {
      DBG( "Invalid device\n" );
      return false;
   }

   writeBufferSize = QMICTLReadyReqSize();
   pWriteBuffer = kmalloc( writeBufferSize, GFP_KERNEL );
   if (pWriteBuffer == NULL)
   {
      return false;
   }

   // An implimentation of down_timeout has not been agreed on,
   //    so it's been added and removed from the kernel several times.
   //    We're just going to ignore it and poll the semaphore.

   // Send a write every 1000 ms and see if we get a response
   for (curTime = 0; curTime < timeout; curTime += 1000)
   {
      // Start read
      sema_init( &readSem, 0 );

      transactionID = atomic_add_return( 1, &pDev->mQMIDev.mQMICTLTransactionID );
      if (transactionID == 0)
      {
         transactionID = atomic_add_return( 1, &pDev->mQMIDev.mQMICTLTransactionID );
      }
      result = ReadAsync( pDev, QMICTL, transactionID, UpSem, &readSem );
      if (result != 0)
      {
         kfree( pWriteBuffer );
         return false;
      }

      // Fill buffer
      result = QMICTLReadyReq( pWriteBuffer,
                               writeBufferSize,
                               transactionID );
      if (result < 0)
      {
         kfree( pWriteBuffer );
         return false;
      }

      // Disregard status.  On errors, just try again
      WriteSync( pDev,
                 pWriteBuffer,
                 writeBufferSize,
                 QMICTL );

      msleep( 1000 );
      if (down_trylock( &readSem ) == 0)
      {
         // Enter critical section
         spin_lock_irqsave( &pDev->mQMIDev.mClientMemLock, flags );

         // Pop the read data
         if (PopFromReadMemList( pDev,
                                 QMICTL,
                                 0, //ignore transaction id
                                 &pReadBuffer,
                                 &readBufferSize ) == true)
         {
            // Success

            // End critical section
            spin_unlock_irqrestore( &pDev->mQMIDev.mClientMemLock, flags );

            // We don't care about the result
            if(pReadBuffer)
            kfree( pReadBuffer );

            break;
         }
         else
         {
            // Read mismatch/failure, unlock and continue
            spin_unlock_irqrestore( &pDev->mQMIDev.mClientMemLock, flags );
         }
      }
      else
      {
         // Enter critical section
         spin_lock_irqsave( &pDev->mQMIDev.mClientMemLock, flags );

         // Timeout, remove the async read
         NotifyAndPopNotifyList( pDev, QMICTL, transactionID );

         // End critical section
         spin_unlock_irqrestore( &pDev->mQMIDev.mClientMemLock, flags );
      }
   }
   kfree( pWriteBuffer );
   // Did we time out?
   if (curTime >= timeout)
   {
      return false;
   }

   DBG( "QMI Ready after %u milliseconds\n", curTime );

   // Success
   return true;
}

/*===========================================================================
METHOD:
   QMIWDSCallback (Public Method)

DESCRIPTION:
   QMI WDS callback function
   Update net stats or link state

PARAMETERS:
   pDev     [ I ] - Device specific memory
   clientID [ I ] - Client ID
   pData    [ I ] - Callback data (unused)

RETURN VALUE:
   None
===========================================================================*/
void QMIWDSCallback(
   sGobiUSBNet *    pDev,
   u16                clientID,
   void *             pData )
{
   bool bRet;
   int result;
   void * pReadBuffer;
   u16 readBufferSize;
   u32 TXOk = (u32)-1;
   u32 RXOk = (u32)-1;
   u32 TXErr = (u32)-1;
   u32 RXErr = (u32)-1;
   u32 TXOfl = (u32)-1;
   u32 RXOfl = (u32)-1;
   u64 TXBytesOk = (u64)-1;
   u64 RXBytesOk = (u64)-1;
   bool bReconfigure;
   unsigned long flags;

   if (IsDeviceValid( pDev ) == false)
   {
      DBG( "Invalid device\n" );
      return;
   }
   
   // Critical section
   spin_lock_irqsave( &pDev->mQMIDev.mClientMemLock, flags );

   bRet = PopFromReadMemList( pDev,
                              clientID,
                              0,
                              &pReadBuffer,
                              &readBufferSize );

   // End critical section
   spin_unlock_irqrestore( &pDev->mQMIDev.mClientMemLock, flags );

   if (bRet == false)
   {
      DBG( "WDS callback failed to get data\n" );
      return;
   }

   // Default values
   pDev->bLinkState = ! GobiTestDownReason( pDev, NO_NDIS_CONNECTION );
   bReconfigure = false;

   result = QMIWDSEventResp( pReadBuffer,
                             readBufferSize,
                             &TXOk,
                             &RXOk,
                             &TXErr,
                             &RXErr,
                             &TXOfl,
                             &RXOfl,
                             &TXBytesOk,
                             &RXBytesOk,
                             (u8*)&pDev->bLinkState,
                             &bReconfigure );
   if (result < 0)
   {
      DBG( "bad WDS packet\n" );
   }
   else
   {
      if (bReconfigure == true)
      {
         DBG( "Net device link reset\n" );
         GobiSetDownReason( pDev, NO_NDIS_CONNECTION );
         GobiClearDownReason( pDev, NO_NDIS_CONNECTION );
      }
      else
      {
         if (pDev->bLinkState == true)
         {
            DBG( "Net device link is connected\n" );
            GobiClearDownReason( pDev, NO_NDIS_CONNECTION );
         }
         else
         {
            DBG( "Net device link is disconnected\n" );
            GobiSetDownReason( pDev, NO_NDIS_CONNECTION );
         }
      }
   }
   if(pReadBuffer)
   kfree( pReadBuffer );

   // Setup next read
   result = ReadAsync( pDev,
                       clientID,
                       0,
                       QMIWDSCallback,
                       pData );
   if (result != 0)
   {
      DBG( "unable to setup next async read\n" );
   }

   return;
}

void QMIQOSCallback(
   sGobiUSBNet *    pDev,
   u16                clientID,
   void *             pData )
{
   bool bRet;
   int result;
   void * pReadBuffer;
   u16 readBufferSize;

   unsigned long flags;

   if (IsDeviceValid( pDev ) == false)
   {
      QDBG( "Invalid device\n" );
      return;
   }

   // Critical section
   spin_lock_irqsave( &pDev->mQMIDev.mClientMemLock, flags );

   bRet = PopFromReadMemList( pDev,
                              clientID,
                              0,
                              &pReadBuffer,
                              &readBufferSize );

   // End critical section
   spin_unlock_irqrestore( &pDev->mQMIDev.mClientMemLock, flags );

   if (bRet == false)
   {
      QDBG( "QOS callback failed to get data\n" );
      return;
   }

   result = QMIQOSEventResp(pDev, pReadBuffer, readBufferSize);

   if (result < 0)
   {
      QDBG( "bad QOS packet\n" );
   }
   if(pReadBuffer)
   kfree( pReadBuffer );

   // Setup next read
   result = ReadAsync( pDev,
                       clientID,
                       0,
                       QMIQOSCallback,
                       pData );
   if (result != 0)
   {
      QDBG( "unable to setup next async read\n" );
   }

   return;
}

/*===========================================================================
METHOD:
   SetupQMIWDSCallback (Public Method)

DESCRIPTION:
   Request client and fire off reqests and start async read for
   QMI WDS callback

PARAMETERS:
   pDev     [ I ] - Device specific memory

RETURN VALUE:
   int - 0 for success
         Negative errno for failure
===========================================================================*/
int SetupQMIWDSCallback( sGobiUSBNet * pDev )
{
   int result;
   void * pWriteBuffer;
   u16 writeBufferSize;
   u16 WDSClientID;

   if (IsDeviceValid( pDev ) == false)
   {
      DBG( "Invalid device\n" );
      return -EFAULT;
   }

   result = GetClientID( pDev, QMIWDS );
   if (result < 0)
   {
      return result;
   }
   WDSClientID = result;

   // QMI WDS Set Event Report
   writeBufferSize = QMIWDSSetEventReportReqSize();
   pWriteBuffer = kmalloc( writeBufferSize, GFP_KERNEL );
   if (pWriteBuffer == NULL)
   {
      return -ENOMEM;
   }

   result = QMIWDSSetEventReportReq( pWriteBuffer,
                                     writeBufferSize,
                                     1 );
   if (result < 0)
   {
      kfree( pWriteBuffer );
      return result;
   }

   result = WriteSync( pDev,
                       pWriteBuffer,
                       writeBufferSize,
                       WDSClientID );
   kfree( pWriteBuffer );

   if (result < 0)
   {
      return result;
   }

   // QMI WDS Get PKG SRVC Status
   writeBufferSize = QMIWDSGetPKGSRVCStatusReqSize();
   pWriteBuffer = kmalloc( writeBufferSize, GFP_KERNEL );
   if (pWriteBuffer == NULL)
   {
      return -ENOMEM;
   }

   result = QMIWDSGetPKGSRVCStatusReq( pWriteBuffer,
                                       writeBufferSize,
                                       2 );
   if (result < 0)
   {
      kfree( pWriteBuffer );
      return result;
   }

   result = WriteSync( pDev,
                       pWriteBuffer,
                       writeBufferSize,
                       WDSClientID );
   kfree( pWriteBuffer );

   if (result < 0)
   {
      return result;
   }

   // Setup asnyc read callback
   result = ReadAsync( pDev,
                       WDSClientID,
                       0,
                       QMIWDSCallback,
                       NULL );
   if (result != 0)
   {
      DBG( "unable to setup async read\n" );
      return result;
   }

   return 0;
}

int SetupQMIQOSCallback( sGobiUSBNet * pDev )
{
   int result;
   u16 QOSClientID;

   if (IsDeviceValid( pDev ) == false)
   {
      DBG( "Invalid device\n" );
      return -EFAULT;
   }

   result = GetClientID( pDev, QMIQOS );
   if (result < 0)
   {
      return result;
   }
   QOSClientID = result;

   //TODO QMI QOS Set Event Report

   // Setup asnyc read callback
   result = ReadAsync( pDev,
                       QOSClientID,
                       0,
                       QMIQOSCallback,
                       NULL );
   if (result != 0)
   {
      DBG( "unable to setup async read\n" );
      return result;
   }

   return 0;
}

/*===========================================================================
METHOD:
   QMIDMSSWISetFCCAuth (Public Method)

DESCRIPTION:
   Register DMS client
   send FCC Authentication req and parse response
   Release DMS client

PARAMETERS:
   pDev     [ I ] - Device specific memory

RETURN VALUE:
   None
===========================================================================*/
int QMIDMSSWISetFCCAuth( sGobiUSBNet * pDev )
{
   int result;
   void * pWriteBuffer;
   u16 writeBufferSize;
   void * pReadBuffer;
   u16 readBufferSize;
   u16 DMSClientID;

   DBG("\n");

   if (IsDeviceValid( pDev ) == false)
   {
      DBG( "Invalid device\n" );
      return -EFAULT;
   }

   result = GetClientID( pDev, QMIDMS );
   if (result < 0)
   {
      return result;
   }
   DMSClientID = result;

   // QMI DMS Get Serial numbers Req
   writeBufferSize = QMIDMSSWISetFCCAuthReqSize();
   pWriteBuffer = kmalloc( writeBufferSize, GFP_KERNEL );
   if (pWriteBuffer == NULL)
   {
      return -ENOMEM;
   }

   result = QMIDMSSWISetFCCAuthReq( pWriteBuffer,
                                    writeBufferSize,
                                    1 );
   if (result < 0)
   {
      kfree( pWriteBuffer );
      return result;
   }

   result = WriteSync( pDev,
                       pWriteBuffer,
                       writeBufferSize,
                       DMSClientID );
   kfree( pWriteBuffer );

   if (result < 0)
   {
      return result;
   }

   // QMI DMS Get Serial numbers Resp
   result = ReadSync( pDev,
                      &pReadBuffer,
                      DMSClientID,
                      1 );
   if (result < 0)
   {
      return result;
   }
   readBufferSize = result;

//   result = QMIDMSSWISetFCCAuthResp( pReadBuffer,
//                                     readBufferSize );
   if(pReadBuffer)
   kfree( pReadBuffer );

   if (result < 0)
   {
      // Non fatal error, device did not return FCC Auth response
      DBG( "Bad FCC Auth resp\n" );
   }

   ReleaseClientID( pDev, DMSClientID );

   // Success
   return 0;
}

/*===========================================================================
METHOD:
   QMIDMSGetMEID (Public Method)

DESCRIPTION:
   Register DMS client
   send MEID req and parse response
   Release DMS client

PARAMETERS:
   pDev     [ I ] - Device specific memory

RETURN VALUE:
   None
===========================================================================*/
int QMIDMSGetMEID( sGobiUSBNet * pDev )
{
   int result;
   void * pWriteBuffer;
   u16 writeBufferSize;
   void * pReadBuffer;
   u16 readBufferSize;
   u16 DMSClientID;

   if (IsDeviceValid( pDev ) == false)
   {
      DBG( "Invalid device\n" );
      return -EFAULT;
   }

   result = GetClientID( pDev, QMIDMS );
   if (result < 0)
   {
      return result;
   }
   DMSClientID = result;

   // QMI DMS Get Serial numbers Req
   writeBufferSize = QMIDMSGetMEIDReqSize();
   pWriteBuffer = kmalloc( writeBufferSize, GFP_KERNEL );
   if (pWriteBuffer == NULL)
   {
      return -ENOMEM;
   }

   result = QMIDMSGetMEIDReq( pWriteBuffer,
                              writeBufferSize,
                              1 );
   if (result < 0)
   {
      kfree( pWriteBuffer );
      return result;
   }

   result = WriteSync( pDev,
                       pWriteBuffer,
                       writeBufferSize,
                       DMSClientID );
   kfree( pWriteBuffer );

   if (result < 0)
   {
      return result;
   }

   // QMI DMS Get Serial numbers Resp
   result = ReadSync( pDev,
                      &pReadBuffer,
                      DMSClientID,
                      1 );
   if (result < 0)
   {
      return result;
   }
   readBufferSize = result;

   result = QMIDMSGetMEIDResp( pReadBuffer,
                               readBufferSize,
                               &pDev->mMEID[0],
                               14 );
   kfree( pReadBuffer );

   if (result < 0)
   {
      DBG( "bad get MEID resp\n" );

      // Non fatal error, device did not return any MEID
      //    Fill with 0's
      memset( &pDev->mMEID[0], '0', 14 );
   }

   ReleaseClientID( pDev, DMSClientID );

   // always return Success as MEID is only available on CDMA devices only
   return 0;
}

/*===========================================================================
METHOD:
   QMICTLSetDataFormat (Public Method)

DESCRIPTION:
   send Data format request and parse response

PARAMETERS:
   pDev     [ I ] - Device specific memory

RETURN VALUE:
   None
===========================================================================*/
int QMICTLSetDataFormat( sGobiUSBNet * pDev )
{
   unsigned long flags;
   u8 transactionID;
   struct semaphore readSem;
   int result;
   void * pWriteBuffer;
   u16 writeBufferSize;
   void * pReadBuffer;
   u16 readBufferSize;

   DBG("\n");

   // Send SET DATA FORMAT REQ
   writeBufferSize = QMICTLSetDataFormatReqSize();

   pWriteBuffer = kmalloc( writeBufferSize, GFP_KERNEL );
   if (pWriteBuffer == NULL)
   {
      return -ENOMEM;
   }

   // Start read
   sema_init( &readSem, 0 );

   transactionID = atomic_add_return( 1, &pDev->mQMIDev.mQMICTLTransactionID );
   if (transactionID == 0)
   {
      transactionID = atomic_add_return( 1, &pDev->mQMIDev.mQMICTLTransactionID );
   }

   result = ReadAsync( pDev, QMICTL, transactionID, UpSem, &readSem );
   if (result != 0)
   {
      kfree( pWriteBuffer );
      return result;
   }

   // Fill buffer
   result = QMICTLSetDataFormatReq( pWriteBuffer,
                            writeBufferSize,
                            transactionID );

   if (result < 0)
   {
      kfree( pWriteBuffer );
      return result;
   }

   DBG("Sending QMI Set Data Format Request, TransactionID: 0x%x\n", transactionID );

   WriteSync( pDev,
              pWriteBuffer,
              writeBufferSize,
              QMICTL );

   msleep( 100 );
   if (down_trylock( &readSem ) == 0)
   {
      // Enter critical section
      spin_lock_irqsave( &pDev->mQMIDev.mClientMemLock, flags );

      // Pop the read data
      if (PopFromReadMemList( pDev,
                              QMICTL,
                              transactionID,
                              &pReadBuffer,
                              &readBufferSize ) == true)
      {
         // Success
         PrintHex(pReadBuffer, readBufferSize);

         // End critical section
         spin_unlock_irqrestore( &pDev->mQMIDev.mClientMemLock, flags );

         // We care about the result: call Response  function
         result = QMICTLSetDataFormatResp( pReadBuffer, readBufferSize);
         if(pReadBuffer)
         kfree( pReadBuffer );

         if (result != 0)
         {
            DBG( "Device cannot set requested data format\n" );
            if(pWriteBuffer)
            kfree( pWriteBuffer );
            return result;
         }
      }
   }
   else
   {
      // Enter critical section
      spin_lock_irqsave( &pDev->mQMIDev.mClientMemLock, flags );

      // Timeout, remove the async read
      NotifyAndPopNotifyList( pDev, QMICTL, transactionID );

      // End critical section
      spin_unlock_irqrestore( &pDev->mQMIDev.mClientMemLock, flags );
   }

   kfree( pWriteBuffer );

   return 0;
}

/*===========================================================================
METHOD:
   QMIWDASetDataFormat (Public Method)

DESCRIPTION:
   Register WDA client
   send Data format request and parse response
   Release WDA client

PARAMETERS:
   pDev     [ I ] - Device specific memory

RETURN VALUE:
   None
===========================================================================*/
int QMIWDASetDataFormat( sGobiUSBNet * pDev )
{
   int result;
   void * pWriteBuffer;
   u16 writeBufferSize;
   void * pReadBuffer;
   u16 readBufferSize;
   u16 WDAClientID;

   DBG("\n");

   if (IsDeviceValid( pDev ) == false)
   {
      DBG( "Invalid device\n" );
      return -EFAULT;
   }

   result = GetClientID( pDev, QMIWDA );
   if (result < 0)
   {
      return result;
   }
   WDAClientID = result;

   // QMI WDA Set Data Format Request
   writeBufferSize = QMIWDASetDataFormatReqSize();
   pWriteBuffer = kmalloc( writeBufferSize, GFP_KERNEL );
   if (pWriteBuffer == NULL)
   {
      return -ENOMEM;
   }

   result = QMIWDASetDataFormatReq( pWriteBuffer,
                                    writeBufferSize,
                                    1 );
   if (result < 0)
   {
      kfree( pWriteBuffer );
      return result;
   }

   result = WriteSync( pDev,
                       pWriteBuffer,
                       writeBufferSize,
                       WDAClientID );
   kfree( pWriteBuffer );

   if (result < 0)
   {
      return result;
   }

   // QMI DMS Get Serial numbers Resp
   result = ReadSync( pDev,
                      &pReadBuffer,
                      WDAClientID,
                      1 );
   if (result < 0)
   {
      return result;
   }
   readBufferSize = result;

   result = QMIWDASetDataFormatResp( pReadBuffer,
                                     readBufferSize );
   if(pReadBuffer)
   kfree( pReadBuffer );

   if (result < 0)
   {
      DBG( "Data Format Cannot be set\n" );
   }

   ReleaseClientID( pDev, WDAClientID );

   // Success
   return result;
}

