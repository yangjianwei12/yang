/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_bt_gatt_private.h"
#include "csr_bt_addr.h"

#ifdef CSR_BT_GATT_CACHING


static void CsrBtGattAddRobustCaching(CsrBtTypedAddr *addrt, CsrUint16 changeAware, PHYSICAL_TRANSPORT_T tp_type)
{
    TP_BD_ADDR_T tp_addrt;

    tp_addrt.tp_type = tp_type;
    tp_addrt.addrt = *addrt;

    attlib_add_robust_caching_req(CSR_BT_GATT_IFACEQUEUE,
                                  0,
                                  &tp_addrt,
                                  changeAware,
                                  NULL);
}

static void CsrBtGattConnInstDbChanged(CsrCmnListElm_t *elem, void *value)
{
    CsrBtGattConnElement *conn = (CsrBtGattConnElement *)elem;
    GattMainInst *inst = (GattMainInst *)value;

    if (CSR_BT_GATT_CONN_IS_CONNECTED(conn->state) &&
        (CSR_BT_GATT_IS_SERVICE_CHANGE_ENABLED(conn) || 
         CSR_BT_GATT_IS_ROBUST_CACHING_ENABLED(conn->connFlags)))
    {
        CSR_BT_GATT_SET_CHANGE_UNAWARE(conn->connFlags);

        if (CSR_BT_GATT_IS_DB_MODIFICATION(inst) && 
            CSR_BT_GATT_IS_SERVICE_CHANGE_ENABLED(conn))
        {
            /* DB change completed, initiate Service change */
            CsrBtGattServiceChangedIndicationSend(inst, conn,
                                                  CSR_BT_GATT_ATTR_HANDLE_START,
                                                  CSR_BT_GATT_ATTR_HANDLE_MAX);
        }
    }
}

