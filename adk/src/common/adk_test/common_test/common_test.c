/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       common_test.c
\brief      Implementation of common testing functions.
*/

#include "common_test.h"

#include <logging.h>
#include <connection_manager.h>
#include <ctype.h>
#include <device_list.h>
#include <vm.h>
#include "hfp_profile.h"
#include "hfp_profile_instance.h"
#include "hfp_profile_typedef.h"
#include "kymera_output_common_chain.h"
#include "handset_service.h"
#include "handset_service_config.h"
#include <av.h>
#include "le_advertising_manager.h"
#include "ui.h"
#ifdef INCLUDE_FAST_PAIR
#include "sass.h"
#include <fast_pair.h>
#include <user_accounts.h>
#endif
#include <focus_plugin.h>
#include <focus_select_config_types.h>
#include "device_properties.h"
#include "profile_manager.h"
#ifdef INCLUDE_GATT_QSS_SERVER
#include "gatt_server_qss.h"
#endif

#ifdef GC_SECTIONS
/* Move all functions in KEEP_PM section to ensure they are not removed during
 * garbage collection */
#pragma unitcodesection KEEP_PM
#endif

#ifdef ENABLE_BITRATE_STATISTIC
#include "context_framework.h"
#endif

bool appTestIsHandsetQhsConnectedAddr(const bdaddr* handset_bd_addr)
{
    bool qhs_connected_status = FALSE;

    if (handset_bd_addr != NULL)
    {
        qhs_connected_status = ConManagerGetQhsConnectStatus(handset_bd_addr);

        DEBUG_LOG_ALWAYS("appTestIsHandsetQhsConnectedAddr addr [%04x,%02x,%06lx] qhs_connected:%d", 
                         handset_bd_addr->nap,
                         handset_bd_addr->uap,
                         handset_bd_addr->lap,
                         qhs_connected_status);
    }
    else
    {
        DEBUG_LOG_WARN("appTestIsHandsetQhsConnectedAddr BT adrress is NULL");
    }

    return qhs_connected_status;
}

bool appTestIsHandsetAddrConnected(const bdaddr* handset_bd_addr)
{
    bool is_connected = FALSE;

    if (handset_bd_addr != NULL)
    {
        device_t device = BtDevice_GetDeviceForBdAddr(handset_bd_addr);
        if (device != NULL)
        {
            uint32 connected_profiles = BtDevice_GetConnectedProfiles(device);
            if ((connected_profiles & (DEVICE_PROFILE_HFP | DEVICE_PROFILE_A2DP | DEVICE_PROFILE_AVRCP)) != 0)
            {
                is_connected = TRUE;
            }
        }

        DEBUG_LOG_ALWAYS("appTestIsHandsetAddrConnected addr [%04x,%02x,%06lx] device:%p is_connected:%d",
                         handset_bd_addr->nap,
                         handset_bd_addr->uap,
                         handset_bd_addr->lap,
                         device,
                         is_connected);
    }
    else
    {
        DEBUG_LOG_WARN("appTestIsHandsetAddrConnected BT address is NULL");
    }

    return is_connected;
}

bool appTestIsHandsetAddrConnectedOverLE(const bdaddr* handset_bd_addr)
{
    bool is_connected = FALSE;

    if (handset_bd_addr != NULL)
    {
        device_t device = BtDevice_GetDeviceForBdAddr(handset_bd_addr);
        if (device != NULL)
        {
            tp_bdaddr handset_tp_bdaddr = {
                .transport = TRANSPORT_BLE_ACL,
                .taddr = {
                    .type = TYPED_BDADDR_PUBLIC,
                    .addr = {
                        .nap = handset_bd_addr->nap,
                        .uap = handset_bd_addr->uap,
                        .lap = handset_bd_addr->lap
                    }
                }
            };

            is_connected = ConManagerIsTpConnected(&handset_tp_bdaddr);
        }

        DEBUG_LOG_ALWAYS("appTestIsHandsetAddrConnectedOverLE addr [%04x,%02x,%06lx] device:%p is_connected:%d",
                         handset_bd_addr->nap,
                         handset_bd_addr->uap,
                         handset_bd_addr->lap,
                         device,
                         is_connected);
    }
    else
    {
        DEBUG_LOG_WARN("appTestIsHandsetAddrConnectedOverLE BT address is NULL");
    }

    return is_connected;
}

