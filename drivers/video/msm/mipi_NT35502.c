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
#include <mach/pmic.h>
#include <mach/vreg.h>

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_NT35502.h"

#include "lcdc_backlight_ic.h"

static struct msm_panel_common_pdata *mipi_nt35502_pdata;
static struct dsi_buf nt35502_tx_buf;
static struct dsi_buf nt35502_rx_buf;

static struct msm_fb_data_type *mfd;

/* in msec */
#define NT35502_RESET_SLEEP_OUT_DELAY 120
#define NT35502_RESET_SLEEP_IN_DELAY 5
#define NT35502_SLEEP_IN_DELAY 120
#define NT35502_SLEEP_OUT_DELAY 120
#define NT35502_HOLD_RESET 10
#define NT35502_CMD_SETTLE 0
#define NT35502_SETTING_SETTLE 0

#define NT35502_USE_DEEP_STANDBY 1

#if (0 == NT35502_USE_DEEP_STANDBY)
#define NT35502_USE_POWERDOWN 1
#else /* (0 == NT35502_USE_DEEP_STANDBY) */
#define NT35502_USE_POWERDOWN 0
#endif /* (0 == NT35502_USE_DEEP_STANDBY) */

static int is_lcd_on;
/*
//#define CONFIG_MDNIE_TUNING
*/
#if defined(CONFIG_MDNIE_TUNING)
#define TUNING_FILE_PATH "/sdcard/"
static int tuning_enable;
unsigned char mDNIe_data[500];
static char tuning_filename[100];
static int cmd_count;
static int cmd_length_ary[50];
static int mDNIe_tx_tuning();
#endif

#define CONFIG_APPLY_MDNIE
#if defined(CONFIG_APPLY_MDNIE)
/*
typedef struct mdnie_config {
	int scenario;
	int negative;
	int outdoor;
};*/
static struct class *mdnie_class;
static struct mdnie_config mDNIe_cfg;
static void set_mDNIe_Mode(struct mdnie_config *mDNIeCfg);
#endif
#if defined(CONFIG_APPLY_MDNIE) || defined(CONFIG_MDNIE_TUNING)
static struct mutex lcdc_mlock;
#endif

/* common setting */
static char exit_sleep[] = {0x11, 0x00};
static char display_on[] = {0x29, 0x00};
static char display_off[] = {0x28, 0x00};
static char enter_sleep[] = {0x10, 0x00};
/*
//static char all_pixel_off[] = {0x22, 0x00};
//static char normal_disp_mode_on[] = {0x13, 0x00};
*/

#if (1 == NT35502_USE_DEEP_STANDBY)
static char deep_standby_on[] = {0x4F, 0x01};
#endif /* (1 == NT35502_USE_DEEP_STANDBY) */
/* //static char write_ram[] = {0x2c, 0x00}; */ /* write ram */


#if defined(CONFIG_MDNIE_TUNING)

static char read_mDNIe_reg[] = {
	0xE4
};

static struct dsi_cmd_desc nt35502_video_read_mDNIe_cmds[] = {
	{DTYPE_DCS_READ, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(read_mDNIe_reg), read_mDNIe_reg},
};


static struct dsi_cmd_desc nt35502_cmd_mDNIe_tune_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,  0, mDNIe_data},
};
#endif
#if defined(CONFIG_APPLY_MDNIE)
/*
//static char inversion_on[2] = {0x21, 0x00};

static struct dsi_cmd_desc nt35502_cmd_inversion_on_cmds = {
	DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(inversion_on), inversion_on};
static char inversion_off[2] = {0x20, 0x00};
static struct dsi_cmd_desc nt35502_cmd_inversion_off_cmds = {
	DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(inversion_off), inversion_off};

*/

#endif

static struct dsi_cmd_desc nt35502_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 120,
		sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 70,
		sizeof(enter_sleep), enter_sleep}
};

#if (1 == NT35502_USE_DEEP_STANDBY)
static struct dsi_cmd_desc nt35502_deep_standby_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(deep_standby_on), deep_standby_on},
/*
	{DTYPE_DCS_WRITE, 1, 0, 0, 50, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 50, sizeof(enter_sleep), enter_sleep}
*/
};
#endif /* (1 == NT35502_USE_DEEP_STANDBY) */

/*Power Setting*/
static char video1[] = {
	0xF0,
		0x55, 0xAA, 0x52, 0x08, 0x01
};
static char video2[] = {
	0xB0,
		0x08
};
static char video3[] = {
	0xB2,
		0x60
};
static char video4[] = {
	0xB3,
		0x2B
};
static char video5[] = {
	0xB4,
		0x1F
};
static char video6[] = {
	0xB5,
		0x64
};
static char video7[] = {
	0xB9,
		0x46
};
static char video8[] = {
	0xBA,
		0x36
};
static char video9[] = {
	0xBC,
		0x60, 0x00
};
static char video10[] = {
	0xBD,
		0x60, 0x00
};
/*
static char video11[] = {
	0xC7,
		0x55, 0x8C, 0xC0
};
*/

/* Gamma Control */
static char video12[] = {
	0xD1,
		0x00, 0xDC, 0x00, 0xE6, 0x00, 0xF5, 0x01, 0x02,
		0x01, 0x0E, 0x01, 0x25, 0x01, 0x39, 0x01, 0x5C
};
static char video13[] = {
	0xD2,
		0x01, 0x7A, 0x01, 0xAC, 0x01, 0xD5, 0x02, 0x18,
		0x02, 0x4F, 0x02, 0x50, 0x02, 0x85, 0x02, 0xBE
};
static char video14[] = {
	0xD3,
		0x02, 0xE4, 0x03, 0x16, 0x03, 0x38, 0x03, 0x66,
		0x03, 0x7F, 0x03, 0xA9, 0x03, 0xBC, 0x03, 0xDA
};
static char video15[] = {
	0xD4,
		0x03, 0xE6, 0x03, 0xE8
};

static char video16[] = {
	0xE0,
		0x00, 0xDC, 0x00, 0xE6, 0x00, 0xF5, 0x01, 0x02,
		0x01, 0x0E, 0x01, 0x25, 0x01, 0x39, 0x01, 0x5C
};
static char video17[] = {
	0xE1,
		0x01, 0x7A, 0x01, 0xAC, 0x01, 0xD5, 0x02, 0x18,
		0x02, 0x4F, 0x02, 0x50, 0x02, 0x85, 0x02, 0xBE
};

static char video18[] = {
	0xE2,
		0x02, 0xE4, 0x03, 0x16, 0x03, 0x38, 0x03, 0x66,
		0x03, 0x7F, 0x03, 0xA9, 0x03, 0xBC, 0x03, 0xDA
};
static char video19[] = {
	0xE3,
		0x03, 0xE6, 0x03, 0xE8
};

static char video20[] = {
	0xD5,
		0x00, 0xF1, 0x00, 0xF8, 0x01, 0x06, 0x01, 0x13,
		0x01, 0x1F, 0x01, 0x35, 0x01, 0x48, 0x01, 0x6A
};
static char video21[] = {
	0xD6,
		0x01, 0x86, 0x01, 0xB7, 0x01, 0xDE, 0x02, 0x1F,
		0x02, 0x55, 0x02, 0x57, 0x02, 0x89, 0x02, 0xC3
};
static char video22[] = {
	0xD7,
		0x02, 0xE7, 0x03, 0x19, 0x03, 0x3A, 0x03, 0x67,
		0x03, 0x84, 0x03, 0xA8, 0x03, 0xBE, 0x03, 0xD6
};
static char video23[] = {
	0xD8,
		0x03, 0xE6, 0x03, 0xE8
};

