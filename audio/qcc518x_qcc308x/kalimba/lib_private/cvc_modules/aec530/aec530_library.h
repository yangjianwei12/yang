// *****************************************************************************
// Copyright (c) 2007 - 2017 Qualcomm Technologies International, Ltd.
//
// %%version
//
// *****************************************************************************

#ifndef AEC530_LIB_H_INCLUDED
#define AEC530_LIB_H_INCLUDED

// *****************************************************************************
// AEC algorithm matlab version 510
//
// VERSION HISTORY:
//    1.0.0 - initial version aec500
//    2.0.0 - aec510: run-time HS/HF/CE configurable
//    3.0.0 - aec510: hdv
//    3.1.0 - aec520: hdv + va
//    3.2.0 - aec530: in-ear
// *****************************************************************************

// -----------------------------------------------------------------------------
// AEC530 external user constants
// -----------------------------------------------------------------------------
#define $aec530.HF_FLAG_HS    0
#define $aec530.HF_FLAG_HF    1
#define $aec530.HF_FLAG_CE    2
#define $aec530.HF_FLAG_VA    3

.CONST $aec530.HS.Num_Primary_Taps           2;
.CONST $aec530.HS.Num_Auxillary_Taps         1;

.CONST $aec530_HF.Num_Auxillary_Taps         3;
.CONST $aec530_HF.Num_Primary_Taps           8;

.CONST $aec530_HF_UWB.Num_Auxillary_Taps     4;
.CONST $aec530_HF_UWB.Num_Primary_Taps       12;
.CONST $aec530.HS_UWB.Num_Primary_Taps       3;
.CONST $aec530.HS_UWB.Num_Auxillary_Taps     1;

.CONST $aec530.RER_DIM                       64;
.CONST $aec530.RER_DIM.UWB                   43;
.CONST $aec530.RER_VEC                       5; // SqGr,L2absGr,LPwrD,Gr(real,imag)

// -----------------------------------------------------------------------------
// AEC530 user parameter structure
// -----------------------------------------------------------------------------

// AEC LMS filter upper frequency
.CONST $aec530.Parameter.OFFSET_LMS_FREQ              MK1 * 0;

// tail-length in seconds, Q7.17 (arch4: Q7.25)
.CONST $aec530.Parameter.TAIL_LENGTH                  MK1 * 1;

// Flag for repeating AEC filtering
.CONST $aec530.Parameter.OFFSET_ENABLE_AEC_REUSE      MK1 * 2;

// Reference Power Threshold. Default set to '$aec530.AEC_L2Px_HB' (Q8.16)
.CONST $aec530.Parameter.OFFSET_AEC_REF_LPWR_HB       MK1 * 3;

// DTC aggressiveness, default 0.5 (Q1.23) (CVC parameter)
.CONST $aec530.Parameter.OFFSET_DTC_AGRESSIVENESS     MK1 * 4;

// Maximum Power Margin, default 2.5 in (Q8.16) (CVC parameter for handsfree), Q8.16 (arch4: Q8.24)
.CONST $aec530.Parameter.OFFSET_MAX_LPWR_MARGIN       MK1 * 5;

// Pre-AEC noise estimator control, Q1.N
.CONST $aec530.Parameter.OFFSET_AEC_NS_CNTRL          MK1 * 6;

// Handsfree only. RER Adaptation. Default 0 (Handsfree CVC parameter), Q1.23 (arch4: Q1.31)
.CONST $aec530.Parameter.OFFSET_RER_ADAPT             MK1 * 7;
// Handsfree only. RER aggresiveness. Default 0x200000 (Q6.18) (Handsfree CVC parameter), Q6.18 (arch4: Q6.26)
.CONST $aec530.Parameter.OFFSET_RER_AGGRESSIVENESS    MK1 * 8;
// Handsfree only. RER power. Default 2 (Handsfree CVC parameter), Integer
.CONST $aec530.Parameter.OFFSET_RER_POWER             MK1 * 9;

// Threshold for RERDT DTC decision, Q8.16
.CONST $aec530.Parameter.OFFSET_L2TH_RERDT_OFF        MK1 * 10;
// RERDT aggressiveness, Q6.18
.CONST $aec530.Parameter.OFFSET_RERDT_ADJUST          MK1 * 11;
// Handsfree only. RERDT power. Integer
.CONST $aec530.Parameter.OFFSET_RERDT_POWER           MK1 * 12;

// HDV mode gain control, Integer: 0 - 5
.CONST $aec530.Parameter.OFFSET_HDV_GAIN_CNTRL        MK1 * 13;

// Multi-channel AEC LRM mode, Integer, 0: LRM off, 1: LRM on
.CONST $aec530.Parameter.OFFSET_AEC_LRM_MODE          MK1 * 14;

// CND gain, default 1.0 in Q3.21 (CVC parameter)
.CONST $aec530.Parameter.OFFSET_CNG_Q_ADJUST          MK1 * 15;

// Flag: Comfort noise color selection -1=wht,0=brn,1=pnk,2=blu,3=pur (CVC parameter)
.CONST $aec530.Parameter.OFFSET_CNG_NOISE_COLOR       MK1 * 16;

// Reference delay in seconds Q7.17 (arch4: Q7.25)
.CONST $aec530.Parameter.REF_DELAY                    MK1 * 17;


// -----------------------------------------------------------------------------
// AEC530 data object structure
// -----------------------------------------------------------------------------

// @DATA_OBJECT AECDATAOBJECT

