/* (C) 1999-2001 Paul `Rusty' Russell
 * (C) 2002-2004 Netfilter Core Team <coreteam@netfilter.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/types.h>
#include <linux/netfilter.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/percpu.h>
#include <linux/netdevice.h>
#include <linux/security.h>
#include <net/net_namespace.h>
#ifdef CONFIG_SYSCTL
#include <linux/sysctl.h>
#endif

#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_core.h>
#include <net/netfilter/nf_conntrack_l3proto.h>
#include <net/netfilter/nf_conntrack_l4proto.h>
#include <net/netfilter/nf_conntrack_expect.h>
#include <net/netfilter/nf_conntrack_helper.h>
#include <net/netfilter/nf_conntrack_acct.h>
#include <net/netfilter/nf_conntrack_zones.h>
#include <net/netfilter/nf_conntrack_timestamp.h>
#include <linux/rculist_nulls.h>

#if defined(CONFIG_BCM_KF_DPI) && defined(CONFIG_BRCM_DPI)
#include <linux/devinfo.h>
#include <linux/dpistats.h>
#include <linux/urlinfo.h>
#include <linux/dpi_ctk.h>
#endif

MODULE_LICENSE("GPL");

#ifdef CONFIG_NF_CONNTRACK_PROCFS
int
print_tuple(struct seq_file *s, const struct nf_conntrack_tuple *tuple,
            const struct nf_conntrack_l3proto *l3proto,
            const struct nf_conntrack_l4proto *l4proto)
{
	return l3proto->print_tuple(s, tuple) || l4proto->print_tuple(s, tuple);
}
EXPORT_SYMBOL_GPL(print_tuple);

struct ct_iter_state {
	struct seq_net_private p;
	unsigned int bucket;
	u_int64_t time_now;
};

static struct hlist_nulls_node *ct_get_first(struct seq_file *seq)
{
	struct net *net = seq_file_net(seq);
	struct ct_iter_state *st = seq->private;
	struct hlist_nulls_node *n;

	for (st->bucket = 0;
	     st->bucket < net->ct.htable_size;
	     st->bucket++) {
		n = rcu_dereference(hlist_nulls_first_rcu(&net->ct.hash[st->bucket]));
		if (!is_a_nulls(n))
			return n;
	}
	return NULL;
}

static struct hlist_nulls_node *ct_get_next(struct seq_file *seq,
				      struct hlist_nulls_node *head)
{
	struct net *net = seq_file_net(seq);
	struct ct_iter_state *st = seq->private;

	head = rcu_dereference(hlist_nulls_next_rcu(head));
	while (is_a_nulls(head)) {
		if (likely(get_nulls_value(head) == st->bucket)) {
			if (++st->bucket >= net->ct.htable_size)
				return NULL;
		}
		head = rcu_dereference(
				hlist_nulls_first_rcu(
					&net->ct.hash[st->bucket]));
	}
	return head;
}

static struct hlist_nulls_node *ct_get_idx(struct seq_file *seq, loff_t pos)
{
	struct hlist_nulls_node *head = ct_get_first(seq);

	if (head)
		while (pos && (head = ct_get_next(seq, head)))
			pos--;
	return pos ? NULL : head;
}

static void *ct_seq_start(struct seq_file *seq, loff_t *pos)
	__acquires(RCU)
{
	struct ct_iter_state *st = seq->private;

	st->time_now = ktime_to_ns(ktime_get_real());
	rcu_read_lock();
	return ct_get_idx(seq, *pos);
}

static void *ct_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	(*pos)++;
	return ct_get_next(s, v);
}

static void ct_seq_stop(struct seq_file *s, void *v)
	__releases(RCU)
{
	rcu_read_unlock();
}

#ifdef CONFIG_NF_CONNTRACK_SECMARK
static int ct_show_secctx(struct seq_file *s, const struct nf_conn *ct)
{
	int ret;
	u32 len;
	char *secctx;

	ret = security_secid_to_secctx(ct->secmark, &secctx, &len);
	if (ret)
		return 0;

	ret = seq_printf(s, "secctx=%s ", secctx);

	security_release_secctx(secctx, len);
	return ret;
}
#else
static inline int ct_show_secctx(struct seq_file *s, const struct nf_conn *ct)
{
	return 0;
}
#endif

#ifdef CONFIG_NF_CONNTRACK_TIMESTAMP
static int ct_show_delta_time(struct seq_file *s, const struct nf_conn *ct)
{
	struct ct_iter_state *st = s->private;
	struct nf_conn_tstamp *tstamp;
	s64 delta_time;

	tstamp = nf_conn_tstamp_find(ct);
	if (tstamp) {
		delta_time = st->time_now - tstamp->start;
		if (delta_time > 0)
			delta_time = div_s64(delta_time, NSEC_PER_SEC);
		else
			delta_time = 0;

		return seq_printf(s, "delta-time=%llu ",
				  (unsigned long long)delta_time);
	}
	return 0;
}
#else
static inline int
ct_show_delta_time(struct seq_file *s, const struct nf_conn *ct)
{
	return 0;
}
#endif

#if defined(CONFIG_BCM_KF_BLOG) && defined(CONFIG_BCM_KF_NETFILTER) && defined(CONFIG_BLOG)
static void ct_blog_query(struct nf_conn *ct, BlogCtTime_t *ct_time_p)
{
	blog_lock();
	if (ct->blog_key[BLOG_PARAM1_DIR_ORIG] != BLOG_KEY_NONE || 
		ct->blog_key[BLOG_PARAM1_DIR_REPLY] != BLOG_KEY_NONE) {
		blog_query(QUERY_FLOWTRACK, (void*)ct, 
	    	ct->blog_key[BLOG_PARAM1_DIR_ORIG],
			ct->blog_key[BLOG_PARAM1_DIR_REPLY], (uint32_t) ct_time_p);
	}
	blog_unlock();
}

static inline long ct_blog_calc_timeout(struct nf_conn *ct, 
		BlogCtTime_t *ct_time_p)
{
	long ct_time;

	blog_lock();
	if (ct->blog_key[BLOG_PARAM1_DIR_ORIG] != BLOG_KEY_NONE || 
		ct->blog_key[BLOG_PARAM1_DIR_REPLY] != BLOG_KEY_NONE) {
		unsigned long partial_intv;             /* to provide more accuracy */
		unsigned long intv_jiffies = ct_time_p->intv * HZ;

        if (jiffies > ct->prev_timeout.expires)
			partial_intv = (jiffies - ct->prev_timeout.expires) % intv_jiffies; 
        else
			partial_intv = (ULONG_MAX - ct->prev_timeout.expires 
												+ jiffies) % intv_jiffies; 

		ct_time = (long)(ct->timeout.expires - ct->prev_timeout.expires 
								- ct_time_p->idle_jiffies - partial_intv);
		if( (ct_time_p->proto == IPPROTO_UDP && test_bit(IPS_SEEN_REPLY_BIT, &ct->status))
			 || ct_time < 0)
		{	 
			 ct_time = (long)(ct_time_p->extra_jiffies- ct_time_p->idle_jiffies);
		 }			
	}
	else
		ct_time = (long)(ct->timeout.expires - jiffies);

	blog_unlock();
	return ct_time;
}
#endif

