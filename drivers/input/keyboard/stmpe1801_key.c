/******************** (C) COPYRIGHT 2010 STMicroelectronics ********************
*
* File Name		: stmpe1801_key.c
* Authors		: AMS Korea TC
*			    : Gyusung SHIM
* Version		: V 2.4
* Date			: 06/14/2012
* Description	: STMPE1801 driver for keypad function
*
********************************************************************************
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* THE PRESENT SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES
* OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, FOR THE SOLE
* PURPOSE TO SUPPORT YOUR APPLICATION DEVELOPMENT.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
* THIS SOFTWARE IS SPECIFICALLY DESIGNED FOR EXCLUSIVE USE WITH ST PARTS.
*
********************************************************************************
* REVISON HISTORY
*
* VERSION | DATE 	   | AUTHORS	     | DESCRIPTION
* 1.0	  | 04/04/2012 | Gyusung SHIM    | First release for Kernel 3.x
* 1.1	  | 04/06/2012 | Gyusung SHIM    | ISR enable
* 1.2	  | 04/11/2012 | Gyusung SHIM    | CONFIG_PM support
* 1.3	  | 04/12/2012 | Gyusung SHIM    | Refine enable/disable & bugfix
* 1.4	  | 04/12/2012 | Gyusung SHIM    | Adding sysfs
* 1.5	  | 04/13/2012 | Gyusung SHIM    | fix kzalloc : size of pdata
* 1.6	  | 04/18/2012 | Gyusung SHIM    | fix 'up' in stmpe1801_key_int_proc is always 0
* 1.7	  | 04/18/2012 | Gyusung SHIM    | fix __stmpe1801_get_keycode_entries reports wrong value
* 1.8	  | 04/18/2012 | Gyusung SHIM    | add #if switch on disable/enable irq in isr
* 1.9	  | 05/09/2012 | Gyusung SHIM    | Spliting pinmask to row & col, optimizing isr.
* 2.0	  | 05/16/2012 | Gyusung SHIM    | Add ghost key list.
* 2.1	  | 06/01/2012 | Gyusung SHIM    | Fix enable/disable return & remove lock in probe
* 2.2	  | 06/08/2012 | Gyusung SHIM    | Revise PM functions
* 2.3	  | 06/13/2012 | Gyusung SHIM    | Fix STMPE1801_REG_SYS_CTRL
* 2.4	  | 06/14/2012 | Gyusung SHIM    | Add STMPE1801_TRIGGER_TYPE_FALLING config/Add pull-up setting on unused
*
*******************************************************************************/
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/input.h>
#include <linux/input/matrix_keypad.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/errno.h>
#include <linux/sec_debug.h>


#include <linux/input/stmpe1801_key.h>

// keyboard led  yuechun.yao
#if defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
#include <linux/leds.h>
#endif
#define DD_VER	"2.4"

#define DBG_PRN	// define for debugging printk
#define STMPE1801_TRIGGER_TYPE_FALLING // define for INT trigger
#define STMPE1801_TMR_INTERVAL	10L


/*
 * Definitions & globals.
 
 */
#define STMPE1801_MAXGPIO				18
#define STMPE1801_KEY_FIFO_LENGTH		10

#define STMPE1801_REG_SYS_CHIPID		0x00
#define STMPE1801_REG_SYS_CTRL			0x02

#define STMPE1801_REG_INT_CTRL_LOW		0x04
#define STMPE1801_REG_INT_MASK_LOW		0x06
#define STMPE1801_REG_INT_STA_LOW		0x08

#define STMPE1801_REG_INT_GPIO_MASK_LOW	0x0A
#define STMPE1801_REG_INT_GPIO_STA_LOW	0x0D

#define STMPE1801_REG_GPIO_SET_yyy		0x10
#define STMPE1801_REG_GPIO_CLR_yyy		0x13
#define STMPE1801_REG_GPIO_MP_yyy		0x16
#define STMPE1801_REG_GPIO_SET_DIR_yyy	0x19
#define STMPE1801_REG_GPIO_RE_yyy		0x1C
#define STMPE1801_REG_GPIO_FE_yyy		0x1F
#define STMPE1801_REG_GPIO_PULL_UP_yyy	0x22

#define STMPE1801_REG_KPC_ROW			0x30
#define STMPE1801_REG_KPC_COL_LOW		0x31
#define STMPE1801_REG_KPC_CTRL_LOW		0x33
#define STMPE1801_REG_KPC_CTRL_MID		0x34
#define STMPE1801_REG_KPC_CTRL_HIGH		0x35
#define STMPE1801_REG_KPC_CMD			0x36
#define STMPE1801_REG_KPC_DATA_BYTE0	0x3A


#define STMPE1801_KPC_ROW_SHIFT			4

