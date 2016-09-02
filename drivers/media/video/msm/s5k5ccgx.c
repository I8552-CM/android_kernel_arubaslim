/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

/*PCAM 1/5" s5k5ccgx*/
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/bitops.h>
#include <mach/camera.h>
#include <media/msm_camera.h>
#include <mach/pmic.h>

#include "s5k5ccgx.h"/*Amazing*/

#include <mach/vreg.h>
#include <linux/io.h>
#include "sec_cam_pmic.h"

#define SENSOR_DEBUG 0


#define CAM_MEGA_STBY	96		/*kk0704.park :: 16*/
#define CAM_MEGA_RST	85		/*kk0704.park :: 49*/
/*#define CAM_VGA_STBY	23*/
/*#define CAM_VGA_RST		123*/
#define CAM_GPIO_SCL	60
#define CAM_GPIO_SDA	61
/*#define PMIC_CAM_EN			0 *//* GPIO1*/
/*#define PMIC_CAM_MEGA_EN	5*/ /* GPIO6*/
/*#define PMIC_CAM_VGA_EN		7*/ /* GPIO8*/
#define CAM_POWER_ON	1		/*kk0704.park*/
#define CAM_POWER_OFF	0		/*kk0704.park*/

/*#define CONFIG_LOAD_FILE*/

#ifdef CONFIG_LOAD_FILE
#define S5K5CCGX_WRITE_LIST(A)\
	s5k5ccgx_sensor_write_list(A, (sizeof(A) / sizeof(A[0])), #A);
#define S5K5CCGX_WRITE_LIST_BURST(A)\
	s5k5ccgx_sensor_write_list(A, (sizeof(A) / sizeof(A[0])), #A);
#else
#define S5K5CCGX_WRITE_LIST(A)\
	s5k5ccgx_sensor_burst_write_list(A, (sizeof(A) / sizeof(A[0])), #A);
#define S5K5CCGX_WRITE_LIST_BURST(A)\
	s5k5ccgx_sensor_burst_write_list(A, (sizeof(A) / sizeof(A[0])), #A);
#endif

#define S5K5CCGX_MODE_PREVIEW 0
#define S5K5CCGX_MODE_CAPTURE 1

static char af_low_lux;
static char mEffect = EXT_CFG_EFFECT_NORMAL;
static char mBrightness = EXT_CFG_BR_STEP_0;
static char mContrast = EXT_CFG_CR_STEP_0;
static char mSaturation = EXT_CFG_SA_STEP_0;
static char mSharpness = EXT_CFG_SP_STEP_0;
static char mWhiteBalance = EXT_CFG_WB_AUTO;
static char mISO = EXT_CFG_ISO_AUTO;
static char mAutoExposure = EXT_CFG_METERING_NORMAL;
static char mScene = EXT_CFG_SCENE_OFF;
static char mLastScene = EXT_CFG_SCENE_OFF;
static char mAfMode = EXT_CFG_AF_SET_NORMAL;
static char mFPS = EXT_CFG_FRAME_AUTO;
static char mDTP;
static char mInit;
static char mMode = S5K5CCGX_MODE_PREVIEW;
static char mCurAfMode = EXT_CFG_AF_OFF;
#ifdef CONFIG_MACH_AMAZING_CDMA
static int previous_fps = -1;
#endif

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* FACTORY TEST */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
struct class *camera_class;
struct device *s5k5ccgx_dev;

static ssize_t camtype_file_cmd_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char camType[] = "LSI_S5K5CCGX_NONE";

	return snprintf(buf, 20, "%s", camType);
}

static ssize_t camtype_file_cmd_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	return size;
}

static DEVICE_ATTR(
rear_camtype, 0664, camtype_file_cmd_show, camtype_file_cmd_store);
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


struct s5k5ccgx_work {
	struct work_struct work;
};

static struct s5k5ccgx_work *s5k5ccgx_sensorw;
static struct i2c_client *s5k5ccgx_client;

struct s5k5ccgx_ctrl {
	const struct msm_camera_sensor_info *sensordata;
};

static unsigned int config_csi2;
static struct s5k5ccgx_ctrl *s5k5ccgx_ctrl;

static DECLARE_WAIT_QUEUE_HEAD(s5k5ccgx_wait_queue);
//kk0704.park :: DECLARE_MUTEX(s5k5ccgx_sem);
static int16_t s5k5ccgx_effect = CAMERA_EFFECT_OFF;


static int cam_hw_init(void);

static void s5k5ccgx_reset_power(void);
#ifdef CONFIG_MACH_AMAZING_CDMA
static void s5k5ccgx_set_power(int status);
#endif
static void s5k5ccgx_set_preview(void);

#ifdef CONFIG_LOAD_FILE
static int s5k5ccgx_regs_table_write(char *name);
#endif





static int s5k5ccgx_sensor_read(unsigned short subaddr, unsigned short *data)
{
	int ret;
	unsigned char buf[2];
	struct i2c_msg msg = { s5k5ccgx_client->addr, 0, 2, buf };

	buf[0] = (subaddr >> 8);
	buf[1] = (subaddr & 0xFF);

	ret = i2c_transfer(s5k5ccgx_client->adapter, &msg, 1) == 1 ? 0 : -EIO;
	if (ret == -EIO)
		goto error;

	msg.flags = I2C_M_RD;

	ret = i2c_transfer(s5k5ccgx_client->adapter, &msg, 1) == 1 ? 0 : -EIO;
	if (ret == -EIO)
		goto error;

	*data = ((buf[0] << 8) | buf[1]);

error:
	return ret;
}

static int s5k5ccgx_sensor_write(unsigned short subaddr, unsigned short val)
{
	unsigned char buf[4];
	struct i2c_msg msg = { s5k5ccgx_client->addr, 0, 4, buf };

	/*	pr_info("[PGH] on write func s5k5ccgx_client->addr : %x\n",
		s5k5ccgx_client->addr);
		pr_info("[PGH] on write func
		s5k5ccgx_client->adapter->nr : %d\n",
		s5k5ccgx_client->adapter->nr);*/


	buf[0] = (subaddr >> 8);
	buf[1] = (subaddr & 0xFF);
	buf[2] = (val >> 8);
	buf[3] = (val & 0xFF);

	return i2c_transfer(s5k5ccgx_client->adapter, &msg, 1) == 1 ? 0 : -EIO;
}


static int s5k5ccgx_sensor_write_list(const u32 *list, int size, char *name)
{
	int ret = 0;
	unsigned short subaddr = 0;
	unsigned short value = 0;
	int i = 0;

	pr_info("s5k5ccgx_sensor_write _list : %s\n", name);

#ifdef CONFIG_LOAD_FILE
	ret = s5k5ccgx_regs_table_write(name);
#else

	for (i = 0; i < size; i++) {

		subaddr = (list[i] >> 16); /*address*/
		value = (list[i] & 0xFFFF); /*value*/

		if (subaddr == 0xffff)	{
			msleep(value);
			pr_info("[s5k5ccgx] msleep %d\n", value);
		} else {
			if (s5k5ccgx_sensor_write(subaddr, value) < 0) {
				pr_info("[s5k5ccgx] sensor_write_list fail...-_-\n");
				ret = -1;
				return ret;
			}
		}
	}


#endif
	return ret;
}



#define BURST_MODE_BUFFER_MAX_SIZE 2700
unsigned char s5k5ccgx_buf_for_burstmode[BURST_MODE_BUFFER_MAX_SIZE];
/*static int s5k5ccgx_sensor_burst_write_list(
 struct samsung_short_t *list, int size, char *name)*/
static int s5k5ccgx_sensor_burst_write_list(const u32 *list,
		int size, char *name)
{
	int err = -EINVAL;
	int i = 0;
	int idx = 0;

	unsigned short subaddr = 0, next_subaddr = 0;
	unsigned short value = 0;

	struct i2c_msg msg = {s5k5ccgx_client->addr, 0, 0,
		s5k5ccgx_buf_for_burstmode};

	pr_info("s5k5ccgx_sensor_burst_write_list : %s\n", name);

	for (i = 0; i < size; i++) {
		if (idx > (BURST_MODE_BUFFER_MAX_SIZE-10)) {
			pr_info(" <= PCAM = > BURST MODE buffer overflow!!!\n");
			return err;
		}

		subaddr = (list[i] >> 16); /*address*/
		if (subaddr == 0x0F12)
			next_subaddr = (list[i+1] >> 16); /*address*/

		value = (list[i] & 0xFFFF); /*value*/


		switch (subaddr) {
		case 0x0F12:
			/*make and fill buffer for burst mode write*/
			if (idx == 0) {
				s5k5ccgx_buf_for_burstmode[idx++] = 0x0F;
				s5k5ccgx_buf_for_burstmode[idx++] = 0x12;
			}
			s5k5ccgx_buf_for_burstmode[idx++] = value >> 8;
			s5k5ccgx_buf_for_burstmode[idx++] = value & 0xFF;

			/*write in burstmode*/
			if (next_subaddr != 0x0F12)	{
				msg.len = idx;
				err = i2c_transfer(s5k5ccgx_client->adapter,
						&msg, 1) == 1 ? 0 : -EIO;
				/*pr_info("s5k4ecgx_sensor_burst_write, idx =
				 %d\n", idx);*/
				idx = 0;
			}
			break;

		case 0xFFFF:
			msleep(value);
			break;

		default:
			idx = 0;
			err = s5k5ccgx_sensor_write(subaddr, value);
			break;
		}
	}

	if (unlikely(err < 0)) {
		pr_info("%s: register set failed\n", __func__);
		return err;
	}
	return 0;

}


unsigned short s5k5ccgx_get_lux(void)
{
	unsigned short	msb, lsb, cur_lux;

	s5k5ccgx_sensor_write(0xFCFC, 0xD000);
	s5k5ccgx_sensor_write(0x002C, 0x7000);
	s5k5ccgx_sensor_write(0x002E, 0x2A3C);
	s5k5ccgx_sensor_read(0x0F12, &lsb);
	s5k5ccgx_sensor_read(0x0F12, &msb);

	cur_lux = (msb << 16) | lsb;
	pr_info("cur_lux : 0x%08x\n", cur_lux);

	return cur_lux;
}



void s5k5ccgx_effect_control(char value)
{

	/*int *i2c_clk_addr;*//* TEMP Dirty Code, Do not use it!*/
	/*i2c_clk_addr = 0xd500c004;*/

	switch (value) {
	case EXT_CFG_EFFECT_NORMAL:
		CAMDRV_DEBUG("EXT_CFG_EFFECT_NORMAL");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_effect_off);
		break;

	case EXT_CFG_EFFECT_NEGATIVE:
		CAMDRV_DEBUG("EXT_CFG_EFFECT_NEGATIVE");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_effect_negative);
		break;

	case EXT_CFG_EFFECT_MONO:
		CAMDRV_DEBUG("EXT_CFG_EFFECT_MONO");
		 S5K5CCGX_WRITE_LIST(s5k5ccgx_effect_mono);
		 break;

	case EXT_CFG_EFFECT_SEPIA:
		 CAMDRV_DEBUG("EXT_CFG_EFFECT_SEPIA");
		 S5K5CCGX_WRITE_LIST(s5k5ccgx_effect_sepia);
		 break;

	default:
		 pr_info(" <= PCAM = > Unexpected Effect mode : %d\n", value);
		 break;

	}

}


