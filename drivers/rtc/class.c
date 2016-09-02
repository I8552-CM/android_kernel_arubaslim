/*
 * RTC subsystem, base class
 *
 * Copyright (C) 2005 Tower Technologies
 * Author: Alessandro Zummo <a.zummo@towertech.it>
 *
 * class skeleton from drivers/hwmon/hwmon.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/rtc.h>
#include <linux/kdev_t.h>
#include <linux/idr.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#include "rtc-core.h"

#if defined(CONFIG_MACH_KYLEPLUS_OPEN) || defined(CONFIG_MACH_ARUBA_OPEN) || defined(CONFIG_MACH_NEVIS3G) || defined(CONFIG_MACH_ARUBASLIM_OPEN)
#define ADJUST_KERNEL_TIME
#endif

#ifdef ADJUST_KERNEL_TIME
#define RTC_WORK_CHECK_TIMEOUT (1 * 60 * HZ)
static int rtc_work_check();
static DECLARE_DELAYED_WORK(rtc_work, rtc_work_check);
#endif

static DEFINE_IDA(rtc_ida);
struct class *rtc_class;

static void rtc_device_release(struct device *dev)
{
	struct rtc_device *rtc = to_rtc_device(dev);
	ida_simple_remove(&rtc_ida, rtc->id);
	kfree(rtc);
}

#if defined(CONFIG_PM) && defined(CONFIG_RTC_HCTOSYS_DEVICE)

/*
 * On suspend(), measure the delta between one RTC and the
 * system's wall clock; restore it on resume().
 */

static struct timespec old_rtc, old_system, old_delta;


static int rtc_suspend(struct device *dev, pm_message_t mesg)
{
	struct rtc_device	*rtc = to_rtc_device(dev);
	struct rtc_time		tm;
	struct timespec		delta, delta_delta;
	if (strcmp(dev_name(&rtc->dev), CONFIG_RTC_HCTOSYS_DEVICE) != 0)
		return 0;

	/* snapshot the current RTC and system time at suspend*/
	rtc_read_time(rtc, &tm);
	getnstimeofday(&old_system);
	rtc_tm_to_time(&tm, &old_rtc.tv_sec);


	/*
	 * To avoid drift caused by repeated suspend/resumes,
	 * which each can add ~1 second drift error,
	 * try to compensate so the difference in system time
	 * and rtc time stays close to constant.
	 */
	delta = timespec_sub(old_system, old_rtc);
	delta_delta = timespec_sub(delta, old_delta);
	if (delta_delta.tv_sec < -2 || delta_delta.tv_sec >= 2) {
		/*
		 * if delta_delta is too large, assume time correction
		 * has occured and set old_delta to the current delta.
		 */
		old_delta = delta;
	} else {
		/* Otherwise try to adjust old_system to compensate */
		old_system = timespec_sub(old_system, delta_delta);
	}

	return 0;
}

static int rtc_resume(struct device *dev)
{
	struct rtc_device	*rtc = to_rtc_device(dev);
	struct rtc_time		tm;
	struct timespec		new_system, new_rtc;
	struct timespec		sleep_time;

	if (strcmp(dev_name(&rtc->dev), CONFIG_RTC_HCTOSYS_DEVICE) != 0)
		return 0;

#if defined(ADJUST_KERNEL_TIME)
	printk("%s [RTC] ==================================== \n", __func__);
#endif

	/* snapshot the current rtc and system time at resume */
	getnstimeofday(&new_system);
	rtc_read_time(rtc, &tm);
	if (rtc_valid_tm(&tm) != 0) {
		pr_debug("%s:  bogus resume time\n", dev_name(&rtc->dev));
		return 0;
	}
	rtc_tm_to_time(&tm, &new_rtc.tv_sec);
	new_rtc.tv_nsec = 0;

	if (new_rtc.tv_sec < old_rtc.tv_sec) {
		pr_debug("%s:  time travel!\n", dev_name(&rtc->dev));
		return 0;
	}

	/* calculate the RTC time delta (sleep time)*/
	sleep_time = timespec_sub(new_rtc, old_rtc);

	/*
	 * Since these RTC suspend/resume handlers are not called
	 * at the very end of suspend or the start of resume,
	 * some run-time may pass on either sides of the sleep time
	 * so subtract kernel run-time between rtc_suspend to rtc_resume
	 * to keep things accurate.
	 */
	sleep_time = timespec_sub(sleep_time,
			timespec_sub(new_system, old_system));

	if (sleep_time.tv_sec >= 0)
		timekeeping_inject_sleeptime(&sleep_time);

#ifdef ADJUST_KERNEL_TIME
	schedule_delayed_work(&rtc_work, RTC_WORK_CHECK_TIMEOUT);
#endif

#if defined(ADJUST_KERNEL_TIME)
	printk("%s [RTC] sleep_time = %ld \n", __func__,sleep_time.tv_sec);
	printk("%s [RTC] time now = %ld\n", __func__,new_rtc.tv_sec);
	printk("%s [RTC] ==================================== \n", __func__);
#endif

	return 0;
}

