/******************************************************************************
 Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#include<stdio.h>
#include "csr_pmem.h"

#include "gatt_telephone_bearer_server_private.h"
#include "gatt_telephone_bearer_server_msg_handler.h"
#include "gatt_telephone_bearer_server_access.h"
#include "gatt_telephone_bearer_server_common.h"
#include "gatt_telephone_bearer_server_debug.h"

ServiceHandle tbsServiceHandle;
CsrSchedTid tId;

void initTbsClientData(TbsClientData* pCd);


/* tbsInitClientData

    Inits(zeros) a tbs_client_data struct
*/
void initTbsClientData(TbsClientData* pCd)
{
    if(pCd)
    {
        pCd->cid = 0;
        pCd->clientCfg.providerNameClientCfg = 0;
        pCd->clientCfg.bearerTechnologyClientCfg = 0;
        pCd->clientCfg.signalStrengthClientCfg = 0;
        pCd->clientCfg.currentCallsListClientCfg = 0;
        pCd->clientCfg.incomingTargetUriClientCfg = 0;
        pCd->clientCfg.statusFlagsClientCfg = 0;
        pCd->clientCfg.callStateClientCfg = 0;
        pCd->clientCfg.callControlPointClientCfg = 0;
        pCd->clientCfg.terminationReasonClientCfg = 0;
        pCd->clientCfg.callFriendlyNameClientCfg = 0;
        pCd->clientCfg.incomingCallClientCfg = 0;
        pCd->clientCfg.bearerUriPrefixListClientCfg = 0;
    }
}


/******************************************************************************/
ServiceHandle GattTelephoneBearerServerInit(AppTask    appTask,
                                            uint16  startHandle,
                                            uint16  endHandle,
                                            GattTbsInitData* initData)
{
    uint16 clientIndex = 0;
    GTBS_T  *telephoneBearerServer = NULL;
    ServiceHandle newServiceHandle = 0;
    CsrBtGattId gattId;
    tId = 0;

    if(appTask == CSR_SCHED_QID_INVALID)
    {
        GATT_TBS_SERVER_PANIC("GattTelephoneBearerServerInit fail invalid params");
        return 0;
    }

    newServiceHandle = ServiceHandleNewInstance((void **) &telephoneBearerServer, sizeof(GTBS_T));
    tbsServiceHandle = newServiceHandle;

    if (newServiceHandle !=0)
    {

        uint16 callId = 0;

        /* Reset all the service library memory */
        CsrMemSet(telephoneBearerServer, 0, sizeof(GTBS_T));

        telephoneBearerServer->srvcHandle =  newServiceHandle;

        /* Set up library handler for external messages */
        telephoneBearerServer->libTask = CSR_BT_TBS_SERVER_IFACEQUEUE;

        /* Store the Task function parameter.
           All library messages need to be sent here */
        telephoneBearerServer->appTask = appTask;

        /* Initialise starting call index id */
        telephoneBearerServer->data.nextCallId = 1;

        /* Initialisation of the TBS Characteristics */
        telephoneBearerServer->data.providerName = NULL;
        telephoneBearerServer->data.providerNameLen = 0;
        telephoneBearerServer->data.uci =  NULL;
        telephoneBearerServer->data.uciLen = 0;
        telephoneBearerServer->data.uriPrefixesList = NULL;
        telephoneBearerServer->data.uriPrefixesLen = 0;
        telephoneBearerServer->data.signalStrength = TBS_SIGNAL_STRENGTH_UNAVAILABLE;
        memset(&telephoneBearerServer->data.incomingTargetBearerUri,0,sizeof(telephoneBearerServer->data.incomingTargetBearerUri));
        memset(&telephoneBearerServer->data.callFriendlyName,0,sizeof(telephoneBearerServer->data.callFriendlyName));
        memset(&telephoneBearerServer->data.incomingCall,0,sizeof(telephoneBearerServer->data.incomingCall));

        /* init the call ids to 0xFF so we know they're unused */
        memset(&telephoneBearerServer->data.currentCallsList,0,sizeof(telephoneBearerServer->data.currentCallsList));
        while(callId < TBS_CURRENT_CALLS_LIST_SIZE )
        {
            telephoneBearerServer->data.currentCallsList[callId].callId = FREE_CALL_SLOT;
            telephoneBearerServer->data.currentCallsList[callId].callState = TBS_CALL_STATE_INVALID;
            callId++;
        }

        /* Update with supplied data if provided */
        if(initData != NULL)
        {
            uint16 len = 0;

            len = initData->providerNameLen;
            if(len != 0 && initData->providerName != NULL)
            {
                telephoneBearerServer->data.providerNameLen = len;
                telephoneBearerServer->data.providerName = CsrPmemAlloc(len);
                CsrMemMove(telephoneBearerServer->data.providerName,
                        initData->providerName,
                        len);
            }

            len = initData->uciLen;
            if(len != 0 && initData->uci != NULL)
            {
                telephoneBearerServer->data.uciLen = len;
                telephoneBearerServer->data.uci = CsrPmemAlloc(len);
                CsrMemMove(telephoneBearerServer->data.uci, initData->uci, len);
            }

            len = initData->uriPrefixesLen;
            if(len != 0 && initData->uriPrefixesList != NULL)
            {
                telephoneBearerServer->data.uriPrefixesLen = len;
                telephoneBearerServer->data.uriPrefixesList = CsrPmemAlloc(len);
                CsrMemMove(telephoneBearerServer->data.uriPrefixesList, initData->uriPrefixesList, len);
            }

            if(initData->signalStrength <= TBS_SIGNAL_STRENGTH_MAX)
            {
                telephoneBearerServer->data.signalStrength = initData->signalStrength;
            }

            telephoneBearerServer->data.technology = initData->technology;
            telephoneBearerServer->data.signalStrengthReportingInterval = initData->signalStrengthReportingInterval;
            telephoneBearerServer->data.contentControlId = initData->contentControlId ;
            telephoneBearerServer->data.statusFlags = initData->statusFlags;
            telephoneBearerServer->data.callControlPointOpcodes = initData->callControlPointOpcodes;

            CsrMemMove(telephoneBearerServer->data.currentCallsList,
                    initData->currentCallsList,
                    sizeof(initData->currentCallsList));

            /* List may contain pointers to URI strings so they need to be copied separately */
            callId = 0;
            while(callId < TBS_CURRENT_CALLS_LIST_SIZE )
            {
                if(telephoneBearerServer->data.currentCallsList[callId].callId != FREE_CALL_SLOT)
                {
                    uint16 uriLen = telephoneBearerServer->data.currentCallsList[callId].callUriLen;
                    if(uriLen != 0)
                    {
                        initData->currentCallsList[callId].callUri = (char*)CsrPmemZalloc(uriLen);
                        CsrMemMove(telephoneBearerServer->data.currentCallsList[callId].callUri,
                                initData->currentCallsList[callId].callUri,
                                uriLen);
                    }
                    else
                    {
                        initData->currentCallsList[callId].callUri = NULL;
                    }
                }

                callId++;
            }

            len = initData->incomingTargetBearerUri.uriLen;
            if(len != 0 && initData->incomingTargetBearerUri.uri != NULL)
            {
                telephoneBearerServer->data.incomingTargetBearerUri.callId = initData->incomingTargetBearerUri.callId;
                telephoneBearerServer->data.incomingTargetBearerUri.uriLen = len;
                telephoneBearerServer->data.incomingTargetBearerUri.uri = (char*)CsrPmemZalloc(len);
                CsrMemMove(telephoneBearerServer->data.incomingTargetBearerUri.uri,
                        initData->incomingTargetBearerUri.uri,
                        len);
            }

            len = telephoneBearerServer->data.callFriendlyName.nameLen;
            if(len != 0 && telephoneBearerServer->data.callFriendlyName.friendlyName != NULL)
            {
                telephoneBearerServer->data.callFriendlyName.callId = initData->callFriendlyName.callId;
                telephoneBearerServer->data.callFriendlyName.nameLen = len;
                telephoneBearerServer->data.callFriendlyName.friendlyName = (char*)CsrPmemZalloc(len);
                CsrMemMove(telephoneBearerServer->data.callFriendlyName.friendlyName,
                        initData->callFriendlyName.friendlyName,
                        len);
            }

            telephoneBearerServer->data.incomingCall.callId = initData->incomingCall.callId;
            /* Check 0 was not provided as the Call ID */
            telephoneBearerServer->data.nextCallId = initData->nextCallId != 0 ? initData->nextCallId : 1;
        }
        else
        {   
			/* Set some defaults that won't be set by provided init values */
            telephoneBearerServer->data.technology = 0;
            telephoneBearerServer->data.signalStrengthReportingInterval = 0;
            telephoneBearerServer->data.contentControlId = 0;
            telephoneBearerServer->data.statusFlags = 0;
            telephoneBearerServer->data.callControlPointOpcodes = 0;
        }



        /* Init connected clients */
        for (clientIndex=0; clientIndex<TBS_MAX_CONNECTIONS; clientIndex++)
        {
            initTbsClientData(&telephoneBearerServer->data.connectedClients[clientIndex]);
        }

        /* Register with the GATT  */
        gattId = CsrBtGattRegister(telephoneBearerServer->libTask);
        /* verify the result */
        if (gattId == CSR_BT_GATT_INVALID_GATT_ID)
        {
            ServiceHandleFreeInstanceData(newServiceHandle);
            newServiceHandle = 0;
        }
        else
        {
            telephoneBearerServer->gattId = gattId;
            CsrBtGattFlatDbRegisterHandleRangeReqSend(gattId, startHandle,endHandle);
        }
    }
    return newServiceHandle;
}


