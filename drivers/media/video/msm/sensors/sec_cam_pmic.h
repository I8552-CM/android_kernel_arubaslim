
#ifndef _SEC_CAM_PMIC_H
#define _SEC_CAM_PMIC_H
#include "msm_sensor.h"

#define	ON		1
#define	OFF		0
#define LOW		0
#define HIGH		1

#define FLASH_MODE_NONE		0
#define FLASH_MODE_CAMERA		1
#define FLASH_MODE_LIGHT		2

#if defined(CONFIG_S5K5CCGX)
#define S5K5CCGX_CAM_STBY	 96
#define S5K5CCGX_CAM_RESET	 85
#if defined(CONFIG_MACH_INFINITE_DUOS_CTC) || defined(CONFIG_MACH_DELOS_DUOS_CTC) || defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
#define S5K5CCGX_CAM_A_EN	128		//VCAM_A_2.8V
#elif defined(CONFIG_MACH_KYLEPLUS_OPEN)
#define S5K5CCGX_CAM_IO_EN	  4		//VCAM_IO_1.8V
#elif defined(CONFIG_MACH_KYLEPLUS_CTC)
#define S5K5CCGX_CAM_A_EN	128		//VCAM_A_2.8V
#if (CONFIG_MACH_KYLEPLUS_CTC_HWREV == 0x0)
#define S5K5CCGX_CAM_IO_EN	4	//VCAM_IO_1.8V
#elif (CONFIG_MACH_KYLEPLUS_CTC_HWREV == 0x1)
#define S5K5CCGX_CAM_IO_EN	129	//VCAM_IO_1.8V
#endif
#endif
#endif


#if defined (CONFIG_MACH_ARUBA_OPEN)	// temperary setting
#define ARUBA_CAM_IO_EN	4
#define ARUBA_CAM_C_EN		107
#define ARUBA_CAM_AF_EN	49
#define ARUBA_CAM_STBY		96
#define ARUBA_CAM_RESET	85
#define ARUBA_CAM_FLASH_EN	58
extern unsigned int board_hw_revision;
#define ARUBA_CAM_FLASH_SET	5
#define ARUBA_CAM_FLASH_SET_OLD	86
#define ARUBA_CAM_A_EN	4
#elif defined (CONFIG_MACH_ARUBASLIM_OPEN)
#define ARUBA_CAM_IO_EN	4
#define ARUBA_CAM_C_EN		107
#define ARUBA_CAM_AF_EN	49
#define ARUBA_CAM_STBY		96
#define ARUBA_CAM_RESET	85
#define ARUBA_CAM_FLASH_EN	58
extern unsigned int board_hw_revision;
#define ARUBA_CAM_FLASH_SET	5
#define ARUBA_CAM_FLASH_SET_OLD	86
#define ARUBA_CAM_A_EN	4
#define FRONT_CAM_STBY		76
#define FRONT_CAM_RESET	98
#elif defined (CONFIG_MACH_DELOS_OPEN)
#define CAM_I2C_SCL 60
#define CAM_I2C_SDA 61
#define CAM_MCLK 15
#define ARUBA_CAM_A_EN  76   //temp
#define ARUBA_CAM_IO_EN	4
#define ARUBA_CAM_C_EN		12
#define FRONT_CAM_STBY  57
#define FRONT_CAM_RESET 98
#define FRONT_CAM_ID    131
#define ARUBA_CAM_AF_EN	49
#define ARUBA_CAM_STBY		96
#define ARUBA_CAM_RESET	85
#define ARUBA_CAM_FLASH_EN	5
#define ARUBA_CAM_FLASH_SET	58
#elif defined (CONFIG_MACH_DELOS_CTC) || defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
#define ARUBA_CAM_A_EN	128
#define ARUBA_CAM_C_EN	119
#define ARUBA_CAM_IO_EN 75 //delos CTC does not use gpio 75 for temp
#define ARUBA_CAM_AF_EN	129
#define ARUBA_CAM_FLASH_EN	11
#define ARUBA_CAM_FLASH_SET	14
#define ARUBA_CAM_STBY		96
#define ARUBA_CAM_RESET	85
#define FRONT_CAM_STBY 83
#define FRONT_CAM_RESET	118
#else	//CONFIG_MACH_ARUBA_OPEN
#define ARUBA_CAM_A_EN	128
#if defined (CONFIG_S5K4ECGX)
#define ARUBA_CAM_C_EN	107
#define ARUBA_CAM_AF_EN	129
#define ARUBA_CAM_FLASH_EN	14
#define ARUBA_CAM_FLASH_SET	11
#endif
#define ARUBA_CAM_STBY		96
#define ARUBA_CAM_RESET	85
#if defined (CONFIG_MACH_BAFFIN_DUOS_CTC)
#define FRONT_CAM_C_EN	73
#define FRONT_CAM_STBY 75
#define FRONT_CAM_RESET	80
#endif	//CONFIG_MACH_ARUBA_OPEN
#endif

void cam_ldo_power_on_1(void);
void cam_ldo_power_on_2(void);
void cam_ldo_power_off_1(void);
void cam_ldo_power_off_2(void);

#ifdef CONFIG_S5K4ECGX
void cam_flash_main_on(int flashMode);
void cam_flash_torch_on(int flashMode);
void cam_flash_off(int flashMode);
static int flash_mode = FLASH_MODE_NONE;
#endif

#ifdef CONFIG_MACH_BAFFIN_DUOS_CTC
int32_t msm_sensor_power_up_baffin_duos(struct msm_sensor_ctrl_t *s_ctrl);
int32_t msm_sensor_power_down_baffin_duos(struct msm_sensor_ctrl_t *s_ctrl);
#endif

#if defined (CONFIG_MACH_DELOS_OPEN) || defined (CONFIG_MACH_DELOS_CTC) || defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
int32_t msm_sensor_power_up_delos_open(struct msm_sensor_ctrl_t *s_ctrl);
int32_t msm_sensor_power_down_delos_open(struct msm_sensor_ctrl_t *s_ctrl);
#endif

#if defined (CONFIG_MACH_ARUBA_OPEN) || defined (CONFIG_MACH_ARUBASLIM_OPEN)
int32_t msm_sensor_power_up_aruba_open(struct msm_sensor_ctrl_t *s_ctrl);
int32_t msm_sensor_power_down_aruba_open(struct msm_sensor_ctrl_t *s_ctrl);
#endif

#if defined (CONFIG_MACH_DELOS_OPEN) || defined (CONFIG_MACH_ARUBASLIM_OPEN)
extern int rear_sensor_check_vendorID(int *vendorID);
#endif
#endif
