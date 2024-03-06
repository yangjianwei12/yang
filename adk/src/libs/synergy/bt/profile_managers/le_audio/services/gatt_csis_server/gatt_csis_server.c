/******************************************************************************
 Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "gatt_csis_server_private.h"
#include "gatt_csis_server_msg_handler.h"
#include "gatt_csis_server_notify.h"
#include "gatt_csis_server_lock_management.h"
#include "gatt_csis_server_debug.h"
#include "gatt_csis_server_access.h"

CsisServerServiceHandleType csis_service_handle;
CsrSchedTid csis_tId;

/****************************************************************************/
CsisServerServiceHandleType GattCsisServerInit(
               AppTask appTask,
               const GattCsisServerInitParamType *initParams,
               uint16 startHandle,
               uint16 endHandle)
{
     GCSISS_T *csis_server = NULL;
     uint8 i;

    /* validate the input parameters */
    if (appTask == CSR_SCHED_QID_INVALID)
    {
        GATT_CSIS_SERVER_PANIC(
                    "CSIS: Invalid Initialization parameters!"
                    );
    }

    csis_service_handle = ServiceHandleNewInstance((void **) &csis_server, sizeof(GCSISS_T));

    if (csis_server)
    {
        /* Reset all the service library memory */
        memset(csis_server, 0, sizeof(GCSISS_T));

        /* Set up interface queue for external messages */
        csis_server->lib_task = CSR_BT_CSIS_SERVER_IFACEQUEUE;

        /* Store the Task function parameter.
         * All library messages need to be sent here */
        csis_server->app_task = appTask;

        /* Save the service handle */
        csis_server->srvc_hndl = csis_service_handle;

        /* Initiliasation of the CSIS Server Charateristics */
        csis_server->data.rank = initParams->rank;
        csis_server->data.cs_size = initParams->csSize;

        CsrMemCpy(csis_server->data.sirk, initParams->sirk, SIZE_SIRK_KEY);

        /* Store the application preference for SIRK operation */
        if ((initParams->flags & 0x0F) == 0x00)
            csis_server->data.sirkOp = GATT_CSIS_SHARE_SIRK_PLAIN_TEXT; /* default if app fails to pass anything */
        else
            csis_server->data.sirkOp = (initParams->flags & 0x0F);

        memset(csis_server->data.connected_clients, 0, (sizeof(csis_client_data) * GATT_CSIS_MAX_CONNECTIONS));

        /* Reset the client config value to default GATT_CSIS_SERVER_INVALID_CLIENT_CONFIG */
        for(i = 0; i < GATT_CSIS_MAX_CONNECTIONS; i++)
        {
           csis_server->data.connected_clients[i].client_cfg.lockValueClientCfg = GATT_CSIS_SERVER_INVALID_CLIENT_CONFIG;
           csis_server->data.connected_clients[i].client_cfg.sirkValueClientCfg = GATT_CSIS_SERVER_INVALID_CLIENT_CONFIG;
           csis_server->data.connected_clients[i].client_cfg.sizeValueClientCfg = GATT_CSIS_SERVER_INVALID_CLIENT_CONFIG;
        }

        /* Setup the default value of Lock and Lock timeout */
        csisServerUpdateLock(CSIS_SERVER_UNLOCKED,
                             GATT_CSIS_SERVER_LOCK_EXPIRY_TIMEOUT_DEFAULT_VALUE,
                             0,
                             0,
                             NULL);

        /* Setup data required for CSIS service
         * to be registered with the GATT 
         */
        csis_server->start_handle = startHandle;
        csis_server->end_handle = endHandle;

        /* Register with the GATT */
        csis_server->gattId = CsrBtGattRegister(csis_server->lib_task);

        /* Check if gattId is valid */
        if (csis_server->gattId == CSR_BT_GATT_INVALID_GATT_ID)
        {
            ServiceHandleFreeInstanceData(csis_service_handle);
            csis_service_handle = 0;
            GATT_CSIS_SERVER_ERROR("Invalid Gatt Id!\n");
            return csis_service_handle;
        }
        else
        {
            CsrBtGattFlatDbRegisterHandleRangeReqSend(csis_server->gattId, startHandle,endHandle);
            return csis_service_handle;
        }
    }
    else
    {
        GATT_CSIS_SERVER_PANIC("Memory allocation of CSIS Server instance failed!\n");
        return 0;
    }
}


