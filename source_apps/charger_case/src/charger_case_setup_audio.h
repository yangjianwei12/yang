/*!
\copyright  Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    %%version
\file       charger_case_setup_audio.h
\brief      Module configure audio chains for charger case application
*/
#ifndef CHARGER_CASE_SETUP_AUDIO_H
#define CHARGER_CASE_SETUP_AUDIO_H

/*! \brief Configure and Set the Audio chains for ChargerCase.
    \param void.
    \return TRUE if audio configuration is success.
*/

bool ChargerCase_SetupAudio(void);

/*! \brief Configures downloadable capabilities.
    \param void.
    \return void
*/
void ChargerCase_SetBundlesConfig(void);

#endif /* CHARGER_CASE_SETUP_AUDIO_H */
