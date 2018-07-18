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
#define MODULE_NAME    "[MMSWITCH_NETLINK]"
/*########################################################################
#                                                                        #
#  HEADER (INCLUDE) SECTION                                              #
#                                                                        #
########################################################################*/
#include <linux/jiffies.h>
#include <net/genetlink.h>
#include <net/net_namespace.h>

#include "mmswitch.h"
#include "mmswitch_netlink_p.h"
#include "mmconn_p.h"
#include "mmconnuser_p.h"
#include "mmconnuser_netlink_p.h"
#include "mmconnkernel_p.h"
#include "mmconnmulticast_p.h"
#include "mmconnrelay_p.h"
#include "mmconnrtcp_p.h"
#include "mmconntone_p.h"

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
static int mmSwitchFillMmConnAttr(struct sk_buff  *skb,
                                  MmConnHndl      mmConn);
static int mmSwitchFillMmConnUsrAttr(struct sk_buff *skb,
                                     MmConnUsrHndl  mmConnUsr);
static int mmSwitchParseMmConnDestruct(struct sk_buff   *skb_2,
                                       struct genl_info *info);
static int mmSwitchParseMmConnXConn(struct sk_buff    *skb_2,
                                    struct genl_info  *info);
static int mmSwitchParseMmConnDisc(struct sk_buff   *skb_2,
                                   struct genl_info *info);
static int mmSwitchParseMmConnSetTraceLevel(struct sk_buff    *skb_2,
                                            struct genl_info  *info);

static int mmSwitchParseMmConnUsrConstruct(struct sk_buff   *skb_2,
                                           struct genl_info *info);
static int mmSwitchParseMmConnUsrSync(struct sk_buff    *skb_2,
                                      struct genl_info  *info);
static int mmSwitchParseMmConnUsrDestroyGeNlFam(struct sk_buff    *skb_2,
                                                struct genl_info  *info);
static int mmSwitchParseMmConnUsrSetTraceLevel(struct sk_buff   *skb_2,
                                               struct genl_info *info);

static int mmSwitchParseMmConnKrnlConstruct(struct sk_buff    *skb_2,
                                            struct genl_info  *info);
static int mmSwitchParseMmConnKrnlSetTraceLevel(struct sk_buff    *skb_2,
                                                struct genl_info  *info);

static int mmSwitchParseMmConnMulticastConstruct(struct sk_buff   *skb_2,
                                                 struct genl_info *info);
static int mmSwitchParseMmConnMulticastSetTraceLevel(struct sk_buff   *skb_2,
                                                     struct genl_info *info);
static int mmSwitchParseMmConnMulticastAddSink(struct sk_buff   *skb_2,
                                               struct genl_info *info);
static int mmSwitchParseMmConnMulticastRemoveSink(struct sk_buff    *skb_2,
                                                  struct genl_info  *info);

static int mmSwitchParseMmConnToneConstruct(struct sk_buff    *skb_2,
                                            struct genl_info  *info);
static int mmSwitchParseMmConnToneSetTraceLevel(struct sk_buff    *skb_2,
                                                struct genl_info  *info);
static int mmSwitchParseMmConnToneSendPattern(struct sk_buff    *skb_2,
                                              struct genl_info  *info);

static int mmSwitchParseMmConnRelayConstruct(struct sk_buff   *skb_2,
                                             struct genl_info *info);
static int mmSwitchParseMmConnRelaySetTraceLevel(struct sk_buff   *skb_2,
                                                 struct genl_info *info);

static int mmSwitchParseMmConnRtcpConstruct(struct sk_buff    *skb_2,
                                            struct genl_info  *info);
static int mmSwitchParseMmConnRtcpSetTraceLevel(struct sk_buff    *skb_2,
                                                struct genl_info  *info);
static int mmSwitchParseMmConnRtcpModPacketType(struct sk_buff    *skb_2,
                                                struct genl_info  *info);
static int mmSwitchParseMmConnRtcpModGenRtcp(struct sk_buff   *skb_2,
                                             struct genl_info *info);
static int mmSwitchParseMmConnRtcpModRemoteMediaAddr(struct sk_buff   *skb_2,
                                                     struct genl_info *info);
static int mmSwitchParseMmConnRtcpModRemoteRtcpAddr(struct sk_buff    *skb_2,
                                                    struct genl_info  *info);
static int mmSwitchParseMmConnRtcpModRtcpBandwidth(struct sk_buff   *skb_2,
                                                   struct genl_info *info);
static int mmSwitchParseMmConnRtcpGetStats(struct sk_buff   *skb_2,
                                           struct genl_info *info);
static int mmSwitchParseMmConnRtcpResetStats(struct sk_buff   *skb_2,
                                             struct genl_info *info);
static int mmSwitchFillMmConnRtcpAttr(struct sk_buff  *skb,
                                      MmConnRtcpHndl  mmConnRtcp);

/*########################################################################
#                                                                        #
#  PRIVATE GLOBALS                                                       #
#                                                                        #
########################################################################*/
MMPBX_TRACEDEF(MMPBX_TRACELEVEL_ERROR);

/* attribute policy: defines which attribute has which type (e.g int, char * etc)
 * possible values defined in net/netlink.h
 */
static struct nla_policy mmSwitchGeNlAttrPolicy[MMSWITCH_ATTR_MAX + 1] =
{
    [MMSWITCH_ATTR_MMCONN]                        = { .type = NLA_NESTED },
    [MMSWITCH_ATTR_MMCONNUSR]                     = { .type = NLA_NESTED },
    [MMSWITCH_ATTR_MMCONNRTCP]                    = { .type = NLA_NESTED },
    [MMSWITCH_ATTR_MMCONNKRNL_ENDPOINT_ID]        = { .type = NLA_U32    },
    [MMSWITCH_ATTR_MMPBX_TRACELEVEL]              = { .type = NLA_U32    },
    [MMSWITCH_ATTR_ENCODINGNAME]                  = { .type = NLA_STRING },
    [MMSWITCH_ATTR_REF_MMCONN]                    = { .type = NLA_UNSPEC },
    [MMSWITCH_ATTR_LOCAL_SOCKFD]                  = { .type = NLA_U32    },
    [MMSWITCH_ATTR_LOCAL_RTCP_SOCKFD]             = { .type = NLA_U32    },
    [MMSWITCH_ATTR_REMOTE_ADDR]                   = { .type = NLA_UNSPEC },
    [MMSWITCH_ATTR_REMOTE_RTCP_ADDR]              = { .type = NLA_UNSPEC },
    [MMSWITCH_ATTR_PACKET_TYPE]                   = { .type = NLA_U32    },
    [MMSWITCH_ATTR_TIMEOUT]                       = { .type = NLA_U32    },
    [MMSWITCH_ATTR_MUTE_TIME]                     = { .type = NLA_U32    },
    [MMSWITCH_ATTR_RTCP_BANDWIDTH]                = { .type = NLA_U32    },
    [MMSWITCH_ATTR_GEN_RTCP]                      = { .type = NLA_U32    },
    [MMSWITCH_ATTR_MMCONNTONE_ENDPOINT_ID]        = { .type = NLA_U32    },
    [MMSWITCH_ATTR_MMCONNTONE_TYPE]               = { .type = NLA_U32    },
    [MMSWITCH_ATTR_MMCONNTONE_PATTERN]            = { .type = NLA_NESTED },
    [MMSWITCH_ATTR_MMCONNTONE_PATTERNTABLE_SIZE]  = { .type = NLA_U32    },
};


static struct nla_policy mmSwitchGeNlAttrMmConnPolicy[MMSWITCH_ATTR_MMCONN_MAX + 1] =
{
    [MMSWITCH_ATTR_MMCONN_REF] = { .type = NLA_UNSPEC },
};

static struct nla_policy mmSwitchGeNlAttrMmConnTonePatternPolicy[MMSWITCH_ATTR_MMCONNTONE_PATTERN_MAX + 1] =
{
    [MMSWITCH_ATTR_MMCONNTONE_PATTERN_ID]               = { .type = NLA_U32 },
    [MMSWITCH_ATTR_MMCONNTONE_PATTERN_ON]               = { .type = NLA_U32 },
    [MMSWITCH_ATTR_MMCONNTONE_PATTERN_FREQ1]            = { .type = NLA_U32 },
    [MMSWITCH_ATTR_MMCONNTONE_PATTERN_FREQ2]            = { .type = NLA_U32 },
    [MMSWITCH_ATTR_MMCONNTONE_PATTERN_FREQ3]            = { .type = NLA_U32 },
    [MMSWITCH_ATTR_MMCONNTONE_PATTERN_FREQ4]            = { .type = NLA_U32 },
    [MMSWITCH_ATTR_MMCONNTONE_PATTERN_POWER1]           = { .type = NLA_U32 },
    [MMSWITCH_ATTR_MMCONNTONE_PATTERN_POWER2]           = { .type = NLA_U32 },
    [MMSWITCH_ATTR_MMCONNTONE_PATTERN_POWER3]           = { .type = NLA_U32 },
    [MMSWITCH_ATTR_MMCONNTONE_PATTERN_POWER4]           = { .type = NLA_U32 },
    [MMSWITCH_ATTR_MMCONNTONE_PATTERN_DURATION]         = { .type = NLA_U32 },
    [MMSWITCH_ATTR_MMCONNTONE_PATTERN_NEXTID]           = { .type = NLA_U32 },
    [MMSWITCH_ATTR_MMCONNTONE_PATTERN_MAXLOOPS]         = { .type = NLA_U32 },
    [MMSWITCH_ATTR_MMCONNTONE_PATTERN_NEXTIDAFTERLOOPS] = { .type = NLA_U32 },
};


