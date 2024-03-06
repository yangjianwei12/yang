/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup voice_ui
    @{
    \brief      Message IDs of the messages send by Voice UI service
*/

#ifndef VOICE_UI_MESSAGE_IDS_H_
#define VOICE_UI_MESSAGE_IDS_H_

#include "domain_message.h"

/*! \brief Messages sent by the voice ui service to interested clients. */
typedef enum
{
    VOICE_UI_IDLE   = VOICE_UI_SERVICE_MESSAGE_BASE,
    VOICE_UI_CONNECTED,
    VOICE_UI_ACTIVE,
    VOICE_UI_MIC_OPEN,
    VOICE_UI_CAPTURE_START,
    VOICE_UI_CAPTURE_END,
    VOICE_UI_MIC_CLOSE,
    VOICE_UI_DISCONNECTED,
#ifdef INCLUDE_AMA
    VOICE_UI_AMA_UNREGISTERED,
    VOICE_UI_AMA_NOT_CONNECTED,
    VOICE_UI_AMA_PRIVACY_MODE_ENABLED,
    VOICE_UI_AMA_PRIVACY_MODE_DISABLED,
#endif  /* INCLUDE_AMA */
#ifdef INCLUDE_GAA
#ifdef INCLUDE_WUW
    VOICE_UI_GAA_DOFF,
#endif  /* INCLUDE_WUW */
#endif  /* INCLUDE_GAA */
    /*! This must be the final message */
    VOICE_UI_SERVICE_MESSAGE_END
} voice_ui_msg_id_t;

#endif // VOICE_UI_MESSAGE_IDS_H_
/*! @} */