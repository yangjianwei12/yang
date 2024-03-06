// *****************************************************************************
// Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
// %%version
//
// $Change$  $DateTime$
// *****************************************************************************

#include "stack.h"
#include "portability_macros.h"
#include "patch_library.h"
#include "cvc_send_data.h"

#if !defined(CAPABILITY_DOWNLOAD_BUILD)
#define $M.CVC_SEND_CAP.CAP_ID_HS_2MIC_MONO_WB_WAKEON       $M.GEN.CVC_SEND_2M_HSE_VA_WAKEON_WB_CAP_ID;

#define $M.CVC_SEND_CAP.auto_data_1mic.dyn_table_main       $M.CVC_SEND_CAP.auto_data_1mic.DynTable_Main
#define $M.CVC_SEND_CAP.auto_data_2mic.dyn_table_main       $M.CVC_SEND_CAP.auto_data_2mic.DynTable_Main
#define $M.CVC_SEND_CAP.headset_data_1mic.dyn_table_main    $M.CVC_SEND_CAP.headset_data_1mic.DynTable_Main
#define $M.CVC_SEND_CAP.headset_data_2mic.dyn_table_main    $M.CVC_SEND_CAP.headset_data_2mic.DynTable_Main
#define $M.CVC_SEND_CAP.headset_2mic_data_va_wakeon.dyn_table_main     $M.CVC_SEND_CAP.headset_2mic_data_va_wakeon.DynTable_Main
#define $M.CVC_SEND_CAP.headset_2mic_data_va_bargein.dyn_table_main    $M.CVC_SEND_CAP.headset_2mic_data_va_bargein.DynTable_Main
#define $M.CVC_SEND_CAP.speaker_data_1mic.dyn_table_main    $M.CVC_SEND_CAP.speaker_data_1mic.DynTable_Main
#define $M.CVC_SEND_CAP.speaker_data_2mic.dyn_table_main    $M.CVC_SEND_CAP.speaker_data_2mic.DynTable_Main
#define $M.CVC_SEND_CAP.speaker_data_3mic.dyn_table_main    $M.CVC_SEND_CAP.speaker_data_3mic.DynTable_Main
#define $M.CVC_SEND_CAP.speaker_data_4mic.dyn_table_main    $M.CVC_SEND_CAP.speaker_data_4mic.DynTable_Main

#define $M.CVC_SEND_CAP.auto_data_1mic.dyn_table_linker     $M.CVC_SEND_CAP.auto_data_1mic.DynTable_Linker
#define $M.CVC_SEND_CAP.auto_data_2mic.dyn_table_linker     $M.CVC_SEND_CAP.auto_data_2mic.DynTable_Linker
#define $M.CVC_SEND_CAP.headset_data_1mic.dyn_table_linker  $M.CVC_SEND_CAP.headset_data_1mic.DynTable_Linker
#define $M.CVC_SEND_CAP.headset_data_2mic.dyn_table_linker  $M.CVC_SEND_CAP.headset_data_2mic.DynTable_Linker
#define $M.CVC_SEND_CAP.headset_2mic_data_va_wakeon.dyn_table_linker $M.CVC_SEND_CAP.headset_2mic_data_va_wakeon.DynTable_Linker
#define $M.CVC_SEND_CAP.headset_2mic_data_va_bargein.dyn_table_linker $M.CVC_SEND_CAP.headset_2mic_data_va_bargein.DynTable_Linker
#define $M.CVC_SEND_CAP.speaker_data_1mic.dyn_table_linker  $M.CVC_SEND_CAP.speaker_data_1mic.DynTable_Linker
#define $M.CVC_SEND_CAP.speaker_data_2mic.dyn_table_linker  $M.CVC_SEND_CAP.speaker_data_2mic.DynTable_Linker
#define $M.CVC_SEND_CAP.speaker_data_3mic.dyn_table_linker  $M.CVC_SEND_CAP.speaker_data_3mic.DynTable_Linker
#define $M.CVC_SEND_CAP.speaker_data_4mic.dyn_table_linker  $M.CVC_SEND_CAP.speaker_data_4mic.DynTable_Linker

#define $M.CVC_SEND_CAP.speaker_circ_data_3mic.dyn_table_main        $M.CVC_SEND_CAP.speaker_circ_data_3mic.DynTable_Main
#define $M.CVC_SEND_CAP.speaker_circ_data_3mic_va.dyn_table_main     $M.CVC_SEND_CAP.speaker_circ_data_3mic_va.DynTable_Main
#define $M.CVC_SEND_CAP.speaker_circ_data_3mic_va4b.dyn_table_main   $M.CVC_SEND_CAP.speaker_circ_data_3mic_va4b.DynTable_Main

#define $M.CVC_SEND_CAP.speaker_circ_data_3mic.dyn_table_linker      $M.CVC_SEND_CAP.speaker_circ_data_3mic.DynTable_Linker
#define $M.CVC_SEND_CAP.speaker_circ_data_3mic_va.dyn_table_linker   $M.CVC_SEND_CAP.speaker_circ_data_3mic_va.DynTable_Linker
#define $M.CVC_SEND_CAP.speaker_circ_data_3mic_va4b.dyn_table_linker $M.CVC_SEND_CAP.speaker_circ_data_3mic_va4b.DynTable_Linker

#define $M.CVC_SEND_CAP.headset_data_1mic_va.dyn_table_main          $M.CVC_SEND_CAP.headset_data_1mic_va.DynTable_Main
#define $M.CVC_SEND_CAP.headset_data_1mic_va.dyn_table_linker        $M.CVC_SEND_CAP.headset_data_1mic_va.DynTable_Linker

#define $M.CVC_SEND_CAP.earbud_2mic.dyn_table_main          $M.CVC_SEND_CAP.earbud_2mic.DynTable_Main
#define $M.CVC_SEND_CAP.earbud_2mic.dyn_table_linker        $M.CVC_SEND_CAP.earbud_2mic.DynTable_Linker
#define $M.CVC_SEND_CAP.earbud_3mic.dyn_table_main          $M.CVC_SEND_CAP.earbud_3mic.DynTable_Main
#define $M.CVC_SEND_CAP.earbud_3mic.dyn_table_linker        $M.CVC_SEND_CAP.earbud_3mic.DynTable_Linker

#define $M.CVC_SEND_CAP.earbud_3mic_hybrid.dyn_table_main   $M.CVC_SEND_CAP.earbud_3mic_hybrid.DynTable_Main
#define $M.CVC_SEND_CAP.earbud_3mic_hybrid.dyn_table_linker $M.CVC_SEND_CAP.earbud_3mic_hybrid.DynTable_Linker

#define $M.CVC_SEND_CAP.headset_data_2mic_hybrid.dyn_table_main    $M.CVC_SEND_CAP.headset_data_2mic_hybrid.DynTable_Main
#define $M.CVC_SEND_CAP.headset_data_2mic_hybrid.dyn_table_linker  $M.CVC_SEND_CAP.headset_data_2mic_hybrid.DynTable_Linker

#define $M.CVC_SEND_CAP.headset_data_1mic_hybrid.dyn_table_main    $M.CVC_SEND_CAP.headset_data_1mic_hybrid.DynTable_Main
#define $M.CVC_SEND_CAP.headset_data_1mic_hybrid.dyn_table_linker  $M.CVC_SEND_CAP.headset_data_1mic_hybrid.DynTable_Linker

