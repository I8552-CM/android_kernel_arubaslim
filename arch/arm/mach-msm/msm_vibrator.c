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

#if defined(CONFIG_MACH_ARUBA_OPEN) || defined(CONFIG_MACH_ARUBA_CTC) || defined(CONFIG_MACH_KYLEPLUS_CTC) || defined(CONFIG_MACH_ARUBA_DUOS_CTC) || defined(CONFIG_MACH_DELOS_CTC) 
#if defined(CONFIG_GPIO_CONTROL_VIBRATOR)
#define MOTOR_EN_GPIO 111
#else
#define MOTOR_EN_GPIO 6
#endif
#elif defined(CONFIG_MACH_ARUBASLIM_OPEN)
#define MOTOR_EN_GPIO 119
#else
#define MOTOR_EN_GPIO 6
#endif

static struct work_struct work_vibrator_on;
static struct work_struct work_vibrator_off;

static struct hrtimer vibe_timer;

#if (defined(CONFIG_MACH_ARUBA_OPEN) || defined(CONFIG_MACH_ARUBASLIM_OPEN) || defined(CONFIG_MACH_ROY)|| defined(CONFIG_MACH_DELOS_OPEN))  \
&& !defined(CONFIG_GPIO_CONTROL_VIBRATOR)
static struct regulator *vreg_msm_vibrator;
static int enabled = 0;
#endif

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
	static int vib_status = 0;

#if (defined(CONFIG_MACH_ARUBA_OPEN) || defined(CONFIG_MACH_ARUBASLIM_OPEN) || defined(CONFIG_MACH_ROY)|| defined(CONFIG_MACH_DELOS_OPEN))  \
&& !defined(CONFIG_GPIO_CONTROL_VIBRATOR)	
	int ret = 0;
#else
	unsigned int vib_level = 0;
#endif 

	printk("%s on = %d vib_status = %d\n", __func__, on, vib_status);

	if(vib_status == on)	{
		pr_err("[VIB] %s: set duplicated. so skipped!", __func__);
		return;
	}
	else {
		vib_status = on;
	}

#if (defined(CONFIG_MACH_ARUBA_OPEN) || defined(CONFIG_MACH_ARUBASLIM_OPEN) || defined(CONFIG_MACH_ROY) || defined(CONFIG_MACH_DELOS_OPEN))  \
&& !defined(CONFIG_GPIO_CONTROL_VIBRATOR)	
	if (on) {

		ret = regulator_set_voltage(vreg_msm_vibrator, 3050000, 3050000);

		if (ret) {
			printk(KERN_ERR "%s: vreg set level failed (%d)\n",
					__func__, ret);
			regulator_put(vreg_msm_vibrator);
			return;
		}
		if (!enabled) {
			enabled = 1;
			ret = regulator_enable(vreg_msm_vibrator);
		}
		if (ret) {
			printk(KERN_ERR "%s: vreg enable failed (%d)\n",
					__func__, ret);
			return;
		}
		mdelay(10);
	}
	else {
		if (enabled) {
			enabled = 0;
			ret = regulator_disable(vreg_msm_vibrator);
		}
		if (ret) {
			printk(KERN_ERR "%s: vreg disable failed (%d)\n",
					__func__, ret);
			return;
		}
	}
#else
	if (on) {
		vib_level = 1;
		}
	else {
		vib_level = 0;
	}

	gpio_set_value(MOTOR_EN_GPIO, vib_level);
#endif
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

#if defined(CONFIG_MACH_KYLEPLUS_OPEN) || defined(CONFIG_MACH_DELOS_OPEN)
		if(value < 30)
		{
			printk("[VIB] Duration is too short don't need to enable : %d sec\n" , value);			
			return;				
		}
#endif
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

static struct timed_output_dev msm_vibrator = {
	.name = "vibrator",
	.get_time = vibrator_get_time,
	.enable = vibrator_enable,
};

static int msm_vibrator_probe(struct platform_device *pdev)
{
	int rc = 0;

	printk("[VIB] Prob\n");

	INIT_WORK(&work_vibrator_on, msm_vibrator_on);
	INIT_WORK(&work_vibrator_off, msm_vibrator_off);

	hrtimer_init(&vibe_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	vibe_timer.function = vibrator_timer_func;

	rc = timed_output_dev_register(&msm_vibrator);
	if (rc < 0) {
		goto err_read_vib;
	}

#if (!defined(CONFIG_MACH_ARUBA_OPEN) && !defined(CONFIG_MACH_ROY) && !defined(CONFIG_MACH_DELOS_OPEN))  \
||defined(CONFIG_GPIO_CONTROL_VIBRATOR)	
	gpio_tlmm_config(GPIO_CFG(MOTOR_EN_GPIO, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_10MA),GPIO_CFG_ENABLE);
#endif

	return 0;

err_read_vib:
	printk(KERN_ERR "[VIB] timed_output_dev_register fail (rc=%d)\n", rc);
	return rc;
}

static int __init msm_timed_vibrator_init(void)
{
	int rc = 0;

	printk("[VIB] Init\n");

	rc = platform_driver_register(&msm_vibrator_platdrv);
	if (rc)	{ 
		printk("[VIB] platform_driver_register failed : %d\n",rc);
	}

#if (defined(CONFIG_MACH_ARUBA_OPEN) || defined(CONFIG_MACH_ROY) || defined(CONFIG_MACH_DELOS_OPEN))  \
&& !defined(CONFIG_GPIO_CONTROL_VIBRATOR)	
	vreg_msm_vibrator = regulator_get(NULL, "ldo01");

	if (IS_ERR (vreg_msm_vibrator)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
				__func__, PTR_ERR(vreg_msm_vibrator));
		return PTR_ERR(vreg_msm_vibrator);
	}
#endif

	return rc;
}

void __exit msm_timed_vibrator_exit(void)
{
	printk("[VIB] Exit\n");

	platform_driver_unregister(&msm_vibrator_platdrv);
}

module_init(msm_timed_vibrator_init);
module_exit(msm_timed_vibrator_exit);

MODULE_DESCRIPTION("timed output vibrator device");
MODULE_LICENSE("GPL");

