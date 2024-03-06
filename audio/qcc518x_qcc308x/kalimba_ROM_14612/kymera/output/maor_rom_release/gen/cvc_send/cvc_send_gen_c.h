// -----------------------------------------------------------------------------
// Copyright (c) 2021                  Qualcomm Technologies International, Ltd.
//
#ifndef __CVC_SEND_GEN_C_H__
#define __CVC_SEND_GEN_C_H__

#ifndef ParamType
typedef unsigned ParamType;
#endif

// CodeBase IDs
#define GEN_CVC_SEND_1M_HS_NB_CAP_ID	0x0023
#define GEN_CVC_SEND_1M_HS_NB_ALT_CAP_ID_0	0x4012
#define GEN_CVC_SEND_1M_HS_NB_SAMPLE_RATE	8000
#define GEN_CVC_SEND_1M_HS_NB_VERSION_MAJOR	7

#define GEN_CVC_SEND_1M_HS_WB_CAP_ID	0x0024
#define GEN_CVC_SEND_1M_HS_WB_ALT_CAP_ID_0	0x4013
#define GEN_CVC_SEND_1M_HS_WB_SAMPLE_RATE	16000
#define GEN_CVC_SEND_1M_HS_WB_VERSION_MAJOR	7

#define GEN_CVC_SEND_1M_HS_UWB_CAP_ID	0x005C
#define GEN_CVC_SEND_1M_HS_UWB_ALT_CAP_ID_0	0x402D
#define GEN_CVC_SEND_1M_HS_UWB_SAMPLE_RATE	24000
#define GEN_CVC_SEND_1M_HS_UWB_VERSION_MAJOR	7

#define GEN_CVC_SEND_1M_HS_SWB_CAP_ID	0x005D
#define GEN_CVC_SEND_1M_HS_SWB_ALT_CAP_ID_0	0x402E
#define GEN_CVC_SEND_1M_HS_SWB_SAMPLE_RATE	32000
#define GEN_CVC_SEND_1M_HS_SWB_VERSION_MAJOR	7

#define GEN_CVC_SEND_1M_HS_FB_CAP_ID	0x005E
#define GEN_CVC_SEND_1M_HS_FB_ALT_CAP_ID_0	0x402F
#define GEN_CVC_SEND_1M_HS_FB_SAMPLE_RATE	48000
#define GEN_CVC_SEND_1M_HS_FB_VERSION_MAJOR	7

#define GEN_CVC_SEND_1M_HS_VA_CAP_ID	0x0084
#define GEN_CVC_SEND_1M_HS_VA_ALT_CAP_ID_0	0x40A3
#define GEN_CVC_SEND_1M_HS_VA_SAMPLE_RATE	16000
#define GEN_CVC_SEND_1M_HS_VA_VERSION_MAJOR	7

#define GEN_CVC_SEND_2M_HSE_NB_CAP_ID	0x0025
#define GEN_CVC_SEND_2M_HSE_NB_ALT_CAP_ID_0	0x4014
#define GEN_CVC_SEND_2M_HSE_NB_SAMPLE_RATE	8000
#define GEN_CVC_SEND_2M_HSE_NB_VERSION_MAJOR	7

#define GEN_CVC_SEND_2M_HSE_WB_CAP_ID	0x0026
#define GEN_CVC_SEND_2M_HSE_WB_ALT_CAP_ID_0	0x4015
#define GEN_CVC_SEND_2M_HSE_WB_SAMPLE_RATE	16000
#define GEN_CVC_SEND_2M_HSE_WB_VERSION_MAJOR	7

#define GEN_CVC_SEND_2M_HSE_UWB_CAP_ID	0x005F
#define GEN_CVC_SEND_2M_HSE_UWB_ALT_CAP_ID_0	0x4030
#define GEN_CVC_SEND_2M_HSE_UWB_SAMPLE_RATE	24000
#define GEN_CVC_SEND_2M_HSE_UWB_VERSION_MAJOR	7

#define GEN_CVC_SEND_2M_HSE_SWB_CAP_ID	0x0060
#define GEN_CVC_SEND_2M_HSE_SWB_ALT_CAP_ID_0	0x4031
#define GEN_CVC_SEND_2M_HSE_SWB_SAMPLE_RATE	32000
#define GEN_CVC_SEND_2M_HSE_SWB_VERSION_MAJOR	7

#define GEN_CVC_SEND_2M_HSE_FB_CAP_ID	0x0061
#define GEN_CVC_SEND_2M_HSE_FB_ALT_CAP_ID_0	0x4032
#define GEN_CVC_SEND_2M_HSE_FB_SAMPLE_RATE	48000
#define GEN_CVC_SEND_2M_HSE_FB_VERSION_MAJOR	7

#define GEN_CVC_SEND_2M_HSE_VA_WAKEON_WB_CAP_ID	0x0080
#define GEN_CVC_SEND_2M_HSE_VA_WAKEON_WB_ALT_CAP_ID_0	0x4091
#define GEN_CVC_SEND_2M_HSE_VA_WAKEON_WB_SAMPLE_RATE	16000
#define GEN_CVC_SEND_2M_HSE_VA_WAKEON_WB_VERSION_MAJOR	7

#define GEN_CVC_SEND_2M_HSE_VA_BARGEIN_WB_CAP_ID	0x0081
#define GEN_CVC_SEND_2M_HSE_VA_BARGEIN_WB_ALT_CAP_ID_0	0x4092
#define GEN_CVC_SEND_2M_HSE_VA_BARGEIN_WB_SAMPLE_RATE	16000
#define GEN_CVC_SEND_2M_HSE_VA_BARGEIN_WB_VERSION_MAJOR	7

#define GEN_CVC_SEND_2M_HSB_NB_CAP_ID	0x0027
#define GEN_CVC_SEND_2M_HSB_NB_ALT_CAP_ID_0	0x4016
#define GEN_CVC_SEND_2M_HSB_NB_SAMPLE_RATE	8000
#define GEN_CVC_SEND_2M_HSB_NB_VERSION_MAJOR	7

#define GEN_CVC_SEND_2M_HSB_WB_CAP_ID	0x0028
#define GEN_CVC_SEND_2M_HSB_WB_ALT_CAP_ID_0	0x4017
#define GEN_CVC_SEND_2M_HSB_WB_SAMPLE_RATE	16000
#define GEN_CVC_SEND_2M_HSB_WB_VERSION_MAJOR	7