#define STMPE1801_KPC_DATA_UP			(0x80)
#define STMPE1801_KPC_DATA_COL			(0x78)
#define STMPE1801_KPC_DATA_ROW			(0x07)
#define STMPE1801_KPC_DATA_NOKEY		(0x0f)


union stmpe1801_kpc_data {
	uint64_t						value;
	uint32_t						val32[2];
	uint8_t 						array[8];
};


struct stmpe1801_dev {
	unsigned int					en_blocks;
	int 							dev_irq;
	struct i2c_client				*client;
	struct input_dev				*input_dev;
	struct delayed_work				worker;
	struct mutex					dev_lock;
	atomic_t						dev_enabled;
	atomic_t						dev_init;
	int								dev_resume_state;

	u8								regs_sys_int_init[4];
	u8								regs_kpc_init[8];
	u8								regs_pullup_init[3];

	struct stmpe1801_platform_data	*pdata;

	uint32_t						*ghost_keys;
	int								ghost_key_entries;

	unsigned short					*keycode;
	int								keycode_entries;
};

#if defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
#define KEY_LED_GPIO 7 //PMIC_GPIO_8 
extern int pmic_gpio_direction_output(unsigned gpio);
extern int pmic_gpio_set_value(unsigned gpio, int value);
#endif
static int __stmpe1801_get_keycode_entries(u32 mask_row, u32 mask_col)
{
	int col, row;
	int i, j, comp;

	row = mask_row & 0xff;
	col = mask_col & 0x3ff;

	for (i = 8; i > 0; i--) {
		comp = 1 << (i - 1);
		if (row & comp) break;
	}

	for (j = 10; j > 0; j--) {
		comp = 1 << (j - 1);
		if (col & comp) break;
	}
#ifdef DBG_PRN
	printk(KERN_WARNING STMPE1801_DRV_NAME " row: %d, col: %d\n", i, j);
#endif

	return (MATRIX_SCAN_CODE(i - 1, j - 1, STMPE1801_KPC_ROW_SHIFT)+1);
}

static void __stmpe1801_discarding_ghost(struct stmpe1801_dev *stmpe1801, union stmpe1801_kpc_data *kpc)
{
	int i, j, idx, val;
	uint8_t comp1, comp2, gho;

	if (!(stmpe1801->ghost_key_entries)) return;

	for (i = 0; i < stmpe1801->ghost_key_entries; i++) {
		comp1 = stmpe1801->ghost_keys[i] & 0x7f;
		comp2 = (stmpe1801->ghost_keys[i]>>8) & 0x7f;
		gho = (stmpe1801->ghost_keys[i]>>16) & 0x7f;
		idx = -1, val = 0;
		for (j = 0; j < 3; j++) {
			if (comp1 == (kpc->array[j] & 0x7f)) val |= 0x1;
			if (comp2 == (kpc->array[j] & 0x7f)) val |= 0x2;
			if (gho == (kpc->array[j] & 0x7f)) idx = j;
		}

		if (val) {
			if (idx >= 0) {
				kpc->array[idx] = 0xf8;
#ifdef DBG_PRN
				printk(KERN_WARNING STMPE1801_DRV_NAME " Ghost key!! : %x  (%x)\n", gho, kpc->val32[0]&0x7f7f7f);
#endif
			}
			break;
		}
	}

	return;
}



static int __i2c_block_read(struct i2c_client *client, u8 regaddr, u8 length, u8 *values)
{
	int ret;
	struct i2c_msg	msgs[] = {
		{
			.addr = client->addr,
			.flags = client->flags,
			.len = 1,
			.buf = &regaddr,
		},
		{
			.addr = client->addr,
			.flags = client->flags | I2C_M_RD,
			.len = length,
			.buf = values,
		},
	};

	ret = i2c_transfer(client->adapter, msgs, 2);

	if (ret != 2) {
		dev_err(&client->dev, "failed to read reg %#x: %d\n", regaddr, ret);
		ret = -EIO;
	}

	//dev_vdbg(&client->dev, "i2c rd: reg %#x => ret %#x\n", regaddr, ret);

	return ret;
}

static int __i2c_block_write(struct i2c_client *client, u8 regaddr, u8 length, const u8 *values)
{
	int ret;
	unsigned char msgbuf0[I2C_SMBUS_BLOCK_MAX+1];
	struct i2c_msg	msgs[] = {
		{
			.addr = client->addr,
			.flags = client->flags,
			.len = 1 + length,
			.buf = msgbuf0,
		},
	};

	if (length > I2C_SMBUS_BLOCK_MAX)
		return -E2BIG;

	memcpy(msgbuf0 + 1, values, length * sizeof(u8));
	msgbuf0[0] = regaddr;
	msgs[0].buf = msgbuf0; // for fix reviewboard error

	ret = i2c_transfer(client->adapter, msgs, 1);

	if (ret != 1) {
		dev_err(&client->dev, "failed to write reg %#x: %d\n", regaddr, ret);
		ret = -EIO;
	}

	return ret;
}


