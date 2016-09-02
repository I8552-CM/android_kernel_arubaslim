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

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/lcd.h>
#include <mach/gpio.h>
#include <mach/pmic.h>
#include <linux/irq.h>

#include "msm_fb.h"

#include "lcdc_backlight_ic.h"
#include "lcdc_ILI9486L_hvga.h"

#define LCDC_ILI9486L_PANEL_NAME		"lcdc_ILI9486L_hvga"

static int spi_cs;
static int spi_sclk;
static int spi_sdi;
static int spi_sdo;

static int lcd_reset;

static int first_boot = 1;

static struct disp_state_type disp_state = { 0 };
static struct msm_panel_common_pdata *lcdc_ili9486l_pdata;

static DEFINE_MUTEX(spi_mutex);

#define DEFAULT_USLEEP	1

#define RDID1			0xDA
#define RDID2			0xDB
#define RDID3			0xDC

#define ESD_RECOVERY
#ifdef ESD_RECOVERY
static unsigned int lcd_det_irq;
static struct delayed_work lcd_reset_work;
static boolean irq_disabled = FALSE;
static boolean wa_first_irq = FALSE;
#endif


static void read_ldi_register(u8 addr, u8 *buf, int count)
{
	long i;

	gpio_set_value(spi_cs, 1);
	udelay(DEFAULT_USLEEP);
	gpio_set_value(spi_sclk, 1);
	udelay(DEFAULT_USLEEP);

	/* Write Command */
	gpio_set_value(spi_cs, 0);
	udelay(DEFAULT_USLEEP);
	gpio_set_value(spi_sclk, 0);
	udelay(DEFAULT_USLEEP);
	gpio_set_value(spi_sdi, 0);
	udelay(DEFAULT_USLEEP);

	gpio_set_value(spi_sclk, 1);
	udelay(DEFAULT_USLEEP);

   	for (i = 7; i >= 0; i--) {
		gpio_set_value(spi_sclk, 0);
		udelay(DEFAULT_USLEEP);
		if ((addr >> i) & 0x1)
			gpio_set_value(spi_sdi, 1);
		else
			gpio_set_value(spi_sdi, 0);
		udelay(DEFAULT_USLEEP);
		gpio_set_value(spi_sclk, 1);
		udelay(DEFAULT_USLEEP);
	}
	
#if defined(CONFIG_MACH_TREBONDUOS_CTC)
        gpio_set_value(spi_sdi, 0);
#else
	/* swith input */
	gpio_direction_input(spi_sdi);
#endif

	if(count > 1) {
		/* dummy clock cycle */
		gpio_set_value(spi_sclk, 0);
		udelay(DEFAULT_USLEEP);
		gpio_set_value(spi_sclk, 1);
		udelay(DEFAULT_USLEEP);
	}

	/* Read Parameter */
	if (count > 0) {
		long j;
		for (j = 0; j < count; j++) {
			for (i = 7; i >= 0; i--) {
				gpio_set_value(spi_sclk, 0);
				udelay(DEFAULT_USLEEP);
				/* read bit */
#if defined(CONFIG_MACH_TREBONDUOS_CTC)
			        if(gpio_get_value(spi_sdo))
#else
				if(gpio_get_value(spi_sdi))
#endif
				{ buf[j] |= (0x1<<i); }
                                
				else
					buf[j] &= ~(0x1<<i);
				gpio_set_value(spi_sclk, 1);
				udelay(DEFAULT_USLEEP);
			}
		}
	}

	gpio_set_value(spi_cs, 1);
	udelay(DEFAULT_USLEEP);

	/* switch output */
	gpio_direction_output(spi_sdi, 0);
}

