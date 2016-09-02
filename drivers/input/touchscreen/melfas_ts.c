/* drivers/input/touchscreen/melfas_ts.c
 *
 * Copyright (C) 2010 Melfas, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#if  defined(CONFIG_MACH_ARUBASLIM_OPEN)
 //#define TUNNING_SUPPORT
 #endif
 
#define SEC_TSP
#ifdef SEC_TSP
#define SEC_TSP_FACTORY_TEST
#define TSP_FACTORY_TEST
#define TSP_BUF_SIZE 1024
#undef ENABLE_NOISE_TEST_MODE
#if defined(CONFIG_KOR_MODEL_SHV_E120S) \
	|| defined(CONFIG_KOR_MODEL_SHV_E120K) \
	|| defined(CONFIG_KOR_MODEL_SHV_E120L) \
	|| defined(CONFIG_KOR_MODEL_SHV_E160S) \
	|| defined(CONFIG_KOR_MODEL_SHV_E160K) \
	|| defined(CONFIG_KOR_MODEL_SHV_E160L)
#define TSP_BOOST
#else
#undef TSP_BOOST
#endif
#endif
#undef TA_DETECTION
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/melfas_ts.h>
#include <linux/mfd/pmic8058.h>

#ifdef CONFIG_SEC_DVFS
#define TSP_BOOSTER
#else
#undef TSP_BOOSTER
#endif
#if defined(TSP_BOOSTER)
#include <linux/cpufreq.h>
#endif

#ifdef SEC_TSP
#include <linux/gpio.h>
#endif
#if 1
#include <linux/miscdevice.h>
#include <linux/ioctl.h>
#include <linux/string.h>
#include <linux/semaphore.h>
#include <linux/kthread.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/firmware.h>
#include <linux/input/mt.h>
#ifdef TUNNING_SUPPORT
#include <linux/cdev.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#endif
#include <asm/io.h>
#include <mach/gpio.h>
#include <mach/vreg.h>
#endif
#include <../mach-msm/smd_private.h>
#include <../mach-msm/smd_rpcrouter.h>

#ifdef SEC_TSP
#define P5_THRESHOLD			0x05
//#define TS_READ_REGS_LEN		5
#define TS_WRITE_REGS_LEN		16
#endif

#if defined(TSP_BOOSTER)
#define TOUCH_BOOSTER			1
#define TOUCH_BOOSTER_OFF_TIME	100
#define TOUCH_BOOSTER_CHG_TIME	200
#endif

#define TS_READ_REGS_LEN		66
#define MELFAS_MAX_TOUCH		5
static int FW_VERSION;
#define DEBUG_PRINT			1
#define PRESS_KEY					1
#define RELEASE_KEY					0
#define SET_DOWNLOAD_BY_GPIO	1
#ifdef TUNNING_SUPPORT
#define MAX_FINGER_NUM			10
#define MAX_LOG_LENGTH			128
#define MMS_UNIVERSAL_CMD		0xA0
#define MMS_UNIVERSAL_RESULT_SZ	0xAE
#define MMS_UNIVERSAL_RESULT	0xAF
#define MMS_CMD_SET_LOG_MODE	0x20
#define MMS_EVENT_PKT_SZ		0x0F
#define MMS_INPUT_EVENT			0x10
#define MMS_LOG_EVENT			0xD
#define MMS_NOTIFY_EVENT		0xE
#define MMS_ERROR_EVENT			0xF
#define FINGER_EVENT_SZ			6
#endif
#if defined(CONFIG_MACH_NEVIS3G)
#define MELFAS_MMS128S
#elif defined(CONFIG_MACH_DELOS_DUOS_CTC) || defined(CONFIG_MACH_DELOS_OPEN) || defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
#define MELFAS_MMS136_4P66
#elif defined(CONFIG_MACH_KYLEPLUS_CTC) || defined(CONFIG_MACH_KYLEPLUS_OPEN) || defined(CONFIG_MACH_INFINITE_DUOS_CTC) || defined(CONFIG_MACH_DELOS_DUOS_CTC) || defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
#define MELFAS_MMS134S
#else //defined(CONFIG_MACH_ARUBA_DUOS_CTC) || defined(CONFIG_MACH_ARUBA_OPEN)
#define MELFAS_MMS136_4P3
#endif

#define TS_MAX_Z_TOUCH		255
#define TS_MAX_W_TOUCH		100
#if defined(MELFAS_MMS128S)	//Nevis
#define TS_MAX_X_COORD		320
#define TS_MAX_Y_COORD		480
#else
#define TS_MAX_X_COORD		480
#define TS_MAX_Y_COORD		800
#endif

#if defined(MELFAS_MMS136_4P66) || defined(MELFAS_MMS136_4P3)
#define SET_DOWNLOAD_CONFIG		1
//#define SET_DOWNLOAD_ISP   
#endif

#define TA_SET_NOISE_MODE

#define TS_READ_VERSION_ADDR		0x1B
#define DOWNLOAD_RETRY_CNT		3
#define MIP_CONTACT_ON_EVENT_THRES	0x05
#define MIP_MOVING_EVENT_THRES		0x06
#define MIP_ACTIVE_REPORT_RATE		0x07
#define MIP_POSITION_FILTER_LEVEL	0x08
#define TS_READ_START_ADDR			0x0F
#define TS_READ_START_ADDR2			0x10
#define MIP_TSP_REVISION				0xF0
#define MIP_HARDWARE_REVISION		0xF1
#define MIP_COMPATIBILITY_GROUP		0xF2
#define MIP_CORE_VERSION				0xF3
#define MIP_PRIVATECUSTOM_VERSION	0xF4
#define MIP_PUBLICCUSTOM_VERSION		0xF5
#define MIP_PRODUCT_CODE				0xF6
#ifdef SEC_TSP_FACTORY_TEST
#if defined(MELFAS_MMS136_4P66)
#define TX_NUM		21
#define RX_NUM		14
#define NODE_NUM	294 /* 21x14 */
#elif defined(MELFAS_MMS134S)
#define TX_NUM		19
#define RX_NUM		12
#define NODE_NUM	228 /* 19x12 */
#elif defined(MELFAS_MMS128S)
#define TX_NUM		16
#define RX_NUM		11
#define NODE_NUM	228 /* 19x12 */
#else
#define TX_NUM		20
#define RX_NUM		12
#define NODE_NUM	240 /* 20x12 */
#endif
/* VSC(Vender Specific Command)  */
#define MMS_VSC_CMD			0xB0	/* vendor specific command */
#define MMS_VSC_MODE			0x1A	/* mode of vendor */
#define MMS_VSC_CMD_ENTER		0X01
#define MMS_VSC_CMD_CM_DELTA		0X02
#define MMS_VSC_CMD_CM_ABS		0X03
#define MMS_VSC_CMD_EXIT		0X05
#define MMS_VSC_CMD_INTENSITY		0X04
#define MMS_VSC_CMD_RAW			0X06
#define MMS_VSC_CMD_REFER		0X07
#define VSC_INTENSITY_TK		0x14
#define VSC_RAW_TK			0x16
#define VSC_THRESHOLD_TK		0x18
#define TSP_CMD_STR_LEN			32
#define TSP_CMD_RESULT_STR_LEN		512
#define TSP_CMD_PARAM_NUM		8
#endif /* SEC_TSP_FACTORY_TEST */
#define SET_TSP_CONFIG
#define TSP_PATTERN_TRACTKING
#undef TSP_PATTERN_TRACTKING
#ifdef SET_DOWNLOAD_BY_GPIO
#ifdef SET_DOWNLOAD_CONFIG

#ifdef SET_DOWNLOAD_ISP
#include <mms100_isp.h>
//#include <mms_ts.h>
#else
#include "mms100_cfg_update.h"
#endif

#define GPIO_TOUCH_INT  		27
#if defined(CONFIG_MACH_DELOS_CTC) || defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
#define GPIO_TSP_SCL    		131
#define GPIO_TSP_SDA      		132
#else
#define GPIO_TSP_SCL    		35
#define GPIO_TSP_SDA      		40
#endif

#else
#include <MMS100S_ISC_Updater.h>
#endif
#endif
#define TS_READ_EXCITING_CH_ADDR	0x2E
#define TS_READ_SENSING_CH_ADDR	    0x2F
#define TS_WRITE_REGS_LEN		    16
#define RMI_ADDR_TESTMODE           0xA0
#define UNIVERSAL_CMD_RESULT_SIZE   0xAE
#define UNIVERSAL_CMD_RESULT        0xAF

#if  defined(CONFIG_MACH_DELOS_OPEN)
#define	TSP_PWR_LDO_GPIO	34
#elif defined(CONFIG_MACH_NEVIS3G)
#define	TSP_PWR_LDO_GPIO	131
#else
#define	TSP_PWR_LDO_GPIO	41
#endif

unsigned long saved_rate;
//static bool lock_status;
static int tsp_enabled;
int touch_is_pressed;
static int tsp_testmode;
static int index;
static int gMenuKey_Intensity, gBackKey_Intensity;
static int before_gMenuKey_Intensity, before_gBackKey_Intensity;
static int gMenuKey_state=0,gBackKey_state=0,gMenuKey_show=1,gBackKey_show=1;

extern unsigned int board_hw_revision;

#if defined(MELFAS_MMS128S)
#define MAX_TX_	16
#define MAX_RX_	11
static const uint16_t SCR_ABS_UPPER_SPEC[MAX_RX_][MAX_TX_] = {
	{1333,1380,1411,1424,1435,1458,1482,1483,1469,1478,1480,1481,1482,1484,1484,1441},
	{1345,1381,1416,1440,1455,1480,1506,1509,1511,1511,1512,1514,1515,1515,1513,1463},
	{1341,1378,1411,1430,1442,1463,1487,1491,1490,1489,1489,1491,1492,1493,1493,1451},
	{1349,1380,1415,1437,1448,1469,1492,1496,1495,1494,1495,1497,1498,1499,1498,1456},
	{1352,1378,1413,1439,1450,1470,1494,1499,1497,1497,1498,1499,1500,1501,1500,1458},
	{1357,1378,1414,1447,1457,1476,1500,1505,1502,1502,1503,1505,1506,1506,1505,1462},
	{1359,1376,1414,1451,1460,1477,1500,1506,1503,1502,1503,1505,1506,1506,1505,1463},
	{1357,1374,1410,1450,1459,1475,1498,1507,1503,1502,1503,1505,1506,1506,1505,1464},
	{1350,1370,1403,1441,1450,1466,1490,1502,1499,1499,1499,1501,1502,1502,1501,1458},
	{1353,1371,1403,1450,1465,1482,1508,1523,1520,1521,1523,1525,1525,1524,1521,1469},
	{1341,1368,1390,1438,1434,1444,1465,1482,1497,1496,1496,1497,1498,1499,1498,1452},
};

static const uint16_t SCR_ABS_LOWER_SPEC[MAX_RX_][MAX_TX_] = {
	{800,828,847,854,861,875,889,890,881,887,888,889,889,890,890,864},
	{807,829,850,864,873,888,904,905,907,906,907,909,909,909,908,878},
	{805,827,847,858,865,878,892,894,894,893,894,895,895,896,896,870},
	{809,828,849,862,869,881,895,898,897,897,897,898,899,899,899,874},
	{811,827,848,864,870,882,897,899,898,898,899,900,900,901,900,875},
	{814,827,848,868,874,886,900,903,901,901,902,903,903,904,903,877},
	{815,826,848,871,876,886,900,903,902,901,902,903,904,904,903,878},
	{814,824,846,870,875,885,899,904,902,901,902,903,904,904,903,878},
	{810,822,842,865,870,879,894,901,900,899,900,900,901,901,901,875},
	{812,822,842,870,879,889,905,914,912,913,914,915,915,914,913,881},
	{804,821,834,863,860,867,879,889,898,897,898,898,899,899,899,871},
};
#elif defined(MELFAS_MMS134S) 
#define MAX_RX_	12
#define MAX_TX_	19
static const uint16_t SCR_ABS_UPPER_SPEC[MAX_RX_][MAX_TX_] = {
	{3171,3304,3323,3339,3352,3362,3365,3369,3371,3371,3381,3383,3385,3387,3388,3389,3390,3390,3377},
	{3187,3308,3320,3339,3349,3360,3362,3367,3368,3370,3377,3379,3381,3383,3384,3385,3385,3386,3372},
	{3199,3315,3334,3351,3367,3377,3383,3388,3393,3397,3403,3406,3409,3410,3411,3411,3411,3411,3380},
	{3212,3325,3337,3355,3365,3377,3379,3383,3384,3387,3392,3394,3396,3398,3399,3400,3400,3400,3387},
	{3218,3324,3340,3355,3368,3377,3381,3383,3386,3388,3391,3394,3396,3398,3399,3400,3400,3400,3386},
	{3220,3326,3339,3356,3367,3377,3380,3383,3385,3388,3391,3394,3396,3398,3399,3399,3400,3400,3386},
	{3214,3320,3334,3350,3362,3370,3374,3377,3379,3382,3384,3387,3389,3391,3392,3393,3394,3394,3379},
	{3210,3320,3334,3350,3362,3370,3373,3376,3379,3382,3384,3387,3389,3391,3392,3392,3393,3393,3379},
	{3208,3320,3335,3350,3363,3370,3373,3376,3379,3381,3382,3386,3388,3390,3391,3392,3393,3393,3379},
	{3204,3321,3338,3358,3372,3381,3385,3391,3396,3402,3404,3408,3410,3412,3412,3412,3413,3412,3379},
	{3198,3314,3329,3345,3357,3365,3368,3370,3373,3376,3373,3378,3380,3382,3383,3384,3385,3385,3373},
	{3208,3324,3338,3354,3365,3373,3375,3378,3380,3383,3350,3376,3382,3384,3385,3384,3387,3388,3375},
};

static const uint16_t SCR_ABS_LOWER_SPEC[MAX_RX_][MAX_TX_] = {
	{1903,1983,1994,2003,2011,2017,2019,2021,2022,2023,2028,2030,2031,2032,2033,2033,2034,2034,2026},
	{1912,1985,1992,2003,2009,2016,2017,2020,2021,2022,2026,2027,2029,2030,2030,2031,2031,2032,2023},
	{1919,1989,2000,2011,2020,2026,2030,2033,2036,2038,2042,2044,2045,2046,2046,2047,2047,2046,2028},
	{1927,1995,2002,2013,2019,2026,2027,2030,2031,2032,2035,2036,2038,2039,2039,2040,2040,2040,2032},
	{1931,1995,2004,2013,2021,2026,2028,2030,2031,2033,2035,2036,2037,2039,2039,2040,2040,2040,2032},
	{1932,1995,2003,2013,2020,2026,2028,2030,2031,2033,2035,2036,2038,2039,2039,2040,2040,2040,2032},
	{1929,1992,2001,2010,2017,2022,2024,2026,2028,2029,2030,2032,2034,2035,2035,2036,2036,2036,2028},
	{1926,1992,2000,2010,2017,2022,2024,2026,2027,2029,2030,2032,2033,2034,2035,2035,2036,2036,2027},
	{1925,1992,2001,2010,2018,2022,2024,2026,2027,2029,2029,2031,2033,2034,2034,2035,2036,2036,2027},
	{1922,1993,2003,2015,2023,2028,2031,2035,2038,2041,2042,2045,2046,2047,2047,2047,2048,2047,2027},
	{1919,1988,1997,2007,2014,2019,2021,2022,2024,2026,2024,2027,2028,2029,2030,2030,2031,2031,2024},
	{1925,1994,2003,2012,2019,2024,2025,2027,2028,2030,2010,2026,2029,2030,2031,2031,2032,2033,2025},
};
#elif defined(MELFAS_MMS136_4P66)
#define MAX_RX_	14
#define MAX_TX_	21
static const uint16_t SCR_ABS_UPPER_SPEC[MAX_RX_][MAX_TX_] = {
	{1486,1519,1525,1535,1552,1577,1603,1626,1655,1676,1696,1727,1742,1756,1766,1773,1779,1783,1786,1787,1756},
	{1490,1521,1526,1535,1554,1580,1604,1628,1657,1679,1699,1726,1741,1754,1763,1771,1777,1781,1783,1785,1754},
	{1492,1524,1530,1540,1558,1585,1614,1639,1669,1695,1717,1745,1764,1779,1792,1801,1809,1815,1819,1821,1791},
	{1492,1523,1529,1539,1557,1584,1609,1634,1663,1685,1706,1730,1745,1757,1767,1775,1780,1784,1786,1788,1758},
	{1491,1522,1528,1538,1557,1584,1610,1634,1664,1685,1707,1728,1745,1756,1767,1774,1779,1783,1786,1787,1757},
	{1490,1521,1526,1537,1557,1584,1609,1634,1664,1685,1707,1728,1743,1755,1765,1772,1777,1781,1784,1785,1755},
	{1488,1519,1526,1537,1557,1585,1609,1635,1665,1686,1709,1727,1744,1755,1765,1772,1777,1781,1783,1785,1755},
	{1494,1525,1531,1544,1565,1593,1617,1644,1673,1694,1718,1735,1751,1762,1772,1779,1784,1787,1790,1791,1761},
	{1492,1524,1530,1544,1566,1595,1618,1645,1674,1696,1719,1734,1751,1762,1772,1778,1784,1787,1789,1790,1760},
	{1486,1519,1526,1541,1564,1593,1616,1645,1673,1696,1718,1734,1749,1761,1770,1777,1781,1784,1787,1788,1757},
	{1484,1519,1526,1542,1565,1595,1617,1648,1675,1698,1720,1733,1751,1761,1771,1776,1782,1784,1787,1787,1757},
	{1482,1516,1525,1543,1568,1598,1623,1656,1686,1711,1736,1753,1770,1784,1796,1804,1811,1816,1819,1822,1791},
	{1477,1510,1523,1542,1567,1593,1619,1652,1676,1701,1722,1730,1749,1758,1768,1773,1778,1781,1783,1784,1753},
	{1473,1507,1519,1539,1566,1591,1619,1652,1677,1703,1723,1705,1742,1757,1766,1772,1777,1780,1782,1783,1751},
};

static const uint16_t SCR_ABS_LOWER_SPEC[MAX_RX_][MAX_TX_] = {
	{892,912,915,921,931,946,962,976,993,1006,1017,1036,1045,1053,1060,1064,1067,1070,1071,1072,1054},
	{894,912,916,921,932,948,963,977,994,1007,1020,1036,1045,1052,1058,1063,1066,1068,1070,1071,1052},
	{895,914,918,924,935,951,968,983,1002,1017,1030,1047,1058,1067,1075,1081,1085,1089,1091,1093,1075},
	{895,914,917,923,934,950,966,980,998,1011,1024,1038,1047,1054,1060,1065,1068,1070,1072,1073,1055},
	{894,913,917,923,934,951,966,980,998,1011,1024,1037,1047,1054,1060,1064,1068,1070,1071,1072,1054},
	{894,913,916,922,934,950,965,980,998,1011,1024,1037,1046,1053,1059,1063,1066,1069,1070,1071,1053},
	{893,912,916,922,934,951,965,981,999,1012,1025,1036,1046,1053,1059,1063,1066,1069,1070,1071,1053},
	{896,915,919,926,939,956,970,986,1004,1017,1031,1041,1050,1057,1063,1067,1070,1072,1074,1074,1056},
	{895,914,918,926,939,957,971,987,1004,1018,1031,1041,1051,1057,1063,1067,1070,1072,1074,1074,1056},
	{891,911,915,925,938,956,969,987,1004,1017,1031,1041,1049,1056,1062,1066,1069,1071,1072,1073,1054},
	{891,911,916,925,939,957,970,989,1005,1019,1032,1040,1050,1057,1062,1066,1069,1071,1072,1072,1054},
	{889,909,915,926,941,959,974,994,1012,1027,1042,1052,1062,1071,1077,1083,1087,1090,1092,1093,1075},
	{886,906,914,925,940,956,971,991,1005,1021,1033,1038,1049,1055,1061,1064,1067,1069,1070,1070,1052},
	{884,904,912,923,939,955,971,991,1006,1022,1034,1023,1045,1054,1060,1063,1066,1068,1069,1070,1051},
};

#else // defined(MELFAS_MMS136_4P3) 
#define MAX_RX_	12
#define MAX_TX_	20
static const uint16_t SCR_ABS_UPPER_SPEC[MAX_RX_][MAX_TX_] = {
	{1424,1494,1499,1506,1505,1504,1515,1529,1547,1556,1573,1587,1591,1599,1609,1616,1620,1620,1620,1523},
	{1425,1492,1498,1507,1507,1508,1520,1537,1556,1571,1586,1604,1612,1620,1632,1641,1644,1645,1647,1546},
	{1418,1486,1491,1499,1498,1497,1508,1522,1541,1550,1562,1577,1584,1588,1596,1603,1608,1610,1610,1515}, 
	{1425,1494,1499,1507,1504,1505,1517,1532,1550,1558,1569,1583,1593,1595,1602,1609,1615,1617,1618,1524}, 
	{1425,1494,1499,1506,1503,1505,1517,1532,1551,1558,1568,1583,1593,1595,1601,1608,1613,1616,1617,1524}, 
	{1425,1494,1499,1506,1503,1505,1516,1532,1550,1556,1565,1579,1592,1594,1598,1604,1610,1613,1614,1522}, 
	{1424,1494,1500,1506,1501,1506,1518,1534,1553,1556,1566,1580,1592,1594,1598,1604,1609,1612,1614,1523}, 
	{1432,1504,1509,1515,1509,1515,1528,1545,1563,1565,1575,1588,1602,1604,1607,1612,1618,1621,1623,1532}, 
	{1429,1503,1509,1513,1507,1515,1529,1546,1563,1565,1574,1588,1601,1604,1606,1612,1617,1620,1622,1532}, 
	{1417,1496,1501,1502,1497,1507,1521,1539,1554,1558,1566,1579,1593,1595,1598,1603,1608,1611,1613,1522}, 
	{1422,1502,1508,1507,1508,1520,1537,1558,1573,1582,1590,1607,1623,1628,1634,1642,1649,1650,1649,1548}, 
	{1407,1491,1495,1492,1493,1505,1520,1539,1546,1558,1552,1571,1586,1588,1592,1597,1603,1605,1606,1513}, 
};