static void CsrBtGattCachingHandler(GattMainInst *inst, 
                                    ATT_READ_BY_TYPE_CFM_T *prim)
{
    CsrUint16 count;
    CsrBool dbChanged = FALSE;
    CsrUint8 storedHash[CSR_BT_DB_HASH_SIZE];

    CsrMemSet(storedHash, 0, CSR_BT_DB_HASH_SIZE);

    (void)CsrBtGattCachingDbHashRead(&storedHash[0]);

    if (CsrMemCmp(storedHash, prim->value, CSR_BT_DB_HASH_SIZE) != 0)
    {
        /* Save new database hash into PS. */
        (void)CsrBtGattCachingDbHashWrite(prim->value);

        dbChanged = TRUE;
    }

    if ((count = CsrBtTdDbCountDevices()) != 0)
    {
        CsrUint8 i;
        CsrBtResultCode result = CSR_BT_RESULT_CODE_TD_DB_SUCCESS;

        for (i=0; i < count; i++)
        {
            CsrBtTdDbGattInfo info = { 0 };
            CsrBtTypedAddr addr = { 0 };
            CsrBool robustCaching, changeAware, serviceChange;

            result = CsrBtTdDbGetEntryByIndex(i,
                                             CSR_BT_TD_DB_SOURCE_GATT,
                                             CSR_BT_TD_DB_GATT_KEY_GATT_INFO,
                                             sizeof(CsrBtTdDbGattInfo),
                                             &info,
                                             &addr);

            if(result == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
            {
                robustCaching = CSR_BT_GATT_IS_ROBUST_CACHING_ENABLED(info.gattSuppFeatInfo);
                changeAware   = CSR_BT_GATT_IS_CHANGE_AWARE(info.gattSuppFeatInfo);
                serviceChange = CSR_MASK_IS_SET(info.gattSuppFeatInfo, CSR_BT_GATT_PS_SERVICE_CHANGE_MASK);
    
                if (dbChanged && (robustCaching || serviceChange))
                {
                  /* DB has changed, mark every trusted device as change-unaware */
                    (void)CsrBtGattTdDbWriteFeatureInfo(&addr, CSR_BT_GATT_SET_CHANGE_UNAWARE(info.gattSuppFeatInfo));
                    changeAware = FALSE;
                }
    
                if (!CSR_BT_GATT_IS_DB_MODIFICATION(inst) && robustCaching)
                {
                    /* DB initialisation scenario:
                     * Remote device has enabled Robust caching. Populate Bluestack
                     * with change aware info*/
                    CsrBtGattAddRobustCaching(&addr, changeAware, LE_ACL);
                }
            }
        }
    }

    if (CSR_BT_GATT_IS_DB_MODIFICATION(inst) && dbChanged)
    {
        /* Trigger GATT caching procedure for DB modification */
        CsrCmnListIterate(&(inst->connInst), CsrBtGattConnInstDbChanged, inst);
    }
    else
    {
        CSR_BT_GATT_SET_DB_INITIALISED(inst);
    }
}

void CsrBtGattDbHashHandler(GattMainInst *inst, ATT_READ_BY_TYPE_CFM_T *prim)
{
    CsrBtConnId btConnId = CSR_BT_GATT_LOCAL_BT_CONN_ID;
    CsrBtGattQueueElement *qElem = CsrBtGattfindQueueElementbtConnId(inst, btConnId);

    CsrBtGattPrim type = *(CsrBtGattPrim *) qElem->gattMsg;

    if ((prim->result == ATT_RESULT_SUCCESS) &&
        (prim->size_value == CSR_BT_DB_HASH_SIZE))
    {
        CsrBtGattCachingHandler(inst, prim);
    }
    else
    {
        /* This can happen if application doesn't include database hash in DB */
        CsrBtGattException(prim->type, "ATT_READ_BY_TYPE_CFM failure");
    }

#ifdef CSR_BT_GATT_INSTALL_FLAT_DB
    if (type == CSR_BT_GATT_FLAT_DB_REGISTER_REQ)
    {
        CsrBtGattFlatDbRegisterReq *reqMsg = (CsrBtGattFlatDbRegisterReq *) qElem->gattMsg;

        CsrBtGattFlatDbRegisterCfmSend(reqMsg->pHandle,
                                       prim->result,
                                       CSR_BT_SUPPLIER_ATT);
    }
#else 
    if (type == CSR_BT_GATT_DB_COMMIT_REQ)
    {
        /* Send result to the application */
        CsrBtGattStdCfmSend(CSR_BT_GATT_DB_COMMIT_CFM,
                            qElem->gattId,
                            prim->result,
                            CSR_BT_SUPPLIER_ATT);
    }
#endif

    /* This procedure is finish. Start the next if any */
    CsrBtGattQueueRestoreHandler(inst, qElem);
}

static void CsrBtGattServiceChangedClientConfigHandler(GattMainInst *inst,
                                                       CsrBtGattConnElement *conn,
                                                       ATT_ACCESS_IND_T *prim)
{
    att_result_t attResult = ATT_RESULT_SUCCESS;
    CsrUint8  *value = NULL;
    CsrUint16 length = 0; 

    if (CSR_MASK_IS_SET(prim->flags, ATT_ACCESS_READ))
    {
        /* Read operation for Service Changed client config */
        CsrUint16 clientConfigValue = CSR_BT_GATT_IS_SERVICE_CHANGE_ENABLED(conn) ?
                                      CSR_BT_GATT_CLIENT_CHARAC_CONFIG_INDICATION :
                                      CSR_BT_GATT_CLIENT_CHARAC_CONFIG_DEFAULT;

        length = CSR_BT_GATT_CLIENT_CONFIG_VALUE_LENGTH;
        value  = (CsrUint8 *) CsrPmemAlloc(length);
        CSR_COPY_UINT16_TO_LITTLE_ENDIAN(clientConfigValue, value);
    }
    else if (prim->size_value == CSR_BT_GATT_CLIENT_CONFIG_VALUE_LENGTH)
    {
        /* Write operation for Service Changed client config */
        CsrUint16 clientConfigVal = CSR_GET_UINT16_FROM_LITTLE_ENDIAN(prim->value);
        /* The other transport conn instance with the same address */
        CsrBtGattConnElement *conn1 = NULL;

        if (conn->leConnection)
        {
            conn1 = CSR_BT_GATT_CONN_INST_FIND_FROM_ADDRESS(inst->connInst,
                                                            CsrBtGattFindConnInstFromAddressFlagsBredr,
                                                            &(conn->peerAddr));
        }
        else
        {
            conn1 = CSR_BT_GATT_CONN_INST_FIND_FROM_ADDRESS(inst->connInst,
                                                            CsrBtGattFindConnInstFromAddressFlagsLe,
                                                            &(conn->peerAddr));
        }
        /* conn and conn1 are basically two different instance for same remote device,
         * one for LE transport and other for BR/EDR
         * and both needs to be in sync in terms of database */
        if (clientConfigVal == CSR_BT_GATT_CLIENT_CHARAC_CONFIG_DEFAULT)
        {
            CSR_BT_GATT_CLEAR_SERVICE_CHANGE(conn);

            if (conn1)
            {
                CSR_BT_GATT_CLEAR_SERVICE_CHANGE(conn1);
            }
        }
        else if (clientConfigVal == CSR_BT_GATT_CLIENT_CHARAC_CONFIG_INDICATION)
        {
            CSR_BT_GATT_SET_SERVICE_CHANGE(conn);

            if (conn1)
            {
                CSR_BT_GATT_SET_SERVICE_CHANGE(conn1);
            }
        }
        else
        {
            attResult = CSR_BT_GATT_ACCESS_RES_CLIENT_CONFIG_IMPROPERLY_CONF;
        }
    }
    else
    {
        /* Write operation for Service Changed client config - invalid length*/
        attResult = CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH;
    }

    attlib_access_rsp(CSR_BT_GATT_IFACEQUEUE, prim->cid, 
                      prim->handle, attResult, length, value, NULL);
}

void CsrBtGattCachingAccessIndHandler(GattMainInst *inst, ATT_ACCESS_IND_T *prim)
{
    CsrBtGattConnElement *conn = 
            CSR_BT_GATT_CONN_INST_FIND_CONNECTED_CID(inst->connInst, &(prim->cid));
    att_result_t attResult = ATT_RESULT_SUCCESS;

    if (conn == NULL)
    {
        attResult = CSR_BT_GATT_ACCESS_RES_UNLIKELY_ERROR;
    }
    else if (prim->handle == HANDLE_GATT_SERVICE_CHANGED_CLIENT_CONFIG)
    {
        CsrBtGattServiceChangedClientConfigHandler(inst, conn, prim);
    }
    else
    {
        attResult = CSR_BT_GATT_ACCESS_RES_UNLIKELY_ERROR;    
    }

    if (attResult != ATT_RESULT_SUCCESS)
    {
        attlib_access_rsp(CSR_BT_GATT_IFACEQUEUE, prim->cid,
                          prim->handle, attResult, 0, NULL, NULL);
    }
}

void CsrBtGattCachingNewConn(GattMainInst *inst, CsrBtGattConnElement *conn)
{
    /* Check for any potential GATT Caching actions when a new
     * GATT connection is established
     * Load GATT caching information into conn structure */
    conn->connFlags = 0;

    if (CsrBtGattTdDbReadFeatureInfo(&conn->peerAddr,
                           &conn->connFlags) == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
    {
        CsrBool changeAware = CSR_BT_GATT_IS_CHANGE_AWARE(conn->connFlags);

        /* Trusted device, reconnection scenario */
        if (CSR_BT_GATT_IS_ROBUST_CACHING_ENABLED(conn->connFlags))
        { /* Set transport type based on the connection info */
            PHYSICAL_TRANSPORT_T tp_type = (conn->leConnection) ? LE_ACL :  BREDR_ACL;

            CsrBtGattAddRobustCaching(&conn->peerAddr, changeAware, tp_type);
        }

        if (!changeAware && CSR_BT_GATT_IS_SERVICE_CHANGE_ENABLED(conn))
        {
            /* Trigger Service changed if the client is change unaware 
             * and has enabled indications */
            CsrBtGattServiceChangedIndicationSend(inst,
                                                  conn,
                                                  CSR_BT_GATT_ATTR_HANDLE_START,
                                                  CSR_BT_GATT_ATTR_HANDLE_MAX);
        }

        CSR_LOG_TEXT_INFO((CsrBtGattLto,
                           CSR_BT_GATT_LTSO_GENERAL,
                           "New conn PS Flags 0x%02X",
                           conn->connFlags));
    }
    else
    {
        /* Device is not available in PS */
        CSR_BT_GATT_SET_CHANGE_AWARE(conn->connFlags);
        CSR_BT_GATT_CLEAR_ROBUST_CACHING(conn->connFlags);
        CSR_BT_GATT_CLEAR_SERVICE_CHANGE(conn);
    }
}

void CsrBtGattAttAddRobustCachingCfmHandler(GattMainInst *inst)
{
    ATT_ADD_ROBUST_CACHING_CFM_T *prim = (ATT_ADD_ROBUST_CACHING_CFM_T *) inst->msg;

    if (prim->result != ATT_RESULT_SUCCESS)
    {
        CsrBtGattException(prim->type, "ATT_ADD_ROBUST_CACHING_CFM Failed");
    }
}

void CsrBtGattAttChangeAwareIndHandler(GattMainInst *inst)
{
    ATT_CHANGE_AWARE_IND_T *ind = (ATT_CHANGE_AWARE_IND_T *) inst->msg;
    CsrBtGattConnElement *conn = CSR_BT_GATT_CONN_INST_FIND_CID(inst->connInst, &ind->cid);

    if (conn)
    {
        CSR_BT_GATT_SET_CHANGE_AWARE(conn->connFlags);
    }
}

void CsrBtGattReadDbHashRestoreHandler(GattMainInst *inst,
                                       CsrBtGattQueueElement *element,
                                       CsrUint16 mtu)
{
    CsrUint32 dbHashUuid[4] = {CSR_BT_GATT_UUID_DATABASE_HASH_CHARAC, 0, 0, 0};

    CSR_UNUSED(inst);
    CSR_UNUSED(element);
    CSR_UNUSED(mtu);

    attlib_read_by_type_req(CSR_BT_GATT_IFACEQUEUE,
                            ATT_CID_LOCAL,
                            CSR_BT_GATT_ATTR_HANDLE_START,
                            CSR_BT_GATT_ATTR_HANDLE_END,
                            ATT_UUID16,
                            (uint32_t *) &dbHashUuid[0],
                            NULL);
}

#ifndef CSR_BT_GATT_INSTALL_FLAT_DB
CsrBool CsrBtGattDbCommitReqHandler(GattMainInst *inst)
{
    CsrBtGattDbCommitReq *prim = (CsrBtGattDbCommitReq*)inst->msg;
    CsrBool tmp = CsrBtGattNewReqHandler(inst,
                                         inst->msg, 
                                         CSR_BT_GATT_LOCAL_BT_CONN_ID, 
                                         prim->gattId,
                                         CsrBtGattReadDbHashRestoreHandler,
                                         NULL,
                                         NULL);
    inst->msg = NULL;
    return tmp;
}
#endif

#endif
