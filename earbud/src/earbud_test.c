/*!
\copyright  Copyright (c) 2017 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of specifc application testing functions
*/
#ifndef DISABLE_TEST_API

#include "earbud_test.h"

#include "system_state.h"
#include "adk_log.h"
#include "earbud_config.h"
#include "pairing.h"
#include "power_manager.h"
#include "earbud_sm.h"
#include "earbud_sm_private.h"
#include "battery_monitor_config.h"
#include "charger_monitor_config.h"
#include "kymera_config.h"
#include "temperature_config.h"
#include "thermistor.h"
#include "ui.h"
#include "ui_user_config.h"
#include "peer_link_keys.h"
#include "microphones.h"
#include "volume_messages.h"
#include "device_db_serialiser.h"
#include "audio_sources.h"
#include "voice_sources.h"
#include "gaia_framework_feature.h"
#include "gaia_features.h"
#include "anc_gaia_plugin.h"
#include "system_clock.h"
#include "earbud_tones.h"
#include "earbud_tones_config_table.h"
#include "temperature.h"
#include "state_of_charge.h"

#ifdef INCLUDE_LE_AUDIO_UNICAST
#include "le_unicast_manager.h"
#endif

#include "anc_state_manager.h"
#include "aanc_quiet_mode.h"
#include "kymera_anc_common.h"
#include "kymera_latency_manager.h"
#include "kymera_dynamic_latency.h"
#include "kymera_data.h"
#include "gaming_mode.h"

#ifdef ENABLE_ANC_NOISE_ID
#include "anc_noise_id.h"
#include "kymera_noise_id.h"
#endif

#ifdef ENABLE_EARBUD_FIT_TEST
#include "fit_test.h"
#endif

#ifdef ENABLE_WIND_DETECT
#include "wind_detect.h"
#endif

#include "audio_plugin_if.h"
#include "audio_clock.h"
#include "audio_power.h"
#include "volume_utils.h"
#ifdef INCLUDE_FAST_PAIR
#include "fast_pair_config.h"
#include <ps_key_map.h>
#endif

#include "app_buttons.h"
#include "upgrade.h"

#include <device_properties.h>
#include <device_list.h>
#include <device.h>
#include <dfu.h>
#include <hfp_profile.h>
#include <hfp_profile_instance.h>
#include <handover_profile.h>
#include <battery_monitor.h>
#include "battery_region.h"
#include <av.h>
#include <av_config.h>
#include <logical_input_switch.h>
#include <profile_manager.h>
#include <peer_find_role_private.h>
#include <peer_pair_le.h>
#include <peer_signalling.h>
#include <handset_service.h>
#include <connection_manager.h>
#include <connection.h>
#include <mirror_profile.h>
#include <volume_service.h>
#include <focus_audio_source.h>
#include <focus_voice_source.h>

#include <kymera.h>
#include <bredr_scan_manager.h>
#include <device_test_service_test.h>
#include <device_test_service.h>
#include <bredr_scan_manager.h>
#include <tws_topology_private.h>
#include <hdma_utils.h>

#include <charger_monitor.h>
#include <kymera.h>
#include <earbud_tones.h>
#include <system_reboot.h>

#include <cryptovm.h>
#include <ps.h>
#include <boot.h>
#include <feature.h>
#include <panic.h>
#include <stdio.h>
#include <bdaddr.h>
#include <util.h>
#ifdef USE_SYNERGY
#include <csr_bt_td_db_sc.h>
#ifdef INCLUDE_HIDD_PROFILE
#include "hidd_profile.h"
#endif
#endif

#ifdef INCLUDE_ACCESSORY
#include "adk_test_accessory.h"
#endif

#ifdef INCLUDE_GAA
#include "adk_test_gaa.h"
#endif

#ifdef INCLUDE_SPATIAL_AUDIO
#include "earbud_spatial_audio.h"
#endif

#ifdef INCLUDE_SPATIAL_DATA
SPATIAL_DATA_REPORT_DATA_IND_T *appTestSpatialAudioGetLatestReport(void)
{
    return EarbudSpatialAudio_GetLatestReport();
}
#endif /* INCLUDE_SPATIAL_DATA */

#include <cc_with_case.h>
#include "earbud_production_test.h"

#ifdef INCLUDE_EXTRA_TESTS
#include "common_test.h"
#endif

#ifdef GC_SECTIONS
/* Move all functions in KEEP_PM section to ensure they are not removed during
 * garbage collection */
#pragma unitcodesection KEEP_PM
#endif

typedef enum
{
    WAIT_FOR_IDLE_THEN_BRING_OUT_OF_CASE,
} delayed_test_event_t;

static void delayedInternalTestEventHandler(Task task, MessageId id, Message msg);
static const TaskData delayedInternalTestEventHandlerTask = { delayedInternalTestEventHandler };

static void testTaskHandler(Task task, MessageId id, Message message);
TaskData testTask = {testTaskHandler};

static struct
{
    uint16 testcase;
    uint16 iteration;
    uint16 last_marker;
} test_support = {0};

uint32 noise_id_stats[5];
static void delayedInternalTestEventHandler(Task task, MessageId id, Message msg)
{
    UNUSED(task);
    UNUSED(msg);
    switch(id)
    {
        case WAIT_FOR_IDLE_THEN_BRING_OUT_OF_CASE:
            if(!appTestIsTopologyIdle())
            {
                MessageSendLater((Task) &delayedInternalTestEventHandlerTask, WAIT_FOR_IDLE_THEN_BRING_OUT_OF_CASE,
                                                     NULL, 100);
            }
            else
            {
                appTestPhyStateOutOfCaseEvent();
            }
            break;
        default:
            break;
    }
}

/*! \brief Handle ACL Mode Changed Event

    This message is received when the link mode of an ACL has changed from sniff
    to active or vice-versa.  Currently the application ignores this message.
*/
static void appTestHandleClDmModeChangeEvent(CL_DM_MODE_CHANGE_EVENT_T *evt)
{
    DEBUG_LOG_VERBOSE("appHandleClDmModeChangeEvent, event %d, %x,%x,%lx", evt->mode, evt->bd_addr.nap, evt->bd_addr.uap, evt->bd_addr.lap);
}

bool appTestHandleConnectionLibraryMessages(MessageId id, Message message,
                                            bool already_handled)
{
    switch(id)
    {
        case CL_DM_MODE_CHANGE_EVENT:
            appTestHandleClDmModeChangeEvent((CL_DM_MODE_CHANGE_EVENT_T *)message);
            return TRUE;
    }

    return already_handled;
}

/*! \brief Returns the current battery voltage
 */
uint16 appTestGetBatteryVoltage(void)
{
    uint16 voltage = appBatteryGetVoltageInstantaneous();
    DEBUG_LOG_ALWAYS("appTestGetBatteryVoltage, %u", voltage);
    return voltage;
}

void appTestSetBatteryVoltage(uint16 new_level)
{
    DEBUG_LOG_ALWAYS("appTestSetBatteryVoltage, %u", new_level);
    appBatteryTestSetFakeVoltage(new_level);
}

void appTestUnsetBatteryVoltage(void)
{
    DEBUG_LOG_ALWAYS("appTestUnsetBatteryVoltage");
    appBatteryTestUnsetFakeVoltage();
}

void appTestInjectBatteryTestVoltage(uint16 new_level)
{
    DEBUG_LOG_ALWAYS("appTestInjectBatteryTestVoltage, %u", new_level);
    appBatteryTestInjectFakeLevel(new_level);
}

void appTestRestartBatteryVoltageMeasurements(void)
{
    DEBUG_LOG_ALWAYS("appTestRestartBatteryVoltageMeasurements");
    appBatteryTestResumeAdcMeasurements();
}

uint8 appTestConvertBatteryVoltageToPercentage(uint16 voltage_mv)
{
    uint8 percent = Soc_ConvertLevelToPercentage(voltage_mv);
    DEBUG_LOG_ALWAYS("appTestConvertBatteryVoltageToPercentage %d mV -> %d%%", voltage_mv, percent);

    return percent;
}

bool appTestBatteryStateIsOk(void)
{
    return (battery_region_ok == BatteryRegion_GetState());
}

bool appTestBatteryStateIsCritical(void)
{
    return (battery_region_critical == BatteryRegion_GetState());
}

bool appTestBatteryStateIsUnsafe(void)
{
    return (battery_region_unsafe == BatteryRegion_GetState());
}

uint32 appTestBatteryFilterResponseTimeMs(void)
{
    return appConfigBatteryReadingPeriodMs();
}

#ifdef HAVE_THERMISTOR
uint16 appTestThermistorDegreesCelsiusToMillivolts(int8 temperature)
{
    return appThermistorDegreesCelsiusToMillivolts(temperature);
}

void appTestSetBatteryTemperature(int8 new_temp)
{
    DEBUG_LOG_ALWAYS("appTestSetBatteryTemperature, %u", new_temp);
    appTemperatureSetFakeValue(new_temp);
}

void appTestUnsetBatteryTemperature(void)
{
    DEBUG_LOG_ALWAYS("appTestUnsetBatteryTemperature");
    appTemperatureUnsetFakeValue();
}

void appTestInjectBatteryTestTemperature(int8 new_temp)
{
    DEBUG_LOG_ALWAYS("appTestInjectBatteryTestTemperature, %d", new_temp);
    appTemperatureTestInjectFakeLevel(new_temp);
}

void appTestRestartTemperatureMeasurements(void)
{
    DEBUG_LOG_ALWAYS("appTestRestartTemperatureMeasurements");
    appTemperatureResumeAdcMeasurements();
}

int8 appTestGetBatteryTemperature(void)
{
    int8 temp = appTemperatureGetInstantaneous();
    DEBUG_LOG_ALWAYS("appTestGetBatteryTemperature, %d", temp);
    return temp;
}

#endif

/*! \brief Put Earbud into Handset Pairing mode
*/
void appTestPairHandset(void)
{
    DEBUG_LOG_ALWAYS("appTestPairHandset");
    Ui_InjectUiInput(ui_input_sm_pair_handset);
}

/*! \brief Delete all Handset pairing
*/
void appTestDeleteHandset(void)
{
    DEBUG_LOG_ALWAYS("appTestDeleteHandset");
    Ui_InjectUiInput(ui_input_sm_delete_handsets);
}

/*! \brief Delete Earbud peer pairing
*/
bool appTestDeletePeer(void)
{
    bdaddr bd_addr;

    DEBUG_LOG_ALWAYS("appTestDeletePeer");
    /* Check if we have previously paired with an earbud */
    if (appDeviceGetPeerBdAddr(&bd_addr))
    {
        return appDeviceDelete(&bd_addr);
    }
    else
    {
        DEBUG_LOG_WARN("appTestDeletePeer: NO PEER TO DELETE");
        return FALSE;
    }
}

bool appTestGetPeerAddr(bdaddr *peer_address)
{
    DEBUG_LOG_ALWAYS("appTestGetPeerAddr");
    if (appDeviceGetPeerBdAddr(peer_address))
        return TRUE;
    else
        return FALSE;
}


static bool appTestReadLocalAddr_completed = FALSE;
void appTestReadLocalAddr(void)
{
    DEBUG_LOG_ALWAYS("appTestReadLocalAddr started");

    appTestReadLocalAddr_completed = FALSE;
    ConnectionReadLocalAddr(&testTask);
}


static bdaddr appTestGetLocalAddr_address = {0};
bool appTestGetLocalAddr(bdaddr *addr)
{
    DEBUG_LOG_ALWAYS("appTestGetLocalAddr. Read:%d %04x:%02x:%06x",
                     appTestReadLocalAddr_completed,
                     appTestGetLocalAddr_address.nap, appTestGetLocalAddr_address.uap, appTestGetLocalAddr_address.lap);

    *addr = appTestGetLocalAddr_address;
    return appTestReadLocalAddr_completed;
}


/*! \brief Return if Earbud is in a Pairing mode
*/
bool appTestIsPairingInProgress(void)
{
    bool pip = !PairingIsIdle();

    DEBUG_LOG_ALWAYS("appTestIsPairingInProgress:%d", pip);

    return pip;
}

/*! \brief Initiate Earbud A2DP connection to the Handset
*/
bool appTestHandsetA2dpConnect(void)
{
    bdaddr bd_addr;

    DEBUG_LOG_ALWAYS("appTestHandsetA2dpConnect");

    if (appDeviceGetHandsetBdAddr(&bd_addr))
    {
        return appAvA2dpConnectRequest(&bd_addr, appTestIsPtsMode() ? A2DP_CONNECT_MEDIA : A2DP_CONNECT_NOFLAGS);
    }
    else
    {
        return FALSE;
    }
}

/*! Used to block the rules for handset pairing */
bool appTestHandsetPairingBlocked = FALSE;

/*! \brief Stop the earbud pairing with a handset
 */
void appTestBlockAutoHandsetPairing(bool block)
{
    bool was_blocked = appTestHandsetPairingBlocked;
    bool paired_already = appTestIsHandsetPaired();

    appTestHandsetPairingBlocked = block;

    DEBUG_LOG_ALWAYS("appTestBlockAutoHandsetPairing(%d) was %d (paired:%d)",
                    block, was_blocked, paired_already);
}

/*! \brief Return if Earbud has a handset pairing
 */
bool appTestIsHandsetPaired(void)
{
    bool paired = BtDevice_IsPairedWithHandset();

    DEBUG_LOG_ALWAYS("appTestIsHandsetPaired:%d",paired);

    return paired;
}

/*! \brief Return if Earbud has an Handset A2DP connection
*/
bool appTestIsHandsetA2dpConnected(void)
{
    bool connected = FALSE;
    bdaddr bd_addr;

    if (appDeviceGetHandsetBdAddr(&bd_addr))
    {
        /* Find handset AV instance */
        avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&bd_addr);
        connected = theInst && appA2dpIsConnected(theInst);
    }

    DEBUG_LOG_ALWAYS("appTestIsHandsetA2dpConnected:%d", connected);

    /* If we get here then there's no A2DP connected for handset */
    return connected;
}

bool appTestIsHandsetA2dpMediaConnected(void)
{
    bool connected = FALSE;
    bdaddr bd_addr;

    if (appDeviceGetHandsetBdAddr(&bd_addr))
    {
        /* Find handset AV instance */
        avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&bd_addr);
        connected = theInst && appA2dpIsConnectedMedia(theInst);
    }

    DEBUG_LOG_ALWAYS("appTestIsHandsetA2dpMediaConnected:%d", connected);

    /* If we get here then there's no A2DP connected for handset */
    return connected;
}

bool appTestIsA2dpPlaying(void)
{
    bool playing = Av_IsPlaying();

    DEBUG_LOG_ALWAYS("appTestIsA2dpPlaying=%d", playing);

    return playing;
}

/*! \brief Initiate Earbud AVRCP connection to the Handset
*/
bool appTestHandsetAvrcpConnect(void)
{
    bdaddr bd_addr;

    DEBUG_LOG_ALWAYS("appTestHandsetAvrcpConnect");

    if (appDeviceGetHandsetBdAddr(&bd_addr))
    {
        avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&bd_addr);

        /* appAvAvrcpConnectRequest() expects valid client task.
         * Create AV instance if not found and use AV task as client task.
         */
        if (theInst == NULL)
            /* No AV instance for this device, so create new instance */
            theInst = appAvInstanceCreate(&bd_addr, &av_plugin_interface);

        return (theInst ? appAvAvrcpConnectRequest(&theInst->av_task, &bd_addr) : FALSE);
    }

    return FALSE;
}

/*! \brief Return if Earbud has an Handset AVRCP connection
*/
bool appTestIsHandsetAvrcpConnected(void)
{
    bool connected = FALSE;
    bdaddr bd_addr;

    if (appDeviceGetHandsetBdAddr(&bd_addr))
    {
        /* Find handset AV instance */
        avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&bd_addr);
        connected = theInst && appAvrcpIsConnected(theInst);
    }

    DEBUG_LOG_ALWAYS("appTestIsHandsetAvrcpConnected = %d", connected);

    /* If we get here then there's no AVRCP connected for handset */
    return connected;
}

/*! \brief Return if Earbud has an Handset HFP connection
*/
bool appTestIsHandsetHfpConnected(void)
{
    bool connected = appHfpIsConnected();

    DEBUG_LOG_ALWAYS("appTestIsHandsetHfpConnected= %d", connected);

    return connected;
}

/*! \brief Return if Earbud has an Handset HFP SCO connection
*/
bool appTestIsHandsetHfpScoActive(void)
{
    bool active = HfpProfile_IsScoActive();

    DEBUG_LOG_ALWAYS("appTestIsHandsetHfpScoActive:%d", active);

    return active;
}

/*! \brief Initiate Earbud HFP Voice Dial request to the Handset
*/
void appTestHandsetHfpVoiceDial(void)
{
    DEBUG_LOG_ALWAYS("appTestHandsetHfpVoiceDial");
    Ui_InjectUiInput(ui_input_voice_dial);
}

void appTestHandsetHfpMuteToggle(void)
{
    DEBUG_LOG_ALWAYS("appTestHandsetHfpMute");
    Ui_InjectUiInput(ui_input_mic_mute_toggle);
}

void appTestHandsetHfpVoiceTransferToAg(void)
{
    DEBUG_LOG_ALWAYS("appTestHandsetHfpVoiceTransferToAg");
    Ui_InjectUiInput(ui_input_voice_transfer_to_ag);
}

void appTestHandsetHfpVoiceTransferToHeadset(void)
{
    DEBUG_LOG_ALWAYS("appTestHandsetHfpVoiceTransferToHeadset");
    Ui_InjectUiInput(ui_input_voice_transfer_to_headset);
}


bool appTestHandsetHfpActivateVoiceRecognition(bool activate)
{
    DEBUG_LOG_ALWAYS("appTestHandsetHfpActivateVoiceRecognition: activate(%d)", activate);

    hfpInstanceTaskData * instance = HfpProfile_GetInstanceForVoiceSourceWithUiFocus();

    if (instance != NULL && appHfpIsConnectedForInstance(instance))
    {
#ifdef USE_SYNERGY
        HfSetVoiceRecognitionReqSend(instance->connection_id , (activate == TRUE) ? CSR_BT_HFP_VR_ENABLE : CSR_BT_HFP_VR_DISABLE);        
#else
        hfp_link_priority link_priority = HfpLinkPriorityFromBdaddr(&instance->ag_bd_addr);
        HfpVoiceRecognitionEnableRequestEx(link_priority, (activate == TRUE) ? hfp_evra_action_enable : hfp_evra_action_disable);
#endif
        return TRUE;
    }
    return FALSE;
}

bool appTestHandsetHfpSetIndicatorValue(uint8 idx, uint8 value)
{
    DEBUG_LOG_ALWAYS("appTestHandsetHfpSetIndicatorValue: idx(%d) value(%d)", idx, value);

    hfpInstanceTaskData * instance = HfpProfile_GetInstanceForVoiceSourceWithUiFocus();

    if (instance != NULL && appHfpIsConnectedForInstance(instance))
    {
#ifdef USE_SYNERGY
        HfSetHfIndicatorValueReqSend(instance->connection_id, idx, value);        
#else
        hfp_link_priority link_priority = HfpLinkPriorityFromBdaddr(&instance->ag_bd_addr);
        HfpBievIndStatusRequest(link_priority, idx, value);
#endif
        return TRUE;
    }
    return FALSE;
}

bool appTestHandsetHfpGetCurrentCallList(void)
{
    DEBUG_LOG_ALWAYS("appTestHandsetHfpGetCurrentCallList");

    hfpInstanceTaskData * instance = HfpProfile_GetInstanceForVoiceSourceWithUiFocus();

    if (instance != NULL && appHfpIsConnectedForInstance(instance))
    {
#ifdef USE_SYNERGY
        HfGetCurrentCallListReqSend(instance->connection_id);
#else
        hfp_link_priority link_priority = HfpLinkPriorityFromBdaddr(&instance->ag_bd_addr);
        HfpCurrentCallsRequest(link_priority);
#endif
        return TRUE;
    }

    return FALSE;
}

void appTestHandsetHfpCallAccept(void)
{
    DEBUG_LOG_ALWAYS("appTestHandsetHfpCallAccept");
    Ui_InjectUiInput(ui_input_voice_call_accept);
}

void appTestHandsetHfpCallReject(void)
{
    DEBUG_LOG_ALWAYS("appTestHandsetHfpCallReject");
    Ui_InjectUiInput(ui_input_voice_call_reject);
}

void appTestHandsetHfpCallHangup(void)
{
    DEBUG_LOG_ALWAYS("appTestHandsetHfpCallHangup");
    Ui_InjectUiInput(ui_input_voice_call_hang_up);
}

void appTestHandsetVoiceCallCycle(void)
{
    DEBUG_LOG_ALWAYS("appTestHandsetVoiceCallCycle");
    Ui_InjectUiInput(ui_input_voice_call_cycle);
}

void appTestHandsetVoiceCallJoinCalls(void)
{
    DEBUG_LOG_ALWAYS("appTestHandsetVoiceCallJoinCalls");
    Ui_InjectUiInput(ui_input_voice_call_join_calls);
}

void appTestHandsetVoiceCallJoinCallsAndHangUp(void)
{
    DEBUG_LOG_ALWAYS("appTestHandsetVoiceCallJoinCallsAndHangUp");
    Ui_InjectUiInput(ui_input_voice_call_join_calls_and_hang_up);
}

bool appTestHandsetHfpCallLastDialed(void)
{
    DEBUG_LOG_ALWAYS("appTestHandsetHfpCallLastDialed");
    hfpInstanceTaskData * instance = HfpProfile_GetInstanceForVoiceSourceWithUiFocus();
    if (instance != NULL && appHfpIsConnectedForInstance(instance))
    {
        Ui_InjectUiInput(ui_input_voice_call_last_dialed);
        return TRUE;
    }
    else
        return FALSE;
}

void appTestHandsetHfpVolumeDownStart(void)
{
    DEBUG_LOG_ALWAYS("appTestHandsetHfpVolumeDownStart");
    Ui_InjectUiInput(ui_input_volume_down_start);
}

void appTestHandsetHfpVolumeUpStart(void)
{
    DEBUG_LOG_ALWAYS("appTestHandsetHfpVolumeUpStart");
    Ui_InjectUiInput(ui_input_volume_up_start);
}

void appTestHandsetHfpVolumeStop(void)
{
    DEBUG_LOG_ALWAYS("appTestHandsetHfpVolumeStop");
    Ui_InjectUiInput(ui_input_volume_stop);
}

