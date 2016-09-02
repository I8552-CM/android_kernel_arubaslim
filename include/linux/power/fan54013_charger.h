/*
 *  Copyright (C) 2009 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __FAN54013_CHARGER_H_
#define __FAN54013_CHARGER_H_

#include <linux/power/fan54013_private.h>

/* Charge current */
/* Argument value of "set_charge_current" */
enum fan54013_68m_iocharge_t {
	FAN54013_CURR_550 = 0,
	FAN54013_CURR_650,
	FAN54013_CURR_750,
	FAN54013_CURR_850,
	FAN54013_CURR_1050,
	FAN54013_CURR_1150,
	FAN54013_CURR_1350,
	FAN54013_CURR_1450,
};	/* FAN54013 + 68mohm */

enum fan54013_100m_iocharge_t {
	FAN54013_CURR_374 = 0,
	FAN54013_CURR_442,
	FAN54013_CURR_510,
	FAN54013_CURR_578,
	FAN54013_CURR_714,
	FAN54013_CURR_782,
	FAN54013_CURR_918,
	FAN54013_CURR_986,
};	/* FAN54013 + 100mohm */

enum fan54103_VRsense_t {	
	FAN54013_VRSENSE_68m = 0,
	FAN54013_VRSENSE_100m,
};

/* See "fan54013_start_charging()" in fan54013_charger.c */
#define CHG_CURR_NONE		(-1)
#define CHG_CURR_TA		0
#define CHG_CURR_TA_EVENT	1
#define CHG_CURR_USB		2

/* IC Information */
/* Return value of "get_ic_info" */
#define CHARGER_IC_FAN5401x	0	//[PN] : XXX
#define CHARGER_IC_FAN54010	1	//[PN] : 001
#define CHARGER_IC_FAN54011	2	//[PN] : 001
#define CHARGER_IC_FAN54012	3	//[PN] : 011
#define CHARGER_IC_FAN54013	4	//[PN] : 101
#define CHARGER_IC_FAN54014	5	//[PN] : 111
#define CHARGER_IC_FAN54015	6	//[PN] : 101

/* Charging fault */
/* Argument value of "tx_charge_fault" */
enum fan54013_fault_t {
	FAN54013_FAULT_NONE = 0,
	FAN54013_FAULT_VBUS_OVP,
	FAN54013_FAULT_SLEEP_MODE,
	FAN54013_FAULT_POOR_INPUT,
	FAN54013_FAULT_BATT_OVP,
	FAN54013_FAULT_THERMAL,
	FAN54013_FAULT_TIMER,
	FAN54013_FAULT_NO_BATT,
};

#define CHG_STOP			0
#define CHG_START			1
#ifdef CONFIG_MSM_BACKGROUND_CHARGING_EX_CHARGER
#define CHG_RE_START		2
#define CHG_BACK_START		3
#endif

struct fan54013_platform_data {
	/* Rx functions from host */
#ifdef CONFIG_MSM_BACKGROUND_CHARGING_EX_CHARGER
	int (*start_charging)(int curr, int control);
#else
	int (*start_charging)(int curr);
#endif
	int (*stop_charging)(void);
	int (*get_vbus_status)(void);
	int (*set_host_status)(bool isAlive);
	int (*get_monitor_bits)(void);	/* support battery test mode (*#0228#) */
	int (*get_fault)(void);
	int (*get_ic_info)(void);
	
	/* Tx functions to host */
	void (*tx_charge_done)(void);
	void (*tx_charge_fault)(enum fan54013_fault_t reason);
	
	int VRsense;				// Sensing register in charger IC

	u8 Isafe;				// Maximum IOCHARGE
	int chg_curr_ta;			// Charging current TA
	int chg_curr_ta_event;
	int chg_curr_usb;			// Charging currnet USB
	u8 Iterm;				// Termination current			
#ifdef CONFIG_MSM_BACKGROUND_CHARGING_EX_CHARGER
	u8 Iterm_2nd;	// Termination current for recharging
#endif
	
	u8 Vsafe;				// Set Maximum VOREG.
	u8 Voreg;				// Battery Voltage Regulation (Charge voltage range)
};

/*

 Host(AP) --> Client(Charger)	: "Rx" from host
 Host(AP) <-- Client(Charger)	: "Tx" to host

*/

#endif
