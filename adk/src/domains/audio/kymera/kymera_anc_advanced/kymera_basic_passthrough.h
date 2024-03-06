/*!
\copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       kymera_basic_passthrough.h
\brief     Header file for Basic Passthrough connected to Mic Framework in noise cancellation usecases.
*/

#ifndef KYMERA_BASIC_PASSTHROUGH_H_
#define KYMERA_BASIC_PASSTHROUGH_H_

#include "kymera_anc_common.h"
#include "operators.h"
#include "kymera_ucid.h"

/*! \brief Create the Basic Passthrough Chain for ANC client
*/
#if defined(ENABLE_ADAPTIVE_ANC)
void KymeraBasicPassthrough_Create(void);
#else
#define KymeraBasicPassthrough_Create() ((void)(0))
#endif

/*! \brief Configure the Basic Passthrough Chain for ANC client
*/
#if defined(ENABLE_ADAPTIVE_ANC)
void KymeraBasicPassthrough_Configure(const KYMERA_INTERNAL_AANC_ENABLE_T* param);
#else
#define KymeraBasicPassthrough_Configure(param) (UNUSED(param))
#endif

/*! \brief Connect the Basic Passthrough Chain for ANC client
*/
#if defined(ENABLE_ADAPTIVE_ANC)
void KymeraBasicPassthrough_Connect(void);
#else
#define KymeraBasicPassthrough_Connect() ((void)(0))
#endif

/*! \brief Start the Basic Passthrough Chain for ANC client
*/
#if defined(ENABLE_ADAPTIVE_ANC)
void KymeraBasicPassthrough_Start(void);
#else
#define KymeraBasicPassthrough_Start() ((void)(0))
#endif

/*! \brief Stop the Basic Passthrough Chain for ANC client
*/
#if defined(ENABLE_ADAPTIVE_ANC)
void KymeraBasicPassthrough_Stop(void);
#else
#define KymeraBasicPassthrough_Stop() ((void)(0))
#endif

/*! \brief Destroy the Basic Passthrough Chain for ANC client
*/
#if defined(ENABLE_ADAPTIVE_ANC)
void KymeraBasicPassthrough_Destroy(void);
#else
#define KymeraBasicPassthrough_Destroy() ((void)(0))
#endif

/*! \brief Check if Basic Passthrough Chain for ANC client
*/
#if defined(ENABLE_ADAPTIVE_ANC)
bool KymeraBasicPassthrough_IsActive(void);
#else
#define KymeraBasicPassthrough_IsActive() (FALSE)
#endif

/*! \brief Get the Playback Ref path sink for Basic Passthrough operator
*/
#if defined(ENABLE_ADAPTIVE_ANC)
Sink KymeraBasicPassthrough_GetRefPathSink(void);
#else
#define KymeraBasicPassthrough_GetRefPathSink() ((void)(0))
#endif

/*! \brief Get the FF mic path sink for Basic Passthrough operator
*/
#if defined(ENABLE_ADAPTIVE_ANC)
Sink KymeraBasicPassthrough_GetFFMicPathSink(void);
#else
#define KymeraBasicPassthrough_GetFFMicPathSink() ((void)(0))
#endif

/*! \brief Get the FB mic path sink for Basic Passthrough operator
*/
#if defined(ENABLE_ADAPTIVE_ANC)
Sink KymeraBasicPassthrough_GetFBMicPathSink(void);
#else
#define KymeraBasicPassthrough_GetFBMicPathSink() ((void)(0))
#endif

/*! \brief Get the Voice mic path sink for Basic Passthrough operator
*/
#if defined(ENABLE_ADAPTIVE_ANC)
Sink KymeraBasicPassthrough_GetVoiceMicPathSink(void);
#else
#define KymeraBasicPassthrough_GetVoiceMicPathSink() ((void)(0))
#endif

/*! \brief Get the BCM path sink for Basic Passthrough operator
*/
#if defined(ENABLE_ADAPTIVE_ANC)
Sink KymeraBasicPassthrough_GetBCMPathSink(void);
#else
#define KymeraBasicPassthrough_GetBCMPathSink() ((void)(0))
#endif

/*! \brief Get the Playback Ref path source for Basic Passthrough operator
*/
#if defined(ENABLE_ADAPTIVE_ANC)
Source KymeraBasicPassthrough_GetRefPathSource(void);
#else
#define KymeraBasicPassthrough_GetRefPathSource() ((void)(0))
#endif

/*! \brief Get the FF mic path source for Basic Passthrough operator
*/
#if defined(ENABLE_ADAPTIVE_ANC)
Source KymeraBasicPassthrough_GetFFMicPathSource(void);
#else
#define KymeraBasicPassthrough_GetFFMicPathSource() ((void)(0))
#endif

/*! \brief Get the FB mic path source for Basic Passthrough operator
*/
#if defined(ENABLE_ADAPTIVE_ANC)
Source KymeraBasicPassthrough_GetFBMicPathSource(void);
#else
#define KymeraBasicPassthrough_GetFBMicPathSource() ((void)(0))
#endif

/*! \brief Get the Voice mic path source for Basic Passthrough operator
*/
#if defined(ENABLE_ADAPTIVE_ANC)
Source KymeraBasicPassthrough_GetVoiceMicPathSource(void);
#else
#define KymeraBasicPassthrough_GetVoiceMicPathSource() ((void)(0))
#endif

/*! \brief Get the BCM path source for Basic Passthrough operator
*/
#if defined(ENABLE_ADAPTIVE_ANC)
Source KymeraBasicPassthrough_GetBCMPathSource(void);
#else
#define KymeraBasicPassthrough_GetBCMPathSource() ((void)(0))
#endif

#endif /*KYMERA_BASIC_PASSTHROUGH_H_*/
