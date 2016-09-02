/*
 * fsa_microusb.c - FSA9480, FSA88x micro USB switch device driver
 *
 * Copyright (C) 2011 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/i2c/fsa9480.h>
//#include <linux/fsaxxxx_usbsw.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/platform_data/fsa9480.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/sec_param.h>

#include <asm/io.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>

#include <linux/delay.h>

#if defined(CONFIG_SEC_FSA_JACK)
#include <linux/input.h>
#endif

/* FSA9480 I2C registers */
#define FSA9480_REG_DEVID		0x01
#define FSA9480_REG_CTRL		0x02
#define FSA9480_REG_INT1		0x03
#define FSA9480_REG_INT2		0x04
#define FSA9480_REG_INT1_MASK	0x05
#define FSA9480_REG_INT2_MASK	0x06
#define FSA9480_REG_ADC			0x07
#define FSA9480_REG_TIMING1		0x08
#define FSA9480_REG_TIMING2		0x09
#define FSA9480_REG_DEV_T1		0x0a
#define FSA9480_REG_DEV_T2		0x0b
#define FSA9480_REG_BTN1		0x0c
#define FSA9480_REG_BTN2		0x0d
#define FSA9480_REG_CK			0x0e
#define FSA9480_REG_CK_INT1		0x0f
#define FSA9480_REG_CK_INT2		0x10
#define FSA9480_REG_CK_INTMASK1	0x11
#define FSA9480_REG_CK_INTMASK2	0x12
#define FSA9480_REG_MANSW1		0x13
#define FSA9480_REG_MANSW2		0x14
#define FSA9480_REG_VBUS                0x1D

/* FSA88x I2C registers */
#define FSA880_REG_DEVID		FSA9480_REG_DEVID
#define FSA880_REG_CTRL			FSA9480_REG_CTRL
#define FSA880_REG_INT1			FSA9480_REG_INT1
#define FSA880_REG_ADC			FSA9480_REG_ADC
#define FSA880_REG_DEV_T1		FSA9480_REG_DEV_T1
#define FSA880_REG_DEV_T2		FSA9480_REG_DEV_T2
#define FSA880_REG_MANSW1		FSA9480_REG_MANSW1
#define FSA880_REG_MANSW2		FSA9480_REG_MANSW2
#define FSA880_REG_RESET		0x1b

/* FSAxxx I2C registers */
#define FSA_REG_DEVID		FSA9480_REG_DEVID
#define FSA_REG_CTRL		FSA9480_REG_CTRL
#define FSA_REG_INT1		FSA9480_REG_INT1
#define FSA_REG_INT2		FSA9480_REG_INT2
#define FSA_REG_ADC		FSA9480_REG_ADC
#define FSA_REG_DEV_T1		FSA9480_REG_DEV_T1
#define FSA_REG_DEV_T2		FSA9480_REG_DEV_T2
#define FSA_REG_MANSW1		FSA9480_REG_MANSW1
#define FSA_REG_MANSW2		FSA9480_REG_MANSW2
#define FSA_REG_RESET		FSA880_REG_RESET


/* Device ID */
#define FSA9480_ID	0
#define FSA9280_ID	1
#define FSA880_ID	2

/* Control */
#define SWITCH_OPEN		(1 << 4)
#define RAW_DATA		(1 << 3)
#define MANUAL_SWITCH	(1 << 2)
#define WAIT			(1 << 1)
#define INT_MASK		(1 << 0)

/* Device Type 1 */
#define DEV_USB_OTG			(1 << 7)
#define DEV_DEDICATED_CHG	(1 << 6)
#define DEV_USB_CHG			(1 << 5)
#define DEV_CAR_KIT			(1 << 4)
#define DEV_UART			(1 << 3)
#define DEV_USB				(1 << 2)
#define DEV_AUDIO_2			(1 << 1)
#define DEV_AUDIO_1			(1 << 0)

/* Device Type 2 */
#define DEV_UNKNOWN			(1 << 7)
#define DEV_AV					(1 << 6)
#define DEV_TTY				(1 << 5)
#define DEV_PPD				(1 << 4)
#define DEV_JIG_UART_OFF			(1 << 3)
#define DEV_JIG_UART_ON			(1 << 2)
#define DEV_JIG_USB_OFF			(1 << 1)
#define DEV_JIG_USB_ON			(1 << 0)

/* MANSW1 */
#if defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
//vaudio pins connected to gsm usb, so Vbus_in must connected to Vbus_out in standard usb mode
#define VAUDIO	0x93
#else
#define VAUDIO	0x90
#endif
#define UART	0x6c
#define AUDIO	0x4B
#define DHOST	0x24
#define AUTO	0x0

/*Int1 mask*/
#define LONG_KEY_PRESS  (1<<3)
#define KEY_PRESS 		(1<<2)
#define DEV_DETACH 		(1<<1)
#define DEV_ATTACH 		(1<<0)






/* Ctrl Mask & Reset Value */
#define CTRL_MASK		(SWITCH_OPEN | RAW_DATA | MANUAL_SWITCH | \
		WAIT | INT_MASK)
#define FSA880_REG_DEV_CTRL_MASK	(MANUAL_SWITCH | INT_MASK)
#define FSA880_REG_CTRL_RESET_VALUE		0x04
#define FSA9480_REG_CTRL_RESET_VALUE		0x1E

/* 9480 Type */
#define FSA9480_DEV_T1_USB_MASK		(DEV_USB | DEV_USB_CHG ) 

#define FSA9480_DEV_T1_UART_MASK	(DEV_UART)
#define FSA9480_DEV_T1_CHARGER_MASK	(DEV_DEDICATED_CHG | DEV_CAR_KIT)
#define FSA9480_DEV_T1_AUDIO1_MASK  (DEV_AUDIO_1)

#define FSA9480_DEV_T2_USB_MASK		(DEV_JIG_USB_OFF | DEV_JIG_USB_ON)
#define FSA9480_DEV_T2_UART_MASK	(DEV_JIG_UART_OFF | DEV_JIG_UART_ON)
#define FSA9480_DEV_T2_JIG_MASK		(DEV_JIG_USB_OFF | DEV_JIG_USB_ON | \
		DEV_JIG_UART_OFF | DEV_JIG_UART_ON)
/* 9280 Type */
#define FSA880_REG_MANSW1_MASK	0x6c

#define FSA880_REG_DEV_T1_MASK	(DEV_DEDICATED_CHG | DEV_USB_CHG | DEV_CAR_KIT | DEV_USB)
#define FSA880_REG_DEV_T2_MASK		(DEV_UNKNOWN | DEV_JIG_UART_OFF | \
		DEV_JIG_UART_ON | DEV_JIG_USB_OFF | DEV_JIG_USB_ON)

#define FSA880_DEV_T1_USB_MASK			(DEV_USB | DEV_USB_CHG ) 
#define FSA880_DEV_T1_UART_MASK			0
#define FSA880_DEV_T1_CHARGER_MASK		(DEV_DEDICATED_CHG | DEV_CAR_KIT )

#define FSA880_DEV_T2_USB_MASK			(DEV_JIG_USB_OFF | DEV_JIG_USB_ON)
#define FSA880_DEV_T2_UART_MASK			(DEV_JIG_UART_OFF | DEV_JIG_UART_ON)
#define FSA880_DEV_T2_JIG_MASK			(DEV_JIG_USB_OFF | DEV_JIG_USB_ON | \
										DEV_JIG_UART_OFF | DEV_JIG_UART_ON)


////////////////////////////////////////////////////////////////////////////////////////////

static int FSA_DEV_T1_USB_MASK = 0;
static int FSA_DEV_T1_CHARGER_MASK = 0;
static int FSA_DEV_T2_USB_MASK = 0;
static int FSA_DEV_T2_UART_MASK = 0;
static int FSA_DEV_T2_JIG_MASK = 0;
static int FSA_DEV_T1_AUDIO1_MASK = 0;
static int FSA_RESET_VALUE = 0;

#if defined(CONFIG_MACH_KYLE_I) ||defined(CONFIG_MACH_ARUBA_OPEN) || defined(CONFIG_MACH_KYLE_CHN)|| defined(CONFIG_MACH_ARUBA_CTC)|| defined(CONFIG_MACH_KYLEPLUS_OPEN)|| defined(CONFIG_MACH_KYLEPLUS_CTC) \
	|| defined(CONFIG_MACH_DELOS_DUOS_CTC) || defined(CONFIG_MACH_HENNESSY_DUOS_CTC) || defined(CONFIG_MACH_NEVIS3G) || defined(CONFIG_MACH_ARUBASLIM_OPEN)
