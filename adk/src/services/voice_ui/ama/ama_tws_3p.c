#ifdef INCLUDE_AMA
/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    ama
    \brief      Implemenetation of AMA TWS functionality specfic to 3P
*/

#include "ama_tws.h"
#include "ama_audio.h"
#include "ama_voice_ui_handle.h"

void AmaTws_RoleChangedIndication(tws_topology_role role)
{
    if (!Ama_IsAmaCurrentSelectedAssistant())
    {
        return;
    }

    if (role == tws_topology_role_primary)
    {
        AmaAudio_StartWakeWordDetection();
    }
    else
    {
        AmaAudio_StopWakeWordDetection();
    }
}
#endif /* INCLUDE_AMA */
