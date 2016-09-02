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

#include "sr200pc20.h"
#include "sr200pc20_regs.h"

#define SENSOR_NAME "sr200pc20"

/*#define CONFIG_LOAD_FILE */
#ifdef CONFIG_LOAD_FILE

#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/string.h>
static char *sr200pc20_regs_table;
static int sr200pc20_regs_table_size;
static int sr200pc20_write_regs_from_sd(char *name);
static int sr200pc20_i2c_write_multi(unsigned short addr, unsigned int w_data);
struct test {
	u8 data;
	struct test *nextBuf;
};
static struct test *testBuf;
static s32 large_file;
#endif
 
static void sr200pc20_set_ev(int ev);

#define SR200PC20_WRT_LIST(A)	\
	sr200pc20_i2c_wrt_list(A, (sizeof(A) / sizeof(A[0])), #A);

#define CAM_REV ((system_rev <= 1) ? 0 : 1)

#ifdef CONFIG_LOAD_FILE

void sr200pc20_regs_table_init(void)
{
	struct file *fp;
	struct test *nextBuf = NULL;

	u8 *nBuf = NULL;
	size_t file_size = 0, max_size = 0, testBuf_size = 0;
	ssize_t nread = 0;
	s32 check = 0, starCheck = 0;
	s32 tmp_large_file = 0;
	s32 i = 0;
	int ret = 0;
	loff_t pos;
	cam_err("CONFIG_LOAD_FILE is enable!!\n");

	/*Get the current address space */
	mm_segment_t fs = get_fs();

	CAM_DEBUG("%s %d", __func__, __LINE__);

	/*Set the current segment to kernel data segment */
	set_fs(get_ds());

	fp = filp_open("/mnt/sdcard/sr200pc20_regs.h", O_RDONLY, 0);
	if (IS_ERR(fp)) {
		cam_err("failed to open /mnt/sdcard/sr200pc20_regs.h");
		return PTR_ERR(fp);
	}

	file_size = (size_t) fp->f_path.dentry->d_inode->i_size;
	max_size = file_size;
	cam_err("file_size = %d", file_size);
	nBuf = kmalloc(file_size, GFP_ATOMIC);
	if (nBuf == NULL) {
		cam_err("Fail to 1st get memory");
		nBuf = vmalloc(file_size);
		if (nBuf == NULL) {
			cam_err("ERR: nBuf Out of Memory");
			ret = -ENOMEM;
			goto error_out;
		}
		tmp_large_file = 1;
	}

	testBuf_size = sizeof(struct test) * file_size;
	if (tmp_large_file) {
		testBuf = vmalloc(testBuf_size);
		large_file = 1;
	} else {
		testBuf = kmalloc(testBuf_size, GFP_ATOMIC);
		if (testBuf == NULL) {
			cam_err("Fail to get mem(%d bytes)", testBuf_size);
			testBuf = vmalloc(testBuf_size);
			large_file = 1;
		}
	}
	if (testBuf == NULL) {
		cam_err("ERR: Out of Memory");
		ret = -ENOMEM;
		goto error_out;
	}

	pos = 0;
	memset(nBuf, 0, file_size);
	memset(testBuf, 0, file_size * sizeof(struct test));
	nread = vfs_read(fp, (char __user *)nBuf, file_size, &pos);
	if (nread != file_size) {
		cam_err("failed to read file ret = %d", nread);
		ret = -1;
		goto error_out;
	}

	set_fs(fs);

	i = max_size;

	cam_err("i = %d", i);

	while (i) {
		testBuf[max_size - i].data = *nBuf;
		if (i != 1) {
			testBuf[max_size - i].nextBuf =
						&testBuf[max_size - i + 1];
		} else {
			testBuf[max_size - i].nextBuf = NULL;
			break;
		}
		i--;
		nBuf++;
	}
	i = max_size;
	nextBuf = &testBuf[0];

	while (i - 1) {
		if (!check && !starCheck) {
			if (testBuf[max_size - i].data == '/') {
				if (testBuf[max_size-i].nextBuf != NULL) {
					if (testBuf[max_size-i].nextBuf->data
								== '/') {
						check = 1;/* when find '//' */
						i--;
					} else if (testBuf[max_size-i].nextBuf->
								data == '*') {
						starCheck = 1;/*when'/ *' */
						i--;
					}
				} else
					break;
			}
			if (!check && !starCheck) {
				/* ignore '\t' */
				if (testBuf[max_size - i].data != '\t') {
					nextBuf->nextBuf = &testBuf[max_size-i];
					nextBuf = &testBuf[max_size - i];
				}
			}
		} else if (check && !starCheck) {
			if (testBuf[max_size - i].data == '/') {
				if (testBuf[max_size-i].nextBuf != NULL) {
					if (testBuf[max_size-i].nextBuf->
								data == '*') {
						starCheck = 1; /*when '/ *' */
						check = 0;
						i--;
					}
				} else
					break;
			}

			 /* when find '\n' */
			if (testBuf[max_size - i].data == '\n' && check) {
				check = 0;
				nextBuf->nextBuf = &testBuf[max_size - i];
				nextBuf = &testBuf[max_size - i];
			}

		} else if (!check && starCheck) {
			if (testBuf[max_size - i].data
						== '*') {
				if (testBuf[max_size-i].nextBuf != NULL) {
					if (testBuf[max_size-i].nextBuf->
								data == '/') {
						starCheck = 0; /*when'* /' */
						i--;
					}
				} else
					break;
			}
		}

		i--;

		if (i < 2) {
			nextBuf = NULL;
			break;
		}

		if (testBuf[max_size - i].nextBuf == NULL) {
			nextBuf = NULL;
			break;
		}
	}

#if FOR_DEBUG /* for print */
	printk(KERN_DEBUG "i = %d\n", i);
	nextBuf = &testBuf[0];
	while (1) {
		if (nextBuf->nextBuf == NULL)
			break;
		/*printk(KERN_DEBUG "DATA---%c", nextBuf->data);*/
		nextBuf = nextBuf->nextBuf;
	}
#endif
	tmp_large_file ? vfree(nBuf) : kfree(nBuf);

error_out:
	if (fp)
		filp_close(fp, current->files);
	return ret;
}
static inline int sr200pc20_write(struct i2c_client *client,
		u16 packet)
{
	u8 buf[2];
	int err = 0, retry_count = 5;

	struct i2c_msg msg;

	if (!client->adapter) {
		cam_err("ERR - can't search i2c client adapter");
		return -EIO;
	}

	buf[0] = (u8) (packet >> 8);
	buf[1] = (u8) (packet & 0xff);

	msg.addr = sr200pc20_client->addr >> 1
	msg.flags = 0;
	msg.len = 2;
	msg.buf = buf;

	do {
		err = i2c_transfer(sr200pc20_client->adapter, &msg, 1);
		if (err == 1)
			return 0;
		retry_count++;
		cam_err("i2c transfer failed, retrying %x err:%d\n",
		       packet, err);
		usleep(3000);

	} while (retry_count <= 5);

	return (err != 1) ? -1 : 0;
}


static int sr200pc20_write_regs_from_sd(char *name)
{
	struct test *tempData;

	int ret = -EAGAIN;
	u16 temp;
	u16 delay = 0;
	u8 data[7];
	s32 searched = 0;
	size_t size = strlen(name);
	s32 i;
	/*msleep(10000);*/
	/*printk("E size = %d, string = %s\n", size, name);*/
	tempData = &testBuf[0];

	*(data + 6) = '\0';

	while (!searched) {
		searched = 1;
		for (i = 0; i < size; i++) {
			if (tempData != NULL) {
				if (tempData->data != name[i]) {
					searched = 0;
				break;
			}
		} else {
		cam_err("tempData is NULL");
		}
			tempData = tempData->nextBuf;
		}
		tempData = tempData->nextBuf;
	}

	/* structure is get..*/
	while (1) {
		if (tempData->data == '{')
			break;
		else
			tempData = tempData->nextBuf;
	}

	while (1) {
		searched = 0;
		while (1) {
			if (tempData->data == 'x') {
				/* get 6 strings.*/
				data[0] = '0';
				for (i = 1; i < 6; i++) {
					data[i] = tempData->data;
					tempData = tempData->nextBuf;
				}
				kstrtoul(data, 16, &temp);
				/*CAM_DEBUG("%s\n", data);
				CAM_DEBUG("kstrtoul data = 0x%x\n", temp);*/
				break;
			} else if (tempData->data == '}') {
				searched = 1;
				break;
			} else
				tempData = tempData->nextBuf;

			if (tempData->nextBuf == NULL)
				return NULL;
		}

		if (searched)
			break;
		if ((temp & 0xFF00) == SR200PC20_DELAY) {
			delay = temp & 0xFF;
			cam_info("sr200pc20 delay(%d)", delay);
			/*step is 10msec */
			mdelay(delay);
			continue;
		}
		ret = sr200pc20_write(sr200pc20_client, temp);

		/* In error circumstances */
		/* Give second shot */
		if (unlikely(ret)) {
			ret = sr200pc20_write(sr200pc20_client, temp);

			/* Give it one more shot */
			if (unlikely(ret))
				ret = sr200pc20_write(sr200pc20_client, temp);
			}
		}

	return 0;
}

void sr200pc20_regs_table_exit(void)
{
	if (testBuf == NULL)
		return;
	else {
		large_file ? vfree(testBuf) : kfree(testBuf);
		large_file = 0;
		testBuf = NULL;
	}
}

#endif

/**
 * sr200pc20_i2c_read_multi: Read (I2C) multiple bytes to the camera sensor
 * @client: pointer to i2c_client
 * @cmd: command register
 * @w_data: data to be written
 * @w_len: length of data to be written
 * @r_data: buffer where data is read
 * @r_len: number of bytes to read
 *
 * Returns 0 on success, <0 on error
 */

/**
 * sr200pc20_i2c_read: Read (I2C) multiple bytes to the camera sensor
 * @client: pointer to i2c_client
 * @cmd: command register
 * @data: data to be read
 *
 * Returns 0 on success, <0 on error
 */
static int sr200pc20_i2c_read(unsigned char subaddr, unsigned char *data)
{
	unsigned char buf[1];
	struct i2c_msg msg = {sr200pc20_client->addr >> 1, 0, 1, buf};

	int err = 0;
	buf[0] = subaddr;

	if (!sr200pc20_client->adapter)
		return -EIO;

	err = i2c_transfer(sr200pc20_client->adapter, &msg, 1);
	if (unlikely(err < 0))
		return -EIO;

	msg.flags = I2C_M_RD;

	err = i2c_transfer(sr200pc20_client->adapter, &msg, 1);
	if (unlikely(err < 0))
		return -EIO;
	/*
	 * Data comes in Little Endian in parallel mode; So there
	 * is no need for byte swapping here
	 */

	*data = buf[0];

	return err;
}

/**
 * sr200pc20_i2c_write_multi: Write (I2C) multiple bytes to the camera sensor
 * @client: pointer to i2c_client
 * @cmd: command register
 * @w_data: data to be written
 * @w_len: length of data to be written
 *
 * Returns 0 on success, <0 on error
 */
static int sr200pc20_i2c_write_multi(unsigned short addr, unsigned int w_data)
{
	int32_t rc = -EFAULT;
	int retry_count = 0;

	unsigned char buf[2];

	struct i2c_msg msg;

	buf[0] = (u8) (addr >> 8);
	buf[1] = (u8) (w_data & 0xff);

	msg.addr = sr200pc20_client->addr >> 1;
	msg.flags = 0;
	msg.len = 1;
	msg.buf = buf;


	cam_err("I2C CHIP ID=0x%x, DATA 0x%x 0x%x\n",
			sr200pc20_client->addr, buf[0], buf[1]);

	do {
		rc = i2c_transfer(sr200pc20_client->adapter, &msg, 1);
		if (rc == 1)
			return 0;
		retry_count++;
		cam_err("retry_count %d\n", retry_count);
		usleep(3000);

	} while (retry_count <= 5);

	return 0;
}


static int32_t sr200pc20_i2c_write_16bit(u16 packet)
{
	int32_t rc = -EFAULT;
	int retry_count = 0;

	unsigned char buf[2];

	struct i2c_msg msg;
//	uint16_t chipid = 0;

	buf[0] = (u8) (packet >> 8);
	buf[1] = (u8) (packet & 0xff);

	msg.addr = sr200pc20_client->addr >> 1;
	msg.flags = 0;
	msg.len = 2;
	msg.buf = buf;

#if 0
	printk("I2C CHIP ID=0x%x, DATA 0x%x 0x%x\n",
			sr200pc20_client->addr, buf[0], buf[1]);
#endif
		//msm_camera_i2c_read(sr200pc20_client->adapter, msg.addr, &chipid, MSM_CAMERA_I2C_BYTE_ADDR);   
		// printk("#####2222 test msm_camera_i2c_read : 0x%x \n", chipid);

	do {
		rc = i2c_transfer(sr200pc20_client->adapter, &msg, 1);
		if (rc == 1){
			// msm_camera_i2c_read(sr200pc20_client, msg.addr, &chipid, MSM_CAMERA_I2C_BYTE_ADDR);   
			// printk("##### test msm_camera_i2c_read : 0x%x \n", chipid);
			return 0;
			}
		retry_count++;
		cam_err("i2c transfer failed, retrying %x err:%d\n",
		       packet, rc);
		usleep(3000);
		return -EIO;

	} while (retry_count <= 5);

	return 0;
}

static int sr200pc20_i2c_wrt_list(const u16 *regs,
	int size, char *name)
{
#ifdef CONFIG_LOAD_FILE
	sr200pc20_write_regs_from_sd(name);
#else

	int i;
	u8 m_delay = 0;
	u16 temp_packet;

	CAM_DEBUG("%s, size=%d", name, size);
	for (i = 0; i < size; i++) {
		temp_packet = regs[i];

		if ((temp_packet & SR200PC20_DELAY) == SR200PC20_DELAY) {
			m_delay = temp_packet & 0xFF;
			cam_info("delay = %d", m_delay*10);
			mdelay(m_delay*10);/*step is 10msec*/
			continue;
		}

		if (sr200pc20_i2c_write_16bit(temp_packet) < 0) {
			cam_err("fail(0x%x, 0x%x:%d)",
					sr200pc20_client->addr, temp_packet, i);
			return -EIO;
		}
		/*udelay(10);*/
	}
#endif

	return 0;
}

static int sr200pc20_set_exif(void)
{
	int err = 0;
	u8 read_value1 = 0;
	u8 read_value2 = 0;
	u8 read_value3 = 0;
	u8 read_value4 = 0;
    int OPCLK = 12000000;
	unsigned short gain_value;

    /* Exposure Time */
	sr200pc20_i2c_write_16bit(0x0320);
	sr200pc20_i2c_read(0x80, &read_value1);
	sr200pc20_i2c_read(0x81, &read_value2);
	sr200pc20_i2c_read(0x82, &read_value3);
	sr200pc20_exif.shutterspeed = OPCLK / ((read_value1 << 19)
		+ (read_value2 << 11) + (read_value3 << 3));
	CAM_DEBUG("Exposure time = %d\n", sr200pc20_exif.shutterspeed);

	sr200pc20_i2c_write_16bit(0x0320);
	sr200pc20_i2c_read(0xb0, &read_value4);
    /* gain_value = (read_value4 / 32 ) + 0.5; */
	gain_value = (read_value4 / 32) * 1000;

	if (gain_value < 626)
		sr200pc20_exif.iso = 50;
	else if (gain_value < 1026)
		sr200pc20_exif.iso = 100;
	else if (gain_value < 2260)
		sr200pc20_exif.iso = 200;
	else
		sr200pc20_exif.iso = 400;

	CAM_DEBUG("ISO = %d\n", sr200pc20_exif.iso);
	return err;
}


static int sr200pc20_get_exif(int exif_cmd, unsigned short value2)
{
	unsigned short val = 0;

	switch (exif_cmd) {
	case EXIF_EXPOSURE_TIME:
		val = sr200pc20_exif.shutterspeed;
		break;

	case EXIF_ISO:
		val = sr200pc20_exif.iso;
		break;

	default:
		break;
	}

	return val;
}

static void sr200pc20_set_init_mode(void)
{
	sr200pc20_control.cam_mode = PREVIEW_MODE;
	sr200pc20_control.vtcall_mode = 0;
}

void sr200pc20_set_preview_size(int32_t index)
{
	CAM_DEBUG("index %d", index);

	sr200pc20_control.settings.preview_size_idx = index;
}

void sr200pc20_set_frame_rate(int32_t fps)
{
	CAM_DEBUG(" %d", fps);

	switch (fps) {
	case 15:
		SR200PC20_WRT_LIST(sr200pc20_15fps);
		break;

	case 20:
	case 30: //for temp, should change HAL
		SR200PC20_WRT_LIST(sr200pc20_20fps);
		break;

	default:
		SR200PC20_WRT_LIST(sr200pc20_Auto_fps);
		break;
	}

	sr200pc20_control.settings.fps = fps;
}
	
static int32_t sr200pc20_sensor_setting(int update_type, int rt)
{
	int32_t rc = 0;

	CAM_DEBUG("Start");

	switch (update_type) {

	case REG_INIT:
		break;

	case UPDATE_PERIODIC:
		msm_sensor_enable_debugfs(sr200pc20_control.s_ctrl);
		if (rt == RES_PREVIEW || rt == RES_CAPTURE) {
			CAM_DEBUG("UPDATE_PERIODIC");
		}
		msleep(30);
		break;
	default:
		rc = -EINVAL;
		break;
	}

	return rc;
}

static int sr200pc20_sensor_match_id(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc ;
    int chipid = 0, version = 0;

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x03, 0x00, MSM_CAMERA_I2C_BYTE_DATA);
    rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, 0x04, &chipid, MSM_CAMERA_I2C_BYTE_ADDR);
	if (rc < 0) {
		pr_err("%s: %s: read id failed\n", __func__,
			s_ctrl->sensordata->sensor_name);
	}

    printk("msm_sensor %s id: 0x%x, version = 0x%x \n",
        s_ctrl->sensordata->sensor_name, chipid, version);

	return rc;
}

