/*
 *  Bq24157-charger.c
 *  TI Bq24157 charger interface driver
 *
 *  Copyright (C) 2012 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/power/bq24157_charger.h>

#if defined(CONFIG_MACH_DELOS_OPEN)
#define GPIO_CHARGER_CD	113
#else
#define GPIO_CHARGER_CD	113
#endif

/* Register define */
#define BQ24157_REG0_STATUS	0x00
#define BQ24157_REG1_CONTROL	0x01
#define BQ24157_REG2_VOLTAGE	0x02
#define BQ24157_REG3_VENDOR	0x03
#define BQ24157_REG4_CURRENT	0x04
#define BQ24157_REG5_SPECIAL	0x05
#define BQ24157_REG6_SAFETY	0x06

#define RETRY_COUNT 3

static struct i2c_client	*bq24157_i2c_client;
static struct bq24157_platform_data *charger_ic;

static int bq24157_chg_current = BQ24157_CURR_NONE;

static int bq24157_write_reg(struct i2c_client *client, int reg, u8 value)
{
	int ret;
	int count = 0;

	do {
		ret = i2c_smbus_write_byte_data(client, reg, value);
		if (ret < 0) {
			pr_err("[BQ24157] %s: err %d\n", __func__, ret);
			count++;
		}
	} while(ret < 0 && count < RETRY_COUNT);

	return ret;
}

static int bq24157_read_reg(struct i2c_client *client, int reg)
{
	int ret;
	int count = 0;

	do {
		ret = i2c_smbus_read_byte_data(client, reg);
		if (ret < 0) {
			pr_err("[BQ24157] %s: err %d\n", __func__, ret);
			count++;
		}
	} while(ret < 0 && count < RETRY_COUNT);

	return ret;
}

static void _bq24157_print_all_register(void)
{
	u8 regs[7];
	regs[0] = bq24157_read_reg(bq24157_i2c_client, BQ24157_REG0_STATUS);
	regs[1] = bq24157_read_reg(bq24157_i2c_client, BQ24157_REG1_CONTROL);
	regs[2] = bq24157_read_reg(bq24157_i2c_client, BQ24157_REG2_VOLTAGE);
	regs[3] = bq24157_read_reg(bq24157_i2c_client, BQ24157_REG3_VENDOR);
	regs[4] = bq24157_read_reg(bq24157_i2c_client, BQ24157_REG4_CURRENT);
	regs[5] = bq24157_read_reg(bq24157_i2c_client, BQ24157_REG5_SPECIAL);
	regs[6] = bq24157_read_reg(bq24157_i2c_client, BQ24157_REG6_SAFETY);

	pr_info("[BQ24157] 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n",
		regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6]);
}

static void _bq24157_customize_register_settings(void)
{
	u8 value = 0;
	int ret = 0;

	/* weak battery voltage 3.4v */
	value = bq24157_read_reg(bq24157_i2c_client, BQ24157_REG1_CONTROL);
	value = value & ~(BQ24157_MASK_LOWV);
	value = value | (BQ24157_LOWV_3400 << BQ24157_SHIFT_LOWV);
	ret = bq24157_write_reg(bq24157_i2c_client, BQ24157_REG1_CONTROL, value);

	/* Charger voltae range 4.34v */
	value = bq24157_read_reg(bq24157_i2c_client, BQ24157_REG2_VOLTAGE);
	value = value & ~(BQ24157_MASK_VOREG);
	value = value | (charger_ic->VOreg << BQ24157_SHIFT_VOREG);
	ret = bq24157_write_reg(bq24157_i2c_client, BQ24157_REG2_VOLTAGE, value);

	/* Special charger voltage 4.52v*/
	value = bq24157_read_reg(bq24157_i2c_client, BQ24157_REG5_SPECIAL);
	value = value & ~(BQ24157_MASK_VSREG);
	value = value | (BQ24157_VSREG_4520 << BQ24157_SHIFT_VSREG);
	ret = bq24157_write_reg(bq24157_i2c_client, BQ24157_REG5_SPECIAL, value);
}	

