/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \file       state_proxy_anc.c
    \ingroup    state_proxy
    \brief      State proxy anc event handling.
*/

/* local includes */
#include "state_proxy.h"
#include "state_proxy_private.h"
#include "state_proxy_marshal_defs.h"
#include "state_proxy_anc.h"
#include "system_clock.h"

/* framework includes */
#include <peer_signalling.h>

/* system includes */
#include <panic.h>
#include <logging.h>
#include <stdlib.h>

#define stateProxyGetToggleIndex(toggle_way_id) (toggle_way_id - anc_toggle_way_config_id_1)

#define stateProxy_GetMsgId(anc_msg_id) (anc_msg_id - ANC_MESSAGE_BASE)

static size_t stateProxy_GetMsgIdSpecificSize(state_proxy_anc_msg_id_t id)
{
    size_t size = 0;

    switch (id)
    {
        case state_proxy_anc_msg_id_mode:
            size = sizeof(ANC_UPDATE_MODE_CHANGED_IND_T);
            break;

        case state_proxy_anc_msg_id_gain:
            size = sizeof(ANC_UPDATE_GAIN_IND_T);
            break;

        case state_proxy_anc_msg_id_toggle_config:
            size = sizeof(ANC_TOGGLE_WAY_CONFIG_UPDATE_IND_T);
            break;

        case state_proxy_anc_msg_id_scenario_config:
            size = sizeof(ANC_SCENARIO_CONFIG_UPDATE_IND_T);
            break;

        case state_proxy_anc_msg_id_world_volume_gain:
            size = sizeof(ANC_WORLD_VOLUME_GAIN_DB_UPDATE_IND_T);
            break;

        case state_proxy_anc_msg_id_world_volume_balance:
            size = sizeof(ANC_WORLD_VOLUME_BALANCE_UPDATE_IND_T);
            break;
            
        case state_proxy_anc_msg_id_prev_config:
            size = sizeof(ANC_UPDATE_PREV_CONFIG_IND_T);
        break;
        
        case state_proxy_anc_msg_id_prev_mode:
            size = sizeof(ANC_UPDATE_PREV_MODE_IND_T);
        break;

       case state_proxy_aanc_msg_id_fb_gain:
           size = sizeof(AANC_FB_GAIN_UPDATE_IND_T);
       break;

        case state_proxy_anc_msg_id_reconnection:
            size = sizeof(STATE_PROXY_RECONNECTION_ANC_DATA_T);
       break;

        default:
            size = 0;
            break;
    }

    return size;
}

static void stateProxy_MarshalAncDataToPeer(state_proxy_anc_msg_id_t id, const void* msg)
{
    if(!stateProxy_Paused() && appPeerSigIsConnected())
    {
        void* copy;
        STATE_PROXY_ANC_DATA_T* anc_data = PanicUnlessMalloc(sizeof(STATE_PROXY_ANC_DATA_T));
        marshal_type_t marshal_type = MARSHAL_TYPE(STATE_PROXY_ANC_DATA_T);

        anc_data->msg_id = id;

        /* msg may be NULL for message without payload */
        if(msg)
        {
            size_t msg_size_specific = stateProxy_GetMsgIdSpecificSize(id);
            memcpy(&anc_data->msg, msg, msg_size_specific);
        }

        copy = PanicUnlessMalloc(sizeof(*anc_data));
        memcpy(copy, anc_data, sizeof(*anc_data));
        appPeerSigMarshalledMsgChannelTx(stateProxy_GetTask(),
                                         PEER_SIG_MSG_CHANNEL_STATE_PROXY,
                                         copy, marshal_type);

        free(anc_data);
    }
}

static void stateProxy_UpdateAncState(state_proxy_data_t* state_proxy_data, bool state)
{
    state_proxy_data->flags.anc_state = state;
}

static void stateProxy_UpdateAncMode(state_proxy_data_t* state_proxy_data, uint8 anc_mode)
{
    state_proxy_data->anc_mode = anc_mode;
}

