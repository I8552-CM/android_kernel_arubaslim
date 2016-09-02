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
 */

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_HX8357.h"

#define HX8357D_PANEL_SDC	0x5B8891
#define HX8357D_PANEL_BOE	0x5BBC91

static u32 read_id=0;

static int __init hx8357d_lcd_read_id(char *param)
{
	if(param[0]=='5'&&param[1]=='B'&&param[2]=='8'&&param[3]=='8'&&param[4]=='9'&&param[5]=='1') {
		read_id=HX8357D_PANEL_SDC;
		printk ( "@@@@@%s-HX8357D_PANEL_SDC\n");
	} else if(param[0]=='5'&&param[1]=='B'&&param[2]=='B'&&param[3]=='C'&&param[4]=='9'&&param[5]=='1') {
		read_id=HX8357D_PANEL_BOE;
		printk ( "@@@@@%s-HX8357D_PANEL_BOE\n");
	} else {
		read_id=HX8357D_PANEL_BOE;
		printk ( "@@@@@%s-default\n");
	}
}
__setup("LCDID=", hx8357d_lcd_read_id);



static char hx8357d_sdc_video_01[] = {
	0xB9,
	0xFF,0x83,0x57
};
static char hx8357d_sdc_video_02[] = {
	0xC6,
	0x00,0xFF,0x00
};
static char hx8357d_sdc_video_03[] = {
	0xCC,
	0x04
};
static char hx8357d_sdc_video_04[] = {
	0xB6,
	0x67
};
static char hx8357d_sdc_video_05[] = {
	0xB3,
	0x43
};
static char hx8357d_sdc_video_06[] = {
	0xB5,
	0x04,0x04,0x67
};
static char hx8357d_sdc_video_07[] = {
	0xB1,
	0x00,0x14,0x1E,0x1E,0xC3,0x47,0x54
};
static char hx8357d_sdc_video_08[] = {
	0xB4,
	0x80,0x40,0x00,0x2A,0x2A,0x03,0x30
};
static char hx8357d_sdc_video_09[] = {
	0xC0,
	0x38,0x38,0x01,0x3C,0x08,0x04
};
static char hx8357d_sdc_video_10[] = {
	0xE3,
	0x04,0x04
};
static char hx8357d_sdc_video_11[] = {
	0xC2,
	0x00,0x00,0x03
};
static char hx8357d_sdc_video_12[] = {
	0xE0,
	0x00,0x38,0x3D,0x46,0x4C,0x59,0x61,0x66,0x35,0x30,
	0x2B,0x24,0x21,0x1D,0x1A,0x03,0x00,0x38,0x3D,0x46,
	0x4C,0x59,0x61,0x66,0x35,0x30,0x2B,0x24,0x21,0x1D,
	0x1A,0x03,0x00,0x00
};
static char hx8357d_sdc_video_13[] = {
	0x3A,
	0x70
};
static char hx8357d_sdc_video_14[] = {
	0xE9,
	0x20
};
static char hx8357d_sdc_video_15[] = {
	0x11,
	0x00
};
static char hx8357d_sdc_video_16[] = {
	0x29,
	0x00
};



static char hx8357d_boe_video_01[] = {
 0x11,
 	0x00
};
static char hx8357d_boe_video_02[] = {
 0xB9,
 	0xFF,0x83,0x57
};
static char hx8357d_boe_video_03[] = {
 0xB1,
 	0x00,0x14,0x15,0x15,0xC6,0x21,0x54
};
static char hx8357d_boe_video_04[] = {
 0xB3,
 	0x43, 0x00, 0x06, 0x06 
};
static char hx8357d_boe_video_05[] = {
 0xB4,
 	0x02,0x40,0x00,0x2A,0x2A,0x01,0x40
};
static char hx8357d_boe_video_06[] = {
 0xB5,
 	0x0B,0x0B,0x66
};
static char hx8357d_boe_video_07[] = {
 0xCC,
 	0x05
};
static char hx8357d_boe_video_08[] = {
 0xC0,
 	0x25,0x25,0x00,0x38,0x08,0x08
};
static char hx8357d_boe_video_09[] = {
 0xC2,
 	0x00,0x08,0x04
};
static char hx8357d_boe_video_10[] = {
 0xE3,
 	0x05,0x05
};
static char hx8357d_boe_video_11[] = {
 0x3A,
 	0x77
};
static char hx8357d_boe_video_12[] = {
 0xE9,
 	0x20
};
static char hx8357d_boe_video_13[] = {
	0xE0,
	   0x00,0x07,0x13,0x23,0x2E,0x43,0x4F,0x56,0x43,0x3D,
	   0x38,0x30,0x2D,0x27,0x23,0x01,0x04,0x06,0x13,0x23,
	   0x2E,0x43,0x4F,0x57,0x43,0x3E,0x37,0x2F,0x2C,0x28,
	   0x23,0x01,0x00,0x01
};
static char hx8357d_boe_video_14[] = {
 0x29,
 	0x00
};
#if defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
static char hx8357d_boe_video_MADCTL[] = {
0x36,
   0xC0
};
#endif


