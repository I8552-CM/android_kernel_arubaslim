/*

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 as
   published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.


   Copyright (C) 2006-2007 - Motorola
   Copyright (c) 2008-2010, Code Aurora Forum. All rights reserved.

   Date         Author           Comment
   -----------  --------------   --------------------------------
   2006-Apr-28	Motorola	 The kernel module for running the Bluetooth(R)
				 Sleep-Mode Protocol from the Host side
   2006-Sep-08  Motorola         Added workqueue for handling sleep work.
   2007-Jan-24  Motorola         Added mbm_handle_ioi() call to ISR.

*/

#include <linux/module.h>	/* kernel module definitions */
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/notifier.h>
#include <linux/proc_fs.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>

#include <linux/irq.h>
#include <linux/param.h>
#include <linux/bitops.h>
#include <linux/termios.h>
#include <linux/wakelock.h>
#include <mach/gpio.h>
#include <mach/msm_serial_hs.h>

#if defined(CONFIG_MACH_DELOS_OPEN)
#include <mach/gpio_delos.h>
#elif defined(CONFIG_MACH_ARUBA_OPEN)||defined(CONFIG_MACH_KYLEPLUS_OPEN)
#include <mach/gpio_aruba.h>
#endif

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h> /* event notifications */
#include "hci_uart.h"
#include <linux/clk.h>

#include <linux/time.h>


#define BT_SLEEP_DBG
#ifndef BT_SLEEP_DBG
#define BT_DBG(fmt, arg...)
#endif
/*
 * Defines
 */

#define VERSION		"1.1"
#define PROC_DIR	"bluetooth/sleep"

struct bluesleep_info {
	unsigned host_wake;
	unsigned host_wake_irq;
	struct uart_port *uport;
	struct wake_lock wake_lock;
};

struct timeval time1;


enum msm_hs_clk_states_e {
      	MSM_HS_CLK_PORT_OFF,     /* port not in use */
      	MSM_HS_CLK_OFF,          /* clock disabled */
      	MSM_HS_CLK_REQUEST_OFF,  /* disable after TX and RX flushed */
      	MSM_HS_CLK_ON,           /* clock enabled */
};

/* work function */
//static void bluesleep_sleep_work(struct work_struct *work);
void bluesleep_sleep_wakeup(void);
static void bluesleep_sleep_work(void);
static irqreturn_t bluesleep_hostwake_isr(int irq, void *dev_id);

/* work queue */
DECLARE_DELAYED_WORK(wake_workqueue, bluesleep_sleep_wakeup);
DECLARE_DELAYED_WORK(sleep_workqueue, bluesleep_sleep_work);

/* Macros for handling sleep work */
#define bluesleep_uart_busy()     schedule_delayed_work(&wake_workqueue, 0)
#define bluesleep_uart_idle()     schedule_delayed_work(&sleep_workqueue, 0)

/* 1 second timeout */
#define TX_TIMER_INTERVAL	3

/* state variable names and bit positions */
#define BT_ASLEEP	0x01
#define BT_TXDATA	0x02
//#define BT_WORKING_TIME_CHECK

/* global pointer to a single hci device. */
static struct hci_dev *bluesleep_hdev;
static struct bluesleep_info *bsi;

/*
 * Local function prototypes
 */

static int bluesleep_hci_event(struct notifier_block *this,
				unsigned long event, void *data);

/*
 * Global variables
 */
extern unsigned int board_hw_revision;

/** Global state flags */
static unsigned long flags;

/** Tasklet to respond to change in hostwake line */
struct work_struct hostwake_work;
struct workqueue_struct *hostwake_work_queue;

/** Transmission timer */
static struct timer_list tx_timer;

/** Lock for state transitions */
static spinlock_t rw_lock;
//static unsigned int bt_wake;

/** Notifier block for HCI events */
struct notifier_block hci_event_nblock = {
	.notifier_call = bluesleep_hci_event,
};

/*
 * Local functions
 */