/******************************************************************************/
GattCsisServerConfigType *GattCsisServerRemoveConfig(
                          CsisServerServiceHandleType handle,
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

            client_config = CsrPmemAlloc(sizeof(GattCsisServerConfigType));
            CsrMemCpy(client_config, &(csis_server->data.connected_clients[i].client_cfg), sizeof(GattCsisServerConfigType));
            
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
    status_t res = CSR_BT_GATT_ACCESS_RES_UNLIKELY_ERROR;

    GCSISS_T *csis_server = (GCSISS_T *) ServiceHandleGetInstanceData(handle);

    if (csis_server == NULL)
    	return CSR_BT_GATT_ACCESS_RES_INVALID_HANDLE;

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
                    GATT_CSIS_SERVER_ERROR("Invalid Client Configuration Characteristic!\n");
                    res = CSR_BT_GATT_ACCESS_RES_INVALID_PDU;
                    break;
                }

                /* Save new ccc for the client */
                csis_server->data.connected_clients[i].client_cfg.lockValueClientCfg = config->lockValueClientCfg;
                csis_server->data.connected_clients[i].client_cfg.sirkValueClientCfg = config->sirkValueClientCfg;
                csis_server->data.connected_clients[i].client_cfg.sizeValueClientCfg = config->sizeValueClientCfg;

                /* Notify the connected client for values for CCCD which are set */
                csisServerNotifyLockChange(csis_server, cid);    /* Lock */
            }

            res = CSR_BT_GATT_ACCESS_RES_SUCCESS;
            break;
        }
    }

    if (res)
    {
        res = CSR_BT_GATT_ACCESS_RES_INSUFFICIENT_RESOURCES;
    }

    return res;
}

/******************************************************************************/
GattCsisServerConfigType *GattCsisServerGetConfig(CsisServerServiceHandleType handle,
    CsisServerConnectionIdType cid)
{
    uint8 i;
    GCSISS_T *csis_server = (GCSISS_T*) ServiceHandleGetInstanceData(handle);
    GattCsisServerConfigType *config;

    if(csis_server)
    {
        for(i=0; i<GATT_CSIS_MAX_CONNECTIONS; i++)
        {
            /* Check the saved CID to find the peeer device */
            if (csis_server->data.connected_clients[i].cid == cid)
            {
                /* Found peer device:
                 * - save last client configurations
                 * - return last client configuration
                 */

                config = (GattCsisServerConfigType *)CsrPmemAlloc(sizeof(GattCsisServerConfigType));
                CsrMemCpy(config, &(csis_server->data.connected_clients[i].client_cfg), sizeof(GattCsisServerConfigType));
                return config;
            }
        }
    }
    return NULL;
}

CsisServerGattStatusType GattCsisServerUpdateInitParam(
                         CsisServerServiceHandleType handle,
                         GattCsisServerInitParamType *initParams)
{
    GCSISS_T* csis_server = (GCSISS_T*)ServiceHandleGetInstanceData(handle);
    bool notifySizeChange = FALSE;
    bool notifySirkChange = FALSE;

    if (csis_server == NULL)
        return CSR_BT_GATT_ACCESS_RES_INVALID_HANDLE;

    /* Return success if the init params passed are same */
    if ((initParams->csSize == csis_server->data.cs_size) &&
        (initParams->rank == csis_server->data.rank) &&
        (memcmp(initParams->sirk, csis_server->data.sirk, SIZE_SIRK_KEY) == 0))
    {
        return CSR_BT_GATT_ACCESS_RES_SUCCESS;
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
        if ((csis_server->data.sirkOp & GATT_CSIS_SHARE_SIRK_PLAIN_TEXT)
            == GATT_CSIS_SHARE_SIRK_PLAIN_TEXT)
        {
            /* We can notify the remote right away */
            csisServerNotifySirkChange(csis_server, 0);
        }
        else if ((csis_server->data.sirkOp & GATT_CSIS_SHARE_SIRK_ENCRYPTED)
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

    return CSR_BT_GATT_ACCESS_RES_SUCCESS;

}


void gatt_csis_server_init(void** gash)
{
    *gash = &csis_service_handle;
    GATT_CSIS_SERVER_INFO("\nCSIS: gatt_csis_server_init\n");
}

#ifdef ENABLE_SHUTDOWN
void gatt_csis_server_deinit(void** gash)
{
    ServiceHandle serviceHandle = *((ServiceHandle*)*gash);

    if (serviceHandle)
    {
        if(ServiceHandleFreeInstanceData(serviceHandle))
        {
            GATT_CSIS_SERVER_INFO("CSIS: gatt_csis_server_deinit\n\n");
        }
        else
        {
            GATT_CSIS_SERVER_PANIC("CSIS: deinit - Unable to free CSIS server instance.\n");
        }
    }
    else
    {
        GATT_CSIS_SERVER_INFO("CSIS: deinit - Invalid Service Handle\n\n");
    }
}
#endif