/******************************************************************************/
status_t GattTelephoneBearerServerAddConfig(
                  ServiceHandle srvcHndl,
                  connection_id_t cid,
                  const GattTbsServerConfig *config)
{
    uint8 i;
    GTBS_T *telephoneBearerServer = (GTBS_T*) ServiceHandleGetInstanceData(srvcHndl);

    if(telephoneBearerServer == NULL)
    {   /* Service handle not found */
        return CSR_BT_GATT_RESULT_UNACCEPTABLE_PARAMETER;
    }

    for(i=0; i<TBS_MAX_CONNECTIONS; i++)
    {
        if(telephoneBearerServer->data.connectedClients[i].cid == 0)
        {
            /* Add client btConnId to the server data */
            telephoneBearerServer->data.connectedClients[i].cid = cid;

            /* Check config parameter:
             * If config is NULL, it indicates a default config should be used for the
             * peer device identified by the CID.
             * The default config is already set when the instance has been initialised.
             */
            if (config)
            {
                /* Check these characteristics can be only notified */
                if (config->clientCfgs.bearerTechnologyClientCfg == CLIENT_CONFIG_INDICATE ||
                    config->clientCfgs.callControlPointClientCfg == CLIENT_CONFIG_INDICATE ||
                    config->clientCfgs.callFriendlyNameClientCfg == CLIENT_CONFIG_INDICATE ||
                    config->clientCfgs.callStateClientCfg == CLIENT_CONFIG_INDICATE ||
                    config->clientCfgs.currentCallsListClientCfg == CLIENT_CONFIG_INDICATE ||
                    config->clientCfgs.incomingCallClientCfg == CLIENT_CONFIG_INDICATE ||
                    config->clientCfgs.incomingTargetUriClientCfg == CLIENT_CONFIG_INDICATE ||
                    config->clientCfgs.providerNameClientCfg == CLIENT_CONFIG_INDICATE ||
                    config->clientCfgs.signalStrengthClientCfg == CLIENT_CONFIG_INDICATE ||
                    config->clientCfgs.statusFlagsClientCfg == CLIENT_CONFIG_INDICATE ||
                    config->clientCfgs.terminationReasonClientCfg == CLIENT_CONFIG_INDICATE ||
                    config->clientCfgs.bearerUriPrefixListClientCfg == CLIENT_CONFIG_INDICATE)
                {                    
                    GATT_TBS_SERVER_ERROR("Invalid Client Configuration Characteristic!\n");
                    return CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED;
                }

                /* Save new ccc for the client */
                CsrMemMove(&telephoneBearerServer->data.connectedClients[i].clientCfg,
                        &config->clientCfgs,
                        sizeof(TbsClientCfgData));

                /* Notify any characteristics that are set */
                if (config->clientCfgs.bearerTechnologyClientCfg == CLIENT_CONFIG_NOTIFY)
                {
                    tbsServerSendCharacteristicChangedNotification(
                            telephoneBearerServer->gattId,
                            telephoneBearerServer->data.connectedClients[i].cid,
                            HANDLE_BEARER_TECHNOLOGY,
                            BEARER_TECHNOLOGY_SIZE,
                            (uint8*)&telephoneBearerServer->data.technology
                            );
                }

                if (config->clientCfgs.providerNameClientCfg == CLIENT_CONFIG_NOTIFY)
                {
                    tbsServerSendCharacteristicChangedNotification(
                            telephoneBearerServer->gattId,
                            telephoneBearerServer->data.connectedClients[i].cid,
                            HANDLE_BEARER_PROVIDER_NAME,
                            telephoneBearerServer->data.providerNameLen,
                            (uint8*)telephoneBearerServer->data.providerName
                            );
                }

                if (config->clientCfgs.statusFlagsClientCfg == CLIENT_CONFIG_NOTIFY)
                {
                    tbsServerSendCharacteristicChangedNotification(
                            telephoneBearerServer->gattId,
                            telephoneBearerServer->data.connectedClients[i].cid,
                            HANDLE_STATUS_FLAGS,
                            sizeof(telephoneBearerServer->data.statusFlags),
                            (uint8*)&telephoneBearerServer->data.statusFlags
                            );
                }

                /* Notify the rest of the characteristcs */
                gattTelephoneBearerServerNotifyCallFriendlyName(telephoneBearerServer, &i);
                gattTelephoneBearerServerNotifyCallState(telephoneBearerServer, &i);
                gattTelephoneBearerServerNotifyCurrentCalls(telephoneBearerServer, &i);
                gattTelephoneBearerServerNotifyIncomingCall(telephoneBearerServer, &i);
                gattTelephoneBearerServerNotifyIncomingCallTargetUri(telephoneBearerServer, &i);
                gattTelephoneBearerServerNotifySignalStrength(telephoneBearerServer, &i);

                /* Note do not notify the following two characteristics as they're not
                 * relevant unless there has been an action */
                /* config->clientCfgs.callControlPointClientCfg
                   config->clientCfgs.terminationReasonClientCfg */
            }

            /* Update of the specified client is complete */
            return CSR_BT_GATT_ACCESS_RES_SUCCESS;
        }
    }
    return CSR_BT_GATT_ACCESS_RES_INSUFFICIENT_RESOURCES;
}