static void stateProxy_UpdateAncLeakthroughGain(state_proxy_data_t* state_proxy_data, uint8 anc_leakthrough_gain)
{
    state_proxy_data->anc_leakthrough_gain = anc_leakthrough_gain;
}

static void stateProxy_UpdateAncToggleConfig(state_proxy_data_t* state_proxy_data, uint8 config_id, uint8 config)
{
    state_proxy_data->toggle_configurations.anc_toggle_way_config[stateProxyGetToggleIndex(config_id)] = config;
}

static void stateProxy_UpdateAncScenarioConfig(state_proxy_data_t* state_proxy_data, uint8 config_id, uint8 config)
{
    switch(config_id)
    {
        case anc_scenario_config_id_standalone:
            state_proxy_data->standalone_config = config;
            break;

        case anc_scenario_config_id_playback:
            state_proxy_data->playback_config = config;
            break;

        case anc_scenario_config_id_sco:
            state_proxy_data->sco_config = config;
            break;

        case anc_scenario_config_id_va:
            state_proxy_data->va_config = config;
            break;

#ifdef INCLUDE_LE_STEREO_RECORDING
        case anc_scenario_config_id_stereo_recording_le:
            state_proxy_data->stereo_recording_le_config = config;
            break;
#endif /* INCLUDE_LE_STEREO_RECORDING */		
    }
}

static void stateProxy_UpdateAncDemoState(state_proxy_data_t* state_proxy_data, bool state)
{
    state_proxy_data->flags.anc_demo_state = state;
}

static void stateProxy_UpdateAncAdaptivityStatus(state_proxy_data_t* state_proxy_data, bool state)
{
    state_proxy_data->flags.adaptivity_status = state;
}

static void stateProxy_UpdateAncWorldVolumeGain(state_proxy_data_t* state_proxy_data, int8 world_volume_gain_dB)
{
    state_proxy_data->world_volume_gain_dB = world_volume_gain_dB;
}

static void stateProxy_UpdateAncWorldVolumeBalance(state_proxy_data_t* state_proxy_data, bool balance_device_side, uint8 balance_percentage)
{
    state_proxy_data->balance_info.balance_device_side = balance_device_side;
    state_proxy_data->balance_info.balance_percentage = balance_percentage;
}

static void stateProxy_HandleLocalAncStateUpdate(MessageId id)
{
    state_proxy_data_t* state_proxy_data = stateProxy_GetData(state_proxy_source_local);

    stateProxy_UpdateAncState(state_proxy_data, (id == ANC_UPDATE_STATE_ENABLE_IND));
}

static void stateProxy_HandleLocalAncModeUpdate(ANC_UPDATE_MODE_CHANGED_IND_T* anc_data)
{
    state_proxy_data_t* state_proxy_data = stateProxy_GetData(state_proxy_source_local);
    uint8 anc_mode = anc_data->mode;

    stateProxy_UpdateAncMode(state_proxy_data, anc_mode);
}

static void stateProxy_HandleLocalAncPrevConfigUpdate(ANC_UPDATE_PREV_CONFIG_IND_T* anc_data)
{
    state_proxy_data_t* state_proxy_data = stateProxy_GetData(state_proxy_source_local);
    state_proxy_data->previous_config = anc_data->previous_config;
}

static void stateProxy_HandleLocalAncPrevModeUpdate(ANC_UPDATE_PREV_MODE_IND_T* anc_data)
{
    state_proxy_data_t* state_proxy_data = stateProxy_GetData(state_proxy_source_local);
    state_proxy_data->previous_mode = anc_data->previous_mode;
}

static void stateProxy_HandleLocalAncGainUpdate(ANC_UPDATE_GAIN_IND_T* anc_data)
{
    state_proxy_data_t* state_proxy_data = stateProxy_GetData(state_proxy_source_local);
    uint8 anc_gain = anc_data->anc_gain;

    stateProxy_UpdateAncLeakthroughGain(state_proxy_data, anc_gain);
}

