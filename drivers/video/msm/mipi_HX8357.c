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

#include <linux/fb.h>
#include <linux/lcd.h>

#include <mach/gpio.h>
#include <mach/pmic.h>
#include <mach/vreg.h>

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_HX8357.h"

#include "lcdc_backlight_ic.h"

int read_recovery=0;
EXPORT_SYMBOL(read_recovery);

extern int charging_boot;

static int __init hx8357d_lcd_read_recovery(char *param)
{
	if( param[0]=='1' )
	{
		read_recovery = 1;
		printk ( "@@@@@%s-read_recovery is 1\n", __func__ );
	}
	else
	{
		read_recovery = 0;
		printk ( "@@@@@%s-read_recovery is 0\n", __func__ );
	}
}
__setup("recovery=", hx8357d_lcd_read_recovery);

static struct msm_panel_common_pdata *mipi_hx8357_pdata;
static struct dsi_buf hx8357_tx_buf;
static struct dsi_buf hx8357_rx_buf;

typedef enum
{
	DISP_STATE_UNKNOWN = 0,
	DISP_STATE_ON,
	DISP_STATE_OFF
} disp_state;

/*
 * when we first time enter kernel
 * the LCD is powered on by bootloaer
 *
 * also: initialising makes init_power_state unneeded
 * (caused serious lockups on Kyle rev 0.2 devides)
 */
static disp_state disp_powered_up = DISP_STATE_ON;

/*
 * Commands used for powerin in the panel.
 * They depend on concrete LCD and are passed to
 * initialiser by specific variants of this driver
 */
struct dsi_cmd_desc *concrete_hx8357_prepare_cmds;
int concrete_hx8357_prepare_cmds_len;

struct msm_fb_data_type *mdnie_mfd;
static char tuning_file_name[50] = {0, };
static unsigned char hx8357_mdnie_data[113] = {0, };

static struct dsi_cmd_desc hx8357_display_mdnie_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_CMD_SETTLE, sizeof(hx8357_mdnie_data), hx8357_mdnie_data}
};

//#define HX8357_MDNIE_ENABLE

#ifdef HX8357_MDNIE_ENABLE

enum Lcd_mDNIe_UI {
	mDNIe_UI_MODE,			// 0 : DEFAULT
	mDNIe_VIDEO_MODE,		// 1 : VIDEO
	mDNIe_VIDEO_WARM_MODE,	// 2 : VIDEO WARM
	mDNIe_VIDEO_COLD_MODE,	// 3 : VIDEO COLD
	mDNIe_CAMERA_MODE,		// 4 : CAMERA
	mDNIe_GALLERY_MODE=6,	// 6 : GALLERY
};

enum Lcd_mDNIe_Outdoor {
	mDNIe_OUTDOOR_OFF = 0,
	mDNIe_OUTDOOR_ON,
};

enum Lcd_mDNIe_Negative {
	mDNIe_NEGATIVE_OFF = 0,
	mDNIe_NEGATIVE_ON,
};

static struct class *mdnie_class;
struct device *tune_mdnie_dev;

enum Lcd_mDNIe_UI current_mDNIe_Mode = mDNIe_UI_MODE;
enum Lcd_mDNIe_Outdoor current_Outdoor_Mode = mDNIe_OUTDOOR_OFF;
enum Lcd_mDNIe_Negative current_Negative_Mode = mDNIe_NEGATIVE_OFF;


