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
#define MODULE_NAME    "MMCONNUSR_NETLINK"
/*########################################################################
#                                                                        #
#  HEADER (INCLUDE) SECTION                                              #
#                                                                        #
########################################################################*/
#include <net/genetlink.h>
#include <linux/workqueue.h>
#include <linux/list.h>
#include <linux/kfifo.h>
#include <net/net_namespace.h>

#include "mmconnuser_netlink_p.h"
#include "mmswitch.h"
#include "mmswitch_netlink_p.h"
#include "mmconnuser_p.h"

/*########################################################################
#                                                                        #
#  MACROS/DEFINES                                                        #
#                                                                        #
########################################################################*/

/*########################################################################
#                                                                        #
#  TYPES                                                                 #
#                                                                        #
########################################################################*/

/*########################################################################
#                                                                        #
#  PRIVATE FUNCTION PROTOTYPES                                           #
#                                                                        #
########################################################################*/
static int mmConnUsrWriteCb(struct sk_buff    *skb_2,
                            struct genl_info  *info);

static int mmConnUsrSyncCb(struct sk_buff   *skb_2,
                           struct genl_info *info);

static MmPbxError mmConnUsrSendMmConnUsrQueueEntry(MmConnUsrHndl        mmConnUsr,
                                                   MmConnUsrQueueEntry  *entry);

/*########################################################################
#                                                                        #
#  PRIVATE GLOBALS                                                       #
#                                                                        #
########################################################################*/
static int _traceLevel = MMPBX_TRACELEVEL_ERROR;
static void mmConnUsrCreateGenlFam_work_fn(struct work_struct *work);
static void mmConnUsrDestroyGenlFam_work_fn(struct work_struct *work);
static void mmConnUsrWrite_work_fn(struct work_struct *work);

/* attribute policy: defines which attribute has which type (e.g int, char * etc)
 * possible values defined in net/netlink.h
 */
static struct nla_policy mmConnUsrGeNlAttrPolicy[MMCONNUSR_ATTR_MAX + 1] =
{
    [MMCONNUSR_ATTR_PACKETHEADER] = { .type = NLA_NESTED },
    [MMCONNUSR_ATTR_DATA]         = { .type = NLA_UNSPEC },
    [MMCONNUSR_ATTR_REF_MMCONN]   = { .type = NLA_UNSPEC },
};

static struct nla_policy mmConnUsrGeNlAttrPacketHeaderPolicy[MMCONNUSR_ATTR_PACKETHEADER_MAX + 1] =
{
    [MMCONNUSR_ATTR_PACKETHEADER_TYPE] = { .type = NLA_U32 },
};



/*########################################################################
#                                                                        #
#   PUBLIC FUNCTION DEFINITIONS                                          #
#                                                                        #
########################################################################*/

/**
 *
 */
MmPbxError mmConnUsrGeNlSetTraceLevel(MmPbxTraceLevel level)
{
    _traceLevel = level;
    MMPBX_TRC_INFO("New trace level : %s\n", mmPbxGetTraceLevelString(level));

    return MMPBX_ERROR_NOERROR;
}

/**
 *
 */
MmPbxError mmConnUsrGeNlInit(void)
{
    MmPbxError err = MMPBX_ERROR_NOERROR;

    return err;
}

/**
 *
 */
MmPbxError mmConnUsrGeNlDeinit(void)
{
    MmPbxError err = MMPBX_ERROR_NOERROR;

    return err;
}

/**
 *
 */
MmPbxError mmConnUsrGeNlCreateFam(MmConnUsrHndl mmConnUsr, struct genl_info *info)
{
    MmPbxError                    err         = MMPBX_ERROR_NOERROR;
    struct MmConnUsrNlAsyncReply  *asyncReply = NULL;

    do {
        asyncReply = kmalloc(sizeof(struct MmConnUsrNlAsyncReply), GFP_KERNEL);
        if (asyncReply == NULL) {
            err = MMPBX_ERROR_NORESOURCES;
            break;
        }

        asyncReply->mmConnUsr = mmConnUsr;
        asyncReply->geNlSeq   = info->snd_seq;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
        asyncReply->geNlPid = info->snd_portid;
#else
        asyncReply->geNlPid = info->snd_pid;
#endif
        /*Workqueue implementation*/
        mmSwitchInitWork(&asyncReply->work, mmConnUsrCreateGenlFam_work_fn);
        mmSwitchScheduleWork(&asyncReply->work);
    } while (0);

    return err;
}

