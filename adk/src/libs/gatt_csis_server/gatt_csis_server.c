/* Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd. */
/*  */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "gatt_csis_server_private.h"
#include "gatt_csis_server_msg_handler.h"
#include "gatt_csis_server_notify.h"
#include "gatt_csis_server_lock_management.h"
#include "gatt_csis_server_access.h"


/****************************************************************************/
CsisServerServiceHandleType GattCsisServerInit(Task appTask,
               const GattCsisServerInitParamType *initParams,
               uint16 startHandle,
               uint16 endHandle)
{
     GCSISS_T *csis_server = NULL;

    /*Registration parameters for CSIS library to GATT manager  */
    gatt_manager_server_registration_params_t reg_params;
    ServiceHandle srvc_hndl = 0;

    /* validate the input parameters */
    if (appTask == NULL)
    {
        GATT_CSIS_SERVER_PANIC((
                    "CSIS: Invalid Initialization parameters!"
                    ));
    }

    srvc_hndl = ServiceHandleNewInstance((void **) &csis_server, sizeof(GCSISS_T));

    if (csis_server)
    {
        /* Reset all the service library memory */
        memset(csis_server, 0, sizeof(GCSISS_T));

        /* Set up library handler for external messages */
        csis_server->lib_task.handler = csisServerMsgHandler;

        /* Store the Task function parameter.
         * All library messages need to be sent here */
        csis_server->app_task = appTask;

        /* Save the service handle */
        csis_server->srvc_hndl = srvc_hndl;

        /* Initiliasation of the CSIS Server Charateristics */
        csis_server->data.rank = initParams->rank;
        csis_server->data.cs_size = initParams->csSize;

        /* Ignore GATT_CSIS_SIRK_UPDATE flag at init, we need to
         * copy SIRK first time anyways
         */
        memcpy(csis_server->data.sirk, initParams->sirk, SIZE_SIRK_KEY);

        /* Store the application preference for SIRK operation */
        if ((initParams->flags & 0x0F) == 0x00)
            csis_server->data.sirkOp = GATT_CSIS_SHARE_SIRK_PLAIN_TEXT; /* default if app fails to pass anything */
        else
            csis_server->data.sirkOp = (initParams->flags & 0x0F);

        memset(csis_server->data.connected_clients, 0, (sizeof(csis_client_data) * GATT_CSIS_MAX_CONNECTIONS));

        /* Setup the default value of Lock and Lock timeout */
        csisServerUpdateLock(CSIS_SERVER_UNLOCKED,
                             GATT_CSIS_SERVER_LOCK_EXPIRY_TIMEOUT_DEFAULT_VALUE,
                             0,
                             0,
                             NULL);

        /* Setup data required for Published Audio Capabilities Service
         * to be registered with the GATT Manager
         */
        reg_params.start_handle = startHandle;
        reg_params.end_handle = endHandle;
        reg_params.task = &csis_server->lib_task;

        /* Register with the GATT Manager and verify the result */
        if (GattManagerRegisterServer(&reg_params) == gatt_manager_status_success)
        {
            return srvc_hndl;
        }
        else
        {
            /* If the registration with GATT Manager fails and we have allocated memory
             * for the new instance successfully (service handle not zero), we have to free
             * the memnory of that instance.
             */
            if (srvc_hndl)
            {
                ServiceHandleFreeInstanceData(srvc_hndl);
            }
            return 0;
        }
    }
    else
    {
        GATT_CSIS_SERVER_DEBUG_PANIC(("Memory alllocation of CSIS Server instance failed!\n"));
        return 0;
    }
}


/******************************************************************************/
GattCsisServerConfigType *GattCsisServerRemoveConfig(CsisServerServiceHandleType handle,
                                                     CsisServerConnectionIdType cid)
{
    uint8 i;
    GCSISS_T *csis_server = (GCSISS_T *) ServiceHandleGetInstanceData(handle);
    GattCsisServerConfigType *client_config = NULL;

    if (csis_server == NULL)
        return client_config;

    for(i=0; i< GATT_CSIS_MAX_CONNECTIONS; i++)
    {
        /* Check the saved CID to find the peeer device */
        if (csis_server->data.connected_clients[i].cid == cid)
        {
            /* Found peer device:
             * - save last client configurations
             * - remove the peer device
             * - free the server instance
             * - return last client configuration
             */

            client_config = PanicUnlessMalloc(sizeof(GattCsisServerConfigType));
            memcpy(client_config, &(csis_server->data.connected_clients[i].client_cfg), sizeof(GattCsisServerConfigType));
            
            if ((i == (GATT_CSIS_MAX_CONNECTIONS-1)) || (i == 0 && csis_server->data.connected_clients[i+1].cid == 0))
            {
                /* The peer device is the only or the last element of the array */
                memset(&(csis_server->data.connected_clients[i]), 0, sizeof(csis_client_data));
            }
            else
            {
                /* The peer device is in the middle of the array */
                uint8 j;
            
                for (j=i; j< (GATT_CSIS_MAX_CONNECTIONS -1) && csis_server->data.connected_clients[j+1].cid != 0; j++)
                {
                    /* Shift all the elements of the array of one position behind */
                    memmove(&(csis_server->data.connected_clients[j]),
                           &(csis_server->data.connected_clients[j+1]),
                           sizeof(csis_client_data));
                }
            
                /* Remove the last element of the array, already shifted behind */
                memset(&(csis_server->data.connected_clients[j]), 0, sizeof(csis_client_data));
            }
        }
    }

    return client_config;
}