#define GEN_CVC_SEND_2M_HSB_UWB_CAP_ID	0x0062
#define GEN_CVC_SEND_2M_HSB_UWB_ALT_CAP_ID_0	0x4033
#define GEN_CVC_SEND_2M_HSB_UWB_SAMPLE_RATE	24000
#define GEN_CVC_SEND_2M_HSB_UWB_VERSION_MAJOR	7

#define GEN_CVC_SEND_2M_HSB_SWB_CAP_ID	0x0063
#define GEN_CVC_SEND_2M_HSB_SWB_ALT_CAP_ID_0	0x4034
#define GEN_CVC_SEND_2M_HSB_SWB_SAMPLE_RATE	32000
#define GEN_CVC_SEND_2M_HSB_SWB_VERSION_MAJOR	7

#define GEN_CVC_SEND_2M_HSB_FB_CAP_ID	0x0064
#define GEN_CVC_SEND_2M_HSB_FB_ALT_CAP_ID_0	0x4035
#define GEN_CVC_SEND_2M_HSB_FB_SAMPLE_RATE	48000
#define GEN_CVC_SEND_2M_HSB_FB_VERSION_MAJOR	7

#define GEN_CVC_SEND_3M_HSE_NB_CAP_ID	0x004B
#define GEN_CVC_SEND_3M_HSE_NB_ALT_CAP_ID_0	0x401F
#define GEN_CVC_SEND_3M_HSE_NB_SAMPLE_RATE	8000
#define GEN_CVC_SEND_3M_HSE_NB_VERSION_MAJOR	7

#define GEN_CVC_SEND_3M_HSE_WB_CAP_ID	0x004C
#define GEN_CVC_SEND_3M_HSE_WB_ALT_CAP_ID_0	0x4020
#define GEN_CVC_SEND_3M_HSE_WB_SAMPLE_RATE	16000
#define GEN_CVC_SEND_3M_HSE_WB_VERSION_MAJOR	7

#define GEN_CVC_SEND_3M_HSE_UWB_CAP_ID	0x0065
#define GEN_CVC_SEND_3M_HSE_UWB_ALT_CAP_ID_0	0x4036
#define GEN_CVC_SEND_3M_HSE_UWB_SAMPLE_RATE	24000
#define GEN_CVC_SEND_3M_HSE_UWB_VERSION_MAJOR	7

#define GEN_CVC_SEND_3M_HSE_SWB_CAP_ID	0x0066
#define GEN_CVC_SEND_3M_HSE_SWB_ALT_CAP_ID_0	0x4037
#define GEN_CVC_SEND_3M_HSE_SWB_SAMPLE_RATE	32000
#define GEN_CVC_SEND_3M_HSE_SWB_VERSION_MAJOR	7

#define GEN_CVC_SEND_3M_HSE_FB_CAP_ID	0x0067
#define GEN_CVC_SEND_3M_HSE_FB_ALT_CAP_ID_0	0x4038
#define GEN_CVC_SEND_3M_HSE_FB_SAMPLE_RATE	48000
#define GEN_CVC_SEND_3M_HSE_FB_VERSION_MAJOR	7

#define GEN_CVC_SEND_3M_HSB_NB_CAP_ID	0x004D
#define GEN_CVC_SEND_3M_HSB_NB_ALT_CAP_ID_0	0x4021
#define GEN_CVC_SEND_3M_HSB_NB_SAMPLE_RATE	8000
#define GEN_CVC_SEND_3M_HSB_NB_VERSION_MAJOR	7

#define GEN_CVC_SEND_3M_HSB_WB_CAP_ID	0x004E
#define GEN_CVC_SEND_3M_HSB_WB_ALT_CAP_ID_0	0x4022
#define GEN_CVC_SEND_3M_HSB_WB_SAMPLE_RATE	16000
#define GEN_CVC_SEND_3M_HSB_WB_VERSION_MAJOR	7

#define GEN_CVC_SEND_3M_HSB_UWB_CAP_ID	0x0068
#define GEN_CVC_SEND_3M_HSB_UWB_ALT_CAP_ID_0	0x4039
#define GEN_CVC_SEND_3M_HSB_UWB_SAMPLE_RATE	24000
#define GEN_CVC_SEND_3M_HSB_UWB_VERSION_MAJOR	7

#define GEN_CVC_SEND_3M_HSB_SWB_CAP_ID	0x0069
#define GEN_CVC_SEND_3M_HSB_SWB_ALT_CAP_ID_0	0x403A
#define GEN_CVC_SEND_3M_HSB_SWB_SAMPLE_RATE	32000
#define GEN_CVC_SEND_3M_HSB_SWB_VERSION_MAJOR	7

#define GEN_CVC_SEND_3M_HSB_FB_CAP_ID	0x006A
#define GEN_CVC_SEND_3M_HSB_FB_ALT_CAP_ID_0	0x403B
#define GEN_CVC_SEND_3M_HSB_FB_SAMPLE_RATE	48000
#define GEN_CVC_SEND_3M_HSB_FB_VERSION_MAJOR	7

#define GEN_CVC_SEND_1M_SPKR_NB_CAP_ID	0x0029
#define GEN_CVC_SEND_1M_SPKR_NB_ALT_CAP_ID_0	0x4018
#define GEN_CVC_SEND_1M_SPKR_NB_SAMPLE_RATE	8000
#define GEN_CVC_SEND_1M_SPKR_NB_VERSION_MAJOR	7

#define GEN_CVC_SEND_1M_SPKR_WB_CAP_ID	0x002A
#define GEN_CVC_SEND_1M_SPKR_WB_ALT_CAP_ID_0	0x4003
#define GEN_CVC_SEND_1M_SPKR_WB_SAMPLE_RATE	16000
#define GEN_CVC_SEND_1M_SPKR_WB_VERSION_MAJOR	7

#define GEN_CVC_SEND_1M_SPKR_UWB_CAP_ID	0x006B
#define GEN_CVC_SEND_1M_SPKR_UWB_ALT_CAP_ID_0	0x403C
#define GEN_CVC_SEND_1M_SPKR_UWB_SAMPLE_RATE	24000
#define GEN_CVC_SEND_1M_SPKR_UWB_VERSION_MAJOR	7

#define GEN_CVC_SEND_1M_SPKR_SWB_CAP_ID	0x006C
#define GEN_CVC_SEND_1M_SPKR_SWB_ALT_CAP_ID_0	0x403D
#define GEN_CVC_SEND_1M_SPKR_SWB_SAMPLE_RATE	32000
#define GEN_CVC_SEND_1M_SPKR_SWB_VERSION_MAJOR	7

