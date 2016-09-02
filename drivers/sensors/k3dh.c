/*
 *  STMicroelectronics k3dh acceleration sensor driver
 *
 *  Copyright (C) 2010 Samsung Electronics Co.Ltd
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/wakelock.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/pm.h>
#include <linux/k3dh.h>
#include <linux/workqueue.h>
#include <linux/hrtimer.h>

#include "sensors_head.h"

#define K3DH_DEBUG 0

#if K3DH_DEBUG
#define ACCDBG(fmt, args...) printk(KERN_INFO fmt, ## args)
#else
#define ACCDBG(fmt, args...)
#endif

#undef DEBUG_REACTIVE_ALERT
/* The default settings when sensor is on is for all 3 axis to be enabled
 * and output data rate set to 400Hz.  Output is via a ioctl read call.
 */
#define DEFAULT_POWER_ON_SETTING (ODR400 | ENABLE_ALL_AXES)
#define DEFAULT_POWER_ON_SETTING_1344HZ (ODR1344 | ENABLE_ALL_AXES)

#ifdef USES_MOVEMENT_RECOGNITION
#define DEFAULT_CTRL3_SETTING		0x60 /* INT enable */
#define DEFAULT_INTERRUPT_SETTING	0x0A /* INT1 XH,YH : enable */
#define DEFAULT_INTERRUPT2_SETTING	0x20 /* INT2 ZH enable */
#define DEFAULT_THRESHOLD		0x7F /* 2032mg (16*0x7F) */
#define DYNAMIC_THRESHOLD		300	/* mg */
#define DYNAMIC_THRESHOLD2		700	/* mg */
#define MOVEMENT_DURATION		0x00 /*INT1 (DURATION/odr)ms*/
enum {
	OFF = 0,
	ON = 1
};
#define ABS(a)		(((a) < 0) ? -(a) : (a))
#define MAX(a, b)	(((a) > (b)) ? (a) : (b))
#endif

#define ACC_DEV_NAME "accelerometer"
#define ACC_DEV_MAJOR 241
#define K3DH_RETRY_COUNT	3

#define CALIBRATION_FILE_PATH	"/efs/calibration_data"
#define CALIBRATION_DATA_AMOUNT	20

#define ACC_ENABLED 1

/* This driver can be used for k3dh, k2dh, k2dm... */
#if defined(CONFIG_MACH_ROY)
#define CHIP_DEV_NAME	"K2DH"
#else
#define CHIP_DEV_NAME	"K3DH"
#endif
#define CHIP_DEV_VENDOR	"STMICRO"

#if defined(CONFIG_SENSORS_ACC_FILTER)
 // coefficients for filter
#define ORDER	2
#define COEFFA0	(100000)
#define COEFFA1	(-153840)
#define COEFFA2	(62650)

#define COEFFB0	(2200)
#define COEFFB1	(4410)
#define COEFFB2	(2200)

#define IN_COEFF_NUM	(ORDER + 1)
#define OUT_COEFF_NUM	(ORDER + 1)
#define INBUFSIZE	(2 * (IN_COEFF_NUM))
#define OUTBUFSIZE	(2 * (OUT_COEFF_NUM))
#endif

static struct device *accelerometer_device;
static const struct odr_delay {
	u8 odr; /* odr reg setting */
	s64 delay_ns; /* odr in ns */
} odr_delay_table[] = {
	{ ODR1344,     744047LL }, /* 1344Hz */
	{  ODR400,    2500000LL }, /*  400Hz */
	{  ODR200,    5000000LL }, /*  200Hz */
	{  ODR100,   10000000LL }, /*  100Hz */
	{   ODR50,   20000000LL }, /*   50Hz */
	{   ODR25,   40000000LL }, /*   25Hz */
	{   ODR10,  100000000LL }, /*   10Hz */
	{    ODR1, 1000000000LL }, /*    1Hz */
};

/* K3DH acceleration data */
struct k3dh_acc {
	s16 x;
	s16 y;
	s16 z;
};

struct k3dh_data {
	struct i2c_client *client;
	struct miscdevice k3dh_device;
	struct input_dev *acc_input_dev;
	struct mutex read_lock;
	struct mutex write_lock;
	struct mutex power_lock;
	struct k3dh_acc cal_data;
	u8 state;
	u8 ctrl_reg1_shadow;
	atomic_t opened; /* opened implies enabled */
#ifdef USES_MOVEMENT_RECOGNITION
	int movement_recog_flag;
	unsigned char interrupt_state;
	struct wake_lock reactive_wake_lock;
	struct k3dh_platform_data *pdata;
	int irq;
#endif
#if defined(CONFIG_SENSORS_ACC_FILTER)
	struct work_struct read_work;
	struct workqueue_struct *read_workqueue;
	struct hrtimer timer;
	ktime_t device_polling_delay;
	unsigned int hrtimer_delay_usec;
	int xyz[3];
	bool reset_filter;
	bool enabled_filter;
#endif
};

#if defined(CONFIG_SENSORS_CORE)
extern struct class *sensors_class;
#endif
static struct k3dh_data * g_k3dh;
static struct k3dh_acc g_acc;
static struct i2c_client *k3dh_client = NULL;
static int acc_mode_cnt = 0;

static int k3dh_open_calibration(void);

static int k3dh_acc_i2c_read(char *buf, int len)
{
	uint8_t i;
	struct i2c_msg msgs[] = {
		{
			.addr	= k3dh_client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= buf,
		},
		{
			.addr	= k3dh_client->addr,
			.flags	= I2C_M_RD,
			.len	= len,
			.buf	= buf,
		}
	};

	for (i = 0; i < K3DH_RETRY_COUNT; i++) {
		if (i2c_transfer(k3dh_client->adapter, msgs, 2) >= 0) {
			break;
		}
		mdelay(10);
	}

	if (i >= K3DH_RETRY_COUNT) {
		pr_err("%s: retry over %d\n", __FUNCTION__, K3DH_RETRY_COUNT);
		return -EIO;
	}

	return 0;
}

static int k3dh_acc_i2c_write(char *buf, int len)
{
	uint8_t i;
	struct i2c_msg msg[] = {
		{
			.addr	= k3dh_client->addr,
			.flags	= 0,
			.len	= len,
			.buf	= buf,
		}
	};

	for (i = 0; i < K3DH_RETRY_COUNT; i++) {
		if (i2c_transfer(k3dh_client->adapter, msg, 1) >= 0) {
			break;
		}
		mdelay(10);
	}

	if (i >= K3DH_RETRY_COUNT) {
		pr_err("%s: retry over %d\n", __FUNCTION__, K3DH_RETRY_COUNT);
		return -EIO;
	}
	return 0;
}


