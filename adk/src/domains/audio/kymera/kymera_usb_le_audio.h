/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       kymera_usb_le_audio.h
\brief      Kymera USB to ISO Driver
*/

#ifndef KYMERA_USB_LE_AUDIO_H
#define KYMERA_USB_LE_AUDIO_H

#ifdef INCLUDE_LE_AUDIO_USB_SOURCE

#include <usb_audio.h>
#include <sink.h>
#include <source.h>
#include <kymera.h>
#include <kymera_adaptation_audio_protected.h>

typedef struct
{
    bool vbc_enabled;
    le_media_config_t to_air_params;
    le_microphone_config_t from_air_params;

    uint8 spkr_channels;
    uint8 spkr_frame_size;
    uint8 qhs_level;
    bool pts_mode;
    bool mute_status; /* Unused */
    Sink mic_sink;
    Source spkr_src;
    uint32 spkr_sample_rate;
    uint32 mic_sample_rate;
    uint32 min_latency_us;
    uint32 max_latency_us;
    uint32 target_latency_us;
} KYMERA_INTERNAL_USB_LE_AUDIO_START_T;

/*! \brief Disconnect message for USB LE Audio. */
typedef struct
{
     Source spkr_src;
     Sink mic_sink;
     void (*kymera_stopped_handler)(Source source);
} KYMERA_INTERNAL_USB_LE_AUDIO_STOP_T;

/*! \brief Start USB LE Audio chain
    \param usb_le_audio connect parameters defined by USB LE audio source.
*/
void KymeraUsbLeAudio_Start(KYMERA_INTERNAL_USB_LE_AUDIO_START_T *usb_le_audio);

/*! \brief Stop USB LE Audio chain
    \param usb_le_audio_stop disconnect parameters defined by USB LE audio source.
*/
void KymeraUsbLeAudio_Stop(KYMERA_INTERNAL_USB_LE_AUDIO_STOP_T *usb_le_audio_stop);


/*! \brief Set table used to determine audio chain based on LE Audio parameters.

    This function must be called before audio is used.

    \param info LE audio chains mapping.
*/
void Kymera_UsbLeAudioSetToAirChainTable(const appKymeraUsbIsoChainTable *chain_table);

/*! \brief Set from air table used to determine audio chain based on LE Audio parameters.

    This function must be called before audio is used.

    \param info LE audio chains mapping.
*/
void Kymera_UsbLeAudioSetFromAirChainTable(const appKymeraIsoUsbChainTable *chain_table);

/*! \brief Set QHS level change indication .

    \param qhs_level QHS level ( Valid range 2- 6, Note only 2, 3, and 4 level are supported).
*/
void Kymera_UsbLeAudioApplyQhsRate(uint16 qhs_level);

#else

#define Kymera_UsbLeAudioApplyQhsRate(qhs_level) UNUSED(qhs_level)

#endif /* INCLUDE_LE_AUDIO_USB_SOURCE */

#endif /* KYMERA_USB_LE_AUDIO_H */