static unsigned char hx8357_mdnie_ui_data[] = {
	0xCD,
	0x5A,
	0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0xFF,0x00,0xFF,0x00,0xFF,0xFF,0x00,0x00,0xFF,
	0x00,0xFF,0x00,0xFF,0xFF,0x00,0x00,0xFF,0x00,0xFF,
	0x00,0xFF,0xFF,0x00,0x02,0x20,0x00,0x20,0x00,0x20,
	0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,
	0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,
	0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,
	0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,
	0x00,0x20,0x00,0x05,0x6b,0x1f,0x10,0x1f,0x85,0x1f,
	0xd1,0x04,0xa9,0x1f,0x86,0x1f,0xd1,0x1f,0x10,0x05,
	0x1f,
};
static unsigned char hx8357_mdnie_video_data[] = {
	0xCD,
	0x5A,
	0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0xFF,0x00,0xFF,0x00,0xFF,0xFF,0x00,0x00,0xFF,
	0x00,0xFF,0x00,0xFF,0xFF,0x00,0x00,0xFF,0x00,0xFF,
	0x00,0xFF,0xFF,0x00,0x06,0x20,0x00,0x20,0x00,0x20,
	0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,
	0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,
	0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,
	0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,
	0x00,0x20,0x00,0x05,0x6b,0x1f,0x10,0x1f,0x85,0x1f,
	0xd1,0x04,0xa9,0x1f,0x86,0x1f,0xd1,0x1f,0x10,0x05,
	0x1f,
};
static unsigned char hx8357_mdnie_video_outdoor_data[] = {
	0xCD,
	0x5A,
	0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0xFF,0x00,0xFF,0x00,0xFF,0xFF,0x00,0x00,0xFF,
	0x00,0xFF,0x00,0xFF,0xFF,0x00,0x00,0xFF,0x00,0xFF,
	0x00,0xFF,0xFF,0x00,0x03,0xff,0x00,0x0a,0xaf,0x0d,
	0x99,0x14,0x6d,0x1b,0x48,0x2c,0x05,0xb4,0x0f,0xbe,
	0x26,0xbe,0x26,0xbe,0x26,0xbe,0x26,0xae,0x0c,0xae,
	0x0c,0xae,0x0c,0xae,0x0c,0xae,0x0c,0xae,0x0c,0x20,
	0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,
	0x00,0x20,0x00,0x06,0x7b,0x1e,0x5b,0x1f,0x2a,0x1f,
	0xae,0x05,0x28,0x1f,0x2a,0x1f,0xae,0x1e,0x5b,0x05,
	0xf7,
};
static unsigned char hx8357_mdnie_video_warm_data[] = {
	0xCD,
	0x5A,
	0x00,0x00,0x33,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0xcf,0x00,0xfa,0x00,0xff,0xFF,0x00,0x00,0xFF,
	0x00,0xFF,0x00,0xFF,0xFF,0x00,0x00,0xFF,0x00,0xFF,
	0x00,0xFF,0xFF,0x00,0x06,0x20,0x00,0x20,0x00,0x20,
	0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,
	0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,
	0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,
	0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,
	0x00,0x20,0x00,0x05,0x6b,0x1f,0x10,0x1f,0x85,0x1f,
	0xd1,0x04,0xa9,0x1f,0x86,0x1f,0xd1,0x1f,0x10,0x05,
	0x1f,
};
static unsigned char hx8357_mdnie_video_warm_outdoor_data[] = {
	0xCD,
	0x5A,
	0x00,0x00,0x33,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0xcf,0x00,0xfa,0x00,0xff,0xFF,0x00,0x00,0xFF,
	0x00,0xFF,0x00,0xFF,0xFF,0x00,0x00,0xFF,0x00,0xFF,
	0x00,0xFF,0xFF,0x00,0x07,0xff,0x00,0x0a,0xaf,0x0d,
	0x99,0x14,0x6d,0x1b,0x48,0x2c,0x05,0xb4,0x0f,0xbe,
	0x26,0xbe,0x26,0xbe,0x26,0xbe,0x26,0xae,0x0c,0xae,
	0x0c,0xae,0x0c,0xae,0x0c,0xae,0x0c,0xae,0x0c,0x20,
	0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,
	0x00,0x20,0x00,0x06,0x7b,0x1e,0x5b,0x1f,0x2a,0x1f,
	0xae,0x05,0x28,0x1f,0x2a,0x1f,0xae,0x1e,0x5b,0x05,
	0xf7,
};
static unsigned char hx8357_mdnie_video_cold_data[] = {
	0xCD,
	0x5A,
	0x00,0x00,0x33,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0xff,0x00,0xf0,0x00,0xea,0xFF,0x00,0x00,0xFF,
	0x00,0xFF,0x00,0xFF,0xFF,0x00,0x00,0xFF,0x00,0xFF,
	0x00,0xFF,0xFF,0x00,0x06,0x20,0x00,0x20,0x00,0x20,
	0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,
	0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,
	0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,
	0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,
	0x00,0x20,0x00,0x05,0x6b,0x1f,0x10,0x1f,0x85,0x1f,
	0xd1,0x04,0xa9,0x1f,0x86,0x1f,0xd1,0x1f,0x10,0x05,
	0x1f,
};
static unsigned char hx8357_mdnie_video_cold_outdoor_data[] = {
	0xCD,
	0x5A,
	0x00,0x00,0x33,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0xff,0x00,0xf0,0x00,0xea,0xFF,0x00,0x00,0xFF,
	0x00,0xFF,0x00,0xFF,0xFF,0x00,0x00,0xFF,0x00,0xFF,
	0x00,0xFF,0xFF,0x00,0x07,0xff,0x00,0x0a,0xaf,0x0d,
	0x99,0x14,0x6d,0x1b,0x48,0x2c,0x05,0xb4,0x0f,0xbe,
	0x26,0xbe,0x26,0xbe,0x26,0xbe,0x26,0xae,0x0c,0xae,
	0x0c,0xae,0x0c,0xae,0x0c,0xae,0x0c,0xae,0x0c,0x20,
	0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,
	0x00,0x20,0x00,0x06,0x7b,0x1e,0x5b,0x1f,0x2a,0x1f,
	0xae,0x05,0x28,0x1f,0x2a,0x1f,0xae,0x1e,0x5b,0x05,
	0xf7,
};
static unsigned char hx8357_mdnie_camera_data[] = {
	0xCD,
	0x5A,
	0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0xFF,0x00,0xFF,0x00,0xFF,0xFF,0x00,0x00,0xFF,
	0x00,0xFF,0x00,0xFF,0xFF,0x00,0x00,0xFF,0x00,0xFF,
	0x00,0xFF,0xFF,0x00,0x00,0x01,0x00,0x01,0x00,0x01,
	0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x01,
	0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x01,
	0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x01,
	0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x01,
	0x00,0x01,0x00,0x08,0x32,0x1C,0x71,0x1F,0x90,0x1E,
	0xCD,0x07,0x4C,0x1E,0x06,0x1F,0x66,0x1E,0x2A,0x06,
	0x70,
};
static unsigned char hx8357_mdnie_gallery_data[] = {
	0xCD,
	0x5A,
	0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0xFF,0x00,0xFF,0x00,0xFF,0xFF,0x00,0x00,0xFF,
	0x00,0xFF,0x00,0xFF,0xFF,0x00,0x00,0xFF,0x00,0xFF,
	0x00,0xFF,0xFF,0x00,0x06,0x20,0x00,0x20,0x00,0x20,
	0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,
	0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,
	0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,
	0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,
	0x00,0x20,0x00,0x05,0x6b,0x1f,0x10,0x1f,0x85,0x1f,
	0xd1,0x04,0xa9,0x1f,0x86,0x1f,0xd1,0x1f,0x10,0x05,
	0x1f,
};
static unsigned char hx8357_mdnie_negative_data[] = {
	0xCD,
	0x5A,
	0x00,0x00,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0xff,0x00,0xff,0x00,0xff,0x00,0x00,0xff,0xff,0x00,
	0xff,0x00,0xff,0x00,0x00,0xff,0xff,0x00,0xff,0x00,
	0xff,0x00,0x00,0xff,0x02,0x20,0x00,0x20,0x00,0x20,
	0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,
	0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,
	0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,
	0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,
	0x00,0x20,0x00,0x05,0xc6,0x1e,0xd3,0x1f,0x67,0x1f,
	0xc6,0x04,0xd3,0x1f,0x67,0x1f,0xc6,0x1e,0xd3,0x05,
	0x67,
};