#else // defined(CAPABILITY_DOWNLOAD_BUILD)
#define $M.CVC_SEND_CAP.CAP_ID_HS_2MIC_MONO_WB_WAKEON       $M.GEN.CVC_SEND_2M_HSE_VA_WAKEON_WB_ALT_CAP_ID_0;

#define $M.CVC_SEND_CAP.auto_data_1mic.dyn_table_main       $M.CVC_SEND_CAP.auto_data_1mic.Downloadable.DynTable_Main
#define $M.CVC_SEND_CAP.auto_data_2mic.dyn_table_main       $M.CVC_SEND_CAP.auto_data_2mic.Downloadable.DynTable_Main
#define $M.CVC_SEND_CAP.headset_data_1mic.dyn_table_main    $M.CVC_SEND_CAP.headset_data_1mic.Downloadable.DynTable_Main
#define $M.CVC_SEND_CAP.headset_data_2mic.dyn_table_main    $M.CVC_SEND_CAP.headset_data_2mic.Downloadable.DynTable_Main
#define $M.CVC_SEND_CAP.headset_2mic_data_va_wakeon.dyn_table_main     $M.CVC_SEND_CAP.headset_2mic_data_va_wakeon.Downloadable.DynTable_Main
#define $M.CVC_SEND_CAP.headset_2mic_data_va_bargein.dyn_table_main    $M.CVC_SEND_CAP.headset_2mic_data_va_bargein.Downloadable.DynTable_Main
#define $M.CVC_SEND_CAP.speaker_data_1mic.dyn_table_main    $M.CVC_SEND_CAP.speaker_data_1mic.Downloadable.DynTable_Main
#define $M.CVC_SEND_CAP.speaker_data_2mic.dyn_table_main    $M.CVC_SEND_CAP.speaker_data_2mic.Downloadable.DynTable_Main
#define $M.CVC_SEND_CAP.speaker_data_3mic.dyn_table_main    $M.CVC_SEND_CAP.speaker_data_3mic.Downloadable.DynTable_Main
#define $M.CVC_SEND_CAP.speaker_data_4mic.dyn_table_main    $M.CVC_SEND_CAP.speaker_data_4mic.Downloadable.DynTable_Main

#define $M.CVC_SEND_CAP.auto_data_1mic.dyn_table_linker     $M.CVC_SEND_CAP.auto_data_1mic.Downloadable.DynTable_Linker
#define $M.CVC_SEND_CAP.auto_data_2mic.dyn_table_linker     $M.CVC_SEND_CAP.auto_data_2mic.Downloadable.DynTable_Linker
#define $M.CVC_SEND_CAP.headset_data_1mic.dyn_table_linker  $M.CVC_SEND_CAP.headset_data_1mic.Downloadable.DynTable_Linker
#define $M.CVC_SEND_CAP.headset_data_2mic.dyn_table_linker  $M.CVC_SEND_CAP.headset_data_2mic.Downloadable.DynTable_Linker
#define $M.CVC_SEND_CAP.headset_2mic_data_va_wakeon.dyn_table_linker $M.CVC_SEND_CAP.headset_2mic_data_va_wakeon.Downloadable.DynTable_Linker
#define $M.CVC_SEND_CAP.headset_2mic_data_va_bargein.dyn_table_linker $M.CVC_SEND_CAP.headset_2mic_data_va_bargein.Downloadable.DynTable_Linker
#define $M.CVC_SEND_CAP.speaker_data_1mic.dyn_table_linker  $M.CVC_SEND_CAP.speaker_data_1mic.Downloadable.DynTable_Linker
#define $M.CVC_SEND_CAP.speaker_data_2mic.dyn_table_linker  $M.CVC_SEND_CAP.speaker_data_2mic.Downloadable.DynTable_Linker
#define $M.CVC_SEND_CAP.speaker_data_3mic.dyn_table_linker  $M.CVC_SEND_CAP.speaker_data_3mic.Downloadable.DynTable_Linker
#define $M.CVC_SEND_CAP.speaker_data_4mic.dyn_table_linker  $M.CVC_SEND_CAP.speaker_data_4mic.Downloadable.DynTable_Linker

#define $M.CVC_SEND_CAP.speaker_circ_data_3mic.dyn_table_main        $M.CVC_SEND_CAP.speaker_circ_data_3mic.Downloadable.DynTable_Main
#define $M.CVC_SEND_CAP.speaker_circ_data_3mic_va.dyn_table_main     $M.CVC_SEND_CAP.speaker_circ_data_3mic_va.Downloadable.DynTable_Main
#define $M.CVC_SEND_CAP.speaker_circ_data_3mic_va4b.dyn_table_main   $M.CVC_SEND_CAP.speaker_circ_data_3mic_va4b.Downloadable.DynTable_Main

#define $M.CVC_SEND_CAP.speaker_circ_data_3mic.dyn_table_linker      $M.CVC_SEND_CAP.speaker_circ_data_3mic.Downloadable.DynTable_Linker
#define $M.CVC_SEND_CAP.speaker_circ_data_3mic_va.dyn_table_linker   $M.CVC_SEND_CAP.speaker_circ_data_3mic_va.Downloadable.DynTable_Linker
#define $M.CVC_SEND_CAP.speaker_circ_data_3mic_va4b.dyn_table_linker $M.CVC_SEND_CAP.speaker_circ_data_3mic_va4b.Downloadable.DynTable_Linker

#define $M.CVC_SEND_CAP.headset_data_1mic_va.dyn_table_main          $M.CVC_SEND_CAP.headset_data_1mic_va.Downloadable.DynTable_Main
#define $M.CVC_SEND_CAP.headset_data_1mic_va.dyn_table_linker        $M.CVC_SEND_CAP.headset_data_1mic_va.Downloadable.DynTable_Linker

#define $M.CVC_SEND_CAP.earbud_2mic.dyn_table_main          $M.CVC_SEND_CAP.earbud_2mic.Downloadable.DynTable_Main
#define $M.CVC_SEND_CAP.earbud_2mic.dyn_table_linker        $M.CVC_SEND_CAP.earbud_2mic.Downloadable.DynTable_Linker
#define $M.CVC_SEND_CAP.earbud_3mic.dyn_table_main          $M.CVC_SEND_CAP.earbud_3mic.Downloadable.DynTable_Main
#define $M.CVC_SEND_CAP.earbud_3mic.dyn_table_linker        $M.CVC_SEND_CAP.earbud_3mic.Downloadable.DynTable_Linker

#define $M.CVC_SEND_CAP.earbud_3mic_hybrid.dyn_table_main   $M.CVC_SEND_CAP.earbud_3mic_hybrid.Downloadable.DynTable_Main
#define $M.CVC_SEND_CAP.earbud_3mic_hybrid.dyn_table_linker $M.CVC_SEND_CAP.earbud_3mic_hybrid.Downloadable.DynTable_Linker

#define $M.CVC_SEND_CAP.headset_data_2mic_hybrid.dyn_table_main   $M.CVC_SEND_CAP.headset_data_2mic_hybrid.Downloadable.DynTable_Main
#define $M.CVC_SEND_CAP.headset_data_2mic_hybrid.dyn_table_linker $M.CVC_SEND_CAP.headset_data_2mic_hybrid.Downloadable.DynTable_Linker

#define $M.CVC_SEND_CAP.headset_data_1mic_hybrid.dyn_table_main   $M.CVC_SEND_CAP.headset_data_1mic_hybrid.Downloadable.DynTable_Main
#define $M.CVC_SEND_CAP.headset_data_1mic_hybrid.dyn_table_linker $M.CVC_SEND_CAP.headset_data_1mic_hybrid.Downloadable.DynTable_Linker

