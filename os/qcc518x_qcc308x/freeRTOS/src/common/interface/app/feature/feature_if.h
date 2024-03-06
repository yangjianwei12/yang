/* Copyright (c) 2016 - 2021 Qualcomm Technologies International, Ltd. */
/*   %%version */
/****************************************************************************
FILE
    feature_if.h

CONTAINS
    Interface elements between firmware (related to Security Decoder and
    Licensable features) and VM applications.

DESCRIPTION
    This file is seen by VM applications and the firmware.
*/

#ifndef __FEATURE_IF_H__
#define __FEATURE_IF_H__

typedef enum 
{
    /* cVc-1 */
    CVC_RECV,
    CVC_SEND_HS_1MIC,
    CVC_SEND_HS_2MIC_MO,
    CVC_SEND_HS_2MIC_B,
    CVC_SEND_HS_3MIC_MO,
    CVC_SEND_HS_3MIC_BI,
    CVC_SEND_SPKR_1MIC,
    CVC_SEND_SPKR_2MIC,
    CVC_SEND_SPKR_3MIC_BS,
    CVC_SEND_SPKR_4MIC_BS,
    CVC_SEND_SPKR_3MIC_CC,
    CVC_SEND_SPKR_4MIC_CC,

    /* cVc-Earbud */
    CVC_SEND_HS_1MIC_EARBUD,
    CVC_SEND_HS_2MIC_MO_EARBUD,

    /* cVc Machine Learning */
    CVC_SEND_ML,
    
    /* Time Domain Full Band Canceller */
    TDFBC = 16,
    TDFBC_MONO,
    CVC_EARBUD_3MIC_IE = 19,

    /* cVc-2 */
    CVC_SEND_AUTO_1MIC = 24,
    CVC_SEND_AUTO_2MIC,

    /* aptX */
    APTX_CLASSIC = 32,
    APTX_HD,
    APTX_LOWLATENCY,
    APTX_CLASSIC_MONO,

    SDS_COMPLIANCE = 39,
    APTX_ADAPTIVE_DECODE,
    APTX_ADAPTIVE_MONO_DECODE,
    APTX_ADAPTIVE_LOSSLESS_DECODE,
    
    /* VAD/SVA */
    VAD = 48,
    SVA,
    VAD_MONO,
    
    /* 3rd Party Voice Activation */
    GVA = 52,
    GVA_MONO,
    AVA,
    AVA_MONO,

    /* ANC */
    IIR_FILTERS_SIDETONE_PATH = 64,
    ANC_FEED_FORWARD,
    ANC_FEED_BACK,
    ANC_HYBRID,
    
    ANC_FEED_FORWARD_EARBUD,
    ANC_FEED_BACK_EARBUD,
    ANC_HYBRID_EARBUD,

    ADAPTIVE_ANC_MONO,
    ADAPTIVE_ANC_STEREO,

    EARBUD_FIT_TEST = 79,
    
    /* 3rd party feature group */
    MACHINE_LEARNING = 84,
    SENSOR_INTEGRATION = 85,
    VIRTUAL_REALITY = 86,
    HEARING_ASSISTANCE = 87,
    KEYWORD_FUNCTIONALITY = 88,
    AUDIO_POST_PROCESSING = 89,
    CODECS = 90,
    ECNS = 91,

    /* LC3 Enhanced Packet Correction */
    LC3_EPC_EARBUD = 96,
    LC3_EPC_HEADSET = 97,

    ATTITUDE_FILTER = 105,

    /* Misc feature bits. */
    USB_DONGLE = 109,
    QHS = 110,
    
    MAX_FEATURES_NUMBER = 111
} feature_id;

#endif /* __FEATURE_IF_H__  */