bool appTestHandsetHfpSetScoVolume(uint8 volume)
{
    voice_source_t focused_source = voice_source_none;
    bool volume_set = FALSE;

    if (TwsTopology_IsRoleSecondary())
    {
        // This Test API should never be called on the secondary Earbud
        DEBUG_LOG_ALWAYS("appTestHandsetHfpScoVolume call on Secondary EB. Volume change won't be applied");
        return volume_set;
    }

    hfpInstanceTaskData * instance = HfpProfile_GetInstanceForVoiceSourceWithUiFocus();
    if (instance != NULL && appHfpIsCallForInstance(instance))
    {
        focused_source = HfpProfileInstance_GetVoiceSourceForInstance(instance);
        if (VoiceSource_IsHfp(focused_source))
        {
            Volume_SendVoiceSourceVolumeUpdateRequest(focused_source, event_origin_local, volume);
            volume_set = TRUE;
        }
    }

    DEBUG_LOG_ALWAYS("appTestHandsetHfpScoVolume enum:voice_source_t:%d", focused_source);

    return volume_set;
}

bool appTestIsHandsetHfpMuted(void)
{
    bool muted = FALSE;
    hfpInstanceTaskData * instance = HfpProfile_GetInstanceForVoiceSourceWithUiFocus();
    if (instance != NULL)
    {
        muted = HfpProfile_IsMicrophoneMuted(instance);
    }

    DEBUG_LOG_ALWAYS("appTestIsHandsetHfpMuted;%d", muted);

    return muted;
}

bool appTestIsHandsetHfpCall(void)
{
    bool is_call = appHfpIsCall();

    DEBUG_LOG_ALWAYS("appTestIsHandsetHfpCall:%d", is_call);

    return is_call;
}

bool appTestIsHandsetHfpCallIncoming(void)
{
    bool incoming = appHfpIsCallIncoming();

    DEBUG_LOG_ALWAYS("appTestIsHandsetHfpCallIncoming:%d", incoming);

    return incoming;
}

bool appTestIsHandsetHfpCallOutgoing(void)
{
    bool outgoing = appHfpIsCallOutgoing();

    DEBUG_LOG_ALWAYS("appTestIsHandsetHfpCallOutgoing:%d", outgoing);

    return outgoing;
}

/*! \brief Return if Earbud has a connection to the Handset
*/
bool appTestIsHandsetConnected(void)
{
    bool connected = appTestIsHandsetA2dpConnected() ||
                     appTestIsHandsetAvrcpConnected() ||
                     appTestIsHandsetHfpConnected();

    DEBUG_LOG_ALWAYS("appTestIsHandsetConnected = %d", connected);

    return connected;
}

bool appTestIsHandsetConnectedOverLe(void)
{
    bool connected = appDeviceIsLeHandsetConnected();
    DEBUG_LOG_ALWAYS("appTestIsHandsetConnectedOverLe = %d", connected);
    return connected;
}

/*! \brief Initiate Earbud A2DP connection to the the Peer
*/
bool appTestPeerA2dpConnect(void)
{
    DEBUG_LOG_ALWAYS("appTestPeerA2dpConnect ***DEPRECATED***");

    return FALSE;
}

/*! \brief Return if Earbud has a Peer A2DP connection
*/
bool appTestIsPeerA2dpConnected(void)
{
    bool connected = FALSE;

    /* Since TWM implemented, test if A2DP mirroring is connected instead. */
    connected = MirrorProfile_IsBredrMirroringConnected();

    DEBUG_LOG_ALWAYS("appTestIsPeerA2dpConnected:%d", connected);

    return connected;
}

/*! \brief Check if Earbud is in A2DP streaming mode with peer Earbud
 */
bool appTestIsPeerA2dpStreaming(void)
{
    bool streaming = FALSE;

    /* Since TWM implemented, test if A2DP mirroring is streaming. */
    streaming = MirrorProfile_IsA2dpActive();

    DEBUG_LOG_ALWAYS("appTestIsPeerA2dpStreaming:%d", streaming);

    return streaming;
}


/*! \brief Initiate Earbud AVRCP connection to the the Peer
*/
bool appTestPeerAvrcpConnect(void)
{
    DEBUG_LOG_ALWAYS("appTestPeerAvrcpConnect ***DEPRECATED***");

    return FALSE;
}

/*! \brief Return if Earbud has a Peer AVRCP connection
*/
bool appTestIsPeerAvrcpConnected(void)
{
    bool connected = FALSE;

    /* Since TWM implemented, consider a peer signalling connection as the equivalent connection. */
    connected = appPeerSigIsConnected();

    DEBUG_LOG_ALWAYS("appTestIsPeerAvrcpConnected:%d", connected);

    return connected;
}

#ifdef INCLUDE_EXTRA_TESTS
void appTestPeerDisconnect(void)
{
    bdaddr bd_addr;

    if(appDeviceGetPeerBdAddr(&bd_addr))
    {
        DEBUG_LOG_ALWAYS("appTestPeerDisconnect peer lap 0x%x", bd_addr.lap);

        ConManagerSendCloseAclRequest(&bd_addr, TRUE);
        appTestTransmitEnable(0);
    }
    else
    {
        DEBUG_LOG_ALWAYS("appTestPeerDisconnect NO PEER");
    }
}
#endif

/*! \brief Send the AV play/pause toggle command
*/
void appTestAvTogglePlayPause(void)
{
    DEBUG_LOG_ALWAYS("appTestAvTogglePlayPause");
    Ui_InjectUiInput(ui_input_toggle_play_pause);
}

#ifdef INCLUDE_HDMA
#include "hdma.h"
/*! \brief Start dynamic handover procedure */
bool earbudTest_DynamicHandover(void)
{
    DEBUG_LOG_ALWAYS("earbudTest_DynamicHandover");
    return Hdma_ExternalHandoverRequest();
}

void earbudTest_TwsTopology_SwapRole(void)
{
    DEBUG_LOG_ALWAYS("earbudTest_TwsTopology_SwapRole");
    TwsTopology_SwapRole();
}

void earbudTest_TwsTopology_SwapRoleAndLeave(void)
{
    DEBUG_LOG_ALWAYS("earbudTest_TwsTopology_SwapRoleAndLeave");
    TwsTopology_SwapRoleAndLeave();
}

#endif

/*! \brief Send the Avrcp pause command to the Handset
*/
void appTestAvPause(void)
{
    DEBUG_LOG_ALWAYS("appTestAvPause");
    Ui_InjectUiInput(ui_input_pause_all);
}

/*! \brief Send the Avrcp play command to the Handset
*/
void appTestAvPlay(void)
{
    DEBUG_LOG_ALWAYS("appTestAvPlay");
    Ui_InjectUiInput(ui_input_play);
}

/*! \brief Send the Avrcp stop command to the Handset
*/
void appTestAvStop(void)
{
    DEBUG_LOG_ALWAYS("appTestAvStop");
    Ui_InjectUiInput(ui_input_stop_av_connection);
}

/*! \brief Send the Avrcp forward command to the Handset
*/
void appTestAvForward(void)
{
    DEBUG_LOG_ALWAYS("appTestAvForward");
    Ui_InjectUiInput(ui_input_av_forward);
}

/*! \brief Send the Avrcp backward command to the Handset
*/
void appTestAvBackward(void)
{
    DEBUG_LOG_ALWAYS("appTestAvBackward");
    Ui_InjectUiInput(ui_input_av_backward);
}

/*! \brief Send the Avrcp fast forward state command to the Handset
*/
void appTestAvFastForwardStart(void)
{
    DEBUG_LOG_ALWAYS("appTestAvFastForwardStart");
    Ui_InjectUiInput(ui_input_av_fast_forward_start);
}

/*! \brief Send the Avrcp fast forward stop command to the Handset
*/
void appTestAvFastForwardStop(void)
{
    DEBUG_LOG_ALWAYS("appTestAvFastForwardStop");
    Ui_InjectUiInput(ui_input_fast_forward_stop);
}

/*! \brief Send the Avrcp rewind start command to the Handset
*/
void appTestAvRewindStart(void)
{
    DEBUG_LOG_ALWAYS("appTestAvRewindStart");
    Ui_InjectUiInput(ui_input_av_rewind_start);
}

/*! \brief Send the Avrcp rewind stop command to the Handset
*/
void appTestAvRewindStop(void)
{
    DEBUG_LOG_ALWAYS("appTestAvRewindStop");
    Ui_InjectUiInput(ui_input_rewind_stop);
}

/*! \brief Send the Avrcp volume change command to the Handset
*/
bool appTestAvVolumeChange(int8 step)
{
    bool volume_change_applied = FALSE;

    DEBUG_LOG_ALWAYS("appTestAvVolumeChange %d", step);

    if (step != 0)
    {
        audio_source_t focused_source = audio_source_none;
        bool is_focused_source_found = Focus_GetAudioSourceForContext(&focused_source);

        if (!is_focused_source_found && MirrorProfile_IsA2dpActive())
        {
            // If there is no focused source, but A2DP mirroring is active, then
            // use a2dp1 audio source (which is hard-coded for mirroring scenario).
            focused_source = audio_source_a2dp_1;
        }

        if (focused_source == audio_source_a2dp_1 || focused_source == audio_source_a2dp_2)
        {
            int step_size = VolumeService_GetUiStepSize(AudioSources_GetVolume(focused_source).config);
            if ((step % step_size) == 0)
            {
                int ui_inputs_to_raise = ABS(step) / step_size;
                ui_input_t ui_input = (step > 0) ? ui_input_volume_up : ui_input_volume_down;
                for (int ui_input_counter = 0; ui_input_counter < ui_inputs_to_raise; ui_input_counter++)
                {
                    Ui_InjectUiInput(ui_input);
                }
                DEBUG_LOG_VERBOSE("appTestAvVolumeChange applied %d volume steps", ui_inputs_to_raise);

                volume_change_applied = TRUE;
            }
            else
            {
                DEBUG_LOG_ALWAYS("step should be in multiple of step size %d", step_size);
            }
        }
        else
        {
            DEBUG_LOG_ALWAYS("focused audio source is not A2dp enum:audio_source_t:%d", focused_source);
        }
    }

    return volume_change_applied;
}

void appTestAvVolumeSet(uint8 volume)
{
    DEBUG_LOG_ALWAYS("appTestAvVolumeSet %d", volume);

    if (TwsTopology_IsRoleSecondary())
    {
        // This Test API should never be called on the secondary Earbud
        Panic();
    }

    audio_source_t focused_source = audio_source_none;
    if (!Focus_GetAudioSourceForContext(&focused_source))
    {
        DEBUG_LOG_ALWAYS("no focused audio source");
        return;
    }

    if (focused_source != audio_source_a2dp_1 && focused_source != audio_source_a2dp_2)
    {
        DEBUG_LOG_ALWAYS("focused audio source is not A2dp");
        return;
    }

    Volume_SendAudioSourceVolumeUpdateRequest(focused_source, event_origin_local, volume);
}

void appTestPowerAllowDormant(bool enable)
{
    powerTaskData *thePower = PowerGetTaskData();

    DEBUG_LOG_ALWAYS("appTestPowerAllowDormant %d", enable);
    thePower->allow_dormant = enable;
}

/*! \brief Returns the DSP clock speed in Mhz during the active mode.
*/
uint32 EarbudTest_DspClockSpeedInActiveMode(void)
{
    audio_dsp_clock kclocks;
    audio_power_save_mode mode = AUDIO_POWER_SAVE_MODE_3;

    PanicFalse(AudioDspGetClock(&kclocks));
    mode = AudioPowerSaveModeGet();
    DEBUG_LOG("EarbudTest_DspClockSpeedInActiveMode, current mode of operation of kymera dsp is %d, Active mode: %d Mhz, Low Power mode: %d Mhz, Trigger mode: %d Mhz", mode, kclocks.active_mode, kclocks.low_power_mode, kclocks.trigger_mode);
    return kclocks.active_mode;
}

#ifdef INCLUDE_EXTRA_TESTS

/*! \brief Test the generation of link kets */
extern void TestLinkkeyGen(void)
{
    bdaddr bd_addr;
    uint16 lk[8];
    uint16 lk_out[8];

    bd_addr.nap = 0x0002;
    bd_addr.uap = 0x5B;
    bd_addr.lap = 0x00FF02;

    lk[0] = 0x9541;
    lk[1] = 0xe6b4;
    lk[2] = 0x6859;
    lk[3] = 0x0791;
    lk[4] = 0x9df9;
    lk[5] = 0x95cd;
    lk[6] = 0x9570;
    lk[7] = 0x814b;

    DEBUG_LOG_ALWAYS("appTestPowerAllowDormant");
    PeerLinkKeys_GenerateKey(&bd_addr, lk, 0x74777332UL, lk_out);

#if 0
    bd_addr.nap = 0x0000;
    bd_addr.uap = 0x74;
    bd_addr.lap = 0x6D7031;

    lk[0] = 0xec02;
    lk[1] = 0x34a3;
    lk[2] = 0x57c8;
    lk[3] = 0xad05;
    lk[4] = 0x3410;
    lk[5] = 0x10a6;
    lk[6] = 0x0a39;
    lk[7] = 0x7d9b;
#endif

    PeerLinkKeys_GenerateKey(&bd_addr, lk, 0x6c656272UL, lk_out);

}

/*! \brief Test the cryptographic key conversion function, producing an H6 key */
extern void TestH6(void)
{
    uint8 key_h7[16] = {0xec,0x02,0x34,0xa3,0x57,0xc8,0xad,0x05,0x34,0x10,0x10,0xa6,0x0a,0x39,0x7d,0x9b};
    //uint32 key_id = 0x6c656272;
    uint32 key_id = 0x7262656c;
    uint8 key_h6[16];

    DEBUG_LOG_ALWAYS("appTestPowerAllowDormant");
    CryptoVmH6(key_h7, key_id, key_h6);
    printf("H6: ");
    for (int h6_i = 0; h6_i < 16; h6_i++)
        printf("%02x ", key_h6[h6_i]);
    printf("\n");
}

#endif /* INCLUDE_EXTRA_TESTS */

/*! \brief report the saved handset information */
extern void appTestHandsetInfo(void)
{
    bdaddr bd_addr;
    DEBUG_LOG_ALWAYS("appTestHandsetInfo");
    if (appDeviceGetHandsetBdAddr(&bd_addr))
    {
        DEBUG_LOG_VERBOSE("appTestHandsetInfo, bdaddr %04x,%02x,%06lx",
                   bd_addr.nap, bd_addr.uap, bd_addr.lap);
    }
}

#include "adk_log.h"

/*! \brief Simple test function to make sure that the DEBUG_LOG macros
        work */
extern void TestDebug(void)
{
    DEBUG_LOG_ALWAYS("TestDebug (Always): test %d %d", 1, 2);
    DEBUG_LOG_ERROR("TestDebug (Error): test");
}

/*! \brief Generate event that Earbud is now in the case. */
void appTestPhyStateInCaseEvent(void)
{
    DEBUG_LOG_ALWAYS("appTestPhyStateInCaseEvent");
    appPhyStateInCaseEvent();
}

/*! \brief Generate event that Earbud is now out of the case. */
void appTestPhyStateOutOfCaseEvent(void)
{
    DEBUG_LOG_ALWAYS("appTestPhyStateOutOfCaseEvent");
    appPhyStateOutOfCaseEvent();
}

/*! \brief Generate event that Earbud is now in ear. */
void appTestPhyStateInEarEvent(void)
{
    DEBUG_LOG_ALWAYS("appTestPhyStateInEarEvent");
    appPhyStateInEarEvent();
}

/*! \brief Generate event that Earbud is now out of the ear. */
void appTestPhyStateOutOfEarEvent(void)
{
    DEBUG_LOG_ALWAYS("appTestPhyStateOutOfEarEvent");
    appPhyStateOutOfEarEvent();
}

/*! \brief Generate event that Earbud is now moving */
void appTestPhyStateMotionEvent(void)
{
    DEBUG_LOG_ALWAYS("appTestPhyStateMotionEvent");
    appPhyStateMotionEvent();
}

/*! \brief Generate event that Earbud is now not moving. */
void appTestPhyStateNotInMotionEvent(void)
{
    DEBUG_LOG_ALWAYS("appTestPhyStateNotInMotionEvent");
    appPhyStateNotInMotionEvent();
}

bool appTestPhyStateIsInEar(void)
{
    return (PHY_STATE_IN_EAR == appPhyStateGetState());
}


bool appTestPhyStateOutOfEar(void)
{
    return     PHY_STATE_OUT_OF_EAR == appPhyStateGetState()
            || PHY_STATE_OUT_OF_EAR_AT_REST == appPhyStateGetState();
}


bool appTestPhyStateIsInCase(void)
{
    return (PHY_STATE_IN_CASE == appPhyStateGetState());
}

static bool reset_happened = TRUE;

/*! \brief Reset an Earbud to factory defaults.
    Will drop any connections, delete all pairing and reboot.
*/
void appTestFactoryReset(void)
{
    DEBUG_LOG_ALWAYS("appTestFactoryReset");
    reset_happened = FALSE;
    LogicalInputSwitch_SendPassthroughDeviceSpecificLogicalInput(ui_input_factory_reset_request);
}

/*! \brief Determine if a reset has happened
    Will return TRUE only once after a reset.
    All subsequent calls will return FALSE
*/
bool appTestResetHappened(void)
{
    bool result = reset_happened;

    DEBUG_LOG_ALWAYS("appTestResetHappened: %d", result);

    if(reset_happened) {
        reset_happened = FALSE;
    }

    return result;
}

/*! \brief Determine if the earbud has a paired peer earbud.
*/
bool appTestIsPeerPaired(void)
{
    bool paired = BtDevice_IsPairedWithPeer();

    DEBUG_LOG_ALWAYS("appTestIsPeerPaired:%d",paired);

    return paired;
}

void appTestConnectHandset(void)
{
    if (appSmIsPrimary())
    {
        DEBUG_LOG_ALWAYS("appTestConnectHandset");
        PrimaryRules_SetEvent(RULE_EVENT_CONNECT_HANDSET_USER);
    }
    else
    {
        DEBUG_LOG_ERROR("appTestConnectHandset called in wrong role");
    }
}

bool appTestConnectHandsetA2dpMedia(void)
{
    DEBUG_LOG_ALWAYS("appTestConnectHandsetA2dpMedia ** DEPRECATED **");
    return FALSE;
}

/*! \brief Send A2dp Media Start request to remote
     Added to support PTS qualification TCs.
*/
bool appTestHandsetA2dpMediaStart(void)
{
    bdaddr bd_addr;

    DEBUG_LOG_ALWAYS("appTestHandsetA2dpMediaStart");

    if (appDeviceGetHandsetBdAddr(&bd_addr))
    {
        /* Find handset AV instance */
        avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&bd_addr);

        if (theInst)
        {
#ifdef USE_SYNERGY
            uint8 *list = PanicUnlessMalloc(1 * sizeof(uint8));

            *list = theInst->a2dp.stream_id;
            AvStartReqSend(A2DP_ASSIGN_TLABEL(theInst), list, 1);
            return TRUE;
#else
            return A2dpMediaStartRequest(theInst->a2dp.device_id, theInst->a2dp.stream_id);
#endif
        }
    }

    return FALSE;
}

#ifndef USE_SYNERGY
/*! \brief Reset AV Media Suspend state explicitly
*/
bool appTestResetAvSuspendState(void)
{
    DEBUG_LOG_ALWAYS("appTestResetAvSuspendState");

    avTaskData *theAv = AvGetTaskData();
    unsigned reason = AV_SUSPEND_REASON_LOCAL;

    theAv->suspend_state &= ~reason;
    return TRUE;
}
#endif /* !USE_SYNERGY */


/*! \brief Send Media Close to remote
    Added to support PTS qualification TCs.
*/
bool appTestHandsetA2dpMediaClose(void)
{
    bdaddr bd_addr;

    DEBUG_LOG_ALWAYS("appTestHandsetA2dpMediaClose");

    if (appDeviceGetHandsetBdAddr(&bd_addr))
    {
        /* Find handset AV instance */
        avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&bd_addr);

        if (theInst)
        {
#ifdef USE_SYNERGY
            AvCloseReqSend(theInst->a2dp.stream_id, theInst->a2dp.tLabel);
            return TRUE;
#else
            return A2dpMediaCloseRequest(theInst->a2dp.device_id, theInst->a2dp.stream_id);
#endif
        }
    }

    return FALSE;
}

/*! \brief Send A2dp Abort stream request to remote
    Added to support PTS qualification TCs.
*/
bool appTestHandsetA2dpAbort(void)
{
    bdaddr bd_addr;

    DEBUG_LOG_ALWAYS("appTestHandsetA2dpAbort");

    if (appDeviceGetHandsetBdAddr(&bd_addr))
    {
#ifdef USE_SYNERGY
        /* Find handset AV instance */
        avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&bd_addr);

        if (theInst)
        {
            AvAbortReqSend(theInst->a2dp.stream_id, A2DP_ASSIGN_TLABEL(theInst));
            return TRUE;
        }
#else
        return FALSE;
#endif
    }
    return FALSE;
}

/*! \brief Send A2dp Reconfigure stream request to remote
    Added to support PTS qualification TCs.
*/
bool appTestHandsetA2dpMediaReconfigure(void)
{
    bdaddr bd_addr;

    DEBUG_LOG_ALWAYS("appTestHandsetA2dpMediaReconfigure");

    if (appDeviceGetHandsetBdAddr(&bd_addr))
    {
        avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&bd_addr);

        if (theInst)
        {
            uint8 *caps = (uint8 *)PanicUnlessMalloc(sizeof(uint8) * 8);

            /* Just hardcode the values */
            caps[0] = 7;
            caps[1] = 6;
            caps[2] = 0;
            caps[3] = 0;
            caps[4] = 17;
            caps[5] = 21;
            caps[6] = 30;
            caps[7] = 50;
    
#ifdef USE_SYNERGY
            /* Find handset AV instance */
            AvReconfigReqSend(theInst->a2dp.stream_id,
                              A2DP_ASSIGN_TLABEL(theInst),
                              8,
                              caps);
            return TRUE;
#else
            return A2dpMediaReconfigureRequest(theInst->a2dp.device_id,
                                               theInst->a2dp.stream_id,
                                               8,
                                               caps);
#endif
        }
    }
    return FALSE;
}

bool appTestIsPeerSyncComplete(void)
{
    DEBUG_LOG_ALWAYS("appTestIsPeerSyncComplete:1 ** DEPRECATED **");
    /* test scripts may rely on this, for now just return TRUE so
     * they keep running. */
    return TRUE;
}

