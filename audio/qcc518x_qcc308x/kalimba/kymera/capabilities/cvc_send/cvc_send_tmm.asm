// *****************************************************************************
// Copyright (c) 2014 - 2023 Qualcomm Technologies International, Ltd.
// %% version
//
// $Change$  $DateTime$
// *****************************************************************************

#include "cvc_send_data.h"
#include "patch/patch_library.h"


//******************************************************************************
// MODULE:
//    $asf100.wnr.tmm.initialize
//
// DESCRIPTION:
//    WNR TMM initialize
//
// INPUTS:
//    - r8 - pointer to WNR TMM data object
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//    - r0-r8
//
// CPU USAGE:
//******************************************************************************
.MODULE $M.asf100.wnr.tmm.initialize;

   .CODESEGMENT CVC_INIT_PM;

$asf100.wnr.tmm.initialize:
   #if defined(PATCH_LIBS)
      LIBS_SLOW_SW_ROM_PATCH_POINT($asf100.wnr.tmm.initialize.PATCH_ID_0, r1)
   #endif

   // --------------------------------------------------------------------------
   // Load HI_Wind common data object
   // --------------------------------------------------------------------------
   r1 = M[r8 + $asf100.wnr.tmm.WNR_MAP_OBJ_FIELD];
   r0 = M[r1 + $asf100.wnr_map.HI_WIND_PARAM_FIELD];

   // --------------------------------------------------------------------------
   // Hysteresis to HI WIND flags
   // --------------------------------------------------------------------------
   r5 = M[r0 + $asf100.wnr_map.param.T_HANG_UP];
   r6 = M[r0 + $asf100.wnr_map.param.T_HANG_DOWN];
   r7 = M[r1 + $asf100.wnr_map.PTR_VARIANT_FIELD];
   r8 = r8 + $asf100.wnr.tmm.HI_WIND_FLAG_FIELD;
   jump $cvc.hyst_init;

.ENDMODULE;


//******************************************************************************
// MODULE:
//    $asf100.wnr.tmm.process
//
// DESCRIPTION:
//    WNR TMM process
//
//    Wind_Silence  = LpN_Mic_dB/10*log2(10) + PWR_ADJUST;
//    HI_Wind_Th_Silence = Wind_Silence + Hi_Wind_Th_Pwr;
//    HI_Power_Flag = Wind_Det_LpX0  > Hi_Wind_Th_Silence;
//    Hi_Wind_Flag = Hi_Power_Flag && HI_Phase_Flag && HI_Coher_Flag;
//
// INPUTS:
//    - r8 - pointer to WNR TMM data object
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//    - r0-r8
//
// CPU USAGE:
//******************************************************************************
.MODULE $M.asf100.wnr.tmm.process;

   .CODESEGMENT CVC_PROC_PM;

$asf100.wnr.tmm.process:
   #if defined(PATCH_LIBS)
      LIBS_SLOW_SW_ROM_PATCH_POINT($asf100.wnr.tmm.process.PATCH_ID_0, r1)
   #endif

   // --------------------------------------------------------------------------
   // Wind_Silence  = LpN_Mic_dB/10*log2(10) + PWR_ADJUST;
   // HI_Wind_Th_Silence = Wind_Silence + Hi_Wind_Th_Pwr;
   // HI_Power_Flag = Wind_Det_LpX0  > Hi_Wind_Th_Silence;
   // Hi_Wind_Flag = Hi_Power_Flag && HI_Phase_Flag && HI_Coher_Flag;
   // --------------------------------------------------------------------------
   r1 = M[r8 + $asf100.wnr.tmm.WNR_MAP_OBJ_FIELD];
   r5 = M[r1 + $asf100.wnr_map.PHASE_COHER_FLAG_FIELD];
   r0 = M[r1 + $asf100.wnr_map.MEAN_PWR_FIELD];
   r1 = M[r8 + $asf100.wnr.tmm.HI_WIND_POW_TH];
   Null = r0 - r1;
   if LE r5 = 0;

   // Hysteresis: Hi_Wind_Flag
   r8 = r8 + $asf100.wnr.tmm.HI_WIND_FLAG_FIELD;
   jump $cvc.hyst_proc;

