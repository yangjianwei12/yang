// *****************************************************************************
// Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
// %%version
//
// *****************************************************************************

#ifndef _CVC_SEND_DATA_ASM_H
#define _CVC_SEND_DATA_ASM_H

#include "cvc_modules.h"

#if !defined(CAPABILITY_DOWNLOAD_BUILD)
    #include "cvc_send_gen_asm.h" 
#else 
    #include "cvc_send_gen_asm_dl.h"
#endif
#include "mbdrc100_library_asm_defs.h"
#include "cvc_send_cap_asm.h"
#include "cvc_send_tmm.h"
#if defined(INSTALL_OPERATOR_CVC_EARBUD_3MIC_HYBRID) || defined(INSTALL_OPERATOR_CVC_HEADSET_2MIC_MONO_HYBRID) || defined(INSTALL_OPERATOR_CVC_HEADSET_1MIC_HYBRID)
   #include "mlfe100_library.h"
   #include "mlfe100_library_c_asm_defs.h"
   #include "cvc_noise_sup_prepost_asm_defs.h"
#endif
#include "cvc_send_cap_dl.h"

.CONST $BEXP                  8; // MK1 * 2 

// -----------------------------------------------------------------------------
// CVC SEND DATA ROOT STRUCTURE
// -----------------------------------------------------------------------------
.CONST $cvc_send.data.config_flag          0*ADDR_PER_WORD;
.CONST $cvc_send.data.cap_root_ptr         1*ADDR_PER_WORD;
.CONST $cvc_send.data.param                2*ADDR_PER_WORD;
.CONST $cvc_send.data.harm_obj             3*ADDR_PER_WORD;
.CONST $cvc_send.data.dms200_obj           4*ADDR_PER_WORD;

.CONST $cvc_send.data.one                  5*ADDR_PER_WORD;
.CONST $cvc_send.data.zero                 6*ADDR_PER_WORD;
.CONST $cvc_send.data.use                  ADDR_PER_WORD + $cvc_send.data.zero;
.CONST $cvc_send.data.mic_mode             ADDR_PER_WORD + $cvc_send.data.use;
.CONST $cvc_send.data.end_fire             ADDR_PER_WORD + $cvc_send.data.mic_mode;
.CONST $cvc_send.data.hfk_config           ADDR_PER_WORD + $cvc_send.data.end_fire;
.CONST $cvc_send.data.dmss_config          ADDR_PER_WORD + $cvc_send.data.hfk_config;
.CONST $cvc_send.data.wind_flag            ADDR_PER_WORD + $cvc_send.data.dmss_config;
.CONST $cvc_send.data.echo_flag            ADDR_PER_WORD + $cvc_send.data.wind_flag;
.CONST $cvc_send.data.vad_flag             ADDR_PER_WORD + $cvc_send.data.echo_flag;
.CONST $cvc_send.data.TP_mode              ADDR_PER_WORD + $cvc_send.data.vad_flag;
.CONST $cvc_send.data.aec_inactive         ADDR_PER_WORD + $cvc_send.data.TP_mode;
.CONST $cvc_send.data.fftwin_power         ADDR_PER_WORD + $cvc_send.data.aec_inactive;
.CONST $cvc_send.data.power_adjust         ADDR_PER_WORD + $cvc_send.data.fftwin_power;
.CONST $cvc_send.data.dmss_obj             ADDR_PER_WORD + $cvc_send.data.power_adjust;
.CONST $cvc_send.data.selfclean100_dobj    ADDR_PER_WORD + $cvc_send.data.dmss_obj;
.CONST $cvc_send.data.selfclean_flag       ADDR_PER_WORD + $cvc_send.data.selfclean100_dobj;
.CONST $cvc_send.data.ref_power_obj        ADDR_PER_WORD + $cvc_send.data.selfclean_flag;

#ifndef CVC_INEAR_MAOR_ROM_HARDWARE_BUILD
.CONST $cvc_send.data.STRUC_SIZE           1 + ($cvc_send.data.ref_power_obj >> LOG2_ADDR_PER_WORD);
#else
.CONST $cvc_send.data.STRUC_SIZE_HW        1 + ($cvc_send.data.ref_power_obj >> LOG2_ADDR_PER_WORD);
#endif

.CONST $cvc_send.stream.refin          MK1 * 0;
.CONST $cvc_send.stream.sndin_left     MK1 * 1;
.CONST $cvc_send.stream.sndin_right    MK1 * 2;
.CONST $cvc_send.stream.sndin_mic3     MK1 * 3;
.CONST $cvc_send.stream.sndin_mic4     MK1 * 4;
.CONST $cvc_send.stream.sndout         MK1 * 5;

.CONST $FDFBC_TWIDDLE_TABLE_SIZE_NB           128;
.CONST $FDFBC_TWIDDLE_TABLE_SIZE_WB           256;

#endif // _CVC_SEND_DATA_ASM_H
