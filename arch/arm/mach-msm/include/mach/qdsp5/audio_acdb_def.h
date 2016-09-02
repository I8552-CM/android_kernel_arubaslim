/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef _MACH_QDSP5_V2_AUDIO_ACDB_DEF_H
#define _MACH_QDSP5_V2_AUDIO_ACDB_DEF_H

/* Define ACDB device ID */
#if 1
/* VOICE CALL */
#define ACDB_ID_HANDSET_RX                 1
#define ACDB_ID_HANDSET_TX                 2
#define ACDB_ID_HEADSET_CALL_RX            3
#define ACDB_ID_HEADSET_CALL_TX            4
#define ACDB_ID_SPEAKER_CALL_RX            5
#define ACDB_ID_SPEAKER_CALL_TX            6
#define ACDB_ID_BT_CALL_RX                 7
#define ACDB_ID_BT_CALL_TX                 8
#define ACDB_ID_BT_NREC_OFF_CALL_RX        9
#define ACDB_ID_BT_NREC_OFF_CALL_TX        0xA
#define ACDB_ID_HANDSET_EXTRA_CALL_RX      0xB
#define ACDB_ID_HANDSET_EXTRA_CALL_TX      0xC
#define ACDB_ID_SPEAKER_EXTRA_CALL_RX      0xD
#define ACDB_ID_SPEAKER_EXTRA_CALL_TX      0xE

/* SOUND */
#define ACDB_ID_HEADSET_AND_SPEAKER_RX     0x15
#define ACDB_ID_MP3_HEADSET_3POLE_RX       0x17
#define ACDB_ID_MP3_HEADSET_3POLE_TX       0x18
#define ACDB_ID_MP3_SPEAKER_RX             0x19
#define ACDB_ID_MP3_SPEAKER_TX             0x1A
#define ACDB_ID_MP3_HEADSET_RX             0x1B
#define ACDB_ID_MP3_HEADSET_TX             0x1C

/* AMR RECORDER */
#define ACDB_ID_VOICE_RECORDER_HPH_RX      0x1F
#define ACDB_ID_VOICE_RECORDER_HPH_TX      0x20
#define ACDB_ID_VOICE_RECORDER_SPK_RX      0x21
#define ACDB_ID_VOICE_RECORDER_SPK_TX      0x22
#define ACDB_ID_VOICE_RECOGNITION_RX       0x23
#define ACDB_ID_VOICE_RECOGNITION_TX       0x24

/* VOIP */
#define ACDB_ID_HANDSET_VOIP_RX            0x29
#define ACDB_ID_HANDSET_VOIP_TX            0x2A
#define ACDB_ID_HEADSET_VOIP_RX            0x2B
#define ACDB_ID_HEADSET_VOIP_TX            0x2C
#define ACDB_ID_SPEAKER_VOIP_RX            0x2D
#define ACDB_ID_SPEAKER_VOIP_TX            0x2E
#define ACDB_ID_BT_VOIP_RX                 0x2F
#define ACDB_ID_BT_VOIP_TX                 0x30

/* VOIP2 */
#define ACDB_ID_HANDSET_VOIP2_RX           0x33
#define ACDB_ID_HANDSET_VOIP2_TX           0x34
#define ACDB_ID_HEADSET_VOIP2_RX           0x35
#define ACDB_ID_HEADSET_VOIP2_TX           0x36
#define ACDB_ID_SPEAKER_VOIP2_RX           0x37
#define ACDB_ID_SPEAKER_VOIP2_TX           0x38
#define ACDB_ID_BT_VOIP2_RX                0x39
#define ACDB_ID_BT_VOIP2_TX                0x3A

/* VOICE CALL2 */
#define ACDB_ID_HANDSET_CALL2_RX           0x3D
#define ACDB_ID_HANDSET_CALL2_TX           0x3E
#define ACDB_ID_HEADSET_CALL2_RX           0x3F
#define ACDB_ID_HEADSET_CALL2_TX           0x40
#define ACDB_ID_SPEAKER_CALL2_RX           0x41
#define ACDB_ID_SPEAKER_CALL2_TX           0x42
#define ACDB_ID_BT_CALL2_RX                0x43
#define ACDB_ID_BT_CALL2_TX                0x44
#define ACDB_ID_BT_NREC_OFF_CALL2_RX       0x45
#define ACDB_ID_BT_NREC_OFF_CALL2_TX       0x46
#define ACDB_ID_HEADSET_3P_CALL2_RX        0x47
#define ACDB_ID_HEADSET_3P_CALL2_TX        0x48

