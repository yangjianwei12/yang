/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_telephone_bearer_server_access.h"
#include "gatt_telephone_bearer_server_private.h"

/* Required octets for values sent to Client Configuration Descriptor */
#define CLIENT_CONFIG_VALUE_SIZE           (2)

/* Service internal Call ID parameters */
#define CALL_ID_OUTGOING             (0)
#define CALL_ID_MIN                  (1)
#define CALL_ID_MAX                  (255)

/* Find the index in the call list of the given caller id */
uint8 tbsFindCall(GTBS_T *telephone_bearer_server, uint8 cId)
{
    uint16 i = 0;

    /* find matching call in the array */
    for(i = 0; i < TBS_CURRENT_CALLS_LIST_SIZE; i++)
    {
        if (telephone_bearer_server->data.currentCallsList[i].callId == cId )
        {
            return i;
        }
    }

    return FREE_CALL_SLOT; /* Not found */
}

/* Call list handling functionality */
/* Adds a new call in the specified state */
uint16 tbsAddCall(GTBS_T *telephone_bearer_server,GattTbsCallStates state, GattTbsCallFlags flags, uint16 callerIdLen, char* callerId)
{
    uint16 i = 0; 
    uint16 newCallId;
    uint8 idLength = 0;
    
    newCallId = telephone_bearer_server->data.nextCallId;
    
    /* find first free call slot in the array */
    while(i < TBS_CURRENT_CALLS_LIST_SIZE &&
          telephone_bearer_server->data.currentCallsList[i].callId != FREE_CALL_SLOT )
    {
        i++;
    }        
    /* no free slots */
    if (i == TBS_CURRENT_CALLS_LIST_SIZE)
        return FREE_CALL_SLOT;    

    /* initialise call array element */
    telephone_bearer_server->data.currentCallsList[i].callId = newCallId;    
    telephone_bearer_server->data.currentCallsList[i].callState = state; 
    telephone_bearer_server->data.currentCallsList[i].callFlags = flags;

    if(callerIdLen > 0)
    {   /* if the caller id is too long then truncate to the max size */
        idLength = callerIdLen <= MAX_CALL_URI_SIZE ? callerIdLen : MAX_CALL_URI_SIZE;
        telephone_bearer_server->data.currentCallsList[i].callUri = PanicUnlessMalloc(idLength);
        telephone_bearer_server->data.currentCallsList[i].callUriLen = idLength;
        memmove(telephone_bearer_server->data.currentCallsList[i].callUri,
                callerId, idLength);
    }
    else
    {
        telephone_bearer_server->data.currentCallsList[i].callUri = NULL;
        telephone_bearer_server->data.currentCallsList[i].callUriLen = 0;
    }
    
    /* increment the call id for the next call */
    if(telephone_bearer_server->data.nextCallId >= TBS_MAX_CALL_ID)
    {
        telephone_bearer_server->data.nextCallId = 1;
    }
    else
    {
        telephone_bearer_server->data.nextCallId += 1;
    }
    
    return newCallId;
}

/* Sets an existing call to the specified state */
bool tbsSetCallState(GTBS_T *telephone_bearer_server, uint8 cId, GattTbsCallStates state)
{
    /* find matching call index */
    uint8 i = tbsFindCall(telephone_bearer_server, cId);

    if(i != FREE_CALL_SLOT)
    {
        telephone_bearer_server->data.currentCallsList[i].callState = state;
        return TRUE;
    }        
    
    return FALSE;
}

bool tbsSetAllCallState(GTBS_T *telephone_bearer_server,
                          const GattTbsCallStates oldState,
                          const GattTbsCallStates newState)
{
    uint8 callId = 0;
    bool retVal = FALSE;

    while(callId < TBS_CURRENT_CALLS_LIST_SIZE)
    {
        /* check if this call element is in use */
        if(telephone_bearer_server->data.currentCallsList[callId].callId != FREE_CALL_SLOT)
        {
            /* check the current state and transition to new state if required */
            if(telephone_bearer_server->data.currentCallsList[callId].callState == oldState)
            {
                telephone_bearer_server->data.currentCallsList[callId].callState = newState;
                retVal = TRUE;
            }
        }
        callId++;
    }

    return retVal;
}

/* delete existing call from the list */
bool tbsDeleteCall(GTBS_T *telephone_bearer_server, uint8 cId)
{
    /* find matching call index */
    uint8 i = tbsFindCall(telephone_bearer_server, cId);
    
    if(i != FREE_CALL_SLOT)
    {
        /* reset element */
        if(telephone_bearer_server->data.currentCallsList[i].callUri != NULL)
        {
            free(telephone_bearer_server->data.currentCallsList[i].callUri);
            telephone_bearer_server->data.currentCallsList[i].callUri = NULL;
        }

        telephone_bearer_server->data.currentCallsList[i].callId = FREE_CALL_SLOT;
        telephone_bearer_server->data.currentCallsList[i].callState = 0;
        telephone_bearer_server->data.currentCallsList[i].callUriLen = 0;
        telephone_bearer_server->data.currentCallsList[i].callFlags = 0;

        gattTelephoneBearerServerNotifyCallChange(telephone_bearer_server);

        return TRUE;
    }        

    return FALSE;
}

/* returns a pointer to a single array of the call list 
   caller must free array */
uint8* tbsGetCallList(const GTBS_T *telephone_bearer_server, uint16* len)
{
    uint8 callId = 0;
    uint8* callArray = NULL;
    uint16 arraySize = 0;
    uint16 nextPtr = 0;
    
    /* determine the size of the array required */ 
    while(callId < TBS_CURRENT_CALLS_LIST_SIZE)
    {
        /* check if this element is in use */
        if(telephone_bearer_server->data.currentCallsList[callId].callId != FREE_CALL_SLOT)
        {
            /* add the size of this element.  Add one octet for the length */
            arraySize += telephone_bearer_server->data.currentCallsList[callId].callUriLen
                         + BEARER_CALL_LIST_ELEMENT_SIZE;
        }
        callId++;
    }    
    
    /* check if any calls found */
    if(arraySize == 0)
    {
        *len = 0;
        return NULL;
    }
    
    callArray = PanicUnlessMalloc(arraySize);
    
    callId = 0;
    while(callId < TBS_CURRENT_CALLS_LIST_SIZE)
    {   /* check if this element is in use */
        if(telephone_bearer_server->data.currentCallsList[callId].callId != FREE_CALL_SLOT )
        {
            uint8 listElement[BEARER_CALL_LIST_ELEMENT_SIZE];
            uint8 elementSize = telephone_bearer_server->data.currentCallsList[callId].callUriLen
                                 + BEARER_CALL_LIST_ELEMENT_SIZE;

            /* Assemble resonse for this call element of the list */
            listElement[0] = elementSize-1;
            listElement[1] = telephone_bearer_server->data.currentCallsList[callId].callId;
            listElement[2] = telephone_bearer_server->data.currentCallsList[callId].callState;
            listElement[3] = telephone_bearer_server->data.currentCallsList[callId].callFlags;

            memmove(callArray+nextPtr,
                    &listElement,
                    BEARER_CALL_LIST_ELEMENT_SIZE);

            if(telephone_bearer_server->data.currentCallsList[callId].callUriLen != 0 &&
               telephone_bearer_server->data.currentCallsList[callId].callUri != NULL )
            {
                memmove(callArray+nextPtr+BEARER_CALL_LIST_ELEMENT_SIZE,
                        &telephone_bearer_server->data.currentCallsList[callId].callUri,
                        telephone_bearer_server->data.currentCallsList[callId].callUriLen);
            }

            nextPtr += elementSize;
        }
        callId++;
    }    
    
    *len = arraySize;
    return callArray;
   
}