void s5k5ccgx_whitebalance_control(char value)
{
	switch (value) {
	case EXT_CFG_WB_AUTO:
		CAMDRV_DEBUG("EXT_CFG_WB_AUTO");
	 S5K5CCGX_WRITE_LIST(s5k5ccgx_wb_auto);
		break;

	case EXT_CFG_WB_DAYLIGHT:
		CAMDRV_DEBUG("EXT_CFG_WB_DAYLIGHT");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_wb_daylight);
		break;

	case EXT_CFG_WB_CLOUDY:
		CAMDRV_DEBUG("EXT_CFG_WB_CLOUDY");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_wb_cloudy);
		break;

	case EXT_CFG_WB_FLUORESCENT:
		CAMDRV_DEBUG("EXT_CFG_WB_FLUORESCENT");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_wb_fluorescent);
		break;

	case EXT_CFG_WB_INCANDESCENT:
		CAMDRV_DEBUG("EXT_CFG_WB_INCANDESCENT");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_wb_incandescent);
		break;

	default:
		pr_info(" <= PCAM = > Unexpected WB mode : %d\n", value);
		break;

	} /* end of switch*/

}


void s5k5ccgx_brightness_control(char value)
{
	switch (value) {
	case EXT_CFG_BR_STEP_P_4:
		CAMDRV_DEBUG("EXT_CFG_BR_STEP_P_4");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_brightness_p_4);
		break;

	case EXT_CFG_BR_STEP_P_3:
		CAMDRV_DEBUG("EXT_CFG_BR_STEP_P_3");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_brightness_p_3);
		break;

	case EXT_CFG_BR_STEP_P_2:
		CAMDRV_DEBUG("EXT_CFG_BR_STEP_P_2");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_brightness_p_2);
		break;

	case EXT_CFG_BR_STEP_P_1:
		CAMDRV_DEBUG("EXT_CFG_BR_STEP_P_1");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_brightness_p_1);
		break;

	case EXT_CFG_BR_STEP_0:
		CAMDRV_DEBUG("EXT_CFG_BR_STEP_0");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_brightness_0);
		break;

	case EXT_CFG_BR_STEP_M_1:
		CAMDRV_DEBUG("EXT_CFG_BR_STEP_M_1");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_brightness_m_1);
		break;

	case EXT_CFG_BR_STEP_M_2:
		CAMDRV_DEBUG("EXT_CFG_BR_STEP_M_2");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_brightness_m_2);
		break;

	case EXT_CFG_BR_STEP_M_3:
		CAMDRV_DEBUG("EXT_CFG_BR_STEP_M_3");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_brightness_m_3);
		break;

	case EXT_CFG_BR_STEP_M_4:
		CAMDRV_DEBUG("EXT_CFG_BR_STEP_M_4");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_brightness_m_4);
		break;

	default:
		pr_info(" <= PCAM = > Unexpected BR mode : %d\n", value);
		break;

	}

}

int s5k5ccgx_fps_control(unsigned int fps)
{
	int rc = 0;

	pr_info("s5k5ccgx_fps_control : fps = %d\n", fps);

	if (mScene != EXT_CFG_SCENE_OFF) {
		pr_info("s5k5ccgx_fps_control : scene mode is set, skip fps setting!!\n");
		return rc;
	}
#ifdef CONFIG_MACH_AMAZING_CDMA
	if (fps == previous_fps) {
		pr_info(KERN_DEBUG "fps is same with previous_fps ");
		return rc;
	}
#endif
	switch (fps) {
	case EXT_CFG_FRAME_AUTO:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_fps_auto);
		break;

	case EXT_CFG_FRAME_FIX_15:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_fps_15fix);
		break;

	case EXT_CFG_FRAME_FIX_24:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_fps_24fix);
		break;

	case EXT_CFG_FRAME_FIX_30:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_fps_30fix);
		break;

	default:
		pr_info("Invalid fps set %d.\n", fps);
		break;
	}
	return rc;
}