// @DOC_FIELD_TEXT AEC table:
// @DOC_FIELD_TEXT    - $aec530.const     (NB/WB/SWB)
// @DOC_FIELD_TEXT    - $aec530.const_uwb (UWB/FB)
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.PTR_CONST_FIELD               0*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to AEC Parameters
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.PARAM_FIELD                   1*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to Variant Flag
// @DOC_FIELD_FORMAT Flag Pointer
.CONST $aec530.VARIANT_FIELD                 2*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Maximum Primary Filter Taps
// @DOC_FIELD_FORMAT Integer
.CONST $aec530.MAX_FILTER_LENGTH_FIELD       3*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Handsfree flag
// @DOC_FIELD_TEXT Headset use case if cleared
// @DOC_FIELD_TEXT Handsfree use case if set
// @DOC_FIELD_FORMAT Flag
.CONST $aec530.HF_FLAG_FIELD                 4*ADDR_PER_WORD;

// @DOC_FIELD_TEXT DTC Enhancement Flag
// @DOC_FIELD_FORMAT Integer
.CONST $aec530.FLAG_DTC_ENH                  5*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to pre-AEC OMS G
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.OMS_G_FIELD                   6*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to pre-AEC OMS MS_LpN
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.OMS_LPN_FIELD                 7*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to AEC reference input stream
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.X_STREAM_FIELD                8*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to AEC reference delayed stream
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.X_STREAM_DELAY_FIELD          9*ADDR_PER_WORD;


// @DOC_FIELD_TEXT Pointer to AEC reference channel X (real/imag/BExp)
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.X_FIELD                       10*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to real part of receive buffer X_buf, size of 'Num_FFT_Freq_Bins*Num_Primary_Taps'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.XBUF_REAL_FIELD               11*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to imaginary part of receive buffer X_buf, size of 'Num_FFT_Freq_Bins*Num_Primary_Taps'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.XBUF_IMAG_FIELD               12*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to BExp_X_buf (internal array, permanant), size of 'Num_Primary_Taps+1'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.XBUF_BEXP_FIELD               13*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to left channel FBC object
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.PTR_FBC_OBJ_FIELD             14*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to AEC (left) channel D (real/imag/BExp)
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.D_FIELD                       15*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to real part of Ga, size of 'Num_FFT_Freq_Bins*Num_Primary_Taps'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.GA_REAL_FIELD                 16*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to imaginary part of Ga, size of 'Num_FFT_Freq_Bins*Num_Primary_Taps'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.GA_IMAG_FIELD                 17*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to BExp_Ga (internal array, permanant), size of Num_FFT_Freq_Bins
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.GA_BEXP_FIELD                 18*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to second channel AEC object
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.DM_OBJ_FIELD                  19*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to LPwrX0 (internal array, permanant), size of Num_FFT_Freq_Bins
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.LPWRX0_FIELD                  20*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to LPwrX1 (internal array, permanant), size of Num_FFT_Freq_Bins
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.LPWRX1_FIELD                  21*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to RatFE (internal array, permanant), size of RER_dim
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.RATFE_FIELD                   22*ADDR_PER_WORD;

// Pointer to RER state memory
.CONST $aec530.RER_STATE_FIELD               23*ADDR_PER_WORD;

// Pointer to CNG sate memory
.CONST $aec530.CNG_STATE_FIELD               24*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to L_adaptA (internal array, scratch in DM1), size of Num_FFT_Freq_Bins
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.SCRPTR_LADAPTA_FIELD          25*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to a scratch memory in DM2 with size of '2*$M.CVC.Num_FFT_Freq_Bins + 1'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.SCRPTR_EXP_MTS_ADAPT_FIELD    26*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to Attenuation (internal array, scratch in DM1), size of Num_FFT_Freq_Bins
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.SCRPTR_ATTENUATION_FIELD      27*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to W_ri (RER internal interleaved complex array)
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.SCRPTR_W_RI_FIELD             28*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to L_adaptR (RER internal real array)
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.SCRPTR_LADAPTR_FIELD          29*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to DTC_lin array, size of Num_FFT_Freq_Bins
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.SCRPTR_DTC_LIN_FIELD          30*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Scratch pointer to channel structure T
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.SCRPTR_T_FIELD                31*ADDR_PER_WORD;

// @DOC_FIELD_TEXT DTC status array for each frequency bins, scratch
// @DOC_FIELD_TEXT Size of Number of FFT bins
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.SCRPTR_RERDT_DTC_FIELD        32*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to real part of Gb, size of 'RER_dim*Num_Auxillary_Taps'
// @DOC_FIELD_TEXT Handsfree only. For headset set to '0'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.GB_REAL_FIELD                 33*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to imaginary part of Gb, size of 'RER_dim*Num_Auxillary_Taps'
// @DOC_FIELD_TEXT Handsfree only. For headset set to '0'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.GB_IMAG_FIELD                 34*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to BExp_Gb (internal array, permanant), size of RER_dim
// @DOC_FIELD_TEXT Handsfree only. For headset set to '0'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.GB_BEXP_FIELD                 35*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to L_RatSqG (internal array, permanant), size of RER_dim
// @DOC_FIELD_TEXT Handsfree only. For headset set to '0'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.L_RATSQG_FIELD                36*ADDR_PER_WORD;

// Internal AEC Data - Variables

// Time/Frequency: NB/WB/UWB/SWB/FB
.CONST $aec530.FRAME_SIZE_FIELD              $aec530.L_RATSQG_FIELD + ADDR_PER_WORD;
.CONST $aec530.REF_DELAY_FIELD               $aec530.FRAME_SIZE_FIELD + ADDR_PER_WORD;
.CONST $aec530.TAIL_LENGTH                   $aec530.REF_DELAY_FIELD + ADDR_PER_WORD;

