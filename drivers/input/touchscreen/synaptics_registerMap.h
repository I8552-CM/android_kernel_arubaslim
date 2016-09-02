  /* Register map header file for S2200, Family 0x00, Revision 0x00 */

  /* Automatically generated on 2012-Jun-28  14:39:31, do not edit */

#ifndef SYNA_REGISTER_MAP_H
  #define SYNA_REGISTER_MAP_H 1

  /*      Register Name                                      Address     Register Description */
  /*      -------------                                      -------     -------------------- */
  #define SYNA_F34_FLASH_DATA00                              0x0000   /* Block Number LSB */
  #define SYNA_F34_FLASH_DATA01                              0x0001   /* Block Number MSB */
  #define SYNA_F34_FLASH_DATA02_00                           0x0002   /* Block Data 0 */
  #define SYNA_F34_FLASH_DATA02_01                           0x0003   /* Block Data 1 */
  #define SYNA_F34_FLASH_DATA02_02                           0x0004   /* Block Data 2 */
  #define SYNA_F34_FLASH_DATA02_03                           0x0005   /* Block Data 3 */
  #define SYNA_F34_FLASH_DATA02_04                           0x0006   /* Block Data 4 */
  #define SYNA_F34_FLASH_DATA02_05                           0x0007   /* Block Data 5 */
  #define SYNA_F34_FLASH_DATA02_06                           0x0008   /* Block Data 6 */
  #define SYNA_F34_FLASH_DATA02_07                           0x0009   /* Block Data 7 */
  #define SYNA_F34_FLASH_DATA02_08                           0x000A   /* Block Data 8 */
  #define SYNA_F34_FLASH_DATA02_09                           0x000B   /* Block Data 9 */
  #define SYNA_F34_FLASH_DATA02_10                           0x000C   /* Block Data 10 */
  #define SYNA_F34_FLASH_DATA02_11                           0x000D   /* Block Data 11 */
  #define SYNA_F34_FLASH_DATA02_12                           0x000E   /* Block Data 12 */
  #define SYNA_F34_FLASH_DATA02_13                           0x000F   /* Block Data 13 */
  #define SYNA_F34_FLASH_DATA02_14                           0x0010   /* Block Data 14 */
  #define SYNA_F34_FLASH_DATA02_15                           0x0011   /* Block Data 15 */
  #define SYNA_F34_FLASH_DATA03                              0x0012   /* Flash Control */
  #define SYNA_F01_RMI_DATA00                                0x0013   /* Device Status */
  #define SYNA_F01_RMI_DATA01_00                             0x0014   /* Interrupt Status */
  #define SYNA_F11_2D_DATA00_00                              0x0015   /* 2D Finger State */
  #define SYNA_F11_2D_DATA00_01                              0x0016   /* 2D Finger State */
  #define SYNA_F11_2D_DATA01_00                              0x0017   /* 2D X Position (11:4) Finger 0 */
  #define SYNA_F11_2D_DATA02_00                              0x0018   /* 2D Y Position (11:4) Finger 0 */
  #define SYNA_F11_2D_DATA03_00                              0x0019   /* 2D Y/X Position (3:0) Finger 0 */
  #define SYNA_F11_2D_DATA04_00                              0x001A   /* 2D Wy/Wx Finger 0 */
  #define SYNA_F11_2D_DATA05_00                              0x001B   /* 2D Z Finger 0 */
  #define SYNA_F11_2D_DATA01_01                              0x001C   /* 2D X Position (11:4) Finger 1 */
  #define SYNA_F11_2D_DATA02_01                              0x001D   /* 2D Y Position (11:4) Finger 1 */
  #define SYNA_F11_2D_DATA03_01                              0x001E   /* 2D Y/X Position (3:0) Finger 1 */
  #define SYNA_F11_2D_DATA04_01                              0x001F   /* 2D Wy/Wx Finger 1 */
  #define SYNA_F11_2D_DATA05_01                              0x0020   /* 2D Z Finger 1 */
  #define SYNA_F11_2D_DATA01_02                              0x0021   /* 2D X Position (11:4) Finger 2 */
  #define SYNA_F11_2D_DATA02_02                              0x0022   /* 2D Y Position (11:4) Finger 2 */
  #define SYNA_F11_2D_DATA03_02                              0x0023   /* 2D Y/X Position (3:0) Finger 2 */
  #define SYNA_F11_2D_DATA04_02                              0x0024   /* 2D Wy/Wx Finger 2 */
  #define SYNA_F11_2D_DATA05_02                              0x0025   /* 2D Z Finger 2 */
  #define SYNA_F11_2D_DATA01_03                              0x0026   /* 2D X Position (11:4) Finger 3 */
  #define SYNA_F11_2D_DATA02_03                              0x0027   /* 2D Y Position (11:4) Finger 3 */
  #define SYNA_F11_2D_DATA03_03                              0x0028   /* 2D Y/X Position (3:0) Finger 3 */
  #define SYNA_F11_2D_DATA04_03                              0x0029   /* 2D Wy/Wx Finger 3 */
  #define SYNA_F11_2D_DATA05_03                              0x002A   /* 2D Z Finger 3 */
  #define SYNA_F11_2D_DATA01_04                              0x002B   /* 2D X Position (11:4) Finger 4 */
  #define SYNA_F11_2D_DATA02_04                              0x002C   /* 2D Y Position (11:4) Finger 4 */
  #define SYNA_F11_2D_DATA03_04                              0x002D   /* 2D Y/X Position (3:0) Finger 4 */
  #define SYNA_F11_2D_DATA04_04                              0x002E   /* 2D Wy/Wx Finger 4 */
  #define SYNA_F11_2D_DATA05_04                              0x002F   /* 2D Z Finger 4 */
  #define SYNA_F11_2D_DATA28                                 0x0030   /* 2D Extended Status */
  #define SYNA_F11_2D_DATA35_00                              0x0031   /* 2D Wy/Wx High Finger 0 */
  #define SYNA_F11_2D_DATA35_01                              0x0032   /* 2D Wy/Wx High Finger 1 */
  #define SYNA_F11_2D_DATA35_02                              0x0033   /* 2D Wy/Wx High Finger 2 */
  #define SYNA_F11_2D_DATA35_03                              0x0034   /* 2D Wy/Wx High Finger 3 */
  #define SYNA_F11_2D_DATA35_04                              0x0035   /* 2D Wy/Wx High Finger 4 */
  #define SYNA_F11_2D_DATA38                                 0x0036   /* LPWG Status */
  #define SYNA_F11_2D_DATA39_00_00                           0x0037   /* Wakeup Gesture X Swipe Distance */
  #define SYNA_F11_2D_DATA39_00_01                           0x0037   /* Wakeup Gesture X Swipe Distance */
  #define SYNA_F11_2D_DATA39_00_02                           0x0037   /* Wakeup Gesture Y Swipe Distance */
  #define SYNA_F11_2D_DATA39_00_03                           0x0037   /* Wakeup Gesture Y Swipe Distance */
  #define SYNA_F34_FLASH_CTRL00_00                           0x0038   /* Customer Defined Config ID 0 */
  #define SYNA_F34_FLASH_CTRL00_01                           0x0039   /* Customer Defined Config ID 1 */
  #define SYNA_F34_FLASH_CTRL00_02                           0x003A   /* Customer Defined Config ID 2 */
  #define SYNA_F34_FLASH_CTRL00_03                           0x003B   /* Customer Defined Config ID 3 */
  #define SYNA_F01_RMI_CTRL00                                0x003C   /* Device Control */
  #define SYNA_F01_RMI_CTRL01_00                             0x003D   /* Interrupt Enable 0 */
  #define SYNA_F01_RMI_CTRL02                                0x003E   /* Doze Interval */
  #define SYNA_F01_RMI_CTRL03                                0x003F   /* Doze Wakeup Threshold */
  #define SYNA_F01_RMI_CTRL05                                0x0040   /* Doze Holdoff */
  #define SYNA_F01_RMI_CTRL09                                0x0041   /* Doze Recalibration Interval */
  #define SYNA_F11_2D_CTRL00                                 0x0042   /* 2D Report Mode */
  #define SYNA_F11_2D_CTRL01                                 0x0043   /* 2D Palm Detect */
  #define SYNA_F11_2D_CTRL02                                 0x0044   /* 2D Delta-X Thresh */
  #define SYNA_F11_2D_CTRL03                                 0x0045   /* 2D Delta-Y Thresh */
  #define SYNA_F11_2D_CTRL04                                 0x0046   /* 2D Velocity */
  #define SYNA_F11_2D_CTRL05                                 0x0047   /* 2D Acceleration */
  #define SYNA_F11_2D_CTRL06                                 0x0048   /* 2D Max X Position (7:0) */
  #define SYNA_F11_2D_CTRL07                                 0x0049   /* 2D Max X Position (11:8) */
  #define SYNA_F11_2D_CTRL08                                 0x004A   /* 2D Max Y Position (7:0) */
  #define SYNA_F11_2D_CTRL09                                 0x004B   /* 2D Max Y Position (11:8) */
  #define SYNA_F11_2D_CTRL29                                 0x004C   /* Z Touch Threshold */
  #define SYNA_F11_2D_CTRL30                                 0x004D   /* Z Hysteresis */
  #define SYNA_F11_2D_CTRL31                                 0x004E   /* Small Z Threshold */
  #define SYNA_F11_2D_CTRL32_00                              0x004F   /* Small Z Scale Factor */
  #define SYNA_F11_2D_CTRL32_01                              0x0050   /* Small Z Scale Factor */
  #define SYNA_F11_2D_CTRL33_00                              0x0051   /* Large Z Scale Factor */
  #define SYNA_F11_2D_CTRL33_01                              0x0052   /* Large Z Scale Factor */
  #define SYNA_F11_2D_CTRL34                                 0x0053   /* Position Calculation & Post Correction */
  #define SYNA_F11_2D_CTRL36                                 0x0054   /* Wx Scale Factor */
  #define SYNA_F11_2D_CTRL37                                 0x0055   /* Wx Offset */
  #define SYNA_F11_2D_CTRL38                                 0x0056   /* Wy Scale Factor */
  #define SYNA_F11_2D_CTRL39                                 0x0057   /* Wy Offset */
  #define SYNA_F11_2D_CTRL40_00                              0x0058   /* X Pitch */
  #define SYNA_F11_2D_CTRL40_01                              0x0059   /* X Pitch */
  #define SYNA_F11_2D_CTRL41_00                              0x005A   /* Y Pitch */
  #define SYNA_F11_2D_CTRL41_01                              0x005B   /* Y Pitch */
  #define SYNA_F11_2D_CTRL42_00                              0x005C   /* Default Finger Width Tx */
  #define SYNA_F11_2D_CTRL42_01                              0x005D   /* Default Finger Width Tx */
  #define SYNA_F11_2D_CTRL43_00                              0x005E   /* Default Finger Width Ty */
  #define SYNA_F11_2D_CTRL43_01                              0x005F   /* Default Finger Width Ty */
  #define SYNA_F11_2D_CTRL44                                 0x0060   /* Report Finger Width */
  #define SYNA_F11_2D_CTRL45                                 0x0061   /* Segmentation Aggressiveness */
  #define SYNA_F11_2D_CTRL46                                 0x0062   /* Rx Clip LSB */
  #define SYNA_F11_2D_CTRL47                                 0x0063   /* Rx Clip MSB */
  #define SYNA_F11_2D_CTRL48                                 0x0064   /* Tx Clip LSB */
  #define SYNA_F11_2D_CTRL49                                 0x0065   /* Tx Clip MSB */
  #define SYNA_F11_2D_CTRL50                                 0x0066   /* Minimum Drumming Separation */
  #define SYNA_F11_2D_CTRL51                                 0x0067   /* Maximum Drumming Movement */
  #define SYNA_F11_2D_CTRL58                                 0x0068   /* Large Object Suppression Parameters */
  #define SYNA_F11_2D_CTRL77                                 0x0069   /* 2D RX Number */
  #define SYNA_F11_2D_CTRL78                                 0x006A   /* 2D TX Number */
  #define SYNA_F11_2D_CTRL80                                 0x006B   /* Finger Limit */
  #define SYNA_F11_2D_CTRL89                                 0x006C   /* Jitter Filter 2 Strength */
  #define SYNA_F11_2D_CTRL90_00_00                           0x006D   /* Land/Lift Control */
  #define SYNA_F11_2D_CTRL90_00_01                           0x006D   /* Land Thresholds */
  #define SYNA_F11_2D_CTRL90_00_02                           0x006D   /* Buffering Distance */
  #define SYNA_F11_2D_CTRL90_00_03                           0x006D   /* Lift Z Differential */
  #define SYNA_F11_2D_CTRL91_00_00                           0x006E   /* Group Decomposition Control */
  #define SYNA_F11_2D_CTRL91_00_01                           0x006E   /* Group Decomposition Control */
  #define SYNA_F11_2D_CTRL91_00_02                           0x006E   /* Group Decomposition Control */
  #define SYNA_F11_2D_CTRL92_00_00                           0x006F   /* LPWG Control */
  #define SYNA_F11_2D_CTRL92_01_00                           0x006F   /* Double-Tap Zone 0 X LSB */
  #define SYNA_F11_2D_CTRL92_01_01                           0x006F   /* Double-Tap Zone 0 Y LSB */
  #define SYNA_F11_2D_CTRL92_01_02                           0x006F   /* Double-Tap Zone 0 XY MSB */
  #define SYNA_F11_2D_CTRL92_01_03                           0x006F   /* Double-Tap Zone 1 X LSB */
  #define SYNA_F11_2D_CTRL92_01_04                           0x006F   /* Double-Tap Zone 1 Y LSB */
  #define SYNA_F11_2D_CTRL92_01_05                           0x006F   /* Double-Tap Zone 1 XY MSB */
  #define SYNA_F11_2D_CTRL92_01_06                           0x006F   /* Maximum Tap Time */
  #define SYNA_F11_2D_CTRL92_01_07                           0x006F   /* Maximum Tap Distance */
  #define SYNA_F11_2D_CTRL92_02_00                           0x006F   /* Swipe Zone 0 X LSB */
  #define SYNA_F11_2D_CTRL92_02_01                           0x006F   /* Swipe Zone 0 Y LSB */
  #define SYNA_F11_2D_CTRL92_02_02                           0x006F   /* Swipe Zone 0 XY MSB */
  #define SYNA_F11_2D_CTRL92_02_03                           0x006F   /* Swipe Zone 1 X LSB */
  #define SYNA_F11_2D_CTRL92_02_04                           0x006F   /* Swipe Zone 1 Y LSB */
  #define SYNA_F11_2D_CTRL92_02_05                           0x006F   /* Swipe Zone 1 XY MSB */
  #define SYNA_F11_2D_CTRL92_02_06                           0x006F   /* Swipe Minimum Distance */
  #define SYNA_F11_2D_CTRL92_02_07                           0x006F   /* Swipe Minimum Speed */
  #define SYNA_F01_RMI_CMD00                                 0x0070   /* Device Command */
  #define SYNA_F11_2D_CMD00                                  0x0071   /* 2D Command */
  #define SYNA_F34_FLASH_QUERY00                             0x0072   /* Bootloader ID 0 */
  #define SYNA_F34_FLASH_QUERY01                             0x0073   /* Bootloader ID 1 */
  #define SYNA_F34_FLASH_QUERY02                             0x0074   /* Flash Properties */
  #define SYNA_F34_FLASH_QUERY03                             0x0075   /* Block Size 0 */
  #define SYNA_F34_FLASH_QUERY04                             0x0076   /* Block Size 1 */
  #define SYNA_F34_FLASH_QUERY05                             0x0077   /* Firmware Block Count 0 */
  #define SYNA_F34_FLASH_QUERY06                             0x0078   /* Firmware Block Count 1 */
  #define SYNA_F34_FLASH_QUERY07_00_00                       0x0079   /* UIConfig Block Count LSB */
  #define SYNA_F34_FLASH_QUERY07_00_01                       0x0079   /* UIConfig Block Count MSB */
  #define SYNA_F34_FLASH_QUERY07_01_00                       0x0079   /* PermConfigBlockCountLSB */
  #define SYNA_F34_FLASH_QUERY07_01_01                       0x0079   /* PermConfigBlockCountMSB */
  #define SYNA_F34_FLASH_QUERY07_02_00                       0x0079   /* BLConfigBlockCountLSB */
  #define SYNA_F34_FLASH_QUERY07_02_01                       0x0079   /* BLConfigBlockCountMSB */
  #define SYNA_F34_FLASH_QUERY08                             0x007A   /* Configuration Block Count 1 */
  #define SYNA_F01_RMI_QUERY00                               0x007B   /* Manufacturer ID Query */
  #define SYNA_F01_RMI_QUERY01                               0x007C   /* Product Properties Query */
  #define SYNA_F01_RMI_QUERY02                               0x007D   /* Customer Family Query */
  #define SYNA_F01_RMI_QUERY03                               0x007E   /* Firmware Revision Query */
  #define SYNA_F01_RMI_QUERY04                               0x007F   /* Device Serialization Query 0 */
  #define SYNA_F01_RMI_QUERY05                               0x0080   /* Device Serialization Query 1 */
  #define SYNA_F01_RMI_QUERY06                               0x0081   /* Device Serialization Query 2 */
  #define SYNA_F01_RMI_QUERY07                               0x0082   /* Device Serialization Query 3 */
  #define SYNA_F01_RMI_QUERY08                               0x0083   /* Device Serialization Query 4 */
  #define SYNA_F01_RMI_QUERY09                               0x0084   /* Device Serialization Query 5 */
  #define SYNA_F01_RMI_QUERY10                               0x0085   /* Device Serialization Query 6 */
  #define SYNA_F01_RMI_QUERY11                               0x0086   /* Product ID Query 0 */
  #define SYNA_F01_RMI_QUERY12                               0x0087   /* Product ID Query 1 */
  #define SYNA_F01_RMI_QUERY13                               0x0088   /* Product ID Query 2 */
  #define SYNA_F01_RMI_QUERY14                               0x0089   /* Product ID Query 3 */
  #define SYNA_F01_RMI_QUERY15                               0x008A   /* Product ID Query 4 */
  #define SYNA_F01_RMI_QUERY16                               0x008B   /* Product ID Query 5 */
  #define SYNA_F01_RMI_QUERY17                               0x008C   /* Product ID Query 6 */
  #define SYNA_F01_RMI_QUERY18                               0x008D   /* Product ID Query 7 */
  #define SYNA_F01_RMI_QUERY19                               0x008E   /* Product ID Query 8 */
  #define SYNA_F01_RMI_QUERY20                               0x008F   /* Product ID Query 9 */
  #define SYNA_F01_RMI_QUERY42                               0x0090   /* Product Properties 2 */
  #define SYNA_F01_RMI_QUERY43_00                            0x0091   /* DS4 Queries0 */
  #define SYNA_F01_RMI_QUERY43_01                            0x0092   /* DS4 Queries1 */
  #define SYNA_F01_RMI_QUERY43_02                            0x0093   /* DS4 Queries2 */
  #define SYNA_F01_RMI_QUERY43_03                            0x0094   /* DS4 Queries3 */
  #define SYNA_F01_RMI_QUERY44                               0x0095   /* Reset Query */
  #define SYNA_F01_RMI_QUERY45_00_00                         0x0096   /* DS4 Tool ID 0 */
  #define SYNA_F01_RMI_QUERY45_00_01                         0x0096   /* DS4 Tool ID 1 */
  #define SYNA_F01_RMI_QUERY45_00_02                         0x0096   /* DS4 Tool ID 2 */
  #define SYNA_F01_RMI_QUERY45_00_03                         0x0096   /* DS4 Tool ID 3 */
  #define SYNA_F01_RMI_QUERY45_00_04                         0x0096   /* DS4 Tool ID 4 */
  #define SYNA_F01_RMI_QUERY45_00_05                         0x0096   /* DS4 Tool ID 5 */
  #define SYNA_F01_RMI_QUERY45_00_06                         0x0096   /* DS4 Tool ID 6 */
  #define SYNA_F01_RMI_QUERY45_00_07                         0x0096   /* DS4 Tool ID 7 */
  #define SYNA_F01_RMI_QUERY45_00_08                         0x0096   /* DS4 Tool ID 8 */
  #define SYNA_F01_RMI_QUERY45_00_09                         0x0096   /* DS4 Tool ID 9 */
  #define SYNA_F01_RMI_QUERY45_00_10                         0x0096   /* DS4 Tool ID 10 */
  #define SYNA_F01_RMI_QUERY45_00_11                         0x0096   /* DS4 Tool ID 11 */
  #define SYNA_F01_RMI_QUERY45_00_12                         0x0096   /* DS4 Tool ID 12 */
  #define SYNA_F01_RMI_QUERY45_00_13                         0x0096   /* DS4 Tool ID 13 */
  #define SYNA_F01_RMI_QUERY45_00_14                         0x0096   /* DS4 Tool ID 14 */
  #define SYNA_F01_RMI_QUERY45_00_15                         0x0096   /* DS4 Tool ID 15 */
  #define SYNA_F01_RMI_QUERY46_00_00                         0x0097   /* Firmware Revision ID 0 */
  #define SYNA_F01_RMI_QUERY46_00_01                         0x0097   /* Firmware Revision ID 1 */
  #define SYNA_F01_RMI_QUERY46_00_02                         0x0097   /* Firmware Revision ID 2 */
  #define SYNA_F01_RMI_QUERY46_00_03                         0x0097   /* Firmware Revision ID 3 */
  #define SYNA_F01_RMI_QUERY46_00_04                         0x0097   /* Firmware Revision ID 4 */
  #define SYNA_F01_RMI_QUERY46_00_05                         0x0097   /* Firmware Revision ID 5 */
  #define SYNA_F01_RMI_QUERY46_00_06                         0x0097   /* Firmware Revision ID 6 */
  #define SYNA_F01_RMI_QUERY46_00_07                         0x0097   /* Firmware Revision ID 7 */
  #define SYNA_F01_RMI_QUERY46_00_08                         0x0097   /* Firmware Revision ID 8 */
  #define SYNA_F01_RMI_QUERY46_00_09                         0x0097   /* Firmware Revision ID 9 */
  #define SYNA_F01_RMI_QUERY46_00_10                         0x0097   /* Firmware Revision ID 10 */
  #define SYNA_F01_RMI_QUERY46_00_11                         0x0097   /* Firmware Revision ID 11 */
  #define SYNA_F01_RMI_QUERY46_00_12                         0x0097   /* Firmware Revision ID 12 */
  #define SYNA_F01_RMI_QUERY46_00_13                         0x0097   /* Firmware Revision ID 13 */
  #define SYNA_F01_RMI_QUERY46_00_14                         0x0097   /* Firmware Revision ID 14 */
  #define SYNA_F01_RMI_QUERY46_00_15                         0x0097   /* Firmware Revision ID 15 */
  #define SYNA_F11_2D_QUERY00                                0x0098   /* Per-device Query */
  #define SYNA_F11_2D_QUERY01                                0x0099   /* General Sensor Information */
  #define SYNA_F11_2D_QUERY02                                0x009A   /* 2D Number of X Electrodes */
  #define SYNA_F11_2D_QUERY03                                0x009B   /* 2D Number of Y Electrodes */
  #define SYNA_F11_2D_QUERY04                                0x009C   /* 2D Maximum Electrodes */
  #define SYNA_F11_2D_QUERY05                                0x009D   /* 2D Absolute Query */
  #define SYNA_F11_2D_QUERY11                                0x009E   /* Advanced Capabilities */
  #define SYNA_F11_2D_QUERY12                                0x009F   /* Advanced Sensing Features 2 */
  #define SYNA_F11_2D_QUERY27                                0x00A0   /* Advanced Sensing Features 3 */
  #define SYNA_F11_2D_QUERY28                                0x00A1   /* Advanced Sensing Features 4 */
  #define SYNA_F11_2D_QUERY32                                0x00A2   /* Size of Ctrl92 */
  #define SYNA_F11_2D_QUERY33                                0x00A3   /* LPWG Query */
  #define SYNA_F11_2D_QUERY34                                0x00A4   /* Size of Ctrl90 */
  #define SYNA_F11_2D_QUERY35                                0x00A5   /* Size of Data39 */

  /* Start of Page Description Table (PDT) */

  #define SYNA_PDT_P00_F11_2D_QUERY_BASE                     0x00DD   /* Query Base */
  #define SYNA_PDT_P00_F11_2D_COMMAND_BASE                   0x00DE   /* Command Base */
  #define SYNA_PDT_P00_F11_2D_CONTROL_BASE                   0x00DF   /* Control Base */
  #define SYNA_PDT_P00_F11_2D_DATA_BASE                      0x00E0   /* Data Base */
  #define SYNA_PDT_P00_F11_2D_INTERRUPTS                     0x00E1   /* Version & Interrupt Count */
  #define SYNA_PDT_P00_F11_2D_EXISTS                         0x00E2   /* Function Exists */
  #define SYNA_PDT_P00_F01_RMI_QUERY_BASE                    0x00E3   /* Query Base */
  #define SYNA_PDT_P00_F01_RMI_COMMAND_BASE                  0x00E4   /* Command Base */
  #define SYNA_PDT_P00_F01_RMI_CONTROL_BASE                  0x00E5   /* Control Base */
  #define SYNA_PDT_P00_F01_RMI_DATA_BASE                     0x00E6   /* Data Base */
  #define SYNA_PDT_P00_F01_RMI_INTERRUPTS                    0x00E7   /* Version & Interrupt Count */
  #define SYNA_PDT_P00_F01_RMI_EXISTS                        0x00E8   /* Function Exists */
  #define SYNA_PDT_P00_F34_FLASH_QUERY_BASE                  0x00E9   /* Query Base */
  #define SYNA_PDT_P00_F34_FLASH_COMMAND_BASE                0x00EA   /* Command Base */
  #define SYNA_PDT_P00_F34_FLASH_CONTROL_BASE                0x00EB   /* Control Base */
  #define SYNA_PDT_P00_F34_FLASH_DATA_BASE                   0x00EC   /* Data Base */
  #define SYNA_PDT_P00_F34_FLASH_INTERRUPTS                  0x00ED   /* Version & Interrupt Count */
  #define SYNA_PDT_P00_F34_FLASH_EXISTS                      0x00EE   /* Function Exists */
  #define SYNA_P00_PDT_PROPERTIES                            0x00EF   /* P00_PDT Properties */
  #define SYNA_P00_PAGESELECT                                0x00FF   /* Page Select register */

  /* Registers on Page 0x01 */

  /*      Register Name                                      Address     Register Description */
  /*      -------------                                      -------     -------------------- */
  #define SYNA_F54_ANALOG_DATA00                             0x0100   /* Report Type */
  #define SYNA_F54_ANALOG_DATA01                             0x0101   /* Report Index LSB */
  #define SYNA_F54_ANALOG_DATA02                             0x0102   /* Report Index MSB */
  #define SYNA_F54_ANALOG_DATA03                             0x0103   /* Report Data */
  #define SYNA_F54_ANALOG_DATA04                             0x0104   /* Current Sense Freq Selection */
  #define SYNA_F54_ANALOG_DATA06_00                          0x0105   /* Interference Metric LSB */
  #define SYNA_F54_ANALOG_DATA06_01                          0x0106   /* Interference Metric MSB */
  #define SYNA_F54_ANALOG_DATA07_00                          0x0107   /* Report Rate LSB */
  #define SYNA_F54_ANALOG_DATA07_01                          0x0108   /* Report Rate MSB */
  #define SYNA_F54_ANALOG_DATA08_00                          0x0109   /* Variance Metric LSB */
  #define SYNA_F54_ANALOG_DATA08_01                          0x010A   /* Variance Metric MSB */
  #define SYNA_F54_ANALOG_DATA10                             0x010B   /* Current Noise State */
  #define SYNA_F54_ANALOG_DATA11                             0x010C   /* Status */
  #define SYNA_F54_ANALOG_CTRL00                             0x010D   /* General Control */
  #define SYNA_F54_ANALOG_CTRL01                             0x010E   /* General Control 1 */
  #define SYNA_F54_ANALOG_CTRL02_00                          0x010F   /* Saturation Capacitance LSB */
  #define SYNA_F54_ANALOG_CTRL02_01                          0x0110   /* Saturation Capacitance MSB */
  #define SYNA_F54_ANALOG_CTRL03                             0x0111   /* Pixel Touch Threshold */
  #define SYNA_F54_ANALOG_CTRL04                             0x0112   /* Misc Analog Control */
  #define SYNA_F54_ANALOG_CTRL05                             0x0113   /* RefCap RefLo Settings */
  #define SYNA_F54_ANALOG_CTRL06                             0x0114   /* RefCap RefHi Settings */
  #define SYNA_F54_ANALOG_CTRL07                             0x0115   /* CBC Cap Settings */
  #define SYNA_F54_ANALOG_CTRL08_00                          0x0116   /* Integration Duration LSB */
  #define SYNA_F54_ANALOG_CTRL08_01                          0x0117   /* Integration Duration MSB */
  #define SYNA_F54_ANALOG_CTRL09                             0x0118   /* Reset Duration */
  #define SYNA_F54_ANALOG_CTRL10                             0x0119   /* Noise Measurement Control */
  #define SYNA_F54_ANALOG_CTRL11_00                          0x011A   /* Reserved */
  #define SYNA_F54_ANALOG_CTRL11_01                          0x011B   /* Reserved */
  #define SYNA_F54_ANALOG_CTRL12                             0x011C   /* Slow Relaxation Rate */
  #define SYNA_F54_ANALOG_CTRL13                             0x011D   /* Fast Relaxation Rate */
  #define SYNA_F54_ANALOG_CTRL17_00                          0x011E   /* Sense Frequency Control 0 0 */
  #define SYNA_F54_ANALOG_CTRL17_01                          0x011F   /* Sense Frequency Control 0 1 */
  #define SYNA_F54_ANALOG_CTRL17_02                          0x0120   /* Sense Frequency Control 0 2 */
  #define SYNA_F54_ANALOG_CTRL17_03                          0x0121   /* Sense Frequency Control 0 3 */
  #define SYNA_F54_ANALOG_CTRL17_04                          0x0122   /* Sense Frequency Control 0 4 */
  #define SYNA_F54_ANALOG_CTRL17_05                          0x0123   /* Sense Frequency Control 0 5 */
  #define SYNA_F54_ANALOG_CTRL17_06                          0x0124   /* Sense Frequency Control 0 6 */
  #define SYNA_F54_ANALOG_CTRL17_07                          0x0125   /* Sense Frequency Control 0 7 */
  #define SYNA_F54_ANALOG_CTRL18_00                          0x0126   /* Sense Frequency Control 1 0 */
  #define SYNA_F54_ANALOG_CTRL18_01                          0x0127   /* Sense Frequency Control 1 1 */
  #define SYNA_F54_ANALOG_CTRL18_02                          0x0128   /* Sense Frequency Control 1 2 */
  #define SYNA_F54_ANALOG_CTRL18_03                          0x0129   /* Sense Frequency Control 1 3 */
  #define SYNA_F54_ANALOG_CTRL18_04                          0x012A   /* Sense Frequency Control 1 4 */
  #define SYNA_F54_ANALOG_CTRL18_05                          0x012B   /* Sense Frequency Control 1 5 */
  #define SYNA_F54_ANALOG_CTRL18_06                          0x012C   /* Sense Frequency Control 1 6 */
  #define SYNA_F54_ANALOG_CTRL18_07                          0x012D   /* Sense Frequency Control 1 7 */
  #define SYNA_F54_ANALOG_CTRL19_00                          0x012E   /* Sense Frequency Control 2 0 */
  #define SYNA_F54_ANALOG_CTRL19_01                          0x012F   /* Sense Frequency Control 2 1 */
  #define SYNA_F54_ANALOG_CTRL19_02                          0x0130   /* Sense Frequency Control 2 2 */
  #define SYNA_F54_ANALOG_CTRL19_03                          0x0131   /* Sense Frequency Control 2 3 */
  #define SYNA_F54_ANALOG_CTRL19_04                          0x0132   /* Sense Frequency Control 2 4 */
  #define SYNA_F54_ANALOG_CTRL19_05                          0x0133   /* Sense Frequency Control 2 5 */
  #define SYNA_F54_ANALOG_CTRL19_06                          0x0134   /* Sense Frequency Control 2 6 */
  #define SYNA_F54_ANALOG_CTRL19_07                          0x0135   /* Sense Frequency Control 2 7 */
  #define SYNA_F54_ANALOG_CTRL20                             0x0136   /* Noise Mitigation General Control */
  #define SYNA_F54_ANALOG_CTRL21_00                          0x0137   /* HNM Frequency Shift Noise Threshold LSB */
  #define SYNA_F54_ANALOG_CTRL21_01                          0x0138   /* HNM Frequency Shift Noise Threshold MSB */
  #define SYNA_F54_ANALOG_CTRL22                             0x0139   /* Reserved */
  #define SYNA_F54_ANALOG_CTRL23_00                          0x013A   /* Medium Noise Threshold LSB */
  #define SYNA_F54_ANALOG_CTRL23_01                          0x013B   /* Medium Noise Threshold MSB */
  #define SYNA_F54_ANALOG_CTRL24_00                          0x013C   /* High Noise Threshold LSB */
  #define SYNA_F54_ANALOG_CTRL24_01                          0x013D   /* High Noise Threshold MSB */
  #define SYNA_F54_ANALOG_CTRL25                             0x013E   /* FNM Frequency Shift Density */
  #define SYNA_F54_ANALOG_CTRL26                             0x013F   /* FNM Exit Threshold */
  #define SYNA_F54_ANALOG_CTRL27                             0x0140   /* IIR Filter Coefficient */
  #define SYNA_F54_ANALOG_CTRL28_00                          0x0141   /* Quiet Threshold LSB */
  #define SYNA_F54_ANALOG_CTRL28_01                          0x0142   /* Quiet Threshold MSB */
  #define SYNA_F54_ANALOG_CTRL29                             0x0143   /* Common-Mode Noise Control */
  #define SYNA_F54_ANALOG_CTRL30                             0x0144   /* CMN Filter Maximum */
  #define SYNA_F54_ANALOG_CTRL31                             0x0145   /* Touch Hysteresis */
  #define SYNA_F54_ANALOG_CTRL38_00                          0x0146   /* Noise Control 1 0 */
  #define SYNA_F54_ANALOG_CTRL38_01                          0x0147   /* Noise Control 1 1 */
  #define SYNA_F54_ANALOG_CTRL38_02                          0x0148   /* Noise Control 1 2 */
  #define SYNA_F54_ANALOG_CTRL38_03                          0x0149   /* Noise Control 1 3 */
  #define SYNA_F54_ANALOG_CTRL38_04                          0x014A   /* Noise Control 1 4 */
  #define SYNA_F54_ANALOG_CTRL38_05                          0x014B   /* Noise Control 1 5 */
  #define SYNA_F54_ANALOG_CTRL38_06                          0x014C   /* Noise Control 1 6 */
  #define SYNA_F54_ANALOG_CTRL38_07                          0x014D   /* Noise Control 1 7 */
  #define SYNA_F54_ANALOG_CTRL39_00                          0x014E   /* Noise Control 2 0 */
  #define SYNA_F54_ANALOG_CTRL39_01                          0x014F   /* Noise Control 2 1 */
  #define SYNA_F54_ANALOG_CTRL39_02                          0x0150   /* Noise Control 2 2 */
  #define SYNA_F54_ANALOG_CTRL39_03                          0x0151   /* Noise Control 2 3 */
  #define SYNA_F54_ANALOG_CTRL39_04                          0x0152   /* Noise Control 2 4 */
  #define SYNA_F54_ANALOG_CTRL39_05                          0x0153   /* Noise Control 2 5 */
  #define SYNA_F54_ANALOG_CTRL39_06                          0x0154   /* Noise Control 2 6 */
  #define SYNA_F54_ANALOG_CTRL39_07                          0x0155   /* Noise Control 2 7 */
  #define SYNA_F54_ANALOG_CTRL40_00                          0x0156   /* Noise Control 3 0 */
  #define SYNA_F54_ANALOG_CTRL40_01                          0x0157   /* Noise Control 3 1 */
  #define SYNA_F54_ANALOG_CTRL40_02                          0x0158   /* Noise Control 3 2 */
  #define SYNA_F54_ANALOG_CTRL40_03                          0x0159   /* Noise Control 3 3 */
  #define SYNA_F54_ANALOG_CTRL40_04                          0x015A   /* Noise Control 3 4 */
  #define SYNA_F54_ANALOG_CTRL40_05                          0x015B   /* Noise Control 3 5 */
  #define SYNA_F54_ANALOG_CTRL40_06                          0x015C   /* Noise Control 3 6 */
  #define SYNA_F54_ANALOG_CTRL40_07                          0x015D   /* Noise Control 3 7 */
  #define SYNA_F54_ANALOG_CTRL41                             0x015E   /* Multi Metric Noise Mitigation Control */
  #define SYNA_F54_ANALOG_CTRL42_00                          0x015F   /* Variance Metric Threshold LSB */
  #define SYNA_F54_ANALOG_CTRL42_01                          0x0160   /* Variance Metric Threshold MSB */
  #define SYNA_F54_ANALOG_CTRL55                             0x0161   /* 0D Slow Relaxation Rate */
  #define SYNA_F54_ANALOG_CTRL56                             0x0162   /* 0D Fast Relaxation Rate */
  #define SYNA_F54_ANALOG_CTRL57                             0x0163   /* 0D CBC Settings */
  #define SYNA_F54_ANALOG_CTRL58                             0x0164   /* 0D Acquisition Scheme */
  #define SYNA_F54_ANALOG_CTRL75_00                          0x0165   /* Sense Frequency Control6 0 */
  #define SYNA_F54_ANALOG_CTRL75_01                          0x0166   /* Sense Frequency Control6 1 */
  #define SYNA_F54_ANALOG_CTRL75_02                          0x0167   /* Sense Frequency Control6 2 */
  #define SYNA_F54_ANALOG_CTRL75_03                          0x0168   /* Sense Frequency Control6 3 */
  #define SYNA_F54_ANALOG_CTRL75_04                          0x0169   /* Sense Frequency Control6 4 */
  #define SYNA_F54_ANALOG_CTRL75_05                          0x016A   /* Sense Frequency Control6 5 */
  #define SYNA_F54_ANALOG_CTRL75_06                          0x016B   /* Sense Frequency Control6 6 */
  #define SYNA_F54_ANALOG_CTRL75_07                          0x016C   /* Sense Frequency Control6 7 */
  #define SYNA_F54_ANALOG_CTRL76                             0x016D   /* SFR Timer */
  #define SYNA_F54_ANALOG_CTRL77                             0x016E   /* ESD Control */
  #define SYNA_F54_ANALOG_CTRL78                             0x016F   /* ESD Threshold */
  #define SYNA_F54_ANALOG_CTRL84                             0x0170   /* Energy Threshold */
  #define SYNA_F54_ANALOG_CTRL85                             0x0171   /* Ratio Threshold */
  #define SYNA_F54_ANALOG_CMD00                              0x0172   /* Analog Command 0 */
  #define SYNA_F54_ANALOG_QUERY00                            0x0173   /* Device Capability Information */
  #define SYNA_F54_ANALOG_QUERY01                            0x0174   /* Device Capability Information */
  #define SYNA_F54_ANALOG_QUERY02                            0x0175   /* Image Reporting Modes */
  #define SYNA_F54_ANALOG_QUERY03_00                         0x0176   /* Clock Rate LSB */
  #define SYNA_F54_ANALOG_QUERY03_01                         0x0177   /* Clock Rate MSB */
  #define SYNA_F54_ANALOG_QUERY04                            0x0178   /* Touch Controller Family */
  #define SYNA_F54_ANALOG_QUERY05                            0x0179   /* Analog Hardware Controls */
  #define SYNA_F54_ANALOG_QUERY06                            0x017A   /* Data Acquisition Controls */
  #define SYNA_F54_ANALOG_QUERY07                            0x017B   /* Curved Lens Compensation */
  #define SYNA_F54_ANALOG_QUERY08                            0x017C   /* Data Acquisition Post-Processing Controls */
  #define SYNA_F54_ANALOG_QUERY09                            0x017D   /* Tuning Control 1 */
  #define SYNA_F54_ANALOG_QUERY10                            0x017E   /* Tuning Control 2 */
  #define SYNA_F54_ANALOG_QUERY11                            0x017F   /* Tuning Control 3 */
  #define SYNA_F54_ANALOG_QUERY12                            0x0180   /* Sense Frequency Control */

  /* Start of Page Description Table (PDT) for Page 0x01 */

  #define SYNA_PDT_P01_F54_ANALOG_QUERY_BASE                 0x01E9   /* Query Base */
  #define SYNA_PDT_P01_F54_ANALOG_COMMAND_BASE               0x01EA   /* Command Base */
  #define SYNA_PDT_P01_F54_ANALOG_CONTROL_BASE               0x01EB   /* Control Base */
  #define SYNA_PDT_P01_F54_ANALOG_DATA_BASE                  0x01EC   /* Data Base */
  #define SYNA_PDT_P01_F54_ANALOG_INTERRUPTS                 0x01ED   /* Version & Interrupt Count */
  #define SYNA_PDT_P01_F54_ANALOG_EXISTS                     0x01EE   /* Function Exists */
  #define SYNA_P01_PDT_PROPERTIES                            0x01EF   /* P01_PDT Properties */
  #define SYNA_P01_PAGESELECT                                0x01FF   /* Page Select register */

  /* Registers on Page 0x02 */

  /*      Register Name                                      Address     Register Description */
  /*      -------------                                      -------     -------------------- */
  #define SYNA_F1A_0D_DATA00                                 0x0200   /* Button Data */
  #define SYNA_F31_LED_CTRL00_00                             0x0201   /* LED Brightness 0 */
  #define SYNA_F31_LED_CTRL00_01                             0x0202   /* LED Brightness 1 */
  #define SYNA_F31_LED_CTRL00_02                             0x0203   /* LED Brightness 2 */
  #define SYNA_F31_LED_CTRL00_03                             0x0204   /* LED Brightness 3 */
  #define SYNA_F31_LED_CTRL00_04                             0x0205   /* LED Brightness 4 */
  #define SYNA_F31_LED_CTRL00_05                             0x0206   /* LED Brightness 5 */
  #define SYNA_F31_LED_CTRL00_06                             0x0207   /* LED Brightness 6 */
  #define SYNA_F31_LED_CTRL00_07                             0x0208   /* LED Brightness 7 */
  #define SYNA_F1A_0D_CTRL00                                 0x0209   /* General Control */
  #define SYNA_F1A_0D_CTRL01                                 0x020A   /* Button Interrupt Enable */
  #define SYNA_F1A_0D_CTRL02                                 0x020B   /* MultiButton Group Selection */
  #define SYNA_F1A_0D_CTRL03_00                              0x020C   /* Tx Button 0 */
  #define SYNA_F1A_0D_CTRL04_00                              0x020D   /* Rx Button 0 */
  #define SYNA_F1A_0D_CTRL03_01                              0x020E   /* Tx Button 1 */
  #define SYNA_F1A_0D_CTRL04_01                              0x020F   /* Rx Button 1 */
  #define SYNA_F1A_0D_CTRL03_02                              0x0210   /* Tx Button 2 */
  #define SYNA_F1A_0D_CTRL04_02                              0x0211   /* Rx Button 2 */
  #define SYNA_F1A_0D_CTRL03_03                              0x0212   /* Tx Button 3 */
  #define SYNA_F1A_0D_CTRL04_03                              0x0213   /* Rx Button 3 */
  #define SYNA_F1A_0D_CTRL03_04                              0x0214   /* Tx Button 4 */
  #define SYNA_F1A_0D_CTRL04_04                              0x0215   /* Rx Button 4 */
  #define SYNA_F1A_0D_CTRL03_05                              0x0216   /* Tx Button 5 */
  #define SYNA_F1A_0D_CTRL04_05                              0x0217   /* Rx Button 5 */
  #define SYNA_F1A_0D_CTRL03_06                              0x0218   /* Tx Button 6 */
  #define SYNA_F1A_0D_CTRL04_06                              0x0219   /* Rx Button 6 */
  #define SYNA_F1A_0D_CTRL03_07                              0x021A   /* Tx Button 7 */
  #define SYNA_F1A_0D_CTRL04_07                              0x021B   /* Rx Button 7 */
  #define SYNA_F1A_0D_CTRL05_00                              0x021C   /* Touch Threshold Button 0 */
  #define SYNA_F1A_0D_CTRL05_01                              0x021D   /* Touch Threshold Button 1 */
  #define SYNA_F1A_0D_CTRL05_02                              0x021E   /* Touch Threshold Button 2 */
  #define SYNA_F1A_0D_CTRL05_03                              0x021F   /* Touch Threshold Button 3 */
  #define SYNA_F1A_0D_CTRL05_04                              0x0220   /* Touch Threshold Button 4 */
  #define SYNA_F1A_0D_CTRL05_05                              0x0221   /* Touch Threshold Button 5 */
  #define SYNA_F1A_0D_CTRL05_06                              0x0222   /* Touch Threshold Button 6 */
  #define SYNA_F1A_0D_CTRL05_07                              0x0223   /* Touch Threshold Button 7 */
  #define SYNA_F1A_0D_CTRL06                                 0x0224   /* Release Threshold */
  #define SYNA_F1A_0D_CTRL07                                 0x0225   /* Strongest Button Hysteresis */
  #define SYNA_F1A_0D_CTRL08                                 0x0226   /* Filter Strength */
  #define SYNA_F31_LED_QUERY00                               0x0227   /* General LED Controls  */
  #define SYNA_F31_LED_QUERY01                               0x0228   /* Number of Leds */
  #define SYNA_F1A_0D_QUERY00                                0x0229   /* Maximum Button Count */
  #define SYNA_F1A_0D_QUERY01                                0x022A   /* Button Features */

  /* Start of Page Description Table (PDT) for Page 0x02 */

  #define SYNA_PDT_P02_F1A_0D_QUERY_BASE                     0x02E3   /* Query Base */
  #define SYNA_PDT_P02_F1A_0D_COMMAND_BASE                   0x02E4   /* Command Base */
  #define SYNA_PDT_P02_F1A_0D_CONTROL_BASE                   0x02E5   /* Control Base */
  #define SYNA_PDT_P02_F1A_0D_DATA_BASE                      0x02E6   /* Data Base */
  #define SYNA_PDT_P02_F1A_0D_INTERRUPTS                     0x02E7   /* Version & Interrupt Count */
  #define SYNA_PDT_P02_F1A_0D_EXISTS                         0x02E8   /* Function Exists */
  #define SYNA_PDT_P02_F31_LED_QUERY_BASE                    0x02E9   /* Query Base */
  #define SYNA_PDT_P02_F31_LED_COMMAND_BASE                  0x02EA   /* Command Base */
  #define SYNA_PDT_P02_F31_LED_CONTROL_BASE                  0x02EB   /* Control Base */
  #define SYNA_PDT_P02_F31_LED_DATA_BASE                     0x02EC   /* Data Base */
  #define SYNA_PDT_P02_F31_LED_INTERRUPTS                    0x02ED   /* Version & Interrupt Count */
  #define SYNA_PDT_P02_F31_LED_EXISTS                        0x02EE   /* Function Exists */
  #define SYNA_P02_PDT_PROPERTIES                            0x02EF   /* P02_PDT Properties */
  #define SYNA_P02_PAGESELECT                                0x02FF   /* Page Select register */

  /* Registers on Page 0x03 */

  /*      Register Name                                      Address     Register Description */
  /*      -------------                                      -------     -------------------- */
  #define SYNA_F55_SENSOR_CTRL00                             0x0300   /* General Control */
  #define SYNA_F55_SENSOR_CTRL01_00_00                       0x0301   /* Sensor Rx Mapping 0 */
  #define SYNA_F55_SENSOR_CTRL01_00_01                       0x0301   /* Sensor Rx Mapping 1 */
  #define SYNA_F55_SENSOR_CTRL01_00_02                       0x0301   /* Sensor Rx Mapping 2 */
  #define SYNA_F55_SENSOR_CTRL01_00_03                       0x0301   /* Sensor Rx Mapping 3 */
  #define SYNA_F55_SENSOR_CTRL01_00_04                       0x0301   /* Sensor Rx Mapping 4 */
  #define SYNA_F55_SENSOR_CTRL01_00_05                       0x0301   /* Sensor Rx Mapping 5 */
  #define SYNA_F55_SENSOR_CTRL01_00_06                       0x0301   /* Sensor Rx Mapping 6 */
  #define SYNA_F55_SENSOR_CTRL01_00_07                       0x0301   /* Sensor Rx Mapping 7 */
  #define SYNA_F55_SENSOR_CTRL01_00_08                       0x0301   /* Sensor Rx Mapping 8 */
  #define SYNA_F55_SENSOR_CTRL01_00_09                       0x0301   /* Sensor Rx Mapping 9 */
  #define SYNA_F55_SENSOR_CTRL01_00_10                       0x0301   /* Sensor Rx Mapping 10 */
  #define SYNA_F55_SENSOR_CTRL01_00_11                       0x0301   /* Sensor Rx Mapping 11 */
  #define SYNA_F55_SENSOR_CTRL01_00_12                       0x0301   /* Sensor Rx Mapping 12 */
  #define SYNA_F55_SENSOR_CTRL01_00_13                       0x0301   /* Sensor Rx Mapping 13 */
  #define SYNA_F55_SENSOR_CTRL01_00_14                       0x0301   /* Sensor Rx Mapping 14 */
  #define SYNA_F55_SENSOR_CTRL01_00_15                       0x0301   /* Sensor Rx Mapping 15 */
  #define SYNA_F55_SENSOR_CTRL01_00_16                       0x0301   /* Sensor Rx Mapping 16 */
  #define SYNA_F55_SENSOR_CTRL01_00_17                       0x0301   /* Sensor Rx Mapping 17 */
  #define SYNA_F55_SENSOR_CTRL02_00_00                       0x0302   /* Sensor Tx Mapping 0 */
  #define SYNA_F55_SENSOR_CTRL02_00_01                       0x0302   /* Sensor Tx Mapping 1 */
  #define SYNA_F55_SENSOR_CTRL02_00_02                       0x0302   /* Sensor Tx Mapping 2 */
  #define SYNA_F55_SENSOR_CTRL02_00_03                       0x0302   /* Sensor Tx Mapping 3 */
  #define SYNA_F55_SENSOR_CTRL02_00_04                       0x0302   /* Sensor Tx Mapping 4 */
  #define SYNA_F55_SENSOR_CTRL02_00_05                       0x0302   /* Sensor Tx Mapping 5 */
  #define SYNA_F55_SENSOR_CTRL02_00_06                       0x0302   /* Sensor Tx Mapping 6 */
  #define SYNA_F55_SENSOR_CTRL02_00_07                       0x0302   /* Sensor Tx Mapping 7 */
  #define SYNA_F55_SENSOR_CTRL02_00_08                       0x0302   /* Sensor Tx Mapping 8 */
  #define SYNA_F55_SENSOR_CTRL02_00_09                       0x0302   /* Sensor Tx Mapping 9 */
  #define SYNA_F55_SENSOR_CTRL02_00_10                       0x0302   /* Sensor Tx Mapping 10 */
  #define SYNA_F55_SENSOR_CTRL03_00_00                       0x0303   /* Edge Compensation 0 */
  #define SYNA_F55_SENSOR_CTRL03_00_01                       0x0303   /* Edge Compensation 1 */
  #define SYNA_F55_SENSOR_CTRL03_00_02                       0x0303   /* Edge Compensation 2 */
  #define SYNA_F55_SENSOR_CTRL03_00_03                       0x0303   /* Edge Compensation 3 */
  #define SYNA_F55_SENSOR_CTRL03_00_04                       0x0303   /* Edge Compensation 4 */
  #define SYNA_F55_SENSOR_CTRL03_00_05                       0x0303   /* Edge Compensation 5 */
  #define SYNA_F55_SENSOR_CTRL03_00_06                       0x0303   /* Edge Compensation 6 */
  #define SYNA_F55_SENSOR_CTRL03_00_07                       0x0303   /* Edge Compensation 7 */
  #define SYNA_F55_SENSOR_CTRL04_00_00                       0x0304   /* Axis 1 Compensation 0 */
  #define SYNA_F55_SENSOR_CTRL04_00_01                       0x0304   /* Axis 1 Compensation 1 */
  #define SYNA_F55_SENSOR_CTRL04_00_02                       0x0304   /* Axis 1 Compensation 2 */
  #define SYNA_F55_SENSOR_CTRL04_00_03                       0x0304   /* Axis 1 Compensation 3 */
  #define SYNA_F55_SENSOR_CTRL04_00_04                       0x0304   /* Axis 1 Compensation 4 */
  #define SYNA_F55_SENSOR_CTRL04_00_05                       0x0304   /* Axis 1 Compensation 5 */
  #define SYNA_F55_SENSOR_CTRL04_00_06                       0x0304   /* Axis 1 Compensation 6 */
  #define SYNA_F55_SENSOR_CTRL04_00_07                       0x0304   /* Axis 1 Compensation 7 */
  #define SYNA_F55_SENSOR_CTRL04_00_08                       0x0304   /* Axis 1 Compensation 8 */
  #define SYNA_F55_SENSOR_CTRL04_00_09                       0x0304   /* Axis 1 Compensation 9 */
  #define SYNA_F55_SENSOR_CTRL04_00_10                       0x0304   /* Axis 1 Compensation 10 */
  #define SYNA_F55_SENSOR_CTRL04_00_11                       0x0304   /* Axis 1 Compensation 11 */
  #define SYNA_F55_SENSOR_CTRL04_00_12                       0x0304   /* Axis 1 Compensation 12 */
  #define SYNA_F55_SENSOR_CTRL04_00_13                       0x0304   /* Axis 1 Compensation 13 */
  #define SYNA_F55_SENSOR_CTRL04_00_14                       0x0304   /* Axis 1 Compensation 14 */
  #define SYNA_F55_SENSOR_CTRL04_00_15                       0x0304   /* Axis 1 Compensation 15 */
  #define SYNA_F55_SENSOR_CTRL04_00_16                       0x0304   /* Axis 1 Compensation 16 */
  #define SYNA_F55_SENSOR_CTRL04_00_17                       0x0304   /* Axis 1 Compensation 17 */
  #define SYNA_F55_SENSOR_CTRL05_00_00                       0x0305   /* Axis 2 Compensation 0 */
  #define SYNA_F55_SENSOR_CTRL05_00_01                       0x0305   /* Axis 2 Compensation 1 */
  #define SYNA_F55_SENSOR_CTRL05_00_02                       0x0305   /* Axis 2 Compensation 2 */
  #define SYNA_F55_SENSOR_CTRL05_00_03                       0x0305   /* Axis 2 Compensation 3 */
  #define SYNA_F55_SENSOR_CTRL05_00_04                       0x0305   /* Axis 2 Compensation 4 */
  #define SYNA_F55_SENSOR_CTRL05_00_05                       0x0305   /* Axis 2 Compensation 5 */
  #define SYNA_F55_SENSOR_CTRL05_00_06                       0x0305   /* Axis 2 Compensation 6 */
  #define SYNA_F55_SENSOR_CTRL05_00_07                       0x0305   /* Axis 2 Compensation 7 */
  #define SYNA_F55_SENSOR_CTRL05_00_08                       0x0305   /* Axis 2 Compensation 8 */
  #define SYNA_F55_SENSOR_CTRL05_00_09                       0x0305   /* Axis 2 Compensation 9 */
  #define SYNA_F55_SENSOR_CTRL05_00_10                       0x0305   /* Axis 2 Compensation 10 */
  #define SYNA_F55_SENSOR_CMD00                              0x0306   /* Analog Command 0 */
  #define SYNA_F55_SENSOR_QUERY00                            0x0307   /* Sensor Information */
  #define SYNA_F55_SENSOR_QUERY01                            0x0308   /* Sensor Information */
  #define SYNA_F55_SENSOR_QUERY02                            0x0309   /* Sensor Controls */

  /* Start of Page Description Table (PDT) for Page 0x03 */

  #define SYNA_PDT_P03_F55_SENSOR_QUERY_BASE                 0x03E9   /* Query Base */
  #define SYNA_PDT_P03_F55_SENSOR_COMMAND_BASE               0x03EA   /* Command Base */
  #define SYNA_PDT_P03_F55_SENSOR_CONTROL_BASE               0x03EB   /* Control Base */
  #define SYNA_PDT_P03_F55_SENSOR_DATA_BASE                  0x03EC   /* Data Base */
  #define SYNA_PDT_P03_F55_SENSOR_INTERRUPTS                 0x03ED   /* Version & Interrupt Count */
  #define SYNA_PDT_P03_F55_SENSOR_EXISTS                     0x03EE   /* Function Exists */
  #define SYNA_P03_PDT_PROPERTIES                            0x03EF   /* P03_PDT Properties */
  #define SYNA_P03_PAGESELECT                                0x03FF   /* Page Select register */

  /* Offsets within the configuration block */

  /*      Register Name                                      Offset      Register Description */
  /*      -------------                                      ------      -------------------- */
  #define SYNA_F34_FLASH_CTRL00_00_CFGBLK_OFS                0x0000   /* Customer Defined Config ID 0 */
  #define SYNA_F34_FLASH_CTRL00_01_CFGBLK_OFS                0x0001   /* Customer Defined Config ID 1 */
  #define SYNA_F34_FLASH_CTRL00_02_CFGBLK_OFS                0x0002   /* Customer Defined Config ID 2 */
  #define SYNA_F34_FLASH_CTRL00_03_CFGBLK_OFS                0x0003   /* Customer Defined Config ID 3 */
  #define SYNA_F01_RMI_CTRL00_CFGBLK_OFS                     0x0004   /* Device Control */
  #define SYNA_F01_RMI_CTRL01_00_CFGBLK_OFS                  0x0005   /* Interrupt Enable 0 */
  #define SYNA_F01_RMI_CTRL02_CFGBLK_OFS                     0x0006   /* Doze Interval */
  #define SYNA_F01_RMI_CTRL03_CFGBLK_OFS                     0x0007   /* Doze Wakeup Threshold */
  #define SYNA_F01_RMI_CTRL05_CFGBLK_OFS                     0x0008   /* Doze Holdoff */
  #define SYNA_F01_RMI_CTRL09_CFGBLK_OFS                     0x0009   /* Doze Recalibration Interval */
  #define SYNA_F11_2D_CTRL00_CFGBLK_OFS                      0x000A   /* 2D Report Mode */
  #define SYNA_F11_2D_CTRL01_CFGBLK_OFS                      0x000B   /* 2D Palm Detect */
  #define SYNA_F11_2D_CTRL02_CFGBLK_OFS                      0x000C   /* 2D Delta-X Thresh */
  #define SYNA_F11_2D_CTRL03_CFGBLK_OFS                      0x000D   /* 2D Delta-Y Thresh */
  #define SYNA_F11_2D_CTRL04_CFGBLK_OFS                      0x000E   /* 2D Velocity */
  #define SYNA_F11_2D_CTRL05_CFGBLK_OFS                      0x000F   /* 2D Acceleration */
  #define SYNA_F11_2D_CTRL06_CFGBLK_OFS                      0x0010   /* 2D Max X Position (7:0) */
  #define SYNA_F11_2D_CTRL07_CFGBLK_OFS                      0x0011   /* 2D Max X Position (11:8) */
  #define SYNA_F11_2D_CTRL08_CFGBLK_OFS                      0x0012   /* 2D Max Y Position (7:0) */
  #define SYNA_F11_2D_CTRL09_CFGBLK_OFS                      0x0013   /* 2D Max Y Position (11:8) */
  #define SYNA_F11_2D_CTRL29_CFGBLK_OFS                      0x0014   /* Z Touch Threshold */
  #define SYNA_F11_2D_CTRL30_CFGBLK_OFS                      0x0015   /* Z Hysteresis */
  #define SYNA_F11_2D_CTRL31_CFGBLK_OFS                      0x0016   /* Small Z Threshold */
  #define SYNA_F11_2D_CTRL32_00_CFGBLK_OFS                   0x0017   /* Small Z Scale Factor */
  #define SYNA_F11_2D_CTRL32_01_CFGBLK_OFS                   0x0018   /* Small Z Scale Factor */
  #define SYNA_F11_2D_CTRL33_00_CFGBLK_OFS                   0x0019   /* Large Z Scale Factor */
  #define SYNA_F11_2D_CTRL33_01_CFGBLK_OFS                   0x001A   /* Large Z Scale Factor */
  #define SYNA_F11_2D_CTRL34_CFGBLK_OFS                      0x001B   /* Position Calculation & Post Correction */
  #define SYNA_F11_2D_CTRL36_CFGBLK_OFS                      0x001C   /* Wx Scale Factor */
  #define SYNA_F11_2D_CTRL37_CFGBLK_OFS                      0x001D   /* Wx Offset */
  #define SYNA_F11_2D_CTRL38_CFGBLK_OFS                      0x001E   /* Wy Scale Factor */
  #define SYNA_F11_2D_CTRL39_CFGBLK_OFS                      0x001F   /* Wy Offset */
  #define SYNA_F11_2D_CTRL40_00_CFGBLK_OFS                   0x0020   /* X Pitch */
  #define SYNA_F11_2D_CTRL40_01_CFGBLK_OFS                   0x0021   /* X Pitch */
  #define SYNA_F11_2D_CTRL41_00_CFGBLK_OFS                   0x0022   /* Y Pitch */
  #define SYNA_F11_2D_CTRL41_01_CFGBLK_OFS                   0x0023   /* Y Pitch */
  #define SYNA_F11_2D_CTRL42_00_CFGBLK_OFS                   0x0024   /* Default Finger Width Tx */
  #define SYNA_F11_2D_CTRL42_01_CFGBLK_OFS                   0x0025   /* Default Finger Width Tx */
  #define SYNA_F11_2D_CTRL43_00_CFGBLK_OFS                   0x0026   /* Default Finger Width Ty */
  #define SYNA_F11_2D_CTRL43_01_CFGBLK_OFS                   0x0027   /* Default Finger Width Ty */
  #define SYNA_F11_2D_CTRL44_CFGBLK_OFS                      0x0028   /* Report Finger Width */
  #define SYNA_F11_2D_CTRL45_CFGBLK_OFS                      0x0029   /* Segmentation Aggressiveness */
  #define SYNA_F11_2D_CTRL46_CFGBLK_OFS                      0x002A   /* Rx Clip LSB */
  #define SYNA_F11_2D_CTRL47_CFGBLK_OFS                      0x002B   /* Rx Clip MSB */
  #define SYNA_F11_2D_CTRL48_CFGBLK_OFS                      0x002C   /* Tx Clip LSB */
  #define SYNA_F11_2D_CTRL49_CFGBLK_OFS                      0x002D   /* Tx Clip MSB */
  #define SYNA_F11_2D_CTRL50_CFGBLK_OFS                      0x002E   /* Minimum Drumming Separation */
  #define SYNA_F11_2D_CTRL51_CFGBLK_OFS                      0x002F   /* Maximum Drumming Movement */
  #define SYNA_F11_2D_CTRL58_CFGBLK_OFS                      0x0030   /* Large Object Suppression Parameters */
  #define SYNA_F11_2D_CTRL77_CFGBLK_OFS                      0x0031   /* 2D RX Number */
  #define SYNA_F11_2D_CTRL78_CFGBLK_OFS                      0x0032   /* 2D TX Number */
  #define SYNA_F11_2D_CTRL80_CFGBLK_OFS                      0x0033   /* Finger Limit */
  #define SYNA_F11_2D_CTRL89_CFGBLK_OFS                      0x0034   /* Jitter Filter 2 Strength */
  #define SYNA_F11_2D_CTRL90_00_00_CFGBLK_OFS                0x0035   /* Land/Lift Control */
  #define SYNA_F11_2D_CTRL90_00_01_CFGBLK_OFS                0x0036   /* Land Thresholds */
  #define SYNA_F11_2D_CTRL90_00_02_CFGBLK_OFS                0x0037   /* Buffering Distance */
  #define SYNA_F11_2D_CTRL90_00_03_CFGBLK_OFS                0x0038   /* Lift Z Differential */
  #define SYNA_F11_2D_CTRL91_00_00_CFGBLK_OFS                0x0039   /* Group Decomposition Control */
  #define SYNA_F11_2D_CTRL91_00_01_CFGBLK_OFS                0x003A   /* Group Decomposition Control */
  #define SYNA_F11_2D_CTRL91_00_02_CFGBLK_OFS                0x003B   /* Group Decomposition Control */
  #define SYNA_F11_2D_CTRL92_00_00_CFGBLK_OFS                0x003C   /* LPWG Control */
  #define SYNA_F11_2D_CTRL92_01_00_CFGBLK_OFS                0x003D   /* Double-Tap Zone 0 X LSB */
  #define SYNA_F11_2D_CTRL92_01_01_CFGBLK_OFS                0x003E   /* Double-Tap Zone 0 Y LSB */
  #define SYNA_F11_2D_CTRL92_01_02_CFGBLK_OFS                0x003F   /* Double-Tap Zone 0 XY MSB */
  #define SYNA_F11_2D_CTRL92_01_03_CFGBLK_OFS                0x0040   /* Double-Tap Zone 1 X LSB */
  #define SYNA_F11_2D_CTRL92_01_04_CFGBLK_OFS                0x0041   /* Double-Tap Zone 1 Y LSB */
  #define SYNA_F11_2D_CTRL92_01_05_CFGBLK_OFS                0x0042   /* Double-Tap Zone 1 XY MSB */
  #define SYNA_F11_2D_CTRL92_01_06_CFGBLK_OFS                0x0043   /* Maximum Tap Time */
  #define SYNA_F11_2D_CTRL92_01_07_CFGBLK_OFS                0x0044   /* Maximum Tap Distance */
  #define SYNA_F11_2D_CTRL92_02_00_CFGBLK_OFS                0x0045   /* Swipe Zone 0 X LSB */
  #define SYNA_F11_2D_CTRL92_02_01_CFGBLK_OFS                0x0046   /* Swipe Zone 0 Y LSB */
  #define SYNA_F11_2D_CTRL92_02_02_CFGBLK_OFS                0x0047   /* Swipe Zone 0 XY MSB */
  #define SYNA_F11_2D_CTRL92_02_03_CFGBLK_OFS                0x0048   /* Swipe Zone 1 X LSB */
  #define SYNA_F11_2D_CTRL92_02_04_CFGBLK_OFS                0x0049   /* Swipe Zone 1 Y LSB */
  #define SYNA_F11_2D_CTRL92_02_05_CFGBLK_OFS                0x004A   /* Swipe Zone 1 XY MSB */
  #define SYNA_F11_2D_CTRL92_02_06_CFGBLK_OFS                0x004B   /* Swipe Minimum Distance */
  #define SYNA_F11_2D_CTRL92_02_07_CFGBLK_OFS                0x004C   /* Swipe Minimum Speed */
  #define SYNA_F54_ANALOG_CTRL00_CFGBLK_OFS                  0x004D   /* General Control */
  #define SYNA_F54_ANALOG_CTRL01_CFGBLK_OFS                  0x004E   /* General Control 1 */
  #define SYNA_F54_ANALOG_CTRL02_00_CFGBLK_OFS               0x004F   /* Saturation Capacitance LSB */
  #define SYNA_F54_ANALOG_CTRL02_01_CFGBLK_OFS               0x0050   /* Saturation Capacitance MSB */
  #define SYNA_F54_ANALOG_CTRL03_CFGBLK_OFS                  0x0051   /* Pixel Touch Threshold */
  #define SYNA_F54_ANALOG_CTRL04_CFGBLK_OFS                  0x0052   /* Misc Analog Control */
  #define SYNA_F54_ANALOG_CTRL05_CFGBLK_OFS                  0x0053   /* RefCap RefLo Settings */
  #define SYNA_F54_ANALOG_CTRL06_CFGBLK_OFS                  0x0054   /* RefCap RefHi Settings */
  #define SYNA_F54_ANALOG_CTRL07_CFGBLK_OFS                  0x0055   /* CBC Cap Settings */
  #define SYNA_F54_ANALOG_CTRL08_00_CFGBLK_OFS               0x0056   /* Integration Duration LSB */
  #define SYNA_F54_ANALOG_CTRL08_01_CFGBLK_OFS               0x0057   /* Integration Duration MSB */
  #define SYNA_F54_ANALOG_CTRL09_CFGBLK_OFS                  0x0058   /* Reset Duration */
  #define SYNA_F54_ANALOG_CTRL10_CFGBLK_OFS                  0x0059   /* Noise Measurement Control */
  #define SYNA_F54_ANALOG_CTRL11_00_CFGBLK_OFS               0x005A   /* Reserved */
  #define SYNA_F54_ANALOG_CTRL11_01_CFGBLK_OFS               0x005B   /* Reserved */
  #define SYNA_F54_ANALOG_CTRL12_CFGBLK_OFS                  0x005C   /* Slow Relaxation Rate */
  #define SYNA_F54_ANALOG_CTRL13_CFGBLK_OFS                  0x005D   /* Fast Relaxation Rate */
  #define SYNA_F54_ANALOG_CTRL17_00_CFGBLK_OFS               0x005E   /* Sense Frequency Control 0 0 */
  #define SYNA_F54_ANALOG_CTRL17_01_CFGBLK_OFS               0x005F   /* Sense Frequency Control 0 1 */
  #define SYNA_F54_ANALOG_CTRL17_02_CFGBLK_OFS               0x0060   /* Sense Frequency Control 0 2 */
  #define SYNA_F54_ANALOG_CTRL17_03_CFGBLK_OFS               0x0061   /* Sense Frequency Control 0 3 */
  #define SYNA_F54_ANALOG_CTRL17_04_CFGBLK_OFS               0x0062   /* Sense Frequency Control 0 4 */
  #define SYNA_F54_ANALOG_CTRL17_05_CFGBLK_OFS               0x0063   /* Sense Frequency Control 0 5 */
  #define SYNA_F54_ANALOG_CTRL17_06_CFGBLK_OFS               0x0064   /* Sense Frequency Control 0 6 */
  #define SYNA_F54_ANALOG_CTRL17_07_CFGBLK_OFS               0x0065   /* Sense Frequency Control 0 7 */
  #define SYNA_F54_ANALOG_CTRL18_00_CFGBLK_OFS               0x0066   /* Sense Frequency Control 1 0 */
  #define SYNA_F54_ANALOG_CTRL18_01_CFGBLK_OFS               0x0067   /* Sense Frequency Control 1 1 */
  #define SYNA_F54_ANALOG_CTRL18_02_CFGBLK_OFS               0x0068   /* Sense Frequency Control 1 2 */
  #define SYNA_F54_ANALOG_CTRL18_03_CFGBLK_OFS               0x0069   /* Sense Frequency Control 1 3 */
  #define SYNA_F54_ANALOG_CTRL18_04_CFGBLK_OFS               0x006A   /* Sense Frequency Control 1 4 */
  #define SYNA_F54_ANALOG_CTRL18_05_CFGBLK_OFS               0x006B   /* Sense Frequency Control 1 5 */
  #define SYNA_F54_ANALOG_CTRL18_06_CFGBLK_OFS               0x006C   /* Sense Frequency Control 1 6 */
  #define SYNA_F54_ANALOG_CTRL18_07_CFGBLK_OFS               0x006D   /* Sense Frequency Control 1 7 */
  #define SYNA_F54_ANALOG_CTRL19_00_CFGBLK_OFS               0x006E   /* Sense Frequency Control 2 0 */
  #define SYNA_F54_ANALOG_CTRL19_01_CFGBLK_OFS               0x006F   /* Sense Frequency Control 2 1 */
  #define SYNA_F54_ANALOG_CTRL19_02_CFGBLK_OFS               0x0070   /* Sense Frequency Control 2 2 */
  #define SYNA_F54_ANALOG_CTRL19_03_CFGBLK_OFS               0x0071   /* Sense Frequency Control 2 3 */
  #define SYNA_F54_ANALOG_CTRL19_04_CFGBLK_OFS               0x0072   /* Sense Frequency Control 2 4 */
  #define SYNA_F54_ANALOG_CTRL19_05_CFGBLK_OFS               0x0073   /* Sense Frequency Control 2 5 */
  #define SYNA_F54_ANALOG_CTRL19_06_CFGBLK_OFS               0x0074   /* Sense Frequency Control 2 6 */
  #define SYNA_F54_ANALOG_CTRL19_07_CFGBLK_OFS               0x0075   /* Sense Frequency Control 2 7 */
  #define SYNA_F54_ANALOG_CTRL20_CFGBLK_OFS                  0x0076   /* Noise Mitigation General Control */
  #define SYNA_F54_ANALOG_CTRL21_00_CFGBLK_OFS               0x0077   /* HNM Frequency Shift Noise Threshold LSB */
  #define SYNA_F54_ANALOG_CTRL21_01_CFGBLK_OFS               0x0078   /* HNM Frequency Shift Noise Threshold MSB */
  #define SYNA_F54_ANALOG_CTRL22_CFGBLK_OFS                  0x0079   /* Reserved */
  #define SYNA_F54_ANALOG_CTRL23_00_CFGBLK_OFS               0x007A   /* Medium Noise Threshold LSB */
  #define SYNA_F54_ANALOG_CTRL23_01_CFGBLK_OFS               0x007B   /* Medium Noise Threshold MSB */
  #define SYNA_F54_ANALOG_CTRL24_00_CFGBLK_OFS               0x007C   /* High Noise Threshold LSB */
  #define SYNA_F54_ANALOG_CTRL24_01_CFGBLK_OFS               0x007D   /* High Noise Threshold MSB */
  #define SYNA_F54_ANALOG_CTRL25_CFGBLK_OFS                  0x007E   /* FNM Frequency Shift Density */
  #define SYNA_F54_ANALOG_CTRL26_CFGBLK_OFS                  0x007F   /* FNM Exit Threshold */
  #define SYNA_F54_ANALOG_CTRL27_CFGBLK_OFS                  0x0080   /* IIR Filter Coefficient */
  #define SYNA_F54_ANALOG_CTRL28_00_CFGBLK_OFS               0x0081   /* Quiet Threshold LSB */
  #define SYNA_F54_ANALOG_CTRL28_01_CFGBLK_OFS               0x0082   /* Quiet Threshold MSB */
  #define SYNA_F54_ANALOG_CTRL29_CFGBLK_OFS                  0x0083   /* Common-Mode Noise Control */
  #define SYNA_F54_ANALOG_CTRL30_CFGBLK_OFS                  0x0084   /* CMN Filter Maximum */
  #define SYNA_F54_ANALOG_CTRL31_CFGBLK_OFS                  0x0085   /* Touch Hysteresis */
  #define SYNA_F54_ANALOG_CTRL38_00_CFGBLK_OFS               0x0086   /* Noise Control 1 0 */
  #define SYNA_F54_ANALOG_CTRL38_01_CFGBLK_OFS               0x0087   /* Noise Control 1 1 */
  #define SYNA_F54_ANALOG_CTRL38_02_CFGBLK_OFS               0x0088   /* Noise Control 1 2 */
  #define SYNA_F54_ANALOG_CTRL38_03_CFGBLK_OFS               0x0089   /* Noise Control 1 3 */
  #define SYNA_F54_ANALOG_CTRL38_04_CFGBLK_OFS               0x008A   /* Noise Control 1 4 */
  #define SYNA_F54_ANALOG_CTRL38_05_CFGBLK_OFS               0x008B   /* Noise Control 1 5 */
  #define SYNA_F54_ANALOG_CTRL38_06_CFGBLK_OFS               0x008C   /* Noise Control 1 6 */
  #define SYNA_F54_ANALOG_CTRL38_07_CFGBLK_OFS               0x008D   /* Noise Control 1 7 */
  #define SYNA_F54_ANALOG_CTRL39_00_CFGBLK_OFS               0x008E   /* Noise Control 2 0 */
  #define SYNA_F54_ANALOG_CTRL39_01_CFGBLK_OFS               0x008F   /* Noise Control 2 1 */
  #define SYNA_F54_ANALOG_CTRL39_02_CFGBLK_OFS               0x0090   /* Noise Control 2 2 */
  #define SYNA_F54_ANALOG_CTRL39_03_CFGBLK_OFS               0x0091   /* Noise Control 2 3 */
  #define SYNA_F54_ANALOG_CTRL39_04_CFGBLK_OFS               0x0092   /* Noise Control 2 4 */
  #define SYNA_F54_ANALOG_CTRL39_05_CFGBLK_OFS               0x0093   /* Noise Control 2 5 */
  #define SYNA_F54_ANALOG_CTRL39_06_CFGBLK_OFS               0x0094   /* Noise Control 2 6 */
  #define SYNA_F54_ANALOG_CTRL39_07_CFGBLK_OFS               0x0095   /* Noise Control 2 7 */
  #define SYNA_F54_ANALOG_CTRL40_00_CFGBLK_OFS               0x0096   /* Noise Control 3 0 */
  #define SYNA_F54_ANALOG_CTRL40_01_CFGBLK_OFS               0x0097   /* Noise Control 3 1 */
  #define SYNA_F54_ANALOG_CTRL40_02_CFGBLK_OFS               0x0098   /* Noise Control 3 2 */
  #define SYNA_F54_ANALOG_CTRL40_03_CFGBLK_OFS               0x0099   /* Noise Control 3 3 */
  #define SYNA_F54_ANALOG_CTRL40_04_CFGBLK_OFS               0x009A   /* Noise Control 3 4 */
  #define SYNA_F54_ANALOG_CTRL40_05_CFGBLK_OFS               0x009B   /* Noise Control 3 5 */
  #define SYNA_F54_ANALOG_CTRL40_06_CFGBLK_OFS               0x009C   /* Noise Control 3 6 */
  #define SYNA_F54_ANALOG_CTRL40_07_CFGBLK_OFS               0x009D   /* Noise Control 3 7 */
  #define SYNA_F54_ANALOG_CTRL41_CFGBLK_OFS                  0x009E   /* Multi Metric Noise Mitigation Control */
  #define SYNA_F54_ANALOG_CTRL42_00_CFGBLK_OFS               0x009F   /* Variance Metric Threshold LSB */
  #define SYNA_F54_ANALOG_CTRL42_01_CFGBLK_OFS               0x00A0   /* Variance Metric Threshold MSB */
  #define SYNA_F54_ANALOG_CTRL55_CFGBLK_OFS                  0x00A1   /* 0D Slow Relaxation Rate */
  #define SYNA_F54_ANALOG_CTRL56_CFGBLK_OFS                  0x00A2   /* 0D Fast Relaxation Rate */
  #define SYNA_F54_ANALOG_CTRL57_CFGBLK_OFS                  0x00A3   /* 0D CBC Settings */
  #define SYNA_F54_ANALOG_CTRL58_CFGBLK_OFS                  0x00A4   /* 0D Acquisition Scheme */
  #define SYNA_F54_ANALOG_CTRL75_00_CFGBLK_OFS               0x00A5   /* Sense Frequency Control6 0 */
  #define SYNA_F54_ANALOG_CTRL75_01_CFGBLK_OFS               0x00A6   /* Sense Frequency Control6 1 */
  #define SYNA_F54_ANALOG_CTRL75_02_CFGBLK_OFS               0x00A7   /* Sense Frequency Control6 2 */
  #define SYNA_F54_ANALOG_CTRL75_03_CFGBLK_OFS               0x00A8   /* Sense Frequency Control6 3 */
  #define SYNA_F54_ANALOG_CTRL75_04_CFGBLK_OFS               0x00A9   /* Sense Frequency Control6 4 */
  #define SYNA_F54_ANALOG_CTRL75_05_CFGBLK_OFS               0x00AA   /* Sense Frequency Control6 5 */
  #define SYNA_F54_ANALOG_CTRL75_06_CFGBLK_OFS               0x00AB   /* Sense Frequency Control6 6 */
  #define SYNA_F54_ANALOG_CTRL75_07_CFGBLK_OFS               0x00AC   /* Sense Frequency Control6 7 */
  #define SYNA_F54_ANALOG_CTRL76_CFGBLK_OFS                  0x00AD   /* SFR Timer */
  #define SYNA_F54_ANALOG_CTRL77_CFGBLK_OFS                  0x00AE   /* ESD Control */
  #define SYNA_F54_ANALOG_CTRL78_CFGBLK_OFS                  0x00AF   /* ESD Threshold */
  #define SYNA_F54_ANALOG_CTRL84_CFGBLK_OFS                  0x00B0   /* Energy Threshold */
  #define SYNA_F54_ANALOG_CTRL85_CFGBLK_OFS                  0x00B1   /* Ratio Threshold */
  #define SYNA_F31_LED_CTRL00_00_CFGBLK_OFS                  0x00B2   /* LED Brightness 0 */
  #define SYNA_F31_LED_CTRL00_01_CFGBLK_OFS                  0x00B3   /* LED Brightness 1 */
  #define SYNA_F31_LED_CTRL00_02_CFGBLK_OFS                  0x00B4   /* LED Brightness 2 */
  #define SYNA_F31_LED_CTRL00_03_CFGBLK_OFS                  0x00B5   /* LED Brightness 3 */
  #define SYNA_F31_LED_CTRL00_04_CFGBLK_OFS                  0x00B6   /* LED Brightness 4 */
  #define SYNA_F31_LED_CTRL00_05_CFGBLK_OFS                  0x00B7   /* LED Brightness 5 */
  #define SYNA_F31_LED_CTRL00_06_CFGBLK_OFS                  0x00B8   /* LED Brightness 6 */
  #define SYNA_F31_LED_CTRL00_07_CFGBLK_OFS                  0x00B9   /* LED Brightness 7 */
  #define SYNA_F1A_0D_CTRL00_CFGBLK_OFS                      0x00BA   /* General Control */
  #define SYNA_F1A_0D_CTRL01_CFGBLK_OFS                      0x00BB   /* Button Interrupt Enable */
  #define SYNA_F1A_0D_CTRL02_CFGBLK_OFS                      0x00BC   /* MultiButton Group Selection */
  #define SYNA_F1A_0D_CTRL03_00_CFGBLK_OFS                   0x00BD   /* Tx Button 0 */
  #define SYNA_F1A_0D_CTRL04_00_CFGBLK_OFS                   0x00BE   /* Rx Button 0 */
  #define SYNA_F1A_0D_CTRL03_01_CFGBLK_OFS                   0x00BF   /* Tx Button 1 */
  #define SYNA_F1A_0D_CTRL04_01_CFGBLK_OFS                   0x00C0   /* Rx Button 1 */
  #define SYNA_F1A_0D_CTRL03_02_CFGBLK_OFS                   0x00C1   /* Tx Button 2 */
  #define SYNA_F1A_0D_CTRL04_02_CFGBLK_OFS                   0x00C2   /* Rx Button 2 */
  #define SYNA_F1A_0D_CTRL03_03_CFGBLK_OFS                   0x00C3   /* Tx Button 3 */
  #define SYNA_F1A_0D_CTRL04_03_CFGBLK_OFS                   0x00C4   /* Rx Button 3 */
  #define SYNA_F1A_0D_CTRL03_04_CFGBLK_OFS                   0x00C5   /* Tx Button 4 */
  #define SYNA_F1A_0D_CTRL04_04_CFGBLK_OFS                   0x00C6   /* Rx Button 4 */
  #define SYNA_F1A_0D_CTRL03_05_CFGBLK_OFS                   0x00C7   /* Tx Button 5 */
  #define SYNA_F1A_0D_CTRL04_05_CFGBLK_OFS                   0x00C8   /* Rx Button 5 */
  #define SYNA_F1A_0D_CTRL03_06_CFGBLK_OFS                   0x00C9   /* Tx Button 6 */
  #define SYNA_F1A_0D_CTRL04_06_CFGBLK_OFS                   0x00CA   /* Rx Button 6 */
  #define SYNA_F1A_0D_CTRL03_07_CFGBLK_OFS                   0x00CB   /* Tx Button 7 */
  #define SYNA_F1A_0D_CTRL04_07_CFGBLK_OFS                   0x00CC   /* Rx Button 7 */
  #define SYNA_F1A_0D_CTRL05_00_CFGBLK_OFS                   0x00CD   /* Touch Threshold Button 0 */
  #define SYNA_F1A_0D_CTRL05_01_CFGBLK_OFS                   0x00CE   /* Touch Threshold Button 1 */
  #define SYNA_F1A_0D_CTRL05_02_CFGBLK_OFS                   0x00CF   /* Touch Threshold Button 2 */
  #define SYNA_F1A_0D_CTRL05_03_CFGBLK_OFS                   0x00D0   /* Touch Threshold Button 3 */
  #define SYNA_F1A_0D_CTRL05_04_CFGBLK_OFS                   0x00D1   /* Touch Threshold Button 4 */
  #define SYNA_F1A_0D_CTRL05_05_CFGBLK_OFS                   0x00D2   /* Touch Threshold Button 5 */
  #define SYNA_F1A_0D_CTRL05_06_CFGBLK_OFS                   0x00D3   /* Touch Threshold Button 6 */
  #define SYNA_F1A_0D_CTRL05_07_CFGBLK_OFS                   0x00D4   /* Touch Threshold Button 7 */
  #define SYNA_F1A_0D_CTRL06_CFGBLK_OFS                      0x00D5   /* Release Threshold */
  #define SYNA_F1A_0D_CTRL07_CFGBLK_OFS                      0x00D6   /* Strongest Button Hysteresis */
  #define SYNA_F1A_0D_CTRL08_CFGBLK_OFS                      0x00D7   /* Filter Strength */
  #define SYNA_F55_SENSOR_CTRL00_CFGBLK_OFS                  0x00D8   /* General Control */
  #define SYNA_F55_SENSOR_CTRL01_00_00_CFGBLK_OFS            0x00D9   /* Sensor Rx Mapping 0 */
  #define SYNA_F55_SENSOR_CTRL01_00_01_CFGBLK_OFS            0x00DA   /* Sensor Rx Mapping 1 */
  #define SYNA_F55_SENSOR_CTRL01_00_02_CFGBLK_OFS            0x00DB   /* Sensor Rx Mapping 2 */
  #define SYNA_F55_SENSOR_CTRL01_00_03_CFGBLK_OFS            0x00DC   /* Sensor Rx Mapping 3 */
  #define SYNA_F55_SENSOR_CTRL01_00_04_CFGBLK_OFS            0x00DD   /* Sensor Rx Mapping 4 */
  #define SYNA_F55_SENSOR_CTRL01_00_05_CFGBLK_OFS            0x00DE   /* Sensor Rx Mapping 5 */
  #define SYNA_F55_SENSOR_CTRL01_00_06_CFGBLK_OFS            0x00DF   /* Sensor Rx Mapping 6 */
  #define SYNA_F55_SENSOR_CTRL01_00_07_CFGBLK_OFS            0x00E0   /* Sensor Rx Mapping 7 */
  #define SYNA_F55_SENSOR_CTRL01_00_08_CFGBLK_OFS            0x00E1   /* Sensor Rx Mapping 8 */
  #define SYNA_F55_SENSOR_CTRL01_00_09_CFGBLK_OFS            0x00E2   /* Sensor Rx Mapping 9 */
  #define SYNA_F55_SENSOR_CTRL01_00_10_CFGBLK_OFS            0x00E3   /* Sensor Rx Mapping 10 */
  #define SYNA_F55_SENSOR_CTRL01_00_11_CFGBLK_OFS            0x00E4   /* Sensor Rx Mapping 11 */
  #define SYNA_F55_SENSOR_CTRL01_00_12_CFGBLK_OFS            0x00E5   /* Sensor Rx Mapping 12 */
  #define SYNA_F55_SENSOR_CTRL01_00_13_CFGBLK_OFS            0x00E6   /* Sensor Rx Mapping 13 */
  #define SYNA_F55_SENSOR_CTRL01_00_14_CFGBLK_OFS            0x00E7   /* Sensor Rx Mapping 14 */
  #define SYNA_F55_SENSOR_CTRL01_00_15_CFGBLK_OFS            0x00E8   /* Sensor Rx Mapping 15 */
  #define SYNA_F55_SENSOR_CTRL01_00_16_CFGBLK_OFS            0x00E9   /* Sensor Rx Mapping 16 */
  #define SYNA_F55_SENSOR_CTRL01_00_17_CFGBLK_OFS            0x00EA   /* Sensor Rx Mapping 17 */
  #define SYNA_F55_SENSOR_CTRL02_00_00_CFGBLK_OFS            0x00EB   /* Sensor Tx Mapping 0 */
  #define SYNA_F55_SENSOR_CTRL02_00_01_CFGBLK_OFS            0x00EC   /* Sensor Tx Mapping 1 */
  #define SYNA_F55_SENSOR_CTRL02_00_02_CFGBLK_OFS            0x00ED   /* Sensor Tx Mapping 2 */
  #define SYNA_F55_SENSOR_CTRL02_00_03_CFGBLK_OFS            0x00EE   /* Sensor Tx Mapping 3 */
  #define SYNA_F55_SENSOR_CTRL02_00_04_CFGBLK_OFS            0x00EF   /* Sensor Tx Mapping 4 */
  #define SYNA_F55_SENSOR_CTRL02_00_05_CFGBLK_OFS            0x00F0   /* Sensor Tx Mapping 5 */
  #define SYNA_F55_SENSOR_CTRL02_00_06_CFGBLK_OFS            0x00F1   /* Sensor Tx Mapping 6 */
  #define SYNA_F55_SENSOR_CTRL02_00_07_CFGBLK_OFS            0x00F2   /* Sensor Tx Mapping 7 */
  #define SYNA_F55_SENSOR_CTRL02_00_08_CFGBLK_OFS            0x00F3   /* Sensor Tx Mapping 8 */
  #define SYNA_F55_SENSOR_CTRL02_00_09_CFGBLK_OFS            0x00F4   /* Sensor Tx Mapping 9 */
  #define SYNA_F55_SENSOR_CTRL02_00_10_CFGBLK_OFS            0x00F5   /* Sensor Tx Mapping 10 */
  #define SYNA_F55_SENSOR_CTRL03_00_00_CFGBLK_OFS            0x00F6   /* Edge Compensation 0 */
  #define SYNA_F55_SENSOR_CTRL03_00_01_CFGBLK_OFS            0x00F7   /* Edge Compensation 1 */
  #define SYNA_F55_SENSOR_CTRL03_00_02_CFGBLK_OFS            0x00F8   /* Edge Compensation 2 */
  #define SYNA_F55_SENSOR_CTRL03_00_03_CFGBLK_OFS            0x00F9   /* Edge Compensation 3 */
  #define SYNA_F55_SENSOR_CTRL03_00_04_CFGBLK_OFS            0x00FA   /* Edge Compensation 4 */
  #define SYNA_F55_SENSOR_CTRL03_00_05_CFGBLK_OFS            0x00FB   /* Edge Compensation 5 */
  #define SYNA_F55_SENSOR_CTRL03_00_06_CFGBLK_OFS            0x00FC   /* Edge Compensation 6 */
  #define SYNA_F55_SENSOR_CTRL03_00_07_CFGBLK_OFS            0x00FD   /* Edge Compensation 7 */
  #define SYNA_F55_SENSOR_CTRL04_00_00_CFGBLK_OFS            0x00FE   /* Axis 1 Compensation 0 */
  #define SYNA_F55_SENSOR_CTRL04_00_01_CFGBLK_OFS            0x00FF   /* Axis 1 Compensation 1 */
  #define SYNA_F55_SENSOR_CTRL04_00_02_CFGBLK_OFS            0x0100   /* Axis 1 Compensation 2 */
  #define SYNA_F55_SENSOR_CTRL04_00_03_CFGBLK_OFS            0x0101   /* Axis 1 Compensation 3 */
  #define SYNA_F55_SENSOR_CTRL04_00_04_CFGBLK_OFS            0x0102   /* Axis 1 Compensation 4 */
  #define SYNA_F55_SENSOR_CTRL04_00_05_CFGBLK_OFS            0x0103   /* Axis 1 Compensation 5 */
  #define SYNA_F55_SENSOR_CTRL04_00_06_CFGBLK_OFS            0x0104   /* Axis 1 Compensation 6 */
  #define SYNA_F55_SENSOR_CTRL04_00_07_CFGBLK_OFS            0x0105   /* Axis 1 Compensation 7 */
  #define SYNA_F55_SENSOR_CTRL04_00_08_CFGBLK_OFS            0x0106   /* Axis 1 Compensation 8 */
  #define SYNA_F55_SENSOR_CTRL04_00_09_CFGBLK_OFS            0x0107   /* Axis 1 Compensation 9 */
  #define SYNA_F55_SENSOR_CTRL04_00_10_CFGBLK_OFS            0x0108   /* Axis 1 Compensation 10 */
  #define SYNA_F55_SENSOR_CTRL04_00_11_CFGBLK_OFS            0x0109   /* Axis 1 Compensation 11 */
  #define SYNA_F55_SENSOR_CTRL04_00_12_CFGBLK_OFS            0x010A   /* Axis 1 Compensation 12 */
  #define SYNA_F55_SENSOR_CTRL04_00_13_CFGBLK_OFS            0x010B   /* Axis 1 Compensation 13 */
  #define SYNA_F55_SENSOR_CTRL04_00_14_CFGBLK_OFS            0x010C   /* Axis 1 Compensation 14 */
  #define SYNA_F55_SENSOR_CTRL04_00_15_CFGBLK_OFS            0x010D   /* Axis 1 Compensation 15 */
  #define SYNA_F55_SENSOR_CTRL04_00_16_CFGBLK_OFS            0x010E   /* Axis 1 Compensation 16 */
  #define SYNA_F55_SENSOR_CTRL04_00_17_CFGBLK_OFS            0x010F   /* Axis 1 Compensation 17 */
  #define SYNA_F55_SENSOR_CTRL05_00_00_CFGBLK_OFS            0x0110   /* Axis 2 Compensation 0 */
  #define SYNA_F55_SENSOR_CTRL05_00_01_CFGBLK_OFS            0x0111   /* Axis 2 Compensation 1 */
  #define SYNA_F55_SENSOR_CTRL05_00_02_CFGBLK_OFS            0x0112   /* Axis 2 Compensation 2 */
  #define SYNA_F55_SENSOR_CTRL05_00_03_CFGBLK_OFS            0x0113   /* Axis 2 Compensation 3 */
  #define SYNA_F55_SENSOR_CTRL05_00_04_CFGBLK_OFS            0x0114   /* Axis 2 Compensation 4 */
  #define SYNA_F55_SENSOR_CTRL05_00_05_CFGBLK_OFS            0x0115   /* Axis 2 Compensation 5 */
  #define SYNA_F55_SENSOR_CTRL05_00_06_CFGBLK_OFS            0x0116   /* Axis 2 Compensation 6 */
  #define SYNA_F55_SENSOR_CTRL05_00_07_CFGBLK_OFS            0x0117   /* Axis 2 Compensation 7 */
  #define SYNA_F55_SENSOR_CTRL05_00_08_CFGBLK_OFS            0x0118   /* Axis 2 Compensation 8 */
  #define SYNA_F55_SENSOR_CTRL05_00_09_CFGBLK_OFS            0x0119   /* Axis 2 Compensation 9 */
  #define SYNA_F55_SENSOR_CTRL05_00_10_CFGBLK_OFS            0x011A   /* Axis 2 Compensation 10 */
  #define SYNA_CFGBLK_CRC1_CFGBLK_OFS                        0x01FC   /* Configuration CRC [7:0] */
  #define SYNA_CFGBLK_CRC2_CFGBLK_OFS                        0x01FD   /* Configuration CRC [15:8] */
  #define SYNA_CFGBLK_CRC3_CFGBLK_OFS                        0x01FE   /* Configuration CRC [23:16] */
  #define SYNA_CFGBLK_CRC4_CFGBLK_OFS                        0x01FF   /* Configuration CRC [31:24] */

  /* Masks for interrupt sources */

  /*      Symbol Name                                        Mask        Description */
  /*      -----------                                        ----        ----------- */
  #define SYNA_F01_RMI_INT_SOURCE_MASK_ALL                   0x0002   /* Mask of all Func $01 (RMI) interrupts */
  #define SYNA_F01_RMI_INT_SOURCE_MASK_STATUS                0x0002   /* Mask of Func $01 (RMI) 'STATUS' interrupt */
  #define SYNA_F11_2D_INT_SOURCE_MASK_ABS0                   0x0004   /* Mask of Func $11 (2D) 'ABS0' interrupt */
  #define SYNA_F11_2D_INT_SOURCE_MASK_ALL                    0x0004   /* Mask of all Func $11 (2D) interrupts */
  #define SYNA_F1A_0D_INT_SOURCE_MASK_ALL                    0x0010   /* Mask of all Func $1A (0D) interrupts */
  #define SYNA_F1A_0D_INT_SOURCE_MASK_BUTTON                 0x0010   /* Mask of Func $1A (0D) 'BUTTON' interrupt */
  #define SYNA_F34_FLASH_INT_SOURCE_MASK_ALL                 0x0001   /* Mask of all Func $34 (FLASH) interrupts */
  #define SYNA_F34_FLASH_INT_SOURCE_MASK_FLASH               0x0001   /* Mask of Func $34 (FLASH) 'FLASH' interrupt */
  #define SYNA_F54_ANALOG_INT_SOURCE_MASK_ALL                0x0008   /* Mask of all Func $54 (ANALOG) interrupts */
  #define SYNA_F54_ANALOG_INT_SOURCE_MASK_ANALOG             0x0008   /* Mask of Func $54 (ANALOG) 'ANALOG' interrupt */
  #define SYNA_F55_SENSOR_INT_SOURCE_MASK_ALL                0x0020   /* Mask of all Func $55 (SENSOR) interrupts */
  #define SYNA_F55_SENSOR_INT_SOURCE_MASK_SENSOR             0x0020   /* Mask of Func $55 (SENSOR) 'SENSOR' interrupt */

#endif  /* SYNA_REGISTER_MAP_H */

