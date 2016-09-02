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

#include "s5k4ecgx.h"
#if defined(CONFIG_MACH_ARUBASLIM_OPEN)
#include "s5k4ecgx_regs_arubaslim.h"
#elif defined (CONFIG_MACH_HENNESSY_DUOS_CTC)
#include "s5k4ecgx_regs_hennesy.h"
#else
#include "s5k4ecgx_regs.h"
#endif
#define SENSOR_NAME "s5k4ecgx"

#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#ifndef CONFIG_LOAD_FILE
#define S5K4ECGX_USE_BURSTMODE
#endif

#ifdef S5K4ECGX_USE_BURSTMODE
#define S5K4ECGX_WRITE_LIST(A) \
	{\
			s5k4ecgx_sensor_burst_write\
			(A##_EVT1, (sizeof(A##_EVT1) /\
			sizeof(A##_EVT1[0])), #A"_EVT1");\
	}
#else
#define S5K4ECGX_WRITE_LIST(A) \
	{\
			s5k4ecgx_sensor_write_list\
			(A##_EVT1, (sizeof(A##_EVT1) /\
			sizeof(A##_EVT1[0])), #A"_EVT1");\
	}
#endif

static int s5k4ecgx_sensor_read(unsigned short subaddr, unsigned short *data)
{
	int ret;
	unsigned char buf[2];
	uint16_t saddr = s5k4ecgx_s_ctrl.sensor_i2c_addr >> 1;
	struct i2c_msg msg = { saddr, 0, 2, buf };

	buf[0] = (subaddr >> 8);
	buf[1] = (subaddr & 0xFF);

	ret = i2c_transfer(s5k4ecgx_s_ctrl.sensor_i2c_client->client->adapter, &msg, 1) == 1 ? 0 : -EIO;
	if (ret == -EIO)
		goto error;

	msg.flags = I2C_M_RD;

	ret = i2c_transfer(s5k4ecgx_s_ctrl.sensor_i2c_client->client->adapter, &msg, 1) == 1 ? 0 : -EIO;
	if (ret == -EIO)
		goto error;

	*data = ((buf[0] << 8) | buf[1]);
	/* [Arun c]Data should be written in Little Endian in parallel mode;
	   So there is no need for byte swapping here */
	/*data = *(unsigned long *)(&buf); */
error:
	return ret;
}

static int s5k4ecgx_sensor_write(unsigned short subaddr, unsigned short val)
{
	int rc;
	uint16_t saddr = s5k4ecgx_s_ctrl.sensor_i2c_addr >> 1;
	unsigned char send_buf[4];
	send_buf[0] = subaddr >> 8;
	send_buf[1] = subaddr;
	send_buf[2] = val >> 8;
	send_buf[3] = val;
	struct i2c_msg msg = { saddr,	0, 4, send_buf };

// 	printk("[PGH] on write func s5k4ecgx_s_ctrl.sensor_i2c_addr : %x\n",  s5k4ecgx_s_ctrl.sensor_i2c_addr);

	rc = i2c_transfer(s5k4ecgx_s_ctrl.sensor_i2c_client->client->adapter, &msg, 1);
	return rc;
}

static int s5k4ecgx_sensor_write_list(const unsigned int *list, int size,
				      char *name)
{
	int ret = 0;
        int i;
	unsigned short subaddr;
	unsigned short value;

	printk("s5k4ecgx_sensor_write_list\n");
#ifdef CONFIG_LOAD_FILE
	ret = s5k4ecgx_regs_table_write(name);
#else
	printk("s5k4ecgx_sensor_write_list : %s start\n", name);

	for (i = 0; i < size; i++) {
		/*CAM_DEBUG("[PGH] %x
		   %x\n", list[i].subaddr, list[i].value); */
		subaddr = (list[i] >> 16);	/*address */
		value = (list[i] & 0xFFFF);	/*value */
		if (subaddr == 0xffff) {
			CAM_DEBUG("SETFILE DELAY : %dms\n", value);
			msleep(value);
		} else {
			if (s5k4ecgx_sensor_write(subaddr, value) < 0) {
				pr_info("[S5K4ECGX]sensor_write_list fail\n");
				return -EIO;
			}
		}
	}
	CAM_DEBUG("s5k4ecgx_sensor_write_list : %s end\n", name);
#endif
	return ret;
}

#ifdef S5K4ECGX_USE_BURSTMODE
#define BURST_MODE_BUFFER_MAX_SIZE 3000
unsigned char s5k4ecgx_buf_for_burstmode[BURST_MODE_BUFFER_MAX_SIZE];
static int s5k4ecgx_sensor_burst_write(const unsigned int *list, int size,
				       char *name)
{
	int i = 0;
	int idx;
	int err = -EINVAL;
	int retry = 0;
	unsigned short subaddr = 0, next_subaddr = 0;
	unsigned short value = 0;
	uint16_t saddr = s5k4ecgx_s_ctrl.sensor_i2c_addr >> 1;

	struct i2c_msg msg = { saddr,
		0, 0, s5k4ecgx_buf_for_burstmode
	};
	CAM_DEBUG("%s", name);

I2C_RETRY:
	idx = 0;
	for (i = 0; i < size; i++) {
		if (idx > (BURST_MODE_BUFFER_MAX_SIZE - 10)) {
			/*pr_info("[S5K4ECGX]s5k4ecgx_buf_for_burstmode
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
				s5k4ecgx_buf_for_burstmode[idx++] = 0x0F;
				s5k4ecgx_buf_for_burstmode[idx++] = 0x12;
			}
			s5k4ecgx_buf_for_burstmode[idx++] = value >> 8;
			s5k4ecgx_buf_for_burstmode[idx++] = value & 0xFF;
			/*write in burstmode */
			if (next_subaddr != 0x0F12) {
				msg.len = idx;
				err =
				    i2c_transfer(s5k4ecgx_s_ctrl.sensor_i2c_client->client->adapter, &msg,
						 1) == 1 ? 0 : -EIO;
				/*pr_info("s5k4ecgx_sensor_burst_write,
				   idx = %d\n",idx); */
				idx = 0;
			}
			break;
		case 0xFFFF:
			break;
		default:
			/* Set Address */
			idx = 0;
			err = s5k4ecgx_sensor_write(subaddr, value);
			break;
		}
	}
	if (unlikely(err < 0)) {
		printk("[S5K4ECGX]%s: register set failed. try again.\n",
			__func__);
		if ((retry++) < 10)
			goto I2C_RETRY;
		return err;
	}
	/*CAM_DEBUG(" end!\n"); */
	return 0;
}
#endif /*S5K4ECGX_USE_BURSTMODE */

#ifdef CONFIG_LOAD_FILE
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

static char *s5k4ecgx_regs_table;
static int s5k4ecgx_regs_table_size;

void s5k4ecgx_regs_table_init(void)
{
	struct file *filp;
	char *dp;
	long l;
	loff_t pos;
	int ret;
	mm_segment_t fs = get_fs();

	printk("%s %d\n", __func__, __LINE__);

	/*Set the current segment to kernel data segment */
	set_fs(get_ds());

#if defined (CONFIG_MACH_ARUBASLIM_OPEN)
	filp = filp_open("/sdcard/s5k4ecgx_regs_arubaslim.h", O_RDONLY, 0);
#else
	filp = filp_open("/data/s5k4ecgx_regs_arubaslim.h", O_RDONLY, 0);
#endif

	if (IS_ERR(filp)) {
#if defined (CONFIG_MACH_ARUBASLIM_OPEN)
		printk("%s(): Error %ld opening %s\n", __FUNCTION__, -PTR_ERR(filp), "/sdcard/s5k4ecgx_regs_arubaslim.h");
#else
		printk("[S5K4ECGX]file open error\n");
#endif
		return;
	}
	l = filp->f_path.dentry->d_inode->i_size;
	printk("l = %ld\n", l);

	
	dp = vmalloc(l);

	if (dp == NULL) {
		printk("[S5K4ECGX]Out of Memory\n");
		filp_close(filp, current->files);
	}
	
	pos = 0;
	memset(dp, 0, l);
	ret = vfs_read(filp, (char __user *)dp, l, &pos);
	if (ret != l) {
		printk("[S5K4ECGX]Failed to read file ret = %d\n", ret);
		if(dp!=NULL)
		vfree(dp);
		filp_close(filp, current->files);
	return;
	}
	
	filp_close(filp, current->files);
	set_fs(fs);

	s5k4ecgx_regs_table = dp;
	s5k4ecgx_regs_table_size = l;
	*((s5k4ecgx_regs_table + s5k4ecgx_regs_table_size) - 1) = '\0';
	printk("[s5k4ecgx_regs_table] 0x%x, %ld\n", dp, l);
}

void s5k4ecgx_regs_table_exit(void)
{
	printk("%s %d\n", __func__, __LINE__);
	if(s5k4ecgx_regs_table != NULL){
		vfree(s5k4ecgx_regs_table);
		s5k4ecgx_regs_table = NULL;
		s5k4ecgx_regs_table_size = 0;
	}

}

static int s5k4ecgx_regs_table_write(char *name)
{
	char *start, *end, *reg;
	unsigned short addr, value;
	unsigned long data;
	char data_buf[11];

	addr = value = 0;
	printk("<KEJ> s5k4ecgx_regs_table_write \n");
	//*(reg_buf + 6) = '\0';
	*(data_buf + 10) = '\0';

	if (s5k4ecgx_regs_table == NULL){
		printk("<KEJ>s5k4ecgx_regs_table == NULL ::: s5k4ecgx_regs_table_write \n");
		return 0;
		}
	printk("write table = s5k4ecgx_regs_table,find string = %s\n", name);
	start = strstr(s5k4ecgx_regs_table, name);
	if (start == NULL){
		printk("<KEJ>start == NULL ::: start \n");
		return 0;
	}
	end = strstr(start, "};");
	if(end == NULL){
		printk("<KEJ>end == NULL ::: end \n");

		return 0;	

	}

	while (1) {
		/* Find Address */
		if(start >= end)
		break;
		reg = strstr(start, "0x");
		if (reg)
			start = (reg + 10);
		
		if (reg == NULL){
			if(reg > end){
			printk("[s5k4ecgx]write end of %s \n",name);
			break;
			}
			else if(reg < end){
				printk(	"[S5K4ECGX]EXCEPTION! reg value : %c  addr : 0x%x,  value : 0x%x\n",
				*reg, addr, value);
			}
		}
		/* Write Value to Address */
		//memcpy(reg_buf, reg, 6);
		memcpy(data_buf, reg, 10);
		
		//addr = (unsigned short)simple_strtoul
		//	(reg_buf, NULL, 16);
		data = (unsigned long)simple_strtoul
			(data_buf, NULL, 16);
		
		addr = (data>>16);
		value = (data&0xffff);
		
		//printk("[S5K4ECGX]addr 0x%04x, value 0x%04x\n",addr, value);

		if (addr == 0xffff) {
			msleep(value);
			printk(
				"[S5K4ECGX]delay 0x%04x, value 0x%04x\n",
				addr, value);
		} else {
		if (s5k4ecgx_sensor_write(addr, value) < 0)	{
				printk(
					"[S5K4ECGX]%s fail on sensor_write\n",
					__func__);
				return -EIO;
			}
		}

	}
	return 0;
}
#endif

void s5k4ecgx_exif_shutter_speed(void)
{
	unsigned short msb = 0;
	unsigned short lsb = 0;
	unsigned long calValue;

	s5k4ecgx_sensor_write(0x002C, 0x7000);
	s5k4ecgx_sensor_write(0x002E, 0x2BC0);
	s5k4ecgx_sensor_read(0x0F12, (unsigned short *)&lsb);
	s5k4ecgx_sensor_read(0x0F12, (unsigned short *)&msb);

	calValue = (unsigned long)((msb << 16) | lsb);
	s5k4ecgx_exif.shutterspeed = (unsigned short)(calValue == 0 ? 65535 : 400000 / calValue);	//guide from sensor company

	printk("s5k4ecgx_exif_shutter_speed = %d , 0x%x\n", s5k4ecgx_exif.shutterspeed, calValue );	
}

void s5k4ecgx_exif_iso(void)
{

	unsigned short analog_gain;
	unsigned short digital_gain;
	unsigned long iso_value;

	//int exif_iso = 0;
	//int err = 0;
	//short unsigned int r_data[1] = {0};

	s5k4ecgx_sensor_write(0x002C, 0x7000);
	s5k4ecgx_sensor_write(0x002E, 0x2BC4);
	s5k4ecgx_sensor_read(0x0F12, (unsigned short *)&analog_gain);
	s5k4ecgx_sensor_read(0x0F12, (unsigned short *)&digital_gain);

	iso_value =(unsigned long) ( analog_gain * digital_gain ) / 256 / 2;
	if (iso_value < 0xD0)
		s5k4ecgx_exif.iso = 50;
	else if(iso_value < 0x1A0)
		s5k4ecgx_exif.iso = 100;
	else if(iso_value < 0x374)
		s5k4ecgx_exif.iso = 200;
	else
		s5k4ecgx_exif.iso = 400;

	printk("s5k4ecgx_exif_iso = %d , iso_value = 0x%x\n", s5k4ecgx_exif.iso, iso_value);
}

static int s5k4ecgx_get_exif(int exif_cmd, unsigned short value2)
{
	unsigned short val = 0;

	switch (exif_cmd) {
	case EXIF_EXPOSURE_TIME:
		/* shutter speed low */
		s5k4ecgx_exif_shutter_speed();
		val = s5k4ecgx_exif.shutterspeed;
		break;

	case EXIF_ISO:
		s5k4ecgx_exif_iso();
		val = s5k4ecgx_exif.iso;
		break;

	default:
		break;
	}

	return val;
}

static void s5k4ecgx_get_LowLightCondition(void)
{
	unsigned short msb = 0;
	unsigned short lsb = 0;
	unsigned short lightCondition;

	s5k4ecgx_sensor_write(0x002C, 0x7000);
	s5k4ecgx_sensor_write(0x002E, 0x2C18);
	s5k4ecgx_sensor_read(0x0F12, (unsigned short *)&lsb);
	s5k4ecgx_sensor_read(0x0F12, (unsigned short *)&msb);

	lightCondition =((msb << 16) | lsb);
	if (lightCondition < 0x38)
		s5k4ecgx_control.lowLight = 1;
	else
		s5k4ecgx_control.lowLight = 0;

	CAM_DEBUG(" lowLight:%d", s5k4ecgx_control.lowLight);
}

static int s5k4ecgx_get_af_result(void)
{
	//int err = 0;
	int ret = 0;
	//int status = 0;
/*	//kk0704.park SHOUD MAKE CODES
	err = s5k4ecgx_sensor_read(0x8B8B,
			(unsigned short *)&status);
	if (err < 0)
		printk(" i2c read returned error, %d\n", err);
	if ((status & 0x1) == 0x1) {
		cam_info(" AF success");
		ret = 1;
	} else if ((status & 0x1) == 0x0) {
		cam_info(" AF fail");
		ret = 2;
	} else {
		cam_info(" AF move");
		ret = 0;
	}
*/
	return ret;
}

static int s5k4ecgx_get_af_status(void)
{
	//uint16_t ae_data[1] = { 0 };
	//int16_t ersc_data[1] = { 0 };
	//int16_t aescl_data[1] = { 0 };
	//int16_t ae_scl = 0;

	CAM_DEBUG(" E");

	if (s5k4ecgx_control.af_status == 1) {
		s5k4ecgx_control.af_status = s5k4ecgx_get_af_result();

	}

	CAM_DEBUG(" X %d", s5k4ecgx_control.af_status);

	return s5k4ecgx_control.af_status;
}

static int s5k4ecgx_get_flash_status(void)
{
	int flash_status = 0;

	if (((s5k4ecgx_control.flash_mode == CAMERA_FLASH_AUTO)
		&& (s5k4ecgx_control.lowLight))
		|| (s5k4ecgx_control.flash_mode == CAMERA_FLASH_ON)) {
		flash_status = 1;
	}

	CAM_DEBUG(" %d", flash_status);

	return flash_status;
}

static void s5k4ecgx_set_flash(int mode)
{
	//int i = 0;

	cam_info(" %d", mode);

	if (torchonoff > 0) {
		printk(" [TorchOnOFF = %d] Do not control flash!\n",
			torchonoff);
		return;
	}

	if (mode == MOVIE_FLASH) {
		CAM_DEBUG(" MOVIE FLASH ON");
		cam_flash_torch_on(FLASH_MODE_CAMERA);

	} else if (mode == CAPTURE_FLASH) {

		CAM_DEBUG(" CAPTURE FLASH ON");
		cam_flash_main_on(FLASH_MODE_CAMERA);
		
	} else if (mode == FLASH_OFF) {
		if (s5k4ecgx_control.settings.flash_state == CAPTURE_FLASH)
			S5K4ECGX_WRITE_LIST(s5k4ecgx_Main_Flash_Off);
		CAM_DEBUG(" FLASH OFF");
		cam_flash_off(FLASH_MODE_CAMERA);
	} else {
		CAM_DEBUG(" UNKNOWN FLASH MODE");
	}

	s5k4ecgx_control.settings.flash_state = mode;
	
}

static int s5k4ecgx_get_sensor_af_status(int32_t index)
{
//	int ret;
	int af_status = 0;

	if (index == 0) {	//ist AF Searching
		msm_camera_i2c_write(s5k4ecgx_s_ctrl.sensor_i2c_client, 0x002C, 0x7000, MSM_CAMERA_I2C_WORD_DATA);
		msm_camera_i2c_write(s5k4ecgx_s_ctrl.sensor_i2c_client, 0x002E, 0x2EEE, MSM_CAMERA_I2C_WORD_DATA);    	
		msm_camera_i2c_read(s5k4ecgx_s_ctrl.sensor_i2c_client, 0x0F12, &af_status, MSM_CAMERA_I2C_WORD_ADDR);

		printk("s5k4ecgx_get_sensor_af_status 1st AF Searching ret = %d \n", af_status);

		if (af_status == EXT_CFG_AF_LOWCONF) {	//AF fail
			s5k4ecgx_control.af_mode = SHUTTER_AF_MODE;
			if (s5k4ecgx_get_flash_status()) {
				s5k4ecgx_set_ae_awb(0);	//kk0704.park :: Sensor Company Guide
				S5K4ECGX_WRITE_LIST(s5k4ecgx_FAST_AE_Off);
				S5K4ECGX_WRITE_LIST(s5k4ecgx_Pre_Flash_Off);
				s5k4ecgx_set_flash(FLASH_OFF);
				mdelay(100);
			}
		}

	} else if (index == 1) {	//2nd AF Searcing
		msm_camera_i2c_write(s5k4ecgx_s_ctrl.sensor_i2c_client, 0x002C, 0x7000, MSM_CAMERA_I2C_WORD_DATA);
		msm_camera_i2c_write(s5k4ecgx_s_ctrl.sensor_i2c_client, 0x002E, 0x2207, MSM_CAMERA_I2C_WORD_DATA);    	
		msm_camera_i2c_read(s5k4ecgx_s_ctrl.sensor_i2c_client, 0x0F12, &af_status, MSM_CAMERA_I2C_WORD_ADDR);

		printk("s5k4ecgx_get_sensor_af_status 2nd AF Searching  ret = %d \n", af_status);

		if (af_status == 0) {	//AF success
			s5k4ecgx_control.af_mode = SHUTTER_AF_MODE;
			if (s5k4ecgx_get_flash_status()) {
				s5k4ecgx_set_ae_awb(0);	//kk0704.park :: Sensor Company Guide
				S5K4ECGX_WRITE_LIST(s5k4ecgx_FAST_AE_Off);
				S5K4ECGX_WRITE_LIST(s5k4ecgx_Pre_Flash_Off);
				s5k4ecgx_set_flash(FLASH_OFF);
				mdelay(100);
			}
		}
	} else {
		printk("s5k4ecgx_get_sensor_af_status  wrong Command\n");
		s5k4ecgx_control.af_mode = SHUTTER_AF_MODE;
		if (s5k4ecgx_get_flash_status()) {

			s5k4ecgx_set_flash(FLASH_OFF);
		}
	}
	return af_status;
}

void s5k4ecgx_set_preview_size(int32_t index)
{
	cam_info(" %d -> %d\n", s5k4ecgx_control.settings.preview_size_idx, index);

	if (index != PREVIEW_SIZE_HD && s5k4ecgx_s_ctrl.is_HD_preview == 1) {
		S5K4ECGX_WRITE_LIST(s5k4ecgx_1280_Preview_Disable);
		printk("DISABLE 1280X720 Preview SIZE \n");
	}
	s5k4ecgx_s_ctrl.is_HD_preview = 0;		
	
	switch (index) {

	case PREVIEW_SIZE_VGA:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_640_Preview);
		break;
		
	case PREVIEW_SIZE_720_D1:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_720_Preview);
		break;

	case PREVIEW_SIZE_WVGA:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_800_Preview);
		break;

	case PREVIEW_SIZE_HD:
		s5k4ecgx_s_ctrl.is_HD_preview = 1;		
		S5K4ECGX_WRITE_LIST(s5k4ecgx_1280_Preview);
		break;

	case PREVIEW_SIZE_HD_DISABLE:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_800_Preview);
		break;

	case PREVIEW_SIZE_W960:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_960_Preview);
		break;

	case PREVIEW_SIZE_QCIF:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_640_Preview);
		break;

	default:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_640_Preview);
		break;
	}
	mdelay(100);

	s5k4ecgx_control.settings.preview_size_idx = index;
}

