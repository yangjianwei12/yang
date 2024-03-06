/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    handset_service
    \brief      The Persistent Device Data User (PDDU) implementation for the Handset Service
*/
#include "handset_service_pddu.h"
#include "handset_service_config.h"
#include "handset_service.h"

#include <bt_device.h>
#include <csrtypes.h>
#include <device.h>
#include <device_db_serialiser.h>
#include <device_properties.h>
#include <logging.h>
#include <macros.h>
#include <pddu_map.h>

static pdd_size_t handsetServicePddu_GetDeviceDataLen(device_t device)
{
    uint8 size_needed = 0;
    handset_service_config_t *handset_service_data = NULL;
    handset_service_config_v1_t *handset_service_data_v1 = NULL;
    size_t handset_service_size = 0;

    if (device != NULL)
    {
        bool table_exists = FALSE;

        table_exists = Device_GetProperty(device, device_property_handset_service_config,
                                          (void *)&handset_service_data, &handset_service_size) ||
                       Device_GetProperty(device, device_property_v1_handset_service_config,
                                          (void *)&handset_service_data_v1, &handset_service_size);

        /* Regardless of which property version exists, we always want to write the latest version,
         * i.e. handset_service_config_t. Any old existing properties will be converted soon to the latest version. */
        if (table_exists)
        {
            size_needed = sizeof(handset_service_config_t);
        }
        else if (device == BtDevice_GetSelfDevice())
        {
            /* The property has never been written -- always fill in a valid default */
            size_needed = sizeof(handset_service_config_t);
        }

        DEBUG_LOG_INFO("handsetServicePddu_GetDeviceDataLen exists=%d,size=%d",
                       table_exists, size_needed);
    }
    return size_needed;
}

static void handsetServicePddu_SerialisePersistentDeviceData(device_t device, void *buf, pdd_size_t offset)
{
    UNUSED(offset);

    handset_service_config_t *handset_service_config = NULL;
    handset_service_config_v1_t *handset_service_config_v1 = NULL;
    size_t handset_service_config_size = 0;
    uint8 * buffer = buf;

    if (Device_GetProperty(device, device_property_handset_service_config, (void *)&handset_service_config, &handset_service_config_size))
    {
        DEBUG_LOG_INFO("handsetServicePddu_SerialisePersistentDeviceData serialise table size=%d", handset_service_config_size);

        /* store the config data to the caller's buffer */
        memcpy(buffer, handset_service_config, handset_service_config_size);
    }
    else if (Device_GetProperty(device, device_property_v1_handset_service_config, (void *)&handset_service_config_v1, &handset_service_config_size))
    {
        DEBUG_LOG_INFO("handsetServicePddu_SerialisePersistentDeviceData (v1) serialise table size=%d", handset_service_config_size);

        /* Convert the old property to the format of the current/latest one */
        handset_service_config_t pdd = { 0 };

        pdd.acl_connect_attempt_limit = handset_service_config_v1->acl_connect_attempt_limit;
        pdd.enable_connection_barge_in = handset_service_config_v1->enable_connection_barge_in;
        pdd.enable_unlimited_acl_reconnection = handset_service_config_v1->enable_unlimited_acl_reconnection;
        pdd.max_bredr_connections = handset_service_config_v1->max_bredr_connections;
        pdd.unlimited_reconnection_page_interval_ms = HandsetService_CalculatePageIntervalMsValue(handset_service_config_v1->page_interval);
        pdd.initial_page_timeout_slots = handset_service_config_v1->page_timeout;
        pdd.initial_page_interval_ms = handsetService_BredrAclConnectRetryDelayMs();

        /* store the config data to the caller's buffer */
        memcpy(buffer, &pdd, sizeof(handset_service_config_t));
    }
    else if (device == BtDevice_GetSelfDevice())
    {
        DEBUG_LOG_INFO("handsetServicePddu_SerialisePersistentDeviceData default table size=%d", sizeof(handset_service_config_t));

        HandsetService_SetDefaultConfig(buffer, sizeof(handset_service_config_t));
    }
}

static void handsetServicePddu_DeserialisePersistentDeviceData(device_t device, void *buf, pdd_size_t data_length, pdd_size_t offset)
{
    UNUSED(offset);

    uint8 * buffer = buf;

    DEBUG_LOG_VERBOSE("handsetServicePddu_DeserialisePersistentDeviceData data_length=%d", data_length);

    Device_SetProperty(device, device_property_handset_service_config, buffer, data_length);
}

void HandsetServicePddu_RegisterPdduInternal(void)
{
    DeviceDbSerialiser_RegisterPersistentDeviceDataUser(
        PDDU_ID_HANDSET_SERVICE,
        handsetServicePddu_GetDeviceDataLen,
        handsetServicePddu_SerialisePersistentDeviceData,
        handsetServicePddu_DeserialisePersistentDeviceData);
}

void HandsetServicePddu_CheckToImportBtDeviceHandsetServiceConfig(void)
{
    device_t device = BtDevice_GetSelfDevice();
    if (device)
    {
        /* Only create the Handset Serivce PDD if it doesn't exist when deserialising the device data, and the
           BT Device PDD version does exist, if neither exist, this represents a factory reset, do nothing. */
        if (!Device_IsPropertySet(device, device_property_handset_service_config))
        {
            handset_service_config_v1_t *config;
            size_t size;
            if (Device_GetProperty(device, device_property_v1_handset_service_config, (void **)&config, &size))
            {
                PanicFalse(size == sizeof(handset_service_config_v1_t));

                handset_service_config_t pdd = { 0 };

                pdd.acl_connect_attempt_limit = config->acl_connect_attempt_limit;
                pdd.enable_connection_barge_in = config->enable_connection_barge_in;
                pdd.enable_unlimited_acl_reconnection = config->enable_unlimited_acl_reconnection;
                pdd.max_bredr_connections = config->max_bredr_connections;
                pdd.unlimited_reconnection_page_interval_ms = HandsetService_CalculatePageIntervalMsValue(config->page_interval);
                pdd.unlimited_reconnection_page_timeout_slots = config->page_timeout;
                pdd.initial_page_interval_ms = handsetService_BredrAclConnectRetryDelayMs();
                pdd.initial_page_timeout_slots = config->page_timeout;
                pdd.max_le_connections = config->max_le_connections;

                PanicFalse(Device_SetProperty(device, device_property_handset_service_config, (void *)&pdd, sizeof(handset_service_config_t)));
            }
        }
    }
}