void s5k5ccgx_iso_control(char value)
{
 /* vasto doesn't use iso function.*/
/*	switch (value) {
	case EXT_CFG_ISO_AUTO:
		CAMDRV_DEBUG("EXT_CFG_ISO_AUTO");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_iso_auto);
		break;

	case EXT_CFG_ISO_50:
		CAMDRV_DEBUG("EXT_CFG_ISO_50");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_iso_50);
		break;

	case EXT_CFG_ISO_100:
		CAMDRV_DEBUG("EXT_CFG_ISO_100");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_iso_100);
		break;

	case EXT_CFG_ISO_200:
		CAMDRV_DEBUG("EXT_CFG_ISO_200");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_iso_200);
		break;

	case EXT_CFG_ISO_400:
		CAMDRV_DEBUG("EXT_CFG_ISO_400");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_iso_400);
		break;

	default:
		pr_info(" <= PCAM = > Unexpected ISO mode : %d\n", value);
		break;

	}
*/
}


void s5k5ccgx_metering_control(char value)
{
	switch (value) {
	case EXT_CFG_METERING_NORMAL:
		CAMDRV_DEBUG("EXT_CFG_METERING_NORMAL");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_metering_normal);
		break;

	case EXT_CFG_METERING_SPOT:
		CAMDRV_DEBUG("EXT_CFG_METERING_SPOT");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_metering_spot);
		break;

	case EXT_CFG_METERING_CENTER:
	 CAMDRV_DEBUG("EXT_CFG_METERING_CENTER");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_metering_center);
		break;

	default:
		pr_info(" <= PCAM = > Unexpected METERING mode : %d\n", value);
		break;
	}

}

void s5k5ccgx_scene_control(char value)
{
#ifdef CONFIG_MACH_AMAZING_CDMA
	if (value != EXT_CFG_SCENE_OFF) {
		CAMDRV_DEBUG("SCENE MODE START, RESET SETTINGS");
		s5k5ccgx_brightness_control(EXT_CFG_BR_STEP_0);
		s5k5ccgx_whitebalance_control(EXT_CFG_WB_AUTO);
		s5k5ccgx_effect_control(EXT_CFG_EFFECT_NORMAL);
		s5k5ccgx_metering_control(EXT_CFG_METERING_NORMAL);
	}
#endif
	switch (value) {
	case EXT_CFG_SCENE_OFF:
		CAMDRV_DEBUG("EXT_CFG_SCENE_OFF");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_off);
		break;

	case EXT_CFG_SCENE_PORTRAIT:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_off);
		CAMDRV_DEBUG("EXT_CFG_SCENE_PORTRAIT");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_portrait);
		break;

	case EXT_CFG_SCENE_LANDSCAPE:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_off);
		CAMDRV_DEBUG("EXT_CFG_SCENE_LANDSCAPE");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_landscape);
		break;

	case EXT_CFG_SCENE_SPORTS:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_off);
		CAMDRV_DEBUG("EXT_CFG_SCENE_SPORTS");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_sports);
		break;

	case EXT_CFG_SCENE_PARTY:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_off);
		CAMDRV_DEBUG("EXT_CFG_SCENE_PARTY");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_party);
		break;

	case EXT_CFG_SCENE_BEACH:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_off);
		CAMDRV_DEBUG("EXT_CFG_SCENE_BEACH");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_beach);
		break;

	case EXT_CFG_SCENE_SUNSET:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_off);
		CAMDRV_DEBUG("EXT_CFG_SCENE_SUNSET");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_sunset);
		break;

	case EXT_CFG_SCENE_DAWN:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_off);
		CAMDRV_DEBUG("EXT_CFG_SCENE_DAWN");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_dawn);
		break;

	case EXT_CFG_SCENE_FALL:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_off);
		CAMDRV_DEBUG("EXT_CFG_SCENE_FALL");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_fall);
		break;
	case EXT_CFG_SCENE_NIGHTSHOT:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_off);
		CAMDRV_DEBUG("EXT_CFG_SCENE_NIGHTSHOT");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_nightshot);
		break;

	case EXT_CFG_SCENE_BACKLIGHT:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_off);
		CAMDRV_DEBUG("EXT_CFG_SCENE_BACKLIGHT");
	 S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_backlight);
		break;

	case EXT_CFG_SCENE_FIREWORK:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_off);
		CAMDRV_DEBUG("EXT_CFG_SCENE_FIREWORK");
	 S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_firework);
		break;

	case EXT_CFG_SCENE_TEXT:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_off);
		CAMDRV_DEBUG("EXT_CFG_SCENE_TEXT");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_text);
		break;

	case EXT_CFG_SCENE_CANDLE:
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_off);
		CAMDRV_DEBUG("EXT_CFG_SCENE_CANDLE");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_scene_candle);
		break;

	default:
		pr_info("<= PCAM = > Unexpected SCENE mode : %d\n", value);
		break;
	}

/* KDH comment because of CTS
	if (mInit == 0 || mLastScene == EXT_CFG_SCENE_TEXT) {
		s5k5ccgx_af_control(mAfMode);
	} else if (value == EXT_CFG_SCENE_TEXT) {
		s5k5ccgx_af_control(EXT_CFG_AF_SET_MACRO);
	}
*/
	mLastScene = value;
}


void s5k5ccgx_contrast_control(char value)
{
	switch (value) {
	case EXT_CFG_CR_STEP_M_2:
		CAMDRV_DEBUG("EXT_CFG_CR_STEP_M_2");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_contrast_m_2);
		break;

	case EXT_CFG_CR_STEP_M_1:
		CAMDRV_DEBUG("EXT_CFG_CR_STEP_M_1");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_contrast_m_1);
		break;

	case EXT_CFG_CR_STEP_0:
		CAMDRV_DEBUG("EXT_CFG_CR_STEP_0");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_contrast_0);
		break;

	case EXT_CFG_CR_STEP_P_1:
		CAMDRV_DEBUG("EXT_CFG_CR_STEP_P_1");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_contrast_p_1);
		break;

	case EXT_CFG_CR_STEP_P_2:
		CAMDRV_DEBUG("EXT_CFG_CR_STEP_P_2");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_contrast_p_2);
		break;

	default:
		pr_info(" <= PCAM = > Unexpected"
				"EXT_CFG_CR_CONTROL mode :%d\n", value);
		break;
	}
}

void s5k5ccgx_saturation_control(char value)
{
	switch (value) {
	case EXT_CFG_SA_STEP_M_2:
		CAMDRV_DEBUG("EXT_CFG_SA_STEP_M_2");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_saturation_m_2);
		break;

	case EXT_CFG_SA_STEP_M_1:
		CAMDRV_DEBUG("EXT_CFG_SA_STEP_M_1");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_saturation_m_1);
		break;

	case EXT_CFG_SA_STEP_0:
		CAMDRV_DEBUG("EXT_CFG_SA_STEP_0");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_saturation_0);
		break;

	case EXT_CFG_SA_STEP_P_1:
		CAMDRV_DEBUG("EXT_CFG_SA_STEP_P_1");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_saturation_p_1);
		break;

	case EXT_CFG_SA_STEP_P_2:
		CAMDRV_DEBUG("EXT_CFG_SA_STEP_P_2");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_saturation_p_2);
		break;

	default:
		pr_info(" <= PCAM = > Unexpected EXT_CFG_SA_CONTROL mode :"
				"%d\n", value);
		break;
	}

}