#define GEN_CVC_SEND_1M_SPKR_FB_CAP_ID	0x006D
#define GEN_CVC_SEND_1M_SPKR_FB_ALT_CAP_ID_0	0x403E
#define GEN_CVC_SEND_1M_SPKR_FB_SAMPLE_RATE	48000
#define GEN_CVC_SEND_1M_SPKR_FB_VERSION_MAJOR	7

#define GEN_CVC_SEND_2M_SPKRB_NB_CAP_ID	0x002D
#define GEN_CVC_SEND_2M_SPKRB_NB_ALT_CAP_ID_0	0x4019
#define GEN_CVC_SEND_2M_SPKRB_NB_SAMPLE_RATE	8000
#define GEN_CVC_SEND_2M_SPKRB_NB_VERSION_MAJOR	7

#define GEN_CVC_SEND_2M_SPKRB_WB_CAP_ID	0x002E
#define GEN_CVC_SEND_2M_SPKRB_WB_ALT_CAP_ID_0	0x401A
#define GEN_CVC_SEND_2M_SPKRB_WB_SAMPLE_RATE	16000
#define GEN_CVC_SEND_2M_SPKRB_WB_VERSION_MAJOR	7

#define GEN_CVC_SEND_2M_SPKRB_UWB_CAP_ID	0x006E
#define GEN_CVC_SEND_2M_SPKRB_UWB_ALT_CAP_ID_0	0x403F
#define GEN_CVC_SEND_2M_SPKRB_UWB_SAMPLE_RATE	24000
#define GEN_CVC_SEND_2M_SPKRB_UWB_VERSION_MAJOR	7

#define GEN_CVC_SEND_2M_SPKRB_SWB_CAP_ID	0x006F
#define GEN_CVC_SEND_2M_SPKRB_SWB_ALT_CAP_ID_0	0x4040
#define GEN_CVC_SEND_2M_SPKRB_SWB_SAMPLE_RATE	32000
#define GEN_CVC_SEND_2M_SPKRB_SWB_VERSION_MAJOR	7

#define GEN_CVC_SEND_2M_SPKRB_FB_CAP_ID	0x0070
#define GEN_CVC_SEND_2M_SPKRB_FB_ALT_CAP_ID_0	0x4041
#define GEN_CVC_SEND_2M_SPKRB_FB_SAMPLE_RATE	48000
#define GEN_CVC_SEND_2M_SPKRB_FB_VERSION_MAJOR	7

#define GEN_CVC_SEND_3M_SPKRB_NB_CAP_ID	0x0044
#define GEN_CVC_SEND_3M_SPKRB_NB_ALT_CAP_ID_0	0x401B
#define GEN_CVC_SEND_3M_SPKRB_NB_SAMPLE_RATE	8000
#define GEN_CVC_SEND_3M_SPKRB_NB_VERSION_MAJOR	7

#define GEN_CVC_SEND_3M_SPKRB_WB_CAP_ID	0x0045
#define GEN_CVC_SEND_3M_SPKRB_WB_ALT_CAP_ID_0	0x401C
#define GEN_CVC_SEND_3M_SPKRB_WB_SAMPLE_RATE	16000
#define GEN_CVC_SEND_3M_SPKRB_WB_VERSION_MAJOR	7

#define GEN_CVC_SEND_3M_SPKRB_UWB_CAP_ID	0x0071
#define GEN_CVC_SEND_3M_SPKRB_UWB_ALT_CAP_ID_0	0x4042
#define GEN_CVC_SEND_3M_SPKRB_UWB_SAMPLE_RATE	24000
#define GEN_CVC_SEND_3M_SPKRB_UWB_VERSION_MAJOR	7

#define GEN_CVC_SEND_3M_SPKRB_SWB_CAP_ID	0x0072
#define GEN_CVC_SEND_3M_SPKRB_SWB_ALT_CAP_ID_0	0x4043
#define GEN_CVC_SEND_3M_SPKRB_SWB_SAMPLE_RATE	32000
#define GEN_CVC_SEND_3M_SPKRB_SWB_VERSION_MAJOR	7

#define GEN_CVC_SEND_3M_SPKRB_FB_CAP_ID	0x0073
#define GEN_CVC_SEND_3M_SPKRB_FB_ALT_CAP_ID_0	0x4044
#define GEN_CVC_SEND_3M_SPKRB_FB_SAMPLE_RATE	48000
#define GEN_CVC_SEND_3M_SPKRB_FB_VERSION_MAJOR	7

#define GEN_CVC_SEND_4M_SPKRB_NB_CAP_ID	0x0046
#define GEN_CVC_SEND_4M_SPKRB_NB_ALT_CAP_ID_0	0x401D
#define GEN_CVC_SEND_4M_SPKRB_NB_SAMPLE_RATE	8000
#define GEN_CVC_SEND_4M_SPKRB_NB_VERSION_MAJOR	7

#define GEN_CVC_SEND_4M_SPKRB_WB_CAP_ID	0x0047
#define GEN_CVC_SEND_4M_SPKRB_WB_ALT_CAP_ID_0	0x401E
#define GEN_CVC_SEND_4M_SPKRB_WB_SAMPLE_RATE	16000
#define GEN_CVC_SEND_4M_SPKRB_WB_VERSION_MAJOR	7

#define GEN_CVC_SEND_4M_SPKRB_UWB_CAP_ID	0x0074
#define GEN_CVC_SEND_4M_SPKRB_UWB_ALT_CAP_ID_0	0x4045
#define GEN_CVC_SEND_4M_SPKRB_UWB_SAMPLE_RATE	24000
#define GEN_CVC_SEND_4M_SPKRB_UWB_VERSION_MAJOR	7

#define GEN_CVC_SEND_4M_SPKRB_SWB_CAP_ID	0x0075
#define GEN_CVC_SEND_4M_SPKRB_SWB_ALT_CAP_ID_0	0x4046
#define GEN_CVC_SEND_4M_SPKRB_SWB_SAMPLE_RATE	32000
#define GEN_CVC_SEND_4M_SPKRB_SWB_VERSION_MAJOR	7

#define GEN_CVC_SEND_4M_SPKRB_FB_CAP_ID	0x0076
#define GEN_CVC_SEND_4M_SPKRB_FB_ALT_CAP_ID_0	0x4047
#define GEN_CVC_SEND_4M_SPKRB_FB_SAMPLE_RATE	48000
#define GEN_CVC_SEND_4M_SPKRB_FB_VERSION_MAJOR	7

