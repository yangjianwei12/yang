/******************************************************************************
 Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

#include "gatt_mics_server_private.h"
#include "gatt_mics_server_access.h"
#include "gatt_mics_server_debug.h"
#include "gatt_mics_server_db.h"

ServiceHandle micsServiceHandle;

static void initMicsClientData(GattMicsClientData*clientDataPtr)
{
    if(clientDataPtr == NULL)
    {
        GATT_MICS_SERVER_PANIC("GMICS_T: Client data pointer is a NULL pointer");
    }
    else
    {
        clientDataPtr->cid = 0;
        clientDataPtr->clientCfg.micsMuteClientCfg = 0;
    }
}

/* Notify all the connected clients when the characteristic of 'type'
 * is changed
 * */

void micsServerNotifyConnectedClients(GMICS_T* mics)
{
    uint16 i;

    for (i=0; i< MICS_MAX_CONNECTIONS; i++)
    {
        if (mics->data.connectedClients[i].cid != 0)
        {
             /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
             if (mics->data.connectedClients[i].clientCfg.micsMuteClientCfg == MICS_SERVER_NOTIFY)
             {
                 micsServerSendCharacteristicChangedNotification(
                                        mics->gattId,
                                        mics->data.connectedClients[i].cid,
                                        HANDLE_MICS_SERVER_MUTE,
                                        MICS_SERVER_MUTE_SIZE,
                                        &(mics->data.micsServerMute)
                                        );
             }
        }
    }
}

/******************************************************************************/
ServiceHandle GattMicsServerInit(AppTask theAppTask,
                                    uint16  startHandle,
                                    uint16  endHandle,
                                    const GattMicsInitData* initData)
{
    GMICS_T  *mics = NULL;
    CsrBtGattId gattId;
	int clientIndex;

    if(theAppTask == CSR_SCHED_QID_INVALID)
    {
        GATT_MICS_SERVER_PANIC("GattMcsServerInit fail invalid params");
        return 0;
    }

    micsServiceHandle = ServiceHandleNewInstance((void **) &mics, sizeof(GMICS_T));

    if (micsServiceHandle !=0)
    {
        /* Reset all the service library memory */
        CsrMemSet(mics, 0, sizeof(GMICS_T));

        mics->srvcHandle = micsServiceHandle;
        mics->ind_pending = FALSE;

        /* Set up library handler for external messages */
        mics->libTask = CSR_BT_MICS_SERVER_IFACEQUEUE;

        /* Store the Task function parameter.
           All library messages need to be sent here */
        mics->appTask = theAppTask;

        if(initData)
        {
            SynMemCpyS(&(mics->data), sizeof(GattMicsInitData),
                             initData, sizeof(GattMicsInitData));
        }
        else{
            CsrMemSet(&(mics->data), 0, sizeof(GattMicsInitData));
        }

        mics->data.numClients = 0;

        for (clientIndex=0; clientIndex< MICS_MAX_CONNECTIONS; clientIndex++)
        {
        	initMicsClientData(&(mics->data.connectedClients[clientIndex]));
        }

        /* Register with the GATT  */
        gattId = CsrBtGattRegister(mics->libTask);
        /* verify the result */
        if (gattId == CSR_BT_GATT_INVALID_GATT_ID)
        {
            ServiceHandleFreeInstanceData(micsServiceHandle);
            micsServiceHandle = 0;
        }
        else
        {
            mics->gattId = gattId;
            CsrBtGattFlatDbRegisterHandleRangeReqSend(gattId, startHandle, endHandle);
        }
    }
    return micsServiceHandle;
}