static int __i2c_reg_read(struct i2c_client *client, u8 regaddr)
{
	int ret;
	u8 buf[4];

	ret = __i2c_block_read(client, regaddr, 1, buf);
	if (ret < 0)
		return ret;

	return buf[0];
}

static int __i2c_reg_write(struct i2c_client *client, u8 regaddr, u8 val)
{
	int ret;

	ret = __i2c_block_write(client, regaddr, 1, &val);
	if (ret < 0)
		return ret;

	return ret;
}

static int __i2c_set_bits(struct i2c_client *client, u8 regaddr, u8 offset, u8 val)
{
	int ret;
	u8 mask;

	ret = __i2c_reg_read(client, regaddr);
	if (ret < 0)
		return ret;

	mask = 1 << offset;

	if (val > 0) {
		ret |= mask;
	}
	else {
		ret &= ~mask;
	}

	return __i2c_reg_write(client, regaddr, ret);
}
#if defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
static void key_led_set_bl_brightness(struct led_classdev *led_cdev,
					enum led_brightness value)
{
	int rc = 0;
	printk( "key_led_set_bl_brightness  value: %d\n", value);
	pmic_gpio_direction_output(KEY_LED_GPIO);
	
	if (value == 255) {
		rc = pmic_gpio_set_value(KEY_LED_GPIO,1);
	}else if(value == 0){
		rc = pmic_gpio_set_value(KEY_LED_GPIO,0);
	}
	
	if (rc < 0) printk("%s pmic gpio set 1 error, data %d", __func__, value);

}

static struct led_classdev key_backlight_led = {
	.name		= "keyboard-backlight",
	.brightness	= 0,
	.brightness_set	= key_led_set_bl_brightness,
};
#endif
static void stmpe1801_regs_init(struct stmpe1801_dev *stmpe1801)
{

#ifdef DBG_PRN
	printk(KERN_NOTICE "dev lock @ %s\n", __func__);
#endif
	mutex_lock(&stmpe1801->dev_lock);

	// 0: SYS_CTRL
	stmpe1801->regs_sys_int_init[0] = stmpe1801->pdata->debounce;
	// 1: INT_CTRL
	stmpe1801->regs_sys_int_init[1] =
		STMPE1801_INT_CTRL_ACTIVE_LOW
#ifdef STMPE1801_TRIGGER_TYPE_FALLING
		|STMPE1801_INT_CTRL_TRIG_EDGE
#else
		|STMPE1801_INT_CTRL_TRIG_LEVEL
#endif
		|STMPE1801_INT_CTRL_GLOBAL_ON;
	// 2: INT_EN_MASK
	stmpe1801->regs_sys_int_init[2] = 0;
	if (stmpe1801->en_blocks & STMPE1801_BLOCK_GPIO)
		stmpe1801->regs_sys_int_init[2] |= STMPE1801_INT_MASK_GPIO;
	if (stmpe1801->en_blocks & STMPE1801_BLOCK_KEY)
		stmpe1801->regs_sys_int_init[2] |= STMPE1801_INT_MASK_KEY|STMPE1801_INT_MASK_KEY_FIFO;

	if ((stmpe1801->en_blocks & STMPE1801_BLOCK_KEY)
		&& (stmpe1801->pdata != NULL) && (stmpe1801->pdata->keypad != NULL)) {		
		unsigned int i;
		// 0 : KPC_ROW
		i = (stmpe1801->pdata->pinmask_keypad_col << 8) | stmpe1801->pdata->pinmask_keypad_row;
		stmpe1801->regs_kpc_init[0] = i & 0xff;	// Row
		// 1 : KPC_COL_LOW
		stmpe1801->regs_kpc_init[1] = (i >> 8) & 0xff; // Col_low
		// 2 : KPC_COL_HIGH
		stmpe1801->regs_kpc_init[2] = (i >> 16) & 0x03; // Col_high
		// Pull-up in unused pin
		i = ~i;
		stmpe1801->regs_pullup_init[0] = i & 0xff;
		stmpe1801->regs_pullup_init[1] = (i >> 8) & 0xff;
		stmpe1801->regs_pullup_init[2] = (i >> 16) & 0x03;
		// 3 : KPC_CTRL_LOW
		i = stmpe1801->pdata->keypad->scan_cnt & 0x0f;
		stmpe1801->regs_kpc_init[3] = (i << STMPE1801_SCAN_CNT_SHIFT) & STMPE1801_SCAN_CNT_MASK;
		// 4 : KPC_CTRL_MID
		stmpe1801->regs_kpc_init[4] = 0x62; // default : 0x62
		// 5 : KPC_CTRL_HIGH
		stmpe1801->regs_kpc_init[5] = (0x40 & ~STMPE1801_SCAN_FREQ_MASK) | STMPE1801_SCAN_FREQ_60HZ;
	}

	if ((stmpe1801->en_blocks & STMPE1801_BLOCK_GPIO)
		&& (stmpe1801->pdata != NULL)) {
		// NOP
	}

#ifdef DBG_PRN
	printk(KERN_NOTICE "dev unlock @ %s\n", __func__);
#endif
	mutex_unlock(&stmpe1801->dev_lock);

	return;
}


