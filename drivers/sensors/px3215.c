/*
 * This file is part of the PX3215C sensor driver.
 * Chip is proximity sensor only.
 *
 * Contact: YC Hou <yc.hou@liteonsemi.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 *
 * Filename: px3215.c
 *
 * Summary:
 *	PX3215C sensor dirver.
 *
 * Modification History:
 * Date     By       Summary
 * -------- -------- -------------------------------------------------------
 * 06/28/11 YC		 Original Creation (Test version:1.0)
 * 06/14/12 YC		 Implement px3215 driver v1.9 final version for SS.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/string.h>
#include <linux/gpio.h>
#include <linux/px3215.h>
#include <linux/workqueue.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include "sensors_head.h"


//#define PX3215C_DRV_NAME	"px3215"
#define PX3215C_DRV_NAME		"dyna"
#define DRIVER_VERSION		"1.9"

#define PX3215C_NUM_CACHABLE_REGS	18
#define OFFSET_ARRAY_LENGTH		10

#define PX3215C_MODE_COMMAND	0x00
#define PX3215C_MODE_SHIFT	(0)
#define PX3215C_MODE_MASK	0x07

#define	AL3212_PX_LSB		0x0e
#define	AL3212_PX_MSB		0x0f
#define	AL3212_PX_LSB_MASK	0x0f
#define	AL3212_PX_MSB_MASK	0x3f

#define PX3215C_OBJ_COMMAND	0x0f
#define PX3215C_OBJ_MASK		0x80
#define PX3215C_OBJ_SHIFT	(7)

#define PX3215C_INT_COMMAND	0x01
#define PX3215C_INT_SHIFT	(1)
#define PX3215C_INT_MASK		0x03
#define PX3215C_INT_PMASK		0x02

#define PX3215C_INT_CLEAR_MANNER	0x02
#define PX3215C_INT_CLEAR_MANNER_SHIFT	(0)
#define PX3215C_INT_CLEAR_MANNER_MASK		0x01

#define PX3215C_PX_LTHL			0x2a
#define PX3215C_PX_LTHL_SHIFT	(0)
#define PX3215C_PX_LTHL_MASK		0x03

#define PX3215C_PX_LTHH			0x2b
#define PX3215C_PX_LTHH_SHIFT	(0)
#define PX3215C_PX_LTHH_MASK		0xff

#define PX3215C_PX_HTHL			0x2c
#define PX3215C_PX_HTHL_SHIFT	(0)
#define PX3215C_PX_HTHL_MASK		0x03

#define PX3215C_PX_HTHH			0x2d
#define PX3215C_PX_HTHH_SHIFT	(0)
#define PX3215C_PX_HTHH_MASK		0xff

#define PX3215C_PX_CONFIGURE		0x20
#define PX3215C_PX_CONFIGURE_SHIFT	(0)
#define PX3215C_PX_CONFIGURE_MASK	0xff

#define PX3215C_PX_LEDWAITING		0x24
#define PX3215C_PX_LEDWAITING_SHIFT	(0)
#define PX3215C_PX_LEDWAITING_MASK	0xff

#define PX3215C_PX_CALIBL		0x28
#define PX3215C_PX_CALIBL_SHIFT	(0)
#define PX3215C_PX_CALIBL_MASK	0x01

#define PX3215C_PX_CALIBH		0x29
#define PX3215C_PX_CALIBH_SHIFT	(0)
#define PX3215C_PX_CALIBH_MASK	0xff

#define PROX_AVG_COUNT	10
#define PROX_MAX			1023
#define PROX_MIN			0

#define CHIP_DEV_NAME	"PX3315"
#define CHIP_DEV_VENDOR	"LITEON"

// [HSS]It should be tuned!!
#define PX_PROX_DEFAULT_ADC 100
#define PX_PROX_DEFAULT_THREL 0x168 // 360
#define PX_PROX_DEFAULT_THREH 0x21c // 540

#define PX_PROX_CAL_THREL 0x168 // 0x12c [HSS] It can be changed
#define PX_PROX_CAL_THREH 0x21c // 0x1fe

#define LSC_DBG
#ifdef LSC_DBG
#define LDBG(s,args...)	{printk("LDBG: func [%s], line [%d], ",__func__,__LINE__); printk(s,## args);}
#else
#define LDBG(s,args...) {}
#endif

struct px3215_data {
	struct i2c_client *client;
	struct mutex lock;
	u8 reg_cache[PX3215C_NUM_CACHABLE_REGS];
	u8 power_state_before_suspend;
	int	gpio;
	int irq;
	struct input_dev *input;
	struct work_struct work_prox_avg;
	struct hrtimer prox_avg_timer;
	struct workqueue_struct *wq_avg;
	ktime_t prox_polling_time;
	int avg[3];
	int prox_avg_enable;
	int offset_value;
	int cal_result;
	int threshold_high;
	bool offset_cal_high;
};

static struct i2c_client *this_client;

static u8 px3215_reg[PX3215C_NUM_CACHABLE_REGS] =
	{0x00,0x01,0x02,0x0a,0x0b,0x0e,0x0f,
	 0x20,0x21,0x22,0x23,0x24,0x28,0x29,0x2a,0x2b,0x2c,0x2d};

#define ADD_TO_IDX(addr,idx)	{														\
									int i;												\
									for(i = 0; i < PX3215C_NUM_CACHABLE_REGS; i++)		\
									{													\
										if (addr == px3215_reg[i])						\
										{												\
											idx = i;									\
											break;										\
										}												\
									}													\
								}

/*
 * register access helpers
 */

