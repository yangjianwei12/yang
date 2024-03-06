/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "gatt_battery_server.h"
#include "gatt_battery_server_private.h"
#include "gatt_battery_server_debug.h"
#include "gatt_battery_server_msg_handler.h"
#include "gatt_battery_server_access.h"

void CsrBtBasHandler(void** gash);

CsrBool basServerInstFindByGattId(CsrCmnListElm_t *elem, void *value)
{
    BasInstanceListElm_t *element = (BasInstanceListElm_t *)elem;
    CsrBtGattId           gattId = *(CsrBtGattId *)value;

    return (element->gattId == gattId);
}

/* The list should be having element type as BasInstanceListElm_t only */
BasInstanceListElm_t *getBasServerInstanceByGattMsg(CsrCmnList_t *list, void *msg)
{
    typedef struct
    {
        CsrPrim type;
        CsrBtGattId gattid;
    } getGattId;

    getGattId *gattMsg = (getGattId *) msg;
    BasInstanceListElm_t* elem = NULL;

    /* Find bas server instance using gattId */
    elem = BAS_SERVER_FIND_INSTANCE_BY_GATTID(*list, gattMsg->gattid);

    return elem;
}


/****************************************************************************/
void batteryServerMsgHandler(void* task, MsgId id, Msg message)
{
    GBASS *basServer = (GBASS *)task;

    switch (id)
    {
        case CSR_BT_GATT_FLAT_DB_REGISTER_HANDLE_RANGE_CFM:
            handleBatteryServerRegisterHandleRangeCfm(basServer, (CsrBtGattFlatDbRegisterHandleRangeCfm *)message);
            break;
        case CSR_BT_GATT_DB_ACCESS_READ_IND:
            /* Read access to characteristic */
            handleBatteryServerReadAccess(basServer, (CsrBtGattDbAccessReadInd *)message);
            break;
        case CSR_BT_GATT_DB_ACCESS_WRITE_IND:
            /* Write access to characteristic */
            handleBatteryServerWriteAccess(basServer, (CsrBtGattDbAccessWriteInd *)message);
            break;
        case CSR_BT_GATT_EVENT_SEND_CFM:
            break;
        default:
            /* Unrecognised GATT message */
            GATT_BATTERY_SERVER_DEBUG_PANIC(("GATT Msg not handled\n"));
            break;
    }
}

/* Synergy Task message handler */
void CsrBtBasHandler(void** gash)
{
    uint16 eventClass = 0;
    void* msg = NULL;
    CsrBtGattPrim id;
    GBASS_T *basInst = (GBASS_T*)*gash;
    BasInstanceListElm_t *elemInst = NULL;

    if (CsrSchedMessageGet(&eventClass, &msg))
    {
        switch (eventClass)
        {
            case CSR_BT_GATT_PRIM:
            {
                id = *(CsrBtGattPrim*)msg;
                /* Get the BAS Server instance based on GATT ID of the message */
                elemInst = getBasServerInstanceByGattMsg(&basInst->instanceList, msg);
                GATT_BATTERY_SERVER_DEBUG_INFO(("GBASS: CsrBtBasHandler gattId 0x%x, MsgId 0x%x",elemInst->gattId, id));
                batteryServerMsgHandler((void*)elemInst->pBass, id, msg);
                break;
            }
            case BAS_SERVER_PRIM:
            default:
                break;
        }
        SynergyMessageFree(eventClass, msg);
    }
}