.ENDMODULE;


// *****************************************************************************
// MODULE:
//    $dmss.snr.tmm.initialize
//
// DESCRIPTION:
//    SNR TMM initialize
//
// MODIFICATIONS:
//    2021/12/08 Wed wms - separated from $dmss.snr.initialize
//
// INPUTS:
//    r8 - SNR TMM data object
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//    - r0-r8
//
// CPU USAGE:
// *****************************************************************************
.MODULE $M.dmss.snr.tmm.initialize;

   .CODESEGMENT CVC_INIT_PM;

$dmss.snr.tmm.initialize:
   #if defined(PATCH_LIBS)
      LIBS_SLOW_SW_ROM_PATCH_POINT($dmss.snr.tmm.init.PATCH_ID_0, r2)
   #endif

   // noise_flag = 0;
   M[r8 + $dmss.snr.tmm.NOISE_FLAG_FIELD] = Null;

   // Hysteresis to noise flag
   r3 = M[r8 + $dmss.snr.tmm.MAIN_OBJ_FIELD];
   r4 = M[r3 + $dmss.snr.PARAM_FIELD];
   r5 = M[r4 + $dmss.snr.param.T_HANG_UP];
   r6 = M[r4 + $dmss.snr.param.T_HANG_DOWN];
   r7 = M[r3 + $dmss.snr.PTR_VARIANT_FIELD];
   r8 = r8 + $dmss.snr.tmm.NOISE_HYST_FIELD;
   jump $cvc.hyst_init;

.ENDMODULE;


// *****************************************************************************
// MODULE:
//    $dmss.snr.tmm.process
//
// DESCRIPTION:
//    if (noise_power > threshold_high_noise_level)
//       High_level_noise = 1;
//    end
//    if (noise_power < threshold_low_noise_level)
//       High_level_noise = 0;
//    end
//
// MODIFICATIONS:
//    2021/12/08 Wed wms - separated from $dmss.snr.initialize
//
// INPUTS:
//    r8 - SNR TMM data object
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//    - r0-r5, r8
//
// CPU USAGE:
//
// NOTES:
// *****************************************************************************
.MODULE $M.dmss.snr.tmm.process;

   .CODESEGMENT CVC_PROC_PM;

$dmss.snr.tmm.process:
   #if defined(PATCH_LIBS)
      LIBS_SLOW_SW_ROM_PATCH_POINT($dmss.snr.tmm.process.PATCH_ID_0, r1)
   #endif

   // --------------------------------------------------------------------------
   // Load noise_power
   // --------------------------------------------------------------------------
   r0 = M[r8 + $dmss.snr.tmm.MAIN_OBJ_FIELD];
   r0 = M[r0 + $dmss.snr.NOISE_EST_FIELD];

   // --------------------------------------------------------------------------
   // if (noise_power > threshold_high_noise_level)
   //    High_level_noise = 1;
   // end
   // if (noise_power < threshold_low_noise_level)
   //    High_level_noise = 0;
   // end
   // --------------------------------------------------------------------------
   r5 = M[r8 + $dmss.snr.tmm.NOISE_FLAG_FIELD];
   r1 = M[r8 + $dmss.snr.tmm.NOISE_ON_THRESH_FIELD];
   Null = r0 - r1;
   if GT r5 = 1;
   r1 = M[r8 + $dmss.snr.tmm.NOISE_OFF_THRESH_FIELD];
   Null = r0 - r1;
   if LT r5 = 0;
   M[r8 + $dmss.snr.tmm.NOISE_FLAG_FIELD] = r5;

   // --------------------------------------------------------------------------
   // Hysteresis to SNR High_level_noise
   // --------------------------------------------------------------------------
   r8 = r8 + $dmss.snr.tmm.NOISE_HYST_FIELD;
   jump $cvc.hyst_proc;

.ENDMODULE;


// *****************************************************************************
// MODULE:
//    $cvc.tmm.initialize
//
// DESCRIPTION:
//
// MODIFICATIONS:
//
// INPUTS:
//    - r8 - module object (tmm_obj)
//    - r9 - cvc data root object
//
// OUTPUTS:
//
// TRASHED REGISTERS:
//
// CPU USAGE:
//
// NOTE:
// *****************************************************************************
.MODULE $M.CVC_SEND.module_init.tmm;

   .CODESEGMENT PM;