#if defined(CONFIG_MACH_ROY)
extern unsigned int board_hw_revision;
#endif

static int __px3215_read_reg(struct i2c_client *client,
			       u32 reg, u8 mask, u8 shift)
{
	struct px3215_data *data = i2c_get_clientdata(client);
	u8 idx = 0x12;

	ADD_TO_IDX(reg,idx)
	return (data->reg_cache[idx] & mask) >> shift;
}

static int __px3215_write_reg(struct i2c_client *client,
				u32 reg, u8 mask, u8 shift, u8 val)
{
	struct px3215_data *data = i2c_get_clientdata(client);
	int ret = 0;
	u8 tmp;
	u8 idx = 0x12;
	ADD_TO_IDX(reg,idx)
	if (idx > 0x2D)
		return -EINVAL;

//	mutex_lock(&data->lock);

	tmp = data->reg_cache[idx];
	tmp &= ~mask;
	tmp |= val << shift;

	ret = i2c_smbus_write_byte_data(this_client, reg, tmp);
	if (!ret)
		data->reg_cache[idx] = tmp;

//	mutex_unlock(&data->lock);
	return ret;
}

/*
 * internally used functions
 */

static int px3215_get_calib(struct i2c_client *client)
{
	int lsb, msb;
	lsb = __px3215_read_reg(client, PX3215C_PX_CALIBL,
				PX3215C_PX_CALIBL_MASK, PX3215C_PX_CALIBL_SHIFT);
	msb = __px3215_read_reg(client, PX3215C_PX_CALIBH,
				PX3215C_PX_CALIBH_MASK, PX3215C_PX_CALIBH_SHIFT);
	return ((msb << 1) | lsb);
}

static int px3215_set_calib(struct i2c_client *client, int val)
{
	int lsb, msb, err;

	msb = val >> 1;
	lsb = val & PX3215C_PX_CALIBL_MASK;
	err = __px3215_write_reg(client, PX3215C_PX_CALIBL,
		PX3215C_PX_CALIBL_MASK, PX3215C_PX_CALIBL_SHIFT, lsb);
	if (err)
		return err;
	err = __px3215_write_reg(client, PX3215C_PX_CALIBH,
		PX3215C_PX_CALIBH_MASK, PX3215C_PX_CALIBH_SHIFT, msb);
	return err;
}

/* mode */
static int px3215_get_mode(struct i2c_client *client)
{
	int ret;
	ret = __px3215_read_reg(client, PX3215C_MODE_COMMAND,
			PX3215C_MODE_MASK, PX3215C_MODE_SHIFT);
	return ret;
}

static int px3215_set_mode(struct i2c_client *client, int mode)
{
	int ret;
	ret = __px3215_write_reg(client, PX3215C_MODE_COMMAND,
				PX3215C_MODE_MASK, PX3215C_MODE_SHIFT, mode);
	return ret;
}

/* PX low threshold */
static int px3215_get_plthres(struct i2c_client *client)
{
	int lsb, msb;
	lsb = __px3215_read_reg(client, PX3215C_PX_LTHL,
				PX3215C_PX_LTHL_MASK, PX3215C_PX_LTHL_SHIFT);
	msb = __px3215_read_reg(client, PX3215C_PX_LTHH,
				PX3215C_PX_LTHH_MASK, PX3215C_PX_LTHH_SHIFT);
	return ((msb << 2) | lsb);
}

static int px3215_set_plthres(struct i2c_client *client, int val)
{
	int lsb, msb, err;

	msb = val >> 2;
	lsb = val & PX3215C_PX_LTHL_MASK;
	err = __px3215_write_reg(client, PX3215C_PX_LTHL,
		PX3215C_PX_LTHL_MASK, PX3215C_PX_LTHL_SHIFT, lsb);
	if (err)
		return err;
	err = __px3215_write_reg(client, PX3215C_PX_LTHH,
		PX3215C_PX_LTHH_MASK, PX3215C_PX_LTHH_SHIFT, msb);
	return err;
}

/* PX high threshold */
static int px3215_get_phthres(struct i2c_client *client)
{
	int lsb, msb;
	lsb = __px3215_read_reg(client, PX3215C_PX_HTHL,
				PX3215C_PX_HTHL_MASK, PX3215C_PX_HTHL_SHIFT);
	msb = __px3215_read_reg(client, PX3215C_PX_HTHH,
				PX3215C_PX_HTHH_MASK, PX3215C_PX_HTHH_SHIFT);
	return ((msb << 2) | lsb);
}

