/*
 * meflas_ts.h
 *
 * Copyright (C) 2009 Samsung Electronics Co.Ltd
 * Author: Joonyoung Shim <jy0922.shim@samsung.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#ifndef __LINUX_MCS7000_TS_H
#define __LINUX_MCS7000_TS_H

struct tsp_callbacks {
	void (*inform_charger)(struct tsp_callbacks *tsp_cb, int mode);
};

struct melfas_platform_data {
	void	(*register_cb)(struct tsp_callbacks *);
};


#endif	/* __LINUX_MCS5000_TS_H */
