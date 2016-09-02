/* Switch mode charger to minimize single cell lithium ion charging time from a USB power source.
 *
 * Copyright (C) 2009 Samsung Electronics
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/power/fan54013_charger.h>

#define DISABLE_T32SEC_TIMER

/* Register address */
#define FAN54013_REG_CONTROL0		0x00
#define FAN54013_REG_CONTROL1		0x01
#define FAN54013_REG_OREG			0x02
#define FAN54013_REG_IC_INFO		0x03
#define FAN54013_REG_IBAT			0x04
#define FAN54013_REG_SP_CHARGER	0x05
#define FAN54013_REG_SAFETY		0x06
#define FAN54013_REG_MONITOR		0x10

static struct i2c_client	*fan54013_i2c_client;
static struct work_struct	chg_work;
static struct timer_list	chg_work_timer;
static struct workqueue_struct	*charger_fan54103_wq;
static struct fan54013_platform_data *charger_ic;

static u8 ic_info;

extern unsigned int board_hw_revision;

static int fan54013_chg_current = CHG_CURR_NONE;

#define TIME_UNIT_SECOND	(HZ)
#define TIME_UNIT_MINUTE	(60*HZ)
#define TIME_UNIT_HOUR		(60*60*HZ)
#define FAN54013_POLLING_INTERVAL	(10 * TIME_UNIT_SECOND)

#ifdef CONFIG_MSM_BACKGROUND_CHARGING_EX_CHARGER
static int fan54013_start_charging(int curr, int control);
#else
static int fan54013_start_charging(int curr);
#endif
static int fan54013_stop_charging(void);

static void _fan54013_print_register(void)
{
	u8 regs[8];
	regs[0] = i2c_smbus_read_byte_data(fan54013_i2c_client, FAN54013_REG_CONTROL0);
	regs[1] = i2c_smbus_read_byte_data(fan54013_i2c_client, FAN54013_REG_CONTROL1);
	regs[2] = i2c_smbus_read_byte_data(fan54013_i2c_client, FAN54013_REG_OREG);
	regs[3] = i2c_smbus_read_byte_data(fan54013_i2c_client, FAN54013_REG_IC_INFO);
	regs[4] = i2c_smbus_read_byte_data(fan54013_i2c_client, FAN54013_REG_IBAT);
	regs[5] = i2c_smbus_read_byte_data(fan54013_i2c_client, FAN54013_REG_SP_CHARGER);
	regs[6] = i2c_smbus_read_byte_data(fan54013_i2c_client, FAN54013_REG_SAFETY);
	regs[7] = i2c_smbus_read_byte_data(fan54013_i2c_client, FAN54013_REG_MONITOR);

	pr_info("/FAN54013/ 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n",
		regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6], regs[7]);
}

static void _fan54013_customize_register_settings(void)
{
	u8 value;

	value = i2c_smbus_read_byte_data(fan54013_i2c_client, FAN54013_REG_CONTROL1);
	value = value & ~(0x3F);	// clear except input limit
	value = value | ((FAN54013_LOWV_34	<< FAN54013_SHIFT_LOWV) |	/* Weak battery voltage 3.4V */
		(FAN54013_TE_DISABLE	<< FAN54013_SHIFT_TE) |
		(FAN54013_CE_ENABLE	<< FAN54013_SHIFT_CE) |
		(FAN54013_HZ_MODE_NO	<< FAN54013_SHIFT_HZ_MODE) |
		(FAN54013_OPA_MODE_CHARGE	<< FAN54013_SHIFT_OPA_MODE));
	i2c_smbus_write_byte_data(fan54013_i2c_client, FAN54013_REG_CONTROL1, value);

	i2c_smbus_write_byte_data(fan54013_i2c_client, FAN54013_REG_OREG,		
		((charger_ic->Voreg)	<< FAN54013_SHIFT_OREG) |	/* Full charging voltage */
		(FAN54013_OTG_PL_HIGH	<< FAN54013_SHIFT_OTG_PL) |
		(FAN54013_OTG_EN_DISABLE	<< FAN54013_SHIFT_OTG_EN));

	i2c_smbus_write_byte_data(fan54013_i2c_client, FAN54013_REG_SP_CHARGER,
		(FAN54013_RESERVED_2	<< FAN54013_SHIFT_RESERVED_2) |
		(FAN54013_DIS_VREG_ON	<< FAN54013_SHIFT_DIS_VREG) |
		(FAN54013_IO_LEVEL_0	<< FAN54013_SHIFT_IO_LEVEL) |	/* Controlled by IOCHARGE bits */
		(FAN54013_SP_RDONLY	<< FAN54013_SHIFT_SP) |
		(FAN54013_EN_LEVEL_RDONLY	<< FAN54013_SHIFT_EN_LEVEL) |
#if defined(CONFIG_MACH_BAFFIN_DUOS_CTC)  || defined(CONFIG_MACH_ARUBASLIM_OPEN)
		(FAN54013_VSP_4213	<< FAN54013_SHIFT_VSP));
#else
		(FAN54013_VSP_4533	<< FAN54013_SHIFT_VSP));