static int px3215_set_phthres(struct i2c_client *client, int val)
{
	int lsb, msb, err;

	msb = val >> 2;
	lsb = val & PX3215C_PX_HTHL_MASK;
	err = __px3215_write_reg(client, PX3215C_PX_HTHL,
		PX3215C_PX_HTHL_MASK, PX3215C_PX_HTHL_SHIFT, lsb);
	if (err)
		return err;
	err = __px3215_write_reg(client, PX3215C_PX_HTHH,
		PX3215C_PX_HTHH_MASK, PX3215C_PX_HTHH_SHIFT, msb);

	return err;
}

static int px3215_set_configure(struct i2c_client *client, int val)
{
	int err;

	err = __px3215_write_reg(client, PX3215C_PX_CONFIGURE,
		PX3215C_PX_CONFIGURE_MASK,
		PX3215C_PX_CONFIGURE_SHIFT, val);

	return err;
}

static int px3215_set_ledwaiting(struct i2c_client *client, int val)
{
	int err;

	err = __px3215_write_reg(client, PX3215C_PX_LEDWAITING,
		PX3215C_PX_LEDWAITING_MASK,
		PX3215C_PX_LEDWAITING_SHIFT, val);

	return err;
}

static int px3215_get_object(struct i2c_client *client, int lock)
{
	/* struct px3215_data *data = i2c_get_clientdata(client); */
	int val;
	/*if (!lock)	mutex_lock(&data->lock);*/
	val = i2c_smbus_read_byte_data(client, PX3215C_OBJ_COMMAND);
	val &= PX3215C_OBJ_MASK;
	/*if (!lock)	mutex_unlock(&data->lock);*/

	return val >> PX3215C_OBJ_SHIFT;
}

static int px3215_get_intstat(struct i2c_client *client)
{
	int val;
	val = i2c_smbus_read_byte_data(client, PX3215C_INT_COMMAND);
	val &= PX3215C_INT_MASK;
	return val >> PX3215C_INT_SHIFT;
}

static int px3215_set_intstat(struct i2c_client *client, int val)
{
	int err;

	err = __px3215_write_reg(client, PX3215C_INT_COMMAND,
		PX3215C_INT_PMASK,
		PX3215C_INT_SHIFT, val);

	return err;
}

static int px3215_get_px_value(struct i2c_client *client)
{
	struct px3215_data *data = i2c_get_clientdata(client);
	int lsb, msb;
	mutex_lock(&data->lock);
	lsb = i2c_smbus_read_byte_data(this_client, AL3212_PX_LSB);
	if (lsb < 0) {
		mutex_unlock(&data->lock);
		return lsb;
	}
	msb = i2c_smbus_read_byte_data(this_client, AL3212_PX_MSB);
	mutex_unlock(&data->lock);
	if (msb < 0)
		return msb;

	return (u32)(((msb & AL3212_PX_MSB_MASK) << 4) | (lsb & AL3212_PX_LSB_MASK));
}

static int px3215_set_int_clear_manner(struct i2c_client *client, int val)
{
	int err;

	err = __px3215_write_reg(client, PX3215C_INT_CLEAR_MANNER,
		PX3215C_INT_CLEAR_MANNER_MASK,
		PX3215C_INT_CLEAR_MANNER_SHIFT, val);

	return err;
}

/*
 * sysfs layer
 */
static int px3215_input_init(struct px3215_data *data)
{
    struct input_dev *dev;
    int err;
    dev = input_allocate_device();
    if (!dev) {
        return -ENOMEM;
    }
    dev->name = "proximity_sensor";
    dev->id.bustype = BUS_I2C;
   /*input_set_capability(dev, EV_ABS, ABS_MISC);
    input_set_capability(dev, EV_ABS, ABS_RUDDER);*/
    input_set_capability(dev, EV_ABS, ABS_DISTANCE);
	input_set_abs_params(dev, ABS_DISTANCE, 0, 1, 0, 0);
    input_set_drvdata(dev, data);
    err = input_register_device(dev);
    if (err < 0) {
        input_free_device(dev);
        return err;
    }
    data->input = dev;

    return 0;
}

static void px3215_input_fini(struct px3215_data *data)
{
    struct input_dev *dev = data->input;
    input_unregister_device(dev);
    input_free_device(dev);
}