void s5k5ccgx_sharpness_control(char value)
{
	switch (value) {
	case EXT_CFG_SP_STEP_M_2:
		CAMDRV_DEBUG("EXT_CFG_SP_STEP_M_2");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_sharpness_m_2);
		break;

	case EXT_CFG_SP_STEP_M_1:
		CAMDRV_DEBUG("EXT_CFG_SP_STEP_M_1");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_sharpness_m_1);
		break;

	case EXT_CFG_SP_STEP_0:
		CAMDRV_DEBUG("EXT_CFG_SP_STEP_0");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_sharpness_0);
		break;

	case EXT_CFG_SP_STEP_P_1:
		CAMDRV_DEBUG("EXT_CFG_SP_STEP_P_1");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_sharpness_p_1);
		break;

	case EXT_CFG_SP_STEP_P_2:
		CAMDRV_DEBUG("EXT_CFG_SP_STEP_P_2");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_sharpness_p_2);
		break;

	default:
		pr_info(" <= PCAM = > Unexpected EXT_CFG_SP_CONTROL mode :"
				"%d\n", value);
		break;
	}
}

int s5k5ccgx_af_control(char value)
{
		unsigned short af_status;
		unsigned short cur_lux;
	switch (value) {
	case EXT_CFG_AF_CHECK_STATUS:
		CAMDRV_DEBUG("EXT_CFG_AF_CHECK_STATUS");
		s5k5ccgx_sensor_write(0x002C, 0x7000);
		s5k5ccgx_sensor_write(0x002E, 0x2D12);
		s5k5ccgx_sensor_read(0x0F12, &af_status);

		switch (af_status) {
		case 0:
		case 3:
		case 4:
		case 6:
		case 8:
			return EXT_CFG_AF_LOWCONF;
		case 1:
			return EXT_CFG_AF_PROGRESS;
		case 2:
			return EXT_CFG_AF_SUCCESS;
		case 9:
		 return EXT_CFG_AF_CANCELED;
		case 10:
		 return EXT_CFG_AF_TIMEOUT;
		default:
		 return af_status;
		}
		break;

	case EXT_CFG_AF_OFF:
	 CAMDRV_DEBUG("EXT_CFG_AF_OFF");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_af_off);
		mCurAfMode = EXT_CFG_AF_OFF;
		break;

	case EXT_CFG_AF_SET_NORMAL:
		if (mCurAfMode != EXT_CFG_AF_SET_NORMAL) {
			CAMDRV_DEBUG("EXT_CFG_AF_SET_NORMAL");
			S5K5CCGX_WRITE_LIST(s5k5ccgx_af_normal_on);
			mCurAfMode = EXT_CFG_AF_SET_NORMAL;
		} else {
			pr_info("already EXT_CFG_AF_SET_NORMAL is set! skip!!");
		}
		break;

	case EXT_CFG_AF_SET_MACRO:
		if (mCurAfMode != EXT_CFG_AF_SET_MACRO) {
			CAMDRV_DEBUG("EXT_CFG_AF_SET_MACRO");
			S5K5CCGX_WRITE_LIST(s5k5ccgx_af_macro_on);
			mCurAfMode = EXT_CFG_AF_SET_MACRO;
		} else {
			pr_info("already EXT_CFG_AF_SET_MACRO is set! skip!!");
		}
		break;

	case EXT_CFG_AF_DO:
		cur_lux = s5k5ccgx_get_lux();
		CAMDRV_DEBUG("EXT_CFG_AF_DO");
		if (cur_lux > 0xFFFE)
			;
		else if (cur_lux < 0x0020) {
			CAMDRV_DEBUG("low lux AF ~~");
			af_low_lux = 1;
		} else
			;
		S5K5CCGX_WRITE_LIST(s5k5ccgx_af_do);
		break;

	case EXT_CFG_AF_ABORT:
		CAMDRV_DEBUG("EXT_CFG_AF_ABORT");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_af_abort);
		break;

	case EXT_CFG_AF_CHECK_2nd_STATUS:
		/*pr_info("EXT_CFG_AF_2ND_CHECK_STATUS");*/
		s5k5ccgx_sensor_write(0x002C, 0x7000);
		s5k5ccgx_sensor_write(0x002E, 0x1F2F);
		s5k5ccgx_sensor_read(0x0F12, &af_status);
		CAMDRV_DEBUG(" <= PCAM = > AF 2nd check status :"
				"%x", af_status);
		return af_status;
		break;

	default:
		pr_info(" <= PCAM = > unexpected AF command : %d\n", value);
		break;

	}
	return 0;
}



void s5k5ccgx_DTP_control(char value)
{
	switch (value) {
	case EXT_CFG_DTP_OFF:
		CAMDRV_DEBUG("DTP OFF");
		/*S5K5CCGX_WRITE_LIST(s5k5ccgx_dtp_off);*/
		s5k5ccgx_reset_power();
		S5K5CCGX_WRITE_LIST_BURST(s5k5ccgx_pre_init0);
		mInit = 0;
		s5k5ccgx_set_preview();
		break;

	case EXT_CFG_DTP_ON:
		CAMDRV_DEBUG("DTP ON");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_DTP_init0);
		break;

	default:
		pr_info(" <= PCAM = > unexpected DTP control on PCAM\n");
		break;
	}
}


void s5k5ccgx_CPU_control(char value)
{
	switch (value) {
	case EXT_CFG_CPU_ONDEMAND:
		CAMDRV_DEBUG("EXT_CFG_CPU_ONDEMAND");
		/*cpufreq_direct_set_policy(0, "ondemand");*/
		break;

	case EXT_CFG_CPU_PERFORMANCE:
		CAMDRV_DEBUG("EXT_CFG_CPU_PERFORMANCE");
		/*cpufreq_direct_set_policy(0, "performance");*/
		break;
	}
}