#define DESK_DOCK_CHARGING_ENABLE_9280 /* since certain dock couldn't be recognized as a charger */
#endif

#if defined(CONFIG_MACH_DELOS_OPEN)
#define DESK_DOCK_CHARGING_ENABLE_880 /* since certain dock couldn't be recognized as a charger */
#endif

#if defined(DESK_DOCK_CHARGING_ENABLE_9280)
u8 intr2_av  = 0;
#endif

#if defined(DESK_DOCK_CHARGING_ENABLE_880)
u8 deskdock_attached  = 0;
#endif

#define USB_SEL_MASK 0x0F
#define UART_SEL_MASK 0xF0
#define USB_SEL_PDA_MASK (1<<0)
#define USB_SEL_MODEM_MASK (1<<1)
#define USB_SEL_ESC_MASK (1<<2)
#define UART_SEL_PDA_MASK ((1<<0)<<4)
#define UART_SEL_MODEM_MASK ((1<<1)<<4)
#define UART_SEL_ESC_MASK ((1<<2)<<4)
#define USB_SWITCH_TRASH_BIT 0x08
#define UART_SWITCH_TRASH_BIT 0x80

int uart_connecting = 0;
EXPORT_SYMBOL(uart_connecting);

#if defined(CONFIG_SEC_FSA_JACK)
extern void jack_status_change(int attached);
extern void earkey_status_change(int pressed, int code);
#endif

extern unsigned int board_hw_revision;

struct fsa9480_info {
	struct i2c_client *client;
	struct fsa9480_platform_data *pdata;
	struct work_struct work;
	int dev1;
	int dev2;
	int mansw;
	unsigned int id;
	const char *name;
	unsigned int switch_sel;
};

#ifdef CONFIG_SEC_DUAL_MODEM
struct fsa9480_wq {
	struct delayed_work work_q;
	struct fsa9480_info *sdata;
	struct list_head entry;
};
#endif

struct fsa9480_delay_wq {
	struct delayed_work work_q;
	struct fsa9480_info *sdata;
	struct list_head entry;
};

static struct fsa9480_info *chip;
static struct fsa9480_ops ops;
static int charger_cb_flag;
static int ovp_cb_flag;
int usb_state_flag;
#if defined(CONFIG_USB_DCHUN_MAC)
extern void set_mac_connected(bool mac_connected);
#endif

#ifdef CONFIG_SEC_DUAL_MODEM
static int curr_usb_path = 0;;
static int curr_uart_path = 0;
#define SWITCH_PDA		0
#define SWITCH_MODEM	1
#define SWITCH_ESC		2


#if defined(CONFIG_MACH_INFINITE_DUOS_CTC) || defined(CONFIG_MACH_ARUBA_DUOS_CTC) || defined(CONFIG_MACH_DELOS_DUOS_CTC) \
	|| defined(CONFIG_MACH_BAFFIN_DUOS_CTC) || defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
#if defined(CONFIG_MACH_INFINITE_DUOS_CTC) || defined(CONFIG_MACH_ARUBA_DUOS_CTC)
#define GPIO_UART_SEL 119
#elif defined(CONFIG_MACH_DELOS_DUOS_CTC)
#define GPIO_UART_SEL 17
#define GPIO_UART_SEL_REV04 30
#elif defined(CONFIG_MACH_BAFFIN_DUOS_CTC)
#define GPIO_UART_SEL 59
#elif defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
#define GPIO_UART_SEL 30
#endif

static void fsa9485_uart_sel_switch_en(bool en)
{
	int gpio_sel = GPIO_UART_SEL;
	
#if defined(CONFIG_MACH_INFINITE_DUOS_CTC)
	if ( board_hw_revision >= 5 )
	{				
#elif defined(CONFIG_MACH_ARUBA_DUOS_CTC)
	if ( board_hw_revision >= 8 )
	{	
#elif defined(CONFIG_MACH_BAFFIN_DUOS_CTC)
	if ( board_hw_revision >= 2 )
	{	
#elif defined(CONFIG_MACH_DELOS_DUOS_CTC)
	if ( board_hw_revision >= 2 )
	{	
		if(board_hw_revision >= 4)
			gpio_sel = GPIO_UART_SEL_REV04;
#endif
		printk("%s : en : %d, gpio_sel : %d \n",__func__, en, gpio_sel);
		gpio_direction_output(gpio_sel, en);			
		mdelay(10);
		printk("%s read GPIO %d UART_SEL : value = %d \n", __func__,gpio_sel,gpio_get_value(gpio_sel));	
#if defined(CONFIG_MACH_INFINITE_DUOS_CTC) || defined(CONFIG_MACH_ARUBA_DUOS_CTC) \
	|| defined(CONFIG_MACH_BAFFIN_DUOS_CTC) || defined(CONFIG_MACH_DELOS_DUOS_CTC)
	}
#endif
}
#endif
#endif


#if  defined(CONFIG_USB_EXTERNAL_CHARGER)
extern void set_pm_gpio_for_usb_enable(int enable);
#endif

void fsa_default_detach_handler(void)
{
	return;
};

void fsa_default_attach_handler(int d)
{
	return;
};

int fsa_microusb_connection_handler_register(
		void (*detach), void (*attach))
{
	if (detach)
		ops.detach_handler = detach;
	if (attach)
		ops.attach_handler = attach;

	if (!(detach || attach))
		return -EINVAL;

	return 0;
}

static int fsa9480_write_reg(struct i2c_client *client, u8 reg, u8 data)
{
	int ret;
	u8 buf[2];
	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = 2,
			.buf = buf,
		},
	};

	buf[0] = reg;
	buf[1] = data;

	printk("KBJ TEST : %s , register = %x , values = %x\n",__func__,reg,data);

	ret = i2c_transfer(client->adapter, msg, 1);
	if (ret != 1) {
		dev_err(&client->dev,
				"[%s] Failed (ret=%d)\n", __func__, ret);
		return -EREMOTEIO;
	}

	pr_debug("[%s] success\n", __func__);
	return 0;
}

static int fsa9480_read_reg(struct i2c_client *client, u8 reg, u8 *data)
{
	int ret;
	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = 1,
			.buf = &reg,
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = 1,
			.buf = data,
		}
	};

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret != 2) {
		dev_err(&client->dev,
				"[%s] Failed (ret=%d)\n", __func__, ret);
		return -EREMOTEIO;
	}

	pr_debug("[%s] success\n", __func__);
	return 0;
}

void fsa9480_set_switch(const char *buf)
{
	struct fsa9480_info *usbsw = chip;
	struct i2c_client *client = usbsw->client;
	unsigned int path;
	u8 value;

	printk("KBJ : %s , buf = %s\n" , __func__ , buf);
	fsa9480_read_reg(client, FSA_REG_CTRL, &value);

	if (!strncmp(buf, "VAUDIO", 6)) {
#if defined(CONFIG_MACH_INFINITE_DUOS_CTC)
		if ( board_hw_revision >= 5 )						
		{
			fsa9485_uart_sel_switch_en(1);
			path = AUTO;
			value |= MANUAL_SWITCH;		
		}
		else
		{
			path = VAUDIO;
			value &= ~MANUAL_SWITCH;
		}
#elif defined(CONFIG_MACH_ARUBA_DUOS_CTC)
		if ( board_hw_revision >= 8 )		
		{
			fsa9485_uart_sel_switch_en(1);
			path = AUTO;
			value |= MANUAL_SWITCH;		
		}
		else
		{
			path = VAUDIO;
			value &= ~MANUAL_SWITCH;
		}
#elif defined(CONFIG_MACH_BAFFIN_DUOS_CTC) || defined(CONFIG_MACH_DELOS_DUOS_CTC)
		if ( board_hw_revision >= 2 )		
		{
			fsa9485_uart_sel_switch_en(1);
			path = AUTO;
			value |= MANUAL_SWITCH;		
		}
		else
		{
			path = VAUDIO;
			value &= ~MANUAL_SWITCH;
		}
#else
		path = VAUDIO;
		value &= ~MANUAL_SWITCH;
#endif
	} else if (!strncmp(buf, "UART", 4)) {
		path = UART;
		value &= ~MANUAL_SWITCH;
	} else if (!strncmp(buf, "AUDIO", 5)) {
		path = AUDIO;
		value &= ~MANUAL_SWITCH;
	} else if (!strncmp(buf, "DHOST", 5)) {
		path = DHOST;
		value &= ~MANUAL_SWITCH;
	} else if (!strncmp(buf, "AUTO", 4)) {
#if defined(CONFIG_MACH_INFINITE_DUOS_CTC) || defined(CONFIG_MACH_ARUBA_DUOS_CTC) || defined(CONFIG_MACH_DELOS_DUOS_CTC) \
	|| defined(CONFIG_MACH_BAFFIN_DUOS_CTC)
		fsa9485_uart_sel_switch_en(0);
#endif
		path = AUTO;
		value |= MANUAL_SWITCH;
	} else {
		dev_err(&client->dev, "Wrong command\n");
		return;
	}

	if (usbsw->id == FSA880_ID)
		path &= FSA880_REG_MANSW1_MASK;

	usbsw->mansw = path;
	fsa9480_write_reg(client, FSA_REG_MANSW1, path);
	fsa9480_write_reg(client, FSA_REG_CTRL, value);
}
EXPORT_SYMBOL_GPL(fsa9480_set_switch);