static struct msm_cam_clk_info cam_clk_info[] = {
	{"cam_m_clk", MSM_SENSOR_MCLK_24HZ},
};

#define ARUBA_CAM_A_EN 128
#define ARUBA_CAM_C_EN	107
#define ARUBA_CAM_AF_EN	129
#define ARUBA_CAM_FLASH_EN	14
#define ARUBA_CAM_FLASH_SET	11
#define ARUBA_CAM_STBY	96
#define ARUBA_CAM_RESET	85
#define ARUBA_CAM_IO_EN	4

#define FRONT_CAM_STBY  75
#define FRONT_CAM_RESET 80
#define FRONT_CAM_C_EN	73


int32_t sr200pc20_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;

#ifdef CONFIG_LOAD_FILE
	sr200pc20_regs_table_init();
#endif

	printk("[CAMDRV] ### sr200pc20_sensor_power_up ###\n");

	s_ctrl->reg_ptr = kzalloc(sizeof(struct regulator *)
			* data->sensor_platform_info->num_vreg, GFP_KERNEL);
	if (!s_ctrl->reg_ptr) {
		pr_err("[CAMDRV] %s: could not allocate mem for regulators\n",
			__func__);
		return -ENOMEM;
	}
    
	rc = msm_camera_config_vreg(&s_ctrl->sensor_i2c_client->client->dev,
			s_ctrl->sensordata->sensor_platform_info->cam_vreg,
			s_ctrl->sensordata->sensor_platform_info->num_vreg,
			s_ctrl->reg_ptr, 1);
	if (rc < 0) {
		pr_err("[CAMDRV] %s: regulator on failed\n", __func__);
		goto config_vreg_failed;
	}

	rc = msm_camera_enable_vreg(&s_ctrl->sensor_i2c_client->client->dev,
			s_ctrl->sensordata->sensor_platform_info->cam_vreg,
			s_ctrl->sensordata->sensor_platform_info->num_vreg,
			s_ctrl->reg_ptr, 1);
	if (rc < 0) {
		pr_err("[CAMDRV] %s: enable regulator failed\n", __func__);
		goto enable_vreg_failed;
	}

	gpio_request(ARUBA_CAM_STBY, "aruba_stby");
	gpio_request(ARUBA_CAM_RESET, "aruba_reset");