/* family definition */
static struct genl_family MmSwitchGeNlFamily =
{
    .id       = GENL_ID_GENERATE,       //genetlink should generate an id
    .hdrsize  =                 0,
    .name     = MMSWITCH_GENL_FAMILY,                               //the name of this family, used by userspace application
    .version  = MMSWITCH_GENL_FAMILY_VERSION,                       //version number
    .maxattr  = MMSWITCH_ATTR_MAX,
};

/* commands: mapping between the command enumeration and the actual function*/
static struct genl_ops mmSwitchGeNlOps[] =
{
    {
        .cmd    = MMSWITCH_CMD_MMCONN_DESTRUCT,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnDestruct,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONN_XCONN,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnXConn,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONN_DISC,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnDisc,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONN_SET_TRACE_LEVEL,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnSetTraceLevel,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNUSR_CONSTRUCT,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnUsrConstruct,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNUSR_SYNC,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnUsrSync,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNUSR_DESTROY_GENL_FAM,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnUsrDestroyGeNlFam,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNUSR_SET_TRACE_LEVEL,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnUsrSetTraceLevel,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNKRNL_CONSTRUCT,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnKrnlConstruct,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNKRNL_SET_TRACE_LEVEL,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnKrnlSetTraceLevel,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNMULTICAST_CONSTRUCT,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnMulticastConstruct,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNMULTICAST_SET_TRACE_LEVEL,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnMulticastSetTraceLevel,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNMULTICAST_ADD_SINK,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnMulticastAddSink,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNMULTICAST_REMOVE_SINK,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnMulticastRemoveSink,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNTONE_CONSTRUCT,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnToneConstruct,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNTONE_SET_TRACE_LEVEL,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnToneSetTraceLevel,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNTONE_SEND_PATTERN,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnToneSendPattern,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNRELAY_CONSTRUCT,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnRelayConstruct,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNRELAY_SET_TRACE_LEVEL,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnRelaySetTraceLevel,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNRTCP_CONSTRUCT,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnRtcpConstruct,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNRTCP_SET_TRACE_LEVEL,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnRtcpSetTraceLevel,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNRTCP_MOD_PACKET_TYPE,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnRtcpModPacketType,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNRTCP_MOD_GEN_RTCP,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnRtcpModGenRtcp,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNRTCP_MOD_REMOTE_MEDIA_ADDR,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnRtcpModRemoteMediaAddr,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNRTCP_MOD_REMOTE_RTCP_ADDR,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnRtcpModRemoteRtcpAddr,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNRTCP_MOD_RTCPBANDWIDTH,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnRtcpModRtcpBandwidth,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNRTCP_GET_STATS,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnRtcpGetStats,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNRTCP_RESET_STATS,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnRtcpResetStats,
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
MmPbxError mmSwitchGeNlSetTraceLevel(MmPbxTraceLevel level)
{
    _traceLevel = level;
    MMPBX_TRC_DEBUG("New trace level : %s\n", mmPbxGetTraceLevelString(level));


    return MMPBX_ERROR_NOERROR;
}

/**
 *
 */
MmPbxError mmSwitchGeNlInit(void)
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
        ret = genl_register_family_with_ops(&MmSwitchGeNlFamily, mmSwitchGeNlOps);
#else
        ret = genl_register_family_with_ops(&MmSwitchGeNlFamily, mmSwitchGeNlOps, ARRAY_SIZE(mmSwitchGeNlOps));
#endif
        if (ret != 0) {
            MMPBX_TRC_ERROR("genl_register_family_with_ops failed\n");
            err = MMPBX_ERROR_INTERNALERROR;
            break;
        }
#else
        ret = genl_register_family(&MmSwitchGeNlFamily);
        if (ret != 0) {
            MMPBX_TRC_ERROR("genl_register_family failed\n");
            err = MMPBX_ERROR_INTERNALERROR;
            break;
        }

        for (i = 0; i < ARRAY_SIZE(mmSwitchGeNlOps); i++) {
            ret = genl_register_ops(&MmSwitchGeNlFamily, &mmSwitchGeNlOps[i]);
            if (ret != 0) {
                MMPBX_TRC_ERROR("genl_register_ops failed\n");
                err = MMPBX_ERROR_INTERNALERROR;
                break;
            }
        }
#endif
    } while (0);

    if (err != MMPBX_ERROR_NOERROR) {
        genl_unregister_family(&MmSwitchGeNlFamily);
    }
    return err;
}

/**
 *
 */
MmPbxError mmSwitchGeNlDeinit(void)
{
    MmPbxError  err = MMPBX_ERROR_NOERROR;
    int         ret = 0;


    do {
        /*unregister the family*/
        ret = genl_unregister_family(&MmSwitchGeNlFamily);
        if (ret != 0) {
            MMPBX_TRC_ERROR("unregister family failed with: %i\n", ret);
            err = MMPBX_ERROR_INTERNALERROR;
            break;
        }
    } while (0);

    return err;
}

/*
 * @brief Craft a netlink reponse to an MMSWITCH_CMD_MMCONNUSR_DESTROY_GENL_FAM request
 */