/* returns a pointer to a single array of the call state list 
   caller must free array */
uint8* tbsGetCallStateList(const GTBS_T *telephone_bearer_server, uint16* len)
{
    uint8 callId = 0;
    uint8* callStateArray = NULL;
    uint16 arraySize = 0;
    uint16 nextPtr = 0;
    
    /* determine the size of the array required */ 
    while(callId < TBS_CURRENT_CALLS_LIST_SIZE)
    {
        /* check if this element is in use */
        if(telephone_bearer_server->data.currentCallsList[callId].callId != FREE_CALL_SLOT)
        {
            /* add the size of this element.  Add one octet for the length */
            arraySize += sizeof(tbsCallStateChracteristic);
        }
        callId++;
    }    
    
    /* check if any calls found */
    if(arraySize == 0)
    {
        *len = 0;
        return NULL;
    }
    
    callStateArray = PanicUnlessMalloc(arraySize);
    
    callId = 0;
    while(callId < TBS_CURRENT_CALLS_LIST_SIZE)
    {   /* check if this element is in use */
        if(telephone_bearer_server->data.currentCallsList[callId].callId != FREE_CALL_SLOT )
        {            
            tbsCallStateChracteristic csElement;
            csElement.callId = telephone_bearer_server->data.currentCallsList[callId].callId;
            csElement.callState = telephone_bearer_server->data.currentCallsList[callId].callState;      
            csElement.callFlags = telephone_bearer_server->data.currentCallsList[callId].callFlags;
            memmove(callStateArray+nextPtr, &csElement, sizeof(tbsCallStateChracteristic));
            nextPtr += sizeof(tbsCallStateChracteristic);
        }
        callId++;
    }    
    
    *len = arraySize;
    return callStateArray;
   
}

/***************************************************************************
NAME
    tbsHandleBearerTechnologyAccess

DESCRIPTION
    Deals with access of the HANDLE_BEARER_TECHNOLOGY handle.
*/

static void tbsHandleBearerTechnologyAccess(
        const GTBS_T *telephone_bearer_server,
        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind
        )
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        sendTbsServerAccessRsp(
                (Task)&telephone_bearer_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_success,
                BEARER_TECHNOLOGY_SIZE,
                (uint8*)&(telephone_bearer_server->data.technology)
                );
    }
    else
    {
        sendTbsServerAccessErrorRsp(
                (Task) &telephone_bearer_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_request_not_supported
                );
    }
}


/***************************************************************************
NAME
    tbsHandleBearerProviderNameAccess

DESCRIPTION
    Deals with access of the HANDLE_BEARER_PROVIDER_NAME handle.
*/

static void tbsHandleBearerProviderNameAccess(
        const GTBS_T *telephone_bearer_server,
        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind
        )
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        sendTbsServerAccessRsp(
                (Task)&telephone_bearer_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_success,
                telephone_bearer_server->data.providerNameLen,
                (uint8*)telephone_bearer_server->data.providerName
                );
    }
    else
    {
        sendTbsServerAccessErrorRsp(
                (Task) &telephone_bearer_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_request_not_supported
                );
    }
}

/***************************************************************************
NAME
    tbsHandleBearerUciAccess

DESCRIPTION
    Deals with access of the HANDLE_BEARER_UCI handle.
*/

static void tbsHandleBearerUciAccess(
        const GTBS_T *telephone_bearer_server,
        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind
        )
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        sendTbsServerAccessRsp(
                (Task)&telephone_bearer_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_success,
                telephone_bearer_server->data.uciLen,
                (uint8*)telephone_bearer_server->data.uci
                );
    }
    else
    {
        sendTbsServerAccessErrorRsp(
                (Task) &telephone_bearer_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_request_not_supported
                );
    }
}

/***************************************************************************
NAME
    tbsHandleBearerUriPrefixAccess

DESCRIPTION
    Deals with access of the HANDLE_BEARER_URI_PREFIX_LIST handle.
*/

static void tbsHandleBearerUriPrefixAccess(
        const GTBS_T *telephone_bearer_server,
        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind
        )
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        sendTbsServerAccessRsp(
                (Task)&telephone_bearer_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_success,
                BEARER_URI_PREFIXES_SIZE,
                (uint8*)&(telephone_bearer_server->data.uriPrefixesList)
                );
    }
    else
    {
        sendTbsServerAccessErrorRsp(
                (Task) &telephone_bearer_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_request_not_supported
                );
    }
}


/***************************************************************************
NAME
    tbsHandleListCurrentCallsAccess

DESCRIPTION
    Deals with access of the HANDLE_LIST_CURRENT_CALLS handle.
*/

static void tbsHandleListCurrentCallsAccess(
        const GTBS_T *telephone_bearer_server,
        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind
        )
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        uint16 len = 0;
        uint8* callList = tbsGetCallList(telephone_bearer_server, &len);
        
        sendTbsServerAccessRsp(
                (Task)&telephone_bearer_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_success,
                len,
                callList
                );
                
        if(callList)
            free(callList);
    }
    else
    {
        sendTbsServerAccessErrorRsp(
                (Task) &telephone_bearer_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_request_not_supported
                );
    }
}

/***************************************************************************
NAME
    tbsHandleContentControlIdAccess

DESCRIPTION
    Deals with access of the HANDLE_CONTENT_CONTROL_ID handle.
*/

static void tbsHandleContentControlIdAccess(
        const GTBS_T *telephone_bearer_server,
        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind
        )
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        sendTbsServerAccessRsp(
                (Task)&telephone_bearer_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_success,
                CONTENT_CONTROL_ID_SIZE,
                (uint8*)&(telephone_bearer_server->data.contentControlId)
                );
    }
    else
    {
        sendTbsServerAccessErrorRsp(
                (Task) &telephone_bearer_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_request_not_supported
                );
    }
}

/***************************************************************************
NAME
    tbsHandleIncomingCallTargetUriAccess

DESCRIPTION
    Deals with access of the HANDLE_INCOMING_CALL_TARGET_BEARER_URI handle.
*/

