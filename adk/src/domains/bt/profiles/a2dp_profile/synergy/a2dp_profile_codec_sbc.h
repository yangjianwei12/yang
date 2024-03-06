/****************************************************************************
Copyright (c) 2020 Qualcomm Technologies International, Ltd.


FILE NAME
    a2dp_profile_codec_sbc.h

DESCRIPTION

*/

#ifndef A2DP_CODEC_SBC_H_
#define A2DP_CODEC_SBC_H_

#include <a2dp.h>

/* Define the maximum data rate for 2 and 1 channel modes.  */
#define SBC_TWO_CHANNEL_RATE    (361500) /* half of Bluetooth max rate */
#define SBC_ONE_CHANNEL_RATE    (220000)

/*************************************************************************
NAME
    appA2dpGetSbcConfigSettings

DESCRIPTION
    Get the sampling rate and channel mode from the codec config settings.

*/
void appA2dpGetSbcConfigSettings(const uint8 *service_caps, a2dp_codec_settings *codec_settings);

/*************************************************************************
NAME
     appA2dpSelectOptimalSbcCapsSink

DESCRIPTION
    Selects the optimal configuration for SBC playback by setting a single
    bit in each field of the passed caps structure.

    Note that the priority of selecting features is a
    design decision and not specified by the AV profiles.

*/
void appA2dpSelectOptimalSbcCapsSink(const uint8 *local_caps, uint8 *caps);

/*************************************************************************
NAME
    appA2dpSelectOptimalSbcCapsSource

DESCRIPTION
    Selects the optimal configuration for SBC playback by setting a single
    bit in each field of the passed caps structure.

    Note that the priority of selecting features is a
    design decision and not specified by the AV profiles.

*/
void appA2dpSelectOptimalSbcCapsSource(const uint8 *local_caps, uint8 *caps);

/****************************************************************************
NAME
    appA2dpSbcFormatFromConfig

DESCRIPTION
    Converts an A2DP SBC Configuration bitfield to the SBC header format.
    It assumes a valid configuration request with one bit in each field.

*/
uint8 appA2dpSbcFormatFromConfig(const uint8 *config);

/****************************************************************************
NAME
    appA2dpSbcSelectBitpool

DESCRIPTION
    Calculates the optimum bitpool and packet size for the requested data rate using
    the specified SBC format.
    - Get frame size from rate.
    - Find optimum L2CAP PDU size for baseband packets.
    - Find integer multiple of frame size to fit in RTP packet.
    - Calculate bitpool required for this frame size (round down).

*/
uint8 appA2dpSbcSelectBitpool(uint8 sbc_header, uint32 rate, uint16 pdu);

#endif /* A2DP_CODEC_SBC_H_ */