/******************************************************************************/
CsisServerGattStatusType GattCsisServerAddConfig(CsisServerServiceHandleType handle,
                                             CsisServerConnectionIdType cid,
                                             GattCsisServerConfigType *config)
{
    uint8 i;
    gatt_status_t res = gatt_status_failure;

    GCSISS_T *csis_server = (GCSISS_T *) ServiceHandleGetInstanceData(handle);

    if (csis_server == NULL)
        return gatt_status_invalid_phandle;

    for(i=0; i< GATT_CSIS_MAX_CONNECTIONS; i++)
    {
        if(csis_server->data.connected_clients[i].cid == 0)
        {
            csis_server->data.connected_clients[i].cid = cid;
            /* Check config parameter:
             * If config is NULL, it indicates a default config should be used for the
             * peer device identified by the CID.
             * The default config is already set when the instance has been initialised.
             */
            if (config)
            {
                if (config->lockValueClientCfg == GATT_CSIS_SERVER_CCC_INDICATE ||
                    config->sirkValueClientCfg == GATT_CSIS_SERVER_CCC_INDICATE ||
                    config->sizeValueClientCfg == GATT_CSIS_SERVER_CCC_INDICATE )
                {
                    /* CSIS characteristics can be only notified as per spec */
                    GATT_CSIS_SERVER_DEBUG_INFO(("Invalid Client Configuration Characteristic!\n"));
                    res = gatt_status_value_not_allowed;
                    break;
                }

                /* Save new ccc for the client */
                csis_server->data.connected_clients[i].client_cfg.lockValueClientCfg = config->lockValueClientCfg;
                csis_server->data.connected_clients[i].client_cfg.sirkValueClientCfg = config->sirkValueClientCfg;
                csis_server->data.connected_clients[i].client_cfg.sizeValueClientCfg = config->sizeValueClientCfg;

                /* Notify the connected client for Lock value */
                csisServerNotifyLockChange(csis_server, cid);    /* Lock */
            }

            res = gatt_status_success;
            break;
        }
    }

    if (res)
    {
        res = gatt_status_insufficient_resources;
    }

    return res;
}

CsisServerGattStatusType GattCsisServerUpdateInitParam(CsisServerServiceHandleType handle,
                                                       GattCsisServerInitParamType *initParams)
{
    GCSISS_T *csis_server = (GCSISS_T *) ServiceHandleGetInstanceData(handle);
    bool notifySizeChange = FALSE;
    bool notifySirkChange = FALSE;

    if (csis_server == NULL)
        return gatt_status_failure;

    /* Return success if the init params passed are same */
    if ((initParams->csSize == csis_server->data.cs_size) &&
        (initParams->rank == csis_server->data.rank) &&
        memcmp(initParams->sirk, csis_server->data.sirk, SIZE_SIRK_KEY) == 0)
    {
        return gatt_status_success;
    }

    /* Initialisation of the CSIS Server Charateristics */
    csis_server->data.rank = initParams->rank;

    if (csis_server->data.cs_size != initParams->csSize)
    {
        csis_server->data.cs_size = initParams->csSize;
        notifySizeChange = TRUE;
    }

    csis_server->data.sirkOp = (initParams->flags & 0x0F);

    if ((initParams->flags & GATT_CSIS_SIRK_UPDATE) == GATT_CSIS_SIRK_UPDATE &&
        memcmp(initParams->sirk, csis_server->data.sirk, SIZE_SIRK_KEY))
    {
        memcpy(csis_server->data.sirk, initParams->sirk, SIZE_SIRK_KEY);
        notifySirkChange = TRUE;
    }

    /* Notify all in case we are connected */
    if (notifySirkChange)
    {
        if((csis_server->data.sirkOp & GATT_CSIS_SHARE_SIRK_PLAIN_TEXT )
               == GATT_CSIS_SHARE_SIRK_PLAIN_TEXT)
        {
            /* We can notify the remote right away */
            csisServerNotifySirkChange(csis_server, 0);
        }
        else if((csis_server->data.sirkOp & GATT_CSIS_SHARE_SIRK_ENCRYPTED )
               == GATT_CSIS_SHARE_SIRK_ENCRYPTED)
        {
            /* We can notify remote connected client(s) only after generating the
             * encrypted SIRK for each connected client
             */
            csisServerGenerateEncryptedSirk(csis_server, GATT_CSIS_SERVER_NOTIFY, 0, 0);
        }
    }

    if (notifySizeChange)
        csisServerNotifySizeChange(csis_server, 0);

    return gatt_status_success;
}


