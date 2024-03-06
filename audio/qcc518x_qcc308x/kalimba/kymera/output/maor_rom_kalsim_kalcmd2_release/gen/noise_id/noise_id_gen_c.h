// -----------------------------------------------------------------------------
// Copyright (c) 2023                  Qualcomm Technologies International, Ltd.
//
// Generated by /home/svc-audio-dspsw/kymera_builds/builds/2023/kymera_2312060823/kalimba/kymera/../../util/CommonParameters/DerivationEngine.py
// code v3.5, file //depot/dspsw/maor_rom_v20/util/CommonParameters/DerivationEngine.py, revision 2
// namespace {com.csr.cps.4}UnifiedParameterSchema on 2023-12-06 09:15:59 by svc-audio-dspsw
//
// input from EMPTY
// last change  by  on 
// -----------------------------------------------------------------------------
#ifndef __NOISE_ID_GEN_C_H__
#define __NOISE_ID_GEN_C_H__

#ifndef ParamType
typedef unsigned ParamType;
#endif

// CodeBase IDs
#define NOISE_ID_NOISE_ID_CAP_ID	0x00EB
#define NOISE_ID_NOISE_ID_ALT_CAP_ID_0	0x40D1
#define NOISE_ID_NOISE_ID_SAMPLE_RATE	16000
#define NOISE_ID_NOISE_ID_VERSION_MAJOR	1

// Constant Definitions


// Runtime Config Parameter Bitfields
// NOISE_ID_CONFIG bits
#define NOISE_ID_CONFIG_NOISE_ID_CONFIG_BYPASS                   		0x00000001
#define NOISE_ID_CONFIG_NOISE_ID_CONFIG_DISABLE_ED               		0x00000002
#define NOISE_ID_CONFIG_NOISE_ID_CONFIG_DISABLE_ED_E_FILTER_CHECK		0x00000004


// System Mode
#define NOISE_ID_SYSMODE_STANDBY		0
#define NOISE_ID_SYSMODE_FULL   		2
#define NOISE_ID_SYSMODE_MAX_MODES		3

// System Control
#define NOISE_ID_CONTROL_MODE_OVERRIDE		0x2000

// Noise ID classification

// Flags
#define NOISE_ID_FLAGS_ED		0x00000001

// Statistics Block
typedef struct _tag_NOISE_ID_STATISTICS
{
	ParamType OFFSET_CUR_MODE;
	ParamType OFFSET_OVR_CONTROL;
	ParamType OFFSET_NOISE_ID;
	ParamType OFFSET_LOW_TO_MID_RATIO;
	ParamType OFFSET_FLAGS;
}NOISE_ID_STATISTICS;

typedef NOISE_ID_STATISTICS* LP_NOISE_ID_STATISTICS;

// Parameters Block
typedef struct _tag_NOISE_ID_PARAMETERS
{
	ParamType OFFSET_NOISE_ID_CONFIG;
	ParamType OFFSET_POWER_RATIO_THRESHOLD;
	ParamType OFFSET_FILTER_ATTACK_TIME;
	ParamType OFFSET_FILTER_DECAY_TIME;
	ParamType OFFSET_NID_HOLD_TIMER;
	ParamType OFFSET_ED_ENVELOPE;
	ParamType OFFSET_ED_RATIO;
	ParamType OFFSET_ED_MIN_SIGNAL;
	ParamType OFFSET_ED_MIN_MAX_ENVELOPE;
	ParamType OFFSET_ED_DELTA_TH;
	ParamType OFFSET_ED_HOLD_FRAMES;
	ParamType OFFSET_ED_E_FILTER_MIN_THRESHOLD;
	ParamType OFFSET_ED_E_FILTER_MIN_COUNTER_THRESHOLD;
	ParamType OFFSET_POWER_RATIO_ID0_THRESHOLD;
	ParamType OFFSET_POWER_RATIO_ID1_THRESHOLD;
}NOISE_ID_PARAMETERS;

typedef NOISE_ID_PARAMETERS* LP_NOISE_ID_PARAMETERS;

unsigned *NOISE_ID_GetDefaults(unsigned capid);

#endif // __NOISE_ID_GEN_C_H__
