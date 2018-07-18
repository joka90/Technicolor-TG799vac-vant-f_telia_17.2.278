/*
 * INET		802.1Q VLAN
 *		Ethernet-type device handling.
 *
 * Authors:	Ben Greear <greearb@candelatech.com>
 *              Please send support related email to: netdev@vger.kernel.org
 *              VLAN Home Page: http://www.candelatech.com/~greear/vlan.html
 *
 * Fixes:
 *              Fix for packet capture - Nick Eggleston <nick@dccinc.com>;
 *		Add HW acceleration hooks - David S. Miller <davem@redhat.com>;
 *		Correct all the locking - David S. Miller <davem@redhat.com>;
 *		Use hash table for VLAN groups - David S. Miller <davem@redhat.com>
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/capability.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/rculist.h>
#include <net/p8022.h>
#include <net/arp.h>
#include <linux/rtnetlink.h>
#include <linux/notifier.h>
#include <net/rtnetlink.h>
#include <net/net_namespace.h>
#include <net/netns/generic.h>
#include <asm/uaccess.h>

#include <linux/if_vlan.h>
#include "vlan.h"
#include "vlanproc.h"

#if defined(CONFIG_BCM_KF_BLOG) && defined(CONFIG_BLOG)
#include <linux/blog.h>
#include <linux/blog_rule.h>
#endif

#define DRV_VERSION "1.8"

/* Global VLAN variables */

int vlan_net_id __read_mostly;

#if defined(CONFIG_BCM_KF_VLAN) && defined(CONFIG_BCM_KF_BLOG) && defined(CONFIG_BLOG) && defined(CONFIG_BLOG_MCAST)
struct vlan_dev_stack {
	unsigned int count;
	struct net_device *vlan_devices[BLOG_RULE_VLAN_TAG_MAX];
};
int vlan_dev_set_nfmark_to_priority(char *, int);
#endif

const char vlan_fullname[] = "802.1Q VLAN Support";
const char vlan_version[] = DRV_VERSION;

/* End of global variables definitions. */

static int vlan_group_prealloc_vid(struct vlan_group *vg, u16 vlan_id)
{
	struct net_device **array;
	unsigned int size;

	ASSERT_RTNL();

	array = vg->vlan_devices_arrays[vlan_id / VLAN_GROUP_ARRAY_PART_LEN];
	if (array != NULL)
		return 0;

	size = sizeof(struct net_device *) * VLAN_GROUP_ARRAY_PART_LEN;
	array = kzalloc(size, GFP_KERNEL);
	if (array == NULL)
		return -ENOBUFS;

	vg->vlan_devices_arrays[vlan_id / VLAN_GROUP_ARRAY_PART_LEN] = array;
	return 0;
}

void unregister_vlan_dev(struct net_device *dev, struct list_head *head)
{
	struct vlan_dev_priv *vlan = vlan_dev_priv(dev);
	struct net_device *real_dev = vlan->real_dev;
	struct vlan_info *vlan_info;
	struct vlan_group *grp;
	u16 vlan_id = vlan->vlan_id;
#if defined(CONFIG_BCM_KF_VLAN) && (defined(CONFIG_BCM_VLAN) || defined(CONFIG_BCM_VLAN_MODULE))
	int err;
#endif

	ASSERT_RTNL();

	vlan_info = rtnl_dereference(real_dev->vlan_info);
	BUG_ON(!vlan_info);

	grp = &vlan_info->grp;

	/* Take it out of our own structures, but be sure to interlock with
	 * HW accelerating devices or SW vlan input packet processing if
	 * VLAN is not 0 (leave it there for 802.1p).
	 */
	if (vlan_id)
		vlan_vid_del(real_dev, vlan_id);

	grp->nr_vlan_devs--;

	if (vlan->flags & VLAN_FLAG_GVRP)
		vlan_gvrp_request_leave(dev);

	vlan_group_set_device(grp, vlan_id, NULL);
#if defined(CONFIG_BCM_KF_VLAN) && (defined(CONFIG_BCM_VLAN) || defined(CONFIG_BCM_VLAN_MODULE))
	err = netdev_path_remove(dev);
	if (err) {
		pr_err("%s: failed to remove %s from Interface Path (%d)\n", __func__, dev->name, err);
		netdev_path_dump(dev);
	}
#endif
	/* Because unregister_netdevice_queue() makes sure at least one rcu
	 * grace period is respected before device freeing,
	 * we dont need to call synchronize_net() here.
	 */
	unregister_netdevice_queue(dev, head);

	if (grp->nr_vlan_devs == 0)
		vlan_gvrp_uninit_applicant(real_dev);

	/* Get rid of the vlan's reference to real_dev */
	dev_put(real_dev);
}

