/*!
\copyright  Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       kymera_fit_test.h
\brief      Private header to connect/manage to Earbud fit test audio graph
*/

#ifndef KYMERA_FIT_TEST_H_
#define KYMERA_FIT_TEST_H_

#include <operators.h>

/*! \brief Registers AANC callbacks in the mic interface layer
*/
#if defined(ENABLE_EARBUD_FIT_TEST) || defined(ENABLE_CONTINUOUS_EARBUD_FIT_TEST)
void KymeraFitTest_Init(void);
#else
#define KymeraFitTest_Init() ((void)(0))
#endif

#if defined(ENABLE_EARBUD_FIT_TEST) || defined(ENABLE_CONTINUOUS_EARBUD_FIT_TEST)
bool KymeraFitTest_IsEftJingleActive(void);
#else
#define KymeraFitTest_IsEftJingleActive() (FALSE)
#endif

#if defined(ENABLE_EARBUD_FIT_TEST) || defined(ENABLE_CONTINUOUS_EARBUD_FIT_TEST)
bool KymeraFitTest_IsEftContinuousFitActive(void);
#else
#define KymeraFitTest_IsEftContinuousFitActive() (FALSE)
#endif

#if defined(ENABLE_EARBUD_FIT_TEST)
void KymeraFitTest_Start(void);
#else
#define KymeraFitTest_Start() ((void)(0))
#endif

#if defined(ENABLE_EARBUD_FIT_TEST)
void KymeraFitTest_Stop(void);
#else
#define KymeraFitTest_Stop() ((void)(0))
#endif

#if defined(ENABLE_EARBUD_FIT_TEST)
void KymeraFitTest_CancelPrompt(void);
#else
#define KymeraFitTest_CancelPrompt() ((void)(0))
#endif

#if defined(ENABLE_EARBUD_FIT_TEST)
FILE_INDEX KymeraFitTest_GetPromptIndex(void);
#else
#define KymeraFitTest_GetPromptIndex() (0)
#endif

#if defined(ENABLE_EARBUD_FIT_TEST)
bool KymeraFitTest_PromptReplayRequired(void);
#else
#define KymeraFitTest_PromptReplayRequired() (FALSE)
#endif

#if defined(ENABLE_EARBUD_FIT_TEST)
bool KymeraFitTest_IsTuningModeActive(void);
#else
#define KymeraFitTest_IsTuningModeActive() (FALSE)
#endif

#if defined(ENABLE_EARBUD_FIT_TEST)
void KymeraFitTest_EnableAanc(void);
#else
#define KymeraFitTest_EnableAanc() ((void)0)
#endif

#if defined(ENABLE_EARBUD_FIT_TEST)
void KymeraFitTest_DisableAanc(void);
#else
#define KymeraFitTest_DisableAanc() ((void)0)
#endif

#if defined(ENABLE_EARBUD_FIT_TEST)
void KymeraFitTest_StartPrompt(void);
#else
#define KymeraFitTest_StartPrompt() ((void)0)
#endif

/* Continuous Earbud Fit Test */

#if defined(ENABLE_CONTINUOUS_EARBUD_FIT_TEST)
void KymeraFitTest_ContinuousEnableEftMicClient(void);
#else
#define KymeraFitTest_ContinuousEnableEftMicClient() ((void)0)
#endif

#if defined(ENABLE_CONTINUOUS_EARBUD_FIT_TEST)
void KymeraFitTest_ContinuousDisableEftMicClient(void);
#else
#define KymeraFitTest_ContinuousDisableEftMicClient() ((void)0)
#endif

#if defined(ENABLE_CONTINUOUS_EARBUD_FIT_TEST)
void KymeraFitTest_ContinuousGetCapturedBins(int16 signal_id, int16 part_id);
#else
#define KymeraFitTest_ContinuousGetCapturedBins(x,y) (UNUSED(x); UNUSED(y))
#endif

#if defined(ENABLE_CONTINUOUS_EARBUD_FIT_TEST)
void KymeraFitTest_ContinuousStartCapture(void);
#else
#define KymeraFitTest_ContinuousStartCapture() ((void)0)
#endif

#if defined(ENABLE_CONTINUOUS_EARBUD_FIT_TEST)
void KymeraFitTest_ContinuousSetSysMode(bool enable);
#else
#define KymeraFitTest_ContinuousSetSysMode(x) (UNUSED(x))
#endif


#endif /* KYMERA_FIT_TEST_H_ */