ssize_t fsa9480_get_switch(char *buf)
{
	struct fsa9480_info *usbsw = chip;
	struct i2c_client *client = usbsw->client;
	u8 value;

	fsa9480_read_reg(client, FSA_REG_MANSW1, &value);

	if (usbsw->id == FSA880_ID)
		value &= FSA880_REG_MANSW1_MASK;

	if (value == VAUDIO)
		return sprintf(buf, "VAUDIO\n");
	else if (value == UART)
		return sprintf(buf, "UART\n");
	else if (value == AUDIO)
		return sprintf(buf, "AUDIO\n");
	else if (value == DHOST)
		return sprintf(buf, "DHOST\n");
	else if (value == AUTO)
		return sprintf(buf, "AUTO\n");
	else
		return sprintf(buf, "%x", value);
}
EXPORT_SYMBOL_GPL(fsa9480_get_switch);

static ssize_t fsa9480_show_status(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fsa9480_info *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	u8 devid, ctrl, adc, dev1, dev2, intr;
	u8 intmask1 = 0, intmask2 = 0, time1 = 0, time2 = 0, mansw1;

	fsa9480_read_reg(client, FSA_REG_DEVID, &devid);
	fsa9480_read_reg(client, FSA_REG_CTRL, &ctrl);
	fsa9480_read_reg(client, FSA_REG_ADC, &adc);
	fsa9480_read_reg(client, FSA_REG_DEV_T1, &dev1);
	fsa9480_read_reg(client, FSA_REG_DEV_T2, &dev2);
	fsa9480_read_reg(client, FSA_REG_MANSW1, &mansw1);

	if (usbsw->id != FSA880_ID) {
		fsa9480_read_reg(client, FSA9480_REG_INT1_MASK, &intmask1);
		fsa9480_read_reg(client, FSA9480_REG_INT2_MASK, &intmask2);
		fsa9480_read_reg(client, FSA9480_REG_TIMING1, &time1);
		fsa9480_read_reg(client, FSA9480_REG_TIMING2, &time2);
	}

	fsa9480_read_reg(client, FSA_REG_INT1, &intr);
	intr &= 0xff;

	return sprintf(buf, "%s: Device ID(%02x), CTRL(%02x)\n"
			"ADC(%02x), DEV_T1(%02x), DEV_T2(%02x)\n"
			"INT(%04x), INTMASK(%02x, %02x)\n"
			"TIMING(%02x, %02x), MANSW1(%02x)\n", usbsw->name,
			devid, ctrl, adc, dev1, dev2, intr,
			intmask1, intmask2, time1, time2, mansw1);
}

static ssize_t fsa9480_show_manualsw(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return fsa9480_get_switch(buf);
}

static ssize_t fsa9480_set_manualsw(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	fsa9480_set_switch(buf);
	return count;
}

static ssize_t fsa_show_usb_state(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct fsa9480_info *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	u8 dev1, dev2;

//	printk(KERN_ERR "%s\n", __func__);
#if defined(CONFIG_MACH_KYLE)
	printk(KERN_ERR "%s: usb_state: %d\n", __func__, usb_state_flag);
	if (usb_state_flag == 1)
		return snprintf(buf, 22, "USB_STATE_CONFIGURED\n");

	return snprintf(buf, 25, "USB_STATE_NOTCONFIGURED\n");
#else
	fsa9480_read_reg(client, FSA_REG_DEV_T1, &dev1);
	fsa9480_read_reg(client, FSA_REG_DEV_T2, &dev2);
	printk(KERN_ERR "%s: dev1: %x dev2: %x\n", __func__, dev1, dev2);
	if (dev1 & FSA_DEV_T1_USB_MASK || dev2 & FSA_DEV_T2_USB_MASK)
		return snprintf(buf, 22, "USB_STATE_CONFIGURED\n");

	return snprintf(buf, 25, "USB_STATE_NOTCONFIGURED\n");
#endif
}


#ifdef CONFIG_SEC_DUAL_MODEM
static ssize_t uart_sel_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct fsa9480_info *usbsw = dev_get_drvdata(dev);
	int uart_sel = usbsw->switch_sel & UART_SEL_MASK;
	
	printk(KERN_ERR "%s : uart_sel : %x\n", __func__, uart_sel);

	if (uart_sel & UART_SEL_PDA_MASK) {
		return sprintf(buf, "AP");
	} else if (uart_sel & UART_SEL_MODEM_MASK) {
		return sprintf(buf, "CP");
	} else if (uart_sel & UART_SEL_ESC_MASK) {
		return sprintf(buf, "ESC");
	} else {
		return sprintf(buf, "CP");
	}
}

static ssize_t uart_sel_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t size)
{
	struct fsa9480_info *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	char value[50];

	sec_get_param(param_index_uartsel,&usbsw->switch_sel);

	printk("%s : get param switch_sel = %x \n", __func__,usbsw->switch_sel);

	memset(value,0x0,sizeof(value));

	if (sscanf(buf, "%49s", value) != 1) {
		pr_err("%s : Invalid value\n", __func__);
		return -EINVAL;
	}

	printk("%s : value = %s \n", __func__,value);
	
	if (!strncmp(value,"PDA",3) || !strncmp(value,"AP",2)) {
		fsa9480_set_switch("AUTO");
		usbsw->switch_sel &= ~UART_SEL_MASK;
		usbsw->switch_sel |= UART_SEL_PDA_MASK;
		curr_uart_path = SWITCH_PDA;
	} else if (!strncmp(value, "MODEM",5) || !strncmp(value, "CP",2)) {
		fsa9480_set_switch("AUTO");
		usbsw->switch_sel &= ~UART_SEL_MASK;
		usbsw->switch_sel |= UART_SEL_MODEM_MASK;
		curr_uart_path = SWITCH_MODEM;
	} else if (!strncmp(value,"ESC",3)) {
	#if defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
		fsa9480_set_switch("AUTO");
	#else
		fsa9480_set_switch("VAUDIO");
	#endif
		usbsw->switch_sel &= ~UART_SEL_MASK;
		usbsw->switch_sel |= UART_SEL_ESC_MASK;
		curr_uart_path = SWITCH_ESC;
	} else {
		fsa9480_set_switch("AUTO");
		usbsw->switch_sel &= ~UART_SEL_MASK;
		usbsw->switch_sel |= UART_SEL_MODEM_MASK;
		curr_uart_path = SWITCH_MODEM;		
	}

#if defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
	if(curr_uart_path == SWITCH_ESC)
	{
		fsa9485_uart_sel_switch_en(1); // 1 - for GSM
	}
	else
	{
		fsa9485_uart_sel_switch_en(0); // 0 - for CDMA
	}
#endif

	printk("%s : usbsw->switch_sel = 0x%x \n", __func__,usbsw->switch_sel);
	sec_set_param(param_index_uartsel, &usbsw->switch_sel);

	return size;

}

static ssize_t is_factory_mode_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t size)
{
	char value[50];

	memset(value,0x0,sizeof(value));

	if (sscanf(buf, "%49s", value) != 1) {
		pr_err("%s : Invalid value\n", __func__);
		return -EINVAL;
	}
	printk("%s : value = %s \n", __func__,value);
	
	if ((!strcmp(value, "1")) || (!strncmp(value,"1",1))) {
		curr_uart_path = SWITCH_PDA;
		fsa9480_set_switch("AUTO");
	}

	return size;
}