/**
 *
 */
MmPbxError mmConnUsrGeNlDestroyFam(MmConnUsrHndl mmConnUsr, struct genl_info *info)
{
    MmPbxError                    err         = MMPBX_ERROR_NOERROR;
    struct MmConnUsrNlAsyncReply  *asyncReply = NULL;

    do {
        asyncReply = kmalloc(sizeof(struct MmConnUsrNlAsyncReply), GFP_KERNEL);
        if (asyncReply == NULL) {
            err = MMPBX_ERROR_NORESOURCES;
            break;
        }

        asyncReply->mmConnUsr = mmConnUsr;
        asyncReply->geNlSeq   = info->snd_seq;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
        asyncReply->geNlPid = info->snd_portid;
#else
        asyncReply->geNlPid = info->snd_pid;
#endif
        /* shedule work, to avoid genl mutex deadlock */
        mmSwitchInitWork(&asyncReply->work, mmConnUsrDestroyGenlFam_work_fn);
        mmSwitchScheduleWork(&asyncReply->work);
    } while (0);

    return err;
}

/**
 *
 */
MmPbxError mmConnUsrGeNlWrite(MmConnUsrHndl       mmConnUsr,
                              MmConnPacketHeader  *header,
                              uint8_t             *buff,
                              unsigned int        bytes)
{
    MmPbxError              err         = MMPBX_ERROR_NOERROR;
    struct MmConnAsyncWork  *asyncWork  = NULL;
    unsigned int            count       = 0;

    /*
     * We will send data to socket asynchronously.
     * This is done to avoid "BUG scheduling while atomic", triggered within sock_sendmsg.
     * This means that it's possible that this function is called multiple times before
     * the data is realy send, that's why we queue the data.
     */

    if (bytes > MAX_MMCONNUSR_DATA_SIZE) {
        err = MMPBX_ERROR_NORESOURCES;
        MMPBX_TRC_ERROR("bytes > MAX_MMCONNUSR_DATA_SIZE\n");
        return err;
    }

    memset(&mmConnUsr->mmConnUsrQueueEntry, 0, sizeof(MmConnUsrQueueEntry));
    memcpy(&mmConnUsr->mmConnUsrQueueEntry.header, header, sizeof(MmConnPacketHeader));
    mmConnUsr->mmConnUsrQueueEntry.datalen = bytes;
    memcpy(mmConnUsr->mmConnUsrQueueEntry.data, buff, bytes);

    do {
        asyncWork = kmalloc(sizeof(struct MmConnAsyncWork), GFP_KERNEL);
        if (asyncWork == NULL) {
            err = MMPBX_ERROR_NORESOURCES;
            MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
            break;
        }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33)
        if (kfifo_avail(&mmConnUsr->fifo) >= sizeof(MmConnUsrQueueEntry)) {
#else
        if (((mmConnUsr->fifo->size) - kfifo_len(mmConnUsr->fifo)) >= sizeof(MmConnUsrQueueEntry)) {
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33)
            count = kfifo_in_locked(&mmConnUsr->fifo, &mmConnUsr->mmConnUsrQueueEntry, sizeof(MmConnUsrQueueEntry), &mmConnUsr->fifo_lock);
#else
            count = kfifo_put(mmConnUsr->fifo, (uint8_t *)&mmConnUsr->mmConnUsrQueueEntry, sizeof(MmConnUsrQueueEntry));
#endif

            if (count != sizeof(MmConnUsrQueueEntry)) {
                err = MMPBX_ERROR_NORESOURCES;
                MMPBX_TRC_ERROR("copied less bytes then expected in fifo\n");
                break;
            }
        }
        else {
            err = MMPBX_ERROR_NORESOURCES;
            MMPBX_TRC_ERROR("Not enough space in fifo\n");
            break;
        }


        /* Use dedicated thread */
        asyncWork->mmConn = (MmConnHndl) mmConnUsr;
        mmSwitchInitWork(&asyncWork->work, mmConnUsrWrite_work_fn);
        mmSwitchScheduleWork(&asyncWork->work);
    } while (0);

    return err;
}