#if defined(CONFIG_SENSORS_ACC_FILTER)
static void k3dh_acc_filter(struct k3dh_acc *acc)
{
	int axis, i;// j, modout;
	s32 mean_avg = 0;
	s32 auto_regr = 0;
	s32 temp = 0;
	int posin, posout;

	static const s32 a0 = COEFFA0;
	static const s32 a1 = COEFFA1;
	static const s32 a2 = COEFFA2;

	static const s32 b0 = COEFFB0;
	static const s32 b1 = COEFFB1;
	static const s32 b2 = COEFFB2;

	static s32 in[3][INBUFSIZE];
	static s32 out[3][OUTBUFSIZE];
	static int currin = INBUFSIZE - 1;
	static int currout = OUTBUFSIZE - 1;

	g_k3dh->xyz[0] = acc->x;
	g_k3dh->xyz[1] = acc->y;
	g_k3dh->xyz[2] = acc->z;

	if (g_k3dh->reset_filter)
	{
		for(axis = 0; axis < 3; axis++)
		{
			for(i = 0; i < INBUFSIZE; i++)
			{
				in[axis][i] = g_k3dh->xyz[axis];
			}
			for(i = 0; i < OUTBUFSIZE; i++)
			{
				out[axis][i] = g_k3dh->xyz[axis];
			}

			currin = INBUFSIZE - 1;
			currout = OUTBUFSIZE - 1;
		}
		g_k3dh->reset_filter = false;
	}

	for (axis = 2; axis >= 0; axis--)
	{
		posin = currin;
		posout = currout ;

		in[axis][currin] = g_k3dh->xyz[axis];

		//MA section
		mean_avg += b0 * in[axis][posin];
		posin--;
		if (posin < 0) posin = INBUFSIZE - 1;
		mean_avg += b1 * in[axis][posin];
		posin--;
		if (posin < 0) posin = INBUFSIZE - 1;
		mean_avg += b2 * in[axis][posin];

		posout--;
		if (posout < 0) posout = OUTBUFSIZE - 1;
		auto_regr += - a1 * out[axis][posout];
		posout--;
		if (posout < 0) posout = OUTBUFSIZE - 1;
		auto_regr += - a2 * out[axis][posout];

		out[axis][currout] = mean_avg + auto_regr; //MA sec + AR section
		temp = out[axis][currout] / a0;

		out[axis][currout] = temp;
		g_k3dh->xyz[axis] = out[axis][currout];

		mean_avg = 0;
		auto_regr = 0;
	}

	acc->x = g_k3dh->xyz[0];
	acc->y = g_k3dh->xyz[1];
	acc->z = g_k3dh->xyz[2];

	currin++;
	if (currin == INBUFSIZE)
		currin = 0;
	currout++;
	if (currout == OUTBUFSIZE)
		currout = 0;

}
#endif

/* Read X,Y and Z-axis acceleration raw data */
static int k3dh_read_accel_raw_xyz(struct k3dh_acc *acc)
{
	int err;
	unsigned char acc_data[6] = {0};

	acc_data[0] = OUT_X_L | AC; /* read from OUT_X_L to OUT_Z_H by auto-inc */
	err = k3dh_acc_i2c_read(acc_data, 6);
	if (err < 0)
	{
		pr_err("k3dh_read_accel_raw_xyz() failed\n");
		return err;
	}

	acc->x = (acc_data[1] << 8) | acc_data[0];
	acc->y = (acc_data[3] << 8) | acc_data[2];
	acc->z = (acc_data[5] << 8) | acc_data[4];

	acc->x = acc->x >> 4;
	acc->y = acc->y >> 4;
	acc->z = acc->z >> 4;

#if defined(CONFIG_SENSORS_ACC_FILTER)
	if (g_k3dh->enabled_filter){
		k3dh_acc_filter(acc);
	}
#endif

	return 0;
}

static int k3dh_read_accel_xyz(struct k3dh_acc *acc)
{
	int err = 0;

	err = k3dh_read_accel_raw_xyz(acc);
	if (err < 0) {
		pr_err("k3dh_read_accel_xyz() failed\n");
		return err;
	}

	acc->x -= g_k3dh->cal_data.x;
	acc->y -= g_k3dh->cal_data.y;
	acc->z -= g_k3dh->cal_data.z;

	g_acc.x = acc->x;
	g_acc.y = acc->y;
	g_acc.z = acc->z;

	return err;
}

static atomic_t flgEna;
static atomic_t delay;
static int probe_done;

int k3dh_get_acceleration_data(int *xyz)
{
	struct k3dh_acc acc;
	int err = -1;

	err = k3dh_read_accel_xyz(&acc);

	xyz[0] = (int)(acc.x);
	xyz[1] = (int)(acc.y);
	xyz[2] = (int)(acc.z);

	ACCDBG("[K3DH] Acc_I2C, x:%d, y:%d, z:%d\n", xyz[0], xyz[1], xyz[2]);

	return err;
}

void k3dh_activate(int flgatm, int flg, int dtime)
{
	unsigned char acc_data[2] = {0};

	printk(KERN_INFO "[K3DH] k3dh_activate : flgatm=%d, flg=%d, dtime=%d\n", flgatm, flg, dtime);

	if (flg != 0) flg = 1;

	//Power modes
	if (flg == 0) //sleep
	{
#ifdef USES_MOVEMENT_RECOGNITION
	if (g_k3dh->movement_recog_flag == ON) {
		printk(KERN_INFO "[K3DH] [%s] LOW_PWR_MODE.\n", __FUNCTION__);
		acc_data[0] = CTRL_REG1;
		acc_data[1] = LOW_PWR_MODE;
		if(k3dh_acc_i2c_write(acc_data, 2) !=0)
			printk(KERN_ERR "[%s] Change to Low Power Mode is failed\n",__FUNCTION__);
	} else
#endif
	{
		acc_data[0] = CTRL_REG1;
		acc_data[1] = PM_OFF;
		if(k3dh_acc_i2c_write(acc_data, 2) !=0)
			printk(KERN_ERR "[%s] Change to Suspend Mode is failed\n",__FUNCTION__);
	}
	g_k3dh->ctrl_reg1_shadow = PM_OFF;
	g_k3dh->state = 0;
#if defined(CONFIG_SENSORS_ACC_FILTER)
	hrtimer_cancel(&g_k3dh->timer);
	cancel_work_sync(&g_k3dh->read_work);
#endif
	acc_mode_cnt=0;
	}
	else
	{
		g_k3dh->ctrl_reg1_shadow = DEFAULT_POWER_ON_SETTING;
		acc_data[0] = CTRL_REG1;
#if defined(CONFIG_SENSORS_ACC_FILTER)
	acc_data[1] = DEFAULT_POWER_ON_SETTING_1344HZ;
#else
		acc_data[1] = DEFAULT_POWER_ON_SETTING;
#endif
		if(k3dh_acc_i2c_write(acc_data, 2) !=0)
			printk(KERN_ERR "[%s] Change to Normal Mode(CTRL_REG1) is failed\n",__FUNCTION__);

		acc_data[0] = CTRL_REG4;
		acc_data[1] = CTRL_REG4_HR;
		if(k3dh_acc_i2c_write(acc_data, 2) !=0)
			printk(KERN_ERR "[%s] Change to Normal Mode(CTRL_REG4) is failed\n",__FUNCTION__);

		g_k3dh->state |= ACC_ENABLED;

#if defined(CONFIG_SENSORS_ACC_FILTER)
	g_k3dh->reset_filter = true;
	hrtimer_start(&g_k3dh->timer, g_k3dh->device_polling_delay, HRTIMER_MODE_REL);
#endif
		if(!acc_mode_cnt)
		{
			printk(KERN_INFO "[K3DH] k3dh_activate : (%d,%d,%d)\n", g_k3dh->cal_data.x, g_k3dh->cal_data.y, g_k3dh->cal_data.z);
			acc_mode_cnt++;
		}
	}

	mdelay(2);

	if (flgatm) {
		atomic_set(&flgEna, flg);
		atomic_set(&delay, dtime);
	}
}

