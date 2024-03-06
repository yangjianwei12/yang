/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    csip
    \brief
*/

#include "csip_set_member.h"
#include "csip_set_member_advertising.h"

#include <gatt_csis_server.h>
#include <logging.h>
#include <panic.h>
#include <util.h>

#include "bt_device.h"
#include "device_db_serialiser.h"
#include "device_properties.h"
#include "gatt_connect.h"
#include "gatt_handler_db_if.h"
#include "multidevice.h"
#include "pddu_map.h"
#include "gatt.h"

#define CSIP_SET_MEMBER_LOG      DEBUG_LOG

#define CSIS_INVALID_HANDLE     0

static ServiceHandle csis_instance = CSIS_INVALID_HANDLE;

static void csipSetMember_MessageHandler(Task task, MessageId id, Message message);
static const TaskData csip_task = { .handler = csipSetMember_MessageHandler };


static void csipSetMember_OnGattConnect(gatt_cid_t cid);
static void csipSetMember_OnGattDisconnect(gatt_cid_t cid);

static const gatt_connect_observer_callback_t le_connect_callbacks =
{
    .OnConnection = csipSetMember_OnGattConnect,
    .OnDisconnection = csipSetMember_OnGattDisconnect
};

static inline ServiceHandle CsipSetMember_GetCsisInstance(void)
{
    PanicZero(csis_instance);
    return csis_instance;
}

static void csipSetMember_MessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);
    if(id == GATT_CSIS_LOCK_STATE_IND)
    {
        CSIP_SET_MEMBER_LOG("csipSetMember_MessageHandler GATT_CSIS_LOCK_STATE_IND");
    }
    else
    {
        CSIP_SET_MEMBER_LOG("csipSetMember_MessageHandler unhandled id=0x%x", id);
    }
}

static void * csipSetMember_RetrieveClientConfig(uint16 cid)
{
    device_t device = GattConnect_GetBtDevice(cid);
    void * device_config = NULL;
    if(device)
    {
        size_t size;
        CSIP_SET_MEMBER_LOG("csipSetMember_RetrieveClientConfig retrieving config device=0x%p", device);
        if (!Device_GetProperty(device, device_property_le_audio_csip_config, &device_config, &size))
        {
            device_config = NULL;
        }
    }
    return device_config;
}

static void csipSetMember_StoreClientConfig(uint16 cid, void * config, uint8 size)
{
    device_t device = GattConnect_GetBtDevice(cid);
    if(device)
    {
        CSIP_SET_MEMBER_LOG("csipSetMember_StoreClientConfig storing config device=0x%p", device);
        Device_SetProperty(device, device_property_le_audio_csip_config, config, size);
    }
}

static void csipSetMember_OnGattConnect(gatt_cid_t cid)
{
    GattCsisServerConfigType *config = NULL;

    CSIP_SET_MEMBER_LOG("csipSetMember_OnGattConnect instance 0x%x cid %u", CsipSetMember_GetCsisInstance(), cid);

    config = csipSetMember_RetrieveClientConfig(cid);

    gatt_status_t status = GattCsisServerAddConfig(CsipSetMember_GetCsisInstance(), cid, config);
    if (status != gatt_status_success)
    {
        CSIP_SET_MEMBER_LOG("csipSetMember_OnGattConnect add config failed status=%d", status);
        Panic();
    }
}

static void csipSetMember_OnGattDisconnect(gatt_cid_t cid)
{
    CSIP_SET_MEMBER_LOG("csipSetMember_OnGattDisconnect cid %u", cid);

    GattCsisServerConfigType *config = GattCsisServerRemoveConfig(CsipSetMember_GetCsisInstance(), cid);
    if (config)
    {
        csipSetMember_StoreClientConfig(cid, config, sizeof(*config));
        free(config);
    }
}

static pdd_size_t csipSetMember_GetDeviceDataLength(device_t device)
{
    void * config = NULL;
    size_t config_size = 0;

    if(Device_GetProperty(device, device_property_le_audio_csip_config, &config, &config_size) == FALSE)
    {
        config_size = 0;
    }
    return config_size;
}