#define GEN_CVC_SEND_3M_SPKRCIRC_NB_CAP_ID	0x004F
#define GEN_CVC_SEND_3M_SPKRCIRC_NB_ALT_CAP_ID_0	0x4023
#define GEN_CVC_SEND_3M_SPKRCIRC_NB_SAMPLE_RATE	8000
#define GEN_CVC_SEND_3M_SPKRCIRC_NB_VERSION_MAJOR	7

#define GEN_CVC_SEND_3M_SPKRCIRC_WB_CAP_ID	0x0050
#define GEN_CVC_SEND_3M_SPKRCIRC_WB_ALT_CAP_ID_0	0x4024
#define GEN_CVC_SEND_3M_SPKRCIRC_WB_SAMPLE_RATE	16000
#define GEN_CVC_SEND_3M_SPKRCIRC_WB_VERSION_MAJOR	7

#define GEN_CVC_SEND_3M_SPKRCIRC_UWB_CAP_ID	0x0077
#define GEN_CVC_SEND_3M_SPKRCIRC_UWB_ALT_CAP_ID_0	0x4048
#define GEN_CVC_SEND_3M_SPKRCIRC_UWB_SAMPLE_RATE	24000
#define GEN_CVC_SEND_3M_SPKRCIRC_UWB_VERSION_MAJOR	7

#define GEN_CVC_SEND_3M_SPKRCIRC_SWB_CAP_ID	0x0078
#define GEN_CVC_SEND_3M_SPKRCIRC_SWB_ALT_CAP_ID_0	0x4049
#define GEN_CVC_SEND_3M_SPKRCIRC_SWB_SAMPLE_RATE	32000
#define GEN_CVC_SEND_3M_SPKRCIRC_SWB_VERSION_MAJOR	7

#define GEN_CVC_SEND_3M_SPKRCIRC_FB_CAP_ID	0x0079
#define GEN_CVC_SEND_3M_SPKRCIRC_FB_ALT_CAP_ID_0	0x404A
#define GEN_CVC_SEND_3M_SPKRCIRC_FB_SAMPLE_RATE	48000
#define GEN_CVC_SEND_3M_SPKRCIRC_FB_VERSION_MAJOR	7

#define GEN_CVC_SEND_3M_SPKRCIRC_VA_WB_CAP_ID	0x007D
#define GEN_CVC_SEND_3M_SPKRCIRC_VA_WB_ALT_CAP_ID_0	0x4086
#define GEN_CVC_SEND_3M_SPKRCIRC_VA_WB_SAMPLE_RATE	16000
#define GEN_CVC_SEND_3M_SPKRCIRC_VA_WB_VERSION_MAJOR	7

#define GEN_CVC_SEND_3M_SPKRCIRC_VA4B_WB_CAP_ID	0x007E
#define GEN_CVC_SEND_3M_SPKRCIRC_VA4B_WB_ALT_CAP_ID_0	0x4087
#define GEN_CVC_SEND_3M_SPKRCIRC_VA4B_WB_SAMPLE_RATE	16000
#define GEN_CVC_SEND_3M_SPKRCIRC_VA4B_WB_VERSION_MAJOR	7

#define GEN_CVC_SEND_4M_SPKRCIRC_NB_CAP_ID	0x0051
#define GEN_CVC_SEND_4M_SPKRCIRC_NB_ALT_CAP_ID_0	0x4025
#define GEN_CVC_SEND_4M_SPKRCIRC_NB_SAMPLE_RATE	8000
#define GEN_CVC_SEND_4M_SPKRCIRC_NB_VERSION_MAJOR	7

#define GEN_CVC_SEND_4M_SPKRCIRC_WB_CAP_ID	0x0052
#define GEN_CVC_SEND_4M_SPKRCIRC_WB_ALT_CAP_ID_0	0x4026
#define GEN_CVC_SEND_4M_SPKRCIRC_WB_SAMPLE_RATE	16000
#define GEN_CVC_SEND_4M_SPKRCIRC_WB_VERSION_MAJOR	7

#define GEN_CVC_SEND_4M_SPKRCIRC_UWB_CAP_ID	0x007A
#define GEN_CVC_SEND_4M_SPKRCIRC_UWB_ALT_CAP_ID_0	0x404B
#define GEN_CVC_SEND_4M_SPKRCIRC_UWB_SAMPLE_RATE	24000
#define GEN_CVC_SEND_4M_SPKRCIRC_UWB_VERSION_MAJOR	7

#define GEN_CVC_SEND_4M_SPKRCIRC_SWB_CAP_ID	0x007B
#define GEN_CVC_SEND_4M_SPKRCIRC_SWB_ALT_CAP_ID_0	0x404C
#define GEN_CVC_SEND_4M_SPKRCIRC_SWB_SAMPLE_RATE	32000
#define GEN_CVC_SEND_4M_SPKRCIRC_SWB_VERSION_MAJOR	7

#define GEN_CVC_SEND_4M_SPKRCIRC_FB_CAP_ID	0x007C
#define GEN_CVC_SEND_4M_SPKRCIRC_FB_ALT_CAP_ID_0	0x404D
#define GEN_CVC_SEND_4M_SPKRCIRC_FB_SAMPLE_RATE	48000
#define GEN_CVC_SEND_4M_SPKRCIRC_FB_VERSION_MAJOR	7

#define GEN_CVC_SEND_1M_AUTO_NB_CAP_ID	0x001C
#define GEN_CVC_SEND_1M_AUTO_NB_ALT_CAP_ID_0	0x400E
#define GEN_CVC_SEND_1M_AUTO_NB_SAMPLE_RATE	8000
#define GEN_CVC_SEND_1M_AUTO_NB_VERSION_MAJOR	7

#define GEN_CVC_SEND_1M_AUTO_WB_CAP_ID	0x001E
#define GEN_CVC_SEND_1M_AUTO_WB_ALT_CAP_ID_0	0x400F
#define GEN_CVC_SEND_1M_AUTO_WB_SAMPLE_RATE	16000
#define GEN_CVC_SEND_1M_AUTO_WB_VERSION_MAJOR	7

#define GEN_CVC_SEND_1M_AUTO_UWB_CAP_ID	0x0056
#define GEN_CVC_SEND_1M_AUTO_UWB_ALT_CAP_ID_0	0x4027
#define GEN_CVC_SEND_1M_AUTO_UWB_SAMPLE_RATE	24000
#define GEN_CVC_SEND_1M_AUTO_UWB_VERSION_MAJOR	7