#endif		

/*
	if (fan54013_chg_current == CHG_CURR_NONE)
		fan54013_stop_charging();
	else
		fan54013_start_charging(fan54013_chg_current);
*/
}

static int _fan54013_set_iocharge(int curr)
{
	u8 value;
	char *fan54013_68m_curr[] = { "550", "650", "750", "850", "1050", "1150", "1350", "1450" }; /* 68mohm */
	char *fan54013_100m_curr[] = { "374", "442", "510", "578", "714", "782", "918", "986" };	/* 100mohm */

	/* Set charge current */
	value = i2c_smbus_read_byte_data(fan54013_i2c_client, FAN54013_REG_IBAT);

	value = value & ~(FAN54013_MASK_RESET);	/* Clear control reset bit */
	value = value & ~(FAN54013_MASK_IOCHARGE);
	value = value | (curr << FAN54013_SHIFT_IOCHARGE);

	if (charger_ic->VRsense == FAN54013_VRSENSE_68m)
		pr_info("%s: IOCHARGE = %smA with 68m Rsense\n", __func__, fan54013_68m_curr[curr]);
	else
		pr_info("%s: IOCHARGE = %smA with 100m Rsense\n", __func__, fan54013_100m_curr[curr]);

	i2c_smbus_write_byte_data(fan54013_i2c_client, FAN54013_REG_IBAT, value);

	/* Set input current limit */
	value = i2c_smbus_read_byte_data(fan54013_i2c_client, FAN54013_REG_CONTROL1);

	value = value & ~(FAN54013_MASK_INLIM);

	if (charger_ic->VRsense == FAN54013_VRSENSE_68m) {
		if (curr <= FAN54013_CURR_550) {
			value = value | (FAN54013_INLIM_500 << FAN54013_SHIFT_INLIM);
			pr_info("%s: INLIM = 500mA\n", __func__);
		} else {
			value = value | (FAN54013_INLIM_NO_LIMIT << FAN54013_SHIFT_INLIM);
			pr_info("%s: INLIM = No limit\n", __func__);
		}
	} else {
		if (curr <= FAN54013_CURR_510) {
			value = value | (FAN54013_INLIM_500 << FAN54013_SHIFT_INLIM);
			pr_info("%s: INLIM = 500mA\n", __func__);
		} else {
			value = value | (FAN54013_INLIM_NO_LIMIT << FAN54013_SHIFT_INLIM);
			pr_info("%s: INLIM = No limit\n", __func__);
		}
	}

	i2c_smbus_write_byte_data(fan54013_i2c_client, FAN54013_REG_CONTROL1, value);

	return curr;
}

static int _fan54013_set_iterm(int iterm)
{
	u8 value;
	char *fan54013_68m_iterm[] = { "49", "97", "146", "194", "243", "291", "340", "388" }; /* 68mohm */
	char *fan54013_100m_iterm[] = { "33", "66", "99", "132", "165", "198", "231", "264" };	/* 100mohm */

	value = i2c_smbus_read_byte_data(fan54013_i2c_client, FAN54013_REG_IBAT);

	value = value & ~(FAN54013_MASK_RESET);	/* Clear control reset bit */
	value = value & ~(FAN54013_MASK_ITERM);

	if (charger_ic->VRsense == FAN54013_VRSENSE_68m) {	
		value = value | (iterm << FAN54013_SHIFT_ITERM);
		pr_info("%s: ITERM %smA\n", __func__, fan54013_68m_iterm[iterm]);
	} else {
		value = value | (iterm << FAN54013_SHIFT_ITERM);
		pr_info("%s: ITERM %smA\n", __func__, fan54013_100m_iterm[iterm]);
	}

	i2c_smbus_write_byte_data(fan54013_i2c_client, FAN54013_REG_IBAT, value);

	return 0;
}

