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

#include <linux/lcd.h>

#include <mach/gpio.h>
#include <mach/gpio_aruba.h>
#include <linux/mutex.h>

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_HX8369B.h"

#include "lcdc_backlight_ic.h"


//#define CONFIG_PANEL_ESD_DETECT


#if defined(CONFIG_PANEL_ESD_DETECT)
#include <linux/irq.h>
#endif


static struct msm_panel_common_pdata *mipi_hx8369b_pdata;
static struct dsi_buf hx8369b_tx_buf;
static struct dsi_buf hx8369b_rx_buf;
static int is_lcd_on;
static struct mutex lcdc_mlock;

u32 hx8369b_read_id;

#define LCDC_DEBUG

#ifdef LCDC_DEBUG
#define DPRINT(x...)	printk(KERN_INFO "[LCD] " x)
#else
#define DPRINT(x...)
#endif

/* in msec */
#define HX8369B_SLEEP_IN_DELAY 	200
#define HX8369B_SLEEP_OUT_DELAY	200
#define HX8369B_DISP_ON_DELAY	100

#define HX8369B_CMD_SETTLE 0

#if defined(CONFIG_MACH_BAFFIN_DUOS_CTC)
#define GPIO_PWM_CTRL	119
#endif

//#define CONFIG_MDNIE_TUNING
#define CONFIG_APPLY_MDNIE
#if defined(CONFIG_MDNIE_TUNING)
#define TUNING_FILE_PATH "/sdcard/mdnie/"
static int tuning_enable;
unsigned char mDNIe_data[500];
static char tuning_filename[100];
#endif
#if defined(CONFIG_APPLY_MDNIE)
struct class *mdnie_class;
struct mdnie_config mDNIe_cfg;
#endif

#if defined(CONFIG_PANEL_ESD_DETECT)
struct lcd_det_data_struct {	
	int    		irq;	
	struct 		work_struct work_lcd_det;
	/* for lcd_det */
};

struct lcd_det_data_struct lcd_det_data;
struct workqueue_struct *lcd_det_wq;
static boolean is_ESD_status = false;
#endif


/* common setting */
static char exit_sleep[] = {0x11, 0x00};
static char display_on[] = {0x29, 0x00};
static char display_off[] = {0x28, 0x00};
static char enter_sleep[] = {0x10, 0x00};

static struct dsi_cmd_desc hx8369b_disp_off_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_SLEEP_IN_DELAY, sizeof(display_off), display_off},
};

static struct dsi_cmd_desc hx8369b_sleep_in_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(enter_sleep), enter_sleep}
};


// 12.11.17 kiyong17.kim Apply the LCD Init code
#if defined(CONFIG_MACH_BAFFIN_DUOS_CTC)

static char BAFFIN_video1[] = {
	0xB9,
	0xFF, 0x83, 0x69
};
static char BAFFIN_video2[] = {
	0x3A,
	0x77
};
static char BAFFIN_video3[] = {
	0xBA,
        0x11, 0x00, 0x16, 0xC6, 0x80, 0x0A, 0x00, 0x10, 0x24, 0x02, 0x21, 0x21, 0x9A, 0x11, 0x14
};

static char BAFFIN_video4[] = {
	0xD5,
        0x00, 0x00, 0x09, 0x03, 0x2D, 0x00, 0x00, 0x12, 0x31, 0x23, 0x00, 0x00, 0x10, 0x70, 0x37,
        0x00, 0x00, 0x0D, 0x01, 0x40, 0x37, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x03,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0xEF, 0x00, 0x13, 0x57, 0x71, 0x00, 0x00, 0x00, 0xEF, 
        0xEF, 0x00, 0x64, 0x20, 0x06, 0x00, 0x00, 0x00, 0xEF, 0xEF, 0x00, 0x02, 0x46, 0x60, 
        0x00, 0x00, 0x00, 0xEF, 0xEF, 0x00, 0x75, 0x31, 0x17, 0x00, 0x00, 0x00, 0xEF, 0x00, 0x00, 
        0x00, 0x03, 0x00, 0x00, 0x00, 0X0F, 0xFC, 0x0C, 0xFC, 0xFF, 0x0F, 0xFC, 0x0C, 0xEC, 0xFF, 
        0x00, 0x00,0x5A
};
static char BAFFIN_video5[] = {
	0xB1,
        0x0A, 0x83, 0x77, 0x00, 0x8F, 0x0F, 0x1C, 0x1C, 0x0C, 0x2A, 0x20, 0xCE
};
static char BAFFIN_video6[] = {
	0xB3,
        0x03, 0x00, 0x30, 0x0B
};

static char BAFFIN_video7[] = {
		0xB4,
        0x02
};
static char BAFFIN_video_backscreen[] = {
	// Display the Black Screen ( 7 frame )
	0xB2,
	0x00, 0x70
};
static char BAFFIN_video8[] = {
	0xB6,
			0xB1, 0xAE, 0x00
};
static char BAFFIN_video9[] = {
	0xBF,
        0x5F, 0x00, 0x00, 0x06
};
static char BAFFIN_video10[] = {
	0xCC,
	0x02
};
static char BAFFIN_video11[] = {
	0xC1,
			0x00
};
static char BAFFIN_video12[] = {
	0xC0,
        0x73, 0x50, 0x00, 0x2C, 0xC4, 0x00
};
static char BAFFIN_video13[] = {
	0xE3,
        0x03, 0x03, 0x03, 0x03
};
static char BAFFIN_video14[] = {
	0xEA,
        0x74
};
static char BAFFIN_video15[] = {
	0xC6,
        0x41, 0xFF, 0x7D
};
static char BAFFIN_video16[] = {
	0xE0,
	0x18, 0x21, 0x27, 0x35, 0x3A, 0x3F, 0x32, 0x4C, 0x09, 0x0E,
	0x0E, 0x12, 0x13, 0x11, 0x13, 0x18, 0x1D, 0x18, 0x21, 0x27,
	0x35, 0x3A, 0x3F, 0x32, 0x4C, 0x09, 0x0E, 0x0E, 0x12, 0x13, 
	0x11, 0x13, 0x18, 0x1D, 0x01
};
static char BAFFIN_video17[] = {
	0xC9,
        0x0F, 0x00
};


static struct dsi_cmd_desc BAFFIN_hx8369b_video_display_init_rev1_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(BAFFIN_video1), BAFFIN_video1},
};

static struct dsi_cmd_desc BAFFIN_hx8369b_video_display_init_rev1_2_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(BAFFIN_video3), BAFFIN_video3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(BAFFIN_video4), BAFFIN_video4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(BAFFIN_video5), BAFFIN_video5},
};

static struct dsi_cmd_desc BAFFIN_hx8369b_video_display_init_rev1_3_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(exit_sleep), exit_sleep},
};

static struct dsi_cmd_desc BAFFIN_hx8369b_video_display_init_rev1_4_cmds[] = {

	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(BAFFIN_video6), BAFFIN_video6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(BAFFIN_video7), BAFFIN_video7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(BAFFIN_video8), BAFFIN_video8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(BAFFIN_video9), BAFFIN_video9},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(BAFFIN_video10), BAFFIN_video10},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(BAFFIN_video11), BAFFIN_video11},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(BAFFIN_video12), BAFFIN_video12},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(BAFFIN_video13), BAFFIN_video13},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(BAFFIN_video14), BAFFIN_video14},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(BAFFIN_video15), BAFFIN_video15},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(BAFFIN_video16), BAFFIN_video16},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(BAFFIN_video17), BAFFIN_video17},
};
#endif