static char video24[] = {
	0xE4,
		0x00, 0xF1, 0x00, 0xF8, 0x01, 0x06, 0x01, 0x13,
		0x01, 0x1F, 0x01, 0x35, 0x01, 0x48, 0x01, 0x6A
};
static char video25[] = {
	0xE5,
		0x01, 0x86, 0x01, 0xB7, 0x01, 0xDE, 0x02, 0x1F,
		0x02, 0x55, 0x02, 0x57, 0x02, 0x89, 0x02, 0xC3
};

static char video26[] = {
	0xE6,
		0x02, 0xE7, 0x03, 0x19, 0x03, 0x3A, 0x03, 0x67,
		0x03, 0x84, 0x03, 0xA8, 0x03, 0xBE, 0x03, 0xD6
};
static char video27[] = {
	0xE7,
		0x03, 0xE6, 0x03, 0xE8
};

static char video28[] = {
	0xD9,
		0x00, 0x00, 0x00, 0x29, 0x00, 0x58, 0x00, 0x7C,
		0x00, 0x97, 0x00, 0xC2, 0x00, 0xE5, 0x01, 0x1A
};
static char video29[] = {
	0xDD,
		0x01, 0x44, 0x01, 0x85, 0x01, 0xB8, 0x02, 0x07,
		0x02, 0x46, 0x02, 0x48, 0x02, 0x7E, 0x02, 0xBC
};
static char video30[] = {
	0xDE,
		0x02, 0xE1, 0x03, 0x15, 0x03, 0x37, 0x03, 0x63,
		0x03, 0x82, 0x03, 0xAA, 0x03, 0xBF, 0x03, 0xDA
};
static char video31[] = {
	0xDF,
		0x03, 0xE6, 0x03, 0xE8
};

static char video32[] = {
	0xE8,
		0x00, 0x00, 0x00, 0x29, 0x00, 0x58, 0x00, 0x7C,
		0x00, 0x97, 0x00, 0xC2, 0x00, 0xE5, 0x01, 0x1A
};
static char video33[] = {
	0xE9,
		0x01, 0x44, 0x01, 0x85, 0x01, 0xB8, 0x02, 0x07,
		0x02, 0x46, 0x02, 0x48, 0x02, 0x7E, 0x02, 0xBC
};

static char video34[] = {
	0xEA,
		0x02, 0xE1, 0x03, 0x15, 0x03, 0x37, 0x03, 0x63,
		0x03, 0x82, 0x03, 0xAA, 0x03, 0xBF, 0x03, 0xDA
};
static char video35[] = {
	0xEB,
		0x03, 0xE6, 0x03, 0xE8
};


/* Display Setting */
static char video36[] = {
	0xF0,
		0x55, 0xAA, 0x52, 0x08, 0x00
};

static char video37[] = {
	0xB1,
		0x07, 0x08
};

static char video38[] = {
	0xB2,
		0x54, 0x11
};

static char video39[] = {
	0xB3,
		0x20, 0x00
};
static char video40[] = {
	0xB4,
		0x09, 0x50
};
static char video41[] = {
	0xB6,
		0x02
};
static char video42[] = {
	0xB7,
		0x00, 0x00
};
static char video43[] = {
	0xB8,
		0x01, 0x00
};
static char video44[] = {
	0xBB,
		0xF2, 0xF5, 0x00
};
static char video45[] = {
	0xBC,
		0x05
};
static char video46[] = {
	0xC9,
		0xC0, 0x82, 0x00, 0x64, 0x64, 0x80, 0xC3, 0x47, 0x43,
		0x00
};
static char video47[] = {
	0xEC,
		0x00, 0x04, 0x0F, 0x05, 0x07, 0x09, 0x0B, 0x0D,
		0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
		0x10, 0x10, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x04, 0x05, 0x07, 0x09,
		0x0B, 0x02, 0x0D, 0x11
};
static char video48[] = {
	0xED,
		0x0E, 0x04, 0x0F, 0x05, 0x07, 0x09, 0x0B, 0x00,
		0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
		0x10, 0x10, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x11, 0x05, 0x07, 0x09,
		0x0B, 0x02, 0x0D, 0x03
};


static char video49[] = {
	0xF0,
		0x55, 0xAA, 0x52, 0x00 , 0x00
};

/*Mafumacture command enable*/
static char video55[] = {
	0xF0,
		0x55, 0xAA, 0x52, 0x08 , 0x00
};

static char esd1[] = {
	0xFF,
		0xAA, 0x55, 0x25, 0x01
};

static char esd2[] = {
	0x6F,
		0x07
};

static char esd3[] = {
	0xF8,
		0x10
};

static char esd4[] = {
	0xFF,
		0xAA, 0x55, 0x25, 0x00
};

/*
//static char config_video_MADCTL[2] = {0x36, 0xC0};
*/
static struct dsi_cmd_desc nt35502_video_display_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 120, sizeof(video1), video1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video2), video2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video3), video3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video4), video4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video5), video5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video6), video6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video7), video7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video8), video8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video9), video9},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video10), video10},
/*	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video11), video11}, */
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video12), video12},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video13), video13},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video14), video14},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video15), video15},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video16), video16},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video17), video17},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video18), video18},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video19), video19},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video20), video20},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video21), video21},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video22), video22},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video23), video23},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video24), video24},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video25), video25},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video26), video26},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video27), video27},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video28), video28},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video29), video29},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video30), video30},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video31), video31},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video32), video32},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video33), video33},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video34), video34},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video35), video35},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(esd2), esd2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(esd3), esd3},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video36), video36},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video37), video37},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video38), video38},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video39), video39},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video40), video40},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video41), video41},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video42), video42},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video43), video43},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video44), video44},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video45), video45},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video46), video46},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video47), video47},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video48), video48},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video49), video49},

	{DTYPE_DCS_WRITE, 1, 0, 0, NT35502_SLEEP_OUT_DELAY, sizeof(exit_sleep),
			exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, NT35502_CMD_SETTLE,
	 sizeof(display_on), display_on},
};

static struct dsi_cmd_desc nt35502_cmd2_enable_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video55), video55},
};

static struct dsi_cmd_desc nt35502_cmd2_disable_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video49), video49},
};

#if defined(CONFIG_APPLY_MDNIE)

