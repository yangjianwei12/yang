/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#ifndef EXCLUDE_CSR_BT_SPP_MODULE
#include "csr_sched.h"
#include "csr_pmem.h"
#include "csr_bt_result.h"
#include "bluetooth.h"
#include "hci_prim.h"
#include "csr_bt_cm_lib.h"
#include "csr_bt_cm_private_lib.h"
#ifndef EXCLUDE_CSR_BT_SC_MODULE
#include "csr_bt_sc_private_lib.h"
#endif
#include "csr_bt_util.h"
#ifndef EXCLUDE_CSR_AM_MODULE
#ifdef CSR_BT_INSTALL_SPP_EXTENDED
#include "csr_am_lib.h"
#endif
#endif /* !EXCLUDE_CSR_AM_MODULE */
#include "csr_log_text_2.h"

#include "csr_bt_spp_main.h"
#include "csr_bt_spp_prim.h"
#include "csr_bt_spp_sef.h"
#include "csr_bt_spp_lib.h"
#include "sds_prim.h"
#include "csr_bt_sdc_support.h"
#include "csr_env_prim.h"

#include "csr_bt_cmn_sdc_rfc_util.h"
#include "csr_bt_cmn_sdr_tagbased_lib.h"
#include "csr_bt_cmn_sdp_lib.h"
#include "csr_unicode.h"


#define SPP_SERVER_CHANNEL_INDEX            24
#define SPP_SERVER_SERVICE_NAME_LEN_INDEX   50
#define SPP_SERVER_SERVICE_NAME_INDEX       51

static const CsrUint8 sdsSppServiceRecord[] =
{
    /* Service class ID list */
    0x09,0x00,0x01,        /* AttrID , ServiceClassIDList */
    0x35,0x03,             /* 3 bytes in total DataElSeq */
    0x19,0x11,0x01,        /* 2 byte UUID, Service class = SerialPortProfile = 0x1101 */

    /* protocol descriptor list */
    0x09,0x00,0x04,        /* AttrId ProtocolDescriptorList */
    0x35,0x0C,             /* 12 bytes in total DataElSeq */
    /* L2CAP */
    0x35,0x03,             /* 3 bytes in DataElSeq */
    0x19,0x01,0x00,        /* 2 byte UUID, Protocol = L2CAP */
    /* RFCOMM */
    0x35,0x05,             /* 5 bytes in DataElSeq */
    0x19,0x00,0x03,        /* 2 byte UUID Protocol = RFCOMM */
    0x08,0x00,             /* 1 byte UINT - server channel template value 0 - to be filled in by app (index:24) */

    /* BrowseGroupList    */
    0x09, 0x00, 0x05,      /* AttrId = BrowseGroupList */
    0x35, 0x03,            /* Data element seq. 3 bytes */
    0x19, 0x10, 0x02,      /* 2 byte UUID, PublicBrowseGroup = 0x1002 */

    /* Bluetooth Profile Descriptor List */
    0x09,0x00,0x09,                    /* AttrId, BluetoothProfileDescriptorList */
    0x35,0x08,                         /* 8 bytes in total DataElSeq */
    0x35,0x06,                         /* 6 bytes in total DataElSeq */
    0x19,0x11,0x01,                    /* 2 byte UUID, Service class = SPP */
    0x09,0x01,0x02,                    /* 2 byte UINT, Profile Version = 0x0102 */

    /* service name */
    0x09,0x01,0x00,        /* AttrId - Service Name. Use language base attribute 0x0100 (primary language) */
    0x25, 0                /* length of service string, default 1 */
    /* string - empty string as default. Use index defines to change */
};

#ifdef CSR_BT_GLOBAL_INSTANCE
/***********************************************************************************
    SPP Utility functions.
    Used by SPP to store/remove instance pointer in the SPP manager.
***********************************************************************************/
static void sppStoreInstanceInSppManager(SppInstanceData_t *instData)
{
    CsrUint8 i;
    /* Get the index from the phandle list */
    for (i = 0; i < sppInstanceData.numberOfSppInstances; i++)
    {
        if (instData && instData->myAppHandle == sppInstanceData.sppInstances->phandles[i])
        {
            sppInstanceData.sppInstances->connInstPtrs[i] = (SppInstanceData_t *) instData;
        }
    }
}

static void sppRemoveInstanceInSppManager(SppInstanceData_t *instData)
{
    CsrUint8 i;
    /* Get the index from the phandle list */
    for (i = 0; i < sppInstanceData.numberOfSppInstances; i++)
    {
        if (instData && instData->myAppHandle == sppInstanceData.sppInstances->phandles[i])
        {
            sppInstanceData.sppInstances->connInstPtrs[i] = NULL;
        }
    }
}
#endif /* CSR_BT_GLOBAL_INSTANCE */
/***********************************************************************************
    Internal message factory functions.
    Used by SPP when sending messages internally or to higher layer.
***********************************************************************************/
void CsrBtSppMessagePut(CsrSchedQid phandle, void *msg)
{
    CsrSchedMessagePut(phandle, CSR_BT_SPP_PRIM, msg);
}

#ifdef INSTALL_SPP_OUTGOING_CONNECTION
static void csrSppRfcResultHandler(CsrSdcOptCallbackType cbType, void *context)
{
    switch(cbType)
    {
        case CSR_SDC_OPT_CB_CON_SELECT_SERVICE_HANDLE:
        {
            CsrRfcConSelectServiceHandleType *params = (CsrRfcConSelectServiceHandleType *) context;

            CsrBtSppSdcSelectServiceHandler(params->instData,
                                            params->cmSdcRfcInstData,
                                            params->deviceAddr,
                                            params->serverChannel,
                                            params->entriesInSdpTaglist,
                                            params->sdpTagList);
        }
        break;

        case CSR_SDC_OPT_RFC_CON_RESULT:
        {
            CsrRfcConResultType *params = (CsrRfcConResultType *) context;

            CsrBtSppRfcSdcConResultHandler(params->instData,
                                           params->localServerCh,
                                           params->btConnId,
                                           params->deviceAddr,
                                           params->maxFrameSize,
                                           params->validPortPar,
                                           *(params->portPar),
                                           params->resultCode,
                                           params->resultSupplier,
                                           params->sdpTag);
        }
        break;

        default:
            break;
    }
}
#endif /* INSTALL_SPP_OUTGOING_CONNECTION */

static void csrBtSppHouseCleaningSend(SppInstanceData_t *instData)
{
    CsrBtSppHouseCleaning    *sppPrim;

    sppPrim = (CsrBtSppHouseCleaning *) CsrPmemAlloc(sizeof(CsrBtSppHouseCleaning));
    sppPrim->type = CSR_BT_SPP_HOUSE_CLEANING;
    CsrBtSppMessagePut(instData->myAppHandle, sppPrim);
}

static void sdsRegisterReqSend(SppInstanceData_t *instData)
{
    CsrSize    nameLen;
    CsrUint16    num_rec_bytes;
    CsrUint8        *record;

    if (instData->serviceName == NULL)
    {
        nameLen = 0;
    }
    else
    {
        nameLen = CsrStrLen((char *)(instData->serviceName));
    }
    /* register the record in SDS */
#ifndef EXCLUDE_CSR_BT_SPP_EXTENDED_ACTIVATE_REQ
    if (!instData->extendedActivationData)
#endif /* !EXCLUDE_CSR_BT_SPP_EXTENDED_ACTIVATE_REQ */
    {
        num_rec_bytes = (CsrUint16)(sizeof(sdsSppServiceRecord) + nameLen);
        record = (CsrUint8 *) CsrPmemAlloc(num_rec_bytes );
        CsrMemCpy(record, sdsSppServiceRecord, num_rec_bytes);
        record[SPP_SERVER_CHANNEL_INDEX] = instData->serverChannel;
        record[SPP_SERVER_SERVICE_NAME_LEN_INDEX] = (CsrUint8) nameLen;
        if (nameLen != 0)
        {
            CsrMemCpy( &(record[SPP_SERVER_SERVICE_NAME_INDEX]), instData->serviceName, nameLen );
        }
    }
#ifndef EXCLUDE_CSR_BT_SPP_EXTENDED_ACTIVATE_REQ
    else
    {
        num_rec_bytes = instData->extendedActivationData->serviceRecordSize;
        record = (CsrUint8 *) CsrPmemAlloc(num_rec_bytes);
        CsrMemCpy(record, instData->extendedActivationData->serviceRecord, num_rec_bytes);
        record[instData->extendedActivationData->serviceRecordSrvChIndex] = instData->serverChannel;
    }
#endif /* !EXCLUDE_CSR_BT_SPP_EXTENDED_ACTIVATE_REQ */

    CsrBtCmSdsRegisterReqSend(instData->myAppHandle, record, num_rec_bytes, CSR_BT_CM_CONTEXT_UNUSED);
}

static void csrBtSppConnectIndSend(SppInstanceData_t *instData, CsrBtDeviceAddr deviceAddr,
                                   CsrUint16 profileMaxFrameSize, CsrBool validPortPar, RFC_PORTNEG_VALUES_T portPar,
                                   CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    CsrBtSppConnectInd    *sppPrim;
#ifdef CSR_BT_INSTALL_SPP_EXTENDED
    instData->extendedConnect       = FALSE;
#endif /* CSR_BT_INSTALL_SPP_EXTENDED */
    sppPrim                         = (CsrBtSppConnectInd *) CsrPmemAlloc(sizeof(CsrBtSppConnectInd));
    sppPrim->type                   = CSR_BT_SPP_CONNECT_IND;
    sppPrim->deviceAddr             = deviceAddr;
    sppPrim->queueId                = instData->myAppHandle;
    sppPrim->serverChannel          = instData->serverChannel;
    sppPrim->profileMaxFrameSize    = profileMaxFrameSize;
    sppPrim->validPortPar           = validPortPar;
    sppPrim->portPar                = portPar;
    sppPrim->resultCode             = resultCode;
    sppPrim->resultSupplier         = resultSupplier;
    sppPrim->btConnId               = instData->sppConnId;

    CsrBtSppMessagePut(instData->ctrlHandle, sppPrim);

    instData->cancelReceived = FALSE;
}

static void csrBtSppActivateCfm(SppInstanceData_t *instData)
{
    CsrBtSppActivateCfm    *sppPrim;

    sppPrim                = (CsrBtSppActivateCfm *) CsrPmemAlloc(sizeof(CsrBtSppActivateCfm));
    sppPrim->type            = CSR_BT_SPP_ACTIVATE_CFM;
    sppPrim->queueId = instData->myAppHandle;
    sppPrim->serverChannel            = instData->serverChannel;
    CsrBtSppMessagePut(instData->ctrlHandle, sppPrim);
}

/* Data application send with fallback to control handle */
static void csrBtSppDataPathSend(SppInstanceData_t *instData, void *msg)
{
    CsrSchedQid target = instData->ctrlHandle;

#ifndef CSR_STREAMS_ENABLE
    target = instData->dataHandle != CSR_SCHED_QID_INVALID ? instData->dataHandle : instData->ctrlHandle;
#endif

    CsrBtSppMessagePut(target, msg);
}

/* Send CSR_BT_SPP_STATUS_IND to data handle */
static void csrBtSppStatusIndSend(SppInstanceData_t *instData, CsrBool connect, CsrUint16 maxMsgSize, CsrBtDeviceAddr addr)
{
#ifndef CSR_STREAMS_ENABLE
    if((instData->dataHandle != CSR_SCHED_QID_INVALID) && (instData->dataHandle != instData->ctrlHandle))
    {
        CsrBtSppStatusInd *prim;
        prim                = (CsrBtSppStatusInd*)CsrPmemAlloc(sizeof(CsrBtSppStatusInd));
        prim->type          = CSR_BT_SPP_STATUS_IND;
        prim->queueId = instData->myAppHandle;
        prim->serverChannel = instData->serverChannel;
        prim->connect       = connect;
        prim->maxMsgSize    = maxMsgSize;
        prim->deviceAddr    = addr;
        CsrBtSppMessagePut(instData->dataHandle, prim);
    }
#else
    CSR_UNUSED(instData);
    CSR_UNUSED(connect);
    CSR_UNUSED(maxMsgSize);
    CSR_UNUSED(addr);
#endif /* !CSR_STREAMS_ENABLE */
}

#ifndef CSR_STREAMS_ENABLE
/* Send CSR_BT_SPP_DATA_PATH_STATUS_IND to control app */
static void csrBtSppDataPathStatusIndSend(SppInstanceData_t *instData, CsrUint8 status)
{
    CsrBtSppDataPathStatusInd *prim;
    prim                   = (CsrBtSppDataPathStatusInd*)CsrPmemAlloc(sizeof(CsrBtSppDataPathStatusInd));
    prim->type             = CSR_BT_SPP_DATA_PATH_STATUS_IND;
    prim->queueId = instData->myAppHandle;
    prim->status           = status;
    CsrBtSppMessagePut(instData->ctrlHandle, prim);
}
#endif /* !CSR_STREAMS_ENABLE */

/***********************************************************************************
    Send a disconnect ind message to the registered higher layer
***********************************************************************************/
static void csrBtSppDisconnectIndSend(SppInstanceData_t *instData,
                                      CsrBtReasonCode reasonCode, CsrBtSupplier reasonSupplier,
                                      CsrBool localTerminated)
{
    CsrBtSppDisconnectInd *sppPrim;
    CsrBtDeviceAddr         addr;

    /* Send a status indication (disconnect) to the data handle before disconnect */
    CsrBtBdAddrZero(&addr);
    csrBtSppStatusIndSend(instData, FALSE, 0, addr);

    sppPrim = (CsrBtSppDisconnectInd *) CsrPmemAlloc(sizeof(CsrBtSppDisconnectInd));
    sppPrim->type = CSR_BT_SPP_DISCONNECT_IND;
    sppPrim->queueId = instData->myAppHandle;
    sppPrim->serverChannel = instData->serverChannel;
    sppPrim->deviceAddr = instData->bdAddr;
    sppPrim->reasonCode = reasonCode;
    sppPrim->reasonSupplier = reasonSupplier;
    sppPrim->localTerminated = localTerminated;

    CsrBtSppMessagePut(instData->ctrlHandle, sppPrim);
}

/***********************************************************************************
    Send a deactivation confirmation to higher layer
***********************************************************************************/
static void csrBtSppDeactivateCfmSend(SppInstanceData_t *instData,
                                      CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    CsrBtSppDeactivateCfm *sppPrim;

    sppPrim = (CsrBtSppDeactivateCfm *) CsrPmemAlloc(sizeof(CsrBtSppDeactivateCfm));
    sppPrim->type = CSR_BT_SPP_DEACTIVATE_CFM;
    sppPrim->phandle = instData->myAppHandle;
    sppPrim->resultCode = resultCode;
    sppPrim->resultSupplier = resultSupplier;

    CsrBtSppMessagePut(instData->ctrlHandle, sppPrim);
}