static const uint16_t SCR_ABS_LOWER_SPEC[MAX_RX_][MAX_TX_] = {
	{854,896,899,904,903,903,909,917,928,934,944,952,955,960,965,969,972,972,972,914},
	{855,895,899,904,904,905,912,922,933,943,952,962,967,972,979,985,987,987,988,927},
	{851,892,895,899,899,898,905,913,924,930,937,946,951,953,957,962,965,966,966,909},
	{855,896,899,904,903,903,910,919,930,935,941,950,956,957,961,965,969,970,971,914},
	{855,896,899,904,902,903,910,919,931,935,941,950,956,957,961,965,968,970,970,914},
	{855,896,899,904,902,903,910,919,930,934,939,947,955,956,959,962,966,968,968,913},
	{855,897,900,903,901,903,911,920,932,933,939,948,955,956,959,962,966,967,968,914},
	{859,902,905,909,905,909,917,927,938,939,945,953,961,963,964,967,971,973,974,919},
	{857,902,905,908,904,909,917,928,938,939,945,953,961,962,964,967,970,972,973,919},
	{850,897,900,901,898,904,913,924,932,935,940,947,956,957,959,962,965,967,968,913},
	{853,901,905,904,905,912,922,935,944,949,954,964,974,977,980,985,990,990,989,929},
	{844,895,897,895,896,903,912,924,927,935,931,942,952,953,955,958,962,963,964,908},
};
#endif

static int g_exciting_ch, g_sensing_ch;
static unsigned char is_inputmethod;
#ifdef TSP_BOOST
static unsigned char is_boost;
#endif
#ifdef TUNNING_SUPPORT
enum {
	CMD_GET_PRE_CRC		= 0x02,
	CMD_ENTER_TEST_MODE	= 0x40,
	CMD_CALC_CRC		= 0x4E,
	MMS_UNIV_CMD 		= 0xA0,
	MMS_UNIV_RESULT_SZ	= 0xAE,
	MMS_UNIV_RESULT		= 0xAF,
};

enum {
	GET_RX_NUM = 1,
	GET_TX_NUM,
	GET_EVENT_DATA,
};

enum {
	LOG_TYPE_U08 = 2,
	LOG_TYPE_S08,
	LOG_TYPE_U16,
	LOG_TYPE_S16,
	LOG_TYPE_U32 = 8,
	LOG_TYPE_S32,
};

struct mms_log_data {
	__u8				*data;
	int				cmd;
};
#endif
enum {
	None = 0,
	TOUCH_SCREEN,
	TOUCH_KEY
};
struct muti_touch_info {
	int strength;
	int width;
	int posX;
	int posY;
};
struct melfas_ts_data {
	uint16_t addr;
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct melfas_tsi_platform_data *pdata;
	struct work_struct  work;
	struct melfas_version *version;
	uint32_t flags;
	int (*power)(int on);
	int (*gpio)(void);
	const u8			*config_fw_version;
#ifdef TUNNING_SUPPORT	
	struct class			*class;
	struct mms_log_data		*log;
	struct cdev			    cdev;
	u8			        	tx_num;
	u8			         	rx_num;	
#endif
	int				irq;
#ifdef TA_DETECTION
	void (*register_cb)(void *);
	void (*read_ta_status)(void *);
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
#if defined(TSP_BOOSTER)
			struct delayed_work work_dvfs_off;
			struct delayed_work work_dvfs_chg;
			bool	dvfs_lock_status;
			struct mutex dvfs_lock;
#endif
	struct mutex			lock;
	bool				enabled;
	u8				fw_ic_ver;
#if defined(TA_SET_NOISE_MODE)
	bool 			noise_mode;
	bool			ta_status;		
#endif	
#if defined(SEC_TSP_FACTORY_TEST)
	struct list_head			cmd_list_head;
	u8			cmd_state;
	char			cmd[TSP_CMD_STR_LEN];
	int			cmd_param[TSP_CMD_PARAM_NUM];
	char			cmd_result[TSP_CMD_RESULT_STR_LEN];
	struct mutex			cmd_lock;
	bool			cmd_is_running;
	unsigned int reference[NODE_NUM];
	unsigned int raw[NODE_NUM]; /* CM_ABS */
	unsigned int inspection[NODE_NUM];/* CM_DELTA */
	unsigned int intensity[NODE_NUM];
	bool ft_flag;
#endif /* SEC_TSP_FACTORY_TEST */
};
static struct melfas_ts_data *ts;
#ifdef SEC_TSP
extern struct class *sec_class;
struct device *sec_touchscreen_dev;
struct device *sec_touchkey_dev;
int menu_pressed;
int back_pressed;

static int melfas_i2c_master_send(struct i2c_client *client, char *buff, int count);
static int melfas_i2c_master_recv(struct i2c_client *client, char *buff, int count);
static int melfas_ts_suspend(struct i2c_client *client, pm_message_t mesg);
static int melfas_ts_resume(struct i2c_client *client);
static void release_all_fingers(struct melfas_ts_data *ts);
#ifdef TSP_BOOST
static int melfas_set_config(struct i2c_client *client, u8 reg, u8 value);
#endif
static int melfas_i2c_write(struct i2c_client *client, char *buf, int length);
static void TSP_reboot(void);
extern void ts_power_control(int en);
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void melfas_ts_early_suspend(struct early_suspend *h);
static void melfas_ts_late_resume(struct early_suspend *h);
#endif
#ifdef TUNNING_SUPPORT	
//static void mms_pwr_on_reset(struct melfas_ts_data *ts);
static void mms_report_input_data(struct melfas_ts_data *ts, u8 sz, u8 *buf);

static void mms_clear_input_data(struct melfas_ts_data *ts)
{
	int i;

	for (i = 0; i< MAX_FINGER_NUM; i++) {
		input_mt_slot(ts->input_dev, i);
		input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);
	}
	input_sync(ts->input_dev);

	return;
}

static int mms_fs_open(struct inode *node, struct file *fp)
{
	struct melfas_ts_data *ts;
	struct i2c_client *client;
	struct i2c_msg msg;
	u8 buf[3] = { 
		MMS_UNIVERSAL_CMD, 
		MMS_CMD_SET_LOG_MODE, 
		true, 
	};

	ts = container_of(node->i_cdev, struct melfas_ts_data, cdev);
	client = ts->client;

	disable_irq(ts->irq);

	fp->private_data = ts;

	msg.addr = client->addr;
	msg.flags = 0;
	msg.buf = buf;
	msg.len = sizeof(buf);

	i2c_transfer(client->adapter, &msg, 1);

	ts->log->data = kzalloc(MAX_LOG_LENGTH * 20 + 5, GFP_KERNEL);

	mms_clear_input_data(ts);

	return 0;
}

static int mms_fs_release(struct inode *node, struct file *fp)
{
	struct melfas_ts_data *ts = fp->private_data;

	mms_clear_input_data(ts);
	TSP_reboot();

	kfree(ts->log->data);
	enable_irq(ts->irq);

	return 0;
}

static void mms_event_handler(struct melfas_ts_data *ts)
{
	struct i2c_client *client = ts->client;
	int sz;
	int ret;
	int row_num;
	u8 reg = MMS_INPUT_EVENT;
	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.buf = &reg,
			.len = 1,
		},
		{
			.addr = client->addr,
			.flags = 1,
			.buf = ts->log->data,
		},
	};
	struct mms_log_pkt {
		u8 marker;
		u8 log_info;
		u8 code;
		u8 element_sz;
		u8 row_sz;
	} __attribute__((packed)) *pkt = (struct mms_log_pkt *)ts->log->data;;

	memset(pkt, 0, sizeof(struct mms_log_pkt));
/*
	if (gpio_get_value(ts->pdata->gpio_resetb)) {
		return;
	}
*/
	sz = i2c_smbus_read_byte_data(client, MMS_EVENT_PKT_SZ);
	msg[1].len = sz;
	ret = i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg));
	if (ret != ARRAY_SIZE(msg)) {
		dev_err(&client->dev,
			"failed to read %d bytes of touch data (%d)\n",
			sz, ret);
		return;
	}

	if ((pkt->marker & 0xf) == MMS_LOG_EVENT) {
		if ((pkt->log_info & 0x7) == 0x1) {
			pkt->element_sz = 0;
			pkt->row_sz = 0;

			return;
		}

		switch (pkt->log_info >> 4) {
		case LOG_TYPE_U08:
		case LOG_TYPE_S08:
			msg[1].len = pkt->element_sz;
			break;
		case LOG_TYPE_U16:
		case LOG_TYPE_S16:
			msg[1].len = pkt->element_sz * 2;
			break;
		case LOG_TYPE_U32:
		case LOG_TYPE_S32:
			msg[1].len = pkt->element_sz * 4;
			break;
		default:
			dev_err(&client->dev, "invalid log type\n");
			break;
		}

		msg[1].buf = ts->log->data + sizeof(struct mms_log_pkt);
		reg = MMS_UNIVERSAL_RESULT;
		row_num = pkt->row_sz ? pkt->row_sz : 1;

		while (row_num--) {
//			while (gpio_get_value(ts->pdata->gpio_resetb))
	//			;

			ret = i2c_transfer(client->adapter, msg, 2);
			msg[1].buf += msg[1].len;
		}; 

	} else {
		mms_report_input_data(ts, sz, ts->log->data);
		memset(pkt, 0, sizeof(struct mms_log_pkt));
	}


	return;

}

static ssize_t mms_fs_read(struct file *fp, char *rbuf, size_t cnt, loff_t *fpos)
{
	struct melfas_ts_data *ts = fp->private_data;
	struct i2c_client *client = ts->client;
	int ret = 0;

	switch (ts->log->cmd) {
	case GET_RX_NUM:
		ret = copy_to_user(rbuf, &ts->rx_num, 1);
		break;

	case GET_TX_NUM:
		ret = copy_to_user(rbuf, &ts->tx_num, 1);
		break;

	case GET_EVENT_DATA:
		mms_event_handler(ts);
		/* send event without log marker */
		ret = copy_to_user(rbuf, ts->log->data + 1, cnt);
		break;

	default:
		dev_err(&client->dev, "unknown command\n");
		ret = -EFAULT;
		goto out;
	}

out:
	return ret; 
}

static ssize_t mms_fs_write(struct file *fp, const char *wbuf, size_t cnt, loff_t *fpos)
{
	struct melfas_ts_data *ts = fp->private_data;
	struct i2c_client *client = ts->client;
	struct i2c_msg msg;
	int ret = 0;
	u8 *buf;

	mutex_lock(&ts->lock);

	buf = kzalloc(cnt + 1, GFP_KERNEL); 

	if ((buf == NULL) || copy_from_user(buf, wbuf, cnt)) {
		dev_err(&client->dev, "failed to read data from user\n");
		ret = -EIO;
		goto out;
	}

	if (cnt == 1) {
		ts->log->cmd = *buf;
	} else {
		msg.addr = client->addr;
		msg.flags = 0;
		msg.buf = buf;
		msg.len = cnt;

		if (i2c_transfer(client->adapter, &msg, 1) != 1) {
			dev_err(&client->dev, "failt to transfer command\n");
			ret = -EIO;
			goto out;
		}
	}

	ret = 0;

out:
	kfree(buf);
	mutex_unlock(&ts->lock);

	return ret;
}

static struct file_operations mms_fops = {
	.owner 				= THIS_MODULE,
	.open 				= mms_fs_open,
	.release 			= mms_fs_release,
	.read 				= mms_fs_read,
	.write 				= mms_fs_write,
};
#endif
#if defined(SEC_TSP_FACTORY_TEST)
#define TSP_CMD(name, func) .cmd_name = name, .cmd_func = func
struct tsp_cmd {
	struct list_head	list;
	const char	*cmd_name;
	void	(*cmd_func)(void *device_data);
};
static void fw_update(void *device_data);
static void get_fw_ver_bin(void *device_data);
static void get_fw_ver_ic(void *device_data);
static void get_config_ver(void *device_data);
static void get_threshold(void *device_data);
static void module_off_master(void *device_data);
static void module_on_master(void *device_data);
static void get_chip_vendor(void *device_data);
static void get_chip_name(void *device_data);
static void get_reference(void *device_data);
static void get_cm_abs(void *device_data);
static void get_cm_delta(void *device_data);
static void get_intensity(void *device_data);
static void get_x_num(void *device_data);
static void get_y_num(void *device_data);
static void run_reference_read(void *device_data);
static void run_cm_abs_read(void *device_data);
static void run_cm_delta_read(void *device_data);
static void run_intensity_read(void *device_data);
static void not_support_cmd(void *device_data);
static int check_delta_value(struct melfas_ts_data *ts);
struct tsp_cmd tsp_cmds[] = {
	{TSP_CMD("fw_update", fw_update),},
	{TSP_CMD("get_fw_ver_bin", get_fw_ver_bin),},
	{TSP_CMD("get_fw_ver_ic", get_fw_ver_ic),},
	{TSP_CMD("get_config_ver", get_config_ver),},
	{TSP_CMD("get_threshold", get_threshold),},
	{TSP_CMD("module_off_master", module_off_master),},
	{TSP_CMD("module_on_master", module_on_master),},
	{TSP_CMD("module_off_slave", not_support_cmd),},
	{TSP_CMD("module_on_slave", not_support_cmd),},
	{TSP_CMD("get_chip_vendor", get_chip_vendor),},
	{TSP_CMD("get_chip_name", get_chip_name),},
	{TSP_CMD("get_x_num", get_x_num),},
	{TSP_CMD("get_y_num", get_y_num),},
	{TSP_CMD("get_reference", get_reference),},
	{TSP_CMD("get_cm_abs", get_cm_abs),},
	{TSP_CMD("get_cm_delta", get_cm_delta),},
	{TSP_CMD("get_intensity", get_intensity),},
	{TSP_CMD("run_reference_read", run_reference_read),},
	{TSP_CMD("run_cm_abs_read", run_cm_abs_read),},
	{TSP_CMD("run_cm_delta_read", run_cm_delta_read),},
	{TSP_CMD("run_intensity_read", run_intensity_read),},
	{TSP_CMD("not_support_cmd", not_support_cmd),},
};
#endif

#if defined(TSP_BOOSTER)
static void change_dvfs_lock(struct work_struct *work)
{
	struct melfas_ts_data *ts = container_of(work,struct melfas_ts_data, work_dvfs_chg.work);
	int ret;
	mutex_lock(&ts->dvfs_lock);
#if defined(CONFIG_MACH_ARUBASLIM_OPEN)
	ret = set_freq_limit(DVFS_TOUCH_ID, 1209600);
#else
	ret = set_freq_limit(DVFS_TOUCH_ID, 1008000);
#endif
	mutex_unlock(&ts->dvfs_lock);

	if (ret < 0)
		printk(KERN_ERR "%s: 1booster stop failed(%d)\n",\
					__func__, __LINE__);
	else
		printk(KERN_INFO "[TSP] %s", __func__);
}

static void set_dvfs_off(struct work_struct *work)
{
	struct melfas_ts_data *ts = container_of(work,struct melfas_ts_data, work_dvfs_off.work);
	mutex_lock(&ts->dvfs_lock);
	set_freq_limit(DVFS_TOUCH_ID, -1);
	ts->dvfs_lock_status = false;
	mutex_unlock(&ts->dvfs_lock);

	printk(KERN_INFO "[TSP] DVFS Off!");
}

static void set_dvfs_lock(struct melfas_ts_data *ts, uint32_t on)
{
	int ret = 0;

	mutex_lock(&ts->dvfs_lock);
	if (on == 0) {
		if (ts->dvfs_lock_status) {
			schedule_delayed_work(&ts->work_dvfs_off,msecs_to_jiffies(TOUCH_BOOSTER_OFF_TIME));
			printk(KERN_INFO "[TSP] DVFS_touch_release");
		}
	} else if (on == 1) {
		cancel_delayed_work(&ts->work_dvfs_off);
		if (!ts->dvfs_lock_status) {
#if defined(CONFIG_MACH_ARUBASLIM_OPEN) 
			ret = set_freq_limit(DVFS_TOUCH_ID, 1209600);
#else
			ret = set_freq_limit(DVFS_TOUCH_ID, 1008000);
#endif			
			if (ret < 0)
				printk(KERN_ERR "%s: cpu lock failed(%d)\n",__func__, ret);

			ts->dvfs_lock_status = true;
			printk(KERN_INFO "[TSP] DVFS On!");
		}
	} else if (on == 2) {
		cancel_delayed_work(&ts->work_dvfs_off);
		schedule_work(&ts->work_dvfs_off.work);
		printk(KERN_INFO "[TSP] DVFS_touch_all_finger_release");
	}
	mutex_unlock(&ts->dvfs_lock);
}
#endif