static char mDNIe_negative_mode1[] = {
	0xE5,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static char mDNIe_negative_mode2[] = {
	0xE6,
		0xFF, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,
		0xFF, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,
		0xFF, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,
};

static char mDNIe_negative_mode3[] = {
	0xE7,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0xFF,
};

static char mDNIe_negative_mode4[] = {
	0xE8,
		0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x04,
};

static char mDNIe_negative_mode5[] = {
	0xE4,
		0x01, 0x00, 0x00, 0x33,
};


static char mDNIe_CAMERA_MODE1[] = {
	0xE5,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static char mDNIe_CAMERA_MODE2[] = {
	0xE6,
		0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,
		0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,
		0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,
};

static char mDNIe_CAMERA_MODE3[] = {
	0xE7,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0xFF,
};

static char mDNIe_CAMERA_MODE4[] = {
	0xE8,
		0xd7, 0x04, 0x4c, 0x1f, 0xdd, 0x1f, 0xa4, 0x1f,
		0x7f, 0x04, 0xdd, 0x1f, 0xa4, 0x1f, 0x4c, 0x1f,
		0x10, 0x05,
};

static char mDNIe_CAMERA_MODE5[] = {
	0xE4,
		0x01, 0x00, 0x06, 0x23,
};

static char mDNIe_CAMERA_OUTDOOR_MODE1[] = {
	0xE5,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static char mDNIe_CAMERA_OUTDOOR_MODE2[] = {
	0xE6,
		0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,
		0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,
		0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,
};

static char mDNIe_CAMERA_OUTDOOR_MODE3[] = {
	0xE7,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x0c, 0xae,
		0x0c, 0xae, 0x0c, 0xae, 0x0c, 0xae, 0x0c, 0xae,
		0x0c, 0xae, 0x26, 0xbe, 0x26, 0xbe, 0x26, 0xbe,
		0x26, 0xbe, 0x0f, 0xb4, 0x05, 0x2c, 0x48, 0x1b,
		0x6d, 0x14, 0x99, 0x0d, 0xaf, 0x0a, 0x00, 0xFF,
};

static char mDNIe_CAMERA_OUTDOOR_MODE4[] = {
	0xE8,
		0xf7, 0x05, 0x5b, 0x1e, 0xae, 0x1f, 0x2a, 0x1f,
		0x28, 0x05, 0xae, 0x1f, 0x2a, 0x1f, 0x5b, 0x1e,
		0x7b, 0x06,
};

static char mDNIe_CAMERA_OUTDOOR_MODE5[] = {
	0xE4,
		0x01, 0x00, 0x07, 0x23,
};

static char mDNIe_GALLERY_MODE1[] = {
	0xE5,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static char mDNIe_GALLERY_MODE2[] = {
	0xE6,
		0x00, 0xff, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
		0x00, 0xff, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
		0x00, 0xff, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
};

static char mDNIe_GALLERY_MODE3[] = {
	0xE7,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0xFF,
};

static char mDNIe_GALLERY_MODE4[] = {
	0xE8,
		0xd7, 0x04, 0x4c, 0x1f, 0xdd, 0x1f, 0xa4, 0x1f,
		0x7f, 0x04, 0xdd, 0x1f, 0xa4, 0x1f, 0x4c, 0x1f,
		0x10, 0x05,
};

static char mDNIe_GALLERY_MODE5[] = {
	0xE4,
		0x01, 0x00, 0x02, 0x23,
};

static char mDNIe_UI_MODE1[] = {
	0xE5,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static char mDNIe_UI_MODE2[] = {
	0xE6,
		0x00, 0xff, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
		0x00, 0xff, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
		0x00, 0xff, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
};

static char mDNIe_UI_MODE3[] = {
	0xE7,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0xFF,
};

static char mDNIe_UI_MODE4[] = {
	0xE8,
		0x67, 0x05, 0xd3, 0x1e, 0xc6, 0x1f, 0x67, 0x1f,
		0xd3, 0x04, 0xc6, 0x1f, 0x67, 0x1f, 0xd3, 0x1e,
		0xc6, 0x05,
};

static char mDNIe_UI_MODE5[] = {
	0xE4,
		0x01, 0x00, 0x02, 0x23,
};

static char mDNIe_VIDEO_MODE1[] = {
	0xE5,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static char mDNIe_VIDEO_MODE2[] = {
	0xE6,
		0x00, 0xff, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
		0x00, 0xff, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
		0x00, 0xff, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
};

static char mDNIe_VIDEO_MODE3[] = {
	0xE7,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0xFF,
};

static char mDNIe_VIDEO_MODE4[] = {
	0xE8,
		0xd7, 0x04, 0x4c, 0x1f, 0xdd, 0x1f, 0xa4, 0x1f,
		0x7f, 0x04, 0xdd, 0x1f, 0xa4, 0x1f, 0x4c, 0x1f,
		0x10, 0x05,
};

static char mDNIe_VIDEO_MODE5[] = {
	0xE4,
		0x01, 0x00, 0x06, 0x23,
};

static char mDNIe_VIDEO_COLD_MODE1[] = {
	0xE5,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static char mDNIe_VIDEO_COLD_MODE2[] = {
	0xE6,
		0x00, 0xff, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
		0x00, 0xff, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
		0x00, 0xff, 0xe1, 0x00, 0xed, 0x00, 0xff, 0x00,
};

static char mDNIe_VIDEO_COLD_MODE3[] = {
	0xE7,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0xFF,
};

static char mDNIe_VIDEO_COLD_MODE4[] = {
	0xE8,
		0xd7, 0x04, 0x4c, 0x1f, 0xdd, 0x1f, 0xa4, 0x1f,
		0x7f, 0x04, 0xdd, 0x1f, 0xa4, 0x1f, 0x4c, 0x1f,
		0x10, 0x05,
};

static char mDNIe_VIDEO_COLD_MODE5[] = {
	0xE4,
		0x01, 0x00, 0x06, 0x33,
};

static char mDNIe_VIDEO_COLD_OUTDOOR_MODE1[] = {
	0xE5,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static char mDNIe_VIDEO_COLD_OUTDOOR_MODE2[] = {
	0xE6,
		0x00, 0xff, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
		0x00, 0xff, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
		0x00, 0xff, 0xe1, 0x00, 0xed, 0x00, 0xff, 0x00,
};

static char mDNIe_VIDEO_COLD_OUTDOOR_MODE3[] = {
	0xE7,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x0c, 0xae,
		0x0c, 0xae, 0x0c, 0xae, 0x0c, 0xae, 0x0c, 0xae,
		0x0c, 0xae, 0x26, 0xbe, 0x26, 0xbe, 0x26, 0xbe,
		0x26, 0xbe, 0x0f, 0xb4, 0x05, 0x2c, 0x48, 0x1b,
		0x6d, 0x14, 0x99, 0x0d, 0xaf, 0x0a, 0x00, 0xFF,
};

static char mDNIe_VIDEO_COLD_OUTDOOR_MODE4[] = {
	0xE8,
		0xf7, 0x05, 0x5b, 0x1e, 0xae, 0x1f, 0x2a, 0x1f,
		0x28, 0x05, 0xae, 0x1f, 0x2a, 0x1f, 0x5b, 0x1e,
		0x7b, 0x06,
};

static char mDNIe_VIDEO_COLD_OUTDOOR_MODE5[] = {
	0xE4,
		0x01, 0x00, 0x07, 0x33,
};

static char mDNIe_VIDEO_OUTDOOR_MODE1[] = {
	0xE5,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static char mDNIe_VIDEO_OUTDOOR_MODE2[] = {
	0xE6,
		0x00, 0xff, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
		0x00, 0xff, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
		0x00, 0xff, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
};

static char mDNIe_VIDEO_OUTDOOR_MODE3[] = {
	0xE7,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x0c, 0xae,
		0x0c, 0xae, 0x0c, 0xae, 0x0c, 0xae, 0x0c, 0xae,
		0x0c, 0xae, 0x26, 0xbe, 0x26, 0xbe, 0x26, 0xbe,
		0x26, 0xbe, 0x0f, 0xb4, 0x05, 0x2c, 0x48, 0x1b,
		0x6d, 0x14, 0x99, 0x0d, 0xaf, 0x0a, 0x00, 0xFF,
};

static char mDNIe_VIDEO_OUTDOOR_MODE4[] = {
	0xE8,
		0xf7, 0x05, 0x5b, 0x1e, 0xae, 0x1f, 0x2a, 0x1f,
		0x28, 0x05, 0xae, 0x1f, 0x2a, 0x1f, 0x5b, 0x1e,
		0x7b, 0x06,
};

static char mDNIe_VIDEO_OUTDOOR_MODE5[] = {
	0xE4,
		0x01, 0x00, 0x07, 0x23,
};

static char mDNIe_VIDEO_WARM_MODE1[] = {
	0xE5,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static char mDNIe_VIDEO_WARM_MODE2[] = {
	0xE6,
		0x00, 0xff, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
		0x00, 0xff, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
		0x00, 0xff, 0xff, 0x00, 0xf3, 0x00, 0xe3, 0x00,
};

static char mDNIe_VIDEO_WARM_MODE3[] = {
	0xE7,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0xFF,
};

static char mDNIe_VIDEO_WARM_MODE4[] = {
	0xE8,
		0xd7, 0x04, 0x4c, 0x1f, 0xdd, 0x1f, 0xa4, 0x1f,
		0x7f, 0x04, 0xdd, 0x1f, 0xa4, 0x1f, 0x4c, 0x1f,
		0x10, 0x05,
};

static char mDNIe_VIDEO_WARM_MODE5[] = {
	0xE4,
		0x01, 0x00, 0x06, 0x33,
};

static char mDNIe_VIDEO_WARM_OUTDOOR_MODE1[] = {
	0xE5,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static char mDNIe_VIDEO_WARM_OUTDOOR_MODE2[] = {
	0xE6,
		0x00, 0xff, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
		0x00, 0xff, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
		0x00, 0xff, 0xff, 0x00, 0xf3, 0x00, 0xe3, 0x00,
};

static char mDNIe_VIDEO_WARM_OUTDOOR_MODE3[] = {
	0xE7,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
		0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x0c, 0xae,
		0x0c, 0xae, 0x0c, 0xae, 0x0c, 0xae, 0x0c, 0xae,
		0x0c, 0xae, 0x26, 0xbe, 0x26, 0xbe, 0x26, 0xbe,
		0x26, 0xbe, 0x0f, 0xb4, 0x05, 0x2c, 0x48, 0x1b,
		0x6d, 0x14, 0x99, 0x0d, 0xaf, 0x0a, 0x00, 0xFF,
};

static char mDNIe_VIDEO_WARM_OUTDOOR_MODE4[] = {
	0xE8,
		0xf7, 0x05, 0x5b, 0x1e, 0xae, 0x1f, 0x2a, 0x1f,
		0x28, 0x05, 0xae, 0x1f, 0x2a, 0x1f, 0x5b, 0x1e,
		0x7b, 0x06,
};

static char mDNIe_VIDEO_WARM_OUTDOOR_MODE5[] = {
	0xE4,
		0x01, 0x00, 0x07, 0x33,
};


static struct dsi_cmd_desc nt35502_video_display_mDNIe_scenario_cmds_neg[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(video55), video55},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_negative_mode1), mDNIe_negative_mode1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_negative_mode2), mDNIe_negative_mode2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_negative_mode3), mDNIe_negative_mode3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_negative_mode4), mDNIe_negative_mode4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_negative_mode5), mDNIe_negative_mode5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(video49), video49},

};

static struct dsi_cmd_desc nt35502_video_display_mDNIe_scenario_cmds_ui[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(video55), video55},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_UI_MODE1), mDNIe_UI_MODE1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_UI_MODE2), mDNIe_UI_MODE2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_UI_MODE3), mDNIe_UI_MODE3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_UI_MODE4), mDNIe_UI_MODE4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_UI_MODE5), mDNIe_UI_MODE5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(video49), video49},
};