#ifdef ADJUST_KERNEL_TIME
static int rtc_work_check()
{
	struct device *dev;
	struct rtc_device *rtc = NULL;
	struct timespec		ktime, adjust_ktime;
	struct timespec		check_delta;
	struct rtc_time		tm;
	struct timespec		rtime;

	printk(KERN_ERR "%s\n", __func__);

	rtc = rtc_class_open(CONFIG_RTC_HCTOSYS_DEVICE);

	if (rtc!=NULL){
		getnstimeofday(&ktime);					// kernel_time
		rtc_read_time(rtc, &tm);
		rtc_tm_to_time(&tm, &rtime.tv_sec);		// rtc_time
		rtc_tm_to_time(&tm, &adjust_ktime.tv_sec);
		
		/* RTC precision is 1 second; adjust delta for avg 1/2 sec err */
		set_normalized_timespec(&check_delta, ktime.tv_sec - rtime.tv_sec, ktime.tv_nsec - (NSEC_PER_SEC >> 1));



		if(abs(check_delta.tv_sec) >= 3)
		{
			printk(KERN_ERR "%s: 1 k_t: %10d\n", __func__, ktime.tv_sec);
			printk(KERN_ERR "%s: 2 r_t: %10d\n", __func__, rtime.tv_sec);
			printk(KERN_ERR "%s: 3 check_delta   s: %3d, s: %10d \n", __func__, check_delta.tv_sec, check_delta.tv_nsec);
			printk(KERN_ERR "%s: 4 old_delta   s: %3d, s: %10d \n", __func__, old_delta.tv_sec, old_delta.tv_nsec);
			adjust_ktime.tv_nsec = 0;
			do_settimeofday(&adjust_ktime);

			rtc_read_time(rtc, &tm);		//request by QC
			
			getnstimeofday(&adjust_ktime);
			printk(KERN_ERR "%s: 5 adjust k time is set as: %10d\n", __func__, adjust_ktime.tv_sec);

			set_normalized_timespec(&old_delta, adjust_ktime.tv_sec - rtime.tv_sec, adjust_ktime.tv_nsec - (NSEC_PER_SEC >> 1));
			printk(KERN_ERR "%s: 5 re-set delta,  s:%3d, n:%10d\n", __func__, old_delta.tv_sec,old_delta.tv_nsec);
		}
	}else{
		printk(KERN_ERR "%s: rtc_dev isn't rtc0, \n", __func__);

	}
	schedule_delayed_work(&rtc_work, RTC_WORK_CHECK_TIMEOUT);
	return 0;
}
#endif

#else
#define rtc_suspend	NULL
#define rtc_resume	NULL
#define rtc_work_check NULL
#endif


/**
 * rtc_device_register - register w/ RTC class
 * @dev: the device to register
 *
 * rtc_device_unregister() must be called when the class device is no
 * longer needed.
 *
 * Returns the pointer to the new struct class device.
 */
struct rtc_device *rtc_device_register(const char *name, struct device *dev,
					const struct rtc_class_ops *ops,
					struct module *owner)
{
	struct rtc_device *rtc;
	struct rtc_wkalrm alrm;
	int id, err;

	id = ida_simple_get(&rtc_ida, 0, 0, GFP_KERNEL);
	if (id < 0) {
		err = id;
		goto exit;
	}

