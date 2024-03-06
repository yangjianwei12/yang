/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #59 $
******************************************************************************/
#include "gatt_tmas_server_access.h"
#include "gatt_tmas_server_private.h"

/***************************************************************************
NAME
    gattTmasServerHandleRoleAccess

DESCRIPTION
    Deals with access of the HANDLE_ROLE handle.
*/
static void gattTmasServerHandleRoleAccess(GTMAS *const tmasServer,
                                           GATT_MANAGER_SERVER_ACCESS_IND_T *const accessInd)
{
    if (accessInd->flags & ATT_ACCESS_READ)
    {
        uint8* value = NULL;
        value = (uint8*) CsrPmemZalloc(GATT_TMAS_SERVER_ROLE_SIZE);

        value[0] = (tmasServer->data.role & 0xff);
        value[1] = ((tmasServer->data.role >> 8) & 0xff);

        gattTmasServerSendAccessRsp(tmasServer->gattId,
                                    accessInd->cid,
                                    accessInd->handle,
                                    CSR_BT_GATT_RESULT_SUCCESS,
                                    GATT_TMAS_SERVER_ROLE_SIZE,
                                    value);
    }
    else
    {
        gattTmasServerSendAccessErrorRsp(tmasServer->gattId,
                                         accessInd->cid,
                                         accessInd->handle,
                                         CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED);
    }
}

/***************************************************************************/
void gattTmasServerHandleAccessIndication(GTMAS *tmasServer,
                                          GATT_MANAGER_SERVER_ACCESS_IND_T *const accessInd)
{
    switch (accessInd->handle)
    {
        case HANDLE_ROLE:
        {
            GATT_TMAS_SERVER_DEBUG("HANDLE_ROLE\n");
            gattTmasServerHandleRoleAccess(tmasServer,
                                           accessInd);
        }
        break;

        default:
        {
            GATT_TMAS_SERVER_WARNING("Unhandled access indication handle:%d\n", accessInd->handle);
        }
        break;
    }
}