static void stateProxy_HandleLocalAncToggleConfigUpdate(ANC_TOGGLE_WAY_CONFIG_UPDATE_IND_T* anc_data)
{
    state_proxy_data_t* state_proxy_data = stateProxy_GetData(state_proxy_source_local);
    uint8 config_id = anc_data->anc_toggle_config_id;
    uint8 config = anc_data->anc_config;

    stateProxy_UpdateAncToggleConfig(state_proxy_data, config_id, config);
}

static void stateProxy_HandleLocalScenarioConfigUpdate(ANC_SCENARIO_CONFIG_UPDATE_IND_T* anc_data)
{
    state_proxy_data_t* state_proxy_data = stateProxy_GetData(state_proxy_source_local);
    uint8 config_id = anc_data->anc_scenario_config_id;
    uint8 config = anc_data->anc_config;

    stateProxy_UpdateAncScenarioConfig(state_proxy_data, config_id, config);
}

static void stateProxy_HandleLocalAncDemoStateUpdate(MessageId id)
{
    state_proxy_data_t* state_proxy_data = stateProxy_GetData(state_proxy_source_local);

    stateProxy_UpdateAncDemoState(state_proxy_data, (id == ANC_UPDATE_DEMO_MODE_ENABLE_IND));
}

static void stateProxy_HandleLocalAncAdaptivityStatusUpdate(MessageId id)
{
    state_proxy_data_t* state_proxy_data = stateProxy_GetData(state_proxy_source_local);

    stateProxy_UpdateAncAdaptivityStatus(state_proxy_data, (id == ANC_UPDATE_AANC_ADAPTIVITY_RESUMED_IND));
}

static void stateProxy_HandleLocalAncWorldVolumeGainUpdate(ANC_WORLD_VOLUME_GAIN_DB_UPDATE_IND_T* anc_data)
{
    state_proxy_data_t* state_proxy_data = stateProxy_GetData(state_proxy_source_local);
    int8 world_volume_gain_dB = anc_data->world_volume_gain_dB;

    stateProxy_UpdateAncWorldVolumeGain(state_proxy_data, world_volume_gain_dB);
}

static void stateProxy_HandleLocalAncWorldVolumeBalanceUpdate(ANC_WORLD_VOLUME_BALANCE_UPDATE_IND_T* anc_data)
{
    state_proxy_data_t* state_proxy_data = stateProxy_GetData(state_proxy_source_local);
    bool balance_device_side = anc_data->balance_device_side;
    uint8 balance_percentage = anc_data->balance_percentage;

    stateProxy_UpdateAncWorldVolumeBalance(state_proxy_data, balance_device_side, balance_percentage);
}

static void stateProxy_HandleRemoteAncStateUpdate(state_proxy_anc_msg_id_t id)
{
    state_proxy_data_t* state_proxy_data = stateProxy_GetData(state_proxy_source_remote);

    stateProxy_UpdateAncState(state_proxy_data, (id == state_proxy_anc_msg_id_enable));
}

static void stateProxy_HandleRemoteAncModeUpdate(ANC_UPDATE_MODE_CHANGED_IND_T* anc_data)
{
    state_proxy_data_t* state_proxy_data = stateProxy_GetData(state_proxy_source_remote);
    uint8 anc_mode = anc_data->mode;

    stateProxy_UpdateAncMode(state_proxy_data, anc_mode);
}

static void stateProxy_HandleRemoteAncGainUpdate(ANC_UPDATE_GAIN_IND_T* anc_data)
{
    state_proxy_data_t* state_proxy_data = stateProxy_GetData(state_proxy_source_remote);
    uint8 anc_gain = anc_data->anc_gain;

    stateProxy_UpdateAncLeakthroughGain(state_proxy_data, anc_gain);
}

