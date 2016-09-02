/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <media/msm_camera.h>
#include <media/v4l2-subdev.h>
#include <mach/gpio.h>
#include <mach/camera.h>

#include <asm/mach-types.h>
#include <mach/vreg.h>
#include <linux/io.h>

#include "msm.h"
#include "msm_ispif.h"
#include "msm_sensor.h"
#include "sec_cam_pmic.h"

#include "s5k5ccgx.h"
#include "s5k5ccgx_regs.h"


#ifdef CONFIG_LOAD_FILE
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#endif

#ifndef CONFIG_LOAD_FILE
#define S5K5CCGX_USE_BURSTMODE
#endif

#ifdef S5K5CCGX_USE_BURSTMODE

#define S5K5CCGX_WRITE_LIST(A)\
	s5k5ccgx_sensor_burst_write(A, (sizeof(A) / sizeof(A[0])), #A);
#else

#define S5K5CCGX_WRITE_LIST(A)\
	s5k5ccgx_sensor_write_list(A, (sizeof(A) / sizeof(A[0])), #A);

#endif


static int s5k5ccgx_sensor_read(unsigned short subaddr, unsigned short *data)
{
	int ret;
	unsigned char buf[2];
	uint16_t saddr = s5k5ccgx_s_ctrl.sensor_i2c_addr >> 1;
	struct i2c_msg msg = { saddr, 0, 2, buf };

	buf[0] = (subaddr >> 8);
	buf[1] = (subaddr & 0xFF);

	ret = i2c_transfer(s5k5ccgx_s_ctrl.sensor_i2c_client->client->adapter, &msg, 1) == 1 ? 0 : -EIO;
	if (ret == -EIO)
		goto error;

	msg.flags = I2C_M_RD;

	ret = i2c_transfer(s5k5ccgx_s_ctrl.sensor_i2c_client->client->adapter, &msg, 1) == 1 ? 0 : -EIO;
	if (ret == -EIO)
		goto error;

	*data = ((buf[0] << 8) | buf[1]);
	/* [Arun c]Data should be written in Little Endian in parallel mode;
	   So there is no need for byte swapping here */
	/*data = *(unsigned long *)(&buf); */
error:
	return ret;
}



static int s5k5ccgx_sensor_write(unsigned short subaddr, unsigned short val)
{
	int rc = 0;
	uint16_t saddr = s5k5ccgx_s_ctrl.sensor_i2c_addr >> 1;
	unsigned char send_buf[4];
	send_buf[0] = subaddr >> 8;
	send_buf[1] = subaddr;
	send_buf[2] = val >> 8;
	send_buf[3] = val;
	struct i2c_msg msg = { saddr,	0, 4, send_buf };

// 	printk("[PGH] on write func s5k5ccgx_s_ctrl.sensor_i2c_addr : %x\n",  s5k5ccgx_s_ctrl.sensor_i2c_addr);

	rc = i2c_transfer(s5k5ccgx_s_ctrl.sensor_i2c_client->client->adapter, &msg, 1);
	return rc;
}


static int s5k5ccgx_sensor_write_list(const unsigned int *list, int size,
				      char *name)
{
	int ret = 0, i;
	unsigned short subaddr;
	unsigned short value;

	printk("s5k5ccgx_sensor_write_list\n");
#ifdef CONFIG_LOAD_FILE
	ret = s5k5ccgx_regs_table_write(name);
#else
	printk("s5k5ccgx_sensor_write_list : %s start\n", name);

	for (i = 0; i < size; i++) {
		/*CAM_DEBUG("[PGH] %x
		   %x\n", list[i].subaddr, list[i].value); */
		subaddr = (list[i] >> 16);	/*address */
		value = (list[i] & 0xFFFF);	/*value */
		if (subaddr == 0xffff) {
			CAM_DEBUG("SETFILE DELAY : %dms\n", value);
			msleep(value);
		} else {
			if (s5k5ccgx_sensor_write(subaddr, value) < 0) {
				pr_info("[S5K5CCGX]sensor_write_list fail\n");
				return -EIO;
			}
		}
	}
	CAM_DEBUG("s5k5ccgx_sensor_write_list : %s end\n", name);
#endif
	return ret;
}


//#ifdef S5K5CCGX_USE_BURSTMODE
#define BURST_MODE_BUFFER_MAX_SIZE 3000
unsigned char s5k5ccgx_buf_for_burstmode[BURST_MODE_BUFFER_MAX_SIZE];
static int s5k5ccgx_sensor_burst_write(const unsigned int *list, int size,
				       char *name)
{
	int i = 0;
	int idx = 0;
	int err = -EINVAL;
	int retry = 0;
	unsigned short subaddr = 0, next_subaddr = 0;
	unsigned short value = 0;
	uint16_t saddr = s5k5ccgx_s_ctrl.sensor_i2c_addr >> 1;

	struct i2c_msg msg = { saddr,
		0, 0, s5k5ccgx_buf_for_burstmode
	};
	CAM_DEBUG("%s", name);

I2C_RETRY:
	idx = 0;
	for (i = 0; i < size; i++) {
		if (idx > (BURST_MODE_BUFFER_MAX_SIZE - 10)) {
			/*pr_info("[S5K5CCGX]s5k5ccgx_buf_for_burstmode
			    overflow will occur!!!\n");*/
			return err;
		}

		/*address */
		subaddr = (list[i] >> 16);
		if (subaddr == 0x0F12)	/*address */
			next_subaddr = list[i + 1] >> 16;
		value = (list[i] & 0xFFFF);	/*value */

		switch (subaddr) {
		case 0x0F12:
			/* make and fill buffer for burst mode write */
			if (idx == 0) {
				s5k5ccgx_buf_for_burstmode[idx++] = 0x0F;
				s5k5ccgx_buf_for_burstmode[idx++] = 0x12;
			}
			s5k5ccgx_buf_for_burstmode[idx++] = value >> 8;
			s5k5ccgx_buf_for_burstmode[idx++] = value & 0xFF;
			/*write in burstmode */
			if (next_subaddr != 0x0F12) {
				msg.len = idx;
				err =
				    i2c_transfer(s5k5ccgx_s_ctrl.sensor_i2c_client->client->adapter, &msg,
						 1) == 1 ? 0 : -EIO;
				/*pr_info("s5k5ccgx_sensor_burst_write,
				   idx = %d\n",idx); */
				idx = 0;
			}
			break;
		case 0xFFFF:
			msleep(value);
			break;
		default:
			/* Set Address */
			idx = 0;
			err = s5k5ccgx_sensor_write(subaddr, value);
			break;
		}
	}
	if (unlikely(err < 0)) {
		printk("[S5K5CCGX]%s: register set failed. try again.\n",
			__func__);
		if ((retry++) < 10)
			goto I2C_RETRY;
		return err;
	}
	/*CAM_DEBUG(" end!\n"); */
	return 0;
}
//#endif /*S5K5CCGX_USE_BURSTMODE */

#ifdef CONFIG_LOAD_FILE
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

static char *s5k5ccgx_regs_table;
static int s5k5ccgx_regs_table_size;


void s5k5ccgx_regs_table_init(void)
{
	struct file *filp;
	char *dp;
	long l;
	loff_t pos;
	int ret;
	mm_segment_t fs = get_fs();

	CAM_DEBUG("%s %d\n", __func__, __LINE__);
	set_fs(get_ds());
		filp = filp_open("/data/s5k5ccgx_regs.h", O_RDONLY, 0);

	if (IS_ERR(filp)) {
		CAM_DEBUG("[S5K5CCGX]file open error\n");
		return;
	}
	l = filp->f_path.dentry->d_inode->i_size;
	CAM_DEBUG("l = %ld\n", l);
	dp = kmalloc(l, GFP_KERNEL);
	if (dp == NULL) {
		CAM_DEBUG("[S5K5CCGX]Out of Memory\n");
		filp_close(filp, current->files);
	}
	pos = 0;
	memset(dp, 0, l);
	ret = vfs_read(filp, (char __user *)dp, l, &pos);
	if (ret != l) {
		CAM_DEBUG("[S5K5CCGX]Failed to read file ret = %d\n", ret);
		kfree(dp);
		filp_close(filp, current->files);
	return;
	}
	filp_close(filp, current->files);
	set_fs(fs);

	s5k5ccgx_regs_table = dp;
	s5k5ccgx_regs_table_size = l;
	*((s5k5ccgx_regs_table + s5k5ccgx_regs_table_size) - 1) = '\0';
	/*CAM_DEBUG("s5k5ccgx_regs_table 0x%x, %ld\n", dp, l);*/
}


void s5k5ccgx_regs_table_exit(void)
{
	CAM_DEBUG("%s %d\n", __func__, __LINE__);
	kfree(s5k5ccgx_regs_table);
	s5k5ccgx_regs_table = NULL;

}

static int s5k5ccgx_regs_table_write(char *name)
{
	char *start, *end, *reg;
	unsigned short addr, value;
	unsigned long data, count;
	char data_buf[11];

	addr = value = count = 0;

	//*(reg_buf + 6) = '\0';
	*(data_buf + 10) = '\0';

	if (s5k5ccgx_regs_table == NULL)
	{
		printk("<KEJ>s5k5ccgx_regs_table == NULL ::: s5k5ccgx_regs_table_write \n");
		return 0;
	}
	CAM_DEBUG("write table = s5k5ccgx_regs_table,find string = %s\n", name);
	start = strstr(s5k5ccgx_regs_table, name);
	end = strstr(start, "};");

	while (1) {
		/* Find Address */
		reg = strstr(start, "0x");
		if (reg)
			start = (reg + 10);
		if ((reg == NULL) || (reg > end))
			break;
		/* Write Value to Address */
		if (reg != NULL) {
			//memcpy(reg_buf, reg, 6);
			memcpy(data_buf, reg, 10);
			
			//addr = (unsigned short)simple_strtoul
			//	(reg_buf, NULL, 16);
			data = (unsigned long)simple_strtoul
				(data_buf, NULL, 16);
			
		
			addr = (data>>16);
			value = (data&0xffff);
			
			//printk("[S5K5CCGX]addr 0x%04x, value 0x%04x\n",addr, value);

			if (addr == 0xffff) {
				msleep(value);
				CAM_DEBUG("[S5K5CCGX]delay 0x%04x, value 0x%04x\n",addr, value);
			} else {
			if (s5k5ccgx_sensor_write(addr, value) < 0)	{
					CAM_DEBUG("[S5K5CCGX]%s fail on sensor_write\n",__func__);
				}
			}
	
			
		} else
			CAM_DEBUG("[S5K5CCGX]EXCEPTION! reg value : %c  addr : 0x%x,  value : 0x%x\n",
				*reg, addr, value);
	}

	return 0;
}
#endif


void s5k5ccgx_exif_shutter_speed(void)
{
	unsigned short msb = 0;
	unsigned short lsb = 0;
	unsigned long calValue = 0;

	s5k5ccgx_sensor_write(0xFCFC, 0xD000);	// shutter speed				
	s5k5ccgx_sensor_write(0x002C, 0x7000);
	s5k5ccgx_sensor_write(0x002E, 0x2A14);
	s5k5ccgx_sensor_read(0x0F12, &lsb);
	s5k5ccgx_sensor_read(0x0F12, &msb);

	calValue = (unsigned long)((msb << 16) | lsb);
	s5k5ccgx_exif.shutterspeed = (unsigned short)(calValue == 0 ? 65535 : 400*1000 / calValue);	//guide from sensor company

	printk("s5k5ccgx_exif_shutter_speed = %d , 0x%x\n", s5k5ccgx_exif.shutterspeed, calValue );	


}


void s5k5ccgx_exif_iso(void)
{
	unsigned short rough_iso;	
	unsigned short iso_value;
	
	s5k5ccgx_sensor_write(0xFCFC, 0xD000);	// iso speed rate				
	s5k5ccgx_sensor_write(0x002C, 0x7000);
	s5k5ccgx_sensor_write(0x002E, 0x2A18);
	s5k5ccgx_sensor_read(0x0F12, &rough_iso);

	iso_value = rough_iso;
	if( iso_value < 0x1ff)
		iso_value = 50;
	else if( iso_value < 0x2ff)
		iso_value = 100;
	else if( iso_value < 0x3ff )
		iso_value = 200;
	else
		iso_value = 400;
	
	s5k5ccgx_exif.iso = iso_value;

	printk("s5k5ccgx_exif_iso = %d , rough_iso = 0x%x\n", s5k5ccgx_exif.iso, rough_iso);
}


static int s5k5ccgx_get_exif(int exif_cmd, unsigned short value2)
{
	unsigned short val = 0;

	switch (exif_cmd) {
		
	case EXIF_EXPOSURE_TIME:
		/* shutter speed low */
		s5k5ccgx_exif_shutter_speed();
		val = s5k5ccgx_exif.shutterspeed;
		break;

	case EXIF_ISO:
		s5k5ccgx_exif_iso();
		val = s5k5ccgx_exif.iso;
		break;

	default:
		break;
	}

	return val;
}


unsigned short s5k5ccgx_get_LuxValue(void)
{
	unsigned short	msb, lsb, cur_lux;
	
	s5k5ccgx_sensor_write(0xFCFC, 0xD000);
	s5k5ccgx_sensor_write(0x002C, 0x7000);
	s5k5ccgx_sensor_write(0x002E, 0x2A3C);
	s5k5ccgx_sensor_read(0x0F12, &lsb);
	s5k5ccgx_sensor_read(0x0F12, &msb);

	cur_lux = (msb<<16) | lsb;
	printk("cur_lux : 0x%08x \n", cur_lux);

	return cur_lux;
}


static int s5k5ccgx_get_LowLightCondition(void)
{
	int err = -1;
	unsigned char l_data[2] = { 0, 0 }, h_data[2] = {
	0, 0};
	unsigned int ldata_temp = 0, hdata_temp = 0;

	s5k5ccgx_get_LuxValue();

	CAM_DEBUG(" lowLight:%d", s5k5ccgx_control.lowLight);

	return err;
}


void s5k5ccgx_set_preview_size(int32_t index)
{
	cam_info(" %d -> %d\n", s5k5ccgx_control.settings.preview_size_idx, index);

	if (index != PREVIEW_SIZE_HD && s5k5ccgx_s_ctrl.is_HD_preview == 1) {
		//S5K5CCGX_WRITE_LIST(s5k5ccgx_1280_Preview_Disable);
		printk("DISABLE 1280X720 Preview SIZE \n");
	}
	s5k5ccgx_s_ctrl.is_HD_preview = 0;		
	
	switch (index) {

	case PREVIEW_SIZE_VGA:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_640_480_Preview);
		mdelay(100);
		S5K5CCGX_WRITE_LIST(s5k5ccgx_update_preview_setting);
		mdelay(100);
		break;
		
	case PREVIEW_SIZE_720_D1:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_720_480_Preview);
		mdelay(100);
		S5K5CCGX_WRITE_LIST(s5k5ccgx_update_preview_setting);
		mdelay(100);
		break;

	case PREVIEW_SIZE_WVGA:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_800_480_Preview);
		mdelay(100);
		S5K5CCGX_WRITE_LIST(s5k5ccgx_update_preview_setting);
		mdelay(100);
		break;

	case PREVIEW_SIZE_W960:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_960_720_Preview);
		mdelay(100);
		S5K5CCGX_WRITE_LIST(s5k5ccgx_update_preview_setting);
		mdelay(100);
		break;

	case PREVIEW_SIZE_HD:
		s5k5ccgx_s_ctrl.is_HD_preview = 1;		
		//S5K5CCGX_WRITE_LIST(s5k5ccgx_1280_Preview);
		break;

	case PREVIEW_SIZE_HD_DISABLE:
		//S5K5CCGX_WRITE_LIST(s5k5ccgx_800_Preview);
		break;

	case PREVIEW_SIZE_QCIF:
		//S5K5CCGX_WRITE_LIST(s5k5ccgx_640_Preview);
		break;

	default:
		//S5K5CCGX_WRITE_LIST(s5k5ccgx_640_Preview);
		break;
	}


	s5k5ccgx_control.settings.preview_size_idx = index;
}