#if defined(CONFIG_BCM_KF_VLAN)
struct net_device_stats *vlan_dev_get_stats(struct net_device *dev)
{
	return &(dev->stats);
}
#if defined(CONFIG_BCM_KF_BLOG) && defined(CONFIG_BLOG)
static inline BlogStats_t *vlan_dev_get_bstats(struct net_device *dev)
{
	return &(vlan_dev_priv(dev)->bstats);
}
static inline struct net_device_stats *vlan_dev_get_cstats(struct net_device *dev)
{
	return &(vlan_dev_priv(dev)->cstats);
}

struct net_device_stats * vlan_dev_collect_stats(struct net_device * dev_p)
{
	BlogStats_t bStats;
	BlogStats_t * bStats_p;
	struct net_device_stats *dStats_p;
	struct net_device_stats *cStats_p;

	if ( dev_p == (struct net_device *)NULL )
		return (struct net_device_stats *)NULL;

	dStats_p = vlan_dev_get_stats(dev_p);
	cStats_p = vlan_dev_get_cstats(dev_p);
	bStats_p = vlan_dev_get_bstats(dev_p);

	memset(&bStats, 0, sizeof(BlogStats_t));

	blog_lock();
	blog_notify(FETCH_NETIF_STATS, (void*)dev_p,
				(uint32_t)&bStats, BLOG_PARAM2_NO_CLEAR);
	blog_unlock();

	memcpy( cStats_p, dStats_p, sizeof(struct net_device_stats) );
	cStats_p->rx_packets += ( bStats.rx_packets + bStats_p->rx_packets );
	cStats_p->tx_packets += ( bStats.tx_packets + bStats_p->tx_packets );

	/* set byte counts to 0 if the bstat packet counts are non 0 and the
		octet counts are 0 */
	if ( ((bStats.rx_bytes + bStats_p->rx_bytes) == 0) &&
		  ((bStats.rx_packets + bStats_p->rx_packets) > 0) )
	{
		cStats_p->rx_bytes = 0;
	}
	else
	{
		cStats_p->rx_bytes   += ( bStats.rx_bytes   + bStats_p->rx_bytes );
	}

	if ( ((bStats.tx_bytes + bStats_p->tx_bytes) == 0) &&
		  ((bStats.tx_packets + bStats_p->tx_packets) > 0) )
	{
		cStats_p->tx_bytes = 0;
	}
	else
	{
		cStats_p->tx_bytes   += ( bStats.tx_bytes   + bStats_p->tx_bytes );
	}
	cStats_p->multicast  += ( bStats.multicast  + bStats_p->multicast );

	return cStats_p;
}

void vlan_dev_update_stats(struct net_device * dev_p, BlogStats_t *blogStats_p)
{
	BlogStats_t * bStats_p;

	if ( dev_p == (struct net_device *)NULL )
		return;
	bStats_p = vlan_dev_get_bstats(dev_p);

	bStats_p->rx_packets += blogStats_p->rx_packets;
	bStats_p->tx_packets += blogStats_p->tx_packets;
	bStats_p->rx_bytes   += blogStats_p->rx_bytes;
	bStats_p->tx_bytes   += blogStats_p->tx_bytes;
	bStats_p->multicast  += blogStats_p->multicast;
	return;
}

void vlan_dev_clear_stats(struct net_device * dev_p)
{
	BlogStats_t * bStats_p;
	struct net_device_stats *dStats_p;
	struct net_device_stats *cStats_p;

	if ( dev_p == (struct net_device *)NULL )
		return;

	dStats_p = vlan_dev_get_stats(dev_p);
	cStats_p = vlan_dev_get_cstats(dev_p); 
	bStats_p = vlan_dev_get_bstats(dev_p);

	blog_lock();
	blog_notify(FETCH_NETIF_STATS, (void*)dev_p, 0, BLOG_PARAM2_DO_CLEAR);
	blog_unlock();

	memset(bStats_p, 0, sizeof(BlogStats_t));
	memset(dStats_p, 0, sizeof(struct net_device_stats));
	memset(cStats_p, 0, sizeof(struct net_device_stats));

	return;
}

#if defined(CONFIG_BLOG_MCAST)
static int vlan_build_dev_stack(struct net_device *vlandev,
				struct vlan_dev_stack *vlan_flow)
{
	struct net_device *vlandev_list[BLOG_RULE_VLAN_TAG_MAX];
	struct net_device *dev;
	int i = 0;