bool appTestIsHandsetHfpScoActiveAddr(const bdaddr* handset_bd_addr)
{
    bool is_sco_active = FALSE;

    if (handset_bd_addr != NULL)
    {
        hfpInstanceTaskData* instance = HfpProfileInstance_GetInstanceForBdaddr(handset_bd_addr);

        is_sco_active = HfpProfile_IsScoActiveForInstance(instance);
        DEBUG_LOG_ALWAYS("appTestIsHandsetHfpScoActiveAddr addr [%04x,%02x,%06lx] is_sco_active:%d", 
                         handset_bd_addr->nap,
                         handset_bd_addr->uap,
                         handset_bd_addr->lap,
                         is_sco_active);
    }
    else
    {
        DEBUG_LOG_WARN("appTestIsHandsetHfpScoActiveAddr BT adrress is NULL");
    }

    return is_sco_active;
}

void appTestEnableCommonChain(void)
{
    DEBUG_LOG_ALWAYS("appTestEnableCommonChain");
    Kymera_OutputCommonChainEnable();
}

void appTestDisableCommonChain(void)
{
    DEBUG_LOG_ALWAYS("appTestDisableCommonChain");
    Kymera_OutputCommonChainDisable();
}

int16 appTestGetRssiOfTpAddr(tp_bdaddr *tpaddr)
{
    int16 rssi = 0;
    if(VmBdAddrGetRssi(tpaddr, &rssi) == FALSE)
    {
        rssi = 0;
    }
    DEBUG_LOG_ALWAYS("appTestGetRssiOfConnectedTpAddr transport=%d tpaddr=%04x,%02x,%06lx RSSI=%d",
                    tpaddr->transport, tpaddr->taddr.addr.lap, tpaddr->taddr.addr.uap, tpaddr->taddr.addr.nap, rssi);
    return rssi;
}

int16 appTestGetBredrRssiOfConnectedHandset(void)
{
    tp_bdaddr tp_addr;
    if(HandsetService_GetConnectedBredrHandsetTpAddress(&tp_addr))
    {
        return appTestGetRssiOfTpAddr(&tp_addr);
    }
    return 0;
}

int16 appTestGetLeRssiOfConnectedHandset(void)
{
    tp_bdaddr tp_addr;
    if(HandsetService_GetConnectedLeHandsetTpAddress(&tp_addr))
    {
        return appTestGetRssiOfTpAddr(&tp_addr);
    }
    return 0;
}

bool appTestEnableConnectionBargeIn(void)
{
    /* Connection barge-in is not compatible with the legacy version. 
       Attempts to enable connection barge-in when MULTIPOINT_BARGE_IN_ENABLED
       will fail. MULTIPOINT_BARGE_IN_ENABLED should only be used where truncated 
       page scan is not supported. */
    return handsetService_SetConnectionBargeInEnable(TRUE);
}

bool appTestDisableConnectionBargeIn(void)
{
    return handsetService_SetConnectionBargeInEnable(FALSE);
}

bool appTestIsConnectionBargeInEnabled(void)
{
    return HandsetService_IsConnectionBargeInEnabled();
}

/*! \brief Return if Earbud/Headset is in A2DP streaming mode with any connected handset
*/
bool appTestIsHandsetA2dpStreaming(void)
{
    bool streaming = FALSE;
    bdaddr* bd_addr = NULL;
    unsigned num_addresses = 0;

    if (BtDevice_GetAllHandsetBdAddr(&bd_addr, &num_addresses))
    {
        unsigned index;
        for(index = 0; index < num_addresses; index++)
        {
            if(appTestIsHandsetA2dpStreamingBdaddr(&bd_addr[index]))
            {
                streaming = TRUE;
                break;
            }
        }
        free(bd_addr);
        bd_addr  = NULL;
    }

    DEBUG_LOG_ALWAYS("appTestIsHandsetA2dpStreaming:%d", streaming);
    return streaming;
}

/*! \brief Return if Earbud/Headset is in A2DP streaming mode with a handset having specified bluetooth address
*/
bool appTestIsHandsetA2dpStreamingBdaddr(bdaddr* bd_addr)
{
    bool streaming = FALSE;

    /* Find handset AV instance */
    avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(bd_addr);
    streaming = theInst && appA2dpIsStreaming(theInst);

    return streaming;
}