static int s5k5ccgx_sensor_ext_config(void __user *arg)
{
	long rc = 0;
	unsigned short lsb, msb, rough_iso;
	unsigned short id = 0; /*CAM FOR FW*/
	ioctl_pcam_info_8bit		ctrl_info;

	if (copy_from_user((void *)&ctrl_info, (const void *)arg,
				sizeof(ctrl_info)))
		pr_info(" <= PCAM = > %s fail copy_from_user!\n", __func__);


	pr_info("[s5k5ccgx] sensor_ext_config %d %d %d %d %d\n",
			ctrl_info.mode, ctrl_info.address,
			ctrl_info.value_1, ctrl_info.value_2,
			ctrl_info.value_3);

	if (mDTP) {
		if (ctrl_info.mode != EXT_CFG_DTP_CONTROL) {
			pr_info("[s5k5ccgx] skip sensor_ext_config : DTP on");
			return 0;
		}
	}

	switch (ctrl_info.mode) {
	case EXT_CFG_GET_INFO:
		s5k5ccgx_sensor_write(0xFCFC, 0xD000);	/*shutter speed*/
		s5k5ccgx_sensor_write(0x002C, 0x7000);
		s5k5ccgx_sensor_write(0x002E, 0x2A14);
		s5k5ccgx_sensor_read(0x0F12, &lsb);
		s5k5ccgx_sensor_read(0x0F12, &msb);
		s5k5ccgx_sensor_write(0xFCFC, 0xD000);	/*iso speed rate*/
		s5k5ccgx_sensor_write(0x002C, 0x7000);
		s5k5ccgx_sensor_write(0x002E, 0x2A18);
		s5k5ccgx_sensor_read(0x0F12, &rough_iso);

		ctrl_info.value_1 = lsb;
		ctrl_info.value_2 = msb;
		pr_info(" <= PCAM = > exposure %x %x\n", lsb, msb);
		ctrl_info.value_3 = rough_iso;
		pr_info(" <= PCAM = > rough_iso %x\n", rough_iso);
		break;

	case EXT_CFG_FRAME_CONTROL:
		mFPS = ctrl_info.value_1;
		if (mInit)
			s5k5ccgx_fps_control(mFPS);
		break;

	case EXT_CFG_AF_CONTROL:
		if (ctrl_info.value_1 == EXT_CFG_AF_SET_MACRO ||
				ctrl_info.value_1 == EXT_CFG_AF_SET_NORMAL ||
				ctrl_info.value_1 == EXT_CFG_AF_OFF) {
			mAfMode = ctrl_info.value_1;
			if (mInit)
				s5k5ccgx_af_control(mAfMode);
		} else
			ctrl_info.value_3 = s5k5ccgx_af_control(
					ctrl_info.value_1);

		break;

	case EXT_CFG_EFFECT_CONTROL:
		mEffect = ctrl_info.value_1;
		if (mInit)
			s5k5ccgx_effect_control(mEffect);

		break;

	case EXT_CFG_WB_CONTROL:
		mWhiteBalance = ctrl_info.value_1;
		if (mInit)
			s5k5ccgx_whitebalance_control(mWhiteBalance);

		break;

	case EXT_CFG_BR_CONTROL:
		mBrightness = ctrl_info.value_1;
		if (mInit)
			s5k5ccgx_brightness_control(mBrightness);

		break;

	case EXT_CFG_ISO_CONTROL:
		mISO = ctrl_info.value_1;
		if (mInit)
			s5k5ccgx_iso_control(mISO);

		break;

	case EXT_CFG_METERING_CONTROL:
		mAutoExposure = ctrl_info.value_1;
		if (mInit)
			s5k5ccgx_metering_control(mAutoExposure);

		break;

	case EXT_CFG_SCENE_CONTROL:
		mScene = ctrl_info.value_1;
		if (mInit)
			s5k5ccgx_scene_control(mScene);

		break;

	case EXT_CFG_AE_AWB_CONTROL:
		switch (ctrl_info.value_1) {
		case EXT_CFG_AE_LOCK:
			CAMDRV_DEBUG("EXT_CFG_AE_LOCK");
			S5K5CCGX_WRITE_LIST(s5k5ccgx_ae_lock);
			break;

		case EXT_CFG_AE_UNLOCK:
			CAMDRV_DEBUG("EXT_CFG_AE_UNLOCK");
			S5K5CCGX_WRITE_LIST(s5k5ccgx_ae_unlock);
			break;

		case EXT_CFG_AWB_LOCK:
			CAMDRV_DEBUG("EXT_CFG_AWB_LOCK");
			S5K5CCGX_WRITE_LIST(s5k5ccgx_awb_lock);
			break;

		case EXT_CFG_AWB_UNLOCK:
		 CAMDRV_DEBUG("EXT_CFG_AWB_UNLOCK");
			S5K5CCGX_WRITE_LIST(s5k5ccgx_awb_unlock);
			break;

		case EXT_CFG_AE_AWB_LOCK:
			CAMDRV_DEBUG("EXT_CFG_AWB_AE_LOCK");
			if (mWhiteBalance == 0) {
				S5K5CCGX_WRITE_LIST(s5k5ccgx_ae_lock);
				S5K5CCGX_WRITE_LIST(s5k5ccgx_awb_lock);
			} else {
				S5K5CCGX_WRITE_LIST(s5k5ccgx_ae_lock);
			}
			break;

		case EXT_CFG_AE_AWB_UNLOCK:
			CAMDRV_DEBUG("EXT_CFG_AWB_AE_UNLOCK");
			if (mWhiteBalance == 0) {
				S5K5CCGX_WRITE_LIST(s5k5ccgx_ae_unlock);
				S5K5CCGX_WRITE_LIST(s5k5ccgx_awb_unlock);
			} else {
				S5K5CCGX_WRITE_LIST(s5k5ccgx_ae_unlock);
			}
			break;

		default:
			pr_info(" <= PCAM = > Unexpected AWB_AE mode : %d\n", \
					ctrl_info.value_1);
			break;
		}
		break;

	case EXT_CFG_CR_CONTROL:
		mContrast = ctrl_info.value_1;
		if (mInit)
			s5k5ccgx_contrast_control(mContrast);

		break;

	case EXT_CFG_SA_CONTROL:
		mSaturation = ctrl_info.value_1;
		if (mInit)
			s5k5ccgx_saturation_control(mSaturation);

		break;

	case EXT_CFG_SP_CONTROL:
		mSharpness = ctrl_info.value_1;
		if (mInit)
			s5k5ccgx_sharpness_control(mSharpness);

		break;

	case EXT_CFG_DTP_CONTROL:
		mDTP = ctrl_info.value_1;
		if (mInit)
			s5k5ccgx_DTP_control(mDTP);

		break;

	case EXT_CFG_GET_MODULE_STATUS:
		/*ctrl_info.value_3 = gpio_get_value(0);*/

		s5k5ccgx_sensor_write(0x002C, 0x0000);
		s5k5ccgx_sensor_write(0x002E, 0x0040);
		s5k5ccgx_sensor_read(0x0F12, &id);

		ctrl_info.value_3 = id;

		pr_info(" <= PCAM = > check current module status : %x\n",\
				ctrl_info.value_3);
		pr_info(" <= PCAM = > PINON/OFF : %d\n", gpio_get_value(0));
		break;

	case EXT_CFG_CPU_CONTROL:
		s5k5ccgx_CPU_control(ctrl_info.value_1);
		break;

	default:
		pr_info(" <= PCAM = > Unexpected mode on sensor_rough_control :"
				"%d\n", ctrl_info.mode);
		break;
	}

	if (copy_to_user((void *)arg, (const void *)&ctrl_info,
				sizeof(ctrl_info)))
		pr_info(" <= PCAM = > %s fail on copy_to_user!\n", __func__);
	return rc;
}

/*
int s5k5ccgx_mipi_mode(int mode)
{
	int rc = 0;
	struct msm_camera_csi_params s5k5ccgx_csi_params;

	CAMDRV_DEBUG("%s E", __func__);

	if (!config_csi2) {
		s5k5ccgx_csi_params.lane_cnt = 1;
		s5k5ccgx_csi_params.data_format = CSI_8BIT;
		s5k5ccgx_csi_params.lane_assign = 0xe4;
		s5k5ccgx_csi_params.dpcm_scheme = 0;
		s5k5ccgx_csi_params.settle_cnt = 0x14;
		rc = msm_camio_csi_config(&s5k5ccgx_csi_params);
		if (rc < 0)
			pr_info(KERN_ERR "config csi controller failed\n");
		config_csi2 = 1;

		//msleep(5); // = > Please add some delay
		usleep(5*1000);
	}
	CAMDRV_DEBUG("%s X", __func__);
	return rc;
}
*/	//kk0704.park :: ARUBA_TEMP
	
int wait_sensor_mode(int mode, int interval, int cnt)
{
	int rc = 0;
	unsigned short sensor_mode;

	pr_info("s5k5ccgx wait_sensor_mode E\n");
	do {
		s5k5ccgx_sensor_write(0x002C, 0x7000);
		s5k5ccgx_sensor_write(0x002E, 0x1E86);
		s5k5ccgx_sensor_read(0x0F12, &sensor_mode);
		pr_info("current sensor mode = %d (wait = %d)\n",
				sensor_mode, cnt);
		msleep(interval);
	} while ((--cnt) > 0 && !(sensor_mode == mode));

	if (cnt == 0) {
		pr_info("!! MODE CHANGE ERROR to %d !!\n", mode);
		rc = -1;
	}
	pr_info("s5k5ccgx wait_sensor_mode X\n");

	return rc;
}

