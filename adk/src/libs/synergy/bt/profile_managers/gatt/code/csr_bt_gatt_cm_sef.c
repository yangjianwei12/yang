/******************************************************************************
 Copyright (c) 2010-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_gatt_private.h"
#include "csr_bt_cm_private_prim.h"


#ifdef CSR_BT_INSTALL_GATT_BREDR
static void csrBtGattCmL2caConnectAcceptCfmHandler(GattMainInst *inst,
                                                   CsrBtCmL2caConnectAcceptCfm *cfm)
{
    if (cfm->resultCode != CSR_BT_RESULT_CODE_CM_SUCCESS &&
        cfm->resultSupplier != CSR_BT_SUPPLIER_CM)
    { /* GATT need to inform the registered application
        that the CSR_BT_GATT_ACCEPT_BREDR_REQ procedure had failed.
        Note this will only occur if the CM cannot set the Controller
        in page scan mode */

        if(inst->bredrAppHandle != CSR_SCHED_QID_INVALID)
        {
            CsrBtGattStdCfmSend(CSR_BT_GATT_ACCEPT_BREDR_CFM,
                                inst->bredrAppHandle,
                                cfm->resultCode,
                                cfm->resultSupplier);
        }
    }
}
#endif

#ifndef CSR_BT_GATT_INSTALL_FLAT_DB
static void csrBtGattCmLocalNameChangeIndHandler(GattMainInst *inst)
{
    CsrUint8  *name;
    CsrUint16 length;
    CsrBtCmLocalNameChangeInd *prim = (CsrBtCmLocalNameChangeInd*) inst->msg;

    CsrPmemFree(inst->localName);

    if (prim->localName)
    {
        length = (CsrUint16)(CsrStrLen((CsrCharString*)prim->localName) + 1);
        inst->localName = prim->localName;
        prim->localName = NULL;
    }
    else
    { /* An empty local name must be present at all time */
        length          = (CsrUint16)(sizeof(CsrUtf8String));
        inst->localName = (CsrUtf8String *) CsrPmemZalloc(sizeof(CsrUtf8String));
    }
    
    name = (CsrUint8 *) CsrPmemAlloc(length);
    SynMemCpyS(name, length, inst->localName, length);
    CsrBtGattWriteLocalReqSend(inst->privateGattId,
                               HANDLE_DEVICE_NAME,
                               length,                  
                               name);
}
#endif /* !CSR_BT_GATT_INSTALL_FLAT_DB */

static void csrBtGattCmLeReadLocalSupportedFeaturesCfmHandler(GattMainInst *inst)
{
    CsrBtCmLeReadLocalSupportedFeaturesCfm *prim = inst->msg;

    if ((prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS) &&
        (prim->resultSupplier == CSR_BT_SUPPLIER_CM))
    {
        /* Keep the local supported features bits value stored */
        SynMemCpyS(inst->localLeFeatures,
                   sizeof(inst->localLeFeatures), 
                   prim->localLeFeatures,
                   sizeof(inst->localLeFeatures));
    }

#if !defined (CSR_BT_GATT_INSTALL_FLAT_DB) && !defined (CSR_BT_GATT_EXCLUDE_MANDATORY_DB_REGISTRATION)
    {
        CsrUint8* pLocalLeFetures = NULL;
        CsrBtGattDb *dbEntry;

        if ((prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS) &&
            (prim->resultSupplier == CSR_BT_SUPPLIER_CM))
        {
            pLocalLeFetures = prim->localLeFeatures;
        }

        dbEntry = CsrBtGattGetMandatoryDbEntry(inst, pLocalLeFetures);
        attlib_add_req(CSR_BT_GATT_IFACEQUEUE, dbEntry, NULL);
    }
#else
    /* Request to get some LE conntroler informations */
    CsrBtCmLeGetControllerInfoReqSend(CSR_BT_GATT_IFACEQUEUE);
#endif

}