#define GEN_CVC_SEND_1M_AUTO_SWB_CAP_ID	0x0057
#define GEN_CVC_SEND_1M_AUTO_SWB_ALT_CAP_ID_0	0x4028
#define GEN_CVC_SEND_1M_AUTO_SWB_SAMPLE_RATE	32000
#define GEN_CVC_SEND_1M_AUTO_SWB_VERSION_MAJOR	7

#define GEN_CVC_SEND_1M_AUTO_FB_CAP_ID	0x0058
#define GEN_CVC_SEND_1M_AUTO_FB_ALT_CAP_ID_0	0x4029
#define GEN_CVC_SEND_1M_AUTO_FB_SAMPLE_RATE	48000
#define GEN_CVC_SEND_1M_AUTO_FB_VERSION_MAJOR	7

#define GEN_CVC_SEND_2M_AUTO_NB_CAP_ID	0x0020
#define GEN_CVC_SEND_2M_AUTO_NB_ALT_CAP_ID_0	0x4010
#define GEN_CVC_SEND_2M_AUTO_NB_SAMPLE_RATE	8000
#define GEN_CVC_SEND_2M_AUTO_NB_VERSION_MAJOR	7

#define GEN_CVC_SEND_2M_AUTO_WB_CAP_ID	0x0021
#define GEN_CVC_SEND_2M_AUTO_WB_ALT_CAP_ID_0	0x4011
#define GEN_CVC_SEND_2M_AUTO_WB_SAMPLE_RATE	16000
#define GEN_CVC_SEND_2M_AUTO_WB_VERSION_MAJOR	7

#define GEN_CVC_SEND_2M_AUTO_UWB_CAP_ID	0x0059
#define GEN_CVC_SEND_2M_AUTO_UWB_ALT_CAP_ID_0	0x402A
#define GEN_CVC_SEND_2M_AUTO_UWB_SAMPLE_RATE	24000
#define GEN_CVC_SEND_2M_AUTO_UWB_VERSION_MAJOR	7

#define GEN_CVC_SEND_2M_AUTO_SWB_CAP_ID	0x005A
#define GEN_CVC_SEND_2M_AUTO_SWB_ALT_CAP_ID_0	0x402B
#define GEN_CVC_SEND_2M_AUTO_SWB_SAMPLE_RATE	32000
#define GEN_CVC_SEND_2M_AUTO_SWB_VERSION_MAJOR	7

#define GEN_CVC_SEND_2M_AUTO_FB_CAP_ID	0x005B
#define GEN_CVC_SEND_2M_AUTO_FB_ALT_CAP_ID_0	0x402C
#define GEN_CVC_SEND_2M_AUTO_FB_SAMPLE_RATE	48000
#define GEN_CVC_SEND_2M_AUTO_FB_VERSION_MAJOR	7

#define GEN_CVC_SEND_2M_EB_IE_WB_CAP_ID	0x0082
#define GEN_CVC_SEND_2M_EB_IE_WB_ALT_CAP_ID_0	0x40A7
#define GEN_CVC_SEND_2M_EB_IE_WB_SAMPLE_RATE	16000
#define GEN_CVC_SEND_2M_EB_IE_WB_VERSION_MAJOR	7

#define GEN_CVC_SEND_3M_EBE_IE_WB_CAP_ID	0x0083
#define GEN_CVC_SEND_3M_EBE_IE_WB_ALT_CAP_ID_0	0x40A5
#define GEN_CVC_SEND_3M_EBE_IE_WB_SAMPLE_RATE	16000
#define GEN_CVC_SEND_3M_EBE_IE_WB_VERSION_MAJOR	7

#define GEN_CVC_SEND_2M_EB_IE_NB_CAP_ID	0x0085
#define GEN_CVC_SEND_2M_EB_IE_NB_ALT_CAP_ID_0	0x40A8
#define GEN_CVC_SEND_2M_EB_IE_NB_SAMPLE_RATE	8000
#define GEN_CVC_SEND_2M_EB_IE_NB_VERSION_MAJOR	7

#define GEN_CVC_SEND_3M_EBE_IE_NB_CAP_ID	0x0086
#define GEN_CVC_SEND_3M_EBE_IE_NB_ALT_CAP_ID_0	0x40A9
#define GEN_CVC_SEND_3M_EBE_IE_NB_SAMPLE_RATE	8000
#define GEN_CVC_SEND_3M_EBE_IE_NB_VERSION_MAJOR	7

// Constant Definitions
#define GEN_CVC_SEND_CONSTANT_CNG_SHAPE_brown 		0x00000000
#define GEN_CVC_SEND_CONSTANT_CNG_SHAPE_pink  		0x00000001
#define GEN_CVC_SEND_CONSTANT_CNG_SHAPE_white 		0xFFFFFFFF
#define GEN_CVC_SEND_CONSTANT_CNG_SHAPE_blue  		0x00000002
#define GEN_CVC_SEND_CONSTANT_CNG_SHAPE_purple		0x00000003