static char hx8369b_auo1[] = {
	0xB9,
	0xFF, 0x83, 0x69
};
static char hx8369b_auo2[] = {
	0x3A,
	0x77
};
static char hx8369b_auo3[] = {
	0xD5,
	0x00, 0x00, 0x13, 0x03, 0x35, 0x00, 0x01, 0x10, 0x01, 0x00, 0x00, 0x00, 0x01, 0x7A, 0x16,
	0x04, 0x04, 0x13, 0x07, 0x40, 0x13, 0x00, 0x00, 0x01, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x40, 0x00, 0x88, 0x88, 0x54, 0x20, 0x00, 
	0x00, 0x00, 0x10, 0x00, 0x88, 0x88, 0x67, 0x13, 0x50, 0x00, 0x00, 0x50, 0x00, 0x88, 0x88, 
	0x76, 0x31, 0x10, 0x00, 0x00, 0x00, 0x00, 0x88, 0x88, 0x45, 0x02, 0x40, 0x00, 0x00, 0x00, 
	0x51, 0x00, 0x00, 0x00, 0x0A, 0X00, 0xEF, 0x00, 0xEF, 0x0A, 0x00, 0xEF, 0x00, 0xEF, 0x00, 
	0x01, 0x5A
};
static char hx8369b_auo3_1[] = {
	0xD5,	
	0x00, 0x00, 0x13, 0x03, 0x35, 0x00, 0x01, 0x10, 0x01, 0x00, 0x00, 0x00, 0x01, 0x7A, 0x16, 
	0x04, 0x04, 0x13, 0x07, 0x40, 0x13, 0x00, 0x00, 0x00, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x48, 0x88, 0x85, 0x42, 0x00, 0x99, 0x99, 
	0x00, 0x00, 0x18, 0x88, 0x86, 0x71, 0x35, 0x99, 0x99, 0x00, 0x00, 0x58, 0x88, 0x87, 0x63, 
	0x11, 0x99, 0x99, 0x00, 0x00, 0x08, 0x88, 0x84, 0x50, 0x24, 0x99, 0x99, 0x00, 0x00, 0x00, 
	0x51, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x00, 0x0F, 0x00, 0x0F, 0x00, 
	0x01, 0x5A
};
static char hx8369b_auo4[] = {
	0xBA,
	0x31, 0x00, 0x16, 0xCA, 0xB1, 0x0A, 0x00, 0x10, 0x28, 0x02, 0x21, 0x21, 0x9A, 0x1A, 0x8F
};
static char hx8369b_auo5[] = {
	0xB1,
	0x09, 0x83, 0x67, 0x00, 0x92, 0x12, 0x16, 0x16, 0x0C, 0x02
};
static char hx8369b_auo6[] = {
	0xB2,
	0x00, 0x70
};
static char hx8369b_auo7[] = {
	0xE0,
	0x00, 0x05, 0x0B, 0x2F, 0x2F, 0x30, 0x1B, 0x3E, 0x07, 0x0D, 0x0E, 0x12, 0x13, 0x12, 0x14,
	0x13, 0x1A, 0x00, 0x05, 0x0B, 0x2F, 0x2F, 0x30, 0x1B, 0x3E, 0x07, 0x0D, 0x0E, 0x12, 0x13,
	0x12, 0x14, 0x13, 0x1A, 0x01
};
static char hx8369b_auo7_1[] = {
	0xE0,
	0x00, 0x05, 0x0B, 0x2F, 0x2F, 0x30, 0x1B, 0x3D, 0x07, 0x0D, 0x0E, 0x12, 0x13, 0x12, 0x13, 
	0x11, 0x1A, 0x00, 0x05, 0x0B, 0x2F, 0x2F, 0x30, 0x1B, 0x3D, 0x07, 0x0D, 0x0E, 0x12, 0x13, 
	0x12, 0x13, 0x11, 0x1A, 0x01
};
static char hx8369b_auo8[] = {
	0xC1,
	0x03, 0x00, 0x09, 0x11, 0x18, 0x1E, 0x27, 0x2F, 0x36, 0x3E, 0x45, 0x4C, 0x54, 0x5C, 0x64, 
	0x6B, 0x73, 0x7C, 0x83, 0x8B, 0x94, 0x9C, 0xA4, 0xAC, 0xB4, 0xBC, 0xC4, 0xCD, 0xD5, 0xDD, 
	0xE6, 0xEE, 0xF7, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 
	0x11, 0x18, 0x1E, 0x27, 0x2F, 0x36, 0x3E, 0x45, 0x4C, 0x54, 0x5C, 0x64, 0x6B, 0x73, 0x7C, 
	0x83, 0x8B, 0x94, 0x9C, 0xA4, 0xAC, 0xB4, 0xBC, 0xC4, 0xCD, 0xD5, 0xDD, 0xE6, 0xEE, 0xF7, 
	0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x11, 0x18, 0x1E, 
	0x27, 0x2F, 0x36, 0x3E, 0x45, 0x4C, 0x54, 0x5C, 0x64, 0x6B, 0x73, 0x7C, 0x83, 0x8B, 0x94, 
	0x9C, 0xA4, 0xAC, 0xB4, 0xBC, 0xC4, 0xCD, 0xD5, 0xDD, 0xE6, 0xEE, 0xF7, 0xFF, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static char hx8369b_auo9[] = {
	0xB3,
	0x83, 0x00, 0x31, 0x03, 0x01, 0x15, 0x14
};
static char hx8369b_auo10[] = {
	0xB4,
	0x02
};
static char hx8369b_auo11[] = {
	0xB5,
	0x0B, 0x0B, 0x24
};
static char hx8369b_auo12[] = {
	0xCC,
	0x02
};
static char hx8369b_auo13[] = {
	0xC6,
	0x41, 0xFF, 0x7A
};
static char hx8369b_auo14[] = {
	0xC0,
	0x73, 0x50, 0x00, 0x34, 0xC4, 0x02
};
static char hx8369b_auo15[] = {
	0xE3,
	0x00, 0x00, 0x13, 0x1B
};
static char hx8369b_auo16[] = {
	0xCB,
	0x6D
};
static char hx8369b_auo17[] = {
	0xEA,
	0x62
};

static char config_video_MADCTL[2] = {0x36, 0xC0};

static struct dsi_cmd_desc hx8369b_video_display_init_auo1_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo1), hx8369b_auo1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo2), hx8369b_auo2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo3), hx8369b_auo3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo4), hx8369b_auo4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo5), hx8369b_auo5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo6), hx8369b_auo6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo7), hx8369b_auo7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo8), hx8369b_auo8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo9), hx8369b_auo9},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo10), hx8369b_auo10},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo11), hx8369b_auo11},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo12), hx8369b_auo12},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo13), hx8369b_auo13},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo14), hx8369b_auo14},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo15), hx8369b_auo15},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo16), hx8369b_auo16},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo17), hx8369b_auo17},
};

static struct dsi_cmd_desc hx8369b_video_display_init_auo2_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo1), hx8369b_auo1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo2), hx8369b_auo2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo3_1), hx8369b_auo3_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo4), hx8369b_auo4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo5), hx8369b_auo5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo6), hx8369b_auo6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo7_1), hx8369b_auo7_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo8), hx8369b_auo8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo9), hx8369b_auo9},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo10), hx8369b_auo10},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo11), hx8369b_auo11},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo12), hx8369b_auo12},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo13), hx8369b_auo13},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo14), hx8369b_auo14},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo15), hx8369b_auo15},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo16), hx8369b_auo16},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_auo17), hx8369b_auo17},
};


static char hx8369b_boe1[] = {
	0xB9,
	0xFF, 0x83, 0x69
};
static char hx8369b_boe2[] = {
	0xBA,
	0x31, 0x00, 0x16, 0xCA, 0xB1, 0x0A, 0x00, 0x10, 0x28, 0x02, 0x21, 0x21, 0x9A, 0x1A, 0x8F
};
static char hx8369b_boe3[] = {
	0x3A,
	0x70
};
static char hx8369b_boe4[] = {
	0xD5,
	0x00, 0x00, 0x08, 0x00, 0x0A, 0x00, 0x00, 0x10, 0x01, 0x00, 0x00, 0x00, 0x01, 0x49, 0x37,
	0x00, 0x00, 0x0A, 0x0A, 0x0B, 0x47, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x03, 0x00, 0x00, 0x10, 0x00, 0x00, 0x91, 0x13, 0x35, 0x57, 0x75, 0x18, 0x00, 
	0x00, 0x00, 0x86, 0x64, 0x42, 0x20, 0x00, 0x49, 0x00, 0x00, 0x00, 0x90, 0x02, 0x24, 0x46, 
	0x64, 0x08, 0x00, 0x00, 0x00, 0x87, 0x75, 0x53, 0x31, 0x11, 0x59, 0x00, 0x00, 0x00, 0x00, 
	0x01, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x0F, 0xFF, 0xFF, 0x0F, 0x00, 0x0F, 0xFF, 0xFF, 0x00, 
	0x80, 0x5A
};
#if defined(CONFIG_MACH_ARUBASLIM_OPEN)
static char hx8369b_boe5[] = {
	0xB1,
	0x0C, 0x83, 0x77, 0x00, 0x0F, 0x0F, 0x18, 0x18, 0x0C, 0x12
};
#else
static char hx8369b_boe5[] = {
	0xB1,
	0x0C, 0x83, 0x77, 0x00, 0x0F, 0x0F, 0x18, 0x18, 0x0C, 0x02
};
#endif
static char hx8369b_boe6[] = {
	0xB2,
	0x00, 0x70
};
static char hx8369b_boe7[] = {
	0xB3,
	0x83, 0x00, 0x31, 0x03
};
static char hx8369b_boe8[] = {
	0xB4,
	0x02
};
static char hx8369b_boe9[] = {
	0xB6,
	0xA0, 0xA0
};
static char hx8369b_boe10[] = {
	0xCC,
	0x02
};
static char hx8369b_boe11[] = {
	0xC6,
	0x41, 0xFF, 0x7A
};
static char hx8369b_boe12[] = {
	0xEA,
	0x72
};
static char hx8369b_boe13[] = {
	0xE3,
	0x07, 0x0F, 0x07, 0x0F
};
static char hx8369b_boe14[] = {
	0xC0,
	0x73, 0x50, 0x00, 0x34, 0xC4, 0x09
};
static char hx8369b_boe15[] = {
	0xE0,
	0x00, 0x07, 0x0C, 0x30, 0x32, 0x3F, 0x1C, 0x3A, 0x08, 0x0D, 0x10, 0x14, 0x16, 0x14, 0x15, 
	0x0E, 0x12, 0x00, 0x07, 0x0C, 0x30, 0x32, 0x3F, 0x1C, 0x3A, 0x08, 0x0D, 0x10, 0x14, 0x16, 
	0x14, 0x15, 0x0E, 0x12, 0x01
};
static char hx8369b_boe16[] = {
	0xC1,
	0x00
};