void s5k5ccgx_set_picture_size(int32_t index)
{
	cam_info(" %d -> %d\n", s5k5ccgx_control.settings.capture_size, index);

	switch (index) {

	case MSM_V4L2_PICTURE_SIZE_2048x1536_3M:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_capture_2048_1536);
		break;

	case MSM_V4L2_PICTURE_SIZE_1600x1200_2M:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_capture_1600_1200);
		break;

	case MSM_V4L2_PICTURE_SIZE_1280x960_1M:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_capture_1280_960);
		break;

	case MSM_V4L2_PICTURE_SIZE_1024x768_8K:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_capture_1024_768);
		break;

	case MSM_V4L2_PICTURE_SIZE_640x480_VGA:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_capture_640_480);
		break;

	case MSM_V4L2_PICTURE_SIZE_2048x1232_2_4M_WIDE:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_capture_2048_1232);
		break;

	case MSM_V4L2_PICTURE_SIZE_800x480_4K_WIDE:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_capture_800_480);
		break;

	default:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_capture_2048_1536);
		break;
	}

	s5k5ccgx_control.settings.capture_size = index;
}


void s5k5ccgx_set_frame_rate(int32_t fps)
{
	cam_info(" %d", fps);

	switch (fps) {
	case 15:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_fps_15fix);
		break;

	case 20:
		//S5K5CCGX_WRITE_LIST(s5k5ccgx_FPS_20);
		break;

	case 24:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_fps_24fix);
		break;

	case 25:
		//S5K5CCGX_WRITE_LIST(s5k5ccgx_fps_auto_max_25);
		break;

	case 30:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_fps_30fix);
		break;

	default:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_fps_auto);
		break;
	}

	s5k5ccgx_control.settings.fps = fps;
}


