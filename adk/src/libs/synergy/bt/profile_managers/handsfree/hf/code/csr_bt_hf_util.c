/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_sched.h"
#include "csr_pmem.h"
#include "bluetooth.h"
#include "hci_prim.h"
#include "csr_bt_cm_lib.h"
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_util.h"
#include "csr_unicode.h"
#include "csr_formatted_io.h"
#include "csr_log_text_2.h"
#include "csr_bt_hf_main.h"
#include "csr_bt_hf_main_sef.h"
#include "csr_bt_hf_prim.h"
#include "csr_bt_hf_util.h"
#ifdef CSR_STREAMS_ENABLE
#include "csr_bt_hf_streams.h"
#endif

#include "csr_bt_hfhs_data_sef.h"
#include "csr_bt_hf_lib.h"
/* Inclusions for use of common SDP search library*/
#include "csr_bt_sdc_support.h"
#include "csr_bt_cmn_sdc_rfc_util.h"
#include "csr_bt_cmn_sdr_tagbased_lib.h"
#include "csr_bt_cmn_sdp_lib.h"
#ifdef EXCLUDE_CSR_BT_SC_MODULE
#include "csr_bt_cm_private_lib.h"
#else
#include "csr_bt_sc_private_lib.h"
#endif

/* change this value according to the actual length of the service
 *name string length. The other index's will be adjusted according to
 *this length */
#define HF_SERVICE_NAME_LEN                        15                /* actual length of service name */

/* do not change these values */
#define HF_SERVER_CHANNEL_INDEX                    ((CsrUint8) 27)
#define HF_SERVICE_NAME_LEN_INDEX                  ((CsrUint8) 53)    /* index for service name length */
#define HF_SERVICE_NAME_INDEX                      ((CsrUint8) 54)    /* fill in the service name from this index */

/* Index to network parameter and supported features parameter The
 * network parameter is a single byte. The supported features is 2
 * byte and the value must be stored with MSB first */
#define HF_SUPPORTED_FEATURES_INDEX                ((CsrUint8) (HF_SERVICE_NAME_LEN_INDEX + HF_SERVICE_NAME_LEN + 5))

#define HS_SERVER_CHANNEL_INDEX                    30
#define HS_REMOTE_AUDIO_INDEX                      68

#define MAX_SERVICE_RECORDS_SEARCH            4  /* Max number of record handles with HFG or AG that amongst which we decide to connect to */

void appendString(CsrCharString **mainStr, CsrUint16 *mainStrLen, CsrCharString **addStr);

static const CsrUint8 sdsHfServiceRecord[] =
{
    /* Service class ID list */
    0x09,0x00,0x01,   /* AttrID , ServiceClassIDList */
    0x35,0x06,        /* 6 bytes in total DataElSeq */
    0x19,0x11,0x1E,   /* 2 byte UUID, Service class = Hands Free */
    0x19,0x12,0x03,   /* 2 byte UUID Service class = GenericAudio */

    /* protocol descriptor list */
    0x09,0x00,0x04,   /* AttrId ProtocolDescriptorList */
    0x35,0x0c,        /* 11 bytes in total DataElSeq */
    0x35,0x03,        /* 3 bytes in DataElSeq */
    0x19, 0x01,0x00,  /* 2 byte UUID, Protocol = L2CAP */

    0x35,0x05,        /* 5 bytes in DataElSeq */
    0x19, 0x00,0x03,  /* 1 byte UUID Protocol = RFCOMM */
    0x08, 0x00,       /* 1 byte UINT - server channel template value 0 - to be filled in by app */

    /* BrowseGroupList    */
    0x09, 0x00, 0x05,    /* AttrId = BrowseGroupList */
    0x35, 0x03,          /* Data element seq. 3 bytes */
    0x19, 0x10, 0x02,    /* 2 byte UUID, PublicBrowseGroup = 0x1002 */

    /* profile descriptor list */
    0x09,0x00,0x09,   /* AttrId, ProfileDescriptorList */
    0x35,0x08,        /* 10 bytes in total DataElSeq */
    0x35,0x06,        /* 6 bytes in total DataElSeq */
    0x19, 0x11,0x1E,  /* 2 byte UUID, Service class = Hands Free */
    0x09, 0x01,0x09,  /* 2 byte UINT, version = 1.9 */

    /* service name */
    0x09, 0x01, 0x00, /* AttrId - Service Name */
    0x25, 0x0F,       /* 15 byte string */
    'H','a','n','d','s','-','F', 'r', 'e', 'e', ' ', 'u', 'n', 'i', 't',

    /* Supported features - passed in from the application */
    0x09, 0x03, 0x11,  /* AttrId - Supported Features */
    0x09, 0x00, 0x04,  /* 2 byte UINT - supported features */
                       /* EC and/or NR fn          - 0 (LSB) */
                       /* 3 way calls              - 0 */
                       /* CLI presentation         - 0 */
                       /* Voice recognition fn     - 0 */
                       /* Remote volume control    - 0 */
                       /* Wide-band speech         - 0 */
                       /* Voice Recognition Text   - 0 */
                       /* Super Wide-band speech   - 0 */
};

static const CsrUint8 sdsHsServiceRecord[] =
{
    /* Service class ID list */
    0x09,0x00,0x01,    /* AttrID , ServiceClassIDList */
    0x35,0x09,         /* 6 bytes in total DataElSeq */
    0x19,0x11,0x08,    /* 2 byte UUID, Service class = Headset = 0x1108 */
    0x19,0x12,0x03,    /* 2 byte UUID, Service class = Generic Audio = 0x1203 */
    0x19,0x11,0x31,    /* 2 byte UUID, Service class = Headset - HS (version 1.2) = 0x1131 */

    /* protocol descriptor list */
    0x09,0x00,0x04,    /* AttrId ProtocolDescriptorList */
    0x35,0x0C,         /* 12 bytes in total DataElSeq */

    /* L2CAP */
    0x35,0x03,         /* 3 bytes in DataElSeq */
    0x19,0x01,0x00,    /* 2 byte UUID, Protocol = L2CAP */

    /* RFCOMM */
    0x35,0x05,         /* 5 bytes in DataElSeq */
    0x19,0x00,0x03,    /* 2 byte UUID Protocol = RFCOMM */
    0x08,0x00,         /* 1 byte UINT - server channel template value 0 - to be filled in by app (index:31) */

    /* BrowseGroupList    */
    0x09, 0x00, 0x05,    /* AttrId = BrowseGroupList */
    0x35, 0x03,          /* Data element seq. 3 bytes */
    0x19, 0x10, 0x02,    /* 2 byte UUID, PublicBrowseGroup = 0x1002 */

    /* Bluetooth Profile descriptor List */
    0x09,0x00,0x09,    /* AttrId Bluetooth Profile DescriptorList = 0x0009 */
    0x35,0x08,         /* 8 bytes in total DataElSeq */
    0x35,0x06,         /* 6 bytes in total DataElSeq */
    0x19,0x11,0x08,    /* 2 byte UUID, Service class = Headset = 0x1108 */

    /* Profile version */
    0x09,0x01,0x02,     /* 2 byte UINT - Profile version = 0x0102 = version 1.2 */

    /* Service name */
    0x09,0x01,0x00,     /* AttrId - Service Name. Use language base attribute 0x0100 (primary language) */
    0x25, 0x07,         /* length of service string: 7 */
    'H','e','a','d','s','e','t',    /* string = "Headset" */

    /* Remote audio volume control */
    0x09,0x03,0x02,     /* AttrId - Remote audio volume control = 0x0302 */
    0x28,0x00,          /* Boolean. Remote control template value = FALSE. (index: 61) */
};

void CsrBtHfMessagePut(CsrSchedQid phandle, void *msg)
{
    CsrSchedMessagePut(phandle, CSR_BT_HF_PRIM, msg);
}

/***********************************************************************************
    Deinitialise SDP search resources
************************************************************************************/
static void csrBtHfSdpDeInit(HfInstanceData_t *linkPtr)
{
    /* searchOngoing means another connection request came before releasing resources,
     * hence it can use the same structures again. */
    if (linkPtr && linkPtr->sdpSearchData)
    {
        CsrBtUtilSdcRfcDeinit(&linkPtr->sdpSearchData);
        linkPtr->sdpSearchData = NULL;
    }
}

/***********************************************************************************
    Make sure not to allow more connections on the connecting channel
************************************************************************************/
void CsrBtHfCancelAcceptOnConnectingChannel(HfInstanceData_t *linkPtr)
{
    HfMainInstanceData_t *instData = CSR_BT_HF_MAIN_INSTANCE_GET(linkPtr);
    linkPtr->accepting = FALSE; /* Link is already in Connected_s / ServiceSearch_s state */
    if (CSR_BT_HF_CONNECTION_HF == linkPtr->linkType)
    {
        CsrBtCmContextCancelAcceptConnectReqSend(CSR_BT_HF_IFACEQUEUE, instData->hfServerChannel, linkPtr->instId);
    }
    else
    {
        CsrBtCmContextCancelAcceptConnectReqSend(CSR_BT_HF_IFACEQUEUE, instData->hsServerChannel, linkPtr->instId);
    }
}

/***********************************************************************************
    Make sure not to allow more connections than allowed by the application
************************************************************************************/
void CsrBtHfCancelAcceptCheck(HfMainInstanceData_t *instData)
{
    CsrUint8 nrActiveHf = 0;
    CsrUint8 nrActiveHs = 0;

    if (instData->maxTotalSimultaneousConnections == CsrBtHfGetNumberOfRecordsInUse(instData,&nrActiveHf,&nrActiveHs))
    {/* Maximum reached! */
        CsrIntFast8 i;
        HfInstanceData_t *linkPtr;
        for (i = 0; i<(instData->maxHFConnections +instData->maxHSConnections);i++)
        {
            linkPtr = (HfInstanceData_t *)&(instData->linkData[i]);
            if ((linkPtr->state == Activate_s) && (linkPtr->accepting))
            {/* Not connected: cancel connect accept */
                linkPtr->accepting = FALSE;
                if (CSR_BT_HF_CONNECTION_HF == linkPtr->linkType)
                {
                    CsrBtCmContextCancelAcceptConnectReqSend(CSR_BT_HF_IFACEQUEUE, instData->hfServerChannel, linkPtr->instId);
                }
                else
                {
                    CsrBtCmContextCancelAcceptConnectReqSend(CSR_BT_HF_IFACEQUEUE, instData->hsServerChannel, linkPtr->instId);
                }
            }
        }
    }
}

/***********************************************************************************
    Check if new connection is allowed to establish
************************************************************************************/
CsrBool HfIsNewConnectionAllowed(HfMainInstanceData_t *instData, CsrUint8 lServerChannel)
{
    CsrUint8 nrActiveHf        = 0;
    CsrUint8 nrActiveHs        = 0;
    CsrBool  allowed           = FALSE;
    CsrUint8 nrTotalActiveConn = CsrBtHfGetNumberOfRecordsInUse(instData, &nrActiveHf, &nrActiveHs);

    if (instData->maxTotalSimultaneousConnections > nrTotalActiveConn &&
        instData->state != Deactivate_s)
    {
        /* Slot available for new connection.
         * Check whether new HF/HS connection to establish is within the limit or not */
        HfInstanceData_t *linkPtr;
        CsrIntFast8 i;

        for (i = 0; i < (instData->maxHFConnections + instData->maxHSConnections); i++)
        {
            linkPtr = (HfInstanceData_t *) &(instData->linkData[i]);

            if (linkPtr->hfConnId  == CSR_BT_CONN_ID_INVALID &&
                linkPtr->accepting == TRUE)
            {
                if (i < instData->maxHFConnections)
                {/* HF slots */
                    if (lServerChannel == instData->hfServerChannel)
                    {
                        /* good to accept new HF connection */
                        allowed = TRUE;
                        break;
                    }
                }
                else
                {/* HS slots */
                    if (lServerChannel == instData->hsServerChannel)
                    {
                        /* good to accept new HS connection */
                        allowed = TRUE;
                        break;
                    }
                }
            }
        }
    }
    else
    {
        /* No slot available */
    }

    return (allowed);
}

/***********************************************************************************
    Make sure to allow more connections if allowed by the application
************************************************************************************/
void CsrBtHfAllowConnectCheck(HfMainInstanceData_t *instData)
{
    CsrUint8 nrActiveHf = 0;
    CsrUint8 nrActiveHs = 0;
    dm_security_level_t secIncoming;

    if ((instData->maxTotalSimultaneousConnections > CsrBtHfGetNumberOfRecordsInUse(instData,&nrActiveHf,&nrActiveHs)) &&
        (instData->state != Deactivate_s))
    {/* Allowed more connections */
        CsrIntFast8 i;
        HfInstanceData_t *linkPtr;

#ifndef INSTALL_HF_CUSTOM_SECURITY_SETTINGS
        CsrBtScSetSecInLevel(&secIncoming,
                             CSR_BT_SEC_DEFAULT,
                             CSR_BT_HANDSFREE_MANDATORY_SECURITY_INCOMING,
                             CSR_BT_HANDSFREE_DEFAULT_SECURITY_INCOMING,
                             CSR_BT_RESULT_CODE_HF_SUCCESS,
                             CSR_BT_RESULT_CODE_HF_UNACCEPTABLE_PARAMETER);
#else /* !INSTALL_HF_CUSTOM_SECURITY_SETTINGS */
        secIncoming = instData->secIncoming;
#endif /* INSTALL_HF_CUSTOM_SECURITY_SETTINGS */

        for (i = 0; i < (instData->maxHFConnections +instData->maxHSConnections); i++)
        {
            linkPtr = (HfInstanceData_t *) &(instData->linkData[i]);

            if ((linkPtr->state == Activate_s) && (linkPtr->accepting == FALSE))
            {/* Not connected: allow connect accept */
                CsrBtDeviceAddr zeroBdAddr = { 0, 0, 0 };
                linkPtr->accepting = TRUE;
                if (i < instData->maxHFConnections )
                {
                    CsrBtCmContextConnectAcceptReqSend(CSR_BT_HF_IFACEQUEUE,
                                                       HF_CLASS_OF_DEVICE,
                                                       0,
                                                       CSR_BT_HF_PROFILE_DEFAULT_MTU_SIZE,
                                                       instData->hfServerChannel,
                                                       secIncoming,
                                                       CSR_BT_HF_PROFILE_UUID,
                                                       linkPtr->instId,
                                                       CSR_BT_MODEM_SEND_CTRL_DTE_DEFAULT,
                                                       CSR_BT_DEFAULT_BREAK_SIGNAL,
                                                       CSR_BT_DEFAULT_MSC_TIMEOUT,
                                                       zeroBdAddr,
                                                       CSRMAX(CSR_BT_SC_DEFAULT_ENC_KEY_SIZE,
                                                              CSR_BT_HANDSFREE_DEFAULT_ENC_KEY_SIZE_VAL));
                }
                else
                {
                    CsrBtCmContextConnectAcceptReqSend(CSR_BT_HF_IFACEQUEUE,
                                                       HS_CLASS_OF_DEVICE,
                                                       0,
                                                       CSR_BT_HF_PROFILE_DEFAULT_MTU_SIZE,
                                                       instData->hsServerChannel,
                                                       secIncoming,
                                                       CSR_BT_HS_PROFILE_UUID,
                                                       linkPtr->instId,
                                                       CSR_BT_MODEM_SEND_CTRL_DTE_DEFAULT,
                                                       CSR_BT_DEFAULT_BREAK_SIGNAL,
                                                       CSR_BT_DEFAULT_MSC_TIMEOUT,
                                                       zeroBdAddr,
                                                       CSRMAX(CSR_BT_SC_DEFAULT_ENC_KEY_SIZE,
                                                              CSR_BT_HANDSFREE_DEFAULT_ENC_KEY_SIZE_VAL));
                }
            }
        }
    }
}

/***********************************************************************************
    Find out how many active or pending connections exist at any given moment
************************************************************************************/
CsrUint8 CsrBtHfGetNumberOfRecordsInUse(HfMainInstanceData_t *instData,CsrUint8 *nrActiveHf,CsrUint8 *nrActiveHs)
{
    CsrUint8 retValue = 0;
    CsrIntFast8 i;
    *nrActiveHf = 0;
    *nrActiveHs = 0;
    for (i=0;i<(instData->maxHFConnections + instData->maxHSConnections);i++)
    {
        if (instData->linkData[i].state != Activate_s)
        {/* Then it is either connected or trying to connect... */
            retValue++;
            if (i < instData->maxHFConnections)
            {
                (*nrActiveHf)++;
            }
            else
            {
                (*nrActiveHs)++;
            }
        }
    }

    return retValue;
}

/***********************************************************************************
    Used when sending messages internally or to higher layer.
***********************************************************************************/
void CsrBtHfSendHfHouseCleaning(HfMainInstanceData_t *instData)
{
    CsrBtHfHouseCleaning    *prim;

    prim = (CsrBtHfHouseCleaning *) CsrPmemAlloc(sizeof(CsrBtHfHouseCleaning));
    prim->type = CSR_BT_HF_HOUSE_CLEANING;
    instData->restoreFlag = TRUE;
    CsrBtHfMessagePut(CSR_BT_HF_IFACEQUEUE, prim);
}

/***********************************************************************************
    Save the message specified by recvMsgP in instData on the internal savequeue.
    Remember to set the recvMsgP to NULL in order not to free it on exit.
***********************************************************************************/
void CsrBtHfSaveMessage(HfMainInstanceData_t *instData)
{
    CsrMessageQueuePush(&instData->saveQueue, CSR_BT_HF_PRIM, instData->recvMsgP);
    instData->recvMsgP = NULL;
}

/***********************************************************************************
    Save the CM prim message specified by recvMsgP in instData on the internal savequeue.
    Remember to set the recvMsgP to NULL in order not to free it on exit.
***********************************************************************************/
void CsrBtHfSaveCmMessage(HfMainInstanceData_t *instData)
{
    CsrMessageQueuePush(&instData->saveQueue, CSR_BT_CM_PRIM, instData->recvMsgP);
    instData->recvMsgP = NULL;
}

/*******************************************************************************
    init instance data for one transaction
*******************************************************************************/
void CsrBtHfInitInstanceData(HfInstanceData_t *hfInstData)
{
    CsrUint16 mi;
    void *mv;

    if (hfInstData->data->atResponseTimerId != 0)
    {
        CsrSchedTimerCancel(hfInstData->data->atResponseTimerId, &mi, &mv);
        hfInstData->data->atResponseTimerId     = 0;
    }

    CsrMemSet(&hfInstData->agIndicators, 0, sizeof(hfInstData->agIndicators));

    hfInstData->nrOfIndicators      = 0;
#ifdef CSR_BT_INSTALL_HF_CONFIG_AUDIO
    if (hfInstData->audioSetupParams)
    {
        CsrPmemFree(hfInstData->audioSetupParams);
        hfInstData->audioSetupParams = NULL;
    }
#endif

    if(hfInstData->remoteHfIndicatorList.first != NULL)
    {
        CsrCmnListDeinit((CsrCmnList_t *)(&hfInstData->remoteHfIndicatorList));
    }

    if (hfInstData->sdpSearchData)
    {
        CsrBtUtilSdcRfcDeinit(&(hfInstData->sdpSearchData));
        hfInstData->sdpSearchData = NULL;
    }

    hfInstData->data->recvAtCmdSize             = 0;
    hfInstData->data->recvAtCmd                 = NULL;
    hfInstData->data->allowed2SendCmData        = TRUE;
    hfInstData->data->dataReceivedInConnected   = FALSE;
    hfInstData->data->cmDataReqQueue            = NULL;

    hfInstData->lastAtCmdSent                   = idleCmd;

    hfInstData->disconnectReqReceived           = FALSE;
    hfInstData->disconnectPeerReceived          = FALSE;
    hfInstData->scoHandle                       = HF_SCO_UNUSED;
    hfInstData->linkState                       = CSR_BT_LINK_STATUS_DISCONNECTED;
    hfInstData->hfConnId                        = CSR_BT_CONN_ID_INVALID;

    hfInstData->audioPending                    = FALSE;
    hfInstData->lastAudioReq                    = CSR_BT_HF_AUDIO_OFF;
    hfInstData->state                           = Activate_s; /* */
    hfInstData->oldState                        = Activate_s;
    hfInstData->remoteVersion                   = 0;
    hfInstData->pcmSlot                         = 0;
    hfInstData->pcmReassign                     = FALSE;
    hfInstData->pcmMappingReceived              = FALSE;
    hfInstData->audioAcceptPending              = FALSE;
    hfInstData->atSequenceState                 = supportFeatures;

    CsrPmemFree(hfInstData->serviceName);
    hfInstData->serviceName                     = NULL;

    hfInstData->scoConnectAcceptPending         = FALSE;
    hfInstData->searchAndCon                    = FALSE;
    hfInstData->searchOngoing                   = FALSE;
    hfInstData->pendingCancel                   = FALSE;
    hfInstData->incomingSlc         = FALSE;
    hfInstData->obtainedServerCh    = CSR_BT_NO_SERVER;
    hfInstData->pendingSlcXtraCmd   = FALSE;
    hfInstData->codecToUse          = CSR_BT_WBS_INVALID_CODEC;
    hfInstData->instReused          = FALSE;
#ifdef CSR_BT_HF_ENABLE_SWB_SUPPORT
    hfInstData->hfQceCodecId        = CSR_BT_HF_QCE_UNSUPPORTED;

#if 0 /* hfAgQceCodecMask is not being used currently */
    hfInstData->hfAgQceCodecMask    = CSR_BT_HF_QCE_UNSUPPORTED;
#endif
#endif /* CSR_BT_HF_ENABLE_SWB_SUPPORT */
    CsrBtHfSetAddrInvalid(&hfInstData->currentDeviceAddress);
}