static struct dsi_cmd_desc hx8369b_video_display_init_boe_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_boe1), hx8369b_boe1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_boe2), hx8369b_boe2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_boe3), hx8369b_boe3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_boe4), hx8369b_boe4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_boe5), hx8369b_boe5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_boe6), hx8369b_boe6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_boe7), hx8369b_boe7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_boe8), hx8369b_boe8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_boe9), hx8369b_boe9},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_boe10), hx8369b_boe10},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_boe11), hx8369b_boe11},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_boe12), hx8369b_boe12},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_boe13), hx8369b_boe13},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_boe14), hx8369b_boe14},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_boe15), hx8369b_boe15},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_boe16), hx8369b_boe16},
};


// Delos
static char hx8369b_55BC90_video1[] = {
	0xB9,
	0xFF, 0x83, 0x69
};
static char hx8369b_55BC90_video2[] = {
	0xBA,
	0x31, 0x00, 0x16, 0xCA, 0xB0, 0x0A, 0x00, 0x10, 0x28, 0x02, 0x21, 0x21, 0x9A, 0x1A, 0x8F
};
static char hx8369b_55BC90_video3[] = {
	0x3A,
	0x70
};
static char hx8369b_55BC90_video4[] = {
	0xD5,
	0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x00, 0x18, 0x01, 0x00, 0x00, 0x00, 0x01, 0x60, 0x37,	
	0x00, 0x00, 0x03, 0x07, 0x08, 0x47, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x03, 0x00, 0x00, 0x10, 0x00, 0x00, 0x91, 0x13, 0x35, 0x57, 0x75, 0x18, 0x00,	
	0x00, 0x00, 0x86, 0x64, 0x42, 0x20, 0x00, 0x49, 0x00, 0x00, 0x00, 0x90, 0x02, 0x24, 0x46,	
	0x64, 0x08, 0x00, 0x00, 0x00, 0x87, 0x75, 0x53, 0x31, 0x11, 0x59, 0x00, 0x00, 0x00, 0x00,	
	0x01, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x0F, 0xFF, 0xFF, 0x0F, 0x00, 0x0F, 0xFF, 0xFF, 0x00,	
	0x85, 0x5A
};
static char hx8369b_55BC90_video5[] = {
	0xB1,
	0x0B, 0x83, 0x77, 0x00, 0x0F, 0x0F, 0x17, 0x17, 0x0C, 0x1A
};
static char hx8369b_55BC90_video6[] = {
	0xB3,
	0x83, 0x00, 0x31, 0x03
};
static char hx8369b_55BC90_video7[] = {
	0xB4,
	0x00
};
static char hx8369b_55BC90_video8[] = {
	0xCB,
	0x6D
};
static char hx8369b_55BC90_video9[] = {
	0xCC,
	0x0E
};
static char hx8369b_55BC90_video10[] = {
	0xC1,
	0x00
};
static char hx8369b_55BC90_video11[] = {
	0xC6,
	0x41, 0xFF, 0x7A, 0xFF
};
static char hx8369b_55BC90_video12[] = {
	0xEA,
	0x72
};
static char hx8369b_55BC90_video13[] = {
	0xE3,
	0x07, 0x0F, 0x07, 0x0F
};
static char hx8369b_55BC90_video14[] = {
	0xC0,
	0x73, 0x50, 0x00, 0x34, 0xC4, 0x09
};
static char hx8369b_55BC90_video15[] = {
	0xE0,
	0x01, 0x0E, 0x12, 0x29, 0x2D, 0x30, 0x1E, 0x3B, 0x08, 0x0D, 0x0F, 0x13, 0x15, 0x13, 0x14, 
	0x10, 0x16, 0x01, 0x0E, 0x11, 0x2A, 0x2D, 0x30, 0x1D, 0x3B, 0x08, 0x0D, 0x0F, 0x13, 0x15, 
	0x13, 0x14, 0x0F, 0x16, 0x01		
};

static struct dsi_cmd_desc hx8369b_video_display_init_55BC90_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_55BC90_video1), hx8369b_55BC90_video1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_55BC90_video2), hx8369b_55BC90_video2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_55BC90_video3), hx8369b_55BC90_video3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_55BC90_video4), hx8369b_55BC90_video4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_55BC90_video5), hx8369b_55BC90_video5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_55BC90_video6), hx8369b_55BC90_video6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_55BC90_video7), hx8369b_55BC90_video7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_55BC90_video8), hx8369b_55BC90_video8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_55BC90_video9), hx8369b_55BC90_video9},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_55BC90_video10), hx8369b_55BC90_video10},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_55BC90_video11), hx8369b_55BC90_video11},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_55BC90_video12), hx8369b_55BC90_video12},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_55BC90_video13), hx8369b_55BC90_video13},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 120, sizeof(hx8369b_55BC90_video14), hx8369b_55BC90_video14},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE, sizeof(hx8369b_55BC90_video15), hx8369b_55BC90_video15},
};


static struct dsi_cmd_desc hx8369b_video_display_on_cmds_rotate[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(config_video_MADCTL), config_video_MADCTL},
};

static struct dsi_cmd_desc hx8369b_video_display_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_SLEEP_OUT_DELAY,  sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_DISP_ON_DELAY, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc hx8369b_video_display_on_smd_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_DISP_ON_DELAY, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc hx8369b_video_display_on_auo2_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_DISP_ON_DELAY, sizeof(display_on), display_on},
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_SLEEP_OUT_DELAY,  sizeof(exit_sleep), exit_sleep},
};

