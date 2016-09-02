/* ROY GPIOS */

#ifndef __GPIO_ROY_H_
#define __GPIO_ROY_H_

#define GPIO_LCD_MCLK			4
#define GPIO_LCD_R_7			5
#define GPIO_LCD_R_6			6
#define GPIO_LCD_R_5			7
#define GPIO_LCDR_4				8
#define GPIO_LCD_R_3			9
#define GPIO_LCD_R_2			10
#define GPIO_LCD_R_1			11
#define GPIO_LCD_R_0			12
#define GPIO_LCD_G_7			13
#define GPIO_LCD_G_6			14
#define GPIO_CAM_MCLK			15
							/* 16 */
							/* 17 */
#define GPIO_SENSOR_SCL			16
#define GPIO_SENSOR_SDA			17


#define GPIO_PHONE_RXD			21
#define GPIO_LCD_RESET_N		23
#define GPIO_PM_INT_N			24
#define GPIO_PS_HOLD			25
#define GPIO_TOUCH_IRQ			27
#define GPIO_MUSB_INT			28
#define GPIO_PROXI_INT			29
#define GPIO_KBC_0				31
							/* 32 */
#define GPIO_WLAN_RESET_N		33

#define GPIO_BT_PWR			77
#define GPIO_BT_HOST_WAKE		86

#define GPIO_TSP_SCL			35
#define GPIO_KBR_1				36
#define GPIO_KBR_2				37

#if (!defined(CONFIG_MACH_ROY_DTV_HWREV)) || (CONFIG_MACH_ROY_DTV_HWREV==0x0)  //	SJINU_DEBUG_HWREV
#define GPIO_LCD_DETECT		38
#define GPIO_UART_RXD_WAKEUP		114
#define	GPIO_ISDBT_DET			115
#else 
// in case of CONFIG_MACH_ROY_DTV_HWREV==01
#define GPIO_LCD_DETECT			34
#define GPIO_UART_RXD_WAKEUP		38
#define GPIO_ISDBT_DET			107
#endif

#define GPIO_KBR_4				39

#define GPIO_TSP_SDA			40
#define GPIO_TOUCH_EN			41
#define GPIO_WLAN_HOST_WAKE		42
#define GPIO_BT_UART_RTS		43
#define GPIO_BT_UART_CTS		44
#define GPIO_BT_UART_RXD		45
#define GPIO_BT_UART_TXD		46
#define GPIO_SIM1_CLK_MSM		47
#define GPIO_JACK_INT_N			48
							/* 49 */
#define GPIO_SIM1_DATA_MSM		50
#define GPIO_MICROSD_DATA_3		51
#define GPIO_MICROSD_DATA_2		52
#define GPIO_MICROSD_DATA_1		53
#define GPIO_MICROSD_DATA_0		54
#define GPIO_MICROSD_CMD		55
#define GPIO_MICROSD_CLK		56
#define GPIO_FM_SCL				58
#define GPIO_SBDT_RTR6285A		59
#define GPIO_CAM_I2C_SCL		60
#define GPIO_CAM_I2C_SDA		61
#define GPIO_WLAN_SD_CLK		62
#define GPIO_WLAN_SD_CMD		63
#define GPIO_WLAN_SD_DATA_3		64
#define GPIO_WLAN_SD_DATA_2		65
#define GPIO_WLAN_SD_DATA_1		66
#define GPIO_WLAN_SD_DATA_0		67
#define GPIO_BT_PCM_DOUT		68
#define GPIO_BT_PCM_DIN			69
#define GPIO_BT_PCM_SYNC		70
#define GPIO_BT_PCM_CLK			71
#define GPIO_ANT_SEL_3			72
#define GPIO_ANT_SEL_2			73
#define GPIO_ANT_SEL_1			74

#define GPIO_KEY_LED_EN			76
#define GPIO_ANT_SEL_0			80

#define GPIO_GPS_LNA_ON			81
							/* 82 */
#define GPIO_FM_RDS_INT			83


#define GPIO_3M_CAM_RESET		85
							/* 86 */

#define GPIO_CAMIO_EN			88

							/* 89 */
#ifdef CONFIG_MACH_ROY_DTV // ROY_LTN_DTV uses UART1(GPIO 122/123 as UART TX/RX)
#define GPIO_MUS_SCL			131
#define GPIO_MUS_SDA			132
#else
#define GPIO_MUS_SCL			123
#define GPIO_MUS_SDA			122
#endif


#define GPIO_nTF_DETECT			94


#define GPIO_3M_CAM_STBY		96
#define GPIO_UART_BOOT_ON		97
							/* 98 */
#if (!defined(CONFIG_MACH_ROY_DTV_HWREV)) || (CONFIG_MACH_ROY_DTV_HWREV==0x0)
#define GPIO_WLAN_18V_EN		114
#else
#define GPIO_WLAN_18V_EN		107
#endif

#define GPIO_PHONE_TXD			108

#define GPIO_WLAN_33V_EN		109

#define GPIO_PA_ON_W900			110
#define GPIO_LCD_G_1			111

#ifdef CONFIG_MACH_NEVIS3G
#define GPIO_ACC_INT			41
#else
#define GPIO_ACC_INT			112
#endif
#define GPIO_LCD_B_7			113
#define GPIO_LCD_B_6			114
#define GPIO_LCD_B_5			115
#define GPIO_LCD_B_4			116
#define GPIO_LCD_B_3			117
#define GPIO_LCD_B_2			118
#define GPIO_LCD_G_4			119
#define GPIO_LCD_G_3			120
#define GPIO_LCDE_G_2			121
							/* 122 */


#define GPIO_LCD_B_1			125
#define GPIO_LCD_B_0			126
#define GPIO_LCD_VSYNC			127
#define GPIO_LCD_HSYNC			128
#define GPIO_LCD_EN				129
#define GPIO_LCD_G_5			130
#define GPIO_I2C_SCL			131
#define GPIO_I2C_SDA			132

#endif	/* __GPIO_ROY_H_ */
