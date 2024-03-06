/*!
    \copyright Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
    \addtogroup ama
    @{
    \brief
        Protected API to only be used within the AMA component
*/

#ifndef AMA_VOICE_UI_HANDLE_H
#define AMA_VOICE_UI_HANDLE_H

#include <voice_ui_va_client_if.h>

voice_ui_handle_t *Ama_GetVoiceUiHandle(void);

bool Ama_IsAmaCurrentSelectedAssistant(void);

#endif // AMA_VOICE_UI_HANDLE_H
/*! @} */