/******************************************************************************/
GattTbsServerConfig * GattTelephoneBearerServerRemoveConfig(
                              ServiceHandle srvcHndl,
                              connection_id_t  cid)
{
    uint8 i;
    GTBS_T *telephoneBearerServer = (GTBS_T *) ServiceHandleGetInstanceData(srvcHndl);
    GattTbsServerConfig *config = NULL;

    if(telephoneBearerServer == NULL)
    {   /* Service handle not found */
        return NULL;
    }

    for(i=0; i<TBS_MAX_CONNECTIONS; i++)
    {
        /* Check the saved CID to find the peeer device */
        if (telephoneBearerServer->data.connectedClients[i].cid == cid)
        {
            /* Found peer device:
             * - save last client configurations
             * - remove the peer device
             * - free the server instance
             * - return last client configuration
             */

            config = CsrPmemAlloc(sizeof(GattTbsServerConfig));
            CsrMemMove(&config->clientCfgs, &(telephoneBearerServer->data.connectedClients[i].clientCfg), sizeof(TbsClientCfgData));

            if ((i == (TBS_MAX_CONNECTIONS-1)) || (i == 0 && telephoneBearerServer->data.connectedClients[i+1].cid == 0))
            {
                /* The peer device is the only or the last element of the array */
                memset(&(telephoneBearerServer->data.connectedClients[i]), 0, sizeof(TbsClientData));
            }
            else
            {
                /* The peer device is in the middle of the array */
                uint8 j;

                for (j=i; j<(TBS_MAX_CONNECTIONS - 1) && telephoneBearerServer->data.connectedClients[j+1].cid != 0; j++)
                {
                    /* Shift all the elements of the array of one position behind */
                    CsrMemMove(&(telephoneBearerServer->data.connectedClients[j]),
                           &(telephoneBearerServer->data.connectedClients[j+1]),
                           sizeof(TbsClientData));
                }

                /* Remove the last element of the array, already shifted behind */
                memset(&(telephoneBearerServer->data.connectedClients[j]), 0, sizeof(TbsClientData));
            }
        }
    }

    return config;
}


bool GattTelephoneBearerServerSetProviderName(const ServiceHandle srvcHndl, char* providerName, uint16 len)
{
    uint16 i;
    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer == NULL)
    {
        return FALSE;
    }

    if(len == 0)
    {
        if(telephoneBearerServer->data.providerName)
        {
            CsrPmemFree(telephoneBearerServer->data.providerName);
        }
        telephoneBearerServer->data.providerName = NULL;
        telephoneBearerServer->data.providerNameLen = 0;
    }
    else
    {
        char* oldName = telephoneBearerServer->data.providerName;
        telephoneBearerServer->data.providerNameLen = (len>PROVIDER_NAME_SIZE? PROVIDER_NAME_SIZE : len );
        telephoneBearerServer->data.providerName =
                (char*)MemRealloc(telephoneBearerServer->data.providerName, telephoneBearerServer->data.providerNameLen);
        /* check if MemRealloc failed */
        if (telephoneBearerServer->data.providerName == NULL)
        {
            if(oldName)
            {
                CsrPmemFree(oldName);
            }
            telephoneBearerServer->data.providerNameLen = 0;
        }
        else
        {
            CsrMemCpy(telephoneBearerServer->data.providerName, providerName,
                      telephoneBearerServer->data.providerNameLen);
        }

    }

    for (i=0; i<TBS_MAX_CONNECTIONS; i++)
    {
        if (telephoneBearerServer->data.connectedClients[i].cid != 0)
        {
            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (telephoneBearerServer->data.connectedClients[i].clientCfg.providerNameClientCfg == CLIENT_CONFIG_NOTIFY)
            {
                tbsServerSendCharacteristicChangedNotification(
                        telephoneBearerServer->gattId,
                        telephoneBearerServer->data.connectedClients[i].cid,
                        HANDLE_BEARER_PROVIDER_NAME,
                        telephoneBearerServer->data.providerNameLen,
                        (uint8*)telephoneBearerServer->data.providerName
                        );
            }
            else
            {
                /* Characteristic not configured for Notification - Nothing to do */
            }
        }
    }


    return TRUE;
}


uint16 GattTelephoneBearerServerGetProviderName(const ServiceHandle srvcHndl, char** providerName)
{
    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer == NULL ||
       telephoneBearerServer->data.providerNameLen == 0)
    {
        return 0;
    }

    {
        char* name_string = (char*)CsrPmemAlloc(sizeof(char) * telephoneBearerServer->data.providerNameLen);
        CsrMemCpy(name_string, telephoneBearerServer->data.providerName, telephoneBearerServer->data.providerNameLen);

        *providerName = name_string;      

        return telephoneBearerServer->data.providerNameLen;
    }
}


bool GattTelephoneBearerServerSetUci(const ServiceHandle srvcHndl, char* uci, uint16 len)
{
    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer == NULL)
    {
        return FALSE;
    }

    if(len == 0)
    {
        if(telephoneBearerServer->data.uci)
        {
            CsrPmemFree(telephoneBearerServer->data.uci);
        }
        telephoneBearerServer->data.uci = NULL;
        telephoneBearerServer->data.uciLen = 0;
    }
    else
    {
        char* oldUci = telephoneBearerServer->data.uci;
        telephoneBearerServer->data.uciLen = (len>BEARER_UCI_SIZE? BEARER_UCI_SIZE : len );
        telephoneBearerServer->data.uci =
                (char*)MemRealloc(telephoneBearerServer->data.uci, telephoneBearerServer->data.uciLen);
        /* check if MemRealloc failed */
        if (telephoneBearerServer->data.uci == NULL)
        {
            if(oldUci)
            {
                CsrPmemFree(oldUci);
            }
            telephoneBearerServer->data.uciLen = 0;
            return FALSE;
        }
        else
        {
            CsrMemCpy(telephoneBearerServer->data.uci, uci,
                      telephoneBearerServer->data.uciLen);
        }

    }

    return TRUE;
}