static void spi_cmds_tx(struct spi_cmd_desc *desc, int cnt)
{
	long i, j, p;

	mutex_lock(&spi_mutex);
	for (p = 0; p < cnt; p++) {
		gpio_set_value(spi_cs, 1);
		udelay(DEFAULT_USLEEP);
		gpio_set_value(spi_sclk, 1);
		udelay(DEFAULT_USLEEP);

		/* Write Command */
		gpio_set_value(spi_cs, 0);
		udelay(DEFAULT_USLEEP);
		gpio_set_value(spi_sclk, 0);
		udelay(DEFAULT_USLEEP);
		gpio_set_value(spi_sdi, 0);
		udelay(DEFAULT_USLEEP);

		gpio_set_value(spi_sclk, 1);
		udelay(DEFAULT_USLEEP);

	   	for (i = 7; i >= 0; i--) {
			gpio_set_value(spi_sclk, 0);
			udelay(DEFAULT_USLEEP);
			if (((char)*(desc+p)->payload >> i) & 0x1)
				gpio_set_value(spi_sdi, 1);
			else
				gpio_set_value(spi_sdi, 0);
			udelay(DEFAULT_USLEEP);
			gpio_set_value(spi_sclk, 1);
			udelay(DEFAULT_USLEEP);
		}

		gpio_set_value(spi_cs, 1);
		udelay(DEFAULT_USLEEP);

		/* Write Parameter */
		if ((desc+p)->dlen < 2)
			goto tx_done;

		for (j = 1; j < (desc+p)->dlen; j++) {
			gpio_set_value(spi_cs, 0);
			udelay(DEFAULT_USLEEP);

			gpio_set_value(spi_sclk, 0);
			udelay(DEFAULT_USLEEP);
			gpio_set_value(spi_sdi, 1);
			udelay(DEFAULT_USLEEP);
			gpio_set_value(spi_sclk, 1);
			udelay(DEFAULT_USLEEP);

			for (i = 7; i >= 0; i--) {
				gpio_set_value(spi_sclk, 0);
				udelay(DEFAULT_USLEEP);
				if (((char)*((desc+p)->payload+j) >> i) & 0x1)
					gpio_set_value(spi_sdi, 1);
				else
					gpio_set_value(spi_sdi, 0);
				udelay(DEFAULT_USLEEP);
				gpio_set_value(spi_sclk, 1);
				udelay(DEFAULT_USLEEP);
			}

			gpio_set_value(spi_cs, 1);
			udelay(DEFAULT_USLEEP);
		}
tx_done :
		if ((desc+p)->wait)
			msleep((desc+p)->wait);
	}
	mutex_unlock(&spi_mutex);
}

static void read_lcd_id()
{
	unsigned char data[4] = {0, };

	read_ldi_register(RDID1, &data[0], 1);
	read_ldi_register(RDID2, &data[1], 1);
	read_ldi_register(RDID3, &data[2], 1);

	printk("ldi mtpdata: %x %x %x\n", data[0], data[1], data[2]);
}

static void spi_init(void)
{
	/* Set the output so that we dont disturb the slave device */
	gpio_set_value(spi_sclk, 0);
	gpio_set_value(spi_sdi, 0);

	/* Set the Chip Select De-asserted */
	gpio_set_value(spi_cs, 1);

}

static void ili9486l_disp_reset(int normal)
{
	gpio_tlmm_config(GPIO_CFG(lcd_reset, 0, GPIO_CFG_OUTPUT
				, GPIO_CFG_NO_PULL
			 	, GPIO_CFG_2MA)
			 	, GPIO_CFG_ENABLE);

	gpio_set_value(lcd_reset, 1);
	msleep(10);
	gpio_set_value(lcd_reset, 0);
	msleep(50);
	gpio_set_value(lcd_reset, 1);
	msleep(120);
}

static void ili9486l_disp_on(void)
{
	printk("start %s\n", __func__);

	if (disp_state.disp_powered_up && !disp_state.display_on) {
		spi_cmds_tx(display_on_cmds, ARRAY_SIZE(display_on_cmds));
		
		msleep(30);
 
		disp_state.display_on = TRUE;
	}
}

static void ili9486l_disp_powerup(void)
{
	printk("start %s\n", __func__);

	if (!disp_state.disp_powered_up && !disp_state.display_on) {
		ili9486l_disp_reset(1);
		disp_state.disp_powered_up = TRUE;
	}
}

static void ili9486l_disp_powerdown(void)
{
	printk("start %s\n", __func__);
	disp_state.disp_powered_up = FALSE;

	gpio_tlmm_config(GPIO_CFG(lcd_reset, 0, GPIO_CFG_OUTPUT
				, GPIO_CFG_NO_PULL
			 	, GPIO_CFG_2MA)
			 	, GPIO_CFG_ENABLE);

	gpio_set_value(lcd_reset, 1);
	msleep(10);
	gpio_set_value(lcd_reset, 0);
	msleep(500);
	gpio_set_value(lcd_reset, 1);
	
	gpio_set_value(spi_sclk, 0);
	gpio_set_value(spi_sdi, 0);
}

static int lcdc_ILI9486L_panel_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	
	printk("start %s\n", __func__);

#ifdef ESD_RECOVERY
	printk("start ESD_RECOVERY\n");

		if (irq_disabled) {
			enable_irq(lcd_det_irq);
			irq_disabled = FALSE;
		}
#endif

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	
	if (!disp_state.disp_initialized) {
		/* Configure reset GPIO that drives DAC */
		lcdc_ili9486l_pdata->panel_config_gpio(1);
		
		spi_init(); /* LCD needs SPI *//* 12.03.19 daesu.jeong Long-time loop fix*/

		ili9486l_disp_powerup();
		
		ili9486l_disp_on();

		disp_state.disp_initialized = TRUE;
		
		//crash booting logo
		if(first_boot)
		{	
			first_boot = 0;
			return 0;
		}

		backlight_ic_set_brightness(mfd->bl_level);
	}

	return 0;
}


