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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/power/smb328a_charger.h>
#include <linux/delay.h>

extern unsigned int board_hw_revision;

static struct i2c_client	*smb328a_i2c_client;
static struct work_struct	chg_work;
static struct timer_list	chg_work_timer;
static struct workqueue_struct	*chg_wqueue;

#define SMB328A_TIMER_INTERVAL	(10 * 1000 * 60)	/* 10 min */

#define REGISTER_00	0x00
#define REGISTER_01	0x01
#define REGISTER_02	0x02
#define REGISTER_03	0x03
#define REGISTER_04	0x04
#define REGISTER_05	0x05
#define REGISTER_06	0x06
#define REGISTER_07	0x07
#define REGISTER_08	0x08
#define REGISTER_09	0x09
#define REGISTER_0A	0x0A
#define REGISTER_30	0x30
#define REGISTER_31	0x31
#define REGISTER_32	0x32
#define REGISTER_33	0x33
#define REGISTER_34	0x34
#define REGISTER_35	0x35
#define REGISTER_36	0x36
#define REGISTER_37	0x37
#define REGISTER_38	0x38
#define REGISTER_39	0x39

static void _smb328a_print_register(void)
{
	int i = 0;
	s32 reg_0x[11];	// 0x00~0x0A
	s32 reg_3x[10];	// 0x30~0x39

	for (i = 0x00; i <= 0x0A; i++)
		reg_0x[i] = i2c_smbus_read_byte_data(smb328a_i2c_client, i);

	for (i = 0x30; i <= 0x39; i++)
		reg_3x[i-0x30] = i2c_smbus_read_byte_data(smb328a_i2c_client, i);

	pr_info("/SMB328A/ 00~0A: 0=0x%x,1=0x%x,2=0x%x,3=0x%x,4=0x%x,5=0x%x,6=0x%x,7=0x%x,8=0x%x,9=0x%x,A=0x%x\n",
		reg_0x[0],reg_0x[1],reg_0x[2],reg_0x[3],reg_0x[4],reg_0x[5],
		reg_0x[6],reg_0x[7],reg_0x[8],reg_0x[9],reg_0x[10]);

	pr_info("/SMB328A/ 30~39: 30=0x%x,31=0x%x,32=0x%x,33=0x%x,34=0x%x,35=0x%x,36=0x%x,37=0x%x,38=0x%x,39=0x%x\n",
		reg_3x[0],reg_3x[1],reg_3x[2],reg_3x[3],reg_3x[4],reg_3x[5],
		reg_3x[6],reg_3x[7],reg_3x[8],reg_3x[9]);
}

static void _smb328a_rw_register(int addr, u8 clear, int shift, u8 value)
{
	u8 data = 0;

	data = i2c_smbus_read_byte_data(smb328a_i2c_client, addr);
	data = (data & ~(clear << shift)) | (value << shift);
	i2c_smbus_write_byte_data(smb328a_i2c_client, addr, data);

	pr_debug("smb328a: write REGISTER_%x -> 0x%x\n", addr, data);
}

static void _smb328a_clear_irq(void)
{
	pr_info("%s\n",__func__);
	/* Write any data for clearing irq */
	i2c_smbus_write_byte_data(smb328a_i2c_client, REGISTER_30, 0x1);
}

static void _smb328a_chg_work(struct work_struct *work)
{
	u8 irq_a, irq_b, irq_c;

	pr_info("%s start \n",__func__);

	/* Read all IRQ registers */
	irq_a = i2c_smbus_read_byte_data(smb328a_i2c_client, REGISTER_32);
	irq_b = i2c_smbus_read_byte_data(smb328a_i2c_client, REGISTER_34);
	irq_c = i2c_smbus_read_byte_data(smb328a_i2c_client, REGISTER_37);
	pr_info("%s irq a=%x b=%x c=%x\n", __func__, irq_a, irq_b, irq_c);

	/* Check no IRQ */
	if (irq_a == 0x00 && irq_b == 0x00 && irq_c == 0x00) {
		/* Clear IRQ (R30h) */
		_smb328a_clear_irq();
	}

	mod_timer(&chg_work_timer, jiffies + msecs_to_jiffies(SMB328A_TIMER_INTERVAL));

	pr_info("%s end \n",__func__);
}

static void _smb328a_chg_work_timer_func(unsigned long param)
{
	pr_info("%s start \n",__func__);
	queue_work(chg_wqueue, &chg_work);
	pr_info("%s end \n",__func__);
}

static void _smb328a_control_timer(bool enable)
{
	pr_info("%s start \n",__func__);
	
	if (!smb328a_i2c_client)
		return ;

	if (enable)
		queue_work(chg_wqueue, &chg_work);
	else
		del_timer_sync(&chg_work_timer);
	pr_info("%s end \n",__func__);
}