/* return 0 on success, 1 in case of error */
static int ct_seq_show(struct seq_file *s, void *v)
{
	struct nf_conntrack_tuple_hash *hash = v;
	struct nf_conn *ct = nf_ct_tuplehash_to_ctrack(hash);
	const struct nf_conntrack_l3proto *l3proto;
	const struct nf_conntrack_l4proto *l4proto;
	int ret = 0;
#if defined(CONFIG_BCM_KF_BLOG) && defined(CONFIG_BCM_KF_NETFILTER) && defined(CONFIG_BLOG)
    BlogCtTime_t ct_time;
#endif

	NF_CT_ASSERT(ct);
	if (unlikely(!atomic_inc_not_zero(&ct->ct_general.use)))
		return 0;

	/* we only want to print DIR_ORIGINAL */
	if (NF_CT_DIRECTION(hash))
		goto release;

	l3proto = __nf_ct_l3proto_find(nf_ct_l3num(ct));
	NF_CT_ASSERT(l3proto);
	l4proto = __nf_ct_l4proto_find(nf_ct_l3num(ct), nf_ct_protonum(ct));
	NF_CT_ASSERT(l4proto);

	ret = -ENOSPC;

#if defined(CONFIG_BCM_KF_BLOG) && defined(CONFIG_BCM_KF_NETFILTER) && defined(CONFIG_BLOG)
    ct_blog_query(ct, &ct_time);
if(ct_time.proto == IPPROTO_UDP && test_bit(IPS_SEEN_REPLY_BIT, &ct->status) )
{
	if (seq_printf(s, "%-8s %u %-8s %u %ld ",
		       l3proto->name, nf_ct_l3num(ct),
		       l4proto->name, nf_ct_protonum(ct),
		       ct_blog_calc_timeout(ct, &ct_time)/HZ) != 0)
	goto release;	       
}    		       
else
{ 
	if (seq_printf(s, "%-8s %u %-8s %u %ld ",
		       l3proto->name, nf_ct_l3num(ct),
		       l4proto->name, nf_ct_protonum(ct),
		       timer_pending(&ct->timeout)
			   ? ct_blog_calc_timeout(ct, &ct_time)/HZ : 0) != 0)
	goto release;
}
			   
#else
	if (seq_printf(s, "%-8s %u %-8s %u %ld ",
		       l3proto->name, nf_ct_l3num(ct),
		       l4proto->name, nf_ct_protonum(ct),
		       timer_pending(&ct->timeout)
		       ? (long)(ct->timeout.expires - jiffies)/HZ : 0) != 0)
	goto release;
#endif

	if (l4proto->print_conntrack && l4proto->print_conntrack(s, ct))
		goto release;

	if (print_tuple(s, &ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple,
			l3proto, l4proto))
		goto release;

	if (seq_print_acct(s, ct, IP_CT_DIR_ORIGINAL))
		goto release;

	if (!(test_bit(IPS_SEEN_REPLY_BIT, &ct->status)))
		if (seq_printf(s, "[UNREPLIED] "))
			goto release;

	if (print_tuple(s, &ct->tuplehash[IP_CT_DIR_REPLY].tuple,
			l3proto, l4proto))
		goto release;

	if (seq_print_acct(s, ct, IP_CT_DIR_REPLY))
		goto release;

	if (test_bit(IPS_ASSURED_BIT, &ct->status))
		if (seq_printf(s, "[ASSURED] "))
			goto release;

#if defined(CONFIG_NF_CONNTRACK_MARK)
	if (seq_printf(s, "mark=%u ", ct->mark))
		goto release;
#endif

	if (ct_show_secctx(s, ct))
		goto release;

#ifdef CONFIG_NF_CONNTRACK_ZONES
	if (seq_printf(s, "zone=%u ", nf_ct_zone(ct)))
		goto release;
#endif

	if (ct_show_delta_time(s, ct))
		goto release;

#if defined(CONFIG_BCM_KF_BLOG)
	if (seq_printf(s, "blog=%u ", ct->blog))
		goto release;

	if (seq_printf(s, "iqprio=%u ", ct->iq_prio))
		goto release;
#endif

	if (seq_printf(s, "use=%u\n", atomic_read(&ct->ct_general.use)))
		goto release;

	ret = 0;
release:
	nf_ct_put(ct);
	return ret;
}

