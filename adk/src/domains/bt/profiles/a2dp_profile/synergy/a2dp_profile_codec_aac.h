/****************************************************************************
Copyright (c) 2020 - 2022 Qualcomm Technologies International, Ltd.


FILE NAME
    a2dp_profile_codec_aac.h

DESCRIPTION

*/

#ifndef A2DP_CODEC_AAC_H_
#define A2DP_CODEC_AAC_H_

#ifndef A2DP_SBC_ONLY
#include <a2dp.h>

/*************************************************************************
NAME
     appA2dpSelectOptimalAacCapsSink

DESCRIPTION
    Selects the optimal configuration for AAC playback by setting a single
    bit in each field of the passed caps structure.

    Note that the priority of selecting features is a
    design decision and not specified by the A2DP profiles.

*/
void appA2dpSelectOptimalAacCapsSink (const uint8 *local_codec_caps, uint8 *remote_codec_caps);

/*************************************************************************
NAME
     appA2dpSelectOptimalAacCapsSource

DESCRIPTION
    Selects the optimal configuration for AAC playback by setting a single
    bit in each field of the passed caps structure.

    Note that the priority of selecting features is a
    design decision and not specified by the A2DP profiles.

*/
void appA2dpSelectOptimalAacCapsSource(const uint8 *local_codec_caps, uint8 *remote_codec_caps);

/*************************************************************************
NAME
     appA2dpGetAacConfigSettings

DESCRIPTION
    Return the codec configuration settings (rate and channel mode) for the physical codec based
    on the A2DP codec negotiated settings.
*/
void appA2dpGetAacConfigSettings(const uint8 *service_caps, a2dp_codec_settings *codec_settings);

#endif  /* A2DP_SBC_ONLY */

#endif  /* A2DP_CODEC_AAC_H_ */