static struct dsi_cmd_desc
	nt35502_video_display_mDNIe_scenario_cmds_gallery[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(video55), video55},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_GALLERY_MODE1), mDNIe_GALLERY_MODE1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_GALLERY_MODE2), mDNIe_GALLERY_MODE2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_GALLERY_MODE3), mDNIe_GALLERY_MODE3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_GALLERY_MODE4), mDNIe_GALLERY_MODE4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_GALLERY_MODE5), mDNIe_GALLERY_MODE5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(video49), video49},
};

static struct
	dsi_cmd_desc nt35502_video_display_mDNIe_scenario_cmds_video[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(video55), video55},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_VIDEO_MODE1), mDNIe_VIDEO_MODE1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_VIDEO_MODE2), mDNIe_VIDEO_MODE2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_VIDEO_MODE3), mDNIe_VIDEO_MODE3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_VIDEO_MODE4), mDNIe_VIDEO_MODE4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_VIDEO_MODE5), mDNIe_VIDEO_MODE5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(video49), video49},
};

static struct
	dsi_cmd_desc nt35502_video_display_mDNIe_scenario_cmds_video_out[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(video55), video55},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_VIDEO_OUTDOOR_MODE1), mDNIe_VIDEO_OUTDOOR_MODE1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_VIDEO_OUTDOOR_MODE2), mDNIe_VIDEO_OUTDOOR_MODE2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_VIDEO_OUTDOOR_MODE3), mDNIe_VIDEO_OUTDOOR_MODE3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_VIDEO_OUTDOOR_MODE4), mDNIe_VIDEO_OUTDOOR_MODE4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_VIDEO_OUTDOOR_MODE5), mDNIe_VIDEO_OUTDOOR_MODE5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(video49), video49},
};

static struct dsi_cmd_desc
	nt35502_video_display_mDNIe_scenario_cmds_video_warm[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(video55), video55},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_VIDEO_WARM_MODE1), mDNIe_VIDEO_WARM_MODE1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_VIDEO_WARM_MODE2), mDNIe_VIDEO_WARM_MODE2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_VIDEO_WARM_MODE3), mDNIe_VIDEO_WARM_MODE3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_VIDEO_WARM_MODE4), mDNIe_VIDEO_WARM_MODE4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_VIDEO_WARM_MODE5), mDNIe_VIDEO_WARM_MODE5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(video49), video49},
};

static struct dsi_cmd_desc
	nt35502_video_display_mDNIe_scenario_cmds_video_warm_out[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(video55), video55},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_VIDEO_WARM_OUTDOOR_MODE1),
		mDNIe_VIDEO_WARM_OUTDOOR_MODE1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_VIDEO_WARM_OUTDOOR_MODE2),
		mDNIe_VIDEO_WARM_OUTDOOR_MODE2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_VIDEO_WARM_OUTDOOR_MODE3),
		mDNIe_VIDEO_WARM_OUTDOOR_MODE3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_VIDEO_WARM_OUTDOOR_MODE4),
		mDNIe_VIDEO_WARM_OUTDOOR_MODE4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_VIDEO_WARM_OUTDOOR_MODE5),
		mDNIe_VIDEO_WARM_OUTDOOR_MODE5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(video49), video49},
};

static struct dsi_cmd_desc
	nt35502_video_display_mDNIe_scenario_cmds_video_cold[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(video55), video55},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_VIDEO_COLD_MODE1), mDNIe_VIDEO_COLD_MODE1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_VIDEO_COLD_MODE2), mDNIe_VIDEO_COLD_MODE2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_VIDEO_COLD_MODE3), mDNIe_VIDEO_COLD_MODE3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_VIDEO_COLD_MODE4), mDNIe_VIDEO_COLD_MODE4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_VIDEO_COLD_MODE5), mDNIe_VIDEO_COLD_MODE5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(video49), video49},
};

static struct dsi_cmd_desc
	nt35502_video_display_mDNIe_scenario_cmds_video_cold_out[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(video55), video55},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_VIDEO_COLD_OUTDOOR_MODE1), mDNIe_VIDEO_COLD_OUTDOOR_MODE1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_VIDEO_COLD_OUTDOOR_MODE2), mDNIe_VIDEO_COLD_OUTDOOR_MODE2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_VIDEO_COLD_OUTDOOR_MODE3), mDNIe_VIDEO_COLD_OUTDOOR_MODE3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_VIDEO_COLD_OUTDOOR_MODE4), mDNIe_VIDEO_COLD_OUTDOOR_MODE4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_VIDEO_COLD_OUTDOOR_MODE5), mDNIe_VIDEO_COLD_OUTDOOR_MODE5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(video49), video49},
};

static struct dsi_cmd_desc
	nt35502_video_display_mDNIe_scenario_cmds_camera[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(video55), video55},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_CAMERA_MODE1), mDNIe_CAMERA_MODE1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_CAMERA_MODE2), mDNIe_CAMERA_MODE2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_CAMERA_MODE3), mDNIe_CAMERA_MODE3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_CAMERA_MODE4), mDNIe_CAMERA_MODE4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_CAMERA_MODE5), mDNIe_CAMERA_MODE5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(video49), video49},
};