static struct dsi_cmd_desc hx8357_mdnie_ui_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_CMD_SETTLE, sizeof(hx8357_mdnie_ui_data), hx8357_mdnie_ui_data}
};
static struct dsi_cmd_desc hx8357_mdnie_video_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_CMD_SETTLE, sizeof(hx8357_mdnie_video_data), hx8357_mdnie_video_data}
};
static struct dsi_cmd_desc hx8357_mdnie_video_outdoor_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_CMD_SETTLE, sizeof(hx8357_mdnie_video_outdoor_data), hx8357_mdnie_video_outdoor_data}
};
static struct dsi_cmd_desc hx8357_mdnie_video_warm_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_CMD_SETTLE, sizeof(hx8357_mdnie_video_warm_data), hx8357_mdnie_video_warm_data}
};
static struct dsi_cmd_desc hx8357_mdnie_video_warm_outdoor_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_CMD_SETTLE, sizeof(hx8357_mdnie_video_warm_outdoor_data), hx8357_mdnie_video_warm_outdoor_data}
};
static struct dsi_cmd_desc hx8357_mdnie_video_cold_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_CMD_SETTLE, sizeof(hx8357_mdnie_video_cold_data), hx8357_mdnie_video_cold_data}
};
static struct dsi_cmd_desc hx8357_mdnie_video_cold_outdoor_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_CMD_SETTLE, sizeof(hx8357_mdnie_video_cold_outdoor_data), hx8357_mdnie_video_cold_outdoor_data}
};
static struct dsi_cmd_desc hx8357_mdnie_camera_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_CMD_SETTLE, sizeof(hx8357_mdnie_camera_data), hx8357_mdnie_camera_data}
};
static struct dsi_cmd_desc hx8357_mdnie_gallery_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_CMD_SETTLE, sizeof(hx8357_mdnie_gallery_data), hx8357_mdnie_gallery_data}
};
static struct dsi_cmd_desc hx8357_mdnie_negative_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_CMD_SETTLE, sizeof(hx8357_mdnie_negative_data), hx8357_mdnie_negative_data}
};

#endif

int lcd_reset;
EXPORT_SYMBOL(lcd_reset);

#define VREG_ENABLE	TRUE
#define VREG_DISABLE	FALSE

#define LCDC_DEBUG

#ifdef LCDC_DEBUG
#define DPRINT(x...)	printk(KERN_ERR "HX8357 " x)
#else
#define DPRINT(x...)
#endif

static void hx8357_vreg_config(boolean vreg_en)
{
	int rc;
	struct vreg *vreg_lcd = NULL;

	DPRINT("start %s\n", __func__);
	if (vreg_lcd == NULL) {
		vreg_lcd = vreg_get(NULL, "vlcd");

		if (IS_ERR(vreg_lcd)) {
			printk(KERN_ERR "%s: vreg_get(%s) failed (%ld)\n",
			       __func__, "vlcd4", PTR_ERR(vreg_lcd));
			return;
		}

		rc = vreg_set_level(vreg_lcd, 3000);
		if (rc) {
			printk(KERN_ERR "%s: LCD set_level failed (%d)\n",
			       __func__, rc);
		}
	}

	if (vreg_en) {
		rc = vreg_enable(vreg_lcd);
		if (rc) {
			printk(KERN_ERR "%s: LCD enable failed (%d)\n",
			       __func__, rc);
		} else {
			printk(KERN_ERR "%s: LCD enable success (%d)\n",
			       __func__, rc);
		}
	} else {
		rc = vreg_disable(vreg_lcd);
		if (rc) {
			printk(KERN_ERR "%s: LCD disable failed (%d)\n",
			       __func__, rc);
		} else {
			printk(KERN_ERR "%s: LCD disable success (%d)\n",
			       __func__, rc);
		}
	}
}

