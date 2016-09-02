/* /linux/drivers/misc/modem_if/modem_modemctl_device_msm7x27.c
 *
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
/* #define DEBUG */
#include <linux/init.h>

#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/delay.h>

#include <linux/platform_data/modem.h>
#include <mach/msm_smsm.h>
#include "modem_prj.h"
#include "modem_link_device_dpram.h"

static int msm7x27_on(struct modem_ctl *mc)
{
	struct dpram_link_device *dpram_ld =
		to_dpram_link_device(get_current_link(mc->iod));

	pr_debug("[MIF] <%s> Called!!\n", __func__);

	dpram_ld->dpram_init_status = DPRAM_INIT_STATE_READY;
	dpram_ld->cmd_phone_start_handler(dpram_ld);
	complete_all(&dpram_ld->dpram_init_cmd);

	//mc->phone_state = STATE_BOOTING;
	pr_info("[MIF] %s: DONE!\n", __func__);

	return 0;
}

static int msm7x27_off(struct modem_ctl *mc)
{
	pr_info("[MIF] <%s> Called!!\n", __func__);
	return 0;
}

static int msm7x27_reset(struct modem_ctl *mc)
{
	struct dpram_link_device *dpram_ld =
		to_dpram_link_device(get_current_link(mc->iod));
	const u16 reboot_magic_code = 0x3569;

	pr_info("[MIF] <%s> Called!!\n", __func__);

	dpram_ld->magic = reboot_magic_code;

	msleep(100);
	smsm_reset_modem(SMSM_SYSTEM_DOWNLOAD);

	return 0;
}

static int msm7x27_boot_on(struct modem_ctl *mc)
{
	pr_info("[MIF] <%s> Called!!\n", __func__);
	/* Do not use this func in msm8x55 */
	return 0;
}

static int msm7x27_boot_off(struct modem_ctl *mc)
{
	pr_info("[MIF] <%s> Called!!\n", __func__);
	/* Do not use this func in msm8x55 */
	return 0;
}

static int msm7x27_force_crash_exit(struct modem_ctl *mc)
{
	pr_info("[MIF] <%s> Called!!\n", __func__);
	/* Do not use this func in msm8x55 */
	return 0;
}

static int msm7x27_dump_reset(struct modem_ctl *mc)
{
	pr_info("[MIF] <%s> Called!!\n", __func__);
	/* Do not use this func in msm8x55 */
	return 0;
}

static void msm7x27_get_ops(struct modem_ctl *mc)
{
	mc->ops.modem_on = msm7x27_on;
	mc->ops.modem_off = msm7x27_off;
	mc->ops.modem_reset = msm7x27_reset;
	mc->ops.modem_boot_on = msm7x27_boot_on;
	mc->ops.modem_boot_off = msm7x27_boot_off;
	mc->ops.modem_force_crash_exit = msm7x27_force_crash_exit;
	mc->ops.modem_dump_reset = msm7x27_dump_reset;
}

int msm7x27_init_modemctl_device(struct modem_ctl *mc,
		struct modem_data *pdata)
{
	pr_info("[MIF] <%s> Called!!\n", __func__);
	msm7x27_get_ops(mc);

	return 0;
}
