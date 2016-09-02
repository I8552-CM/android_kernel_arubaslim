/* linux/arch/arm/mach-xxxx/board-amazing-cdma-modems.c
 * Copyright (C) 2010 Samsung Electronics. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#define DEBUG
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <mach/msm_smsm.h>
#include <mach/msm_iomap.h>
#include <linux/platform_data/modem.h>

#define MSM_A2M_INT(n) (MSM_CSR_BASE + 0x400 + (n) * 4)

#define INT_MASK_REQ_ACK_F	0x0020
#define INT_MASK_REQ_ACK_R	0x0010
#define INT_MASK_RES_ACK_F	0x0008
#define INT_MASK_RES_ACK_R	0x0004
#define INT_MASK_SEND_F		0x0002
#define INT_MASK_SEND_R		0x0001

#define INT_MASK_REQ_ACK_RFS	0x0400	/* Request RES_ACK_RFS           */
#define INT_MASK_RES_ACK_RFS	0x0200	/* Response of REQ_ACK_RFS       */
#define INT_MASK_SEND_RFS		0x0100	/* Indicate sending RFS data     */

#define DPRAM_START_ADDRESS	0x0
#define DPRAM_SIZE              0x8000
#define DPRAM_END_ADDRESS	(DPRAM_START_ADDRESS + DPRAM_SIZE - 1)

#define MSM_DP_FMT_TX_BUFF_SZ	1020
#define MSM_DP_RAW_TX_BUFF_SZ	7160
#define MSM_DP_FMT_RX_BUFF_SZ	1020
#define MSM_DP_RAW_RX_BUFF_SZ	23544

#define MAX_MSM_EDPRAM_IPC_DEV	2	/* FMT, RAW */

struct msm_edpram_ipc_cfg {
	u16 magic;
	u16 access;

	u16 fmt_tx_head;
	u16 fmt_tx_tail;
	u8 fmt_tx_buff[MSM_DP_FMT_TX_BUFF_SZ];

	u16 raw_tx_head;
	u16 raw_tx_tail;
	u8 raw_tx_buff[MSM_DP_RAW_TX_BUFF_SZ];

	u16 fmt_rx_head;
	u16 fmt_rx_tail;
	u8 fmt_rx_buff[MSM_DP_FMT_RX_BUFF_SZ];

	u16 raw_rx_head;
	u16 raw_rx_tail;
	u8 raw_rx_buff[MSM_DP_RAW_RX_BUFF_SZ];

	u16 mbx_ap2cp;
	u16 mbx_cp2ap;
};

#define DP_BOOT_CLEAR_OFFSET	0
#define DP_BOOT_RSRVD_OFFSET	0
#define DP_BOOT_SIZE_OFFSET		0
#define DP_BOOT_TAG_OFFSET		0
#define DP_BOOT_COUNT_OFFSET	0

#define DP_BOOT_FRAME_SIZE_LIMIT 0

static struct dpram_ipc_map msm_ipc_map;

static struct modemlink_dpram_control msm_edpram_ctrl = {
	.dp_type = EXT_DPRAM,

	.dpram_irq = MSM8625_INT_A9_M2A_3,
	.dpram_irq_flags = IRQF_TRIGGER_RISING,

	.max_ipc_dev = IPC_RFS,
	.ipc_map = &msm_ipc_map,

	.boot_size_offset = DP_BOOT_SIZE_OFFSET,
	.boot_tag_offset = DP_BOOT_TAG_OFFSET,
	.boot_count_offset = DP_BOOT_COUNT_OFFSET,
	.max_boot_frame_size = DP_BOOT_FRAME_SIZE_LIMIT,
};

