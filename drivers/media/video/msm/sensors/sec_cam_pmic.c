
/***************************************************************
  CAMERA Power control
 ****************************************************************/


#include <mach/gpio.h>
#include <asm/gpio.h>

#include <linux/clk.h>
#include <linux/io.h>
#include <mach/board.h>
#include <mach/msm_iomap.h>

#include <linux/regulator/consumer.h>
#include <mach/vreg.h>
#include <linux/gpio.h>

#include "sec_cam_pmic.h"


extern unsigned int board_hw_revision;


#if defined(CONFIG_S5K5CCGX)//Power on/off functions for S5KCCGX sensor
void cam_ldo_power_on_1(void)
{
	printk("#### cam_ldo_power_on_1 ####\n");
	
	if(gpio_request(S5K5CCGX_CAM_STBY, "s5k5ccgx_standby")<0)
		pr_err("%s: Request GPIO Standby failed\n", __func__);
	if(gpio_request(S5K5CCGX_CAM_RESET, "s5k5ccgx_reset")<0)
		pr_err("%s: Request GPIO Reset failed\n", __func__);


	gpio_direction_output(S5K5CCGX_CAM_STBY, 0);
	gpio_direction_output(S5K5CCGX_CAM_RESET, 0);

	gpio_set_value_cansleep(S5K5CCGX_CAM_STBY, 0);
	gpio_set_value_cansleep(S5K5CCGX_CAM_RESET, 0);	

//VCAM_A_2.8V
#if defined(CONFIG_MACH_INFINITE_DUOS_CTC) || defined(CONFIG_MACH_KYLEPLUS_CTC)
	if(gpio_request(S5K5CCGX_CAM_A_EN, "s5k5ccgx_vcam_a_en")<0)
		pr_err("%s: Request GPIO VCAM_A failed\n", __func__);
	gpio_direction_output(S5K5CCGX_CAM_A_EN, 0);
	gpio_set_value_cansleep(S5K5CCGX_CAM_A_EN, 0);
	gpio_set_value(S5K5CCGX_CAM_A_EN, 1);
	msleep(1);
#endif
	
//VCAM_IO_1.8V
#if defined(CONFIG_MACH_KYLEPLUS_OPEN) || defined(CONFIG_MACH_KYLEPLUS_CTC)
	if(gpio_request(S5K5CCGX_CAM_IO_EN, "s5k5ccgx_vcam_io")<0)
		pr_err("%s: Request GPIO VCAM_IO failed\n", __func__);
	gpio_direction_output(S5K5CCGX_CAM_IO_EN, 0);
	gpio_set_value_cansleep(S5K5CCGX_CAM_IO_EN, 0);
	gpio_set_value(S5K5CCGX_CAM_IO_EN, 1);
	msleep(10);
#endif
}

void cam_ldo_power_on_2(void)
{
	printk("#### cam_ldo_power_on_2 ####\n");
	msleep(1);
	gpio_set_value_cansleep(S5K5CCGX_CAM_STBY, 1);
	msleep(1);
	gpio_set_value_cansleep(S5K5CCGX_CAM_RESET, 1);
	msleep(1);
}

void cam_ldo_power_off_1(void)
{
	printk("#### cam_ldo_power_off_1 ####\n");
	gpio_set_value_cansleep(S5K5CCGX_CAM_RESET, 0);
	msleep(1);

}
void cam_ldo_power_off_2(void)
{
	printk("#### cam_ldo_power_off_2 ####\n");
	
	gpio_set_value_cansleep(S5K5CCGX_CAM_STBY, 0);
	msleep(1);

//VCAM_A_2.8V
#if defined(CONFIG_MACH_INFINITE_DUOS_CTC) || defined(CONFIG_MACH_KYLEPLUS_CTC) 
	gpio_set_value_cansleep(S5K5CCGX_CAM_A_EN, 0);
	gpio_free(S5K5CCGX_CAM_A_EN);
	msleep(1);
#endif

//VCAM_IO_1.8V
#if defined(CONFIG_MACH_KYLEPLUS_OPEN) || defined(CONFIG_MACH_KYLEPLUS_CTC)
	gpio_set_value_cansleep(S5K5CCGX_CAM_IO_EN, 0);
	gpio_free(S5K5CCGX_CAM_IO_EN);
	msleep(1);
#endif

	gpio_free(S5K5CCGX_CAM_STBY);
	gpio_free(S5K5CCGX_CAM_RESET);
}

#else

void cam_ldo_power_on_1(void)
{
	printk("#### cam_ldo_power_on_1 ####\n");

	gpio_request(ARUBA_CAM_STBY, "aruba_stby");
	gpio_request(ARUBA_CAM_RESET, "aruba_reset");
	gpio_request(ARUBA_CAM_A_EN, "aruba_a_en");
#ifdef CONFIG_S5K4ECGX
	gpio_request(ARUBA_CAM_C_EN, "aruba_c_en");
	gpio_request(ARUBA_CAM_AF_EN, "aruba_af_en");
#endif
	gpio_direction_output(ARUBA_CAM_STBY, 0);
	gpio_direction_output(ARUBA_CAM_RESET, 0);
	gpio_direction_output(ARUBA_CAM_A_EN, 0);
#ifdef CONFIG_S5K4ECGX
	gpio_direction_output(ARUBA_CAM_C_EN, 0);
	gpio_direction_output(ARUBA_CAM_AF_EN, 0);
#endif

	gpio_set_value_cansleep(ARUBA_CAM_STBY, 0);		//STBYN :: MSM_GPIO 96
	gpio_set_value_cansleep(ARUBA_CAM_RESET, 0);	//RSTN :: MSM_GPIO 85
	gpio_set_value_cansleep(ARUBA_CAM_A_EN, 0);
#ifdef CONFIG_S5K4ECGX
	gpio_set_value_cansleep(ARUBA_CAM_C_EN, 0);
	msleep(1);

//	printk("ARUBA_CAM_C_EN = %d , CAM_AF_EN = %d \n", gpio_get_value(ARUBA_CAM_C_EN), gpio_get_value(ARUBA_CAM_AF_EN));
	gpio_set_value(ARUBA_CAM_C_EN, 1);
	gpio_set_value(ARUBA_CAM_AF_EN, 1);
	msleep(1);
//	printk("ARUBA_CAM_C_EN = %d , CAM_AF_EN = %d \n", gpio_get_value(ARUBA_CAM_C_EN), gpio_get_value(ARUBA_CAM_AF_EN));
#endif

//	printk("ARUBA_CAM_A_EN = %d\n", gpio_get_value(ARUBA_CAM_A_EN));

	gpio_set_value(ARUBA_CAM_A_EN, 1);
//	printk("ARUBA_CAM_A_EN = %d\n", gpio_get_value(ARUBA_CAM_A_EN));



}


