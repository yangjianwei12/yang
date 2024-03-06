// -----------------------------------------------------------------------------
// Copyright (c) 2023                  Qualcomm Technologies International, Ltd.
//
// Generated by /home/svc-audio-dspsw/kymera_builds/builds/2023/kymera_2312060823/kalimba/kymera/../../util/CommonParameters/DerivationEngine.py
// code v3.5, file //depot/dspsw/maor_rom_v20/util/CommonParameters/DerivationEngine.py, revision 2
// namespace {com.csr.cps.4}UnifiedParameterSchema on 2023-12-06 09:10:34 by svc-audio-dspsw
//
// input from EMPTY
// last change  by  on 
// -----------------------------------------------------------------------------
#ifndef __HCGR_GEN_C_H__
#define __HCGR_GEN_C_H__

#ifndef ParamType
typedef unsigned ParamType;
#endif

// CodeBase IDs
#define HCGR_HCGR_16K_CAP_ID	0x00D8
#define HCGR_HCGR_16K_ALT_CAP_ID_0	0x40B9
#define HCGR_HCGR_16K_SAMPLE_RATE	16000
#define HCGR_HCGR_16K_VERSION_MAJOR	2

// Constant Definitions


// Runtime Config Parameter Bitfields
#define HCGR_CONFIG_IS_FF_AND_FB_HCGR		0x00000001
#define HCGR_CONFIG_BYPASS           		0x00000002


// System Mode
#define HCGR_SYSMODE_STANDBY		0
#define HCGR_SYSMODE_FULL   		1
#define HCGR_SYSMODE_MAX_MODES		2

// System Control
#define HCGR_CONTROL_MODE_OVERRIDE		0x2000

// Flags
#define HCGR_FLAGS_FF_RECOVERY		0x00000002
#define HCGR_FLAGS_FB_RECOVERY		0x00000004

// FF HW Target Gain

// FB HW Target Gain

// HCGR FF Delta Gain

// HCGR FB Delta Gain

// Statistics Block
typedef struct _tag_HCGR_STATISTICS
{
	ParamType OFFSET_CUR_MODE;
	ParamType OFFSET_OVR_CONTROL;
	ParamType OFFSET_FLAGS;
	ParamType OFFSET_FF_HW_TARGET_GAIN;
	ParamType OFFSET_FB_HW_TARGET_GAIN;
	ParamType OFFSET_BLOCK_LEVEL;
	ParamType OFFSET_HOWL_BLOCK_LEVEL;
	ParamType OFFSET_HOWL_FREQ;
	ParamType OFFSET_HCGR_FF_GAIN;
	ParamType OFFSET_HCGR_FB_GAIN;
}HCGR_STATISTICS;

typedef HCGR_STATISTICS* LP_HCGR_STATISTICS;

// Parameters Block
typedef struct _tag_HCGR_PARAMETERS
{
	ParamType OFFSET_HCGR_CONFIG;
	ParamType OFFSET_HCGR_PRIORITY;
	ParamType OFFSET_NOP_BELOW_BIN;
	ParamType OFFSET_PTPR_THRESHOLD_EXP;
	ParamType OFFSET_PTPR_THRESHOLD_MANT;
	ParamType OFFSET_PTPR_THRESHOLD_DC_BEXP;
	ParamType OFFSET_PAPR_THRESHOLD_SHIFT;
	ParamType OFFSET_PNPR_THRESHOLD;
	ParamType OFFSET_IFPR_GROWTH_SCALE;
	ParamType OFFSET_BIN1_TRIGGER_DETECT_COUNT;
	ParamType OFFSET_BIN1_FRAME_RESET_COUNT;
	ParamType OFFSET_IMPULSE_SCALE;
	ParamType OFFSET_COUNTER_IMPULSIVE;
	ParamType OFFSET_COUNTER_REGULAR;
	ParamType OFFSET_COUNTER_TRIGGER;
	ParamType OFFSET_STEP_SIZE_0DB_FS;
	ParamType OFFSET_STEP_SIZE_6DB_FS;
	ParamType OFFSET_STEP_SIZE_12DB_FS;
	ParamType OFFSET_STEP_SIZE_18DB_FS;
	ParamType OFFSET_STEP_SIZE_24DB_FS;
	ParamType OFFSET_STEP_SIZE_30DB_FS;
	ParamType OFFSET_STEP_SIZE_36DB_FS;
	ParamType OFFSET_STEP_SIZE_42DB_FS;
	ParamType OFFSET_MINIMUM_FF_GAIN;
	ParamType OFFSET_MINIMUM_FB_GAIN;
	ParamType OFFSET_GAIN_ATTACK_TIME;
	ParamType OFFSET_AHM_FRAME_SIZE;
	ParamType OFFSET_GAIN_RECOVERY_RATE;
	ParamType OFFSET_GAIN_RECOVERY_RATE_SLOW;
	ParamType OFFSET_GAIN_RECOVERY_RATE_SLOWEST;
	ParamType OFFSET_SLOW_RECOVERY_GAIN_THRESHOLD;
}HCGR_PARAMETERS;

typedef HCGR_PARAMETERS* LP_HCGR_PARAMETERS;

unsigned *HCGR_GetDefaults(unsigned capid);

#endif // __HCGR_GEN_C_H__