static void hsuart_power(int on)
{
#if defined(BT_WORKING_TIME_CHECK)
	long long time;

	do_gettimeofday(&time1);
	time = time1.tv_sec * 1000000LL + time1.tv_usec;
	BT_INFO("hsuart_power: time=%lld",time);
#endif

	if (bsi->uport == NULL) {
		BT_DBG("[BT] %s : uart is not opened!", __func__);
		return;
	}

	if (on) {
		if (bsi->uport == NULL) {
			BT_DBG("[BT] %s : uart is not opened!", __func__);
			return;
		}
		msm_hs_request_clock_on(bsi->uport);
		msm_hs_set_mctrl(bsi->uport, TIOCM_RTS);
	} else {
		if (bsi->uport == NULL) {
			BT_DBG("[BT] %s : uart is not opened!", __func__);
			return;
		}
		msm_hs_set_mctrl(bsi->uport, 0);
		msm_hs_request_clock_off(bsi->uport);
	}
#if defined(CONFIG_MACH_ARUBA_OPEN)||defined(CONFIG_MACH_KYLEPLUS_OPEN)||defined(CONFIG_MACH_DELOS_OPEN)
	BT_INFO("hsuart_power on=%d jiffies=%u",on, jiffies);

#endif
}

void bluesleep_sleep_wakeup(void)
{

	BT_INFO("[BT] bluesleep_sleep_wakeup");
	mod_timer(&tx_timer, jiffies +( TX_TIMER_INTERVAL * HZ));

	if (bsi->uport != NULL && test_bit(BT_ASLEEP, &flags)) {
		BT_INFO("[BT] waking up... flags=%d jiffies=%u",flags,jiffies);

		/* Start the timer */
		clear_bit(BT_ASLEEP, &flags);
		/*Activating UART */
		hsuart_power(1);

		wake_lock(&bsi->wake_lock);

	}

}

static void bluesleep_sleep_work(void)
{
	BT_INFO("bluesleep_sleep_work");

	hsuart_power(0);

}

/**
 * A tasklet function that runs in tasklet context and reads the value
 * of the HOST_WAKE GPIO pin and further defer the work.
 * @param data Not used.
 */
static void bluesleep_hostwake_work(struct work_struct *work)
{
	unsigned long irq_flags;
#if defined(BT_WORKING_TIME_CHECK)
	long long time;

	do_gettimeofday(&time1);
	time = time1.tv_sec * 1000000LL + time1.tv_usec;

	BT_INFO("bluesleep_hostwake_work time=%lld",time);
#endif

	//spin_lock(&rw_lock);
	spin_lock_irqsave(&rw_lock, irq_flags);

	bluesleep_uart_busy();
	//bluesleep_sleep_wakeup();

	//spin_unlock(&rw_lock);
	spin_unlock_irqrestore(&rw_lock, irq_flags);
}


static void bluesleep_outgoing_data(void)
{
	unsigned long irq_flags;
#if defined(BT_WORKING_TIME_CHECK)
	long long time;

	do_gettimeofday(&time1);
	time = time1.tv_sec * 1000000LL + time1.tv_usec;

	BT_INFO("bluesleep_outgoing_data time=%lld",time);
#endif

	//spin_lock(&rw_lock);
	spin_lock_irqsave(&rw_lock, irq_flags);

	set_bit(BT_TXDATA, &flags);
	//bluesleep_sleep_wakeup();
	bluesleep_uart_busy();

	//spin_unlock(&rw_lock);
	spin_unlock_irqrestore(&rw_lock, irq_flags);
}

/**
 * Handles HCI device events.
 * @param this Not used.
 * @param event The event that occurred.
 * @param data The HCI device associated with the event.
 * @return <code>NOTIFY_DONE</code>.
 */
static int bluesleep_hci_event(struct notifier_block *this,
				unsigned long event, void *data)
{
	struct hci_dev *hdev = (struct hci_dev *) data;
	struct hci_uart *hu;
	struct uart_state *state;

	if (!hdev)
		return NOTIFY_DONE;

	switch (event) {
	case HCI_DEV_REG:
		if (!bluesleep_hdev) {

			bluesleep_hdev = hdev;
			hu  = (struct hci_uart *) hdev->driver_data;
			state = (struct uart_state *) hu->tty->driver_data;
			bsi->uport = state->uart_port;
			BT_INFO("[BT] HCI_DEV_REG. board =%d\n",board_hw_revision);

		}
		break;
	case HCI_DEV_UNREG:
		BT_INFO("[BT] HCI_DEV_UNREG. board =%d\n",board_hw_revision);

		wake_lock_timeout(&bsi->wake_lock, HZ);
		bluesleep_hdev = NULL;
		bsi->uport = NULL;

		break;

	case HCI_DEV_READ:
	case HCI_DEV_WRITE:
		//BT_INFO("HCI_WRITE: wake");
		bluesleep_outgoing_data();
		break;
	}

	return NOTIFY_DONE;
}