#if defined(CONFIG_MDNIE_TUNING)
static struct dsi_cmd_desc hx8369b_video_display_mDNIe_tune_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE,  0, mDNIe_data},
};
#endif
#if defined(CONFIG_APPLY_MDNIE)
static char mDNIe_UI_MODE[] = {
	0xE6,
	0x5A, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00,
	0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00,
	0x02, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00,
	0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
	0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00,
	0x20, 0x00, 0x20, 0x00, 0x05, 0xc6, 0x1e, 0xd3, 0x1f, 0x67, 0x1f, 0xc6, 0x04, 0xd3, 0x1f,
	0x67, 0x1f, 0xc6, 0x1e, 0xd3, 0x05, 0x67
};
static char mDNIe_VIDEO_MODE[] = {
	0xE6,
	0x5A, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00,
	0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00,
	0x06, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00,
	0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
	0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00,
	0x20, 0x00, 0x20, 0x00, 0x05, 0x10, 0x1f, 0x4c, 0x1f, 0xa4, 0x1f, 0xdd, 0x04, 0x7f, 0x1f,
	0xa4, 0x1f, 0xdd, 0x1f, 0x4c, 0x04, 0xd7
};
static char mDNIe_VIDEO_WARM_MODE[] = {
	0xE6,
	0x5A, 0x00, 0x00, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe3, 0x00, 0xf3, 0x00, 0xff, 0xFF, 0x00, 0x00,
	0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00,
	0x06, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00,
	0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
	0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00,
	0x20, 0x00, 0x20, 0x00, 0x05, 0x10, 0x1f, 0x4c, 0x1f, 0xa4, 0x1f, 0xdd, 0x04, 0x7f, 0x1f,
	0xa4, 0x1f, 0xdd, 0x1f, 0x4c, 0x04, 0xd7
};
static char mDNIe_VIDEO_COLD_MODE[] = {
	0xE6,
	0x5A, 0x00, 0x00, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0xed, 0x00, 0xe1, 0xFF, 0x00, 0x00,
	0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00,
	0x06, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00,
	0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
	0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00,
	0x20, 0x00, 0x20, 0x00, 0x05, 0x10, 0x1f, 0x4c, 0x1f, 0xa4, 0x1f, 0xdd, 0x04, 0x7f, 0x1f,
	0xa4, 0x1f, 0xdd, 0x1f, 0x4c, 0x04, 0xd7
};
static char mDNIe_CAMERA_MODE[] = {
	0xE6,
	0x5A, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00,
	0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00,
	0x06, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00,
	0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
	0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00,
	0x20, 0x00, 0x20, 0x00, 0x05, 0x10, 0x1f, 0x4c, 0x1f, 0xa4, 0x1f, 0xdd, 0x04, 0x7f, 0x1f,
	0xa4, 0x1f, 0xdd, 0x1f, 0x4c, 0x04, 0xd7
};
static char mDNIe_NAVI_MODE[] = {
	0xE6,
	0x00
};
static char mDNIe_GALLERY_MODE[] = {
	0xE6,
	0x5A, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00,
	0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00,
	0x02, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00,
	0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
	0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 
	0x20, 0x00, 0x20, 0x00, 0x05, 0x10, 0x1f, 0x4c, 0x1f, 0xa4, 0x1f, 0xdd, 0x04, 0x7f, 0x1f, 
	0xa4, 0x1f, 0xdd, 0x1f, 0x4c, 0x04, 0xd7
};
static char mDNIe_VT_MODE[] = {
	0xE6,
	0x00
};
static char mDNIe_NEGATIVE_MODE[] = {
	0xE6,
	0x5A, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0x00, 0xff, 0xff,
	0x00, 0xff, 0x00, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0x00, 0xff,
	0x02, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00,
	0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
	0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00,
	0x20, 0x00, 0x20, 0x00, 0x05, 0xc6, 0x1e, 0xd3, 0x1f, 0x67, 0x1f, 0xc6, 0x04, 0xd3, 0x1f,
	0x67, 0x1f, 0xc6, 0x1e, 0xd3, 0x05, 0x67
};
static char mDNIe_VIDEO_OUTDOOR_MODE[] = {
	0xE6,
	0x5A, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00,
	0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00,
	0x07, 0xff, 0x00, 0x0a, 0xaf, 0x0d, 0x99, 0x14, 0x6d, 0x1b, 0x48, 0x2c, 0x05, 0xb4, 0x0f,
	0xbe, 0x26, 0xbe, 0x26, 0xbe, 0x26, 0xbe, 0x26, 0xae, 0x0c, 0xae, 0x0c, 0xae, 0x0c, 0xae,
	0x0c, 0xae, 0x0c, 0xae, 0x0c, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00,
	0x20, 0x00, 0x20, 0x00, 0x06, 0x7b, 0x1e, 0x5b, 0x1f, 0x2a, 0x1f, 0xae, 0x05, 0x28, 0x1f,
	0x2a, 0x1f, 0xae, 0x1e, 0x5b, 0x05, 0xf7
};
static char mDNIe_VIDEO_WARM_OUTDOOR_MODE[] = {
	0xE6,
	0x5A, 0x00, 0x00, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe3, 0x00, 0xf3, 0x00, 0xff, 0xFF, 0x00, 0x00,
	0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00,
	0x07, 0xff, 0x00, 0x0a, 0xaf, 0x0d, 0x99, 0x14, 0x6d, 0x1b, 0x48, 0x2c, 0x05, 0xb4, 0x0f,
	0xbe, 0x26, 0xbe, 0x26, 0xbe, 0x26, 0xbe, 0x26, 0xae, 0x0c, 0xae, 0x0c, 0xae, 0x0c, 0xae,
	0x0c, 0xae, 0x0c, 0xae, 0x0c, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00,
	0x20, 0x00, 0x20, 0x00, 0x06, 0x7b, 0x1e, 0x5b, 0x1f, 0x2a, 0x1f, 0xae, 0x05, 0x28, 0x1f,
	0x2a, 0x1f, 0xae, 0x1e, 0x5b, 0x05, 0xf7
};
static char mDNIe_VIDEO_COLD_OUTDOOR_MODE[] = {
	0xE6,
	0x5A, 0x00, 0x00, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0xed, 0x00, 0xe1, 0xFF, 0x00, 0x00,
	0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00,
	0x07, 0xff, 0x00, 0x0a, 0xaf, 0x0d, 0x99, 0x14, 0x6d, 0x1b, 0x48, 0x2c, 0x05, 0xb4, 0x0f,
	0xbe, 0x26, 0xbe, 0x26, 0xbe, 0x26, 0xbe, 0x26, 0xae, 0x0c, 0xae, 0x0c, 0xae, 0x0c, 0xae,
	0x0c, 0xae, 0x0c, 0xae, 0x0c, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00,
	0x20, 0x00, 0x20, 0x00, 0x06, 0x7b, 0x1e, 0x5b, 0x1f, 0x2a, 0x1f, 0xae, 0x05, 0x28, 0x1f,
	0x2a, 0x1f, 0xae, 0x1e, 0x5b, 0x05, 0xf7
};
static char mDNIe_CAMERA_OUTDOOR_MODE[] = {
	0xE6,
	0x5A, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00,
	0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00,
	0x07, 0xff, 0x00, 0x0a, 0xaf, 0x0d, 0x99, 0x14, 0x6d, 0x1b, 0x48, 0x2c, 0x05, 0xb4, 0x0f,
	0xbe, 0x26, 0xbe, 0x26, 0xbe, 0x26, 0xbe, 0x26, 0xae, 0x0c, 0xae, 0x0c, 0xae, 0x0c, 0xae,
	0x0c, 0xae, 0x0c, 0xae, 0x0c, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 
	0x20, 0x00, 0x20, 0x00, 0x06, 0x7b, 0x1e, 0x5b, 0x1f, 0x2a, 0x1f, 0xae, 0x05, 0x28, 0x1f,
	0x2a, 0x1f, 0xae, 0x1e, 0x5b, 0x05, 0xf7
};

static struct dsi_cmd_desc hx8369b_video_display_mDNIe_scenario_cmds[][1] = {
	{{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE,  sizeof(mDNIe_UI_MODE), mDNIe_UI_MODE},},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE,  sizeof(mDNIe_VIDEO_MODE), mDNIe_VIDEO_MODE},},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE,  sizeof(mDNIe_VIDEO_WARM_MODE), mDNIe_VIDEO_WARM_MODE},},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE,  sizeof(mDNIe_VIDEO_COLD_MODE), mDNIe_VIDEO_COLD_MODE},},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE,  sizeof(mDNIe_CAMERA_MODE), mDNIe_CAMERA_MODE},},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE,  sizeof(mDNIe_NAVI_MODE), mDNIe_NAVI_MODE},},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE,  sizeof(mDNIe_GALLERY_MODE), mDNIe_GALLERY_MODE},},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE,  sizeof(mDNIe_VT_MODE), mDNIe_VT_MODE},},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE,  sizeof(mDNIe_NEGATIVE_MODE), mDNIe_NEGATIVE_MODE},},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE,  sizeof(mDNIe_VIDEO_OUTDOOR_MODE), mDNIe_VIDEO_OUTDOOR_MODE},},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE,  sizeof(mDNIe_VIDEO_WARM_OUTDOOR_MODE), mDNIe_VIDEO_WARM_OUTDOOR_MODE},},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE,  sizeof(mDNIe_VIDEO_COLD_OUTDOOR_MODE), mDNIe_VIDEO_COLD_OUTDOOR_MODE},},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, HX8369B_CMD_SETTLE,  sizeof(mDNIe_CAMERA_OUTDOOR_MODE), mDNIe_CAMERA_OUTDOOR_MODE},},
};
#endif

/*
static void hx8369b_disp_powerup(void)
{
	DPRINT("%s\n", __func__);
}

static void hx8369b_disp_powerdown(void)
{
	DPRINT("%s\n", __func__);
}
*/

// CHECK_TEMPERATURE = TEMPERATURE VALUE * 10
#define CHECK_TEMPERATURE_HIGH	450
#define CHECK_TEMPERATURE	0
int get_lcd_temperature=0;

