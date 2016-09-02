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
#include <linux/memblock.h>
#include <asm/mach-types.h>
#include <linux/memblock.h>
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
#include <linux/usb/android.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <mach/vreg.h>
#include <mach/pmic.h>
#include <mach/socinfo.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <asm/mach/mmc.h>
#include <linux/i2c.h>
#include <linux/i2c/sx150x.h>
#include <linux/gpio.h>
#include <linux/android_pmem.h>
#include <linux/bootmem.h>
#include <linux/mfd/marimba.h>
#include <mach/vreg.h>
#include <linux/power_supply.h>
#include <linux/regulator/consumer.h>
#include <mach/rpc_pmapp.h>
#include <mach/msm_battery.h>
#include <linux/smsc911x.h>
#include <linux/atmel_maxtouch.h>
#include <linux/msm_adc.h>
#include <linux/ion.h>
#include <linux/dma-contiguous.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include "devices.h"
#include "timer.h"
#include "board-msm7x27a-regulator.h"
#include "devices-msm7x2xa.h"
#include "pm.h"
#include <mach/rpc_server_handset.h>
#include <mach/socinfo.h>
#include "pm-boot.h"
#include "board-msm7627a.h"
#ifdef CONFIG_SEC_DEBUG
#include <linux/sec_debug.h>
#endif
#define PMEM_KERNEL_EBI1_SIZE	0x3A000
#define MSM_PMEM_AUDIO_SIZE	0xF0000
#define BOOTLOADER_BASE_ADDR	0x10000

#ifndef CONFIG_BT_CSR_7820
#define CONFIG_BT_CSR_7820
#endif

#include <mach/gpio_roy.h>

#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/i2c/sx150x.h>

#ifdef CONFIG_SAMSUNG_JACK
#include <linux/sec_jack.h>
#endif

//#include <linux/fsaxxxx_usbsw.h>
#include <linux/platform_data/fsa9480.h>
#ifdef CONFIG_PROXIMITY_SENSOR
#include <linux/gp2a.h>
#endif

#ifdef CONFIG_TOUCHSCREEN_MELFAS
#include <linux/i2c/melfas_ts.h>
#endif

#ifdef CONFIG_MAX17048_FUELGAUGE
#include <linux/fuelgauge_max17048.h>
#endif

#ifdef CONFIG_BATTERY_STC3115
#include <linux/stc3115_battery.h>
#endif

#include "proc_comm.h"

#ifdef CONFIG_RMI4_I2C
#include <linux/interrupt.h>
#include <linux/rmi.h>
#include <mach/gpio.h>

#ifdef CONFIG_ISDBT
#include <media/isdbt_pdata.h>
#endif

#define SYNA_TM2303 0x00000800
#define SYNA_BOARDS SYNA_TM2303 /* (SYNA_TM1333 | SYNA_TM1414) */
#define SYNA_BOARD_PRESENT(board_mask) (SYNA_BOARDS & board_mask)

struct syna_gpio_data {
	u16 gpio_number;
	char* gpio_name;
};
#endif

#ifdef CONFIG_SENSORS_K2DH
#include <linux/k3dh.h>
#endif
#ifdef CONFIG_SENSORS_PX3215
#include <linux/px3215.h>
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


#ifdef CONFIG_SAMSUNG_JACK
#define GPIO_JACK_S_35	48
#define GPIO_SEND_END	92
#if defined(CONFIG_SAMSUNG_JACK_GNDLDET)
#define GPIO_GND_DET	18
#endif
#endif
#ifdef CONFIG_PROXIMITY_SENSOR
#define GPIO_PROX_INT 29
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
#if defined(CONFIG_SAMSUNG_JACK_GNDLDET)
	/* Active Low */
	return(gpio_get_value(GPIO_GND_DET)) ^ 1;
#else
	/* Active Low */
	return(gpio_get_value(GPIO_JACK_S_35)) ^ 1;
#endif
}
EXPORT_SYMBOL(get_msm7x27a_det_jack_state);

#if defined(CONFIG_SAMSUNG_JACK_GNDLDET)
int get_msm7x27a_L_det_jack_state(void)
{
	/* Active Low */
	return(gpio_get_value(GPIO_JACK_S_35)) ^ 1;
}
#endif

static int get_msm7x27a_send_key_state(void)
{
	/* Aruba use hssd module for send-end interrupt */
	return current_key_state;
}

