/* kernel/power/earlysuspend.c
 *
 * Copyright (C) 2005-2008 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/earlysuspend.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/rtc.h>
#include <linux/wakelock.h>
#include <linux/workqueue.h>

#include "power.h"

enum {
	DEBUG_USER_STATE = 1U << 0,
	DEBUG_SUSPEND = 1U << 2,
	DEBUG_VERBOSE = 1U << 3,
};
static int debug_mask = DEBUG_USER_STATE | DEBUG_VERBOSE;
#if defined(CONFIG_MACH_KYLEPLUS_CTC)
#define EARLY_SUSPEND_LIST_DEBUG
#endif

#ifdef EARLY_SUSPEND_LIST_DEBUG
#define EALAY_SUS_MAX_FUN_NAME 40
#define EARLY_SUS_LIST 460
static char early_suspend_list_debug[EARLY_SUS_LIST + EALAY_SUS_MAX_FUN_NAME];
static char late_resume_list_debug[EARLY_SUS_LIST + EALAY_SUS_MAX_FUN_NAME];
static char * early_suspend_list_p	= &early_suspend_list_debug;
static char * late_resume_list_p 	= &late_resume_list_debug;
#endif

module_param_named(debug_mask, debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);

static DEFINE_MUTEX(early_suspend_lock);
static LIST_HEAD(early_suspend_handlers);
static void early_suspend(struct work_struct *work);
static void late_resume(struct work_struct *work);
static DECLARE_WORK(early_suspend_work, early_suspend);
static DECLARE_WORK(late_resume_work, late_resume);
static DEFINE_SPINLOCK(state_lock);
enum {
	SUSPEND_REQUESTED = 0x1,
	SUSPENDED = 0x2,
	SUSPEND_REQUESTED_AND_SUSPENDED = SUSPEND_REQUESTED | SUSPENDED,
};
static int state;

void register_early_suspend(struct early_suspend *handler)
{
	struct list_head *pos;

	mutex_lock(&early_suspend_lock);
	list_for_each(pos, &early_suspend_handlers) {
		struct early_suspend *e;
		e = list_entry(pos, struct early_suspend, link);
		if (e->level > handler->level)
			break;
	}
	list_add_tail(&handler->link, pos);
	if ((state & SUSPENDED) && handler->suspend)
		handler->suspend(handler);
	mutex_unlock(&early_suspend_lock);
}
EXPORT_SYMBOL(register_early_suspend);

void unregister_early_suspend(struct early_suspend *handler)
{
	mutex_lock(&early_suspend_lock);
	list_del(&handler->link);
	mutex_unlock(&early_suspend_lock);
}
EXPORT_SYMBOL(unregister_early_suspend);

static void early_suspend(struct work_struct *work)
{
	struct early_suspend *pos;
	unsigned long irqflags;
	int abort = 0;
#ifdef EARLY_SUSPEND_LIST_DEBUG
	uint offset_sum = 0;
	uint offset =0;
#endif

	mutex_lock(&early_suspend_lock);
	spin_lock_irqsave(&state_lock, irqflags);
	if (state == SUSPEND_REQUESTED)
		state |= SUSPENDED;
	else
		abort = 1;
	spin_unlock_irqrestore(&state_lock, irqflags);

	if (abort) {
		if (debug_mask & DEBUG_SUSPEND)
			pr_info("early_suspend: abort, state %d\n", state);
		mutex_unlock(&early_suspend_lock);
		goto abort;
	}

	if (debug_mask & DEBUG_SUSPEND)
		pr_info("early_suspend: call handlers\n");

#ifdef EARLY_SUSPEND_LIST_DEBUG
	memset(early_suspend_list_debug, 0, EARLY_SUS_LIST + EALAY_SUS_MAX_FUN_NAME);
#endif
	
	list_for_each_entry(pos, &early_suspend_handlers, link) {
		if (pos->suspend != NULL) {
			if (debug_mask & DEBUG_VERBOSE)
				pr_info("early_suspend: calling %pf\n", pos->suspend);
#ifdef EARLY_SUSPEND_LIST_DEBUG
			if (offset_sum < EARLY_SUS_LIST) {
				offset = snprintf(early_suspend_list_p + offset_sum, EALAY_SUS_MAX_FUN_NAME, "+%pf", pos->suspend);
				offset_sum += (offset > 0)? min(offset, EALAY_SUS_MAX_FUN_NAME-1) : 0;
			}
#endif			
			pos->suspend(pos);
#ifdef EARLY_SUSPEND_LIST_DEBUG
			if (offset_sum < EARLY_SUS_LIST) {
				offset = snprintf(early_suspend_list_p + offset_sum, EALAY_SUS_MAX_FUN_NAME, "-; ");
				offset_sum += (offset > 0) ? offset : 0;
			}
#endif
		}
	}
	mutex_unlock(&early_suspend_lock);

	suspend_sys_sync_queue();
abort:
	spin_lock_irqsave(&state_lock, irqflags);
	if (state == SUSPEND_REQUESTED_AND_SUSPENDED)
		wake_unlock(&main_wake_lock);
	spin_unlock_irqrestore(&state_lock, irqflags);
}

static void late_resume(struct work_struct *work)
{
	struct early_suspend *pos;
	unsigned long irqflags;
	int abort = 0;
#ifdef EARLY_SUSPEND_LIST_DEBUG
	uint offset_sum = 0;
	uint offset =0;
#endif

	mutex_lock(&early_suspend_lock);
	spin_lock_irqsave(&state_lock, irqflags);
	if (state == SUSPENDED)
		state &= ~SUSPENDED;
	else
		abort = 1;
	spin_unlock_irqrestore(&state_lock, irqflags);

	if (abort) {
		if (debug_mask & DEBUG_SUSPEND)
			pr_info("late_resume: abort, state %d\n", state);
		goto abort;
	}
	if (debug_mask & DEBUG_SUSPEND)
		pr_info("late_resume: call handlers\n");

#ifdef EARLY_SUSPEND_LIST_DEBUG
	memset(late_resume_list_debug, 0, EARLY_SUS_LIST + EALAY_SUS_MAX_FUN_NAME);
#endif

	list_for_each_entry_reverse(pos, &early_suspend_handlers, link) {
		if (pos->resume != NULL) {
			if (debug_mask & DEBUG_VERBOSE)
				pr_info("late_resume: calling %pf\n", pos->resume);
#ifdef EARLY_SUSPEND_LIST_DEBUG
			if (offset_sum < EARLY_SUS_LIST) {
				offset = snprintf(late_resume_list_p + offset_sum, EALAY_SUS_MAX_FUN_NAME, "+%pf", pos->resume);
				offset_sum += (offset > 0)? min(offset, EALAY_SUS_MAX_FUN_NAME-1) : 0;
			}
#endif
			pos->resume(pos);
#ifdef EARLY_SUSPEND_LIST_DEBUG
			if (offset_sum < EARLY_SUS_LIST) {
				offset = snprintf(late_resume_list_p + offset_sum, EALAY_SUS_MAX_FUN_NAME, "-; ");
				offset_sum += (offset > 0) ? offset : 0;
			}
#endif
		}
	}
	if (debug_mask & DEBUG_SUSPEND)
		pr_info("late_resume: done\n");
abort:
	mutex_unlock(&early_suspend_lock);
}

void request_suspend_state(suspend_state_t new_state)
{
	unsigned long irqflags;
	int old_sleep;

	spin_lock_irqsave(&state_lock, irqflags);
	old_sleep = state & SUSPEND_REQUESTED;
	if (debug_mask & DEBUG_USER_STATE) {
		struct timespec ts;
		struct rtc_time tm;
		getnstimeofday(&ts);
		rtc_time_to_tm(ts.tv_sec, &tm);
		pr_info("request_suspend_state: %s (%d->%d) at %lld "
			"(%d-%02d-%02d %02d:%02d:%02d.%09lu UTC)\n",
			new_state != PM_SUSPEND_ON ? "sleep" : "wakeup",
			requested_suspend_state, new_state,
			ktime_to_ns(ktime_get()),
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec);
	}
#ifdef EARLY_SUSPEND_LIST_DEBUG
	if (new_state != PM_SUSPEND_ON)
		pr_info("the_last_resume_list: %s\n", late_resume_list_p);
	else
		pr_info("the_last_suspend_list: %s\n", early_suspend_list_p);
#endif
	if (!old_sleep && new_state != PM_SUSPEND_ON) {
		state |= SUSPEND_REQUESTED;
		queue_work(suspend_work_queue, &early_suspend_work);
	} else if (old_sleep && new_state == PM_SUSPEND_ON) {
		state &= ~SUSPEND_REQUESTED;
		wake_lock(&main_wake_lock);
		queue_work(suspend_work_queue, &late_resume_work);
	}
	requested_suspend_state = new_state;
	spin_unlock_irqrestore(&state_lock, irqflags);
}

suspend_state_t get_suspend_state(void)
{
	return requested_suspend_state;
}