static const struct seq_operations ct_seq_ops = {
	.start = ct_seq_start,
	.next  = ct_seq_next,
	.stop  = ct_seq_stop,
	.show  = ct_seq_show
};

static int ct_open(struct inode *inode, struct file *file)
{
	return seq_open_net(inode, file, &ct_seq_ops,
			sizeof(struct ct_iter_state));
}

static const struct file_operations ct_file_ops = {
	.owner   = THIS_MODULE,
	.open    = ct_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release_net,
};

#if defined(CONFIG_BCM_KF_DPI) && defined(CONFIG_BRCM_DPI)
static void *ct_dpi_seq_start(struct seq_file *seq, loff_t *pos)
	__acquires(RCU)
{
	struct ct_iter_state *st = seq->private;

	rcu_read_lock();
	if (*pos == 0)
		return SEQ_START_TOKEN;

	st->time_now = ktime_to_ns(ktime_get_real());
	return ct_get_idx(seq, *pos);
}

static void *ct_dpi_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
    if (v == SEQ_START_TOKEN)
    {
        return ct_get_idx(s, *pos);
    }

	(*pos)++;
	return ct_get_next(s, v);
}

static void ct_dpi_seq_stop(struct seq_file *s, void *v)
	__releases(RCU)
{
	rcu_read_unlock();
}