static int _fan54013_charging_control(int enable)
{
	u8 value;

	value = i2c_smbus_read_byte_data(fan54013_i2c_client, FAN54013_REG_CONTROL1);

	value = value & ~(FAN54013_MASK_CE);
	value = value & ~(FAN54013_MASK_TE);

	/* Set CE and TE bit */
	switch (enable) {
		case CHG_STOP:
			value = value |
				(FAN54013_TE_DISABLE << FAN54013_SHIFT_TE) |
				(FAN54013_CE_DISABLE << FAN54013_SHIFT_CE);
			break;
#ifdef CONFIG_MSM_BACKGROUND_CHARGING_EX_CHARGER
		case CHG_RE_START:
		case CHG_BACK_START:
#endif
		case CHG_START:
		value = value |
			(FAN54013_TE_ENABLE << FAN54013_SHIFT_TE) |
			(FAN54013_CE_ENABLE << FAN54013_SHIFT_CE);
			break;
		default:
			break;
	}

	i2c_smbus_write_byte_data(fan54013_i2c_client, FAN54013_REG_CONTROL1, value);

	return 0;
}

static void _fan54013_chg_work(struct work_struct *work)
{
	u8 value;
	static int log_cnt = 0;

	/* Reset 32 sec timer while host is alive */
	value = i2c_smbus_read_byte_data(fan54013_i2c_client, FAN54013_REG_CONTROL0);

	value = value & ~(FAN54013_MASK_TMR_RST);
	value = value | (FAN54013_TMR_RST_RESET << FAN54013_SHIFT_TMR_RST);

	i2c_smbus_write_byte_data(fan54013_i2c_client, FAN54013_REG_CONTROL0, value);

	/* print registers every 10 minutes */
	log_cnt++;
	if (log_cnt == 60) {
		_fan54013_print_register();
		log_cnt = 0;
	}

	pr_debug("fan54013 host is alive\n");

	mod_timer(&chg_work_timer, jiffies + FAN54013_POLLING_INTERVAL);
}

static void _fan54013_chg_work_timer_func(unsigned long param)
{
	pr_info("[FAN54103] chg_work timer expired\n");
	queue_work(charger_fan54103_wq, &chg_work);
}

#ifdef CONFIG_MSM_BACKGROUND_CHARGING_EX_CHARGER
static int fan54013_start_charging(int curr, int control)
{
	if (!fan54013_i2c_client)
		return 0;

	fan54013_chg_current = curr;
	if (fan54013_chg_current != (-1)) {
		_fan54013_set_iocharge(fan54013_chg_current);
		if (control == CHG_START)
			_fan54013_set_iterm(charger_ic->Iterm);
		else // set 2nd terminatino current with CHG_RE_START or CHG_BACK_START 
			_fan54013_set_iterm(charger_ic->Iterm_2nd);
		_fan54013_charging_control(control);
	} else {		
		pr_err("%s: Error (curr = %d)\n", __func__, fan54013_chg_current);
		_fan54013_charging_control(CHG_STOP);
	}	
	_fan54013_print_register();
}
#else
static int fan54013_start_charging(int curr)
{
	int iocharge;

	if (!fan54013_i2c_client)
		return 0;

	fan54013_chg_current = curr;

	if (fan54013_chg_current != (-1)) {
		_fan54013_set_iocharge(fan54013_chg_current);
		_fan54013_set_iterm(charger_ic->Iterm);
		_fan54013_charging_control(1);
	} else {
		pr_err("%s: Error (curr = %d)\n", __func__, fan54013_chg_current);
		_fan54013_charging_control(0);
	}

	_fan54013_print_register();
	return 0;
}
#endif

static int fan54013_stop_charging(void)
{
	if (!fan54013_i2c_client)
		return 0;

	fan54013_chg_current = CHG_CURR_NONE;
	_fan54013_charging_control(0);

	pr_info("%s\n", __func__);

	_fan54013_print_register();
	return 0;
}

