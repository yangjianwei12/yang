/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include <string.h>
#include <stdio.h>

#include <gatt.h>

#include "gatt_telephone_bearer_client_notification.h"
#include "gatt_telephone_bearer_client_discover.h"
#include "gatt_telephone_bearer_client_read.h"


/****************************************************************************
Internal functions
****************************************************************************/

/***************************************************************************/
static void handleRegisterForNotification(const GTBSC *tbs_client,
                                              bool enable,
                                              uint16 handle)
{
    /* Validate the Input Parameters */
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_PANIC(("GTBSC: Null client instance\n"));
    }

    MAKE_TBSC_MESSAGE(TELEPHONE_BEARER_INTERNAL_MSG_SET_NOTIFICATION);
    message->notificationsEnable = enable;
    message->handle = handle;    
    MessageSend((Task)&tbs_client->lib_task,
                 TELEPHONE_BEARER_INTERNAL_MSG_SET_NOTIFICATION,
                 message);
}

void tbsHandleInternalRegisterForNotification(GTBSC *const tbs_client,
                                                    bool enable,
                                                    uint16 handle)
{
    uint8 value[TBS_CLIENT_CHARACTERISTIC_CONFIG_SIZE];

    GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: Register notification handle = 0x%x enable = 0x%x\n",handle, enable));

    value[0] = enable ? TELEPHONE_BEARER_NOTIFICATION_VALUE : 0;
    value[1] = 0;
    GattManagerWriteCharacteristicValue((Task)&tbs_client->lib_task,
                                        handle,
                                        TBS_CLIENT_CHARACTERISTIC_CONFIG_SIZE,
                                        value);
}



/*******************************************************************************
 * Send a GATT_TBS_CLIENT_PROVIDER_NAME_IND message to the application task.
 */