$cvc.tmm.initialize:
   LIBS_SLOW_SW_ROM_PATCH_POINT($cvc.tmm.initialize.PATCH_ID_0, r3)

   // r7 - cvc data root object
   r7 = r9;
   // r9 -> cap_data
   r9 = M[r9 + $cvc_send.data.cap_root_ptr];
   // r3 -> CPS param
   r3 = M[r9 + $cvc_send.cap.PARAMS_PTR_FIELD];

   // register tmm_obj into cap data
   M[r9 + $cvc_send.cap.TMM_OBJ_FIELD] = r8;

   // External NS params
   r4 = r8 + $cvc.tmm.PARAM_NS_EXT;
   r2 = M[r7 + $cvc_send.data.dms200_obj];
   M[r2 + $M.oms280.PARAM_FIELD] = r4;
   r0 = M[r3 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_DMS_AGGR];
   M[r4 + $dms200.param.AGRESSIVENESS_FIELD] = r0;
   r0 = M[r3 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_DMS_RESIDUAL_NFLOOR];
   M[r4 + $dms200.param.RESIDUAL_NOISE_FIELD] = r0;
   r0 = M[r3 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_DMS_NSN_AGGR];
   M[r4 + $dms200.param.NSN_AGGR_FIELD] = r0;
   r0 = M[r3 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_DMS_AGGR_VA];
   M[r4 + $dms200.param.VA_AGRESSIVENESS_FIELD] = r0;

   // DMSS RNR params
   r5 = M[r7 + $cvc_send.data.dmss_obj];
   r0 = r3 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_RNR_AGGR;
   M[r5 + $dmss.PARAM_FIELD] = r0;

   // WNR object: Tier Moderate
   r0 = M[r3 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_MODERATE_WNR_THRESHOLD];
   r2 = M[r8 + $cvc.tmm.MODERATE_WNR_OBJ_PTR];
   M[r2 + $asf100.wnr_map.HI_WIND_POW_TH_FIELD] = r0;

   // WNR object: Tier High
   r0 = M[r3 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_HIGH_WNR_THRESHOLD];
   r4 = r8 + $cvc.tmm.HIGH_WNR_OBJ;
   M[r4 + $asf100.wnr.tmm.HI_WIND_POW_TH] = r0;
   M[r4 + $asf100.wnr.tmm.WNR_MAP_OBJ_FIELD] = r2;

   // WNR object: Tier Extreme
   r0 = M[r3 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_EXTREME_WNR_THRESHOLD];
   r4 = r8 + $cvc.tmm.EXTREME_WNR_OBJ;
   M[r4 + $asf100.wnr.tmm.HI_WIND_POW_TH] = r0;
   M[r4 + $asf100.wnr.tmm.WNR_MAP_OBJ_FIELD] = r2;

   // SNR object: Tier Moderate
   r0 = M[r3 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_MODERATE_NOISE_THRESH_HIGH];
   r1 = M[r3 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_MODERATE_NOISE_THRESH_LOW];
   r2 = M[r8 + $cvc.tmm.MODERATE_SNR_OBJ_PTR];
   M[r2 + $dmss.snr.NOISE_ON_THRESH_FIELD] = r0;
   M[r2 + $dmss.snr.NOISE_OFF_THRESH_FIELD] = r1;

   // SNR object: Tier High
   r0 = M[r3 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_HIGH_NOISE_THRESH_HIGH];
   r1 = M[r3 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_HIGH_NOISE_THRESH_LOW];
   r4 = r8 + $cvc.tmm.HIGH_SNR_OBJ;
   M[r4 + $dmss.snr.tmm.NOISE_ON_THRESH_FIELD] = r0;
   M[r4 + $dmss.snr.tmm.NOISE_OFF_THRESH_FIELD] = r1;
   M[r4 + $dmss.snr.tmm.MAIN_OBJ_FIELD] = r2;

   // SNR object: Tier Extreme
   r0 = M[r3 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_EXTREME_NOISE_THRESH_HIGH];
   r1 = M[r3 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_EXTREME_NOISE_THRESH_LOW];
   r4 = r8 + $cvc.tmm.EXTREME_SNR_OBJ;
   M[r4 + $dmss.snr.tmm.NOISE_ON_THRESH_FIELD] = r0;
   M[r4 + $dmss.snr.tmm.NOISE_OFF_THRESH_FIELD] = r1;
   M[r4 + $dmss.snr.tmm.MAIN_OBJ_FIELD] = r2;

   push rLink;

   // In-ear modules
   Null = M[r8 + $cvc.tmm.BLEND_OBJ];
   if NZ call tmm.ie_init;

   // mlmask_obj
   Null = M[r9 + $cvc_send.cap.MLFE_OBJ_FIELD];
   if NZ call $cvc.tmm.ml_init;

   // MBDRC100: r8 -> tmm_obj, r6 -> CPS param, r2 -> mbdrc100_obj
   r6 = M[r9 + $cvc_send.cap.PARAMS_PTR_FIELD];
   r2 = M[r9 + $cvc_send.cap.MBDRC_OBJECT_FIELD];
   r0 = r8 + $cvc.tmm.PARAM_MBDRC;
   M[r2 + $mbdrc100_library.mbdrc100_object_t_struct.PARAM_PTR_FIELD] = r0;
   call $cvc.tmm.mbdrc_default;

   // wnr/noise detection init
   r9 = r8;

   // WNR TMM initialize - Tier High
   r8 = r9 + $cvc.tmm.HIGH_WNR_OBJ;
   call $asf100.wnr.tmm.initialize;

   // WNR TMM initialize - Tier Extreme
   r8 = r9 + $cvc.tmm.EXTREME_WNR_OBJ;
   call $asf100.wnr.tmm.initialize;

   // SNR TMM initialize - Tier High
   r8 = r9 + $cvc.tmm.HIGH_SNR_OBJ;
   call $dmss.snr.tmm.initialize;

   // SNR TMM initialize - Tier Extreme
   r8 = r9 + $cvc.tmm.EXTREME_SNR_OBJ;
   call $dmss.snr.tmm.initialize;

   pop rLink;
   rts;