static void tbsHandleIncomingCallTargetUriAccess(
        const GTBS_T *telephone_bearer_server,
        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind
        )
{
    /* Create a buffer and assemble response */
    uint16 bufSize = telephone_bearer_server->data.incomingTargetBearerUri.uriLen +
                     sizeof(telephone_bearer_server->data.incomingTargetBearerUri.callId);
    uint8* buffer = PanicUnlessMalloc(bufSize);

    memmove(buffer, &telephone_bearer_server->data.incomingTargetBearerUri.callId,
            sizeof(telephone_bearer_server->data.incomingTargetBearerUri.callId));
    memmove(buffer + sizeof(telephone_bearer_server->data.incomingTargetBearerUri.callId),
            &telephone_bearer_server->data.incomingTargetBearerUri.uri,
            telephone_bearer_server->data.incomingTargetBearerUri.uriLen);

    if (access_ind->flags & ATT_ACCESS_READ)
    {
        sendTbsServerAccessRsp(
                (Task)&telephone_bearer_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_success,
                bufSize,
                buffer
                );
    }
    else
    {
        sendTbsServerAccessErrorRsp(
                (Task) &telephone_bearer_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_request_not_supported
                );
    }

    free(buffer);
}

/***************************************************************************
NAME
    tbsHandleSupportedFeaturesAccess

DESCRIPTION
    Deals with access of the HANDLE_SUPPORTED_FEATEURES handle.
*/

static void tbsHandleSupportedFeaturesAccess(
        const GTBS_T *telephone_bearer_server,
        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind
        )
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        sendTbsServerAccessRsp(
                (Task)&telephone_bearer_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_success,
                SUPPORTED_FEATURES_SIZE,
                (uint8*)&(telephone_bearer_server->data.statusFlags)
                );
    }
    else
    {
        sendTbsServerAccessErrorRsp(
                (Task) &telephone_bearer_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_request_not_supported
                );
    }
}


/***************************************************************************
NAME
    tbsHandleCallStateAccess

DESCRIPTION
    Deals with access of the HANDLE_CALL_STATE handle.
*/

static void tbsHandleCallStateAccess(
        const GTBS_T *telephone_bearer_server,
        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind
        )
{
 
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        uint16 len = 0;
        uint8* callStateList = tbsGetCallStateList(telephone_bearer_server, &len);
        
        sendTbsServerAccessRsp(
                (Task)&telephone_bearer_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_success,
                len,
                callStateList
                );
                               
        if(callStateList)
            free(callStateList);                                            
    }
    else
    {
        sendTbsServerAccessErrorRsp(
                (Task) &telephone_bearer_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_request_not_supported
                );
    }
}


/***************************************************************************
NAME
    tbsHandleCallControlPointOpcodesAccess

DESCRIPTION
    Deals with access of the HANDLE_CALL_CONTROL_POINT_OPCODES handle.
*/

static void tbsHandleCallControlPointOpcodesAccess(
        const GTBS_T *telephone_bearer_server,
        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind
        )
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        sendTbsServerAccessRsp(
                (Task)&telephone_bearer_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_success,
                CCP_OPCODES_SIZE,
                (uint8*)&(telephone_bearer_server->data.callControlPointOpcodes)
                );
    }
    else
    {
        sendTbsServerAccessErrorRsp(
                (Task) &telephone_bearer_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_request_not_supported
                );
    }
}

/***************************************************************************
NAME
    tbsHandleCallFriendlyNameAccess

DESCRIPTION
    Deals with access of the HANDLE_INCOMING_FRIENDLY_NAME_ID handle.
*/
static void tbsHandleCallFriendlyNameAccess(
        const GTBS_T *telephone_bearer_server,
        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind
        )
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        sendTbsServerAccessRsp(
                (Task)&telephone_bearer_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_success,
                sizeof(tbsCallFriendlyNameChracteristic),
                (uint8*)&(telephone_bearer_server->data.callFriendlyName)
                );
    }
    else
    {
        sendTbsServerAccessErrorRsp(
                (Task) &telephone_bearer_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_request_not_supported
                );
    }
}

/***************************************************************************
NAME
    tbsHandleIncomingCallAccess

DESCRIPTION
    Deals with access of the HANDLE_INCOMING_CALL handle.
*/

static void tbsHandleIncomingCallAccess(
        const GTBS_T *telephone_bearer_server,
        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind
        )
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        sendTbsServerAccessRsp(
                (Task)&telephone_bearer_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_success,
                (telephone_bearer_server->data.incomingCall.callId == 0) ? 0 : sizeof(tbsIncomingCallChracteristic),
                (uint8*)&(telephone_bearer_server->data.incomingCall)
                );
    }
    else
    {
        sendTbsServerAccessErrorRsp(
                (Task) &telephone_bearer_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_request_not_supported
                );
    }
}

/***************************************************************************
NAME
    tbsHandleSignalStrengthAccess

DESCRIPTION
    Deals with access of the HANDLE_SIGNAL_STRENGTH handle.
*/

static void tbsHandleSignalStrengthAccess(
        const GTBS_T *telephone_bearer_server,
        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind
        )
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        sendTbsServerAccessRsp(
                (Task)&telephone_bearer_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_success,
                SIGNAL_STRENGTH_SIZE,
                (uint8*)&(telephone_bearer_server->data.signalStrength)
                );
    }
    else
    {
        sendTbsServerAccessErrorRsp(
                (Task) &telephone_bearer_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_request_not_supported
                );
    }
}

/***************************************************************************
NAME
    tbsHandleSignalStrengthIntervalAccess

DESCRIPTION
    Deals with access of the HANDLE_SIGNAL_STRENGTH_REPORTING_INTERVAL handle.
*/

static void tbsHandleSignalStrengthIntervalAccess(
        const GTBS_T *telephone_bearer_server,
        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind
        )
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        sendTbsServerAccessRsp(
        (Task)&telephone_bearer_server->lib_task,
        access_ind->cid,
        access_ind->handle,
        gatt_status_success,
        SIGNAL_STRENGTH_INTERVAL_SIZE,
        (uint8*)&(telephone_bearer_server->data.signalStrengthReportingInterval)
        );
    }
    else if (access_ind->flags & ATT_ACCESS_WRITE)
    {
        /* Store the written value */
        telephone_bearer_server->data.signalStrengthReportingInterval = access_ind->value[0];

        gattTelephoneBearerServerWriteGenericResponse(
                    (Task) &telephone_bearer_server->lib_task,
                    access_ind->cid,
                    gatt_status_success,
                    HANDLE_SIGNAL_STRENGTH_REPORTING_INTERVAL
                    );

        /* Send Indication to the application that the Signal Strength Interval has changed */
        MAKE_TBS_MESSAGE( GattTbsBearerSignalStrengthIntervalInd );

        message->srvcHndl = telephone_bearer_server->srvc_handle;
        message->cid = access_ind->cid;
        message->interval = telephone_bearer_server->data.signalStrengthReportingInterval;

        MessageSend(telephone_bearer_server->appTask,
                    GattTbsBearerSignalStrengthIntervalInd,
                    message
                    );
    }
    else
    {
        sendTbsServerAccessErrorRsp(
                (Task) &telephone_bearer_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_request_not_supported
                );
    }
}

