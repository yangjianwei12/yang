/*!
    \copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.
    All Rights Reserved.
    Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    gatt_client_ancs
    \brief      Application support for GATT ANCS client
*/

#include "../gatt_client/gatt_client.h"
#include "gatt_client_protected.h"
#include "gatt_client_ancs.h"
#include "gatt_client_ancs_event_handler.h"
#include <gatt_apple_notification_client.h>
#include <panic.h>
#include <logging.h>

static GANCS *ancs_data = NULL;

#ifdef USE_SYNERGY
static bool gatt_client_deinit_done = FALSE;
static gatt_client_t *gatt_client_instance = NULL;
#endif

static bool ns_notification_enable = TRUE;
static bool ds_notification_enable = FALSE;

/* Forward function declarations */
static void GattAncsClientMsgHandler(Task task, MessageId id, Message message);

#define DEBUG_GATT_CLIENT_AMS       "Gatt Client ANCS:"

#ifdef USE_SYNERGY
static void GattAncsClientDeallocateMemory(gatt_client_t* instance);

/* Create a ANCS gatt client instance */
INIT_CUSTOM_GATT_CLIENT_INSTANCE(gatt_ancs_client,
                                 GattClient_Protected_AllocateMemory,
                                 GattAncsClientDeallocateMemory,
                                 GattClient_Protected_GetServiceForDiscovery,
                                 GattClient_Protected_GetDiscoveryStopRequest,
                                 GattClient_Protected_ClientLibInit,
                                 GattClient_Protected_ClientLibDeinit,
                                 GattAncsInit,
                                 GattAncsDestroy,
                                 GattAncsClientMsgHandler,
                                 ATT_UUID128, 0x7905f431u, 0xb5ce4e99u, 0xa40f4b1eu, 0x122d00d0u,
                                 CONTINUE_SERVICE_DISCOVERY,
                                 ancs_data);
#else
/* Create a AMS gatt client instance */
INIT_SIMPLE_GATT_CLIENT_INSTANCE(gatt_ancs_client,
                                GattAncsClientMsgHandler,
                                gatt_uuid128, 0x7905f431u, 0xb5ce4e99u, 0xa40f4b1eu, 0x122d00d0u,
                                CONTINUE_SERVICE_DISCOVERY,
                                GattAncsInit,
                                GattAncsDestroy,
                                ancs_data);

#endif

static uint16 GattGetAncsNotificationEnableMask(void)
{
    /* TODO - read ancs notification config. item */
    return 0;
}

static void gattAncsNSNotificationInd(const GATT_ANCS_NS_IND_T *ind)
{
    /*  Notification Source Data format
    * -------------------------------------------------------
    * |  Event  |  Event  |  Cat  |  Cat   |  Notification  |
    * |  ID     |  Flag   |  ID   |  Count |  UUID          |
    * |---------------------------------------------------- |
    * |   1B    |   1B    |   1B  |   1B   |   4B           |
    * -------------------------------------------------------
    */
    DEBUG_LOG(DEBUG_GATT_CLIENT_AMS "Received ANCS Notification Alert,\n\t- eventId - %d\n\t- eventFlag - %d\n"
        "\t- categoryId - %d\n\t- categoryCnt - %d\n\t- NotificationUID - %lx\n", ind->event_id,
        ind->event_flag, ind->category_id, ind->category_count, ind->notification_uid);

    /* only notify if notifications were added/modified */
    if((ind->event_id == gatt_ancs_notification_added) || (ind->event_id == gatt_ancs_notification_modified))
    {
        /* TODO - Hmmm, looks like all we do in the sink app when these notifications are received is pass them onto main message handler
         * (via the message cancel/send in the following code block) and print a debug message. Need to check, but for the time being will
         * just forward these onto an ancs specific message handler that's split from the main ancs client code to allow customer modiciation
         * of ancs client without having to modify the client core code */
        static TaskData task = {.handler = GattClientAncs_eventHandler};
        MessageCancelAll(&task, ind->category_id);
        MessageSend(&task, ind->category_id, 0);
    }
}

#ifdef USE_SYNERGY
static void GattAncsClientDeallocateMemory(gatt_client_t* instance)
{
    if(instance)
        gatt_client_instance = instance;

    if(gatt_client_deinit_done)
    {
        if(gatt_client_instance)
        {
            DEBUG_LOG(DEBUG_GATT_CLIENT_AMS "Deallocate ANCS Client\n");
            GattClient_Protected_DeallocateMemory(gatt_client_instance);
            gatt_client_instance = NULL;
            gatt_client_deinit_done = FALSE;
        }
    }
}
#endif

static void GattAncsClientMsgHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch(id)
    {
        case GATT_ANCS_INIT_CFM:
            if(GattClient_Protected_GetStatus(&gatt_ancs_client) == gatt_client_status_service_attached)
            {
                bool notification_enable = (ns_notification_enable && (GattGetAncsNotificationEnableMask() != ANCS_NO_CATEGORY));
                GattAncsSetNSNotificationEnableRequest(ancs_data, notification_enable, GattGetAncsNotificationEnableMask());
                GattAncsSetDSNotificationEnableRequest(ancs_data, ds_notification_enable);
            }
            break;

        case GATT_ANCS_NS_IND:
            gattAncsNSNotificationInd((GATT_ANCS_NS_IND_T*)message);
            break;

#ifdef USE_SYNERGY
        case GATT_ANCS_DEINIT_CFM:
            gatt_client_deinit_done = TRUE;
            GattAncsClientDeallocateMemory(NULL);
            break;
#endif

        default:
            DEBUG_LOG(DEBUG_GATT_CLIENT_AMS "Unhandled ANCS msg MESSAGE:0x%x\n", id);
            break;
    }
}

void GattAncsClientDisableNotificationsByDefault(void)
{
    ns_notification_enable = FALSE;
    ds_notification_enable = FALSE;
}
