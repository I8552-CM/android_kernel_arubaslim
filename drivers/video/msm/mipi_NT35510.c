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
#include "mipi_NT35510.h"

#include "lcdc_backlight_ic.h"

static struct msm_panel_common_pdata *mipi_nt35510_pdata;
static struct dsi_buf nt35510_tx_buf;
static struct dsi_buf nt35510_rx_buf;

/* in msec */
#define NT35510_RESET_SLEEP_OUT_DELAY 120
#define NT35510_RESET_SLEEP_IN_DELAY 5
#define NT35510_SLEEP_IN_DELAY 120
#define NT35510_SLEEP_OUT_DELAY 5
#define NT35510_HOLD_RESET 10
#define NT35510_CMD_SETTLE 0
#define NT35510_SETTING_SETTLE 0

#define NT35510_USE_DEEP_STANDBY 1

#if (0 == NT35510_USE_DEEP_STANDBY)
#define NT35510_USE_POWERDOWN 1
#else /* (0 == NT35510_USE_DEEP_STANDBY) */
#define NT35510_USE_POWERDOWN 0
#endif /* (0 == NT35510_USE_DEEP_STANDBY) */

#if defined(CONFIG_APPLY_MDNIE)
typedef struct mdnie_config {
	int scenario;
	int negative;
	int outdoor;
};
struct class *mdnie_class;
struct mdnie_config mDNIe_cfg;
static int is_lcd_on=0;
static void set_mDNIe_Mode(struct mdnie_config *mDNIeCfg);
#endif

/* common setting */
static char exit_sleep[] = {0x11, 0x00};
static char display_on[] = {0x29, 0x00};
static char display_off[] = {0x28, 0x00};
static char enter_sleep[] = {0x10, 0x00};
static char all_pixel_off[] = {0x22, 0x00};
static char normal_disp_mode_on[] = {0x13, 0x00};

#if (1 == NT35510_USE_DEEP_STANDBY)
static char deep_standby_on[] = {0x4F, 0x01};
#endif /* (1 == NT35510_USE_DEEP_STANDBY) */
static char write_ram[] = {0x2c, 0x00}; /* write ram */
	
#if defined(CONFIG_APPLY_MDNIE)
static char inversion_on[2] = {0x21, 0x00};
static struct dsi_cmd_desc nt35510_cmd_inversion_on_cmds = {
	DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(inversion_on), inversion_on};

static char inversion_off[2] = {0x20, 0x00};
static struct dsi_cmd_desc nt35510_cmd_inversion_off_cmds = {
	DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(inversion_off), inversion_off};

#endif

static struct dsi_cmd_desc nt35510_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, NT35510_SLEEP_IN_DELAY, sizeof(enter_sleep), enter_sleep}
};

#if (1 == NT35510_USE_DEEP_STANDBY)
static struct dsi_cmd_desc nt35510_deep_standby_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(deep_standby_on), deep_standby_on},
	{DTYPE_DCS_WRITE, 1, 0, 0, 50, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 50, sizeof(enter_sleep), enter_sleep}
};
#endif /* (1 == NT35510_USE_DEEP_STANDBY) */

// kiyong17.kim  12.12.5
/*Power Setting*/
static char cmd1[] = {
        0xF0,
        0x55, 0xAA, 0x52, 0x08, 0x01
};
static char cmd2[] = {
	0xB0,
        0x09, 0x09, 0x09
};
static char cmd3[] = {
	0xB6,
        0x34, 0x34, 0x34
};
static char cmd4[] = {
	0xB1,
        0x09, 0x09, 0x09
};
static char cmd5[] = {
	0xB7,
        0x24, 0x24, 0x24
};
static char cmd6[] = {
	0xB3,
        0x05, 0x05, 0x05
};
static char cmd7[] = {
	0xB9,
        0x24, 0x24, 0x24
};
static char cmd8[] = {
	0xBF,
        0x01
};
static char cmd9[] = {
	0xB5,
        0x0b, 0x0b, 0x0b,
};
static char cmd10[] = {
	0xBA,
        0x24, 0x24, 0x24
};
static char cmd11[] = {
	0xBC,
        0x00, 0xA3, 0x00
};
static char cmd12[] = {
	0xBD,
        0x00, 0xA3, 0x00
};


/* Display Setting */
static char cmd13[] = {
	0xF0,
        0x55, 0xAA, 0x52, 0x08, 0x00
};

static char cmd14[] = {
	0xBD,
        0x01, 0x84, 0x07, 0x32, 0x00
};