/**
 * Handles transmission timer expiration.
 * @param data Not used.
 */
static void bluesleep_tx_timer_expire(unsigned long data)
{
	unsigned long irq_flags;

	BT_INFO("[BT] bluesleep_tx_timer_expire jiffies=%u",jiffies);

	spin_lock_irqsave(&rw_lock, irq_flags);

	/* were we silent during the last timeout? */
	if (test_bit(BT_TXDATA, &flags)){
		BT_INFO("Tx or Rx data during last period [flags=%d][h_w=%d]\n", flags, gpio_get_value(bsi->host_wake));
		mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL*HZ));
	}
	else{
		set_bit(BT_ASLEEP, &flags);
		/*Deactivating UART */
		hsuart_power(0);
		wake_lock_timeout(&bsi->wake_lock, HZ);
	}

	/* clear the incoming data flag */
	clear_bit(BT_TXDATA, &flags);

	spin_unlock_irqrestore(&rw_lock, irq_flags);
}

/**
 * Schedules a tasklet to run when receiving an interrupt on the
 * <code>HOST_WAKE</code> GPIO pin.
 * @param irq Not used.
 * @param dev_id Not used.
 */
static irqreturn_t bluesleep_hostwake_isr(int irq, void *dev_id)
{
	unsigned long irq_flags;


	/* schedule a workqueue to handle the change in the host wake line */
	queue_work(hostwake_work_queue, &hostwake_work);
	return IRQ_HANDLED;
}

static int __init bluesleep_probe(struct platform_device *pdev)
{
	int ret;
	struct resource *res;

	bsi = kzalloc(sizeof(struct bluesleep_info), GFP_KERNEL);
	if (!bsi)
		return -ENOMEM;


	res = platform_get_resource_byname(pdev, IORESOURCE_IO,
				"gpio_host_wake");
	if (!res) {
		BT_ERR("couldn't find host_wake gpio\n");
		ret = -ENODEV;
		goto free_bsi;
	}
	bsi->host_wake = res->start;

#if defined(CONFIG_MACH_ARUBA_OPEN)&&!defined(CONFIG_MACH_KYLEPLUS_OPEN)&&!defined(CONFIG_MACH_DELOS_OPEN)
	if(board_hw_revision <= 6) //for aruba open old bt hw
	{
		bsi->host_wake = 34;
		BT_INFO("bluesleeprobe ak host wake=%d",bsi->host_wake);
	}
#elif defined(CONFIG_MACH_KYLEPLUS_OPEN)|| defined(CONFIG_MACH_DELOS_OPEN)
	if(board_hw_revision < 1) //for kyleplus open old bt hw
	{
		bsi->host_wake = 34;
		BT_INFO("bluesleeprobe kd host wake=%d",bsi->host_wake);
	}
#endif
	ret = gpio_request(bsi->host_wake, "bt_host_wake");
	if (ret)
		goto free_bsi;
	ret = gpio_direction_input(bsi->host_wake);
	if (ret)
		goto free_bt_host_wake;

	bsi->host_wake_irq = platform_get_irq_byname(pdev, "host_wake");
	if (bsi->host_wake_irq < 0) {
		BT_ERR("couldn't find host_wake irq\n");
		ret = -ENODEV;
		goto free_bt_host_wake_irq;
	}

#if defined(CONFIG_MACH_ARUBA_OPEN)&&!defined(CONFIG_MACH_KYLEPLUS_OPEN)&&!defined(CONFIG_MACH_DELOS_OPEN)
	if(board_hw_revision <= 6) //for aruba open old bt hw
	{
		bsi->host_wake_irq = MSM_GPIO_TO_INT(bsi->host_wake);
		BT_INFO("bluesleeprobe host wake=%d",bsi->host_wake_irq);
	}
#elif defined(CONFIG_MACH_KYLEPLUS_OPEN)|| defined(CONFIG_MACH_DELOS_OPEN)
	if(board_hw_revision < 1) //for kyleplus open old bt hw
	{
		bsi->host_wake_irq = MSM_GPIO_TO_INT(bsi->host_wake);
		BT_INFO("bluesleeprobe host wake=%d",bsi->host_wake_irq);
	}
#endif
	wake_lock_init(&bsi->wake_lock, WAKE_LOCK_SUSPEND, "bluesleep");

	return 0;

free_bt_host_wake_irq:
	free_irq(bsi->host_wake_irq, NULL);
free_bt_host_wake:
	gpio_free(bsi->host_wake);
free_bsi:
	kfree(bsi);
	return ret;
}