static void _smb328a_custom_charger(void)
{
	pr_info("%s start \n",__func__);
	/* Allow volatile writes */
	_smb328a_rw_register(REGISTER_31, 0x1, 7, 0x1);
//Disable the charging before seting the charging parameter
#if defined (CONFIG_MACH_DELOS_CTC)
	_smb328a_rw_register(REGISTER_31, 0x1, 4, 0x1);
#endif
	/* AC mode */
	_smb328a_rw_register(REGISTER_31, 0x1, 2, 0x1);

	/* Enable(EN) control */
	_smb328a_rw_register(REGISTER_05, 0x3, 2, 0x0);

	/* Fast charge current control method */
	_smb328a_rw_register(REGISTER_05, 0x1, 1, 0x0);

	/* Input current limit control method */
	_smb328a_rw_register(REGISTER_05, 0x1, 0, 0x0);

	/* Enable IRQ */
	_smb328a_rw_register(REGISTER_04, 0x1, 0, 0x1);

	/* Disable auto recharge */
	_smb328a_rw_register(REGISTER_03, 0x1, 7, 0x1);

	/* Disable watchdog timer */
	_smb328a_rw_register(REGISTER_04, 0x1, 1, 0x0);

	/* Disable charge safety timers */
	_smb328a_rw_register(REGISTER_04, 0x1, 3, 0x1);

	/* Enable AICL */ 
	_smb328a_rw_register(REGISTER_01, 0x1, 2, 0x0);

	/* Set AICL threshold voltage to 4.25V */
#if defined (CONFIG_MACH_DELOS_CTC)
	_smb328a_rw_register(REGISTER_01, 0x3, 0, 0x0);
#endif

	/* Float voltage */
#if defined (CONFIG_MACH_DELOS_CTC)
	_smb328a_rw_register(REGISTER_02, 0x7F, 0, 0x58); //4.34V
#else  /* else of CONFIG_MACH_DELOS_CTC */

#ifdef CONFIG_MACH_CHIEF
#ifdef CONFIG_TIKAL_MPCS
	_smb328a_rw_register(REGISTER_02, 0x7F, 0, 0x4C);   // 4.22V
#else	
	_smb328a_rw_register(REGISTER_02, 0x7F, 0, 0x4A);	// 4.20V
#endif	
#else
	_smb328a_rw_register(REGISTER_02, 0x7F, 0, 0x4C);   // 4.22V
#endif

#endif  /* end of CONFIG_MACH_DELOS_CTC */

	pr_info("%s end \n",__func__);
}

static int _smb328a_set_fast_charge_current(enum smb328a_current_t curr)
{
	u8 ac_limit = 0;
	static char *curr_text[] = {
		"500", "600", "700", "800", "900", "1000", "1100", "1200"
	};
	static char *limit_text[] = {
		"450", "600", "700", "800", "900", "1000", "1100", "1200"
	};

	/* Set fast charge current */
	pr_info("%s: CURR = %s mA\n", __func__, curr_text[curr]);
	_smb328a_rw_register(REGISTER_00, 0x7, 5, curr);
        /*set the pre charge current 125mA for tikal mpcs*/	
	#ifdef CONFIG_TIKAL_MPCS
	_smb328a_rw_register(REGISTER_00, 0x3, 3, 0x3);
        #endif 

	/* Set input current limit */
	if (curr == CHG_CURR_USB)
		ac_limit = CHG_AC_LIMIT_USB;
	else
		ac_limit = CHG_AC_LIMIT_TA;

	pr_info("%s: INLIM = %s mA\n", __func__, limit_text[ac_limit]);
	_smb328a_rw_register(REGISTER_01, 0x7, 5, ac_limit);

	pr_info("%s end \n",__func__);
	
	return curr;
}

static int _smb328a_set_termination_current(void)
{
	static char *term_text[] = {
		"25", "50", "75", "100", "125", "150", "175", "200"
	};

	pr_info("%s start \n",__func__);
	pr_info("%s: ITERM = %s mA\n", __func__, term_text[CHG_TERM_CURR]);

	_smb328a_rw_register(REGISTER_00, 0x7, 0, CHG_TERM_CURR);
	_smb328a_rw_register(REGISTER_07, 0x7, 5, CHG_TERM_CURR);

	/* Termination and Taper charging state triggers irq signal */
	_smb328a_rw_register(REGISTER_09, 0x1, 4, 0x1);

	pr_info("%s end \n",__func__);

	return 0;
}