static void s5k5ccgx_set_preview(void)
{
//kk0704.park ARUBA_TEMP	int rc = 0;
//kk0704.park ARUBA_TEMP	int is_initial_preview = 0;
	/*unsigned short read_value;*/
//kk0704.park ARUBA_TEMP 	unsigned short	id = 0;

	CAMDRV_DEBUG("%s E", __func__);
	CAMDRV_DEBUG("I2C address : 0x%x", s5k5ccgx_client->addr);

//kk0704.park :: ARUBA_TEMP	s5k5ccgx_mipi_mode(1);

	if (mDTP) {
		if (mInit == 0) {
			S5K5CCGX_WRITE_LIST_BURST(s5k5ccgx_DTP_init0);
			wait_sensor_mode(S5K5CCGX_MODE_PREVIEW, 10, 10);
		}
	} else {
		if (mInit == 0) {
			S5K5CCGX_WRITE_LIST_BURST(s5k5ccgx_init0);
			S5K5CCGX_WRITE_LIST(s5k5ccgx_update_preview_setting);
			wait_sensor_mode(S5K5CCGX_MODE_PREVIEW, 10, 10);
			s5k5ccgx_scene_control(mScene);
			if (mScene == EXT_CFG_SCENE_OFF) {
				s5k5ccgx_effect_control(mEffect);
				s5k5ccgx_whitebalance_control(mWhiteBalance);
				/*s5k5ccgx_iso_control(mISO);*/
				s5k5ccgx_metering_control(mAutoExposure);
				/*s5k5ccgx_contrast_control(mContrast);*/
				/*s5k5ccgx_saturation_control(mSaturation);*/
				/*s5k5ccgx_sharpness_control(mSharpness);*/
				s5k5ccgx_brightness_control(mBrightness);
				s5k5ccgx_fps_control(mFPS);
			}
		} else {
			S5K5CCGX_WRITE_LIST(s5k5ccgx_preview);
			wait_sensor_mode(S5K5CCGX_MODE_PREVIEW, 10, 10);
		}
	}

	mInit = 1;
	mMode = S5K5CCGX_MODE_PREVIEW;
}

void s5k5ccgx_set_capture(void)
{
	unsigned short cur_lux = s5k5ccgx_get_lux();

	CAMDRV_DEBUG("s5k5ccgx_set_capture");

	if (cur_lux > 0xFFFE) {
		CAMDRV_DEBUG("HighLight Snapshot!\n");
		pr_info("HighLight Snapshot!\n");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_highlight_snapshot);
	} else if (cur_lux < 0x0020) {
		if ((mScene == EXT_CFG_SCENE_NIGHTSHOT) ||
				(mScene == EXT_CFG_SCENE_FIREWORK)) {
			CAMDRV_DEBUG("Night or Firework Snapshot!\n");
			pr_info("Night or Firework Snapshot!\n");
			S5K5CCGX_WRITE_LIST(s5k5ccgx_night_snapshot);
		} else {
			CAMDRV_DEBUG("LowLight Snapshot delay ~~~~!\n");
			pr_info("LowLight Snapshot!\n");
			S5K5CCGX_WRITE_LIST(s5k5ccgx_lowlight_snapshot);
		}
	} else {
		CAMDRV_DEBUG("Normal Snapshot!\n");
		pr_info("Normal Snapshot!\n");
		S5K5CCGX_WRITE_LIST(s5k5ccgx_snapshot);
	}
	wait_sensor_mode(S5K5CCGX_MODE_CAPTURE, 10, 40);
	mMode = S5K5CCGX_MODE_CAPTURE;
}


static long s5k5ccgx_set_sensor_mode(int mode)
{
	CAMDRV_DEBUG("s5k5ccgx_set_sensor_mode : %d", mode);
	switch (mode) {
	case SENSOR_PREVIEW_MODE:
		s5k5ccgx_set_preview();
		break;
	case SENSOR_SNAPSHOT_MODE:
		s5k5ccgx_set_capture();
		break;
	case SENSOR_RAW_SNAPSHOT_MODE:
		CAMDRV_DEBUG("RAW_SNAPSHOT NOT SUPPORT!!");
		break;
	default:
		return -EINVAL;
	}
	return 0;
}


static void s5k5ccgx_reset_power(void)
{
#ifndef CONFIG_MACH_AMAZING_CDMA
	struct vreg *vreg_cam_vcamc;
	int rc;

	vreg_cam_vcamc = vreg_get(NULL, "vcamc");

	if (IS_ERR(vreg_cam_vcamc)) {
		pr_info(KERN_ERR "%s: vreg get failed (%ld)\n",\
				__func__, PTR_ERR(vreg_cam_vcamc));
		return;
	}

	/*power off*/
	gpio_set_value(CAM_MEGA_RST, 0);
	udelay(50); /* >50us*/

	msm_camio_clk_disable(CAMIO_CAM_MCLK_CLK);

	gpio_set_value(CAM_MEGA_STBY, 0);
	/*gpio_set_value(CAM_VGA_RST, 0);*/
	/*gpio_set_value(CAM_VGA_STBY, 0);*/

	/*pmic_gpio_set_value(PMIC_CAM_EN, 0);*/
	/*pmic_gpio_set_value(PMIC_CAM_VGA_EN, 0); // sub*/
	/*pmic_gpio_set_value(PMIC_CAM_MEGA_EN, 0); // Main A 2.8V*/
	vreg_disable(vreg_cam_vcamc);

	/*add delay*/
	/*msleep(10);*/
	usleep(10*1000);

	/*power on*/
	vreg_set_level(vreg_cam_vcamc, 1200);
	/*end of gpio & pmic config*/

	/*start power sequence*/
	/*gpio_set_value(CAM_VGA_RST, 1);*/
/*	msleep(1);*/ /* >1ms*/
	usleep(1*1000);

	vreg_enable(vreg_cam_vcamc); /* VCAMC_1.2V*/
	udelay(1); /* >0us*/
	/*
	 pmic_gpio_set_value(PMIC_CAM_EN, 1);
	 udelay(20); // >20us

	 pmic_gpio_set_value(PMIC_CAM_VGA_EN, 1); // VGA_CORE_1.8V
	 udelay(15); // >15us

	 pmic_gpio_set_value(PMIC_CAM_MEGA_EN, 1); // VCAMA_2.8V
	 udelay(20); // >20us

	 gpio_set_value(CAM_VGA_RST, 0);
	 msleep(10); // >10ms
	 */

	msm_camio_clk_enable(CAMIO_CAM_MCLK_CLK);
	msm_camio_clk_rate_set(24000000);
	udelay(10); /* >10us (temp)*/

	/*
	 gpio_set_value(CAM_VGA_RST, 1);
	 msleep(5); // >10ms

	 gpio_set_value(CAM_VGA_STBY, 1); // Sub
	 udelay(10); // >10us
	 */
	gpio_set_value(CAM_MEGA_STBY, 1);
	udelay(15); /* >15us*/

	gpio_set_value(CAM_MEGA_RST, 1);
	msleep(50); /* >50ms*/
#elif CONFIG_MACH_AMAZING_CDMA
	s5k5ccgx_set_power(CAM_POWER_OFF);
	msleep(100);
	s5k5ccgx_set_power(CAM_POWER_ON);
	msleep(100);
#endif
}

