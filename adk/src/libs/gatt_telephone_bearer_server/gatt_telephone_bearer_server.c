/*******************************************************************************
Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 
*******************************************************************************/

#include "gatt_telephone_bearer_server_private.h"
#include "gatt_telephone_bearer_server_msg_handler.h"
#include "gatt_telephone_bearer_server_access.h"
#include "gatt_telephone_bearer_server_common.h"



void initClientData(TbsClientData* pCd);


/* initClientData

    Inits(zeros) a tbs_client_data struct
*/
void initClientData(TbsClientData* pCd)
{
    PanicNull(pCd);

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
}


/******************************************************************************/
ServiceHandle GattTelephoneBearerServerInit( Task    appTask,
                                        uint16  startHandle,
                                        uint16  endHandle,
                                        gattTbsInitData* initData)
{
    gatt_manager_server_registration_params_t reg_params;
    uint16 clientIndex = 0;
    GTBS_T  *telephone_bearer_server = NULL;
    ServiceHandle new_service_handle = 0;

    if(appTask == NULL)
    {
        GATT_TELEPHONE_BEARER_SERVER_DEBUG_PANIC(("GattTelephoneBearerServerInit fail invalid params"));
        return 0;
    }

    new_service_handle = ServiceHandleNewInstance((void **) &telephone_bearer_server, sizeof(GTBS_T));

    if (new_service_handle !=0)
    {
        uint16 callId = 0;

        /* Reset all the service library memory */
        memset(telephone_bearer_server, 0, sizeof(GTBS_T));

        telephone_bearer_server->srvc_handle =  new_service_handle;;

        /* Set up library handler for external messages */
        telephone_bearer_server->lib_task.handler = tbsServerMsgHandler;

        /* Store the Task function parameter.
           All library messages need to be sent here */
        telephone_bearer_server->appTask = appTask;

        /* Initialise starting call index id */
        telephone_bearer_server->data.nextCallId = 1;

        /* Initialisation of the TBS Characteristics */
        telephone_bearer_server->data.providerName = NULL;
        telephone_bearer_server->data.providerNameLen = 0;
        telephone_bearer_server->data.uci =  NULL;
        telephone_bearer_server->data.uciLen = 0;
        telephone_bearer_server->data.uriPrefixesList = NULL;
        telephone_bearer_server->data.uriPrefixesLen = 0;
        telephone_bearer_server->data.signalStrength = TBS_SIGNAL_STRENGTH_UNAVAILABLE;
        memset(&telephone_bearer_server->data.incomingTargetBearerUri,0,sizeof(telephone_bearer_server->data.incomingTargetBearerUri));
        memset(&telephone_bearer_server->data.callFriendlyName,0,sizeof(telephone_bearer_server->data.callFriendlyName));
        memset(&telephone_bearer_server->data.incomingCall,0,sizeof(telephone_bearer_server->data.incomingCall));

        /* init the call ids to 0xFF so we know they're unused */
        memset(&telephone_bearer_server->data.currentCallsList,0,sizeof(telephone_bearer_server->data.currentCallsList));
        while(callId < TBS_CURRENT_CALLS_LIST_SIZE )
        {
            telephone_bearer_server->data.currentCallsList[callId].callId = FREE_CALL_SLOT;
            telephone_bearer_server->data.currentCallsList[callId].callState = TBS_CALL_STATE_INVALID;
            callId++;
        }

        /* Update with supplied data if provided */
        if(initData != NULL)
        {
            uint16 len = 0;

            len = initData->providerNameLen;
            if(len != 0 && initData->providerName != NULL)
            {
                telephone_bearer_server->data.providerNameLen = len;
                telephone_bearer_server->data.providerName = PanicUnlessMalloc(len);
                memmove(telephone_bearer_server->data.providerName,
                        initData->providerName,
                        len);
            }

            len = initData->uciLen;
            if(len != 0 && initData->uci != NULL)
            {
                telephone_bearer_server->data.uciLen = len;
                telephone_bearer_server->data.uci = PanicUnlessMalloc(len);
                memmove(telephone_bearer_server->data.uci, initData->uci, len);
            }

            len = initData->uriPrefixesLen;
            if(len != 0 && initData->uriPrefixesList != NULL)
            {
                telephone_bearer_server->data.uriPrefixesLen = len;
                telephone_bearer_server->data.uriPrefixesList = PanicUnlessMalloc(len);
                memmove(telephone_bearer_server->data.uriPrefixesList, initData->uriPrefixesList, len);
            }

            if(initData->signalStrength <= TBS_SIGNAL_STRENGTH_MAX)
            {
                telephone_bearer_server->data.signalStrength = initData->signalStrength;
            }

            telephone_bearer_server->data.technology = initData->technology;
            telephone_bearer_server->data.signalStrengthReportingInterval = initData->signalStrengthReportingInterval;
            telephone_bearer_server->data.contentControlId = initData->contentControlId ;
            telephone_bearer_server->data.statusFlags = initData->statusFlags;
            telephone_bearer_server->data.callControlPointOpcodes = initData->callControlPointOpcodes;

            memmove(telephone_bearer_server->data.currentCallsList,
                    initData->currentCallsList,
                    sizeof(initData->currentCallsList));

            /* List may contain pointers to URI strings so they need to be copied separately */
            callId = 0;
            while(callId < TBS_CURRENT_CALLS_LIST_SIZE )
            {
                if(telephone_bearer_server->data.currentCallsList[callId].callId != FREE_CALL_SLOT)
                {
                    uint16 uriLen = telephone_bearer_server->data.currentCallsList[callId].callUriLen;
                    if(uriLen != 0)
                    {
                        initData->currentCallsList[callId].callUri = PanicUnlessMalloc(uriLen);
                        memmove(telephone_bearer_server->data.currentCallsList[callId].callUri,
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
                telephone_bearer_server->data.incomingTargetBearerUri.callId = initData->incomingTargetBearerUri.callId;
                telephone_bearer_server->data.incomingTargetBearerUri.uriLen = len;
                telephone_bearer_server->data.incomingTargetBearerUri.uri = PanicUnlessMalloc(len);
                memmove(telephone_bearer_server->data.incomingTargetBearerUri.uri,
                        initData->incomingTargetBearerUri.uri,
                        len);
            }

            len = telephone_bearer_server->data.callFriendlyName.nameLen;
            if(len != 0 && telephone_bearer_server->data.callFriendlyName.friendlyName != NULL)
            {
                telephone_bearer_server->data.callFriendlyName.callId = initData->callFriendlyName.callId;
                telephone_bearer_server->data.callFriendlyName.nameLen = len;
                telephone_bearer_server->data.callFriendlyName.friendlyName = PanicUnlessMalloc(len);
                memmove(telephone_bearer_server->data.callFriendlyName.friendlyName,
                        initData->callFriendlyName.friendlyName,
                        len);
            }

            telephone_bearer_server->data.incomingCall.callId = initData->incomingCall.callId;
        }
        else
        {   /* Set some defaults that won't be set by provided init values */
            telephone_bearer_server->data.technology = 0;
            telephone_bearer_server->data.signalStrengthReportingInterval = 0;
            telephone_bearer_server->data.contentControlId = 0;
            telephone_bearer_server->data.statusFlags = 0;
            telephone_bearer_server->data.callControlPointOpcodes = 0;
        }



        /* Init connected clients */
        for (clientIndex=0; clientIndex<TBS_MAX_CONNECTIONS; clientIndex++)
        {
            initClientData(&telephone_bearer_server->data.connected_clients[clientIndex]);
        }

        /* Setup data required for TBS to be registered with the GATT Manager */
        reg_params.start_handle = startHandle;
        reg_params.end_handle = endHandle;
        reg_params.task = &telephone_bearer_server->lib_task;

        /* Register with the GATT Manager and verify the result */
        if (GattManagerRegisterServer(&reg_params) != gatt_manager_status_success)
        {
            ServiceHandleFreeInstanceData(new_service_handle);
            new_service_handle = 0;
        }
    }

    return new_service_handle;
}


/******************************************************************************/
gatt_status_t GattTelephoneBearerServerAddConfig(
                  ServiceHandle srvcHndl,
                  connection_id_t cid,
                  const gattTbsServerConfig *config)
{
    uint8 i;
    GTBS_T *telephone_bearer_server = (GTBS_T*) ServiceHandleGetInstanceData(srvcHndl);

    for(i=0; i<TBS_MAX_CONNECTIONS; i++)
    {
        if(telephone_bearer_server->data.connected_clients[i].cid == 0)
        {
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
                    config->clientCfgs.terminationReasonClientCfg == CLIENT_CONFIG_INDICATE)
                {                    
                    GATT_TELEPHONE_BEARER_SERVER_DEBUG_INFO(("Invalid Client Configuration Characteristic!\n"));
                    return gatt_status_value_not_allowed;
                }

                /* Save new ccc for the client */
                memmove(&telephone_bearer_server->data.connected_clients[i].clientCfg,
                        &config->clientCfgs,
                        sizeof(TbsClientCfgData));

                /* Notify any characteristics that are set */
                if (config->clientCfgs.bearerTechnologyClientCfg == CLIENT_CONFIG_NOTIFY)
                {
                    tbsServerSendCharacteristicChangedNotification(
                            (Task) &telephone_bearer_server->lib_task,
                            telephone_bearer_server->data.connected_clients[i].cid,
                            HANDLE_BEARER_TECHNOLOGY,
                            BEARER_TECHNOLOGY_SIZE,
                            (uint8*)&telephone_bearer_server->data.technology
                            );
                }

                if (config->clientCfgs.providerNameClientCfg == CLIENT_CONFIG_NOTIFY)
                {
                    tbsServerSendCharacteristicChangedNotification(
                            (Task) &telephone_bearer_server->lib_task,
                            telephone_bearer_server->data.connected_clients[i].cid,
                            HANDLE_BEARER_PROVIDER_NAME,
                            telephone_bearer_server->data.providerNameLen,
                            (uint8*)telephone_bearer_server->data.providerName
                            );
                }

                if (config->clientCfgs.statusFlagsClientCfg == CLIENT_CONFIG_NOTIFY)
                {
                    tbsServerSendCharacteristicChangedNotification(
                            (Task) &telephone_bearer_server->lib_task,
                            telephone_bearer_server->data.connected_clients[i].cid,
                            HANDLE_STATUS_FLAGS,
                            sizeof(telephone_bearer_server->data.statusFlags),
                            (uint8*)&telephone_bearer_server->data.statusFlags
                            );
                }

                /* Notify the rest of the characteristcs */
                gattTelephoneBearerServerNotifyCallFriendlyName(telephone_bearer_server, &i);
                gattTelephoneBearerServerNotifyCallState(telephone_bearer_server, &i);
                gattTelephoneBearerServerNotifyCurrentCalls(telephone_bearer_server, &i);
                gattTelephoneBearerServerNotifyIncomingCall(telephone_bearer_server, &i);
                gattTelephoneBearerServerNotifyIncomingCallTargetUri(telephone_bearer_server, &i);
                gattTelephoneBearerServerNotifySignalStrength(telephone_bearer_server, &i);

                /* Note do not notify the following two characteristics as they're not
                 * relevant unless there has been an action */
                /* config->clientCfgs.callControlPointClientCfg
                   config->clientCfgs.terminationReasonClientCfg */
            }

            telephone_bearer_server->data.connected_clients[i].cid = cid;

            /* Update of the specified client is complete */
            return gatt_status_success;
        }
    }

    return gatt_status_insufficient_resources;
}

/******************************************************************************/
gattTbsServerConfig * GattTelephoneBearerServerRemoveConfig(
                              ServiceHandle srvcHndl,
                              connection_id_t  cid)
{
    uint8 i;
    GTBS_T *telephone_bearer_server = (GTBS_T *) ServiceHandleGetInstanceData(srvcHndl);
    gattTbsServerConfig *config = NULL;

    for(i=0; i<TBS_MAX_CONNECTIONS; i++)
    {
        /* Check the saved CID to find the peeer device */
        if (telephone_bearer_server->data.connected_clients[i].cid == cid)
        {
            /* Found peer device:
             * - save last client configurations
             * - remove the peer device
             * - free the server instance
             * - return last client configuration
             */

            config = PanicUnlessMalloc(sizeof(gattTbsServerConfig));
            memmove(&config->clientCfgs, &(telephone_bearer_server->data.connected_clients[i].clientCfg), sizeof(TbsClientCfgData));

            if ((i == (TBS_MAX_CONNECTIONS-1)) || (i == 0 && telephone_bearer_server->data.connected_clients[i+1].cid == 0))
            {
                /* The peer device is the only or the last element of the array */
                memset(&(telephone_bearer_server->data.connected_clients[i]), 0, sizeof(TbsClientData));
            }
            else
            {
                /* The peer device is in the middle of the array */
                uint8 j;

                for (j=i; j<(TBS_MAX_CONNECTIONS - 1) && telephone_bearer_server->data.connected_clients[j+1].cid != 0; j++)
                {
                    /* Shift all the elements of the array of one position behind */
                    memmove(&(telephone_bearer_server->data.connected_clients[j]),
                           &(telephone_bearer_server->data.connected_clients[j+1]),
                           sizeof(TbsClientData));
                }

                /* Remove the last element of the array, already shifted behind */
                memset(&(telephone_bearer_server->data.connected_clients[j]), 0, sizeof(TbsClientData));
            }
        }
    }

    return config;
}


bool GattTelephoneBearerServerSetProviderName(const ServiceHandle srvcHndl, char* providerName, uint16 len)
{
    uint16 i;
    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server == NULL)
    {
        return FALSE;
    }

    if(len == 0)
    {
        if(telephone_bearer_server->data.providerName)
        {
            free(telephone_bearer_server->data.providerName);
        }
        telephone_bearer_server->data.providerName = NULL;
        telephone_bearer_server->data.providerNameLen = 0;
    }
    else
    {
        char* oldName = telephone_bearer_server->data.providerName;
        telephone_bearer_server->data.providerNameLen = (len>PROVIDER_NAME_SIZE? PROVIDER_NAME_SIZE : len );
        telephone_bearer_server->data.providerName =
                realloc(telephone_bearer_server->data.providerName, telephone_bearer_server->data.providerNameLen);
        /* check if realloc failed */
        if (telephone_bearer_server->data.providerName == NULL)
        {
            if(oldName)
            {
                free(oldName);
            }
            telephone_bearer_server->data.providerNameLen = 0;
        }
        else
        {
            memcpy(telephone_bearer_server->data.providerName, providerName,
                   telephone_bearer_server->data.providerNameLen);
        }

    }

    for (i=0; i<TBS_MAX_CONNECTIONS; i++)
    {
        if (telephone_bearer_server->data.connected_clients[i].cid != 0)
        {
            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (telephone_bearer_server->data.connected_clients[i].clientCfg.providerNameClientCfg == CLIENT_CONFIG_NOTIFY)
            {
                tbsServerSendCharacteristicChangedNotification(
                        (Task) &telephone_bearer_server->lib_task,
                        telephone_bearer_server->data.connected_clients[i].cid,
                        HANDLE_BEARER_PROVIDER_NAME,
                        telephone_bearer_server->data.providerNameLen,
                        (uint8*)telephone_bearer_server->data.providerName
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
    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server == NULL ||
       telephone_bearer_server->data.providerNameLen == 0)
    {
        return 0;
    }

    {
        char* name_string = PanicUnlessMalloc(sizeof(char) * telephone_bearer_server->data.providerNameLen);
        memmove(name_string, telephone_bearer_server->data.providerName, telephone_bearer_server->data.providerNameLen);

        *providerName = name_string;      

        return telephone_bearer_server->data.providerNameLen;
    }
}


bool GattTelephoneBearerServerSetUci(const ServiceHandle srvcHndl, char* uci, uint16 len)
{
    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server == NULL)
    {
        return FALSE;
    }

    if(len == 0)
    {
        if(telephone_bearer_server->data.uci)
        {
            free(telephone_bearer_server->data.uci);
        }
        telephone_bearer_server->data.uci = NULL;
        telephone_bearer_server->data.uciLen = 0;
    }
    else
    {
        char* oldUci = telephone_bearer_server->data.uci;
        telephone_bearer_server->data.uciLen = (len>BEARER_UCI_SIZE? BEARER_UCI_SIZE : len );
        telephone_bearer_server->data.uci =
                realloc(telephone_bearer_server->data.uci, telephone_bearer_server->data.uciLen);
        /* check if realloc failed */
        if (telephone_bearer_server->data.uci == NULL)
        {
            if(oldUci)
            {
                free(oldUci);
            }
            telephone_bearer_server->data.uciLen = 0;
            return FALSE;
        }
        else
        {
            memcpy(telephone_bearer_server->data.uci, uci,
                   telephone_bearer_server->data.uciLen);
        }

    }

    return TRUE;
}



uint16 GattTelephoneBearerServerGetUci(const ServiceHandle srvcHndl, char** uci)
{
    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server == NULL)
    {
        return 0;
    }

    {
        char* uci_string = PanicUnlessMalloc(sizeof(char) * telephone_bearer_server->data.uciLen);

        memcpy(uci_string, telephone_bearer_server->data.uci, telephone_bearer_server->data.uciLen);

        *uci = uci_string;

        return telephone_bearer_server->data.uciLen;
    }
}

bool GattTelephoneBearerServerSetTechnology(const ServiceHandle srvcHndl, GattTbsTechnology technology)
{
    uint16 i;
    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server == NULL)
    {
        return FALSE;
    }

    telephone_bearer_server->data.technology = technology;

    for (i=0; i<TBS_MAX_CONNECTIONS; i++)
    {
        if (telephone_bearer_server->data.connected_clients[i].cid != 0)
        {
            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (telephone_bearer_server->data.connected_clients[i].clientCfg.bearerTechnologyClientCfg == CLIENT_CONFIG_NOTIFY)
            {
                tbsServerSendCharacteristicChangedNotification(
                        (Task) &telephone_bearer_server->lib_task,
                        telephone_bearer_server->data.connected_clients[i].cid,
                        HANDLE_BEARER_TECHNOLOGY,
                        BEARER_TECHNOLOGY_SIZE,
                        (uint8*)&telephone_bearer_server->data.technology
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
    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server == NULL)
    {
        return 0;
    }

    return telephone_bearer_server->data.technology;
}


bool GattTelephoneBearerServerSetUriPrefixList(const ServiceHandle srvcHndl, char* prefixList, uint16 length)
{  
    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server == NULL)
    {
        return FALSE;
    }

    if(prefixList)
    {
        char* newPrefixes = realloc(telephone_bearer_server->data.uriPrefixesList, length);
        if(!newPrefixes)
        {
            /* Tidy up if realloc failed */
            if(telephone_bearer_server->data.uriPrefixesList)
            {
                free(telephone_bearer_server->data.uriPrefixesList);
                telephone_bearer_server->data.uriPrefixesList = NULL;
            }
            return FALSE;
        }

        memmove(newPrefixes, prefixList, length);
        telephone_bearer_server->data.uriPrefixesList = newPrefixes;
        telephone_bearer_server->data.uriPrefixesLen  = length;
    }

    return TRUE;
}

uint16 GattTelephoneBearerServerGetUriPrefix(const ServiceHandle srvcHndl, char** uriList)
{
    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server == NULL)
    {
        return 0;
    }

    if(!telephone_bearer_server->data.uriPrefixesList)
    {
        return 0;
    }

    char* uri_list_string = PanicUnlessMalloc(sizeof(char) * telephone_bearer_server->data.uriPrefixesLen);

    memcpy(uri_list_string, telephone_bearer_server->data.uriPrefixesList, telephone_bearer_server->data.uriPrefixesLen);

    *uriList = uri_list_string;

    return telephone_bearer_server->data.uriPrefixesLen;
}

bool GattTelephoneBearerServerFindUriPrefix(const ServiceHandle srvcHndl, char* prefix)
{
    bool r;
    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server == NULL)
    {
        return FALSE;
    }

    if(!telephone_bearer_server->data.uriPrefixesList)
    {
        return FALSE;
    }

    r = strstr(telephone_bearer_server->data.uriPrefixesList, prefix) != NULL;
    return(r);
}

