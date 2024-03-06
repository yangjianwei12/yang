/****************************************************************************
Copyright (c) 2016 - 2023 Qualcomm Technologies International, Ltd.


FILE NAME
    operators_constants.h

DESCRIPTION
    Constants used by operators.
*/

#ifndef OPERATORS_CONSTANTS_H_
#define OPERATORS_CONSTANTS_H_

#define OPERATOR_CREATE_KEY_SET_PRIORITY   1
#define OPERATOR_CREATE_KEY_SET_PROCESSOR_ID 2

#define RESAMPLER_SET_CONVERSION_RATE   2

#define IIR_RESAMPLER_SET_SAMPLE_RATES 1

#define RTP_SET_MODE                     1
#define RTP_SET_CODEC_TYPE               2
#define RTP_SET_CONTENT_PROTECTION       3
#define RTP_SET_AAC_CODEC                4
#define RTP_SET_MAX_PACKET_LENGTH        5
#define RTP_SET_SSRC_LATENCY_MAPPING     7
#define RTP_SET_SSRC_CHANGE_NOTIFICATION 9
#define RTP_SET_TTP_NOTIFICATION         10

#define MIXER_SET_GAINS            1
#define MIXER_SET_STREAM_CHANNELS  2
#define MIXER_SET_RAMP_NUM_SAMPLES 3
#define MIXER_SET_PRIMARY_STREAM   4
#define MIXER_SET_CHANNEL_GAINS    5

#define SOURCE_SYNC_SET_ROUTES       1
#define SOURCE_SYNC_SET_SINK_GROUP   3
#define SOURCE_SYNC_SET_SOURCE_GROUP 4

/* Common Message Ids */
#define GET_CAPABILITY_VERSION   0x1000

#define ENABLE_FADE_OUT          0x2000
#define DISABLE_FADE_OUT         0x2001
#define SET_CONTROL              0x2002
#define GET_PARAMS               0x2003
#define SET_PARAMS               0x2005
#define GET_STATUS               0x2006
#define SET_UCID                 0x2007
#define SET_BUFFER_SIZE          0x200c
#define SET_TERMINAL_BUFFER_SIZE 0x200d
#define SET_SAMPLE_RATE          0x200e
#define SET_BACK_KICK_THRESHOLD  0x2020

#define SET_TIME_TO_PLAY_LATENCY 0x2012

#define SET_LATENCY_LIMITS       0x2015
#define SET_TTP_STATE            0x201a

#define GET_APVA_METADATA        0x0004

#define USB_AUDIO_SET_CONNECTION_CONFIG 0x0002

#define SBC_ENCODER_SET_ENCODING_PARAMS 0x0001

#define APTXAD_ENCODER_SET_ENCODING_PARAMS 0x0001

#define APTXAD_96K_ENCODER_CONFIG_PARAMS 0x0004

#define APTXAD_96K_ENCODER_RF_SIGNAL_PARAMS 0x0003

#define APTXAD_R3_ENCODER_CONFIG_PARAMS 0x0001

#define APTXAD_R3_ENCODER_RF_SIGNAL_PARAMS 0x0002

#define APTXAD_DECODER_SET_EXTRACT_MODE 0x0001

#define APTXAD_DECODER_SET_INTERNAL_DELAY_MODE 0x0002

#define CELT_ENCODER_SET_ENCODING_PARAMS 0x0001

/* In 1Hz units, limited to 48kHz */
#define AEC_REF_SET_SAMPLE_RATES 0x00FE
/* In 25Hz units, not limited to 48kHz */
#define AEC_REF_SET_INPUT_OUTPUT_SAMPLE_RATES 0x00FD

#define AEC_REF_SET_TASK_PERIOD 0x0007

#define AEC_REF_MUTE_MIC_OUTPUT 0x0009

#define AEC_REF_ENABLE_SPKR_GATE 0x000A

#define SPDIF_SET_OUTPUT_SAMPLE_RATE    0x0003

#define MSBC_ENCODER_SET_BITPOOL_VALUE 0x0001