	rtc = kzalloc(sizeof(struct rtc_device), GFP_KERNEL);
	if (rtc == NULL) {
		err = -ENOMEM;
		goto exit_ida;
	}

	rtc->id = id;
	rtc->ops = ops;
	rtc->owner = owner;
	rtc->irq_freq = 1;
	rtc->max_user_freq = 64;
	rtc->dev.parent = dev;
	rtc->dev.class = rtc_class;
	rtc->dev.release = rtc_device_release;

	mutex_init(&rtc->ops_lock);
	spin_lock_init(&rtc->irq_lock);
	spin_lock_init(&rtc->irq_task_lock);
	init_waitqueue_head(&rtc->irq_queue);

	/* Init timerqueue */
	timerqueue_init_head(&rtc->timerqueue);
	INIT_WORK(&rtc->irqwork, rtc_timer_do_work);
	/* Init aie timer */
	rtc_timer_init(&rtc->aie_timer, rtc_aie_update_irq, (void *)rtc);
	/* Init uie timer */
	rtc_timer_init(&rtc->uie_rtctimer, rtc_uie_update_irq, (void *)rtc);
	/* Init pie timer */
	hrtimer_init(&rtc->pie_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	rtc->pie_timer.function = rtc_pie_update_irq;
	rtc->pie_enabled = 0;

	/* Check to see if there is an ALARM already set in hw */
	err = __rtc_read_alarm(rtc, &alrm);

	if (!err && !rtc_valid_tm(&alrm.time))
		rtc_initialize_alarm(rtc, &alrm);

	strlcpy(rtc->name, name, RTC_DEVICE_NAME_SIZE);
	dev_set_name(&rtc->dev, "rtc%d", id);

	rtc_dev_prepare(rtc);

	err = device_register(&rtc->dev);
	if (err) {
		put_device(&rtc->dev);
		goto exit_kfree;
	}

	rtc_dev_add_device(rtc);
	rtc_sysfs_add_device(rtc);
	rtc_proc_add_device(rtc);

	dev_info(dev, "rtc core: registered %s as %s\n",
			rtc->name, dev_name(&rtc->dev));

	return rtc;

exit_kfree:
	kfree(rtc);

exit_ida:
	ida_simple_remove(&rtc_ida, id);

exit:
	dev_err(dev, "rtc core: unable to register %s, err = %d\n",
			name, err);
	return ERR_PTR(err);
}
EXPORT_SYMBOL_GPL(rtc_device_register);


/**
 * rtc_device_unregister - removes the previously registered RTC class device
 *
 * @rtc: the RTC class device to destroy
 */
void rtc_device_unregister(struct rtc_device *rtc)
{
	if (get_device(&rtc->dev) != NULL) {
		mutex_lock(&rtc->ops_lock);
		/* remove innards of this RTC, then disable it, before
		 * letting any rtc_class_open() users access it again
		 */
		rtc_sysfs_del_device(rtc);
		rtc_dev_del_device(rtc);
		rtc_proc_del_device(rtc);
		device_unregister(&rtc->dev);
		rtc->ops = NULL;
		mutex_unlock(&rtc->ops_lock);
		put_device(&rtc->dev);
	}
}
EXPORT_SYMBOL_GPL(rtc_device_unregister);

static int __init rtc_init(void)
{
	rtc_class = class_create(THIS_MODULE, "rtc");
	if (IS_ERR(rtc_class)) {
		printk(KERN_ERR "%s: couldn't create class\n", __FILE__);
		return PTR_ERR(rtc_class);
	}
	rtc_class->suspend = rtc_suspend;
	rtc_class->resume = rtc_resume;
	rtc_dev_init();
	rtc_sysfs_init(rtc_class);

#ifdef ADJUST_KERNEL_TIME
	schedule_delayed_work(&rtc_work, RTC_WORK_CHECK_TIMEOUT);
#endif
	return 0;
}

static void __exit rtc_exit(void)
{
	rtc_dev_exit();
	class_destroy(rtc_class);
	ida_destroy(&rtc_ida);
}

subsys_initcall(rtc_init);
module_exit(rtc_exit);

MODULE_AUTHOR("Alessandro Zummo <a.zummo@towertech.it>");
MODULE_DESCRIPTION("RTC class support");
MODULE_LICENSE("GPL");