static int lcdc_ILI9486L_panel_off(struct platform_device *pdev)
{
	printk("start %s\n", __func__);
	if (disp_state.disp_powered_up && disp_state.display_on) {
#ifdef ESD_RECOVERY
  		disable_irq_nosync(lcd_det_irq);
  		irq_disabled = TRUE;
#endif
		lcdc_ili9486l_pdata->panel_config_gpio(0);
		disp_state.display_on = FALSE;
		disp_state.disp_initialized = FALSE;
		backlight_ic_set_brightness(0);
		spi_cmds_tx(display_off_cmds, ARRAY_SIZE(display_off_cmds));
		ili9486l_disp_powerdown();
	}
	return 0;
}

#ifdef ESD_RECOVERY
static irqreturn_t trebon_disp_breakdown_det(int irq, void *handle)
{
	if(disp_state.disp_initialized)
		schedule_delayed_work(&lcd_reset_work, 0);

	return IRQ_HANDLED;
}

static void lcdc_dsip_reset_work(struct work_struct *work_ptr)
{
	if (!wa_first_irq) {
		printk("skip lcd reset\n");
		wa_first_irq = TRUE;
		return;
	}

	printk("lcd reset\n");

	disable_irq_nosync(lcd_det_irq);
//LCD OFF
	lcdc_ili9486l_pdata->panel_config_gpio(0);
	disp_state.display_on = FALSE;
	disp_state.disp_initialized = FALSE;
	spi_cmds_tx(display_off_cmds, ARRAY_SIZE(display_off_cmds));
	disp_state.disp_powered_up = FALSE;
	printk("lcd off\n");


// LCD ON
	ili9486l_disp_powerup();
        msleep(1000);

	lcdc_ili9486l_pdata->panel_config_gpio(1);		
	ili9486l_disp_on();
	disp_state.disp_initialized = TRUE;
	printk("lcd on\n");

	enable_irq(lcd_det_irq);

        disp_state.disp_initialized = TRUE;

	wa_first_irq = FALSE;

}//LCD DETECT Fix  
#endif


static void lcdc_ILI9486L_set_backlight(struct msm_fb_data_type *mfd)
{
	int level = mfd->bl_level;

	printk("BL : %d, disp_on[%d]\n", level, disp_state.display_on);

	if(!disp_state.display_on)
		return;

        /* function will spin lock */
	backlight_ic_set_brightness(level);
}

static void ili9486l_disp_shutdown(struct platform_device *pdev)
{
	lcdc_ILI9486L_panel_off(pdev);
}

static int ili9486l_disp_set_power(struct lcd_device *dev, int power)
{
	printk("ili9486l_disp_set_power\n");
	return 0;
}

static int ili9486l_disp_get_power(struct lcd_device *dev, int power)
{
	printk("ili9486l_disp_get_power(%d)\n", disp_state.disp_initialized);
	return disp_state.disp_initialized;
}

static struct lcd_ops ili9486l_lcd_props = {
	.get_power = ili9486l_disp_get_power,
	.set_power = ili9486l_disp_set_power,
};

static ssize_t trebon_lcdtype_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	char temp[30];
	sprintf(temp, "BOE_BT035HVMG003-A301\n");
	strcat(buf, temp);
	return strlen(buf);	
}

static DEVICE_ATTR(lcd_type, S_IRUGO, trebon_lcdtype_show, NULL);