MmPbxError mmSwitchMmConnUsrSendDestoyGenlFamReply(MmConnUsrHndl mmConnUsr, u32 geNlSeq, u32 geNlPid)
{
    MmPbxError      err       = MMPBX_ERROR_NOERROR;
    struct sk_buff  *skb      = NULL;
    int             rc        = 0;
    void            *msg_head = NULL;

    do {
        /* send a message back*/
        /* allocate some memory, since the size is not yet known use NLMSG_GOODSIZE*/
        skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
        if (skb == NULL) {
            MMPBX_TRC_ERROR("genlmsg_new failed\n");
            err = MMPBX_ERROR_NORESOURCES;
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
        msg_head = genlmsg_put(skb, 0, geNlSeq, &MmSwitchGeNlFamily, 0, MMSWITCH_CMD_MMCONNUSR_DESTROY_GENL_FAM);
        if (msg_head == NULL) {
            MMPBX_TRC_ERROR("genlmsg_put failed\n");
            err = MMPBX_ERROR_INTERNALERROR;
            break;
        }

        /* add MMCONN attribute */
        rc = mmSwitchFillMmConnAttr(skb, (MmConnHndl) mmConnUsr);
        if (rc != 0) {
            MMPBX_TRC_ERROR("mmSwitchFillMmConnAttr failed\n");
            err = MMPBX_ERROR_INTERNALERROR;
            break;
        }

        /* finalize the message */
        genlmsg_end(skb, msg_head);

        /* send the message back */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
        rc = genlmsg_unicast(&init_net, skb, geNlPid);
#else
        rc = genlmsg_unicast(skb, mmConnUsr->geNlPid);
#endif
        if (rc != 0) {
            err = MMPBX_ERROR_INTERNALERROR;
            MMPBX_TRC_ERROR("genlmsg_unicast failed\n");
            break;
        }
    } while (0);

    if ((err != MMPBX_ERROR_NOERROR) && (skb != NULL)) {
        genlmsg_cancel(skb, msg_head);
        nlmsg_free(skb);
    }

    return err;
}

/*
 * @brief Craft a netlink reponse to an MMSWITCH_CMD_MMCONNUSR_CONSTRUCT request
 */
MmPbxError mmSwitchMmConnUsrSendConstructReply(MmConnUsrHndl mmConnUsr, u32 geNlSeq, u32 geNlPid)
{
    MmPbxError      err       = MMPBX_ERROR_NOERROR;
    struct sk_buff  *skb      = NULL;
    int             rc        = 0;
    void            *msg_head = NULL;

    do {
        /* send a message back*/
        /* allocate some memory, since the size is not yet known use NLMSG_GOODSIZE*/
        skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
        if (skb == NULL) {
            MMPBX_TRC_ERROR("genlmsg_new failed\n");
            err = MMPBX_ERROR_NORESOURCES;
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
        msg_head = genlmsg_put(skb, 0, geNlSeq, &MmSwitchGeNlFamily, 0, MMSWITCH_CMD_MMCONNUSR_CONSTRUCT);
        if (msg_head == NULL) {
            MMPBX_TRC_ERROR("genlmsg_put failed\n");
            err = MMPBX_ERROR_INTERNALERROR;
            break;
        }

        /* add MMCONN attribute */
        rc = mmSwitchFillMmConnAttr(skb, (MmConnHndl) mmConnUsr);
        if (rc != 0) {
            MMPBX_TRC_ERROR("mmSwitchFillMmConnAttr failed\n");
            err = MMPBX_ERROR_INTERNALERROR;
            break;
        }

        /* finalize the message */
        genlmsg_end(skb, msg_head);

        /* send the message back */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
        rc = genlmsg_unicast(&init_net, skb, geNlPid);
#else
        rc = genlmsg_unicast(skb, geNlPid);
#endif
        if (rc != 0) {
            err = MMPBX_ERROR_INTERNALERROR;
            MMPBX_TRC_ERROR("genlmsg_unicast failed\n");
            break;
        }
    } while (0);

    if ((err != MMPBX_ERROR_NOERROR) && (skb != NULL)) {
        genlmsg_cancel(skb, msg_head);
        nlmsg_free(skb);
    }

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
static int mmSwitchFillMmConnAttr(struct sk_buff  *skb,
                                  MmConnHndl      mmConn)
{
    struct nlattr *nest;

    do {
        nest = nla_nest_start(skb, MMSWITCH_ATTR_MMCONN);
        if (!nest) {
            break;
        }
        if (nla_put(skb, MMSWITCH_ATTR_MMCONN_REF, sizeof(void *), &mmConn) < 0) {
            break;
        }

        nla_nest_end(skb, nest);
        return 0;
    } while (0);

    MMPBX_TRC_ERROR("an error occured in %s\n", __func__);

    return -1;
}

/**
 *
 */
static int mmSwitchFillMmConnUsrAttr(struct sk_buff *skb,
                                     MmConnUsrHndl  mmConnUsr)
{
    struct nlattr *nest;


    nest = nla_nest_start(skb, MMSWITCH_ATTR_MMCONNUSR);
    do {
        if (!nest) {
            break;
        }

        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNUSR_GENL_FAMID, mmConnUsr->geNlFam.id) < 0) {
            break;
        }

        if (nla_put_string(skb, MMSWITCH_ATTR_MMCONNUSR_GENL_FAMNAME, mmConnUsr->geNlFam.name) < 0) {
            break;
        }

        nla_nest_end(skb, nest);
        return 0;
    } while (0);

    MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
    return -1;
}

static void mmConnDistructWorkFn(struct work_struct *work)
{
    struct MmConnAsyncWork *asyncWork = NULL;

    asyncWork = container_of(work, struct MmConnAsyncWork, work);
    if (mmConnDestruct(&asyncWork->mmConn) != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnDestruct failed\n");
    }
    kfree(asyncWork);
}

/*
 *
 */
static int mmSwitchParseMmConnDestruct(struct sk_buff   *skb_2,
                                       struct genl_info *info)
{
    MmConnHndl              mmConn = NULL;
    struct nlattr           *nla;
    int                     rc          = 0;
    struct MmConnAsyncWork  *asyncWork  = NULL;

    do {
        if (info == NULL) {
            MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
            rc = -1;
            break;
        }

        /*Parse attribute */
        nla = info->attrs[MMSWITCH_ATTR_REF_MMCONN];
        if (!nla) {
            rc = -EINVAL;
            break;
        }

        nla_memcpy(&mmConn, nla, sizeof(void *));

        asyncWork = kmalloc(sizeof(struct MmConnAsyncWork), GFP_KERNEL);
        if (asyncWork == NULL) {
            rc = -ENOMEM;
            break;
        }

        asyncWork->mmConn = mmConn;
        mmSwitchInitWork(&asyncWork->work, mmConnDistructWorkFn);
        mmSwitchScheduleWork(&asyncWork->work);
    } while (false);

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnXConn(struct sk_buff    *skb_2,
                                    struct genl_info  *info)
{
    MmPbxError    err     = MMPBX_ERROR_NOERROR;
    MmConnHndl    source  = NULL;
    MmConnHndl    target  = NULL;
    struct nlattr *parentAttr;
    struct nlattr *tb[MMSWITCH_ATTR_MAX + 1];
    int           rc = 0;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse MMCONN attribute */
    parentAttr = info->attrs[MMSWITCH_ATTR_MMCONN];
    if (!parentAttr) {
        return -EINVAL;
    }
    rc = nla_parse_nested(tb, MMSWITCH_ATTR_MMCONN_MAX, parentAttr, mmSwitchGeNlAttrMmConnPolicy);
    if (rc < 0) {
        MMPBX_TRC_ERROR("nla_parse_nested failed\n");
        return rc;
    }

    if (!tb[MMSWITCH_ATTR_MMCONN_REF]) {
        MMPBX_TRC_ERROR("ATTR_MCONN_REF not found !\n");
        return -EINVAL;
    }

    nla_memcpy(&source, tb[MMSWITCH_ATTR_MMCONN_REF], sizeof(void *));

    /*Parse REF_MMCONN attribute */
    parentAttr = info->attrs[MMSWITCH_ATTR_REF_MMCONN];
    if (!parentAttr) {
        MMPBX_TRC_ERROR("ATTR_MCONN_REF_MMCONN not found !\n");
        return -EINVAL;
    }
    nla_memcpy(&target, parentAttr, sizeof(void *));

    err = mmConnXConn(source, target);
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnXConn failed\n");
        rc = -1;
    }



    return rc;
}

static void mmConnDiscWorkFn(struct work_struct *work)
{
    int                       rc          = 0;
    struct MmConnNlAsyncReply *asyncReply = NULL;
    void                      *msg_head   = NULL;
    struct sk_buff            *skb        = NULL;

    do {
        asyncReply = container_of(work, struct MmConnNlAsyncReply, work);

        if (mmConnDisc(asyncReply->mmConn) != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnDisc failed\n");
            rc = -1;
            break;
        }

        /* send a message back*/
        /* allocate some memory, since the size is not yet known use NLMSG_GOODSIZE*/
        skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
        if (skb == NULL) {
            MMPBX_TRC_ERROR("genlmsg_new failed\n");
            rc = -1;
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
        msg_head = genlmsg_put(skb, 0, asyncReply->geNlSeq, &MmSwitchGeNlFamily, 0, MMSWITCH_CMD_MMCONN_DISC);
        if (msg_head == NULL) {
            MMPBX_TRC_ERROR("genlmsg_put failed\n");
            rc = -ENOMEM;
            break;
        }

        /* add MMCONNRTCP attribute */
        rc = mmSwitchFillMmConnAttr(skb, asyncReply->mmConn);
        if (rc != 0) {
            MMPBX_TRC_ERROR("mmSwitchFillMmConnAttr failed\n");
            rc = -ENOMEM;
            break;
        }

        /* finalize the message */
        genlmsg_end(skb, msg_head);

        /* send the message back */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
        rc = genlmsg_unicast(&init_net, skb, asyncReply->geNlPid);
#else
        rc = genlmsg_unicast(skb, asyncReply->geNlPid);
#endif
        if (rc != 0) {
            MMPBX_TRC_ERROR("genlmsg_unicast failed\n");
            break;
        }
    } while (0);

    if ((rc != 0) && (skb != NULL)) {
        genlmsg_cancel(skb, msg_head);
        nlmsg_free(skb);
    }
    kfree(asyncReply);
}

int _scheduleMmConnDisc(MmConnHndl mmConn, struct genl_info *info)
{
    int                       rc          = 0;
    struct MmConnNlAsyncReply *asyncReply = NULL;

    do {
        asyncReply = kmalloc(sizeof(struct MmConnNlAsyncReply), GFP_KERNEL);
        if (asyncReply == NULL) {
            rc = -ENOMEM;
            break;
        }

        asyncReply->mmConn  = mmConn;
        asyncReply->geNlSeq = info->snd_seq;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
        asyncReply->geNlPid = info->snd_portid;
#else
        asyncReply->geNlPid = info->snd_pid;
#endif
        mmSwitchInitWork(&asyncReply->work, mmConnDiscWorkFn);
        mmSwitchScheduleWork(&asyncReply->work);
    } while (0);

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnDisc(struct sk_buff   *skb_2,
                                   struct genl_info *info)
{
    MmConnHndl    mmConn = NULL;
    struct nlattr *nla;
    int           rc = 0;

    do {
        if (info == NULL) {
            MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
            rc = -1;
            break;
        }

        /*Parse attribute */
        nla = info->attrs[MMSWITCH_ATTR_REF_MMCONN];
        if (!nla) {
            rc = -EINVAL;
            break;
        }

        nla_memcpy(&mmConn, nla, sizeof(void *));

        if (_scheduleMmConnDisc(mmConn, info) < 0) {
            MMPBX_TRC_ERROR("_scheduleMmConnDisc failed with\n");
            rc = -1;
            break;
        }
    } while (0);

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnSetTraceLevel(struct sk_buff    *skb_2,
                                            struct genl_info  *info)
{
    MmPbxError    err = MMPBX_ERROR_NOERROR;
    struct nlattr *nla;
    int           rc = 0;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse attribute */
    nla = info->attrs[MMSWITCH_ATTR_MMPBX_TRACELEVEL];
    if (!nla) {
        return -EINVAL;
    }

    err = mmConnSetTraceLevel(nla_get_u32(nla));
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnSetTraceLevel failed\n");
        rc = -1;
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnUsrSetTraceLevel(struct sk_buff   *skb_2,
                                               struct genl_info *info)
{
    MmPbxError    err = MMPBX_ERROR_NOERROR;
    struct nlattr *nla;
    int           rc = 0;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse attribute */
    nla = info->attrs[MMSWITCH_ATTR_MMPBX_TRACELEVEL];
    if (!nla) {
        return -EINVAL;
    }

    err = mmConnUsrSetTraceLevel(nla_get_u32(nla));
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnSetTraceLevel failed\n");
        rc = -1;
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnUsrConstruct(struct sk_buff   *skb_2,
                                           struct genl_info *info)
{
    int           rc        = 0;
    MmPbxError    err       = MMPBX_ERROR_NOERROR;
    MmConnUsrHndl mmConnUsr = NULL;

    if (info == NULL) {
        MMPBX_TRC_ERROR("Invalid info == NULL\n");
        return -1;
    }

    do {
        err = mmConnUsrConstruct(&mmConnUsr, info);
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnUsrConstruct failed\n");
            rc = -1;
            break;
        }
    } while (0);

    return rc;
}

/*
 *  Synchronize kernel space mmConnUsr with userspace mmConnUsr
 */
static int mmSwitchParseMmConnUsrSync(struct sk_buff    *skb_2,
                                      struct genl_info  *info)
{
    MmConnHndl      mmConn = NULL;
    struct nlattr   *nla;
    struct sk_buff  *skb      = NULL;
    int             rc        = 0;
    void            *msg_head = NULL;



    if (info == NULL) {
        MMPBX_TRC_ERROR("info == NULL\n");
        return -1;
    }

    /*Parse attribute */
    nla = info->attrs[MMSWITCH_ATTR_REF_MMCONN];
    if (!nla) {
        MMPBX_TRC_ERROR("Invalid nla == NULL\n");
        return -EINVAL;
    }

    nla_memcpy(&mmConn, nla, sizeof(void *));

    if (mmConn == NULL) {
        MMPBX_TRC_ERROR("Invalid mmConn (ref is NULL)\n");
        return -EINVAL;
    }

    MMPBX_TRC_DEBUG("called, mmConnUsr = %p\n", mmConn);

    do {
        /* send a message back*/
        /* allocate some memory, since the size is not yet known use NLMSG_GOODSIZE*/
        skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
        if (skb == NULL) {
            MMPBX_TRC_ERROR("genlmsg_new failed\n");
            rc = -ENOMEM;
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
        msg_head = genlmsg_put(skb, 0, info->snd_seq, &MmSwitchGeNlFamily, 0, MMSWITCH_CMD_MMCONNUSR_SYNC);
        if (msg_head == NULL) {
            MMPBX_TRC_ERROR("genlmsg_put failed\n");
            rc = -ENOMEM;
            break;
        }

        /* add MMCONNUSR attribute */
        rc = mmSwitchFillMmConnUsrAttr(skb, (MmConnUsrHndl) mmConn);
        if (rc != 0) {
            MMPBX_TRC_ERROR("mmSwitchFillMmConnUsrAttr failed\n");
            rc = -ENOMEM;
            break;
        }

        /* finalize the message */
        genlmsg_end(skb, msg_head);

        /* send the message back */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
        rc = genlmsg_unicast(&init_net, skb, info->snd_portid);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
        rc = genlmsg_unicast(&init_net, skb, info->snd_pid);
#else
        rc = genlmsg_unicast(skb, info->snd_pid);
#endif
        if (rc != 0) {
            MMPBX_TRC_ERROR("genlmsg_unicast failed\n");
            break;
        }

        return rc;
    } while (0);
    genlmsg_cancel(skb, msg_head);
    nlmsg_free(skb);

    return rc;
}

/*
 *  Destroy mmConnUsr generic netlink family
 */
static int mmSwitchParseMmConnUsrDestroyGeNlFam(struct sk_buff    *skb_2,
                                                struct genl_info  *info)
{
    MmPbxError    err       = MMPBX_ERROR_NOERROR;
    MmConnUsrHndl mmConnUsr = NULL;
    struct nlattr *nla;
    int           rc = 0;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse attribute */
    nla = info->attrs[MMSWITCH_ATTR_REF_MMCONN];
    if (!nla) {
        return -EINVAL;
    }

    nla_memcpy(&mmConnUsr, nla, sizeof(void *));

    if (mmConnUsr == NULL) {
        MMPBX_TRC_ERROR("Invalid mmConnUsr (ref is NULL)\n");
        return -EINVAL;
    }

    err = mmConnUsrGeNlDestroyFam((MmConnUsrHndl) mmConnUsr, info);
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnDestruct failed\n");
        rc = -1;
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnKrnlConstruct(struct sk_buff    *skb_2,
                                            struct genl_info  *info)
{
    MmPbxError        err         = MMPBX_ERROR_NOERROR;
    MmConnKrnlHndl    mmConnKrnl  = NULL;
    struct nlattr     *nla        = NULL;
    struct sk_buff    *skb        = NULL;
    int               rc          = 0;
    void              *msg_head   = NULL;
    MmConnKrnlConfig  config;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    memset(&config, 0, sizeof(MmConnKrnlConfig));

    /*Parse  attribute */
    nla = info->attrs[MMSWITCH_ATTR_MMCONNKRNL_ENDPOINT_ID];
    if (!nla) {
        return -EINVAL;
    }
    config.endpointId = nla_get_u32(nla);

    do {
        err = mmConnKrnlConstruct(&config, &mmConnKrnl);
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnUsrConstruct failed\n");
            rc = -1;
            break;
        }

        /* send a message back*/
        /* allocate some memory, since the size is not yet known use NLMSG_GOODSIZE*/
        skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
        if (skb == NULL) {
            MMPBX_TRC_ERROR("genlmsg_new failed\n");
            rc = -ENOMEM;
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
        msg_head = genlmsg_put(skb, 0, info->snd_seq, &MmSwitchGeNlFamily, 0, MMSWITCH_CMD_MMCONNKRNL_CONSTRUCT);
        if (msg_head == NULL) {
            MMPBX_TRC_ERROR("genlmsg_put failed\n");
            rc = -ENOMEM;
            break;
        }

        /* add MMCONN attribute */
        rc = mmSwitchFillMmConnAttr(skb, (MmConnHndl) mmConnKrnl);
        if (rc != 0) {
            MMPBX_TRC_ERROR("mmSwitchFillMmConnAttr failed\n");
            rc = -ENOMEM;
            break;
        }

        /* finalize the message */
        genlmsg_end(skb, msg_head);

        /* send the message back */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
        rc = genlmsg_unicast(&init_net, skb, info->snd_portid);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
        rc = genlmsg_unicast(&init_net, skb, info->snd_pid);
#else
        rc = genlmsg_unicast(skb, info->snd_pid);
#endif
        if (rc != 0) {
            MMPBX_TRC_ERROR("genlmsg_unicast failed\n");
            break;
        }
        return rc;
    } while (0);

    genlmsg_cancel(skb, msg_head);
    nlmsg_free(skb);
    if (mmConnKrnl != NULL) {
        err = mmConnDestruct((MmConnHndl *)&mmConnKrnl);
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnUsrDestruct failed\n");
        }
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnKrnlSetTraceLevel(struct sk_buff    *skb_2,
                                                struct genl_info  *info)
{
    MmPbxError    err = MMPBX_ERROR_NOERROR;
    struct nlattr *nla;
    int           rc = 0;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse attribute */
    nla = info->attrs[MMSWITCH_ATTR_MMPBX_TRACELEVEL];
    if (!nla) {
        return -EINVAL;
    }

    err = mmConnKrnlSetTraceLevel(nla_get_u32(nla));
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnSetTraceLevel failed\n");
        rc = -1;
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnMulticastConstruct(struct sk_buff   *skb_2,
                                                 struct genl_info *info)
{
    MmPbxError          err             = MMPBX_ERROR_NOERROR;
    MmConnMulticastHndl mmConnMulticast = NULL;
    struct sk_buff      *skb            = NULL;
    void                *msg_head       = NULL;
    int                 rc = 0;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    do {
        err = mmConnMulticastConstruct(&mmConnMulticast);
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnMulticastConstruct failed\n");
            rc = -1;
            break;
        }

        /* send a message back*/
        /* allocate some memory, since the size is not yet known use NLMSG_GOODSIZE*/
        skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
        if (skb == NULL) {
            MMPBX_TRC_ERROR("genlmsg_new failed\n");
            rc = -ENOMEM;
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
        msg_head = genlmsg_put(skb, 0, info->snd_seq, &MmSwitchGeNlFamily, 0, MMSWITCH_CMD_MMCONNMULTICAST_CONSTRUCT);
        if (msg_head == NULL) {
            MMPBX_TRC_ERROR("genlmsg_put failed\n");
            rc = -ENOMEM;
            break;
        }

        /* add MMCONN attribute */
        rc = mmSwitchFillMmConnAttr(skb, (MmConnHndl) mmConnMulticast);
        if (rc != 0) {
            MMPBX_TRC_ERROR("mmSwitchFillMmConnAttr failed\n");
            rc = -ENOMEM;
            break;
        }

        /* finalize the message */
        genlmsg_end(skb, msg_head);

        /* send the message back */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
        rc = genlmsg_unicast(&init_net, skb, info->snd_portid);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
        rc = genlmsg_unicast(&init_net, skb, info->snd_pid);
#else
        rc = genlmsg_unicast(skb, info->snd_pid);
#endif
        if (rc != 0) {
            MMPBX_TRC_ERROR("genlmsg_unicast failed\n");
            break;
        }
        return rc;
    } while (0);

    genlmsg_cancel(skb, msg_head);
    nlmsg_free(skb);
    if (mmConnMulticast != NULL) {
        err = mmConnDestruct((MmConnHndl *)&mmConnMulticast);
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnMulticastDestruct failed\n");
        }
    }
    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnMulticastSetTraceLevel(struct sk_buff   *skb_2,
                                                     struct genl_info *info)
{
    int           rc  = 0;
    MmPbxError    err = MMPBX_ERROR_NOERROR;
    struct nlattr *nla;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse attribute */
    nla = info->attrs[MMSWITCH_ATTR_MMPBX_TRACELEVEL];
    if (!nla) {
        return -EINVAL;
    }

    err = mmConnMulticastSetTraceLevel(nla_get_u32(nla));
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnMulticastSetTraceLevel failed\n");
        rc = -1;
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnMulticastAddSink(struct sk_buff   *skb_2,
                                               struct genl_info *info)
{
    MmPbxError    ret     = MMPBX_ERROR_NOERROR;
    MmConnHndl    source  = NULL;
    MmConnHndl    sink    = NULL;
    struct nlattr *parentAttr;
    struct nlattr *tb[MMSWITCH_ATTR_MAX + 1];
    int           rc = 0;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse MMCONN attribute */
    parentAttr = info->attrs[MMSWITCH_ATTR_MMCONN];
    if (!parentAttr) {
        return -EINVAL;
    }
    rc = nla_parse_nested(tb, MMSWITCH_ATTR_MMCONN_MAX, parentAttr, mmSwitchGeNlAttrMmConnPolicy);
    if (rc < 0) {
        MMPBX_TRC_ERROR("nla_parse_nested failed\n");
        return rc;
    }

    if (!tb[MMSWITCH_ATTR_MMCONN_REF]) {
        return -EINVAL;
    }

    nla_memcpy(&source, tb[MMSWITCH_ATTR_MMCONN_REF], sizeof(void *));

    /*Parse REF_MMCONN attribute */
    parentAttr = info->attrs[MMSWITCH_ATTR_REF_MMCONN];
    if (!parentAttr) {
        return -EINVAL;
    }
    nla_memcpy(&sink, parentAttr, sizeof(void *));

    ret = mmConnMulticastAddSink((MmConnMulticastHndl)source, (MmConnMulticastHndl) sink);
    if (ret != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnMulticastAddSink failed with error: %s\n", mmPbxGetErrorString(ret));
        rc = -1;
    }


    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnMulticastRemoveSink(struct sk_buff    *skb_2,
                                                  struct genl_info  *info)
{
    MmPbxError    ret     = MMPBX_ERROR_NOERROR;
    MmConnHndl    source  = NULL;
    MmConnHndl    sink    = NULL;
    struct nlattr *parentAttr;
    struct nlattr *tb[MMSWITCH_ATTR_MAX + 1];
    int           rc = 0;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse MMCONN attribute */
    parentAttr = info->attrs[MMSWITCH_ATTR_MMCONN];
    if (!parentAttr) {
        return -EINVAL;
    }
    rc = nla_parse_nested(tb, MMSWITCH_ATTR_MMCONN_MAX, parentAttr, mmSwitchGeNlAttrMmConnPolicy);
    if (rc < 0) {
        MMPBX_TRC_ERROR("nla_parse_nested failed\n");
        return rc;
    }

    if (!tb[MMSWITCH_ATTR_MMCONN_REF]) {
        return -EINVAL;
    }

    nla_memcpy(&source, tb[MMSWITCH_ATTR_MMCONN_REF], sizeof(void *));

    /*Parse REF_MMCONN attribute */
    parentAttr = info->attrs[MMSWITCH_ATTR_REF_MMCONN];
    if (!parentAttr) {
        return -EINVAL;
    }
    nla_memcpy(&sink, parentAttr, sizeof(void *));

    ret = mmConnMulticastRemoveSink((MmConnMulticastHndl)source, (MmConnMulticastHndl) sink);
    if (ret != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnMulticastRemoveSink failed with error: %s\n", mmPbxGetErrorString(ret));
        rc = -1;
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnToneConstruct(struct sk_buff    *skb_2,
                                            struct genl_info  *info)
{
    MmPbxError        err         = MMPBX_ERROR_NOERROR;
    MmConnToneHndl    mmConnTone  = NULL;
    struct nlattr     *nla        = NULL;
    struct sk_buff    *skb        = NULL;
    int               rc          = 0;
    void              *msg_head   = NULL;
    MmConnToneConfig  config;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    memset(&config, 0, sizeof(MmConnToneConfig));

    /*Parse  attributes */
    nla = info->attrs[MMSWITCH_ATTR_MMCONNTONE_ENDPOINT_ID];
    if (!nla) {
        return -EINVAL;
    }
    config.endpointId = nla_get_u32(nla);

    nla = info->attrs[MMSWITCH_ATTR_MMCONNTONE_TYPE];
    if (!nla) {
        return -EINVAL;
    }
    config.type = nla_get_u32(nla);

    nla = info->attrs[MMSWITCH_ATTR_MMCONNTONE_PATTERNTABLE_SIZE];
    if (!nla) {
        return -EINVAL;
    }
    config.toneTable.size = nla_get_u32(nla);

    nla = info->attrs[MMSWITCH_ATTR_ENCODINGNAME];
    if (!nla) {
        return -EINVAL;
    }
    strncpy(config.encodingName, nla_data(nla), ENCODING_NAME_LENGTH);

    nla = info->attrs[MMSWITCH_ATTR_PACKETPERIOD];
    if (!nla) {
        return -EINVAL;
    }
    config.packetPeriod = nla_get_u32(nla);

    do {
        err = mmConnToneConstruct(&config, &mmConnTone);
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnToneConstruct failed\n");
            rc = -1;
            break;
        }

        /* send a message back*/
        /* allocate some memory, since the size is not yet known use NLMSG_GOODSIZE*/
        skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
        if (skb == NULL) {
            MMPBX_TRC_ERROR("genlmsg_new failed\n");
            rc = -ENOMEM;
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
        msg_head = genlmsg_put(skb, 0, info->snd_seq, &MmSwitchGeNlFamily, 0, MMSWITCH_CMD_MMCONNTONE_CONSTRUCT);
        if (msg_head == NULL) {
            MMPBX_TRC_ERROR("genlmsg_put failed\n");
            rc = -ENOMEM;
            break;
        }

        /* add MMCONN attribute */
        rc = mmSwitchFillMmConnAttr(skb, (MmConnHndl) mmConnTone);
        if (rc != 0) {
            MMPBX_TRC_ERROR("mmSwitchFillMmConnAttr failed\n");
            rc = -ENOMEM;
            break;
        }

        /* finalize the message */
        genlmsg_end(skb, msg_head);

        /* send the message back */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
        rc = genlmsg_unicast(&init_net, skb, info->snd_portid);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
        rc = genlmsg_unicast(&init_net, skb, info->snd_pid);
#else
        rc = genlmsg_unicast(skb, info->snd_pid);
#endif
        if (rc != 0) {
            MMPBX_TRC_ERROR("genlmsg_unicast failed\n");
            break;
        }
        return rc;
    } while (0);

    genlmsg_cancel(skb, msg_head);
    nlmsg_free(skb);
    if (mmConnTone != NULL) {
        err = mmConnDestruct((MmConnHndl *)&mmConnTone);
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnDestruct failed\n");
        }
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnToneSetTraceLevel(struct sk_buff    *skb_2,
                                                struct genl_info  *info)
{
    MmPbxError    err = MMPBX_ERROR_NOERROR;
    struct nlattr *nla;
    int           rc = 0;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse attribute */
    nla = info->attrs[MMSWITCH_ATTR_MMPBX_TRACELEVEL];
    if (!nla) {
        return -EINVAL;
    }

    err = mmConnToneSetTraceLevel(nla_get_u32(nla));
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnToneSetTraceLevel failed\n");
        rc = -1;
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnToneSendPattern(struct sk_buff    *skb_2,
                                              struct genl_info  *info)
{
    MmPbxError        err     = MMPBX_ERROR_NOERROR;
    MmConnHndl        mmConn  = NULL;
    MmConnTonePattern pattern;
    struct nlattr     *parentAttr;
    struct nlattr     *tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_MAX + 1];
    int               rc = 0;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse REF_MMCONN attribute */
    parentAttr = info->attrs[MMSWITCH_ATTR_REF_MMCONN];
    if (!parentAttr) {
        MMPBX_TRC_ERROR("missing MMSWITCH_ATTR_REF_MMCONN\n");
        return -EINVAL;
    }

    nla_memcpy(&mmConn, parentAttr, sizeof(void *));

    /*Parse MMCONNTONE_PATTERN attribute */
    parentAttr = info->attrs[MMSWITCH_ATTR_MMCONNTONE_PATTERN];
    if (!parentAttr) {
        return -EINVAL;
    }
    rc = nla_parse_nested(tb, MMSWITCH_ATTR_MMCONNTONE_PATTERN_MAX, parentAttr, mmSwitchGeNlAttrMmConnTonePatternPolicy);
    if (rc < 0) {
        MMPBX_TRC_ERROR("nla_parse_nested failed\n");
        return rc;
    }

    if ((!tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_ID]) ||
        (!tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_ON]) ||
        (!tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_FREQ1]) ||
        (!tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_FREQ2]) ||
        (!tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_FREQ3]) ||
        (!tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_FREQ4]) ||
        (!tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_POWER1]) ||
        (!tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_POWER2]) ||
        (!tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_POWER3]) ||
        (!tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_POWER4]) ||
        (!tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_DURATION]) ||
        (!tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_NEXTID]) ||
        (!tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_MAXLOOPS]) ||
        (!tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_NEXTIDAFTERLOOPS])
        ) {
        MMPBX_TRC_ERROR("missing pattern parameters\n");
        return -EINVAL;
    }

    pattern.id                = nla_get_u32(tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_ID]);
    pattern.on                = nla_get_u32(tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_ON]);
    pattern.freq1             = nla_get_u32(tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_FREQ1]);
    pattern.freq2             = nla_get_u32(tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_FREQ2]);
    pattern.freq3             = nla_get_u32(tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_FREQ3]);
    pattern.freq4             = nla_get_u32(tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_FREQ4]);
    pattern.power1            = nla_get_u32(tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_POWER1]);
    pattern.power2            = nla_get_u32(tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_POWER2]);
    pattern.power3            = nla_get_u32(tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_POWER3]);
    pattern.power4            = nla_get_u32(tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_POWER4]);
    pattern.duration          = nla_get_u32(tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_DURATION]);
    pattern.nextId            = nla_get_u32(tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_NEXTID]);
    pattern.maxLoops          = nla_get_u32(tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_MAXLOOPS]);
    pattern.nextIdAfterLoops  = nla_get_u32(tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_NEXTIDAFTERLOOPS]);


    err = mmConnToneSavePattern((MmConnToneHndl) mmConn, &pattern);
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnToneSavePattern failed with error: %s\n", mmPbxGetErrorString(err));
        rc = -1;
    }



    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnRelayConstruct(struct sk_buff   *skb_2,
                                             struct genl_info *info)
{
    MmPbxError        err         = MMPBX_ERROR_NOERROR;
    MmConnRelayHndl   mmConnRelay = NULL;
    struct nlattr     *nla        = NULL;
    struct sk_buff    *skb        = NULL;
    int               rc          = 0;
    void              *msg_head   = NULL;
    MmConnRelayConfig config;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    memset(&config, 0, sizeof(MmConnRelayConfig));