static void _bq24157_charger_function_conrol(void)
{
}

static void _bq24157_charger_otg_conrol(void)
{
}

/* REG06(Safety Limit Register) is write only once after reset! */
static void _bq24157_set_safety_limits(void)
{	
	u8 value = 0;
	int ret = 0;
	/* Set Maximum Charge Current */
	value = (charger_ic->VMchrg << BQ24157_SHIFT_VMCHRG);
	
	/* Set Maximum Regulation Voltage */
	value = value | (charger_ic->VMreg << BQ24157_SHIFT_VMREG);

	ret = bq24157_write_reg(bq24157_i2c_client, BQ24157_REG6_SAFETY, value);
	return ret;
}

static int _bq24157_set_in_limit(int limit)
{
	char *bq24157_in_limit[] = { "100", "500", "800", "None"};

	u8 value = 0;
	int ret = 0;

	value = bq24157_read_reg(bq24157_i2c_client, BQ24157_REG1_CONTROL);
	value = value & ~(BQ24157_MASK_IN_LIMIT);
	value = value | (limit << BQ24157_SHIFT_IN_LIMIT);

	ret = bq24157_write_reg(bq24157_i2c_client, BQ24157_REG1_CONTROL, value);
	if (ret == 0) {
		pr_info("%s: IN_LIMIT = %s mA\n", __func__, bq24157_in_limit[limit]);
	} else {
		pr_info("%s: error\n", __func__);
	}

	return ret;
}	

static int _bq24157_set_charge_current_sense(int low)
{
	u8 value = 0;
	int ret = 0;
	
	value = bq24157_read_reg(bq24157_i2c_client, BQ24157_REG5_SPECIAL);
	value = value & ~(BQ24157_MASK_LOW_CHG);
	if (low) {
		value = value | (BQ24157_LOW_CHG_LOW_SENSE << BQ24157_SHIFT_LOW_CHG);			
	}
	ret = bq24157_write_reg(bq24157_i2c_client, BQ24157_REG5_SPECIAL, value);
	return ret;
}

static int _bq24157_set_charge_current(int curr)
{
	char *bq24157_55m_curr[] = { "680", "804", "927", "1051", "1145", "1269", "1392", "1516" }; /* 55mohm */
	char *bq24157_68m_curr[] = { "550", "650", "750", "850", "950", "1050", "1150", "1250" }; /* 68mohm */
	char *bq24157_100m_curr[] = { "374", "442", "510", "578", "646", "714", "782", "850" }; /* 100mohm */
	
	u8 value = 0;
	int ret = 0;	

	value = bq24157_read_reg(bq24157_i2c_client, BQ24157_REG4_CURRENT);
	value = value & ~(BQ24157_MASK_ICHRG);
	value = value | (curr << BQ24157_SHIFT_ICHRG);

	ret = bq24157_write_reg(bq24157_i2c_client, BQ24157_REG4_CURRENT, value);
	if (ret == 0) {
		if (charger_ic->Rsense == BQ24157_VRSENSE_55m)
			pr_info("%s: ICHRG = %smA with 55m Rsense\n", __func__, bq24157_55m_curr[curr]);
		else if (charger_ic->Rsense == BQ24157_VRSENSE_100m)
			pr_info("%s: ICHRG = %smA with 100m Rsense\n", __func__, bq24157_100m_curr[curr]);
		else
			pr_info("%s: ICHRG = %smA with 68m Rsense\n", __func__, bq24157_68m_curr[curr]);

		if (curr <= charger_ic->chg_curr_usb) {/* Set input limit 500mA under USB charging current */
			_bq24157_set_in_limit(BQ24157_IN_LIMIT_500mA);
			/* Low charge current sense */
			//_bq24157_set_charge_current_sense(1);
		}
		else {
			_bq24157_set_in_limit(BQ24157_IN_LIMIT_NONE);
			/* Normal charge Current Sense */
			//_bq24157_set_charge_current_sense(0);
		}
	} else {
		pr_info("%s: error\n", __func__);
	}

	return ret;
}

