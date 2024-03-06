/******************************************************************************
 Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #2 $
******************************************************************************/


#include "gatt_gmas_client_private.h"
#include "gatt_gmas_client_debug.h"
#include "gatt_gmas_client.h"
#include "gatt_gmas_client_discovery.h"
#include "gatt_gmas_client_read.h"
#include "csr_bt_gatt_client_util_lib.h"
#include "gatt_gmas_client_init.h"
#include "gatt_gmas_client_common_util.h"

static void gattGmasClientHandleGattMsg(void *task, MsgId id, Msg msg)
{
    GGMASC *gattGmasClient = (GGMASC *)task;
    
    switch (id)
    {
        case CSR_BT_GATT_DISCOVER_CHARAC_CFM:
        {
            gattGmasClientHandleDiscoverAllGmasCharacteristicsResp(gattGmasClient,
                                                                   (const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *)msg);
        }
        break;

        case CSR_BT_GATT_READ_CFM:
        {
            CsrBtGattReadCfm* message = (CsrBtGattReadCfm*)msg;
            gattGmasClientHandleReadValueRespCfm(gattGmasClient,
                                                 (const CsrBtGattReadCfm *) msg);

            if (message->value && message->valueLength)
            {
                CsrPmemFree(message->value);
                message->value = NULL;
            }
        }
        break;

        default:
        {
            /* Unrecognised GATT Manager message */
            GATT_GMAS_CLIENT_WARNING("GGMASC: GMA Client GattMgr Msg not handled [0x%x]\n", id);
        }
        break;
    }
}

/***************************************************************************/
static void  gattGmasClientHandleInternalMessage(void *task, MsgId id, Msg msg)
{
    GGMASC *gmasClient = (GGMASC *)task;

    GATT_GMAS_CLIENT_INFO("Message id (%d)\n",id);

    if (gmasClient)
    {
        switch(id)
        {
            case GATT_GMAS_CLIENT_INTERNAL_MSG_READ_ROLE:
            {
                GattGmasClientInternalMsgReadRole* message = (GattGmasClientInternalMsgReadRole*) msg;

                gattGmasClientHandleInternalReadRole(gmasClient,
                                                     message->handle);
            }
            break;

            case GATT_GMAS_CLIENT_INTERNAL_MSG_READ_UNICAST_FEATURES:
            {
                GattGmasClientInternalMsgReadUnicastFeatures* message = (GattGmasClientInternalMsgReadUnicastFeatures*) msg;

                gattGmasClientHandleInternalReadUnicastFeatures(gmasClient,
                                                                message->handle);
            }
            break;

            case GATT_GMAS_CLIENT_INTERNAL_MSG_READ_BROADCAST_FEATURES:
            {
                GattGmasClientInternalMsgReadBroadcastFeatures* message = (GattGmasClientInternalMsgReadBroadcastFeatures*) msg;

                gattGmasClientHandleInternalReadBroadcastFeatures(gmasClient,
                                                                  message->handle);
            }
            break;

            default:
            {
                /* Internal unrecognised messages */
                GATT_GMAS_CLIENT_WARNING("Unknown Message received from Internal To lib \n");
            }
            break;
        }
    }
}

/****************************************************************************/
void gattGmasClientMsgHandler(void **gash)
{
    CsrUint16 eventClass = 0;
    void *message = NULL;
    GattGmasClient *inst = *((GattGmasClient **)gash);

    if (CsrSchedMessageGet(&eventClass, &message))
    {
        switch (eventClass)
        {
            case CSR_BT_GATT_PRIM:
            {
                CsrBtGattPrim *id = (CsrBtGattPrim*)message;
                void *msg = NULL;
                GGMASC *gmasClient = (GGMASC *) GetServiceClientByGattMsg(&inst->serviceHandleList, message);

                msg = GetGattManagerMsgFromGattMsg(message, id);

                if (gmasClient)
                    gattGmasClientHandleGattMsg(gmasClient, *id, msg);

                if(message != msg)
                {
                    CsrPmemFree(msg);
                    msg = NULL;
                }
            }
            break;

            case GMAS_CLIENT_PRIM:
            {
                GattGmasClientInternalMsg *id = (GattGmasClientInternalMsg *)message;
                GGMASC *gmasClient = (GGMASC *) GetServiceClientByServiceHandle(message);
                gattGmasClientHandleInternalMessage(gmasClient, *id, message);
            }
            break;

            default:
                GATT_GMAS_CLIENT_WARNING("GGMASC: Client Msg not handled [0x%x]\n", eventClass);
        }
        SynergyMessageFree(eventClass, message);
    }
}