bool GattTelephoneBearerServerSetSignalStrength(const ServiceHandle srvcHndl, uint8 signalStrength)
{
    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server == NULL)
    {
        return FALSE;
    }

    /* validate signal strength range */
    if (signalStrength > TBS_SIGNAL_STRENGTH_MAX && signalStrength != TBS_SIGNAL_STRENGTH_UNAVAILABLE)
    {
        GATT_TELEPHONE_BEARER_SERVER_DEBUG_INFO(("Set signal strength - out of range\n"))
        return FALSE;
    }

    telephone_bearer_server->data.signalStrength = signalStrength;

    if(telephone_bearer_server->data.signal_strength_timer_flag)
    {
        return FALSE;
    }

    /* Notify Clients */
    gattTelephoneBearerServerNotifySignalStrength(telephone_bearer_server, NULL);

    return TRUE;
}


uint8 GattTelephoneBearerServerGetSignalStrength(const ServiceHandle srvcHndl)
{
    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server == NULL)
    {
        return 0;
    }

    return telephone_bearer_server->data.signalStrength;
}

bool GattTelephoneBearerServerSetSignalStrengthInterval(const ServiceHandle srvcHndl, uint8 interval)
{
    uint16 msgWaiting;
    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server == NULL)
    {
        return FALSE;
    }

    if(telephone_bearer_server->data.signalStrengthReportingInterval == interval)
    {
        /* No changes, don't do anything */
        return FALSE;
    }

    /* Store new interval */
    telephone_bearer_server->data.signalStrengthReportingInterval = interval;

    /* Cancel signal strength timer messages */
    msgWaiting = MessageCancelAll((Task) &telephone_bearer_server->lib_task,
                             GATT_TBS_INTERNAL_SIGNAL_STRENGTH_TIMER);

    /* If we are waiting for a signal strength interval timer to fire, reset it with the new
     * timer */
    if(msgWaiting && telephone_bearer_server->data.signalStrengthReportingInterval != 0)
    {
        MAKE_TBS_MESSAGE(GATT_TBS_INTERNAL_SIGNAL_STRENGTH_TIMER);

        telephone_bearer_server->data.signal_strength_timer_flag = TRUE;

        MessageSendLater((Task) &telephone_bearer_server->lib_task,
                            GATT_TBS_INTERNAL_SIGNAL_STRENGTH_TIMER,
                            message,
                            D_SEC(telephone_bearer_server->data.signalStrengthReportingInterval));

    }
    else
    {
        /* Not currently waiting for a signal strength interval timer to fire or
         * signal strength reporting is disabled (ie == 0)*/
        telephone_bearer_server->data.signal_strength_timer_flag = FALSE;
    }

    return TRUE;
}