#ifdef USE_SYNERGY
static void handleSynergyCmPrim(Task task, Message message)
{
    UNUSED(task);
    CsrBtCmPrim *prim = (CsrBtCmPrim*) message;

    switch (*prim)
    {
        case CSR_BT_CM_READ_LOCAL_BD_ADDR_CFM:
            {
                CsrBtCmReadLocalBdAddrCfm *cfm = (CsrBtCmReadLocalBdAddrCfm*)message;
                bdaddr addr = {0};
                BdaddrConvertBluestackToVm(&addr, &cfm->deviceAddr);
                appTestReadLocalAddr_completed = TRUE;
                appTestGetLocalAddr_address = addr;

                DEBUG_LOG_ALWAYS("appTestReadLocalAddr completed. %04x:%02x:%06x",
                                 appTestGetLocalAddr_address.nap,
                                 appTestGetLocalAddr_address.uap,
                                 appTestGetLocalAddr_address.lap);
            }
            break;

        default:
            DEBUG_LOG("handleSynergyCmPrim Unexpected CM primitive = 0x%04x\n",
                      *prim);
            break;
    }

    CmFreeUpstreamMessageContents((void *) message);
}
#endif

bool power_off_tone_have_been_played = FALSE;
bool power_off_prompt_have_been_played = FALSE;

#ifdef INCLUDE_SPATIAL_AUDIO
bool appTestSpatialAudioEnable(uint16 sample_interval_hz, spatial_data_report_id_t report_id)
{
    return EarbudSpatialAudio_Enable(sample_interval_hz, report_id);
}

bool appTestSpatialAudioDisable(void)
{
    return EarbudSpatialAudio_Disable();
}

bool appTestSpatialAudioSetSyncDataExchange(uint16 data_len, uint8 command)
{
    uint8 *data;
    uint8 counter;
    bool ret = FALSE;

    if (!data_len)
    {
        return FALSE;
    }
    data = (uint8 *)(PanicUnlessMalloc(data_len));

    /* Create dummy data for testing sync data exchange between earbuds. */
    for (counter = 0; counter < data_len; counter++)
    {
        if (counter % 2)
        {
            data[counter] = command*3;
        }
        else
        {
            data[counter] = command*2;
        }
    }

    ret = EarbudSpatialAudio_SendSynchronisedData((earbud_spatial_audio_sync_data_command_t)command, data_len, data);
    free(data);

    return ret;
}
#endif /* INCLUDE_SPATIAL_AUDIO */

#ifdef USE_SYNERGY
#ifdef INCLUDE_HIDD_PROFILE
/*Once toggled TRUE, continuous reports will be sent to Host.*/
static const uint8 hid_keyboard_service_record[] =
{
    0x09, 0x00, 0x01,       /* ServiceClassIDList(0x0001) */
    0x35, 0x03,             /* DataElSeq 3 bytes */
    0x19, 0x11, 0x24,       /* uuid16_t HID(0x1124) */

    0x09, 0x00, 0x04,       /* ProtocolDescriptorList(0x0004) */
    0x35, 0x0d,             /* DataElSeq 13 bytes */
    0x35, 0x06,             /* DataElSeq 6 bytes */
    0x19, 0x01, 0x00,       /* uuid16_t L2CAP(0x0100) */
    0x09, 0x00, 0x11,       /* CsrUint16 0x0011 */
    0x35, 0x03,             /* DataElSeq 3 bytes */
    0x19, 0x00, 0x11,       /* uuid16_t HIDP(0x0011) */

    /* BrowseGroupList    */
    0x09, 0x00, 0x05,      /* AttrId = BrowseGroupList */
    0x35, 0x03,            /* Data element seq. 3 bytes */
    0x19, 0x10, 0x02,      /* 2 byte UUID, PublicBrowseGroup = 0x1002 */

    0x09, 0x00, 0x06,       /* LanguageBaseAttributeIDList(0x0006) */
    0x35, 0x09,             /* DataElSeq 9 bytes */
    0x09, 0x65, 0x6e,       /* CsrUint16 0x656e */
    0x09, 0x00, 0x6a,       /* CsrUint16 0x006a */
    0x09, 0x01, 0x00,       /* CsrUint16 0x0100 */

    0x09, 0x00, 0x09,       /* BluetoothProfileDescriptorList(0x0009) */
    0x35, 0x08,             /* DataElSeq 8 bytes */
    0x35, 0x06,             /* DataElSeq 6 bytes */
    0x19, 0x11, 0x24,       /* uuid16_t HID(0x1124) */
    0x09, 0x01, 0x00,       /* CsrUint16 0x0100 */

    0x09, 0x00, 0x0d,       /* AdditionalProtocolDescriptorList(0x000d) */
    0x35, 0x0f,             /* DataElSeq 15 bytes */
    0x35, 0x0d,             /* DataElSeq 13 bytes */
    0x35, 0x06,             /* DataElSeq 6 bytes */
    0x19, 0x01, 0x00,       /* uuid16_t L2CAP(0x0100) */
    0x09, 0x00, 0x13,       /* CsrUint16 0x0013 */
    0x35, 0x03,             /* DataElSeq 3 bytes */
    0x19, 0x00, 0x11,       /* uuid16_t HIDP(0x0011) */

    0x09, 0x01, 0x00,       /* ServiceName(0x0100) = "QC Keyboard" */
    0x25, 0x0b,             /* String length 11 */
    'Q','C',' ','K','e','y','b','o','a','r','d',

    0x09, 0x01, 0x01,       /* ServiceDescription(0x0101) = "Keyboard" */
    0x25, 0x08,             /* String length 8 */
    'K','e','y','b','o','a','r','d',

    0x09, 0x01, 0x02,       /* ProviderName(0x0102) = "HIDEngine" */
    0x25, 0x09,             /* String length 9 */
    'H','I','D','E','n','g','i','n','e',

    0x09, 0x02, 0x00,       /* HIDDeviceReleaseNumber(0x0200) = "0x0100" */
    0x09, 0x01, 0x00,       /* CsrUint16 0x0100 */
    0x09, 0x02, 0x01,       /* HIDParserVersion(0x0201) = "0x0100" */
    0x09, 0x01, 0x00,       /* CsrUint16 0x0100 */

    0x09, 0x02, 0x02,       /* HIDDeviceSubclass(0x0202) = "0x40" */
    0x08, 0x40,             /* CsrUint8 0x40 */
    0x09, 0x02, 0x03,       /* HIDCountryCode(0x0203) = "0x33" */
    0x08, 0x33,             /* CsrUint8 0x33 */

    0x09, 0x02, 0x04,       /* HIDVirtualCable(0x0204) = "true" */
    0x28, 0x01,             /* CsrBool true */

    0x09, 0x02, 0x05,       /* HIDReconnectInitiate(0x0205) = "true" */
    0x28, 0x01,             /* CsrBool true */

    0x09, 0x02, 0x06,       /* HIDDescriptorList(0x0206) */
    0x35, 0x49,             /* DataElSeq 73 bytes */
    0x35, 0x47,             /* DataElSeq 71 bytes */
    0x08, 0x22,             /* CsrUint8 0x22 */
    0x25,0x43,              /* HID Descriptor Length 67 bytes */
    /* Start of Raw Data */
    0x05,0x01, /* USAGE_PAGE(Generic Desktop) */
    0x09,0x06, /* USAGE (Joystick) */
    0xa1,0x01, /* COLLECTION (Application) */
    0x05,0x07, /* USAGE_PAGE (Keypad) */
    0x85,0x01, /* REPORT_ID (0x01) */
    0x19,0xe0, /* USAGE_MIN (0xe0) */
    0x29,0xe7, /* USAGE_MAX (0xe7) */
    0x15,0x00, /* LOGICAL_MIN (0x0) */
    0x25,0x01, /* LOGICAL_MAX (0x1) */
    0x75,0x01, /* REPORT_SIZE (0x1) */
    0x95,0x08, /* REPORT_COUNT (0x8) */
    0x81,0x02, /* INPUT (data/var/abs) */
    0x95,0x01, /* REPORT_COUNT (0x1) */
    0x75,0x08, /* REPORT_SIZE (0x8) */
    0x81,0x01, /* INPUT (const/array/abs) */
    0x95,0x05, /* REPORT_COUNT (0x5) */
    0x75,0x01, /* REPORT_SIZE (0x1) */
    0x05,0x08, /* USAGE_PAGE (LEDS) */
    0x85,0x01, /* REPORT_ID (0x01) */
    0x19,0x01, /* USAGE_MIN (0x01) */
    0x29,0x05, /* USAGE_MAX (0x05) */
    0x91,0x02, /* OUTPUT (Var) */
    0x95,0x01, /* REPORT_COUNT (0x1) */
    0x75,0x03, /* REPORT_SIZE (0x3) */
    0x91,0x03, /* OUTPUT (Const) */
    0x95,0x06, /* REPORT_COUNT (0x6) */
    0x75,0x08, /* REPORT_SIZE (0x8) */
    0x15,0x00, /* LOGICAL_MIN (0x0) */
    0x25,0x65, /* LOGICAL_MAX (0x65) */
    0x05,0x07, /* USAGE_PAGE (Keypad) */
    0x19,0x00, /* USAGE_MIN (0x0) */
    0x29,0x65, /* USAGE_MAX (0x65) */
    0x81,0x00, /* INPUT (data/array/abs) */
    0xc0, /* END_COLLECTION */
    /* End of Raw Data */
    0x09, /* HIDLANGIDBaseList(0x0207) */
    0x02,
    0x07,
    0x35, /* DataElSeq 8 bytes */ /*270*/
    0x08,
    0x35, /* DataElSeq 6 bytes */
    0x06,
    0x09, /* CsrUint16 0x0409 */
    0x04,
    0x09,
    0x09, /* CsrUint16 0x0100 */
    0x01,
    0x00,
    0x09, /* HIDSDPDisable(0x0208) = "false" */
    0x02,
    0x08,
    0x28, /* CsrBool false */
    0x00,
    0x09, /* HIDBatteryPower(0x0209) = "true" */
    0x02,
    0x09,
    0x28, /* CsrBool true */
    0x01,
    0x09, /* HIDRemoteWake(0x020a) = "true" */ /*290*/
    0x02,
    0x0a,
    0x28, /* CsrBool true */
    0x01,
    0x09, /* HIDProfileVersion(0x020b) = "0x0100" */
    0x02,
    0x0b,
    0x09, /* CsrUint16 0x0100 */
    0x01,
    0x00,
    0x09, /* HIDSupervisionTimeout(0x020c) = "0x1f40" */
    0x02,
    0x0c,
    0x09, /* CsrUint16 0x1f40 */
    0x1f,
    0x40,
    0x09, /* HIDNormallyConnectable(0x020d) = "false" */
    0x02,
    0x0d,
    0x28, /* CsrBool false */
    0x00,
    0x09, /* HIDBootDevice(0x020e) = "false" */
    0x02,
    0x0e,
    0x28, /* CsrBool false */
    0x00
};

void appTestHiddRegister(void)
{
    uint16 flush_to = 0xFFFF; /* infinite flush timeout */
    HiddProfile_RegisterDevice(&testTask,
                               sizeof(hid_keyboard_service_record),
                               (uint8*)hid_keyboard_service_record,
                               flush_to);
}

void appTestHiddReactivate(void)
{
    HiddProfile_Deinit();
}

bool appTestHiddDisconnect(bdaddr *addr)
{
    return HiddProfile_Disconnect(addr);
}

bool appTestHiddConnect(bdaddr *addr)
{
    return HiddProfile_Connect(addr);
}

bool appTestHiddSendHandshake(hidd_handshake_result_t status)
{
    return HiddProfile_Handshake(status);
}

bool appTestHiddSendKbReport(uint8 ascii_key)
{
    bool status;
    uint8 report_len = 8; /* Keyboard input report size = 8 */
    uint8 *data = malloc(report_len);
    memset(data, 0, report_len);
    uint8 report_id = 1; /* 1 = keyboard, 2 = mouse */
    data[3] = ascii_key;
    status = HiddProfile_DataReq(report_id, report_len, data);
    free(data);
    return status;
}

bool appTestHiddSendCtrlReport(hidd_report_type_t type, uint8 report_id, uint16 size, uint8* data)
{
    bool status;
    status = HiddProfile_DataRes(type, report_id, size, data);
    return status;
}

#endif /*INCLUDE_HIDD_PROFILE*/
#endif /*USE_SYNERGY*/

static void testTaskHandler(Task task, MessageId id, Message message)
{
    static lp_power_mode   powermode = 42;
    static uint16          interval = (uint16)-1;

    UNUSED(task);

    switch (id)
    {
#ifdef USE_SYNERGY
        case CM_PRIM:
            handleSynergyCmPrim(task, message);
        break;
#ifdef INCLUDE_HIDD_PROFILE
        case HIDD_PROFILE_DATA_CFM:
            DEBUG_LOG_VERBOSE("HIDD_PROFILE_DATA_CFM");
        break;
        case HIDD_PROFILE_CONNECT_IND:
            DEBUG_LOG_VERBOSE("HIDD_PROFILE_CONNECT_IND");
        break;
        case HIDD_PROFILE_DISCONNECT_IND:
            DEBUG_LOG_VERBOSE("HIDD_PROFILE_DISCONNECT_IND");
        break;
        case HIDD_PROFILE_GET_REPORT_IND:
        {
            DEBUG_LOG_VERBOSE("HIDD_PROFILE_GET_REPORT_IND");
            HIDD_PROFILE_GET_REPORT_IND_T *ind = (HIDD_PROFILE_GET_REPORT_IND_T*)message;
            DEBUG_LOG_VERBOSE("Report Type: %d, Report ID: %d, Report Size: %d", ind->report_type, ind->reportid, ind->size);
        }
        break;
        case HIDD_PROFILE_ACTIVATE_CFM:
            DEBUG_LOG_VERBOSE("HIDD_PROFILE_ACTIVATE_CFM");
        break;
        case HIDD_PROFILE_DEACTIVATE_CFM:
            DEBUG_LOG_VERBOSE("HIDD_PROFILE_DEACTIVATE_CFM");
        break;
        case HIDD_PROFILE_SET_REPORT_IND:
        {
            DEBUG_LOG("HIDD_PROFILE_SET_REPORT_IND");
            HIDD_PROFILE_SET_REPORT_IND_T *ind = (HIDD_PROFILE_SET_REPORT_IND_T*)message;
            DEBUG_LOG_VERBOSE("Report Type: %d, Id: %d Report Len: %d", ind->type, ind->reportid, ind->reportLen);
            DEBUG_LOG_DATA(ind->data,ind->reportLen);
            free(ind->data);
            /* Send Handshake response based on reportid validity */
        }
        break;
        case HIDD_PROFILE_DATA_IND:
        {
            DEBUG_LOG_VERBOSE("HIDD_PROFILE_DATA_IND");
            HIDD_PROFILE_DATA_IND_T *ind = (HIDD_PROFILE_DATA_IND_T*)message;
            DEBUG_LOG_VERBOSE("Report Type: %d, Report Len: %d", ind->type, ind->reportLen);
            DEBUG_LOG_DATA(ind->data,ind->reportLen);
            free(ind->data);
        }
        break;
#endif /*INCLUDE_HIDD_PROFILE*/
#endif /*!USE_SYNERGY*/
        case CL_DM_ROLE_CFM:
            {
                const CL_DM_ROLE_CFM_T *cfm = (const CL_DM_ROLE_CFM_T *)message;
                bdaddr  peer;
                tp_bdaddr sink_addr;

                if (  !SinkGetBdAddr(cfm->sink, &sink_addr)
                   || !appDeviceGetPeerBdAddr(&peer))
                {
                    return;
                }

                if (   BdaddrIsSame(&peer, &sink_addr.taddr.addr)
                    && hci_success == cfm->status)
                {
                    if (hci_role_master == cfm->role)
                    {
                        DEBUG_LOG_VERBOSE("SCO FORWARDING LINK IS: MASTER");
                    }
                    else if (hci_role_slave == cfm->role)
                    {
                        DEBUG_LOG_VERBOSE("SCO FORWARDING LINK IS: SLAVE");
                    }
                    else
                    {
                        DEBUG_LOG_VERBOSE("SCO FORWARDING LINK STATE IS A MYSTERY: %d",cfm->role);
                    }
                    DEBUG_LOG_VERBOSE("SCO FORWARDING POWER MODE (cached) IS %d (sniff = %d)",powermode,lp_sniff);
                    DEBUG_LOG_VERBOSE("SCO FORWARDING INTERVAL (cached) IS %d",interval);
                }
            }
            break;

        case CL_DM_MODE_CHANGE_EVENT:
            {
                bdaddr peer;
                const CL_DM_MODE_CHANGE_EVENT_T *mode = (const CL_DM_MODE_CHANGE_EVENT_T *)message;

                if (!appDeviceGetPeerBdAddr(&peer))
                    return;

                if (BdaddrIsSame(&mode->bd_addr, &peer))
                {
                    powermode = mode->mode;
                    interval = mode->interval;
                }
            }
            break;

        case CL_DM_LOCAL_BD_ADDR_CFM:
            {
                const CL_DM_LOCAL_BD_ADDR_CFM_T *addr_cfm = (const CL_DM_LOCAL_BD_ADDR_CFM_T *)message;
                if (addr_cfm->status == hci_success)
                {
                    appTestReadLocalAddr_completed = TRUE;
                    appTestGetLocalAddr_address = addr_cfm->bd_addr;

                    DEBUG_LOG_ALWAYS("appTestReadLocalAddr completed. %04x:%02x:%06x",
                                     appTestGetLocalAddr_address.nap,
                                     appTestGetLocalAddr_address.uap,
                                     appTestGetLocalAddr_address.lap);
                }
                else
                {
                    DEBUG_LOG_WARN("appTestReadLocalAddr failed. Status:%d",
                                   addr_cfm->status);
                }
            }
            break;

        case KYMERA_NOTIFICATION_TONE_STARTED:
            {
                const KYMERA_NOTIFICATION_TONE_STARTED_T *tone_msg = (KYMERA_NOTIFICATION_TONE_STARTED_T *)message;

                DEBUG_LOG("testTaskHandler KYMERA_NOTIFICATION_TONE_STARTED received tone %p", tone_msg->tone);

                if(tone_msg->tone == app_tone_battery_empty)
                {
                    DEBUG_LOG_VERBOSE("testTaskHandler Received tone is app_tone_battery_empty");
                    power_off_tone_have_been_played = TRUE;
                }
                else if(tone_msg->tone == app_tone_av_connected)
                {
                    DEBUG_LOG_VERBOSE("testTaskHandler Received tone is app_tone_av_connected");
                }
                else
                {
                    DEBUG_LOG_VERBOSE("testTaskHandler Received tone is not yet recognised");
                }
            }
            break;

        case PEER_PAIR_LE_PAIR_CFM:
            {
                const PEER_PAIR_LE_PAIR_CFM_T *pair_cfm = (const PEER_PAIR_LE_PAIR_CFM_T *)message;

                DEBUG_LOG_ALWAYS("testTaskHandler Peer Pair LE completed. status enum:peer_pair_le_status_t:%d", pair_cfm->status);
            }
            break;
    }
}

void appTestScoFwdLinkStatus(void)
{
}

bool appTestScoFwdConnect(void)
{
    return FALSE;
}

bool appTestScoFwdDisconnect(void)
{
    return TRUE;
}

bool appTestPowerOff(void)
{
    DEBUG_LOG_ALWAYS("appTestPowerOff");
    return appPowerOffRequest();
}

/* Remove these once feature_if.h has been updated */
#define APTX_ADAPTIVE (40)
#define APTX_ADAPTIVE_MONO (41)

bool appTestLicenseCheck(void)
{
    const uint8 license_table[7] =
    {
        APTX_CLASSIC, APTX_CLASSIC_MONO,
        CVC_RECV, CVC_SEND_HS_1MIC, CVC_SEND_HS_2MIC_MO,
        APTX_ADAPTIVE_MONO
    };
    const bool license_enabled[7] =
    {
        appConfigAptxEnabled(), appConfigAptxEnabled(),
        TRUE, TRUE, TRUE,
        appConfigAptxAdaptiveEnabled()
    };

    bool licenses_ok = TRUE;

    DEBUG_LOG_ALWAYS("appTestLicenseCheck");
    for (int i = 0; i < ARRAY_DIM(license_table); i++)
    {
        if (license_enabled[i])
        {
            if (!FeatureVerifyLicense(license_table[i]))
            {
                DEBUG_LOG_ALWAYS("appTestLicenseCheck: License for feature %d not valid", license_table[i]);
                licenses_ok = FALSE;
            }
            else
                DEBUG_LOG_ALWAYS("appTestLicenseCheck: License for feature %d valid", license_table[i]);
        }
    }

    return licenses_ok;
}

void appTestConnectAfterPairing(bool enable)
{
    UNUSED(enable);
    DEBUG_LOG_WARN("appTestConnectAfterPairing ** DEPRECATED **");
}

bool appTestScoFwdForceDroppedPackets(unsigned percentage_to_drop, int multiple_packets)
{
    UNUSED(percentage_to_drop);
    UNUSED(multiple_packets);
    return FALSE;
}

bool appTestMicFwdLocalMic(void)
{
    return FALSE;
}

bool appTestMicFwdRemoteMic(void)
{
    return FALSE;
}

bool appTestIsPtsMode(void)
{
    return A2dpProfile_IsPtsMode();
}

bool appTestSetPtsMode(bool enable)
{
    DEBUG_LOG_ALWAYS("appTestSetPtsMode");

    A2dpProfile_SetPtsMode(enable);

    return TRUE;
}

bool appTestPtsSetAsPeer(void)
{
    bdaddr handset_addr;
    bdaddr peer_addr;
    device_t device;
    uint16 flags = 0;
    deviceType type = DEVICE_TYPE_EARBUD;

    if (appDeviceGetHandsetBdAddr(&handset_addr))
    {
        device = BtDevice_GetDeviceForBdAddr(&handset_addr);
        if (device)
        {
            if (appDeviceGetPeerBdAddr(&peer_addr))
            {
                ConnectionAuthSetPriorityDevice(&peer_addr, FALSE);
                ConnectionSmDeleteAuthDevice(&peer_addr);
            }
            Device_SetProperty(device, device_property_type, &type, sizeof(deviceType));
            Device_GetPropertyU16(device, device_property_flags, &flags);
            flags |= DEVICE_FLAGS_SECONDARY_ADDR;
            flags |= DEVICE_FLAGS_IS_PTS;
            Device_SetPropertyU16(device, device_property_flags, flags);
            Device_SetPropertyU32(device, device_property_supported_profiles, DEVICE_PROFILE_A2DP);

            DeviceDbSerialiser_Serialise();

            SystemReboot_Reboot();
            return TRUE;
        }
    }

    DEBUG_LOG_ALWAYS("appTestPtsSetAsPeer, failed");
    return FALSE;
}