static ssize_t uart_sel_nsave_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t size)
{
	char value[50];

	memset(value,0x0,sizeof(value));

	if (sscanf(buf, "%49s", value) != 1) {
		pr_err("%s : Invalid value\n", __func__);
		return -EINVAL;
	}

	printk("%s : value = %s \n", __func__,value);
	
	if (!strncmp(value,"PDA",3) || !strncmp(value,"AP",2)) {
		fsa9480_set_switch("AUTO");
		curr_uart_path = SWITCH_PDA;
	} else if (!strncmp(value, "MODEM",5) || !strncmp(value, "CP",2)) {
		fsa9480_set_switch("AUTO");
		curr_uart_path = SWITCH_MODEM;
	} else if (!strncmp(value,"ESC",3)) {	
	#if defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
		fsa9480_set_switch("AUTO");
	#else	
		fsa9480_set_switch("VAUDIO");
	#endif
		curr_uart_path = SWITCH_ESC;
	} else {
		fsa9480_set_switch("AUTO");
		curr_uart_path = SWITCH_MODEM;		
	}

#if defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
		if(curr_uart_path == SWITCH_ESC)
		{
			fsa9485_uart_sel_switch_en(1); // 1 - for GSM
		}
		else
		{
			fsa9485_uart_sel_switch_en(0); // 0 - for CDMA
		}
#endif

	return size;
}

static ssize_t usb_sel_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct fsa9480_info *usbsw = dev_get_drvdata(dev);
	int usb_sel = usbsw->switch_sel & USB_SEL_MASK;

	printk(KERN_ERR "%s : usb_sel : %x\n", __func__, usb_sel);

	if (usb_sel & USB_SEL_PDA_MASK) {
		return sprintf(buf, "PDA");
	} else if (usb_sel & USB_SEL_MODEM_MASK) {
		return sprintf(buf, "MODEM");
	} else if (usb_sel & USB_SEL_ESC_MASK) {
		return sprintf(buf, "ESC");
	} else {
		return sprintf(buf, "PDA");
	}
}

static ssize_t usb_sel_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t size)
{
	struct fsa9480_info *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	char value[50];

	sec_get_param(param_index_uartsel,&usbsw->switch_sel);

	printk("%s : get param switch_sel = %x \n", __func__,usbsw->switch_sel);

	memset(value,0x0,sizeof(value));

	if (sscanf(buf, "%49s", value) != 1) {
		pr_err("%s : Invalid value\n", __func__);
		return -EINVAL;
	}

	printk("%s : value = %s \n", __func__,value);

	if (!strncmp(value,"PDA",3)) {
		fsa9480_set_switch("AUTO");
		usbsw->switch_sel &= ~USB_SEL_MASK;
		usbsw->switch_sel |= USB_SEL_PDA_MASK;
		curr_usb_path = SWITCH_PDA;		
	}
	else if (!strncmp(value, "MODEM",5)) {
		fsa9480_set_switch("AUTO");
		usbsw->switch_sel &= ~USB_SEL_MASK;
		usbsw->switch_sel |= USB_SEL_MODEM_MASK;
		curr_usb_path = SWITCH_MODEM;		
	}
	else if (!strncmp(value,"ESC",3)) {		
	#if defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
		fsa9480_set_switch("VAUDIO");
	#else
		fsa9480_set_switch("AUDIO");
	#endif
		usbsw->switch_sel &= ~USB_SEL_MASK;
		usbsw->switch_sel |= USB_SEL_ESC_MASK;
		curr_usb_path = SWITCH_ESC;		
	} else {
		fsa9480_set_switch("AUTO");
		usbsw->switch_sel &= ~USB_SEL_MASK;
		usbsw->switch_sel |= USB_SEL_PDA_MASK;
		curr_usb_path = SWITCH_PDA;		
	}

	printk("%s : usbsw->switch_sel = 0x%x \n", __func__,usbsw->switch_sel);
	sec_set_param(param_index_uartsel, &usbsw->switch_sel);

	return size;

}
#endif

static ssize_t fsa9480_show_adc(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct fsa9480_info *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	u8 adc;

	fsa9480_read_reg(client, FSA_REG_ADC, &adc);
	
	if (adc < 0) {
		dev_err(&client->dev,
			"%s: err at read adc %d\n", __func__, adc);
		return snprintf(buf, 9, "UNKNOWN\n");
	}
	printk("%d\n", adc);
	return snprintf(buf, 4, "%x\n", adc);
}

static DEVICE_ATTR(status, S_IRUGO, fsa9480_show_status, NULL);
static DEVICE_ATTR(switch, 0644,
		fsa9480_show_manualsw, fsa9480_set_manualsw);
static DEVICE_ATTR(usb_state, S_IRUGO, fsa_show_usb_state, NULL);
static DEVICE_ATTR(adc, S_IRUGO, fsa9480_show_adc, NULL);
#ifdef CONFIG_SEC_DUAL_MODEM
static DEVICE_ATTR(uart_sel, S_IRUGO | S_IWUSR | S_IWGRP, uart_sel_show, uart_sel_store);
static DEVICE_ATTR(usb_sel, S_IRUGO | S_IWUSR | S_IWGRP, usb_sel_show, usb_sel_store);
static DEVICE_ATTR(is_factory_mode, S_IRUGO | S_IWUSR | S_IWGRP, NULL, is_factory_mode_store);
static DEVICE_ATTR(uart_sel_nsave, S_IRUGO | S_IWUSR | S_IWGRP, NULL, uart_sel_nsave_store);
#endif

static struct attribute *fsa9480_attributes[] = {
	&dev_attr_status.attr,
	&dev_attr_switch.attr,
	NULL
};

static const struct attribute_group fsa9480_group = {
	.attrs = fsa9480_attributes,
};

static irqreturn_t fsa9480_irq_handler(int irq, void *data)
{
	struct fsa9480_info *usbsw = data;
	if (!work_pending(&usbsw->work)) {
		disable_irq_nosync(irq);
		schedule_work(&usbsw->work);
	}
	return IRQ_HANDLED;
}

#if defined(CONFIG_TOUCHSCREEN_MELFAS_KYLE) 
// defined(CONFIG_MACH_ARUBA_DUOS_CTC) || defined(CONFIG_MACH_ARUBA_OPEN) || defined(CONFIG_MACH_ARUBASLIM_OPEN) || defined(CONFIG_MACH_KYLEPLUS_CTC) || defined(CONFIG_MACH_KYLEPLUS_OPEN) || defined(CONFIG_MACH_INFINITE_DUOS_CTC) || defined(CONFIG_MACH_DELOS_DUOS_CTC)
extern void touch_ta_noti(int en);
#endif