uint8 GattTelephoneBearerServerGetSignalStrengthInterval(const ServiceHandle srvcHndl)
{
    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server == NULL)
    {
        return 0;
    }

    return telephone_bearer_server->data.signalStrengthReportingInterval;
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
    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server == NULL)
    {
        return TBS_CCP_RESULT_LACK_OF_RESOURCES;
    }

    *newCallId = tbsAddCall(telephone_bearer_server, callState, callFlags, callUriSize, callUri);
    if(*newCallId == FREE_CALL_SLOT)
        return TBS_CCP_RESULT_LACK_OF_RESOURCES;
    
    /* notify call state */
    gattTelephoneBearerServerNotifyCallState(telephone_bearer_server, NULL);

    switch(callState)
    {
    case TBS_CALL_STATE_INCOMING: /* Incoming call */
        {
            uint8 incCallId = *newCallId;
            telephone_bearer_server->data.incomingCall.callId = incCallId;

            gattTelephoneBearerServerNotifyIncomingCall(telephone_bearer_server, NULL);

            GattTelephoneBearerServerSetIncomingCallTargetUri(srvcHndl,
                                                              *newCallId,
                                                              targetUri,
                                                              targetUriSize);
        }
        break;

    case TBS_CALL_STATE_DIALLING: /* Outgoing calls */
    case TBS_CALL_STATE_ALERTING:
        break;

    default:
        break;
    }

    /* Notify current calls for all call states */
    gattTelephoneBearerServerNotifyCurrentCalls(telephone_bearer_server, NULL);
    
    return TBS_CCP_RESULT_SUCCESS;
}