void cam_ldo_power_on_2(void)
{
	printk("#### cam_ldo_power_on_2 ####\n");

//	printk("ARUBA_CAM_STBY = %d\n", gpio_get_value(ARUBA_CAM_STBY));
	msleep(1);
	gpio_set_value_cansleep(ARUBA_CAM_STBY, 1);		//STBYN :: MSM_GPIO 96
	msleep(1);
//	printk("ARUBA_CAM_STBY = %d\n", gpio_get_value(ARUBA_CAM_STBY));
//	printk("ARUBA_CAM_RESET = %d\n", gpio_get_value(ARUBA_CAM_RESET));
	gpio_set_value_cansleep(ARUBA_CAM_RESET, 1);	//RSTN :: MSM_GPIO 85
	msleep(1);
//	printk("ARUBA_CAM_RESET = %d\n", gpio_get_value(ARUBA_CAM_RESET));	
}




void cam_ldo_power_off_1(void)
{
	printk("#### cam_ldo_power_off_1 ####\n");

	gpio_set_value_cansleep(ARUBA_CAM_RESET, 0);	//RSTN :: MSM_GPIO 85

}
void cam_ldo_power_off_2(void)
{
	printk("#### cam_ldo_power_off_2 ####\n");
	
	gpio_set_value_cansleep(ARUBA_CAM_STBY, 0);		//STBYN :: MSM_GPIO 96
	gpio_set_value_cansleep(ARUBA_CAM_A_EN, 0);
#ifdef CONFIG_S5K4ECGX
	gpio_set_value_cansleep(ARUBA_CAM_C_EN, 0);
	gpio_set_value_cansleep(ARUBA_CAM_AF_EN, 0);
#endif
	msleep(10);
#ifdef CONFIG_S5K4ECGX
	gpio_free(ARUBA_CAM_C_EN);
	gpio_free(ARUBA_CAM_AF_EN);
#endif
	gpio_free(ARUBA_CAM_A_EN);
	gpio_free(ARUBA_CAM_STBY);
	gpio_free(ARUBA_CAM_RESET);
}
#endif

#if defined(CONFIG_S5K4ECGX)
static DEFINE_SPINLOCK(spinlock);

void cam_flash_main_on(int flashMode)
{
	int ret, i;
	printk("#### cam_flash_main_on ####\n");

	if(flash_mode == FLASH_MODE_NONE) {
		flash_mode = flashMode;
	} else {
		printk("FLASH MAIN ON SKIP \n");
		return;
	}
	
	ret = gpio_request(ARUBA_CAM_FLASH_EN, "aruba_flash_en");
	if (ret < 0) {
		printk("cam_flash_main_on GPIO Request Fail !!! \n");
	}

	ret = gpio_request(ARUBA_CAM_FLASH_SET, "aruba_flash_set");

	if (ret < 0) {
		printk("cam_flash_main_on GPIO Request Fail !!! \n");
	}

	printk("ARUBA_CAM_FLASH_EN = %d\n", gpio_get_value(ARUBA_CAM_FLASH_EN));
	printk("ARUBA_CAM_FLASH_SET = %d\n", gpio_get_value(ARUBA_CAM_FLASH_SET));
	
	gpio_set_value_cansleep(ARUBA_CAM_FLASH_SET, 0);

#if defined (CONFIG_MACH_DELOS_CTC)
	if(board_hw_revision == 4){
	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 1);
	gpio_set_value_cansleep(ARUBA_CAM_FLASH_SET, 0);
	udelay(30);  //DS data start time

	for(i=0; i<7; i++){
		gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 0);
		udelay(10);
		gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 1); //Low 0
		udelay(5);  
		}

	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 0);  
	udelay(5);
	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 1);
	udelay(10);                                     //high 1

	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 0); //EOD_L End of Low for 10 us
	udelay(10);
	
	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 1); //EOD_HEnd of High for 400 us
	udelay(400);
	
//start Flash mode
	gpio_set_value_cansleep(ARUBA_CAM_FLASH_SET, 1);

	}else{

	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 1);
	gpio_set_value_cansleep(ARUBA_CAM_FLASH_SET, 1);
	msleep(10);
	}

#elif defined(CONFIG_MACH_DELOS_OPEN)

	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 1);
	gpio_set_value_cansleep(ARUBA_CAM_FLASH_SET, 1);
	msleep(10);
	printk("ARUBA_CAM_FLASH_EN = %d\n", gpio_get_value(ARUBA_CAM_FLASH_EN));
	printk("ARUBA_CAM_FLASH_SET = %d\n", gpio_get_value(ARUBA_CAM_FLASH_SET));

#else
	for(i=0;i<15;i++)
	{
		gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 1);
		mdelay(1);
		gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 0);
		printk("count %d \n",i);
	}
	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 1);
	gpio_set_value_cansleep(ARUBA_CAM_FLASH_SET, 0);
	msleep(10);
	printk("ARUBA_CAM_FLASH_EN = %d\n", gpio_get_value(ARUBA_CAM_FLASH_EN));
	printk("ARUBA_CAM_FLASH_SET = %d\n", gpio_get_value(ARUBA_CAM_FLASH_SET));
	
#endif
}