static void hx8357_disp_powerup(void)
{
	static int is_booting_on = 1;

	DPRINT("start %s\n", __func__);

	if( is_booting_on == 1 )
	{
		is_booting_on = 0;

		hx8357_vreg_config(VREG_ENABLE); 
		msleep(20);
#ifndef CONFIG_MACH_HENNESSY_DUOS_CTC
		return; // for first booting
#endif
	}
	
	if( read_recovery > 0 )
	{
		hx8357_vreg_config(VREG_ENABLE);
		msleep(20);
	}

	gpio_tlmm_config(GPIO_CFG
			 (lcd_reset, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
			  GPIO_CFG_2MA)
			 , GPIO_CFG_ENABLE);

	gpio_set_value(lcd_reset, 0);
	msleep(20);
	gpio_set_value(lcd_reset, 1);
	msleep(100);
}
static void hx8357_disp_powerdown(void)
{	

	DPRINT("start %s\n", __func__);
	
	if( read_recovery > 0 )
	{
		gpio_set_value(lcd_reset, 0);
		msleep(20);
		hx8357_vreg_config(VREG_DISABLE);
	}
}

/* not used as it causes serious lockups on rev0.2 devices */
#define HX8357_DISPLAY_POWER_MODE_SLEEP_OUT (0x10)
#define HX8357_DISPLAY_POWER_MODE_DISPLAY_ON (0x04)
static void mipi_hx8357_init_power_state(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	DPRINT("start %s\n", __func__);

	disp_powered_up = DISP_STATE_OFF;

	if (!pdev)
		return;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return;
	if (mfd->key != MFD_KEY)
		return;

	mipi_dsi_cmds_rx(mfd, &hx8357_tx_buf, &hx8357_rx_buf,
			 hx8357_read_display_power_mode_cmds,
			 ARRAY_SIZE(hx8357_read_display_power_mode_cmds));

	if (
		(HX8357_DISPLAY_POWER_MODE_SLEEP_OUT
		 | HX8357_DISPLAY_POWER_MODE_DISPLAY_ON)
		== (hx8357_rx_buf.data[0] &
		  (HX8357_DISPLAY_POWER_MODE_SLEEP_OUT
		   | HX8357_DISPLAY_POWER_MODE_DISPLAY_ON)
	    )
	    ) {
		disp_powered_up = DISP_STATE_ON;
	}
	DPRINT("%s(length %d)[%x] = %d\n",
	       __func__,
	       hx8357_rx_buf.len,
	       hx8357_rx_buf.data[0], disp_powered_up);
}

static int mipi_hx8357_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;
	mdnie_mfd = platform_get_drvdata(pdev);

	static int is_charging_boot_first = 1;

	/* workaround : when recovery mode, do lcd on only at first */
	if( read_recovery == 1 )
		read_recovery = 2;
	else
		;
	/************************************************************/

	/*
	 * at this point in initialization
	 * regardless whether display is on or off
	 * tx has to be working properly
	 * this function will be called only once
	 * since after it returns disp_powered up will
	 * have proper value
	 */
	/* CHANGED: never used as it caused lockups */
	if(DISP_STATE_UNKNOWN == disp_powered_up) {
		mipi_hx8357_init_power_state(pdev);
	}

#if 0
	if (DISP_STATE_ON == disp_powered_up) {
		DPRINT(" %s: already on\n", __func__);
		hx8357_vreg_config(VREG_ENABLE);
		return 0;
	}
#endif

	DPRINT("start %s\n", __func__);

	if (!pdev)
		return -ENODEV;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;

	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi = &mfd->panel_info.mipi;

	hx8357_disp_powerup();

		mipi_dsi_cmds_tx(&hx8357_tx_buf,
				 concrete_hx8357_prepare_cmds,
				 concrete_hx8357_prepare_cmds_len);

#ifdef HX8357_MDNIE_ENABLE

	if( current_Negative_Mode == mDNIe_NEGATIVE_ON )
	{
		mipi_dsi_cmds_tx(&hx8357_tx_buf, hx8357_mdnie_negative_cmds, ARRAY_SIZE(hx8357_mdnie_negative_cmds));
	}
	else
	{
		switch ( current_mDNIe_Mode ) {
		case mDNIe_UI_MODE:
			mipi_dsi_cmds_tx(&hx8357_tx_buf, hx8357_mdnie_ui_cmds, ARRAY_SIZE(hx8357_mdnie_ui_cmds));
			break;
		case mDNIe_VIDEO_MODE:
			if( current_Outdoor_Mode == mDNIe_OUTDOOR_ON )
			{
				mipi_dsi_cmds_tx(&hx8357_tx_buf, hx8357_mdnie_video_outdoor_cmds, ARRAY_SIZE(hx8357_mdnie_video_outdoor_cmds));
			}
			else
			{
				mipi_dsi_cmds_tx(&hx8357_tx_buf, hx8357_mdnie_video_cmds, ARRAY_SIZE(hx8357_mdnie_video_cmds));
			}
			break;
		case mDNIe_VIDEO_WARM_MODE:
			if( current_Outdoor_Mode == mDNIe_OUTDOOR_ON )
			{
				mipi_dsi_cmds_tx(&hx8357_tx_buf, hx8357_mdnie_video_warm_outdoor_cmds, ARRAY_SIZE(hx8357_mdnie_video_warm_outdoor_cmds));
			}
			else
			{
				mipi_dsi_cmds_tx(&hx8357_tx_buf, hx8357_mdnie_video_warm_cmds, ARRAY_SIZE(hx8357_mdnie_video_warm_cmds));
			}
			break;
		case mDNIe_VIDEO_COLD_MODE:
			if( current_Outdoor_Mode == mDNIe_OUTDOOR_ON )
			{
				mipi_dsi_cmds_tx(&hx8357_tx_buf, hx8357_mdnie_video_cold_outdoor_cmds, ARRAY_SIZE(hx8357_mdnie_video_cold_outdoor_cmds));
			}
			else
			{
				mipi_dsi_cmds_tx(&hx8357_tx_buf, hx8357_mdnie_video_cold_cmds, ARRAY_SIZE(hx8357_mdnie_video_cold_cmds));
			}
			break;
		case mDNIe_CAMERA_MODE:
			mipi_dsi_cmds_tx(&hx8357_tx_buf, hx8357_mdnie_camera_cmds, ARRAY_SIZE(hx8357_mdnie_camera_cmds));
			break;
		case mDNIe_GALLERY_MODE:
			mipi_dsi_cmds_tx(&hx8357_tx_buf, hx8357_mdnie_gallery_cmds, ARRAY_SIZE(hx8357_mdnie_gallery_cmds));
			break;
		default:
			printk ( "default\n" );
			mipi_dsi_cmds_tx(&hx8357_tx_buf, hx8357_mdnie_ui_cmds, ARRAY_SIZE(hx8357_mdnie_ui_cmds));
			break;
		}
	}

