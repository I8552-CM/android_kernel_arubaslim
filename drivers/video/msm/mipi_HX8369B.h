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

#ifndef MIPI_HX8369B_H
#define MIPI_HX8369B_H

#define HX8369B_AUO_554890	0x554890	// ARUBA AUO Old
#define HX8369B_AUO_554990	0x554990	// ARUBA AUO New
#define HX8369B_BOE_55C090	0x55C090	// ARUBA BOE
#define HX8369B_BOE_55BC90	0x55BC90	// DELOS BOE
#define HX8369B_SMD_83695A 	0x83695A	// BAFFIN SMD

extern u32 hx8369b_read_id;

int mipi_hx8369b_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel);

enum SCENARIO {
	UI_MODE,
	VIDEO_MODE,
	VIDEO_WARM_MODE,
	VIDEO_COLD_MODE,
	CAMERA_MODE,
	NAVI_MODE,
	GALLERY_MODE,
	VT_MODE,
	SCENARIO_MAX,
};

enum OUTDOOR {
	OUTDOOR_OFF,
	OUTDOOR_ON,
	OUTDOOR_MAX,
};

struct mdnie_config {
	int scenario;
	int negative;
	int outdoor;
	int curIndex;
};
#endif  /* MIPI_HX8369B_H */