static void fsa9480_detect_dev(struct fsa9480_info *usbsw, u8 intr)
{
	u8 val1, val2, ctrl;
	u8 vbus;
	u8 test_adc;
	struct fsa9480_platform_data *pdata = usbsw->pdata;
	struct i2c_client *client = usbsw->client;

	fsa9480_read_reg(client, FSA_REG_DEV_T1, &val1);
	fsa9480_read_reg(client, FSA_REG_DEV_T2, &val2);
	fsa9480_read_reg(client, FSA_REG_CTRL, &ctrl);
	fsa9480_read_reg(client, FSA_REG_ADC, &test_adc);
	dev_info(&client->dev, "intr: 0x%x, dev1: 0x%x, dev2: 0x%x, adc: 0x%x\n",
			intr, val1, val2, test_adc);
#if 1 // KBJ  
	printk("%s : REG_CTRL = %x , usbsw->mansw : %x\n", __func__,ctrl, usbsw->mansw);
#endif

	if (usbsw->id == FSA880_ID) {
		val1 &= FSA880_REG_DEV_T1_MASK;
		val2 &= FSA880_REG_DEV_T2_MASK;
		ctrl &= FSA880_REG_DEV_CTRL_MASK;
	}
#if defined(CONFIG_MACH_JENA)
/* WORKAROUND for not recognizing ta detaching */
	/* interrupt reg(0x3h) == 0*/
	if  (!intr) {
		/* device type 1&2reg (0x0Ah, 0x0Bh) == 0*/
		if (val1 == 0 || val2 == 0) {
			if (usbsw->dev1 & FSA_DEV_T1_USB_MASK ||
					usbsw->dev2 & FSA_DEV_T2_USB_MASK) {
				if (pdata->usb_cb) {
					/*detach event of USB*/
					pdata->usb_cb(FSA_DETACHED, &ops);
						dev_info(&client->dev, "USB detached[WORKAROUND]\n");
					}
				}
			if (usbsw->dev1 & FSA_DEV_T1_CHARGER_MASK) {
				if (pdata->charger_cb) {
					/*detach event of CHARGER*/
					pdata->charger_cb(FSA_DETACHED, &ops);
					dev_info(&client->dev, "charger detached[WORKAROUND]\n");
					}
			}
			/* reset USB switch */
			fsa9480_write_reg(client, FSA_REG_CTRL, FSA_RESET_VALUE);
		}
}
#endif
#if defined(CONFIG_MACH_ARUBA_DUOS_CTC)
/* WORKAROUND which avoid to receive sudden detach irq */
	/* interrupt reg(0x3h) == 0*///detach
	if (!intr) {
		if (val1 == 0 || val2 == 0) {
			if(charger_cb_flag == 0) {//already detach status
				dev_info(&client->dev, "ignore detach irq[WORKAROUND]\n");
				return;
			}
		}
	}
#endif

	
	/* Attached */
	if (intr & (1 << 0)) {
		if (val1 & FSA_DEV_T1_USB_MASK ||
				val2 & FSA_DEV_T2_USB_MASK) {
			if (pdata->usb_cb) {
				dev_info(&client->dev, "usb attached\n");
				pdata->usb_cb(FSA_ATTACHED, &ops);
				usb_state_flag = 1;
			}
#if defined(CONFIG_MACH_HENNESSY_DUOS_CTC)			
			//If usb attached, we must check whether CDMA USB or GSM USB, and then switch the channel
			usbsw->mansw = (usbsw->switch_sel & USB_SEL_ESC_MASK) ? VAUDIO : AUTO;
			fsa9480_write_reg(client, FSA_REG_MANSW1,usbsw->mansw);
#else			
			if (usbsw->mansw)
				fsa9480_write_reg(client, FSA_REG_MANSW1,usbsw->mansw);
#if  defined(CONFIG_USB_EXTERNAL_CHARGER)
			else
			{
				set_pm_gpio_for_usb_enable(1);
			}
#endif
#endif		
			charger_cb_flag = 1;
		}
		if (val1 & FSA9480_DEV_T1_UART_MASK ||
				val2 & FSA_DEV_T2_UART_MASK) {
			if (pdata->uart_cb)
				pdata->uart_cb(FSA_ATTACHED, &ops);
#if defined(CONFIG_MACH_HENNESSY_DUOS_CTC)			
			//If uart attached, we must check whether CDMA UART or GSM UART, and then switch the channel
			usbsw->mansw = AUTO;
			fsa9480_write_reg(client,FSA_REG_MANSW1, usbsw->mansw);	
#else
			if (usbsw->mansw) {
				fsa9480_write_reg(client,FSA_REG_MANSW1, usbsw->mansw);				
			}
#endif
		}
		if (val1 & FSA_DEV_T1_CHARGER_MASK) {
#if defined(CONFIG_MACH_ARUBA_CTC) || defined(CONFIG_MACH_ARUBA_OPEN)
			mdelay(250);
			fsa9480_read_reg(client, FSA9480_REG_VBUS, &vbus);
			dev_info(&client->dev, "vbus: 0x%x\n", vbus);
			if (vbus & 0x2) {
			if (pdata->charger_cb)
				pdata->charger_cb(FSA_ATTACHED, &ops);
			charger_cb_flag = 1;
		}
#else
			if (pdata->charger_cb) {
				dev_info(&client->dev, "charger attached\n");
				pdata->charger_cb(FSA_ATTACHED, &ops);
			}
			charger_cb_flag = 1;
#endif
		}
		if (val2 & FSA_DEV_T2_JIG_MASK) {
			uart_connecting = 1;			
			if (pdata->jig_cb)
				pdata->jig_cb(FSA_ATTACHED, &ops);
		}

#if defined(CONFIG_SEC_FSA_JACK)
		if (val1 & FSA_DEV_T1_AUDIO1_MASK) {
			printk("jack_status_change()!");
			jack_status_change(1);			
		}
#endif


		
#if defined(DESK_DOCK_CHARGING_ENABLE_880)
		if (val2 & DEV_UNKNOWN) {
			printk("%s unknown cable is attached\n",__func__);
			fsa9480_read_reg(client, FSA9480_REG_VBUS, &vbus);
			dev_info(&client->dev, "vbus: 0x%x\n", vbus);
			if (vbus & 0x2) {
			if (pdata->charger_cb)
				pdata->charger_cb(FSA_ATTACHED, &ops);
			charger_cb_flag = 1;
			deskdock_attached = 1;
			}
		}
#endif				
		/*Desk dock*/
#ifdef CONFIG_MACH_AMAZING_CDMA
		if (val2 & DEV_AV) {
			if (pdata->charger_cb)
				pdata->charger_cb(FSA_ATTACHED, &ops);
			charger_cb_flag = 1;
		}
#endif
		if (val1 == 0 && val2 == 0) {
			if (usbsw->dev1 & FSA_DEV_T1_USB_MASK ||
					usbsw->dev2 & FSA_DEV_T2_USB_MASK) {
				if (pdata->usb_cb) {
					/*detach event of USB*/
					pdata->usb_cb(FSA_DETACHED, &ops);
						dev_info(&client->dev, "USB detached[WORKAROUND]\n");
					}
				}
			if (usbsw->dev1 & FSA_DEV_T1_CHARGER_MASK) {
				if (pdata->charger_cb) {
					/*detach event of CHARGER*/
					pdata->charger_cb(FSA_DETACHED, &ops);
					dev_info(&client->dev, "charger detached[WORKAROUND]\n");
					}
			}
			/* reset USB switch */
			fsa9480_write_reg(client, FSA_REG_CTRL, FSA_RESET_VALUE);
		} else if (intr & (1 << 5)) {
			if (pdata->ovp_cb)
				pdata->ovp_cb(FSA_ATTACHED, &ops);
			pr_info("[BATT] OVP occur connect charger!%d\n",
					ovp_cb_flag);
			ovp_cb_flag = 1;
		}
#if defined(DESK_DOCK_CHARGING_ENABLE_9280)
		if (intr2_av & (1 << 0)) {
			if (val2 & (1 << 6)) {
				if (pdata->charger_cb)
					pdata->charger_cb(FSA_ATTACHED, &ops);
				charger_cb_flag = 1;
			}
		}
#endif		

#if defined(CONFIG_TOUCHSCREEN_MELFAS_KYLE)
	// defined(CONFIG_MACH_ARUBA_DUOS_CTC) || defined(CONFIG_MACH_ARUBA_OPEN) || defined(CONFIG_MACH_ARUBASLIM_OPEN) || defined(CONFIG_MACH_KYLEPLUS_CTC) || defined(CONFIG_MACH_KYLEPLUS_OPEN) || defined(CONFIG_MACH_INFINITE_DUOS_CTC) || defined(CONFIG_MACH_DELOS_DUOS_CTC)
			if(charger_cb_flag ==1){ //val1 & (DEV_USB_CHG|DEV_DEDICATED_CHG)){
				touch_ta_noti(1);
			}
#endif

	} else if (intr & (1 << 1)) {
		if (usbsw->dev1 & FSA_DEV_T1_USB_MASK ||
				usbsw->dev2 & FSA_DEV_T2_USB_MASK) {
			if (pdata->usb_cb) {
				pdata->usb_cb(FSA_DETACHED, &ops);
				usb_state_flag = 0;
#if defined(CONFIG_USB_DCHUN_MAC)
				set_mac_connected(0);
#endif
				}
			charger_cb_flag = 0;
#if defined(CONFIG_USB_EXTERNAL_CHARGER)
			set_pm_gpio_for_usb_enable(0);
#endif	
		}
		if (usbsw->dev1 & FSA9480_DEV_T1_UART_MASK ||
				usbsw->dev2 & FSA_DEV_T2_UART_MASK) {
			if (pdata->uart_cb)
				pdata->uart_cb(FSA_DETACHED, &ops);
		}
		if (usbsw->dev1 & FSA_DEV_T1_CHARGER_MASK) {
			if (pdata->charger_cb)
				pdata->charger_cb(FSA_DETACHED, &ops);
			charger_cb_flag = 0;
		}
#if defined(DESK_DOCK_CHARGING_ENABLE_9280)
		if (usbsw->dev2 & (1 << 6)) {
			if (pdata->charger_cb)
				pdata->charger_cb(FSA_DETACHED, &ops);
			charger_cb_flag = 0;
			fsa9480_write_reg(client, FSA_REG_CTRL, FSA_RESET_VALUE);
		}
#endif	
#if defined(DESK_DOCK_CHARGING_ENABLE_880)
		if ((val2 == 0x00) && deskdock_attached) {
			printk("%s unknown cable is dettached\n",__func__);
			if (pdata->charger_cb)
				pdata->charger_cb(FSA_DETACHED, &ops);
			charger_cb_flag = 0;
			deskdock_attached = 0;
			fsa9480_write_reg(client, FSA_REG_CTRL, FSA_RESET_VALUE);
		}
#endif				
		if (usbsw->dev2 & FSA_DEV_T2_JIG_MASK) {
			uart_connecting = 0;		
			if (pdata->jig_cb)
				pdata->jig_cb(FSA_DETACHED, &ops);
		}
#ifdef CONFIG_MACH_AMAZING_CDMA
		if (usbsw->dev2 & DEV_AV) {
			if (pdata->charger_cb)
				pdata->charger_cb(FSA_DETACHED, &ops);
			charger_cb_flag = 0;
		}
#endif
#ifdef CONFIG_SEC_DUAL_MODEM
		if(curr_usb_path != SWITCH_ESC) {
			printk("[FSA9480]: %s :: curr_usb_path : %d\n",__func__, curr_usb_path);	
#endif
		if (usbsw->dev1 == 0x00 && usbsw->dev2 == 0x00) {
			if (pdata->charger_cb)
				pdata->charger_cb(FSA_DETACHED, &ops);
				charger_cb_flag = 0;
			fsa9480_write_reg(client, FSA_REG_CTRL, FSA_RESET_VALUE);
		}
#ifdef CONFIG_SEC_DUAL_MODEM		
		}
#endif
		if ((intr & (1 << 7)) && ovp_cb_flag) {
			pr_info("[BATT] OVP is not!%d\n", ovp_cb_flag);
			if (pdata->ovp_cb)
				pdata->ovp_cb(FSA_DETACHED, &ops);
			ovp_cb_flag = 0;
		}
		//workaround, for some speciall time
		if (ovp_cb_flag) {
			pr_info("[BATT] OVP[workaround] is not!%d\n", ovp_cb_flag);
			if (pdata->ovp_cb)
				pdata->ovp_cb(FSA_DETACHED, &ops);
			ovp_cb_flag = 0;
		}
#if  defined(CONFIG_TOUCHSCREEN_MELFAS_KYLE)  
// defined(CONFIG_MACH_ARUBA_DUOS_CTC) || defined(CONFIG_MACH_ARUBA_OPEN) || defined(CONFIG_MACH_ARUBASLIM_OPEN) || defined(CONFIG_MACH_KYLEPLUS_CTC) || defined(CONFIG_MACH_KYLEPLUS_OPEN) || defined(CONFIG_MACH_INFINITE_DUOS_CTC) || defined(CONFIG_MACH_DELOS_DUOS_CTC)
		if(charger_cb_flag==0){ // (usbsw->dev1 & (DEV_USB_CHG|DEV_DEDICATED_CHG)) && (!(val1 & (DEV_USB_CHG|DEV_DEDICATED_CHG)))){
			touch_ta_noti(0);
		}
#endif		

#if defined(CONFIG_SEC_FSA_JACK)
		if (usbsw->dev1 & FSA_DEV_T1_AUDIO1_MASK) {
			printk("Deattach!! jack_status_change()!");
			jack_status_change(0);			
		}
#endif

	} else if (intr == 0) {
		if (val1 == 0 && val2 == 0) {
			if (usbsw->dev1 & FSA_DEV_T1_USB_MASK ||
					usbsw->dev2 & FSA_DEV_T2_USB_MASK) {
				if (pdata->usb_cb) {
					/*detach event of USB*/
					pdata->usb_cb(FSA_DETACHED, &ops);
						dev_info(&client->dev, "USB detached[WORKAROUND]\n");
					}
				}
			if (usbsw->dev1 & FSA_DEV_T1_CHARGER_MASK) {
				if (pdata->charger_cb) {
					/*detach event of CHARGER*/
					pdata->charger_cb(FSA_DETACHED, &ops);
					dev_info(&client->dev, "charger detached[WORKAROUND]\n");
					}
			}
			/* reset USB switch */
			fsa9480_write_reg(client, FSA_REG_CTRL, FSA_RESET_VALUE);
		}
	} else if (intr & (1 << 5)) {
		if (pdata->ovp_cb)
			pdata->ovp_cb(FSA_ATTACHED, &ops);
		pr_info("[BATT] OVP is occur!%d\n", ovp_cb_flag);
		ovp_cb_flag = 1;
	} else if ((intr & (1 << 7)) && ovp_cb_flag) {
		pr_info("[BATT] OVP is not!%d\n", ovp_cb_flag);
		if (pdata->ovp_cb)
			pdata->ovp_cb(FSA_DETACHED, &ops);
		ovp_cb_flag = 0;
	}

#if defined(CONFIG_SEC_FSA_JACK)
	else if((intr&KEY_PRESS)||(intr&LONG_KEY_PRESS)) {
		pr_info("KEY pressed!!%d\n", intr);
		earkey_status_change(1, KEY_MEDIA);

		earkey_status_change(0, KEY_MEDIA);
	}
#endif
	usbsw->dev1 = val1;
	usbsw->dev2 = val2;
	chip->dev1 = val1;
	chip->dev2 = val2;
	ctrl &= ~INT_MASK;
#if 0 // KBJ  temp block
	fsa9480_write_reg(client, FSA_REG_CTRL, ctrl);
#endif
}