#endif

	if( (charging_boot == 1) && (is_charging_boot_first == 1) )
	{
		is_charging_boot_first = 0;
		msleep ( 100 );
		backlight_ic_set_brightness(255);
	}

	disp_powered_up = DISP_STATE_ON;

	DPRINT("exit %s\n", __func__);
	return 0;
}

static int mipi_hx8357_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	if (DISP_STATE_OFF == disp_powered_up) {
		DPRINT(" %s: already off\n", __func__);
		return 0;
	}

	DPRINT("start %s\n", __func__);

	if (!pdev)
		return -ENODEV;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	backlight_ic_set_brightness(0);
	mipi_dsi_cmds_tx(&hx8357_tx_buf, hx8357_display_off_cmds,
			 ARRAY_SIZE(hx8357_display_off_cmds));

	msleep(20);

	disp_powered_up = DISP_STATE_OFF;
	hx8357_disp_powerdown();

	DPRINT("exit %s\n", __func__);
	return 0;
}

/** sysfs handling ***/
static ssize_t mipi_hx8357_lcdtype_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
#if defined(CONFIG_MACH_KYLE)
	strcat(buf, "HYD_HV070WS1\n");
#elif defined(CONFIG_MACH_ROY)
	strcat(buf, "SDC_LMS327DF02\n");
#else
	strcat(buf, "UNKNOWN\n");
#endif
	return strlen(buf);
}

static int mipi_hx8357_set_power(struct lcd_device *ldev, int power)
{
	DPRINT("%s[%d] (ignored)\n", __func__, power);
	return 0;
}

static int mipi_hx8357_get_power(struct lcd_device *dev)
{
	DPRINT("%s\n", __func__);
	/* other power states not supported */
	return (DISP_STATE_ON == disp_powered_up)
	    ? FB_BLANK_UNBLANK : FB_BLANK_POWERDOWN;
}

static struct lcd_ops mipi_lcd_props = {
	.get_power = mipi_hx8357_get_power,
	.set_power = mipi_hx8357_set_power,
};

static DEVICE_ATTR(lcd_type, S_IRUGO, mipi_hx8357_lcdtype_show, NULL);


static int parse_text(char *src, int len)
{
	int i, count, ret;
	int index = 0;
	char *str_line[120];
	char *sstart;
	char *c;
	unsigned int data2;
	c = src;
	count = 0;
	sstart = c;
	for (i = 0; i < len; i++, c++) {
		char a = *c;
		if (a == '\r' || a == '\n') {
			if (c > sstart) {
				str_line[count] = sstart;
				count++;
			}
			*c = '\0';
			sstart = c+1;
		}
	}
	if (c > sstart) {
		str_line[count] = sstart;
		count++;
	}
	printk(KERN_INFO "Total number of lines:%d\n", count);
	hx8357_mdnie_data[index++] = 0xCD;    /* set mDNIe command */
	for (i = 0; i < count; i++) {
		printk(KERN_INFO "line:%d, [start]%s[end]\n", i, str_line[i]);

		ret = sscanf(str_line[i], "0x%x\n", &data2);
		printk(KERN_INFO "Result => [0x%2x] %s\n", data2, (ret == 1) ? "Ok" : "Not available");
		if (ret == 1) {
			hx8357_mdnie_data[index++] = (unsigned char)data2;
		}
	}
	return index;
}

