/*!
    \copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \ingroup    gatt_server_gatt
    \brief      Implementation of the GATT Server module.
*/

#include "gatt_server_gatt.h"

#include "gatt_handler.h"
#include "gatt_handler_db_if.h"
#include "gatt_connect.h"
#include "bt_device.h"
#include "device_properties.h"

#include <logging.h>
#include <bdaddr.h>
#include <device_list.h>
#include <gatt.h>

#include <panic.h>
#include <stdlib.h>
#ifdef USE_SYNERGY
#include "gatt_lib.h"
#endif

gattServerGattData gatt_server_gatt = {0};

#ifdef USE_SYNERGY
static void gattServerGatt_RegisterCfm(const CsrBtGattRegisterCfm *cfm);
static void gattServerGatt_EventSendCfm(const CsrBtGattEventSendCfm *cfm);
static void gattServerGatt_handleGattPrim(Message message);
static void gattServerGatt_SendServiceChangedInd(gatt_cid_t cid);
#endif
static void gattServerGatt_AddClient(gatt_cid_t cid);
static void gattServerGatt_RemoveClient(gatt_cid_t cid);

static const gatt_connect_observer_callback_t gatt_server_connect_observer_callback =
{
    .OnConnection = gattServerGatt_AddClient,
    .OnDisconnection = gattServerGatt_RemoveClient
};

static gattServerGattClientData *gattServerGatt_GetClientDataByCid(gatt_cid_t cid)
{
    uint16 client_count = 0;
    gattServerGattClientData *client_data = NULL;

    for (client_count = 0; client_count < NUMBER_GATT_CLIENTS; client_count++)
    {
        if (gatt_server_gatt.client_data[client_count].cid == cid)
        {
            client_data = &gatt_server_gatt.client_data[client_count];
        }
    }

    return client_data;
}

static bool gattServerGatt_ReadClientConfigFromStore(gatt_cid_t cid, uint16 *config)
{
    uint16 stored_client_config = 0;
    bdaddr public_addr;
    bool client_config_read = FALSE;

    if (GattHandler_GetPublicAddressFromConnectId(gattConnect_GetCid(cid), &public_addr))
    {
        client_config_read = appDeviceGetGattServerConfig(&public_addr, &stored_client_config);
        DEBUG_LOG("gattServerGatt_ReadClientConfigFromStore. Read persistent store, read=[%d] config=[0x%x]", client_config_read, stored_client_config);
        DEBUG_LOG("  cid=[0x%x] addr=[%x:%x:%x]", cid, public_addr.nap, public_addr.uap, public_addr.lap);
        
        if (client_config_read)
        {
            *config = stored_client_config;
        }
    }

    return client_config_read;
}

#ifndef USE_SYNERGY
static void gattServerGatt_WriteClientConfigToStore(gatt_cid_t cid, uint16 client_config)
{
    bdaddr public_addr;

    UNUSED(client_config);

    if (GattHandler_GetPublicAddressFromConnectId(gattConnect_GetCid(cid), &public_addr))
    {
        DEBUG_LOG("gattServerGatt_WriteClientConfigToStore. Found public addr");
        DEBUG_LOG("  cid=[0x%x] addr=[%x:%x:%x]", cid, public_addr.nap, public_addr.uap, public_addr.lap);
        appDeviceSetGattServerConfig(&public_addr, client_config);
    }
    else
    {
        DEBUG_LOG("gattServerGatt_WriteClientConfigToStore. Not persistent data");
    }
}

static void gattServerGatt_ReadClientConfig(const GATT_SERVER_READ_CLIENT_CONFIG_IND_T * ind)
{
    uint16 client_config = 0;
    gattServerGattClientData *client_data = NULL;

    /* Return the current value of the client configuration descriptor for the device */
    DEBUG_LOG("gattServerGatt_ReadClientConfig cid=[0x%x]", ind->cid);

    client_data = gattServerGatt_GetClientDataByCid(ind->cid);
    if (client_data != NULL)
    {
        client_config = client_data->config;
    }

    GattServerReadClientConfigResponse(GetGattServerGattGgatts(), ind->cid, ind->handle, client_config);
    DEBUG_LOG("  client_config=[0x%x]\n", client_config);
}

static void gattServerGatt_WriteClientConfig(const GATT_SERVER_WRITE_CLIENT_CONFIG_IND_T * ind)
{
    gattServerGattClientData *client_data = NULL;

    /* Return the current value of the client configuration descriptor for the device */
    DEBUG_LOG("gattServerGatt_WriteClientConfig cid=[0x%x] value=[0x%x]\n",
        ind->cid, ind->config_value);

    client_data = gattServerGatt_GetClientDataByCid(ind->cid);
    if (client_data != NULL)
    {
        client_data->config = ind->config_value;

        /* Write client config to persistent store */
        gattServerGatt_WriteClientConfigToStore(ind->cid, client_data->config);
    }
}
#endif