tbsCurrentCallListChracteristic* GattTelephoneBearerServerGetCurrentCalls(const ServiceHandle srvcHndl)
{
    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server == NULL)
    {
        return NULL;
    }

    return telephone_bearer_server->data.currentCallsList;
}


bool GattTelephoneBearerServerSetContentControlId(const ServiceHandle srvcHndl, uint8 ccid)
{
    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server == NULL)
    {
        return FALSE;
    }

    telephone_bearer_server->data.contentControlId = ccid;

    return TRUE;
}

uint8 GattTelephoneBearerServerGetContentControlId(const ServiceHandle srvcHndl)
{
    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server == NULL)
    {
        return 0;
    }

    return telephone_bearer_server->data.contentControlId;
}

bool GattTelephoneBearerServerSetIncomingCallTargetUri(const ServiceHandle srvcHndl, uint8 callId, char* incomingUri, uint16 len)
{
    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server == NULL)
    {
        return FALSE;
    }

    telephone_bearer_server->data.incomingTargetBearerUri.callId = callId;

    if(len == 0)
    {
        if(telephone_bearer_server->data.incomingTargetBearerUri.uri)
        {
            free(telephone_bearer_server->data.incomingTargetBearerUri.uri);
        }
        telephone_bearer_server->data.incomingTargetBearerUri.uri = NULL;
        telephone_bearer_server->data.incomingTargetBearerUri.uriLen = 0;
        telephone_bearer_server->data.incomingTargetBearerUri.callId = 0;

        return FALSE;
    }
    else
    {
        char* oldUri = telephone_bearer_server->data.incomingTargetBearerUri.uri;
        telephone_bearer_server->data.incomingTargetBearerUri.uriLen = (len>MAX_CALL_URI_SIZE? MAX_CALL_URI_SIZE : len );
        telephone_bearer_server->data.incomingTargetBearerUri.uri =
                realloc(telephone_bearer_server->data.incomingTargetBearerUri.uri,
                        telephone_bearer_server->data.incomingTargetBearerUri.uriLen);
        /* check if realloc failed */
        if (telephone_bearer_server->data.incomingTargetBearerUri.uri == NULL)
        {
            if(oldUri)
            {
                free(oldUri);
            }
            telephone_bearer_server->data.incomingTargetBearerUri.uriLen = 0;
            telephone_bearer_server->data.incomingTargetBearerUri.callId = 0;

            return FALSE;
        }
        else
        {
            memmove(telephone_bearer_server->data.incomingTargetBearerUri.uri,
                    incomingUri,
                    telephone_bearer_server->data.incomingTargetBearerUri.uriLen);
        }

    }

    /* Notify clients */
    gattTelephoneBearerServerNotifyIncomingCallTargetUri(telephone_bearer_server, NULL);

    return TRUE;
}