static int __init hx8369b_lcd_read_id(char *param)
{
	int index, digit, negative;
	if(param[0]=='5'&&param[1]=='5'&&param[2]=='4'&&param[3]=='8'&&param[4]=='9'&&param[5]=='0') {
		hx8369b_read_id=HX8369B_AUO_554890;
		DPRINT("AUO1\n");
	} else if(param[0]=='5'&&param[1]=='5'&&param[2]=='4'&&param[3]=='9'&&param[4]=='9'&&param[5]=='0') {
		hx8369b_read_id=HX8369B_AUO_554990;
		DPRINT("AUO2\n");
	} else if(param[0]=='5'&&param[1]=='5'&&param[2]=='C'&&param[3]=='0'&&param[4]=='9'&&param[5]=='0') {
		hx8369b_read_id=HX8369B_BOE_55C090;
		DPRINT("BOE\n");		
	} else if(param[0]=='5'&&param[1]=='5'&&param[2]=='B'&&param[3]=='C'&&param[4]=='9'&&param[5]=='0') {
		hx8369b_read_id=HX8369B_BOE_55BC90;
		DPRINT("BOE_DELOS\n");		
	} else if(param[0]=='8'&&param[1]=='3'&&param[2]=='6'&&param[3]=='9'&&param[4]=='5'&&param[5]=='A') {
		hx8369b_read_id=HX8369B_SMD_83695A;
		DPRINT("SMD\n");
	} else {
		hx8369b_read_id=HX8369B_BOE_55C090;
		DPRINT("Default\n");
	}

	if(hx8369b_read_id==HX8369B_BOE_55BC90||hx8369b_read_id==HX8369B_SMD_83695A)
		return 1;

	index=7;
	digit=0;
	negative=1;
	get_lcd_temperature=0;
	
	while (digit < 3) {
		if(param[index] == '-') {
			negative = -1;
			index++;
			continue;
		} else if(param[index] >= '0' && param[index] <= '9') {
			get_lcd_temperature *= 10;
			get_lcd_temperature += (param[index]-'0');
		} else {
			break;
		}

		digit++;
		index++;
	} 
	get_lcd_temperature *= (negative * 10);

	return 1;
}
__setup("LCDID=", hx8369b_lcd_read_id);

static void hx8369b_lcd_set_temperature(void)
{
	DPRINT("Temperature ADC : %d\n", get_lcd_temperature);

/*
	if(get_lcd_temperature>=CHECK_TEMPERATURE_HIGH) {
		hx8369b_auo3_1[20]=0x40;
	  
		hx8369b_boe4[3]=0x08;	
		hx8369b_boe4[5]=0x0A;
		hx8369b_boe4[18]=0x0A;
		hx8369b_boe4[19]=0x01;
		hx8369b_boe4[20]=0x02;
		hx8369b_boe4[21]=0x47;
		hx8369b_boe4[91]=0x80;
	
		hx8369b_boe5[1]=0x0C;
		hx8369b_boe5[3]=0x77;
	
		hx8369b_boe13[1]=0x07;
		hx8369b_boe13[2]=0x0F;
		hx8369b_boe13[3]=0x07;
		hx8369b_boe13[4]=0x0F;
	
		hx8369b_boe14[4]=0x34;
		hx8369b_boe14[6]=0x09;
	} else 
*/
  if(get_lcd_temperature>=CHECK_TEMPERATURE) {
		hx8369b_auo3_1[20]=0x40;
		  
		hx8369b_boe4[3]=0x08;	// D5
		hx8369b_boe4[5]=0x0A;
		hx8369b_boe4[18]=0x0A;
		hx8369b_boe4[19]=0x0A;
		hx8369b_boe4[20]=0x0B;
		hx8369b_boe4[21]=0x47;
		hx8369b_boe4[36]=0x26;
		hx8369b_boe4[91]=0x80;
	
		hx8369b_boe5[1]=0x0C;
		hx8369b_boe5[3]=0x77;
	
		hx8369b_boe13[1]=0x07;
		hx8369b_boe13[2]=0x0F;
		hx8369b_boe13[3]=0x07;
		hx8369b_boe13[4]=0x0F;
		  
		hx8369b_boe14[6]=0x09;	  
	} else {
		hx8369b_auo3_1[20]=0x4C;
		  
		hx8369b_boe4[3]=0x0B;
		hx8369b_boe4[5]=0x0C;
		hx8369b_boe4[18]=0x0C;
		hx8369b_boe4[19]=0x01;
		hx8369b_boe4[20]=0x49;
		hx8369b_boe4[21]=0x37;
		hx8369b_boe4[36]=0x10;
		hx8369b_boe4[91]=0x05;
		  
		hx8369b_boe5[1]=0x11;
		hx8369b_boe5[3]=0x78;
		  
		hx8369b_boe13[1]=0x00;
		hx8369b_boe13[2]=0x00;
		hx8369b_boe13[3]=0x16;
		hx8369b_boe13[4]=0x16;
		  
		hx8369b_boe14[6]=0x03;	  
	}
}
EXPORT_SYMBOL(get_lcd_temperature);


#if defined(CONFIG_MDNIE_TUNING)
static int parse_text(char *src, int len)
{
	int i;
	int data=0, value=0, count=0, comment=0;
	char *cur_position;

	mDNIe_data[count] = 0xE6;
	count++;
	
	cur_position = src;
	for(i=0; i<len; i++, cur_position++) {
		char a = *cur_position;
		switch(a) {
			case '\r':
			case '\n':
				comment = 0;
				data = 0;
				break;
			case '/':
				comment++;
				data = 0;
				break;
			case '0'...'9':
				if(comment>1)
					break;
				if(data==0 && a=='0')
					data=1;
				else if(data==2){
					data=3;
					value = (a-'0')*16;
				}
				else if(data==3){
					value += (a-'0');
					mDNIe_data[count]=value;
					DPRINT("Tuning value[%d]=0x%02X\n", count, value);
					count++;
					data=0;
				}
				break;
			case 'a'...'f':
			case 'A'...'F':
				if(comment>1)
					break;
				if(data==2){
					data=3;
					if(a<'a') value = (a-'A'+10)*16;
					else value = (a-'a'+10)*16;
				}
				else if(data==3){
					if(a<'a') value += (a-'A'+10);
					else value += (a-'a'+10);
					mDNIe_data[count]=value;
					DPRINT("Tuning value[%d]=0x%02X\n", count, value);
					count++;
					data=0;
				}
				break;
			case 'x':
			case 'X':
				if(data==1)
					data=2;
				break;
			default:
				if(comment==1)
					comment = 0;
				data = 0;
				break;
		}
	}

	return count;
}

static int load_tuning_data(char *filename)
{
    struct file *filp;
	char	*dp;
	long	l ;
	loff_t  pos;
	int     ret, num;
	mm_segment_t fs;

	DPRINT("[INFO]:%s called loading file name : [%s]\n",__func__,filename);

	fs = get_fs();
	set_fs(get_ds());

	filp = filp_open(filename, O_RDONLY, 0);
	if(IS_ERR(filp)) 
	{
		printk(KERN_ERR "[ERROR]:File open failed\n");
		return -1;
	}

	l = filp->f_path.dentry->d_inode->i_size;
	DPRINT("[INFO]: Loading File Size : %ld(bytes)", l);

	dp = kmalloc(l+10, GFP_KERNEL);	
	if(dp == NULL){
		DPRINT("[ERROR]:Can't not alloc memory for tuning file load\n");
		filp_close(filp, current->files);
		return -1;
	}
	pos = 0;
	memset(dp, 0, l);
	DPRINT("[INFO] : before vfs_read()\n");
	ret = vfs_read(filp, (char __user *)dp, l, &pos);   
	DPRINT("[INFO] : after vfs_read()\n");

	if(ret != l) {
        DPRINT("[ERROR] : vfs_read() filed ret : %d\n",ret);
        kfree(dp);
		filp_close(filp, current->files);
		return -1;
	}

	filp_close(filp, current->files);

	set_fs(fs);
	num = parse_text(dp, l);
	hx8369b_video_display_mDNIe_tune_cmds[0].dlen = num;

	if(!num) {
		DPRINT("[ERROR]:Nothing to parse\n");
		kfree(dp);
		return -1;
	}

	DPRINT("[INFO] : Loading Tuning Value's Count : %d", num);

	kfree(dp);
	return num;
}

static ssize_t mDNIeTuning_show(struct device *dev,
        struct device_attribute *attr, char *buf)

{
    int ret = 0;
    ret = sprintf(buf,"Tunned File Name : %s\n",tuning_filename);

    return ret;
}

static ssize_t mDNIeTuning_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
	char *pt;
	char a;
	unsigned long tunning_mode=0;

	a = *buf;

	if(a=='1') {
		tuning_enable = 1;
        DPRINT("%s:Tuning_enable\n",__func__);
	} else if(a=='0') {
		tuning_enable = 0;
        DPRINT("%s:Tuning_disable\n",__func__);
	} else {
        memset(tuning_filename,0,sizeof(tuning_filename));
        sprintf(tuning_filename,"%s%s",TUNING_FILE_PATH,buf);
    
        pt = tuning_filename;
        while(*pt)
        {
            if(*pt =='\r'|| *pt =='\n')
            {
                *pt = 0;
                break;
            }
            pt++;
        }
        DPRINT("%s:%s\n",__func__,tuning_filename);
    
        if (load_tuning_data(tuning_filename) <= 0) {
            DPRINT("[ERROR]:load_tunig_data() failed\n");
            return size;
        }

        mutex_lock(&lcdc_mlock);
        if(tuning_enable && hx8369b_video_display_mDNIe_tune_cmds[0].dlen > 0) {
            mipi_dsi_cmds_tx(&hx8369b_tx_buf,
                hx8369b_video_display_mDNIe_tune_cmds,
                ARRAY_SIZE(hx8369b_video_display_mDNIe_tune_cmds));
        }
        mutex_unlock(&lcdc_mlock);

    }
    return size;
}

