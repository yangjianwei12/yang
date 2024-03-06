/*!
    \copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file       voice_ui.h
    \defgroup   voice_ui  Voice UI
    @{
        \ingroup    voice_ui_layer
        \brief      A component responsible for controlling voice assistants.

        Responsibilities:
        - Voice Ui control (like A way to start/stop voice capture as a result of some user action) 
        Notifications to be sent to the Application raised by the VA.

        The Voice Ui uses  \ref audio_domain Audio domain and \ref bt_domain BT domain.

*/

#ifndef VOICE_UI_H_
#define VOICE_UI_H_

#include "voice_ui_message_ids.h"

#if defined(INCLUDE_WUW) && !defined(INCLUDE_VOICE_UI)
    #error INCLUDE_VOICE_UI is missing in combination with INCLUDE_WUW
#endif

/*! \brief Voice UI Provider contexts */
typedef enum
{
    context_voice_ui_default = 0
} voice_ui_context_t;

/*! \brief Initialise the voice ui service

    \param init_task Unused
 */
bool VoiceUi_Init(Task init_task);

/*! \brief Checks if a VA session is in progress
    \return TRUE if a session is ongoing, FALSE otherwise
 */
#ifdef INCLUDE_VOICE_UI
bool VoiceUi_IsSessionInProgress(void);
#else
#define VoiceUi_IsSessionInProgress() FALSE
#endif

#endif /* VOICE_UI_H_ */

/*! @} */