/*! \brief Determine if appConfigScoForwardingEnabled is TRUE
*/
bool appTestIsScoFwdIncluded(void)
{
    return FALSE;
}

/*! \brief Determine if appConfigMicForwardingEnabled is TRUE.
*/
bool appTestIsMicFwdIncluded(void)
{
    return FALSE;
}

/*! \brief Macro to create function declaration/definition that returns the value
    of the specified configuration.
    \param returntype The type returned by the function.
    \param name Given a configuration named appConfigXYZ(), XYZ should be passed
    to the name argument and a function will be created called appTestConfigXYZ().*/
#define MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(returntype, name) \
returntype appTestConfig##name(void);                   \
returntype appTestConfig##name(void)                    \
{                                                       \
    return appConfig##name();                           \
}

/*! \name Config Accessor functions.
    \note These don't have a public declaration as they are not expected to be
    called within the earbud code - the appConfig functions/macros should be used
    instead.
*/
//!@{
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(uint16, BatteryFullyCharged)
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(uint16, BatteryVoltageOk)
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(uint16, BatteryVoltageLow)
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(uint16, BatteryVoltageCritical)
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(int8, BatteryChargingTemperatureMax)
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(int8, BatteryChargingTemperatureMin)
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(int8, BatteryDischargingTemperatureMax)
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(int8, BatteryDischargingTemperatureMin)
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(uint32, BatteryReadingPeriodMs)
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(uint16, ChargerTrickleCurrent)
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(uint16, ChargerPreCurrent)
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(uint16, ChargerPreFastThresholdVoltage)
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(uint16, ChargerFastCurrent)
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(uint16, ChargerTerminationCurrent)
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(uint16, ChargerTerminationVoltage)
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(uint32, ChargerPreChargeTimeoutMs)
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(uint32, ChargerFastChargeTimeoutMs)
#ifdef INCLUDE_TEMPERATURE
MAKE_TEST_API_FUNC_FROM_CONFIG_FUNC(uint32, TemperatureReadingPeriodMs)
#endif
//!@}

#ifdef INCLUDE_EXTRA_TESTS

const uint16 FFA_coefficients[15] = {0xFD66, 0x00C4, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0101, 0xFFE6, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000};
const uint16 FFB_coefficients[15] = {0xFE66, 0xFF5F, 0xFFC8, 0x0028, 0x0071, 0x001C, 0xFFC9, 0x00E1, 0xFFEC, 0xFF18, 0xFFF2, 0x0028, 0x0015, 0x0027, 0xFFE6};
const uint16 FB_coefficients[15] =  {0x004C, 0xFCE0, 0xFF5E, 0x0118, 0x0094, 0x0016, 0xFFCB, 0x00C6, 0x0100, 0xFDD4, 0xFF4C, 0x0161, 0xFF64, 0xFFFD, 0x0057};
const uint16 ANC0_FLP_A_SHIFT_1 = 8; //LPF
const uint16 ANC0_FLP_A_SHIFT_2 = 9;
const uint16 ANC0_FLP_B_SHIFT_1 = 8;
const uint16 ANC0_FLP_B_SHIFT_2 = 8;
const uint16 ANC1_FLP_A_SHIFT_1 = 5;
const uint16 ANC1_FLP_A_SHIFT_2 = 5;
const uint16 ANC1_FLP_B_SHIFT_1 = 5;
const uint16 ANC1_FLP_B_SHIFT_2 = 5;
const uint16 DC_FILTER_SHIFT = 7;

#include <operator.h>
#include <vmal.h>
#include <micbias.h>
#include <audio_anc.h>
#include <cap_id_prim.h>
#include <opmsg_prim.h>
#include "kymera.h"

extern void appKymeraExternalAmpSetup(void);
extern void appKymeraExternalAmpControl(bool enable);

extern void appTestAncSetFilters(void);
void appTestAncSetFilters(void)
{
    //Source ADC_A = (Source)PanicFalse(StreamAudioSource(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A));
    Source ADC_A = (Source)PanicFalse(StreamAudioSource(AUDIO_HARDWARE_DIGITAL_MIC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A));

    /* lLad IIRs for both ANC0 and ANC1 */
    PanicFalse(AudioAncFilterIirSet(AUDIO_ANC_INSTANCE_0, AUDIO_ANC_PATH_ID_FFA, 15, FFA_coefficients));
    PanicFalse(AudioAncFilterIirSet(AUDIO_ANC_INSTANCE_0, AUDIO_ANC_PATH_ID_FFB, 15, FFB_coefficients));
    PanicFalse(AudioAncFilterIirSet(AUDIO_ANC_INSTANCE_0, AUDIO_ANC_PATH_ID_FB, 15, FB_coefficients));
    //PanicFalse(AudioAncFilterIirSet(AUDIO_ANC_INSTANCE_1, AUDIO_ANC_PATH_ID_FFA, 15, FFA_coefficients));
    //PanicFalse(AudioAncFilterIirSet(AUDIO_ANC_INSTANCE_1, AUDIO_ANC_PATH_ID_FFB, 15, FFB_coefficients));
    //PanicFalse(AudioAncFilterIirSet(AUDIO_ANC_INSTANCE_1, AUDIO_ANC_PATH_ID_FB, 15, FB_coefficients));

    /* Set LPF filter shifts */
    PanicFalse(AudioAncFilterLpfSet(AUDIO_ANC_INSTANCE_0, AUDIO_ANC_PATH_ID_FFA, ANC0_FLP_A_SHIFT_1, ANC0_FLP_A_SHIFT_2));
    //PanicFalse(AudioAncFilterLpfSet(AUDIO_ANC_INSTANCE_1, AUDIO_ANC_PATH_ID_FFA, ANC1_FLP_A_SHIFT_1, ANC1_FLP_A_SHIFT_2));

    /* Set DC filters */
    PanicFalse(SourceConfigure(ADC_A, STREAM_ANC_FFA_DC_FILTER_ENABLE, 1));
    PanicFalse(SourceConfigure(ADC_A, STREAM_ANC_FFA_DC_FILTER_SHIFT, DC_FILTER_SHIFT));
    //PanicFalse(SourceConfigure(ADC_B, STREAM_ANC_FFA_DC_FILTER_ENABLE, 1));
    //PanicFalse(SourceConfigure(ADC_B, STREAM_ANC_FFA_DC_FILTER_SHIFT, DC_FILTER_SHIFT));

    /* Set LPF gains */
    PanicFalse(SourceConfigure(ADC_A, STREAM_ANC_FFA_GAIN, 128));
    PanicFalse(SourceConfigure(ADC_A, STREAM_ANC_FFA_GAIN_SHIFT, 0));
    //PanicFalse(SourceConfigure(ADC_B, STREAM_ANC_FFA_GAIN, 128));
    //PanicFalse(SourceConfigure(ADC_B, STREAM_ANC_FFA_GAIN_SHIFT, 0));
}

extern void appTestAncSetup(void);
void appTestAncSetup(void)
{
    const uint16 sample_rate = 48000;
    Source adc_a = NULL, adc_b = NULL;
    Sink dac_l = NULL, dac_r = NULL;

    OperatorsFrameworkEnable();

    if(appConfigAncFeedForwardMic())
    {
        Microphones_SetMicRate(appConfigAncFeedForwardMic(), sample_rate, high_priority_user);
        adc_a = Microphones_TurnOnMicrophone(appConfigAncFeedForwardMic(), high_priority_user);
    }
    if(appConfigAncFeedBackMic())
    {
        Microphones_SetMicRate(appConfigAncFeedBackMic(), sample_rate, high_priority_user);
        adc_b = Microphones_TurnOnMicrophone(appConfigAncFeedBackMic(), high_priority_user);
    }
    SourceSynchronise(adc_a, adc_b);

    /* Get the DAC output sinks */
    dac_l = (Sink)PanicFalse(StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A));
    PanicFalse(SinkConfigure(dac_l, STREAM_CODEC_OUTPUT_RATE, sample_rate));
    PanicFalse(SinkConfigure(dac_l, STREAM_CODEC_OUTPUT_GAIN, 12));
    dac_r = (Sink)StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_B);
    if(dac_r)
    {
        PanicFalse(SinkConfigure(dac_r, STREAM_CODEC_OUTPUT_RATE, sample_rate));
        PanicFalse(SinkConfigure(dac_r, STREAM_CODEC_OUTPUT_GAIN, 12));
    }

    /* Set DAC gains */
    if(dac_l)
    {
        PanicFalse(SinkConfigure(dac_r, STREAM_CODEC_RAW_OUTPUT_GAIN, 7));
    }
    if(dac_r)
    {
        PanicFalse(SinkConfigure(dac_r, STREAM_CODEC_RAW_OUTPUT_GAIN, 7));
    }

    /* feedforward or feedback with analog mics */
    if(dac_l && adc_a)
    {
        PanicFalse(SourceConfigure(adc_a, STREAM_ANC_INSTANCE, AUDIO_ANC_INSTANCE_0));
    }
    if(dac_r && adc_b)
    {
        PanicFalse(SourceConfigure(adc_b, STREAM_ANC_INSTANCE, AUDIO_ANC_INSTANCE_1));
    }
    if(dac_l && adc_a)
    {
        PanicFalse(SourceConfigure(adc_a, STREAM_ANC_INPUT, AUDIO_ANC_PATH_ID_FFA));
    }
    if(dac_r && adc_b)
    {
        PanicFalse(SourceConfigure(adc_b, STREAM_ANC_INPUT, AUDIO_ANC_PATH_ID_FFA));
    }

    /* Associate DACS */
    if(dac_l && adc_a)
    {
        PanicFalse(SinkConfigure(dac_l, STREAM_ANC_INSTANCE, AUDIO_ANC_INSTANCE_0));
    }
    if(dac_r && adc_b)
    {
        PanicFalse(SinkConfigure(dac_r, STREAM_ANC_INSTANCE, AUDIO_ANC_INSTANCE_1));
    }

    /* Setup ANC filters */
    appTestAncSetFilters();

    /* Turn on ANC */
    PanicFalse(AudioAncStreamEnable(0x9, dac_r && adc_b ? 0x09 : 0x0));

    appKymeraExternalAmpSetup();
    appKymeraExternalAmpControl(TRUE);
}

#define GAIN_DB_TO_Q6N_SF (11146541)
#define GAIN_DB(x)      ((int32)(GAIN_DB_TO_Q6N_SF * (x)))

extern void appTestAudioPassthrough(void);
void appTestAudioPassthrough(void)
{
    const uint16 sample_rate = 48000;
    const int32 initial_gain = GAIN_DB(0);

    OperatorsFrameworkEnable();

    /* Set up MICs */
    Microphones_SetMicRate(appConfigAncFeedForwardMic(), sample_rate, high_priority_user);
    Source ADC_A = Microphones_TurnOnMicrophone(appConfigAncFeedForwardMic(), high_priority_user);
    Microphones_SetMicRate(appConfigAncFeedBackMic(), sample_rate, high_priority_user);
    Source ADC_B = Microphones_TurnOnMicrophone(appConfigAncFeedBackMic(), high_priority_user);
    SourceSynchronise(ADC_A, ADC_B);

    /* Get the DAC output sinks */
    Sink DAC_L = (Sink)PanicFalse(StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A));
    PanicFalse(SinkConfigure(DAC_L, STREAM_CODEC_OUTPUT_RATE, sample_rate));
    PanicFalse(SinkConfigure(DAC_L, STREAM_CODEC_OUTPUT_GAIN, 12));
    Sink DAC_R = (Sink)StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_B);
    if (DAC_R)
    {
        PanicFalse(SinkConfigure(DAC_R, STREAM_CODEC_OUTPUT_RATE, sample_rate));
        PanicFalse(SinkConfigure(DAC_R, STREAM_CODEC_OUTPUT_GAIN, 12));
    }

    /* Set DAC gains */
    PanicFalse(SinkConfigure(DAC_L, STREAM_CODEC_RAW_OUTPUT_GAIN, 7));
    if (DAC_R)
        PanicFalse(SinkConfigure(DAC_R, STREAM_CODEC_RAW_OUTPUT_GAIN, 7));

    /* Now create the operator for routing the audio */
    Operator passthrough = (Operator)PanicFalse(VmalOperatorCreate(CAP_ID_BASIC_PASS));

    /* Configure the operators
     * Using the information from CS-312572-UG, Kymera Capability
     * Library User Guide
     */
    uint16 set_gain[] = { OPMSG_COMMON_ID_SET_PARAMS, 1, 1, 1, UINT32_MSW(initial_gain), UINT32_LSW(initial_gain) };
    PanicFalse(VmalOperatorMessage(passthrough, set_gain, 6, NULL, 0));

    /* And connect everything */
    /* ...line_in to the passthrough */
    PanicFalse(StreamConnect(ADC_A, StreamSinkFromOperatorTerminal(passthrough, 0)));
    if (ADC_B && DAC_R)
        PanicFalse(StreamConnect(ADC_B, StreamSinkFromOperatorTerminal(passthrough, 1)));

    /* ...and passthrough to line out */
    PanicFalse(StreamConnect(StreamSourceFromOperatorTerminal(passthrough, 0), DAC_L));
    if (DAC_R)
        PanicFalse(StreamConnect(StreamSourceFromOperatorTerminal(passthrough, 1), DAC_R));

    /* Finally start the operator */
    PanicFalse(OperatorStartMultiple(1, &passthrough, NULL));

    appKymeraExternalAmpSetup();
    appKymeraExternalAmpControl(TRUE);
}



#include <usb_hub.h>

extern void appTestUsbAudioPassthrough(void);
void appTestUsbAudioPassthrough(void)
{
    const uint16 sample_rate = 48000;

    OperatorsFrameworkEnable();

    /* Attach USB */
    //UsbHubAttach();

    /* Get the DAC output sinks */
    Sink DAC_L = (Sink)PanicFalse(StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A));
    PanicFalse(SinkConfigure(DAC_L, STREAM_CODEC_OUTPUT_RATE, sample_rate));
    PanicFalse(SinkConfigure(DAC_L, STREAM_CODEC_OUTPUT_GAIN, 12));
    Sink DAC_R = (Sink)StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_B);
    if (DAC_R)
    {
        PanicFalse(SinkConfigure(DAC_R, STREAM_CODEC_OUTPUT_RATE, sample_rate));
        PanicFalse(SinkConfigure(DAC_R, STREAM_CODEC_OUTPUT_GAIN, 12));
    }

    /* Set DAC gains */
    PanicFalse(SinkConfigure(DAC_L, STREAM_CODEC_RAW_OUTPUT_GAIN, 7));
    if (DAC_R)
        PanicFalse(SinkConfigure(DAC_R, STREAM_CODEC_RAW_OUTPUT_GAIN, 7));

    /* Now create the operator for routing the audio */
    Operator usb_rx = (Operator)PanicFalse(VmalOperatorCreate(CAP_ID_USB_AUDIO_RX));

    /* Configure the operators
     * Using the information from CS-312572-UG, Kymera Capability
     * Library User Guide
     */
    uint16 set_config[] =
    {
        OPMSG_USB_AUDIO_ID_SET_CONNECTION_CONFIG,
        0,                  // data_format
        sample_rate / 25,   // sample_rate
        1,                  // number_of_channels
        2 * 8,              // subframe_size
        2 * 8               // subframe_resolution
    };

    PanicFalse(VmalOperatorMessage(usb_rx, set_config, 6, NULL, 0));

    /* And connect everything */
    /* ...USB sournce to USB Rx operator */
    PanicFalse(StreamConnect(StreamUsbEndPointSource(end_point_iso_in), StreamSinkFromOperatorTerminal(usb_rx, 0)));

    /* ...and USB Rx operator to line out */
    PanicFalse(StreamConnect(StreamSourceFromOperatorTerminal(usb_rx, 0), DAC_L));
    if (DAC_R)
        PanicFalse(StreamConnect(StreamSourceFromOperatorTerminal(usb_rx, 1), DAC_R));

    /* Finally start the operator */
    PanicFalse(OperatorStartMultiple(1, &usb_rx, NULL));

    appKymeraExternalAmpSetup();
    appKymeraExternalAmpControl(TRUE);
}

#define ANC_TUNING_SINK_USB_LEFT      0 /*can be any other backend device. PCM used in this tuning graph*/
#define ANC_TUNING_SINK_MIC1_LEFT     4 /* must be connected to internal ADC. Analog or digital */

extern void appTestAncTuningSetSource(void);
void appTestAncTuningSetSource(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    uint16 set_source_control[8] =
    {
        0x2002, 2,
        6, 0, ANC_TUNING_SINK_MIC1_LEFT,
        8, 0, ANC_TUNING_SINK_USB_LEFT,
    };
    PanicFalse(VmalOperatorMessage(theKymera->anc_tuning,
                                   &set_source_control, SIZEOF_OPERATOR_MESSAGE(set_source_control),
                                   NULL, 0));
}

#endif /* INCLUDE_EXTRA_TESTS */

bool appTestEnterDfuWhenEnteringCase(bool unused)
{
    bool enter_dfu_logical_input_sent = FALSE;

    DEBUG_LOG_ALWAYS("appTestEnterDfuWhenEnteringCase");

    UNUSED(unused);

    if (appPeerSigIsConnected() && appSmIsOutOfCase())
    {
        MessageSend(LogicalInputSwitch_GetTask(), LI_MFB_BUTTON_RELEASE_DFU, NULL);
        enter_dfu_logical_input_sent = TRUE;
    }

    DEBUG_LOG_ALWAYS("appTestEnterDfuWhenEnteringCase cmd_sent=%d", enter_dfu_logical_input_sent);

    return enter_dfu_logical_input_sent;
}

bool appTestIsInitialisationCompleted(void)
{
    bool completed = SystemState_GetState() > system_state_initialised;

    DEBUG_LOG_ALWAYS("appTestIsInitialisationCompleted:%d", completed);

    return completed;
}

bool appTestIsPrimary(void)
{
    bool prim = appSmIsPrimary();

    DEBUG_LOG_ALWAYS("appTestIsPrimary: %d",prim);

    return prim;
}

bool appTestIsRight(void)
{
    bool right = appConfigIsRight();

    DEBUG_LOG_ALWAYS("appTestIsRight: %d", right);

    return right;
}

bool appTestIsTopologyRole(app_test_topology_role_enum_t checked_role)
{
    bool role_matches = FALSE;

    switch (checked_role)
    {
        case app_test_topology_role_none:
            role_matches = !TwsTopology_IsRolePrimary() && !TwsTopology_IsRoleSecondary();
            DEBUG_LOG_ALWAYS("appTestIsTopologyRole. No role:%d", role_matches);
            break;

        case app_test_topology_role_any_primary:
            role_matches = TwsTopology_IsRolePrimary();
            DEBUG_LOG_ALWAYS("appTestIsTopologyRole. Primary:%d (Peer Connected:%d)",
                            role_matches, TwsTopology_IsRolePrimaryConnectedToPeer());
            break;

        case app_test_topology_role_primary:
            role_matches = TwsTopology_IsRolePrimaryConnectedToPeer();
            DEBUG_LOG_ALWAYS("appTestIsTopologyRole. Primary(Full):%d", role_matches);
            break;

        case app_test_topology_role_acting_primary:
            role_matches = TwsTopology_IsRoleStandAlonePrimary();
            DEBUG_LOG_ALWAYS("appTestIsTopologyRole. Acting Primary:%d", role_matches);
            break;

        case app_test_topology_role_secondary:
            role_matches = TwsTopology_IsRoleSecondary();
            DEBUG_LOG_ALWAYS("appTestIsTopologyRole. Secondary:%d", role_matches);
            break;

        default:
            DEBUG_LOG_ALWAYS("appTestIsTopologyRole. Unsupported role:%d",checked_role);
            Panic();
            break;
    }

    return role_matches;
}


bool appTestIsTopologyIdle(void)
{
    bool idle = !TwsTopology_IsRolePrimary() && !TwsTopology_IsRoleSecondary();

    DEBUG_LOG_ALWAYS("appTestIsTopologyIdle:%d", idle);

    return idle;
}


bool appTestIsTopologyRunning(void)
{
    bool running = twsTopology_IsRunning();

    DEBUG_LOG_ALWAYS("appTestIsTopologyRunning:%d", running);

    return running;
}


/*! List of application states to be included in specific debug.

    Macros are used as it allows us to define a string for log
    purposes, and create an entry in a switch statement while
    keeping the two synchronised.
 */
#define NAMED_STATES(APP_S) \
    APP_S(APP_STATE_STARTUP) \
    APP_S(APP_STATE_IN_CASE_IDLE) \
    APP_S(APP_STATE_IN_CASE_DFU) \
    APP_S(APP_STATE_OUT_OF_CASE_IDLE) \
    APP_S(APP_STATE_IN_EAR_IDLE)

/*! Macro to create a hydra logging string for each state */
#define HYD_STRING(_state) HYDRA_LOG_STRING(HLS_STATE_NAME_ ## _state, #_state);
/*! Macro to create a hydra logging string for each state */
#define STATE_CASE(_state) case _state: state_name = HLS_STATE_NAME_ ## _state; break;

bool appTestIsApplicationState(appState checked_state)
{
    bool state_matches;
    appState state = appSmGetState();
    const char *state_name;

    NAMED_STATES(HYD_STRING)

    state_matches = state == checked_state;

    switch (checked_state)
    {
        NAMED_STATES(STATE_CASE)

        default:
            DEBUG_LOG_ALWAYS("appTestIsApplicationState. State:x%x:%d (state is x%x)",
                            checked_state, state_matches, state);
            return state_matches;
    }

    if (state_matches)
    {
        DEBUG_LOG_ALWAYS("appTestIsApplicationState. %s:%d", state_name, state_matches);
    }
    else
    {
        DEBUG_LOG_ALWAYS("appTestIsApplicationState. %s:%d (state is x%x)",
                            state_name, state_matches, state);
    }
    return state_matches;
}


bool appTestIsPeerFindRoleRunning(void)
{
    bool running = peer_find_role_is_running();

    DEBUG_LOG_ALWAYS("appTestIsPeerFindRoleRunning:%d", running);

    return running;
}


bool appTestIsPeerPairLeRunning(void)
{
    bool running = PeerPairLeIsRunning();

    DEBUG_LOG_ALWAYS("appTestIsPeerPairLeRunning:%d", running);

    return running;
}


#ifdef INCLUDE_EXTRA_TESTS
void EarbudTest_DeviceDatabaseReport(void)
{
    appTest_DeviceDatabaseReport();
}
#endif