static int32_t s5k5ccgx_sensor_setting(int update_type, int rt)
{
	int32_t rc = 0;

	cam_info(" E");

	switch (update_type) {
	case REG_INIT:
		break;

	case UPDATE_PERIODIC:
printk("@@@@@ s5k5ccgx_sensor_setting 11111@@@@@\n");
		msm_sensor_enable_debugfs(s5k5ccgx_control.s_ctrl);
printk("@@@@@ s5k5ccgx_sensor_setting 22222@@@@@\n");
		if (rt == RES_PREVIEW || rt == RES_CAPTURE) {
			CAM_DEBUG(" UPDATE_PERIODIC");

			msleep(30);
/*
			CAM_DEBUG(" start AP MIPI setting");
			if (s5k5ccgx_control.s_ctrl->sensordata->
					sensor_platform_info->
					csi_lane_params != NULL) {
					CAM_DEBUG(" lane_assign ="\
						" 0x%x",
						s5k4ecgx_control.s_ctrl->
						sensordata->
						sensor_platform_info->
						csi_lane_params->
						csi_lane_assign);
					CAM_DEBUG(" lane_mask ="\
						" 0x%x",
						s5k4ecgx_control.s_ctrl->
						sensordata->
						sensor_platform_info->
						csi_lane_params->
						csi_lane_mask);
			}
			mb();
			msleep(20);
*/
			config_csi2 = 1;
			msleep(30);
		}
		break;
	default:
		rc = -EINVAL;
		break;
	}

	return rc;
}