uint16 GattTelephoneBearerServerGetUci(const ServiceHandle srvcHndl, char** uci)
{
    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    uint16 size;

    if(telephoneBearerServer == NULL)
    {
        return 0;
    }

    size = telephoneBearerServer->data.uciLen;

    {
        char* uciString = (char*)CsrPmemAlloc(sizeof(char) * size);

        CsrMemCpy(uciString, telephoneBearerServer->data.uci, size);

        *uci = uciString;

        return telephoneBearerServer->data.uciLen;
    }
}

bool GattTelephoneBearerServerSetTechnology(const ServiceHandle srvcHndl, GattTbsTechnology technology)
{
    uint16 i;
    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer == NULL)
    {
        return FALSE;
    }

    telephoneBearerServer->data.technology = (uint8)technology;

    for (i=0; i<TBS_MAX_CONNECTIONS; i++)
    {
        if (telephoneBearerServer->data.connectedClients[i].cid != 0)
        {
            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (telephoneBearerServer->data.connectedClients[i].clientCfg.bearerTechnologyClientCfg == CLIENT_CONFIG_NOTIFY)
            {
                tbsServerSendCharacteristicChangedNotification(
                        telephoneBearerServer->gattId,
                        telephoneBearerServer->data.connectedClients[i].cid,
                        HANDLE_BEARER_TECHNOLOGY,
                        BEARER_TECHNOLOGY_SIZE,
                        (uint8*)&telephoneBearerServer->data.technology
                        );
            }
            else
            {
                /* Characteristic not configured for Notification - Nothing to do */
            }
        }
    }

    return TRUE;
}


GattTbsTechnology GattTelephoneBearerServerGetTechnology(const ServiceHandle srvcHndl)
{
    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer == NULL)
    {
        return 0;
    }

    return telephoneBearerServer->data.technology;
}


bool GattTelephoneBearerServerSetUriPrefixList(const ServiceHandle srvcHndl, char* prefixList, uint16 length)
{  
    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer == NULL)
    {
        return FALSE;
    }

    if(prefixList)
    {
        char* newPrefixes = (char*)MemRealloc(telephoneBearerServer->data.uriPrefixesList, length);
        if(!newPrefixes)
        {
            /* Tidy up if MemRealloc failed */
            if(telephoneBearerServer->data.uriPrefixesList)
            {
                CsrPmemFree(telephoneBearerServer->data.uriPrefixesList);
                telephoneBearerServer->data.uriPrefixesList = NULL;
            }
            return FALSE;
        }

        CsrMemMove(newPrefixes, prefixList, length);
        telephoneBearerServer->data.uriPrefixesList = newPrefixes;
        telephoneBearerServer->data.uriPrefixesLen  = length;
    }


    /* Notify Clients */
    gattTelephoneBearerServerNotifyBearerUriPrefixList(telephoneBearerServer, NULL);

    return TRUE;
}

uint16 GattTelephoneBearerServerGetUriPrefix(const ServiceHandle srvcHndl, char** uriList)
{
    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer == NULL)
    {
        return 0;
    }

    if(!telephoneBearerServer->data.uriPrefixesList)
    {
        return 0;
    }
    else
    {
        char* uriListString = (char*)CsrPmemAlloc(sizeof(char) * telephoneBearerServer->data.uriPrefixesLen);

        CsrMemCpy(uriListString, telephoneBearerServer->data.uriPrefixesList, telephoneBearerServer->data.uriPrefixesLen);

        *uriList = uriListString;
        return telephoneBearerServer->data.uriPrefixesLen;
    }
}

bool GattTelephoneBearerServerFindUriPrefix(const ServiceHandle srvcHndl, char* prefix)
{
    bool r;
    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer == NULL)
    {
        return FALSE;
    }

    if(!telephoneBearerServer->data.uriPrefixesList)
    {
        return FALSE;
    }

    r = strstr(telephoneBearerServer->data.uriPrefixesList, prefix) != NULL;
    return(r);
}

bool GattTelephoneBearerServerSetSignalStrength(const ServiceHandle srvcHndl, uint8 signalStrength)
{
    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer == NULL)
    {
        return FALSE;
    }

    /* validate signal strength range */
    if (signalStrength > TBS_SIGNAL_STRENGTH_MAX && signalStrength != TBS_SIGNAL_STRENGTH_UNAVAILABLE)
    {
        GATT_TBS_SERVER_WARNING("\n TBS :Set signal strength - out of range\n");
        return FALSE;
    }

    telephoneBearerServer->data.signalStrength = signalStrength;

    if(telephoneBearerServer->data.signalStrengthTimerFlag)
    {
        return FALSE;
    }

    /* Notify Clients */
    gattTelephoneBearerServerNotifySignalStrength(telephoneBearerServer, NULL);

    return TRUE;
}


uint8 GattTelephoneBearerServerGetSignalStrength(const ServiceHandle srvcHndl)
{
    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer == NULL)
    {
        return 0;
    }

    return telephoneBearerServer->data.signalStrength;
}

bool GattTelephoneBearerServerSetSignalStrengthInterval(const ServiceHandle srvcHndl, uint8 interval)
{
    uint16 msgWaiting;
    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer == NULL)
    {
        return FALSE;
    }

    if(telephoneBearerServer->data.signalStrengthReportingInterval == interval)
    {
        /* No changes, don't do anything */
        return FALSE;
    }

    /* Store new interval */
    telephoneBearerServer->data.signalStrengthReportingInterval = interval;

    /* Cancel signal strength timer messages */
    msgWaiting = CsrSchedTimerCancel(tId,NULL,NULL);


    /* If we are waiting for a signal strength interval timer to fire, reset it with the new
     * timer */
    if(msgWaiting)
    {
        MAKE_TBS_MESSAGE(GattTelephoneBearerServerInternalSignalStrengthTimer);

        telephoneBearerServer->data.signalStrengthTimerFlag = TRUE;
        message->id = GATT_TELEPHONE_BEARER_SERVER_INTERNAL_SIGNAL_STRENGTH_TIMER;

        tId = CsrSchedTimerSet(1000000*(telephoneBearerServer->data.signalStrengthReportingInterval),
                         &TbsMessageSendLater,
                         telephoneBearerServer->libTask,
                         (void*)message);

    }
    else
    {
        /* Not currently waiting for a signal strength interval timer */
        telephoneBearerServer->data.signalStrengthTimerFlag = FALSE;
    }

    return TRUE;
}

uint8 GattTelephoneBearerServerGetSignalStrengthInterval(const ServiceHandle srvcHndl)
{
    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer == NULL)
    {
        return 0;
    }

    return telephoneBearerServer->data.signalStrengthReportingInterval;
}


