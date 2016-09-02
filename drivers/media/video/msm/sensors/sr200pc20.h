/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
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

#ifndef __SR200PC20_H__
#define __SR200PC20_H__

#include "msm_sensor.h"
#define SENSOR_NAME "sr200pc20"
#define PLATFORM_DRIVER_NAME "msm_camera_sr200pc20"
#define sr200pc20_obj sr200pc20_##obj


#include <linux/types.h>
#include <mach/board.h>

#undef DEBUG_LEVEL_HIGH
#undef DEBUG_LEVEL_MID
#define DEBUG_LEVEL_HIGH
/*#define DEBUG_LEVEL_MID */

#if defined(DEBUG_LEVEL_HIGH)
#define CAM_DEBUG(fmt, arg...)	\
	do {					\
		printk(KERN_DEBUG "[%s:%d] " fmt,	\
			__func__, __LINE__, ##arg);	\
	}						\
	while (0)

#define cam_info(fmt, arg...)			\
	do {					\
		printk(KERN_INFO "[%s:%d] " fmt,	\
			__func__, __LINE__, ##arg);	\
	}						\
	while (0)
#elif defined(DEBUG_LEVEL_MID)
#define CAM_DEBUG(fmt, arg...)
#define cam_info(fmt, arg...)			\
	do {					\
		printk(KERN_INFO "[%s:%d] " fmt,	\
			__func__, __LINE__, ##arg);	\
	}						\
	while (0)
#else
#define CAM_DEBUG(fmt, arg...)
#define cam_info(fmt, arg...)
#endif

#undef DEBUG_CAM_I2C
#define DEBUG_CAM_I2C

#if defined(DEBUG_CAM_I2C)
#define cam_i2c_dbg(fmt, arg...)		\
	do {					\
		printk(KERN_DEBUG "[%s:%d] " fmt,	\
			__func__, __LINE__, ##arg);	\
	}						\
	while (0)
#else
#define cam_i2c_dbg(fmt, arg...)
#endif


#define cam_err(fmt, arg...)	\
	do {					\
		printk(KERN_ERR "[%s:%d] " fmt,		\
			__func__, __LINE__, ##arg);	\
	}						\
	while (0)

#define SR200PC20_DELAY		0xFF00

struct sr200pc20_userset {
	unsigned int	focus_mode;
	unsigned int	focus_status;
	unsigned int	continuous_af;

	unsigned int	metering;
	unsigned int	exposure;
	unsigned int	wb;
	unsigned int	iso;
	int				contrast;
	int				saturation;
	int				sharpness;
	int				brightness;
	int				ev;
	int				scene;
	unsigned int	zoom;
	unsigned int	effect;		/* Color FX (AKA Color tone) */
	unsigned int	scenemode;
	unsigned int	detectmode;
	unsigned int	antishake;
	unsigned int	fps;
	unsigned int	flash_mode;
	unsigned int	flash_state;
	unsigned int	stabilize;	/* IS */
	unsigned int	strobe;
	unsigned int	jpeg_quality;
	/*unsigned int preview_size;*/
	unsigned int	preview_size_idx;
	unsigned int	capture_size;
	unsigned int	thumbnail_size;
};

struct sr200pc20_work {
	struct work_struct work;
};

struct sr200pc20_exif_data {
	unsigned short iso;
	unsigned short shutterspeed;
};

struct sr200pc20_ctrl {
	const struct msm_camera_sensor_info *sensordata;
	struct sr200pc20_userset settings;
	struct msm_camera_i2c_client *sensor_i2c_client;
	struct msm_sensor_ctrl_t *s_ctrl;
	struct v4l2_subdev *sensor_dev;
	struct v4l2_subdev sensor_v4l2_subdev;
	struct v4l2_subdev_info *sensor_v4l2_subdev_info;
	uint8_t sensor_v4l2_subdev_info_size;
	struct v4l2_subdev_ops *sensor_v4l2_subdev_ops;

	int op_mode;
	int dtp_mode;
	int cam_mode;
	int vtcall_mode;
	int started;
	int flash_mode;
	int lowLight;
	int dtpTest;
	int af_mode;
	int af_status;
	unsigned int lux;
	int awb_mode;
	int samsungapp;
};

enum sr200pc20_camera_mode_t {
	PREVIEW_MODE = 0,
	MOVIE_MODE
};

enum sr200pc20_setting {
	RES_PREVIEW,
	RES_CAPTURE
};

enum sr200pc20_reg_update {
	/* Sensor egisters that need to be updated during initialization */
	REG_INIT,
	/* Sensor egisters that needs periodic I2C writes */
	UPDATE_PERIODIC,
	/* All the sensor Registers will be updated */
	UPDATE_ALL,
	/* Not valid update */
	UPDATE_INVALID
};

static struct sr200pc20_ctrl sr200pc20_control;
static struct i2c_client *sr200pc20_client;
static struct sr200pc20_exif_data sr200pc20_exif;
static struct msm_sensor_ctrl_t sr200pc20_s_ctrl;
static struct device sr200pc20_dev;
static DECLARE_WAIT_QUEUE_HEAD(sr200pc20_wait_queue);
DEFINE_MUTEX(sr200pc20_mut);

#endif /* __sr200pc20_H__ */