.CONST $aec530.LMS_MAX_FREQ                  $aec530.TAIL_LENGTH + ADDR_PER_WORD;
.CONST $aec530.OFFSET_NUM_FREQ_BINS          $aec530.LMS_MAX_FREQ + ADDR_PER_WORD;
.CONST $aec530.LMS_dim                       $aec530.OFFSET_NUM_FREQ_BINS + ADDR_PER_WORD;
.CONST $aec530.DTC_dim                       $aec530.LMS_dim + ADDR_PER_WORD;
.CONST $aec530.RER_dim                       $aec530.DTC_dim + ADDR_PER_WORD;
.CONST $aec530.AEC_hold_dL2Px                $aec530.RER_dim + ADDR_PER_WORD;
.CONST $aec530.AEC_hold_adapt                $aec530.AEC_hold_dL2Px + ADDR_PER_WORD;
.CONST $aec530.AEC_kL_FBpwr                  $aec530.AEC_hold_adapt + ADDR_PER_WORD;
.CONST $aec530.AEC_kH_FBpwr                  $aec530.AEC_kL_FBpwr + ADDR_PER_WORD;
.CONST $aec530.DTC_kL_ref                    $aec530.AEC_kH_FBpwr + ADDR_PER_WORD;
.CONST $aec530.DTC_kH_ref                    $aec530.DTC_kL_ref + ADDR_PER_WORD;
.CONST $aec530.DTC_scale_dLpX                $aec530.DTC_kH_ref + ADDR_PER_WORD;
.CONST $aec530.L_dtc                         $aec530.DTC_scale_dLpX + ADDR_PER_WORD;
.CONST $aec530.alfaSQG                       $aec530.L_dtc + ADDR_PER_WORD;
.CONST $aec530.kL_rerdt                      $aec530.alfaSQG + ADDR_PER_WORD;
.CONST $aec530.kH_rerdt                      $aec530.kL_rerdt + ADDR_PER_WORD;
.CONST $aec530.alfa_rerdt                    $aec530.kH_rerdt + ADDR_PER_WORD;
.CONST $aec530.kL_GrTilt                     $aec530.alfa_rerdt + ADDR_PER_WORD;
.CONST $aec530.kH_GrTilt                     $aec530.kL_GrTilt + ADDR_PER_WORD;
.CONST $aec530.HD_kL_Gain                    $aec530.kH_GrTilt + ADDR_PER_WORD;
.CONST $aec530.HD_kH_Gain                    $aec530.HD_kL_Gain + ADDR_PER_WORD;
.CONST $aec530.HD_alfa_Gain                  $aec530.HD_kH_Gain + ADDR_PER_WORD;

// HS/HF
.CONST $aec530.OFFSET_NUM_PRIMARY_TAPS       $aec530.HD_alfa_Gain + ADDR_PER_WORD;
.CONST $aec530.OFFSET_NUM_AUXILLARY_TAPS     $aec530.OFFSET_NUM_PRIMARY_TAPS + ADDR_PER_WORD;
.CONST $aec530.OFFSET_AEC_L_MUA_ON           $aec530.OFFSET_NUM_AUXILLARY_TAPS + ADDR_PER_WORD;
.CONST $aec530.OFFSET_AEC_L_MUB_ON           $aec530.OFFSET_AEC_L_MUA_ON + ADDR_PER_WORD;
.CONST $aec530.OFFSET_AEC_ALFA_A             $aec530.OFFSET_AEC_L_MUB_ON + ADDR_PER_WORD;
.CONST $aec530.OFFSET_AEC_L_ALFA_A           $aec530.OFFSET_AEC_ALFA_A + ADDR_PER_WORD;

// PREP
.CONST $aec530.G_OMS_IN2_FIELD               $aec530.OFFSET_AEC_L_ALFA_A + ADDR_PER_WORD;
.CONST $aec530.L2PXT_FIELD                   $aec530.G_OMS_IN2_FIELD + ADDR_PER_WORD;
.CONST $aec530.L2PDT_FIELD                   $aec530.L2PXT_FIELD + ADDR_PER_WORD;

// DTC
.CONST $aec530.L_MUA_FIELD                   $aec530.L2PDT_FIELD + ADDR_PER_WORD;
.CONST $aec530.L_MUB_FIELD                   $aec530.L_MUA_FIELD + ADDR_PER_WORD;
.CONST $aec530.L_DTC_HFREQ_FEF_FIELD         $aec530.L_MUB_FIELD + ADDR_PER_WORD;
.CONST $aec530.DTC_AVG_FIELD                 $aec530.L_DTC_HFREQ_FEF_FIELD + ADDR_PER_WORD;
.CONST $aec530.DTC_PROB_FIELD                $aec530.DTC_AVG_FIELD + ADDR_PER_WORD;
.CONST $aec530.DTC_AVGRFE_FIELD              $aec530.DTC_PROB_FIELD + ADDR_PER_WORD;
.CONST $aec530.DTC_STDRFE_FIELD              $aec530.DTC_AVGRFE_FIELD + ADDR_PER_WORD;
.CONST $aec530.mn_L_RatSqGt                  $aec530.DTC_STDRFE_FIELD + ADDR_PER_WORD;

.CONST $aec530.OFFSET_L_RatSqG               $aec530.mn_L_RatSqGt + ADDR_PER_WORD;
.CONST $aec530.OFFSET_dL2PxFB                $aec530.OFFSET_L_RatSqG + ADDR_PER_WORD;
.CONST $aec530.OFFSET_L2Pxt0                 $aec530.OFFSET_dL2PxFB + ADDR_PER_WORD;
.CONST $aec530.DTC_dLpX                      $aec530.OFFSET_L2Pxt0 + ADDR_PER_WORD;
.CONST $aec530.DTC_LpXt_curr                 $aec530.DTC_dLpX + ADDR_PER_WORD;
.CONST $aec530.DTC_LpXt_prev                 $aec530.DTC_LpXt_curr + ADDR_PER_WORD;

.CONST $aec530.OFFSET_tt_dtc                 $aec530.DTC_LpXt_prev + ADDR_PER_WORD;
.CONST $aec530.OFFSET_ct_init                $aec530.OFFSET_tt_dtc + ADDR_PER_WORD;
.CONST $aec530.OFFSET_ct_Px                  $aec530.OFFSET_ct_init + ADDR_PER_WORD;
.CONST $aec530.FUNC_DTC_USER                 $aec530.OFFSET_ct_Px + ADDR_PER_WORD;