//	gpio_request(ARUBA_CAM_IO_EN, "aruba_a_en");
	gpio_request(ARUBA_CAM_C_EN, "aruba_c_en");
	gpio_request(ARUBA_CAM_AF_EN, "aruba_af_en");
	gpio_request(FRONT_CAM_STBY, "front_cam_stby");
	gpio_request(FRONT_CAM_RESET, "front_cam_reset");

	if(rc) {
		pr_err("[CAMDRV] %s: gpio_request failed\n", __func__);
	}

	gpio_direction_output(ARUBA_CAM_STBY, 0);
	gpio_direction_output(ARUBA_CAM_RESET, 0);
	//gpio_direction_output(ARUBA_CAM_IO_EN, 0);
	gpio_direction_output(ARUBA_CAM_C_EN, 0);
	gpio_direction_output(ARUBA_CAM_AF_EN, 0);
	gpio_direction_output(FRONT_CAM_STBY, 0);
	gpio_direction_output(FRONT_CAM_RESET, 0);
    
	gpio_set_value_cansleep(ARUBA_CAM_STBY, 0);		//STBYN :: MSM_GPIO 96
	gpio_set_value_cansleep(ARUBA_CAM_RESET, 0);	//RSTN :: MSM_GPIO 85
//	gpio_set_value_cansleep(ARUBA_CAM_IO_EN, 0);
	gpio_set_value_cansleep(ARUBA_CAM_C_EN, 0);
	gpio_set_value_cansleep(FRONT_CAM_STBY, 0);
	gpio_set_value_cansleep(FRONT_CAM_RESET, 0);
    
	gpio_set_value(ARUBA_CAM_A_EN, 1);  // VVT_2.8V and CAM_A_2.8V
	mdelay(1);
	gpio_set_value(FRONT_CAM_C_EN, 1);   //2M FRONT_C_EN 
	mdelay(1);
    gpio_set_value(ARUBA_CAM_C_EN, 1);	// 5M Core 1.2 high
    mdelay(3);
    gpio_set_value(ARUBA_CAM_C_EN, 0);	//5M Core 1.2 Low
      mdelay(1);
	gpio_set_value(FRONT_CAM_STBY, 1);
	mdelay(1);
	
		if (s_ctrl->clk_rate != 0)
		cam_clk_info->clk_rate = s_ctrl->clk_rate;

	rc = msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
		cam_clk_info, &s_ctrl->cam_clk, ARRAY_SIZE(cam_clk_info), 1);
	if (rc < 0) {
		pr_err("[CAMDRV] %s: clk enable failed\n", __func__);
		goto enable_clk_failed;
	}
	
	mdelay(35);
    gpio_set_value(FRONT_CAM_RESET, 1);  // 5M Core 1.2 High
    mdelay(1);
     
	if (data->sensor_platform_info->ext_power_ctrl != NULL)
		data->sensor_platform_info->ext_power_ctrl(1);

	if (data->sensor_platform_info->i2c_conf &&
		data->sensor_platform_info->i2c_conf->use_i2c_mux)
		msm_sensor_enable_i2c_mux(data->sensor_platform_info->i2c_conf);

	return rc;

