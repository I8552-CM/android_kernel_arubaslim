/*
 *  Copyright (C) 2012 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __BQ24157_CHARGER_H_
#define __BQ24157_CHARGER_H_

#include <linux/power/bq24157_private.h>

#define SEC_CHARGER_I2C_SLAVEADDR	0x6A

/* Charge current */
#define BQ24157_CURR_NONE (-1)
/* Argument value of "set_charge_current" */
enum bq24157_55mohm_t {
	BQ24157_55m_CURR_680 = 0, //default
	BQ24157_55m_CURR_804,
	BQ24157_55m_CURR_927,
	BQ24157_55m_CURR_1051,
	BQ24157_55m_CURR_1145,
	BQ24157_55m_CURR_1269,
	BQ24157_55m_CURR_1392,
	BQ24157_55m_CURR_1516,

	BQ24157_55m_ITERM_62 = 0,
	BQ24157_55m_ITERM_124,	//default
	BQ24157_55m_ITERM_186,
	BQ24157_55m_ITERM_248,
	BQ24157_55m_ITERM_309,
	BQ24157_55m_ITERM_371,
	BQ24157_55m_ITERM_433,
	BQ24157_55m_ITERM_495,

	BQ24157_55m_VMCHG_680 = 0,
	BQ24157_55m_VMCHG_804,
	BQ24157_55m_VMCHG_927,
	BQ24157_55m_VMCHG_1051,
	BQ24157_55m_VMCHG_1145,	//deafult
	BQ24157_55m_VMCHG_1269,
	BQ24157_55m_VMCHG_1392,
	BQ24157_55m_VMCHG_1516,
	BQ24157_55m_VMCHG_1669,
	BQ24157_55m_VMCHG_1793,
	BQ24157_55m_VMCHG_1916,
};	/* BQ24157 + 55mohm */

enum bq24157_68mohm_t {
	BQ24157_68m_CURR_550 = 0, //default
	BQ24157_68m_CURR_650,
	BQ24157_68m_CURR_750,
	BQ24157_68m_CURR_850,
	BQ24157_68m_CURR_950,
	BQ24157_68m_CURR_1050,
	BQ24157_68m_CURR_1150,
	BQ24157_68m_CURR_1250,

	BQ24157_68m_ITERM_50 = 0,
	BQ24157_68m_ITERM_100,	//default
	BQ24157_68m_ITERM_150,
	BQ24157_68m_ITERM_200,
	BQ24157_68m_ITERM_250,
	BQ24157_68m_ITERM_300,
	BQ24157_68m_ITERM_350,
	BQ24157_68m_ITERM_400,

	BQ24157_68m_VMCHG_550 = 0,
	BQ24157_68m_VMCHG_650,
	BQ24157_68m_VMCHG_750,
	BQ24157_68m_VMCHG_850,
	BQ24157_68m_VMCHG_950,	//deafult
	BQ24157_68m_VMCHG_1050,
	BQ24157_68m_VMCHG_1150,
	BQ24157_68m_VMCHG_1250,
	BQ24157_68m_VMCHG_1350,
	BQ24157_68m_VMCHG_1450,
	BQ24157_68m_VMCHG_1550,
};	/* BQ24157 + 68mohm */

enum bq24157_100m_t {
	BQ24157_100m_CURR_374 = 0, //default
	BQ24157_100m_CURR_442,
	BQ24157_100m_CURR_510,
	BQ24157_100m_CURR_578,
	BQ24157_100m_CURR_646,
	BQ24157_100m_CURR_714,
	BQ24157_100m_CURR_782,
	BQ24157_100m_CURR_850,

	BQ24157_100m_ITERM_34 = 0,
	BQ24157_100m_ITERM_68,	//default
	BQ24157_100m_ITERM_102,
	BQ24157_100m_ITERM_136,
	BQ24157_100m_ITERM_170,
	BQ24157_100m_ITERM_204,
	BQ24157_100m_ITERM_268,
	BQ24157_100m_ITERM_272,

	BQ24157_100m_VMCHG_374 = 0,
	BQ24157_100m_VMCHG_442,
	BQ24157_100m_VMCHG_510,
	BQ24157_100m_VMCHG_578,
	BQ24157_100m_VMCHG_646,	//deafult
	BQ24157_100m_VMCHG_714,
	BQ24157_100m_VMCHG_782,
	BQ24157_100m_VMCHG_850,
	BQ24157_100m_VMCHG_918,
	BQ24157_100m_VMCHG_986,
	BQ24157_100m_VMCHG_1054,
};	/* BQ24157 + 100mohm */

enum bq24157_VRsense_t {	
	BQ24157_VRSENSE_55m = 0,
	BQ24157_VRSENSE_68m,
	BQ24157_VRSENSE_100m,
};

enum bq24157_fault_t {
	BQ24157_FAULT_NORMAL = 0,
	BQ24157_FAULT_VBUS_OVP,
	BQ24157_FAULT_SLEEP_MODE,
	BQ24157_FAULT_POOR_INPUT,
	BQ24157_FAULT_BATT_OVP,
	BQ24157_FAULT_THERMAL,
	BQ24157_FAULT_TIMER,
	BQ24157_FAULT_NO_BATT,
};

#define CHG_CURR_NONE		(-1)
#define CHG_CURR_TA		0
#define CHG_CURR_TA_EVENT	1
#define CHG_CURR_USB		2

#define CHG_STOP			0
#define CHG_START			1
#ifdef CONFIG_MSM_BACKGROUND_CHARGING_EX_CHARGER
#define CHG_RE_START		2
#define CHG_BACK_START		3
#endif

struct bq24157_platform_data {
	/* Rx functions from host */
#ifdef CONFIG_MSM_BACKGROUND_CHARGING_EX_CHARGER
	int (*start_charging)(int curr, int control);
#else
	int (*start_charging)(int curr);
#endif
	int (*stop_charging)(void);
	int (*get_fault)(void);
	int (*get_status)(void);

	/* Tx functions to host */
	void (*tx_charge_done)(void);
	void (*tx_charge_fault)(enum bq24157_fault_t reason);

	int Rsense;		// Sensing register in charger IC

	u8 VMchrg;		// Maximum set charge current
	int chg_curr_ta;	// Charging current TA
	int chg_curr_usb;	// Charging currnet USB
	u8 Iterm;		// Termination current
#ifdef CONFIG_MSM_BACKGROUND_CHARGING_EX_CHARGER
	u8 Iterm_2nd;	// Termination current for recharging
#endif

	u8 VMreg;		// Maximum battery regulation voltage
	u8 VOreg;		// Battery regulation voltage(Charge voltage range)
};

#endif