#ifndef EXCLUDE_CSR_AM_MODULE
#ifdef CSR_BT_INSTALL_SPP_EXTENDED
/***********************************************************************************
    Send a audio indication to higher layer
***********************************************************************************/
static void csrBtSppAudioIndSend(SppInstanceData_t *instData, CsrUint16 scoHandle, CsrUint8 linkType,
                                 CsrUint8 txInterval, CsrUint8 weSco, CsrUint16 rxPacketLength,
                                 CsrUint16 txPacketLength, CsrUint8 airMode, CsrUint8 pcmSlot,
                                 CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    CsrBtSppAudioConnectInd *sppPrim;

    sppPrim                 = (CsrBtSppAudioConnectInd *) CsrPmemAlloc(sizeof(CsrBtSppAudioConnectInd));
    if(instData->pendingSco)
    {
        instData->pendingSco = FALSE;
        sppPrim->type       = CSR_BT_SPP_AUDIO_CONNECT_CFM;
    }
    else
    {
        sppPrim->type       = CSR_BT_SPP_AUDIO_CONNECT_IND;
    }
    sppPrim->resultCode     = resultCode;
    sppPrim->resultSupplier = resultSupplier;
    sppPrim->queueId = instData->myAppHandle;
    sppPrim->serverChannel  = instData->serverChannel;
    sppPrim->scoHandle      = scoHandle;
    sppPrim->airMode        = airMode;
    sppPrim->linkType       = linkType;
    sppPrim->rxPacketLength = rxPacketLength;
    sppPrim->txPacketLength = txPacketLength;
    sppPrim->txInterval     = txInterval;
    sppPrim->weSco          = weSco;
    sppPrim->pcmSlot        = pcmSlot;
    CsrBtSppMessagePut(instData->ctrlHandle, sppPrim);
}

static void csrBtSppAudioRenegotiateCfmSend(SppInstanceData_t *instData,
                                            CsrBtResultCode resultCode,
                                            CsrBtSupplier resultSupplier,
                                            hci_connection_handle_t theScoHandle)
{
    CsrBtSppAudioRenegotiateCfm *sppPrim;

    sppPrim                 = (CsrBtSppAudioRenegotiateCfm *) CsrPmemAlloc(sizeof(CsrBtSppAudioRenegotiateCfm));
    sppPrim->type           = CSR_BT_SPP_AUDIO_RENEGOTIATE_CFM;
    sppPrim->queueId        = instData->myAppHandle;
    sppPrim->serverChannel  = instData->serverChannel;
    sppPrim->resultCode     = resultCode;
    sppPrim->resultSupplier = resultSupplier;
    sppPrim->scoHandle      = theScoHandle;
    CsrBtSppMessagePut(instData->ctrlHandle, sppPrim);
}
#endif /* CSR_BT_INSTALL_SPP_EXTENDED */
#endif /* !EXCLUDE_CSR_AM_MODULE */

#ifdef INSTALL_SPP_OUTGOING_CONNECTION
static void csrBtSppSdcFeatureSearch(SppInstanceData_t *instData)
{
    if (instData->searchOngoing == FALSE)
    {/* Perform HF search if not performing HF search already and
        either trying to connect or already connected in an incoming connection */
        CsrBtUuid32 searchUuid;
        CmnCsrBtLinkedListStruct *sdpTagList = NULL;
        CsrUint16 shIndex;
        dm_security_level_t      secOutgoing;             /* Incoming security level */

#ifndef INSTALL_SPP_CUSTOM_SECURITY_SETTINGS
        CsrBtScSetSecOutLevel(&secOutgoing, CSR_BT_SEC_DEFAULT,
                                CSR_BT_SERIAL_PORT_MANDATORY_SECURITY_OUTGOING,
                                CSR_BT_SERIAL_PORT_DEFAULT_SECURITY_OUTGOING,
                                CSR_BT_RESULT_CODE_SPP_SUCCESS,
                                CSR_BT_RESULT_CODE_SPP_UNACCEPTABLE_PARAMETER);
#else
        secOutgoing = instData->secOutgoing;
#endif /* INSTALL_SPP_CUSTOM_SECURITY_SETTINGS */

        instData->searchOngoing = TRUE;

#ifdef CSR_BT_INSTALL_SPP_EXTENDED
        if (!instData->extendedConnect)
#endif /* CSR_BT_INSTALL_SPP_EXTENDED */
        {
            searchUuid = CSR_BT_SPP_PROFILE_UUID;
            sdpTagList = CsrBtUtilSdrCreateServiceHandleEntryFromUuid32(sdpTagList, searchUuid, &shIndex);
        }
#ifdef CSR_BT_INSTALL_SPP_EXTENDED
        else
        {
            if( instData->extendedProfileUuid != 0)
            {
                searchUuid = instData->extendedProfileUuid;
                sdpTagList = CsrBtUtilSdrCreateServiceHandleEntryFromUuid32(sdpTagList, searchUuid, &shIndex);
            }
            else
            {
                CsrBtUuid128 search128Uuid;

                CsrMemCpy(search128Uuid, instData->uuid128Profile, sizeof(CsrBtUuid128));
                sdpTagList = CsrBtUtilSdrCreateServiceHandleEntryFromUuid128(sdpTagList, &search128Uuid, &shIndex);
            }
        }
#endif /* CSR_BT_INSTALL_SPP_EXTENDED */

        CsrBtUtilSdrInsertLocalServerChannel(sdpTagList, shIndex, instData->serverChannel);
        /* Frind at least remote server channel */
        CsrBtUtilSdrCreateAndInsertAttribute(sdpTagList, shIndex, CSR_BT_BLUETOOTH_PROFILE_DESCRIPTOR_LIST_ATTRIBUTE_IDENTIFIER, NULL, 0);
        /* Now choose the attributes to search for.... */
        CsrBtUtilSdrCreateAndInsertAttribute(sdpTagList, shIndex, CSR_BT_SERVICE_NAME_ATTRIBUTE_IDENTIFIER, NULL, 0);

        instData->subState  = SPP_SUB_CONNECTING_STATE;

        if (!instData->sdpSppSearchConData)
        {
            instData->sdpSppSearchConData = CsrBtUtilSdpRfcConInit(csrSppRfcResultHandler,
                                                                   CSR_SDC_OPT_CB_SELECT_SVC_HANDLE_MASK | CSR_SDC_OPT_CB_RFC_CON_RESULT_MASK,
                                                                   instData->myAppHandle);
        }

        if (instData->validPortPar)
        {
            CsrBtUtilRfcConStart((void *) instData,
                            instData->sdpSppSearchConData,
                            sdpTagList,
                            instData->bdAddr,
                            secOutgoing,
                            instData->requestPortPar,
                            instData->portPar,
                            CSR_BT_SPP_PROFILE_DEFAULT_MTU_SIZE,
                            instData->modemStatus,
                            CSR_BT_DEFAULT_MSC_TIMEOUT,
                            CSRMAX(CSR_BT_SC_DEFAULT_ENC_KEY_SIZE,
                                   CSR_BT_SPP_DEFAULT_ENC_KEY_SIZE_VAL));
        }
        else
        {
            CsrBtUtilRfcConStart((void *) instData,
                            instData->sdpSppSearchConData,
                            sdpTagList,
                            instData->bdAddr,
                            secOutgoing,
                            instData->requestPortPar,
                            NULL,
                            CSR_BT_SPP_PROFILE_DEFAULT_MTU_SIZE,
                            instData->modemStatus,
                            CSR_BT_DEFAULT_MSC_TIMEOUT,
                            CSRMAX(CSR_BT_SC_DEFAULT_ENC_KEY_SIZE,
                                   CSR_BT_SPP_DEFAULT_ENC_KEY_SIZE_VAL));
        }
    }
    else
    { /* A search is already ongoing; just wait for it and reject this one...*/
        if (instData->numberOfConnections > 0)
        {
            instData->numberOfConnections--;
        }
        csrBtSppConnectIndSend(instData,
                               instData->bdAddr,
                               0,
                               FALSE,
                               *instData->portPar,
                               CSR_BT_RESULT_CODE_SPP_ALREADY_CONNECTING,
                               CSR_BT_SUPPLIER_SPP);
    }
}
#endif /* INSTALL_SPP_OUTGOING_CONNECTION */

/* Send SPP register data path handle confirmation only when stream is not used for SPP data */
static void CsrBtSppRegisterDataPathHandleCfmSend(SppInstanceData_t *instData)
{
#ifndef CSR_STREAMS_ENABLE
    if (instData->sppRegisterDataPathPending)
    {
        CsrBtSppRegisterDataPathHandleCfm *cfmPrim;
        cfmPrim = (CsrBtSppRegisterDataPathHandleCfm*)CsrPmemAlloc(sizeof(CsrBtSppRegisterDataPathHandleCfm));
        cfmPrim->type = CSR_BT_SPP_REGISTER_DATA_PATH_HANDLE_CFM;
        cfmPrim->resultCode = CSR_BT_RESULT_CODE_SPP_SUCCESS;
        cfmPrim->resultSupplier = CSR_BT_SUPPLIER_SPP;
        CsrBtSppMessagePut(instData->ctrlHandle, cfmPrim);

        instData->sppRegisterDataPathPending = FALSE;
    }
#else
    CSR_UNUSED(instData);
#endif /* !CSR_STREAMS_ENABLE */
}

/***********************************************************************************
    Cleanup from lost control/data handle
***********************************************************************************/
void CsrBtSppEnvironmentCleanupHandler(SppInstanceData_t *instData)
{
    CsrCleanupInd *prim;
    CsrBtDeviceAddr      addr;

    prim              = (CsrCleanupInd*)instData->recvMsgP;

    /* Dead control application */
    if(prim->phandle == instData->ctrlHandle)
    {
        /* Always enter idle state. This also makes sure we ignore any disconnect
         * and deactivate confirm signal from the CM. They don't matter as there's
         * no user application at this point */
        instData->state    = Idle_s;
        instData->subState = SPP_SUB_IDLE_STATE;

        /* Send a disconnect status indication to data app before disconnect */
        CsrBtBdAddrZero(&addr);
        csrBtSppStatusIndSend(instData, FALSE, 0, addr);
    }
    /* Dead data application */
#ifndef CSR_STREAMS_ENABLE
    else if(prim->phandle == instData->dataHandle)
    {
        /* Send a data path lost indication to control app */
        csrBtSppDataPathStatusIndSend(instData, CSR_BT_DATA_PATH_STATUS_LOST);
        instData->dataHandle = CSR_SCHED_QID_INVALID;
    }
#endif /* !CSR_STREAMS_ENABLE */
}



/*************************************************************************************
    cm register cfm response
************************************************************************************/
void CsrBtSppInitStateCmRegisterCfmHandler(SppInstanceData_t *instData)
{
    CsrBtCmRegisterCfm *cmPrim;

    cmPrim = (CsrBtCmRegisterCfm *) instData->recvMsgP;
    instData->serverChannel = cmPrim->serverChannel;
    /* now ready. check for saved signals on the queue */
    instData->state    = Idle_s;
    instData->subState = SPP_SUB_IDLE_STATE;
    instData->restoreSppFlag = TRUE;
    csrBtSppHouseCleaningSend(instData);
}

/*************************************************************************************
    The server channel is registered in SDS.
************************************************************************************/
void CsrBtSppInitStateSdsRegisterCfmHandler(SppInstanceData_t *instData)
{
    CsrBtCmSdsRegisterCfm    *prim;

    prim = (CsrBtCmSdsRegisterCfm *) instData->recvMsgP;
    if (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && prim->resultSupplier == CSR_BT_SUPPLIER_CM)
    {
        dm_security_level_t      secIncoming;             /* Incoming security level */
#ifndef INSTALL_SPP_CUSTOM_SECURITY_SETTINGS
        CsrBtScSetSecInLevel(&secIncoming, CSR_BT_SEC_DEFAULT,
                        CSR_BT_SERIAL_PORT_MANDATORY_SECURITY_INCOMING,
                        CSR_BT_SERIAL_PORT_DEFAULT_SECURITY_INCOMING,
                        CSR_BT_RESULT_CODE_SPP_SUCCESS,
                        CSR_BT_RESULT_CODE_SPP_UNACCEPTABLE_PARAMETER);
#else
        secIncoming = instData->secIncoming;
#endif /* !INSTALL_SPP_CUSTOM_SECURITY_SETTINGS */

        csrBtSppActivateCfm(instData);
#ifdef CSR_BT_GLOBAL_INSTANCE
        /* Activation complete. Store the instance pointer in the SPP manager */
        sppStoreInstanceInSppManager(instData);
#endif /* CSR_BT_GLOBAL_INSTANCE */

#ifndef EXCLUDE_CSR_BT_SPP_EXTENDED_ACTIVATE_REQ
        if(!instData->extendedActivationData)
#endif /* !EXCLUDE_CSR_BT_SPP_EXTENDED_ACTIVATE_REQ */
        {
            CsrBtCmConnectAcceptReqSend(instData->myAppHandle,
                                   SPP_CLASS_OF_DEVICE,
                                   instData->activateTime,
                                   CSR_BT_SPP_PROFILE_DEFAULT_MTU_SIZE,
                                   instData->serverChannel,
                                   secIncoming,
                                   CSR_BT_SPP_PROFILE_UUID,
                                   instData->modemStatus,
                                   instData->breakSignal,
                                   CSR_BT_DEFAULT_MSC_TIMEOUT,
                                   CSRMAX(CSR_BT_SC_DEFAULT_ENC_KEY_SIZE,
                                          CSR_BT_SPP_DEFAULT_ENC_KEY_SIZE_VAL));
        }
#ifndef EXCLUDE_CSR_BT_SPP_EXTENDED_ACTIVATE_REQ
        else
        {
            CsrBtCmConnectAcceptReqSend(instData->myAppHandle,
                                   instData->extendedActivationData->cod,
                                   instData->activateTime,
                                   CSR_BT_SPP_PROFILE_DEFAULT_MTU_SIZE,
                                   instData->serverChannel,
                                   secIncoming,
                                   0,
                                   instData->modemStatus,
                                   instData->breakSignal,
                                   CSR_BT_DEFAULT_MSC_TIMEOUT,
                                   CSRMAX(CSR_BT_SC_DEFAULT_ENC_KEY_SIZE,
                                          CSR_BT_SPP_DEFAULT_ENC_KEY_SIZE_VAL));
            CsrPmemFree(instData->extendedActivationData->serviceRecord);
            CsrPmemFree(instData->extendedActivationData);
            instData->extendedActivationData = NULL;
        }
#endif /* !EXCLUDE_CSR_BT_SPP_EXTENDED_ACTIVATE_REQ */
        instData->state                = Activated_s;
        instData->sdsRecHandle        = prim->serviceRecHandle;
        instData->sdsRecordObtain    = TRUE;
        if(instData->serviceName != NULL)
        {
            CsrPmemFree(instData->serviceName);
            instData->serviceName        = NULL;
        }

        instData->restoreSppFlag        = TRUE;
        csrBtSppHouseCleaningSend(instData);
    }
    else
    {
        RFC_PORTNEG_VALUES_T   portPar;
        CsrBtDeviceAddr    deviceAddr;

        CsrBtPortParDefault( &(portPar));
        CsrBtBdAddrZero(&deviceAddr);
        csrBtSppConnectIndSend(instData, deviceAddr, 0, FALSE, portPar,
            prim->resultCode, prim->resultSupplier);

        if(instData->serviceName != NULL)
        {
            CsrPmemFree(instData->serviceName);
            instData->serviceName        = NULL;
        }
#ifndef EXCLUDE_CSR_BT_SPP_EXTENDED_ACTIVATE_REQ
        if(instData->extendedActivationData != NULL)
        {
            CsrPmemFree(instData->extendedActivationData->serviceRecord);
            CsrPmemFree(instData->extendedActivationData);
            instData->extendedActivationData = NULL;
        }
#endif /* !EXCLUDE_CSR_BT_SPP_EXTENDED_ACTIVATE_REQ */
        instData->state    = Idle_s;
        instData->subState = SPP_SUB_IDLE_STATE;

        instData->restoreSppFlag        = TRUE;
        csrBtSppHouseCleaningSend(instData);
    }
}