static char cmd15[] = {
	0x36,
        0x02
};
static char cmd16[] = {
	0xB6,
        0x0A
};
static char cmd17[] = {
	0xB7,
        0x00, 0x00
};
static char cmd18[] = {
	0xB8,
        0x01, 0x05, 0x05, 0x05
};
static char cmd19[] = {
	0xBA,
        0x01,
};
static char cmd_rotate[3] = {
	0xB1,
        0x4C, 0x04
};
static char cmd20[] = {
	0xBE,
        0x01, 0x84, 0x07, 0x31, 0x00
};
static char cmd21[] = {
	0xBF,
        0x01, 0x84, 0x07, 0x31, 0x00
};
static char cmd22[] = {
	0x35,
        0x00
};
static char cmd23[] = {
	0xCC,
        0x03, 0x00, 0x00,
};


/* Gamma Control */
static char cmd26[] = {
	0xD1,
        0x00, 0x01, 0x00, 0x43, 0x00, 0x6B, 0x00, 0x87,
        0x00, 0xA3, 0x00, 0xCE, 0x00, 0xF1, 0x01, 0x27,
        0x01, 0x53, 0x01, 0x98, 0x01, 0xCE, 0x02, 0x22,
        0x02, 0x83, 0x02, 0x78, 0x02, 0x9E, 0x02, 0xDD,
        0x03, 0x00, 0x03, 0x2E, 0x03, 0x54, 0x03, 0x7F,
        0x03, 0x95, 0x03, 0xB3, 0x03, 0xC2, 0x03, 0xE1,
        0x03, 0xF1, 0x03, 0xFE
};
static char cmd27[] = {
        0xD2,
         0x00, 0x01, 0x00, 0x43, 0x00, 0x6B, 0x00, 0x87,
        0x00, 0xA3, 0x00, 0xCE, 0x00, 0xF1, 0x01, 0x27,
        0x01, 0x53, 0x01, 0x98, 0x01, 0xCE, 0x02, 0x22,
        0x02, 0x83, 0x02, 0x78, 0x02, 0x9E, 0x02, 0xDD,
        0x03, 0x00, 0x03, 0x2E, 0x03, 0x54, 0x03, 0x7F,
        0x03, 0x95, 0x03, 0xB3, 0x03, 0xC2, 0x03, 0xE1,
        0x03, 0xF1, 0x03, 0xFE
};
static char cmd28[] = {
	0xD3,
         0x00, 0x01, 0x00, 0x43, 0x00, 0x6B, 0x00, 0x87,
        0x00, 0xA3, 0x00, 0xCE, 0x00, 0xF1, 0x01, 0x27,
        0x01, 0x53, 0x01, 0x98, 0x01, 0xCE, 0x02, 0x22,
        0x02, 0x83, 0x02, 0x78, 0x02, 0x9E, 0x02, 0xDD,
        0x03, 0x00, 0x03, 0x2E, 0x03, 0x54, 0x03, 0x7F,
        0x03, 0x95, 0x03, 0xB3, 0x03, 0xC2, 0x03, 0xE1,
        0x03, 0xF1, 0x03, 0xFE
};
static char cmd29[] = {
	0xD4, 
        0x00, 0x01, 0x00, 0x43, 0x00, 0x6B, 0x00, 0x87,
        0x00, 0xA3, 0x00, 0xCE, 0x00, 0xF1, 0x01, 0x27,
        0x01, 0x53, 0x01, 0x98, 0x01, 0xCE, 0x02, 0x22,
        0x02, 0x43, 0x02, 0x50, 0x02, 0x9E, 0x02, 0xDD,
        0x03, 0x00, 0x03, 0x2E, 0x03, 0x54, 0x03, 0x7F,
        0x03, 0x95, 0x03, 0xB3, 0x03, 0xC2, 0x03, 0xE1,
        0x03, 0xF1, 0x03, 0xFE,
};
static char cmd30[] = {
	0xD5,
        0x00, 0x01, 0x00, 0x43, 0x00, 0x6B, 0x00, 0x87,
        0x00, 0xA3, 0x00, 0xCE, 0x00, 0xF1, 0x01, 0x27,
        0x01, 0x53, 0x01, 0x98, 0x01, 0xCE, 0x02, 0x22,
        0x02, 0x43, 0x02, 0x50, 0x02, 0x9E, 0x02, 0xDD,
        0x03, 0x00, 0x03, 0x2E, 0x03, 0x54, 0x03, 0x7F,
        0x03, 0x95, 0x03, 0xB3, 0x03, 0xC2, 0x03, 0xE1,
        0x03, 0xF1, 0x03, 0xFE,
};
static char cmd31[] = {
	0xD6,
         0x00, 0x01, 0x00, 0x43, 0x00, 0x6B, 0x00, 0x87,
        0x00, 0xA3, 0x00, 0xCE, 0x00, 0xF1, 0x01, 0x27,
        0x01, 0x53, 0x01, 0x98, 0x01, 0xCE, 0x02, 0x22,
        0x02, 0x43, 0x02, 0x50, 0x02, 0x9E, 0x02, 0xDD,
        0x03, 0x00, 0x03, 0x2E, 0x03, 0x54, 0x03, 0x7F,
        0x03, 0x95, 0x03, 0xB3, 0x03, 0xC2, 0x03, 0xE1,
        0x03, 0xF1, 0x03, 0xFE,
};

