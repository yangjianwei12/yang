/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file		kymera_broadcast_concurrency.h
\defgroup   kymera Kymera
\ingroup    audio_domain
\brief      The Kymera Broadcast Concurrency Manager API

*/

#ifndef KYMERA_BROADCAST_CONCURRENCY_H
#define KYMERA_BROADCAST_CONCURRENCY_H

#include "kymera_data.h"

#define Kymera_IsAudioBroadcasting() KymeraBroadcastConcurrency_GetLeAudioBroadcastingStatus()

#if defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined(ENABLE_SIMPLE_SPEAKER) && defined (ENABLE_LE_AUDIO_TRANSCODE_BROADCAST_SUPPORT)
/*! \brief Configure transcoding chain when transcoding is done at broadaster.
 *
 *  \param[in] rate Input sample rate.
*/
void KymeraBroadcastConcurrency_ConfigureTranscodingChain(uint32 rate);
#else
#define KymeraBroadcastConcurrency_ConfigureTranscodingChain(rate) (UNUSED(rate))
#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined(ENABLE_SIMPLE_SPEAKER) && defined (ENABLE_LE_AUDIO_TRANSCODE_BROADCAST_SUPPORT) */

#if defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined(ENABLE_SIMPLE_SPEAKER)
/*! \brief Configure broadcast splitter in order to provide additional inputs to the To-Air chain.
*/
void KymeraBroadcastConcurrency_ConfigureBroadcastSplitter(void);
#else
#define KymeraBroadcastConcurrency_ConfigureBroadcastSplitter() ((void)(0))
#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined(ENABLE_SIMPLE_SPEAKER) */

#if defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined(ENABLE_SIMPLE_SPEAKER) && !defined (ENABLE_LE_AUDIO_TRANSCODE_BROADCAST_SUPPORT)
/*! \brief Configure TTP Latency buffer in order to accommodate delay(ISO interval & PD of receiver) in the input chain
 *         so that audio played at broadcaster and receivers are in synchronization.
 *
 *  \param[in] rate Sample rate.
*/
void KymeraBroadcastConcurrency_ConfigureTTPLatencyBuffer(uint32 rate);
#else
#define KymeraBroadcastConcurrency_ConfigureTTPLatencyBuffer(rate) (UNUSED(rate))
#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined(ENABLE_SIMPLE_SPEAKER) && !defined (ENABLE_LE_AUDIO_TRANSCODE_BROADCAST_SUPPORT) */

#if defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined(ENABLE_SIMPLE_SPEAKER)
/*! \brief Set the audio broadcasting status.
*/
void KymeraBroadcastConcurrency_CreateToAirChain(void);
#else
#define KymeraBroadcastConcurrency_CreateToAirChain() ((void)(0))
#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined(ENABLE_SIMPLE_SPEAKER) */

#if defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined(ENABLE_SIMPLE_SPEAKER)
/*! \brief Set the audio broadcasting status.

    \param[in] rate Input sample rate which is provided as input to IIR Resampler.
*/
void KymeraBroadcastConcurrency_ConfigureToAirChain(uint32 rate);
#else
#define KymeraBroadcastConcurrency_ConfigureToAirChain(rate) (UNUSED(rate))
#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined(ENABLE_SIMPLE_SPEAKER) */

#if defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined(ENABLE_SIMPLE_SPEAKER)
/*! \brief Set the audio broadcasting status.
*/
void KymeraBroadcastConcurrency_JoinToAirChain(void);
#else
#define KymeraBroadcastConcurrency_JoinToAirChain() ((void)(0))
#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined(ENABLE_SIMPLE_SPEAKER) */

#if defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined(ENABLE_SIMPLE_SPEAKER)
/*! \brief Set the audio broadcasting status.
*/
void KymeraBroadcastConcurrency_StartToAirChain(void);
#else
#define KymeraBroadcastConcurrency_StartToAirChain() ((void)(0))
#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined(ENABLE_SIMPLE_SPEAKER) */

#if defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined(ENABLE_SIMPLE_SPEAKER)
/*! \brief Set the audio broadcasting status.
*/
void KymeraBroadcastConcurrency_DisconnectToAirChain(void);
#else
#define KymeraBroadcastConcurrency_DisconnectToAirChain() ((void)(0))
#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined(ENABLE_SIMPLE_SPEAKER) */

#if defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined(ENABLE_SIMPLE_SPEAKER)
/*! \brief Destroy To-Air chain.
*/
void KymeraBroadcastConcurrency_DestroyToAirChain(void);
#else
#define KymeraBroadcastConcurrency_DestroyToAirChain() ((void)(0))
#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined(ENABLE_SIMPLE_SPEAKER) */

#if defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined(ENABLE_SIMPLE_SPEAKER)
/*! \brief Set the audio broadcasting status.

    \param[in] is_broadcasting_enabled Set the broadcasting status based on enable/disable request.
*/
void KymeraBroadcastConcurrency_SetLeAudioBroadcastingStatus(bool is_broadcasting_enabled);
#else
#define KymeraBroadcastConcurrency_SetLeAudioBroadcastingStatus(is_broadcasting_enabled) (UNUSED(is_broadcasting_enabled))
#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined(ENABLE_SIMPLE_SPEAKER) */

#if defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined(ENABLE_SIMPLE_SPEAKER)
/*! \brief Get the audio broadcasting status.

    \returns Return is_audio_broadcasting status.
*/
bool KymeraBroadcastConcurrency_GetLeAudioBroadcastingStatus(void);
#else
#define KymeraBroadcastConcurrency_GetLeAudioBroadcastingStatus() (FALSE)
#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined(ENABLE_SIMPLE_SPEAKER) */

#endif // KYMERA_BROADCAST_CONCURRENCY_H