static void prox_work_func_avg(struct work_struct *work)
{
	struct px3215_data *data = container_of(work, struct px3215_data,
		work_prox_avg);

	u16 proximity_value = 0;
	int min = 0, max = 0, avg = 0;
	int i = 0;

	for (i = 0; i < PROX_AVG_COUNT; i++) {
		proximity_value = px3215_get_px_value(data->client);
		if (proximity_value > PROX_MIN) {
			if (proximity_value > PROX_MAX)
				proximity_value = PROX_MAX;
			avg += proximity_value;
			if (!i)
				min = proximity_value;
			if (proximity_value < min)
				min = proximity_value;
			if (proximity_value > max)
				max = proximity_value;
		} else {
			proximity_value = PROX_MIN;
		}
		msleep(40);
	}
	avg /= i;
	data->avg[0] = min;
	data->avg[1] = avg;
	data->avg[2] = max;
}

static enum hrtimer_restart prox_timer_func(struct hrtimer *timer)
{
	struct px3215_data *data = container_of(timer, struct px3215_data,
		prox_avg_timer);
	queue_work(data->wq_avg, &data->work_prox_avg);
	hrtimer_forward_now(&data->prox_avg_timer, data->prox_polling_time);

	return HRTIMER_RESTART;
}

static int proximity_open_calibration(struct px3215_data  *data)
{
	struct file *cal_filp = NULL;
	int err = 0;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open("/efs/prox_cal",
		O_RDONLY, S_IRUGO | S_IWUSR | S_IWGRP);
	if (IS_ERR(cal_filp)) {
		pr_err("%s: Can't open calibration file\n", __func__);
		set_fs(old_fs);
		err = PTR_ERR(cal_filp);
		goto done;
	}

	err = cal_filp->f_op->read(cal_filp,
		(char *)&data->offset_value,
			sizeof(int), &cal_filp->f_pos);
	if (err != sizeof(int)) {
		pr_err("%s: Can't read the cal data from file\n", __func__);
		err = -EIO;
	}

	pr_info("%s: (%d)\n", __func__,
		data->offset_value);

	filp_close(cal_filp, current->files);
done:
	set_fs(old_fs);
	return err;
}

static int proximity_adc_read(struct px3215_data *data)
{
	int sum[OFFSET_ARRAY_LENGTH];
	int i = OFFSET_ARRAY_LENGTH-1;
	int avg = 0;
	int min = 0;
	int max = 0;
	int total = 0;

	do {
		msleep(50);
		sum[i] = px3215_get_px_value(data->client);
		if (i == OFFSET_ARRAY_LENGTH-1) {
			min = sum[i];
			max = sum[i];
		} else {
			if (sum[i] < min)
				min = sum[i];
			else if (sum[i] > max)
				max = sum[i];
		}
		total += sum[i];
	} while (i--);

	total -= (min + max);
	avg = (total / (OFFSET_ARRAY_LENGTH - 2));
	pr_info("%s: offset = %d\n", __func__, avg);

	return avg;
}

