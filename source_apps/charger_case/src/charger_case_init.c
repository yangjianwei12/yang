
/*!
\copyright  Copyright (c) 2020 - 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Initialisation module
*/

#ifndef CHARGER_CASE_INIT_C
#define CHARGER_CASE_INIT_C

#include "charger_case_init.h"
#include "charger_case_a2dp_source.h"
#include "charger_case_advertising.h"
#include "charger_case_audio.h"
#include "charger_case_config.h"
#include "charger_case_dfu.h"
#include "charger_case_setup_audio.h"
#include "charger_case_sm.h"
#include "charger_case_ui_config.h"
#include "charger_case_ui.h"
#include "app_task.h"
#include "charger_case_temperature_config.h"
#include "charger_case_region_config.h"
#include "charger_case_soc_config.h"
#include "authentication.h"
#include "adk_log.h"
#include "unexpected_message.h"
#include "temperature.h"
#include "battery_region.h"
#include "bandwidth_manager.h"
#include "state_of_charge.h"
#include "handset_service.h"
#include "pairing.h"
#include "power_manager.h"
#include "power_manager_action.h"
#include "gaia.h"
#include "gaia_framework.h"
#include "upgrade_gaia_plugin.h"
#include "gatt_handler.h"
#include "gatt_connect.h"
#include "gatt_server_battery.h"
#include "gatt_server_gatt.h"
#include "gatt_server_gap.h"
#include "connection_manager_config.h"
#include "device_db_serialiser.h"
#include "device_properties.h"
#include "bt_device_class.h"
#include "link_policy.h"
#include "input_event_manager.h"
#include "pio_monitor.h"
#include "local_addr.h"
#include "local_name.h"
#include "connection_message_dispatcher.h"
#include "ui.h"
#include "ui_indicator_tones.h"
#include "volume_messages.h"
#include "media_player.h"
#include "audio_sources.h"
#include "voice_sources.h"
#include "volume_service.h"
#include "le_advertising_manager.h"
#include "le_scan_manager.h"
#include "wired_audio_source.h"
#include "focus_select.h"
#include "charger_case_volume_observer.h"

#include "anc_state_manager.h"
#include "charger_case_buttons.h"
#include "usb_device.h"
#include "usb_application.h"
#include "usb_app_default.h"
#include "usb_app_source_audio.h"
#include "usb_app_voice.h"

#include <bredr_scan_manager.h>
#include <connection_manager.h>
#include <device_list.h>
#include <dfu.h>
#include <charger_monitor.h>
#include <message_broker.h>
#include <profile_manager.h>
#include <ui_indicator_prompts.h>
#include <led_manager.h>
#include <av.h>
#include <ui.h>
#include <ui_indicator_leds.h>
#include <multidevice.h>
#include <system_state.h>
#include <tx_power.h>
#include <battery_monitor.h>
#include <cc_with_earbuds.h>
#include <telephony_service.h>
#include <telephony_messages.h>

#include "charger_case_init_bt.h"
#include "charger_case_setup_unexpected_message.h"

#include <panic.h>
#include <pio.h>
#include <stdio.h>
#include <feature.h>

#include <app/bluestack/dm_prim.h>
#include "charger_case_adaptive_latency.h"

/*! The number of non BT Devices supported by Charger Case application */
#define appConfigChargerCaseMaxNonBtDevicesSupported() (USB_DEVICE_MAX_USB_HOSTS_SUPPORTED + WIRED_AUDIO_MAX_ANALOG_SOURCE_SUPPORTED)


/*! \brief Transport manager does not have proper init function, so add a fix for it   */
static bool charger_caseInitTransportManagerInitFixup(Task init_task)
{
    UNUSED(init_task);

    TransportMgrInit();

    return TRUE;
}

static const bt_device_default_value_callback_t property_default_values[] =
{
        {device_property_handset_service_config, HandsetService_SetDefaultConfig}
};

static const bt_device_default_value_callback_list_t default_value_callback_list = {property_default_values, ARRAY_DIM(property_default_values)};


/*! \brief Utility function to Init device DB serialiser   */
static bool appInitDeviceDbSerialiser(Task init_task)
{
    UNUSED(init_task);

    DeviceDbSerialiser_Init();

    BtDevice_RegisterPropertyDefaults(&default_value_callback_list);

    /* Register persistent device data users */
    BtDevice_RegisterPddu();

    HandsetService_RegisterPddu();

    /* Allow space in device list to store all paired devices + connected handsets not yet paired + non BT devices*/
    DeviceList_Init(appConfigChargerCaseMaxDevicesSupported() + appConfigMaxDeviceInstancesUsedForPairing() + appConfigChargerCaseMaxNonBtDevicesSupported());
    /* Set maximum supported BT devices */
    BtDevice_SetMaxTrustedDevices(appConfigChargerCaseMaxDevicesSupported() + appConfigMaxDeviceInstancesUsedForPairing());
    DeviceDbSerialiser_Deserialise();
    
    return TRUE;
}