// RER/RERDT
.CONST $aec530.RER_E_FIELD                   $aec530.FUNC_DTC_USER + ADDR_PER_WORD;
.CONST $aec530.OFFSET_LPXFB_RERDT            $aec530.RER_E_FIELD + ADDR_PER_WORD;
.CONST $aec530.RERDT_DTC_ACTIVE_FIELD        $aec530.OFFSET_LPXFB_RERDT + ADDR_PER_WORD;

// CNG/MAP
.CONST $aec530.OFFSET_OMS_AGGRESSIVENESS     $aec530.RERDT_DTC_ACTIVE_FIELD + ADDR_PER_WORD;

// HD
.CONST $aec530.OFFSET_AEC_COUPLING           $aec530.OFFSET_OMS_AGGRESSIVENESS + ADDR_PER_WORD;
.CONST $aec530.OFFSET_HD_L_AECgain           $aec530.OFFSET_AEC_COUPLING + ADDR_PER_WORD;

// Local scratch variables
.CONST $aec530.LPWRX_MARGIN_FIELD            $aec530.OFFSET_HD_L_AECgain + ADDR_PER_WORD;
.CONST $aec530.MN_PWRX_DIFF_FIELD            $aec530.LPWRX_MARGIN_FIELD + ADDR_PER_WORD;
.CONST $aec530.OFFSET_TEMP_FIELD             $aec530.MN_PWRX_DIFF_FIELD + ADDR_PER_WORD;

// Sub-module control
.CONST $aec530.FLAG_AEC_HDMODE_FIELD         $aec530.OFFSET_TEMP_FIELD + ADDR_PER_WORD;
.CONST $aec530.FLAG_BYPASS_CNG_FIELD         $aec530.FLAG_AEC_HDMODE_FIELD    + ADDR_PER_WORD;
.CONST $aec530.FLAG_BYPASS_RER_FIELD         $aec530.FLAG_BYPASS_CNG_FIELD + ADDR_PER_WORD;
.CONST $aec530.FLAG_BYPASS_RERDT_FIELD       $aec530.FLAG_BYPASS_RER_FIELD + ADDR_PER_WORD;
.CONST $aec530.FLAG_BYPASS_FBC_FIELD         $aec530.FLAG_BYPASS_RERDT_FIELD + ADDR_PER_WORD;
.CONST $aec530.FLAG_BYPASS_AUX_FIELD         $aec530.FLAG_BYPASS_FBC_FIELD + ADDR_PER_WORD;

.CONST $aec530.STRUCT_SIZE.LMS              ($aec530.FLAG_BYPASS_AUX_FIELD >> LOG2_ADDR_PER_WORD) + 1;

// RER
.CONST $aec530.RER_START_FILED               $aec530.STRUCT_SIZE.LMS * MK1;
.CONST $aec530.rer.AGGR_FIELD                $aec530.RER_START_FILED + MK1 * 0;
.CONST $aec530.rer.kL_ref                    $aec530.RER_START_FILED + MK1 * 1;
.CONST $aec530.rer.kH_ref                    $aec530.RER_START_FILED + MK1 * 2;
.CONST $aec530.rer.alfaL2P_Rs                $aec530.RER_START_FILED + MK1 * 3;
.CONST $aec530.rer.inv_L_L2P_Rd              $aec530.RER_START_FILED + MK1 * 4;

.CONST $aec530.rer.L2ABSGR_FIELD             $aec530.RER_START_FILED + MK1 * 5;
.CONST $aec530.rer.LPWRD_FIELD               $aec530.RER_START_FILED + MK1 * 6;
.CONST $aec530.rer.GR_REAL_FIELD             $aec530.RER_START_FILED + MK1 * 7;
.CONST $aec530.rer.GR_IMAG_FIELD             $aec530.RER_START_FILED + MK1 * 8;
.CONST $aec530.rer.SQGR_REVERED_FIELD        $aec530.RER_START_FILED + MK1 * 9;
.CONST $aec530.rer.ET_REAL_FIELD             $aec530.RER_START_FILED + MK1 * 10;
.CONST $aec530.rer.ET_IMAG_FIELD             $aec530.RER_START_FILED + MK1 * 11;

.CONST $aec530.rer.BEXP_FIELD                $aec530.RER_START_FILED + MK1 * 12;
.CONST $aec530.rer.L2PET_FIELD               $aec530.RER_START_FILED + MK1 * 13;
.CONST $aec530.rer.L2PxRs                    $aec530.RER_START_FILED + MK1 * 14;
.CONST $aec530.rer.L2PxRd                    $aec530.RER_START_FILED + MK1 * 15;
.CONST $aec530.rer.L2PdRs                    $aec530.RER_START_FILED + MK1 * 16;
.CONST $aec530.rer.L2PdRd                    $aec530.RER_START_FILED + MK1 * 17;
.CONST $aec530.rer.GrOffset                  $aec530.RER_START_FILED + MK1 * 18;
.CONST $aec530.rer.HBGAIN_FIELD              $aec530.RER_START_FILED + MK1 * 19;

.CONST $aec530.STRUCT_SIZE.LMS_RER          ($aec530.rer.HBGAIN_FIELD >> LOG2_ADDR_PER_WORD) + 1;

// CNG
.CONST $aec530.CNG_START_FILED               $aec530.STRUCT_SIZE.LMS_RER * MK1;
.CONST $aec530.cng.CNG_offset                $aec530.CNG_START_FILED + MK1 * 0;
.CONST $aec530.cng.alfaCNG                   $aec530.CNG_START_FILED + MK1 * 1;
.CONST $aec530.cng.LB_cng                    $aec530.CNG_START_FILED + MK1 * 2;
.CONST $aec530.cng.HB_cng                    $aec530.CNG_START_FILED + MK1 * 3;
.CONST $aec530.cng.tt_cng                    $aec530.CNG_START_FILED + MK1 * 4;
.CONST $aec530.cng.LPZNZ_FIELD               $aec530.CNG_START_FILED + MK1 * 5;
.CONST $aec530.cng.Q_ADJUST                  $aec530.CNG_START_FILED + MK1 * 6;
.CONST $aec530.cng.CUR_NZ_TABLE_FIELD        $aec530.CNG_START_FILED + MK1 * 7;