static int proximity_do_calibrate(struct px3215_data  *data,
			bool do_calib, bool thresh_set)
{
	struct file *cal_filp;
	int err;
	int xtalk_avg = 0;
	mm_segment_t old_fs;

	if (do_calib) {
		if (thresh_set) {
			/* for proximity_thresh_store */
			data->offset_value =
				data->threshold_high;
		} else {
			/* tap offset button */
			/* get offset value */
			xtalk_avg = proximity_adc_read(data);
			if (xtalk_avg < PX_PROX_DEFAULT_ADC) {
			/* do not need calibration */
				data->cal_result = 0;
				err = 0;
				goto no_cal;
			}
			data->offset_value = xtalk_avg;// - PX_PROX_DEFAULT_ADC;
		}
		/* update offest */
		px3215_set_calib(data->client, data->offset_value);

		px3215_set_plthres(data->client,
			PX_PROX_CAL_THREL);
		px3215_set_phthres(data->client,
			PX_PROX_CAL_THREH);
		/* calibration result */
		data->cal_result = 1;
	} else {
		/* tap reset button */
		data->offset_value = 0;
		/* update offest */
		px3215_set_calib(data->client, data->offset_value);

		px3215_set_plthres(data->client,
			PX_PROX_DEFAULT_THREL);
		px3215_set_phthres(data->client,
			PX_PROX_DEFAULT_THREH);
		/* calibration result */
		data->cal_result = 2;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open("/efs/prox_cal",
			O_CREAT | O_TRUNC | O_WRONLY,
			S_IRUGO | S_IWUSR | S_IWGRP);
	if (IS_ERR(cal_filp)) {
		pr_err("%s: Can't open calibration file\n", __func__);
		set_fs(old_fs);
		err = PTR_ERR(cal_filp);
		goto done;
	}

	err = cal_filp->f_op->write(cal_filp,
		(char *)&data->offset_value, sizeof(int),
			&cal_filp->f_pos);
	if (err != sizeof(int)) {
		pr_err("%s: Can't write the cal data to file\n", __func__);
		err = -EIO;
	}

	filp_close(cal_filp, current->files);
done:
	set_fs(old_fs);
no_cal:
	return err;
}

static int proximity_manual_offset(struct px3215_data  *data, u8 change_on)
{
	struct file *cal_filp;
	int err;
	mm_segment_t old_fs;

	data->offset_value = change_on;
	/* update threshold */
	px3215_set_calib(data->client, data->offset_value);

	px3215_set_plthres(data->client,
		data->offset_value);
	px3215_set_phthres(data->client,
		data->offset_value);

	/* calibration result */
	data->cal_result = 1;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open("/efs/prox_cal",
			O_CREAT | O_TRUNC | O_WRONLY,
			S_IRUGO | S_IWUSR | S_IWGRP);
	if (IS_ERR(cal_filp)) {
		pr_err("%s: Can't open calibration file\n", __func__);
		set_fs(old_fs);
		err = PTR_ERR(cal_filp);
		goto done;
	}

	err = cal_filp->f_op->write(cal_filp,
		(char *)&data->offset_value, sizeof(int),
			&cal_filp->f_pos);
	if (err != sizeof(int)) {
		pr_err("%s: Can't write the cal data to file\n", __func__);
		err = -EIO;
	}

	filp_close(cal_filp, current->files);
done:
	set_fs(old_fs);
	return err;
}

/* mode */
static ssize_t px3215_show_mode(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct px3215_data *data = input_get_drvdata(input);
	return sprintf(buf, "%d\n", px3215_get_mode(data->client));
}

static ssize_t px3215_store_mode(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct px3215_data *data = input_get_drvdata(input);
	unsigned long val;
	int ret;

	if ((strict_strtoul(buf, 10, &val) < 0) || (val > 7))
		return -EINVAL;

	ret = px3215_set_mode(data->client, val);

	if (ret < 0)
		return ret;
	return count;
}

/* prox_avg */
static ssize_t px3215_show_avg(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct px3215_data *data = input_get_drvdata(input);
	return sprintf(buf, "%d,%d,%d\n", data->avg[0], data->avg[1],
				data->avg[2]);
}

static ssize_t px3215_store_avg(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct px3215_data *data = input_get_drvdata(input);
	int new_value = 0;

	if (sysfs_streq(buf, "1")) {
		new_value = true;
	} else if (sysfs_streq(buf, "0")) {
		new_value = false;
	} else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}
	if (data->prox_avg_enable == new_value)
		printk("[%s] same status\n", __func__);
	else if (new_value == 1) {
		printk("[%s] starting poll timer, delay %lldns\n",__func__,
		ktime_to_ns(data->prox_polling_time));
		hrtimer_start(&data->prox_avg_timer,
			data->prox_polling_time, HRTIMER_MODE_REL);
		data->prox_avg_enable = 1;
	} else {
		printk("[%s] cancelling prox avg poll timer\n", __func__);
		hrtimer_cancel(&data->prox_avg_timer);
		cancel_work_sync(&data->work_prox_avg);
		data->prox_avg_enable = 0;
	}

	return count;
}

static DEVICE_ATTR(mode,S_IRUGO | S_IWUSR | S_IWGRP,
		   px3215_show_mode, px3215_store_mode);


/* Px data */
static ssize_t px3215_show_pxvalue(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct px3215_data *data = input_get_drvdata(input);

	/* No Px data if power down */
	if (px3215_get_mode(data->client) == 0x00)
		return -EBUSY;

	return sprintf(buf, "%d\n", px3215_get_px_value(data->client));
}

static DEVICE_ATTR(pxvalue,S_IRUGO | S_IWUSR | S_IWGRP, px3215_show_pxvalue, NULL);


/* proximity object detect */
static ssize_t px3215_show_object(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct px3215_data *data = input_get_drvdata(input);
	return sprintf(buf, "%d\n", px3215_get_object(data->client,0));
}

static DEVICE_ATTR(object,S_IRUGO | S_IWUSR | S_IWGRP, px3215_show_object, NULL);


/* Px low threshold */
static ssize_t px3215_show_plthres(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct px3215_data *data = input_get_drvdata(input);
	return sprintf(buf, "%d\n", px3215_get_plthres(data->client));
}

static ssize_t px3215_store_plthres(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct px3215_data *data = input_get_drvdata(input);
	unsigned long val;
	int ret;

	if (strict_strtoul(buf, 10, &val) < 0)
		return -EINVAL;

	ret = px3215_set_plthres(data->client, val);
	if (ret < 0)
		return ret;

	return count;
}

static DEVICE_ATTR(plthres, S_IRUGO | S_IWUSR | S_IWGRP,
		   px3215_show_plthres, px3215_store_plthres);

/* Px high threshold */
static ssize_t px3215_show_phthres(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct px3215_data *data = input_get_drvdata(input);
	return sprintf(buf, "%d\n", px3215_get_phthres(data->client));
}

static ssize_t px3215_store_phthres(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct px3215_data *data = input_get_drvdata(input);
	unsigned long val;
	int ret;

	if (strict_strtoul(buf, 10, &val) < 0)
		return -EINVAL;

	ret = px3215_set_phthres(data->client, val);
	if (ret < 0)
		return ret;

	return count;
}