static struct dsi_cmd_desc
	nt35502_video_display_mDNIe_scenario_cmds_camera_out[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(video55), video55},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_CAMERA_OUTDOOR_MODE1), mDNIe_CAMERA_OUTDOOR_MODE1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_CAMERA_OUTDOOR_MODE2), mDNIe_CAMERA_OUTDOOR_MODE2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_CAMERA_OUTDOOR_MODE3), mDNIe_CAMERA_OUTDOOR_MODE3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_CAMERA_OUTDOOR_MODE4), mDNIe_CAMERA_OUTDOOR_MODE4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(mDNIe_CAMERA_OUTDOOR_MODE5), mDNIe_CAMERA_OUTDOOR_MODE5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, NT35502_CMD_SETTLE,
		sizeof(video49), video49},
};
#endif

#if 0
static struct dsi_cmd_desc nt35502_video_display_on_cmds_rotate[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 150,
		sizeof(config_video_MADCTL), config_video_MADCTL},
};
#endif
extern unsigned int board_hw_revision;
#define LCDC_DEBUG

#ifdef LCDC_DEBUG
#define DPRINT(x...)	printk(KERN_ERR "NT35502 " x)
#else
#define DPRINT(x...)
#endif

#if defined(CONFIG_FB_MSM_MIPI_CMD_PANEL_AVOID_MOSAIC) \
	|| defined(CONFIG_MACH_KYLEPLUS_OPEN) \
	|| defined(CONFIG_MACH_INFINITE_DUOS_CTC)
static void mipi_nt35502_unblank(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;

	mfd = platform_get_drvdata(pdev);
	mipi  = &mfd->panel_info.mipi;

	DPRINT("%s +:\n", __func__);
	/*
	if (mipi->mode == DSI_CMD_MODE) {
		mipi_dsi_cmd_mdp_busy();
		mipi_dsi_cmds_tx(&nt35502_tx_buf, nt35502_cmd_unblank,
			ARRAY_SIZE(nt35502_cmd_unblank));
	}*/
	DPRINT("%s -:\n", __func__);
}
#endif

static int first_time_disp_on = 1;
static int mipi_nt35502_lcd_on(struct platform_device *pdev)
{
/*
	//struct msm_fb_data_type *mfd;
	//int ret = 0;
*/
	struct mipi_panel_info *mipi;
#if 0
	int rotate = 1;
#endif

	DPRINT("start %s\n", __func__);

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;

	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi  = &mfd->panel_info.mipi;
	if (is_lcd_on)
		backlight_ic_set_brightness(0);
#if 0
#if (1 == NT35502_USE_DEEP_STANDBY)
		nt35502_disp_exit_deep_standby();
#endif /* #if (1 == NT35502_USE_DEEP_STANDBY) */
#if (1 == NT35502_USE_POWERDOWN)
		nt35502_disp_powerup();
#endif /* (1 == NT35502_USE_POWERDOWN) */
#endif
	if (!mfd->cont_splash_done) {
		mfd->cont_splash_done = 1;
		return 0;
	}

	if (!first_time_disp_on) {
		/* DPRINT("[LCD] not first time %s\n", __func__); */
		msleep(10);
		gpio_set_value_cansleep(23, 0);
		msleep(10);
		gpio_set_value_cansleep(23, 1);
		msleep(150);
	} else {
		/* DPRINT("[LCD] first time %s\n", __func__); */
		first_time_disp_on = 0;
	}
#if 0
	if (mipi_nt35502_pdata && mipi_nt35502_pdata->rotate_panel)
		rotate = mipi_nt35502_pdata->rotate_panel();
#endif
	/*if (mipi->mode == DSI_VIDEO_MODE)*/
	{
		mipi_dsi_cmds_tx(&nt35502_tx_buf,
			nt35502_video_display_on_cmds,
			ARRAY_SIZE(nt35502_video_display_on_cmds));

#if 0
		if (rotate) {
			mipi_dsi_cmds_tx(&nt35502_tx_buf,
				nt35502_video_display_on_cmds_rotate,
			ARRAY_SIZE(nt35502_video_display_on_cmds_rotate));
		}
#endif
	}
	/*else if (mipi->mode == DSI_CMD_MODE) {

		ret = mipi_dsi_cmds_tx(&nt35502_tx_buf,
			nt35502_cmd_display_on_kyleplus_exit_sleep,
			ARRAY_SIZE(nt35502_cmd_display_on_kyleplus_exit_sleep));
		if(ret==0)
			goto MIPI_ERROR;
		mdelay(5);
		ret = mipi_dsi_cmds_tx(&nt35502_tx_buf,
			nt35502_cmd_display_on_kyleplus1,
			ARRAY_SIZE(nt35502_cmd_display_on_kyleplus1));
		if(ret==0)
			goto MIPI_ERROR;
		ret = mipi_dsi_cmds_tx(&nt35502_tx_buf,
			nt35502_cmd_display_on_kyleplus2,
				ARRAY_SIZE(nt35502_cmd_display_on_kyleplus2));
		if(ret==0)
			goto MIPI_ERROR;
		if (rotate) {
			ret = mipi_dsi_cmds_tx(&nt35502_tx_buf,
				nt35502_cmd_display_on_cmds_rotate,
			ARRAY_SIZE(nt35502_cmd_display_on_cmds_rotate));
			if(ret==0)
			goto MIPI_ERROR;
		}
		ret = mipi_dsi_cmds_tx(&nt35502_tx_buf,
			nt35502_cmd_display_on_kyleplus3,
			ARRAY_SIZE(nt35502_cmd_display_on_kyleplus3));
		if(ret==0)
			goto MIPI_ERROR;

#if defined(CONFIG_FB_MSM_MIPI_CMD_PANEL_AVOID_MOSAIC) \
	|| defined(CONFIG_MACH_KYLEPLUS_OPEN) \
	|| defined(CONFIG_MACH_INFINITE_DUOS_CTC)
		// delay to avoid flash caused by display_on command,
		// it work with msm_fb_blank_sub() backlight protection
		msleep(160);
#endif
	}
	*/
	is_lcd_on = 1;
#if defined(CONFIG_APPLY_MDNIE)
	set_mDNIe_Mode(&mDNIe_cfg);
#endif
#if defined(CONFIG_MDNIE_TUNING)
	mDNIe_tx_tuning();
#endif

	backlight_ic_set_brightness(mfd->bl_level);

	DPRINT("exit %s\n", __func__);
	return 0;

/*
MIPI_ERROR:
	DPRINT("%s MIPI_ERROR -\n", __func__);
	return ret;
	*/
}
static int mipi_nt35502_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
/*
	//static int b_first_off = 1;
*/
	DPRINT("start %s\n", __func__);

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

#if !defined(CONFIG_MACH_KYLEPLUS_CTC) \
	&& !defined(CONFIG_MACH_KYLEPLUS_OPEN) \
	&& !defined(CONFIG_MACH_INFINITE_DUOS_CTC)
	backlight_ic_set_brightness(0);
#endif
#if 0 /* VE_GROUP*/
	if (b_first_off) { /* workaround.*/
		nt35502_disp_exit_deep_standby();

		msleep(20);

		mipi_dsi_cmds_tx(&nt35502_tx_buf,
			nt35502_cmd_display_on_kyleplus_exit_sleep,
			ARRAY_SIZE(nt35502_cmd_display_on_kyleplus_exit_sleep));
		mdelay(120);
		mipi_dsi_cmds_tx(&nt35502_tx_buf,
			nt35502_cmd_display_on_kyleplus1,
			ARRAY_SIZE(nt35502_cmd_display_on_kyleplus1));
		mdelay(120);
		mipi_dsi_cmds_tx(&nt35502_tx_buf,
			nt35502_cmd_display_on_kyleplus2,
			ARRAY_SIZE(nt35502_cmd_display_on_kyleplus2));
		if (1) {
			mipi_dsi_cmds_tx(&nt35502_tx_buf,
				nt35502_cmd_display_on_cmds_rotate,
				ARRAY_SIZE(nt35502_cmd_display_on_cmds_rotate));
		}
		mipi_dsi_cmds_tx(&nt35502_tx_buf,
			nt35502_cmd_display_on_kyleplus3,
			ARRAY_SIZE(nt35502_cmd_display_on_kyleplus3));
		mdelay(10);

		msleep(20);
		b_first_off = 0;
	}