#define MAX_NOTES_NUM 63

#define USB_AUDIO_DATA_FORMAT_PCM   0

#define SPLITTER_SET_RUNNING_STREAMS 1
#define SPLITTER_ACTIVATE_STREAMS 2
#define SPLITTER_DEACTIVATE_STREAMS 4
#define SPLITTER_ACTIVATE_STREAMS_AFTER_TIMESTAMP 3
#define SPLITTER_BUFFER_STREAMS 5
#define SPLITTER_SET_MODE 6
#define SPLITTER_SET_LOCATION 7
#define SPLITTER_SET_PACKING 8
#define SPLITTER_SET_REFRAMING 9
#define SPLITTER_SET_DATA_FORMAT 10

#define SPLITTER_MODE_CLONE_INPUT 0
#define SPLITTER_MODE_BUFFER_INPUT 1

#define SPLITTER_PACKING_UNPACKED 0
#define SPLITTER_PACKING_PACKED 1

#define SPLITTER_REFRAMING_DISABLED 0
#define SPLITTER_REFRAMING_ENABLED 1

#define PASSTHRUOGH_SET_INPUT_DATA_FORMAT 10
#define PASSTHRUOGH_SET_OUTPUT_DATA_FORMAT 11

#define VAD_SET_MODE 0x01
#define VOL_CTRL_ID_SET_AUX_TIME_TO_PLAY 0x1

/* Custom Message IDs for Adaptive ANC */
#define ADAPTIVE_ANC_SET_STATIC_GAIN    1
#define ADAPTIVE_ANC_SET_PLANT_MODEL     2
#define ADAPTIVE_ANC_SET_CONTROL_MODEL   3

/* Custom Message IDs for Adaptive ANC V2 */
#define ADAPTIVE_ANC_GET_ADAPTIVE_GAIN 4

/* Constants for Volume Operator Control Ids */
#define POST_GAIN    0x0020
#define MAIN_GAIN    0x0021
#define AUX_GAIN1    0x0030
#define AUX_GAIN2    0x0031
#define AUX_GAIN3    0x0032
#define AUX_GAIN4    0x0033
#define AUX_GAIN5    0x0034
#define AUX_GAIN6    0x0035
#define AUX_GAIN7    0x0036
#define AUX_GAIN8    0x0037
#define TRIM_GAIN1   0x0010
#define TRIM_GAIN2   0x0011
#define TRIM_GAIN3   0x0012
#define TRIM_GAIN4   0x0013
#define TRIM_GAIN5   0x0014
#define TRIM_GAIN6   0x0015
#define TRIM_GAIN7   0x0016
#define TRIM_GAIN8   0x0017

/* Constants for Adaptive ANC Set Control message Ids */
#define ADAPTIVE_ANC_IN_OUT_EAR_CTRL     0x0003
#define ADAPTIVE_ANC_GAIN_CTRL           0x0004
#define ADAPTIVE_ANC_HW_CHANNEL_CTRL     0x0005
#define ADAPTIVE_ANC_FEEDFORWARD_CTRL    0x0006
#define ADAPTIVE_ANC_MODE_OVERRIDE_CTRL  0x0001
#define ADAPTIVE_ANC_FILTER_CONFIG_CTRL  0x000c
#define ADAPTIVE_ANC_SET_SAMPLE_RATE_CTRL  0x0011

/* Adaptive ANC Configuration parameters supported by capability*/
#define ADAPTIVE_ANC_AG_CALC_OFFSET      1

#define ADAPTIVE_ANC_DISABLE_AG_CALC     0x1
#define ADAPTIVE_ANC_ENABLE_AG_CALC      0x0

/* Mu Mantissa & exponent parameters offsets */
#define ADAPTIVE_ANC_MU_MANTISSA_OFFSET     2
#define ADAPTIVE_ANC_MU_EXPONENT_OFFSET    3

