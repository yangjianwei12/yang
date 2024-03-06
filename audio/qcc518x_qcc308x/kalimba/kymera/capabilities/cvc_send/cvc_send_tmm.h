// *****************************************************************************
// Copyright (c) 2014 - 2023 Qualcomm Technologies International, Ltd.
// %%version
//
// *****************************************************************************

#ifndef _CVC_SEND_TMM_ASM_H
#define _CVC_SEND_TMM_ASM_H

// -----------------------------------------------------------------------------
// WNR TMM - Used for Extra Wind Tier
// -----------------------------------------------------------------------------
#define $asf100.wnr.tmm.WNR_MAP_OBJ_FIELD    MK1 * 0
#define $asf100.wnr.tmm.HI_WIND_POW_TH       MK1 * 1
#define $asf100.wnr.tmm.HI_WIND_FLAG_FIELD   MK1 * 2
#define $asf100.wnr.tmm.STRUC_SIZE                (2 + $hyst.STRUCT_SIZE)

// -----------------------------------------------------------------------------
// DMSS SNR TMM - Used for Extra SNR Tier
// -----------------------------------------------------------------------------
#define $dmss.snr.tmm.MAIN_OBJ_FIELD         MK1 * 0
#define $dmss.snr.tmm.NOISE_ON_THRESH_FIELD  MK1 * 1
#define $dmss.snr.tmm.NOISE_OFF_THRESH_FIELD MK1 * 2
#define $dmss.snr.tmm.NOISE_FLAG_FIELD       MK1 * 3
#define $dmss.snr.tmm.NOISE_HYST_FIELD       MK1 * 4
#define $dmss.snr.tmm.STRUC_SIZE                  (4 + $hyst.STRUCT_SIZE)

// -----------------------------------------------------------------------------
// TMM (Tiering Manager Module) data structure and mode definitions
// -----------------------------------------------------------------------------
#define $cvc.tmm.MODERATE_WNR_OBJ_PTR        MK1 * 0
#define $cvc.tmm.MODERATE_SNR_OBJ_PTR        MK1 * 1
#define $cvc.tmm.NS_INT_OBJ                  MK1 * 2
#define $cvc.tmm.MAP_OBJ                     MK1 * 3
#define $cvc.tmm.BLEND_OBJ                   MK1 * 4
#define $cvc.tmm.INT_MULFUNC_OBJ             MK1 * 5
#define $cvc.tmm.INT_FIT_OBJ                 MK1 * 6
#define $cvc.tmm.MODE_FLAG                   MK1 * 7
#define $cvc.tmm.BLEND_ON_FLAG               MK1 * 8
#define $cvc.tmm.PEQ_OFF_FLAG                MK1 * 9
#define $cvc.tmm.STREAM_GAIN                 MK1 * 10
#define $cvc.tmm.PARAM_NS_EXT                MK1 * 11
#define $cvc.tmm.PARAM_NS_INT               (MK1 * 4 + $cvc.tmm.PARAM_NS_EXT)
#define $cvc.tmm.PARAM_MBDRC                (MK1 * 2 + $cvc.tmm.PARAM_NS_INT)   // store all 4 MLFE params
#define $cvc.tmm.PARAM_MLFE                 (MK1 * 29 + $cvc.tmm.PARAM_MBDRC)
#define $cvc.tmm.HIGH_WNR_OBJ               (MK1 * 4 + $cvc.tmm.PARAM_MLFE)
#define $cvc.tmm.EXTREME_WNR_OBJ            (MK1 * $asf100.wnr.tmm.STRUC_SIZE + $cvc.tmm.HIGH_WNR_OBJ)
#define $cvc.tmm.HIGH_SNR_OBJ               (MK1 * $asf100.wnr.tmm.STRUC_SIZE + $cvc.tmm.EXTREME_WNR_OBJ)
#define $cvc.tmm.EXTREME_SNR_OBJ            (MK1 * $dmss.snr.tmm.STRUC_SIZE + $cvc.tmm.HIGH_SNR_OBJ)
#define $cvc.tmm.STRUC_SIZE                 ($cvc.tmm.EXTREME_SNR_OBJ >> LOG2_ADDR_PER_WORD) + $dmss.snr.tmm.STRUC_SIZE

// -----------------------------------------------------------------------------
// TMM (Tiering Modes), to be kept in this order to avoid unexpected behaviour
// -----------------------------------------------------------------------------
#define $cvc.tmm.mode.LOW                    0
#define $cvc.tmm.mode.MODERATE_WIND          1
#define $cvc.tmm.mode.MODERATE_NOISE         2
#define $cvc.tmm.mode.MODERATE               3
#define $cvc.tmm.mode.HIGH_WIND              4
#define $cvc.tmm.mode.HIGH_NOISE             5
#define $cvc.tmm.mode.HIGH                   6
#define $cvc.tmm.mode.EXTREME_WIND           7
#define $cvc.tmm.mode.EXTREME_NOISE          8
#define $cvc.tmm.mode.EXTREME                9

#endif // _CVC_SEND_TMM_ASM_H
