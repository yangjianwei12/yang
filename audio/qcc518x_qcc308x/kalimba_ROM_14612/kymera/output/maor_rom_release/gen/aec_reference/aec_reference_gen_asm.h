// -----------------------------------------------------------------------------
// Copyright (c) 2021                  Qualcomm Technologies International, Ltd.
//
#ifndef __AEC_REFERENCE_GEN_ASM_H__
#define __AEC_REFERENCE_GEN_ASM_H__

// CodeBase IDs
.CONST $M.AEC_REFERENCE_AECREF_CAP_ID       	0x0043;
.CONST $M.AEC_REFERENCE_AECREF_ALT_CAP_ID_0       	0x4007;
.CONST $M.AEC_REFERENCE_AECREF_SAMPLE_RATE       	0;
.CONST $M.AEC_REFERENCE_AECREF_VERSION_MAJOR       	2;

// Constant Values
.CONST $M.AEC_REFERENCE.CONSTANT.CONN_MIKE_1           		0x00000001;
.CONST $M.AEC_REFERENCE.CONSTANT.CONN_MIKE_2           		0x00000002;
.CONST $M.AEC_REFERENCE.CONSTANT.CONN_MIKE_3           		0x00000004;
.CONST $M.AEC_REFERENCE.CONSTANT.CONN_MIKE_4           		0x00000008;
.CONST $M.AEC_REFERENCE.CONSTANT.CONN_MIKE_5           		0x00010000;
.CONST $M.AEC_REFERENCE.CONSTANT.CONN_MIKE_6           		0x00020000;
.CONST $M.AEC_REFERENCE.CONSTANT.CONN_MIKE_7           		0x00040000;
.CONST $M.AEC_REFERENCE.CONSTANT.CONN_MIKE_8           		0x00080000;
.CONST $M.AEC_REFERENCE.CONSTANT.CONN_MIKE_1_INPUT_ONLY		0x00100000;
.CONST $M.AEC_REFERENCE.CONSTANT.CONN_SPKR_1           		0x00000010;
.CONST $M.AEC_REFERENCE.CONSTANT.CONN_SPKR_2           		0x00000020;
.CONST $M.AEC_REFERENCE.CONSTANT.CONN_SPKR_3           		0x00000040;
.CONST $M.AEC_REFERENCE.CONSTANT.CONN_SPKR_4           		0x00000080;
.CONST $M.AEC_REFERENCE.CONSTANT.CONN_SPKR_5           		0x00000100;
.CONST $M.AEC_REFERENCE.CONSTANT.CONN_SPKR_6           		0x00000200;
.CONST $M.AEC_REFERENCE.CONSTANT.CONN_SPKR_7           		0x00000400;
.CONST $M.AEC_REFERENCE.CONSTANT.CONN_SPKR_8           		0x00000800;
.CONST $M.AEC_REFERENCE.CONSTANT.CONN_TYPE_PARA        		0x00001000;
.CONST $M.AEC_REFERENCE.CONSTANT.CONN_TYPE_MIX         		0x00002000;
.CONST $M.AEC_REFERENCE.CONSTANT.CONN_TYPE_REF         		0x00004000;


// Piecewise Disables
.CONST $M.AEC_REFERENCE.CONFIG.SIDETONEENA        		0x00000010;
.CONST $M.AEC_REFERENCE.CONFIG.SIDETONE_DISABLE   		0x00000008;
.CONST $M.AEC_REFERENCE.CONFIG.SUPPORT_HW_SIDETONE		0x00000004;


// Statistic Block
.CONST $M.AEC_REFERENCE.STATUS.CUR_MODE_OFFSET   		0*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.STATUS.SYS_CONTROL_OFFSET		1*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.STATUS.COMPILED_CONFIG   		2*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.STATUS.VOLUME            		3*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.STATUS.CONNECTION        		4*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.STATUS.MIC_SAMPLERATE    		5*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.STATUS.OUT_SAMPLERATE    		6*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.STATUS.IN_SAMPLERATE     		7*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.STATUS.SPKR_SAMPLERATE   		8*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.STATUS.RATE_ENACT_MIC    		9*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.STATUS.RATE_ENACT_SPKR   		10*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.STATUS.MIC_MEASURE       		11*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.STATUS.SPKR_MEASURE      		12*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.STATUS.MIC_ADJUST        		13*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.STATUS.SPKR_ADJUST       		14*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.STATUS.REF_ADJUST        		15*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.STATUS.SPKR_INSERTS      		16*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.STATUS.REF_DROPS_INSERTS 		17*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.STATUS.ST_DROPS          		18*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.STATUS.MIC_REF_DELAY     		19*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.STATUS.HL_DETECT_CNT     		20*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.STATUS.BLOCK_SIZE             	21;

