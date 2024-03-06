/*!
\copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version
\file      kymera_noise_id.h
\brief     Implementation of Kymera Noise ID and related functionality
*/

#ifndef KYMERA_NOISE_ID_H
#define KYMERA_NOISE_ID_H

#include "operators.h"
#include "kymera_ucid.h"


/*! \brief Get the Noise ID Chain
*/
#if defined(ENABLE_ADAPTIVE_ANC)
kymera_chain_handle_t KymeraNoiseID_GetChain(void);
#else
#define KymeraNoiseID() ((void)(0))
#endif

/*! \brief Create the NoiseID Chain
*/
#if defined(ENABLE_ADAPTIVE_ANC)
void KymeraNoiseID_Create(void);
#else
#define KymeraNoiseID_Create() ((void)(0))
#endif

/*! \brief Configure the NoiseID Chain
*/
#if defined(ENABLE_ADAPTIVE_ANC)
void KymeraNoiseID_Configure(void);
#else
#define KymeraNoiseID_Configure() ((void)(0))
#endif

/*! \brief Connect the NoiseID Chain
*/
#if defined(ENABLE_ADAPTIVE_ANC)
void KymeraNoiseID_Connect(void);
#else
#define KymeraNoiseID_Connect() ((void)(0))
#endif

/*! \brief Disconnect the NoiseID Chain
*/
#if defined(ENABLE_ADAPTIVE_ANC)
void KymeraNoiseID_Disconnect(void);
#else
#define KymeraNoiseID_Disconnect() ((void)(0))
#endif

/*! \brief Get the NoiseID mic sink
*/
#if defined(ENABLE_ADAPTIVE_ANC)
Sink KymeraNoiseID_GetFFMicPathSink(void);
#else
#define KymeraNoiseID_GetFFMicPathSink() ((Sink)(0))
#endif

/*! \brief Get the NoiseID mic source
*/
#if defined(ENABLE_ADAPTIVE_ANC)
Source KymeraNoiseID_GetFFMicPathSource(void);
#else
#define KymeraNoiseID_GetFFMicPathSource() ((Source)(0))
#endif

/*! \brief Start the NoiseID Chain
*/
#if defined(ENABLE_ADAPTIVE_ANC)
void KymeraNoiseID_Start(void);
#else
#define KymeraNoiseID_Start() ((void)(0))
#endif

/*! \brief Enable Noise ID processing
*/
#if defined(ENABLE_ADAPTIVE_ANC)
void KymeraNoiseID_Enable(void);
#else
#define KymeraNoiseID_Enable() ((void)(0))
#endif

/*! \brief Disable Noise ID processing
*/
#if defined(ENABLE_ADAPTIVE_ANC)
void KymeraNoiseID_Disable(void);
#else
#define KymeraNoiseID_Disable() ((void)(0))
#endif

/*! \brief Stop the NoiseID Chain
*/
#if defined(ENABLE_ADAPTIVE_ANC)
void KymeraNoiseID_Stop(void);
#else
#define KymeraNoiseID_Stop() ((void)(0))
#endif

/*! \brief Destroy the NoiseID detect Chain
*/
#if defined(ENABLE_ADAPTIVE_ANC)
void KymeraNoiseID_Destroy(void);
#else
#define KymeraNoiseID_Destroy() ((void)(0))
#endif

/*! \brief Check if NoiseID is active
*/
#if defined(ENABLE_ADAPTIVE_ANC)
bool KymeraNoiseID_IsActive(void);
#else
#define KymeraNoiseID_IsActive() (FALSE)
#endif

/*! \brief Set NoiseID sys mode
*/
#if defined(ENABLE_ADAPTIVE_ANC)
void KymeraNoiseID_SetSysMode(noise_id_sysmode_t mode);
#else
#define KymeraNoiseID_SetSysMode(mode) (UNUSED(mode))
#endif

/*! \brief Set current Noise ID category in capability
*/
#if defined(ENABLE_ADAPTIVE_ANC)
void KymeraNoiseID_SetCurrentCategory(noise_id_category_t category);
#else
#define KymeraNoiseID_SetCurrentCategory(category) (UNUSED(category))
#endif

/*! \brief Get current Noise ID category from capability
*/
#if defined(ENABLE_ADAPTIVE_ANC)
get_status_data_t* KymeraNoiseID_GetStatusData(void);
#else
#define KymeraNoiseID_GetStatusData(void) ((void)(0))
#endif

/*! \brief Get current Noise ID category from capability
*/
#if defined(ENABLE_ADAPTIVE_ANC)
void KymeraNoiseID_GetCurrentNoiseCategory(noise_id_category_t *category);
#else
#define KymeraNoiseID_GetCurrentNoiseCategory(category) (UNUSED(category))
#endif

/*! \brief Set Noise ID category in capability based on the current mode
*/
#if defined(ENABLE_ADAPTIVE_ANC)
void KymeraNoiseID_SetCategoryBasedOnCurrentMode(void);
#else
#define KymeraNoiseID_SetCategoryBasedOnCurrentMode() ((void)(0))
#endif

#endif /* KYMERA_NOISE_ID_H */