GattTbsCcpNotificationResultCodes tbsPrevalidateCcpOpcode(const ServiceHandle srvcHndl,
                                                                 tbsCallControlPointChracteristic *controlPoint,
                                                                    uint16 size,
                                                                    uint8* opcode,
                                                                    uint8* callId)
{
    bool validCallId = FALSE;
    GattTbsCcpNotificationResultCodes result;
    GattTbsCallStates currentCallState, newCallState;
    uint16 numJoinCalls; /* number of calls to join */
    uint8* joinCallsList; /* list of calls to join */

    *opcode = controlPoint->opcode;

    /* check opcode support */
    result = GattTelephoneBearerServerCheckOpcodeSupport(srvcHndl,
                                                         *opcode);
    if(result != TBS_CCP_RESULT_SUCCESS)
    {
        return result;
    }

    /* if opcode originate there's nothing more to check */
    if(controlPoint->opcode == TBS_OPCODE_ORIGINATE)
    {
        *callId = 0;
        return TBS_CCP_RESULT_SUCCESS;
    }

    /* anything else but join checks are done */
    if(*opcode == TBS_OPCODE_JOIN)
    {
        /* Just join opcode checks from here */
        numJoinCalls = size-1; /* number of calls to join */
        joinCallsList = (uint8*)&controlPoint->param1[0]; /* list of calls to join */

        if(numJoinCalls < 2)
        {   /* need at least 2 call ids */
            return TBS_CCP_RESULT_OPERATION_NOT_POSSIBLE;
        }

        uint8 callNum = 0;
        validCallId = TRUE;

        /* Validate the callids in the join list actually exist */
        while(result == TBS_CCP_RESULT_SUCCESS &&
              validCallId && callNum < numJoinCalls)
        {
            uint8 nextCallId = *(joinCallsList+callNum);
            validCallId = GattTelephoneBearerServerGetCallState(srvcHndl,
                                                                nextCallId,
                                                                &currentCallState );
            if(!validCallId)
            {   /* Call ID not valid */
                return TBS_CCP_RESULT_INVALID_CALL_INDEX;
            }
            else
            {
                /* Validate that Join opcode can be used on a call in this state */
                newCallState = validateCcpOpcodeCallId(srvcHndl, nextCallId, *opcode );
                if(newCallState != TBS_CALL_STATE_INVALID)
                {
                    return TBS_CCP_RESULT_OPERATION_NOT_POSSIBLE;
                }
            }

            callNum++;
        }
    }
    else
    {   /* All other opcodes except join */
        /* Validate the call id and get the current call state */
        *callId = controlPoint->param1[0];
        if(!GattTelephoneBearerServerGetCallState(srvcHndl,
                                                  *callId,
                                                  &currentCallState))
        {
            return TBS_CCP_RESULT_INVALID_CALL_INDEX;
        }

        /* Validate that this opcode can be used on a call in this state */
        newCallState = validateCcpOpcodeCallId(srvcHndl, *callId, *opcode );
        if(newCallState != TBS_CALL_STATE_INVALID)
        {
            return TBS_CCP_RESULT_STATE_MISMATCH;
        }
    }

    return TBS_CCP_RESULT_SUCCESS;
}

/***************************************************************************
NAME
    tbsHandleCallControlPointAccess

DESCRIPTION
    Deals with access of the HANDLE_CALL_CONTROL_POINT handle.
*/

static void tbsHandleCallControlPointAccess(
        GTBS_T *telephone_bearer_server,
        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind
        )
{
    if (access_ind->flags & ATT_ACCESS_WRITE)
    {
        /* This may contain multiple control point opcodes */
        uint16 ccpSize = access_ind->size_value;
        uint8 result, callId, opcode;

        gattTelephoneBearerServerWriteGenericResponse(
                    (Task) &telephone_bearer_server->lib_task,
                    access_ind->cid,
                    gatt_status_success,
                    HANDLE_CALL_CONTROL_POINT
                    );

        /* Prevalidate the control point access before we notify the app */
        result = tbsPrevalidateCcpOpcode(telephone_bearer_server->srvc_handle,
                                         (tbsCallControlPointChracteristic*)access_ind->value,
                                         ccpSize, &opcode, &callId);

        if(result != TBS_CCP_RESULT_SUCCESS)
        {
            tbsCallControlPointNotification ccpn;

            ccpn.resultCode = result;
            ccpn.opcode = opcode;
            ccpn.callId = callId;
            GattTelephoneBearerServerCallControlPointResponse(telephone_bearer_server->srvc_handle,
                                                              &ccpn);

        }
        else
        {
            /* Send Indication to the application, notifications are sent upon response to this message */
            MAKE_TBS_MESSAGE_WITH_LEN_U8( GattTbsCallControlPointInd, ccpSize );

            message->srvcHndl = telephone_bearer_server->srvc_handle;
            message->cid = access_ind->cid;
            message->cpLen = ccpSize;
            memmove((void*)&message->controlPoint, access_ind->value, ccpSize);

            MessageSend(telephone_bearer_server->appTask,
                        GattTbsCallControlPointInd,
                        message
                        );
        }
    }
    else
    {
        sendTbsServerAccessErrorRsp(
                (Task) &telephone_bearer_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_request_not_supported
                );
    }
}


/***************************************************************************/
bool tbsFindCid(const GTBS_T *telephone_bearer_server, uint16 cid, uint8 *index)
{
    uint8 i;
    bool res = FALSE;

    for(i=0; i<TBS_MAX_CONNECTIONS; i++)
    {
        if(telephone_bearer_server->data.connected_clients[i].cid == cid)
        {
            (*index) = i;
            res = TRUE;
        }
    }

    return res;
}

