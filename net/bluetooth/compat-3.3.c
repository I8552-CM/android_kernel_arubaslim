/*
 * Copyright 2012  Luis R. Rodriguez <mcgrof@frijolero.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Backport functionality introduced in Linux 3.3.
 */

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/skbuff.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <net/dst.h>
#include <net/xfrm.h>

static void __copy_skb_header(struct sk_buff *new, const struct sk_buff *old)
{
	new->tstamp		= old->tstamp;
	new->dev		= old->dev;
	new->transport_header	= old->transport_header;
	new->network_header	= old->network_header;
	new->mac_header		= old->mac_header;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35))
	skb_dst_copy(new, old);
	new->rxhash		= old->rxhash;
#else
	skb_dst_set(new, dst_clone(skb_dst(old)));
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,1,0))
	new->ooo_okay		= old->ooo_okay;
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0))
	new->l4_rxhash		= old->l4_rxhash;
#endif
#ifdef CONFIG_XFRM
	new->sp			= secpath_get(old->sp);
#endif
	memcpy(new->cb, old->cb, sizeof(old->cb));
	new->csum		= old->csum;
	new->local_df		= old->local_df;
	new->pkt_type		= old->pkt_type;
	new->ip_summed		= old->ip_summed;
	skb_copy_queue_mapping(new, old);
	new->priority		= old->priority;
#if IS_ENABLED(CONFIG_IP_VS)
	new->ipvs_property	= old->ipvs_property;
#endif
	new->protocol		= old->protocol;
	new->mark		= old->mark;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
	new->skb_iif		= old->skb_iif;
#endif
	__nf_copy(new, old);
#if IS_ENABLED(CONFIG_NETFILTER_XT_TARGET_TRACE)
	new->nf_trace		= old->nf_trace;
#endif
#ifdef CONFIG_NET_SCHED
	new->tc_index		= old->tc_index;
#ifdef CONFIG_NET_CLS_ACT
	new->tc_verd		= old->tc_verd;
#endif
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
	new->vlan_tci		= old->vlan_tci;
#endif

	skb_copy_secmark(new, old);
}

static void copy_skb_header(struct sk_buff *new, const struct sk_buff *old)
{
#ifndef NET_SKBUFF_DATA_USES_OFFSET
	/*
	 *	Shift between the two data areas in bytes
	 */
	unsigned long offset = new->data - old->data;
#endif

	__copy_skb_header(new, old);

#ifndef NET_SKBUFF_DATA_USES_OFFSET
	/* {transport,network,mac}_header are relative to skb->head */
	new->transport_header += offset;
	new->network_header   += offset;
	if (skb_mac_header_was_set(new))
		new->mac_header	      += offset;
#endif
	skb_shinfo(new)->gso_size = skb_shinfo(old)->gso_size;
	skb_shinfo(new)->gso_segs = skb_shinfo(old)->gso_segs;
	skb_shinfo(new)->gso_type = skb_shinfo(old)->gso_type;
}

static void skb_clone_fraglist(struct sk_buff *skb)
{
	struct sk_buff *list;

	skb_walk_frags(skb, list)
		skb_get(list);
}

static DEFINE_SPINLOCK(wq_name_lock);
static LIST_HEAD(wq_name_list);

struct wq_name {
	struct list_head list;
	struct workqueue_struct *wq;
	char name[24];
};

struct workqueue_struct *
backport_alloc_workqueue(const char *fmt, unsigned int flags,
			 int max_active, struct lock_class_key *key,
			 const char *lock_name, ...)
{
	struct workqueue_struct *wq;
	struct wq_name *n = kzalloc(sizeof(*n), GFP_KERNEL);
	va_list args;

	if (!n)
		return NULL;

	va_start(args, lock_name);
	vsnprintf(n->name, sizeof(n->name), fmt, args);
	va_end(args);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
	wq = __create_workqueue_key(n->name, max_active == 1, 0,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
				    0,
#endif
				    key, lock_name);
#else
	wq = __alloc_workqueue_key(n->name, flags, max_active, key, lock_name);
#endif
	if (!wq) {
		kfree(n);
		return NULL;
	}

	n->wq = wq;
	spin_lock(&wq_name_lock);
	list_add(&n->list, &wq_name_list);
	spin_unlock(&wq_name_lock);

	return wq;
}
EXPORT_SYMBOL_GPL(backport_alloc_workqueue);