static void csipSetMember_SerialisetDeviceData(device_t device, void *buf, pdd_size_t offset)
{
    UNUSED(offset);
    void * config = NULL;
    size_t config_size = 0;

    if(Device_GetProperty(device, device_property_le_audio_csip_config, &config, &config_size))
    {
        memcpy(buf, config, config_size);
    }
}

static void csipSetMember_DeserialisetDeviceData(device_t device, void *buf, pdd_size_t data_length, pdd_size_t offset)
{
    UNUSED(offset);
    Device_SetProperty(device, device_property_le_audio_csip_config, buf, data_length);
}

static void csipSetMember_RegisterAsPersistentDeviceDataUser(void)
{
    DeviceDbSerialiser_RegisterPersistentDeviceDataUser(
        PDDU_ID_CSIP_SET_MEMBER,
        csipSetMember_GetDeviceDataLength,
        csipSetMember_SerialisetDeviceData,
        csipSetMember_DeserialisetDeviceData);
}

static void csipSetMember_CreateSirk(uint8 *sirk)
{
    for(uint8 i=0; i < SIZE_SIRK_KEY; i++)
    {
        if(!Multidevice_IsDeviceStereo())
        {
            /* temporary fixed SIRK for testing */
            /* SIRK to be updated when earbuds connect to each other when lib API becomes available */
            sirk[i] = i;
        }
        else
        {
            sirk[i] = (uint8)UtilRandom();
        }
    }
}

static void csipSetMember_SetupParams(GattCsisServerInitParamType *params)
{
    if(!Multidevice_IsDeviceStereo())
    {
        params->csSize = 2;
        if(Multidevice_GetSide() == multidevice_side_left)
        {
            params->rank = 1;
        }
        else
        {
            params->rank = 2;
        }
    }
    else
    {
        params->csSize = 1;
        params->rank = 1;
    }

    csipSetMember_CreateSirk(params->sirk);
    params->flags = GATT_CSIS_SHARE_SIRK_PLAIN_TEXT;
}

bool CsipSetMember_Init(Task init_task)
{
    UNUSED(init_task);
    CSIP_SET_MEMBER_LOG("CsipSetMember_Init : ");
    GattCsisServerInitParamType csis_params;
    memset(&csis_params, 0, sizeof(csis_params));
    csipSetMember_SetupParams(&csis_params);

#ifdef USE_SYNERGY
    csis_instance = GattCsisServerInit(TrapToOxygenTask((Task)&csip_task),
                                        &csis_params,
                                        HANDLE_COORDINATED_SET_IDENTIFICATION_SERVICE_N(1),
                                        HANDLE_COORDINATED_SET_IDENTIFICATION_SERVICE_END_N(1));
#else
    csis_instance = GattCsisServerInit((Task)&csip_task,
                                        &csis_params,
                                        HANDLE_COORDINATED_SET_IDENTIFICATION_SERVICE_N(1),
                                        HANDLE_COORDINATED_SET_IDENTIFICATION_SERVICE_END_N(1));
#endif
    PanicZero(csis_instance);

    csipSetMember_RegisterAsPersistentDeviceDataUser();

    GattConnect_RegisterObserver(&le_connect_callbacks);

    CsipSetMember_SetupLeAdvertisingData(GattCsisServerGetRsi(csis_instance));

    return TRUE;
}

void CsipSetMember_SetSirkKey(uint8 *sirk_key)
{
    GattCsisServerInitParamType csis_params;
    if (csis_instance != CSIS_INVALID_HANDLE)
    {
        memset(&csis_params, 0, sizeof(csis_params));
        csipSetMember_SetupParams(&csis_params);
        /* Update init parameters with the Sirk Key */
        memcpy(csis_params.sirk, sirk_key, SIZE_SIRK_KEY);

        GattCsisServerUpdateInitParam(csis_instance, &csis_params);
        CsipSetMember_UpdatePsri(GattCsisServerGetRsi(csis_instance));
    }
}

#ifdef HOSTED_TEST_ENVIRONMENT

const TaskData * CsipSetMember_GetCsisMessageHandler(void)
{
    return &csip_task;
}

#endif