#endif

	is_lcd_on = 0;

#if defined(CONFIG_FB_MSM_MIPI_CMD_PANEL_AVOID_MOSAIC) \
	|| defined(CONFIG_MACH_KYLEPLUS_OPEN) \
	|| defined(CONFIG_MACH_INFINITE_DUOS_CTC)
	/* let screen light off smoothly,
		because backlight is turning off in another thread now */
	msleep(80);
#endif

	mipi_dsi_cmds_tx(&nt35502_tx_buf, nt35502_display_off_cmds,
			ARRAY_SIZE(nt35502_display_off_cmds));

	mipi_dsi_cmds_tx(&nt35502_tx_buf, nt35502_deep_standby_cmds,
			ARRAY_SIZE(nt35502_deep_standby_cmds));

#if 0
#if (1 == NT35502_USE_DEEP_STANDBY)
	nt35502_disp_enter_deep_standby(mfd, &nt35502_tx_buf);
#endif /* (1 == NT35502_USE_DEEP_STANDBY) */
#if (1 == NT35502_USE_POWERDOWN)
	nt35502_disp_powerdown();
#endif /* (1 == NT35502_USE_POWERDOWN) */
#endif
	mipi_set_tx_power_mode(1);	/* Low Speed Power*/

	DPRINT("exit %s\n", __func__);
	return 0;
}



static ssize_t mipi_nt35502_lcdtype_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
		char temp[20];
		sprintf(temp, "INH_558CC0\n");
		strncat(buf, temp, 11);
		return strlen(buf);
}

/* FIXME: add handlers */
static struct lcd_ops mipi_lcd_props;
static DEVICE_ATTR(lcd_type, S_IRUGO, mipi_nt35502_lcdtype_show, NULL);

#if defined(CONFIG_MDNIE_TUNING)

static int mDNIe_tx_tuning()
{
	int i;

	if (!tuning_enable)
		return 0;

	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);

	mipi_dsi_cmds_tx(&nt35502_tx_buf,
		nt35502_cmd2_enable_cmds,
		ARRAY_SIZE(nt35502_cmd2_enable_cmds));

	for (i = 0; i < cmd_count; i++) {
		if (i == 0) {
			nt35502_cmd_mDNIe_tune_cmds[0].dlen
				= cmd_length_ary[i]+1;
			nt35502_cmd_mDNIe_tune_cmds[0].payload
				= mDNIe_data;
		} else {
			nt35502_cmd_mDNIe_tune_cmds[0].dlen
				= cmd_length_ary[i]-cmd_length_ary[i-1];
			nt35502_cmd_mDNIe_tune_cmds[0].payload
				= &(mDNIe_data[cmd_length_ary[i-1]])+1;

		}
		DPRINT("%s, %d, i=%d, dlen = %d, payload num = %d\n",
			__func__, __LINE__, i,
			nt35502_cmd_mDNIe_tune_cmds[0].dlen,
			cmd_length_ary[i-1]+1);

		mutex_lock(&lcdc_mlock);

		mipi_dsi_cmds_tx(&nt35502_tx_buf,
			nt35502_cmd_mDNIe_tune_cmds,
			ARRAY_SIZE(nt35502_cmd_mDNIe_tune_cmds));

		mutex_unlock(&lcdc_mlock);

	}

	mipi_dsi_cmds_tx(&nt35502_tx_buf,
		nt35502_cmd2_disable_cmds,
		ARRAY_SIZE(nt35502_cmd2_disable_cmds));

	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);

	/*
	mutex_lock(&lcdc_mlock);

	mipi_dsi_cmds_tx(&nt35502_tx_buf,
		nt35502_extra_cmds,
		ARRAY_SIZE(nt35502_extra_cmds));

	mutex_unlock(&lcdc_mlock);
	*/


	return 0;
}

static int parse_text(char *src, int len)
{
	int i;
	int data = 0, value = 0, count = 0, comment = 0, newline = 0;
	char *cur_position;
	cmd_count = 0;

/*DNIe_data[count] = 0xE6;*/
/*ount++;*/
	cur_position = src;
	for (i = 0; i < len; i++, cur_position++) {
		char a = *cur_position;
		switch (a) {
		case '\n':
			if (!newline)
				newline = 1;
			else {
				cmd_length_ary[cmd_count++] = count-1;
				newline = 0;
			}
		case '\r':
			comment = 0;
			data = 0;
			break;
		case '/':
			newline = 0;
			comment++;
			data = 0;
			break;
		case '0'...'9':
			newline = 0;
			if (comment > 1)
				break;
			if (data == 0 && a == '0')
				data = 1;
			else if (data == 2) {
				data = 3;
				value = (a-'0')*16;
			} else if (data == 3) {
				value += (a-'0');
				mDNIe_data[count] = value;
				DPRINT("Tuning value[%d]=0x%02X\n",
					count, value);
				count++;
				data = 0;
			}
			break;
		case 'a'...'f':
		case 'A'...'F':
			newline = 0;
			if (comment > 1)
				break;
			if (data == 2) {
				data = 3;
				if (a < 'a')
					value = (a-'A'+10)*16;
				else
					value = (a-'a'+10)*16;
			} else if (data == 3) {
				if (a < 'a')
					value += (a-'A'+10);
				else
					value += (a-'a'+10);
				mDNIe_data[count] = value;
				DPRINT("Tuning value[%d]=0x%02X\n",
					count, value);
				count++;
				data = 0;
			}
			break;
		case 'x':
		case 'X':
			newline = 0;
			if (data == 1)
				data = 2;
			break;
		default:
			newline = 0;
			if (comment == 1)
				comment = 0;
			data = 0;
			break;
		}
	}
	cmd_length_ary[cmd_count++] = count-1;
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

	DPRINT("[INFO]:%s called loading file name : [%s]\n",
		__func__, filename);

	fs = get_fs();
	set_fs(get_ds());

	filp = filp_open(filename, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		printk(KERN_ERR "[ERROR]:File open failed\n");
		return -1;
	}

	l = filp->f_path.dentry->d_inode->i_size;
	DPRINT("[INFO]: Loading File Size : %ld(bytes)", l);

	dp = kmalloc(l+10, GFP_KERNEL);
	if (dp == NULL) {
		DPRINT("[ERROR]:Can't not alloc memory for tuning file load\n");
		filp_close(filp, current->files);
		return -1;
	}
	pos = 0;
	memset(dp, 0, l);
	DPRINT("[INFO] : before vfs_read()\n");
	ret = vfs_read(filp, (char __user *)dp, l, &pos);
	DPRINT("[INFO] : after vfs_read()\n");

	if (ret != l) {
		DPRINT("[ERROR] : vfs_read() filed ret : %d\n", ret);
		kfree(dp);
		filp_close(filp, current->files);
		return -1;
	}

	filp_close(filp, current->files);

	set_fs(fs);
	num = parse_text(dp, l);
/*	nt35502_cmd_mDNIe_tune_cmds[0].dlen = num;*/

	if (!num) {
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
	/* char *bf; */
	int size = 4;
	int i;
	/* struct msm_fb_data_type *mfd; */

	ret = sprintf(buf, "Tunned File Name : %s\n", tuning_filename);


	/* mfd = platform_get_drvdata(pdevp); */
	/* if (!mfd)
		return -ENODEV;

	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi_dsi_cmds_tx(&nt35502_tx_buf,
		nt35502_cmd2_enable_cmds,
		ARRAY_SIZE(nt35502_cmd2_enable_cmds));

	mipi_dsi_cmds_rx(mfd, &nt35502_tx_buf, &nt35502_rx_buf,
		nt35502_video_read_mDNIe_cmds, size);


    DPRINT("Tuning Value of E4 : ");
	//sprintf(bf,"Tuning Value of E4 : ");
	for (i=0;i<size;i++)
		DPRINT("%X ", nt35502_rx_buf.data[i]);
		//sprintf(bf,"%X ", nt35502_rx_buf.data[i]);
	DPRINT("\n ");

	mipi_dsi_cmds_tx(&nt35502_tx_buf,
		nt35502_cmd2_disable_cmds,
		ARRAY_SIZE(nt35502_cmd2_disable_cmds));

	//sprintf(bf,"Tuning Value of E4 : %s\n", nt35502_rx_buf.data);
	*/

	return ret;
}

static ssize_t mDNIeTuning_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	char *pt;
	char a;
	/* unsigned long tunning_mode = 0; */

	a = *buf;

	if (a == '1') {
		tuning_enable = 1;
		DPRINT("%s:Tuning_enable\n", __func__);
	} else if (a == '0') {
		tuning_enable = 0;
		DPRINT("%s:Tuning_disable\n", __func__);
	} else {
		memset(tuning_filename, 0, sizeof(tuning_filename));
		sprintf(tuning_filename, "%s%s", TUNING_FILE_PATH, buf);

		pt = tuning_filename;
		while (*pt) {
			if (*pt == '\r' || *pt == '\n') {
				*pt = 0;
				break;
			}
			pt++;
		}
		DPRINT("%s:%s\n", __func__, tuning_filename);

		if (load_tuning_data(tuning_filename) <= 0) {
			DPRINT("[ERROR]:load_tunig_data() failed\n");
			return size;
		}

	tuning_enable = 1;
	mDNIe_tx_tuning();

	}
	return size;
}