enable_clk_failed:

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
request_gpio_failed:
	kfree(s_ctrl->reg_ptr);
	return rc;
}

int32_t sr200pc20_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;
    
	printk("[CAMDRV] ### sr200pc20_sensor_power_down ###\n");
    
	if (data->sensor_platform_info->i2c_conf &&
		data->sensor_platform_info->i2c_conf->use_i2c_mux)
		msm_sensor_disable_i2c_mux(
			data->sensor_platform_info->i2c_conf);

	if (data->sensor_platform_info->ext_power_ctrl != NULL)
		data->sensor_platform_info->ext_power_ctrl(0);
	msleep(1);
    
	gpio_set_value_cansleep(FRONT_CAM_RESET, 0);	//VT_CAM_RESET low

    // MCLK disable 
	msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
		cam_clk_info, &s_ctrl->cam_clk, ARRAY_SIZE(cam_clk_info), 0);

	gpio_set_value_cansleep(FRONT_CAM_STBY, 0);		// VT_CAM_STBY low	


	gpio_set_value_cansleep(FRONT_CAM_C_EN, 0);
	msleep(10);
	gpio_set_value_cansleep(ARUBA_CAM_A_EN, 0);
	
		msm_camera_enable_vreg(&s_ctrl->sensor_i2c_client->client->dev,
		s_ctrl->sensordata->sensor_platform_info->cam_vreg,
		s_ctrl->sensordata->sensor_platform_info->num_vreg,
		s_ctrl->reg_ptr, 0);
	msm_camera_config_vreg(&s_ctrl->sensor_i2c_client->client->dev,
		s_ctrl->sensordata->sensor_platform_info->cam_vreg,
		s_ctrl->sensordata->sensor_platform_info->num_vreg,
		s_ctrl->reg_ptr, 0);
	kfree(s_ctrl->reg_ptr);
	
	msleep(1);

	gpio_free(ARUBA_CAM_C_EN);
	gpio_free(ARUBA_CAM_AF_EN);
	gpio_free(ARUBA_CAM_STBY);
	gpio_free(ARUBA_CAM_RESET);
	gpio_free(FRONT_CAM_RESET);
	gpio_free(FRONT_CAM_STBY);    
    