/*******************************************************************************
    Empty the internal save queue's. Called before entering idle state.
*******************************************************************************/
void CsrBtHfSaveQueueCleanUp(HfMainInstanceData_t *instData)
{
    CsrUint16            eventClass;
    void                *msg;
    HfInstanceData_t *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    /* pop all (if any) messages from the message save queue */
    while (CsrMessageQueuePop(&(instData->saveQueue), &eventClass, &msg))
    {
        CsrBtHfPrim         *primType;

        /* find the message type */
        primType = (CsrBtHfPrim *)msg;
        if (*primType == CSR_BT_HF_AT_CMD_REQ)
        {
            CsrBtHfAtCmdReq *prim;

            prim = (CsrBtHfAtCmdReq *) msg;
            CsrPmemFree(prim->atCmdString);
        }
        SynergyMessageFree(eventClass, msg);
    }

    /* pop all (if any) messages from the CM save queue */
    if (linkPtr->data != NULL)
    {
        while (CsrMessageQueuePop( &(linkPtr->data->cmDataReqQueue ), &eventClass, &msg))
        {
            if (eventClass == CSR_BT_CM_PRIM)
            {
                CsrBtCmDataReq *prim;

                prim = (CsrBtCmDataReq *) msg;
                CsrPmemFree(prim->payload);
                SynergyMessageFree(eventClass, msg);
            }
            else if (eventClass == CSR_BT_HF_PRIM)
            {
                CsrBtHfPrim  *primType = (CsrBtHfPrim *)msg;

                if (*primType == CSR_BT_HF_AT_CMD_REQ)
                {
                    CsrBtHfAtCmdReq *prim;

                    prim = (CsrBtHfAtCmdReq *) msg;
                    CsrPmemFree(prim->atCmdString);
                }
                else if (*primType == CSR_BT_HF_DIAL_REQ)
                {
                    CsrBtHfDialReq *prim = (CsrBtHfDialReq *)msg;
                    CsrPmemFree(prim->number);
                }
                SynergyMessageFree(eventClass, msg);
            }
        }
        CsrPmemFree(linkPtr->data->recvAtCmd);
        linkPtr->data->recvAtCmdSize = 0;
        linkPtr->data->recvAtCmd = NULL;
    }
    linkPtr->lastAtCmdSent = idleCmd;
}




/*************************************************************************************
    Send a confirm message to app
************************************************************************************/
void CsrBtHfSendConfirmMessage(HfMainInstanceData_t *instData, CsrBtHfPrim type)
{
    CsrBtHfConfigAudioCfm * prim;

    prim = (CsrBtHfConfigAudioCfm *) CsrPmemAlloc( sizeof (CsrBtHfConfigAudioCfm));
    prim->type = type;
    CsrBtHfMessagePut(instData->appHandle, prim);
}

/*************************************************************************************
    Send a CSR_BT_HF_MIC_GAIN_IND to app
************************************************************************************/
void CsrBtHfSendHfMicGainInd(HfMainInstanceData_t *instData,
                        CsrUint8 returnValue)
{
    CsrBtHfMicGainInd    *prim;
    HfInstanceData_t *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    prim    = (CsrBtHfMicGainInd *)CsrPmemAlloc(sizeof(CsrBtHfMicGainInd));
    prim->type        = CSR_BT_HF_MIC_GAIN_IND;
    prim->gain        = returnValue;
    prim->connectionId = linkPtr->hfConnId;

    CsrBtHfMessagePut(instData->appHandle, prim);
}


/*************************************************************************************
    Send a CSR_BT_HF_SPEAKER_GAIN_IND to app
************************************************************************************/
void CsrBtHfSendHfSpeakerGainInd(HfMainInstanceData_t *instData,
                            CsrUint8 returnValue)
{
    CsrBtHfSpeakerGainInd    *prim;
    HfInstanceData_t *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    prim    = (CsrBtHfSpeakerGainInd *)CsrPmemAlloc(sizeof(CsrBtHfSpeakerGainInd));
    prim->type        = CSR_BT_HF_SPEAKER_GAIN_IND;
    prim->gain        = returnValue;
    prim->connectionId = linkPtr->hfConnId;

    CsrBtHfMessagePut(instData->appHandle, prim);
}

/********************************************************************************************
    Send a cfm message to the app that only takes type, connectionId and result as parameters
*********************************************************************************************/
void CsrBtHfSendHfGeneralCfmMsg(HfMainInstanceData_t *instData, CsrUint16 result, CsrBtHfPrim type)
{
    HF_GENERAL_CFM_T  *prim;
    HfInstanceData_t *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    prim    = (HF_GENERAL_CFM_T *)CsrPmemAlloc(sizeof(HF_GENERAL_CFM_T));
    prim->type        = type;
    prim->connectionId = linkPtr->hfConnId;
    prim->cmeeResultCode = result;

    CsrBtHfMessagePut(instData->appHandle, prim);
}

/*************************************************************************************
    Send a CsrBtHfInbandRingSettingChangedInd to app
************************************************************************************/
void CsrBtHfSendHfInBandRingToneInd(HfMainInstanceData_t *instData,
                               CsrBool                returnValue)
{
    CsrBtHfInbandRingSettingChangedInd    *prim;
    HfInstanceData_t *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    prim                 = (CsrBtHfInbandRingSettingChangedInd *)CsrPmemAlloc(sizeof(CsrBtHfInbandRingSettingChangedInd));
    prim->type           = CSR_BT_HF_INBAND_RING_SETTING_CHANGED_IND;
    prim->inbandRingingActivated  = returnValue;
    prim->connectionId = linkPtr->hfConnId;

    CsrBtHfMessagePut(instData->appHandle, prim);
}

/*************************************************************************************
    Send a CSR_BT_HF_AUDIO_DISCONNECT_CFM to app
************************************************************************************/
void CsrBtHfSendHfAudioDisconnectCfm(HfMainInstanceData_t *instData,
                           CsrUint16 scoHandle,
                           CsrBtResultCode      resultCode,
                           CsrBtSupplier  resultSupplier)
{
    HfInstanceData_t *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    CsrBtHfAudioDisconnectCfm    *prim;
    prim    = (CsrBtHfAudioDisconnectCfm *)CsrPmemAlloc(sizeof(CsrBtHfAudioDisconnectCfm));
    prim->type        = CSR_BT_HF_AUDIO_DISCONNECT_CFM;
    prim->resultCode = resultCode;
    prim->scoHandle = scoHandle;
    prim->resultSupplier   = resultSupplier;
    prim->connectionId = linkPtr->hfConnId;
    CsrBtHfMessagePut(instData->appHandle, prim);
}

/*************************************************************************************
    Send a CSR_BT_HF_AUDIO_DISCONNECT_IND to app
************************************************************************************/
void CsrBtHfSendHfAudioDisconnectInd(HfMainInstanceData_t *instData,
                           CsrUint16 scoHandle,
                           CsrBtReasonCode      reasonCode,
                           CsrBtSupplier  reasonSupplier)
{
    HfInstanceData_t *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    CsrBtHfAudioDisconnectInd    *prim;
    prim    = (CsrBtHfAudioDisconnectInd *)CsrPmemAlloc(sizeof(CsrBtHfAudioDisconnectInd));
    prim->type        = CSR_BT_HF_AUDIO_DISCONNECT_IND;
    prim->scoHandle = scoHandle;
    prim->reasonCode = reasonCode;
    prim->reasonSupplier   = reasonSupplier;
    prim->connectionId = linkPtr->hfConnId;
    CsrBtHfMessagePut(instData->appHandle, prim);
}

/*************************************************************************************
    Send a CSR_BT_HF_AUDIO_CONNECT_CFM to app
************************************************************************************/
void CsrBtHfSendHfAudioConnectInd(HfMainInstanceData_t *instData,
                           CsrUint8              pcmSlot,
                           CsrUint8              theScoLinkType,
                           CsrUint8              weSco,
                           CsrUint16             rxPacketLength,
                           CsrUint16             txPacketLength,
                           CsrUint8              airMode,
                           CsrUint8              txInterval,
                           CsrBtResultCode       resultCode,
                           CsrBtReasonCode       reasonCode,
                           CsrBtSupplier         resultSupplier,
                           CsrBtHfPrim           primType)
{
    CsrBtHfAudioConnectInd    *prim;
    HfInstanceData_t *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    if (resultSupplier == CSR_BT_SUPPLIER_HF &&
        resultCode == CSR_BT_RESULT_CODE_HF_SUCCESS)
    {
        if (instData->deferSelCodecInd)
        {
            /* Clear for the next attempt. */
            instData->deferSelCodecInd = FALSE;

            /* This is done for the cases where the SLC connection is not complete,
             * but the codec negotiation was received. For such scenarios,
             * the selected codec indication is delayed and is sent with the
             * audio connect indication.*/
            CsrBtHfSendSelectedCodecInd(instData);
        }
    }

    prim     = (CsrBtHfAudioConnectInd *)CsrPmemAlloc(sizeof(CsrBtHfAudioConnectInd));

    prim->linkType       = theScoLinkType;
    prim->connectionId   = linkPtr->hfConnId;
    prim->scoHandle      = linkPtr->scoHandle;
    prim->pcmSlot        = pcmSlot;
    prim->txInterval     = txInterval;
    prim->weSco          = weSco;
    prim->rxPacketLength = rxPacketLength;
    prim->txPacketLength = txPacketLength;
    prim->airMode        = airMode;
    prim->resultCode     = resultCode;
    prim->resultSupplier = resultSupplier;
    prim->type           = primType;
#ifdef CSR_BT_HF_ENABLE_SWB_SUPPORT
    prim->qceCodecId     = linkPtr->hfQceCodecId;
#else
    prim->qceCodecId     = CSR_BT_HF_QCE_UNSUPPORTED;
#endif /* CSR_BT_HF_ENABLE_SWB_SUPPORT */
    CsrBtHfMessagePut(instData->appHandle, prim);
    CSR_UNUSED(reasonCode);
}

/*************************************************************************************
    Send a CSR_BT_HF_SERVICE_CONNECT_IND to app
************************************************************************************/
void CsrBtHfSendHfServiceConnectInd(HfMainInstanceData_t *instData,
                                    CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    CsrBtHfServiceConnectInd *prim;
    HfInstanceData_t *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    prim = (CsrBtHfServiceConnectInd *)CsrPmemZalloc(sizeof(CsrBtHfServiceConnectInd));
    linkPtr->nrOfIndicators = 0;
    if (linkPtr->incomingSlc)
    {
        prim->type = CSR_BT_HF_SERVICE_CONNECT_IND;
    }
    else
    {
        prim->type = CSR_BT_HF_SERVICE_CONNECT_CFM;
    }

#ifndef CSR_BT_INSTALL_APP_APT_HANDLING
    if (resultCode == CSR_BT_RESULT_CODE_HF_SUCCESS)
    {
        CsrBtTpdAddrT tpAddrt;

        tpAddrt.addrt.addr = linkPtr->currentDeviceAddress;
        tpAddrt.addrt.type = TBDADDR_PUBLIC;
        tpAddrt.tp_type = CSR_BT_TRANSPORT_BREDR;
        CsrBtCmWriteAuthPayloadTimeoutReqSend(CSR_BT_HF_IFACEQUEUE, tpAddrt, CSR_BT_HFP_AUTHENTICATED_PAYLOAD_TIMEOUT);
    }
#endif /* !CSR_BT_INSTALL_APP_APT_HANDLING */

    prim->resultCode = resultCode;
    prim->resultSupplier = resultSupplier;
    prim->indicatorSupported = NULL;
    prim->indicatorValue = NULL;
    prim->chldString = NULL;
    prim->serviceName = NULL;
    prim->connectionType = linkPtr->linkType;
    prim->btConnId = CSR_BT_CONN_ID_INVALID;
    prim->hfgSupportedHfIndicators = NULL;
    prim->hfgSupportedHfIndicatorsCount = 0;

    if (prim->connectionType == CSR_BT_HF_CONNECTION_HF)
    {
        prim->deviceAddr = linkPtr->currentDeviceAddress;
        prim->supportedFeatures = linkPtr->supportedFeatures;
        prim->network = linkPtr->network;
        prim->remoteVersion = linkPtr->remoteVersion;
        prim->serviceName = (CsrCharString *) linkPtr->serviceName;
        linkPtr->serviceName = NULL;
        prim->connectionId = linkPtr->hfConnId;

        if(linkPtr->remoteHfIndicatorList.first != NULL)
        {
            CsrUint16 count;
            CsrBtHfRemoteHfIndicator *currentHfInd;
            CsrBtHfAgHfIndicator *agHfIndicators;

            agHfIndicators = (CsrBtHfAgHfIndicator *) CsrPmemZalloc(
                                linkPtr->remoteHfIndicatorList.count * sizeof(CsrBtHfAgHfIndicator));

            count =0;
            for(currentHfInd = REMOTE_HF_INDICATOR_GET_FIRST((CsrCmnList_t *)(&linkPtr->remoteHfIndicatorList));
                    currentHfInd; currentHfInd = currentHfInd->next)
            {
                agHfIndicators[count].hfIndicatorID = currentHfInd->agHfIndicator;
                agHfIndicators[count].status = currentHfInd->indStatus;

                count++;
            }
            prim->hfgSupportedHfIndicators = agHfIndicators;
            prim->hfgSupportedHfIndicatorsCount = count;
        }
        if (linkPtr->agIndicators.cindData.instCount > 0)
        {
            CsrUint8 *cindString;
            CsrUint16 length;

            /* Note that below function will also allocate the string */
            cindString = CsrBtHfDecodeCindString(&linkPtr->agIndicators.cindData);

            if(cindString != NULL)
            {
                length = (CsrUint16)(CsrStrLen((char *)cindString) + 1);

                prim->indicatorSupported = CsrPmemAlloc(length);
                SynMemCpyS(prim->indicatorSupported, length, cindString, length-1);
                prim->indicatorSupported[length-1] = 0;

                CsrPmemFree(cindString);
            }
        }
        if (linkPtr->agIndicators.cindStartValueString[0])
        {
            CsrUintFast8 j = 0;
            CsrUint16 length = (CsrUint16)CsrStrLen((char *)linkPtr->agIndicators.cindStartValueString)+1;
            prim->indicatorValue = CsrPmemAlloc(length);
            SynMemCpyS(prim->indicatorValue, length, linkPtr->agIndicators.cindStartValueString,length);
            prim->indicatorValue[length-1] = 0;
            linkPtr->nrOfIndicators = 1;
            for (j=0;prim->indicatorValue[j] != '\0'; j++)
            {
                if (prim->indicatorValue[j] == ',')
                {
                    linkPtr->nrOfIndicators++;
                }
            }
        }
        if (linkPtr->agIndicators.chldStringStored[0])
        {
            CsrUint16 length = (CsrUint16)CsrStrLen((char *)linkPtr->agIndicators.chldStringStored)+1;
            prim->chldString = CsrPmemAlloc(length);
            CsrStrNCpyZero((char *)prim->chldString,(char *)linkPtr->agIndicators.chldStringStored,length);
        }
        if ((resultCode == CSR_BT_RESULT_CODE_HF_SUCCESS) &&
            (resultSupplier == CSR_BT_SUPPLIER_HF))
        {
            if (!((instData->mainConfig & CSR_BT_HF_CNF_DISABLE_AUTOMATIC_CLIP_ACTIVATION) &&
                   (instData->mainConfig & CSR_BT_HF_CNF_DISABLE_AUTOMATIC_CCWA_ACTIVATION) &&
                   (instData->mainConfig & CSR_BT_HF_CNF_DISABLE_AUTOMATIC_CMEE_ACTIVATION)))
            {
                linkPtr->pendingSlcXtraCmd = FALSE;

                if ((instData->mainConfig & CSR_BT_HF_CNF_DISABLE_AUTOMATIC_CLIP_ACTIVATION) == 0)
                {/* send AT+CLIP=1? */
                    if (instData->localSupportedFeatures & CSR_BT_HF_SUPPORT_CLI_PRESENTATION_CAPABILITY)
                    {
                        CsrBtHfSetCallNotificationIndicationReqSend(linkPtr->hfConnId, TRUE);
                        linkPtr->pendingSlcXtraCmd = TRUE;
                    }
                }
                if ((instData->mainConfig & CSR_BT_HF_CNF_DISABLE_AUTOMATIC_CCWA_ACTIVATION) == 0)
                {/* send AT+CCWA=1 */
                    if (((linkPtr->supportedFeatures & CSR_BT_HFG_SUPPORT_THREE_WAY_CALLING) ||
                         (linkPtr->supportedFeatures & CSR_BT_HFG_SUPPORT_ENHANCED_CALL_CONTROL)) &&
                        ((instData->localSupportedFeatures & CSR_BT_HF_SUPPORT_CALL_WAITING_THREE_WAY_CALLING) ||
                         (instData->localSupportedFeatures & CSR_BT_HF_SUPPORT_ENHANCED_CALL_CONTROL)))
                    {
                        CsrBtHfSetCallWaitingNotificationReqSend(linkPtr->hfConnId, TRUE);
                        linkPtr->pendingSlcXtraCmd = TRUE;
                    }

                }
                if ((instData->mainConfig & CSR_BT_HF_CNF_DISABLE_AUTOMATIC_CMEE_ACTIVATION) == 0)
                {/* send AT+CMEE=1 */
                    if (linkPtr->supportedFeatures & CSR_BT_HFG_SUPPORT_EXTENDED_ERROR_CODES)
                    {
                        CsrBtHfSetExtendedAgErrorResultCodeReqSend(linkPtr->hfConnId, TRUE);
                        linkPtr->pendingSlcXtraCmd = TRUE;
                    }
                }
            }
        }
    }
    else if (prim->connectionType == CSR_BT_HF_CONNECTION_HS)
    {
        prim->deviceAddr = linkPtr->currentDeviceAddress;
        prim->supportedFeatures = 0;
        prim->network = 0;
        prim->remoteVersion = 0;
        prim->serviceName = (CsrCharString *) linkPtr->serviceName;
        linkPtr->serviceName = NULL;

        prim->connectionId = linkPtr->hfConnId;
    }
    else
    {
        prim->deviceAddr = instData->currentDeviceAddress;
        prim->supportedFeatures = 0;
        prim->network = 0;
        prim->remoteVersion = 0;
        prim->connectionId = CSR_BT_HF_CONNECTION_ALL;
    }

    CsrBtHfMessagePut(instData->appHandle, prim);
}