/*########################################################################
#                                                                        #
#   PRIVATE FUNCTION DEFINITIONS                                         #
#                                                                        #
########################################################################*/

/**
 *
 */
static void mmConnUsrCreateGenlFam_work_fn(struct work_struct *work)
{
    MmPbxError                    err         = MMPBX_ERROR_NOERROR;
    struct MmConnUsrNlAsyncReply  *asyncReply = NULL;
    MmConnUsrHndl                 mmConnUsr   = NULL;
    int                           ret         = 0;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 31)
    int i;
#endif

    asyncReply  = container_of(work, struct MmConnUsrNlAsyncReply, work);
    mmConnUsr   = asyncReply->mmConnUsr;

    /* Prepare family for usage */
    mmConnUsr->geNlFam.id = GENL_ID_GENERATE; //genetlink should generate an id
    snprintf(mmConnUsr->geNlFam.name, sizeof(mmConnUsr->geNlFam.name), "%lx", (unsigned long int) mmConnUsr);
    mmConnUsr->geNlFam.hdrsize  = 0;
    mmConnUsr->geNlFam.version  = MMCONNUSR_GENL_FAMILY_VERSION;
    mmConnUsr->geNlFam.maxattr  = MMCONNUSR_ATTR_MAX;

    /* Fill operations */
    mmConnUsr->geNlOps[0].cmd     = MMCONNUSR_CMD_WRITE;
    mmConnUsr->geNlOps[0].flags   = 0;
    mmConnUsr->geNlOps[0].policy  = mmConnUsrGeNlAttrPolicy;
    mmConnUsr->geNlOps[0].doit    = mmConnUsrWriteCb;
    mmConnUsr->geNlOps[0].dumpit  = NULL;

    mmConnUsr->geNlOps[1].cmd     = MMCONNUSR_CMD_SYNC;
    mmConnUsr->geNlOps[1].flags   = 0;
    mmConnUsr->geNlOps[1].policy  = mmConnUsrGeNlAttrPolicy;
    mmConnUsr->geNlOps[1].doit    = mmConnUsrSyncCb;
    mmConnUsr->geNlOps[1].dumpit  = NULL;

    do {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
        /*register new family and commands (functions) of the new family*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0)
        ret = genl_register_family_with_ops(&mmConnUsr->geNlFam, mmConnUsr->geNlOps);
#else
        ret = genl_register_family_with_ops(&mmConnUsr->geNlFam, mmConnUsr->geNlOps, ARRAY_SIZE(mmConnUsr->geNlOps));
#endif
        if (ret != 0) {
            MMPBX_TRC_ERROR("genl_register_family_with_ops failed with %d\n", ret);
            break;
        }
        MMPBX_TRC_INFO("mmConnUsr->geNlFam.name: %s\n", mmConnUsr->geNlFam.name);
#else
        ret = genl_register_family(&mmConnUsr->geNlFam);
        if (ret != 0) {
            MMPBX_TRC_ERROR("genl_register_family failed\n");
            err = MMPBX_ERROR_INTERNALERROR;
            break;
        }

        for (i = 0; i < ARRAY_SIZE(mmConnUsr->geNlOps); i++) {
            ret = genl_register_ops(&mmConnUsr->geNlFam, &mmConnUsr->geNlOps[i]);
            if (ret != 0) {
                MMPBX_TRC_ERROR("genl_register_ops failed\n");
                err = MMPBX_ERROR_INTERNALERROR;
                break;
            }
        }
        MMPBX_TRC_INFO("mmConnUsr->geNlFam.name: %s\n", mmConnUsr->geNlFam.name);
#endif
        // Send a netlink reponse
        if (mmSwitchMmConnUsrSendConstructReply(mmConnUsr, asyncReply->geNlSeq, asyncReply->geNlPid) != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmSwitchMmConnUsrSendConstructReply failed \n");
            err = MMPBX_ERROR_INTERNALERROR;
            break;
        }
    } while (0);

    if (err != MMPBX_ERROR_NOERROR) {
        /*Something went wrong, cleanup */
        genl_unregister_family(&mmConnUsr->geNlFam);

        if (mmConnUsr != NULL) {
            err = mmConnDestruct((MmConnHndl *)&mmConnUsr);
            if (err != MMPBX_ERROR_NOERROR) {
                MMPBX_TRC_ERROR("mmConnUsrDestruct failed\n");
            }
        }
    }

    kfree(asyncReply);
}