static void stateProxy_HandleRemoteAncToggleConfigUpdate(STATE_PROXY_ANC_DATA_T* anc_data)
{
    state_proxy_data_t* state_proxy_data = stateProxy_GetData(state_proxy_source_remote);
    uint8 config_id = anc_data->msg.toggle_config.anc_toggle_config_id;
    uint8 config = anc_data->msg.toggle_config.anc_config;

    stateProxy_UpdateAncToggleConfig(state_proxy_data, config_id, config);
}

static void stateProxy_HandleRemoteScenarioConfigUpdate(STATE_PROXY_ANC_DATA_T* anc_data)
{
    state_proxy_data_t* state_proxy_data = stateProxy_GetData(state_proxy_source_remote);
    uint8 config_id = anc_data->msg.scenario_config.anc_scenario_config_id;
    uint8 config = anc_data->msg.scenario_config.anc_config;

    stateProxy_UpdateAncScenarioConfig(state_proxy_data, config_id, config);
}

static void stateProxy_HandleRemoteAncDemoStateUpdate(STATE_PROXY_ANC_DATA_T* anc_data)
{
    state_proxy_data_t* state_proxy_data = stateProxy_GetData(state_proxy_source_remote);
    state_proxy_anc_msg_id_t id = anc_data->msg_id;

    stateProxy_UpdateAncDemoState(state_proxy_data, (id == state_proxy_anc_msg_id_demo_state_enable));
}

static void stateProxy_HandleRemoteAncAdaptivityStatusUpdate(STATE_PROXY_ANC_DATA_T* anc_data)
{
    state_proxy_data_t* state_proxy_data = stateProxy_GetData(state_proxy_source_remote);
    state_proxy_anc_msg_id_t id = anc_data->msg_id;

    stateProxy_UpdateAncAdaptivityStatus(state_proxy_data, (id == state_proxy_anc_msg_id_adaptivity_enable));
}

static void stateProxy_HandleRemoteAncWorldVolumeGainUpdate(STATE_PROXY_ANC_DATA_T* anc_data)
{
    state_proxy_data_t* state_proxy_data = stateProxy_GetData(state_proxy_source_remote);
    int8 world_volume_gain_dB = anc_data->msg.world_volume_gain.world_volume_gain_dB;

    stateProxy_UpdateAncWorldVolumeGain(state_proxy_data, world_volume_gain_dB);
}

static void stateProxy_HandleRemoteAncWorldVolumeBalanceUpdate (STATE_PROXY_ANC_DATA_T* anc_data)
{
    state_proxy_data_t* state_proxy_data = stateProxy_GetData(state_proxy_source_remote);
    bool balance_device_side = anc_data->msg.balance_info.balance_device_side;
    uint8 balance_percentage = anc_data->msg.balance_info.balance_percentage;

    stateProxy_UpdateAncWorldVolumeBalance(state_proxy_data, balance_device_side, balance_percentage);
}

static void stateProxy_HandleRemoteAncPrevConfigUpdate (STATE_PROXY_ANC_DATA_T* anc_data)
{
    state_proxy_data_t* state_proxy_data = stateProxy_GetData(state_proxy_source_remote);
    state_proxy_data->previous_config = anc_data->msg.prev_config.previous_config;
}

static void stateProxy_HandleRemoteAncPrevModeUpdate (STATE_PROXY_ANC_DATA_T* anc_data)
{
    state_proxy_data_t* state_proxy_data = stateProxy_GetData(state_proxy_source_remote);
    state_proxy_data->previous_mode= anc_data->msg.prev_mode.previous_mode;
}