void cam_flash_torch_on(int flashMode)
{
	int ret,i;
	unsigned long flags;

	printk("#### cam_flash_torch_on ####\n");

	printk("#######board_hw_revision : %d\n",board_hw_revision);
	
	if(flash_mode == FLASH_MODE_NONE) {
		flash_mode = flashMode;
	} else {
		printk("FLASH TORCH ON SKIP \n");
		return;
	}

	ret = gpio_request(ARUBA_CAM_FLASH_EN, "aruba_flash_en");
	if (ret < 0) {
		printk("cam_flash_main_on GPIO Request Fail !!! \n");
	}
	ret = gpio_request(ARUBA_CAM_FLASH_SET, "aruba_flash_set");
	if (ret < 0) {
		printk("cam_flash_main_on GPIO Request Fail !!! \n");
	}
//	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 1);
//	gpio_set_value_cansleep(ARUBA_CAM_FLASH_SET, 1);

#if defined (CONFIG_MACH_DELOS_CTC)

	if(board_hw_revision == 4){
        spin_lock_irqsave(&spinlock, flags);

	//LVP setting before starting Torch mode
	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 1);
		gpio_set_value_cansleep(ARUBA_CAM_FLASH_SET, 0);
	usleep(15);  //DS data start time


	for(i=0; i<7; i++){
		gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 0);
		udelay(5);
		gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 1); //Low 0
		udelay(2);  
		}

	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 0);  
	udelay(2);
	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 1);
	udelay(5);                                     //high 1

	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 0); //EOD_L End of Low for 10 us
	udelay(5);
	
	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 1); //EOD_HEnd of High for 400 us
	udelay(400);  

	//start torch enable
	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 1);
	gpio_set_value_cansleep(ARUBA_CAM_FLASH_SET, 0);
	udelay(15);  //DS data start time

	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 0);
	udelay(2);
	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 1);
	udelay(5);                                     //high 1

	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 0);
	udelay(5);
	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 1); //Low 0
	udelay(2); 

	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 0);  
	udelay(2);
	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 1);
	udelay(5);                                     //high 1

	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 0);
	udelay(5);
	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 1); //Low 0
	udelay(2); 
	
	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 0);
	udelay(5); 
	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 1); //Low 0
	udelay(2);

	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 0);
	udelay(5); 
	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 1); //Low 0
	udelay(2);


	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 0);
	udelay(5); 
	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 1); //Low 0
	udelay(2);

	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 0);  
	udelay(2);
	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 1);
	udelay(5);                                    //high 1
	
	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 0); //EOD_L End of Low for 10 us
	udelay(2);

	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 1); //EOD_HEnd of High for 400 us
	udelay(400);  

	spin_unlock_irqrestore(&spinlock, flags);

	}else{
		//RT8515 FLASH
		gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 1);
		gpio_set_value_cansleep(ARUBA_CAM_FLASH_SET, 0);
	}
	
#else

	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 1);
	gpio_set_value_cansleep(ARUBA_CAM_FLASH_SET, 0);

#endif


}

void cam_flash_off(int flashMode)
{
	int ret;
	printk("#### cam_flash_off ####\n");

	if(flash_mode == flashMode) {
		flash_mode = FLASH_MODE_NONE;
	} else {
		printk("FLASH OFF SKIP \n");
		return;
	}

	ret = gpio_request(ARUBA_CAM_FLASH_EN, "aruba_flash_en");
	if (ret < 0) {
		printk("cam_flash_main_on GPIO Request Fail !!! \n");
	}
	
	ret = gpio_request(ARUBA_CAM_FLASH_SET, "aruba_flash_set");
	if (ret < 0) {
		printk("cam_flash_main_on GPIO Request Fail !!! \n");
	}

#if defined(CONFIG_MACH_DELOS_OPEN)	

	gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 0);
	gpio_set_value_cansleep(ARUBA_CAM_FLASH_SET, 0);

	gpio_free(ARUBA_CAM_FLASH_EN);
	gpio_free(ARUBA_CAM_FLASH_SET);
	
#else

	if(board_hw_revision == 4){
               gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 0); 
               gpio_set_value_cansleep(ARUBA_CAM_FLASH_SET, 0);
               mdelay(1);
	}else{
		gpio_set_value_cansleep(ARUBA_CAM_FLASH_EN, 0);
		gpio_set_value_cansleep(ARUBA_CAM_FLASH_SET, 0);
	}

#endif
}


#endif

#if defined(CONFIG_MACH_BAFFIN_DUOS_CTC)
static struct msm_cam_clk_info cam_clk_info[] = {
	{"cam_m_clk", MSM_SENSOR_MCLK_24HZ},
};

