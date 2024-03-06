/******************************************************************************
 Copyright (c) 2008-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_cm_main.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_util.h"
#include "csr_log_text_2.h"

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_INQUIRY_PAGE_STATE
#include "csr_bt_cm_events_handler.h"
#endif

static void csrBtCmCopyTheRemoteName(cmInstanceData_t *cmData, CsrBtDeviceName theName)
{
    CsrUintFast16                      i,j;
    CsrUint8                          *ptr;
    DM_HCI_REMOTE_NAME_CFM_T          *dmPrim;

    CsrUint16 nNameLen   = 0;
    CsrBool  bNullFound  = FALSE;
    dmPrim              = (DM_HCI_REMOTE_NAME_CFM_T *)cmData->recvMsgP;

    for ( i = 0 ; ( ( i < HCI_LOCAL_NAME_BYTE_PACKET_PTRS ) && ( nNameLen < CSR_BT_MAX_FRIENDLY_NAME_LEN ) ) ; i++ )
    {
        ptr = dmPrim->name_part[ i ];
        for ( j = 0 ; ( ( j < HCI_LOCAL_NAME_BYTES_PER_PTR ) && ( nNameLen < CSR_BT_MAX_FRIENDLY_NAME_LEN ) ) ; j++ )
        {
            if ( *ptr == '\0' )
            {
                bNullFound = TRUE;
                break;
            }
            theName[nNameLen++] = *ptr++;
        } /*  end for (j ...) */
        /* quit loop if we got to the end of the string */
        if ( bNullFound )
        {
            break;
        }
    } /*  end for (i = .... */
    /* terminate the string */
    CsrUtf8StrTruncate(theName, nNameLen);
}

void CsrBtCmReadRemoteNameCfmSend(cmInstanceData_t *cmData, CsrSchedQid phandle,
             CsrBtDeviceAddr deviceAddr, CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    CsrBtCmReadRemoteNameCfm *cmPrim = (CsrBtCmReadRemoteNameCfm *)CsrPmemAlloc(sizeof(CsrBtCmReadRemoteNameCfm));
    cmPrim->type                     = CSR_BT_CM_READ_REMOTE_NAME_CFM;
    cmPrim->deviceAddr               = deviceAddr;
    cmPrim->resultCode               = resultCode;
    cmPrim->resultSupplier           = resultSupplier;

    /* Sending confirmation to the application indicates that the read remote name request
     * procedure is completed, hence clearing the device address here.*/
    CsrBtBdAddrZero(&cmData->dmVar.readNameDeviceAddr);

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_INQUIRY_PAGE_STATE
    if (cmData->dmVar.pagingInProgress == TRUE)
    {
        cmData->dmVar.pagingInProgress = FALSE;
        CsrBtCmPropgateEvent(cmData,
                             CsrBtCmPropagateInquiryPageEvents,
                             CSR_BT_CM_EVENT_MASK_SUBSCRIBE_INQUIRY_PAGE_STATE,
                             HCI_SUCCESS,
                             NULL,
                             NULL);

        CSR_LOG_TEXT_INFO((CsrBtCmLto, 0, "Paging Stopped"));
    }
#endif

    if (resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && resultSupplier == CSR_BT_SUPPLIER_CM)
    {
        CsrBtDeviceName tmpnam;
        csrBtCmCopyTheRemoteName(cmData, tmpnam);
        cmPrim->friendlyName = CsrUtf8StrDup((CsrUtf8String*)&tmpnam);
    }
    else
    {
        cmPrim->friendlyName         = CsrUtf8StrDup((const CsrUtf8String*)"");
    }
    CsrBtCmPutMessage(phandle, cmPrim);
}

void CsrBtCmReadRemoteNameReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmReadRemoteNameReq *cmPrim = (CsrBtCmReadRemoteNameReq*) cmData->recvMsgP;

    cmData->dmVar.appHandle = cmPrim->phandle;
    CsrBtBdAddrCopy(&cmData->dmVar.readNameDeviceAddr, &cmPrim->deviceAddr);

    if (!CmDuHandleAutomaticProcedure(cmData,
                                      CM_DU_AUTO_EVENT_RNR,
                                      NULL,
                                      &cmPrim->deviceAddr))
    {
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_INQUIRY_PAGE_STATE
        aclTable *aclConnectionElement;
        returnAclConnectionElement(cmData, cmData->dmVar.readNameDeviceAddr, &aclConnectionElement);

        /* Check ACL is up or not */
        if (!aclConnectionElement)
        {
            cmData->dmVar.pagingInProgress = TRUE;
            CsrBtCmPropgateEvent(cmData,
                                 CsrBtCmPropagateInquiryPageEvents,
                                 CSR_BT_CM_EVENT_MASK_SUBSCRIBE_INQUIRY_PAGE_STATE,
                                 HCI_SUCCESS,
                                 NULL,
                                 NULL);
            CSR_LOG_TEXT_INFO((CsrBtCmLto, 0, "Paging Started"));
        }
#endif
        dm_hci_remote_name_request(&cmData->dmVar.readNameDeviceAddr, NULL);
    }
}

void CsrBtCmCancelReadRemoteNameReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmCancelReadRemoteNameReq *prim;

    prim = (CsrBtCmCancelReadRemoteNameReq*) cmData->recvMsgP;

    if (cmData->dmVar.lockMsg == CSR_BT_CM_READ_REMOTE_NAME_REQ &&
        CsrBtBdAddrEq(&cmData->dmVar.readNameDeviceAddr, &prim->deviceAddr) &&
        cmData->dmVar.appHandle == prim->appHandle)
    { /* Read remote name request under process */
#if defined(INSTALL_CM_DEVICE_UTILITY) && defined(CSR_BT_INSTALL_CM_CACHE_PARAMS)
        if (!cmData->dmVar.readingName)
        {
            /* Read remote name command has not been sent yet.
             * Wait for application of cache parameters */
            cmData->dmVar.cancel = TRUE;
        }
        else
#endif /* INSTALL_CM_DEVICE_UTILITY && CSR_BT_INSTALL_CM_CACHE_PARAMS */
        {
            dm_hci_remote_name_req_cancel(&prim->deviceAddr, NULL);
        }
    }
    else
    { /* Read remote name request may be in stored message queue */
        if (cancelDmMsg(cmData,
                        CSR_BT_CM_READ_REMOTE_NAME_REQ,
                        prim->appHandle,
                        prim->deviceAddr))
        {
            CsrBtCmReadRemoteNameCfmSend(cmData,
                                         prim->appHandle,
                                         prim->deviceAddr,
                                         CSR_BT_RESULT_CODE_CM_CANCELLED,
                                         CSR_BT_SUPPLIER_CM);
        }
        /* Else, the CFM has already been sent, or cancel is sent without name request. Ignore in both cases. */
    }
}

void CsrBtCmDmHciRemoteNameCompleteHandler(cmInstanceData_t *cmData)
{ /* Notification of remote name is received. Read the name and restore the
     local DM queue */
    CsrUintFast16 i;
    DM_HCI_REMOTE_NAME_CFM_T *dmPrim = (DM_HCI_REMOTE_NAME_CFM_T *)cmData->recvMsgP;

    if (dmPrim->status == HCI_SUCCESS && 
        cmData->dmVar.appHandle != CSR_BT_CM_SD_HANDLE(cmData) &&
        CSR_BT_CM_SD_HANDLE(cmData) != CSR_SCHED_QID_INVALID)
    { /* The attempt to find a remote device is a success, and it is not
         initiated by the SD module */
        CsrBtDeviceName tmpnam;

        csrBtCmCopyTheRemoteName(cmData, tmpnam);
        if (tmpnam[0] != '\0')
        { /* A valid name is found, pass this information to the SD module */
            CsrBtCmReadRemoteNameInd *sdPrim;

            sdPrim = (CsrBtCmReadRemoteNameInd *) CsrPmemAlloc(sizeof(CsrBtCmReadRemoteNameInd));
            sdPrim->type         = CSR_BT_CM_READ_REMOTE_NAME_IND;
            sdPrim->deviceAddr   = dmPrim->bd_addr;
            sdPrim->friendlyName = CsrUtf8StrDup(tmpnam);
            CsrBtCmPutMessage(CSR_BT_CM_SD_HANDLE(cmData), sdPrim);
        }
    }

    if (!CmDuHandleCommandComplete(cmData, CM_DU_CMD_COMPLETE_RNR))
    {
        /* Event is not handled by device utility, continue processing here.*/
        CsrBtCmReadRemoteNameCfmSend(cmData,
                                     cmData->dmVar.appHandle,
                                     dmPrim->bd_addr,
                                     dmPrim->status == HCI_SUCCESS ? CSR_BT_RESULT_CODE_CM_SUCCESS: (CsrBtResultCode) dmPrim->status,
                                     dmPrim->status == HCI_SUCCESS ? CSR_BT_SUPPLIER_CM: CSR_BT_SUPPLIER_HCI);
        CsrBtCmDmLocalQueueHandler();
    }

    for ( i = 0 ; i < HCI_LOCAL_NAME_BYTE_PACKET_PTRS ; i++ )
    {
        /* In order to prevent a memory leak, the memory associated with pointers
         * are CsrPmemFree.*/
        CsrPmemFree(dmPrim->name_part[i]);
        dmPrim->name_part[i] = NULL;
    }
}