// AEC530 object size
.CONST $aec530.STRUCT_SIZE.FULL             ($aec530.cng.LPZNZ_FIELD >> LOG2_ADDR_PER_WORD) + 1;
.CONST $aec530.STRUCT_SIZE.FULL.DL          ($aec530.cng.CUR_NZ_TABLE_FIELD >> LOG2_ADDR_PER_WORD) + 1;

// @END  DATA_OBJECT AECDATAOBJECT


// -----------------------------------------------------------------------------
// AEC530 FNLMS channel data object structure
// -----------------------------------------------------------------------------

// Pointer to AEC FNLMS channel (D)(real/imag/BExp)
.CONST $aec530.fnlms.D_FIELD                 0*ADDR_PER_WORD;

// Pointer to real part of LMS G, size of 'LMS_dim*N_Taps'
.CONST $aec530.fnlms.G_REAL_FIELD            1*ADDR_PER_WORD;

// Pointer to imaginary part of LMS G, size of 'LMS_dim*N_Taps'
.CONST $aec530.fnlms.G_IMAG_FIELD            2*ADDR_PER_WORD;

// Pointer to BExp_G (internal array, permanant), size of 'LMS_dim'
.CONST $aec530.fnlms.G_BEXP_FIELD            3*ADDR_PER_WORD;

// Pointer to FBC object
.CONST $aec530.fnlms.PTR_FBC_OBJ_FIELD       4*ADDR_PER_WORD;

// Start of next channel
// If 0, no more channel, otherwise repeat fnlsm structure
.CONST $aec530.fnlms.LRM_NEXT_FIELD          1*ADDR_PER_WORD;
.CONST $aec530.fnlms.NEXT_FIELD              5*ADDR_PER_WORD;


// -----------------------------------------------------------------------------
// AEC530 dual microphone (second channel) data object structure
// -----------------------------------------------------------------------------

// @DATA_OBJECT AECDM_DATAOBJECT

// @DOC_FIELD_TEXT Pointer to external microphone mode
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.dm.PTR_MIC_MODE_FIELD         0*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to rightt channel FBC object
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.dm.FLAG_AEC_LRM_FIELD         1*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to AEC (right) channel (D1)(real/imag/BExp)
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.dm.D1_FIELD                   2*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to real part of Ga1, size of 'Num_FFT_Freq_Bins*Num_Primary_Taps'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.dm.GA1_REAL_FIELD             3*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to imaginary part of Ga1, size of 'Num_FFT_Freq_Bins*Num_Primary_Taps'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.dm.GA1_IMAG_FIELD             4*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to BExp_Ga1 (internal array, permanant), size of Num_FFT_Freq_Bins
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.dm.GA1_BEXP_FIELD             5*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to rightt channel FBC object
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.dm.PTR_FBC1_OBJ_FIELD         6*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to AEC next (3rd) channel start
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.dm.NEXT_CHANNEL_FIELD         7*ADDR_PER_WORD;

// @END  DATA_OBJECT AECDM_DATAOBJECT

// -----------------------------------------------------------------------------
// AEC530 FDNLP/VSM sub-module object structure
// -----------------------------------------------------------------------------
// FDNLP - (Handsfree)
.CONST $aec530.fdnlp.OFFSET_VSM_HB              0*ADDR_PER_WORD;
.CONST $aec530.fdnlp.OFFSET_VSM_LB              $aec530.fdnlp.OFFSET_VSM_HB + ADDR_PER_WORD;
.CONST $aec530.fdnlp.OFFSET_VSM_MAX_ATT         $aec530.fdnlp.OFFSET_VSM_LB + ADDR_PER_WORD;
.CONST $aec530.fdnlp.OFFSET_FDNLP_HB            $aec530.fdnlp.OFFSET_VSM_MAX_ATT + ADDR_PER_WORD;
.CONST $aec530.fdnlp.OFFSET_FDNLP_LB            $aec530.fdnlp.OFFSET_FDNLP_HB + ADDR_PER_WORD;
.CONST $aec530.fdnlp.OFFSET_FDNLP_MB            $aec530.fdnlp.OFFSET_FDNLP_LB + ADDR_PER_WORD;
.CONST $aec530.fdnlp.OFFSET_FDNLP_NBINS         $aec530.fdnlp.OFFSET_FDNLP_MB + ADDR_PER_WORD;
.CONST $aec530.fdnlp.OFFSET_FDNLP_ATT           $aec530.fdnlp.OFFSET_FDNLP_NBINS + ADDR_PER_WORD;
.CONST $aec530.fdnlp.OFFSET_FDNLP_ATT_THRESH    $aec530.fdnlp.OFFSET_FDNLP_ATT + ADDR_PER_WORD;
.CONST $aec530.fdnlp.OFFSET_FDNLP_ECHO_THRESH   $aec530.fdnlp.OFFSET_FDNLP_ATT_THRESH + ADDR_PER_WORD;
.CONST $aec530.fdnlp.STRUCT_SIZE                ($aec530.fdnlp.OFFSET_FDNLP_ECHO_THRESH >> LOG2_ADDR_PER_WORD) + 1;


// -----------------------------------------------------------------------------
// AEC530 NLP user parameter structure
// -----------------------------------------------------------------------------