/******************************************************************************/
status_t GattMicsServerAddConfig(ServiceHandle srvcHndl,
                                connection_id_t  cid,
                                GattMicsClientConfigDataType *const config)
{
    uint8 i;
    GMICS_T* mics = (GMICS_T*)ServiceHandleGetInstanceData(srvcHndl);

    if (mics == NULL)
    {
        GATT_MICS_SERVER_ERROR("GMICS_T: MICS server is a NULL pointer");
        return CSR_BT_GATT_ACCESS_RES_INVALID_HANDLE;
    }

    for (i = 0; i < MICS_MAX_CONNECTIONS; i++)
    {
        if (mics->data.connectedClients[i].cid == 0)
        {
            /* Add client btConnId to the server data */
            mics->data.connectedClients[i].cid = cid;

            /* Check config parameter:
             * If config is NULL, it indicates a default config should be used for the
             * peer device identified by the CID.
             * The default config is already set when the instance has been initialised.
             */

            if (config)
            {
                /* Save new ccc for the client */
                if(config->micsMuteClientCfg == MICS_SERVER_INDICATE)
                {
                    GATT_MICS_SERVER_ERROR("Invalid Client Configuration Characteristic!\n");
                    return CSR_BT_GATT_RESULT_INVALID_ATTRIBUTE_VALUE_RECEIVED;
                }
                else
                {
                    SynMemCpyS(&(mics->data.connectedClients[i].clientCfg),
                              sizeof(GattMicsClientConfigDataType),
                              config,
                              sizeof(GattMicsClientConfigDataType));
                }
                /* Notify  characteristic enabled for notification */
                micsServerNotifyConnectedClients(mics);
            }
            return CSR_BT_GATT_ACCESS_RES_SUCCESS;
        }
    }

    return CSR_BT_GATT_ACCESS_RES_INSUFFICIENT_RESOURCES;
}

/******************************************************************************/
GattMicsClientConfigDataType* GattMicsServerRemoveConfig(ServiceHandle srvcHndl,
                                                         connection_id_t  cid)
{
    uint8 i;
    GattMicsClientConfigDataType* config = NULL;
    GMICS_T* mics = (GMICS_T*)ServiceHandleGetInstanceData(srvcHndl);

    if (mics == NULL)
    {
        GATT_MICS_SERVER_ERROR("\n MICS: NULL instance \n");
        return NULL;
    }

    for (i = 0; i < MICS_MAX_CONNECTIONS; i++)
    {
        /* Check the saved CID to find the peeer device */
        if (mics->data.connectedClients[i].cid == cid)
        {
            /* Found peer device:
             * - save last client configurations
             * - remove the peer device
             * - free the server instance
             * - return last client configuration
             */

            config = CsrPmemAlloc(sizeof(GattMicsClientConfigDataType));
            SynMemCpyS(config, sizeof(GattMicsClientConfigDataType),
                       &(mics->data.connectedClients[i].clientCfg),
                              sizeof(GattMicsClientConfigDataType));

            if ((i == (MICS_MAX_CONNECTIONS - 1)) || (i == 0 && mics->data.connectedClients[i + 1].cid == 0))
            {
                /* The peer device is the only or the last element of the array */
                CsrMemSet(&(mics->data.connectedClients[i]), 0, sizeof(GattMicsClientData));
            }
            else
            {
                /* The peer device is in the middle of the array */
                uint8 j;

                for (j = i; j < (MICS_MAX_CONNECTIONS - 1) && mics->data.connectedClients[j + 1].cid != 0; j++)
                {
                    /* Shift all the elements of the array of one position behind */
                    CsrMemMove(&(mics->data.connectedClients[j]),
                                 &(mics->data.connectedClients[j + 1]),
                                         sizeof(GattMicsClientData));
                }

                /* Remove the last element of the array, already shifted behind */
                CsrMemSet(&(mics->data.connectedClients[j]), 0, sizeof(GattMicsClientData));
            }
        }
    }
    return config;
}

/******************************************************************************/
GattMicsClientConfigDataType* GattMicsServerGetConfig(
                        ServiceHandle srvcHndl,
                        connection_id_t  cid)
{
    uint8 i;
    GattMicsClientConfigDataType* config;
    GMICS_T* mics = (GMICS_T*)ServiceHandleGetInstanceData(srvcHndl);

    if(mics)
    {
        for(i=0; i<MICS_MAX_CONNECTIONS; i++)
        {
            /* Check the saved CID to find the peeer device */
            if (mics->data.connectedClients[i].cid == cid)
            {
                /* Found peer device:
                 * - save last client configurations
                 * - return last client configuration
                 */

                config = (GattMicsClientConfigDataType *)CsrPmemAlloc(sizeof(GattMicsClientConfigDataType));
                CsrMemCpy(config, &(mics->data.connectedClients[i].clientCfg), sizeof(GattMicsClientConfigDataType));
                return config;
            }
        }
    }
    return NULL;

}

