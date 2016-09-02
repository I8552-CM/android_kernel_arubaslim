/****************************************************************************

**

** COPYRIGHT(C) : Samsung Electronics Co.Ltd, 2006-2012 ALL RIGHTS RESERVED

**

****************************************************************************/

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/mm.h>


#include <linux/irq.h>

#include <linux/version.h>


#include <mach/msm_battery.h>
#include <mach/hardware.h>
#include <asm/uaccess.h>
#include <mach/msm_iomap.h>
#include <linux/proc_fs.h>
#include <linux/wakelock.h>
#include <asm/io.h>
#include "sec_dump.h"
#include "../../arch/arm/mach-msm/proc_comm.h"
#include "../arch/arm/mach-msm/smd_private.h"

#ifdef CONFIG_SEC_MISC
#include <linux/sec_param.h>

#define SEC_DEBUG_LEVEL_LOW 0x4674AEAF
#define SEC_DEBUG_LEVEL_MID 0x4674BEBF
#define SEC_DEBUG_LEVEL_HIGH 0x4674CECF

int param_debug_level;
struct delayed_work wq_param_init;
#endif
struct wake_lock silent_wake_lock;
#define _DEBUG
#ifdef _DEBUG
#define dprintk(s, args...) \
	printk(KERN_INFO "%s:%d - " s, __func__, __LINE__,  ##args)
#else
#define dprintk(s, args...)
#endif	/* _DEBUG */

samsung_vendor1_id *smem_vendor1;
int silent_value;
#if defined(CONFIG_MACH_KYLE)
int default_dump_enable_flag;
#else
int default_dump_enable_flag;
#endif
int dump_enable_flag;
EXPORT_SYMBOL(dump_enable_flag);

extern void *smem_alloc(unsigned, unsigned);

#if 0
static void secdump_ramdump(void);

static void secdump_ramdump(void)
{
	dprintk("[DPRAM] RAMDUMP MODE START!\n");
	writel(0xCCCC, MSM_SHARED_RAM_BASE + 0x30); 
	dprintk("[DPRAM] call msm_proc_comm_reset_modem_now func\n");
	msm_proc_comm_reset_modem_now();
}
#endif

#ifdef CONFIG_SEC_MISC
static void param_init(struct work_struct *data)
{
	unsigned int power_off_reason;
	unsigned int update_cp_bin;

#ifdef CONFIG_SEC_DEBUG
	if (charging_boot != 1) {
		sec_get_param(param_power_off_reason, &power_off_reason);
		printk(KERN_INFO "Previous Power Off Status ==> %s\n",\
			((power_off_reason & 0xF0) == 0xF0) ? "Normal" :\
			(((power_off_reason & 0xF0) == 0xB0) ? "Reset" :\
								"Abnormal"));
		power_off_reason = (power_off_reason | 0xA) << 4;
		sec_set_param(param_power_off_reason, &power_off_reason);
	}
#endif
	sec_get_param(param_index_debuglevel, &param_debug_level);
	dprintk("param_debug_level : 0x%X\n", param_debug_level);

	sec_get_param(param_update_cp_bin, &update_cp_bin);
	dprintk("param_update_cp_bin : 0x%X\n", update_cp_bin);

	dump_enable_flag = default_dump_enable_flag;
	smem_vendor1->ram_dump_level = dump_enable_flag;

	if (default_dump_enable_flag)
		return;

	if (param_debug_level != SEC_DEBUG_LEVEL_LOW
			&& param_debug_level != SEC_DEBUG_LEVEL_MID
			&& param_debug_level != SEC_DEBUG_LEVEL_HIGH) {
		if (dump_enable_flag == 0)
			param_debug_level = SEC_DEBUG_LEVEL_LOW;
		else if (dump_enable_flag == 1)
			param_debug_level = SEC_DEBUG_LEVEL_MID;
		else if (dump_enable_flag == 2)
			param_debug_level = SEC_DEBUG_LEVEL_HIGH;
		else
			dprintk("Error Setup param for debug_level\n");

		sec_set_param(param_index_debuglevel, &param_debug_level);
	} else {
		if (param_debug_level == SEC_DEBUG_LEVEL_LOW)
			dump_enable_flag = 0;
		else if (param_debug_level == SEC_DEBUG_LEVEL_MID)
			dump_enable_flag = 1;
		else if (param_debug_level == SEC_DEBUG_LEVEL_HIGH)
			dump_enable_flag = 2;
		else
			dprintk("Error Setup param for debug_level\n");

	}

	smem_vendor1->ram_dump_level = dump_enable_flag;
}
#endif



static int __devinit secdump_probe(struct platform_device *dev)
{
//	int retval;

	/* allocate smem dpram area */
	dprintk("secdump_probe\n");

#ifdef CONFIG_SEC_MISC
	INIT_DELAYED_WORK(&wq_param_init, param_init);
	schedule_delayed_work(&wq_param_init, msecs_to_jiffies(3000));
#endif

	return 0;
}

static int __devexit secdump_remove(struct platform_device *dev)
{
	return 0;
}

static struct platform_driver platform_secdump_driver = {
	.probe = secdump_probe,
	.remove = __devexit_p(secdump_remove),
	.driver = {
		.name = "secdump",
	},
};

static int silent_read_proc_debug(char *page, char **start, off_t offset,
		                    int count, int *eof, void *data)
{
	*eof = 1;
	return sprintf(page, "%u\n", silent_value);
}

static int silent_write_proc_debug(struct file *file, const char *buffer,
		                    unsigned long count, void *data)
{
	char *buf;

	if (count < 1)
		return -EINVAL;

	buf = kmalloc(count, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	if (copy_from_user(buf, buffer, count)) {
		kfree(buf);
		return -EFAULT;
	}

	if (buf[0] == '0') {
		silent_value = 0;
		dprintk("Set silent : %d\n", silent_value);
	} else if (buf[0] == '1') {
		silent_value = 1;
		dprintk("Set silent : %d\n", silent_value);
	} else {
		kfree(buf);
		return -EINVAL;
	}

	kfree(buf);
	return count;
}

static int dump_read_proc_debug(char *page, char **start, off_t offset,
		                    int count, int *eof, void *data)
{
	*eof = 1;
	printk("[DUMP] %s %d\n",__func__, __LINE__);
	return sprintf(page, "%u\n", dump_enable_flag);
}

static int dump_write_proc_debug(struct file *file, const char *buffer,
		                    unsigned long count, void *data)
{
	char *buf;

	printk("[DUMP] %s %d\n",__func__, __LINE__);

	if (count < 1)
		return -EINVAL;

	buf = kmalloc(count, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	if (copy_from_user(buf, buffer, count)) {
		kfree(buf);
		return -EFAULT;
	}

	if (buf[0] == '0') { /* low (no RAM dump) */
		dump_enable_flag = 0;
		if (smem_vendor1)
			smem_vendor1->silent_reset = 0xAEAEAEAE;
	} else if (buf[0] == '1') { /* middle (kernel fault) */
		dump_enable_flag = 1;
		if (smem_vendor1)
			smem_vendor1->silent_reset = 0xA9A9A9A9;
	} else if (buf[0] == '2') { /* high (user fault) */
		dump_enable_flag = 2;
		if (smem_vendor1)
			smem_vendor1->silent_reset = 0xA9A9A9A9;
	} else {
		kfree(buf);
		return -EINVAL;
	}

	printk("[DUMP] %s %d dump_enable_flag:%d \n",__func__, __LINE__, dump_enable_flag);

	smem_vendor1->ram_dump_level = dump_enable_flag;

#ifdef CONFIG_SEC_MISC
	if (dump_enable_flag == 0)
		param_debug_level = SEC_DEBUG_LEVEL_LOW;
	else if (dump_enable_flag == 1)
		param_debug_level = SEC_DEBUG_LEVEL_MID;
	else if (dump_enable_flag == 2)
		param_debug_level = SEC_DEBUG_LEVEL_HIGH;

	printk("[DUMP] param_debug_level : 0x%X, dump_enable_level : %d\n",
			param_debug_level, dump_enable_flag);
	sec_set_param(param_index_debuglevel, &param_debug_level);
#endif

	if (!smem_vendor1)
		pr_err("smem_vendor1 is NULL!");
	else
		dprintk("dump_enable_flag : %d, smem_vendor1 : 0x%08x\n",
				dump_enable_flag, smem_vendor1->silent_reset);

	kfree(buf);
	return count;
}

/* init & cleanup. */
static int __init secdump_init(void)
{
	int ret;
	struct proc_dir_entry *ent;

	printk("[DUMP] %s %d\n",__func__, __LINE__);
	
	ret = platform_driver_register(&platform_secdump_driver);
	if (ret)
		goto error_return;

	wake_lock_init(&silent_wake_lock, WAKE_LOCK_SUSPEND, "SILENT_RESET");
	platform_device_register_simple("secdump", -1, NULL, 0);

	/* For silent ram dump mode */
	ent = create_proc_entry("silent", S_IRWXUGO, NULL);
	ent->read_proc = silent_read_proc_debug;
	ent->write_proc = silent_write_proc_debug;
	smem_vendor1 = (samsung_vendor1_id *)smem_alloc2(SMEM_ID_VENDOR1,
			sizeof(samsung_vendor1_id));
	if (smem_vendor1 && smem_vendor1->silent_reset == 0xAEAEAEAE)
		silent_value = 1;

	ent = create_proc_entry("dump_enable", S_IRWXUGO, NULL);
	ent->read_proc = dump_read_proc_debug;
	ent->write_proc = dump_write_proc_debug;
	if (smem_vendor1) {
		smem_vendor1->silent_reset = 0;
		dprintk("smem_vendor1 = 0x%X\n", (unsigned int)smem_vendor1);
		/* Save smem vendor1 address , do NOT change it */
		writel_relaxed((void *)(smem_vendor1) - MSM_SHARED_RAM_BASE,
				MSM_SHARED_RAM_BASE + 0xFFA00);
	}

	printk("[Silent Value] : %d\n", silent_value);

error_return:
	return ret;
}

static void __exit secdump_exit(void)
{
	wake_lock_destroy(&silent_wake_lock);

	platform_driver_unregister(&platform_secdump_driver);
}

module_init(secdump_init);
module_exit(secdump_exit);

MODULE_AUTHOR("SAMSUNG ELECTRONICS CO., LTD");
MODULE_DESCRIPTION("SEC DUMP Device Driver for Linux MITs.");
MODULE_LICENSE("GPL");

