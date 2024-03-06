/*!
\copyright  Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Voice source specific parameters used in the kymera adaptation layer
*/

#ifndef KYMERA_VOICE_PROTECTED_H_
#define KYMERA_VOICE_PROTECTED_H_

#include "volume_types.h"
#include <stream.h>
#include "kymera_adaptation_audio_protected.h"

typedef enum
{
    hfp_codec_mode_none,
    hfp_codec_mode_narrowband,
    hfp_codec_mode_wideband,
    hfp_codec_mode_ultra_wideband,
    hfp_codec_mode_super_wideband,
#ifdef INCLUDE_SWB_LC3
    hfp_codec_mode_super_wideband_lc3,
#endif
} hfp_codec_mode_t;

typedef struct
{
    Sink audio_sink;
    hfp_codec_mode_t codec_mode;
    uint8 wesco;
    volume_t volume;
    uint8 pre_start_delay;
    uint8 tesco;
    bool synchronised_start;
    void (*started_handler)(void);
} voice_connect_parameters_t;

typedef struct
{
    uint8 mode;
    uint8 spkr_channels;
    uint8 spkr_frame_size;
    Source spkr_src;
    Sink mic_sink;
    uint32 spkr_sample_rate;
    uint32 mic_sample_rate;
    volume_t volume;
    bool mute_status;
    uint32 min_latency_ms;
    uint32 max_latency_ms;
    uint32 target_latency_ms;
    void (*kymera_stopped_handler)(Source source);
} usb_voice_connect_parameters_t;

typedef struct
{
     Source spkr_src;
     Sink mic_sink;
     void (*kymera_stopped_handler)(Source source);
} usb_voice_disconnect_parameters_t;

typedef struct
{
    uint32 presentation_delay;
    uint32 sample_rate;
    uint16 frame_length;
    uint16 frame_duration;
    uint16 stream_type;
    /*! ISO handle used for Mono (audio location not specified or device is supporting single location like in EB) 
        or Stereo (audio location is both left and right) stream. */
    uint16 iso_handle;
    /*! Right ISO handle when device supports both audio location (Ex: headset) */
    uint16 iso_handle_right;
    uint8 codec_type;
    uint8 codec_version;
    uint8 codec_frame_blocks_per_sdu;
} le_speaker_config_t;

typedef struct
{
    volume_t volume;
    bool microphone_present;
    bool speaker_present;
    bool reconfig;
    le_speaker_config_t speaker;
    le_microphone_config_t microphone;
} le_voice_connect_parameters_t;

#endif /* KYMERA_VOICE_PROTECTED_H_ */