static DEVICE_ATTR(tuning, 0664, mDNIeTuning_show, mDNIeTuning_store);
#endif

#if defined(CONFIG_APPLY_MDNIE)
static void set_mDNIe_Mode(struct mdnie_config *mDNIeCfg)
{
	int value;
	DPRINT("%s +:[mDNIe] negative=%d\n", __func__, mDNIe_cfg.negative);
	if (!is_lcd_on)
		return;

	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);

	if (mDNIe_cfg.negative) {
		mutex_lock(&lcdc_mlock);
		mipi_dsi_cmds_tx(&nt35502_tx_buf,
			nt35502_video_display_mDNIe_scenario_cmds_neg,
			ARRAY_SIZE
				(nt35502_video_display_mDNIe_scenario_cmds_neg));
		mutex_unlock(&lcdc_mlock);

		mDNIeCfg->curIndex = SCENARIO_MAX;

		MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);
		return;
	}


	switch (mDNIeCfg->scenario) {
	case UI_MODE:

		mutex_lock(&lcdc_mlock);
		mipi_dsi_cmds_tx(&nt35502_tx_buf,
			nt35502_video_display_mDNIe_scenario_cmds_ui,
			ARRAY_SIZE
				(nt35502_video_display_mDNIe_scenario_cmds_ui));
		mutex_unlock(&lcdc_mlock);

		mDNIeCfg->curIndex = value;

		break;

	case GALLERY_MODE:
		value = mDNIeCfg->scenario;

		mutex_lock(&lcdc_mlock);
		mipi_dsi_cmds_tx(&nt35502_tx_buf,
			nt35502_video_display_mDNIe_scenario_cmds_gallery,
			ARRAY_SIZE
				(nt35502_video_display_mDNIe_scenario_cmds_gallery));
		mutex_unlock(&lcdc_mlock);

		mDNIeCfg->curIndex = value;

		break;

	case VIDEO_MODE:
		if (mDNIeCfg->outdoor == OUTDOOR_ON) {
			value = SCENARIO_MAX + 1;

			mutex_lock(&lcdc_mlock);
		mipi_dsi_cmds_tx(&nt35502_tx_buf,
			nt35502_video_display_mDNIe_scenario_cmds_video_out,
			ARRAY_SIZE
				(nt35502_video_display_mDNIe_scenario_cmds_video_out));
		mutex_unlock(&lcdc_mlock);

		    mDNIeCfg->curIndex = value;

		} else {
			value = mDNIeCfg->scenario;

			mutex_lock(&lcdc_mlock);
			mipi_dsi_cmds_tx(&nt35502_tx_buf,
				nt35502_video_display_mDNIe_scenario_cmds_video,
				ARRAY_SIZE
					(nt35502_video_display_mDNIe_scenario_cmds_video));
			mutex_unlock(&lcdc_mlock);

			mDNIeCfg->curIndex = value;
		}
		break;

	case VIDEO_WARM_MODE:
		if (mDNIeCfg->outdoor == OUTDOOR_ON) {
			value = SCENARIO_MAX + 2;

			mutex_lock(&lcdc_mlock);
			mipi_dsi_cmds_tx(&nt35502_tx_buf,
				nt35502_video_display_mDNIe_scenario_cmds_video_warm_out,
				ARRAY_SIZE
					(nt35502_video_display_mDNIe_scenario_cmds_video_warm_out));
			mutex_unlock(&lcdc_mlock);

			mDNIeCfg->curIndex = value;

		} else {

			mutex_lock(&lcdc_mlock);
			mipi_dsi_cmds_tx(&nt35502_tx_buf,
				nt35502_video_display_mDNIe_scenario_cmds_video_warm,
				ARRAY_SIZE
					(nt35502_video_display_mDNIe_scenario_cmds_video_warm));
			mutex_unlock(&lcdc_mlock);

			value = mDNIeCfg->scenario;
		}
		break;

	case VIDEO_COLD_MODE:
		if (mDNIeCfg->outdoor == OUTDOOR_ON) {
			value = SCENARIO_MAX + 3;

			mutex_lock(&lcdc_mlock);
			mipi_dsi_cmds_tx(&nt35502_tx_buf,
				nt35502_video_display_mDNIe_scenario_cmds_video_cold_out,
				ARRAY_SIZE
					(nt35502_video_display_mDNIe_scenario_cmds_video_cold_out));
			mutex_unlock(&lcdc_mlock);

			mDNIeCfg->curIndex = value;

		} else {
			value = mDNIeCfg->scenario;

			mutex_lock(&lcdc_mlock);
		mipi_dsi_cmds_tx(&nt35502_tx_buf,
			nt35502_video_display_mDNIe_scenario_cmds_video_cold,
			ARRAY_SIZE(nt35502_video_display_mDNIe_scenario_cmds_video_cold));
		mutex_unlock(&lcdc_mlock);

		mDNIeCfg->curIndex = value;
		}
		break;

	case CAMERA_MODE:
		if (mDNIeCfg->outdoor == OUTDOOR_ON) {
			value = SCENARIO_MAX + 4;

			mutex_lock(&lcdc_mlock);
			mipi_dsi_cmds_tx(&nt35502_tx_buf,
				nt35502_video_display_mDNIe_scenario_cmds_camera_out,
				ARRAY_SIZE
					(nt35502_video_display_mDNIe_scenario_cmds_camera_out));
			mutex_unlock(&lcdc_mlock);

			mDNIeCfg->curIndex = value;

		} else {
			value = mDNIeCfg->scenario;

			mutex_lock(&lcdc_mlock);
			mipi_dsi_cmds_tx(&nt35502_tx_buf,
				nt35502_video_display_mDNIe_scenario_cmds_camera,
				ARRAY_SIZE
					(nt35502_video_display_mDNIe_scenario_cmds_camera));
			mutex_unlock(&lcdc_mlock);

		mDNIeCfg->curIndex = value;
		}
		break;

	default:
		value = UI_MODE;

		mutex_lock(&lcdc_mlock);
		mipi_dsi_cmds_tx(&nt35502_tx_buf,
			nt35502_video_display_mDNIe_scenario_cmds_ui,
			ARRAY_SIZE
				(nt35502_video_display_mDNIe_scenario_cmds_ui));
		mutex_unlock(&lcdc_mlock);

		mDNIeCfg->curIndex = value;
		break;
	};

	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);

	/* DPRINT("%s:[mDNIe] value=%d\n", __func__, value); */

    /*
	if(mDNIeCfg->curIndex != value) {
		mutex_lock(&lcdc_mlock);
		mipi_dsi_cmds_tx(&nt35502_tx_buf,
			nt35502_video_display_mDNIe_scenario_cmds[value],
			ARRAY_SIZE
				(nt35502_video_display_mDNIe_scenario_cmds[value]));
			mDNIeCfg->curIndex = value;
		mutex_unlock(&lcdc_mlock);
	}
	*/


    /*
	mipi_dsi_cmd_mdp_busy();

	if (mDNIe_cfg.negative)
		mipi_dsi_cmds_tx
			(&nt35502_tx_buf, &nt35502_cmd_inversion_on_cmds, 1);
	else
		mipi_dsi_cmds_tx
			(&nt35502_tx_buf, &nt35502_cmd_inversion_off_cmds, 1);

	DPRINT("%s -:[mDNIe]", __func__);
	*/


}