extern uint8 profile_list[4];

static device_t earbudTest_GetHandset(void)
{
    bool is_mru_handset = TRUE;
    device_t handset_device = DeviceList_GetFirstDeviceWithPropertyValue(device_property_mru, &is_mru_handset, sizeof(uint8));
    if (!handset_device)
    {
        bdaddr handset_address = {0,0,0};
        if (appDeviceGetHandsetBdAddr(&handset_address))
        {
            handset_device = BtDevice_GetDeviceForBdAddr(&handset_address);
        }
    }
    return handset_device;
}

void EarbudTest_ConnectHandset(void)
{
    if (appSmIsPrimary())
    {
        DEBUG_LOG_ALWAYS("EarbudTest_ConnectHandset");
        PrimaryRules_SetEvent(RULE_EVENT_CONNECT_HANDSET_USER);
    }
    else
    {
        DEBUG_LOG_ERROR("EarbudTest_ConnectHandset called in wrong role");
    }
}

void EarbudTest_DisconnectHandset(void)
{
    DEBUG_LOG_ALWAYS("EarbudTest_DisconnectHandset");
    EarbudTest_DisconnectHandsetExcept(0);
}

void EarbudTest_DisconnectHandsetExcept(uint32 profiles)
{
    bdaddr handset_addr = {0};

    if (appDeviceGetHandsetBdAddr(&handset_addr))
    {
        DEBUG_LOG_ALWAYS("EarbudTest_DisconnectHandsetExcept lap=%6x, except 0x%08x", handset_addr.lap, profiles);

        HandsetService_DisconnectRequest(SmGetTask(), &handset_addr, profiles);
    }
}

bool appTestIsHandsetFullyConnected(void)
{
    bool connected = FALSE;
    device_t handset_device = earbudTest_GetHandset();

    if (handset_device)
    {
        connected = HandsetService_Connected(handset_device);
        DEBUG_LOG_ALWAYS("appTestIsHandsetFullyConnected:%d", connected);
    }
    else
    {
        DEBUG_LOG_ALWAYS("appTestIsHandsetFullyConnected:0  No handset device");
    }
    return connected;
}

bool appTestIsHandsetAclConnected(void)
{
    bool connected = FALSE;
    device_t handset_device = earbudTest_GetHandset();

    if (handset_device)
    {
        bdaddr addr = DeviceProperties_GetBdAddr(handset_device);
        connected = ConManagerIsConnected(&addr);
        DEBUG_LOG_ALWAYS("appTestIsHandsetAclConnected:%d 0x%06x", connected, addr.lap);
    }
    else
    {
        DEBUG_LOG_ALWAYS("appTestIsHandsetAclConnected:0 No handset device/addr");
    }
    return connected;
}


bool appTestPrimaryAddressIsFromThisBoard(void)
{
    bdaddr self;
    uint16 flags;
    bool is_primary = FALSE;

    HYDRA_LOG_STRING(mine, "MINE");
    HYDRA_LOG_STRING(peers, "PEERS");


    appDeviceGetMyBdAddr(&self);

    if (appDeviceGetFlags(&self, &flags))
    {
        if (flags & DEVICE_FLAGS_MIRRORING_C_ROLE)
        {
            is_primary = TRUE;
        }
    }

    DEBUG_LOG_ALWAYS("appTestPrimaryAddressIsFromThisBoard %04x%02x%06x %s",
            self.nap, self.uap, self.lap,
            is_primary ? mine : peers);

    return is_primary;
}

static void earbudTest_RestartFindRole(void)
{
    appTestPhyStateInCaseEvent();
    MessageCancelAll((Task) &delayedInternalTestEventHandlerTask, WAIT_FOR_IDLE_THEN_BRING_OUT_OF_CASE);
    MessageSendLater((Task) &delayedInternalTestEventHandlerTask, WAIT_FOR_IDLE_THEN_BRING_OUT_OF_CASE,
                                         NULL, 100);
}

void EarbudTest_PeerFindRoleOverrideScore(uint16 score)
{
    DEBUG_LOG_ALWAYS("EarbudTest_PeerFindRoleOverrideScore 0x%x", score);
    PeerFindRole_OverrideScore((grss_figure_of_merit_t)score);
    if(!appTestIsTopologyIdle())
    {
        earbudTest_RestartFindRole();
    }
}

bool EarbudTest_IsPeerSignallingConnected(void)
{
    bool connected = appPeerSigIsConnected();
    DEBUG_LOG_ALWAYS("EarbudTest_IsPeerSignallingConnected connected %u", connected);
    return connected;
}

bool EarbudTest_IsPeerSignallingDisconnected(void)
{
    bool disconnected = appPeerSigIsDisconnected();
    DEBUG_LOG_ALWAYS("EarbudTest_IsPeerSignallingDisconnected disconnected %u", disconnected);
    return disconnected;
}

#ifdef INCLUDE_HDMA_RSSI_EVENT
const hdma_thresholds_t *appTestGetRSSIthreshold(void)
{
    DEBUG_LOG_VERBOSE("halfLife: critical [%d] high [%d], low [%d]", rssi.halfLife_ms.critical, rssi.halfLife_ms.high, rssi.halfLife_ms.low);
    DEBUG_LOG_VERBOSE("maxAge: critical [%d] high [%d], low [%d]", rssi.maxAge_ms.critical, rssi.maxAge_ms.high, rssi.maxAge_ms.low);
    DEBUG_LOG_VERBOSE("absThreshold: critical [%d] high [%d], low [%d]", rssi.absThreshold.critical, rssi.absThreshold.high, rssi.absThreshold.low);
    DEBUG_LOG_VERBOSE("relThreshold: critical [%d] high [%d], low [%d]", rssi.relThreshold.critical, rssi.relThreshold.high, rssi.relThreshold.low);
    return &rssi;
}
#endif

#ifdef INCLUDE_HDMA_MIC_QUALITY_EVENT
const hdma_thresholds_t *appTestGetMICthreshold(void)
{
    DEBUG_LOG_VERBOSE("halfLife: critical [%d] high [%d], low [%d]", mic.halfLife_ms.critical, mic.halfLife_ms.high, mic.halfLife_ms.low);
    DEBUG_LOG_VERBOSE("maxAge: critical [%d] high [%d], low [%d]", mic.maxAge_ms.critical, mic.maxAge_ms.high, mic.maxAge_ms.low);
    DEBUG_LOG_VERBOSE("absThreshold: critical [%d] high [%d], low [%d]", mic.absThreshold.critical, mic.absThreshold.high, mic.absThreshold.low);
    DEBUG_LOG_VERBOSE("relThreshold: critical [%d] high [%d], low [%d]", mic.relThreshold.critical, mic.relThreshold.high, mic.relThreshold.low);
    return &mic;
}
#endif


/* private API added to upgrade library */
bool appTestUpgradeResetState(void)
{
    DEBUG_LOG_ALWAYS("appTestUpgradeResetState");

    return Dfu_ClearPsStore();
}

#ifdef INCLUDE_DFU
void EarbudTest_EnterInCaseDfu(void)
{
    DEBUG_LOG_ALWAYS("EarbudTest_EnterInCaseDfu");

    appSmEnterDfuModeInCase(TRUE, TRUE);
    appTestPhyStateInCaseEvent();
}
#endif

uint16 appTestSetTestNumber(uint16 test_number)
{
    test_support.testcase = test_number;

    return test_number;
}

uint16 appTestSetTestIteration(uint16 test_iteration)
{
    test_support.iteration = test_iteration;

    return test_iteration;
}

uint16 appTestWriteMarker(uint16 marker)
{
    static unsigned testcase = 0;
    test_support.last_marker = marker;

    if (   test_support.testcase
        && (  test_support.testcase != testcase
           || marker == 0))
    {
        testcase = test_support.testcase;

        if (test_support.iteration)
        {
            DEBUG_LOG_VERBOSE("@@@Testcase:%d  Iteration:%d -------------------------------", testcase, test_support.iteration);
        }
        else
        {
            DEBUG_LOG_VERBOSE("@@@Testcase:%d  ------------------------------------------", testcase);
        }
    }

    if (marker)
    {
        if (test_support.testcase && test_support.iteration)
        {
            DEBUG_LOG_VERBOSE("@@@Testcase marker: TC%d Iteration:%d Step:%d *************************",
                    testcase, test_support.iteration, marker);
        }
        else if (test_support.testcase)
        {
            DEBUG_LOG_VERBOSE("@@@Testcase marker: TC%d Step:%d *************************",
                    testcase, marker);
        }
        else
        {
            DEBUG_LOG_VERBOSE("@@@Testcase marker: Iteration:%d Step:%d *************************",
                    test_support.iteration, marker);
        }
    }

    return marker;
}


void appTestCvcPassthrough(void)
{
    DEBUG_LOG_ALWAYS("appTestCvcPassthrough");
    Kymera_SetCvcPassthroughMode(KYMERA_CVC_RECEIVE_PASSTHROUGH | KYMERA_CVC_SEND_PASSTHROUGH, 0);
}

void EarbudTest_SetAncEnable(void)
{
    if (appPhyStateIsOutOfCase())
    {
        DEBUG_LOG_ALWAYS("EarbudTest_SetAncEnable");
        Ui_InjectUiInput(ui_input_anc_on);
    }
}

void EarbudTest_SetAncDisable(void)
{
    if (appPhyStateIsOutOfCase())
    {
        DEBUG_LOG_ALWAYS("EarbudTest_SetAncDisable");
        Ui_InjectUiInput(ui_input_anc_off);
    }
}

void EarbudTest_SetAncToggleOnOff(void)
{
    if (appPhyStateIsOutOfCase())
    {
        DEBUG_LOG_ALWAYS("EarbudTest_SetAncToggleOnOff");
        Ui_InjectUiInput(ui_input_anc_toggle_on_off);
    }
}

#if defined(ENABLE_ADAPTIVE_ANC) && defined(ENABLE_WIND_DETECT)
void EarbudTest_EnableWindDetect(void)
{
    if (appPhyStateIsOutOfCase())
    {
        Ui_InjectUiInput(ui_input_anc_wind_enable);
    }
}

void EarbudTest_DisableWindDetect(void)
{
    if (appPhyStateIsOutOfCase())
    {
        Ui_InjectUiInput(ui_input_anc_wind_disable);
    }
}

static void earbudTest_SendWindEvent(kymera_msg_t msg_id, wind_detect_intensity_t intensity)
{
    if(appPhyStateIsOutOfCase())
    {
        kymeraTaskData *theKymera = KymeraGetTaskData();
        MESSAGE_MAKE(ind, kymera_aanc_event_msg_t);
        ind->info = (uint16)intensity;
        TaskList_MessageSendWithSize(theKymera->client_tasks, msg_id, ind, sizeof (ind));
    }
}

void EarbudTest_SetStageOneWindDetectWithIntensity(wind_detect_intensity_t intensity)
{
    DEBUG_LOG_ALWAYS("EarbudTest_SetStageOneWindDetectWithIntensity, intensity: enum:wind_detect_intensity_t:%d", intensity);
    earbudTest_SendWindEvent(KYMERA_WIND_STAGE1_DETECTED, intensity);
}

void EarbudTest_SetStageTwoWindDetectWithIntensity(wind_detect_intensity_t intensity)
{
    DEBUG_LOG_ALWAYS("EarbudTest_SetStageTwoWindDetectWithIntensity, intensity: enum:wind_detect_intensity_t:%d", intensity);
    earbudTest_SendWindEvent(KYMERA_WIND_STAGE2_DETECTED, intensity);
}

void EarbudTest_SetStageOneWindDetect(void)
{
    EarbudTest_SetStageOneWindDetectWithIntensity(wind_detect_intensity_none);
}

void EarbudTest_SetStageTwoWindDetect(void)
{
    EarbudTest_SetStageTwoWindDetectWithIntensity(wind_detect_intensity_none);
}

void EarbudTest_SetStageOneWindReleased(void)
{
    DEBUG_LOG_ALWAYS("EarbudTest_SetStageOneWindReleased");
    earbudTest_SendWindEvent(KYMERA_WIND_STAGE1_RELEASED, wind_detect_intensity_none);
}

void EarbudTest_SetStageTwoWindReleased(void)
{
    DEBUG_LOG_ALWAYS("EarbudTest_SetStageTwoWindReleased");
    earbudTest_SendWindEvent(KYMERA_WIND_STAGE2_RELEASED, wind_detect_intensity_none);
}

#endif

#ifdef ENABLE_AUTO_AMBIENT
uint16 EarbudTest_GetAtrReleaseDuration(void)
{
    DEBUG_LOG_ALWAYS("EarbudTest_GetAtrReleaseDuration");
    return (AncAutoAmbient_GetReleaseTimeConfig());
}

void EarbudTest_SetAtrReleaseDuration(uint16 release_time)
{
    DEBUG_LOG_ALWAYS("EarbudTest_SetAtrReleaseDuration");
    AncAutoAmbient_StoreReleaseConfig(release_time);
    AncAutoAmbient_UpdateReleaseTime();
}
#endif

void EarbudTest_AncToggleDiagnostic(void)
{
    if (appPhyStateIsOutOfCase())
    {
        DEBUG_LOG_ALWAYS("EarbudTest_AncToggleDiagnostic");
        Ui_InjectUiInput(ui_input_anc_toggle_diagnostic);
    }
}

uint8 EarbudTest_AncReadFineGain(audio_anc_path_id gain_path)
{
    uint8 gain;
    
    AncReadFineGainFromInstance(AUDIO_ANC_INSTANCE_0, gain_path, &gain);
    
    DEBUG_LOG_ALWAYS("EarbudTest_AncReadFineGain %d", gain);
    return gain;
}

bool EarbudTest_AncWriteFineGain(audio_anc_path_id gain_path, uint8 gain)
{
    bool status = FALSE;

    if(appPhyStateIsOutOfCase())
    {
        DEBUG_LOG_ALWAYS("EarbudTest_AncWriteFineGain");
        status = AncWriteFineGain(gain_path, gain);
    }
    return status;
}

/*! \brief Write ANC delta gains into ANC_DELTA_USR_PSKEY(USR13 PSKEY) for FFA, FFB and FB paths in one shot.
    \note Delta gains remains same to both ANC instances (ANC0 and ANC1), all 10 ANC modes.
	
    \param ffa_delta ANC FFA path delta gain (in dB scale S6.9 representation; i.e., (int16) dB * 512)
    \param ffb_delta ANC FFB path delta gain (in dB scale S6.9 representation; i.e., (int16) dB * 512)
    \param fb_delta  ANC FB/EC path delta gain (in dB scale S6.9 representation; i.e., (int16) dB * 512)

    \return Returns delta gain write status
*/
bool EarbudTest_WriteAncDeltaGains(int16 ffa_delta, int16 ffb_delta, int16 fb_delta)
{
#define FINE_DELTA_GAIN_ENTRIES     (3U)
#define ANC_DELTA_USR_PSKEY         (13U)
    uint16 fine_delta_gains[FINE_DELTA_GAIN_ENTRIES] = {0};

    fine_delta_gains[0] = (uint16)ffa_delta;
    fine_delta_gains[1] = (uint16)ffb_delta;
    fine_delta_gains[2] = (uint16)fb_delta;

    return (PsStore(ANC_DELTA_USR_PSKEY, fine_delta_gains, FINE_DELTA_GAIN_ENTRIES) == FINE_DELTA_GAIN_ENTRIES);
}

/*! \brief Write ANC delta gain into ANC_DELTA_USR_PSKEY(USR13 PSKEY) for specific ANC path.

    \param path Options AUDIO_ANC_PATH_ID_FFA(1) or AUDIO_ANC_PATH_ID_FFB(2) or AUDIO_ANC_PATH_ID_FB(3)
    \param delta  Delta gain to write (in dB; S6.9 representation)

    \return Returns delta gain write status
*/
bool EarbudTest_WriteAncDeltaGain(audio_anc_path_id path, int16 delta)
{
#define FINE_DELTA_GAIN_ENTRIES     (3U)
#define ANC_DELTA_USR_PSKEY         (13U)
    uint16 fine_delta_gains[FINE_DELTA_GAIN_ENTRIES] = {0};

    /* Read current delta gain from persistant */
    PsRetrieve(ANC_DELTA_USR_PSKEY, fine_delta_gains, FINE_DELTA_GAIN_ENTRIES);

    switch(path)
    {
        case AUDIO_ANC_PATH_ID_FFA:
            fine_delta_gains [0] = (uint16)delta;
            break;

        case AUDIO_ANC_PATH_ID_FFB:
            fine_delta_gains [1] = (uint16)delta;
            break;

        case AUDIO_ANC_PATH_ID_FB:
            fine_delta_gains [2] = (uint16)delta;
            break;

        default:
            break;
    }

    return (PsStore(ANC_DELTA_USR_PSKEY, fine_delta_gains, FINE_DELTA_GAIN_ENTRIES) == FINE_DELTA_GAIN_ENTRIES);
}

/*! \brief Read delta gain value from ANC_DELTA_USR_PSKEY(USR13 PSKEY) for given ANC path.
    \param path Options are AUDIO_ANC_PATH_ID_FFA(1) or AUDIO_ANC_PATH_ID_FFB(2) or AUDIO_ANC_PATH_ID_FB(3)

    \return Returns delta gain for given path
*/
int16 EarbudTest_ReadAncDeltaGain(audio_anc_path_id path)
{
#define FINE_DELTA_GAIN_ENTRIES (3U)
#define ANC_DELTA_USR_PSKEY         (13U)
    uint16 fine_delta_gains[FINE_DELTA_GAIN_ENTRIES] = {0};

    /* check if key having required entries */
    if(PsRetrieve(ANC_DELTA_USR_PSKEY, fine_delta_gains, FINE_DELTA_GAIN_ENTRIES) == FINE_DELTA_GAIN_ENTRIES)
    {
        DEBUG_LOG_ALWAYS("Valid PSKEY");
        switch(path)
        {
            case AUDIO_ANC_PATH_ID_FFA:
                return (int16)fine_delta_gains[0];
            case AUDIO_ANC_PATH_ID_FFB:
                return (int16)fine_delta_gains[1];
            case AUDIO_ANC_PATH_ID_FB:
                return (int16)fine_delta_gains[2];
            default:
                return 0;
        }
    }
    else
    {
        DEBUG_LOG_ALWAYS("Invalid PSKEY");
        return 0;
    }
}

void EarbudTest_SetAncMode(anc_mode_t mode)
{
    if (appPhyStateIsOutOfCase())
    {
        DEBUG_LOG_ALWAYS("EarbudTest_SetAncMode");
        switch(mode)
        {
            case anc_mode_1:
                Ui_InjectUiInput(ui_input_anc_set_mode_1);
                break;
            case anc_mode_2:
                Ui_InjectUiInput(ui_input_anc_set_mode_2);
                break;
            case anc_mode_3:
                Ui_InjectUiInput(ui_input_anc_set_mode_3);
                break;
            case anc_mode_4:
                Ui_InjectUiInput(ui_input_anc_set_mode_4);
                break;
            case anc_mode_5:
                Ui_InjectUiInput(ui_input_anc_set_mode_5);
                break;
            case anc_mode_6:
                Ui_InjectUiInput(ui_input_anc_set_mode_6);
                break;
            case anc_mode_7:
                Ui_InjectUiInput(ui_input_anc_set_mode_7);
                break;
            case anc_mode_8:
                Ui_InjectUiInput(ui_input_anc_set_mode_8);
                break;
            case anc_mode_9:
                Ui_InjectUiInput(ui_input_anc_set_mode_9);
                break;
            case anc_mode_10:
                Ui_InjectUiInput(ui_input_anc_set_mode_10);
                break;
            default:
                Ui_InjectUiInput(ui_input_anc_set_mode_1);
                break;
        }
    }
}

void EarbudTest_SetNextAncMode(void)
{
    if (appPhyStateIsOutOfCase())
    {
        DEBUG_LOG("EarbudTest_SetNextAncMode");
        Ui_InjectUiInput(ui_input_anc_set_next_mode);
    }
}

void EarbudTest_SetAncLeakthroughGain(uint8 gain)
{
    if(appPhyStateIsOutOfCase())
    {
        DEBUG_LOG("EarbudTest_SetAncLeakthroughGain %d", gain);
        AncStateManager_StoreAncLeakthroughGain(gain);
        Ui_InjectRedirectableUiInput(ui_input_anc_set_leakthrough_gain, FALSE);
    }
}

bool EarbudTest_SetAncWorldVolumeGain(int8 gain)
{
    if(appPhyStateIsOutOfCase())
    {
        DEBUG_LOG_INFO("EarbudTest_SetAncWorldVolumeGain %d", gain);
        if(AncStateManager_StoreWorldVolumeGain(gain))
        {
            Ui_InjectRedirectableUiInput(ui_input_anc_set_world_volume_gain, FALSE);
            return TRUE;
        }
    }
    return FALSE;
}

void EarbudTest_SetAncWorldVolumeGainLeftBalance(uint8 balance_percentage)
{
    anc_sm_world_volume_gain_balance_info_t balance_info;

    if(appPhyStateIsOutOfCase())
    {
        DEBUG_LOG_INFO("EarbudTest_SetAncWorldVolumeGainLeftBalance %d", balance_percentage);
        balance_info.balance_device_side = FALSE;
        balance_info.balance_percentage = balance_percentage;
        AncStateManager_StoreWorldVolumeBalanceInfo(balance_info);
        Ui_InjectRedirectableUiInput(ui_input_anc_set_world_volume_balance, FALSE);
    }
}

void EarbudTest_SetAncWorldVolumeGainRightBalance(uint8 balance_percentage)
{
    anc_sm_world_volume_gain_balance_info_t balance_info;

    if(appPhyStateIsOutOfCase())
    {
        DEBUG_LOG_INFO("EarbudTest_SetAncWorldVolumeGainLeftBalance %d", balance_percentage);
        balance_info.balance_device_side = TRUE;
        balance_info.balance_percentage = balance_percentage;
        AncStateManager_StoreWorldVolumeBalanceInfo(balance_info);
        Ui_InjectRedirectableUiInput(ui_input_anc_set_world_volume_balance, FALSE);
    }
}

void EarbudTest_SetAncToggleConfig1(uint8 config)
{
    DEBUG_LOG("EarbudTest_SetAncToggleConfig1");
    AncStateManager_SetAncToggleConfiguration(anc_toggle_way_config_id_1, config);
}

void EarbudTest_SetAncToggleConfig2(uint8 config)
{
    DEBUG_LOG("EarbudTest_SetAncToggleConfig2");
    AncStateManager_SetAncToggleConfiguration(anc_toggle_way_config_id_2, config);
}