#endif // defined(CAPABILITY_DOWNLOAD_BUILD)



// *****************************************************************************
// MODULE:
//    void CVC_SEND_CAP_Config_auto_1mic(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
//    void CVC_SEND_CAP_Config_auto_2mic(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
//    void CVC_SEND_CAP_Config_headset_1mic(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
//    void CVC_SEND_CAP_Config_headset_1mic_va(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
//    void CVC_SEND_CAP_Config_headset_2mic_mono(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
//    void CVC_SEND_CAP_Config_headset_2mic_wakeon(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
//    void CVC_SEND_CAP_Config_headset_2mic_bargein(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
//    void CVC_SEND_CAP_Config_headset_2mic_binaural(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
//    void CVC_SEND_CAP_Config_speaker_1mic(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
//    void CVC_SEND_CAP_Config_speaker_2mic(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
//    void CVC_SEND_CAP_Config_speaker_3mic(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
//    void CVC_SEND_CAP_Config_speaker_4mic(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
//    void CVC_SEND_CAP_Config_spkr_circ_3mic_va(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
//    void CVC_SEND_CAP_Config_spkr_circ_3mic_va4b(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
//    void CVC_SEND_CAP_Config_earbud_2mic_ie(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
//    void CVC_SEND_CAP_Config_earbud_3mic_ie(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
//    extern void CVC_SEND_CAP_Config_earbud_3mic_hybrid(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
//    extern void CVC_SEND_CAP_Config_headset_2mic_mono_hybrid(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
//    extern void CVC_SEND_CAP_Config_headset_1mic_hybrid(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
//
// DESCRIPTION:
//    CVC SEND Configuration (per capability) (C-callable)
//
// MODIFICATIONS:
//
// INPUTS:
//    r0 - Extended Data
//    r1 - data_variant
//
// OUTPUTS:
//    - cVc send configuration data
//
// TRASHED REGISTERS:
//
// CPU USAGE:
// *****************************************************************************
#define $cvc_send.mic_config.ONE_MIC               MIC_CONFIG_DEFAULT
#define $cvc_send.mic_config.AUTO_2MIC             MIC_CONFIG_DEFAULT
#define $cvc_send.mic_config.HEADSET_MONO          MIC_CONFIG_ENDFIRE
#define $cvc_send.mic_config.HEADSET_BINAURAL      MIC_CONFIG_BROADSIDE
#define $cvc_send.mic_config.SPEAKER_2MIC          MIC_CONFIG_BROADSIDE + MIC_CONFIG_ENDFIRE
#define $cvc_send.mic_config.SPEAKER_MULTI_MIC     MIC_CONFIG_BROADSIDE
#define $cvc_send.mic_config.SPEAKER_CIRCULAR_MIC  MIC_CONFIG_CIRCULAR

.MODULE $M.CVC_SEND_CAP.config.auto_1mic;
   .CODESEGMENT PM;

$_CVC_SEND_CAP_Config_auto_1mic:
   r2 = $cvc_send.mic_config.ONE_MIC;
   r3 = $cvc_send.AUTO;
   I3 = $M.CVC_SEND_CAP.auto_data_1mic.dyn_table_main;
   I7 = $M.CVC_SEND_CAP.auto_data_1mic.dyn_table_linker;
   r10 = 1;
   jump $cvc_send.dyn_config;
.ENDMODULE;


.MODULE $M.CVC_SEND_CAP.config.auto_2mic;
   .CODESEGMENT PM;

$_CVC_SEND_CAP_Config_auto_2mic:
   r2 = $cvc_send.mic_config.AUTO_2MIC;
   r3 = $cvc_send.AUTO;
   I3 = $M.CVC_SEND_CAP.auto_data_2mic.dyn_table_main;
   I7 = $M.CVC_SEND_CAP.auto_data_2mic.dyn_table_linker;
   r10 = 2;
   jump $cvc_send.dyn_config;
.ENDMODULE;


.MODULE $M.CVC_SEND_CAP.config.headset_1mic;
   .CODESEGMENT PM;

$_CVC_SEND_CAP_Config_headset_1mic:
   r2 = $cvc_send.mic_config.ONE_MIC;
   r3 = $cvc_send.HEADSET;
   I3 = $M.CVC_SEND_CAP.headset_data_1mic.dyn_table_main;
   I7 = $M.CVC_SEND_CAP.headset_data_1mic.dyn_table_linker;
   r10 = 1;
   jump $cvc_send.dyn_config;
.ENDMODULE;


.MODULE $M.CVC_SEND_CAP.config.headset_1mic_va;
   .CODESEGMENT PM;

$_CVC_SEND_CAP_Config_headset_1mic_va:
   r2 = $cvc_send.mic_config.ONE_MIC;
   r3 = $cvc_send.HEADSET;
   I3 = $M.CVC_SEND_CAP.headset_data_1mic_va.dyn_table_main;
   I7 = $M.CVC_SEND_CAP.headset_data_1mic_va.dyn_table_linker;
   r10 = 1;
   jump $cvc_send.dyn_config;
.ENDMODULE;


.MODULE $M.CVC_SEND_CAP.config.headset_2mic;
   .CODESEGMENT PM;

$_CVC_SEND_CAP_Config_headset_2mic_binaural:
   r2 = $cvc_send.mic_config.HEADSET_BINAURAL;
   jump Config_headset_2mic;

$_CVC_SEND_CAP_Config_headset_2mic_mono:
   M[r0 + $cvc_send.cap.ASF_WNR_AVAILABLE] = r0;
   r2 = $cvc_send.mic_config.HEADSET_MONO;

Config_headset_2mic:
   r3 = $cvc_send.HEADSET;
   I3 = $M.CVC_SEND_CAP.headset_data_2mic.dyn_table_main;
   I7 = $M.CVC_SEND_CAP.headset_data_2mic.dyn_table_linker;
   r10 = 2;
   jump $cvc_send.dyn_config;
.ENDMODULE;


.MODULE $M.CVC_SEND_CAP.config.speaker_1mic;
   .CODESEGMENT PM;

$_CVC_SEND_CAP_Config_speaker_1mic:
   r2 = $cvc_send.mic_config.ONE_MIC;
   r3 = $cvc_send.SPEAKER;
   I3 = $M.CVC_SEND_CAP.speaker_data_1mic.dyn_table_main;
   I7 = $M.CVC_SEND_CAP.speaker_data_1mic.dyn_table_linker;
   r10 = 1;
   jump $cvc_send.dyn_config;
.ENDMODULE;


.MODULE $M.CVC_SEND_CAP.config.speaker_2mic;
   .CODESEGMENT PM;

$_CVC_SEND_CAP_Config_speaker_2mic:
   r2 = $cvc_send.mic_config.SPEAKER_2MIC;
   r3 = $cvc_send.SPEAKER;
   I3 = $M.CVC_SEND_CAP.speaker_data_2mic.dyn_table_main;
   I7 = $M.CVC_SEND_CAP.speaker_data_2mic.dyn_table_linker;
   r10 = 2;
   jump $cvc_send.dyn_config;
.ENDMODULE;

.MODULE $M.CVC_SEND_CAP.config.speaker_3mic;
   .CODESEGMENT PM;

$_CVC_SEND_CAP_Config_speaker_3mic:
   r2 = $cvc_send.mic_config.SPEAKER_MULTI_MIC;
   r3 = $cvc_send.SPEAKER;
   I3 = $M.CVC_SEND_CAP.speaker_data_3mic.dyn_table_main;
   I7 = $M.CVC_SEND_CAP.speaker_data_3mic.dyn_table_linker;
   r10 = 3;
   jump $cvc_send.dyn_config;
