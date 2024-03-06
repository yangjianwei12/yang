/******************************************************************************
 Copyright (c) 2010-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_cm_le.h"
#ifdef CSR_STREAMS_ENABLE
#include "csr_bt_cm_streams_handler.h"
#endif

static CsrBool csrBtCmLeAdvertiseProcedure(cmInstanceData_t *cmInst, CsrBtCmLeAdvertiseReq *req);

/* Connection parameter update COEX handler */
static void csrBtCmLeParamUpdateCoex(cmInstanceData_t *cmInst,
                                     CsrBtTypedAddr *addr,
                                     CsrUint16 interval,
                                     CsrUint16 latency,
                                     CsrUint16 timeout,
                                     CsrUint8 status)
{
    leConnVar *conn;
    CsrUint32 var;

    /* Find cache entry */
    for (conn = cmInst->leVar.connCache;
         conn != NULL;
         conn = conn->next)
    {
        if (CsrBtAddrEq(&(conn->addr), addr))
        {
            /* Update connection params if they changed */
            if ((status == HCI_SUCCESS) &&
                ((conn->connParams.conn_interval != interval) ||
                 (conn->connParams.conn_latency != latency) ||
                 (conn->connParams.supervision_timeout != timeout)))
            {
                conn->connParams.conn_interval = interval;
                conn->connParams.conn_latency = latency;
                conn->connParams.supervision_timeout = timeout;
                /* accuracy is not modified */
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY
                /* Notify COEX update. Note that the LE cache contains the
                 * updated DM_ACL_CONN_HANDLE_IND with the params */
                var = CSR_BT_LE_EVENT_CONNECTION_UPDATE_COMPLETE | (status << 8);

                CsrBtCmPropgateEvent(cmInst,
                                     CsrBtCmPropagateLeConnectionEvent,
                                     CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY,
                                     HCI_SUCCESS,
                                     &var,
                                     conn);
#endif
            }
            break;
        }
    }
}

/* Send connection parameter update complete indication */
static void csrBtCmLeConnparamUpdateCmpIndSend(CsrSchedQid qid,
                                               CsrBtTypedAddr *address,
                                               CsrBtResultCode resultCode,
                                               CsrBtSupplier resultSupplier)
{
    CsrBtCmLeConnparamUpdateCmpInd *ind;
    ind = CsrPmemAlloc(sizeof(CsrBtCmLeConnparamUpdateCmpInd));
    ind->type = CSR_BT_CM_LE_CONNPARAM_UPDATE_CMP_IND;
    ind->address = *address;
    ind->resultCode = resultCode;
    ind->resultSupplier = resultSupplier;
    CsrBtCmPutMessage(qid, ind);
}                                               

/* Callback completion handler for connection update data request */
static void csrBtCmLeConnectionUpdateComplete(cmInstanceData_t *cmInst,
                                              struct cmCallbackObjTag *object,
                                              void *context,
                                              void *event)
{
    DM_BLE_UPDATE_CONNECTION_PARAMETERS_CFM_T *prim;
    CsrBtCmLeConnparamUpdateReq *ctx;

    prim = (DM_BLE_UPDATE_CONNECTION_PARAMETERS_CFM_T*)event;
    ctx = (CsrBtCmLeConnparamUpdateReq*)context;

    csrBtCmLeConnparamUpdateCmpIndSend(ctx->appHandle,
                                       &prim->addrt,
                                       (CsrBtResultCode)(prim->status == HCI_SUCCESS
                                                         ? CSR_BT_RESULT_CODE_CM_SUCCESS
                                                         : prim->status),
                                       (CsrBtSupplier)(prim->status == HCI_SUCCESS
                                                       ? CSR_BT_SUPPLIER_CM
                                                       : CSR_BT_SUPPLIER_HCI));
    CSR_UNUSED(cmInst);
    CSR_UNUSED(object);
}

/* Callback completion handler for connection update data request */
static void csrBtCmLeConnectionSetComplete(cmInstanceData_t *cmInst,
                                           struct cmCallbackObjTag *object,
                                           void *context,
                                           void *event)
{
    DM_SET_BLE_CONNECTION_PARAMETERS_CFM_T *prim;
    CsrBtCmLeConnparamReq *ctx;
    CsrBtCmLeConnparamCfm *cfm;

    prim = (DM_SET_BLE_CONNECTION_PARAMETERS_CFM_T*)event;
    ctx = (CsrBtCmLeConnparamReq*)context;
    cfm = (CsrBtCmLeConnparamCfm*)CsrPmemAlloc(sizeof(CsrBtCmLeConnparamCfm));   
    cfm->type = CSR_BT_CM_LE_CONNPARAM_CFM;
    
    if(prim->status == HCI_SUCCESS)
    {
        cfm->resultSupplier = CSR_BT_SUPPLIER_CM;
        cfm->resultCode = CSR_BT_RESULT_CODE_CM_SUCCESS;
    }
    else
    {
        cfm->resultSupplier = CSR_BT_SUPPLIER_HCI;
        cfm->resultCode = prim->status;
    }
    CsrBtCmPutMessage(ctx->appHandle, cfm);
    CSR_UNUSED(cmInst);
    CSR_UNUSED(object);
}

/* Handle scan request from app */
static void csrBtCmLeScanCfmSend(CsrSchedQid     qid,
                                 CsrUint16       context,
                                 CsrUint8        scanMode,
                                 CsrBool         whiteListEnable,
                                 CsrBtResultCode resultCode,
                                 CsrBtSupplier   resultSupplier)
{
    CsrBtCmLeScanCfm *cfm = (CsrBtCmLeScanCfm*)CsrPmemAlloc(sizeof(CsrBtCmLeScanCfm));
    cfm->type             = CSR_BT_CM_LE_SCAN_CFM;
    cfm->context          = context;   
    cfm->scanMode         = scanMode;  
    cfm->resultCode       = resultCode;
    cfm->resultSupplier   = resultSupplier;
    cfm->whiteListEnable  = whiteListEnable;
    CsrBtCmPutMessage(qid, cfm);
}

static void csrBtCmLeScanDisable(cmInstanceData_t        *cmInst,
                                 struct cmCallbackObjTag *object,
                                 void                    *context,
                                 void                    *event)
{ /* Cm has stopped the scan procedure. Send Cfm to the application.
     Do always consider this as a success. */
    CsrBtCmLeScanReq *req  = (CsrBtCmLeScanReq *) context;
    CsrUint32 var;
    
    csrBtCmLeScanCfmSend(req->appHandle,
                         req->context,
                         CSR_BT_CM_LE_MODE_OFF,
                         FALSE,
                         CSR_BT_RESULT_CODE_CM_SUCCESS,
                         CSR_BT_SUPPLIER_CM);
    
    if (cmInst->leVar.scanMode)
    {
        cmInst->leVar.scanMode = FALSE;
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY
        var = cmInst->leVar.scanMode;

        CsrBtCmPropgateEvent(cmInst,
                             CsrBtCmPropagateLeScanEvent,
                             CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY,
                             HCI_SUCCESS,
                             &var,
                             NULL);
#endif
    }
    CSR_UNUSED(object);
    CSR_UNUSED(event);
}

static void csrBtCmLeScanEnable(cmInstanceData_t        *cmInst,
                                struct cmCallbackObjTag *object,
                                void                    *context,
                                void                    *event)
{ /* Cm has (re)started the scan procedure. Send Cfm to the application */
    DM_HCI_ULP_SET_SCAN_ENABLE_CFM_T *prim = (DM_HCI_ULP_SET_SCAN_ENABLE_CFM_T * ) event;
    CsrBtCmLeScanReq *req     = (CsrBtCmLeScanReq *) context;
    CsrBtResultCode  result   = (prim->status == HCI_SUCCESS) ? CSR_BT_RESULT_CODE_CM_SUCCESS : prim->status;
    CsrBtSupplier    supplier = (prim->status == HCI_SUCCESS) ? CSR_BT_SUPPLIER_CM : CSR_BT_SUPPLIER_HCI;
    CsrBool          mode     = (prim->status == HCI_SUCCESS) ? TRUE : FALSE;
    CsrBool          wlEnable = (req->scanningFilterPolicy == HCI_ULP_SCAN_FP_ALLOW_WHITELIST) ? TRUE : FALSE;

    csrBtCmLeScanCfmSend(req->appHandle, req->context, mode, wlEnable, result, supplier);

    if (mode != cmInst->leVar.scanMode)
    {
        CsrUint32 var;

        cmInst->leVar.scanMode = mode;
        var = cmInst->leVar.scanMode;

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY
        CsrBtCmPropgateEvent(cmInst,
                             CsrBtCmPropagateLeScanEvent,
                             CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY,
                             HCI_SUCCESS,
                             &var,
                             NULL);
#endif
    }
    CSR_UNUSED(object);
}

static void csrBtCmLeScanParamsSet(cmInstanceData_t        *cmInst,
                                   struct cmCallbackObjTag *object,
                                   void                    *context,
                                   void                    *event)
{ /* Cm has setup new scan parameters. Restart the scan procedure */
    DM_HCI_ULP_SET_SCAN_PARAMETERS_CFM_T *prim = (DM_HCI_ULP_SET_SCAN_PARAMETERS_CFM_T * ) event;
    CsrBtCmLeScanReq *req = (CsrBtCmLeScanReq *) context;
    
    if (prim->status == HCI_SUCCESS)
    { /* The scan parameters has been updated, enable scan */
        DM_UPRIM_T *dmPrim;
      
        cmInst->leVar.params.scan.interval = req->scanInterval;
        cmInst->leVar.params.scan.window = req->scanWindow;
        cmInst->leVar.scanType = req->scanType;

        dm_hci_ulp_set_scan_enable_req(HCI_ULP_SCAN_ENABLED,
                                       req->filterDuplicates,
                                       &dmPrim);

        CsrBtCmCallbackSendSimpleBlockDm(cmInst,
                                         DM_HCI_ULP_SET_SCAN_ENABLE_CFM,
                                         req,
                                         dmPrim,
                                         csrBtCmLeScanEnable);
        object->context = NULL;
    }
    else
    { /* Fail to update the scan parameteres, return error */
        csrBtCmLeScanCfmSend(req->appHandle, req->context, CSR_BT_CM_LE_MODE_OFF, FALSE, prim->status, CSR_BT_SUPPLIER_HCI);
    }
}

static void csrBtCmLeScanWhitelistAdd(cmInstanceData_t        *cmInst,
                                      struct cmCallbackObjTag *object,
                                      void                    *context,
                                      void                    *event)
{ /* Cm has added a device to the white list. Check if other devices 
     need to be added */
    CsrUint16       rspType;
    DM_UPRIM_T      *dmPrim       = NULL;
    CmCallbackFct   *callBackFunc = NULL;
    DM_HCI_ULP_ADD_DEVICE_TO_WHITE_LIST_CFM_T *prim = (DM_HCI_ULP_ADD_DEVICE_TO_WHITE_LIST_CFM_T*) event;
    CsrBtCmLeScanReq *req = (CsrBtCmLeScanReq *) context;
    
    if (prim->status == HCI_SUCCESS && req->addressCount > 0)
    {
        rspType      = DM_HCI_ULP_ADD_DEVICE_TO_WHITE_LIST_CFM;
        callBackFunc = csrBtCmLeScanWhitelistAdd;
        req->addressCount--;
        dm_hci_ulp_add_device_to_white_list_req(&(req->addressList[req->addressCount]), &dmPrim);
    }
    else
    { 
        rspType      = DM_HCI_ULP_SET_SCAN_PARAMETERS_CFM;
        callBackFunc = csrBtCmLeScanParamsSet;
        if (prim->status != HCI_SUCCESS)
        { /* Just setup scanning without using the white list */
            req->scanningFilterPolicy = HCI_ULP_SCAN_FP_ALLOW_ALL;
        }
        dm_hci_ulp_set_scan_parameters_req(req->scanType,
                                           req->scanInterval,
                                           req->scanWindow,
                                           cmInst->leVar.ownAddressType,
                                           req->scanningFilterPolicy,
                                           &dmPrim);
    }
    CsrBtCmCallbackSendSimpleBlockDm(cmInst, rspType, req, dmPrim, callBackFunc);
    object->context = NULL;
}

static void csrBtCmLeScanWhitelistClear(cmInstanceData_t        *cmInst,
                                        struct cmCallbackObjTag *object,
                                        void                    *context,
                                        void                    *event)
{ /* Cm has clear the white list. Start adding new device to it */ 
    CsrUint16       rspType;
    DM_UPRIM_T      *dmPrim       = NULL;
    CmCallbackFct   *callBackFunc = NULL;
    CsrBtCmLeScanReq *req = (CsrBtCmLeScanReq *) context;
    DM_HCI_ULP_CLEAR_WHITE_LIST_CFM_T *prim = (DM_HCI_ULP_CLEAR_WHITE_LIST_CFM_T*) event;

    if (prim->status == HCI_SUCCESS && req->addressCount > 0)
    {
        rspType      = DM_HCI_ULP_ADD_DEVICE_TO_WHITE_LIST_CFM;  
        callBackFunc = csrBtCmLeScanWhitelistAdd;
        req->addressCount--;
        dm_hci_ulp_add_device_to_white_list_req(&(req->addressList[req->addressCount]), &dmPrim);
    }
    else
    { /* Just setup scanning without using the white list */
        rspType                   = DM_HCI_ULP_SET_SCAN_PARAMETERS_CFM;  
        callBackFunc              = csrBtCmLeScanParamsSet;
        req->scanningFilterPolicy = HCI_ULP_SCAN_FP_ALLOW_ALL;
        dm_hci_ulp_set_scan_parameters_req(req->scanType,
                                           req->scanInterval,
                                           req->scanWindow,
                                           cmInst->leVar.ownAddressType,
                                           req->scanningFilterPolicy,
                                           &dmPrim);
    }
    CsrBtCmCallbackSendSimpleBlockDm(cmInst, rspType, req, dmPrim, callBackFunc);
    object->context = NULL;
}


static void csrBtCmLeScanSetupParams(cmInstanceData_t        *cmInst,
                                     struct cmCallbackObjTag *object,
                                     void                    *context,
                                     void                    *event)
{ /* Cm has disable scan in order to set new scan parameters or to 
     modify the white list */
    CsrUint16       rspType;
    DM_UPRIM_T      *dmPrim       = NULL;
    CmCallbackFct   *callBackFunc = NULL;
    CsrBtCmLeScanReq *req         = (CsrBtCmLeScanReq *) context;

    if (req->addressCount > 0)
    { /* Set white list before scan params */ 
        rspType      = DM_HCI_ULP_CLEAR_WHITE_LIST_CFM;
        callBackFunc = csrBtCmLeScanWhitelistClear;
        dm_hci_ulp_clear_white_list_req(&dmPrim);
    }
    else
    {
        rspType      = DM_HCI_ULP_SET_SCAN_PARAMETERS_CFM;
        callBackFunc = csrBtCmLeScanParamsSet;
        dm_hci_ulp_set_scan_parameters_req(req->scanType,
                                           req->scanInterval,
                                           req->scanWindow,
                                           cmInst->leVar.ownAddressType,
                                           req->scanningFilterPolicy,
                                           &dmPrim);
    }
    CsrBtCmCallbackSendSimpleBlockDm(cmInst, rspType, req, dmPrim, callBackFunc);
    object->context = NULL;
    CSR_UNUSED(event);
}