void EarbudTest_SetAncToggleConfig3(uint8 config)
{
    DEBUG_LOG("EarbudTest_SetAncToggleConfig3");
    AncStateManager_SetAncToggleConfiguration(anc_toggle_way_config_id_3, config);
}

void EarbudTest_SetAncConfigDuringStandalone(uint8 config)
{
    DEBUG_LOG("EarbudTest_SetAncConfigDuringStandalone");
    AncStateManager_SetAncScenarioConfiguration(anc_scenario_config_id_standalone, config);
}

void EarbudTest_SetAncConfigDuringPlayback(uint8 config)
{
    DEBUG_LOG("EarbudTest_SetAncConfigDuringPlayback");
    AncStateManager_SetAncScenarioConfiguration(anc_scenario_config_id_playback, config);
}

void EarbudTest_SetAncConfigDuringSco(uint8 config)
{
    DEBUG_LOG("EarbudTest_SetAncConfigDuringSco");
    AncStateManager_SetAncScenarioConfiguration(anc_scenario_config_id_sco, config);
}

void EarbudTest_SetAncConfigDuringVa(uint8 config)
{
    DEBUG_LOG("EarbudTest_SetAncConfigDuringVa");
    AncStateManager_SetAncScenarioConfiguration(anc_scenario_config_id_va, config);
}

#ifdef INCLUDE_LE_STEREO_RECORDING 
void EarbudTest_SetAncConfigDuringLeStereoRecording(uint8 config)
{
    DEBUG_LOG("EarbudTest_SetAncConfigDuringLeStereoRecording");
    AncStateManager_SetAncScenarioConfiguration(anc_scenario_config_id_stereo_recording_le, config);
}
#endif /* INCLUDE_LE_STEREO_RECORDING */

void EarbudTest_SetAncDemoState(bool demo_state)
{
    DEBUG_LOG("EarbudTest_SetAncDemoState");
    AncStateManager_SetDemoState(demo_state);
}

void EarbudTest_ToggleAncAdaptivity(void)
{
    if (appPhyStateIsOutOfCase())
    {
        DEBUG_LOG("EarbudTest_ToggleAncAdaptivity");
        Ui_InjectUiInput(ui_input_anc_adaptivity_toggle_on_off);
    }
}

void EarbudTest_ToggleAncConfig(void)
{
    if (appPhyStateIsOutOfCase())
    {
        DEBUG_LOG("EarbudTest_ToggleAncConfig");
        Ui_InjectUiInput(ui_input_anc_toggle_way);
    }
}

bool EarbudTest_GetAncstate(void)
{
     DEBUG_LOG_ALWAYS("EarbudTest_GetAncstate");
     return AncStateManager_IsEnabled();

}

anc_mode_t EarbudTest_GetAncMode(void)
{
     DEBUG_LOG_ALWAYS("EarbudTest_GetAncMode");
     return (AncStateManager_GetMode());
}

uint8 EarbudTest_GetAncLeakthroughGain(void)
{
    DEBUG_LOG("EarbudTest_GetAncLeakthroughGain");
    return AncStateManager_GetAncGain();
}

uint8 EarbudTest_GetAncToggleConfig1(void)
{
     DEBUG_LOG("EarbudTest_GetAncToggleConfig1");
     return AncStateManager_GetAncToggleConfiguration(anc_toggle_way_config_id_1);
}

uint8 EarbudTest_GetAncToggleConfig2(void)
{
     DEBUG_LOG("EarbudTest_GetAncToggleConfig2");
     return AncStateManager_GetAncToggleConfiguration(anc_toggle_way_config_id_2);
}

uint8 EarbudTest_GetAncToggleConfig3(void)
{
     DEBUG_LOG("EarbudTest_GetAncToggleConfig3");
     return AncStateManager_GetAncToggleConfiguration(anc_toggle_way_config_id_3);
}

uint8 EarbudTest_GetAncConfigDuringStandalone(void)
{
     DEBUG_LOG("EarbudTest_GetAncConfigDuringStandalone");
     return AncStateManager_GetAncScenarioConfiguration(anc_scenario_config_id_standalone);
}

uint8 EarbudTest_GetAncConfigDuringPlayback(void)
{
     DEBUG_LOG("EarbudTest_GetAncConfigDuringPlayback");
     return AncStateManager_GetAncScenarioConfiguration(anc_scenario_config_id_playback);
}

uint8 EarbudTest_GetAncConfigDuringSco(void)
{
     DEBUG_LOG("EarbudTest_GetAncConfigDuringSco");
     return AncStateManager_GetAncScenarioConfiguration(anc_scenario_config_id_sco);
}

#ifdef INCLUDE_LE_STEREO_RECORDING 
uint8 EarbudTest_GetAncConfigDuringLeStereoRecording(void)
{
     DEBUG_LOG("EarbudTest_GetAncConfigDuringLeStereoRecording");
     return AncStateManager_GetAncScenarioConfiguration(anc_scenario_config_id_stereo_recording_le);
}
#endif

uint8 EarbudTest_GetAncConfigDuringVa(void)
{
     DEBUG_LOG("EarbudTest_GetAncConfigDuringVa");
     return AncStateManager_GetAncScenarioConfiguration(anc_scenario_config_id_va);
}

bool EarbudTest_GetAncDemoState(void)
{
     DEBUG_LOG("EarbudTest_GetAncDemoState");
     return AncStateManager_IsDemoStateActive();
}

bool EarbudTest_GetAncAdaptivity(void)
{
     DEBUG_LOG("EarbudTest_GetAncAdaptivity");
     return AncStateManager_GetAdaptiveAncAdaptivity();
}

int8 EarbudTest_GetAncWorldVolumeGain(void)
{
    DEBUG_LOG("EarbudTest_GetAncWorldVolumeGain");
    int8 cur_world_volume_gain_dB = 0;

    if(AncStateManager_GetCurrentWorldVolumeGain(&cur_world_volume_gain_dB))
    {
        DEBUG_LOG_INFO("EarbudTest_GetAncWorldVolumeGain, gain: %d", cur_world_volume_gain_dB);
    }
    return cur_world_volume_gain_dB;
}

int8 EarbudTest_GetAncWorldVolumeBalance(void)
{
    DEBUG_LOG("EarbudTest_GetAncWorldVolumeBalance");
    return AncStateManager_GetBalancePercentage();
}

void EarbudTest_GetAncWorldVolumeConfig(void)
{
    DEBUG_LOG("EarbudTest_GetAncWorldVolumeConfig");

    anc_sm_world_volume_gain_mode_config_t world_volume_gain_mode_config = {0, 0};
    AncStateManager_GetCurrentWorldVolumeConfig(&world_volume_gain_mode_config);

    DEBUG_LOG_ALWAYS("EarbudTest_GetAncWorldVolumeConfig, max_gain_dB %d", world_volume_gain_mode_config.max_gain_dB);
    DEBUG_LOG_ALWAYS("EarbudTest_GetAncWorldVolumeConfig, min_gain_dB %d", world_volume_gain_mode_config.min_gain_dB);
}

void EarbudTest_StartAncTuning(void)
{
    /* Check if Static ANC tuning is already running */
    if(AncStateManager_IsTuningModeActive())
    {
        DEBUG_LOG_INFO("Static ANC tuning is already in progress");
    }
    else
    {
        /* Check if Adaptive ANC tuning is running. If yes, Stop it before starting Static ANC tuning */
        if(AncStateManager_IsAdaptiveAncTuningModeActive())
        {
            DEBUG_LOG_INFO("Destroying Adaptive ANC tuning chain as Adaptive ANC tuning is in progress");
            AncStateManager_ExitAdaptiveAncTuningMode();
        }

        /* Start ANC tuing only if appConfigAncTuningEnabled() is set to TRUE and device is incase */
        if(appConfigAncTuningEnabled() && appPhyStateGetState() == PHY_STATE_IN_CASE)
        {
            DEBUG_LOG_DEBUG("EarbudTest_StartAncTuning");
            Ui_InjectRedirectableUiInput(ui_input_anc_enter_tuning_mode, FALSE);
        }
    }
}

void EarbudTest_StopAncTuning(void)
{
    DEBUG_LOG_FN_ENTRY("EarbudTest_StopAncTuning");
    Ui_InjectRedirectableUiInput(ui_input_anc_exit_tuning_mode, FALSE);
}

void EarbudTest_StartAdaptiveAncTuning(void)
{
    /* Check if Adaptive ANC tuning is already running */
    if(AncStateManager_IsAdaptiveAncTuningModeActive())
    {
        DEBUG_LOG_INFO("Adaptive ANC tuning is already in progress");
    }
    else
    {
        /* Check if Static ANC tuning is running. If yes, Stop it before starting Adaptive ANC tuning */
        if(AncStateManager_IsTuningModeActive())
        {
            DEBUG_LOG_INFO("Destroying Static ANC tuning chain as Static ANC tuning is in progress");
            AncStateManager_ExitAncTuningMode();
        }

        /* Start AANC tuing only if appConfigAncTuningEnabled() is set to TRUE and device is incase */
        if(appConfigAncTuningEnabled() && appPhyStateGetState() == PHY_STATE_IN_CASE)
        {
            DEBUG_LOG_DEBUG("EarbudTest_StartAdaptiveAncTuning");
            Ui_InjectRedirectableUiInput(ui_input_anc_enter_adaptive_anc_tuning_mode, FALSE);
        }
    }
}

void EarbudTest_StopAdaptiveAncTuning(void)
{
    DEBUG_LOG_FN_ENTRY("EarbudTest_StopAdaptiveAncTuning");
    Ui_InjectRedirectableUiInput(ui_input_anc_exit_adaptive_anc_tuning_mode, FALSE);
}

bool EarbudTest_OverrideA2dpLatency(uint32 latency_ms)
{
    return Kymera_LatencyManagerSetOverrideLatency(latency_ms);
}

/* void EarbudTest_GAIACommandGetAncState(void)
{
     DEBUG_LOG_ALWAYS("EarbudTest_GAIACommandGetAncState");
     earbudTest_SendGaiaCommandToAnc(anc_gaia_get_anc_state, 1, 0);
}

void EarbudTest_GAIACommandSetAncEnable(void)
{
     DEBUG_LOG_ALWAYS("EarbudTest_GAIACommandSetAncEnable");
     earbudTest_SendGaiaCommandToAnc(anc_gaia_set_anc_state, ANC_GAIA_SET_ANC_STATE_PAYLOAD_LENGTH, ANC_GAIA_SET_ANC_STATE_ENABLE);
}

void EarbudTest_GAIACommandSetAncDisable(void)
{
     DEBUG_LOG_ALWAYS("EarbudTest_GAIACommandSetAncDisable");
     earbudTest_SendGaiaCommandToAnc(anc_gaia_set_anc_state, ANC_GAIA_SET_ANC_STATE_PAYLOAD_LENGTH, ANC_GAIA_SET_ANC_STATE_DISABLE);
}

void EarbudTest_GAIACommandGetAncNumOfModes(void)
{
     DEBUG_LOG_ALWAYS("EarbudTest_GAIACommandGetAncNumOfModes");
     earbudTest_SendGaiaCommandToAnc(anc_gaia_get_num_anc_modes, 1, 0);
}

void EarbudTest_GAIACommandSetAncMode(uint8 mode)
{
     DEBUG_LOG_ALWAYS("EarbudTest_GAIACommandSetAncMode");
     earbudTest_SendGaiaCommandToAnc(anc_gaia_set_anc_mode, ANC_GAIA_SET_ANC_MODE_PAYLOAD_LENGTH, mode);
}

void EarbudTest_GAIACommandGetAncLeakthroughGain(void)
{
     DEBUG_LOG_ALWAYS("EarbudTest_GAIACommandGetAncLeakthroughGain");
     earbudTest_SendGaiaCommandToAnc(anc_gaia_get_configured_leakthrough_gain, 1, 0);
}

void EarbudTest_GAIACommandSetAncLeakthroughGain(uint8 gain)
{
     DEBUG_LOG_ALWAYS("EarbudTest_GAIACommandSetAncLeakthroughGain");
     earbudTest_SendGaiaCommandToAnc(anc_gaia_set_configured_leakthrough_gain, ANC_GAIA_SET_ANC_LEAKTHROUGH_GAIN_PAYLOAD_LENGTH, gain);
} */

void EarbudTest_EnableLeakthrough(void)
{
    if(appPhyStateIsOutOfCase())
    {
        DEBUG_LOG("EarbudTest_EnableLeakthrough");
        Ui_InjectUiInput(ui_input_leakthrough_on);
    }
}

void EarbudTest_DisableLeakthrough(void)
{
    if(appPhyStateIsOutOfCase())
    {
        DEBUG_LOG("EarbudTest_DisableLeakthrough");
        Ui_InjectUiInput(ui_input_leakthrough_off);
    }
}

void EarbudTest_ToggleLeakthroughOnOff(void)
{
    if(appPhyStateIsOutOfCase())
    {
        DEBUG_LOG_ALWAYS("EarbudTest_LeakthroughToggleOnOff");
        Ui_InjectUiInput(ui_input_leakthrough_toggle_on_off);
    }
}

void EarbudTest_SetLeakthroughMode(leakthrough_mode_t leakthrough_mode)
{
    if(appPhyStateIsOutOfCase())
    {
        DEBUG_LOG("EarbudTest_SetLeakthroughMode");
        switch(leakthrough_mode)
        {
            case LEAKTHROUGH_MODE_1:
                Ui_InjectUiInput(ui_input_leakthrough_set_mode_1);
                break;
            case LEAKTHROUGH_MODE_2:
                Ui_InjectUiInput(ui_input_leakthrough_set_mode_2);
                break;
            case LEAKTHROUGH_MODE_3:
                Ui_InjectUiInput(ui_input_leakthrough_set_mode_3);
                break;
            default:
                DEBUG_LOG("Invalid value of leakthrough mode is passed");
                break;
        }
    }
}

void EarbudTest_SetNextLeakthroughMode(void)
{
    if(appPhyStateIsOutOfCase())
    {
        Ui_InjectUiInput(ui_input_leakthrough_set_next_mode);
    }
}

leakthrough_mode_t EarbudTest_GetLeakthroughMode(void)
{
    DEBUG_LOG("EarbudTest_GetLeakthroughMode");
    return AecLeakthrough_GetMode();
}

bool EarbudTest_IsLeakthroughEnabled(void)
{
    DEBUG_LOG("EarbudTest_IsLeakthroughEnabled");
    return AecLeakthrough_IsLeakthroughEnabled();
}

void EarbudTest_EnableAutoTransparency(void)
{
    if(appTestPhyStateIsInEar())
    {
        AncAutoAmbient_Enable(TRUE);
    }
}

void EarbudTest_DisableAutoTransparency(void)
{
    if(appTestPhyStateIsInEar())
    {
        AncAutoAmbient_Enable(FALSE);
    }
}

bool EarbudTest_IsAutoTransparencyEnabled(void)
{
    return AncAutoAmbient_IsEnabled();
}

#ifdef ENABLE_ADAPTIVE_ANC
void EarbudTest_EnableAah(void)
{
    if(appPhyStateIsOutOfCase())
    {
        Ui_InjectUiInput(ui_input_anc_adverse_acoustic_handler_enable);
    }
}

void EarbudTest_DisableAah(void)
{
    if(appPhyStateIsOutOfCase())
    {
        Ui_InjectUiInput(ui_input_anc_adverse_acoustic_handler_disable);
    }
}

void EarbudTest_EnableHcgr(void)
{
    if(appPhyStateIsOutOfCase())
    {
        Ui_InjectUiInput(ui_input_anc_anti_howling_enable);
    }
}

void EarbudTest_DisableHcgr(void)
{
    if(appPhyStateIsOutOfCase())
    {
        Ui_InjectUiInput(ui_input_anc_anti_howling_disable);
    }
}

void EarbudTest_DisableAllAncFeatures(void)
{
    EarbudTest_DisableHcgr();
    EarbudTest_DisableAah();
#ifdef ENABLE_WIND_DETECT
    EarbudTest_DisableWindDetect();
#endif
    if(EarbudTest_GetAncAdaptivity())
    {
        EarbudTest_ToggleAncAdaptivity();
    }
}

void EarbudTest_EnableNoiseId(void)
{
    Ui_InjectRedirectableUiInput(ui_input_anc_noise_id_enable, FALSE);
}

void EarbudTest_DisableNoiseId(void)
{
    Ui_InjectRedirectableUiInput(ui_input_anc_noise_id_disable, FALSE);
}

bool EarbudTest_IsNoiseIdEnabled(void)
{
    return AncNoiseId_IsFeatureEnabled();
}

#ifdef ENABLE_ANC_DUAL_FILTER_TOPOLOGY
void EarbudTest_SetAncFilterTopologyParallel(void)
{
    if(appPhyStateIsOutOfCase())
    {
        DEBUG_LOG("EarbudTest_SetAncFilterTopologyParallel");
        Ui_InjectUiInput(ui_input_anc_set_filter_topology_parallel);
    }
}

void EarbudTest_SetAncFilterTopologyDual(void)
{
    if(appPhyStateIsOutOfCase())
    {
        DEBUG_LOG("EarbudTest_SetAncFilterTopologyDual");
        Ui_InjectUiInput(ui_input_anc_set_filter_topology_dual);
    }
}
#endif

#endif  //ENABLE_ADAPTIVE_ANC

void appTestSetFixedRole(peer_find_role_fixed_role_t role)
{
    DEBUG_LOG_ALWAYS("appTestSetFixedRole role=%d", role);
    PeerFindRole_SetFixedRole(role);
}

#ifdef INCLUDE_FAST_PAIR
/* Refer comments given in the function prototype to configure fast pair model ID.
*/
void appTestSetFPModelID(uint32 fast_pair_model_id)
{
    UNUSED(fast_pair_model_id);
    DEBUG_LOG("appTestSetFPModelID no longer supported, please setup ApplicationReadOnlyKey0 MIB in build config");
}

/* Refer comments given in the function prototype to configure scrambled aspk.
*/
void appTestSetFPASPK(uint32 dword1, uint32 dword2, uint32 dword3, uint32 dword4,
                          uint32 dword5, uint32 dword6, uint32 dword7, uint32 dword8)
{
    UNUSED(dword1);
    UNUSED(dword2);
    UNUSED(dword3);
    UNUSED(dword4);
    UNUSED(dword5);
    UNUSED(dword6);
    UNUSED(dword7);
    UNUSED(dword8);
    DEBUG_LOG("appTestSetFPASPK no longer supported, please setup ApplicationReadOnlyKey1 MIB in build config");
}
#endif /* INCLUDE_FAST_PAIR */

appKymeraScoMode appTestGetHfpCodec(void)
{
    const appKymeraScoChainInfo *sco_info = KymeraGetTaskData()->sco_info;
    appKymeraScoMode sco_mode = NO_SCO;
    if (sco_info)
    {
        sco_mode = sco_info->mode;
    }
    DEBUG_LOG("appTestGetHfpCodec enum:appKymeraScoMode:%d", sco_mode);
    return sco_mode;
}

bool appTestAnyBredrConnection(void)
{
    bool bredr_connection = ConManagerAnyTpLinkConnected(cm_transport_bredr);

    DEBUG_LOG_ALWAYS("appTestAnyBredrConnection: %d", bredr_connection);

    return bredr_connection;
}

bool appTestAnyLeConnection(void)
{
    bool le_connection = ConManagerAnyTpLinkConnected(cm_transport_ble);
    DEBUG_LOG_ALWAYS("appTestAnyLeConnection: %d", le_connection);
    return le_connection;
}

int appTestGetCurrentAudioVolume(void)
{
    int volume = 0;

    audio_source_t focused_source = audio_source_none;
    if (Focus_GetAudioSourceForContext(&focused_source))
    {
        volume = AudioSources_GetVolume(focused_source).value;
    }
    if (focused_source == audio_source_none && MirrorProfile_IsA2dpActive())
    {
        // If there is no focused source, but A2DP mirroring is active, then
        // use a2dp1 audio source (which is hard-coded for mirroring scenario).
        focused_source = audio_source_a2dp_1;
        volume = AudioSources_GetVolume(focused_source).value;
    }

    DEBUG_LOG_ALWAYS("appTestGetCurrentAudioVolume enum:audio_source_t:%d volume %d",
                     focused_source, volume);

    return volume;
}

int appTestGetCurrentVoiceVolume(void)
{
    int volume = 0;

    voice_source_t focused_source = voice_source_none;
    if (Focus_GetVoiceSourceForContext(ui_provider_telephony, &focused_source))
    {
        volume = VoiceSources_GetVolume(focused_source).value;
    }
    if (focused_source == voice_source_none && MirrorProfile_GetScoSink())
    {
        // If there is no focused source, but mirroring is active with SCO, then
        // use hfp1 voice source (which is hard-coded for mirroring scenario).
        focused_source = voice_source_hfp_1;
        volume = VoiceSources_GetVolume(focused_source).value;
    }

    DEBUG_LOG_ALWAYS("appTestGetCurrentVoiceVolume enum:voice_source_t:%d volume %d",
                     focused_source, volume);

    return volume;
}

#if defined(ENABLE_ANC_NOISE_ID) && defined(ENABLE_ADAPTIVE_ANC)
void EarbudTest_SetCurrentNoiseID(uint8 mode)
{
    if(mode)
    {
        KymeraAncCommon_SetNoiseID(noise_id_category_b);
    }
    else
    {
        KymeraAncCommon_SetNoiseID(noise_id_category_a);
    }
}

uint8 EarbudTest_GetCurrentNoiseID(void)
{
    uint8 nid = (uint8) KymeraAncCommon_GetNoiseID();
    return nid;
}

void EarbudTest_GetNoiseIDStatistics(void)
{
    uint32* stats = KymeraNoiseID_GetStatusData()->value;
    for(int i = 0; i < 5; ++i)
    {
        noise_id_stats[i] = stats[i];
    }
    free(stats);
}
#endif

#ifdef ENABLE_ADAPTIVE_ANC
uint8 appTestGetAdaptiveAncFeedForwardGain(void)
{
    return EarbudTest_GetAdaptiveAncFfbPathFineGain();
}

uint8 EarbudTest_GetAdaptiveAncFBGain(void)
{
    return EarbudTest_GetAdaptiveAncFfaPathFineGain();
}

uint8 EarbudTest_GetAdaptiveAncFfaPathFineGain(void)
{
    return EarbudTest_GetAdaptiveAncFfaPathFineGainForInstance(AUDIO_ANC_INSTANCE_0);
}