/**
 *
 */
static void mmConnUsrDestroyGenlFam_work_fn(struct work_struct *work)
{
    struct MmConnUsrNlAsyncReply  *asyncReply = NULL;
    MmConnUsrHndl                 mmConnUsr   = NULL;
    int                           ret         = 0;

    asyncReply  = container_of(work, struct MmConnUsrNlAsyncReply, work);
    mmConnUsr   = asyncReply->mmConnUsr;

    MMPBX_TRC_INFO("mmConnUsr->geNlFam.name: %s\n", mmConnUsr->geNlFam.name);

    do {
        /*unregister the family*/
        ret = genl_unregister_family(&mmConnUsr->geNlFam);
        if (ret != 0) {
            MMPBX_TRC_ERROR("unregister family failed with: %d\n", ret);
            /* Do not break otherwise the mmpbx mmswitch will hang for ever*/
        }

        /* This will indicate that the family is well unregistered */
        mmConnUsr->geNlFam.id = GENL_ID_GENERATE;

        // Send a netlink reponse
        if (mmSwitchMmConnUsrSendDestoyGenlFamReply(mmConnUsr, asyncReply->geNlSeq, asyncReply->geNlPid) != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmSwitchMmConnUsrSendDestoyGenlFamReply failed \n");
            break;
        }
    } while (0);

    kfree(asyncReply);
}

/**
 *
 */
static void mmConnUsrWrite_work_fn(struct work_struct *work)
{
    MmPbxError              err         = MMPBX_ERROR_NOERROR;
    struct MmConnAsyncWork  *asyncWork  = NULL;
    MmConnUsrHndl           mmConnUsr   = NULL;
    MmConnUsrQueueEntry     *entry      = NULL;

    asyncWork = container_of(work, struct MmConnAsyncWork, work);
    mmConnUsr = (MmConnUsrHndl) asyncWork->mmConn;

    /* Dequeue fifo */
    entry = (MmConnUsrQueueEntry *) kmalloc(sizeof(MmConnUsrQueueEntry), GFP_ATOMIC);
    if (entry == NULL) {
        MMPBX_TRC_ERROR("kmalloc sizeof(MmConnUsrQueueEntry) failed\n");
        return;
    }
    memset(entry, 0, sizeof(MmConnUsrQueueEntry));

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33)
    while (kfifo_out_locked(&mmConnUsr->fifo, entry, sizeof(MmConnUsrQueueEntry), &mmConnUsr->fifo_lock) == sizeof(MmConnUsrQueueEntry)) {
#else
    while (kfifo_get(mmConnUsr->fifo, (uint8_t *)entry, sizeof(MmConnUsrQueueEntry)) == sizeof(MmConnUsrQueueEntry)) {
#endif

        err = mmConnUsrSendMmConnUsrQueueEntry(mmConnUsr, entry);
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnUsrSendMmConnUsrQueueEntry failed with error %s\n", mmPbxGetErrorString(err));
        }
    }

    kfree(entry);
    kfree(asyncWork);
}

/**
 *
 */
static int mmConnUsrWriteCb(struct sk_buff    *skb_2,
                            struct genl_info  *info)
{
    MmPbxError          err       = MMPBX_ERROR_NOERROR;
    MmConnUsrHndl       mmConnUsr = NULL;
    MmConnPacketHeader  header;
    struct nlattr       *parentAttr;
    struct nlattr       *tb[MMCONNUSR_ATTR_MAX + 1];
    int                 rc = 0;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /* Parse REF_MMCONN attribute */
    parentAttr = info->attrs[MMCONNUSR_ATTR_REF_MMCONN];
    if (!parentAttr) {
        return -EINVAL;
    }
    nla_memcpy(&mmConnUsr, parentAttr, nla_len(parentAttr));