tmm.ie_init:
   // Internal NS params
   r6 = r8 + $cvc.tmm.PARAM_NS_INT;
   r1 = M[r8 + $cvc.tmm.NS_INT_OBJ];
   M[r1 + $M.oms280.PARAM_FIELD] = r6;
   r0 = M[r3 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_INT_DMS_AGGR];
   M[r6 + $dms200.param.AGRESSIVENESS_FIELD] = r0;
   r0 = M[r3 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_INT_DMS_RESIDUAL_NFLOOR];
   M[r6 + $dms200.param.RESIDUAL_NOISE_FIELD] = r0;
   rts;

.ENDMODULE;


// *****************************************************************************
// MODULE:
//    $cvc.tmm.process
//
// DESCRIPTION:
//
// MODIFICATIONS:
//
// INPUTS:
//    - r8 - module object (tmm_obj)
//    - r9 - cvc data root object
//
// OUTPUTS:
//
// TRASHED REGISTERS:
//
// CPU USAGE:
//
// NOTE:
// *****************************************************************************
.MODULE $M.CVC_SEND.module_control.tmm;

   .CODESEGMENT PM;

$cvc.tmm.process:
   LIBS_SLOW_SW_ROM_PATCH_POINT($cvc.tmm.process.PATCH_ID_0, r3)

   push rLink;
   r7 = r8;

   // WNR TMM process - Tier High
   r8 = r7 + $cvc.tmm.HIGH_WNR_OBJ;
   call $asf100.wnr.tmm.process;

   // WNR TMM process - Tier Extreme
   r8 = r7 + $cvc.tmm.EXTREME_WNR_OBJ;
   call $asf100.wnr.tmm.process;

   // SNR TMM process - Tier High
   r8 = r7 + $cvc.tmm.HIGH_SNR_OBJ;
   call $dmss.snr.tmm.process;

   // SNR TMM process - Tier Extreme
   r8 = r7 + $cvc.tmm.EXTREME_SNR_OBJ;
   call $dmss.snr.tmm.process;

   // r8 -> tmm_obj
   r8 = r7;
   // r7 -> CPS param
   r7 = M[r9 + $cvc_send.data.param];
   // r5 -> HFK_CONFIG
   r5 = M[r7 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_HFK_CONFIG];

   // Tiering mode 3 - extreme wind/noise ?
   r3 = r8 + $cvc.tmm.EXTREME_WNR_OBJ;
   r1 = r8 + $cvc.tmm.EXTREME_SNR_OBJ;
   r0 = $cvc.tmm.mode.EXTREME_WIND;
   Null = M[r3 + $asf100.wnr.tmm.HI_WIND_FLAG_FIELD];
   if NZ jump tiering_process_extreme_wind;
   r0 = $cvc.tmm.mode.EXTREME_NOISE;
   Null = M[r1 + $dmss.snr.tmm.NOISE_HYST_FIELD];
   if NZ jump tiering_process_extreme;

   // Tiering mode 2 - high wind/noise ?
   r3 = r8 + $cvc.tmm.HIGH_WNR_OBJ;
   r1 = r8 + $cvc.tmm.HIGH_SNR_OBJ;
   r0 = $cvc.tmm.mode.HIGH_WIND;
   Null = M[r3 + $asf100.wnr.tmm.HI_WIND_FLAG_FIELD];
   if NZ jump tiering_process_high_wind;
   r0 = $cvc.tmm.mode.HIGH_NOISE;
   Null = M[r1 + $dmss.snr.tmm.NOISE_HYST_FIELD];
   if NZ jump tiering_process_high;

   // Tiering mode 1 - moderate wind/noise ?
   r3 = M[r8 + $cvc.tmm.MODERATE_WNR_OBJ_PTR];
   r1 = M[r8 + $cvc.tmm.MODERATE_SNR_OBJ_PTR];
   r0 = $cvc.tmm.mode.MODERATE_WIND;
   Null = M[r3 + $asf100.wnr_map.HI_WIND_FLAG_FIELD];
   if NZ jump tiering_process_moderate_wind;
   r0 = $cvc.tmm.mode.MODERATE_NOISE;
   Null = M[r1 + $dmss.snr.NOISE_HYST_FIELD];
   if NZ jump tiering_process_moderate;

   // Tiering mode 0 - Noise clean