int32_t msm_sensor_power_up_baffin_duos(struct msm_sensor_ctrl_t *s_ctrl)
{

	int32_t rc = 0;
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;
	printk("#### msm_sensor_power_up_baffin_duos ####\n");
	CDBG("%s: %d\n", __func__, __LINE__);

	gpio_tlmm_config(GPIO_CFG(FRONT_CAM_STBY, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(FRONT_CAM_RESET, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(FRONT_CAM_C_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

	s_ctrl->reg_ptr = kzalloc(sizeof(struct regulator *)
			* data->sensor_platform_info->num_vreg, GFP_KERNEL);
	if (!s_ctrl->reg_ptr) {
		pr_err("%s: could not allocate mem for regulators\n",
			__func__);
		return -ENOMEM;
	}
	rc = msm_camera_config_vreg(&s_ctrl->sensor_i2c_client->client->dev,
			s_ctrl->sensordata->sensor_platform_info->cam_vreg,
			s_ctrl->sensordata->sensor_platform_info->num_vreg,
			s_ctrl->reg_ptr, 1);
	if (rc < 0) {
		pr_err("%s: regulator on failed\n", __func__);
		goto config_vreg_failed;
	}
	rc = msm_camera_enable_vreg(&s_ctrl->sensor_i2c_client->client->dev,
			s_ctrl->sensordata->sensor_platform_info->cam_vreg,
			s_ctrl->sensordata->sensor_platform_info->num_vreg,
			s_ctrl->reg_ptr, 1);
	if (rc < 0) {
		pr_err("%s: msm_camera_enable_vreg on failed\n", __func__);
		goto config_vreg_failed;
	}

	
	gpio_request(ARUBA_CAM_STBY, "baffin_stby");
	gpio_request(ARUBA_CAM_RESET, "baffin_reset");
	gpio_request(ARUBA_CAM_A_EN, "baffin_a_en");
	gpio_request(ARUBA_CAM_C_EN, "baffin_c_en");
	gpio_request(ARUBA_CAM_AF_EN, "baffin_af_en");
	gpio_request(FRONT_CAM_STBY, "front_cam_stby");
	gpio_request(FRONT_CAM_RESET, "front_cam_reset");

	gpio_direction_output(ARUBA_CAM_STBY, 0);
	gpio_direction_output(ARUBA_CAM_RESET, 0);
	gpio_direction_output(ARUBA_CAM_A_EN, 0);
	gpio_direction_output(ARUBA_CAM_C_EN, 0);
	gpio_direction_output(ARUBA_CAM_AF_EN, 0);
	gpio_direction_output(FRONT_CAM_STBY, 0);
	gpio_direction_output(FRONT_CAM_RESET, 0);

	
	gpio_set_value_cansleep(ARUBA_CAM_STBY, 0); 	//STBYN :: MSM_GPIO 96
	gpio_set_value_cansleep(ARUBA_CAM_RESET, 0);	//RSTN :: MSM_GPIO 85
	gpio_set_value_cansleep(ARUBA_CAM_A_EN, 0);
	gpio_set_value_cansleep(ARUBA_CAM_C_EN, 0);
	gpio_set_value_cansleep(FRONT_CAM_STBY, 0);
	gpio_set_value_cansleep(FRONT_CAM_RESET, 0);
	
	
	gpio_set_value(ARUBA_CAM_A_EN, 1);  // 2 - MAIN VREG_A 2.8 ON
	mdelay(1);

   	gpio_set_value(FRONT_CAM_C_EN, 1);  // 3- 2M CAM CORE 1.8V ON
   	gpio_set_value(ARUBA_CAM_AF_EN, 1);
	mdelay(1);

	rc = msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
		cam_clk_info, &s_ctrl->cam_clk, ARRAY_SIZE(cam_clk_info), 1);
	if (rc < 0) {
		pr_err("%s: clk enable failed\n", __func__);
		goto enable_clk_failed;
	}
	mdelay(10);

	gpio_set_value(ARUBA_CAM_C_EN, 1);  // 8 - 5M CAM_C_EN ON 
	mdelay(1);
	
	gpio_set_value(ARUBA_CAM_STBY, 1); //9 - 5M STBY ON
	mdelay(1);

	gpio_set_value(ARUBA_CAM_RESET, 1); // 10 - 5M RESET ON
	
	if (data->sensor_platform_info->ext_power_ctrl != NULL)
		data->sensor_platform_info->ext_power_ctrl(1);

	if (data->sensor_platform_info->i2c_conf &&
		data->sensor_platform_info->i2c_conf->use_i2c_mux)
		msm_sensor_enable_i2c_mux(data->sensor_platform_info->i2c_conf);

	return rc;

enable_clk_failed:
//kk0704.park :: ARUBA TEMP		msm_camera_config_gpio_table(data, 0);
config_gpio_failed:
	msm_camera_enable_vreg(&s_ctrl->sensor_i2c_client->client->dev,
			s_ctrl->sensordata->sensor_platform_info->cam_vreg,
			s_ctrl->sensordata->sensor_platform_info->num_vreg,
			s_ctrl->reg_ptr, 0);

enable_vreg_failed:
	msm_camera_config_vreg(&s_ctrl->sensor_i2c_client->client->dev,
		s_ctrl->sensordata->sensor_platform_info->cam_vreg,
		s_ctrl->sensordata->sensor_platform_info->num_vreg,
		s_ctrl->reg_ptr, 0);
config_vreg_failed:
//kk0704.park ARUBA TEMP	msm_camera_request_gpio_table(data, 0);
request_gpio_failed:
	kfree(s_ctrl->reg_ptr);
	return rc;
	
}

int32_t msm_sensor_power_down_baffin_duos(struct msm_sensor_ctrl_t *s_ctrl)
{
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;
	CDBG("%s\n", __func__);
	printk("#### msm_sensor_power_down_baffin_duos ####\n");
	if (data->sensor_platform_info->i2c_conf &&
		data->sensor_platform_info->i2c_conf->use_i2c_mux)
		msm_sensor_disable_i2c_mux(
			data->sensor_platform_info->i2c_conf);
	
	if (data->sensor_platform_info->ext_power_ctrl != NULL)
		data->sensor_platform_info->ext_power_ctrl(0);
	msleep(1);


	gpio_set_value_cansleep(FRONT_CAM_RESET, 0);	// turn off : 2M FRONT_CAM_RESET

	gpio_set_value_cansleep(ARUBA_CAM_RESET, 0);	// turn off : 5M CAM_RESET 

	msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
	cam_clk_info, &s_ctrl->cam_clk, ARRAY_SIZE(cam_clk_info), 0);
	msleep(1);

	gpio_set_value_cansleep(ARUBA_CAM_STBY, 0);		//turn off : 5M STBY
	gpio_set_value_cansleep(FRONT_CAM_C_EN, 0);		//turn off 2M CAM_C_EN

	msm_camera_enable_vreg(&s_ctrl->sensor_i2c_client->client->dev,
		s_ctrl->sensordata->sensor_platform_info->cam_vreg,
		s_ctrl->sensordata->sensor_platform_info->num_vreg,
		s_ctrl->reg_ptr, 0);
	msm_camera_config_vreg(&s_ctrl->sensor_i2c_client->client->dev,
		s_ctrl->sensordata->sensor_platform_info->cam_vreg,
		s_ctrl->sensordata->sensor_platform_info->num_vreg,
		s_ctrl->reg_ptr, 0);
		
	gpio_set_value_cansleep(ARUBA_CAM_A_EN, 0);
	gpio_set_value_cansleep(ARUBA_CAM_C_EN, 0);
	gpio_set_value_cansleep(ARUBA_CAM_AF_EN, 0);

	msleep(10);
#ifdef CONFIG_S5K4ECGX
	gpio_free(ARUBA_CAM_C_EN);
	gpio_free(ARUBA_CAM_AF_EN);
#endif
	
	gpio_free(ARUBA_CAM_A_EN);
	gpio_free(ARUBA_CAM_STBY);
	gpio_free(ARUBA_CAM_RESET);
	gpio_free(FRONT_CAM_RESET);
	gpio_free(FRONT_CAM_STBY);
	gpio_free(FRONT_CAM_C_EN);	
}
#endif

#if defined (CONFIG_MACH_ARUBASLIM_OPEN)
static struct msm_cam_clk_info cam_clk_info[] = {
	{"cam_m_clk", MSM_SENSOR_MCLK_24HZ},
};

int32_t msm_sensor_power_up_aruba_open(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;
	CDBG("%s: %d\n", __func__, __LINE__);
	s_ctrl->reg_ptr = kzalloc(sizeof(struct regulator *)
			* data->sensor_platform_info->num_vreg, GFP_KERNEL);
	if (!s_ctrl->reg_ptr) {
		pr_err("%s: could not allocate mem for regulators\n",
			__func__);
		return -ENOMEM;
	}
	rc = msm_camera_config_vreg(&s_ctrl->sensor_i2c_client->client->dev,
			s_ctrl->sensordata->sensor_platform_info->cam_vreg,
			s_ctrl->sensordata->sensor_platform_info->num_vreg,
			s_ctrl->reg_ptr, 1);
	if (rc < 0) {
		pr_err("%s: regulator on failed\n", __func__);
		goto config_vreg_failed;
	}

	gpio_request(ARUBA_CAM_STBY, "aruba_stby");
	gpio_request(ARUBA_CAM_RESET, "aruba_reset");
	gpio_request(ARUBA_CAM_IO_EN, "aruba_a_en");
	gpio_request(ARUBA_CAM_C_EN, "aruba_c_en");
	gpio_request(ARUBA_CAM_AF_EN, "aruba_af_en");

	if (rc < 0) {
		pr_err("%s: [ARUBASLIM]gpio_request failed\n", __func__);
	}

	gpio_direction_output(ARUBA_CAM_STBY, 0);
	gpio_direction_output(ARUBA_CAM_RESET, 0);
	gpio_direction_output(ARUBA_CAM_IO_EN, 0);
	gpio_direction_output(ARUBA_CAM_C_EN, 0);
	gpio_direction_output(ARUBA_CAM_AF_EN, 0);

	gpio_set_value_cansleep(ARUBA_CAM_STBY, 0);		//STBYN :: MSM_GPIO 96
	gpio_set_value_cansleep(ARUBA_CAM_RESET, 0);	//RSTN :: MSM_GPIO 85
	gpio_set_value_cansleep(ARUBA_CAM_IO_EN, 0);
	gpio_set_value_cansleep(ARUBA_CAM_C_EN, 0);

	msleep(1);
	rc = msm_camera_enable_vreg(&s_ctrl->sensor_i2c_client->client->dev,
			s_ctrl->sensordata->sensor_platform_info->cam_vreg,
			s_ctrl->sensordata->sensor_platform_info->num_vreg,
			s_ctrl->reg_ptr, 1);
	if (rc < 0) {
		pr_err("%s: [ARUBASLIM]enable regulator failed\n", __func__);
		goto enable_vreg_failed;
	}
	udelay(150);// VCAM_A_2.8V

	gpio_set_value(ARUBA_CAM_IO_EN, 1);
	udelay(70);// GPIO 4 : EN_C_EN / VVT_1.8V

	gpio_set_value(ARUBA_CAM_C_EN, 1);	// GPIO 107 : CAM_C_EN
	gpio_set_value(ARUBA_CAM_AF_EN, 1);

	gpio_set_value(FRONT_CAM_STBY, 1);
	msleep(2);	// gpio 76 : VT_CAM_STBY

	rc = msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
		cam_clk_info, &s_ctrl->cam_clk, ARRAY_SIZE(cam_clk_info), 1);
	if (rc < 0) {
		pr_err("%s: clk enable failed\n", __func__);
		goto enable_clk_failed;
	}
	msleep(2);	// CAM_MCLK

	gpio_set_value(FRONT_CAM_RESET, 1);
	msleep(1);	// VT_CAM_RESET

	gpio_set_value_cansleep(FRONT_CAM_STBY, 0);
	gpio_free(FRONT_CAM_STBY);	
	gpio_set_value(FRONT_CAM_STBY, 0);
	msleep(1);	// VT_CAM_RESET : LOW
	
	gpio_set_value(ARUBA_CAM_C_EN, 1);
	msleep(1);	//GPIO 107 : CAM_C_EN

	gpio_set_value_cansleep(ARUBA_CAM_STBY, 1);
	msleep(1);	//GPIO 96 : 5M_CAM_STBY
	
	gpio_set_value_cansleep(ARUBA_CAM_RESET, 1);
	msleep(1);	//GPIO 85 : 5M_CAM_RESET
	usleep_range(1000, 2000);

	if (data->sensor_platform_info->ext_power_ctrl != NULL)
		data->sensor_platform_info->ext_power_ctrl(1);

	if (data->sensor_platform_info->i2c_conf &&
		data->sensor_platform_info->i2c_conf->use_i2c_mux)
		msm_sensor_enable_i2c_mux(data->sensor_platform_info->i2c_conf);

	return rc;

enable_clk_failed:
//kk0704.park :: ARUBA TEMP		msm_camera_config_gpio_table(data, 0);
config_gpio_failed:
	msm_camera_enable_vreg(&s_ctrl->sensor_i2c_client->client->dev,
			s_ctrl->sensordata->sensor_platform_info->cam_vreg,
			s_ctrl->sensordata->sensor_platform_info->num_vreg,
			s_ctrl->reg_ptr, 0);

enable_vreg_failed:
	msm_camera_config_vreg(&s_ctrl->sensor_i2c_client->client->dev,
		s_ctrl->sensordata->sensor_platform_info->cam_vreg,
		s_ctrl->sensordata->sensor_platform_info->num_vreg,
		s_ctrl->reg_ptr, 0);
config_vreg_failed:
//kk0704.park ARUBA TEMP	msm_camera_request_gpio_table(data, 0);
request_gpio_failed:
	kfree(s_ctrl->reg_ptr);
	return rc;
}

int32_t msm_sensor_power_down_aruba_open(struct msm_sensor_ctrl_t *s_ctrl)
{
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;
	CDBG("%s\n", __func__);
	if (data->sensor_platform_info->i2c_conf &&
		data->sensor_platform_info->i2c_conf->use_i2c_mux)
		msm_sensor_disable_i2c_mux(
			data->sensor_platform_info->i2c_conf);

	if (data->sensor_platform_info->ext_power_ctrl != NULL)
		data->sensor_platform_info->ext_power_ctrl(0);
	msleep(1);

	cam_flash_off(FLASH_MODE_CAMERA);	//kk0704.park :: Flash Off when Power down

	gpio_set_value_cansleep(ARUBA_CAM_RESET, 0);	// GPIO 85	
	msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
		cam_clk_info, &s_ctrl->cam_clk, ARRAY_SIZE(cam_clk_info), 0);
	msleep(1);

	gpio_set_value_cansleep(ARUBA_CAM_STBY, 0);		//GPIO 96
	gpio_set_value_cansleep(FRONT_CAM_RESET, 0);
	gpio_set_value_cansleep(ARUBA_CAM_C_EN, 0);
	gpio_set_value_cansleep(ARUBA_CAM_IO_EN, 0);
	gpio_set_value_cansleep(ARUBA_CAM_AF_EN, 0);

	gpio_free(ARUBA_CAM_RESET);
	gpio_free(ARUBA_CAM_STBY);
	gpio_free(FRONT_CAM_RESET);
	gpio_free(ARUBA_CAM_C_EN);
	gpio_free(ARUBA_CAM_IO_EN);
	gpio_free(ARUBA_CAM_AF_EN);

	msm_camera_enable_vreg(&s_ctrl->sensor_i2c_client->client->dev,
		s_ctrl->sensordata->sensor_platform_info->cam_vreg,
		s_ctrl->sensordata->sensor_platform_info->num_vreg,
		s_ctrl->reg_ptr, 0);
	msm_camera_config_vreg(&s_ctrl->sensor_i2c_client->client->dev,
		s_ctrl->sensordata->sensor_platform_info->cam_vreg,
		s_ctrl->sensordata->sensor_platform_info->num_vreg,
		s_ctrl->reg_ptr, 0);
	kfree(s_ctrl->reg_ptr);
	return 0;
}
#elif (defined (CONFIG_MACH_ARUBA_OPEN) && !defined(CONFIG_MACH_KYLEPLUS_OPEN))
static struct msm_cam_clk_info cam_clk_info[] = {
	{"cam_m_clk", MSM_SENSOR_MCLK_24HZ},
};

