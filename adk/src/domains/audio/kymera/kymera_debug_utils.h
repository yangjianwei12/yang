/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       kymera_debug_utils.h
\defgroup   kymera Kymera
\ingroup    audio_domain

\brief      Kymera Audio interface to monitor audio-DSP characteristics for debug purpose
*/
#ifndef KYMERA_DEBUG_UTILS_H
#define KYMERA_DEBUG_UTILS_H
#ifdef INCLUDE_KYMERA_AUDIO_DEBUG
/*! \brief Enable notifications carrying Audio-DSP debug information for any audio use case.

    This API shall be invoked 
    - To monitor the MCPS of audio subsystem at regular intervals.
    - To monitor A2DP Sequence errors in A2DP use cases.
    - To monitor A2DP TTP statistics for the recieved packets (Both Q2Q and Non-Q2Q)
    - To monitor error packets recieved in case of Voice decode use cases (NB / WBS)
    - To monitor error packets recieved in case of LEA decode use cases (LEA Voice / LEA Music)

    In case of earbuds, This API should be called individually on primary as well as secondary earbuds respectively
    and #KymeraDebugUtil_StopAudioDspMonitor shall be called to disable MCPS monitoring.

    Application shall call this API only when audio-DSP is powered on i.e. after exercising
    OperatorFrameworkEnable(MAIN_PROCESSOR_ON)
*/
void KymeraDebugUtil_StartAudioDspMonitor(void);

/*! \brief Disable Audio-DSP debug notifications.

    In case of earbuds, This API should be called individually on primary as well as secondary earbuds respectively
    Application shall call this API before powering off audio DSP i.e. before exercising 
    OperatorFrameworkEnable(MAIN_PROCESSOR_OFF)
*/
void KymeraDebugUtil_StopAudioDspMonitor(void);

/*! \brief To print different metrics from A2DP Decoder

    - sequence number errors from the RTP Headers of audio packet (if there are any)
    - Print  TTP statistics for use cases invloving decoders. Array of 6 values as explained below
        query_ttp_stats[0] => No of packets arrived with TTP < 0ms
        query_ttp_stats[1] => No of packets arrived with TTP > 0ms and < 25ms
        query_ttp_stats[2] => No of packets arrived with TTP > 25ms and < 50ms
        query_ttp_stats[3] => No of packets arrived with TTP > 50ms and < 100ms
        query_ttp_stats[4] => No of packets arrived with TTP > 100ms and < 150ms
        query_ttp_stats[5] => No of packets arrived with TTP > 150ms

    In case of earbuds, This API should be called individually on primary as well as secondary earbuds respectively
    Application shall call this API before powering off audio DSP i.e. before exercising 
    OperatorFrameworkEnable(MAIN_PROCESSOR_OFF)

    Note: This API is not useful in case of aptX Classic as a codec because audio packets in case of aptX Classic does
    not contain RTP Headers from which the information about the sequential errors are obtained for other codecs!
*/

void KymeraDebugUtil_GetA2dpDecoderStats(void);

/*! \brief To get the no of error frames recived over SCO path

    In case of earbuds, This API should be called individually on primary as well as secondary earbuds respectively
    Application shall call this API before powering off audio DSP i.e. before exercising 
    OperatorFrameworkEnable(MAIN_PROCESSOR_OFF)
*/
void KymeraDebugUtil_GetScoDecoderStats(void);

#if defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST)
/*! \brief To get the no of error frames in case LE Audio use cases

    In case of earbuds, This API should be called individually on primary as well as secondary earbuds respectively
    Application shall call this API before powering off audio DSP i.e. before exercising 
    OperatorFrameworkEnable(MAIN_PROCESSOR_OFF)
*/
void KymeraDebugUtil_GetLc3DecoderStats(void);
#endif

#else

#define KymeraDebugUtil_StartAudioDspMonitor() /* Nothing to do */

#define KymeraDebugUtil_StopAudioDspMonitor() /* Nothing to do */

#define KymeraDebugUtil_GetA2dpDecoderStats() /* Nothing to do */

#define KymeraDebugUtil_GetScoDecoderStats() /* Nothing to do */

#define KymeraDebugUtil_GetLc3DecoderStats() /* Nothing to do */

#endif /* INCLUDE_KYMERA_AUDIO_DEBUG */

#endif /* KYMERA_DEBUG_UTILS_H */