int melfas_fw_i2c_read(u16 addr, u8 *value, u16 length);
int melfas_fw_i2c_write(char *buf, int length);
#ifdef LOW_LEVEL_CHECK
static ssize_t check_init_lowleveldata(void);
#endif
static struct muti_touch_info g_Mtouch_info[MELFAS_MAX_TOUCH];
#define VREG_ENABLE		1
#define VREG_DISABLE	0
#define TOUCH_ON  1
#define TOUCH_OFF 0
//static struct regulator *touch_regulator;
static void ts_power_enable(int en)
{
	printk(KERN_ERR "%s %s\n", __func__, (en) ? "on" : "off");
#if (defined(CONFIG_MACH_ARUBA_OPEN) && !defined(CONFIG_MACH_KYLEPLUS_OPEN)) || defined(CONFIG_MACH_ARUBASLIM_OPEN)\
	||  defined(CONFIG_MACH_NEVIS3G) || defined(CONFIG_MACH_DELOS_OPEN) ||defined(CONFIG_MACH_ARUBASLIMQ_OPEN_CHN)
	
	gpio_request(TSP_PWR_LDO_GPIO, "tsp-power");
	gpio_tlmm_config(GPIO_CFG(TSP_PWR_LDO_GPIO, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE);	
	if (en) {
		gpio_direction_output(TSP_PWR_LDO_GPIO, 1);
	} else {
		gpio_direction_output(TSP_PWR_LDO_GPIO, 0);
	}
	gpio_free(TSP_PWR_LDO_GPIO);
	
#else	
	int rc;
	struct vreg *vreg_lcd = NULL;

	printk("start %s\n", __func__);
	if (vreg_lcd == NULL) {

		vreg_lcd = vreg_get(NULL, "vtsp");
		if (IS_ERR(vreg_lcd)) {
			printk(KERN_ERR "%s: vreg_get(%s) failed (%ld)\n", __func__, "vtsp", PTR_ERR(vreg_lcd));
			return;
		}

		rc = vreg_set_level(vreg_lcd, 3000);
		if (rc) {
			printk(KERN_ERR "%s: TSP set_level failed (%d)\n", __func__, rc);
		}
	}

	if (en) {
		rc = vreg_enable(vreg_lcd);
		if (rc) {
			printk(KERN_ERR "%s: TSP enable failed (%d)\n", __func__, rc);
			msleep(200);
			rc = vreg_enable(vreg_lcd);
			printk(KERN_ERR "%s: TSP enable failed 2 (%d)\n", __func__, rc);
		} else {
			printk(KERN_ERR "%s: TSP enable success (%d)\n", __func__, rc);
		}
	} else {
		rc = vreg_disable(vreg_lcd);
		if (rc) {
			printk(KERN_ERR "%s: TSP disable failed (%d)\n", __func__, rc);
			msleep(200);
			rc = vreg_disable(vreg_lcd);
			printk(KERN_ERR "%s: TSP disable failed 2 (%d)\n", __func__, rc);
		} else {
			printk(KERN_ERR "%s: TSP disable success (%d)\n", __func__, rc);
		}
	}
#endif
	
}

void ts_power_control(int en)
{
	ts_power_enable(en);
}
EXPORT_SYMBOL(ts_power_control);

#if defined(TA_SET_NOISE_MODE)
static int tsp_init_for_ta;
static int retry_ta_noti;

static void mms_set_noise_mode(struct melfas_ts_data *ts)
{
	//struct i2c_client *client = ts->client;
	u8 setLowLevelData[2];
	printk(KERN_ERR "mms_set_noise_mode   ts_status = %d, %d\n",ts->ta_status, ts->noise_mode);
	if (!ts->noise_mode)
		return;

#if defined(MELFAS_MMS134S) || defined(MELFAS_MMS128S) || defined(CONFIG_MACH_ARUBASLIM_OPEN)
		setLowLevelData[0] = 0x30;
#else
	setLowLevelData[0] = 0x33;
#endif

	if (ts->ta_status) {
		setLowLevelData[1] = 0x01;
	} else {
		setLowLevelData[1] = 0x02;
		ts->noise_mode = 0;
	}
	printk(KERN_INFO "TA connect : %d!!\n", ts->ta_status);
	melfas_i2c_write(ts->client, setLowLevelData, 2);
}

void touch_ta_noti(int en)
{
	printk(KERN_ERR "touch_ta_noti; enable = %d\n",en);

	if(tsp_init_for_ta == 2){			// 2 = init end
		if (ts == NULL)
			return;
		if (en == 0)
			ts->ta_status = 0;
		else
			ts->ta_status = 1;
	
		mms_set_noise_mode(ts);
	}else{
		printk(KERN_ERR"[TSP] noti not yet %d,%d. \n", en, tsp_init_for_ta);
		retry_ta_noti = en;
	}	
}
EXPORT_SYMBOL(touch_ta_noti);

#endif  
#ifdef PANEL_INIT
static int melfas_init_panel(struct melfas_ts_data *ts)
{
	char buf = 0x00;
	int ret;
	melfas_i2c_master_send(ts->client, &buf, 1);
	ret = melfas_i2c_master_send(ts->client, &buf, 1);
	if (ret < 0) {
		printk(KERN_ERR "%s : i2c_master_send() failed\n [%d]", __func__, ret);
		return 0;
	}
	return true;
}
#endif
#ifdef TA_DETECTION
static void tsp_ta_probe(int ta_status)
{
	u8 write_buffer[3];
	printk(KERN_ERR"[TSP] %s : TA is %s. \n", __func__, ta_status ? "on" : "off");
	if (tsp_enabled == false) {
		printk(KERN_ERR"[TSP] tsp_enabled is 0\n");
		return;
	}
	write_buffer[0] = 0xB0;
	write_buffer[1] = 0x11;
	if (ta_status)
		write_buffer[2] = 1;
	else
		write_buffer[2] = 0;
	melfas_i2c_write(ts_data->client, (char *)write_buffer, 3);
}
#endif
#ifdef TSP_BOOST
static void TSP_boost(struct melfas_ts_data *ts, bool onoff)
{
	printk(KERN_ERR "[TSP] TSP_boost %s\n", is_boost ? "ON" : "Off");
	if (onoff) {
		melfas_set_config(ts->client, MIP_POSITION_FILTER_LEVEL, 2);
	} else {
		melfas_set_config(ts->client, MIP_POSITION_FILTER_LEVEL, 80);
	}
}
#endif
#ifdef TSP_PATTERN_TRACTKING
/* To do forced calibration when ghost touch occured at the same point
    for several second. */
#define MAX_GHOSTCHECK_FINGER				10
#define MAX_GHOSTTOUCH_COUNT					300
#define MAX_GHOSTTOUCH_BY_PATTERNTRACKING	5
static int tcount_finger[MAX_GHOSTCHECK_FINGER] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static int touchbx[MAX_GHOSTCHECK_FINGER] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static int touchby[MAX_GHOSTCHECK_FINGER] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static int ghosttouchcount;
static int cFailbyPattenTracking;
static void clear_tcount(void)
{
	int i;
	for (i = 0; i < MAX_GHOSTCHECK_FINGER; i++) {
		tcount_finger[i] = 0;
		touchbx[i] = 0;
		touchby[i] = 0;
	}
}
static int diff_two_point(int x, int y, int oldx, int oldy)
{
	int diffx, diffy;
	int distance;
	diffx = x-oldx;
	diffy = y-oldy;
	distance = abs(diffx) + abs(diffy);
	if (distance < 3)
		return 1;
	else
		return 0;
}
static int tsp_pattern_tracking(struct melfas_ts_data *ts, int fingerindex, int x, int y)
{
	int i;
	int ghosttouch = 0;
	if (i == fingerindex) {
		if (diff_two_point (x, y, touchbx[i], touchby[i])) {
			tcount_finger[i] = tcount_finger[i]+1;
		} else {
			tcount_finger[i] = 0;
		}
		touchbx[i] = x;
		touchby[i] = y;
		if (tcount_finger[i] > MAX_GHOSTTOUCH_COUNT) {
			ghosttouch = 1;
			ghosttouchcount++;
			printk(KERN_DEBUG "[TSP] SUNFLOWER (PATTERN TRACKING) %d\n", ghosttouchcount);
			clear_tcount();
			cFailbyPattenTracking++;
			if (cFailbyPattenTracking > MAX_GHOSTTOUCH_BY_PATTERNTRACKING) {
				cFailbyPattenTracking = 0;
				printk(KERN_INFO "[TSP] Reboot.\n");
				TSP_reboot();
			} else {
				/* Do something for calibration */
			}
		}
	}
	return ghosttouch;
}
#endif
static void melfas_ts_get_data(struct work_struct *work)
{
	struct melfas_ts_data *ts = container_of(work, struct melfas_ts_data, work);
	int ret = 0, i, j;
	uint8_t buf[TS_READ_REGS_LEN];
#if defined(CONFIG_MACH_ARUBASLIM_OPEN)	
	uint8_t buf1[TS_READ_REGS_LEN];
#endif	
	int read_num, FingerID;
	int _touch_is_pressed, line;
	int keyID = 0, touchType = 0, touchState = 0;
	if (tsp_enabled == false) {
		printk(KERN_ERR "[TSP ]%s. tsp_disabled.\n", __func__);
		msleep(500);
		return;
	}
#if DEBUG_PRINT
	// printk(KERN_ERR "%s start\n", __func__);
	if (ts == NULL)
			printk(KERN_ERR "%s : TS NULL\n", __func__);
#endif
	for (j = 0; j < 3; j++) {
		buf[0] = TS_READ_START_ADDR;
		ret = melfas_i2c_master_send(ts->client, buf, 1);
		if (ret < 0) {
			line = __LINE__;
			goto tsp_error;
		}
		ret = melfas_i2c_master_recv(ts->client, buf, 1);
		if (ret < 0) {
			line = __LINE__;
			goto tsp_error;
		}
		read_num = buf[0];
		if (read_num < 60)
			break;
	}
	if (read_num > TS_READ_REGS_LEN)
		read_num = TS_READ_REGS_LEN;

	if (read_num > 0) {
		buf[0] = TS_READ_START_ADDR2;
		ret = melfas_i2c_master_send(ts->client, buf, 1);
		if (ret < 0) {
			line = __LINE__;
			goto tsp_error;
		}
		ret = melfas_i2c_master_recv(ts->client, buf, read_num);
		if (ret < 0) {
			line = __LINE__;
			goto tsp_error;
		}
#if defined(CONFIG_MACH_ARUBASLIM_OPEN)		
		buf1[0] = 0x11;
		ret = melfas_i2c_master_send(ts->client, buf1, 1);
		ret = melfas_i2c_master_recv(ts->client, buf1, read_num);
#endif	
#if defined(TA_SET_NOISE_MODE)
#if defined(CONFIG_MACH_ARUBASLIM_OPEN)
		if (buf[0] == 0x0E && buf1[0] == 0x01) {
#else
		if (buf[0] == 0x0E) {
#endif		
			u8 setLowLevelData[2];
			printk(KERN_ERR "[TSP] noise mode enter!!\n");
			ts->noise_mode = 1 ;
#if defined(CONFIG_MACH_ARUBASLIM_OPEN)
			// not assigned
#else
			setLowLevelData[0] = 0x10;
			setLowLevelData[1] = 0x00;
			ret = melfas_i2c_write(ts->client, setLowLevelData, 2);
#endif
			mms_set_noise_mode(ts);
		}
#endif
		for (i = 0; i < read_num; i = i + 6) {
		if (buf[i] == 0x0F) {
			printk(KERN_ERR "%s : ESD-DETECTED!!!\n", __func__);
			line = __LINE__;
			goto tsp_error;
		}
				touchType = (buf[i] >> 5) & 0x03;
				if (touchType == TOUCH_SCREEN) {
					FingerID = (buf[i] & 0x0F) - 1;
					if(FingerID < MELFAS_MAX_TOUCH){
					g_Mtouch_info[FingerID].posX = (uint16_t)(buf[i+1] & 0x0F) << 8 | buf[i+2];
					g_Mtouch_info[FingerID].posY = (uint16_t)(buf[i+1] & 0xF0) << 4 | buf[i+3];
					if ((buf[i] & 0x80) == 0)
						g_Mtouch_info[FingerID].strength = 0;
					else
						g_Mtouch_info[FingerID].strength = buf[i+4];
					g_Mtouch_info[FingerID].width = buf[i+5];
					}else
					{
						printk(KERN_ERR "%s : Finger Over Max:%d, Id:%d!!!\n", __func__,MELFAS_MAX_TOUCH, FingerID );
					}
					}
				else if (touchType == TOUCH_KEY) {
					keyID = (buf[i] & 0x0F);
					touchState = (buf[i] & 0x80);
					gMenuKey_Intensity = 0;
					gBackKey_Intensity = 0;
			printk(KERN_ERR "[TSP] keyID : %d, touchstate : %d\n"
					, keyID, touchState);
					if (keyID == 0x1) {
						if (touchState){
							menu_pressed = 1;
							before_gMenuKey_Intensity = buf[i + 5];
							gMenuKey_show=0;
						}
						else
							menu_pressed = 0;
						gMenuKey_Intensity = buf[i + 5];
						gMenuKey_state = menu_pressed;
						input_report_key(
						ts->input_dev, KEY_MENU,
						touchState ?
						PRESS_KEY : RELEASE_KEY);
					}
					if (keyID == 0x2) {
						if (touchState){
							back_pressed = 1;
							before_gBackKey_Intensity = buf[i + 5];
							gBackKey_show=0;
						}
						else
							back_pressed = 0;
						gBackKey_Intensity = buf[i + 5];
						gBackKey_state = back_pressed;
						input_report_key(
						ts->input_dev, KEY_BACK,
						touchState ?
						PRESS_KEY : RELEASE_KEY);
					}
				}
		}
	}
	_touch_is_pressed = 0;
	for (i = 0; i < MELFAS_MAX_TOUCH; i++) {
		if (g_Mtouch_info[i].strength == -1)
			continue;
#ifdef TSP_PATTERN_TRACTKING
		tsp_pattern_tracking(ts, i,  g_Mtouch_info[i].posX,  g_Mtouch_info[i].posY);
#endif
		if (g_Mtouch_info[i].strength) {
			input_mt_slot(ts->input_dev, i);
			input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, true);
//			input_report_abs(ts->input_dev, ABS_MT_PRESSURE, g_Mtouch_info[i].strength);
			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, g_Mtouch_info[i].strength);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_X, g_Mtouch_info[i].posX);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, g_Mtouch_info[i].posY);
			input_report_key(ts->input_dev, BTN_TOUCH, g_Mtouch_info[i].strength);
		} else {
			input_mt_slot(ts->input_dev, i);
			input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);
		}
#if defined(CONFIG_MACH_ARUBASLIM_OPEN)
		printk(KERN_ERR "[TSP] ID: %d, State : %d\n",	i, (g_Mtouch_info[i].strength > 0));
#else
		printk(KERN_ERR "[TSP] ID: %d, State : %d, x: %d, y: %d, z: %d w: %d\n",
		i, (g_Mtouch_info[i].strength > 0),
		g_Mtouch_info[i].posX, g_Mtouch_info[i].posY,
		g_Mtouch_info[i].strength, g_Mtouch_info[i].width);
#endif

		if (g_Mtouch_info[i].strength == 0)
			g_Mtouch_info[i].strength = -1;
		if (g_Mtouch_info[i].strength > 0)
			_touch_is_pressed = 1;
	}
	input_sync(ts->input_dev);
	touch_is_pressed = _touch_is_pressed;
	return ;
tsp_error:
	printk(KERN_ERR "[TSP] %s: i2c failed(%d)\n", __func__, line);
	TSP_reboot();
}
static irqreturn_t melfas_ts_irq_handler(int irq, void *handle)
{
	struct melfas_ts_data *ts = (struct melfas_ts_data *)handle;
#ifdef TUNNING_SUPPORT	
	struct i2c_client *client = ts->client;
	u8 buf[MAX_FINGER_NUM * FINGER_EVENT_SZ] = { 0 };
	int ret;
	int sz;
	u8 reg = MMS_INPUT_EVENT;
	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.buf = &reg,
			.len = 1,
		},
		{
			.addr = client->addr,
			.flags = 1,
			.buf = buf,
		},
	};

	sz = i2c_smbus_read_byte_data(client, MMS_EVENT_PKT_SZ);
	msg[1].len = sz;
	ret = i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg));
	if (ret != ARRAY_SIZE(msg)) {
		dev_err(&client->dev,
			"failed to read %d bytes of touch data (%d)\n",
			sz, ret);
	}


	mms_report_input_data(ts, sz, buf);
#endif
#if DEBUG_PRINT
	//printk(KERN_ERR "melfas_ts_irq_handler\n");
#endif
	melfas_ts_get_data(&ts->work);
#if defined(TSP_BOOSTER)
	set_dvfs_lock(ts, !!touch_is_pressed);
#endif
	return IRQ_HANDLED;
}
#ifdef SEC_TSP
#define I2C_FAIL_RETRY_CNT	3
static int melfas_i2c_master_send(struct i2c_client *client, char *buff, int count)
{
	int ret, i;
	int retry = I2C_FAIL_RETRY_CNT;
	
	for (i = 0; i < retry; i++) {
		ret = i2c_master_send(client, buff, count);
		if (ret >= 0)
			break;
	}
	if (i >= retry) {
		printk(KERN_ERR "[TSP] Error code : %d, %d\n", __LINE__, ret);
	}


	return ret;
}

static int melfas_i2c_master_recv(struct i2c_client *client, char *buff, int count)
{
	int ret, i;
	int retry = I2C_FAIL_RETRY_CNT;
	
	for (i = 0; i < retry; i++) {
		ret = i2c_master_recv(client, buff, count);
		if (ret >= 0)
			break;
	}
	if (i >= retry) {
		printk(KERN_ERR "[TSP] Error code : %d, %d\n", __LINE__, ret);
	}

	return ret;
}
	
static int melfas_i2c_read(struct i2c_client *client, u16 addr, u16 length, u8 *value)
{
	struct i2c_adapter *adapter = client->adapter;
	struct i2c_msg msg[2];
	msg[0].addr  = client->addr;
	msg[0].flags = 0x00;
	msg[0].len   = 1;
	msg[0].buf   = (u8 *) &addr;
	msg[1].addr  = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len   = length;
	msg[1].buf   = (u8 *) value;
	if  (i2c_transfer(adapter, msg, 2) == 2)
		return 0;
	else
		return -EIO;
}
static int melfas_i2c_read_without_addr(struct i2c_client *client,
	u16 length, u8 *value)
{
	struct i2c_adapter *adapter = client->adapter;
	struct i2c_msg msg[1];
	msg[0].addr  = client->addr;
	msg[0].flags = I2C_M_RD;
	msg[0].len   = length;
	msg[0].buf   = (u8 *) value;
	if  (i2c_transfer(adapter, msg, 1) == 1)
		return 0;
	else
		return -EIO;
}
static int melfas_i2c_busrt_write(struct i2c_client *client, u16 length,
	u8 *value)
{
	struct i2c_adapter *adapter = client->adapter;
	struct i2c_msg msg[1];
	msg[0].addr  = client->addr;
	msg[0].flags = 0;
	msg[0].len   = length;
	msg[0].buf   = (u8 *) value;
	if  (i2c_transfer(adapter, msg, 1) == 1)
		return 0;
	else
		return -EIO;
}

static int melfas_i2c_write(struct i2c_client *client, char *buf, int length)
{
	int i;
	char data[TS_WRITE_REGS_LEN];
	if (length > TS_WRITE_REGS_LEN) {
		pr_err("[TSP] size error - %s\n", __func__);
		return -EINVAL;
	}
	for (i = 0; i < length; i++)
		data[i] = *buf++;
	i = melfas_i2c_master_send(client, (char *)data, length);
	if (i == length)
		return length;
	else{
		printk(KERN_ERR "%s (line_%d) length error\n ", __func__, __LINE__);
		return -EIO;
	}
}
int melfas_fw_i2c_read(u16 addr, u8 *value, u16 length)
{
	if (melfas_i2c_read(ts->client, addr, length, value) == 0)
		return 1;
	else
		return 0;
}
int melfas_fw_i2c_write(char *buf, int length)
{
	int ret;
	ret = melfas_i2c_write(ts->client, buf, length);
	printk(KERN_ERR "<MELFAS> mass erase start write ret%d\n\n", ret);
	if (ret > 0)
		return 1;
	else
		return 0;
}
int melfas_fw_i2c_read_without_addr(u8 *value, u16 length)
{
	if (melfas_i2c_read_without_addr(ts->client, length, value) == 0)
		return 1;
	else
		return 0;
}
int melfas_fw_i2c_busrt_write(u8 *value, u16 length)
{
	if (melfas_i2c_busrt_write(ts->client, length, value) == 0)
		return 1;
	else
		return 0;
}
#if 0
static ssize_t set_tsp_firm_version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf (buf, sizeof(buf), "%#02x, %#02x, %#02x\n", ts->version->core, ts->version->private, ts->version->public);
}
static ssize_t set_tsp_firm_version_read_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	u8 fw_latest_version, privatecustom_version, publiccustom_version;
	int ret;
	char buff[6] = {0,};
	buff[0] = MIP_TSP_REVISION;
	ret = melfas_i2c_master_send(ts->client, &buff, 1);
	if (ret < 0) {
		printk(KERN_ERR "%s : i2c_master_send [%d]\n", __func__, ret);
	}
	ret = melfas_i2c_master_recv(ts->client, &buff, 7);
	if (ret < 0) {
		printk(KERN_ERR "%s : i2c_master_recv [%d]\n", __func__, ret);
	}
	fw_latest_version		= buff[3];
	privatecustom_version	= buff[4];
	publiccustom_version	= buff[5];
	return snprintf (buf, sizeof(buf), "%#02x, %#02x, %#02x\n", fw_latest_version, privatecustom_version, publiccustom_version);
}
static ssize_t set_tsp_threshold_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	u8 threshold;
	melfas_i2c_read(ts->client, P5_THRESHOLD, 1, &threshold);
	return snprintf (buf, sizeof(buf), "%d\n", threshold);
}
static ssize_t tsp_touchtype_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char temp[15];
	snprintf (temp, sizeof(temp), "TSP : MMS144\n");
	return 1;
}
#endif
ssize_t set_tsp_for_inputmethod_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk(KERN_ERR "[TSP] %s is called.. is_inputmethod=%d\n", __func__, is_inputmethod);
	if (is_inputmethod)
		*buf = '1';
	else
		*buf = '0';
	return 0;
}
ssize_t set_tsp_for_inputmethod_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	//unsigned int register_address = 0;
	if (tsp_enabled == false) {
		printk(KERN_ERR "[TSP ]%s. tsp_enabled is 0\n", __func__);
		return 1;
	}
	if (*buf == '1' && (!is_inputmethod)) {
		is_inputmethod = 1;
		printk(KERN_ERR "[TSP] Set TSP inputmethod IN\n");
		/* to do */
	} else if (*buf == '0' && (is_inputmethod)) {
		is_inputmethod = 0;
		printk(KERN_ERR "[TSP] Set TSP inputmethod OUT\n");
		/* to do */
	}
	return 1;
}
static ssize_t tsp_call_release_touch(struct device *dev, struct device_attribute *attr, char *buf)
{

	printk(KERN_ERR " %s is called\n", __func__);
	TSP_reboot();
	return snprintf (buf, sizeof(buf), "0\n");
}

#ifdef TSP_BOOST
ssize_t set_tsp_for_boost_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk(KERN_ERR "[TSP] %s is called.. is_inputmethod=%d\n", __func__, is_boost);
	if (is_boost)
		*buf = '1';
	else
		*buf = '0';

	return 0;
}
ssize_t set_tsp_for_boost_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	u16 obj_address = 0;
	u16 size_one;
	int ret;
	u8 value;
	int jump_limit = 0;
	int mrgthr = 0;
	u8 val = 0;
	unsigned int register_address = 0;
	if (tsp_enabled == false) {
		printk(KERN_ERR "[TSP ]%s. tsp_enabled is 0\n", __func__);
		return 1;
	}
	if (*buf == '1' && (!is_boost)) {
		is_boost = 1;
	} else if (*buf == '0' && (is_boost)) {
		is_boost = 0;
	}
	printk(KERN_ERR "[TSP] set_tsp_for_boost_store() called. %s!\n", is_boost ? "On" : "Off");
	TSP_boost(ts, is_boost);
	return 1;
}
#endif
#if 0
static DEVICE_ATTR(tsp_threshold, S_IRUGO | S_IWUSR | S_IWGRP, set_tsp_threshold_mode_show, NULL);
static DEVICE_ATTR(set_tsp_for_inputmethod, S_IRUGO | S_IWUSR | S_IWGRP, set_tsp_for_inputmethod_show, set_tsp_for_inputmethod_store); /* For 3x4 Input Method, Jump limit changed API */
static DEVICE_ATTR(call_release_touch, S_IRUGO | S_IWUSR | S_IWGRP, tsp_call_release_touch, NULL);
static DEVICE_ATTR(mxt_touchtype, S_IRUGO | S_IWUSR | S_IWGRP,	tsp_touchtype_show, NULL);
#ifdef TSP_BOOST
static DEVICE_ATTR(set_tsp_boost, S_IRUGO | S_IWUSR | S_IWGRP, set_tsp_for_boost_show, set_tsp_for_boost_store); /* Control wait_filter to boost response. */
#endif

