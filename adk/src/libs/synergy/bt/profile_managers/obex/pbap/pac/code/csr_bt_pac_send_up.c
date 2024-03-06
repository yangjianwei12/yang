/******************************************************************************
 Copyright (c) 2014-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

#include "csr_bt_pac_handler.h"

static void csrBtPacCopyVersionInfo(PacInst* pInst, CsrBtPbVersionInfo *pVersionInfo)
{
    CsrUint8 size;
    CsrUint8 folderId = pInst->targetFolderId >> 8;

    if ((folderId & CSR_BT_PB_PB_ID)
        || (folderId & CSR_BT_PB_FAV_ID)
        || (folderId & CSR_BT_PB_SPD_ID))
    {
        size = sizeof(CsrBtPbVersionInfo);
    }
    else
    {
        size = sizeof(pVersionInfo->databaseIdentifier)
               + sizeof(pVersionInfo->primaryVersionCounter);
    }

    CsrMemCpy(pVersionInfo, &pInst->versionInfo, size);
}

void CsrBtPacPullPbCfmSend(PacInst *pInst,
                           CsrBtObexResponseCode responseCode)
{
    CsrBtPacPullPbCfm *pMsg = CsrPmemZalloc(sizeof(CsrBtPacPullPbCfm));

    pMsg->type          = CSR_BT_PAC_PULL_PB_CFM;
    pMsg->newMissedCall = pInst->newMissedCall;
    pMsg->pbSize        = pInst->pbSize;
    pMsg->responseCode  = responseCode;
    pMsg->instanceId    = pInst->pacInstanceId;
    csrBtPacCopyVersionInfo(pInst, &pMsg->versionInfo);
    CsrBtPacMessagePut(pInst->appHandle, pMsg);
}

void CsrBtPacPullPbIndSend(PacInst *pInst,
                           CsrUint16 bodyOffset,
                           CsrUint16 bodyLength,
                           CsrUint8 * obexPacket,
                           CsrUint16 obexPacketLength)
{
    CsrBtPacPullPbInd *pMsg = CsrPmemZalloc(sizeof(CsrBtPacPullPbInd));

    pMsg->type          = CSR_BT_PAC_PULL_PB_IND;
    pMsg->bodyLength    = bodyLength;
    pMsg->bodyOffset    = bodyOffset;
    pMsg->payload       = obexPacket;
    pMsg->payloadLength = obexPacketLength;
    pMsg->instanceId    = pInst->pacInstanceId;
    CsrBtPacMessagePut(pInst->appHandle, pMsg);
}

void CsrBtPacPullvCardEntryCfmSend(PacInst *pInst,
                                   CsrBtObexResponseCode responseCode)
{
    CsrBtPacPullVcardEntryCfm *pMsg = CsrPmemZalloc(sizeof(CsrBtPacPullVcardEntryCfm));

    pMsg->type         = CSR_BT_PAC_PULL_VCARD_ENTRY_CFM;
    pMsg->responseCode = responseCode;
    pMsg->instanceId   = pInst->pacInstanceId;
    CsrMemCpy(pMsg->databaseId,
              pInst->versionInfo.databaseIdentifier,
              sizeof(pInst->versionInfo.databaseIdentifier));
    CsrBtPacMessagePut(pInst->appHandle, pMsg);
}

void CsrBtPacPullvCardEntryIndSend(PacInst *pInst,
                                   CsrUint16 bodyOffset,
                                   CsrUint16 bodyLength,
                                   CsrUint8 * obexPacket,
                                   CsrUint16 obexPacketLength)

{
    CsrBtPacPullVcardEntryInd *pMsg = CsrPmemZalloc(sizeof(CsrBtPacPullVcardEntryInd));

    pMsg->type          = CSR_BT_PAC_PULL_VCARD_ENTRY_IND;
    pMsg->bodyLength    = bodyLength;
    pMsg->bodyOffset    = bodyOffset;
    pMsg->payload       = obexPacket;
    pMsg->payloadLength = obexPacketLength;
    pMsg->instanceId    = pInst->pacInstanceId;
    CsrBtPacMessagePut(pInst->appHandle, pMsg);
}

void CsrBtPacPullvCardListIndSend(PacInst *pInst,
                                  CsrUint16 bodyOffset,
                                  CsrUint16 bodyLength,
                                  CsrUint8 * obexPacket,
                                  CsrUint16 obexPacketLength)
{
    CsrBtPacPullVcardListInd *pMsg = CsrPmemZalloc(sizeof(CsrBtPacPullVcardListInd));

    pMsg->type          = CSR_BT_PAC_PULL_VCARD_LIST_IND;
    pMsg->bodyLength    = bodyLength;
    pMsg->bodyOffset    = bodyOffset;
    pMsg->payload       = obexPacket;
    pMsg->payloadLength = obexPacketLength;
    pMsg->instanceId    = pInst->pacInstanceId;
    CsrBtPacMessagePut(pInst->appHandle, pMsg);
}

void CsrBtPacPullvCardListCfmSend(PacInst *pInst,
                                  CsrBtObexResponseCode responseCode)
{
    CsrBtPacPullVcardListCfm *pMsg = CsrPmemZalloc(sizeof(CsrBtPacPullVcardListCfm));

    pMsg->type          = CSR_BT_PAC_PULL_VCARD_LIST_CFM;
    pMsg->newMissedCall = pInst->newMissedCall;
    pMsg->pbSize        = pInst->pbSize;
    pMsg->responseCode  = responseCode;
    pMsg->instanceId    = pInst->pacInstanceId;
    csrBtPacCopyVersionInfo(pInst, &pMsg->versionInfo);
    CsrBtPacMessagePut(pInst->appHandle, pMsg);
}


void CsrBtPacSetFolderCfmSend(PacInst* pInst,
                              CsrBtObexResponseCode responseCode)
{
    CsrBtPacSetFolderCfm* pMsg = CsrPmemZalloc(sizeof(CsrBtPacSetFolderCfm));

    pMsg->type         = CSR_BT_PAC_SET_FOLDER_CFM;
    pMsg->responseCode = responseCode;
    pMsg->instanceId   = pInst->pacInstanceId;
    CsrBtPacMessagePut(pInst->appHandle, pMsg);
}

void CsrBtPacSetBackFolderCfmSend(PacInst* pInst,
                                  CsrBtObexResponseCode responseCode)
{
    CsrBtPacSetBackFolderCfm* pMsg = CsrPmemZalloc(sizeof(CsrBtPacSetBackFolderCfm));

    pMsg->type         = CSR_BT_PAC_SET_BACK_FOLDER_CFM;
    pMsg->responseCode = responseCode;
    pMsg->instanceId   = pInst->pacInstanceId;
    CsrBtPacMessagePut(pInst->appHandle, pMsg);
}

void CsrBtPacSetRootFolderCfmSend(PacInst* pInst,
                                  CsrBtObexResponseCode responseCode)
{
    CsrBtPacSetRootFolderCfm* pMsg = CsrPmemZalloc(sizeof(CsrBtPacSetRootFolderCfm));

    pMsg->type         = CSR_BT_PAC_SET_ROOT_FOLDER_CFM;
    pMsg->responseCode = responseCode;
    pMsg->instanceId   = pInst->pacInstanceId;
    CsrBtPacMessagePut(pInst->appHandle, pMsg);
}

void CsrBtPacAbortCfmSend(PacInst* pInst)
{
    CsrBtPacAbortCfm* pMsg = CsrPmemZalloc(sizeof(CsrBtPacAbortCfm));

    pMsg->type       = CSR_BT_PAC_ABORT_CFM;
    pMsg->instanceId = pInst->pacInstanceId;
    CsrBtPacMessagePut(pInst->appHandle, pMsg);
}

#ifdef INSTALL_PAC_CUSTOM_SECURITY_SETTINGS
void CsrBtPacSecurityOutCfmSend(PacInst *pInst,
                                CsrBtPacSecurityOutReq* prim,
                                CsrBtResultCode result)
{
    CsrBtPacSecurityOutCfm* pMsg = CsrPmemZalloc(sizeof(CsrBtPacSecurityOutCfm));

    pMsg->type           = CSR_BT_PAC_SECURITY_OUT_CFM;
    pMsg->resultCode     = result;
    pMsg->resultSupplier = CSR_BT_SUPPLIER_OBEX_PROFILES;
    pMsg->instanceId     = pInst->pacInstanceId;
    CsrBtPacMessagePut(prim->appHandle, pMsg);
}
#endif

void CsrBtPacConnectCfmSend(PacInst*        pInst,
                            CsrBtResultCode resultCode,
                            CsrBtSupplier   resultSupplier,
                            CsrUint16       maxPeerObexPacketLength,
                            CsrBtConnId     cid,
                            CsrSchedQid     appHandle)
{
    CsrBtPacConnectCfm* pMsg = CsrPmemZalloc(sizeof(CsrBtPacConnectCfm));

    pMsg->type                  = CSR_BT_PAC_CONNECT_CFM;
    pMsg->resultCode            = resultCode;
    pMsg->resultSupplier        = resultSupplier;
    pMsg->obexPeerMaxPacketSize = maxPeerObexPacketLength;
    pMsg->supportedRepositories = pInst->supportedRepositories;
    pMsg->supportedFeatures     = pInst->supportedFeatures;
    pMsg->btConnId              = cid;
    pMsg->instanceId            = pInst->pacInstanceId;
    CsrBtPacMessagePut(appHandle, pMsg);
}

void CsrBtPacAuthenticateIndSend(PacInst* pInst,
                                 const CsrBtDeviceAddr* deviceAddr,
                                 CsrUint8 options,
                                 CsrUint16 realmLength,
                                 CsrUint8* realm)
{
    CsrBtPacAuthenticateInd* pMsg = CsrPmemZalloc(sizeof(CsrBtPacAuthenticateInd));

    pMsg->type        = CSR_BT_PAC_AUTHENTICATE_IND;
    pMsg->deviceAddr  = *deviceAddr;
    pMsg->options     = options;
    pMsg->realmLength = realmLength;
    pMsg->realm       = realm;
    pMsg->instanceId  = pInst->pacInstanceId;
    CsrBtPacMessagePut(pInst->appHandle, pMsg);
}

void CsrBtPacDisconnectIndSend(PacInst* pInst,
                               CsrBtReasonCode reasonCode,
                               CsrBtSupplier reasonSupplier)
{
    CsrBtPacDisconnectInd* pMsg = CsrPmemZalloc(sizeof(CsrBtPacDisconnectInd));

    pMsg->type           = CSR_BT_PAC_DISCONNECT_IND;
    pMsg->reasonCode     = reasonCode;
    pMsg->reasonSupplier = reasonSupplier;
    pMsg->instanceId     = pInst->pacInstanceId;
    CsrBtPacMessagePut(pInst->appHandle, pMsg);
}

void PacGetInstanceIdsCfmSend(PacInst* pInst,
                              PacGetInstanceIdsReq *req)
{
    PacGetInstanceIdsCfm *pMsg = CsrPmemZalloc(sizeof(PacGetInstanceIdsCfm));
    CsrUint8 instCount         = 1;
    CsrUint8 instSizeToCpy     = sizeof(CsrSchedQid) * instCount;
    void *instPtr              = (void *)&pInst->pacInstanceId;

#ifdef INSTALL_PAC_MULTI_INSTANCE_SUPPORT
    if (pInst->pacInstances != NULL)
    {
        instCount     = pInst->pacInstances->numberInPool;
        instSizeToCpy = sizeof(CsrSchedQid) * instCount;
        instPtr = (void *) (pInst->pacInstances->phandles);
    }
#endif

    pMsg->type            = PAC_GET_INSTANCE_IDS_CFM;
    pMsg->instanceCount   = instCount;
    pMsg->instanceIdsList = (CsrSchedQid *)CsrPmemAlloc(instSizeToCpy);
    CsrMemCpy(pMsg->instanceIdsList, instPtr, instSizeToCpy);

    CsrBtPacMessagePut(req->appHandle, pMsg);
}