static int _bq24157_set_iterm(int term)
{
	char *bq24157_55m_iterm[] = { "62", "124", "186", "248", "309", "371", "433", "495" }; /* 55mohm */
	char *bq24157_68m_iterm[] = { "50", "100", "150", "200", "250", "300", "350", "400" }; /* 68mohm */
	char *bq24157_100m_iterm[] = { "34", "68", "102", "136", "170", "204", "268", "272" };	/* 100mohm */

	u8 value = 0;
	int ret = 0;

	value = bq24157_read_reg(bq24157_i2c_client, BQ24157_REG4_CURRENT);
	value = value & ~(BQ24157_MASK_ITERM);
	value = value | (term << BQ24157_SHIFT_ITERM);

	ret = bq24157_write_reg(bq24157_i2c_client, BQ24157_REG4_CURRENT, value);
	if (ret == 0) {
		if (charger_ic->Rsense == BQ24157_VRSENSE_55m)
			pr_info("%s: ITERM = %smA with 55m Rsense\n", __func__, bq24157_55m_iterm[term]);
		else if (charger_ic->Rsense == BQ24157_VRSENSE_100m)
			pr_info("%s: ITERM = %smA with 100m Rsense\n", __func__, bq24157_100m_iterm[term]);
		else
			pr_info("%s: ITERM = %smA with 68m Rsense\n", __func__, bq24157_68m_iterm[term]);
	} else {
		pr_info("%s: error\n", __func__);
	}

	return 0;
}

static int _bq24157_charging_control(int enable)
{
	u8 value = 0;
	int ret = 0;

	value = bq24157_read_reg(bq24157_i2c_client, BQ24157_REG1_CONTROL);

	value = value & ~(BQ24157_MASK_CE);
	value = value & ~(BQ24157_MASK_TE);

	switch (enable) {
		case CHG_STOP:
			gpio_set_value(GPIO_CHARGER_CD, 1);	/* Set CD pin HIGH */

			value = value |
				(BQ24157_TE_DISABLE << BQ24157_SHIFT_TE) |
				(BQ24157_CE_DISABLE << BQ24157_SHIFT_CE);
			break;
#ifdef CONFIG_MSM_BACKGROUND_CHARGING_EX_CHARGER
		case CHG_RE_START:
#endif
		case CHG_START:
			gpio_set_value(GPIO_CHARGER_CD, 0);	/* Set CD pin LOW */

			value = value |
				(BQ24157_TE_ENABLE << BQ24157_SHIFT_TE) |
				(BQ24157_CE_ENABLE << BQ24157_SHIFT_CE);
			break;
#ifdef CONFIG_MSM_BACKGROUND_CHARGING_EX_CHARGER
		case CHG_BACK_START:
			gpio_set_value(GPIO_CHARGER_CD, 0);	/* Set CD pin LOW */

#ifdef CONFIG_MSM_BACKGROUND_CHARGING_EX_CHARGER_TIMER
			/* If BACKGROUND charging with timer concept, turn off TE bit */
			value = value |
				(BQ24157_TE_DISABLE << BQ24157_SHIFT_TE) |
				(BQ24157_CE_ENABLE << BQ24157_SHIFT_CE);
#else
			value = value |
				(BQ24157_TE_ENABLE << BQ24157_SHIFT_TE) |
				(BQ24157_CE_ENABLE << BQ24157_SHIFT_CE);
#endif
			break;
#endif
		default:
			break;
	}

	ret = bq24157_write_reg(bq24157_i2c_client, BQ24157_REG1_CONTROL, value);
	if (ret < 0)
		pr_info("%s: error\n", __func__);

	return ret;
}