/***************************************************************************/
void tbsHandleAccessIndication(
        GTBS_T *telephone_bearer_server,
        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind
        )
{
    uint8 i;

    if(tbsFindCid(telephone_bearer_server, access_ind->cid, &i))
    {
        switch (access_ind->handle)
        {

            case HANDLE_BEARER_PROVIDER_NAME:
            {
                tbsHandleBearerProviderNameAccess(
                            telephone_bearer_server,
                            access_ind
                            );
                break;
            }            
            
            case HANDLE_BEARER_PROVIDER_NAME_CLIENT_CONFIG:
            {
                if (access_ind->flags & ATT_ACCESS_READ)
                {
                    tbsHandleReadClientConfigAccess(
                                (Task) &telephone_bearer_server->lib_task,
                                access_ind->cid,
                                access_ind->handle,
                                telephone_bearer_server->data.connected_clients[i].clientCfg.providerNameClientCfg
                                );
                }
                else if (access_ind->flags & ATT_ACCESS_WRITE)
                {
                    uint16 newCcc = 0;
                    tbsHandleWriteClientConfigAccess(
                                                (Task) &telephone_bearer_server->lib_task,
                                                access_ind,
                                                &newCcc);
                    telephone_bearer_server->data.connected_clients[i].clientCfg.providerNameClientCfg = TBS_CCC_MASK(newCcc);
                }
                else
                {
                    sendTbsServerAccessErrorRsp(
                                (Task) &telephone_bearer_server->lib_task,
                                access_ind->cid,
                                access_ind->handle,
                                gatt_status_request_not_supported
                                );
                }
                break;
            }

            case HANDLE_BEARER_UCI:
                tbsHandleBearerUciAccess(
                            telephone_bearer_server,
                            access_ind
                            );
                break;


            case HANDLE_BEARER_TECHNOLOGY:
                tbsHandleBearerTechnologyAccess(
                            telephone_bearer_server,
                            access_ind
                            );
                break;            
                
            case HANDLE_BEARER_TECHNOLOGY_CLIENT_CONFIG:   
            {
                if (access_ind->flags & ATT_ACCESS_READ)
                {
                    tbsHandleReadClientConfigAccess(
                                (Task) &telephone_bearer_server->lib_task,
                                access_ind->cid,
                                access_ind->handle,
                                telephone_bearer_server->data.connected_clients[i].clientCfg.bearerTechnologyClientCfg
                                );
                }   
                else if (access_ind->flags & ATT_ACCESS_WRITE)
                {
                    uint16 newCcc = 0;
                    tbsHandleWriteClientConfigAccess(
                                                (Task) &telephone_bearer_server->lib_task,
                                                access_ind,
                                                &newCcc);

                    telephone_bearer_server->data.connected_clients[i].clientCfg.bearerTechnologyClientCfg = TBS_CCC_MASK(newCcc);
                }                
                else
                {
                    sendTbsServerAccessErrorRsp(
                                (Task) &telephone_bearer_server->lib_task,
                                access_ind->cid,
                                access_ind->handle,
                                gatt_status_request_not_supported
                                );
                }
                break;                
            }          

            case HANDLE_BEARER_URI_PREFIX_LIST:
                tbsHandleBearerUriPrefixAccess(
                            telephone_bearer_server,
                            access_ind
                            );
                break;
                

            case HANDLE_SIGNAL_STRENGTH:
                tbsHandleSignalStrengthAccess(
                            telephone_bearer_server,
                            access_ind
                            );
                break;

            case HANDLE_SIGNAL_STRENGTH_CLIENT_CONFIG:
            {
                if (access_ind->flags & ATT_ACCESS_READ)
                {
                    tbsHandleReadClientConfigAccess(
                                (Task) &telephone_bearer_server->lib_task,
                                access_ind->cid,
                                access_ind->handle,
                                telephone_bearer_server->data.connected_clients[i].clientCfg.signalStrengthClientCfg
                                );
                }   
                else if (access_ind->flags & ATT_ACCESS_WRITE)
                {
                    uint16 newCcc = 0;
                    tbsHandleWriteClientConfigAccess(
                                                (Task) &telephone_bearer_server->lib_task,
                                                access_ind,
                                                &newCcc);

                    telephone_bearer_server->data.connected_clients[i].clientCfg.signalStrengthClientCfg = TBS_CCC_MASK(newCcc);
                }                
                else
                {
                    sendTbsServerAccessErrorRsp(
                                (Task) &telephone_bearer_server->lib_task,
                                access_ind->cid,
                                access_ind->handle,
                                gatt_status_request_not_supported
                                );
                }
                break;                
            }                 
            case HANDLE_SIGNAL_STRENGTH_REPORTING_INTERVAL:
                tbsHandleSignalStrengthIntervalAccess(
                            telephone_bearer_server,
                            access_ind
                            );
                break;
            
            case HANDLE_LIST_CURRENT_CALLS:
                tbsHandleListCurrentCallsAccess(
                            telephone_bearer_server,
                            access_ind
                            );
                break;              
            
            case HANDLE_CURRENT_CALLS_CLIENT_CONFIG:
            {
                if (access_ind->flags & ATT_ACCESS_READ)
                {
                    tbsHandleReadClientConfigAccess(
                                (Task) &telephone_bearer_server->lib_task,
                                access_ind->cid,
                                access_ind->handle,
                                telephone_bearer_server->data.connected_clients[i].clientCfg.currentCallsListClientCfg
                                );
                }   
                else if (access_ind->flags & ATT_ACCESS_WRITE)
                {
                    uint16 newCcc = 0;
                    tbsHandleWriteClientConfigAccess(
                                                (Task) &telephone_bearer_server->lib_task,
                                                access_ind,
                                                &newCcc);

                    telephone_bearer_server->data.connected_clients[i].clientCfg.currentCallsListClientCfg = TBS_CCC_MASK(newCcc);
                }                
                else
                {
                    sendTbsServerAccessErrorRsp(
                                (Task) &telephone_bearer_server->lib_task,
                                access_ind->cid,
                                access_ind->handle,
                                gatt_status_request_not_supported
                                );
                }
                break;                
            }                
            
            case HANDLE_CONTENT_CONTROL_ID:
                tbsHandleContentControlIdAccess(
                            telephone_bearer_server,
                            access_ind
                            );
                break;  
                     
            case HANDLE_INCOMING_CALL_TARGET_BEARER_URI:
                tbsHandleIncomingCallTargetUriAccess(
                            telephone_bearer_server,
                            access_ind
                            );
                break;
                
            case HANDLE_INCOMING_CALL_TARGET_BEARER_URI_CLIENT_CONFIG:
            {
                if (access_ind->flags & ATT_ACCESS_READ)
                {
                    tbsHandleReadClientConfigAccess(
                                (Task) &telephone_bearer_server->lib_task,
                                access_ind->cid,
                                access_ind->handle,
                                telephone_bearer_server->data.connected_clients[i].clientCfg.incomingTargetUriClientCfg
                                );
                }   
                else if (access_ind->flags & ATT_ACCESS_WRITE)
                {     
                    uint16 newCcc = 0;
                    tbsHandleWriteClientConfigAccess(
                                                (Task) &telephone_bearer_server->lib_task,
                                                access_ind,
                                                &newCcc);

                    telephone_bearer_server->data.connected_clients[i].clientCfg.incomingTargetUriClientCfg = TBS_CCC_MASK(newCcc);
                }                
                else
                {
                    sendTbsServerAccessErrorRsp(
                                (Task) &telephone_bearer_server->lib_task,
                                access_ind->cid,
                                access_ind->handle,
                                gatt_status_request_not_supported
                                );
                }
                break;                
            }             
            case HANDLE_STATUS_FLAGS:
                tbsHandleSupportedFeaturesAccess(
                            telephone_bearer_server,
                            access_ind
                            );
                break;

            case HANDLE_STATUS_FLAGS_CLIENT_CONFIG:
            {
                if (access_ind->flags & ATT_ACCESS_READ)
                {
                    tbsHandleReadClientConfigAccess(
                                (Task) &telephone_bearer_server->lib_task,
                                access_ind->cid,
                                access_ind->handle,
                                telephone_bearer_server->data.connected_clients[i].clientCfg.statusFlagsClientCfg
                                );
                }   
                else if (access_ind->flags & ATT_ACCESS_WRITE)
                {                   
                    uint16 newCcc = 0;
                    tbsHandleWriteClientConfigAccess(
                                                (Task) &telephone_bearer_server->lib_task,
                                                access_ind,
                                                &newCcc);

                    telephone_bearer_server->data.connected_clients[i].clientCfg.statusFlagsClientCfg = TBS_CCC_MASK(newCcc);
                }                
                else
                {
                    sendTbsServerAccessErrorRsp(
                                (Task) &telephone_bearer_server->lib_task,
                                access_ind->cid,
                                access_ind->handle,
                                gatt_status_request_not_supported
                                );
                }
                break;                
            }      
            
            case HANDLE_CALL_STATE:
                tbsHandleCallStateAccess(
                            telephone_bearer_server,
                            access_ind
                            );
                break;        
                
            case HANDLE_CALL_STATE_CLIENT_CONFIG:
            {
                if (access_ind->flags & ATT_ACCESS_READ)
                {
                    tbsHandleReadClientConfigAccess(
                                (Task) &telephone_bearer_server->lib_task,
                                access_ind->cid,
                                access_ind->handle,
                                telephone_bearer_server->data.connected_clients[i].clientCfg.callStateClientCfg
                                );
                }   
                else if (access_ind->flags & ATT_ACCESS_WRITE)
                {
                    uint16 newCcc = 0;
                    tbsHandleWriteClientConfigAccess(
                                                (Task) &telephone_bearer_server->lib_task,
                                                access_ind,
                                                &newCcc);

                    telephone_bearer_server->data.connected_clients[i].clientCfg.callStateClientCfg = TBS_CCC_MASK(newCcc);
                }                
                else
                {
                    sendTbsServerAccessErrorRsp(
                                (Task) &telephone_bearer_server->lib_task,
                                access_ind->cid,
                                access_ind->handle,
                                gatt_status_request_not_supported
                                );
                }
                break;                
            }               
                
            case HANDLE_CALL_CONTROL_POINT:            
            {
                tbsHandleCallControlPointAccess(
                            telephone_bearer_server,
                            access_ind
                            );
            }
            break;

            case HANDLE_CALL_CONTROL_POINT_OPCODES:
            {
                tbsHandleCallControlPointOpcodesAccess(
                            telephone_bearer_server,
                            access_ind
                            );
            }
            break;
            
            case HANDLE_CALL_CONTROL_POINT_CLIENT_CONFIG:
            {
                if (access_ind->flags & ATT_ACCESS_READ)
                {
                    tbsHandleReadClientConfigAccess(
                                (Task) &telephone_bearer_server->lib_task,
                                access_ind->cid,
                                access_ind->handle,
                                telephone_bearer_server->data.connected_clients[i].clientCfg.callControlPointClientCfg
                                );
                }
                else if (access_ind->flags & ATT_ACCESS_WRITE)
                {
                    uint16 newCcc = 0;
                    tbsHandleWriteClientConfigAccess(
                                                (Task) &telephone_bearer_server->lib_task,
                                                access_ind,
                                                &newCcc);

                    telephone_bearer_server->data.connected_clients[i].clientCfg.callControlPointClientCfg = TBS_CCC_MASK(newCcc);

                }
                else
                {
                    sendTbsServerAccessErrorRsp(
                                (Task) &telephone_bearer_server->lib_task,
                                access_ind->cid,
                                access_ind->handle,
                                gatt_status_request_not_supported
                                );
                }
                break;
            }

            case HANDLE_TERMINATION_REASON:
            {
                /* This characteristic is notify only so send error response */
                sendTbsServerAccessErrorRsp(
                    (Task) &telephone_bearer_server->lib_task,
                    access_ind->cid,
                    access_ind->handle,
                    gatt_status_request_not_supported
                    );

                break;
            }
            
            case HANDLE_TERMINATION_REASON_CLIENT_CONFIG:
            {
                if (access_ind->flags & ATT_ACCESS_READ)
                {
                    tbsHandleReadClientConfigAccess(
                                (Task) &telephone_bearer_server->lib_task,
                                access_ind->cid,
                                access_ind->handle,
                                telephone_bearer_server->data.connected_clients[i].clientCfg.terminationReasonClientCfg
                                );
                }   
                else if (access_ind->flags & ATT_ACCESS_WRITE)
                {
                    uint16 newCcc = 0;
                    tbsHandleWriteClientConfigAccess(
                                                (Task) &telephone_bearer_server->lib_task,
                                                access_ind,
                                                &newCcc);

                    telephone_bearer_server->data.connected_clients[i].clientCfg.terminationReasonClientCfg = TBS_CCC_MASK(newCcc);
                }                
                else
                {
                    sendTbsServerAccessErrorRsp(
                                (Task) &telephone_bearer_server->lib_task,
                                access_ind->cid,
                                access_ind->handle,
                                gatt_status_request_not_supported
                                );
                }
                break;                
            }                

            case HANDLE_REMOTE_FRIENDLY_NAME:
                tbsHandleCallFriendlyNameAccess(
                            telephone_bearer_server,
                            access_ind
                            );
                break;

            case HANDLE_REMOTE_FRIENDLY_NAME_CLIENT_CONFIG:
            {
                if (access_ind->flags & ATT_ACCESS_READ)
                {
                    tbsHandleReadClientConfigAccess(
                                (Task) &telephone_bearer_server->lib_task,
                                access_ind->cid,
                                access_ind->handle,
                                telephone_bearer_server->data.connected_clients[i].clientCfg.callFriendlyNameClientCfg
                                );
                }
                else if (access_ind->flags & ATT_ACCESS_WRITE)
                {
                    uint16 newCcc = 0;
                    tbsHandleWriteClientConfigAccess(
                                                (Task) &telephone_bearer_server->lib_task,
                                                access_ind,
                                                &newCcc);

                    telephone_bearer_server->data.connected_clients[i].clientCfg.callFriendlyNameClientCfg = TBS_CCC_MASK(newCcc);
                }
                else
                {
                    sendTbsServerAccessErrorRsp(
                                (Task) &telephone_bearer_server->lib_task,
                                access_ind->cid,
                                access_ind->handle,
                                gatt_status_request_not_supported
                                );
                }
                break;
            }

            case HANDLE_INCOMING_CALL:
                tbsHandleIncomingCallAccess(
                            telephone_bearer_server,
                            access_ind
                            );
                break;

            case HANDLE_INCOMING_CALL_CLIENT_CONFIG:
            {
                if (access_ind->flags & ATT_ACCESS_READ)
                {
                    tbsHandleReadClientConfigAccess(
                                (Task) &telephone_bearer_server->lib_task,
                                access_ind->cid,
                                access_ind->handle,
                                telephone_bearer_server->data.connected_clients[i].clientCfg.incomingCallClientCfg
                                );
                }
                else if (access_ind->flags & ATT_ACCESS_WRITE)
                {                   
                    uint16 newCcc = 0;
                    tbsHandleWriteClientConfigAccess(
                                                (Task) &telephone_bearer_server->lib_task,
                                                access_ind,
                                                &newCcc);

                    telephone_bearer_server->data.connected_clients[i].clientCfg.incomingCallClientCfg = TBS_CCC_MASK(newCcc);
                }
                else
                {
                    sendTbsServerAccessErrorRsp(
                                (Task) &telephone_bearer_server->lib_task,
                                access_ind->cid,
                                access_ind->handle,
                                gatt_status_request_not_supported
                                );
                }
                break;
            }


            default:
            {
                sendTbsServerAccessErrorRsp(
                            (Task) &telephone_bearer_server->lib_task,
                            access_ind->cid,
                            access_ind->handle,
                            gatt_status_invalid_handle
                            );
                break;
            }
        } /* switch */
    }
    else
    {
        GATT_TELEPHONE_BEARER_SERVER_DEBUG_PANIC((
                    "GTBS: No valid Cid!\n"
                    ));
    }
}