static DEVICE_ATTR(tuning, 0664, mDNIeTuning_show, mDNIeTuning_store);
#endif

#if defined(CONFIG_APPLY_MDNIE)
static void set_mDNIe_Mode(struct mdnie_config *mDNIeCfg)
{
	int value;

	DPRINT("%s:[mDNIe] negative=%d\n", __func__, mDNIe_cfg.negative);
	if(!is_lcd_on)
		return;
		
	if (mDNIe_cfg.negative) {
		mutex_lock(&lcdc_mlock);
		mipi_dsi_cmds_tx(&hx8369b_tx_buf,
			hx8369b_video_display_mDNIe_scenario_cmds[SCENARIO_MAX],
			ARRAY_SIZE(hx8369b_video_display_mDNIe_scenario_cmds[SCENARIO_MAX]));
		mDNIeCfg->curIndex = SCENARIO_MAX;
		mutex_unlock(&lcdc_mlock);
		return;
	}
	
	switch(mDNIeCfg->scenario) {
	case UI_MODE:
	case GALLERY_MODE:
		value = mDNIeCfg->scenario;
		break;

	case VIDEO_MODE:
		if(mDNIeCfg->outdoor == OUTDOOR_ON) {
			value = SCENARIO_MAX + 1;
		} else {
			value = mDNIeCfg->scenario;
		}
		break;

	case VIDEO_WARM_MODE:
		if(mDNIeCfg->outdoor == OUTDOOR_ON) {
			value = SCENARIO_MAX + 2;
		} else {
			value = mDNIeCfg->scenario;
		}
		break;

	case VIDEO_COLD_MODE:
		if(mDNIeCfg->outdoor == OUTDOOR_ON) {
			value = SCENARIO_MAX + 3;
		} else {
			value = mDNIeCfg->scenario;
		}
		break;

	case CAMERA_MODE:
		if(mDNIeCfg->outdoor == OUTDOOR_ON) {
			value = SCENARIO_MAX + 4;
		} else {
			value = mDNIeCfg->scenario;
		}
		break;

	default:
		value = UI_MODE;
		break;
	};

	DPRINT("%s:[mDNIe] value=%d\n", __func__, value);

	if(mDNIeCfg->curIndex != value) {
		mutex_lock(&lcdc_mlock);
		mipi_dsi_cmds_tx(&hx8369b_tx_buf,
			hx8369b_video_display_mDNIe_scenario_cmds[value],
			ARRAY_SIZE(hx8369b_video_display_mDNIe_scenario_cmds[value]));
			mDNIeCfg->curIndex = value;
		mutex_unlock(&lcdc_mlock);
	}
}


static ssize_t mDNIeScenario_show(struct device *dev,
        struct device_attribute *attr, char *buf)

{
    int ret = 0;
    ret = sprintf(buf,"mDNIeScenario_show : %d\n", mDNIe_cfg.scenario);

    return ret;
}

static ssize_t mDNIeScenario_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned int value;
	int ret;

	ret = strict_strtoul(buf, 0, (unsigned long *)&value);

	DPRINT("%s:value=%d\n", __func__, value);

	switch(value) {
	case UI_MODE:
	case VIDEO_MODE:
	case VIDEO_WARM_MODE:
	case VIDEO_COLD_MODE:
	case CAMERA_MODE:
	case GALLERY_MODE:
		break;
		
	default:
		value = UI_MODE;
		break;
	};

	mDNIe_cfg.scenario = value;

//	mipi_set_tx_power_mode(0);	// High Speed Power
	set_mDNIe_Mode(&mDNIe_cfg);
//	mipi_set_tx_power_mode(1);	// Low Speed Power

    return size;
}

static DEVICE_ATTR(scenario, 0664, mDNIeScenario_show, mDNIeScenario_store);


static ssize_t mDNIeOutdoor_show(struct device *dev,
        struct device_attribute *attr, char *buf)

{
    int ret = 0;
    ret = sprintf(buf,"mDNIeOutdoor_show : %d\n", mDNIe_cfg.outdoor);

    return ret;
}

static ssize_t mDNIeOutdoor_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned int value;
	int ret;

	ret = strict_strtoul(buf, 0, (unsigned long *)&value);

	DPRINT("%s:value=%d\n", __func__, value);

	mDNIe_cfg.outdoor = value;

//	mipi_set_tx_power_mode(0);	// High Speed Power
	set_mDNIe_Mode(&mDNIe_cfg);
//	mipi_set_tx_power_mode(1);	// Low Speed Power

    return size;
}

static DEVICE_ATTR(outdoor, 0664, mDNIeOutdoor_show, mDNIeOutdoor_store);

static ssize_t mDNIeNegative_show(struct device *dev,
        struct device_attribute *attr, char *buf)

{
    int ret = 0;
    ret = sprintf(buf,"mDNIeNegative_show : %d\n", mDNIe_cfg.negative);

    return ret;
}

static ssize_t mDNIeNegative_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned int value;
	int ret;

	ret = strict_strtoul(buf, 0, (unsigned long *)&value);

	DPRINT("%s:value=%d\n", __func__, value);

	if(value == 1) {
		mDNIe_cfg.negative = 1;
	} else {
		mDNIe_cfg.negative = 0;
		mDNIe_cfg.scenario = 0;
	}
		
//	mipi_set_tx_power_mode(0);	// High Speed Power
	set_mDNIe_Mode(&mDNIe_cfg);
//	mipi_set_tx_power_mode(1);	// Low Speed Power
		
    return size;
}

static DEVICE_ATTR(negative, 0664, mDNIeNegative_show, mDNIeNegative_store);
#endif

#ifdef CONFIG_PANEL_ESD_DETECT


static int esd_det_irq_enable(void);

static void esd_det_irq_disable(void)
{
	free_irq(lcd_det_data.irq, &lcd_det_data);
	DPRINT("ESD : Disable\n");
}

static void esd_det_init_wait(int mdelay_cnt)
{
	while(is_ESD_status && mdelay_cnt) 
	{
		msleep(100);
		mdelay_cnt--;
		DPRINT("ESD : Wait 100ms\n",__func__);
	}
}

static void lcd_det_work_func(struct work_struct *work)
{
	int rc=0;
	DPRINT("ESD +\n");

//	esd_det_irq_disable();	

	is_ESD_status = true;

	esd_det_init_wait(2);


	// Panel power off
	is_lcd_on = 0;

	
	gpio_set_value_cansleep(GPIO_LCD_RESET_N, 0);
	msleep(20);
	gpio_set_value_cansleep(GPIO_LCD_RESET_N, 1);
	msleep(150);	
	DPRINT("ESD : HW Reset\n");


	if(hx8369b_read_id==HX8369B_AUO_554890)
	{
		mipi_dsi_cmds_tx(&hx8369b_tx_buf, hx8369b_video_display_init_auo1_cmds, ARRAY_SIZE(hx8369b_video_display_init_auo1_cmds));
	}
	else if(hx8369b_read_id==HX8369B_AUO_554990)
	{
		mipi_dsi_cmds_tx(&hx8369b_tx_buf, hx8369b_video_display_init_auo2_cmds, ARRAY_SIZE(hx8369b_video_display_init_auo2_cmds));
	}
	else if(hx8369b_read_id==HX8369B_SMD_83695A)	{
#if defined(CONFIG_MACH_BAFFIN_DUOS_CTC)		
			mipi_dsi_cmds_tx(&hx8369b_tx_buf, BAFFIN_hx8369b_video_display_init_rev1_cmds, ARRAY_SIZE(BAFFIN_hx8369b_video_display_init_rev1_cmds));
			mdelay(5);
			mipi_dsi_cmds_tx(&hx8369b_tx_buf, BAFFIN_hx8369b_video_display_init_rev1_2_cmds, ARRAY_SIZE(BAFFIN_hx8369b_video_display_init_rev1_2_cmds));
			mipi_dsi_cmds_tx(&hx8369b_tx_buf, BAFFIN_hx8369b_video_display_init_rev1_4_cmds, ARRAY_SIZE(BAFFIN_hx8369b_video_display_init_rev1_4_cmds));
			mipi_dsi_cmds_tx(&hx8369b_tx_buf, hx8369b_video_display_on_cmds_rotate, ARRAY_SIZE(hx8369b_video_display_on_cmds_rotate));
			mdelay(5);
			mipi_dsi_cmds_tx(&hx8369b_tx_buf, BAFFIN_hx8369b_video_display_init_rev1_3_cmds, ARRAY_SIZE(BAFFIN_hx8369b_video_display_init_rev1_3_cmds));
			mdelay(180);
			DPRINT("ID : SMD 0x%06X \n",hx8369b_read_id);	
#endif			
	}

	DPRINT("ESD : display_init_cmds\n");
	
#if defined(CONFIG_APPLY_MDNIE)
	set_mDNIe_Mode(&mDNIe_cfg);
#endif
	
#if defined(CONFIG_MDNIE_TUNING)
	if(tuning_enable && hx8369b_video_display_mDNIe_tune_cmds[0].dlen > 0) {
		mipi_dsi_cmds_tx(&hx8369b_tx_buf, hx8369b_video_display_mDNIe_tune_cmds, ARRAY_SIZE(hx8369b_video_display_mDNIe_tune_cmds));
		DPRINT("ESD : mDNIe_tune_cmds\n");
	}
#endif


	if(hx8369b_read_id==HX8369B_SMD_83695A) {
			mipi_dsi_cmds_tx(&hx8369b_tx_buf, hx8369b_video_display_on_smd_cmds, ARRAY_SIZE(hx8369b_video_display_on_smd_cmds));
			mdelay(100);
	}else{
		mipi_dsi_cmds_tx(&hx8369b_tx_buf, hx8369b_video_display_on_cmds, 	ARRAY_SIZE(hx8369b_video_display_on_cmds));
	}

	DPRINT("ESD : display_on_cmds\n");

	is_lcd_on = 1;

	enable_irq(lcd_det_data.irq);
	//esd_det_irq_enable();
	is_ESD_status = false;

	DPRINT("ESD -\n");
}