/* Adaptive ANC Statistics parameters reported by capability*/
#define ADAPTIVE_ANC_GAIN_CALC_OFFSET    6  /*Calculated adaptive ANC FF gain (steps)*/
#define ADAPTIVE_ANC_FF_GAIN_OFFSET      7  /*Current ANC FF gain (steps)*/

#define ADAPTIVE_ANC_GENTLE_MUTE_TIMER_OFFSET 4

/* Constants for Adaptive ANC version2 Set Control message Ids */
#define ADAPTIVE_ANCV2_MODE_OVERRIDE_CTRL    0x0001
#define ADAPTIVE_ANCV2_FILTER_CONFIG_CTRL    0x0003
#define ADAPTIVE_ANCV2_SET_SAMPLE_RATE_CTRL  0x0004

/*ANC Gain Priority parameters*/
#define AHM_FF_RAMP_PRIORITY    1
#define ANC_COMPANDER_PRIORITY  42

/*Custom message ID for ANC Compander*/
#define COMPANDER_GET_ADJUSTED_GAIN 1
#define COMPANDER_SET_MAKEUP_GAIN 3

/*ANC Link Control Messages*/
#define ANC_LINK_HW_MANAGER          0x2025
#define ANC_HCGR_LINK_TARGET_GAIN    0x01

/* Constants for ANC hardware manager (AHM) Set Control message Ids */
#define AHM_SYSMODE_CTRL                0x1
#define AHM_INEAR_CTRL                  0x3
#define AHM_CHANNEL_CTRL                0x4
#define AHM_FFPATH_CTRL                 0x5
#define AHM_AMBIENT_CTRL                0x6
#define AHM_TRIGGER_TRANSITION_CTRL     0x7
#define AHM_FFPATH_FINE_GAIN_CTRL       0xB

/* Constants for ANC hardware manager (AHM) Set/Get Param message Ids */
#define AHM_CONFIG_PARAM_INDEX                  0x00
#define AHM_WIND_FF_RAMP_DURATION_INDEX         0x06
#define AHM_WIND_FB_RAMP_DURATION_INDEX         0x07
#define AHM_WIND_FF_FINE_GAIN_INDEX             0x0E
#define AHM_WIND_FB_FINE_GAIN_INDEX             0x0F

/*AHM specific messages*/
#define AHM_SET_STATIC_GAIN                 1
#define AHM_GET_GAIN                        2
#define AHM_SET_FINE_TARGET_GAIN            4
#define AHM_SET_IIR_FILTER_COEFFS           5
#define AHM_SET_TASK_PERIOD                 7

#define AHM_FF_FILTER               0

/* Constants for ANC Compander makeup gain update */
#define ANC_COMPANDER_SYSMODE_CTRL    0x1
#define ANC_COMPANDER_MAKEUP_GAIN_PARAM_INDEX 22

/* Constants for ANC howling control capability */
#define HCGR_ANC_SYSMODE_CTRL    0x1
#define HCGR_ANC_CHANNEL     0
#define HCGR_ANC_FF_FILTER   0
#define HCGR_ANC_FINE_GAIN   0
/* Constants for ANC howling control (HCGR) Set/Get Param message Ids */
#define HCGR_CONFIG_PARAM_INDEX          0x00

/* Constants for Wind Detect Set Control message Ids */
#define WIND_DETECT_SYSMODE_CTRL                0x1
#define WIND_DETECT_INTENSITY_UPDATE_CTRL       0x2

#define WIND_DETECT_GET_MITIGATION_PARAMTERS_COUNT 4U

#define WIND_DETECT_LOW_INTENSITY_FF_RAMP_DURATION_INDEX            18
#define WIND_DETECT_LOW_INTENSITY_FB_RAMP_DURATION_INDEX            19
#define WIND_DETECT_LOW_INTENSITY_FF_FINE_GAIN_INDEX                20
#define WIND_DETECT_LOW_INTENSITY_FB_FINE_GAIN_INDEX                21

