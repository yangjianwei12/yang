// -----------------------------------------------------------------------------
// Copyright (c) 2023                  Qualcomm Technologies International, Ltd.
//
// Generated by /home/svc-audio-dspsw/kymera_builds/builds/2023/kymera_2312060823/kalimba/kymera/../../util/CommonParameters/DerivationEngine.py
// code v3.5, file //depot/dspsw/maor_rom_v20/util/CommonParameters/DerivationEngine.py, revision 2
// namespace {com.csr.cps.4}UnifiedParameterSchema on 2023-12-06 09:11:19 by svc-audio-dspsw
//
// input from //depot/dspsw/dev/aanc/aamb_stre_dev/kalimba/kymera/capabilities/compander/compander.xml#1
// last change INVALID by ud02 on INVALID
// -----------------------------------------------------------------------------
#ifndef __ANC_COMPANDER_GEN_C_H__
#define __ANC_COMPANDER_GEN_C_H__

#ifndef ParamType
typedef unsigned ParamType;
#endif

// CodeBase IDs
#define ANC_COMPANDER_ANC_COMPANDER_CAP_ID	0x00D6
#define ANC_COMPANDER_ANC_COMPANDER_ALT_CAP_ID_0	0x40B7
#define ANC_COMPANDER_ANC_COMPANDER_SAMPLE_RATE	0
#define ANC_COMPANDER_ANC_COMPANDER_VERSION_MAJOR	1

// Constant Definitions
#define ANC_COMPANDER_CONSTANT_LEVEL_ESTIMATION_FLAG_RMS 		0x00000000
#define ANC_COMPANDER_CONSTANT_LEVEL_ESTIMATION_FLAG_Peak		0x00000001


// Runtime Config Parameter Bitfields
#define ANC_COMPANDER_CONFIG_BYPASS         		0x00000001
#define ANC_COMPANDER_CONFIG_IS_FB_COMPANDER		0x00000002


// System Mode
#define ANC_COMPANDER_SYSMODE_PASS_THRU		0
#define ANC_COMPANDER_SYSMODE_UNUSED   		1
#define ANC_COMPANDER_SYSMODE_FULL     		2
#define ANC_COMPANDER_SYSMODE_MAX_MODES		3

// System Control
#define ANC_COMPANDER_CONTROL_MODE_OVERRIDE		0x2000

// Operator state

// Lookahead state

// Statistics Block
typedef struct _tag_ANC_COMPANDER_STATISTICS
{
	ParamType OFFSET_CUR_MODE;
	ParamType OFFSET_OVR_CONTROL;
	ParamType OFFSET_OP_STATE;
	ParamType OFFSET_LOOKAHEAD_FLAG;
	ParamType OFFSET_ANC_GAIN;
	ParamType OFFSET_BLOCK_SIZE;
	ParamType OFFSET_LEVEL;
}ANC_COMPANDER_STATISTICS;

typedef ANC_COMPANDER_STATISTICS* LP_ANC_COMPANDER_STATISTICS;

// Parameters Block
typedef struct _tag_ANC_COMPANDER_PARAMETERS
{
	ParamType OFFSET_COMPANDER_CONFIG;
	ParamType OFFSET_NUM_SECTIONS;
	ParamType OFFSET_GAIN_RATIO_SECTION1;
	ParamType OFFSET_GAIN_RATIO_SECTION2;
	ParamType OFFSET_GAIN_RATIO_SECTION3;
	ParamType OFFSET_GAIN_RATIO_SECTION4;
	ParamType OFFSET_GAIN_RATIO_SECTION5;
	ParamType OFFSET_GAIN_THRESHOLD_SECTION1;
	ParamType OFFSET_GAIN_THRESHOLD_SECTION2;
	ParamType OFFSET_GAIN_THRESHOLD_SECTION3;
	ParamType OFFSET_GAIN_THRESHOLD_SECTION4;
	ParamType OFFSET_GAIN_THRESHOLD_SECTION5;
	ParamType OFFSET_GAIN_KNEEWIDTH_SECTION1;
	ParamType OFFSET_GAIN_KNEEWIDTH_SECTION2;
	ParamType OFFSET_GAIN_KNEEWIDTH_SECTION3;
	ParamType OFFSET_GAIN_KNEEWIDTH_SECTION4;
	ParamType OFFSET_GAIN_KNEEWIDTH_SECTION5;
	ParamType OFFSET_GAIN_ATTACK_TC;
	ParamType OFFSET_GAIN_RELEASE_TC;
	ParamType OFFSET_LEVEL_ATTACK_TC;
	ParamType OFFSET_LEVEL_RELEASE_TC;
	ParamType OFFSET_LEVEL_AVERAGE_TC;
	ParamType OFFSET_MAKEUP_GAIN;
	ParamType OFFSET_LOOKAHEAD_TIME;
	ParamType OFFSET_LEVEL_ESTIMATION_FLAG;
	ParamType OFFSET_GAIN_UPDATE_FLAG;
	ParamType OFFSET_GAIN_INTERP_FLAG;
	ParamType OFFSET_SOFT_KNEE_1_COEFF_A;
	ParamType OFFSET_SOFT_KNEE_1_COEFF_B;
	ParamType OFFSET_SOFT_KNEE_1_COEFF_C;
	ParamType OFFSET_SOFT_KNEE_2_COEFF_A;
	ParamType OFFSET_SOFT_KNEE_2_COEFF_B;
	ParamType OFFSET_SOFT_KNEE_2_COEFF_C;
	ParamType OFFSET_SOFT_KNEE_3_COEFF_A;
	ParamType OFFSET_SOFT_KNEE_3_COEFF_B;
	ParamType OFFSET_SOFT_KNEE_3_COEFF_C;
	ParamType OFFSET_SOFT_KNEE_4_COEFF_A;
	ParamType OFFSET_SOFT_KNEE_4_COEFF_B;
	ParamType OFFSET_SOFT_KNEE_4_COEFF_C;
	ParamType OFFSET_SOFT_KNEE_5_COEFF_A;
	ParamType OFFSET_SOFT_KNEE_5_COEFF_B;
	ParamType OFFSET_SOFT_KNEE_5_COEFF_C;
	ParamType OFFSET_COMPANDER_PRIORITY;
}ANC_COMPANDER_PARAMETERS;

typedef ANC_COMPANDER_PARAMETERS* LP_ANC_COMPANDER_PARAMETERS;

unsigned *ANC_COMPANDER_GetDefaults(unsigned capid);

#endif // __ANC_COMPANDER_GEN_C_H__