static int s5k5ccgx_set_effect(int effect)
{
	CAM_DEBUG(" %d", effect);

	switch (effect) {
	case CAMERA_EFFECT_OFF:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_effect_off);
		break;

	case CAMERA_EFFECT_MONO:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_effect_mono);
		break;

	case CAMERA_EFFECT_NEGATIVE:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_effect_negative);
		break;

	case CAMERA_EFFECT_SEPIA:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_effect_sepia);
		break;

	default:
		cam_info(" effect : default");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_effect_off);
		return 0;
	}

	s5k5ccgx_control.settings.effect = effect;

	return 0;
}

static int s5k5ccgx_set_whitebalance(int wb)
{
	CAM_DEBUG(" %d", wb);

	switch (wb) {
	case CAMERA_WHITE_BALANCE_AUTO:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_wb_auto);
		break;

	case CAMERA_WHITE_BALANCE_INCANDESCENT:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_wb_incandescent);
		break;

	case CAMERA_WHITE_BALANCE_FLUORESCENT:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_wb_fluorescent);
		break;

	case CAMERA_WHITE_BALANCE_DAYLIGHT:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_wb_daylight);
		break;

	case CAMERA_WHITE_BALANCE_CLOUDY_DAYLIGHT:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_wb_cloudy);
		break;

	default:
		cam_info(" WB : default");
		return 0;
	}

	s5k5ccgx_control.settings.wb = wb;

	return 0;
}

static void s5k5ccgx_check_dataline(int val)
{
	if (val) {
		CAM_DEBUG(" DTP ON");
	//	S5K5CCGX_WRITE_LIST(s5k5ccgx_DTP_init0);
		s5k5ccgx_control.dtpTest = 1;

	} else {
		CAM_DEBUG(" DTP OFF");
		//S5K5CCGX_WRITE_LIST(s5k5ccgx_DTP_stop);
		s5k5ccgx_control.dtpTest = 0;
	}
}

int sensor_get_DTP_Value()
{
	return s5k5ccgx_control.dtpTest;
}

static void s5k5ccgx_set_ae_awb(int lock)
{
	if (lock) {
		CAM_DEBUG(" AE_AWB LOCK");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_ae_lock);
		S5K5CCGX_WRITE_LIST(s5k5ccgx_awb_lock);
	} else {
		CAM_DEBUG(" AE_AWB UNLOCK");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_ae_unlock);
		S5K5CCGX_WRITE_LIST(s5k5ccgx_awb_unlock);
	}
}