static void csrBtGattCmLeEventConnectionIndHandler(GattMainInst *inst,
                                                   CsrBtCmLeEventConnectionInd *ind)
{
    CsrBtGattConnElement *conn = CSR_BT_GATT_CONN_INST_FIND_FROM_ADDRESS_FLAGS_LE(inst->connInst,
                                                           CsrBtGattFindConnInstFromAddressFlagsLe,
                                                           &(ind->deviceAddr));

    if (ind->event == CSR_BT_CM_LE_MODE_ON)
    {
#if defined(CSR_BT_INSTALL_EXTENDED_SCANNING) ||                            \
    defined(CSR_BT_INSTALL_EXTENDED_ADVERTISING) ||                         \
    defined(CSR_BT_INSTALL_PERIODIC_SCANNING) ||                            \
    defined(CSR_BT_INSTALL_PERIODIC_ADVERTISING)
#ifdef CSR_BT_LE_ENABLE
        /* Note:- With Extended Advertising implementation in Bluestack, ATT_CONNECT_IND event
         * sent to Synergy GATT for every incoming and outgoing LE ACL connection created using
         * DM_ACL_OPEN_REQ 
         * Extended Advertising implementation is enabled or disabled in BLuestack at run time 
         * based on local controller supports Extended Advertising feature 
         * So, sends ATT_CONNECT_REQ to Bluestack only if local controller does not supports 
         * Extended Advertising feature. */
        if (!CSR_BT_LE_LOCAL_FEATURE_SUPPORTED(inst->localLeFeatures,
                                               LE_FEATURE_EXTENDED_ADVERTISING))
#endif /* CSR_BT_LE_ENABLE */
#endif /* CSR_BT_INSTALL_EXTENDED_SCANNING || CSR_BT_INSTALL_EXTENDED_ADVERTISING ||
        * CSR_BT_INSTALL_PERIODIC_SCANNING || CSR_BT_INSTALL_PERIODIC_ADVERTISING */
        {
            L2CA_CONNECTION_T method = ((ind->role & HCI_SLAVE) ? 
                                        L2CA_CONNECTION_LE_SLAVE_UNDIRECTED :
                                        L2CA_CONNECTION_LE_MASTER_DIRECTED);
            /* GATT connection element is created either on receiving CSR_BT_CM_LE_EVENT_CONNECTION_IND 
             * or ATT_CONNECT_IND. ATT_CONNECT_REQ is sent on receiving CSR_BT_CM_LE_EVENT_CONNECTION_IND 
             * to fetch cid details if ATT_CONNECT_IND is not yet received.  
             * CSR_BT_CM_LE_EVENT_CONNECTION_IND or ATT_CONNECT_CFM would be ignored if ATT_CONNECT_IND 
             * is already received. */
            if (!conn)
            {
                conn = CSR_BT_GATT_CONN_INST_ADD_LAST(inst->connInst);
                conn->peerAddr = ind->deviceAddr;
                /* Send ATT_CONNECT_REQ to fetch cid details for fixed ATT channel */
                attlib_connect_req(CSR_BT_GATT_IFACEQUEUE,
                               &ind->deviceAddr,
                               method,
                               L2CA_CONFLAG_ENUM(method),
                               NULL);
            }
        }
    }
    else if (conn && (ind->event == CSR_BT_CM_LE_MODE_OFF))
    {
        if (conn->cid == 0) {
            /*ATT_CONNECT_CFM was never received with success for this connection*/
            CSR_BT_GATT_CONN_INST_REMOVE(inst->connInst, conn);
        }
    }
}

#ifndef EXCLUDE_CSR_BT_CM_MODULE
#ifdef CSR_BT_GATT_INSTALL_EATT
static void csrBtGattEattConnectionHandler(CsrBtGattConnElement* conn)
{
    /* Allow EATT connection to initiate only when below conditions are satisfied:
       There is no previous EATT connection is established between the local and remote device.
       Remote device supports EATT feature. There is no current on going Eatt connect Request.
       If the above three criterias are satisified then only initiate EATT connetion */
    if ((CSR_BT_GATT_IS_SSF_ENABLED(conn->connFlags)) && conn->localInitiated == LOCAL_EATT_IDLE)
    {
        bool         mode = MODE_EATT;                             /* ATT mode for connection */
        CsrUint16    num_bearers = LOCAL_INITIATED_EATT_BEARER;    /* Number of EATT bearers requested */
        TP_BD_ADDR_T tp_bdaddr;
        tp_bdaddr.tp_type = LE_ACL;
        tp_bdaddr.addrt = conn->peerAddr;

        attlib_enhanced_connect_req(CSR_BT_GATT_IFACEQUEUE,
                                    &tp_bdaddr,
                                    conn->l2capFlags,
                                    mode,
                                    num_bearers,
                                    gattMainInstPtr->preferredEattMtu,
                                    INITIAL_CREDITS,
                                    ZERO_PRIORITY,
                                    NULL);

        conn->localInitiated = LOCAL_EATT_INITIATING;
    }
}
#endif

