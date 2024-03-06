/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include <string.h>
#include <stdio.h>

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
        GATT_TBS_CLIENT_PANIC("GTBSC: Null client instance\n");
        return;
    }
    else
    {
        MAKE_TBSC_MESSAGE(TelephoneBearerInternalMsgSetNotification);
        message->notificationsEnable = enable;
        message->srvcHndl = tbs_client->srvcElem->service_handle;
        message->handle = handle;
        TbsClientMessageSend((AppTask)tbs_client->lib_task,
                     TELEPHONE_BEARER_INTERNAL_MSG_SET_NOTIFICATION,
                     message);
    }
}

void tbsHandleInternalRegisterForNotification(GTBSC *const tbs_client,
                                                    bool enable,
                                                    uint16 handle)
{
    uint8 *value;
    value = (uint8*)CsrPmemZalloc(TBS_CLIENT_CHARACTERISTIC_CONFIG_SIZE*sizeof(uint8));

    GATT_TBS_CLIENT_DEBUG("GTBSC: Register notification handle = 0x%x enable = 0x%x\n",handle, enable);

    value[0] = enable ? TELEPHONE_BEARER_NOTIFICATION_VALUE : 0;
    value[1] = 0;

    CsrBtGattWriteReqSend(tbs_client->srvcElem->gattId,
                          tbs_client->srvcElem->cid,
                          handle,
                          0,
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
    MAKE_TBSC_MESSAGE_WITH_LEN(GattTelephoneBearerClientProviderNameInd, size);

    GATT_TBS_CLIENT_DEBUG("GTBSC: Provider name ind. handle = 0x%x size = 0x%x\n", tbs_client->srvcHandle, size);

    message->tbsHandle = tbs_client->srvcElem->service_handle;

    memmove(message->providerName, provider_name, size);
    message->providerNameSize = size;

    TbsClientMessageSend(tbs_client->appTask, GATT_TELEPHONE_BEARER_CLIENT_PROVIDER_NAME_IND, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_BEARER_TECHNOLOGY_IND message to the application task.
 */
static void makeTbsBearerTechnologyIndMsg(GTBSC *tbs_client,
                                                  uint16 size,
                                                  char* bearer_tech)
{
    MAKE_TBSC_MESSAGE_WITH_LEN(GattTelephoneBearerClientBearerTechnologyInd, size);
    memset(message, 0, sizeof(GattTelephoneBearerClientBearerTechnologyInd));
    GATT_TBS_CLIENT_DEBUG("GTBSC: Bearer Tech ind. handle = 0x%x size = 0x%x\n", tbs_client->srvcHandle, size);
    message->tbsHandle = tbs_client->srvcElem->service_handle;

    memmove(message->bearerTech, bearer_tech, size);
    message->bearerTechSize = size;

    TbsClientMessageSend(tbs_client->appTask, GATT_TELEPHONE_BEARER_CLIENT_BEARER_TECHNOLOGY_IND, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_SIGNAL_STRENGTH_IND message to the application task.
 */
static void makeTbsSignalStrengthIndMsg(GTBSC *tbs_client,
                                                uint8 signal_strength)
{
    MAKE_TBSC_MESSAGE(GattTelephoneBearerClientSignalStrengthInd);
    GATT_TBS_CLIENT_DEBUG("GTBSC: Signal strength ind. handle = 0x%x strength = 0x%x\n", tbs_client->srvcHandle, signal_strength);
    message->tbsHandle = tbs_client->srvcElem->service_handle;

    message->signalStrength = signal_strength;

    TbsClientMessageSend(tbs_client->appTask, GATT_TELEPHONE_BEARER_CLIENT_SIGNAL_STRENGTH_IND, message);
}



/*******************************************************************************
 * Send a GATT_TBS_CLIENT_CURRENT_CALLS_IND message to the application task.
 */
static void makeTbsCurrentCallsListIndMsg(GTBSC *tbs_client,
                                                  uint16 size,
                                                  uint8* current_calls)
{
    MAKE_TBSC_MESSAGE_WITH_LEN(GattTelephoneBearerClientCurrentCallsInd, size);
    GATT_TBS_CLIENT_DEBUG("GTBSC: Current calls list ind. handle = 0x%x size = 0x%x\n", tbs_client->srvcHandle, size);
    message->tbsHandle = tbs_client->srvcElem->service_handle;

    memmove(message->currentCallsList, current_calls, size);
    message->currentCallsListSize = size;

    TbsClientMessageSend(tbs_client->appTask, GATT_TELEPHONE_BEARER_CLIENT_CURRENT_CALLS_IND, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_FLAGS_IND message to the application task.
 */
static void makeTbsFlagsIndMsg(GTBSC *tbs_client,
                                      uint16 flags)
{
    MAKE_TBSC_MESSAGE(GattTelephoneBearerClientFlagsInd);
    GATT_TBS_CLIENT_DEBUG("GTBSC: Flags ind. handle = 0x%x flags = 0x%x\n", tbs_client->srvcHandle, flags);
    message->tbsHandle = tbs_client->srvcElem->service_handle;

    message->flags = flags;

    TbsClientMessageSend(tbs_client->appTask, GATT_TELEPHONE_BEARER_CLIENT_FLAGS_IND, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_INCOMING_CALL_TARGET_BEARER_URI_IND message to the application task.
 */
static void makeTbsIncomingCallTargetBearerUriIndMsg(GTBSC *tbs_client,
                                                               uint16 size,
                                                               char* uri)
{
    MAKE_TBSC_MESSAGE_WITH_LEN(GattTelephoneBearerClientIncomingCallTargetBearerUriInd, size);
    memset(message, 0, sizeof(GattTelephoneBearerClientIncomingCallTargetBearerUriInd));

    GATT_TBS_CLIENT_DEBUG("GTBSC: Incoming Call T.B. Uri ind. handle = 0x%x size = 0x%x\n", tbs_client->srvcHandle, size);

    message->tbsHandle = tbs_client->srvcElem->service_handle;

    if(size>=2)
    {
        uint8 uriSize = size-1;

        /* first octet is the call id */
        message->callId = uri[0];

        /* further octets are the uri */
        memmove(message->uri, uri+1, uriSize);
        message->uriSize= uriSize;
    }

    TbsClientMessageSend(tbs_client->appTask, GATT_TELEPHONE_BEARER_CLIENT_INCOMING_CALL_TARGET_BEARER_URI_IND, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_CALL_STATE_IND message to the application task.
 */
static void makeTbsCallStateIndMsg(GTBSC *tbs_client,
                                          uint16 size,
                                          uint8* call_state)
{
    uint8 stateSize = sizeof(TbsCallState);
    MAKE_TBSC_MESSAGE_WITH_LEN(GattTelephoneBearerClientCallStateInd,size);
    memset(message, 0, sizeof(GattTelephoneBearerClientCallStateInd));
    GATT_TBS_CLIENT_DEBUG("GTBSC: Call state ind. handle = 0x%x size = 0x%x\n", tbs_client->srvcHandle, size);

    message->tbsHandle = tbs_client->srvcElem->service_handle;

    /* check if the size is correct */
    if (size % stateSize == 0)
    {
        memmove(message->callStateList, call_state, size);
        message->callStateListSize = size / stateSize;
    }

    TbsClientMessageSend(tbs_client->appTask, GATT_TELEPHONE_BEARER_CLIENT_CALL_STATE_IND, message);
}

/*******************************************************************************
 * Send a GATT_TBS_CLIENT_CALL_CONTROL_POINT_IND message to the application task.
 */
static void makeTbsCallControlPointIndMsg(GTBSC *tbs_client,
                                              uint16 size,
                                              uint8* ccp)
{
    MAKE_TBSC_MESSAGE_WITH_LEN(GattTelephoneBearerClientCallControlPointInd,size);

    message->tbsHandle = tbs_client->srvcElem->service_handle;

    if(size==3)
    {
        message->opcode = ccp[0];
        message->callId = ccp[1];
        message->resultCode = ccp[2];

        GATT_TBS_CLIENT_DEBUG("GTBSC: Call Control Point ind. opcode = 0x%x call id = 0x%x result code = 0x%x\n",
                              message->opcode, message->callId, message->resultCode);
    }

    TbsClientMessageSend(tbs_client->appTask, GATT_TELEPHONE_BEARER_CLIENT_CALL_CONTROL_POINT_IND, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_TERMINATION_REASON_IND message to the application task.
 */
static void makeTbsTerminationReasonIndMsg(GTBSC *tbs_client,
                                              uint16 size,
                                              uint8* reason)
{
    MAKE_TBSC_MESSAGE_WITH_LEN(GattTelephoneBearerClientTerminationReasonInd,size);
    GATT_TBS_CLIENT_DEBUG("GTBSC: Termination reason ind. handle = 0x%x size = 0x%x\n", tbs_client->srvcHandle, size);

    message->tbsHandle = tbs_client->srvcElem->service_handle;

    if(size==2)
    {
        message->callId = reason[0];
        message->reasonCode = reason[1];
    }

    TbsClientMessageSend(tbs_client->appTask, GATT_TELEPHONE_BEARER_CLIENT_TERMINATION_REASON_IND, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_INCOMING_CALL_IND message to the application task.
 */
static void makeTbsIncomingCallIndMsg(GTBSC *tbs_client,
                                              uint16 size,
                                              uint8* incoming_call)
{

    MAKE_TBSC_MESSAGE_WITH_LEN(GattTelephoneBearerClientIncomingCallInd,size);
    memset(message, 0, sizeof(GattTelephoneBearerClientIncomingCallInd));

    message->tbsHandle = tbs_client->srvcElem->service_handle;

    if(size>=1)
    {
        /* first octet is the call id */
        message->callId = incoming_call[0];
        if(size >= 2)
        {
            /* further octets are the uri */
            uint8 uriSize = size-1;
            memmove(message->uri, &incoming_call[1], uriSize);
            message->uriSize = uriSize;
        }
    }

    GATT_TBS_CLIENT_INFO("GTBSC: Incoming call ind. handle = 0x%x call id = 0x%x\n", tbs_client->srvcHandle, message->callId);

    TbsClientMessageSend(tbs_client->appTask, GATT_TELEPHONE_BEARER_CLIENT_INCOMING_CALL_IND, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_CALL_FRIENDLY_NAME_IND message to the application task.
 */
static void makeTbsCallFriendlyNameIndMsg(GTBSC *tbs_client,
                                                           uint16 size,
                                                           uint8* friendly_name)
{
    MAKE_TBSC_MESSAGE_WITH_LEN(GattTelephoneBearerClientCallFriendlyNameInd,size);
    memset(message, 0, sizeof(GattTelephoneBearerClientCallFriendlyNameInd));

    message->tbsHandle = tbs_client->srvcElem->service_handle;

    if(size>=2)
    {
        uint8 nameSize = size-1;

        /* first octet is the call id */
        message->callId = friendly_name[0];

        /* further octets are the uri */
        memmove(message->friendlyName, &friendly_name[1], nameSize);
        message->friendlyNameSize = nameSize;
    }

    GATT_TBS_CLIENT_DEBUG("GTBSC: Call friendly name ind. handle = 0x%x call id = 0x%x size = 0x%x\n",
                          tbs_client->srvcHandle, message->callId, message->friendlyNameSize);

    TbsClientMessageSend(tbs_client->appTask, GATT_TELEPHONE_BEARER_CLIENT_CALL_FRIENDLY_NAME_IND, message);
}


/***************************************************************************/
void handleTbsNotification(const ServiceHandle tbs_handle, const CsrBtGattClientNotificationInd *ind)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbs_handle);

    /* Check parameters */
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_ERROR("GTBSC: Invalid parameters - TBS Client Notifications\n");
        return;
    }

    /* Check the handle if it's a known and send Indication messages to the registered task if a
     *  value has been sent in the notification message */
    if(ind->valueHandle == tbs_client->handles.bearerNameHandle)
    {
        makeTbsBearerProviderNameIndMsg(tbs_client,ind->valueLength, (char*)ind->value);
    }
    else if(ind->valueHandle == tbs_client->handles.bearerTechHandle)
    {        
        makeTbsBearerTechnologyIndMsg(tbs_client,ind->valueLength, (char*)ind->value);
    }
    else if(ind->valueHandle == tbs_client->handles.signalStrengthHandle)
    {
        makeTbsSignalStrengthIndMsg(tbs_client, ind->value[0]);
    }
    else if(ind->valueHandle == tbs_client->handles.listCurrentCallsHandle)
    {
        makeTbsCurrentCallsListIndMsg(tbs_client,ind->valueLength, (uint8*)ind->value);
    }
    else if(ind->valueHandle == tbs_client->handles.statusFlagsHandle)
    {
        makeTbsFlagsIndMsg(tbs_client, ((uint16) ind->value[0]) | (((uint16) ind->value[1]) << 8));
    }
    else if(ind->valueHandle == tbs_client->handles.incomingTargetBearerUriHandle)
    {
        makeTbsIncomingCallTargetBearerUriIndMsg(tbs_client,ind->valueLength, (char*)ind->value);
    }
    else if(ind->valueHandle == tbs_client->handles.callStateHandle)
    {
        makeTbsCallStateIndMsg(tbs_client,ind->valueLength, (uint8*)ind->value);
    }
    else if(ind->valueHandle == tbs_client->handles.callControlPointHandle)
    {
        makeTbsCallControlPointIndMsg(tbs_client,ind->valueLength, (uint8*)ind->value);
    }
    else if(ind->valueHandle == tbs_client->handles.terminationReasonHandle)
    {
        makeTbsTerminationReasonIndMsg(tbs_client,ind->valueLength, (uint8*)ind->value);
    }
    else if(ind->valueHandle == tbs_client->handles.incomingCallHandle)
    {
        makeTbsIncomingCallIndMsg(tbs_client,ind->valueLength, (uint8*)ind->value);
    }
    else if(ind->valueHandle == tbs_client->handles.remoteFriendlyNameHandle)
    {
        makeTbsCallFriendlyNameIndMsg(tbs_client,ind->valueLength, (uint8*)ind->value);
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
        GATT_TBS_CLIENT_ERROR("GTBSC: Invalid params - Provider name notify req\n");
        return;
    }

    handleRegisterForNotification(tbs_client,
                                  notificationsEnable,
                                  tbs_client->handles.bearerNameCccHandle);
}

void GattTelephoneBearerClientSetTechnologyNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);

    /* Check parameters */
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_ERROR("GTBSC: Invalid params - technology notify req\n");
        return;
    }

    handleRegisterForNotification(tbs_client,
                                  notificationsEnable,
                                  tbs_client->handles.bearerTechCccHandle);
}

void GattTelephoneBearerClientSetSignalStrengthNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);

    /* Check parameters */
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_ERROR("GTBSC: Invalid params - signal strength notify req\n");
        return;
    }

    handleRegisterForNotification(tbs_client,
                                  notificationsEnable,
                                  tbs_client->handles.signalStrengthCccHandle);
}

void GattTelephoneBearerClientSetListCurrentCallsNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);

    /* Check parameters */
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_ERROR("GTBSC: Invalid params - current calls notify req\n");
        return;
    }

    handleRegisterForNotification(tbs_client,
                                  notificationsEnable,
                                  tbs_client->handles.listCurrentCallsCccHandle);
}

void GattTelephoneBearerClientSetFlagsNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);

    /* Check parameters */
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_ERROR("GTBSC: Invalid params - flags notify req\n");
        return;
    }

    handleRegisterForNotification(tbs_client,
                                  notificationsEnable,
                                  tbs_client->handles.statusFlagsCccHandle);
}

void GattTelephoneBearerClientSetIncomingCallTargetBearerUriNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);

    /* Check parameters */
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_ERROR("GTBSC: Invalid params - target bearer uri notify req\n");
        return;
    }

    handleRegisterForNotification(tbs_client,
                                  notificationsEnable,
                                  tbs_client->handles.incomingTargetBearerUriCccHandle);
}

void GattTelephoneBearerClientSetCallStateNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);

    /* Check parameters */
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_ERROR("GTBSC: Invalid params - call state notify req\n");
        return;
    }

    handleRegisterForNotification(tbs_client,
                                  notificationsEnable,
                                  tbs_client->handles.callStateCccHandle);
}

void GattTelephoneBearerClientSetCallControlPointNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);

    /* Check parameters */
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_ERROR("GTBSC: Invalid params - ccp notify req\n");
        return;
    }

    handleRegisterForNotification(tbs_client,
                                  notificationsEnable,
                                  tbs_client->handles.callControlPointCccHandle);
}

void GattTelephoneBearerClientSetTerminationReasonNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);

    /* Check parameters */
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_ERROR("GTBSC: Invalid params - termination reason notify req\n");
        return;
    }

    handleRegisterForNotification(tbs_client,
                                  notificationsEnable,
                                  tbs_client->handles.terminationReasonCccHandle);
}

void GattTelephoneBearerClientSetIncomingCallNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);

    /* Check parameters */
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_ERROR("GTBSC: Invalid params - inc. call notify req\n");
        return;
    }

    handleRegisterForNotification(tbs_client,
                                  notificationsEnable,
                                  tbs_client->handles.incomingCallCccHandle);
}

void GattTelephoneBearerClientSetCallFriendlyNameNotificationRequest(const ServiceHandle tbsHandle, bool notificationsEnable)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);

    /* Check parameters */
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_ERROR("GTBSC: Invalid params - call friendly name notify req\n");
        return;
    }

    handleRegisterForNotification(tbs_client,
                                  notificationsEnable,
                                  tbs_client->handles.remoteFriendlyNameCccHandle);
}