.ENDMODULE;

.MODULE $M.CVC_SEND_CAP.config.speaker_4mic;
   .CODESEGMENT PM;

$_CVC_SEND_CAP_Config_speaker_4mic:
   r2 = $cvc_send.mic_config.SPEAKER_MULTI_MIC;
   r3 = $cvc_send.SPEAKER;
   I3 = $M.CVC_SEND_CAP.speaker_data_4mic.dyn_table_main;
   I7 = $M.CVC_SEND_CAP.speaker_data_4mic.dyn_table_linker;
   r10 = 4;
   jump $cvc_send.dyn_config;
.ENDMODULE;

.MODULE $M.CVC_SEND_CAP_Config_spkr_circ_3mic;
   .CODESEGMENT PM;
$_CVC_SEND_CAP_Config_spkr_circ_3mic:
   r2 = $cvc_send.mic_config.SPEAKER_CIRCULAR_MIC;
   r3 = $cvc_send.SPEAKER;
   I3 = $M.CVC_SEND_CAP.speaker_circ_data_3mic.dyn_table_main;
   I7 = $M.CVC_SEND_CAP.speaker_circ_data_3mic.dyn_table_linker;
   r10 = 3;
   jump $cvc_send.dyn_config;
.ENDMODULE;

.MODULE $M.CVC_SEND_CAP_Config_spkr_circ_3mic_va1b;
   .CODESEGMENT PM;
$_CVC_SEND_CAP_Config_spkr_circ_3mic_va:
   r2 = 4;
   M[r0 + $cvc_send.cap.NUM_VA_OUTPUTS] = r2;
   r2 = $cvc_send.mic_config.SPEAKER_CIRCULAR_MIC;
   r3 = $cvc_send.SPEAKER;
   I3 = $M.CVC_SEND_CAP.speaker_circ_data_3mic_va.dyn_table_main;
   I7 = $M.CVC_SEND_CAP.speaker_circ_data_3mic_va.dyn_table_linker;
   r10 = 3;
   jump $cvc_send.dyn_config;
.ENDMODULE;

.MODULE $M.CVC_SEND_CAP_Config_spkr_circ_3mic_va4b;
   .CODESEGMENT PM;
$_CVC_SEND_CAP_Config_spkr_circ_3mic_va4b:
   r2 = 4;
   M[r0 + $cvc_send.cap.NUM_VA_OUTPUTS] = r2;
   r2 = $cvc_send.mic_config.SPEAKER_CIRCULAR_MIC;
   r3 = $cvc_send.SPEAKER;
   I3 = $M.CVC_SEND_CAP.speaker_circ_data_3mic_va4b.dyn_table_main;
   I7 = $M.CVC_SEND_CAP.speaker_circ_data_3mic_va4b.dyn_table_linker;
   r10 = 3;
   jump $cvc_send.dyn_config;
.ENDMODULE;

.MODULE $M.CVC_SEND_CAP_Config_headset_2mic_wakeon;
   .CODESEGMENT PM;
$_CVC_SEND_CAP_Config_headset_2mic_wakeon:
   r2 = $cvc_send.mic_config.HEADSET_MONO;
   r3 = $cvc_send.HEADSET;
   I3 = $M.CVC_SEND_CAP.headset_2mic_data_va_wakeon.dyn_table_main;
   I7 = $M.CVC_SEND_CAP.headset_2mic_data_va_wakeon.dyn_table_linker;
   r10 = 2;
   jump $cvc_send.dyn_config;
.ENDMODULE;

.MODULE $M.CVC_SEND_CAP_Config_headset_2mic_bargein;
   .CODESEGMENT PM;
$_CVC_SEND_CAP_Config_headset_2mic_bargein:
   r2 = $cvc_send.mic_config.HEADSET_MONO;
   r3 = $cvc_send.HEADSET;
   I3 = $M.CVC_SEND_CAP.headset_2mic_data_va_bargein.dyn_table_main;
   I7 = $M.CVC_SEND_CAP.headset_2mic_data_va_bargein.dyn_table_linker;
   r10 = 2;
   jump $cvc_send.dyn_config;
.ENDMODULE;

.MODULE $M.CVC_SEND_CAP_Config_earbud_2mic_ie;
   .CODESEGMENT PM;
$_CVC_SEND_CAP_Config_earbud_2mic_ie:
   I3 = $M.CVC_SEND_CAP.earbud_2mic.dyn_table_main;
   I7 = $M.CVC_SEND_CAP.earbud_2mic.dyn_table_linker;
   r10 = 2;
   r3 = $cvc_send.HEADSET;
   r2 = $cvc_send.mic_config.ONE_MIC;
   r2 = r2 + MIC_CONFIG_INEAR;
   jump $cvc_send.dyn_config;
.ENDMODULE;

.MODULE $M.CVC_SEND_CAP_Config_earbud_3mic_ie;
   .CODESEGMENT PM;
$_CVC_SEND_CAP_Config_earbud_3mic_ie:
   M[r0 + $cvc_send.cap.ASF_WNR_AVAILABLE] = r0;
   r2 = $cvc_send.mic_config.HEADSET_MONO;
   r2 = r2 + MIC_CONFIG_INEAR;
   r3 = $cvc_send.HEADSET;
   I3 = $M.CVC_SEND_CAP.earbud_3mic.dyn_table_main;
   I7 = $M.CVC_SEND_CAP.earbud_3mic.dyn_table_linker;
   r10 = 3;
   jump $cvc_send.dyn_config;
.ENDMODULE;

.MODULE $M.CVC_SEND_CAP_Config_earbud_3mic_hybrid;
   .CODESEGMENT PM;
$_CVC_SEND_CAP_Config_earbud_3mic_hybrid:
   M[r0 + $cvc_send.cap.ASF_WNR_AVAILABLE] = r0;
   r2 = $cvc_send.mic_config.HEADSET_MONO;
   r2 = r2 + (MIC_CONFIG_INEAR + MIC_CONFIG_AI);
   r3 = $cvc_send.HEADSET;
   I3 = $M.CVC_SEND_CAP.earbud_3mic_hybrid.dyn_table_main;
   I7 = $M.CVC_SEND_CAP.earbud_3mic_hybrid.dyn_table_linker;
   r10 = 3;
   jump $cvc_send.dyn_config;
.ENDMODULE;

.MODULE $M.CVC_SEND_CAP.config.headset_2mic_mono_hybrid;
   .CODESEGMENT PM;
$_CVC_SEND_CAP_Config_headset_2mic_mono_hybrid:
   M[r0 + $cvc_send.cap.ASF_WNR_AVAILABLE] = r0;
   r2 = $cvc_send.mic_config.HEADSET_MONO;
   r2 = r2 + MIC_CONFIG_AI;
   r3 = $cvc_send.HEADSET;
   I3 = $M.CVC_SEND_CAP.headset_data_2mic_hybrid.dyn_table_main;
   I7 = $M.CVC_SEND_CAP.headset_data_2mic_hybrid.dyn_table_linker;
   r10 = 2;
   jump $cvc_send.dyn_config;
.ENDMODULE;

.MODULE $M.CVC_SEND_CAP.config.headset_1mic_hybrid;
   .CODESEGMENT PM;