// Runtime Config Parameter Bitfields
// HFK_CONFIG bits
#define GEN_CVC_SEND_CONFIG_HFK_BYP_AEC         		0x00000001
#define GEN_CVC_SEND_CONFIG_HFK_BYP_RER         		0x00000002
#define GEN_CVC_SEND_CONFIG_HFK_BYP_CNG         		0x00000004
#define GEN_CVC_SEND_CONFIG_HFK_BYP_HD          		0x00000008
#define GEN_CVC_SEND_CONFIG_HFK_BYP_DMS         		0x00000010
#define GEN_CVC_SEND_CONFIG_HFK_BYP_HARM        		0x00000020
#define GEN_CVC_SEND_CONFIG_HFK_BYP_WNR         		0x00000040
#define GEN_CVC_SEND_CONFIG_HFK_BYP_NFLOOR      		0x00000080
#define GEN_CVC_SEND_CONFIG_HFK_BYP_AUX         		0x00000100
#define GEN_CVC_SEND_CONFIG_HFK_BYP_AGC         		0x00000400
#define GEN_CVC_SEND_CONFIG_HFK_BYP_NDVC        		0x00000800
#define GEN_CVC_SEND_CONFIG_HFK_BYP_FBC         		0x00001000
// DMSS_CONFIG bits
#define GEN_CVC_SEND_CONFIG_DMSS_BYP_ASF        		0x00000001
#define GEN_CVC_SEND_CONFIG_DMSS_BYP_MGDC       		0x00000002
#define GEN_CVC_SEND_CONFIG_DMSS_BYP_NC         		0x00000004
#define GEN_CVC_SEND_CONFIG_DMSS_BYP_NPC        		0x00000008
#define GEN_CVC_SEND_CONFIG_DMSS_BYP_TP         		0x00000010
#define GEN_CVC_SEND_CONFIG_DMSS_BYP_RNR        		0x00000020
#define GEN_CVC_SEND_CONFIG_DMSS_BYP_SPP        		0x00000100
#define GEN_CVC_SEND_CONFIG_DMSS_BYP_VAD_S      		0x00000200
#define GEN_CVC_SEND_CONFIG_DMSS_BYP_MGDCPERSIST		0x00000400
#define GEN_CVC_SEND_CONFIG_DMSS_BYP_AEC_INT    		0x00001000
#define GEN_CVC_SEND_CONFIG_DMSS_BYP_RER_INT    		0x00002000
#define GEN_CVC_SEND_CONFIG_DMSS_BYP_AUX_INT    		0x00004000
#define GEN_CVC_SEND_CONFIG_DMSS_BYP_HD_INT     		0x00008000
#define GEN_CVC_SEND_CONFIG_DMSS_BYP_FBC_INT    		0x00010000
#define GEN_CVC_SEND_CONFIG_DMSS_BYP_NS_INT     		0x00020000
#define GEN_CVC_SEND_CONFIG_DMSS_BYP_INT_MAP    		0x00040000
#define GEN_CVC_SEND_CONFIG_DMSS_BYP_BLEND      		0x00080000


// System Mode
#define GEN_CVC_SEND_SYSMODE_STATIC           		0
#define GEN_CVC_SEND_SYSMODE_STANDBY          		1
#define GEN_CVC_SEND_SYSMODE_FULL             		2
#define GEN_CVC_SEND_SYSMODE_LOWVOLUME        		3
#define GEN_CVC_SEND_SYSMODE_PASS_THRU_LEFT   		4
#define GEN_CVC_SEND_SYSMODE_PASS_THRU_RIGHT  		5
#define GEN_CVC_SEND_SYSMODE_PASS_THRU_MIC3   		6
#define GEN_CVC_SEND_SYSMODE_PASS_THRU_MIC4   		7
#define GEN_CVC_SEND_SYSMODE_PASS_THRU_AEC_REF		8
#define GEN_CVC_SEND_SYSMODE_MAX_MODES        		9

// System Mode
#define GEN_CVC_SEND_SYSMODE_STATIC           		0
#define GEN_CVC_SEND_SYSMODE_STANDBY          		1
#define GEN_CVC_SEND_SYSMODE_FULL             		2
#define GEN_CVC_SEND_SYSMODE_LOWVOLUME        		3
#define GEN_CVC_SEND_SYSMODE_PASS_THRU_LEFT   		4
#define GEN_CVC_SEND_SYSMODE_PASS_THRU_RIGHT  		5
#define GEN_CVC_SEND_SYSMODE_PASS_THRU_MIC3   		6
#define GEN_CVC_SEND_SYSMODE_PASS_THRU_MIC4   		7
#define GEN_CVC_SEND_SYSMODE_PASS_THRU_AEC_REF		8
#define GEN_CVC_SEND_SYSMODE_MAX_MODES        		9

// System Control
#define GEN_CVC_SEND_CONTROL_MODE_OVERRIDE		0x2000
#define GEN_CVC_SEND_CONTROL_MUTE_OVERRIDE		0x0001
#define GEN_CVC_SEND_CONTROL_OMNI_OVERRIDE		0x0002

// W_Flag

// Mute Control

// Omni Mode Control

// NLP State

// HI_W_Flag

// Statistics Block
typedef struct _tag_CVC_SEND_STATISTICS
{
	ParamType OFFSET_CUR_MODE_OFFSET;
	ParamType OFFSET_SYS_CONTROL_OFFSET;
	ParamType OFFSET_COMPILED_CONFIG;
	ParamType OFFSET_PEAK_ADC_LEFT_OFFSET;
	ParamType OFFSET_PEAK_ADC_RIGHT_OFFSET;
	ParamType OFFSET_PEAK_SCO_OUT_OFFSET;
	ParamType OFFSET_NDVC_NOISE_EST_OFFSET;
	ParamType OFFSET_NDVC_VOL_ADJ_OFFSET;
	ParamType OFFSET_SND_AGC_SPEECH_LVL;
	ParamType OFFSET_SND_AGC_GAIN;
	ParamType OFFSET_AEC_COUPLING_OFFSET;
	ParamType OFFSET_WNR_POWER;
	ParamType OFFSET_WNR_WIND_PHASE;
	ParamType OFFSET_WIND_FLAG;
	ParamType OFFSET_MUTE_FLAG;
	ParamType OFFSET_OMNI_FLAG;
	ParamType OFFSET_PEAK_ADC_MIC3_OFFSET;
	ParamType OFFSET_PEAK_ADC_MIC4_OFFSET;
	ParamType OFFSET_PEAK_AEC_REF_OFFSET;
	ParamType OFFSET_FBC_POWER_DROP;
	ParamType OFFSET_NLP_TIER_HC_ACTIVE;
	ParamType OFFSET_PEAK_VA1_OFFSET;
	ParamType OFFSET_SELFCLEAN_DETECTED;
	ParamType OFFSET_INTMIC_AEC_COUPLING_OFFSET;
	ParamType OFFSET_HI_WIND_FLAG;
}CVC_SEND_STATISTICS;

typedef CVC_SEND_STATISTICS* LP_CVC_SEND_STATISTICS;