static irqreturn_t lcd_det_irq_handler(int irq, void *dev_id)
{
	struct lcd_det_data_struct *lcd_det = dev_id;

	disable_irq_nosync(lcd_det->irq);
	DPRINT("lcd_det->irq  %d\n",lcd_det->irq);

	if(is_lcd_on==1)
	{	
		if(lcd_det->irq !=-1)
		{
			queue_work(lcd_det_wq, &lcd_det->work_lcd_det);
		}

		DPRINT("IRQ_HANDLED \n");

		return IRQ_HANDLED;
	} 
	else 
	{
		DPRINT("LCD Power down OK. \n");
		enable_irq(lcd_det->irq);
		return IRQ_HANDLED;
	}

}


static int esd_det_irq_enable(void)
{
	int ret;

	/* work queue setting */
	lcd_det_wq= create_singlethread_workqueue("lcd_det_wq");
	if (!lcd_det_wq)
		return -ENOMEM;
	INIT_WORK(&lcd_det_data.work_lcd_det, lcd_det_work_func);
	DPRINT("ESD : Workqueue Settings complete\n");

	DPRINT("lcd_det_data.irq = %d \n", lcd_det_data.irq);
	
	/* INT setting */
//	irq_set_irq_type(lcd_det_data.irq, IRQ_TYPE_EDGE_RISING);
	
	ret = request_irq(lcd_det_data.irq, lcd_det_irq_handler, IRQF_TRIGGER_HIGH, "gpio_lcd_detect", &lcd_det_data);
	if (ret)
	{
		free_irq(lcd_det_data.irq, &lcd_det_data);
		DPRINT("ESD : request_irq failed for lcd_det\n");
	}

	return ret;
}


#endif


static int mipi_hx8369b_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;
	static int rotate;
	int ret = 0;

#ifdef CONFIG_PANEL_ESD_DETECT
	esd_det_init_wait(6);
#endif

	DPRINT("%s +\n", __func__);

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;

	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi  = &mfd->panel_info.mipi;

	if(is_lcd_on)
		backlight_ic_set_brightness(0);

	if(hx8369b_read_id==HX8369B_AUO_554890||hx8369b_read_id==HX8369B_AUO_554990||hx8369b_read_id==HX8369B_BOE_55C090)
		hx8369b_lcd_set_temperature();

	if (mipi_hx8369b_pdata && mipi_hx8369b_pdata->rotate_panel)
		rotate = mipi_hx8369b_pdata->rotate_panel();

	mutex_lock(&lcdc_mlock);

	DPRINT("Setting +\n");

	if (mipi->mode == DSI_VIDEO_MODE) {
		if(hx8369b_read_id==HX8369B_AUO_554890) {
			ret  = mipi_dsi_cmds_tx(&hx8369b_tx_buf, hx8369b_video_display_init_auo1_cmds, ARRAY_SIZE(hx8369b_video_display_init_auo1_cmds));
			DPRINT("ID : AUO1 0x%06X \n",hx8369b_read_id);
			if(ret==0)
				goto MIPI_ERROR;
		} else if(hx8369b_read_id==HX8369B_AUO_554990)	{
			ret = mipi_dsi_cmds_tx(&hx8369b_tx_buf, hx8369b_video_display_init_auo2_cmds, ARRAY_SIZE(hx8369b_video_display_init_auo2_cmds));
			DPRINT("ID : AUO2 0x%06X \n",hx8369b_read_id);
			if(ret==0)
				goto MIPI_ERROR;
		} else if(hx8369b_read_id==HX8369B_BOE_55C090) {
			ret = mipi_dsi_cmds_tx(&hx8369b_tx_buf, hx8369b_video_display_init_boe_cmds, ARRAY_SIZE(hx8369b_video_display_init_boe_cmds));
			DPRINT("ID : BOE 0x%06X \n",hx8369b_read_id);				
			if(ret==0)
				goto MIPI_ERROR;
		} else if(hx8369b_read_id==HX8369B_BOE_55BC90) {
			ret = mipi_dsi_cmds_tx(&hx8369b_tx_buf, hx8369b_video_display_init_55BC90_cmds, ARRAY_SIZE(hx8369b_video_display_init_55BC90_cmds));
			DPRINT("ID : BOE 0x%06X \n",hx8369b_read_id);				
			if(ret==0)
				goto MIPI_ERROR;
		} else if(hx8369b_read_id==HX8369B_SMD_83695A)	{
#if defined(CONFIG_MACH_BAFFIN_DUOS_CTC)		
			ret = mipi_dsi_cmds_tx(&hx8369b_tx_buf, BAFFIN_hx8369b_video_display_init_rev1_cmds, ARRAY_SIZE(BAFFIN_hx8369b_video_display_init_rev1_cmds));
			if(ret==0)
				goto MIPI_ERROR;
			mdelay(5);
			ret = mipi_dsi_cmds_tx(&hx8369b_tx_buf, BAFFIN_hx8369b_video_display_init_rev1_2_cmds, ARRAY_SIZE(BAFFIN_hx8369b_video_display_init_rev1_2_cmds));
			if(ret==0)
				goto MIPI_ERROR;
			ret = mipi_dsi_cmds_tx(&hx8369b_tx_buf, BAFFIN_hx8369b_video_display_init_rev1_4_cmds, ARRAY_SIZE(BAFFIN_hx8369b_video_display_init_rev1_4_cmds));
			if(ret==0)
				goto MIPI_ERROR;
			DPRINT("ID : SMD 0x%06X \n",hx8369b_read_id);
#endif			
		} else {
			ret = mipi_dsi_cmds_tx(&hx8369b_tx_buf, hx8369b_video_display_init_auo1_cmds, ARRAY_SIZE(hx8369b_video_display_init_auo1_cmds));
			DPRINT("ID : Default 0x%06X \n",hx8369b_read_id);
			if(ret==0)
				goto MIPI_ERROR;
		}

		if (rotate&&hx8369b_read_id!=HX8369B_BOE_55BC90) {
			ret = mipi_dsi_cmds_tx(&hx8369b_tx_buf,
				hx8369b_video_display_on_cmds_rotate,
			ARRAY_SIZE(hx8369b_video_display_on_cmds_rotate));
			if(ret==0)
				goto MIPI_ERROR;
		}
	} else if (mipi->mode == DSI_CMD_MODE) {
#if 0	// not using
			mipi_dsi_cmds_tx(&hx8369b_tx_buf,
				nt35510_cmd_display_on_cmds,
				ARRAY_SIZE(nt35510_cmd_display_on_cmds));
#endif
	}
		
#if defined(CONFIG_APPLY_MDNIE)
	if(mDNIe_cfg.negative) {
		ret = mipi_dsi_cmds_tx(&hx8369b_tx_buf,
			hx8369b_video_display_mDNIe_scenario_cmds[SCENARIO_MAX],
			ARRAY_SIZE(hx8369b_video_display_mDNIe_scenario_cmds[SCENARIO_MAX]));
		mDNIe_cfg.curIndex = SCENARIO_MAX;
		if(ret==0)
			goto MIPI_ERROR;
	} else {
		ret = mipi_dsi_cmds_tx(&hx8369b_tx_buf,
			hx8369b_video_display_mDNIe_scenario_cmds[0],
			ARRAY_SIZE(hx8369b_video_display_mDNIe_scenario_cmds[0]));
		mDNIe_cfg.curIndex = UI_MODE;
		if(ret==0)
			goto MIPI_ERROR;
	}