	memset(vlan_flow, 0, sizeof(vlan_flow));
	dev = vlandev;
	while (dev) {
		if (netdev_path_is_root(dev))
			break;

		if (is_vlan_dev(dev)) {
			if (i == ARRAY_SIZE(vlandev_list)) {
				pr_err("%s: too many stacked VLAN interfaces\n", __func__);
				return -ENOMEM;
			}

			vlandev_list[i++] = dev;
		}

		dev = netdev_path_next_dev(dev);
	}

	while (i)
		vlan_flow->vlan_devices[vlan_flow->count++] = vlandev_list[--i];

	return 0;
}

static int vlan_fill_rule_filter(struct net_device **vlandev_list,
				 unsigned int nbr_of_vlandevs,
				 u32 *skb_prio,
				 blogRuleFilter_t *rule_filter)
{
	unsigned int i;

	if (nbr_of_vlandevs > BLOG_RULE_VLAN_TAG_MAX) {
		pr_err("%s: number of VLAN filters <%u> exceeds Blog Rule VLAN filters <%u>\n",
		       __func__, nbr_of_vlandevs, BLOG_RULE_VLAN_TAG_MAX);
		return -EINVAL;
	}

	rule_filter->nbrOfVlanTags = nbr_of_vlandevs;

	for (i = 0; i < nbr_of_vlandevs; i++) {
		rule_filter->vlan[i].mask.h_vlan_TCI |= VLAN_VID_MASK;
		rule_filter->vlan[i].value.h_vlan_TCI = vlan_dev_vlan_id(vlandev_list[i]);
        }

	if (skb_prio)
		rule_filter->skb.priority = *skb_prio;

	return 0;
}

static int vlan_flows2blog_rule(blogRule_t ***blog_rule_lnk,
			        blogRule_t *root_rule,
				struct vlan_dev_stack *rx_vlan_flow,
				struct vlan_dev_stack *tx_vlan_flow)
{
	blogRuleAction_t rule_action;
	unsigned int i;
	int ret = 0;

	**blog_rule_lnk = blog_rule_alloc();
	if (**blog_rule_lnk == NULL) {
		pr_err("%s: could not allocate Blog Rule\n", __func__);
		return -ENOMEM;
	}

	if (root_rule != NULL)
		/* Copy contents of root rule to new rule */
		***blog_rule_lnk = *root_rule;
	else
		blog_rule_init(**blog_rule_lnk);

	if (rx_vlan_flow) {
		ret = vlan_fill_rule_filter(rx_vlan_flow->vlan_devices, rx_vlan_flow->count,
					    NULL, &(**blog_rule_lnk)->filter);
		if (ret < 0)
			goto out;

		for (i = 0; i < rx_vlan_flow->count; i++) {
			memset(&rule_action, 0, sizeof(rule_action));
			rule_action.cmd = BLOG_RULE_CMD_POP_VLAN_HDR;

			ret = blog_rule_add_action(**blog_rule_lnk, &rule_action);
			if (ret < 0)
				goto out;
		}
	}

	if (tx_vlan_flow) {
		for (i = 0; i < tx_vlan_flow->count; i++) {
			memset(&rule_action, 0, sizeof(rule_action));
			rule_action.cmd = BLOG_RULE_CMD_PUSH_VLAN_HDR;

			ret = blog_rule_add_action(**blog_rule_lnk, &rule_action);
			if (ret < 0)
				goto out;

			memset(&rule_action, 0, sizeof(rule_action));
			rule_action.cmd = BLOG_RULE_CMD_SET_VID;
			rule_action.toTag = i;
			rule_action.vid = vlan_dev_vlan_id(tx_vlan_flow->vlan_devices[i]);

			ret = blog_rule_add_action(**blog_rule_lnk, &rule_action);
			if (ret < 0)
				goto out;
		}
	}

out:
	if (ret < 0) {
		blog_rule_free(**blog_rule_lnk);
		**blog_rule_lnk = NULL;
	} else
		*blog_rule_lnk = &(**blog_rule_lnk)->next_p;

	return ret;
}

static int vlan_create_blog_rules(Blog_t *blog,
				  struct vlan_dev_stack *rx_vlan_flow,
				  struct vlan_dev_stack *tx_vlan_flow)
{
	/* Function is written in the assumption that multiple VLAN rules */
	/* can be inserted in the blog                                    */
	blogRule_t *root_rule, **lnk = (blogRule_t **)&blog->blogRule_p;
	int ret = 0;

