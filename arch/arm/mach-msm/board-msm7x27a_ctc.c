/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gpio_event.h>
#include <linux/usb/android.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/i2c.h>
#include <linux/android_pmem.h>
#include <linux/bootmem.h>
#include <linux/mfd/marimba.h>
#include <linux/power_supply.h>
#include <linux/input/rmi_platformdata.h>
#include <linux/input/rmi_i2c.h>
#include <linux/i2c/atmel_mxt_ts.h>
#include <linux/regulator/consumer.h>
#include <linux/memblock.h>
#include <linux/input/ft5x06_ts.h>
#include <linux/msm_adc.h>
#include <linux/regulator/msm-gpio-regulator.h>
#include <linux/ion.h>
#include <linux/i2c-gpio.h>
#include <linux/regulator/onsemi-ncp6335d.h>
#include <linux/dma-contiguous.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <asm/mach/mmc.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/hardware/gic.h>
#include <mach/board.h>
#include <mach/msm_iomap.h>
#include <mach/msm_hsusb.h>
#include <mach/rpc_hsusb.h>
#include <mach/rpc_pmapp.h>
#include <mach/usbdiag.h>
#include <mach/msm_memtypes.h>
#include <mach/msm_serial_hs.h>
#include <mach/msm_serial_pdata.h>
#include <mach/pmic.h>
#include <mach/socinfo.h>
#include <mach/vreg.h>
#include <mach/rpc_pmapp.h>
#include <mach/msm_battery.h>
#include <mach/rpc_server_handset.h>
#include <mach/socinfo.h>
#include "board-msm7x27a-regulator.h"
#include "devices.h"
#include "devices-msm7x2xa.h"
#include "pm.h"
#include "timer.h"
#include "pm-boot.h"
#include "board-msm7x27a-regulator.h"
#include "board-msm7627a.h"
#ifdef CONFIG_SEC_DEBUG
#include <linux/sec_debug.h>
#endif
#include <linux/mutex.h>

#ifndef CONFIG_BT_CSR_7820
#define CONFIG_BT_CSR_7820
#endif
/////////////////////////////////////////////////////////////////////////////////


#include <mach/gpio_delos.h>

#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/i2c/sx150x.h>

#if defined(CONFIG_SAMSUNG_JACK) || defined(CONFIG_SEC_FSA_JACK)
#include <linux/sec_jack.h>
#endif

//#include <linux/fsaxxxx_usbsw.h>
#include <linux/platform_data/fsa9480.h>
#ifdef CONFIG_PROXIMITY_SENSOR
#include <linux/gp2a.h>
#endif
#ifdef CONFIG_SENSORS_GP2A030
#include <linux/gp2ap030.h>
#endif
#ifdef CONFIG_SENSORS_GYRO_K3G
#include <linux/k3g.h>
#endif

#ifdef CONFIG_TOUCHSCREEN_MELFAS
#include <linux/i2c/melfas_ts.h>
#endif
#ifdef CONFIG_MAX17048_FUELGAUGE
#include <linux/fuelgauge_max17048.h>
#endif
#if defined(CONFIG_KEYBOARD_GPIO) || defined(CONFIG_KEYBOARD_WAKEUP)
#include <linux/gpio_keys.h>
#endif

#ifdef CONFIG_CHARGER_FAN54013
#include <linux/power/fan54013_charger.h>
#endif
#ifdef CONFIG_CHARGER_BQ24157
#include <linux/power/bq24157_charger.h>
#endif
#ifdef CONFIG_CHARGER_SMB328A
#include <linux/power/smb328a_charger.h>
#endif

#if defined (CONFIG_SENSORS_BMA2X2)
#include "../../../drivers/sensors/bst_sensor_common.h"
#endif

#include "proc_comm.h"

#ifdef CONFIG_KEYBOARD_STMPE1801
#include <linux/input/matrix_keypad.h>
#include <linux/input/stmpe1801_key.h>
#endif

#ifdef CONFIG_TOUCHSCREEN_SYNAPTICS_DSX_I2C
#include <linux/input/synaptics_dsx.h>
#endif

#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_S2200_I2C)
#include <linux/interrupt.h>
#include <linux/rmi.h>
#include <mach/gpio.h>


#define SYNA_TM2303 0x00000800

#define SYNA_BOARDS SYNA_TM2303 /* (SYNA_TM1333 | SYNA_TM1414) */
#define SYNA_BOARD_PRESENT(board_mask) (SYNA_BOARDS & board_mask)

struct syna_gpio_data {
	u16 gpio_number;
	char* gpio_name;
};

#define TOUCH_ON 1
#define TOUCH_OFF 0
//#define TOUCH_POWER_GPIO 43 //PSJ