/*! \brief Get ANC data for initial state message. */
void stateProxy_GetInitialAncData(void)
{
    DEBUG_LOG_FN_ENTRY("stateProxy_GetInitialAncData");

#ifdef ENABLE_ANC
    if (!AncStateManager_IsInitialized())
    {
        DEBUG_LOG_WARN("ANC state manager needs to be initialized before getting initial data!");
        Panic();
    }
#endif

    state_proxy_task_data_t *proxy = stateProxy_GetTaskData();
    anc_sm_world_volume_gain_balance_info_t balance_info = {0,0};
    int8 cur_world_volume_gain_dB = 0;

    proxy->local_state->flags.anc_state = AncStateManager_IsEnabled();
    proxy->local_state->anc_mode = AncStateManager_GetMode();
    proxy->local_state->anc_leakthrough_gain = AncStateManager_GetAncGain();
    proxy->local_state->aanc_fb_gain = AncStateManager_GetAancFBGain();

    proxy->local_state->previous_config = AncStateManager_GetPreviousConfig();
    proxy->local_state->previous_mode =  AncStateManager_GetPreviousMode();

    proxy->local_state->toggle_configurations.anc_toggle_way_config[0] = AncStateManager_GetAncToggleConfiguration(anc_toggle_way_config_id_1);
    proxy->local_state->toggle_configurations.anc_toggle_way_config[1] = AncStateManager_GetAncToggleConfiguration(anc_toggle_way_config_id_2);
    proxy->local_state->toggle_configurations.anc_toggle_way_config[2] = AncStateManager_GetAncToggleConfiguration(anc_toggle_way_config_id_3);

    proxy->local_state->standalone_config = AncStateManager_GetAncScenarioConfiguration(anc_scenario_config_id_standalone);
    proxy->local_state->playback_config = AncStateManager_GetAncScenarioConfiguration(anc_scenario_config_id_playback);
    proxy->local_state->sco_config = AncStateManager_GetAncScenarioConfiguration(anc_scenario_config_id_sco);
    proxy->local_state->va_config = AncStateManager_GetAncScenarioConfiguration(anc_scenario_config_id_va);
#ifdef INCLUDE_LE_STEREO_RECORDING
    proxy->local_state->stereo_recording_le_config = AncStateManager_GetAncScenarioConfiguration(anc_scenario_config_id_stereo_recording_le);
#endif /* INCLUDE_LE_STEREO_RECORDING */

    proxy->local_state->flags.anc_demo_state = AncStateManager_IsDemoStateActive();

    proxy->local_state->flags.adaptivity_status = AncStateManager_GetAdaptiveAncAdaptivity();

    if(AncStateManager_GetCurrentWorldVolumeGain(&cur_world_volume_gain_dB))
    {
        proxy->local_state->world_volume_gain_dB = cur_world_volume_gain_dB;
    }

    if(AncStateManager_GetCurrentBalanceInfo(&balance_info))
    {
        proxy->local_state->balance_info.balance_device_side = balance_info.balance_device_side;
        proxy->local_state->balance_info.balance_percentage = balance_info.balance_percentage;
    }
}

