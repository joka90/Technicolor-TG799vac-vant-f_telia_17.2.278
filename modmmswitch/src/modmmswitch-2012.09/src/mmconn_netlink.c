/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2010-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
**                                                                      **
*************************************************************************/

/** \file
 * Multimedia Switch API.
 *
 * A multimedia switch routes (multimedia) streams between all connection instances attached to the multimedia switch.
 * A multimedia switch is implemented in user space and kernel space.
 * All configuration/management is done in user space, all stream handling in kernel space.
 *
 * \version v1.0
 *
 *************************************************************************/


/*
 * Define tracing prefix, needs to be defined before includes.
 */
#define MODULE_NAME    "MMCONN_NETLINK"
/*########################################################################
#                                                                        #
#  HEADER (INCLUDE) SECTION                                              #
#                                                                        #
########################################################################*/
#include <net/genetlink.h>
#include <linux/list.h>
#include <linux/module.h>
#include "mmswitch.h"

#include "mmconn_netlink_p.h"
#include "mmconn_p.h"

/*########################################################################
#                                                                        #
#  MACROS/DEFINES                                                        #
#                                                                        #
########################################################################*/
#define MMCONN_EVENT_FIFO_SIZE    (1 * PAGE_SIZE)

/*########################################################################
#                                                                        #
#  TYPES                                                                 #
#                                                                        #
########################################################################*/
typedef struct {
    MmConnEvent       event; /**< Work queue info */
    struct list_head  list;
} MmConnQueueEntry;

/*########################################################################
#                                                                        #
#  PRIVATE FUNCTION PROTOTYPES                                           #
#                                                                        #
########################################################################*/

static int mmConnParseNotify(struct sk_buff   *skb_2,
                             struct genl_info *info);

static void mmConnSendEvent_work_fn(struct work_struct *work);
static MmPbxError mmConnSendMmConnQueueEntry(MmConnHndl       mmConn,
                                             MmConnQueueEntry *entry);

/*########################################################################
#                                                                        #
#  PRIVATE GLOBALS                                                       #
#                                                                        #
########################################################################*/
static int          _traceLevel = MMPBX_TRACELEVEL_ERROR;
static unsigned int seq_num     = 0;
/* attribute policy: defines which attribute has which type (e.g int, char * etc)
 * possible values defined in net/netlink.h
 */
static struct nla_policy mmConnGeNlAttrPolicy[MMCONN_ATTR_MAX + 1] =
{
    [MMCONN_ATTR_REF_MMCONN]  = { .type = NLA_UNSPEC },
    [MMCONN_ATTR_EVENT]       = { .type = NLA_U32    },
    [MMCONN_ATTR_EVENT_PARM]  = { .type = NLA_U32    },
};


/* family definition */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0)
enum mmConn_multicast_groups {
    MMCONN_EVENT_GROUP,
};

static const struct genl_multicast_group mmConnGeNlMcEventGrps[] =
{
    [MMCONN_EVENT_GROUP] = { .name = MMCONN_GENL_MCGRP_EV_NAME, },
};
#else
static struct genl_multicast_group mmConnGeNlMcEventGrp =
{
    .name = MMCONN_GENL_MCGRP_EV_NAME,
};
#endif
static struct genl_family mmConnGeNlFamily =
{
    .id       = GENL_ID_GENERATE,                    //genetlink should generate an id
    .hdrsize  =               0,
    .name     = MMCONN_GENL_FAMILY,                   //the name of this family, used by userspace application
    .version  = MMCONN_GENL_FAMILY_VERSION,           //version number
    .maxattr  = MMCONN_ATTR_MAX,
};

/* commands: mapping between the command enumeration and the actual function*/
static struct genl_ops mmConnGeNlOps[] =
{
    {
        .cmd    = MMCONN_CMD_NOTIFY,
        .flags  = 0,
        .policy = mmConnGeNlAttrPolicy,
        .doit   = mmConnParseNotify,
        .dumpit = NULL,
    },
};

/*########################################################################
#                                                                        #
#   PUBLIC FUNCTION DEFINITIONS                                          #
#                                                                        #
########################################################################*/

/**
 *
 */
MmPbxError mmConnGeNlSetTraceLevel(MmPbxTraceLevel level)
{
    _traceLevel = level;
    MMPBX_TRC_INFO("New trace level : %s\n", mmPbxGetTraceLevelString(level));


    return MMPBX_ERROR_NOERROR;
}

/**
 *
 */
MmPbxError mmConnGeNlInit(void)
{
    MmPbxError  err = MMPBX_ERROR_NOERROR;
    int         ret = 0;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 31)
    int i;
