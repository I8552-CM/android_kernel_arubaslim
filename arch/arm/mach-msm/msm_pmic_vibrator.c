/* include/asm/mach-msm/htc_pwrsink.h
 *
 * Copyright (C) 2008 HTC Corporation.
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2011 Code Aurora Forum. All rights reserved.
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
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/hrtimer.h>
#include <linux/clk.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/delay.h>

#include <mach/samsung_vibe.h>
#include <mach/gpio.h>
#include <mach/vreg.h>

#include "pmic.h"
#include "timed_output.h"
#include <linux/regulator/consumer.h>

static struct work_struct work_vibrator_on;
static struct work_struct work_vibrator_off;

static struct hrtimer vibe_timer;

static int msm_vibrator_suspend(struct platform_device *pdev, pm_message_t state);
static int msm_vibrator_resume(struct platform_device *pdev);
static int msm_vibrator_probe(struct platform_device *pdev);
static int msm_vibrator_exit(struct platform_device *pdev);

static void board_vibrator_ctrl(int on);

/* for the suspend/resume VIBRATOR Module */
static struct platform_driver msm_vibrator_platdrv = 
{
	.probe   = msm_vibrator_probe,
	.suspend = msm_vibrator_suspend,
	.resume  = msm_vibrator_resume,
	.remove  = msm_vibrator_exit,
	.driver = 
	{
		.name = MODULE_NAME,
		.owner = THIS_MODULE,
	},
};

static int msm_vibrator_suspend(struct platform_device *pdev, pm_message_t state)
{
	printk("[VIB] Susepend\n");

	board_vibrator_ctrl(0);

	return VIBE_S_SUCCESS;
}

static int msm_vibrator_resume(struct platform_device *pdev)
{
	printk("[VIB] Resume\n");

	return VIBE_S_SUCCESS;
}

static int msm_vibrator_exit(struct platform_device *pdev)
{
	printk("[VIB] Exit\n");

	return VIBE_S_SUCCESS;
}

static void board_vibrator_ctrl(int on)
{
	int ret = 0;
	static int nStatus = 0;
	unsigned int vib_voltage = 0;

	printk("%s on = %d nStatus = %d\n", __func__, on, nStatus);

	if(nStatus == on)	{
		printk("%s set already!\n", __func__);
		return VIBE_E_FAIL;
	}
	else {
		nStatus = on;
	}

	vib_voltage = 3100;

	if(on)	{
		ret = pmic_vib_mot_set_volt(vib_voltage);
	}
	else {
		ret = pmic_vib_mot_set_volt(0);
	}

	printk("%s status = %d\n", __func__, ret);

	return VIBE_S_SUCCESS;
}

static void msm_vibrator_on(struct work_struct *work)
{
	board_vibrator_ctrl(1);
}

static void msm_vibrator_off(struct work_struct *work)
{
	board_vibrator_ctrl(0);
}

static void timed_vibrator_on(struct timed_output_dev *sdev)
{
	printk("[VIB] %s\n",__func__);

	schedule_work(&work_vibrator_on);
}

static void timed_vibrator_off(struct timed_output_dev *sdev)
{
	printk("[VIB] %s\n",__func__);

	schedule_work(&work_vibrator_off);
}

static void vibrator_enable(struct timed_output_dev *dev, int value)
{
	hrtimer_cancel(&vibe_timer);

	if (value == 0) {
		printk("[VIB] OFF\n");

		timed_vibrator_off(dev);
	}
	else {
		printk("[VIB] ON\n");

		printk("[VIB] Duration : %d sec\n" , value/1000);

		timed_vibrator_on(dev);

		if (value == 0x7fffffff){
			printk("[VIB} No Use Timer %d \n", value);				
		}
		else	{
			value = (value > 15000 ? 15000 : value);
		        hrtimer_start(&vibe_timer, ktime_set(value / 1000, (value % 1000) * 1000000), HRTIMER_MODE_REL);
                }
	}
}

static int vibrator_get_time(struct timed_output_dev *dev)
{
	if (hrtimer_active(&vibe_timer)) {
		ktime_t r = hrtimer_get_remaining(&vibe_timer);
		struct timeval t = ktime_to_timeval(r);

		return (t.tv_sec * 1000 + t.tv_usec / 1000);
	}

	return 0;
}

static enum hrtimer_restart vibrator_timer_func(struct hrtimer *timer)
{
	printk("[VIB] %s\n",__func__);

	timed_vibrator_off(NULL);

	return HRTIMER_NORESTART;
}

static struct timed_output_dev pmic_vibrator = {
	.name = "vibrator",
	.get_time = vibrator_get_time,
	.enable = vibrator_enable,
};

static int msm_vibrator_probe(struct platform_device *pdev)
{
	int rc;

	printk("[VIB] Prob\n");

	INIT_WORK(&work_vibrator_on, msm_vibrator_on);
	INIT_WORK(&work_vibrator_off, msm_vibrator_off);

	hrtimer_init(&vibe_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	vibe_timer.function = vibrator_timer_func;

	rc = timed_output_dev_register(&pmic_vibrator);
	if (rc < 0) {
		goto err_read_vib;
	}

	return 0;

err_read_vib:
	printk(KERN_ERR "[VIB] timed_output_dev_register fail (rc=%d)\n", rc);
	return rc;
}

static int __init msm_init_pmic_vibrator(void)
{
	int rc;

	rc = platform_driver_register(&msm_vibrator_platdrv);

	printk("[VIB] platform driver register result : %d\n",rc);
	if (rc)	{ 
		printk("[VIB] platform_driver_register failed\n");
	}

	return rc;

}

void __exit msm_exit_pmic_vibrator(void)
{
	printk("[VIB] Exit\n");

	platform_driver_unregister(&msm_vibrator_platdrv);
}

module_init(msm_init_pmic_vibrator);
module_exit(msm_exit_pmic_vibrator);


MODULE_DESCRIPTION("timed output pmic vibrator device");
MODULE_LICENSE("GPL");