int32_t msm_sensor_power_up_aruba_open(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;
	CDBG("%s: %d\n", __func__, __LINE__);
	s_ctrl->reg_ptr = kzalloc(sizeof(struct regulator *)
			* data->sensor_platform_info->num_vreg, GFP_KERNEL);
	if (!s_ctrl->reg_ptr) {
		pr_err("%s: could not allocate mem for regulators\n",
			__func__);
		return -ENOMEM;
	}
	rc = msm_camera_config_vreg(&s_ctrl->sensor_i2c_client->client->dev,
			s_ctrl->sensordata->sensor_platform_info->cam_vreg,
			s_ctrl->sensordata->sensor_platform_info->num_vreg,
			s_ctrl->reg_ptr, 1);
	if (rc < 0) {
		pr_err("%s: regulator on failed\n", __func__);
		goto config_vreg_failed;
	}

	gpio_request(ARUBA_CAM_STBY, "aruba_stby");
	gpio_request(ARUBA_CAM_RESET, "aruba_reset");
	gpio_request(ARUBA_CAM_IO_EN, "aruba_a_en");
	gpio_request(ARUBA_CAM_C_EN, "aruba_c_en");
	gpio_request(ARUBA_CAM_AF_EN, "aruba_af_en");


	gpio_direction_output(ARUBA_CAM_STBY, 0);
	gpio_direction_output(ARUBA_CAM_RESET, 0);
	gpio_direction_output(ARUBA_CAM_IO_EN, 0);
	gpio_direction_output(ARUBA_CAM_C_EN, 0);
	gpio_direction_output(ARUBA_CAM_AF_EN, 0);

	gpio_set_value_cansleep(ARUBA_CAM_STBY, 0);		//STBYN :: MSM_GPIO 96
	gpio_set_value_cansleep(ARUBA_CAM_RESET, 0);	//RSTN :: MSM_GPIO 85
	gpio_set_value_cansleep(ARUBA_CAM_IO_EN, 0);
	gpio_set_value_cansleep(ARUBA_CAM_C_EN, 0);

	msleep(1);
	gpio_set_value(ARUBA_CAM_C_EN, 1);
	gpio_set_value(ARUBA_CAM_AF_EN, 1);
	msleep(1);

	rc = msm_camera_enable_vreg(&s_ctrl->sensor_i2c_client->client->dev,
			s_ctrl->sensordata->sensor_platform_info->cam_vreg,
			s_ctrl->sensordata->sensor_platform_info->num_vreg,
			s_ctrl->reg_ptr, 1);
	if (rc < 0) {
		pr_err("%s: enable regulator failed\n", __func__);
		goto enable_vreg_failed;
	}
	msleep(1);
	gpio_set_value(ARUBA_CAM_IO_EN, 1);

//	gpio_tlmm_config(GPIO_CFG(15, 1, 1, 1, 3, 1),GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(ARUBA_CAM_FLASH_SET, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

	if (s_ctrl->clk_rate != 0)
		cam_clk_info->clk_rate = s_ctrl->clk_rate;

	rc = msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
		cam_clk_info, &s_ctrl->cam_clk, ARRAY_SIZE(cam_clk_info), 1);
	if (rc < 0) {
		pr_err("%s: clk enable failed\n", __func__);
		goto enable_clk_failed;
	}

	msleep(1);
	gpio_set_value_cansleep(ARUBA_CAM_STBY, 1);		//STBYN :: MSM_GPIO 96
	msleep(1);
	gpio_set_value_cansleep(ARUBA_CAM_RESET, 1);	//RSTN :: MSM_GPIO 85
	msleep(1);
	usleep_range(1000, 2000);

	if (data->sensor_platform_info->ext_power_ctrl != NULL)
		data->sensor_platform_info->ext_power_ctrl(1);

	if (data->sensor_platform_info->i2c_conf &&
		data->sensor_platform_info->i2c_conf->use_i2c_mux)
		msm_sensor_enable_i2c_mux(data->sensor_platform_info->i2c_conf);

	return rc;