static void makeTbsBearerProviderNameIndMsg(GTBSC *tbs_client,
                                                    uint16 size,
                                                    char* provider_name)
{
    MAKE_TBSC_MESSAGE_WITH_LEN(GATT_TBS_CLIENT_PROVIDER_NAME_IND, size);

    GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: Provider name ind. handle = 0x%x size = 0x%x\n", tbs_client->srvcHandle, size));

    message->tbsHandle = tbs_client->srvcHandle;
    memmove(message->providerName, provider_name, size);
    message->providerNameSize = size;

    MessageSend(tbs_client->appTask, GATT_TBS_CLIENT_PROVIDER_NAME_IND, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_BEARER_TECHNOLOGY_IND message to the application task.
 */
static void makeTbsBearerTechnologyIndMsg(GTBSC *tbs_client,
                                                  uint8 size,
                                                  char* bearer_tech)
{
    MAKE_TBSC_MESSAGE_WITH_LEN(GATT_TBS_CLIENT_BEARER_TECHNOLOGY_IND, size);

    GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: Bearer Tech ind. handle = 0x%x size = 0x%x\n", tbs_client->srvcHandle, size));

    memset(message, 0, sizeof(GATT_TBS_CLIENT_BEARER_TECHNOLOGY_IND_T));
    message->tbsHandle = tbs_client->srvcHandle;

    memmove(message->bearerTech, bearer_tech, size);
    message->bearerTechSize = size;

    MessageSend(tbs_client->appTask, GATT_TBS_CLIENT_BEARER_TECHNOLOGY_IND, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_SIGNAL_STRENGTH_IND message to the application task.
 */
static void makeTbsSignalStrengthIndMsg(GTBSC *tbs_client,
                                                uint8 signal_strength)
{
    MAKE_TBSC_MESSAGE(GATT_TBS_CLIENT_SIGNAL_STRENGTH_IND);

    GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: Signal strength ind. handle = 0x%x strength = 0x%x\n", tbs_client->srvcHandle, signal_strength));

    message->tbsHandle = tbs_client->srvcHandle;
    message->signalStrength = signal_strength;

    MessageSend(tbs_client->appTask, GATT_TBS_CLIENT_SIGNAL_STRENGTH_IND, message);
}



/*******************************************************************************
 * Send a GATT_TBS_CLIENT_CURRENT_CALLS_IND message to the application task.
 */
static void makeTbsCurrentCallsListIndMsg(GTBSC *tbs_client,
                                                  uint8 size,
                                                  uint8* current_calls)
{
    MAKE_TBSC_MESSAGE_WITH_LEN(GATT_TBS_CLIENT_CURRENT_CALLS_IND, size);

    GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: Current calls list ind. handle = 0x%x size = 0x%x\n", tbs_client->srvcHandle, size));

    memset(message, 0, sizeof(GATT_TBS_CLIENT_CURRENT_CALLS_IND_T));
    message->tbsHandle = tbs_client->srvcHandle;
    memmove(message->currentCallsList, current_calls, size);
    message->currentCallsListSize = size;

    MessageSend(tbs_client->appTask, GATT_TBS_CLIENT_CURRENT_CALLS_IND, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_FLAGS_IND message to the application task.
 */
static void makeTbsFlagsIndMsg(GTBSC *tbs_client,
                                      uint16 flags)
{
    MAKE_TBSC_MESSAGE(GATT_TBS_CLIENT_FLAGS_IND);

    GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: Flags ind. handle = 0x%x flags = 0x%x\n", tbs_client->srvcHandle, flags));

    message->tbsHandle = tbs_client->srvcHandle;
    message->flags = flags;

    MessageSend(tbs_client->appTask, GATT_TBS_CLIENT_FLAGS_IND, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_INCOMING_CALL_TARGET_BEARER_URI_IND message to the application task.
 */
static void makeTbsIncomingCallTargetBearerUriIndMsg(GTBSC *tbs_client,
                                                               uint8 size,
                                                               char* uri)
{
    MAKE_TBSC_MESSAGE_WITH_LEN(GATT_TBS_CLIENT_INCOMING_CALL_TARGET_BEARER_URI_IND, size);

    GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: Incoming Call T.B. Uri ind. handle = 0x%x size = 0x%x\n", tbs_client->srvcHandle, size));

    memset(message, 0, sizeof(GATT_TBS_CLIENT_INCOMING_CALL_TARGET_BEARER_URI_IND_T));
    message->tbsHandle = tbs_client->srvcHandle;
    memmove(message->uri, uri, size);
    message->uriSize = size;

    MessageSend(tbs_client->appTask, GATT_TBS_CLIENT_INCOMING_CALL_TARGET_BEARER_URI_IND, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_CALL_STATE_IND message to the application task.
 */
static void makeTbsCallStateIndMsg(GTBSC *tbs_client,
                                          uint8 size,
                                          uint8* call_state)
{
    uint8 stateSize = sizeof(TbsCallState);
    MAKE_TBSC_MESSAGE_WITH_LEN(GATT_TBS_CLIENT_CALL_STATE_IND,size);

    GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: Call state ind. handle = 0x%x size = 0x%x\n", tbs_client->srvcHandle, size));

    memset(message, 0, sizeof(GATT_TBS_CLIENT_CALL_STATE_IND_T));
    message->tbsHandle = tbs_client->srvcHandle;

    /* check if the size is correct */
    if (size % stateSize == 0)
    {
        memmove(message->callStateList, call_state, size);
        message->callStateListSize = size / stateSize;
    }

    MessageSend(tbs_client->appTask, GATT_TBS_CLIENT_CALL_STATE_IND, message);
}

/*******************************************************************************
 * Send a GATT_TBS_CLIENT_CALL_CONTROL_POINT_IND message to the application task.
 */
static void makeTbsCallControlPointIndMsg(GTBSC *tbs_client,
                                              uint8 size,
                                              uint8* ccp)
{
    MAKE_TBSC_MESSAGE_WITH_LEN(GATT_TBS_CLIENT_CALL_CONTROL_POINT_IND,size);

    GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: Call Control Point ind. handle = 0x%x size = 0x%x\n", tbs_client->srvcHandle, size));

    message->tbsHandle = tbs_client->srvcHandle;

    if(size==3)
    {
        message->opcode = ccp[0];
        message->callId = ccp[1];
        message->resultCode = ccp[2];

        GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: Call Control Point ind. opcode = 0x%x call id = 0x%x result code = 0x%x\n",
                                    message->opcode, message->callId, message->resultCode));
    }

    MessageSend(tbs_client->appTask, GATT_TBS_CLIENT_CALL_CONTROL_POINT_IND, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_TERMINATION_REASON_IND message to the application task.
 */
static void makeTbsTerminationReasonIndMsg(GTBSC *tbs_client,
                                              uint8 size,
                                              uint8* reason)
{

    MAKE_TBSC_MESSAGE_WITH_LEN(GATT_TBS_CLIENT_TERMINATION_REASON_IND,size);

    GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: Termination reason ind. handle = 0x%x size = 0x%x\n", tbs_client->srvcHandle, size));

    message->tbsHandle = tbs_client->srvcHandle;

    if(size==2)
    {
        message->callId = reason[0];
        message->reasonCode = reason[1];
    }

    MessageSend(tbs_client->appTask, GATT_TBS_CLIENT_TERMINATION_REASON_IND, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_INCOMING_CALL_IND message to the application task.
 */
static void makeTbsIncomingCallIndMsg(GTBSC *tbs_client,
                                              uint8 size,
                                              uint8* incoming_call)
{

    MAKE_TBSC_MESSAGE_WITH_LEN(GATT_TBS_CLIENT_INCOMING_CALL_IND,size);
    memset(message, 0, sizeof(GATT_TBS_CLIENT_INCOMING_CALL_IND_T));
    message->tbsHandle = tbs_client->srvcHandle;

    if(size>=1)
    {
        /* first octet is the call id */
        message->callId = incoming_call[0];
        if(size >= 2)
        {
            /* further octets are the uri */
            uint8 uriSize = size-1;
            memmove(message->uri, incoming_call+1, uriSize);
            message->uriSize = uriSize;
        }
    }

    GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: Incoming call ind. handle = 0x%x call id = 0x%x\n", tbs_client->srvcHandle, message->callId));

    MessageSend(tbs_client->appTask, GATT_TBS_CLIENT_INCOMING_CALL_IND, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_CALL_FRIENDLY_NAME_IND message to the application task.
 */
static void makeTbsCallFriendlyNameIndMsg(GTBSC *tbs_client,
                                                           uint8 size,
                                                           uint8* friendly_name)
{
    MAKE_TBSC_MESSAGE_WITH_LEN(GATT_TBS_CLIENT_CALL_FRIENDLY_NAME_IND,size);
    memset(message, 0, sizeof(GATT_TBS_CLIENT_CALL_FRIENDLY_NAME_IND_T));
    message->tbsHandle = tbs_client->srvcHandle;

    if(size>=2)
    {
        uint8 nameSize = size-1;

        /* first octet is the call id */
        message->callId = friendly_name[0];

        /* further octets are the uri */
        memmove(message->friendlyName, friendly_name+1, nameSize);
        message->friendlyNameSize = nameSize;
    }

    GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: Call friendly name ind. handle = 0x%x call id = 0x%x size = 0x%x\n",
                                tbs_client->srvcHandle, message->callId, message->friendlyNameSize));

    MessageSend(tbs_client->appTask, GATT_TBS_CLIENT_CALL_FRIENDLY_NAME_IND, message);
}



/***************************************************************************/
void handleTbsNotification(const ServiceHandle tbsHandle, const GATT_MANAGER_REMOTE_SERVER_NOTIFICATION_IND_T *ind)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);

    /* Check parameters */
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_DEBUG_PANIC(("GTBSC: Invalid parameters - TBS Client Notifications\n"));
        return;
    }

    /* Check the handle if it's a known and send Indication messages to the registered task if a
     *  value has been sent in the notification message */
    if(ind->handle == tbs_client->bearer_name_handle)
    {
        makeTbsBearerProviderNameIndMsg(tbs_client,ind->size_value, (char*)ind->value);
    }
    else if(ind->handle == tbs_client->bearer_tech_handle)
    {        
        makeTbsBearerTechnologyIndMsg(tbs_client,ind->size_value, (char*)ind->value);
    }
    else if(ind->handle == tbs_client->signal_strength_handle)
    {
        makeTbsSignalStrengthIndMsg(tbs_client, ind->value[0]);
    }
    else if(ind->handle == tbs_client->list_current_calls_handle)
    {
        makeTbsCurrentCallsListIndMsg(tbs_client,ind->size_value, (uint8*)ind->value);
    }
    else if(ind->handle == tbs_client->status_flags_handle)
    {
        makeTbsFlagsIndMsg(tbs_client, ((uint16) ind->value[0]) | (((uint16) ind->value[1]) << 8));
    }
    else if(ind->handle == tbs_client->incoming_target_bearer_uri_handle)
    {
        makeTbsIncomingCallTargetBearerUriIndMsg(tbs_client,ind->size_value, (char*)ind->value);
    }
    else if(ind->handle == tbs_client->call_state_handle)
    {
        makeTbsCallStateIndMsg(tbs_client,ind->size_value, (uint8*)ind->value);
    }
    else if(ind->handle == tbs_client->call_control_point_handle)
    {
        makeTbsCallControlPointIndMsg(tbs_client,ind->size_value, (uint8*)ind->value);
    }
    else if(ind->handle == tbs_client->termination_reason_handle)
    {
        makeTbsTerminationReasonIndMsg(tbs_client,ind->size_value, (uint8*)ind->value);
    }
    else if(ind->handle == tbs_client->incoming_call_handle)
    {
        makeTbsIncomingCallIndMsg(tbs_client,ind->size_value, (uint8*)ind->value);
    }
    else if(ind->handle == tbs_client->remote_friendly_name_handle)
    {
        makeTbsCallFriendlyNameIndMsg(tbs_client,ind->size_value, (uint8*)ind->value);
    }
}



/****************************************************************************
Public API
****************************************************************************/

/****************************************************************************/
void GattTelephoneBearerClientSetProviderNameNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);

    /* Check parameters */
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_DEBUG_PANIC(("GTBSC: Invalid params - Provider name notify req\n"));
        return;
    }

    handleRegisterForNotification(tbs_client,
                                  notificationsEnable,
                                  tbs_client->bearer_name_ccc_handle);
}

void GattTelephoneBearerClientSetTechnologyNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);

    /* Check parameters */
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_DEBUG_PANIC(("GTBSC: Invalid params - technology notify req\n"));
        return;
    }

    handleRegisterForNotification(tbs_client,
                                  notificationsEnable,
                                  tbs_client->bearer_tech_ccc_handle);
}

void GattTelephoneBearerClientSetSignalStrengthNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);

    /* Check parameters */
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_DEBUG_PANIC(("GTBSC: Invalid params - signal strength notify req\n"));
        return;
    }

    handleRegisterForNotification(tbs_client,
                                  notificationsEnable,
                                  tbs_client->signal_strength_ccc_handle);
}