static int bq24157_stop_charging(void)
{
	if (!bq24157_i2c_client)
		return 0;

	bq24157_chg_current = BQ24157_CURR_NONE;

	_bq24157_charging_control(CHG_STOP);

	pr_info("%s\n", __func__);

	_bq24157_print_all_register();
	return 0;
}

#ifdef CONFIG_MSM_BACKGROUND_CHARGING_EX_CHARGER
static int bq24157_start_charging(int curr, int control)
{
	if (!bq24157_i2c_client)
		return 0;

	bq24157_chg_current = curr;
	if (bq24157_chg_current != (BQ24157_CURR_NONE)) {
		_bq24157_set_charge_current(bq24157_chg_current);
		if (control == CHG_RE_START)
			_bq24157_set_iterm(charger_ic->Iterm_2nd);
		else
			_bq24157_set_iterm(charger_ic->Iterm);
		_bq24157_charging_control(control);
	} else {
		pr_err("%s: Error (curr = %d)\n", __func__, bq24157_chg_current);
		_bq24157_charging_control(CHG_STOP);
	}
	_bq24157_print_all_register();
}
#else
static int bq24157_start_charging(int curr)
{
	if (!bq24157_i2c_client)
		return 0;

	bq24157_chg_current = curr;
	if (bq24157_chg_current != (BQ24157_CURR_NONE)) {
		_bq24157_set_charge_current(bq24157_chg_current);
		_bq24157_set_iterm(charger_ic->Iterm);
		_bq24157_charging_control(CHG_START);
	} else {
		pr_err("%s: Error (curr = %d)\n", __func__, bq24157_chg_current);
		_bq24157_charging_control(CHG_STOP);
	}
	_bq24157_print_all_register();
}
#endif

static int _bq24157_get_charging_fault(void)
{
	u8 value = 0;

	value = bq24157_read_reg(bq24157_i2c_client, BQ24157_REG0_STATUS);
	switch(value & BQ24157_MASK_FAULT) {		
		case BQ24157_FAULT_NORMAL:
			pr_info("%s: none\n", __func__);
			break;
		case BQ24157_FAULT_VBUS_OVP:
			pr_info("%s: vbus ovp\n", __func__);
			break;
		case BQ24157_FAULT_SLEEP_MODE:
			pr_info("%s: sleep mode\n", __func__);
			break;
		case BQ24157_FAULT_POOR_INPUT:
			pr_info("%s: poor input source\n", __func__);
			break;
		case BQ24157_FAULT_BATT_OVP:
			pr_info("%s: battery ovp\n", __func__);
			break;
		case BQ24157_FAULT_THERMAL:
			pr_info("%s: thermal fault\n", __func__);
			break;
		case BQ24157_FAULT_TIMER:
			pr_info("%s: timer fault\n", __func__);
			break;
		case BQ24157_FAULT_NO_BATT:
			pr_info("%s: no battery\n", __func__);
			break;
		}
		
		return (value & BQ24157_MASK_FAULT);
}

static int bq24157_get_charging_status(void)
{
	u8 value;

	value = bq24157_read_reg(bq24157_i2c_client, BQ24157_REG0_STATUS);
	value = value & BQ24157_MASK_STAT;

	if (value == (BQ24157_STAT_READY << BQ24157_SHIFT_STAT))
		return BQ24157_STAT_READY;
	else if (value == (BQ24157_STAT_CHARGING << BQ24157_SHIFT_STAT))
		return BQ24157_STAT_CHARGING;
	else if (value == (BQ24157_STAT_CHG_DONE << BQ24157_SHIFT_STAT))
		return BQ24157_STAT_CHG_DONE;
	else
		return BQ24157_STAT_FAULT;

}

