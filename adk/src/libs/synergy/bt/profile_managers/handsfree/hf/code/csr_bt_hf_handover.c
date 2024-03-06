/******************************************************************************
 Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_handover_if.h"
#include "csr_bt_marshal_util.h"
#include "csr_bt_util.h"
#include "csr_bt_panic.h"
#include "csr_bt_hf_main.h"
#include "csr_bt_hf_util.h"
#include "csr_bt_hf_streams.h"


#define VM_SINK_MESSAGES                    0x0004

#ifdef CSR_LOG_ENABLE
#define CSR_BT_HF_LTSO_HANDOVER             0
#define CSR_BT_HF_HANDOVER_LOG_INFO(...)    CSR_LOG_TEXT_INFO((CsrBtHfLto, CSR_BT_HF_LTSO_HANDOVER, __VA_ARGS__))
#define CSR_BT_HF_HANDOVER_LOG_WARNING(...) CSR_LOG_TEXT_WARNING((CsrBtHfLto, CSR_BT_HF_LTSO_HANDOVER, __VA_ARGS__))
#define CSR_BT_HF_HANDOVER_LOG_ERROR(...)   CSR_LOG_TEXT_ERROR((CsrBtHfLto, CSR_BT_HF_LTSO_HANDOVER, __VA_ARGS__))
#else
#define CSR_BT_HF_HANDOVER_LOG_INFO(...)
#define CSR_BT_HF_HANDOVER_LOG_WARNING(...)
#define CSR_BT_HF_HANDOVER_LOG_ERROR(...)
#endif

static CsrBtMarshalUtilInst *hfConvInst;

static void convHfHsData(CsrBtMarshalUtilInst *conv, HfHsData_t *data)
{
    CsrBtMarshalUtilConvertObj(conv, data->allowed2SendCmData);
    CsrBtMarshalUtilConvertObj(conv, data->maxRfcFrameSize);
}

static void deserHfHsData(CsrBtMarshalUtilInst *conv, HfHsData_t *data)
{
    convHfHsData(conv, data);
}

static void serHfHsData(CsrBtMarshalUtilInst *conv, HfHsData_t *data)
{
    convHfHsData(conv, data);
}

#ifdef CSR_BT_INSTALL_HF_CONFIG_AUDIO
static void convAudioSetupParams(CsrBtMarshalUtilInst *conv,
                                 audioSetupParams_t *audioSetupParams)
{
    CsrBtMarshalUtilConvert(conv, audioSetupParams, sizeof(*audioSetupParams));
}

static void deserAudioSetupParams(CsrBtMarshalUtilInst *conv,
                                  audioSetupParams_t *audioSetupParams)
{
    convAudioSetupParams(conv, audioSetupParams);
}

static void serAudioSetupParams(CsrBtMarshalUtilInst *conv,
                                audioSetupParams_t *audioSetupParams)
{
    convAudioSetupParams(conv, audioSetupParams);
}
#endif

static void convRemoteHfInd(CsrCmnListElm_t *elem, void *data)
{
    CsrBtHfRemoteHfIndicator *remoteHfIndElem = (CsrBtHfRemoteHfIndicator *) elem;
    CsrBtMarshalUtilInst *conv = (CsrBtMarshalUtilInst *)data;

    if (remoteHfIndElem)
    {
        const CsrUint8 bitfieldBlobOffset = CsrOffsetOf(CsrBtHfRemoteHfIndicator, agHfIndicator) + 
                                                          sizeof(remoteHfIndElem->agHfIndicator);
        const CsrUint16 bitfieldBlobSize = sizeof(CsrBtHfRemoteHfIndicator) - bitfieldBlobOffset;
        void *bitfieldBlobPtr = &((CsrUint8 *) remoteHfIndElem)[bitfieldBlobOffset];
    
        CsrBtMarshalUtilConvertObj(conv, remoteHfIndElem->indvalue);
        CsrBtMarshalUtilConvertObj(conv, remoteHfIndElem->agHfIndicator);
        CsrBtMarshalUtilConvert(conv, bitfieldBlobPtr, bitfieldBlobSize);        
    }
}

static void serRemoteHfIndicatorList(CsrBtMarshalUtilInst *conv, HfInstanceData_t *hfInst)
{
    CsrBtMarshalUtilConvertObj(conv,((CsrCmnList_t*)&hfInst->remoteHfIndicatorList)->count);
    CsrCmnListIterate((CsrCmnList_t*) &hfInst->remoteHfIndicatorList,
                                convRemoteHfInd,
                                conv);            
}

static void deserRemoteHfIndicatorList(CsrBtMarshalUtilInst *conv, HfInstanceData_t *hfInst)
{
    CsrUint8 count = 0;
    CsrBtMarshalUtilConvertObj(conv, count);

    while (count)
    {
        CsrBtHfRemoteHfIndicator *recvdHfInd;
        recvdHfInd = REMOTE_HF_INDICATOR_ADD_LAST((CsrCmnList_t *)&hfInst->remoteHfIndicatorList);
        convRemoteHfInd((CsrCmnListElm_t*)recvdHfInd,conv);
        count--;
    }
}

static void convHfInstanceData(CsrBtMarshalUtilInst *conv, HfInstanceData_t *hfInst)
{
    const CsrUint8 bitfieldBlobOffset = CsrOffsetOf(HfInstanceData_t, instId) + sizeof(hfInst->instId);
    const CsrUint16 bitfieldBlobSize = sizeof(HfInstanceData_t) - bitfieldBlobOffset;
    void *bitfieldBlobPtr = &((CsrUint8 *) hfInst)[bitfieldBlobOffset];

    CsrBtMarshalUtilConvertObj(conv, hfInst->agIndicators);
    CsrBtMarshalUtilConvertObj(conv, hfInst->hfConnId);
    CsrBtMarshalUtilConvertObj(conv, hfInst->supportedFeatures);
    CsrBtMarshalUtilConvertObj(conv, hfInst->remoteVersion);
    CsrBtMarshalUtilConvertObj(conv, hfInst->obtainedServerCh);
    CsrBtMarshalUtilConvertObj(conv, hfInst->pcmSlot);
    CsrBtMarshalUtilConvertObj(conv, hfInst->hfQceCodecId);

    CsrBtMarshalUtilConvert(conv, bitfieldBlobPtr, bitfieldBlobSize);
}

static void deserHfInstanceData(CsrBtMarshalUtilInst *conv, HfInstanceData_t *hfInst)
{
    if (hfInst)
    {
        CsrBool present;

        deserRemoteHfIndicatorList(conv, hfInst);
        convHfInstanceData(conv, hfInst);

        present = hfInst->data ? TRUE : FALSE;
        CsrBtMarshalUtilConvertObj(conv, present);
        if (present)
        {
            if (!hfInst->data)
            {
                hfInst->data = CsrPmemZalloc(sizeof(*hfInst->data));
            }

            deserHfHsData(conv, hfInst->data);
        }

#ifdef CSR_BT_INSTALL_HF_CONFIG_AUDIO
        present = hfInst->audioSetupParams ? TRUE : FALSE;
        CsrBtMarshalUtilConvertObj(conv, present);
        if (present)
        {
            if (!hfInst->audioSetupParams)
            {
                hfInst->audioSetupParams = CsrPmemZalloc(sizeof(*hfInst->audioSetupParams));
            }

            deserAudioSetupParams(conv, hfInst->audioSetupParams);
        }
#endif
    }
}

static void serHfInstanceData(CsrBtMarshalUtilInst *conv, HfInstanceData_t *hfInst)
{
    CsrUint8 dlci;
    CsrUint16 cid = CM_GET_UINT16ID_FROM_BTCONN_ID(hfInst->hfConnId);
    CSR_BT_HF_HANDOVER_LOG_INFO("serHfInstanceData cid=%d", cid);
    Sink sink = StreamRfcommSink(cid);

    if (sink == NULL)
    {
        CsrPanic(CSR_TECH_BT,
                 CSR_BT_PANIC_MYSTERY,
                 "Sink is NULL");
    }

    dlci = SinkGetRfcommDlci(sink);

    if (dlci != RFC_INVALID_SERV_CHANNEL)
    {
        CsrBool present;

        CsrBtMarshalUtilConvertObj(conv, dlci);
        CsrBtMarshalUtilConvertObj(conv, hfInst->instId);
        serRemoteHfIndicatorList(conv, hfInst);
        convHfInstanceData(conv, hfInst);

        present = hfInst->data ? TRUE : FALSE;
        CsrBtMarshalUtilConvertObj(conv, present);
        if (present)
        {
            serHfHsData(conv, hfInst->data);
        }

#ifdef CSR_BT_INSTALL_HF_CONFIG_AUDIO
        present = hfInst->audioSetupParams ? TRUE : FALSE;
        CsrBtMarshalUtilConvertObj(conv, present);
        if (present)
        {
            serAudioSetupParams(conv, hfInst->audioSetupParams);
        }
#endif
    }
}

static void deserHfMainInstanceData(CsrBtMarshalUtilInst *conv,
                                    HfMainInstanceData_t *hfMainInst,
                                    const CsrBtDeviceAddr *addr)
{
    HfInstanceData_t *linkPtr;
    CsrUint8 dlci, instId;

    CsrBtMarshalUtilConvertObj(conv, dlci);

    /* Get the correct instance based on provided instId. */
    CsrBtMarshalUtilConvertObj(conv, instId);
    CsrBtHfSetCurrentConnIndexFromInstId(hfMainInst, instId);
    linkPtr = &hfMainInst->linkData[hfMainInst->index];

    /* Reset the complete instance before setting any unmarshalled values */
    CsrBtHfInitInstanceData(linkPtr);

    linkPtr->currentDeviceAddress = *addr;

    deserHfInstanceData(conv, linkPtr);

    /* Temporarily store dlci in hfConnId, convert it on commit */
    linkPtr->hfConnId = dlci;
}

