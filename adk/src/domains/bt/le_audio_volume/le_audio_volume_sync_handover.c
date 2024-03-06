/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    le_audio_volume
    \brief      Handover interface for the LE audio synchronisation module.
*/

#if (defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST)) && defined(INCLUDE_MIRRORING)

#include "app_handover_if.h"
#include "le_audio_volume.h"
#include "le_audio_volume_sync.h"

#include <logging.h>
#include <stdlib.h>


static bool leAudioVolumeSync_Veto(void);
static void leAudioVolumeSync_Commit(bool is_primary);

REGISTER_HANDOVER_INTERFACE_NO_MARSHALLING(LE_AUDIO_VOLUME_SYNC, &leAudioVolumeSync_Veto, &leAudioVolumeSync_Commit);


/*! \brief Do veto check during handovers.

    LE audio sync will never veto a handover.

    \return FALSE always.
*/
static bool leAudioVolumeSync_Veto(void)
{
    return FALSE;
}

/*! \brief Commit to the given role when a handover is comitted.
*/
static void leAudioVolumeSync_Commit(bool is_primary)
{
    DEBUG_LOG("leAudioVolumeSync_Commit is primary %d", is_primary);

    LeAudioVolumeSync_SetRole(is_primary);
}

#endif /* (defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST)) && defined(INCLUDE_MIRRORING) */