void s5k5ccgx_set_power(int status)
{
	gpio_tlmm_config(GPIO_CFG(CAM_MEGA_STBY, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(CAM_MEGA_RST, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);

	if (status == CAM_POWER_ON) {
		CAMDRV_DEBUG("s5k5ccgx_set_power : POWER ON");
		gpio_set_value(CAM_MEGA_STBY, 0);
		gpio_set_value(CAM_MEGA_RST, 0);

		cam_ldo_power_on();

		msm_camio_clk_enable(CAMIO_CAM_MCLK_CLK);
		msm_camio_clk_rate_set(24000000);
		udelay(20); /* >10us (temp)*/

		gpio_set_value(CAM_MEGA_STBY, 1);
	/*	msleep(10);*/
		usleep(10*1000);
		gpio_set_value(CAM_MEGA_RST, 1);
	} else {
		CAMDRV_DEBUG("s5k5ccgx_set_power : POWER OFF");

		gpio_set_value(CAM_MEGA_RST, 0);
		udelay(70);

		msm_camio_clk_disable(CAMIO_CAM_MCLK_CLK);
		udelay(20);

		gpio_set_value(CAM_MEGA_STBY, 0);
		/*msleep(1);*/
		usleep(1*1000);

		cam_ldo_power_off();
	}
}

static int s5k5ccgx_check_sensor_id(void)
{
	int rc = 0;
	int i = 0;
	int id = 0;

	pr_info("[S5K5CCGX] s5k5ccgx_check_sensor_id()\n");

	/* read firmware id */
	s5k5ccgx_sensor_write(0x002C, 0x0000);
	s5k5ccgx_sensor_write(0x002E, 0x0040);
	s5k5ccgx_sensor_read(0x0F12, &id);

	if (id != 0x05CC) {
		pr_info("[S5K5CCGX] WRONG SENSOR FW = > id 0x%x\n", id);
		/*rc = -1;*/
	} else {
		pr_info("[S5K5CCGX] CURRENT SENSOR FW = > id 0x%x\n", id);
	}

	return rc;
}

#ifdef CONFIG_LOAD_FILE

#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>


static char *s5k5ccgx_regs_table;
static int s5k5ccgx_regs_table_size;
static char tunning_data[300000];

void s5k5ccgx_regs_table_init(void)
{
	struct file *filp;
	char *dp;
	long l = 2000;
	long file_size;
	loff_t pos = 0;
	loff_t pos_copy = 0;
	int ret;
	mm_segment_t fs = get_fs();

	pr_info("%s %d\n", __func__, __LINE__);

	set_fs(get_ds());

	/*Open the file s5k5ccgx.h*/
#ifdef CONFIG_MACH_TORINO_CTC
	filp = filp_open("/mnt/sdcard/external_sd/s5k5ccgx_torino.h",
			O_RDONLY, 0);/*Torino*/
#else
	filp = filp_open("/mnt/sdcard/external_sd/s5k5ccgx.h",
			O_RDONLY, 0);/*Amazing*/
#endif
	file_size = filp->f_path.dentry->d_inode->i_size;
	pr_info("[S5K5CCGX] file_size = %d, l = %ld", file_size, l);

	if (IS_ERR(filp)) {/*Return if problem while openning the file*/
		pr_info("[S5K5CCGX] file open error\n");
		return;
	}

	pr_info("[S5K5CCGX] Read file start");
	dp = kmalloc(l, GFP_KERNEL);
	if (dp == NULL) {
		pr_info("[S5K5CCGX] Out of Memory");
		filp_close(filp, current->files);
	}

	while (1) {/*Read the file s5k5ccgx.h*/
		pos_copy = pos;
		pr_info("[S5K5CCGX] pos = %d, pos_copy = %d",\
				pos, pos_copy);
		ret = vfs_read(filp, (char __user *)dp, l, &pos);
		/*strcpy(tunning_data+pos_copy, dp);*/
		strncpy(tunning_data+pos_copy, dp,
				pos);
		if (ret != l)
			break;
	}

	/*Copy the data contained in the file s5k5ccgx.h
	  to s5k5ccgx_regs_table*/
	s5k5ccgx_regs_table = tunning_data;
	s5k5ccgx_regs_table_size = file_size;
	*((s5k5ccgx_regs_table + s5k5ccgx_regs_table_size) - 1) = '\0';

	/*Close the file s5k5ccgx.h*/
	filp_close(filp, current->files);
	set_fs(fs);
	kfree(dp);
	return 1;

}

void s5k5ccgx_regs_table_exit(void)
{
	pr_info("%s %d\n", __func__, __LINE__);
	if (s5k5ccgx_regs_table != NULL) {
		kfree(s5k5ccgx_regs_table);
		s5k5ccgx_regs_table = NULL;
	}
}


static int s5k5ccgx_is_hexnum(char *num)
{
	int i;
	for (i = 2; num[i] != '\0' ; i++) {
		if (!((num[i] >= '0' && num[i] <= '9') ||
					(num[i] >= 'a' && num[i] <= 'f') ||
					(num[i] >= 'A' && num[i] <= 'F'))) {
			return 0;
		}
	}
	return 1;
}

char *sec_strstr(const char *s, const char *substring)
{
	/*int n = strlen(substring);*/
	int n = strnlen(substring, sizeof(substring)-1);

	for (; *s != '\0'; s++)
		if (strncmp(s, substring, n) == 0)
			return (char *)s;

	return NULL;
}


static int s5k5ccgx_regs_table_write(char *name)
{
	char *start, *end, *reg;/*, *data;*/
	unsigned short addr, value;
	unsigned long data;
	char data_buf[11];

	addr = value = 0;

	*(data_buf + 10) = '\0';

	start = sec_strstr(s5k5ccgx_regs_table, name);

	end = sec_strstr(start, "};");

	while (1) {
		/* Find Address */
		reg = sec_strstr(start, "0x");
		if (reg)
			start = (reg + 10);

		if ((reg == NULL) || (reg > end)) {
			pr_info("[s5k5ccgx] write end of %s\n", name);
			break;
		}
		/* Write Value to Address */
		memcpy(data_buf, reg, 10);

		if (s5k5ccgx_is_hexnum(data_buf) == 0) {
			pr_info("[s5k5ccgx] it's not hex number %s\n",
					data_buf);
			continue;
		}

	/*	data = (unsigned long)simple_strtoul(data_buf, NULL, 16);*/
		data = (unsigned long)kstrtol(data_buf, NULL, 16);
		/*pr_info("data 0x%08x\n", data);*/
		addr = (data >> 16);
		value = (data & 0xffff);

		if (addr == 0xffff) {
			msleep(value);
			pr_info("delay 0x%04x, value 0x%04x\n", addr, value);
		} else {
			if (s5k5ccgx_sensor_write(addr, value) < 0)
				pr_info("[s5k5ccgx] %s fail on sensor_write :"
						"addr[0x%04x], value[0x%04x]\n",
						__func__, addr, value);

		}
	}

	return 0;
}
#endif



int s5k5ccgx_sensor_init(const struct msm_camera_sensor_info *data)
{
	int rc = 0;

	s5k5ccgx_ctrl = kzalloc(sizeof(struct s5k5ccgx_ctrl), GFP_KERNEL);
	if (!s5k5ccgx_ctrl) {
		CDBG("s5k5ccgx_init failed!\n");
		rc = -ENOMEM;
		goto init_done;
	}

	if (data)
		s5k5ccgx_ctrl->sensordata = data;

	mEffect = EXT_CFG_EFFECT_NORMAL;
	mBrightness = EXT_CFG_BR_STEP_0;
	mContrast = EXT_CFG_CR_STEP_0;
	mSaturation = EXT_CFG_SA_STEP_0;
	mSharpness = EXT_CFG_SP_STEP_0;
	mWhiteBalance = EXT_CFG_WB_AUTO;
	mISO = EXT_CFG_ISO_AUTO;
	mAutoExposure = EXT_CFG_METERING_NORMAL;
	mScene = EXT_CFG_SCENE_OFF;
	mAfMode = EXT_CFG_AF_SET_NORMAL;
	mCurAfMode = EXT_CFG_AF_OFF;
	mFPS = EXT_CFG_FRAME_AUTO;
	mMode = S5K5CCGX_MODE_PREVIEW;
	mDTP = 0;
	mInit = 0;

	config_csi2 = 0;
#ifdef CONFIG_LOAD_FILE
	s5k5ccgx_regs_table_init();
#endif


	s5k5ccgx_set_power(CAM_POWER_ON);
	/*msleep(10);*/
	usleep(10*1000);

	rc = s5k5ccgx_check_sensor_id();

	if (rc < 0) {
		CDBG("s5k5ccgx_sensor_init failed!\n");
		goto init_fail;
	}

	S5K5CCGX_WRITE_LIST(s5k5ccgx_pre_init0);

init_done:
	return rc;

init_fail:
	s5k5ccgx_set_power(CAM_POWER_OFF);
	kfree(s5k5ccgx_ctrl);
	return rc;
}

static int s5k5ccgx_init_client(struct i2c_client *client)
{
	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&s5k5ccgx_wait_queue);
	return 0;
}

int s5k5ccgx_sensor_config(void __user *argp)
{
	struct sensor_cfg_data cfg_data;
	long rc = 0;

	if (copy_from_user(&cfg_data,
				(void *)argp,
				sizeof(struct sensor_cfg_data)))
		return -EFAULT;

	/* down(&s5k5ccgx_sem); */

	CDBG("s5k5ccgx_ioctl, cfgtype = %d, mode = %d\n",\
			cfg_data.cfgtype, cfg_data.mode);

	switch (cfg_data.cfgtype) {
	case CFG_SET_MODE:
		rc = s5k5ccgx_set_sensor_mode(
				cfg_data.mode);
		break;

	case CFG_SET_EFFECT:
		/*rc = s5k5ccgx_set_effect(cfg_data.mode,
		  cfg_data.cfg.effect);*/
		break;

	case CFG_GET_AF_MAX_STEPS:
	default:
		rc = -EINVAL;
		pr_info("s5k5ccgx_sensor_config : Invalid cfgtype ! %d\n",\
				cfg_data.cfgtype);
		break;
	}

	/* up(&s5k5ccgx_sem); */

	return rc;
}

int s5k5ccgx_sensor_release(void)
{
	int rc = 0;

	/*If did not init below that, it can keep the previous status.
	 it depend on concept by PCAM*/
	mEffect = EXT_CFG_EFFECT_NORMAL;
	mBrightness = EXT_CFG_BR_STEP_0;
	mContrast = EXT_CFG_CR_STEP_0;
	mSaturation = EXT_CFG_SA_STEP_0;
	mSharpness = EXT_CFG_SP_STEP_0;
	mWhiteBalance = EXT_CFG_WB_AUTO;
	mISO = EXT_CFG_ISO_AUTO;
	mAutoExposure = EXT_CFG_METERING_NORMAL;
	mScene = EXT_CFG_SCENE_OFF;
	mAfMode = EXT_CFG_AF_SET_NORMAL;
	mCurAfMode = EXT_CFG_AF_OFF;
	mFPS = EXT_CFG_FRAME_AUTO;
	mMode = S5K5CCGX_MODE_PREVIEW;
	mDTP = 0;
	mInit = 0;

	if (mAfMode == EXT_CFG_AF_SET_MACRO) {
		pr_info("[S5K5CCGX] wait change lens position\n");
		s5k5ccgx_af_control(EXT_CFG_AF_SET_NORMAL);
		msleep(150);
	} else
		pr_info("[S5K5CCGX] mAfMode : %d\n", mAfMode);

	pr_info("[S5K5CCGX] s5k5ccgx_sensor_release\n");

	kfree(s5k5ccgx_ctrl);
	/* up(&s5k5ccgx_sem); */

#ifdef CONFIG_LOAD_FILE
	s5k5ccgx_regs_table_exit();
#endif

	s5k5ccgx_set_power(CAM_POWER_OFF);
	return rc;
}

static int s5k5ccgx_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int rc = 0;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		rc = -ENOTSUPP;
		goto probe_failure;
	}

	s5k5ccgx_sensorw =
		kzalloc(sizeof(struct s5k5ccgx_work), GFP_KERNEL);

	if (!s5k5ccgx_sensorw) {
		rc = -ENOMEM;
		goto probe_failure;
	}

	camera_class = class_create(THIS_MODULE, "camera");

	if (IS_ERR(camera_class))
		pr_err("Failed to create class(camera)!\n");

	s5k5ccgx_dev = device_create(camera_class, NULL, 0, NULL, "rear");
	if (IS_ERR(s5k5ccgx_dev)) {
		pr_err("Failed to create device!");
		goto probe_failure;
	}

	if (device_create_file(s5k5ccgx_dev, &dev_attr_rear_camtype) < 0) {
		CDBG("failed to create device file, %s\n",
		dev_attr_rear_camtype.attr.name);
	}

	i2c_set_clientdata(client, s5k5ccgx_sensorw);
	s5k5ccgx_init_client(client);
	s5k5ccgx_client = client;


	CDBG("s5k5ccgx_probe succeeded!\n");

	return 0;