static void csrBtGattConnUpdateEncryptStatus(CsrCmnListElm_t *elem, void *value)
{
    CsrBtGattConnElement *conn  = (CsrBtGattConnElement *) elem;
    CsrBtGattEncryptIds *ids    = (CsrBtGattEncryptIds *)value;

    if (CSR_BT_GATT_CONN_IS_CONNECTED(conn->state) && CsrBtAddrEq(&(conn->peerAddr), &(ids->address)))
    {
        conn->encrypted = ids->encrypted;

#ifdef CSR_BT_GATT_INSTALL_EATT
        /* This will handle the reconnection scenario for the EATT connection */
        if ((conn->encrypted) && (CsrBtTdDbDeviceExists(conn->peerAddr.type, &conn->peerAddr.addr)))
        {
            csrBtGattEattConnectionHandler(conn);
        }
#endif /* CSR_BT_GATT_INSTALL_EATT */
    }
}

static void csrBtGattCmEncryptChangeIndHandler(GattMainInst *inst, CsrBtCmEncryptChangeInd *ind)
{
    if (ind->transportType == CSR_BT_TRANSPORT_LE)
    {
        CsrBtGattEncryptIds ids;

        ids.encrypted = (ind->encryptType == CSR_BT_CM_ENC_TYPE_NONE) ? FALSE : TRUE;
        ids.address.addr = ind->deviceAddr;
        ids.address.type = ind->deviceAddrType;
        CSR_BT_GATT_CONN_INST_ITERATE(inst->connInst, csrBtGattConnUpdateEncryptStatus, &ids);
    }
}
#endif /* !EXCLUDE_CSR_BT_CM_MODULE */

static void csrBtGattCmLeGetControllerInfoCfmHandler(GattMainInst *inst, CsrBtCmLeGetControllerInfoCfm *cfm)
{ /* GATT has read the Controller info as part of GATT init procedure. Last step is to Register an SDP record */
    CsrBtConnId btConnId               = CSR_BT_GATT_LOCAL_BT_CONN_ID;

    CsrBtGattQueueElement *qElem = CsrBtGattfindQueueElementbtConnId(inst, btConnId);

    if (qElem)
    {
        CSR_UNUSED(cfm);

        /* This procedure is finish. Start the next if any */
        CsrBtGattQueueRestoreHandler(inst, qElem);
    }
    else
    {
        CsrGeneralException(CsrBtGattLto,
                            0,
                            CSR_BT_CM_PRIM,
                            cfm->type,
                            0,
                            "No qElem");
    }
}

static void csrBtGattCmAddressMappedIndHandler(GattMainInst *inst, CsrBtCmLeAddressMappedInd *ind)
{
    CsrBtTypedAddr addr;
    CsrBtGattConnElement *conn;

    addr.addr = ind->randomAddr;
    addr.type = CSR_BT_ADDR_RANDOM;

    conn = CSR_BT_GATT_CONN_INST_FIND_FROM_ADDRESS(inst->connInst,
                                                   CsrBtGattFindConnectedConnInstFromAddress,
                                                   &addr);
    if (conn)
    {
        conn->idAddr = ind->idAddr;
    }
}


static void csrBtGattCmSecurityEventIndHandler(GattMainInst *inst, CsrBtCmSecurityEventInd *ind)
{
    if (ind->transportMask & CSR_BT_TRANSPORT_TYPE_FLAG_LE)
    {
        CsrBtGattConnElement *conn;

        conn = CSR_BT_GATT_CONN_INST_FIND_FROM_ADDRESS(inst->connInst,
                                                       CsrBtGattFindConnectedConnInstFromAddress,
                                                       &ind->addrt);
        if (conn)
        {
            switch (ind->event)
            {
                case CSR_BT_CM_SECURITY_EVENT_BOND:
                {
                    if (CsrBtTdDbDeviceExists(ind->addrt.type,
                                              &ind->addrt.addr))
                    {
                        CsrBtTdDbGattInfo info = { 0 };

                        CsrBtTdDbSetGattInfo(ind->addrt.type,
                                             &ind->addrt.addr,
                                             &info);
                    }

#ifdef CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT
                    if (CSR_BT_LE_LOCAL_FEATURE_SUPPORTED(inst->localLeFeatures,
                                                          CSR_BT_LE_FEATURE_LL_PRIVACY))
                    {
                        CsrBtGattReadByUuidPrivateHandler(inst,
                                                          CSR_BT_GATT_UUID_CENTRAL_ADDRESS_RESOLUTION_CHARAC,
                                                          conn->btConnId);
                    }
#endif
                    break;
                }

                default:
                    /* Ignore */
                    break;
            }
        }
    }
}

