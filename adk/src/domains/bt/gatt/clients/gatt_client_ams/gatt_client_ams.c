/*!
    \copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.
    All Rights Reserved.
    Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \defgroup   gatt_client_ams GATT AMS Client
    @{
    \ingroup    gatt_client_domain
    \brief      Application support for GATT AMS client
*/

#include "gatt_client.h"
#include "gatt_client_protected.h"
#include <gatt_ams_client.h>
#include <panic.h>
#include <logging.h>

static GAMS *ams_data = NULL;

#ifdef USE_SYNERGY
static bool gatt_client_deinit_done = FALSE;
static gatt_client_t *gatt_client_instance = NULL;
#endif

/* Always disabled */
#define DISABLE_REMOTE_CLIENT_NOTIFICATIONS     FALSE
#define DISABLE_ENTITY_UPDATE_NOTIFICATIONS     FALSE

#define DEBUG_GATT_CLIENT_AMS                   "Gatt Client AMS:"


/*! \brief Forward function declarations */
static void GattAmsClient_MsgHandler(Task task, MessageId id, Message message);

#ifdef USE_SYNERGY
static void GattAmsClientDeallocateMemory(gatt_client_t* instance);

/*! \brief Create a AMS gatt client instance */
INIT_CUSTOM_GATT_CLIENT_INSTANCE(gatt_ams_client,
                                 GattClient_Protected_AllocateMemory,
                                 GattAmsClientDeallocateMemory,
                                 GattClient_Protected_GetServiceForDiscovery,
                                 GattClient_Protected_GetDiscoveryStopRequest,
                                 GattClient_Protected_ClientLibInit,
                                 GattClient_Protected_ClientLibDeinit,
                                 GattAmsInit,
                                 GattAmsDestroy,
                                 GattAmsClient_MsgHandler,
                                 ATT_UUID128, 0x89D3502Bu, 0x0F36433Au, 0x8EF4C502u, 0xAD55F8DCu,
                                 CONTINUE_SERVICE_DISCOVERY,
                                 ams_data);
#else
INIT_SIMPLE_GATT_CLIENT_INSTANCE(gatt_ams_client,
                                GattAmsClient_MsgHandler,
                                gatt_uuid128, 0x89D3502Bu, 0x0F36433Au, 0x8EF4C502u, 0xAD55F8DCu,
                                CONTINUE_SERVICE_DISCOVERY,
                                GattAmsInit,
                                GattAmsDestroy,
                                ams_data);
#endif

#ifdef USE_SYNERGY
static void GattAmsClientDeallocateMemory(gatt_client_t* instance)
{
    if(instance)
        gatt_client_instance = instance;

    if(gatt_client_deinit_done)
    {
        if(gatt_client_instance)
        {
            DEBUG_LOG(DEBUG_GATT_CLIENT_AMS "Deallocate AMS Client\n");
            GattClient_Protected_DeallocateMemory(gatt_client_instance);
            gatt_client_instance = NULL;
            gatt_client_deinit_done = FALSE;
        }
    }
}
#endif

static void GattAmsClient_MsgHandler (Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    switch(id)
    {
        case GATT_AMS_CLIENT_INIT_CFM:
            if(GattClient_Protected_GetStatus(&gatt_ams_client) == gatt_client_status_service_attached)
            {
                GattAmsSetRemoteCommandNotificationEnableRequest(NULL, ams_data, DISABLE_REMOTE_CLIENT_NOTIFICATIONS);
                GattAmsSetEntityUpdateNotificationEnableRequest(NULL, ams_data, DISABLE_ENTITY_UPDATE_NOTIFICATIONS);
            }
            break;

#ifdef USE_SYNERGY
        case GATT_AMS_CLIENT_DEINIT_CFM:
            gatt_client_deinit_done = TRUE;
            GattAmsClientDeallocateMemory(NULL);
            break;
#endif

        default:
            DEBUG_LOG(DEBUG_GATT_CLIENT_AMS "Unhandled AMS msg MESSAGE:0x%04x\n", id);
            break;
    }
}