static struct dsi_cmd_desc hx8357_smd_cmd_prepare_panel[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_SETTING_SETTLE,	sizeof(hx8357d_sdc_video_01), hx8357d_sdc_video_01},
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_SETTING_SETTLE,	sizeof(hx8357d_sdc_video_02), hx8357d_sdc_video_02},
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_CMD_SETTLE,		sizeof(hx8357d_sdc_video_03), hx8357d_sdc_video_03},
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_CMD_SETTLE,		sizeof(hx8357d_sdc_video_04), hx8357d_sdc_video_04},
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_CMD_SETTLE,		sizeof(hx8357d_sdc_video_05), hx8357d_sdc_video_05},
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_CMD_SETTLE,		sizeof(hx8357d_sdc_video_06), hx8357d_sdc_video_06},
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_CMD_SETTLE,		sizeof(hx8357d_sdc_video_07), hx8357d_sdc_video_07},
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_CMD_SETTLE,		sizeof(hx8357d_sdc_video_08), hx8357d_sdc_video_08},
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_CMD_SETTLE,		sizeof(hx8357d_sdc_video_09), hx8357d_sdc_video_09},
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_CMD_SETTLE,		sizeof(hx8357d_sdc_video_10), hx8357d_sdc_video_10},
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_CMD_SETTLE,		sizeof(hx8357d_sdc_video_11), hx8357d_sdc_video_11},
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_SETTING_GAMMA,	sizeof(hx8357d_sdc_video_12), hx8357d_sdc_video_12},
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_CMD_SETTLE,		sizeof(hx8357d_sdc_video_13), hx8357d_sdc_video_13},
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_CMD_SETTLE,		sizeof(hx8357d_sdc_video_14), hx8357d_sdc_video_14},
	{DTYPE_DCS_WRITE,  1, 0, 0, HX8357_SLEEP_OUT_DELAY, sizeof(hx8357d_sdc_video_15), hx8357d_sdc_video_15},
	{DTYPE_DCS_WRITE,  1, 0, 0, HX8357_HOLD_RESET,		sizeof(hx8357d_sdc_video_16), hx8357d_sdc_video_16},
};

static struct dsi_cmd_desc hx8357_boe_cmd_prepare_panel[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, HX8357_SLEEP_OUT_DELAY,	sizeof(hx8357d_boe_video_01), hx8357d_boe_video_01},
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_SETTING_SETTLE,	sizeof(hx8357d_boe_video_02), hx8357d_boe_video_02},
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_SETTING_SETTLE,	sizeof(hx8357d_boe_video_03), hx8357d_boe_video_03},
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_CMD_SETTLE,		sizeof(hx8357d_boe_video_04), hx8357d_boe_video_04},
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_CMD_SETTLE,		sizeof(hx8357d_boe_video_05), hx8357d_boe_video_05},
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_CMD_SETTLE,		sizeof(hx8357d_boe_video_06), hx8357d_boe_video_06},
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_CMD_SETTLE,		sizeof(hx8357d_boe_video_07), hx8357d_boe_video_07},
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_CMD_SETTLE,		sizeof(hx8357d_boe_video_08), hx8357d_boe_video_08},
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_CMD_SETTLE,		sizeof(hx8357d_boe_video_09), hx8357d_boe_video_09},
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_CMD_SETTLE,		sizeof(hx8357d_boe_video_10), hx8357d_boe_video_10},
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_CMD_SETTLE,		sizeof(hx8357d_boe_video_11), hx8357d_boe_video_11},
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_CMD_SETTLE,		sizeof(hx8357d_boe_video_12), hx8357d_boe_video_12},
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_SETTING_GAMMA,	sizeof(hx8357d_boe_video_13), hx8357d_boe_video_13},
	{DTYPE_DCS_WRITE,  1, 0, 0, HX8357_HOLD_RESET, 		sizeof(hx8357d_boe_video_14), hx8357d_boe_video_14},
#if defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
	{DTYPE_GEN_LWRITE, 1, 0, 0, HX8357_CMD_SETTLE,		sizeof(hx8357d_boe_video_MADCTL), hx8357d_boe_video_MADCTL},
#endif
};


static struct msm_panel_info pinfo;