tiering_process_clean_voice:
   // Tiering Mode
   r0 = $cvc.tmm.mode.LOW;
   M[r8 + $cvc.tmm.MODE_FLAG] = r0;

   // PEQ control
   M[r8 + $cvc.tmm.PEQ_OFF_FLAG] = Null;

   // Tiering Parameters
   r0 = r7 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_RNR_AGGR;
   r1 = M[r7 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_DMS_AGGR];
   r2 = M[r7 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_DMS_NSN_AGGR];
   r3 = M[r7 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_INT_DMS_AGGR];
   r4 = M[r7 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_BLEND_CROSSOVER_FREQ_MODERATE];
   r5 = M[r7 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_EQ_NULL_FREQ_MODERATE];
   r6 = M[r7 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_EQ_MAX_AMPLIFICATION];
   jump tiering_param_set;

tiering_process_moderate_wind:
   r2 = $cvc.tmm.mode.MODERATE;
   Null = M[r1 + $dmss.snr.NOISE_HYST_FIELD];
   if NZ r0 = r2;
tiering_process_moderate:
   // Tiering Mode
   M[r8 + $cvc.tmm.MODE_FLAG] = r0;

   // PEQ control
   r0 = r5 AND $M.GEN.CVC_SEND.CONFIG.HFK.BYP_PEQ_TIER_MODERATE;
   M[r8 + $cvc.tmm.PEQ_OFF_FLAG] = r0;

   // Tiering Parameters
   r0 = r7 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_RNR_AGGR_MODERATE;
   r1 = M[r7 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_DMS_AGGR_MODERATE];
   r2 = M[r7 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_DMS_NSN_AGGR_MODERATE];
   r3 = M[r7 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_INT_DMS_AGGR];
   r4 = M[r7 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_BLEND_CROSSOVER_FREQ_MODERATE];
   r5 = M[r7 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_EQ_NULL_FREQ_MODERATE];
   r6 = M[r7 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_EQ_MAX_AMPLIFICATION];
   jump tiering_param_set;