static char nt35510_boe_dummy_data[] = {
	0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,	
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
};

static char nt35510_boe_eng1[] = {
	0xFF,
	0xAA, 0x55, 0x25, 0x01
};


static char nt35510_boe_eng2[] = {
	0xFC,
	0x06, 0xA0, 0x24, 0x00,
	0x3C, 0x00, 0x04, 0x80,
	0x10
};


static char nt35510_boe_eng3[] = {
	0xF8,
	0x01, 0x02, 0x00, 0x20, 0x33,
	0x13, 0x00, 0x40, 0x00, 0x00,
	0x23, 0x02, 0x19, 0xC8, 0x00,
	0x00,0x11
};


static struct dsi_cmd_desc nt35510_boe_cmd_dummy[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, NT35510_CMD_SETTLE,
	sizeof(nt35510_boe_dummy_data), nt35510_boe_dummy_data}
};


static struct dsi_cmd_desc nt35510_cmd_display_on_kyleplus_exit_sleep[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(exit_sleep), exit_sleep},
};

static struct dsi_cmd_desc nt35510_cmd_display_on_kyleplus1[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(cmd1), cmd1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(cmd2), cmd2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(cmd3), cmd3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(cmd4), cmd4},
	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(cmd5), cmd5},
	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(cmd6), cmd6},
	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(cmd7), cmd7},
	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(cmd8), cmd8},
	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(cmd9), cmd9},
	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(cmd10), cmd10},
	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(cmd11), cmd11},
	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(cmd12), cmd12},
};

static struct dsi_cmd_desc nt35510_cmd_display_on_kyleplus2[] = {		
	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(cmd26), cmd26},
	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(cmd27), cmd27},
	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(cmd28), cmd28},
	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(cmd29), cmd29},
	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(cmd30), cmd30},
	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(cmd31), cmd31},
	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(cmd13), cmd13},

};

static struct dsi_cmd_desc nt35510_cmd_display_on_kyleplus3[] = {		
//	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(cmd_rotate), cmd_rotate},
	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(cmd15), cmd15},
	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(cmd16), cmd16},
	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(cmd17), cmd17},
	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(cmd18), cmd18},
	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(cmd19), cmd19},
	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(cmd14), cmd14},
	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(cmd20), cmd20},
	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(cmd21), cmd21},
	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(cmd22), cmd22},
	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(cmd23), cmd23},
#if defined(CONFIG_FB_MSM_MIPI_CMD_PANEL_AVOID_MOSAIC) || defined(CONFIG_MACH_KYLEPLUS_OPEN) || defined(CONFIG_MACH_INFINITE_DUOS_CTC)
	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(all_pixel_off), all_pixel_off},
#endif
	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(display_on), display_on},
};
#if defined(CONFIG_FB_MSM_MIPI_CMD_PANEL_AVOID_MOSAIC) || defined(CONFIG_MACH_KYLEPLUS_OPEN) || defined(CONFIG_MACH_INFINITE_DUOS_CTC)
static struct dsi_cmd_desc nt35510_cmd_unblank[] = {
	// don't use display_on command, because it causes flash
	{DTYPE_GEN_LWRITE, 1, 0, 0, NT35510_CMD_SETTLE, sizeof(normal_disp_mode_on), normal_disp_mode_on},
};
#endif
static struct dsi_cmd_desc nt35510_cmd_display_on_cmds_rotate[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd_rotate), cmd_rotate},
};