uint8 EarbudTest_GetAdaptiveAncFfbPathFineGain(void)
{
    return EarbudTest_GetAdaptiveAncFfbPathFineGainForInstance(AUDIO_ANC_INSTANCE_0);
}

uint8 EarbudTest_GetAdaptiveAncFbPathFineGain(void)
{
    return EarbudTest_GetAdaptiveAncFbPathFineGainForInstance(AUDIO_ANC_INSTANCE_0);
}

int8 EarbudTest_GetAdaptiveAncFfaPathCoarseGain(void)
{
    return EarbudTest_GetAdaptiveAncFfaPathCoarseGainForInstance(AUDIO_ANC_INSTANCE_0);
}

int8 EarbudTest_GetAdaptiveAncFfbPathCoarseGain(void)
{
    return EarbudTest_GetAdaptiveAncFfbPathCoarseGainForInstance(AUDIO_ANC_INSTANCE_0);
}

int8 EarbudTest_GetAdaptiveAncFbPathCoarseGain(void)
{
    return EarbudTest_GetAdaptiveAncFbPathCoarseGainForInstance(AUDIO_ANC_INSTANCE_0);
}

uint8 EarbudTest_GetAdaptiveAncFfaPathFineGainForInstance(audio_anc_instance anc_inst)
{
    if (anc_inst != AUDIO_ANC_INSTANCE_NONE)
    {
        uint8 gain_inst0 = 0, gain_inst1 = 0;
        KymeraAncCommon_GetFineGain(&gain_inst0, &gain_inst1, AUDIO_ANC_PATH_ID_FFA);
        DEBUG_LOG("EarbudTest_GetAdaptiveAncFfaPathFineGainForInstance, instance: enum:audio_anc_instance:%d, inst0 gain: %d, inst1 gain : %d", anc_inst, gain_inst0, gain_inst1);
        return (anc_inst == AUDIO_ANC_INSTANCE_0) ? gain_inst0 : gain_inst1;
    }
    else
    {
        DEBUG_LOG_ERROR("EarbudTest_GetAdaptiveAncFfaPathFineGainForInstance, invalid ANC instance: %d", anc_inst);
        return 0;
    }
}

uint8 EarbudTest_GetAdaptiveAncFfbPathFineGainForInstance(audio_anc_instance anc_inst)
{
    if (anc_inst != AUDIO_ANC_INSTANCE_NONE)
    {
        uint8 gain_inst0 = 0, gain_inst1 = 0;
        KymeraAncCommon_GetFineGain(&gain_inst0, &gain_inst1, AUDIO_ANC_PATH_ID_FFB);
        DEBUG_LOG("EarbudTest_GetAdaptiveAncFfbPathFineGainForInstance, instance: enum:audio_anc_instance:%d, inst0 gain: %d, inst1 gain : %d", anc_inst, gain_inst0, gain_inst1);
        return (anc_inst == AUDIO_ANC_INSTANCE_0) ? gain_inst0 : gain_inst1;
    }
    else
    {
        DEBUG_LOG_ERROR("EarbudTest_GetAdaptiveAncFfbPathFineGainForInstance, invalid ANC instance: %d", anc_inst);
        return 0;
    }
}

uint8 EarbudTest_GetAdaptiveAncFbPathFineGainForInstance(audio_anc_instance anc_inst)
{
    if (anc_inst != AUDIO_ANC_INSTANCE_NONE)
    {
        uint8 gain_inst0 = 0, gain_inst1 = 0;
        KymeraAncCommon_GetFineGain(&gain_inst0, &gain_inst1, AUDIO_ANC_PATH_ID_FB);
        DEBUG_LOG("EarbudTest_GetAdaptiveAncFbPathFineGainForInstance, instance: enum:audio_anc_instance:%d, inst0 gain: %d, inst1 gain : %d", anc_inst, gain_inst0, gain_inst1);
        return (anc_inst == AUDIO_ANC_INSTANCE_0) ? gain_inst0 : gain_inst1;
    }
    else
    {
        DEBUG_LOG_ERROR("EarbudTest_GetAdaptiveAncFbPathFineGainForInstance, invalid ANC instance: %d", anc_inst);
        return 0;
    }
}

int8 EarbudTest_GetAdaptiveAncFfaPathCoarseGainForInstance(audio_anc_instance anc_inst)
{
    if (anc_inst != AUDIO_ANC_INSTANCE_NONE)
    {
        int8 gain_inst0 = 0, gain_inst1 = 0;
        KymeraAncCommon_GetCoarseGain(&gain_inst0, &gain_inst1, AUDIO_ANC_PATH_ID_FFA);
        DEBUG_LOG("EarbudTest_GetAdaptiveAncFfaPathCoarseGainForInstance, instance: enum:audio_anc_instance:%d, inst0 gain: %d, inst1 gain : %d", anc_inst, gain_inst0, gain_inst1);
        return (anc_inst == AUDIO_ANC_INSTANCE_0) ? gain_inst0 : gain_inst1;
    }
    else
    {
        DEBUG_LOG_ERROR("EarbudTest_GetAdaptiveAncFfaPathCoarseGainForInstance, invalid ANC instance: %d", anc_inst);
        return 0;
    }
}

int8 EarbudTest_GetAdaptiveAncFfbPathCoarseGainForInstance(audio_anc_instance anc_inst)
{
    if (anc_inst != AUDIO_ANC_INSTANCE_NONE)
    {
        int8 gain_inst0 = 0, gain_inst1 = 0;
        KymeraAncCommon_GetCoarseGain(&gain_inst0, &gain_inst1, AUDIO_ANC_PATH_ID_FFB);
        DEBUG_LOG("EarbudTest_GetAdaptiveAncFfbPathCoarseGainForInstance, instance: enum:audio_anc_instance:%d, inst0 gain: %d, inst1 gain : %d", anc_inst, gain_inst0, gain_inst1);
        return (anc_inst == AUDIO_ANC_INSTANCE_0) ? gain_inst0 : gain_inst1;
    }
    else
    {
        DEBUG_LOG_ERROR("EarbudTest_GetAdaptiveAncFfbPathCoarseGainForInstance, invalid ANC instance: %d", anc_inst);
        return 0;
    }
}

int8 EarbudTest_GetAdaptiveAncFbPathCoarseGainForInstance(audio_anc_instance anc_inst)
{
    if (anc_inst != AUDIO_ANC_INSTANCE_NONE)
    {
        int8 gain_inst0 = 0, gain_inst1 = 0;
        KymeraAncCommon_GetCoarseGain(&gain_inst0, &gain_inst1, AUDIO_ANC_PATH_ID_FB);
        DEBUG_LOG("EarbudTest_GetAdaptiveAncFbPathCoarseGainForInstance, instance: enum:audio_anc_instance:%d, inst0 gain: %d, inst1 gain : %d", anc_inst, gain_inst0, gain_inst1);
        return (anc_inst == AUDIO_ANC_INSTANCE_0) ? gain_inst0 : gain_inst1;
    }
    else
    {
        DEBUG_LOG_ERROR("EarbudTest_GetAdaptiveAncFbPathCoarseGainForInstance, invalid ANC instance: %d", anc_inst);
        return 0;
    }
}

adaptive_anc_mode_t appTestGetCurrentAdaptiveAncMode(void)
{
    adaptive_anc_mode_t aanc_mode;
    if(KymeraAncCommon_GetApdativeAncCurrentMode(&aanc_mode))
    {
        DEBUG_LOG("appTestGetCurrentAdaptiveAncMode, aanc_mode = %d", aanc_mode);
        return aanc_mode;
    }
    DEBUG_LOG("appTestGetCurrentAdaptiveAncMode, failed!");
    return 0;
}

adaptive_ancv2_sysmode_t appTestGetCurrentAdaptiveAncV2Mode(void)
{
    adaptive_ancv2_sysmode_t aanc_mode;
    if(KymeraAncCommon_GetApdativeAncV2CurrentMode(&aanc_mode))
    {
        DEBUG_LOG("appTestGetCurrentAdaptiveAncV2Mode, aanc_mode = %d", aanc_mode);
        return aanc_mode;
    }
    DEBUG_LOG("appTestGetCurrentAdaptiveAncV2Mode, failed!");
    return 0;
}

ahm_sysmode_t EarbudTest_GetCurrentAhmMode(void)
{
    ahm_sysmode_t ahm_mode;
    if(KymeraAncCommon_GetAhmMode(&ahm_mode))
    {
        DEBUG_LOG("appTestGetCurrentAhmMode, AHM mode = %d", ahm_mode);
        return ahm_mode;
    }
    DEBUG_LOG("appTestGetCurrentAhmMode, failed");
    return 0;
}
#endif /* ENABLE_ADAPTIVE_ANC */


bool appTestAdaptiveAncIsNoiseLevelBelowQuietModeThreshold(void)
{
    DEBUG_LOG("appTestAdaptiveAncIsNoiseLevelBelowQuietModeThreshold");
    return Kymera_AdaptiveAncIsNoiseLevelBelowQuietModeThreshold();
}

bool appTestAdaptiveAncIsQuietModeDetectedOnLocalDevice(void)
{
    DEBUG_LOG("appTestAdaptiveAncIsQuietModeDetectedOnLocalDevice");
    return AancQuietMode_IsQuietModeDetected();
}

void appTestSetAdaptiveAncGainParameters(uint32 mantissa, uint32 exponent)
{
    DEBUG_LOG("appTestSetAdaptiveAncGainParameters");

    /* Just to keep compiler happy */
    UNUSED(mantissa);
    UNUSED(exponent);

    KymeraAncCommon_AdaptiveAncSetGainValues(mantissa, exponent);
}

bool appTestIsDeviceTestServiceEnabled(void)
{
    bool enabled = DeviceTestService_TestMode();

    DEBUG_LOG_ALWAYS("appTestIsDeviceTestServiceEnabled:%d", enabled);
    return enabled;
}


bool appTestIsDeviceTestServiceActive(void)
{
    bool active = FALSE;

#ifdef INCLUDE_DEVICE_TEST_SERVICE
    active = DeviceTestServiceIsActive();
#endif

    DEBUG_LOG_ALWAYS("appTestIsDeviceTestServiceActive. Active:%d", active);

    return active;
}


bool appTestIsDeviceTestServiceAuthenticated(void)
{
    bool authenticated = FALSE;

#ifdef INCLUDE_DEVICE_TEST_SERVICE
    authenticated = DeviceTestServiceIsAuthenticated();
#endif

    DEBUG_LOG_ALWAYS("appTestIsDeviceTestServiceAuthenticated. Authenticated:%d", authenticated);

    return authenticated;
}


bool appTestDeviceTestServiceEnable(device_test_service_mode_t mode)
{
    bool saved = FALSE;

    DeviceTestService_SaveTestMode(mode);

    /* The SaveTestMode API does not check if the PSKEY write was
       successful. Check now by reading the value back.
     */
    saved = (mode == DeviceTestService_TestModeType());

    DEBUG_LOG_ALWAYS("appTestDeviceTestServiceEnable. Saved:%d", saved);

    return saved;
}

unsigned appTestDeviceTestServiceDevices(device_t **devices)
{
    unsigned num_found = deviceTestService_GetDtsDevices(devices);

    DEBUG_LOG_ALWAYS("appTestDeviceTestServiceDevices. Devices:%u",
                      num_found);

    return num_found;
}


bool appTestIsBredrScanEnabled(void)
{
    bool scan_enabled = !BredrScanManager_IsScanDisabled();

    DEBUG_LOG_ALWAYS("appTestIsBredrScanEnabled Enabled:%d", scan_enabled);

    return scan_enabled;
}

bool appTestIsPreparingToSleep(void)
{
    appState state = appSmGetState();
    return (state & APP_SUBSTATE_SOPORIFIC_TERMINATING) != 0;
}

#ifdef INCLUDE_GAMING_MODE
bool appTestIsGamingModeOn(void)
{
    return GamingMode_IsGamingModeEnabled();
}

bool appTestSetGamingMode(bool new_gaming_state)
{
    bool current_gaming_state = appTestIsGamingModeOn();
    if(current_gaming_state != new_gaming_state)
    {
        Ui_InjectUiInput(ui_input_gaming_mode_toggle);
        return TRUE;
    }

    return FALSE;
}
#endif
#ifdef INCLUDE_LATENCY_MANAGER
bool appTestIsDynamicLatencyAdjustmentEnabled(void)
{
    return Kymera_DynamicLatencyIsEnabled();
}

uint16 appTestGetCurrentA2dpLatency(void)
{
    return Kymera_LatencyManagerGetLatency();
}

void appTestSetDynamicLatencyAdjustment(bool enable)
{
    if (enable)
    {
        Kymera_DynamicLatencyEnableDynamicAdjustment();
    }
    else
    {
        Kymera_DynamicLatencyDisableDynamicAdjustment();
    }
}
#endif

bool appTestIsKymeraIdle(void)
{
    bool idle = Kymera_IsIdle();

    DEBUG_LOG_ALWAYS("appTestIsKymeraIdle. Idle:%d", idle);

    return idle;
}


bool appTestIsPeerQhsConnected(void)
{
    bool qhs_connected_status = FALSE;
    bdaddr bd_addr;

    if (appDeviceGetPeerBdAddr(&bd_addr))
    {
        qhs_connected_status = ConManagerGetQhsConnectStatus(&bd_addr);
    }

    DEBUG_LOG_ALWAYS("appTestIsPeerQhsConnected:%d", qhs_connected_status);

    return qhs_connected_status;
}

bool appTestIsHandsetQhsConnected(void)
{
    bool qhs_connected_status = FALSE;
    bdaddr bd_addr;

    if (appDeviceGetHandsetBdAddr(&bd_addr))
    {
        qhs_connected_status = ConManagerGetQhsConnectStatus(&bd_addr);
    }

    DEBUG_LOG_ALWAYS("appTestIsHandsetQhsConnected:%d", qhs_connected_status);

    return qhs_connected_status;
}

bool appTestIsTimeToSleep(void)
{
    bool time_to_sleep = (appSmGetState() == APP_STATE_OUT_OF_CASE_IDLE);

    DEBUG_LOG_ALWAYS("appTestIsTimeToSleep %d", time_to_sleep);
    return time_to_sleep;
}

void appTestTriggerSleep(void)
{
    DEBUG_LOG_ALWAYS("appTestTriggerSleep");
    MessageCancelFirst(SmGetTask(), SM_INTERNAL_TIMEOUT_IDLE);
    MessageSend(SmGetTask(), SM_INTERNAL_TIMEOUT_IDLE, NULL);
}

void appTestForceAllowSleep(void)
{
    DEBUG_LOG_ALWAYS("appTestForceAllowSleep");
    Charger_ForceAllowPowerOff(TRUE);
}

void appTestDisallowDormant(void)
{
    DEBUG_LOG_ALWAYS("appTestDisallowDormant");
    Charger_DisallowDormant(TRUE);
}

void appTestRegisterForKymeraNotifications(void)
{
    DEBUG_LOG_ALWAYS("appTestRegisterForKymeraNotifications");
    Kymera_RegisterNotificationListener(&testTask);
}

/*! \brief Get A2DP Codec
     This can only be called from primary earbud
     because we don't have av instance for mirroring in secondary earbud
*/
app_test_a2dp_codec_t EarbudTest_GetA2dpCodec(void)
{

    DEBUG_LOG_ALWAYS("earbudTest_getA2dpCodec");
    bdaddr bd_addr;
    if (appDeviceGetHandsetBdAddr(&bd_addr))
    {
        /* Find handset AV instance */
        avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&bd_addr);
        if(theInst)
        {
            switch(theInst->a2dp.current_seid)
            {
                case AV_SEID_SBC_SRC:
                case AV_SEID_SBC_SNK:
                   case AV_SEID_SBC_MONO_TWS_SNK:
                    return a2dp_codec_sbc;

                case AV_SEID_AAC_SNK:
                case AV_SEID_AAC_STEREO_TWS_SNK:
                    return a2dp_codec_aac;

                case AV_SEID_APTX_SNK:
                case AV_SEID_APTX_MONO_TWS_SNK:
                    return a2dp_codec_aptx;

                case AV_SEID_APTX_ADAPTIVE_SNK:
                case AV_SEID_APTX_ADAPTIVE_TWS_SNK:
                    return a2dp_codec_aptx_adaptive;
                default:
                    break;
            }
        }
    }

    return a2dp_codec_unknown;
}

#ifdef GC_SECTIONS
/**
 * @brief appTestShowKeptSymbols
 *
 * A list of ADK and library functions and/or variables that are referenced
 * directly via pydbg test scripts, but which are otherwise unused given the
 * current software configuration.
 *
 * This is an alternative mechanism to scattering #pragma unitcodesection KEEP_PM
 * over the library code.  Doing that would unnecessarily include code in production
 * builds even if such code was not actually used.
 *
 * So here we fake our interest by making pointers to things of interest.
 * Nothing is called or dereferenced, but it's enough to stop the linker
 * garbage collecting otherwise unused resources.
 *
 * It's not necessary to call this function, but it's harmess if you do.
 */
static const void *tableOfSymbolsToKeep[] = {
    (void*)ConnectionReadTxPower,
    (void*)Kymera_RegisterNotificationListener,
    (void*)SinkGetRssi,
};
void appTestShowKeptSymbols(void);
void appTestShowKeptSymbols(void)
{
    for( size_t i = 0 ; i < sizeof(tableOfSymbolsToKeep)/sizeof(tableOfSymbolsToKeep[0]) ; i++ )
    {
        if( tableOfSymbolsToKeep[i] != NULL )
        {
            DEBUG_LOG_ALWAYS("Have %p",tableOfSymbolsToKeep[i]);
        }
    }
}
#endif

#ifdef INCLUDE_TEST_TONES
static rtime_t getTestToneStartTime(void)
{
    return rtime_add(SystemClockGetTimerTime(), 300*US_PER_MS);
}

void appTestToneStartContinuous(void)
{
    appKymeraTonePlay(app_tone_test_continuous, getTestToneStartTime(), TRUE, FALSE, NULL, 0);
}

void appTestToneStartIndexed(void)
{
    appKymeraTonePlay(app_tone_test_indexed, getTestToneStartTime(), TRUE, FALSE, NULL, 0);
}

void appTestToneStartByEvent(uint16 event_id)
{
    int i;
    for (i = AppTonesConfigTable_SingleGetSize() - 1; i >= 0; i -= 1)
    {
        if (app_ui_tones_table[i].sys_event == event_id)
        {
            appKymeraTonePlay(app_ui_tones_table[i].data.tone.tone, getTestToneStartTime(), TRUE, FALSE, NULL, 0);
        }
    }
}

void appTestToneStop(void)
{
    appKymeraTonePromptCancel();
}
#endif

void appTestCaseLidOpenEvent(void)
{
    DEBUG_LOG_ALWAYS("appTestCaseLidOpenEvent");
    CcWithCase_LidEvent(CASE_LID_STATE_OPEN);
}

void appTestCaseLidClosedEvent(void)
{
    DEBUG_LOG_ALWAYS("appTestCaseLidClosedEvent");
    CcWithCase_LidEvent(CASE_LID_STATE_CLOSED);
}

void appTestCasePowerMsgEvent(uint8 case_battery_state,
                              uint8 peer_battery_state, uint8 local_battery_state,
                              bool case_charger_connected)
{
    DEBUG_LOG_ALWAYS("appTestCasePowerMsgEvent 0x%x 0x%x 0x%x %d", case_battery_state, peer_battery_state, local_battery_state, case_charger_connected);
    CcWithCase_PowerEvent(case_battery_state, peer_battery_state, local_battery_state, case_charger_connected);
}

bool appTestCaseEventsSupported(void)
{
    bool case_events_supported = CcWithCase_EventsEnabled();
    DEBUG_LOG_ALWAYS("appTestCaseEventsSupported %d", case_events_supported);
    return case_events_supported;
}

bool appTestTransmitEnable(bool val)
{
    bool successful = VmTransmitEnable(val);

    DEBUG_LOG_ALWAYS("appTestTransmitEnable(%d)  success=%d", val, successful);

    return successful;
}

void appTestDisconnectHandsets(void)
{
    PrimaryRules_SetEvent(RULE_EVENT_DISCONNECT_ALL_HANDSETS_USER);
}

bool earbudTestPeerPairWithAddress(bdaddr *address)
{
    bool result = PeerPairing_PeerPairToAddress(&testTask, address);

    DEBUG_LOG_ALWAYS("earbudTestPeerPairWithAddress %04x%02x%06lx Failed:%d",
                        address->nap, address->uap, address->lap,
                        !result);

    return result;
}

#ifdef USE_SYNERGY
int appTestGetKeys(void)
{
    uint8 index = 0;

    if (CsrBtTdDbCountDevices() == 0)
    {
        DEBUG_LOG_ALWAYS(" No device in Database\n");
        return index;
    }

    for (index = 0; index < appConfigMaxDevicesSupported(); index++)
    {
        CsrBtTypedAddr addr;
        CsrBtTdDbBredrKey bredrKey;
        CsrBtTdDbLeKeys leKeys;

        if ( (CsrBtTdDbGetEntryByIndex(index,
                                     CSR_BT_TD_DB_SOURCE_SC,
                                     CSR_BT_TD_DB_SC_KEY_BREDR_KEY,
                                     sizeof(CsrBtTdDbBredrKey),
                                     &bredrKey,
                                     &addr) == CSR_BT_RESULT_CODE_TD_DB_SUCCESS) ||
             (CsrBtTdDbGetEntryByIndex(index,
                                      CSR_BT_TD_DB_SOURCE_SC,
                                      CSR_BT_TD_DB_SC_KEY_LE_KEYS,
                                      sizeof(CsrBtTdDbLeKeys),
                                      &leKeys,
                                      &addr) == CSR_BT_RESULT_CODE_TD_DB_SUCCESS))
        {
            DEBUG_LOG_ALWAYS("appTestGetKeys: %1X:%04X:%02X:%06X",
                      addr.type,
                      addr.addr.nap,
                      addr.addr.uap,
                      addr.addr.lap);

            /* Displaying BREDR link key in reverse order as required by Sniffer tools */
            DEBUG_LOG_ALWAYS("Link Key: 0x%04X, 0x%04X, 0x%04X, 0x%04X",
                      *(uint16 *)(&bredrKey.linkkey[14]),
                      *(uint16 *)(&bredrKey.linkkey[12]),
                      *(uint16 *)(&bredrKey.linkkey[10]),
                      *(uint16 *)(&bredrKey.linkkey[8]));

            DEBUG_LOG_ALWAYS("0x%04X, 0x%04X, 0x%04X, 0x%04X",
                      *(uint16 *)(&bredrKey.linkkey[6]),
                      *(uint16 *)(&bredrKey.linkkey[4]),
                      *(uint16 *)(&bredrKey.linkkey[2]),
                      *(uint16 *)(&bredrKey.linkkey[0]));

            CsrBtTdDbGetEntryByIndex(index,
                                     CSR_BT_TD_DB_SOURCE_SC,
                                     CSR_BT_TD_DB_SC_KEY_LE_KEYS,
                                     sizeof(CsrBtTdDbLeKeys),
                                     &leKeys,
                                     &addr);

            /* Displaying LE link key in reverse order as required by Sniffer tools */
            DEBUG_LOG_ALWAYS("LTK: 0x%04X, 0x%04X, 0x%04X, 0x%04X",
                      leKeys.encCentral.ltk[7],
                      leKeys.encCentral.ltk[6],
                      leKeys.encCentral.ltk[5],
                      leKeys.encCentral.ltk[4]);

            DEBUG_LOG_ALWAYS("0x%04X, 0x%04X, 0x%04X, 0x%04X",
                      leKeys.encCentral.ltk[3],
                      leKeys.encCentral.ltk[2],
                      leKeys.encCentral.ltk[1],
                      leKeys.encCentral.ltk[0]);
        }
        else
        {
            break;
        }
    }

    return index;
}
#endif