#define WIND_DETECT_MEDIUM_INTENSITY_FF_RAMP_DURATION_INDEX         22
#define WIND_DETECT_MEDIUM_INTENSITY_FB_RAMP_DURATION_INDEX         23
#define WIND_DETECT_MEDIUM_INTENSITY_FF_FINE_GAIN_INDEX             24
#define WIND_DETECT_MEDIUM_INTENSITY_FB_FINE_GAIN_INDEX             25

#define WIND_DETECT_HIGH_INTENSITY_FF_RAMP_DURATION_INDEX           26
#define WIND_DETECT_HIGH_INTENSITY_FB_RAMP_DURATION_INDEX           27
#define WIND_DETECT_HIGH_INTENSITY_FF_FINE_GAIN_INDEX               28
#define WIND_DETECT_HIGH_INTENSITY_FB_FINE_GAIN_INDEX               29

/* Constants for Auto transparency VAD Set Control message Ids */
#define ATR_VAD_SYSMODE_CTRL    0x1

/*ATR VAD Custom messages*/
#define ATR_VAD_SET_RELEASE_DURATION    1
#define ATR_VAD_GET_RELEASE_DURATION    2
#define ATR_VAD_SET_SENSITIVITY         3
#define ATR_VAD_GET_SENSITIVITY         4

/* Constants for ADRC Set/Get Param message Ids */
#define ADRC_COMPANDER_CONFIG_PARAM_INDEX          0x00

/* Constants for AAH Set Control message Ids */
#define AAH_SYSMODE_CTRL          0x1
#define AAH_LIMIT_LEVEL_FB_DB     0x8
#define AAH_LIMIT_LEVEL_COMB_DB   0xa

/* Constants for FBC Set Control message Ids */
#define FBC_SYSMODE_CTRL          0x1

/* Constants for Noise ID Set Control message Ids */
#define NOISE_ID_SYSMODE_CTRL          0x1

/* Constants for Noise ID custom messages */
#define NOISE_ID_SET_CURRENT_CATEGORY          0x1

/* Constants for GEQ Set Control message Ids */
#define GEQ_SYSMODE_CTRL          0x1

/* Constants for Earbud fit test operator message ids */
#define EARBUD_FIT_TEST_IN_OUT_EAR_CTRL      (0x0003U)
#define EARBUD_FIT_TEST_SET_EQU_OP_ID        (0x0003U)
#define EARBUD_FIT_TEST_POWER_SMOOTH_FACTOR  (0x0000U)
#define EARBUD_FIT_TEST_SINGLE_CAPTURE_PARAM (0x0001U)
#define EARBUD_FIT_TEST_GET_CAPTURED_BINS    (0x0002U)
#define EARBUD_FIT_TEST_SYSMODE_CTRL          0x1

/* Constants for Operator Set Control Message */
/* originally defined in the dsp project workspace in opsmsg_prim.h*/
#define OPMSG_CONTROL_MODE_ID 0x0001
#define OPMSG_CONTROL_MUTE_ID 0x0002
#define OPMSG_CONTROL_MUTE_PERIOD_ID 0x0022

/* Constants for Operator Set Control Mode values */
#define CONTROL_MODE_STANDBY         1
#define CONTROL_MODE_FULL_PROCESSING 2
#define CONTROL_MODE_PASSTHROUGH     3

/* Constants for Operator Set Control Mode values */
#define MUSIC_PROCESSING_MODE_STANDBY         1
#define MUSIC_PROCESSING_MODE_FULL_PROCESSING 2
#define MUSIC_PROCESSING_MODE_PASSTHROUGH     3

/* Constants for Set Data Format messages values */
#define SET_DATA_FORMAT_ENCODED               0
#define SET_DATA_FORMAT_PCM                   1
#define SET_DATA_FORMAT_16_BIT_WITH_METADATA  2
#define SET_DATA_FORMAT_ENCODED_32_BIT       13

#define SOURCE_SYNC_GROUP_META_DATA_FLAG      ((uint32)(1 << 31))
#define SOURCE_SYNC_GROUP_TTP_FLAG            ((uint32)(1 << 30))
#define SOURCE_SYNC_GROUP_RATE_MATCH_FLAG     ((uint32)(1 << 27))