void CsrBtCmLeScanReqHandler(cmInstanceData_t *cmInst)
{
    CsrBtCmLeScanReq *req = (CsrBtCmLeScanReq*) cmInst->recvMsgP;

    if (HCI_FEATURE_IS_SUPPORTED(LMP_FEATURES_LE_SUPPORTED_CONTROLLER_BIT, cmInst->dmVar.lmpSuppFeatures))
    {
        if(req->mode & CSR_BT_CM_LE_MODE_COEX_NOTIFY)
        { /* COEX notification from GATT which doing a Connection 
             attempt as Central */
            CsrBool mode = (req->mode & CSR_BT_CM_LE_MODE_ON) ? TRUE : FALSE;
            CsrUint32 var;

            if (cmInst->leVar.scanMode != mode)
            {
                cmInst->leVar.scanMode = mode;
                cmInst->leVar.scanType= CSR_BT_CM_LE_SCANTYPE_INITIATING;

                if (mode)
                {
                    cmInst->leVar.params.scan.interval = req->scanInterval;
                    cmInst->leVar.params.scan.window   = req->scanWindow;
                }
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY
                var = mode;

                CsrBtCmPropgateEvent(cmInst,
                                     CsrBtCmPropagateLeScanEvent,
                                     CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY,
                                     HCI_SUCCESS,
                                     &var,
                                     NULL);
#endif
            }
        }
        else
        { /* GATT want to enable or disable the scan procedure */
            CsrUint16       rspType       = 0;
            DM_UPRIM_T      *dmPrim       = NULL;
            CmCallbackFct   *callBackFunc = NULL;

            switch (req->mode)
            {
                case CSR_BT_CM_LE_MODE_OFF:
                { /* Turn off the scan procedure */
                    rspType      = DM_HCI_ULP_SET_SCAN_ENABLE_CFM;
                    callBackFunc = csrBtCmLeScanDisable;
                    dm_hci_ulp_set_scan_enable_req(HCI_ULP_SCAN_DISABLED,
                                                   HCI_ULP_FILTER_DUPLICATES_DISABLED,
                                                   &dmPrim);
                    break;
                }
                case CSR_BT_CM_LE_MODE_ON:
                { /* Set scan parameters and turn on the scan procedure */
                    if (req->addressCount > 0) 
                    { /* Set white list before scan params */ 
                        rspType      = DM_HCI_ULP_CLEAR_WHITE_LIST_CFM;
                        callBackFunc = csrBtCmLeScanWhitelistClear;
                        dm_hci_ulp_clear_white_list_req(&dmPrim);
                    }
                    else
                    {
                        rspType      = DM_HCI_ULP_SET_SCAN_PARAMETERS_CFM;
                        callBackFunc = csrBtCmLeScanParamsSet;
                        dm_hci_ulp_set_scan_parameters_req(req->scanType,
                                                           req->scanInterval,
                                                           req->scanWindow,
                                                           cmInst->leVar.ownAddressType,
                                                           req->scanningFilterPolicy,
                                                           &dmPrim);
                    }
                    break;
                }
                case CSR_BT_CM_LE_MODE_MODIFY:
                { /* Turn off the scan procedure, Set scan parameters 
                     and turn on the scan procedure on again */
                    rspType      = DM_HCI_ULP_SET_SCAN_ENABLE_CFM;
                    callBackFunc = csrBtCmLeScanSetupParams;
                    dm_hci_ulp_set_scan_enable_req(HCI_ULP_SCAN_DISABLED,
                                                   HCI_ULP_FILTER_DUPLICATES_DISABLED,
                                                   &dmPrim);
                    break;
                }
                default:
                { 
                    CsrGeneralException(CsrBtCmLto,
                                        0,
                                        CSR_BT_CM_PRIM,
                                        req->type,
                                        0,
                                        "Invalid Mode");
                    break;
                }
            }

            if (dmPrim)
            { /* Start the scanning procedure */
                CsrBtCmCallbackSendSimpleBlockDm(cmInst, rspType, req, dmPrim, callBackFunc);
                cmInst->recvMsgP = NULL;
            }
        }
    }
    else
    {
        if(!(req->mode & CSR_BT_CM_LE_MODE_COEX_NOTIFY))
        {
            csrBtCmLeScanCfmSend(req->appHandle, req->context, CSR_BT_CM_LE_MODE_OFF, FALSE,
                                 CSR_BT_RESULT_CODE_CM_UNSUPPORTED_FEATURE, CSR_BT_SUPPLIER_CM);
        }
    }
}

/* Handle advertise request from app */
static void csrBtCmLeAdvertiseCfmSend(CsrSchedQid       qid,
                                      CsrUint16         context,
                                      CsrBool           wlEnable,
                                      CsrUint8          advMode,
                                      CsrUint8          advType,
                                      CsrBtResultCode   resultCode,
                                      CsrBtSupplier     resultSupplier)
{
    CsrBtCmLeAdvertiseCfm *cfm = (CsrBtCmLeAdvertiseCfm*)CsrPmemAlloc(sizeof(CsrBtCmLeAdvertiseCfm));
    cfm->type                  = CSR_BT_CM_LE_ADVERTISE_CFM;
    cfm->context               = context;
    cfm->whiteListEnable       = wlEnable;
    cfm->advMode               = advMode;
    cfm->advType               = advType; 
    cfm->resultCode            = resultCode;
    cfm->resultSupplier        = resultSupplier;
    CsrBtCmPutMessage(qid, cfm);
}

static void csrBtCmLeAdvertiseSetScanResponseData(cmInstanceData_t        *cmInst,
                                                  struct cmCallbackObjTag *object,
                                                  void                    *context,
                                                  void                    *event)
{
    CsrBtCmLeAdvertiseReq *req                    = (CsrBtCmLeAdvertiseReq *) context;
    DM_HCI_ULP_SET_SCAN_RESPONSE_DATA_CFM_T *prim = (DM_HCI_ULP_SET_SCAN_RESPONSE_DATA_CFM_T *) event;

    if (prim->status == HCI_SUCCESS)
    {
        if (csrBtCmLeAdvertiseProcedure(cmInst, req))
        { /* Still setting params */
            object->context = NULL;
        }
        else 
        { /* All Done. Note Confirm message is sent by csrBtCmLeAdvertiseProcedure */
            ;
        }
    }
    else
    {
        csrBtCmLeAdvertiseCfmSend(req->appHandle,
                                  req->context,
                                  FALSE,
                                  (CsrUint8) (cmInst->leVar.advMode ? CSR_BT_CM_LE_MODE_ON : CSR_BT_CM_LE_MODE_OFF),
                                  cmInst->leVar.advType,
                                  prim->status,
                                  CSR_BT_SUPPLIER_HCI);
    }
}

static void csrBtCmLeAdvertiseSetAdvData(cmInstanceData_t        *cmInst,
                                         struct cmCallbackObjTag *object,
                                         void                    *context,
                                         void                    *event)
{
    CsrBtCmLeAdvertiseReq *req                  = (CsrBtCmLeAdvertiseReq *) context;
    DM_HCI_ULP_SET_ADVERTISING_DATA_CFM_T *prim = (DM_HCI_ULP_SET_ADVERTISING_DATA_CFM_T *) event;

    if (prim->status == HCI_SUCCESS)
    {
        if (csrBtCmLeAdvertiseProcedure(cmInst, req))
        { /* Still setting params */
            object->context = NULL;
        }
        else 
        { /* All Done. Note Confirm message is sent by csrBtCmLeAdvertiseProcedure */
            ;
        }
    }
    else
    {
        csrBtCmLeAdvertiseCfmSend(req->appHandle,
                                  req->context,
                                  FALSE,
                                  (CsrUint8) (cmInst->leVar.advMode ? CSR_BT_CM_LE_MODE_ON : CSR_BT_CM_LE_MODE_OFF),
                                  cmInst->leVar.advType,
                                  prim->status,
                                  CSR_BT_SUPPLIER_HCI);
    }
}

static void csrBtCmLeAdvertiseSetAdvParams(cmInstanceData_t        *cmInst,
                                           struct cmCallbackObjTag *object,
                                           void                    *context,
                                           void                    *event)
{
    CsrBtCmLeAdvertiseReq *req                        = (CsrBtCmLeAdvertiseReq *) context;
    DM_HCI_ULP_SET_ADVERTISING_PARAMETERS_CFM_T *prim = (DM_HCI_ULP_SET_ADVERTISING_PARAMETERS_CFM_T *) event;

    if (prim->status == HCI_SUCCESS)
    {
        cmInst->leVar.params.adv.intervalMin = req->advIntervalMin;
        cmInst->leVar.params.adv.intervalMax = req->advIntervalMax;
        cmInst->leVar.advType        = req->advertisingType;
        cmInst->leVar.advChannelMap  = req->advertisingChannelMap;
        
        if (csrBtCmLeAdvertiseProcedure(cmInst, req))
        { /* Still setting params */
            object->context = NULL;
        }
        else 
        { /* All Done. Note Confirm message is sent by csrBtCmLeAdvertiseProcedure */
            ;
        }
    }
    else
    {
        csrBtCmLeAdvertiseCfmSend(req->appHandle,
                                  req->context,
                                  FALSE,
                                  (CsrUint8) (cmInst->leVar.advMode ? CSR_BT_CM_LE_MODE_ON : CSR_BT_CM_LE_MODE_OFF),
                                  cmInst->leVar.advType,
                                  prim->status,
                                  CSR_BT_SUPPLIER_HCI);
    }
}

static void csrBtCmLeAdvertiseWhitelistAdd(cmInstanceData_t        *cmInst,
                                           struct cmCallbackObjTag *object,
                                           void                    *context,
                                           void                    *event)
{ /* Cm has added a device to the white list. Check if other devices 
     need to be added */
    CsrBtCmLeAdvertiseReq *req                      = (CsrBtCmLeAdvertiseReq *) context;
    DM_HCI_ULP_ADD_DEVICE_TO_WHITE_LIST_CFM_T *prim = (DM_HCI_ULP_ADD_DEVICE_TO_WHITE_LIST_CFM_T*) event;
    
    if (prim->status == HCI_SUCCESS)
    {
        if (req->whitelistAddrCount > 0)
        {
            DM_UPRIM_T *dmPrim;
            req->whitelistAddrCount--;
            dm_hci_ulp_add_device_to_white_list_req(&(req->whitelistAddrList[req->whitelistAddrCount]), &dmPrim);
            CsrBtCmCallbackSendSimpleBlockDm(cmInst, DM_HCI_ULP_ADD_DEVICE_TO_WHITE_LIST_CFM, req, dmPrim, csrBtCmLeAdvertiseWhitelistAdd);
            object->context = NULL;
        }
        else if (csrBtCmLeAdvertiseProcedure(cmInst, req))
        { /* Still setting params */
            object->context = NULL;
        }
        else 
        { /* All Done. Note Confirm message is sent by csrBtCmLeAdvertiseProcedure */
            ;
        }
    }
    else
    {
        csrBtCmLeAdvertiseCfmSend(req->appHandle,
                                  req->context,
                                  FALSE,
                                  (CsrUint8) (cmInst->leVar.advMode ? CSR_BT_CM_LE_MODE_ON : CSR_BT_CM_LE_MODE_OFF),
                                  cmInst->leVar.advType,
                                  prim->status,
                                  CSR_BT_SUPPLIER_HCI);
    }
}

static void csrBtCmLeAdvertiseWhitelistClear(cmInstanceData_t        *cmInst,
                                             struct cmCallbackObjTag *object,
                                             void                    *context,
                                             void                    *event)
{
    CsrBtCmLeAdvertiseReq *req                      = (CsrBtCmLeAdvertiseReq *) context;
    DM_HCI_ULP_CLEAR_WHITE_LIST_CFM_T *prim = (DM_HCI_ULP_CLEAR_WHITE_LIST_CFM_T*) event;

    if (prim->status == HCI_SUCCESS)
    { /* Add devices to the WhiteList */
        DM_UPRIM_T *dmPrim;
        req->whitelistAddrCount--;
        dm_hci_ulp_add_device_to_white_list_req(&(req->whitelistAddrList[req->whitelistAddrCount]), &dmPrim);
        CsrBtCmCallbackSendSimpleBlockDm(cmInst, DM_HCI_ULP_ADD_DEVICE_TO_WHITE_LIST_CFM, req, dmPrim, csrBtCmLeAdvertiseWhitelistAdd);
        object->context = NULL;
    }
    else 
    {
        csrBtCmLeAdvertiseCfmSend(req->appHandle,
                                  req->context,
                                  FALSE,
                                  (CsrUint8) (cmInst->leVar.advMode ? CSR_BT_CM_LE_MODE_ON : CSR_BT_CM_LE_MODE_OFF),
                                  cmInst->leVar.advType,
                                  prim->status,
                                  CSR_BT_SUPPLIER_HCI);
    }
}

static void csrBtCmLeAdvertiseDisable(cmInstanceData_t *cmInst,
                                      struct cmCallbackObjTag *object,
                                      void *context,
                                      void *event)
{ /* Cm has stopped the advertise procedure */
    CsrBtCmLeAdvertiseReq *req = (CsrBtCmLeAdvertiseReq *) context;
    CsrUint32 var;

    if (cmInst->leVar.advMode)
    {
        cmInst->leVar.advMode = FALSE;
        cmInst->leVar.advType = HCI_ULP_ADVERT_CONNECTABLE_UNDIRECTED;
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY
        var = cmInst->leVar.advMode;

        CsrBtCmPropgateEvent(cmInst,
                             CsrBtCmPropagateLeAdvertisingEvent,
                             CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY,
                             HCI_SUCCESS,
                             &var,
                             NULL);
#endif
    }

    if (csrBtCmLeAdvertiseProcedure(cmInst, req))
    { /* Still setting params */
        object->context = NULL;
    }
    CSR_UNUSED(event);
}

static void csrBtCmLeAdvertiseEnable(cmInstanceData_t        *cmInst,
                                     struct cmCallbackObjTag *object,
                                     void                    *context,
                                     void                    *event)
{ /* Cm has (re)started the advertise procedure. Send Cfm to the application */
    
    CsrBtCmLeAdvertiseReq *req                  = (CsrBtCmLeAdvertiseReq *) context;
    DM_HCI_ULP_SET_ADVERTISE_ENABLE_CFM_T *prim = (DM_HCI_ULP_SET_ADVERTISE_ENABLE_CFM_T * ) event;
    CsrBool          wlEnable = FALSE;
    CsrBtResultCode  result   = (prim->status == HCI_SUCCESS) ? CSR_BT_RESULT_CODE_CM_SUCCESS : prim->status;
    CsrBtSupplier    supplier = (prim->status == HCI_SUCCESS) ? CSR_BT_SUPPLIER_CM : CSR_BT_SUPPLIER_HCI;
    CsrBool          mode     = (prim->status == HCI_SUCCESS) ? TRUE : FALSE;
    CsrUint32 var;
    
    if (prim->status == HCI_SUCCESS)
    {
        wlEnable = (req->advertisingFilterPolicy == HCI_ULP_ADV_FP_ALLOW_ANY) ? FALSE : TRUE;
    }
    csrBtCmLeAdvertiseCfmSend(req->appHandle,
                              req->context,
                              wlEnable,
                              mode,
                              cmInst->leVar.advType,
                              result,
                              supplier);
    
    if (mode != cmInst->leVar.advMode)
    {
        cmInst->leVar.advMode = mode;
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY
        var = cmInst->leVar.advMode;

        CsrBtCmPropgateEvent(cmInst,
                             CsrBtCmPropagateLeAdvertisingEvent,
                             CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY,
                             HCI_SUCCESS,
                             &var,
                             NULL);
#endif
    }
    CSR_UNUSED(object);
}

static CsrBool csrBtCmLeAdvertiseProcedure(cmInstanceData_t *cmInst,
                                           CsrBtCmLeAdvertiseReq *req)
{ /* Drive LE advertise request */
    CsrUint16 rspType = 0;
    DM_UPRIM_T *dmPrim = NULL;
    CmCallbackFct *callBackFunc = NULL;

    if (req->whitelistAddrCount > 0)
    {
        rspType = DM_HCI_ULP_CLEAR_WHITE_LIST_CFM;
        callBackFunc = csrBtCmLeAdvertiseWhitelistClear;
        dm_hci_ulp_clear_white_list_req(&dmPrim);
    }
    else if (req->scanResponseDataLength > 0)
    {
        rspType = DM_HCI_ULP_SET_SCAN_RESPONSE_DATA_CFM;
        callBackFunc = csrBtCmLeAdvertiseSetScanResponseData;
        dm_hci_ulp_set_scan_response_data_req(req->scanResponseDataLength,
                                              req->scanResponseData,
                                              &dmPrim);
        req->scanResponseDataLength = 0;
    }
    else if (req->advertisingDataLength > 0)
    {
        rspType = DM_HCI_ULP_SET_ADVERTISING_DATA_CFM;
        callBackFunc = csrBtCmLeAdvertiseSetAdvData;
        dm_hci_ulp_set_advertising_data_req(req->advertisingDataLength,
                                            req->advertisingData,
                                            &dmPrim);
        req->advertisingDataLength = 0;
    }
    else if (CSR_MASK_IS_SET(req->paramChange, CSR_BT_CM_LE_PARCHG_PAR))
    {
        CsrBtTypedAddr address;
        CsrBtAddrZero(&(address));
        rspType = DM_HCI_ULP_SET_ADVERTISING_PARAMETERS_CFM;
        callBackFunc = csrBtCmLeAdvertiseSetAdvParams;
        dm_hci_ulp_set_advertising_parameters_req(req->advIntervalMin,
                                                  req->advIntervalMax,
                                                  req->advertisingType,
                                                  cmInst->leVar.ownAddressType,
                                                  &address,
                                                  req->advertisingChannelMap,
                                                  req->advertisingFilterPolicy,
                                                  &dmPrim);
        CSR_MASK_UNSET(req->paramChange, CSR_BT_CM_LE_PARCHG_PAR);
    }
    else if (CSR_MASK_IS_SET(req->paramChange, CSR_BT_CM_LE_PARCHG_DIRECT_ADV))
    {
        rspType = DM_HCI_ULP_SET_ADVERTISING_PARAMETERS_CFM;
        callBackFunc = csrBtCmLeAdvertiseSetAdvParams;
        dm_hci_ulp_set_advertising_parameters_req(req->advIntervalMin,
                                                  req->advIntervalMax,
                                                  req->advertisingType,
                                                  cmInst->leVar.ownAddressType,
                                                  &req->address,
                                                  req->advertisingChannelMap,
                                                  req->advertisingFilterPolicy,
                                                  &dmPrim);
        CSR_MASK_UNSET(req->paramChange, CSR_BT_CM_LE_PARCHG_DIRECT_ADV);
    }
    else if (req->mode == CSR_BT_CM_LE_MODE_MODIFY
             || req->mode == CSR_BT_CM_LE_MODE_ON)
    {
        rspType = DM_HCI_ULP_SET_ADVERTISE_ENABLE_CFM;
        callBackFunc = csrBtCmLeAdvertiseEnable;
        dm_hci_ulp_set_advertise_enable_req(HCI_ULP_ADVERTISING_ENABLED,
                                            &dmPrim);
    }
    else if (req->mode == CSR_BT_CM_LE_MODE_CONTINUE
             || req->mode == CSR_BT_CM_LE_MODE_OFF)
    {
        /* All Done, send confirm message to the application.
        Set "whiteListEnable" for CSR_BT_CM_LE_MODE_CONTINUE */
        CsrBool wlEnable;
        wlEnable = (req->advertisingFilterPolicy
                    == HCI_ULP_ADV_FP_ALLOW_ANY)
                   ? FALSE : TRUE;
        csrBtCmLeAdvertiseCfmSend(req->appHandle,
                                  req->context,
                                  wlEnable,
                                  (CsrUint8) (cmInst->leVar.advMode ? CSR_BT_CM_LE_MODE_ON : CSR_BT_CM_LE_MODE_OFF),
                                  cmInst->leVar.advType,
                                  CSR_BT_RESULT_CODE_CM_SUCCESS,
                                  CSR_BT_SUPPLIER_CM);
    }

    if (dmPrim)
    {
        CsrBtCmCallbackSendSimpleBlockDm(cmInst,
                                         rspType,
                                         req,
                                         dmPrim,
                                         callBackFunc);
        return TRUE;
    }
    return FALSE;
}