    /*Parse PACKETHEADER attribute */
    parentAttr = info->attrs[MMCONNUSR_ATTR_PACKETHEADER];
    if (!parentAttr) {
        return -EINVAL;
    }
    rc = nla_parse_nested(tb, MMCONNUSR_ATTR_PACKETHEADER_MAX, parentAttr, mmConnUsrGeNlAttrPacketHeaderPolicy);
    if (rc < 0) {
        MMPBX_TRC_ERROR("nla_parse_nested failed\n");
        return rc;
    }

    if (!tb[MMCONNUSR_ATTR_PACKETHEADER_TYPE]) {
        return -EINVAL;
    }

    header.type = nla_get_u32(tb[MMCONNUSR_ATTR_PACKETHEADER_TYPE]);

    /*Parse DATA attribute */
    parentAttr = info->attrs[MMCONNUSR_ATTR_DATA];
    if (!parentAttr) {
        return -EINVAL;
    }

    err = mmConnWrite((MmConnHndl)mmConnUsr, &header, (uint8_t *) nla_data(parentAttr), nla_len(parentAttr));
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnWrite failed with: %s\n", mmPbxGetErrorString(err));
    }


    return rc;
}

/**
 *
 */
static int mmConnUsrSyncCb(struct sk_buff   *skb_2,
                           struct genl_info *info)
{
    MmConnUsrHndl mmConnUsr = NULL;
    struct nlattr *parentAttr;
    int           rc = 0;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /* Parse REF_MMCONN attribute */
    parentAttr = info->attrs[MMCONNUSR_ATTR_REF_MMCONN];
    if (!parentAttr) {
        return -EINVAL;
    }
    nla_memcpy(&mmConnUsr, parentAttr, nla_len(parentAttr));

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
    mmConnUsr->geNlPid = info->snd_portid;
#else
    mmConnUsr->geNlPid = info->snd_pid;
#endif
    MMPBX_TRC_INFO("nlpid: %d\n", mmConnUsr->geNlPid);

    mmConnUsr->geNlSeq = info->snd_seq;
    MMPBX_TRC_INFO("nlseq: %d\n", mmConnUsr->geNlSeq);

    return rc;
}

static MmPbxError mmConnUsrSendMmConnUsrQueueEntry(MmConnUsrHndl        mmConnUsr,
                                                   MmConnUsrQueueEntry  *entry)
{
    MmPbxError      err   = MMPBX_ERROR_NOERROR;
    struct sk_buff  *skb  = NULL;
    struct nlattr   *nest;
    void            *msg_head = NULL;
    int             rc        = 0;

    if (mmConnUsr == NULL) {
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
        mmConnUsr->geNlSeq  += 1;
        msg_head            = genlmsg_put(skb, mmConnUsr->geNlPid, mmConnUsr->geNlSeq, &mmConnUsr->geNlFam, 0, MMCONNUSR_CMD_WRITE);
        if (msg_head == NULL) {
            err = MMPBX_ERROR_NORESOURCES;
            MMPBX_TRC_ERROR("genlmsg_put failed\n");
            break;
        }

        /* add PACKETHEADER attribute */
        nest = nla_nest_start(skb, MMCONNUSR_ATTR_PACKETHEADER);
        if (!nest) {
            err = MMPBX_ERROR_NORESOURCES;
            MMPBX_TRC_ERROR("nla_nest_start failed\n");
            break;
        }

        if (nla_put_u32(skb, MMCONNUSR_ATTR_PACKETHEADER_TYPE, entry->header.type) < 0) {
            break;
        }

        nla_nest_end(skb, nest);

        /* add DATA attribute */
        if (nla_put(skb, MMCONNUSR_ATTR_DATA, entry->datalen, entry->data) < 0) {
            break;
        }

        /* finalize the message */
        genlmsg_end(skb, msg_head);

        /* send the message */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
        rc = genlmsg_unicast(&init_net, skb, mmConnUsr->geNlPid);
#else
        rc = genlmsg_unicast(skb, mmConnUsr->geNlPid);
#endif
        if (rc != 0) {
            err = MMPBX_ERROR_NORESOURCES;
            MMPBX_TRC_ERROR("genlmsg_unicast failed with error: %d\n", rc);
        }

        return err;
    } while (0);

    genlmsg_cancel(skb, msg_head);
    nlmsg_free(skb);
    return err;
}
