/* Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd. */
/* %%version */


#include "gatt_mcs_client_private.h"
#include "gatt_mcs_client_debug.h"
#include "gatt_mcs_client.h"
#include "gatt_mcs_client_discovery.h"
#include "gatt_mcs_client_read.h"
#include "gatt_mcs_client_write.h"
#include "gatt_mcs_client_notification.h"
#include "csr_bt_gatt_client_util_lib.h"
#include "gatt_mcs_client_init.h"
#include "gatt_mcs_client_common_util.h"

/***************************************************************************/
static void mcsClientHandleWriteValueRespCfm(GMCSC *const mcsClient, const CsrBtGattWriteCfm *const writeCfm)
{
    if (mcsClient->pendingCmd == MCS_CLIENT_PENDING_OP_WRITE_CCCD)
    {
        MediaPlayerAttribute charac;

        charac = getMcsCharacFromCccHandle(mcsClient, writeCfm->handle);
        mcsClient->writeCccCount--;

        mcsClientNotifInd(mcsClient, charac, writeCfm->resultCode, GATT_MCS_CLIENT_NTF_IND);

        if (!mcsClient->writeCccCount)
        {
            mcsClient->pendingCmd = MCS_CLIENT_PENDING_OP_NONE;
            mcsClientNotifCfm(mcsClient, GATT_MCS_CLIENT_STATUS_SUCCESS, GATT_MCS_CLIENT_NTF_CFM);
        }
    }
    else
    {
        handleMcsWriteValueResp(mcsClient,
                                writeCfm->handle,
                                writeCfm->resultCode);
    }
}

static void mcsClientHandleReadValueRespCfm(GMCSC *const mcsClient, const CsrBtGattReadCfm *readCfm)
{
    handleMcsReadValueResp(mcsClient,
                           readCfm->handle,
                           readCfm->resultCode,
                           readCfm->valueLength,
                           readCfm->value);
}

static void mcsClientHandleNotification(GMCSC *const mcsClient, const CsrBtGattClientNotificationInd *ind)
{
    handleMcsClientNotification(mcsClient,
                                ind->valueHandle,
                                ind->valueLength,
                                ind->value);
}

/****************************************************************************/
static void handleGattMsg(void *task, MsgId id, Msg msg)
{
    GMCSC *gattClient = (GMCSC *)task;
    
    if (gattClient)
    {
        switch (id)
        {
            case CSR_BT_GATT_DISCOVER_CHARAC_CFM:
            {
                handleDiscoverAllMcsCharacteristicsResp(gattClient,
                                                        (const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *)msg);
            }
            break;

            case CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_CFM:
            {
                handleDiscoverAllMcsCharacteristicDescriptorsResp(gattClient,
                                                                  (const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *)msg);
            }
            break;

            case CSR_BT_GATT_WRITE_CFM:
            {
                /* Write/Notification Confirmation */
                mcsClientHandleWriteValueRespCfm(gattClient,
                                                 (const CsrBtGattWriteCfm*) msg);
            }
            break;

            case CSR_BT_GATT_READ_CFM:
            {
                mcsClientHandleReadValueRespCfm(gattClient,
                                                (const CsrBtGattReadCfm *) msg);
            }
            break;

            case CSR_BT_GATT_CLIENT_NOTIFICATION_IND:
            {
                mcsClientHandleNotification(gattClient,
                                            (const CsrBtGattClientNotificationInd *) msg);
            }
            break;

            default:
            {
                /* Unrecognised GATT Manager message */
                GATT_MCS_CLIENT_WARNING("GMCSC: MCS Client GattMgr Msg not handled [0x%x]\n", id);
            }
            break;
        }
    }
    CsrBtGattFreeUpstreamMessageContents(CSR_BT_GATT_PRIM, msg);
}

/***************************************************************************/
static void  handleMcsInternalMessage(void *task, MsgId id, Msg msg)
{
    GMCSC * mcsClient = (GMCSC *)task;

    GATT_MCS_CLIENT_INFO("Message id (%d)\n",id);

    if (mcsClient)
    {
        switch(id)
        {
            case MCS_CLIENT_INTERNAL_MSG_READ_REQ:
            {
                McsClientInternalMsgRead* message = (McsClientInternalMsgRead*) msg;

                mcsClientHandleInternalRead(mcsClient,
                                            message->charac);
            }
            break;

            case MCS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ:
            {
                McsClientInternalMsgNotificationReq* message = (McsClientInternalMsgNotificationReq*) msg;

                mcsClientHandleInternalRegisterForNotification(mcsClient,
                                                               message->characType,
                                                               message->notifValue);
            }
            break;

            case MCS_CLIENT_INTERNAL_MSG_WRITE_REQ:
            {
                McsClientInternalMsgWrite* message = (McsClientInternalMsgWrite*) msg;

                mcsClientHandleInternalWrite(mcsClient,
                                             message->charac,
                                             message->sizeValue,
                                             message->value);
            }
            break;

            case MCS_CLIENT_INTERNAL_MSG_SET_MEDIA_CONTROL_POINT_REQ:
            {
                McsClientInternalMsgSetMediaControlPoint* message = (McsClientInternalMsgSetMediaControlPoint*) msg;

                mcsClientHandleInternalSetMediaControlPoint(mcsClient,
                                                            message->op,
                                                            message->val);
            }
            break;

            default:
            {
                /* Internal unrecognised messages */
                GATT_MCS_CLIENT_WARNING("Unknown Message received from Internal To lib \n");
            }
            break;
        }
    }
}

/****************************************************************************/
void gattMcsClientMsgHandler(void **gash)
{
    CsrUint16 eventClass = 0;
    void *message = NULL;
    GattMcsClient *inst = *((GattMcsClient **)gash);

    if (CsrSchedMessageGet(&eventClass, &message))
    {
        switch (eventClass)
        {
            case CSR_BT_GATT_PRIM:
            {
                CsrBtGattPrim *id = message;
                GMCSC *mcsClient = (GMCSC *) GetServiceClientByGattMsg(&inst->serviceHandleList, message);
                void *msg = GetGattManagerMsgFromGattMsg(message, id);

                handleGattMsg(mcsClient, *id, msg);
                if(msg!=message)
                    CsrPmemFree(msg);
            }
                break;
            case MCS_CLIENT_PRIM:
            {
                McsClientInternalMsg *id = (McsClientInternalMsg *)message;
                GMCSC *mcsClient = (GMCSC *) GetServiceClientByServiceHandle(message);
                handleMcsInternalMessage(mcsClient, *id, message);
            }
                break;
            default:
                GATT_MCS_CLIENT_WARNING("GMCSC: Client Msg not handled [0x%x]\n", eventClass);
        }
        SynergyMessageFree(eventClass, message);
    }
}