/* return 0 on success, 1 in case of error */
static int ct_dpi_seq_show(struct seq_file *s, void *v)
{
	struct nf_conntrack_tuple_hash *hash;
	struct nf_conn *ct;
	const struct nf_conntrack_l3proto *l3proto;
	const struct nf_conntrack_l4proto *l4proto;
	int ret = 0;

	if (v == SEQ_START_TOKEN) {
		seq_printf(s, "AppID  Mac               Vendor OS Class Type Dev"
						" UpPkt UpByte UpTS DnPkt DnByte DnTS Status"
						" UpTuple DnTuple URL\n");
		return 0;
	}

	hash = v;
	ct = nf_ct_tuplehash_to_ctrack(hash);

	NF_CT_ASSERT(ct);
	if (unlikely(!atomic_inc_not_zero(&ct->ct_general.use)))
		return 0;

	/* we only want to print DIR_ORIGINAL */
	if (NF_CT_DIRECTION(hash))
		goto release;

	ret = -ENOSPC;

	if (ct->dpi.app_id == 0)
	{
		ret = 0;
		goto release;
	}

	l3proto = __nf_ct_l3proto_find(nf_ct_l3num(ct));
	NF_CT_ASSERT(l3proto);
	l4proto = __nf_ct_l4proto_find(nf_ct_l3num(ct), nf_ct_protonum(ct));
	NF_CT_ASSERT(l4proto);

	if (seq_printf(s, "%08x ", ct->dpi.app_id))
		goto release;

	if (ct->dpi.dev_key != DEVINFO_IX_INVALID)
	{
		uint8_t mac[ETH_ALEN];
		DevInfoEntry_t entry;

		devinfo_getmac(ct->dpi.dev_key, mac);
		devinfo_get(ct->dpi.dev_key, &entry);

		if (seq_printf(s, "%02x:%02x:%02x:%02x:%02x:%02x %u %u %u %u %u ",
					mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
					entry.vendor_id, entry.os_id, entry.class_id,entry.type_id,
					entry.dev_id))
			goto release;
	}
	else
	{
		if (seq_printf(s, "NoMac "))
			goto release;
	}

	if (!IS_CTK_INIT_FROM_WAN(ct))
	{
		if (seq_print_acct_dpi(s, ct, IP_CT_DIR_ORIGINAL))
			goto release;

		if (!(test_bit(IPS_SEEN_REPLY_BIT, &ct->status)))
			if (seq_printf(s, "[UNREPLIED] "))
				goto release;

		if (seq_print_acct_dpi(s, ct, IP_CT_DIR_REPLY))
			goto release;
	}
	else
	{
		if (!(test_bit(IPS_SEEN_REPLY_BIT, &ct->status)))
			if (seq_printf(s, "[UNREPLIED] "))
				goto release;

		if (seq_print_acct_dpi(s, ct, IP_CT_DIR_REPLY))
			goto release;

		if (seq_print_acct_dpi(s, ct, IP_CT_DIR_ORIGINAL))
			goto release;
	}

	if (seq_printf(s, "%x ", ct->dpi.flags))
		goto release;

	if (print_tuple(s, &ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple,
			l3proto, l4proto))
		goto release;

	if (!(test_bit(IPS_SEEN_REPLY_BIT, &ct->status)))
		if (seq_printf(s, "[UNREPLIED] "))
			goto release;

	if (print_tuple(s, &ct->tuplehash[IP_CT_DIR_REPLY].tuple,
			l3proto, l4proto))
		goto release;

	if (ct->dpi.url_id != URLINFO_IX_INVALID)
	{
		UrlInfoEntry_t entry;

		urlinfo_get(ct->dpi.url_id, &entry);

		if (seq_printf(s, "%s ", entry.host))
			goto release;
	}

	if (seq_printf(s, "\n"))
		goto release;

	ret = 0;
release:
	nf_ct_put(ct);
	return ret;
}