static DEVICE_ATTR(phthres, S_IRUGO | S_IWUSR | S_IWGRP,
		   px3215_show_phthres, px3215_store_phthres);


#ifdef LSC_DBG
/* engineer mode */
static ssize_t px3215_em_read(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct px3215_data *data = i2c_get_clientdata(client);
	int i;
	u8 tmp;

	for (i = 0; i < ARRAY_SIZE(data->reg_cache); i++)
	{
		mutex_lock(&data->lock);
		tmp = i2c_smbus_read_byte_data(data->client, px3215_reg[i]);
		mutex_unlock(&data->lock);

		printk("Reg[0x%x] Val[0x%x]\n", px3215_reg[i], tmp);
	}

	return 0;
}

static ssize_t px3215_em_write(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct px3215_data *data = i2c_get_clientdata(client);
	u32 addr,val,idx=0;
	int ret = 0;

	sscanf(buf, "%2x%2x", &addr, &val);

	printk("Write [%x] to Reg[%x]...\n",val,addr);
	mutex_lock(&data->lock);

	ret = i2c_smbus_write_byte_data(data->client, addr, val);
	ADD_TO_IDX(addr,idx)
	if (!ret)
		data->reg_cache[idx] = val;

	mutex_unlock(&data->lock);

	return count;
}
static DEVICE_ATTR(em, S_IRUGO | S_IWUSR | S_IWGRP,
				   px3215_em_read, px3215_em_write);
#endif

static ssize_t proximity_enable_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct px3215_data *data = input_get_drvdata(input);
	return sprintf(buf, "%d\n", (px3215_get_mode(data->client)) ? 1 : 0);
}

static ssize_t proximity_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct input_dev *input = to_input_dev(dev);
	struct px3215_data *data = input_get_drvdata(input);
	int enable = simple_strtoul(buf, NULL,10);
	int err = 0;

	printk("%s: enable = %d\n", __func__, enable);

	if(enable)
	{
		input_report_abs(data->input, ABS_DISTANCE, 1);
		input_sync(data->input);
		err = proximity_open_calibration(data);
		if (err < 0 && err != -ENOENT)
			pr_err("%s: proximity_open_offset() failed\n",
				__func__);
		else {
			if (data->cal_result==1) {
				px3215_set_calib(data->client,
					data->offset_value);
				px3215_set_plthres(data->client,
					PX_PROX_CAL_THREL);
				px3215_set_phthres(data->client,
					PX_PROX_CAL_THREH);
			}
		}
		px3215_set_mode(data->client, 2);
		enable_irq(data->irq);
		enable_irq_wake(data->irq);
	} else {
		disable_irq_wake(data->irq);
		disable_irq(data->irq);
		px3215_set_mode(data->client, 0);
	}
	return size;
}


static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
	proximity_enable_show, proximity_enable_store);

static struct attribute *px3215_attributes[] = {
	&dev_attr_mode.attr,
	&dev_attr_object.attr,
	&dev_attr_pxvalue.attr,
	&dev_attr_plthres.attr,
	&dev_attr_phthres.attr,
	&dev_attr_enable.attr,
#ifdef LSC_DBG
	&dev_attr_em.attr,
#endif
	NULL
};

static const struct attribute_group px3215_attr_group = {
	.attrs = px3215_attributes,
};


static DEVICE_ATTR(adc, S_IRUGO | S_IWUSR | S_IWGRP,
	px3215_show_mode, px3215_store_mode);
static DEVICE_ATTR(raw_data, S_IRUGO | S_IWUSR | S_IWGRP,
	px3215_show_pxvalue, NULL);
static DEVICE_ATTR(prox_avg, S_IRUGO | S_IWUSR | S_IWGRP,
	px3215_show_avg, px3215_store_avg);
static DEVICE_ATTR(state, S_IRUGO | S_IWUSR | S_IWGRP,
	px3215_show_pxvalue, NULL);

static ssize_t name_read(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_DEV_NAME);
}

static DEVICE_ATTR(name, S_IRUSR | S_IRGRP, name_read, NULL);

static ssize_t vendor_read(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_DEV_VENDOR);
}

static DEVICE_ATTR(vendor, S_IRUSR | S_IRGRP, vendor_read, NULL);

static ssize_t proximity_cal_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct px3215_data *data = dev_get_drvdata(dev);
	msleep(20);
	return sprintf(buf, "%d,%d\n",
			data->offset_value, px3215_get_phthres(data->client));
}

static ssize_t proximity_cal_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct px3215_data *data = dev_get_drvdata(dev);
	bool do_calib;
	int err;

	if (sysfs_streq(buf, "1")) { /* calibrate cancelation value */
		do_calib = true;
	} else if (sysfs_streq(buf, "0")) { /* reset cancelation value */
		do_calib = false;
	} else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		err = -EINVAL;
		goto done;
	}
	err = proximity_do_calibrate(data, do_calib, false);
	if (err < 0) {
		pr_err("%s: proximity_store_offset() failed\n", __func__);
		goto done;
	} else
		err = size;
