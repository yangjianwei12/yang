/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #57 $
******************************************************************************/


#include "gatt_tmas_client_private.h"
#include "gatt_tmas_client_debug.h"
#include "gatt_tmas_client.h"
#include "gatt_tmas_client_discovery.h"
#include "gatt_tmas_client_read.h"
#include "csr_bt_gatt_client_util_lib.h"
#include "gatt_tmas_client_init.h"
#include "gatt_tmas_client_common_util.h"

static void gattTmasClientHandleReadValueRespCfm(GTMASC *const tmasClient, const CsrBtGattReadCfm *readCfm)
{

    if (readCfm->handle == tmasClient->handles.roleHandle)
    {
        /* Send read GATT_TMAS_CLIENT_ROLE_CFM message to application */
        gattTmasClientHandleReadRoleValueResp(tmasClient,
                                              readCfm->resultCode,
                                              readCfm->valueLength,
                                              readCfm->value);
    }
}

/****************************************************************************/
static void gattTmasClientHandleGattMsg(void *task, MsgId id, Msg msg)
{
    GTMASC *gattTmasClient = (GTMASC *)task;
    
    switch (id)
    {
        case CSR_BT_GATT_DISCOVER_CHARAC_CFM:
        {
            gattTmasClientHandleDiscoverAllTmasCharacteristicsResp(gattTmasClient,
                                                                   (const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *)msg);
        }
        break;

        case CSR_BT_GATT_READ_CFM:
        {
            CsrBtGattReadCfm* message = (CsrBtGattReadCfm*)msg;
            gattTmasClientHandleReadValueRespCfm(gattTmasClient,
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
            GATT_TMAS_CLIENT_WARNING("GTMASC: TMA Client GattMgr Msg not handled [0x%x]\n", id);
        }
        break;
    }
}

/***************************************************************************/
static void  gattTmasClientHandleInternalMessage(void *task, MsgId id, Msg msg)
{
    GTMASC * tmasClient = (GTMASC *)task;

    GATT_TMAS_CLIENT_INFO("Message id (%d)\n",id);

    if (tmasClient)
    {
        switch(id)
        {
            case GATT_TMAS_CLIENT_INTERNAL_MSG_READ:
            {
                GattTmasClientInternalMsgRead* message = (GattTmasClientInternalMsgRead*) msg;

                gattTmasClientHandleInternalRead(tmasClient,
                                                 message->handle);
            }
            break;

            default:
            {
                /* Internal unrecognised messages */
                GATT_TMAS_CLIENT_WARNING("Unknown Message received from Internal To lib \n");
            }
            break;
        }
    }
}

/****************************************************************************/
void gattTmasClientMsgHandler(void **gash)
{
    CsrUint16 eventClass = 0;
    void *message = NULL;
    GattTmasClient *inst = *((GattTmasClient **)gash);

    if (CsrSchedMessageGet(&eventClass, &message))
    {
        switch (eventClass)
        {
            case CSR_BT_GATT_PRIM:
            {
                CsrBtGattPrim *id = (CsrBtGattPrim*)message;
                void *msg = NULL;
                GTMASC *tmasClient = (GTMASC *) GetServiceClientByGattMsg(&inst->serviceHandleList, message);

                msg = GetGattManagerMsgFromGattMsg(message, id);

                if (tmasClient)
                    gattTmasClientHandleGattMsg(tmasClient, *id, msg);

                if(message != msg)
                {
                    CsrPmemFree(msg);
                    msg = NULL;
                }
            }
            break;

            case TMAS_CLIENT_PRIM:
            {
                GattTmasClientInternalMsg *id = (GattTmasClientInternalMsg *)message;
                GTMASC *tmasClient = (GTMASC *) GetServiceClientByServiceHandle(message);
                gattTmasClientHandleInternalMessage(tmasClient, *id, message);
            }
            break;

            default:
                GATT_TMAS_CLIENT_WARNING("GTMASC: Client Msg not handled [0x%x]\n", eventClass);
        }
        SynergyMessageFree(eventClass, message);
    }
}