	/* get the root blog rule */
	root_rule = (blogRule_t *)blog->blogRule_p;
	if (root_rule != NULL && root_rule->next_p != NULL) {
		pr_err("%s: only one Root Blog Rule is supported\n", __func__);
		ret = -EINVAL;
		goto out;
	}

	ret = vlan_flows2blog_rule(&lnk, root_rule, rx_vlan_flow, tx_vlan_flow);

	/* Installing ingress qos mapping (tci -> skb_prio) is not supported by BRCM fap API */
	/* No use to install rules for egress mapping */

out:
	if (root_rule != NULL)
		blog_rule_free(root_rule);

	return ret;
}

static int vlan_blog_vlan_flows(Blog_t *blog, struct net_device *rx_vlandev,
				struct net_device *tx_vlandev)
{
	struct vlan_dev_stack rx_vlan_flow, tx_vlan_flow;
	int ret = 0;

	if (!rx_vlandev && !tx_vlandev) {
		pr_err("%s: no vlan devices specified\n", __func__);
		ret = -EINVAL;
		goto out;
	}

	if (rx_vlandev && !is_vlan_dev(rx_vlandev)) {
		pr_err("%s: device %s is not a VLAN device\n", __func__, rx_vlandev->name);
		ret = -EINVAL;
		goto out;
	}

	if (tx_vlandev && !is_vlan_dev(tx_vlandev)) {
		pr_err("%s: device %s is not a VLAN device\n", __func__, tx_vlandev->name);
		ret = -EINVAL;
		goto out;
	}

	ret = vlan_build_dev_stack(rx_vlandev, &rx_vlan_flow);
	if (ret < 0) {
		pr_err("%s: failed to build rx vlan device stack\n", __func__);
		goto out;
	}

	ret = vlan_build_dev_stack(tx_vlandev, &tx_vlan_flow);
	if (ret < 0) {
		pr_err("%s: failed to build tx vlan device stack\n", __func__);
		goto out;
	}

	ret = vlan_create_blog_rules(blog, &rx_vlan_flow, &tx_vlan_flow);

out:
	return ret;
}

static int vlan_flow_init(void)
{
	if (blogRuleVlan802_1QHook != NULL) {
		pr_err("%s: failed to bind Blog Rule VLAN 802.1Q hook\n", __func__);
		return -EINVAL;
	}

	blogRuleVlan802_1QHook = vlan_blog_vlan_flows;

	return 0;
}

static void vlan_flow_uninit(void)
{
	if (blogRuleVlan802_1QHook == vlan_blog_vlan_flows)
		blogRuleVlan802_1QHook = NULL;
}
#endif //CONFIG_BLOG_MCAST
#endif //CONFIG_BLOG
#endif //CONFIG_VLAN_STATS

int vlan_check_real_dev(struct net_device *real_dev, u16 vlan_id)
{
	const char *name = real_dev->name;
	const struct net_device_ops *ops = real_dev->netdev_ops;

	if (real_dev->features & NETIF_F_VLAN_CHALLENGED) {
		pr_info("VLANs not supported on %s\n", name);
		return -EOPNOTSUPP;
	}

	if ((real_dev->features & NETIF_F_HW_VLAN_FILTER) &&
	    (!ops->ndo_vlan_rx_add_vid || !ops->ndo_vlan_rx_kill_vid)) {
		pr_info("Device %s has buggy VLAN hw accel\n", name);
		return -EOPNOTSUPP;
	}

	if (vlan_find_dev(real_dev, vlan_id) != NULL)
		return -EEXIST;

	return 0;
}

int register_vlan_dev(struct net_device *dev)
{
	struct vlan_dev_priv *vlan = vlan_dev_priv(dev);
	struct net_device *real_dev = vlan->real_dev;
	u16 vlan_id = vlan->vlan_id;
	struct vlan_info *vlan_info;
	struct vlan_group *grp;
	int err;

	err = vlan_vid_add(real_dev, vlan_id);
	if (err)
		return err;

	vlan_info = rtnl_dereference(real_dev->vlan_info);
	/* vlan_info should be there now. vlan_vid_add took care of it */
	BUG_ON(!vlan_info);

	grp = &vlan_info->grp;
	if (grp->nr_vlan_devs == 0) {
		err = vlan_gvrp_init_applicant(real_dev);
		if (err < 0)
			goto out_vid_del;
	}

	err = vlan_group_prealloc_vid(grp, vlan_id);
	if (err < 0)
		goto out_uninit_applicant;

	err = register_netdevice(dev);
	if (err < 0)
		goto out_uninit_applicant;

	/* Account for reference in struct vlan_dev_priv */
	dev_hold(real_dev);

	netif_stacked_transfer_operstate(real_dev, dev);
	linkwatch_fire_event(dev); /* _MUST_ call rfc2863_policy() */

	/* So, got the sucker initialized, now lets place
	 * it into our local structure.
	 */
	vlan_group_set_device(grp, vlan_id, dev);
	grp->nr_vlan_devs++;

	return 0;

out_uninit_applicant:
	if (grp->nr_vlan_devs == 0)
		vlan_gvrp_uninit_applicant(real_dev);
out_vid_del:
	vlan_vid_del(real_dev, vlan_id);
	return err;
}