void s5k4ecgx_set_picture_size(int32_t index)
{
	cam_info(" %d -> %d\n", s5k4ecgx_control.settings.capture_size, index);

	switch (index) {

	case MSM_V4L2_PICTURE_SIZE_2560x1920_5M:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_640_Preview);
		S5K4ECGX_WRITE_LIST(s5k4ecgx_5M_Capture);
		break;
		
	case MSM_V4L2_PICTURE_SIZE_2048x1536_3M:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_640_Preview);
		S5K4ECGX_WRITE_LIST(s5k4ecgx_3M_Capture);
		break;

	case MSM_V4L2_PICTURE_SIZE_1600x1200_2M:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_640_Preview);
		S5K4ECGX_WRITE_LIST(s5k4ecgx_2M_Capture);
		break;

	case MSM_V4L2_PICTURE_SIZE_1280x960_1M:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_640_Preview);
		S5K4ECGX_WRITE_LIST(s5k4ecgx_1M_Capture);
		break;

	case MSM_V4L2_PICTURE_SIZE_1024x768_8K:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_640_Preview);
		S5K4ECGX_WRITE_LIST(s5k4ecgx_XGA_Capture);
		break;

	case MSM_V4L2_PICTURE_SIZE_640x480_VGA:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_640_Preview);
		S5K4ECGX_WRITE_LIST(s5k4ecgx_VGA_Capture);
		break;

	case MSM_V4L2_PICTURE_SIZE_2560x1536_4M_WIDE:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_800_Preview);
		S5K4ECGX_WRITE_LIST(s5k4ecgx_4M_WIDE_Capture);
		break;

	case MSM_V4L2_PICTURE_SIZE_2048x1232_2_4M_WIDE:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_800_Preview);
		S5K4ECGX_WRITE_LIST(s5k4ecgx_3M_WIDE_Capture);
		break;

	case MSM_V4L2_PICTURE_SIZE_1600x960_1_5M_WIDE:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_800_Preview);
		S5K4ECGX_WRITE_LIST(s5k4ecgx_1_5M_WIDE_Capture);
		break;

	case MSM_V4L2_PICTURE_SIZE_800x480_4K_WIDE:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_800_Preview);
		S5K4ECGX_WRITE_LIST(s5k4ecgx_4K_WIDE_Capture);
		break;

	default:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_640_Preview);
		S5K4ECGX_WRITE_LIST(s5k4ecgx_5M_Capture);
		break;
	}


	s5k4ecgx_control.settings.capture_size = index;
}