void gattTelephoneBearerServerNotifyCallControlPoint(GTBS_T *telephone_bearer_server, tbsCallControlPointNotification *ccpn, uint8* clientIndex)
{
    uint16 i=0;

    /* if an index of a client has been specified notify that one only */
    if(clientIndex != NULL)
    {
        i = *clientIndex;
    }

    /* Notify connected clients of the change */
    for (; i<TBS_MAX_CONNECTIONS; i++)
    {
        if (telephone_bearer_server->data.connected_clients[i].cid != 0)
        {
            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (telephone_bearer_server->data.connected_clients[i].clientCfg.callControlPointClientCfg == CLIENT_CONFIG_NOTIFY)
            {
                tbsServerSendCharacteristicChangedNotification(
                        (Task) &telephone_bearer_server->lib_task,
                        telephone_bearer_server->data.connected_clients[i].cid,
                        HANDLE_CALL_CONTROL_POINT,
                        sizeof(tbsCallControlPointNotification),
                        (uint8*)ccpn
                        );
            }
            else
            {
                /* Characteristic not configured for Notification - Nothing to do */
            }
        }

        if(clientIndex != NULL)
        {
            /* only notify the specified connection */
            break;
        }
    }
}


/* Send a notification that the call list has changed */
void gattTelephoneBearerServerNotifyCurrentCalls(GTBS_T *telephone_bearer_server, uint8* clientIndex)
{
    uint16 i=0;

    /* if an index of a client has been specified notify that one only */
    if(clientIndex != NULL)
    {
        i = *clientIndex;
    }

    /* Notify connected clients of the change */
    for (; i<TBS_MAX_CONNECTIONS; i++)
    {
        if (telephone_bearer_server->data.connected_clients[i].cid != 0)
        {
            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (telephone_bearer_server->data.connected_clients[i].clientCfg.currentCallsListClientCfg == CLIENT_CONFIG_NOTIFY)
            {
                uint16 len = 0;
                uint8* callList = tbsGetCallList(telephone_bearer_server, &len);

                tbsServerSendCharacteristicChangedNotification(
                        (Task) &telephone_bearer_server->lib_task,
                        telephone_bearer_server->data.connected_clients[i].cid,
                        HANDLE_LIST_CURRENT_CALLS,
                        len,
                        callList
                        );

                if(callList)
                    free(callList);
            }
            else
            {
                /* Characteristic not configured for Notification - Nothing to do */
            }
        }

        if(clientIndex != NULL)
        {
            /* only notify the specified connection */
            break;
        }
    }
}


