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

#ifndef MIPI_HX8357_H
#define MIPI_HX8357_H

#include "mipi_dsi.h"

/* in milliseconds */
#define HX8357_RESET_SLEEP_OUT_DELAY 120
#define HX8357_RESET_SLEEP_IN_DELAY 5
#define HX8357_SLEEP_IN_DELAY 120
#define HX8357_SLEEP_OUT_DELAY 150
#define HX8357_CMD_SETTLE 0
#define HX8357_SETTING_SETTLE 5
#define HX8357_CMD_DELAY_10MS	10
#define HX8357_CMD_DELAY_50MS	50
#define HX8357_SETTING_GAMMA 1
#define HX8357_SLEEP_IN_DELAY_50MS 50

/* in microseconds */
#define HX8357_EXIT_DEEP_STANDBY 3001
#define HX8357_HOLD_RESET 11

extern unsigned int board_hw_revision;

int mipi_hx8357_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel,
					struct dsi_cmd_desc *panel_prepare,
					int panel_prepare_length);


/***** common commands ******/
static char hx8357_read_display_power_mode[] = { 0x0A, 0x00 };
static char hx8357_deep_standby_on[] = { 0x4F, 0x01 };

static char hx8357_exit_sleep[] = { 0x11, 0x00 };
static char hx8357_display_on[] = { 0x29, 0x00 };
static char hx8357_display_off[] = { 0x28, 0x00 };
static char hx8357_enter_sleep[] = { 0x10, 0x00 };
static char hx8357_internal_osc[] = {0xB0, 0x66, 0x00}; /*for new version lcd*/


static struct dsi_cmd_desc hx8357_deep_standby_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, HX8357_CMD_SETTLE,
	sizeof(hx8357_deep_standby_on), hx8357_deep_standby_on}
};

static struct dsi_cmd_desc hx8357_read_display_power_mode_cmds[] = {
	{DTYPE_DCS_READ, 1, 0, 0, HX8357_CMD_SETTLE,
		sizeof(hx8357_read_display_power_mode), hx8357_read_display_power_mode}
};

static struct dsi_cmd_desc hx8357_display_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, HX8357_CMD_SETTLE, sizeof(hx8357_display_on),
	hx8357_display_on}
	,
};

static struct dsi_cmd_desc hx8357_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, HX8357_CMD_DELAY_50MS,sizeof(hx8357_display_off), hx8357_display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, HX8357_SLEEP_IN_DELAY_50MS, sizeof(hx8357_enter_sleep), hx8357_enter_sleep}
};

#endif  /* MIPI_HX8357_H */
