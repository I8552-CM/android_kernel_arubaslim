/* /linux/drivers/misc/modem_if/modem_modemctl_device_sprd6500.c
 *
 * Copyright (C) 2010 Google, Inc.
 * Copyright (C) 2010 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/init.h>

#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/platform_device.h>

#include <linux/platform_data/modem.h>
#include "modem_prj.h"
#include <linux/regulator/consumer.h>

//#include <plat/gpio-cfg.h>		//ygil.Yang temp

#if 0 //ygil.Yang temp
#if defined(CONFIG_LINK_DEVICE_DPRAM)
#include "modem_link_device_dpram.h"
#elif defined(CONFIG_LINK_DEVICE_PLD)
#include "modem_link_device_pld.h"
#endif
#else
#if defined(CONFIG_LINK_DEVICE_PLD)
#include "modem_link_device_pld.h"
#else
#include "modem_link_device_dpram.h"
#endif
#endif

#if defined(CONFIG_MACH_DELOS_DUOS_CTC)
#include <mach/gpio_delos.h>
#endif

#define GPIO_FPGA1_CS_N 127
#define GPIO_FPGA_SPI_MOSI 126

#if defined(CONFIG_LINK_DEVICE_DPRAM) || defined(CONFIG_LINK_DEVICE_PLD)
//#include <linux/mfd/max77693.h>	//ygil.Yang temp

#define PIF_TIMEOUT		(180 * HZ)
#define DPRAM_INIT_TIMEOUT	(30 * HZ)


#if defined(CONFIG_MACH_DELOS_DUOS_CTC)
static uint32_t sprd6500_gpio_UART_on[] = {	
	GPIO_CFG(GPIO_BT_UART_RTS, 2/*1*/, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_BT_UART_CTS, 2/*1*/, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_BT_UART_RXD, 2/*1*/, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_BT_UART_TXD, 2/*1*/, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static uint32_t sprd6500_gpio_UART_off[] = {	
	GPIO_CFG(GPIO_BT_UART_RTS, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_BT_UART_CTS, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_BT_UART_RXD, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_BT_UART_TXD, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
};

#endif


#if defined(CONFIG_MACH_DELOS_DUOS_CTC)
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

static void sprd6500_UART_config(struct modem_ctl *mc, int onoff)
{
	extern unsigned int board_hw_revision;

	if (board_hw_revision >= 4)	{
		if (onoff)
			config_gpio_table(sprd6500_gpio_UART_on, ARRAY_SIZE(sprd6500_gpio_UART_on));
		else
			config_gpio_table(sprd6500_gpio_UART_off, ARRAY_SIZE(sprd6500_gpio_UART_off));
	}
}
#endif

static int sprd6500_on(struct modem_ctl *mc)
{
	int ret;
	struct dpram_link_device *dpram_ld =
		to_dpram_link_device(get_current_link(mc->iod));

	pr_err("[MODEM_IF:ESC] <%s> start!!!\n", __func__);

	if (!mc->gpio_cp_reset) {
		pr_err("[MODEM_IF:ESC] no gpio data\n");
		return -ENXIO;
	}

	disable_irq(mc->irq_phone_active);

#if defined(CONFIG_MACH_DELOS_DUOS_CTC)
	// UART1_SEL for UART download
	sprd6500_UART_config(mc, 1);
	gpio_set_value(GPIO_UART1_SEL, 0);
#endif

	if (mc->gpio_reset_req_n)
		gpio_set_value(mc->gpio_reset_req_n, 1);

	pr_err("[MODEM_IF:ESC] <%s> CP On(%d,%d)\n", __func__, 
			gpio_get_value(mc->gpio_cp_on),
			gpio_get_value(mc->gpio_phone_active));

	gpio_set_value(mc->gpio_cp_on, 1);
	msleep(100);

	pr_err("[MODEM_IF:ESC] <%s> IRQ enabled\n", __func__);
	pr_err("[MODEM_IF:ESC] <%s> Active = %d\n", __func__, 
			gpio_get_value(mc->gpio_phone_active));

	enable_irq(mc->irq_phone_active);
	msleep(280);

	pr_err("[MODEM_IF:ESC] <%s> Active = %d\n", __func__, 
			gpio_get_value(mc->gpio_phone_active));

	gpio_set_value(GPIO_FPGA_SPI_MOSI, 1);
	gpio_set_value(mc->gpio_fpga1_cs_n, 1);
	gpio_set_value(mc->gpio_pda_active, 1);

	return 0;
}

static int sprd6500_off(struct modem_ctl *mc)
{
	pr_info("[MODEM_IF:ESC] sprd6500_off()\n");

#if 0	// SPRD don't have reset pin
	if (!mc->gpio_cp_reset) {
		pr_err("[MODEM_IF:ESC] no gpio data\n");
		return -ENXIO;
	}
#endif
	mc->iod->modem_state_changed(mc->iod, STATE_OFFLINE);

	gpio_set_value(mc->gpio_cp_reset, 0);
	gpio_set_value(mc->gpio_cp_on, 0);

	msleep(10);	// less 2ms needed.

	return 0;
}

static int sprd6500_reset(struct modem_ctl *mc)
{
	int ret = 0;

	pr_debug("[MODEM_IF:ESC] sprd6500_reset()\n");

	ret = sprd6500_off(mc);
	if (ret)
		return -ENXIO;

	msleep(100);

	ret = sprd6500_on(mc);
	if (ret)
		return -ENXIO;

	return 0;
}

int sprd6500_boot_on(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->iod);

	pr_info("[MODEM_IF:ESC] <%s>\n", __func__);

	/* Need to init uart byt gpio_flm_uart_sel GPIO */
	if (!mc->gpio_cp_reset || !mc->gpio_flm_uart_sel) {
		pr_err("[MODEM_IF:ESC] no gpio data\n");
		return -ENXIO;
	}

	gpio_set_value(mc->gpio_flm_uart_sel, 0);
	msleep(20);

//	pr_info("  - ESC_PHONE_ON : %d, ESC_RESET_N : %d\n",
//			gpio_get_value(mc->gpio_cp_on),
//			gpio_get_value(mc->gpio_cp_reset));

//	gpio_set_value(mc->gpio_cp_on, 0);
	gpio_direction_output(mc->gpio_cp_reset, 1);
//	msleep(100);

	gpio_direction_output(mc->gpio_cp_on, 1);
//	msleep(44);

	pr_info("  - ESC_PHONE_ON : %d, ESC_RESET_N : %d\n",
			gpio_get_value(mc->gpio_cp_on),
			gpio_get_value(mc->gpio_cp_reset));

//	gpio_direction_input(mc->gpio_cp_reset);
//	msleep(600);
//	gpio_direction_output(mc->gpio_cp_on, 0);

//	msleep(20);
	pr_info("  - ESC_PHONE_ON : %d, ESC_RESET_N : %d\n",
			gpio_get_value(mc->gpio_cp_on),
			gpio_get_value(mc->gpio_cp_reset));

	gpio_set_value(mc->gpio_fpga1_cs_n, 1);
	gpio_set_value(mc->gpio_pda_active, 1);
	
//	mc->iod->modem_state_changed(mc->iod, STATE_BOOTING);
	ld->mode = LINK_MODE_BOOT;

	return 0;
}