void appTestConfigureLinkLossReconnectionParameters(
        bool use_unlimited_reconnection_attempts,
        uint8 num_connection_attempts,
        uint32 reconnection_page_interval_ms,
        uint16 reconnection_page_timeout_ms)
{
    DEBUG_LOG_ALWAYS("appTestConfigureLinkLossReconnectionParameters use_unlimited_reconnection_attempts=%d "
                     "num_connection_attempts=%d reconnection_page_interval_ms=%d reconnection_page_timeout_ms=%d",
                     use_unlimited_reconnection_attempts, num_connection_attempts,
                     reconnection_page_interval_ms, reconnection_page_timeout_ms);

    HandsetService_ConfigureLinkLossReconnectionParameters(
                use_unlimited_reconnection_attempts,
                num_connection_attempts,
                handsetService_GetInitialReconnectionPageInterval(),
                reconnection_page_timeout_ms,
                reconnection_page_interval_ms,
                reconnection_page_timeout_ms);
}

void appTestConfigureRecommendedInfiniteLinkLossReconnectionParameters(bool use_unlimited_reconnection_attempts)
{
    DEBUG_LOG_ALWAYS("appTestConfigureRecommendedInfiniteLinkLossReconnectionParameters enabled=%d",
                     use_unlimited_reconnection_attempts);

    HandsetService_ConfigureLinkLossReconnectionParameters(use_unlimited_reconnection_attempts, 3, 500, 5000, 30000, 5000);
}

/*! \brief Send A2dp Media Suspend request to remote
    Added to support PTS qualification TCs.
*/
bool appTestHandsetA2dpMediaSuspend(void)
{
    DEBUG_LOG_ALWAYS("appTestHandsetA2dpMediaSuspend");

    bdaddr bd_addr;

    if (appDeviceGetHandsetBdAddr(&bd_addr))
    {
#ifdef USE_SYNERGY
        /* Find handset AV instance */
        avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&bd_addr);

        if (theInst)
        {
            uint8 *list = PanicUnlessMalloc(1 * sizeof(uint8));

            *list = theInst->a2dp.stream_id;
            theInst->a2dp.suspend_state |= AV_SUSPEND_REASON_LOCAL;
            AvSuspendReqSend(A2DP_ASSIGN_TLABEL(theInst), list, 1);
        }
#else
        /* Suspend AV link with reason as AV activity. */
        appAvStreamingSuspend(AV_SUSPEND_REASON_LOCAL);
#endif
		return TRUE;
    }

    return FALSE;
}

#ifdef INCLUDE_FAST_PAIR
void appTestAssignFastPairAccountKeyToAllHandsets(void)
{
    for (unsigned idx = 0; idx < DeviceList_GetMaxTrustedDevices(); idx++)
    {
        device_t device = DeviceList_GetDeviceAtIndex(idx);

        if (device && BtDevice_GetDeviceType(device) == DEVICE_TYPE_HANDSET)
        {
            bdaddr bd_addr = DeviceProperties_GetBdAddr(device);
            uint8 hs_key_index = UserAccounts_GetAccountKeyIndexWithHandset(device);

            /* Ignore existing handsets which already have bound account keys */
            if (hs_key_index != INVALID_USER_ACCOUNT_KEY_INDEX)
            {
                DEBUG_LOG_INFO("appTestAssignFastPairAccountKeyToAllHandsets, existing, lap=%06lx, aidx=%u", bd_addr.lap, hs_key_index);
                continue;
            }

            /* Generate a new account key and bind it to this handset */
            uint8 account_key[FAST_PAIR_ACCOUNT_KEY_LEN];
            memset(account_key, 0xfe, sizeof(account_key));

            /* First byte must be 0x4, per GFP rules */
            account_key[0] = 0x04;
            account_key[FAST_PAIR_ACCOUNT_KEY_LEN - 1] = UtilRandom() & 0xff;

            hs_key_index = UserAccounts_AddAccountKeyForHandset(&bd_addr, account_key, sizeof(account_key), user_account_type_fast_pair);
            PanicFalse(hs_key_index != INVALID_USER_ACCOUNT_KEY_INDEX);

            DEBUG_LOG_INFO("appTestAssignFastPairAccountKeyToAllHandsets, new, lap=%06lx, aidx=%u", bd_addr.lap, hs_key_index);
            DEBUG_LOG_DATA_INFO(account_key, sizeof(account_key));
        }
    }
}

