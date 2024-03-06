// -----------------------------------------------------------------------------
// Copyright (c) 2023                  Qualcomm Technologies International, Ltd.
//
// Generated by /home/svc-audio-dspsw/kymera_builds/builds/2023/kymera_2312060823/kalimba/kymera/../../util/CommonParameters/DerivationEngine.py
// code v3.5, file //depot/dspsw/maor_rom_v20/util/CommonParameters/DerivationEngine.py, revision 2
// namespace {com.csr.cps.4}UnifiedParameterSchema on 2023-12-06 08:45:22 by svc-audio-dspsw
//
// input from EMPTY
// last change  by  on 
// -----------------------------------------------------------------------------
#include "earbud_fit_test_gen_c.h"

#ifndef __GNUC__ 
_Pragma("datasection CONST")
#endif /* __GNUC__ */

static unsigned defaults_earbud_fit_testEARBUD_FIT_TEST_16K[] = {
   0x00CBC6A8u,			// POWER_SMOOTH_TIME
   0x05000000u,			// FIT_THRESHOLD
   0x00300000u,			// EVENT_GOOD_FIT
   0x00000001u,			// BIN_SELECT
   0x000001F4u,			// AUTO_FIT_CAPTURE_INTERVAL_MS
   0x00000004u,			// AUTO_FIT_MSGS_PER_CAPTURE_INTERVAL
   0x0000012Cu,			// AUTO_FIT_GAIN_SMOOTH_TC_MS
   0xFFFFFF9Cu,			// AUTO_FIT_SENSITIVITY_THRSHLD_DB
   0x00000000u,			// AUTO_FIT_CLIPPING_THRSHLD_DB
   0xFFFFFED4u,			// AUTO_FIT_BAND_GAIN_MIN_DB
   0x00000384u,			// AUTO_FIT_BAND_GAIN_MAX_DB
   0x00000004u,			// AUTO_FIT_NUM_BANDS
   0x0000007Du,			// AUTO_FIT_BAND1_START_FREQ_HZ
   0x0000012Cu,			// AUTO_FIT_BAND1_FREQ_HZ
   0x00000226u,			// AUTO_FIT_BAND2_FREQ_HZ
   0x00000320u,			// AUTO_FIT_BAND3_FREQ_HZ
   0x0000041Au,			// AUTO_FIT_BAND4_FREQ_HZ
   0x00000514u,			// AUTO_FIT_BAND5_FREQ_HZ
   0x0000060Eu,			// AUTO_FIT_BAND6_FREQ_HZ
   0x00000000u,			// AUTO_FIT_BAND1_EQ_GAIN_OFFS_DB
   0x00000000u,			// AUTO_FIT_BAND2_EQ_GAIN_OFFS_DB
   0x00000000u,			// AUTO_FIT_BAND3_EQ_GAIN_OFFS_DB
   0x00000000u,			// AUTO_FIT_BAND4_EQ_GAIN_OFFS_DB
   0x00000000u,			// AUTO_FIT_BAND5_EQ_GAIN_OFFS_DB
   0x00000000u			// AUTO_FIT_BAND6_EQ_GAIN_OFFS_DB
};

unsigned *EARBUD_FIT_TEST_GetDefaults(unsigned capid){
	switch(capid){
		case 0x00CA: return defaults_earbud_fit_testEARBUD_FIT_TEST_16K;
		case 0x40A2: return defaults_earbud_fit_testEARBUD_FIT_TEST_16K;
	}
	return((unsigned *)0);
}