static const struct seq_operations ct_dpi_seq_ops = {
	.start = ct_dpi_seq_start,
	.next  = ct_dpi_seq_next,
	.stop  = ct_dpi_seq_stop,
	.show  = ct_dpi_seq_show
};

static int ct_dpi_open(struct inode *inode, struct file *file)
{
	return seq_open_net(inode, file, &ct_dpi_seq_ops,
			sizeof(struct ct_iter_state));
}

static const struct file_operations ct_dpi_file_ops = {
	.owner   = THIS_MODULE,
	.open    = ct_dpi_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release_net,
};

static void *dpi_seq_start(struct seq_file *seq, loff_t *pos)
	__acquires(RCU)
{
	struct ct_iter_state *st = seq->private;

	rcu_read_lock();
	if (*pos == 0)
		return SEQ_START_TOKEN;

	st->time_now = ktime_to_ns(ktime_get_real());
	return ct_get_idx(seq, *pos);
}

static void *dpi_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
    if (v == SEQ_START_TOKEN)
    {
        return ct_get_idx(s, *pos);
    }

	(*pos)++;
	return ct_get_next(s, v);
}

static void dpi_seq_stop(struct seq_file *s, void *v)
	__releases(RCU)
{
	dpistats_show(s);
	rcu_read_unlock();
}

/* return 0 on success, 1 in case of error */
static int dpi_seq_show(struct seq_file *s, void *v)
{
	struct nf_conntrack_tuple_hash *hash;
	struct nf_conn *ct;
	int ret = 0;
	DpiStatsEntry_t stats;

	if (v == SEQ_START_TOKEN) {
		seq_printf(s, "AppID  Mac               Vendor OS Class Type Dev"
						" UpPkt UpByte DnPkt DnByte\n");
		dpistats_info(0, NULL); //inform DpiStats module to reset
		return ret;
	}

	hash = v;
	ct = nf_ct_tuplehash_to_ctrack(hash);

	NF_CT_ASSERT(ct);
	if (unlikely(!atomic_inc_not_zero(&ct->ct_general.use)))
		return 0;

	/* we only want to print DIR_ORIGINAL */
	if (NF_CT_DIRECTION(hash))
		goto release;

#if 0
    if (ct->stats_idx == DPISTATS_IX_INVALID)
    {
        if (ct->dpi.app_id == 0) goto release;

        ct->stats_idx = dpistats_lookup(&ct->dpi);

        if (ct->stats_idx == DPISTATS_IX_INVALID)
        {
            printk("fail to alloc dpistats_id?\n");
            goto release;
        }
    }
#endif
    if (ct->dpi.app_id == 0) goto release;

    ct->stats_idx = dpistats_lookup(&ct->dpi);
    if (ct->stats_idx == DPISTATS_IX_INVALID)
    {
        printk("fail to alloc dpistats_id?\n");
        goto release;
    }

	stats.result.app_id = ct->dpi.app_id;
	stats.result.dev_key = ct->dpi.dev_key;
	stats.result.flags = ct->dpi.flags;

	/* origin direction is upstream */
	if (!IS_CTK_INIT_FROM_WAN(ct))
	{
		if (conntrack_get_stats(ct, IP_CT_DIR_ORIGINAL, &stats.upstream))
        {
            printk("1conntrack_get_stats(upstream) fails");
			goto release;
        }

		if ((test_bit(IPS_SEEN_REPLY_BIT, &ct->status)))
        {
			if (conntrack_get_stats(ct, IP_CT_DIR_REPLY, &stats.dnstream))
            {
                printk("1conntrack_get_stats(dnstream) fails");
				goto release;
            }
        }
        else
	        memset(&stats.dnstream, 0 , sizeof(CtkStats_t));
	}
	else /* origin direction is dnstream */
	{
		if (conntrack_get_stats(ct, IP_CT_DIR_ORIGINAL, &stats.dnstream))
        {
            printk("2conntrack_get_stats(dnstream) fails");
			goto release;
        }

		if ((test_bit(IPS_SEEN_REPLY_BIT, &ct->status)))
        {
			if (conntrack_get_stats(ct, IP_CT_DIR_REPLY, &stats.upstream))
            {
                printk("2conntrack_get_stats(upstream) fails");
				goto release;
            }
        }
        else
        	memset(&stats.upstream, 0 , sizeof(CtkStats_t));
	}

	dpistats_info(ct->stats_idx, &stats);

release:
	nf_ct_put(ct);
	return ret;
}

