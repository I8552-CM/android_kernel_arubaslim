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
#include <asm/io.h>

#include <linux/regulator/consumer.h>
#include "board-msm7x27a-regulator.h"

#if defined(CONFIG_LINK_DEVICE_PLD)
#if defined(CONFIG_GSM_MODEM_SPRD6500)
#include <mach/pld_data_sprd6500.h>
#else
#include <mach/pld_data.h>
#endif
#endif

/* For MDM6600 EDPRAM (External DPRAM) */
#if defined(CONFIG_LINK_DEVICE_PLD)

#if defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
#define GPIO_ESC_PHONE_ON 83
#else	//others include delos ctc
#define GPIO_ESC_PHONE_ON 110
#endif

#define GPIO_ESC_RESET_N 72
#define GPIO_MSM_ACTIVE 84
#define GPIO_ESC_ACTIVE 41

#if defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
#define GPIO_UART2_SEL 0	//No used
#else	//others include delos ctc
#define GPIO_UART2_SEL 113
#endif


#define GPIO_MSM_DPRAM_INT_N 114
#if defined(CONFIG_MACH_DELOS_DUOS_CTC) || defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
#define GPIO_FPGA1_CRESET 109
#else
#define GPIO_FPGA1_CRESET 131
#endif
#define GPIO_FPGA1_CDONE 130
#if defined(CONFIG_MACH_DELOS_DUOS_CTC) || defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
#define GPIO_FPGA1_RST_N 13
#else
#define GPIO_FPGA1_RST_N 132
#endif
#define GPIO_FPGA1_CS_N 127
#define GPIO_MSM_UART1_RXD 122
#define GPIO_MSM_UART1_TXD 123
#define GPIO_FPGA_SPI_CLK 124
#define GPIO_FPGA_SPI_MOSI 126
#endif

#if defined(CONFIG_MACH_DELOS_DUOS_CTC) || defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
#define GPIO_UART1_SEL			98
#endif


/* For MDM6600 EDPRAM (External DPRAM) */
#if defined(CONFIG_LINK_DEVICE_PLD)
#define MSM_EDPRAM_SIZE		0x10000	/* 8 KB */
#else
#define MSM_EDPRAM_SIZE		0x4000	/* 16 KB */
#endif

#define MSM_EDPRAM_ADDR		0x8C000000

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


static int __init init_modem(void);

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


/*
	mbx_ap2cp +			0x0
	magic_code +
	access_enable +
	padding +
	mbx_cp2ap +			0x1000
	magic_code +
	access_enable +
	padding +
	fmt_tx_head + fmt_tx_tail + fmt_tx_buff +		0x2000
	raw_tx_head + raw_tx_tail + raw_tx_buff +
	fmt_rx_head + fmt_rx_tail + fmt_rx_buff +		0x3000
	raw_rx_head + raw_rx_tail + raw_rx_buff +
  =	2 +
	4094 +
	2 +
	4094 +
	2 +
	2 +
	2 + 2 + 1020 +
	2 + 2 + 3064 +
	2 + 2 + 1020 +
	2 + 2 + 3064
 */

//case : MSM_EDPRAM_SIZE (8k)
#define GSM_DP_FMT_TX_BUFF_SZ	1024
#define GSM_DP_RAW_TX_BUFF_SZ	3072
#define GSM_DP_FMT_RX_BUFF_SZ	1024
#define GSM_DP_RAW_RX_BUFF_SZ	3072

#define MAX_GSM_EDPRAM_IPC_DEV	2	/* FMT, RAW */

struct gsm_edpram_ipc_cfg {
	u16 mbx_ap2cp;
	u16 magic_ap2cp;
	u16 access_ap2cp;
	u16 fmt_tx_head;
	u16 raw_tx_head;
	u16 fmt_rx_tail;
	u16 raw_rx_tail;
	u16 temp1;
	u8 padding1[4080];

	u16 mbx_cp2ap;
	u16 magic_cp2ap;
	u16 access_cp2ap;
	u16 fmt_tx_tail;
	u16 raw_tx_tail;
	u16 fmt_rx_head;
	u16 raw_rx_head;
	u16 temp2;
	u8 padding2[4080];

	u8 fmt_tx_buff[GSM_DP_FMT_TX_BUFF_SZ];
	u8 raw_tx_buff[GSM_DP_RAW_TX_BUFF_SZ];
	u8 fmt_rx_buff[GSM_DP_FMT_RX_BUFF_SZ];
	u8 raw_rx_buff[GSM_DP_RAW_RX_BUFF_SZ];