void GattTelephoneBearerClientSetListCurrentCallsNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);

    /* Check parameters */
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_DEBUG_PANIC(("GTBSC: Invalid params - current calls notify req\n"));
        return;
    }

    handleRegisterForNotification(tbs_client,
                                  notificationsEnable,
                                  tbs_client->list_current_calls_ccc_handle);
}

void GattTelephoneBearerClientSetFlagsNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);

    /* Check parameters */
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_DEBUG_PANIC(("GTBSC: Invalid params - flags notify req\n"));
        return;
    }

    handleRegisterForNotification(tbs_client,
                                  notificationsEnable,
                                  tbs_client->status_flags_ccc_handle);
}

void GattTelephoneBearerClientSetIncomingCallTargetBearerUriNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);

    /* Check parameters */
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_DEBUG_PANIC(("GTBSC: Invalid params - target bearer uri notify req\n"));
        return;
    }

    handleRegisterForNotification(tbs_client,
                                  notificationsEnable,
                                  tbs_client->incoming_target_bearer_uri_ccc_handle);
}

void GattTelephoneBearerClientSetCallStateNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);

    /* Check parameters */
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_DEBUG_PANIC(("GTBSC: Invalid params - call state notify req\n"));
        return;
    }

    handleRegisterForNotification(tbs_client,
                                  notificationsEnable,
                                  tbs_client->call_state_ccc_handle);
}