/*************************************************************************************
    Activate service. Register the server channel is sds to allow client side to connect.
************************************************************************************/
void CsrBtSppIdleStateSppActivateReqHandler(SppInstanceData_t *instData)
{
    if (instData->sdsUnregInProgress == TRUE)
    {
        CsrBtSppSaveMessage(instData);
    }
    else
    {
        CsrBtSppActivateReq    *prim;
        prim                        = (CsrBtSppActivateReq *) instData->recvMsgP;

        instData->ctrlHandle         = prim->phandle;
        instData->role               = prim->role;
        instData->activateTime       = prim->timeout;
        CsrBtBdAddrZero(&instData->bdAddr);
        CsrBtSppRegisterDataPathHandleCfmSend(instData);

        if (instData->sdsRecordObtain)
        {
            dm_security_level_t      secIncoming;             /* Incoming security level */
#ifndef INSTALL_SPP_CUSTOM_SECURITY_SETTINGS
            CsrBtScSetSecInLevel(&secIncoming, CSR_BT_SEC_DEFAULT,
                            CSR_BT_SERIAL_PORT_MANDATORY_SECURITY_INCOMING,
                            CSR_BT_SERIAL_PORT_DEFAULT_SECURITY_INCOMING,
                            CSR_BT_RESULT_CODE_SPP_SUCCESS,
                            CSR_BT_RESULT_CODE_SPP_UNACCEPTABLE_PARAMETER);
#else
            secIncoming = instData->secIncoming;
#endif /* !INSTALL_SPP_CUSTOM_SECURITY_SETTINGS */

            instData->state        = Activated_s;
            csrBtSppActivateCfm(instData);
#ifdef CSR_BT_GLOBAL_INSTANCE
            /* Activation complete. Store the instance pointer in the SPP manager */
            sppStoreInstanceInSppManager(instData);
#endif /* CSR_BT_GLOBAL_INSTANCE */
            CsrBtCmConnectAcceptReqSend(instData->myAppHandle,
                                   SPP_CLASS_OF_DEVICE,
                                   instData->activateTime,
                                   CSR_BT_SPP_PROFILE_DEFAULT_MTU_SIZE,
                                   instData->serverChannel,
                                   secIncoming,
                                   CSR_BT_SPP_PROFILE_UUID,
                                   instData->modemStatus,
                                   instData->breakSignal,
                                   CSR_BT_DEFAULT_MSC_TIMEOUT,
                                   CSRMAX(CSR_BT_SC_DEFAULT_ENC_KEY_SIZE,
                                          CSR_BT_SPP_DEFAULT_ENC_KEY_SIZE_VAL));
            /* Remember to de-allocate the service name */
            if (prim->serviceName != NULL)
            {
                CsrPmemFree(prim->serviceName);
            }
        }
        else
        {
            /* store the service name if sds register fails */
            instData->serviceName = prim->serviceName;
            sdsRegisterReqSend(instData);
            instData->state = Init_s;
            instData->subState = SPP_SUB_IDLE_STATE;
        }
    }
}

#ifndef EXCLUDE_CSR_BT_SPP_EXTENDED_ACTIVATE_REQ
/*************************************************************************************
    Activate service. Register the server channel in sds to allow client side to connect.
************************************************************************************/
void CsrBtSppIdleStateSppExtendedActivateReqHandler(SppInstanceData_t *instData)
{
    if (instData->sdsUnregInProgress == TRUE)
    {
        CsrBtSppSaveMessage(instData);
    }
    else
    {
        dm_security_level_t      secIncoming;             /* Incoming security level */
        CsrBtSppExtendedActivateReq    *prim;
        prim = (CsrBtSppExtendedActivateReq *) instData->recvMsgP;

        instData->ctrlHandle        = prim->phandle;
        instData->activateTime      = prim->timeout;
#ifdef INSTALL_SPP_CUSTOM_SECURITY_SETTINGS
        instData->secIncoming = prim->secLevel;
        secIncoming = prim->secLevel;
#else
       CsrBtScSetSecInLevel(&secIncoming, CSR_BT_SEC_DEFAULT,
                        CSR_BT_SERIAL_PORT_MANDATORY_SECURITY_INCOMING,
                        CSR_BT_SERIAL_PORT_DEFAULT_SECURITY_INCOMING,
                        CSR_BT_RESULT_CODE_SPP_SUCCESS,
                        CSR_BT_RESULT_CODE_SPP_UNACCEPTABLE_PARAMETER);
#endif /* INSTALL_SPP_CUSTOM_SECURITY_SETTINGS */

        CsrBtSppRegisterDataPathHandleCfmSend(instData);

        if (instData->sdsRecordObtain)
        {
            instData->state        = Activated_s;
            csrBtSppActivateCfm(instData);
            CsrBtCmConnectAcceptReqSend(instData->myAppHandle,
                                   prim->classOfDevice,
                                   instData->activateTime,
                                   CSR_BT_SPP_PROFILE_DEFAULT_MTU_SIZE,
                                   instData->serverChannel,
                                   secIncoming,
                                   0,
                                   instData->modemStatus,
                                   instData->breakSignal,
                                   CSR_BT_DEFAULT_MSC_TIMEOUT,
                                   CSRMAX(CSR_BT_SC_DEFAULT_ENC_KEY_SIZE,
                                          CSR_BT_SPP_DEFAULT_ENC_KEY_SIZE_VAL));
            CsrPmemFree(prim->serviceRecord);
        }
        else
        {
            instData->extendedActivationData = CsrPmemZalloc(sizeof(SppExtActivationData_t));
            instData->extendedActivationData->cod = prim->classOfDevice;
            instData->extendedActivationData->serviceRecord = prim->serviceRecord;
            instData->extendedActivationData->serviceRecordSize = prim->serviceRecordSize;
            instData->extendedActivationData->serviceRecordSrvChIndex = prim->serverChannelIndex;
            sdsRegisterReqSend(instData);
            instData->state = Init_s;
            instData->subState = SPP_SUB_IDLE_STATE;
        }
    }
}
#endif /* !EXCLUDE_CSR_BT_SPP_EXTENDED_ACTIVATE_REQ */

#ifndef EXCLUDE_CSR_BT_SPP_EXTENDED_ACTIVATE_REQ
void CsrBtSppDummyStateSppActivateReqHandler(SppInstanceData_t *instData)
{
    CsrBtSppExtendedActivateReq *prim;
    prim = (CsrBtSppExtendedActivateReq *) instData->recvMsgP;

    CsrPmemFree(prim->serviceRecord);
}
#endif /* !EXCLUDE_CSR_BT_SPP_EXTENDED_ACTIVATE_REQ */

void CsrBtSppIdleStateSppDeactivateReqHandler(SppInstanceData_t *instData)
{
    csrBtSppDeactivateCfmSend(instData, CSR_BT_RESULT_CODE_SPP_SUCCESS,
        CSR_BT_SUPPLIER_SPP);
#ifdef CSR_BT_GLOBAL_INSTANCE
    sppRemoveInstanceInSppManager(instData);
#endif /* CSR_BT_GLOBAL_INSTANCE */
}

#ifdef INSTALL_SPP_OUTGOING_CONNECTION
/*************************************************************************************
    A connection request from a higher layer is received. Forward the request
    to the CM.
************************************************************************************/
void CsrBtSppIdleStateSppConnectReqHandler(SppInstanceData_t *instData)
{
    if (instData->sdsUnregInProgress == TRUE)
    {
        CsrBtSppSaveMessage(instData);
    }
    else
    {
        CsrBtSppConnectReq *sppPrim;

        sppPrim = (CsrBtSppConnectReq *) instData->recvMsgP;
        /* Store the data related to the connection attempt */
        if (instData->numberOfConnections < MAX_NUMBER_OF_CONNECTIONS)
        {
            instData->ctrlHandle            = sppPrim->phandle;
            instData->role                  = sppPrim->role;
            instData->requestPortPar        = sppPrim->requestPortPar;
            instData->validPortPar          = sppPrim->validPortPar;
            *instData->portPar              = sppPrim->portPar;
            instData->bdAddr                = sppPrim->deviceAddr;
            instData->numberOfConnections++;

            CsrBtSppRegisterDataPathHandleCfmSend(instData);
            csrBtSppSdcFeatureSearch(instData);
        }
        else
        {
            /* Max number of connections already exceeded, so reject any new attempt */
            csrBtSppConnectIndSend(instData, sppPrim->deviceAddr, 0, FALSE,
                sppPrim->portPar,
                CSR_BT_RESULT_CODE_SPP_MAX_NUM_OF_CONNECTIONS,
                CSR_BT_SUPPLIER_SPP);
        }
    }
}

/*************************************************************************************
    Connect to client when service is originally activated. Cancel the activate
    and wait for confirmation before connecting
************************************************************************************/
void CsrBtSppActivateStateSppConnectReqHandler(SppInstanceData_t *instData)
{
    CsrBtSppConnectReq *sppPrim;

    sppPrim = (CsrBtSppConnectReq *) instData->recvMsgP;
    /* Store the data related to the connection attempt but remain in Active_s state */
    if (instData->numberOfConnections < MAX_NUMBER_OF_CONNECTIONS)
    {
        instData->numberOfConnections++;
        instData->ctrlHandle            = sppPrim->phandle;
        instData->bdAddr                = sppPrim->deviceAddr;
        instData->role                  = sppPrim->role;
        instData->requestPortPar        = sppPrim->requestPortPar;
        instData->validPortPar          = sppPrim->validPortPar;
        *instData->portPar              = sppPrim->portPar;
        instData->connectReqActivated   = TRUE;

        CsrBtSppRegisterDataPathHandleCfmSend(instData);
        CsrBtCmCancelAcceptConnectReqSend(instData->myAppHandle, instData->serverChannel);
    }
    else
    {
        /* Max number of connections already exceeded, so reject any new attempt */
        csrBtSppConnectIndSend(instData, sppPrim->deviceAddr, 0,
            sppPrim->validPortPar, sppPrim->portPar,
            CSR_BT_RESULT_CODE_SPP_MAX_NUM_OF_CONNECTIONS,
            CSR_BT_SUPPLIER_SPP);
    }
}

/*************************************************************************************
    A service name response from a higher layer is received. Forward the respond
    to the CM.
************************************************************************************/
void CsrBtSppIdleStateSppServiceNameResHandler(SppInstanceData_t *instData)
{
    CsrBtSppServiceNameRes *prim;
    CsrBool goOn = FALSE;

    prim = (CsrBtSppServiceNameRes *) instData->recvMsgP;

    if (prim->accept)
    {
        CsrUintFast16 idx;

        for (idx=0;idx < instData->serviceHandleListSize;idx++)
        {
            if (instData->serviceHandleList[idx].serviceHandle == prim->serviceHandle)
            {
                CsrUint16 * ptrHandle = CsrPmemAlloc(sizeof(CsrUint16));
                *ptrHandle  = instData->serviceHandleList[idx].selectServiceIdx;
                CsrBtUtilRfcConSetServiceHandleIndexList((void *)instData, instData->cmSdcRfcInstData, ptrHandle,1);
                goOn = TRUE;
            }
        }
    }

    if (!goOn)
    {
        RFC_PORTNEG_VALUES_T portPar;

        CsrBtPortParDefault( &(portPar));
        /* Now cancel the connection and wait for the result function to be called. */
        instData->connectReqActivated = FALSE;
        CsrBtUtilRfcConCancel((void*)(instData),instData->sdpSppSearchConData);
    }

    if (instData->serviceHandleList)
    {
        CsrPmemFree(instData->serviceHandleList);
        instData->serviceHandleList     = NULL;
        instData->serviceHandleListSize = 0;
    }
}

static void csrBtSppServiceNameIndSend(SppInstanceData_t *instData)
{
    CsrBtSppServiceNameInd * prim;
    CsrUint16 nrOfBytes = sizeof(CsrBtSppServiceName)*instData->serviceHandleListSize;

    prim            = (CsrBtSppServiceNameInd *) CsrPmemAlloc(sizeof(CsrBtSppServiceNameInd));
    prim->type            = CSR_BT_SPP_SERVICE_NAME_IND;
    prim->queueId = instData->myAppHandle;
    prim->serviceNameList = CsrPmemAlloc(nrOfBytes);
    CsrMemCpy((CsrUint8 *)prim->serviceNameList,(CsrUint8 *)instData->sdpServiceNameList,nrOfBytes);
    prim->serviceNameListSize    = instData->serviceHandleListSize;

    CsrBtSppMessagePut(instData->ctrlHandle, prim);
}
#endif /* INSTALL_SPP_OUTGOING_CONNECTION */

#ifdef CSR_BT_INSTALL_SPP_EXTENDED
/*************************************************************************************
    A connection request from a higher layer is received. Forward the request
    to the CM.
************************************************************************************/
void CsrBtSppIdleStateSppExtendedConnectReqHandler(SppInstanceData_t *instData)
{
    if (instData->sdsUnregInProgress == TRUE)
    {
        CsrBtSppSaveMessage(instData);
    }
    else
    {
        CsrBtSppExtendedConnectReq *sppPrim;

        sppPrim = (CsrBtSppExtendedConnectReq *) instData->recvMsgP;
        /* Store the data related to the connection attempt */
        if (instData->numberOfConnections < MAX_NUMBER_OF_CONNECTIONS)
        {
            instData->extendedProfileUuid   = sppPrim->profileUuid;
            instData->ctrlHandle            = sppPrim->phandle;
            instData->role                  = sppPrim->role;
            instData->requestPortPar        = sppPrim->requestPortPar;
            instData->validPortPar          = sppPrim->validPortPar;
            *instData->portPar              = sppPrim->portPar;
            instData->bdAddr                = sppPrim->deviceAddr;
#ifdef INSTALL_SPP_CUSTOM_SECURITY_SETTINGS
            instData->secOutgoing           = sppPrim->secLevel;
#endif
            instData->extendedConnect       = TRUE;
            instData->numberOfConnections++;

            CsrBtSppRegisterDataPathHandleCfmSend(instData);
            csrBtSppSdcFeatureSearch(instData);
        }
        else
        {
            /* Max number of connections already exceeded, so reject any new attempt */
            csrBtSppConnectIndSend(instData, sppPrim->deviceAddr, 0, FALSE,
                sppPrim->portPar,
                CSR_BT_RESULT_CODE_SPP_MAX_NUM_OF_CONNECTIONS,
                CSR_BT_SUPPLIER_SPP);
        }
    }
}
#endif /* CSR_BT_INSTALL_SPP_EXTENDED */