static void s5k4ecgx_set_af_mode(int mode, int index)
{
	CAM_DEBUG(" %d", mode);
	if (mode != CAMERA_AF_MACRO)
		mode = CAMERA_AF_AUTO;

	switch (mode) {
	case CAMERA_AF_AUTO:
		if (index == 1) {
		S5K4ECGX_WRITE_LIST(s5k4ecgx_AF_Normal_mode_1);
		} else if (index == 2) {
		S5K4ECGX_WRITE_LIST(s5k4ecgx_AF_Normal_mode_2);
		} else if (index == 3)	{
		S5K4ECGX_WRITE_LIST(s5k4ecgx_AF_Normal_mode_3);
		} else {
			printk("s5k4ecgx_set_af_mode SETTING ERROR! \n");
		}
		s5k4ecgx_control.settings.focus_status = IN_AUTO_MODE;
		break;

	case CAMERA_AF_MACRO:
		if (index == 1) {
		S5K4ECGX_WRITE_LIST(s5k4ecgx_AF_Macro_mode_1);
		} else if (index == 2) {
		S5K4ECGX_WRITE_LIST(s5k4ecgx_AF_Macro_mode_2);
		} else if (index == 3) {
		S5K4ECGX_WRITE_LIST(s5k4ecgx_AF_Macro_mode_3);
		} else {
			printk("s5k4ecgx_set_af_mode SETTING ERROR! \n");
		}
		s5k4ecgx_control.settings.focus_status = IN_MACRO_MODE;
		break;

	default:
		break;
	}

	s5k4ecgx_control.settings.focus_mode = mode;
}