static irqreturn_t _bq24157_handle_stat(int irq, void *data)
{
	struct bq24157_platform_data *pdata = data;	
	enum bq24157_fault_t fault_reason;
	int stat;

	msleep(500);

	stat = bq24157_get_charging_status();

	switch (stat) {
	case BQ24157_STAT_READY:
		pr_info("%s: ready\n", __func__);
		break;
	case BQ24157_STAT_CHARGING:
		pr_info("%s: charging\n", __func__);
		break;
	case BQ24157_STAT_CHG_DONE:
		pdata->tx_charge_done();
		pr_info("%s: chg done\n", __func__);
		break;
	case BQ24157_STAT_FAULT:
		pr_info("%s: chg fault\n", __func__);
		_bq24157_print_all_register();
		fault_reason = _bq24157_get_charging_fault();
		pdata->tx_charge_fault(fault_reason);
		_bq24157_customize_register_settings();
#ifdef CONFIG_MSM_BACKGROUND_CHARGING_EX_CHARGER
		bq24157_start_charging(bq24157_chg_current, CHG_START);
#else
		bq24157_start_charging(bq24157_chg_current);
#endif
		break;
	}

	return IRQ_HANDLED;
}

static int bq24157_get_fault(void)
{
	enum bq24157_fault_t fault_reason = BQ24157_FAULT_NORMAL;
	
	if (bq24157_get_charging_status() == BQ24157_STAT_FAULT)
		fault_reason = _bq24157_get_charging_fault();

	return fault_reason;
}

static int _bq24157_get_ic_info(void)
{
	return 0;
}

static int __devinit bq24157_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct bq24157_platform_data *pdata = client->dev.platform_data;
	int ret;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -EIO;

	printk("[BQ24157] %s: bq2415x driver Loading! \n", __func__);

	bq24157_i2c_client = client;
	charger_ic = pdata;

	pdata->start_charging = bq24157_start_charging;
	pdata->stop_charging = bq24157_stop_charging;
	pdata->get_fault = bq24157_get_fault;
	pdata->get_status = bq24157_get_charging_status;

	ret = request_threaded_irq(client->irq, NULL, _bq24157_handle_stat,
				   (IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING),
				   "bq24157-stat", pdata);

	_bq24157_set_safety_limits();
	_bq24157_get_ic_info();
	_bq24157_customize_register_settings();
	_bq24157_print_all_register();

	return ret;
}

static int __devexit bq24157_remove(struct i2c_client *client)
{
	return 0;
}

#ifdef CONFIG_PM
static int bq24157_suspend(struct i2c_client *client,
		pm_message_t state)
{
	//struct bq24157_chip *chip = i2c_get_clientdata(client);

	return 0;
}

static int bq24157_resume(struct i2c_client *client)
{
	//struct bq2415x_chip *chip = i2c_get_clientdata(client);

	return 0;
}
#else
#define bq24157_suspend NULL
#define bq24157_resume NULL
#endif /* CONFIG_PM */

static const struct i2c_device_id bq24157_id[] = {
	{ "bq24157", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, bq24157_id);

static struct i2c_driver bq24157_i2c_driver = {
	.driver	= {
		.name	= "bq24157",
	},
	.probe		= bq24157_probe,
	.remove		= __devexit_p(bq24157_remove),
	.suspend		= bq24157_suspend,
	.resume		= bq24157_resume,
	.id_table		= bq24157_id,
};

static int __init bq24157_init(void)
{
	int err;
	
	pr_info("[BQ24157] %s\n", __func__);
	
	err = i2c_add_driver(&bq24157_i2c_driver);
	if (err)
		pr_info("%s[BQ24157] add driver failed "
		       "(err: %d)\n", __func__, err);
	else
		pr_info("%s[BQ24157] added driver %s, (err: %d)\n",
		       __func__, bq24157_i2c_driver.driver.name, err);
	return err;
}
module_init(bq24157_init);

static void __exit bq24157_exit(void)
{
	i2c_del_driver(&bq24157_i2c_driver);
}
module_exit(bq24157_exit);


MODULE_DESCRIPTION("TI Bq24157 charger control driver");