static int fan54013_get_vbus_status(void)
{
	u8 value;

	if (!fan54013_i2c_client)
		return 0;

	value = i2c_smbus_read_byte_data(fan54013_i2c_client, FAN54013_REG_MONITOR);

	if (value & FAN54013_MASK_VBUS_VALID)
		return 1;
	else
		return 0;
}

static int fan54013_set_host_status(bool isAlive)
{
	if (!fan54013_i2c_client)
		return 0;

	if (isAlive) {
		/* Reset 32 sec timer every 10 sec to ensure the host is alive */
		queue_work(charger_fan54103_wq, &chg_work);
	} else
		del_timer_sync(&chg_work_timer);

	return isAlive;
}

static int _fan54013_get_charging_fault(void)
{
	u8 value;

	if (!fan54013_i2c_client)
		return 0;

	value = i2c_smbus_read_byte_data(fan54013_i2c_client, FAN54013_REG_CONTROL0);

	switch (value & FAN54013_MASK_FAULT) {
	case FAN54013_FAULT_NONE:
		pr_info("%s: none\n", __func__);
		break;
	case FAN54013_FAULT_VBUS_OVP:
		pr_info("%s: vbus ovp\n", __func__);
		break;
	case FAN54013_FAULT_SLEEP_MODE:
		pr_info("%s: sleep mode\n", __func__);
		break;
	case FAN54013_FAULT_POOR_INPUT:
		pr_info("%s: poor input source\n", __func__);
		break;
	case FAN54013_FAULT_BATT_OVP:
		pr_info("%s: battery ovp\n", __func__);
		break;
	case FAN54013_FAULT_THERMAL:
		pr_info("%s: thermal shutdown\n", __func__);
		break;
	case FAN54013_FAULT_TIMER:
		pr_info("%s: timer fault\n", __func__);
		break;
	case FAN54013_FAULT_NO_BATT:
		pr_info("%s: no battery\n", __func__);
		break;
	}

	return (value & FAN54013_MASK_FAULT);
}

static int _fan54013_get_charging_status(void)
{
	u8 value;

	value = i2c_smbus_read_byte_data(fan54013_i2c_client, FAN54013_REG_CONTROL0);
	value = value & FAN54013_MASK_STAT;

	if (value == (FAN54013_STAT_READY << FAN54013_SHIFT_STAT))
		return FAN54013_STAT_READY;
	else if (value == (FAN54013_STAT_CHARGING << FAN54013_SHIFT_STAT))
		return FAN54013_STAT_CHARGING;
	else if (value == (FAN54013_STAT_CHG_DONE << FAN54013_SHIFT_STAT))
		return FAN54013_STAT_CHG_DONE;
	else
		return FAN54013_STAT_FAULT;
}

static irqreturn_t _fan54013_handle_stat(int irq, void *data)
{
	struct fan54013_platform_data *pdata = data;
	int stat;
	enum fan54013_fault_t fault_reason;

	msleep(500);

	stat = _fan54013_get_charging_status();

	switch (stat) {
	case FAN54013_STAT_READY:
		pr_info("%s: ready\n", __func__);
		break;
	case FAN54013_STAT_CHARGING:
		pr_info("%s: charging\n", __func__);
		//_fan54013_customize_register_settings();
		break;
	case FAN54013_STAT_CHG_DONE:
		pdata->tx_charge_done();
		pr_info("%s: chg done\n", __func__);
		break;
	case FAN54013_STAT_FAULT:
		pr_info("%s: chg fault\n", __func__);
		_fan54013_print_register();
		fault_reason = _fan54013_get_charging_fault();
		pdata->tx_charge_fault(fault_reason);
		_fan54013_customize_register_settings();
		if (fan54013_chg_current == CHG_CURR_NONE)
			fan54013_stop_charging();
		else
#ifdef CONFIG_MSM_BACKGROUND_CHARGING_EX_CHARGER
			fan54013_start_charging(fan54013_chg_current, CHG_START);
#else
			fan54013_start_charging(fan54013_chg_current);
#endif
		break;
	}

	return IRQ_HANDLED;
}