void CsrBtCmLeAdvertiseReqHandler(cmInstanceData_t *cmInst)
{
    CsrBtCmLeAdvertiseReq *req = (CsrBtCmLeAdvertiseReq*) cmInst->recvMsgP;

    if (HCI_FEATURE_IS_SUPPORTED(LMP_FEATURES_LE_SUPPORTED_CONTROLLER_BIT, cmInst->dmVar.lmpSuppFeatures))
    {
        if(req->mode & CSR_BT_CM_LE_MODE_COEX_NOTIFY)
        { /* COEX notification from GATT which doing a Connection 
             attempt as Peripheral */
            CsrBool mode = (req->mode & CSR_BT_CM_LE_MODE_ON) ? TRUE: FALSE;
            CsrUint32 var;

            if (cmInst->leVar.advMode != mode)
            {
                cmInst->leVar.advMode = mode;
                cmInst->leVar.advType = req->advertisingType;
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY
                var = mode;

                CsrBtCmPropgateEvent(cmInst,
                                     CsrBtCmPropagateLeAdvertisingEvent,
                                     CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY,
                                     HCI_SUCCESS,
                                     &var,
                                     NULL);
#endif
            }
        }
        else
        {
#ifdef CSR_BT_INSTALL_EXTENDED_ADVERTISING
            if (req->mode == CSR_BT_CM_LE_MODE_OFF)
            {
                cmInst->extAdvAppHandle[ADV_HANDLE_FOR_LEGACY_API] = CSR_SCHED_QID_INVALID;
            }
            else
            {
                /* If extended advertisements are supported, then even when application uses
                 * only legacy advertisement APIs, the application will still receive the unsolicited
                 * message DM_ULP_EXT_ADV_TERMINATED_IND_T on legacy advert termination due to connection.
                 * Now for legacy adverts, the event will always come with adv handle value as 0.
                 * Storing the APP handle at index 0(corresponding to adv handle value 0) so that the
                 * event can be propagted to the right APP handle.
                 * Please note that ADV handle 0 is reserved for legacy advertisements. No registration call to
                 * dm_ext_adv_register_app_adv_set_req is required for ADV handle 0.
                 */
                cmInst->extAdvAppHandle[ADV_HANDLE_FOR_LEGACY_API] = req->appHandle;
            }
#endif

            if (req->mode == CSR_BT_CM_LE_MODE_OFF || req->mode == CSR_BT_CM_LE_MODE_MODIFY)
            { /* Turn off the advertise procedure */
                DM_UPRIM_T *dmPrim;
                dm_hci_ulp_set_advertise_enable_req(HCI_ULP_ADVERTISING_DISABLED, &dmPrim);
                CsrBtCmCallbackSendSimpleBlockDm(cmInst, DM_HCI_ULP_SET_ADVERTISE_ENABLE_CFM, req, dmPrim, csrBtCmLeAdvertiseDisable);
                cmInst->recvMsgP = NULL;
            }
            else if (csrBtCmLeAdvertiseProcedure(cmInst, req))
            { /* Need to set some Adv Params */
                cmInst->recvMsgP = NULL;
            }
            else 
            { /* All Done. Note Confirm message is sent by csrBtCmLeAdvertiseProcedure */
                ;
            }
        }
    }
    else
    {
        if(!(req->mode & CSR_BT_CM_LE_MODE_COEX_NOTIFY))
        {
            csrBtCmLeAdvertiseCfmSend(req->appHandle, req->context, FALSE, CSR_BT_CM_LE_MODE_OFF, req->advertisingType, 
                                      CSR_BT_RESULT_CODE_CM_UNSUPPORTED_FEATURE, CSR_BT_SUPPLIER_CM);
        }
    }
}

/* Control LE whitelist */
static void csrBtCmLeWhitelistSetCfmSend(CsrSchedQid qid, CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    CsrBtCmLeWhitelistSetCfm *cfm = (CsrBtCmLeWhitelistSetCfm*)CsrPmemAlloc(sizeof(CsrBtCmLeWhitelistSetCfm));
    cfm->type           = CSR_BT_CM_LE_WHITELIST_SET_CFM;
    cfm->resultCode     = resultCode;
    cfm->resultSupplier = resultSupplier;
    CsrBtCmPutMessage(qid, cfm);
}

static void csrBtCmLeWhitelistSetAdd(cmInstanceData_t         *cmInst,
                                     struct cmCallbackObjTag *object,
                                     void                    *context,
                                     void                    *event)
{
    DM_HCI_ULP_ADD_DEVICE_TO_WHITE_LIST_CFM_T *prim = (DM_HCI_ULP_ADD_DEVICE_TO_WHITE_LIST_CFM_T*) event;
    CsrBtCmLeWhitelistSetReq *req = (CsrBtCmLeWhitelistSetReq *) context;
    
    if (prim->status == HCI_SUCCESS && req->addressCount > 0)
    {
        DM_UPRIM_T *dmPrim;
        req->addressCount--;
        dm_hci_ulp_add_device_to_white_list_req(&(req->addressList[req->addressCount]), &dmPrim);
        CsrBtCmCallbackSendSimpleBlockDm(cmInst,
                                         DM_HCI_ULP_ADD_DEVICE_TO_WHITE_LIST_CFM,
                                         req,
                                         dmPrim,
                                         csrBtCmLeWhitelistSetAdd);
        object->context = NULL;
    }
    else
    {
        csrBtCmLeWhitelistSetCfmSend(req->appHandle,
                                     (CsrBtResultCode)((prim->status == HCI_SUCCESS) ? CSR_BT_RESULT_CODE_CM_SUCCESS : prim->status),
                                     (CsrBtSupplier)(prim->status == HCI_SUCCESS ? CSR_BT_SUPPLIER_CM : CSR_BT_SUPPLIER_HCI));
    }
}

static void csrBtCmLeWhitelistSetClear(cmInstanceData_t         *cmInst,
                                       struct cmCallbackObjTag *object,
                                       void                    *context,
                                       void                    *event)
{ /* The whitelist is now cleared, start to add the new devices */
    CsrBtCmLeWhitelistSetReq *req = (CsrBtCmLeWhitelistSetReq *) context;
    DM_HCI_ULP_CLEAR_WHITE_LIST_CFM_T *prim = (DM_HCI_ULP_CLEAR_WHITE_LIST_CFM_T*) event;

    if (prim->status == HCI_SUCCESS && req->addressList)
    {
        DM_UPRIM_T *dmPrim;
        req->addressCount--;
        dm_hci_ulp_add_device_to_white_list_req(&(req->addressList[req->addressCount]), &dmPrim);
        CsrBtCmCallbackSendSimpleBlockDm(cmInst,
                                         DM_HCI_ULP_ADD_DEVICE_TO_WHITE_LIST_CFM,
                                         req,
                                         dmPrim,
                                         csrBtCmLeWhitelistSetAdd);
        object->context = NULL;
    }
    else
    { /* Queue system will free event and ctx */
        csrBtCmLeWhitelistSetCfmSend(req->appHandle,
                                     (CsrBtResultCode)((prim->status == HCI_SUCCESS) ? CSR_BT_RESULT_CODE_CM_SUCCESS : prim->status),
                                     (CsrBtSupplier)(prim->status == HCI_SUCCESS ? CSR_BT_SUPPLIER_CM : CSR_BT_SUPPLIER_HCI));
    }
}

void CsrBtCmLeWhitelistSetReqHandler(cmInstanceData_t *cmData)
{    
    CsrBtCmLeWhitelistSetReq *prim = (CsrBtCmLeWhitelistSetReq*)cmData->recvMsgP;
    
    if (HCI_FEATURE_IS_SUPPORTED(LMP_FEATURES_LE_SUPPORTED_CONTROLLER_BIT, cmData->dmVar.lmpSuppFeatures))
    {
        DM_UPRIM_T *dmPrim;
        dm_hci_ulp_clear_white_list_req(&dmPrim);
        CsrBtCmCallbackSendSimpleBlockDm(cmData,
                                         DM_HCI_ULP_CLEAR_WHITE_LIST_CFM,
                                         cmData->recvMsgP,
                                         dmPrim,
                                         csrBtCmLeWhitelistSetClear);
        cmData->recvMsgP = NULL;
    }
    else
    {
        csrBtCmLeWhitelistSetCfmSend(prim->appHandle, CSR_BT_RESULT_CODE_CM_UNSUPPORTED_FEATURE, CSR_BT_SUPPLIER_CM);
    }
}

void CsrBtCmLeConnparamReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmLeConnparamReq *req;
    DM_UPRIM_T *prim;
    req = (CsrBtCmLeConnparamReq*)cmData->recvMsgP;
    
    if (!HCI_FEATURE_IS_SUPPORTED(LMP_FEATURES_LE_SUPPORTED_CONTROLLER_BIT, cmData->dmVar.lmpSuppFeatures))
    {
        CsrBtCmLeConnparamCfm *cfm;
        cfm = (CsrBtCmLeConnparamCfm*)CsrPmemAlloc(sizeof(CsrBtCmLeConnparamCfm));
        cfm->type = CSR_BT_CM_LE_CONNPARAM_CFM;
        cfm->resultSupplier = CSR_BT_SUPPLIER_CM;
        cfm->resultCode = CSR_BT_RESULT_CODE_CM_UNSUPPORTED_FEATURE;
        CsrBtCmPutMessage(req->appHandle, cfm);
        return;
    }

    cmData->dmVar.connParams.scanInterval          = req->scanInterval;
    cmData->dmVar.connParams.scanWindow            = req->scanWindow;
    cmData->dmVar.connParams.connIntervalMin       = req->connIntervalMin;
    cmData->dmVar.connParams.connIntervalMax       = req->connIntervalMax;
    cmData->dmVar.connParams.connLatency           = req->connLatency;
    cmData->dmVar.connParams.supervisionTimeout    = req->supervisionTimeout;
    cmData->dmVar.connParams.connLatencyMax        = req->connLatencyMax;
    cmData->dmVar.connParams.supervisionTimeoutMin = req->supervisionTimeoutMin;
    cmData->dmVar.connParams.supervisionTimeoutMax = req->supervisionTimeoutMax;

    dm_set_ble_connection_parameters_req(req->scanInterval,
                                         req->scanWindow,
                                         req->connIntervalMin,
                                         req->connIntervalMax,
                                         req->connLatency,
                                         req->supervisionTimeout,
                                         CSR_BT_LE_DEFAULT_CONN_ATTEMPT_TIMEOUT,
                                         req->connLatencyMax,
                                         req->supervisionTimeoutMin,
                                         req->supervisionTimeoutMax,
                                         cmData->leVar.ownAddressType,
                                         &prim);
    CsrBtCmCallbackSendSimpleBlockDm(cmData,
                                     DM_SET_BLE_CONNECTION_PARAMETERS_CFM,
                                     req,
                                     prim,
                                     csrBtCmLeConnectionSetComplete);        
    cmData->recvMsgP = NULL;
}

void CsrBtCmLeConnparamUpdateReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmLeConnparamUpdateReq *req;
    DM_UPRIM_T *prim;

    req = (CsrBtCmLeConnparamUpdateReq*)cmData->recvMsgP;

    if (!HCI_FEATURE_IS_SUPPORTED(LMP_FEATURES_LE_SUPPORTED_CONTROLLER_BIT, cmData->dmVar.lmpSuppFeatures))
    {
        csrBtCmLeConnparamUpdateCmpIndSend(req->appHandle,
                                           &(req->address),
                                           CSR_BT_RESULT_CODE_CM_UNSUPPORTED_FEATURE,
                                           CSR_BT_SUPPLIER_CM);
        return;
    }

    /* Change par ameters for a connection */
    dm_ble_update_connection_update_req(&req->address,
                                        req->connIntervalMin,
                                        req->connIntervalMax,
                                        req->connLatency,
                                        req->supervisionTimeout,
                                        req->minimumCeLength,
                                        req->maximumCeLength,
                                        &prim);
    CsrBtCmCallbackSendSimpleBlockDm(cmData,
                                     DM_BLE_UPDATE_CONNECTION_PARAMETERS_CFM,
                                     req,
                                     prim,
                                     csrBtCmLeConnectionUpdateComplete);
    cmData->recvMsgP = NULL;
}

void CsrBtCmLeAcceptConnparamUpdateResHandler(cmInstanceData_t *cmData)
{
    CsrBtCmLeAcceptConnparamUpdateRes *req = 
                    (CsrBtCmLeAcceptConnparamUpdateRes*) cmData->recvMsgP;
    
    dm_ble_connection_par_update_rsp(req->l2caSignalId,
                                     req->address,
                                     req->connIntervalMin,
                                     req->connIntervalMax,
                                     req->connLatency,
                                     req->supervisionTimeout,
                                     (CsrUint16)(req->accept ? L2CAP_CONNECTION_PARAMETER_UPDATE_ACCEPT
                                                             : L2CAP_CONNECTION_PARAMETER_UPDATE_REJECT),
                                     NULL);
}

#ifdef INSTALL_CONTEXT_TRANSFER
void CsrBtCmLeAclOpenedIndExtHandler(DM_ACL_OPENED_IND_T *prim)
{
    /* Add connection to LE cache */
    leConnVar *conn = CsrPmemAlloc(sizeof(*conn));
    conn->addr = prim->addrt;
    conn->connParams = prim->ble_conn_params;
    conn->master = (prim->flags & DM_ACL_FLAG_INCOMING) ? FALSE: TRUE;
    conn->next = csrBtCmData.leVar.connCache;
    csrBtCmData.leVar.connCache = conn;
}
#endif /* #ifdef INSTALL_CONTEXT_TRANSFER */

void CsrBtCmLeAclOpenedIndHandler(cmInstanceData_t *cmData)
{
    DM_ACL_OPENED_IND_T *prim = (DM_ACL_OPENED_IND_T*) cmData->recvMsgP;
    CsrUint32 var;

    if (cmData->leVar.advMode == CSR_BT_CM_LE_MODE_ON)
    {
        cmData->leVar.advMode = CSR_BT_CM_LE_MODE_OFF;
        cmData->leVar.advType = HCI_ULP_ADVERT_CONNECTABLE_UNDIRECTED;
    }

    /* Connection on */
    if (prim->status == HCI_SUCCESS)
    {
        /* Add connection to LE cache */
        leConnVar *conn = CsrPmemAlloc(sizeof(*conn));
        conn->addr = prim->addrt;
        conn->connParams = prim->ble_conn_params;
        conn->master = (prim->flags & DM_ACL_FLAG_INCOMING) ? FALSE: TRUE;
#ifdef EXCLUDE_CSR_BT_SC_MODULE
        conn->bondRequired = TRUE;
#endif
        conn->next = cmData->leVar.connCache;
        cmData->leVar.connCache = conn;

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY
        var = CSR_BT_LE_EVENT_CONNECT_SUCCESS | (prim->status << 8);

        CsrBtCmPropgateEvent(cmData,
                             CsrBtCmPropagateLeConnectionEvent,
                             CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY,
                             prim->status,
                             &var,
                             conn);
#endif
    }
    else if (prim->status == HCI_ERROR_CONN_TERM_LOCAL_HOST)
    {
        leConnVar *conn;
        leConnVar **cppn;

        /* Remove connection from LE cache. Use double indirection */
        for (cppn = &(cmData->leVar.connCache); (conn = *cppn) != NULL; cppn = &(conn->next))
        {
            if (CsrBtAddrEq(&(conn->addr), &(prim->addrt)))
            {
                *cppn = conn->next;

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY
                var = CSR_BT_LE_EVENT_CONNECT_FAIL | (prim->status << 8);

                CsrBtCmPropgateEvent(cmData,
                                     CsrBtCmPropagateLeConnectionEvent,
                                     CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY,
                                     HCI_SUCCESS,
                                     &var,
                                     conn);
#endif
                CsrPmemFree(conn);
                return;
            }
        }
    }
    else
    {
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY
        leConnVar *conn = CsrPmemAlloc(sizeof(*conn));
        conn->addr = prim->addrt;
        conn->connParams = prim->ble_conn_params;
        conn->master = (prim->flags & DM_ACL_FLAG_INCOMING) ? FALSE: TRUE;

        var = CSR_BT_LE_EVENT_CONNECT_FAIL | (prim->status << 8);

        CsrBtCmPropgateEvent(cmData,
                             CsrBtCmPropagateLeConnectionEvent,
                             CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY,
                             prim->status,
                             &var,
                             conn);
        CsrPmemFree(conn);
#endif
    }
}