static int s5k4ecgx_set_af_stop(int af_check)
{
	CAM_DEBUG(" %d", af_check);

	if (s5k4ecgx_get_flash_status()) {
		S5K4ECGX_WRITE_LIST(s5k4ecgx_FAST_AE_Off);
		S5K4ECGX_WRITE_LIST(s5k4ecgx_Pre_Flash_Off);
		s5k4ecgx_set_flash(FLASH_OFF);
	}

	s5k4ecgx_set_af_mode(s5k4ecgx_control.settings.focus_mode, 1);
	msleep(100);
	s5k4ecgx_set_af_mode(s5k4ecgx_control.settings.focus_mode, 2);
	msleep(100);
	s5k4ecgx_set_af_mode(s5k4ecgx_control.settings.focus_mode, 3);
	msleep(100);

	s5k4ecgx_control.af_mode = SHUTTER_AF_MODE;

	return 0;
}

void s5k4ecgx_set_frame_rate(int32_t fps)
{
	cam_info(" %d", fps);

	switch (fps) {
	case 15:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_FPS_15);
		break;

	case 20:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_FPS_20);
		break;

	case 24:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_FPS_24);
		break;

	case 25:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_FPS_25);
		break;

	case 30:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_FPS_30);
		break;

	default:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_FPS_Auto);
		break;
	}

	s5k4ecgx_control.settings.fps = fps;
}

static int32_t s5k4ecgx_sensor_setting(int update_type, int rt)
{
	int32_t rc = 0;

	cam_info(" E");

	switch (update_type) {
	case REG_INIT:
		break;

	case UPDATE_PERIODIC:
printk("@@@@@ s5k4ecgx_sensor_setting 11111@@@@@\n");
		msm_sensor_enable_debugfs(s5k4ecgx_control.s_ctrl);
printk("@@@@@ s5k4ecgx_sensor_setting 22222@@@@@\n");
		if (rt == RES_PREVIEW || rt == RES_CAPTURE) {
			CAM_DEBUG(" UPDATE_PERIODIC");
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

static int s5k4ecgx_set_effect(int effect)
{
	CAM_DEBUG(" %d", effect);

	switch (effect) {
	case CAMERA_EFFECT_OFF:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_Effect_Normal);
		break;

	case CAMERA_EFFECT_MONO:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_Effect_Black_White);
		break;

	case CAMERA_EFFECT_NEGATIVE:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_Effect_Negative);
		break;

	case CAMERA_EFFECT_SEPIA:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_Effect_Sepia);
		break;

	default:
		cam_info(" effect : default");
		S5K4ECGX_WRITE_LIST(s5k4ecgx_Effect_Normal);
		return 0;
	}

	s5k4ecgx_control.settings.effect = effect;

	return 0;
}

static int s5k4ecgx_set_whitebalance(int wb)
{
	CAM_DEBUG(" %d", wb);

	switch (wb) {
	case CAMERA_WHITE_BALANCE_AUTO:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_WB_Auto);
		break;

	case CAMERA_WHITE_BALANCE_INCANDESCENT:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_WB_Tungsten);
		break;

	case CAMERA_WHITE_BALANCE_FLUORESCENT:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_WB_Fluorescent);
		break;

	case CAMERA_WHITE_BALANCE_DAYLIGHT:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_WB_Sunny);
		break;

	case CAMERA_WHITE_BALANCE_CLOUDY_DAYLIGHT:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_WB_Cloudy);
		break;

	default:
		cam_info(" WB : default");
		return 0;
	}

	s5k4ecgx_control.settings.wb = wb;

	return 0;
}

static void s5k4ecgx_check_dataline(int val)
{
	if (val) {
		CAM_DEBUG(" DTP ON");
	//	S5K4ECGX_WRITE_LIST(s5k4ecgx_DTP_init);
		s5k4ecgx_control.dtpTest = 1;

	} else {
		CAM_DEBUG(" DTP OFF");
	//	S5K4ECGX_WRITE_LIST(s5k4ecgx_DTP_stop);
		s5k4ecgx_control.dtpTest = 0;
	}
}

int sensor_get_DTP_Value()
{
	return s5k4ecgx_control.dtpTest;
}

static void s5k4ecgx_set_ae_awb(int lock)
{

	if( (s5k4ecgx_control.settings.wb != CAMERA_WHITE_BALANCE_AUTO) ||
		(s5k4ecgx_get_flash_status()) ||
		(s5k4ecgx_control.settings.scenemode == CAMERA_SCENE_SUNSET ||
		s5k4ecgx_control.settings.scenemode == CAMERA_SCENE_DAWN ||
		s5k4ecgx_control.settings.scenemode == CAMERA_SCENE_CANDLE)
		) {
		CAM_DEBUG("AWB SKIP , AE SET ");
		if (lock) {
			CAM_DEBUG(" AE LOCK");
			S5K4ECGX_WRITE_LIST(s5k4ecgx_ae_lock);
		} else {
			CAM_DEBUG(" AE UNLOCK");
			S5K4ECGX_WRITE_LIST(s5k4ecgx_ae_unlock);
		}
		return;
	}
	
	if (lock) {
		CAM_DEBUG(" AE_AWB LOCK");
		S5K4ECGX_WRITE_LIST(s5k4ecgx_ae_lock);
		S5K4ECGX_WRITE_LIST(s5k4ecgx_awb_lock);
	} else {
		CAM_DEBUG(" AE_AWB UNLOCK");
		S5K4ECGX_WRITE_LIST(s5k4ecgx_ae_unlock);
		S5K4ECGX_WRITE_LIST(s5k4ecgx_awb_unlock);
	}
}

static void s5k4ecgx_set_ev(int ev)
{
	CAM_DEBUG(" %d", ev);

	switch (ev) {
	case CAMERA_EV_M4:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_EV_Minus_4);
		break;

	case CAMERA_EV_M3:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_EV_Minus_3);
		break;

	case CAMERA_EV_M2:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_EV_Minus_2);
		break;

	case CAMERA_EV_M1:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_EV_Minus_1);
		break;

	case CAMERA_EV_DEFAULT:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_EV_Default);
		break;

	case CAMERA_EV_P1:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_EV_Plus_1);
		break;

	case CAMERA_EV_P2:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_EV_Plus_2);
		break;

	case CAMERA_EV_P3:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_EV_Plus_3);
		break;

	case CAMERA_EV_P4:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_EV_Plus_4);
		break;
	default:
		cam_info(" ev : default");
		break;
	}

	s5k4ecgx_control.settings.brightness = ev;
}

static void s5k4ecgx_set_metering(int mode)
{
	CAM_DEBUG(" %d", mode);

	switch (mode) {
	case CAMERA_CENTER_WEIGHT:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_Metering_Center);
		break;

	case CAMERA_AVERAGE:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_Metering_Matrix);
		break;

	case CAMERA_SPOT:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_Metering_Spot);
		break;

	default:
		cam_info(" AE : default");
		break;
	}

	s5k4ecgx_control.settings.metering = mode;
}

static void s5k4ecgx_set_scene_mode(int mode)
{
	CAM_DEBUG(" %d", mode);
	if( s5k4ecgx_control.settings.scenemode == mode) {
		printk("SCENE MODE IS SAME WITH BEFORE");
		return;
	}
/* kk0704.park :: sensor company request */
	if (mode != CAMERA_SCENE_OFF && mode != CAMERA_SCENE_AUTO)
		S5K4ECGX_WRITE_LIST(s5k4ecgx_Scene_Default);

	switch (mode) {

	case CAMERA_SCENE_OFF:
	case CAMERA_SCENE_AUTO:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_Scene_Default);
		break;

	case CAMERA_SCENE_LANDSCAPE:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_Scene_Landscape);
		break;

	case CAMERA_SCENE_DAWN:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_Scene_Duskdawn);
		break;

	case CAMERA_SCENE_BEACH:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_Scene_Beach_Snow);
		break;

	case CAMERA_SCENE_SUNSET:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_Scene_Sunset);
		break;

	case CAMERA_SCENE_NIGHT:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_Scene_Nightshot);
		break;

	case CAMERA_SCENE_PORTRAIT:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_Scene_Portrait);
		break;

	case CAMERA_SCENE_AGAINST_LIGHT:
		CAM_DEBUG("SCENE AGAINST_LIGHT SETTING is same with Scene Default");
/* kk0704.park :: sensor company request 
    Against Light setting is same with Scene_Default */