int k3dh_io_open(void)
{
	if(probe_done == PROBE_SUCCESS)
		k3dh_open_calibration();
	return probe_done;
}

EXPORT_SYMBOL(k3dh_get_acceleration_data);
EXPORT_SYMBOL(k3dh_activate);
EXPORT_SYMBOL(k3dh_io_open);

#if defined(CONFIG_SENSORS_ACC_FILTER)
static void k3dh_acc_read_work_func(struct work_struct *work)
{
	struct k3dh_acc acc;
	mutex_lock(&g_k3dh->read_lock);
	k3dh_read_accel_xyz(&acc);
	hrtimer_start(&g_k3dh->timer, g_k3dh->device_polling_delay, HRTIMER_MODE_REL);
	mutex_unlock(&g_k3dh->read_lock);
}

static enum hrtimer_restart k3dh_acc_timer_func(struct hrtimer *timer)
{
	queue_work(g_k3dh->read_workqueue, &g_k3dh->read_work);
	return HRTIMER_NORESTART;
}
#endif


static int k3dh_open_calibration(void)
{
	struct file *cal_filp = NULL;
	int err = 0, ret = 0;
	mm_segment_t old_fs;

	printk(KERN_INFO "[K3DH] %s\n",__FUNCTION__);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CALIBRATION_FILE_PATH, O_RDONLY, 0660);
	if (IS_ERR(cal_filp)) {
		err = PTR_ERR(cal_filp);
		pr_err("%s: Can't open calibration file= %d\n", __func__, err);
		set_fs(old_fs);
		return err;
	}

	if (cal_filp && cal_filp->f_op && cal_filp->f_op->read){
		printk(KERN_INFO "file read %u @ %llu \n", 3 * sizeof(s16), (unsigned long long)cal_filp->f_pos);
		ret = cal_filp->f_op->read(cal_filp, (char *)(&g_k3dh->cal_data), 3 * sizeof(s16), &cal_filp->f_pos);
		if (ret != 3 * sizeof(s16)) {
			pr_err("%s: Can't read the cal data from file= %d\n", __func__, ret);
			err = -EIO;
		}

		printk(KERN_INFO "file read count=%d \n", (int) ret);
		printk(KERN_INFO "%s: (%d,%d,%d)\n", __func__, g_k3dh->cal_data.x, g_k3dh->cal_data.y, g_k3dh->cal_data.z);

	}

	if(cal_filp)
		filp_close(cal_filp, NULL);
	set_fs(old_fs);
	printk(KERN_INFO "k3dh_open_calibration done!!\n");
	return err;
}


static int k3dh_do_calibration(void)
{
	struct k3dh_acc data = { 0, };
	int sum[3] = { 0, };
	int err = 0;
	int i;

	printk(KERN_INFO "[K3DH] %s\n",__FUNCTION__);

	for (i = 0; i < CALIBRATION_DATA_AMOUNT; i++) {
		err = k3dh_read_accel_raw_xyz(&data);
		if (err < 0) {
			pr_err("%s: k3dh_read_accel_raw_xyz() failed in the %dth loop\n", __func__, i);
			return err;
		}

		sum[0] += data.x;
		sum[1] += data.y;
		sum[2] += data.z;

		ACCDBG("[K3DH] calibration sum data (%d,%d,%d)\n", sum[0], sum[1], sum[2]);
	}

	g_k3dh->cal_data.x = sum[0] / CALIBRATION_DATA_AMOUNT;//K3DH(12bit) 0+-154, K3DM(8bit) 0+-12
	g_k3dh->cal_data.y = sum[1] / CALIBRATION_DATA_AMOUNT;//K3DH(12bit) 0+-154, K3DM(8bit) 0+-12
	g_k3dh->cal_data.z = (sum[2] / CALIBRATION_DATA_AMOUNT) + 1024;//K3DH(12bit) 1024 +-226, K3DM(8bit) 64+-16

	printk(KERN_INFO "%s: cal data (%d,%d,%d)\n", __func__,	g_k3dh->cal_data.x, g_k3dh->cal_data.y, g_k3dh->cal_data.z);

	return err;
}