static void fsa9480_delay_mask(struct work_struct *work)
{
	struct fsa9480_info *usbsw = chip;
	struct i2c_client *client = usbsw->client;

	dev_info(&client->dev, "%s: unmasking attach/detach irq[delay 700ms]\n",__func__);
	fsa9480_write_reg(client, FSA9480_REG_INT1_MASK, 0x00);
}

static void fsa9480_work_cb(struct work_struct *work)
{
	struct fsa9480_info *usbsw =
		container_of(work, struct fsa9480_info, work);
	struct i2c_client *client = usbsw->client;

	struct fsa9480_platform_data *pdata = usbsw->pdata;
	struct delayed_work *dw = container_of(work, struct delayed_work, work);		
	struct fsa9480_delay_wq *wq = container_of(dw, struct fsa9480_delay_wq, work_q);
	u8 intr, intr2;

	/* clear interrupt */
/*	if (usbsw->id != FSA880_ID)
		fsa9480_read_reg(client, FSA9480_REG_INT2, &intr);
*/
	fsa9480_read_reg(client, FSA_REG_INT1, &intr);
	fsa9480_read_reg(client, FSA_REG_INT2, &intr2);

	dev_info(&client->dev, "%s: intr: 0x%x intr2: 0x%x\n",
					__func__, intr, intr2);

	//workaround --- when connecting some special TA, to avoid to receive attach/detach irq repeatedly
	if(intr & 0x01 || intr & 0x02) {
		fsa9480_write_reg(client, FSA9480_REG_INT1_MASK, 0x03);//masking to don't recognize the attach/detach irq

		/* run work queue */
		wq = kmalloc(sizeof(struct fsa9480_delay_wq), GFP_ATOMIC);
		if (wq) {
			dev_info(&client->dev, "%s: start unmasking attach/detach irq\n",__func__);			
			INIT_DELAYED_WORK(&wq->work_q, fsa9480_delay_mask);
			schedule_delayed_work(&wq->work_q, msecs_to_jiffies(750));
		} 
	}	
	intr &= 0xff;
#if defined(DESK_DOCK_CHARGING_ENABLE_9280)
	intr2_av = intr2 & 0x1f;
#endif	
	/* device detection */
	fsa9480_detect_dev(usbsw, intr);
	enable_irq(client->irq);
}