/*		S5K4ECGX_WRITE_LIST(s5k4ecgx_Scene_Backlight);	*/
		break;

	case CAMERA_SCENE_SPORT:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_Scene_Sports);
		break;

	case CAMERA_SCENE_FALL:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_Scene_Fall_Color);
		break;

	case CAMERA_SCENE_TEXT:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_Scene_Text);
		//Sensor Company Request
		S5K4ECGX_WRITE_LIST(s5k4ecgx_AF_Macro_mode_1);
		msleep(100);
		S5K4ECGX_WRITE_LIST(s5k4ecgx_AF_Macro_mode_2);
		msleep(100);
		S5K4ECGX_WRITE_LIST(s5k4ecgx_AF_Macro_mode_3);
		msleep(100);
		break;

	case CAMERA_SCENE_CANDLE:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_Scene_Candle_Light);
		break;

	case CAMERA_SCENE_FIRE:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_Scene_Fireworks);
		break;

	case CAMERA_SCENE_PARTY:
		S5K4ECGX_WRITE_LIST(s5k4ecgx_Scene_Party_Indoor);
		break;

	default:
		cam_info(" scene : default");
		break;
	}

	if ( mode!=CAMERA_SCENE_TEXT && s5k4ecgx_control.settings.scenemode == CAMERA_SCENE_TEXT ) {
		s5k4ecgx_set_af_mode(s5k4ecgx_control.settings.focus_mode, 1);	//Sensor Company Request
		msleep(100);
		s5k4ecgx_set_af_mode(s5k4ecgx_control.settings.focus_mode, 2);	//Sensor Company Request
		msleep(100);
		s5k4ecgx_set_af_mode(s5k4ecgx_control.settings.focus_mode, 3);	//Sensor Company Request
		msleep(100);
	}

	s5k4ecgx_control.settings.scenemode = mode;
}

static void s5k4ecgx_set_iso(int iso)
{
	CAM_DEBUG(" %d", iso);

	if (s5k4ecgx_control.awb_mode == CAMERA_WHITE_BALANCE_AUTO) {
		switch (iso) {
			case CAMERA_ISO_MODE_AUTO:
				S5K4ECGX_WRITE_LIST(s5k4ecgx_ISO_Auto);
				break;

			case CAMERA_ISO_MODE_50:
				S5K4ECGX_WRITE_LIST(s5k4ecgx_ISO_50);
				break;

			case CAMERA_ISO_MODE_100:
				S5K4ECGX_WRITE_LIST(s5k4ecgx_ISO_100);
				break;

			case CAMERA_ISO_MODE_200:
				S5K4ECGX_WRITE_LIST(s5k4ecgx_ISO_200);
				break;

			case CAMERA_ISO_MODE_400:
				S5K4ECGX_WRITE_LIST(s5k4ecgx_ISO_400);
				break;

			default:
				cam_info(" iso : default");
				break;
		}
	} else {
		switch (iso) {
			case CAMERA_ISO_MODE_AUTO:
				S5K4ECGX_WRITE_LIST(s5k4ecgx_ISO_Auto_MWB_on);
				break;

			case CAMERA_ISO_MODE_50:
				S5K4ECGX_WRITE_LIST(s5k4ecgx_ISO_50_MWB_on);
				break;

			case CAMERA_ISO_MODE_100:
				S5K4ECGX_WRITE_LIST(s5k4ecgx_ISO_100_MWB_on);
				break;

			case CAMERA_ISO_MODE_200:
				S5K4ECGX_WRITE_LIST(s5k4ecgx_ISO_200_MWB_on);
				break;

			case CAMERA_ISO_MODE_400:
				S5K4ECGX_WRITE_LIST(s5k4ecgx_ISO_400_MWB_on);
				break;

			default:
				cam_info(" iso : default");
				break;
		}
	}

	s5k4ecgx_control.settings.iso = iso;
}

static void s5k4ecgx_touchaf_set_resolution(unsigned int addr, unsigned int value)
{
	s5k4ecgx_sensor_write(0xFCFC, 0xD000);
	s5k4ecgx_sensor_write(0x0028, 0x7000);
	s5k4ecgx_sensor_write(0x002A, addr);
	s5k4ecgx_sensor_write(0x0F12, value);
}

static int s5k4ecgx_set_touchaf_pos(int x, int y)
{

	static unsigned int inWindowWidth = 143;
	static unsigned int inWindowHeight = 143;
	static unsigned int outWindowWidth = 320;
	static unsigned int outWindowHeight = 266;

	unsigned int previewWidth;
	unsigned int previewHeight;

	if (s5k4ecgx_control.settings.preview_size_idx == PREVIEW_SIZE_WVGA) {
		previewWidth = 800;
		previewHeight = 480;
	} else {
		previewWidth = 640;
		previewHeight = 480;
	}

	x = previewWidth - x;
	y = previewHeight - y;

	s5k4ecgx_sensor_write(0x002C, 0x7000);
	s5k4ecgx_sensor_write(0x002E, 0x02A0);
	s5k4ecgx_sensor_read(0x0F12, (unsigned short *)&inWindowWidth);
	s5k4ecgx_sensor_write(0x002C, 0x7000);
	s5k4ecgx_sensor_write(0x002E, 0x02A2);
	s5k4ecgx_sensor_read(0x0F12, (unsigned short *)&inWindowHeight);
	inWindowWidth = inWindowWidth * previewWidth / 1024;
	inWindowHeight = inWindowHeight * previewHeight / 1024;

	s5k4ecgx_sensor_write(0x002C, 0x7000);
	s5k4ecgx_sensor_write(0x002E, 0x0298);
	s5k4ecgx_sensor_read(0x0F12, (unsigned short *)&outWindowWidth);
	s5k4ecgx_sensor_write(0x002C, 0x7000);
	s5k4ecgx_sensor_write(0x002E, 0x029A);
	s5k4ecgx_sensor_read(0x0F12, (unsigned short *)&outWindowHeight);
	outWindowWidth = outWindowWidth * previewWidth / 1024;
	outWindowHeight = outWindowHeight * previewHeight / 1024;

	if (x < inWindowWidth/2)	
		x = inWindowWidth/2+1;
	else if (x > previewWidth - inWindowWidth/2)
		x = previewWidth - inWindowWidth/2 -1;
	if (y < inWindowHeight/2)	
		y = inWindowHeight/2+1;
	else if (y > previewHeight - inWindowHeight/2)
		y = previewHeight - inWindowHeight/2 -1;

	s5k4ecgx_touchaf_set_resolution(0x029C, (x - inWindowWidth/2) * 1024 / previewWidth);		
	s5k4ecgx_touchaf_set_resolution(0x029E, (y - inWindowHeight/2) * 1024 / previewHeight);		

	if (x < outWindowWidth/2)	
		x = outWindowWidth/2+1;
	else if (x > previewWidth - outWindowWidth/2)
		x = previewWidth - outWindowWidth/2 -1;
	if (y < outWindowHeight/2)	
		y = outWindowHeight/2+1;
	else if (y > previewHeight - outWindowHeight/2)
		y = previewHeight - outWindowHeight/2 -1;

	s5k4ecgx_touchaf_set_resolution(0x0294, (x - outWindowWidth/2) * 1024 / previewWidth);		
	s5k4ecgx_touchaf_set_resolution(0x0296, (y - outWindowHeight/2) * 1024 / previewHeight);		

	s5k4ecgx_touchaf_set_resolution(0x02A4, 0x0001);

	msleep(100);		// 1frame delay

	return 0;
	
}

static struct msm_cam_clk_info cam_clk_info[] = {
	{"cam_m_clk", MSM_SENSOR_MCLK_24HZ},
};

static int s5k4ecgx_sensor_match_id(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;

	cam_info(" Nothing");

	return rc;
}

static int s5k4ecgx_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
#if defined (CONFIG_MACH_ARUBASLIM_OPEN)
	int32_t rc = 0;
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;

	CAM_DEBUG("%s   E\n", __func__);

#ifdef CONFIG_LOAD_FILE
	CAM_DEBUG("[In sensor_power_up]s5k4ecgx_reg_table_init_start!!");
	s5k4ecgx_regs_table_init();
