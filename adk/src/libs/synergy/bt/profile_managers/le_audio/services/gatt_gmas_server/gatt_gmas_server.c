/******************************************************************************
 Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #4 $
******************************************************************************/

#include "gatt_gmas_server_msg_handler.h"
#include "gatt_gmas_server_common.h"
/******************************************************************************/
ServiceHandle gmasServerServiceHandle;

ServiceHandle GattGmasServerInit(AppTask    theAppTask,
                                 uint16  startHandle,
                                 uint16  endHandle,
                                 GattGmasInitData* initData)
{
    GGMAS *gattGmasServerInst = NULL;


    if (theAppTask == CSR_SCHED_QID_INVALID)
    {
        GATT_GMAS_SERVER_PANIC("Application Task is NULL\n");
    }

    gmasServerServiceHandle = ServiceHandleNewInstance((void **) &gattGmasServerInst, sizeof(GGMAS));

    if (gattGmasServerInst)
    {
        /* Reset all the service library memory */
        CsrMemSet(gattGmasServerInst, 0, sizeof(GGMAS));
        /* Reset the client data memory */
        CsrMemSet(gattGmasServerInst->data.connectedClients, 0, 
                    (sizeof(GattGmasClientData) * GATT_GMAS_MAX_CONNECTIONS));

        /* Store the Task function parameter.
         * All library messages need to be sent here */
        gattGmasServerInst->appTask = theAppTask;

        gattGmasServerInst->srvcHndl = gmasServerServiceHandle;
        gattGmasServerInst->startHandle = startHandle;
        gattGmasServerInst->endHandle = endHandle;

        if(initData)
        {
           /* Initiliasation of the GMAS Charateristics  and parameters*/
            gattGmasServerInst->data.role = initData->role;

#if defined(ENABLE_GMAP_UGT_BGR)
            gattGmasServerInst->data.ugtFeatures = initData->ugtFeatures;
            gattGmasServerInst->data.bgrFeatures = initData->bgrFeatures;
#endif
#if defined(ENABLE_GMAP_UGG_BGS)
            gattGmasServerInst->data.uggFeatures = initData->uggFeatures;
            gattGmasServerInst->data.bgsFeatures = initData->bgsFeatures;
#endif
        }

        /* Register with the GATT */
        CsrBtGattRegisterReqSend(CSR_BT_GMAS_SERVER_IFACEQUEUE, 0);
        return gattGmasServerInst->srvcHndl;
    }
    else
    {
        GATT_GMAS_SERVER_PANIC("Memory alllocation of GMAS Server instance failed!\n");
    }
    return 0;
}

/******************************************************************************/
status_t GattGmasServerAddConfig(ServiceHandle srvcHndl,
                                 connection_id_t  cid)
{
    uint8 i;
    GGMAS* gattGmasServerInst = (GGMAS*)ServiceHandleGetInstanceData(srvcHndl);

    if (gattGmasServerInst == NULL)
    {
        GATT_GMAS_SERVER_ERROR("\nGMAS: Instance is NULL\n");
        return CSR_BT_GATT_ACCESS_RES_INVALID_HANDLE;
    }

    for (i = 0; i < GATT_GMAS_MAX_CONNECTIONS; i++)
    {
        if (gattGmasServerInst->data.connectedClients[i].cid == 0)
        {
            /* Add client btConnId to the server data */
            gattGmasServerInst->data.connectedClients[i].cid = cid;

            return CSR_BT_GATT_ACCESS_RES_SUCCESS;
        }
    }

    return CSR_BT_GATT_ACCESS_RES_INSUFFICIENT_RESOURCES;
}