static char video0[6] = {
	0xF0, 0x55, 0xAA, 0x52,
	0x08, 0x01,
};
static char video1[4] = {
	0xBC, 0x00, 0xA0, 0x00,
};
static char video2[4] = {
	0xBD, 0x00, 0xA0, 0x00,
};
static char video3[3] = {
	0xBE, 0x00, 0x79,
};
static char video4[53] = {
	0xD1, 0x00, 0x00, 0x00,
	0x14, 0x00, 0x32, 0x00,
	0x4F, 0x00, 0x65, 0x00,
	0x8B, 0x00, 0xA8, 0x00,
	0xD5, 0x00, 0xF7, 0x01,
	0x2B, 0x01, 0x54, 0x01,
	0x8E, 0x01, 0xBB, 0x01,
	0xBC, 0x01, 0xE3, 0x02,
	0x08, 0x02, 0x1C, 0x02,
	0x39, 0x02, 0x4F, 0x02,
	0x76, 0x02, 0xA3, 0x02,
	0xE3, 0x03, 0x12, 0x03,
	0x4C, 0x03, 0x66, 0x03,
	0x9A,
};
static char video5[53] = {
	0xD2, 0x00, 0x00, 0x00,
	0x14, 0x00, 0x32, 0x00,
	0x4F, 0x00, 0x65, 0x00,
	0x8B, 0x00, 0xA8, 0x00,
	0xD5, 0x00, 0xF7, 0x01,
	0x2B, 0x01, 0x54, 0x01,
	0x8E, 0x01, 0xBB, 0x01,
	0xBC, 0x01, 0xE3, 0x02,
	0x08, 0x02, 0x1C, 0x02,
	0x39, 0x02, 0x4F, 0x02,
	0x76, 0x02, 0xA3, 0x02,
	0xE3, 0x03, 0x12, 0x03,
	0x4C, 0x03, 0x66, 0x03,
	0x9A,
};
static char video6[53] = {
	0xD3, 0x00, 0x00, 0x00,
	0x14, 0x00, 0x32, 0x00,
	0x4F, 0x00, 0x65, 0x00,
	0x8B, 0x00, 0xA8, 0x00,
	0xD5, 0x00, 0xF7, 0x01,
	0x2B, 0x01, 0x54, 0x01,
	0x8E, 0x01, 0xBB, 0x01,
	0xBC, 0x01, 0xE3, 0x02,
	0x08, 0x02, 0x1C, 0x02,
	0x39, 0x02, 0x4F, 0x02,
	0x76, 0x02, 0xA3, 0x02,
	0xE3, 0x03, 0x12, 0x03,
	0x4C, 0x03, 0x66, 0x03,
	0x9A,
};
static char video7[53] = {
	0xD4, 0x00, 0x00, 0x00,
	0x14, 0x00, 0x32, 0x00,
	0x4F, 0x00, 0x65, 0x00,
	0x8B, 0x00, 0xA8, 0x00,
	0xD5, 0x00, 0xF7, 0x01,
	0x2B, 0x01, 0x54, 0x01,
	0x8E, 0x01, 0xBB, 0x01,
	0xBC, 0x01, 0xE3, 0x02,
	0x08, 0x02, 0x1C, 0x02,
	0x39, 0x02, 0x4F, 0x02,
	0x76, 0x02, 0xA3, 0x02,
	0xE3, 0x03, 0x12, 0x03,
	0x4C, 0x03, 0x66, 0x03,
	0x9A,
};
static char video8[53] = {
	0xD5, 0x00, 0x00, 0x00,
	0x14, 0x00, 0x32, 0x00,
	0x4F, 0x00, 0x65, 0x00,
	0x8B, 0x00, 0xA8, 0x00,
	0xD5, 0x00, 0xF7, 0x01,
	0x2B, 0x01, 0x54, 0x01,
	0x8E, 0x01, 0xBB, 0x01,
	0xBC, 0x01, 0xE3, 0x02,
	0x08, 0x02, 0x1C, 0x02,
	0x39, 0x02, 0x4F, 0x02,
	0x76, 0x02, 0xA3, 0x02,
	0xE3, 0x03, 0x12, 0x03,
	0x4C, 0x03, 0x66, 0x03,
	0x9A,
};
static char video9[53] = {
	0xD6, 0x00, 0x00, 0x00,
	0x14, 0x00, 0x32, 0x00,
	0x4F, 0x00, 0x65, 0x00,
	0x8B, 0x00, 0xA8, 0x00,
	0xD5, 0x00, 0xF7, 0x01,
	0x2B, 0x01, 0x54, 0x01,
	0x8E, 0x01, 0xBB, 0x01,
	0xBC, 0x01, 0xE3, 0x02,
	0x08, 0x02, 0x1C, 0x02,
	0x39, 0x02, 0x4F, 0x02,
	0x76, 0x02, 0xA3, 0x02,
	0xE3, 0x03, 0x12, 0x03,
	0x4C, 0x03, 0x66, 0x03,
	0x9A,
};
static char video10[4] = {
	0xB0, 0x0A, 0x0A, 0x0A,
};
static char video11[4] = {
	0xB1, 0x0A, 0x0A, 0x0A,
};
static char video12[4] = {
	0xBA, 0x24, 0x24, 0x24,
};
static char video13[4] = {
	0xB9, 0x24, 0x24, 0x24,
};
static char video14[4] = {
	0xB8, 0x24, 0x24, 0x24,
};
static char video15[6] = {
	0xF0, 0x55, 0xAA, 0x52,
	0x08, 0x00,
};
static char video16[2] = {
	0xB3, 0x00,
};
static char video17[2] = {
	0xB4, 0x10,
};
static char video18[2] = {
	0xB6, 0x02,
};
static char video19[3] = {
	0xB1, 0xFC, 0x06,
};
static char video20[4] = {
	0xBC, 0x05, 0x05, 0x05,
};
static char video21[3] = {
	0xB7, 0x20, 0x20,
};
static char video22[5] = {
	0xB8, 0x01, 0x03, 0x03,
	0x03,
};
static char video23[19] = {
	0xC8, 0x01, 0x00, 0x78,
	0x50, 0x78, 0x50, 0x78,
	0x50, 0x78, 0x50, 0xC8,
	0x3C, 0x3C, 0xC8, 0xC8,
	0x3C, 0x3C, 0xC8,
};
static char video24[6] = {
	0xBD, 0x01, 0x84, 0x07,
	0x31, 0x00,
};
static char video25[6] = {
	0xBE, 0x01, 0x84, 0x07,
	0x31, 0x00,
};
static char video26[6] = {
	0xBF, 0x01, 0x84, 0x07,
	0x31, 0x00,
};
static char video27[2] = {
	0x35, 0x00,
};
static char config_video_MADCTL[2] = {0x36, 0xC0};
static struct dsi_cmd_desc nt35510_video_display_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video0), video0},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video1), video1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video2), video2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video3), video3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video4), video4},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video5), video5},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video6), video6},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video7), video7},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video8), video8},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video9), video9},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video10), video10},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video11), video11},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video12), video12},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video13), video13},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video14), video14},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video15), video15},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video16), video16},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video17), video17},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video18), video18},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video19), video19},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video20), video20},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video21), video21},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video22), video22},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video23), video23},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video24), video24},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video25), video25},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video26), video26},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video27), video27},
	{DTYPE_DCS_WRITE, 1, 0, 0, NT35510_SLEEP_OUT_DELAY, sizeof(exit_sleep),
			exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, NT35510_CMD_SETTLE,
	 sizeof(display_on), display_on},
};