#ifdef CSR_BT_INSTALL_SPP_EXTENDED
void CsrBtSppIdleStateSppExtendedUuidConnectReqHandler(SppInstanceData_t *instData)
{
    if (instData->sdsUnregInProgress == TRUE)
    {
        CsrBtSppSaveMessage(instData);
    }
    else
    {
        CsrBtSppExtendedUuidConnectReq *sppPrim;

        sppPrim = (CsrBtSppExtendedUuidConnectReq *) instData->recvMsgP;
        /* Store the data related to the connection attempt */
        if (instData->numberOfConnections < MAX_NUMBER_OF_CONNECTIONS)
        {
            CsrMemCpy(instData->uuid128Profile, sppPrim->profileUuid, sizeof(CsrBtUuid128));

            instData->extendedProfileUuid    = 0;
            instData->ctrlHandle            = sppPrim->phandle;
            instData->role                  = sppPrim->role;
            instData->requestPortPar        = sppPrim->requestPortPar;
            instData->validPortPar          = sppPrim->validPortPar;
            *instData->portPar              = sppPrim->portPar;
            instData->bdAddr                = sppPrim->deviceAddr;
            instData->extendedConnect       = TRUE;
            instData->extendedConnect       = TRUE;
#ifdef INSTALL_SPP_CUSTOM_SECURITY_SETTINGS
            instData->secOutgoing           = sppPrim->secLevel;
#endif
            instData->numberOfConnections++;
            CsrPmemFree(instData->sdpServiceNameList);
            instData->sdpServiceNameList = NULL;

            CsrBtSppRegisterDataPathHandleCfmSend(instData);
            csrBtSppSdcFeatureSearch(instData);
        }
        else
        {
            /* Max number of connections already exceeded, so reject any new attempt */
            csrBtSppConnectIndSend(instData, sppPrim->deviceAddr, 0, FALSE,
                sppPrim->portPar,
                CSR_BT_RESULT_CODE_SPP_MAX_NUM_OF_CONNECTIONS,
                CSR_BT_SUPPLIER_SPP);
        }
    }
}
#endif /* CSR_BT_INSTALL_SPP_EXTENDED */

#ifndef EXCLUDE_CSR_AM_MODULE
#ifdef CSR_BT_INSTALL_SPP_EXTENDED
void CsrBtSppConnectedStateSppAudioReqHandler(SppInstanceData_t *instData)
{
    CsrBtSppAudioConnectReq *sppPrim;

    sppPrim = (CsrBtSppAudioConnectReq *) instData->recvMsgP;

    switch (instData->audioState)
    {
        case audioAcceptReq_s:
        {
            CsrBtCmScoCancelReqSend(instData->myAppHandle, instData->sppConnId);
            /* !! FALL THROUGH !! */
        }
        case audioOff_s:
        {
            CsrBtCmScoCommonParms *parms;
            CsrUint16 parmsOffset;
            CsrUint16 parmsLen;

            instData->audioState = audioReq_s;
            instData->pendingSco = TRUE;

            if(sppPrim->audioParameters == NULL)
            { /* Use default from csr_bt_usr_config.h */
                parmsLen = 1;
                CsrBtCmCommonScoConnectPrepare(&parms, &parmsOffset, parmsLen);

                CsrBtCmCommonScoConnectBuild(parms,
                                        &parmsOffset,
                                        CSR_BT_ESCO_DEFAULT_CONNECT_AUDIO_QUALITY,
                                        CSR_BT_ESCO_DEFAULT_CONNECT_TX_BANDWIDTH,
                                        CSR_BT_ESCO_DEFAULT_CONNECT_RX_BANDWIDTH,
                                        CSR_BT_ESCO_DEFAULT_CONNECT_MAX_LATENCY,
                                        CSR_BT_ESCO_DEFAULT_CONNECT_VOICE_SETTINGS,
                                        CSR_BT_ESCO_DEFAULT_CONNECT_RE_TX_EFFORT);
            }
            else
            { /* Use application parameters */
                CsrUintFast8 i;
                parmsLen = sppPrim->audioParametersLength;
                CsrBtCmCommonScoConnectPrepare(&parms, &parmsOffset, parmsLen);

                for(i=0; i < sppPrim->audioParametersLength; i++)
                {
                    CsrBtCmCommonScoConnectBuild(parms,
                                            &parmsOffset,
                                            sppPrim->audioParameters[i].packetType,
                                            sppPrim->audioParameters[i].txBandwidth,
                                            sppPrim->audioParameters[i].rxBandwidth,
                                            sppPrim->audioParameters[i].maxLatency,
                                            sppPrim->audioParameters[i].voiceSettings,
                                            sppPrim->audioParameters[i].reTxEffort);
                }
                CsrPmemFree(sppPrim->audioParameters);
            }

            CsrBtCmScoConnectReqSend(CSR_BT_SPP_IFACEQUEUE,
                                     sppPrim->pcmSlot,
                                     sppPrim->pcmReassign,
                                     parms,
                                     parmsLen,
                                     instData->sppConnId);
            break;
        }
        case audioAcceptInProgress_s:
        {
            /* Incoming SCO already in progress - discard audio request */
            break;
        }
        default :
        {
            instData->pendingSco = TRUE;
            csrBtSppAudioIndSend(instData, 0, 0, 0, 0, 0, 0, 0, sppPrim->pcmSlot,
                    CSR_BT_RESULT_CODE_SPP_COMMAND_DISALLOWED, CSR_BT_SUPPLIER_SPP);
            break;
        }
    }
}

void CsrBtSppConnectedStateSppAcceptAudioReqHandler(SppInstanceData_t *instData)
{
    CsrBtSppAcceptAudioReq *sppPrim;

    sppPrim = (CsrBtSppAcceptAudioReq *) instData->recvMsgP;

    instData->audioQuality  = sppPrim->audioQuality;
    instData->txBandwidth   = sppPrim->txBandwidth;
    instData->rxBandwidth   = sppPrim->rxBandwidth;
    instData->maxLatency    = sppPrim->maxLatency;
    instData->voiceSettings = sppPrim->voiceSettings;
    instData->reTxEffort    = sppPrim->reTxEffort;

    if (instData->audioState == audioOff_s)
    {
        instData->audioState = audioAcceptReq_s;
        CsrBtCmScoAcceptConnectReqSend(instData->myAppHandle,
                                       instData->sppConnId,
                                       instData->audioQuality,
                                       instData->txBandwidth,
                                       instData->rxBandwidth,
                                       instData->maxLatency,
                                       instData->voiceSettings,
                                       instData->reTxEffort);
    }
}

void CsrBtSppConnectedStateSppAudioRenegotiateReqHandler(SppInstanceData_t *instData)
{
    CsrBtSppAudioRenegotiateReq *sppPrim;

    sppPrim = (CsrBtSppAudioRenegotiateReq *) instData->recvMsgP;

    if (instData->audioState == audioOn_s)
    {
        CsrBtcmScoRenegotiateReqSend(instData->myAppHandle,
                                     instData->sppConnId,
                                     sppPrim->audioQuality,
                                     sppPrim->maxLatency,
                                     sppPrim->reTxEffort);
    }
    else
    {
        csrBtSppAudioRenegotiateCfmSend(instData, CSR_BT_RESULT_CODE_SPP_COMMAND_DISALLOWED, CSR_BT_SUPPLIER_SPP, sppPrim->scoHandle);
    }
}

void CsrBtSppConnectedStateSppCancelAcceptAudioReqHandler(SppInstanceData_t *instData)
{
    if (instData->audioState == audioAcceptReq_s || instData->audioState == audioAcceptInProgress_s)
    {
        instData->audioState = audioOff_s;
        CsrBtCmScoCancelReqSend(instData->myAppHandle, instData->sppConnId);
    }
}

void CsrBtSppConnectedStateSppAudioReleaseReqHandler(SppInstanceData_t *instData)
{
    if (instData->audioState == audioOn_s)
    {
        instData->audioState = audioRelease_s;
        instData->pendingScoDisconnect = TRUE;
        CsrBtCmScoDisconnectReqSend(instData->myAppHandle, instData->sppConnId);

        if ((instData->audioUseAm == UseAm) && (instData->amConnId != CSR_AM_NO_CONN_ID))
        {
            CsrAmAudioPathReleaseReqSend(CSR_BT_SPP_IFACEQUEUE, instData->amConnId);
            instData->amSppCallBack = CsrSppAmReleaseCfm;
        }
    }
}
#endif /* CSR_BT_INSTALL_SPP_EXTENDED */
#endif /* !EXCLUDE_CSR_AM_MODULE */

void CsrBtSppIdleStateCmDisconnectIndHandler(SppInstanceData_t *instData)
{
    /* This disconnect was started because a connectionestablishment in idle_s crossed a cancelConnectReq
     *This event should be  reported as a cancel connection connection attempt to the connectReq */
    CsrBtCmLogicalChannelTypeReqSend(CSR_BT_NO_ACTIVE_LOGICAL_CHANNEL,
                                     instData->bdAddr,
                                     instData->sppConnId);

    instData->sppConnId = SPP_NO_CONNID;
#ifdef INSTALL_SPP_OUTGOING_CONNECTION
    if ((instData->searchOngoing) && (CsrBtUtilRfcConVerifyCmMsg(instData->recvMsgP)))
    {
        CsrBtUtilRfcConCmMsgHandler(instData, instData->sdpSppSearchConData, instData->recvMsgP);

#ifdef CSR_BT_INSTALL_CM_DISABLE_AUTO_DISC_RESP
        /* Respective SDC handler for the CM messages will take care of sending disconnect response to bluestack.
         * Hence there is no need to call CsrBtCmRfcDisconnectRspSend here. */
#endif
    }
    else
#endif /* INSTALL_SPP_OUTGOING_CONNECTION */
    {
#ifdef CSR_BT_INSTALL_CM_DISABLE_AUTO_DISC_RESP
        if (!(((CsrBtCmDisconnectInd *) instData->recvMsgP)->localTerminated))
        {
            /* For remote disconnections, profile needs to respond to RFC_DISCONNECT_IND. */
            CsrBtCmRfcDisconnectRspSend(((CsrBtCmDisconnectInd *) instData->recvMsgP)->btConnId);
        }
#endif
        if(instData->cancelReceived)
        {
            RFC_PORTNEG_VALUES_T  portPar;

            CsrBtPortParDefault( &(portPar));

            instData->cancelReceived = FALSE;
            csrBtSppConnectIndSend(instData, instData->bdAddr, 0, FALSE,
                portPar,
                CSR_BT_RESULT_CODE_CM_CANCELLED, /* XXX: CM spoofing */
                CSR_BT_SUPPLIER_CM);
            instData->subState = SPP_SUB_IDLE_STATE;
            if (instData->numberOfConnections > 0)
            {
                instData->numberOfConnections--;
            }
        }
        else
        {
            ; /* Just ignore it because it should not happen in this state under any other circumstance */
        }
        CsrBtBdAddrZero(&instData->bdAddr);
    }
}

#ifndef CSR_STREAMS_ENABLE
/*************************************************************************************
    Downstream data received. Send the data to CM.

    Please note: the original signal is reused in order to optimise.
************************************************************************************/
void CsrBtSppConnectedStateSppDataReqHandler(SppInstanceData_t *instData)
{
    CsrBtSppDataReq    *prim;

    prim = (CsrBtSppDataReq *) instData->recvMsgP;

    /* the received signal is passed on to CM so set pointer to NULL in order
       not to free it in the main function */
    CsrBtCmDataReqSend(instData->sppConnId,
                       prim->payloadLength,
                       prim->payload);
}

/*************************************************************************************
    Data is received when not expected. Free the data and ignore the signal
************************************************************************************/
void CsrBtSppNotConnectedSppDataReqHandler(SppInstanceData_t *instData)
{
    CsrBtSppDataReq    *prim;

    prim = (CsrBtSppDataReq *) instData->recvMsgP;
    CsrPmemFree(prim->payload);
}

/*************************************************************************************
    Downstream data is received and consumed by the CM. Send the confirm
    signal to the higher layer. The higher layer must not send any more data
    before the data is confirmed by the CM

    Please note: the original signal is reused in order to optimise.
************************************************************************************/
void CsrBtSppConnectedStateCmDataCfmHandler(SppInstanceData_t *instData)
{

    CsrBtSppDataCfm    *myPrim;

    myPrim = (CsrBtSppDataCfm *) CsrPmemAlloc(sizeof(CsrBtSppDataCfm));
    myPrim->type = CSR_BT_SPP_DATA_CFM;
    myPrim->queueId = instData->myAppHandle;
    myPrim->serverChannel = instData->serverChannel;

    csrBtSppDataPathSend(instData, myPrim);
}

/*************************************************************************************
    Upstream data is received. Send the data on to the application.

    Please note: the original signal is reused in order to optimise.
************************************************************************************/
void CsrBtSppConnectedStateCmDataIndHandler(SppInstanceData_t *instData)
{
    CsrBtCmDataInd    *prim;
    CsrBtSppDataInd    *myPrim;

    prim = (CsrBtCmDataInd *) instData->recvMsgP;

    myPrim = (CsrBtSppDataInd *) CsrPmemAlloc(sizeof(CsrBtSppDataInd));
    myPrim->type = CSR_BT_SPP_DATA_IND;
    myPrim->queueId = instData->myAppHandle;
    myPrim->serverChannel = instData->serverChannel;
    myPrim->payloadLength = prim->payloadLength;
    myPrim->payload = prim->payload;

    csrBtSppDataPathSend(instData, myPrim);
}

/***********************************************************************************
    Set data application handle
***********************************************************************************/
void CsrBtSppAnyStateRegisterDataPathHandleReqHandler(SppInstanceData_t *instData)
{
    CsrBtSppRegisterDataPathHandleReq *reqPrim;

    /* Set data handle */
    reqPrim = (CsrBtSppRegisterDataPathHandleReq*)instData->recvMsgP;
    instData->dataHandle = reqPrim->dataAppHandle;
    if (instData->ctrlHandle != CSR_SCHED_QID_INVALID)
    {/* Send confirm */
        CsrBtSppRegisterDataPathHandleCfm *cfmPrim;
        cfmPrim = (CsrBtSppRegisterDataPathHandleCfm*)CsrPmemAlloc(sizeof(CsrBtSppRegisterDataPathHandleCfm));
        cfmPrim->type = CSR_BT_SPP_REGISTER_DATA_PATH_HANDLE_CFM;
        cfmPrim->resultCode = CSR_BT_RESULT_CODE_SPP_SUCCESS;
        cfmPrim->resultSupplier = CSR_BT_SUPPLIER_SPP;
        CsrBtSppMessagePut(instData->ctrlHandle, cfmPrim);
    }
    else
    {
        instData->sppRegisterDataPathPending = TRUE;
    }
}

/*************************************************************************************
        Handle data path status changes - send indication to control handle
************************************************************************************/
void CsrBtSppConnectedStateSppDataPathStatusReqHandler(SppInstanceData_t *instData)
{
    CsrBtSppDataPathStatusReq *reqPrim;
    CsrBtSppDataPathStatusInd *indPrim;

    reqPrim = (CsrBtSppDataPathStatusReq*)instData->recvMsgP;
    indPrim = (CsrBtSppDataPathStatusInd*)CsrPmemAlloc(sizeof(CsrBtSppDataPathStatusInd));

    indPrim->type             = CSR_BT_SPP_DATA_PATH_STATUS_IND;
    indPrim->queueId = instData->myAppHandle;
    indPrim->status           = reqPrim->status;

    CsrBtSppMessagePut(instData->ctrlHandle, indPrim);
}

/*************************************************************************************
    Upstream data is received and consumed by the higher layer. Send the confirm
    signal to the CM. The higher layer will not receive any more data before the
    data is confirmed by the higher layer

    Please note: the original signal is reused in order to optimise.
************************************************************************************/
void CsrBtSppConnectedStateSppDataResHandler(SppInstanceData_t *instData)
{
    CsrBtCmDataResSend(instData->sppConnId);
}
#endif /* !CSR_STREAMS_ENABLE */