/*! \brief Utility function to get input actions  */
static const InputActionMessage_t* appInitGetInputActions(uint16* input_actions_dim)
{
    const InputActionMessage_t* input_actions = NULL;

    *input_actions_dim = ARRAY_DIM(default_message_group);
    input_actions = default_message_group;

    return input_actions;
}
/*! \brief Utility function to init event manager   */
static bool appInputEventMangerInit(Task init_task)
{
    const InputActionMessage_t* input_actions = NULL;
    uint16 input_actions_dim = 0;
    UNUSED(init_task);
    
    input_actions = appInitGetInputActions(&input_actions_dim);
    PanicNull((void*)input_actions);
    InputEventManagerInit(Ui_GetUiTask(), input_actions, input_actions_dim, &input_event_config);
    return TRUE;
}


#ifdef UNMAP_AFH_CH78

/*! Unmap AFH channel 78

    It is need to meet regulatory requirements when QHS is used.
*/
static bool chargerCase_RemapAfh78(Task init_task)
{
    static const uint8_t afh_map[10] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3f};

    UNUSED(init_task);

    MESSAGE_MAKE(prim, DM_HCI_SET_AFH_CHANNEL_CLASS_REQ_T);
    prim->common.op_code = DM_HCI_SET_AFH_CHANNEL_CLASS_REQ;
    prim->common.length = sizeof(DM_HCI_SET_AFH_CHANNEL_CLASS_REQ_T);
    memcpy(prim->map, afh_map, sizeof(afh_map));

    VmSendDmPrim(prim);

    return TRUE;
}
#endif

/*! \brief Utility to check license for codec and cvc

    This function is used to verify different codec/cVc license are available or not
*/
static bool charger_caseLicenseCheck(Task init_task)
{
    UNUSED(init_task);
    return TRUE;
}

/*! \brief Function to initialise application AV interface.

    Used to initialise A2DP & AVRCP related functionality in the application.
*/
static bool charger_caseAvSourceInitCfmHandler(Message message)
{
    UNUSED(message);
    ChargerCase_A2dpSourceInit();
    ChargerCase_VolumeObserverInit();
    return TRUE;
}


#ifdef INIT_DEBUG
/*! Debug function blocks execution until appInitDebugWait is cleared:
    apps1.fw.env.vars['charger_caseInitDebugWait'].set_value(0) */
static bool charger_caseInitDebug(Task init_task)
{
    volatile static bool charger_caseInitDebugWait = TRUE;
    while(charger_caseInitDebugWait);

    UNUSED(init_task);
    return TRUE;
}
#endif

/*
 * Not enabled by default yet, as ANC tuning is still using legacy
 * usb_device_class library.
*/
static bool charger_caseUsbApplication(Task init_task)
{
   UNUSED(init_task);

#if defined(INCLUDE_USB_AUDIO)
   UsbApplication_Open(&usb_app_source_audio);
#endif

   return TRUE;
}

#ifdef INCLUDE_DFU
static bool chargerCaseInit_DfuSetAppVersionInfo(Task init_task)
{
    UNUSED(init_task);

    DEBUG_LOG_FN_ENTRY("chargerCaseInit_DfuAppRegister");

    Dfu_SetVersionInfo(UPGRADE_INIT_VERSION_MAJOR, UPGRADE_INIT_VERSION_MINOR, UPGRADE_INIT_CONFIG_VERSION);

    return TRUE;
}

static bool chargerCaseInit_UpgradeGaiaPluginRegister(Task init_task)
{
    UNUSED(init_task);

    DEBUG_LOG_FN_ENTRY("chargerCaseInit_UpgradeGaiaPluginRegister");

    UpgradeGaiaPlugin_Init();

    return TRUE;
}
#endif /* INCLUDE_DFU */

/*! \brief Utility function to config the gatt battery server */
#define BATTERY_SERVERS_MAX 1
static bool ChargerCaseGattServerBatteryConfig(Task init_task)
{
    GattServerBattery_SetNumberOfBatteryServers(BATTERY_SERVERS_MAX);

    UNUSED(init_task);
    return TRUE;
}

#ifdef INCLUDE_TEMPERATURE
/*! \brief Utility function to Initialise Temperature component   */
static bool charger_caseTemperatureInit(Task init_task)
{
    const temperature_lookup_t* config_table;
    unsigned config_table_size;

    config_table = ChargerCaseTemperature_GetConfigTable(&config_table_size);

    /* set voltage->temperature config table */
    Temperature_SetConfigurationTable(config_table, config_table_size);
    
    appTemperatureInit(init_task);
    return TRUE;
}
#endif

/*! \brief Utility function to Initialise Battery Region component   */
static bool charger_caseBatteryRegionInit(Task init_task)
{
    const charge_region_t* config_table;
    unsigned config_table_size;
    const battery_region_handlers_t* handlers_list;

    UNUSED(init_task);

    config_table = ChargerCaseRegion_GetChargeModeConfigTable(&config_table_size);

    /* set charge mode config table */
    BatteryRegion_SetChargeRegionConfigTable(CHARGE_MODE, config_table, config_table_size);

    config_table = ChargerCaseRegion_GetDischargeModeConfigTable(&config_table_size);

    /* set discharge mode config table */
    BatteryRegion_SetChargeRegionConfigTable(DISCHARGE_MODE, config_table, config_table_size);

    /* get handler functions list */
    handlers_list = ChargerCaseRegion_GetRegionHandlers();

    /* set region state handler functions list */
    BatteryRegion_SetHandlerStructure(handlers_list);
    
    BatteryRegion_Init();
    return TRUE;
}

/*! \brief Utility function to Initialise SoC component   */
static bool charger_caseSoCInit(Task init_task)
{
    const soc_lookup_t* config_table;
    unsigned config_table_size;

    UNUSED(init_task);
    config_table = ChargerCaseSoC_GetConfigTable(&config_table_size);

    /* set voltage->percentage config table */
    Soc_SetConfigurationTable(config_table, config_table_size);
    
    Soc_Init();
    return TRUE;
}

/*! \brief Table of initialisation functions */
static const system_state_step_t charger_caseInitTable[] =
{
#ifdef INIT_DEBUG
    {charger_caseInitDebug,      0, NULL},
#endif
    {PioMonitorInit,        0, NULL},
    {Ui_Init,               0, NULL},
    {charger_caseLicenseCheck,   0, NULL},
#ifdef INCLUDE_TEMPERATURE
    {charger_caseTemperatureInit,    0, NULL},
#endif
    {appBatteryInit,        MESSAGE_BATTERY_INIT_CFM, NULL},
#ifdef INCLUDE_CHARGER
    {Charger_Init,          0, NULL},
#endif
    {LedManager_Init,       0, NULL},
    {charger_caseBatteryRegionInit,        0, NULL},
    {appPowerInit,          APP_POWER_INIT_CFM, NULL},
    {charger_caseSoCInit,        0, NULL},
#ifdef INCLUDE_DFU
    {Dfu_EarlyInit,         0, NULL},
#endif /* INCLUDE_DFU */
    {AppConnectionInit, INIT_CL_CFM, NULL},
#ifdef UNMAP_AFH_CH78
    {chargerCase_RemapAfh78,       0, NULL},
#endif
    {AppMessageDispatcherRegister, 0, NULL},
	{appInputEventMangerInit, 0, NULL},
    {LocalAddr_Init,        0, NULL},
    {ConManagerInit,        0, NULL},
    {appInitDeviceDbSerialiser, 0, NULL},
    {appDeviceInit,         INIT_READ_LOCAL_BD_ADDR_CFM, appDeviceHandleClDmLocalBdAddrCfm},
    {BandwidthManager_Init, 0, NULL},
    {BredrScanManager_Init, BREDR_SCAN_MANAGER_INIT_CFM, NULL},
    {LocalName_Init,        LOCAL_NAME_INIT_CFM, NULL},
    {LeAdvertisingManager_Init,     0, NULL},
    {LeScanManager_Init,    0, NULL},
    {Volume_InitMessages,   0, NULL},
    {AudioSources_Init,     0, NULL},
    {Pairing_Init,          PAIRING_INIT_CFM, NULL},
    {FocusSelect_Init,      0, NULL},
    {UsbDevice_Init,        0, NULL},
#ifdef INCLUDE_CASE_COMMS
    {CcWithEarbuds_Init,    0, NULL},
#endif

    {charger_caseUsbApplication, 0, NULL},
    {appKymeraInit,         0, NULL},
    {ChargerCase_AudioInit, 0, NULL},

    {charger_caseInitTransportManagerInitFixup, 0, NULL},        //! \todo TransportManager does not meet expected init interface

    {GattConnect_Init,      0, NULL},   // GATT functionality is initialised by calling GattConnect_Init then GattConnect_ServerInitComplete.
    // All GATT Servers MUST be initialised after GattConnect_Init and before GattConnect_ServerInitComplete.
    {GattHandlerInit,      0, NULL},
#ifdef INCLUDE_GATT_BATTERY_SERVER
    {ChargerCaseGattServerBatteryConfig,  0, NULL},
    {GattServerBattery_Init,0, NULL},
#endif
    {GattServerGatt_Init,   0, NULL},
    {GattServerGap_Init,    0, NULL},
    {ProfileManager_Init,   0, NULL},
    {HandsetService_Init,   0, NULL},

#ifdef INCLUDE_GAIA
    {GaiaFramework_Init,    APP_GAIA_INIT_CFM, NULL},   // Gatt needs GAIA
#ifdef INCLUDE_DFU
    {chargerCaseInit_UpgradeGaiaPluginRegister, 0, NULL},
#endif /* INCLUDE_DFU */
#endif /* INCLUDE_GAIA */

#ifdef INCLUDE_DFU
    {chargerCaseInit_DfuSetAppVersionInfo, 0, NULL},    // Must be called before Dfu_Init
    {Dfu_Init,              UPGRADE_INIT_CFM, NULL},
    {ChargerCaseDfu_Init,   0, NULL},
#endif /* INCLUDE_DFU */
    {VolumeService_Init,    0, NULL},
    {UiLeds_Init,           0, NULL},
    {ChargerCaseUi_Init,    0, NULL},
    {Telephony_InitMessages, 0, NULL},

#ifndef USE_SYNERGY
    // All GATT Servers MUST be initialised before GATT initialisation is complete.
    {GattConnect_ServerInitComplete, GATT_CONNECT_SERVER_INIT_COMPLETE_CFM, NULL},
#endif /* USE_SYNERGY */    
    {appAvInit,             AV_INIT_CFM, charger_caseAvSourceInitCfmHandler},
    {ChargerCaseAdvertising_Init, LE_ADV_MGR_ALLOW_ADVERTISING_CFM, NULL},
    {ChargerCaseAdvertising_EnableLePrivateAddresses, LOCAL_ADDR_CONFIGURE_BLE_GENERATION_CFM, NULL},
    {ChargerCase_AdaptiveLatencyInit, 0, NULL},
    {ChargerCaseSmInit,     0, NULL},   // Must be after Ui_Init, Dfu_EarlyInit, ConManagerInit (registers as a client)
                                        // Also after appAvInit, UsbDevice_Init, ChargerCase_AudioInit

    {ChargerCase_RegisterForBtMessages, 0, NULL},


};

static bool finalPowerOffStep(Task task)
{
    UNUSED(task);
    appPowerDoPowerOff();

    return TRUE;
}

static const system_state_step_t shutdown_table[] =
{
    {finalPowerOffStep, 0, NULL}
};

static void chargerCaseInit_SetMessageBrokerRegistrations(void)
{
    unsigned registrations_array_dim = (unsigned)message_broker_group_registrations_end -
                              (unsigned)message_broker_group_registrations_begin;
    PanicFalse((registrations_array_dim % sizeof(message_broker_group_registration_t)) == 0);
    registrations_array_dim /= sizeof(message_broker_group_registration_t);
    MessageBroker_Init(message_broker_group_registrations_begin,
                       registrations_array_dim);
}

void ChargerCaseInit_StartInitialisation(void)
{
    DEBUG_LOG_INFO("ChargerCaseInit_StartInitialisation: ChargerCase Application Initialising");
    ChargerCase_StartBtInit();

    chargerCaseInit_SetMessageBrokerRegistrations();

    LedManager_SetHwConfig(&charger_case_led_config);

    ChargerCase_SetBundlesConfig();

    SystemState_Init();
    SystemState_RemoveLimboState();

    SystemState_RegisterForStateChanges(appGetAppTask());
    SystemState_RegisterTableForInitialise(charger_caseInitTable, ARRAY_DIM(charger_caseInitTable));
    SystemState_RegisterTableForShutdown(shutdown_table, ARRAY_DIM(shutdown_table));
    SystemState_RegisterTableForEmergencyShutdown(shutdown_table, ARRAY_DIM(shutdown_table));

    SystemState_Initialise();
}

/*! \brief Initialize charger_case UI */
static void chargerCase_Init_CompleteUiInitialisation(void)
{
    const ui_config_table_content_t* config_table;
    unsigned config_table_size;

    config_table = ChargerCaseUi_GetConfigTable(&config_table_size);

    Ui_SetConfigurationTable(config_table, config_table_size);

    /* UI and App is completely initialized, system is ready for inputs */
    PioMonitorEnable();
}


void ChargerCaseInit_CompleteInitialisation(void)
{
    DEBUG_LOG_FN_ENTRY("ChargerCaseInit_CompleteInitialisation");

    chargerCase_Init_CompleteUiInitialisation();

    /* complete power manager initialisation*/
    appPowerInitComplete();

    ChargerCase_SetupUnexpectedMessage();

    ChargerCase_SetupAudio();

    SystemState_StartUp();
	
    DEBUG_LOG_INFO("ChargerCaseInit_CompleteInitialisation:Completed Initialisation");
}

#endif /* CHARGER_CASE_INIT_C */