/*************************************************************************************
    Send a CSR_BT_HF_SERVICE_CONNECT_IND to app which failed validation in profile
    manager itself
************************************************************************************/
void CsrBtHfSendHfFailedServiceConnectCfm(HfMainInstanceData_t *instData, CsrBtDeviceAddr deviceAddr,
                                    CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    CsrBtHfServiceConnectInd *prim;

    prim = (CsrBtHfServiceConnectInd *) CsrPmemZalloc(sizeof(CsrBtHfServiceConnectInd));
    prim->type = CSR_BT_HF_SERVICE_CONNECT_CFM;
    prim->resultCode = resultCode;
    prim->resultSupplier = resultSupplier;
    prim->deviceAddr = deviceAddr;
    CsrBtHfMessagePut(instData->appHandle, prim);
}

/*************************************************************************************
    Send a CSR_BT_HF_DISCONNECT_IND to app
************************************************************************************/
void CsrBtHfSendHfDisconnectInd(HfMainInstanceData_t *instData,
                                CsrBtResultCode reasonCode, CsrBtSupplier reasonSupplier)
{
    CsrBtHfDisconnectInd *prim;
    HfInstanceData_t *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    prim = (CsrBtHfDisconnectInd *)CsrPmemAlloc(sizeof(CsrBtHfDisconnectInd));
    if (linkPtr->disconnectReqReceived)
    {
        prim->type = CSR_BT_HF_DISCONNECT_CFM;
    }
    else
    {
        prim->type = CSR_BT_HF_DISCONNECT_IND;
    }
    prim->connectionId   = linkPtr->hfConnId;

    /* Applications expect the supplier to be CSR_BT_SUPPLIER_HF and
     * CSR_BT_RESULT_CODE_HF_SUCCESS for success cases.
     */
    if (((reasonCode == CSR_BT_RESULT_CODE_CM_SUCCESS) &&
          (reasonSupplier == CSR_BT_SUPPLIER_CM)) ||
         ((reasonCode == RFC_SUCCESS) &&
          (reasonSupplier == CSR_BT_SUPPLIER_RFCOMM)))
    {
        prim->reasonCode = CSR_BT_RESULT_CODE_HF_SUCCESS;
        prim->reasonSupplier = CSR_BT_SUPPLIER_HF;
    }
    else
    {
        prim->reasonCode     = reasonCode;
        prim->reasonSupplier = reasonSupplier;
    }
    CsrBtHfMessagePut(instData->appHandle, prim);
}

/*************************************************************************************
    Send a CSR_BT_HF_CALL_RINGING_IND to app
************************************************************************************/
void CsrBtHfSendHfRingInd(HfMainInstanceData_t *instData)
{
    CsrBtHfCallRingingInd *prim;
    HfInstanceData_t *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    prim = (CsrBtHfCallRingingInd *)CsrPmemAlloc(sizeof(CsrBtHfCallRingingInd));
    prim->type = CSR_BT_HF_CALL_RINGING_IND;
    prim->connectionId = linkPtr->hfConnId;

    CsrBtHfMessagePut(instData->appHandle, prim);
}

/*************************************************************************************
    Send a CSR_BT_HF_DEACTIVATE_CFM to app
************************************************************************************/
void CsrBtHfSendHfDeactivateCfm(HfMainInstanceData_t *instData,
                                CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    CsrBtHfDeactivateCfm    *prim;

    prim = (CsrBtHfDeactivateCfm *) CsrPmemAlloc(sizeof(CsrBtHfDeactivateCfm));
    prim->type            = CSR_BT_HF_DEACTIVATE_CFM;
    prim->resultCode      = resultCode;
    prim->resultSupplier  = resultSupplier;
    CsrBtHfMessagePut(instData->appHandle, prim);
}

/*************************************************************************************
    Send a CSR_BT_HF_ACTIVATE_CFM to app
************************************************************************************/
void CsrBtHfSendHfActivateCfm(HfMainInstanceData_t *instData,
                              CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    CsrBtHfActivateCfm    *prim;

    prim = (CsrBtHfActivateCfm *) CsrPmemAlloc(sizeof(CsrBtHfActivateCfm));
    prim->type           = CSR_BT_HF_ACTIVATE_CFM;
    prim->resultCode     = resultCode;
    prim->resultSupplier = resultSupplier;

    CsrBtHfMessagePut(instData->appHandle, prim);
}


/*************************************************************************************
    Send a CSR_BT_HF_GET_ALL_STATUS_INDICATORS_CFM to app
************************************************************************************/
void CsrBtHfSendIndicatorsUpdateCfm(HfMainInstanceData_t *instData, CsrUint16 result)
{
    CsrBtHfGetAllStatusIndicatorsCfm  *prim;
    HfInstanceData_t *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    prim = (CsrBtHfGetAllStatusIndicatorsCfm *)CsrPmemAlloc(sizeof(CsrBtHfGetAllStatusIndicatorsCfm));
    prim->type = CSR_BT_HF_GET_ALL_STATUS_INDICATORS_CFM;
    prim->connectionId          = linkPtr->hfConnId;
    prim->indicatorSupported    = NULL;
    prim->indicatorValue        = NULL;
    if (result == CSR_BT_CME_SUCCESS)
    {
        if (linkPtr->agIndicators.cindData.instCount > 0)
        {
            CsrUint8 *cindString;
            CsrUint16 length;

            /* Note that below function will also allocate the string */
            cindString = CsrBtHfDecodeCindString(&linkPtr->agIndicators.cindData);

            if(cindString != NULL)
            {
                length = (CsrUint16)(CsrStrLen((char *)cindString) + 1);

                prim->indicatorSupported = CsrPmemAlloc(length);
                SynMemCpyS(prim->indicatorSupported, length, cindString, length-1);
                prim->indicatorSupported[length-1] = 0;

                CsrPmemFree(cindString);
            }
        }
        if (linkPtr->agIndicators.cindStartValueString[0])
        {
            CsrUint16 length = (CsrUint16)CsrStrLen((char *)linkPtr->agIndicators.cindStartValueString)+1;
            prim->indicatorValue = CsrPmemAlloc(length);
            SynMemCpyS(prim->indicatorValue, length, linkPtr->agIndicators.cindStartValueString,length);
            prim->indicatorValue[length-1] = 0;
        }
    }

    prim->cmeeResultCode = result;

    CsrBtHfMessagePut(instData->appHandle, prim);
}


/*************************************************************************************
    Send a CSR_BT_HF_UPDATE_SUPPORTED_CODEC_CFM to app
************************************************************************************/
void CsrBtHfSendUpdateCodecSupportedCfm(HfMainInstanceData_t *instData)
{
    CsrBtHfUpdateSupportedCodecCfm *prim = (CsrBtHfUpdateSupportedCodecCfm *)CsrPmemAlloc(sizeof(CsrBtHfUpdateSupportedCodecCfm));

    prim->type = CSR_BT_HF_UPDATE_SUPPORTED_CODEC_CFM;
    prim->resultCode     = CSR_BT_RESULT_CODE_HF_SUCCESS;
    prim->resultSupplier = CSR_BT_SUPPLIER_HF;

    CsrBtHfMessagePut(instData->appHandle, prim);
}

#ifdef HF_ENABLE_OPTIONAL_CODEC_SUPPORT
void HfSendUpdateOptionalCodecCfm(HfMainInstanceData_t *instData, CsrBtResultCode resultCode)
{
    HfUpdateOptionalCodecCfm *prim = (HfUpdateOptionalCodecCfm *)CsrPmemAlloc(sizeof(HfUpdateOptionalCodecCfm));

    prim->type = HF_UPDATE_OPTIONAL_CODEC_CFM;
    prim->resultCode     = resultCode;
    prim->resultSupplier = CSR_BT_SUPPLIER_HF;

    CsrBtHfMessagePut(instData->appHandle, prim);
}
#endif

/*************************************************************************************
    Send a CSR_BT_HF_SELECTED_CODEC_IND to app
************************************************************************************/
void CsrBtHfSendSelectedCodecInd(HfMainInstanceData_t *instData)
{
    CsrBtHfSelectedCodecInd    *prim;
    HfInstanceData_t *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    prim               = (CsrBtHfSelectedCodecInd *)CsrPmemAlloc(sizeof(CsrBtHfSelectedCodecInd));
    prim->type         = CSR_BT_HF_SELECTED_CODEC_IND;
    prim->codecToUse   = linkPtr->codecToUse;
    prim->connectionId = linkPtr->hfConnId;

    CsrBtHfMessagePut(instData->appHandle, prim);
}

/* Required changes in csrBtHfGetNegotiateCount, csrBtHfGetNegotiateOffset etc. must be done
   for any change in below negotiation order table */
static const audioSetupParams_t scoNegotiationOrder[] =
{
    {
        CSR_BT_ESCO_DEFAULT_2P0_S4_TX_BANDWIDTH,
        CSR_BT_ESCO_DEFAULT_2P0_S4_RX_BANDWIDTH,
        CSR_BT_ESCO_DEFAULT_2P0_S4_AUDIO_QUALITY,
        CSR_BT_ESCO_DEFAULT_2P0_S4_MAX_LATENCY,
        CSR_BT_ESCO_DEFAULT_2P0_S4_VOICE_SETTINGS,
        CSR_BT_ESCO_DEFAULT_2P0_S4_RE_TX_EFFORT,
    },

    {
        CSR_BT_ESCO_DEFAULT_2P0_S3_TX_BANDWIDTH,
        CSR_BT_ESCO_DEFAULT_2P0_S3_RX_BANDWIDTH,
        CSR_BT_ESCO_DEFAULT_2P0_S3_AUDIO_QUALITY,
        CSR_BT_ESCO_DEFAULT_2P0_S3_MAX_LATENCY,
        CSR_BT_ESCO_DEFAULT_2P0_S3_VOICE_SETTINGS,
        CSR_BT_ESCO_DEFAULT_2P0_S3_RE_TX_EFFORT,
    },

    {
        CSR_BT_ESCO_DEFAULT_2P0_S2_TX_BANDWIDTH,
        CSR_BT_ESCO_DEFAULT_2P0_S2_RX_BANDWIDTH,
        CSR_BT_ESCO_DEFAULT_2P0_S2_AUDIO_QUALITY,
        CSR_BT_ESCO_DEFAULT_2P0_S2_MAX_LATENCY,
        CSR_BT_ESCO_DEFAULT_2P0_S2_VOICE_SETTINGS,
        CSR_BT_ESCO_DEFAULT_2P0_S2_RE_TX_EFFORT,
    },

    {
        CSR_BT_ESCO_DEFAULT_1P2_S1_TX_BANDWIDTH,
        CSR_BT_ESCO_DEFAULT_1P2_S1_RX_BANDWIDTH,
        CSR_BT_ESCO_DEFAULT_1P2_S1_AUDIO_QUALITY,
        CSR_BT_ESCO_DEFAULT_1P2_S1_MAX_LATENCY,
        CSR_BT_ESCO_DEFAULT_1P2_S1_VOICE_SETTINGS,
        CSR_BT_ESCO_DEFAULT_1P2_S1_RE_TX_EFFORT,
    },

    {
        CSR_BT_SCO_DEFAULT_1P1_TX_BANDWIDTH,
        CSR_BT_SCO_DEFAULT_1P1_RX_BANDWIDTH,
        CSR_BT_SCO_DEFAULT_1P1_AUDIO_QUALITY,
        CSR_BT_SCO_DEFAULT_1P1_MAX_LATENCY,
        CSR_BT_SCO_DEFAULT_1P1_VOICE_SETTINGS,
        CSR_BT_SCO_DEFAULT_1P1_RE_TX_EFFORT,
    },
};

/*************************************************************************************
    Find the number of negotiations we need to do according to the start parameter 'start_setting'
************************************************************************************/
static CsrUint16 csrBtHfGetNegotiateCount(HfInstanceData_t *linkPtr, CsrUint8 start_setting)
{
    CsrUint16 size;
    if (start_setting == CSR_BT_SCO_DEFAULT_1P1)
    {
        size = 1;
    }
    else
    {
        if((CSR_BT_HF_MAIN_INSTANCE_GET(linkPtr)->localSupportedFeatures & CSR_BT_HF_SUPPORT_ESCO_S4_T2_SETTINGS) &&
           (linkPtr->supportedFeatures & CSR_BT_HFG_SUPPORT_ESCO_S4_T2_SETTINGS))
        {
            size = CSR_ARRAY_SIZE(scoNegotiationOrder);
        }
        else
        {
            size = CSR_ARRAY_SIZE(scoNegotiationOrder) - 1;
        }
    }
    return size;
}

/*************************************************************************************
    Find the offset to the given start parameter 'start_setting'
************************************************************************************/
static CsrUint8 csrBtHfGetNegotiateOffset(HfInstanceData_t *linkPtr, CsrUint8 start_setting)
{
    CsrUint8 index;
    if (start_setting == CSR_BT_SCO_DEFAULT_1P1)
    {
        /* The last entry in the negotiation order */
        index = CSR_ARRAY_SIZE(scoNegotiationOrder)-1;
    }
    else
    {
        if ((CSR_BT_HF_MAIN_INSTANCE_GET(linkPtr)->localSupportedFeatures & CSR_BT_HF_SUPPORT_ESCO_S4_T2_SETTINGS) &&
            (linkPtr->supportedFeatures & CSR_BT_HFG_SUPPORT_ESCO_S4_T2_SETTINGS))
        {
            /* Set the index "0" to form the negotiation order S4->S3->S2->S1->Regular SCO */
            index = 0;
        }
        else
        {
            /* Set the index "1" to form the negotiation order S3->S2->S1->Regular SCO */
            index = 1;
        }
    }
    return index;
}

void CsrBtHfSendCmScoConnectReq(HfInstanceData_t *linkPtr, CsrUint8 default_setting)
{
    CsrBtCmScoCommonParms *parms;
    audioSetupParams_t *audioSetupParams;
    CsrUint16 parmsOffset;
    CsrUintFast32 idx;

    /* we need to handle the parms specified by the app */
    CsrUint16 parmCount = 1;

    parmCount          += csrBtHfGetNegotiateCount(linkPtr, default_setting);

    CsrBtCmCommonScoConnectPrepare(&parms, &parmsOffset, parmCount);

    /* Handle the parameters from the App(either default or configured by App)*/
#ifdef CSR_BT_INSTALL_HF_CONFIG_AUDIO
    if (linkPtr->audioSetupParams)
    {
        audioSetupParams = linkPtr->audioSetupParams;
    }
    else
#endif
    {
        audioSetupParams = &CSR_BT_HF_MAIN_INSTANCE_GET(linkPtr)->generalAudioParams;
    }

    CsrBtCmCommonScoConnectBuild(parms,
                                 &parmsOffset,
                                 audioSetupParams->theAudioQuality,
                                 audioSetupParams->theTxBandwidth,
                                 audioSetupParams->theRxBandwidth,
                                 audioSetupParams->theMaxLatency,
                                 audioSetupParams->theVoiceSettings,
                                 audioSetupParams->theReTxEffort);

    for (idx = csrBtHfGetNegotiateOffset(linkPtr, default_setting); idx < CSR_ARRAY_SIZE(scoNegotiationOrder); ++idx)
    {
        CsrBtCmCommonScoConnectBuild(parms,
                                     &parmsOffset,
                                     scoNegotiationOrder[idx].theAudioQuality,
                                     scoNegotiationOrder[idx].theTxBandwidth,
                                     scoNegotiationOrder[idx].theRxBandwidth,
                                     scoNegotiationOrder[idx].theMaxLatency,
                                     scoNegotiationOrder[idx].theVoiceSettings,
                                     scoNegotiationOrder[idx].theReTxEffort);
    }
    CsrBtCmScoConnectReqSend(CSR_BT_HF_IFACEQUEUE,
                             linkPtr->pcmSlot,
                             linkPtr->pcmReassign,
                             parms,
                             parmCount,
                             linkPtr->hfConnId);
}

/*************************************************************************************
    Send CSR_BT_CM_SDS_REGISTER_REQ to register service record for HF
************************************************************************************/
void CsrBtHfSendSdsRegisterReq(HfMainInstanceData_t *instData)
{
    CsrUint8 * record;
    CsrUint16 num_rec_bytes;

    /* copy the record */
    num_rec_bytes = sizeof(sdsHfServiceRecord);
    record = (CsrUint8 *) CsrPmemAlloc(num_rec_bytes );
    SynMemCpyS(record, num_rec_bytes, sdsHfServiceRecord, num_rec_bytes);

    /* insert the server channel found during register in appropriate place */
    record[HF_SERVER_CHANNEL_INDEX] = instData->hfServerChannel;
    record[HF_SUPPORTED_FEATURES_INDEX + 1] =
        (CsrUint8)((instData->localSupportedFeatures) & CSR_BT_HFP_SDP_SUPPORT_MASK);
    if (instData->localSupportedFeatures & CSR_BT_HF_SUPPORT_CODEC_NEGOTIATION)
    {
        record[HF_SUPPORTED_FEATURES_INDEX + 1] |= CSR_BT_HFP_SDP_CODEC_NEGOTIATION;
        if (instData->supportedCodecsMask & CSR_BT_WBS_LC3SWB_CODEC_MASK)
        {/* Update Super Wideband Speech bit in Most significant byte of 2 byte Supported features */
            record[HF_SUPPORTED_FEATURES_INDEX] |= (CSR_BT_HFP_SDP_SWBS >> 8);
        }
    }

    if (instData->localSupportedFeatures & CSR_BT_HF_SUPPORT_ENHANCE_VOICE_RECOG_STATUS)
    {
        record[HF_SUPPORTED_FEATURES_INDEX + 1] |= CSR_BT_HFP_SDP_ENHANCED_VOICE_RECOG_STATUS;
    }

    if (instData->localSupportedFeatures & CSR_BT_HF_SUPPORT_VOICE_RECOG_TEXT)
    {
        record[HF_SUPPORTED_FEATURES_INDEX + 1] |= CSR_BT_HFP_SDP_VOICE_RECOG_TEXT;
    }

    CsrBtCmSdsRegisterReqSend(CSR_BT_HF_IFACEQUEUE,
                                 record,
                                 num_rec_bytes,
                                 instData->hfServerChannel);
}

/*************************************************************************************
    Send CSR_BT_CM_SDS_REGISTER_REQ to register service record for HS
************************************************************************************/
void HsSendSdsRegisterReq(HfMainInstanceData_t *instData)
{
    CsrUint8 * record;
    CsrUint16 num_rec_bytes;

    /* copy the record */
    num_rec_bytes = sizeof(sdsHsServiceRecord);
    record = (CsrUint8 *) CsrPmemAlloc(num_rec_bytes );
    SynMemCpyS(record, num_rec_bytes, sdsHsServiceRecord, num_rec_bytes);

    /* insert the server channel found during register in appropriate place */
    record[HS_SERVER_CHANNEL_INDEX] = instData->hsServerChannel;
    record[HS_REMOTE_AUDIO_INDEX] =
        !(instData->mainConfig & CSR_BT_HF_CNF_DISABLE_REMOTE_VOLUME_CONTROL);
    CsrBtCmSdsRegisterReqSend(CSR_BT_HF_IFACEQUEUE,
                                 record,
                                 num_rec_bytes,
                                 instData->hsServerChannel);
}