static int fsa9480_irq_init(struct fsa9480_info *usbsw)
{
	struct i2c_client *client = usbsw->client;
	u8 ctrl = 0;
	int ret, irq = -1;
	u8 intr;
	u8 mansw1;

	if (usbsw->id != FSA880_ID)	
		ctrl = CTRL_MASK;	
	else
		ctrl =  FSA880_REG_DEV_CTRL_MASK;

	/* clear interrupt */
	dev_info(&client->dev, "fsa9480 id : %d \n",usbsw->id );
	
	if (usbsw->id != FSA880_ID)
		fsa9480_read_reg(client, FSA9480_REG_INT2, &intr);
	fsa9480_read_reg(client, FSA_REG_INT1, &intr);

	if (usbsw->id != FSA880_ID) {
		/* unmask interrupt (attach/detach only) */
		ret = fsa9480_write_reg(client, FSA9480_REG_INT1_MASK, 0x00);
		if (ret < 0)
			return ret;

		ret = fsa9480_write_reg(client, FSA9480_REG_INT2_MASK, 0x00);
		if (ret < 0)
			return ret;
	}

	fsa9480_read_reg(client, FSA_REG_MANSW1, &mansw1);
	usbsw->mansw = mansw1;
	ctrl &= ~INT_MASK;              /* Unmask Interrupt */

	/* XXX: should check android spec */
	if (usbsw->mansw)
		ctrl &= ~MANUAL_SWITCH; /* Manual Switching Mode */
	fsa9480_write_reg(client, FSA_REG_CTRL, ctrl);
	dev_info(&client->dev, "[%s] set ctrl=0x%02x\n", __func__, ctrl);

	INIT_WORK(&usbsw->work, fsa9480_work_cb);

	switch (usbsw->id) {
	case FSA9480_ID:
		ret = request_irq(client->irq, fsa9480_irq_handler,
				IRQF_TRIGGER_LOW | IRQF_DISABLED,
				"fsa9480 micro USB", usbsw);
		break;
	case FSA9280_ID:
		ret = request_irq(client->irq, fsa9480_irq_handler,
				IRQF_TRIGGER_LOW | IRQF_DISABLED,
				"fsa9280 micro USB", usbsw);
		break;
	case FSA880_ID:
		ret = request_irq(client->irq, fsa9480_irq_handler,
				IRQF_TRIGGER_LOW | IRQF_DISABLED,
				"fsa880 micro USB", usbsw);
		break;
	}

	if (ret) {
		dev_err(&client->dev,
				"%s micro USB: Unable to get IRQ %d\n",
				usbsw->name, irq);
		goto out;
	}

	return 0;

out:
	return ret;
}

