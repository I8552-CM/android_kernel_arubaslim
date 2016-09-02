/* DELOS GPIOS */

#ifndef __GPIO_DELOS_H_
#define __GPIO_DELOS_H_

							/* 4 */
							/* 5 */
							/* 6 */
							/* 7 */
							/* 8 */
							/* 9 */
							/* 10 */
							/* 11 */
							/* 12 */
							/* 13 */
							/* 14 */
#define GPIO_CAM_MCLK			15
#if defined(CONFIG_MACH_DELOS_CTC) || defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
#define GPIO_SENSOR_SCL 		16
#define GPIO_SENSOR_SDA			4
#elif defined(CONFIG_MACH_DELOS_OPEN)
#define GPIO_SENSOR_SCL 		16
#define GPIO_SENSOR_SDA			17
#endif/* 19 */
#define GPIO_PHONE_RXD			21
#if defined(CONFIG_MACH_DELOS_CTC) || defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
#define GPIO_LCD_RESET_N			22
#elif defined(CONFIG_MACH_DELOS_OPEN)
#define GPIO_LCD_RESET_N			23
#endif
#define GPIO_PM_INT_N			24
#define GPIO_PS_HOLD			25
							/* 26 */
#define GPIO_TOUCH_IRQ			27

#define GPIO_MUSB_INT			28
#define GPIO_PROXI_INT			29
							/* 30 */
#define GPIO_KBC_0				31
							/* 32 */
#define GPIO_WLAN_RESET_N		33
#if defined(CONFIG_MACH_DELOS_OPEN)
#define GPIO_BT_PWR			77
#define GPIO_BT_HOST_WAKE		18
#define GPIO_BT_HOST_WAKE_GPIO34        18
#elif defined(CONFIG_MACH_DELOS_CTC) || defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
#define GPIO_BT_PWR			34
#define GPIO_BT_HOST_WAKE		18
#define GPIO_BT_HOST_WAKE_rev03		18
#endif

#if defined(CONFIG_MACH_DELOS_CTC) || defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
#define GPIO_TSP_SCL			131
#define GPIO_TSP_SDA			132
#elif defined(CONFIG_MACH_DELOS_OPEN)
#define GPIO_TSP_SCL			35
#define GPIO_TSP_SDA			40
#endif

#define GPIO_KBR_1				36
#define GPIO_KBR_2				37
#if defined(CONFIG_MACH_DELOS_OPEN)
#define GPIO_LCD_DETECT			13
#elif defined(CONFIG_MACH_DELOS_CTC) || defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
#define GPIO_LCD_DETECT			49
#endif
#define GPIO_KBR_4				39



#define GPIO_FM_SDA				41
#define GPIO_WLAN_HOST_WAKE		42
#define GPIO_BT_UART_RTS		43
#define GPIO_BT_UART_CTS		44
#define GPIO_BT_UART_RXD		45
#define GPIO_BT_UART_TXD		46
#define GPIO_SIM1_CLK_MSM		47
#define GPIO_JACK_INT_N			48
#define GPIO_SIM1_DATA_MSM		50
#define GPIO_MICROSD_DATA_3		51
#define GPIO_MICROSD_DATA_2		52
#define GPIO_MICROSD_DATA_1		53
#define GPIO_MICROSD_DATA_0		54
#define GPIO_MICROSD_CMD		55
#define GPIO_MICROSD_CLK		56
							/* 57 */
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
#define GPIO_PROX_LDO			82
//CSR BT #define GPIO_FM_RDS_INT			83

#define GPIO_3M_CAM_RESET		85
							/* 86 */

#define GPIO_CAMIO_EN			88

							/* 89 */
#define GPIO_MUS_SCL			123
#define GPIO_MUS_SDA			122

#define GPIO_nTF_DETECT			94

#define GPIO_3M_CAM_STBY		96
#define GPIO_UART_BOOT_ON		97

#if defined(CONFIG_MACH_DELOS_DUOS_CTC)
#define GPIO_UART1_SEL			98
#endif

#define GPIO_WLAN_18V_EN		107

#define GPIO_PHONE_TXD			108

#define GPIO_WLAN_33V_EN		109


#define GPIO_PA_ON_W900			110

#if defined(CONFIG_MACH_DELOS_DUOS_CTC)
#define GPIO_VGA_CAM_ID		113
#endif
							/* 111 */
							/* 112 */
							/* 113 */
							/* 114 */
							/* 115 */
							/* 116 */
							/* 117 */
							/* 118 */
							/* 119 */
							/* 120 */
							/* 121 */
							/* 122 */
							/* 125 */
							/* 126 */
							/* 127 */
							/* 128 */
							/* 129 */
							/* 130 */
#if defined(CONFIG_MACH_DELOS_CTC) || defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
#define GPIO_I2C_SCL			75 // null  131 is tsp
#define GPIO_I2C_SDA			75 // null 132 is tsp
#elif defined(CONFIG_MACH_DELOS_OPEN)
#define GPIO_I2C_SCL			131
#define GPIO_I2C_SDA			132
#endif

#endif	/* __GPIO_DELOS_H_ */
