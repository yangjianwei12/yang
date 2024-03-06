// -----------------------------------------------------------------------------
// Copyright (c) 2023                  Qualcomm Technologies International, Ltd.
//
#ifndef __XOVER_GEN_C_H__
#define __XOVER_GEN_C_H__

#ifndef ParamType
typedef unsigned ParamType;
#endif

// CodeBase IDs
#define XOVER_XOVER_CAP_ID	0x000033
#define XOVER_XOVER_ALT_CAP_ID_0	0x406B
#define XOVER_XOVER_SAMPLE_RATE	0
#define XOVER_XOVER_VERSION_MAJOR	1

#define XOVER_XOVER_3BAND_CAP_ID	0x000034
#define XOVER_XOVER_3BAND_ALT_CAP_ID_0	0x409D
#define XOVER_XOVER_3BAND_SAMPLE_RATE	0
#define XOVER_XOVER_3BAND_VERSION_MAJOR	1

// Constant Definitions
#define XOVER_CONSTANT_XOVER_FILTER_TYPE_Single_precision		0x00000000
#define XOVER_CONSTANT_XOVER_FILTER_TYPE_First_order     		0x00000001
#define XOVER_CONSTANT_XOVER_FILTER_TYPE_Double_precision		0x00000002
#define XOVER_CONSTANT_LP_TYPE_Butterworth               		0x00000001
#define XOVER_CONSTANT_LP_TYPE_Linkwitz_Riley            		0x00000002
#define XOVER_CONSTANT_LP_TYPE_APC                       		0x00000003
#define XOVER_CONSTANT_HP_TYPE_Butterworth               		0x00000001
#define XOVER_CONSTANT_HP_TYPE_Linkwitz_Riley            		0x00000002
#define XOVER_CONSTANT_HP_TYPE_APC                       		0x00000003
#define XOVER_CONSTANT_LP_TYPE_F2_Butterworth            		0x00000001
#define XOVER_CONSTANT_LP_TYPE_F2_Linkwitz_Riley         		0x00000002
#define XOVER_CONSTANT_LP_TYPE_F2_APC                    		0x00000003
#define XOVER_CONSTANT_HP_TYPE_F2_Butterworth            		0x00000001
#define XOVER_CONSTANT_HP_TYPE_F2_Linkwitz_Riley         		0x00000002
#define XOVER_CONSTANT_HP_TYPE_F2_APC                    		0x00000003


// Runtime Config Parameter Bitfields
#define XOVER_CONFIG_BYPXFAD  		0x00000001
#define XOVER_CONFIG_INV_BAND1		0x00000002
#define XOVER_CONFIG_INV_BAND2		0x00000004
#define XOVER_CONFIG_INV_BAND3		0x00000008


// System Mode
#define XOVER_SYSMODE_STATIC   		0
#define XOVER_SYSMODE_MUTE     		1
#define XOVER_SYSMODE_FULL     		2
#define XOVER_SYSMODE_PASS_THRU		3
#define XOVER_SYSMODE_MAX_MODES		4

// System Control
#define XOVER_CONTROL_MODE_OVERRIDE		0x2000

// CompCfg

// Operator state

// Operator internal state

// Statistics Block
typedef struct _tag_XOVER_STATISTICS
{
	ParamType OFFSET_CUR_MODE;
	ParamType OFFSET_OVR_CONTROL;
	ParamType OFFSET_COMPILED_CONFIG;
	ParamType OFFSET_OP_STATE;
	ParamType OFFSET_OP_INTERNAL_STATE;
}XOVER_STATISTICS;

typedef XOVER_STATISTICS* LP_XOVER_STATISTICS;

// Parameters Block
typedef struct _tag_XOVER_PARAMETERS
{
	ParamType OFFSET_XOVER_CONFIG;
	ParamType OFFSET_XOVER_FILTER_TYPE;
	ParamType OFFSET_LP_TYPE;
	ParamType OFFSET_LP_FC;
	ParamType OFFSET_LP_ORDER;
	ParamType OFFSET_HP_TYPE;
	ParamType OFFSET_HP_FC;
	ParamType OFFSET_HP_ORDER;
	ParamType OFFSET_LP_TYPE_F2;
	ParamType OFFSET_LP_FC_F2;
	ParamType OFFSET_LP_ORDER_F2;
	ParamType OFFSET_HP_TYPE_F2;
	ParamType OFFSET_HP_FC_F2;
	ParamType OFFSET_HP_ORDER_F2;
}XOVER_PARAMETERS;

typedef XOVER_PARAMETERS* LP_XOVER_PARAMETERS;

unsigned *XOVER_GetDefaults(unsigned capid);

#endif // __XOVER_GEN_C_H__