static int _smb328a_charging_control(int enable)
{
	pr_info("%s start \n",__func__);
	/* Battery charge enable */
	if (enable){
		_smb328a_rw_register(REGISTER_31, 0x1, 4, 0x0);
		/* BMD use hardwre PIN */
		_smb328a_rw_register(REGISTER_05, 0x1, 7, 0x0);
		
		/* Enable battery missing detection */
		_smb328a_rw_register(REGISTER_06, 0x1, 7, 0x1);

		/* Trigger interrupt signal for missing battery */
		_smb328a_rw_register(REGISTER_09, 0x1, 5, 0x1);
	}
	else
	{
		/* Disable battery missing detection */
		_smb328a_rw_register(REGISTER_06, 0x1, 7, 0x0);

		/* Disable Trigger interrupt signal for missing battery */
		_smb328a_rw_register(REGISTER_09, 0x1, 5, 0x0);

		_smb328a_rw_register(REGISTER_31, 0x1, 4, 0x1);
	}
	
	pr_info("%s end \n",__func__);
	
	return 0;
}

static int smb328a_start_charging(enum smb328a_current_t curr)
{
	pr_info("%s !(start)\n", __func__);
	_smb328a_print_register();
	_smb328a_custom_charger();
	_smb328a_set_fast_charge_current(curr);
	_smb328a_set_termination_current();
	_smb328a_charging_control(1);

	_smb328a_control_timer(true);

	pr_info("%s !(end)\n", __func__);
	_smb328a_print_register();

	return 0;
}

static int smb328a_stop_charging(void)
{

	pr_info("%s start \n",__func__);
	
	_smb328a_charging_control(0);
	_smb328a_custom_charger();

	_smb328a_control_timer(false);
	_smb328a_print_register();

	pr_info("%s end \n",__func__);

	return 0;
}

static int smb328a_get_vbus_status(void)
{
	int value = 0, data = 0;

	pr_info("%s start \n",__func__);

	value = i2c_smbus_read_byte_data(smb328a_i2c_client, REGISTER_33);

	data = (value & 0x02);

	pr_info("%s data=%d\n",__func__, data);

	if (data)
		return 1;
	else
		return 0;
}

static irqreturn_t _smb328a_isr(int irq, void *data)
{
	struct smb328a_platform_data *pdata = data;
	u8 value = 0, value2 = 0, value3 = 0;

	pr_info("%s start \n",__func__);

	value = i2c_smbus_read_byte_data(smb328a_i2c_client, REGISTER_32);
	value2 = i2c_smbus_read_byte_data(smb328a_i2c_client, REGISTER_34);
	value3 = i2c_smbus_read_byte_data(smb328a_i2c_client, REGISTER_37);
	printk("%s irq status => (32h):0x%x  (34h):0x%x  (37h):0x%x\n", __func__,value, value2, value3);

	/* Check current termination IRQ */
	if (value & 0x08) {
		pr_info("%s: Current Termination\n", __func__);
		pdata->tx_charge_done();
	} else if (value & 0x04) {
		pr_info("%s: Taper Charging\n", __func__);
	} 

	value = i2c_smbus_read_byte_data(smb328a_i2c_client, REGISTER_34);

	if(value & 0x01){
		pr_info("%s: Battery Missing\n", __func__);
		pdata->bat_missing();		
	}

	/* Clear IRQ */
	_smb328a_clear_irq();

	pr_info("%s end \n",__func__);

	return IRQ_HANDLED;
}

static int __devinit smb328a_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct smb328a_platform_data *pdata = client->dev.platform_data;
	int ret = 0;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -EIO;

	pr_info("+++++++++++++++ %s +++++++++++++++\n", __func__);

	smb328a_i2c_client = client;

	pdata->start_charging = smb328a_start_charging;
	pdata->stop_charging = smb328a_stop_charging;
	pdata->get_vbus_status = smb328a_get_vbus_status;

	INIT_WORK(&chg_work, _smb328a_chg_work);
	setup_timer(&chg_work_timer, _smb328a_chg_work_timer_func, (unsigned long)pdata);
	chg_wqueue = create_freezable_workqueue("smb328a_wqueue");
	
	ret = request_threaded_irq(client->irq, NULL, _smb328a_isr,
				   IRQF_TRIGGER_RISING,
				   "smb328a-isr", pdata);
	pr_info("%s, ret=%x\n", __func__, ret);

	_smb328a_print_register();
	return 0;
}

static int __devexit smb328a_remove(struct i2c_client *client)
{
        return 0;
}

static const struct i2c_device_id smb328a_id[] = {
	{"smb328a", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, smb328a_id);

static struct i2c_driver smb328a_i2c_driver = {
	.driver = {
		.name = "smb328a",
	},
	.probe = smb328a_probe,
	.remove = __devexit_p(smb328a_remove),
	.id_table = smb328a_id,
};

static int __init smb328a_init(void)
{
#if defined (CONFIG_MACH_DELOS_CTC)
	if(board_hw_revision<4)
	{
		pr_info("%s not used at OLD version\n", __func__);
		return 0;
	}
#endif
	return i2c_add_driver(&smb328a_i2c_driver);
}
module_init(smb328a_init);

static void __exit smb328a_exit(void)
{
	i2c_del_driver(&smb328a_i2c_driver);
}
module_exit(smb328a_exit);