#ifdef INSTALL_SPP_MODEM_STATUS_COMMAND
/*************************************************************************************
    Send a control signal.
************************************************************************************/
void CsrBtSppConnectedStateSppControlReqHandler(SppInstanceData_t *instData)
{
    CsrBtSppControlReq    *prim;
    CsrUint8                myModemStatus;

    prim = (CsrBtSppControlReq *) instData->recvMsgP;
    myModemStatus = CsrBtMapSendingControlSignal(prim->modemstatus, instData->role);
    CsrBtCmControlReqSend(instData->sppConnId, myModemStatus, prim->break_signal);
    instData->modemStatus = myModemStatus;
    instData->breakSignal = prim->break_signal;
}
#endif /* INSTALL_SPP_MODEM_STATUS_COMMAND */

/*************************************************************************************
    Send a control signal.
************************************************************************************/
void CsrBtSppConnectedStateCmControlIndHandler(SppInstanceData_t *instData)
{
    CsrBtCmControlInd    *cmPrim;
    CsrBtSppControlInd    *sppPrim;

    cmPrim = (CsrBtCmControlInd *) instData->recvMsgP;

    sppPrim = (CsrBtSppControlInd *)CsrPmemAlloc(sizeof(CsrBtSppControlInd));
    sppPrim->type = CSR_BT_SPP_CONTROL_IND;
    sppPrim->modemstatus = CsrBtMapReceivedControlSignal(cmPrim->modemstatus, instData->role);
    sppPrim->break_signal = cmPrim->break_signal;
    sppPrim->queueId = instData->myAppHandle;
    sppPrim->serverChannel = instData->serverChannel;

    /* Control signals is data */
    csrBtSppDataPathSend(instData, sppPrim);
}


void CsrBtSppConnectedStateSppDeactivateReqHandler(SppInstanceData_t *instData)
{
    if (instData->sdsUnregInProgress == TRUE)
    {
        CsrBtSppSaveMessage(instData);
    }
    else
    {
        instData->state = Deactivate_s;
        CsrBtCmDisconnectReqSend(instData->sppConnId);
#ifdef CSR_BT_GLOBAL_INSTANCE
        sppRemoveInstanceInSppManager(instData);
#endif /* CSR_BT_GLOBAL_INSTANCE */
    }
}

/*************************************************************************************
    Request a disconnect of the logical connection.
************************************************************************************/
void CsrBtSppConnectedStateSppDisconnectReqHandler(SppInstanceData_t *instData)
{
    if (instData->sdsUnregInProgress == TRUE)
    {
        CsrBtSppSaveMessage(instData);
    }
    else
    {
        CsrBtCmDisconnectReqSend(instData->sppConnId);
        instData->restoreSppFlag        = TRUE;
        csrBtSppHouseCleaningSend(instData);
    }
}

/*************************************************************************************
    A message is received which should be ignored. This may be due to the
    chance/risc of a race condition.
************************************************************************************/
void CsrBtSppIgnoreMessageHandler(SppInstanceData_t *instData)
{
    CSR_UNUSED(instData);
    /* do nothing */
}

/*************************************************************************************
    Disconnect the logical connection.
************************************************************************************/
void CsrBtSppConnectedStateCmDisconnectIndHandler(SppInstanceData_t *instData)
{
#ifndef EXCLUDE_CSR_AM_MODULE
#ifdef CSR_BT_INSTALL_SPP_EXTENDED
    instData->audioState = audioOff_s;
#endif /* CSR_BT_INSTALL_SPP_EXTENDED */
#endif /* !EXCLUDE_CSR_AM_MODULE */

    if (instData->sdsUnregInProgress == TRUE)
    {
        CsrMessageQueuePush(&instData->saveQueue, CSR_BT_CM_PRIM, instData->recvMsgP);
        instData->recvMsgP = NULL;
    }
    else
    {
        /* send the disconnect to the higher layer and set instance variables */
        CsrBtCmDisconnectInd *cmPrim = (CsrBtCmDisconnectInd *) instData->recvMsgP;

#ifdef CSR_BT_INSTALL_CM_DISABLE_AUTO_DISC_RESP
        if (!cmPrim->localTerminated)
        {
            /* For remote disconnections, profile needs to respond to RFC_DISCONNECT_IND. */
            CsrBtCmRfcDisconnectRspSend(cmPrim->btConnId);
        }
#endif

        CsrBtCmLogicalChannelTypeReqSend(CSR_BT_NO_ACTIVE_LOGICAL_CHANNEL,
                                         instData->bdAddr,
                                         instData->sppConnId);

        instData->state    = Idle_s;
        if (instData->numberOfConnections > 0)
        {
            instData->numberOfConnections--;
        }
        instData->sppConnId = SPP_NO_CONNID;

#ifdef INSTALL_SPP_REMOTE_PORT_NEGOTIATION
        if (instData->sppPortNegPending)
        {
            CsrBtSppPortnegCfm *cfmPrim = (CsrBtSppPortnegCfm *)CsrPmemAlloc(sizeof(CsrBtSppPortnegCfm));
            RFC_PORTNEG_VALUES_T      portPar;

            CsrBtPortParDefault(&portPar);

            cfmPrim->type            = CSR_BT_SPP_PORTNEG_CFM;
            cfmPrim->serverChannel   = instData->serverChannel;
            cfmPrim->portPar         = portPar;
            cfmPrim->queueId         = instData->myAppHandle;
            cfmPrim->resultCode      = CSR_BT_RESULT_CODE_SPP_COMMAND_DISALLOWED;
            cfmPrim->resultSupplier  = CSR_BT_SUPPLIER_SPP;

            csrBtSppDataPathSend(instData, cfmPrim);

            instData->sppPortNegPending = FALSE;
        }
#endif /* INSTALL_SPP_REMOTE_PORT_NEGOTIATION */

        if (instData->subState == SPP_SUB_CROSSING_CONNECT_STATE)
        {
        /* This disconnect was started because an incoming
         * connection in activated_s crossed an outgoing Don't
         * report this failed connection - continue to serve the
         * outgoing */
            RFC_PORTNEG_VALUES_T   portPar;

            CsrBtPortParDefault(&portPar);
            if(instData->cancelReceived)
            {
                /* The outgoing request has also been cancelled - so
                 * don't start it. Decrement connection counter
                 * accordingly */
                instData->cancelReceived = FALSE;
                csrBtSppConnectIndSend(instData, instData->bdAddr, 0, FALSE,
                    portPar,
                    CSR_BT_RESULT_CODE_CM_CANCELLED, /* XXX: CM spoofing */
                    CSR_BT_SUPPLIER_CM);
                instData->subState = SPP_SUB_IDLE_STATE;
                if (instData->numberOfConnections > 0)
                {
                    instData->numberOfConnections--;
                }
            }
            else
            {
#ifdef INSTALL_SPP_OUTGOING_CONNECTION
                csrBtSppSdcFeatureSearch(instData);
#endif /* INSTALL_SPP_OUTGOING_CONNECTION */

            }
        }
        else /* all other substates */
        {
        /* This was a normal disconnect or a cancel after full
         * connection - report it as such */
            if (instData->cancelReceived)
            {
                instData->cancelReceived = FALSE;
                csrBtSppDisconnectIndSend(instData,
                    CSR_BT_RESULT_CODE_CM_CANCELLED,
                    CSR_BT_SUPPLIER_CM,
                    FALSE);
            }
            else
            {
                csrBtSppDisconnectIndSend(instData,
                    cmPrim->reasonCode,
                    cmPrim->reasonSupplier,
                    cmPrim->localTerminated);
            }

            instData->subState = SPP_SUB_IDLE_STATE;
        }
        CsrBtBdAddrZero(&instData->bdAddr);
    }
}

#ifndef EXCLUDE_CSR_AM_MODULE
#ifdef CSR_BT_INSTALL_SPP_EXTENDED
void CsrBtSppConnectedStateCmScoConnectCfmHandler(SppInstanceData_t *instData)
{
    CsrBtCmScoConnectCfm *cmPrim;

    cmPrim = (CsrBtCmScoConnectCfm *) instData->recvMsgP;

    if(cmPrim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && cmPrim->resultSupplier == CSR_BT_SUPPLIER_CM)
    {
        instData->audioState    = audioOn_s;
        if ((instData->audioUseAm == UseAm) && (instData->amConnId == CSR_AM_NO_CONN_ID))
        {/* Allocate the audio stream */
            CsrSppConnectStream(instData, CSR_AM_CVSD_AUDIO, cmPrim->pcmSlot, cmPrim->eScoHandle);
            instData->amSppCallBack = CsrSppAmConnectCfm;
        }

        cmPrim->resultCode = CSR_BT_RESULT_CODE_SPP_SUCCESS;
        cmPrim->resultSupplier = CSR_BT_SUPPLIER_SPP;
    }
    else
    {
        instData->audioState = audioOff_s;
    }

    csrBtSppAudioIndSend(instData, cmPrim->eScoHandle, cmPrim->linkType,
        cmPrim->txInterval, cmPrim->weSco, cmPrim->rxPacketLength,
        cmPrim->txPacketLength, cmPrim->airMode, cmPrim->pcmSlot,
        cmPrim->resultCode, cmPrim->resultSupplier);
}

static void csrBtSppSendSppAudioDisconnectInd(SppInstanceData_t *instData,
                                              hci_connection_handle_t eScoHandle,
                                              CsrBtReasonCode reasonCode,
                                              CsrBtSupplier reasonSupplier)
{
    CsrBtSppAudioDisconnectInd * sppPrim;
    sppPrim                    = (CsrBtSppAudioDisconnectInd *) CsrPmemAlloc(sizeof(CsrBtSppAudioDisconnectInd));

    sppPrim->type              = CSR_BT_SPP_AUDIO_DISCONNECT_IND;
    sppPrim->queueId           = instData->myAppHandle;
    sppPrim->serverChannel     = instData->serverChannel;
    sppPrim->reasonCode        = reasonCode;
    sppPrim->reasonSupplier    = reasonSupplier;
    sppPrim->scoHandle         = eScoHandle;

    CsrBtSppMessagePut(instData->ctrlHandle, sppPrim);
}

static void csrBtSppSendSppAudioDisconnectCfm(SppInstanceData_t *instData,
                                              hci_connection_handle_t eScoHandle,
                                              CsrBtResultCode resultCode,
                                              CsrBtSupplier resultSupplier)
{
    CsrBtSppAudioDisconnectCfm * sppPrim;
    sppPrim                    = (CsrBtSppAudioDisconnectCfm *) CsrPmemAlloc(sizeof(CsrBtSppAudioDisconnectCfm));

    instData->pendingScoDisconnect = FALSE;
    sppPrim->type              = CSR_BT_SPP_AUDIO_DISCONNECT_IND;
    sppPrim->queueId           = instData->myAppHandle;
    sppPrim->serverChannel     = instData->serverChannel;
    sppPrim->resultCode        = resultCode;
    sppPrim->resultSupplier    = resultSupplier;
    sppPrim->scoHandle         = eScoHandle;

    CsrBtSppMessagePut(instData->ctrlHandle, sppPrim);
}

void CsrBtSppConnectedStateCmScoDisconnectIndHandler(SppInstanceData_t *instData)
{
    CsrBtCmScoDisconnectInd        * cmPrim;

    cmPrim                    = (CsrBtCmScoDisconnectInd *) instData->recvMsgP;

    if (cmPrim->status == TRUE)
    {
        instData->audioState    = audioOff_s;
        if ((instData->audioUseAm == UseAm) && (instData->amConnId != CSR_AM_NO_CONN_ID))
        {
            CsrAmAudioPathReleaseReqSend(CSR_BT_SPP_IFACEQUEUE, instData->amConnId);
            instData->amSppCallBack = CsrSppAmReleaseCfm;
        }
    }
    else if (instData->audioState == audioRelease_s)
    {
        instData->audioState    = audioOn_s;
    }

    if(instData->pendingScoDisconnect)
    {
        CsrBtReasonCode reasonCode;
        CsrBtSupplier   reasonSupplier;

        if (cmPrim->status == TRUE)
        {
            reasonCode = CSR_BT_RESULT_CODE_SPP_SUCCESS;
            reasonSupplier = CSR_BT_SUPPLIER_SPP;
        }
        else
        {
            reasonCode = cmPrim->reasonCode;
            reasonSupplier = cmPrim->reasonSupplier;
        }

        csrBtSppSendSppAudioDisconnectCfm(instData,cmPrim->eScoHandle, reasonCode, reasonSupplier);
    }
    else
    {
        csrBtSppSendSppAudioDisconnectInd(instData,cmPrim->eScoHandle, cmPrim->reasonCode, cmPrim->reasonSupplier);
    }
}

void CsrBtSppCmScoRenegotiateCfmHandler(SppInstanceData_t *instData)
{
    CsrBtCmScoRenegotiateCfm    * cmPrim;

    cmPrim        = (CsrBtCmScoRenegotiateCfm *) instData->recvMsgP;

    if (cmPrim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && cmPrim->resultSupplier == CSR_BT_SUPPLIER_CM)
    {
        csrBtSppAudioRenegotiateCfmSend(instData, CSR_BT_RESULT_CODE_SPP_SUCCESS, CSR_BT_SUPPLIER_SPP, cmPrim->eScoHandle);
    }
    else
    {
        csrBtSppAudioRenegotiateCfmSend(instData, cmPrim->resultCode, cmPrim->resultSupplier, cmPrim->eScoHandle);
    }
}

void CsrBtSppCmScoRenegotiateIndHandler(SppInstanceData_t *instData)
{
    CsrBtSppAudioRenegotiateInd *sppPrim;
    CsrBtCmScoRenegotiateInd    * cmPrim;

    cmPrim                  = (CsrBtCmScoRenegotiateInd *) instData->recvMsgP;
    sppPrim                 = (CsrBtSppAudioRenegotiateInd *) CsrPmemAlloc(sizeof(CsrBtSppAudioRenegotiateInd));
    sppPrim->type           = CSR_BT_SPP_AUDIO_RENEGOTIATE_IND;
    sppPrim->queueId        = instData->myAppHandle;
    sppPrim->serverChannel  = instData->serverChannel;
    sppPrim->scoHandle      = cmPrim->eScoHandle;

    if (cmPrim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && cmPrim->resultSupplier == CSR_BT_SUPPLIER_CM)
    {
        cmPrim->resultCode     = CSR_BT_RESULT_CODE_SPP_SUCCESS;
        cmPrim->resultSupplier = CSR_BT_SUPPLIER_SPP;
    }

    sppPrim->resultCode     = cmPrim->resultCode;
    sppPrim->resultSupplier = cmPrim->resultSupplier;

    CsrBtSppMessagePut(instData->ctrlHandle, sppPrim);
}
#endif /* CSR_BT_INSTALL_SPP_EXTENDED */
#endif /* !EXCLUDE_CSR_AM_MODULE */