static struct mipi_dsi_phy_ctrl dsi_cmd_mode_phy_db = { 
	{0x03, 0x01, 0x01, 0x00}, /* regulator */ 
	/* timing */ 
	{0xAD, 0x8B, 0x19, 0x00, 0x93, 0x97, 0x1C, 
	0x8D, 0x13, 0x03, 0x04}, 
	{0x7f, 0x00, 0x00, 0x00}, /* phy ctrl */ 
	{0xee, 0x02, 0x86, 0x00}, /* strength */ 
	/* pll control */ 
	{0x40, 0x8A, 0x31, 0xD2, 0x00, 0x50, 0x48, 0x63, 
	0x01, 0x0f, 0x0f, 
	0x05, 0x14, 0x03, 0x0, 0x0, 0x54, 0x06, 0x10, 0x04, 0x0},
}; 


static int mipi_cmd_hx8357_hvga_pt_init(void)
{
	int ret;

	if (msm_fb_detect_client("mipi_cmd_hx8357_smd_hvga"))
		return 0;

	printk(KERN_INFO "hx8357: detected smd cmd panel.\n");

	pinfo.xres = 320;
	pinfo.yres = 480;
	pinfo.type = MIPI_VIDEO_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	pinfo.lcdc.h_back_porch = 60;
	pinfo.lcdc.h_front_porch = 60;
	pinfo.lcdc.h_pulse_width = 8;
	pinfo.lcdc.v_back_porch = 20;
	pinfo.lcdc.v_front_porch = 20;
	pinfo.lcdc.v_pulse_width = 1;

	pinfo.lcdc.border_clr = 0;	/* blk */
	pinfo.lcdc.underflow_clr = 0xff;	/* blue */
	pinfo.lcdc.hsync_skew = 0;

#if defined (CONFIG_MACH_ROY) || defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
	pinfo.bl_max = 255;
#else
	pinfo.bl_max = 100;
#endif
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;

	pinfo.clk_rate = 400000000;//270000000;//332000000;

	pinfo.lcd.vsync_enable = TRUE;
	pinfo.lcd.hw_vsync_mode = TRUE;
	pinfo.lcd.refx100 = 6000; /* adjust refx100 to prevent tearing */
	pinfo.lcd.v_back_porch = 20;
	pinfo.lcd.v_front_porch = 20;
	pinfo.lcd.v_pulse_width = 1;

	pinfo.mipi.mode = DSI_VIDEO_MODE;
	pinfo.mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_RGB;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = FALSE;
	pinfo.mipi.t_clk_post = 0x20;
	pinfo.mipi.t_clk_pre = 0x2F;
	pinfo.mipi.stream = 0; /* dma_p */
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_NONE;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.te_sel = 1; /* TE from vsync gpio */
	pinfo.mipi.interleave_max = 1;
	pinfo.mipi.insert_dcs_cmd = TRUE;
	pinfo.mipi.wr_mem_continue = 0x3c;
	pinfo.mipi.wr_mem_start = 0x2c;
	pinfo.mipi.dsi_phy_db = &dsi_cmd_mode_phy_db;
	pinfo.mipi.tx_eot_append = 0x01;
	pinfo.mipi.rx_eot_ignore = 0x0;

	pinfo.mipi.pulse_mode_hsa_he = TRUE;
	pinfo.mipi.hfp_power_stop = FALSE; /* LP-11 during the HFP period */
	pinfo.mipi.hbp_power_stop = FALSE; /* LP-11 during the HBP period */
	pinfo.mipi.hsa_power_stop = FALSE; /* LP-11 during the HSA period */
	pinfo.mipi.eof_bllp_power_stop = TRUE;
	pinfo.mipi.bllp_power_stop = TRUE;
	pinfo.mipi.traffic_mode = DSI_BURST_MODE;
	pinfo.mipi.frame_rate = 60; /* FIXME */


	if( read_id == HX8357D_PANEL_SDC )
	{
		ret = mipi_hx8357_device_register(&pinfo, MIPI_DSI_PRIM, MIPI_DSI_PANEL_WVGA_PT,
			hx8357_smd_cmd_prepare_panel, ARRAY_SIZE(hx8357_smd_cmd_prepare_panel));
	}
	else if( read_id == HX8357D_PANEL_BOE )
	{
		ret = mipi_hx8357_device_register(&pinfo, MIPI_DSI_PRIM, MIPI_DSI_PANEL_WVGA_PT,
			hx8357_boe_cmd_prepare_panel, ARRAY_SIZE(hx8357_boe_cmd_prepare_panel));
	}
	else
	{
		ret = mipi_hx8357_device_register(&pinfo, MIPI_DSI_PRIM, MIPI_DSI_PANEL_WVGA_PT,
			hx8357_boe_cmd_prepare_panel, ARRAY_SIZE(hx8357_boe_cmd_prepare_panel));
	}

	if (ret)
		pr_err("%s: failed to register device!\n", __func__);

	return ret;
}

module_init(mipi_cmd_hx8357_hvga_pt_init);
