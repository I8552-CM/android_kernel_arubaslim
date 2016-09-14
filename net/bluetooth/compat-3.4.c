/*
 * Copyright 2012  Luis R. Rodriguez <mcgrof@frijolero.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Backport functionality introduced in Linux 3.4.
 */

#include <linux/fs.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/wait.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0))
#include <linux/regmap.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0)) */
// @daniel, for hid_alloc_report_buf()
#include <linux/export.h>
#include <linux/hid.h>
#include <linux/bug.h>
// @
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0))

#if defined(CONFIG_REGMAP)
static void devm_regmap_release(struct device *dev, void *res)
{
	regmap_exit(*(struct regmap **)res);
}

#if defined(CONFIG_REGMAP_I2C)
static int regmap_i2c_write(
			    struct device *dev,
			    const void *data,
			    size_t count)
{
	struct i2c_client *i2c = to_i2c_client(dev);
	int ret;

	ret = i2c_master_send(i2c, data, count);
	if (ret == count)
		return 0;
	else if (ret < 0)
		return ret;
	else
		return -EIO;
}

static int regmap_i2c_gather_write(
				   struct device *dev,
				   const void *reg, size_t reg_size,
				   const void *val, size_t val_size)
{
	struct i2c_client *i2c = to_i2c_client(dev);
	struct i2c_msg xfer[2];
	int ret;

	/* If the I2C controller can't do a gather tell the core, it
	 * will substitute in a linear write for us.
	 */
	if (!i2c_check_functionality(i2c->adapter, I2C_FUNC_NOSTART))
		return -ENOTSUPP;

	xfer[0].addr = i2c->addr;
	xfer[0].flags = 0;
	xfer[0].len = reg_size;
	xfer[0].buf = (void *)reg;

	xfer[1].addr = i2c->addr;
	xfer[1].flags = I2C_M_NOSTART;
	xfer[1].len = val_size;
	xfer[1].buf = (void *)val;

	ret = i2c_transfer(i2c->adapter, xfer, 2);
	if (ret == 2)
		return 0;
	if (ret < 0)
		return ret;
	else
		return -EIO;
}

static int regmap_i2c_read(
			   struct device *dev,
			   const void *reg, size_t reg_size,
			   void *val, size_t val_size)
{
	struct i2c_client *i2c = to_i2c_client(dev);
	struct i2c_msg xfer[2];
	int ret;

	xfer[0].addr = i2c->addr;
	xfer[0].flags = 0;
	xfer[0].len = reg_size;
	xfer[0].buf = (void *)reg;

	xfer[1].addr = i2c->addr;
	xfer[1].flags = I2C_M_RD;
	xfer[1].len = val_size;
	xfer[1].buf = val;

	ret = i2c_transfer(i2c->adapter, xfer, 2);
	if (ret == 2)
		return 0;
	else if (ret < 0)
		return ret;
	else
		return -EIO;
}

static struct regmap_bus regmap_i2c = {
	.write = regmap_i2c_write,
	.gather_write = regmap_i2c_gather_write,
	.read = regmap_i2c_read,
};
#endif /* defined(CONFIG_REGMAP_I2C) */

#endif /* defined(CONFIG_REGMAP) */
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0)) */

/* daniel, backport 3.13-1, from backport-3.12.c
 * Allocator for buffer that is going to be passed to hid_output_report()
 */
u8 *hid_alloc_report_buf(struct hid_report *report, gfp_t flags)
{
	/*
	 * 7 extra bytes are necessary to achieve proper functionality
	 * of implement() working on 8 byte chunks
	 */

	int len = ((report->size - 1) >> 3) + 1 + (report->id > 0) + 7;

	return kmalloc(len, flags);
}
EXPORT_SYMBOL_GPL(hid_alloc_report_buf);