static void gattServerGatt_ResetServicesChangedInStore(gatt_cid_t cid)
{
    bdaddr public_addr;

    if (GattHandler_GetPublicAddressFromConnectId(gattConnect_GetCid(cid), &public_addr))
    {
        DEBUG_LOG("gattServerGatt_ResetServicesChangedInStore. Found public addr");
        appDeviceSetGattServerServicesChanged(&public_addr, 0);
    }
    else
    {
        DEBUG_LOG("gattServerGatt_ResetServicesChangedInStore. Not persistent data");
    }
}

static bool gattServerGatt_ReadServicesChangedFromStore(gatt_cid_t cid, uint8 *value)
{
    uint8 stored_service_changed = 0;
    bdaddr public_addr;
    bool config_read = FALSE;

    if (GattHandler_GetPublicAddressFromConnectId(gattConnect_GetCid(cid), &public_addr))
    {
        config_read = appDeviceGetGattServerServicesChanged(&public_addr, &stored_service_changed);
        DEBUG_LOG("gattServerGatt_ReadServicesChangedFromStore. Read persistent store, read=[%d] value=[0x%x]", config_read, stored_service_changed);

        if (config_read)
        {
            *value = stored_service_changed;
        }
    }

    return config_read;
}

static void gattServerGatt_MessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    DEBUG_LOG("gattServerGatt_MessageHandler MESSAGE:0x%x", id);

    switch (id)
    {
#ifdef USE_SYNERGY
        case GATT_PRIM:
            gattServerGatt_handleGattPrim(message);
            break;
#else
        case GATT_SERVER_READ_CLIENT_CONFIG_IND:
            gattServerGatt_ReadClientConfig((const GATT_SERVER_READ_CLIENT_CONFIG_IND_T *) message);
            break;

        case GATT_SERVER_WRITE_CLIENT_CONFIG_IND:
            gattServerGatt_WriteClientConfig((const GATT_SERVER_WRITE_CLIENT_CONFIG_IND_T *) message);
            break;

        case GATT_SERVER_SERVICE_CHANGED_INDICATION_CFM:
            break;
#endif

        default:
            DEBUG_LOG("gattServerGatt_MessageHandler. Unhandled message MESSAGE:0x%x", id);
            break;
    }
}

static void gattServerGatt_init(void)
{
    gatt_server_gatt.gatt_task.handler = gattServerGatt_MessageHandler;

#ifdef USE_SYNERGY
    GattRegisterReqSend(GetGattServerGattTask(), 0);
#else
    if (GattServerInit(GetGattServerGattGgatts(),
                        GetGattServerGattTask(),
                        HANDLE_GATT_SERVICE,
                        HANDLE_GATT_SERVICE_END) != gatt_server_status_success)
    {
        DEBUG_LOG("gattServerGatt_init Server failed");
        Panic();
    }
#endif /* USE_SYNERGY */

    GattConnect_RegisterObserver(&gatt_server_connect_observer_callback);
}

static void gattServerGatt_IndicateServicesChangedByCid(gatt_cid_t cid)
{
    uint8 stored_service_changed = 0;

    if (gattServerGatt_ReadServicesChangedFromStore(cid, &stored_service_changed))
    {
        if (stored_service_changed != 0)
        {
            DEBUG_LOG("gattServerGatt_IndicateServicesChangedByCid. cid=[0x%x]", cid);

#ifdef USE_SYNERGY
            gattServerGatt_SendServiceChangedInd(cid);
#else
            GattServerSendServiceChangedIndication(GetGattServerGattGgatts(), cid);
#endif /* USE_SYNERGY */
            gattServerGatt_ResetServicesChangedInStore(cid);
        }
    }
}

static void gattServerGatt_IndicateServicesChanged(void)
{
    uint16 client_count = 0;

    for (client_count = 0; client_count < NUMBER_GATT_CLIENTS; client_count++)
    {
        if (gatt_server_gatt.client_data[client_count].cid)
        {
            DEBUG_LOG("gattServerGatt_IndicateServicesChanged. cid=[0x%x]", 
                        gatt_server_gatt.client_data[client_count].cid);
            gattServerGatt_IndicateServicesChangedByCid(
                        gatt_server_gatt.client_data[client_count].cid);
        }
    }
}

static void gattServerGatt_AddClient(gatt_cid_t cid)
{
    uint16 stored_client_config = 0;
    gattServerGattClientData *client_data = NULL;
    
    PanicNull(GetGattServerGattTask());
    
    client_data = gattServerGatt_GetClientDataByCid(0);

    if (client_data != NULL)
    {
        client_data->cid = cid;
        /* Read client config from persistent store */
        if (gattServerGatt_ReadClientConfigFromStore(cid, &stored_client_config))
        {
            client_data->config = stored_client_config;
        }

        /* Inform of any service changes on connection */
        gattServerGatt_IndicateServicesChangedByCid(cid);
    }
}