void CsrBtCmLeAcceptConnparamUpdateIndHandler(cmInstanceData_t *cmData)
{
    DM_BLE_ACCEPT_CONNECTION_PAR_UPDATE_IND_T * dmPrim;
    dmPrim = (DM_BLE_ACCEPT_CONNECTION_PAR_UPDATE_IND_T *) cmData->recvMsgP;

#ifdef CSR_BT_INSTALL_CM_HANDLE_REGISTER
    if (CSR_BT_CM_LE_HANDLE(cmData) != CSR_SCHED_QID_INVALID)
    {

#ifdef CSR_TARGET_PRODUCT_VM
        dm_ble_connection_par_update_rsp(dmPrim->signal_id,
                                         dmPrim->bd_addrt,
                                         dmPrim->conn_interval_min,
                                         dmPrim->conn_interval_max,
                                         dmPrim->conn_latency,
                                         dmPrim->supervision_timeout,
                                         L2CAP_CONNECTION_PARAMETER_UPDATE_ACCEPT,
                                         NULL);
#else /* CSR_TARGET_PRODUCT_VM */
        CsrBtCmLeAcceptConnparamUpdateInd *ind = (CsrBtCmLeAcceptConnparamUpdateInd*)
                                                  CsrPmemAlloc(sizeof(CsrBtCmLeAcceptConnparamUpdateInd));

        ind->type               = CSR_BT_CM_LE_ACCEPT_CONNPARAM_UPDATE_IND;
        ind->address            = dmPrim->bd_addrt; 
        ind->l2caSignalId       = dmPrim->signal_id;
        ind->connIntervalMin    = dmPrim->conn_interval_min;
        ind->connIntervalMax    = dmPrim->conn_interval_max;
        ind->connLatency        = dmPrim->conn_latency;
        ind->supervisionTimeout = dmPrim->supervision_timeout;
        CsrBtCmPutMessage(CSR_BT_CM_LE_HANDLE(cmData), ind);
#endif /* ! CSR_TARGET_PRODUCT_VM */

    }
    else
#endif /* CSR_BT_INSTALL_CM_HANDLE_REGISTER */
    {
        dm_ble_connection_par_update_rsp(dmPrim->signal_id,
                                         dmPrim->bd_addrt,
                                         dmPrim->conn_interval_min,
                                         dmPrim->conn_interval_max,
                                         dmPrim->conn_latency,
                                         dmPrim->supervision_timeout,
                                         L2CAP_CONNECTION_PARAMETER_UPDATE_REJECT,
                                         NULL);
    }
}

void CsrBtCmLeAclClosedIndHandler(cmInstanceData_t *cmData)
{
    DM_ACL_CLOSED_IND_T *prim = (DM_ACL_CLOSED_IND_T*)cmData->recvMsgP;
    leConnVar *conn;
    leConnVar **cppn;
    CsrUint32 var;

    /* Remove connection from LE cache. Use double indirection */
    for(cppn = &(cmData->leVar.connCache);
        (conn = *cppn) != NULL;
        cppn = &(conn->next))
    {
        if (CsrBtAddrEq(&(conn->addr), &(prim->addrt)))
        {
            *cppn = conn->next;
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY
            if (prim->reason == HCI_ERROR_CONNECTION_FAILED_TO_BE_ESTABLISHED)
            {
                var = CSR_BT_LE_EVENT_DISCONNECT_SYNC_TO | (prim->reason << 8);
            }
            else
            {
                var = CSR_BT_LE_EVENT_DISCONNECT | (prim->reason << 8);
            }

            CsrBtCmPropgateEvent(cmData,
                                 CsrBtCmPropagateLeConnectionEvent,
                                 CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY,
                                 HCI_SUCCESS,
                                 &var,
                                 conn);
#endif
            CsrPmemFree(conn);
            return;
        }
    }
}

/* LE advertise report handler */
void CsrBtCmLeReportIndHandler(cmInstanceData_t *cmInst,
                               DM_HCI_ULP_ADVERTISING_REPORT_IND_T *report)
{
#ifdef CSR_BT_INSTALL_CM_HANDLE_REGISTER
    if (CSR_BT_CM_LE_HANDLE(cmInst) != CSR_SCHED_QID_INVALID)
#endif
    {
        CsrBtCmLeReportInd *ind;
        CsrUint8 length;
        length = report->length_data;

        ind = (CsrBtCmLeReportInd*)CsrPmemAlloc(sizeof(CsrBtCmLeReportInd));
        ind->type = CSR_BT_CM_LE_REPORT_IND;
        ind->eventType = report->event_type;
        ind->address = report->current_addrt;
        ind->permanentAddress = report->permanent_addrt;
        ind->lengthData = length;
        ind->rssi = report->rssi;
        SynMemCpyS(ind->data, CSR_BT_CM_LE_MAX_REPORT_LENGTH, report->data, length);
        CsrMemSet(&ind->data[length], 0, CSR_BT_CM_LE_MAX_REPORT_LENGTH - length);
        CsrBtCmPutMessage(CSR_BT_CM_LE_HANDLE(cmInst), ind);
    }
}

void CsrBtCmLeConnectionUpdateCmpIndHandler(cmInstanceData_t *cmInst,
                                            DM_HCI_ULP_CONNECTION_UPDATE_COMPLETE_IND_T* dmPrim)
{
    /* COEX */
    csrBtCmLeParamUpdateCoex(cmInst,
                             &(dmPrim->addrt),
                             dmPrim->conn_interval,
                             dmPrim->conn_latency,
                             dmPrim->supervision_timeout,
                             dmPrim->status);
}

/* De-init code for LE instance data */
void CsrBtCmLeDeinit(cmInstanceData_t *cmData)
{
    while(cmData->leVar.connCache)
    {
        leConnVar *next = cmData->leVar.connCache->next;
        CsrPmemFree(cmData->leVar.connCache);
        cmData->leVar.connCache = next;
    }
}

void CsrBtCmLePhysicalLinkStatusReqHandler(cmInstanceData_t *cmData)
{
#if defined(INSTALL_CM_DEVICE_UTILITY) && defined(INSTALL_CM_INTERNAL_LPM)
    CsrBtCmLePhysicalLinkStatusReq * prim = (CsrBtCmLePhysicalLinkStatusReq *) cmData->recvMsgP;
    aclTable *aclConnectionElement = NULL;

    if (!prim->radioType &&
        (returnAclConnectionElement(cmData, CsrBtAddrAddr(prim->address), &aclConnectionElement) != CM_ERROR) &&
        (aclConnectionElement != NULL))
    {
        aclConnectionElement->gattConnectionActive = prim->status;
    }
#endif /* INSTALL_CM_DEVICE_UTILITY && INSTALL_CM_INTERNAL_LPM */
    CsrBtCmServiceManagerLocalQueueHandler(cmData);
}

static void csrBtCmLeReceiverTestComplete(cmInstanceData_t *cmInst,
                                          struct cmCallbackObjTag *object,
                                          void *context,
                                          void *event)
{
    DM_HCI_ULP_RECEIVER_TEST_CFM_T *prim;
    CsrBtCmLeTransmitterTestReq *ctx;
    CsrBtCmLeTransmitterTestCfm *cfm;
    
    prim = (DM_HCI_ULP_RECEIVER_TEST_CFM_T*)event;
    ctx = (CsrBtCmLeTransmitterTestReq*)context;
    cfm = (CsrBtCmLeTransmitterTestCfm*)CsrPmemAlloc(sizeof(CsrBtCmLeTransmitterTestCfm));

    cfm->type = CSR_BT_CM_LE_RECEIVER_TEST_CFM;
    cfm->resultCode = (CsrBtResultCode)(prim->status == HCI_SUCCESS
                                        ? CSR_BT_RESULT_CODE_CM_SUCCESS
                                        : prim->status);
    cfm->resultSupplier = (CsrBtSupplier)(prim->status == HCI_SUCCESS
                                          ? CSR_BT_SUPPLIER_CM
                                          : CSR_BT_SUPPLIER_HCI);
    CsrBtCmPutMessage(ctx->appHandle, cfm);
    CSR_UNUSED(cmInst);
    CSR_UNUSED(object);
}

void CsrBtCmLeReceiverTestReqHandler(cmInstanceData_t *cmInst)
{
    CsrBtCmLeReceiverTestReq *req;
    DM_UPRIM_T *prim;

    req = (CsrBtCmLeReceiverTestReq*)cmInst->recvMsgP;
    cmInst->recvMsgP = NULL;

    dm_hci_ulp_receiver_test_req(req->rxFrequency,
                                 &prim);

    CsrBtCmCallbackSendSimpleBlockDm(cmInst,
                                     DM_HCI_ULP_RECEIVER_TEST_CFM,
                                     req, /* ctx */
                                     prim,
                                     csrBtCmLeReceiverTestComplete);
}

static void csrBtCmLeTransmitterTestComplete(cmInstanceData_t *cmInst,
                                             struct cmCallbackObjTag *object,
                                             void *context,
                                             void *event)
{
    DM_HCI_ULP_TRANSMITTER_TEST_CFM_T *prim;
    CsrBtCmLeTransmitterTestReq *ctx;
    CsrBtCmLeTransmitterTestCfm *cfm;
    
    prim = (DM_HCI_ULP_TRANSMITTER_TEST_CFM_T*)event;
    ctx = (CsrBtCmLeTransmitterTestReq*)context;
    cfm = (CsrBtCmLeTransmitterTestCfm*)CsrPmemAlloc(sizeof(CsrBtCmLeTransmitterTestCfm));

    cfm->type = CSR_BT_CM_LE_TRANSMITTER_TEST_CFM;
    cfm->resultCode = (CsrBtResultCode)(prim->status == HCI_SUCCESS
                                        ? CSR_BT_RESULT_CODE_CM_SUCCESS
                                        : prim->status);
    cfm->resultSupplier = (CsrBtSupplier)(prim->status == HCI_SUCCESS
                                          ? CSR_BT_SUPPLIER_CM
                                          : CSR_BT_SUPPLIER_HCI);

    CsrBtCmPutMessage(ctx->appHandle, cfm);
    CSR_UNUSED(cmInst);
    CSR_UNUSED(object);
}

void CsrBtCmLeTransmitterTestReqHandler(cmInstanceData_t *cmInst)
{
    CsrBtCmLeTransmitterTestReq *req;
    DM_UPRIM_T *prim;

    req = (CsrBtCmLeTransmitterTestReq*)cmInst->recvMsgP;
    cmInst->recvMsgP = NULL;

    dm_hci_ulp_transmitter_test_req(req->txFrequency,
                                    req->lengthOfTestData,
                                    req->packetPayload,
                                    &prim);

    CsrBtCmCallbackSendSimpleBlockDm(cmInst,
                                     DM_HCI_ULP_TRANSMITTER_TEST_CFM,
                                     req, /* ctx */
                                     prim,
                                     csrBtCmLeTransmitterTestComplete);
}

static void csrBtCmLeTestEndComplete(cmInstanceData_t *cmInst,
                                     struct cmCallbackObjTag *object,
                                     void *context,
                                     void *event)
{
    DM_HCI_ULP_TEST_END_CFM_T *prim;
    CsrBtCmLeTestEndReq *ctx;
    CsrBtCmLeTestEndCfm *cfm;
    
    prim = (DM_HCI_ULP_TEST_END_CFM_T*)event;
    ctx = (CsrBtCmLeTestEndReq*)context;
    cfm = (CsrBtCmLeTestEndCfm*)CsrPmemAlloc(sizeof(CsrBtCmLeTestEndCfm));

    cfm->type = CSR_BT_CM_LE_TEST_END_CFM;
    cfm->numberOfPackets = prim->number_of_packets;
    cfm->resultCode = (CsrBtResultCode)(prim->status == HCI_SUCCESS
                                        ? CSR_BT_RESULT_CODE_CM_SUCCESS
                                        : prim->status);
    cfm->resultSupplier = (CsrBtSupplier)(prim->status == HCI_SUCCESS
                                          ? CSR_BT_SUPPLIER_CM
                                          : CSR_BT_SUPPLIER_HCI);

    CsrBtCmPutMessage(ctx->appHandle, cfm);
    CSR_UNUSED(cmInst);
    CSR_UNUSED(object);
}

void CsrBtCmLeTestEndReqHandler(cmInstanceData_t *cmInst)
{
    CsrBtCmLeTestEndReq *req;
    DM_UPRIM_T *prim;

    req = (CsrBtCmLeTestEndReq*)cmInst->recvMsgP;
    cmInst->recvMsgP = NULL;

    dm_hci_ulp_test_end_req(&prim);

    CsrBtCmCallbackSendSimpleBlockDm(cmInst,
                                     DM_HCI_ULP_TEST_END_CFM,
                                     req, /* ctx */
                                     prim,
                                     csrBtCmLeTestEndComplete);
}

static void CmLeEnhancedReceiverTestComplete(cmInstanceData_t *cmInst,
                                             struct cmCallbackObjTag *object,
                                             void *context,
                                             void *event)
{
    DM_HCI_ULP_ENHANCED_RECEIVER_TEST_CFM_T *prim;
    CmLeEnhancedReceiverTestReq *ctx;
    CmLeEnhancedReceiverTestCfm *cfm;
    
    prim = (DM_HCI_ULP_ENHANCED_RECEIVER_TEST_CFM_T*)event;
    ctx = (CmLeEnhancedReceiverTestReq*)context;
    cfm = (CmLeEnhancedReceiverTestCfm*)CsrPmemAlloc(sizeof(CmLeEnhancedReceiverTestCfm));

    cfm->type = CM_LE_ENHANCED_RECEIVER_TEST_CFM;
    cfm->resultCode = (CsrBtResultCode)(prim->status == HCI_SUCCESS
                                        ? CSR_BT_RESULT_CODE_CM_SUCCESS
                                        : prim->status);
    cfm->resultSupplier = (CsrBtSupplier)(prim->status == HCI_SUCCESS
                                          ? CSR_BT_SUPPLIER_CM
                                          : CSR_BT_SUPPLIER_HCI);
    CsrBtCmPutMessage(ctx->appHandle, cfm);
    CSR_UNUSED(cmInst);
    CSR_UNUSED(object);
}

void CmLeEnhancedReceiverTestReqHandler(cmInstanceData_t *cmInst)
{
    CmLeEnhancedReceiverTestReq *req;
    DM_UPRIM_T *prim;

    req = (CmLeEnhancedReceiverTestReq*)cmInst->recvMsgP;
    cmInst->recvMsgP = NULL;

    dm_hci_ulp_enhanced_receiver_test_req(req->rxFrequency,
                                          req->phy,
                                          req->modIndex,
                                          &prim);

    CsrBtCmCallbackSendSimpleBlockDm(cmInst,
                                     DM_HCI_ULP_ENHANCED_RECEIVER_TEST_CFM,
                                     req, /* ctx */
                                     prim,
                                     CmLeEnhancedReceiverTestComplete);
}


static void CmLeEnhancedTransmitterTestComplete(cmInstanceData_t *cmInst,
                                                struct cmCallbackObjTag *object,
                                                void *context,
                                                void *event)
{
    DM_HCI_ULP_ENHANCED_TRANSMITTER_TEST_CFM_T *prim;
    CmLeEnhancedTransmitterTestReq *ctx;
    CmLeEnhancedTransmitterTestCfm *cfm;
    
    prim = (DM_HCI_ULP_ENHANCED_TRANSMITTER_TEST_CFM_T*)event;
    ctx = (CmLeEnhancedTransmitterTestReq*)context;
    cfm = (CmLeEnhancedTransmitterTestCfm*)CsrPmemAlloc(sizeof(CmLeEnhancedTransmitterTestCfm));

    cfm->type = CM_LE_ENHANCED_TRANSMITTER_TEST_CFM;
    cfm->resultCode = (CsrBtResultCode)(prim->status == HCI_SUCCESS
                                        ? CSR_BT_RESULT_CODE_CM_SUCCESS
                                        : prim->status);
    cfm->resultSupplier = (CsrBtSupplier)(prim->status == HCI_SUCCESS
                                          ? CSR_BT_SUPPLIER_CM
                                          : CSR_BT_SUPPLIER_HCI);

    CsrBtCmPutMessage(ctx->appHandle, cfm);
    CSR_UNUSED(cmInst);
    CSR_UNUSED(object);
}