static int s2200_ts_power(int on_off)
{

	pr_info("%s: TS power change to %d.\n", __func__, on_off);
	int rc;
	struct vreg *vreg_lcd = NULL;
	printk("start %s\n", __func__);
	if (vreg_lcd == NULL) {
		vreg_lcd = vreg_get(NULL, "ldo19");

		if (IS_ERR(vreg_lcd)) {
			printk(KERN_ERR "%s: vreg_get(%s) failed (%ld)\n", __func__, "vlcd4", PTR_ERR(vreg_lcd));
			return;
		}

		rc = vreg_set_level(vreg_lcd, 3000);
		if (rc) {
			printk(KERN_ERR "%s: TSP set_level failed (%d)\n", __func__, rc);
		}
	}

	if (on_off) {
		rc = vreg_enable(vreg_lcd);
		if (rc) {
			printk(KERN_ERR "%s: TSP enable failed (%d)\n", __func__, rc);
		} else {
			printk(KERN_ERR "%s: TSP enable success (%d)\n", __func__, rc);
		}
	} else {
		rc = vreg_disable(vreg_lcd);
		if (rc) {
			printk(KERN_ERR "%s: TSP disable failed (%d)\n", __func__, rc);
		} else {
			printk(KERN_ERR "%s: TSP disable success (%d)\n", __func__, rc);
		}
	}
	return 0;
}
static int synaptics_touchpad_gpio_setup(void *gpio_data, bool configure)
{
	int retval=0;
	struct syna_gpio_data *data = gpio_data;

	pr_info("%s: RMI4 gpio configuration set to %d.\n", __func__,
		configure);

	if (configure) {
		retval = gpio_request(data->gpio_number, "rmi4_attn");
		if (retval) {
			pr_err("%s: Failed to get attn gpio %d. Code: %d.",
			       __func__, data->gpio_number, retval);
			return retval;
		}

		//omap_mux_init_signal(data->gpio_name, OMAP_PIN_INPUT_PULLUP);
		retval = gpio_direction_input(data->gpio_number);
		if (retval) {
			pr_err("%s: Failed to setup attn gpio %d. Code: %d.",
			       __func__, data->gpio_number, retval);
			gpio_free(data->gpio_number);
		}
	} else {
		pr_warn("%s: No way to deconfigure gpio %d.",
		       __func__, data->gpio_number);
	}
	gpio_tlmm_config(GPIO_CFG(27, 0, GPIO_CFG_INPUT,
				GPIO_CFG_PULL_UP, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	
	gpio_tlmm_config(GPIO_CFG(118, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	gpio_set_value(118,0);					// TSP sel  gpio , 0 : main tsp, 1 sub tsp
	gpio_tlmm_config(GPIO_CFG(132, 0, GPIO_CFG_INPUT,
				GPIO_CFG_PULL_UP, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(131, 0, GPIO_CFG_INPUT,
				GPIO_CFG_PULL_UP, GPIO_CFG_2MA),GPIO_CFG_ENABLE);

	return s2200_ts_power(configure);
}
#endif

#ifdef CONFIG_KEYBOARD_STMPE1801
/************************** platform_data for expansion port ***************************/
// STMPE1801 KEY ===================================

#define GPIO_EXP_PORT_IRQ	111

static const unsigned int cust_keymap[] = {
	KEY(0, 0, KEY_SEND), // SEND
	KEY(0, 1, KEY_1),	// 1
	KEY(0, 2, KEY_4),	// 4
	KEY(0, 3, KEY_7),	// 7
	KEY(0, 4, KEY_NUMERIC_STAR),	// *

	KEY(1, 0, KEY_BACKSPACE),	// clear
	KEY(1, 1, KEY_2),	// 2
	KEY(1, 2, KEY_5),	// 5
	KEY(1, 3, KEY_8),	// 8
	KEY(1, 4, KEY_0),	// 0

	KEY(2, 0, KEY_HOMEPAGE),	// MULTI
	KEY(2, 1, KEY_3),	// 3
	KEY(2, 2, KEY_6),	// 6
	KEY(2, 3, KEY_9),	// 9
	KEY(2, 4, KEY_NUMERIC_POUND),	// #

	KEY(3, 0, KEY_NETWORK),	// C/G
	KEY(3, 1, KEY_LEFT),	// navi_l
	KEY(3, 2, KEY_BACK),	// back
	KEY(3, 3, KEY_UP),	// navi_up
	KEY(3, 4, KEY_MENU),	// menu

	KEY(4, 0, KEY_SELECT),	// navi_ok
	KEY(4, 1, KEY_DOWN),	// navi_dn
	KEY(4, 2, KEY_RIGHT),	// navi_r
	KEY(4, 3, KEY_RESERVED),	// no use
	KEY(4, 4, KEY_RESERVED),	// no use

	KEY(5, 0, KEY_VOLUMEUP),	// vol_up
	KEY(5, 1, KEY_VOLUMEDOWN),	// vol_dn
	KEY(5, 2, KEY_WAKEUP),	// hold
	KEY(5, 3, KEY_RESERVED),	// no use
	KEY(5, 4, KEY_RESERVED),	// no use
};

static const struct matrix_keymap_data stmpe_keymap_data = {
	.keymap         = cust_keymap,
	.keymap_size    = ARRAY_SIZE(cust_keymap),
};


// STMPE1801 Ghost key combination
static const unsigned int gho_keymap[] = {
	STMPE1801_GHOST_KEY_VAL(1,0, 1,1, 6,0),
	STMPE1801_GHOST_KEY_VAL(1,0, 6,1, 6,0),
	STMPE1801_GHOST_KEY_VAL(1,1, 6,1, 6,0),
};


static const struct matrix_keymap_data stmpe1801_ghostkey_data = {
	.keymap         = NULL,//not used now. gho_keymap,
	.keymap_size    = 0,//ARRAY_SIZE(gho_keymap),
};

static struct stmpe1801_keypad_data stmpe1801_keypad = {
	.keymap_data = &stmpe_keymap_data,
	.ghost_data = &stmpe1801_ghostkey_data,
	.scan_freq = STMPE1801_SCAN_FREQ_275HZ,
	.scan_cnt = 3,
	.autorepeat = false,
};

static struct stmpe1801_platform_data stmpe1801_data = {
	.blocks = STMPE1801_BLOCK_KEY,
	.debounce = STMPE1801_DEBOUNCE_210US,
	.pinmask_keypad_row = 0x3f,
	.pinmask_keypad_col = 0x1f,

	.int_gpio_no = GPIO_EXP_PORT_IRQ,
	.wake_up = 0,
	.keypad = &stmpe1801_keypad,
	.power_on = NULL,
	.power_off = NULL,
};


static struct i2c_gpio_platform_data stmpe1801_i2c_gpio_data = {
	.sda_pin    = 37,
	.scl_pin    = 40,
	.udelay	= 1,
};
static struct platform_device stmpe1801_i2c_gpio_device = {  
	.name       = "i2c-gpio",
	.id     =  8,
	.dev        = {
		.platform_data  = &stmpe1801_i2c_gpio_data,
	},
};

// Expansion
static struct i2c_board_info stmpe1801_i2c_expansion[] = {
#warning "STMPE1801_KEY"
	{
		I2C_BOARD_INFO(STMPE1801_DRV_NAME, 0x40),
		.platform_data = &stmpe1801_data,
	},
};
	
#endif //CONFIG_KEYBOARD_STMPE1801

extern unsigned int board_hw_revision;

#if defined (CONFIG_MACH_DELOS_CTC)
static struct platform_device delos_charger_gpio_device;
#endif

#define WLAN_33V_CONTROL_FOR_BT_ANTENNA

#define WLAN_OK (0)
#define WLAN_ERROR (-1)

#ifdef WLAN_33V_CONTROL_FOR_BT_ANTENNA
#define WLAN_33V_WIFI_FLAG (0x01)
#define WLAN_33V_BT_FLAG (0x02)

int wlan_33v_flag;

int wlan_setup_ldo_33v(int input_flag, int on);
#endif

//#define GPIO_RST_BT	77
//#define GPIO_BT_EN	82
#define GPIO_BT_LEVEL_LOW			0
#define GPIO_BT_LEVEL_HIGH			1
#define GPIO_BT_LEVEL_NONE			2

#ifdef CONFIG_PROXIMITY_SENSOR
#define GPIO_PROX_INT 29
#define GPIO_PROX_LDO_EN 82
#endif
#ifdef CONFIG_INPUT_BMA2x2_ACC_ALERT_INT
#define ACC_INT 38
#endif
#ifdef CONFIG_SENSORS_GP2A030
#define GPIO_PROX_INT 29
#define GPIO_PROX_LDO_EN 82
#endif
#ifdef CONFIG_SENSORS_GYRO_K3G
#define MSM_GPIO_GYRO_FIFO_INT	30 	               /* MSM GPIO Number 30 */
#endif

#if defined(CONFIG_SAMSUNG_JACK) || defined(CONFIG_SEC_FSA_JACK)
#if defined(CONFIG_MACH_DELOS_CTC) || defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
#define GPIO_JACK_S_35	48
#define GPIO_SEND_END	92
#else
#define GPIO_JACK_S_35	48
#define GPIO_SEND_END	92
#endif

static struct sec_jack_zone jack_zones[] = {
	[0] = {
		.adc_high	= 3,
		.delay_ms	= 10,
		.check_count	= 5,
		.jack_type	= SEC_HEADSET_3POLE,
	},
	[1] = {
		.adc_high	= 99,
		.delay_ms	= 10,
		.check_count	= 10,
		.jack_type	= SEC_HEADSET_3POLE,
	},
	[2] = {
		.adc_high	= 9999,
		.delay_ms	= 10,
		.check_count	= 5,
		.jack_type	= SEC_HEADSET_4POLE,
	},
};

int get_msm7x27a_det_jack_state(void)
{
	/* Active Low */
	return(gpio_get_value(GPIO_JACK_S_35)) ^ 1;
}
EXPORT_SYMBOL(get_msm7x27a_det_jack_state);

static int get_msm7x27a_send_key_state(void)
{
	/* Aruba use hssd module for send-end interrupt */
	return current_key_state;
}

#define SMEM_PROC_COMM_MICBIAS_ONOFF		PCOM_OEM_MICBIAS_ONOFF
#define SMEM_PROC_COMM_MICBIAS_ONOFF_REG5	PCOM_OEM_MICBIAS_ONOFF_REG5
#define SMEM_PROC_COMM_GET_ADC				PCOM_OEM_SAMSUNG_GET_ADC

#define SCAN_00 36
#define SCAN_01 31
#define SENSE_00 39
#define SENSE_01 37

#ifdef CONFIG_TOUCHSCREEN_MELFAS
#define MELFAS_TSP_ADDR 0x48	// melfas TSP slave address
#endif

enum {
	SMEM_PROC_COMM_GET_ADC_BATTERY = 0x0,
	SMEM_PROC_COMM_GET_ADC_TEMP,
	SMEM_PROC_COMM_GET_ADC_VF,
	SMEM_PROC_COMM_GET_ADC_ALL, // data1 : VF(MSB 2 bytes) vbatt_adc(LSB 2bytes), data2 : temp_adc
	SMEM_PROC_COMM_GET_ADC_EAR_ADC,		// 3PI_ADC
	SMEM_PROC_COMM_GET_ADC_MAX,
};

enum {
	SMEM_PROC_COMM_MICBIAS_CONTROL_OFF = 0x0,
	SMEM_PROC_COMM_MICBIAS_CONTROL_ON,
	SMEM_PROC_COMM_MICBIAS_CONTROL_MAX
};

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_TMA46X
#include "board-touch-cyttsp4_core.c"
#endif

static void set_msm7x27a_micbias_state_reg5(bool state)
{
	/* int res = 0;
	 * int data1 = 0;
	 * int data2 = 0;
	 * if (!state)
	 * {
		 * data1 = SMEM_PROC_COMM_MICBIAS_CONTROL_OFF;
		 * res = msm_proc_comm(SMEM_PROC_COMM_MICBIAS_ONOFF_REG5, &data1, &data2);
		 * if(res < 0)
		 * {
			 * pr_err("sec_jack: micbias_reg5 %s  fail \n",state?"on":"off");
		 * }
	 * } */
}

static bool cur_state = false;

static void set_msm7x27a_micbias_state(bool state)
{

	if(cur_state == state)
	{
		pr_info("sec_jack : earmic_bias same as cur_state\n");
		return;
	}

	if(state)
	{
		pmic_hsed_enable(PM_HSED_CONTROLLER_0, PM_HSED_ENABLE_ALWAYS);
		msleep(130);
		cur_state = true;
	}
	else
	{
		pmic_hsed_enable(PM_HSED_CONTROLLER_0, PM_HSED_ENABLE_OFF);
		cur_state = false;
	}

	report_headset_status(state);

	pr_info("sec_jack : earmic_bias %s\n", state?"on":"off");

}

static int sec_jack_get_adc_value(void)
{
	return current_jack_type;
}

void sec_jack_gpio_init(void)
{

#ifdef CONFIG_MACH_HENNESSY_DUOS_CTC
	gpio_tlmm_config(GPIO_CFG(73, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	gpio_set_value(73,1);
#else
	gpio_tlmm_config(GPIO_CFG(GPIO_JACK_S_35, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE); //gpio 48 JACK_INT_N
	if(gpio_request(GPIO_JACK_S_35, "h2w_detect")<0)
		pr_err("sec_jack:gpio_request fail\n");
	if(gpio_direction_input(GPIO_JACK_S_35)<0)
		pr_err("sec_jack:gpio_direction fail\n");
#endif

}

static struct sec_jack_platform_data sec_jack_data = {
	.get_det_jack_state	= get_msm7x27a_det_jack_state,
	.get_send_key_state	= get_msm7x27a_send_key_state,
	.set_micbias_state	= set_msm7x27a_micbias_state,
	.set_micbias_state_reg5	= set_msm7x27a_micbias_state_reg5,
	.get_adc_value	= sec_jack_get_adc_value,
	.zones		= jack_zones,
	.num_zones	= ARRAY_SIZE(jack_zones),
	.det_int	= MSM_GPIO_TO_INT(GPIO_JACK_S_35),
	.send_int	= MSM_GPIO_TO_INT(GPIO_SEND_END),
};

static struct platform_device sec_device_jack = {
	.name           = "sec_jack",
	.id             = -1,
	.dev            = {
		.platform_data  = &sec_jack_data,
	},
};
#endif

#ifdef CONFIG_MSM_VIBRATOR
static struct platform_device msm_vibrator_device = {
	.name	= "msm_vibrator",
	.id		= -1,
};
#endif

int charging_boot;
EXPORT_SYMBOL(charging_boot);
int	fota_boot;
EXPORT_SYMBOL(fota_boot);


struct class *sec_class;
EXPORT_SYMBOL(sec_class);

static void samsung_sys_class_init(void)
{
	pr_info("samsung sys class init.\n");

	sec_class = class_create(THIS_MODULE, "sec");
	if (IS_ERR(sec_class))
		pr_err("Failed to create class(sec) !\n");

}

void sensor_gpio_init(void)
{
        printk("sensor gpio init!!");
	gpio_tlmm_config(GPIO_CFG(GPIO_SENSOR_SCL, 0, GPIO_CFG_INPUT,
				GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(GPIO_SENSOR_SDA, 0, GPIO_CFG_INPUT,
				GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
#if defined (CONFIG_PROXIMITY_SENSOR) || defined(CONFIG_SENSORS_GP2A030) 
        gpio_tlmm_config(GPIO_CFG(GPIO_PROX_INT,0, GPIO_CFG_INPUT, 
                                GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
#endif
#ifdef CONFIG_SENSORS_GYRO_K3G
	gpio_tlmm_config(GPIO_CFG(MSM_GPIO_GYRO_FIFO_INT, 0, GPIO_CFG_INPUT, 
				GPIO_CFG_PULL_DOWN, GPIO_CFG_8MA), GPIO_CFG_ENABLE);
#endif
#ifdef CONFIG_INPUT_BMA2x2_ACC_ALERT_INT
        gpio_tlmm_config(GPIO_CFG(ACC_INT,0, GPIO_CFG_INPUT, 
                                GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
#endif
}

#ifdef CONFIG_PROXIMITY_SENSOR
static int gp2a_power(bool on)
{
	gpio_tlmm_config(GPIO_CFG(GPIO_PROX_LDO_EN,0, GPIO_CFG_OUTPUT, 
	                        GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (on) {
	        gpio_set_value(GPIO_PROX_LDO_EN, 1);
	} else {
		gpio_set_value(GPIO_PROX_LDO_EN, 0);
        }
	printk("End of gp2a function\n");
	return 0;
}
static struct gp2a_platform_data gp2a_pdata = {
	.p_out =29,
	.power =gp2a_power,
};
#endif//CONFIG_PROXIMITY_SENSOR

#ifdef CONFIG_SENSORS_GP2A030
static void gp2a_power_on(int onoff)
{
	/* Power On */
	#ifdef CONFIG_MACH_HENNESSY_DUOS_CTC
	int ret=0;
	printk(KERN_INFO "[GP2A] %s called",__func__); 
	gpio_tlmm_config(GPIO_CFG(GPIO_PROX_LDO_EN,0, GPIO_CFG_OUTPUT, 
	                        GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (onoff) {
	        gpio_set_value(GPIO_PROX_LDO_EN, 1);
	} else {
		gpio_set_value(GPIO_PROX_LDO_EN, 0);
        }
	#endif

}

static void gp2a_led_onoff(int onoff)
{
	gpio_tlmm_config(GPIO_CFG(GPIO_PROX_LDO_EN,0, GPIO_CFG_OUTPUT, 
	                        GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (onoff) {
	        gpio_set_value(GPIO_PROX_LDO_EN, 1);
	} else {
		gpio_set_value(GPIO_PROX_LDO_EN, 0);
        }

}

static int gp2a_get_threshold(void)
{
	int new_threshold = 7; /* LTH value */

//	new_threshold = 100;

	return new_threshold;
}

static struct gp2a_platform_data gp2a_platform_data = {
	.p_out = GPIO_PROX_INT,
    	.power_gpio = GPIO_PROX_LDO_EN,        
    	.power_on = gp2a_power_on,
    	.gp2a_led_on	= gp2a_led_onoff,
    	.gp2a_get_threshold = gp2a_get_threshold,
};

static struct platform_device opt_gp2a = {
	.name = "gp2a-opt",
	.id = -1,
	.dev = {
		.platform_data = &gp2a_platform_data,
	},
};

#endif //CONFIG_SENSORS_GP2A030

#if 0//def CONFIG_SENSORS_GYRO_K3G
static struct k3g_platform_data k3g_pdata = {
	.power_on = sensor_power_on,
	.power_off = sensor_power_off,
};

static struct platform_device gyro_i2c_gpio_device = {  
	.name       = "i2c-gpio",
	.id     = MSM_GYRO_I2C_BUS_ID,
	.dev        = {
		.platform_data  = &k3g_pdata,
	},
};
#endif

#if defined(CONFIG_SENSORS_BMA2X2)
struct bma2x2_platform_data {
	int p_out;  /* acc-sensor-irq gpio */
	int (*power)(bool); /* power to the chip */
	char *name;
	/* 0 to 7 */
	unsigned int place:3;
	int irq;
};

static struct bma2x2_platform_data bma2x2_pdata = {
	.p_out = ACC_INT,
	.name = "bmc150", /*the real sensor name of the whole package*/
	.place = 3,
};
#endif //CONFIG_SENSORS_BMA2X2

#if defined(CONFIG_SENSORS_BMM050)
static struct bosch_sensor_specific bss_bmm = {
	.name = "bmc150",
	.place = 3,
};
#endif

static struct i2c_gpio_platform_data sensor_i2c_gpio_data = {
	.sda_pin    = GPIO_SENSOR_SDA,
	.scl_pin    = GPIO_SENSOR_SCL,
	.udelay	= 1,
};
static struct platform_device sensor_i2c_gpio_device = {  
	.name       = "i2c-gpio",
	.id     =  4,
	.dev        = {
		.platform_data  = &sensor_i2c_gpio_data,
	},
};

static struct i2c_board_info sensor_devices[] = {
#if defined(CONFIG_SENSORS_BMA222)
{
	I2C_BOARD_INFO("bma222", 0x08),
},
#endif
#if defined(CONFIG_SENSORS_BMA222E)
{
	I2C_BOARD_INFO("bma222e", 0x18),
},
#endif
#if defined(CONFIG_SENSORS_HSCD)
{
	I2C_BOARD_INFO("hscd_i2c", 0x0c),
},
#endif
#if defined(CONFIG_SENSORS_BMA2X2)
{
	I2C_BOARD_INFO("bma2x2", 0x10),
#ifdef CONFIG_INPUT_BMA2x2_ACC_ALERT_INT
        .platform_data = &bma2x2_pdata,
#endif
},
#endif
#if defined(CONFIG_SENSORS_BMM050)
{
	I2C_BOARD_INFO("bmm050", 0x12),
	.platform_data = &bss_bmm,
},
#endif
#if defined(CONFIG_PROXIMITY_SENSOR)
{
	I2C_BOARD_INFO("gp2a", 0x44 ),
	.platform_data = &gp2a_pdata,
	},
#endif
#if defined(CONFIG_SENSORS_GP2A030)
{
	I2C_BOARD_INFO("gp2a", 0x72>>1),
},
#endif
#if defined(CONFIG_SENSORS_GYRO_K3G)
{
	I2C_BOARD_INFO("k3g", 0x69),
	.irq = MSM_GPIO_TO_INT(MSM_GPIO_GYRO_FIFO_INT),
},
#endif
};


#if defined(CONFIG_TOUCHSCREEN_CYPRESS_TMA46X) 
	//I2C_BOARD_INFO(CYTTSP4_I2C_NAME, CYTTSP4_I2C_TCH_ADR),
	//.irq = gpio_to_irq(TMA400_GPIO_TSP_INT),
	//.platform_data = CYTTSP4_I2C_NAME,
static struct i2c_gpio_platform_data touch_i2c_gpio_data = {
	.sda_pin    = GPIO_TSP_SDA,
	.scl_pin    = GPIO_TSP_SCL,
	.udelay	= 1,
};
static struct platform_device touch_i2c_gpio_device = {
	.name       = "i2c-gpio",
	.id     =  5,
	.dev        = {
		.platform_data  = &touch_i2c_gpio_data,
	},
};
/* I2C 2 */
static struct i2c_board_info touch_i2c_devices[] = {
	{
		I2C_BOARD_INFO(CYTTSP4_I2C_NAME, CYTTSP4_I2C_TCH_ADR),
	        .irq = MSM_GPIO_TO_INT( GPIO_TOUCH_IRQ ),
	},
};

#elif defined(CONFIG_TOUCHSCREEN_MELFAS_KYLE) 
static struct i2c_gpio_platform_data touch_i2c_gpio_data = {
	.sda_pin    = GPIO_TSP_SDA,
	.scl_pin    = GPIO_TSP_SCL,
	.udelay	= 1,
};
static struct platform_device touch_i2c_gpio_device = {
	.name       = "i2c-gpio",
	.id     =  5,
	.dev        = {
		.platform_data  = &touch_i2c_gpio_data,
	},
};
/* I2C 2 */
static struct i2c_board_info touch_i2c_devices[] = {
	{
		I2C_BOARD_INFO("sec_touch", 0x48),
	        .irq = MSM_GPIO_TO_INT( GPIO_TOUCH_IRQ ),
	},
};

#elif  defined(CONFIG_TOUCHSCREEN_ZINITIX_ZANIN) || defined(CONFIG_TOUCHSCREEN_ZINITIX_VASTO) || defined(CONFIG_TOUCHSCREEN_ZINITIX_INFINITE)
static struct i2c_gpio_platform_data touch_i2c_gpio_data = {
	.sda_pin    = GPIO_TSP_SDA,
	.scl_pin    = GPIO_TSP_SCL,
	.udelay	= 1,
};

static struct platform_device touch_i2c_gpio_device = {  
	.name       = "i2c-gpio",
	.id     =  5,
	.dev        = {
		.platform_data  = &touch_i2c_gpio_data,
	},
};


/* I2C 2 */
static struct i2c_board_info touch_i2c_devices[] = {
	{
		I2C_BOARD_INFO("zinitix_isp", 0x50),
	},
	{
		I2C_BOARD_INFO("sec_touchscreen", 0x20),
	        .irq = MSM_GPIO_TO_INT( GPIO_TOUCH_IRQ ),
	},
};
#endif

#ifdef CONFIG_TOUCHSCREEN_ZINITIX_VASTO
static void tsp_power_on(void)
{
	int rc = 0;
	printk("[TSP] %s start \n", __func__);
	rc = gpio_request(41, "touch_en");

	if (rc < 0) {
		pr_err("failed to request touch_en\n");
	}

	gpio_tlmm_config(GPIO_CFG(41, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_direction_output(41, 1);

	gpio_tlmm_config(GPIO_CFG(GPIO_TSP_SDA, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_PULL_UP, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(GPIO_TSP_SCL, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_PULL_UP, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(GPIO_MUS_SDA, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_PULL_UP, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(GPIO_MUS_SCL, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_PULL_UP, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	printk("[TSP] %s =======gpio_request==test======ln=%d\n",
			__func__, __LINE__);

}
#endif

#if defined(CONFIG_TOUCHSCREEN_ZINITIX_INFINITE)

extern void ts_power_control(int en);
static void tsp_power_on(void)
{
	ts_power_control(1);
}

#endif
//synaptics s2200 control IC
#if defined (CONFIG_TOUCHSCREEN_SYNAPTICS_S2200_I2C)
#if SYNA_BOARD_PRESENT(SYNA_TM2303)
	/* tm2303 has four buttons.
	 */

#define AXIS_ALIGNMENT { }

#define TM2303_ADDR 0x20
#define TM2303_ATTN 27
static unsigned char tm2303_f1a_button_codes[] = {KEY_MENU, KEY_HOMEPAGE, KEY_BACK};

#define TSP_SDA 132
#define TSP_SCL 131
#if defined(CONFIG_I2C_GPIO)
static struct i2c_gpio_platform_data touch_i2c_gpio_data = {
        .sda_pin    = TSP_SDA,
        .scl_pin    = TSP_SCL,
		//.sda_is_open_drain = true,
		//.scl_is_open_drain = true,
        .udelay  = 3,  //// brian :3
        .timeout = 100,
};

static struct platform_device touch_i2c_gpio_device = {
        .name       = "i2c-gpio",
        .id     = 5,
        .dev        = {
            .platform_data  = &touch_i2c_gpio_data,
        },
};
#endif
static int tm2303_post_suspend(void *pm_data) {
	pr_info("%s: RMI4 callback.\n", __func__);
	return s2200_ts_power(TOUCH_OFF);
}

static int tm2303_pre_resume(void *pm_data) {
	pr_info("%s: RMI4 callback.\n", __func__);
	return s2200_ts_power(TOUCH_ON);
}

static struct rmi_f1a_button_map tm2303_f1a_button_map = {
	.nbuttons = ARRAY_SIZE(tm2303_f1a_button_codes),
	.map = tm2303_f1a_button_codes,
};

static struct syna_gpio_data tm2303_gpiodata = {
	.gpio_number = TM2303_ATTN,
	.gpio_name = "sdmmc2_clk.gpio_127",
};

static struct rmi_device_platform_data tm2303_platformdata = {
	.driver_name = "rmi_generic",
	.attn_gpio = TM2303_ATTN,
	.attn_polarity = RMI_ATTN_ACTIVE_LOW,
	.reset_delay_ms = 250,
	.gpio_data = &tm2303_gpiodata,
	.gpio_config = synaptics_touchpad_gpio_setup,
	.axis_align = AXIS_ALIGNMENT,
	.f1a_button_map = &tm2303_f1a_button_map,
	.post_suspend = tm2303_post_suspend,
	.pre_resume = tm2303_pre_resume,
	.f11_type_b = true,	
};

static struct i2c_board_info __initdata synaptics_i2c_devices[] = {
	{
         I2C_BOARD_INFO("rmi_i2c", TM2303_ADDR),
        .platform_data = &tm2303_platformdata,
	},
};

#endif /* TM2303 */
#endif /* TOUCHSCREEN_SYNAPTICS_S2200_I2C */

/* Synaptics changes for Panda Board */
#ifdef CONFIG_TOUCHSCREEN_SYNAPTICS_DSX_I2C
#define TSP_SDA 132
#define TSP_SCL 131
#if defined(CONFIG_I2C_GPIO)
static struct i2c_gpio_platform_data touch_i2c_gpio_data = {
        .sda_pin    = TSP_SDA,
        .scl_pin    = TSP_SCL,
		//.sda_is_open_drain = true,
		//.scl_is_open_drain = true,
        .udelay  = 3,  //// brian :3
        .timeout = 100,
};

static struct platform_device touch_i2c_gpio_device = {
        .name       = "i2c-gpio",
        .id     = 5,
        .dev        = {
            .platform_data  = &touch_i2c_gpio_data,
        },
};
#endif

static int synaptics_gpio_setup(unsigned gpio, bool configure)
{
	int retval=0;
	if (configure)
	{
		retval = gpio_request(gpio, "rmi4_attn");
		if (retval) {
			pr_err("%s: Failed to get attn gpio %d. Code: %d.",
			       __func__, gpio, retval);
			return retval;
		}

		retval = gpio_direction_input(gpio);
		if (retval) {
			pr_err("%s: Failed to setup attn gpio %d. Code: %d.",
			       __func__, gpio, retval);
			gpio_free(gpio);
		}
	} else {
		pr_warn("%s: No way to deconfigure gpio %d.",
		       __func__, gpio);
	}
	gpio_tlmm_config(GPIO_CFG(27, 0, GPIO_CFG_INPUT,
				GPIO_CFG_PULL_UP, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(118, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	gpio_set_value(118,0);					// TSP sel  gpio , 0 : main tsp, 1 sub tsp
	gpio_tlmm_config(GPIO_CFG(132, 0, GPIO_CFG_INPUT,
				GPIO_CFG_PULL_UP, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(131, 0, GPIO_CFG_INPUT,
				GPIO_CFG_PULL_UP, GPIO_CFG_2MA),GPIO_CFG_ENABLE);

	return retval;
}


#define TM_SAMPLE3_ADDR	0x20
#define TM_SAMPLE3_ATTN	27

static unsigned char TM_SAMPLE3_f1a_button_codes[] = {KEY_MENU, KEY_HOMEPAGE, KEY_BACK};

static struct synaptics_dsx_cap_button_map TM_SAMPLE3_cap_button_map = {
	.nbuttons = ARRAY_SIZE(TM_SAMPLE3_f1a_button_codes),
	.map = TM_SAMPLE3_f1a_button_codes,
};

static struct synaptics_dsx_platform_data dsx_platformdata = {
	.irq_flags = IRQF_TRIGGER_FALLING,
	.irq_gpio = TM_SAMPLE3_ATTN,
	.regulator_en = true,
	.gpio_config = synaptics_gpio_setup,
	.cap_button_map = &TM_SAMPLE3_cap_button_map,
};

static struct i2c_board_info synaptics_i2c_devices[] = {
	{
		I2C_BOARD_INFO("synaptics_dsx_i2c", TM_SAMPLE3_ADDR),
		.platform_data = &dsx_platformdata,
	},
};

#endif
#if defined(CONFIG_INPUT_FLIP)
struct sec_flip_pdata {
	int wakeup;
};

static struct sec_flip_pdata sec_flip_dev_data = {
	.wakeup			= 1,
};

static struct platform_device sec_flip_device = {
	.name = "sec_flip",
	.id = -1,
	.dev = {
		.platform_data = &sec_flip_dev_data,
	}
};
#endif

#ifdef CONFIG_KEYBOARD_GPIO
static struct gpio_keys_button gpio_keys_button[] = {
	{
		.code			= KEY_HOME, //KEY_HOMEPAGE,
		.type			= EV_KEY,
		.gpio			= 37,
		.active_low		= 1,
		.wakeup			= 1,
		.debounce_interval	= 5, /* ms */
		.desc			= "Home",
	},
};
static struct gpio_keys_platform_data gpio_keys_platform_data = {
	.buttons	= gpio_keys_button,
	.nbuttons	= ARRAY_SIZE(gpio_keys_button),
	.rep		= 0,
};

static struct platform_device msm7x27a_gpio_keys_device = {
	.name	= "gpio-keys",
	.id	= -1,
	.dev	= {
		.platform_data	= &gpio_keys_platform_data,
	}
};
#endif


#define CONFIG_GPIOKEY_KYLEP	// jjLee

#ifdef CONFIG_GPIOKEY_KYLEP
#if defined(CONFIG_MACH_DELOS_DUOS_CTC) || defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
#define GPIO_MUSB_INT_ARUBA 28
#define GPIO_MUS_SCL_ARUBA 121
#define GPIO_MUS_SDA_ARUBA 120
#else
#define GPIO_MUSB_INT_ARUBA 28
#define GPIO_MUS_SCL_ARUBA 123
#define GPIO_MUS_SDA_ARUBA 122
#endif

#define KP_INDEX(row, col) ((row)*ARRAY_SIZE(kp_col_gpios) + (col))

#ifdef CONFIG_KEYBOARD_WAKEUP
#if defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
static unsigned int kp_row_gpios[] = {17,80,112}; 
static unsigned int kp_col_gpios[] = {72}; 
static unsigned int kp_wakeup_gpios[] = {112};
#else
// delos ctc
static unsigned int kp_row_gpios[] = {35,31}; 
static unsigned int kp_col_gpios[] = {40}; 
static unsigned int kp_wakeup_gpios[] = {37};
#endif
static const unsigned short keymap[ARRAY_SIZE(kp_col_gpios) *  ARRAY_SIZE(kp_row_gpios)] = {
	[KP_INDEX(0, 0)] = KEY_VOLUMEDOWN,
	[KP_INDEX(1, 0)] = KEY_VOLUMEUP,
#if defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
	[KP_INDEX(2, 0)] = KEY_HOME,
#endif
};
//	[KP_INDEX(0, 1)] = KEY_HOME,
#else
static unsigned int kp_row_gpios[] = {36,31}; 
static unsigned int kp_col_gpios[] = {39,37}; 
static const unsigned short keymap[ARRAY_SIZE(kp_col_gpios) *  ARRAY_SIZE(kp_row_gpios)] = {
	[KP_INDEX(0, 0)] = KEY_VOLUMEDOWN,
	[KP_INDEX(0, 1)] = KEY_HOME,
	[KP_INDEX(1, 0)] = KEY_VOLUMEUP,
};
#endif
/* SURF keypad platform device information */
static struct gpio_event_matrix_info kp_matrix_info = {
	.info.func	= gpio_event_matrix_func,
	.keymap		= keymap,
#if defined(CONFIG_MACH_HENNESSY_DUOS_CTC)	
	.output_gpios	= NULL,
	.input_gpios	= NULL,
#ifdef CONFIG_KEYBOARD_WAKEUP	
	.wakeup_gpios	= NULL,
	.nwakeups	= 0,
#endif
	.noutputs	= 0,
	.ninputs	= 0,
#else //CONFIG_MACH_HENNESSY_DUOS_CTC
	.output_gpios	= kp_row_gpios,
	.input_gpios	= kp_col_gpios,
#ifdef CONFIG_KEYBOARD_WAKEUP	
	.wakeup_gpios	= kp_wakeup_gpios,
	.nwakeups	= ARRAY_SIZE(kp_wakeup_gpios),
#endif
	.noutputs	= ARRAY_SIZE(kp_row_gpios),
	.ninputs	= ARRAY_SIZE(kp_col_gpios),
#endif //CONFIG_MACH_HENNESSY_DUOS_CTC
//	.settle_time.tv_nsec = 40 * NSEC_PER_USEC, //ARUBA_JB
//	.poll_time.tv_nsec = 20 * NSEC_PER_MSEC, //ARUBA_JB
	//.debounce_delay.tv_nsec = 20 * NSEC_PER_MSEC,
	.flags = GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_DRIVE_INACTIVE | GPIOKPF_PRINT_UNMAPPED_KEYS | GPIOKPF_DEBOUNCE,
};

static struct gpio_event_info *kp_info[] = {
	&kp_matrix_info.info
};

static struct gpio_event_platform_data kp_pdata = {
	.name		= "7x27a_kp",
	.info		= kp_info,
	.info_count	= ARRAY_SIZE(kp_info)
};

static struct platform_device kp_pdev = {
	.name	= GPIO_EVENT_DEV_NAME,
	.id	= -1,
	.dev	= {
		.platform_data	= &kp_pdata,
	},
};


static struct msm_handset_platform_data hs_platform_data = {
	.hs_name = "7k_handset",
	.pwr_key_delay_ms = 500, /* 0 will disable end key */
};

static struct platform_device hs_pdev = {
	.name   = "msm-handset",
	.id     = -1,
	.dev    = {
		.platform_data = &hs_platform_data,
	},
};

#endif   // CONFIG_GPIOKEY_KYLEP


struct msm_battery_callback *charger_callbacks;
static enum cable_type_t set_cable_status;
static enum acc_type_t set_acc_status;
static enum ovp_type_t set_ovp_status;

#if defined(CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE)
static int checkChargerType()
{
	return set_cable_status;
}
#endif

static void msm_battery_register_callback(
		struct msm_battery_callback *ptr)
{
	charger_callbacks = ptr;
	pr_info("[BATT] msm_battery_register_callback start\n");
	if ((set_acc_status != 0) && charger_callbacks
		&& charger_callbacks->set_acc_type)
		charger_callbacks->set_acc_type(charger_callbacks,
		set_acc_status);

	if ((set_cable_status != 0) && charger_callbacks
		&& charger_callbacks->set_cable)
		charger_callbacks->set_cable(charger_callbacks,
		set_cable_status);

	if ((set_ovp_status != 0) && charger_callbacks
		&& charger_callbacks->set_ovp_type)
		charger_callbacks->set_ovp_type(charger_callbacks,
		set_ovp_status);
}

#ifdef CONFIG_CHARGER_FAN54013
static void fan54013_tx_charge_done(void)
{
	if (charger_callbacks && charger_callbacks->charge_done)
		charger_callbacks->charge_done(charger_callbacks);
}

static void fan54013_tx_charge_fault(enum fan54013_fault_t reason)
{
	if (charger_callbacks && charger_callbacks->charge_fault)
		charger_callbacks->charge_fault(charger_callbacks, reason);
}

static struct fan54013_platform_data fan54013_pdata = {
	/* Rx functions from host will be initialized at "fan54013_probe" */
	.start_charging = NULL,
	.stop_charging = NULL,
	.get_vbus_status = NULL,
	.set_host_status = NULL,
	.get_monitor_bits = NULL,
	.get_fault = NULL,
	.get_ic_info = NULL,

	/* Tx functions to host should be initialized now! */
	.tx_charge_done = fan54013_tx_charge_done,
	.tx_charge_fault = fan54013_tx_charge_fault,
	
#if defined(CONFIG_MACH_DELOS_DUOS_CTC) || defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
	.VRsense = FAN54013_VRSENSE_100m,
	.Isafe = FAN54013_ISAFE_986,
	.chg_curr_ta = FAN54013_CURR_986,
	.chg_curr_usb = FAN54013_CURR_550,
	.Iterm = FAN54013_ITERM_99,
	.Vsafe = FAN54013_VSAFE_434,
	.Voreg = FAN54013_OREG_434,
#else
	.VRsense = FAN54013_VRSENSE_68m, /* 68mohm or 100mohm */
	.Isafe = FAN54013_ISAFE_1050, /* Maximum IOCHARGE */
	.chg_curr_ta = FAN54013_CURR_850,
	.chg_curr_usb = FAN54013_CURR_550,
	.Iterm = FAN54013_ITERM_97,
	.Vsafe = FAN54013_VSAFE_434,	/* Maximum OREG */
	.Voreg = FAN54013_OREG_434,
#endif
};
#endif /* CONFIG_CHARGER_FAN54013 */

#ifdef CONFIG_CHARGER_SMB328A
static void smb328a_tx_charge_done(void)
{
        if (charger_callbacks && charger_callbacks->charge_done)
                charger_callbacks->charge_done(charger_callbacks);
}
#ifdef CONFIG_MACH_DELOS_CTC
static void smb328a_bat_missing_detection(void)
{
        if (charger_callbacks && charger_callbacks->bat_missing)
                charger_callbacks->bat_missing(charger_callbacks);
}
#endif

static struct smb328a_platform_data smb328a_pdata = {
        .start_charging = NULL,
        .stop_charging = NULL,
        .get_vbus_status = NULL,
        .tx_charge_done = smb328a_tx_charge_done,
        .chg_curr_ta = SMB328A_CURR_1000,
	  .chg_curr_usb = SMB328A_CURR_500,
 #ifdef CONFIG_MACH_DELOS_CTC
        .bat_missing    = smb328a_bat_missing_detection,
 #endif
	
};
#endif /* CONFIG_CHARGER_SMB328A */

#ifdef CONFIG_CHARGER_BQ24157
static void bq24157_tx_charge_done(void)
{
	if (charger_callbacks && charger_callbacks->charge_done)
		charger_callbacks->charge_done(charger_callbacks);
}

static void bq24157_tx_charge_fault(enum bq24157_fault_t reason)
{
	if (charger_callbacks && charger_callbacks->charge_fault)
		charger_callbacks->charge_fault(charger_callbacks, reason);
}

static struct bq24157_platform_data bq24157_pdata = {
	/* Rx functions from host will be initialized at "bq24157_probe" */
	.start_charging = NULL,
	.stop_charging = NULL,
	.get_fault = NULL,
	.get_status = NULL,

	/* Tx functions to host should be initialized now! */
	.tx_charge_done = bq24157_tx_charge_done,
	.tx_charge_fault = bq24157_tx_charge_fault,
#if defined(CONFIG_MACH_DELOS_OPEN)
	.Rsense = BQ24157_VRSENSE_68m,
	.VMchrg = BQ24157_68m_VMCHG_1250,
	.chg_curr_ta = BQ24157_68m_CURR_1250,
	.chg_curr_usb = BQ24157_68m_CURR_550,
	.Iterm = BQ24157_68m_ITERM_250,
#ifdef CONFIG_MSM_BACKGROUND_CHARGING_EX_CHARGER
	.Iterm_2nd = BQ24157_68m_ITERM_150,
#endif
	.VMreg = BQ24157_VMREG_4340,
	.VOreg = BQ24157_VOREG_4340,
#else
	.Rsense = BQ24157_VRSENSE_68m,
	.VMchrg = BQ24157_68m_VMCHG_950,
	.chg_curr_ta = BQ24157_68m_CURR_950,
	.chg_curr_usb = BQ24157_68m_CURR_550,
	.Iterm = BQ24157_68m_ITERM_150,
	.VMreg = BQ24157_VMREG_4340,
	.VOreg = BQ24157_VOREG_4340,
#endif
};
#endif /* CONFIG_CHARGER_BQ24157 */
static u32 msm_calculate_batt_capacity(u32 current_voltage);

static struct msm_charger_data aries_charger = {
	.register_callbacks	= msm_battery_register_callback,
#ifdef CONFIG_MACH_DELOS_CTC
	.charger_ic = &fan54013_pdata,
	.charger_ic2 = &smb328a_pdata,
#else  /* else of CONFIG_MACH_DELOS_CTC */

#ifdef CONFIG_CHARGER_FAN54013
	.charger_ic = &fan54013_pdata,
#endif
#ifdef CONFIG_CHARGER_SMB328A
	.charger_ic = &smb328a_pdata,
#endif
#ifdef CONFIG_CHARGER_BQ24157
	.charger_ic = &bq24157_pdata,
#endif

#endif  /* end of CONFIG_MACH_DELOS_CTC */
};

static struct msm_psy_batt_pdata msm_psy_batt_data = {
	.charger			= &aries_charger,
	.voltage_min_design     = 3200,
	.voltage_max_design     = 4200,
	.voltage_fail_safe      = 3340,
	.avail_chg_sources      = POWER_SUPPLY_TYPE_MAINS | POWER_SUPPLY_TYPE_USB ,
	.batt_technology        = POWER_SUPPLY_TECHNOLOGY_LION,
	.calculate_capacity     = &msm_calculate_batt_capacity,
};

static struct platform_device msm_batt_device = {
	.name		= "msm-battery",
	.id		= -1,
	.dev.platform_data	= &msm_psy_batt_data,
};

static u32 msm_calculate_batt_capacity(u32 current_voltage)
{
	u32 low_voltage	 = msm_psy_batt_data.voltage_min_design;
	u32 high_voltage = msm_psy_batt_data.voltage_max_design;

	if (current_voltage <= low_voltage)
		return 0;
	else if (current_voltage >= high_voltage)
		return 100;
	else
		return (current_voltage - low_voltage) * 100
			/ (high_voltage - low_voltage);
}


int fsa_cable_type = CABLE_TYPE_UNKNOWN;
int fsa9480_get_charger_status(void);
int fsa9480_get_charger_status(void)
{
	return fsa_cable_type;
}

int fsa_ovp_state = OVP_TYPE_NONE;
int fsa9480_get_ovp_status(void);
int fsa9480_get_ovp_status(void)
{
	return fsa_ovp_state;
}

static void jena_usb_power(int onoff, char *path)
{
#if 0
	struct vreg *usb_vreg = vreg_get("");

	if (IS_ERR(regulator))
		return;

	if (onoff) {
		if (!regulator_use_count(regulator))
			regulator_enable(regulator);
	} else {
		if (regulator_use_count(regulator))
			regulator_force_disable(regulator);
	}
#endif
}

void trebon_chg_connected(enum chg_type chgtype)
{
	char *chg_types[] = {"STD DOWNSTREAM PORT",
			"CARKIT",
			"DEDICATED CHARGER",
			"INVALID"};
	unsigned int data1 = 0;	
	unsigned int data2 = 0;	
	int ret = 0;

	switch (chgtype) {
	case USB_CHG_TYPE__SDP:
		ret = msm_proc_comm(PCOM_CHG_USB_IS_PC_CONNECTED,
				data1, data2);
		break;
	case USB_CHG_TYPE__WALLCHARGER:
		ret = msm_proc_comm(PCOM_CHG_USB_IS_CHARGER_CONNECTED,
				data1, data2);
		break;
	case USB_CHG_TYPE__INVALID:
		ret = msm_proc_comm(PCOM_CHG_USB_IS_DISCONNECTED,
				data1, data2);
		break;
	default:
		break;
	}

	if (ret < 0)
		pr_err("%s: connection err, ret=%d\n", __func__, ret);

	pr_info("\nCharger Type: %s\n", chg_types[chgtype]);
}

static void jena_usb_cb(u8 attached, struct fsausb_ops *ops)
{
	pr_info("[BATT] [%s] Board file [fsa9480]: USB Callback\n", __func__);

	set_acc_status = attached ? ACC_TYPE_USB : ACC_TYPE_NONE;
	if (charger_callbacks && charger_callbacks->set_acc_type)
		charger_callbacks->set_acc_type(charger_callbacks,
		set_acc_status);

	set_cable_status = attached ? CABLE_TYPE_USB : CABLE_TYPE_UNKNOWN;
	if (charger_callbacks && charger_callbacks->set_cable)
		charger_callbacks->set_cable(charger_callbacks,
		set_cable_status);
}

//extern  void charger_enable(int enable);
static void jena_charger_cb(u8 attached, struct fsausb_ops *ops)
{
	pr_info("[BATT] Board file [fsa9480]: Charger Callback\n");

	set_acc_status = attached ? ACC_TYPE_CHARGER : ACC_TYPE_NONE;
	if (charger_callbacks && charger_callbacks->set_acc_type)
		charger_callbacks->set_acc_type(charger_callbacks,
		set_acc_status);

	set_cable_status = attached ? CABLE_TYPE_TA : CABLE_TYPE_UNKNOWN;
	if (charger_callbacks && charger_callbacks->set_cable)
		charger_callbacks->set_cable(charger_callbacks,
		set_cable_status);

	//charger_enable(set_cable_status);
}

static void jena_jig_cb(u8 attached, struct fsa9480_ops *ops)
{
	printk("\nBoard file [fsa9480]: Jig Callback \n");
}

static void jena_ovp_cb(u8 attached, struct fsa9480_ops *ops)
{
	pr_info("[BATT] Board file [fsa9480]: OVP Callback type:%d\n",attached);

	set_ovp_status = attached ? OVP_TYPE_OVP : OVP_TYPE_NONE;
	if (charger_callbacks && charger_callbacks->set_ovp_type)
		charger_callbacks->set_ovp_type(charger_callbacks,
		set_ovp_status);
}

static void jena_fsa9480_reset_cb(void)
{
	printk("\nBoard file [fsa9480]: Reset Callback \n");
}

/* For uUSB Switch */
static struct fsa9480_platform_data jena_fsa9480_pdata = {
       .usb_cb         = jena_usb_cb,
       .uart_cb        = NULL,
       .charger_cb     = jena_charger_cb,
       .jig_cb         = NULL, //jena_jig_cb, 
       .ovp_cb		= jena_ovp_cb,
};

static struct i2c_gpio_platform_data fsa9480_i2c_gpio_data = {
	.sda_pin    = GPIO_MUS_SDA_ARUBA,
	.scl_pin    = GPIO_MUS_SCL_ARUBA,
	.udelay = 1,
};

static struct platform_device fsa9480_i2c_gpio_device = {  
	.name       = "i2c-gpio",
	.id     =  3,
	.dev        = {
		.platform_data  = &fsa9480_i2c_gpio_data,
	},
};

static struct i2c_board_info fsa9480_i2c_devices[] = {
	{
		I2C_BOARD_INFO("FSA9480", 0x4A >> 1),
		.platform_data =  &jena_fsa9480_pdata,
		.irq = MSM_GPIO_TO_INT(GPIO_MUSB_INT_ARUBA),
	},
};

static void __init fsa9480_gpio_init(void)
{
	gpio_tlmm_config(GPIO_CFG(GPIO_MUSB_INT_ARUBA,  0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
//	gpio_tlmm_config(GPIO_CFG(GPIO_MUS_SDA_ARUBA,  0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),GPIO_CFG_DISABLE);
//	gpio_tlmm_config(GPIO_CFG(GPIO_MUS_SCL_ARUBA,  0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),GPIO_CFG_DISABLE);

//	mdelay(100); 
	gpio_tlmm_config(GPIO_CFG(GPIO_MUS_SDA_ARUBA,  0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(GPIO_MUS_SCL_ARUBA,  0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE);

//	gpio_tlmm_config(GPIO_CFG(GPIO_MUS_SDA_ARUBA,  0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),0);
//	gpio_tlmm_config(GPIO_CFG(GPIO_MUS_SCL_ARUBA,  0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),0);

}

#ifdef CONFIG_SEC_DUAL_MODEM
void fsa9480_i2c_init(void)
{
	gpio_tlmm_config(GPIO_CFG(GPIO_MUS_SDA_ARUBA,  0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(GPIO_MUS_SCL_ARUBA,  0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),GPIO_CFG_ENABLE);

	gpio_direction_output(GPIO_MUS_SDA_ARUBA, 1);
	gpio_direction_output(GPIO_MUS_SCL_ARUBA, 1);	
}
EXPORT_SYMBOL(fsa9480_i2c_init);
#endif

#ifdef CONFIG_CHARGER_BQ24157
#define MSM_CHARGER_I2C_BUS_ID	7
#if defined(CONFIG_MACH_DELOS_OPEN)
#define GPIO_CHARGER_SCL 	9
#define GPIO_CHARGER_SDA 	10
#define GPIO_CHARGER_STAT	112
#define GPIO_CHARGER_CD		113
#else
#define GPIO_CHARGER_SCL 	9
#define GPIO_CHARGER_SDA 	10
#define GPIO_CHARGER_STAT	112
#define GPIO_CHARGER_CD		113
#endif

static struct i2c_gpio_platform_data charger_i2c_gpio_data = {
	.sda_pin		= GPIO_CHARGER_SDA,
	.scl_pin		= GPIO_CHARGER_SCL,
};

static struct platform_device charger_i2c_gpio_device = {
	.name			= "i2c-gpio",
	.id			= MSM_CHARGER_I2C_BUS_ID,
	.dev	= {
	.platform_data	= &charger_i2c_gpio_data,
	},
};

static struct i2c_board_info charger_i2c_devices[] __initdata = {
	{
		I2C_BOARD_INFO( "bq24157", 0x6A ),
		.platform_data	= &bq24157_pdata,
		.irq = MSM_GPIO_TO_INT(GPIO_CHARGER_STAT),
	},
};

static void bq24157_gpio_init(void)
{
	gpio_tlmm_config(GPIO_CFG(GPIO_CHARGER_SDA, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(GPIO_CHARGER_SCL, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(GPIO_CHARGER_STAT,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(GPIO_CHARGER_CD, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_set_value(GPIO_CHARGER_CD, 0);
}
#endif /* CONFIG_CHARGER_BQ24157 */

#ifdef CONFIG_BQ27425_FUEL_GAUGE

#define FUEL_I2C_SCL 10
#define FUEL_I2C_SDA 9

/* Fuel_gauge */
static struct i2c_gpio_platform_data fuelgauge_i2c_gpio_data = {
	.sda_pin = FUEL_I2C_SDA,
	.scl_pin = FUEL_I2C_SCL,
};

static struct platform_device fuelgauge_i2c_gpio_device = {
	.name	= "i2c-gpio",
	.id	= 6,
	.dev	= {
	.platform_data	= &fuelgauge_i2c_gpio_data,
	},
};
static struct i2c_board_info fg_i2c_devices[] = {
	{
		I2C_BOARD_INFO( "bq27425", 0xAA>>1 ),
	},
};
#endif

#ifdef CONFIG_MAX17048_FUELGAUGE

#define FUEL_I2C_SCL 10
#define FUEL_I2C_SDA 9

static int max17048_low_batt_cb(void)
{
	pr_err("%s: Low battery alert\n", __func__);
	struct power_supply *psy = power_supply_get_by_name("battery");
	union power_supply_propval value;
	if (!psy) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return -ENODEV;
	}
	value.intval = POWER_SUPPLY_CAPACITY_LEVEL_CRITICAL;
	return psy->set_property(psy, POWER_SUPPLY_PROP_CAPACITY_LEVEL, &value);
}
int check_battery_type(void)
{
	return BATT_TYPE_NORMAL;
}
static int max17048_power_supply_register(struct device *parent,
	struct power_supply *psy)
{
	aries_charger.psy_fuelgauge = psy;
	return 0;
}

static void max17048_power_supply_unregister(struct power_supply *psy)
{
	aries_charger.psy_fuelgauge = NULL;
}

/* Fuel_gauge */
static struct i2c_gpio_platform_data fg_smb_i2c_gpio_data = {
	.sda_pin = FUEL_I2C_SDA,
	.scl_pin = FUEL_I2C_SCL,
	.udelay	= 1,
};

static struct platform_device fg_smb_i2c_gpio_device = {
	.name	= "i2c-gpio",
	.id	= 6,
	.dev	= {
	.platform_data	= &fg_smb_i2c_gpio_data,
	},
};

static struct max17048_platform_data max17048_pdata = {
	.low_batt_cb = NULL, /* max17048_low_batt_cb, */
	.check_batt_type = check_battery_type,
	.power_supply_register = max17048_power_supply_register,
	.power_supply_unregister = max17048_power_supply_unregister,
	.rcomp_value = 0x501c,
};

static struct i2c_board_info fg_smb_i2c_devices[] = {
    {
		I2C_BOARD_INFO("max17048", 0x6D>>1),		
		.platform_data = &max17048_pdata,
		.irq = NULL, /* MSM_GPIO_TO_INT(GPIO_FUEL_INT), */
    },
};

static void __init fg_max17048_gpio_init(void)
{
	gpio_tlmm_config(GPIO_CFG(FUEL_I2C_SDA, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_PULL_UP, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(FUEL_I2C_SCL, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_PULL_UP, GPIO_CFG_2MA),GPIO_CFG_ENABLE);	
}
#endif

#ifdef CONFIG_CHARGER_FAN54013
#define MSM_CHARGER_I2C_BUS_ID	7
#if defined(CONFIG_MACH_DELOS_CTC) || defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
#define GPIO_CHARGER_SCL 	116
#define GPIO_CHARGER_SDA 	125
#define GPIO_CHARGER_STAT	117
#endif

static struct i2c_gpio_platform_data fan_charger_i2c_gpio_data = {
	.sda_pin		= GPIO_CHARGER_SDA,
	.scl_pin		= GPIO_CHARGER_SCL,
	.udelay 		= 2, /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device fan_charger_i2c_gpio_device = {
	.name			= "i2c-gpio",
	.id			= MSM_CHARGER_I2C_BUS_ID,
	.dev	= {
	.platform_data	= &fan_charger_i2c_gpio_data,
	},
};

static struct i2c_board_info fan_charger_i2c_devices[] __initdata = {
	{
		I2C_BOARD_INFO( "fan54013", 0x6B ),
		.platform_data	= &fan54013_pdata,
		.irq = MSM_GPIO_TO_INT(GPIO_CHARGER_STAT),
	},
};

static void fan54013_gpio_init(void)
{	
	gpio_tlmm_config(GPIO_CFG(GPIO_CHARGER_SDA, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(GPIO_CHARGER_SCL, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);	
	gpio_tlmm_config(GPIO_CFG(GPIO_CHARGER_STAT,  0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);	
}
#endif /* CONFIG_CHARGER_FAN54013 */

#ifdef CONFIG_CHARGER_SMB328A
#define MSM_CHARGER_I2C_BUS_ID	7
#if defined(CONFIG_MACH_DELOS_CTC)
#define GPIO_CHARGER_SCL 	116
#define GPIO_CHARGER_SDA 	125
#define GPIO_CHARGER_STAT	117
#endif

static struct i2c_gpio_platform_data smb_charger_i2c_gpio_data = {
	.sda_pin		= GPIO_CHARGER_SDA,
	.scl_pin		= GPIO_CHARGER_SCL,
	.udelay 		= 2, /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device smb_charger_i2c_gpio_device = {
	.name			= "i2c-gpio",
	.id			= MSM_CHARGER_I2C_BUS_ID,
	.dev	= {
	.platform_data	= &smb_charger_i2c_gpio_data,
	},
};

static struct i2c_board_info smb_charger_i2c_devices[] __initdata = {
	{
		I2C_BOARD_INFO("smb328a", (0x69 >> 1)),
		.platform_data	= &smb328a_pdata,
		.irq = MSM_GPIO_TO_INT(GPIO_CHARGER_STAT),
	},
};

static void smb328a_gpio_init(void)
{	
	gpio_tlmm_config(GPIO_CFG(GPIO_CHARGER_SDA, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(GPIO_CHARGER_SCL, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);	
	gpio_tlmm_config(GPIO_CFG(GPIO_CHARGER_STAT,  0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);	
}
#endif /* CONFIG_CHARGER_SMB328A */
/////////////////////////////////////////////////////////////////////////////////

#define PMEM_KERNEL_EBI1_SIZE	0x3A000
#define MSM_PMEM_AUDIO_SIZE	0xF0000
#define BOOTLOADER_BASE_ADDR    0x10000
#define BAHAMA_SLAVE_ID_FM_REG 0x02
#define FM_GPIO	83
#define BT_PCM_BCLK_MODE  0x88
#define BT_PCM_DIN_MODE   0x89
#define BT_PCM_DOUT_MODE  0x8A
#define BT_PCM_SYNC_MODE  0x8B
#define FM_I2S_SD_MODE    0x8E
#define FM_I2S_WS_MODE    0x8F
#define FM_I2S_SCK_MODE   0x90
#define I2C_PIN_CTL       0x15
#define I2C_NORMAL        0x40

#ifndef ATH_POLLING
void (*wlan_status_notify_cb)(int card_present, void *dev_id);
void *wlan_devid;

int register_wlan_status_notify(void (*callback)(int card_present, void *dev_id), void *dev_id)
{
	printk("%s --enter\n", __func__);

	wlan_status_notify_cb = callback;
	wlan_devid = dev_id;
	return 0;
}

unsigned int wlan_status(struct device *dev)
{
	int rc;

	printk("%s entered\n", __func__);

	rc = gpio_get_value(GPIO_WLAN_RESET_N/*gpio_wlan_reset_n*/);

	return rc;
}
#endif /* ATH_POLLING */

static struct platform_device msm_wlan_ar6000_pm_device = {
	.name           = "wlan_ar6000_pm_dev",
	.id             = -1,
};

static struct msm_serial_platform_data msm_8625_uart1_pdata = {
	.userid		= 10,
};

static struct msm_gpio qup_i2c_gpios_io[] = {
	{ GPIO_CFG(60, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
		"qup_scl" },
	{ GPIO_CFG(61, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
		"qup_sda" },
};

static struct msm_gpio qup_i2c_gpios_hw[] = {
	{ GPIO_CFG(60, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
		"qup_scl" },
	{ GPIO_CFG(61, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
		"qup_sda" },
};

static void gsbi_qup_i2c_gpio_config(int adap_id, int config_type)
{
	int rc;

	if (adap_id < 0 || adap_id > 1)
		return;

	/* Each adapter gets 2 lines from the table */
	if (config_type)
		rc = msm_gpios_request_enable(&qup_i2c_gpios_hw[adap_id*2], 2);
	else
		rc = msm_gpios_request_enable(&qup_i2c_gpios_io[adap_id*2], 2);
	if (rc < 0)
		pr_err("QUP GPIO request/enable failed: %d\n", rc);
}

static struct msm_i2c_platform_data msm_gsbi0_qup_i2c_pdata = {
	.clk_freq		= 100000,
	.msm_i2c_config_gpio	= gsbi_qup_i2c_gpio_config,
};

static struct msm_i2c_platform_data msm_gsbi1_qup_i2c_pdata = {
	.clk_freq		= 100000,
	.msm_i2c_config_gpio	= gsbi_qup_i2c_gpio_config,
};

static struct msm_gpio msm8625q_i2c_gpio_config[] = {
#if defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
	{ GPIO_CFG(36, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
		"qup_scl" },
	{ GPIO_CFG(39, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
		"qup_sda" },
#else
	{ GPIO_CFG(39, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
		"qup_scl" },
	{ GPIO_CFG(36, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
		"qup_sda" },
#endif
};

static struct i2c_gpio_platform_data msm8625q_i2c_gpio_pdata = {
#if defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
	.scl_pin = 36,
	.sda_pin = 39,
#else
	.scl_pin = 39,
	.sda_pin = 36,
#endif
	.udelay = 5, /* 100 Khz */
};

static struct platform_device msm8625q_i2c_gpio = {
	.name	= "i2c-gpio",
	.id	= 2,
	.dev	= {
		.platform_data = &msm8625q_i2c_gpio_pdata,
	}
};

#ifdef CONFIG_ARCH_MSM7X27A
#define MSM_PMEM_MDP_SIZE       0x2300000
#define MSM_PMEM_ADSP_SIZE      0x1200000
#define CAMERA_ZSL_SIZE		(SZ_1M * 60)

#define MSM_ION_AUDIO_SIZE	(MSM_PMEM_AUDIO_SIZE + PMEM_KERNEL_EBI1_SIZE)
#define MSM_ION_CAMERA_SIZE	MSM_PMEM_ADSP_SIZE
#define MSM_ION_SF_SIZE		MSM_PMEM_MDP_SIZE
#define MSM_ION_HEAP_NUM	5
#ifdef CONFIG_ION_MSM
#define MSM_ION_HEAP_NUM	5
static struct platform_device ion_dev;
static int msm_ion_camera_size;
static int msm_ion_audio_size;
static int msm_ion_sf_size;
static int msm_ion_camera_size_carving;
#endif
#endif

#define CAMERA_HEAP_BASE        0x0
#ifdef CONFIG_CMA
#define CAMERA_HEAP_TYPE	ION_HEAP_TYPE_DMA
#else
#define CAMERA_HEAP_TYPE	ION_HEAP_TYPE_CARVEOUT
#endif

static struct android_usb_platform_data android_usb_pdata = {
	.update_pid_and_serial_num = usb_diag_update_pid_and_serial_num,
	.cdrom = 1,
};

static struct platform_device android_usb_device = {
	.name	= "android_usb",
	.id	= -1,
	.dev	= {
		.platform_data = &android_usb_pdata,
	},
};

#define SAMSUNG_LPM
static int __init boot_mode_boot(char *onoff)
{
#ifdef SAMSUNG_LPM
	if (strncmp(onoff, "true", 4) == 0) {
#else
	if (strncmp(onoff, "charger", 7) == 0) {
#endif
		charging_boot = 1;
		fota_boot = 0;
		pr_info("%s[BATT]charging_boot: %d\n",
			__func__, charging_boot);
	} else if (strcmp(onoff, "fota") == 0) {
		fota_boot = 1;
		charging_boot = 0;
	} else {
		charging_boot = 0;
		fota_boot = 0;
	}
	return 1;
}
#ifdef SAMSUNG_LPM
__setup("androidboot.bootchg=", boot_mode_boot);
#else
__setup("androidboot.mode=", boot_mode_boot);
#endif

#ifdef CONFIG_USB_EHCI_MSM_72K
static void msm_hsusb_vbus_power(unsigned phy_info, int on)
{
	int rc = 0;
	unsigned gpio;

	gpio = QRD_GPIO_HOST_VBUS_EN;

	rc = gpio_request(gpio,	"i2c_host_vbus_en");
	if (rc < 0) {
		pr_err("failed to request %d GPIO\n", gpio);
		return;
	}
	gpio_direction_output(gpio, !!on);
	gpio_set_value_cansleep(gpio, !!on);
	gpio_free(gpio);
}

static struct msm_usb_host_platform_data msm_usb_host_pdata = {
	.phy_info       = (USB_PHY_INTEGRATED | USB_PHY_MODEL_45NM),
};

static void __init msm7627a_init_host(void)
{
	msm_add_host(0, &msm_usb_host_pdata);
}
#endif

#ifdef CONFIG_USB_MSM_OTG_72K
static int hsusb_rpc_connect(int connect)
{
	if (connect)
		return msm_hsusb_rpc_connect();
	else
		return msm_hsusb_rpc_close();
}

static struct regulator *reg_hsusb;
static int msm_hsusb_ldo_init(int init)
{
	int rc = 0;

	if (init) {
		reg_hsusb = regulator_get(NULL, "usb");
		if (IS_ERR(reg_hsusb)) {
			rc = PTR_ERR(reg_hsusb);
			pr_err("%s: could not get regulator: %d\n",
					__func__, rc);
			goto out;
		}

		rc = regulator_set_voltage(reg_hsusb, 3300000, 3300000);
		if (rc) {
			pr_err("%s: could not set voltage: %d\n",
					__func__, rc);
			goto reg_free;
		}

		return 0;
	}
	/* else fall through */
reg_free:
	regulator_put(reg_hsusb);
out:
	reg_hsusb = NULL;
	return rc;
}

static int msm_hsusb_ldo_enable(int enable)
{
	static int ldo_status;

	if (IS_ERR_OR_NULL(reg_hsusb))
		return reg_hsusb ? PTR_ERR(reg_hsusb) : -ENODEV;

	if (ldo_status == enable)
		return 0;

	ldo_status = enable;

	return enable ?
		regulator_enable(reg_hsusb) :
		regulator_disable(reg_hsusb);
}

#ifndef CONFIG_USB_EHCI_MSM_72K
static int msm_hsusb_pmic_notif_init(void (*callback)(int online), int init)
{
	int ret = 0;

	if (init)
		ret = msm_pm_app_rpc_init(callback);
	else
		msm_pm_app_rpc_deinit(callback);

	return ret;
}
#endif

static struct msm_otg_platform_data msm_otg_pdata = {
#ifndef CONFIG_USB_EHCI_MSM_72K
	.pmic_vbus_notif_init	 = msm_hsusb_pmic_notif_init,
#else
	.vbus_power		 = msm_hsusb_vbus_power,
#endif
	.rpc_connect		 = hsusb_rpc_connect,
	.pemp_level		 = PRE_EMPHASIS_WITH_20_PERCENT,
	.cdr_autoreset		 = CDR_AUTO_RESET_DISABLE,
	.drv_ampl		 = HS_DRV_AMPLITUDE_DEFAULT,
	.se1_gating		 = SE1_GATING_DISABLE,
	.ldo_init		 = msm_hsusb_ldo_init,
	.ldo_enable		 = msm_hsusb_ldo_enable,
	.chg_init		 = hsusb_chg_init,
	.chg_connected		 = hsusb_chg_connected,
	.chg_vbus_draw		 = hsusb_chg_vbus_draw,
};
#endif

static struct msm_hsusb_gadget_platform_data msm_gadget_pdata = {
	.is_phy_status_timer_on = 1,
#if defined(CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE)
	.get_usb_chg_type = checkChargerType,
#endif
	.prop_chg = 0,
};

#define WLAN_HOST_WAKE


#ifdef WLAN_HOST_WAKE
struct wlansleep_info {
	unsigned host_wake;
	unsigned host_wake_irq;
	struct wake_lock wake_lock;
};


static struct wlansleep_info *wsi;
static DEFINE_MUTEX(wlan_33v_mutex);
static struct tasklet_struct hostwake_task;


static void wlan_hostwake_task(unsigned long data)
{
	printk(KERN_INFO "WLAN: wake lock timeout 0.5 sec...\n");

	wake_lock_timeout(&wsi->wake_lock, HZ / 2);
}


static irqreturn_t wlan_hostwake_isr(int irq, void *dev_id)
{
//please fix    gpio_clear_detect_status(wsi->host_wake_irq);

	/* schedule a tasklet to handle the change in the host wake line */
	tasklet_schedule(&hostwake_task);
	return IRQ_HANDLED;
}


static int wlan_host_wake_init(void)
{
	int ret;

	gpio_tlmm_config(GPIO_CFG(42, 0, GPIO_CFG_INPUT,
			GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);

	wsi = kzalloc(sizeof(struct wlansleep_info), GFP_KERNEL);
	if (!wsi)
		return -ENOMEM;

	wake_lock_init(&wsi->wake_lock, WAKE_LOCK_SUSPEND, "wlansleep");
	tasklet_init(&hostwake_task, wlan_hostwake_task, 0);

	wsi->host_wake = GPIO_WLAN_HOST_WAKE;
	wsi->host_wake_irq = MSM_GPIO_TO_INT(wsi->host_wake);

//please fix    gpio_configure(wsi->host_wake, GPIOF_INPUT);
	ret = request_irq(wsi->host_wake_irq, wlan_hostwake_isr,
						IRQF_DISABLED | IRQF_TRIGGER_RISING,
						"wlan hostwake", NULL);
	if (ret < 0) {
		printk(KERN_ERR "WLAN: Couldn't acquire WLAN_HOST_WAKE IRQ");
		return -1;
	}

	ret = enable_irq_wake(wsi->host_wake_irq);
	if (ret < 0) {
		printk(KERN_ERR "WLAN: Couldn't enable WLAN_HOST_WAKE as wakeup interrupt");
		free_irq(wsi->host_wake_irq, NULL);
		return -1;
	}

	return 0;
}


static void wlan_host_wake_exit(void)
{
	if (!wsi)
		return;

	if (disable_irq_wake(wsi->host_wake_irq))
		printk(KERN_ERR "WLAN: Couldn't disable hostwake IRQ wakeup mode \n");

	free_irq(wsi->host_wake_irq, NULL);

	wake_lock_destroy(&wsi->wake_lock);
	kfree(wsi);
}
#endif /* WLAN_HOST_WAKE */

static int wlan_set_gpio(unsigned gpio, int on)
{
	int rc = 0;
	int gpio_value = 0;

	printk("%s - %d : %s\n", __func__, gpio, on ? "on" : "off");

	// Request
	if (gpio_request(gpio, "wlan_ar6000_pm")) {
		printk(KERN_ERR "%s: gpio_request for %d failed\n",
				__func__, gpio);
		return -1;
	}

	gpio_value = gpio_get_value(gpio);
	printk(KERN_INFO "%s: before (%d) :: gpio_get_value = %d",
			__func__, on, gpio_value);

	// Direction Output On/Off
	rc = gpio_direction_output(gpio, on);
	gpio_free(gpio);

	gpio_value = gpio_get_value(gpio);
	printk(KERN_INFO "%s: after (%d) :: gpio_get_value = %d",
			__func__, on, gpio_value);

	if (rc) {
		printk(KERN_ERR "%s: gpio_direction_output for %d failed\n",
				__func__, gpio);
		return -1;
	}

	return 0;
}


#ifdef WLAN_33V_CONTROL_FOR_BT_ANTENNA
int wlan_setup_ldo_33v(int input_flag, int on)
{
	int skip = 0;
	int temp_flag = wlan_33v_flag;

	/* locking */
	mutex_lock(&wlan_33v_mutex);

	printk(KERN_INFO "%s - set by %s : %s\n",
			__func__,
			(input_flag == WLAN_33V_WIFI_FLAG) ? "Wifi" : "BT",
			on ? "on" : "off");
	printk(KERN_INFO "%s - old wlan_33v_flag : %d\n",
			__func__, temp_flag);

	if (on) {
		if (temp_flag)  /* Already On */
			skip = 1;

		temp_flag |= input_flag;
	} else {
		temp_flag &= (~input_flag);

		/* Keep WLAN_33V_EN on if either BT or Wifi is turned on*/
		if (temp_flag)
			skip = 1;
	}

	printk(KERN_INFO "%s - new wlan_33v_flag : %d\n",
			__func__, temp_flag);

	if (skip) {
		printk(KERN_INFO "%s - Skip WLAN_33V_EN %s\n",
				__func__, on ? "on" : "off");
	} else {
		int rc;
		struct vreg *vreg_wlan = NULL;

		if (vreg_wlan == NULL) {
			vreg_wlan = vreg_get(NULL, "wlan");

			if (IS_ERR(vreg_wlan)) {
				printk(KERN_ERR "%s: vreg_get(%s) failed (%ld)\n", __func__, "vlcd4", PTR_ERR(vreg_wlan));
				/* unlocking */
				mutex_unlock(&wlan_33v_mutex);
				return;
			}

			rc = vreg_set_level(vreg_wlan, 3300);
			if (rc) {
				printk(KERN_ERR "%s: wlan set_level failed (%d)\n", __func__, rc);
			}
		}
		
		if (on) {
			rc = vreg_enable(vreg_wlan);
			if (rc) {
				printk(KERN_ERR "%s: wlan enable failed (%d)\n", __func__, rc);
			} else {
				printk(KERN_ERR "%s: wlan enable success (%d)\n", __func__, rc);
			}
		} else {
			rc = vreg_disable(vreg_wlan);
			if (rc) {
				printk(KERN_ERR "%s: wlan disable failed (%d)\n", __func__, rc);
			} else {
				printk(KERN_ERR "%s: wlan disable success (%d)\n", __func__, rc);
			}
		}
	}

	wlan_33v_flag = temp_flag;
	/* unlocking */
	mutex_unlock(&wlan_33v_mutex);

	return WLAN_OK;
}
#endif

void wlan_setup_power(int on, int detect)
{
	printk("%s %s --enter\n", __func__, on ? "on" : "down");

	if (on) {
#ifdef WLAN_33V_CONTROL_FOR_BT_ANTENNA
		/* WLAN_33V_EN - On */
		if (wlan_setup_ldo_33v(WLAN_33V_WIFI_FLAG, 1))
			return;
#endif

		udelay(200);

		// GPIO_WLAN_RESET_N - On
		if (wlan_set_gpio(GPIO_WLAN_RESET_N, 1))
			return;

#ifdef WLAN_HOST_WAKE
		wlan_host_wake_init();
#endif /* WLAN_HOST_WAKE */
	}
	else {
#ifdef WLAN_HOST_WAKE
		wlan_host_wake_exit();
#endif /* WLAN_HOST_WAKE */

		// GPIO_WLAN_RESET_N - Off
		if (wlan_set_gpio(GPIO_WLAN_RESET_N, 0))
			return;

		udelay(60);

#ifdef WLAN_33V_CONTROL_FOR_BT_ANTENNA
		/* WLAN_33V_EN - Off */
		if (wlan_setup_ldo_33v(WLAN_33V_WIFI_FLAG, 0))
			return;
#endif
	}

#ifndef ATH_POLLING
	mdelay(100);

	if (detect) {
		/* Detect card */
		if (wlan_status_notify_cb)
			wlan_status_notify_cb(on, wlan_devid);
		else
			printk(KERN_ERR "WLAN: No notify available\n");
	}
#endif /* ATH_POLLING */
}
EXPORT_SYMBOL(wlan_setup_power);
//EXPORT_SYMBOL(board_hw_revision);


static int wlan_power_init(void)
{
#ifdef WLAN_33V_CONTROL_FOR_BT_ANTENNA
	wlan_33v_flag = 0;
#endif

	return WLAN_OK;
}

#ifdef CONFIG_SERIAL_MSM_HS
static struct msm_serial_hs_platform_data msm_uart_dm1_pdata = {
	.inject_rx_on_wakeup	= 1,
	.rx_to_inject		= 0xFD,
};
#endif
static struct msm_pm_platform_data msm7627a_pm_data[MSM_PM_SLEEP_MODE_NR] = {
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 16000,
					.residency = 20000,
	},
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 12000,
					.residency = 20000,
	},
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 0,
					.suspend_enabled = 1,
					.latency = 2000,
					.residency = 0,
	},
	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 2,
					.residency = 0,
	},
};

static struct msm_pm_boot_platform_data msm_pm_boot_pdata __initdata = {
	.mode = MSM_PM_BOOT_CONFIG_RESET_VECTOR_PHYS,
	.p_addr = 0,
};

/* 8625 PM platform data */
static struct msm_pm_platform_data
		msm8625_pm_data[MSM_PM_SLEEP_MODE_NR * CONFIG_NR_CPUS] = {
	/* CORE0 entries */
	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_POWER_COLLAPSE)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 0,
					.suspend_enabled = 0,
					.latency = 16000,
					.residency = 20000,
	},

	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 0,
					.suspend_enabled = 0,
					.latency = 12000,
					.residency = 20000,
	},

	/* picked latency & redisdency values from 7x30 */
	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
#if 0
					.idle_enabled = 1,
					.suspend_enabled = 1,
#else
					.idle_enabled = 0,
					.suspend_enabled = 0,
#endif
					.latency = 500,
					.residency = 500,
	},

	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 2,
					.residency = 10,
	},

	/* picked latency & redisdency values from 7x30 */
	[MSM_PM_MODE(1, MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
#if 0
					.idle_enabled = 1,
					.suspend_enabled = 1,
#else
					.idle_enabled = 0,
					.suspend_enabled = 0,
#endif
					.latency = 500,
					.residency = 500,
	},

	[MSM_PM_MODE(1, MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 2,
					.residency = 10,
	},

	/* picked latency & redisdency values from 7x30 */
	[MSM_PM_MODE(2, MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
#if 0
					.idle_enabled = 1,
					.suspend_enabled = 1,
#else
					.idle_enabled = 0,
					.suspend_enabled = 0,
#endif
					.latency = 500,
					.residency = 500,
	},

	[MSM_PM_MODE(2, MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 2,
					.residency = 10,
	},

	/* picked latency & redisdency values from 7x30 */
	[MSM_PM_MODE(3, MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
#if 0
					.idle_enabled = 1,
					.suspend_enabled = 1,
#else
					.idle_enabled = 0,
					.suspend_enabled = 0,
#endif
					.latency = 500,
					.residency = 500,
	},

	[MSM_PM_MODE(3, MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 2,
					.residency = 10,
	},

};

static struct msm_pm_boot_platform_data msm_pm_8625_boot_pdata __initdata = {
	.mode = MSM_PM_BOOT_CONFIG_REMAP_BOOT_ADDR,
	.v_addr = MSM_CFG_CTL_BASE,
};

static struct android_pmem_platform_data android_pmem_adsp_pdata = {
	.name = "pmem_adsp",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 1,
	.memory_type = MEMTYPE_EBI1,
};

static struct platform_device android_pmem_adsp_device = {
	.name = "android_pmem",
	.id = 1,
	.dev = { .platform_data = &android_pmem_adsp_pdata },
};

static unsigned pmem_mdp_size = MSM_PMEM_MDP_SIZE;
static int __init pmem_mdp_size_setup(char *p)
{
	pmem_mdp_size = memparse(p, NULL);
	return 0;
}

early_param("pmem_mdp_size", pmem_mdp_size_setup);

static unsigned pmem_adsp_size = MSM_PMEM_ADSP_SIZE;
static int __init pmem_adsp_size_setup(char *p)
{
	pmem_adsp_size = memparse(p, NULL);
	return 0;
}

early_param("pmem_adsp_size", pmem_adsp_size_setup);

static struct android_pmem_platform_data android_pmem_audio_pdata = {
	.name = "pmem_audio",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
	.memory_type = MEMTYPE_EBI1,
};

static struct platform_device android_pmem_audio_device = {
	.name = "android_pmem",
	.id = 2,
	.dev = { .platform_data = &android_pmem_audio_pdata },
};

static struct android_pmem_platform_data android_pmem_pdata = {
	.name = "pmem",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 1,
	.memory_type = MEMTYPE_EBI1,
};
static struct platform_device android_pmem_device = {
	.name = "android_pmem",
	.id = 0,
	.dev = { .platform_data = &android_pmem_pdata },
};

static char *msm_adc_surf_device_names[] = {
	"XO_ADC",
};

static struct msm_adc_platform_data msm_adc_pdata = {
	.dev_names = msm_adc_surf_device_names,
	.num_adc = ARRAY_SIZE(msm_adc_surf_device_names),
	.target_hw = MSM_8x25,
};

static struct platform_device msm_adc_device = {
	.name   = "msm_adc",
	.id = -1,
	.dev = {
		.platform_data = &msm_adc_pdata,
	},
};

/* GGSM sc47.yun CSR7820 Project 2012.04.23 */
#ifdef CONFIG_BT_CSR_7820
static struct resource bluesleep_resources[] = {
	{
		.name = "gpio_host_wake",
/*
		.start = GPIO_BT_PWR,
		.end = GPIO_BT_PWR,
*/
		.start = GPIO_BT_HOST_WAKE,
		.end = GPIO_BT_HOST_WAKE,
	
		.flags = IORESOURCE_IO,
	},
	{
		.name = "host_wake",
/*		
		.start = MSM_GPIO_TO_INT(GPIO_BT_PWR),
		.end = MSM_GPIO_TO_INT(GPIO_BT_PWR),
*/
		.start = MSM_GPIO_TO_INT(GPIO_BT_HOST_WAKE),
		.end = MSM_GPIO_TO_INT(GPIO_BT_HOST_WAKE),
		
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device msm_bt_power_device_csr = {
	.name = "bt_power",
};

static struct platform_device msm_bluesleep_device = {
	.name = "bluesleep",
	.id = -1,
	.num_resources = ARRAY_SIZE(bluesleep_resources),
	.resource = bluesleep_resources,
};
#endif
/* GGSM sc47.yun CSR7820 Project end */


#define GPIO_VREG_INIT(_id, _reg_name, _gpio_label, _gpio, _active_low) \
	[GPIO_VREG_ID_##_id] = { \
		.init_data = { \
			.constraints = { \
				.valid_ops_mask	= REGULATOR_CHANGE_STATUS, \
			}, \
			.num_consumer_supplies	= \
					ARRAY_SIZE(vreg_consumers_##_id), \
			.consumer_supplies	= vreg_consumers_##_id, \
		}, \
		.regulator_name	= _reg_name, \
		.active_low	= _active_low, \
		.gpio_label	= _gpio_label, \
		.gpio		= _gpio, \
	}

#define GPIO_VREG_ID_EXT_2P85V	0
#define GPIO_VREG_ID_EXT_1P8V	1
#define GPIO_VREG_ID_EXT_2P85V_EVBD	2
#define GPIO_VREG_ID_EXT_1P8V_EVBD	3

static struct regulator_consumer_supply vreg_consumers_EXT_2P85V[] = {
	REGULATOR_SUPPLY("cam_ov5647_avdd", "0-006c"),
	REGULATOR_SUPPLY("cam_ov7692_avdd", "0-0078"),
	REGULATOR_SUPPLY("cam_ov8825_avdd", "0-000d"),
	REGULATOR_SUPPLY("lcd_vdd", "mipi_dsi.1"),
};

static struct regulator_consumer_supply vreg_consumers_EXT_1P8V[] = {
	REGULATOR_SUPPLY("cam_ov5647_vdd", "0-006c"),
	REGULATOR_SUPPLY("cam_ov7692_vdd", "0-0078"),
	REGULATOR_SUPPLY("cam_ov8825_vdd", "0-000d"),
	REGULATOR_SUPPLY("lcd_vddi", "mipi_dsi.1"),
};

static struct regulator_consumer_supply vreg_consumers_EXT_2P85V_EVBD[] = {
	REGULATOR_SUPPLY("cam_ov5648_avdd", "0-006c"),
	REGULATOR_SUPPLY("cam_ov7695_avdd", "0-0042"),
	REGULATOR_SUPPLY("lcd_vdd", "mipi_dsi.1"),
};

static struct regulator_consumer_supply vreg_consumers_EXT_1P8V_EVBD[] = {
	REGULATOR_SUPPLY("cam_ov5648_vdd", "0-006c"),
	REGULATOR_SUPPLY("cam_ov7695_vdd", "0-0042"),
	REGULATOR_SUPPLY("lcd_vddi", "mipi_dsi.1"),
};

/* GPIO regulator constraints */
static struct gpio_regulator_platform_data msm_gpio_regulator_pdata[] = {
	GPIO_VREG_INIT(EXT_2P85V, "ext_2p85v", "ext_2p85v_en", 35, 0),
	GPIO_VREG_INIT(EXT_1P8V, "ext_1p8v", "ext_1p8v_en", 40, 0),
	GPIO_VREG_INIT(EXT_2P85V_EVBD, "ext_2p85v_evbd",
						"ext_2p85v_evbd_en", 5, 0),
	GPIO_VREG_INIT(EXT_1P8V_EVBD, "ext_1p8v_evbd",
						"ext_1p8v_evbd_en", 6, 0),
};

/* GPIO regulator */
static struct platform_device qrd_vreg_gpio_ext_2p85v __devinitdata = {
	.name	= GPIO_REGULATOR_DEV_NAME,
	.id	= 35,
	.dev	= {
		.platform_data =
			&msm_gpio_regulator_pdata[GPIO_VREG_ID_EXT_2P85V],
	},
};

static struct platform_device qrd_vreg_gpio_ext_1p8v __devinitdata = {
	.name	= GPIO_REGULATOR_DEV_NAME,
	.id	= 40,
	.dev	= {
		.platform_data =
			&msm_gpio_regulator_pdata[GPIO_VREG_ID_EXT_1P8V],
	},
};

static struct platform_device evbd_vreg_gpio_ext_2p85v __devinitdata = {
	.name	= GPIO_REGULATOR_DEV_NAME,
	.id	= 5,
	.dev	= {
		.platform_data =
			&msm_gpio_regulator_pdata[GPIO_VREG_ID_EXT_2P85V_EVBD],
	},
};

static struct platform_device evbd_vreg_gpio_ext_1p8v __devinitdata = {
	.name	= GPIO_REGULATOR_DEV_NAME,
	.id	= 6,
	.dev	= {
		.platform_data =
			&msm_gpio_regulator_pdata[GPIO_VREG_ID_EXT_1P8V_EVBD],
	},
};

/* Regulator configuration for the NCP6335D buck */
struct regulator_consumer_supply ncp6335d_consumer_supplies[] = {
	REGULATOR_SUPPLY("ncp6335d", NULL),
	/* TO DO: NULL entry needs to be fixed once
	 * we fix the cross-dependencies.
	*/
	REGULATOR_SUPPLY("vddx_cx", NULL),
};

static struct regulator_init_data ncp6335d_init_data = {
	.constraints	= {
		.name		= "ncp6335d_sw",
		.min_uV		= 600000,
		.max_uV		= 1400000,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
				REGULATOR_CHANGE_STATUS |
				REGULATOR_CHANGE_MODE,
		.valid_modes_mask = REGULATOR_MODE_NORMAL |
				REGULATOR_MODE_FAST,
		.initial_mode	= REGULATOR_MODE_NORMAL,
		.always_on	= 1,
	},
	.num_consumer_supplies = ARRAY_SIZE(ncp6335d_consumer_supplies),
	.consumer_supplies = ncp6335d_consumer_supplies,
};

static struct ncp6335d_platform_data ncp6335d_pdata = {
	.init_data = &ncp6335d_init_data,
	.default_vsel = NCP6335D_VSEL0,
	.slew_rate_ns = 333,
	.rearm_disable = 1,
};

static struct i2c_board_info i2c2_info[] __initdata = {
	{
		I2C_BOARD_INFO("ncp6335d", 0x38 >> 1),
		.platform_data = &ncp6335d_pdata,
	},
};

static struct platform_device *common_devices[] __initdata = {
	&android_usb_device,
	&android_pmem_device,
	&android_pmem_adsp_device,
	&android_pmem_audio_device,
	&msm_batt_device,
	&msm_device_adspdec,
	&msm_device_snd,
	&msm_device_cad,
	&asoc_msm_pcm,
	&asoc_msm_dai0,
	&asoc_msm_dai1,
	&msm_adc_device,
	&msm_bt_power_device_csr,
	&msm_bluesleep_device,
#ifdef CONFIG_ION_MSM
	&ion_dev,
#endif
};

static struct platform_device *qrd7627a_devices[] __initdata = {
	&msm_device_dmov,
	&msm_device_smd,
	&msm_device_uart1,
	&msm_device_uart_dm1,
	&msm_gsbi0_qup_i2c_device,
	&msm_gsbi1_qup_i2c_device,
	&msm_device_otg,
	&msm_device_gadget_peripheral,
	&msm_kgsl_3d0,
	&qrd_vreg_gpio_ext_2p85v,
	&qrd_vreg_gpio_ext_1p8v,
};

static struct platform_device *qrd3_devices[] __initdata = {
	&msm_device_nand,
};

static struct platform_device *msm8625_evb_devices[] __initdata = {
	&msm8625_device_dmov,
	&msm8625_device_smd,
	&msm8625_gsbi0_qup_i2c_device,
	&msm8625_gsbi1_qup_i2c_device,
	&msm8625_device_uart_dm1,
	&msm8625_device_otg,
	&msm8625_device_gadget_peripheral,
	&msm8625_kgsl_3d0,

	&touch_i2c_gpio_device,
	&sensor_i2c_gpio_device,
	&fsa9480_i2c_gpio_device,
#ifdef CONFIG_KEYBOARD_STMPE1801
	&stmpe1801_i2c_gpio_device,
#endif
#ifdef CONFIG_SENSORS_GP2A030
	&opt_gp2a,
#endif
#ifdef CONFIG_MSM_VIBRATOR
	&msm_vibrator_device,
#endif
#if defined(CONFIG_SAMSUNG_JACK) || defined(CONFIG_SEC_FSA_JACK)
	&sec_device_jack,
#endif
#ifdef CONFIG_MAX17048_FUELGAUGE
	&fg_smb_i2c_gpio_device,
#endif
#ifdef CONFIG_BQ27425_FUEL_GAUGE
	&fuelgauge_i2c_gpio_device,
#endif
#ifdef CONFIG_KEYBOARD_GPIO
	& msm7x27a_gpio_keys_device,
#endif

#ifdef CONFIG_MACH_DELOS_CTC /* delos ctc has 2 type of charger */
	&delos_charger_gpio_device,
#else  /* else of CONFIG_MACH_DELOS_CTC */

#ifdef CONFIG_CHARGER_FAN54013
	&fan_charger_i2c_gpio_device,
#endif
#ifdef CONFIG_CHARGER_SMB328A
	&smb_charger_i2c_gpio_device,
#endif
#ifdef CONFIG_CHARGER_BQ24157
	&charger_i2c_gpio_device,
#endif

#endif  /* end of CONFIG_MACH_DELOS_CTC */
};

static struct platform_device *msm8625_lcd_camera_devices[] __initdata = {
	&qrd_vreg_gpio_ext_2p85v,
	&qrd_vreg_gpio_ext_1p8v,
};

static struct platform_device *msm8625q_lcd_camera_devices[] __initdata = {
	&evbd_vreg_gpio_ext_2p85v,
	&evbd_vreg_gpio_ext_1p8v,
};

static unsigned pmem_kernel_ebi1_size = PMEM_KERNEL_EBI1_SIZE;
static int __init pmem_kernel_ebi1_size_setup(char *p)
{
	pmem_kernel_ebi1_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_kernel_ebi1_size", pmem_kernel_ebi1_size_setup);

static unsigned pmem_audio_size = MSM_PMEM_AUDIO_SIZE;
static int __init pmem_audio_size_setup(char *p)
{
	pmem_audio_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_audio_size", pmem_audio_size_setup);

static void fix_sizes(void)
{
	if (get_ddr_size() > SZ_512M)
		pmem_adsp_size = CAMERA_ZSL_SIZE;
	else {
		if (machine_is_qrd_skud_prime() || machine_is_msm8625q_evbd()
					|| machine_is_msm8625q_skud())
			pmem_mdp_size = 0;
	}
#ifdef CONFIG_ION_MSM
	msm_ion_camera_size = pmem_adsp_size;
	msm_ion_audio_size = MSM_PMEM_AUDIO_SIZE;
#ifdef CONFIG_CMA
	msm_ion_camera_size_carving = 0;
#else
	msm_ion_camera_size_carving = msm_ion_camera_size;
#endif
	msm_ion_sf_size = pmem_mdp_size;
#endif
}

#ifdef CONFIG_ION_MSM
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
static struct ion_co_heap_pdata co_ion_pdata = {
	.adjacent_mem_id = INVALID_HEAP_ID,
	.align = PAGE_SIZE,
};

static struct ion_co_heap_pdata co_mm_ion_pdata = {
	.adjacent_mem_id = INVALID_HEAP_ID,
	.align = PAGE_SIZE,
};

static u64 msm_dmamask = DMA_BIT_MASK(32);

static struct platform_device ion_cma_device = {
	.name = "ion-cma-device",
	.id = -1,
	.dev = {
		.dma_mask = &msm_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	}
};
#endif

/**
 * These heaps are listed in the order they will be allocated.
 * Don't swap the order unless you know what you are doing!
 */
struct ion_platform_heap msm7627a_heaps[] = {
		{
			.id	= ION_SYSTEM_HEAP_ID,
			.type	= ION_HEAP_TYPE_SYSTEM,
			.name	= ION_VMALLOC_HEAP_NAME,
		},
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
		/* PMEM_ADSP = CAMERA */
		{
			.id	= ION_CAMERA_HEAP_ID,
			.type	= CAMERA_HEAP_TYPE,
			.name	= ION_CAMERA_HEAP_NAME,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *)&co_mm_ion_pdata,
			.priv	= (void *)&ion_cma_device.dev,
		},
		/* AUDIO HEAP 1*/
		{
			.id	= ION_AUDIO_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_AUDIO_HEAP_NAME,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *)&co_ion_pdata,
		},
		/* PMEM_MDP = SF */
		{
			.id	= ION_SF_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_SF_HEAP_NAME,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *)&co_ion_pdata,
		},
		/* AUDIO HEAP 2*/
		{
			.id    = ION_AUDIO_HEAP_BL_ID,
			.type  = ION_HEAP_TYPE_CARVEOUT,
			.name  = ION_AUDIO_BL_HEAP_NAME,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *)&co_ion_pdata,
			.base = BOOTLOADER_BASE_ADDR,
		},
#endif
};

static struct ion_platform_data ion_pdata = {
	.nr = MSM_ION_HEAP_NUM,
	.has_outer_cache = 1,
	.heaps = msm7627a_heaps,
};

static struct platform_device ion_dev = {
	.name = "ion-msm",
	.id = 1,
	.dev = { .platform_data = &ion_pdata },
};
#endif

static struct memtype_reserve msm7627a_reserve_table[] __initdata = {
	[MEMTYPE_SMI] = {
	},
	[MEMTYPE_EBI0] = {
		.flags	=	MEMTYPE_FLAGS_1M_ALIGN,
	},
	[MEMTYPE_EBI1] = {
		.flags	=	MEMTYPE_FLAGS_1M_ALIGN,
	},
};

#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
static struct android_pmem_platform_data *pmem_pdata_array[] __initdata = {
		&android_pmem_adsp_pdata,
		&android_pmem_audio_pdata,
		&android_pmem_pdata,
};
#endif
#endif

static void __init size_pmem_devices(void)
{
#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
	unsigned int i;

	android_pmem_adsp_pdata.size = pmem_adsp_size;
	android_pmem_pdata.size = pmem_mdp_size;
	android_pmem_audio_pdata.size = pmem_audio_size;
#endif
#endif
}

#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
static void __init reserve_memory_for(struct android_pmem_platform_data *p)
{
	msm7627a_reserve_table[p->memory_type].size += p->size;
}
#endif
#endif

static void __init reserve_pmem_memory(void)
{
#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
	unsigned int i;
	for (i = 0; i < ARRAY_SIZE(pmem_pdata_array); ++i)
		reserve_memory_for(pmem_pdata_array[i]);

	msm7627a_reserve_table[MEMTYPE_EBI1].size += pmem_kernel_ebi1_size;
#endif
#endif
}

static void __init size_ion_devices(void)
{
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
	ion_pdata.heaps[1].size = msm_ion_camera_size;
	ion_pdata.heaps[2].size = PMEM_KERNEL_EBI1_SIZE;
	ion_pdata.heaps[3].size = msm_ion_sf_size;
	ion_pdata.heaps[4].size = msm_ion_audio_size;
#endif
}

static void __init reserve_ion_memory(void)
{
#if defined(CONFIG_ION_MSM) && defined(CONFIG_MSM_MULTIMEDIA_USE_ION)
	msm7627a_reserve_table[MEMTYPE_EBI1].size += PMEM_KERNEL_EBI1_SIZE;
	msm7627a_reserve_table[MEMTYPE_EBI1].size += msm_ion_sf_size;
	msm7627a_reserve_table[MEMTYPE_EBI1].size +=
		msm_ion_camera_size_carving;
#endif
}

static void __init msm7627a_calculate_reserve_sizes(void)
{
	fix_sizes();
	size_pmem_devices();
	reserve_pmem_memory();
	size_ion_devices();
	reserve_ion_memory();
}

static int msm7627a_paddr_to_memtype(unsigned int paddr)
{
	return MEMTYPE_EBI1;
}

static struct reserve_info msm7627a_reserve_info __initdata = {
	.memtype_reserve_table = msm7627a_reserve_table,
	.calculate_reserve_sizes = msm7627a_calculate_reserve_sizes,
	.paddr_to_memtype = msm7627a_paddr_to_memtype,
};

static void __init msm7627a_reserve(void)
{
	reserve_info = &msm7627a_reserve_info;
	memblock_remove(MSM8625_NON_CACHE_MEM, SZ_2K);
	memblock_remove(BOOTLOADER_BASE_ADDR, msm_ion_audio_size);
	msm_reserve();
#ifdef CONFIG_CMA
	dma_declare_contiguous(
			&ion_cma_device.dev,
			msm_ion_camera_size,
			CAMERA_HEAP_BASE,
			0xa0000000);
#endif
}

static void __init msm8625_reserve(void)
{
	memblock_remove(MSM8625_NON_CACHE_MEM, SZ_2K);
	memblock_remove(MSM8625_CPU_PHYS, SZ_8);
	memblock_remove(MSM8625_WARM_BOOT_PHYS, SZ_32);
	msm7627a_reserve();
}

static void msmqrd_adsp_add_pdev(void)
{
	int rc = 0;
	struct rpc_board_dev *rpc_adsp_pdev;

	rpc_adsp_pdev = kzalloc(sizeof(struct rpc_board_dev), GFP_KERNEL);
	if (rpc_adsp_pdev == NULL) {
		pr_err("%s: Memory Allocation failure\n", __func__);
		return;
	}
	rpc_adsp_pdev->prog = ADSP_RPC_PROG;

	if (cpu_is_msm8625() || cpu_is_msm8625q())
		rpc_adsp_pdev->pdev = msm8625_device_adsp;
	else
		rpc_adsp_pdev->pdev = msm_adsp_device;
	rc = msm_rpc_add_board_dev(rpc_adsp_pdev, 1);
	if (rc < 0) {
		pr_err("%s: return val: %d\n",	__func__, rc);
		kfree(rpc_adsp_pdev);
	}
}

static void __init msm7627a_device_i2c_init(void)
{
	msm_gsbi0_qup_i2c_device.dev.platform_data = &msm_gsbi0_qup_i2c_pdata;
	msm_gsbi1_qup_i2c_device.dev.platform_data = &msm_gsbi1_qup_i2c_pdata;
}

static void __init msm8625_device_i2c_init(void)
{
	int i, rc;

	msm8625_gsbi0_qup_i2c_device.dev.platform_data
					= &msm_gsbi0_qup_i2c_pdata;
	msm8625_gsbi1_qup_i2c_device.dev.platform_data
					= &msm_gsbi1_qup_i2c_pdata;
	if (machine_is_qrd_skud_prime() || cpu_is_msm8625q()) {
		for (i = 0 ; i < ARRAY_SIZE(msm8625q_i2c_gpio_config); i++) {
			rc = gpio_tlmm_config(
					msm8625q_i2c_gpio_config[i].gpio_cfg,
					GPIO_CFG_ENABLE);
			if (rc)
				pr_err("I2C-gpio tlmm config failed\n");
		}
		rc = platform_device_register(&msm8625q_i2c_gpio);
		if (rc)
			pr_err("%s: could not register i2c-gpio device: %d\n",
						__func__, rc);
	}
}

static struct platform_device msm_proccomm_regulator_dev = {
	.name   = PROCCOMM_REGULATOR_DEV_NAME,
	.id     = -1,
	.dev    = {
		.platform_data = &msm7x27a_proccomm_regulator_data
	}
};

static void __init msm7627a_init_regulators(void)
{
	int rc = platform_device_register(&msm_proccomm_regulator_dev);
	if (rc)
		pr_err("%s: could not register regulator device: %d\n",
				__func__, rc);
}

static int __init msm_qrd_init_ar6000pm(void)
{
	msm_wlan_ar6000_pm_device.dev.platform_data = &ar600x_wlan_power;
	return platform_device_register(&msm_wlan_ar6000_pm_device);
}

static void __init msm_add_footswitch_devices(void)
{
	platform_add_devices(msm_footswitch_devices,
				msm_num_footswitch_devices);
}

static void __init add_platform_devices(void)
{
	if (machine_is_msm8625_evb() || machine_is_msm8625_qrd7()
				|| machine_is_msm8625_evt()
				|| machine_is_msm8625q_evbd()
				|| machine_is_msm8625q_skud()
				|| machine_is_qrd_skud_prime()) {
		platform_add_devices(msm8625_evb_devices,
				ARRAY_SIZE(msm8625_evb_devices));

#if defined(CONFIG_MACH_DELOS_DUOS_CTC)
		if (board_hw_revision < 4)	
#endif
		platform_device_register(&msm8625_device_uart1);

		platform_add_devices(qrd3_devices,
				ARRAY_SIZE(qrd3_devices));
	} else {
		platform_add_devices(qrd7627a_devices,
				ARRAY_SIZE(qrd7627a_devices));
	}

	if (machine_is_msm8625_evb() || machine_is_msm8625_evt())
		platform_add_devices(msm8625_lcd_camera_devices,
				ARRAY_SIZE(msm8625_lcd_camera_devices));
	else if (machine_is_msm8625q_evbd())
		platform_add_devices(msm8625q_lcd_camera_devices,
				ARRAY_SIZE(msm8625q_lcd_camera_devices));

	if (machine_is_msm7627a_qrd3() || machine_is_msm7627a_evb())
		platform_add_devices(qrd3_devices,
				ARRAY_SIZE(qrd3_devices));

	platform_add_devices(common_devices,
			ARRAY_SIZE(common_devices));
}

#define UART1DM_RX_GPIO		45
static void __init qrd7627a_uart1dm_config(void)
{
//	Inband sleep BT don't need rx irq, so remove it to prevent uart dma sleep issue
//	msm_uart_dm1_pdata.wakeup_irq = gpio_to_irq(UART1DM_RX_GPIO);
	msm_uart_dm1_pdata.wakeup_irq = 0;
	if (cpu_is_msm8625() || cpu_is_msm8625q())
		msm8625_device_uart_dm1.dev.platform_data =
			&msm_uart_dm1_pdata;
	else
		msm_device_uart_dm1.dev.platform_data = &msm_uart_dm1_pdata;
}

static void __init qrd7627a_otg_gadget(void)
{
	if (cpu_is_msm8625() || cpu_is_msm8625q()) {
		msm_otg_pdata.swfi_latency = msm8625_pm_data
		[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].latency;
		msm8625_device_otg.dev.platform_data = &msm_otg_pdata;
		msm8625_device_gadget_peripheral.dev.platform_data =
					&msm_gadget_pdata;

	} else {
	msm_otg_pdata.swfi_latency = msm7627a_pm_data
		[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency;
		msm_device_otg.dev.platform_data = &msm_otg_pdata;
		msm_device_gadget_peripheral.dev.platform_data =
					&msm_gadget_pdata;
	}
}

static void __init msm_pm_init(void)
{

	if (!cpu_is_msm8625() && !cpu_is_msm8625q()) {
		msm_pm_set_platform_data(msm7627a_pm_data,
				ARRAY_SIZE(msm7627a_pm_data));
		BUG_ON(msm_pm_boot_init(&msm_pm_boot_pdata));
	} else {
		msm_pm_set_platform_data(msm8625_pm_data,
				ARRAY_SIZE(msm8625_pm_data));
		BUG_ON(msm_pm_boot_init(&msm_pm_8625_boot_pdata));
		msm8x25_spm_device_init();
		msm_pm_register_cpr_ops();
	}
}

extern unsigned int kernel_uart_flag;

/* GGSM sc47.yun CSR7820 Project 2012.04.23 */
#ifdef CONFIG_BT_CSR_7820

#ifdef WLAN_33V_CONTROL_FOR_BT_ANTENNA
extern void bluetooth_setup_ldo_33v(int on)
{
	wlan_setup_ldo_33v(WLAN_33V_BT_FLAG, on);
}
#endif

static uint32_t bt_config_power_on[] = {
//	GPIO_CFG(GPIO_RST_BT, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
//	GPIO_CFG(GPIO_BT_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	/*GPIO_CFG(GPIO_BT_WAKE, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),*/

	GPIO_CFG(GPIO_BT_PWR, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	
	GPIO_CFG(GPIO_BT_UART_RTS, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_BT_UART_CTS, 2, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_BT_UART_RXD, 2, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_BT_UART_TXD, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),

	GPIO_CFG(GPIO_BT_PCM_DOUT, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_BT_PCM_DIN, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_BT_PCM_SYNC, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_BT_PCM_CLK, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	//GPIO_CFG(GPIO_BT_PWR, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	//GPIO_CFG(GPIO_BT_HOST_WAKE, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static uint32_t bt_config_power_off[] = {
//	GPIO_CFG(GPIO_RST_BT, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
//	GPIO_CFG(GPIO_BT_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	/*GPIO_CFG(GPIO_BT_WAKE, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),*/
	
	GPIO_CFG(GPIO_BT_PWR, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	
	GPIO_CFG(GPIO_BT_UART_RTS, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_BT_UART_CTS, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_BT_UART_RXD, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_BT_UART_TXD, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_BT_PCM_DOUT, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_BT_PCM_DIN, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_BT_PCM_SYNC, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_BT_PCM_CLK, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	//GPIO_CFG(GPIO_BT_PWR, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	//GPIO_CFG(GPIO_BT_HOST_WAKE, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
};

static void config_gpio_table(uint32_t *table, int len)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n], GPIO_CFG_ENABLE);
		if (rc) {
			printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}

static int bluetooth_power(int on)
{
	int rc = 0;
	const char *id = "BTPW";
	
	printk(KERN_DEBUG "%s = %d, 1-on 0-off\n", __func__, on);
	
	

	if (on) {
		/*
		gpio_set_value(GPIO_BT_EN, 1);
		printk(KERN_DEBUG "config_gpio_table bt pwr on\n");
		msleep(100);
		config_gpio_table(bt_config_power_on, ARRAY_SIZE(bt_config_power_on));
		msleep(500);
		pr_info("bluetooth_power GPIO_RST_BT:%d   \n", gpio_get_value(GPIO_RST_BT));

		printk(KERN_DEBUG "not use GPIO_BT_WAKE\n");
		gpio_direction_output(GPIO_RST_BT, GPIO_BT_LEVEL_HIGH);
		msleep(50);
		gpio_direction_output(GPIO_RST_BT, GPIO_BT_LEVEL_LOW);

		msleep(100);
		gpio_direction_output(GPIO_RST_BT, GPIO_BT_LEVEL_HIGH);
		*/
	
		printk(KERN_DEBUG "config_gpio_table bt pwr on, board_hw_revision = %d\n", board_hw_revision);
		msleep(100);
		config_gpio_table(bt_config_power_on, ARRAY_SIZE(bt_config_power_on));
		/* BT_HOST_WAKE pin is changed bcuz of MPM mode issue. */
		gpio_tlmm_config(GPIO_CFG(GPIO_BT_HOST_WAKE_rev03, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

		msleep(500);
		//pr_info("bluetooth_power GPIO_RST_BT:%d   \n", gpio_get_value(GPIO_BT_PWR));
	
		printk(KERN_DEBUG "not use GPIO_BT_WAKE\n");
		gpio_direction_output(GPIO_BT_PWR, GPIO_BT_LEVEL_HIGH);
		//msleep(50);
		//gpio_direction_output(GPIO_BT_PWR, GPIO_BT_LEVEL_LOW);

		msleep(100);
		//gpio_direction_output(GPIO_BT_PWR, GPIO_BT_LEVEL_HIGH);

	} else {

		msleep(10);
		gpio_direction_output(GPIO_BT_PWR, GPIO_BT_LEVEL_LOW);

		printk(KERN_DEBUG "config_gpio_table bt pwr off, board_hw_revision = %d\n", board_hw_revision);
		config_gpio_table(bt_config_power_off, ARRAY_SIZE(bt_config_power_off));
		gpio_tlmm_config(GPIO_CFG(GPIO_BT_HOST_WAKE_rev03, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		/*
		msleep(10);
		gpio_direction_output(GPIO_RST_BT, GPIO_BT_LEVEL_LOW);

		printk(KERN_DEBUG "config_gpio_table bt pwr off\n");
		config_gpio_table(bt_config_power_off, ARRAY_SIZE(bt_config_power_off));

		gpio_set_value(GPIO_BT_EN, 0);
		*/
	}
	
#ifdef WLAN_33V_CONTROL_FOR_BT_ANTENNA
	wlan_setup_ldo_33v(WLAN_33V_BT_FLAG, on);
#endif
	return 0;
}


static void __init bt_power_init(void)
{
	int rc = 0;

	pr_info("bt_power_init \n");

	msm_bt_power_device_csr.dev.platform_data = &bluetooth_power;

	pr_info("bt_gpio_init : low \n");

	config_gpio_table(bt_config_power_off, ARRAY_SIZE(bt_config_power_off));
	gpio_tlmm_config(GPIO_CFG(GPIO_BT_HOST_WAKE_rev03, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
/*
	rc = gpio_request(GPIO_RST_BT, "BT_RST");
	if (!rc)
		gpio_direction_output(GPIO_RST_BT, 0);
	else {
		pr_err("%s: gpio_direction_output %d = %d\n", __func__,
			GPIO_RST_BT, rc);
		gpio_free(GPIO_RST_BT);
	}

	rc = gpio_request(GPIO_BT_EN, "BT_EN");
	if (!rc)
		gpio_direction_output(GPIO_BT_EN, 0);
	else {
		pr_err("%s: gpio_direction_output %d = %d\n", __func__,
			GPIO_BT_EN, rc);
		gpio_free(GPIO_BT_EN);
	}
*/	

	rc = gpio_request(GPIO_BT_PWR, "BT_PWR");
	if (!rc)
		gpio_direction_output(GPIO_BT_PWR, 0);
	else {
		pr_err("%s: gpio_direction_output %d = %d\n", __func__,
			GPIO_BT_PWR, rc);
		gpio_free(GPIO_BT_PWR);
	}
}
#endif
/* GGSM sc47.yun CSR7820 Project end */

#if defined(CONFIG_USB_EXTERNAL_CHARGER)
void set_pm_gpio_for_usb_enable(int enable)
{
#if defined(CONFIG_MACH_BAFFIN_DUOS_CTC)
	if( board_hw_revision >= 3 )
	{
		if(enable)
		{
			pmic_gpio_set_value(PMIC_GPIO_2,1);
			printk("%s : chg_valid enabled !!!\n");
		}
		else
		{
			pmic_gpio_set_value(PMIC_GPIO_2,0);
			printk("%s : chg_valid disabled !!!\n");
		}
	}
#endif
}
#endif


static void __init msm_qrd_init(void)
{
	msm7x2x_misc_init();
	msm7627a_init_regulators();
	msmqrd_adsp_add_pdev();

	if (cpu_is_msm8625() || cpu_is_msm8625q())
		msm8625_device_i2c_init();
	else
		msm7627a_device_i2c_init();

	/* uart1dm*/
	qrd7627a_uart1dm_config();
	/*OTG gadget*/
	qrd7627a_otg_gadget();

	fsa9480_gpio_init();
// GSCHO
////////////////////////////////////////////////////////////////////////////

#ifdef CONFIG_SEC_DEBUG
        sec_debug_init();
#endif
#if defined(CONFIG_SAMSUNG_JACK) || defined(CONFIG_SEC_FSA_JACK)
	sec_jack_gpio_init();
#endif

#if defined(CONFIG_SENSORS_BMA2X2) || defined(CONFIG_SENSORS_BMM050) || \
	defined(CONFIG_PROXIMITY_SENSOR) || defined(CONFIG_SENSORS_GP2A030) || defined(CONFIG_SENSORS_GYRO_K3G)
        sensor_gpio_init();
#endif

#if defined(CONFIG_TOUCHSCREEN_ZINITIX_VASTO) ||  defined(CONFIG_TOUCHSCREEN_ZINITIX_INFINITE)
	tsp_power_on();
#endif

#ifdef CONFIG_MAX17048_FUELGAUGE
	fg_max17048_gpio_init();
#endif

#ifdef CONFIG_MACH_DELOS_CTC
	if (board_hw_revision < 4) /* FAN54013 */
		fan54013_gpio_init();
	else						/* SMB328A */
		smb328a_gpio_init();
#else  /* else of CONFIG_MACH_DELOS_CTC */

#ifdef CONFIG_CHARGER_FAN54013
	fan54013_gpio_init();
#endif
#ifdef CONFIG_CHARGER_SMB328A
	smb328a_gpio_init();
#endif
#ifdef CONFIG_CHARGER_BQ24157
	bq24157_gpio_init();
#endif

#endif  /* end of CONFIG_MACH_DELOS_CTC */

	samsung_sys_class_init();

#if defined (CONFIG_TOUCHSCREEN_SYNAPTICS_S2200_I2C) || defined (CONFIG_TOUCHSCREEN_SYNAPTICS_DSX_I2C)
		i2c_register_board_info(5, synaptics_i2c_devices, ARRAY_SIZE(synaptics_i2c_devices)); //PSJ
#else
	i2c_register_board_info( 5, touch_i2c_devices, ARRAY_SIZE(touch_i2c_devices));
#endif
#ifdef CONFIG_INPUT_FLIP
		platform_device_register(&sec_flip_device);
		gpio_tlmm_config(GPIO_CFG(35, 0, GPIO_CFG_INPUT,
				GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
#endif
	i2c_register_board_info(3, fsa9480_i2c_devices,	ARRAY_SIZE(fsa9480_i2c_devices));

#ifdef CONFIG_BQ27425_FUEL_GAUGE
	i2c_register_board_info(6, fg_i2c_devices, ARRAY_SIZE(fg_i2c_devices));
#endif 
#ifdef CONFIG_MAX17048_FUELGAUGE
	i2c_register_board_info(6, fg_smb_i2c_devices, ARRAY_SIZE(fg_smb_i2c_devices));
#endif

#ifdef CONFIG_MACH_DELOS_CTC /* Delos CTC has 2 type of charger 02~03:FAN54013, 04~:smb328a */
	if (board_hw_revision < 4) /* FAN54013 */
	{
		i2c_register_board_info(MSM_CHARGER_I2C_BUS_ID, fan_charger_i2c_devices, ARRAY_SIZE(fan_charger_i2c_devices));
	}
	else /* smb328a */
	{
		i2c_register_board_info(MSM_CHARGER_I2C_BUS_ID, smb_charger_i2c_devices, ARRAY_SIZE(smb_charger_i2c_devices));
	}
#else  /* else of CONFIG_MACH_DELOS_CTC */ 

#ifdef CONFIG_CHARGER_FAN54013
	i2c_register_board_info(MSM_CHARGER_I2C_BUS_ID, fan_charger_i2c_devices, ARRAY_SIZE(fan_charger_i2c_devices));
#endif /* CONFIG_CHARGER_FAN54013 */

#ifdef CONFIG_CHARGER_BQ24157
	i2c_register_board_info(MSM_CHARGER_I2C_BUS_ID, charger_i2c_devices, ARRAY_SIZE(charger_i2c_devices));
#endif

#ifdef CONFIG_CHARGER_SMB328A
	i2c_register_board_info(MSM_CHARGER_I2C_BUS_ID, smb_charger_i2c_devices, ARRAY_SIZE(smb_charger_i2c_devices));
#endif

#endif  /* end of CONFIG_MACH_DELOS_CTC */ 

#if defined(CONFIG_SENSORS_BMA2X2) || defined(CONFIG_SENSORS_BMM050) || \
		defined(CONFIG_PROXIMITY_SENSOR) || defined(CONFIG_SENSORS_GP2A030) || defined(CONFIG_SENSORS_GYRO_K3G)
	i2c_register_board_info(4,sensor_devices, ARRAY_SIZE(sensor_devices));
#endif//CONFIG_SENSORS_BMA2X2

#ifdef CONFIG_KEYBOARD_STMPE1801
	i2c_register_board_info(8, stmpe1801_i2c_expansion, ARRAY_SIZE(stmpe1801_i2c_expansion));
#endif


#ifdef CONFIG_GPIOKEY_KYLEP
	platform_device_register(&kp_pdev);		
	platform_device_register(&hs_pdev);
#endif

	if (!kernel_uart_flag)	{
		platform_device_register(&msm_device_uart3);
	}

////////////////////////////////////////////////////////////////////////////
#if defined(CONFIG_MACH_DELOS_CTC)
	printk("%s board_hw_revision=  %d \n",__func__,board_hw_revision);
	aries_charger.charger_ic = &fan54013_pdata;
	aries_charger.charger_ic2 = &smb328a_pdata;
	if (board_hw_revision < 4)
		delos_charger_gpio_device = fan_charger_i2c_gpio_device;
	else
		delos_charger_gpio_device = smb_charger_i2c_gpio_device;
#endif

	msm_add_footswitch_devices();
	add_platform_devices();

	/* Ensure ar6000pm device is registered before MMC/SDC */
	msm_qrd_init_ar6000pm();
	msm7627a_init_mmc();

#ifdef CONFIG_USB_EHCI_MSM_72K
	msm7627a_init_host();
#endif
	msm_pm_init();

	msm_pm_register_irqs();
	msm_fb_add_devices();

	if (machine_is_qrd_skud_prime() || machine_is_msm8625q_evbd()
					|| machine_is_msm8625q_skud())
		i2c_register_board_info(2, i2c2_info,
				ARRAY_SIZE(i2c2_info));

	wlan_power_init();
	//msm7627a_bt_power_init();
	bt_power_init();

	msm7627a_camera_init();
	qrd7627a_add_io_devices();
	msm7x25a_kgsl_3d0_init();
	msm8x25_kgsl_3d0_init();
#if defined(CONFIG_TOUCHSCREEN_CYPRESS_TMA46X) 
	board_tsp_init();
#endif
}

static void __init qrd7627a_init_early(void)
{
	msm_msm7627a_allocate_memory_regions();
}

MACHINE_START(MSM7627A_QRD1, "QRD MSM7627a QRD1")
	.atag_offset	= 0x100,
	.map_io		= msm_common_io_init,
	.reserve	= msm7627a_reserve,
	.init_irq	= msm_init_irq,
	.init_machine	= msm_qrd_init,
	.timer		= &msm_timer,
	.init_early	= qrd7627a_init_early,
	.handle_irq	= vic_handle_irq,
MACHINE_END
MACHINE_START(MSM7627A_QRD3, "QRD MSM7627a QRD3")
	.atag_offset	= 0x100,
	.map_io		= msm_common_io_init,
	.reserve	= msm7627a_reserve,
	.init_irq	= msm_init_irq,
	.init_machine	= msm_qrd_init,
	.timer		= &msm_timer,
	.init_early	= qrd7627a_init_early,
	.handle_irq	= vic_handle_irq,
MACHINE_END
MACHINE_START(MSM7627A_EVB, "QRD MSM7627a EVB")
	.atag_offset	= 0x100,
	.map_io		= msm_common_io_init,
	.reserve	= msm7627a_reserve,
	.init_irq	= msm_init_irq,
	.init_machine	= msm_qrd_init,
	.timer		= &msm_timer,
	.init_early	= qrd7627a_init_early,
	.handle_irq	= vic_handle_irq,
MACHINE_END
MACHINE_START(MSM8625_EVB, "QRD MSM8625 EVB")
	.atag_offset	= 0x100,
	.map_io		= msm8625_map_io,
	.reserve	= msm8625_reserve,
	.init_irq	= msm8625_init_irq,
	.init_machine	= msm_qrd_init,
	.timer		= &msm_timer,
	.init_early	= qrd7627a_init_early,
	.handle_irq	= gic_handle_irq,
MACHINE_END
MACHINE_START(MSM8625_QRD7, "QRD MSM8625 QRD7")
	.atag_offset	= 0x100,
	.map_io		= msm8625_map_io,
	.reserve	= msm8625_reserve,
	.init_irq	= msm8625_init_irq,
	.init_machine	= msm_qrd_init,
	.timer		= &msm_timer,
	.init_early	= qrd7627a_init_early,
	.handle_irq	= gic_handle_irq,
MACHINE_END
MACHINE_START(MSM8625_EVT, "QRD MSM8625 EVT")
	.atag_offset	= 0x100,
	.map_io		= msm8625_map_io,
	.reserve	= msm8625_reserve,
	.init_irq	= msm8625_init_irq,
	.init_machine	= msm_qrd_init,
	.timer		= &msm_timer,
	.init_early	= qrd7627a_init_early,
	.handle_irq	= gic_handle_irq,
MACHINE_END
MACHINE_START(QRD_SKUD_PRIME, "QRD MSM8625 SKUD PRIME")
	.atag_offset	= 0x100,
	.map_io		= msm8625_map_io,
	.reserve	= msm8625_reserve,
	.init_irq	= msm8625_init_irq,
	.init_machine	= msm_qrd_init,
	.timer		= &msm_timer,
	.init_early	= qrd7627a_init_early,
	.handle_irq	= gic_handle_irq,
MACHINE_END
MACHINE_START(MSM8625Q_EVBD, "QRD MSM8625Q EVBD")
	.atag_offset	= 0x100,
	.map_io		= msm8625_map_io,
	.reserve	= msm8625_reserve,
	.init_irq	= msm8625_init_irq,
	.init_machine	= msm_qrd_init,
	.timer		= &msm_timer,
	.init_early	= qrd7627a_init_early,
	.handle_irq	= gic_handle_irq,
MACHINE_END
MACHINE_START(MSM8625Q_SKUD, "QRD MSM8625Q SKUD")
	.atag_offset	= 0x100,
	.map_io		= msm8625_map_io,
	.reserve	= msm8625_reserve,
	.init_irq	= msm8625_init_irq,
	.init_machine	= msm_qrd_init,
	.timer		= &msm_timer,
	.init_early	= qrd7627a_init_early,
	.handle_irq	= gic_handle_irq,
MACHINE_END