#ifdef CONFIG_LOAD_FILE
	sr200pc20_regs_table_exit();
#endif

	return 0;
}

static void sr200pc20_set_ev(int ev)
{
	CAM_DEBUG("[sr200pc20] %s : %d", __func__, ev);

	switch (ev) {
	case CAMERA_EV_M4:
		SR200PC20_WRT_LIST(sr200pc20_brightness_M4);
		break;

	case CAMERA_EV_M3:
		SR200PC20_WRT_LIST(sr200pc20_brightness_M3);
		break;

	case CAMERA_EV_M2:
		SR200PC20_WRT_LIST(sr200pc20_brightness_M2);
		break;

	case CAMERA_EV_M1:
		SR200PC20_WRT_LIST(sr200pc20_brightness_M1);
		break;

	case CAMERA_EV_DEFAULT:
		SR200PC20_WRT_LIST(sr200pc20_brightness_default);
		break;

	case CAMERA_EV_P1:
		SR200PC20_WRT_LIST(sr200pc20_brightness_P1);
		break;

	case CAMERA_EV_P2:
		SR200PC20_WRT_LIST(sr200pc20_brightness_P2);
		break;

	case CAMERA_EV_P3:
		SR200PC20_WRT_LIST(sr200pc20_brightness_P3);
		break;

	case CAMERA_EV_P4:
		SR200PC20_WRT_LIST(sr200pc20_brightness_P4);
		break;

	default:
		CAM_DEBUG("[sr200pc20] unexpected ev mode %s/%d",
			__func__, __LINE__);
		break;
	}

	sr200pc20_control.settings.brightness = ev;
}