static const struct seq_operations dpi_seq_ops = {
	.start = dpi_seq_start,
	.next  = dpi_seq_next,
	.stop  = dpi_seq_stop,
	.show  = dpi_seq_show
};

static int dpi_open(struct inode *inode, struct file *file)
{
	return seq_open_net(inode, file, &dpi_seq_ops,
			sizeof(struct ct_iter_state));
}

static const struct file_operations dpi_file_ops = {
	.owner   = THIS_MODULE,
	.open    = dpi_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release_net,
};
#endif

static void *ct_cpu_seq_start(struct seq_file *seq, loff_t *pos)
{
	struct net *net = seq_file_net(seq);
	int cpu;

	if (*pos == 0)
		return SEQ_START_TOKEN;

	for (cpu = *pos-1; cpu < nr_cpu_ids; ++cpu) {
		if (!cpu_possible(cpu))
			continue;
		*pos = cpu + 1;
		return per_cpu_ptr(net->ct.stat, cpu);
	}

	return NULL;
}

static void *ct_cpu_seq_next(struct seq_file *seq, void *v, loff_t *pos)
{
	struct net *net = seq_file_net(seq);
	int cpu;

	for (cpu = *pos; cpu < nr_cpu_ids; ++cpu) {
		if (!cpu_possible(cpu))
			continue;
		*pos = cpu + 1;
		return per_cpu_ptr(net->ct.stat, cpu);
	}

	return NULL;
}

static void ct_cpu_seq_stop(struct seq_file *seq, void *v)
{
}

static int ct_cpu_seq_show(struct seq_file *seq, void *v)
{
	struct net *net = seq_file_net(seq);
	unsigned int nr_conntracks = atomic_read(&net->ct.count);
	const struct ip_conntrack_stat *st = v;

	if (v == SEQ_START_TOKEN) {
		seq_printf(seq, "entries  searched found new invalid ignore delete delete_list insert insert_failed drop early_drop icmp_error  expect_new expect_create expect_delete search_restart\n");
		return 0;
	}

	seq_printf(seq, "%08x  %08x %08x %08x %08x %08x %08x %08x "
			"%08x %08x %08x %08x %08x  %08x %08x %08x %08x\n",
		   nr_conntracks,
		   st->searched,
		   st->found,
		   st->new,
		   st->invalid,
		   st->ignore,
		   st->delete,
		   st->delete_list,
		   st->insert,
		   st->insert_failed,
		   st->drop,
		   st->early_drop,
		   st->error,

		   st->expect_new,
		   st->expect_create,
		   st->expect_delete,
		   st->search_restart
		);
	return 0;
}

static const struct seq_operations ct_cpu_seq_ops = {
	.start	= ct_cpu_seq_start,
	.next	= ct_cpu_seq_next,
	.stop	= ct_cpu_seq_stop,
	.show	= ct_cpu_seq_show,
};

static int ct_cpu_seq_open(struct inode *inode, struct file *file)
{
	return seq_open_net(inode, file, &ct_cpu_seq_ops,
			    sizeof(struct seq_net_private));
}