	u8 padding3[16384];

	u16 address_buffer;
};

#define DP_GSM_BOOT_CLEAR_OFFSET    0
#define DP_GSM_BOOT_RSRVD_OFFSET    0
#define DP_GSM_BOOT_SIZE_OFFSET     0x2
#define DP_GSM_BOOT_TAG_OFFSET      0x4
#define DP_GSM_BOOT_COUNT_OFFSET    0x6

#define DP_GSM_BOOT_FRAME_SIZE_LIMIT     0x1000	/* 15KB = 15360byte = 0x3C00 */


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

/*
** CDMA target platform data
*/
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
			//ygil.Yang temp
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
	},
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
#if !defined(CONFIG_MACH_KYLEPLUS_CTC)
	.dpram_ctl = &msm_edpram_ctrl,
#endif

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


/* For ESC6270 modem */
#if defined(CONFIG_GSM_MODEM_ESC6270) || defined(CONFIG_GSM_MODEM_SPRD6500)
static struct dpram_ipc_map gsm_ipc_map;


static struct modemlink_dpram_control gsm_edpram_ctrl = {
	.dp_type = EXT_DPRAM,

	.dpram_irq = MSM_GPIO_TO_INT(GPIO_MSM_DPRAM_INT_N),
	.dpram_irq_flags = IRQF_TRIGGER_FALLING,

	.max_ipc_dev = IPC_RFS,
	.ipc_map = &gsm_ipc_map,

	.boot_size_offset = DP_GSM_BOOT_SIZE_OFFSET,
	.boot_tag_offset = DP_GSM_BOOT_TAG_OFFSET,
	.boot_count_offset = DP_GSM_BOOT_COUNT_OFFSET,
	.max_boot_frame_size = DP_GSM_BOOT_FRAME_SIZE_LIMIT,

#if defined(CONFIG_LINK_DEVICE_PLD)
	.aligned = 1,
#endif
};