$_CVC_SEND_CAP_Config_headset_1mic_hybrid:
   r2 = $cvc_send.mic_config.ONE_MIC;
   r2 = r2 + MIC_CONFIG_AI;
   r3 = $cvc_send.HEADSET;
   I3 = $M.CVC_SEND_CAP.headset_data_1mic_hybrid.dyn_table_main;
   I7 = $M.CVC_SEND_CAP.headset_data_1mic_hybrid.dyn_table_linker;
   r10 = 1;
   jump $cvc_send.dyn_config;
.ENDMODULE;


// *****************************************************************************
// MODULE:
//    $cvc_send.dyn_config
//
// DESCRIPTION:
//    Dynanic memory configuration
//
// MODIFICATIONS:
//
// INPUTS:
//    r0 - Extended Data
//    r1 - data_variant
//    r2 - mic_config
//    r3 - major_config
//   r10 - num_mics
//    I3 - dyn_table_main
//    I7 - dyn_table_linker
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//
// CPU USAGE:
// *****************************************************************************
.MODULE $M.CVC_SEND_CAP.dyn_config;
   .CODESEGMENT PM;

$cvc_send.dyn_config:
   M[r0 + $cvc_send.cap.NUM_MICS] = r10;
   M[r0 + $cvc_send.cap.DATA_VARIANT] = r1;
   M[r0 + $cvc_send.cap.MIC_CONFIG] = r2;
   M[r0 + $cvc_send.cap.MAJOR_CONFIG] = r3;
   r2 = I3;
   r3 = I7;
   M[r0 + $cvc_send.cap.DYN_MAIN]   = r2;
   M[r0 + $cvc_send.cap.DYN_LINKER] = r3;
   
   // The standard (legacy) CVCs do not set the NUM_OUTPUTS field 
   // in their configuration functions, while the VA4B does.
   // So we need to set this field to 1 for the standard(legacy) CVCs here.
   Null = M[r0 + $cvc_send.cap.NUM_VA_OUTPUTS];
   if NZ rts;

#if !defined(CVC_VA_DISABLE)
   r1 = 1;
#else
   r1 = 0;
#endif
   M[r0 + $cvc_send.cap.NUM_VA_OUTPUTS] = r1;
   rts;

.ENDMODULE;


// *****************************************************************************
// MODULE:
//    unsigned CVC_SEND_CAP_Create(CVC_SEND_OP_DATA *op_extra_data);
//
// DESCRIPTION:
//
// MODIFICATIONS:
//
// INPUTS:
//    - r0 - Extended Data
//
// OUTPUTS:
//    - r0 - status
//
// TRASHED REGISTERS:
//
// CPU USAGE:
// *****************************************************************************
.MODULE $M.CVC_SEND_CAP.Create;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

$_CVC_SEND_CAP_Create:
   push rLink;
   pushm <r8,r9>;

   r8 = r0; // Extended Data

   LIBS_SLOW_SW_ROM_PATCH_POINT($CVC_SEND_CAP_CREATE.PATCH_ID_0, r1)

   //
   //  Reserve Scratch Data
   //
   r0 = r8;
   r1 = M[r8 + $cvc_send.cap.DYN_MAIN];
   r2 = M[r8 + $cvc_send.cap.DATA_VARIANT];
   call $_DynLoaderScratchReserve;
   NULL = r0;
   if Z CALL_PANIC_OR_JUMP(create_failed);

   //
   //  Allocate Shared Variables
   //

   // CVCLIB table
   r0 = r8 + $cvc_send.cap.CVCLIB_TABLE;
   r1 = M[r8 + $cvc_send.cap.DATA_VARIANT];
   call $_cvclib_table_alloc;
   r0 = M[r8 + $cvc_send.cap.CVCLIB_TABLE];
   if Z CALL_PANIC_OR_JUMP(create_failed);

   // Hybrid config
   r0 = M[r8 + $cvc_send.cap.MIC_CONFIG];
   Null = r0 AND (MIC_CONFIG_AI);
   if Z jump end_hybrid_config;
      // High Resolution filter_bank configuration table
      r0 = r8 + $cvc_send.cap.HR_FILTERBANK_CONFIG;
      r1 = M[r8 + $cvc_send.cap.DATA_VARIANT];
      call $_dyn_load_filter_bank_config_high_res;
      r0 = M[r8 + $cvc_send.cap.HR_FILTERBANK_CONFIG];
      if Z CALL_PANIC_OR_JUMP(create_failed);

      // High Resolution filter_bank split table
      r1 = M[r8 + $cvc_send.cap.DATA_VARIANT];
      Null = r1 - DATA_VARIANT_SWB;
      if LT jump end_hybrid_config;

      r0 = r8 + $cvc_send.cap.FFTSPLIT_TABLE;
      call $_dyn_load_filter_bank_split_fft1024;
      r0 = M[r8 + $cvc_send.cap.FFTSPLIT_TABLE];
      if Z CALL_PANIC_OR_JUMP(create_failed);
   end_hybrid_config:

   // filter_bank split table
   r0 = r8 + $cvc_send.cap.FFTSPLIT_TABLE;
   Null = M[r0];
   if Z call $_dyn_load_filter_bank_split;
   r0 = M[r8 + $cvc_send.cap.FFTSPLIT_TABLE];
   if Z CALL_PANIC_OR_JUMP(create_failed);

   // filter_bank configuration table
   //    headset      - Hanning window
   //    auto/speaker - Custom window
   r2 = M[r8 + $cvc_send.cap.MAJOR_CONFIG];
   r2 = r2 - $cvc_send.HEADSET;
   r1 = M[r8 + $cvc_send.cap.DATA_VARIANT];
   r0 = r8 + $cvc_send.cap.FILTERBANK_CONFIG;
   call $_dyn_load_filter_bank_config;
   r0 = M[r8 + $cvc_send.cap.FILTERBANK_CONFIG];
   if Z CALL_PANIC_OR_JUMP(create_failed);

   // OMS_IN
   r0 = r8  + $cvc_send.cap.OMS_CONST;
   r1 = M[r8 + $cvc_send.cap.DATA_VARIANT];
   call $_oms280_AllocMem;
   r0 = M[r8 + $cvc_send.cap.OMS_CONST];
   if Z CALL_PANIC_OR_JUMP(create_failed);

   // SEND NS
   // speaker sound bar: linear, others: PBP
   // r2 -> PBP(0)/Linear(1) flag
   r2 = 1;
   r0 = M[r8 + $cvc_send.cap.MAJOR_CONFIG];
   Null = r0 - $cvc_send.SPEAKER;
   if NZ r2 = 0;
   r0 = M[r8 + $cvc_send.cap.NUM_MICS];
   NULL = r0 - 1;
   if Z r2 = 0;
   r0 = M[r8 + $cvc_send.cap.MIC_CONFIG];
   Null = r0 AND (MIC_CONFIG_BROADSIDE);
   if Z r2 = 0;
   r1 = M[r8 + $cvc_send.cap.DATA_VARIANT];
   r0 = r8 + $cvc_send.cap.DMS200_MODE;
   call $_dms200_AllocMem;
   r0 = M[r8 + $cvc_send.cap.DMS200_MODE];
   if Z CALL_PANIC_OR_JUMP(create_failed);

   // ASF: not available for 1mic, and speaker-3mic-circ-va
   r0 = M[r8 + $cvc_send.cap.NUM_MICS];
   NULL = r0 - 1;
   if Z jump jp_no_asf_share;
      r0 = M[r8 + $cvc_send.cap.MIC_CONFIG];
      Null = r0 AND (MIC_CONFIG_CIRCULAR);
      if NZ jump jp_no_asf_share;
      r0 = r8 + $cvc_send.cap.ASF_MODE_TABLE;
      r1 = M[r8 + $cvc_send.cap.DATA_VARIANT];
      call $_asf100_AllocMem;
      r0 = M[r8 + $cvc_send.cap.ASF_MODE_TABLE];
      if Z CALL_PANIC_OR_JUMP(create_failed);

      //load wnr mode table
      Null = M[r8 + $cvc_send.cap.ASF_WNR_AVAILABLE];
      if Z jump jp_no_asf_share;
      
      r0 = r8 + $cvc_send.cap.WNR_MODE_TABLE;
      r1 = M[r8 + $cvc_send.cap.DATA_VARIANT];
      call $_asf100_wnr_AllocMem;
      r0 = M[r8 + $cvc_send.cap.WNR_MODE_TABLE];
      if Z CALL_PANIC_OR_JUMP(create_failed);

   jp_no_asf_share:

   // DCBLOCK or REF_VAD
   r0 = r8 + $cvc_send.cap.VAD_DC_COEFFS;
   r1 = M[r8 + $cvc_send.cap.DATA_VARIANT];
   call $_vad410_AllocMem;
   r0 = M[r8 + $cvc_send.cap.VAD_DC_COEFFS];
   if Z CALL_PANIC_OR_JUMP(create_failed);

	// AEC
   r0 = M[r8 + $cvc_send.cap.CAPABILITY_ID];
   Null = r0 - $M.CVC_SEND_CAP.CAP_ID_HS_2MIC_MONO_WB_WAKEON;
   if Z jump jp_no_aec_alloc_mem;
      r0 = r8 + $cvc_send.cap.AEC_MODE;
      r1 = M[r8 + $cvc_send.cap.DATA_VARIANT];
      call $_aec530_AllocMem;
      r0 = M[r8 + $cvc_send.cap.AEC_MODE];
      if Z CALL_PANIC_OR_JUMP(create_failed);
   jp_no_aec_alloc_mem:


   // Set Cap Root
   M[r8 + $cvc_send.cap.CAP_ROOT_PTR_FIELD] = r8;

   // Set MGDC persistent state pointer
   r0 = r8 + $cvc_send.cap.MGDC_STATE_FIELD;
   M[r8 + $cvc_send.cap.MGDC_STATE_PTR_FIELD] = r0;

   //
   //  Allocate Persistent Data
   //
   r0 = r8;
   r1 = M[r8 + $cvc_send.cap.DYN_MAIN];
   r2 = M[r8 + $cvc_send.cap.DYN_LINKER];
   r3 = M[r8 + $cvc_send.cap.DATA_VARIANT];
   call $_DynLoaderProcessDynamicAllocations;
   NULL = r0;
   if Z CALL_PANIC_OR_JUMP(create_failed);

   // internal streams
   r10 = (CVC_SEND_MAX_NUM_INPUT + CVC_SEND_MAX_NUM_OUTPUT - 1);
   r1 = r8 + $cvc_send.cap.PTR_INPUT_STREAM_REF_FIELD;
   r0 = M[r1];
   do stream_create;
      r0 = r0 + ($cvc_stream.frmbuffer_struct.STRUC_SIZE * MK1);
      r1 = r1 + MK1;
      M[r1] = r0;
   stream_create:

#if defined(CVC_USE_FDFBC)
fdfbc_twiddle_alloc_check:   
   r0 = M[r8 + $cvc_send.cap.NUM_MICS];
   Null = r0 - 1;
   if NZ jump create_succeeded;
   r0 = M[r8 + $cvc_send.cap.MAJOR_CONFIG];
   Null = r0 - $cvc_send.SPEAKER;
   if NZ jump create_succeeded;
   r0 = M[r8 + $cvc_send.cap.DATA_VARIANT];
   Null = r0 - DATA_VARIANT_UWB;
   if Z jump create_succeeded;
   Null = r0 - DATA_VARIANT_SWB;
   if Z jump create_succeeded;
   Null = r0 - DATA_VARIANT_FB;
   if Z jump create_succeeded;
      
fdfbc_twiddle_alloc:   
   r0 = $FDFBC_TWIDDLE_TABLE_SIZE_NB;
   r2 = $FDFBC_TWIDDLE_TABLE_SIZE_WB;
   Null = M[r8 + $cvc_send.cap.DATA_VARIANT];
   if NZ r0 = r2;                        
   call $math.fft_twiddle.alloc;
   NULL = r0;
   if Z CALL_PANIC_OR_JUMP(create_failed);
#endif

create_succeeded:
   r0 = 0;                                                         // create succeeded
create_done:
   popm <r8,r9>;
   pop rLink;
   rts;
create_failed:
   r0 = 1;                                                         // create failed
   jump create_done;

.ENDMODULE;


// *****************************************************************************
// MODULE:
//    void CVC_SEND_CAP_Destroy(CVC_SEND_OP_DATA *op_extra_data);
//
// DESCRIPTION:
//
// MODIFICATIONS:
//
// INPUTS:
//    - r0 - Extended Data
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//
// CPU USAGE:
// *****************************************************************************
.MODULE $M.CVC_SEND_CAP.Destroy;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

$_CVC_SEND_CAP_Destroy:
   push rLink;
   push r8;

   r8 = r0;  // extended data

   // unregister component
   call $Security.UnregisterComponent;

   // Release Scratch Data
   r0 = M[r8 + $cvc_send.cap.SCRATCH_ALLOC_PTR_FIELD];
   if NZ call $_DynLoaderDeregisterScratch;
   M[r8 + $cvc_send.cap.SCRATCH_ALLOC_PTR_FIELD]=NULL;

   // Release Shared Variables
   r0 = M[r8 + $cvc_send.cap.CVCLIB_TABLE];
   if NZ call $_cvclib_table_release;
   M[r8 + $cvc_send.cap.CVCLIB_TABLE] = NULL;

   r0 = M[r8 + $cvc_send.cap.ASF_MODE_TABLE];
   if NZ call $_asf100_ReleaseMem;
   M[r8 + $cvc_send.cap.ASF_MODE_TABLE] = NULL;

   r0 = M[r8 + $cvc_send.cap.WNR_MODE_TABLE];
   if NZ call $_asf100_wnr_ReleaseMem;
   M[r8 + $cvc_send.cap.WNR_MODE_TABLE] = NULL;

   r0 = M[r8 + $cvc_send.cap.DMS200_MODE];
   if NZ call $_dms200_ReleaseMem;
   M[r8 + $cvc_send.cap.DMS200_MODE]= NULL;

   r0 = M[r8 + $cvc_send.cap.OMS_CONST];
   if NZ call $_oms280_ReleaseMem;
   M[r8 + $cvc_send.cap.OMS_CONST]=NULL;

   r0 = M[r8 + $cvc_send.cap.VAD_DC_COEFFS];
   if NZ call $_vad410_ReleaseMem;
   M[r8 + $cvc_send.cap.VAD_DC_COEFFS]=NULL;


#if defined(CVC_USE_FDFBC)
fdfbc_twiddle_release_check:   
   r0 = M[r8 + $cvc_send.cap.NUM_MICS];
   Null = r0 - 1;
   if NZ jump fdfdbc_twiddle_release_done;
   r0 = M[r8 + $cvc_send.cap.MAJOR_CONFIG];
   Null = r0 - $cvc_send.SPEAKER;
   if NZ jump fdfdbc_twiddle_release_done;
   r0 = M[r8 + $cvc_send.cap.DATA_VARIANT];
   Null = r0 - DATA_VARIANT_UWB;
   if Z jump fdfdbc_twiddle_release_done;
   Null = r0 - DATA_VARIANT_SWB;
   if Z jump fdfdbc_twiddle_release_done;
   Null = r0 - DATA_VARIANT_FB;
   if Z jump fdfdbc_twiddle_release_done;
   