static void s5k5ccgx_set_ev(int ev)
{
	CAM_DEBUG(" %d", ev);

	switch (ev) {
	case CAMERA_EV_M4:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_brightness_m_4);
		break;

	case CAMERA_EV_M3:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_brightness_m_3);
		break;

	case CAMERA_EV_M2:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_brightness_m_2);
		break;

	case CAMERA_EV_M1:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_brightness_m_1);
		break;

	case CAMERA_EV_DEFAULT:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_brightness_0);
		break;

	case CAMERA_EV_P1:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_brightness_p_1);
		break;

	case CAMERA_EV_P2:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_brightness_p_2);
		break;

	case CAMERA_EV_P3:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_brightness_p_3);
		break;

	case CAMERA_EV_P4:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_brightness_p_4);
		break;
	default:
		cam_info(" ev : default");
		break;
	}

	s5k5ccgx_control.settings.brightness = ev;
}

static void s5k5ccgx_set_scene_mode(int mode)
{
	CAM_DEBUG(" %d", mode);

	if( s5k5ccgx_control.settings.scenemode == mode) {
		printk("SCENE MODE IS SAME WITH BEFORE");
		return;
	}

    if((mode != CAMERA_SCENE_AUTO) && (mode != CAMERA_SCENE_OFF) )
	{
		CAMDRV_DEBUG("SCENE MODE START, RESET SETTINGS");
		//when scene mode was setted, defualt settings should be set. vendor request
		S5K5CCGX_WRITE_LIST(s5k5ccgx_effect_off);
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_off);
	}

	switch (mode) {
	case CAMERA_SCENE_AUTO:
	case CAMERA_SCENE_OFF:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_off);
		break;

	case CAMERA_SCENE_LANDSCAPE:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_landscape);
		break;

	case CAMERA_SCENE_DAWN:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_dawn);
		break;

	case CAMERA_SCENE_BEACH:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_beach);
		break;

	case CAMERA_SCENE_SUNSET:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_sunset);
		break;

	case CAMERA_SCENE_NIGHT:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_nightshot);
		break;

	case CAMERA_SCENE_PORTRAIT:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_portrait);
		break;

	case CAMERA_SCENE_AGAINST_LIGHT:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_backlight);
		break;

	case CAMERA_SCENE_SPORT:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_sports);
		break;

	case CAMERA_SCENE_FALL:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_fall);
		break;

	case CAMERA_SCENE_TEXT:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_text);
		break;

	case CAMERA_SCENE_CANDLE:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_candle);
		break;

	case CAMERA_SCENE_FIRE:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_firework);
		break;

	case CAMERA_SCENE_PARTY:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_party);
		break;

	default:
		cam_info(" scene : default");
		break;
	}
	s5k5ccgx_control.settings.scenemode = mode;
}


static void s5k5ccgx_set_metering(int mode)
{
	CAM_DEBUG(" %d", mode);

	switch (mode) {
	case CAMERA_CENTER_WEIGHT:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_metering_center);
		break;

	case CAMERA_AVERAGE:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_metering_normal); 
		break;

	case CAMERA_SPOT:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_metering_spot);
		break;

	default:
		cam_info(" AE : default");
		break;
	}

	s5k5ccgx_control.settings.metering = mode;
}

static struct msm_cam_clk_info cam_clk_info[] = {
	{"cam_m_clk", MSM_SENSOR_MCLK_24HZ},
};


static int s5k5ccgx_sensor_match_id(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;

	cam_info(" Nothing");

	return rc;
}


static int s5k5ccgx_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;
	int temp = 0;
	unsigned short test_read;
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;

	printk("s5k5ccgx_sensor_power_up E\n");

	/*Power on the LDOs */
	cam_ldo_power_on_1();

	usleep(10);

	if (s_ctrl->clk_rate != 0)
		cam_clk_info->clk_rate = s_ctrl->clk_rate;

	rc = msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
		cam_clk_info, &s_ctrl->cam_clk, ARRAY_SIZE(cam_clk_info), 1);
	if (rc < 0)
		printk(" Mclk enable failed\n");

	usleep(10000);


	/*reset Main cam */
	/*standby Main cam */
	cam_ldo_power_on_2();

	CAM_DEBUG(" X");

	return rc;
}


static int s5k5ccgx_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;
	int temp = 0;
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;

	CAM_DEBUG(" E");

	/*standby Main cam */
	/*reset Main cam */
	cam_ldo_power_off_1();

	/*CAM_MCLK0*/
	msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
		cam_clk_info, &s_ctrl->cam_clk, ARRAY_SIZE(cam_clk_info), 0);

	udelay(WAIT_1MS);

	/*power off the LDOs */
	cam_ldo_power_off_2();

	config_csi2 = 0;
	g_bCameraRunning = false;

#ifdef CONFIG_LOAD_FILE
	s5k5ccgx_regs_table_exit();
#endif
	CAM_DEBUG(" X");

	return rc;
}


