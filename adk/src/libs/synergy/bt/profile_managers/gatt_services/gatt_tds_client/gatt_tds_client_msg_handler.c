/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */


#include "gatt_tds_client_private.h"
#include "gatt_tds_client_debug.h"
#include "gatt_tds_client.h"
#include "gatt_tds_client_discovery.h"
#include "gatt_tds_client_read.h"
#include "gatt_tds_client_write.h"
#include "gatt_tds_client_indication.h"
#include "csr_bt_gatt_client_util_lib.h"
#include "gatt_tds_client_init.h"
#include "gatt_tds_client_common_util.h"


/***************************************************************************/
static void tdsClientHandleWriteValueRespCfm(GTDSC *const tdsClient, const CsrBtGattWriteCfm *const writeCfm)
{
    if (tdsClient != NULL)
    {
        if (tdsClient->pendingCmd == TDS_CLIENT_PENDING_OP_WRITE_CCCD)
        {
            if( writeCfm->handle == tdsClient->handles.tdsControlPointCCCDHandle)
            {
                tdsClient->pendingCmd = TDS_CLIENT_PENDING_OP_NONE;
                tdsClientIndicationCfm(tdsClient, GATT_TDS_CLIENT_STATUS_SUCCESS,
                                        GATT_TDS_CLIENT_INDICATION_CFM);
            }
        }
        else
        {
            handleTdsWriteValueResp(tdsClient,
                                    writeCfm->handle,
                                    writeCfm->resultCode);
        }
    }
    else
    {
        GATT_TDS_CLIENT_PANIC("Null instance\n");
    }
}

static void tdsClientHandleReadValueRespCfm(GTDSC *const tdsClient, const CsrBtGattReadCfm *readCfm)
{
    handleTdsReadValueResp(tdsClient,
                           readCfm->handle,
                           readCfm->resultCode,
                           readCfm->valueLength,
                           readCfm->value);
}

static void tdsClientHandleIndication(GTDSC *const tdsClient, const CsrBtGattClientIndicationInd *ind)
{
    handleTdsClientIndication(tdsClient,
                                ind->valueHandle,
                                ind->valueLength,
                                ind->value);
}

/****************************************************************************/
static void handleGattMsg(void *task, MsgId id, Msg msg)
{
    GTDSC *gattTdsClient = (GTDSC *)task;
    
    switch (id)
    {
        case CSR_BT_GATT_DISCOVER_CHARAC_CFM:
        {
            handleDiscoverAllTdsCharacteristicsResp(gattTdsClient,
                                                     (const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *)msg);
        }
        break;

        case CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_CFM:
        {
            handleDiscoverAllTdsCharacteristicDescriptorsResp(gattTdsClient,
                                                               (const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *)msg);
        }
        break;

        case CSR_BT_GATT_WRITE_CFM:
        {
            /* Write/Indication Confirmation */
            tdsClientHandleWriteValueRespCfm(gattTdsClient,
                                             (const CsrBtGattWriteCfm*) msg);
        }
        break;

        case CSR_BT_GATT_READ_CFM:
        {
            tdsClientHandleReadValueRespCfm(gattTdsClient,
                                            (const CsrBtGattReadCfm *) msg);
        }
        break;

        case CSR_BT_GATT_CLIENT_INDICATION_IND:
        {
            tdsClientHandleIndication(gattTdsClient,
                                        (const CsrBtGattClientIndicationInd *) msg);
        }
        break;

        default:
        {
            /* Unrecognised GATT Manager message */
            GATT_TDS_CLIENT_WARNING("GTDSC: TDS Client GattMgr Msg not handled [0x%x]\n", id);
        }
        break;
    }
    CsrBtGattFreeUpstreamMessageContents(CSR_BT_GATT_PRIM, msg);
}

/***************************************************************************/
static void  handleTdsInternalMessage(void *task, MsgId id, Msg msg)
{
    GTDSC * tdsClient = (GTDSC *)task;

    GATT_TDS_CLIENT_INFO("Message id (%d)\n",id);

    if (tdsClient)
    {
        switch(id)
        {
            case TDS_CLIENT_INTERNAL_MSG_READ_REQ:
            {
                TdsClientInternalMsgRead* message = (TdsClientInternalMsgRead*) msg;

                tdsClientHandleInternalRead(tdsClient,
                                            message->charac);
            }
            break;

            case TDS_CLIENT_INTERNAL_MSG_INDICATION_REQ:
            {
                TdsClientInternalMsgIndicationReq* message = (TdsClientInternalMsgIndicationReq*) msg;

                tdsClientHandleInternalRegisterForIndication(tdsClient,
                                                               message->indicValue);
            }
            break;

            case TDS_CLIENT_INTERNAL_MSG_SET_TDS_POINT_REQ:
            {
                TdsClientInternalMsgSetDiscoveryControlPoint* message = (TdsClientInternalMsgSetDiscoveryControlPoint*) msg;

                tdsClientHandleInternalSetTdsControlPoint(tdsClient,
                                                            message->op,
                                                            message->sizeValue,
                                                            message->value);
            }
            break;

            default:
            {
                /* Internal unrecognised messages */
                GATT_TDS_CLIENT_WARNING("Unknown Message received from Internal To lib \n");
            }
            break;
        }
    }
}

/****************************************************************************/
void gattTdsClientMsgHandler(void **gash)
{
    CsrUint16 eventClass = 0;
    void *message = NULL;
    GattTdsClient *inst = *((GattTdsClient **)gash);

    if (CsrSchedMessageGet(&eventClass, &message))
    {
        switch (eventClass)
        {
            case CSR_BT_GATT_PRIM:
            {
                CsrBtGattPrim *id = message;
                GTDSC *tdsClient = (GTDSC *) GetServiceClientByGattMsg(&inst->serviceHandleList, message);
                void *msg = GetGattManagerMsgFromGattMsg(message, id);

                if (tdsClient)
                    handleGattMsg(tdsClient, *id, msg);

                if(msg!=message)
                    CsrPmemFree(msg);
            }
                break;
            case TDS_CLIENT_PRIM:
            {
                TdsClientInternalMsg *id = (TdsClientInternalMsg *)message;
                GTDSC *tdsClient = (GTDSC *) GetServiceClientByServiceHandle(message);
                handleTdsInternalMessage(tdsClient, *id, message);
            }
                break;
            default:
                GATT_TDS_CLIENT_WARNING("GTDSC: Client Msg not handled [0x%x]\n", eventClass);
        }
        SynergyMessageFree(eventClass, message);
    }
}