#ifdef CSR_BT_GATT_INSTALL_EATT
static void csrBtGattCmSimplePairingCompleteIndHandler(GattMainInst* inst, CsrBtCmSmSimplePairingCompleteInd* ind)
{
    CsrBtGattConnElement *conn = CSR_BT_GATT_CONN_INST_FIND_FROM_ADDRESS(inst->connInst,
                                                                         CsrBtGattFindConnInstFromAddressFlagsLe,
                                                                         &ind->tp_addrt.addrt);

    /* Check the pairing status and Transport type, EATT connection is allowed for LE transport only on
     * an encrytped link */
    if ((conn && conn->encrypted) && (ind->status == HCI_SUCCESS) && (ind->tp_addrt.tp_type == LE_ACL))
    {
        /* Initiate the EATT connection */
        csrBtGattEattConnectionHandler(conn);
    }
}
#endif

void CsrBtGattDispatchCm(GattMainInst *inst)
{
    CsrPrim type = *(CsrPrim *)inst->msg;
    switch(type)
    {
        case CSR_BT_CM_SET_EVENT_MASK_CFM:
        {
            /* Just ignore */
            break;
        }
        case CSR_BT_CM_BLUECORE_INITIALIZED_IND:
        {
            /* Register GATT to the EATT subsystem */
#ifdef CSR_BT_GATT_INSTALL_EATT
            attlib_enhanced_register_req(CSR_BT_GATT_IFACEQUEUE, 0, ATT_EATT_LE_TRANSPORT_SUPPORT , NULL);
#else
            attlib_register_req(CSR_BT_GATT_IFACEQUEUE, NULL);
#endif /* CSR_BT_GATT_INSTALL_EATT */
            CsrBtCmLeReadLocalSupportedFeaturesReqSend(CSR_BT_GATT_IFACEQUEUE);
            break;
        }
        case CSR_BT_CM_LE_READ_LOCAL_SUPPORTED_FEATURES_CFM:
        {
            csrBtGattCmLeReadLocalSupportedFeaturesCfmHandler(inst);
            break;
        }
#ifndef CSR_BT_GATT_INSTALL_FLAT_DB
        case CSR_BT_CM_LOCAL_NAME_CHANGE_IND:
        {
            csrBtGattCmLocalNameChangeIndHandler(inst);
            break;
        }
#endif /* !CSR_BT_GATT_INSTALL_FLAT_DB */
        case CSR_BT_CM_L2CA_CONNECT_ACCEPT_CFM:
        {
            /* This message return error if CM cannot enter page scan.
             Need to handle this */
#ifdef CSR_BT_INSTALL_GATT_BREDR
             csrBtGattCmL2caConnectAcceptCfmHandler(inst, inst->msg);
#endif
            break;
        }
        case CSR_BT_CM_LE_EVENT_CONNECTION_IND:
        {
            csrBtGattCmLeEventConnectionIndHandler(inst, inst->msg);
            break;
        }

        case CSR_BT_CM_LE_GET_CONTROLLER_INFO_CFM:
        {
            csrBtGattCmLeGetControllerInfoCfmHandler(inst, inst->msg);
            break;
        }
#ifndef EXCLUDE_CSR_BT_CM_MODULE
        case CSR_BT_CM_ENCRYPT_CHANGE_IND:
        {
            csrBtGattCmEncryptChangeIndHandler(inst, inst->msg);
            break;
        }
#ifdef CSR_BT_GATT_INSTALL_EATT
        case CSR_BT_CM_SM_SIMPLE_PAIRING_COMPLETE_IND:
        {
            csrBtGattCmSimplePairingCompleteIndHandler(inst, inst->msg);
            break;
        }
#endif
#endif
        case CSR_BT_CM_LE_ADDRESS_MAPPED_IND:
            csrBtGattCmAddressMappedIndHandler(inst,
                                               (CsrBtCmLeAddressMappedInd*) inst->msg);
            break;
        case CSR_BT_CM_SECURITY_EVENT_IND:
            csrBtGattCmSecurityEventIndHandler(inst,
                                               (CsrBtCmSecurityEventInd*) inst->msg);
            break;
        default:
        {
            CsrGeneralException(CsrBtGattLto,
                                0,
                                CSR_BT_CM_PRIM,
                                type,
                                0,
                                "GATT handler - unknown CM primitive");
            break;
        }
    }
}