// Parameters Block
typedef struct _tag_CVC_SEND_PARAMETERS
{
	ParamType OFFSET_HFK_CONFIG;
	ParamType OFFSET_DMSS_CONFIG;
// NDVC
	ParamType OFFSET_NDVC_HYSTERESIS;
	ParamType OFFSET_NDVC_ATK_TC;
	ParamType OFFSET_NDVC_DEC_TC;
	ParamType OFFSET_NDVC_NUMVOLSTEPS;
	ParamType OFFSET_NDVC_MAXNOISELVL;
	ParamType OFFSET_NDVC_MINNOISELVL;
	ParamType OFFSET_NDVC_LOW_FREQ;
	ParamType OFFSET_NDVC_HIGH_FREQ;
	ParamType OFFSET_SND_PEQ_CONFIG;
	ParamType OFFSET_SND_PEQ_GAIN_EXP;
	ParamType OFFSET_SND_PEQ_GAIN_MANT;
	ParamType OFFSET_SND_PEQ_STAGE1_B2;
	ParamType OFFSET_SND_PEQ_STAGE1_B1;
	ParamType OFFSET_SND_PEQ_STAGE1_B0;
	ParamType OFFSET_SND_PEQ_STAGE1_A2;
	ParamType OFFSET_SND_PEQ_STAGE1_A1;
	ParamType OFFSET_SND_PEQ_STAGE2_B2;
	ParamType OFFSET_SND_PEQ_STAGE2_B1;
	ParamType OFFSET_SND_PEQ_STAGE2_B0;
	ParamType OFFSET_SND_PEQ_STAGE2_A2;
	ParamType OFFSET_SND_PEQ_STAGE2_A1;
	ParamType OFFSET_SND_PEQ_STAGE3_B2;
	ParamType OFFSET_SND_PEQ_STAGE3_B1;
	ParamType OFFSET_SND_PEQ_STAGE3_B0;
	ParamType OFFSET_SND_PEQ_STAGE3_A2;
	ParamType OFFSET_SND_PEQ_STAGE3_A1;
	ParamType OFFSET_SND_PEQ_STAGE4_B2;
	ParamType OFFSET_SND_PEQ_STAGE4_B1;
	ParamType OFFSET_SND_PEQ_STAGE4_B0;
	ParamType OFFSET_SND_PEQ_STAGE4_A2;
	ParamType OFFSET_SND_PEQ_STAGE4_A1;
	ParamType OFFSET_SND_PEQ_STAGE5_B2;
	ParamType OFFSET_SND_PEQ_STAGE5_B1;
	ParamType OFFSET_SND_PEQ_STAGE5_B0;
	ParamType OFFSET_SND_PEQ_STAGE5_A2;
	ParamType OFFSET_SND_PEQ_STAGE5_A1;
	ParamType OFFSET_SND_PEQ_STAGE6_B2;
	ParamType OFFSET_SND_PEQ_STAGE6_B1;
	ParamType OFFSET_SND_PEQ_STAGE6_B0;
	ParamType OFFSET_SND_PEQ_STAGE6_A2;
	ParamType OFFSET_SND_PEQ_STAGE6_A1;
	ParamType OFFSET_SND_PEQ_STAGE7_B2;
	ParamType OFFSET_SND_PEQ_STAGE7_B1;
	ParamType OFFSET_SND_PEQ_STAGE7_B0;
	ParamType OFFSET_SND_PEQ_STAGE7_A2;
	ParamType OFFSET_SND_PEQ_STAGE7_A1;
	ParamType OFFSET_SND_PEQ_STAGE8_B2;
	ParamType OFFSET_SND_PEQ_STAGE8_B1;
	ParamType OFFSET_SND_PEQ_STAGE8_B0;
	ParamType OFFSET_SND_PEQ_STAGE8_A2;
	ParamType OFFSET_SND_PEQ_STAGE8_A1;
	ParamType OFFSET_SND_PEQ_STAGE9_B2;
	ParamType OFFSET_SND_PEQ_STAGE9_B1;
	ParamType OFFSET_SND_PEQ_STAGE9_B0;
	ParamType OFFSET_SND_PEQ_STAGE9_A2;
	ParamType OFFSET_SND_PEQ_STAGE9_A1;
	ParamType OFFSET_SND_PEQ_STAGE10_B2;
	ParamType OFFSET_SND_PEQ_STAGE10_B1;
	ParamType OFFSET_SND_PEQ_STAGE10_B0;
	ParamType OFFSET_SND_PEQ_STAGE10_A2;
	ParamType OFFSET_SND_PEQ_STAGE10_A1;
	ParamType OFFSET_SND_PEQ_SCALE1;
	ParamType OFFSET_SND_PEQ_SCALE2;
	ParamType OFFSET_SND_PEQ_SCALE3;
	ParamType OFFSET_SND_PEQ_SCALE4;
	ParamType OFFSET_SND_PEQ_SCALE5;
	ParamType OFFSET_SND_PEQ_SCALE6;
	ParamType OFFSET_SND_PEQ_SCALE7;
	ParamType OFFSET_SND_PEQ_SCALE8;
	ParamType OFFSET_SND_PEQ_SCALE9;
	ParamType OFFSET_SND_PEQ_SCALE10;
	ParamType OFFSET_SNDGAIN_MANTISSA;
	ParamType OFFSET_SNDGAIN_EXPONENT;
	ParamType OFFSET_PT_SNDGAIN_MANTISSA;
	ParamType OFFSET_PT_SNDGAIN_EXPONENT;
	ParamType OFFSET_VA_GAIN_MANTISSA;
	ParamType OFFSET_VA_GAIN_EXPONENT;
	ParamType OFFSET_SND_AGC_G_INITIAL;
	ParamType OFFSET_SND_AGC_TARGET;
	ParamType OFFSET_SND_AGC_ATTACK_TC;
	ParamType OFFSET_SND_AGC_DECAY_TC;
	ParamType OFFSET_SND_AGC_A_90_PK;
	ParamType OFFSET_SND_AGC_D_90_PK;
	ParamType OFFSET_SND_AGC_G_MAX;
	ParamType OFFSET_SND_AGC_START_COMP;
	ParamType OFFSET_SND_AGC_COMP;
	ParamType OFFSET_SND_AGC_INP_THRESH;
	ParamType OFFSET_SND_AGC_SP_ATTACK;
	ParamType OFFSET_SND_AGC_AD_THRESH1;
	ParamType OFFSET_SND_AGC_AD_THRESH2;
	ParamType OFFSET_SND_AGC_G_MIN;
	ParamType OFFSET_SND_AGC_ECHO_HOLD;
	ParamType OFFSET_SND_AGC_NOISE_HOLD;
	ParamType OFFSET_DMSS_LPN_MIC;
	ParamType OFFSET_ASF_BEAM0_AGGR;
	ParamType OFFSET_ASF_MIC_DISTANCE;
	ParamType OFFSET_ASF_BETA;
	ParamType OFFSET_DOA0;
	ParamType OFFSET_DOA1;
	ParamType OFFSET_WNR_AGGR;
	ParamType OFFSET_WNR_THRESHOLD;
	ParamType OFFSET_WNR_HOLD;
	ParamType OFFSET_WNR_THRESH_PHASE;
	ParamType OFFSET_WNR_THRESH_COHERENCE;
	ParamType OFFSET_MGDC_MAXCOMP;
	ParamType OFFSET_MGDC_TH;
	ParamType OFFSET_NS_POWER;
	ParamType OFFSET_DMS_AGGR;
	ParamType OFFSET_DMS_RESIDUAL_NFLOOR;
	ParamType OFFSET_DMS_NSN_AGGR;
	ParamType OFFSET_DMS_AGGR_VA;
	ParamType OFFSET_NC_TAP;
	ParamType OFFSET_NC_AGGR;
	ParamType OFFSET_NC_RPT;
	ParamType OFFSET_NC_CTRL_BIAS;
	ParamType OFFSET_NC_CTRL_TRANS;
	ParamType OFFSET_RNR_AGGR;
	ParamType OFFSET_LVMODE_THRES;
	ParamType OFFSET_LMS_FREQ;
	ParamType OFFSET_AEC_FILTER_LENGTH;
	ParamType OFFSET_ENABLE_AEC_REUSE;
	ParamType OFFSET_AEC_REF_LPWR_HB;
	ParamType OFFSET_DTC_AGGR;
	ParamType OFFSET_MAX_LPWRX_MARGIN;
	ParamType OFFSET_AEC_NS_CNTRL;
	ParamType OFFSET_RER_ADAPT;
	ParamType OFFSET_RER_AGGRESSIVENESS;
	ParamType OFFSET_RER_POWER;
	ParamType OFFSET_RERDT_OFF_THRESHOLD;
	ParamType OFFSET_RERDT_AGGRESSIVENESS;
	ParamType OFFSET_RERDT_POWER;
	ParamType OFFSET_HDV_GAIN_CNTRL;
	ParamType OFFSET_AEC_LRM_MODE;
	ParamType OFFSET_CNG_Q;
	ParamType OFFSET_CNG_SHAPE;
	ParamType OFFSET_REF_DELAY;
	ParamType OFFSET_HD_THRESH_GAIN;
	ParamType OFFSET_TIER2_THRESH;
	ParamType OFFSET_VSM_HB_TIER1;
	ParamType OFFSET_VSM_LB_TIER1;
	ParamType OFFSET_VSM_MAX_ATT_TIER1;
	ParamType OFFSET_FDNLP_HB_TIER1;
	ParamType OFFSET_FDNLP_LB_TIER1;
	ParamType OFFSET_FDNLP_MB_TIER1;
	ParamType OFFSET_FDNLP_NBINS_TIER1;
	ParamType OFFSET_FDNLP_ATT_TIER1;
	ParamType OFFSET_FDNLP_ATT_THRESH_TIER1;
	ParamType OFFSET_FDNLP_ECHO_THRESH_TIER1;
	ParamType OFFSET_VSM_HB_TIER2;
	ParamType OFFSET_VSM_LB_TIER2;
	ParamType OFFSET_VSM_MAX_ATT_TIER2;
	ParamType OFFSET_FDNLP_HB_TIER2;
	ParamType OFFSET_FDNLP_LB_TIER2;
	ParamType OFFSET_FDNLP_MB_TIER2;
	ParamType OFFSET_FDNLP_NBINS_TIER2;
	ParamType OFFSET_FDNLP_ATT_TIER2;
	ParamType OFFSET_FDNLP_ATT_THRESH_TIER2;
	ParamType OFFSET_FDNLP_ECHO_THRESH_TIER2;
	ParamType OFFSET_MIC_SWITCH;
	ParamType OFFSET_DMP_MODE;
	ParamType OFFSET_ASF_ARRAY_TYPE;
	ParamType OFFSET_ASF_MICDIST_B;
	ParamType OFFSET_ASF_BETA_B;
	ParamType OFFSET_ASF_DOA0_B;
	ParamType OFFSET_ASF_BEAM0_MODE;
	ParamType OFFSET_HPF_ON;
	ParamType OFFSET_FDFBC_PERC;
	ParamType OFFSET_FBC_FILTER_LENGTH;
	ParamType OFFSET_INT_MODE;
	ParamType OFFSET_BLEND_SLOPE;
	ParamType OFFSET_BLEND_CROSSOVER_FREQ;
	ParamType OFFSET_MASK_CUTOFF_FREQ;
	ParamType OFFSET_HYST_CLEAR_THRES;
	ParamType OFFSET_HYST_ASSERT_THRES;
	ParamType OFFSET_SELF_CLEAN_TC;
	ParamType OFFSET_SELF_CLEAN_LOWER;
	ParamType OFFSET_SELF_CLEAN_UPPER;
	ParamType OFFSET_SELF_CLEAN_THRESH;
	ParamType OFFSET_INT_DMS_AGGR;
	ParamType OFFSET_INT_DMS_RESIDUAL_NFLOOR;
	ParamType OFFSET_EXT_DMS_AGGR;
	ParamType OFFSET_EXT_DMS_RESIDUAL_NFLOOR;
	ParamType OFFSET_MAP_LMS_FREQ;
	ParamType OFFSET_AEC_MAP_FILTER_LENGTH;
	ParamType OFFSET_AEC_MAP_REPEAT;
	ParamType OFFSET_AEC_MAP_REF_LPWR_HB;
	ParamType OFFSET_MAP_DTC_AGGR;
	ParamType OFFSET_MAP_MAX_LPWRX_MARGIN;
	ParamType OFFSET_INT_LMS_FREQ;
	ParamType OFFSET_AEC_INT_FILTER_LENGTH;
	ParamType OFFSET_AEC_INT_REPEAT;
	ParamType OFFSET_AEC_INT_REF_LPWR_HB;
	ParamType OFFSET_INT_DTC_AGGR;
	ParamType OFFSET_INT_MAX_LPWRX_MARGIN;
	ParamType OFFSET_AEC_INT_NS_CNTRL;
	ParamType OFFSET_INT_RER_ADAPT;
	ParamType OFFSET_HI_WNR_THRESH_PHASE;
	ParamType OFFSET_HI_WNR_THRESH_COHERENCE;
	ParamType OFFSET_HI_WNR_THRESHOLD;
}CVC_SEND_PARAMETERS;

typedef CVC_SEND_PARAMETERS* LP_CVC_SEND_PARAMETERS;

unsigned *CVC_SEND_GetDefaults(unsigned capid);

#endif // __CVC_SEND_GEN_C_H__