static const struct file_operations ct_cpu_seq_fops = {
	.owner	 = THIS_MODULE,
	.open	 = ct_cpu_seq_open,
	.read	 = seq_read,
	.llseek	 = seq_lseek,
	.release = seq_release_net,
};

static int nf_conntrack_standalone_init_proc(struct net *net)
{
	struct proc_dir_entry *pde;

	pde = proc_net_fops_create(net, "nf_conntrack", 0440, &ct_file_ops);
	if (!pde)
		goto out_nf_conntrack;

	pde = proc_create("nf_conntrack", S_IRUGO, net->proc_net_stat,
			  &ct_cpu_seq_fops);
	if (!pde)
		goto out_stat_nf_conntrack;
#if defined(CONFIG_BCM_KF_DPI) && defined(CONFIG_BRCM_DPI)
	pde = proc_net_fops_create(net, "conntrack_dpi", 0440, &ct_dpi_file_ops);
	if (!pde)
		goto out_conntrack_dpi;
	pde = proc_net_fops_create(net, "dpi_stat", 0440, &dpi_file_ops);
	if (!pde)
		goto out_dpi_stat;
#endif

	return 0;

#if defined(CONFIG_BCM_KF_DPI) && defined(CONFIG_BRCM_DPI)
out_dpi_stat:
	proc_net_remove(net, "conntrack_dpi");
out_conntrack_dpi:
	remove_proc_entry("nf_conntrack", net->proc_net_stat);
#endif
out_stat_nf_conntrack:
	proc_net_remove(net, "nf_conntrack");
out_nf_conntrack:
	return -ENOMEM;
}

static void nf_conntrack_standalone_fini_proc(struct net *net)
{
#if defined(CONFIG_BCM_KF_DPI) && defined(CONFIG_BRCM_DPI)
	proc_net_remove(net, "conntrack_dpi");
	proc_net_remove(net, "dpi_stat");
#endif
	remove_proc_entry("nf_conntrack", net->proc_net_stat);
	proc_net_remove(net, "nf_conntrack");
}
#else
static int nf_conntrack_standalone_init_proc(struct net *net)
{
	return 0;
}

static void nf_conntrack_standalone_fini_proc(struct net *net)
{
}
#endif /* CONFIG_NF_CONNTRACK_PROCFS */

/* Sysctl support */

#ifdef CONFIG_SYSCTL
/* Log invalid packets of a given protocol */
static int log_invalid_proto_min = 0;
static int log_invalid_proto_max = 255;

static struct ctl_table_header *nf_ct_netfilter_header;

