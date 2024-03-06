/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #58 $
******************************************************************************/

#include "gatt_tmas_server_msg_handler.h"
#include "gatt_tmas_server_common.h"
/******************************************************************************/
ServiceHandle tmasServerServiceHandle;

ServiceHandle GattTmasServerInit(AppTask    theAppTask,
                                 uint16  startHandle,
                                 uint16  endHandle,
                                 GattTmasInitData* initData)
{
    GTMAS *gattTmasServerInst = NULL;


    if (theAppTask == CSR_SCHED_QID_INVALID)
    {
        GATT_TMAS_SERVER_PANIC("Application Task NULL\n");
    }

    tmasServerServiceHandle = ServiceHandleNewInstance((void **) &gattTmasServerInst, sizeof(GTMAS));

    if (gattTmasServerInst)
    {
        /* Reset all the service library memory */
        CsrMemSet(gattTmasServerInst, 0, sizeof(GTMAS));

        /* Store the Task function parameter.
         * All library messages need to be sent here */
        gattTmasServerInst->appTask = theAppTask;

       /* Initiliasation of the TMAS Charateristics  and parameters*/
        gattTmasServerInst->data.role = initData->role;

        gattTmasServerInst->srvcHndl = tmasServerServiceHandle;
        gattTmasServerInst->startHandle = startHandle;
        gattTmasServerInst->endHandle = endHandle;

        /* Reset the client data memory */
        CsrMemSet(gattTmasServerInst->data.connectedClients, 0, (sizeof(gattTmasClientData) * GATT_TMAS_MAX_CONNECTIONS));


        /* Register with the GATT */
        CsrBtGattRegisterReqSend(CSR_BT_TMAS_SERVER_IFACEQUEUE,
                                 0);

        return gattTmasServerInst->srvcHndl;

    }
    else
    {
        GATT_TMAS_SERVER_PANIC("Memory alllocation of TMAS Server instance failed!\n");
    }
    return 0;
}

/******************************************************************************/
status_t GattTmasServerAddConfig(ServiceHandle srvcHndl,
                                 connection_id_t  cid)
{
    uint8 i;
    GTMAS* gattTmasServerInst = (GTMAS*)ServiceHandleGetInstanceData(srvcHndl);

    if (gattTmasServerInst == NULL)
    {
        GATT_TMAS_SERVER_ERROR("/nTMAS: is NULL/n");
        return CSR_BT_GATT_ACCESS_RES_INVALID_HANDLE;
    }

    for (i = 0; i < GATT_TMAS_MAX_CONNECTIONS; i++)
    {
        if (gattTmasServerInst->data.connectedClients[i].cid == 0)
        {
            /* Add client btConnId to the server data */
            gattTmasServerInst->data.connectedClients[i].cid = cid;

            return CSR_BT_GATT_ACCESS_RES_SUCCESS;
        }
    }

    return CSR_BT_GATT_ACCESS_RES_INSUFFICIENT_RESOURCES;
}

/******************************************************************************/
bool GattTmasServerRemoveConfig(ServiceHandle srvcHndl,
                                connection_id_t  cid)
{
    uint8 i;
    GTMAS* gattTmasServerInst = (GTMAS*)ServiceHandleGetInstanceData(srvcHndl);

    if (gattTmasServerInst == NULL)
    {
        GATT_TMAS_SERVER_ERROR("\n TMAS: NULL instance \n");
        return FALSE;
    }

    for (i = 0; i < GATT_TMAS_MAX_CONNECTIONS; i++)
    {
        /* Check the saved CID to find the peeer device */
        if (gattTmasServerInst->data.connectedClients[i].cid == cid)
        {
            if ((i == (GATT_TMAS_MAX_CONNECTIONS - 1)) || (i == 0 && gattTmasServerInst->data.connectedClients[i + 1].cid == 0))
            {
                /* The peer device is the only or the last element of the array */
                memset(&(gattTmasServerInst->data.connectedClients[i]), 0, sizeof(gattTmasClientData));
            }
            else
            {
                /* The peer device is in the middle of the array */
                uint8 j;

                for (j = i; j < (GATT_TMAS_MAX_CONNECTIONS - 1) && gattTmasServerInst->data.connectedClients[j + 1].cid != 0; j++)
                {
                    /* Shift all the elements of the array of one position behind */
                    memmove(&(gattTmasServerInst->data.connectedClients[j]),
                        &(gattTmasServerInst->data.connectedClients[j + 1]),
                             sizeof(gattTmasClientData));
                }

                /* Remove the last element of the array, already shifted behind */
                memset(&(gattTmasServerInst->data.connectedClients[j]), 0, sizeof(gattTmasClientData));
            }
        }
    }

    return TRUE;
}


/******************************************************************************/
bool GattTmasServerSetRole(ServiceHandle srvcHndl, uint16 role)
{
    GTMAS *tmasServer = (GTMAS *) ServiceHandleGetInstanceData(srvcHndl);

    if (tmasServer && (role != tmasServer->data.role))
    {
        /* The role value to set is valid and different from the actual one*/
        tmasServer->data.role = role;

        return TRUE;
    }
    return FALSE;
}

uint16 GattTmasServerGetRole(ServiceHandle srvcHndl)
{
    GTMAS *tmasServer = (GTMAS *) ServiceHandleGetInstanceData(srvcHndl);
    uint16 role = tmasServer ? tmasServer->data.role : 0;
    return role;
}

void gattTmasServerInit(void** gash)
{
    *gash = &tmasServerServiceHandle;
    GATT_TMAS_SERVER_INFO("TMAS: gattTmasServerInit\n\n");

}

/*Synergy Task deinit*/
#ifdef ENABLE_SHUTDOWN
void gattTmasServerDeinit(void** gash)
{
    ServiceHandle srvcHndl = *((ServiceHandle*)*gash);
    if(ServiceHandleFreeInstanceData(srvcHndl))
    {
        GATT_TMAS_SERVER_INFO("TMAS: gattTmasServerDeinit\n\n");
    }
    else
    {
        GATT_TMAS_SERVER_ERROR("Unable to free the TMA server instance\n");
    }
}
#endif