fdfbc_twiddle_release:   
   r0 = $FDFBC_TWIDDLE_TABLE_SIZE_NB;
   r2 = $FDFBC_TWIDDLE_TABLE_SIZE_WB;
   Null = M[r8 + $cvc_send.cap.DATA_VARIANT];
   if NZ r0 = r2;    
   call $math.fft_twiddle.release;

fdfdbc_twiddle_release_done:
#endif

   r0 = M[r8 + $cvc_send.cap.FFTSPLIT_TABLE];
   if NZ call $_dyn_free_filter_bank_split;
   M[r8 + $cvc_send.cap.FFTSPLIT_TABLE]=NULL;

   r0 = M[r8 + $cvc_send.cap.FILTERBANK_CONFIG];
   if NZ call $_dyn_free_filter_bank_config;
   M[r8 + $cvc_send.cap.FILTERBANK_CONFIG]=NULL;

   r0 = M[r8 + $cvc_send.cap.HR_FILTERBANK_CONFIG];
   if NZ call $_dyn_free_filter_bank_config;
   M[r8 + $cvc_send.cap.HR_FILTERBANK_CONFIG]=NULL;

   r0 = M[r8 + $cvc_send.cap.AEC_MODE];
   if NZ call $_aec530_ReleaseMem;
   M[r8 + $cvc_send.cap.AEC_MODE] = NULL;

   //  deallocate Persistent Data
   r0 = M[r8 + $cvc_send.cap.INST_ALLOC_PTR_FIELD];
   if NZ call $_DynLoaderReleaseDynamicAllocations;
   M[r8 + $cvc_send.cap.INST_ALLOC_PTR_FIELD]=NULL;

   pop r8;
   pop rLink;
   rts;

.ENDMODULE;


// *****************************************************************************
// MODULE:
//    void CVC_SEND_CAP_Initialize(CVC_SEND_OP_DATA *op_extra_data);
//
// DESCRIPTION:
//
// MODIFICATIONS:
//
// INPUTS:
//    - r0 - Extended Data
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//
// CPU USAGE:
// *****************************************************************************
.MODULE $M.CVC_SEND_CAP.Initialize;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

$_CVC_SEND_CAP_Initialize:
   M[r0 + $cvc_send.cap.ALGREINIT]  = Null;
   r1 = M[r0 + $cvc_send.cap.CVC_DATA_ROOT_FIELD];
   r0 = M[r0 + $cvc_send.cap.INIT_TABLE_PTR_FIELD];
   jump $_cvc_run_frame_proc_function_table;

.ENDMODULE;


// *****************************************************************************
// MODULE:
//    void CVC_SEND_CAP_Process(CVC_SEND_OP_DATA *op_extra_data);
//
// DESCRIPTION:
//
// MODIFICATIONS:
//
// INPUTS:
//    - r0 - Extended Data
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//
// CPU USAGE:
// *****************************************************************************
.MODULE $M.CVC_SEND_CAP.Process;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

$_CVC_SEND_CAP_Process:
   push rLink;
   call $_cvc_register_save;  // preseve FP

   push r0;    // Save Root Object. As local variable

   LIBS_SLOW_SW_ROM_PATCH_POINT($CVC_SEND_CAP_PROCESS.PATCH_ID_0, r1)

   r8 = M[SP - 1 * ADDR_PER_WORD];   // extended data
   r0 = M[r8 + $cvc_send.cap.INST_ALLOC_PTR_FIELD];
   r1 = M[r8 + $cvc_send.cap.SCRATCH_ALLOC_PTR_FIELD];
   if NZ call $_DynLoaderScratchCommit;

   r0 = M[SP - 1 * ADDR_PER_WORD];   // extended data
   NULL = M[r0 + $cvc_send.cap.ALGREINIT];
   if NZ call $_CVC_SEND_CAP_Initialize;

   // microphone stream configuration
   // r0 -> frame pointer
   // r2 -> scratch (DM1)
   r8 = M[SP - 1 * ADDR_PER_WORD];   // extended data
   r0 = M[r8 + $cvc_send.cap.PTR_INPUT_STREAM_MIC1_FIELD];
   call $cvc.stream.get_buffer_with_start_address;
   r2 = M[r8 + $cvc_send.cap.SCRATCH_BUFFER];
   r2 = M[r2];
   Null = r2 - r0;
   if NZ call $cvc_send.mic_stream_update;

   // Input stream processing: copy external input streams into internal streams
   r9 = M[SP - 1 * ADDR_PER_WORD];
   r4 = r9 + $cvc_send.cap.REF_INPUT_STREAM_MAP_PTR_FIELD;
   r5 = r9 + $cvc_send.cap.PTR_INPUT_STREAM_REF_FIELD;
   r6 = M[r9 + $cvc_send.cap.FRAME_SIZE_FIELD];
   r9 = 0; 
   r2 = M[r8 + $cvc_send.cap.PARAMS_PTR_FIELD];                                  // load pointer to params
   rMACB = r2 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_PROG_STREAM_GAIN_REF;          // load pointer to ref_gain
   
   input_stream_proc:
      r7 = M[r4 + r9];
      r8 = M[r5 + r9];
      call $cvc.stream_transfer.peak.cbuffer_in;
      r1 = M[rMACB + r9];                                                        // load the prog_gain param for stream
      call $cvc.stream_prog_gains_func;                                          // call stream gains function
      r9 = r9 + MK1;
   Null = r9 - (CVC_SEND_MAX_NUM_INPUT * MK1);
   if NZ jump input_stream_proc;

   // Pass in Current Mode & Overide Control
   r8 = M[SP - 1 * ADDR_PER_WORD];   // extended data
   r1 = M[r8 + $cvc_send.cap.CUR_MODE];
   r2 = M[r8 + $cvc_send.cap.OVR_CONTROL];
   call $cvc_send.process_data;

   // Output stream processing: copy internal streams into external output streams
   r9 = M[SP - 1 * ADDR_PER_WORD];
   r4 = r9 + $cvc_send.cap.PTR_OUTPUT_STREAM_SND_FIELD;
   r5 = r9 + $cvc_send.cap.OUTPUT_STREAM_MAP_PTR_FIELD;
   r6 = M[r9 + $cvc_send.cap.FRAME_SIZE_FIELD];
   r9 = M[r9 + $cvc_send.cap.NUM_VA_OUTPUTS];
   r9 = r9 + 1;
   output_stream_proc:
      r7 = M[r4];
      r8 = M[r5];
      call $cvc.stream_transfer.peak.cbuffer_out;
      r4 = r4 + MK1;
      r5 = r5 + MK1;
      r9 = r9 - 1;
   if NZ jump output_stream_proc;

   // Release committed scratch
   r8 = M[SP - 1 * ADDR_PER_WORD];
   NULL =  M[r8 + $cvc_send.cap.SCRATCH_ALLOC_PTR_FIELD];
   if NZ call $_scratch_free;

   pop r0;     // remove local variables

   call $_cvc_register_restore;
   pop rLink;
   rts;