#ifdef CONFIG_SEC_DUAL_MODEM
static void sec_switch_init_work(struct work_struct *work)
{
#if !defined(CONFIG_MACH_ROY)
	struct delayed_work *dw = container_of(work, struct delayed_work, work);
	struct fsa9480_wq *wq = container_of(dw, struct fsa9480_wq, work_q);
	struct fsa9480_info *usbsw = wq->sdata;
	int usb_sel, uart_sel;

	printk("[FSA9480]: %s :: \n",__func__);	

	if (!sec_get_param(param_index_uartsel,&usbsw->switch_sel)) { 
		schedule_delayed_work(&wq->work_q, msecs_to_jiffies(1000));
		printk("[FSA9480]: %s :: schedule_delayed_work\n",__func__); 		
		return;
	} else {
		cancel_delayed_work(&wq->work_q);
		sec_get_param(param_index_uartsel,&usbsw->switch_sel);		
		printk("[FSA9480]: %s :: cancel delayed work \n",__func__);
	}	

	printk(" usbsw->switch_sel : %x\n",  usbsw->switch_sel);

#ifdef CONFIG_MACH_HENNESSY_DUOS_CTC
	if(usbsw->switch_sel == 0
		|| (usbsw->switch_sel & USB_SWITCH_TRASH_BIT) || (usbsw->switch_sel & UART_SWITCH_TRASH_BIT)) {//first booting
		usb_sel = USB_SEL_PDA_MASK;
		uart_sel = UART_SEL_MODEM_MASK;			
		usbsw->switch_sel = uart_sel | usb_sel;
		sec_set_param(param_index_uartsel, &usbsw->switch_sel);
		printk("[FSA9480]:First booting, set usbsw->switch_sel = 0x%x\n",  usbsw->switch_sel);
	} 
#else	
	if(usbsw->switch_sel & USB_SWITCH_TRASH_BIT || usbsw->switch_sel & UART_SWITCH_TRASH_BIT) {//first booting
		uart_sel = USB_SEL_PDA_MASK;
		usb_sel = UART_SEL_MODEM_MASK;			
		usbsw->switch_sel = (uart_sel<<4) || (usb_sel);
		sec_set_param(param_index_uartsel, &usbsw->switch_sel);
	}
#endif
	else {
		uart_sel = usbsw->switch_sel & UART_SEL_MASK;
		usb_sel = usbsw->switch_sel & USB_SEL_MASK;
	}

	printk("after cal : usbsw->switch_sel : %x\n",  usbsw->switch_sel);

	if (usb_sel & USB_SEL_PDA_MASK) {
		fsa9480_set_switch("AUTO");
		curr_usb_path = SWITCH_PDA;		
	} else if (usb_sel & USB_SEL_MODEM_MASK) {
		fsa9480_set_switch("AUTO");
		curr_usb_path = SWITCH_MODEM;	
	} else if (usb_sel & USB_SEL_ESC_MASK) {
	#if defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
		fsa9480_set_switch("VAUDIO");
	#else
		fsa9480_set_switch("AUDIO");
	#endif
		curr_usb_path = SWITCH_ESC; 	
	}

#ifdef CONFIG_MACH_HENNESSY_DUOS_CTC
	gpio_tlmm_config(GPIO_CFG(GPIO_UART_SEL, 0, GPIO_CFG_OUTPUT,
			GPIO_CFG_PULL_UP, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
#endif

	if(curr_usb_path !=SWITCH_ESC) {
		if (uart_sel & UART_SEL_PDA_MASK) {
			curr_uart_path = SWITCH_PDA;
			fsa9480_set_switch("AUTO");	
		} else if (uart_sel & UART_SEL_MODEM_MASK) {
			curr_uart_path = SWITCH_MODEM;
			fsa9480_set_switch("AUTO");	
		} else if (uart_sel & UART_SEL_ESC_MASK) {
			curr_uart_path = SWITCH_ESC;
		#ifdef CONFIG_MACH_HENNESSY_DUOS_CTC
			fsa9480_set_switch("AUTO");
		#else
			fsa9480_set_switch("VAUDIO");
		#endif
		}

	#if defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
		if(curr_uart_path == SWITCH_ESC)
		{
			fsa9485_uart_sel_switch_en(1); // 1 - for GSM
		}
		else
		{
			fsa9485_uart_sel_switch_en(0); // 0 - for CDMA
		}
	#endif
	}
	return;
#endif
}
#endif

extern struct class *sec_class;
static int __devinit fsa9480_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct fsa9480_info *usbsw;
	int ret = 0;
	struct device *switch_dev;
#ifdef CONFIG_SEC_DUAL_MODEM	
	struct fsa9480_wq *wq;
#endif
	
	dev_info(&client->dev, "[FSA9480 micro usb] PROBE ...\n");

	usbsw = kzalloc(sizeof(struct fsa9480_info), GFP_KERNEL);
	if (!usbsw) {
		dev_err(&client->dev, "failed to allocate driver data\n");
		return -ENOMEM;
	}
	usbsw->client = client;
	usbsw->pdata = client->dev.platform_data;

	chip = usbsw;
//	ops.detach_handler = fsa_default_detach_handler;
//	ops.attach_handler = fsa_default_attach_handler;
	charger_cb_flag = 0;

	if (id->driver_data > FSA880_ID) {
		dev_err(&client->dev, "mismatch device error\n");
		goto fsa9480_probe_fail;
	}
	usbsw->name = id->name;
	usbsw->id = id->driver_data;

	if( usbsw->id == FSA9480_ID )
	{
		FSA_DEV_T1_USB_MASK = FSA9480_DEV_T1_USB_MASK;
		FSA_DEV_T1_CHARGER_MASK = FSA9480_DEV_T1_CHARGER_MASK;
		FSA_DEV_T2_USB_MASK = FSA9480_DEV_T2_USB_MASK;
		FSA_DEV_T2_UART_MASK = FSA9480_DEV_T2_UART_MASK;
		FSA_DEV_T2_JIG_MASK = FSA9480_DEV_T2_JIG_MASK;
		FSA_DEV_T1_AUDIO1_MASK = FSA9480_DEV_T1_AUDIO1_MASK;
		FSA_RESET_VALUE = FSA9480_REG_CTRL_RESET_VALUE;
	}
	else if( usbsw->id == FSA880_ID )
	{
		FSA_DEV_T1_USB_MASK = FSA880_DEV_T1_USB_MASK;
		FSA_DEV_T1_CHARGER_MASK = FSA880_DEV_T1_CHARGER_MASK;
		FSA_DEV_T2_USB_MASK = FSA880_DEV_T2_USB_MASK;
		FSA_DEV_T2_UART_MASK = FSA880_DEV_T2_UART_MASK;
		FSA_DEV_T2_JIG_MASK = FSA880_DEV_T2_JIG_MASK;
		FSA_RESET_VALUE = FSA880_REG_CTRL_RESET_VALUE;
	}
	else
	{
		FSA_DEV_T1_USB_MASK = FSA9480_DEV_T1_USB_MASK;
		FSA_DEV_T1_CHARGER_MASK = FSA9480_DEV_T1_CHARGER_MASK;
		FSA_DEV_T2_USB_MASK = FSA9480_DEV_T2_USB_MASK;
		FSA_DEV_T2_UART_MASK = FSA9480_DEV_T2_UART_MASK;
		FSA_DEV_T2_JIG_MASK = FSA9480_DEV_T2_JIG_MASK;
		FSA_RESET_VALUE = FSA9480_REG_CTRL_RESET_VALUE;
	}


	i2c_set_clientdata(client, usbsw);

	ret = fsa9480_irq_init(usbsw);
	if (ret)
		goto fsa9480_probe_fail;

#if defined(DESK_DOCK_CHARGING_ENABLE_9280) || defined(DESK_DOCK_CHARGING_ENABLE_880)
	enable_irq_wake(client->irq);
#endif

	/* make sysfs node /sys/class/sec/switch/usb_state */
	switch_dev = device_create(sec_class, NULL, 0, NULL, "switch");
	if (IS_ERR(switch_dev)) {
		dev_err(&client->dev,
			"Failed to create device(switch_dev)!\n");
		return PTR_ERR(switch_dev);
	}

	ret = device_create_file(switch_dev, &dev_attr_usb_state);
	if (ret < 0) {
		dev_err(&client->dev,
			"Failed to create device (usb_state)!\n");
		goto err_create_file_state;
	}

#ifdef CONFIG_SEC_DUAL_MODEM
	ret = device_create_file(switch_dev, &dev_attr_uart_sel);
	if (ret < 0) {
		dev_err(&client->dev,
			"Failed to create device (uart_sel)!\n");
		goto err_create_file_state;
	}

	ret = device_create_file(switch_dev, &dev_attr_usb_sel);
	if (ret < 0) {
		dev_err(&client->dev,
			"Failed to create device (usb_sel)!\n");
		goto err_create_file_state;
	}

	ret = device_create_file(switch_dev, &dev_attr_is_factory_mode);
	if (ret < 0) {
		dev_err(&client->dev,
			"Failed to create device (is_factory_mode)!\n");
		goto err_create_file_state;
	}	

	ret = device_create_file(switch_dev, &dev_attr_uart_sel_nsave);
	if (ret < 0) {
		dev_err(&client->dev,
			"Failed to create device (uart_sel_nsave)!\n");
		goto err_create_file_state;
	}	
#endif

	ret = device_create_file(switch_dev, &dev_attr_adc);
	if (ret < 0) {
		pr_err("[FSA9480] Failed to create device (adc)!\n");
		goto err_create_file_adc;
	}
	ret = sysfs_create_group(&client->dev.kobj, &fsa9480_group);
	if (ret) {
		dev_err(&client->dev,
				"Creating fsa attribute group failed");
		goto fsa9480_probe_fail2;
	}
	dev_info(&client->dev, "[%s] %d\n", __func__, __LINE__);

	dev_set_drvdata(switch_dev, usbsw);

	if (usbsw->id != FSA880_ID)
		fsa9480_write_reg(client, FSA9480_REG_TIMING1, 0x6);

	if (chip->pdata->reset_cb)
		chip->pdata->reset_cb();

	/* device detection */
	fsa9480_detect_dev(usbsw, 1);

	dev_info(&client->dev, "[%s] PROBE Done.\n", usbsw->name);

#ifdef CONFIG_SEC_DUAL_MODEM
	/* run work queue */
	wq = kmalloc(sizeof(struct fsa9480_wq), GFP_ATOMIC);
	if (wq) {
		wq->sdata = usbsw;
		INIT_DELAYED_WORK(&wq->work_q, sec_switch_init_work);
#if	defined(CONFIG_MACH_DELOS_DUOS_CTC)	 || defined(CONFIG_MACH_HENNESSY_DUOS_CTC)	
		schedule_delayed_work(&wq->work_q, msecs_to_jiffies(5500));
#else
		schedule_delayed_work(&wq->work_q, msecs_to_jiffies(6000));
#endif
	} else
		return -ENOMEM;
#endif

	return 0;
err_create_file_state:
	device_remove_file(switch_dev, &dev_attr_usb_state);
#ifdef CONFIG_SEC_DUAL_MODEM
	device_remove_file(switch_dev, &dev_attr_uart_sel);
	device_remove_file(switch_dev, &dev_attr_usb_sel);
#endif

err_create_file_adc:
	device_remove_file(switch_dev, &dev_attr_adc);
	
fsa9480_probe_fail2:
	if (client->irq)
		free_irq(client->irq, NULL);

fsa9480_probe_fail:
	i2c_set_clientdata(client, NULL);
	kfree(usbsw);
	return ret;
}

static int __devexit fsa9480_remove(struct i2c_client *client)
{
	struct fsa9480_info *usbsw = i2c_get_clientdata(client);

	if (client->irq)
		free_irq(client->irq, NULL);

	i2c_set_clientdata(client, NULL);
	sysfs_remove_group(&client->dev.kobj, &fsa9480_group);
	kfree(usbsw);

	return 0;
}
#ifdef CONFIG_SEC_DUAL_MODEM
extern void fsa9480_i2c_init(void);
static void fsa9480_i2c_gpio_init(struct i2c_client *client)
{
	u8 ctrl;

	dev_info(&client->dev, "fsa9480_i2c_gpio_init\n");

	fsa9480_i2c_init();

	fsa9480_read_reg(client, FSA_REG_CTRL, &ctrl);
	dev_info(&client->dev, "ctrl: 0x%x\n", ctrl);

	fsa9480_write_reg(client, FSA_REG_CTRL, ctrl);
}
#endif
#ifdef CONFIG_PM
static int fsa9480_resume(struct i2c_client *client)
{
	struct fsa9480_info *usbsw = i2c_get_clientdata(client);
	u8 intr;
	u8 val1, val2;
#ifdef CONFIG_SEC_DUAL_MODEM
	fsa9480_i2c_gpio_init(client);
#endif
	/* for hibernation */
	fsa9480_read_reg(client, FSA_REG_DEV_T1, &val1);
	fsa9480_read_reg(client, FSA_REG_DEV_T2, &val2);

	if (val1 || val2)
	{
		intr = 1 << 0;
#if defined(DESK_DOCK_CHARGING_ENABLE_9280)
		if(val2)
			intr2_av = 1 << 0;
#endif		
	}	
	else
		intr = 1 << 1;

	/* device detection */
	fsa9480_detect_dev(usbsw, intr);
	return 0;
}
#else
#define fsa9480_resume         NULL
#endif

static const struct i2c_device_id fsa9480_id[] = {
	{"FSA9480", FSA9480_ID},
	{"FSA9280", FSA9280_ID},
	{"FSA880", FSA880_ID},
	{}
};
MODULE_DEVICE_TABLE(i2c, fsa9480_id);

static struct i2c_driver fsa9480_i2c_driver = {
	.driver = {
		.name = "fsa_microusb",
	},
	.probe = fsa9480_probe,
	.remove = __devexit_p(fsa9480_remove),
	.resume = fsa9480_resume,
	.id_table = fsa9480_id,
};

static int __init fsa9480_init(void)
{
	return i2c_add_driver(&fsa9480_i2c_driver);
}

static void __exit fsa9480_exit(void)
{
	i2c_del_driver(&fsa9480_i2c_driver);
}

module_init(fsa9480_init);
module_exit(fsa9480_exit);

MODULE_AUTHOR("handohee");
MODULE_DESCRIPTION("FSAxxxx USB Switch driver");
MODULE_LICENSE("GPL");