static int k3dh_do_calibration_fs(int is_cal_erase)
{
	struct k3dh_acc data = { 0, };
	struct file *cal_filp = NULL;
	int sum[3] = { 0, };
	int err = 0, ret = 0;
	int i;
	mm_segment_t old_fs;
	unsigned long calSize, tmpSize;
	char* tmpBuf = NULL;

	printk(KERN_INFO "[K3DH] %s\n",__FUNCTION__);

	if (is_cal_erase == 1)
	{
		g_k3dh->cal_data.x = 0;
		g_k3dh->cal_data.y = 0;
		g_k3dh->cal_data.z = 0;
	}
	else
	{
		for (i = 0; i < CALIBRATION_DATA_AMOUNT; i++) {
			err = k3dh_read_accel_raw_xyz(&data);
			if (err < 0) {
				pr_err("%s: k3dh_read_accel_raw_xyz() failed in the %dth loop\n", __func__, i);
				return err;
			}

			sum[0] += data.x;
			sum[1] += data.y;
			sum[2] += data.z;

			ACCDBG("[K3DH] calibration sum data (%d,%d,%d)\n", sum[0], sum[1], sum[2]);
		}

		g_k3dh->cal_data.x = sum[0] / CALIBRATION_DATA_AMOUNT;//K3DH(12bit) 0+-154, K3DM(8bit) 0+-12
		g_k3dh->cal_data.y = sum[1] / CALIBRATION_DATA_AMOUNT;//K3DH(12bit) 0+-154, K3DM(8bit) 0+-12
#if defined (CALIBRATION_BASED_ON_CURRENT_Z)
		/* Set z-axis calibration target based on current z-axis value */
		if ( sum[2] >= 0)
			g_k3dh->cal_data.z = (sum[2] / CALIBRATION_DATA_AMOUNT) - 1024;//K3DH(12bit) 1024 +-226, K3DM(8bit) 64+-16
		else
			g_k3dh->cal_data.z = (sum[2] / CALIBRATION_DATA_AMOUNT) + 1024;//K3DH(12bit) 1024 +-226, K3DM(8bit) 64+-16
#else
		g_k3dh->cal_data.z = (sum[2] / CALIBRATION_DATA_AMOUNT) + 1024;//K3DH(12bit) 1024 +-226, K3DM(8bit) 64+-16
#endif
	}

	printk(KERN_INFO "%s: cal data (%d,%d,%d)\n", __func__,	g_k3dh->cal_data.x, g_k3dh->cal_data.y, g_k3dh->cal_data.z);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CALIBRATION_FILE_PATH, O_CREAT |O_TRUNC | O_WRONLY, 0660);
	if (IS_ERR(cal_filp)) {
		err = PTR_ERR(cal_filp);
		pr_err("%s: Can't open calibration file= %d\n", __func__, err);
		set_fs(old_fs);
		return err;
	}

	if (cal_filp && cal_filp->f_op && cal_filp->f_op->write){
		tmpBuf = kmalloc(PAGE_SIZE, GFP_KERNEL);
		if ( !tmpBuf ) {
			pr_err("%s: failed to allocate temp buf \n", __func__);
			err = -EIO;
			filp_close(cal_filp, NULL);
			set_fs(old_fs);
			return err;
		}

		printk(KERN_INFO "file write %u @ %llu \n", 3 * sizeof(s16), (unsigned long long)cal_filp->f_pos);

		calSize = sizeof(s16) * 3;
		tmpSize = (calSize > PAGE_SIZE) ? PAGE_SIZE : calSize;
		memcpy( tmpBuf, (&g_k3dh->cal_data), tmpSize);

		ret = cal_filp->f_op->write(cal_filp,tmpBuf, tmpSize, &cal_filp->f_pos);
		if (ret != 3 * sizeof(s16)) {
			pr_err("%s: Can't write the cal data to file = %d\n", __func__, ret);
			err = -EIO;
		}

		printk(KERN_INFO "file write writtened=%d \n", (int) ret);
		kfree( tmpBuf );
		tmpBuf = NULL;
	}
	else
	{
		pr_err("%s: (cal_filp && cal_filp->f_op && cal_filp->f_op->write)= 0\n", __func__);
		err = -EIO;
	}

	if(cal_filp)
		filp_close(cal_filp, NULL);
	set_fs(old_fs);
	printk(KERN_INFO "k3dh_do_calibration_fs done!!\n");

	mdelay(200);

	return err;
}


/*  open command for K3DH device file  */
static int k3dh_open(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "[K3DH] %s\n",__FUNCTION__);
	printk(KERN_INFO "[K3DH] file->private_data = %ld\n",(long)file->private_data);
	printk(KERN_INFO "[K3DH]: dev=%02x:%02x, inode=%lu\n", MAJOR(inode->i_sb->s_dev), MINOR(inode->i_sb->s_dev), inode->i_ino);

	return 0;
}

/*  release command for K3DH device file */
static int k3dh_close(struct inode *inode, struct file *file)
{
	int err = 0;

	printk(KERN_INFO "[K3DH] %s\n",__FUNCTION__);

	atomic_sub(1, &g_k3dh->opened);

	return err;
}

static s64 k3dh_get_delay(struct k3dh_data *k3dh)
{
	int i;
	u8 odr;
	s64 delay = -1;

	odr = k3dh->ctrl_reg1_shadow & ODR_MASK;
	for (i = 0; i < ARRAY_SIZE(odr_delay_table); i++) {
		if (odr == odr_delay_table[i].odr) {
			delay = odr_delay_table[i].delay_ns;
			break;
		}
	}
	return delay;
}

static int k3dh_set_delay(struct k3dh_data *k3dh, s64 delay_ns)
{
	int odr_value = ODR1;
	int res = 0;
	int i;
	/* round to the nearest delay that is less than
	 * the requested value (next highest freq)
	 */
	ACCDBG(" passed %lldns\n", delay_ns);
	for (i = 0; i < ARRAY_SIZE(odr_delay_table); i++) {
		if (delay_ns < odr_delay_table[i].delay_ns)
			break;
	}
	if (i > 0)
		i--;
	ACCDBG("matched rate %lldns, odr = 0x%x\n",
			odr_delay_table[i].delay_ns,
			odr_delay_table[i].odr);
	odr_value = odr_delay_table[i].odr;
	delay_ns = odr_delay_table[i].delay_ns;
	mutex_lock(&k3dh->write_lock);
	ACCDBG("old = %lldns, new = %lldns\n",
		     k3dh_get_delay(k3dh), delay_ns);
	if (odr_value != (k3dh->ctrl_reg1_shadow & ODR_MASK)) {
		u8 ctrl = (k3dh->ctrl_reg1_shadow & ~ODR_MASK);
		ctrl |= odr_value;
		k3dh->ctrl_reg1_shadow = ctrl;
		res = i2c_smbus_write_byte_data(k3dh->client, CTRL_REG1, ctrl);
		ACCDBG("writing odr value 0x%x\n", odr_value);
	}
	mutex_unlock(&k3dh->write_lock);
	return res;
}

/*  ioctl command for K3DH device file */
static long k3dh_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int err = 0;
	struct k3dh_data *k3dh = file->private_data;
	struct k3dh_acc data = { 0, };
	s64 delay_ns;

	/* cmd mapping */
	switch (cmd) {
	case K3DH_IOCTL_SET_DELAY:
		printk(KERN_INFO "[K3DH] K3DH_IOCTL_SET_DELAY\n");
		if (copy_from_user(&delay_ns, (void __user *)arg, sizeof(delay_ns)))
			return -EFAULT;
		err = k3dh_set_delay(k3dh, delay_ns);
		break;
	case K3DH_IOCTL_GET_DELAY:
		printk(KERN_INFO "[K3DH] K3DH_IOCTL_GET_DELAY\n");
		delay_ns = k3dh_get_delay(k3dh);
		if (put_user(delay_ns, (s64 __user *)arg))
			return -EFAULT;
		break;

	case K3DH_IOCTL_SET_CALIBRATION:
		printk(KERN_INFO "[K3DH] K3DH_IOCTL_SET_CALIBRATION\n");
		if (copy_from_user(&data, (void __user *)arg, sizeof(data)))
			return -EFAULT;
		g_k3dh->cal_data.x = data.x;
		g_k3dh->cal_data.y = data.y;
		g_k3dh->cal_data.z = data.z;
		printk(KERN_INFO "[K3DH] cal data (%d,%d,%d)\n",  g_k3dh->cal_data.x, g_k3dh->cal_data.y, g_k3dh->cal_data.z);
		break;
	case K3DH_IOCTL_GET_CALIBRATION:
		printk(KERN_INFO "[K3DH] K3DH_IOCTL_GET_CALIBRATION\n");
		data.x = g_k3dh->cal_data.x;
		data.y = g_k3dh->cal_data.y;
		data.z = g_k3dh->cal_data.z;
		if (copy_to_user((void __user *)arg, &data, sizeof(data)))
			return -EFAULT;
		break;
	case K3DH_IOCTL_DO_CALIBRATION:
		printk(KERN_INFO "[K3DH] K3DH_IOCTL_DO_CALIBRATION\n");
		k3dh_do_calibration();
		data.x = g_k3dh->cal_data.x;
		data.y = g_k3dh->cal_data.y;
		data.z = g_k3dh->cal_data.z;
		if (copy_to_user((void __user *)arg, &data, sizeof(data)))
			return -EFAULT;
		break;
	case K3DH_IOCTL_READ_ACCEL_XYZ:
		printk(KERN_INFO "[K3DH] K3DH_IOCTL_READ_ACCEL_XYZ\n");
		err = k3dh_read_accel_xyz(&data);
		if (err)
			break;
		if (copy_to_user((void __user *)arg, &data, sizeof(data)))
			return -EFAULT;
		break;
	default:
		err = -EINVAL;
		break;
	}

	return err;
}

