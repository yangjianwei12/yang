/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version
\file       kymera_analog_le_audio.h
\brief      Kymera Wired (Analog) to ISO Driver
*/

#ifndef KYMERA_ANALOG_LE_AUDIO_H
#define KYMERA_ANALOG_LE_AUDIO_H

#include <kymera_adaptation_audio_protected.h>

typedef struct
{
    le_media_config_t to_air_params;

    uint32 sample_rate;

    uint32 min_latency_us;
    uint32 max_latency_us;
    uint32 target_latency_us;
} KYMERA_INTERNAL_ANALOG_LE_AUDIO_START_T;

#ifdef INCLUDE_LE_AUDIO_ANALOG_SOURCE

/*! \brief Start Analog (Line In) LE Audio chain
    \param wired_le_audio connect parameters defined by both Analog source and LE audio sink.
*/
void KymeraAnalogLeAudio_Start(KYMERA_INTERNAL_ANALOG_LE_AUDIO_START_T *wired_le_audio);

/*! \brief Stop Analog LE Audio chain
*/
void KymeraAnalogLeAudio_Stop(void);


/*! \brief Set table used to determine audio chain based on LE Audio parameters.

    This function must be called before audio is used.

    \param info LE audio chains mapping.
*/
void KymeraAnalogLeAudio_SetToAirChainTable(const appKymeraAnalogIsoChainTable *chain_table);

#else

#define KymeraAnalogLeAudio_Start(wired_le_audio)                UNUSED(wired_le_audio)
#define KymeraAnalogLeAudio_Stop()                               ((void)(0))
#define KymeraAnalogLeAudio_SetToAirChainTable(chain_table)      UNUSED(chain_table)

#endif /* INCLUDE_LE_AUDIO_ANALOG_SOURCE */

#endif // KYMERA_ANALOG_LE_AUDIO_H