static void sr200pc20_set_effect(int effect)
{
	CAM_DEBUG(" %d", effect);

	switch (effect) {
	case CAMERA_EFFECT_OFF:
		SR200PC20_WRT_LIST(sr200pc20_Effect_Normal);
		break;

	case CAMERA_EFFECT_MONO:
		SR200PC20_WRT_LIST(sr200pc20_Effect_Gray);
		break;

	case CAMERA_EFFECT_NEGATIVE:
		SR200PC20_WRT_LIST(sr200pc20_Effect_Negative);
		break;

	case CAMERA_EFFECT_SEPIA:
		SR200PC20_WRT_LIST(sr200pc20_Effect_Sepia);
		break;

	default:
		cam_info(" effect : default");
		SR200PC20_WRT_LIST(sr200pc20_Effect_Normal);
		return 0;
	}

	sr200pc20_control.settings.effect = effect;

	return 0;
}


static void sr200pc20_set_whitebalance(int wb)
{
	CAM_DEBUG(" %d", wb);

	switch (wb) {
	case CAMERA_WHITE_BALANCE_AUTO:
		SR200PC20_WRT_LIST(sr200pc20_WB_Auto);
		break;

	case CAMERA_WHITE_BALANCE_INCANDESCENT:
		SR200PC20_WRT_LIST(sr200pc20_WB_Incandescent);
		break;

	case CAMERA_WHITE_BALANCE_FLUORESCENT:
		SR200PC20_WRT_LIST(sr200pc20_WB_Fluorescent);
		break;

	case CAMERA_WHITE_BALANCE_DAYLIGHT:
		SR200PC20_WRT_LIST(sr200pc20_WB_Daylight);
		break;

	case CAMERA_WHITE_BALANCE_CLOUDY_DAYLIGHT:
		SR200PC20_WRT_LIST(sr200pc20_WB_Cloudy);
		break;

	default:
		cam_info(" WB : default");
		return 0;
	}

	sr200pc20_control.settings.wb= wb;

	return 0;
}
	

void sensor_native_control_front(void __user *arg)
{
	struct ioctl_native_cmd ctrl_info;
	struct msm_camera_v4l2_ioctl_t *ioctl_ptr = arg;

	if (copy_from_user(&ctrl_info,
		(void __user *)ioctl_ptr->ioctl_ptr,
		sizeof(ctrl_info))) {
		cam_err("fail copy_from_user!");
        goto FAIL_END;
	}

	CAM_DEBUG("mode : %d", ctrl_info.mode);

	switch (ctrl_info.mode) {

	case EXT_CAM_EV:
		sr200pc20_set_ev(ctrl_info.value_1);
		break;

	case  EXT_CAM_MOVIE_MODE:
		CAM_DEBUG("MOVIE mode : %d", ctrl_info.value_1);
		sr200pc20_control.cam_mode = ctrl_info.value_1;
		break;

	case EXT_CAM_EXIF:
		ctrl_info.value_1 = sr200pc20_get_exif(ctrl_info.address,
			ctrl_info.value_2);
		break;
	case EXT_CAM_VT_MODE:
		CAM_DEBUG(" VT mode : %d", ctrl_info.value_1);
		sr200pc20_control.vtcall_mode = ctrl_info.value_1;
		break;

	case EXT_CAM_EFFECT:
		sr200pc20_set_effect(ctrl_info.value_1);
		break;

	case EXT_CAM_WB:
		sr200pc20_set_whitebalance(ctrl_info.value_1);
		break;
/*
	case EXT_CAM_DTP_TEST:
		sr200pc20_check_dataline(ctrl_info.value_1);
		break;
*/
	case EXT_CAM_PREVIEW_SIZE:
		sr200pc20_set_preview_size(ctrl_info.value_1);
		break;

	case EXT_CAM_SET_FPS:
		CAM_DEBUG("EXT_CAM_SET_FPS : %d\n",ctrl_info.value_1);
		sr200pc20_set_frame_rate(ctrl_info.value_1);
		break;

	default:
		CAM_DEBUG("[sr200pc20] default mode");
		break;
	}

	if (copy_to_user((void __user *)ioctl_ptr->ioctl_ptr,
		(const void *)&ctrl_info,
			sizeof(ctrl_info))) {
		cam_err("fail copy_to_user!");
        goto FAIL_END;
	}

    return;

FAIL_END:
	printk("Error : can't handle native control\n");
}