static int sprd6500_boot_off(struct modem_ctl *mc)
{
	pr_info("[MODEM_IF:ESC] <%s>\n", __func__);

	if (!mc->gpio_flm_uart_sel) {
		pr_err("[MODEM_IF:ESC] no gpio data\n");
		return -ENXIO;
	}

	gpio_set_value(mc->gpio_flm_uart_sel, 1);

//	mc->iod->modem_state_changed(mc->iod, STATE_OFFLINE);

	return 0;
}

static int sprd6500_force_crash_exit(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->iod);

	pr_info("[MODEM_IF:SPRD] <%s> Called!!\n", __func__);
	
	/* Make DUMP start */
	ld->force_dump(ld, mc->iod);

	msleep_interruptible(1000);

	mc->iod->modem_state_changed(mc->iod, STATE_CRASH_EXIT);

	return 0;
}


static int sprd6500_active_count;

static irqreturn_t phone_active_irq_handler(int irq, void *arg)
{
	struct modem_ctl *mc = (struct modem_ctl *)arg;
	int phone_reset = 0;
	int phone_active = 0;
	int phone_state = 0;
	int cp_dump_int = 0;

	if (!mc->gpio_cp_reset ||
		!mc->gpio_phone_active) { /* || !mc->gpio_cp_dump_int) { */
		pr_err("[MODEM_IF:ESC] no gpio data\n");
		return IRQ_HANDLED;
	}

	phone_reset = gpio_get_value(mc->gpio_cp_reset);
	phone_active = gpio_get_value(mc->gpio_phone_active);
	cp_dump_int = gpio_get_value(mc->gpio_cp_dump_int);

	phone_reset = 1; // actually, we don't use "reset pin"

	pr_info("[MODEM_IF:ESC] <%s> phone_reset=%d, phone_active=%d, cp_dump_int=%d\n",
		__func__, phone_reset, phone_active, cp_dump_int);

	if (phone_reset && phone_active) {
		phone_state = STATE_ONLINE;
		if (mc->iod && mc->iod->modem_state_changed)	{
			struct dpram_link_device *dpram_ld =
				to_dpram_link_device(get_current_link(mc->iod));

			mc->iod->modem_state_changed(mc->iod, phone_state);
		
			/* Do after PHONE_ACTIVE High */
			dpram_ld->dpram_init_status = DPRAM_INIT_STATE_READY;
			dpram_ld->cmd_phone_start_handler(dpram_ld);
			complete_all(&dpram_ld->dpram_init_cmd);
		}
	} else if (phone_reset && !phone_active) {
		if (mc->phone_state == STATE_ONLINE) {
			phone_state = STATE_CRASH_EXIT;
			if (mc->iod && mc->iod->modem_state_changed)
				mc->iod->modem_state_changed(mc->iod,
							     phone_state);
		}
	} else {
		phone_state = STATE_OFFLINE;
		if (mc->iod && mc->iod->modem_state_changed)
			mc->iod->modem_state_changed(mc->iod, phone_state);
	}

	if (phone_active)
		irq_set_irq_type(mc->irq_phone_active, IRQ_TYPE_LEVEL_LOW);
	else
		irq_set_irq_type(mc->irq_phone_active, IRQ_TYPE_LEVEL_HIGH);

	pr_info("[MODEM_IF:ESC] <%s> phone_state = %d\n",
			__func__, phone_state);

	return IRQ_HANDLED;
}