// -----------------------------------------------------------------------------
// cVc microphone streams configuration: setting frame size and working buffer
// -----------------------------------------------------------------------------
$cvc_send.mic_stream_update:
   push rLink;

   // r2 from input
   // r3 -> step size
   r0 = r8 + $cvc_send.cap.DATA_VARIANT;
   call $cvc.variant.fft_numbins;
   r3 = r0;
   Words2Addr(r3);

   r0 = M[r8 + $cvc_send.cap.PTR_INPUT_STREAM_MIC1_FIELD];
   r1 = M[r8 + $cvc_send.cap.FRAME_SIZE_FIELD];
   call $cvc.stream.set_address_and_size;

   r0 = M[r8 + $cvc_send.cap.PTR_INPUT_STREAM_MIC2_FIELD];
   r2 = r2 + r3;
   call $cvc.stream.set_address_and_size;

   r0 = M[r8 + $cvc_send.cap.PTR_INPUT_STREAM_MIC3_FIELD];
   r2 = r2 + r3;
   call $cvc.stream.set_address_and_size;

   r0 = M[r8 + $cvc_send.cap.PTR_INPUT_STREAM_MIC4_FIELD];
   r2 = r2 + r3;
   call $cvc.stream.set_address_and_size;

   jump $pop_rLink_and_rts;

.ENDMODULE;



// *****************************************************************************
// MODULE:
//      extern unsigned COMPUTE_VOICE_QUALITY_METRIC(CVC_SEND_OP_DATA *op_extra_data);
//
// DESCRIPTION:
//
// MODIFICATIONS:
//
// INPUTS:
//    - r0 - *extra_op_data
//
// OUTPUTS:
//    - r0 - result
//
// TRASHED REGISTERS:
//    - 
//
// CPU USAGE:
// *****************************************************************************
.MODULE $M.COMPUTE_VOICE_QUALITY_METRIC;
   .CODESEGMENT PM;
   .DATASEGMENT DM;
   
$_compute_voice_quality_metric:
   
   LIBS_PUSH_REGS_SLOW_SW_ROM_PATCH_POINT($CVC_Send_Set_Voice_Quality.PATCH_ID)
   push rLink;
   pushm <r5,r6,r7>;


   //check mode 
   r5 = M[r0 + $cvc_send.cap.CUR_MODE];
   Null = r5 - $M.GEN.CVC_SEND.SYSMODE.FULL;
   if Z jump compute_WPL;
   Null = r5 - $M.GEN.CVC_SEND.SYSMODE.LOWVOLUME;
   if Z jump compute_WPL;
   jump return_error;

   compute_WPL:
      // WPL = 0 : no shift applied to the scale
      // WPL = -1 : right shift applied to the scale (divide by 2)
      r5 = r0;// extra_op_data, from op_data  
      r6 = M[r5 + $cvc_send.cap.CVC_DATA_ROOT_FIELD]; //cvc_root from extra_op_data
      r0 = M[r6 + $cvc_send.data.hfk_config]; 
      r0 = r0 AND $M.GEN.CVC_SEND.CONFIG.HFK.BYP_DMS;
      if NZ jump return_error;//if dms200_obj is bypassed, quit

      r7 = M[r5 + $cvc_send.cap.STATUS_TABLE_PTR_FIELD]; //status table from *extra_op_data
      r7 = r7 - $M.GEN.CVC_SEND.STATUS.COMPILED_CONFIG;//status array from status table
      
      r0 = M[r5 + $cvc_send.cap.NUM_MICS];//number of mics from *extra_op_data
      r0 = r0 - 1;
      if Z jump no_ASF; // 1 mic mode, no ASF

      //r6: cvc_root
      //r7: status array
      use_ASF:
         r5 = M[r6 + $cvc_send.data.dms200_obj];// $dms200_obj from *cvc_root
         r6 = 0; //default WPL
         r0 = M[r7 + $M.GEN.CVC_SEND.STATUS.WIND_FLAG];
         r0 = M[r0];
         if Z jump compute_metric;// no wind, no shift
         
         // get the mean G0 
         r0 = M[r7 + $M.GEN.CVC_SEND.STATUS.WNR_WIND_PHASE];
         r0 = r0 - 0.5;//format: Q1.N
         if NEG jump return_0_scale; // high wind
         
         r6 = -1; // medium wind, set the right shift for the SNR_scale to -1
         jump compute_metric;
        
      //r5: extra_op_data 
      //r6: cvc_root 
      no_ASF:
         r5 = M[r6 + $cvc_send.data.dms200_obj]; // $dms200_obj from *cvc_root
         r7 = M[r5 + $M.oms280.WNR_ENABLED_FIELD]; //$wnr_obj from $dms200_obj 
         
         r6 = 0; //default WPL
         Null = r7;
         if Z jump compute_metric;// no WNR object, no shift
         
         r0 = M[r7 + $M.oms280.wnr.PTR_WIND_FLAG_FIELD];
         Null = M[r0];
         if Z jump compute_metric; // no wind, no shift
         
         r7 = M[r7 + $M.oms280.wnr.ROLLOFF_IDX_FIELD]; //rolloff_idx from wnr_obj
         r7 = r7 - $MAX_ROLLOFF_IDX_DIVIDED_BY_2; //
         if NEG jump return_0_scale;// high wind, return 0 scale

         r6 = -1; // medium wind, set the right shift for the SNR_scale to -1

      // r6: WPL
      compute_metric:
         r0 = M[r5 + $dms200.VOICED_SNR_FIELD];        // voiced SNR from dms200_obj
         r0 = r0 - $SNR_MIN_FIXED_POINT;
         r0 = r0 * $SNR_FACTOR_FIXED_POINT (frac); // (SNR_scale_h - SNR_scale_l + 1.0)/(SNR_max - SNR_min)   
         r0 = r0 LSHIFT 7; // shift to recover the Q8.24 format                
         if NEG jump return_0_scale;	//Scale_snr = min(Scale_snr, Scale_l), here,Scale_l is 0
         
         r5 = $SNR_SCALE_HIGHER_LIMIT_FIXED_POINT;
         Null = r0 - $SNR_SCALE_HIGHER_LIMIT_FIXED_POINT;
         if GT r0 = r5;//Scale_snr = max(Scale_snr, Scale_h)
         
         r0 = r0 ASHIFT r6;//scale the Scale_snr by the WPL shift amount
         r0 = r0 ASHIFT -24;//get rid of the fractional part
         jump exit;
         
      return_0_scale:
         r0 = 0;// metric scale
         jump exit;
         
      return_error:
         r0 = $VOICE_QUALITY_METRIC_ERROR_CODE;
exit:  
   popm <r5,r6,r7>;  
   pop rLink;
   rts;

.ENDMODULE;


// *****************************************************************************
// MODULE:
//    $M.cvc_send.stream_prog_gains
//
// DESCRIPTION:
//    Programmable gains on the cvc_send streams
//
// MODIFICATIONS:
//
// INPUTS:
//    r8 - input stream
//    r1 - gain in linear domain
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//
// CPU USAGE:
// *****************************************************************************
.MODULE $M.cvc_send.stream_prog_gains;
   .CODESEGMENT PM;

$cvc.stream_prog_gains_func:

  push rLink;
  
  pushm <r5,r6>;                           // save r5,r6
  r7 = r8;                                 // set output for $cvc.stream_gain
  r5 = r1;                                 // r5:mantissa
  r6 = 7;                                  // r6:exponent
  call $cvc.stream_gain;                   // call gain apply function
  popm <r5,r6>;                            // pop r5,r6
  
  jump $pop_rLink_and_rts;

.ENDMODULE;  