static struct dsi_cmd_desc nt35510_video_display_on_cmds_rotate[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 150,
		sizeof(config_video_MADCTL), config_video_MADCTL},
};
extern unsigned int board_hw_revision;
static int lcd_reset = 0;

#define VREG_ENABLE	TRUE
#define VREG_DISABLE	FALSE

#define LCDC_DEBUG

#ifdef LCDC_DEBUG
#define DPRINT(x...)	printk(KERN_ERR "NT35510 " x)
#else
#define DPRINT(x...)
#endif

static void nt35510_vreg_config(boolean vreg_en)
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

static void nt35510_disp_reset(boolean special_case, boolean sleep_in)
{
        /* appsbl leaves LCD powered up */
        static boolean first_time = TRUE;
        if(TRUE == first_time) {
                first_time = FALSE;
                return;
        }

	DPRINT("start %s\n", __func__);
#if !defined(CONFIG_MACH_KYLEPLUS_CTC)
	gpio_tlmm_config(GPIO_CFG(lcd_reset, 0, GPIO_CFG_OUTPUT
				, GPIO_CFG_NO_PULL
				, GPIO_CFG_2MA)
				, GPIO_CFG_ENABLE);
#endif
	if (special_case) {
		gpio_set_value(lcd_reset, 0);
		msleep(NT35510_HOLD_RESET);
		gpio_set_value(lcd_reset, 1);
		msleep(sleep_in ? NT35510_RESET_SLEEP_IN_DELAY : NT35510_RESET_SLEEP_OUT_DELAY);
	}
	gpio_set_value(lcd_reset, 0);
	msleep(NT35510_HOLD_RESET);
	gpio_set_value(lcd_reset, 1);
	msleep(sleep_in ? NT35510_RESET_SLEEP_IN_DELAY : NT35510_RESET_SLEEP_OUT_DELAY);
}