tiering_process_high_wind:
   r2 = $cvc.tmm.mode.HIGH;
   Null = M[r1 + $dmss.snr.tmm.NOISE_HYST_FIELD];
   if NZ r0 = r2;
tiering_process_high:
   // Tiering Mode
   M[r8 + $cvc.tmm.MODE_FLAG] = r0;

   // PEQ control
   r0 = r5 AND $M.GEN.CVC_SEND.CONFIG.HFK.BYP_PEQ_TIER_HIGH;
   M[r8 + $cvc.tmm.PEQ_OFF_FLAG] = r0;

   // Tiering Parameters
   r0 = r7 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_RNR_AGGR_HIGH;
   r1 = M[r7 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_DMS_AGGR_HIGH];
   r2 = M[r7 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_DMS_NSN_AGGR_HIGH];
   r3 = M[r7 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_INT_DMS_AGGR];
   r4 = M[r7 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_BLEND_CROSSOVER_FREQ_HIGH];
   r5 = M[r7 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_EQ_NULL_FREQ_HIGH];
   r6 = M[r7 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_EQ_MAX_AMPLIFICATION];
   jump tiering_param_set;

tiering_process_extreme_wind:
   r2 = $cvc.tmm.mode.EXTREME;
   Null = M[r1 + $dmss.snr.tmm.NOISE_HYST_FIELD];
   if NZ r0 = r2;
tiering_process_extreme:
   // Tiering Mode
   M[r8 + $cvc.tmm.MODE_FLAG] = r0;

   // PEQ control
   r0 = r5 AND $M.GEN.CVC_SEND.CONFIG.HFK.BYP_PEQ_TIER_EXTREME;
   M[r8 + $cvc.tmm.PEQ_OFF_FLAG] = r0;

   // Tiering Parameters
   r0 = r7 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_RNR_AGGR_HIGH;
   r1 = M[r7 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_DMS_AGGR_HIGH];
   r2 = M[r7 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_DMS_NSN_AGGR_HIGH];
   r3 = M[r7 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_INT_DMS_AGGR_EXTREME];
   r4 = M[r7 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_BLEND_CROSSOVER_FREQ_EXTREME];
   r5 = M[r7 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_EQ_NULL_FREQ_EXTREME];
   r6 = M[r7 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_EQ_MAX_AMPLIFICATION_EXTREME];

tiering_param_set:
   // RNR
   r7 = M[r9 + $cvc_send.data.dmss_obj];
   M[r7 + $dmss.PARAM_FIELD] = r0;

   // External NS
   r0 = r8 + $cvc.tmm.PARAM_NS_EXT;
   M[r0 + $dms200.param.AGRESSIVENESS_FIELD] = r1;
   M[r0 + $dms200.param.NSN_AGGR_FIELD] = r2;

   // In-ear modules
   Null = M[r8 + $cvc.tmm.BLEND_OBJ];
   if Z jump tmm_ml;

   // Internal NS
   r0 = r8 + $cvc.tmm.PARAM_NS_INT;
   M[r0 + $dms200.param.AGRESSIVENESS_FIELD] = r3;

   // Blend
   r0 = M[r8 + $cvc.tmm.BLEND_OBJ];
   M[r0 + $blend100.CROSSOVER_FREQ_FIELD] = r4;

   // EQ MAP
   r0 = M[r8 + $cvc.tmm.MAP_OBJ];
   M[r0 + $eq100.NULL_FREQ] = r5;
   M[r0 + $eq100.MAX_AMPLIFICATION] = r6;

tmm_ml:
   // r5 -> cap_data
   r5 = M[r9 + $cvc_send.data.cap_root_ptr];

   // r7 -> mlmask_obj
   r7 = M[r5 + $cvc_send.cap.MLFE_OBJ_FIELD];
   if NZ call $cvc.tmm.ml_proc;