static struct v4l2_subdev_info sr200pc20_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_YUYV8_2X8,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	/* more can be supported, to be added later */
};

void sr200pc20_sensor_start_stream(struct msm_sensor_ctrl_t *s_ctrl)
{
	CAM_DEBUG("Preview_Mode \n");

	//SR200PC20_WRT_LIST(sr200pc20_Init_Reg); //Init setting is included in fps setting
	//sr200pc20_WRT_LIST(sr200pc20_Preview);
}

void sr200pc20_sensor_stop_stream(struct msm_sensor_ctrl_t *s_ctrl)
{
	CAM_DEBUG("Enter \n");
}

void sr200pc20_sensor_preview_mode(struct msm_sensor_ctrl_t *s_ctrl)
{
	CAM_DEBUG("Enter \n");
	SR200PC20_WRT_LIST(sr200pc20_Preview);
}

void sr200pc20_sensor_capture_mode(struct msm_sensor_ctrl_t *s_ctrl)
{
	CAM_DEBUG("Enter \n");
	SR200PC20_WRT_LIST(sr200pc20_Capture);
	sr200pc20_set_exif();

}

static int sr200pc20_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc;
	struct msm_sensor_ctrl_t *s_ctrl;

	cam_err("%s_i2c_probe called \n", client->name);

	gpio_tlmm_config(GPIO_CFG(FRONT_CAM_STBY, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(FRONT_CAM_RESET, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(FRONT_CAM_C_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		//rc = -ENOTSUPP;
		cam_err("i2c_check_functionality failed\n");
		rc = -EFAULT;
		return rc;
	}

	s_ctrl = (struct msm_sensor_ctrl_t *)(id->driver_data);
	if (s_ctrl->sensor_i2c_client != NULL) {
		s_ctrl->sensor_i2c_client->client = client;
		if (s_ctrl->sensor_i2c_addr != 0)
			s_ctrl->sensor_i2c_client->client->addr =
				s_ctrl->sensor_i2c_addr;
	} else {
		cam_err("s_ctrl->sensor_i2c_client is NULL\n");
		rc = -EFAULT;
		return rc;
	}

	s_ctrl->sensordata = client->dev.platform_data;
	if (s_ctrl->sensordata == NULL) {
		cam_err("NULL sensor data \n");
		return -EFAULT;
	}
/*	//kk0704.park :: REMOVE Camera Power control at booting time
	rc = s_ctrl->func_tbl->sensor_power_up(s_ctrl);
	if (rc < 0) {
		pr_err("%s %s power up failed\n", __func__, client->name);
		return rc;
	}

	if (s_ctrl->func_tbl->sensor_match_id)
		rc = s_ctrl->func_tbl->sensor_match_id(s_ctrl);
	else
		rc = msm_sensor_match_id(s_ctrl);
	if (rc < 0)
		return rc;
*/
	sr200pc20_client = client;
	sr200pc20_dev = s_ctrl->sensor_i2c_client->client->dev;

	cam_err("sr200pc20_probe succeeded! \n");

	snprintf(s_ctrl->sensor_v4l2_subdev.name,
		sizeof(s_ctrl->sensor_v4l2_subdev.name), "%s", id->name);
	v4l2_i2c_subdev_init(&s_ctrl->sensor_v4l2_subdev, client,
		s_ctrl->sensor_v4l2_subdev_ops);
	s_ctrl->sensor_v4l2_subdev.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	media_entity_init(&s_ctrl->sensor_v4l2_subdev.entity, 0, NULL, 0);
	s_ctrl->sensor_v4l2_subdev.entity.type = MEDIA_ENT_T_V4L2_SUBDEV;
	s_ctrl->sensor_v4l2_subdev.entity.group_id = SENSOR_DEV;
	s_ctrl->sensor_v4l2_subdev.entity.name =
		s_ctrl->sensor_v4l2_subdev.name;
	msm_sensor_register(&s_ctrl->sensor_v4l2_subdev);
	s_ctrl->sensor_v4l2_subdev.entity.revision =
		s_ctrl->sensor_v4l2_subdev.devnode->num;
	goto power_down;

probe_failure:
	cam_err("sr200pc20_probe failed!");
	return rc;

power_down:
	if (rc > 0)
		rc = 0;
//kk0704.park :: REMOVE Camera Power control at booting time	s_ctrl->func_tbl->sensor_power_down(s_ctrl);
	return rc;
}

static struct msm_sensor_output_info_t sr200pc20_dimensions[] = {
	{
		.x_output = 1600,
		.y_output = 1200,
		.line_length_pclk = 1600,
		.frame_length_lines = 1200,
		.vt_pixel_clk = 9216000,
		.op_pixel_clk = 9216000,
		.binning_factor = 1,
	},	
	{
		.x_output = 0x280,
		.y_output = 0x1E0,
		.line_length_pclk = 0x280,
		.frame_length_lines = 0x1E0,
		.vt_pixel_clk = 9216000,
		.op_pixel_clk = 9216000,
		.binning_factor = 1,
	},
};

static struct msm_camera_csi_params sr200pc20_csi_params = {
	.data_format = CSI_8BIT,
	.lane_cnt    = 1,
	.lane_assign = 0xe4,
	.dpcm_scheme = 0,
	.settle_cnt  = 0x14,
};

static struct msm_camera_csi_params *sr200pc20_csi_params_array[] = {
	&sr200pc20_csi_params,
	&sr200pc20_csi_params,
};

static struct msm_sensor_output_reg_addr_t sr200pc20_reg_addr = {
	.x_output = 0x280,
	.y_output = 0x1E0,
	.line_length_pclk = 0x280,
	.frame_length_lines = 0x1E0,
};

static struct msm_sensor_id_info_t sr200pc20_id_info = {
	.sensor_id_reg_addr = 0x04,
	.sensor_id = 0xb4,
	.sensor_version = 0x00,	
};

static const struct i2c_device_id sr200pc20_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&sr200pc20_s_ctrl},
	{},
};