GattTbsCcpNotificationResultCodes GattTelephoneBearerServerCreateCallEx(const ServiceHandle srvcHndl,
                                            GattTbsCallStates callState,
                                            GattTbsCallFlags callFlags,
                                            uint16 callUriSize,
                                            char* callUri,
                                            uint16 targetUriSize,
                                            char* targetUri,
                                            bool joinAllowed,
                                            uint8* newCallId)
{
    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer == NULL)
    {
        return TBS_CCP_RESULT_LACK_OF_RESOURCES;
    }

    /* Check for a valid call state */
    if(callState > TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD)
    {
        return TBS_CCP_RESULT_OPERATION_NOT_POSSIBLE;
    }

    /* Check URI is specified */
    if(callUriSize == 0 || callUri == NULL)
    {
        return TBS_CCP_RESULT_INVALID_OUTGOING_URI;
    }

    *newCallId = tbsAddCall(telephoneBearerServer, callState, callFlags, callUriSize, callUri, joinAllowed);
    if(*newCallId == FREE_CALL_SLOT)
        return TBS_CCP_RESULT_LACK_OF_RESOURCES;
    
    /* notify call state */
    gattTelephoneBearerServerNotifyCallState(telephoneBearerServer, NULL);

    switch (callState)
    {
        case TBS_CALL_STATE_INCOMING: /* Incoming call */
        {
            uint8 incCallId = *newCallId;
            telephoneBearerServer->data.incomingCall.callId = incCallId;

            gattTelephoneBearerServerNotifyIncomingCall(telephoneBearerServer, NULL);

            GattTelephoneBearerServerSetIncomingCallTargetUri(srvcHndl,
                                                              *newCallId,
                                                              targetUri,
                                                              targetUriSize);
        }
        break;

    case TBS_CALL_STATE_DIALING: /* Outgoing calls */
    case TBS_CALL_STATE_ALERTING:
        break;

    default:
        break;
    }

    /* Notify current calls for all call states */
    gattTelephoneBearerServerNotifyCurrentCalls(telephoneBearerServer, NULL);

    return TBS_CCP_RESULT_SUCCESS;
}

GattTbsCcpNotificationResultCodes GattTelephoneBearerServerCreateCall(const ServiceHandle srvcHndl,
                                            GattTbsCallStates callState,
                                            GattTbsCallFlags callFlags,
                                            uint16 callUriSize,
                                            char* callUri,
                                            uint16 targetUriSize,
                                            char* targetUri,
                                            uint8* newCallId)
{
    return GattTelephoneBearerServerCreateCallEx(srvcHndl,
                                                 callState,
                                                 callFlags,
                                                 callUriSize,
                                                 callUri,
                                                 targetUriSize,
                                                 targetUri,
                                                 TRUE,
                                                 newCallId);

}

TbsCurrentCallListChracteristic* GattTelephoneBearerServerGetCurrentCalls(const ServiceHandle srvcHndl)
{
    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer == NULL)
    {
        return NULL;
    }

    return telephoneBearerServer->data.currentCallsList;
}


bool GattTelephoneBearerServerSetContentControlId(const ServiceHandle srvcHndl, uint8 ccid)
{
    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer == NULL)
    {
        return FALSE;
    }

    telephoneBearerServer->data.contentControlId = ccid;

    return TRUE;
}

uint8 GattTelephoneBearerServerGetContentControlId(const ServiceHandle srvcHndl)
{
    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer == NULL)
    {
        return 0;
    }

    return telephoneBearerServer->data.contentControlId;
}

bool GattTelephoneBearerServerSetIncomingCallTargetUri(const ServiceHandle srvcHndl, uint8 callId, char* incomingUri, uint16 len)
{
    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer == NULL)
    {
        return FALSE;
    }

    telephoneBearerServer->data.incomingTargetBearerUri.callId = callId;

    if(len == 0)
    {
        if(telephoneBearerServer->data.incomingTargetBearerUri.uri)
        {
            CsrPmemFree(telephoneBearerServer->data.incomingTargetBearerUri.uri);
        }
        telephoneBearerServer->data.incomingTargetBearerUri.uri = NULL;
        telephoneBearerServer->data.incomingTargetBearerUri.uriLen = 0;
        telephoneBearerServer->data.incomingTargetBearerUri.callId = 0;

        return FALSE;
    }
    else
    {
        char* oldUri = telephoneBearerServer->data.incomingTargetBearerUri.uri;
        char* newUri;
        telephoneBearerServer->data.incomingTargetBearerUri.uriLen = (len>MAX_CALL_URI_SIZE? MAX_CALL_URI_SIZE : len );
        newUri = (char*)MemRealloc(telephoneBearerServer->data.incomingTargetBearerUri.uri,
                        telephoneBearerServer->data.incomingTargetBearerUri.uriLen);
        /* check if MemRealloc failed */
        if(newUri == NULL)
        {
            if(oldUri)
            {
                CsrPmemFree(oldUri);
            }
            telephoneBearerServer->data.incomingTargetBearerUri.uriLen = 0;
            telephoneBearerServer->data.incomingTargetBearerUri.callId = 0;

            return FALSE;
        }
        else
        {
            telephoneBearerServer->data.incomingTargetBearerUri.uri = newUri;
            CsrMemMove(telephoneBearerServer->data.incomingTargetBearerUri.uri,
                    incomingUri,
                    telephoneBearerServer->data.incomingTargetBearerUri.uriLen);
        }

    }

    /* Notify clients */
    gattTelephoneBearerServerNotifyIncomingCallTargetUri(telephoneBearerServer, NULL);

    return TRUE;
}


TbsIncomingCallTargetUriChracteristic* GattTelephoneBearerServerGetIncomingCallTargetUri(const ServiceHandle srvcHndl)
{
    TbsIncomingCallTargetUriChracteristic* retUri = NULL;

    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer != NULL)
    {
        retUri = (TbsIncomingCallTargetUriChracteristic*)
                     CsrPmemZalloc(sizeof(TbsIncomingCallTargetUriChracteristic));

        retUri->callId = telephoneBearerServer->data.incomingTargetBearerUri.callId;
        retUri->uriLen = telephoneBearerServer->data.incomingTargetBearerUri.uriLen;
        if (retUri->uriLen > 0)
        {
            retUri->uri = (char*)CsrPmemZalloc(retUri->uriLen);
            CsrMemMove(retUri->uri,
                    telephoneBearerServer->data.incomingTargetBearerUri.uri,
                    retUri->uriLen);
        }
        else
        {
            retUri->uri = NULL;
        }
    }

    return retUri;
}

bool GattTelephoneBearerServerSetStatusFlags(const ServiceHandle srvcHndl, uint16 flags)
{
    uint16 i;

    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer == NULL)
    {
        return FALSE;
    }

    /* Validate flags  */
    if(flags & ~TBS_STATUS_FLAGS_ALL)
    {
        return FALSE;
    }

    /* Assign new feature flags */
    telephoneBearerServer->data.statusFlags = flags;

    /* Notify connected clients of the change */
    for (i=0; i<TBS_MAX_CONNECTIONS; i++)
    {
        if (telephoneBearerServer->data.connectedClients[i].cid != 0)
        {
            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (telephoneBearerServer->data.connectedClients[i].clientCfg.statusFlagsClientCfg == CLIENT_CONFIG_NOTIFY)
            {
                tbsServerSendCharacteristicChangedNotification(
                        telephoneBearerServer->gattId,
                        telephoneBearerServer->data.connectedClients[i].cid,
                        HANDLE_STATUS_FLAGS,
                        sizeof(telephoneBearerServer->data.statusFlags),
                        (uint8*)&telephoneBearerServer->data.statusFlags
                        );
            }
            else
            {
                /* Characteristic not configured for Notification - Nothing to do */
            }
        }
    }

    return TRUE;
}