static struct attribute *sec_touch_attributes[] = {
	&dev_attr_tsp_firm_version_phone.attr,
	&dev_attr_tsp_firm_version_panel.attr,
	&dev_attr_tsp_threshold.attr,
	&dev_attr_set_tsp_for_inputmethod.attr,
	&dev_attr_call_release_touch.attr,
	&dev_attr_mxt_touchtype.attr,
#ifdef TSP_BOOST
	&dev_attr_set_tsp_boost.attr,
#endif
	NULL,
};
#endif
#if 0
static struct attribute_group sec_touch_attr_group = {
	.attrs = sec_touch_attributes,
};
#endif
#endif
#ifdef TSP_FACTORY_TEST
//static bool debug_print = true;
//static u16 inspection_data[NODE_NUM] = { 0, };
static u16 lntensity_data[NODE_NUM] = { 0, };
static u16 CmDelta_data[NODE_NUM] = { 0, }; /* inspection */
static u16 CmABS_data[NODE_NUM] = { 0, }; /* reference */
static ssize_t set_tsp_module_on_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret;
	ret = melfas_ts_resume(ts->client);
	if (ret  == 0)
		*buf = '1';
	else
		*buf = '0';
	msleep(500);
	return 0;
}
static ssize_t set_tsp_module_off_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret;
	ret = melfas_ts_suspend(ts->client, PMSG_SUSPEND);
	if (ret  == 0)
		*buf = '1';
	else
		*buf = '0';
	return 0;
}
/* CM ABS */
static int check_debug_data(struct melfas_ts_data *ts)
{
	u8 setLowLevelData[4];
	u8 read_data_buf[50] = {0,};
	//u16 read_data_buf1[50] = {0,};
	//int read_data_len;
	int sensing_ch, exciting_ch;
	int ret, i, j, status = 0;
	int size;
	tsp_testmode = 1;
	printk(KERN_ERR "[TSP] %s entered. line : %d\n", __func__, __LINE__);
	printk(KERN_ERR "[TSP] %s disable IRQ( %d)\n", __func__, __LINE__);
	disable_irq(ts->client->irq);
	exciting_ch = g_exciting_ch;
	sensing_ch = g_sensing_ch;
	/* Read Reference Data */
	setLowLevelData[0] = 0xA0; /* UNIVERSAL_CMD */
	setLowLevelData[1] = 0x40; /* UNIVCMD_ENTER_TEST_MODE */
	ret = melfas_i2c_write(ts->client, setLowLevelData, 2);
	while (gpio_get_value(GPIO_TOUCH_INT))
		;

	ret = melfas_i2c_read(ts->client, 0xAE, 1, read_data_buf);
	printk(KERN_ERR "\n\n --- CM_ABS --- ");
	/* Read Reference Data */
	setLowLevelData[0] = 0xA0;
	setLowLevelData[1] = 0x43; /* UNIVCMD_TEST_CM_ABS */
	melfas_i2c_write(ts->client, setLowLevelData, 2);
	while (gpio_get_value(GPIO_TOUCH_INT))
		;

	ret = melfas_i2c_read(ts->client, 0xAE, 1, read_data_buf);
	printk(KERN_ERR "[TSP] %s ret= %d\n", __func__, ret);
	for (i = 0; i < sensing_ch; i++) {
		for (j = 0; j < exciting_ch; j++) {
			setLowLevelData[0] = 0xA0;
			setLowLevelData[1] = 0x44;
			setLowLevelData[2] = j;
			setLowLevelData[3] = i;
			melfas_i2c_write(ts->client, setLowLevelData, 4);
			while (gpio_get_value(GPIO_TOUCH_INT))
				;

			melfas_i2c_read(ts->client, 0xAE,
				1, read_data_buf);
			size = read_data_buf[0];
			melfas_i2c_read(ts->client, 0xAF,
				size, read_data_buf);
			CmABS_data[(i * exciting_ch) + j]
				= (read_data_buf[0] |  read_data_buf[1] << 8);
			if ((CmABS_data[(i * exciting_ch) + j]
					>= SCR_ABS_LOWER_SPEC[i][j])
				&& (CmABS_data[(i * exciting_ch) + j]
					<= SCR_ABS_UPPER_SPEC[i][j]))
				status = 1; /* fail */
			else
				status = 0; /* pass */
		}
	}
	printk(KERN_ERR "[TSP] CmABS_data\n");
	for (i = 0; i < exciting_ch * sensing_ch; i++) {
		if (0 == i % exciting_ch)
			printk(KERN_INFO "\n");
		printk(KERN_ERR "%4d, ", CmABS_data[i]);
	}
	printk(KERN_INFO "\n");
	/* Read Reference Data */
	setLowLevelData[0] = 0xA0;
	setLowLevelData[1] = 0x4F;
	melfas_i2c_write(ts->client, setLowLevelData, 2);

	printk(KERN_ERR "[TSP] %s enable IRQ( %d)\n", __func__, __LINE__);
	enable_irq(ts->client->irq);
	tsp_testmode = 0;
	TSP_reboot();
	printk(KERN_ERR "%s : end\n", __func__);
	return status;
}
/* inspection = CmDelta_data */
static int check_delta_data(struct melfas_ts_data *ts)
{
	u8 setLowLevelData[4];
	u8 read_data_buf[50] = {0,};
	//u16 read_data_buf1[50] = {0,};
	//int read_data_len;
	int sensing_ch, exciting_ch;
	int i, j; //, status=0;
	int size;
	printk(KERN_ERR "[TSP] %s entered. line : %d,\n", __func__, __LINE__);

	printk(KERN_ERR "[TSP] %s disable IRQ( %d)\n", __func__, __LINE__);
	disable_irq(ts->client->irq);
	exciting_ch = g_exciting_ch;
	sensing_ch	 = g_sensing_ch;
	/* Read Reference Data */
	setLowLevelData[0] = 0xA0; /* UNIVERSAL_CMD */
	setLowLevelData[1] = 0x40; /* UNIVCMD_ENTER_TEST_MODE */
	melfas_i2c_write(ts->client, setLowLevelData, 2);
	while (gpio_get_value(GPIO_TOUCH_INT))
		;

	melfas_i2c_read(ts->client, 0xAE, 1, read_data_buf);
	printk(KERN_ERR "\n\n --- CM_DELTA --- ");
	/* Read Reference Data */
	setLowLevelData[0] = 0xA0;
	setLowLevelData[1] = 0x41;
	melfas_i2c_write(ts->client, setLowLevelData, 2);
	while (gpio_get_value(GPIO_TOUCH_INT))
		;

	melfas_i2c_read(ts->client, 0xAE, 1, read_data_buf);
	for (i = 0; i < sensing_ch; i++) {
		for (j = 0; j < exciting_ch; j++) {
			setLowLevelData[0] = 0xA0;
			setLowLevelData[1] = 0x42;
			setLowLevelData[2] = j; /* Exciting CH. */
			setLowLevelData[3] = i; /* Sensing CH. */
			melfas_i2c_write(ts->client, setLowLevelData, 4);
			while (gpio_get_value(GPIO_TOUCH_INT))
				;

			melfas_i2c_read(ts->client, 0xAE,
				1, read_data_buf);
			size = read_data_buf[0];
			melfas_i2c_read(ts->client, 0xAF,
				size, read_data_buf);
			CmDelta_data[(i * exciting_ch) + j]
				= (read_data_buf[0] |  read_data_buf[1] << 8);
		}
	}
	printk(KERN_ERR "[TSP] CmDelta_data\n");
	for (i = 0; i < exciting_ch * sensing_ch; i++) {
		if (0 == i % exciting_ch)
			printk(KERN_INFO "\n");
		printk(KERN_ERR "%4d, ", CmDelta_data[i]);
	}
	printk(KERN_INFO "\n");
	/* Read Reference Data */
	setLowLevelData[0] = 0xA0;
	setLowLevelData[1] = 0x4F;
	melfas_i2c_write(ts->client, setLowLevelData, 2);
	printk(KERN_ERR "[TSP] %s enable IRQ( %d)\n", __func__, __LINE__);
	enable_irq(ts->client->irq);
	tsp_testmode = 0;
	TSP_reboot();
	printk(KERN_ERR "%s : end\n", __func__);
	return 0; //status;
}
static int atoi(const char *str)
{
	int result = 0;
	int count = 0;
	if (str == NULL)
		return result;
	while ( ((str+count) != NULL) \
		&& (str[count] >= '0') \
		&& (str[count] <= '9')) {
		result = result * 10 + str[count] - '0';
		++count;
	}
	return result;
}
ssize_t disp_all_refdata_show(struct device *dev, \
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, 5, "%u\n", CmABS_data[index]);
}
ssize_t disp_all_refdata_store(struct device *dev, \
	struct device_attribute *attr, const char *buf, size_t size)
{
	index = atoi(buf);
	printk(KERN_ERR "%s : value %d\n", __func__, index);
	return size;
}
static ssize_t set_all_delta_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int status = 0;
/* Delta */
	check_delta_data(ts);
	set_tsp_module_off_show(dev, attr, buf);
	set_tsp_module_on_show(dev, attr, buf);
	return snprintf (buf, sizeof(buf), "%d\n", status);
}
static ssize_t set_all_refer_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int status;
/* ABS */
	status = check_debug_data(ts);
	set_tsp_module_off_show(dev, attr, buf);
	set_tsp_module_on_show(dev, attr, buf);
	return snprintf (buf, sizeof(buf), "%d\n", status);
}
ssize_t disp_all_deltadata_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk(KERN_ERR "disp_all_deltadata_show : value %d\n", CmDelta_data[index]);
	return snprintf (buf, sizeof(buf), "%u\n",  CmDelta_data[index]);
}
ssize_t disp_all_deltadata_store(struct device *dev, struct device_attribute *attr,
								   const char *buf, size_t size)
{
	index = atoi(buf);
	printk(KERN_ERR "Delta data %d", index);
	return size;
}
static void check_intensity_data(struct melfas_ts_data *ts)
{
	//int16_t menu = 0;
	//int16_t back = 0;
	u8 setLowLevelData[6];
	u8 read_data_buf[50] = {0,};
	int sensing_ch, exciting_ch;
	int i, j;
	printk(KERN_ERR "[TSP] %s entered. line : %d\n", __func__, __LINE__);
	//menu = gMenuKey_Intensity;
	//back = gBackKey_Intensity;
	exciting_ch = g_exciting_ch;
	sensing_ch = g_sensing_ch;

	printk(KERN_ERR "[TSP] %s disable IRQ( %d)\n", __func__, __LINE__);
	disable_irq(ts->client->irq);
	for (i = 0; i < sensing_ch; i++) {
		for (j = 0 ; j < exciting_ch; j++) {
			setLowLevelData[0] = 0xB0; /* VENDOR_SPECIFIC_CMD */
			setLowLevelData[1] = 0x1A; /* VENDOR_CMD_SS_TSP_S */
			setLowLevelData[2] = j;
			setLowLevelData[3] = i;
			setLowLevelData[4] = 0x00; /* Reserved */
			setLowLevelData[5] = 0x04;
			melfas_i2c_write(ts->client, setLowLevelData, 6);
			melfas_i2c_read(ts->client, 0xBF,
				1, read_data_buf);
			lntensity_data[(i * exciting_ch) + j]
				= read_data_buf[0];
		}
/*
		if (i == 0)
				lntensity_data[(i * exciting_ch) + j-1] = menu;
		else if (i == 1)
				lntensity_data[(i * exciting_ch) + j-1] = back;
		*/
	}
	setLowLevelData[0] = 0xB0; /* VENDOR_SPECIFIC_CMD */
	setLowLevelData[1] = 0x1A; /* VENDOR_CMD_SS_TSP_S */
	setLowLevelData[2] = 0x00;
	setLowLevelData[3] = 0x00;
	setLowLevelData[4] = 0x00; /* Reserved */
	setLowLevelData[5] = 0x05; /* VENDOR_CMD_SS_TSP_S_EXIT_MODE */
	melfas_i2c_write(ts->client, setLowLevelData, 6);
#if 1
	printk(KERN_ERR "[TSP] lntensity_data\n");
	for (i = 0; i < exciting_ch * sensing_ch; i++) {
		if (0 == i % exciting_ch)
			printk(KERN_INFO"\n");
		printk(KERN_ERR"%4d, ", lntensity_data[i]);
	}
	printk(KERN_INFO"\n");
#endif

	printk(KERN_ERR "[TSP] %s enable IRQ( %d)\n", __func__, __LINE__);
	enable_irq(ts->client->irq);
	msleep(20);
	check_delta_data(ts);
	printk(KERN_ERR "%s : end\n", __func__);
}

/*
static ssize_t set_refer0_mode_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	u16 refrence = 0;
	check_intensity_data(ts);
	refrence = inspection_data[28];
	return snprintf (buf, sizeof(buf), "%u\n", refrence);
}
static ssize_t set_refer1_mode_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	u16 refrence = 0;
	refrence = inspection_data[288];
	return snprintf (buf, sizeof(buf), "%u\n", refrence);
}
static ssize_t set_refer2_mode_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	u16 refrence = 0;
	refrence = inspection_data[194];
	return snprintf (buf, sizeof(buf), "%u\n", refrence);
}
static ssize_t set_refer3_mode_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	u16 refrence = 0;
	refrence = inspection_data[49];
	return snprintf (buf, sizeof(buf), "%u\n", refrence);
}
static ssize_t set_refer4_mode_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	u16 refrence = 0;
	refrence = inspection_data[309];
	return snprintf (buf, sizeof(buf), "%u\n", refrence);
}
static ssize_t set_intensity0_mode_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	u16 intensity = 0;
	intensity = lntensity_data[28];
	return snprintf (buf, sizeof(buf), "%u\n", intensity);
}
static ssize_t set_intensity1_mode_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	u16 intensity = 0;
	intensity = lntensity_data[288];
	return snprintf (buf, sizeof(buf), "%u\n", intensity);
}
static ssize_t set_intensity2_mode_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	u16 intensity = 0;
	intensity = lntensity_data[194];
	return snprintf (buf, sizeof(buf), "%u\n", intensity);
}
static ssize_t set_intensity3_mode_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	u16 intensity = 0;
	intensity = lntensity_data[49];
	return snprintf (buf, sizeof(buf), "%u\n", intensity);
}
static ssize_t set_intensity4_mode_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	u16 intensity = 0;
	intensity = lntensity_data[309];
	return snprintf (buf, sizeof(buf), "%u\n", intensity);
}

static DEVICE_ATTR(set_refer0, S_IRUGO | S_IWUSR | S_IWGRP, set_refer0_mode_show, NULL);
static DEVICE_ATTR(set_delta0, S_IRUGO | S_IWUSR | S_IWGRP, set_intensity0_mode_show, NULL);
static DEVICE_ATTR(set_refer1, S_IRUGO | S_IWUSR | S_IWGRP, set_refer1_mode_show, NULL);
static DEVICE_ATTR(set_delta1, S_IRUGO | S_IWUSR | S_IWGRP, set_intensity1_mode_show, NULL);
static DEVICE_ATTR(set_refer2, S_IRUGO | S_IWUSR | S_IWGRP, set_refer2_mode_show, NULL);
static DEVICE_ATTR(set_delta2, S_IRUGO | S_IWUSR | S_IWGRP, set_intensity2_mode_show, NULL);
static DEVICE_ATTR(set_refer3, S_IRUGO | S_IWUSR | S_IWGRP, set_refer3_mode_show, NULL);
static DEVICE_ATTR(set_delta3, S_IRUGO | S_IWUSR | S_IWGRP, set_intensity3_mode_show, NULL);
static DEVICE_ATTR(set_refer4, S_IRUGO | S_IWUSR | S_IWGRP, set_refer4_mode_show, NULL);
static DEVICE_ATTR(set_delta4, S_IRUGO | S_IWUSR | S_IWGRP, set_intensity4_mode_show, NULL);
static DEVICE_ATTR(set_threshould, S_IRUGO | S_IWUSR | S_IWGRP,
	set_tsp_threshold_mode_show, NULL);
static struct attribute *sec_touch_facotry_attributes[] = {
	&dev_attr_set_refer0.attr,
	&dev_attr_set_delta0.attr,
	&dev_attr_set_refer1.attr,
	&dev_attr_set_delta1.attr,
	&dev_attr_set_refer2.attr,
	&dev_attr_set_delta2.attr,
	&dev_attr_set_refer3.attr,
	&dev_attr_set_delta3.attr,
	&dev_attr_set_refer4.attr,
	&dev_attr_set_delta4.attr,
	&dev_attr_set_threshould.attr,
	NULL,
};
*/
#endif
static void release_all_fingers(struct melfas_ts_data *ts)
{
	int i;
	printk(KERN_ERR "%s start.\n", __func__);
	for (i = 0; i < MELFAS_MAX_TOUCH; i++) {
		if (-1 == g_Mtouch_info[i].strength) {
			g_Mtouch_info[i].posX = 0;
			g_Mtouch_info[i].posY = 0;
			continue;
		}
		printk(KERN_ERR "%s %s(%d)\n", __func__,
				ts->input_dev->name, i);
		g_Mtouch_info[i].strength = 0;
		input_mt_slot(ts->input_dev, i);
		input_mt_report_slot_state(ts->input_dev,
							MT_TOOL_FINGER, false);
		g_Mtouch_info[i].posX = 0;
		g_Mtouch_info[i].posY = 0;
		if (0 == g_Mtouch_info[i].strength)
			g_Mtouch_info[i].strength = -1;
		}
	input_sync(ts->input_dev);
#if defined(TSP_BOOSTER)
	set_dvfs_lock(ts, 2);
	printk(KERN_INFO "[TSP] dvfs_lock free.\n ");
#endif
}
static void TSP_reboot(void)
{
#if 1
	printk(KERN_ERR "%s start!\n", __func__);
	printk(KERN_ERR "[TSP] %s disable IRQ( %d)\n", __func__, __LINE__);
	disable_irq_nosync(ts->client->irq);
	tsp_enabled = false;

	ts_power_enable(0);
	msleep(60);

	release_all_fingers(ts);

	msleep(60);
	ts_power_enable(1);
	msleep(60);

	printk(KERN_ERR "[TSP] %s enable IRQ( %d)\n", __func__, __LINE__);
	enable_irq(ts->client->irq);
	tsp_enabled = true;

#else
	if (tsp_enabled == false)
		return;
	printk(KERN_ERR "%s satrt!\n", __func__);
	disable_irq_nosync(ts->client->irq);
	tsp_enabled = false;
	touch_is_pressed = 0;
	release_all_fingers(ts);
	ts->gpio();
	ts->power(false);
	msleep(200);
	ts->power(true);
#ifdef TSP_BOOST
	TSP_boost(ts, is_boost);
#endif
	printk(KERN_ERR "[TSP] %s enable IRQ( %d)\n", __func__, __LINE__);
	enable_irq(ts->client->irq);

	tsp_enabled = true;
#endif
};
void TSP_force_released(void)
{
	printk(KERN_ERR "%s satrt!\n", __func__);
	if (tsp_enabled == false) {
		printk(KERN_ERR "[TSP] Disabled\n");
		return;
	}
	release_all_fingers(ts);
	touch_is_pressed = 0;
};
EXPORT_SYMBOL(TSP_force_released);
void TSP_ESD_seq(void)
{
	TSP_reboot();
	printk(KERN_ERR "%s satrt!\n", __func__);
};
EXPORT_SYMBOL(tsp_call_release_touch);
#if defined(SET_TSP_CONFIG) && defined(TSP_BOOST)
static int melfas_set_config(struct i2c_client *client, u8 reg, u8 value)
{
	u8 buffer[2];
	int ret;
	struct melfas_ts_data *ts = i2c_get_clientdata(client);
	buffer[0] = reg;
	buffer[1] = value;
	ret = melfas_i2c_write(ts->client, (char *)buffer, 2);
	return ret;
}
#endif
int tsp_i2c_read_melfas(u8 reg, unsigned char *rbuf, int buf_size)
{
	int i, ret = -1;
	struct i2c_msg rmsg;
	uint8_t start_reg;
	int retry = 3;
	for (i = 0; i < retry; i++) {
		rmsg.addr = ts->client->addr;
		rmsg.flags = 0;
		rmsg.len = 1;
		rmsg.buf = &start_reg;
		start_reg = reg;
		ret = i2c_transfer(ts->client->adapter, &rmsg, 1);
		if (ret >= 0) {
			rmsg.flags = I2C_M_RD;
			rmsg.len = buf_size;
			rmsg.buf = rbuf;
			ret = i2c_transfer(ts->client->adapter, &rmsg, 1);
			if (ret >= 0)
				break;
		}
		if (i == (retry - 1)) {
			printk(KERN_ERR "[TSP] Error code : %d, %d\n", __LINE__, ret);
		}
	}
	return ret;
}
#ifdef TUNNING_SUPPORT
static void mms_report_input_data(struct melfas_ts_data *ts, u8 sz, u8 *buf)
{
	int i;
	struct i2c_client *client = ts->client;

	if (buf[0] == MMS_NOTIFY_EVENT) {
		dev_info(&client->dev,
			 "TSP mode changed (%d)\n",
			 buf[1]);
	} else if(buf[0] == MMS_ERROR_EVENT) {
		dev_info(&client->dev,
			 "Error deteced, restarting TSP\n");

		for (i = 0; i< MAX_FINGER_NUM; i++) {
			input_mt_slot(ts->input_dev, i);
			input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);
		}
		input_sync(ts->input_dev);

//		mms_pwr_on_reset(ts);
		TSP_reboot();

	} else {
		for (i = 0; i < sz; i += FINGER_EVENT_SZ) {
			u8 *tmp = &buf[i];
			int id = (tmp[0] & 0xf) - 1;
			int x = tmp[2] | ((tmp[1] & 0xf) << 8);
			int y = tmp[3] | (((tmp[1] >> 4) & 0xf) << 8);
			int touch_major = tmp[4];
			int pressure = tmp[5];

			if(!(tmp[0] & 0x80)) {
				input_mt_slot(ts->input_dev, id);
				input_mt_report_slot_state(ts->input_dev,
							   MT_TOOL_FINGER, false);
				continue;
			}

			input_mt_slot(ts->input_dev, id);
			input_mt_report_slot_state(ts->input_dev,
						   MT_TOOL_FINGER, true);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);
			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, touch_major);
			input_report_abs(ts->input_dev, ABS_MT_PRESSURE, pressure);

		}

		input_sync(ts->input_dev);
	}
}
#endif
#if 0
static int melfas_i2c_read(struct i2c_client *p_client, u8 reg, u8 *data, int len)
{
	struct i2c_msg msg;
	/* set start register for burst read */
	/* send separate i2c msg to give STOP signal after writing. */
	/* Continous start is not allowed for cypress touch sensor. */
	msg.addr = p_client->addr;
	msg.flags = 0;
	msg.len = 1;
	msg.buf = &reg;
	if (1 != i2c_transfer(p_client->adapter, &msg, 1)) {
		printk(KERN_ERR "[TSP][MMS128][%s] set data pointer fail! reg(%x)\n", __func__, reg);
		return -EIO;
	}
	/* begin to read from the starting address */
	msg.addr = p_client->addr;
	msg.flags = I2C_M_RD;
	msg.len = len;
	msg.buf = data;
	if (1 != i2c_transfer(p_client->adapter, &msg, 1)) {
		printk(KERN_ERR "[TSP][MMS128][%s] fail! reg(%x)\n", __func__, reg);
		return -EIO;
	}
	return 0;
}
#endif
static ssize_t firmware_panel_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	extern unsigned char TSP_PanelVersion;
	int panel;
	panel = TSP_PanelVersion;
	printk(KERN_ERR "firmware_panel_show : [%d]\n", panel);
	return snprintf(buf, sizeof(buf), "%x\n", panel);
}
static ssize_t firmware_phone_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	extern unsigned char TSP_PhoneVersion;
	int NEW_FIRMWARE_VERSION = TSP_PhoneVersion;
	return snprintf(buf, sizeof(buf), "%x\n", NEW_FIRMWARE_VERSION);
}
static ssize_t threshold_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int threshold;	//touchKey

