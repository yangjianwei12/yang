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
#ifndef __HCGR_GEN_ASM_H__
#define __HCGR_GEN_ASM_H__

// CodeBase IDs
.CONST $M.HCGR_HCGR_16K_CAP_ID       	0x00D8;
.CONST $M.HCGR_HCGR_16K_ALT_CAP_ID_0       	0x40B9;
.CONST $M.HCGR_HCGR_16K_SAMPLE_RATE       	16000;
.CONST $M.HCGR_HCGR_16K_VERSION_MAJOR       	2;

// Constant Values


// Piecewise Disables
.CONST $M.HCGR.CONFIG.IS_FF_AND_FB_HCGR		0x00000001;
.CONST $M.HCGR.CONFIG.BYPASS           		0x00000002;


// Statistic Block
.CONST $M.HCGR.STATUS.CUR_MODE         		0*ADDR_PER_WORD;
.CONST $M.HCGR.STATUS.OVR_CONTROL      		1*ADDR_PER_WORD;
.CONST $M.HCGR.STATUS.FLAGS            		2*ADDR_PER_WORD;
.CONST $M.HCGR.STATUS.FF_HW_TARGET_GAIN		3*ADDR_PER_WORD;
.CONST $M.HCGR.STATUS.FB_HW_TARGET_GAIN		4*ADDR_PER_WORD;
.CONST $M.HCGR.STATUS.BLOCK_LEVEL      		5*ADDR_PER_WORD;
.CONST $M.HCGR.STATUS.HOWL_BLOCK_LEVEL 		6*ADDR_PER_WORD;
.CONST $M.HCGR.STATUS.HOWL_FREQ        		7*ADDR_PER_WORD;
.CONST $M.HCGR.STATUS.HCGR_FF_GAIN     		8*ADDR_PER_WORD;
.CONST $M.HCGR.STATUS.HCGR_FB_GAIN     		9*ADDR_PER_WORD;
.CONST $M.HCGR.STATUS.BLOCK_SIZE            	10;

// System Mode
.CONST $M.HCGR.SYSMODE.STANDBY		0;
.CONST $M.HCGR.SYSMODE.FULL   		1;
.CONST $M.HCGR.SYSMODE.MAX_MODES		2;

// System Control
.CONST $M.HCGR.CONTROL.MODE_OVERRIDE		0x2000;

// Flags
.CONST $M.HCGR.FLAGS.FF_RECOVERY		0x00000002;
.CONST $M.HCGR.FLAGS.FB_RECOVERY		0x00000004;

// FF HW Target Gain

// FB HW Target Gain

// HCGR FF Delta Gain

// HCGR FB Delta Gain

// Parameter Block
.CONST $M.HCGR.PARAMETERS.OFFSET_HCGR_CONFIG                 		0*ADDR_PER_WORD;
.CONST $M.HCGR.PARAMETERS.OFFSET_HCGR_PRIORITY               		1*ADDR_PER_WORD;
.CONST $M.HCGR.PARAMETERS.OFFSET_NOP_BELOW_BIN               		2*ADDR_PER_WORD;
.CONST $M.HCGR.PARAMETERS.OFFSET_PTPR_THRESHOLD_EXP          		3*ADDR_PER_WORD;
.CONST $M.HCGR.PARAMETERS.OFFSET_PTPR_THRESHOLD_MANT         		4*ADDR_PER_WORD;
.CONST $M.HCGR.PARAMETERS.OFFSET_PTPR_THRESHOLD_DC_BEXP      		5*ADDR_PER_WORD;
.CONST $M.HCGR.PARAMETERS.OFFSET_PAPR_THRESHOLD_SHIFT        		6*ADDR_PER_WORD;
.CONST $M.HCGR.PARAMETERS.OFFSET_PNPR_THRESHOLD              		7*ADDR_PER_WORD;
.CONST $M.HCGR.PARAMETERS.OFFSET_IFPR_GROWTH_SCALE           		8*ADDR_PER_WORD;
.CONST $M.HCGR.PARAMETERS.OFFSET_BIN1_TRIGGER_DETECT_COUNT   		9*ADDR_PER_WORD;
.CONST $M.HCGR.PARAMETERS.OFFSET_BIN1_FRAME_RESET_COUNT      		10*ADDR_PER_WORD;
.CONST $M.HCGR.PARAMETERS.OFFSET_IMPULSE_SCALE               		11*ADDR_PER_WORD;
.CONST $M.HCGR.PARAMETERS.OFFSET_COUNTER_IMPULSIVE           		12*ADDR_PER_WORD;
.CONST $M.HCGR.PARAMETERS.OFFSET_COUNTER_REGULAR             		13*ADDR_PER_WORD;
.CONST $M.HCGR.PARAMETERS.OFFSET_COUNTER_TRIGGER             		14*ADDR_PER_WORD;
.CONST $M.HCGR.PARAMETERS.OFFSET_STEP_SIZE_0DB_FS            		15*ADDR_PER_WORD;
.CONST $M.HCGR.PARAMETERS.OFFSET_STEP_SIZE_6DB_FS            		16*ADDR_PER_WORD;
.CONST $M.HCGR.PARAMETERS.OFFSET_STEP_SIZE_12DB_FS           		17*ADDR_PER_WORD;
.CONST $M.HCGR.PARAMETERS.OFFSET_STEP_SIZE_18DB_FS           		18*ADDR_PER_WORD;
.CONST $M.HCGR.PARAMETERS.OFFSET_STEP_SIZE_24DB_FS           		19*ADDR_PER_WORD;
.CONST $M.HCGR.PARAMETERS.OFFSET_STEP_SIZE_30DB_FS           		20*ADDR_PER_WORD;
.CONST $M.HCGR.PARAMETERS.OFFSET_STEP_SIZE_36DB_FS           		21*ADDR_PER_WORD;
.CONST $M.HCGR.PARAMETERS.OFFSET_STEP_SIZE_42DB_FS           		22*ADDR_PER_WORD;
.CONST $M.HCGR.PARAMETERS.OFFSET_MINIMUM_FF_GAIN             		23*ADDR_PER_WORD;
.CONST $M.HCGR.PARAMETERS.OFFSET_MINIMUM_FB_GAIN             		24*ADDR_PER_WORD;
.CONST $M.HCGR.PARAMETERS.OFFSET_GAIN_ATTACK_TIME            		25*ADDR_PER_WORD;
.CONST $M.HCGR.PARAMETERS.OFFSET_AHM_FRAME_SIZE              		26*ADDR_PER_WORD;
.CONST $M.HCGR.PARAMETERS.OFFSET_GAIN_RECOVERY_RATE          		27*ADDR_PER_WORD;
.CONST $M.HCGR.PARAMETERS.OFFSET_GAIN_RECOVERY_RATE_SLOW     		28*ADDR_PER_WORD;
.CONST $M.HCGR.PARAMETERS.OFFSET_GAIN_RECOVERY_RATE_SLOWEST  		29*ADDR_PER_WORD;
.CONST $M.HCGR.PARAMETERS.OFFSET_SLOW_RECOVERY_GAIN_THRESHOLD		30*ADDR_PER_WORD;
.CONST $M.HCGR.PARAMETERS.STRUCT_SIZE                       		31;


#endif // __HCGR_GEN_ASM_H__