static struct i2c_driver sr200pc20_i2c_driver = {
	.id_table = sr200pc20_i2c_id,
	.probe  = sr200pc20_i2c_probe,
	.driver = {
		.name = "msm_camera_sr200pc20",
	},
};

static struct msm_camera_i2c_client sr200pc20_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
};

static int __init sr200pc20_init(void)
{
    printk("sr200pc20\n");
	return i2c_add_driver(&sr200pc20_i2c_driver);
}

static struct msm_camera_i2c_reg_conf sr200pc20_capture_settings[] = {
	{0x03, 0x00},
	{0x11, 0x90}, /*B[0]_horizontal flip funtion(0:off,1:on)*/
};

static struct msm_camera_i2c_reg_conf sr200pc20_preview_return_settings[] = {
	{0xFF, 0x00},
};

static struct msm_camera_i2c_conf_array sr200pc20_confs[] = {
	{&sr200pc20_capture_settings[0],
	ARRAY_SIZE(sr200pc20_capture_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&sr200pc20_preview_return_settings[0],
	ARRAY_SIZE(sr200pc20_preview_return_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_sensor_reg_t sr200pc20_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.output_settings = &sr200pc20_dimensions[0],
	.num_conf = ARRAY_SIZE(sr200pc20_confs),
};

static struct v4l2_subdev_core_ops sr200pc20_subdev_core_ops = {
	.s_ctrl = msm_sensor_v4l2_s_ctrl,
	.queryctrl = msm_sensor_v4l2_query_ctrl,
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops sr200pc20_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops sr200pc20_subdev_ops = {
	.core = &sr200pc20_subdev_core_ops,
	.video  = &sr200pc20_subdev_video_ops,
};

static struct msm_sensor_fn_t sr200pc20_func_tbl = {
	.sensor_start_stream = sr200pc20_sensor_start_stream,
	.sensor_stop_stream = sr200pc20_sensor_stop_stream,
	.sensor_preview_mode = sr200pc20_sensor_preview_mode,
	.sensor_capture_mode = sr200pc20_sensor_capture_mode,
	.sensor_set_fps = NULL,
	.sensor_write_exp_gain = NULL,
	.sensor_write_snapshot_exp_gain = NULL,
	.sensor_csi_setting = msm_sensor_setting1,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = sr200pc20_sensor_power_up,
	.sensor_power_down = sr200pc20_sensor_power_down,
	.sensor_match_id = NULL,
	.sensor_adjust_frame_lines = NULL,
	.sensor_get_csi_params = msm_sensor_get_csi_params,
};

static struct msm_sensor_ctrl_t sr200pc20_s_ctrl = {
	.msm_sensor_reg = &sr200pc20_regs,
	.sensor_i2c_client = &sr200pc20_sensor_i2c_client,
	.sensor_i2c_addr = 0x40,
	.sensor_output_reg_addr = &sr200pc20_reg_addr,
	.sensor_id_info = &sr200pc20_id_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csic_params = &sr200pc20_csi_params_array[0],
	.msm_sensor_mutex = &sr200pc20_mut,
	.sensor_i2c_driver = &sr200pc20_i2c_driver,
	.sensor_v4l2_subdev_info = sr200pc20_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(sr200pc20_subdev_info),
	.sensor_v4l2_subdev_ops = &sr200pc20_subdev_ops,
	.func_tbl = &sr200pc20_func_tbl,
	.clk_rate = MSM_SENSOR_MCLK_24HZ,
};


module_init(sr200pc20_init);