#if defined(CONFIG_MACH_INFINITE_DUOS_CTC)
	threshold = 25;
#elif defined(CONFIG_MACH_KYLEPLUS_CTC) || defined(CONFIG_MACH_KYLEPLUS_OPEN)
	threshold = 30;
#elif defined(MELFAS_MMS136_4P66) || defined(MELFAS_MMS136_4P3) || defined(CONFIG_MACH_NEVIS3G)
	threshold = 20;
#else
	threshold = 30;
#endif
	return snprintf(buf, sizeof(buf), "%d\n", threshold);
}
static ssize_t firmware_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	u8 buf1[6] = {0,};
	u8 hw_rev, fw_ver, phone_ver;
	if (0 == melfas_i2c_read(ts->client, MIP_TSP_REVISION, 6, buf1))
	{
		hw_rev = buf1[1];
		fw_ver = buf1[5];
		phone_ver = FW_VERSION;
		snprintf(buf, sizeof(buf), "%03X%02X%02X\n",
			hw_rev, fw_ver, phone_ver);
		printk(KERN_ERR "[TSP][MMS128][%s]  phone_ver=%d, fw_ver=%d, hw_rev=%d\n",
			buf, phone_ver, fw_ver, hw_rev);
	} else {
		printk(KERN_ERR "[TSP][MMS128][%s] Can't find HW Ver, FW ver!\n",
				__func__);
	}
	return snprintf(buf, sizeof(buf), "%s", buf);
}
static ssize_t firmware_store(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t size)
{
	int ret;
	//bool flag = false;
	printk(KERN_INFO "START firmware store\n");
	ts_power_enable(0);
	msleep(500);
	ts_power_enable(1);
	msleep(500);

	printk(KERN_ERR "[TSP] %s disable IRQ( %d)\n", __func__, __LINE__);
	disable_irq(ts->client->irq);
	local_irq_disable();

#if SET_DOWNLOAD_BY_GPIO
#if defined(SET_DOWNLOAD_CONFIG)
	ret = mms100_ISC_download_mbinary(ts->client , false); //flag);
	ts_power_enable(0);
	mdelay(30);
	ts_power_enable(1);
	msleep(200);
#else    	// ICS mode for kyle 
	ret = MFS_ISC_update();
#endif
	printk(KERN_ERR "mcsdl_download_binary_data : [%d]\n", ret);
#endif

	local_irq_enable();
	printk(KERN_ERR "[TSP] %s enable IRQ( %d)\n", __func__, __LINE__);
	enable_irq(ts->client->irq);
#if 0
	hrtimer_start(&ts->timer, ktime_set(0, 200000000), HRTIMER_MODE_REL);
#if defined (__TOUCH_TA_CHECK__)
	b_Firmware_store = false;
#endif
	if (ret == MCSDL_RET_SUCCESS)
		firmware_ret_val = 1;
	else
		firmware_ret_val = 0;
#endif
	printk(KERN_INFO"[TSP] Firmware update end!!\n");
	ts_power_enable(0);
	msleep(500);
	ts_power_enable(1);
	
	printk(KERN_INFO "firmware store END\n");
	return 0;
}
static ssize_t tsp_threshold_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	u8 threshold = 30;
	return snprintf(buf, sizeof(buf), "%d\n", threshold);
}
static ssize_t tsp_firm_update_status_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	u8 firm_update_status = 0;
	return snprintf(buf, sizeof(buf), "%d\n", firm_update_status);
}
static ssize_t touchkey_back_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
#if defined(CONFIG_MACH_INFINITE_DUOS_CTC)
	int ret;
	unsigned char touchkey_back_addr = 0x66;
	unsigned char rd_buf = 0x00;

	ret = melfas_i2c_read(ts->client, touchkey_back_addr, 1, &rd_buf);
	if (ret<0)
	{
		pr_info("[TSP] %s,%d: i2c read fail[%d] \n", __FUNCTION__, __LINE__, ret);
	}
	if((gBackKey_state==0)&&(gBackKey_show==0))
	{
		rd_buf = before_gBackKey_Intensity;	
	}
	gBackKey_show++;	
	return snprintf(buf, 10, "%d\n", rd_buf);
#else
	#if defined(CONFIG_MACH_DELOS_DUOS_CTC) || defined(CONFIG_MACH_DELOS_OPEN)
		gBackKey_show++;
		if((gBackKey_state==0)&&(gBackKey_show==1))
			return snprintf(buf, 10, "%d\n", before_gBackKey_Intensity);			
	#endif
		
	return snprintf(buf, 10, "%d\n", gBackKey_Intensity);
#endif
}
static ssize_t touchkey_menu_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
#if defined(CONFIG_MACH_INFINITE_DUOS_CTC)
	int ret;
	unsigned char touchkey_menu_addr = 0x65;
	unsigned char rd_buf = 0x00;

	ret = melfas_i2c_read(ts->client, touchkey_menu_addr, 1, &rd_buf);
	if (ret<0)
	{
		pr_info("[TSP] %s,%d: i2c read fail[%d] \n", __FUNCTION__, __LINE__, ret);
	}
	if((gMenuKey_state==0)&&(gMenuKey_show==0))
	{
		rd_buf = before_gMenuKey_Intensity;	
	}
	gMenuKey_show++;	
	return snprintf(buf, 10, "%d\n", rd_buf);
#else
	#if defined(CONFIG_MACH_DELOS_DUOS_CTC) || defined(CONFIG_MACH_DELOS_OPEN)
		gMenuKey_show++;
		if((gMenuKey_state==0)&&(gMenuKey_show==1))
			return snprintf(buf, 10, "%d\n", before_gMenuKey_Intensity);
	#endif

	return snprintf(buf, 10, "%d\n", gMenuKey_Intensity);
#endif
}
#ifdef LOW_LEVEL_CHECK
static ssize_t	check_init_lowleveldata()
{
	u8 read_buf[1] = {0,};
	int ret;
		ret = melfas_i2c_read(ts->client, 0x2e, 1, read_buf);
		if (ret < 0) {
			printk(KERN_ERR "[TSP]Exciting CH. melfas_i2c_read fail! %s : %d, \n",
				__func__, __LINE__);
			return 0;
		}
		g_exciting_ch = read_buf[0];
		ret = melfas_i2c_read(ts->client, 0x2f, 1, read_buf);
		if (ret < 0) {
			printk(KERN_ERR "[TSP]Sensing CH. melfas_i2c_read fail! %s : %d, \n",
				__func__, __LINE__);
			return 0;
		}
		g_sensing_ch = read_buf[0];

	return ret;
}
#endif
//static int start_rawcounter = 1;
static ssize_t tkey_rawcounter_store(struct device *dev, \
struct device_attribute *attr, const char *buf, size_t size)
{
	char *after;
	unsigned long value = simple_strtoul(buf, &after, 10);
	printk(KERN_INFO "[TSP] %s, %d, value=%ld\n", __func__, __LINE__, value);
	return size;
}
static ssize_t tkey_rawcounter_show(struct device *dev, \
			struct device_attribute *attr, char *buf)
{
	printk(KERN_ERR "[TSP] menukey : %d, backKey : %d\n", \
	gMenuKey_Intensity, gBackKey_Intensity);
	mdelay(1);
	return snprintf(buf, 10, "%d %d\n", \
	gMenuKey_Intensity, gBackKey_Intensity);
}
static ssize_t set_module_off_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	TSP_reboot();
	msleep(300);
	printk(KERN_INFO "set_tsp_test_mode_disable0 \n");
	tsp_testmode = 0;
	return 0;
}
static ssize_t set_module_on_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	tsp_testmode = 1;
	printk(KERN_INFO "set_tsp_test_mode_enable0 \n");
	mdelay(50);
	ts_power_enable(0);
	mdelay(500);
	ts_power_enable(1);
	return 0;
}
static ssize_t touchtype_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, 15, "MMS128\n");
}
static ssize_t set_all_intensity_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int status = 0;
	check_intensity_data(ts);
	set_tsp_module_off_show(dev, attr, buf);
	set_tsp_module_on_show(dev, attr, buf);
	return snprintf (buf, sizeof(buf), "%d\n", status);
}
ssize_t disp_all_intendata_store(struct device *dev, struct device_attribute *attr,
								   const char *buf, size_t size)
{
	index = atoi(buf);
	printk(KERN_ERR "Intensity data %d", index);
	return size;
}
ssize_t disp_all_intendata_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	printk(KERN_ERR "disp_all_intendata_show : value %d, index=%d\n",
		   lntensity_data[index], index);
    return snprintf (buf, sizeof(buf), "%u\n",  lntensity_data[index]);
}
static ssize_t rawdata_pass_fail_melfas(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u8 setLowLevelData[2] = {0x09, 0x01,};
	u8 read_data_buf[50] = {0,};
	u16 read_data_buf1[50] = {0,};
	int read_data_len, sensing_ch;
	int ret, i, j;
	tsp_testmode = 1;

	printk(KERN_ERR "[TSP] %s disable IRQ( %d)\n", __func__, __LINE__);
	disable_irq(ts->client->irq);
	read_data_len = g_exciting_ch * 2;
	sensing_ch	 = g_sensing_ch;
	for (i = 0; i < sensing_ch; i++) {
		ret = melfas_i2c_write(ts->client, setLowLevelData, 2);
		while (gpio_get_value(GPIO_TOUCH_INT)) {
			udelay(50);
		}
		udelay(300);
		ret = melfas_i2c_read(ts->client, 0xb2, read_data_len, read_data_buf);
		if (ret < 0)
			printk(KERN_ERR "can't read rawdata_pass_fail_tst200 Data %dth\n", i);
		udelay(5);
		for (j = 0 ; j < read_data_len / 2; j++) {
			read_data_buf1[j] = (read_data_buf[j*2] << 8) + read_data_buf[j*2+1];
			if ((SCR_ABS_UPPER_SPEC[i][j] < read_data_buf1[j])
				|| (SCR_ABS_LOWER_SPEC[i][j] > read_data_buf1[j])) {
				printk(KERN_ERR "\n SCR_ABS_UPPER_SPEC[i][j] = %d",
						SCR_ABS_UPPER_SPEC[i][j]);
				printk(KERN_ERR "\n SCR_ABS_LOWER_SPEC[i][j] = %d",
						SCR_ABS_LOWER_SPEC[i][j]);
				printk(KERN_ERR "\n i=%d, j=%d, read_data_buf1[j]=%d",
						i, j, read_data_buf1[j]);
			printk(KERN_ERR "[TSP] %s enable IRQ( %d)\n"
				, __func__, __LINE__);
				enable_irq(ts->client->irq);
				udelay(10);
				TSP_reboot();
				return snprintf (buf, sizeof(buf), "0");
			}
		}
		printk(KERN_INFO "\n");
#if 1
		printk(KERN_ERR "[%d]:", i);
		for (j = 0; j < read_data_len; j++) {
			printk(KERN_ERR "[%03d],", read_data_buf[j]);
		}
		printk(KERN_INFO "\n");
#endif
		msleep(1);
	}

	printk(KERN_ERR "[TSP] %s enable IRQ( %d)\n", __func__, __LINE__);
	enable_irq(ts->client->irq);
	tsp_testmode = 0;
	TSP_reboot();
    return snprintf (buf, sizeof(buf), "1");
}
static ssize_t touch_sensitivity_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, sizeof(int), "%x\n", 0);
}
#if 0
static ssize_t touchkey_firm_store(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret, i;
	//bool flag = false;
	for (i = 0; i < DOWNLOAD_RETRY_CNT; i++) {

#if SET_DOWNLOAD_BY_GPIO
#if defined(SET_DOWNLOAD_CONFIG)
		ret = mms100_ISC_download_mbinary(ts->client, false); //flag);
		ts_power_enable(0);
		mdelay(30);
		ts_power_enable(1);
		msleep(200);
#else    	// ICS mode for kyle 
		ret = MFS_ISC_update();
#endif
#endif
		printk(KERN_ERR "mcsdl_download_binary_data : [%d]\n", ret);
		if (ret != 0)
			printk(KERN_ERR "SET Download Fail - error code [%d]\n",
				ret);
		else
			break;
	}

	return printk(KERN_INFO "\n[Melfas]TSP firmware update by kyestring");
}
#endif
#if defined(CONFIG_MACH_DELOS_OPEN)
#define KEY_LED_GPIO 124
#elif defined(CONFIG_MACH_DELOS_DUOS_CTC) || defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
#define KEY_LED_GPIO 7 //PMIC_GPIO_8 
extern int pmic_gpio_direction_output(unsigned gpio);
extern int pmic_gpio_set_value(unsigned gpio, int value);
#elif defined(CONFIG_MACH_ARUBA_DUOS_CTC)
#define KEY_LED_GPIO 73
#elif (defined(CONFIG_MACH_ARUBA_OPEN) && !defined(CONFIG_MACH_KYLEPLUS_OPEN)) || defined(CONFIG_MACH_ARUBASLIM_OPEN)
#define KEY_LED_GPIO 124
#elif defined(CONFIG_MACH_ARUBASLIMQ_OPEN_CHN)
#define KEY_LED_GPIO 98
#endif

static ssize_t touch_led_control(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t size)
{
	//int int_data;
	//int errnum;	
#if defined(CONFIG_MACH_DELOS_DUOS_CTC) || defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
	int rc = 0;
	unsigned char data;
	printk("touch_led_control start\n");

	if (sscanf(buf, "%c\n", &data) == 1) {
		printk( "touch_led_control data: %d\n", data);
		pmic_gpio_direction_output(KEY_LED_GPIO);
		
		if (data==49) {
			rc = pmic_gpio_set_value(KEY_LED_GPIO,1);
		}else if(data==48){
			rc = pmic_gpio_set_value(KEY_LED_GPIO,0);
		}
		if (rc < 0) printk("%s pmic gpio set 1 error, data %d", __func__, data);
	} else
		printk("touch_led_control Error\n");
	
	return size;

#elif defined(CONFIG_MACH_ARUBA_DUOS_CTC) || (defined(CONFIG_MACH_ARUBA_OPEN) && !defined(CONFIG_MACH_KYLEPLUS_OPEN))\
	|| defined(CONFIG_MACH_DELOS_OPEN) || defined(CONFIG_MACH_ARUBASLIM_OPEN) ||defined(CONFIG_MACH_ARUBASLIMQ_OPEN_CHN)
	unsigned char data;
		printk("touch_led_control start\n");
		
	if (sscanf(buf, "%c\n", &data) == 1) {
//		int_data = atoi(&data);
//		data = data *0x10;
		printk( "touch_led_control data: %d\n", data);

		gpio_tlmm_config(GPIO_CFG(KEY_LED_GPIO, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
		if(data==49){
			gpio_set_value(KEY_LED_GPIO, 1);
		}
		else if(data==48){
			gpio_set_value(KEY_LED_GPIO, 0);
		}

		
	} else
		printk("touch_led_control Error\n");

	return size;
#else
	return 0;
#endif
}


static DEVICE_ATTR(tsp_firm_version_phone,
			S_IRUGO | S_IWUSR | S_IWGRP, firmware_phone_show, NULL);
static DEVICE_ATTR(tsp_firm_version_panel, S_IRUGO | S_IWUSR | S_IWGRP,
			firmware_panel_show, NULL);
static DEVICE_ATTR(tsp_firm_update, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
			firmware_store);
static DEVICE_ATTR(tsp_threshold, S_IRUGO | S_IWUSR | S_IWGRP,
			tsp_threshold_show, NULL);
static DEVICE_ATTR(tsp_firm_update_status, S_IRUGO | S_IWUSR | S_IWGRP,
			tsp_firm_update_status_show, NULL);
static DEVICE_ATTR(set_all_reference, S_IRUGO | S_IWUSR | S_IWGRP,
			set_all_refer_mode_show, NULL);
static DEVICE_ATTR(disp_all_refdata, S_IRUGO | S_IWUSR | S_IWGRP,
			disp_all_refdata_show, disp_all_refdata_store);
static DEVICE_ATTR(set_all_inspection, S_IRUGO | S_IWUSR | S_IWGRP,
			set_all_delta_mode_show, NULL);
static DEVICE_ATTR(disp_all_insdata, S_IRUGO | S_IWUSR | S_IWGRP,
			disp_all_deltadata_show, disp_all_refdata_store);
static DEVICE_ATTR(set_all_intensity, S_IRUGO | S_IWUSR | S_IWGRP,
			set_all_intensity_mode_show, NULL);
static DEVICE_ATTR(disp_all_intdata, S_IRUGO | S_IWUSR | S_IWGRP,
			disp_all_intendata_show, disp_all_intendata_store);
static DEVICE_ATTR(touchtype, S_IRUGO | S_IWUSR | S_IWGRP,
			touchtype_show, NULL);
static DEVICE_ATTR(set_module_off, S_IRUGO | S_IWUSR | S_IWGRP,
			set_module_off_show, NULL);
static DEVICE_ATTR(set_module_on, S_IRUGO | S_IWUSR | S_IWGRP,
			set_module_on_show, NULL);
static DEVICE_ATTR(firmware	, S_IRUGO | S_IWUSR | S_IWGRP,
			firmware_show, firmware_store);
static DEVICE_ATTR(raw_value, 0444, rawdata_pass_fail_melfas, NULL) ;
static DEVICE_ATTR(touchkey_back, S_IRUGO | S_IWUSR | S_IWGRP,
			touchkey_back_show, NULL);
static DEVICE_ATTR(touchkey_menu, S_IRUGO | S_IWUSR | S_IWGRP,
			touchkey_menu_show, NULL);
static DEVICE_ATTR(touchkey_raw_data, S_IRUGO | \
	S_IWUSR | S_IWGRP, tkey_rawcounter_show, tkey_rawcounter_store);
static DEVICE_ATTR(touch_sensitivity, S_IRUGO | S_IWUSR | S_IWGRP,
			touch_sensitivity_show, NULL);
static DEVICE_ATTR(touchkey_threshold, S_IRUGO | S_IWUSR | S_IWGRP,
			threshold_show, NULL);

static DEVICE_ATTR(brightness, S_IRUGO | S_IWUSR | S_IWGRP, NULL,  touch_led_control);


#ifdef SEC_TSP_FACTORY_TEST
static void set_default_result(struct melfas_ts_data *ts)
{
	char delim = ':';
	memset(ts->cmd_result, 0x00, ARRAY_SIZE(ts->cmd_result));
	memcpy(ts->cmd_result, ts->cmd, strlen(ts->cmd));
	strncat(ts->cmd_result, &delim, 1);
}
static void set_cmd_result(struct melfas_ts_data *ts, char *buff, int len)
{
	strncat(ts->cmd_result, buff, len);
}
static inline int msm_irq_to_gpio(unsigned irq)
{
	/* TODO : Need to verify chip->base=0 */
	return irq - MSM_GPIO_TO_INT(0);
}
static void get_raw_data_all(struct melfas_ts_data *ts, u8 cmd)
{
	u8 w_buf[6];
	u8 read_buffer[2]; /* 52 */
	char buff[TSP_CMD_STR_LEN] = {0};
	int gpio;
	int ret;
	int i, j;
	u32 max_value, min_value;
	u32 raw_data;
	gpio = msm_irq_to_gpio(ts->irq);
	printk(KERN_ERR "[TSP] %s disable IRQ( %d)\n", __func__, __LINE__);
	disable_irq(ts->irq);

	w_buf[0] = MMS_VSC_CMD;		/* vendor specific command id */
	w_buf[1] = MMS_VSC_MODE;	/* mode of vendor */
	w_buf[2] = 0;			/* tx line */
	w_buf[3] = 0;			/* rx line */
	w_buf[4] = 0;			/* reserved */
	w_buf[5] = 0;			/* sub command */
	if (cmd == MMS_VSC_CMD_EXIT) {
		w_buf[5] = MMS_VSC_CMD_EXIT; /* exit test mode */
		ret = i2c_smbus_write_i2c_block_data(ts->client,
			w_buf[0], 5, &w_buf[1]);
		if (ret < 0)
			goto err_i2c;
		touch_is_pressed = 0;
		release_all_fingers(ts);
		msleep(50);
		ts_power_enable(0);
		msleep(500);
		ts_power_enable(1);
		msleep(300);
		printk(KERN_ERR "[TSP] %s enable IRQ( %d)\n"
			, __func__, __LINE__);
		enable_irq(ts->irq);
		return ;
	}
	/* MMS_VSC_CMD_CM_DELTA or MMS_VSC_CMD_CM_ABS
	 * this two mode need to enter the test mode
	 * exit command must be followed by testing.
	 */
	if (cmd == MMS_VSC_CMD_CM_DELTA || cmd == MMS_VSC_CMD_CM_ABS) {
		/* enter the debug mode */
		w_buf[2] = 0x0; /* tx */
		w_buf[3] = 0x0; /* rx */
		w_buf[5] = MMS_VSC_CMD_ENTER;
		ret = i2c_smbus_write_i2c_block_data(ts->client,
			w_buf[0], 5, &w_buf[1]);
		if (ret < 0)
			goto err_i2c;
		/* wating for the interrupt */
		while (gpio_get_value(gpio))
			udelay(100);
	}
	max_value = 0;
	min_value = 0;
	for (i = 0; i < RX_NUM; i++) {
		for (j = 0; j < TX_NUM; j++) {
			w_buf[2] = j; /* tx */
			w_buf[3] = i; /* rx */
			w_buf[5] = cmd;
			ret = i2c_smbus_write_i2c_block_data(ts->client,
					w_buf[0], 5, &w_buf[1]);
			if (ret < 0)
				goto err_i2c;
			usleep_range(1, 5);
			ret = i2c_smbus_read_i2c_block_data(ts->client, 0xBF,
					2, read_buffer);
			if (ret < 0)
				goto err_i2c;
			raw_data = ((u16)read_buffer[1] << 8) | read_buffer[0];

			if (i == 0 && j == 0) {
				max_value = min_value = raw_data;
			} else {
				max_value = max(max_value, raw_data);
				min_value = min(min_value, raw_data);
			}
			if (cmd == MMS_VSC_CMD_INTENSITY) {
				ts->intensity[j * RX_NUM + i] = raw_data;
				dev_dbg(&ts->client->dev, "[TSP] intensity[%d][%d] = %d\n",
					i, j, ts->intensity[j * RX_NUM + i]);
			} else if (cmd == MMS_VSC_CMD_CM_DELTA) {
				ts->inspection[j * RX_NUM + i] = raw_data;
				dev_dbg(&ts->client->dev, "[TSP] delta[%d][%d] = %d\n",
					i, j, ts->inspection[j * RX_NUM + i]);
			} else if (cmd == MMS_VSC_CMD_CM_ABS) {
				ts->raw[j * RX_NUM + i] = raw_data;
				dev_dbg(&ts->client->dev, "[TSP] raw[%d][%d] = %d\n",
					i, j, ts->raw[j * RX_NUM + i]);
			} else if (cmd == MMS_VSC_CMD_REFER) {
				ts->reference[j * RX_NUM + i] =
						raw_data >> 3;
				dev_dbg(&ts->client->dev, "[TSP] reference[%d][%d] = %d\n",
					i, j, ts->reference[j * RX_NUM + i]);
			}
		}
	}
	snprintf(buff, sizeof(buff), "%d,%d", min_value, max_value);
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));

	printk(KERN_ERR "[TSP] %s enable IRQ( %d)\n", __func__, __LINE__);
	enable_irq(ts->irq);
err_i2c:
	dev_err(&ts->client->dev, "%s: fail to i2c (cmd=%d)\n",
			__func__, cmd);
}
#if defined(ESD_DEBUG) || defined(SEC_TKEY_FACTORY_TEST)
static u32 get_raw_data_one(struct melfas_ts_data *ts, u16 rx_idx, u16 tx_idx,
		u8 cmd)
{
	u8 w_buf[6];
	u8 read_buffer[2];
	int ret;
	u32 raw_data;
	w_buf[0] = MMS_VSC_CMD;		/* vendor specific command id */
	w_buf[1] = MMS_VSC_MODE;	/* mode of vendor */
	w_buf[2] = 0;			/* tx line */
	w_buf[3] = 0;			/* rx line */
	w_buf[4] = 0;			/* reserved */
	w_buf[5] = 0;			/* sub command */
	if (cmd != MMS_VSC_CMD_INTENSITY && cmd != MMS_VSC_CMD_RAW &&
		cmd != MMS_VSC_CMD_REFER && cmd != VSC_INTENSITY_TK &&
		cmd != VSC_RAW_TK) {
		dev_err(&ts->client->dev, "%s: not profer command(cmd=%d)\n",
				__func__, cmd);
		return FAIL;
	}
	w_buf[2] = tx_idx;	/* tx */
	w_buf[3] = rx_idx;	/* rx */
	w_buf[5] = cmd;		/* sub command */
	ret = i2c_smbus_write_i2c_block_data(ts->client, w_buf[0], 5,
			&w_buf[1]);
	if (ret < 0)
		goto err_i2c;
	ret = i2c_smbus_read_i2c_block_data(ts->client, 0xBF, 2,
			read_buffer);
	if (ret < 0)
		goto err_i2c;
	raw_data = ((u16)read_buffer[1] << 8) | read_buffer[0];
	if (cmd == MMS_VSC_CMD_REFER)
		raw_data = raw_data >> 4;
	return raw_data;
err_i2c:
	dev_err(&ts->client->dev, "%s: fail to i2c (cmd=%d)\n",
			__func__, cmd);
	return FAIL;
}
#endif
static ssize_t show_close_tsp_test(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct melfas_ts_data *ts = dev_get_drvdata(dev);
	get_raw_data_all(ts, MMS_VSC_CMD_EXIT);
	ts->ft_flag = 0;

	return snprintf(buf, TSP_BUF_SIZE, "%u\n", 0);
}
static int check_rx_tx_num(void *device_data)
{
	struct melfas_ts_data *ts = (struct melfas_ts_data *)device_data;
	char buff[TSP_CMD_STR_LEN] = {0};
	int node;
	if (ts->cmd_param[0] < 0 ||
			ts->cmd_param[0] >= TX_NUM  ||
			ts->cmd_param[1] < 0 ||
			ts->cmd_param[1] >= RX_NUM) {
		snprintf(buff, sizeof(buff) , "%s", "NG");
		set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
		ts->cmd_state = 3;

		dev_info(&ts->client->dev, "%s: parameter error: %u,%u\n",
				__func__, ts->cmd_param[0],
				ts->cmd_param[1]);
		node = -1;
		return node;
	}
	node = ts->cmd_param[1] * TX_NUM + ts->cmd_param[0];
	dev_info(&ts->client->dev, "%s: node = %d\n", __func__,
			node);
	return node;
}
static void not_support_cmd(void *device_data)
{
	struct melfas_ts_data *ts = (struct melfas_ts_data *)device_data;
	char buff[16] = {0};
	set_default_result(ts);
	snprintf(buff, sizeof(buff), "%s", "NA");
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = 4;
	dev_info(&ts->client->dev, "%s: \"%s(%d)\"\n", __func__,
				buff, strnlen(buff, sizeof(buff)));
	return;
}
static void fw_update(void *device_data)
{
	struct melfas_ts_data *ts = (struct melfas_ts_data *)device_data;
	struct i2c_client *client = ts->client;
	int ret, i;
	//bool flag = false;
	
	printk(KERN_ERR "%s start.\n", __func__);
	set_default_result(ts);
	for (i = 0; i < DOWNLOAD_RETRY_CNT; i++) {

#if SET_DOWNLOAD_BY_GPIO
#if defined(SET_DOWNLOAD_CONFIG)
		ret = mms100_ISC_download_mbinary(client, false);//flag);
		ts_power_enable(0);
		mdelay(30);
		ts_power_enable(1);
		msleep(200);
#else    	// ICS mode for kyle 
		ret = MFS_ISC_update();
#endif		
#endif
		printk(KERN_ERR "mcsdl_download_binary_data : [%d]\n", ret);
		if (ret != 0)
			printk(KERN_ERR "SET Download Fail - error code [%d]\n",ret);
		else
			break;
	}
	ts->cmd_state = 2;
	return;
}