void sensor_native_control(void __user *arg)
{
	struct ioctl_native_cmd ctrl_info;
	struct msm_camera_v4l2_ioctl_t *ioctl_ptr = arg;

	if (copy_from_user(&ctrl_info,
		(void __user *)ioctl_ptr->ioctl_ptr,
		sizeof(ctrl_info))) {
		printk("fail copy_from_user!\n");
		goto FAIL_END;
	}

	cam_info("mode : %d \n", ctrl_info.mode);
	

	switch (ctrl_info.mode) {	/* for fast initialising */
	case EXT_CAM_EV:
		printk("EXT_CAM_EV\n");
		if (s5k5ccgx_control.settings.scene == CAMERA_SCENE_BEACH)
			break;
		else {
			s5k5ccgx_set_ev(ctrl_info.value_1);
			break;
		}

	case EXT_CAM_WB:
		printk("EXT_CAM_WB\n");
		s5k5ccgx_set_whitebalance(ctrl_info.value_1);
		s5k5ccgx_control.awb_mode = ctrl_info.value_1;
		break;

	case EXT_CAM_METERING:
		printk("EXT_CAM_METERING\n");
		s5k5ccgx_set_metering(ctrl_info.value_1);
		break;

	case EXT_CAM_EFFECT:
		printk("EXT_CAM_EFFECT START\n");
		s5k5ccgx_set_effect(ctrl_info.value_1);
		printk("EXT_CAM_EFFECT END\n");
		
		break;

	case EXT_CAM_SCENE_MODE:
		printk("EXT_CAM_SCENE_MODE\n");
		s5k5ccgx_set_scene_mode(ctrl_info.value_1);
		break;

	case EXT_CAM_MOVIE_MODE:
		printk("EXT_CAM_MOVIE_MODE\n");
		CAM_DEBUG(" MOVIE mode : %d", ctrl_info.value_1);
		s5k5ccgx_control.cam_mode = ctrl_info.value_1;
		break;

	case EXT_CAM_PREVIEW_SIZE:
		printk("EXT_CAM_PREVIEW_SIZE\n");
		s5k5ccgx_set_preview_size(ctrl_info.value_1);
		break;

	case CFG_SET_PICTURE_SIZE:
		printk("CFG_SET_PICTURE_SIZE\n");
		s5k5ccgx_set_picture_size(ctrl_info.value_1);
		break;

	case EXT_CAM_DTP_TEST:
		s5k5ccgx_check_dataline(ctrl_info.value_1);
		break;

	case EXT_CAM_SET_AE_AWB:
		printk("EXT_CAM_SET_AE_AWB\n");
		s5k5ccgx_set_ae_awb(ctrl_info.value_1);
		break;

	case EXT_CAM_EXIF:
		printk("EXT_CAM_EXIF\n");
		ctrl_info.value_1 = s5k5ccgx_get_exif(ctrl_info.address,
			ctrl_info.value_2);
		break;

	case EXT_CAM_VT_MODE:
		printk("EXT_CAM_VT_MODE\n");
		CAM_DEBUG(" VT mode : %d", ctrl_info.value_1);
		s5k5ccgx_control.vtcall_mode = ctrl_info.value_1;
		break;

	case EXT_CAM_SET_FPS:
		printk("EXT_CAM_SET_FPS\n");
		s5k5ccgx_set_frame_rate(ctrl_info.value_1);
		break;

	case EXT_CAM_SAMSUNG_CAMERA:
		CAM_DEBUG(" SAMSUNG camera : %d", ctrl_info.value_1);
		s5k5ccgx_control.samsungapp = ctrl_info.value_1;
		break;

	default:
		break;
	}

	if (copy_to_user((void __user *)ioctl_ptr->ioctl_ptr,
		  (const void *)&ctrl_info,
			sizeof(ctrl_info))) {
		printk("fail copy_to_user!\n");
		goto FAIL_END;
	}

	return;

FAIL_END:
	printk("Error : can't handle native control\n");

}

static struct v4l2_subdev_info s5k5ccgx_subdev_info[] = {
	{
		.code   = V4L2_MBUS_FMT_YUYV8_2X8,
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt    = 1,
		.order    = 0,
	},
	/* more can be supported, to be added later */
};


void s5k5ccgx_sensor_start_stream(struct msm_sensor_ctrl_t *s_ctrl)
{

#ifdef CONFIG_LOAD_FILE
	printk("[MSM_SENSOR]s5k5ccgx_reg_table_init_start!!");
	s5k5ccgx_regs_table_init();
#endif


	printk(" s5k5ccgx_sensor_start_stream E\n");
	S5K5CCGX_WRITE_LIST(s5k5ccgx_pre_init0);
	S5K5CCGX_WRITE_LIST(s5k5ccgx_init0);
	S5K5CCGX_WRITE_LIST(s5k5ccgx_update_preview_setting);

	s5k5ccgx_control.settings.preview_size_idx = PREVIEW_SIZE_VGA;
	s5k5ccgx_control.settings.capture_size = MSM_V4L2_PICTURE_SIZE_2048x1536_3M;
	s5k5ccgx_control.settings.scenemode = CAMERA_SCENE_AUTO;

	printk("  s5k5ccgx_sensor_start_stream X\n");
}


void s5k5ccgx_sensor_stop_stream(struct msm_sensor_ctrl_t *s_ctrl)
{
	printk(" s5k5ccgx_sensor_stop_stream E\n");
//kk0704.park SHOUD MAKE CODES	S5K4ECGX_WRITE_LIST(s5k4ecgx_stop_stream);
	printk(" s5k5ccgx_sensor_stop_stream X\n");
}

void s5k5ccgx_sensor_preview_mode(struct msm_sensor_ctrl_t *s_ctrl)
{
	printk(" s5k5ccgx_sensor_preview_mode E\n");
	S5K5CCGX_WRITE_LIST(s5k5ccgx_update_preview_setting);
	if(changedPreviewSize)
	s5k5ccgx_set_preview_size(s5k5ccgx_control.settings.preview_size_idx);
	printk("  s5k5ccgx_sensor_preview_mode X\n");
}