#endif

    do {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
        /*register new family and commands (functions) of the new family*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0)
        ret = genl_register_family_with_ops_groups(&mmConnGeNlFamily, mmConnGeNlOps, mmConnGeNlMcEventGrps);
        if (ret != 0) {
            MMPBX_TRC_ERROR("genl_register_family_with_ops failed\n");
            err = MMPBX_ERROR_INTERNALERROR;
            break;
        }
#else
        ret = genl_register_family_with_ops(&mmConnGeNlFamily, mmConnGeNlOps, ARRAY_SIZE(mmConnGeNlOps));
        if (ret != 0) {
            MMPBX_TRC_ERROR("genl_register_family_with_ops failed\n");
            err = MMPBX_ERROR_INTERNALERROR;
            break;
        }
        ret = genl_register_mc_group(&mmConnGeNlFamily, &mmConnGeNlMcEventGrp);
        if (ret != 0) {
            MMPBX_TRC_ERROR("genl_register_mc_group failed\n");
            err = MMPBX_ERROR_INTERNALERROR;
            break;
        }
#endif
#else
        ret = genl_register_family(&mmConnGeNlFamily);
        if (ret != 0) {
            MMPBX_TRC_ERROR("genl_register_family failed\n");
            err = MMPBX_ERROR_INTERNALERROR;
            break;
        }

        for (i = 0; i < ARRAY_SIZE(mmConnGeNlOps); i++) {
            ret = genl_register_ops(&mmConnGeNlFamily, &mmConnGeNlOps[i]);
            if (ret != 0) {
                MMPBX_TRC_ERROR("genl_register_ops failed\n");
                err = MMPBX_ERROR_INTERNALERROR;
                break;
            }
        }
#endif
    } while (0);

    if (err != MMPBX_ERROR_NOERROR) {
        genl_unregister_family(&mmConnGeNlFamily);
    }
    return err;
}

/**
 *
 */
MmPbxError mmConnGeNlDeinit(void)
{
    MmPbxError  err = MMPBX_ERROR_NOERROR;
    int         ret = 0;


    do {
        /*unregister the family, will unregister the multicast groups*/
        ret = genl_unregister_family(&mmConnGeNlFamily);
        if (ret != 0) {
            MMPBX_TRC_ERROR("unregister family failed with: %i\n", ret);
            err = MMPBX_ERROR_INTERNALERROR;
            break;
        }

        return err;
    } while (0);

    return err;
}

/**
 *
 */
MmPbxError mmConnGeNlPrepare(MmConnHndl mmConn)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33)
    int err = 0;
#endif

    spin_lock_init(&mmConn->event_fifo_lock);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33)
    err = kfifo_alloc(&mmConn->event_fifo, MMCONN_EVENT_FIFO_SIZE, GFP_KERNEL);
    if (err != 0) {
        ret = MMPBX_ERROR_NORESOURCES;
        MMPBX_TRC_ERROR("kfifo_alloc failed with error: %d\n", err);
    }
#else
    mmConn->event_fifo = kfifo_alloc(MMCONN_EVENT_FIFO_SIZE, GFP_KERNEL, &mmConn->event_fifo_lock);
#endif

    return ret;
}

/**
 *
 */
MmPbxError mmConnGeNlDestroy(MmConnHndl mmConn)
{
    MmPbxError err = MMPBX_ERROR_NOERROR;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33)
    kfifo_free(&mmConn->event_fifo);
#else
    kfifo_free(mmConn->event_fifo);
#endif


    return err;
}

/**
 *
 */
