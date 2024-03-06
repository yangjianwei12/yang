/******************************************************************************
 Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "gatt_telephone_bearer_server_access.h"
#include "gatt_telephone_bearer_server_debug.h"
#include "gatt_telephone_bearer_server_private.h"

/* Required octets for values sent to Client Configuration Descriptor */
#define CLIENT_CONFIG_VALUE_SIZE           (2)

/* Service internal Call ID parameters */
#define CALL_ID_OUTGOING             (0)
#define CALL_ID_MIN                  (1)
#define CALL_ID_MAX                  (255)

extern CsrSchedTid tId;

/* Find the index in the call list of the given caller id */
uint8 tbsFindCall(GTBS_T *telephoneBearerServer, uint8 cId)
{
    uint8 i = 0;

    /* find matching call in the array */
    for(i = 0; i < TBS_CURRENT_CALLS_LIST_SIZE; i++)
    {
        if (telephoneBearerServer->data.currentCallsList[i].callId == cId )
        {
            return i;
        }
    }

    return FREE_CALL_SLOT; /* Not found */
}

/* Call list handling functionality */
/* Adds a new call in the specified state */
uint8 tbsAddCall(GTBS_T *telephoneBearerServer,GattTbsCallStates state, GattTbsCallFlags flags, uint16 callerIdLen, char* callerId, bool joinAllowed)
{
    uint16 i = 0; 
    uint8 newCallId;
    uint8 idLength = 0;
    
    newCallId = telephoneBearerServer->data.nextCallId;
    
    /* find first free call slot in the array */
    while(i < TBS_CURRENT_CALLS_LIST_SIZE &&
          telephoneBearerServer->data.currentCallsList[i].callId != FREE_CALL_SLOT )
    {
        i++;
    }        
    /* no free slots */
    if (i == TBS_CURRENT_CALLS_LIST_SIZE)
        return FREE_CALL_SLOT;    

    /* initialise call array element */
    telephoneBearerServer->data.currentCallsList[i].callId = newCallId;    
    telephoneBearerServer->data.currentCallsList[i].callState = state; 
    telephoneBearerServer->data.currentCallsList[i].callFlags = flags  & TBS_CALL_FLAGS_RFU_MASK;
    telephoneBearerServer->data.currentCallsList[i].allowJoin = joinAllowed;

    if(callerIdLen > 0 && callerId != NULL)
    {   /* if the caller id is too long then truncate to the max size */
        idLength = callerIdLen <= MAX_CALL_URI_SIZE ? callerIdLen : MAX_CALL_URI_SIZE;
        telephoneBearerServer->data.currentCallsList[i].callUri = CsrPmemAlloc(idLength);
        telephoneBearerServer->data.currentCallsList[i].callUriLen = idLength;
        CsrMemMove(telephoneBearerServer->data.currentCallsList[i].callUri,
                callerId, idLength);
    }
    else
    {
        telephoneBearerServer->data.currentCallsList[i].callUri = NULL;
        telephoneBearerServer->data.currentCallsList[i].callUriLen = 0;
    }
    
    /* increment the call id for the next call */
    if(telephoneBearerServer->data.nextCallId >= (TBS_MAX_CALL_ID - 1))
    {
        telephoneBearerServer->data.nextCallId = 1;
    }
    else
    {
        telephoneBearerServer->data.nextCallId += 1;
    }
    
    return newCallId;
}

/* Sets an existing call to the specified state */
bool tbsSetCallState(GTBS_T *telephoneBearerServer, uint8 cId, GattTbsCallStates state)
{
    /* find matching call index */
    uint8 i = tbsFindCall(telephoneBearerServer, cId);

    if(i != FREE_CALL_SLOT)
    {
        telephoneBearerServer->data.currentCallsList[i].callState = state;
        return TRUE;
    }        
    
    return FALSE;
}

bool tbsSetAllCallState(GTBS_T *telephoneBearerServer,
                          const GattTbsCallStates oldState,
                          const GattTbsCallStates newState)
{
    uint8 callId = 0;
    bool retVal = FALSE;

    while(callId < TBS_CURRENT_CALLS_LIST_SIZE)
    {
        /* check if this call element is in use */
        if(telephoneBearerServer->data.currentCallsList[callId].callId != FREE_CALL_SLOT)
        {
            /* check the current state and transition to new state if required */
            if(telephoneBearerServer->data.currentCallsList[callId].callState == oldState)
            {
                telephoneBearerServer->data.currentCallsList[callId].callState = newState;
                retVal = TRUE;
            }
        }
        callId++;
    }

    return retVal;
}

/* delete existing call from the list */
bool tbsDeleteCall(GTBS_T *telephoneBearerServer, uint8 cId)
{
    /* find matching call index */
    uint8 i = tbsFindCall(telephoneBearerServer, cId);
    
    if(i != FREE_CALL_SLOT)
    {
        /* reset element */
        if(telephoneBearerServer->data.currentCallsList[i].callUri != NULL)
        {
            CsrPmemFree(telephoneBearerServer->data.currentCallsList[i].callUri);
            telephoneBearerServer->data.currentCallsList[i].callUri = NULL;
        }

        telephoneBearerServer->data.currentCallsList[i].callId = FREE_CALL_SLOT;
        telephoneBearerServer->data.currentCallsList[i].callState = 0;
        telephoneBearerServer->data.currentCallsList[i].callUriLen = 0;
        telephoneBearerServer->data.currentCallsList[i].callFlags = 0;

        gattTelephoneBearerServerNotifyCallChange(telephoneBearerServer);

        return TRUE;
    }        

    return FALSE;
}

/* returns a pointer to a single array of the call list 
   caller must free array */
