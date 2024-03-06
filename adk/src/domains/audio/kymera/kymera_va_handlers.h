/*!
\copyright  Copyright (c) 2019-2021 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera module that implements basic build block functions to handle Voice Assistant related actions
*/

#ifndef KYMERA_VA_HANDLERS_H_
#define KYMERA_VA_HANDLERS_H_

#include "va_audio_types.h"

typedef struct
{
    Task handler;
    const va_audio_wuw_detection_params_t *params;
} wuw_detection_start_t;

/*! \param params Must be valid pointer to va_audio_voice_capture_params_t
*/
void KymeraVaHandler_CreateMicChainForLiveCapture(const void *params);

/*! \param params Must be valid pointer to wuw_detection_start_t
*/
void KymeraVaHandler_CreateMicChainForWuw(const void *params);

/*! \param params Ignored (can be NULL)
*/
void KymeraVaHandler_ConnectToMics(const void *params);

/*! \param params Ignored (can be NULL)
*/
void KymeraVaHandler_DisconnectFromMics(const void *params);

/*! \param params Ignored (can be NULL)
*/
void KymeraVaHandler_StartMicChain(const void *params);

/*! \param params Ignored (can be NULL)
*/
void KymeraVaHandler_StopMicChain(const void *params);

/*! \param params Ignored (can be NULL)
*/
void KymeraVaHandler_DestroyMicChain(const void *params);

/*! \param params Ignored (can be NULL)
*/
void KymeraVaHandler_ActivateMicChainWuwOutput(const void *params);

/*! \param params Ignored (can be NULL)
*/
void KymeraVaHandler_DeactivateMicChainWuwOutput(const void *params);

/*! \param params Ignored (can be NULL)
*/
void KymeraVaHandler_ActivateMicChainEncodeOutputForLiveCapture(const void *params);

/*! \param params Must be valid pointer to va_audio_wuw_capture_params_t
*/
void KymeraVaHandler_ActivateMicChainEncodeOutputForWuwCapture(const void *params);

/*! \param params Ignored (can be NULL)
*/
void KymeraVaHandler_DeactivateMicChainEncodeOutput(const void *params);

/*! \param params Ignored (can be NULL)
*/
void KymeraVaHandler_BufferMicChainEncodeOutput(const void *params);


/*! \brief Creates or reconfigures the encode chain as needed.
    \param params Must be valid pointer to va_audio_voice_capture_params_t
*/
void KymeraVaHandler_CreateEncodeChainForLiveCapture(const void *params);

/*! \brief Creates or reconfigures the encode chain as needed.
    \param params Must be valid pointer to va_audio_wuw_capture_params_t
*/
void KymeraVaHandler_CreateEncodeChainForWuwCapture(const void *params);

/*! \param params Ignored (can be NULL)
*/
void KymeraVaHandler_StartEncodeChain(const void *params);

/*! \param params Ignored (can be NULL)
*/
void KymeraVaHandler_StopEncodeChain(const void *params);

/*! \brief Destroys encode chain only if an instance exists.
    \param params Ignored (can be NULL)
*/
void KymeraVaHandler_DestroyEncodeChain(const void *params);


/*! \param params Must be valid pointer to wuw_detection_start_t
*/
void KymeraVaHandler_CreateWuwChain(const void *params);

/*! \param params Ignored (can be NULL)
*/
void KymeraVaHandler_StartWuwChain(const void *params);

/*! \param params Ignored (can be NULL)
*/
void KymeraVaHandler_StopWuwChain(const void *params);

/*! \param params Ignored (can be NULL)
*/
void KymeraVaHandler_DestroyWuwChain(const void *params);

/*! \param params Ignored (can be NULL)
*/
void KymeraVaHandler_ConnectWuwChainToMicChain(const void *params);


/*! \param params Ignored (can be NULL)
*/
void KymeraVaHandler_StartGraphManagerDelegation(const void *params);

/*! \param params Ignored (can be NULL)
*/
void KymeraVaHandler_StopGraphManagerDelegation(const void *params);


/*! \brief Makes sure the audio framework is kept on.
           This is required when no chains are instantiated.
    \param params Ignored (can be NULL).
*/
void KymeraVaHandler_EnterKeepDspOn(const void *params);

/*! \param params Ignored (can be NULL).
*/
void KymeraVaHandler_ExitKeepDspOn(const void *params);

/*! \param params Ignored (can be NULL)
*/
void KymeraVaHandler_UpdateDspKickPeriod(const void *params);

/*! \param params Ignored (can be NULL)
*/
void KymeraVaHandler_UpdateDspClock(const void *params);

/*! \param params Ignored (can be NULL).
*/
void KymeraVaHandler_BoostClockForChainCreation(const void *params);

/*! \param params Must be valid pointer to wuw_detection_start_t
*/
void KymeraVaHandler_SetWuwSampleRate(const void *params);

/*! \param params Must be valid pointer to va_audio_voice_capture_params_t
*/
void KymeraVaHandler_SetLiveCaptureSampleRate(const void *params);

/*! \param params Ignored (can be NULL).
*/
void KymeraVaHandler_LoadDownloadableCapsForPrompt(const void *params);

/*! \param params Ignored (can be NULL).
*/
void KymeraVaHandler_UnloadDownloadableCapsForPrompt(const void *params);

/*! \brief Interrogate the kymera VA component about its low power state
    \return TRUE if low power is enabled for VA, FALSE otherwise
*/
bool Kymera_VaIsLowPowerEnabled(void);

/*! \brief Interrogate the kymera VA component about whether it can support
           the default low power clock
    \return TRUE if the default low power clock is supported, FALSE otherwise
*/
bool Kymera_WuwEngineSupportsDefaultLpClock(void);

#endif /* KYMERA_VA_HANDLERS_H_ */
