/*
 *  max17043_fuelgauge.c
 *  fuel-gauge systems for lithium-ion (Li+) batteries
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/power_supply.h>
#include <linux/fuelgauge_max17043.h>
#include <linux/slab.h>

#define MAX17043_VCELL_MSB	0x02
#define MAX17043_VCELL_LSB	0x03
#define MAX17043_SOC_MSB	0x04
#define MAX17043_SOC_LSB	0x05
#define MAX17043_MODE_MSB	0x06
#define MAX17043_MODE_LSB	0x07
#define MAX17043_VER_MSB	0x08
#define MAX17043_VER_LSB	0x09
#define MAX17043_RCONFIG_MSB	0x0C
#define MAX17043_RCONFIG_LSB	0x0D
#define MAX17043_CMD_MSB	0xFE
#define MAX17043_CMD_LSB	0xFF

#define MAX17043_DELAY		1000
#define MAX17043_BATTERY_FULL	95

#ifdef CONFIG_MACH_AMAZING_CDMA
#define SOC_CALC_CONST_1	19531
#define SOC_CALC_CONST_2	10000000
#endif

struct max17043_chip {
	struct i2c_client		*client;
	struct power_supply		battery;
	struct max17043_platform_data	*pdata;
	struct timespec			next_update_time;

	/* State Of Connect */
	int online;
	/* battery voltage */
	int vcell;
	/* battery capacity */
	int soc;
	/* State Of Charge */
	int status;
};


static void max17043_update_values(struct max17043_chip *chip);

static int max17043_get_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    union power_supply_propval *val)
{
	struct max17043_chip *chip = container_of(psy,
				struct max17043_chip, battery);
	struct timespec now;

	ktime_get_ts(&now);
	monotonic_to_bootbased(&now);
	if (timespec_compare(&now, &chip->next_update_time) >= 0)
		max17043_update_values(chip);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = chip->status;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = chip->online;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = chip->vcell;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = chip->soc;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int max17043_write_reg(struct i2c_client *client, int reg, u8 value)
{
	int ret;

	ret = i2c_smbus_write_byte_data(client, reg, value);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}

static int max17043_read_reg(struct i2c_client *client, int reg)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, reg);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}

static void max17043_reset(struct i2c_client *client)
{
	max17043_write_reg(client, MAX17043_CMD_MSB, 0x54);
	max17043_write_reg(client, MAX17043_CMD_LSB, 0x00);
}

/*static int max17043_probe_config(struct i2c_client *client)
{

	printk("%s: initialise CONFIG register\n"__func__);
	max17043_write_reg(client, MAX17043_RCONFIG_MSB, 0xB0);
	max17043_write_reg(client, MAX17043_RCONFIG_LSB, 0x1C);

	return 0;
}*/

static int max17043_set_property(struct power_supply *psy,
                            enum power_supply_property psp,
                            union power_supply_propval *val)
{
        struct max17043_chip *chip = container_of(psy,
                                struct max17043_chip, battery);

        u8 msb;
        u8 lsb;

        switch(psp) {
        case POWER_SUPPLY_PROP_FUELGAUGE_RCONFIG:
                         i2c_smbus_write_word_data(chip->client, MAX17043_RCONFIG_MSB,
                        swab16(val->intval));
                        msb = max17043_read_reg(chip->client, MAX17043_RCONFIG_MSB);
                        lsb = max17043_read_reg(chip->client, MAX17043_RCONFIG_LSB);
                        printk("%s: rconfig = 0x%x 0x%x\n", __func__, msb, lsb);
                break;
        case POWER_SUPPLY_PROP_FUELGAUGE_RESET :
                        max17043_reset(chip->client);
						printk(KERN_DEBUG "Chip Reset %s\n" , __func__);
						break;

        default :
                return -EINVAL;
        }
        return 0;
}

static void max17043_get_vcell(struct i2c_client *client)
{
	struct max17043_chip *chip = i2c_get_clientdata(client);
	u8 msb;
	u8 lsb;

	msb = max17043_read_reg(client, MAX17043_VCELL_MSB);
	lsb = max17043_read_reg(client, MAX17043_VCELL_LSB);

	chip->vcell = ((msb << 4) + (lsb >> 4)) * 1250;
}

static void max17043_get_soc(struct i2c_client *client)
{
	struct max17043_chip *chip = i2c_get_clientdata(client);
	u8 msb;
	u8 lsb;
	int soc;

#ifdef CONFIG_MACH_AMAZING_CDMA

	u32 CalcStep1;
	u32 CalcStep2;
	int socValue;
#else

	int pure_soc, adj_soc;
        int full = chip->pdata->psoc_full;
        int empty = chip->pdata->psoc_empty;

        if (full == 0)
                full = 100;
        if (empty == 0)
                empty = 0;
#endif

	msb = max17043_read_reg(client, MAX17043_SOC_MSB);
	lsb = max17043_read_reg(client, MAX17043_SOC_LSB);

#ifdef CONFIG_MACH_AMAZING_CDMA
	CalcStep1 = (msb*256) + lsb;
	CalcStep2 = CalcStep1 * SOC_CALC_CONST_1;
	socValue  = CalcStep2 / SOC_CALC_CONST_2;
	if (socValue >= 0)
		soc = ((((socValue*10)-0)/10)*100)/(((995)-0)/10);
	else
		soc = 0;
#else

	pure_soc = msb * 100 + (lsb * 100) / 256;

        if (pure_soc >= 0)
                adj_soc = ((pure_soc * 10000) - empty) / (full - empty);
        else
                adj_soc = 0;

        soc = adj_soc / 100;

        if (adj_soc % 100 >= 50)        // ¹Ý¿Ã¸²
                soc += 1;

#endif

		if (soc > 100)
			soc = 100;
		/*error handling(i2c fail when battery low)*/
		if (msb == 0 && lsb == 0)
			soc = chip->soc;
		chip->soc = min(soc, (uint)100);



}