int mdnie_txtbuf_to_parsing(char const *pFilepath)
{
	struct file *filp;
	char	*dp;
	long	l;
	loff_t  pos;
	int     ret, num;
	mm_segment_t fs;

	fs = get_fs();
	set_fs(get_ds());

	printk(KERN_INFO "%s:", pFilepath);

	if (!pFilepath) {
		printk(KERN_ERR "Error : mdnie_txtbuf_to_parsing has invalid filepath.\n");
		goto parse_err;
	}

	filp = filp_open(pFilepath, O_RDONLY, 0);

	if (IS_ERR(filp)) {
		printk(KERN_ERR "file open error:%d\n", (s32)filp);
		goto parse_err;
	}

	l = filp->f_path.dentry->d_inode->i_size;
	dp = kmalloc(l+10, GFP_KERNEL);		/* add cushion : origianl code is 'dp = kmalloc(l, GFP_KERNEL);' */
	if (dp == NULL) {
		printk(KERN_INFO "Out of Memory!\n");
		filp_close(filp, current->files);
		goto parse_err;
	}
	pos = 0;
	memset(dp, 0, l);
	ret = vfs_read(filp, (char __user *)dp, l, &pos);

	if (ret != l) {
		printk(KERN_INFO "Failed to read file (ret = %d)\n", ret);
		kfree(dp);
		filp_close(filp, current->files);
		goto parse_err;
	}

	filp_close(filp, current->files);
	set_fs(fs);
	num = parse_text(dp, l);
	if (!num) {
		printk(KERN_ERR "Nothing to parse!\n");
		kfree(dp);
		goto parse_err;
	}

	mipi_dsi_cmds_tx(&hx8357_tx_buf, hx8357_display_mdnie_cmds,
			 ARRAY_SIZE(hx8357_display_mdnie_cmds));

	kfree(dp);
	DPRINT(" %s : mDNIe tunning done \n", __func__);
	num = num / 2;
	return num;

parse_err:
	return -EPERM;
}

ssize_t panel_tuning_store (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	static tuning = false;
	if (!strncmp(buf, "0", 1)) {
		tuning = false;
		DPRINT("%s :: tuning is disabled.\n", __func__);
	} else if (!strncmp(buf, "1", 1)) {
		tuning = true;
		DPRINT("%s :: tuning is enabled.\n", __func__);
	} else {
		if (!tuning) {
			DPRINT("%s :: Enable tuning first.\n", __func__);
			return count;
		}
		memset(tuning_file_name, 0, sizeof(tuning_file_name));
		strcpy(tuning_file_name, "/sdcard/mdnie/");
		strncat(tuning_file_name, buf, count-1);
		mdnie_txtbuf_to_parsing(tuning_file_name);
	}
	return count;
}

static struct device_attribute panel_tuning = {
			.attr = {		.name = "tuning",
						.mode = 0664},
						.show = NULL,
						.store = panel_tuning_store
			};

#ifdef HX8357_MDNIE_ENABLE

static ssize_t scenario_show ( struct device *dev, struct device_attribute *attr, char *buf )
{
	int mdnie_ui = 0;

	printk ( "called %s\n", __func__ );

	switch (current_mDNIe_Mode) {
		case mDNIe_UI_MODE:
			mdnie_ui = mDNIe_UI_MODE;
			break;
		case mDNIe_VIDEO_MODE:
			mdnie_ui = mDNIe_VIDEO_MODE;
			break;
		case mDNIe_VIDEO_WARM_MODE:
			mdnie_ui = mDNIe_VIDEO_WARM_MODE;
			break;
		case mDNIe_VIDEO_COLD_MODE:
			mdnie_ui = mDNIe_VIDEO_COLD_MODE;
			break;
		case mDNIe_CAMERA_MODE:
			mdnie_ui = mDNIe_CAMERA_MODE;
			break;
		case mDNIe_GALLERY_MODE:
			mdnie_ui = mDNIe_GALLERY_MODE;
			break;
		default:
			mdnie_ui = mDNIe_UI_MODE;
			break;
	}

	return sprintf(buf, "%u\n", mdnie_ui);
}
static ssize_t scenario_store ( struct device *dev, struct device_attribute *attr, const char *buf, size_t size )
{
	int value;


	if(disp_powered_up == DISP_STATE_OFF)
		return;
	
	sscanf(buf, "%d", &value);

	switch (value) {
	case mDNIe_UI_MODE:
		current_mDNIe_Mode = mDNIe_UI_MODE;
		if( current_Negative_Mode != mDNIe_NEGATIVE_ON )
		{
			mipi_dsi_cmds_tx(&hx8357_tx_buf, hx8357_mdnie_ui_cmds, ARRAY_SIZE(hx8357_mdnie_ui_cmds));
		}
		break;
	case mDNIe_VIDEO_MODE:
		current_mDNIe_Mode = mDNIe_VIDEO_MODE;
		if( current_Negative_Mode != mDNIe_NEGATIVE_ON )
		{
			if( current_Outdoor_Mode == mDNIe_OUTDOOR_ON )
			{
				mipi_dsi_cmds_tx(&hx8357_tx_buf, hx8357_mdnie_video_outdoor_cmds, ARRAY_SIZE(hx8357_mdnie_video_outdoor_cmds));
			}
			else
			{
				mipi_dsi_cmds_tx(&hx8357_tx_buf, hx8357_mdnie_video_cmds, ARRAY_SIZE(hx8357_mdnie_video_cmds));
			}
		}
		break;
	case mDNIe_VIDEO_WARM_MODE:
		current_mDNIe_Mode = mDNIe_VIDEO_WARM_MODE;
		if( current_Negative_Mode != mDNIe_NEGATIVE_ON )
		{
			if( current_Outdoor_Mode == mDNIe_OUTDOOR_ON )
			{
				mipi_dsi_cmds_tx(&hx8357_tx_buf, hx8357_mdnie_video_warm_outdoor_cmds, ARRAY_SIZE(hx8357_mdnie_video_warm_outdoor_cmds));
			}
			else
			{
				mipi_dsi_cmds_tx(&hx8357_tx_buf, hx8357_mdnie_video_warm_cmds, ARRAY_SIZE(hx8357_mdnie_video_warm_cmds));
			}
		}
		break;
	case mDNIe_VIDEO_COLD_MODE:
		current_mDNIe_Mode = mDNIe_VIDEO_COLD_MODE;
		if( current_Negative_Mode != mDNIe_NEGATIVE_ON )
		{
			if( current_Outdoor_Mode == mDNIe_OUTDOOR_ON )
			{
				mipi_dsi_cmds_tx(&hx8357_tx_buf, hx8357_mdnie_video_cold_outdoor_cmds, ARRAY_SIZE(hx8357_mdnie_video_cold_outdoor_cmds));
			}
			else
			{
				mipi_dsi_cmds_tx(&hx8357_tx_buf, hx8357_mdnie_video_cold_cmds, ARRAY_SIZE(hx8357_mdnie_video_cold_cmds));
			}
		}
		break;
	case mDNIe_CAMERA_MODE:
		current_mDNIe_Mode = mDNIe_CAMERA_MODE;
		if( current_Negative_Mode != mDNIe_NEGATIVE_ON )
		{
			mipi_dsi_cmds_tx(&hx8357_tx_buf, hx8357_mdnie_camera_cmds, ARRAY_SIZE(hx8357_mdnie_camera_cmds));
		}
		break;
	case mDNIe_GALLERY_MODE:
		current_mDNIe_Mode = mDNIe_GALLERY_MODE;
		if( current_Negative_Mode != mDNIe_NEGATIVE_ON )
		{
			mipi_dsi_cmds_tx(&hx8357_tx_buf, hx8357_mdnie_gallery_cmds, ARRAY_SIZE(hx8357_mdnie_gallery_cmds));
		}
		break;
	default:
		printk ( "scenario_store value is wrong : value(%d)\n", value );
		break;
	}

	return size;
}
static DEVICE_ATTR ( scenario, 0664, scenario_show, scenario_store );