/* Support battery test mode (*#0228#) */
static int fan54013_get_monitor_bits(void)
{
	/*
		VICHG not supported with FAN5401x...

		0:	Discharging (no charger)
		1:	CC charging
		2:	CV charging
		3:	Not charging (charge termination) include fault
	*/

	u8 value = 0;

	if (fan54013_get_vbus_status()) {
		if (_fan54013_get_charging_status() == FAN54013_STAT_CHARGING) {
			/* Check CV bit */
			value = i2c_smbus_read_byte_data(fan54013_i2c_client, FAN54013_REG_MONITOR);
			if (value & FAN54013_MASK_CV)
				return 2;
			else
				return 1;
		}
		return 3;
	}
	return 0;
}

static int fan54013_get_fault(void)
{
	enum fan54013_fault_t fault_reason = FAN54013_FAULT_NONE;

	if (_fan54013_get_charging_status() == FAN54013_STAT_FAULT)
		fault_reason = _fan54013_get_charging_fault();

	return fault_reason;
}

static int fan54013_get_ic_info(void)
{
	u8 value = 0;
	char *fan54013_ic_name[] = {"FAN5401x", "FAN54010", "FAN54011", "FAN54012", "FAN54013", "FAN54014", "FAN54015"};
	
	if (!fan54013_i2c_client) {
		pr_err("%s: i2c not initialized!\n", __func__);
		return -1;
	}

	value = i2c_smbus_read_byte_data(fan54013_i2c_client, FAN54013_REG_IC_INFO);
	
	if ((value & 0x1C) == 0x14)
		ic_info = CHARGER_IC_FAN54013;
	else
		ic_info = CHARGER_IC_FAN5401x;

	pr_info("[%s] %s - REV 1.%d\n", __func__, fan54013_ic_name[ic_info], value & 0x3);

	return ic_info;
}

static void _fan54013_set_safety_limits(void)
{
	u8 value = 0;
	value = (charger_ic->Isafe << FAN54013_SHIFT_ISAFE) |	/* Maximum IOCHARGE */
		(charger_ic->Vsafe << FAN54013_SHIFT_VSAFE);	/* Maximum OREG */

	i2c_smbus_write_byte_data(fan54013_i2c_client, FAN54013_REG_SAFETY, value);
}


#ifdef DISABLE_T32SEC_TIMER
static void _fan54013_disable_T32sec_timer(void)
{
	u8 value = 0;

	/* step 1 : Set HZ bit ( Reg1[1] ) to disable charging prior to entering test mode */
	value = i2c_smbus_read_byte_data(fan54013_i2c_client, FAN54013_REG_CONTROL1);
	value |= 0x02;
	i2c_smbus_write_byte_data(fan54013_i2c_client, FAN54013_REG_CONTROL1, value);

	/* step 2 : Enter Test Mode*/
	i2c_smbus_write_byte_data(fan54013_i2c_client, 0xC3, 0x3D);

	/* step 3 : Write data to disable T32Sec timer */
	i2c_smbus_write_byte_data(fan54013_i2c_client, 0xA0, 0xAF);

	/* step 4 : Exit test mode */
	i2c_smbus_write_byte_data(fan54013_i2c_client, 0xC3, 0x3C);

	/* step 5 : Reset HZ bit to re-enable charging */
	value = i2c_smbus_read_byte_data(fan54013_i2c_client, FAN54013_REG_CONTROL1);
	value &= ~(0x02);
	i2c_smbus_write_byte_data(fan54013_i2c_client, FAN54013_REG_CONTROL1, value);
	pr_info("%s", __func__);
}


static void _fan54013_enable_T32sec_timer(void)
{
	u8 value = 0;

	/* step 1 : Set HZ bit ( Reg1[1] ) to disable charging prior to entering test mode */
	value = i2c_smbus_read_byte_data(fan54013_i2c_client, FAN54013_REG_CONTROL1);
	value |= 0x02;
	i2c_smbus_write_byte_data(fan54013_i2c_client, FAN54013_REG_CONTROL1, value);

	/* step 2 : Enter Test Mode*/
	i2c_smbus_write_byte_data(fan54013_i2c_client, 0xC3, 0x3D);

	/* step 3 : Clear data to disable T32Sec timer */
	i2c_smbus_write_byte_data(fan54013_i2c_client, 0xA0, 0x00);

	/* step 4 : Exit test mode */
	i2c_smbus_write_byte_data(fan54013_i2c_client, 0xC3, 0x3C);

	/* step 5 : Reset HZ bit to re-enable charging */
	value = i2c_smbus_read_byte_data(fan54013_i2c_client, FAN54013_REG_CONTROL1);
	value &= ~(0x02);
	i2c_smbus_write_byte_data(fan54013_i2c_client, FAN54013_REG_CONTROL1, value);
	pr_info("%s", __func__);
}
#endif

