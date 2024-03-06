/*!
\copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       kymera_aah.h
\brief     Header file for Adverse acoustic event handling in noise cancellation usecases.
*/

#ifndef KYMERA_AAH_H_
#define KYMERA_AAH_H_

#include "kymera_anc_common.h"
#include "operators.h"
#include "kymera_ucid.h"

/* Compiler error checks for macros that are supported under ANC V2 only */
#ifndef INCLUDE_ANC_V2
    #if defined(ENABLE_ANC_AAH)
        #error ENABLE_ANC_AAH  can be used along with INCLUDE_ANC_V2 only
    #endif
#endif

/*! \brief Create the Adverse acoustic event handling Chain
*/
#if defined(ENABLE_ADAPTIVE_ANC) && defined(ENABLE_ANC_AAH)
void KymeraAah_Create(void);
#else
#define KymeraAah_Create() ((void)(0))
#endif

/*! \brief Configure the Adverse acoustic event handling
*/
#if defined(ENABLE_ADAPTIVE_ANC) && defined(ENABLE_ANC_AAH)
void KymeraAah_Configure(const KYMERA_INTERNAL_AANC_ENABLE_T* param);
#else
#define KymeraAah_Configure(param) (UNUSED(param))
#endif

/*! \brief Connect the Adverse acoustic event handling
*/
#if defined(ENABLE_ADAPTIVE_ANC) && defined(ENABLE_ANC_AAH)
void KymeraAah_Connect(void);
#else
#define KymeraAah_Connect() ((void)(0))
#endif

/*! \brief Start the Adverse acoustic event handling
*/
#if defined(ENABLE_ADAPTIVE_ANC) && defined(ENABLE_ANC_AAH)
void KymeraAah_Start(void);
#else
#define KymeraAah_Start() ((void)(0))
#endif

/*! \brief Set the Adverse acoustic event System control mode
*/
#if defined(ENABLE_ADAPTIVE_ANC) && defined(ENABLE_ANC_AAH)
void KymeraAah_SetSysMode(aah_sysmode_t mode);
#else
#define KymeraAah_SetSysMode(mode) (UNUSED(mode))
#endif

/*! \brief Get current state of Adverse acoustic handler
*/
#if defined(ENABLE_ADAPTIVE_ANC) && defined(ENABLE_ANC_AAH)
bool KymeraAah_GetCurrentState(void);
#else
#define KymeraAah_GetCurrentState(void) (FALSE)
#endif

/*! \brief Stop the Adverse acoustic event handling
*/
#if defined(ENABLE_ADAPTIVE_ANC) && defined(ENABLE_ANC_AAH)
void KymeraAah_Stop(void);
#else
#define KymeraAah_Stop() ((void)(0))
#endif

/*! \brief Disconnect the Adverse acoustic event handling output sources
*/
#if defined(ENABLE_ADAPTIVE_ANC) && defined(ENABLE_ANC_AAH)
void KymeraAah_Disconnect(void);
#else
#define KymeraAah_Disconnect() ((void)(0))
#endif


/*! \brief Destroy the Adverse acoustic event handling
*/
#if defined(ENABLE_ADAPTIVE_ANC) && defined(ENABLE_ANC_AAH)
void KymeraAah_Destroy(void);
#else
#define KymeraAah_Destroy() ((void)(0))
#endif

/*! \brief Check if Adverse acoustic event handling chain active
*/
#if defined(ENABLE_ADAPTIVE_ANC) && defined(ENABLE_ANC_AAH)
bool KymeraAah_IsActive(void);
#else
#define KymeraAah_IsActive() (FALSE)
#endif

/*! \brief Apply mode change to Adverse acoustic event handling chain
*/
#if defined(ENABLE_ADAPTIVE_ANC) && defined(ENABLE_ANC_AAH)
void KymeraAah_ApplyModeChange(const KYMERA_INTERNAL_AANC_ENABLE_T* param);
#else
#define KymeraAah_ApplyModeChange(param) (UNUSED(param))
#endif

/*! \brief Get the Playback Ref path sink for AAH operator
*/
#if defined(ENABLE_ADAPTIVE_ANC) && defined(ENABLE_ANC_AAH)
Sink KymeraAah_GetRefPathSink(void);
#else
#define KymeraAah_GetRefPathSink() ((Sink)(0))
#endif

/*! \brief Get the FF mic path sink for AAH operator
*/
#if defined(ENABLE_ADAPTIVE_ANC) && defined(ENABLE_ANC_AAH)
Sink KymeraAah_GetFFMicPathSink(void);
#else
#define KymeraAah_GetFFMicPathSink() ((Sink)(0))
#endif

/*! \brief Get the FB mic path sink for AAH operator
*/
#if defined(ENABLE_ADAPTIVE_ANC) && defined(ENABLE_ANC_AAH)
Sink KymeraAah_GetFBMicPathSink(void);
#else
#define KymeraAah_GetFBMicPathSink() ((Sink)(0))
#endif

/*! \brief Get the Playback Ref path source for AAH operator
*/
#if defined(ENABLE_ADAPTIVE_ANC) && defined(ENABLE_ANC_AAH)
Source KymeraAah_GetRefPathSource(void);
#else
#define KymeraAah_GetRefPathSource() ((Source)(0))
#endif

/*! \brief Get the FF mic path source for AAH operator
*/
#if defined(ENABLE_ADAPTIVE_ANC) && defined(ENABLE_ANC_AAH)
Source KymeraAah_GetFFMicPathSource(void);
#else
#define KymeraAah_GetFFMicPathSource() ((Source)(0))
#endif

/*! \brief Get the FB mic path source for AAH operator
*/
#if defined(ENABLE_ADAPTIVE_ANC) && defined(ENABLE_ANC_AAH)
Source KymeraAah_GetFBMicPathSource(void);
#else
#define KymeraAah_GetFBMicPathSource() ((Source)(0))
#endif

/*! \brief Set AAH limits depending on standalone or concurrency mode
*/
#if defined(ENABLE_ADAPTIVE_ANC) && defined(ENABLE_ANC_AAH)
void KymeraAah_SetLimitsForConcurrency(void);
void KymeraAah_SetLimitsForStandalone(void);
#else
#define KymeraAah_SetLimitsForConcurrency() ((void)(0))
#define KymeraAah_SetLimitsForStandalone() ((void)(0))
#endif

/*!
    \brief API to identify if AAH feature is supported or not
    \param None
    \return TRUE for feature supported otherwise FALSE
*/
#if defined(ENABLE_ADAPTIVE_ANC) && defined(ENABLE_ANC_AAH)
bool KymeraAah_IsFeatureSupported(void);
#else
#define KymeraAah_IsFeatureSupported() (FALSE)
#endif

#endif /*KYMERA_AAH_H_*/