// System Mode
.CONST $M.AEC_REFERENCE.SYSMODE.STATIC		0;
.CONST $M.AEC_REFERENCE.SYSMODE.FULL  		1;
.CONST $M.AEC_REFERENCE.SYSMODE.MAX_MODES		2;

// System Control

// Mic RM Type
.CONST $M.AEC_REFERENCE.RATEMATCHING_SUPPORT_NONE		0;
.CONST $M.AEC_REFERENCE.RATEMATCHING_SUPPORT_SW  		1;
.CONST $M.AEC_REFERENCE.RATEMATCHING_SUPPORT_HW  		2;
.CONST $M.AEC_REFERENCE.RATEMATCHING_SUPPORT_AUTO		3;

// Spkr RM Type
.CONST $M.AEC_REFERENCE.RATEMATCHING_SUPPORT_NONE		0;
.CONST $M.AEC_REFERENCE.RATEMATCHING_SUPPORT_SW  		1;
.CONST $M.AEC_REFERENCE.RATEMATCHING_SUPPORT_HW  		2;
.CONST $M.AEC_REFERENCE.RATEMATCHING_SUPPORT_AUTO		3;

// Parameter Block
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_CONFIG              		0*ADDR_PER_WORD;
// Microphone Gains
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_ADC_GAIN1           		1*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_ADC_GAIN2           		2*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_ADC_GAIN3           		3*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_ADC_GAIN4           		4*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_ADC_GAIN5           		5*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_ADC_GAIN6           		6*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_ADC_GAIN7           		7*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_ADC_GAIN8           		8*ADDR_PER_WORD;
// Sidetone with High-Pass Filters
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_ST_CLIP_POINT       		9*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_ST_ADJUST_LIMIT     		10*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_STF_SWITCH          		11*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_STF_NOISE_LOW_THRES 		12*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_STF_NOISE_HIGH_THRES		13*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_STF_GAIN_EXP        		14*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_STF_GAIN_MANTISSA   		15*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_ST_PEQ_CONFIG       		16*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_ST_PEQ_GAIN_EXP     		17*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_ST_PEQ_GAIN_MANT    		18*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_ST_PEQ_STAGE1_B2    		19*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_ST_PEQ_STAGE1_B1    		20*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_ST_PEQ_STAGE1_B0    		21*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_ST_PEQ_STAGE1_A2    		22*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_ST_PEQ_STAGE1_A1    		23*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_ST_PEQ_STAGE2_B2    		24*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_ST_PEQ_STAGE2_B1    		25*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_ST_PEQ_STAGE2_B0    		26*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_ST_PEQ_STAGE2_A2    		27*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_ST_PEQ_STAGE2_A1    		28*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_ST_PEQ_STAGE3_B2    		29*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_ST_PEQ_STAGE3_B1    		30*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_ST_PEQ_STAGE3_B0    		31*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_ST_PEQ_STAGE3_A2    		32*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_ST_PEQ_STAGE3_A1    		33*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_ST_PEQ_SCALE1       		34*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_ST_PEQ_SCALE2       		35*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_ST_PEQ_SCALE3       		36*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_HL_SWITCH           		37*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_HL_LIMIT_LEVEL      		38*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_HL_LIMIT_THRESHOLD  		39*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_HL_LIMIT_HOLD_MS    		40*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_HL_LIMIT_DETECT_FC  		41*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_HL_LIMIT_TC         		42*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_STF_GAIN_RAMP_EXP   		43*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_STF_GAIN_RAMP_LIN   		44*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.OFFSET_SIDETONE_MICS_MAP0  		45*ADDR_PER_WORD;
.CONST $M.AEC_REFERENCE.PARAMETERS.STRUCT_SIZE               		46;


#endif // __AEC_REFERENCE_GEN_ASM_H__
