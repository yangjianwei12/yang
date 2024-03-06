/*!
\copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\addtogroup av_state_machines
@{
\brief      Includes A2DP stream endpoint IDs
*/

#ifndef AV_SEIDS_H_
#define AV_SEIDS_H_

/*! \brief A2DP stream endpoint IDs

    \note Don't changing the ordering of these IDs as the A2DP code
          makes assumptions on the ordering. When updating this enum
          list, double check for any side-effects with the macros in
          a2dp_profile.h.
*/
enum
{
    AV_SEID_INVALID = 0,
    AV_SEID_SBC_SRC = 0x01,                 /*!< SBC source endpoint for PTS testing */
    AV_SEID_APTX_CLASSIC_SRC = 0x02,        /*!< APTX Classic Source */
    AV_SEID_APTXHD_SRC = 0x03,              /*!< APTX HD Source */
    AV_SEID_APTX_ADAPTIVE_SRC = 0x04,       /*!< APTX Adaptive Source. */

    AV_SEID_SBC_SNK = 0x05,                 /*!< SBC Sink endpoint for standard handsets */
    AV_SEID_AAC_SNK = 0x06,                 /*!< AAC Sink endpoint for standard handsets */
    AV_SEID_APTX_SNK = 0x07,                /*!< APTX Sink endpoint for standard handsets */
    AV_SEID_APTXHD_SNK = 0x08,              /*!< APTX HD Sink endpoint for standard handsets */
    AV_SEID_APTX_ADAPTIVE_SNK = 0x09,       /*!< APTX adaptive Sink endpoint for standard handsets (placeholder) */

    AV_SEID_SBC_MONO_TWS_SRC = 0x0A,        /*!< Mono TTP SBC Source endpoint for earbud forwarding TWS */
    AV_SEID_AAC_STEREO_TWS_SRC = 0x0B,      /*!< Stereo TTP AAC Source endpoint for earbud forwarding TWS */
    AV_SEID_APTX_MONO_TWS_SRC = 0x0C,       /*!< Mono TTP APTX Source endpoint for earbud forwarding TWS  */
    AV_SEID_APTX_ADAPTIVE_TWS_SRC = 0x0D,   /*!< TTP APTX adaptive Source endpoint for earbud forwarding TWS (placeholder) */

    AV_SEID_SBC_MONO_TWS_SNK = 0x0E,        /*!< Mono TTP SBC Sink endpoint for peer earbud receiving TWS */
    AV_SEID_AAC_STEREO_TWS_SNK = 0x0F,      /*!< Stereo TTP AAC Sink endpoint for peer earbud receiving TWS */
    AV_SEID_APTX_MONO_TWS_SNK = 0x10,       /*!< Mono TTP APTX Sink endpoint for peer earbud receiving TWS & TWS+ Handsets */

    /* Virtual SEID, generated when A2DP SEID from library is AV_SEID_APTX_ADAPTIVE_SNK but configured for TWS+ */
    AV_SEID_APTX_ADAPTIVE_TWS_SNK = AV_SEID_APTX_ADAPTIVE_SNK + 0x10,  /*!< Mono TTP APTX adaptive Sink endpoint for TWS+ Handsets */
};

#endif /* AV_SEIDS_H_ */
/**! @} !*/