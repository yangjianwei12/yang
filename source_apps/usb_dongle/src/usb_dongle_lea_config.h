/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       usb_dongle_lea_config.h
\brief      Header file for the USB dongle lea configuration.
*/

#ifndef USB_DONGLE_LEA_CONFIG_H
#define USB_DONGLE_LEA_CONFIG_H

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO

#include "le_audio_client.h"

/*! Adv UUID filters for pairing */
#define USB_LEA_CAS_UUID                                            0x1853
#define USB_LEA_ASCS_UUID                                           0x184E
#define USB_LEA_TMAS_UUID                                           0x1855
#define USB_LEA_GMCS_UUID                                           0x1849
#define USB_LEA_GTBS_UUID                                           0x184C
#define USB_LEA_VCS_UUID                                            0x1844
#define USB_LEA_CSIS_UUID                                           0x1846

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE

#define USB_LEA_DEF_BROADCAST_SRC_NAME_MAX_LEN                      30u
#define USB_LEA_BROADCAST_CODE_MAX_LEN                              16u
#define USB_LEA_BROADCAST_CODE_MIN_LEN                              4u
#define USB_LEA_BROADCAST_ID_SIZE                                   3u
#define USB_LEA_CONFIG_BROADCAST_DEFAULT_NUM_OF_RETRANSMISSION      0x4
#define USB_LEA_CONFIG_BROADCAST_DEFAULT_MAX_CODEC_PER_SDU          0x1
#define USB_LEA_CONFIG_BROADCAST_DEFAULT_SDU_SIZE                   0x64
#define USB_LEA_CONFIG_BROADCAST_DEFAULT_MAX_LATENCY                0x46   /* 70ms */
#define USB_LEA_CONFIG_BROADCAST_DEFAULT_SDU_INTERVAL               0x2710 /* 10ms */
#define USB_LEA_CONFIG_BROADCAST_DEFAULT_PRESENTATION_DELAY         0x4E20 /* 20ms */

/*! Number of BIS for Default configuration */
#define USB_LEA_CONFIG_BROADCAST_DEFAULT_NUM_BIS                    2u

/*! Number of audio channels per BIS Default configuration */
#define USB_LEA_CONFIG_BROADCAST_DEFAULT_AUDIO_CHANNELS_PER_BIS     1u

/*! Default broadcast advertising interval in range 20ms to 10.24 sec( Interval = N * 0.625ms) */
#define USB_LEA_ADV_INTERVAL_MIN_DEFAULT                            (0x0080u) /* 80 ms */
#define USB_LEA_ADV_INTERVAL_MAX_DEFAULT                            (0x00C0u) /* 120 ms */

/*! \brief Default periodic advertisement interval to use (PA interval = N * 1.25ms) */
#define USB_LEA_PA_INTERVAL_DEFAULT                                 (360u) /* 450ms */

/*! \brief Periodic advertisement interval max and min to use (PA interval = N * 1.25ms) */
#define USB_LEA_PA_INTERVAL_DELTA_DEFAULT                           (16u) /* 20ms */
#define USB_LEA_PA_INTERVAL_MAX_DEFAULT                             (USB_LEA_PA_INTERVAL_DEFAULT + USB_LEA_PA_INTERVAL_DELTA_DEFAULT)
#define USB_LEA_PA_INTERVAL_MIN_DEFAULT                             (USB_LEA_PA_INTERVAL_DEFAULT - USB_LEA_PA_INTERVAL_DELTA_DEFAULT)

/*! The Advertisement TX power level.Ranges from -127 to +20dBm.*/
#define USB_LEA_ADVERTISEMENT_TX_POWER                              20

/*! LE Audio Configuration data */
typedef struct
{
    /*! Broadcast source name and its length */
    char                                broadcast_source_name[USB_LEA_DEF_BROADCAST_SRC_NAME_MAX_LEN];

    uint8                               broadcast_source_name_len;

    /*! Is public broadcast (PBP) mode enabled */
    bool                                public_broadcast_mode;

    /*! Is broadcast encrypted */
    bool                                is_encrypted_broadcast;

    /*! Code for broadcast encryption */
    uint8                               broadcast_code[USB_LEA_BROADCAST_CODE_MAX_LEN];

    /*! ID for broadcast */
    uint32                              broadcast_id;

    /*! Broadcast configuration used when PBP is not enabled */
    LE_AUDIO_CLIENT_BROADCAST_CONFIG_T  tmap_broadcast_audio_config;

    /*! Broadcast configuration used when PBP is enabled */
    LE_AUDIO_CLIENT_BROADCAST_CONFIG_T  pbp_broadcast_audio_config;

    /*! Broadcast advertisement configuration for PBP/TMAP broadcast */
    CapClientBcastSrcAdvParams          broadcast_adv_param;
} usb_dongle_lea_config_data_t;

/*! \brief  Sets the broadcast Stream capability (TmapClientStreamCapability)

     \param broadcast_stream_capability BAP configuration TmapClientStreamCapability.
     \param is_public_broadcast Is this for public broadcast source

     \return TRUE if able to set, returns FALSE otherwise.
*/
bool UsbDongle_LeaConfigSetBroadcastStreamCapability(uint32 broadcast_stream_capability, bool is_public_broadcast);

/*! \brief  Sets the broadcast BIS configuration.

     \param number_of_bis Number of BISes to configure.
     \param audio_channels_per_bis Audio channels per BIS (to determine BAP broadcast configs)

     \return TRUE if able to set, returns FALSE otherwise.
*/
bool UsbDongle_LeaConfigSetBroadcastBisConfig(uint16 number_of_bis, uint16 audio_channels_per_bis);

/*! \brief  Sets the broadcast source name.

     \param name Pointer to the broadcast source name.
     \param len Length of broadcast source name.

     \return TRUE if able to set the name, returns FALSE otherwise.
*/
bool UsbDongle_LeaConfigSetLeaBroadcastSourceName(const char *name, uint8 len);

/*! \brief Sets the broadcast code for streaming.

     \param code Pointer to the 16 octet broadcast code. If not encrypted, NULL can be passed here
     \param len Length of broadcast code.
*/
void UsbDongle_LeaConfigSetLeaBroadcastCode(const uint8 *code, uint8 len);

/*! \brief  Sets the PBP broadcast enable/disable.

     \param enable Enable/Disable PBP
*/
void UsbDongle_LeaConfigSetPbpBroadcastmode(bool enable);

/*! \brief Sets the broadcast advertisement settings

     \param adv_setting Pointer to the broadcast adv settings data
     \param len Length of broadcast adv settings data.
*/
bool UsbDongle_LeaConfigSetLeaBroadcastAdvSettings(const uint8 *adv_setting, uint8 len);

/*! \brief Sets the broadcast BAP audio configurations

     \param adv_setting Pointer to the broadcast audio config
     \param len Length of broadcast audio config data.
*/
bool UsbDongle_LeaConfigSetBroadcastAudioConfig(const uint8 *bcast_audio_config, uint8 len);

/*! \brief Sets the broadcast ID to use

     \param bcast_id Pointer to the broadcast id
     \param len Length of broadcast id
*/
bool UsbDongle_LeaConfigSetLeaBroadcastID(const uint8 *bcast_id, uint8 len);

#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

/*! \brief Initializes the LEA configurations
*/
void UsbDongle_LeaConfigInit(void);

#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */

#endif /* USB_DONGLE_LEA_CONFIG_H */