static int bluesleep_remove(struct platform_device *pdev)
{
	/* assert bt wake */
	if (disable_irq_wake(bsi->host_wake_irq))
		BT_ERR("Couldn't disable hostwake IRQ wakeup mode\n");

	free_irq(bsi->host_wake_irq, NULL);
	del_timer(&tx_timer);

	if (test_bit(BT_ASLEEP, &flags))
		hsuart_power(1);

	gpio_free(bsi->host_wake);
	kfree(bsi);
	return 0;
}

static struct platform_driver bluesleep_driver = {
	.remove = bluesleep_remove,
	.driver = {
		.name = "bluesleep",
		.owner = THIS_MODULE,
	},
};
/**
 * Initializes the module.
 * @return On success, 0. On error, -1, and <code>errno</code> is set
 * appropriately.
 */
static int __init bluesleep_init(void)
{
	int retval;
	char name[64];

	BT_INFO("MSM Sleep Mode Driver Ver %s", VERSION);

	retval = platform_driver_probe(&bluesleep_driver, bluesleep_probe);
	if (retval)
		return retval;

	bluesleep_hdev = NULL;

	flags = 0; /* clear all status bits */

	/* Initialize spinlock. */
	spin_lock_init(&rw_lock);

	/* Initialize timer */
	init_timer(&tx_timer);
	tx_timer.function = bluesleep_tx_timer_expire;
	tx_timer.data = 0;

	/* create the workqueue for the hostwake */
	snprintf(name, sizeof(name), "blue_sleep");
	hostwake_work_queue = create_singlethread_workqueue(name);
	if (hostwake_work_queue == NULL) {
		BT_SLEEP_DBG("Unable to create workqueue \n");
		retval = -ENODEV;

		return retval;
	}

	/* Initialise the work */
	INIT_WORK(&hostwake_work, bluesleep_hostwake_work);

	/* assert bt wake */
	//gpio_set_value(bsi->ext_wake, 0); 
	hci_register_notifier(&hci_event_nblock);
	
	retval = request_irq(bsi->host_wake_irq, bluesleep_hostwake_isr,	IRQF_DISABLED | IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,	"bluetooth hostwake", NULL);
	if (retval < 0) {
		BT_ERR("Couldn't acquire BT_HOST_WAKE IRQ");
		retval = -ENODEV;

		return retval;
	}
	retval = enable_irq_wake(bsi->host_wake_irq);
	if (retval < 0) {
		pr_err("[BT] Set_irq_wake failed.\n");        	

		return retval;
	}
	
	BT_INFO("BT_HOST_WAKE IRQ registered");

	return 0;
}

/**
 * Cleans up the module.
 */
static void __exit bluesleep_exit(void)
{
	BT_INFO("%s", __func__);
	hci_unregister_notifier(&hci_event_nblock);
	platform_driver_unregister(&bluesleep_driver);
	destroy_workqueue(hostwake_work_queue);
}

module_init(bluesleep_init);
module_exit(bluesleep_exit);

MODULE_DESCRIPTION("Bluetooth Sleep Mode Driver ver %s " VERSION);
#ifdef MODULE_LICENSE
MODULE_LICENSE("GPL");
#endif