uint16 GattTelephoneBearerServerGetStatusFlags(const ServiceHandle srvcHndl)
{
    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer == NULL)
    {
        return 0;
    }

    return telephoneBearerServer->data.statusFlags;
}

bool GattTelephoneBearerServerSetCallState(const ServiceHandle srvcHndl,
                                            uint8 callId,
                                            GattTbsCallStates callState,
                                            const bool notify)
{
    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer == NULL)
    {
        return FALSE;
    }

    /* Check for a valid call state */
    if(callState > TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD)
    {
        return FALSE;
    }

    if(!tbsSetCallState(telephoneBearerServer, callId, callState))
    {
        return FALSE;
    }

    /* Notify Clients */
    if(notify)
    {
        gattTelephoneBearerServerNotifyCallChange(telephoneBearerServer);
    }

    return TRUE;
    
}

bool GattTelephoneBearerServerNotifyCallState(const ServiceHandle srvcHndl)
{
    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer == NULL)
    {
        return FALSE;
    }

    gattTelephoneBearerServerNotifyCallChange(telephoneBearerServer);

    return TRUE;

}

bool GattTelephoneBearerServerCallControlPointAcceptOpcode(const ServiceHandle srvcHndl,
                                            uint8 callId,
                                            GattTbsCallControlPointOpcode opcode,
                                            GattTbsCallStates callState)
{
    uint8 callIndex = 0;
    GattTbsCallStates currentCallState;

    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer == NULL)
    {
        return FALSE;
    }

    /* check if the call exists */
    callIndex = tbsFindCall(telephoneBearerServer, callId);
    if(callIndex == FREE_CALL_SLOT)
    {
        return FALSE;
    }

    currentCallState = telephoneBearerServer->data.currentCallsList[callIndex].callState;

    /* Check and update the call state of other calls */
    switch(opcode)
    {
        case TBS_OPCODE_ACCEPT:
            tbsSetAllCallState(telephoneBearerServer, TBS_CALL_STATE_ACTIVE, TBS_CALL_STATE_LOCALLY_HELD);
            tbsSetAllCallState(telephoneBearerServer, TBS_CALL_STATE_REMOTELY_HELD, TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD);
        break;

        case TBS_OPCODE_LOCAL_RETRIEVE:
            if(currentCallState == TBS_CALL_STATE_LOCALLY_HELD)
            {
                tbsSetAllCallState(telephoneBearerServer, TBS_CALL_STATE_ACTIVE, TBS_CALL_STATE_LOCALLY_HELD);
            }
            else if (currentCallState == TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD ||
                     currentCallState == TBS_CALL_STATE_REMOTELY_HELD)
            {
                tbsSetAllCallState(telephoneBearerServer, TBS_CALL_STATE_ACTIVE, TBS_CALL_STATE_LOCALLY_HELD);
                tbsSetAllCallState(telephoneBearerServer, TBS_CALL_STATE_REMOTELY_HELD, TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD);
            }
        break;

        case TBS_OPCODE_ORIGINATE:
            tbsSetAllCallState(telephoneBearerServer, TBS_CALL_STATE_ACTIVE, TBS_CALL_STATE_LOCALLY_HELD);
        break;

        case TBS_OPCODE_JOIN:

        case TBS_OPCODE_LOCAL_HOLD:
        case TBS_OPCODE_TERMINATE:
        default:
            break;
    }

    tbsSetCallState(telephoneBearerServer, callId, callState);

    /* Notify Clients */
    gattTelephoneBearerServerNotifyCallChange(telephoneBearerServer);

    return TRUE;

}

bool GattTelephoneBearerServerCallControlPointAcceptJoin(const ServiceHandle srvcHndl,
                                                               uint8 numJoinCalls,
                                                               uint8* joinCallsList)
{
    uint8 callNum = 0;
    TbsCurrentCallListChracteristic *allCallsList; /*ptr to all calls */
    uint8 allCallNum;
    bool validCallId = TRUE;
    bool result = TBS_CCP_RESULT_SUCCESS;
    GattTbsCallStates currentCallState, newCallState = 0;

    if(joinCallsList == NULL ||
       numJoinCalls < 2 )
    {
        return FALSE;
    }

    /* Set state of all calls that are to be joined */
    callNum = 0;
    while(result == TBS_CCP_RESULT_SUCCESS &&
          callNum < numJoinCalls)
    {
        uint8 nextCallId = *(joinCallsList+callNum);
        /* Validate state */
        newCallState = GattTelephoneBearerServerCheckOpcodeState(srvcHndl,
                                                                 nextCallId,
                                                                 TBS_OPCODE_JOIN);
        if(newCallState != TBS_CALL_STATE_INVALID)
        {
            /* set the call state */
            GattTelephoneBearerServerSetCallState(srvcHndl,
                                                  nextCallId,
                                                  newCallState,
                                                  FALSE);
        }
        else
        {
            result = TBS_CCP_RESULT_OPERATION_NOT_POSSIBLE;
        }

        callNum++;
    }


    /* make sure any currently active calls are held, except the ones we want to join */
    allCallsList = GattTelephoneBearerServerGetCurrentCalls(srvcHndl);
    if(allCallsList == NULL)
    {
        return FALSE;
    }

    allCallNum = 0;
    while(result == TBS_CCP_RESULT_SUCCESS &&
          allCallNum < TBS_CURRENT_CALLS_LIST_SIZE)
    {
        TbsCurrentCallListChracteristic nextAllCall = *(allCallsList+allCallNum);

        validCallId = GattTelephoneBearerServerGetCallState(srvcHndl,
                                                            nextAllCall.callId,
                                                            &currentCallState );

        if(validCallId && currentCallState == TBS_CALL_STATE_ACTIVE)
        {
            bool callMatch = FALSE;
            callNum = 0;
            /*Check this isn't one of the calls we want to join */
            while(!callMatch &&
                  (callNum < numJoinCalls))
            {
                uint8 nextCallId = *(joinCallsList+callNum);

                if(nextCallId == nextAllCall.callId)
                {
                    /* this call is one of the ones we're joining
                     * so don't do anything with it */
                    callMatch = TRUE;
                }

                callNum++;
            }

            /* not one of the calls we're joining and it's active so set it to
             * Locally held */
            if(callMatch == FALSE)
            {
                /* set the call state */
                GattTelephoneBearerServerSetCallState(srvcHndl,
                                                      nextAllCall.callId,
                                                      TBS_CALL_STATE_LOCALLY_HELD,
                                                      FALSE);
            }
        }

        allCallNum++;
    }


    if(result == TBS_CCP_RESULT_SUCCESS)
    {
        /* Notify call state changes */
        GattTelephoneBearerServerNotifyCallState(srvcHndl);

        return TRUE;
    }

    return FALSE;
}


