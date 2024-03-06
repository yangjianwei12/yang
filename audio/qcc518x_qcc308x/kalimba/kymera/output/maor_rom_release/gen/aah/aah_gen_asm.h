// -----------------------------------------------------------------------------
// Copyright (c) 2023                  Qualcomm Technologies International, Ltd.
//
// Generated by /home/svc-audio-dspsw/kymera_builds/builds/2023/kymera_2312060823/kalimba/kymera/../../util/CommonParameters/DerivationEngine.py
// code v3.5, file //depot/dspsw/maor_rom_v20/util/CommonParameters/DerivationEngine.py, revision 2
// namespace {com.csr.cps.4}UnifiedParameterSchema on 2023-12-06 08:59:15 by svc-audio-dspsw
//
// input from EMPTY
// last change  by  on 
// -----------------------------------------------------------------------------
#ifndef __AAH_GEN_ASM_H__
#define __AAH_GEN_ASM_H__

// CodeBase IDs
.CONST $M.AAH_AAH_CAP_ID       	0x00EA;
.CONST $M.AAH_AAH_ALT_CAP_ID_0       	0x40D0;
.CONST $M.AAH_AAH_SAMPLE_RATE       	16000;
.CONST $M.AAH_AAH_VERSION_MAJOR       	1;

// Constant Values


// Piecewise Disables
.CONST $M.AAH.CONFIG.ENABLE		0x00000001;


// Statistic Block
.CONST $M.AAH.STATUS.CUR_MODE        		0*ADDR_PER_WORD;
.CONST $M.AAH.STATUS.EC_DELTA_GAIN   		1*ADDR_PER_WORD;
.CONST $M.AAH.STATUS.FB_DELTA_GAIN   		2*ADDR_PER_WORD;
.CONST $M.AAH.STATUS.FF_DELTA_GAIN   		3*ADDR_PER_WORD;
.CONST $M.AAH.STATUS.FLAGS           		4*ADDR_PER_WORD;
.CONST $M.AAH.STATUS.ACTUAL_DELAY    		5*ADDR_PER_WORD;
.CONST $M.AAH.STATUS.FRAMES          		6*ADDR_PER_WORD;
.CONST $M.AAH.STATUS.WATERMARK_IN_BUF		7*ADDR_PER_WORD;
.CONST $M.AAH.STATUS.SYNC_LOSS       		8*ADDR_PER_WORD;
.CONST $M.AAH.STATUS.BLOCK_SIZE           	9;

// System Mode
.CONST $M.AAH.SYSMODE.STANDBY		0;
.CONST $M.AAH.SYSMODE.FULL   		1;
.CONST $M.AAH.SYSMODE.MAX_MODES		2;

// Parameter Block
.CONST $M.AAH.PARAMETERS.OFFSET_AHM_PRIORITY        		0*ADDR_PER_WORD;
.CONST $M.AAH.PARAMETERS.OFFSET_AHM_ATTACK_TIME     		1*ADDR_PER_WORD;
.CONST $M.AAH.PARAMETERS.OFFSET_AHM_RELEASE_TIME    		2*ADDR_PER_WORD;
.CONST $M.AAH.PARAMETERS.OFFSET_AAH_CONFIG          		3*ADDR_PER_WORD;
.CONST $M.AAH.PARAMETERS.OFFSET_GAIN_SCALE_RX_DB    		4*ADDR_PER_WORD;
.CONST $M.AAH.PARAMETERS.OFFSET_GAIN_SCALE_FB_MIC_DB		5*ADDR_PER_WORD;
.CONST $M.AAH.PARAMETERS.OFFSET_GAIN_SCALE_FF_MIC_DB		6*ADDR_PER_WORD;
.CONST $M.AAH.PARAMETERS.OFFSET_GAIN_SCALE_RX_MIX_DB		7*ADDR_PER_WORD;
.CONST $M.AAH.PARAMETERS.OFFSET_LIMIT_LEVEL_FB_DB   		8*ADDR_PER_WORD;
.CONST $M.AAH.PARAMETERS.OFFSET_LIMIT_LEVEL_FF_DB   		9*ADDR_PER_WORD;
.CONST $M.AAH.PARAMETERS.OFFSET_LIMIT_LEVEL_COMB_DB 		10*ADDR_PER_WORD;
.CONST $M.AAH.PARAMETERS.OFFSET_AAH_TC_ATTACK       		11*ADDR_PER_WORD;
.CONST $M.AAH.PARAMETERS.OFFSET_AAH_TC_RELEASE      		12*ADDR_PER_WORD;
.CONST $M.AAH.PARAMETERS.OFFSET_AAH_TC_HOLD         		13*ADDR_PER_WORD;
.CONST $M.AAH.PARAMETERS.OFFSET_PXIIR_EC_COEF_0     		14*ADDR_PER_WORD;
.CONST $M.AAH.PARAMETERS.OFFSET_PXIIR_EC_COEF_1     		15*ADDR_PER_WORD;
.CONST $M.AAH.PARAMETERS.OFFSET_PXIIR_EC_COEF_2     		16*ADDR_PER_WORD;
.CONST $M.AAH.PARAMETERS.OFFSET_PXIIR_FB_COEF_0     		17*ADDR_PER_WORD;
.CONST $M.AAH.PARAMETERS.OFFSET_PXIIR_FB_COEF_1     		18*ADDR_PER_WORD;
.CONST $M.AAH.PARAMETERS.OFFSET_PXIIR_FB_COEF_2     		19*ADDR_PER_WORD;
.CONST $M.AAH.PARAMETERS.OFFSET_PXIIR_FF_COEF_0     		20*ADDR_PER_WORD;
.CONST $M.AAH.PARAMETERS.OFFSET_PXIIR_FF_COEF_1     		21*ADDR_PER_WORD;
.CONST $M.AAH.PARAMETERS.OFFSET_PXIIR_FF_COEF_2     		22*ADDR_PER_WORD;
.CONST $M.AAH.PARAMETERS.OFFSET_RX_DELAY            		23*ADDR_PER_WORD;
.CONST $M.AAH.PARAMETERS.STRUCT_SIZE               		24;


#endif // __AAH_GEN_ASM_H__