static void serHfMainInstanceData(CsrBtMarshalUtilInst *conv,
                                  HfMainInstanceData_t *hfMainInst,
                                  const CsrBtDeviceAddr *addr)
{
    HfInstanceData_t *linkPtr = CsrBtHfGetConnectedInstFromBdAddr(hfMainInst, addr);

    if (linkPtr)
    {
        /* Marshal only for a valid connected instance. */
        serHfInstanceData(conv, linkPtr);
    }
}

static bool csrBtHfVeto(void)
{
    CsrUint8 i;
    bool veto = FALSE;
    CsrUint8 count = csrBtHfInstance.maxHSConnections + csrBtHfInstance.maxHFConnections;

    for (i = 0; i < count; i++)
    {
        HfInstanceData_t *linkPtr = (HfInstanceData_t*) &(csrBtHfInstance.linkData[i]);

        if (linkPtr->hfConnId != CSR_BT_CONN_ID_INVALID)
        {
            /* If link is in intermediate State of Activated or Connect, veto handover */
            if (linkPtr->state == Connect_s ||
                linkPtr->state == Activate_s ||
                linkPtr->state == ServiceSearch_s ||
                linkPtr->disconnectReqReceived ||
                linkPtr->disconnectPeerReceived)
            {
                veto = TRUE;
                break;
            }
            else if (linkPtr->data && linkPtr->data->cmDataReqQueue)
            {
                /* If link has CM data requests pending in the queue, veto handover. */
                veto = TRUE;
                break;
            }
        }
    }

    if (!veto)
    {
        if (csrBtHfInstance.saveQueue ||
            SynergySchedMessagesPendingForTask(CSR_BT_HF_IFACEQUEUE, NULL) != 0)
        {
            /* If there are pending messages in either the synergy queue or savequeue of HF, veto handover. */
            veto = TRUE;
        }
    }

    CSR_BT_HF_HANDOVER_LOG_INFO("csrBtHfVeto %d", veto);

    return veto;
}