#ifndef EXCLUDE_CSR_AM_MODULE
#ifdef CSR_BT_INSTALL_SPP_EXTENDED
void CsrBtSppConnectedStateCmScoAcceptConnectCfmHandler(SppInstanceData_t *instData)
{
    CsrBtCmScoAcceptConnectCfm *cmPrim;

    cmPrim = (CsrBtCmScoAcceptConnectCfm *) instData->recvMsgP;
    if(cmPrim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && cmPrim->resultSupplier == CSR_BT_SUPPLIER_CM)
    {
        instData->audioState = audioOn_s;
        if ((instData->audioUseAm == UseAm) && (instData->amConnId == CSR_AM_NO_CONN_ID))
        {/* Allocate the audio stream */
            CsrSppConnectStream(instData, CSR_AM_CVSD_AUDIO, cmPrim->pcmSlot, cmPrim->eScoHandle);
            instData->amSppCallBack = CsrSppAmConnectCfm;
        }
        csrBtSppAudioIndSend(instData, cmPrim->eScoHandle, cmPrim->linkType,
                    cmPrim->txInterval, cmPrim->weSco, cmPrim->rxPacketLength,
                    cmPrim->txPacketLength, cmPrim->airMode, cmPrim->pcmSlot,
                    CSR_BT_RESULT_CODE_SPP_SUCCESS, CSR_BT_SUPPLIER_SPP);
    }
    else if (instData->audioState == audioAcceptReq_s ||
             instData->audioState == audioAcceptInProgress_s)
    {
        instData->audioState = audioOff_s;
        csrBtSppAudioIndSend(instData, cmPrim->eScoHandle, cmPrim->linkType,
                    cmPrim->txInterval, cmPrim->weSco, cmPrim->rxPacketLength,
                    cmPrim->txPacketLength, cmPrim->airMode, cmPrim->pcmSlot,
                    cmPrim->resultCode, cmPrim->resultSupplier);
    }
}

void CsrBtSppDeactivateStateCmScoXHandler(SppInstanceData_t *instData)
{
    instData->audioState = audioOff_s;
}
#endif /* CSR_BT_INSTALL_SPP_EXTENDED */
#endif /* !EXCLUDE_CSR_AM_MODULE */

/*************************************************************************************
    Deactivate the service by turning off inquiry scan. Send message to CM and wait
    for the confirm message that scanning is turned off.
************************************************************************************/
void CsrBtSppActivateStateSppDeactivateReqHandler(SppInstanceData_t *instData)
{
    CsrBtCmCancelAcceptConnectReqSend(instData->myAppHandle, instData->serverChannel);
    instData->state = Deactivate_s;
#ifdef CSR_BT_GLOBAL_INSTANCE
    sppRemoveInstanceInSppManager(instData);
#endif /* CSR_BT_GLOBAL_INSTANCE */
}

