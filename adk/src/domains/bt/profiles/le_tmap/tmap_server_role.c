/*!
\copyright  Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\ingroup    tmap_profile
\brief
*/

#include "tmap_server_role.h"
#include "cap_server_role.h"
#include "tmap_server_role_advertising.h"
#include "tmap_server_private.h"
#include "tmap_profile_mcs_tbs_private.h"

#ifdef GC_SECTIONS
/* Move all functions in KEEP_PM section to ensure they are not removed during
 * garbage collection */
#pragma unitcodesection KEEP_PM
#endif

#define TBS_SERVER_SERVICE_PRIM             (SYNERGY_EVENT_BASE + TBS_SERVER_PRIM)

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
tmap_profile_mcs_tbs_server_common_data_t tmap_server_data =
{
    .pts_mode = FALSE
};
#endif

void LeTmapServer_Init(void)
{
#ifdef ENABLE_TMAP_PROFILE
    TmapServer_SetupLeAdvertisingData();
#endif
    LeCapServer_Init();
}

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
void tmapServer_McsTbsServerInit(Task app_task)
{
    tmapProfileMcsServer_Init(app_task);
    tmapProfileTbsServer_Init(app_task);
}

void tmapServer_McsTbsServerAddConfig(gatt_cid_t cid)
{
    tmapProfileMcsServer_AddConfig(cid);
    tmapProfileTbsServer_AddConfig(cid);
}

void tmapServer_McsTbsServerRemoveConfig(gatt_cid_t cid)
{
    tmapProfileMcsServer_RemoveConfig(cid);
    tmapProfileTbsServer_RemoveConfig(cid);
}

void tmapServer_ProcessMcsTbsServerMessage(MessageId id, Message message)
{
    switch (id)
    {
        case MCS_SERVER_SERVICE_PRIM:
            tmapProfileMcsServer_ProcessMcsServerMessage(message);
        break;

        case TBS_SERVER_SERVICE_PRIM:
            tmapProfileTbsServer_ProcessTbsServerMessage(message);
        break;

        default:
        break;
    }
}

void LeTmapServer_RegisterForRemoteCallControls(Task task)
{
    tmapProfileTbsServer_RegisterForRemoteCallControls(task);
}

bool LeTmapServer_CreateCall(uint8 call_state, uint8 call_flags)
{
    return (tmapProfileTbsServer_CreateCall(call_state, call_flags, 0, NULL) == TBS_CCP_RESULT_SUCCESS);
}

bool LeTmapServer_TerminateCall(uint8 reason)
{
    return tmapProfileTbsServer_TerminateCall(reason);
}

bool LeTmapServer_IsCallPresent(void)
{
    return tmapProfileTbsServer_IsCallPresent();
}

bool LeTmapServer_IsCallInActiveState(void)
{
    return tmapProfileTbsServer_IsCallInActiveState();
}

GattTbsCallStates LeTmapServer_GetCallState(void)
{
    return tmapProfileTbsServer_GetCallState();
}

bool LeTmapServer_SetCallState(GattTbsCallStates state)
{
    return tmapProfileTbsServer_SetCallState(state);
}

GattMcsMediaStateType LeTmapServer_GetCurrentMediaState(void)
{
    return tmapProfileMcsServer_GetMediaState();
}

bool LeTmapServer_SetMediaState(GattMcsMediaStateType media_state)
{
    return tmapProfileMcsServer_SetMediaState(media_state);
}

bool LeTmapServer_SetDefaultProviderName(void)
{
    return tmapProfileTbsServer_SetProviderName(TMAP_PROFILE_TBS_SERVER_PTS_DEF_PROVIDER_NAME_LEN,
                                                TMAP_PROFILE_TBS_SERVER_PTS_DEF_PROVIDER_NAME);
}

bool LeTmapServer_SetTechnology(uint8 technology)
{
    return tmapProfileTbsServer_SetTechnology(technology);
}

bool LeTmapServer_SetDefaultUriPrefixList(void)
{
    return tmapProfileTbsServer_SetUriPrefixList(TMAP_PROFILE_TBS_SERVER_PTS_DEF_URI_PREFIX_LEN,
                                                 TMAP_PROFILE_TBS_SERVER_PTS_DEF_URI_PREFIX);
}

bool LeTmapServer_SetStatusFlags(uint16 flags)
{
    return tmapProfileTbsServer_SetStatusFlags(flags);
}

void LeTmapServer_SetValidUriFlag(bool is_valid)
{
    tmapProfileTbsServer_SetValidUriFlag(is_valid);
}

void LeTmapServer_EnablePtsMode(bool enable)
{
    tmap_server_data.pts_mode = enable;
    tmapProfileMcsServer_EnablePtsMode(enable);
}

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */
