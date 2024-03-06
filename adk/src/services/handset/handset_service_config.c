/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    handset_service
    \brief      Handset service config
*/

#include "handset_service_config.h"
#include "handset_service_pddu.h"
#include "handset_service_protected.h"
#include "handset_service.h"

#include <connection_manager.h>
#include <device_properties.h>
#include <device_db_serialiser.h>

#if defined(INCLUDE_GAIA)
#include "handset_service_gaia_plugin.h"
#endif
#include "context_framework.h"

#include <device_list.h>

#include <panic.h>

#if defined(ENABLE_CONNECTION_BARGE_IN) && defined(MULTIPOINT_BARGE_IN_ENABLED)
#error "MULTIPOINT_BARGE_IN_ENABLED incompatible with ENABLE_CONNECTION_BARGE_IN"
#endif

#if defined(ENABLE_LE_MULTIPOINT) && defined(INCLUDE_MIRRORING) && !defined(ENABLE_LE_HANDOVER)
#error "ENABLE_LE_MULTIPOINT requires that LE handover is enabled if mirroring is included"
#endif


#define PAGE_TIMEOUT_IN_MS ((rtime_t) 5120)

const handset_service_config_t handset_service_multipoint_config =
{
    /* Two connections supported */
    .max_bredr_connections = 2,
#ifdef ENABLE_LE_MULTIPOINT
    /* Two LE connections supported */
    .max_le_connections = 2,
#else
    /* Only one LE connection supported */
    .max_le_connections = 1,
#endif
    /* Two ACL reconnection attempts per supported connection */
    .acl_connect_attempt_limit = 2,
    /* Page interval of 500ms */
    .initial_page_interval_ms = handsetService_BredrAclConnectRetryDelayMs(),
    .enable_unlimited_acl_reconnection = FALSE,
    .unlimited_reconnection_page_interval_ms = 30 * MS_PER_SEC,
    .unlimited_reconnection_page_timeout_slots = MS_TO_BT_SLOTS(PAGE_TIMEOUT_IN_MS),
    /* Page timeout 5.12 seconds*/
    .initial_page_timeout_slots = MS_TO_BT_SLOTS(PAGE_TIMEOUT_IN_MS),
#ifdef ENABLE_CONNECTION_BARGE_IN
    .enable_connection_barge_in = TRUE
#else
    .enable_connection_barge_in = FALSE
#endif
};

const handset_service_config_t handset_service_singlepoint_config =
{
    /* One connection supported */
    .max_bredr_connections = 1,
    /* Only one LE connection supported */
    .max_le_connections = 1,
    /* Three ACL reconnection attempts per supported connection */
    .acl_connect_attempt_limit = 3,
    /* Page interval of 500ms */
    .initial_page_interval_ms = handsetService_BredrAclConnectRetryDelayMs(),
    .enable_unlimited_acl_reconnection = FALSE,
    .unlimited_reconnection_page_interval_ms = 30 * MS_PER_SEC,
    .unlimited_reconnection_page_timeout_slots = MS_TO_BT_SLOTS(10 * MS_PER_SEC),
    /* Page timeout 10 seconds*/
    .initial_page_timeout_slots = MS_TO_BT_SLOTS(10 * MS_PER_SEC),
#ifdef ENABLE_CONNECTION_BARGE_IN
    .enable_connection_barge_in = TRUE
#else
    .enable_connection_barge_in = FALSE
#endif
};

static handset_service_config_t *handsetService_GetConfig(void)
{
    device_t device = BtDevice_GetSelfDevice();
    if(device)
    {
        handset_service_config_t *config;
        size_t size;
        if(Device_GetProperty(device, device_property_handset_service_config, (void **)&config, &size))
        {
            PanicFalse(size == sizeof(handset_service_config_t));
            return config;
        }
    }

    return NULL;
}

uint8 handsetService_LeAclMaxConnections(void)
{
    handset_service_config_t *config = handsetService_GetConfig();
    if(config)
    {
        return config->max_le_connections;
    }
    return 1;
}

bool handsetService_IsUnlimitedAclReconnectionEnabled(void)
{
    handset_service_config_t *config = handsetService_GetConfig();
    if(config)
    {
        return config->enable_unlimited_acl_reconnection;
    }
    return FALSE;
}

uint32 HandsetService_CalculatePageIntervalMsValue(uint8 page_interval_config)
{
    return (1 << page_interval_config) * 500;
}

uint32 handsetService_GetInitialReconnectionPageInterval(void)
{
    handset_service_config_t *config = handsetService_GetConfig();
    if(config)
    {
        return config->initial_page_interval_ms;
    }
    return handsetService_BredrAclConnectRetryDelayMs();
}

uint16 handsetService_GetInitialReconnectionPageTimeout(void)
{
    handset_service_config_t *config = handsetService_GetConfig();
    if(config)
    {
        return config->initial_page_timeout_slots;
    }
    return MS_TO_BT_SLOTS(5 * MS_PER_SEC);
}

uint32 handsetService_GetUnlimitedReconnectionPageInterval(void)
{
    handset_service_config_t *config = handsetService_GetConfig();
    if(config)
    {
        return config->unlimited_reconnection_page_interval_ms;
    }
    return handsetService_BredrAclConnectRetryDelayMs();
}

uint16 handsetService_GetUnlimitedReconnectionPageTimeout(void)
{
    handset_service_config_t *config = handsetService_GetConfig();
    if(config)
    {
        return config->unlimited_reconnection_page_timeout_slots;
    }
    return MS_TO_BT_SLOTS(5 * MS_PER_SEC);
}

uint8 handsetService_BredrAclConnectAttemptLimit(void)
{
    handset_service_config_t *config = handsetService_GetConfig();
    if(config)
    {
        return config->acl_connect_attempt_limit;
    }

    return 1;
}

uint8 handsetService_BredrAclMaxConnections(void)
{
    handset_service_config_t *config = handsetService_GetConfig();
    if(config)
    {
        return config->max_bredr_connections;
    }

    return 1;
}

bool handsetService_IsConnectionBargeInEnabled(void)
{
    handset_service_config_t *config = handsetService_GetConfig();
    if(config)
    {
        return config->enable_connection_barge_in;
    }

    return FALSE;
}

static bool handsetService_MaxBredrAclContextCallback(unsigned * context_data, uint8 context_data_size)
{
    PanicZero(context_data_size >= sizeof(context_maximum_connected_handsets_t));
    memset(context_data, 0, sizeof(context_maximum_connected_handsets_t));
    *(context_maximum_connected_handsets_t *)context_data = handsetService_BredrAclMaxConnections();
    return TRUE;
}

bool handsetService_SetConnectionBargeInEnable(bool enabled)
{
    handset_service_config_t *config = handsetService_GetConfig();
    if(config)
    {
        if(config->enable_connection_barge_in == enabled)
        {
            return TRUE;
        }

        handset_service_config_t new_config;
        memcpy(&new_config, config, sizeof(handset_service_config_t));
        new_config.enable_connection_barge_in = enabled;
        return HandsetService_Configure(new_config);
    }
    return FALSE;
}

void HandsetServiceConfig_Init(void)
{
    HandsetServicePddu_CheckToImportBtDeviceHandsetServiceConfig();

    handsetService_HandleConfigUpdate();
    ContextFramework_RegisterContextProvider(context_maximum_connected_handsets, handsetService_MaxBredrAclContextCallback);

    /* Handle situation when the SELF device is already created, but device_property_handset_service_config wasn't yet set.
       That is expected on first boot of non-earbud applications.
    */
    device_t device = BtDevice_GetSelfDevice();
    if(device)
    {
        if(!Device_IsPropertySet(device, device_property_handset_service_config))
        {
            handsetService_HandleSelfCreated();
        }
    }
}

bool HandsetService_Configure(handset_service_config_t config)
{
    if(config.max_bredr_connections > HANDSET_SERVICE_MAX_PERMITTED_BREDR_CONNECTIONS)
    {
        return FALSE;
    }

    if(config.max_bredr_connections < 1)
    {
        return FALSE;
    }
    
#ifdef MULTIPOINT_BARGE_IN_ENABLED
    /* Connection barge-in is not compatible with the legacy version. 
       Attempts to enable connection barge-in when MULTIPOINT_BARGE_IN_ENABLED
       will fail. MULTIPOINT_BARGE_IN_ENABLED should only be used where truncated 
       page scan is not supported. */
    if(config.enable_connection_barge_in)
    {
        return FALSE;
    }
#endif

    ConManager_SetPageTimeout(config.initial_page_timeout_slots);

    device_t device = BtDevice_GetSelfDevice();
    if(device)
    {
        Device_SetProperty(device, device_property_handset_service_config, &config, sizeof(config));
        DeviceDbSerialiser_SerialiseDevice(device);
    }

#if defined(INCLUDE_GAIA)
    if(config.max_bredr_connections > 1)
    {
        HandsetServicegGaiaPlugin_MultipointEnabledChanged(TRUE);
    }
    else
    {
        HandsetServicegGaiaPlugin_MultipointEnabledChanged(FALSE);
    }
#endif /* INCLUDE_GAIA */

    return TRUE;
}

void HandsetService_SetDefaultConfig(void *value, uint8 size)
{
    PanicFalse(size == sizeof(handset_service_config_t));
#ifdef ENABLE_MULTIPOINT
    memcpy(value, &handset_service_multipoint_config, size);
#else
    memcpy(value, &handset_service_singlepoint_config, size);
#endif
}

void handsetService_HandleSelfCreated(void)
{
#ifdef ENABLE_MULTIPOINT
    HandsetService_Configure(handset_service_multipoint_config);
#else
    HandsetService_Configure(handset_service_singlepoint_config);
#endif

    device_t device = BtDevice_GetSelfDevice();
    if(device)
    {
        DeviceDbSerialiser_SerialiseDevice(device);
    }
}

void handsetService_HandleConfigUpdate(void)
{
    handset_service_config_t *config = handsetService_GetConfig();
    if(config)
    {
        ConManager_SetPageTimeout(config->initial_page_timeout_slots);
        ContextFramework_NotifyContextUpdate(context_maximum_connected_handsets);
    }
}

void HandsetService_ConfigureLinkLossReconnectionParameters(
        bool use_unlimited_reconnection_attempts,
        uint8 num_connection_attempts,
        uint32 initial_reconnection_page_interval_ms,
        uint16 initial_reconnection_page_timeout_ms,
        uint32 unlimited_reconnection_page_interval_ms,
        uint16 unlimited_reconnection_page_timeout_ms)
{
    handset_service_config_t *config = handsetService_GetConfig();
    if(config)
    {
        bool is_config_updated = FALSE;
        handset_service_config_t new_config;
        memcpy(&new_config, config, sizeof(handset_service_config_t));

        /* Guard against overflow errors as MessageSendLater converts the page interval to a time in
           microseconds, and this can cause very large page intervals to overflow a uint32. */
        initial_reconnection_page_interval_ms = MIN(initial_reconnection_page_interval_ms, ULONG_MAX/US_PER_MS);
        unlimited_reconnection_page_interval_ms = MIN(unlimited_reconnection_page_interval_ms, ULONG_MAX/US_PER_MS);

        if(config->acl_connect_attempt_limit != num_connection_attempts)
        {
            new_config.acl_connect_attempt_limit = num_connection_attempts;
            is_config_updated = TRUE;
        }

        /* For page timeouts, convert to slots and limit to max number of 16 bit slots */
        uint32 page_timeout_bt_slots = MS_TO_BT_SLOTS(unlimited_reconnection_page_timeout_ms);
        page_timeout_bt_slots = MIN(USHRT_MAX, page_timeout_bt_slots);
        if(config->unlimited_reconnection_page_timeout_slots != page_timeout_bt_slots)
        {
            new_config.unlimited_reconnection_page_timeout_slots = page_timeout_bt_slots;
            is_config_updated = TRUE;
        }

        if(config->unlimited_reconnection_page_interval_ms != unlimited_reconnection_page_interval_ms)
        {
            new_config.unlimited_reconnection_page_interval_ms = unlimited_reconnection_page_interval_ms;
            is_config_updated = TRUE;
        }

        page_timeout_bt_slots = MS_TO_BT_SLOTS(initial_reconnection_page_timeout_ms);
        page_timeout_bt_slots = MIN(USHRT_MAX, page_timeout_bt_slots);
        if(config->initial_page_timeout_slots != page_timeout_bt_slots)
        {
            new_config.initial_page_timeout_slots = page_timeout_bt_slots;
            is_config_updated = TRUE;
        }

        if(config->initial_page_interval_ms != initial_reconnection_page_interval_ms)
        {
            new_config.initial_page_interval_ms = initial_reconnection_page_interval_ms;
            is_config_updated = TRUE;
        }

        if(config->enable_unlimited_acl_reconnection != use_unlimited_reconnection_attempts)
        {
            new_config.enable_unlimited_acl_reconnection = use_unlimited_reconnection_attempts;
            is_config_updated = TRUE;
        }

        if (is_config_updated)
        {
            HandsetService_Configure(new_config);
        }
    }
}