void gattTelephoneBearerServerNotifyIncomingCallTargetUri(GTBS_T *telephone_bearer_server, uint8* clientIndex)
{
    uint16 i=0;
    /* Create a buffer and assemble response */
    uint16 bufSize = telephone_bearer_server->data.incomingTargetBearerUri.uriLen +
                     sizeof(telephone_bearer_server->data.incomingTargetBearerUri.callId);
    uint8* buffer = PanicUnlessMalloc(bufSize);

    memmove(buffer, &telephone_bearer_server->data.incomingTargetBearerUri.callId,
            sizeof(telephone_bearer_server->data.incomingTargetBearerUri.callId));
    memmove(buffer + sizeof(telephone_bearer_server->data.incomingTargetBearerUri.callId),
            telephone_bearer_server->data.incomingTargetBearerUri.uri,
            telephone_bearer_server->data.incomingTargetBearerUri.uriLen);

    /* if an index of a client has been specified notify that one only */
    if(clientIndex != NULL)
    {
        i = *clientIndex;
    }

    /* Notify connected clients of the change */
    for (; i<TBS_MAX_CONNECTIONS; i++)
    {
        if (telephone_bearer_server->data.connected_clients[i].cid != 0)
        {
            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (telephone_bearer_server->data.connected_clients[i].clientCfg.incomingTargetUriClientCfg == CLIENT_CONFIG_NOTIFY)
            {
                tbsServerSendCharacteristicChangedNotification(
                        (Task) &telephone_bearer_server->lib_task,
                        telephone_bearer_server->data.connected_clients[i].cid,
                        HANDLE_INCOMING_CALL_TARGET_BEARER_URI,
                        bufSize,
                        buffer
                        );
            }
            else
            {
                /* Characteristic not configured for Notification - Nothing to do */
            }
        }

        if(clientIndex != NULL)
        {
            /* only notify the specified connection */
            break;
        }
    }

    free(buffer);
}


void gattTelephoneBearerServerNotifyCallState(GTBS_T *telephone_bearer_server, uint8* clientIndex)
{
    uint16 i =0;
    uint16 len = 0;
    uint8* callStateList = NULL;

    callStateList = tbsGetCallStateList(telephone_bearer_server, &len);

    /* if an index of a client has been specified notify that one only */
    if(clientIndex != NULL)
    {
        i = *clientIndex;
    }

    /* Notify connected clients of the change */
    for (; i<TBS_MAX_CONNECTIONS; i++)
    {
        if (telephone_bearer_server->data.connected_clients[i].cid != 0)
        {
            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (telephone_bearer_server->data.connected_clients[i].clientCfg.callStateClientCfg == CLIENT_CONFIG_NOTIFY)
            {
                tbsServerSendCharacteristicChangedNotification(
                        (Task) &telephone_bearer_server->lib_task,
                        telephone_bearer_server->data.connected_clients[i].cid,
                        HANDLE_CALL_STATE,
                        len,
                        callStateList
                        );
            }
            else
            {
                /* Characteristic not configured for Notification - Nothing to do */
            }
        }

        if(clientIndex != NULL)
        {
            /* only notify the specified connection */
            break;
        }
    }

    if(callStateList)
        free(callStateList);

}