/*
** GSM target platform data
*/
#if defined(CONFIG_LINK_DEVICE_PLD)
static struct modem_io_t gsm_io_devices[] = {
	[0] = {
		.name = "gsm_boot0",
		.id = 0x1,
		.format = IPC_BOOT,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_PLD),
	},
	[1] = {
		.name = "gsm_ipc0",
		.id = 0x01,
		.format = IPC_FMT,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_PLD),
	},
	[2] = {
		.name = "gsm_rfs0",
		.id = 0x28,
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_PLD),
	},
	[3] = {
		.name = "gsm_multi_pdp",
		.id = 0x1,
		.format = IPC_MULTI_RAW,
		.io_type = IODEV_DUMMY,
		.links = LINKTYPE(LINKDEV_PLD),
	},
			//ygil.Yang temp
	[4] = {
		.name = "gsm_rmnet0",
		.id = 0x2A,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_PLD),
	},
	[5] = {
		.name = "gsm_rmnet1",
		.id = 0x2B,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_PLD),
	},
	[6] = {
		.name = "gsm_rmnet2",
		.id = 0x2C,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_PLD),
	},
	[7] = {
		.name = "gsm_router",
		.id = 0x39,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_PLD),
	},
	/*
	[8] = {
		.name = "gsm_csd",
		.id = 0x21,
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_PLD),
	},
	[9] = {
		.name = "gsm_ramdump0",
		.id = 0x1,
		.format = IPC_RAMDUMP,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_PLD),
	},
	[10] = {
		.name = "gsm_loopback0",
		.id = 0x3F,
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_PLD),
	},
	*/
};
#else
static struct modem_io_t gsm_io_devices[] = {
	[0] = {
		.name = "gsm_boot0",
		.id = 0x1,
		.format = IPC_BOOT,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[1] = {
		.name = "gsm_ipc0",
		.id = 0x01,
		.format = IPC_FMT,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[2] = {
		.name = "gsm_rfs0",
		.id = 0x28,
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[3] = {
		.name = "gsm_multi_pdp",
		.id = 0x1,
		.format = IPC_MULTI_RAW,
		.io_type = IODEV_DUMMY,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[4] = {
		.name = "gsm_rmnet0",
		.id = 0x2A,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[5] = {
		.name = "gsm_rmnet1",
		.id = 0x2B,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[6] = {
		.name = "gsm_rmnet2",
		.id = 0x2C,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[7] = {
		.name = "gsm_router",
		.id = 0x39,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[8] = {
		.name = "gsm_csd",
		.id = 0x21,
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[9] = {
		.name = "gsm_ramdump0",
		.id = 0x1,
		.format = IPC_RAMDUMP,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
	[10] = {
		.name = "gsm_loopback0",
		.id = 0x3F,
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_DPRAM),
	},
};
#endif

static struct modem_data gsm_modem_data = {
#if defined(CONFIG_GSM_MODEM_ESC6270)
	.name = "esc6270",
#elif defined(CONFIG_GSM_MODEM_SPRD6500)
	.name = "sprd6500",
#endif

	.gpio_cp_on = GPIO_ESC_PHONE_ON,
	.gpio_cp_off = 0,
	.gpio_reset_req_n = 0,	/* GPIO_CP_MSM_PMU_RST, */
	.gpio_cp_reset = GPIO_ESC_RESET_N,
	.gpio_pda_active = GPIO_MSM_ACTIVE,
	.gpio_phone_active = GPIO_ESC_ACTIVE,
	.gpio_flm_uart_sel = GPIO_UART2_SEL,

	.gpio_dpram_int = GPIO_MSM_DPRAM_INT_N,

	.gpio_cp_dump_int = 0,
	.gpio_cp_warm_reset = 0,

#if defined(CONFIG_LINK_DEVICE_PLD)
	.gpio_fpga1_creset = GPIO_FPGA1_CRESET,
	.gpio_fpga1_cdone = GPIO_FPGA1_CDONE,
	.gpio_fpga1_rst_n = GPIO_FPGA1_RST_N,
	.gpio_fpga1_cs_n = GPIO_FPGA1_CS_N,
#endif


#if defined(CONFIG_SIM_DETECT)
	.gpio_sim_detect = GPIO_ESC_SIM_DETECT,
#else
	.gpio_sim_detect = 0,
#endif

	.use_handover = false,

	.modem_net = CDMA_NETWORK,

#if defined(CONFIG_GSM_MODEM_ESC6270)
	.modem_type = QC_ESC6270,
#elif defined(CONFIG_GSM_MODEM_SPRD6500)
	.modem_type = SC_SPRD6500,
#endif

#if defined(CONFIG_LINK_DEVICE_PLD)
	.link_types = LINKTYPE(LINKDEV_PLD),
#else
	.link_types = LINKTYPE(LINKDEV_DPRAM),
#endif

#if defined(CONFIG_GSM_MODEM_ESC6270)
	.link_name = "esc6270_edpram",
#elif defined(CONFIG_GSM_MODEM_SPRD6500)
	.link_name = "sprd6500_edpram",
#endif

	.dpram_ctl = &gsm_edpram_ctrl,

	.ipc_version = SIPC_VER_41,

	.num_iodevs = ARRAY_SIZE(gsm_io_devices),
	.iodevs     = gsm_io_devices,
};

static struct resource gsm_modem_res[] = {
	[0] = {
		.name = "cp_active_irq",
		.start = MSM_GPIO_TO_INT(GPIO_ESC_ACTIVE),
		.end = MSM_GPIO_TO_INT(GPIO_ESC_ACTIVE),
		.flags = IORESOURCE_IRQ,
	},
	[1] = {
		.name = "dpram_irq",
		.start = MSM_GPIO_TO_INT(GPIO_MSM_DPRAM_INT_N),
		.end = MSM_GPIO_TO_INT(GPIO_MSM_DPRAM_INT_N),
		.flags = IORESOURCE_IRQ,
	},
#if defined(CONFIG_SIM_DETECT)
	[2] = {
		.name = "sim_irq",
		.start = ESC_SIM_DETECT_IRQ,
		.end = ESC_SIM_DETECT_IRQ,
		.flags = IORESOURCE_IRQ,
	},
#endif
};

static struct platform_device gsm_modem = {
	.name = "modem_if",
	.id = 1,
	.num_resources = ARRAY_SIZE(gsm_modem_res),
	.resource = gsm_modem_res,
	.dev = {
		.platform_data = &gsm_modem_data,
	},
};

extern unsigned int board_hw_revision;

void config_gsm_modem_gpio(void)
{
	int err = 0;
	unsigned gpio_cp_on = gsm_modem_data.gpio_cp_on;
	unsigned gpio_cp_off = gsm_modem_data.gpio_cp_off;
	unsigned gpio_rst_req_n = gsm_modem_data.gpio_reset_req_n;
	unsigned gpio_cp_rst = gsm_modem_data.gpio_cp_reset;
	unsigned gpio_pda_active = gsm_modem_data.gpio_pda_active;
	unsigned gpio_phone_active = gsm_modem_data.gpio_phone_active;
	unsigned gpio_flm_uart_sel = gsm_modem_data.gpio_flm_uart_sel;
	unsigned gpio_dpram_int = gsm_modem_data.gpio_dpram_int;
	unsigned gpio_sim_detect = gsm_modem_data.gpio_sim_detect;


#if defined(CONFIG_LINK_DEVICE_PLD)
	unsigned gpio_fpga1_creset = gsm_modem_data.gpio_fpga1_creset;
	unsigned gpio_fpga1_cdone = gsm_modem_data.gpio_fpga1_cdone;
	unsigned gpio_fpga1_rst_n = gsm_modem_data.gpio_fpga1_rst_n;
	unsigned gpio_fpga1_cs_n = gsm_modem_data.gpio_fpga1_cs_n;
#endif


	pr_err("[MODEMS] <%s>\n", __func__);

	if (gpio_pda_active) {
		err = gpio_request(gpio_pda_active, "PDA_ACTIVE");
		if (err) {
			pr_err("fail to request gpio %s, gpio %d, errno %d\n",
					"PDA_ACTIVE", gpio_pda_active, err);
		} else {
			gpio_tlmm_config(GPIO_CFG(gpio_pda_active,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			gpio_set_value(gpio_pda_active, 0);
		}
	}

	if (gpio_phone_active) {
		err = gpio_request(gpio_phone_active, "ESC_ACTIVE");
		if (err) {
			pr_err("fail to request gpio %s, gpio %d, errno %d\n",
					"ESC_ACTIVE", gpio_phone_active, err);
		} else {
			gpio_tlmm_config(GPIO_CFG(gpio_phone_active, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
			irq_set_irq_type(gpio_phone_active, IRQ_TYPE_EDGE_BOTH);
		}
	}

	if (gpio_flm_uart_sel) {
		err = gpio_request(gpio_flm_uart_sel, "BOOT_SW_SEL2");
		if (err) {
			pr_err("fail to request gpio %s, gpio %d, errno %d\n",
					"BOOT_SW_SEL2", gpio_flm_uart_sel, err);
		} else {
			gpio_tlmm_config(GPIO_CFG(gpio_flm_uart_sel,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			gpio_set_value(gpio_flm_uart_sel, 0);
		}
	}

	if (gpio_cp_on) {
		err = gpio_request(gpio_cp_on, "ESC_ON");
		if (err) {
			pr_err("fail to request gpio %s, gpio %d, errno %d\n",
					"ESC_ON", gpio_cp_on, err);
		} else {
			gpio_tlmm_config(GPIO_CFG(gpio_cp_on,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			gpio_set_value(gpio_cp_on, 0);
		}
	}

	if (gpio_cp_off) {
		err = gpio_request(gpio_cp_off, "ESC_OFF");
		if (err) {
			pr_err("fail to request gpio %s, gpio %d, errno %d\n",
					"ESC_OFF", (gpio_cp_off), err);
		} else {
			gpio_tlmm_config(GPIO_CFG(gpio_cp_off,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			gpio_set_value(gpio_cp_off, 1);
		}
	}

	if (gpio_rst_req_n) {
		err = gpio_request(gpio_rst_req_n, "ESC_RST_REQ");
		if (err) {
			pr_err("fail to request gpio %s, gpio %d, errno %d\n",
					"ESC_RST_REQ", gpio_rst_req_n, err);
		} else {
			gpio_tlmm_config(GPIO_CFG(gpio_rst_req_n,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			gpio_set_value(gpio_rst_req_n, 0);
		}
	}
#ifndef CONFIG_MACH_HENNESSY_DUOS_CTC //don't use gpio72 to reset
	if (gpio_cp_rst) {
		err = gpio_request(gpio_cp_rst, "ESC_RST");
		if (err) {
			pr_err("fail to request gpio %s, gpio %d, errno %d\n",
					"ESC_RST", gpio_cp_rst, err);
		} else {
			gpio_tlmm_config(GPIO_CFG(gpio_cp_rst,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			gpio_set_value(gpio_cp_rst, 0);
		}
	}
#endif
	if (gpio_dpram_int) {
		pr_err("[MODEMS] gpio_dpram_int <%s>\n", __func__);
		err = gpio_request(gpio_dpram_int, "ESC_DPRAM_INT");
		if (err) {
			pr_err("fail to request gpio %s, gpio %d, errno %d\n",
					"ESC_DPRAM_INT", gpio_dpram_int, err);
		} else {
			/* Configure as a wake-up source */
			//gpio_tlmm_config(GPIO_CFG(gpio_phone_active, 0, gpio_dpram_int, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
			gpio_tlmm_config(GPIO_CFG(gpio_dpram_int, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
			irq_set_irq_type(gpio_dpram_int, IRQ_TYPE_LEVEL_LOW);
		}
	}

#if defined(CONFIG_MACH_DELOS_DUOS_CTC)
	if (board_hw_revision >= 4)	{
		pr_err("[MODEMS] UART1 SEL port <%s>\n", __func__);
		err = gpio_request(GPIO_UART1_SEL, "UART1_SEL");
		if (err) {
			pr_err("fail to request gpio %s, gpio %d, errno %d\n",
					"UART1_SEL", GPIO_UART1_SEL, err);
		} else {
			gpio_tlmm_config(GPIO_CFG(GPIO_UART1_SEL, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
			gpio_set_value(GPIO_UART1_SEL, 0);
		}
	}
#else if defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
	if (1)	{
		pr_err("[MODEMS] UART1 SEL port <%s>\n", __func__);
		err = gpio_request(GPIO_UART1_SEL, "UART1_SEL");
		if (err) {
			pr_err("fail to request gpio %s, gpio %d, errno %d\n",
					"UART1_SEL", GPIO_UART1_SEL, err);
		} else {
			gpio_tlmm_config(GPIO_CFG(GPIO_UART1_SEL, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
			gpio_set_value(GPIO_UART1_SEL, 0);
		}
	}
#endif

#if 0
	if (err) {
		pr_err("fail to request gpio %s, gpio %d, errno %d\n",
				"AP_CP2_UART_RXD", GPIO_MSM_UART1_RXD, err);
	} else {
		gpio_tlmm_config(GPIO_CFG(GPIO_MSM_UART1_RXD, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE); 

	}

	err = gpio_request(GPIO_MSM_UART1_TXD, "AP_CP2_UART_TXD");
	if (err) {
		pr_err("fail to request gpio %s, gpio %d, errno %d\n",
				"AP_CP2_UART_TXD", GPIO_MSM_UART1_TXD, err);
	} else {
		gpio_tlmm_config(GPIO_CFG(GPIO_MSM_UART1_TXD, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE); 

	}
#endif

	if (gpio_sim_detect) {
		err = gpio_request(gpio_sim_detect, "ESC_SIM_DETECT");
		if (err) {
			pr_err("fail to request gpio %s: %d\n",
					"ESC_SIM_DETECT", err);
		} else {
			gpio_tlmm_config(GPIO_CFG(gpio_sim_detect, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE); 
			/* irq_set_irq_type(gpio_sim_detect, IRQ_TYPE_EDGE_BOTH); */
		}
	}

#if defined(CONFIG_LINK_DEVICE_PLD)
	if (gpio_fpga1_creset) {
		err = gpio_request(gpio_fpga1_creset, "FPGA1_CRESET");
		if (err) {
			pr_err("fail to request gpio %s, gpio %d, errno %d\n",
					"FPGA1_CRESET", gpio_fpga1_creset, err);
		} else {
			gpio_tlmm_config(GPIO_CFG(gpio_fpga1_creset,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			gpio_set_value(gpio_fpga1_creset, 0);
		}
	}

	if (gpio_fpga1_cdone) {
		err = gpio_request(gpio_fpga1_cdone, "FPGA1_CDONE");
		if (err) {
			pr_err("fail to request gpio %s, gpio %d, errno %d\n",
					"FPGA1_CDONE", gpio_fpga1_cdone, err);
		} else {
			gpio_tlmm_config(GPIO_CFG(gpio_fpga1_cdone, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE); 
		}
	}
	
	if (gpio_fpga1_rst_n)	{
		err = gpio_request(gpio_fpga1_rst_n, "FPGA1_RST");
		if (err) {
			pr_err("fail to request gpio %s, gpio %d, errno %d\n",
					"FPGA1_RST", gpio_fpga1_rst_n, err);
		} else {
			gpio_tlmm_config(GPIO_CFG(gpio_fpga1_rst_n,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			gpio_set_value(gpio_fpga1_rst_n, 0);
		}
	}

	if (gpio_fpga1_cs_n)	{
		err = gpio_request(gpio_fpga1_cs_n, "SPI_CS2_1");
		if (err) {
			pr_err("fail to request gpio %s, gpio %d, errno %d\n",
					"SPI_CS2_1", gpio_fpga1_cs_n, err);
		} else {
			gpio_tlmm_config(GPIO_CFG(gpio_fpga1_cs_n,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			gpio_set_value(gpio_fpga1_cs_n, 0);
		}
	}
#endif

}

static u8 *gsm_edpram_remap_mem_region()
{
	int dp_addr = 0;
	int dp_size = 0;
	u8 __iomem *dp_base = NULL;
	struct gsm_edpram_ipc_cfg *ipc_map = NULL;
	struct dpram_ipc_device *dev = NULL;
	struct resource *reso;

	dp_addr = MSM_EDPRAM_ADDR;	
	dp_size = MSM_EDPRAM_SIZE;


	reso = request_mem_region(MSM_EDPRAM_ADDR, MSM_EDPRAM_SIZE, "IDPRAM");

	if (!reso) {
		pr_err("[MDM] <%s> dpram base mem_region fail\n", __func__);
		return NULL;	
	}
	
	dp_base = (u8 *)ioremap_nocache(dp_addr, dp_size);
	if (!dp_base) {
		pr_err("[MDM] <%s> dpram base ioremap fail\n", __func__);
		return NULL;
	}
	pr_info("[MDM] <%s> DPRAM VA=0x%08X\n", __func__, (int)dp_base);

	gsm_edpram_ctrl.dp_base = (u8 __iomem *)dp_base;
	gsm_edpram_ctrl.dp_size = dp_size;

	/* Map for IPC */
	ipc_map = (struct gsm_edpram_ipc_cfg *)dp_base;

	/* Magic code and access enable fields */
#if defined(CONFIG_LINK_DEVICE_PLD)
	/* Magic code and access enable fields */
	gsm_ipc_map.magic_ap2cp = (u16 __iomem *) &ipc_map->magic_ap2cp;
	gsm_ipc_map.access_ap2cp = (u16 __iomem *) &ipc_map->access_ap2cp;

	gsm_ipc_map.magic_cp2ap = (u16 __iomem *) &ipc_map->magic_cp2ap;
	gsm_ipc_map.access_cp2ap = (u16 __iomem *) &ipc_map->access_cp2ap;

	gsm_ipc_map.address_buffer = (u16 __iomem *) &ipc_map->address_buffer;
#else
	/* Magic code and access enable fields */
	gsm_ipc_map.magic = (u16 __iomem *)&ipc_map->magic;
	gsm_ipc_map.access = (u16 __iomem *)&ipc_map->access;
#endif

	/* FMT */
	dev = &gsm_ipc_map.dev[IPC_FMT];

	strcpy(dev->name, "FMT");
	dev->id = IPC_FMT;

	dev->txq.head = (u16 __iomem *)&ipc_map->fmt_tx_head;
	dev->txq.tail = (u16 __iomem *)&ipc_map->fmt_tx_tail;
	dev->txq.buff = (u8 __iomem *)&ipc_map->fmt_tx_buff[0];
	dev->txq.size = GSM_DP_FMT_TX_BUFF_SZ;

	dev->rxq.head = (u16 __iomem *)&ipc_map->fmt_rx_head;
	dev->rxq.tail = (u16 __iomem *)&ipc_map->fmt_rx_tail;
	dev->rxq.buff = (u8 __iomem *)&ipc_map->fmt_rx_buff[0];
	dev->rxq.size = GSM_DP_FMT_RX_BUFF_SZ;

	dev->mask_req_ack = INT_MASK_REQ_ACK_F;
	dev->mask_res_ack = INT_MASK_RES_ACK_F;
	dev->mask_send = INT_MASK_SEND_F;

	/* RAW */
	dev = &gsm_ipc_map.dev[IPC_RAW];

	strcpy(dev->name, "RAW");
	dev->id = IPC_RAW;

	dev->txq.head = (u16 __iomem *)&ipc_map->raw_tx_head;
	dev->txq.tail = (u16 __iomem *)&ipc_map->raw_tx_tail;
	dev->txq.buff = (u8 __iomem *)&ipc_map->raw_tx_buff[0];
	dev->txq.size = GSM_DP_RAW_TX_BUFF_SZ;

	dev->rxq.head = (u16 __iomem *)&ipc_map->raw_rx_head;
	dev->rxq.tail = (u16 __iomem *)&ipc_map->raw_rx_tail;
	dev->rxq.buff = (u8 __iomem *)&ipc_map->raw_rx_buff[0];
	dev->rxq.size = GSM_DP_RAW_RX_BUFF_SZ;

	dev->mask_req_ack = INT_MASK_REQ_ACK_R;
	dev->mask_res_ack = INT_MASK_RES_ACK_R;
	dev->mask_send = INT_MASK_SEND_R;

	/* Mailboxes */
	gsm_ipc_map.mbx_ap2cp = (u16 __iomem *)&ipc_map->mbx_ap2cp;
	gsm_ipc_map.mbx_cp2ap = (u16 __iomem *)&ipc_map->mbx_cp2ap;

	return dp_base;
}
#endif

#if CONFIG_LINK_DEVICE_PLD



static int fpga_ldo_init()
{
	int rc = 0;

	struct regulator *reg_fpga;
	struct regulator *reg_msmp;
	
	reg_fpga = regulator_get(NULL, "ldo05");		//ldo5

	if (IS_ERR(reg_fpga)) {
		rc = PTR_ERR(reg_fpga);
		pr_err("%s: reg_fpga could not get regulator: %d\n",
			__func__, rc);
		goto out;
	}

	rc = regulator_set_voltage(reg_fpga, 1200000, 1200000);
	if (rc) {
		pr_err("%s: reg_fpga could not set voltage: %d\n",
		__func__, rc);
		goto reg_fpga_free;
	}


	rc = regulator_enable(reg_fpga);
	if (rc) {
		pr_err("%s: reg_fpga could not reg_enable: %d\n",
		__func__, rc);
		goto reg_fpga_free;
	}	
		
	reg_msmp = regulator_get(NULL, "ldo12");

		
	if (IS_ERR(reg_msmp)) {
		rc = PTR_ERR(reg_msmp);
		pr_err("%s: reg_msmp could not get regulator: %d\n",
			__func__, rc);
		goto out;
	}

	rc = regulator_set_voltage(reg_msmp, 2850000, 2850000);
	if (rc) {
		pr_err("%s:reg_msmp could not set voltage: %d\n",
		__func__, rc);
		goto reg_msmp_free;
	}

	rc = regulator_enable(reg_msmp);
	if (rc) {
		pr_err("%s:reg_msmp could not reg_enable: %d\n",
		__func__, rc);
		goto reg_msmp_free;
	}
	
	return 0;
	
	/* else fall through */
reg_fpga_free:
	regulator_put(reg_fpga);
reg_msmp_free:
	regulator_put(reg_msmp);
out:
	reg_fpga = NULL;
	reg_msmp = NULL;
	return rc;
}

static int init_FPGA(void)
{
	//internal variables
	unsigned int i,j,k; 				// loop variables
	unsigned int spibit;				// current FPGA configuration data storage
	int cdone_gpio_v;
	char dummy_data[8] = "abcdefg";
	

	//gpio_tlmm_config(GPIO_FPGA_SPI_CLK); 		//RA6 SPI_CLK
	//gpio_tlmm_config(GPIO_FPGA_SPI_MOSI); 		//RG12 SPI_SI
	//gpio_tlmm_config(GPIO_FPGA1_CS_N); 		//RG13 SPI_SS_B
	//gpio_tlmm_config(GPIO_FPGA1_CRESET); 		//RG14 CRESET_B
	//gpio_tlmm_config(GPIO_FPGA1_RST_N); 		// FPGA_RSTN

	//ldo setting

	
	gpio_tlmm_config(GPIO_CFG(GPIO_FPGA_SPI_CLK,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(GPIO_FPGA_SPI_MOSI,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(GPIO_FPGA1_CS_N,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(GPIO_FPGA1_CRESET,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(GPIO_FPGA1_RST_N,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	
	gpio_tlmm_config(GPIO_CFG(GPIO_FPGA1_CDONE, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE); 
	//gpio_direction_input(GPIO_FPGA1_CDONE);
		
	//Index 1: reset FPGA Configuration State Machine : > 200ns
	gpio_set_value(GPIO_FPGA1_CRESET, 0);
	gpio_set_value(GPIO_FPGA1_CS_N, 0);
	gpio_set_value(GPIO_FPGA_SPI_CLK, 0);
	gpio_set_value(GPIO_FPGA_SPI_MOSI, 0);	
	gpio_set_value(GPIO_FPGA1_RST_N, 0);
	i=0;	

	cdone_gpio_v = gpio_get_value(GPIO_FPGA1_CDONE);
	pr_info("[MIF] <%s> init_FPGA _ CDONE %d\n", __func__, cdone_gpio_v);
	
	while (i<64) {
		gpio_set_value(GPIO_FPGA_SPI_CLK, 0);	 
		i=i+1;			  
		gpio_set_value(GPIO_FPGA_SPI_CLK, 0);
	}	 

	//Index 2: force FPGA Configuration Slave Mode : > 300us
	gpio_set_value(GPIO_FPGA1_CRESET, 1);
	i=0;
	while (i<1000) {
		gpio_set_value(GPIO_FPGA_SPI_CLK, 0);	  
		i=i+1;			  
		gpio_set_value(GPIO_FPGA_SPI_CLK, 0);
	}

	msleep(20);
	//Index 3:	  
	i=0;
	while (i < 32216) {
		j=0;
		spibit=fpga_bin[i];
		while (j < 8) { 				
			gpio_set_value(GPIO_FPGA_SPI_CLK, 0);
			if (spibit & 0x80) {gpio_set_value(GPIO_FPGA_SPI_MOSI, 1);} else {gpio_set_value(GPIO_FPGA_SPI_MOSI, 0);}
			j=j+1;
			gpio_set_value(GPIO_FPGA_SPI_CLK, 1);
			spibit = spibit<<1;
		}
		i=i+1;
	}

	//Index 3-1:	  
	i=0;
	while (i < 8) {
		j=0;
		spibit=dummy_data[i];
		while (j < 8) { 				
			gpio_set_value(GPIO_FPGA_SPI_CLK, 0);
			if (spibit & 0x80) {gpio_set_value(GPIO_FPGA_SPI_MOSI, 1);} else {gpio_set_value(GPIO_FPGA_SPI_MOSI, 0);}
			j=j+1;
			gpio_set_value(GPIO_FPGA_SPI_CLK, 1);
			spibit = spibit<<1;
		}
		i=i+1;
	}

	//Index 4:		  
	//Start up FPGA
	i=0;
	while (i<100) {
		gpio_set_value(GPIO_FPGA_SPI_CLK, 0);	
		i=i+1;			  
		gpio_set_value(GPIO_FPGA_SPI_CLK, 1);
	}			 

	gpio_set_value(GPIO_FPGA_SPI_CLK, 0);
	gpio_set_value(GPIO_FPGA1_RST_N, 1);

	msleep(20);
	cdone_gpio_v = gpio_get_value(GPIO_FPGA1_CDONE);
	pr_info("[MIF] <%s> init_FPGA _ CDONE %d\n", __func__, cdone_gpio_v);
}
#endif

#if defined(CONFIG_MACH_ARUBA_DUOS_CTC) || defined(CONFIG_MACH_INFINITE_DUOS_CTC) || defined(CONFIG_MACH_DELOS_DUOS_CTC) || defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
extern int charging_boot;
#endif
static int __init init_modem(void)
{
	int ret;
#if defined(CONFIG_MACH_ARUBA_DUOS_CTC) || defined(CONFIG_MACH_INFINITE_DUOS_CTC) || defined(CONFIG_MACH_DELOS_DUOS_CTC) || defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
	if (charging_boot) {
		pr_info("[MIF] skip <%s> init_modem in charging_boot\n", __func__);
		return 0;
	}
#endif
	pr_info("[MIF] <%s> init_modem\n", __func__);

	if (!msm_edpram_remap_mem_region())
		ret = -1;

	ret = platform_device_register(&cdma_modem);
	if (ret < 0)
		pr_err("[MIF] <%s> init_modem failed!!\n", __func__);

/* For ESC6270 modem */
#if defined(CONFIG_GSM_MODEM_ESC6270) || defined(CONFIG_GSM_MODEM_SPRD6500)

	config_gsm_modem_gpio();

#if defined(CONFIG_LINK_DEVICE_PLD)
	fpga_ldo_init();
	init_FPGA();		//send pld binary
#endif

	if (!gsm_edpram_remap_mem_region())
		ret = -1;

	platform_device_register(&gsm_modem);
#endif

	return ret;
}
late_initcall(init_modem);