.CONST $aec530.nlp.Parameter.OFFSET_HD_THRESH_GAIN           0*ADDR_PER_WORD;
.CONST $aec530.nlp.Parameter.OFFSET_TIER2_THRESH             1*ADDR_PER_WORD;
.CONST $aec530.nlp.Parameter.OFFSET_TIER0_CONFIG             -1;
.CONST $aec530.nlp.Parameter.OFFSET_TIER1_CONFIG             $aec530.nlp.Parameter.OFFSET_TIER2_THRESH + ADDR_PER_WORD;
.CONST $aec530.nlp.Parameter.OFFSET_TIER2_CONFIG             $aec530.nlp.Parameter.OFFSET_TIER1_CONFIG + $aec530.fdnlp.STRUCT_SIZE*ADDR_PER_WORD;

.CONST $aec530.nlp.Parameter.HF_OBJECT_SIZE                  ($aec530.nlp.Parameter.OFFSET_TIER2_CONFIG >> LOG2_ADDR_PER_WORD) + $aec530.fdnlp.STRUCT_SIZE;
.CONST $aec530.nlp.Parameter.HS_OBJECT_SIZE                  2;


// -----------------------------------------------------------------------------
// AEC530 NLP object structure
// -----------------------------------------------------------------------------

// @DATA_OBJECT NLPDATAOBJECT

// @DOC_FIELD_TEXT Pointer to AEC master object
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.nlp.AEC_OBJ_PTR                  0*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to Non-Linear Processing Parameters
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.nlp.OFFSET_PARAM_PTR             1*ADDR_PER_WORD;

// FDNLP - VSM
// @DOC_FIELD_TEXT Pointer to current system call state flag
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.nlp.OFFSET_CALLSTATE_PTR         2*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to receive path signal VAD flag
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.nlp.OFFSET_PTR_RCV_DETECT        3*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to Attenuation, same array as used in AEC main object
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.nlp.OFFSET_SCRPTR_Attenuation    4*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to scratch memory with size of Num_FFT_Freq_Bins + RER_dim
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.nlp.OFFSET_SCRPTR                5*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Function pointer for FDNLP
// @DOC_FIELD_TEXT To enable: set '$aec530.FdnlpProcess'
// @DOC_FIELD_TEXT To disable: set '0'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.nlp.FDNLP_FUNCPTR                6*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Function pointer for VSM
// @DOC_FIELD_TEXT To enable: set '$aec530.VsmProcess'
// @DOC_FIELD_TEXT To disable: set '0'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec530.nlp.VSM_FUNCPTR                  7*ADDR_PER_WORD;

// SP.  Internal FNDLP Data
.CONST $aec530.nlp.OFFSET_PTR_RatFE             $aec530.nlp.VSM_FUNCPTR + ADDR_PER_WORD;
.CONST $aec530.nlp.OFFSET_SCRPTR_absGr          $aec530.nlp.OFFSET_PTR_RatFE + ADDR_PER_WORD;
.CONST $aec530.nlp.OFFSET_SCRPTR_temp           $aec530.nlp.OFFSET_SCRPTR_absGr + ADDR_PER_WORD;
.CONST $aec530.nlp.OFFSET_CUR_TIER              $aec530.nlp.OFFSET_SCRPTR_temp + ADDR_PER_WORD;
.CONST $aec530.nlp.OFFSET_PTR_CUR_CONFIG        $aec530.nlp.OFFSET_CUR_TIER + ADDR_PER_WORD;
.CONST $aec530.nlp.OFFSET_hd_ct_hold            $aec530.nlp.OFFSET_PTR_CUR_CONFIG + $aec530.fdnlp.STRUCT_SIZE*ADDR_PER_WORD;
.CONST $aec530.nlp.OFFSET_hd_att                $aec530.nlp.OFFSET_hd_ct_hold + ADDR_PER_WORD;
.CONST $aec530.nlp.OFFSET_G_vsm                 $aec530.nlp.OFFSET_hd_att + ADDR_PER_WORD;
.CONST $aec530.nlp.OFFSET_fdnlp_cont_test       $aec530.nlp.OFFSET_G_vsm + ADDR_PER_WORD;
.CONST $aec530.nlp.OFFSET_mean_len              $aec530.nlp.OFFSET_fdnlp_cont_test + ADDR_PER_WORD;
.CONST $aec530.nlp.OFFSET_avg_RatFE             $aec530.nlp.OFFSET_mean_len + ADDR_PER_WORD;
.CONST $aec530.nlp.OFFSET_Vad_ct_burst          $aec530.nlp.OFFSET_avg_RatFE + ADDR_PER_WORD;
.CONST $aec530.nlp.OFFSET_Vad_ct_hang           $aec530.nlp.OFFSET_Vad_ct_burst + ADDR_PER_WORD; // must follow ct_burst

.CONST $aec530.nlp.FLAG_BYPASS_HD_FIELD         $aec530.nlp.OFFSET_Vad_ct_hang + ADDR_PER_WORD;
.CONST $aec530.nlp.OFFSET_HC_TIER_STATE         $aec530.nlp.FLAG_BYPASS_HD_FIELD + ADDR_PER_WORD;
.CONST $aec530.nlp.FLAG_HD_MODE_FIELD           $aec530.nlp.OFFSET_HC_TIER_STATE + ADDR_PER_WORD;
.CONST $aec530.nlp.RER_dim                      $aec530.nlp.FLAG_HD_MODE_FIELD + ADDR_PER_WORD;
.CONST $aec530.nlp.OFFSET_NUM_FREQ_BINS         $aec530.nlp.RER_dim + ADDR_PER_WORD;
.CONST $aec530.nlp.OFFSET_D_REAL_PTR            $aec530.nlp.OFFSET_NUM_FREQ_BINS + ADDR_PER_WORD;
.CONST $aec530.nlp.OFFSET_D_IMAG_PTR            $aec530.nlp.OFFSET_D_REAL_PTR + ADDR_PER_WORD;