tmm_mbdrc:
   // r6 -> CPS param
   r6 = M[r9 + $cvc_send.data.param];
   // Tier
   r5 = M[r8 + $cvc.tmm.MODE_FLAG];

   // Low Tier
   Null = r5 - $cvc.tmm.mode.LOW;
   if Z jump tier_mgdc_set_default;

   // Moderate Tier
   r0 = r6 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_MODERATE_MBDRC_BAND0_THRESHOLD;
   Null = r5 - $cvc.tmm.mode.MODERATE;
   if LE jump tier_mgdc_set_param;

   // High Tier
   r0 = r6 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_HIGH_MBDRC_BAND0_THRESHOLD;
   Null = r5 - $cvc.tmm.mode.HIGH;
   if LE jump tier_mgdc_set_param;

   // Extreme Tier
   r0 = r6 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_EXTREME_MBDRC_BAND0_THRESHOLD;

tier_mgdc_set_param:
   M3 = 4;
   I4 = r0;
   r4 = r8 + $cvc.tmm.PARAM_MBDRC;

   // band-0
   I0 = r4 + ($M.GEN.CVC_SEND.PARAMETERS.OFFSET_SND_MBDRC_BAND0_THRESHOLD - $M.GEN.CVC_SEND.PARAMETERS.OFFSET_SND_MBDRC_NUM_BANDS);
   call $cvc.tmm.copy.I4_to_I0;
   // band-1
   I0 = r4 + ($M.GEN.CVC_SEND.PARAMETERS.OFFSET_SND_MBDRC_BAND1_THRESHOLD - $M.GEN.CVC_SEND.PARAMETERS.OFFSET_SND_MBDRC_NUM_BANDS);
   call $cvc.tmm.copy.I4_to_I0;
   // band-2
   I0 = r4 + ($M.GEN.CVC_SEND.PARAMETERS.OFFSET_SND_MBDRC_BAND2_THRESHOLD - $M.GEN.CVC_SEND.PARAMETERS.OFFSET_SND_MBDRC_NUM_BANDS);
   call $cvc.tmm.copy.I4_to_I0;
   // band-3
   I0 = r4 + ($M.GEN.CVC_SEND.PARAMETERS.OFFSET_SND_MBDRC_BAND3_THRESHOLD - $M.GEN.CVC_SEND.PARAMETERS.OFFSET_SND_MBDRC_NUM_BANDS);
   call $cvc.tmm.copy.I4_to_I0;

   pop rLink;
   rts;

tier_mgdc_set_default:
   pop rLink;
$cvc.tmm.mbdrc_default:
   M3 = 29;
   r0 = r8 + $cvc.tmm.PARAM_MBDRC;
   r1 = r6 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_SND_MBDRC_NUM_BANDS;
   jump $cvc.copy;

.ENDMODULE;


// *****************************************************************************
// MODULE:
//    TMM utility functions
//
// DESCRIPTION:
//
// MODIFICATIONS:
//
// INPUTS:
//
// OUTPUTS:
//
// TRASHED REGISTERS:
//
// CPU USAGE:
//
// NOTE:
// *****************************************************************************
.MODULE $M.CVC_SEND.tmm.utils;

   .CODESEGMENT PM;

$cvc.tmm.copy.I4_to_I0:
   r10 = M3;
   do vcp_loop;
      r0 = M[I4,MK1];
      M[I0,MK1] = r0;
   vcp_loop:
   rts;

.ENDMODULE;


// *****************************************************************************
// MODULE:
//    TMM ML
//
// DESCRIPTION:
//
// MODIFICATIONS:
//
// INPUTS:
//
// OUTPUTS:
//
// TRASHED REGISTERS:
//
// CPU USAGE:
//
// NOTE:
// *****************************************************************************
.MODULE $M.CVC_SEND.tmm.ml;

   .CODESEGMENT PM;