MmPbxError mmConnSendEvent(MmConnHndl   mmConn,
                           MmConnEvent  *event)
{
    MmPbxError              err = MMPBX_ERROR_NOERROR;
    MmConnQueueEntry        entry;
    int                     count       = 0;
    struct MmConnAsyncWork  *asyncWork  = NULL;

    MMPBX_TRC_INFO("called\n");

    if (mmConn == NULL) {
        err = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    if (event == NULL) {
        err = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    /*
     * We will send data to socket asynchronously.
     * This is done to avoid "BUG scheduling while atomic", triggered within sock_sendmsg.
     * This means that it's possible that this function is called multiple times before
     * the data is realy send, that's why we queue the data.
     */

    entry.event.type  = event->type;
    entry.event.parm  = event->parm;

    do {
        asyncWork = kmalloc(sizeof(struct MmConnAsyncWork), GFP_KERNEL);
        if (asyncWork == NULL) {
            err = MMPBX_ERROR_NORESOURCES;
            MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
            break;
        }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33)
        if (kfifo_avail(&mmConn->event_fifo) >= sizeof(MmConnQueueEntry)) {
#else
        if (((mmConn->event_fifo->size) - kfifo_len(mmConn->event_fifo)) >= sizeof(MmConnQueueEntry)) {
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33)
            count = kfifo_in_locked(&mmConn->event_fifo, &entry, sizeof(MmConnQueueEntry), &mmConn->event_fifo_lock);
#else
            count = kfifo_put(mmConn->event_fifo, (uint8_t *)&entry, sizeof(MmConnQueueEntry));
#endif

            if (count != sizeof(MmConnQueueEntry)) {
                err = MMPBX_ERROR_NORESOURCES;
                MMPBX_TRC_ERROR("copied less bytes then expected in fifo\n");
            }
        }
        else {
            err = MMPBX_ERROR_NORESOURCES;
            MMPBX_TRC_ERROR("Not enough space in fifo\n");
            break;
        }


        asyncWork->mmConn = mmConn;
        mmSwitchInitWork(&asyncWork->work, mmConnSendEvent_work_fn);
        mmSwitchScheduleWork(&asyncWork->work);
    } while (0);

    return err;
}

/*########################################################################
#                                                                        #
#   EXPORTS                                                              #
#                                                                        #
########################################################################*/
EXPORT_SYMBOL(mmConnSendEvent);

/*########################################################################
#                                                                        #
#   PRIVATE FUNCTION DEFINITIONS                                         #
#                                                                        #
########################################################################*/

/*
 *
 */
static int mmConnParseNotify(struct sk_buff   *skb_2,
                             struct genl_info *info)
{
    int rc = 0;

    MMPBX_TRC_INFO("called\n");

    return rc;
}

/*
 *
 */
static void mmConnSendEvent_work_fn(struct work_struct *work)
{
    MmPbxError              err         = MMPBX_ERROR_NOERROR;
    struct MmConnAsyncWork  *asyncWork  = NULL;
    MmConnHndl              mmConn      = NULL;
    MmConnQueueEntry        entry;

    asyncWork = container_of(work, struct MmConnAsyncWork, work);
    mmConn    = asyncWork->mmConn;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33)
    while (kfifo_out_locked(&mmConn->event_fifo, &entry, sizeof(MmConnQueueEntry), &mmConn->event_fifo_lock) == sizeof(MmConnQueueEntry)) {
#else
    while (kfifo_get(mmConn->event_fifo, (uint8_t *)&entry, sizeof(MmConnQueueEntry)) == sizeof(MmConnQueueEntry)) {
#endif
        err = mmConnSendMmConnQueueEntry(mmConn, &entry);
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnSendMmConnQueueEntry failed with error %s\n", mmPbxGetErrorString(err));
        }
    }

    kfree(asyncWork);
}

/*
 *
 */
static MmPbxError mmConnSendMmConnQueueEntry(MmConnHndl       mmConn,
                                             MmConnQueueEntry *entry)
{
    MmPbxError      err       = MMPBX_ERROR_NOERROR;
    void            *msg_head = NULL;
    int             rc        = 0;
    struct sk_buff  *skb      = NULL;

    if (mmConn == NULL) {
        err = MMPBX_ERROR_INVALIDPARAMS;
        return err;
    }

    if (entry == NULL) {
        err = MMPBX_ERROR_INVALIDPARAMS;
        return err;
    }

    do {
        /* Prepare message to send*/
        /* Data will be send to user space when read command is received */
        /* allocate some memory, since the size is not yet known use NLMSG_GOODSIZE*/
        skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
        if (skb == NULL) {
            err = MMPBX_ERROR_NORESOURCES;
            MMPBX_TRC_ERROR("genlmsg_new failed\n");
            break;
        }

        /* create the message headers */
        /* arguments of genlmsg_put:
           struct sk_buff *,
           int (sending) pid,
           int sequence number,
           struct genl_family *,
           int flags,
           u8 command index (why do we need this?)
         */
        msg_head = genlmsg_put(skb, 0, seq_num++, &mmConnGeNlFamily, 0, MMCONN_CMD_NOTIFY);
        if (msg_head == NULL) {
            err = MMPBX_ERROR_NORESOURCES;
            MMPBX_TRC_ERROR("genlmsg_put failed\n");
            break;
        }

        MMPBX_TRC_INFO("entry->event.type: %d\n", entry->event.type);
        MMPBX_TRC_INFO("entry->event.parm: %d\n", entry->event.parm);

        /* add REF_MMCONN attribute */
        if (nla_put(skb, MMCONN_ATTR_REF_MMCONN, sizeof(void *), &mmConn) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMCONN_ATTR_EVENT, entry->event.type) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMCONN_ATTR_EVENT_PARM, entry->event.parm) < 0) {
            break;
        }

        /* finalize the message */
        genlmsg_end(skb, msg_head);


        /* send the message */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0)
        rc = genlmsg_multicast(&mmConnGeNlFamily, skb, 0, MMCONN_EVENT_GROUP, GFP_ATOMIC);
#else
        rc = genlmsg_multicast(skb, 0, mmConnGeNlMcEventGrp.id, GFP_ATOMIC);
#endif
        if (rc != 0) {
            err = MMPBX_ERROR_INTERNALERROR;
            MMPBX_TRC_ERROR("genlmsg_multicast failed with error: %d\n", rc);
        }

        return err;
    } while (0);

    genlmsg_cancel(skb, msg_head);
    nlmsg_free(skb);
    return err;
}