static int __devinit ili9486l_disp_probe(struct platform_device *pdev)
{
	struct lcd_device *lcd_device;
        int ret;

	printk("[%s] id=%d\n", __func__, pdev->id);
	if (pdev->id == 0) {
		
		int i;
		
		disp_state.disp_initialized = FALSE;
		disp_state.disp_powered_up = FALSE;
		disp_state.display_on = FALSE;

		lcdc_ili9486l_pdata = pdev->dev.platform_data;
		spi_sclk = *(lcdc_ili9486l_pdata->gpio_num);
		spi_cs	 = *(lcdc_ili9486l_pdata->gpio_num + 1);
		spi_sdo = *(lcdc_ili9486l_pdata->gpio_num + 2); 
		spi_sdi  = *(lcdc_ili9486l_pdata->gpio_num + 3);
		lcd_reset= *(lcdc_ili9486l_pdata->gpio_num + 4);

		spi_init();

		lcd_device = lcd_device_register("panel", &pdev->dev, NULL,
						&ili9486l_lcd_props);
		
		ret = sysfs_create_file(&lcd_device->dev.kobj, &dev_attr_lcd_type.attr);
        	if (ret)
		          printk(KERN_ERR "sysfs create fail - %s\n",dev_attr_lcd_type.attr.name);

		if (IS_ERR(lcd_device)) {
			ret = PTR_ERR(lcd_device);
			printk(KERN_ERR "lcd : failed to register device\n");
			return ret;
		}
		
//		ret = sysfs_create_file(&lcd_device->dev.kobj, &dev_attr_lcd_type.attr);
//		if (ret) {
//			printk("sysfs create fail - %s\n", dev_attr_lcd_type.attr.name);
//		}


#ifdef ESD_RECOVERY
		for (i = 0; i < pdev->num_resources; i++) {
			if (!strncmp(pdev->resource[i].name,"lcd_breakdown_det", 17)) {
				lcd_det_irq = pdev->resource[i].start;
				if (!lcd_det_irq) {
					printk(KERN_ERR "LCD_DETECT_IRQ is NULL!\n");
						}
					}
				}

		printk("lcd_det_irq = %d\n", lcd_det_irq);
#endif


		return 0;
	}

	msm_fb_add_device(pdev);


#ifdef ESD_RECOVERY
	INIT_DELAYED_WORK(&lcd_reset_work, lcdc_dsip_reset_work);

	ret = request_irq(lcd_det_irq, trebon_disp_breakdown_det, IRQF_TRIGGER_FALLING, "lcd_esd_det", NULL);
	if (ret) {
		pr_err("Request_irq failed for TLMM_MSM_SUMMARY_IRQ - %d\n",ret);
		return ret;
	}

	printk("lRequest_irq successed\n");
#endif

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = ili9486l_disp_probe,
	.shutdown	= ili9486l_disp_shutdown,
	.driver = {
		.name   = LCDC_ILI9486L_PANEL_NAME,
	},
};

static struct msm_fb_panel_data ILI9486L_panel_data = {
	.on = lcdc_ILI9486L_panel_on,
	.off = lcdc_ILI9486L_panel_off,
	.set_backlight = lcdc_ILI9486L_set_backlight,
};

static struct platform_device this_device = {
	.name   = "lcdc_ILI9486L_hvga",
	.id	= 1,
	.dev	= {
		.platform_data = &ILI9486L_panel_data,
	}
};

#define LCDC_HBP	   18
#define LCDC_HPW	   2
#define LCDC_HFP	   26
#define LCDC_VBP	   8
#define LCDC_VPW	   2
#define LCDC_VFP	  12
#define LCDC_BPP	  18
#define LCDC_PCLK	12288000
#define LCDC_FB_XRES	320
#define LCDC_FB_YRES	480

static int __init lcdc_ILI9486L_panel_init(void)
{
	int ret;
	struct msm_panel_info *pinfo;

#ifdef CONFIG_FB_MSM_TRY_MDDI_CATCH_LCDC_PRISM
	ret = msm_fb_detect_client("lcdc_ILI9486L_hvga");
	if (ret) {
		printk(KERN_ERR "%s:msm_fb_detect_client failed!\n", __func__);
		return 0;
	}
#endif

	ret = platform_driver_register(&this_driver);
	if (ret)
		return ret;
		
/*
* Read panel id and update the function for each panel
*/

	pinfo = &ILI9486L_panel_data.panel_info;
	pinfo->xres = LCDC_FB_XRES;
	pinfo->yres = LCDC_FB_YRES;
	MSM_FB_SINGLE_MODE_PANEL(pinfo);
	pinfo->type = LCDC_PANEL;
	pinfo->pdest = DISPLAY_1;
	pinfo->wait_cycle = 0;
	pinfo->bpp = LCDC_BPP;
	pinfo->fb_num = 2;
	pinfo->clk_rate = LCDC_PCLK; /* (9388 * 1000); */
	pinfo->bl_max = 255;
	pinfo->bl_min = 1;
	pinfo->lcdc.h_back_porch = LCDC_HBP;

	pinfo->lcdc.h_front_porch = LCDC_HFP;
	pinfo->lcdc.h_pulse_width = LCDC_HPW;
	pinfo->lcdc.v_back_porch = LCDC_VBP;
	pinfo->lcdc.v_front_porch = LCDC_VFP;
	pinfo->lcdc.v_pulse_width = LCDC_VPW;
	pinfo->lcdc.border_clr = 0;     /* blk */
	pinfo->lcdc.underflow_clr = 0xff;       /* blue */
	pinfo->lcdc.hsync_skew = 0;

	ret = platform_device_register(&this_device);
	if (ret) {
		printk(KERN_ERR "%s not able to register the device\n",
			 __func__);
		platform_driver_unregister(&this_driver);
	}

	return ret;
}


module_init(lcdc_ILI9486L_panel_init);