    /*Parse  attributes */
    nla = info->attrs[MMSWITCH_ATTR_LOCAL_SOCKFD];
    if (!nla) {
        return -EINVAL;
    }
    config.localSockFd = nla_get_u32(nla);

    nla = info->attrs[MMSWITCH_ATTR_REMOTE_ADDR];
    if (!nla) {
        return -EINVAL;
    }
    nla_memcpy(&config.remoteAddr, nla, sizeof(struct sockaddr_storage));

    /*Parse  attributes */
    nla = info->attrs[MMSWITCH_ATTR_PACKET_TYPE];
    if (!nla) {
        return -EINVAL;
    }
    config.header.type = nla_get_u32(nla);

    nla = info->attrs[MMSWITCH_ATTR_TIMEOUT];
    if (!nla) {
        return -EINVAL;
    }
    config.mediaTimeout = nla_get_u32(nla);


    do {
        err = mmConnRelayConstruct(&config, &mmConnRelay);
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnRelayConstruct failed\n");
            rc = -1;
            break;
        }

        /* send a message back*/
        /* allocate some memory, since the size is not yet known use NLMSG_GOODSIZE*/
        skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
        if (skb == NULL) {
            MMPBX_TRC_ERROR("genlmsg_new failed\n");
            rc = -ENOMEM;
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
        msg_head = genlmsg_put(skb, 0, info->snd_seq, &MmSwitchGeNlFamily, 0, MMSWITCH_CMD_MMCONNRELAY_CONSTRUCT);
        if (msg_head == NULL) {
            MMPBX_TRC_ERROR("genlmsg_put failed\n");
            rc = -ENOMEM;
            break;
        }

        /* add MMCONN attribute */
        rc = mmSwitchFillMmConnAttr(skb, (MmConnHndl) mmConnRelay);
        if (rc != 0) {
            MMPBX_TRC_ERROR("mmSwitchFillMmConnAttr failed\n");
            rc = -ENOMEM;
            break;
        }

        /* finalize the message */
        genlmsg_end(skb, msg_head);

        /* send the message back */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
        rc = genlmsg_unicast(&init_net, skb, info->snd_portid);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
        rc = genlmsg_unicast(&init_net, skb, info->snd_pid);
#else
        rc = genlmsg_unicast(skb, info->snd_pid);
#endif
        if (rc != 0) {
            MMPBX_TRC_ERROR("genlmsg_unicast failed\n");
            break;
        }
        return rc;
    } while (0);