#if (1 == NT35510_USE_DEEP_STANDBY)
static void nt35510_disp_enter_deep_standby(
                struct msm_fb_data_type *mfd,
                struct dsi_buf *buf
                                           )
{
        mipi_dsi_cmds_tx(
                buf, 
                nt35510_deep_standby_cmds,
                ARRAY_SIZE(nt35510_deep_standby_cmds)
                        );
}

static void nt35510_disp_exit_deep_standby(void)
{
        /* LCD must've been in sleep to deep standby */
        nt35510_disp_reset(FALSE, TRUE);
}
#endif /* (1 == NT35510_USE_DEEP_STANDBY) */

#if (1 == NT35510_USE_POWERDOWN)
static void nt35510_disp_powerup(void)
{
	DPRINT("start %s\n", __func__);
        nt35510_vreg_config(VREG_ENABLE);

        /* after vdd and vddi are enabled we are @ sleep in */
        nt35510_disp_reset(FALSE, TRUE);
}

static void nt35510_disp_powerdown(void)
{
	DPRINT("start %s\n", __func__);

        /* nt35510_disp_powerdown called after sleep in */
        nt35510_disp_reset(FALSE, TRUE);
	nt35510_vreg_config(VREG_DISABLE);
}
#endif /* (1 == NT35510_USE_POWERDOWN) */

#if defined(CONFIG_FB_MSM_MIPI_CMD_PANEL_AVOID_MOSAIC) || defined(CONFIG_MACH_KYLEPLUS_OPEN) || defined(CONFIG_MACH_INFINITE_DUOS_CTC)
static void mipi_nt35510_unblank(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;

	mfd = platform_get_drvdata(pdev);
	mipi  = &mfd->panel_info.mipi;
	
	DPRINT("%s +:\n", __func__);
	if (mipi->mode == DSI_CMD_MODE) {
		mipi_dsi_cmd_mdp_busy();		
		mipi_dsi_cmds_tx(&nt35510_tx_buf, nt35510_cmd_unblank, ARRAY_SIZE(nt35510_cmd_unblank));
	}
	DPRINT("%s -:\n", __func__);
}
#endif

static int mipi_nt35510_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;
	int rotate = 1;
        int ret = 0;

	DPRINT("start %s\n", __func__);

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;

	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi  = &mfd->panel_info.mipi;
		
#if (1 == NT35510_USE_DEEP_STANDBY)
        nt35510_disp_exit_deep_standby();
#endif /* #if (1 == NT35510_USE_DEEP_STANDBY) */
#if (1 == NT35510_USE_POWERDOWN)
        nt35510_disp_powerup();
#endif /* (1 == NT35510_USE_POWERDOWN) */
	if (!mfd->cont_splash_done) {
		mfd->cont_splash_done = 1;
		return 0;
	}

	if (mipi_nt35510_pdata && mipi_nt35510_pdata->rotate_panel)
		rotate = mipi_nt35510_pdata->rotate_panel();

	if (mipi->mode == DSI_VIDEO_MODE) {
		mipi_dsi_cmds_tx(&nt35510_tx_buf,
			nt35510_video_display_on_cmds,
			ARRAY_SIZE(nt35510_video_display_on_cmds));

		if (rotate) {
			mipi_dsi_cmds_tx(&nt35510_tx_buf,
				nt35510_video_display_on_cmds_rotate,
			ARRAY_SIZE(nt35510_video_display_on_cmds_rotate));
		}
	} else if (mipi->mode == DSI_CMD_MODE) {
		
		ret = mipi_dsi_cmds_tx(&nt35510_tx_buf, nt35510_cmd_display_on_kyleplus_exit_sleep, ARRAY_SIZE(nt35510_cmd_display_on_kyleplus_exit_sleep));
		if(ret==0)
			goto MIPI_ERROR;
		mdelay(5);
		ret = mipi_dsi_cmds_tx(&nt35510_tx_buf, nt35510_cmd_display_on_kyleplus1, ARRAY_SIZE(nt35510_cmd_display_on_kyleplus1));
		if(ret==0)
			goto MIPI_ERROR;
		ret = mipi_dsi_cmds_tx(&nt35510_tx_buf, nt35510_cmd_display_on_kyleplus2, ARRAY_SIZE(nt35510_cmd_display_on_kyleplus2));
		if(ret==0)
			goto MIPI_ERROR;
		if (rotate) {
			ret = mipi_dsi_cmds_tx(&nt35510_tx_buf,
				nt35510_cmd_display_on_cmds_rotate,
			ARRAY_SIZE(nt35510_cmd_display_on_cmds_rotate));
			if(ret==0)
			goto MIPI_ERROR;
		}
		ret = mipi_dsi_cmds_tx(&nt35510_tx_buf, nt35510_cmd_display_on_kyleplus3, ARRAY_SIZE(nt35510_cmd_display_on_kyleplus3));
		if(ret==0)
			goto MIPI_ERROR;

#if defined(CONFIG_FB_MSM_MIPI_CMD_PANEL_AVOID_MOSAIC) || defined(CONFIG_MACH_KYLEPLUS_OPEN) || defined(CONFIG_MACH_INFINITE_DUOS_CTC)
		// delay to avoid flash caused by display_on command, 
		// it work with msm_fb_blank_sub() backlight protection
		msleep(160);
#endif
	}
	