static void gattServerGatt_RemoveClient(gatt_cid_t cid)
{
    gattServerGattClientData *client_data = NULL;
    
    PanicNull(GetGattServerGattTask());
    
    client_data = gattServerGatt_GetClientDataByCid(cid);
    
    if (client_data != NULL)
    {
        client_data->cid = 0;
        client_data->config = 0;
    }
}


/*****************************************************************************/
bool GattServerGatt_Init(Task init_task)
{
    UNUSED(init_task);

    gattServerGatt_init();
    
    DEBUG_LOG("GattServerGatt_Init. Server initialised");

    return TRUE;
}


void GattServerGatt_SetGattDbChanged(void)
{
    device_t* devices = NULL;
    unsigned num_devices = 0;
    uint16 property_value = 0x2;
    bdaddr addr;

    DEBUG_LOG("GattServerGatt_SetGattDbChanged");
    
    DeviceList_GetAllDevicesWithPropertyValue(device_property_gatt_server_config, &property_value, sizeof(uint16), &devices, &num_devices);
    if (devices && num_devices)
    {
        DEBUG_LOG("GattServerGatt_SetGattDbChanged num_devices=[0x%x]", num_devices);
        for (int i=0; i< num_devices; i++)
        {
            addr = DeviceProperties_GetBdAddr(devices[i]);
            DEBUG_LOG("GattServerGatt_SetGattDbChanged addr=[%x:%x:%x]", addr.uap, addr.lap, addr.nap);
            appDeviceSetGattServerServicesChanged(&addr, 1);
        }
        
        /* Indicate changes to any connected clients */
        gattServerGatt_IndicateServicesChanged();
    }
    free(devices);
    devices = NULL;
}

void GattServerGatt_SetServerServicesChanged(gatt_cid_t cid)
{
    bdaddr bd_addr;

    DEBUG_LOG("GattServerGatt_SetServerServicesChanged");

    if (GattHandler_GetPublicAddressFromConnectId(gattConnect_GetCid(cid), &bd_addr))
    {
        DEBUG_LOG("\tbdaddr nap %x uap %x lap %x", bd_addr.nap, bd_addr.uap, bd_addr.lap);
        if(!appDeviceIsPeer(&bd_addr))
        {
            if(appDeviceSetGattServerServicesChanged(&bd_addr, 1))
            {
                DEBUG_LOG("GattServerGatt_SetServerServicesChanged: appDeviceSetGattServerServicesChanged success");
                gattServerGatt_IndicateServicesChangedByCid(cid);
            }
            else
            {
                DEBUG_LOG("GattServerGatt_SetServerServicesChanged: appDeviceSetGattServerServicesChanged FAILED");
            }
        }
    }
}

#ifdef USE_SYNERGY
static void gattServerGatt_RegisterCfm(const CsrBtGattRegisterCfm *cfm)
{
    DEBUG_LOG("gattServerGatt_RegisterCfm: resultCode(0x%04x) resultSupplier(0x%04x) ", cfm->resultCode, cfm->resultSupplier);

    if (cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS &&
        cfm->resultSupplier == CSR_BT_SUPPLIER_GATT)
    {
		/* Not required to call GattFlatDbRegisterReqSend() as it does not deal with specific service DB.
		   Absolute value in Synergy lib for Service Changed Handle is same as HANDLE_GATT_SERVICE_CHANGED.
		*/
        gatt_server_gatt.gattId = cfm->gattId;

    }
    else
    {
        DEBUG_LOG("gattServerGatt_RegisterCfm: Gatt Registration failed");
        Panic();
    }
}

static void gattServerGatt_EventSendCfm(const CsrBtGattEventSendCfm *cfm)
{
    DEBUG_LOG("gattServerGatt_EventSendCfm: resultCode(0x%04x) resultSupplier(0x%04x) ", cfm->resultCode, cfm->resultSupplier);
}

static void gattServerGatt_handleGattPrim(Message message)
{
    CsrBtGattPrim *prim = (CsrBtGattPrim *)message;

    switch (*prim)
    {
        case CSR_BT_GATT_REGISTER_CFM:
            gattServerGatt_RegisterCfm((const CsrBtGattRegisterCfm*) message);
            break;

        case CSR_BT_GATT_EVENT_SEND_CFM:
            gattServerGatt_EventSendCfm((const CsrBtGattEventSendCfm*) message);
            break;

        default:
            DEBUG_LOG("gattServerGatt_handleGattPrim. Unhandled message MESSAGE:0x%04x", *prim);
            break;
    }
    GattFreeUpstreamMessageContents((void *)message);
}

static void gattServerGatt_SendServiceChangedInd(gatt_cid_t cid)
{
    CsrUint8 *value = CsrPmemAlloc(sizeof(CsrUint8) * 4);
    /* Service changed, set the "changed range" to the full database */
    value[0] = value[1] = 0x00;
    value[2] = value[3] = 0xFF;

    GattIndicationEventReqSend(gatt_server_gatt.gattId,
                               cid,
                               HANDLE_GATT_SERVICE_CHANGED,
                               4,
                               value);
}
#endif /* USE_SYNERGY */