    genlmsg_cancel(skb, msg_head);
    nlmsg_free(skb);
    if (mmConnRelay != NULL) {
        err = mmConnDestruct((MmConnHndl *)&mmConnRelay);
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnRelayDestruct failed\n");
        }
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnRelaySetTraceLevel(struct sk_buff   *skb_2,
                                                 struct genl_info *info)
{
    MmPbxError    err = MMPBX_ERROR_NOERROR;
    struct nlattr *nla;
    int           rc = 0;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse attribute */
    nla = info->attrs[MMSWITCH_ATTR_MMPBX_TRACELEVEL];
    if (!nla) {
        return -EINVAL;
    }

    err = mmConnRelaySetTraceLevel(nla_get_u32(nla));
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnSetTraceLevel failed\n");
        rc = -1;
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnRtcpConstruct(struct sk_buff    *skb_2,
                                            struct genl_info  *info)
{
    MmPbxError        err         = MMPBX_ERROR_NOERROR;
    MmConnRtcpHndl    mmConnRtcp  = NULL;
    struct nlattr     *nla        = NULL;
    struct sk_buff    *skb        = NULL;
    int               rc          = 0;
    void              *msg_head   = NULL;
    MmConnRtcpConfig  config;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    memset(&config, 0, sizeof(MmConnRtcpConfig));