/*! \brief Handle remote events for ANC data update during reconnect cases. */
void stateProxy_HandleInitialPeerAncData(state_proxy_data_t * new_state)
{
    DEBUG_LOG_FN_ENTRY("stateProxy_HandleInitialPeerAncData");
    state_proxy_task_data_t *proxy = stateProxy_GetTaskData();

    /* Update remote device data if local device is a slave; else ignored */
    if(!StateProxy_IsPrimary())
    {
       STATE_PROXY_ANC_DATA_T anc_msg_data;
       STATE_PROXY_RECONNECTION_ANC_DATA_T anc_data;

       proxy->remote_state->anc_mode = new_state->anc_mode;
       proxy->remote_state->flags.anc_state = new_state->flags.anc_state;
       
       proxy->remote_state->previous_config= new_state->previous_config;
       proxy->remote_state->previous_mode= new_state->previous_mode;
             
       proxy->remote_state->anc_leakthrough_gain= new_state->anc_leakthrough_gain;
       proxy->remote_state->toggle_configurations = new_state->toggle_configurations;
       proxy->remote_state->standalone_config = new_state->standalone_config;
       proxy->remote_state->playback_config = new_state->playback_config;
       proxy->remote_state->sco_config = new_state->sco_config;
       proxy->remote_state->va_config = new_state->va_config;

#ifdef INCLUDE_LE_STEREO_RECORDING
       proxy->remote_state->stereo_recording_le_config = new_state->stereo_recording_le_config;
#endif /* INCLUDE_LE_STEREO_RECORDING */

       proxy->remote_state->flags.anc_demo_state = new_state->flags.anc_demo_state;
       proxy->remote_state->flags.adaptivity_status = new_state->flags.adaptivity_status;
       proxy->remote_state->world_volume_gain_dB = new_state->world_volume_gain_dB;
       proxy->remote_state->balance_info = new_state->balance_info;

       anc_data.mode = new_state->anc_mode;
       anc_data.state = new_state->flags.anc_state;
       anc_data.previous_config = new_state->previous_config;
       anc_data.previous_mode = new_state->previous_mode;
       anc_data.gain = new_state->anc_leakthrough_gain;
       anc_data.toggle_configurations = new_state->toggle_configurations;
       anc_data.standalone_config = new_state->standalone_config;
       anc_data.playback_config = new_state->playback_config;
       anc_data.sco_config = new_state->sco_config;
       anc_data.va_config = new_state->va_config;

#ifdef INCLUDE_LE_STEREO_RECORDING
       anc_data.stereo_recording_le_config = new_state->stereo_recording_le_config;
#endif /* INCLUDE_LE_STEREO_RECORDING */

       anc_data.anc_demo_state = new_state->flags.anc_demo_state;
       anc_data.adaptivity = new_state->flags.adaptivity_status;
       anc_data.world_volume_gain_dB = new_state->world_volume_gain_dB;
       anc_data.balance_info = new_state->balance_info;

       anc_msg_data.msg_id = state_proxy_anc_msg_id_reconnection;
       anc_msg_data.msg.reconnection_data = anc_data;

       stateProxy_MsgStateProxyEventClients(state_proxy_source_remote,
                                         state_proxy_event_type_anc,
                                         &anc_msg_data);
    }
}

/*! \brief Handle local events for ANC data update. */
void stateProxy_HandleLocalAncUpdate(MessageId id, Message anc_data)
{
    DEBUG_LOG_FN_ENTRY("stateProxy_HandleLocalAncUpdate");

    switch(id)
    {
        case ANC_UPDATE_STATE_DISABLE_IND:
        case ANC_UPDATE_STATE_ENABLE_IND:
            stateProxy_HandleLocalAncStateUpdate(id);
            break;

        case ANC_UPDATE_MODE_CHANGED_IND:
            stateProxy_HandleLocalAncModeUpdate((ANC_UPDATE_MODE_CHANGED_IND_T*)anc_data);
            break;

        case ANC_UPDATE_GAIN_IND:
            stateProxy_HandleLocalAncGainUpdate((ANC_UPDATE_GAIN_IND_T*)anc_data);
            break;

        case ANC_TOGGLE_WAY_CONFIG_UPDATE_IND:
            stateProxy_HandleLocalAncToggleConfigUpdate((ANC_TOGGLE_WAY_CONFIG_UPDATE_IND_T*)anc_data);
            break;

        case ANC_SCENARIO_CONFIG_UPDATE_IND:
            stateProxy_HandleLocalScenarioConfigUpdate((ANC_SCENARIO_CONFIG_UPDATE_IND_T*)anc_data);
            break;

        case ANC_WORLD_VOLUME_GAIN_DB_UPDATE_IND:
            stateProxy_HandleLocalAncWorldVolumeGainUpdate((ANC_WORLD_VOLUME_GAIN_DB_UPDATE_IND_T*)anc_data);
            break;

        case ANC_WORLD_VOLUME_BALANCE_UPDATE_IND:
            stateProxy_HandleLocalAncWorldVolumeBalanceUpdate((ANC_WORLD_VOLUME_BALANCE_UPDATE_IND_T*)anc_data);
            break;

        case ANC_UPDATE_DEMO_MODE_DISABLE_IND:
        case ANC_UPDATE_DEMO_MODE_ENABLE_IND:
            stateProxy_HandleLocalAncDemoStateUpdate(id);
            break;

        /* Will be moved to state_proxy_aanc module */
        case ANC_UPDATE_AANC_ADAPTIVITY_PAUSED_IND:
        case ANC_UPDATE_AANC_ADAPTIVITY_RESUMED_IND:
            stateProxy_HandleLocalAncAdaptivityStatusUpdate(id);
            break;
            
        case ANC_UPDATE_PREV_CONFIG_IND:
            stateProxy_HandleLocalAncPrevConfigUpdate((ANC_UPDATE_PREV_CONFIG_IND_T*)anc_data);
            break;
            
        case ANC_UPDATE_PREV_MODE_IND:
            stateProxy_HandleLocalAncPrevModeUpdate((ANC_UPDATE_PREV_MODE_IND_T*)anc_data);
            break;
    }

    stateProxy_MarshalAncDataToPeer(stateProxy_GetMsgId(id), anc_data);
}