/******************************************************************************/
bool GattMicsServerSetMicState(ServiceHandle srvcHndl,
                              uint8 value)
{
    GMICS_T *mics = (GMICS_T *) ServiceHandleGetInstanceData(srvcHndl);

    if(mics == NULL)
    {
        GATT_MICS_SERVER_PANIC("\nGMICS: NULL instance encountered\n");
        return FALSE;
    }

    if (value > MICS_SERVER_MUTE_DISABLED)
    {
        GATT_MICS_SERVER_ERROR("\nGMICS: Value out of range\n");
        return FALSE;
    }

    /* update the mute characteristic value */
    if(value == mics->data.micsServerMute)
    {
        return TRUE;
    }

    mics->data.micsServerMute = value;

    /* notify the change*/
    micsServerNotifyConnectedClients(mics);
    return TRUE;
}

/******************************************************************************/
bool GattMicsServerReadMicState(ServiceHandle srvcHndl,
                              uint8 *value)
{
    GMICS_T *mics = (GMICS_T *) ServiceHandleGetInstanceData(srvcHndl);

    if(value == NULL)
    {
        GATT_MICS_SERVER_PANIC("\nGMICS: value is NULL\n");
        return FALSE;
    }

    if(mics == NULL)
    {
        GATT_MICS_SERVER_PANIC("\nGMICS: NULL instance encountered\n");
        *value = 0xFF;
        return FALSE;
    }

    *value = mics->data.micsServerMute;
    return TRUE;
}

/******************************************************************************/
bool GattMicsServerSetMicStateRsp(ServiceHandle srvcHndl,
                               connection_id_t cid)
{
    GMICS_T *mics = (GMICS_T *) ServiceHandleGetInstanceData(srvcHndl);

    if(mics == NULL)
    {
        GATT_MICS_SERVER_PANIC("\nGMICS: NULL instance encountered\n");
        return FALSE;
    }

    if(mics->ind_pending == FALSE)
    {
        GATT_MICS_SERVER_ERROR("\nGMICS: No indication pending that needs response\n");
        return FALSE;
    }

    gattMicsServerWriteGenericResponse(
                                 mics->gattId,
                                 cid,
                                 CSR_BT_GATT_ACCESS_RES_SUCCESS,
                                 HANDLE_MICS_SERVER_MUTE
                                 );

    /* Toggle the mute state and clear ind_pending */
    mics->data.micsServerMute ^= 1;
    mics->ind_pending = FALSE;

    /* notify the change*/
    micsServerNotifyConnectedClients(mics);
    return TRUE;
}



/******************************************************************************/
/* MICS server init Synergy Scheduler Task */

void gatt_mics_server_init(void** gash)
{
    *gash = &micsServiceHandle;
    GATT_MICS_SERVER_INFO("GMICS: gatt_mics_server_init\n\n");
}

/******************************************************************************/
/* MICS server De-init Synergy Scheduler Task */

#ifdef ENABLE_SHUTDOWN
void gatt_mics_server_deinit(void** gash)
{
    ServiceHandle srvcHndl = *((ServiceHandle*)*gash);

    if (srvcHndl)
    {
        if(ServiceHandleFreeInstanceData(srvcHndl))
        {
            GATT_MICS_SERVER_INFO("GMICS: gatt_mic_server_deinit\n\n");
        }
        else
        {
            GATT_MICS_SERVER_PANIC("Unable to free the GMICS server instance\n");
        }
    }
    else
    {
        GATT_MICS_SERVER_DEBUG("GMICS: Invalid Service Handle\n\n");
    }
}
#endif