static int k3dh_suspend(struct device *dev)
{
	unsigned char acc_data[2] = {0};

#ifdef USES_MOVEMENT_RECOGNITION
	if (g_k3dh->movement_recog_flag == ON) {
		printk(KERN_INFO "[K3DH] [%s] LOW_PWR_MODE.\n", __FUNCTION__);
		acc_data[0] = CTRL_REG1;
		acc_data[1] = LOW_PWR_MODE;
		if(k3dh_acc_i2c_write(acc_data, 2) !=0)
			printk(KERN_ERR "[%s] Change to Low Power Mode is failed\n",__FUNCTION__);
	} else
#endif
	{
		acc_data[0] = CTRL_REG1;
		acc_data[1] = PM_OFF;
		if(k3dh_acc_i2c_write(acc_data, 2) !=0)
			printk(KERN_ERR "[%s] Change to Suspend Mode is failed\n",__FUNCTION__);
	}

	printk(KERN_INFO "[K3DH] [%s] K3DH !!suspend mode!!\n",__FUNCTION__);
	return 0;
}

static int k3dh_resume(struct device *dev)
{
	unsigned char acc_data[2] = {0};

	acc_data[0] = CTRL_REG1;
	acc_data[1] = g_k3dh->ctrl_reg1_shadow;
	if(k3dh_acc_i2c_write(acc_data, 2) !=0)
		printk(KERN_ERR "[%s] Change to Normal Mode is failed\n",__FUNCTION__);

	printk(KERN_INFO "[K3DH] [%s] K3DH !!resume mode!! : ctrl_reg1_shadow = 0x%x\n",__FUNCTION__, g_k3dh->ctrl_reg1_shadow);
	return 0;
}

static const struct file_operations k3dh_fops = {
	.owner = THIS_MODULE,
	.open = k3dh_open,
	.release = k3dh_close,
	.unlocked_ioctl = k3dh_ioctl,
};

static ssize_t k3dh_fs_read(struct device *dev, struct device_attribute *attr, char *buf)
{
	int count = 0;
	struct k3dh_acc acc = { 0, };

	if (g_k3dh->state & ACC_ENABLED)
	{
		acc.x = g_acc.x;
		acc.y = g_acc.y;
		acc.z = g_acc.z;
	}
	else
	{
		k3dh_read_accel_xyz(&acc);
	}

	count = sprintf(buf,"%d,%d,%d\n", acc.x, acc.y, acc.z );

	return count;
}

static ssize_t k3dh_calibration_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int err=1;

	if ((g_k3dh->cal_data.x  == 0) &&
		(g_k3dh->cal_data.y == 0) &&
		(g_k3dh->cal_data.z == 0))
		   err = 0;
	return snprintf(buf, PAGE_SIZE, "%d, %d, %d, %d\n",
		err,
		g_k3dh->cal_data.x,
		g_k3dh->cal_data.y,
		g_k3dh->cal_data.z);
}

static ssize_t k3dh_calibration_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int err = -1;
	int64_t enable;

	err =  strict_strtoll(buf, 10, &enable);
	if (err < 0) {
		pr_info("k3dh_calibration Error.\n");
		return size;
	}
	pr_info("k3dh_calibration enable = %d\n", (int)enable);
	if (enable >= 1) {
		err = k3dh_do_calibration_fs(0);
		if (err != 0)
			pr_info("k3dh_calibration fail.\n");
		else
			pr_info("k3dh_calibration success.\n");
	} else {
		err = k3dh_do_calibration_fs(1);
		if (err != 0)
			pr_info("k3dh_erase fail.\n");
		else
			pr_info("k3dh_erase success.\n");
	}

	return size;
}