enable_clk_failed:
//kk0704.park :: ARUBA TEMP		msm_camera_config_gpio_table(data, 0);
config_gpio_failed:
	msm_camera_enable_vreg(&s_ctrl->sensor_i2c_client->client->dev,
			s_ctrl->sensordata->sensor_platform_info->cam_vreg,
			s_ctrl->sensordata->sensor_platform_info->num_vreg,
			s_ctrl->reg_ptr, 0);

enable_vreg_failed:
	msm_camera_config_vreg(&s_ctrl->sensor_i2c_client->client->dev,
		s_ctrl->sensordata->sensor_platform_info->cam_vreg,
		s_ctrl->sensordata->sensor_platform_info->num_vreg,
		s_ctrl->reg_ptr, 0);
config_vreg_failed:
//kk0704.park ARUBA TEMP	msm_camera_request_gpio_table(data, 0);
request_gpio_failed:
	kfree(s_ctrl->reg_ptr);
	return rc;
}

int32_t msm_sensor_power_down_aruba_open(struct msm_sensor_ctrl_t *s_ctrl)
{
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;
	CDBG("%s\n", __func__);
	if (data->sensor_platform_info->i2c_conf &&
		data->sensor_platform_info->i2c_conf->use_i2c_mux)
		msm_sensor_disable_i2c_mux(
			data->sensor_platform_info->i2c_conf);

	if (data->sensor_platform_info->ext_power_ctrl != NULL)
		data->sensor_platform_info->ext_power_ctrl(0);
	msleep(1);

	cam_flash_off(FLASH_MODE_CAMERA);	//kk0704.park :: Flash Off when Power down

	gpio_set_value_cansleep(ARUBA_CAM_RESET, 0);	//RSTN :: MSM_GPIO 85
	msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
		cam_clk_info, &s_ctrl->cam_clk, ARRAY_SIZE(cam_clk_info), 0);
	msleep(1);
	gpio_set_value_cansleep(ARUBA_CAM_STBY, 0);		//STBYN :: MSM_GPIO 96
	gpio_set_value_cansleep(ARUBA_CAM_IO_EN, 0);
	gpio_set_value_cansleep(ARUBA_CAM_C_EN, 0);
	gpio_set_value_cansleep(ARUBA_CAM_AF_EN, 0);
	msleep(10);

	gpio_free(ARUBA_CAM_C_EN);
	gpio_free(ARUBA_CAM_AF_EN);
	gpio_free(ARUBA_CAM_IO_EN);
	gpio_free(ARUBA_CAM_STBY);
	gpio_free(ARUBA_CAM_RESET);
	msm_camera_enable_vreg(&s_ctrl->sensor_i2c_client->client->dev,
		s_ctrl->sensordata->sensor_platform_info->cam_vreg,
		s_ctrl->sensordata->sensor_platform_info->num_vreg,
		s_ctrl->reg_ptr, 0);
	msm_camera_config_vreg(&s_ctrl->sensor_i2c_client->client->dev,
		s_ctrl->sensordata->sensor_platform_info->cam_vreg,
		s_ctrl->sensordata->sensor_platform_info->num_vreg,
		s_ctrl->reg_ptr, 0);
	kfree(s_ctrl->reg_ptr);
	return 0;
}