#endif

	s_ctrl->reg_ptr = kzalloc(sizeof(struct regulator *)
			* data->sensor_platform_info->num_vreg, GFP_KERNEL);
	if (!s_ctrl->reg_ptr) {
		pr_err("%s: could not allocate mem for regulators\n",
			__func__);
		return -ENOMEM;
	}
	rc = msm_camera_config_vreg(&s_ctrl->sensor_i2c_client->client->dev,
			s_ctrl->sensordata->sensor_platform_info->cam_vreg,
			s_ctrl->sensordata->sensor_platform_info->num_vreg,
			s_ctrl->reg_ptr, 1);
	if (rc < 0) {
		pr_err("%s: regulator on failed\n", __func__);
		goto config_vreg_failed;
	}

	gpio_request(ARUBA_CAM_STBY, "aruba_stby");
	gpio_request(ARUBA_CAM_RESET, "aruba_reset");
	gpio_request(ARUBA_CAM_IO_EN, "aruba_a_en");
	gpio_request(ARUBA_CAM_C_EN, "aruba_c_en");
	gpio_request(ARUBA_CAM_AF_EN, "aruba_af_en");

	if (rc < 0) {
		pr_err("%s: [ARUBASLIM]gpio_request failed\n", __func__);
	}

	gpio_direction_output(ARUBA_CAM_STBY, 0);
	gpio_direction_output(ARUBA_CAM_RESET, 0);
	gpio_direction_output(ARUBA_CAM_IO_EN, 0);
	gpio_direction_output(ARUBA_CAM_C_EN, 0);
	gpio_direction_output(ARUBA_CAM_AF_EN, 0);

	gpio_set_value_cansleep(ARUBA_CAM_STBY, 0);		//STBYN :: MSM_GPIO 96
	gpio_set_value_cansleep(ARUBA_CAM_RESET, 0);	//RSTN :: MSM_GPIO 85
	gpio_set_value_cansleep(ARUBA_CAM_IO_EN, 0);
	gpio_set_value_cansleep(ARUBA_CAM_C_EN, 0);

	msleep(1);
	rc = msm_camera_enable_vreg(&s_ctrl->sensor_i2c_client->client->dev,
			s_ctrl->sensordata->sensor_platform_info->cam_vreg,
			s_ctrl->sensordata->sensor_platform_info->num_vreg,
			s_ctrl->reg_ptr, 1);
	if (rc < 0) {
		pr_err("%s: [ARUBASLIM]enable regulator failed\n", __func__);
		goto enable_vreg_failed;
	}
	udelay(150);// VCAM_A_2.8V

	gpio_set_value(ARUBA_CAM_IO_EN, 1);
	udelay(250);// GPIO 4 : EN_C_EN / VVT_1.8V

	gpio_set_value(ARUBA_CAM_AF_EN, 1);

	gpio_set_value(FRONT_CAM_STBY, 1);
	msleep(2);	// gpio 76 : VT_CAM_STBY

	rc = msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
		cam_clk_info, &s_ctrl->cam_clk, ARRAY_SIZE(cam_clk_info), 1);
	if (rc < 0) {
		pr_err("%s: clk enable failed\n", __func__);
		goto enable_clk_failed;
	}
	msleep(2);	// CAM_MCLK

	gpio_set_value(FRONT_CAM_RESET, 1);
	msleep(1);	// VT_CAM_RESET

	gpio_set_value(FRONT_CAM_STBY, 0);
	msleep(1);	// VT_CAM_RESET : LOW

	gpio_set_value(ARUBA_CAM_C_EN, 1); // GPIO 107 : CAM_C_EN
	msleep(1);	//GPIO 107 : CAM_C_EN

	gpio_set_value_cansleep(ARUBA_CAM_STBY, 1);
	msleep(1);	//GPIO 96 : 5M_CAM_STBY

	gpio_set_value_cansleep(ARUBA_CAM_RESET, 1);
	msleep(1);	//GPIO 85 : 5M_CAM_RESET
	usleep_range(1000, 2000);

	if (data->sensor_platform_info->ext_power_ctrl != NULL)
		data->sensor_platform_info->ext_power_ctrl(1);

	if (data->sensor_platform_info->i2c_conf &&
		data->sensor_platform_info->i2c_conf->use_i2c_mux)
		msm_sensor_enable_i2c_mux(data->sensor_platform_info->i2c_conf);

	return rc;

enable_clk_failed:
//kk0704.park :: ARUBA TEMP		msm_camera_config_gpio_table(data, 0);
config_gpio_failed:
	msm_camera_enable_vreg(&s_ctrl->sensor_i2c_client->client->dev,
			s_ctrl->sensordata->sensor_platform_info->cam_vreg,
			s_ctrl->sensordata->sensor_platform_info->num_vreg,
			s_ctrl->reg_ptr, 0);

enable_vreg_failed:
	msm_camera_config_vreg(&s_ctrl->sensor_i2c_client->client->dev,
		s_ctrl->sensordata->sensor_platform_info->cam_vreg,
		s_ctrl->sensordata->sensor_platform_info->num_vreg,
		s_ctrl->reg_ptr, 0);
config_vreg_failed:
//kk0704.park ARUBA TEMP	msm_camera_request_gpio_table(data, 0);
request_gpio_failed:
	kfree(s_ctrl->reg_ptr);

	CAM_DEBUG("%s  X\n", __func__);

	return rc;
#else
	int rc = 0;

	printk("s5k4ecgx_sensor_power_up E\n");

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
#endif
}


static int s5k4ecgx_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
#if defined (CONFIG_MACH_ARUBASLIM_OPEN)
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;

	CAM_DEBUG("%s   E\n", __func__);

	if (data->sensor_platform_info->i2c_conf &&
		data->sensor_platform_info->i2c_conf->use_i2c_mux)
		msm_sensor_disable_i2c_mux(
			data->sensor_platform_info->i2c_conf);

	if (data->sensor_platform_info->ext_power_ctrl != NULL)
		data->sensor_platform_info->ext_power_ctrl(0);
	msleep(1);

	cam_flash_off(FLASH_MODE_CAMERA); //kk0704.park :: Flash Off when Power down

	gpio_set_value_cansleep(ARUBA_CAM_RESET, 0); // GPIO 85	
	msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
		cam_clk_info, &s_ctrl->cam_clk, ARRAY_SIZE(cam_clk_info), 0);
	msleep(1);

	gpio_set_value_cansleep(ARUBA_CAM_STBY, 0); //GPIO 96
	gpio_set_value_cansleep(FRONT_CAM_RESET, 0);
	gpio_set_value_cansleep(ARUBA_CAM_C_EN, 0);
	gpio_set_value_cansleep(ARUBA_CAM_IO_EN, 0);
	gpio_set_value_cansleep(ARUBA_CAM_AF_EN, 0);

	gpio_free(ARUBA_CAM_RESET);
	gpio_free(ARUBA_CAM_STBY);
	gpio_free(FRONT_CAM_RESET);
	gpio_free(ARUBA_CAM_C_EN);
	gpio_free(ARUBA_CAM_IO_EN);
	gpio_free(ARUBA_CAM_AF_EN);

	msm_camera_enable_vreg(&s_ctrl->sensor_i2c_client->client->dev,
		s_ctrl->sensordata->sensor_platform_info->cam_vreg,
		s_ctrl->sensordata->sensor_platform_info->num_vreg,
		s_ctrl->reg_ptr, 0);
	msm_camera_config_vreg(&s_ctrl->sensor_i2c_client->client->dev,
		s_ctrl->sensordata->sensor_platform_info->cam_vreg,
		s_ctrl->sensordata->sensor_platform_info->num_vreg,
		s_ctrl->reg_ptr, 0);
	kfree(s_ctrl->reg_ptr);

#ifdef CONFIG_LOAD_FILE
	s5k4ecgx_regs_table_exit();
#endif
	CAM_DEBUG("%s  X\n", __func__);

	return 0;
#else
	int rc = 0;

	CAM_DEBUG(" E");

	s5k4ecgx_set_flash(FLASH_OFF);

	/*Soft landing */
	S5K4ECGX_WRITE_LIST(s5k4ecgx_FAST_AE_Off);
	S5K4ECGX_WRITE_LIST(s5k4ecgx_Pre_Flash_Off);
	usleep(WAIT_10MS);

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
	s5k4ecgx_regs_table_exit();
#endif
	CAM_DEBUG(" X");

	return rc;
#endif
}

void s5k4ecgx_check_ae_stable()
{
	unsigned short ae_stable = 0;
	int ae_count = 0;
	
	msleep(200);
	while (ae_count++ < 5) {
		s5k4ecgx_sensor_write(0x002C, 0x7000);
		s5k4ecgx_sensor_write(0x002E, 0x2C74);
		s5k4ecgx_sensor_read(0x0F12, (unsigned short *)&ae_stable);
		if (ae_stable == 0x01)
			break;
		msleep(100);
	}
	if (ae_count > 4) {
		printk("[s5k4ecgx_check_ae_stable] AE NOT STABLE \n");
	}
}