static bool csrBtHfMarshal(const tp_bdaddr *vmTpAddrt,
                           CsrUint8 *buf,
                           CsrUint16 length,
                           CsrUint16 *written)
{
    CsrBtTpdAddrT tpAddrt = { 0 };

    CSR_BT_HF_HANDOVER_LOG_INFO("csrBtHfMarshal");

    BdaddrConvertTpVmToBluestack(&tpAddrt, vmTpAddrt);

    if (!hfConvInst)
    {
        hfConvInst = CsrBtMarshalUtilCreate(CSR_BT_MARSHAL_UTIL_SERIALIZER);
    }

    CsrBtMarshalUtilResetBuffer(hfConvInst, length, buf, TRUE);

    serHfMainInstanceData(hfConvInst, &csrBtHfInstance, &tpAddrt.addrt.addr);

    *written = length - CsrBtMarshalUtilRemainingLengthGet(hfConvInst);

    return CsrBtMarshalUtilStatus(hfConvInst);
}

static bool csrBtHfUnmarshal(const tp_bdaddr *vmTpAddrt,
                             const CsrUint8 *buf,
                             CsrUint16 length,
                             CsrUint16 *written)
{
    CsrBtTpdAddrT tpAddrt = { 0 };

    CSR_BT_HF_HANDOVER_LOG_INFO("csrBtHfUnmarshal");

    BdaddrConvertTpVmToBluestack(&tpAddrt, vmTpAddrt);

    if (!hfConvInst)
    {
        hfConvInst = CsrBtMarshalUtilCreate(CSR_BT_MARSHAL_UTIL_DESERIALIZER);
    }

    CsrBtMarshalUtilResetBuffer(hfConvInst, length, (CsrUint8 *) buf, TRUE);

    deserHfMainInstanceData(hfConvInst, &csrBtHfInstance, &tpAddrt.addrt.addr);

    *written = length - CsrBtMarshalUtilRemainingLengthGet(hfConvInst);

    return CsrBtMarshalUtilStatus(hfConvInst);
}