#if defined(CONFIG_SIM_DETECT)
static irqreturn_t sim_detect_irq_handler(int irq, void *_mc)
{
	struct modem_ctl *mc = (struct modem_ctl *)_mc;

	pr_info("[MODEM_IF:ESC] <%s> gpio_sim_detect = %d\n",
		__func__, gpio_get_value(mc->gpio_sim_detect));

	if (mc->iod && mc->iod->sim_state_changed)
		mc->iod->sim_state_changed(mc->iod,
		!gpio_get_value(mc->gpio_sim_detect));

	return IRQ_HANDLED;
}
#endif

static void sprd6500_get_ops(struct modem_ctl *mc)
{
	mc->ops.modem_on = sprd6500_on;
	mc->ops.modem_off = sprd6500_off;
	mc->ops.modem_reset = sprd6500_reset;
	mc->ops.modem_boot_on = sprd6500_boot_on;
	mc->ops.modem_boot_off = sprd6500_boot_off;
	mc->ops.modem_force_crash_exit = sprd6500_force_crash_exit;
}

int sprd6500_init_modemctl_device(struct modem_ctl *mc, struct modem_data *pdata)
{
	int ret = 0;
	struct platform_device *pdev;

	mc->gpio_cp_on = pdata->gpio_cp_on;
	mc->gpio_reset_req_n = pdata->gpio_reset_req_n;
	mc->gpio_cp_reset = pdata->gpio_cp_reset;
	mc->gpio_pda_active = pdata->gpio_pda_active;
	mc->gpio_phone_active = pdata->gpio_phone_active;
	mc->gpio_cp_dump_int = pdata->gpio_cp_dump_int;
	mc->gpio_flm_uart_sel = pdata->gpio_flm_uart_sel;
	mc->gpio_cp_warm_reset = pdata->gpio_cp_warm_reset;
	mc->gpio_sim_detect = pdata->gpio_sim_detect;

#if defined(CONFIG_LINK_DEVICE_PLD)
	mc->gpio_fpga1_cs_n = pdata->gpio_fpga1_cs_n;
#endif
	gpio_set_value(mc->gpio_cp_reset, 0);
	gpio_set_value(mc->gpio_cp_on, 0);

	pdev = to_platform_device(mc->dev);
	mc->irq_phone_active = platform_get_irq_byname(pdev, "cp_active_irq");
	pr_info("[MODEM_IF:ESC] <%s> PHONE_ACTIVE IRQ# = %d\n",
		__func__, mc->irq_phone_active);

	sprd6500_get_ops(mc);

	if (mc->irq_phone_active) {
		ret = request_irq(mc->irq_phone_active,
				  phone_active_irq_handler,
				  IRQF_TRIGGER_HIGH,
				  "esc_active",
				  mc);
		if (ret) {
			pr_err("[MODEM_IF:ESC] <%s> failed to request_irq IRQ# %d (err=%d)\n",
				__func__, mc->irq_phone_active, ret);
			return ret;
		}

		ret = enable_irq_wake(mc->irq_phone_active);
		if (ret) {
			pr_err("[MODEM_IF:ESC] %s: failed to enable_irq_wake IRQ# %d (err=%d)\n",
				__func__, mc->irq_phone_active, ret);
			free_irq(mc->irq_phone_active, mc);
			return ret;
		}
	}

#if defined(CONFIG_SIM_DETECT)
	mc->irq_sim_detect = platform_get_irq_byname(pdev, "sim_irq");
	pr_info("[MODEM_IF:ESC] <%s> SIM_DECTCT IRQ# = %d\n",
		__func__, mc->irq_sim_detect);

	if (mc->irq_sim_detect) {
		ret = request_irq(mc->irq_sim_detect, sim_detect_irq_handler,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			"esc_sim_detect", mc);
		if (ret) {
			mif_err("failed to request_irq: %d\n", ret);
				mc->sim_state.online = false;
				mc->sim_state.changed = false;
			return ret;
		}

		ret = enable_irq_wake(mc->irq_sim_detect);
		if (ret) {
			mif_err("failed to enable_irq_wake: %d\n", ret);
			free_irq(mc->irq_sim_detect, mc);
			mc->sim_state.online = false;
			mc->sim_state.changed = false;
			return ret;
		}

		/* initialize sim_state => insert: gpio=0, remove: gpio=1 */
		mc->sim_state.online = !gpio_get_value(mc->gpio_sim_detect);
	}
#endif

	return ret;
}
#endif
