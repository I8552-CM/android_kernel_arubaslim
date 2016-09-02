/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */


#ifndef __MSM_BATTERY_H__
#define __MSM_BATTERY_H__

#define NO_CHG     0x00000000
#define AC_CHG     0x00000001
#define USB_CHG    0x00000002
#ifdef CONFIG_CHARGER_FAN54013
#include <linux/power/fan54013_charger.h>
#endif
#ifdef CONFIG_CHARGER_BQ24157
#include <linux/power/bq24157_charger.h>
#endif
#ifdef CONFIG_CHARGER_SMB328A
#include <linux/power/smb328a_charger.h>
#endif

enum cable_type_t {
	CABLE_TYPE_UNKNOWN = 0,
	CABLE_TYPE_USB,
	CABLE_TYPE_TA,
    CABLE_TYPE_USB_OTG,
};

enum acc_type_t {
	ACC_TYPE_NONE = 0,
	ACC_TYPE_USB,
	ACC_TYPE_CHARGER,
	ACC_TYPE_CAR_DOCK,
	ACC_TYPE_DESK_DOCK,
	ACC_TYPE_JIG,
};

enum ovp_type_t {
	OVP_TYPE_NONE = 0,
	OVP_TYPE_OVP,
	OVP_TYPE_RECOVER,
};

extern bool power_down;
extern int charging_boot;

struct msm_battery_callback {
	void (*set_cable)(struct msm_battery_callback *ptr,
		enum cable_type_t status);
	void (*set_acc_type)(struct msm_battery_callback *ptr,
		enum acc_type_t status);
	void (*set_ovp_type)(struct msm_battery_callback *ptr,
		enum ovp_type_t status);
#if defined(CONFIG_CHARGER_SMB328A) || defined(CONFIG_CHARGER_FAN54013) || defined(CONFIG_CHARGER_BQ24157)
	void (*charge_done)(struct msm_battery_callback *ptr);	/* switching charger */
#endif
#if defined(CONFIG_CHARGER_FAN54013) || defined(CONFIG_CHARGER_BQ24157)
	void (*charge_fault)(struct msm_battery_callback *ptr, int reason);
#endif
#if defined(CONFIG_CHARGER_SMB328A)
void (*bat_missing)(struct max8998_charger_callbacks *ptr);
#endif
};

struct msm_charger_data {
	struct power_supply *psy_fuelgauge;
	void (*register_callbacks)(struct msm_battery_callback *ptr);

#if defined (CONFIG_MACH_DELOS_CTC)
	struct fan54013_platform_data *charger_ic;	/* FAN54013 switching charger */
	struct smb328a_platform_data *charger_ic2;	/* SMB328A switching charger */
#else  /* else of CONFIG_MACH_DELOS_CTC */

#if defined(CONFIG_CHARGER_FAN54013)
	struct fan54013_platform_data *charger_ic;	/* FAN54013 switching charger */
#elif defined(CONFIG_CHARGER_BQ24157)
	struct bq24157_platform_data *charger_ic;	/* BQ24157 switching charger */
#elif defined(CONFIG_CHARGER_SMB328A)
	struct smb328a_platform_data *charger_ic;	/* SMB328A switching charger */
#endif

#endif  /* end of CONFIG_MACH_DELOS_CTC */
};

struct msm_psy_batt_pdata {
	struct msm_charger_data	*charger;
	u32 voltage_max_design;
	u32 voltage_min_design;
	u32 voltage_fail_safe;
	u32 avail_chg_sources;
	u32 batt_technology;
	u32 (*calculate_capacity)(u32 voltage);
};

int msm_battery_fuel_alert(void);

#endif