/*  Attach a VLAN device to a mac address (ie Ethernet Card).
 *  Returns 0 if the device was created or a negative error code otherwise.
 */
static int register_vlan_device(struct net_device *real_dev, u16 vlan_id)
{
	struct net_device *new_dev;
	struct net *net = dev_net(real_dev);
	struct vlan_net *vn = net_generic(net, vlan_net_id);
	char name[IFNAMSIZ];
	int err;

	if (vlan_id >= VLAN_VID_MASK)
		return -ERANGE;

	err = vlan_check_real_dev(real_dev, vlan_id);
	if (err < 0)
		return err;

	/* Gotta set up the fields for the device. */
	switch (vn->name_type) {
	case VLAN_NAME_TYPE_RAW_PLUS_VID:
		/* name will look like:	 eth1.0005 */
		snprintf(name, IFNAMSIZ, "%s.%.4i", real_dev->name, vlan_id);
		break;
	case VLAN_NAME_TYPE_PLUS_VID_NO_PAD:
		/* Put our vlan.VID in the name.
		 * Name will look like:	 vlan5
		 */
		snprintf(name, IFNAMSIZ, "vlan%i", vlan_id);
		break;
	case VLAN_NAME_TYPE_RAW_PLUS_VID_NO_PAD:
		/* Put our vlan.VID in the name.
		 * Name will look like:	 eth0.5
		 */
		snprintf(name, IFNAMSIZ, "%s.%i", real_dev->name, vlan_id);
		break;
	case VLAN_NAME_TYPE_PLUS_VID:
		/* Put our vlan.VID in the name.
		 * Name will look like:	 vlan0005
		 */
	default:
		snprintf(name, IFNAMSIZ, "vlan%.4i", vlan_id);
	}

	new_dev = alloc_netdev(sizeof(struct vlan_dev_priv), name, vlan_setup);

	if (new_dev == NULL)
		return -ENOBUFS;

#if defined(CONFIG_BCM_KF_VLAN) && (defined(CONFIG_BCM_VLAN) || defined(CONFIG_BCM_VLAN_MODULE))
	/* If real device is a hardware switch port, the vlan device must also be */
	new_dev->priv_flags |= real_dev->priv_flags;
	/* it's not because the real device is acting as a bridge port, the VLAN device should to */
	new_dev->priv_flags &= ~IFF_BRIDGE_PORT;
#endif

	dev_net_set(new_dev, net);
	/* need 4 bytes for extra VLAN header info,
	 * hope the underlying device can handle it.
	 */
	new_dev->mtu = real_dev->mtu;

	vlan_dev_priv(new_dev)->vlan_id = vlan_id;
	vlan_dev_priv(new_dev)->real_dev = real_dev;
	vlan_dev_priv(new_dev)->dent = NULL;
	vlan_dev_priv(new_dev)->flags = VLAN_FLAG_REORDER_HDR;

#if defined(CONFIG_BCM_KF_VLAN) && (defined(CONFIG_BCM_VLAN) || defined(CONFIG_BCM_VLAN_MODULE))
	new_dev->path.hw_port_type = real_dev->path.hw_port_type;
	err = netdev_path_add(new_dev, real_dev);
	if (err < 0) {
		pr_err("%s: failed to add %s to Interface path (%d)\n", __func__, new_dev->name, err);
		goto out_free_newdev;
	}

	netdev_path_dump(new_dev);
#endif

	new_dev->rtnl_link_ops = &vlan_link_ops;
	err = register_vlan_dev(new_dev);
	if (err < 0)
		goto out_free_newdev;

	return 0;

out_free_newdev:
	free_netdev(new_dev);
	return err;
}

static void vlan_sync_address(struct net_device *dev,
			      struct net_device *vlandev)
{
	struct vlan_dev_priv *vlan = vlan_dev_priv(vlandev);

	/* May be called without an actual change */
	if (!compare_ether_addr(vlan->real_dev_addr, dev->dev_addr))
		return;