    /*Parse  attributes */
    nla = info->attrs[MMSWITCH_ATTR_LOCAL_SOCKFD];
    if (!nla) {
        return -EINVAL;
    }
    config.localMediaSockFd = nla_get_u32(nla);

    nla = info->attrs[MMSWITCH_ATTR_REMOTE_ADDR];
    if (!nla) {
        return -EINVAL;
    }
    nla_memcpy(&config.remoteMediaAddr, nla, sizeof(struct sockaddr_storage));

    nla = info->attrs[MMSWITCH_ATTR_LOCAL_RTCP_SOCKFD];
    if (!nla) {
        return -EINVAL;
    }
    config.localRtcpSockFd = nla_get_u32(nla);

    nla = info->attrs[MMSWITCH_ATTR_REMOTE_RTCP_ADDR];
    if (!nla) {
        return -EINVAL;
    }
    nla_memcpy(&config.remoteRtcpAddr, nla, sizeof(struct sockaddr_storage));

    /*Parse  attributes */
    nla = info->attrs[MMSWITCH_ATTR_PACKET_TYPE];
    if (!nla) {
        return -EINVAL;
    }
    config.header.type = nla_get_u32(nla);

    nla = info->attrs[MMSWITCH_ATTR_TIMEOUT];
    if (!nla) {
        return -EINVAL;
    }
    config.mediaTimeout = nla_get_u32(nla);

    nla = info->attrs[MMSWITCH_ATTR_MUTE_TIME];
    if (!nla) {
        return -EINVAL;
    }
    config.mediaMuteTime = nla_get_u32(nla);

    nla = info->attrs[MMSWITCH_ATTR_RTCP_BANDWIDTH];
    if (!nla) {
        return -EINVAL;
    }
    config.rtcpBandwidth = nla_get_u32(nla);

    nla = info->attrs[MMSWITCH_ATTR_GEN_RTCP];
    if (!nla) {
        return -EINVAL;
    }
    config.genRtcp = nla_get_u32(nla);

    do {
        err = mmConnRtcpConstruct(&config, &mmConnRtcp);
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnRtcpConstruct failed\n");
            rc = -1;
            break;
        }

        /* send a message back*/
        /* allocate some memory, since the size is not yet known use NLMSG_GOODSIZE*/
        skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
        if (skb == NULL) {
            MMPBX_TRC_ERROR("genlmsg_new failed\n");
            rc = -ENOMEM;
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
        msg_head = genlmsg_put(skb, 0, info->snd_seq, &MmSwitchGeNlFamily, 0, MMSWITCH_CMD_MMCONNRTCP_CONSTRUCT);
        if (msg_head == NULL) {
            MMPBX_TRC_ERROR("genlmsg_put failed\n");
            rc = -ENOMEM;
            break;
        }

        /* add MMCONN attribute */
        rc = mmSwitchFillMmConnAttr(skb, (MmConnHndl) mmConnRtcp);
        if (rc != 0) {
            MMPBX_TRC_ERROR("mmSwitchFillMmConnAttr failed\n");
            rc = -ENOMEM;
            break;
        }

        /* finalize the message */
        genlmsg_end(skb, msg_head);

        /* send the message back */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
        rc = genlmsg_unicast(&init_net, skb, info->snd_portid);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
        rc = genlmsg_unicast(&init_net, skb, info->snd_pid);
#else
        rc = genlmsg_unicast(skb, info->snd_pid);
#endif
        if (rc != 0) {
            MMPBX_TRC_ERROR("genlmsg_unicast failed\n");
            break;
        }
        return rc;
    } while (0);

    genlmsg_cancel(skb, msg_head);
    nlmsg_free(skb);
    if (mmConnRtcp != NULL) {
        err = mmConnDestruct((MmConnHndl *)&mmConnRtcp);
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnRtcpDestruct failed\n");
        }
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnRtcpSetTraceLevel(struct sk_buff    *skb_2,
                                                struct genl_info  *info)
{
    MmPbxError    err = MMPBX_ERROR_NOERROR;
    struct nlattr *nla;
    int           rc = 0;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse attribute */
    nla = info->attrs[MMSWITCH_ATTR_MMPBX_TRACELEVEL];
    if (!nla) {
        return -EINVAL;
    }