tbsIncomingCallTargetUriChracteristic* GattTelephoneBearerServerGetIncomingCallTargetUri(const ServiceHandle srvcHndl)
{
    tbsIncomingCallTargetUriChracteristic* retUri = NULL;

    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server != NULL)
    {
        retUri = PanicUnlessNew(tbsIncomingCallTargetUriChracteristic);

        retUri->callId = telephone_bearer_server->data.incomingTargetBearerUri.callId;
        retUri->uriLen = telephone_bearer_server->data.incomingTargetBearerUri.uriLen;

        if(retUri->uriLen > 0)
        {
            retUri->uri = PanicUnlessMalloc(retUri->uriLen);
            memmove(retUri->uri,
                    telephone_bearer_server->data.incomingTargetBearerUri.uri,
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

    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server == NULL)
    {
        return FALSE;
    }

    /* Validate flags  */
    if(flags & ~TBS_STATUS_FLAGS_ALL)
    {
        return FALSE;
    }

    /* Assign new feature flags */
    telephone_bearer_server->data.statusFlags = flags;

    /* Notify connected clients of the change */
    for (i=0; i<TBS_MAX_CONNECTIONS; i++)
    {
        if (telephone_bearer_server->data.connected_clients[i].cid != 0)
        {
            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (telephone_bearer_server->data.connected_clients[i].clientCfg.statusFlagsClientCfg == CLIENT_CONFIG_NOTIFY)
            {
                tbsServerSendCharacteristicChangedNotification(
                        (Task) &telephone_bearer_server->lib_task,
                        telephone_bearer_server->data.connected_clients[i].cid,
                        HANDLE_STATUS_FLAGS,
                        sizeof(telephone_bearer_server->data.statusFlags),
                        (uint8*)&telephone_bearer_server->data.statusFlags
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
    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server == NULL)
    {
        return 0;
    }

    return telephone_bearer_server->data.statusFlags;
}

bool GattTelephoneBearerServerSetCallState(const ServiceHandle srvcHndl,
                                            uint8 callId,
                                            GattTbsCallStates callState,
                                            const bool notify)
{
    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server == NULL)
    {
        return FALSE;
    }

    if(!tbsSetCallState(telephone_bearer_server, callId, callState))
        return FALSE;

    /* Notify Clients */
    if(notify)
    {
        gattTelephoneBearerServerNotifyCallChange(telephone_bearer_server);
    }

    return TRUE;
    
}

bool GattTelephoneBearerServerNotifyCallState(const ServiceHandle srvcHndl)
{
    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server == NULL)
    {
        return FALSE;
    }

    gattTelephoneBearerServerNotifyCallChange(telephone_bearer_server);

    return TRUE;

}

bool GattTelephoneBearerServerCallControlPointAcceptOpcode(const ServiceHandle srvcHndl,
                                            uint8 callId,
                                            GattTbsCallControlPointOpcode opcode,
                                            GattTbsCallStates callState)
{
    uint8 callIndex = 0;
    GattTbsCallStates currentCallState;

    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server == NULL)
    {
        return FALSE;
    }

    /* check if the call exists */
    callIndex = tbsFindCall(telephone_bearer_server, callId);
    if(callIndex == FREE_CALL_SLOT)
    {
        return FALSE;
    }

    currentCallState = telephone_bearer_server->data.currentCallsList[callIndex].callState;

    /* Check and update the call state of other calls */
    switch(opcode)
    {
        case TBS_OPCODE_ACCEPT:
            tbsSetAllCallState(telephone_bearer_server, TBS_CALL_STATE_ACTIVE, TBS_CALL_STATE_LOCALLY_HELD);
            tbsSetAllCallState(telephone_bearer_server, TBS_CALL_STATE_REMOTELY_HELD, TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD);
        break;

        case TBS_OPCODE_LOCAL_RETREIVE:
            if(currentCallState == TBS_CALL_STATE_LOCALLY_HELD)
            {
                tbsSetAllCallState(telephone_bearer_server, TBS_CALL_STATE_ACTIVE, TBS_CALL_STATE_LOCALLY_HELD);
            }
            else if (currentCallState == TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD ||
                     currentCallState == TBS_CALL_STATE_REMOTELY_HELD)
            {
                tbsSetAllCallState(telephone_bearer_server, TBS_CALL_STATE_ACTIVE, TBS_CALL_STATE_LOCALLY_HELD);
                tbsSetAllCallState(telephone_bearer_server, TBS_CALL_STATE_REMOTELY_HELD, TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD);
            }
        break;

        case TBS_OPCODE_ORIGINATE:
            tbsSetAllCallState(telephone_bearer_server, TBS_CALL_STATE_ACTIVE, TBS_CALL_STATE_LOCALLY_HELD);
        break;

        case TBS_OPCODE_JOIN:

        case TBS_OPCODE_LOCAL_HOLD:
        case TBS_OPCODE_TERMINATE:
        default:
            break;
    }

    tbsSetCallState(telephone_bearer_server, callId, callState);

    /* Notify Clients */
    gattTelephoneBearerServerNotifyCallChange(telephone_bearer_server);

    return TRUE;

}


bool GattTelephoneBearerServerCallControlPointResponse(const ServiceHandle srvcHndl, tbsCallControlPointNotification *ccpn)
{
    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server == NULL)
    {
        return FALSE;
    }

    /* if the operation was not a success the call id should be 0 */
    if(ccpn->resultCode != TBS_CCP_RESULT_SUCCESS)
    {
        ccpn->callId = 0;
    }

    /* Notify connected clients of the change */
    gattTelephoneBearerServerNotifyCallControlPoint(telephone_bearer_server, ccpn, NULL );

    return TRUE;
}

