/*
 * Copyright (C) 2010 Samsung Electronics
 * Minkyu Kang <mk7.kang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _FSA9480_H_
#define _FSA9480_H_

#include <linux/types.h>

#define FSA9480_ATTACHED (1)
#define FSA9480_DETACHED (0)
#define FSA_ATTACHED	FSA9480_ATTACHED
#define FSA_DETACHED	FSA9480_DETACHED

#define USB_PATH_NA		-1
#define USB_PATH_AP		0
#define USB_PATH_CP		1

#define USB_POWER_AP	"USB_AP"
#define USB_POWER_CP	"USB_CP"

struct fsa9480_ops {
	void (*attach_handler)(int);
	void (*detach_handler)(void);
};

struct fsa9480_platform_data {
       int intb_gpio;
       void (*usb_cb) (u8 attached, struct fsa9480_ops *ops);
       void (*uart_cb) (u8 attached, struct fsa9480_ops *ops);
       void (*charger_cb) (u8 attached, struct fsa9480_ops *ops);
       void (*jig_cb) (u8 attached, struct fsa9480_ops *ops);
	void (*ovp_cb) (u8 attached, struct fsa9480_ops *ops);
       void (*reset_cb) (void);
};

void fsa9480_set_switch(const char *buf);
ssize_t fsa9480_get_switch(char *buf);
int fsa_microusb_connection_handler_register(
		void (*detach), void (*attach));


#endif /* _FSA9480_H_ */