static int stmpe1801_hw_init(struct stmpe1801_dev *stmpe1801)
{
	int ret;

	//Check ID
	ret = __i2c_reg_read(stmpe1801->client, STMPE1801_REG_SYS_CHIPID);
#ifdef DBG_PRN
	printk(KERN_NOTICE "Chip ID = %x %x\n" , 
		ret, __i2c_reg_read(stmpe1801->client, STMPE1801_REG_SYS_CHIPID+1));
#endif
	if (ret != 0xc1)  goto err_hw_init;

	//soft reset & set debounce time 
	ret = __i2c_reg_write(stmpe1801->client, STMPE1801_REG_SYS_CTRL, 
		0x80 | stmpe1801->regs_sys_int_init[0]);
	if (ret < 0) goto err_hw_init;
	mdelay(20);	// waiting reset

	// INT CTRL
	ret = __i2c_reg_write(stmpe1801->client, STMPE1801_REG_INT_CTRL_LOW, 
		stmpe1801->regs_sys_int_init[1]);
	if (ret < 0) goto err_hw_init;


	// INT EN Mask
	ret = __i2c_reg_write(stmpe1801->client, STMPE1801_REG_INT_MASK_LOW, 
		stmpe1801->regs_sys_int_init[2]);
	if (ret < 0) goto err_hw_init;

	if (stmpe1801->en_blocks & STMPE1801_BLOCK_KEY) {
		ret = __i2c_block_write(stmpe1801->client, STMPE1801_REG_KPC_ROW, 6, stmpe1801->regs_kpc_init);
		if (ret < 0) goto err_hw_init;
		ret = __i2c_block_write(stmpe1801->client, STMPE1801_REG_GPIO_PULL_UP_yyy, 3, stmpe1801->regs_pullup_init);
		if (ret < 0) goto err_hw_init;
#ifdef DBG_PRN
		printk(KERN_NOTICE "Keypad setting done.\n");
#endif
	}

	if (stmpe1801->en_blocks & STMPE1801_BLOCK_GPIO) {
		// NOP
	}


err_hw_init:

	return ret;
}


static int stmpe1801_dev_power_off(struct stmpe1801_dev *stmpe1801)
{
	int ret = -EINVAL;

#ifdef DBG_PRN
	printk(KERN_NOTICE STMPE1801_DRV_NAME " %s\n", __func__);
#endif

	if (stmpe1801->pdata->power_off) {
		ret = stmpe1801->pdata->power_off(stmpe1801->client);
		if (ret < 0) {
#ifdef DBG_PRN
			printk(KERN_NOTICE "return err1 @ %s\n", __func__);
#endif
			return ret;
		}
	}

	if (atomic_read(&stmpe1801->dev_init) > 0) {
		atomic_set(&stmpe1801->dev_init, 0);
	}

#ifdef DBG_PRN
	printk(KERN_NOTICE "return ok @ %s\n", __func__);
#endif

	return 0;
}


static int stmpe1801_dev_power_on(struct stmpe1801_dev *stmpe1801)
{
	int ret = -EINVAL;

#ifdef DBG_PRN
	printk(KERN_NOTICE STMPE1801_DRV_NAME " %s\n", __func__);
#endif

	if (stmpe1801->pdata->power_on) {
		ret = stmpe1801->pdata->power_on(stmpe1801->client);
		if (ret < 0) {
			return ret;
		}
	}

	if (!atomic_read(&stmpe1801->dev_init)) {
		ret = stmpe1801_hw_init(stmpe1801);
		if (ret < 0) {
			stmpe1801_dev_power_off(stmpe1801);
			return ret;
		}
		atomic_set(&stmpe1801->dev_init, 1);
	}

	return 0;
}


static int stmpe1801_dev_kpc_disable(struct stmpe1801_dev *stmpe1801)
{
	int ret = -EINVAL;

	if (atomic_cmpxchg(&stmpe1801->dev_enabled, 1, 0)) {
#ifdef DBG_PRN
		printk(KERN_NOTICE "dev lock @ %s\n", __func__);
#endif
		mutex_lock(&stmpe1801->dev_lock);

		if (atomic_read(&stmpe1801->dev_init) > 0) {
			if (stmpe1801->pdata->int_gpio_no >= 0)
				disable_irq_nosync(stmpe1801->dev_irq);
			else
				cancel_delayed_work(&stmpe1801->worker);
		}

		ret = stmpe1801_dev_power_off(stmpe1801);
		if (ret < 0) goto err_dis;

		// stop scanning
		ret = __i2c_set_bits(stmpe1801->client, STMPE1801_REG_KPC_CMD, 0, 0);
		if (ret < 0) goto err_dis;

#ifdef DBG_PRN
		printk(KERN_NOTICE "dev unlock @ %s\n", __func__);
#endif
		mutex_unlock(&stmpe1801->dev_lock);
	}

	return 0;

err_dis:
#ifdef DBG_PRN
	printk(KERN_NOTICE "err: dev unlock @ %s\n", __func__);
#endif
	mutex_unlock(&stmpe1801->dev_lock);

	return ret;
}