void appTest_SassDisableConnectionSwitch(void)
{
    DEBUG_LOG_ALWAYS("appTest_SassDisableConnectionSwitch");
    Ui_InjectUiInput(ui_input_disable_sass_connection_switch);
}

void appTest_SassEnableConnectionSwitch(void)
{
    DEBUG_LOG_ALWAYS("appTest_SassEnableConnectionSwitch");
    Ui_InjectUiInput(ui_input_enable_sass_connection_switch);
}

bool appTest_IsSassSwitchDisabled(void)
{
    bool disabled = Sass_IsConnectionSwitchDisabled();
    DEBUG_LOG_ALWAYS("appTest_IsSassSwitchDisabled:%d",disabled);
    return disabled;
}
#endif /* INCLUDE_FAST_PAIR */

bool appTestBlockDisconnectOfHandsetsWithDfuConnection(bool block)
{
    return Focus_SetConfig(focus_select_block_disconnect_with_dfu, &block);
}

bool appTestBlockDisconnectOfHandsetsWithVoiceOrAudioFocus(bool block)
{
    return Focus_SetConfig(focus_select_block_disconnect_with_audio, &block);
}

bool appTestCancelAptxVoicePacketsCounter(const bdaddr* handset_bd_addr)
{
    bool disabled =  FALSE;

    if (handset_bd_addr != NULL)
    {
       disabled = HfpProfileInstance_DisableAptxBlacklistTimer(handset_bd_addr);
    }

    return disabled;
}


#ifdef INCLUDE_GATT_QSS_SERVER
bool appTest_SetQssUserDescription(const char *description, uint8 len)
{
    bool status = GattServerQss_SetQssUserDescription(description, len);

    DEBUG_LOG_ALWAYS("appTest_SetQssUserDescription, status: %d", status);
    return status;
}
#endif