// NB/WB/UWB/SWB/FB
.CONST $aec530.nlp.VAD_Th_hang                  $aec530.nlp.OFFSET_D_IMAG_PTR + ADDR_PER_WORD;
.CONST $aec530.nlp.VAD_Th_burst                 $aec530.nlp.VAD_Th_hang + ADDR_PER_WORD;
.CONST $aec530.nlp.HD_Th_hold                   $aec530.nlp.VAD_Th_burst + ADDR_PER_WORD;
.CONST $aec530.nlp.FDNLP_Th_cont                $aec530.nlp.HD_Th_hold + ADDR_PER_WORD;

.CONST $aec530.nlp.STRUCT_SIZE                  ($aec530.nlp.FDNLP_Th_cont >> LOG2_ADDR_PER_WORD) + 1;

// @END  DATA_OBJECT NLPDATAOBJECT


//------------------------------------------------------------------------------
// Constants used in FDFBC
//------------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// FDFBC data object structure
// -----------------------------------------------------------------------------

// @DATA_OBJECT FBCDATAOBJECT

// @DOC_FIELD_TEXT Pointer to mic (D) input stream map
// @DOC_FIELD_FORMAT Pointer
.CONST $fdfbc.D_STREAM_PTR_FIELD                                0*ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to reference (X) input stream map
// @DOC_FIELD_FORMAT Pointer
.CONST $fdfbc.X_STREAM_PTR_FIELD                                $fdfbc.D_STREAM_PTR_FIELD + ADDR_PER_WORD;

// @DOC_FIELD_TEXT Pointer to frequency domain object of the pure mic inputs (before FDFBC filtering)
// @DOC_FIELD_FORMAT Pointer
.CONST $fdfbc.DD_FIELD                                          $fdfbc.X_STREAM_PTR_FIELD + ADDR_PER_WORD;

// @DOC_FIELD_TEXT Tail length that the FDFBC covers
// @DOC_FIELD_FORMAT Fractional
.CONST $fdfbc.TAIL_LENGTH_FIELD                                 $fdfbc.DD_FIELD + ADDR_PER_WORD;

// @DOC_FIELD_TEXT Threshold to perform copy of coefficients
// @DOC_FIELD_FORMAT Integer
.CONST $fdfbc.COPY_COEFF_THRESHOLD_FIELD                        $fdfbc.TAIL_LENGTH_FIELD + ADDR_PER_WORD;

// @DOC_FIELD_TEXT User parameter that determines FBC aggressiveness/percentage appplied
// @DOC_FIELD_FORMAT Fractional
.CONST $fdfbc.PERC_PTR_FIELD                                    $fdfbc.COPY_COEFF_THRESHOLD_FIELD + ADDR_PER_WORD;

// @DOC_FIELD_TEXT when far-end is active and echo suppression is insufficient, forcely attenuate FBC output
// @DOC_FIELD_FORMAT Flag
.CONST $fdfbc.ATTENUATION_ON_FIELD                              $fdfbc.PERC_PTR_FIELD + ADDR_PER_WORD;

// @DOC_FIELD_TEXT Copy of X to be used in FDFBC
// @DOC_FIELD_FORMAT pointer
.CONST $fdfbc.X_FDFBC_FIELD                                     $fdfbc.ATTENUATION_ON_FIELD + ADDR_PER_WORD;

//Internal fields
.CONST $fdfbc.G_TIME_DOMAIN_FIX                                 $fdfbc.X_FDFBC_FIELD + ADDR_PER_WORD;

// G_FIX frequency domain buffer
.CONST $fdfbc.G_FIX_REAL                                        $fdfbc.G_TIME_DOMAIN_FIX + ADDR_PER_WORD;
.CONST $fdfbc.G_FIX_IMAG                                        $fdfbc.G_FIX_REAL + ADDR_PER_WORD;
.CONST $fdfbc.G_FIX_BEXP                                        $fdfbc.G_FIX_IMAG + ADDR_PER_WORD;

.CONST $fdfbc.START_HISTORY_BUF                                 $fdfbc.G_FIX_BEXP + ADDR_PER_WORD;

.CONST $fdfbc.GFIX_ALIGN_REAL_SCRPTR                            $fdfbc.START_HISTORY_BUF +  ADDR_PER_WORD;

.CONST $fdfbc.GFIX_ALIGN_IMAG_SCRPTR                            $fdfbc.GFIX_ALIGN_REAL_SCRPTR +  ADDR_PER_WORD;

.CONST $fdfbc.GFIX_ALIGN_BEXP                                   $fdfbc.GFIX_ALIGN_IMAG_SCRPTR +  ADDR_PER_WORD;

.CONST $fdfbc.Y0_REAL_SCRPTR                                    $fdfbc.GFIX_ALIGN_BEXP +  ADDR_PER_WORD;

.CONST $fdfbc.Y0_IMAG_SCRPTR                                    $fdfbc.Y0_REAL_SCRPTR +  ADDR_PER_WORD;

.CONST $fdfbc.Y0_BEXP                                           $fdfbc.Y0_IMAG_SCRPTR +  ADDR_PER_WORD;

.CONST $fdfbc.Y1_REAL_SCRPTR                                    $fdfbc.Y0_BEXP +  ADDR_PER_WORD;

.CONST $fdfbc.Y1_IMAG_SCRPTR                                    $fdfbc.Y1_REAL_SCRPTR +  ADDR_PER_WORD;

.CONST $fdfbc.Y1_BEXP                                           $fdfbc.Y1_IMAG_SCRPTR +  ADDR_PER_WORD;

.CONST $fdfbc.Y2_REAL_SCRPTR                                    $fdfbc.Y1_BEXP +  ADDR_PER_WORD;

.CONST $fdfbc.Y2_IMAG_SCRPTR                                    $fdfbc.Y2_REAL_SCRPTR +  ADDR_PER_WORD;

.CONST $fdfbc.Y2_BEXP                                           $fdfbc.Y2_IMAG_SCRPTR +  ADDR_PER_WORD;