void gattTelephoneBearerServerNotifyCallFriendlyName(GTBS_T *telephone_bearer_server, uint8* clientIndex)
{
    uint16 i=0;

    /* Create a buffer and assemble response */
    uint16 bufSize = telephone_bearer_server->data.callFriendlyName.nameLen +
                     sizeof(telephone_bearer_server->data.callFriendlyName.callId);
    uint8* buffer = PanicUnlessMalloc(bufSize);

    memmove(buffer, &telephone_bearer_server->data.callFriendlyName.callId,
            sizeof(telephone_bearer_server->data.callFriendlyName.callId));
    memmove(buffer + sizeof(telephone_bearer_server->data.callFriendlyName.callId),
            telephone_bearer_server->data.callFriendlyName.friendlyName,
            telephone_bearer_server->data.callFriendlyName.nameLen);

    /* if an index of a client has been specified notify that one only */
    if(clientIndex != NULL)
    {
        i = *clientIndex;
    }

    /* Notify connected clients of the change */
    for (; i<TBS_MAX_CONNECTIONS; i++)
    {
        if (telephone_bearer_server->data.connected_clients[i].cid != 0)
        {
            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (telephone_bearer_server->data.connected_clients[i].clientCfg.callFriendlyNameClientCfg == CLIENT_CONFIG_NOTIFY)
            {
                tbsServerSendCharacteristicChangedNotification(
                        (Task) &telephone_bearer_server->lib_task,
                        telephone_bearer_server->data.connected_clients[i].cid,
                        HANDLE_REMOTE_FRIENDLY_NAME,
                        bufSize,
                        buffer
                        );
            }
            else
            {
                /* Characteristic not configured for Notification - Nothing to do */
            }
        }

        if(clientIndex != NULL)
        {
            /* only notify the specified connection */
            break;
        }
    }

    free(buffer);
}

void gattTelephoneBearerServerNotifySignalStrength(GTBS_T *telephone_bearer_server, uint8* clientIndex)
{
    uint16 i=0;

    /* If the reporting interval is Zero then do not notify the change */
    if(telephone_bearer_server->data.signalStrengthReportingInterval == 0)
    {
        return;
    }

    /* Flag to block further notifications of signal strength */
    telephone_bearer_server->data.signal_strength_timer_flag = TRUE;

    /* Store the last value actually notified */
    telephone_bearer_server->data.signalStrengthNotified = telephone_bearer_server->data.signalStrength;

    /* if an index of a client has been specified notify that one only */
    if(clientIndex != NULL)
    {
        i = *clientIndex;
    }

    /* Notify connected clients of the change */
    for (; i<TBS_MAX_CONNECTIONS; i++)
    {
        if (telephone_bearer_server->data.connected_clients[i].cid != 0)
        {
            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (telephone_bearer_server->data.connected_clients[i].clientCfg.signalStrengthClientCfg == CLIENT_CONFIG_NOTIFY)
            {
                tbsServerSendCharacteristicChangedNotification(
                        (Task) &telephone_bearer_server->lib_task,
                        telephone_bearer_server->data.connected_clients[i].cid,
                        HANDLE_SIGNAL_STRENGTH,
                        SIGNAL_STRENGTH_SIZE,
                        (uint8*)&telephone_bearer_server->data.signalStrength
                        );
            }
            else
            {
                /* Characteristic not configured for Notification - Nothing to do */
            }
        }

        if(clientIndex != NULL)
        {
            /* only notify the specified connection */
            break;
        }
    }

    {
        /* Block further signal strength reports until the timer fires */
        MAKE_TBS_MESSAGE(GATT_TBS_INTERNAL_SIGNAL_STRENGTH_TIMER);

        MessageSendLater((Task) &telephone_bearer_server->lib_task,
                            GATT_TBS_INTERNAL_SIGNAL_STRENGTH_TIMER,
                            message,
                            D_SEC(telephone_bearer_server->data.signalStrengthReportingInterval));
    }
}


void gattTelephoneBearerServerNotifyIncomingCall(GTBS_T *telephone_bearer_server, uint8* clientIndex)
{   
    uint16 i=0;
    uint8 callSlot;

    /* Find call id */
    callSlot = tbsFindCall(telephone_bearer_server, telephone_bearer_server->data.incomingCall.callId);

    if(callSlot == FREE_CALL_SLOT)
    {
        GATT_TELEPHONE_BEARER_SERVER_DEBUG_INFO((
                    "GTBS: NotifyIncomingCall - No valid incoming call id!\n"
                    ));
        return;
    }

    /* Create a buffer and assemble response */
    uint16 bufSize =sizeof(telephone_bearer_server->data.incomingCall.callId) +
                     sizeof(telephone_bearer_server->data.currentCallsList[callSlot].callUri);
    uint8* buffer = PanicUnlessMalloc(bufSize);

    memmove(buffer, &telephone_bearer_server->data.incomingCall.callId,
            sizeof(telephone_bearer_server->data.incomingCall.callId));
    memmove(buffer + sizeof(telephone_bearer_server->data.incomingCall.callId),
            telephone_bearer_server->data.currentCallsList[callSlot].callUri,
            sizeof(telephone_bearer_server->data.incomingCall.callId));

    /* if an index of a client has been specified notify that one only */
    if(clientIndex != NULL)
    {
        i = *clientIndex;
    }

    /* Notify connected clients of the change */
    for (; i<TBS_MAX_CONNECTIONS; i++)
    {
        if (telephone_bearer_server->data.connected_clients[i].cid != 0)
        {
            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (telephone_bearer_server->data.connected_clients[i].clientCfg.incomingCallClientCfg == CLIENT_CONFIG_NOTIFY)
            {
                tbsServerSendCharacteristicChangedNotification(
                        (Task) &telephone_bearer_server->lib_task,
                        telephone_bearer_server->data.connected_clients[i].cid,
                        HANDLE_INCOMING_CALL,
                        bufSize,
                        buffer
                        );
            }
            else
            {
                /* Characteristic not configured for Notification - Nothing to do */
            }
        }

        if(clientIndex != NULL)
        {
            /* only notify the specified connection */
            break;
        }
    }

    free(buffer);
}

/* Something has changed in a call state, send all the notifications to do with that */
void gattTelephoneBearerServerNotifyCallChange(GTBS_T *telephone_bearer_server)
{
    gattTelephoneBearerServerNotifyCallState(telephone_bearer_server, NULL);
    gattTelephoneBearerServerNotifyCurrentCalls(telephone_bearer_server, NULL);
}