#define SMEM_PROC_COMM_MICBIAS_ONOFF		PCOM_OEM_MICBIAS_ONOFF
#define SMEM_PROC_COMM_MICBIAS_ONOFF_REG5	PCOM_OEM_MICBIAS_ONOFF_REG5
#define SMEM_PROC_COMM_GET_ADC				PCOM_OEM_SAMSUNG_GET_ADC


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
	gpio_tlmm_config(GPIO_CFG(GPIO_JACK_S_35, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE); //gpio 48 JACK_INT_N
	if(gpio_request(GPIO_JACK_S_35, "h2w_detect")<0)
		pr_err("sec_jack:gpio_request fail\n");
	if(gpio_direction_input(GPIO_JACK_S_35)<0)
		pr_err("sec_jack:gpio_direction fail\n");
#if defined(CONFIG_SAMSUNG_JACK_GNDLDET)
	gpio_tlmm_config(GPIO_CFG(GPIO_GND_DET, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE); //gpio 18
	if(gpio_request(GPIO_GND_DET, "h2w_GND_detect")<0)
		pr_err("sec_jack: G-det gpio_request fail\n");
	if(gpio_direction_input(GPIO_GND_DET)<0)
		pr_err("sec_jack: G-det gpio_direction fail\n");
#endif
}

static struct sec_jack_platform_data sec_jack_data = {
#if defined(CONFIG_SAMSUNG_JACK_GNDLDET)
	.get_l_jack_state	= get_msm7x27a_L_det_jack_state,
#endif
	.get_det_jack_state	= get_msm7x27a_det_jack_state,
	.get_send_key_state	= get_msm7x27a_send_key_state,
	.set_micbias_state	= set_msm7x27a_micbias_state,
	.set_micbias_state_reg5	= set_msm7x27a_micbias_state_reg5,
	.get_adc_value	= sec_jack_get_adc_value,
	.zones		= jack_zones,
	.num_zones	= ARRAY_SIZE(jack_zones),
#if defined(CONFIG_SAMSUNG_JACK_GNDLDET)
	.det_int	= MSM_GPIO_TO_INT(GPIO_GND_DET),
#else
	.det_int	= MSM_GPIO_TO_INT(GPIO_JACK_S_35),
#endif
	.send_int	= MSM_GPIO_TO_INT(GPIO_SEND_END),
};

static struct platform_device sec_device_jack = {
	.name           = "sec_jack",
	.id             = -1,
	.dev            = {
		.platform_data  = &sec_jack_data,
	},
};


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

#if defined(CONFIG_ISDBT)
#define GPIO_ISDBT_RST_N	30
#define GPIO_ISDBT_EN		58
#define GPIO_ISDBT_INT		82
#define GPIO_ISDBT_IRQ		MSM_GPIO_TO_INT(GPIO_ISDBT_INT)

#if defined(CONFIG_ISDBT_ANT_DET)
#if (!defined(CONFIG_MACH_ROY_DTV_HWREV)) || (CONFIG_MACH_ROY_DTV_HWREV==0x0)
#define GPIO_ISDBT_DET		115
#else
#define GPIO_ISDBT_DET		107
#endif

#define GPIO_ISDBT_DET_IRQ		MSM_GPIO_TO_INT(GPIO_ISDBT_DET)
#endif  // CONFIG_ISDBT_ANT_DET


#define GPIO_ISDBT_EBI2_ADDR0	121
#define GPIO_ISDBT_EBI2_ADDR1	120


#define GPIO_LEVEL_HIGH		1
#define GPIO_LEVEL_LOW		0

static void isdbt_set_config_poweron(void)
{
	gpio_tlmm_config(GPIO_CFG(GPIO_ISDBT_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_set_value(GPIO_ISDBT_EN, GPIO_LEVEL_LOW);


	gpio_tlmm_config(GPIO_CFG(GPIO_ISDBT_RST_N, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_set_value(GPIO_ISDBT_RST_N, GPIO_LEVEL_LOW);

	// ROY_LTN_DTV, set GPIO_ISDBT_INT
	gpio_tlmm_config(GPIO_CFG(GPIO_ISDBT_INT, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

#if defined(CONFIG_ISDBT_ANT_DET)
	gpio_tlmm_config(GPIO_CFG(GPIO_ISDBT_DET, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
#endif

#if CONFIG_DTV_USES_HPI_MODE
	// ROY_LTN_DTV, GPIO_126/GPIO_127(EBI2 addr 0/1) should be configured as EBI2 port if we uses HPI mode
	
	gpio_tlmm_config(GPIO_CFG(GPIO_ISDBT_EBI2_ADDR0, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(GPIO_ISDBT_EBI2_ADDR1, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
#endif

}
static void isdbt_set_config_poweroff(void)
{
	gpio_tlmm_config(GPIO_CFG(GPIO_ISDBT_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_set_value(GPIO_ISDBT_EN, GPIO_LEVEL_LOW);

	gpio_tlmm_config(GPIO_CFG(GPIO_ISDBT_RST_N, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_set_value(GPIO_ISDBT_RST_N, GPIO_LEVEL_LOW);

	gpio_tlmm_config(GPIO_CFG(GPIO_ISDBT_INT, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_set_value(GPIO_ISDBT_INT, GPIO_LEVEL_LOW);


}

static void isdbt_gpio_on(void)
{
	isdbt_set_config_poweron();

	gpio_set_value(GPIO_ISDBT_EN, GPIO_LEVEL_LOW);
	msleep(1000); // usleep_range(1000, 1000);
	gpio_set_value(GPIO_ISDBT_EN, GPIO_LEVEL_HIGH);

	msleep(1000); // usleep_range(1000, 1000);
	gpio_set_value(GPIO_ISDBT_RST_N, GPIO_LEVEL_LOW);
	msleep(2000); //usleep_range(2000, 2000);
	gpio_set_value(GPIO_ISDBT_RST_N, GPIO_LEVEL_HIGH);
	msleep(1000); //usleep_range(1000, 1000);

}

static void isdbt_gpio_off(void)
{
	isdbt_set_config_poweroff();

	gpio_set_value(GPIO_ISDBT_RST_N, GPIO_LEVEL_LOW);
	msleep(1000); //usleep_range(1000, 1000);

	gpio_set_value(GPIO_ISDBT_EN, GPIO_LEVEL_LOW);
}

static struct isdbt_platform_data isdbt_pdata = {
	.gpio_on = isdbt_gpio_on,
	.gpio_off = isdbt_gpio_off,
};

static struct platform_device isdbt_device = {
	.name			= "isdbt",
	.id				= -1,
	.dev			= {
		.platform_data = &isdbt_pdata,
	},
};

static int __init isdbt_dev_init(void)
{

#if defined(CONFIG_ISDBT_ANT_DET)
	unsigned int isdbt_ant_det_gpio;
	unsigned int isdbt_ant_det_irq;

	isdbt_pdata.gpio_ant_det = GPIO_ISDBT_DET;
	isdbt_pdata.irq_ant_det = GPIO_ISDBT_DET_IRQ;	
#endif
	
	isdbt_set_config_poweroff();
	// s5p_register_gpio_interrupt(GPIO_ISDBT_INT);
	isdbt_pdata.irq = GPIO_ISDBT_IRQ;
	platform_device_register(&isdbt_device);

	printk(KERN_ERR "isdbt_dev_init\n");

	return 0;
}

#endif



#if defined(CONFIG_SENSORS_BMA2X2) \
	|| defined(CONFIG_PROXIMITY_SENSOR) || defined(CONFIG_SENSORS_BMM050) \
	|| defined(CONFIG_SENSORS_BMA222E) || defined(CONFIG_SENSORS_BMA222) \
	|| defined(CONFIG_SENSORS_HSCDTD006A) || defined(CONFIG_SENSORS_HSCDTD008A) \
	|| defined(CONFIG_SENSORS_PX3215) || defined(CONFIG_SENSORS_K2DH)
void sensor_gpio_init(void)
{
        printk("sensor gpio init!!");
	gpio_tlmm_config(GPIO_CFG(GPIO_SENSOR_SCL, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_PULL_UP, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(GPIO_SENSOR_SDA, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_PULL_UP, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
#ifdef CONFIG_PROXIMITY_SENSOR
        gpio_tlmm_config(GPIO_CFG(GPIO_PROX_INT,0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
#endif
}
#endif

#ifdef CONFIG_PROXIMITY_SENSOR
static int gp2a_power(bool on)
{
#if defined(CONFIG_MACH_ARUBA_OPEN)
        pmic_gpio_direction_output(PMIC_GPIO_11);
	if (on) {
		pr_err("%s gpio set to 1 \n", __func__);
                pmic_gpio_set_value(PMIC_GPIO_11,1);
	} else {
		pr_err("%s gpio set to 0 \n", __func__);
                pmic_gpio_set_value(PMIC_GPIO_11,0);
        }
#elif defined(CONFIG_MACH_ROY)&&defined(CONFIG_MACH_NEVIS3G)
     gpio_tlmm_config(GPIO_CFG(82,0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
     if (on) {
     	pr_err("%s gpio set to 1 ", __func__);
     	gpio_set_value(82, 1);
     } else {
     	pr_err("%s gpio set to 0 ", __func__);
     	gpio_set_value(82, 0);
     	}
#elif defined(CONFIG_MACH_ROY)
	// common sensor power. do nothing.

#else
	gpio_tlmm_config(GPIO_CFG(82,0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (on) {
		pr_err("%s gpio set to 1 ", __func__);
		gpio_set_value(82, 1);
	} else {
		pr_err("%s gpio set to 0 ", __func__);
		gpio_set_value(82, 0);
        }
#endif
	printk("End of gp2a function\n");
	return 0;
}
static struct gp2a_platform_data gp2a_pdata = {
	.p_out =29,
	.power =gp2a_power,
};
#endif//CONFIG_PROXIMITY_SENSOR

#if defined(CONFIG_SENSORS_PX3215)
static int px3215_setup(void);

static struct px3215_platform_data px3215_plat_data = {
	.ps_vout_gpio	= 29,
	.hw_setup	= px3215_setup,
};

static int px3215_setup(void)
{
	int err;

	/* Configure the GPIO for the interrupt */
	err = gpio_request(px3215_plat_data.ps_vout_gpio, "PS_VOUT");
	if (err < 0) {
		pr_err("PS_VOUT: failed to request GPIO %d,"
			" err %d\n", px3215_plat_data.ps_vout_gpio, err);

		goto err1;
	}

	err = gpio_tlmm_config(
	GPIO_CFG(px3215_plat_data.ps_vout_gpio,0,
			 GPIO_CFG_INPUT,
			 GPIO_CFG_PULL_UP,
			 GPIO_CFG_2MA),
		GPIO_CFG_ENABLE);
	if (err < 0) {
		pr_err("PS_VOUT: failed to configure gpio_tlmm_config"
			" GPIO %d, err %d\n",
			px3215_plat_data.ps_vout_gpio, err);

		goto err2;
	}

	return 0;

err2:
	gpio_free(px3215_plat_data.ps_vout_gpio);
err1:
	return err;
}
#endif

#ifdef CONFIG_SENSORS_K2DH
static int k3dh_int_init(void)
{
	int err;

	/* Configure the GPIO for the interrupt */
	err = gpio_request(GPIO_ACC_INT, "acc_int");
	if (err < 0) {
		pr_err("acc_int: failed to request GPIO %d,"
			" err %d\n", GPIO_ACC_INT, err);

		goto err1;
	}

	err = gpio_tlmm_config(
	GPIO_CFG(GPIO_ACC_INT,0,
			 GPIO_CFG_INPUT,
			 GPIO_CFG_PULL_DOWN,
			 GPIO_CFG_2MA),
		GPIO_CFG_ENABLE);
	if (err < 0) {
		pr_err("acc_int: failed to configure gpio_tlmm_config"
			" GPIO %d, err %d\n",
			GPIO_ACC_INT, err);

		goto err2;
	}

	return 0;

err2:
	gpio_free(GPIO_ACC_INT);
err1:
	return err;
}

static struct k3dh_platform_data k3dh_pdata = {
	.gpio_acc_int =GPIO_ACC_INT,
	.int_init = k3dh_int_init,
};
#endif

#if defined(CONFIG_SENSORS_BMA2X2) \
	|| defined(CONFIG_PROXIMITY_SENSOR) || defined(CONFIG_SENSORS_BMM050) \
	|| defined(CONFIG_SENSORS_BMA222E) || defined(CONFIG_SENSORS_BMA222) \
	|| defined(CONFIG_SENSORS_HSCDTD006A) || defined(CONFIG_SENSORS_HSCDTD008A) \
	|| defined(CONFIG_SENSORS_PX3215) || defined(CONFIG_SENSORS_K2DH)
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
#if defined(CONFIG_SENSORS_HSCDTD006A) || defined(CONFIG_SENSORS_HSCDTD008A)
{
	I2C_BOARD_INFO("hscd_i2c", 0x0c),
},
#endif
#if defined(CONFIG_SENSORS_BMA2X2) || defined(CONFIG_SENSORS_BMM050)
{
	I2C_BOARD_INFO("bma2x2", 0x10),
},
{
	I2C_BOARD_INFO("bmm050", 0x12),
},
#endif
#ifdef CONFIG_SENSORS_K2DH
{
	I2C_BOARD_INFO("k3dh", 0x19),
	.platform_data = &k3dh_pdata,
},
#endif
#if defined(CONFIG_PROXIMITY_SENSOR)
{
	I2C_BOARD_INFO("gp2a", 0x44 ),
	.platform_data = &gp2a_pdata,
	},
#endif
#ifdef CONFIG_SENSORS_PX3215
{
	I2C_BOARD_INFO("dyna", 0x1e ),
	.platform_data = &px3215_plat_data,
},
#endif
};
#endif

static struct i2c_gpio_platform_data touch_i2c_gpio_data = {
	.sda_pin    = GPIO_TSP_SDA,
	.scl_pin    = GPIO_TSP_SCL,
	.udelay	= 1,
};
static struct platform_device touch_i2c_gpio_device = {
	.name       = "i2c-gpio",
	.id     =  2,
	.dev        = {
		.platform_data  = &touch_i2c_gpio_data,
	},
};

#if defined(CONFIG_TOUCHSCREEN_MELFAS_KYLE) 
/* I2C 2 */
static struct i2c_board_info touch_i2c_devices[] = {
	{
		I2C_BOARD_INFO("sec_touch", 0x48),
	        .irq = MSM_GPIO_TO_INT( GPIO_TOUCH_IRQ ),
	},
};

#endif

#if defined (CONFIG_RMI4_I2C)
#if SYNA_BOARD_PRESENT(SYNA_TM2303)
	/* tm2303 has four buttons.
	 */

#define TOUCH_ON  1
#define TOUCH_OFF 0
#define AXIS_ALIGNMENT { }

#define TM2303_ADDR 0x20
#define TM2303_ATTN 27												// please change this!!!

static unsigned char tm2303_f1a_button_codes[] = {KEY_MENU, KEY_BACK};


static int s2200_ts_power(int on_off)
{
	int retval;

	retval = gpio_request(GPIO_TOUCH_EN, "touch_en");
	if (retval) {
		pr_err("%s: Failed to acquire power GPIO, code = %d.\n",
			 __func__, retval);
		return retval;
	}

	if (on_off == TOUCH_ON) {
		retval = gpio_direction_output(GPIO_TOUCH_EN,1);
		if (retval) {
			pr_err("%s: Failed to set power GPIO to 1, code = %d.\n",
				__func__, retval);
			return retval;
	}
		gpio_set_value(GPIO_TOUCH_EN,1);
	} else {
		retval = gpio_direction_output(GPIO_TOUCH_EN,0);
		if (retval) {
			pr_err("%s: Failed to set power GPIO to 0, code = %d.\n",
				__func__, retval);
			return retval;
		}
		gpio_set_value(GPIO_TOUCH_EN,0);
	}

	gpio_free(GPIO_TOUCH_EN);
	msleep(230);

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

	return s2200_ts_power(configure);
}

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
	.gpio_number = 27,
	.gpio_name = "sdmmc2_clk.gpio_130",
};

static struct rmi_device_platform_data tm2303_platformdata = {
	.driver_name = "rmi_generic",
	.attn_gpio = 27,
	.attn_polarity = RMI_ATTN_ACTIVE_LOW,
	.reset_delay_ms = 200,
	.gpio_data = &tm2303_gpiodata,
	.gpio_config = synaptics_touchpad_gpio_setup,
	.axis_align = AXIS_ALIGNMENT,
	.f1a_button_map = &tm2303_f1a_button_map,
	.post_suspend = tm2303_post_suspend,
	.pre_resume = tm2303_pre_resume,
	.f11_type_b = true,	
};
static struct i2c_board_info synaptics_i2c_devices[] = {
	{
		I2C_BOARD_INFO("rmi_i2c", TM2303_ADDR),
		.platform_data = &tm2303_platformdata,
	},
};
#endif

static void tsp_power_on(void)
{
	int rc = 0;

	rc = gpio_request(GPIO_TOUCH_EN, "touch_en");

	if (rc < 0) {
		pr_err("failed to request touch_en\n");
	}

	gpio_tlmm_config(GPIO_CFG(GPIO_TOUCH_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_direction_output(GPIO_TOUCH_EN, 0);
	gpio_free(GPIO_TOUCH_EN);

}
#endif

#define CONFIG_GPIOKEY_KYLEP	// jjLee

#ifdef CONFIG_GPIOKEY_KYLEP

#define GPIO_MUSB_INT_ARUBA 28
#if defined(CONFIG_MACH_ROY_DTV)
// ROY_LTN_DTV(GT-S6313T) uses UART1B, so MUS_SCL/SDA pins are changed to GPIO 131/132
#define GPIO_MUS_SCL_ARUBA 131
#define GPIO_MUS_SDA_ARUBA 132
#else
#define GPIO_MUS_SCL_ARUBA 123
#define GPIO_MUS_SDA_ARUBA 122
#endif

#define KP_INDEX(row, col) ((row)*ARRAY_SIZE(kp_col_gpios) + (col))

static unsigned int kp_row_gpios[] = {36, 37, 39};
static unsigned int kp_col_gpios[] = {31};
static unsigned int kp_wakeup_gpios[] = {37};

static const unsigned short keymap[ARRAY_SIZE(kp_col_gpios) *
					  ARRAY_SIZE(kp_row_gpios)] = {
	[KP_INDEX(0, 0)] = KEY_VOLUMEDOWN,
	[KP_INDEX(1, 0)] = KEY_HOME,
	[KP_INDEX(2, 0)] = KEY_VOLUMEUP,
};

/* SURF keypad platform device information */
static struct gpio_event_matrix_info kp_matrix_info = {
	.info.func	= gpio_event_matrix_func,
	.keymap		= keymap,
	.output_gpios	= kp_col_gpios,
	.input_gpios	= kp_row_gpios,
	.wakeup_gpios	= kp_wakeup_gpios,
	.nwakeups	= ARRAY_SIZE(kp_wakeup_gpios),
	.noutputs	= ARRAY_SIZE(kp_col_gpios),
	.ninputs	= ARRAY_SIZE(kp_row_gpios),
//	.settle_time.tv_nsec = 40 * NSEC_PER_USEC,
//	.poll_time.tv_nsec = 20 * NSEC_PER_MSEC,
//	.debounce_delay.tv_nsec = 20 * NSEC_PER_MSEC,
	.flags		= GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_DRIVE_INACTIVE |
			  GPIOKPF_PRINT_UNMAPPED_KEYS | GPIOKPF_DEBOUNCE,
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



static u32 msm_calculate_batt_capacity(u32 current_voltage);

static struct msm_charger_data aries_charger = {
	.register_callbacks	= msm_battery_register_callback,
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
#if !defined(CONFIG_MACH_NEVIS3G)
       .ovp_cb		= jena_ovp_cb,
#else 
	.ovp_cb		= NULL, /* NEVIS dosen't use FSA' OVP */
#endif
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
	gpio_tlmm_config(GPIO_CFG(GPIO_MUSB_INT_ARUBA,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
//	gpio_tlmm_config(GPIO_CFG(GPIO_MUS_SDA_ARUBA,  0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),GPIO_CFG_DISABLE);
//	gpio_tlmm_config(GPIO_CFG(GPIO_MUS_SCL_ARUBA,  0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),GPIO_CFG_DISABLE);

//	mdelay(100); 
	gpio_tlmm_config(GPIO_CFG(GPIO_MUS_SDA_ARUBA,  0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(GPIO_MUS_SCL_ARUBA,  0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),GPIO_CFG_ENABLE);

//	gpio_tlmm_config(GPIO_CFG(GPIO_MUS_SDA_ARUBA,  0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),0);
//	gpio_tlmm_config(GPIO_CFG(GPIO_MUS_SCL_ARUBA,  0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),0);
	
}

#ifdef CONFIG_MAX17048_FUELGAUGE
#define FUEL_I2C_SCL 78
#define FUEL_I2C_SDA 79


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
#ifdef CONFIG_CHARGER_SMB328A
    {
            I2C_BOARD_INFO("smb328a", (0x69 >> 1)),
            .platform_data = &smb328a_pdata,
            .irq = MSM_GPIO_TO_INT(18),
    },
#endif

};

static void __init fg_max17048_gpio_init(void)
{
	gpio_tlmm_config(GPIO_CFG(FUEL_I2C_SDA, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_PULL_UP, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(FUEL_I2C_SCL, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_PULL_UP, GPIO_CFG_2MA),GPIO_CFG_ENABLE);	
}
#endif

#ifdef CONFIG_BATTERY_STC3115

#define FUEL_I2C_SDA		79
#define FUEL_I2C_SCL		78

static struct i2c_gpio_platform_data fuelgauge_i2c_gpio_data = {
	.sda_pin		= FUEL_I2C_SDA,
	.scl_pin		= FUEL_I2C_SCL,
	.udelay			= 2,
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device stc_fuelgauge_i2c_gpio_device = {
	.name			= "i2c-gpio",
	.id			= 6,
	.dev	= {
	.platform_data	= &fuelgauge_i2c_gpio_data,
	},
};

static void __init fg_STC3115_gpio_init(void)
{
	gpio_tlmm_config(GPIO_CFG(FUEL_I2C_SDA, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_PULL_UP, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(FUEL_I2C_SCL, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_PULL_UP, GPIO_CFG_2MA),GPIO_CFG_ENABLE);	
}

int null_fn(void)
{
        return 0;                // for discharging status
}

int Temperature_fn(void)
{
	return (25);
}

static int stc_power_supply_register(struct device *parent,
	struct power_supply *psy)
{
	aries_charger.psy_fuelgauge = psy;
	return 0;
}

static void stc_power_supply_unregister(struct power_supply *psy)
{
	aries_charger.psy_fuelgauge = NULL;
}

static struct stc311x_platform_data stc3115_data = {
                .battery_online = NULL,
                .charger_online = null_fn, 		// used in stc311x_get_status()
                .charger_enable = null_fn,		// used in stc311x_get_status()
                .power_supply_register = stc_power_supply_register,
                .power_supply_unregister = stc_power_supply_unregister,

		.Vmode= 0,       /*REG_MODE, BIT_VMODE 1=Voltage mode, 0=mixed mode */
  		.Alm_SOC = 10,      /* SOC alm level %*/
  		.Alm_Vbat = 3600,   /* Vbat alm level mV*/
  		.CC_cnf = 257,      /* nominal CC_cnf, coming from battery characterisation*/
  		.VM_cnf = 261,      /* nominal VM cnf , coming from battery characterisation*/
  		.Cnom = 1300,       /* nominal capacity in mAh, coming from battery characterisation*/
  		.Rsense = 10,       /* sense resistor mOhms*/
  		.RelaxCurrent = 65, /* current for relaxation in mA (< C/20) */
  		.Adaptive = 1,     /* 1=Adaptive mode enabled, 0=Adaptive mode disabled */

		.CapDerating[6] = 240,   /* capacity derating in 0.1%, for temp = -20°C */
  		.CapDerating[5] = 120,   /* capacity derating in 0.1%, for temp = -10°C */
		.CapDerating[4] = 90,   /* capacity derating in 0.1%, for temp = 0°C */
		.CapDerating[3] = 30,   /* capacity derating in 0.1%, for temp = 10°C */
		.CapDerating[2] = -20,   /* capacity derating in 0.1%, for temp = 25°C */
		.CapDerating[1] = -40,   /* capacity derating in 0.1%, for temp = 40°C */
		.CapDerating[0] = -50,   /* capacity derating in 0.1%, for temp = 60°C */

  		.OCVOffset[15] = 35,    /* OCV curve adjustment */
		.OCVOffset[14] = 22,   /* OCV curve adjustment */
		.OCVOffset[13] = 19,    /* OCV curve adjustment */
		.OCVOffset[12] = 10,    /* OCV curve adjustment */
		.OCVOffset[11] = 0,    /* OCV curve adjustment */
		.OCVOffset[10] = 10,    /* OCV curve adjustment */
		.OCVOffset[9] = 25,     /* OCV curve adjustment */
		.OCVOffset[8] = 3,      /* OCV curve adjustment */
		.OCVOffset[7] = -2,      /* OCV curve adjustment */
		.OCVOffset[6] = 4,    /* OCV curve adjustment */
		.OCVOffset[5] = 19,    /* OCV curve adjustment */
		.OCVOffset[4] = 44,     /* OCV curve adjustment */
		.OCVOffset[3] = 23,    /* OCV curve adjustment */
		.OCVOffset[2] = 28,     /* OCV curve adjustment */
		.OCVOffset[1] = 127,    /* OCV curve adjustment */
		.OCVOffset[0] = 40,     /* OCV curve adjustment */
		
		.OCVOffset2[15] = 17,    /* OCV curve adjustment */
		.OCVOffset2[14] = 14,   /* OCV curve adjustment */
		.OCVOffset2[13] = 10,    /* OCV curve adjustment */
		.OCVOffset2[12] = 3,    /* OCV curve adjustment */
		.OCVOffset2[11] = 5,    /* OCV curve adjustment */
		.OCVOffset2[10] = 20,    /* OCV curve adjustment */
		.OCVOffset2[9] = 1,     /* OCV curve adjustment */
		.OCVOffset2[8] = -2,      /* OCV curve adjustment */
		.OCVOffset2[7] = 7,      /* OCV curve adjustment */
		.OCVOffset2[6] = 20,    /* OCV curve adjustment */
		.OCVOffset2[5] = 36,    /* OCV curve adjustment */
		.OCVOffset2[4] = 26,     /* OCV curve adjustment */
		.OCVOffset2[3] = 19,    /* OCV curve adjustment */
		.OCVOffset2[2] = 77,     /* OCV curve adjustment */
		.OCVOffset2[1] = 126,    /* OCV curve adjustment */
		.OCVOffset2[0] = 0,     /* OCV curve adjustment */

			/*if the application temperature data is preferred than the STC3115 temperature*/
  		.ExternalTemperature = Temperature_fn, /*External temperature fonction, return °C*/
  		.ForceExternalTemperature = 0, /* 1=External temperature, 0=STC3115 temperature */
		
};



static struct i2c_board_info stc_i2c2_boardinfo[] = {

	{
		I2C_BOARD_INFO("stc3115", 0x70),
		.platform_data = &stc3115_data, //MSH AM
	},

};
#endif



/////////////////////////////////////////////////////////////////////////////////


#if defined(CONFIG_GPIO_SX150X)
enum {
	SX150X_CORE,
};

static struct sx150x_platform_data sx150x_data[] __initdata = {
	[SX150X_CORE]	= {
		.gpio_base		= GPIO_CORE_EXPANDER_BASE,
		.oscio_is_gpo		= false,
		.io_pullup_ena		= 0,
		.io_pulldn_ena		= 0x02,
		.io_open_drain_ena	= 0xfef8,
		.irq_summary		= -1,
	},
};
#endif

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

#if defined(CONFIG_I2C) && defined(CONFIG_GPIO_SX150X)
static struct i2c_board_info core_exp_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("sx1509q", 0x3e),
	},
};

static void __init register_i2c_devices(void)
{
	if (machine_is_msm7x27a_surf() || machine_is_msm7625a_surf() ||
			machine_is_msm8625_surf())
		sx150x_data[SX150X_CORE].io_open_drain_ena = 0xe0f0;

	core_exp_i2c_info[0].platform_data =
			&sx150x_data[SX150X_CORE];

	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
				core_exp_i2c_info,
				ARRAY_SIZE(core_exp_i2c_info));
#if defined(CONFIG_SENSORS_BMA2X2) \
	|| defined(CONFIG_PROXIMITY_SENSOR) || defined(CONFIG_SENSORS_BMM050) \
	|| defined(CONFIG_SENSORS_BMA222E) || defined(CONFIG_SENSORS_BMA222) \
	|| defined(CONFIG_SENSORS_HSCDTD006A) || defined(CONFIG_SENSORS_HSCDTD008A) \
	|| defined(CONFIG_SENSORS_PX3215) || defined(CONFIG_SENSORS_K2DH)
	i2c_register_board_info(4,sensor_devices, ARRAY_SIZE(sensor_devices));
#endif
}
#endif

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
	.clk_freq		= 400000,
	.msm_i2c_config_gpio	= gsbi_qup_i2c_gpio_config,
};

static struct msm_i2c_platform_data msm_gsbi1_qup_i2c_pdata = {
	.clk_freq		= 100000,
	.msm_i2c_config_gpio	= gsbi_qup_i2c_gpio_config,
};

#ifdef CONFIG_ARCH_MSM7X27A
#define MSM_PMEM_MDP_SIZE       0x2300000
#define MSM7x25A_MSM_PMEM_MDP_SIZE       0x1500000

#define MSM_PMEM_ADSP_SIZE      0x1300000
#define MSM7x25A_MSM_PMEM_ADSP_SIZE      0xB91000
#define CAMERA_ZSL_SIZE		(SZ_1M * 60)
#endif

#ifdef CONFIG_ION_MSM
#define MSM_ION_HEAP_NUM        5
static struct platform_device ion_dev;
static int msm_ion_camera_size;
static int msm_ion_audio_size;
static int msm_ion_sf_size;
static int msm_ion_camera_size_carving;
#endif

#define CAMERA_HEAP_BASE	0x0
#ifdef CONFIG_CMA
#define CAMERA_HEAP_TYPE	ION_HEAP_TYPE_DMA
#else
#define CAMERA_HEAP_TYPE	ION_HEAP_TYPE_CARVEOUT
#endif

static struct android_usb_platform_data android_usb_pdata = {
	.update_pid_and_serial_num = usb_diag_update_pid_and_serial_num,
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

	gpio = GPIO_HOST_VBUS_EN;

	rc = gpio_request(gpio, "i2c_host_vbus_en");
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

static void __init msm7x2x_init_host(void)
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
	.drv_ampl		 = HS_DRV_AMPLITUDE_75_PERCENT,
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
};

static struct resource smc91x_resources[] = {
	[0] = {
		.start = 0x90000300,
		.end   = 0x900003ff,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = MSM_GPIO_TO_INT(4),
		.end   = MSM_GPIO_TO_INT(4),
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device smc91x_device = {
	.name           = "smc91x",
	.id             = 0,
	.num_resources  = ARRAY_SIZE(smc91x_resources),
	.resource       = smc91x_resources,
};

#define WLAN_HOST_WAKE


#ifdef WLAN_HOST_WAKE
struct wlansleep_info {
	unsigned host_wake;
	unsigned host_wake_irq;
	struct wake_lock wake_lock;
};


static struct wlansleep_info *wsi;
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

	wake_lock_init(&wsi->wake_lock, WAKE_LOCK_SUSPEND, "bluesleep");
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

		/* Keep GPIO_WLAN_33V_EN on if either BT or Wifi is turned on*/
		if (temp_flag)
			skip = 1;
	}

	printk(KERN_INFO "%s - new wlan_33v_flag : %d\n",
			__func__, temp_flag);

	if (skip) {
		printk(KERN_INFO "%s - Skip GPIO_WLAN_33V_EN %s\n",
				__func__, on ? "on" : "off");
	} else {
		/* GPIO_WLAN_33V_EN - On / Off */
		if (wlan_set_gpio(GPIO_WLAN_33V_EN, on))
			return WLAN_ERROR;
	}

	wlan_33v_flag = temp_flag;

	return WLAN_OK;
}
#endif

void wlan_setup_power(int on, int detect)
{
	printk("%s %s --enter\n", __func__, on ? "on" : "down");

	if (on) {
#ifdef WLAN_33V_CONTROL_FOR_BT_ANTENNA
		/* GPIO_WLAN_33V_EN - On */
		if (wlan_setup_ldo_33v(WLAN_33V_WIFI_FLAG, 1))
			return;
#endif

		udelay(60);

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
		/* GPIO_WLAN_33V_EN - Off */
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

	/* Set config - GPIO_WLAN_33V_EN */
	if (gpio_tlmm_config(GPIO_CFG(GPIO_WLAN_33V_EN, 0,
			GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP,
			GPIO_CFG_2MA), GPIO_CFG_ENABLE)) {
		printk(KERN_ERR "%s: gpio_tlmm_config for %d failed\n",
				__func__, GPIO_WLAN_33V_EN);
		return WLAN_ERROR;
	}
#endif

	return WLAN_OK;
}

#ifdef CONFIG_SERIAL_MSM_HS
static struct msm_serial_hs_platform_data msm_uart_dm1_pdata = {
	.inject_rx_on_wakeup	= 1,
	.rx_to_inject		= 0xFD,
};
#endif
static struct msm_pm_platform_data msm7x27a_pm_data[MSM_PM_SLEEP_MODE_NR] = {
	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_POWER_COLLAPSE)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 16000,
					.residency = 20000,
	},
	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 12000,
					.residency = 20000,
	},
	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 0,
					.suspend_enabled = 1,
					.latency = 2000,
					.residency = 0,
	},
	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT)] = {
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
static struct msm_pm_platform_data msm8625_pm_data[MSM_PM_SLEEP_MODE_NR * 2] = {
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
					.idle_enabled = 0,
					.suspend_enabled = 0,
					.latency = 500,
					.residency = 6000,
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
					.idle_enabled = 0,
					.suspend_enabled = 0,
					.latency = 500,
					.residency = 6000,
	},

	[MSM_PM_MODE(1, MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT)] = {
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

static struct smsc911x_platform_config smsc911x_config = {
	.irq_polarity	= SMSC911X_IRQ_POLARITY_ACTIVE_HIGH,
	.irq_type	= SMSC911X_IRQ_TYPE_PUSH_PULL,
	.flags		= SMSC911X_USE_16BIT,
};

static struct resource smsc911x_resources[] = {
	[0] = {
		.start	= 0x90000000,
		.end	= 0x90007fff,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= MSM_GPIO_TO_INT(48),
		.end	= MSM_GPIO_TO_INT(48),
		.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL,
	},
};

static struct platform_device smsc911x_device = {
	.name		= "smsc911x",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(smsc911x_resources),
	.resource	= smsc911x_resources,
	.dev		= {
		.platform_data	= &smsc911x_config,
	},
};

static struct msm_gpio smsc911x_gpios[] = {
	{ GPIO_CFG(48, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_6MA),
							 "smsc911x_irq"  },
	{ GPIO_CFG(49, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_6MA),
							 "eth_fifo_sel" },
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

#define ETH_FIFO_SEL_GPIO	49
static void msm7x27a_cfg_smsc911x(void)
{
	int res;

	res = msm_gpios_request_enable(smsc911x_gpios,
				 ARRAY_SIZE(smsc911x_gpios));
	if (res) {
		pr_err("%s: unable to enable gpios for SMSC911x\n", __func__);
		return;
	}

	/* ETH_FIFO_SEL */
	res = gpio_direction_output(ETH_FIFO_SEL_GPIO, 0);
	if (res) {
		pr_err("%s: unable to get direction for gpio %d\n", __func__,
							 ETH_FIFO_SEL_GPIO);
		msm_gpios_disable_free(smsc911x_gpios,
						 ARRAY_SIZE(smsc911x_gpios));
		return;
	}
	gpio_set_value(ETH_FIFO_SEL_GPIO, 0);
}

#if defined(CONFIG_SERIAL_MSM_HSL_CONSOLE) \
		&& defined(CONFIG_MSM_SHARED_GPIO_FOR_UART2DM)
static struct msm_gpio uart2dm_gpios[] = {
	{GPIO_CFG(19, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
							"uart2dm_rfr_n" },
	{GPIO_CFG(20, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
							"uart2dm_cts_n" },
	{GPIO_CFG(21, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
							"uart2dm_rx"    },
	{GPIO_CFG(108, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
							"uart2dm_tx"    },
};

static void msm7x27a_cfg_uart2dm_serial(void)
{
	int ret;
	ret = msm_gpios_request_enable(uart2dm_gpios,
					ARRAY_SIZE(uart2dm_gpios));
	if (ret)
		pr_err("%s: unable to enable gpios for uart2dm\n", __func__);
}
#else
static void msm7x27a_cfg_uart2dm_serial(void) { }
#endif

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
static struct platform_device *rumi_sim_devices[] __initdata = {
	&msm_device_dmov,
	&msm_device_smd,
	&smc91x_device,
#ifndef CONFIG_MACH_ROY_DTV
	&msm_device_uart1,
#endif
	&msm_device_nand,
	&msm_device_uart_dm1,
	&msm_gsbi0_qup_i2c_device,
	&msm_gsbi1_qup_i2c_device,
};

static struct platform_device *msm8625_rumi3_devices[] __initdata = {
	&msm8625_device_dmov,
	&msm8625_device_smd,
	&msm8625_device_uart1,
	&msm8625_gsbi0_qup_i2c_device,
};

static struct platform_device *msm7627a_surf_ffa_devices[] __initdata = {
	&msm_device_dmov,
	&msm_device_smd,
#ifndef CONFIG_MACH_ROY_DTV
	&msm_device_uart1,
#endif	
	&msm_device_uart_dm1,
//	&msm_device_uart_dm2,
	&msm_gsbi0_qup_i2c_device,
	&msm_gsbi1_qup_i2c_device,
	&msm_device_otg,
	&msm_device_gadget_peripheral,
	&smsc911x_device,
	&msm_kgsl_3d0,
	&fsa9480_i2c_gpio_device,	
#ifdef CONFIG_SAMSUNG_JACK
	&sec_device_jack,
#endif
#if defined(CONFIG_MACH_ROY) 
	&touch_i2c_gpio_device,
#endif	
#ifdef CONFIG_MSM_VIBRATOR
	&msm_vibrator_device,
#endif
#if defined(CONFIG_SENSORS_BMA2X2) \
	|| defined(CONFIG_PROXIMITY_SENSOR) || defined(CONFIG_SENSORS_BMM050) \
	|| defined(CONFIG_SENSORS_BMA222E) || defined(CONFIG_SENSORS_BMA222) \
	|| defined(CONFIG_SENSORS_HSCDTD006A) || defined(CONFIG_SENSORS_HSCDTD008A) \
	|| defined(CONFIG_SENSORS_PX3215) || defined(CONFIG_SENSORS_K2DH)
	&sensor_i2c_gpio_device,
#endif
#ifdef CONFIG_BATTERY_STC3115
	&stc_fuelgauge_i2c_gpio_device,
#endif
};

static struct platform_device *common_devices[] __initdata = {
	&android_usb_device,
	&android_pmem_device,
	&android_pmem_adsp_device,
	&android_pmem_audio_device,
	&msm_device_nand,
	&msm_device_snd,
	&msm_device_adspdec,
	&msm_device_cad,
	&asoc_msm_pcm,
	&asoc_msm_dai0,
	&asoc_msm_dai1,
	&msm_batt_device,
	&msm_adc_device,
	&msm_bt_power_device_csr,
	&msm_bluesleep_device,
#ifdef CONFIG_ION_MSM
	&ion_dev,
#endif
};

static struct platform_device *msm8625_surf_devices[] __initdata = {
	&msm8625_device_dmov,
	&msm8625_device_uart1,
	&msm8625_device_uart_dm1,
	//&msm8625_device_uart_dm2,
	&msm8625_gsbi0_qup_i2c_device,
	&msm8625_gsbi1_qup_i2c_device,
	&msm8625_device_smd,
	&msm8625_device_otg,
	&msm8625_device_gadget_peripheral,
	&msm8625_kgsl_3d0,

// GSCHO
	&touch_i2c_gpio_device,
	&sensor_i2c_gpio_device,
	&fsa9480_i2c_gpio_device,
#ifdef CONFIG_MSM_VIBRATOR
	&msm_vibrator_device,
#endif
#ifdef CONFIG_SAMSUNG_JACK
	&sec_device_jack,
#endif
#ifdef CONFIG_MAX17048_FUELGAUGE
	&fg_smb_i2c_gpio_device,
#endif
#ifdef CONFIG_BATTERY_STC3115
	&stc_fuelgauge_i2c_gpio_device,
#endif
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
	if (machine_is_msm7625a_surf() || machine_is_msm7625a_ffa()) {
		pmem_mdp_size = MSM7x25A_MSM_PMEM_MDP_SIZE;
		pmem_adsp_size = MSM7x25A_MSM_PMEM_ADSP_SIZE;
	} else {
		pmem_mdp_size = MSM_PMEM_MDP_SIZE;
		pmem_adsp_size = MSM_PMEM_ADSP_SIZE;
	}

	if (get_ddr_size() > SZ_512M)
		pmem_adsp_size = CAMERA_ZSL_SIZE;
#ifdef CONFIG_ION_MSM
	msm_ion_camera_size = pmem_adsp_size;
	msm_ion_audio_size = MSM_PMEM_AUDIO_SIZE;
	msm_ion_sf_size = pmem_mdp_size;
#ifdef CONFIG_CMA
	msm_ion_camera_size_carving = 0;
#else
	msm_ion_camera_size_carving = msm_ion_camera_size;
#endif
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
struct ion_platform_heap msm7x27a_heaps[] = {
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
			.id	= ION_AUDIO_HEAP_BL_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_AUDIO_BL_HEAP_NAME,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *)&co_ion_pdata,
			.base = BOOTLOADER_BASE_ADDR,
		},
#endif
};

static struct ion_platform_data ion_pdata = {
	.nr = MSM_ION_HEAP_NUM,
	.has_outer_cache = 1,
	.heaps = msm7x27a_heaps,
};

static struct platform_device ion_dev = {
	.name = "ion-msm",
	.id = 1,
	.dev = { .platform_data = &ion_pdata },
};
#endif

static struct memtype_reserve msm7x27a_reserve_table[] __initdata = {
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
	msm7x27a_reserve_table[p->memory_type].size += p->size;
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

	msm7x27a_reserve_table[MEMTYPE_EBI1].size += pmem_kernel_ebi1_size;
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
	msm7x27a_reserve_table[MEMTYPE_EBI1].size += PMEM_KERNEL_EBI1_SIZE;
	msm7x27a_reserve_table[MEMTYPE_EBI1].size +=
		msm_ion_camera_size_carving;
	msm7x27a_reserve_table[MEMTYPE_EBI1].size += msm_ion_sf_size;
#endif
}

static void __init msm7x27a_calculate_reserve_sizes(void)
{
	fix_sizes();
	size_pmem_devices();
	reserve_pmem_memory();
	size_ion_devices();
	reserve_ion_memory();
}

static int msm7x27a_paddr_to_memtype(unsigned int paddr)
{
	return MEMTYPE_EBI1;
}

static struct reserve_info msm7x27a_reserve_info __initdata = {
	.memtype_reserve_table = msm7x27a_reserve_table,
	.calculate_reserve_sizes = msm7x27a_calculate_reserve_sizes,
	.paddr_to_memtype = msm7x27a_paddr_to_memtype,
};

static void __init msm7x27a_reserve(void)
{
	reserve_info = &msm7x27a_reserve_info;
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
	memblock_remove(MSM8625_CPU_PHYS, SZ_8);
	memblock_remove(MSM8625_WARM_BOOT_PHYS, SZ_32);
	memblock_remove(MSM8625_NON_CACHE_MEM, SZ_2K);
	msm7x27a_reserve();
}

static void __init msm7x27a_device_i2c_init(void)
{
	msm_gsbi0_qup_i2c_device.dev.platform_data = &msm_gsbi0_qup_i2c_pdata;
	msm_gsbi1_qup_i2c_device.dev.platform_data = &msm_gsbi1_qup_i2c_pdata;
}

static void __init msm8625_device_i2c_init(void)
{
	msm8625_gsbi0_qup_i2c_device.dev.platform_data =
		&msm_gsbi0_qup_i2c_pdata;
	msm8625_gsbi1_qup_i2c_device.dev.platform_data =
		&msm_gsbi1_qup_i2c_pdata;
}

#define MSM_EBI2_PHYS			0xa0d00000
#define MSM_EBI2_XMEM_CS2_CFG1		0xa0d10030


#ifdef CONFIG_ISDBT
#define MSM_EBI2_CFG_1				0xa0d00004
#define MSM_EBI2_ARBITER_CFG		0xa0d00030

#define	MSM_EBI2_XMEM_CS1_CFG0		0xa0d1000c
#define	MSM_EBI2_XMEM_CS1_CFG1		0xa0d1002c
#define MSM_EBI2_XMEM_DATA_CS1		0x8c000000

#define MSM_JTAG_TMUX_EBI2			0xA9000204
#endif


static void __init msm7x27a_init_ebi2(void)
{
	uint32_t ebi2_cfg;
	void __iomem *ebi2_cfg_ptr;

	ebi2_cfg_ptr = ioremap_nocache(MSM_EBI2_PHYS, sizeof(uint32_t));
	if (!ebi2_cfg_ptr)
		return;

	ebi2_cfg = readl(ebi2_cfg_ptr);
	if (machine_is_msm7x27a_rumi3() || machine_is_msm7x27a_surf() ||
		machine_is_msm7625a_surf() || machine_is_msm8625_surf())
		ebi2_cfg |= (1 << 4); /* CS2 */

#ifdef CONFIG_ISDBT
		// ebi2_cfg &= ~(0x3 << 0);
		ebi2_cfg &= ~(0x3 << 2); // clear CS1
		ebi2_cfg &= ~(0x1 << 4); // clear CS2
		ebi2_cfg &= ~(0x3 << 6); // clear CS4
		ebi2_cfg &= ~(0x3 << 8); // clear CS5

		// ebi2_cfg = 0;
		ebi2_cfg |=(0x2 << 2); /* setting CS1 as SRAM */
		pr_err("(%s) remapped ptr=%08x, EBI2 CFG VAL=%08x, settting CS1=%x\n", __func__, ebi2_cfg_ptr, ebi2_cfg, ebi2_cfg&0x0000000C);
				
#endif


	writel(ebi2_cfg, ebi2_cfg_ptr);
	iounmap(ebi2_cfg_ptr);

#ifdef CONFIG_ISDBT
		ebi2_cfg_ptr = ioremap_nocache(MSM_EBI2_CFG_1, sizeof(uint32_t));
	
		ebi2_cfg = readl(ebi2_cfg_ptr);
	//	ebi2_cfg &= ~(0x1 << 15);
	//	ebi2_cfg |=(0x1 << 15); /* setting FORCE_IE_HIGH */
		
	//	writel(ebi2_cfg, ebi2_cfg_ptr);
		pr_err("(%s) EBI2 CFG+04 VAL=%08x\n", __func__, ebi2_cfg);
		iounmap(ebi2_cfg_ptr);			


		ebi2_cfg_ptr = ioremap_nocache(MSM_EBI2_ARBITER_CFG, sizeof(uint32_t));

		ebi2_cfg = readl(ebi2_cfg_ptr);
		ebi2_cfg &= ~(0x3 << 4);  // clean the priority of EBI2_XMEMC
		ebi2_cfg &= ~(0x3 << 0);  // clean the priority of EBI2_LCDC
		
		ebi2_cfg |=(0x1 << 4); /* XMEMC : priority highest */
		ebi2_cfg |=(0x3 << 0); /* LCDC : priority highest */
		
		writel(ebi2_cfg, ebi2_cfg_ptr);
		iounmap(ebi2_cfg_ptr);			
#endif


	/* Enable A/D MUX[bit 31] from EBI2_XMEM_CS2_CFG1 */
	ebi2_cfg_ptr = ioremap_nocache(MSM_EBI2_XMEM_CS2_CFG1,
							 sizeof(uint32_t));
	if (!ebi2_cfg_ptr)
		return;

	ebi2_cfg = readl(ebi2_cfg_ptr);
	if (machine_is_msm7x27a_surf() || machine_is_msm7625a_surf())
		ebi2_cfg |= (1 << 31);

	writel(ebi2_cfg, ebi2_cfg_ptr);
	iounmap(ebi2_cfg_ptr);

#ifdef CONFIG_ISDBT
	/* LCD(PPI) mode: Enable A/D MUX[bit 31] from EBI2_XMEM_CS2_CFG1 */
	/* HPI mode: Disable A/D MUX[bit 31] from EBI2_XMEM_CS2_CFG1 */

	ebi2_cfg_ptr = ioremap_nocache(MSM_EBI2_XMEM_CS1_CFG0,
							 sizeof(uint32_t));
	if (!ebi2_cfg_ptr)
		return;

	ebi2_cfg = readl(ebi2_cfg_ptr);

	ebi2_cfg &= ~(0xF << 28); // clear CS to CS recovery
	ebi2_cfg |= 0xF << 28; // recovery time = 300ns
	
	ebi2_cfg &=~((0xF << 24) | (0xFF<< 16) | ( 0xF << 4)); // clear write timing
	ebi2_cfg &=~((0xF << 0) | (0xFF << 8)); // clear read timing

	ebi2_cfg |= (0x0A << 4); // max WRITE_WAIT  org 5 
	ebi2_cfg |= (0x1F << 16); // max initial latency WR org F
	ebi2_cfg |= (0xF << 24); // max hold write  org  6
	
	ebi2_cfg |= (0x0A << 0); // max read wait  org 7
	ebi2_cfg |= (0x1F << 8); // max initial latency RD  org 1F

	
	writel(ebi2_cfg, ebi2_cfg_ptr);	
	ebi2_cfg = readl(ebi2_cfg_ptr);
	pr_err("(%s) MSM_EBI2_XMEM_CS1_CFG0=%08x\n", __func__, ebi2_cfg);
	iounmap(ebi2_cfg_ptr);

	ebi2_cfg_ptr = ioremap_nocache(MSM_EBI2_XMEM_CS1_CFG1,
							 sizeof(uint32_t));
	if (!ebi2_cfg_ptr)
		return;

	ebi2_cfg = readl(ebi2_cfg_ptr);
	ebi2_cfg &= ~(0x1 << 31); // changed to NOT muxed
//	ebi2_cfg |= (0x1 << 31); // A/D Muxed

	ebi2_cfg &=~(0xF << 24);
	ebi2_cfg |= (0xF << 24); // max hold read  org A
//	ebi2_cfg |= (0x1 << 7); // BYTE DEVICE ENABLE
	
	ebi2_cfg &=~(0x3 << 16); // ADV_OE_RECOVERY
	ebi2_cfg |= (0x3 << 16); // ADV_OE_RECOVER val = 3

	ebi2_cfg &=~(0xF << 12); // PRECHARGE_CYC
	ebi2_cfg |= (0x4 << 12); // inter 4 cycles of PRECHARGE for consecutive read

	
	writel(ebi2_cfg, ebi2_cfg_ptr); 
	ebi2_cfg = readl(ebi2_cfg_ptr);

	pr_err("(%s) MSM_EBI2_XMEM_CS1_CFG1=%08x\n", __func__, ebi2_cfg);

	iounmap(ebi2_cfg_ptr);

// EBI2 is used, set the TMUX_EBI2 as 0x00 which means we uses EBI2 port as EBI2 functionality
	ebi2_cfg_ptr = ioremap_nocache(MSM_JTAG_TMUX_EBI2, sizeof(uint32_t));
	
	writel(0, ebi2_cfg_ptr);
	iounmap(ebi2_cfg_ptr);
	
#endif
}

static struct platform_device msm_proccomm_regulator_dev = {
	.name   = PROCCOMM_REGULATOR_DEV_NAME,
	.id     = -1,
	.dev    = {
		.platform_data = &msm7x27a_proccomm_regulator_data
	}
};

static void msm_adsp_add_pdev(void)
{
	int rc = 0;
	struct rpc_board_dev *rpc_adsp_pdev;

	rpc_adsp_pdev = kzalloc(sizeof(struct rpc_board_dev), GFP_KERNEL);
	if (rpc_adsp_pdev == NULL) {
		pr_err("%s: Memory Allocation failure\n", __func__);
		return;
	}
	rpc_adsp_pdev->prog = ADSP_RPC_PROG;

	if (cpu_is_msm8625())
		rpc_adsp_pdev->pdev = msm8625_device_adsp;
	else
		rpc_adsp_pdev->pdev = msm_adsp_device;
	rc = msm_rpc_add_board_dev(rpc_adsp_pdev, 1);
	if (rc < 0) {
		pr_err("%s: return val: %d\n",	__func__, rc);
		kfree(rpc_adsp_pdev);
	}
}

static void __init msm7627a_rumi3_init(void)
{
	msm7x27a_init_ebi2();
	platform_add_devices(rumi_sim_devices,
			ARRAY_SIZE(rumi_sim_devices));
}

static void __init msm8625_rumi3_init(void)
{
	msm7x2x_misc_init();
	msm_adsp_add_pdev();
	msm8625_device_i2c_init();
	platform_add_devices(msm8625_rumi3_devices,
			ARRAY_SIZE(msm8625_rumi3_devices));

	msm_pm_set_platform_data(msm8625_pm_data,
			 ARRAY_SIZE(msm8625_pm_data));
	BUG_ON(msm_pm_boot_init(&msm_pm_8625_boot_pdata));
	msm8x25_spm_device_init();
}

#define UART1DM_RX_GPIO		45

static int __init msm7x27a_init_ar6000pm(void)
{
	return platform_device_register(&msm_wlan_ar6000_pm_device);
}

static void __init msm7x27a_init_regulators(void)
{
	int rc = platform_device_register(&msm_proccomm_regulator_dev);
	if (rc)
		pr_err("%s: could not register regulator device: %d\n",
				__func__, rc);
}

static void __init msm7x27a_add_footswitch_devices(void)
{
	platform_add_devices(msm_footswitch_devices,
			msm_num_footswitch_devices);
}

static void __init msm7x27a_add_platform_devices(void)
{
	if (machine_is_msm8625_surf() || machine_is_msm8625_ffa()) {
		platform_add_devices(msm8625_surf_devices,
			ARRAY_SIZE(msm8625_surf_devices));
	} else {
		platform_add_devices(msm7627a_surf_ffa_devices,
			ARRAY_SIZE(msm7627a_surf_ffa_devices));
	}

	platform_add_devices(common_devices,
			ARRAY_SIZE(common_devices));
}

static void __init msm7x27a_uartdm_config(void)
{
	msm7x27a_cfg_uart2dm_serial();
	msm_uart_dm1_pdata.wakeup_irq = gpio_to_irq(UART1DM_RX_GPIO);
	if (cpu_is_msm8625())
		msm8625_device_uart_dm1.dev.platform_data =
			&msm_uart_dm1_pdata;
	else
		msm_device_uart_dm1.dev.platform_data = &msm_uart_dm1_pdata;
}

static void __init msm7x27a_otg_gadget(void)
{
	if (cpu_is_msm8625()) {
		msm_otg_pdata.swfi_latency =
		msm8625_pm_data[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].latency;
		msm8625_device_otg.dev.platform_data = &msm_otg_pdata;
		msm8625_device_gadget_peripheral.dev.platform_data =
			&msm_gadget_pdata;
	} else {
		msm_otg_pdata.swfi_latency =
		msm7x27a_pm_data[
		MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency;
		msm_device_otg.dev.platform_data = &msm_otg_pdata;
		msm_device_gadget_peripheral.dev.platform_data =
			&msm_gadget_pdata;
	}
}

static void __init msm7x27a_pm_init(void)
{
	if (machine_is_msm8625_surf() || machine_is_msm8625_ffa()) {
		msm_pm_set_platform_data(msm8625_pm_data,
				ARRAY_SIZE(msm8625_pm_data));
		BUG_ON(msm_pm_boot_init(&msm_pm_8625_boot_pdata));
		msm8x25_spm_device_init();
	} else {
		msm_pm_set_platform_data(msm7x27a_pm_data,
				ARRAY_SIZE(msm7x27a_pm_data));
		BUG_ON(msm_pm_boot_init(&msm_pm_boot_pdata));
	}

	msm_pm_register_irqs();
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
	
	GPIO_CFG(GPIO_BT_UART_RTS, 2/*1*/, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_BT_UART_CTS, 2/*1*/, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_BT_UART_RXD, 2/*1*/, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_BT_UART_TXD, 2/*1*/, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),

	GPIO_CFG(GPIO_BT_PCM_DOUT, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_BT_PCM_DIN, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_BT_PCM_SYNC, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_BT_PCM_CLK, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	//GPIO_CFG(GPIO_BT_PWR, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_BT_HOST_WAKE, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
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
	GPIO_CFG(GPIO_BT_HOST_WAKE, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
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
	
	printk(KERN_DEBUG "%s\n", __func__);
	
	

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
	
		printk(KERN_DEBUG "config_gpio_table bt pwr on\n");
		msleep(100);
		config_gpio_table(bt_config_power_on, ARRAY_SIZE(bt_config_power_on));
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

		printk(KERN_DEBUG "config_gpio_table bt pwr off\n");
		config_gpio_table(bt_config_power_off, ARRAY_SIZE(bt_config_power_off));
				
		/*
		msleep(10);
		gpio_direction_output(GPIO_RST_BT, GPIO_BT_LEVEL_LOW);

		printk(KERN_DEBUG "config_gpio_table bt pwr off\n");
		config_gpio_table(bt_config_power_off, ARRAY_SIZE(bt_config_power_off));

		gpio_set_value(GPIO_BT_EN, 0);
		*/
	}
	return 0;
}


static void __init bt_power_init(void)
{
	int rc = 0;

	pr_info("bt_power_init \n");

	msm_bt_power_device_csr.dev.platform_data = &bluetooth_power;

	pr_info("bt_gpio_init : low \n");

	config_gpio_table(bt_config_power_off, ARRAY_SIZE(bt_config_power_off));
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

static void keypad_gpio_init(void)
{
	int rc = 0;
	printk(KERN_INFO "[KEY] %s start\n", __func__);

	gpio_tlmm_config(GPIO_CFG(36, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, \
						GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(37, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, \
						GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(39, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, \
						GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(31, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, \
						GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_set_value_cansleep(31, 0);
}

static void nc_gpio_init(void)
{
	printk(KERN_INFO "[NC] %s start\n", __func__);
	gpio_tlmm_config(GPIO_CFG(49,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(75,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(76,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(82,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(98,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(113, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(124, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
#ifndef CONFIG_MACH_ROY_DTV //gpio 131/132 must be set PULLUP for latin roy-dtv
	gpio_tlmm_config(GPIO_CFG(131, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(132, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
#endif	
}

static void __init msm7x2x_init(void)
{
	msm7x2x_misc_init();

	/* Initialize regulators first so that other devices can use them */
	msm7x27a_init_regulators();
	msm_adsp_add_pdev();
	if (cpu_is_msm8625())
		msm8625_device_i2c_init();
	else
		msm7x27a_device_i2c_init();
	msm7x27a_init_ebi2();
	msm7x27a_uartdm_config();

	msm7x27a_otg_gadget();
	msm7x27a_cfg_smsc911x();

	fsa9480_gpio_init();
// GSCHO
////////////////////////////////////////////////////////////////////////////
#ifdef CONFIG_SEC_DEBUG
    sec_debug_init();
#endif
#ifdef CONFIG_SAMSUNG_JACK
	sec_jack_gpio_init();
#endif

#if defined(CONFIG_SENSORS_BMA2X2) \
	|| defined(CONFIG_PROXIMITY_SENSOR) || defined(CONFIG_SENSORS_BMM050) \
	|| defined(CONFIG_SENSORS_BMA222E) || defined(CONFIG_SENSORS_BMA222) \
	|| defined(CONFIG_SENSORS_HSCDTD006A) || defined(CONFIG_SENSORS_HSCDTD008A) \
	|| defined(CONFIG_SENSORS_PX3215) || defined(CONFIG_SENSORS_K2DH)
        sensor_gpio_init();
#endif

#if defined (CONFIG_RMI4_I2C)
	tsp_power_on();
#endif

#ifdef CONFIG_MAX17048_FUELGAUGE
	fg_max17048_gpio_init();
#endif

#if defined(CONFIG_BATTERY_STC3115)
	fg_STC3115_gpio_init();
#endif

	samsung_sys_class_init();

#if defined(CONFIG_TOUCHSCREEN_MELFAS_KYLE) 
	i2c_register_board_info( 2, touch_i2c_devices, ARRAY_SIZE(touch_i2c_devices));
#endif
#if defined (CONFIG_RMI4_I2C)
	i2c_register_board_info( 2, synaptics_i2c_devices, ARRAY_SIZE(synaptics_i2c_devices));
#endif

	i2c_register_board_info(3, fsa9480_i2c_devices,	ARRAY_SIZE(fsa9480_i2c_devices));

#ifdef CONFIG_MAX17048_FUELGAUGE
	i2c_register_board_info(6, fg_smb_i2c_devices, ARRAY_SIZE(fg_smb_i2c_devices));
#endif

#ifdef CONFIG_BATTERY_STC3115
	printk("STC3115 is registered");
	i2c_register_board_info(6, stc_i2c2_boardinfo, ARRAY_SIZE(stc_i2c2_boardinfo));
#endif

	keypad_gpio_init();

#ifdef CONFIG_GPIOKEY_KYLEP
	platform_device_register(&kp_pdev);		
	platform_device_register(&hs_pdev);
#endif

	if (!kernel_uart_flag)	{
#if !defined(CONFIG_MACH_ROY_DTV)  // ROY_LTN_DTV uses not UART2_DM but UART1
		platform_device_register(&msm_device_uart_dm2);
#else
		platform_device_register(&msm_device_uart1);
#endif
	}

////////////////////////////////////////////////////////////////////////////





	msm7x27a_add_footswitch_devices();
	msm7x27a_add_platform_devices();
	/* Ensure ar6000pm device is registered before MMC/SDC */
	msm7x27a_init_ar6000pm();
	msm7627a_init_mmc();
	mipi_kyle_gpio_init();
	msm_fb_add_devices();
	msm7x2x_init_host();
	msm7x27a_pm_init();
	register_i2c_devices();
	wlan_power_init();
	//msm7627a_bt_power_init();
	bt_power_init();
	msm7627a_camera_init();
	msm7627a_add_io_devices();

#ifdef CONFIG_ISDBT
	isdbt_dev_init();
#endif
	
	/*7x25a kgsl initializations*/
	msm7x25a_kgsl_3d0_init();
	/*8x25 kgsl initializations*/
	msm8x25_kgsl_3d0_init();
	
	nc_gpio_init();
}

static void __init msm7x2x_init_early(void)
{
	msm_msm7627a_allocate_memory_regions();
}

MACHINE_START(MSM7X27A_RUMI3, "QCT MSM7x27a RUMI3")
	.atag_offset	= 0x100,
	.map_io		= msm_common_io_init,
	.reserve	= msm7x27a_reserve,
	.init_irq	= msm_init_irq,
	.init_machine	= msm7627a_rumi3_init,
	.timer		= &msm_timer,
	.init_early     = msm7x2x_init_early,
	.handle_irq	= vic_handle_irq,
MACHINE_END
MACHINE_START(MSM7X27A_SURF, "QCT MSM7x27a SURF")
	.atag_offset	= 0x100,
	.map_io		= msm_common_io_init,
	.reserve	= msm7x27a_reserve,
	.init_irq	= msm_init_irq,
	.init_machine	= msm7x2x_init,
	.timer		= &msm_timer,
	.init_early     = msm7x2x_init_early,
	.handle_irq	= vic_handle_irq,
MACHINE_END
MACHINE_START(MSM7X27A_FFA, "QCT MSM7x27a FFA")
	.atag_offset	= 0x100,
	.map_io		= msm_common_io_init,
	.reserve	= msm7x27a_reserve,
	.init_irq	= msm_init_irq,
	.init_machine	= msm7x2x_init,
	.timer		= &msm_timer,
	.init_early     = msm7x2x_init_early,
	.handle_irq	= vic_handle_irq,
MACHINE_END
MACHINE_START(MSM7625A_SURF, "QCT MSM7625a SURF")
	.atag_offset    = 0x100,
	.map_io         = msm_common_io_init,
	.reserve        = msm7x27a_reserve,
	.init_irq       = msm_init_irq,
	.init_machine   = msm7x2x_init,
	.timer          = &msm_timer,
	.init_early     = msm7x2x_init_early,
	.handle_irq	= vic_handle_irq,
MACHINE_END
MACHINE_START(MSM7625A_FFA, "QCT MSM7625a FFA")
	.atag_offset    = 0x100,
	.map_io         = msm_common_io_init,
	.reserve        = msm7x27a_reserve,
	.init_irq       = msm_init_irq,
	.init_machine   = msm7x2x_init,
	.timer          = &msm_timer,
	.init_early     = msm7x2x_init_early,
	.handle_irq	= vic_handle_irq,
MACHINE_END
MACHINE_START(MSM8625_RUMI3, "QCT MSM8625 RUMI3")
	.atag_offset    = 0x100,
	.map_io         = msm8625_map_io,
	.reserve        = msm8625_reserve,
	.init_irq       = msm8625_init_irq,
	.init_machine   = msm8625_rumi3_init,
	.timer          = &msm_timer,
	.handle_irq	= gic_handle_irq,
MACHINE_END
MACHINE_START(MSM8625_SURF, "QCT MSM8625 SURF")
	.atag_offset    = 0x100,
	.map_io         = msm8625_map_io,
	.reserve        = msm8625_reserve,
	.init_irq       = msm8625_init_irq,
	.init_machine   = msm7x2x_init,
	.timer          = &msm_timer,
	.init_early     = msm7x2x_init_early,
	.handle_irq	= gic_handle_irq,
MACHINE_END
MACHINE_START(MSM8625_FFA, "QCT MSM8625 FFA")
	.atag_offset    = 0x100,
	.map_io         = msm8625_map_io,
	.reserve        = msm8625_reserve,
	.init_irq       = msm8625_init_irq,
	.init_machine   = msm7x2x_init,
	.timer          = &msm_timer,
	.init_early     = msm7x2x_init_early,
	.handle_irq	= gic_handle_irq,
MACHINE_END