static ctl_table nf_ct_sysctl_table[] = {
	{
		.procname	= "nf_conntrack_max",
		.data		= &nf_conntrack_max,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec,
	},
	{
		.procname	= "nf_conntrack_count",
		.data		= &init_net.ct.count,
		.maxlen		= sizeof(int),
		.mode		= 0444,
		.proc_handler	= proc_dointvec,
	},
	{
		.procname       = "nf_conntrack_buckets",
		.data           = &init_net.ct.htable_size,
		.maxlen         = sizeof(unsigned int),
		.mode           = 0444,
		.proc_handler   = proc_dointvec,
	},
	{
		.procname	= "nf_conntrack_checksum",
		.data		= &init_net.ct.sysctl_checksum,
		.maxlen		= sizeof(unsigned int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec,
	},
	{
		.procname	= "nf_conntrack_log_invalid",
		.data		= &init_net.ct.sysctl_log_invalid,
		.maxlen		= sizeof(unsigned int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec_minmax,
		.extra1		= &log_invalid_proto_min,
		.extra2		= &log_invalid_proto_max,
	},
#if defined(CONFIG_BCM_KF_NETFILTER)
	{
		.procname	= "nf_conntrack_iqloprio",
		.data		= &init_net.ct.iqloprio,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec,
	},
	{
		.procname	= "nf_conntrack_iqhiprio",
		.data		= &init_net.ct.iqhiprio,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec,
	},
#endif
	{
		.procname	= "nf_conntrack_expect_max",
		.data		= &nf_ct_expect_max,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec,
	},
	{ }
};

#define NET_NF_CONNTRACK_MAX 2089

static ctl_table nf_ct_netfilter_table[] = {
	{
		.procname	= "nf_conntrack_max",
		.data		= &nf_conntrack_max,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec,
	},
	{ }
};

static struct ctl_path nf_ct_path[] = {
	{ .procname = "net", },
	{ }
};

static int nf_conntrack_standalone_init_sysctl(struct net *net)
{
	struct ctl_table *table;

	if (net_eq(net, &init_net)) {
		nf_ct_netfilter_header =
		       register_sysctl_paths(nf_ct_path, nf_ct_netfilter_table);
		if (!nf_ct_netfilter_header)
			goto out;
	}

	table = kmemdup(nf_ct_sysctl_table, sizeof(nf_ct_sysctl_table),
			GFP_KERNEL);
	if (!table)
		goto out_kmemdup;

	table[1].data = &net->ct.count;
	table[2].data = &net->ct.htable_size;
	table[3].data = &net->ct.sysctl_checksum;
	table[4].data = &net->ct.sysctl_log_invalid;
#if defined(CONFIG_BCM_KF_NETFILTER)
	table[5].data = &net->ct.iqloprio;
	table[6].data = &net->ct.iqhiprio;
#endif
	net->ct.sysctl_header = register_net_sysctl_table(net,
					nf_net_netfilter_sysctl_path, table);
	if (!net->ct.sysctl_header)
		goto out_unregister_netfilter;

	return 0;

out_unregister_netfilter:
	kfree(table);
out_kmemdup:
	if (net_eq(net, &init_net))
		unregister_sysctl_table(nf_ct_netfilter_header);
out:
	printk(KERN_ERR "nf_conntrack: can't register to sysctl.\n");
	return -ENOMEM;
}

static void nf_conntrack_standalone_fini_sysctl(struct net *net)
{
	struct ctl_table *table;

	if (net_eq(net, &init_net))
		unregister_sysctl_table(nf_ct_netfilter_header);
	table = net->ct.sysctl_header->ctl_table_arg;
	unregister_net_sysctl_table(net->ct.sysctl_header);
	kfree(table);
}
#else
static int nf_conntrack_standalone_init_sysctl(struct net *net)
{
	return 0;
}

static void nf_conntrack_standalone_fini_sysctl(struct net *net)
{
}
#endif /* CONFIG_SYSCTL */

static int nf_conntrack_net_init(struct net *net)
{
	int ret;

	ret = nf_conntrack_init(net);
	if (ret < 0)
		goto out_init;
	ret = nf_conntrack_standalone_init_proc(net);
	if (ret < 0)
		goto out_proc;
	net->ct.sysctl_checksum = 1;
	net->ct.sysctl_log_invalid = 0;
	ret = nf_conntrack_standalone_init_sysctl(net);
	if (ret < 0)
		goto out_sysctl;
	return 0;

out_sysctl:
	nf_conntrack_standalone_fini_proc(net);
out_proc:
	nf_conntrack_cleanup(net);
out_init:
	return ret;
}

static void nf_conntrack_net_exit(struct net *net)
{
	nf_conntrack_standalone_fini_sysctl(net);
	nf_conntrack_standalone_fini_proc(net);
	nf_conntrack_cleanup(net);
}

static struct pernet_operations nf_conntrack_net_ops = {
	.init = nf_conntrack_net_init,
	.exit = nf_conntrack_net_exit,
};

static int __init nf_conntrack_standalone_init(void)
{
	return register_pernet_subsys(&nf_conntrack_net_ops);
}

static void __exit nf_conntrack_standalone_fini(void)
{
	unregister_pernet_subsys(&nf_conntrack_net_ops);
}

module_init(nf_conntrack_standalone_init);
module_exit(nf_conntrack_standalone_fini);

/* Some modules need us, but don't depend directly on any symbol.
   They should call this. */
void need_conntrack(void)
{
}
EXPORT_SYMBOL_GPL(need_conntrack);