static ssize_t outdoor_show ( struct device *dev, struct device_attribute *attr, char *buf )
{
	printk ( "called %s\n", __func__ );
	return sprintf(buf, "%u\n", current_Outdoor_Mode );
}
static ssize_t outdoor_store ( struct device *dev, struct device_attribute *attr, const char *buf, size_t size )
{
	int value;

	sscanf(buf, "%d", &value);

	printk ( "[mdnie set]inmdnieset_outdoor_file_cmd_store, input value = %d\n", value );

	switch (value) {
	case mDNIe_OUTDOOR_OFF:
		current_Outdoor_Mode = mDNIe_OUTDOOR_OFF;
		if( current_mDNIe_Mode == mDNIe_VIDEO_MODE )
		{
			mipi_dsi_cmds_tx(&hx8357_tx_buf, hx8357_mdnie_video_cmds, ARRAY_SIZE(hx8357_mdnie_video_cmds));
		}
		else if( current_mDNIe_Mode == mDNIe_VIDEO_WARM_MODE )
		{
			mipi_dsi_cmds_tx(&hx8357_tx_buf, hx8357_mdnie_video_warm_cmds, ARRAY_SIZE(hx8357_mdnie_video_warm_cmds));
		}
		else if( current_mDNIe_Mode == mDNIe_VIDEO_COLD_MODE )
		{
			mipi_dsi_cmds_tx(&hx8357_tx_buf, hx8357_mdnie_video_cold_cmds, ARRAY_SIZE(hx8357_mdnie_video_cold_cmds));
		}
		else
		{
			;
		}
		break;

	case mDNIe_OUTDOOR_ON:
		current_Outdoor_Mode = mDNIe_OUTDOOR_ON;
		if( current_mDNIe_Mode == mDNIe_VIDEO_MODE )
		{
			mipi_dsi_cmds_tx(&hx8357_tx_buf, hx8357_mdnie_video_outdoor_cmds, ARRAY_SIZE(hx8357_mdnie_video_outdoor_cmds));
		}
		else if( current_mDNIe_Mode == mDNIe_VIDEO_WARM_MODE )
		{
			mipi_dsi_cmds_tx(&hx8357_tx_buf, hx8357_mdnie_video_warm_outdoor_cmds, ARRAY_SIZE(hx8357_mdnie_video_warm_outdoor_cmds));
		}
		else if( current_mDNIe_Mode == mDNIe_VIDEO_COLD_MODE )
		{
			mipi_dsi_cmds_tx(&hx8357_tx_buf, hx8357_mdnie_video_cold_outdoor_cmds, ARRAY_SIZE(hx8357_mdnie_video_cold_outdoor_cmds));
		}
		else
		{
			;
		}
		break;

	default:
		printk ( "outdoor_store value is wrong : value(%d)\n", value );
		break;
	}


	return size;
}
static DEVICE_ATTR ( outdoor, 0664, outdoor_show, outdoor_store );