bool GattTelephoneBearerServerSetControlPointOpcodes(const ServiceHandle srvcHndl, uint16 opcodes)
{
    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server == NULL)
    {
        return FALSE;
    }

    telephone_bearer_server->data.callControlPointOpcodes = opcodes;

    return TRUE;
}

uint16 GattTelephoneBearerServerGetCallControlPointOpcodes(const ServiceHandle srvcHndl)
{
    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server == NULL)
    {
        return 0;
    }

    return telephone_bearer_server->data.callControlPointOpcodes;
}

bool GattTelephoneBearerServerTerminateCall(const ServiceHandle srvcHndl, uint8 callId, GattTbsCallTerminationReason reason)
{
    uint16 i;
    bool retVal = TRUE;
    tbsTerminationReasonChracteristic tr;

    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server == NULL)
    {
        return FALSE;
    }
    
    /* Delete the call from the call list */
    if(!tbsDeleteCall(telephone_bearer_server, callId))
        retVal = FALSE; /* call does not exist */
    
    tr.callId = callId;
    tr.reason = reason;

    /* Notify connected clients of the change */
    for (i=0; i<TBS_MAX_CONNECTIONS; i++)
    {
        if (telephone_bearer_server->data.connected_clients[i].cid != 0)
        {
            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (telephone_bearer_server->data.connected_clients[i].clientCfg.terminationReasonClientCfg == CLIENT_CONFIG_NOTIFY)
            {              
                tbsServerSendCharacteristicChangedNotification(
                        (Task) &telephone_bearer_server->lib_task,
                        telephone_bearer_server->data.connected_clients[i].cid,
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
    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server == NULL)
    {
        return FALSE;
    }

    if(len == 0)
    {
        if(telephone_bearer_server->data.callFriendlyName.friendlyName)
        {
            free(telephone_bearer_server->data.callFriendlyName.friendlyName);
        }
        telephone_bearer_server->data.callFriendlyName.friendlyName = NULL;
        telephone_bearer_server->data.callFriendlyName.nameLen = 0;
        telephone_bearer_server->data.callFriendlyName.callId = 0;

        return FALSE;
    }
    else
    {
        char* oldUri = telephone_bearer_server->data.callFriendlyName.friendlyName;
        telephone_bearer_server->data.callFriendlyName.nameLen = (len>MAX_CALL_URI_SIZE? MAX_CALL_URI_SIZE : len );
        telephone_bearer_server->data.callFriendlyName.friendlyName =
                realloc(telephone_bearer_server->data.callFriendlyName.friendlyName,
                        telephone_bearer_server->data.callFriendlyName.nameLen);

        /* check if realloc failed */
        if (telephone_bearer_server->data.callFriendlyName.friendlyName == NULL)
        {
            if(oldUri)
            {
                free(oldUri);
            }
            telephone_bearer_server->data.callFriendlyName.nameLen = 0;
            telephone_bearer_server->data.callFriendlyName.callId = 0;

            return FALSE;
        }
        else
        {
            memmove(telephone_bearer_server->data.callFriendlyName.friendlyName,
                    name,
                    telephone_bearer_server->data.callFriendlyName.nameLen);
        }

        telephone_bearer_server->data.callFriendlyName.callId = callId;
    }

    /*Notify clients */
    gattTelephoneBearerServerNotifyCallFriendlyName(telephone_bearer_server, NULL);

    return TRUE;
}


tbsCallFriendlyNameChracteristic* GattTelephoneBearerServerGetRemoteFriendlyName(const ServiceHandle srvcHndl)
{
    tbsCallFriendlyNameChracteristic *retName = NULL;
    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server != NULL)
    {              
        retName = PanicUnlessNew(tbsCallFriendlyNameChracteristic);
        retName->callId = telephone_bearer_server->data.callFriendlyName.callId;
        retName->nameLen = telephone_bearer_server->data.callFriendlyName.nameLen;

        if(retName->nameLen > 0)
        {
            retName->friendlyName = PanicUnlessMalloc(retName->nameLen);
            memmove(retName->friendlyName,
                    telephone_bearer_server->data.callFriendlyName.friendlyName,
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
    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server == NULL)
    {
        return FALSE;
    }

    /* Find call id */
    callSlot = tbsFindCall(telephone_bearer_server, callId);

    if((callSlot != FREE_CALL_SLOT) && state)
    {
        *state = (telephone_bearer_server->data.currentCallsList[callSlot].callState);
        return TRUE;
    }

    return FALSE;
}

GattTbsCcpNotificationResultCodes GattTelephoneBearerServerCheckOpcodeSupport(const ServiceHandle srvcHndl, uint8 opcode)
{
    uint16 ccpOptionalFlags = 0;

    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server == NULL)
    {
        return TBS_CCP_RESULT_LACK_OF_RESOURCES;
    }

    ccpOptionalFlags = telephone_bearer_server->data.callControlPointOpcodes;

    /* Check this is a supported opcode */
    if(opcode == TBS_OPCODE_LOCAL_HOLD && !(ccpOptionalFlags & TBS_CCP_OPTIONAL_LOCAL_HOLD))
    {
        return TBS_CCP_RESULT_OPCODE_NOT_SUPPORTED;
    }

    /* Check the join opcode, this has a seperate result code */
    if(opcode == TBS_OPCODE_JOIN && !(ccpOptionalFlags & TBS_CCP_OPTIONAL_JOIN))
    {
        return TBS_CCP_RESULT_OPERATION_NOT_POSSIBLE;
    }

    return TBS_CCP_RESULT_SUCCESS;
}

GattTbsCallStates GattTelephoneBearerServerCheckOpcodeState(const ServiceHandle srvcHndl, uint8 callId, uint8 opcode)
{
    uint8 i; /* call array index */

    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server == NULL)
    {
        return TBS_CALL_STATE_INVALID;
    }

    /* Find call id */
    i = tbsFindCall(telephone_bearer_server, callId);

    if(i == FREE_CALL_SLOT)
    {
        return TBS_CALL_STATE_INVALID;
    }

    return validateCcpOpcodeCallId(srvcHndl, i, opcode);
}

/* Validate the opcode for the given call id */
/* returns new state following opcode or TBS_CALL_STATE_INVALID if invalid transition */
/* assumed that the callIndex is valid */
GattTbsCallStates validateCcpOpcodeCallId(const ServiceHandle srvcHndl, uint8 callIndex, uint16 oc)
{
    GTBS_T *telephone_bearer_server = (GTBS_T*)ServiceHandleGetInstanceData(srvcHndl);
    if(telephone_bearer_server == NULL)
    {
        return TBS_CALL_STATE_INVALID;
    }

    /* Validate the opcode and state change of the call */
    switch(telephone_bearer_server->data.currentCallsList[callIndex].callState)
    {
        case TBS_CALL_STATE_INCOMING:
            {
                switch (oc)
                {
                    case TBS_OPCODE_LOCAL_HOLD:
                        return TBS_CALL_STATE_LOCALLY_HELD;
                    case TBS_OPCODE_TERMINATE:
                        return TBS_CALL_STATE_INVALID;
                    case TBS_OPCODE_ACCEPT:
                        return TBS_CALL_STATE_ACTIVE;
                }
            }
        case TBS_CALL_STATE_DIALLING:
            {
                switch (oc)
                {
                    case TBS_OPCODE_TERMINATE:
                        return TBS_CALL_STATE_INVALID;
                    case TBS_OPCODE_JOIN: /* joining this state is allowed but makes no change */
                     return TBS_CALL_STATE_DIALLING;
                    /* Note remote answer is not an opcode driven state change */
                }
            }
            break;
        case TBS_CALL_STATE_ALERTING:
            {
                switch (oc)
                {
                    case TBS_OPCODE_TERMINATE:
                        return TBS_CALL_STATE_INVALID;
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
                    case TBS_OPCODE_TERMINATE:
                        return TBS_CALL_STATE_INVALID;
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
                    case TBS_OPCODE_TERMINATE:
                        return TBS_CALL_STATE_INVALID;
                    case TBS_OPCODE_LOCAL_RETREIVE:
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
                    case TBS_OPCODE_TERMINATE:
                        return TBS_CALL_STATE_INVALID;
                    case TBS_OPCODE_LOCAL_HOLD:
                        return TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD;
                }
            }
            break;
        case TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD:
            {
                switch (oc)
                {
                    case TBS_OPCODE_TERMINATE:
                        return TBS_CALL_STATE_INVALID;
                    case TBS_OPCODE_LOCAL_RETREIVE:
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
                        return TBS_CALL_STATE_DIALLING;
                }
            }
            break;
        default:
            /* Unknown state */
            GATT_TELEPHONE_BEARER_SERVER_DEBUG_PANIC(("GTBS: Unknown state!\n"));
    }

    return TBS_CALL_STATE_INVALID;
}
