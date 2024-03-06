/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       headset_lea_src_config.h
\brief      Header file for the Speaker lea source configuration.
*/

#ifndef HEADSET_LEA_SRC_CONFIG_H
#define HEADSET_LEA_SRC_CONFIG_H

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
#include "le_audio_client.h"

#define HEADSET_LEA_DEF_BROADCAST_SRC_NAME_MAX_LEN                      30u
#define HEADSET_LEA_BROADCAST_CODE_MAX_LEN                              16u
#define HEADSET_LEA_BROADCAST_CODE_MIN_LEN                              4u
#define HEADSET_LEA_BROADCAST_ID_SIZE                                   3u
#define HEADSET_LEA_CONFIG_BROADCAST_DEFAULT_MAX_CODEC_PER_SDU          0x1

/* for supporting Speaker concurrency use-case with A2DP & Broadcast the 
   preferred configuration is (for 2 BIS):
   SDU Size - 100
   SDU Interval - 10ms
   Latency - 56
   RTN - 2
   PD (receiver) - 20ms
*/
#define HEADSET_LEA_CONFIG_BROADCAST_DEFAULT_NUM_OF_RETRANSMISSION      0x2
#define HEADSET_LEA_CONFIG_BROADCAST_DEFAULT_SDU_SIZE                   0x64
#define HEADSET_LEA_CONFIG_BROADCAST_DEFAULT_MAX_LATENCY                0x38   /* 56ms */
#define HEADSET_LEA_CONFIG_BROADCAST_DEFAULT_SDU_INTERVAL               0x2710 /* 10ms */
#define HEADSET_LEA_CONFIG_BROADCAST_DEFAULT_PRESENTATION_DELAY         0x4E20 /* 20ms */
#define HEADSET_LEA_CONFIG_BROADCAST_TRANSPORT_LATENCY                  0x9072 /* 36.978ms */


/*! Number of BIS for Default configuration */
#define HEADSET_LEA_CONFIG_BROADCAST_DEFAULT_NUM_BIS                    2u

/*! Number of audio channels per BIS Default configuration */
#define HEADSET_LEA_CONFIG_BROADCAST_DEFAULT_AUDIO_CHANNELS_PER_BIS     1u

/*! Default broadcast advertising interval in range 20ms to 10.24 sec( Interval = N * 0.625ms) */
#define HEADSET_LEA_ADV_INTERVAL_MIN_DEFAULT                            (0x0080u) /* 80 ms */
#define HEADSET_LEA_ADV_INTERVAL_MAX_DEFAULT                            (0x00C0u) /* 120 ms */

/*! \brief Default periodic advertisement interval to use (PA interval = N * 1.25ms) */
#define HEADSET_LEA_PA_INTERVAL_DEFAULT                                 (80u) /* 100ms */

/*! \brief Periodic advertisement interval max and min to use (PA interval = N * 1.25ms) */
#define HEADSET_LEA_PA_INTERVAL_DELTA_DEFAULT                           (16u) /* 20ms */
#define HEADSET_LEA_PA_INTERVAL_MAX_DEFAULT                             (HEADSET_LEA_PA_INTERVAL_DEFAULT + HEADSET_LEA_PA_INTERVAL_DELTA_DEFAULT)
#define HEADSET_LEA_PA_INTERVAL_MIN_DEFAULT                             (HEADSET_LEA_PA_INTERVAL_DEFAULT - HEADSET_LEA_PA_INTERVAL_DELTA_DEFAULT)

/*! The Advertisement TX power level.Ranges from -127 to +20dBm.*/
#define HEADSET_LEA_ADVERTISEMENT_TX_POWER                              20

/*! LE Audio Configuration data */
typedef struct
{
    /*! Broadcast source name and its length */
    char                                broadcast_source_name[HEADSET_LEA_DEF_BROADCAST_SRC_NAME_MAX_LEN];

    uint8                               broadcast_source_name_len;

    /*! Is public broadcast (PBP) mode enabled */
    bool                                public_broadcast_mode;

    /*! Is broadcast encrypted */
    bool                                is_encrypted_broadcast;

    /*! Code for broadcast encryption */
    uint8                               broadcast_code[HEADSET_LEA_BROADCAST_CODE_MAX_LEN];

    /*! ID for broadcast */
    uint32                              broadcast_id;

    /*! Broadcast configuration used when PBP is not enabled */
    LE_AUDIO_CLIENT_BROADCAST_CONFIG_T  tmap_broadcast_audio_config;

    /*! Broadcast configuration used when PBP is enabled */
    LE_AUDIO_CLIENT_BROADCAST_CONFIG_T  pbp_broadcast_audio_config;

    /*! Broadcast advertisement configuration for PBP/TMAP broadcast */
    CapClientBcastSrcAdvParams          broadcast_adv_param;
}headset_lea_config_data_t;


/*! \brief  Sets the PBP broadcast enable/disable.

     \param enable Enable/Disable PBP
*/
void HeadsetLeaSrcConfig_SetPbpBroadcastmode(bool enable);


/*! \brief Initializes the LEA configurations
*/
void HeadsetLeaSrcConfig_Init(void);

/*! \brief Set LC3 Codec params in kymera layer as configured in the application.
*/
void HeadsetLeaSrcConfig_SetLc3CodecParams(void);

/*! \brief  Sets the Lea broadcast Stream capability (TmapClientStreamCapability)

     \param broadcast_stream_capability BAP configuration TmapClientStreamCapability.
     \param is_public_broadcast Is this for public broadcast source

     \return TRUE if able to set, returns FALSE otherwise.
*/
bool HeadsetLeaSrcConfig_SetLeaBroadcastStreamCapability(uint32 broadcast_stream_capability, bool is_public_broadcast);

/*! \brief Sets the broadcast code for streaming.

     \param code Pointer to the 16 octet broadcast code. If not encrypted, NULL can be passed here
     \param length Length of broadcast code.
*/
void HeadsetLeaSrcConfig_SetLeaBroadcastCode(const uint8 *code, uint8 length);

/*! \brief  Update LEA broadcast audio config for the session

     \param sdu_interval SDU interval
     \param sdu_size Maximum SDU size
     \param max_transport_latency Maximum transport latency
     \param rtn Number of broadcast retransmissions
*/
void HeadsetLeaSrcConfig_SetLeaBroadcastAudioConfig(uint32 sdu_interval, uint16 sdu_size, uint16 max_transport_latency, uint8 rtn);

/*! \brief  Set LEA broadcast ID for the session

     \param bcast_id Broadcast ID to be set
     \param length Length of the broadcast ID

     \return TRUE if able to set, returns FALSE otherwise.
*/
bool HeadsetLeaSrcConfig_SetLeaBroadcastID(const uint8 *bcast_id, uint8 length);

#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

#endif /* HEADSET_LEA_SRC_CONFIG_H */