static ssize_t negative_show ( struct device *dev, struct device_attribute *attr, char *buf )
{
	printk ( "called %s\n", __func__ );
	return sprintf(buf, "%u\n", current_Negative_Mode );
}
static ssize_t negative_store ( struct device *dev, struct device_attribute *attr, const char *buf, size_t size )
{
	int value;

	sscanf(buf, "%d", &value);

	printk ( "[mdnie set]negative_store, input value = %d\n", value );

	switch (value) {
	case mDNIe_NEGATIVE_OFF:
		current_Negative_Mode = mDNIe_NEGATIVE_OFF;
		mipi_dsi_cmds_tx(&hx8357_tx_buf, hx8357_mdnie_ui_cmds, ARRAY_SIZE(hx8357_mdnie_ui_cmds)); // correct?
		break;

	case mDNIe_NEGATIVE_ON:
		current_Negative_Mode = mDNIe_NEGATIVE_ON;
		mipi_dsi_cmds_tx(&hx8357_tx_buf, hx8357_mdnie_negative_cmds, ARRAY_SIZE(hx8357_mdnie_negative_cmds));
		break;

	default:
		printk ( "negative_store value is wrong : value(%d)\n", value );
		break;
	}

	return size;
}
static DEVICE_ATTR ( negative, 0664, negative_show, negative_store );

#endif

static int __devinit mipi_hx8357_lcd_probe(struct platform_device *pdev)
{
	struct platform_device *msm_fb_pdev;
	struct lcd_device *lcd_device;
	int ret;

	DPRINT("%s\n", __func__);

	if (pdev->id == 0) {
		mipi_hx8357_pdata = pdev->dev.platform_data;
		return 0;
	}

	/*
	 * save returned struct platform_device pointer
	 * as we later need to get msm_fb_data_type
	 */
	msm_fb_pdev = msm_fb_add_device(pdev);

	/* struct lcd_device now has needed platform data */
	lcd_device = lcd_device_register("panel", &pdev->dev,
					 platform_get_drvdata(msm_fb_pdev),
					 &mipi_lcd_props);

	if (IS_ERR(lcd_device)) {
		ret = PTR_ERR(lcd_device);
		printk(KERN_ERR "lcd : failed to register device\n");
		return ret;
	}

	ret = sysfs_create_file(&lcd_device->dev.kobj, &dev_attr_lcd_type.attr);
	if (ret)
		printk(KERN_ERR "sysfs create fail - %s\n",
		       dev_attr_lcd_type.attr.name);

	ret = sysfs_create_file(&lcd_device->dev.kobj, &panel_tuning.attr);
	if (ret)
		printk(KERN_ERR "panel_tuning sysfs create fail - %s\n",
		       panel_tuning.attr.name);

#ifdef HX8357_MDNIE_ENABLE

	mdnie_class = class_create ( THIS_MODULE, "mdnie" );
	if( IS_ERR ( mdnie_class ) )
		printk ( KERN_ERR "Failed to create class(mdnie)!\n" );

	tune_mdnie_dev = device_create(mdnie_class, NULL, 0, NULL, "mdnie");
	if( IS_ERR ( tune_mdnie_dev ) )
		printk ( KERN_ERR "Failed to create device(mdnie)!\n" );

	if( device_create_file ( tune_mdnie_dev, &dev_attr_scenario ) < 0 )
		printk ( KERN_ERR "Failed to create device file(%s)!\n", dev_attr_scenario.attr.name );

	if( device_create_file ( tune_mdnie_dev, &dev_attr_outdoor) < 0 )
		printk ( KERN_ERR "Failed to create device file(%s)!\n", dev_attr_outdoor.attr.name );

	if( device_create_file ( tune_mdnie_dev, &dev_attr_negative) < 0 )
		printk ( KERN_ERR "Failed to create device file(%s)!\n", dev_attr_negative.attr.name );

#endif

	return 0;
}

static struct platform_driver this_driver = {
	.probe = mipi_hx8357_lcd_probe,
	.driver = {
		   .name = "mipi_HX8357",
		   },
};

static void mipi_hx8357_set_backlight(struct msm_fb_data_type *mfd)
{
	int level = mfd->bl_level;

	/* function will spin lock */
	backlight_ic_set_brightness(level);
}

static struct msm_fb_panel_data hx8357_panel_data = {
	.on = mipi_hx8357_lcd_on,
	.off = mipi_hx8357_lcd_off,
	.set_backlight = mipi_hx8357_set_backlight,
};

static int ch_used[3];

static int mipi_hx8357_lcd_init(void)
{
	DPRINT("start %s\n", __func__);
#if defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
	lcd_reset=22;
#else
	lcd_reset = 23;
#endif

	mipi_dsi_buf_alloc(&hx8357_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&hx8357_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}

int mipi_hx8357_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel,
					struct dsi_cmd_desc *panel_prepare,
					int panel_prepare_length)
{
	struct platform_device *pdev = NULL;
	int ret;

	DPRINT("start %s\n", __func__);
	concrete_hx8357_prepare_cmds = panel_prepare;
	concrete_hx8357_prepare_cmds_len = panel_prepare_length;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	ret = mipi_hx8357_lcd_init();
	if (ret) {
		DPRINT("mipi_HX8357_lcd_init() failed with ret %u\n", ret);
		return ret;
	}

	pdev = platform_device_alloc("mipi_HX8357", (panel << 8) | channel);
	if (!pdev)
		return -ENOMEM;

	hx8357_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &hx8357_panel_data,
				       sizeof(hx8357_panel_data));
	if (ret) {
		pr_debug("%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		pr_debug("%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

 err_device_put:
	platform_device_put(pdev);
	return ret;
}