static void csrBtHfHandoverCommit(const tp_bdaddr *vmTpAddrt,
                                  bool newPrimary)
{
    CsrBtTpdAddrT tpAddrt = { 0 };
    HfInstanceData_t *linkPtr;

    CSR_BT_HF_HANDOVER_LOG_INFO("csrBtHfHandoverCommit");

    if (newPrimary)
    {
        BdaddrConvertTpVmToBluestack(&tpAddrt, vmTpAddrt);
        linkPtr = CsrBtHfGetConnectedInstFromBdAddr(&csrBtHfInstance, &tpAddrt.addrt.addr);

        if (linkPtr)
        {
            Sink sink;
            CsrUint16 cid = L2CA_CID_INVALID;

            /* Convert hfConnId to dlci, see deserHfMainInstanceData */
            CsrUint8 dlci = (CsrUint8)(linkPtr->hfConnId & 0xFF);

            csrBtHfInstance.currentDeviceAddress = tpAddrt.addrt.addr;

            /* Extract new CID for the connection instance and configure its stream */
            sink = StreamRfcommSinkFromDlci(vmTpAddrt, dlci);
            if (sink)
            {
                cid = SinkGetRfcommConnId(sink);
                linkPtr->hfConnId = CM_CREATE_RFC_CONN_ID(cid);

                /* Stitch the RFCOMM sink and the task */
                CsrStreamsRegister(cid,
                                   RFCOMM_ID,
                                   CSR_BT_HF_IFACEQUEUE);

                CsrStreamsSourceHandoverPolicyConfigure(cid,
                                                        RFCOMM_ID,
                                                        SOURCE_HANDOVER_ALLOW_WITHOUT_DATA);

                /* Configure RFCOMM sink messages */
                SinkConfigure(sink, VM_SINK_MESSAGES, VM_MESSAGES_ALL);
            }
        }
    }
}

static void csrBtHfHandoverComplete(bool newPrimary)
{
    CSR_BT_HF_HANDOVER_LOG_INFO("csrBtHfHandoverComplete");

    if (hfConvInst)
    {
        CsrBtMarshalUtilDestroy(hfConvInst);
        hfConvInst = NULL;
    }

    CSR_UNUSED(newPrimary);
}

static void csrBtHfHandoverAbort(void)
{
    CSR_BT_HF_HANDOVER_LOG_INFO("csrBtHfHandoverAbort");

    if (hfConvInst && CsrBtMarshalUtilTypeGet(hfConvInst) == CSR_BT_MARSHAL_UTIL_DESERIALIZER)
    {
        CsrUint8 i;
        CsrUint8 count = csrBtHfInstance.maxHSConnections + csrBtHfInstance.maxHFConnections;

        for (i = 0; i < count; i++)
        {
            HfInstanceData_t *linkPtr = (HfInstanceData_t*) &(csrBtHfInstance.linkData[i]);
            CsrBtHfInitInstanceData(linkPtr);
            linkPtr->accepting = TRUE;
        }
    }
    csrBtHfHandoverComplete(FALSE);
}

const handover_interface csr_bt_hf_handover_if =
        MAKE_BREDR_HANDOVER_IF(&csrBtHfVeto,
                               &csrBtHfMarshal,
                               &csrBtHfUnmarshal,
                               &csrBtHfHandoverCommit,
                               &csrBtHfHandoverComplete,
                               &csrBtHfHandoverAbort);