static int stmpe1801_dev_kpc_enable(struct stmpe1801_dev *stmpe1801)
{
	int ret = -EINVAL;

	if (!atomic_cmpxchg(&stmpe1801->dev_enabled, 0, 1)) {
#ifdef DBG_PRN
		printk(KERN_NOTICE "dev lock @ %s\n", __func__);
#endif
		mutex_lock(&stmpe1801->dev_lock);

		ret = stmpe1801_dev_power_on(stmpe1801);
		if (ret < 0) goto err_en;

		// start scanning
		ret = __i2c_set_bits(stmpe1801->client, STMPE1801_REG_KPC_CMD, 0, 1);
		if (ret < 0) goto err_en;

		if (atomic_read(&stmpe1801->dev_init) > 0) {
			if (stmpe1801->pdata->int_gpio_no >= 0)
				enable_irq(stmpe1801->dev_irq);
			else
				schedule_delayed_work(&stmpe1801->worker, STMPE1801_TMR_INTERVAL);
		}

#ifdef DBG_PRN
		printk(KERN_NOTICE "dev unlock @ %s\n", __func__);
#endif
		mutex_unlock(&stmpe1801->dev_lock);
	}
	return 0;

err_en:
	atomic_set(&stmpe1801->dev_enabled, 0);
#ifdef DBG_PRN
	printk(KERN_NOTICE "err: dev unlock @ %s\n", __func__);
#endif
	mutex_unlock(&stmpe1801->dev_lock);

	return ret;
}

int stmpe1801_dev_kpc_input_open(struct input_dev *dev)
{
	struct stmpe1801_dev *stmpe1801 = input_get_drvdata(dev);

	return stmpe1801_dev_kpc_enable(stmpe1801);
}

void stmpe1801_dev_kpc_input_close(struct input_dev *dev)
{
	struct stmpe1801_dev *stmpe1801 = input_get_drvdata(dev);

	stmpe1801_dev_kpc_disable(stmpe1801);

	return;
}

static void stmpe1801_dev_int_proc(struct stmpe1801_dev *stmpe1801)
{
	int ret;
	union stmpe1801_kpc_data key_values;

	mutex_lock(&stmpe1801->dev_lock);

	// disable chip global int
	__i2c_set_bits(stmpe1801->client, STMPE1801_REG_INT_CTRL_LOW, 0, 0);

	// get int src
	ret = __i2c_reg_read(stmpe1801->client, STMPE1801_REG_INT_STA_LOW);
	if (ret < 0) goto out_proc;

	if (stmpe1801->en_blocks & STMPE1801_BLOCK_KEY) {	// KEYPAD mode
		if((ret & (STMPE1801_INT_MASK_KEY|STMPE1801_INT_MASK_KEY_FIFO)) > 0) { // INT
			int i;
			for (i = 0; i < STMPE1801_KEY_FIFO_LENGTH; i++) {
				ret = __i2c_block_read(stmpe1801->client, STMPE1801_REG_KPC_DATA_BYTE0, 5, key_values.array);
				if (ret) {
					if (key_values.value != 0xffff8f8f8LL) {
						int j;
						__stmpe1801_discarding_ghost(stmpe1801, &key_values);
						for (j = 0; j < 3; j++) {								
							int data, col, row, code;
							bool up;
							data = key_values.array[j];
							col = (data & STMPE1801_KPC_DATA_COL) >> 3;
							row = data & STMPE1801_KPC_DATA_ROW;
							code = MATRIX_SCAN_CODE(row, col, STMPE1801_KPC_ROW_SHIFT);
							up = data & STMPE1801_KPC_DATA_UP ? 1 : 0;

							if (col == STMPE1801_KPC_DATA_NOKEY)
								continue;

#ifdef DBG_PRN
							printk(KERN_WARNING STMPE1801_DRV_NAME " Keycode(%d) code(%x) row:col(%d:%d) up(%d)\n",
								stmpe1801->keycode[code], code, row, col, up);
#endif

							input_event(stmpe1801->input_dev, EV_MSC, MSC_SCAN, code);
							input_report_key(stmpe1801->input_dev, stmpe1801->keycode[code], !up);
							input_sync(stmpe1801->input_dev);
#if defined(CONFIG_SEC_DEBUG) && defined(CONFIG_SEC_DUMP)	
							printk(KERN_ERR "dump_enable_flag :%d\n",dump_enable_flag);
							//if (dump_enable_flag != 0)
								sec_check_crash_key(stmpe1801->keycode[code], !up);
#endif /*CONFIG_SEC_DUMP*/
						}
					}
				}
			}
#ifdef DBG_PRN
			printk(KERN_WARNING STMPE1801_DRV_NAME "-- dev_int_proc --\n");
#endif
		}
	}

out_proc:
	// enable chip global int
	__i2c_set_bits(stmpe1801->client, STMPE1801_REG_INT_CTRL_LOW, 0, 1);

	mutex_unlock(&stmpe1801->dev_lock);
}