done:
	return err;
}

static DEVICE_ATTR(prox_cal, S_IRUGO | S_IWUSR | S_IWGRP,
	proximity_cal_show, proximity_cal_store);

static ssize_t proximity_cal2_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct px3215_data *data = dev_get_drvdata(dev);
	u8 change_on;
	int err;

	if (sysfs_streq(buf, "1")) /* change hi threshold by -2 */
		change_on = -2;
	else if (sysfs_streq(buf, "2")) /*change hi threshold by +4 */
		change_on = 4;
	else if (sysfs_streq(buf, "3")) /*change hi threshold by +8 */
		change_on = 8;
	else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		err = -EINVAL;
		goto done;
	}
	err = proximity_manual_offset(data, change_on);
	if (err < 0) {
		pr_err("%s: proximity_store_offset() failed\n", __func__);
		goto done;
	}
done:
	return size;
}

static DEVICE_ATTR(prox_cal2, S_IRUGO | S_IWUSR | S_IWGRP,
	NULL, proximity_cal2_store);

static ssize_t proximity_thresh_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct px3215_data *data = dev_get_drvdata(dev);
	int thresh_hi = 0;

	msleep(20);
	thresh_hi = px3215_get_phthres(data->client);
	pr_info("%s: THRESHOLD = %d\n", __func__, thresh_hi);

	return sprintf(buf, "prox_threshold = %d\n", thresh_hi);
}

static ssize_t proximity_thresh_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct px3215_data *data = dev_get_drvdata(dev);
	long thresh_value = 0;
	int err = 0;

	err = strict_strtol(buf, 10, &thresh_value);
	if (unlikely(err < 0)) {
		pr_err("%s, kstrtoint failed.", __func__);
		goto done;
	}

	err = px3215_set_phthres(data->client, thresh_value);
	if (err < 0) {
		pr_err("%s: thresh_store failed\n", __func__);
	}
done:
	return size;
}

// [HSS] perrmission will be changed
static DEVICE_ATTR(prox_thresh, S_IRUGO | S_IWUGO,
	proximity_thresh_show, proximity_thresh_store);

static ssize_t prox_offset_pass_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct px3215_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", data->cal_result);
}

static DEVICE_ATTR(prox_offset_pass, S_IRUGO | S_IWUSR | S_IWGRP,
	prox_offset_pass_show, NULL);

static struct device_attribute *proximity_attrs[] = {
	&dev_attr_adc,
	&dev_attr_raw_data,
	&dev_attr_prox_avg,
	&dev_attr_state,
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_prox_cal,
	&dev_attr_prox_cal2,
	&dev_attr_prox_thresh,
	&dev_attr_prox_offset_pass,
	NULL,
};

static int px3215_init_client(struct i2c_client *client)
{
	struct px3215_data *data = i2c_get_clientdata(client);
	int i;

	/* read all the registers once to fill the cache.
	 * if one of the reads fails, we consider the init failed */
	for (i = 0; i < ARRAY_SIZE(data->reg_cache); i++) {
		int v = i2c_smbus_read_byte_data(client, px3215_reg[i]);
		if (v < 0)
			return -ENODEV;

		data->reg_cache[i] = v;
	}

	/* set defaults */
	px3215_set_mode(client, 0);
	msleep(20);
	px3215_set_plthres(client, PX_PROX_DEFAULT_THREL);
	px3215_set_phthres(client, PX_PROX_DEFAULT_THREH);
	px3215_set_configure(client, 0x79);
	px3215_set_ledwaiting(client, 0x07);
	px3215_set_int_clear_manner(client, 0x01);
	px3215_set_mode(client, 2);
	msleep(20);

	return 0;
}

/*
 * I2C layer
 */

static irqreturn_t px3215_irq(int irq, void *data_)
{
	struct px3215_data *data = data_;
	int Pval;
	mutex_lock(&data->lock);

	Pval = px3215_get_object(data->client,1);
	printk("%s\n", Pval ? "obj near":"obj far");
	input_report_abs(data->input, ABS_DISTANCE, !Pval);
	input_sync(data->input);
	px3215_set_intstat(data->client, 0x01);

	mutex_unlock(&data->lock);

	return IRQ_HANDLED;
}