#elif defined (CONFIG_MACH_DELOS_OPEN) || defined (CONFIG_MACH_DELOS_CTC) || defined(CONFIG_MACH_HENNESSY_DUOS_CTC)

static struct msm_cam_clk_info cam_clk_info[] = {
	{"cam_m_clk", MSM_SENSOR_MCLK_24HZ},
};

int32_t msm_sensor_power_up_delos_open(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	printk("msm_sensor_power_up_delos_open \n");
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;
	CDBG("%s: %d\n", __func__, __LINE__);

#if defined(CONFIG_MACH_DELOS_OPEN)
	gpio_tlmm_config(GPIO_CFG(CAM_MCLK, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(CAM_I2C_SCL, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(CAM_I2C_SDA, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
#endif
#if defined(CONFIG_MACH_DELOS_CTC)
	gpio_tlmm_config(GPIO_CFG(113, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	//set input GPIO CFG 113
#endif
#if defined (CONFIG_MACH_DELOS_DUOS_CTC) || defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
	rc = gpio_request(ARUBA_CAM_A_EN, "aruba_stby");
	gpio_direction_output(ARUBA_CAM_A_EN, 0);
	gpio_set_value_cansleep(ARUBA_CAM_A_EN, 0);
	gpio_set_value(ARUBA_CAM_A_EN, 1);
#endif

	s_ctrl->reg_ptr = kzalloc(sizeof(struct regulator *)
			* data->sensor_platform_info->num_vreg, GFP_KERNEL);
	if (!s_ctrl->reg_ptr) {
		pr_err("%s: could not allocate mem for regulators\n",
			__func__);
		return -ENOMEM;
	}
	rc = msm_camera_config_vreg(&s_ctrl->sensor_i2c_client->client->dev,
			s_ctrl->sensordata->sensor_platform_info->cam_vreg,
			s_ctrl->sensordata->sensor_platform_info->num_vreg,
			s_ctrl->reg_ptr, 1);
	if (rc < 0) {
		pr_err("%s: regulator on failed\n", __func__);
		goto config_vreg_failed;
	}

	rc = gpio_request(ARUBA_CAM_STBY, "aruba_stby");
	rc = gpio_request(ARUBA_CAM_RESET, "aruba_reset");
	rc = gpio_request(ARUBA_CAM_IO_EN, "aruba_a_en");
	rc = gpio_request(ARUBA_CAM_C_EN, "aruba_c_en");
	rc = gpio_request(ARUBA_CAM_AF_EN, "aruba_af_en");

	gpio_direction_output(ARUBA_CAM_STBY, 0);
	gpio_direction_output(ARUBA_CAM_RESET, 0);
	gpio_direction_output(ARUBA_CAM_IO_EN, 0);
	gpio_direction_output(ARUBA_CAM_C_EN, 0);
	gpio_direction_output(ARUBA_CAM_AF_EN, 0);

	gpio_set_value_cansleep(ARUBA_CAM_STBY, 0);		//STBYN :: MSM_GPIO 96
	gpio_set_value_cansleep(ARUBA_CAM_RESET, 0);	//RSTN :: MSM_GPIO 85
	gpio_set_value_cansleep(ARUBA_CAM_IO_EN, 0);
	gpio_set_value_cansleep(ARUBA_CAM_C_EN, 0);


	rc = msm_camera_enable_vreg(&s_ctrl->sensor_i2c_client->client->dev,
			s_ctrl->sensordata->sensor_platform_info->cam_vreg,
			s_ctrl->sensordata->sensor_platform_info->num_vreg,
			s_ctrl->reg_ptr, 1);
	if (rc < 0) {
		pr_err("%s: enable regulator failed\n", __func__);
		goto enable_vreg_failed;
	}

	//msleep(1);
	usleep(50);

//	gpio_set_value(FRONT_CAM_ID, 1);	//VT-VGA core High
//	printk("FRONT_CAM_ID = %d\n", gpio_get_value(FRONT_CAM_ID));
	gpio_set_value(ARUBA_CAM_IO_EN, 1);	//SENSOR I/O High
	printk("ARUBA_CAM_IO_EN = %d\n", gpio_get_value(ARUBA_CAM_IO_EN));
	usleep(50);
#if !defined(CONFIG_MACH_HENNESSY_DUOS_CTC)	
	gpio_set_value(FRONT_CAM_STBY, 1);	//VT STBY High
	usleep(10);
	printk("FRONT_CAM_STBY = %d\n", gpio_get_value(FRONT_CAM_STBY));
#endif	
	if (s_ctrl->clk_rate != 0)
		cam_clk_info->clk_rate = s_ctrl->clk_rate;

	rc = msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
		cam_clk_info, &s_ctrl->cam_clk, ARRAY_SIZE(cam_clk_info), 1);
	if (rc < 0) {
		pr_err("%s: clk enable failed\n", __func__);
		goto enable_clk_failed;
	}

	usleep(2000);	//at least VT Reset high 4ms after VT STBY on
#if !defined(CONFIG_MACH_HENNESSY_DUOS_CTC)	
	gpio_set_value(FRONT_CAM_RESET, 1);	//VT RESET High
	printk("FRONT_CAM_RESET = %d\n", gpio_get_value(FRONT_CAM_RESET));
	usleep(1000);  
	gpio_set_value(FRONT_CAM_STBY, 0);	//VT STBY Low
	printk("FRONT_CAM_STBY = %d\n", gpio_get_value(FRONT_CAM_STBY));
#endif	
	gpio_set_value(ARUBA_CAM_C_EN, 1);
	printk("ARUBA_CAM_C_EN = %d\n", gpio_get_value(ARUBA_CAM_C_EN));
	gpio_set_value(ARUBA_CAM_AF_EN, 1);	//5M Core High
	printk("ARUBA_CAM_AF_EN = %d\n", gpio_get_value(ARUBA_CAM_AF_EN));


//	gpio_tlmm_config(GPIO_CFG(15, 1, 1, 1, 3, 1),GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(ARUBA_CAM_FLASH_SET, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

	
	usleep(100);
	gpio_set_value_cansleep(ARUBA_CAM_STBY, 1);		//STBYN :: MSM_GPIO 96
	printk("ARUBA_CAM_STBY = %d\n", gpio_get_value(ARUBA_CAM_STBY));
	usleep(15);
	gpio_set_value_cansleep(ARUBA_CAM_RESET, 1);	//RSTN :: MSM_GPIO 85
	printk("ARUBA_CAM_RESET = %d\n", gpio_get_value(ARUBA_CAM_RESET));
	usleep(10);

	if (data->sensor_platform_info->ext_power_ctrl != NULL)
		data->sensor_platform_info->ext_power_ctrl(1);

	if (data->sensor_platform_info->i2c_conf &&
		data->sensor_platform_info->i2c_conf->use_i2c_mux)
		msm_sensor_enable_i2c_mux(data->sensor_platform_info->i2c_conf);

	return rc;

enable_clk_failed:
//kk0704.park :: ARUBA TEMP		msm_camera_config_gpio_table(data, 0);
config_gpio_failed:
	msm_camera_enable_vreg(&s_ctrl->sensor_i2c_client->client->dev,
			s_ctrl->sensordata->sensor_platform_info->cam_vreg,
			s_ctrl->sensordata->sensor_platform_info->num_vreg,
			s_ctrl->reg_ptr, 0);

enable_vreg_failed:
	msm_camera_config_vreg(&s_ctrl->sensor_i2c_client->client->dev,
		s_ctrl->sensordata->sensor_platform_info->cam_vreg,
		s_ctrl->sensordata->sensor_platform_info->num_vreg,
		s_ctrl->reg_ptr, 0);
config_vreg_failed:
//kk0704.park ARUBA TEMP	msm_camera_request_gpio_table(data, 0);
request_gpio_failed:
	kfree(s_ctrl->reg_ptr);
	return rc;
}

int32_t msm_sensor_power_down_delos_open(struct msm_sensor_ctrl_t *s_ctrl)
{
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;
	CDBG("%s\n", __func__);
	if (data->sensor_platform_info->i2c_conf &&
		data->sensor_platform_info->i2c_conf->use_i2c_mux)
		msm_sensor_disable_i2c_mux(
			data->sensor_platform_info->i2c_conf);

	if (data->sensor_platform_info->ext_power_ctrl != NULL)
		data->sensor_platform_info->ext_power_ctrl(0);
	msleep(1);
	printk("msm_sensor_power_down_delos_open \n");
	cam_flash_off(FLASH_MODE_CAMERA);	//kk0704.park :: Flash Off when Power down

	gpio_set_value_cansleep(ARUBA_CAM_RESET, 0);	//RSTN :: MSM_GPIO 85
	msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
		cam_clk_info, &s_ctrl->cam_clk, ARRAY_SIZE(cam_clk_info), 0);
	msleep(1);
	gpio_set_value_cansleep(ARUBA_CAM_STBY, 0);		//STBYN :: MSM_GPIO 96
	usleep(10);
#if !defined(CONFIG_MACH_HENNESSY_DUOS_CTC)	
	gpio_set_value_cansleep(FRONT_CAM_RESET,0);
#endif	
	gpio_set_value_cansleep(ARUBA_CAM_C_EN, 0);
	usleep(2000);
	gpio_set_value_cansleep(ARUBA_CAM_IO_EN, 0);
//	gpio_set_value_cansleep(FRONT_CAM_ID, 0);
	gpio_set_value_cansleep(ARUBA_CAM_AF_EN, 0);
#if defined (CONFIG_MACH_DELOS_DUOS_CTC) || defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
	gpio_set_value_cansleep(ARUBA_CAM_A_EN, 0);
#endif

	msm_camera_enable_vreg(&s_ctrl->sensor_i2c_client->client->dev,
		s_ctrl->sensordata->sensor_platform_info->cam_vreg,
		s_ctrl->sensordata->sensor_platform_info->num_vreg,
		s_ctrl->reg_ptr, 0);
	msm_camera_config_vreg(&s_ctrl->sensor_i2c_client->client->dev,
		s_ctrl->sensordata->sensor_platform_info->cam_vreg,
		s_ctrl->sensordata->sensor_platform_info->num_vreg,
		s_ctrl->reg_ptr, 0);
	kfree(s_ctrl->reg_ptr);

#if defined (CONFIG_MACH_DELOS_DUOS_CTC) || defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
	gpio_free(ARUBA_CAM_A_EN);
#endif
	gpio_free(ARUBA_CAM_C_EN);
	gpio_free(ARUBA_CAM_AF_EN);
	gpio_free(ARUBA_CAM_IO_EN);
	gpio_free(ARUBA_CAM_STBY);
	gpio_free(ARUBA_CAM_RESET);

#if defined(CONFIG_MACH_DELOS_OPEN)
	gpio_tlmm_config(GPIO_CFG(CAM_MCLK, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(CAM_I2C_SCL, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(CAM_I2C_SDA, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
#endif	
#if defined(CONFIG_MACH_DELOS_CTC)
	gpio_tlmm_config(GPIO_CFG(113, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	//set input GPIO CFG 113
#endif
	return 0;
}
#endif