static irqreturn_t stmpe1801_dev_isr(int irq, void *handle)
{
	struct stmpe1801_dev *stmpe1801 = (struct stmpe1801_dev *)handle;

	stmpe1801_dev_int_proc(stmpe1801);

	return IRQ_HANDLED;
}


static void stmpe1801_dev_worker(struct work_struct *work)
{
	struct stmpe1801_dev *stmpe1801 = container_of((struct delayed_work *)work, struct stmpe1801_dev, worker);

	stmpe1801_dev_int_proc(stmpe1801);

	schedule_delayed_work(&stmpe1801->worker, STMPE1801_TMR_INTERVAL);
}



// sysfs functions
static ssize_t attr_show_enabled(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct stmpe1801_dev *stmpe1801 = dev_get_drvdata(dev);
	int cnt;

	cnt = sprintf(buf, "%s -> dev_enabled = %d, input->users = %d\n", 
		dev->driver->name, atomic_read(&stmpe1801->dev_enabled), stmpe1801->input_dev->users);

	return cnt;
}

static ssize_t attr_store_enabled(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct stmpe1801_dev *stmpe1801 = dev_get_drvdata(dev);
	unsigned long val;

	val = strict_strtoul(buf, 10, NULL);

	if (val)
		stmpe1801_dev_kpc_enable(stmpe1801);
	else
		stmpe1801_dev_kpc_disable(stmpe1801);

	return count;
}


static struct device_attribute dev_attributes[] = {
	__ATTR(dev_enabled, S_IRUGO|S_IWUSR, attr_show_enabled, attr_store_enabled),
};

static int stmpe1801_dev_create_sysfs(struct device *dev)
{
	int i, ret;

	for (i = 0; i < ARRAY_SIZE(dev_attributes); i++) {
		ret = device_create_file(dev, dev_attributes + i);
		if (ret < 0) goto err_sysfs;
	}

#ifdef DBG_PRN
	printk(KERN_WARNING STMPE1801_DRV_NAME ": sysfs create..\n");
#endif

	return 0;

err_sysfs:
	for ( ; i >= 0; i--)
		device_remove_file(dev, dev_attributes + i);
	dev_err(dev, "%s:Unable to create interface\n", __func__);
	return ret;
}

static void stmpe1801_dev_remove_sysfs(struct device *dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(dev_attributes); i++)
		device_remove_file(dev, dev_attributes + i);
}


static int __devinit stmpe1801_dev_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = -EINVAL;
	//int irq;
	struct stmpe1801_dev *device_data;
	struct stmpe1801_platform_data *platform_data = client->dev.platform_data;


#ifdef DBG_PRN
	printk(KERN_WARNING STMPE1801_DRV_NAME ": stmpe1801_dev_probe v%s\n", DD_VER);
#endif


	if (platform_data == NULL) {
		dev_err(&client->dev, "missing platform data\n");
		return -ENODEV;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "I2C_FUNC_I2C not Supported\n");
		return -EIO;
	}

	device_data = kzalloc(sizeof(struct stmpe1801_dev), GFP_KERNEL);
	if (device_data == NULL) {
		dev_err(&client->dev, "kzalloc memory error\n");
		return -ENOMEM;
	}
#ifdef DBG_PRN
	printk(KERN_WARNING STMPE1801_DRV_NAME ": device_data addr (%p)\n", device_data);
#endif

	device_data->client = client;

	atomic_set(&device_data->dev_enabled, 0);
	atomic_set(&device_data->dev_init, 0);

	mutex_init(&device_data->dev_lock);
	INIT_DELAYED_WORK(&device_data->worker, stmpe1801_dev_worker);

	device_data->en_blocks = platform_data->blocks;
	device_data->client = client;
	// pdata
	device_data->pdata = kzalloc(sizeof(struct stmpe1801_platform_data), GFP_KERNEL);
	if (device_data->pdata == NULL) {
		dev_err(&client->dev, "kzalloc memory error\n");
		goto err_0;
	}
#ifdef DBG_PRN
	printk(KERN_WARNING STMPE1801_DRV_NAME ": pdata addr (%p)\n", device_data->pdata);
#endif
	memcpy(device_data->pdata, platform_data, sizeof(struct stmpe1801_platform_data));