	/* vlan address was different from the old address and is equal to
	 * the new address */
	if (compare_ether_addr(vlandev->dev_addr, vlan->real_dev_addr) &&
	    !compare_ether_addr(vlandev->dev_addr, dev->dev_addr))
		dev_uc_del(dev, vlandev->dev_addr);

	/* vlan address was equal to the old address and is different from
	 * the new address */
	if (!compare_ether_addr(vlandev->dev_addr, vlan->real_dev_addr) &&
	    compare_ether_addr(vlandev->dev_addr, dev->dev_addr))
		dev_uc_add(dev, vlandev->dev_addr);

	memcpy(vlan->real_dev_addr, dev->dev_addr, ETH_ALEN);
}

static void vlan_transfer_features(struct net_device *dev,
				   struct net_device *vlandev)
{
	vlandev->gso_max_size = dev->gso_max_size;

	if (dev->features & NETIF_F_HW_VLAN_TX)
		vlandev->hard_header_len = dev->hard_header_len;
	else
		vlandev->hard_header_len = dev->hard_header_len + VLAN_HLEN;

#if defined(CONFIG_FCOE) || defined(CONFIG_FCOE_MODULE)
	vlandev->fcoe_ddp_xid = dev->fcoe_ddp_xid;
#endif

	netdev_update_features(vlandev);
}

static void __vlan_device_event(struct net_device *dev, unsigned long event)
{
	switch (event) {
	case NETDEV_CHANGENAME:
		vlan_proc_rem_dev(dev);
		if (vlan_proc_add_dev(dev) < 0)
			pr_warn("failed to change proc name for %s\n",
				dev->name);
		break;
	case NETDEV_REGISTER:
		if (vlan_proc_add_dev(dev) < 0)
			pr_warn("failed to add proc entry for %s\n", dev->name);
		break;
	case NETDEV_UNREGISTER:
		vlan_proc_rem_dev(dev);
		break;
	}
}

static int vlan_device_event(struct notifier_block *unused, unsigned long event,
			     void *ptr)
{
	struct net_device *dev = ptr;
	struct vlan_group *grp;
	struct vlan_info *vlan_info;
	int i, flgs;
	struct net_device *vlandev;
	struct vlan_dev_priv *vlan;
	LIST_HEAD(list);

	if (is_vlan_dev(dev))
		__vlan_device_event(dev, event);

	if ((event == NETDEV_UP) &&
	    (dev->features & NETIF_F_HW_VLAN_FILTER)) {
		pr_info("adding VLAN 0 to HW filter on device %s\n",
			dev->name);
		vlan_vid_add(dev, 0);
	}

	vlan_info = rtnl_dereference(dev->vlan_info);
	if (!vlan_info)
		goto out;
	grp = &vlan_info->grp;

	/* It is OK that we do not hold the group lock right now,
	 * as we run under the RTNL lock.
	 */

	switch (event) {
	case NETDEV_CHANGE:
		/* Propagate real device state to vlan devices */
		for (i = 0; i < VLAN_N_VID; i++) {
			vlandev = vlan_group_get_device(grp, i);
			if (!vlandev)
				continue;

			netif_stacked_transfer_operstate(dev, vlandev);
		}
		break;

	case NETDEV_CHANGEADDR:
		/* Adjust unicast filters on underlying device */
		for (i = 0; i < VLAN_N_VID; i++) {
			vlandev = vlan_group_get_device(grp, i);
			if (!vlandev)
				continue;

			flgs = vlandev->flags;
			if (!(flgs & IFF_UP))
				continue;

			vlan_sync_address(dev, vlandev);
		}
		break;

	case NETDEV_CHANGEMTU:
		for (i = 0; i < VLAN_N_VID; i++) {
			vlandev = vlan_group_get_device(grp, i);
			if (!vlandev)
				continue;

			if (vlandev->mtu <= dev->mtu)
				continue;

			dev_set_mtu(vlandev, dev->mtu);
		}
		break;

	case NETDEV_FEAT_CHANGE:
		/* Propagate device features to underlying device */
		for (i = 0; i < VLAN_N_VID; i++) {
			vlandev = vlan_group_get_device(grp, i);
			if (!vlandev)
				continue;

			vlan_transfer_features(dev, vlandev);
		}

		break;

	case NETDEV_DOWN:
		if (dev->features & NETIF_F_HW_VLAN_FILTER)
			vlan_vid_del(dev, 0);

		/* Put all VLANs for this dev in the down state too.  */
		for (i = 0; i < VLAN_N_VID; i++) {
			vlandev = vlan_group_get_device(grp, i);
			if (!vlandev)
				continue;

			flgs = vlandev->flags;
			if (!(flgs & IFF_UP))
				continue;

			vlan = vlan_dev_priv(vlandev);
			if (!(vlan->flags & VLAN_FLAG_LOOSE_BINDING))
				dev_change_flags(vlandev, flgs & ~IFF_UP);
			netif_stacked_transfer_operstate(dev, vlandev);
		}
		break;

	case NETDEV_UP:
		/* Put all VLANs for this dev in the up state too.  */
		for (i = 0; i < VLAN_N_VID; i++) {
			vlandev = vlan_group_get_device(grp, i);
			if (!vlandev)
				continue;

			flgs = vlandev->flags;
			if (flgs & IFF_UP)
				continue;

			vlan = vlan_dev_priv(vlandev);
			if (!(vlan->flags & VLAN_FLAG_LOOSE_BINDING))
				dev_change_flags(vlandev, flgs | IFF_UP);
			netif_stacked_transfer_operstate(dev, vlandev);
		}
		break;

	case NETDEV_UNREGISTER:
		/* twiddle thumbs on netns device moves */
		if (dev->reg_state != NETREG_UNREGISTERING)
			break;

		for (i = 0; i < VLAN_N_VID; i++) {
			vlandev = vlan_group_get_device(grp, i);
			if (!vlandev)
				continue;

			/* removal of last vid destroys vlan_info, abort
			 * afterwards */
			if (vlan_info->nr_vids == 1)
				i = VLAN_N_VID;

			unregister_vlan_dev(vlandev, &list);
		}
		unregister_netdevice_many(&list);
		break;

	case NETDEV_PRE_TYPE_CHANGE:
		/* Forbid underlaying device to change its type. */
		return NOTIFY_BAD;

	case NETDEV_NOTIFY_PEERS:
	case NETDEV_BONDING_FAILOVER:
		/* Propagate to vlan devices */
		for (i = 0; i < VLAN_N_VID; i++) {
			vlandev = vlan_group_get_device(grp, i);
			if (!vlandev)
				continue;

			call_netdevice_notifiers(event, vlandev);
		}
		break;
	}

out:
	return NOTIFY_DONE;
}