void sendBia(HfMainInstanceData_t * instData)
{
    CsrUintFast16   i = 1, len;
    CsrUint8    *body;
    CsrBtHfIndicatorActivationReq *prim = (CsrBtHfIndicatorActivationReq *)instData->recvMsgP;
    HfInstanceData_t *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    len = (CsrUint16)((linkPtr->nrOfIndicators *2) + BIA_CMD_INDEX);
    /* Allocate space enough for a message with the number of indicators supported by the remote device */
    body = CsrPmemAlloc(len+1);
    CsrMemSet(body,0,len+1);
    SynMemCpyS(body, len+1, BIA_CMD, BIA_CMD_INDEX);
    for (i=BIA_CMD_INDEX; i<len ; i++)
    {
        CsrUint8 value = (prim->indicatorBitMask & 1) + '0';
        body[i] = value;
        i++;
        if (i<(len-1))
        {
            body[i] = ',';
            /* shift value one bit (divide by two) to get the next bit value */
            prim->indicatorBitMask  >>= 1;
        }
        else
        {
            body[i] = '\r';
        }
    }
    body[i] = 0;
    if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
    {
        instData->linkData[instData->index].lastAtCmdSent = bia;
    }
    CsrBtHfHsSendCmDataReq(instData,(CsrUint16) len, body);
}

void sendBrsf(HfMainInstanceData_t * instData)
{
    CsrUint16    length, strLen;
    CsrUint8    *body;
    CsrCharString *charStr;

    charStr = (CsrCharString *) CsrPmemZalloc(I2B10_MAX);
    body = NULL;

    CsrIntToBase10(instData->localSupportedFeatures, charStr);

    strLen = (CsrUint16)CsrStrLen(charStr);
    length = SUPPORT_FEATURES_LENGTH + strLen +1;

    /* Add "AT+BRSF="*/
    body = (CsrUint8 *) CsrPmemZalloc(length);
    SynMemCpyS(body, length, SUPPORT_FEATURES, SUPPORT_FEATURES_LENGTH);
    body[SUPPORT_FEATURES_LENGTH] = '\0';

    /*Append supported feature string*/
    CsrStrLCat((CsrCharString *) body, charStr, length);
    body[length - 1] = '\r';

    CsrPmemFree(charStr);


    if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
    {
        instData->linkData[instData->index].lastAtCmdSent = brsf;
    }
    CsrBtHfHsSendCmDataReq(instData, length, body);
}

static CsrUint8 hfNumberOfStandardCodecsSupported(HfMainInstanceData_t * instData)
{
    CsrUint8 supportedCodecs = instData->supportedCodecsMask;
    CsrUint8 codecs = 0;

    do
    {
        if (supportedCodecs & 1)
        {
            codecs++;
        }
        supportedCodecs >>= 1;
    } while (supportedCodecs);
    return (codecs);
}

static CsrUint8 hfNumberOfOptionalCodecsSupported(HfMainInstanceData_t * instData)
{
#ifdef HF_ENABLE_OPTIONAL_CODEC_SUPPORT
    return instData->optionalCodecCount;
#else /* !HF_ENABLE_OPTIONAL_CODEC_SUPPORT */
    CSR_UNUSED(instData);
    return 0;
#endif
}

void sendCodecSupport(HfMainInstanceData_t * instData)
{
    CsrUint8        *dataPtr;
    CsrUint16       strLen;
    CsrUint8        i, codecIdx, stdCodecCount, optCodecCount, totalCodecCount;
    HfCodecId       *codecList;

    /* Standard codec count */
    stdCodecCount = hfNumberOfStandardCodecsSupported(instData);
    /* Optional codec count */
    optCodecCount = hfNumberOfOptionalCodecsSupported(instData); 
    /* Number of codec supported, both standard and optional */
    totalCodecCount = stdCodecCount + optCodecCount;
    codecList =  CsrPmemZalloc(totalCodecCount * sizeof(HfCodecId));

    /* Add standard codecs to the codec list */
    codecIdx = 0;
    for (i = 0; stdCodecCount > 0; i++)
    {
        if((instData->supportedCodecsMask & (1 << i)) > 0)
        {
            HfCodecId codecId = i+1;
    
            codecList[codecIdx] = codecId;
            codecIdx++;
            stdCodecCount--;
        }
    }

    /* Add optional codecs to codec list */
#ifdef HF_ENABLE_OPTIONAL_CODEC_SUPPORT
    SynMemCpyS((codecList + codecIdx), (optCodecCount * sizeof(HfCodecId)), instData->optionalCodecList, (optCodecCount * sizeof(HfCodecId)));
#endif

    /* Add AT command to the AT string */
    strLen = (CsrUint16)(CsrStrLen(CODEC_SUPPORT));
    dataPtr = (CsrUint8 *)CsrPmemZalloc(strLen);
    SynMemCpyS(dataPtr, strLen, CODEC_SUPPORT, strLen);

    /* Add Codec IDs to the AT string */
    i = 0;
    do
    {
        CsrCharString *tempStr;

        tempStr = (CsrCharString *)CsrPmemZalloc(I2B10_MAX);
        CsrIntToBase10(codecList[i], tempStr);
        appendString((CsrCharString **) &dataPtr, &strLen, &tempStr);

        i++;
        if(i < totalCodecCount)
        {
            dataPtr[strLen - 1] = ',';
        }
        else
        {
            dataPtr[strLen - 1] = '\r';
        }

    } while (i < totalCodecCount);

    if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
    {
        instData->linkData[instData->index].lastAtCmdSent = bac;
    }
    CsrBtHfHsSendCmDataReq(instData, strLen, dataPtr);
}

void CsrBtHfSendAtBcc(HfMainInstanceData_t *instData)
{
    CsrUint8    *body = CsrPmemAlloc(BCC_CMD_LENGTH);

    SynMemCpyS(body, BCC_CMD_LENGTH, BCC_CMD,BCC_CMD_LENGTH);

    if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
    {
        instData->linkData[instData->index].lastAtCmdSent = bcc;
    }
    CsrBtHfHsSendCmDataReq(instData, BCC_CMD_LENGTH, body);
}

void CsrBtHfSendAtBcs(HfMainInstanceData_t *instData)
{
    CsrUint8         *dataPtr;
    CsrUint16         strLen;
    CsrCharString    *tempStr;
    HfInstanceData_t *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);


    /* Add AT command to the AT string */
    strLen = (CsrUint16)(CsrStrLen(BCS_CMD));
    dataPtr = (CsrUint8 *)CsrPmemZalloc(strLen);
    SynMemCpyS(dataPtr, strLen, BCS_CMD, strLen);

    /* Add selected Codec ID to the AT string */
    tempStr = (CsrCharString *)CsrPmemZalloc(I2B10_MAX);
    CsrIntToBase10(linkPtr->codecToUse, tempStr);
    appendString((CsrCharString **) &dataPtr, &strLen, &tempStr);

    dataPtr[strLen - 1] = '\r';

    if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
    {
        instData->linkData[instData->index].lastAtCmdSent = bcs;
    }
    CsrBtHfHsSendCmDataReq(instData, strLen, dataPtr);
}

void sendCindSupport(HfMainInstanceData_t * instData)
{
    CsrUint8        *body;

    body = CsrPmemAlloc(CIND_SUPPORT_LENGTH);
    SynMemCpyS(body, CIND_SUPPORT_LENGTH, CIND_SUPPORT, CIND_SUPPORT_LENGTH);
    if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
    {
        instData->linkData[instData->index].lastAtCmdSent = cindSupportCmd;
    }
    CsrBtHfHsSendCmDataReq(instData, CIND_SUPPORT_LENGTH, body);
}

void sendCindStatus(HfMainInstanceData_t * instData)
{
    CsrUint8        *body;

    body = CsrPmemAlloc(CIND_STATUS_LENGTH);
    SynMemCpyS(body, CIND_STATUS_LENGTH, CIND_STATUS, CIND_STATUS_LENGTH);
    if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
    {
        instData->linkData[instData->index].lastAtCmdSent = cindStatusCmd;
    }
    CsrBtHfHsSendCmDataReq(instData, CIND_STATUS_LENGTH, body);
}

void sendSetCmer(HfMainInstanceData_t * instData,CsrBool enable)
{
    CsrUint8        *body;

    body = CsrPmemAlloc(SET_CMER_LENGTH);
    if (enable)
    {
        SynMemCpyS(body, SET_CMER_LENGTH, SET_CMER, SET_CMER_LENGTH);
    }
    else
    {
        SynMemCpyS(body, SET_CMER_LENGTH, RESET_CMER, SET_CMER_LENGTH);
    }

    if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
    {
        instData->linkData[instData->index].lastAtCmdSent = cmer;
    }
    CsrBtHfHsSendCmDataReq(instData, SET_CMER_LENGTH, body);
}

void sendCallHoldStatus(HfMainInstanceData_t * instData)
{
    CsrUint8        *body;

    body = CsrPmemAlloc(CALL_HOLD_STATUS_LENGTH);
    SynMemCpyS(body, CALL_HOLD_STATUS_LENGTH, CALL_HOLD_STATUS, CALL_HOLD_STATUS_LENGTH);
    if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
    {
        instData->linkData[instData->index].lastAtCmdSent = chldSupport;
    }
    CsrBtHfHsSendCmDataReq(instData, CALL_HOLD_STATUS_LENGTH, body);
}

static void buildHfgServiceTagList(HfInstanceData_t *linkPtr, CmnCsrBtLinkedListStruct **sdpTagList, CsrUint16 *shIndex, CsrBool outgoing)
{
    HfMainInstanceData_t *instData = CSR_BT_HF_MAIN_INSTANCE_GET(linkPtr);

    *sdpTagList = CsrBtUtilSdrCreateServiceHandleEntryFromUuid32(*sdpTagList, CSR_BT_HFG_PROFILE_UUID, shIndex);

    CsrBtUtilSdrInsertLocalServerChannel(*sdpTagList, *shIndex, CSR_BT_NO_SERVER);

    if (outgoing)
    { /* Outgoing connection: make sure to find the remote server channel to use!*/
#ifdef INSTALL_CMN_ENHANCED_SDP_FEATURE
        CsrBtUtilSdrCreateAndInsertAttribute(*sdpTagList, *shIndex, CSR_BT_SERVICE_RECORD_HANDLE_ATTRIBUTE_IDENTIFIER, NULL, 0);
        CsrBtUtilSdrCreateAndInsertAttribute(*sdpTagList, *shIndex, CSR_BT_SERVICE_CLASS_ID_LIST, NULL, 0);
        CsrBtUtilSdrCreateAndInsertAttribute(*sdpTagList, *shIndex, CSR_BT_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_IDENTIFIER, NULL, 0);
#endif
        CsrBtUtilSdrCreateAndInsertAttribute(*sdpTagList, *shIndex, CSR_BT_BLUETOOTH_PROFILE_DESCRIPTOR_LIST_ATTRIBUTE_IDENTIFIER, NULL, 0);
        /* Now choose the attributes to search for.... */
        if ((instData->mainConfig & CSR_BT_HF_CNF_DISABLE_OUT_SDP_SEARCH) == 0)
        {/* Search not disabled: now add tags for attributes to search. Always search for remote protocol verison number at least */
            if ((instData->mainConfig & CSR_BT_HF_CNF_DISABLE_OUT_SERVICE_NAME_SEARCH) == 0)
            {/* Service name search not disabled */
                CsrBtUtilSdrCreateAndInsertAttribute(*sdpTagList, *shIndex, CSR_BT_SERVICE_NAME_ATTRIBUTE_IDENTIFIER, NULL, 0);
            }
            if ((instData->mainConfig & CSR_BT_HF_CNF_DISABLE_OUT_NETWORK_SEARCH) == 0)
            {/* Network attribute search not disabled */
                CsrBtUtilSdrCreateAndInsertAttribute(*sdpTagList, *shIndex, CSR_BT_NETWORK_ATTRIBUTE_IDENTIFIER, NULL, 0);
            }
            if ((instData->mainConfig & CSR_BT_HF_CNF_DISABLE_OUT_SUP_FEATURES_SEARCH) == 0)
            {/* Supported features search not disabled */
                CsrBtUtilSdrCreateAndInsertAttribute(*sdpTagList, *shIndex, CSR_BT_SUPPORTED_FEATURES_ATTRIBUTE_IDENTIFIER, NULL, 0);
            }
        }
    }
    else
    { /* This is an incoming HF connection; find out what to look for (if anything at all!) */
        if ((instData->mainConfig & CSR_BT_HF_CNF_DISABLE_INC_SDP_SEARCH) == 0)
        {/* search enabled */
            CsrBtUtilSdrCreateAndInsertAttribute(*sdpTagList, *shIndex, CSR_BT_BLUETOOTH_PROFILE_DESCRIPTOR_LIST_ATTRIBUTE_IDENTIFIER, NULL, 0);
            if ((instData->mainConfig & CSR_BT_HF_CNF_DISABLE_INC_SERVICE_NAME_SEARCH) == 0)
            {/* Service name search not disabled */
                CsrBtUtilSdrCreateAndInsertAttribute(*sdpTagList, *shIndex, CSR_BT_SERVICE_NAME_ATTRIBUTE_IDENTIFIER, NULL, 0);
            }
            if ((instData->mainConfig & CSR_BT_HF_CNF_DISABLE_INC_NETWORK_SEARCH) == 0)
            {/* Network attribute search not disabled */
                CsrBtUtilSdrCreateAndInsertAttribute(*sdpTagList, *shIndex, CSR_BT_NETWORK_ATTRIBUTE_IDENTIFIER, NULL, 0);
            }
            if ((instData->mainConfig & CSR_BT_HF_CNF_DISABLE_INC_SUP_FEATURES_SEARCH) == 0)
            {/* Supported features search not disabled */
                CsrBtUtilSdrCreateAndInsertAttribute(*sdpTagList, *shIndex, CSR_BT_SUPPORTED_FEATURES_ATTRIBUTE_IDENTIFIER, NULL, 0);
            }
        }
    }
}

static void buildAgServiceTagList(HfInstanceData_t *linkPtr, CmnCsrBtLinkedListStruct **sdpTagList, CsrUint16 *shIndex)
{
    *sdpTagList = CsrBtUtilSdrCreateServiceHandleEntryFromUuid32(*sdpTagList,
                                                                 CSR_BT_HEADSET_AG_SERVICE_UUID,
                                                                 shIndex);
    CsrBtUtilSdrInsertLocalServerChannel(*sdpTagList,
                                         *shIndex,
                                         CSR_BT_NO_SERVER);

    CSR_UNUSED(linkPtr);
}

static void csrHfSdcRfcResultHandler(CsrSdcOptCallbackType cbType, void *context)
{
    switch(cbType)
    {
        case CSR_SDC_OPT_CB_CON_SELECT_SERVICE_HANDLE:
        {
            CsrRfcConSelectServiceHandleType *params = (CsrRfcConSelectServiceHandleType *) context;

            CsrBtHfSdcSelectServiceHandler(params->instData,
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

            CsrBtHfRfcSdcConResultHandler(params->instData,
                                          params->localServerCh,
                                          params->btConnId,
                                          params->deviceAddr,
                                          params->maxFrameSize,
                                          params->validPortPar,
                                          params->portPar,
                                          params->resultCode,
                                          params->resultSupplier,
                                          params->sdpTag);
        }
        break;
        case CSR_SDC_OPT_CB_SEARCH_RESULT:
        {
            CsrSdcResultFuncType *params = (CsrSdcResultFuncType *)context;

            CsrBtHfSdcResultHandler(params->instData,
                                    params->sdpTagList,
                                    params->deviceAddr,
                                    params->resultCode,
                                    params->resultSupplier);
        }
        break;

        default:
            break;
    }
}

void startSdcFeatureSearch(HfMainInstanceData_t * instData, CsrBool outgoing)
{
    CmnCsrBtLinkedListStruct    *sdpTagList = NULL;
    CsrUint16                    shIndex;
    CsrBtDeviceAddr              deviceAddr = instData->currentDeviceAddress;
    HfInstanceData_t            *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);
    CsrUint8                     searchIdxHf = 0xFF;
    CsrUint8                     searchIdxHs = 0xFF;

    /* Find first available entry if outgoing...*/
    if (outgoing)
    {
        CsrBtHfServiceConnectReq * prim;
        CsrIntFast8 i;

        prim = (CsrBtHfServiceConnectReq *) instData->recvMsgP;

        if (prim->connectionType != CSR_BT_HF_CONNECTION_HS)
        {
            for (i = 0; i < instData->maxHFConnections; i++)
            {
                linkPtr = (HfInstanceData_t *) &(instData->linkData[i]);

                if (linkPtr->state == Activate_s)
                { /* hf Index found */
                    searchIdxHf = i;
                    break;
                }
            }
        }
        if (prim->connectionType != CSR_BT_HF_CONNECTION_HF)
        {/* Now find HS index if HS connection to be tried...*/
            for (i = instData->maxHFConnections; i < (instData->maxHFConnections + instData->maxHSConnections); i++)
            {
                linkPtr = (HfInstanceData_t *)&(instData->linkData[i]);
                if (linkPtr->state == Activate_s)
                {
                    /* Whether connection request was initiated due to user action or reconnection attempt */
                    linkPtr->userAction = prim->userAction;
                    searchIdxHs = i;
                    break;
                }
            }
        }
    }
    else if (linkPtr->searchOngoing == FALSE)
    {/* Perform HF search if not performing HF search already and
        either trying to connect or already connected in an incoming connection */
        if(instData->index < instData->maxHFConnections)
        {
            searchIdxHf = instData->index;
        }
        else
        {
            searchIdxHs= instData->index;
        }
    }

    if (searchIdxHf != 0xFF)
    {
        linkPtr = (HfInstanceData_t*) &(instData->linkData[searchIdxHf]);
        buildHfgServiceTagList(linkPtr, &sdpTagList, &shIndex, outgoing);

        linkPtr->searchOngoing = TRUE;
        linkPtr->searchAndCon = outgoing;
        /* Set default values before search operation, to ensure spec compliance if search fails or needs not be performed.*/
        linkPtr->supportedFeatures = HF_HFG_DEFAULT_SUPPORTED_FEATURES;
        linkPtr->network = 1; /* Default according to HFP spec.*/

        if (!linkPtr->serviceName)
        {
            linkPtr->serviceName = CsrPmemZalloc(CSR_BT_MAX_FRIENDLY_NAME_LEN + 1);
        }
        CsrUtf8StrNCpyZero(linkPtr->serviceName,
                           (const CsrUtf8String *) "Audio Gateway",
                           CSR_BT_MAX_FRIENDLY_NAME_LEN + 1);
        CsrUtf8StrTruncate(linkPtr->serviceName,
                           CSR_BT_MAX_FRIENDLY_NAME_LEN);

        if (outgoing)
        {
            linkPtr->oldState = Connect_s;
            linkPtr->currentDeviceAddress = instData->currentDeviceAddress;
        }

        deviceAddr = linkPtr->currentDeviceAddress;
        linkPtr->state = ServiceSearch_s;
    }

    if (searchIdxHs != 0xFF)
    {
        linkPtr = (HfInstanceData_t*) &(instData->linkData[searchIdxHs]);
        buildAgServiceTagList(linkPtr, &sdpTagList, &shIndex);

        linkPtr->searchOngoing = TRUE;
        linkPtr->searchAndCon = outgoing;

        if (outgoing)
        {
            linkPtr->oldState = Activate_s;
            linkPtr->currentDeviceAddress = instData->currentDeviceAddress;
            linkPtr->state = Connect_s;
        }
    }

   if(sdpTagList != NULL)
   {
        if (outgoing)
        {
            dm_security_level_t          secOutgoing;

            /* If this is already allocated, the previous request was not released and hence the memory can be reused. */
            if(linkPtr->sdpSearchData == NULL)
            {
                /* Instance is allocated on connection request either outgoing or incoming. */
                linkPtr->sdpSearchData = CsrBtUtilSdpRfcInit(csrHfSdcRfcResultHandler,
                                                             (CSR_SDC_OPT_CB_SELECT_SVC_HANDLE_MASK | CSR_SDC_OPT_CB_RFC_CON_RESULT_MASK),
                                                             CSR_BT_HF_IFACEQUEUE,
                                                             linkPtr->instId);
            }

#ifndef INSTALL_HF_CUSTOM_SECURITY_SETTINGS
            CsrBtScSetSecOutLevel(&secOutgoing,
                                  CSR_BT_SEC_DEFAULT,
                                  CSR_BT_HANDSFREE_MANDATORY_SECURITY_OUTGOING,
                                  CSR_BT_HANDSFREE_DEFAULT_SECURITY_OUTGOING,
                                  CSR_BT_RESULT_CODE_HF_SUCCESS,
                                  CSR_BT_RESULT_CODE_HF_UNACCEPTABLE_PARAMETER);
#else
            secOutgoing = instData->secOutgoing;
#endif /* INSTALL_HF_CUSTOM_SECURITY_SETTINGS */

            CsrBtUtilRfcConStart((void *)linkPtr,
                                 linkPtr->sdpSearchData,
                                 sdpTagList,
                                 deviceAddr,
                                 secOutgoing,
                                 FALSE,
                                 NULL,
                                 CSR_BT_HF_PROFILE_DEFAULT_MTU_SIZE,
                                 CSR_BT_MODEM_SEND_CTRL_DTE_DEFAULT,
                                 0,
                                 CSRMAX(CSR_BT_SC_DEFAULT_ENC_KEY_SIZE,
                                        CSR_BT_HANDSFREE_DEFAULT_ENC_KEY_SIZE_VAL));
        }
        else
        {
            if(linkPtr->sdpSearchData == NULL)
            {
                linkPtr->sdpSearchData = CsrBtUtilSdcInit(csrHfSdcRfcResultHandler, CSR_BT_HF_IFACEQUEUE);
            }

            CsrBtUtilSdcSearchStart((void *)linkPtr, linkPtr->sdpSearchData, sdpTagList, deviceAddr);
        }
   }
   else
   {/* search not started! If outgoing, let the app know */
       if (outgoing)
       {
           CsrBtHfSendHfFailedServiceConnectCfm(instData, deviceAddr, CSR_BT_RESULT_CODE_HF_SDC_SEARCH_FAILED, CSR_BT_SUPPLIER_HF);
       }
   }
}