extern unsigned char melfas_hw_version;
static void get_fw_ver_bin(void *device_data)
{
	extern unsigned char TSP_PhoneVersion;
#if defined(CONFIG_MACH_ARUBASLIM_OPEN)
	extern unsigned char TSP_PhoneConfig;
#endif
	struct melfas_ts_data *ts = (struct melfas_ts_data *)device_data;
	int NEW_FIRMWARE_VERSION = TSP_PhoneVersion;
	char buff[16] = {0};
	set_default_result(ts);
#if defined(CONFIG_MACH_ARUBASLIM_OPEN)
	snprintf(buff, sizeof(buff), "ME%02x%02x%02x", melfas_hw_version, NEW_FIRMWARE_VERSION, TSP_PhoneConfig);
#else
	snprintf(buff, sizeof(buff), "ME%02x%04x", melfas_hw_version, NEW_FIRMWARE_VERSION);
#endif
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = 2;
	dev_info(&ts->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}
static void get_fw_ver_ic(void *device_data)
{
	extern unsigned char TSP_PanelVersion;
#if defined(CONFIG_MACH_ARUBASLIM_OPEN)
	extern unsigned char TSP_PanelConfig;
#endif
	struct melfas_ts_data *ts = (struct melfas_ts_data *)device_data;
	char buff[16] = {0};
	int ver;
	ts->fw_ic_ver = TSP_PanelVersion;
	set_default_result(ts);
	ver = ts->fw_ic_ver;
#if defined(CONFIG_MACH_ARUBASLIM_OPEN)
	snprintf(buff, sizeof(buff), "ME%02x%02x%02x", melfas_hw_version, ver, TSP_PanelConfig);
#else
	snprintf(buff, sizeof(buff), "ME%02x%04x", melfas_hw_version, ver);
#endif
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = 2;
	dev_info(&ts->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}
static void get_config_ver(void *device_data)
{
	struct melfas_ts_data *ts = (struct melfas_ts_data *)device_data;
	char buff[20] = {0};
	extern unsigned char TSP_PanelConfig, TSP_PhoneConfig;

	set_default_result(ts);

	
#if defined(MELFAS_MMS134S) || defined(MELFAS_MMS128S)
	snprintf(buff, sizeof(buff), "%s%", ts->config_fw_version );
#else
	snprintf(buff, sizeof(buff), "%s%02X%02X", ts->config_fw_version,TSP_PhoneConfig, TSP_PanelConfig );
#endif
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = 2;
	dev_info(&ts->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}
static void get_threshold(void *device_data)
{
	struct melfas_ts_data *ts = (struct melfas_ts_data *)device_data;
	char buff[16] = {0};
	int threshold;	//TSP

#if defined(CONFIG_MACH_INFINITE_DUOS_CTC)
	threshold = 35;
#elif defined(CONFIG_MACH_KYLEPLUS_CTC) || defined(CONFIG_MACH_KYLEPLUS_OPEN)
	threshold = 30;
#elif defined(MELFAS_MMS136_4P66) || defined(MELFAS_MMS136_4P3)
	threshold = 40;	
#elif defined(CONFIG_MACH_NEVIS3G)
	threshold = 20;
#else
	threshold = 30;
#endif
	set_default_result(ts);
	/*
	melfas_i2c_read(ts->client, P5_THRESHOLD, 1, &threshold);
	*/
	if (threshold < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
		ts->cmd_state = 3;
		return;
	}
	snprintf(buff, sizeof(buff), "%d", threshold);
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = 2;
	dev_info(&ts->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}
static void module_off_master(void *device_data)
{
	struct melfas_ts_data *ts = (struct melfas_ts_data *)device_data;
	char buff[3] = {0};

	tsp_enabled = false;
	printk(KERN_ERR "[TSP] %s disable IRQ( %d)\n", __func__, __LINE__);
	disable_irq(ts->client->irq);
	release_all_fingers(ts);
		touch_is_pressed = 0;
	ts_power_enable(0);
	
	snprintf(buff, sizeof(buff), "%s", "OK");
	set_default_result(ts);
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	if (strncmp(buff, "OK", 2) == 0)
		ts->cmd_state = 2;
	else
		ts->cmd_state = 3;
	dev_info(&ts->client->dev, "%s: %s\n", __func__, buff);
}
static void module_on_master(void *device_data)
{
	struct melfas_ts_data *ts = (struct melfas_ts_data *)device_data;
	char buff[3] = {0};

	ts_power_enable(1);
	msleep(50);

	tsp_enabled = true;
	printk(KERN_ERR "[TSP] %s enable IRQ( %d)\n", __func__, __LINE__);
	enable_irq(ts->client->irq);

	snprintf(buff, sizeof(buff), "%s", "OK");
	set_default_result(ts);
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	if (strncmp(buff, "OK", 2) == 0)
		ts->cmd_state = 2;
	else
		ts->cmd_state = 3;
	dev_info(&ts->client->dev, "%s: %s\n", __func__, buff);
}
static void get_chip_vendor(void *device_data)
{
	struct melfas_ts_data *ts = (struct melfas_ts_data *)device_data;
	char buff[16] = {0};
	set_default_result(ts);
	snprintf(buff, sizeof(buff), "%s", "MELFAS");
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = 2;
	dev_info(&ts->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}
static void get_chip_name(void *device_data)
{
	struct melfas_ts_data *ts = (struct melfas_ts_data *)device_data;
	char buff[16] = {0};
	set_default_result(ts);
	snprintf(buff, sizeof(buff), "%s", "MMS136");
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = 2;
	dev_info(&ts->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}
static void get_reference(void *device_data)
{
	struct melfas_ts_data *ts = (struct melfas_ts_data *)device_data;
	char buff[16] = {0};
	unsigned int val;
	int node;
	set_default_result(ts);
	node = check_rx_tx_num(ts);
	if (node < 0)
		return ;
	val = ts->reference[node];
	snprintf(buff, sizeof(buff), "%u", val);
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = 2;
	dev_info(&ts->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}
static void get_cm_abs(void *device_data)
{
	struct melfas_ts_data *ts = (struct melfas_ts_data *)device_data;
	char buff[16] = {0};
	unsigned int val;
	int node;
	set_default_result(ts);
	node = check_rx_tx_num(ts);
	if (node < 0)
		return;
	val = ts->raw[node];
	snprintf(buff, sizeof(buff), "%u", val);
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = 2;
	dev_info(&ts->client->dev, "%s: %s(%d)\n", __func__, buff,
			strnlen(buff, sizeof(buff)));
	}
static void get_cm_delta(void *device_data)
{
	struct melfas_ts_data *ts = (struct melfas_ts_data *)device_data;
	char buff[16] = {0};
	unsigned int val;
	int node;
	set_default_result(ts);
	node = check_rx_tx_num(ts);
	if (node < 0)
		return;
	val = ts->inspection[node];
	snprintf(buff, sizeof(buff), "%u", val);
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = 2;
	dev_info(&ts->client->dev, "%s: %s(%d)\n", __func__, buff,
			strnlen(buff, sizeof(buff)));
}
/* CM ABS */
static int check_debug_value(struct melfas_ts_data *ts)
{
	u8 setLowLevelData[4];
	u8 read_data_buf[50] = {0,};
	//u16 read_data_buf1[50] = {0,};
	char buff[TSP_CMD_STR_LEN] = {0};
	//int read_data_len;
	int sensing_ch, exciting_ch;
	int ret, i, j, status=0;
	int size;
	u32 max_value, min_value;
	u32 raw_data;
	tsp_testmode = 1;
	
	printk(KERN_ERR "[TSP] %s entered. line : %d\n", __func__, __LINE__);

	printk(KERN_ERR "[TSP] %s disable IRQ( %d)\n", __func__, __LINE__);
	disable_irq(ts->client->irq);

	exciting_ch = g_exciting_ch;
	sensing_ch = g_sensing_ch;
	/* Read Reference Data */
	setLowLevelData[0] = 0xA0; /* UNIVERSAL_CMD */
	setLowLevelData[1] = 0x40; /* UNIVCMD_ENTER_TEST_MODE */
	melfas_i2c_write(ts->client, setLowLevelData, 2);

#if defined(MELFAS_MMS128S) || defined(MELFAS_MMS134S)
	while (gpio_get_value(GPIO_TOUCH_INT))
		;
	melfas_i2c_read(ts->client, 0xAE, 1, read_data_buf);
#else	// insert by melfas klski98
	{
	/* event type check */
		int retry =1;
		u8 r_buf;

		while (retry) {
			while (gpio_get_value(GPIO_TOUCH_INT))
				udelay(100);

			ret = i2c_smbus_read_i2c_block_data(ts->client, 0x0F, 1, &r_buf);
			if (ret < 0)
				goto err_i2c;

			ret = i2c_smbus_read_i2c_block_data(ts->client,	0x10, 1, &r_buf);
			if (ret < 0)
				goto err_i2c;

			dev_info(&ts->client->dev, "event type = 0x%x\n", r_buf);
			if (r_buf == 0x0C)
				retry = 0;
		}
	}
#endif

	printk(KERN_ERR "\n\n --- CM_ABS --- ");
	/* Read Reference Data */
	setLowLevelData[0] = 0xA0;
	setLowLevelData[1] = 0x43; /* UNIVCMD_TEST_CM_ABS */
	melfas_i2c_write(ts->client, setLowLevelData, 2);
	while (gpio_get_value(GPIO_TOUCH_INT))
		;

	ret = melfas_i2c_read(ts->client, 0xAE, 1, read_data_buf);
	printk(KERN_ERR "[TSP] %s ret= %d\n", __func__, ret);
	max_value = 0;
	min_value = 0;
	for (i = 0; i < sensing_ch; i++) {
		for (j = 0; j < exciting_ch; j++) {
			setLowLevelData[0] = 0xA0;
			setLowLevelData[1] = 0x44;
			setLowLevelData[2] = j;
			setLowLevelData[3] = i;
			melfas_i2c_write(ts->client, setLowLevelData, 4);
			while (gpio_get_value(GPIO_TOUCH_INT))
				;

			melfas_i2c_read(ts->client, 0xAE,
				1, read_data_buf);
			size = read_data_buf[0];
			melfas_i2c_read(ts->client, 0xAF,
				size, read_data_buf);
			CmABS_data[(i * exciting_ch) + j]
				= (read_data_buf[0] |  read_data_buf[1] << 8);
			raw_data = CmABS_data[(i * exciting_ch) + j];
			ts->raw[(i * exciting_ch) + j] = raw_data;
			if (i == 0 && j == 0) {
				max_value = min_value = raw_data;
			} else {
				max_value = max(max_value, raw_data);
				min_value = min(min_value, raw_data);
			}
			if ((CmABS_data[(i * exciting_ch) + j]
					>= SCR_ABS_LOWER_SPEC[i][j])
				&& (CmABS_data[(i * exciting_ch) + j]
					<= SCR_ABS_UPPER_SPEC[i][j]))
				status = 1; /* fail */
			else
				status = 0; /* pass */
		}
	}
	printk(KERN_ERR "[TSP] CmABS_data\n");
	for (i = 0; i < exciting_ch * sensing_ch; i++) {
		if (0 == i % exciting_ch)
			printk(KERN_INFO "\n");
		printk(KERN_ERR "%4d, ", CmABS_data[i]);
	}
	printk(KERN_INFO "\n");
	snprintf(buff, sizeof(buff), "%d,%d", min_value, max_value);
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	/* Read Reference Data */
	setLowLevelData[0] = 0xA0;
	setLowLevelData[1] = 0x4F;
	melfas_i2c_write(ts->client, setLowLevelData, 2);

	printk(KERN_ERR "[TSP] %s enable IRQ( %d)\n", __func__, __LINE__);
	enable_irq(ts->client->irq);
	tsp_testmode = 0;
	TSP_reboot();
	printk(KERN_ERR "%s : end %d\n", __func__,status);
	return status;

	err_i2c:
	printk(KERN_ERR "%s : fail to i2c, line %d\n", __func__, __LINE__);
	return status;
}
static int check_delta_value(struct melfas_ts_data *ts)
{
	u8 setLowLevelData[4];
	u8 read_data_buf[50] = {0,};
	//u16 read_data_buf1[50] = {0,};
	char buff[TSP_CMD_STR_LEN] = {0};
	//int read_data_len;
	int sensing_ch, exciting_ch;
	int i, j, status=0;
	int size;
	u32 max_value, min_value;
	u32 raw_data;

		
	printk(KERN_ERR "[TSP] %s entered. line : %d,\n", __func__, __LINE__);

	printk(KERN_ERR "[TSP] %s disable IRQ( %d)\n", __func__, __LINE__);
	disable_irq(ts->client->irq);
	exciting_ch = g_exciting_ch;
	sensing_ch	 = g_sensing_ch;
	/* Read Reference Data */
	setLowLevelData[0] = 0xA0; /* UNIVERSAL_CMD */
	setLowLevelData[1] = 0x40; /* UNIVCMD_ENTER_TEST_MODE */
	melfas_i2c_write(ts->client, setLowLevelData, 2);
#if defined(MELFAS_MMS128S) || defined(MELFAS_MMS134S)
	while (gpio_get_value(GPIO_TOUCH_INT))
		;

	melfas_i2c_read(ts->client, 0xAE, 1, read_data_buf);
#else	// insert by melfas klski98
	/* event type check */
	{
	int ret;
	int retry = 1;
	u8 r_buf = 0;

	while (retry) {
		while (gpio_get_value(GPIO_TOUCH_INT))
			udelay(100);

		ret = i2c_smbus_read_i2c_block_data(ts->client, 0x0F, 1, &r_buf);
		if (ret < 0)
			goto err_i2c;

		ret = i2c_smbus_read_i2c_block_data(ts->client,	0x10, 1, &r_buf);
		if (ret < 0)
			goto err_i2c;

		dev_info(&ts->client->dev, "event type = 0x%x\n", r_buf);
		if (r_buf == 0x0C)
			retry = 0;
	}
	}	
#endif 
	printk(KERN_ERR "\n\n --- CM_DELTA --- ");
	/* Read Reference Data */
	setLowLevelData[0] = 0xA0;
	setLowLevelData[1] = 0x41;
	melfas_i2c_write(ts->client, setLowLevelData, 2);
	while (gpio_get_value(GPIO_TOUCH_INT))
		;

	melfas_i2c_read(ts->client, 0xAE, 1, read_data_buf);
	max_value = 0;
	min_value = 0;
	for (i = 0; i < sensing_ch; i++) {
		for (j = 0; j < exciting_ch; j++) {
			setLowLevelData[0] = 0xA0;
			setLowLevelData[1] = 0x42;
			setLowLevelData[2] = j; /* Exciting CH. */
			setLowLevelData[3] = i; /* Sensing CH. */
			melfas_i2c_write(ts->client, setLowLevelData, 4);
			while (gpio_get_value(GPIO_TOUCH_INT))
				;

			melfas_i2c_read(ts->client, 0xAE,
				1, read_data_buf);
			size = read_data_buf[0];
			melfas_i2c_read(ts->client, 0xAF,
				size, read_data_buf);
			CmDelta_data[(i * exciting_ch) + j]
				= (read_data_buf[0] |  read_data_buf[1] << 8);
			raw_data = CmDelta_data[(i * exciting_ch) + j];
			ts->inspection[(i * exciting_ch) + j] = raw_data;
			if (i == 0 && j == 0) {
				max_value = min_value = raw_data;
			} else {
				max_value = max(max_value, raw_data);
				min_value = min(min_value, raw_data);
			}
		}
	}
	printk(KERN_ERR "[TSP] CmDelta_data\n");
	for (i = 0; i < exciting_ch * sensing_ch; i++) {
		if (0 == i % exciting_ch)
			printk(KERN_INFO "\n");
		printk(KERN_ERR "%4d, ", CmDelta_data[i]);
	}
	printk(KERN_ERR "min:%d,max:%d", min_value, max_value);
	printk(KERN_INFO "\n");
	snprintf(buff, sizeof(buff), "%d,%d", min_value, max_value);
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	/* Read Reference Data */
	setLowLevelData[0] = 0xA0;
	setLowLevelData[1] = 0x4F;
	melfas_i2c_write(ts->client, setLowLevelData, 2);
	printk(KERN_ERR "[TSP] %s enable IRQ( %d)\n", __func__, __LINE__);
	enable_irq(ts->client->irq);
	tsp_testmode = 0;
	TSP_reboot();
	printk(KERN_ERR "%s : end\n", __func__);
	return status;

	err_i2c:
	printk(KERN_ERR "%s : fail to i2c, line %d\n", __func__, __LINE__);
	return status;

}
static void get_intensity(void *device_data)
{
	struct melfas_ts_data *ts = (struct melfas_ts_data *)device_data;

	char buff[16] = {0};
	unsigned int val;
	int node;
	set_default_result(ts);
	node = check_rx_tx_num(ts);
	if (node < 0)
		return ;
	val = ts->intensity[node];
	snprintf(buff, sizeof(buff), "%u", val);
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = 2;
	dev_info(&ts->client->dev, "%s: %s(%d)\n", __func__, buff,
			strnlen(buff, sizeof(buff)));
}
static void get_x_num(void *device_data)
{
	struct melfas_ts_data *ts = (struct melfas_ts_data *)device_data;
	char buff[16] = {0};
	int val;
	int exciting_ch;
	set_default_result(ts);
/*
	val = i2c_smbus_read_byte_data(ts->client, 0xEF);
*/
	exciting_ch = g_exciting_ch;
	val = exciting_ch;
	if (val < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
		ts->cmd_state = 3;
		dev_info(&ts->client->dev,
			"%s: fail to read num of x (%d).\n", __func__, val);
		return ;
	}
	snprintf(buff, sizeof(buff), "%d", val);
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = 2;
	dev_info(&ts->client->dev, "%s: %s(%d)\n", __func__, buff,
			strnlen(buff, sizeof(buff)));
}
static void get_y_num(void *device_data)
{
	struct melfas_ts_data *ts = (struct melfas_ts_data *)device_data;
	char buff[16] = {0};
	int val;
	int sensing_ch;
	set_default_result(ts);
/*
	val = i2c_smbus_read_byte_data(ts->client, 0xEE);
*/
	sensing_ch = g_sensing_ch;
	val = sensing_ch;
	if (val < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
		ts->cmd_state = 3;
		dev_info(&ts->client->dev,
			"%s: fail to read num of y (%d).\n", __func__, val);
		return ;
	}
	snprintf(buff, sizeof(buff), "%d", val);
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = 2;
	dev_info(&ts->client->dev, "%s: %s(%d)\n", __func__, buff,
			strnlen(buff, sizeof(buff)));
}
static void run_reference_read(void *device_data)
{
	struct melfas_ts_data *ts = (struct melfas_ts_data *)device_data;
	set_default_result(ts);
	get_raw_data_all(ts, MMS_VSC_CMD_REFER);
	ts->cmd_state = 2;
	dev_info(&ts->client->dev, "%s:\n", __func__);
}
static void run_cm_abs_read(void *device_data)
{
	struct melfas_ts_data *ts = (struct melfas_ts_data *)device_data;
	set_default_result(ts);
	check_debug_value(ts);
	ts->cmd_state = 2;
	dev_info(&ts->client->dev, "%s:\n", __func__);
}
static void run_cm_delta_read(void *device_data)
{
	struct melfas_ts_data *ts = (struct melfas_ts_data *)device_data;
	set_default_result(ts);
	check_delta_value(ts);
	ts->cmd_state = 2;
	dev_info(&ts->client->dev, "%s:\n", __func__);
}
static void run_intensity_read(void *device_data)
{
	struct melfas_ts_data *ts = (struct melfas_ts_data *)device_data;
	set_default_result(ts);
	get_raw_data_all(ts, MMS_VSC_CMD_INTENSITY);
	ts->cmd_state = 2;
	dev_info(&ts->client->dev, "%s:\n", __func__);
}
static ssize_t store_cmd(struct device *dev, struct device_attribute
		*devattr, const char *buf, size_t count)
{
	struct melfas_ts_data *ts = dev_get_drvdata(dev);
	struct i2c_client *client = ts->client;
	char *cur, *start, *end;
	char buff[TSP_CMD_STR_LEN] = {0};
	int len, i;
	struct tsp_cmd *tsp_cmd_ptr = NULL;
	char delim = ',';
	bool cmd_found = false;
	int param_cnt = 0;
	if (ts->cmd_is_running == true) {
		dev_err(&ts->client->dev, "tsp_cmd: other cmd is running.\n");
		goto err_out;
	}
	/* check lock  */
	mutex_lock(&ts->cmd_lock);
	ts->cmd_is_running = true;
	mutex_unlock(&ts->cmd_lock);
	ts->cmd_state = 1;
	for (i = 0; i < ARRAY_SIZE(ts->cmd_param); i++)
		ts->cmd_param[i] = 0;

	len = (int)count;
	if (*(buf + len - 1) == '\n')
		len--;
	memset(ts->cmd, 0x00, ARRAY_SIZE(ts->cmd));
	memcpy(ts->cmd, buf, len);
	cur = strchr(buf, (int)delim);
	if (cur)
		memcpy(buff, buf, cur - buf);
	else
		memcpy(buff, buf, len);
	/* find command */
	list_for_each_entry(tsp_cmd_ptr, &ts->cmd_list_head, list) {
		if (!strcmp(buff, tsp_cmd_ptr->cmd_name)) {
			cmd_found = true;
			break;
		}
	}
	/* set not_support_cmd */
	if (!cmd_found) {
		list_for_each_entry(tsp_cmd_ptr, &ts->cmd_list_head, list) {
			if (!strcmp("not_support_cmd", tsp_cmd_ptr->cmd_name))
				break;
		}
	}
	/* parsing parameters */
	if (cur && cmd_found) {
		cur++;
		start = cur;
		memset(buff, 0x00, ARRAY_SIZE(buff));
		do {
			if (*cur == delim || cur - buf == len) {
				end = cur;
				memcpy(buff, start, end - start);
				*(buff + strlen(buff)) = '\0';
				if (kstrtoint(buff, 10,
					ts->cmd_param + param_cnt) < 0)
					goto err_out;
				start = cur + 1;
				memset(buff, 0x00, ARRAY_SIZE(buff));
				param_cnt++;
			}
			cur++;
		} while (cur - buf <= len);
	}
	dev_info(&client->dev, "cmd = %s\n", tsp_cmd_ptr->cmd_name);
	for (i = 0; i < param_cnt; i++)
		dev_info(&client->dev, "cmd param %d= %d\n", i,
							ts->cmd_param[i]);
	/*for*/
	tsp_cmd_ptr->cmd_func(ts);
err_out:
	return count;
}
static ssize_t show_cmd_status(struct device *dev,
		struct device_attribute *devattr, char *buf)
{
	struct melfas_ts_data *ts = dev_get_drvdata(dev);
	char buff[16] = {0};
	dev_info(&ts->client->dev, "tsp cmd: status:%d\n",
			ts->cmd_state);
	if (ts->cmd_state == 0)
		snprintf(buff, sizeof(buff), "WAITING");
	else if (ts->cmd_state == 1)
		snprintf(buff, sizeof(buff), "RUNNING");
	else if (ts->cmd_state == 2)
		snprintf(buff, sizeof(buff), "OK");
	else if (ts->cmd_state == 3)
		snprintf(buff, sizeof(buff), "FAIL");
	else if (ts->cmd_state == 4)
		snprintf(buff, sizeof(buff), "NOT_APPLICABLE");
	else
		snprintf(buff, sizeof(buff), "%d", ts->cmd_state);
	return snprintf(buf, TSP_BUF_SIZE, "%s\n", buff);
}
static ssize_t show_cmd_result(struct device *dev, struct device_attribute
		*devattr, char *buf)
{
	struct melfas_ts_data *ts = dev_get_drvdata(dev);
	dev_info(&ts->client->dev, "tsp cmd: result: %s\n", ts->cmd_result);
	mutex_lock(&ts->cmd_lock);
	ts->cmd_is_running = false;
	mutex_unlock(&ts->cmd_lock);
	ts->cmd_state = 0;
	return snprintf(buf, TSP_BUF_SIZE, "%s\n", ts->cmd_result);
}
#ifdef ESD_DEBUG
static bool intensity_log_flag;
static ssize_t show_intensity_logging_on(struct device *dev,
		struct device_attribute *devattr, char *buf)
{
	struct melfas_ts_data *ts = dev_get_drvdata(dev);
	struct i2c_client *client = ts->client;
	struct file *fp;
	char log_data[160] = {0,};
	char buff[16] = {0,};
	mm_segment_t old_fs;
	long nwrite;
	u32 val;
	int i, y, c;
	old_fs = get_fs();
	set_fs(KERNEL_DS);
#define MELFAS_DEBUG_LOG_PATH "/sdcard/melfas_log"
	dev_info(&client->dev, "%s: start.\n", __func__);
	fp = filp_open(MELFAS_DEBUG_LOG_PATH, O_RDWR|O_CREAT,
			S_IRWXU|S_IRWXG|S_IRWXO);
	if (IS_ERR(fp)) {
		dev_err(&client->dev, "%s: fail to open log file\n", __func__);
		goto open_err;
	}
	intensity_log_flag = 1;
	do {
		for (y = 0; y < 3; y++) {
			/* for tx chanel 0~2 */
			memset(log_data, 0x00, 160);
			snprintf(buff, 16, "%1d: ", y);
			strncat(log_data, buff, strnlen(buff, 16));
			for (i = 0; i < RX_NUM; i++) {
				val = get_raw_data_one(ts, i, y,
						MMS_VSC_CMD_INTENSITY);
				snprintf(buff, 16, "%5u, ", val);
				strncat(log_data, buff, strnlen(buff, 16));
			}
			memset(buff, '\n', 2);
			c = (y == 2) ? 2 : 1;
			strncat(log_data, buff, c);
			nwrite = vfs_write(fp, (const char __user *)log_data,
					strnlen(log_data, 160), &fp->f_pos);
		}
		usleep_range(5000);
	} while (intensity_log_flag);
	filp_close(fp, current->files);
	set_fs(old_fs);
	return 0;
 open_err:
	set_fs(old_fs);
	return FAIL;
}
static ssize_t show_intensity_logging_off(struct device *dev,
		struct device_attribute *devattr, char *buf)
{
	struct melfas_ts_data *ts = dev_get_drvdata(dev);
	intensity_log_flag = 0;
	usleep_range(10000);
	get_raw_data_all(ts, MMS_VSC_CMD_EXIT);
	return 0;
}
#endif
#ifdef SEC_TKEY_FACTORY_TEST
static ssize_t tkey_threshold_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct melfas_ts_data *ts = dev_get_drvdata(dev);
	struct i2c_client *client = ts->client;
	int tkey_threshold;
	tkey_threshold = i2c_smbus_read_byte_data(ts->client,
						VSC_THRESHOLD_TK);
	dev_info(&client->dev, "touch key threshold: %d\n", tkey_threshold);

	return snprintf(buf, sizeof(int), "%d\n", tkey_threshold);
}
static ssize_t back_key_state_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct melfas_ts_data *ts = dev_get_drvdata(dev);
	struct i2c_client *client = ts->client;
	int i, ret, val;
	for (i = 0; i < ARRAY_SIZE(ts->keycode); i++) {
		if (ts->keycode[i] == KEY_BACK)
			break;
	}
	dev_info(&client->dev, "back key state: %d\n", ts->key_pressed[i]);
	/* back key*/
	printk(KERN_ERR "[TSP] %s disable IRQ( %d)\n", __func__, __LINE__);
	disable_irq(ts->irq);

	ret = get_raw_data_one(ts, 0, 0, VSC_INTENSITY_TK);
	if (ret < 0)
		dev_err(&client->dev, "%s: fail to read (%d)\n", __func__, ret);

	printk(KERN_ERR "[TSP] %s enable IRQ( %d)\n", __func__, __LINE__);
	enable_irq(ts->irq);
	val = (u16)ret;
	dev_info(&client->dev, "%s: val=%d\n", __func__, val);
	return snprintf(buf, sizeof(buf), "%d\n", val);
}
static ssize_t home_key_state_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct melfas_ts_data *ts = dev_get_drvdata(dev);
	struct i2c_client *client = ts->client;
	int i, ret, val;
	for (i = 0; i < ARRAY_SIZE(ts->keycode); i++) {
		if (ts->keycode[i] == KEY_HOMEPAGE)
			break;
	}
	dev_info(&client->dev, "back key state: %d\n", ts->key_pressed[i]);
	/* home key*/
	printk(KERN_ERR "[TSP] %s disable IRQ( %d)\n", __func__, __LINE__);
	disable_irq(ts->irq);

	ret = get_raw_data_one(ts, 0, 1, VSC_INTENSITY_TK);
	if (ret < 0)
		dev_err(&client->dev, "%s: fail to read (%d)\n", __func__, ret);

	printk(KERN_ERR "[TSP] %s enable IRQ( %d)\n", __func__, __LINE__);
	enable_irq(ts->irq);
	val = (u16)ret;
	dev_info(&client->dev, "%s: val=%d\n", __func__, val);
	return snprintf(buf, sizeof(buf), "%d\n", val);
}
static ssize_t recent_key_state_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct melfas_ts_data *ts = dev_get_drvdata(dev);
	struct i2c_client *client = ts->client;
	int i, ret, val;
	for (i = 0; i < ARRAY_SIZE(ts->keycode); i++) {
		if (ts->keycode[i] == KEY_F3)
			break;
	}
	dev_info(&client->dev, "back key state: %d\n", ts->key_pressed[i]);
	/* recent key*/
	printk(KERN_ERR "[TSP] %s disable IRQ( %d)\n", __func__, __LINE__);
	disable_irq(ts->irq);

	ret = get_raw_data_one(ts, 0, 2, VSC_INTENSITY_TK);
	if (ret < 0)
		dev_err(&client->dev, "%s: fail to read (%d)\n", __func__, ret);

	printk(KERN_ERR "[TSP] %s enable IRQ( %d)\n", __func__, __LINE__);
	enable_irq(ts->irq);
	val = (u16)ret;
	dev_info(&client->dev, "%s: val=%d\n", __func__, val);
	return snprintf(buf, sizeof(buf), "%d\n", val);
}
static ssize_t menu_key_state_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct melfas_ts_data *ts = dev_get_drvdata(dev);
	struct i2c_client *client = ts->client;
	int i, ret, val;
	for (i = 0; i < ARRAY_SIZE(ts->keycode); i++) {
		if (ts->keycode[i] == KEY_MENU)
			break;
	}
	dev_info(&client->dev, "back key state: %d\n", ts->key_pressed[i]);
	/* recent key*/
	printk(KERN_ERR "[TSP] %s disable IRQ( %d)\n", __func__, __LINE__);
	disable_irq(ts->irq);

	ret = get_raw_data_one(ts, 0, 3, VSC_INTENSITY_TK);
	if (ret < 0)
		dev_err(&client->dev, "%s: fail to read (%d)\n", __func__, ret);

	printk(KERN_ERR "[TSP] %s enable IRQ( %d)\n", __func__, __LINE__);
	enable_irq(ts->irq);
	val = (u16)ret;
	dev_info(&client->dev, "%s: val=%d\n", __func__, val);
	return snprintf(buf, sizeof(buf), "%d\n", val);
}
static ssize_t tkey_rawcounter_show0(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct melfas_ts_data *ts = dev_get_drvdata(dev);
	struct i2c_client *client = ts->client;
	u32 ret;
	u16 val;
	/* back key*/
	printk(KERN_ERR "[TSP] %s disable IRQ( %d)\n", __func__, __LINE__);
	disable_irq(ts->irq);

	ret = get_raw_data_one(ts, 0, 0, VSC_RAW_TK);
	if (ret < 0)
		dev_err(&client->dev, "%s: fail to read (%d)\n", __func__, ret);

	printk(KERN_ERR "[TSP] %s enable IRQ( %d)\n", __func__, __LINE__);
	enable_irq(ts->irq);
	val = (u16)ret;
	dev_info(&client->dev, "%s: val=%d\n", __func__, val);
	return snprintf(buf, sizeof(buf), "%d\n", val);
}
static ssize_t tkey_rawcounter_show1(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct melfas_ts_data *ts = dev_get_drvdata(dev);
	struct i2c_client *client = ts->client;
	int ret;
	u16 val;
	/* home key*/
	printk(KERN_ERR "[TSP] %s disable IRQ( %d)\n", __func__, __LINE__);
	disable_irq(ts->irq);

	ret = get_raw_data_one(ts, 0, 1, VSC_RAW_TK);
	if (ret < 0)
		dev_err(&client->dev, "%s: fail to read (%d)\n", __func__, ret);

	printk(KERN_ERR "[TSP] %s enable IRQ( %d)\n", __func__, __LINE__);
	enable_irq(ts->irq);
	val = (u16)ret;
	dev_info(&client->dev, "%s: val=%d\n", __func__, val);
	return snprintf(buf, sizeof(buf), "%d\n", val);
}
static ssize_t tkey_rawcounter_show2(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct melfas_ts_data *ts = dev_get_drvdata(dev);
	struct i2c_client *client = ts->client;
	int ret;
	u16 val;
	/* recent key*/
	printk(KERN_ERR "[TSP] %s disable IRQ( %d)\n", __func__, __LINE__);
	disable_irq(ts->irq);

	ret = get_raw_data_one(ts, 0, 2, VSC_RAW_TK);
	if (ret < 0)
		dev_err(&client->dev, "%s: fail to read (%d)\n", __func__, ret);

	printk(KERN_ERR "[TSP] %s enable IRQ( %d)\n", __func__, __LINE__);
	enable_irq(ts->irq);
	val = (u16)ret;
	dev_info(&client->dev, "%s: val=%d\n", __func__, val);
	return snprintf(buf, sizeof(buf), "%d\n", val);
}
static ssize_t tkey_rawcounter_show3(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct melfas_ts_data *ts = dev_get_drvdata(dev);
	struct i2c_client *client = ts->client;
	int ret;
	u16 val;
	/* menu key*/
	printk(KERN_ERR "[TSP] %s disable IRQ( %d)\n", __func__, __LINE__);
	disable_irq(ts->irq);

	ret = get_raw_data_one(ts, 0, 3, VSC_RAW_TK);
	if (ret < 0)
		dev_err(&client->dev, "%s: fail to read (%d)\n", __func__, ret);

	printk(KERN_ERR "[TSP] %s enable IRQ( %d)\n", __func__, __LINE__);
	enable_irq(ts->irq);
	val = (u16)ret;
	dev_info(&client->dev, "%s: val=%d\n", __func__, val);
	return snprintf(buf, sizeof(buf), "%d\n", val);
}
#endif
#ifdef SEC_TKEY_FACTORY_TEST
static DEVICE_ATTR(touchkey_threshold, S_IRUGO, tkey_threshold_show, NULL);
static DEVICE_ATTR(touchkey_back, S_IRUGO, back_key_state_show, NULL);
static DEVICE_ATTR(touchkey_home, S_IRUGO, home_key_state_show, NULL);
static DEVICE_ATTR(touchkey_recent, S_IRUGO, recent_key_state_show, NULL);
static DEVICE_ATTR(touchkey_menu, S_IRUGO, menu_key_state_show, NULL);
static DEVICE_ATTR(touchkey_raw_data0, S_IRUGO, tkey_rawcounter_show0, NULL);
static DEVICE_ATTR(touchkey_raw_data1, S_IRUGO, tkey_rawcounter_show1, NULL);
static DEVICE_ATTR(touchkey_raw_data2, S_IRUGO, tkey_rawcounter_show2, NULL);
static DEVICE_ATTR(touchkey_raw_data3, S_IRUGO, tkey_rawcounter_show3, NULL);
static struct attribute *touchkey_attributes[] = {
	&dev_attr_touchkey_threshold.attr,
	&dev_attr_touchkey_back.attr,
	&dev_attr_touchkey_home.attr,
	&dev_attr_touchkey_recent.attr,
	&dev_attr_touchkey_menu.attr,
	&dev_attr_touchkey_raw_data0.attr,
	&dev_attr_touchkey_raw_data1.attr,
	&dev_attr_touchkey_raw_data2.attr,
	&dev_attr_touchkey_raw_data3.attr,
	NULL,
};
static struct attribute_group touchkey_attr_group = {
	.attrs = touchkey_attributes,
};
static int factory_init_tk(struct melfas_ts_data *ts)
{
	struct i2c_client *client = ts->client;
	int ret;
	ts->dev_tk = device_create(sec_class, NULL, (dev_t)NULL, ts,
								"sec_touchkey");
	if (IS_ERR(ts->dev_tk)) {
		dev_err(&client->dev, "Failed to create fac touchkey dev\n");
		ret = -ENODEV;
		ts->dev_tk = NULL;
		goto err_create_dev_tk;
	}
	ret = sysfs_create_group(&ts->dev_tk->kobj, &touchkey_attr_group);
	if (ret) {
		dev_err(&client->dev,
			"Failed to create sysfs (touchkey_attr_group).\n");
		ret = (ret > 0) ? -ret : ret;
		goto err_create_tk_sysfs;
	}
	ts->key_pressed = kzalloc(sizeof(bool) * ARRAY_SIZE(ts->keycode),
								GFP_KERNEL);
	if (!ts->key_pressed) {
		dev_err(&client->dev, "Failed to allocate memory\n");
		ret = -ENOMEM;
		goto err_alloc;
	}
	return 0;
err_alloc:
	sysfs_remove_group(&ts->dev_tk->kobj, &touchkey_attr_group);
err_create_tk_sysfs:
err_create_dev_tk:
	return ret;
}
#endif
static DEVICE_ATTR(close_tsp_test, S_IRUGO, show_close_tsp_test, NULL);
static DEVICE_ATTR(cmd, S_IWUSR | S_IWGRP, NULL, store_cmd);
static DEVICE_ATTR(cmd_status, S_IRUGO, show_cmd_status, NULL);
static DEVICE_ATTR(cmd_result, S_IRUGO, show_cmd_result, NULL);
#ifdef ESD_DEBUG
static DEVICE_ATTR(intensity_logging_on, S_IRUGO, show_intensity_logging_on,
		NULL);
static DEVICE_ATTR(intensity_logging_off, S_IRUGO, show_intensity_logging_off,
		NULL);
#endif
static struct attribute *sec_touch_facotry_attributes[] = {
	&dev_attr_close_tsp_test.attr,
	&dev_attr_cmd.attr,
	&dev_attr_cmd_status.attr,
	&dev_attr_cmd_result.attr,
#ifdef ESD_DEBUG
	&dev_attr_intensity_logging_on.attr,
	&dev_attr_intensity_logging_off.attr,
#endif
	NULL,
};
static struct attribute_group sec_touch_factory_attr_group = {
	.attrs = sec_touch_facotry_attributes,
};
#endif /* SEC_TSP_FACTORY_TEST */

//static int tsp_reboot_count;
static int melfas_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
#if 0
	struct device *qt602240_noise_test;
#ifdef TA_DETECTION
	bool ta_status;
#endif
#endif
#ifdef SEC_TSP_FACTORY_TEST
	struct device *fac_dev_ts;
#endif
#ifdef TUNNING_SUPPORT
	dev_t mms_dev;
#endif
	int ret = 0, i;
	//bool flag = false;
	//uint8_t buf[6] = {0, };

#if DEBUG_PRINT
	printk(KERN_ERR "%s start.\n", __func__);
#endif
	tsp_enabled = false;

	ts_power_enable(0);
	msleep(60);
	ts_power_enable(1);
	msleep(60);

	gpio_tlmm_config(GPIO_CFG(GPIO_TSP_SDA, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(GPIO_TSP_SCL, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE);


#if defined(MELFAS_MMS136_4P66) 
	g_exciting_ch = 21;
	g_sensing_ch = 14;
#elif defined(MELFAS_MMS128S) 
	g_exciting_ch = 16;
	g_sensing_ch = 11;
#elif defined(MELFAS_MMS134S) 
	g_exciting_ch = 19;
	g_sensing_ch = 12;
#else
	g_exciting_ch = 20;
	g_sensing_ch = 12;
#endif

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk(KERN_ERR "%s: need I2C_FUNC_I2C\n", __func__);
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}
	ts = kmalloc(sizeof(struct melfas_ts_data), GFP_KERNEL);
	if (ts == NULL) {
	printk(KERN_ERR "%s: failed to create a state of melfas-ts\n", __func__);
	ret = -ENOMEM;
	goto err_alloc_data_failed;
	}
#if 0
	ts_data = ts;
	data = client->dev.platform_data;
	ts->power = data->power;
	ts->gpio = data->gpio;
	ts->version = data->version;
#ifdef TA_DETECTION
	ts->register_cb = data->register_cb;
	ts->read_ta_status = data->read_ta_status;
#endif
    ts->client = client;
    i2c_set_clientdata(client, ts);
	ts->power(true);
#endif
	ts->client = client;
	i2c_set_clientdata(client, ts);
	mdelay(200);
#if 1
	printk(KERN_ERR "%s: i2c_master_send() [%d], Add[%d]\n", __func__,
		ret, ts->client->addr);
#endif
#if SET_DOWNLOAD_BY_GPIO
#if 0
	buf[0] = TS_READ_VERSION_ADDR;
	ret = melfas_i2c_master_send(ts->client, &buf, 1);
	if (ret < 0) {
		printk(KERN_ERR "melfas_ts_work_func : i2c_master_send [%d]\n", ret);
	}
	ret = melfas_i2c_master_recv(ts->client, &buf, 3);
#endif
			gpio_tlmm_config(GPIO_CFG(GPIO_TSP_SDA, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
			gpio_tlmm_config(GPIO_CFG(GPIO_TSP_SCL, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE);

		for (i = 0; i < DOWNLOAD_RETRY_CNT; i++) {
#ifdef SET_DOWNLOAD_CONFIG
#ifdef SET_DOWNLOAD_ISP

			// ISP mode
			ts_power_enable(0);
			mdelay(30);
			ts_power_enable(1);
			mdelay(200);
			ret = mms_flash_fw(client);//, mms_pdata);

#else		// ICS mode
			ret = mms100_ISC_download_mbinary(client, false);// flag);
			ts_power_enable(0);
			msleep(60); // mdelay(30);
			ts_power_enable(1);
			msleep(200);

#endif		

#else    	// ICS mode for kyle 
			ret = MFS_ISC_update();
#endif
		printk(KERN_ERR "mcsdl_download_binary_data : [%d]\n", ret);
			if (ret != 0)
				printk(KERN_ERR "SET Download Fail - error code [%d]\n", ret);
			else
				break;
	}
#endif

	gpio_tlmm_config(GPIO_CFG(GPIO_TSP_SDA, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(GPIO_TSP_SCL, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE);

	ts->input_dev = input_allocate_device();
    if (!ts->input_dev) {
		printk(KERN_ERR "%s: Not enough memory\n", __func__);
		ret = -ENOMEM;
		goto err_input_dev_alloc_failed;
	}
	ts->input_dev->name = "sec_touchscreen" ;
	ts->input_dev->evbit[0] = BIT_MASK(EV_ABS) | BIT_MASK(EV_KEY);

	set_bit(INPUT_PROP_DIRECT, ts->input_dev->propbit);	//JB touch mode setting

	ts->input_dev->keybit[BIT_WORD(KEY_MENU)] |= BIT_MASK(KEY_MENU);
	ts->input_dev->keybit[BIT_WORD(KEY_HOME)] |= BIT_MASK(KEY_HOME);
	ts->input_dev->keybit[BIT_WORD(KEY_BACK)] |= BIT_MASK(KEY_BACK);
	ts->input_dev->keybit[BIT_WORD(KEY_SEARCH)] |= BIT_MASK(KEY_SEARCH);
	input_mt_init_slots(ts->input_dev, MELFAS_MAX_TOUCH);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X,
						0, TS_MAX_X_COORD, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y,
						0, TS_MAX_Y_COORD, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR,
						0, TS_MAX_Z_TOUCH, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID,
						0, MELFAS_MAX_TOUCH-1, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR,
						0, TS_MAX_W_TOUCH, 0, 0);
	__set_bit(EV_LED, ts->input_dev->evbit);
	__set_bit(LED_MISC, ts->input_dev->ledbit);
    ret = input_register_device(ts->input_dev);
    if (ret) {
	printk(KERN_ERR "%s: Failed to register device\n", __func__);
	ret = -ENOMEM;
	goto err_input_register_device_failed;
    }
	
#if defined(TSP_BOOSTER)
			mutex_init(&ts->dvfs_lock);
			INIT_DELAYED_WORK(&ts->work_dvfs_off, set_dvfs_off);
			INIT_DELAYED_WORK(&ts->work_dvfs_chg, change_dvfs_lock);
			ts->dvfs_lock_status = false;
#endif

    if (ts->client->irq) {
#if DEBUG_PRINT
	printk(KERN_ERR "%s: trying to request irq: %s-%d\n", __func__,
		ts->client->name, ts->client->irq);
#endif
	ret = request_threaded_irq(client->irq, NULL, melfas_ts_irq_handler,
						IRQF_TRIGGER_LOW | IRQF_ONESHOT, ts->client->name, ts);
	if (ret > 0) {
		printk(KERN_ERR "%s: Can't allocate irq %d, ret %d\n",
			__func__, ts->client->irq, ret);
		ret = -EBUSY;
		goto err_request_irq;
	}
    }


#if defined(TA_SET_NOISE_MODE)
	ts->noise_mode = 0;
	tsp_init_for_ta = 2;
	touch_ta_noti(retry_ta_noti);	// absolute once do. 1 or 0, because resume.
#endif
		
	for (i = 0; i < MELFAS_MAX_TOUCH ; i++)  /* _SUPPORT_MULTITOUCH_ */
		g_Mtouch_info[i].strength = -1;

#if 0
	printk(KERN_ERR "[TSP] tsp_enabled is %d", tsp_enabled);
	data->register_cb(tsp_ta_probe);
	if (data->read_ta_status) {
		data->read_ta_status(&ta_status);
		printk(KERN_ERR "[TSP] ta_status is %d", ta_status);
		tsp_ta_probe(ta_status);
	}
#endif
#if DEBUG_PRINT
	printk(KERN_ERR "%s: succeed to register input device\n", __func__);
#endif
#if 0
	sec_touchscreen = device_create(sec_class, NULL, 0, ts, "sec_touchscreen");
	if (IS_ERR(sec_touchscreen))
		pr_err("[TSP] Failed to create device for the sysfs\n");
	ret = sysfs_create_group(&sec_touchscreen->kobj, &sec_touch_attr_group);
	if (ret)
		pr_err("[TSP] Failed to create sysfs group\n");
#endif
#if 0
	qt602240_noise_test = device_create(sec_class, NULL, 0, ts, "qt602240_noise_test");
	if (IS_ERR(qt602240_noise_test))
		pr_err("[TSP] Failed to create device for the sysfs\n");
	ret = sysfs_create_group(&qt602240_noise_test->kobj, &sec_touch_factory_attr_group);
	if (ret)
		pr_err("[TSP] Failed to create sysfs group\n");
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
	printk(KERN_ERR "%s: register earlysuspend.\n", __func__);
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = melfas_ts_early_suspend;
	ts->early_suspend.resume = melfas_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif
#ifdef SEC_TSP_FACTORY_TEST
	INIT_LIST_HEAD(&ts->cmd_list_head);
	for (i = 0; i < ARRAY_SIZE(tsp_cmds); i++)
		list_add_tail(&tsp_cmds[i].list, &ts->cmd_list_head);
	mutex_init(&ts->cmd_lock);
	ts->cmd_is_running = false;
	ts->cmd_state = 0;
#if defined(CONFIG_MACH_NEVIS3G)
	ts->config_fw_version = "S6812i_Me_0107";
#elif defined(CONFIG_MACH_DELOS_DUOS_CTC) || defined(CONFIG_MACH_HENNESSY_DUOS_CTC)
	ts->config_fw_version = "I869_Me_";
#elif defined(CONFIG_MACH_DELOS_OPEN)
	ts->config_fw_version = "I8552_Me_";
#elif defined(CONFIG_MACH_KYLEPLUS_CTC)
	ts->config_fw_version = "I739_Me_1016";
#elif defined(CONFIG_MACH_KYLEPLUS_OPEN)
	ts->config_fw_version = "S7572_Me_1016";
#elif defined(CONFIG_MACH_INFINITE_DUOS_CTC)
	ts->config_fw_version = "I759_Me_1107";
#elif defined(CONFIG_MACH_ARUBA_OPEN)
	ts->config_fw_version = "I8262D_Me_";
#elif defined(CONFIG_MACH_ARUBASLIM_OPEN)
	ts->config_fw_version = "I8262_Me_";
#elif defined(CONFIG_MACH_ARUBA_DUOS_CTC)
	ts->config_fw_version = "I829_Me_";
#else
	ts->config_fw_version = "I000_Me_";
#endif
	fac_dev_ts = device_create(sec_class,
			NULL, 0, ts, "tsp");
	if (IS_ERR(fac_dev_ts))
		dev_err(&client->dev, "Failed to create device for the sysfs\n");
	ret = sysfs_create_group(&fac_dev_ts->kobj,
			       &sec_touch_factory_attr_group);
	if (ret)
		dev_err(&client->dev, "Failed to create sysfs group\n");
#endif
#ifdef TUNNING_SUPPORT
	if (alloc_chrdev_region(&mms_dev, 0, 1, "mms_ts")) {
		dev_err(&client->dev, "failed to allocated device region\n");
		return -ENOMEM;
	}

	ts->class = class_create(THIS_MODULE, "mms_ts");

	cdev_init(&ts->cdev, &mms_fops);
	ts->cdev.owner = THIS_MODULE;

	if (cdev_add(&ts->cdev, mms_dev, 1)) {
		dev_err(&client->dev, "failed to add ch dev\n");
		return -EIO;
	}

	device_create(ts->class, NULL, mms_dev, NULL, "mms_ts");

	ts->log = kzalloc(sizeof(struct mms_log_data), GFP_KERNEL);

#endif

#ifdef USE_TEST_RAW_TH_DATA_MODE
	sema_init(&touch_dev->raw_data_lock, 1);
	misc_touch_dev = touch_dev;
	ret = misc_register(&touch_misc_device);
	if (ret) {
		zinitix_debug_msg("Fail to register touch misc device.\n");
	}
	if (device_create_file(touch_misc_device.this_device,
		&dev_attr_get_touch_test_raw_data) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n",
			dev_attr_get_touch_test_raw_data.attr.name);
	if (device_create_file(touch_misc_device.this_device,
		&dev_attr_raw_enable) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n",
			dev_attr_raw_enable.attr.name);
	if (device_create_file(touch_misc_device.this_device,
		&dev_attr_raw_disable) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n",
			dev_attr_raw_disable.attr.name);
	if (device_create_file(touch_misc_device.this_device,
		&dev_attr_raw_show) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n",
			dev_attr_raw_show.attr.name);
#endif
	sec_touchscreen_dev = device_create(sec_class,
	NULL, 0, NULL, "sec_touchscreen");
	if (IS_ERR(sec_touchscreen_dev))
		pr_err("Failed to create device(sec_touchscreen)!\n");
	if (device_create_file(sec_touchscreen_dev,
			&dev_attr_tsp_firm_version_phone) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_tsp_firm_version_phone.attr.name);
	if (device_create_file(sec_touchscreen_dev,
			&dev_attr_tsp_firm_version_panel) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_tsp_firm_version_panel.attr.name);
	if (device_create_file(sec_touchscreen_dev,
			&dev_attr_tsp_firm_update) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_tsp_firm_update.attr.name);
	if (device_create_file(sec_touchscreen_dev,
			&dev_attr_tsp_threshold) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_tsp_threshold.attr.name);
	if (device_create_file(sec_touchscreen_dev,
			&dev_attr_tsp_firm_update_status) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_tsp_firm_update_status.attr.name);
	if (device_create_file(sec_touchscreen_dev,
			&dev_attr_set_all_reference) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_set_all_reference.attr.name);
	if (device_create_file(sec_touchscreen_dev,
			&dev_attr_disp_all_refdata) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_disp_all_refdata.attr.name);
	if (device_create_file(sec_touchscreen_dev,
			&dev_attr_set_all_inspection) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_set_all_inspection.attr.name);
	if (device_create_file(sec_touchscreen_dev,
			&dev_attr_disp_all_insdata) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_disp_all_insdata.attr.name);
	if (device_create_file(sec_touchscreen_dev,
			&dev_attr_set_all_intensity) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_set_all_intensity.attr.name);
	if (device_create_file(sec_touchscreen_dev,
			&dev_attr_disp_all_intdata) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_disp_all_intdata.attr.name);
	if (device_create_file(sec_touchscreen_dev,
			&dev_attr_firmware) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_firmware.attr.name);
	if (device_create_file(sec_touchscreen_dev,
			&dev_attr_raw_value) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_raw_value.attr.name);
	if (device_create_file(sec_touchscreen_dev,
			&dev_attr_touchtype) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_touchtype.attr.name);
	if (device_create_file(sec_touchscreen_dev,
			&dev_attr_set_module_off) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_set_module_off.attr.name);
	if (device_create_file(sec_touchscreen_dev,
			    &dev_attr_set_module_on) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_set_module_on.attr.name);
	sec_touchkey_dev = device_create(sec_class,
			NULL, 0, NULL, "sec_touchkey");
	if (IS_ERR(sec_touchkey_dev))
		pr_err("Failed to create device(sec_touchscreen)!\n");
	if (device_create_file(sec_touchkey_dev,
			&dev_attr_touchkey_back) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_touchkey_back.attr.name);
	if (device_create_file(sec_touchkey_dev,
			&dev_attr_touchkey_menu) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_touchkey_menu.attr.name);
	if (device_create_file(sec_touchkey_dev,
			&dev_attr_touchkey_raw_data) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_touchkey_raw_data.attr.name);
	if (device_create_file(sec_touchkey_dev, &dev_attr_touch_sensitivity) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_touch_sensitivity.attr.name);
	if (device_create_file(sec_touchkey_dev,
			&dev_attr_touchkey_threshold) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_touchkey_threshold.attr.name);
	if (device_create_file(sec_touchkey_dev,
			&dev_attr_brightness) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_brightness.attr.name);
	tsp_enabled = true;
	return 0;
#if 0
	TSP_boost(ts, is_boost);
#endif
#if DEBUG_PRINT
	printk(KERN_ERR "%s: Start touchscreen. name: %s, irq: %d\n",
		__func__, ts->client->name, ts->client->irq);
#endif
	return 0;

//err_detect_failed:
//	ts->power(false);
//	printk(KERN_ERR "melfas-ts: err_detect failed\n");
//	kfree(ts);
err_request_irq:
	printk(KERN_ERR "melfas-ts: err_request_irq failed\n");
	free_irq(client->irq, ts);
err_input_register_device_failed:
	printk(KERN_ERR "melfas-ts: err_input_register_device failed\n");
	input_free_device(ts->input_dev);
err_input_dev_alloc_failed:
	printk(KERN_ERR "melfas-ts: err_input_dev_alloc failed\n");
err_alloc_data_failed:
	printk(KERN_ERR "melfas-ts: err_alloc_data failed_\n");
#if 0
	if (tsp_reboot_count < 3) {
		tsp_reboot_count++;
		goto init_again;
	}
#endif
err_check_functionality_failed:
	printk(KERN_ERR "melfas-ts: err_check_functionality failed_\n");
	return ret;
}
static int melfas_ts_remove(struct i2c_client *client)
{
	struct melfas_ts_data *ts = i2c_get_clientdata(client);
	unregister_early_suspend(&ts->early_suspend);
	free_irq(client->irq, ts);
	ts->power(false);
	input_unregister_device(ts->input_dev);
	kfree(ts);
	return 0;
}
static int melfas_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	//int ret;
	struct melfas_ts_data *ts = i2c_get_clientdata(client);
	tsp_enabled = false;
	printk(KERN_ERR "[TSP] %s disable IRQ( %d)\n", __func__, __LINE__);
	disable_irq(client->irq);
	release_all_fingers(ts);
	touch_is_pressed = 0;
	ts_power_enable(0);
	return 0;
}
static int melfas_ts_resume(struct i2c_client *client)
{
	struct melfas_ts_data *ts = i2c_get_clientdata(client);
#if 0
	bool ta_status = 0;
#endif

#if 0
	TSP_boost(ts, is_boost);
#endif
	ts_power_enable(1);

	msleep(50);
#if defined(TA_SET_NOISE_MODE)
	mms_set_noise_mode(ts);
#endif
	tsp_enabled = true;
	printk(KERN_ERR "[TSP] %s enable IRQ( %d)\n", __func__, __LINE__);
	enable_irq(client->irq);
#if 0
	if (ts->read_ta_status) {
		ts->read_ta_status(&ta_status);
		printk(KERN_ERR "[TSP] ta_status is %d", ta_status);
		tsp_ta_probe(ta_status);
	}
#endif

	return 0;
}
#ifdef CONFIG_HAS_EARLYSUSPEND
static void melfas_ts_early_suspend(struct early_suspend *h)
{
	struct melfas_ts_data *ts;
	ts = container_of(h, struct melfas_ts_data, early_suspend);
	melfas_ts_suspend(ts->client, PMSG_SUSPEND);
}
static void melfas_ts_late_resume(struct early_suspend *h)
{
	struct melfas_ts_data *ts;
	ts = container_of(h, struct melfas_ts_data, early_suspend);
	melfas_ts_resume(ts->client);
}
#endif
static const struct i2c_device_id melfas_ts_id[] = {
	{ "sec_touch", 0 },
	{ }
};
static struct i2c_driver melfas_ts_driver = {
    .driver = {
    .name = "sec_touch",
    },
    .id_table = melfas_ts_id,
    .probe = melfas_ts_probe,
    .remove = __devexit_p(melfas_ts_remove),
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend		= melfas_ts_suspend,
	.resume		= melfas_ts_resume,
#endif
};
#ifdef CONFIG_BATTERY_SEC
extern unsigned int is_lpcharging_state(void);
#endif
static int __devinit melfas_ts_init(void)
{
#ifdef CONFIG_BATTERY_SEC
	if (is_lpcharging_state()) {
		pr_info("%s : LPM Charging Mode! return 0\n", __func__);
		return 0;
	}
#endif
	if (board_hw_revision == 0)
		FW_VERSION = 0x05;
	else if (board_hw_revision == 1)
		FW_VERSION = 0x08;
	else
		FW_VERSION = 0xFF;
	return i2c_add_driver(&melfas_ts_driver);
}
static void __exit melfas_ts_exit(void)
{
	i2c_del_driver(&melfas_ts_driver);
}
MODULE_DESCRIPTION("Driver for Melfas MTSI Touchscreen Controller");
MODULE_AUTHOR("MinSang, Kim <kimms@melfas.com>");
MODULE_VERSION("0.1");
MODULE_LICENSE("GPL");
module_init(melfas_ts_init);
module_exit(melfas_ts_exit);