#ifdef USES_MOVEMENT_RECOGNITION
static ssize_t k3dh_accel_reactive_alert_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	int onoff = OFF, err = 0, ctrl_reg = 0;
	bool factory_test = false;
	struct k3dh_acc raw_data;
	u8 thresh1 = 0, thresh2 = 0;
	unsigned char acc_data[2] = {0};

	if (sysfs_streq(buf, "1"))
		onoff = ON;
	else if (sysfs_streq(buf, "0"))
		onoff = OFF;
	else if (sysfs_streq(buf, "2")) {
		onoff = ON;
		factory_test = true;
		printk(KERN_INFO "[K3DH] [%s] factory_test = %d\n", __FUNCTION__, factory_test);
	} else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	if (onoff == ON && g_k3dh->movement_recog_flag == OFF) {
		printk(KERN_INFO "[K3DH] [%s] reactive alert is on.\n", __FUNCTION__);
		g_k3dh->interrupt_state = 0; /* Init interrupt state.*/

		if (!(g_k3dh->state & ACC_ENABLED)) {
			acc_data[0] = CTRL_REG1;
			acc_data[1] = FASTEST_MODE;
			if(k3dh_acc_i2c_write(acc_data, 2) !=0)
			{
				printk(KERN_ERR "[%s] Change to Fastest Mode is failed\n",__FUNCTION__);
				ctrl_reg = CTRL_REG1;
				goto err_i2c_write;
			}

			/* trun on time, T = 7/odr ms */
			usleep_range(10000, 10000);
		}
		enable_irq(g_k3dh->irq);
		if (device_may_wakeup(&g_k3dh->client->dev))
			enable_irq_wake(g_k3dh->irq);
		/* Get x, y, z data to set threshold1, threshold2. */
		err = k3dh_read_accel_xyz(&raw_data);
		printk(KERN_INFO "[K3DH] [%s] raw x = %d, y = %d, z = %d\n",
			__FUNCTION__, raw_data.x, raw_data.y, raw_data.z);
		if (err < 0) {
			pr_err("%s: k3dh_accel_read_xyz failed\n",
				__func__);
			goto exit;
		}
		if (!(g_k3dh->state & ACC_ENABLED)) {
			acc_data[0] = CTRL_REG1;
			acc_data[1] = LOW_PWR_MODE; /* Change to 50Hz*/
			if(k3dh_acc_i2c_write(acc_data, 2) !=0){
				printk(KERN_ERR "[%s] Change to Low Power Mode is failed\n",__FUNCTION__);
				ctrl_reg = CTRL_REG1;
				goto err_i2c_write;
			}
		}
		/* Change raw data to threshold value & settng threshold */
		thresh1 = (MAX(ABS(raw_data.x), ABS(raw_data.y))
				+ DYNAMIC_THRESHOLD)/16;
		if (factory_test == true)
			thresh2 = 0; /* for z axis */
		else
			thresh2 = (ABS(raw_data.z) + DYNAMIC_THRESHOLD2)/16;
		printk(KERN_INFO "[K3DH] [%s] threshold1 = 0x%x, threshold2 = 0x%x\n",
			__FUNCTION__, thresh1, thresh2);
		acc_data[0] = INT1_THS;
		acc_data[1] = thresh1;
		if(k3dh_acc_i2c_write(acc_data, 2) !=0){
			ctrl_reg = INT1_THS;
			goto err_i2c_write;
		}
		acc_data[0] = INT2_THS;
		acc_data[1] = thresh2;
		if(k3dh_acc_i2c_write(acc_data, 2) !=0){
			ctrl_reg = INT2_THS;
			goto err_i2c_write;
		}

		/* INT enable */
		acc_data[0] = CTRL_REG3;
		acc_data[1] = DEFAULT_CTRL3_SETTING;
		if(k3dh_acc_i2c_write(acc_data, 2) !=0){
			ctrl_reg = CTRL_REG3;
			goto err_i2c_write;
		}

		g_k3dh->movement_recog_flag = ON;
	} else if (onoff == OFF && g_k3dh->movement_recog_flag == ON) {
		printk(KERN_INFO "[K3DH] [%s] reactive alert is off.\n", __FUNCTION__);

		/* INT disable */
		acc_data[0] = CTRL_REG3;
		acc_data[1] = PM_OFF;
		if(k3dh_acc_i2c_write(acc_data, 2) !=0){
			ctrl_reg = CTRL_REG3;
			goto err_i2c_write;
		}
		if (device_may_wakeup(&g_k3dh->client->dev))
			disable_irq_wake(g_k3dh->irq);
		disable_irq_nosync(g_k3dh->irq);
		/* return the power state */
		acc_data[0] = CTRL_REG1;
		acc_data[1] = g_k3dh->ctrl_reg1_shadow;
		if(k3dh_acc_i2c_write(acc_data, 2) !=0){
			printk(KERN_ERR "[%s] Change to shadow status : 0x%x\n",__FUNCTION__, g_k3dh->ctrl_reg1_shadow);
			ctrl_reg = CTRL_REG1;
			goto err_i2c_write;
		}
		g_k3dh->movement_recog_flag = OFF;
		g_k3dh->interrupt_state = 0; /* Init interrupt state.*/
	}
	return count;
err_i2c_write:
	pr_err("%s: i2c write ctrl_reg = 0x%d failed(err=%d)\n",
				__func__, ctrl_reg, err);
exit:
	return ((err < 0) ? err : -err);
}

static ssize_t k3dh_accel_reactive_alert_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%d\n", g_k3dh->interrupt_state);
}
#endif

static ssize_t name_read(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_DEV_NAME);
}

static ssize_t vendor_read(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_DEV_VENDOR);
}

static DEVICE_ATTR(calibration, S_IRUGO | S_IWUSR | S_IWGRP, k3dh_calibration_show, k3dh_calibration_store);
static DEVICE_ATTR(raw_data, S_IRUGO, k3dh_fs_read, NULL);
#ifdef USES_MOVEMENT_RECOGNITION
static DEVICE_ATTR(reactive_alert, S_IRUGO | S_IWUSR | S_IWGRP,
	k3dh_accel_reactive_alert_show,
	k3dh_accel_reactive_alert_store);
#endif
static DEVICE_ATTR(name, S_IRUSR | S_IRGRP, name_read, NULL);
static DEVICE_ATTR(vendor, S_IRUSR | S_IRGRP, vendor_read, NULL);

static struct device_attribute *accelerometer_attrs[] = {
	&dev_attr_calibration,
	&dev_attr_raw_data,
#ifdef USES_MOVEMENT_RECOGNITION
	&dev_attr_reactive_alert,
#endif
	&dev_attr_name,
	&dev_attr_vendor,
	NULL,
};

/////////////////////////////////////////////////////////////////////////////////////

static ssize_t poll_delay_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return 0;
}


static ssize_t poll_delay_store(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
	int64_t new_delay;
	int err;

	err = strict_strtoll(buf, 10, &new_delay);
	if (err < 0)
		return err;

	return size;
}

static ssize_t acc_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", (g_k3dh->state & ACC_ENABLED) ? 1 : 0);
}


static ssize_t acc_enable_store(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
	bool new_value;

	if (sysfs_streq(buf, "1"))
		new_value = true;
	else if (sysfs_streq(buf, "0"))
		new_value = false;
	else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	mutex_lock(&g_k3dh->power_lock);

	ACCDBG("[BMA222] new_value = %d, old state = %d\n", new_value, (g_k3dh->state & ACC_ENABLED) ? 1 : 0);

	if (new_value && !(g_k3dh->state & ACC_ENABLED)) {
		g_k3dh->state |= ACC_ENABLED;
#if defined(CONFIG_SENSORS_ACC_FILTER)
	g_k3dh->reset_filter = true;
#endif
	} else if (!new_value && (g_k3dh->state & ACC_ENABLED)) {
		g_k3dh->state = 0;
	}
	mutex_unlock(&g_k3dh->power_lock);
	return size;
}

static DEVICE_ATTR(poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,   poll_delay_show, poll_delay_store);

static struct device_attribute dev_attr_acc_enable =
	__ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
	       acc_enable_show, acc_enable_store);

static struct attribute *acc_sysfs_attrs[] = {
	&dev_attr_acc_enable.attr,
	&dev_attr_poll_delay.attr,
	NULL
};

static struct attribute_group acc_attribute_group = {
	.attrs = acc_sysfs_attrs,
};

