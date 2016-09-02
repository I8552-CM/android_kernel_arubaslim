/* Fuel Gauge
*/

#ifndef __MAX17043_BATTERY_H_
#define __MAX17043_BATTERY_H_

struct max17043_platform_data {
	int (*battery_online)(void);
	int (*charger_online)(void);
	int (*charger_enable)(void);
	int (*power_supply_register)(struct device *parent,
		struct power_supply *psy);
	void (*power_supply_unregister)(struct power_supply *psy);
	u16 rcomp_value;
	int psoc_full;
	int psoc_empty;
};

#endif