probe_failure:
	kfree(s5k5ccgx_sensorw);
	s5k5ccgx_sensorw = NULL;
	CDBG("s5k5ccgx_probe failed!\n");
	return rc;
}

static const struct i2c_device_id s5k5ccgx_i2c_id[] = {
	{ "s5k5ccgx", 0},
	{ },
};

static struct i2c_driver s5k5ccgx_i2c_driver = {
	.id_table = s5k5ccgx_i2c_id,
	.probe = s5k5ccgx_i2c_probe,
	.remove = __exit_p(s5k5ccgx_i2c_remove),
	.driver = {
		.name = "s5k5ccgx",
	},
};

static int s5k5ccgx_sensor_probe(const struct msm_camera_sensor_info *info,
		struct msm_sensor_ctrl *s)
{
	/*	unsigned short read_value;*/
	unsigned short	id = 0;

	int rc = i2c_add_driver(&s5k5ccgx_i2c_driver);
	if (rc < 0 || s5k5ccgx_client == NULL) {
		rc = -ENOTSUPP;
		goto probe_done;
	}

	s5k5ccgx_set_power(CAM_POWER_ON);
	/*msleep(10);*/
	usleep(10*1000);

	rc = s5k5ccgx_check_sensor_id();
	if (rc < 0) {
		CDBG("s5k5ccgx_sensor_init failed!\n");
		goto probe_done;
	}

	s->s_init = s5k5ccgx_sensor_init;
	s->s_release = s5k5ccgx_sensor_release;
	s->s_config = s5k5ccgx_sensor_config;
//kk0704.park :: ARUBA_TEMP	s->s_ext_config = s5k5ccgx_sensor_ext_config;

//kk0704.park :: ARUBA_TEMP	s->s_camera_type = BACK_CAMERA_2D;
	s->s_mount_angle = 90;


probe_done:

	s5k5ccgx_set_power(CAM_POWER_OFF);

	pr_info("%s %s:%d\n", __FILE__, __func__, __LINE__);
	return rc;
}

static int __devinit s5k5ccgx_probe(struct platform_device *pdev)
{
	return msm_camera_drv_start(pdev, s5k5ccgx_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
	.probe = s5k5ccgx_probe,
	.driver = {
		.name = "msm_camera_s5k5ccgx",
		.owner = THIS_MODULE,
	},
};

static int __init s5k5ccgx_init(void)
{
	return platform_driver_register(&msm_camera_driver);
}

module_init(s5k5ccgx_init);
MODULE_DESCRIPTION("Samsung 3 MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");
