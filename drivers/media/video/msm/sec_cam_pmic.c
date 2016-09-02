
/***************************************************************
  CAMERA Power control
 ****************************************************************/


#include "sec_cam_pmic.h"

#include <mach/gpio.h>
#include <asm/gpio.h>

#include <linux/clk.h>
#include <linux/io.h>
#include <mach/board.h>
#include <mach/msm_iomap.h>

#include <linux/regulator/consumer.h>
#include <mach/vreg.h>
#include <mach/camera.h>
#include "sec_cam_pmic.h"
#define CAM_TEST_REV03 //temp, rev03

//L6 => VCAM_C_1.2V
//L15 => VCAM_IO_1.8V

struct regulator *l6, *l15;

void cam_ldo_power_on(void)
{
	int ret;
	unsigned int mclk_cfg;
	printk("#### cam_ldo_power_on ####\n");

	gpio_set_value_cansleep(CAM_A_EN, 1);
	msleep(20);

	l15 = regulator_get(NULL, "ldo15");
	if (!l15)
		printk(KERN_DEBUG "%s: VREG L15 get failed\n", __func__);

	if (regulator_set_voltage(l15 , 1800000, 1800000))
		printk(KERN_DEBUG "%s: vreg_set_level failed\n", __func__);

	if (regulator_enable(l15))
		printk(KERN_DEBUG "%s: reg_enable failed\n", __func__);

	msleep(20); /*t<=2ms */

	/*VCAM-CORE 1.2V*/
		l6 = regulator_get(NULL, "ldo06");
	if (!l6)
		printk(KERN_DEBUG "%s: VREG l6 get failed\n", __func__);

	if (regulator_set_voltage(l6 , 1200000, 1200000))
		printk(KERN_DEBUG "%s: vreg_set_level failed\n", __func__);

	if (regulator_enable(l6))
		printk(KERN_DEBUG "%s: reg_enable failed\n", __func__);

	msleep(20); /*t<=2ms */

}


void cam_ldo_power_off(void)
{
	int ret;



	gpio_set_value_cansleep(CAM_A_EN, 0);
	msleep(20);
	if (l6)
		ret = regulator_disable(l6);
			/*ret=vreg_disable(l6);*/
	if (ret)
		printk(KERN_DEBUG "%s: error disabling regulator\n", __func__);
		/*regulator_put(l8);*/

	msleep(20);

	if (l15)
		ret = regulator_disable(l15);
		/*ret = vreg_disable(l15);*/

	if (ret)
		printk(KERN_DEBUG "%s: error disabling regulator\n", __func__);
		/*regulator_put(l8);*/

	msleep(20);

}