static struct notifier_block vlan_notifier_block __read_mostly = {
	.notifier_call = vlan_device_event,
};

/*
 *	VLAN IOCTL handler.
 *	o execute requested action or pass command to the device driver
 *   arg is really a struct vlan_ioctl_args __user *.
 */
static int vlan_ioctl_handler(struct net *net, void __user *arg)
{
	int err;
	struct vlan_ioctl_args args;
	struct net_device *dev = NULL;

	if (copy_from_user(&args, arg, sizeof(struct vlan_ioctl_args)))
		return -EFAULT;

	/* Null terminate this sucker, just in case. */
	args.device1[23] = 0;
	args.u.device2[23] = 0;

	rtnl_lock();

	switch (args.cmd) {
	case SET_VLAN_INGRESS_PRIORITY_CMD:
	case SET_VLAN_EGRESS_PRIORITY_CMD:
	case SET_VLAN_FLAG_CMD:
	case ADD_VLAN_CMD:
	case DEL_VLAN_CMD:
	case GET_VLAN_REALDEV_NAME_CMD:
	case GET_VLAN_VID_CMD:
		err = -ENODEV;
		dev = __dev_get_by_name(net, args.device1);
		if (!dev)
			goto out;

		err = -EINVAL;
		if (args.cmd != ADD_VLAN_CMD && !is_vlan_dev(dev))
			goto out;
	}

	switch (args.cmd) {
	case SET_VLAN_INGRESS_PRIORITY_CMD:
		err = -EPERM;
		if (!capable(CAP_NET_ADMIN))
			break;
		vlan_dev_set_ingress_priority(dev,
					      args.u.skb_priority,
					      args.vlan_qos);
		err = 0;
		break;

	case SET_VLAN_EGRESS_PRIORITY_CMD:
		err = -EPERM;
		if (!capable(CAP_NET_ADMIN))
			break;
		err = vlan_dev_set_egress_priority(dev,
						   args.u.skb_priority,
						   args.vlan_qos);
		break;

#if defined(CONFIG_BCM_KF_VLAN) && (defined(CONFIG_BCM_VLAN) || defined(CONFIG_BCM_VLAN_MODULE))
	case SET_VLAN_NFMARK_TO_PRIORITY_CMD:
		err = vlan_dev_set_nfmark_to_priority(args.device1,
						   args.u.nfmark_to_priority);
		break;
#endif  

	case SET_VLAN_FLAG_CMD:
		err = -EPERM;
		if (!capable(CAP_NET_ADMIN))
			break;
		err = vlan_dev_change_flags(dev,
					    args.vlan_qos ? args.u.flag : 0,
					    args.u.flag);
		break;

	case SET_VLAN_NAME_TYPE_CMD:
		err = -EPERM;
		if (!capable(CAP_NET_ADMIN))
			break;
		if ((args.u.name_type >= 0) &&
		    (args.u.name_type < VLAN_NAME_TYPE_HIGHEST)) {
			struct vlan_net *vn;

			vn = net_generic(net, vlan_net_id);
			vn->name_type = args.u.name_type;
			err = 0;
		} else {
			err = -EINVAL;
		}
		break;

	case ADD_VLAN_CMD:
		err = -EPERM;
		if (!capable(CAP_NET_ADMIN))
			break;
		err = register_vlan_device(dev, args.u.VID);
		break;

	case DEL_VLAN_CMD:
		err = -EPERM;
		if (!capable(CAP_NET_ADMIN))
			break;
		unregister_vlan_dev(dev, NULL);
		err = 0;
		break;

	case GET_VLAN_REALDEV_NAME_CMD:
		err = 0;
		vlan_dev_get_realdev_name(dev, args.u.device2);
		if (copy_to_user(arg, &args,
				 sizeof(struct vlan_ioctl_args)))
			err = -EFAULT;
		break;

	case GET_VLAN_VID_CMD:
		err = 0;
		args.u.VID = vlan_dev_vlan_id(dev);
		if (copy_to_user(arg, &args,
				 sizeof(struct vlan_ioctl_args)))
		      err = -EFAULT;
		break;

	default:
		err = -EOPNOTSUPP;
		break;
	}
out:
	rtnl_unlock();
	return err;
}