void CmLeEnhancedTransmitterTestReqHandler(cmInstanceData_t *cmInst)
{
    CmLeEnhancedTransmitterTestReq *req;
    DM_UPRIM_T *prim;

    req = (CmLeEnhancedTransmitterTestReq*)cmInst->recvMsgP;
    cmInst->recvMsgP = NULL;

    dm_hci_ulp_enhanced_transmitter_test_req(req->txFrequency,
                                             req->lengthOfTestData,
                                             req->packetPayload,
                                             req->phy,
                                             &prim);

    CsrBtCmCallbackSendSimpleBlockDm(cmInst,
                                     DM_HCI_ULP_ENHANCED_TRANSMITTER_TEST_CFM,
                                     req, /* ctx */
                                     prim,
                                     CmLeEnhancedTransmitterTestComplete);
}

void CsrBtCmLeLockSmQueueHandler(cmInstanceData_t *cmData)
{
    CsrBtCmLeLockSmQueueReq *prim = (CsrBtCmLeLockSmQueueReq *) cmData->recvMsgP;
    CsrBtCmLeLockSmQueueInd *ind  = (CsrBtCmLeLockSmQueueInd *) CsrPmemAlloc(sizeof(CsrBtCmLeLockSmQueueInd));
    ind->type                     = CSR_BT_CM_LE_LOCK_SM_QUEUE_IND;
    ind->address                  = prim->address;  
    /* Register Outgoing Security on an invalid PSM to MITM. This will ensure that GATT/ATT will raise security
       to MITM if both devices IO Capabilities allows it */ 
    dm_sm_service_register_outgoing_req(CSR_BT_CM_IFACEQUEUE,
                                        0,
                                        &(prim->address.addr),
                                        SEC_PROTOCOL_L2CAP,
                                        L2CA_PSM_INVALID,
                                        SECL4_OUT_LEVEL_3,
                                        CSR_BT_SC_DEFAULT_ENC_KEY_SIZE,
                                        NULL);
    CsrBtCmPutMessage(prim->appHandle, ind);
}

void CsrBtCmLeUnLockSmQueueHandler(cmInstanceData_t *cmData)
{
    CsrBtCmLeUnlockSmQueueReq *prim = (CsrBtCmLeUnlockSmQueueReq *) cmData->recvMsgP;

    if (cmData->smVar.smInProgress && cmData->smVar.smMsgTypeInProgress == CSR_BT_CM_LE_LOCK_SM_QUEUE_REQ)
    { /* The SM Queue is currently lock by GATT. This were done to ensure that
         we do not create two l2cap connection at the same time. Today l2cap
         cannnot handle this. Unlock it again */
       
        /* Unregister the invalid PSM which were register by CsrBtCmLeLockSmQueueHandler */
        dm_sm_unregister_outgoing_req(CSR_BT_CM_IFACEQUEUE, 0, &(prim->address.addr), SEC_PROTOCOL_L2CAP, L2CA_PSM_INVALID, NULL);        
        CsrBtCmServiceManagerLocalQueueHandler(cmData);
    }
    else
    { /* Ckeck if the CSR_BT_CM_LE_LOCK_SM_QUEUE_REQ message is placed on the local SM queue */
        CsrUint8        dummy;
        CsrUint16       context;
        (void)(cancelServiceManagerMsg(cmData, CSR_BT_CM_LE_LOCK_SM_QUEUE_REQ, CSR_SCHED_QID_INVALID, prim->address.addr, CSR_BT_NO_SERVER, L2CA_PSM_INVALID, &dummy, &context));
    }
}

/* Handle CsrBtCmLeGetControllerInfoReq from app */
static void csrBtCmLeGetControllerInfoCfmSend(CsrSchedQid       qid,
                                              CsrUint8          whiteListSize, 
                                              CsrUint32         leStatesUpper, 
                                              CsrUint32         leStatesLower,
                                              CsrBtResultCode   resultCode,
                                              CsrBtSupplier     resultSupplier)
{
    CsrBtCmLeGetControllerInfoCfm *cfm = (CsrBtCmLeGetControllerInfoCfm*)CsrPmemZalloc(sizeof(CsrBtCmLeGetControllerInfoCfm));
    cfm->type                          = CSR_BT_CM_LE_GET_CONTROLLER_INFO_CFM;
    cfm->whiteListSize                 = whiteListSize;
    cfm->leStatesUpper                 = leStatesUpper;
    cfm->leStatesLower                 = leStatesLower;
    cfm->resultCode                    = resultCode;
    cfm->resultSupplier                = resultSupplier;
    CsrBtCmPutMessage(qid, cfm);
}

static void csrBtCmLeReadSupportedState(cmInstanceData_t        *cmInst,
                                        struct cmCallbackObjTag *object,
                                        void                    *context,
                                        void                    *event)
{
    CsrBtCmLeGetControllerInfoReq *req           = (CsrBtCmLeGetControllerInfoReq *) context;
    DM_HCI_ULP_READ_SUPPORTED_STATES_CFM_T *prim = (DM_HCI_ULP_READ_SUPPORTED_STATES_CFM_T*) event;

    if(prim->status == HCI_SUCCESS)
    {
        csrBtCmLeGetControllerInfoCfmSend(req->appHandle, 
                                          req->whiteListSize, 
                                          (CsrUint32)((prim->supported_states[7] << 24) | (prim->supported_states[6] << 16) | (prim->supported_states[5] << 8) | prim->supported_states[4]), 
                                          (CsrUint32)((prim->supported_states[3] << 24) | (prim->supported_states[2] << 16) | (prim->supported_states[1] << 8) | prim->supported_states[0]), 
                                          CSR_BT_RESULT_CODE_CM_SUCCESS, 
                                          CSR_BT_SUPPLIER_CM);

    }
    else
    {
        csrBtCmLeGetControllerInfoCfmSend(req->appHandle, 0, 0, 0, (CsrBtResultCode) prim->status, CSR_BT_SUPPLIER_HCI);
    }
    CSR_UNUSED(cmInst);
    CSR_UNUSED(object);
}

static void csrBtCmLeReadWhiteListSize(cmInstanceData_t        *cmInst,
                                       struct cmCallbackObjTag *object,
                                       void                    *context,
                                       void                    *event)
{ 
    DM_UPRIM_T *dmPrim;
    CsrBtCmLeGetControllerInfoReq *req = (CsrBtCmLeGetControllerInfoReq *) context;
    DM_HCI_ULP_READ_WHITE_LIST_SIZE_CFM_T *prim = (DM_HCI_ULP_READ_WHITE_LIST_SIZE_CFM_T*) event;

    if (prim->status == HCI_SUCCESS)
    {
        req->whiteListSize = prim->white_list_size;
    }
    dm_hci_ulp_read_supported_states_req(&dmPrim);
    CsrBtCmCallbackSendSimpleBlockDm(cmInst,
                                     DM_HCI_ULP_READ_SUPPORTED_STATES_CFM,
                                     req,
                                     dmPrim,
                                     csrBtCmLeReadSupportedState);
    object->context = NULL;
}

void CsrBtCmLeGetControllerInfoReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmLeGetControllerInfoReq *prim = (CsrBtCmLeGetControllerInfoReq*) cmData->recvMsgP;
    
    if (HCI_FEATURE_IS_SUPPORTED(LMP_FEATURES_LE_SUPPORTED_CONTROLLER_BIT, cmData->dmVar.lmpSuppFeatures))
    {
        DM_UPRIM_T *dmPrim;
        dm_hci_ulp_read_white_list_size_req(&dmPrim);
        CsrBtCmCallbackSendSimpleBlockDm(cmData,
                                           DM_HCI_ULP_READ_WHITE_LIST_SIZE_CFM,
                                           cmData->recvMsgP,
                                           dmPrim,
                                           csrBtCmLeReadWhiteListSize);
        cmData->recvMsgP = NULL;
    }
    else
    {
        csrBtCmLeGetControllerInfoCfmSend(prim->appHandle, 0, 0, 0, CSR_BT_RESULT_CODE_CM_UNSUPPORTED_FEATURE, CSR_BT_SUPPLIER_CM);
    }
}

static void csrBtCmLeReadRemoteUsedFeaturesReqComplete(cmInstanceData_t *cmInst,
                                                       struct cmCallbackObjTag *object,
                                                       void *context,
                                                       void *event)
{
    CsrBtCmLeReadRemoteUsedFeaturesReq *req;
    DM_HCI_ULP_READ_REMOTE_USED_FEATURES_CFM_T *prim;
    CsrBtCmLeReadRemoteUsedFeaturesCfm *cfm;

    CSR_UNUSED(cmInst);
    CSR_UNUSED(object);

    req = (CsrBtCmLeReadRemoteUsedFeaturesReq *) context;
    prim = (DM_HCI_ULP_READ_REMOTE_USED_FEATURES_CFM_T *) event;
    cfm = (CsrBtCmLeReadRemoteUsedFeaturesCfm *) CsrPmemAlloc(sizeof(*cfm));

    cfm->type = CSR_BT_CM_LE_READ_REMOTE_USED_FEATURES_CFM;
    cfm->address = prim->addrt;
    SynMemCpyS(cfm->remoteLeFeatures,
              sizeof(cfm->remoteLeFeatures),
              prim->feature_set,
              sizeof(cfm->remoteLeFeatures));
    cfm->resultCode = (CsrBtResultCode) (prim->status == HCI_SUCCESS
                                         ? CSR_BT_RESULT_CODE_CM_SUCCESS :
                                           prim->status);
    cfm->resultSupplier = (prim->status == HCI_SUCCESS
                           ? CSR_BT_SUPPLIER_CM :
                           CSR_BT_SUPPLIER_HCI);

    CsrBtCmPutMessage(req->appHandle, cfm);
}

void CsrBtCmLeReadRemoteUsedFeaturesReqHandler(cmInstanceData_t *cmInst)
{
    CsrBtCmLeReadRemoteUsedFeaturesReq *req;
    DM_UPRIM_T *prim;

    req = (CsrBtCmLeReadRemoteUsedFeaturesReq *) cmInst->recvMsgP;
    cmInst->recvMsgP = NULL;

    dm_hci_ulp_read_remote_used_features_req(&req->address, &prim);
    CsrBtCmCallbackSendSimpleBlockDm(cmInst,
                                     DM_HCI_ULP_READ_REMOTE_USED_FEATURES_CFM,
                                     req,
                                     prim,
                                     csrBtCmLeReadRemoteUsedFeaturesReqComplete);
}

static void csrBtCmLeReadLocalSupportedFeaturesComplete(cmInstanceData_t *cmInst,
                                                        struct cmCallbackObjTag *object,
                                                        void *context,
                                                        void *event)
{
    DM_HCI_ULP_READ_LOCAL_SUPPORTED_FEATURES_CFM_T *prim;
    CsrBtCmLeReadLocalSupportedFeaturesReq *req;
    CsrBtCmLeReadLocalSupportedFeaturesCfm *cfm;

    CSR_UNUSED(object);

    prim = (DM_HCI_ULP_READ_LOCAL_SUPPORTED_FEATURES_CFM_T *) event;
    req  = (CsrBtCmLeReadLocalSupportedFeaturesReq *) context;
    cfm  = (CsrBtCmLeReadLocalSupportedFeaturesCfm *) CsrPmemAlloc(sizeof(*cfm));

    cfm->type = CSR_BT_CM_LE_READ_LOCAL_SUPPORTED_FEATURES_CFM;
    SynMemCpyS(cfm->localLeFeatures,
              sizeof(cfm->localLeFeatures),
              prim->feature_set,
              sizeof(cfm->localLeFeatures));
    cfm->resultCode = (CsrBtResultCode) (prim->status == HCI_SUCCESS ?
                                         CSR_BT_RESULT_CODE_CM_SUCCESS :
                                         prim->status);
    cfm->resultSupplier = (prim->status == HCI_SUCCESS ?
                           CSR_BT_SUPPLIER_CM :
                           CSR_BT_SUPPLIER_HCI);
#ifdef CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT
    /* Keep the result stored into CM instance LE record */
    if (CSR_BT_LE_LOCAL_FEATURE_SUPPORTED(prim->feature_set,
                                          CSR_BT_LE_FEATURE_LL_PRIVACY))
    {
        cmInst->leVar.llFeaturePrivacy = TRUE;
    }
    else
#endif
    {
        cmInst->leVar.llFeaturePrivacy = FALSE;
    }

    CsrBtCmPutMessage(req->appHandle, cfm);
}

void CsrBtCmLeReadLocalSupportedFeaturesReqHandler(cmInstanceData_t *cmInst)
{
    CsrBtCmLeReadLocalSupportedFeaturesReq *req;
    DM_UPRIM_T *prim;

    req = (CsrBtCmLeReadLocalSupportedFeaturesReq *) cmInst->recvMsgP;

    dm_hci_ulp_read_local_supported_features_req(&prim);
    CsrBtCmCallbackSendSimpleBlockDm(cmInst,
                                     DM_HCI_ULP_READ_LOCAL_SUPPORTED_FEATURES_CFM,
                                     req,
                                     prim,
                                     csrBtCmLeReadLocalSupportedFeaturesComplete);
    cmInst->recvMsgP = NULL;
}