static void appTest_ReportProperty(device_t device, device_property_t property)
{
    if (Device_IsPropertySet(device,property))
    {
        uint8 value = 0;
        uint16 u16_value = 0;
        uint32 u32_value = 0;
        switch (property)
        {
            case device_property_bdaddr:
                {
                    bdaddr bd_addr = DeviceProperties_GetBdAddr(device);
                    DEBUG_LOG_VERBOSE("\tbdaddr nap %x uap %x lap %x", bd_addr.nap, bd_addr.uap, bd_addr.lap);
                }
                break;
            case device_property_audio_volume:
                Device_GetPropertyU8(device, property, &value);
                DEBUG_LOG_VERBOSE("\taudio_volume %02x", value);
                break;
            case device_property_audio_source:
                Device_GetPropertyU8(device, property, &value);
                DEBUG_LOG_VERBOSE("\taudio_source %02x", value);
                break;
            case device_property_av_instance:
                DEBUG_LOG_VERBOSE("\tav_instance %08x", Av_InstanceFindFromDevice(device));
                break;
            case device_property_hfp_profile:
                Device_GetPropertyU8(device, property, &value);
                DEBUG_LOG_VERBOSE("\thfp_profile %02x", value);
                break;
            case device_property_type:
                Device_GetPropertyU8(device, property, &value);
                DEBUG_LOG_VERBOSE("\ttype %02x", value);
                break;
            case device_property_link_mode:
                Device_GetPropertyU8(device, property, &value);
                DEBUG_LOG_VERBOSE("\tlink_mode %02x", value);
                break;
            case device_property_supported_profiles:
                Device_GetPropertyU32(device, property, &u32_value);
                DEBUG_LOG_VERBOSE("\tsupported_profiles %08x", u32_value);
                break;
            case device_property_connected_profiles:
                Device_GetPropertyU32(device, property, &u32_value);
                DEBUG_LOG_VERBOSE("\tcurrently connected_profiles %08x", u32_value);
                break;
            case device_property_flags:
                Device_GetPropertyU16(device, property, &u16_value);
                DEBUG_LOG_VERBOSE("\tflags %04x", u16_value);
                break;
            case device_property_sco_fwd_features:
                Device_GetPropertyU16(device, property, &u16_value);
                DEBUG_LOG_VERBOSE("\tsco_fwd_features %04x", u16_value);
                break;
            case device_property_mru:
                Device_GetPropertyU8(device, property, &value);
                DEBUG_LOG_VERBOSE("\tmru %1x", value);
                break;
            case device_property_profile_request_index:
                Device_GetPropertyU8(device, property, &value);
                DEBUG_LOG_VERBOSE("\tprofile_request_index %1x", value);
                break;
            case device_property_profiles_connect_order:
                {
                    profile_t *ptr_profile_order = NULL;
                    size_t size;
                    PanicFalse(Device_GetProperty(device, property, (void *)&ptr_profile_order, &size));
                    profile_t array[5] = {profile_manager_bad_profile};
                    memcpy(array, ptr_profile_order, MIN(size,5));
                    DEBUG_LOG_VERBOSE("\tprofiles_connect_order %d,%d,%d,%d,%d", array[0], array[1], array[2], array[3], array[4]);
                }
                break;
            case device_property_voice_volume:
                Device_GetPropertyU8(device, property, &value);
                DEBUG_LOG_VERBOSE("\tvoice_volume %02x", value);
                break;
            case device_property_va_flags:
                Device_GetPropertyU8(device, property, &value);
                DEBUG_LOG_VERBOSE("\tva_flags %02x", value);
                break;
            case device_property_va_locale:
                {
                    uint8 va_locale[4] = { '#','#','#','#' };
                    void *ptr;
                    size_t size;
                    if( Device_GetProperty(device, property, &ptr, &size) )
                    {
                        memcpy(va_locale, ptr, MIN(size,sizeof(va_locale)));
                        for ( size_t i = 0 ; i < sizeof(va_locale) ; i++ )
                        {
                            if ( va_locale[i] == '\0' ) va_locale[i] = '.';
                            if ( !isprint(va_locale[i]) ) va_locale[i] = '?';
                        }
                    }
                    DEBUG_LOG_VERBOSE("\tva_locale \"%c%c%c%c\"",
                                    va_locale[0], va_locale[1], va_locale[2], va_locale[3]);
                }
                break;
            case device_property_voice_assistant:
                Device_GetPropertyU8(device, property, &value);
                DEBUG_LOG_VERBOSE("\tvoice_assistant %02x", value);
                break;
            default:
            {
                void * property_value = NULL;
                size_t property_size = 0;
                Device_GetProperty(device, property, &property_value, &property_size);
                DEBUG_LOG_VERBOSE("\tenum:earbud_device_property_t:%d", property);
                DEBUG_LOG_DATA_VERBOSE(property_value, property_size);
                break;
            }
        }
    }
}

static void inline appTest_ReportDevicePropertyRange(device_t device, int start_index, int end_index)
{
    for (int property=start_index; property < end_index; property++)
    {
        if (Device_IsPropertySet(device, property))
        {
            appTest_ReportProperty(device, property);
        }
    }
}

static void appTest_ReportDeviceData(device_t device, void * data)
{
    UNUSED(data);

    DEBUG_LOG_ALWAYS("Device %x", device);
    appTest_ReportDevicePropertyRange(device, 0, device_property_max_num);
    appTest_ReportDevicePropertyRange(device, device_property_customer_specific_properties_start, device_property_customer_specific_properties_max);
}

void appTest_DeviceDatabaseReport(void)
{
    DEBUG_LOG_ALWAYS("DeviceDatabase");
    DeviceList_Iterate(appTest_ReportDeviceData, NULL);
}
#ifdef ENABLE_BITRATE_STATISTIC
static context_streaming_info_t streaming_info;
bool appTestGetStreamingInfo(void)
{
    if (ContextFramework_GetContextItem(context_streaming_info, (void *)&streaming_info, sizeof (context_streaming_info_t)))
    {
        DEBUG_LOG_ALWAYS("appTestGetStreamingInfo: seid %d, pri rssi %d, pri link quality %d", streaming_info.seid, streaming_info.primary_rssi, streaming_info.primary_link_quality);
        DEBUG_LOG_ALWAYS("appTestGetStreamingInfo: bitrate %d, is aptx adaptive %d, is aptx lossless %d", streaming_info.bitrate, streaming_info.is_adaptive, streaming_info.is_lossless);
        return TRUE;
    }

    return FALSE;
}
#endif
