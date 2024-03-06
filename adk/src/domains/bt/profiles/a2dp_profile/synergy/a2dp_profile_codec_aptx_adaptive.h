/****************************************************************************
Copyright (c) 2020 - 2022 Qualcomm Technologies International, Ltd.


FILE NAME
    a2dp_profile_codec_aptx_adaptive.h

DESCRIPTION
    This file contains aptX Adaptive specific code.

*/

#ifndef A2DP_CODEC_APTX_AD_H_
#define A2DP_CODEC_APTX_AD_H_

#ifndef A2DP_SBC_ONLY
#include <a2dp.h>

/*************************************************************************
NAME
     appA2dpAreAptxAdCodecsCompatible

DESCRIPTION
    Checks whether the sink and source aptX Adaptive service capabilities are compatible.
*/
bool appA2dpAreAptxAdCodecsCompatible(const uint8 *local_caps, const uint8 *remote_caps, uint8 local_losc, uint8 remote_losc);

/*************************************************************************
NAME
     appA2dpSelectOptimalAptxAdCapsSink

DESCRIPTION
    Selects the optimal configuration for aptX Adaptive playback by setting a single
    bit in each field of the passed caps structure.

    Note that the priority of selecting features is a
    design decision and not specified by the A2DP profiles.

*/
void appA2dpSelectOptimalAptxAdCapsSink(const uint8 *local_caps, uint8 *remote_caps);

/*************************************************************************
NAME
     appA2dpSelectOptimalAptxAdCapsSource

DESCRIPTION
    Selects the optimal configuration for aptX Adaptive playback by setting a single
    bit in each field of the passed caps structure.

    Note that the priority of selecting features is a
    design decision and not specified by the A2DP profiles.

*/
void appA2dpSelectOptimalAptxAdCapsSource(const uint8 *local_caps, uint8 *remote_caps);

/*************************************************************************
NAME
     appA2dpGetAptxAdConfigSettings

DESCRIPTION
    Return the codec configuration settings (rate and channel mode) for the physical codec based
    on the A2DP codec negotiated settings.
*/
void appA2dpGetAptxAdConfigSettings(const uint8 *service_caps, a2dp_codec_settings *codec_settings);

/*************************************************************************
NAME
     appA2dpGetAptxAdQ2qMode

DESCRIPTION
    Return TRUE if Q2Q mode is enabled
*/
bool appA2dpGetAptxAdQ2qMode(const avInstanceTaskData* av_inst);

/*************************************************************************
NAME
     appA2dpGetAptxR22Supported

DESCRIPTION
    Return TRUE if aptX adaptive R22 (Sink Decoder and Source Encoder) is supported
*/
bool appA2dpGetAptxR22Supported(uint8 feature_byte1);
#endif  /* A2DP_SBC_ONLY */

#endif  /* A2DP_CODEC_APTX_AD_H_ */
