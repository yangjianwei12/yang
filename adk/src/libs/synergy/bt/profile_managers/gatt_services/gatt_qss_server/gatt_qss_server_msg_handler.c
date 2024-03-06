/******************************************************************************
 Copyright (c) 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #4 $
******************************************************************************/

#include "gatt_qss_server.h"
#include "gatt_qss_server_private.h"
#include "gatt_qss_server_debug.h"
#include "gatt_qss_server_msg_handler.h"
#include "gatt_qss_server_access.h"

void gattQssMsgHandler(void* task, MsgId id, Msg msg)
{
    GQSSS *qss = (GQSSS *) task;
    CsrBtGattRegisterCfm* cfm;
    switch (id)
    {
        case CSR_BT_GATT_DB_ACCESS_READ_IND:
        {
            /* Read Access to charateristic */
            GATT_MANAGER_SERVER_ACCESS_IND_T* message = 
                (GATT_MANAGER_SERVER_ACCESS_IND_T*) CsrPmemAlloc(sizeof(GATT_MANAGER_SERVER_ACCESS_IND_T));
            
            GattQssPopulateReadAccessInd(message, ((CsrBtGattDbAccessReadInd*)msg));
            gattQssServerHandleReadAccessIndication(qss, (GATT_MANAGER_SERVER_ACCESS_IND_T*)message);
            break;
        }
        case CSR_BT_GATT_DB_ACCESS_WRITE_IND:
        {
            /* Write Access to charateristic */
            GATT_MANAGER_SERVER_ACCESS_IND_T* message = 
                (GATT_MANAGER_SERVER_ACCESS_IND_T*) CsrPmemAlloc(sizeof(GATT_MANAGER_SERVER_ACCESS_IND_T));

            GattQssPopulateWriteAccessInd(message, ((CsrBtGattDbAccessWriteInd*)msg));
            gattQssServerHandleWriteAccessIndication(qss, (GATT_MANAGER_SERVER_ACCESS_IND_T*)message);
            break;
        }
        case CSR_BT_GATT_REGISTER_CFM:
        {
            /* Gatt registration confirmation after initalisation of instance */
            cfm = (CsrBtGattRegisterCfm*)msg;
            if (cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS &&
                cfm->resultSupplier == CSR_BT_SUPPLIER_GATT)
            {
                qss->gattId = cfm->gattId;
                CsrBtGattFlatDbRegisterHandleRangeReqSend(qss->gattId,
                                                          qss->startHandle,
                                                          qss->endHandle);
            }
            break;
        }
        case CSR_BT_GATT_FLAT_DB_REGISTER_HANDLE_RANGE_CFM:
        {
            GATT_QSS_SERVER_DEBUG_INFO(("\n GQSSS: Received GATT FLAT DB handle range confirmation. \n"));
            break;
        }
        case CSR_BT_GATT_EVENT_SEND_CFM:
        {
            GATT_QSS_SERVER_DEBUG_INFO(("\n GQSSS: Received GATT event confirmation. \n"));
            break;
        }
        default:
        {
            /* Unrecognised GATT message */
            GATT_QSS_SERVER_ERROR(("\n GQSSS: Msg 0x%04 not handled \n",id));
        }
    } /* switch */

    CsrBtGattFreeUpstreamMessageContents(CSR_BT_GATT_PRIM, msg);
}

/* Synergy Task message handler */
void gattQssServerMsgHandler(void** gash)
{
    void* msg = NULL;
    GQSSS* gattQssServerInst = NULL;
    CsrBtGattPrim id;
    uint16 eventClass = 0;
    ServiceHandle srvHndl = 0;

    srvHndl = *((ServiceHandle* )*gash);
    gattQssServerInst = ServiceHandleGetInstanceData(srvHndl);

    if (CsrSchedMessageGet(&eventClass, &msg))
    {
        switch (eventClass)
        {
            case CSR_BT_GATT_PRIM:
            {
                if (gattQssServerInst) 
                {
                    id = *(CsrBtGattPrim*)msg;
                    gattQssMsgHandler((void*)gattQssServerInst, id, msg);
                }
                break;
            }

            case QSS_SERVER_PRIM:
            default:
                break;
        }
        SynergyMessageFree(eventClass, msg);
    }
}