void s5k5ccgx_sensor_capture_mode(struct msm_sensor_ctrl_t *s_ctrl)
{	
	unsigned short cur_lux = s5k5ccgx_get_LuxValue();
    changedPreviewSize = false;

	
	if (s5k5ccgx_control.settings.preview_size_idx != PREVIEW_SIZE_VGA )
	{
		if ( (s5k5ccgx_control.settings.capture_size == MSM_V4L2_PICTURE_SIZE_2048x1536_3M) ||
			(s5k5ccgx_control.settings.capture_size == MSM_V4L2_PICTURE_SIZE_1600x1200_2M) ||
			(s5k5ccgx_control.settings.capture_size == MSM_V4L2_PICTURE_SIZE_1280x960_1M) ||
			(s5k5ccgx_control.settings.capture_size == MSM_V4L2_PICTURE_SIZE_1024x768_8K) ||
			(s5k5ccgx_control.settings.capture_size == MSM_V4L2_PICTURE_SIZE_640x480_VGA) )
		{
  		//set preview capture size ratelimit_state
			S5K5CCGX_WRITE_LIST(s5k5ccgx_640_480_Preview);
			mdelay(100);
			S5K5CCGX_WRITE_LIST(s5k5ccgx_update_preview_setting);
			mdelay(100);
 			s5k5ccgx_set_picture_size(s5k5ccgx_control.settings.capture_size);
			changedPreviewSize = true;
		}
	}else
	{
		if ((s5k5ccgx_control.settings.capture_size == MSM_V4L2_PICTURE_SIZE_2048x1232_2_4M_WIDE) ||
			(s5k5ccgx_control.settings.capture_size == MSM_V4L2_PICTURE_SIZE_800x480_4K_WIDE))
		{
  			S5K5CCGX_WRITE_LIST(s5k5ccgx_800_480_Preview);
			mdelay(100);
			S5K5CCGX_WRITE_LIST(s5k5ccgx_update_preview_setting);
			mdelay(100);
 			s5k5ccgx_set_picture_size(s5k5ccgx_control.settings.capture_size);
			changedPreviewSize = true;
		}
	}

	printk(" s5k5ccgx_sensor_capture_mode E\n");
	if(cur_lux > 0xFFFE)
	{
		CAMDRV_DEBUG("HighLight Snapshot!\n");
		printk("HighLight Snapshot!\n");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_highlight_snapshot);
		mdelay(60);
	}
	else if(cur_lux < 0x0023)
	{
		if((s5k5ccgx_control.settings.scenemode == CAMERA_SCENE_NIGHT) 
			||(s5k5ccgx_control.settings.scenemode == CAMERA_SCENE_FIRE) )
		{
			CAMDRV_DEBUG("Night or Firework  Snapshot!\n");
			printk("Night or Firework Snapshot!\n");			
			S5K5CCGX_WRITE_LIST(s5k5ccgx_night_snapshot);	
			mdelay(350);
		}
		else
		{
			CAMDRV_DEBUG("LowLight Snapshot delay ~~~~!\n");
			printk("LowLight Snapshot!\n");
			S5K5CCGX_WRITE_LIST(s5k5ccgx_lowlight_snapshot);
			mdelay(150);
		}
	}
	else
	{
		CAMDRV_DEBUG("Normal Snapshot!\n");
		printk("Normal Snapshot!\n");		
		S5K5CCGX_WRITE_LIST(s5k5ccgx_snapshot);
		mdelay(150);
	}		

	printk(" s5k5ccgx_sensor_capture_mode X\n");

}


static struct v4l2_subdev_video_ops s5k5ccgx_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};


static struct msm_sensor_output_info_t s5k5ccgx_dimensions[] = {
	{
		.x_output = 2048,
		.y_output = 1536,
		.line_length_pclk = 2048,
		.frame_length_lines = 1536,
		.vt_pixel_clk = 9216000,
		.op_pixel_clk = 9216000,
		.binning_factor = 1,
	},
	{
		.x_output = 960,
		.y_output = 720,
		.line_length_pclk = 960,
		.frame_length_lines = 720,
		.vt_pixel_clk = 9216000,
		.op_pixel_clk = 9216000,
		.binning_factor = 1,
	},
};


static struct msm_camera_csi_params s5k5ccgx_csi_params = {
	.data_format = CSI_8BIT,
	.lane_cnt    = 1,
	.lane_assign = 0xe4,
	.dpcm_scheme = 0,
	.settle_cnt  = 0x11,
};


static struct msm_camera_csi_params *s5k5ccgx_csi_params_array[] = {
	&s5k5ccgx_csi_params,
	&s5k5ccgx_csi_params,
};


static struct msm_sensor_output_reg_addr_t s5k5ccgx_reg_addr = {
	.x_output = 640,
	.y_output = 480,
	.line_length_pclk = 640,
	.frame_length_lines = 480,
};


static struct msm_sensor_id_info_t s5k5ccgx_id_info = {
	.sensor_id_reg_addr = 0x0000,  //kk0704.park :: should check
	.sensor_id = 0x05CC,  //kk0704.park :: should check
//	.sensor_version = 0x11,
};

static const struct i2c_device_id s5k5ccgx_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&s5k5ccgx_s_ctrl},
	{}
};