/******************************************************************************/
bool GattGmasServerRemoveConfig(ServiceHandle srvcHndl,
                                connection_id_t  cid)
{
    uint8 i;
    GGMAS* gattGmasServerInst = (GGMAS*)ServiceHandleGetInstanceData(srvcHndl);

    if (gattGmasServerInst == NULL)
    {
        GATT_GMAS_SERVER_ERROR("\n GMAS: NULL instance \n");
        return FALSE;
    }

    for (i = 0; i < GATT_GMAS_MAX_CONNECTIONS; i++)
    {
        /* Check the saved CID to find the peeer device */
        if (gattGmasServerInst->data.connectedClients[i].cid == cid)
        {
            if ((i == (GATT_GMAS_MAX_CONNECTIONS - 1)) || (i == 0 && gattGmasServerInst->data.connectedClients[i + 1].cid == 0))
            {
                /* The peer device is the only or the last element of the array */
                memset(&(gattGmasServerInst->data.connectedClients[i]), 0, sizeof(GattGmasClientData));
            }
            else
            {
                /* The peer device is in the middle of the array */
                uint8 j;

                for (j = i; j < (GATT_GMAS_MAX_CONNECTIONS - 1) && gattGmasServerInst->data.connectedClients[j + 1].cid != 0; j++)
                {
                    /* Shift all the elements of the array of one position behind */
                    memmove(&(gattGmasServerInst->data.connectedClients[j]),
                        &(gattGmasServerInst->data.connectedClients[j + 1]),
                             sizeof(GattGmasClientData));
                }

                /* Remove the last element of the array, already shifted behind */
                memset(&(gattGmasServerInst->data.connectedClients[j]), 0, sizeof(GattGmasClientData));
            }
        }
    }

    return TRUE;
}


/******************************************************************************/
bool GattGmasServerSetRoleFeature(ServiceHandle srvcHndl, GmasRole role, uint8 unicastFeatures, uint8 broadcastFeatures)
{
    GGMAS *gattGmasServerInst = (GGMAS *) ServiceHandleGetInstanceData(srvcHndl);

    if (gattGmasServerInst)
    {
        if (!(((role & GMAS_ROLE_UNICAST_GAME_GATEWAY) ==  GMAS_ROLE_UNICAST_GAME_GATEWAY)  ||
           ((role & GMAS_ROLE_UNICAST_GAME_TERMINAL) ==  GMAS_ROLE_UNICAST_GAME_TERMINAL) ||
           ((role & GMAS_ROLE_BROADCAST_GAME_SENDER) ==  GMAS_ROLE_BROADCAST_GAME_SENDER) ||
           ((role & GMAS_ROLE_BROADCAST_GAME_RECEIVER) ==  GMAS_ROLE_BROADCAST_GAME_RECEIVER)))
        {
            GATT_GMAS_SERVER_WARNING("GattGmasServerSetRoleFeature:Invalid role");
            return FALSE;
        }

        gattGmasServerInst->data.role = role;

#if defined(ENABLE_GMAP_UGG_BGS)
        if ((role & GMAS_ROLE_UNICAST_GAME_GATEWAY) ==  GMAS_ROLE_UNICAST_GAME_GATEWAY)
        {
            gattGmasServerInst->data.uggFeatures = unicastFeatures;
        }

        if ((role & GMAS_ROLE_BROADCAST_GAME_SENDER) ==  GMAS_ROLE_BROADCAST_GAME_SENDER)
        {
            gattGmasServerInst->data.bgsFeatures = broadcastFeatures;
        }
#endif
#if defined(ENABLE_GMAP_UGT_BGR)
        if ((role & GMAS_ROLE_UNICAST_GAME_TERMINAL) ==  GMAS_ROLE_UNICAST_GAME_TERMINAL)
        {
            gattGmasServerInst->data.ugtFeatures = unicastFeatures;
        }

        if ((role & GMAS_ROLE_BROADCAST_GAME_RECEIVER) ==  GMAS_ROLE_BROADCAST_GAME_RECEIVER)
        {
            gattGmasServerInst->data.bgrFeatures = broadcastFeatures;
        }
#endif
        return TRUE;
    }
    GATT_GMAS_SERVER_WARNING("GattGmasServerSetRoleFeature:Invalid Service Handle");
    return FALSE;
}