void GattTelephoneBearerClientSetCallControlPointNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);

    /* Check parameters */
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_DEBUG_PANIC(("GTBSC: Invalid params - ccp notify req\n"));
        return;
    }

    handleRegisterForNotification(tbs_client,
                                  notificationsEnable,
                                  tbs_client->call_control_point_ccc_handle);
}

void GattTelephoneBearerClientSetTerminationReasonNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);

    /* Check parameters */
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_DEBUG_PANIC(("GTBSC: Invalid params - termination reason notify req\n"));
        return;
    }

    handleRegisterForNotification(tbs_client,
                                  notificationsEnable,
                                  tbs_client->termination_reason_ccc_handle);
}

void GattTelephoneBearerClientSetIncomingCallNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);

    /* Check parameters */
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_DEBUG_PANIC(("GTBSC: Invalid params - inc. call notify req\n"));
        return;
    }

    handleRegisterForNotification(tbs_client,
                                  notificationsEnable,
                                  tbs_client->incoming_call_ccc_handle);
}

void GattTelephoneBearerClientSetCallFriendlyNameNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);

    /* Check parameters */
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_DEBUG_PANIC(("GTBSC: Invalid params - call friendly name notify req\n"));
        return;
    }

    handleRegisterForNotification(tbs_client,
                                  notificationsEnable,
                                  tbs_client->remote_friendly_name_ccc_handle);
}