/*! \brief Handle remote events for ANC data update. */
void stateProxy_HandleRemoteAncUpdate(const STATE_PROXY_ANC_DATA_T* new_state)
{
    DEBUG_LOG_FN_ENTRY("stateProxy_HandleRemoteAncUpdate");

    switch(new_state->msg_id)
    {
        case state_proxy_anc_msg_id_disable:
        case state_proxy_anc_msg_id_enable:
            stateProxy_HandleRemoteAncStateUpdate(new_state->msg_id);
            break;

        case state_proxy_anc_msg_id_mode:
            stateProxy_HandleRemoteAncModeUpdate(&new_state->msg.mode);
            break;

        case state_proxy_anc_msg_id_gain:
            stateProxy_HandleRemoteAncGainUpdate(&new_state->msg.gain);
            break;

        case state_proxy_anc_msg_id_toggle_config:
            stateProxy_HandleRemoteAncToggleConfigUpdate((STATE_PROXY_ANC_DATA_T*)new_state);
            break;

        case state_proxy_anc_msg_id_scenario_config:
            stateProxy_HandleRemoteScenarioConfigUpdate((STATE_PROXY_ANC_DATA_T*)new_state);
            break;

        case state_proxy_anc_msg_id_demo_state_disable:
        case state_proxy_anc_msg_id_demo_state_enable:
            stateProxy_HandleRemoteAncDemoStateUpdate((STATE_PROXY_ANC_DATA_T*)new_state);
            break;

        /* Will be moved to state_proxy_aanc module */
        case state_proxy_anc_msg_id_adaptivity_disable:
        case state_proxy_anc_msg_id_adaptivity_enable:
            stateProxy_HandleRemoteAncAdaptivityStatusUpdate((STATE_PROXY_ANC_DATA_T*)new_state);
            break;

        case state_proxy_anc_msg_id_world_volume_gain:
            stateProxy_HandleRemoteAncWorldVolumeGainUpdate((STATE_PROXY_ANC_DATA_T*)new_state);
            break;

        case state_proxy_anc_msg_id_world_volume_balance:
            stateProxy_HandleRemoteAncWorldVolumeBalanceUpdate((STATE_PROXY_ANC_DATA_T*)new_state);
            break;
            
        case state_proxy_anc_msg_id_prev_config:
            stateProxy_HandleRemoteAncPrevConfigUpdate((STATE_PROXY_ANC_DATA_T*)new_state);
            break;
            
        case state_proxy_anc_msg_id_prev_mode:
            stateProxy_HandleRemoteAncPrevModeUpdate((STATE_PROXY_ANC_DATA_T*)new_state);
            break;
            
        default:
            break;

    }
    /* Update peer data to ANC module */
    stateProxy_MsgStateProxyEventClients(state_proxy_source_remote,
                                      state_proxy_event_type_anc,
                                      new_state);
}