/* VT */
#define ACDB_ID_HANDSET_VT_RX                 0x49
#define ACDB_ID_HANDSET_VT_TX                 0x4A
#define ACDB_ID_HEADSET_CALL_VT_RX        0x4B
#define ACDB_ID_HEADSET_CALL_VT_TX         0x4C
#define ACDB_ID_SPEAKER_CALL_VT_RX         0x4D
#define ACDB_ID_SPEAKER_CALL_VT_TX         0x4E
#define ACDB_ID_BT_CALL_VT_RX                   0x4F
#define ACDB_ID_BT_CALL_VT_TX                   0x50

/* LOOPBCAK */
#define ACDB_ID_HANDSET_LOOPBACK_RX        0x51
#define ACDB_ID_HANDSET_LOOPBACK_TX        0x52
#define ACDB_ID_HEADSET_LOOPBACK_RX        0x53
#define ACDB_ID_HEADSET_LOOPBACK_TX        0x54
#define ACDB_ID_SPEAKER_LOOPBACK_RX        0x55
#define ACDB_ID_SPEAKER_LOOPBACK_TX        0x56

/* FM RADIO */
#define ACDB_ID_FM_DIGITAL_STEREO_HEADSET       0x5B
#define ACDB_ID_FM_DIGITAL_SPEAKER_PHONE        0x5C
#define ACDB_ID_FM_ANALOG_STEREO_HEADSET        0x5D
#define ACDB_ID_FM_ANALOG_STEREO_HEADSET_CODEC  0x5E

/* AUDIO DOCK */
#define ACDB_ID_AUDIO_DOCK_RX              0x5F
#define ACDB_ID_AUDIO_DOCK_TX              0x60


/*Replace the max device ID,if any new device is added Specific to RTC only*/
#define ACDB_ID_MAX                        ACDB_ID_FM_ANALOG_STEREO_HEADSET_CODEC

#else

#define ACDB_ID_HANDSET_SPKR				1
#define ACDB_ID_HANDSET_MIC				2
#define ACDB_ID_HEADSET_MIC				3
#define ACDB_ID_HEADSET_SPKR_MONO			4
#define ACDB_ID_HEADSET_SPKR_STEREO			5
#define ACDB_ID_SPKR_PHONE_MIC				6
#define ACDB_ID_SPKR_PHONE_MONO				7
#define ACDB_ID_SPKR_PHONE_STEREO			8
#define ACDB_ID_BT_SCO_MIC				9
#define ACDB_ID_BT_SCO_SPKR				0x0A
#define ACDB_ID_BT_A2DP_SPKR				0x0B
#define ACDB_ID_BT_A2DP_TX				0x10
#define ACDB_ID_TTY_HEADSET_MIC				0x0C
#define ACDB_ID_TTY_HEADSET_SPKR			0x0D
#define ACDB_ID_HEADSET_MONO_PLUS_SPKR_MONO_RX		0x11
#define ACDB_ID_HEADSET_STEREO_PLUS_SPKR_STEREO_RX	0x14
#define ACDB_ID_FM_TX_LOOPBACK				0x17
#define ACDB_ID_FM_TX					0x18
#define ACDB_ID_LP_FM_SPKR_PHONE_STEREO_RX		0x19
#define ACDB_ID_LP_FM_HEADSET_SPKR_STEREO_RX		0x1A
#define ACDB_ID_I2S_RX					0x20
#define ACDB_ID_SPKR_PHONE_MIC_BROADSIDE		0x2B
#define ACDB_ID_HANDSET_MIC_BROADSIDE			0x2C
#define ACDB_ID_SPKR_PHONE_MIC_ENDFIRE			0x2D
#define ACDB_ID_HANDSET_MIC_ENDFIRE			0x2E
#define ACDB_ID_I2S_TX					0x30
#define ACDB_ID_HDMI					0x40
#define ACDB_ID_FM_RX					0x4F
/*Replace the max device ID,if any new device is added Specific to RTC only*/
#define ACDB_ID_MAX                                 ACDB_ID_FM_RX
#endif

/* ID used for virtual devices */
#define PSEUDO_ACDB_ID					0xFFFF

int is_acdb_enabled(void);
#endif /* _MACH_QDSP5_V2_AUDIO_ACDB_DEF_H */