static struct modem_io_t cdma_io_devices[] = {
	[0] = {
		.name = "cdma_ipc0",
		.id = 0x00,
		.format = IPC_FMT,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[1] = {
		.name = "multi_pdp",
		.id = 0x1,
		.format = IPC_MULTI_RAW,
		.io_type = IODEV_DUMMY,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[2] = {
		.name = "cdma_CSD",
		.id = (1 | 0x20),
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[3] = {
		.name = "cdma_FOTA",
		.id = (2 | 0x20),
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[4] = {
		.name = "cdma_GPS",
		.id = (5 | 0x20),
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[5] = {
		.name = "cdma_XTRA",
		.id = (6 | 0x20),
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[6] = {
		.name = "cdma_CDMA",
		.id = (7 | 0x20),
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},

	[7] = {
		.name = "cdma_EFS",
		.id = (8 | 0x20),
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[8] = {
		.name = "cdma_TRFB",
		.id = (9 | 0x20),
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	/*		//GIL_TEMP
	[9] = {
		.name = "rmnet0",
		.id = 0x2A,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[10] = {
		.name = "rmnet1",
		.id = 0x2B,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[11] = {
		.name = "rmnet2",
		.id = 0x2C,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[12] = {
		.name = "rmnet3",
		.id = 0x2D,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[13] = {
		.name = "cdma_SMD",
		.id = (25 | 0x20),
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[14] = {
		.name = "cdma_VTVD",
		.id = (26 | 0x20),
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[15] = {
		.name = "cdma_VTAD",
		.id = (27 | 0x20),
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},*/
/*
	[16] = {
		.name = "cdma_VTCTRL",
		.id = (28 | 0x20),
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[17] = {
		.name = "cdma_VTENT",
		.id = (29 | 0x20),
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
*/
};

static struct modem_data cdma_modem_data = {
	.name = "msm7x27",
	.modem_net = CDMA_NETWORK,
	.modem_type = QC_MSM7x27,
	.link_types = LINKTYPE(LINKDEV_DPRAM),
	.link_name = "msm7x27_edpram",
	.dpram_ctl = &msm_edpram_ctrl,

	.ipc_version = SIPC_VER_41,

	.num_iodevs = ARRAY_SIZE(cdma_io_devices),
	.iodevs = cdma_io_devices,
};

static struct resource cdma_modem_res[] = {
	[0] = {
		.name = "smd_info",
		.start = SMEM_ID_VENDOR0, /* SMEM ID */
		.end = DPRAM_SIZE, /* Interface Memory size (SMD) */
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.name = "smd_irq_info",
		.start = MSM8625_INT_A9_M2A_3, /* IRQ number */
		.end = IRQ_TYPE_EDGE_RISING, /* IRQ Trigger type */
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		.name = "smd_irq_arg",
		.start = 0,
		.end = MSM_A2M_INT(3),
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device cdma_modem = {
	.name = "modem_if",
	.id = 2,
	.num_resources = ARRAY_SIZE(cdma_modem_res),
	.resource = cdma_modem_res,
	.dev = {
		.platform_data = &cdma_modem_data,
	},
};

static u8 *msm_edpram_remap_mem_region(void)
{
	int dp_addr = 0;
	int dp_size = 0;
	u8 __iomem *dp_base = NULL;
	struct msm_edpram_ipc_cfg *ipc_map = NULL;
	struct dpram_ipc_device *dev = NULL;

	dp_addr = SMEM_ID_VENDOR0;
	dp_size = DPRAM_SIZE;

	dp_base = (volatile unsigned char *)
		(smem_alloc2(dp_addr, dp_size));

	if (!dp_base) {
		pr_err("[LNK] <%s> smem_do_alloc failed!\n", __func__);
		return (struct link_device *)-ENOMEM;
	}

	pr_info("[MDM] <%s> DPRAM VA=0x%08X\n", __func__, (int)dp_base);

	msm_edpram_ctrl.dp_base = (u8 __iomem *) dp_base;
	msm_edpram_ctrl.dp_size = dp_size;

	/* Map for IPC */
	ipc_map = (struct msm_edpram_ipc_cfg *)dp_base;

	/* Magic code and access enable fields */
	msm_ipc_map.magic = (u16 __iomem *) &ipc_map->magic;
	msm_ipc_map.access = (u16 __iomem *) &ipc_map->access;

	/* FMT */
	dev = &msm_ipc_map.dev[IPC_FMT];

	strcpy(dev->name, "FMT");
	dev->id = IPC_FMT;

	dev->txq.head = (u16 __iomem *) &ipc_map->fmt_tx_head;
	dev->txq.tail = (u16 __iomem *) &ipc_map->fmt_tx_tail;
	dev->txq.buff = (u8 __iomem *) &ipc_map->fmt_tx_buff[0];
	dev->txq.size = MSM_DP_FMT_TX_BUFF_SZ;

	dev->rxq.head = (u16 __iomem *) &ipc_map->fmt_rx_head;
	dev->rxq.tail = (u16 __iomem *) &ipc_map->fmt_rx_tail;
	dev->rxq.buff = (u8 __iomem *) &ipc_map->fmt_rx_buff[0];
	dev->rxq.size = MSM_DP_FMT_RX_BUFF_SZ;

	dev->mask_req_ack = INT_MASK_REQ_ACK_F;
	dev->mask_res_ack = INT_MASK_RES_ACK_F;
	dev->mask_send = INT_MASK_SEND_F;

	/* RAW */
	dev = &msm_ipc_map.dev[IPC_RAW];

	strcpy(dev->name, "RAW");
	dev->id = IPC_RAW;

	dev->txq.head = (u16 __iomem *) &ipc_map->raw_tx_head;
	dev->txq.tail = (u16 __iomem *) &ipc_map->raw_tx_tail;
	dev->txq.buff = (u8 __iomem *) &ipc_map->raw_tx_buff[0];
	dev->txq.size = MSM_DP_RAW_TX_BUFF_SZ;

	dev->rxq.head = (u16 __iomem *) &ipc_map->raw_rx_head;
	dev->rxq.tail = (u16 __iomem *) &ipc_map->raw_rx_tail;
	dev->rxq.buff = (u8 __iomem *) &ipc_map->raw_rx_buff[0];
	dev->rxq.size = MSM_DP_RAW_RX_BUFF_SZ;

	dev->mask_req_ack = INT_MASK_REQ_ACK_R;
	dev->mask_res_ack = INT_MASK_RES_ACK_R;
	dev->mask_send = INT_MASK_SEND_R;

	/* Mailboxes */
	msm_ipc_map.mbx_ap2cp = (u16 __iomem *) &ipc_map->mbx_ap2cp;
	msm_ipc_map.mbx_cp2ap = (u16 __iomem *) &ipc_map->mbx_cp2ap;

	return dp_base;
}

static int __init init_modem(void)
{
	int ret;
	pr_info("[MIF] <%s> init_modem\n", __func__);

	if (!msm_edpram_remap_mem_region())
		return -1;

	ret = platform_device_register(&cdma_modem);
	if (ret < 0)
		pr_err("[MIF] <%s> init_modem failed!!\n", __func__);

	return ret;
}
late_initcall(init_modem);