void CsrBtCmLeReadRandomAddressCompleteHandler(cmInstanceData_t *cmData)
{
    DM_SM_READ_RANDOM_ADDRESS_CFM_T *dmPrim = 
                            (DM_SM_READ_RANDOM_ADDRESS_CFM_T *)cmData->recvMsgP;
    CsrBtCmLeReadRandomAddressCfm *cfm = CsrPmemAlloc(sizeof(*cfm));

    cfm->type           = CSR_BT_CM_LE_READ_RANDOM_ADDRESS_CFM;
    cfm->idAddress.addr = dmPrim->tp_peer_addrt.addrt.addr;
    cfm->idAddress.type = dmPrim->tp_peer_addrt.addrt.type;
    cfm->flag           = (CsrUint8) dmPrim->flags; /* either local or peer RPA as requested */
    cfm->rpa            = dmPrim->tp_addrt.addrt.addr;
    cfm->resultCode     = (CsrBtResultCode) (dmPrim->status == HCI_SUCCESS ?
                                             CSR_BT_RESULT_CODE_CM_SUCCESS :
                                             dmPrim->status);
    cfm->resultSupplier = (CsrBtSupplier) (dmPrim->status == HCI_SUCCESS ?
                                           CSR_BT_SUPPLIER_CM :
                                           CSR_BT_SUPPLIER_HCI);

    CsrBtCmPutMessage(cmData->dmVar.appHandle, cfm);

    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmLeReadRandomAddressReqHandler(cmInstanceData_t *cmInst)
{
    CsrBtCmLeReadRandomAddressReq *req;
    TP_BD_ADDR_T tpAddr;

    req = (CsrBtCmLeReadRandomAddressReq *) cmInst->recvMsgP;

    cmInst->dmVar.appHandle = req->appHandle;
    CsrBtAddrCopy(&(tpAddr.addrt), &(req->idAddress));
    tpAddr.tp_type = CSR_BT_TRANSPORT_LE;

    dm_sm_read_random_address_req(&tpAddr, (CsrUint16)req->flag, NULL);
}

#ifdef CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT
static void csrBtCmLeReadResolvingListSizeComplete(cmInstanceData_t *cmInst,
                                                   struct cmCallbackObjTag *object,
                                                   void *context,
                                                   void *event)
{
    DM_HCI_ULP_READ_RESOLVING_LIST_SIZE_CFM_T *prim = (DM_HCI_ULP_READ_RESOLVING_LIST_SIZE_CFM_T *) event;
    CsrBtCmLeReadResolvingListSizeReq *req = (CsrBtCmLeReadResolvingListSizeReq *) context;
    CsrBtCmLeReadResolvingListSizeCfm *cfm = CsrPmemAlloc(sizeof(*cfm));

    CSR_UNUSED(cmInst);
    CSR_UNUSED(object);

    cfm->type              = CSR_BT_CM_LE_READ_RESOLVING_LIST_SIZE_CFM;
    cfm->resolvingListSize = prim->resolving_list_size;
    cfm->resultCode        = (CsrBtResultCode) (prim->status == HCI_SUCCESS ?
                                                CSR_BT_RESULT_CODE_CM_SUCCESS :
                                                prim->status);
    cfm->resultSupplier    = (CsrBtSupplier) (prim->status == HCI_SUCCESS ?
                                              CSR_BT_SUPPLIER_CM :
                                              CSR_BT_SUPPLIER_HCI);

    CsrBtCmPutMessage(req->appHandle, cfm);
}

void CsrBtCmLeReadResolvingListSizeReqHandler(cmInstanceData_t *cmInst)
{
    CsrBtCmLeReadResolvingListSizeReq *req;

    req = (CsrBtCmLeReadResolvingListSizeReq *) cmInst->recvMsgP;

    if (cmInst->leVar.llFeaturePrivacy)
    { /* Local controller supports LL_PRIVACY feature */
        DM_UPRIM_T *prim;

        dm_hci_ulp_read_resolving_list_size_req(&prim);
        CsrBtCmCallbackSendSimpleBlockDm(cmInst,
                                         DM_HCI_ULP_READ_RESOLVING_LIST_SIZE_CFM,
                                         req,
                                         prim,
                                         csrBtCmLeReadResolvingListSizeComplete);
        cmInst->recvMsgP = NULL;
    }
    else
    { /* controller does not support LL_PRIVACY feature */
        CsrBtCmLeReadResolvingListSizeCfm *cfm = CsrPmemAlloc(sizeof(*cfm));

        cfm->type           = CSR_BT_CM_LE_READ_RESOLVING_LIST_SIZE_CFM;
        cfm->resultCode     = CSR_BT_RESULT_CODE_CM_UNSUPPORTED_FEATURE;
        cfm->resultSupplier = CSR_BT_SUPPLIER_CM;
        CsrBtCmPutMessage(req->appHandle, cfm);
    }
}

static void csrBtCmLeSetPrivacyModeComplete(cmInstanceData_t *cmInst,
                                            struct cmCallbackObjTag *object,
                                            void *context,
                                            void *event)
{
    DM_HCI_ULP_SET_PRIVACY_MODE_CFM_T *prim = (DM_HCI_ULP_SET_PRIVACY_MODE_CFM_T *) event;
    CsrBtCmLeSetPrivacyModeReq *req = (CsrBtCmLeSetPrivacyModeReq *) context;
    CsrBtCmLeSetPrivacyModeCfm *cfm = CsrPmemAlloc(sizeof(*cfm));

    CSR_UNUSED(cmInst);
    CSR_UNUSED(object);

    cfm->type           = CSR_BT_CM_LE_SET_PRIVACY_MODE_CFM;
    cfm->resultCode     = (CsrBtResultCode) (prim->status == HCI_SUCCESS ?
                                             CSR_BT_RESULT_CODE_CM_SUCCESS :
                                             prim->status);
    cfm->resultSupplier = (CsrBtSupplier) (prim->status == HCI_SUCCESS ?
                                           CSR_BT_SUPPLIER_CM :
                                           CSR_BT_SUPPLIER_HCI);

    CsrBtCmPutMessage(req->appHandle, cfm);
}

void CsrBtCmLeSetPrivacyModeReqHandler(cmInstanceData_t *cmInst)
{
    CsrBtCmLeSetPrivacyModeReq *req;

    req = (CsrBtCmLeSetPrivacyModeReq *) cmInst->recvMsgP;

    if (cmInst->leVar.llFeaturePrivacy)
    { /* Local controller supports LL_PRIVACY feature */
        DM_UPRIM_T *prim;

        dm_hci_ulp_set_privacy_mode_req(&(req->peerIdAddress), req->privacyMode, &prim);
        CsrBtCmCallbackSendSimpleBlockDm(cmInst,
                                         DM_HCI_ULP_SET_PRIVACY_MODE_CFM,
                                         req,
                                         prim,
                                         csrBtCmLeSetPrivacyModeComplete);
        cmInst->recvMsgP = NULL;
    }
    else
    { /* controller does not support LL_PRIVACY feature */
        CsrBtCmLeSetPrivacyModeCfm *cfm = CsrPmemAlloc(sizeof(*cfm));

        cfm->type           = CSR_BT_CM_LE_SET_PRIVACY_MODE_CFM;
        cfm->resultCode     = CSR_BT_RESULT_CODE_CM_UNSUPPORTED_FEATURE;
        cfm->resultSupplier = CSR_BT_SUPPLIER_CM;
        CsrBtCmPutMessage(req->appHandle, cfm);
    }
}
#endif /* CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT */

void CsrBtCmLeSetOwnAddressTypeReqHandler(cmInstanceData_t *cmInst)
{
    CsrBtCmLeSetOwnAddressTypeReq *req;
    CsrBtCmLeSetOwnAddressTypeCfm *cfm;
    CsrBtResultCode resultCode = CSR_BT_RESULT_CODE_CM_COMMAND_DISALLOWED;

    req = (CsrBtCmLeSetOwnAddressTypeReq *) cmInst->recvMsgP;
    cfm = (CsrBtCmLeSetOwnAddressTypeCfm *) CsrPmemAlloc(sizeof(*cfm));

/* If default random address is configured static, own address type cannot
 * be changed. */
#ifndef CSR_BT_LE_RANDOM_ADDRESS_TYPE_STATIC
    /* allow only if scanning and advertisement not in progress */
    if (!cmInst->leVar.scanMode && !cmInst->leVar.advMode)
    {
        /* own address type should be in valid range [0x00 - 0x03]*/
        switch (req->ownAddressType)
        {
#ifdef CSR_BT_LE_RANDOM_ADDRESS_TYPE_RPA
#ifdef CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT
            case CSR_BT_ADDR_TYPE_RPA_OR_PUBLIC:
            case CSR_BT_ADDR_TYPE_RPA_OR_RANDOM:
            {
                if (cmInst->leVar.llFeaturePrivacy)
                { /* controller supports LL_PRIVACY feature bit */
                    resultCode = CSR_BT_RESULT_CODE_CM_SUCCESS;
                }
                else
                {
                    resultCode = CSR_BT_RESULT_CODE_CM_UNSUPPORTED_FEATURE;
                }
                break;
            }
#endif /* CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT */
#endif /* CSR_BT_LE_RANDOM_ADDRESS_TYPE_RPA */

            case CSR_BT_ADDR_TYPE_PUBLIC:
            case CSR_BT_ADDR_TYPE_RANDOM:
            {
                resultCode = CSR_BT_RESULT_CODE_CM_SUCCESS;
                break;
            }
            default:
            {
                resultCode = CSR_BT_RESULT_CODE_CM_UNACCEPTABLE_PARAMETER;
                break;
            }
        }
    }

    if (resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS &&
        req->ownAddressType != cmInst->leVar.ownAddressType)
    { /* current and new own address types are different */
        cmInst->leVar.ownAddressType = req->ownAddressType;

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LE_OWN_ADDR_TYPE_CHANGE
        /* Propagate own address type change event to subscribers */
        CsrBtCmPropgateEvent(cmInst,
                             CsrBtCmPropagateLeOwnAddressTypeChangedEvent,
                             CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LE_OWN_ADDR_TYPE_CHANGE,
                             HCI_SUCCESS,
                             NULL,
                             NULL);
#endif
    }
#endif /* CSR_BT_LE_RANDOM_ADDRESS_TYPE_STATIC */

    cfm->type           = CSR_BT_CM_LE_SET_OWN_ADDRESS_TYPE_CFM;
    cfm->resultCode     = resultCode;
    cfm->resultSupplier = CSR_BT_SUPPLIER_CM;
    CsrBtCmPutMessage(req->appHandle, cfm);
}

void CsrBtCmLeSetStaticAddressReqHandler(cmInstanceData_t *cmInst)
{
    CsrBtCmLeSetStaticAddressReq *req;
    CsrBtCmLeSetStaticAddressCfm *cfm;
    CsrBtResultCode resultCode = CSR_BT_RESULT_CODE_CM_COMMAND_DISALLOWED;

    req = (CsrBtCmLeSetStaticAddressReq *) cmInst->recvMsgP;
    cfm = (CsrBtCmLeSetStaticAddressCfm *) CsrPmemAlloc(sizeof(*cfm));

#ifdef CSR_BT_LE_RANDOM_ADDRESS_TYPE_STATIC
    if (!cmInst->leVar.staticAddrSet)
    {
        TP_BD_ADDR_T staticAddress;

        staticAddress.addrt.addr = req->staticAddress;
        staticAddress.addrt.type = CSR_BT_ADDR_RANDOM;

        if (CsrBtAddrIsStatic(&staticAddress.addrt))
        {
            cmInst->leVar.localStaticAddr = req->staticAddress;
            resultCode = CSR_BT_RESULT_CODE_CM_SUCCESS;
        }
        else
        {
            resultCode = CSR_BT_RESULT_CODE_CM_UNACCEPTABLE_PARAMETER;
        }
    }
#endif

    cfm->type           = CSR_BT_CM_LE_SET_STATIC_ADDRESS_CFM;
    cfm->resultCode     = resultCode;
    cfm->resultSupplier = CSR_BT_SUPPLIER_CM;
    CsrBtCmPutMessage(req->appHandle, cfm);
}

static void csrBtCmLeSetPvtAddrTimeoutComplete(cmInstanceData_t *cmInst,
                                               struct cmCallbackObjTag *object,
                                               void *context,
                                               void *event)
{
    DM_SM_AUTO_CONFIGURE_LOCAL_ADDRESS_CFM_T *prim = (DM_SM_AUTO_CONFIGURE_LOCAL_ADDRESS_CFM_T *) event;
    CsrBtCmLeSetPvtAddrTimeoutReq *req = (CsrBtCmLeSetPvtAddrTimeoutReq *) context;

    CSR_UNUSED(cmInst);
    CSR_UNUSED(object);

    /* DM_SM_AUTO_CONFIGURE_LOCAL_ADDRESS_CFM_T can be received in two cases.
     * 1. In case of default auto configure request to generate random address
     *    at the time of intialization procedure and
     * 2. When application requests to set private address timeout.
     * In first case, "req" will be NULL and there will not be any confirmation
     * sent to application where in second case "req" is having valid context
     * and confirmation shall be sent to respective application */
    if (req)
    {
        CsrBtCmLeSetPvtAddrTimeoutCfm *cfm = CsrPmemAlloc(sizeof(*cfm));

        cfm->type           = CSR_BT_CM_LE_SET_PVT_ADDR_TIMEOUT_CFM;
        cfm->resultCode     = (CsrBtResultCode) (prim->status == HCI_SUCCESS ?
                                                 CSR_BT_RESULT_CODE_CM_SUCCESS :
                                                 prim->status);
        cfm->resultSupplier = (CsrBtSupplier) (prim->status == HCI_SUCCESS ?
                                               CSR_BT_SUPPLIER_CM :
                                               CSR_BT_SUPPLIER_HCI);

        CsrBtCmPutMessage(req->appHandle, cfm);
    }
}

void CsrBtCmLeSetPvtAddrTimeoutReqHandler(cmInstanceData_t *cmInst)
{
    CsrBtCmLeSetPvtAddrTimeoutReq *req;
    CsrBtResultCode resultCode = CSR_BT_RESULT_CODE_CM_COMMAND_DISALLOWED;

    req = (CsrBtCmLeSetPvtAddrTimeoutReq *) cmInst->recvMsgP;

#if defined(CSR_BT_LE_RANDOM_ADDRESS_TYPE_RPA) || defined(CSR_BT_LE_RANDOM_ADDRESS_TYPE_NRPA)
    /* Check whether requested RPA Timeout value is in valid range */
    if (req->timeout >= CSR_BT_RPA_TIMEOUT_MIN &&
        req->timeout <= CSR_BT_RPA_TIMEOUT_MAX)
    {
        CsrBtAddressType randomAddrType;
        DM_UPRIM_T *prim;

#ifdef CSR_BT_LE_RANDOM_ADDRESS_TYPE_RPA
        randomAddrType = CSR_BT_LE_GEN_RPA;
#else
        randomAddrType = CSR_BT_LE_GEN_NRPA;
#endif
        resultCode = CSR_BT_RESULT_CODE_CM_SUCCESS;
        cmInst->leVar.pvtAddrTimeout = req->timeout;
        /* configure random address again to get new RPA timeout applicable */
        dm_sm_auto_configure_local_address_req(randomAddrType,
                                               NULL,
                                               req->timeout,
                                               &prim);
        CsrBtCmCallbackSendSimpleBlockDm(cmInst,
                                         DM_SM_AUTO_CONFIGURE_LOCAL_ADDRESS_CFM,
                                         req,
                                         prim,
                                         csrBtCmLeSetPvtAddrTimeoutComplete);
        cmInst->recvMsgP = NULL;
    }
    else
    { /* timeout value is out of range */
        resultCode = CSR_BT_RESULT_CODE_CM_UNACCEPTABLE_PARAMETER;
    }
#endif /* End of #if defined() */

    if (resultCode != CSR_BT_RESULT_CODE_CM_SUCCESS)
    {
        CsrBtCmLeSetPvtAddrTimeoutCfm *cfm = CsrPmemAlloc(sizeof(*cfm));

        cfm->type           = CSR_BT_CM_LE_SET_PVT_ADDR_TIMEOUT_CFM;
        cfm->resultCode     = resultCode;
        cfm->resultSupplier = CSR_BT_SUPPLIER_CM;
        CsrBtCmPutMessage(req->appHandle, cfm);
    }
}

void CsrBtCmLeConfigureRandomAddress(cmInstanceData_t *cmData)
{
    CsrBtAddressType randomAddrType;
    CsrBtTpdAddrT    staticAddr;
    DM_UPRIM_T       *prim;
    CsrUint16        timeout = CSR_BT_RPA_TIMEOUT_INVALID;

#if defined(CSR_BT_LE_RANDOM_ADDRESS_TYPE_RPA) || defined(CSR_BT_LE_RANDOM_ADDRESS_TYPE_NRPA)
    timeout = cmData->leVar.pvtAddrTimeout;
#ifdef CSR_BT_LE_RANDOM_ADDRESS_TYPE_RPA
    randomAddrType = CSR_BT_LE_GEN_RPA;
#else
    randomAddrType = CSR_BT_LE_GEN_NRPA;
#endif /* CSR_BT_LE_RANDOM_ADDRESS_TYPE_RPA */
#endif

    CsrMemSet(&staticAddr, 0, sizeof(CsrBtTpdAddrT));

#ifdef CSR_BT_LE_RANDOM_ADDRESS_TYPE_STATIC
    {
        CsrBtTypedAddr addrt;

        addrt.addr = cmData->leVar.localStaticAddr;
        addrt.type = CSR_BT_ADDR_RANDOM;

        if (CsrBtAddrIsStatic(&addrt))
        {
            /* Set below flag to true just to protect the behaviour of static
             * address; once set it can't be changed again for same power cycle
             * and copy the static address to be configured */
            CsrBtAddrCopy(&(staticAddr.addrt), &addrt);
            staticAddr.tp_type = CSR_BT_TRANSPORT_LE;
            randomAddrType     = CSR_BT_LE_WRITE_STATIC_ADDRESS;
        }
        else
        {
            randomAddrType = CSR_BT_LE_GEN_STATIC_ADDRESS;
        }
        cmData->leVar.staticAddrSet = TRUE;
    }
#endif /* CSR_BT_LE_RANDOM_ADDRESS_TYPE_STATIC */

    dm_sm_auto_configure_local_address_req(randomAddrType,
                                           CsrBtBdAddrEqZero(&(staticAddr.addrt.addr)) ?
                                                             NULL :
                                                             &staticAddr,
                                           timeout,
                                           &prim);
     CsrBtCmCallbackSendSimpleBlockDm(cmData,
                                      DM_SM_AUTO_CONFIGURE_LOCAL_ADDRESS_CFM,
                                      NULL,
                                      prim,
                                      csrBtCmLeSetPvtAddrTimeoutComplete);
}

leConnVar *CsrBtCmLeFindConn(cmInstanceData_t *cmData, const CsrBtTypedAddr *addr)
{
    leConnVar *conn;

    /* Traverse LE connections */
    for (conn = cmData->leVar.connCache; conn != NULL; conn = conn->next)
    {
        if (CsrBtAddrEq(addr, &conn->addr))
        {
            return (conn);
        }
    }

    return (NULL);
}


#ifdef CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT
void CsrBtCmLeReadLocalIrkCompleteHandler(cmInstanceData_t *cmData)
{
    DM_SM_READ_LOCAL_IRK_CFM_T *dmPrim = cmData->recvMsgP;
    CsrBtCmLeReadLocalIrkCfm *cfm = CsrPmemAlloc(sizeof(*cfm));

    cfm->type = CSR_BT_CM_LE_READ_LOCAL_IRK_CFM;
    cfm->remoteAddress = dmPrim->addrt;
    SynMemCpyS(cfm->irk, sizeof(cfm->irk), dmPrim->irk, sizeof(dmPrim->irk));

    cfm->resultCode = (CsrBtResultCode) (dmPrim->status == HCI_SUCCESS ?
                                         CSR_BT_RESULT_CODE_CM_SUCCESS :
                                         dmPrim->status);

    cfm->resultSupplier = (CsrBtSupplier) (dmPrim->status == HCI_SUCCESS ?
                                           CSR_BT_SUPPLIER_CM :
                                           CSR_BT_SUPPLIER_HCI);

    CsrBtCmPutMessage(cmData->dmVar.appHandle, cfm);

    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmLeReadLocalIrkReqHandler(cmInstanceData_t *cmInst)
{
    CsrBtCmLeReadLocalIrkReq *req;

    req = (CsrBtCmLeReadLocalIrkReq *) cmInst->recvMsgP;
    cmInst->dmVar.appHandle = req->appHandle;
    dm_sm_read_local_irk_req(CSR_BT_CM_IFACEQUEUE, &(req->remoteAddress), NULL);
}
#endif

#ifndef EXCLUDE_DM_SM_SIRK_OPERATION_REQ
void CsrBtCmLeSirkOperationCompleteHandler(cmInstanceData_t *cmData)
{
    DM_SM_SIRK_OPERATION_CFM_T *dmPrim = cmData->recvMsgP;
    CsrBtCmLeSirkOperationCfm *cfm = CsrPmemAlloc(sizeof(*cfm));

    cfm->type = CSR_BT_CM_LE_SIRK_OPERATION_CFM;
    cfm->tpAddrt = dmPrim->tp_addrt;
    SynMemCpyS(cfm->sirkKey, sizeof(cfm->sirkKey), dmPrim->sirk_key, sizeof(dmPrim->sirk_key));

    cfm->resultCode = (CsrBtResultCode) (dmPrim->status == HCI_SUCCESS ?
                                         CSR_BT_RESULT_CODE_CM_SUCCESS :
                                         dmPrim->status);

    CsrBtCmPutMessage(cmData->dmVar.appHandle, cfm);

    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmLeSirkOperationReqHandler(cmInstanceData_t *cmInst)
{
    CsrBtCmLeSirkOperationReq *req;

    req = (CsrBtCmLeSirkOperationReq *) cmInst->recvMsgP;
    cmInst->dmVar.appHandle = req->appHandle;
    dm_sm_sirk_operation_req(CSR_BT_CM_IFACEQUEUE,
                             0,
                             &req->tpAddrt,
                             (uint8_t)req->flags,
                             req->sirkKey,
                             NULL);
}
#endif

#ifdef DM_ULP_SET_DATA_RELATED_ADDRESS_CHANGES_REQ
void CsrBtCmLeSetDataRelatedAddressChangesCompleteHandler(cmInstanceData_t *cmData)
{
    DM_ULP_SET_DATA_RELATED_ADDRESS_CHANGES_CFM_T *dmPrim = cmData->recvMsgP;
    CsrBtCmLeSetDataRelatedAddressChangesCfm *cfm = CsrPmemAlloc(sizeof(*cfm));

    cfm->type = CSR_BT_CM_LE_SET_DATA_RELATED_ADDRESS_CHANGES_CFM;
    cfm->resultCode     = (CsrBtResultCode) (dmPrim->status == HCI_SUCCESS ?
                                          CSR_BT_RESULT_CODE_CM_SUCCESS :
                                          dmPrim->status);
    cfm->resultSupplier = (CsrBtSupplier) (dmPrim->status == HCI_SUCCESS ?
                                        CSR_BT_SUPPLIER_CM :
                                        CSR_BT_SUPPLIER_HCI);
    cfm->advHandle = cmData->advHandle;

    CsrBtCmPutMessage(cmData->dmVar.appHandle, cfm);

    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmLeSetDataRelatedAddressChangesReqHandler(cmInstanceData_t *cmInst)
{
    CsrBtCmLeSetDataRelatedAddressChangesReq *req;

    req = (CsrBtCmLeSetDataRelatedAddressChangesReq *) cmInst->recvMsgP;
    cmInst->dmVar.appHandle = req->appHandle;
    cmInst->advHandle = req->advHandle;
    
    dm_ulp_set_data_related_address_changes_req(req->advHandle,
                                                req->flags,
                                                req->changeReasons,
                                                NULL);
}
#endif

#if defined(CSR_BT_INSTALL_EXTENDED_SCANNING) || defined(CSR_BT_INSTALL_EXTENDED_ADVERTISING) \
    || defined(CSR_BT_INSTALL_PERIODIC_SCANNING) || defined(CSR_BT_INSTALL_PERIODIC_ADVERTISING)

#ifdef CSR_STREAMS_ENABLE

void CmExtScanFilteredAdvReportDoneIndHandler(cmInstanceData_t *cmData)
{
    CmExtScanFilteredAdvReportDoneInd *req = (CmExtScanFilteredAdvReportDoneInd*) cmData->recvMsgP;
    Source source = *((Source*)req->data);
    SourceDrop(source, req->dataLength);

    if(cmData->extScanState & CSR_BT_EA_PA_TERMINATING)
    {
        CmStreamFlushSource(source);
        cmData->extScanState &= ~(CSR_BT_EA_PA_TERMINATING | CSR_BT_EA_PA_REPORT_PROCESSING);
        cmData->extScanSource = 0;
        return;
    }

    /* Ready to process stream again for Advert Report */
    cmData->extScanState &= ~CSR_BT_EA_PA_REPORT_PROCESSING;

    CsrBtCmStreamProcessData(cmData, source);
}

void CmPeriodicScanSyncAdvReportDoneIndHandler(cmInstanceData_t *cmData)
{
    CmPeriodicScanSyncAdvReportDoneInd *req = (CmPeriodicScanSyncAdvReportDoneInd*) cmData->recvMsgP;
    CsrUint8 i;
    Source source = *((Source*)req->data);

    SourceDrop(source, req->dataLength);

    /* Read Streams again for Advert Report */
    for (i = 0; i < MAX_PERIODIC_SCAN_APP; i++)
    {
        if (cmData->periodicScanHandles[i].source == source)
        {
            /* PA train terminated/lost, so set source to 0 now */
            if(cmData->periodicScanHandles[i].paSyncState & CSR_BT_EA_PA_TERMINATING)
            {
                CmStreamFlushSource(cmData->periodicScanHandles[i].source);

                cmData->periodicScanHandles[i].pHandle = CSR_SCHED_QID_INVALID;
                cmData->periodicScanHandles[i].syncHandle = CSR_BT_PERIODIC_SCAN_HANDLE_INVALID;
                cmData->periodicScanHandles[i].source = 0;
                cmData->periodicScanHandles[i].paSyncState &= ~(CSR_BT_EA_PA_TERMINATING | CSR_BT_EA_PA_REPORT_PROCESSING);
                return;
            }

            cmData->periodicScanHandles[i].paSyncState &= ~CSR_BT_EA_PA_REPORT_PROCESSING;
            break;
        }
    }

    CsrBtCmStreamProcessData(cmData, source);
}
#endif /* CSR_STREAMS_ENABLE */

void CsrBtCmDmGetAdvScanCapabilitiesCfmHandler(cmInstanceData_t *cmData, void *msg)
{
    DM_ULP_GET_ADV_SCAN_CAPABILITIES_CFM_T *dmPrim = msg;
    CmGetAdvScanCapabilitiesCfm *prim;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_BT_CM_GET_ADV_SCAN_CAPABILITIES_CFM;
    prim->resultCode = (CsrBtResultCode) dmPrim->status;
    prim->availableApi = dmPrim->available_api;
    prim->availableAdvSets = dmPrim->available_adv_sets;
    prim->stackReservedAdvSets = dmPrim->stack_reserved_adv_sets;
    prim->maxPeriodicSyncListSize = dmPrim->max_periodic_sync_list_size;
    prim->supportedPhys = dmPrim->supported_phys;
    prim->maxPotentialSizeOfTxAdvData = dmPrim->max_potential_size_of_tx_adv_data;
    prim->maxPotentialSizeOfTxPeriodicAdvData = dmPrim->max_potential_size_of_tx_periodic_adv_data;
    prim->maxPotentialSizeOfRxAdvData = dmPrim->max_potential_size_of_rx_adv_data;
    prim->maxPotentialSizeOfRxPeriodicAdvData = dmPrim->max_potential_size_of_rx_periodic_adv_data;

    CsrSchedMessagePut(cmData->dmVar.appHandle, CSR_BT_CM_PRIM, (prim));
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmGetAdvScanCapabilitiesReqHandler(cmInstanceData_t *cmData)
{
    CmGetAdvScanCapabilitiesReq *req;

    req = (CmGetAdvScanCapabilitiesReq *) cmData->recvMsgP;
    cmData->dmVar.appHandle = req->appHandle;
    dm_ulp_get_adv_scan_capabilities_req(NULL);
}
#endif

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LE_SUBRATE_CHANGE
void CsrBtCmLeSetDefaultSubrateCfmHandler(cmInstanceData_t *cmData)
{
    DM_HCI_ULP_SET_DEFAULT_SUBRATE_CFM_T *dmPrim = cmData->recvMsgP;
    CsrBtCmLeSetDefaultSubrateCfm *cfm;

    cfm = CsrPmemAlloc(sizeof(CsrBtCmLeSetDefaultSubrateCfm));
    cfm->type = CSR_BT_CM_LE_SET_DEFAULT_SUBRATE_CFM;
    cfm->resultCode = (dmPrim->status == HCI_SUCCESS) ?
                       CSR_BT_RESULT_CODE_CM_SUCCESS : dmPrim->status;

    CsrBtCmPutMessage(cmData->dmVar.appHandle, cfm);
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmLeSetDefaultSubrateReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmLeSetDefaultSubrateReq *req;

    req = (CsrBtCmLeSetDefaultSubrateReq*)cmData->recvMsgP;
    cmData->dmVar.appHandle = req->appHandle;

    /* Set default subrate and connection parameters */
    dm_hci_ulp_set_default_subrate_req(req->subrate_min,
                                       req->subrate_max,
                                       req->max_latency,
                                       req->continuation_num,
                                       req->supervision_timeout,
                                       NULL);
}

void CsrBtCmLeSubrateChangeCfmHandler(cmInstanceData_t *cmData)
{
    DM_HCI_ULP_SUBRATE_CHANGE_CFM_T *dmPrim = cmData->recvMsgP;
    CsrBtCmLeSubrateChangeCfm *cfm;

    cfm = CsrPmemAlloc(sizeof(CsrBtCmLeSubrateChangeCfm));
    cfm->type = CSR_BT_CM_LE_SUBRATE_CHANGE_CFM;
    cfm->resultCode = (dmPrim->status == HCI_SUCCESS) ?
                       CSR_BT_RESULT_CODE_CM_SUCCESS : dmPrim->status;
    cfm->address = dmPrim->addrt;

    CsrBtCmPutMessage(cmData->dmVar.appHandle, cfm);
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmLeSubrateChangeReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmLeSubrateChangeReq *req;

    req = (CsrBtCmLeSubrateChangeReq*)cmData->recvMsgP;
    cmData->dmVar.appHandle = req->appHandle;

    /* Change subrate and connection parameters for a connection */
    dm_hci_ulp_subrate_change_req(&req->address,
                                  req->subrate_min,
                                  req->subrate_max,
                                  req->max_latency,
                                  req->continuation_num,
                                  req->supervision_timeout,
                                  NULL);
}

void CsrBtCmLeSubrateChangeIndHandler(cmInstanceData_t *cmData)
{
    DM_HCI_ULP_SUBRATE_CHANGE_IND_T *dmPrim;

    dmPrim = (DM_HCI_ULP_SUBRATE_CHANGE_IND_T*) cmData->recvMsgP;

    CsrBtCmPropgateEvent(cmData,
                         CsrBtCmPropgateLeSubrateChangeEvent,
                         CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LE_SUBRATE_CHANGE,
                         HCI_SUCCESS,
                         dmPrim,
                         NULL);
}
#endif /* CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LE_SUBRATE_CHANGE */

#ifdef INSTALL_CM_LE_PHY_UPDATE_FEATURE
void CmDmLeReadPhyReqHandler(cmInstanceData_t *cmData)
{
    CmDmLeReadPhyReq *req = (CmDmLeReadPhyReq *) cmData->recvMsgP;

    cmData->dmVar.appHandle = req->appHandle;

    dm_ulp_read_phy_req(req->appHandle,
                       &req->tpAddr,
                       NULL);
}

void CmDmLeSetPhyReqHandler(cmInstanceData_t *cmData)
{
    CmDmLeSetPhyReq *req = (CmDmLeSetPhyReq *) cmData->recvMsgP;

    cmData->dmVar.appHandle = req->appHandle;

    dm_ulp_set_phy_req(req->appHandle,
                       &req->tpAddr,
                       req->phyInfo.minTxRate,
                       req->phyInfo.maxTxRate,
                       req->phyInfo.minRxRate,
                       req->phyInfo.maxRxRate,
                       req->phyInfo.flags,
                       NULL);
}

void CmDmLeSetPhyCfmHandler(cmInstanceData_t *cmData)
{
    DM_ULP_SET_PHY_CFM_T *dmPrim = (DM_ULP_SET_PHY_CFM_T *)cmData->recvMsgP;
    CmDmLeSetPhyCfm *cfm = (CmDmLeSetPhyCfm *)CsrPmemAlloc(sizeof(*cfm));

    cfm->type       = CM_DM_LE_PHY_SET_CFM;
    cfm->tpAddr     = dmPrim->tp_addrt;
    cfm->txPhyType  = dmPrim->tx_phy_type;
    cfm->rxPhyType  = dmPrim->rx_phy_type;
    cfm->status     = dmPrim->status;

    CsrBtCmPutMessage(cmData->dmVar.appHandle, cfm);
    CsrBtCmDmLocalQueueHandler();
}

void CmDmLeSetDefaultPhyReqHandler(cmInstanceData_t *cmData)
{
    CmDmLeSetDefaultPhyReq *req = (CmDmLeSetDefaultPhyReq *)cmData->recvMsgP;

    cmData->dmVar.appHandle = req->appHandle;

    dm_ulp_set_default_phy_req(req->appHandle,
                               req->phyInfo.minTxRate,
                               req->phyInfo.maxTxRate,
                               req->phyInfo.minRxRate,
                               req->phyInfo.maxRxRate,
                               req->phyInfo.flags,
                               NULL);
}

void CmDmLeSetDefaultPhyCfmHandler(cmInstanceData_t *cmData)
{
    DM_ULP_SET_DEFAULT_PHY_CFM_T *dmPrim = (DM_ULP_SET_DEFAULT_PHY_CFM_T *)cmData->recvMsgP;
    CmDmLeSetDefaultPhyCfm *cfm = (CmDmLeSetDefaultPhyCfm *)CsrPmemAlloc(sizeof(*cfm));

    cfm->type       = CM_DM_LE_DEFAULT_PHY_SET_CFM;
    cfm->status     = dmPrim->status;

    CsrBtCmPutMessage(cmData->dmVar.appHandle, cfm);
    CsrBtCmDmLocalQueueHandler();
}

void CmDmLeSetDefaultPhyIndHandler(cmInstanceData_t *cmData)
{
    DM_ULP_PHY_UPDATE_IND_T *dmPrim = (DM_ULP_PHY_UPDATE_IND_T *)cmData->recvMsgP;
    CmDmLePhyUpdateInd *ind = (CmDmLePhyUpdateInd *)CsrPmemAlloc(sizeof(*ind));

    ind->type       = CM_DM_LE_PHY_UPDATE_IND;
    ind->tpAddr     = dmPrim->tp_addrt;
    ind->txPhyType  = dmPrim->tx_phy_type;
    ind->rxPhyType  = dmPrim->rx_phy_type;

    CsrBtCmPutMessage(CSR_BT_CM_LE_HANDLE(cmData), ind);
}

void CmDmLeReadPhyCfmHandler(cmInstanceData_t *cmData)
{
    DM_ULP_READ_PHY_CFM_T *dmPrim = (DM_ULP_READ_PHY_CFM_T *)cmData->recvMsgP;
    CmDmReadPhyCfm *cfm = (CmDmReadPhyCfm *)CsrPmemAlloc(sizeof(*cfm));

    cfm->type       = CM_DM_LE_PHY_READ_CFM;
    cfm->tp_addrt     = dmPrim->tp_addrt;
    cfm->tx_phy_type  = dmPrim->tx_phy_type;
    cfm->rx_phy_type  = dmPrim->rx_phy_type;
    cfm->status  = dmPrim->status;

    CsrBtCmPutMessage(cmData->dmVar.appHandle, cfm);
    CsrBtCmDmLocalQueueHandler();
}

#endif /* INSTALL_CM_LE_PHY_UPDATE_FEATURE */

static void cmLeAddDeviceToWhitelistCfmSend(CsrSchedQid qid, CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    CmLeAddDeviceToWhiteListCfm *cfm = (CmLeAddDeviceToWhiteListCfm*)CsrPmemAlloc(sizeof(CmLeAddDeviceToWhiteListCfm));
    cfm->type           = CM_LE_ADD_DEVICE_TO_WHITE_LIST_CFM;
    cfm->resultCode     = resultCode;
    cfm->resultSupplier = resultSupplier;
    CsrBtCmPutMessage(qid, cfm);
}

static void cmLeAddDeviceToWhitelistCfm(cmInstanceData_t         *cmInst,
                                        struct cmCallbackObjTag  *object,
                                        void                     *context,
                                        void                     *event)
{   
    /* The device is added to whitelist, send confirmation  */
    CmLeAddDeviceToWhiteListReq *req = (CmLeAddDeviceToWhiteListReq *) context;
    DM_HCI_ULP_ADD_DEVICE_TO_WHITE_LIST_CFM_T *prim = (DM_HCI_ULP_ADD_DEVICE_TO_WHITE_LIST_CFM_T*) event;

    cmLeAddDeviceToWhitelistCfmSend(req->appHandle,
                                    (CsrBtResultCode)((prim->status == HCI_SUCCESS) ? CSR_BT_RESULT_CODE_CM_SUCCESS : prim->status),
                                    (CsrBtSupplier)(prim->status == HCI_SUCCESS ? CSR_BT_SUPPLIER_CM : CSR_BT_SUPPLIER_HCI));
    CSR_UNUSED(cmInst);
    CSR_UNUSED(object);
}

void CmLeAddDeviceToWhiteListReqHandler(cmInstanceData_t *cmData)
{    
    CmLeAddDeviceToWhiteListReq *prim = (CmLeAddDeviceToWhiteListReq*)cmData->recvMsgP;
    
    if (HCI_FEATURE_IS_SUPPORTED(LMP_FEATURES_LE_SUPPORTED_CONTROLLER_BIT, cmData->dmVar.lmpSuppFeatures))
    {
        DM_UPRIM_T *dmPrim;
        CsrBtTypedAddr addrt;
        addrt.addr = prim->deviceAddr;
        addrt.type = prim->addrType;
        
        dm_hci_ulp_add_device_to_white_list_req(&addrt, &dmPrim);
        CsrBtCmCallbackSendSimpleBlockDm(cmData,
                                         DM_HCI_ULP_ADD_DEVICE_TO_WHITE_LIST_CFM,
                                         cmData->recvMsgP,
                                         dmPrim,
                                         cmLeAddDeviceToWhitelistCfm);
        cmData->recvMsgP = NULL;
    }
    else
    {
        cmLeAddDeviceToWhitelistCfmSend(prim->appHandle, CSR_BT_RESULT_CODE_CM_UNSUPPORTED_FEATURE, CSR_BT_SUPPLIER_CM);
    }
}

static void cmLeRemoveDeviceFromWhitelistCfmSend(CsrSchedQid qid, CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    CmLeRemoveDeviceFromWhiteListCfm *cfm = (CmLeRemoveDeviceFromWhiteListCfm*)CsrPmemAlloc(sizeof(CmLeRemoveDeviceFromWhiteListCfm));
    cfm->type           = CM_LE_REMOVE_DEVICE_FROM_WHITE_LIST_CFM;
    cfm->resultCode     = resultCode;
    cfm->resultSupplier = resultSupplier;
    CsrBtCmPutMessage(qid, cfm);
}

static void cmLeRemoveDeviceFromWhitelistCfm(cmInstanceData_t         *cmInst,
                                            struct cmCallbackObjTag  *object,
                                            void                     *context,
                                            void                     *event)
{  

    /* The device is removed from whitelist, send confirmation  */
    CmLeRemoveDeviceFromWhiteListReq *req = (CmLeRemoveDeviceFromWhiteListReq *) context;
    DM_HCI_ULP_REMOVE_DEVICE_FROM_WHITE_LIST_CFM_T *prim = (DM_HCI_ULP_REMOVE_DEVICE_FROM_WHITE_LIST_CFM_T*) event;

    cmLeRemoveDeviceFromWhitelistCfmSend(req->appHandle,
                                         (CsrBtResultCode)((prim->status == HCI_SUCCESS) ? CSR_BT_RESULT_CODE_CM_SUCCESS : prim->status),
                                         (CsrBtSupplier)(prim->status == HCI_SUCCESS ? CSR_BT_SUPPLIER_CM : CSR_BT_SUPPLIER_HCI));
    CSR_UNUSED(cmInst);
    CSR_UNUSED(object);
}

void CmLeRemoveDeviceFromWhiteListReqHandler(cmInstanceData_t *cmData)
{    
    CmLeRemoveDeviceFromWhiteListReq *prim = (CmLeRemoveDeviceFromWhiteListReq*)cmData->recvMsgP;
    
    if (HCI_FEATURE_IS_SUPPORTED(LMP_FEATURES_LE_SUPPORTED_CONTROLLER_BIT, cmData->dmVar.lmpSuppFeatures))
    {
        DM_UPRIM_T *dmPrim;
        CsrBtTypedAddr addrt;
        addrt.addr = prim->deviceAddr;
        addrt.type = prim->addrType;
        
        dm_hci_ulp_remove_device_from_white_list_req(&addrt, &dmPrim);
        CsrBtCmCallbackSendSimpleBlockDm(cmData,
                                         DM_HCI_ULP_REMOVE_DEVICE_FROM_WHITE_LIST_CFM,
                                         cmData->recvMsgP,
                                         dmPrim,
                                         cmLeRemoveDeviceFromWhitelistCfm);
        cmData->recvMsgP = NULL;
    }
    else
    {
        csrBtCmLeWhitelistSetCfmSend(prim->appHandle, CSR_BT_RESULT_CODE_CM_UNSUPPORTED_FEATURE, CSR_BT_SUPPLIER_CM);
    }
}

#ifdef INSTALL_CM_LE_READ_REMOTE_TRANSMIT_POWER_LEVEL
void CmDmLeReadRemoteTransmitPowerLevelReqHandler(cmInstanceData_t *cmData)
{
    CmDmLeReadRemoteTransmitPowerLevelReq    *req = (CmDmLeReadRemoteTransmitPowerLevelReq *)cmData->recvMsgP;
    cmData->dmVar.appHandle                       = req->appHandle;
    dm_read_remote_transmit_power_level_req(&req->tpAddrt, req->phy, NULL);
}

void CmDmLeReadRemoteTransmitPowerLevelCfmHandler(cmInstanceData_t *cmData)
{
    CmDmLeReadRemoteTransmitPowerLevelCfm            *cfm = (CmDmLeReadRemoteTransmitPowerLevelCfm *)CsrPmemZalloc(sizeof(*cfm));
    DM_ULP_READ_REMOTE_TRANSMIT_POWER_LEVEL_CFM_T    *dmPrim = (DM_ULP_READ_REMOTE_TRANSMIT_POWER_LEVEL_CFM_T *)cmData->recvMsgP;

    cfm->type      = CM_DM_LE_READ_REMOTE_TRANSMIT_POWER_LEVEL_CFM;
    cfm->tpAddrt   = dmPrim->tp_addrt;
    cfm->phy       = dmPrim->phy;
    if(dmPrim->status == HCI_SUCCESS)
    {
        cfm->reason            = dmPrim->reason;
        cfm->txPowerLevel      = dmPrim->tx_pwr_level;
        cfm->txPowerLevelFlag  = dmPrim->tx_pwr_level_flag;
        cfm->delta             = dmPrim->delta;
        cfm->resultCode        = CSR_BT_RESULT_CODE_CM_SUCCESS;
        cfm->resultSupplier    = CSR_BT_SUPPLIER_CM;
    }
    else
    {
        cfm->resultCode        = dmPrim->status;
        cfm->resultSupplier    = CSR_BT_SUPPLIER_HCI;
    }
    CsrBtCmPutMessage(cmData->dmVar.appHandle, cfm);
    CsrBtCmDmLocalQueueHandler();
}
#endif /* INSTALL_CM_LE_READ_REMOTE_TRANSMIT_POWER_LEVEL */

#ifdef INSTALL_CM_LE_SET_TRANSMIT_POWER_REPORTING
void CmDmLeSetTransmitPowerReportingEnableReqHandler(cmInstanceData_t *cmData)
{
    CmDmLeSetTransmitPowerReportingEnableReq    *req = (CmDmLeSetTransmitPowerReportingEnableReq *)cmData->recvMsgP;
    cmData->dmVar.appHandle                          = req->appHandle;
    dm_set_transmit_power_reporting_enable_req(&req->tpAddrt, req->localEnable, req->remoteEnable, NULL);
}

void CmDmLeSetTransmitPowerReportingEnableCfmHandler(cmInstanceData_t *cmData)
{
    CmDmLeSetTransmitPowerReportingEnableCfm               *cfm = (CmDmLeSetTransmitPowerReportingEnableCfm *)CsrPmemZalloc(sizeof(*cfm));
    DM_ULP_SET_TRANSMIT_POWER_REPORTING_ENABLE_CFM_T    *dmPrim = (DM_ULP_SET_TRANSMIT_POWER_REPORTING_ENABLE_CFM_T *)cmData->recvMsgP;

    cfm->type      = CM_DM_LE_SET_TRANSMIT_POWER_REPORTING_ENABLE_CFM;
    cfm->tpAddrt   = dmPrim->tp_addrt;
    if(dmPrim->status == HCI_SUCCESS)
    {
        cfm->resultCode        = CSR_BT_RESULT_CODE_CM_SUCCESS;
        cfm->resultSupplier    = CSR_BT_SUPPLIER_CM;
    }
    else
    {
        cfm->resultCode        = dmPrim->status;
        cfm->resultSupplier    = CSR_BT_SUPPLIER_HCI;
    }
    CsrBtCmPutMessage(cmData->dmVar.appHandle, cfm);
    CsrBtCmDmLocalQueueHandler();
}

void CmDmLeTransmitPowerReportingIndHandler(cmInstanceData_t *cmData)
{
    DM_ULP_TRANSMIT_POWER_REPORTING_IND_T   *dmPrim = (DM_ULP_TRANSMIT_POWER_REPORTING_IND_T *)cmData->recvMsgP;
    CmDmLeTransmitPowerReportingInd            *ind = (CmDmLeTransmitPowerReportingInd *)CsrPmemZalloc(sizeof(*ind));

    ind->type              = CM_DM_LE_TRANSMIT_POWER_REPORTING_IND;
    ind->tpAddrt           = dmPrim->tp_addrt;
    ind->reason            = dmPrim->reason;
    ind->phy               = dmPrim->phy;
    ind->txPowerLevel      = dmPrim->tx_pwr_level;
    ind->txPowerLevelFlag  = dmPrim->tx_pwr_level_flag;
    ind->delta             = dmPrim->delta;

    CsrBtCmPutMessage(CSR_BT_CM_LE_HANDLE(cmData), ind);
}
#endif /* INSTALL_CM_LE_SET_TRANSMIT_POWER_REPORTING */

#ifdef INSTALL_CM_LE_ENHANCED_READ_TRANSMIT_POWER_LEVEL
void CmDmLeEnhancedReadTransmitPowerLevelReqHandler(cmInstanceData_t *cmData)
{
    CmDmLeEnhancedReadTransmitPowerLevelReq  *req = (CmDmLeEnhancedReadTransmitPowerLevelReq *)cmData->recvMsgP;
    cmData->dmVar.appHandle                       = req->appHandle;
    dm_enhanced_read_transmit_power_level_req(&req->tpAddrt, req->phy, NULL);
}

void CmDmLeEnhancedReadTransmitPowerLevelCfmHandler(cmInstanceData_t *cmData)
{
    CmDmLeEnhancedReadTransmitPowerLevelCfm              *cfm = (CmDmLeEnhancedReadTransmitPowerLevelCfm *)CsrPmemZalloc(sizeof(*cfm));
    DM_ULP_ENHANCED_READ_TRANSMIT_POWER_LEVEL_CFM_T   *dmPrim = (DM_ULP_ENHANCED_READ_TRANSMIT_POWER_LEVEL_CFM_T *)cmData->recvMsgP;

    cfm->type      = CM_DM_LE_ENHANCED_READ_TRANSMIT_POWER_LEVEL_CFM;
    cfm->tpAddrt   = dmPrim->tp_addrt;
    cfm->phy       = dmPrim->phy;
    if(dmPrim->status == HCI_SUCCESS)
    {
        cfm->curTxPowerLevel   = dmPrim->current_tx_pwr_level;
        cfm->maxTxPowerLevel   = dmPrim->max_tx_pwr_level;
        cfm->resultCode        = CSR_BT_RESULT_CODE_CM_SUCCESS;
        cfm->resultSupplier    = CSR_BT_SUPPLIER_CM;
    }
    else
    {
        cfm->resultCode        = dmPrim->status;
        cfm->resultSupplier    = CSR_BT_SUPPLIER_HCI;
    }
    CsrBtCmPutMessage(cmData->dmVar.appHandle, cfm);
    CsrBtCmDmLocalQueueHandler();
}
#endif /* INSTALL_CM_LE_ENHANCED_READ_TRANSMIT_POWER_LEVEL */

#ifdef INSTALL_CM_LE_PATH_LOSS_REPORTING
void CmDmLeSetPathLossReportingParametersReqHandler(cmInstanceData_t *cmData)
{
    CmDmLeSetPathLossReportingParametersReq *req = (CmDmLeSetPathLossReportingParametersReq *)cmData->recvMsgP;
    cmData->dmVar.appHandle = req->appHandle;
    dm_set_path_loss_reporting_parameters_req(
                &req->tpAddrt,
                req->highThreshold,
                req->highHysteresis,
                req->lowThreshold,
                req->lowHysteresis,
                req->minTimeSpent,
                NULL);
}

void CmDmLeSetPathLossReportingParametersCfmHandler(cmInstanceData_t *cmData)
{
    CmDmLeSetPathLossReportingParametersCfm               *cfm = (CmDmLeSetPathLossReportingParametersCfm *)CsrPmemZalloc(sizeof(*cfm));
    DM_ULP_SET_PATH_LOSS_REPORTING_PARAMETERS_CFM_T    *dmPrim = (DM_ULP_SET_PATH_LOSS_REPORTING_PARAMETERS_CFM_T *)cmData->recvMsgP;

    cfm->type      = CM_DM_LE_SET_PATH_LOSS_REPORTING_PARAMETERS_CFM;
    cfm->tpAddrt   = dmPrim->tp_addrt;
    if(dmPrim->status == HCI_SUCCESS)
    {
        cfm->resultCode        = CSR_BT_RESULT_CODE_CM_SUCCESS;
        cfm->resultSupplier    = CSR_BT_SUPPLIER_CM;
    }
    else
    {
        cfm->resultCode        = dmPrim->status;
        cfm->resultSupplier    = CSR_BT_SUPPLIER_HCI;
    }
    CsrBtCmPutMessage(cmData->dmVar.appHandle, cfm);
    CsrBtCmDmLocalQueueHandler();
}

void CmDmLeSetPathLossReportingEnableReqHandler(cmInstanceData_t *cmData)
{
    CmDmLeSetPathLossReportingEnableReq *req = (CmDmLeSetPathLossReportingEnableReq *)cmData->recvMsgP;
    cmData->dmVar.appHandle = req->appHandle;
    dm_set_path_loss_reporting_enable_req(&req->tpAddrt, req->enable, NULL);
}

void CmDmLeSetPathLossReportingEnableCfmHandler(cmInstanceData_t *cmData)
{
    CmDmLeSetPathLossReportingEnableCfm         *cfm = (CmDmLeSetPathLossReportingEnableCfm *)CsrPmemZalloc(sizeof(*cfm));
    DM_ULP_SET_PATH_LOSS_REPORTING_ENABLE_CFM_T *dmPrim = (DM_ULP_SET_PATH_LOSS_REPORTING_ENABLE_CFM_T *)cmData->recvMsgP;

    cfm->type      = CM_DM_LE_SET_PATH_LOSS_REPORTING_ENABLE_CFM;
    cfm->tpAddrt   = dmPrim->tp_addrt;
    if(dmPrim->status == HCI_SUCCESS)
    {
        cfm->resultCode        = CSR_BT_RESULT_CODE_CM_SUCCESS;
        cfm->resultSupplier    = CSR_BT_SUPPLIER_CM;
    }
    else
    {
        cfm->resultCode        = dmPrim->status;
        cfm->resultSupplier    = CSR_BT_SUPPLIER_HCI;
    }
    CsrBtCmPutMessage(cmData->dmVar.appHandle, cfm);
    CsrBtCmDmLocalQueueHandler();
}

void CmDmLePathLossThresholdIndHandler(cmInstanceData_t *cmData)
{
    CmDmLePathLossThresholdInd *ind = (CmDmLePathLossThresholdInd *)CsrPmemZalloc(sizeof(*ind));
    DM_ULP_PATH_LOSS_THRESHOLD_IND_T *dm_prim = (DM_ULP_PATH_LOSS_THRESHOLD_IND_T *)cmData->recvMsgP;

    ind->type = CM_DM_LE_PATH_LOSS_THRESHOLD_IND;
    ind->tpAddrt = dm_prim->tp_addrt;
    ind->currPathLoss = dm_prim->curr_path_loss;
    ind->zoneEntered = dm_prim->zone_entered;

    CsrBtCmPutMessage(CSR_BT_CM_LE_HANDLE(cmData), ind);
}

#endif /* INSTALL_CM_LE_PATH_LOSS_REPORTING */