bool GattTelephoneBearerServerCallControlPointResponse(const ServiceHandle srvcHndl, TbsCallControlPointNotification *ccpn)
{
    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer == NULL)
    {
        return FALSE;
    }

    /* if the operation was not a success the call id should be 0 */
    if(ccpn->resultCode != TBS_CCP_RESULT_SUCCESS)
    {
        ccpn->callId = 0;
    }

    /* Notify connected clients of the change */
    gattTelephoneBearerServerNotifyCallControlPoint(telephoneBearerServer, ccpn, NULL );

    return TRUE;
}

bool GattTelephoneBearerServerSetControlPointOpcodes(const ServiceHandle srvcHndl, uint16 opcodes)
{
    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer == NULL)
    {
        return FALSE;
    }

    telephoneBearerServer->data.callControlPointOpcodes = opcodes;

    return TRUE;
}

uint16 GattTelephoneBearerServerGetCallControlPointOpcodes(const ServiceHandle srvcHndl)
{
    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer == NULL)
    {
        return 0;
    }

    return telephoneBearerServer->data.callControlPointOpcodes;
}

bool GattTelephoneBearerServerTerminateCall(const ServiceHandle srvcHndl, uint8 callId, GattTbsCallTerminationReason reason)
{
    uint16 i;
    bool retVal = TRUE;
    TbsTerminationReasonChracteristic tr;

    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer == NULL)
    {
        return FALSE;
    }
    
    /* Delete the call from the call list */
    if(!tbsDeleteCall(telephoneBearerServer, callId))
        retVal = FALSE; /* call does not exist */
    
    tr.callId = callId;
    tr.reason = reason;

    /* Notify connected clients of the change */
    for (i=0; i<TBS_MAX_CONNECTIONS; i++)
    {
        if (telephoneBearerServer->data.connectedClients[i].cid != 0)
        {
            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (telephoneBearerServer->data.connectedClients[i].clientCfg.terminationReasonClientCfg == CLIENT_CONFIG_NOTIFY)
            {              
                tbsServerSendCharacteristicChangedNotification(
                        telephoneBearerServer->gattId,
                        telephoneBearerServer->data.connectedClients[i].cid,
                        HANDLE_TERMINATION_REASON,
                        TERMINATION_REASON_SIZE,
                        (uint8*)&tr
                        );
            }
            else
            {
                /* Characteristic not configured for Notification - Nothing to do */
            }
        }
    }

    return retVal;

}

bool GattTelephoneBearerServerSetCallFriendlyName(const ServiceHandle srvcHndl, uint8 callId, uint16 len, char* name)
{    
    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer == NULL)
    {
        return FALSE;
    }

    if(len == 0)
    {
        if(telephoneBearerServer->data.callFriendlyName.friendlyName)
        {
            CsrPmemFree(telephoneBearerServer->data.callFriendlyName.friendlyName);
        }
        telephoneBearerServer->data.callFriendlyName.friendlyName = NULL;
        telephoneBearerServer->data.callFriendlyName.nameLen = 0;
        telephoneBearerServer->data.callFriendlyName.callId = 0;

        return FALSE;
    }
    else
    {
        char* oldFn = telephoneBearerServer->data.callFriendlyName.friendlyName;
        char* newFn;
        telephoneBearerServer->data.callFriendlyName.nameLen = (len>MAX_CALL_FRIENDLY_NAME_SIZE? MAX_CALL_FRIENDLY_NAME_SIZE : len );
        newFn = (char*)MemRealloc(telephoneBearerServer->data.callFriendlyName.friendlyName,
                                  telephoneBearerServer->data.callFriendlyName.nameLen);

        /* check if MemRealloc failed */
        if (newFn == NULL)
        {
            if(oldFn)
            {
                CsrPmemFree(oldFn);
            }
            telephoneBearerServer->data.callFriendlyName.nameLen = 0;
            telephoneBearerServer->data.callFriendlyName.callId = 0;

            return FALSE;
        }
        else
        {
            telephoneBearerServer->data.callFriendlyName.friendlyName = newFn;
            CsrMemMove(telephoneBearerServer->data.callFriendlyName.friendlyName,
                    name,
                    telephoneBearerServer->data.callFriendlyName.nameLen);
        }

        telephoneBearerServer->data.callFriendlyName.callId = callId;
    }

    /*Notify clients */
    gattTelephoneBearerServerNotifyCallFriendlyName(telephoneBearerServer, NULL);

    return TRUE;
}


TbsCallFriendlyNameChracteristic* GattTelephoneBearerServerGetRemoteFriendlyName(const ServiceHandle srvcHndl)
{
    TbsCallFriendlyNameChracteristic *retName = NULL;
    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer != NULL)
    {              
        retName = (TbsCallFriendlyNameChracteristic*)
                     CsrPmemZalloc(sizeof(TbsCallFriendlyNameChracteristic));
        retName->callId = telephoneBearerServer->data.callFriendlyName.callId;
        retName->nameLen = telephoneBearerServer->data.callFriendlyName.nameLen;

        if(retName->nameLen > 0)
        {
            retName->friendlyName = (char*)CsrPmemZalloc(retName->nameLen);
            CsrMemMove(retName->friendlyName,
                    telephoneBearerServer->data.callFriendlyName.friendlyName,
                    retName->nameLen);
        }
        else
        {
            retName->friendlyName = NULL;
        }
    }

    return retName;
}

bool GattTelephoneBearerServerGetCallState(const ServiceHandle srvcHndl, uint8 callId, GattTbsCallStates *state)
{
    uint8 callSlot;
    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer == NULL)
    {
        return FALSE;
    }

    /* Find call id */
    callSlot = tbsFindCall(telephoneBearerServer, callId);

    if((callSlot != FREE_CALL_SLOT) && state)
    {
        *state = (telephoneBearerServer->data.currentCallsList[callSlot].callState);
        return TRUE;
    }

    return FALSE;
}

GattTbsCcpNotificationResultCodes GattTelephoneBearerServerCheckOpcodeSupport(const ServiceHandle srvcHndl, uint8 opcode)
{
    uint16 ccpOptionalFlags = 0;

    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer == NULL)
    {
        return TBS_CCP_RESULT_LACK_OF_RESOURCES;
    }

    /* Check this opcode is not RFU */
    if(opcode >= TBS_OPCODE_RFU )
    {
        return TBS_CCP_RESULT_OPCODE_NOT_SUPPORTED;
    }

    ccpOptionalFlags = telephoneBearerServer->data.callControlPointOpcodes;

    /* Check Local Hold opcode */
    if(opcode == TBS_OPCODE_LOCAL_HOLD && !(ccpOptionalFlags & TBS_CCP_OPTIONAL_LOCAL_HOLD))
    {
        return TBS_CCP_RESULT_OPCODE_NOT_SUPPORTED;
    }

    /* Check the join opcode */
    if(opcode == TBS_OPCODE_JOIN && !(ccpOptionalFlags & TBS_CCP_OPTIONAL_JOIN))
    {
        return TBS_CCP_RESULT_OPCODE_NOT_SUPPORTED;
    }

    return TBS_CCP_RESULT_SUCCESS;
}