uint8 GattGmasServerGetUnicastFeatures(ServiceHandle srvcHndl, GmasRole unicastRole)
{
    GGMAS *gattGmasServerInst = (GGMAS *) ServiceHandleGetInstanceData(srvcHndl);
    uint8 charcValue = 0;
    if (gattGmasServerInst)
    {
        if (!(((unicastRole & GMAS_ROLE_UNICAST_GAME_GATEWAY) ==  GMAS_ROLE_UNICAST_GAME_GATEWAY) ||
           ((unicastRole & GMAS_ROLE_UNICAST_GAME_TERMINAL) ==  GMAS_ROLE_UNICAST_GAME_TERMINAL)))
        {
            GATT_GMAS_SERVER_WARNING("GattGmasServerGetUnicastFeatures:Invalid role");
            return 0;
        }

#if defined(ENABLE_GMAP_UGG_BGS)
        if ((unicastRole & GMAS_ROLE_UNICAST_GAME_GATEWAY) ==  GMAS_ROLE_UNICAST_GAME_GATEWAY)
        {
            charcValue = gattGmasServerInst->data.uggFeatures;
        }
#endif
#if defined(ENABLE_GMAP_UGT_BGR)
        if ((unicastRole & GMAS_ROLE_UNICAST_GAME_TERMINAL) ==  GMAS_ROLE_UNICAST_GAME_TERMINAL)
        {
            charcValue = gattGmasServerInst->data.ugtFeatures;
        }
#endif
    }

    return charcValue;
}

uint8 GattGmasServerGetBroadcastFeatures(ServiceHandle srvcHndl, GmasRole broadcastRole)
{
    GGMAS *gattGmasServerInst = (GGMAS *) ServiceHandleGetInstanceData(srvcHndl);
    uint8 charcValue = 0;
    if (gattGmasServerInst)
    {
        if (!(((broadcastRole & GMAS_ROLE_BROADCAST_GAME_SENDER) ==  GMAS_ROLE_BROADCAST_GAME_SENDER) ||
           ((broadcastRole & GMAS_ROLE_BROADCAST_GAME_RECEIVER) ==  GMAS_ROLE_BROADCAST_GAME_RECEIVER)))
        {
            GATT_GMAS_SERVER_WARNING("GattGmasServerGetBroadcastFeatures:Invalid role");
            return 0;
        }

#if defined(ENABLE_GMAP_UGG_BGS)
        if ((broadcastRole & GMAS_ROLE_BROADCAST_GAME_SENDER) ==  GMAS_ROLE_BROADCAST_GAME_SENDER)
        {
            charcValue = gattGmasServerInst->data.bgsFeatures;
        }
#endif
#if defined(ENABLE_GMAP_UGT_BGR)
        if ((broadcastRole & GMAS_ROLE_BROADCAST_GAME_RECEIVER) ==  GMAS_ROLE_BROADCAST_GAME_RECEIVER)
        {
            charcValue = gattGmasServerInst->data.bgrFeatures;
        }
#endif
    }

    return charcValue;
}

void gattGmasServerInit(void** gash)
{
    *gash = &gmasServerServiceHandle;
    GATT_GMAS_SERVER_INFO("\nGMAS: gattGmasServerInit\n");

}

/*Synergy Task deinit*/
#ifdef ENABLE_SHUTDOWN
void gattGmasServerDeinit(void** gash)
{
    ServiceHandle srvcHndl = *((ServiceHandle*)*gash);
    if(ServiceHandleFreeInstanceData(srvcHndl))
    {
        GATT_GMAS_SERVER_INFO("\nGMAS: gattGmasServerDeinit\n");
    }
    else
    {
        GATT_GMAS_SERVER_ERROR("\nUnable to free the GMA server instance\n");
    }
}
#endif
