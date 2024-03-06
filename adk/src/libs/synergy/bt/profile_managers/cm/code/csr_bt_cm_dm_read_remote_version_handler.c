/******************************************************************************
 Copyright (c) 2008-2020 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_cm_main.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_util.h"

static void csrBtCmSaveRemoteVersionHandler(cmInstanceData_t *cmData,
                                            CsrBtDeviceAddr  deviceAddr,
                                            CsrUint8         lmpVersion,
                                            CsrUint16        manufacturerName,
                                            CsrUint16        lmpSubversion,
                                            CsrUint8         hciStatus)
{
    if (hciStatus == HCI_SUCCESS)
    {
        aclTable *aclConnectionElement;
        returnAclConnectionElement(cmData, deviceAddr, &aclConnectionElement);

        if (aclConnectionElement)
        {
            aclConnectionElement->lmpVersion        = lmpVersion;
            aclConnectionElement->manufacturerName  = manufacturerName;
            aclConnectionElement->lmpSubversion     = lmpSubversion;
        }
    }
}

static void csrBtCmReadRemoteVersionReqKick(cmInstanceData_t *cmData)
{
    cmPendingMsg_t *pendingMsg = (cmPendingMsg_t *) CSR_BT_CM_PENDING_MSG_FIND_TYPE(cmData->pendingMsgs,
                                                                                    CSR_BT_CM_PENDING_MSG_REMOTE_VERSION);

    if (pendingMsg)
    {
        remoteVersionReq *req = (remoteVersionReq *) &pendingMsg->arg.remoteVer;
        TP_BD_ADDR_T tpAddr;

        /* Send next request if list isn't empty */
        tpAddr.addrt.addr = req->addr;
        tpAddr.addrt.type = req->addressType;
        tpAddr.tp_type = req->transportType;

        dm_hci_read_remote_version(&tpAddr, NULL);
    }
}

void CsrBtCmDmHciReadRemoteVersionCompleteHandler(cmInstanceData_t *cmData)
{
    DM_HCI_READ_REMOTE_VER_INFO_CFM_T *dmPrim = (DM_HCI_READ_REMOTE_VER_INFO_CFM_T *) cmData->recvMsgP;
    cmPendingMsg_t *pendingMsg = (cmPendingMsg_t *) CSR_BT_CM_PENDING_MSG_FIND_TYPE(cmData->pendingMsgs,
                                                                                    CSR_BT_CM_PENDING_MSG_REMOTE_VERSION);


#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_REMOTE_VERSION
    CsrBtCmPropgateEvent(cmData,
                         CsrBtCmPropgateReadRemoteVersionEvents,
                         CSR_BT_CM_EVENT_MASK_SUBSCRIBE_REMOTE_VERSION,
                         dmPrim->status,
                         dmPrim,
                         NULL);
#endif

    if (pendingMsg)
    {
        remoteVersionReq *req = (remoteVersionReq*) &pendingMsg->arg.remoteVer;

        if (req->appHandle != CSR_BT_CM_IFACEQUEUE)
        {
            CsrBtCmReadRemoteVersionCfm *cmPrim;

            cmPrim = (CsrBtCmReadRemoteVersionCfm *)CsrPmemAlloc(sizeof(CsrBtCmReadRemoteVersionCfm));
            cmPrim->type                = CSR_BT_CM_READ_REMOTE_VERSION_CFM;
            cmPrim->deviceAddr          = dmPrim->tp_addrt.addrt.addr;
            cmPrim->addressType         = dmPrim->tp_addrt.addrt.type;
            cmPrim->transportType       = dmPrim->tp_addrt.tp_type;
            cmPrim->lmpSubversion       = dmPrim->LMP_subversion;
            cmPrim->lmpVersion          = dmPrim->LMP_version;
            cmPrim->manufacturerName    = dmPrim->manufacturer_name;

            if (dmPrim->status == HCI_SUCCESS)
            {
                cmPrim->resultCode      = CSR_BT_RESULT_CODE_CM_SUCCESS;
                cmPrim->resultSupplier  = CSR_BT_SUPPLIER_CM;
            }
            else
            {
                cmPrim->resultCode      = (CsrBtResultCode) dmPrim->status;
                cmPrim->resultSupplier  = CSR_BT_SUPPLIER_HCI;
            }

            CsrBtCmPutMessage(req->appHandle, cmPrim);
        }

        CsrPCmnListRemove(cmData->pendingMsgs, (CsrCmnListElm_t *) pendingMsg);
    }
    else
    {
        /* The Cm has issue this request. */
        ;
    }

    csrBtCmSaveRemoteVersionHandler(cmData,
                                    dmPrim->tp_addrt.addrt.addr,
                                    dmPrim->LMP_version,
                                    dmPrim->manufacturer_name,
                                    dmPrim->LMP_subversion,
                                    dmPrim->status);

    /* Fire off next pending request */
    csrBtCmReadRemoteVersionReqKick(cmData);
}

void CsrBtCmReadRemoteVersionReqHandler(cmInstanceData_t *cmData)
{
    /* Add to queue. Kicker will ensure next pending request is
     * sent */
    remoteVersionReq *qe;
    cmPendingMsg_t *pendingMsg;
    CsrBtCmReadRemoteVersionReq *cmPrim = (CsrBtCmReadRemoteVersionReq *) cmData->recvMsgP;

    CsrPCmnListAddLast(cmData->pendingMsgs, sizeof(cmPendingMsg_t), pendingMsg);
    qe = (remoteVersionReq *) &pendingMsg->arg.remoteVer;

    pendingMsg->type = CSR_BT_CM_PENDING_MSG_REMOTE_VERSION;

    qe->appHandle = cmPrim->appHandle;
    qe->addr = cmPrim->deviceAddr;
    qe->addressType = cmPrim->addressType;
    qe->transportType = cmPrim->transportType;

    csrBtCmReadRemoteVersionReqKick(cmData);
}