static int __devinit fan54013_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct fan54013_platform_data *pdata = client->dev.platform_data;
	int ret;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -EIO;

	pr_info("+++++++++++++++ %s +++++++++++++++\n", __func__);

	fan54013_i2c_client = client;
	charger_ic = pdata;
	
#if defined(CONFIG_MACH_ARUBASLIM_OPEN)				// Battery model change as hw revision
	if( board_hw_revision <= 3 )
	{
		charger_ic->Vsafe = FAN54013_VSAFE_420;
		charger_ic->Voreg = FAN54013_OREG_420;
	}		
#endif

	/* Init Rx functions from host */
	pdata->start_charging = fan54013_start_charging;
	pdata->stop_charging = fan54013_stop_charging;
	pdata->get_vbus_status = fan54013_get_vbus_status;
	pdata->set_host_status = fan54013_set_host_status;
	pdata->get_monitor_bits = fan54013_get_monitor_bits;
	pdata->get_fault = fan54013_get_fault;
	pdata->get_ic_info = fan54013_get_ic_info;

	/* SAFETY register have to be written before any other register is written */
	_fan54013_set_safety_limits();

#ifdef DISABLE_T32SEC_TIMER
#if !defined(CONFIG_MACH_BAFFIN_DUOS_CTC) && !defined(CONFIG_MACH_ARUBASLIM_OPEN) /* move to bootloader */
	_fan54013_disable_T32sec_timer();
#endif
#else
	INIT_WORK(&chg_work, _fan54013_chg_work);
	setup_timer(&chg_work_timer, _fan54013_chg_work_timer_func, (unsigned long)pdata);
	charger_fan54103_wq = create_singlethread_workqueue("fan54013_wq");
	if (!charger_fan54103_wq) {
		pr_info("%s: create workque failed\n", __func__);
		return -ENOMEM;
	}
	queue_work(charger_fan54103_wq, &chg_work);
	mod_timer(&chg_work_timer, jiffies + FAN54013_POLLING_INTERVAL);
#endif

	ret = request_threaded_irq(client->irq, NULL, _fan54013_handle_stat,
				   (IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING),
				   "fan54013-stat", pdata);

	fan54013_get_ic_info();
	_fan54013_customize_register_settings();
	_fan54013_print_register();

	return 0;
}

static int __devexit fan54013_remove(struct i2c_client *client)
{
	destroy_workqueue(charger_fan54103_wq);
	return 0;
}

static int fan54013_resume(struct i2c_client *client)
{
	struct fan54013_platform_data *pdata = client->dev.platform_data;

	if (_fan54013_get_charging_status() == FAN54013_STAT_CHG_DONE) {
		pr_info("%s: chg done\n", __func__);
		pdata->tx_charge_done();
	}

	return 0;
}

static const struct i2c_device_id fan54013_id[] = {
	{"fan54013", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, fan54013_id);

static struct i2c_driver fan54013_i2c_driver = {
	.driver = {
		.name = "fan54013",
	},
	.probe = fan54013_probe,
	.remove = __devexit_p(fan54013_remove),
	.resume = fan54013_resume,
	.id_table = fan54013_id,
};

static int __init fan54013_init(void)
{
	int err;

#if defined(CONFIG_MACH_DELOS_CTC)
	if(board_hw_revision>=4)
	{
		pr_info("[FAN54013] %s is not used at NEW version\n", __func__);
		return 0;
	}
#endif

	pr_info("[FAN54013] %s\n", __func__);
	
	err = i2c_add_driver(&fan54013_i2c_driver);
	if (err)
		pr_info("%s[FAN54013] add driver failed "
		       "(err: %d)\n", __func__, err);
	else
		pr_info("%s[FAN54013] added driver %s, (err: %d)\n",
		       __func__, fan54013_i2c_driver.driver.name, err);
	return err;
}
module_init(fan54013_init);

static void __exit fan54013_exit(void)
{
	i2c_del_driver(&fan54013_i2c_driver);
}
module_exit(fan54013_exit);