static int __net_init vlan_init_net(struct net *net)
{
	struct vlan_net *vn = net_generic(net, vlan_net_id);
	int err;

	vn->name_type = VLAN_NAME_TYPE_RAW_PLUS_VID_NO_PAD;

	err = vlan_proc_init(net);

	return err;
}

static void __net_exit vlan_exit_net(struct net *net)
{
	vlan_proc_cleanup(net);
}

static struct pernet_operations vlan_net_ops = {
	.init = vlan_init_net,
	.exit = vlan_exit_net,
	.id   = &vlan_net_id,
	.size = sizeof(struct vlan_net),
};

static int __init vlan_proto_init(void)
{
	int err;

	pr_info("%s v%s\n", vlan_fullname, vlan_version);

	err = register_pernet_subsys(&vlan_net_ops);
	if (err < 0)
		goto err0;

	err = register_netdevice_notifier(&vlan_notifier_block);
	if (err < 0)
		goto err2;

	err = vlan_gvrp_init();
	if (err < 0)
		goto err3;

	err = vlan_netlink_init();
	if (err < 0)
		goto err4;

#if defined(CONFIG_BCM_KF_VLAN) && defined(CONFIG_BCM_KF_BLOG) && defined(CONFIG_BLOG) && defined(CONFIG_BLOG_MCAST)
	err = vlan_flow_init();
	if (err < 0)
		goto err5;
#endif
	vlan_ioctl_set(vlan_ioctl_handler);
	return 0;
#if defined(CONFIG_BCM_KF_VLAN) && defined(CONFIG_BCM_KF_BLOG) && defined(CONFIG_BLOG) && defined(CONFIG_BLOG_MCAST)
err5:
	vlan_netlink_fini();
#endif
err4:
	vlan_gvrp_uninit();
err3:
	unregister_netdevice_notifier(&vlan_notifier_block);
err2:
	unregister_pernet_subsys(&vlan_net_ops);
err0:
	return err;
}

static void __exit vlan_cleanup_module(void)
{
	vlan_ioctl_set(NULL);
	vlan_netlink_fini();

	unregister_netdevice_notifier(&vlan_notifier_block);

	unregister_pernet_subsys(&vlan_net_ops);
	rcu_barrier(); /* Wait for completion of call_rcu()'s */

	vlan_gvrp_uninit();
#if defined(CONFIG_BCM_KF_VLAN) && defined(CONFIG_BCM_KF_BLOG) && defined(CONFIG_BLOG) && defined(CONFIG_BLOG_MCAST)
	vlan_flow_uninit();
#endif
}

module_init(vlan_proto_init);
module_exit(vlan_cleanup_module);

MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