static struct i2c_driver s5k5ccgx_i2c_driver = {
	.id_table = s5k5ccgx_i2c_id,
	.probe = msm_sensor_i2c_probe,
	.driver = {
		   .name = "msm_camera_s5k5ccgx",
	},
};

static struct msm_camera_i2c_client s5k5ccgx_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};


static int __init msm_sensor_init_module(void)
{
	int rc = 0;
	CDBG("S5K5CCGX\n");

	rc = i2c_add_driver(&s5k5ccgx_i2c_driver);

	return rc;
}


static struct msm_camera_i2c_reg_conf s5k5ccgx_capture_settings[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x0D1E},
	{0x0F12, 0xA102},
	{0x002A, 0x0210},
	{0x0F12, 0x0000},
	{0x002A, 0x01F4},
	{0x0F12, 0x0001},
	{0x002A, 0x0212},
	{0x0F12, 0x0001},
	{0x002A, 0x01E8},
	{0x0F12, 0x0001},
	{0x0F12, 0x0001},
};


static struct msm_camera_i2c_reg_conf s5k5ccgx_preview_return_settings[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},

	{0x002A, 0x0D1E},
	{0x0F12, 0x2102},	//70000D1E

	{0x002A, 0x085C},
	{0x0F12, 0x004A}, //0049//#afit_uNoiseIndInDoor_0_
	{0x0F12, 0x004E}, //005F//#afit_uNoiseIndInDoor_1_

	//PREVIEW
	{0x002A, 0x0208},
	{0x0F12, 0x0000},	//REG_TC_GP_ActivePrevConfig
	{0x002A, 0x0210},
	{0x0F12, 0x0000},	//REG_TC_GP_ActiveCapConfig
	{0x002A, 0x020C},
	{0x0F12, 0x0001},	//REG_TC_GP_PrevOpenAfterChange
	{0x002A, 0x01F4},
	{0x0F12, 0x0001},	//REG_TC_GP_NewConfigSync
	{0x002A, 0x020A},
	{0x0F12, 0x0001},	//REG_TC_GP_PrevConfigChanged
	{0x002A, 0x0212},
	{0x0F12, 0x0001},	//REG_TC_GP_CapConfigChanged
	{0x002A, 0x01E8},
	{0x0F12, 0x0000},	//REG_TC_GP_EnableCapture
	{0x0F12, 0x0001},	//REG_TC_GP_EnableCaptureChanged

};


static struct msm_camera_i2c_conf_array s5k5ccgx_confs[] = {
	{&s5k5ccgx_capture_settings[0],ARRAY_SIZE(s5k5ccgx_capture_settings), 0, MSM_CAMERA_I2C_WORD_DATA},
	{&s5k5ccgx_preview_return_settings[0],ARRAY_SIZE(s5k5ccgx_preview_return_settings), 0, MSM_CAMERA_I2C_WORD_DATA},
};


static struct v4l2_subdev_core_ops s5k5ccgx_subdev_core_ops = {
	.s_ctrl = msm_sensor_v4l2_s_ctrl,
	.queryctrl = msm_sensor_v4l2_query_ctrl,
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};


static struct v4l2_subdev_ops s5k5ccgx_subdev_ops = {
	.core = &s5k5ccgx_subdev_core_ops,
	.video  = &s5k5ccgx_subdev_video_ops,
};


static struct msm_sensor_fn_t s5k5ccgx_func_tbl = {
	.sensor_start_stream = s5k5ccgx_sensor_start_stream,
	.sensor_stop_stream = s5k5ccgx_sensor_stop_stream,
	.sensor_preview_mode = s5k5ccgx_sensor_preview_mode,
	.sensor_capture_mode = s5k5ccgx_sensor_capture_mode,
	.sensor_set_fps = NULL,
	.sensor_write_exp_gain = NULL,
	.sensor_write_snapshot_exp_gain = NULL,
	.sensor_csi_setting = msm_sensor_setting1,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = msm_sensor_power_up,
	.sensor_power_down =  msm_sensor_power_down,
	.sensor_match_id = NULL,
	.sensor_adjust_frame_lines = NULL,
	.sensor_get_csi_params = msm_sensor_get_csi_params,
};


static struct msm_sensor_reg_t s5k5ccgx_regs = {
	.default_data_type = MSM_CAMERA_I2C_WORD_DATA,
	.output_settings = &s5k5ccgx_dimensions[0],
	.num_conf = ARRAY_SIZE(s5k5ccgx_confs),
};


static struct msm_sensor_ctrl_t s5k5ccgx_s_ctrl = {
	.msm_sensor_reg = &s5k5ccgx_regs,
	.sensor_i2c_client = &s5k5ccgx_sensor_i2c_client,
	.sensor_i2c_addr = 0x5A,
	.sensor_output_reg_addr = &s5k5ccgx_reg_addr,
	.sensor_id_info = &s5k5ccgx_id_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csic_params = &s5k5ccgx_csi_params_array[0],
	.msm_sensor_mutex = &s5k5ccgx_mut,
	.sensor_i2c_driver = &s5k5ccgx_i2c_driver,
	.sensor_v4l2_subdev_info = s5k5ccgx_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(s5k5ccgx_subdev_info),
	.sensor_v4l2_subdev_ops = &s5k5ccgx_subdev_ops,
	.func_tbl = &s5k5ccgx_func_tbl,
	.clk_rate = MSM_SENSOR_MCLK_24HZ,
};


module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Samsung 3MP YUV sensor driver");
MODULE_LICENSE("GPL v2");