.CONST $fdfbc.dBExp_SCRPTR                                      $fdfbc.Y2_BEXP + ADDR_PER_WORD; 

.CONST $fdfbc.FFT_STRUCT_SCRPTR                                 $fdfbc.dBExp_SCRPTR + ADDR_PER_WORD;                             

.CONST $fdfbc.L2TH_ADAPT                                        $fdfbc.FFT_STRUCT_SCRPTR + ADDR_PER_WORD;

.CONST $fdfbc.L2TH_INIT                                         $fdfbc.L2TH_ADAPT + ADDR_PER_WORD;

.CONST $fdfbc.L2TH_STEP                                         $fdfbc.L2TH_INIT + ADDR_PER_WORD;

.CONST $fdfbc.L2TH_GOHD                                         $fdfbc.L2TH_STEP + ADDR_PER_WORD;

.CONST $fdfbc.WT_TAPS                                           $fdfbc.L2TH_GOHD + ADDR_PER_WORD;

.CONST $fdfbc.FRAME_SIZE                                        $fdfbc.WT_TAPS + ADDR_PER_WORD;

.CONST $fdfbc.HD_CONT                                           $fdfbc.FRAME_SIZE + ADDR_PER_WORD;

.CONST $fdfbc.TERLE0                                            $fdfbc.HD_CONT + ADDR_PER_WORD;

.CONST $fdfbc.FERLE0                                            $fdfbc.TERLE0 + ADDR_PER_WORD;

.CONST $fdfbc.FERLE1                                            $fdfbc.FERLE0 + ADDR_PER_WORD;

.CONST $fdfbc.FERLE2                                            $fdfbc.FERLE1 + ADDR_PER_WORD;

.CONST $fdfbc.HD_FLAG                                           $fdfbc.FERLE2 + ADDR_PER_WORD;

.CONST $fdfbc.WT_FLAG                                           $fdfbc.HD_FLAG + ADDR_PER_WORD;

.CONST $fdfbc.CPY_FLAG                                          $fdfbc.WT_FLAG + ADDR_PER_WORD;

.CONST $fdfbc.MAXPWR_X                                          $fdfbc.CPY_FLAG + ADDR_PER_WORD;

.CONST $fdfbc.AVGLRAT                                           $fdfbc.MAXPWR_X + ADDR_PER_WORD;

.CONST $fdfbc.MAXLRAT                                           $fdfbc.AVGLRAT + ADDR_PER_WORD;

.CONST $fdfbc.COUT_ADPT                                         $fdfbc.MAXLRAT +ADDR_PER_WORD;

.CONST $fdfbc.FBC_GAIN                                          $fdfbc.COUT_ADPT + ADDR_PER_WORD;

.CONST $fdfbc.PTR_HISTORY_BUF                                   $fdfbc.FBC_GAIN + ADDR_PER_WORD;

.CONST $fdfbc.L2P_IBUF_D_FIELD                                  $fdfbc.PTR_HISTORY_BUF + ADDR_PER_WORD;

.CONST $fdfbc.L2P_OBUF_D_FIELD                                  $fdfbc.L2P_IBUF_D_FIELD + ADDR_PER_WORD;

.CONST $fdfbc.LRAT                                              $fdfbc.L2P_OBUF_D_FIELD + ADDR_PER_WORD;

.CONST $fdfbc.POWER_DIFF                                        $fdfbc.LRAT + ADDR_PER_WORD;

.CONST $fdfbc.IBUF_D_PRE_PWR_FIELD                              $fdfbc.POWER_DIFF + ADDR_PER_WORD;

.CONST $fdfbc.OBUF_D_PRE_PWR_FIELD                              $fdfbc.IBUF_D_PRE_PWR_FIELD + ADDR_PER_WORD;

.CONST $fdfbc.L2P_PREP_FDFBC_FIELD                              $fdfbc.OBUF_D_PRE_PWR_FIELD + ADDR_PER_WORD;

.CONST $fdfbc.STRUCT_SIZE                                       ($fdfbc.L2P_PREP_FDFBC_FIELD   >> LOG2_ADDR_PER_WORD) + 2;
// @END  DATA_OBJECT AECFBC_DATAOBJECT


// reference power object
// @DOC_FIELD_TEXT Pointer to "X" (echo reference) freqbuf
// @DOC_FIELD_FORMAT pointer
.CONST $refpwr.AEC_REF_PTR_FIELD                                0*ADDR_PER_WORD;
// @DOC_FIELD_TEXT Poiter to parameter object
// @DOC_FIELD_FORMAT pointer
.CONST $refpwr.PARAM_PTR_FIELD                                  1*ADDR_PER_WORD;
// @DOC_FIELD_TEXT Pointer to variant
// @DOC_FIELD_FORMAT pointer
.CONST $refpwr.PTR_VARIANT_FIELD                                2*ADDR_PER_WORD;
// @DOC_FIELD_TEXT Pointer to scratch memory
// @DOC_FIELD_FORMAT pointer
.CONST $refpwr.SCRATCH_PTR                                      3*ADDR_PER_WORD;
// =========================== START OF INTERNAL FIELDS ============================================
.CONST $refpwr.ALFA_FIELD                                       4*ADDR_PER_WORD;
.CONST $refpwr.PEAK_FIELD                                       5*ADDR_PER_WORD;
.CONST $refpwr.ECHO_FLAG_FIELD                                  6*ADDR_PER_WORD;
.CONST $refpwr.STRUCT_SIZE                                      7;
// =========================== PARAMETERS ============================================
.CONST $refpwr.param.REF_THRESH                                 0*ADDR_PER_WORD;





// -----------------------------------------------------------------------------
// Shared fields
// -----------------------------------------------------------------------------
#define $aec530.map.SELFCLEAN_PTR_FIELD           $aec530.X_STREAM_FIELD

#endif // AEC530_LIB_H_INCLUDED