#ifdef USES_MOVEMENT_RECOGNITION
static irqreturn_t k3dh_accel_interrupt_thread(int irq\
	, void *k3dh_data_p)
{
#ifdef DEBUG_REACTIVE_ALERT
	u8 int1_src_reg = 0, int2_src_reg = 0;
// [HSS][TEMP]
#if 1
	struct k3dh_acc raw_data;
#endif
#endif
	unsigned char acc_data[2] = {0};

#ifndef DEBUG_REACTIVE_ALERT
	/* INT disable */
	acc_data[0] = CTRL_REG3;
	acc_data[1] = PM_OFF;
	if(k3dh_acc_i2c_write(acc_data, 2) !=0)
	{
		printk(KERN_ERR "[%s] i2c write ctrl_reg3 failed\n",__FUNCTION__);
	}
#else
	acc_data[0] = INT1_SRC;
	if(k3dh_acc_i2c_read(acc_data, 1) < 0)
	{
		printk(KERN_ERR "[%s] i2c read int1_src failed\n",__FUNCTION__);
	}
	int1_src_reg = acc_data[0];
	if (int1_src_reg < 0)
	{
		printk(KERN_ERR "[%s] read int1_src failed\n",__FUNCTION__);
	}
	printk(KERN_INFO "[K3DH] [%s] interrupt source reg1 = 0x%x\n", __FUNCTION__, int1_src_reg);

	acc_data[0] = INT2_SRC;
	if(k3dh_acc_i2c_read(acc_data, 1) < 0)
	{
		printk(KERN_ERR "[%s] i2c read int2_src failed\n",__FUNCTION__);
	}
	int2_src_reg = acc_data[0];
	if (int1_src_reg < 0)
	{
		printk(KERN_ERR "[%s] read int2_src failed\n",__FUNCTION__);
	}
	printk(KERN_INFO "[K3DH] [%s] interrupt source reg2 = 0x%x\n", __FUNCTION__, int2_src_reg);
#endif

	g_k3dh->interrupt_state = 1;
	wake_lock_timeout(&g_k3dh->reactive_wake_lock, msecs_to_jiffies(2000));
	printk(KERN_INFO "[K3DH] [%s] irq is handled\n", __FUNCTION__);

	return IRQ_HANDLED;
}
#endif

///////////////////////////////////////////////////////////////////////////////////

void k3dh_shutdown(struct i2c_client *client)
{
	unsigned char acc_data[2] = {0};

	acc_data[0] = CTRL_REG1;
	acc_data[1] = PM_OFF;
	if(k3dh_acc_i2c_write(acc_data, 2) !=0)
		pr_err("%s: pm_off failed\n", __func__);
}

static int k3dh_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct k3dh_data *k3dh;
	struct input_dev *input_dev;
	int err, tempvalue;
#ifdef USES_MOVEMENT_RECOGNITION
	unsigned char acc_data[2] = {0};
#endif
	printk(KERN_INFO "[K3DH] %s\n",__FUNCTION__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s: i2c functionality check failed!\n", __func__);
		err = -ENODEV;
		goto exit;
	}

	k3dh = kzalloc(sizeof(struct k3dh_data), GFP_KERNEL);
	if (k3dh == NULL) {
		dev_err(&client->dev, "failed to allocate memory for module data\n");
		err = -ENOMEM;
		goto exit;
	}

	printk(KERN_INFO "[K3DH] [%s] slave addr = %x\n", __func__, client->addr);

	/* read chip id */
	tempvalue = WHO_AM_I;
	err = i2c_master_send(client, (char*)&tempvalue, 1);
	if(err < 0)
	{
		printk(KERN_ERR "k3dh_probe : i2c_master_send [%d]\n", err);
		goto err_sensor_info_check;
	}

	err = i2c_master_recv(client, (char*)&tempvalue, 1);
	if(err < 0)
	{
		printk(KERN_ERR "k3dh_probe : i2c_master_recv [%d]\n", err);
		goto err_sensor_info_check;
	}

	if((tempvalue&0x00FF) == 0x0033)  // changed for K3DM.
	{
		printk(KERN_INFO "KR3DM I2C driver registered 0x%x!\n", tempvalue);
	}
	else
	{
		printk(KERN_ERR "KR3DM I2C driver not registered 0x%x!\n", tempvalue);
		goto err_sensor_info_check;
	}

	k3dh_client = client;
	k3dh->client = k3dh_client;
	i2c_set_clientdata(client, k3dh);

	/* sensor HAL expects to find /dev/accelerometer */
	k3dh->k3dh_device.minor = MISC_DYNAMIC_MINOR;
	k3dh->k3dh_device.name = "k3dh";
	k3dh->k3dh_device.fops = &k3dh_fops;

	g_k3dh = k3dh;