/*************************************************************************************
    Peer side has connected or accept connect has timed out.
************************************************************************************/
void CsrBtSppDeOrActivatedStateCmConnectAcceptCfmHandler(SppInstanceData_t *instData)
{
    CsrBtCmConnectAcceptCfm *cmPrim;
    RFC_PORTNEG_VALUES_T portPar;

    CsrBtPortParDefault(&(portPar));

    cmPrim = (CsrBtCmConnectAcceptCfm *)instData->recvMsgP;

    if(instData->state == Deactivate_s)
    {
        /* If the incoming connection was successful, disconnect it. SDS unregistration is handled on cancel accept connect confirm */
        if (cmPrim->resultSupplier == CSR_BT_SUPPLIER_CM &&
            cmPrim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
            {
                CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_DATA_CHANNEL,cmPrim->deviceAddr,
                                                    cmPrim->btConnId);
                CsrBtCmDisconnectReqSend(cmPrim->btConnId);
                instData->subState = SPP_SUB_CROSSING_CONNECT_STATE;
            }
    }
    else
    {
#ifdef INSTALL_SPP_OUTGOING_CONNECTION
        if (!instData->connectReqActivated)
#endif /* INSTALL_SPP_OUTGOING_CONNECTION */
        {
            instData->cancelReceived = FALSE;

            if (cmPrim->resultSupplier == CSR_BT_SUPPLIER_CM &&
                cmPrim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
            {
                CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_DATA_CHANNEL,cmPrim->deviceAddr,
                        cmPrim->btConnId);
                if (instData->sppConnId == SPP_NO_CONNID)
                {
                    instData->sppConnId = cmPrim->btConnId;
                }
                csrBtSppConnectIndSend(instData, cmPrim->deviceAddr,
                    cmPrim->profileMaxFrameSize, FALSE, portPar,
                    CSR_BT_RESULT_CODE_SPP_SUCCESS,
                    CSR_BT_SUPPLIER_SPP);
            }
            else
            {
                csrBtSppConnectIndSend(instData, cmPrim->deviceAddr,
                    cmPrim->profileMaxFrameSize, FALSE, portPar,
                    cmPrim->resultCode, cmPrim->resultSupplier);
            }
        }

        if (cmPrim->resultSupplier == CSR_BT_SUPPLIER_CM &&
            cmPrim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
        {
            /* a peer device has connected */
            instData->numberOfConnections++;
            instData->state                = Connected_s;
            instData->currentLinkMode    = CSR_BT_ACTIVE_MODE;
            if (instData->sppConnId == SPP_NO_CONNID)
            {
                instData->sppConnId = cmPrim->btConnId;
            }
#ifdef INSTALL_SPP_OUTGOING_CONNECTION
            if (instData->connectReqActivated)
            {
                /*A connection has been initiated from the application so
                 *  this is disconnected - save the address for reporting
                 *  failed connect */
                CsrBtCmDisconnectReqSend(cmPrim->btConnId);
                instData->subState = SPP_SUB_CROSSING_CONNECT_STATE;
            }
            else
#endif /* INSTALL_SPP_OUTGOING_CONNECTION */
            {
                /* Send a status indication (connect) to the data handle after the connect */
                instData->bdAddr    = cmPrim->deviceAddr;
                csrBtSppStatusIndSend(instData,
                                 TRUE,
                                 cmPrim->profileMaxFrameSize,
                                 cmPrim->deviceAddr);
            }
        }
        else
        {
            instData->state    = Idle_s;

#ifdef INSTALL_SPP_OUTGOING_CONNECTION
            if(instData->connectReqActivated)
            {
                /* An outgoing connect has been requested, and the cancel
                 * connect accept operation has finished. Start a search
                 * to complete the queued connect.*/
                csrBtSppSdcFeatureSearch(instData);
            }
            else
#endif /* INSTALL_SPP_OUTGOING_CONNECTION */
            {
                instData->subState = SPP_SUB_IDLE_STATE;
            }
        }

        /* always send the unregister as a new register is done when
         * activated again.  ignore the unregister cfm in either idle or
         * connected states */
        CsrBtCmSdsUnRegisterReqSend(instData->myAppHandle, instData->sdsRecHandle, CSR_BT_CM_CONTEXT_UNUSED);
        instData->sdsUnregInProgress = TRUE;
    }
}

/*************************************************************************************
    Time out during activate. Unregistering in sds complete
************************************************************************************/
void CsrBtSppActivatedStateSdsUnregisterCfmHandler(SppInstanceData_t *instData)
{
    CsrBtCmSdsUnregisterCfm * cmPrim = (CsrBtCmSdsUnregisterCfm *) instData->recvMsgP;
    /* the cancel is successful so send the cancel confirmation to
     * the higher layer and set instance variables */
    if (cmPrim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && cmPrim->resultSupplier == CSR_BT_SUPPLIER_CM)
    {
#ifdef INSTALL_SPP_OUTGOING_CONNECTION
        if (!instData->connectReqActivated)
#endif /* INSTALL_SPP_OUTGOING_CONNECTION */
        {
            RFC_PORTNEG_VALUES_T   portPar;

            CsrBtPortParDefault( &(portPar));
            csrBtSppConnectIndSend(instData, instData->bdAddr, 0,
                FALSE, portPar,
                CSR_BT_RESULT_CODE_SPP_TIMEOUT, CSR_BT_SUPPLIER_SPP);
            instData->state             = Idle_s;
            instData->subState          = SPP_SUB_IDLE_STATE;
            instData->sdsRecordObtain   = FALSE;
        }
    }
    else
    {
        /* unregister failed. We can not do anything else than accept.
           Activate will later fail */
        RFC_PORTNEG_VALUES_T   portPar;

        CsrBtPortParDefault( &(portPar));
        csrBtSppConnectIndSend(instData, instData->bdAddr, 0,
            FALSE, portPar, cmPrim->resultCode, cmPrim->resultSupplier);
        instData->state    = Idle_s;
        instData->subState = SPP_SUB_IDLE_STATE;
    }
}

/*************************************************************************************
    Unregistering in sds complete. Check result and try again if fail
************************************************************************************/
void CsrBtSppIdleOrConnectedStateSdsUnregisterCfmHandler(SppInstanceData_t *instData)
{
    CsrBtCmSdsUnregisterCfm    * cmPrim;

    cmPrim = (CsrBtCmSdsUnregisterCfm *) instData->recvMsgP;
    /* the cancel is successful so send the cancel confirmation to
     * the higher layer and set instance variables */
    if (cmPrim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && cmPrim->resultSupplier == CSR_BT_SUPPLIER_CM)
    {
        /* restore save queue */
        instData->sdsUnregInProgress    = FALSE;
        instData->restoreSppFlag        = TRUE;
        instData->sdsRecordObtain        = FALSE;
        csrBtSppHouseCleaningSend(instData);
    }
    else
    {
        /* unregister failed. We can not do anything else than
         * accept and restore the save queue. Activate will later
         * fail */
        instData->sdsUnregInProgress    = FALSE;
        instData->restoreSppFlag        = TRUE;
        csrBtSppHouseCleaningSend(instData);
    }
}


#ifdef CSR_BT_INSTALL_SPP_EXTENDED
/*************************************************************************************
    Connect to client when service is originally activated. Cancel the activate
    and wait for confirmation before connecting
************************************************************************************/
void CsrBtSppActivateStateSppExtendedConnectReqHandler(SppInstanceData_t *instData)
{
    CsrBtSppExtendedConnectReq *sppPrim;

    sppPrim = (CsrBtSppExtendedConnectReq *) instData->recvMsgP;
    /* Store the data related to the connection attempt but remain in
     * Active_s state */
    if (instData->numberOfConnections < MAX_NUMBER_OF_CONNECTIONS)
    {
        instData->numberOfConnections++;
        instData->ctrlHandle            = sppPrim->phandle;

        instData->bdAddr                = sppPrim->deviceAddr;
        instData->role                  = sppPrim->role;
        instData->requestPortPar        = sppPrim->requestPortPar;
        instData->validPortPar          = sppPrim->validPortPar;
        *instData->portPar              = sppPrim->portPar;
        instData->extendedConnect       = TRUE;
        instData->connectReqActivated   = TRUE;
        instData->extendedProfileUuid   = sppPrim->profileUuid;
#ifdef INSTALL_SPP_CUSTOM_SECURITY_SETTINGS
        instData->secOutgoing           = sppPrim->secLevel;
#endif

        CsrBtSppRegisterDataPathHandleCfmSend(instData);
        CsrBtCmCancelAcceptConnectReqSend(instData->myAppHandle, instData->serverChannel);
    }
    else
        /* Max number of connections already exceeded, so reject any
         * new attempt */
    {
        csrBtSppConnectIndSend(instData, sppPrim->deviceAddr, 0,
            sppPrim->validPortPar, sppPrim->portPar,
            CSR_BT_RESULT_CODE_SPP_MAX_NUM_OF_CONNECTIONS,
            CSR_BT_SUPPLIER_SPP);
    }
}
#endif /* CSR_BT_INSTALL_SPP_EXTENDED */

#ifdef CSR_BT_INSTALL_SPP_EXTENDED
/*************************************************************************************
    Connect to client when service is originally activated. Cancel the activate
    and wait for confirmation before connecting
************************************************************************************/
void CsrBtSppActivateStateSppExtendedUuidConnectReqHandler(SppInstanceData_t *instData)
{
    CsrBtSppExtendedUuidConnectReq *sppPrim;

    sppPrim = (CsrBtSppExtendedUuidConnectReq *) instData->recvMsgP;
    /* Store the data related to the connection attempt but remain in Active_s state */
    if (instData->numberOfConnections < MAX_NUMBER_OF_CONNECTIONS)
    {
        instData->numberOfConnections++;
        instData->ctrlHandle            = sppPrim->phandle;

        instData->bdAddr                = sppPrim->deviceAddr;
        instData->role                  = sppPrim->role;
        instData->requestPortPar        = sppPrim->requestPortPar;
        instData->validPortPar          = sppPrim->validPortPar;
        *instData->portPar              = sppPrim->portPar;
        instData->extendedConnect       = TRUE;
        instData->connectReqActivated   = TRUE;
        instData->extendedProfileUuid   = 0; /* 128 bit UUID */
        CsrMemCpy(instData->uuid128Profile, sppPrim->profileUuid, sizeof(CsrBtUuid128));
#ifdef INSTALL_SPP_CUSTOM_SECURITY_SETTINGS
        instData->secOutgoing           = sppPrim->secLevel;
#endif

        CsrBtSppRegisterDataPathHandleCfmSend(instData);
        CsrBtCmCancelAcceptConnectReqSend(instData->myAppHandle, instData->serverChannel);
    }
    else
    {
        /* Max number of connections already exceeded, so reject any
         * new attempt */
        csrBtSppConnectIndSend(instData, sppPrim->deviceAddr, 0,
            sppPrim->validPortPar, sppPrim->portPar,
            CSR_BT_RESULT_CODE_SPP_MAX_NUM_OF_CONNECTIONS,
            CSR_BT_SUPPLIER_SPP);
    }
}
#endif /* CSR_BT_INSTALL_SPP_EXTENDED */

/*************************************************************************************
    Cancel of activate confirmed. Continue with connection.
************************************************************************************/
void CsrBtSppActivateStateCmCancelAcceptConnectCfmHandler(SppInstanceData_t *instData)
{
    CsrBtCmCancelAcceptConnectCfm *cmPrim;

    cmPrim = (CsrBtCmCancelAcceptConnectCfm *) instData->recvMsgP;

    /* the cancel is successful send the cancel confirmation to the
     * higher layer and set instance variables */
    if (cmPrim->resultSupplier == CSR_BT_SUPPLIER_CM &&
        cmPrim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
    {
        instData->state    = Idle_s;
        instData->subState = SPP_SUB_IDLE_STATE;
        if (instData->cancelReceived)
        {
            RFC_PORTNEG_VALUES_T  portPar;

            CsrBtPortParDefault( &(portPar));
            csrBtSppConnectIndSend(instData, instData->bdAddr, 0, FALSE,
                portPar, CSR_BT_RESULT_CODE_SPP_CANCELLED_CONNECT_ATTEMPT,
                CSR_BT_SUPPLIER_SPP);
            instData->cancelReceived = FALSE;
        }
        else
        {
#ifdef CSR_BT_INSTALL_SPP_EXTENDED
            if (!instData->extendedConnect)
#endif /* CSR_BT_INSTALL_SPP_EXTENDED */
            {
                /* unregister the service in SDS and ignore the
                 * unregister cfm in idle state */
                CsrBtCmSdsUnRegisterReqSend(instData->myAppHandle, instData->sdsRecHandle, CSR_BT_CM_CONTEXT_UNUSED);
                instData->sdsUnregInProgress    = TRUE;
            }
#ifdef INSTALL_SPP_OUTGOING_CONNECTION
            csrBtSppSdcFeatureSearch(instData);
#endif /* INSTALL_SPP_OUTGOING_CONNECTION */
        }
    }
    else
    {
        /* the cancel may fail if a connect is in progress. Wait for
         * CM_ACCEPT_CONNECT_CFM and then disconnect */
    }
}

void CsrBtSppDeactivateStateCmDisconnectIndHandler(SppInstanceData_t *instData)
{
#ifndef EXCLUDE_CSR_AM_MODULE
#ifdef CSR_BT_INSTALL_SPP_EXTENDED
    instData->audioState = audioOff_s;
#endif /* CSR_BT_INSTALL_SPP_EXTENDED */
#endif /* !EXCLUDE_CSR_AM_MODULE */

    if (instData->sdsUnregInProgress == TRUE)
    {
        CsrMessageQueuePush(&instData->saveQueue, CSR_BT_CM_PRIM, instData->recvMsgP);
        instData->recvMsgP = NULL;
    }
    else
    {
#ifdef CSR_BT_INSTALL_CM_DISABLE_AUTO_DISC_RESP
        if (!(((CsrBtCmDisconnectInd *) instData->recvMsgP)->localTerminated))
        {
            /* For remote disconnections, profile needs to respond to RFC_DISCONNECT_IND. */
            CsrBtCmRfcDisconnectRspSend(((CsrBtCmDisconnectInd *) instData->recvMsgP)->btConnId);
        }
#endif
        CsrBtCmLogicalChannelTypeReqSend(CSR_BT_NO_ACTIVE_LOGICAL_CHANNEL,
                                         instData->bdAddr,
                                         instData->sppConnId);

        /* If we disconnected because an incoming connection crossed with deactivation, wait for
         * cancel accept connect to send deactivate confirmation */
        if(instData->subState != SPP_SUB_CROSSING_CONNECT_STATE)
        {
            csrBtSppDeactivateCfmSend(instData, CSR_BT_RESULT_CODE_SPP_SUCCESS,
                CSR_BT_SUPPLIER_SPP);
            instData->state    = Idle_s;
            if (instData->numberOfConnections > 0)
            {
                instData->numberOfConnections--;
            }
            instData->sppConnId  = SPP_NO_CONNID;
            instData->restoreSppFlag        = TRUE;
            instData->cancelReceived = FALSE;
            csrBtSppHouseCleaningSend(instData);
        }
        instData->subState = SPP_SUB_IDLE_STATE;
    }
}

/*************************************************************************************
    Deactivate the service is complete. Inform app layer and change state to idle.
************************************************************************************/
void CsrBtSppDeactivateStateCmCancelAcceptConnectCfmHandler(SppInstanceData_t *instData)
{
    CsrBtCmCancelAcceptConnectCfm *cmPrim;

    cmPrim = (CsrBtCmCancelAcceptConnectCfm *) instData->recvMsgP;

#ifndef EXCLUDE_CSR_AM_MODULE
#ifdef CSR_BT_INSTALL_SPP_EXTENDED
    instData->audioState = audioOff_s;
#endif /* CSR_BT_INSTALL_SPP_EXTENDED */
#endif /* !EXCLUDE_CSR_AM_MODULE */

    /* the cancel is successful so send the cancel confirmation to
     * the higher layer and set instance variables. A crossing connection
     * may cause the NOTHING_TO_CANCEL code to be sent. Handle this like success */
    if((cmPrim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS || cmPrim->resultCode == CSR_BT_RESULT_CODE_CM_NOTHING_TO_CANCEL) && cmPrim->resultSupplier == CSR_BT_SUPPLIER_CM)
    {
        CsrBtCmSdsUnRegisterReqSend(instData->myAppHandle, instData->sdsRecHandle, CSR_BT_CM_CONTEXT_UNUSED);
        instData->sdsUnregInProgress = TRUE;
    }
    /* the cancel may fail if a connect is in progress */
    else
    {
        /* ignore the signal and wait for the connect that will follow */
    }
}

/*************************************************************************************
    Sds record has been removed. Send deactivate cfm to app
************************************************************************************/
void CsrBtSppDeactivateStateSdsUnregisterCfmHandler(SppInstanceData_t *instData)
{
    CsrBtCmSdsUnregisterCfm *cmPrim = (CsrBtCmSdsUnregisterCfm *) instData->recvMsgP;
    /* the cancel is successful so send the cancel confirmation to
     * the higher layer and set instance variables */
    if (cmPrim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && cmPrim->resultSupplier == CSR_BT_SUPPLIER_CM)
    {
        csrBtSppDeactivateCfmSend(instData, CSR_BT_RESULT_CODE_SPP_SUCCESS,
            CSR_BT_SUPPLIER_SPP);
        instData->state                 = Idle_s;
        instData->subState              = SPP_SUB_IDLE_STATE;
        instData->sdsUnregInProgress    = FALSE;
        instData->sdsRecordObtain       = FALSE;
    }
    else
    {
        /* unregister failed. We can not do anything else than
         * accept. Activate will later fail */
        csrBtSppDeactivateCfmSend(instData,
            cmPrim->resultCode, cmPrim->resultSupplier);
        instData->sdsUnregInProgress    = FALSE;
    }
}

/*************************************************************************************
    Data is received when not expected. Free the data and ignore the signal
************************************************************************************/
void CsrBtSppNotConnectedCmDataIndHandler(SppInstanceData_t *instData)
{
    CsrBtCmDataInd    *prim;

    prim = (CsrBtCmDataInd *) instData->recvMsgP;
    CsrPmemFree(prim->payload);
    /* return a response signal so CM can send more data if needed */
    CsrBtCmDataResSend(prim->btConnId);
}

/*************************************************************************************
    Receive a port neg and forward to app layer.
************************************************************************************/
void CsrBtSppConnectedStateCmPortnegIndHandler(SppInstanceData_t *instData)
{
    CsrBtCmPortnegInd *prim;
    CsrBtSppPortnegInd *sppPrim;

    prim = (CsrBtCmPortnegInd *) instData->recvMsgP;
    sppPrim = (CsrBtSppPortnegInd *) CsrPmemAlloc(sizeof(CsrBtSppPortnegInd));
    sppPrim->type = CSR_BT_SPP_PORTNEG_IND;
    sppPrim->queueId = instData->myAppHandle;
    if (instData->sppConnId == SPP_NO_CONNID)
    {
        instData->sppConnId = prim->btConnId;
    }
    sppPrim->portPar = prim->portPar;
    sppPrim->request = prim->request;
    sppPrim->serverChannel = instData->serverChannel;

    csrBtSppDataPathSend(instData, sppPrim);
}

/*************************************************************************************
    Receive a port neg response from the app layer and pass on to CM.
************************************************************************************/
void CsrBtSppConnectedStateSppPortnegResHandler(SppInstanceData_t *instData)
{
    CsrBtSppPortnegRes *sppPrim;

    sppPrim = (CsrBtSppPortnegRes *) instData->recvMsgP;
    CsrBtCmPortnegResSend(instData->sppConnId, &sppPrim->portPar);
}

#ifdef INSTALL_SPP_REMOTE_PORT_NEGOTIATION
/**************************************************************************************
    Receive a port neg request from the app layer and pass on to CM, or send a confirm
    immediately if not connected.
***************************************************************************************/
void CsrBtSppXStatePortnegReqHandler(SppInstanceData_t *instData)
{
    CsrBtSppPortnegReq *sppPrim = (CsrBtSppPortnegReq *)instData->recvMsgP;
    if (instData->state == Connected_s)
    {
        instData->sppPortNegPending = TRUE;
        CsrBtCmPortnegReqSend(instData->sppConnId, &sppPrim->portPar, 0);
    }
    else
    {/* Reject */
        CsrBtSppPortnegCfm *cfmPrim = (CsrBtSppPortnegCfm *)CsrPmemAlloc(sizeof(CsrBtSppPortnegCfm));

        cfmPrim->type            = CSR_BT_SPP_PORTNEG_CFM;
        cfmPrim->serverChannel   = sppPrim->serverChannel;
        cfmPrim->portPar         = sppPrim->portPar;
        cfmPrim->queueId         = instData->myAppHandle;
        cfmPrim->resultCode      = CSR_BT_RESULT_CODE_SPP_COMMAND_DISALLOWED;
        cfmPrim->resultSupplier  = CSR_BT_SUPPLIER_SPP;

        csrBtSppDataPathSend(instData, cfmPrim);
    }
}

void CsrBtCppConnectedStateCmPortnegCfmHandler(SppInstanceData_t * instData)
{
    CsrBtSppPortnegCfm *cfmPrim = (CsrBtSppPortnegCfm *)CsrPmemAlloc(sizeof(CsrBtSppPortnegCfm));
    CsrBtCmPortnegCfm *cmPrim = (CsrBtCmPortnegCfm *)instData->recvMsgP;

    cfmPrim->type            = CSR_BT_SPP_PORTNEG_CFM;
    cfmPrim->serverChannel   = instData->serverChannel;
    cfmPrim->portPar         = cmPrim->portPar;
    cfmPrim->queueId         = instData->myAppHandle;
    cfmPrim->resultCode      = CSR_BT_RESULT_CODE_SPP_SUCCESS;
    cfmPrim->resultSupplier  = CSR_BT_SUPPLIER_SPP;

    csrBtSppDataPathSend(instData, cfmPrim);

    instData->sppPortNegPending = FALSE;
}
#endif /* INSTALL_SPP_REMOTE_PORT_NEGOTIATION */


/*
 * CSR_BT_SPP_GET_INSTANCES_QID_REQ is received. Return the registered SPP
 * instances in a CSR_BT_SPP_GET_INSTANCES_QID_CFM
 */
void CsrBtSppAllStateSppGetInstancesQIDReqHandler(SppInstanceData_t *instData)
{
    CsrBtSppGetInstancesQidReq *sppPrim_req;
    CsrBtSppGetInstancesQidCfm *sppPrim_cfm;
    SppInstancesPool_t          *ptr;
    CsrUintFast8                 offset;

#ifndef EXCLUDE_CSR_EXCEPTION_HANDLER_MODULE
    if (instData->myAppHandle == CSR_BT_SPP_IFACEQUEUE)
#endif
    {
        sppPrim_req=(CsrBtSppGetInstancesQidReq *) instData->recvMsgP;

        sppPrim_cfm = CsrPmemAlloc(sizeof(CsrBtSppGetInstancesQidCfm));

        sppPrim_cfm->type = CSR_BT_SPP_GET_INSTANCES_QID_CFM;
        sppPrim_cfm->phandlesListSize = instData->numberOfSppInstances;

        if (instData->numberOfSppInstances == 0)
        {
            sppPrim_cfm->phandlesList = NULL;
        }
        else
        {
            sppPrim_cfm->phandlesList=CsrPmemAlloc(sizeof(CsrSchedQid) *
                                              instData->numberOfSppInstances);

            ptr = instData->sppInstances;
            offset = 0;

            while(ptr)
            {
                CsrMemCpy(&sppPrim_cfm->phandlesList[offset],
                       ptr->phandles,
                       sizeof(CsrSchedQid) * ptr->numberInPool);

                offset += ptr->numberInPool;
                ptr = ptr->next;
            }
        }

        CsrBtSppMessagePut(sppPrim_req->phandle,sppPrim_cfm);
    }
#ifndef EXCLUDE_CSR_EXCEPTION_HANDLER_MODULE
    else
    {
        CsrGeneralException(CsrBtSppLto,
                            0,
                            CSR_BT_SPP_PRIM,
                            CSR_BT_SPP_GET_INSTANCES_QID_REQ,
                            instData->state,
                            "Task not SPP-manager");
    }
#endif
}

void CsrBtSppAllStateSppRegisterQIDReqHandler(SppInstanceData_t *instData)
{
    CsrBtSppRegisterQidReq *prim;
    SppInstancesPool_t *ptr;
    SppInstancesPool_t *prev;

    prim = (CsrBtSppRegisterQidReq *) instData->recvMsgP;

#ifndef EXCLUDE_CSR_EXCEPTION_HANDLER_MODULE
    if (instData->myAppHandle == CSR_BT_SPP_IFACEQUEUE)
#endif
    {
        ptr = instData->sppInstances;
        prev = NULL;

        while((ptr) && (ptr->numberInPool == SPP_INSTANCES_POOL_SIZE))
        {
            prev = ptr;
            ptr = ptr->next;
        }

        if (ptr)
        {
            /* Do nothing */
        }
        else
        {
            ptr = CsrPmemZalloc(sizeof(SppInstancesPool_t));

            if (prev)
            {
                prev->next = ptr;
            }
            else
            {
                instData->sppInstances = ptr;
            }
        }

        ptr->phandles[ptr->numberInPool++] = prim->phandle;
        instData->numberOfSppInstances++;
    }
#ifndef EXCLUDE_CSR_EXCEPTION_HANDLER_MODULE
    else
    {
        CsrGeneralException(CsrBtSppLto,
                            0,
                            CSR_BT_SPP_PRIM,
                            CSR_BT_SPP_REGISTER_QID_REQ,
                            instData->state,
                            "Task not SPP-manager");
    }
#endif
}

void CsrBtSppIdleStateCmCancelAcceptConnectCfmHandler(SppInstanceData_t *instData)
{
    /* A failed CancelAcceptConnectCfm received in Idle state can be
     * result of a connectReq that fails and is crossed by an incoming
     * connection */
    CsrBtCmCancelAcceptConnectCfm *cmPrim;

    cmPrim = (CsrBtCmCancelAcceptConnectCfm *) instData->recvMsgP;

    if(cmPrim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && cmPrim->resultSupplier == CSR_BT_SUPPLIER_CM)
    {
        ;
    }
    else
    {
#ifdef INSTALL_SPP_OUTGOING_CONNECTION
        instData->connectReqActivated = FALSE;
#endif
        if (instData->numberOfConnections > 0)
        {
            instData->numberOfConnections--;
        }
    }
}

#ifndef EXCLUDE_CSR_AM_MODULE
#ifdef CSR_BT_INSTALL_SPP_EXTENDED
void CsrBtSppCmMapScoPcmHandler(SppInstanceData_t * instData)
{
    CsrBtSppAudioAcceptConnectInd *prim;
    CsrBtCmMapScoPcmInd *msg = instData->recvMsgP;

    if (instData->audioState == audioAcceptReq_s)
    {
        instData->audioState = audioAcceptInProgress_s;
        prim = (CsrBtSppAudioAcceptConnectInd *)CsrPmemAlloc(sizeof(CsrBtSppAudioAcceptConnectInd));
        prim->type = CSR_BT_SPP_AUDIO_ACCEPT_CONNECT_IND;
        prim->queueId = instData->myAppHandle;
        prim->serverChannel = instData->serverChannel;
        prim->linkType = msg->linkType;
        CsrBtSppMessagePut(instData->ctrlHandle, prim);
    }
    else
    {
        CsrBtCmMapScoPcmResSend(instData->sppConnId,
                                HCI_ERROR_REJ_BY_REMOTE_NO_RES,
                                NULL,
                                CSR_BT_PCM_DONT_CARE,
                                FALSE);
    }
}

void CsrBtSppMapScoPcmResHandler(SppInstanceData_t * instData)
{
    CsrBtSppAudioAcceptConnectRes *prim;
    CsrBtCmScoCommonParms *scoParms = NULL;

    prim = (CsrBtSppAudioAcceptConnectRes *)instData->recvMsgP;

    if(prim->acceptParameters)
    {
        scoParms = CsrPmemAlloc(sizeof(CsrBtCmScoCommonParms));
        scoParms->audioQuality = prim->acceptParameters->packetTypes;
        scoParms->txBandwidth = prim->acceptParameters->txBandwidth;
        scoParms->rxBandwidth = prim->acceptParameters->rxBandwidth;
        scoParms->maxLatency = prim->acceptParameters->maxLatency;
        scoParms->voiceSettings = prim->acceptParameters->contentFormat;
        scoParms->reTxEffort = prim->acceptParameters->reTxEffort;
        CsrPmemFree(prim->acceptParameters);
    }

    CsrBtCmMapScoPcmResSend(instData->sppConnId,
                            prim->acceptResponse,
                            scoParms,
                            prim->pcmSlot,
                            prim->pcmReassign);
}
#endif /* CSR_BT_INSTALL_SPP_EXTENDED */
#endif /* !EXCLUDE_CSR_AM_MODULE */

#ifdef INSTALL_SPP_OUTGOING_CONNECTION
void CsrBtSppInitStateSppCancelConnectReqHandler(SppInstanceData_t * instData)
{
    CsrUint16                eventClass;
    void                    *pMsg;
    CsrMessageQueueType    *tempQueue  = NULL;

    instData->cancelReceived = FALSE;
    while(CsrMessageQueuePop(&instData->saveQueue, &eventClass, &pMsg))
    {
        if (eventClass == CSR_BT_SPP_PRIM && (*((CsrBtSppPrim*)pMsg) == CSR_BT_SPP_CONNECT_REQ))
        {
            CsrBtSppConnectReq *prim;
            RFC_PORTNEG_VALUES_T   portPar;

            CsrBtPortParDefault( &(portPar));
            prim = (CsrBtSppConnectReq *) pMsg;
            if (instData->numberOfConnections > 0)
            {
                instData->numberOfConnections--;
            }
            instData->ctrlHandle = prim->phandle;

            CsrBtSppRegisterDataPathHandleCfmSend(instData);
            csrBtSppConnectIndSend(instData, instData->bdAddr, 0,
                FALSE, portPar,
                CSR_BT_RESULT_CODE_SPP_CANCELLED_CONNECT_ATTEMPT,
                CSR_BT_SUPPLIER_SPP);
            CsrPmemFree(pMsg);
        }
        else
        {
            CsrMessageQueuePush(&tempQueue, eventClass, pMsg);
        }
    }
    while (CsrMessageQueuePop(&tempQueue, &eventClass, &pMsg))
    {
        CsrMessageQueuePush(&instData->saveQueue, eventClass, pMsg);
    }

}

void CsrBtSppXStateSppCancelConnectReqHandler(SppInstanceData_t * instData)
{
    instData->cancelReceived = TRUE;

    if (instData->subState == SPP_SUB_IDLE_STATE)
    {
        RFC_PORTNEG_VALUES_T portPar;

        CsrBtPortParDefault( &(portPar));
        if (instData->numberOfConnections > 0)
        {
            instData->numberOfConnections--;
        }
        if (instData->connectReqActivated)
        {
            if (instData->state == Idle_s)
            {   /*Only send if master state is Idle_s, other states are handled elsewhere*/
                    csrBtSppConnectIndSend(instData, instData->bdAddr, 0,
                        FALSE, portPar,
                        CSR_BT_RESULT_CODE_SPP_CANCELLED_CONNECT_ATTEMPT,
                        CSR_BT_SUPPLIER_SPP);
                    instData->connectReqActivated = FALSE;
            }
        }
        else
        {
            csrBtSppConnectIndSend(instData, instData->bdAddr, 0,
                FALSE, portPar,
                CSR_BT_RESULT_CODE_SPP_NOTHING_TO_CANCEL,
                CSR_BT_SUPPLIER_SPP);
        }
    }
    else
    { /* Our connection request is no longer in process */
        instData->connectReqActivated = FALSE;
        CsrBtUtilRfcConCancel((void*)(instData),instData->sdpSppSearchConData);
    }
}

void CsrBtSppConnectedStateSppCancelConnectReqHandler(SppInstanceData_t * instData)
{
    instData->cancelReceived = TRUE;

    if (instData->subState != SPP_SUB_CROSSING_CONNECT_STATE)
    {
        if (instData->sdsUnregInProgress == TRUE)
        {
            CsrBtSppSaveMessage(instData);
        }
        else
        {
            CsrBtCmDisconnectReqSend(instData->sppConnId);
            instData->restoreSppFlag        = TRUE;
            csrBtSppHouseCleaningSend(instData);
        }
    }
}

void CsrBtSppDeactivateStateSppCancelConnectReqHandler(SppInstanceData_t * instData)
{
    CSR_UNUSED(instData);
}
#endif /* INSTALL_SPP_OUTGOING_CONNECTION */

#ifdef INSTALL_SPP_CUSTOM_SECURITY_SETTINGS
void CsrBtSppSecurityInReqHandler(SppInstanceData_t *instData)
{
    CsrBtResultCode rval;
    CsrBtSppSecurityInReq *prim;

    prim = (CsrBtSppSecurityInReq*)instData->recvMsgP;

    rval = CsrBtScSetSecInLevel(&instData->secIncoming, prim->secLevel,
        CSR_BT_SERIAL_PORT_MANDATORY_SECURITY_INCOMING,
        CSR_BT_SERIAL_PORT_DEFAULT_SECURITY_INCOMING,
        CSR_BT_RESULT_CODE_SPP_SUCCESS,
        CSR_BT_RESULT_CODE_SPP_UNACCEPTABLE_PARAMETER);

    CsrBtSppSecurityInCfmSend(prim->appHandle,
        rval, CSR_BT_SUPPLIER_SPP);
}

void CsrBtSppSecurityOutReqHandler(SppInstanceData_t *instData)
{
    CsrBtResultCode rval;
    CsrBtSppSecurityOutReq *prim;

    prim = (CsrBtSppSecurityOutReq*)instData->recvMsgP;

    rval = CsrBtScSetSecOutLevel(&instData->secOutgoing, prim->secLevel,
        CSR_BT_SERIAL_PORT_MANDATORY_SECURITY_OUTGOING,
        CSR_BT_SERIAL_PORT_DEFAULT_SECURITY_OUTGOING,
        CSR_BT_RESULT_CODE_SPP_SUCCESS,
        CSR_BT_RESULT_CODE_SPP_UNACCEPTABLE_PARAMETER);

    CsrBtSppSecurityOutCfmSend(prim->appHandle,
        rval, CSR_BT_SUPPLIER_SPP);
}
#endif /* INSTALL_SPP_CUSTOM_SECURITY_SETTINGS */


void CsrBtSppSdpDeinitReq(SppInstanceData_t *instData)
{
#ifdef INSTALL_SPP_OUTGOING_CONNECTION
    /* Check searchOngoing to see if another connect request has come while this is getting freed,
     * if yes then same resources can be used for this request. */
    if (instData->sdpSppSearchConData)
    {
        CsrBtUtilSdcRfcDeinit(&instData->sdpSppSearchConData);
        instData->sdpSppSearchConData = NULL;
    }
#else
    CSR_UNUSED(instData);
#endif /* INSTALL_SPP_OUTGOING_CONNECTION */
}

#ifdef INSTALL_SPP_CUSTOM_SECURITY_SETTINGS
void CsrBtSppSecurityInCfmSend(CsrSchedQid appHandle, CsrBtResultCode resultCode,
    CsrBtSupplier resultSupplier)
{
    CsrBtSppSecurityInCfm *prim;
    prim = (CsrBtSppSecurityInCfm*)CsrPmemAlloc(sizeof(CsrBtSppSecurityInCfm));
    prim->type = CSR_BT_SPP_SECURITY_IN_CFM;
    prim->resultCode     = resultCode;
    prim->resultSupplier = resultSupplier;
    CsrBtSppMessagePut(appHandle, prim);
}

void CsrBtSppSecurityOutCfmSend(CsrSchedQid appHandle, CsrBtResultCode resultCode,
    CsrBtSupplier resultSupplier)
{
    CsrBtSppSecurityOutCfm *prim;
    prim = (CsrBtSppSecurityOutCfm*)CsrPmemAlloc(sizeof(CsrBtSppSecurityOutCfm));
    prim->type = CSR_BT_SPP_SECURITY_OUT_CFM;
    prim->resultCode     = resultCode;
    prim->resultSupplier = resultSupplier;
    CsrBtSppMessagePut(appHandle, prim);
}
#endif /* INSTALL_SPP_CUSTOM_SECURITY_SETTINGS */

#ifdef INSTALL_SPP_OUTGOING_CONNECTION
void CsrBtSppRfcSdcConResultHandler(void                       *inst,
                                    CsrUint8                    localServerCh,
                                    CsrUint32                   sppConnId,
                                    CsrBtDeviceAddr             deviceAddr,
                                    CsrUint16                   maxFrameSize,
                                    CsrBool                     validPortPar,
                                    RFC_PORTNEG_VALUES_T        portPar,
                                    CsrBtResultCode             resultCode,
                                    CsrBtSupplier               resultSupplier,
                                    CmnCsrBtLinkedListStruct   *sdpTagList)
{
    SppInstanceData_t *instData = (SppInstanceData_t *)inst;

    CSR_UNUSED(localServerCh);

    instData->searchOngoing = FALSE;
    instData->bdAddr        = deviceAddr;
    instData->connectReqActivated   = FALSE;

    if (resultSupplier == CSR_BT_SUPPLIER_CM &&
        resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
    {
        CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_DATA_CHANNEL,deviceAddr,sppConnId);

        if (instData->sppConnId == SPP_NO_CONNID)
        {
            instData->sppConnId = sppConnId;
        }
        csrBtSppConnectIndSend(instData, deviceAddr, maxFrameSize,
            validPortPar, portPar,
            CSR_BT_RESULT_CODE_SPP_SUCCESS, CSR_BT_SUPPLIER_SPP);
        instData->state = Connected_s;
        instData->currentLinkMode    = CSR_BT_ACTIVE_MODE;
        csrBtSppStatusIndSend(instData, TRUE, maxFrameSize, deviceAddr);
    }
    else
    {
        instData->state    = Idle_s;
        instData->subState = SPP_SUB_IDLE_STATE;
        if (instData->numberOfConnections > 0)
        {
            instData->numberOfConnections--;
        }
        if (instData->cancelReceived)
        {
            instData->cancelReceived = FALSE;
            csrBtSppConnectIndSend(instData, instData->bdAddr,
                0, FALSE, portPar,
                CSR_BT_RESULT_CODE_CM_CANCELLED,
                CSR_BT_SUPPLIER_CM); /* CM spoofing */
        }
        else
        {
            csrBtSppConnectIndSend(instData, instData->bdAddr,
                0, FALSE, portPar, resultCode, resultSupplier);
        }
    }

    CsrBtUtilBllFreeLinkedList(&sdpTagList, CsrBtUtilBllPfreeWrapper);

    /* This is not immediately freed since the caller uses the instance for more processing. */
    CsrBtSppSdpDeinitReq(instData);
}


void CsrBtSppSdcSelectServiceHandler(void                    * inst,
                                     void                    * cmSdcRfcInstData,
                                     CsrBtDeviceAddr            deviceAddr,
                                     CsrUint8                 serverChannel,
                                     CsrUint16                entriesInSdpTaglist,
                                     CmnCsrBtLinkedListStruct * sdpTagList)
{
    SppInstanceData_t *instData = (SppInstanceData_t *)inst;
    CsrUint8        i;
    CsrBtUuid32     tmpUuid32 = 0;
    CsrUint16       idx = 0;
    CsrUint16       tmpResult;
    CsrUint16       dummy1,dummy2;
    CsrUint8        *string;
    CsrUint16       stringLen;
    CsrBool         getName = FALSE;
#ifdef CSR_BT_INSTALL_128_BIT_SERVICE_SEARCH
        CsrBtUuid128    *tmpUuid128 = (CsrBtUuid128 *)CsrPmemAlloc(sizeof(CsrBtUuid128));
#endif

    CSR_UNUSED(deviceAddr);
    CSR_UNUSED(serverChannel);

    instData->cmSdcRfcInstData = cmSdcRfcInstData;

    for (i=0; i < entriesInSdpTaglist; i++ )
    {
        if (CsrBtUtilSdrGetServiceUuid32AndResult(sdpTagList,i, &tmpUuid32, &tmpResult, &dummy1, &dummy2)
#ifdef CSR_BT_INSTALL_128_BIT_SERVICE_SEARCH
            || CsrBtUtilSdrGetServiceUuid128AndResult(sdpTagList,i, &tmpUuid128, &tmpResult, &dummy1, &dummy2)
#endif
            )
        {
            if (tmpUuid32 == CSR_BT_SPP_PROFILE_UUID)   /* Normal SPP connect */
            {
                CsrBtUuid32 serviceHandleTemp = 0;

                if (TRUE == CsrBtUtilSdrGetServiceHandle(sdpTagList, i, &serviceHandleTemp))
                {
                    if (instData->sdpServiceNameList == NULL)
                    {
                        instData->sdpServiceNameList =  (CsrBtSppServiceName *)CsrPmemAlloc(sizeof(CsrBtSppServiceName) * entriesInSdpTaglist);
                    }
                    if (instData->serviceHandleList == NULL)
                    {
                        instData->serviceHandleListSize = entriesInSdpTaglist;
                        instData->serviceHandleList =   (sppServiceHandle_t *)CsrPmemAlloc(sizeof(sppServiceHandle_t) * entriesInSdpTaglist);
                    }

                    instData->serviceHandleList[idx].selectServiceIdx = i;

                    getName = TRUE;
                    if (TRUE == CsrBtUtilSdrGetStringAttributeFromAttributeUuid(sdpTagList, i, CSR_BT_SERVICE_NAME_ATTRIBUTE_IDENTIFIER, &string, &stringLen))
                    {
                        if (stringLen > CSR_BT_MAX_FRIENDLY_NAME_LEN)
                        {
                            stringLen = CSR_BT_MAX_FRIENDLY_NAME_LEN;
                        }
                        CsrMemSet((CsrUint8 *) instData->sdpServiceNameList[idx].serviceName,0,CSR_BT_MAX_FRIENDLY_NAME_LEN);
                        CsrMemCpy((CsrUint8 *) instData->sdpServiceNameList[idx].serviceName, string, stringLen);
                        CsrUtf8StrTruncate(instData->sdpServiceNameList[idx].serviceName, stringLen);
                    }
                    else
                    {
                        instData->sdpServiceNameList[idx].serviceName[0] = '\0';
                    }
                    instData->sdpServiceNameList[idx].serviceHandle = instData->serviceHandleList[idx].serviceHandle = serviceHandleTemp;
                    idx++;
                }
            }
            else /* Extended SPP */
            {
                CsrUint16 *serviceHandleIndex = CsrPmemAlloc(sizeof(CsrUint16));

                *serviceHandleIndex = i;

                CsrBtUtilRfcConSetServiceHandleIndexList(instData, cmSdcRfcInstData, serviceHandleIndex, 1);

                break; /* ex for-loop */
            }
        }
    }


    if ( getName )
    {
        csrBtSppServiceNameIndSend(instData);
        CsrPmemFree(instData->sdpServiceNameList);
        instData->sdpServiceNameList = NULL;
    }
    else
    {
        /* State not handled */
    }
#ifdef CSR_BT_INSTALL_128_BIT_SERVICE_SEARCH
    CsrPmemFree(tmpUuid128);
#endif
}
#endif /* INSTALL_SPP_OUTGOING_CONNECTION */
#endif /* !EXCLUDE_CSR_BT_SPP_MODULE */