static CsrBool csrBtHfSdpGetBluetoothProfileDescriptorList(CmnCsrBtLinkedListStruct *bll_p,
                                                        CsrUint16  serviceHandleIndex,
                                                        CsrUint16  *version)
{
    CsrBool    retBool = FALSE;

    CsrUint8  *att_p;
    CsrUintFast16 nofAttributes,x;
    CsrUint16   attDataLen, nofBytesToAttribute, emptyAttSize, consumedBytes, totalConsumedBytes = 0, tempVar;
    CsrUint32  returnValue, protocolValue;

    if (TRUE == CsrBtUtilSdrGetNofAttributes(bll_p, serviceHandleIndex, &nofAttributes))
    {
        for (x=0; x<nofAttributes; x++)
        {
            att_p = CsrBtUtilSdrGetAttributePointer(bll_p, serviceHandleIndex,(CsrUint16) x, &nofBytesToAttribute);

            if (att_p)
            {
                /* Check if the UUID in the 'outer' attribute struct is correct */
                SynMemCpyS(&tempVar, SDR_ENTRY_SIZE_SERVICE_UINT16, att_p + SDR_ENTRY_INDEX_ATTRIBUTE_UUID, SDR_ENTRY_SIZE_SERVICE_UINT16);
                if (CSR_BT_BLUETOOTH_PROFILE_DESCRIPTOR_LIST_ATTRIBUTE_IDENTIFIER == tempVar)
                {
                    CsrBtUtilSdrGetEmptyAttributeSize(&emptyAttSize);
                    SynMemCpyS(&tempVar, SDR_ENTRY_SIZE_SERVICE_UINT16, att_p, SDR_ENTRY_SIZE_SERVICE_UINT16);
                    attDataLen = tempVar - emptyAttSize + SDR_ENTRY_SIZE_TAG_LENGTH;

                    /* First extract the attribute uuid from the attribute SDP data */
                    if (TRUE == CsrBtUtilSdpExtractUint(att_p + SDR_ENTRY_INDEX_ATTRIBUTE_DATA,
                                                  attDataLen,
                                                  &returnValue,
                                                  &consumedBytes,
                                                  FALSE))
                    {
                        /* Check if the UUID in the 'inner' attribute sdp data struct is correct */
                        if (CSR_BT_BLUETOOTH_PROFILE_DESCRIPTOR_LIST_ATTRIBUTE_IDENTIFIER == returnValue)
                        {
                            attDataLen = attDataLen - consumedBytes;
                            totalConsumedBytes += consumedBytes;
                            /* first find the protocol UUID */
                            if (TRUE == CsrBtUtilSdpExtractUint(att_p + SDR_ENTRY_INDEX_ATTRIBUTE_DATA + totalConsumedBytes,
                                                          attDataLen,
                                                          &protocolValue,
                                                          &consumedBytes,
                                                          TRUE))
                            {
                                attDataLen = attDataLen - consumedBytes;
                                totalConsumedBytes += consumedBytes;
                                /* Now find the value */
                                if (TRUE == CsrBtUtilSdpExtractUint(att_p + SDR_ENTRY_INDEX_ATTRIBUTE_DATA + totalConsumedBytes,
                                                              attDataLen,
                                                              &returnValue,
                                                              &consumedBytes,
                                                              TRUE))
                                {
                                    attDataLen = attDataLen - consumedBytes;
                                    totalConsumedBytes += consumedBytes;

                                    if (CSR_BT_HF_PROFILE_UUID == protocolValue)
                                    {
                                        *version = (CsrUint16)returnValue;
                                        retBool = TRUE;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return retBool;
}

static void csrBtHfUtilCarryOnAfterSdp(HfInstanceData_t    *linkPtr,
                                       CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    /* This function is only called if the connection establsihement failed or if it succeeded and this is a HF connection */
    HfMainInstanceData_t *instData = CSR_BT_HF_MAIN_INSTANCE_GET(linkPtr);

    if (resultCode == CSR_BT_RESULT_CODE_HF_SUCCESS &&
        resultSupplier == CSR_BT_SUPPLIER_HF)
    {
        if ((linkPtr->disconnectReqReceived) || (linkPtr->pendingCancel))
        {
            if (linkPtr->disconnectPeerReceived)
            {
                CsrBtHfSendHfDisconnectInd(instData, CSR_BT_RESULT_CODE_HF_SUCCESS, CSR_BT_SUPPLIER_HF);
                CsrBtHfSaveQueueCleanUp(instData);
                linkPtr->linkState = CSR_BT_LINK_STATUS_DISCONNECTED;
                linkPtr->state = Activate_s;
                CsrBtHfAllowConnectCheck(instData);
            }
            else if(linkPtr->disconnectReqReceived)
            {
                CsrBtCmDisconnectReqSend(linkPtr->hfConnId);
            }
            else
            { /* pendingCancel */
                if (linkPtr->state == Connected_s)
                {
                    CsrBtHfSendHfDisconnectInd(instData, CSR_BT_RESULT_CODE_HF_CANCELLED_CONNECT_ATTEMPT, CSR_BT_SUPPLIER_HF);
                }
                else
                {
                    /* App does not not about connection, so simply
                     * make it look like it was cancelled */
                    CsrBtHfSendHfServiceConnectInd(instData, CSR_BT_RESULT_CODE_HF_CANCELLED_CONNECT_ATTEMPT, CSR_BT_SUPPLIER_HF);
                }
                CsrBtHfSaveQueueCleanUp(instData);
                linkPtr->linkState = CSR_BT_LINK_STATUS_DISCONNECTED;
                linkPtr->state = Activate_s;
                CsrBtHfAllowConnectCheck(instData);
                CsrBtHfInitInstanceData(linkPtr);
            }
        }
        else
        {
            if (linkPtr->disconnectPeerReceived)
            {
                CsrBtHfSaveQueueCleanUp(instData);
                linkPtr->linkState = CSR_BT_LINK_STATUS_DISCONNECTED;
                linkPtr->state = Activate_s;
                if (linkPtr->oldState == Connect_s)
                {
                    CsrBtHfSendHfDisconnectInd(instData, CSR_BT_RESULT_CODE_HF_SUCCESS, CSR_BT_SUPPLIER_HF);
                }
                CsrBtHfAllowConnectCheck(instData);
                CsrBtHfInitInstanceData(linkPtr);
            }
            else
            {
                /* SDC finished. */
                if ((linkPtr->oldState == Activate_s) || (linkPtr->oldState == Connect_s))
                {
                    linkPtr->serviceState = btConnect_s;

                    if(linkPtr->oldState == Connect_s)
                    {
                        CsrBtHfCancelAcceptOnConnectingChannel(linkPtr);
                    }
                    /*If TRANSPARENT mode, then the App will send BRSF */
                    if ((instData->mainConfig & CSR_BT_HF_AT_MODE_TRANSPARENT_ENABLE) == CSR_BT_HF_AT_MODE_TRANSPARENT_ENABLE)
                    {
                        linkPtr->state = Connected_s;
                        linkPtr->atSequenceState = serviceLevel; /* Must set to guarantee that BT_HF_DISCONNECT_IND is issued properly */
                        CsrBtHfSendHfServiceConnectInd(instData, CSR_BT_RESULT_CODE_HF_SUCCESS, CSR_BT_SUPPLIER_HF);
                        CsrBtHfAcceptIncomingSco(linkPtr);
                        /* Make sure not to allow more connections than indicated by the application */
                        CsrBtHfCancelAcceptCheck(instData);
                    }
                    else
                    {
                        startAtFeatureSearch(instData);
                    }
                }
            }
        }
    }
    else
    {
        if ((linkPtr->disconnectPeerReceived) || (linkPtr->pendingCancel))
        {
            /* other side already disconnect rfcomm link */
            CsrBtHfSaveQueueCleanUp(instData);
            linkPtr->linkState = CSR_BT_LINK_STATUS_DISCONNECTED;
            linkPtr->state = Activate_s;
            CsrBtHfAllowConnectCheck(instData);
            if ((linkPtr->oldState == Connect_s) && (!linkPtr->pendingCancel))
            {
                CsrBtHfSendHfServiceConnectInd(instData, CSR_BT_RESULT_CODE_HF_CONNECT_ATTEMPT_FAILED, CSR_BT_SUPPLIER_HF);
            }
            else if (linkPtr->pendingCancel)
            {
                /* App does not not about connection, so simply
                 * make it look like it was cancelled */
                CsrBtHfSendHfServiceConnectInd(instData,CSR_BT_RESULT_CODE_HF_CANCELLED_CONNECT_ATTEMPT, CSR_BT_SUPPLIER_HF);
            }
            CsrBtHfInitInstanceData(linkPtr);
        }
        else
        {
            if (linkPtr->oldState == Activate_s)
            {
                /* SDC search failed, but rfcomm connection already established. Use default features for AG and start AT sequence */
                linkPtr->supportedFeatures = HF_HFG_DEFAULT_SUPPORTED_FEATURES;
                linkPtr->network = HF_HFG_DEFAULT_NETWORK_FEATURES;
                linkPtr->serviceState = btConnect_s;

                /*If TRANSPARENT mode, then the App will send BRSF/AT features */
                if ((instData->mainConfig & CSR_BT_HF_AT_MODE_TRANSPARENT_ENABLE) == CSR_BT_HF_AT_MODE_TRANSPARENT_ENABLE)
                {
                    linkPtr->state = Connected_s;
                    linkPtr->atSequenceState = serviceLevel; /* Must set to guarantee that BT_HF_DISCONNECT_IND is issued properly */
                    CsrBtHfSendHfServiceConnectInd(instData, CSR_BT_RESULT_CODE_HF_SUCCESS, CSR_BT_SUPPLIER_HF);
                    CsrBtHfAcceptIncomingSco(linkPtr);
                    /* Make sure not to allow more connections than indicated by the application */
                    CsrBtHfCancelAcceptCheck(instData);
                }
                else
                {
                    startAtFeatureSearch(instData);
                }
            }
            else
            {
                CsrBtHfSaveQueueCleanUp(instData);
                linkPtr->linkState = CSR_BT_LINK_STATUS_DISCONNECTED;
                linkPtr->state = Activate_s;
                CsrBtHfAllowConnectCheck(instData);
                if ((resultSupplier == CSR_BT_SUPPLIER_SDP_SDC || resultSupplier == CSR_BT_SUPPLIER_SDP_SDC_OPEN_SEARCH) && 
                       (resultCode == SDC_NO_RESPONSE_DATA))
                {
                    CsrBtHfSendHfServiceConnectInd(instData, CSR_BT_RESULT_CODE_HF_SDC_SEARCH_FAILED, CSR_BT_SUPPLIER_HF);
                }
                else
                {
                    CsrBtHfSendHfServiceConnectInd(instData, CSR_BT_RESULT_CODE_HF_CONNECT_ATTEMPT_FAILED, CSR_BT_SUPPLIER_HF);
                }
            }
        }
    }
}


void CsrBtHfSdcResultHandler(void                     * inst,
                             CmnCsrBtLinkedListStruct * sdpTagList,
                             CsrBtDeviceAddr          deviceAddr,
                             CsrBtResultCode          resultCode,
                             CsrBtSupplier      resultSupplier)
{
    CsrBtUuid32    tmpUuid = 0;
    CsrBtHfConnectionType   localConnectionType = CSR_BT_HF_CONNECTION_UNKNOWN;
    HfInstanceData_t    *linkPtr = (HfInstanceData_t *) inst;
    HfMainInstanceData_t *instData = CSR_BT_HF_MAIN_INSTANCE_GET(linkPtr);

    localConnectionType = linkPtr->linkType;

    linkPtr->searchOngoing =FALSE;
    linkPtr->searchAndCon = FALSE;

    if (!linkPtr->disconnectReqReceived && !linkPtr->disconnectPeerReceived && !linkPtr->pendingCancel)
    {
        if (resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS &&
            resultSupplier == CSR_BT_SUPPLIER_CM  &&
            sdpTagList != NULL)
        {
            CsrUintFast16    numOfSdpRecords = CsrBtUtilBllGetNofEntriesEx(sdpTagList);
            CsrUintFast16    sdpRecordIndex;
            CsrUint16    tmpResult;
            CsrUint16    dummy1, dummy2; /* Currently CSR_UNUSED */
            CsrUint32    returnValue;
            CsrUint8     *string;
            CsrUint16    stringLen;
            CsrUint16    version = 0;

            for (sdpRecordIndex = 0; sdpRecordIndex < numOfSdpRecords; sdpRecordIndex++)
            {
                if (CsrBtUtilSdrGetServiceUuid32AndResult(sdpTagList,
                                                sdpRecordIndex,
                                                &tmpUuid,
                                                &tmpResult,
                                                &dummy1,
                                                &dummy2))
                {
                    if (tmpResult == SDR_SDC_SEARCH_SUCCESS)
                    {
                        if (tmpUuid == CSR_BT_HFG_PROFILE_UUID)
                        { /* Handsfree connection */
                           localConnectionType = CSR_BT_HF_CONNECTION_HF;
                          if (TRUE == CsrBtUtilSdrGetUintAttributeDataValueFromAttributeUuid(sdpTagList, sdpRecordIndex,
                                                        CSR_BT_SUPPORTED_FEATURES_ATTRIBUTE_IDENTIFIER, &returnValue))
                          {
                              linkPtr->supportedFeatures = (CsrUint16) returnValue;
                          }
                          if (TRUE == CsrBtUtilSdrGetUintAttributeDataValueFromAttributeUuid(sdpTagList, sdpRecordIndex,
                                                        CSR_BT_NETWORK_ATTRIBUTE_IDENTIFIER, &returnValue))
                          {
                              linkPtr->network = (CsrUint8) returnValue;
                          }
                          if (TRUE == CsrBtUtilSdrGetStringAttributeFromAttributeUuid(sdpTagList, sdpRecordIndex, CSR_BT_SERVICE_NAME_ATTRIBUTE_IDENTIFIER, &string, &stringLen))
                          {
                              if (stringLen > CSR_BT_MAX_FRIENDLY_NAME_LEN)
                              {
                                  stringLen = CSR_BT_MAX_FRIENDLY_NAME_LEN;
                              }

                              if (!linkPtr->serviceName)
                              {
                                  linkPtr->serviceName = CsrPmemZalloc(CSR_BT_MAX_FRIENDLY_NAME_LEN + 1);
                              }
                              SynMemCpyS(linkPtr->serviceName, CSR_BT_MAX_FRIENDLY_NAME_LEN + 1, string, stringLen);
                              CsrUtf8StrTruncate(linkPtr->serviceName, stringLen);
                          }
                          if (TRUE == csrBtHfSdpGetBluetoothProfileDescriptorList(sdpTagList, (CsrUint16)sdpRecordIndex, &version))
                          {
                              linkPtr->remoteVersion = version;
                          }
                        }
                       else if (tmpUuid == CSR_BT_HEADSET_AG_SERVICE_UUID)
                        {  /* AG: handset connection  */
                           localConnectionType = CSR_BT_HF_CONNECTION_HS;
                        }
                    }
                    else
                    {/* This is a HS connection...*/
                        localConnectionType = CSR_BT_HF_CONNECTION_HS;
                    }
                }
            }
        }
    }

    CsrBtUtilBllFreeLinkedList(&sdpTagList, CsrBtUtilBllPfreeWrapper);

    /* Now that we have handled the SDC close message, decide what to do based on whether the
       service discovery operation went well or not, and on whether the application has decided to cancel
       or diconnect before connection establishement */
        if ((resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS) &&
            (resultSupplier == CSR_BT_SUPPLIER_CM) &&
            (localConnectionType != CSR_BT_HF_CONNECTION_UNKNOWN))
        {
            if (localConnectionType == CSR_BT_HF_CONNECTION_HF)
            {  /* HFP connection */
                csrBtHfUtilCarryOnAfterSdp(linkPtr, CSR_BT_RESULT_CODE_HF_SUCCESS, CSR_BT_SUPPLIER_HF);
                if (linkPtr->state == Connect_s)
                {
                   linkPtr->state = Activate_s;
                }
            }
            else
            {  /* HSP connection */
                if (((CsrBtCmConnectCfm*)(instData->recvMsgP))->type == CSR_BT_CM_CONNECT_CFM)
                {
                    CsrBtCmConnectCfm * prim;
                    prim = (CsrBtCmConnectCfm *) instData->recvMsgP;


                    linkPtr->currentDeviceAddress = prim->deviceAddr;
                    if (!linkPtr->disconnectReqReceived)
                    {
                        CsrBtHfCancelAcceptOnConnectingChannel(linkPtr);

                        linkPtr->linkState = CSR_BT_LINK_STATUS_CONNECTED;
                        linkPtr->hfConnId = prim->btConnId;
                        linkPtr->data->maxRfcFrameSize = prim->profileMaxFrameSize;
                        CsrBtHfAcceptIncomingSco(linkPtr);

                        if(linkPtr->userAction)
                        {
                            /* HS Connection was initiated on user action. Send "AT+CKPD=200" to AG */
                            sendCkpd(instData);
                        }

                        CsrBtHfSendHfServiceConnectInd(instData, CSR_BT_RESULT_CODE_HF_SUCCESS, CSR_BT_SUPPLIER_HF);
                        linkPtr->state = Connected_s;

                        /* Make sure not to allow more connections than indicated by the application */
                        CsrBtHfCancelAcceptCheck(instData);
                    }
                    else
                    {
                        CsrBtCmDisconnectReqSend(prim->btConnId);
                    }
                }
                else
                {
                    linkPtr->state = Connect_s;
                    linkPtr->linkType = CSR_BT_HF_CONNECTION_HS;
                    CsrBtHfpHandler(instData);
                }
            }
        }
        else
        {
            if (localConnectionType == CSR_BT_HF_CONNECTION_HS)
            { /* This execution path is possible only for failure case */
                if (linkPtr->pendingCancel)
                {
                    CsrBtHfSendHfServiceConnectInd(instData, CSR_BT_RESULT_CODE_HF_CANCELLED_CONNECT_ATTEMPT, CSR_BT_SUPPLIER_HF);
                    linkPtr->pendingCancel = FALSE;
                }
                else
                {
                    CsrBtHfSendHfServiceConnectInd(instData, CSR_BT_RESULT_CODE_HF_CONNECT_ATTEMPT_FAILED, CSR_BT_SUPPLIER_HF);
                }
                linkPtr->state = Activate_s;
            }
            else
            {/* HF_CONNECTION*/

                if (linkPtr->instReused &&
                    resultCode == CSR_BT_RESULT_CODE_CM_CANCELLED &&
                    resultSupplier == CSR_BT_SUPPLIER_CM)
                {
                    /* This instance is reused for incoming connection and the outgoing SDC search
                     * has been cancelled in favor of incoming. Send a house cleaning message in order to
                     * restore the incoming connection request message which was deffered in order
                     * for outgoing request to get handled. Inform the application regarding the cancellation
                     * of the outgoing connection request. */
                    CsrBtHfSendHfHouseCleaning(instData);
                    linkPtr->linkState = CSR_BT_LINK_STATUS_DISCONNECTED;
                    linkPtr->state = Activate_s;

                    CsrBtHfSendHfServiceConnectInd(instData, CSR_BT_RESULT_CODE_HF_CANCELLED_CONNECT_ATTEMPT, CSR_BT_SUPPLIER_HF);

                    /* Instance needs to get cleared to accomodate the saved incoming connection in the hf save queue. */
                    CsrBtHfInitInstanceData(linkPtr);
                }
                else
                {
                    if (((CsrBtCmConnectCfm*)(instData->recvMsgP))->type == CSR_BT_CM_CONNECT_CFM)
                    {   /* search was started from hf_main_sef because of connect req: outgoing connection */
                        if (resultCode != CSR_BT_RESULT_CODE_CM_SUCCESS ||
                            resultSupplier != CSR_BT_SUPPLIER_CM)
                        {
                            if (linkPtr->pendingCancel)
                            {
                                CsrBtHfSendHfServiceConnectInd(instData, CSR_BT_RESULT_CODE_HF_CANCELLED_CONNECT_ATTEMPT, CSR_BT_SUPPLIER_HF);
                                linkPtr->pendingCancel = FALSE;
                            }
                            else
                            {   /* Inform application that RFCOMM connection has failed */
                                CsrBtHfSendHfServiceConnectInd(instData, CSR_BT_RESULT_CODE_HF_CONNECT_ATTEMPT_FAILED, CSR_BT_SUPPLIER_HF);
                            }

                            CsrBtHfSaveQueueCleanUp(instData);
                            linkPtr->linkState = CSR_BT_LINK_STATUS_DISCONNECTED;
                            linkPtr->state = Activate_s;
                            CsrBtHfAllowConnectCheck(instData);
                        }
                        else
                        {
                            csrBtHfUtilCarryOnAfterSdp(linkPtr, CSR_BT_RESULT_CODE_HF_SUCCESS, CSR_BT_SUPPLIER_HF);
                        }
                    }
                    else
                    {   /* this is HF doing SDC search after another device has connected */
                        if (resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS &&
                            resultSupplier == CSR_BT_SUPPLIER_CM)
                        {
                            csrBtHfUtilCarryOnAfterSdp(linkPtr, CSR_BT_RESULT_CODE_HF_SUCCESS, CSR_BT_SUPPLIER_HF);
                        }
                        else
                        {
                            csrBtHfUtilCarryOnAfterSdp(linkPtr, resultCode, resultSupplier);
                        }
                    }
                }
            }
        }
    CSR_UNUSED(deviceAddr);
}

void CsrBtHfRfcSdcConResultHandler(void                        *inst,
                                   CsrUint8               localServerCh,
                                   CsrUint32                    hfConnId,
                                   CsrBtDeviceAddr             deviceAddr,
                                   CsrUint16                    maxFrameSize,
                                   CsrBool                      validPortPar,
                                   RFC_PORTNEG_VALUES_T        *portPar,
                                   CsrBtResultCode             resultCode,
                                   CsrBtSupplier         resultSupplier,
                                   CmnCsrBtLinkedListStruct    *sdpTagList)
{
    HfInstanceData_t *linkPtr = (HfInstanceData_t *) inst;
    HfMainInstanceData_t *instData = CSR_BT_HF_MAIN_INSTANCE_GET(linkPtr);
    CsrUint8 i = 0;

    CsrBtUuid32    tmpUuid = 0;
    CsrUint16    tmpResult;
    CsrUint16    dummy1, dummy2; /* Currently CSR_UNUSED */
    CsrBtHfConnectionType   localConnectionType = CSR_BT_HF_CONNECTION_UNKNOWN;

    if (resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS &&
        resultSupplier == CSR_BT_SUPPLIER_CM)
    {
        if (sdpTagList != NULL)
        {
            if (CsrBtUtilSdrGetServiceUuid32AndResult(sdpTagList,
                                            0,
                                            &tmpUuid,
                                            &tmpResult,
                                            &dummy1,
                                            &dummy2))
            {
                if (tmpResult == SDR_SDC_SEARCH_SUCCESS)
                {
                    if (tmpUuid == CSR_BT_HFG_PROFILE_UUID)
                    { /* Handsfree connection */
                        localConnectionType = CSR_BT_HF_CONNECTION_HF;
                    }
                    else if (tmpUuid == CSR_BT_HEADSET_AG_SERVICE_UUID)
                    { /* HS connection */
                        localConnectionType = CSR_BT_HF_CONNECTION_HS;
                    }
                }
            }
        }
    }

    /* Map the right instance if connection succeeded.
     * reset other connection Instances.
     */
    while(i < instData->maxHFConnections + instData->maxHSConnections)
    {
        CsrBtDeviceAddr devAddr;
        devAddr = instData->linkData[i].currentDeviceAddress;
        if(CsrBtBdAddrEq(&devAddr, &deviceAddr) &&
            instData->linkData[i].searchAndCon)
        {
            if (CSR_BT_HF_CONNECTION_UNKNOWN != localConnectionType)
            { /* Connection success case */
                if(instData->linkData[i].linkType != localConnectionType)
                { /* The other instance which needs reset */
                    instData->linkData[i].searchAndCon = FALSE;
                    instData->linkData[i].searchOngoing = FALSE;
                    instData->linkData[i].state = Activate_s;
                    CsrBtHfSetAddrInvalid(&instData->linkData[i].currentDeviceAddress);
                    instData->linkData[i].obtainedServerCh = CSR_BT_NO_SERVER;
                }
                else
                { /* Found the right instance to map */
                    instData->index = i;
                }
            }
            else
            { /* Connection failed case */
                if(instData->linkData[i].instId != linkPtr->instId)
                {
                    instData->linkData[i].searchAndCon = FALSE;
                    instData->linkData[i].searchOngoing = FALSE;
                    instData->linkData[i].state = Activate_s;
                    CsrBtHfSetAddrInvalid(&instData->linkData[i].currentDeviceAddress);
                }
            }
        }
        i++;
    }
    linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    /* SDC RFC lib obtained server channel need to be copied to the mapped instance.
     * This will be used to unregister CM server channel during disconnect.
     */
    if (resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS &&
        resultSupplier == CSR_BT_SUPPLIER_CM)
    {
        linkPtr->obtainedServerCh = localServerCh;
    }

    if (((resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && resultSupplier == CSR_BT_SUPPLIER_CM) ||
          (linkPtr->oldState == Activate_s)) &&
          (linkPtr->searchOngoing == TRUE) )
    {
        linkPtr->hfConnId = hfConnId;
        linkPtr->data->maxRfcFrameSize = maxFrameSize;
        linkPtr->oldState = Connect_s;
        linkPtr->serviceState = btConnect_s;
        linkPtr->linkState = CSR_BT_LINK_STATUS_CONNECTED;
        CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_CONTROL_CHANNEL,deviceAddr,hfConnId);
        if((instData->mainConfig & CSR_BT_HF_AT_MODE_TRANSPARENT_ENABLE) == CSR_BT_HF_AT_MODE_TRANSPARENT_ENABLE)
        {
            linkPtr->state = Connected_s;
        }
        else
        {
            linkPtr->state = ServiceSearch_s;
        }
    }
    CsrBtHfSdcResultHandler(linkPtr, sdpTagList, deviceAddr, resultCode, resultSupplier);

    csrBtHfSdpDeInit(linkPtr);
    CSR_UNUSED(validPortPar);
    CSR_UNUSED(portPar);
}

static CsrUint8 getUsedHfConnections(HfMainInstanceData_t *inst)
{
    CsrUint8 returnVal = 0;
    CsrUintFast8 i;

    for (i=0;i<inst->maxHFConnections;i++)
    {
        if (inst->linkData[i].state == Connected_s)
        {
            returnVal++;
        }
    }

    return returnVal;
}

static CsrUint8 getUsedHsConnections(HfMainInstanceData_t *inst)
{
    CsrUint8 returnVal = 0;
    CsrIntFast8 i;

    for (i=inst->maxHFConnections;i<(inst->maxHFConnections + inst->maxHSConnections);i++)
    {
        if (inst->linkData[i].state == Connected_s)
        {
            returnVal++;
        }
    }

    return returnVal;
}

void CsrBtHfSdcSelectServiceHandler(void                    * instData,
                               void                    * cmSdcRfcInstData,
                               CsrBtDeviceAddr            deviceAddr,
                               CsrUint8           serverChannel,
                               CsrUint16                entriesInSdpTaglist,
                               CmnCsrBtLinkedListStruct * sdpTagList)
{
    CsrUint16 *serviceHandleIndexList = CsrPmemAlloc(sizeof(CsrUint16) * MAX_SERVICE_RECORDS_SEARCH);
    CsrBtUuid32    tmpUuid = 0;
    CsrUint16    sdpRecordIndex;
    CsrUint16    idx = 0;
    CsrUint16    tmpResult;
    CsrUint16    dummy1,dummy2;
    HfInstanceData_t *linkPtr = (HfInstanceData_t *) instData;
    HfMainInstanceData_t *inst = CSR_BT_HF_MAIN_INSTANCE_GET(linkPtr);

    if (inst->maxHFConnections > getUsedHfConnections(inst))
    {/* Still possible to establish a HFP connection */

        /* First find all the HFG records*/
        for (sdpRecordIndex = 0; ((sdpRecordIndex < entriesInSdpTaglist) && (idx < MAX_SERVICE_RECORDS_SEARCH)); sdpRecordIndex++)
        {
            if (CsrBtUtilSdrGetServiceUuid32AndResult(sdpTagList,
                                            sdpRecordIndex,
                                            &tmpUuid,
                                            &tmpResult,
                                            &dummy1,
                                            &dummy2))
            {
                if (tmpResult == SDR_SDC_SEARCH_SUCCESS)
                {
                    if (tmpUuid == CSR_BT_HFG_PROFILE_UUID)
                    {
                        serviceHandleIndexList[idx] = sdpRecordIndex;
                        idx++;
                    }
                }
            }
        }
    }


    if (inst->maxHSConnections > getUsedHsConnections(inst))
    {/* Still possible to establish a HSP connection */

        for (sdpRecordIndex = 0; ((sdpRecordIndex < entriesInSdpTaglist) && (idx < MAX_SERVICE_RECORDS_SEARCH)); sdpRecordIndex++)
        {
            if (CsrBtUtilSdrGetServiceUuid32AndResult(sdpTagList,
                                            sdpRecordIndex,
                                            &tmpUuid,
                                            &tmpResult,
                                            &dummy1,
                                            &dummy2))
            {
                if (tmpResult == SDR_SDC_SEARCH_SUCCESS)
                {
                    if (tmpUuid == CSR_BT_HEADSET_AG_SERVICE_UUID)
                    {
                        serviceHandleIndexList[idx] = sdpRecordIndex;
                        idx++;
                    }
                }
            }
        }
    }

    /* Select the preferred service or services to connect to in prioritized order*/
    CsrBtUtilRfcConSetServiceHandleIndexList(instData, cmSdcRfcInstData, serviceHandleIndexList,idx);

    CSR_UNUSED(deviceAddr);
    CSR_UNUSED(serverChannel);
}

void sendCkpd(HfMainInstanceData_t * instData)
{
    CsrUint8        *body;

    body = CsrPmemAlloc(CKPD200_LENGTH);
    SynMemCpyS(body, CKPD200_LENGTH, CKPD200, CKPD200_LENGTH);
    if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
    {
        instData->linkData[instData->index].lastAtCmdSent = ckpd;
    }
    CsrBtHfHsSendCmDataReq(instData, CKPD200_LENGTH, body);
}


void startAtFeatureSearch(HfMainInstanceData_t * instData)
{
    HfInstanceData_t    *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    if ((linkPtr->remoteVersion != CSR_BT_FIRST_HFP_NO_ESCO) || (linkPtr->remoteVersion == 0))
    {/* Version 0.96 does not care about AT+BRSF; so do not send it if the remote
        version supported is found and is 0.96 */
        linkPtr->atSequenceState = supportFeatures;
        sendBrsf(instData);
    }
    else
    {
        linkPtr->atSequenceState = cindSupport;
        linkPtr->serviceState = serviceConnect_s;
        sendCindSupport(instData);
    }
    /* Make sure not to allow more connections than indicated by the application */
    CsrBtHfCancelAcceptCheck(instData);
}

/*************************************************************************************
    Send a CSR_BT_HF_CALL_HANDLING_IND to app
************************************************************************************/
void CsrBtHfSendHfCallHandlingInd(HfMainInstanceData_t *instData, CsrUint8 *atTextString)
{
    CsrBtHfCallHandlingInd  *prim;
    char *index_i;

    prim = (CsrBtHfCallHandlingInd  *)CsrPmemAlloc(sizeof(CsrBtHfCallHandlingInd));
    prim->type = CSR_BT_HF_CALL_HANDLING_IND;

    index_i = (char *)atTextString;
    index_i = CsrStrChr(index_i,' ');

    if(index_i == NULL)
    {
        index_i = (char *)atTextString;
        index_i = CsrStrChr(index_i,':');
    }
    if(index_i != NULL)
    {
        prim->event = index_i[1] - '0';
        prim->connectionId = instData->linkData[instData->index].hfConnId;
        CsrBtHfMessagePut(instData->appHandle, prim);
    }
    else
    {
        CsrPmemFree(prim);
    }
}

/*************************************************************************************
    Send AT+COPS (operator name format setting)
************************************************************************************/
void CsrBtHfAtCopsSetCommandSend(HfMainInstanceData_t *instData,CsrUint8 mode, CsrUint8 format)
{
    CsrUint8         *dataPtr;
    CsrUint16       strLen;

    strLen = (CsrUint16)(CsrStrLen(COPS_SET_FORMAT));
    dataPtr = (CsrUint8 *) CsrStrDup(COPS_SET_FORMAT);
    *(dataPtr + COPS_SET_FORMAT_INDEX) = format;
    *(dataPtr + COPS_SET_MODE_INDEX) = mode;
    instData->linkData[instData->index].data->dataReceivedInConnected = TRUE;
    if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
    {
        instData->linkData[instData->index].lastAtCmdSent = copsSet;
    }
    CsrBtHfHsSendCmDataReq(instData,strLen,dataPtr);
}

/*************************************************************************************
    Send AT+COPS=? (operator name query)
************************************************************************************/
void CsrBtHfAtCopsQuerySend(HfMainInstanceData_t *instData)
{
    CsrUint8         *dataPtr;
    CsrUint16       strLen;

    strLen = (CsrUint16)(CsrStrLen(COPS_QUERY));
    dataPtr = (CsrUint8 *) CsrStrDup(COPS_QUERY);

    instData->linkData[instData->index].data->dataReceivedInConnected = TRUE;
    if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
    {
        instData->linkData[instData->index].lastAtCmdSent = copsQueryCmd;
    }
    CsrBtHfHsSendCmDataReq(instData,strLen,dataPtr);
}

/*************************************************************************************
    Send AT+CLCC (call list)
************************************************************************************/
void CsrBtHfAtClccSend(HfMainInstanceData_t *instData)
{
    CsrUint8         *dataPtr;
    CsrUint16       strLen;

    strLen = (CsrUint16)(CsrStrLen(CALL_LIST_QUERY));
    dataPtr = (CsrUint8 *) CsrStrDup(CALL_LIST_QUERY);
    if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
    {
        instData->linkData[instData->index].lastAtCmdSent = clcc;
    }
    CsrBtHfHsSendCmDataReq(instData,strLen,dataPtr);
}

/*************************************************************************************
    Send AT+CCWA (call waiting)
************************************************************************************/
void CsrBtHfAtCcwaSend(HfMainInstanceData_t *instData, CsrBool enable)
{
    CsrUint8         *dataPtr;
    CsrUint16       strLen;

    strLen = (CsrUint16)(CsrStrLen(CALL_WAITING_CMD));
    dataPtr = (CsrUint8 *) CsrStrDup(CALL_WAITING_CMD);
    if (enable)
    {
        *(dataPtr + CCWA_VALUE_INDEX) = '1';
    }
    else
    {
        *(dataPtr + CCWA_VALUE_INDEX) = '0';
    }
    if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
    {
        instData->linkData[instData->index].lastAtCmdSent = ccwa;
    }
    CsrBtHfHsSendCmDataReq(instData,strLen,dataPtr);
}


/*************************************************************************************
    Send AT+CLIP (calling line identification)
************************************************************************************/
void CsrBtHfAtClipSend(HfMainInstanceData_t *instData, CsrBool enable)
{
    CsrUint8         *dataPtr;
    CsrUint16       strLen;

    strLen = (CsrUint16)(CsrStrLen(CLIP_COMMAND));
    dataPtr = (CsrUint8 *) CsrStrDup(CLIP_COMMAND);
    if (enable)
    {
        *(dataPtr + CLIP_VALUE_INDEX) = '1';
    }
    else
    {
        *(dataPtr + CLIP_VALUE_INDEX) = '0';
    }
    if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
    {
        instData->linkData[instData->index].lastAtCmdSent = clip;
    }
    CsrBtHfHsSendCmDataReq(instData,strLen,dataPtr);
}

/*************************************************************************************
    Send AT+NREC (noise reduction and echo cancellation)
************************************************************************************/
void CsrBtHfAtNrecSend(HfMainInstanceData_t *instData, CsrBool enable)
{
    CsrUint8         *dataPtr;
    CsrUint16       strLen;

    strLen = (CsrUint16)(CsrStrLen(ECHO_NOISE_REDUCTION_CMD));
    dataPtr = (CsrUint8 *) CsrStrDup(ECHO_NOISE_REDUCTION_CMD);
    if (enable)
    {
        *(dataPtr + NREC_VALUE_INDEX) = '1';
    }
    else
    {
        *(dataPtr + NREC_VALUE_INDEX) = '0';
    }
    if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
    {
        instData->linkData[instData->index].lastAtCmdSent = nrec;
    }
    CsrBtHfHsSendCmDataReq(instData,strLen,dataPtr);
}


/*************************************************************************************
    Send AT+BVRA (voice recognition)
************************************************************************************/
void CsrBtHfAtBvraSend(HfMainInstanceData_t *instData, CsrUint8 value)
{
    CsrUint8         *dataPtr;
    CsrUint16       strLen;

    strLen = (CsrUint16)(CsrStrLen(VOICE_RECOGNITION_CMD));
    dataPtr = (CsrUint8 *) CsrStrDup(VOICE_RECOGNITION_CMD);

    *(dataPtr + BVRA_VALUE_INDEX) = value + '0';

    if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
    {
        instData->linkData[instData->index].lastAtCmdSent = bvra;
    }
    CsrBtHfHsSendCmDataReq(instData,strLen,dataPtr);
}


/*************************************************************************************
    Send AT+VTS (generate DTMF)
************************************************************************************/
void CsrBtHfAtVtsSend(HfMainInstanceData_t *instData, CsrUint8 dtmf)
{
    CsrUint8         *dataPtr;
    CsrUint16       strLen;

    strLen = (CsrUint16)(CsrStrLen(DTMF_SEND_CMD));
    dataPtr = (CsrUint8 *) CsrStrDup(DTMF_SEND_CMD);

    *(dataPtr + DTMF_VALUE_INDEX) = dtmf;

    if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
    {
        instData->linkData[instData->index].lastAtCmdSent = vts;
    }
    CsrBtHfHsSendCmDataReq(instData,strLen,dataPtr);
}


/*************************************************************************************
    Send AT+CNUM (subscription number)
************************************************************************************/
void CsrBtHfAtCnumSend(HfMainInstanceData_t *instData)
{
    CsrUint8         *dataPtr;
    CsrUint16       strLen;

    strLen = (CsrUint16)(CsrStrLen(SUBSCRIBER_QUERY));
    dataPtr = (CsrUint8 *) CsrStrDup(SUBSCRIBER_QUERY);
    if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
    {
        instData->linkData[instData->index].lastAtCmdSent = cnum;
    }
    CsrBtHfHsSendCmDataReq(instData,strLen,dataPtr);
}

/*************************************************************************************
    Send AT+CMEE (extended error)
************************************************************************************/
void CsrBtHfAtCmeeSetCommandSend(HfMainInstanceData_t *instData,CsrBool enable)
{
    CsrUint8         *dataPtr;
    CsrUint16       strLen;

    strLen = (CsrUint16)(CsrStrLen(EXTENDED_ERROR));
    dataPtr = (CsrUint8 *) CsrStrDup(EXTENDED_ERROR);
    if (enable)
    {
        *(dataPtr + EXTENDED_ERROR_INDEX) = '1';
    }
    else
    {
        *(dataPtr + EXTENDED_ERROR_INDEX) = '0';
    }
    if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
    {
        instData->linkData[instData->index].lastAtCmdSent = cmee;
    }
    CsrBtHfHsSendCmDataReq(instData,strLen,dataPtr);
}

/*************************************************************************************
    Send AT+CIND (indicators status query)
************************************************************************************/
void CsrBtHfAtStatusIndValueSend(HfMainInstanceData_t *instData)
{
    CsrUint8         *dataPtr;
    CsrUint16       strLen;

    strLen = (CsrUint16)(CsrStrLen(CIND_STATUS));
    dataPtr = (CsrUint8 *) CsrStrDup(CIND_STATUS);

    if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
    {
        instData->linkData[instData->index].lastAtCmdSent = cindSupportCmd;
    }
    CsrBtHfHsSendCmDataReq(instData,strLen,dataPtr);
}

/*************************************************************************************
    Send AT+BINP (BT input: associate a phone book entry to a voice tag)
************************************************************************************/
void CsrBtHfAtBinpSend(HfMainInstanceData_t *instData, CsrUint32 dataRequest)
{
    CsrUint8         *dataPtr;
    CsrUint16       strLen;

    strLen = (CsrUint16)(CsrStrLen(BT_INPUT_CMD));
    dataPtr = (CsrUint8 *) CsrStrDup(BT_INPUT_CMD);

    *(dataPtr + BINP_VALUE_INDEX) = (CsrUint8)dataRequest + '0'; /* Value in ASCII: only value '1' allowed so far! */

    if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
    {
        instData->linkData[instData->index].lastAtCmdSent = binp;
    }
    CsrBtHfHsSendCmDataReq(instData,strLen,dataPtr);
}

/*************************************************************************************
    Send ATD'number', ATD>'memory index' or AT+BLDN (dial number, dial number at
    memory index, or redial, respectively)
************************************************************************************/
void CsrBtHfAtDialSend(HfMainInstanceData_t *instData, CsrBtHfDialCommand  command, CsrCharString *number)
{
    CsrUint8         *dataPtr;
    CsrUint16       strLen;

    switch(command)
    {
        case CSR_BT_HF_DIAL_NUMBER:
        case CSR_BT_HF_DIAL_MEMORY:
        {
            CsrUint8  index = DIAL_CMD_INDEX;

            strLen = (CsrUint16)(DIAL_CMD_INDEX + CsrStrLen((char*)number));

            if (command == CSR_BT_HF_DIAL_MEMORY)
            {
                strLen++;
                index++;
            }
            dataPtr = CsrPmemAlloc(strLen);
            if (command == CSR_BT_HF_DIAL_NUMBER)
            {
                CsrStrNCpyZero((char*) dataPtr, DIAL_CMD, strLen);
            }
            else
            {
                CsrStrNCpyZero((char*) dataPtr, DIAL_MEM_CMD, strLen);
            }
            if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
            {
                SynMemCpyS(&dataPtr[index], strLen - index, (CsrUint8 *)number,CsrStrLen((char*)number));
                if (command == CSR_BT_HF_DIAL_NUMBER)
                {
                    instData->linkData[instData->index].lastAtCmdSent = dialNumber;
                }
                else
                {
                   instData->linkData[instData->index].lastAtCmdSent = dialMem;
                }
            }
            break;
        }
        default:
        {/* This can only be redial.... */
            strLen = (CsrUint16)(CsrStrLen(REDIAL_CMD));
            dataPtr = (CsrUint8 *) CsrStrDup(REDIAL_CMD);
            if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
            {
                instData->linkData[instData->index].lastAtCmdSent = redial;
            }
            break;
        }
    }

    CsrBtHfHsSendCmDataReq(instData,strLen,dataPtr);

    CsrPmemFree(number);
}

HfInstanceData_t *CsrBtHfGetInstFromBdAddr(HfMainInstanceData_t *hfMainInst,
                                     const CsrBtDeviceAddr *addr)
{
    CsrUint8 i;
    CsrUint8 count = hfMainInst->maxHSConnections + hfMainInst->maxHFConnections;

    for (i = 0; i < count; i++)
    {
        HfInstanceData_t *linkPtr = (HfInstanceData_t*) &(hfMainInst->linkData[i]);

        if (CsrBtBdAddrEq(&linkPtr->currentDeviceAddress, addr))
        {
            return linkPtr;
        }
    }

    CSR_UNUSED(hfMainInst);
    return NULL;
}

HfInstanceData_t *CsrBtHfGetConnectedInstFromBdAddr(HfMainInstanceData_t *hfMainInst,
                                     const CsrBtDeviceAddr *addr)
{
    CsrUint8 i;
    CsrUint8 count = hfMainInst->maxHSConnections + hfMainInst->maxHFConnections;

    for (i = 0; i < count; i++)
    {
        HfInstanceData_t *linkPtr = (HfInstanceData_t*) &(hfMainInst->linkData[i]);

        if (CsrBtBdAddrEq(&linkPtr->currentDeviceAddress, addr) &&
            linkPtr->state == Connected_s)
        {
            return linkPtr;
        }
    }

    CSR_UNUSED(hfMainInst);
    return NULL;
}

CsrBool CsrBtHfSetCurrentConnIndexFromBdAddr(HfMainInstanceData_t * instData, CsrBtDeviceAddr deviceAddr)
{
    CsrIntFast8 i = 0;
    HfInstanceData_t *linkPtr;
    CsrBool found = FALSE;

    for (i=0; i < (instData->maxHFConnections + instData->maxHSConnections); i++)
    {
        linkPtr = (HfInstanceData_t *) &(instData->linkData[i]);

        if ((linkPtr->currentDeviceAddress.lap == deviceAddr.lap) &&
            (linkPtr->currentDeviceAddress.uap == deviceAddr.uap) &&
            (linkPtr->currentDeviceAddress.nap == deviceAddr.nap))
        {
            instData->index = i;
            found = TRUE;
            break;
        }
    }

    return found;
}

CsrBool CsrBtHfSetCurrentConnIndexFromBdAddrSdc(HfMainInstanceData_t *instData,
                                                CsrBtDeviceAddr deviceAddr)
{
    CsrUint8 i;
    HfInstanceData_t *linkPtr;
    CsrBool found = FALSE;

    for (i = 0; i < instData->maxHFConnections + instData->maxHSConnections; i++)
    {
        linkPtr = (HfInstanceData_t *) &instData->linkData[i];

        if (CsrBtBdAddrEq(&linkPtr->currentDeviceAddress, &deviceAddr))
        {
            if (linkPtr->hfConnId != CSR_BT_CONN_ID_INVALID &&
                linkPtr->state == ServiceSearch_s)
            {
                instData->index = i;
                found = TRUE;
                break;
            }
        }
    }

    return found;
}

CsrBool CsrBtHfSetCurrentConnIndexFromConnId(HfMainInstanceData_t * instData, CsrBtHfConnectionId connId)
{
    CsrIntFast8 i = 0;
    HfInstanceData_t *linkPtr;
    CsrBool found = FALSE;


    for (i=0; i < (instData->maxHFConnections + instData->maxHSConnections); i++)
    {
        linkPtr = (HfInstanceData_t *) &(instData->linkData[i]);
        if (linkPtr->hfConnId == connId)
        {
            instData->index = i;
            found = TRUE;
            break;
        }
    }
    return found;
}

CsrBool CsrBtHfSetCurrentConnIndexFromInstId(HfMainInstanceData_t * instData, CsrUint8 instId)
{
    CsrIntFast8 i = 0;
    HfInstanceData_t *linkPtr;
    CsrBool found = FALSE;

    for (i=0; i < (instData->maxHFConnections + instData->maxHSConnections); i++)
    {
        linkPtr = (HfInstanceData_t *) &(instData->linkData[i]);
        if (linkPtr->instId == instId)
        {
            instData->index = i;
            found = TRUE;
            break;
        }
    }
    return found;
}

CsrBtConnId CsrBtHfGetBtConnIdFromInstId(void *inst,
                                         CsrUint32 instId)
{
    HfMainInstanceData_t *instData = (HfMainInstanceData_t *) inst;
    HfInstanceData_t *linkPtr;
    CsrBtConnId connId = CSR_BT_CONN_ID_INVALID;
    CsrIntFast8 i = 0;

    for (i=0; i < (instData->maxHFConnections + instData->maxHSConnections); i++)
    {
        linkPtr = (HfInstanceData_t *) &(instData->linkData[i]);
        if (linkPtr->instId == (CsrUint8)instId)
        {
            connId = linkPtr->hfConnId;
        }
    }
    return connId;
}

void sendHfSupportedHfInd(HfMainInstanceData_t * instData)
{
    CsrUint8         *dataPtr;
    CsrUint16       strLen;
    CsrBtHfpHfIndicatorId *localHfInd;
    CsrUint16    i, count;

    localHfInd = instData->localHfIndicatorList;
    count = instData->indCount;

    strLen = (CsrUint16)(CsrStrLen(BIND_SET_CMD));
    dataPtr = (CsrUint8 *)CsrPmemZalloc(strLen);
    SynMemCpyS(dataPtr, strLen, BIND_SET_CMD, strLen);

    i = 0;
    do
    {
        CsrCharString *tempStr;

        tempStr = (CsrCharString *)CsrPmemZalloc(I2B10_MAX);
        CsrIntToBase10(localHfInd[i], tempStr);
        appendString((CsrCharString **) &dataPtr, &strLen, &tempStr);

        i++;

        if(i < count)
        {
            dataPtr[strLen - 1] = ',';
        }
        else
        {
            dataPtr[strLen - 1] = '\r';
        }

    }while (i < count);

    if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
    {
        instData->linkData[instData->index].lastAtCmdSent = bindSet;
    }
    CsrBtHfHsSendCmDataReq(instData,strLen,dataPtr);
}

void queryAgSupportedHfInd(HfMainInstanceData_t * instData)
{
    CsrUint8        *dataPtr;
    CsrUint16       strLen;

    strLen = (CsrUint16)(CsrStrLen(BIND_TEST_CMD));
    dataPtr = (CsrUint8 *)CsrPmemZalloc(strLen);
    SynMemCpyS(dataPtr, strLen, BIND_TEST_CMD, strLen);

    if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
    {
        instData->linkData[instData->index].lastAtCmdSent = bindTest;
    }
    CsrBtHfHsSendCmDataReq(instData,strLen,dataPtr);
}

void queryAgEnabledHfInd(HfMainInstanceData_t * instData)
{
    CsrUint8         *dataPtr;
    CsrUint16       strLen;

    strLen = (CsrUint16)(CsrStrLen(BIND_READ_CMD));
    dataPtr = (CsrUint8 *)CsrPmemZalloc(strLen);
    SynMemCpyS(dataPtr, strLen, BIND_READ_CMD, strLen);


    if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
    {
        instData->linkData[instData->index].lastAtCmdSent = bindRead;
    }
    CsrBtHfHsSendCmDataReq(instData,strLen,dataPtr);
}

static CsrBool HfMessagePopNextValidMessage(HfMainInstanceData_t *instData, CsrUint16 *event, void **message)
{
    HfInstanceData_t *linkPtr;
    CsrBool foundValidMsg = FALSE;

    linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);
    while((foundValidMsg == FALSE) && CsrMessageQueuePop(&linkPtr->data->cmDataReqQueue, event, message))
    {
        if (*event == CSR_BT_HF_PRIM && *(CsrBtHfPrim*)(*message) == CSR_BT_HF_SET_HF_INDICATOR_VALUE_REQ)
        {
            CsrBtHfRemoteHfIndicator *hfInd;
            CsrBtHfSetHfIndicatorValueReq *prim = (CsrBtHfSetHfIndicatorValueReq*)(*message);
            hfInd = REMOTE_HF_INDICATOR_GET_FROM_IND_ID((CsrCmnList_t *)(&linkPtr->remoteHfIndicatorList), (CsrUint8)prim->indId);
            if((hfInd != NULL) && (hfInd->indStatus != CSR_BT_HFP_HF_INDICATOR_STATE_DISABLE))
            {
                if(hfInd->validVal && hfInd->indvalue == prim->value)
                {
                    CsrBtHfSendHfGeneralCfmMsg(instData, CSR_BT_CME_SUCCESS, CSR_BT_HF_SET_HF_INDICATOR_VALUE_CFM);
                }
                else
                {
                    foundValidMsg = TRUE;
                }
            }
            else
            {
                CsrBtHfSendHfGeneralCfmMsg(instData, CSR_BT_CME_OPERATION_NOT_ALLOWED, CSR_BT_HF_SET_HF_INDICATOR_VALUE_CFM);
            }
        }
        else
        {
            foundValidMsg = TRUE;
        }
    }

    return (foundValidMsg);
}

void CsrBtHfSendUpdatedHfIndValue(HfMainInstanceData_t *instData, CsrUint16 indicator, CsrUint16 value)
{
    HfInstanceData_t *linkPtr;
    CsrUint16  eventClass;
    void *     msg;
    CsrBtHfRemoteHfIndicator *hfInd;
    CsrBtCmeeResultCode result = CSR_BT_CME_OPERATION_NOT_ALLOWED;

    linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    hfInd = REMOTE_HF_INDICATOR_GET_FROM_IND_ID((CsrCmnList_t *)(&linkPtr->remoteHfIndicatorList), (CsrUint8)indicator);

    if((hfInd != NULL) &&
         (hfInd->indStatus != CSR_BT_HFP_HF_INDICATOR_STATE_DISABLE))
    {
        if(hfInd->validVal && hfInd->indvalue == value)
        {
            result = CSR_BT_CME_SUCCESS;
        }
        else
        {
            CsrUint8         *dataPtr;
            CsrCharString    *tempStr;
            CsrUint16         strLen;

            hfInd->indvalue = value;
            hfInd->validVal = TRUE;

            strLen = (CsrUint16)(CsrStrLen(BIEV_SET_CMD));
            dataPtr = (CsrUint8 *)CsrPmemZalloc(strLen);
            SynMemCpyS(dataPtr, strLen, BIEV_SET_CMD, strLen);

            tempStr = (CsrCharString *)CsrPmemZalloc(I2B10_MAX);
            CsrIntToBase10(indicator, tempStr);
            appendString((CsrCharString **) &dataPtr, &strLen, &tempStr);
            dataPtr[strLen - 1] = ',';

            tempStr = (CsrCharString *)CsrPmemZalloc(I2B10_MAX);
            CsrIntToBase10(value, tempStr);
            appendString((CsrCharString **) &dataPtr, &strLen, &tempStr);
            dataPtr[strLen - 1] = '\r';


            if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
            {
                instData->linkData[instData->index].lastAtCmdSent = bievSet;
            }
            CsrBtHfHsSendCmDataReq(instData,strLen,dataPtr);

            return;
        }
    }

    CsrBtHfSendHfGeneralCfmMsg(instData, result, CSR_BT_HF_SET_HF_INDICATOR_VALUE_CFM);

    /* Send the next command to handle from the Tx Queue */
    if(HfMessagePopNextValidMessage(instData, &eventClass, &msg))
    {
        if (eventClass == CSR_BT_HF_PRIM)
        {/* This is a request to send an AT command */
            linkPtr->data->allowed2SendCmData = TRUE;
            instData->eventClass = eventClass;
            SynergyMessageFree(eventClass, instData->recvMsgP);
            instData->recvMsgP = msg;
            CsrBtHfCommonAtCmdPrimReqHandler(instData,(CsrBtHfPrim *)msg);
        }
        else
        {/* CSR_BT_CM_PRIM */
            CsrBtCmDataReq *cmPrim = msg;
            linkPtr->lastAtCmdSent = FindCurrentCmdFromPayload(cmPrim->payload);
#ifdef CSR_STREAMS_ENABLE
            CsrStreamsDataSend(CM_GET_UINT16ID_FROM_BTCONN_ID(linkPtr->hfConnId),
                               RFCOMM_ID,
                               cmPrim->payloadLength,
                               cmPrim->payload);
            SynergyMessageFree(eventClass, cmPrim);
#else
            CsrSchedMessagePut(CSR_BT_CM_IFACEQUEUE, eventClass, cmPrim);
#endif
        }
    }
}

void appendString(CsrCharString **mainStr, CsrUint16 *mainStrLen, CsrCharString **addStr)
{
        CsrCharString *oldStr, *newStr;
        CsrUint16 oldStrLen, strLen;

        oldStr = *mainStr;
        oldStrLen = *mainStrLen;

        strLen = (CsrUint16)CsrStrLen(*addStr) + oldStrLen +1;
        newStr = (CsrCharString *)CsrPmemZalloc(strLen);

        SynMemCpyS(newStr, strLen, oldStr, oldStrLen);
        newStr[oldStrLen] = '\0';
        CsrStrLCat(newStr, *addStr, strLen);

        *mainStr = newStr;
        *mainStrLen = strLen;

        CsrPmemFree(oldStr);
        CsrPmemFree(*addStr);
}

/*************************************************************************************
    Send a CSR_BT_HF_HF_INDICATOR_STATUS_IND to app
************************************************************************************/
void CsrBtHfSendHfIndicatorStatusInd(HfMainInstanceData_t *instData, CsrUint16 indId, CsrBtHfpHfIndicatorStatus status)
{
    CsrBtHfHfIndicatorStatusInd    *prim;
    HfInstanceData_t *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    prim    = (CsrBtHfHfIndicatorStatusInd *)CsrPmemZalloc(sizeof(CsrBtHfHfIndicatorStatusInd));
    prim->type        = CSR_BT_HF_HF_INDICATOR_STATUS_IND;
    prim->connectionId = linkPtr->hfConnId;
    prim->indId = (CsrBtHfpHfIndicatorId)indId;
    prim->status = status;

    CsrBtHfMessagePut(instData->appHandle, prim);
}
void CsrBtHfSetAddrInvalid(CsrBtDeviceAddr *pktBdAddr)
{
    pktBdAddr->lap = 0xFFFFFF;
    pktBdAddr->uap = 0xFF;
    pktBdAddr->nap = 0xFFFF;
}

/*************************************************************************************
    Check support of codec type to be used and set audio params accordingly;
    If codecToUse is not set either CSR_BT_WBS_MSBC_CODEC or CSR_BT_WBS_CVSD_CODEC
    then set scoParms with default accept audio parameters.
************************************************************************************/
void CsrBtHfSetIncomingScoAudioParams(HfInstanceData_t *linkPtr, CsrBtCmScoCommonParms *scoParms)
{
    /* Set SCO params with default audio accept parameters first */
    scoParms->audioQuality  = CSR_BT_COMPLETE_SCO_DEFAULT_ACCEPT_AUDIO_QUALITY; /* 0x38F */
    scoParms->txBandwidth   = CSR_BT_SCO_DEFAULT_ACCEPT_TX_BANDWIDTH;   /* 8000 */
    scoParms->rxBandwidth   = CSR_BT_SCO_DEFAULT_ACCEPT_RX_BANDWIDTH;   /* 8000 */
    scoParms->maxLatency    = CSR_BT_SCO_DEFAULT_ACCEPT_MAX_LATENCY;    /* 0xFF */
    scoParms->voiceSettings = CSR_BT_SCO_DEFAULT_ACCEPT_VOICE_SETTINGS; /* CVSD */
    scoParms->reTxEffort    = CSR_BT_SCO_DEFAULT_ACCEPT_RE_TX_EFFORT;   /* 0xFF */

#ifdef CSR_BT_HF_ENABLE_SWB_SUPPORT
    if (linkPtr->hfQceCodecId != CSR_BT_HF_QCE_UNSUPPORTED)
    {
        scoParms->audioQuality = CSR_BT_HF_QCE_DEFAULT_AUDIO_QUALITY;

        /* If the QCE codec is selected, then set the parameters accoring to the selected codec */
        switch (linkPtr->hfQceCodecId)
        {
            case CSR_BT_HF_QCE_CODEC_TYPE_64_2_EV3:
            case CSR_BT_HF_QCE_CODEC_TYPE_64_2_EV3_QHS3:
            {
                scoParms->txBandwidth   = CSR_BT_SCO_DEFAULT_ACCEPT_TX_BANDWIDTH;
                scoParms->rxBandwidth   = CSR_BT_SCO_DEFAULT_ACCEPT_RX_BANDWIDTH;
                scoParms->maxLatency    = CSR_BT_HF_QCE_Q0_MAX_LATENCY;
                scoParms->voiceSettings = CSR_BT_HF_QCE_QO_VOICE_SETTINGS;
                scoParms->reTxEffort    = CSR_BT_QCE_Q0_RETX_EFFORT;
            }
            break;

            case CSR_BT_HF_QCE_CODEC_TYPE_128_QHS3:
            {
                scoParms->txBandwidth   = CSR_BT_HF_QCE_Q1_RX_TX_BANDWIDTH;
                scoParms->rxBandwidth   = CSR_BT_HF_QCE_Q1_RX_TX_BANDWIDTH;
                scoParms->maxLatency    = CSR_BT_HF_QCE_Q1_MAX_LATENCY;
                scoParms->voiceSettings = CSR_BT_HF_QCE_Q1_VOICE_SETTINGS;
                scoParms->reTxEffort    = CSR_BT_QCE_Q1_RETX_EFFORT;
            }
            break;

            case CSR_BT_HF_QCE_CODEC_TYPE_64_QHS3:
            {
                scoParms->txBandwidth   = CSR_BT_SCO_DEFAULT_ACCEPT_TX_BANDWIDTH;
                scoParms->rxBandwidth   = CSR_BT_SCO_DEFAULT_ACCEPT_RX_BANDWIDTH;
                scoParms->maxLatency    = CSR_BT_HF_QCE_Q2_MAX_LATENCY;
                scoParms->voiceSettings = CSR_BT_HF_QCE_Q2_VOICE_SETTINGS;
                scoParms->reTxEffort    = CSR_BT_QCE_Q2_RETX_EFFORT;
            }
            break;
        }
    }
    else
#endif /* CSR_BT_HF_ENABLE_SWB_SUPPORT */
    {
        /* Set SCO params as per codec to use if codec negotiation is supported and done */
        if ((linkPtr->supportedFeatures & CSR_BT_HFG_SUPPORT_CODEC_NEGOTIATION) &&
            (CSR_BT_HF_MAIN_INSTANCE_GET(linkPtr)->localSupportedFeatures & CSR_BT_HF_SUPPORT_CODEC_NEGOTIATION) &&
            (linkPtr->codecToUse != CSR_BT_WBS_INVALID_CODEC))
        {
            if (linkPtr->codecToUse == CSR_BT_WBS_MSBC_CODEC
                || linkPtr->codecToUse == CSR_BT_WBS_LC3SWB_CODEC)
            {
                /* mSBC or LC3: transparent data; only 2-EV3 OR EV3 to be used; tx-rx-bdwidth = 8000
                max Latency = 13ms; retransmission effort = 0x02 */
                scoParms->voiceSettings |= CSR_BT_AIRCODING_TRANSPARENT_DATA;
                scoParms->audioQuality   = CSR_BT_HF_MSBC_DEFAULT_AUDIO_QUALITY; /* 0x0388 */
                scoParms->maxLatency     = CSR_BT_HF_MSBC_MAX_LATENCY;           /* 0x000D */
                scoParms->reTxEffort     = CSR_BT_HF_MSBC_DEFAULT_RE_TX_EFFORT;  /* 0x02   */
            }
            else if (linkPtr->codecToUse == CSR_BT_WBS_CVSD_CODEC)
            {/* CVSD; only 2-EV3 OR EV3 OR HV3 OR HV1 to be used; tx-rx-bdwidth = 8000
                max Latency = 12ms; retransmission effort = Don't care */
                scoParms->audioQuality   = CSR_BT_HF_CVSD_DEFAULT_AUDIO_QUALITY; /* 0x038D */
                scoParms->maxLatency     = CSR_BT_HF_CVSD_MAX_LATENCY;           /* 0x000C */
            }
        }
    }
}

static CsrUint8 csrGetCindIndex(CsrUint8 *str)
{
    CsrUint8 index = CIND_INDEX_INVALID;

    if(!CsrMemCmp(str, CIND_SERVICE, CsrStrLen(CIND_SERVICE)))
    {
        index = CIND_INDEX_SERVICE;
    }
    else if(!CsrMemCmp(str, CIND_CALL, CsrStrLen(CIND_CALL)))
    {
        index = CIND_INDEX_CALL;
    }
    else if(!CsrMemCmp(str, CIND_CALLSETUP, CsrStrLen(CIND_CALLSETUP)))
    {
        index = CIND_INDEX_CALLSETUP;
    }
    else if(!CsrMemCmp(str, CIND_CALLHELD, CsrStrLen(CIND_CALLHELD)))
    {
        index = CIND_INDEX_CALLHELD;
    }
    else if(!CsrMemCmp(str, CIND_SIGNAL, CsrStrLen(CIND_SIGNAL)))
    {
        index = CIND_INDEX_SIGNAL;
    }
    else if(!CsrMemCmp(str, CIND_ROAM, CsrStrLen(CIND_ROAM)))
    {
        index = CIND_INDEX_ROAM;
    }
    else if(!CsrMemCmp(str, CIND_BATTCHG, CsrStrLen(CIND_BATTCHG)))
    {
        index = CIND_INDEX_BATTCHG;
    }
    else if(!CsrMemCmp(str, CIND_SERVICE, CsrStrLen(CIND_SERVICE)))
    {
        index = CIND_INDEX_SERVICE;
    }
    else if(!CsrMemCmp(str, CIND_SERVICE, CsrStrLen(CIND_SERVICE)))
    {
        index = CIND_INDEX_SERVICE;
    }

    return index;
}

static CsrUint8 csrBtHfGetCindDataInstances(CsrUint8 *cindString, CsrUint32 cindLen)
{
    CsrUint16 idx;
    CsrUint8  instCount = 0;

    for(idx=0; idx < cindLen; idx++)
    {
        if(cindString[idx] == '(')
        {
            instCount++;
        }
    }

    instCount /= 2;

    return instCount;
}

void CsrBtHfEncodeCindString(CsrUint8 *cindString, CsrUint32 cindLen, hfAgCindSupportInd_t *cindData)
{
    CsrUint8    idx, tempIdx, cindIndex;
    CsrBool     stopEncoding = FALSE;
    CsrUint16   strIdx = 0;
    CsrUint8    tempStr[CIND_MAX_STRING_MAX_LEN];
    CsrCharString   *comma = ",";

    if(cindString && cindData)
    {
        cindData->instCount = csrBtHfGetCindDataInstances(cindString, cindLen);

        if(cindData->instCount > 0)
        {
            for(idx = 0; idx < cindData->instCount; idx++)
            {
                tempIdx = 0;
                CsrMemSet(tempStr, 0, sizeof(tempStr));

                while(strIdx < cindLen)
                {
                    if(cindString[strIdx] == (CsrUint8)comma[0])
                    {
                        cindData->cindEncodedData[idx].index = csrGetCindIndex(tempStr);
                        stopEncoding = cindData->cindEncodedData[idx].index == CIND_INDEX_INVALID;
                        cindData->cindEncodedData[idx].range_start = cindString[strIdx + 2];
                        cindData->cindEncodedData[idx].range_end = cindString[strIdx + 4];
                        strIdx += CIND_FIXED_LEN;
                        break;
                    }
                    else if (tempIdx >= CIND_MAX_STRING_MAX_LEN)
                    {
                        stopEncoding = TRUE;
                        break;
                    }
                    else
                    {
                        tempStr[tempIdx++] = cindString[strIdx];
                    }

                    strIdx++;
                }

                if(stopEncoding)
                {
                    break;
                }
            }
        }
    }
    CSR_UNUSED(cindIndex);
}

static CsrUint8 csrGetCindStringLen(hfAgCindSupportInd_t *cind)
{
    CsrUint8 len = 0, idx;

    for(idx = 0; idx < cind->instCount; idx++)
    {
        switch(cind->cindEncodedData[idx].index)
        {
            case CIND_INDEX_SERVICE:
            {
               len += (CsrUint8)CsrStrLen(CIND_SERVICE);
            }
            break;
            case CIND_INDEX_CALL:
            {
                len += (CsrUint8)CsrStrLen(CIND_CALL);
            }
            break;
            case CIND_INDEX_CALLSETUP:
            {
                len += (CsrUint8)CsrStrLen(CIND_CALLSETUP);
            }
            break;
            case CIND_INDEX_CALLHELD:
            {
                len += (CsrUint8)CsrStrLen(CIND_CALLHELD);
            }
            break;
            case CIND_INDEX_SIGNAL:
            {
                len += (CsrUint8)CsrStrLen(CIND_SIGNAL);
            }
            break;
            case CIND_INDEX_ROAM:
            {
                len += (CsrUint8)CsrStrLen(CIND_ROAM);
            }
            break;
            case CIND_INDEX_BATTCHG:
            {
                len += (CsrUint8)CsrStrLen(CIND_BATTCHG);
            }
            break;
        }
    }

    len += (cind->instCount * CIND_FIXED_LEN);

    return (len - 1);
}

static void csrCindStringAppend(CsrUint8 *str, CsrUint8 *idx, const CsrUint8 *append_str)
{
    CsrUint8 srcIdx, srcLen;

    if(str && append_str && idx)
    {
        srcIdx = 0;
        srcLen = (CsrUint8)CsrStrLen((const char *)append_str);

        while(srcIdx < srcLen)
        {
            str[*idx] = append_str[srcIdx];
            (*idx)++;
            srcIdx++;
        }
    }
}

CsrUint8 * CsrBtHfDecodeCindString(hfAgCindSupportInd_t *cindData)
{
    CsrUint8 idx, cindStringLen, strIdx = 0;
    CsrUint8 *cindString = NULL;

    if(cindData)
    {
        cindStringLen = csrGetCindStringLen(cindData);

        if(cindStringLen > 0)
        {
            cindString = CsrPmemAlloc(cindStringLen);

            for(idx = 0; idx < cindData->instCount; idx++)
            {
                switch(cindData->cindEncodedData[idx].index)
                {
                    case CIND_INDEX_SERVICE:
                    {
                        csrCindStringAppend(cindString, &strIdx, (const CsrUint8 *)CIND_SERVICE);
                    }
                    break;
                    case CIND_INDEX_CALL:
                    {
                        csrCindStringAppend(cindString, &strIdx, (const CsrUint8 *)CIND_CALL);
                    }
                    break;
                    case CIND_INDEX_CALLSETUP:
                    {
                        csrCindStringAppend(cindString, &strIdx, (const CsrUint8 *)CIND_CALLSETUP);
                    }
                    break;
                    case CIND_INDEX_CALLHELD:
                    {
                        csrCindStringAppend(cindString, &strIdx, (const CsrUint8 *)CIND_CALLHELD);
                    }
                    break;
                    case CIND_INDEX_SIGNAL:
                    {
                        csrCindStringAppend(cindString, &strIdx, (const CsrUint8 *)CIND_SIGNAL);
                    }
                    break;
                    case CIND_INDEX_ROAM:
                    {
                        csrCindStringAppend(cindString, &strIdx, (const CsrUint8 *)CIND_ROAM);
                    }
                    break;
                    case CIND_INDEX_BATTCHG:
                    {
                        csrCindStringAppend(cindString, &strIdx, (const CsrUint8 *)CIND_BATTCHG);
                    }
                    break;
                }

                /* Append range values */
                cindString[strIdx++] = ',';
                cindString[strIdx++] = '(';
                cindString[strIdx++] = (CsrUint8)(cindData->cindEncodedData[idx].range_start + '0');
                cindString[strIdx++] = '-';
                cindString[strIdx++] = (CsrUint8)(cindData->cindEncodedData[idx].range_end + '0');
                cindString[strIdx++] = ')';
                cindString[strIdx++] = ')';

                if(strIdx < cindStringLen)
                {
                    cindString[strIdx++] = ',';
                }
            }
        }
    }

    return cindString;
}

#ifdef CSR_TARGET_PRODUCT_VM
void CsrBtHfSetScoHandle(HfMainInstanceData_t *instData,
                         CsrBtConnId connId,
                         CsrBtDeviceAddr addr,
                         hci_connection_handle_t scoHandle)
{    
    HfInstanceData_t *linkPtr = NULL;

    /* Caller would have provided either a valid ConnID or a valid addr */
    if (connId != CSR_BT_CONN_ID_INVALID)
    {
        if (CsrBtHfSetCurrentConnIndexFromConnId(instData, connId))
        {
           linkPtr = (HfInstanceData_t*) &(instData->linkData[instData->index]);           
        }
    }
    else
    {
        linkPtr = CsrBtHfGetInstFromBdAddr(instData, &addr);
    }

    if (linkPtr)
    {
         linkPtr->scoHandle = scoHandle;
         CsrBtCmUpdateScoHandle(linkPtr->hfConnId, scoHandle);
    }
}
#endif /* CSR_TARGET_PRODUCT_VM */

#ifdef CSR_BT_HF_ENABLE_SWB_SUPPORT
static CsrBtHfQceCodecType csrHfUtilGetCodecId(CsrUint16 codecMask)
{
    CsrBtHfQceCodecType codecType = CSR_BT_HF_QCE_CODEC_TYPE_64_2_EV3;

    switch (codecMask)
    {
        case CSR_BT_HF_QCE_CODEC_MASK_64_2_EV3:
        {
            codecType = CSR_BT_HF_QCE_CODEC_TYPE_64_2_EV3;
        }
        break;

        case CSR_BT_HF_QCE_CODEC_MASK_64_2_EV3_QHS3:
        {
            codecType = CSR_BT_HF_QCE_CODEC_TYPE_64_2_EV3_QHS3;
        }
        break;

        case CSR_BT_HF_QCE_CODEC_MASK_128_QHS3:
        {
            codecType = CSR_BT_HF_QCE_CODEC_TYPE_128_QHS3;
        }
        break;

        case CSR_BT_HF_QCE_CODEC_MASK_64_QHS3:
        {
            codecType = CSR_BT_HF_QCE_CODEC_TYPE_64_QHS3;
        }
        break;
    }

    return codecType;
}

static CsrUint8 csrHfUtilGetNumberOfSupportedQceCodecs(CsrUint16 hfCodecMask)
{
    CsrUint8 numCodecs = 0;

    while (hfCodecMask)
    {
        numCodecs += (hfCodecMask & 0x1);
        hfCodecMask >>= 1;
    }

    return numCodecs;
}

void sendQac(HfMainInstanceData_t * instData)
{
    CsrUint8 *payload;
    CsrUint8 bit, codecModeId, index = 0;
    CsrUint16 hfCodecMask = instData->hfQceCodecMask, bitMask = 0;

    /* +1 for \r, *3 is done for worst case allocation when supported codecs are all in double digits */
    payload = CsrPmemZalloc(QAC_CMD_LENGTH + (csrHfUtilGetNumberOfSupportedQceCodecs(hfCodecMask) * 3) + 1);

    /* Copy AT+QAC= to the payload */
    SynMemCpyS(payload, QAC_CMD_LENGTH, QAC_CMD, QAC_CMD_LENGTH);

    index = QAC_CMD_LENGTH;

    for (bit = 0; bit < 16 && hfCodecMask; bit++)
    {
        bitMask = (1 << bit);

        /* Only report those codecs which are enabled by application to be supported */
        if (!(bitMask & hfCodecMask))
        {
            continue;
        }

        codecModeId = (CsrUint8) csrHfUtilGetCodecId(bitMask);
        index += CsrSnprintf((CsrCharString *) &payload[index],
                             (codecModeId > 9 ? 3: 2),
                             "%d",
                             codecModeId);

        /* Clear the bit from the mask as it is incorporated */
        hfCodecMask ^= bitMask;

        if (hfCodecMask)
        {
            /* The codec mode ids are comma separated */
            payload[index++] = ',';
        }
    }

    payload[index++] = '\r';

    if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
    {
        instData->linkData[instData->index].lastAtCmdSent = qac;
    }

    CsrBtHfHsSendCmDataReq(instData, index, payload);
}

void sendQcs(HfMainInstanceData_t * instData)
{
    CsrUint8 *payload;
    CsrUint8 index;
    HfInstanceData_t *linkPtr = (HfInstanceData_t *) &(instData->linkData[instData->index]);

    payload = CsrPmemZalloc(QCS_CMD_LENGTH + CSR_BT_HF_QCE_UNSUPPORTED_LEN + 1);

    /* Copy AT+%QCS command to the payload */
    SynMemCpyS(payload, QCS_CMD_LENGTH, QCS_CMD, QCS_CMD_LENGTH);

    index = QCS_CMD_INDEX;

    if (linkPtr->hfQceCodecId != CSR_BT_HF_QCE_UNSUPPORTED)
    {
        index += CsrSnprintf((CsrCharString *) &payload[index],
                             (linkPtr->hfQceCodecId > 9 ? 3 : 2) + 1,
                             "%d\r",
                             linkPtr->hfQceCodecId);
    }
    else
    {
        index += CsrSnprintf((CsrCharString *) &payload[index],
                             CSR_BT_HF_QCE_UNSUPPORTED_LEN + 1,
                             "%d\r",
                             CSR_BT_HF_QCE_UNSUPPORTED);
    }

    if (instData->linkData[instData->index].lastAtCmdSent == idleCmd)
    {
        instData->linkData[instData->index].lastAtCmdSent = qcs;
    }

    CsrBtHfHsSendCmDataReq(instData, index, payload);
}
#endif /* CSR_BT_HF_ENABLE_SWB_SUPPORT */

#ifdef CSR_TARGET_PRODUCT_VM
CsrBtResultCode HfUtilGetBdAddrFromConnectionId(HfMainInstanceData_t *instData,
                                                CsrBtHfConnectionId connectionId,
                                                CsrBtDeviceAddr *deviceAddr)
{
    HfInstanceData_t *linkPtr;
    CsrIntFast8 i;

    for (i=0; i < (instData->maxHFConnections + instData->maxHSConnections); i++)
    {
        linkPtr = (HfInstanceData_t *) &instData->linkData[i];

        if (linkPtr->hfConnId == connectionId)
        {
            *deviceAddr = linkPtr->currentDeviceAddress;
            return CSR_BT_RESULT_CODE_HF_SUCCESS;
        }
    }

    return CSR_BT_RESULT_CODE_HF_INVALID_PARAMETER;
}
#endif /* CSR_TARGET_PRODUCT_VM */