static ssize_t mDNIeScenario_show(struct device *dev,
	struct device_attribute *attr, char *buf)

{
	int ret = 0;
	ret = sprintf(buf, "mDNIeScenario_show : %d\n", mDNIe_cfg.scenario);

	return ret;
}

static ssize_t mDNIeScenario_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned int value;
	int ret;

	ret = strict_strtoul(buf, 0, (unsigned long *)&value);

	DPRINT("%s:value=%d\n", __func__, value);


	switch (value) {
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


/*//	mipi_set_tx_power_mode(0);	// High Speed Power */
	set_mDNIe_Mode(&mDNIe_cfg);
/*//	mipi_set_tx_power_mode(1);	// Low Speed Power */

	return size;
}

static DEVICE_ATTR(scenario, 0664, mDNIeScenario_show, mDNIeScenario_store);


static ssize_t mDNIeOutdoor_show(struct device *dev,
	struct device_attribute *attr, char *buf)

{
	int ret = 0;
	ret = sprintf(buf, "mDNIeOutdoor_show : %d\n", mDNIe_cfg.outdoor);

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

/* //	mipi_set_tx_power_mode(0);	// High Speed Power */
	set_mDNIe_Mode(&mDNIe_cfg);
/* //	mipi_set_tx_power_mode(1);	// Low Speed Power */

	return size;
}

static DEVICE_ATTR(outdoor, 0664, mDNIeOutdoor_show, mDNIeOutdoor_store);

static ssize_t mDNIeNegative_show(struct device *dev,
	struct device_attribute *attr, char *buf)

{
	int ret = 0;
	ret = sprintf(buf, "mDNIeNegative_show : %d\n", mDNIe_cfg.negative);

	return ret;
}
static ssize_t mDNIeNegative_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned int value;
	int ret;

	ret = strict_strtoul(buf, 0, (unsigned long *)&value);

	DPRINT("%s:value=%d\n", __func__, value);

	if (value == 1)
		mDNIe_cfg.negative = 1;
	else
		mDNIe_cfg.negative = 0;

	set_mDNIe_Mode(&mDNIe_cfg);
	return size;
}

static DEVICE_ATTR(negative, 0664, mDNIeNegative_show, mDNIeNegative_store);
#endif

static int __devinit mipi_nt35502_lcd_probe(struct platform_device *pdev)
{
	struct lcd_device *lcd_device;
	struct msm_fb_panel_data *pdata;
	int ret;
#if defined(CONFIG_APPLY_MDNIE)
	struct class *mdnie_class;
	struct device *mdnie_device;
#endif

	DPRINT("%s\n", __func__);

	if (pdev->id == 0) {
		mipi_nt35502_pdata = pdev->dev.platform_data;
		return 0;
	}

#if defined(CONFIG_MDNIE_TUNING) || defined(CONFIG_APPLY_MDNIE)
	mutex_init(&lcdc_mlock);
#endif

	msm_fb_add_device(pdev);
	lcd_device = lcd_device_register("panel", &pdev->dev, NULL,
					&mipi_lcd_props);

#if 0
	pdata = pdev->dev.platform_data;
	if (mipi_nt35502_pdata && mipi_nt35502_pdata->rotate_panel()
			&& pdata->panel_info.type == MIPI_CMD_PANEL)
		pdata->panel_info.lcd.refx100 = 6200;
#endif

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
	if (ret < 0)
		printk(KERN_ERR "sysfs create fail - %s\n",
	dev_attr_scenario.attr.name);

	ret = device_create_file(mdnie_device, &dev_attr_outdoor);
	if (ret < 0)
		printk(KERN_ERR "sysfs create fail - %s\n",
	dev_attr_outdoor.attr.name);


	ret = device_create_file(mdnie_device, &dev_attr_negative);
	if (ret < 0)
		printk(KERN_ERR "sysfs create fail - %s\n",
		dev_attr_negative.attr.name);
#endif

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_nt35502_lcd_probe,
	.driver = {
		.name   = "mipi_NT35502",
	},
};


static void mipi_nt35502_set_backlight(struct msm_fb_data_type *mfd)
{
	int level = mfd->bl_level;

#if !defined(CONFIG_MACH_KYLEPLUS_CTC)
	pr_info("BL : %d\n", level);
#endif
	/* function will spin lock */
	backlight_ic_set_brightness(level);

#if 0
	if (!bl_value) {
		backlight_ic_set_brightness(0);
		/*  Turn off Backlight, don't check disp_initialized value */
		lcd_prf = 1;

	} else {
		backlight_ic_set_brightness(bl_value);
		if (lcd_prf)
			return;

		while (!disp_state.disp_initialized) {
			msleep(100);
			lockup_count++;

			if (lockup_count > 50) {
				pr_err("Prevent infinite loop(wait for 5s)\n");
				pr_err("LCD can't initialize with in %d ms\n",
						lockup_count*100);
				lockup_count = 0;

				down(&backlight_sem);
				return;
			}
		}
	}

	backlight_ic_set_brightness(bl_value);
#endif
}

static struct msm_fb_panel_data nt35502_panel_data = {
	.on	= mipi_nt35502_lcd_on,
	.off = mipi_nt35502_lcd_off,
	.set_backlight = mipi_nt35502_set_backlight,
#if defined(CONFIG_FB_MSM_MIPI_CMD_PANEL_AVOID_MOSAIC) \
	|| defined(CONFIG_MACH_KYLEPLUS_OPEN) \
	|| defined(CONFIG_MACH_INFINITE_DUOS_CTC)
	.unblank = mipi_nt35502_unblank,
#endif
};

static int ch_used[3];

static int mipi_nt35502_lcd_init(void)
{
	DPRINT("start %s\n", __func__);

	mipi_dsi_buf_alloc(&nt35502_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&nt35502_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}

int mipi_nt35502_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	DPRINT("start %s\n", __func__);

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	ret = mipi_nt35502_lcd_init();
	if (ret) {
		DPRINT("mipi_nt35502_lcd_init() failed with ret %u\n", ret);
		return ret;
	}

	pdev = platform_device_alloc("mipi_NT35502", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	nt35502_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &nt35502_panel_data,
				sizeof(nt35502_panel_data));
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
