/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #60 $
******************************************************************************/


#include "gatt_tmas_client.h"
#include "gatt_tmas_client_debug.h"
#include "gatt_tmas_client_private.h"
#include "gatt_tmas_client_read.h"
#include "gatt_tmas_client_common_util.h"

/***************************************************************************/
void gattTmasClientHandleInternalRead(const GTMASC * tmasClient, uint16 handle)
{
    if (handle != GATT_ATTR_HANDLE_INVALID)
    {
        CsrBtGattReadReqSend(tmasClient->srvcElem->gattId,
                             tmasClient->srvcElem->cid,
                             handle,
                             0);
    }
    else
    {
        GattTmasClientRoleCfm *message = CsrPmemZalloc(sizeof(*message));

        message->srvcHndl = tmasClient->srvcElem->service_handle;
        message->status = CSR_BT_GATT_RESULT_UNACCEPTABLE_PARAMETER;
        message->role = 0x00;

        GattTmasClientMessageSend(tmasClient->appTask, GATT_TMAS_CLIENT_ROLE_CFM, message);
    }
}

/****************************************************************************/
void gattTmasClientHandleReadRoleValueResp(GTMASC *tmasClient,
                                           status_t resultCode,
                                           uint16 valueLength,
                                           uint8 *value)
{
    GattTmasClientRoleCfm *message = CsrPmemZalloc(sizeof(*message));

    message->srvcHndl = tmasClient->srvcElem->service_handle;
    message->status = getTmasClientStatusFromGattStatus(resultCode);

    if(valueLength && value)
    {
        message->role = value[0];
        message->role = ((value[1] << 8) | message->role);
    }
    else
    {
        message->role = 0;
    }

    GattTmasClientMessageSend(tmasClient->appTask, GATT_TMAS_CLIENT_ROLE_CFM, message);
}

void GattTmasClientReadRoleReq(ServiceHandle clntHndl)
{
    GTMASC*tmasClient = ServiceHandleGetInstanceData(clntHndl);

    if (tmasClient)
    {
        GattTmasClientInternalMsgRead *message = CsrPmemZalloc(sizeof(*message));
        message->srvcHndl = tmasClient->srvcElem->service_handle;
        message->handle = tmasClient->handles.roleHandle;

        GattTmasClientMessageSend(tmasClient->libTask, GATT_TMAS_CLIENT_INTERNAL_MSG_READ, message);
    }
    else
    {
        GATT_TMAS_CLIENT_PANIC("Invalid Tmas instance!\n");
    }
}