static int px3215_probe(struct i2c_client *client,
				    const struct i2c_device_id *id)
{
	printk("[PX3215] probe\n");
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct px3215_platform_data *pdata = client->dev.platform_data;
	struct px3215_data *data;
	struct device *proximity_device = NULL;
	int err = 0;
	int error = 0;
	this_client = client;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE))
		return -EIO;
	data = kzalloc(sizeof(struct px3215_data), GFP_KERNEL);
	if (!data)
	{
		pr_err("%s: allocate data fail\n", __func__);
		err = -ENOMEM;
		goto exit_alloc_data;
	}

	dev_set_name(&client->dev, client->name);
	data->gpio = pdata->ps_vout_gpio;
	data->client = client;
	data->irq = client->irq;
	i2c_set_clientdata(client, data);
	mutex_init(&data->lock);

	/* initialize the PX3215C chip */
	err = px3215_init_client(this_client);
	if (err)
	{
		pr_err("%s: chip init fail\n", __func__);
		goto exit_init_client;
	}
	err = px3215_input_init(data);
	if (err)
	{
		pr_err("%s: input device init fail\n", __func__);
		goto exit_input_device_init;
	}
	/* register sysfs hooks */
	err = sysfs_create_group(&data->input->dev.kobj, &px3215_attr_group);
	if (err)
	{
		pr_err("%s: could not create sysfs group\n", __func__);
		goto exit_sysfs_create_group;
	}

	/* INT Settings */
	if (pdata->hw_setup)
		pdata->hw_setup();
	data->irq = gpio_to_irq(data->gpio);
	if (data->irq < 0) {
		err = data->irq;
		pr_err("%s: Failed to convert GPIO %u to IRQ [errno=%d]",
				__func__, data->gpio, err);
		goto exit_setup_irq;
	}
	err = request_threaded_irq(data->irq, NULL, px3215_irq,
				 IRQF_TRIGGER_FALLING,
				 "px3215", data);
	if (err) {
		pr_err("%s: request_irq failed for px3215\n", __func__);
		goto exit_setup_irq;
	}
	/*irq_set_irq_wake(data->irq, 1);*/
	disable_irq(data->irq);

	/*initialize workqueue and timer for prox_avg calculation*/
	data->wq_avg = create_singlethread_workqueue("prox_wq_avg");
	if (!data->wq_avg) {
		err = -ENOMEM;
		pr_err("%s: could not create workqueue\n", __func__);
		goto err_create_avg_workqueue;
	}

	INIT_WORK(&data->work_prox_avg, prox_work_func_avg);
	data->prox_avg_enable = 0;

	hrtimer_init(&data->prox_avg_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	data->prox_polling_time = ns_to_ktime(2000 * NSEC_PER_MSEC);
	data->prox_avg_timer.function = prox_timer_func;

	error = sensors_register(&proximity_device, data, proximity_attrs,
						"proximity_sensor");
	if (error < 0) {
		pr_err("%s: could not register proximity sensor device(%d).\n",
					__func__, error);
		goto exit_sensors_register;
	}

	/* set initial proximity value as 1 */
	input_report_abs(data->input, ABS_DISTANCE, 1);
	input_sync(data->input);

	dev_info(&client->dev, "PX3215C driver version %s enabled\n", DRIVER_VERSION);
	return 0;

exit_sensors_register:
	destroy_workqueue(data->wq_avg);
err_create_avg_workqueue:
	free_irq(data->irq, data);
exit_setup_irq:
	sysfs_remove_group(&data->input->dev.kobj, &px3215_attr_group);
exit_sysfs_create_group:
	px3215_input_fini(data);
exit_input_device_init:
exit_init_client:
	kfree(data);
exit_alloc_data:
	return err;
}

static int px3215_remove(struct i2c_client *client)
{
	struct px3215_data *data = i2c_get_clientdata(client);
	free_irq(data->irq, data);

	sysfs_remove_group(&data->input->dev.kobj, &px3215_attr_group);
	px3215_set_mode(client, 0);
	kfree(i2c_get_clientdata(client));
	return 0;
}


#define px3215_suspend	NULL
#define px3215_resume		NULL

static const struct i2c_device_id px3215_id[] = {
	//{ "px3215", 0 },
	{ "dyna", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, px3215_id);

static struct i2c_driver px3215_driver = {
	.driver = {
		.name	= PX3215C_DRV_NAME,
		.owner	= THIS_MODULE,
	},
	.suspend = px3215_suspend,
	.resume	= px3215_resume,
	.probe	= px3215_probe,
	.remove	= px3215_remove,
	.id_table = px3215_id,
};

static int __init px3215_init(void)
{
#if defined(CONFIG_MACH_ROY)&& !defined(CONFIG_MACH_NEVIS3G)
#if defined(CONFIG_MACH_ROY_DTV)
	if (board_hw_revision >= 3)
#else
	if (board_hw_revision >= 4)
#endif
	{
		pr_err("%s: hw_revision(%d) : No px3215\n", __func__, board_hw_revision);
		return -1;
	}
#endif
	return i2c_add_driver(&px3215_driver);
}

static void __exit px3215_exit(void)
{
	i2c_del_driver(&px3215_driver);
}

MODULE_AUTHOR("YC Hou, LiteOn-semi corporation.");
MODULE_DESCRIPTION("Test PX3215C driver on mini6410 with SS main chip.");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(DRIVER_VERSION);

module_init(px3215_init);
module_exit(px3215_exit);