#ifdef CONFIG_MACH_HENNESSY_DUOS_CTC
	////zixiaolong add. GPIO re-init
	gpio_tlmm_config(GPIO_CFG(112, 0, GPIO_CFG_INPUT,
			GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(17, 0, GPIO_CFG_INPUT,
			GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);	
	gpio_tlmm_config(GPIO_CFG(80, 0, GPIO_CFG_INPUT,
			GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(72, 0, GPIO_CFG_INPUT,
			GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);	
	//gpio_set_value(72, 1);
	
#endif

	// ghost key list
	if (platform_data->keypad->ghost_data != NULL) {
		device_data->ghost_key_entries = platform_data->keypad->ghost_data->keymap_size;
		device_data->ghost_keys = kzalloc(sizeof(uint32_t) * device_data->ghost_key_entries, GFP_KERNEL);
		if (device_data->ghost_keys == NULL) {
			dev_err(&client->dev, "kzalloc memory error\n");
			goto err_1;
		}
#ifdef DBG_PRN
		printk(KERN_WARNING STMPE1801_DRV_NAME ": ghost_keys addr (%p)\n", device_data->ghost_keys);
#endif
		memcpy(device_data->ghost_keys, platform_data->keypad->ghost_data->keymap, 
			sizeof(uint32_t) * device_data->ghost_key_entries);
	}
	else {
		device_data->ghost_key_entries = 0;
	}

	stmpe1801_regs_init(device_data);


	device_data->keycode_entries = __stmpe1801_get_keycode_entries(device_data->pdata->pinmask_keypad_row, device_data->pdata->pinmask_keypad_col);
#ifdef DBG_PRN
	printk(KERN_WARNING STMPE1801_DRV_NAME ": keycode entries (%d)\n", device_data->keycode_entries);
	printk(KERN_WARNING STMPE1801_DRV_NAME ": keymap size (%d)\n", device_data->pdata->keypad->keymap_data->keymap_size);
#endif
	device_data->keycode = kzalloc(device_data->keycode_entries * sizeof(unsigned short), GFP_KERNEL);
	if (device_data->keycode == NULL) {
		dev_err(&client->dev, "kzalloc memory error\n");
		goto err_2;
	}
#ifdef DBG_PRN
	printk(KERN_WARNING STMPE1801_DRV_NAME ": keycode addr (%p)\n", device_data->keycode);
#endif

	i2c_set_clientdata(client, device_data);

	ret = stmpe1801_dev_power_on(device_data);
	if (ret < 0) goto err_3;

	ret = stmpe1801_dev_power_off(device_data);
	if (ret < 0) goto err_3;


	// Make input dev
	device_data->input_dev = input_allocate_device();
	if (!device_data->input_dev) goto err_3;

	device_data->input_dev->dev.parent = &client->dev;
	device_data->input_dev->name = STMPE1801_DRV_NAME;
	device_data->input_dev->id.bustype = BUS_I2C;
	device_data->input_dev->flush = NULL;
	device_data->input_dev->event = NULL;
	device_data->input_dev->open = stmpe1801_dev_kpc_input_open;
	device_data->input_dev->close = stmpe1801_dev_kpc_input_close;


	input_set_drvdata(device_data->input_dev, device_data);
	input_set_capability(device_data->input_dev, EV_MSC, MSC_SCAN);
	__set_bit(EV_KEY, device_data->input_dev->evbit);
	if (device_data->pdata->keypad->autorepeat)
		__set_bit(EV_REP, device_data->input_dev->evbit);

	device_data->input_dev->keycode = device_data->keycode;
	device_data->input_dev->keycodesize = sizeof(unsigned short);
	device_data->input_dev->keycodemax = device_data->keycode_entries;

	matrix_keypad_build_keymap(device_data->pdata->keypad->keymap_data, STMPE1801_KPC_ROW_SHIFT,
		device_data->input_dev->keycode, device_data->input_dev->keybit);


	// request irq
	if (device_data->pdata->int_gpio_no >= 0) {
		gpio_tlmm_config(GPIO_CFG(device_data->pdata->int_gpio_no, 0, GPIO_CFG_INPUT,
			GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);//zixiaolong add
		device_data->dev_irq = gpio_to_irq(device_data->pdata->int_gpio_no);
		
#ifdef DBG_PRN
		printk(KERN_WARNING STMPE1801_DRV_NAME ": INT mode (%d)\n", device_data->dev_irq);
#endif
		ret = request_threaded_irq(device_data->dev_irq, NULL, stmpe1801_dev_isr,
#ifdef STMPE1801_TRIGGER_TYPE_FALLING
#warning "Falling triggering"
			IRQF_TRIGGER_FALLING|IRQF_ONESHOT,
#else
#warning "Level triggering"
			IRQF_TRIGGER_LOW|IRQF_ONESHOT,
#endif
			client->name, device_data);

		if (ret < 0) {
			goto err_4;
		}
		disable_irq_nosync(device_data->dev_irq);
	}
	else {
#ifdef DBG_PRN
		printk(KERN_WARNING STMPE1801_DRV_NAME ": poll mode\n");
#endif
		schedule_delayed_work(&device_data->worker, STMPE1801_TMR_INTERVAL); 
	}


	ret = input_register_device(device_data->input_dev);
	if (ret) goto err_4;

	stmpe1801_dev_create_sysfs(&client->dev);

	if (device_data->pdata->int_gpio_no >= 0)
		device_init_wakeup(&client->dev, device_data->pdata->wake_up);

#if defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
	if (led_classdev_register(&client->dev, &key_backlight_led))
		printk(KERN_ERR "[key] led_classdev_register failed\n");
#endif	
	return 0;

err_4:
	input_free_device(device_data->input_dev);

err_3:
	kfree(device_data->keycode);

err_2:
	if (device_data->ghost_keys)
		kfree(device_data->ghost_keys);

err_1:
	kfree(device_data->pdata);

err_0:
	kfree(device_data);

#ifdef DBG_PRN
	printk(KERN_ERR STMPE1801_DRV_NAME ": Error at stmpe1801_dev_probe\n");
#endif

	return ret;
}

static int __devexit stmpe1801_dev_remove(struct i2c_client *client)
{
	struct stmpe1801_dev *device_data = i2c_get_clientdata(client);

	device_init_wakeup(&client->dev, 0);

	stmpe1801_dev_remove_sysfs(&client->dev);

	input_unregister_device(device_data->input_dev);
	input_free_device(device_data->input_dev);

	
	if (device_data->pdata->int_gpio_no >= 0)
		free_irq(device_data->dev_irq, device_data);

	kfree(device_data->keycode);
	if (device_data->ghost_keys)
		kfree(device_data->ghost_keys);
	kfree(device_data->pdata);
	kfree(device_data);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int stmpe1801_dev_suspend(struct device *dev)
{
	struct stmpe1801_dev *device_data = dev_get_drvdata(dev);

#ifdef DBG_PRN
	printk(KERN_NOTICE STMPE1801_DRV_NAME " %s\n", __func__);
#endif

	if (device_may_wakeup(dev)) {
		if (device_data->pdata->int_gpio_no >= 0)
			enable_irq_wake(device_data->dev_irq);
	} else {
		mutex_lock(&device_data->input_dev->mutex);

		if (device_data->input_dev->users)
			stmpe1801_dev_kpc_disable(device_data);

		mutex_unlock(&device_data->input_dev->mutex);
	}

	return 0;
}

static int stmpe1801_dev_resume(struct device *dev)
{
	struct stmpe1801_dev *device_data = dev_get_drvdata(dev);

#ifdef DBG_PRN
	printk(KERN_NOTICE STMPE1801_DRV_NAME " %s\n", __func__);
#endif

	if (device_may_wakeup(dev)) {
		if (device_data->pdata->int_gpio_no >= 0)
			disable_irq_wake(device_data->dev_irq);
	} else {
		mutex_lock(&device_data->input_dev->mutex);

		if (device_data->input_dev->users)
			stmpe1801_dev_kpc_enable(device_data);

		mutex_unlock(&device_data->input_dev->mutex);
	}

	return 0;
}
#endif


static const struct i2c_device_id stmpe1801_dev_id[] = {
	{ STMPE1801_DRV_NAME, 0 },
	{ }
};



static SIMPLE_DEV_PM_OPS(stmpe1801_dev_pm_ops, stmpe1801_dev_suspend, stmpe1801_dev_resume);


static struct i2c_driver stmpe1801_dev_driver = {
	.driver = {
		.name = STMPE1801_DRV_NAME,
		.owner = THIS_MODULE,
		.pm = &stmpe1801_dev_pm_ops,
	},
	.probe = stmpe1801_dev_probe,
	.remove = __devexit_p(stmpe1801_dev_remove),
	.id_table = stmpe1801_dev_id,
};

static int __init stmpe1801_dev_init(void)
{
#ifdef DBG_PRN
	printk(KERN_WARNING "%s driver init\n", STMPE1801_DRV_NAME); 	
#endif

	return i2c_add_driver(&stmpe1801_dev_driver);
}

static void __exit stmpe1801_dev_exit(void)
{
	i2c_del_driver(&stmpe1801_dev_driver);

#ifdef DBG_PRN	
	printk(KERN_WARNING "%s driver exit\n", STMPE1801_DRV_NAME); 
#endif
}


module_init(stmpe1801_dev_init);
module_exit(stmpe1801_dev_exit);

MODULE_DEVICE_TABLE(i2c, stmpe1801_dev_id);

MODULE_DESCRIPTION("STMPE1801 Key Driver");
MODULE_AUTHOR("Gyusung SHIM <gyusung.shim@st.com>");
MODULE_LICENSE("GPL");