void s5k4ecgx_set_af_status(unsigned short index)
{
	if (index == 1) {	//index == 1 : Do AF

		s5k4ecgx_get_LowLightCondition();	//check Light Condition

		if (s5k4ecgx_get_flash_status()) {

			S5K4ECGX_WRITE_LIST(s5k4ecgx_FAST_AE_On);
			S5K4ECGX_WRITE_LIST(s5k4ecgx_Pre_Flash_On);
			s5k4ecgx_set_flash(MOVIE_FLASH);
			s5k4ecgx_check_ae_stable();
		}

//		if (s5k4ecgx_control.af_mode != TOUCH_AF_MODE)
//			S5K4ECGX_WRITE_LIST(s5k4ecgx_ae_lock);

	} else if (index == 2) {

		S5K4ECGX_WRITE_LIST(s5k4ecgx_Single_AF_Start);

		if (s5k4ecgx_control.settings.scenemode == CAMERA_SCENE_NIGHT)
			msleep(100);
		
		if (s5k4ecgx_control.lowLight)
			msleep(400);
		else
			msleep(200);
		
	} else {			//index == 0 : AF Cancel
		s5k4ecgx_set_af_mode(s5k4ecgx_control.settings.focus_mode, 1);
		msleep(100);
		s5k4ecgx_set_af_mode(s5k4ecgx_control.settings.focus_mode, 2);
		msleep(100);
		s5k4ecgx_set_af_mode(s5k4ecgx_control.settings.focus_mode, 3);
		msleep(100);
	}
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
		if (s5k4ecgx_control.settings.scene == CAMERA_SCENE_BEACH)
			break;
		else {
			s5k4ecgx_set_ev(ctrl_info.value_1);
			break;
		}

	case EXT_CAM_WB:
		printk("EXT_CAM_WB\n");
		s5k4ecgx_set_whitebalance(ctrl_info.value_1);
		s5k4ecgx_control.awb_mode = ctrl_info.value_1;
		break;

	case EXT_CAM_METERING:
		printk("EXT_CAM_METERING\n");
		s5k4ecgx_set_metering(ctrl_info.value_1);
		break;

	case EXT_CAM_ISO:
		printk("EXT_CAM_ISO\n");
		s5k4ecgx_set_iso(ctrl_info.value_1);
		break;

	case EXT_CAM_EFFECT:
		printk("EXT_CAM_EFFECT START\n");
		s5k4ecgx_set_effect(ctrl_info.value_1);
		printk("EXT_CAM_EFFECT END\n");
		
		break;

	case EXT_CAM_SCENE_MODE:
		printk("EXT_CAM_SCENE_MODE\n");
		s5k4ecgx_set_scene_mode(ctrl_info.value_1);
		break;

	case EXT_CAM_MOVIE_MODE:
		printk("EXT_CAM_MOVIE_MODE\n");
		CAM_DEBUG(" MOVIE mode : %d", ctrl_info.value_1);
		s5k4ecgx_control.cam_mode = ctrl_info.value_1;
		break;

	case EXT_CAM_PREVIEW_SIZE:
		printk("EXT_CAM_PREVIEW_SIZE\n");
		s5k4ecgx_set_preview_size(ctrl_info.value_1);
		break;

	case CFG_SET_PICTURE_SIZE:
		printk("CFG_SET_PICTURE_SIZE\n");
		s5k4ecgx_set_picture_size(ctrl_info.value_1);
		break;

	case EXT_CAM_SET_AF_STATUS:
		printk("EXT_CAM_SET_AF_STATUS\n");
		s5k4ecgx_set_af_status(ctrl_info.value_1);
		break;

	case EXT_CAM_GET_AF_STATUS:
		printk("EXT_CAM_GET_AF_STATUS\n");
		ctrl_info.value_1 = s5k4ecgx_get_sensor_af_status(ctrl_info.value_1);
		break;

	case EXT_CAM_GET_AF_RESULT:
		printk("EXT_CAM_GET_AF_RESULT\n");
		ctrl_info.value_1 = s5k4ecgx_get_af_result();
		break;

	case EXT_CAM_SET_TOUCHAF_POS:
		printk("EXT_CAM_SET_TOUCHAF_POS\n");
		s5k4ecgx_set_touchaf_pos(ctrl_info.value_1,
			ctrl_info.value_2);
		s5k4ecgx_control.af_mode = TOUCH_AF_MODE;
		break;

	case EXT_CAM_SET_AF_MODE:
		printk("EXT_CAM_SET_AF_MODE\n");
		if (ctrl_info.value_1)
			s5k4ecgx_control.af_mode = TOUCH_AF_MODE;
		else
			s5k4ecgx_control.af_mode = SHUTTER_AF_MODE;

		break;

	case EXT_CAM_FLASH_STATUS:
		printk("EXT_CAM_FLASH_STATUS\n");
		s5k4ecgx_set_flash(ctrl_info.value_1);
		break;

	case EXT_CAM_GET_FLASH_STATUS:
		printk("EXT_CAM_GET_FLASH_STATUS\n");
		ctrl_info.value_1 = s5k4ecgx_get_flash_status();
		break;

	case EXT_CAM_FLASH_MODE:
		printk("EXT_CAM_FLASH_MODE\n");
		cam_info("flash mode value : %d", ctrl_info.value_1);
		s5k4ecgx_control.flash_mode = ctrl_info.value_1;
		break;

	case EXT_CAM_FOCUS:
		printk("EXT_CAM_FOCUS\n");
		s5k4ecgx_set_af_mode(ctrl_info.value_1, ctrl_info.value_2);
		break;

	case EXT_CAM_DTP_TEST:
		s5k4ecgx_check_dataline(ctrl_info.value_1);
		break;

	case EXT_CAM_SET_AE_AWB:
		printk("EXT_CAM_SET_AE_AWB\n");
		s5k4ecgx_set_ae_awb(ctrl_info.value_1);
		break;

	case EXT_CAM_SET_AF_STOP:
		printk("EXT_CAM_SET_AF_STOP\n");
		s5k4ecgx_set_af_stop(ctrl_info.value_1);
		break;

	case EXT_CAM_EXIF:
		printk("EXT_CAM_EXIF\n");
		ctrl_info.value_1 = s5k4ecgx_get_exif(ctrl_info.address,
			ctrl_info.value_2);
		break;

	case EXT_CAM_VT_MODE:
		printk("EXT_CAM_VT_MODE\n");
		CAM_DEBUG(" VT mode : %d", ctrl_info.value_1);
		s5k4ecgx_control.vtcall_mode = ctrl_info.value_1;
		break;

	case EXT_CAM_SET_FPS:
		printk("EXT_CAM_SET_FPS\n");
		s5k4ecgx_set_frame_rate(ctrl_info.value_1);
		break;

	case EXT_CAM_SAMSUNG_CAMERA:
		CAM_DEBUG(" SAMSUNG camera : %d", ctrl_info.value_1);
		s5k4ecgx_control.samsungapp = ctrl_info.value_1;
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

static struct v4l2_subdev_info s5k4ecgx_subdev_info[] = {
	{
		.code   = V4L2_MBUS_FMT_YUYV8_2X8,
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt    = 1,
		.order    = 0,
	},
	/* more can be supported, to be added later */
};

void s5k4ecgx_sensor_start_stream(struct msm_sensor_ctrl_t *s_ctrl)
{

	printk(" s5k4ecgx_sensor_start_stream E\n");
#ifndef CONFIG_MACH_ARUBASLIM_OPEN
#ifdef CONFIG_LOAD_FILE
	printk("[MSM_SENSOR]s5k4ecgx_reg_table_init_start!!");
	s5k4ecgx_regs_table_init();
#endif
#endif
	S5K4ECGX_WRITE_LIST(s5k4ecgx_init_reg1);
	S5K4ECGX_WRITE_LIST(s5k4ecgx_init_reg2);
	s5k4ecgx_control.settings.scenemode = CAMERA_SCENE_AUTO;

	printk("  s5k4ecgx_sensor_start_stream X\n");

}

void s5k4ecgx_sensor_stop_stream(struct msm_sensor_ctrl_t *s_ctrl)
{

	printk(" s5k4ecgx_sensor_stop_stream E\n");
	//AF position Default
	S5K4ECGX_WRITE_LIST(s5k4ecgx_Preview_Return);
	msleep(10);	
	S5K4ECGX_WRITE_LIST(s5k4ecgx_AF_Normal_mode_1);
	msleep(150);
	S5K4ECGX_WRITE_LIST(s5k4ecgx_AF_Normal_mode_2);
	msleep(150);
	printk(" s5k4ecgx_sensor_stop_stream X\n");
}

void s5k4ecgx_sensor_preview_mode(struct msm_sensor_ctrl_t *s_ctrl)
{

	printk(" s5k4ecgx_sensor_preview_mode E\n");
	s5k4ecgx_set_ae_awb(0);
	S5K4ECGX_WRITE_LIST(s5k4ecgx_Preview_Return);

	printk("  s5k4ecgx_sensor_preview_mode X\n");

}

void s5k4ecgx_sensor_capture_mode(struct msm_sensor_ctrl_t *s_ctrl)
{

	printk(" s5k4ecgx_sensor_capture_mode E\n");

//kk0704.park :: 	s5k4ecgx_get_LowLightCondition();
	
	if (s5k4ecgx_get_flash_status()) {

		s5k4ecgx_set_flash(CAPTURE_FLASH);
		S5K4ECGX_WRITE_LIST(s5k4ecgx_Main_Flash_On);
	}

	S5K4ECGX_WRITE_LIST(s5k4ecgx_Capture_Start);

	/* reset AF mode */
	s5k4ecgx_control.af_mode = SHUTTER_AF_MODE;

	printk("  s5k4ecgx_sensor_capture_mode X\n");

}

#if defined (CONFIG_MACH_DELOS_OPEN) || defined (CONFIG_MACH_ARUBASLIM_OPEN)
int rear_sensor_check_vendorID(int *vendorID)
{
	printk("rear_sensor_check_vendorID_before\n");
//	msm_sensor_power_up_baffin_duos(&s5k4ecgx_s_ctrl);
	s5k4ecgx_sensor_write(0x0012, 0x0001);
	s5k4ecgx_sensor_write(0x007A, 0x0000);
	s5k4ecgx_sensor_write(0xA000, 0x0004);
	s5k4ecgx_sensor_write(0xA062, 0x4000);
	s5k4ecgx_sensor_write(0xA002, 0x0006);
	s5k4ecgx_sensor_write(0xA000, 0x0001);
	mdelay(100);
	s5k4ecgx_sensor_read(0xA006, (unsigned short *)vendorID);
	printk("rear_sensor_check_vendorID : %d\n",*vendorID);
	
//	*vendorID = s5k4ecgx_sensor_match_id(&s5k4ecgx_s_ctrl);
//	msm_sensor_power_down_baffin_duos(&s5k4ecgx_s_ctrl);
}
#endif

static struct v4l2_subdev_video_ops s5k4ecgx_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct msm_sensor_output_info_t s5k4ecgx_dimensions[] = {
	{
		.x_output = 0xA00,
		.y_output = 0x780,
		.line_length_pclk = 0xA00,
		.frame_length_lines = 0x780,
		.vt_pixel_clk = 9216000,
		.op_pixel_clk = 9216000,
		.binning_factor = 1,
	},
	{
		.x_output = 0x500,
		.y_output = 0x2D0,
		.line_length_pclk = 0x500,
		.frame_length_lines = 0x2D0,
		.vt_pixel_clk = 9216000,
		.op_pixel_clk = 9216000,
		.binning_factor = 1,
	},
};

static struct msm_camera_csi_params s5k4ecgx_csi_params = {
	.data_format = CSI_8BIT,
	.lane_cnt    = 2,
	.lane_assign = 0xe4,
	.dpcm_scheme = 0,
	.settle_cnt  = 0x14,
};

static struct msm_camera_csi_params *s5k4ecgx_csi_params_array[] = {
	&s5k4ecgx_csi_params,
	&s5k4ecgx_csi_params,
};

static struct msm_sensor_output_reg_addr_t s5k4ecgx_reg_addr = {
	.x_output = 0x280,
	.y_output = 0x1E0,
	.line_length_pclk = 0x280,
	.frame_length_lines = 0x1E0,
};

static struct msm_sensor_id_info_t s5k4ecgx_id_info = {
	.sensor_id_reg_addr = 0x0000,  //kk0704.park :: should check
	.sensor_id = 0x4EC,  //kk0704.park :: should check
	.sensor_version = 0x11,
};

static const struct i2c_device_id s5k4ecgx_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&s5k4ecgx_s_ctrl},
	{}
};

