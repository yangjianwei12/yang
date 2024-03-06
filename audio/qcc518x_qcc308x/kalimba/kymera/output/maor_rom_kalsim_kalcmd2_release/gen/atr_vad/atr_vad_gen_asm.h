// -----------------------------------------------------------------------------
// Copyright (c) 2023                  Qualcomm Technologies International, Ltd.
//
// Generated by /home/svc-audio-dspsw/kymera_builds/builds/2023/kymera_2312060823/kalimba/kymera/../../util/CommonParameters/DerivationEngine.py
// code v3.5, file //depot/dspsw/maor_rom_v20/util/CommonParameters/DerivationEngine.py, revision 2
// namespace {com.csr.cps.4}UnifiedParameterSchema on 2023-12-06 09:14:54 by svc-audio-dspsw
//
// input from EMPTY
// last change  by  on 
// -----------------------------------------------------------------------------
#ifndef __ATR_VAD_GEN_ASM_H__
#define __ATR_VAD_GEN_ASM_H__

// CodeBase IDs
.CONST $M.ATR_VAD_ATR_VAD_CAP_ID       	0x00E3;
.CONST $M.ATR_VAD_ATR_VAD_ALT_CAP_ID_0       	0x40C9;
.CONST $M.ATR_VAD_ATR_VAD_SAMPLE_RATE       	16000;
.CONST $M.ATR_VAD_ATR_VAD_VERSION_MAJOR       	1;

// Constant Values


// Piecewise Disables
// ATR_VAD_CONFIG bits
.CONST $M.ATR_VAD.CONFIG.ATR_VAD_CONFIG.2MIC_ONLY		0x00000001;


// Statistic Block
.CONST $M.ATR_VAD.STATUS.CUR_MODE   		0*ADDR_PER_WORD;
.CONST $M.ATR_VAD.STATUS.OVR_CONTROL		1*ADDR_PER_WORD;
.CONST $M.ATR_VAD.STATUS.POWER      		2*ADDR_PER_WORD;
.CONST $M.ATR_VAD.STATUS.DETECTION  		3*ADDR_PER_WORD;
.CONST $M.ATR_VAD.STATUS.EVENT_STATE		4*ADDR_PER_WORD;
.CONST $M.ATR_VAD.STATUS.BLOCK_SIZE      	5;

// System Mode
.CONST $M.ATR_VAD.SYSMODE.STANDBY		0;
.CONST $M.ATR_VAD.SYSMODE.1MIC   		2;
.CONST $M.ATR_VAD.SYSMODE.1MIC_MS		3;
.CONST $M.ATR_VAD.SYSMODE.2MIC   		4;
.CONST $M.ATR_VAD.SYSMODE.MAX_MODES		5;

// System Control
.CONST $M.ATR_VAD.CONTROL.MODE_OVERRIDE		0x2000;

// Detection
.CONST $M.ATR_VAD.DETECTION.1MIC		0x00000001;
.CONST $M.ATR_VAD.DETECTION.2MIC		0x00000002;

// Event State
.CONST $M.ATR_VAD.EVENT_STATE.RELEASE        		0x00000000;
.CONST $M.ATR_VAD.EVENT_STATE.ATTACK_COUNT   		0x00000001;
.CONST $M.ATR_VAD.EVENT_STATE.ATTACK_MESSAGE 		0x00000002;
.CONST $M.ATR_VAD.EVENT_STATE.ATTACK         		0x00000003;
.CONST $M.ATR_VAD.EVENT_STATE.RELEASE_COUNT  		0x00000004;
.CONST $M.ATR_VAD.EVENT_STATE.RELEASE_MESSAGE		0x00000005;

// Parameter Block
.CONST $M.ATR_VAD.PARAMETERS.OFFSET_ATR_VAD_CONFIG         		0*ADDR_PER_WORD;
.CONST $M.ATR_VAD.PARAMETERS.OFFSET_VAD_ATTACK_TIME        		1*ADDR_PER_WORD;
.CONST $M.ATR_VAD.PARAMETERS.OFFSET_VAD_RELEASE_TIME       		2*ADDR_PER_WORD;
.CONST $M.ATR_VAD.PARAMETERS.OFFSET_VAD_ENVELOPE_TIME      		3*ADDR_PER_WORD;
.CONST $M.ATR_VAD.PARAMETERS.OFFSET_VAD_RATIO              		4*ADDR_PER_WORD;
.CONST $M.ATR_VAD.PARAMETERS.OFFSET_VAD_INIT_TIME          		5*ADDR_PER_WORD;
.CONST $M.ATR_VAD.PARAMETERS.OFFSET_VAD_PWR_THRESHOLD      		6*ADDR_PER_WORD;
.CONST $M.ATR_VAD.PARAMETERS.OFFSET_VAD_ENV_THRESHOLD      		7*ADDR_PER_WORD;
.CONST $M.ATR_VAD.PARAMETERS.OFFSET_VAD_DELTA_THRESHOLD    		8*ADDR_PER_WORD;
.CONST $M.ATR_VAD.PARAMETERS.OFFSET_VAD_COUNT_TH_TIME      		9*ADDR_PER_WORD;
.CONST $M.ATR_VAD.PARAMETERS.OFFSET_SMOOTH_ATTACK_TIME     		10*ADDR_PER_WORD;
.CONST $M.ATR_VAD.PARAMETERS.OFFSET_SMOOTH_RELEASE_TIME    		11*ADDR_PER_WORD;
.CONST $M.ATR_VAD.PARAMETERS.OFFSET_HOLD_ATTACK_TIME       		12*ADDR_PER_WORD;
.CONST $M.ATR_VAD.PARAMETERS.OFFSET_HOLD_RELEASE_TIME      		13*ADDR_PER_WORD;
.CONST $M.ATR_VAD.PARAMETERS.OFFSET_DET_THRESHOLD          		14*ADDR_PER_WORD;
.CONST $M.ATR_VAD.PARAMETERS.OFFSET_MSG_SHORT_RELEASE_TIME 		15*ADDR_PER_WORD;
.CONST $M.ATR_VAD.PARAMETERS.OFFSET_MSG_NORMAL_RELEASE_TIME		16*ADDR_PER_WORD;
.CONST $M.ATR_VAD.PARAMETERS.OFFSET_MSG_LONG_RELEASE_TIME  		17*ADDR_PER_WORD;
.CONST $M.ATR_VAD.PARAMETERS.OFFSET_MSG_ATTACK_TIME        		18*ADDR_PER_WORD;
.CONST $M.ATR_VAD.PARAMETERS.STRUCT_SIZE                  		19;


#endif // __ATR_VAD_GEN_ASM_H__