#if defined(CONFIG_SENSORS_ACC_FILTER)
	hrtimer_init(&g_k3dh->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	g_k3dh->hrtimer_delay_usec = 10526; //Fs=95Hz
	g_k3dh->device_polling_delay = ns_to_ktime( (g_k3dh->hrtimer_delay_usec) * USEC_PER_MSEC);
	g_k3dh->timer.function = k3dh_acc_timer_func;

	INIT_WORK(&g_k3dh->read_work, k3dh_acc_read_work_func);
	g_k3dh->read_workqueue = create_workqueue("k3dh_acc_read_wq");
	if (!g_k3dh->read_workqueue) {
		err = -ENOMEM;
		printk(KERN_ERR "cannot create workqueue read_wq: %d\n", err);
	goto err_wq_create;
	}
	/* Accelerometer Software Filter */
	g_k3dh->reset_filter = true;
	g_k3dh->enabled_filter = true;
	printk(KERN_INFO "[K3DH] Enabled Accelerometer Software Filtering!\n");
#endif
	mutex_init(&g_k3dh->read_lock);
	mutex_init(&g_k3dh->write_lock);
	mutex_init(&g_k3dh->power_lock);
	atomic_set(&g_k3dh->opened, 0);

	err = misc_register(&k3dh->k3dh_device);
	if (err) {
		pr_err("%s: misc_register failed\n", __FILE__);
		goto err_misc_register;
	}

	/* allocate lightsensor-level input_device */
	input_dev = input_allocate_device();
	if (!input_dev) {
		printk(KERN_ERR "%s: could not allocate input device\n", __func__);
		err = -ENOMEM;
		goto err_input_allocate_device;
	}
	input_set_drvdata(input_dev, g_k3dh);
	input_dev->name = "accelerometer_sensor";

	set_bit(EV_REL, input_dev->evbit);
	/* 32768 == 1g, range -4g ~ +4g */
	/* acceleration x-axis */
	input_set_capability(input_dev, EV_REL, REL_X);
	input_set_abs_params(input_dev, REL_X, -256, 256, 0, 0);
	/* acceleration y-axis */
	input_set_capability(input_dev, EV_REL, REL_Y);
	input_set_abs_params(input_dev, REL_Y, -256, 256, 0, 0);
	/* acceleration z-axis */
	input_set_capability(input_dev, EV_REL, REL_Z);
	input_set_abs_params(input_dev, REL_Z, -256, 256, 0, 0);

	printk(KERN_INFO "[K3DH] registering sensor-level input device\n");

	err = input_register_device(input_dev);
	if (err < 0) {
		printk(KERN_ERR "%s: could not register input device\n", __func__);
		goto err_input_register_device;
	}
	g_k3dh->acc_input_dev = input_dev;

	err = sysfs_create_group(&input_dev->dev.kobj,&acc_attribute_group);
	if (err) {
		printk(KERN_ERR "Creating k3dh attribute group failed");
		goto err_sysfs_create_group;
	}

	/* initialized sensor cal data */
	g_k3dh->cal_data.x=0;
	g_k3dh->cal_data.y=0;
	g_k3dh->cal_data.z=0;

#ifdef USES_MOVEMENT_RECOGNITION
	g_k3dh->pdata = g_k3dh->client->dev.platform_data;
	g_k3dh->movement_recog_flag = OFF;
	/* wake lock init for accelerometer sensor */
	wake_lock_init(&g_k3dh->reactive_wake_lock, WAKE_LOCK_SUSPEND,
			"reactive_wake_lock");
	acc_data[0] = INT1_THS;
	acc_data[1] = DEFAULT_THRESHOLD;
	if(k3dh_acc_i2c_write(acc_data, 2) !=0)
	{
		printk(KERN_ERR "[%s] i2c write int1_ths failed\n",__FUNCTION__);
		goto err_request_irq;
	}

	acc_data[0] = INT1_DURATION;
	acc_data[1] = MOVEMENT_DURATION;
	if(k3dh_acc_i2c_write(acc_data, 2) !=0)
	{
		printk(KERN_ERR "[%s] i2c write int1_duration failed\n",__FUNCTION__);
		goto err_request_irq;
	}

	acc_data[0] = INT1_CFG;
	acc_data[1] = DEFAULT_INTERRUPT_SETTING;
	if(k3dh_acc_i2c_write(acc_data, 2) !=0)
	{
		printk(KERN_ERR "[%s] i2c write int1_cfg failed\n",__FUNCTION__);
		goto err_request_irq;
	}

	acc_data[0] = INT2_THS;
	acc_data[1] = DEFAULT_THRESHOLD;
	if(k3dh_acc_i2c_write(acc_data, 2) !=0)
	{
		printk(KERN_ERR "[%s] i2c write int2_ths failed\n",__FUNCTION__);
		goto err_request_irq;
	}

	acc_data[0] = INT2_DURATION;
	acc_data[1] = MOVEMENT_DURATION;
	if(k3dh_acc_i2c_write(acc_data, 2) !=0)
	{
		printk(KERN_ERR "[%s] i2c write int1_duration failed\n",__FUNCTION__);
		goto err_request_irq;
	}
	acc_data[0] = INT2_CFG;
	acc_data[1] = DEFAULT_INTERRUPT2_SETTING;
	if(k3dh_acc_i2c_write(acc_data, 2) !=0)
	{
		printk(KERN_ERR "[%s] i2c write ctrl_reg3 failed\n",__FUNCTION__);
		goto err_request_irq;
	}

	g_k3dh->pdata->int_init();
	g_k3dh->irq = gpio_to_irq(g_k3dh->pdata->gpio_acc_int);

	err = request_threaded_irq(g_k3dh->irq, NULL,
		k3dh_accel_interrupt_thread\
		, IRQF_TRIGGER_RISING | IRQF_ONESHOT,\
			"k3dh_accel", g_k3dh);
	if (err < 0) {
		pr_err("%s: can't allocate irq.\n", __func__);
		goto err_request_irq;
	}

	disable_irq(g_k3dh->irq);
	err = device_init_wakeup(&g_k3dh->client->dev, 1);
	if (err) {
		printk(KERN_ERR "wake_lock init fail");
		goto err_device_init_wakeup;
	}
	g_k3dh->interrupt_state = 0;
#endif

	err = sensors_register(&accelerometer_device, NULL,
		accelerometer_attrs, "accelerometer_sensor");
	if (err < 0) {
		printk(KERN_ERR "Could not sensors_register\n");
		goto exit_sensors_register;
	}
	probe_done = PROBE_SUCCESS;
	return 0;

exit_sensors_register:
#ifdef USES_MOVEMENT_RECOGNITION
	device_init_wakeup(&g_k3dh->client->dev, 0);
err_device_init_wakeup:
	free_irq(g_k3dh->irq, g_k3dh);
err_request_irq:
	wake_lock_destroy(&g_k3dh->reactive_wake_lock);
#endif
	sysfs_remove_group(&input_dev->dev.kobj,
		&acc_attribute_group);
err_sysfs_create_group:
	input_unregister_device(input_dev);
err_input_register_device:
	input_free_device(input_dev);
err_input_allocate_device:
	misc_deregister(&k3dh->k3dh_device);
err_misc_register:
	mutex_destroy(&k3dh->read_lock);
	mutex_destroy(&k3dh->write_lock);
	mutex_destroy(&k3dh->power_lock);
#if defined(CONFIG_SENSORS_ACC_FILTER)
	destroy_workqueue(k3dh->read_workqueue);
err_wq_create:
	cancel_work_sync(&k3dh->read_work);
	hrtimer_cancel(&k3dh->timer);
#endif
err_sensor_info_check:
	kfree(k3dh);
exit:
	return err;
}

static int k3dh_remove(struct i2c_client *client)
{
	struct k3dh_data *k3dh = i2c_get_clientdata(client);

#ifdef USES_MOVEMENT_RECOGNITION
	wake_lock_destroy(&g_k3dh->reactive_wake_lock);
#endif

	sysfs_remove_group(&k3dh->acc_input_dev->dev.kobj, &acc_attribute_group);

	input_unregister_device(k3dh->acc_input_dev);

#ifdef USES_MOVEMENT_RECOGNITION
	device_init_wakeup(&g_k3dh->client->dev, 0);
	free_irq(g_k3dh->irq, g_k3dh);
#endif

	misc_deregister(&k3dh->k3dh_device);
	mutex_destroy(&k3dh->read_lock);
	mutex_destroy(&k3dh->write_lock);
	mutex_destroy(&k3dh->power_lock);
#if defined(CONFIG_SENSORS_ACC_FILTER)
	destroy_workqueue(k3dh->read_workqueue);
	cancel_work_sync(&k3dh->read_work);
	hrtimer_cancel(&k3dh->timer);
#endif
	kfree(k3dh);
	g_k3dh = NULL;

	return 0;
}

static const struct i2c_device_id k3dh_id[] = {
	{ "k3dh", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, k3dh_id);

static struct i2c_driver k3dh_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "k3dh",
	},
	.id_table = k3dh_id,
	.probe = k3dh_probe,
	.shutdown = k3dh_shutdown,
	.remove = k3dh_remove,
	.suspend	= k3dh_suspend,
	.resume	= k3dh_resume,
};


static int __init k3dh_init(void)
{
	probe_done = PROBE_FAIL;

	printk(KERN_INFO "[K3DH] %s\n",__FUNCTION__);

	return i2c_add_driver(&k3dh_driver);
}

static void __exit k3dh_exit(void)
{
	printk(KERN_INFO "[K3DH] %s\n",__FUNCTION__);

	i2c_del_driver(&k3dh_driver);
}

module_init(k3dh_init);
module_exit(k3dh_exit);

MODULE_DESCRIPTION("k3dh accelerometer driver");
MODULE_AUTHOR("STMicroelectronics");
MODULE_LICENSE("GPL");