GattTbsCallStates GattTelephoneBearerServerCheckOpcodeState(const ServiceHandle srvcHndl, uint8 callId, uint8 opcode)
{
    uint8 i; /* call array index */

    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer == NULL)
    {
        return TBS_CALL_STATE_INVALID;
    }

    /* Find call index from the call id */
    i = tbsFindCall(telephoneBearerServer, callId);

    if(i == FREE_CALL_SLOT)
    {
        return TBS_CALL_STATE_INVALID;
    }

    return validateCcpOpcodeCallId(srvcHndl, i, opcode);
}

bool GattTelephoneBearerServerSetCallJoin(const ServiceHandle srvcHndl,
                                                uint8 callId,
                                                const bool joinAllowed)

{
    uint8 i; /* call array index */

    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer == NULL)
    {
        return FALSE;
    }

    /* Find call index from the call id */
    i = tbsFindCall(telephoneBearerServer, callId);

    if(i == FREE_CALL_SLOT)
    {
        return FALSE;
    }

    telephoneBearerServer->data.currentCallsList[i].allowJoin = joinAllowed;

    return TRUE;
}

bool GattTelephoneBearerServerGetCallJoin(const ServiceHandle srvcHndl, uint8 callId, bool *joinAllowed)

{
    uint8 i; /* call array index */

    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer == NULL)
    {
        return FALSE;
    }

    /* Find call index from the call id */
    i = tbsFindCall(telephoneBearerServer, callId);

    if(i == FREE_CALL_SLOT)
    {
        return FALSE;
    }

    *joinAllowed = telephoneBearerServer->data.currentCallsList[i].allowJoin;

    return TRUE;
}


/* Validate the opcode for the given call id */
/* returns new state following opcode or TBS_CALL_STATE_INVALID if invalid transition */
/* assumed that the callIndex is valid */
GattTbsCallStates validateCcpOpcodeCallId(const ServiceHandle srvcHndl, uint8 callIndex, uint16 oc)
{
    GTBS_T *telephoneBearerServer = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephoneBearerServer == NULL || callIndex >= TBS_CURRENT_CALLS_LIST_SIZE)
    {
        return TBS_CALL_STATE_INVALID;
    }

    /* Opcode Terminate is valid for all call states but the call will be destroyed not
     * transition to another state */
    if(oc == TBS_OPCODE_TERMINATE)
    {
        return TBS_CALL_STATE_TERMINATING;
    }

    /* Check if the opcode is Join but the call's bearer disallows it */
    if(oc == TBS_OPCODE_JOIN &&
       !telephoneBearerServer->data.currentCallsList[callIndex].allowJoin)
    {
        return TBS_CALL_STATE_INVALID;
    }

    /* Validate the opcode and state change of the call */
    switch(telephoneBearerServer->data.currentCallsList[callIndex].callState)
    {
        case TBS_CALL_STATE_INCOMING:
            {
                switch (oc)
                {
                    case TBS_OPCODE_LOCAL_HOLD:
                        return TBS_CALL_STATE_LOCALLY_HELD;
                    case TBS_OPCODE_ACCEPT:
                        return TBS_CALL_STATE_ACTIVE;
                }
            }
            break;
        case TBS_CALL_STATE_DIALING:
            {
                switch (oc)
                {
                    case TBS_OPCODE_JOIN: /* joining this state is allowed but makes no change */
                        return TBS_CALL_STATE_DIALING;
                    /* Note remote answer is not an opcode driven state change */
                }
            }
            break;
        case TBS_CALL_STATE_ALERTING:
            {
                switch (oc)
                {
                    case TBS_OPCODE_LOCAL_HOLD:
                        return TBS_CALL_STATE_LOCALLY_HELD;
                    case TBS_OPCODE_ACCEPT:
                        return TBS_CALL_STATE_ACTIVE;
                    case TBS_OPCODE_JOIN: /* joining this state is allowed but makes no change */
                        return TBS_CALL_STATE_ALERTING;
                }
            }
            break;
        case TBS_CALL_STATE_ACTIVE:
            {
                switch (oc)
                {
                    case TBS_OPCODE_LOCAL_HOLD:
                        return TBS_CALL_STATE_LOCALLY_HELD;
                    case TBS_OPCODE_JOIN: /* joining this state is allowed but makes no change */
                        return TBS_CALL_STATE_ACTIVE;

                }
            }
            break;
        case TBS_CALL_STATE_LOCALLY_HELD:
            {
                switch (oc)
                {
                    case TBS_OPCODE_LOCAL_RETRIEVE:
                        return TBS_CALL_STATE_ACTIVE;
                    case TBS_OPCODE_JOIN:
                        return TBS_CALL_STATE_ACTIVE;
                }
            }
            break;
        case TBS_CALL_STATE_REMOTELY_HELD:
            {
                switch (oc)
                {
                    case TBS_OPCODE_LOCAL_HOLD:
                        return TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD;
                }
            }
            break;
        case TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD:
            {
                switch (oc)
                {
                    case TBS_OPCODE_LOCAL_RETRIEVE:
                        return TBS_CALL_STATE_REMOTELY_HELD;
                    case TBS_OPCODE_JOIN:
                        return TBS_CALL_STATE_REMOTELY_HELD;
                }
            }
            break;
        case TBS_CALL_STATE_INVALID:
            {
                switch (oc)
                {
                    case TBS_OPCODE_ORIGINATE:
                        return TBS_CALL_STATE_DIALING;
                }
            }
            break;
        default:
            /* Unknown state */
            GATT_TBS_SERVER_PANIC("GTBS: Unknown state!\n");
    }

    return TBS_CALL_STATE_INVALID;
}

void gattTbsServerInit(void** gash)
{
    *gash = &tbsServiceHandle;
    GATT_TBS_SERVER_INFO("GTBS: gattTbsServerInit\n\n");
}

#ifdef ENABLE_SHUTDOWN
void gattTbsServerDeinit(void** gash)
{
    ServiceHandle srvcHndl = *((ServiceHandle*)*gash);

    if (srvcHndl)
    {
        if (ServiceHandleFreeInstanceData(srvcHndl))
        {
            GATT_TBS_SERVER_INFO("Tbs: gattTbsServerDeinit\n\n");
        }
        else
        {
            GATT_TBS_SERVER_PANIC("GTBS ERROR:Unable to free the TBS server instance\n");
        }
    }
    else 
    {
        GATT_TBS_SERVER_DEBUG("Tbs: GTBS INFO: Service Handle Invalid\n\n");
    }
}
#endif