#if defined(INSTALL_OPERATOR_CVC_EARBUD_3MIC_HYBRID) || defined(INSTALL_OPERATOR_CVC_HEADSET_2MIC_MONO_HYBRID) || defined(INSTALL_OPERATOR_CVC_HEADSET_1MIC_HYBRID)
$cvc.tmm.ml_init:
   // r3=CPS params r8=TMM object, r9= root object
   push rLink;
   
   // point MLFE params_ptr to TMM MLFE params
   r0 = r8 + $cvc.tmm.PARAM_MLFE;
   r7 = M[r9 + $cvc_send.cap.MLFE_OBJ_FIELD]; // r7=MLFE object
   M[r7 + $mlfe100_library_c.MLFE100_STRUC_struct.PARAM_PTR_FIELD] = r0;
   
   // copy all 4 MFLE parameters from curparams MLFE low tier to TMM MLFE params
   M3 = 4;
   r1 = r3 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_ML_GAIN;
   call $cvc.copy;

   // set bypass flag to low tier bypass
   r1 = M[r3 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_HFK_CONFIG];
   r0 = r1 AND $M.GEN.CVC_SEND.CONFIG.HFK.BYP_ML_TIER_LOW;
   M[r7 + $mlfe100_library_c.MLFE100_STRUC_struct.BYPASS_FLAG_FIELD] = r0;

   // ML stream gain
   r0 = M[r3 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_SNDGAIN];
   M[r8 + $cvc.tmm.STREAM_GAIN] = r0;

   pop rLink;
   rts;

$cvc.tmm.ml_proc:
   // r6 -> CPS param
   r6 = M[r9 + $cvc_send.data.param];
   // r3 -> HFK_CONFIG
   r3 = M[r6 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_HFK_CONFIG];

   // Tier
   r4 = M[r8 + $cvc.tmm.MODE_FLAG];

   // Low Tier
   r0 = r6 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_ML_GAIN;
   r1 = M[r6 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_SNDGAIN];
   r2 = r3 AND $M.GEN.CVC_SEND.CONFIG.HFK.BYP_ML_TIER_LOW;
   Null = r4 - $cvc.tmm.mode.LOW;
   if Z jump tier_ml_set_param;

   // Moderate Tier
   r0 = r6 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_ML_GAIN_MODERATE;
   r1 = M[r6 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_SNDGAIN_MODERATE];
   r2 = r3 AND $M.GEN.CVC_SEND.CONFIG.HFK.BYP_ML_TIER_MODERATE;
   Null = r4 - $cvc.tmm.mode.MODERATE;
   if LE jump tier_ml_set_param;

   // High Tier
   r0 = r6 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_ML_GAIN_HIGH;
   r1 = M[r6 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_SNDGAIN_HIGH];
   r2 = r3 AND $M.GEN.CVC_SEND.CONFIG.HFK.BYP_ML_TIER_HIGH;
   Null = r4 - $cvc.tmm.mode.HIGH;
   if LE jump tier_ml_set_param;

   // Extreme Tier
   r0 = r6 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_ML_GAIN_EXTREME;
   r1 = M[r6 + $M.GEN.CVC_SEND.PARAMETERS.OFFSET_SNDGAIN_EXTREME];
   r2 = r3 AND $M.GEN.CVC_SEND.CONFIG.HFK.BYP_ML_TIER_EXTREME;

   tier_ml_set_param:
   // Hybrid-ML stream gain
   M[r8 + $cvc.tmm.STREAM_GAIN] = r1;

   push rLink;
   // Copy first 3 MLFE params from curparams to TMM MLFE params
   M3 = 3;
   r1 = r0;
   r0 = r8 + $cvc.tmm.PARAM_MLFE;
   call $cvc.copy;
   pop rLink;

   // MLFE bypass setting
   r0 = 1;
   M[r7 + $mlfe100_library_c.MLFE100_STRUC_struct.BYPASS_FLAG_FIELD] = r2;
   if NZ r0 = 0;
   // G_Smooth_on: NS-ext
   r2 = M[r9 + $cvc_send.data.dms200_obj];
   M[r2 + $dms200.BYPASS_GSMOOTH_FIELD] = r0;
   // G_Smooth_on: NS-int
   r2 = M[r8 + $cvc.tmm.NS_INT_OBJ];
   if Z rts;
   M[r2 + $dms200.BYPASS_GSMOOTH_FIELD] = r0;

   rts;

#else

$cvc.tmm.ml_init:
$cvc.tmm.ml_proc:
   rts;

#endif

.ENDMODULE;