#endif

#if defined(CONFIG_MDNIE_TUNING)
	if(tuning_enable && hx8369b_video_display_mDNIe_tune_cmds[0].dlen > 0) {
		ret  = mipi_dsi_cmds_tx(&hx8369b_tx_buf,
			hx8369b_video_display_mDNIe_tune_cmds,
			ARRAY_SIZE(hx8369b_video_display_mDNIe_tune_cmds));
		if(ret==0)
			goto MIPI_ERROR;
	}
#endif

	if(hx8369b_read_id==HX8369B_AUO_554990) {
		ret = mipi_dsi_cmds_tx(&hx8369b_tx_buf, hx8369b_video_display_on_auo2_cmds, ARRAY_SIZE(hx8369b_video_display_on_auo2_cmds));
		if(ret==0)
			goto MIPI_ERROR;
	} 
	else if(hx8369b_read_id==HX8369B_SMD_83695A)	{
#if defined(CONFIG_MACH_BAFFIN_DUOS_CTC)
		mdelay(5);
		mipi_dsi_cmds_tx(&hx8369b_tx_buf, BAFFIN_hx8369b_video_display_init_rev1_3_cmds, ARRAY_SIZE(BAFFIN_hx8369b_video_display_init_rev1_3_cmds));
		if(ret==0)
			goto MIPI_ERROR;
		mdelay(200);
		mipi_dsi_cmds_tx(&hx8369b_tx_buf, hx8369b_video_display_on_smd_cmds, ARRAY_SIZE(hx8369b_video_display_on_smd_cmds));
		if(ret==0)
			goto MIPI_ERROR;
#endif
	}
	else {
		ret = mipi_dsi_cmds_tx(&hx8369b_tx_buf, hx8369b_video_display_on_cmds, ARRAY_SIZE(hx8369b_video_display_on_cmds));
		if(ret==0)
			goto MIPI_ERROR;
	}

	DPRINT("Setting -\n");

	is_lcd_on = 1;		
	mutex_unlock(&lcdc_mlock);

	backlight_ic_set_brightness(mfd->bl_level);

#ifdef CONFIG_PANEL_ESD_DETECT
	esd_det_irq_enable();
#endif
	DPRINT("%s -\n", __func__);
	return ret;

MIPI_ERROR:
	mutex_unlock(&lcdc_mlock);
	DPRINT("%s MIPI_ERROR -\n", __func__);
	return ret;
}

static int mipi_hx8369b_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	int ret = 0;

	DPRINT("%s +\n", __func__);

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	backlight_ic_set_brightness(0);
	
#if defined(CONFIG_MACH_BAFFIN_DUOS_CTC)
		gpio_set_value(GPIO_PWM_CTRL, 0);
#endif
	
	is_lcd_on = 0;
	
	ret = mipi_dsi_cmds_tx(&hx8369b_tx_buf, hx8369b_disp_off_cmds,
			ARRAY_SIZE(hx8369b_disp_off_cmds));
	if(ret==0)
		goto MIPI_ERROR;	

	ret = mipi_dsi_cmds_tx(&hx8369b_tx_buf, hx8369b_sleep_in_cmds,
			ARRAY_SIZE(hx8369b_sleep_in_cmds));
	if(ret==0)
		goto MIPI_ERROR;
	
#ifdef CONFIG_PANEL_ESD_DETECT
	esd_det_irq_disable();
#endif

	mipi_set_tx_power_mode(1);	// Low Speed Power
	
	DPRINT("%s -\n", __func__);
	return 0;

MIPI_ERROR:
	DPRINT("%s MIPI_ERROR -\n", __func__);
	return ret;
}


static ssize_t mipi_hx8369b_lcdtype_show(struct device *dev, 
        struct device_attribute *attr, char *buf)
{
	char temp[30];
	if(hx8369b_read_id==HX8369B_BOE_55BC90) {
		sprintf(temp, "INH_%x\n", hx8369b_read_id);
	} else if(hx8369b_read_id==HX8369B_SMD_83695A) {
		sprintf(temp, "SMD_LMS501KF07\n");
	} else {
		sprintf(temp, "AUO_H430VAN01.0\n");
	}
	strcat(buf, temp);
	return strlen(buf);
}

/* FIXME: add handlers */
static struct lcd_ops mipi_lcd_props;
static DEVICE_ATTR(lcd_type, S_IRUGO, mipi_hx8369b_lcdtype_show, NULL);

static int __devinit mipi_hx8369b_lcd_probe(struct platform_device *pdev)
{
	struct lcd_device *lcd_device;
#if defined(CONFIG_APPLY_MDNIE)
	struct class *mdnie_class;
	struct device *mdnie_device;
#endif
        int ret;

#ifdef CONFIG_PANEL_ESD_DETECT
	lcd_det_data.irq = MSM_GPIO_TO_INT(GPIO_LCD_DETECT);
#endif

#if defined(CONFIG_MACH_BAFFIN_DUOS_CTC)
		gpio_tlmm_config(GPIO_CFG(GPIO_PWM_CTRL, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
#endif

	DPRINT("%s\n", __func__);

	if (pdev->id == 0) {
		mipi_hx8369b_pdata = pdev->dev.platform_data;
		return 0;
	}

	mutex_init( &lcdc_mlock );
	
	msm_fb_add_device(pdev);
	lcd_device = lcd_device_register("panel", &pdev->dev, NULL,
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

#if defined(CONFIG_MDNIE_TUNING)
	ret = sysfs_create_file(&lcd_device->dev.kobj, &dev_attr_tuning.attr);
	if (ret)
		printk(KERN_ERR "sysfs create fail - %s\n",
		dev_attr_tuning.attr.name);
#endif
#if defined(CONFIG_APPLY_MDNIE)
	mDNIe_cfg.scenario = UI_MODE;
	mDNIe_cfg.negative = 0;
	mDNIe_cfg.outdoor = 0;

	mdnie_class = class_create(THIS_MODULE, "mdnie");
	if (IS_ERR(mdnie_class)) {
		ret = PTR_ERR(mdnie_class);
		printk(KERN_ERR "mdnie : failed to register class\n");
		return ret;
	}

	mdnie_device = device_create(mdnie_class, NULL, 0, NULL, "mdnie");
	if (IS_ERR(mdnie_device)) {
		ret = PTR_ERR(mdnie_device);
		printk(KERN_ERR "mdnie : failed to register device\n");
		return ret;
	}

	ret = device_create_file(mdnie_device, &dev_attr_scenario);
	if(ret < 0)
		printk(KERN_ERR "sysfs create fail - %s\n",
		dev_attr_scenario.attr.name);

	ret = device_create_file(mdnie_device, &dev_attr_outdoor);
	if(ret < 0)
		printk(KERN_ERR "sysfs create fail - %s\n",
		dev_attr_outdoor.attr.name);
		
	ret = device_create_file(mdnie_device, &dev_attr_negative);
	if(ret < 0)
		printk(KERN_ERR "sysfs create fail - %s\n",
		dev_attr_negative.attr.name);
#endif

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_hx8369b_lcd_probe,
	.driver = {
		.name   = "mipi_HX8369B",
	},
};


static void mipi_hx8369b_set_backlight(struct msm_fb_data_type *mfd)
{
	int level = mfd->bl_level;

	pr_info("BL : %d, lcd_on : %d\n", level, is_lcd_on);

	if(!is_lcd_on)
		return;
        /* function will spin lock */
	backlight_ic_set_brightness(level);
}

static struct msm_fb_panel_data hx8369b_panel_data = {
	.on	= mipi_hx8369b_lcd_on,
	.off = mipi_hx8369b_lcd_off,
	.set_backlight = mipi_hx8369b_set_backlight,
};

static int ch_used[3];

static int mipi_hx8369b_lcd_init(void)
{
	DPRINT("%s\n", __func__);

	mipi_dsi_buf_alloc(&hx8369b_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&hx8369b_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}

int mipi_hx8369b_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	DPRINT("%s\n", __func__);

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	ret = mipi_hx8369b_lcd_init();
	if (ret) {
		DPRINT("mipi_hx8369b_lcd_init() failed with ret %u\n", ret);
		return ret;
	}

	pdev = platform_device_alloc("mipi_HX8369B", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	hx8369b_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &hx8369b_panel_data,
				sizeof(hx8369b_panel_data));
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