/* Constants for celt encoder */
#define CELT_CODEC_FRAME_SIZE_DEFAULT 220
/*! CELT encoder mode: 512 samples per frame, 48000Hz */
#define CELT_ENC_MODE_512_48000 0
/*! CELT encoder mode: 512 samples per frame, 44100Hz */
#define CELT_ENC_MODE_512_44100 1
#define CELT_ENC_MODE_MONO 0
#define CELT_ENC_MODE_STEREO 1

/* Constants for Common Wake-up word engine wrapper operator */
#define WUW_RESET_STATUS 0x0000
#define WUW_TRIGGER_PHRASE_LOAD 0x0002
#define WUW_SET_MIN_MAX_TRIGGER_PHRASE_LEN 0x0005

/* Constants for Switched Passthrough Consumer operator */
#define SPC_SET_TRANSITION 0x0001
#define SPC_SET_FORMAT 0x0002
#define SPC_SET_BUFFERING 0x0003
#define SPC_SELECT_PASSTHROUGH_INPUT 0x0005

/* Framework configuration parameters */
#define FRAMEWORK_KICK_PERIOD_PARAM 7

/* Constants for cVc send operator */
#define CVC_SEND_INT_MIC_MODE 0x00AA
#define CVC_SEND_DMSS_CONFIG 0x0001

#define SWB_ENCODE_SET_CODEC_MODE   1
#define SWB_DECODE_SET_CODEC_MODE   3

/* Constants for LC3 Decoder for SCO_ISO operator */
#define LC3_DEC_SCO_ISO_SET_PACKET_LEN      0x0001
#define LC3_DEC_SCO_ISO_SET_FRAME_DURATION  0x0002
#define LC3_DEC_SCO_ISO_SET_NUM_OF_CHANNELS 0x0004
#define LC3_DEC_SCO_ISO_SET_MONO_DECODE     0x0005
#define LC3_DEC_SCO_ISO_SET_BLOCKS_PER_SDU  0x0006
#define LC3_DEC_SCO_ISO_ID_CONNECT_SCO_ENDPOINT 0x0008

/* Constants for LC3 Encoder for SCO_ISO operator */
#define LC3_ENC_SCO_ISO_SET_PACKET_LEN 0x0001
#define LC3_ENC_SCO_ISO_SET_FRAME_DURATION 0x0002
#define LC3_ENC_SCO_ISO_SET_BLOCKS_PER_SDU 0x0003
#define LC3_ENC_SCO_ISO_SET_ERROR_RESILIENCE 0x0004
#define LC3_ENC_SCO_ISO_SET_NUM_OF_CHANNELS 0x0005
#define LC3_ENC_SCO_ISO_ID_CONNECT_SCO_ENDPOINT 0x0006

/* Constants for Aptx Lite codec for SCO_ISO operator */
#define APTX_LITE_DEC_SCO_ISO_FRAME_DURATION      0x1
#define APTX_LITE_ENC_SCO_ISO_ENCODING_PARAM      0x1

/* Constants for Aptx Adaptive codec for SCO_ISO operator */
#define APTX_ADAPTIVE_ENC_SCO_ISO_SET_QHS_LEVEL      0x2

/* Constants for Aptx Adaptive codec for SCO_ISO operator */
#define APTX_ADAPTIVE_DEC_SCO_ISO_FRAME_DURATION     0x0002
#define APTX_ADAPTIVE_DEC_SCO_ISO_PREDECODE_DURATION 0x0004

/* Constants for VA Graph Manager operator */
#define VA_GM_SET_MODE 0x0003

/* Constants for AAC Decoder operator */
#define AAC_DECODER_SET_FRAME_TYPE 0x0002

typedef enum {
    aptx_ad_disable_mode_notifications = 0,
    aptx_ad_enable_mode_notifications  = 1
} aptx_adaptive_mode_notifications_t;

#endif /* OPERATORS_CONSTANTS_H_ */
