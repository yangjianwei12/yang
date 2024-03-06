/******************************************************************************
 Copyright (c) 2020 - 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #59 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_gatt_private.h"

void CsrBtGattDispatchDm(GattMainInst *inst)
{
    CsrPrim type = *(CsrPrim *)inst->msg;
    switch(type)
    {
#ifdef EXCLUDE_CSR_BT_CM_MODULE
        case DM_ACL_CONN_HANDLE_IND:
        case DM_ACL_OPENED_IND:
        {
            DM_ACL_CONN_HANDLE_IND_T *prim = inst->msg;
            CsrBtGattConnElement *conn;
            if ((prim->status == HCI_SUCCESS) && (prim->flags & DM_ACL_FLAG_ULP))
            {
                l2ca_conflags_t flags = L2CA_CONFLAG_ENUM((prim->flags & DM_ACL_FLAG_INCOMING) ? 
                                                                L2CA_CONNECTION_LE_SLAVE_UNDIRECTED : 
                                                                L2CA_CONNECTION_LE_MASTER_DIRECTED);
                conn = CSR_BT_GATT_CONN_INST_ADD_LAST(inst->connInst);
                conn->peerAddr = prim->addrt;

                attlib_connect_req(CSR_BT_GATT_IFACEQUEUE,
                                   &prim->addrt,
                                   CSR_BT_GATT_GET_L2CA_METHOD(flags),
                                   flags,
                                   NULL);
            }
        }
        break;

#if !defined(CSR_BT_GATT_INSTALL_FLAT_DB) && !defined (CSR_BT_GATT_EXCLUDE_MANDATORY_DB_REGISTRATION)
        case DM_HCI_ULP_READ_LOCAL_SUPPORTED_FEATURES_CFM:
        {
            DM_HCI_ULP_READ_LOCAL_SUPPORTED_FEATURES_CFM_T *prim = inst->msg;
            CsrUint8* pLocalLeFeatures = NULL;
            CsrBtGattDb *dbEntry;
            if (prim->status == HCI_SUCCESS)
            {
                pLocalLeFeatures = prim->feature_set;
            }
            dbEntry = CsrBtGattGetMandatoryDbEntry(inst, pLocalLeFeatures);
            attlib_add_req(CSR_BT_GATT_IFACEQUEUE, dbEntry, NULL);
        }
        break;
#endif
        case DM_SET_BT_VERSION_CFM:
        {
            /* This indicates Bluestack chip interrogation is complete */
#if defined(CSR_BT_GATT_INSTALL_FLAT_DB) || defined(CSR_BT_GATT_EXCLUDE_MANDATORY_DB_REGISTRATION)
            CsrBtConnId btConnId         = CSR_BT_GATT_LOCAL_BT_CONN_ID;
            CsrBtGattQueueElement *qElem = CSR_BT_GATT_QUEUE_FIND_BT_CONN_ID(inst->queue[0], &btConnId);

            /* This procedure is finish. Start the next if any */
            CsrBtGattQueueRestoreHandler(inst, qElem);
#else
            dm_hci_ulp_read_local_supported_features_req(NULL);
#endif
        }
        break;
#endif /* EXCLUDE_CSR_BT_CM_MODULE */
        case DM_SM_REGISTER_CFM:
            /* Ignore */
        break;
        default:
            CsrGeneralException(CsrBtGattLto,
                                0,
                                DM_PRIM,
                                type,
                                0,
                                "GATT handler - unknown DM primitive");
        break;
    }
    dm_free_upstream_primitive((DM_UPRIM_T*)inst->msg);
    inst->msg = NULL;
}