#ifdef INCLUDE_MUSIC_PROCESSING
#include "music_processing.h"

void appTest_MusicProcessingGetAvailablePresets(void)
{
    DEBUG_LOG_ALWAYS("appTest_MusicProcessingGetAvailablePresets");
}

void appTest_MusicProcessingSetPreset(uint8 preset)
{
    DEBUG_LOG_ALWAYS("appTest_MusicProcessingSetPreset Preset:%d", preset);
    MusicProcessing_SetPreset(preset);
}

void appTest_MusicProcessingSetUserEqBand(uint8 band, int16 gain)
{
    int16 band_gain = gain;
    DEBUG_LOG_ALWAYS("appTest_MusicProcessingSetUserEqBands Band:%d Gain: %d", band, gain);
    MusicProcessing_SetUserEqBands(band, band, &band_gain);
}

#endif /* INCLUDE_MUSIC_PROCESSING */

bool earbudTestAddPeerPairing(const bdaddr *primary,
                              const bdaddr *secondary,
                              bool this_is_primary,
                              const cl_root_keys *randomised_keys,
                              PEER_PAIRING_LONG_TERM_KEY_T *bredr,
                              PEER_PAIRING_LONG_TERM_KEY_T *le)
{
    DEBUG_LOG_ALWAYS("earbudTestAddPeerPairing. Adding earbud pairing record. Primary:%d",
                      this_is_primary);

    bool successful = PeerPairing_AddPeerPairing(primary, secondary, this_is_primary,
                                                 randomised_keys,
                                                 bredr, le);

    if (!successful)
    {
        DEBUG_LOG_ALWAYS("earbudTestAddPeerPairing. Adding earbud pairing record. FAILED");
    }

    return successful;
}

#ifdef INCLUDE_CHARGER

void appTestChargerStatus(charger_status chg_status)
{
    Charger_TestModeControl(TRUE);
    Charger_TestChargerStatus(chg_status);
}

void appTestChargerDetected(usb_attached_status attached_status,
                                       uint16 charger_dp_millivolts,
                                       uint16 charger_dm_millivolts,
                                       usb_type_c_advertisement cc_status)
{
    Charger_TestModeControl(TRUE);
    Charger_TestChargerDetected(attached_status,
                                charger_dp_millivolts,
                                charger_dm_millivolts,
                                cc_status);
}

void appTestChargerConnected(bool is_connected)
{
    Charger_TestModeControl(TRUE);
    Charger_TestChargerConnected(is_connected);
}

void appTestChargerRestore(void)
{
    Charger_TestModeControl(FALSE);
}

#endif /* INCLUDE_CHARGER */

uint32 appTestUpgradeGetVersion(void)
{
    uint32 result = 0;
    uint16 major, minor, config;
    if (UpgradeGetVersion(&major, &minor, &config))
    {
        DEBUG_LOG_ALWAYS("appTestUpgradeGetVersion: %d.%d.%d", major, minor, config);
        result = (major << 24);
        result |= (minor << 8);
        result |= (config & 0xFF);
    }
    else
    {
        DEBUG_LOG_ALWAYS("appTestUpgradeGetVersion: UpgradeGetVersion failed");
    }

    return result;
}

#ifdef ENABLE_EARBUD_FIT_TEST
void EarbudTest_StartFitTest(void)
{
    DEBUG_LOG_ALWAYS("EarbudTest_StartFitTest");
    Ui_InjectUiInput(ui_input_fit_test_start);
}

void EarbudTest_StopFitTest(void)
{
    DEBUG_LOG_ALWAYS("EarbudTest_StopFitTest");
    Ui_InjectUiInput(ui_input_fit_test_abort);
}

uint32 EarbudTest_GetLocalDeviceFitTestStatus(void)
{
    fit_test_result_t result = FitTest_GetLocalDeviceTestResult();
    DEBUG_LOG_ALWAYS("EarbudTest_GetLocalDeviceFitTestStatus: enum:fit_test_result_t:%d", result);
    return (uint32)result;
}

uint32 EarbudTest_GetRemoteDeviceFitTestStatus(void)
{
    fit_test_result_t result = FitTest_GetRemoteDeviceTestResult();
    DEBUG_LOG_ALWAYS("EarbudTest_GetRemoteDeviceFitTestStatus: enum:fit_test_result_t:%d", result);
    return (uint32)result;
}

void EarbudTest_StartFitTestTuning(void)
{
    /* Check if Fit Test tuning is already running */
    if(FitTest_IsTuningModeActive())
    {
        DEBUG_LOG_INFO("Fit Test tuning is already in progress");
    }
    else
    {
        /* Start Fit test tuning only if device is not incase */
        if(appPhyStateIsOutOfCase())
        {
            DEBUG_LOG_DEBUG("EarbudTest_StartFitTestTuning");
            Ui_InjectRedirectableUiInput(ui_input_fit_test_enter_tuning_mode, FALSE);
        }
    }
}

void EarbudTest_StopFitTestTuning(void)
{
    DEBUG_LOG_FN_ENTRY("EarbudTest_StopFitTestTuning");
    Ui_InjectRedirectableUiInput(ui_input_fit_test_exit_tuning_mode, FALSE);
}
#endif
#ifdef ENABLE_CONTINUOUS_EARBUD_FIT_TEST
void EarbudTest_ContinuousFitTestStart(void)
{
    DEBUG_LOG_ALWAYS("EarbudTest_ContinuousFitTestStart");
    Ui_InjectUiInput(ui_input_continuous_fit_test_start);
}

void EarbudTest_ContinuousFitTestStop(void)
{
    DEBUG_LOG_ALWAYS("EarbudTest_ContinuousFitTestStop");
    Ui_InjectUiInput(ui_input_continuous_fit_test_stop);
}

void EarbudTest_ContinuousFitTestSingleCapture(void)
{
    DEBUG_LOG_ALWAYS("EarbudTest_ContinuousFitTestSingleCapture");
    Ui_InjectUiInput(ui_input_continuous_fit_test_single_capture);
}
#endif

#ifdef INCLUDE_LE_AUDIO_UNICAST

bool appTestLeaUnicastIsAnyCisDelegated(void)
{
#ifdef ENABLE_LEA_CIS_DELEGATION
     bool is_cis_delegated = MirrorProfile_IsCisMirroringConnected();

     DEBUG_LOG_ALWAYS("appTestIsLeaUnicastIsCisDelegated: %d", is_cis_delegated);
     return is_cis_delegated;
#else
     return FALSE;
#endif/* ENABLE_LEA_CIS_DELEGATION */
}

#endif /* INCLUDE_LE_AUDIO_UNICAST */

#ifdef INCLUDE_UI_USER_CONFIG
ui_user_gesture_table_content_t test_ui_config_table[] =
{
    { .gesture_id = ui_gesture_tap, .originating_touchpad = touchpad_left, .context_id = ui_context_passthrough, .action_id = ui_action_volume_down },
    { .gesture_id = ui_gesture_tap, .originating_touchpad = touchpad_right, .context_id = ui_context_passthrough, .action_id = ui_action_volume_up },

    { .gesture_id = ui_gesture_swipe_up, .originating_touchpad = touchpad_left_and_right, .context_id = ui_context_voice_incoming, .action_id = ui_action_voice_accept_call },
    { .gesture_id = ui_gesture_swipe_up, .originating_touchpad = touchpad_left_and_right, .context_id = ui_context_media_idle, .action_id = ui_action_media_play_pause_toggle },
    { .gesture_id = ui_gesture_swipe_up, .originating_touchpad = touchpad_left_and_right, .context_id = ui_context_media_streaming, .action_id = ui_action_media_play_pause_toggle },

    { .gesture_id = ui_gesture_swipe_down, .originating_touchpad = touchpad_left_and_right, .context_id = ui_context_voice_in_call, .action_id = ui_action_voice_hang_up_call },

    { .gesture_id = ui_gesture_press_and_hold, .originating_touchpad = touchpad_left, .context_id = ui_context_passthrough, .action_id = ui_action_va_fetch_query },
};

void EarbudTest_ApplyUiUserConfigTestVector(void)
{
    UiUserConfig_SetUserGestureConfiguration(
            test_ui_config_table,
            ARRAY_DIM(test_ui_config_table)*sizeof(ui_user_gesture_table_content_t));
}

void EarbudTest_SetConfigurationForGesture(
        ui_user_config_gesture_id_t gesture,
        ui_user_gesture_table_content_t * configuration,
        uint8 configuration_length_in_elements)
{
    UiUserConfig_SetConfigurationForGesture(
            gesture,
            0,
            configuration,
            configuration_length_in_elements*sizeof(ui_user_gesture_table_content_t));
}

void EarbudTest_ResetAllGestureConfiguration(void)
{
    UiUserConfig_DeleteUserConfiguration(FALSE);
}
#endif

#ifdef ENABLE_ADAPTIVE_ANC
void EarbudTest_SetWorldVolumeUp(void)
{
    if(appPhyStateIsOutOfCase())
    {
        DEBUG_LOG_ALWAYS("EarbudTest_SetWorldVolumeUp");
        Ui_InjectUiInput(ui_input_anc_set_world_vol_up);
    }
}

void EarbudTest_SetWorldVolumeDown(void)
{
    if(appPhyStateIsOutOfCase())
    {
        DEBUG_LOG_ALWAYS("EarbudTest_SetWorldVolumeDown");
        Ui_InjectUiInput(ui_input_anc_set_world_vol_down);
    }
}

#endif /* ENABLE_ADAPTIVE_ANC */
#if defined HAVE_RDP_HW_18689 || defined INCLUDE_SENSOR_20_23709_TEST
#include <bitserial_api.h>
#include <pio.h>

static bitserial_handle i2c_h = BITSERIAL_HANDLE_ERROR;
static uint32 i2c_mask;
static uint8 i2c_bank;

static void earbudTest_InitBitserialPio(void)
{
    if (i2c_h != BITSERIAL_HANDLE_ERROR)
    {
        DEBUG_LOG_WARN("earbudTest_InitBitserialPio: closing BITSERIAL");
        EarbudTest_BitserialClose();
    }

    /* Set PIOs up for strong pull ups */
    PioSetMapPins32Bank(i2c_bank, i2c_mask, i2c_mask);
    PanicNotZero(PioSetDir32Bank(i2c_bank, i2c_mask, 0));
    PanicNotZero(PioSet32Bank(i2c_bank, i2c_mask, i2c_mask));
    PanicNotZero(PioSetStrongBias32Bank(i2c_bank, i2c_mask, i2c_mask));
}

static void earbudTest_BitserialOpen(bitserial_config *i2c_config, uint8 sda_pio, uint8 sck_pio)
{
    i2c_h = BitserialOpen(BITSERIAL_BLOCK_0, i2c_config);
    PanicFalse(i2c_h != BITSERIAL_HANDLE_ERROR);

    /* Pass control of PIOs to bitserial 0 */
    PioSetMapPins32Bank(i2c_bank, i2c_mask, 0);
    PioSetFunction(sck_pio, BITSERIAL_0_CLOCK_OUT);
    PioSetFunction(sck_pio, BITSERIAL_0_CLOCK_IN);
    PioSetFunction(sda_pio, BITSERIAL_0_DATA_OUT);
    PioSetFunction(sda_pio, BITSERIAL_0_DATA_IN);
}

bool EarbudTest_BitserialI2cLsm(uint32 i2c_clk, uint32 i2c_sda)
{
    i2c_mask = (1UL << (i2c_clk % 32)) | (1UL << (i2c_sda % 32));
    i2c_bank = (i2c_clk / 32);
    earbudTest_InitBitserialPio();

    static bitserial_config i2c_config;
    i2c_config.mode = BITSERIAL_MODE_I2C_MASTER;
    i2c_config.clock_frequency_khz = 400;
    i2c_config.timeout_ms = 1000;
    i2c_config.u.i2c_cfg.flags = 0x00;
    i2c_config.u.i2c_cfg.i2c_address = 0x6A; /* LSM6DS3 */
    earbudTest_BitserialOpen(&i2c_config, i2c_sda, i2c_clk);

    const uint8 tx_data[] = { 0x0f };
    uint8 rx_data[1];
    bitserial_result result;

    /* First write the register address to be read */
    result = BitserialWrite(i2c_h,
                            BITSERIAL_NO_MSG,
                            tx_data, sizeof(tx_data),
                            BITSERIAL_FLAG_BLOCK);
    if (result == BITSERIAL_RESULT_SUCCESS)
    {
        /* Now read the actual data in the register */
        result = BitserialRead(i2c_h,
                               BITSERIAL_NO_MSG,
                               rx_data, sizeof(rx_data),
                               BITSERIAL_FLAG_BLOCK);

        DEBUG_LOG_WARN("LSM6DS3: WHO_AM_I = %02X", rx_data[0]);
    }
    else
        DEBUG_LOG_WARN("TMD26353: error %u", result);

    return (result == BITSERIAL_RESULT_SUCCESS);
}

bool EarbudTest_BitserialI2cLis(uint32 i2c_clk, uint32 i2c_sda)
{
    i2c_mask = (1UL << (i2c_clk % 32)) | (1UL << (i2c_sda % 32));
    i2c_bank = (i2c_clk / 32);
    earbudTest_InitBitserialPio();

    static bitserial_config i2c_config;
    i2c_config.mode = BITSERIAL_MODE_I2C_MASTER;
    i2c_config.clock_frequency_khz = 400;
    i2c_config.timeout_ms = 0;
    i2c_config.u.i2c_cfg.flags = 0x00;
    i2c_config.u.i2c_cfg.i2c_address = 0x19; /* LIS25BA */
    earbudTest_BitserialOpen(&i2c_config, i2c_sda, i2c_clk);

    const uint8 tx_data[] = { 0x0f };
    uint8 rx_data[1];
    bitserial_result result;

    /* First write the register address to be read */
    result = BitserialWrite(i2c_h,
                            BITSERIAL_NO_MSG,
                            tx_data, sizeof(tx_data),
                            BITSERIAL_FLAG_BLOCK);
    if (result == BITSERIAL_RESULT_SUCCESS)
    {
        /* Now read the actual data in the register */
        result = BitserialRead(i2c_h,
                               BITSERIAL_NO_MSG,
                               rx_data, sizeof(rx_data),
                               BITSERIAL_FLAG_BLOCK);

        DEBUG_LOG_WARN("LIS25BA: WHO_AM_I = %02X", rx_data[0]);
    }
    else
        DEBUG_LOG_WARN("TMD26353: error %u", result);

    return (result == BITSERIAL_RESULT_SUCCESS);
}

bool EarbudTest_BitserialI2cTmd(uint32 i2c_clk, uint32 i2c_sda)
{
    i2c_mask = (1UL << (i2c_clk % 32)) | (1UL << (i2c_sda % 32));
    i2c_bank = (i2c_clk / 32);
    earbudTest_InitBitserialPio();

    /* Open bitserial 0 */
    static bitserial_config i2c_config;
    i2c_config.mode = BITSERIAL_MODE_I2C_MASTER;
    i2c_config.clock_frequency_khz = 400;
    i2c_config.timeout_ms = 1000;
    i2c_config.u.i2c_cfg.flags = 0x00;
    i2c_config.u.i2c_cfg.i2c_address = 0x39; /* TMD26353 */
    earbudTest_BitserialOpen(&i2c_config, i2c_sda, i2c_clk);

    const uint8 tx_data[] = { 0x92 };
    uint8 rx_data[1];
    bitserial_result result;

    /* Dummy access to fix I2C address */
    BitserialWrite(i2c_h,
                   BITSERIAL_NO_MSG,
                   tx_data, sizeof(tx_data),
                   BITSERIAL_FLAG_BLOCK);

    /* First write the register address to be read */
    result = BitserialWrite(i2c_h,
                            BITSERIAL_NO_MSG,
                            tx_data, sizeof(tx_data),
                            BITSERIAL_FLAG_BLOCK | BITSERIAL_FLAG_NO_STOP);
    if (result == BITSERIAL_RESULT_SUCCESS)
    {
        /* Now read the actual data in the register */
        result = BitserialRead(i2c_h,
                               BITSERIAL_NO_MSG,
                               rx_data, sizeof(rx_data),
                               BITSERIAL_FLAG_BLOCK);

        DEBUG_LOG_WARN("TMD26353: ID = %02X", rx_data[0]);
    }
    else
        DEBUG_LOG_WARN("TMD26353: error %u", result);

    return (result == BITSERIAL_RESULT_SUCCESS);
}

bool EarbudTest_BitserialI2cIqs(uint32 i2c_clk, uint32 i2c_sda, uint32 iqs_rdy)
{
    i2c_mask = (1UL << (i2c_clk % 32)) | (1UL << (i2c_sda % 32));
    i2c_bank = (i2c_clk / 32);
    const uint32 rdy_mask = (1UL << (iqs_rdy % 32));
    const uint32 rdy_bank = (iqs_rdy / 32);
    earbudTest_InitBitserialPio();

    static bitserial_config i2c_config;
    i2c_config.mode = BITSERIAL_MODE_I2C_MASTER;
    i2c_config.clock_frequency_khz = 400;
    i2c_config.timeout_ms = 1000;
    i2c_config.u.i2c_cfg.flags = 0x00;
    i2c_config.u.i2c_cfg.i2c_address = 0x44; /* IQS269A00QNR */
    earbudTest_BitserialOpen(&i2c_config, i2c_sda, i2c_clk);

    /* Set RDY PIO as input */
    PioSetMapPins32Bank(rdy_bank, rdy_mask, rdy_mask);
    PioSetDir32Bank(rdy_bank, rdy_mask, 0);
    PioSet32Bank(rdy_bank, rdy_mask, rdy_mask);

    /* Wait for RDY to go low */
    while (PioGet32Bank(rdy_bank) & rdy_mask);

    /* Read ID register (2 bytes) */
    const uint8 tx_data[] = { 0x00 };
    uint8 rx_data[2];
    bitserial_result result;
    result = BitserialTransfer(i2c_h, BITSERIAL_NO_MSG,
                      tx_data, sizeof(tx_data), rx_data, sizeof(rx_data));

   if (result == BITSERIAL_RESULT_SUCCESS)
   {
       DEBUG_LOG_WARN("IQS269A00QNR: ID = %02X %02X", rx_data[0], rx_data[1]);
   }
   else
   {
       DEBUG_LOG_WARN("IQS269A00QNR: error %u", result);
   }

    return (result == BITSERIAL_RESULT_SUCCESS);
}

#if defined HAVE_RDP_HW_18689 || defined INCLUDE_SENSOR_20_23709_TEST
bool EarbudTest_BitserialI2cTxcpa224(uint32 i2c_clk, uint32 i2c_sda)
{
    i2c_mask = (1UL << (i2c_clk % 32)) | (1UL << (i2c_sda % 32));
    i2c_bank = (i2c_clk / 32);
    earbudTest_InitBitserialPio();

    /* Open bitserial 0 */
    static bitserial_config i2c_config;
    i2c_config.mode = BITSERIAL_MODE_I2C_MASTER;
    i2c_config.clock_frequency_khz = 100;
    i2c_config.timeout_ms = 1000;
    i2c_config.u.i2c_cfg.flags = 0x00;
    i2c_config.u.i2c_cfg.i2c_address = 0x1E; /* TXCPA224 */
    earbudTest_BitserialOpen(&i2c_config, i2c_sda, i2c_clk);

    const uint8 tx_data[] = { 0x7F };
    uint8 rx_data[1];
    bitserial_result result;

    /* Dummy access to fix I2C address */
    BitserialWrite(i2c_h,
                   BITSERIAL_NO_MSG,
                   tx_data, sizeof(tx_data),
                   BITSERIAL_FLAG_BLOCK);

    /* First write the register address to be read */
    result = BitserialWrite(i2c_h,
                            BITSERIAL_NO_MSG,
                            tx_data, sizeof(tx_data),
                            BITSERIAL_FLAG_BLOCK | BITSERIAL_FLAG_NO_STOP);
    if (result == BITSERIAL_RESULT_SUCCESS)
    {
        /* Now read the actual data in the register */
        result = BitserialRead(i2c_h,
                               BITSERIAL_NO_MSG,
                               rx_data, sizeof(rx_data),
                               BITSERIAL_FLAG_BLOCK);

        DEBUG_LOG_WARN("TXCPA224: ID = %02X", rx_data[0]);
    }
    else
        DEBUG_LOG_WARN("TXCPA224: error %u", result);

    return (result == BITSERIAL_RESULT_SUCCESS);
}
#endif

void EarbudTest_BitserialClose(void)
{
    /* Map pins back to software control before closing Bitserial to avoid PIO glitches */
    PioSetMapPins32Bank(i2c_bank, i2c_mask, i2c_mask);
    BitserialClose(i2c_h);

    i2c_h = BITSERIAL_HANDLE_ERROR;
}

#endif // INCLUDE_SENSOR_20_23709_TEST

#ifdef PRODUCTION_TEST_MODE
void appTestPTMode(void)
{
    appSmEnterProductionTestMode();
}
#endif

#ifdef INCLUDE_ACCESSORY
void appTestIncludeAccessoryTests(void);
void appTestIncludeAccessoryTests(void)
{
    Accessory_IncludeTestApis();
}
#endif

#ifdef INCLUDE_GAA
void appTestIncludeGaaTests(void);
void appTestIncludeGaaTests(void)
{
    Gaa_IncludeTestApis();
}
#endif

#endif // DISABLE_TEST_API