#if defined(CONFIG_APPLY_MDNIE)
	is_lcd_on = 1;
	set_mDNIe_Mode(&mDNIe_cfg);
#endif

	DPRINT("exit %s\n", __func__);
	return 0;
	
MIPI_ERROR:
	DPRINT("%s MIPI_ERROR -\n", __func__);
	return ret;
}
static int mipi_nt35510_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	static int b_first_off=1;

	DPRINT("start %s\n", __func__);

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;
	
#if !defined(CONFIG_MACH_KYLEPLUS_CTC) && !defined(CONFIG_MACH_KYLEPLUS_OPEN) && !defined(CONFIG_MACH_INFINITE_DUOS_CTC)
	backlight_ic_set_brightness(0);
#endif	
	if(b_first_off)	// workaround.
	{
		nt35510_disp_exit_deep_standby();

		msleep(20);

		mipi_dsi_cmds_tx(&nt35510_tx_buf, nt35510_cmd_display_on_kyleplus_exit_sleep, ARRAY_SIZE(nt35510_cmd_display_on_kyleplus_exit_sleep));
		mdelay(120);
		mipi_dsi_cmds_tx(&nt35510_tx_buf, nt35510_cmd_display_on_kyleplus1, ARRAY_SIZE(nt35510_cmd_display_on_kyleplus1));
		mdelay(120);
		mipi_dsi_cmds_tx(&nt35510_tx_buf, nt35510_cmd_display_on_kyleplus2, ARRAY_SIZE(nt35510_cmd_display_on_kyleplus2));
		if (1) {
			mipi_dsi_cmds_tx(&nt35510_tx_buf,
				nt35510_cmd_display_on_cmds_rotate,
			ARRAY_SIZE(nt35510_cmd_display_on_cmds_rotate));
		}
		mipi_dsi_cmds_tx(&nt35510_tx_buf, nt35510_cmd_display_on_kyleplus3, ARRAY_SIZE(nt35510_cmd_display_on_kyleplus3));
		mdelay(10);	
		
		msleep(20);
		b_first_off=0;
	}
			
	
	
#if defined(CONFIG_APPLY_MDNIE)
	is_lcd_on = 0;
#endif

#if defined(CONFIG_FB_MSM_MIPI_CMD_PANEL_AVOID_MOSAIC) || defined(CONFIG_MACH_KYLEPLUS_OPEN) || defined(CONFIG_MACH_INFINITE_DUOS_CTC)
	// let screen light off smoothly, because backlight is turning off in another thread now
	msleep(80);
#endif

	mipi_dsi_cmds_tx(&nt35510_tx_buf, nt35510_display_off_cmds,
			ARRAY_SIZE(nt35510_display_off_cmds));
#if (1 == NT35510_USE_DEEP_STANDBY)
        nt35510_disp_enter_deep_standby(mfd, &nt35510_tx_buf);
#endif /* (1 == NT35510_USE_DEEP_STANDBY) */
#if (1 == NT35510_USE_POWERDOWN)
        nt35510_disp_powerdown();
#endif /* (1 == NT35510_USE_POWERDOWN) */

	DPRINT("exit %s\n", __func__);
	return 0;
}