static void max17043_get_version(struct i2c_client *client)
{
	u8 msb;
	u8 lsb;

	msb = max17043_read_reg(client, MAX17043_VER_MSB);
	lsb = max17043_read_reg(client, MAX17043_VER_LSB);

	dev_info(&client->dev, "MAX17043 Fuel-Gauge Ver %d%d\n", msb, lsb);
}

static void max17043_get_online(struct i2c_client *client)
{
	struct max17043_chip *chip = i2c_get_clientdata(client);

	if (chip->pdata->battery_online)
		chip->online = chip->pdata->battery_online();
	else
		chip->online = 1;
}

static void max17043_get_status(struct i2c_client *client)
{
	struct max17043_chip *chip = i2c_get_clientdata(client);

	if (!chip->pdata->charger_online || !chip->pdata->charger_enable) {
		chip->status = POWER_SUPPLY_STATUS_UNKNOWN;
		return;
	}

	if (chip->pdata->charger_online()) {
		if (chip->pdata->charger_enable())
			chip->status = POWER_SUPPLY_STATUS_CHARGING;
		else
			chip->status = POWER_SUPPLY_STATUS_NOT_CHARGING;
	} else {
		chip->status = POWER_SUPPLY_STATUS_DISCHARGING;
	}

	if (chip->soc > MAX17043_BATTERY_FULL)
		chip->status = POWER_SUPPLY_STATUS_FULL;
}

static void max17043_update_values(struct max17043_chip *chip)
{
	max17043_get_vcell(chip->client);
	max17043_get_soc(chip->client);
	max17043_get_online(chip->client);
	max17043_get_status(chip->client);

	/* next update time must be atleast 1 sec or later  */
	ktime_get_ts(&chip->next_update_time);
	monotonic_to_bootbased(&chip->next_update_time);

	chip->next_update_time.tv_sec++ ;
	printk("%s chip vcell %d, chip soc %d\n", __func__, chip->vcell, chip->soc);
}


static enum power_supply_property max17043_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
};

static int __devinit max17043_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct max17043_chip *chip;
	int ret;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE))
		return -EIO;

	chip = kzalloc(sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->client = client;
	chip->pdata = client->dev.platform_data;

	i2c_set_clientdata(client, chip);

	chip->battery.name		= "battery";
	chip->battery.type		= POWER_SUPPLY_TYPE_BATTERY;
	chip->battery.get_property	= max17043_get_property;
	chip->battery.set_property	= max17043_set_property;
	chip->battery.properties	= max17043_battery_props;
	chip->battery.num_properties	= ARRAY_SIZE(max17043_battery_props);

	if (chip->pdata && chip->pdata->power_supply_register)
		ret = chip->pdata->power_supply_register(&client->dev, &chip->battery);
	else
		ret = power_supply_register(&client->dev, &chip->battery);
	if (ret) {
		dev_err(&client->dev, "failed: power supply register\n");
		kfree(chip);
		return ret;
	}

#ifdef CONFIG_MACH_AMAZING_CDMA
	max17043_update_values(chip);
	max17043_get_version(client);
#else
	max17043_reset(client);
	/*max17043_probe_config(client);TODO */
	max17043_get_version(client);
	if (chip->pdata)
		i2c_smbus_write_word_data(client, MAX17043_RCONFIG_MSB,
				swab16(chip->pdata->rcomp_value));

#endif

	return 0;
}

static int __devexit max17043_remove(struct i2c_client *client)
{
	struct max17043_chip *chip = i2c_get_clientdata(client);

	if (chip->pdata && chip->pdata->power_supply_unregister)
		chip->pdata->power_supply_unregister(&chip->battery);
	power_supply_unregister(&chip->battery);
	kfree(chip);
	return 0;
}

#ifdef CONFIG_PM

static int max17043_suspend(struct i2c_client *client,
		pm_message_t state)
{
	struct max17043_chip *chip = i2c_get_clientdata(client);

	return 0;
}

static int max17043_resume(struct i2c_client *client)
{
	struct max17043_chip *chip = i2c_get_clientdata(client);

	return 0;
}

#else

#define max17043_suspend NULL
#define max17043_resume NULL

#endif /* CONFIG_PM */

static const struct i2c_device_id max17043_id[] = {
	{ "max17043", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, max17043_id);

static struct i2c_driver max17043_i2c_driver = {
	.driver	= {
		.name	= "max17043",
	},
	.probe		= max17043_probe,
	.remove		= __devexit_p(max17043_remove),
	.suspend	= max17043_suspend,
	.resume		= max17043_resume,
	.id_table	= max17043_id,
};

static int __init max17043_init(void)
{
	return i2c_add_driver(&max17043_i2c_driver);
}
module_init(max17043_init);

static void __exit max17043_exit(void)
{
	i2c_del_driver(&max17043_i2c_driver);
}
module_exit(max17043_exit);

//MODULE_AUTHOR("Minkyu Kang <mk7.kang@samsung.com>");
//MODULE_DESCRIPTION("MAX17040 Fuel Gauge");
//MODULE_LICENSE("GPL");