    err = mmConnRtcpSetTraceLevel(nla_get_u32(nla));
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnSetTraceLevel failed\n");
        rc = -1;
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnRtcpModPacketType(struct sk_buff    *skb_2,
                                                struct genl_info  *info)
{
    MmPbxError    err     = MMPBX_ERROR_NOERROR;
    MmConnHndl    mmConn  = NULL;
    struct nlattr *nla;
    int           rc    = 0;
    int           type  = 0;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse attribute */
    nla = info->attrs[MMSWITCH_ATTR_REF_MMCONN];
    if (!nla) {
        return -EINVAL;
    }
    nla_memcpy(&mmConn, nla, sizeof(void *));

    nla = info->attrs[MMSWITCH_ATTR_PACKET_TYPE];
    if (!nla) {
        return -EINVAL;
    }

    type = nla_get_u32(nla);


    err = mmConnRtcpModMediaPacketType((MmConnRtcpHndl) mmConn, type);
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnRtcpModPacketType failed\n");
        rc = -1;
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnRtcpModGenRtcp(struct sk_buff   *skb_2,
                                             struct genl_info *info)
{
    MmPbxError    err     = MMPBX_ERROR_NOERROR;
    MmConnHndl    mmConn  = NULL;
    struct nlattr *nla;
    int           rc      = 0;
    int           rtcpGen = 0;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse attribute */
    nla = info->attrs[MMSWITCH_ATTR_REF_MMCONN];
    if (!nla) {
        return -EINVAL;
    }
    nla_memcpy(&mmConn, nla, sizeof(void *));

    nla = info->attrs[MMSWITCH_ATTR_GEN_RTCP];
    if (!nla) {
        return -EINVAL;
    }

    rtcpGen = nla_get_u32(nla);


    err = mmConnRtcpModGenRtcp((MmConnRtcpHndl) mmConn, rtcpGen);
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnRtcpModGenRtcp failed\n");
        rc = -1;
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnRtcpModRemoteMediaAddr(struct sk_buff   *skb_2,
                                                     struct genl_info *info)
{
    MmPbxError              err     = MMPBX_ERROR_NOERROR;
    MmConnHndl              mmConn  = NULL;
    struct nlattr           *nla;
    int                     rc = 0;
    struct sockaddr_storage remoteAddr;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse attributes */
    nla = info->attrs[MMSWITCH_ATTR_REF_MMCONN];
    if (!nla) {
        return -EINVAL;
    }
    nla_memcpy(&mmConn, nla, sizeof(void *));

    nla = info->attrs[MMSWITCH_ATTR_REMOTE_ADDR];
    if (!nla) {
        return -EINVAL;
    }
    nla_memcpy(&remoteAddr, nla, sizeof(struct sockaddr_storage));


    err = mmConnRtcpModRemoteMediaAddr((MmConnRtcpHndl) mmConn, &remoteAddr);
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnRtcpModRemoteMediaAddr failed\n");
        rc = -1;
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnRtcpModRemoteRtcpAddr(struct sk_buff    *skb_2,
                                                    struct genl_info  *info)
{
    MmPbxError              err     = MMPBX_ERROR_NOERROR;
    MmConnHndl              mmConn  = NULL;
    struct nlattr           *nla;
    int                     rc = 0;
    struct sockaddr_storage remoteRtcpAddr;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse attributes */
    nla = info->attrs[MMSWITCH_ATTR_REF_MMCONN];
    if (!nla) {
        return -EINVAL;
    }
    nla_memcpy(&mmConn, nla, sizeof(void *));

    nla = info->attrs[MMSWITCH_ATTR_REMOTE_RTCP_ADDR];
    if (!nla) {
        return -EINVAL;
    }
    nla_memcpy(&remoteRtcpAddr, nla, sizeof(struct sockaddr_storage));

    err = mmConnRtcpModRemoteRtcpAddr((MmConnRtcpHndl) mmConn, &remoteRtcpAddr);
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnRtcpModRemoteRtcpAddr failed\n");
        rc = -1;
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnRtcpModRtcpBandwidth(struct sk_buff   *skb_2,
                                                   struct genl_info *info)
{
    MmPbxError    err     = MMPBX_ERROR_NOERROR;
    MmConnHndl    mmConn  = NULL;
    struct nlattr *nla;
    int           rc            = 0;
    int           rtcpBandwidth = 0;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse attribute */
    nla = info->attrs[MMSWITCH_ATTR_REF_MMCONN];
    if (!nla) {
        return -EINVAL;
    }
    nla_memcpy(&mmConn, nla, sizeof(void *));

    nla = info->attrs[MMSWITCH_ATTR_RTCP_BANDWIDTH];
    if (!nla) {
        return -EINVAL;
    }

    rtcpBandwidth = nla_get_u32(nla);


    err = mmConnRtcpModRtcpBandwidth((MmConnRtcpHndl) mmConn, rtcpBandwidth);
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnRtcpModRtcpBandwidth failed\n");
        rc = -1;
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnRtcpGetStats(struct sk_buff   *skb_2,
                                           struct genl_info *info)
{
    MmConnRtcpHndl  mmConnRtcp = NULL;
    struct nlattr   *nla;
    int             rc        = 0;
    struct sk_buff  *skb      = NULL;
    void            *msg_head = NULL;

#ifdef MMPBX_DSP_SUPPORT_RTCPXR
    int i = 0;
#endif

    MMPBX_TRC_DEBUG("Called\n");

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse attribute */
    nla = info->attrs[MMSWITCH_ATTR_REF_MMCONN];
    if (!nla) {
        return -EINVAL;
    }
    nla_memcpy(&mmConnRtcp, nla, sizeof(void *));

    if (mmConnRtcp == NULL) {
        return -EINVAL;
    }