static struct i2c_driver s5k4ecgx_i2c_driver = {
	.id_table = s5k4ecgx_i2c_id,
	.probe = msm_sensor_i2c_probe,
	.driver = {
		   .name = "msm_camera_s5k4ecgx",
	},
};

static struct msm_camera_i2c_client s5k4ecgx_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static int __init msm_sensor_init_module(void)
{
	int rc;
	CDBG("S5K4ECGX\n");

	rc = i2c_add_driver(&s5k4ecgx_i2c_driver);

	return rc;
}

static struct msm_camera_i2c_reg_conf s5k4ecgx_capture_settings[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x0242},
	{0x0F12, 0x0001},
	{0x002A, 0x024E},
	{0x0F12, 0x0001},
	{0x002A, 0x0244},
	{0x0F12, 0x0001},
};

static struct msm_camera_i2c_reg_conf s5k4ecgx_preview_return_settings[] = {
	{0x0028, 0x7000},
	{0x002A, 0x05D0},
	{0x0F12, 0x0000},
	{0x002A, 0x0972},
	{0x0F12, 0x0000},
	{0x002A, 0x0242},
	{0x0F12, 0x0000},
	{0x002A, 0x024E},
	{0x0F12, 0x0001},
	{0x002A, 0x0244},
	{0x0F12, 0x0001},

};

static struct msm_camera_i2c_conf_array s5k4ecgx_confs[] = {
	{&s5k4ecgx_capture_settings[0],
	ARRAY_SIZE(s5k4ecgx_capture_settings), 0, MSM_CAMERA_I2C_WORD_DATA},
	{&s5k4ecgx_preview_return_settings[0],
	ARRAY_SIZE(s5k4ecgx_preview_return_settings), 0, MSM_CAMERA_I2C_WORD_DATA},
};


static struct v4l2_subdev_core_ops s5k4ecgx_subdev_core_ops = {
	.s_ctrl = msm_sensor_v4l2_s_ctrl,
	.queryctrl = msm_sensor_v4l2_query_ctrl,
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_ops s5k4ecgx_subdev_ops = {
	.core = &s5k4ecgx_subdev_core_ops,
	.video  = &s5k4ecgx_subdev_video_ops,
};

static struct msm_sensor_fn_t s5k4ecgx_func_tbl = {
	.sensor_start_stream = s5k4ecgx_sensor_start_stream,
	.sensor_stop_stream = s5k4ecgx_sensor_stop_stream,
	.sensor_preview_mode = s5k4ecgx_sensor_preview_mode,
	.sensor_capture_mode = s5k4ecgx_sensor_capture_mode,
	.sensor_set_fps = NULL,
	.sensor_write_exp_gain = NULL,
	.sensor_write_snapshot_exp_gain = NULL,
	.sensor_csi_setting = msm_sensor_setting1,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
#if defined (CONFIG_MACH_DELOS_OPEN) || defined (CONFIG_MACH_DELOS_CTC) || defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
    .sensor_power_up = msm_sensor_power_up_delos_open,
	.sensor_power_down = msm_sensor_power_down_delos_open,
#elif defined (CONFIG_MACH_ARUBA_OPEN)
	.sensor_power_up = msm_sensor_power_up_aruba_open,
	.sensor_power_down = msm_sensor_power_down_aruba_open,
#elif defined (CONFIG_MACH_ARUBASLIM_OPEN)
	.sensor_power_up = s5k4ecgx_sensor_power_up,
	.sensor_power_down = s5k4ecgx_sensor_power_down,
#elif defined CONFIG_MACH_BAFFIN_DUOS_CTC
	.sensor_power_up = msm_sensor_power_up_baffin_duos,
	.sensor_power_down = msm_sensor_power_down_baffin_duos,
#else
	.sensor_power_up = msm_sensor_power_up,
	.sensor_power_down =  msm_sensor_power_down,
#endif
	.sensor_match_id = NULL,
	.sensor_adjust_frame_lines = NULL,
	.sensor_get_csi_params = msm_sensor_get_csi_params,
	.sensor_get_flash_status = s5k4ecgx_get_flash_status,
};

static struct msm_sensor_reg_t s5k4ecgx_regs = {
	.default_data_type = MSM_CAMERA_I2C_WORD_DATA,
	.output_settings = &s5k4ecgx_dimensions[0],
	.num_conf = ARRAY_SIZE(s5k4ecgx_confs),
};

static struct msm_sensor_ctrl_t s5k4ecgx_s_ctrl = {
	.msm_sensor_reg = &s5k4ecgx_regs,
	.sensor_i2c_client = &s5k4ecgx_sensor_i2c_client,
	.sensor_i2c_addr = 0xAC,
	.sensor_output_reg_addr = &s5k4ecgx_reg_addr,
	.sensor_id_info = &s5k4ecgx_id_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csic_params = &s5k4ecgx_csi_params_array[0],
	.msm_sensor_mutex = &s5k4ecgx_mut,
	.sensor_i2c_driver = &s5k4ecgx_i2c_driver,
	.sensor_v4l2_subdev_info = s5k4ecgx_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(s5k4ecgx_subdev_info),
	.sensor_v4l2_subdev_ops = &s5k4ecgx_subdev_ops,
	.func_tbl = &s5k4ecgx_func_tbl,
	.clk_rate = MSM_SENSOR_MCLK_24HZ,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Samsung 5MP YUV sensor driver");
MODULE_LICENSE("GPL v2");