uint8* tbsGetCallList(const GTBS_T *telephoneBearerServer, uint16* len)
{
    uint8 callId = 0;
    uint8* callArray = NULL;
    uint16 arraySize = 0;
    uint16 nextPtr = 0;
    
    /* determine the size of the array required */ 
    while(callId < TBS_CURRENT_CALLS_LIST_SIZE)
    {
        /* check if this element is in use */
        if(telephoneBearerServer->data.currentCallsList[callId].callId != FREE_CALL_SLOT)
        {
            /* add the size of this element.  Add one octet for the length */
            arraySize += telephoneBearerServer->data.currentCallsList[callId].callUriLen
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
    
    callArray = CsrPmemAlloc(arraySize);
    
    callId = 0;
    while(callId < TBS_CURRENT_CALLS_LIST_SIZE)
    {   /* check if this element is in use */
        if(telephoneBearerServer->data.currentCallsList[callId].callId != FREE_CALL_SLOT )
        {
            uint8 listElement[BEARER_CALL_LIST_ELEMENT_SIZE];
            uint8 elementSize = telephoneBearerServer->data.currentCallsList[callId].callUriLen
                                 + BEARER_CALL_LIST_ELEMENT_SIZE;

            /* Assemble resonse for this call element of the list */
            listElement[0] = elementSize-1;
            listElement[1] = telephoneBearerServer->data.currentCallsList[callId].callId;
            listElement[2] = telephoneBearerServer->data.currentCallsList[callId].callState;
            listElement[3] = telephoneBearerServer->data.currentCallsList[callId].callFlags;

            CsrMemMove(callArray+nextPtr,
                    &listElement,
                    BEARER_CALL_LIST_ELEMENT_SIZE);

            if(telephoneBearerServer->data.currentCallsList[callId].callUriLen != 0 &&
               telephoneBearerServer->data.currentCallsList[callId].callUri != NULL )
            {
                CsrMemMove(callArray+nextPtr+BEARER_CALL_LIST_ELEMENT_SIZE,
                        telephoneBearerServer->data.currentCallsList[callId].callUri,
                        telephoneBearerServer->data.currentCallsList[callId].callUriLen);
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
uint8* tbsGetCallStateList(const GTBS_T *telephoneBearerServer, uint16* len)
{
    uint8 callId = 0;
    uint8* callStateArray = NULL;
    uint16 arraySize = 0;
    uint16 nextPtr = 0;
    
    /* determine the size of the array required */ 
    while(callId < TBS_CURRENT_CALLS_LIST_SIZE)
    {
        /* check if this element is in use */
        if(telephoneBearerServer->data.currentCallsList[callId].callId != FREE_CALL_SLOT)
        {
            /* add the size of this element.  Add one octet for the length */
            arraySize += sizeof(TbsCallStateChracteristic);
        }
        callId++;
    }    
    
    /* check if any calls found */
    if(arraySize == 0)
    {
        *len = 0;
        return NULL;
    }
    
    callStateArray = CsrPmemZalloc(arraySize);
    
    callId = 0;
    while(callId < TBS_CURRENT_CALLS_LIST_SIZE)
    {   /* check if this element is in use */
        if(telephoneBearerServer->data.currentCallsList[callId].callId != FREE_CALL_SLOT )
        {            
            TbsCallStateChracteristic csElement;
            csElement.callId = telephoneBearerServer->data.currentCallsList[callId].callId;
            csElement.callState = telephoneBearerServer->data.currentCallsList[callId].callState;      
            csElement.callFlags = telephoneBearerServer->data.currentCallsList[callId].callFlags;
            CsrMemMove(callStateArray+nextPtr, &csElement, sizeof(TbsCallStateChracteristic));
            nextPtr += sizeof(TbsCallStateChracteristic);
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
        GTBS_T *const telephoneBearerServer,
        GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd
        )
{
    if (accessInd->flags & ATT_ACCESS_READ)
    {
        sendTbsServerAccessRsp(
                telephoneBearerServer->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                BEARER_TECHNOLOGY_SIZE,
                (uint8*)&(telephoneBearerServer->data.technology)
                );
    }
    else
    {
        sendTbsServerAccessErrorRsp(
                telephoneBearerServer->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
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
        GTBS_T *const telephoneBearerServer,
        GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd
        )
{
    if (accessInd->flags & ATT_ACCESS_READ)
    {
        sendTbsServerAccessRsp(
                telephoneBearerServer->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                telephoneBearerServer->data.providerNameLen,
                (uint8*)telephoneBearerServer->data.providerName
                );
    }
    else
    {
        sendTbsServerAccessErrorRsp(
                telephoneBearerServer->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
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
        GTBS_T *const telephoneBearerServer,
        GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd
        )
{
    if (accessInd->flags & ATT_ACCESS_READ)
    {
        sendTbsServerAccessRsp(
                telephoneBearerServer->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                telephoneBearerServer->data.uciLen,
                (uint8*)telephoneBearerServer->data.uci
                );
    }
    else
    {
        sendTbsServerAccessErrorRsp(
                telephoneBearerServer->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
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
        GTBS_T *const telephoneBearerServer,
        GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd
        )
{
    if (accessInd->flags & ATT_ACCESS_READ)
    {
        sendTbsServerAccessRsp(
                telephoneBearerServer->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                telephoneBearerServer->data.uriPrefixesLen,
                (uint8*)(telephoneBearerServer->data.uriPrefixesList)
                );
    }
    else
    {
        sendTbsServerAccessErrorRsp(
                telephoneBearerServer->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
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
        GTBS_T *const telephoneBearerServer,
        GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd
        )
{
    if (accessInd->flags & ATT_ACCESS_READ)
    {
        uint16 len = 0;
        uint8* callList = tbsGetCallList(telephoneBearerServer, &len);
        
        sendTbsServerAccessRsp(
                telephoneBearerServer->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                len,
                callList
                );
                
        if(callList)
            CsrPmemFree(callList);
    }
    else
    {
        sendTbsServerAccessErrorRsp(
                telephoneBearerServer->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
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
        GTBS_T *const telephoneBearerServer,
        GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd
        )
{
    if (accessInd->flags & ATT_ACCESS_READ)
    {
        sendTbsServerAccessRsp(
                telephoneBearerServer->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                CONTENT_CONTROL_ID_SIZE,
                (uint8*)&(telephoneBearerServer->data.contentControlId)
                );
    }
    else
    {
        sendTbsServerAccessErrorRsp(
                telephoneBearerServer->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
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
        GTBS_T *const telephoneBearerServer,
        GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd
        )
{
    /* Create a buffer and assemble response */
    uint16 bufSize = telephoneBearerServer->data.incomingTargetBearerUri.uriLen +
                     sizeof(telephoneBearerServer->data.incomingTargetBearerUri.callId);
    uint8* buffer = CsrPmemAlloc(bufSize);

    CsrMemMove(buffer, &telephoneBearerServer->data.incomingTargetBearerUri.callId,
            sizeof(telephoneBearerServer->data.incomingTargetBearerUri.callId));
    CsrMemMove(buffer + sizeof(telephoneBearerServer->data.incomingTargetBearerUri.callId),
            telephoneBearerServer->data.incomingTargetBearerUri.uri,
            telephoneBearerServer->data.incomingTargetBearerUri.uriLen);

    if (accessInd->flags & ATT_ACCESS_READ)
    {
        sendTbsServerAccessRsp(
                telephoneBearerServer->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                bufSize,
                buffer
                );
    }
    else
    {
        sendTbsServerAccessErrorRsp(
                telephoneBearerServer->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                );
    }

    CsrPmemFree(buffer);
}

/***************************************************************************
NAME
    tbsHandleSupportedFeaturesAccess

DESCRIPTION
    Deals with access of the HANDLE_SUPPORTED_FEATEURES handle.
*/

static void tbsHandleSupportedFeaturesAccess(
        GTBS_T *const telephoneBearerServer,
        GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd
        )
{
    if (accessInd->flags & ATT_ACCESS_READ)
    {
        sendTbsServerAccessRsp(
                telephoneBearerServer->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                SUPPORTED_FEATURES_SIZE,
                (uint8*)&(telephoneBearerServer->data.statusFlags)
                );
    }
    else
    {
        sendTbsServerAccessErrorRsp(
                telephoneBearerServer->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
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
        GTBS_T *const telephoneBearerServer,
        GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd
        )
{
 
    if (accessInd->flags & ATT_ACCESS_READ)
    {
        uint16 len = 0;
        uint8* callStateList = tbsGetCallStateList(telephoneBearerServer, &len);
        
        sendTbsServerAccessRsp(
                telephoneBearerServer->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                len,
                callStateList
                );
                               
        if(callStateList)
            CsrPmemFree(callStateList);
    }
    else
    {
        sendTbsServerAccessErrorRsp(
                telephoneBearerServer->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
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
        GTBS_T *const telephoneBearerServer,
        GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd
        )
{
    if (accessInd->flags & ATT_ACCESS_READ)
    {
        sendTbsServerAccessRsp(
                telephoneBearerServer->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                CCP_OPCODES_SIZE,
                (uint8*)&(telephoneBearerServer->data.callControlPointOpcodes)
                );
    }
    else
    {
        sendTbsServerAccessErrorRsp(
                telephoneBearerServer->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
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
        GTBS_T *const telephoneBearerServer,
        GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd
        )
{
    if (accessInd->flags & ATT_ACCESS_READ)
    {
        /* Create a buffer and assemble response */
        uint16 bufSize = telephoneBearerServer->data.callFriendlyName.nameLen +
                         sizeof(telephoneBearerServer->data.callFriendlyName.callId);
        uint8* buffer = CsrPmemAlloc(bufSize);

        CsrMemMove(buffer, &telephoneBearerServer->data.callFriendlyName.callId,
                sizeof(telephoneBearerServer->data.callFriendlyName.callId));
        CsrMemMove(buffer + sizeof(telephoneBearerServer->data.callFriendlyName.callId),
                telephoneBearerServer->data.callFriendlyName.friendlyName,
                telephoneBearerServer->data.callFriendlyName.nameLen);

        sendTbsServerAccessRsp(
                telephoneBearerServer->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                bufSize,
                buffer);

        CsrPmemFree(buffer);
    }
    else
    {
        sendTbsServerAccessErrorRsp(
                telephoneBearerServer->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
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
        GTBS_T *const telephoneBearerServer,
        GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd
        )
{
    if (accessInd->flags & ATT_ACCESS_READ)
    {
        /* Create a buffer and assemble response */
        uint16 bufSize;
        uint8* buffer;
        char nullChar = 0x00;
        char* uri;
        uint16 uriLen;
        /* Find the call in the database */
        uint8 callIndex = tbsFindCall(telephoneBearerServer, telephoneBearerServer->data.incomingCall.callId);

        if(callIndex != FREE_CALL_SLOT &&
           telephoneBearerServer->data.currentCallsList[callIndex].callUri != NULL &&
           telephoneBearerServer->data.currentCallsList[callIndex].callUriLen != 0)
        {
            uri = telephoneBearerServer->data.currentCallsList[callIndex].callUri;
            uriLen = telephoneBearerServer->data.currentCallsList[callIndex].callUriLen;
        }
        else
        {
            /* Call URI not found so just send Null */
            uri = &nullChar;
            uriLen = sizeof(nullChar);
        }

        bufSize = sizeof(telephoneBearerServer->data.incomingCall.callId) + uriLen;
        buffer = CsrPmemAlloc(bufSize);
        CsrMemMove(buffer, &telephoneBearerServer->data.incomingCall.callId,
                   sizeof(telephoneBearerServer->data.incomingCall.callId));
        CsrMemMove(buffer + sizeof(telephoneBearerServer->data.incomingCall.callId),
                   uri,
                   uriLen);

        sendTbsServerAccessRsp(
                telephoneBearerServer->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                bufSize,
                buffer);

        CsrPmemFree(buffer);
    }
    else
    {
        sendTbsServerAccessErrorRsp(
                telephoneBearerServer->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
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
        GTBS_T *const telephoneBearerServer,
        GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd
        )
{
    if (accessInd->flags & ATT_ACCESS_READ)
    {
        sendTbsServerAccessRsp(
                telephoneBearerServer->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                SIGNAL_STRENGTH_SIZE,
                (uint8*)&(telephoneBearerServer->data.signalStrength)
                );
    }
    else
    {
        sendTbsServerAccessErrorRsp(
                telephoneBearerServer->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
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
        GTBS_T *const telephoneBearerServer,
        GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd
        )
{
    if (accessInd->flags & ATT_ACCESS_READ)
    {
        sendTbsServerAccessRsp(
        telephoneBearerServer->gattId,
        accessInd->cid,
        accessInd->handle,
        CSR_BT_GATT_ACCESS_RES_SUCCESS,
        SIGNAL_STRENGTH_INTERVAL_SIZE,
        (uint8*)&(telephoneBearerServer->data.signalStrengthReportingInterval)
        );
    }/* should never use long write here but better to be safe */
    else if ((accessInd->flags & ATT_ACCESS_WRITE) && accessInd->value != NULL)
    {
        MAKE_TBS_MESSAGE( GattTelephoneBearerServerSignalStrengthIntervalInd );

        /* Store the written value */
        telephoneBearerServer->data.signalStrengthReportingInterval = accessInd->value[0];

        gattTelephoneBearerServerWriteGenericResponse(
                    telephoneBearerServer->gattId,
                    accessInd->cid,
                    CSR_BT_GATT_ACCESS_RES_SUCCESS,
                    HANDLE_SIGNAL_STRENGTH_REPORTING_INTERVAL
                    );

        /* Send Indication to the application that the Signal Strength Interval has changed */
        message->srvcHndl = telephoneBearerServer->srvcHandle;
        message->cid = accessInd->cid;
        message->interval = telephoneBearerServer->data.signalStrengthReportingInterval;

        TbsMessageSend(telephoneBearerServer->appTask,
                    GATT_TELEPHONE_BEARER_SERVER_SIGNAL_STRENGTH_INTERVAL_IND,
                    message
                    );
    }
    else
    {
        sendTbsServerAccessErrorRsp(
                telephoneBearerServer->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                );
    }
}

GattTbsCcpNotificationResultCodes tbsPrevalidateCcpOpcode(const ServiceHandle srvcHndl,
                                                                 TbsCallControlPointChracteristic *controlPoint,
                                                                    uint16 size,
                                                                    uint8* opcode,
                                                                    uint8* callId)
{
    GattTbsCcpNotificationResultCodes result;
    GattTbsCallStates currentCallState, newCallState;
    uint16 numJoinCalls; /* number of calls to join */
    uint8* joinCallsList; /* list of calls to join */
    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);

    if(telephoneBearerServer == NULL)
    {
        return TBS_CCP_RESULT_LACK_OF_RESOURCES;
    }

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
        else
        {
            uint8 joinCallNum = 0; /* index of the call in the join list being checked */
            bool validCallId = TRUE;

            /* Validate the callids in the join list actually exist */
            while(validCallId && joinCallNum < numJoinCalls)
            {
                uint8 nextCallId = *(joinCallsList+joinCallNum);
                validCallId = GattTelephoneBearerServerGetCallState(srvcHndl,
                                                                    nextCallId,
                                                                    &currentCallState );
                if(!validCallId)
                {   /* Call ID not valid */
                    return TBS_CCP_RESULT_INVALID_CALL_INDEX;
                }
                else
                {
                    uint8 dupCheckCallNum = 0;

                    /* Find call index from the call id */
                    uint8 i = tbsFindCall(telephoneBearerServer, nextCallId);

                    /* Validate that Join opcode can be used on a call in this state */
                    newCallState = validateCcpOpcodeCallId(srvcHndl, i, *opcode );
                    if(newCallState == TBS_CALL_STATE_INVALID)
                    {
                        return TBS_CCP_RESULT_OPERATION_NOT_POSSIBLE;
                    }

                    /* Validate the call number is not duplicated in the Join list */
                    while(dupCheckCallNum < numJoinCalls)
                    {
                        uint8 dupCheckNextCallId = *(joinCallsList+dupCheckCallNum);
                        if((joinCallNum != dupCheckCallNum) &&
                           (nextCallId == dupCheckNextCallId))
                        {
                            return TBS_CCP_RESULT_OPERATION_NOT_POSSIBLE;
                        }

                        dupCheckCallNum++;
                    }
                }

                joinCallNum++;
            }
        }
    }
    else
    {   /* All other opcodes except join */
        /* Validate the call id and get the current call state */
        uint8 i;

        *callId = controlPoint->param1[0];
        if(!GattTelephoneBearerServerGetCallState(srvcHndl,
                                                  *callId,
                                                  &currentCallState))
        {
            return TBS_CCP_RESULT_INVALID_CALL_INDEX;
        }

        /* Find call index from the call id */
        i = tbsFindCall(telephoneBearerServer, *callId);

        /* Validate that this opcode can be used on a call in this state */
        newCallState = validateCcpOpcodeCallId(srvcHndl, i, *opcode );
        if(newCallState == TBS_CALL_STATE_INVALID)
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
        GTBS_T *telephoneBearerServer,
        GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd
        )
{
    if (accessInd->flags & ATT_ACCESS_WRITE)
    {
        /* This may contain multiple control point opcodes */
        uint16 ccpSize = accessInd->size_value;
        uint8 result, callId, opcode;

        gattTelephoneBearerServerWriteGenericResponse(
                                 telephoneBearerServer->gattId,
                                 accessInd->cid,
                                 CSR_BT_GATT_ACCESS_RES_SUCCESS,
                                 HANDLE_CALL_CONTROL_POINT
                                 );

        if ((accessInd->size_value == 0) || (accessInd->value == NULL))
        {
            return; /* Long gatt write access permission */
        }

        /* Prevalidate the control point access before we notify the app */
        result = tbsPrevalidateCcpOpcode(telephoneBearerServer->srvcHandle,
                                         (TbsCallControlPointChracteristic*)accessInd->value,
                                         ccpSize, &opcode, &callId);

        if(result != TBS_CCP_RESULT_SUCCESS)
        {
            TbsCallControlPointNotification ccpn;

            ccpn.resultCode = result;
            ccpn.opcode = opcode;
            ccpn.callId = callId;
            GattTelephoneBearerServerCallControlPointResponse(telephoneBearerServer->srvcHandle,
                                                              &ccpn);

        }
        else
        {
            /* Send Indication to the application, notifications are sent upon response to this message */
            MAKE_TBS_MESSAGE_WITH_LEN_U8(GattTelephoneBearerServerCallControlPointInd, ccpSize);

            message->srvcHndl = telephoneBearerServer->srvcHandle;
            message->cid = accessInd->cid;
            message->cpLen = ccpSize;
            memmove((void*)&message->controlPoint, accessInd->value, ccpSize);

            TbsMessageSend(telephoneBearerServer->appTask,
                       GATT_TELEPHONE_BEARER_SERVER_CALL_CONTROL_POINT_IND,
                       message
                       );
        }
    }
    else
    {
        sendTbsServerAccessErrorRsp(
                telephoneBearerServer->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                );
    }
}


/***************************************************************************/
bool tbsFindCid(const GTBS_T *telephoneBearerServer, connection_id_t cid, uint8 *index)
{
    uint8 i;
    bool res = FALSE;

    for(i=0; i<TBS_MAX_CONNECTIONS; i++)
    {
        if(telephoneBearerServer->data.connectedClients[i].cid == cid)
        {
            (*index) = i;
            res = TRUE;
        }
    }

    return res;
}

/***************************************************************************/
void tbsHandleAccessIndication(
        GTBS_T *telephoneBearerServer,
        GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd
        )
{
    uint8 i;

    if(tbsFindCid(telephoneBearerServer, accessInd->cid, &i))
    {
        switch (accessInd->handle)
        {

            case HANDLE_BEARER_PROVIDER_NAME:
            {
                tbsHandleBearerProviderNameAccess(
                            telephoneBearerServer,
                            accessInd
                            );
                break;
            }            
            
            case HANDLE_BEARER_PROVIDER_NAME_CLIENT_CONFIG:
            {
                if (accessInd->flags & ATT_ACCESS_READ)
                {
                    tbsHandleReadClientConfigAccess(
                                telephoneBearerServer->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                telephoneBearerServer->data.connectedClients[i].clientCfg.providerNameClientCfg
                                );
                }
                else if (accessInd->flags & ATT_ACCESS_WRITE)
                {
                    uint16 newCcc = 0;
                    tbsHandleWriteClientConfigAccess(
                                                telephoneBearerServer->gattId,
                                                accessInd,
                                                &newCcc
                                                );
                    telephoneBearerServer->data.connectedClients[i].clientCfg.providerNameClientCfg = TBS_CCC_MASK(newCcc);

                }
                else
                {
                    sendTbsServerAccessErrorRsp(
                                telephoneBearerServer->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                                );
                }
                break;
            }

            case HANDLE_BEARER_UCI:
                tbsHandleBearerUciAccess(
                            telephoneBearerServer,
                            accessInd
                            );
                break;


            case HANDLE_BEARER_TECHNOLOGY:
                tbsHandleBearerTechnologyAccess(
                            telephoneBearerServer,
                            accessInd
                            );
                break;            
                
            case HANDLE_BEARER_TECHNOLOGY_CLIENT_CONFIG:
            {
                if (accessInd->flags & ATT_ACCESS_READ)
                {
                    tbsHandleReadClientConfigAccess(
                                telephoneBearerServer->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                telephoneBearerServer->data.connectedClients[i].clientCfg.bearerTechnologyClientCfg
                                );
                }   
                else if (accessInd->flags & ATT_ACCESS_WRITE)
                {
                    uint16 newCcc = 0;
                    tbsHandleWriteClientConfigAccess(
                                                telephoneBearerServer->gattId,
                                                accessInd,
                                                &newCcc
                                                ); 
                    telephoneBearerServer->data.connectedClients[i].clientCfg.bearerTechnologyClientCfg = TBS_CCC_MASK(newCcc);
                }                
                else
                {
                    sendTbsServerAccessErrorRsp(
                                telephoneBearerServer->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                                );
                }
                break;                
            }          

            case HANDLE_BEARER_URI_PREFIX_LIST:
                tbsHandleBearerUriPrefixAccess(
                            telephoneBearerServer,
                            accessInd
                            );
                break;

            case HANDLE_BEARER_URI_PREFIX_LIST_CLIENT_CONFIG:
            {
                if (accessInd->flags & ATT_ACCESS_READ)
                {
                    tbsHandleReadClientConfigAccess(
                                telephoneBearerServer->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                telephoneBearerServer->data.connectedClients[i].clientCfg.bearerUriPrefixListClientCfg
                                );
                }
                else if (accessInd->flags & ATT_ACCESS_WRITE)
                {
                    uint16 newCcc = 0;
                    tbsHandleWriteClientConfigAccess(
                                                telephoneBearerServer->gattId,
                                                accessInd,
                                                &newCcc
                                                );
                    telephoneBearerServer->data.connectedClients[i].clientCfg.bearerUriPrefixListClientCfg = TBS_CCC_MASK(newCcc);
                }
                else
                {
                    sendTbsServerAccessErrorRsp(
                                telephoneBearerServer->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                                );
                }
                break;
            }
                

            case HANDLE_SIGNAL_STRENGTH:
                tbsHandleSignalStrengthAccess(
                            telephoneBearerServer,
                            accessInd
                            );
                break;

            case HANDLE_SIGNAL_STRENGTH_CLIENT_CONFIG:
            {
                if (accessInd->flags & ATT_ACCESS_READ)
                {
                    tbsHandleReadClientConfigAccess(
                                telephoneBearerServer->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                telephoneBearerServer->data.connectedClients[i].clientCfg.signalStrengthClientCfg
                                );
                }   
                else if (accessInd->flags & ATT_ACCESS_WRITE)
                {
                    uint16 newCcc = 0;
                    tbsHandleWriteClientConfigAccess(
                                                telephoneBearerServer->gattId,
                                                accessInd,
                                                &newCcc
                                                );
                    telephoneBearerServer->data.connectedClients[i].clientCfg.signalStrengthClientCfg = TBS_CCC_MASK(newCcc);
                }
                else
                {
                    sendTbsServerAccessErrorRsp(
                                telephoneBearerServer->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                                );
                }
                break;                
            }                 
            case HANDLE_SIGNAL_STRENGTH_REPORTING_INTERVAL:
                tbsHandleSignalStrengthIntervalAccess(
                            telephoneBearerServer,
                            accessInd
                            );
                break;
            
            case HANDLE_LIST_CURRENT_CALLS:
                tbsHandleListCurrentCallsAccess(
                            telephoneBearerServer,
                            accessInd
                            );
                break;
            
            case HANDLE_CURRENT_CALLS_CLIENT_CONFIG:
            {
                if (accessInd->flags & ATT_ACCESS_READ)
                {
                    tbsHandleReadClientConfigAccess(
                                telephoneBearerServer->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                telephoneBearerServer->data.connectedClients[i].clientCfg.currentCallsListClientCfg
                                );
                }   
                else if (accessInd->flags & ATT_ACCESS_WRITE)
                {
                    uint16 newCcc = 0;
                    tbsHandleWriteClientConfigAccess(
                                                telephoneBearerServer->gattId,
                                                accessInd,
                                                &newCcc
                                                );   
                    telephoneBearerServer->data.connectedClients[i].clientCfg.currentCallsListClientCfg = TBS_CCC_MASK(newCcc);

                }                
                else
                {
                    sendTbsServerAccessErrorRsp(
                                telephoneBearerServer->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                                );
                }
                break;                
            }                
            
            case HANDLE_CONTENT_CONTROL_ID:
                tbsHandleContentControlIdAccess(
                            telephoneBearerServer,
                            accessInd
                            );
                break;  
                     
            case HANDLE_INCOMING_CALL_TARGET_BEARER_URI:
                tbsHandleIncomingCallTargetUriAccess(
                            telephoneBearerServer,
                            accessInd
                            );
                break;
                
            case HANDLE_INCOMING_CALL_TARGET_BEARER_URI_CLIENT_CONFIG:
            {
                if (accessInd->flags & ATT_ACCESS_READ)
                {
                    tbsHandleReadClientConfigAccess(
                                telephoneBearerServer->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                telephoneBearerServer->data.connectedClients[i].clientCfg.incomingTargetUriClientCfg
                                );
                }   
                else if (accessInd->flags & ATT_ACCESS_WRITE)
                {
                    uint16 newCcc = 0;
                    tbsHandleWriteClientConfigAccess(
                                                telephoneBearerServer->gattId,
                                                accessInd,
                                                &newCcc
                                                );  
                    telephoneBearerServer->data.connectedClients[i].clientCfg.incomingTargetUriClientCfg = TBS_CCC_MASK(newCcc);
                }                
                else
                {
                    sendTbsServerAccessErrorRsp(
                                telephoneBearerServer->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                                );
                }
                break;
            }             
            case HANDLE_STATUS_FLAGS:
                tbsHandleSupportedFeaturesAccess(
                            telephoneBearerServer,
                            accessInd
                            );
                break;

            case HANDLE_STATUS_FLAGS_CLIENT_CONFIG:
            {
                if (accessInd->flags & ATT_ACCESS_READ)
                {
                    tbsHandleReadClientConfigAccess(
                                telephoneBearerServer->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                telephoneBearerServer->data.connectedClients[i].clientCfg.statusFlagsClientCfg
                                );
                }   
                else if (accessInd->flags & ATT_ACCESS_WRITE)
                {
                    uint16 newCcc = 0;
                    tbsHandleWriteClientConfigAccess(
                                                telephoneBearerServer->gattId,
                                                accessInd,
                                                &newCcc
                                                ); 
                    telephoneBearerServer->data.connectedClients[i].clientCfg.statusFlagsClientCfg = TBS_CCC_MASK(newCcc);
                }                
                else
                {
                    sendTbsServerAccessErrorRsp(
                                telephoneBearerServer->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                                );
                }
                break;                
            }      
            
            case HANDLE_CALL_STATE:
                tbsHandleCallStateAccess(
                            telephoneBearerServer,
                            accessInd
                            );
                break;        
                
            case HANDLE_CALL_STATE_CLIENT_CONFIG:
            {
                if (accessInd->flags & ATT_ACCESS_READ)
                {
                    tbsHandleReadClientConfigAccess(
                                telephoneBearerServer->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                telephoneBearerServer->data.connectedClients[i].clientCfg.callStateClientCfg
                                );
                }   
                else if (accessInd->flags & ATT_ACCESS_WRITE)
                {
                    uint16 newCcc = 0;
                    tbsHandleWriteClientConfigAccess(
                                                telephoneBearerServer->gattId,
                                                accessInd,
                                                &newCcc
                                                );
                    telephoneBearerServer->data.connectedClients[i].clientCfg.callStateClientCfg = TBS_CCC_MASK(newCcc);
                }                
                else
                {
                    sendTbsServerAccessErrorRsp(
                                telephoneBearerServer->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                                );
                }
                break;                
            }               
                
            case HANDLE_CALL_CONTROL_POINT:
            {
                tbsHandleCallControlPointAccess(
                            telephoneBearerServer,
                            accessInd
                            );
            }
            break;

            case HANDLE_CALL_CONTROL_POINT_OPCODES:
            {
                tbsHandleCallControlPointOpcodesAccess(
                            telephoneBearerServer,
                            accessInd
                            );
            }
            break;
            
            case HANDLE_CALL_CONTROL_POINT_CLIENT_CONFIG:
            {
                if (accessInd->flags & ATT_ACCESS_READ)
                {
                    tbsHandleReadClientConfigAccess(
                                telephoneBearerServer->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                telephoneBearerServer->data.connectedClients[i].clientCfg.callControlPointClientCfg
                                );
                }
                else if (accessInd->flags & ATT_ACCESS_WRITE)
                {
                    uint16 newCcc = 0;
                    tbsHandleWriteClientConfigAccess(
                                telephoneBearerServer->gattId,
                                accessInd,
                                &newCcc
                                );
                    telephoneBearerServer->data.connectedClients[i].clientCfg.callControlPointClientCfg = TBS_CCC_MASK(newCcc);
                }
                else
                {
                    sendTbsServerAccessErrorRsp(
                                telephoneBearerServer->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                                );
                }
                break;
            }

            case HANDLE_TERMINATION_REASON:
            {
                /* This characteristic is notify only so send error response */
                sendTbsServerAccessErrorRsp(
                    telephoneBearerServer->gattId,
                    accessInd->cid,
                    accessInd->handle,
                    CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                    );
            }
            break;
            
            case HANDLE_TERMINATION_REASON_CLIENT_CONFIG:
            {
                if (accessInd->flags & ATT_ACCESS_READ)
                {
                    tbsHandleReadClientConfigAccess(
                                telephoneBearerServer->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                telephoneBearerServer->data.connectedClients[i].clientCfg.terminationReasonClientCfg
                                );
                }   
                else if (accessInd->flags & ATT_ACCESS_WRITE)
                {
                    uint16 newCcc = 0;
                    tbsHandleWriteClientConfigAccess(
                                            telephoneBearerServer->gattId,
                                            accessInd,
                                            &newCcc
                                            );       
                    telephoneBearerServer->data.connectedClients[i].clientCfg.terminationReasonClientCfg = TBS_CCC_MASK(newCcc);
                }                
                else
                {
                    sendTbsServerAccessErrorRsp(
                                telephoneBearerServer->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                                );
                }
                break;                
            }                

            case HANDLE_REMOTE_FRIENDLY_NAME:
                tbsHandleCallFriendlyNameAccess(
                            telephoneBearerServer,
                            accessInd
                            );
                break;

            case HANDLE_REMOTE_FRIENDLY_NAME_CLIENT_CONFIG:
            {
                if (accessInd->flags & ATT_ACCESS_READ)
                {
                    tbsHandleReadClientConfigAccess(
                                telephoneBearerServer->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                telephoneBearerServer->data.connectedClients[i].clientCfg.callFriendlyNameClientCfg
                                );
                }
                else if (accessInd->flags & ATT_ACCESS_WRITE)
                {
                    uint16 newCcc = 0;
                    tbsHandleWriteClientConfigAccess(
                                                telephoneBearerServer->gattId,
                                                accessInd,
                                                &newCcc
                                                );
                    telephoneBearerServer->data.connectedClients[i].clientCfg.callFriendlyNameClientCfg = TBS_CCC_MASK(newCcc);
                }
                else
                {
                    sendTbsServerAccessErrorRsp(
                                telephoneBearerServer->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                                );
                }
                break;
            }

            case HANDLE_INCOMING_CALL:
                tbsHandleIncomingCallAccess(
                            telephoneBearerServer,
                            accessInd
                            );
                break;

            case HANDLE_INCOMING_CALL_CLIENT_CONFIG:
            {
                if (accessInd->flags & ATT_ACCESS_READ)
                {
                    tbsHandleReadClientConfigAccess(
                                telephoneBearerServer->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                telephoneBearerServer->data.connectedClients[i].clientCfg.incomingCallClientCfg
                                );
                }
                else if (accessInd->flags & ATT_ACCESS_WRITE)
                {
                    uint16 newCcc = 0;
                    tbsHandleWriteClientConfigAccess(
                                                telephoneBearerServer->gattId,
                                                accessInd,
                                                &newCcc);

                    telephoneBearerServer->data.connectedClients[i].clientCfg.incomingCallClientCfg = TBS_CCC_MASK(newCcc);
                }
                else
                {
                    sendTbsServerAccessErrorRsp(
                                telephoneBearerServer->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                                );
                }
                break;
            }


            default:
            {
                sendTbsServerAccessErrorRsp(
                            telephoneBearerServer->gattId,
                            accessInd->cid,
                            accessInd->handle,
                            CSR_BT_GATT_ACCESS_RES_INVALID_HANDLE
                            );
                break;
            }
        } /* switch */
    }
    else
    {
        GATT_TBS_SERVER_ERROR(
                    "GTBS: No valid Cid!\n"
                    );
    }
}


void gattTelephoneBearerServerNotifyCallControlPoint(GTBS_T *telephoneBearerServer, TbsCallControlPointNotification *ccpn, uint8* clientIndex)
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
        if (telephoneBearerServer->data.connectedClients[i].cid != 0)
        {
            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (telephoneBearerServer->data.connectedClients[i].clientCfg.callControlPointClientCfg == CLIENT_CONFIG_NOTIFY)
            {
                tbsServerSendCharacteristicChangedNotification(
                        telephoneBearerServer->gattId,
                        telephoneBearerServer->data.connectedClients[i].cid,
                        HANDLE_CALL_CONTROL_POINT,
                        sizeof(TbsCallControlPointNotification),
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
void gattTelephoneBearerServerNotifyCurrentCalls(GTBS_T *telephoneBearerServer, uint8* clientIndex)
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
        if (telephoneBearerServer->data.connectedClients[i].cid != 0)
        {
            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (telephoneBearerServer->data.connectedClients[i].clientCfg.currentCallsListClientCfg == CLIENT_CONFIG_NOTIFY)
            {
                uint16 len = 0;
                uint8* callList = tbsGetCallList(telephoneBearerServer, &len);

                tbsServerSendCharacteristicChangedNotification(
                        telephoneBearerServer->gattId,
                        telephoneBearerServer->data.connectedClients[i].cid,
                        HANDLE_LIST_CURRENT_CALLS,
                        len,
                        callList
                        );

                if(callList)
                    CsrPmemFree(callList);
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


void gattTelephoneBearerServerNotifyIncomingCallTargetUri(GTBS_T *telephoneBearerServer, uint8* clientIndex)
{
    uint16 i=0;
    /* Create a buffer and assemble response */
    uint16 bufSize = telephoneBearerServer->data.incomingTargetBearerUri.uriLen +
                     sizeof(telephoneBearerServer->data.incomingTargetBearerUri.callId);
    uint8* buffer = CsrPmemAlloc(bufSize);

    CsrMemMove(buffer, &telephoneBearerServer->data.incomingTargetBearerUri.callId,
            sizeof(telephoneBearerServer->data.incomingTargetBearerUri.callId));
    CsrMemMove(buffer + sizeof(telephoneBearerServer->data.incomingTargetBearerUri.callId),
            telephoneBearerServer->data.incomingTargetBearerUri.uri,
            telephoneBearerServer->data.incomingTargetBearerUri.uriLen);

    /* if an index of a client has been specified notify that one only */
    if(clientIndex != NULL)
    {
        i = *clientIndex;
    }

    /* Notify connected clients of the change */
    for (; i<TBS_MAX_CONNECTIONS; i++)
    {
        if (telephoneBearerServer->data.connectedClients[i].cid != 0)
        {
            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (telephoneBearerServer->data.connectedClients[i].clientCfg.incomingTargetUriClientCfg == CLIENT_CONFIG_NOTIFY)
            {
                tbsServerSendCharacteristicChangedNotification(
                        telephoneBearerServer->gattId,
                        telephoneBearerServer->data.connectedClients[i].cid,
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

    CsrPmemFree(buffer);
}


void gattTelephoneBearerServerNotifyCallState(GTBS_T *telephoneBearerServer, uint8* clientIndex)
{
    uint16 i =0;
    uint16 len = 0;
    uint8* callStateList = NULL;

    callStateList = tbsGetCallStateList(telephoneBearerServer, &len);

    /* if an index of a client has been specified notify that one only */
    if(clientIndex != NULL)
    {
        i = *clientIndex;
    }

    /* Notify connected clients of the change */
    for (; i<TBS_MAX_CONNECTIONS; i++)
    {
        if (telephoneBearerServer->data.connectedClients[i].cid != 0)
        {
            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (telephoneBearerServer->data.connectedClients[i].clientCfg.callStateClientCfg == CLIENT_CONFIG_NOTIFY)
            {
                tbsServerSendCharacteristicChangedNotification(
                        telephoneBearerServer->gattId,
                        telephoneBearerServer->data.connectedClients[i].cid,
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
        CsrPmemFree(callStateList);

}

void gattTelephoneBearerServerNotifyCallFriendlyName(GTBS_T *telephoneBearerServer, uint8* clientIndex)
{
    uint16 i=0;

    /* Create a buffer and assemble response */
    uint16 bufSize = telephoneBearerServer->data.callFriendlyName.nameLen +
                     sizeof(telephoneBearerServer->data.callFriendlyName.callId);
    uint8* buffer = CsrPmemAlloc(bufSize);

    CsrMemMove(buffer, &telephoneBearerServer->data.callFriendlyName.callId,
            sizeof(telephoneBearerServer->data.callFriendlyName.callId));
    CsrMemMove(buffer + sizeof(telephoneBearerServer->data.callFriendlyName.callId),
            telephoneBearerServer->data.callFriendlyName.friendlyName,
            telephoneBearerServer->data.callFriendlyName.nameLen);

    /* if an index of a client has been specified notify that one only */
    if(clientIndex != NULL)
    {
        i = *clientIndex;
    }

    /* Notify connected clients of the change */
    for (; i<TBS_MAX_CONNECTIONS; i++)
    {
        if (telephoneBearerServer->data.connectedClients[i].cid != 0)
        {
            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (telephoneBearerServer->data.connectedClients[i].clientCfg.callFriendlyNameClientCfg == CLIENT_CONFIG_NOTIFY)
            {
                tbsServerSendCharacteristicChangedNotification(
                        telephoneBearerServer->gattId,
                        telephoneBearerServer->data.connectedClients[i].cid,
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

    CsrPmemFree(buffer);
}

void gattTelephoneBearerServerNotifySignalStrength(GTBS_T *telephoneBearerServer, uint8* clientIndex)
{
    uint16 i=0;

    /* Flag to block further notifications of signal strength */
    telephoneBearerServer->data.signalStrengthTimerFlag = TRUE;

    /* Store the last value actually notified */
    telephoneBearerServer->data.signalStrengthNotified = telephoneBearerServer->data.signalStrength;

    /* if an index of a client has been specified notify that one only */
    if(clientIndex != NULL)
    {
        i = *clientIndex;
    }

    /* Notify connected clients of the change */
    for (; i<TBS_MAX_CONNECTIONS; i++)
    {
        if (telephoneBearerServer->data.connectedClients[i].cid != 0)
        {
            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (telephoneBearerServer->data.connectedClients[i].clientCfg.signalStrengthClientCfg == CLIENT_CONFIG_NOTIFY)
            {
                tbsServerSendCharacteristicChangedNotification(
                        telephoneBearerServer->gattId,
                        telephoneBearerServer->data.connectedClients[i].cid,
                        HANDLE_SIGNAL_STRENGTH,
                        SIGNAL_STRENGTH_SIZE,
                        (uint8*)&telephoneBearerServer->data.signalStrength
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

    if(telephoneBearerServer->data.signalStrengthReportingInterval == 0)
    {
        telephoneBearerServer->data.signalStrengthTimerFlag = FALSE;
    }
    else
    {
        /* Block further signal strength reports until the timer fires */
        MAKE_TBS_MESSAGE(GattTelephoneBearerServerInternalSignalStrengthTimer);
        message->id = GATT_TELEPHONE_BEARER_SERVER_INTERNAL_SIGNAL_STRENGTH_TIMER;

        tId = CsrSchedTimerSet(1000000*(telephoneBearerServer->data.signalStrengthReportingInterval),
                               &TbsMessageSendLater,
                               telephoneBearerServer->libTask,
                               (void*)message);

    }
}


void gattTelephoneBearerServerNotifyIncomingCall(GTBS_T *telephoneBearerServer, uint8* clientIndex)
{   
    uint16 i=0;
    uint8 callSlot;

    /* Find call id */
    callSlot = tbsFindCall(telephoneBearerServer, telephoneBearerServer->data.incomingCall.callId);

    if(callSlot == FREE_CALL_SLOT)
    {
        GATT_TBS_SERVER_DEBUG("GTBS: NotifyIncomingCall - No valid incoming call id!\n");
        return;
    }
    else
    {
        /* Create a buffer and assemble response */
        uint16 bufSize;
        uint8* buffer;
        char nullChar = 0x00;
        char* uri;
        uint16 uriLen;
        /* Find the call in the database */
        uint8 callIndex = tbsFindCall(telephoneBearerServer, telephoneBearerServer->data.incomingCall.callId);

        if(callIndex != FREE_CALL_SLOT &&
           telephoneBearerServer->data.currentCallsList[callIndex].callUri != NULL &&
           telephoneBearerServer->data.currentCallsList[callIndex].callUriLen != 0)
        {
            uri = telephoneBearerServer->data.currentCallsList[callIndex].callUri;
            uriLen = telephoneBearerServer->data.currentCallsList[callIndex].callUriLen;
        }
        else
        {
            /* Call URI not found so just send Null */
            uri = &nullChar;
            uriLen = sizeof(nullChar);
        }

        bufSize = sizeof(telephoneBearerServer->data.incomingCall.callId) + uriLen;
        buffer = CsrPmemAlloc(bufSize);
        CsrMemMove(buffer, &telephoneBearerServer->data.incomingCall.callId,
                   sizeof(telephoneBearerServer->data.incomingCall.callId));
        CsrMemMove(buffer + sizeof(telephoneBearerServer->data.incomingCall.callId),
                   uri,
                   uriLen);


        /* if an index of a client has been specified notify that one only */
        if(clientIndex != NULL)
        {
            i = *clientIndex;
        }

        /* Notify connected clients of the change */
        for (; i<TBS_MAX_CONNECTIONS; i++)
        {
            if (telephoneBearerServer->data.connectedClients[i].cid != 0)
            {
                /* If the Client Config is 0x01 (Notify is TRUE), a notification will
                 * be sent to the client */
                if (telephoneBearerServer->data.connectedClients[i].clientCfg.incomingCallClientCfg == CLIENT_CONFIG_NOTIFY)
                {
                    tbsServerSendCharacteristicChangedNotification(
                            telephoneBearerServer->gattId,
                            telephoneBearerServer->data.connectedClients[i].cid,
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

        CsrPmemFree(buffer);
    }
}

void gattTelephoneBearerServerNotifyBearerUriPrefixList(GTBS_T *telephoneBearerServer, uint8* clientIndex)
{
    uint16 i=0;

    /* Create a buffer and assemble response */
    uint16 bufSize = telephoneBearerServer->data.uriPrefixesLen;
    uint8* buffer = CsrPmemAlloc(bufSize);

    CsrMemMove(buffer, telephoneBearerServer->data.uriPrefixesList, bufSize);

    /* if an index of a client has been specified notify that one only */
    if(clientIndex != NULL)
    {
        i = *clientIndex;
    }

    /* Notify connected clients of the change */
    for (; i<TBS_MAX_CONNECTIONS; i++)
    {
        if (telephoneBearerServer->data.connectedClients[i].cid != 0)
        {
            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (telephoneBearerServer->data.connectedClients[i].clientCfg.bearerUriPrefixListClientCfg == CLIENT_CONFIG_NOTIFY)
            {
                tbsServerSendCharacteristicChangedNotification(
                        telephoneBearerServer->gattId,
                        telephoneBearerServer->data.connectedClients[i].cid,
                        HANDLE_BEARER_URI_PREFIX_LIST,
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

    CsrPmemFree(buffer);
}

/* Something has changed in a call state, send all the notifications to do with that */
void gattTelephoneBearerServerNotifyCallChange(GTBS_T *telephoneBearerServer)
{
    gattTelephoneBearerServerNotifyCallState(telephoneBearerServer, NULL);
    gattTelephoneBearerServerNotifyCurrentCalls(telephoneBearerServer, NULL);
}
#if 0
void PanicNull(void* ptr)
{
    if(ptr == NULL)
    {
        GATT_TBS_SERVER_PANIC("NULL POINTER!");
    }
}
#endif