    do {
        /* send a message back*/
        /* allocate some memory, since the size is not yet known use NLMSG_GOODSIZE*/
        skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
        if (skb == NULL) {
            MMPBX_TRC_ERROR("genlmsg_new failed\n");
            rc = -ENOMEM;
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
        msg_head = genlmsg_put(skb, 0, info->snd_seq, &MmSwitchGeNlFamily, 0, MMSWITCH_CMD_MMCONNRTCP_GET_STATS);
        if (msg_head == NULL) {
            MMPBX_TRC_ERROR("genlmsg_put failed\n");
            rc = -ENOMEM;
            break;
        }
#ifdef MMPBX_DSP_SUPPORT_RTCPXR
        /* Trigger to collect statistics */
        mmConnRtcpUpdateStats(mmConnRtcp);
#endif

        /* add MMCONNRTCP attribute */
        rc = mmSwitchFillMmConnRtcpAttr(skb, mmConnRtcp);
        if (rc != 0) {
            MMPBX_TRC_ERROR("mmSwitchFillMmConnRtcpAttr failed\n");
            rc = -ENOMEM;
            break;
        }

        /* finalize the message */
        genlmsg_end(skb, msg_head);

        /* send the message back */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
        rc = genlmsg_unicast(&init_net, skb, info->snd_portid);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
        rc = genlmsg_unicast(&init_net, skb, info->snd_pid);
#else
        rc = genlmsg_unicast(skb, info->snd_pid);
#endif

#ifdef MMPBX_DSP_SUPPORT_RTCPXR
        for ( ; i < MMCONN_RTCP_MAX_SAVED_RTCPXR_FRAME; i++) {
            if (mmConnRtcp->stats.rxRtcpFrameBuffer[i] != NULL) {
                kfree(mmConnRtcp->stats.rxRtcpFrameBuffer[i]);
                mmConnRtcp->stats.rxRtcpFrameBuffer[i] = NULL;
            }
            if (mmConnRtcp->stats.txRtcpFrameBuffer[i] != NULL) {
                kfree(mmConnRtcp->stats.txRtcpFrameBuffer[i]);
                mmConnRtcp->stats.txRtcpFrameBuffer[i] = NULL;
            }
        }
#endif

        if (rc != 0) {
            MMPBX_TRC_ERROR("genlmsg_unicast failed\n");
            break;
        }

        return rc;
    } while (0);
    genlmsg_cancel(skb, msg_head);
    nlmsg_free(skb);

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnRtcpResetStats(struct sk_buff   *skb_2,
                                             struct genl_info *info)
{
    int rc = 0;

#ifdef MMPBX_DSP_SUPPORT_RTCPXR
    MmPbxError      err         = MMPBX_ERROR_NOERROR;
    MmConnRtcpHndl  mmConnRtcp  = NULL;
    struct nlattr   *nla;


    MMPBX_TRC_DEBUG("Called\n");

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse attribute */
    nla = info->attrs[MMSWITCH_ATTR_REF_MMCONN];
    if (!nla) {
        return -EINVAL;
    }
    nla_memcpy(&mmConnRtcp, nla, sizeof(void *));

    if (mmConnRtcp == NULL) {
        return -EINVAL;
    }

    MMPBX_TRC_DEBUG("mmConn: %p Stats Reset \n\r", mmConnRtcp);

    /* Update the statistics */
    err = mmConnRtcpResetStats((MmConnRtcpHndl)mmConnRtcp);
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnRtcpModRtcpBandwidth failed\n");
        rc = -1;
    }
#endif

    return rc;
}

/**
 *
 */
static int mmSwitchFillMmConnRtcpAttr(struct sk_buff  *skb,
                                      MmConnRtcpHndl  mmConnRtcp)
{
    struct nlattr *nest;


    do {
        nest = nla_nest_start(skb, MMSWITCH_ATTR_MMCONNRTCP);
        if (!nest) {
            break;
        }

        /*Update Calllength */
        mmConnRtcp->stats.callLength = jiffies_to_msecs((get_jiffies_64() - mmConnRtcp->callStartJiffies));

        /*Add stats to message */
        if (nla_put_u64(skb, MMSWITCH_ATTR_MMCONNRTCP_CALLLENGTH, mmConnRtcp->stats.callLength) < 0) {
            break;
        }
        if (nla_put_u64(skb, MMSWITCH_ATTR_MMCONNRTCP_TXRTPPACKETS, mmConnRtcp->stats.txRtpPackets) < 0) {
            break;
        }
        if (nla_put_u64(skb, MMSWITCH_ATTR_MMCONNRTCP_TXRTPBYTES, mmConnRtcp->stats.txRtpBytes) < 0) {
            break;
        }
        if (nla_put_u64(skb, MMSWITCH_ATTR_MMCONNRTCP_RXRTPPACKETS, mmConnRtcp->stats.rxRtpPackets) < 0) {
            break;
        }
        if (nla_put_u64(skb, MMSWITCH_ATTR_MMCONNRTCP_RXRTPBYTES, mmConnRtcp->stats.rxRtpBytes) < 0) {
            break;
        }
        if (nla_put_u64(skb, MMSWITCH_ATTR_MMCONNRTCP_RXTOTALPACKETSLOST, mmConnRtcp->stats.rxTotalPacketsLost) < 0) {
            break;
        }
        if (nla_put_u64(skb, MMSWITCH_ATTR_MMCONNRTCP_RXTOTALPACKETSDISCARDED, mmConnRtcp->stats.rxTotalPacketsDiscarded) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_RXPACKETSLOSTRATE, mmConnRtcp->stats.rxPacketsLostRate) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_RXPACKETSDISCARDEDRATE, mmConnRtcp->stats.rxPacketsDiscardedRate) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_SIGNALLEVEL, mmConnRtcp->stats.signalLevel) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_NOISELEVEL, mmConnRtcp->stats.noiseLevel) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_RERL, mmConnRtcp->stats.RERL) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_RFACTOR, mmConnRtcp->stats.RFactor) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_EXTERNALRFACTOR, mmConnRtcp->stats.externalRFactor) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_MOSLQ, mmConnRtcp->stats.mosLQ) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_MOSCQ, mmConnRtcp->stats.mosCQ) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_MEANE2EDELAY, mmConnRtcp->stats.meanE2eDelay) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_WORSTE2EDELAY, mmConnRtcp->stats.worstE2eDelay) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_CURRENTE2EDELAY, mmConnRtcp->stats.currentE2eDelay) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_RXJITTER, mmConnRtcp->stats.rxJitter) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_RXMINJITTER, mmConnRtcp->stats.rxMinJitter) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_RXMAXJITTER, mmConnRtcp->stats.rxMaxJitter) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_RXDEVJITTER, mmConnRtcp->stats.rxDevJitter) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_RXMEANJITTER, mmConnRtcp->stats.meanRxJitter) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_RXWORSTJITTER, mmConnRtcp->stats.worstRxJitter) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_JITTER_BUFFER_OVERRUNS, mmConnRtcp->stats.jitterBufferOverruns) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_JITTER_BUFFER_UNDERRUNS, mmConnRtcp->stats.jitterBufferUnderruns) < 0) {
            break;
        }
        if (nla_put_u64(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_TXRTPPACKETS, mmConnRtcp->stats.remoteTxRtpPackets) < 0) {
            break;
        }
        if (nla_put_u64(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_TXRTPBYTES, mmConnRtcp->stats.remoteTxRtpBytes) < 0) {
            break;
        }
        if (nla_put_u64(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_RXTOTALPACKETSLOST, mmConnRtcp->stats.remoteRxTotalPacketsLost) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_RXPACKETSLOSTRATE, mmConnRtcp->stats.remoteRxPacketsLostRate) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_RXPACKETSDISCARDEDRATE, mmConnRtcp->stats.remoteRxPacketsDiscardedRate) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_SIGNALLEVEL, mmConnRtcp->stats.remoteSignalLevel) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_NOISELEVEL, mmConnRtcp->stats.remoteNoiseLevel) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_RERL, mmConnRtcp->stats.remoteRERL) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_RFACTOR, mmConnRtcp->stats.remoteRFactor) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_EXTERNALRFACTOR, mmConnRtcp->stats.remoteExternalRFactor) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_MOSLQ, mmConnRtcp->stats.remoteMosLQ) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_MOSCQ, mmConnRtcp->stats.remoteMosCQ) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_MEANE2EDELAY, mmConnRtcp->stats.remoteMeanE2eDelay) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_WORSTE2EDELAY, mmConnRtcp->stats.remoteWorstE2eDelay) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_CURRENTE2EDELAY, mmConnRtcp->stats.remoteCurrentE2eDelay) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_RXJITTER, mmConnRtcp->stats.remoteRxJitter) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_RXMINJITTER, mmConnRtcp->stats.minRemoteRxJitter) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_RXMAXJITTER, mmConnRtcp->stats.maxRemoteRxJitter) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_RXDEVJITTER, mmConnRtcp->stats.devRemoteRxJitter) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_RXMEANJITTER, mmConnRtcp->stats.meanRemoteRxJitter) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_RXWORSTJITTER, mmConnRtcp->stats.remoteRxWorstJitter) < 0) {
            break;
        }

        /*AT&T extension*/
        if (nla_put_u64(skb, MMSWITCH_ATTR_MMCONNRTCP_TXRTCPPACKETS, mmConnRtcp->stats.txRtcpPackets) < 0) {
            break;
        }
        if (nla_put_u64(skb, MMSWITCH_ATTR_MMCONNRTCP_RXRTCPPACKETS, mmConnRtcp->stats.rxRtcpPackets) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_SUM_FRACTIONLOSS, mmConnRtcp->stats.localSumFractionLoss) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_SUM_SQR_FRACTIONLOSS, mmConnRtcp->stats.localSumSqrFractionLoss) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_SUM_FRACTIONLOSS, mmConnRtcp->stats.remoteSumFractionLoss) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_SUM_SQR_FRACTIONLOSS, mmConnRtcp->stats.remoteSumSqrFractionLoss) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_SUM_JITTER, mmConnRtcp->stats.localSumJitter) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_SUM_SQR_JITTER, mmConnRtcp->stats.localSumSqrJitter) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_SUM_JITTER, mmConnRtcp->stats.remoteSumJitter) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_SUM_SQR_JITTER, mmConnRtcp->stats.remoteSumSqrJitter) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_SUM_E2EDELAY, mmConnRtcp->stats.sumRoundTripDelay) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_SUM_SQR_E2EDELAY, mmConnRtcp->stats.sumSqrRoundTripDelay) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_MAX_ONEWAYDELAY, mmConnRtcp->stats.maxOneWayDelay) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_SUM_ONEWAYDELAY, mmConnRtcp->stats.sumOneWayDelay) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_SUM_SQR_ONEWAYDELAY, mmConnRtcp->stats.sumSqrOneWayDelay) < 0) {
            break;
        }
#if MMCONN_RTCP_MAX_SAVED_RTCPXR_FRAME > 0
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_FIRST_RX_RTCP_LENGTH, mmConnRtcp->stats.rxRtcpFrameLength[0]) < 0) {
            break;
        }

        if (mmConnRtcp->stats.rxRtcpFrameLength[0] > 0) {
            nla_put(skb, MMSWITCH_ATTR_MMCONNRTCP_FIRST_RX_RTCP_BUFFER, mmConnRtcp->stats.rxRtcpFrameLength[0], mmConnRtcp->stats.rxRtcpFrameBuffer[0]);
        }

        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_FIRST_TX_RTCP_LENGTH, mmConnRtcp->stats.txRtcpFrameLength[0]) < 0) {
            break;
        }

        if (mmConnRtcp->stats.txRtcpFrameLength[0] > 0) {
            nla_put(skb, MMSWITCH_ATTR_MMCONNRTCP_FIRST_TX_RTCP_BUFFER, mmConnRtcp->stats.txRtcpFrameLength[0], mmConnRtcp->stats.txRtcpFrameBuffer[0]);
        }

#if MMCONN_RTCP_MAX_SAVED_RTCPXR_FRAME > 1
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_SECOND_RX_RTCP_LENGTH, mmConnRtcp->stats.rxRtcpFrameLength[1]) < 0) {
            break;
        }
        if (mmConnRtcp->stats.rxRtcpFrameLength[1] > 0) {
            nla_put(skb, MMSWITCH_ATTR_MMCONNRTCP_SECOND_RX_RTCP_BUFFER, mmConnRtcp->stats.rxRtcpFrameLength[1], mmConnRtcp->stats.rxRtcpFrameBuffer[1]);
        }


        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_SECOND_TX_RTCP_LENGTH, mmConnRtcp->stats.txRtcpFrameLength[1]) < 0) {
            break;
        }
        if (mmConnRtcp->stats.txRtcpFrameLength[1] > 0) {
            nla_put(skb, MMSWITCH_ATTR_MMCONNRTCP_SECOND_TX_RTCP_BUFFER, mmConnRtcp->stats.txRtcpFrameLength[1], mmConnRtcp->stats.txRtcpFrameBuffer[1]);
        }
#endif
#endif

        nla_nest_end(skb, nest);

        return 0;
    } while (0);

    MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
    return -1;
}
