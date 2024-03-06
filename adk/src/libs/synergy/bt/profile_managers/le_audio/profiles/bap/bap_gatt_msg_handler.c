/*******************************************************************************

Copyright (C) 2019-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include <stdio.h>
#include <string.h>
#include "bap_utils.h"
#include "bap_client_lib.h"
#include "bap_connection.h"
#include "bap_client_list_util_private.h"
#include "bap_gatt_msg_handler.h"
#include "bap_client_debug.h"
#include "l2cap_prim.h"
#include "csr_bt_gatt_lib.h"
#include "csr_pmem.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT

#define CSR_BT_GATT_UUID_ASCS             ((CsrBtUuid16) 0x184E)

static void bapGattRegisterCfmHandler(BAP * const bap,
                                      CsrBtGattRegisterCfm *cfm)
{
    if (cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS &&
            cfm->resultSupplier == CSR_BT_SUPPLIER_GATT)
    {
        bap->gattId = cfm->gattId;
    }
}


static void csrBtAscsGattConnectIndHandler(BAP * const bap,
                                           CsrBtGattConnectInd *ind)
{
    BapConnection *connection;

        if (ind->resultSupplier == CSR_BT_SUPPLIER_GATT &&
            ind->resultCode == CSR_BT_GATT_RESULT_SUCCESS)
        {
            if (bapClientFindConnectionByTypedBdAddr(bap, &(ind->address), &connection))
            {
                connection->cid = ind->btConnId;
            }

            CsrBtGattDiscoverPrimaryServicesBy16BitUuidReqSend(bap->gattId,
                                                               ind->btConnId,
                                                               CSR_BT_GATT_UUID_ASCS);
        }
        else
        {

        }
    }

static void csrBtAscsGattDiscoverPrimaryServiceHandler(BAP * const bap,
                                                       CsrBtGattDiscoverServicesInd *ind)
{
    BapConnection *connection = NULL;

    if (bapClientFindConnectionByCid(bap, ind->btConnId, &connection))
    {
        switch (CSR_BT_UUID_GET_16(ind->uuid))
        {
            case CSR_BT_GATT_UUID_ASCS:
            {
                GattAscsClientInitParams params;

                connection->ascs.ascsStartHandle = ind->btConnId;
                connection->ascs.ascsEndHandle = ind->endHandle;
                connection->ascs.srvcHndl = INVALID_SERVICE_HANDLE;
                connection->numService++;

                params.cid = ind->btConnId;
                params.startHandle = ind->startHandle;
                params.endHandle = ind->endHandle;

                GattAscsClientInit(CSR_BT_BAP_IFACEQUEUE, &params, NULL);
            }
            break;
            case CSR_BT_GATT_UUID_PACS:
            {
                GattPacsClientInitData params;

                connection->pacs.pacsStartHandle = ind->startHandle;
                connection->pacs.pacsEndHandle = ind->endHandle;
                connection->pacs.srvcHndl = INVALID_SERVICE_HANDLE;
                connection->numService++;

                params.cid = ind->btConnId;
                params.startHandle = connection->pacs.pacsStartHandle;
                params.endHandle = connection->pacs.pacsEndHandle;

                GattPacsClientInitReq(CSR_BT_BAP_IFACEQUEUE, &params, NULL);
            }
            break;
            default:
                 BAP_CLIENT_WARNING("(BAP) csrBtAscsGattDiscoverPrimaryServiceHandler default uuid %x \n\n", CSR_BT_UUID_GET_16(ind->uuid));
            break;

        }
    }
    else
    {
         BAP_CLIENT_ERROR("(BAP) csrBtAscsGattDiscoverPrimaryServiceHandler Conn not found cid %d \n\n", ind->btConnId);
    }
}

/* Common GATT message handler */
void bapGattMessageHandler(BAP * const bap,
                           uint16 primiveId,
                           void *primitive)
{
    CsrBtGattPrim *prim = primitive;

    switch (*prim)
    {
        case CSR_BT_GATT_REGISTER_CFM:
            bapGattRegisterCfmHandler(bap, primitive);
            break;
        case CSR_BT_GATT_CONNECT_IND:
            csrBtAscsGattConnectIndHandler(bap, primitive);
            break;
        case CSR_BT_GATT_DISCOVER_SERVICES_IND:
            csrBtAscsGattDiscoverPrimaryServiceHandler(bap, primitive);
            break;
        case CSR_BT_GATT_DISCOVER_SERVICES_CFM:
            break;

        default:
            /* status = FALSE; */
            break;
    }
    CSR_UNUSED(primiveId);
}
#endif