static ssize_t mipi_nt35510_lcdtype_show(struct device *dev, 
        struct device_attribute *attr, char *buf)
{
        char temp[20];
        sprintf(temp, "INH_GH96-05884A\n");
        strcat(buf, temp);
        return strlen(buf);
}

/* FIXME: add handlers */
static struct lcd_ops mipi_lcd_props;
static DEVICE_ATTR(lcd_type, S_IRUGO, mipi_nt35510_lcdtype_show, NULL);
#if defined(CONFIG_APPLY_MDNIE)
static void set_mDNIe_Mode(struct mdnie_config *mDNIeCfg)
{
	int value;

	DPRINT("%s +:[mDNIe] negative=%d\n", __func__, mDNIe_cfg.negative);
	if(!is_lcd_on)
		return;
	mipi_dsi_cmd_mdp_busy();		
	if (mDNIe_cfg.negative) {
		mipi_dsi_cmds_tx(&nt35510_tx_buf, &nt35510_cmd_inversion_on_cmds, 1);
	} else {
		mipi_dsi_cmds_tx(&nt35510_tx_buf, &nt35510_cmd_inversion_off_cmds, 1);
	}
	DPRINT("%s -:[mDNIe]", __func__);

}

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
	}
	
	set_mDNIe_Mode(&mDNIe_cfg);
    return size;
}

static DEVICE_ATTR(negative, 0664, mDNIeNegative_show, mDNIeNegative_store);
#endif

static int __devinit mipi_nt35510_lcd_probe(struct platform_device *pdev)
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
		mipi_nt35510_pdata = pdev->dev.platform_data;
		return 0;
	}

	msm_fb_add_device(pdev);
	lcd_device = lcd_device_register("panel", &pdev->dev, NULL,
					&mipi_lcd_props);
	pdata = pdev->dev.platform_data;
	if (mipi_nt35510_pdata && mipi_nt35510_pdata->rotate_panel()
			&& pdata->panel_info.type == MIPI_CMD_PANEL)
		pdata->panel_info.lcd.refx100 = 6200;

	if (IS_ERR(lcd_device)) {
		ret = PTR_ERR(lcd_device);
		printk(KERN_ERR "lcd : failed to register device\n");
		return ret;
	}

	ret = sysfs_create_file(&lcd_device->dev.kobj, &dev_attr_lcd_type.attr);
	if (ret)
		printk(KERN_ERR "sysfs create fail - %s\n",
		dev_attr_lcd_type.attr.name);
	
#if defined(CONFIG_APPLY_MDNIE)
	mDNIe_cfg.negative = 0;

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

	ret = device_create_file(mdnie_device, &dev_attr_negative);
	if(ret < 0)
		printk(KERN_ERR "sysfs create fail - %s\n",
		dev_attr_negative.attr.name);
#endif

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_nt35510_lcd_probe,
	.driver = {
		.name   = "mipi_NT35510",
	},
};


static void mipi_nt35510_set_backlight(struct msm_fb_data_type *mfd)
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

static struct msm_fb_panel_data nt35510_panel_data = {
	.on	= mipi_nt35510_lcd_on,
	.off = mipi_nt35510_lcd_off,
	.set_backlight = mipi_nt35510_set_backlight,
#if defined(CONFIG_FB_MSM_MIPI_CMD_PANEL_AVOID_MOSAIC) || defined(CONFIG_MACH_KYLEPLUS_OPEN) || defined(CONFIG_MACH_INFINITE_DUOS_CTC)
	.unblank = mipi_nt35510_unblank,
#endif
};

static int ch_used[3];

static int mipi_nt35510_lcd_init(void)
{
	DPRINT("start %s\n", __func__);

#if defined(CONFIG_MACH_KYLEPLUS_OPEN)
	lcd_reset = 23;
#else
        lcd_reset = 22;
#endif

	mipi_dsi_buf_alloc(&nt35510_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&nt35510_rx_buf, DSI_BUF_SIZE);
	
	return platform_driver_register(&this_driver);
}

int mipi_nt35510_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	DPRINT("start %s\n", __func__);

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	ret = mipi_nt35510_lcd_init();
	if (ret) {
		DPRINT("mipi_nt35510_lcd_init() failed with ret %u\n", ret);
		return ret;
	}

	pdev = platform_device_alloc("mipi_NT35510", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	nt35510_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &nt35510_panel_data,
				sizeof(nt35510_panel_data));
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
