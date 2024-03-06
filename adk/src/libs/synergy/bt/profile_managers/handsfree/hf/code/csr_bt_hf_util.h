#ifndef CSR_BT_HF_UTIL_H__
#define CSR_BT_HF_UTIL_H__
/******************************************************************************
 Copyright (c) 2008-2023 Qualcomm Technologies International, Ltd.
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
#include "csr_log_text_2.h"
#include "sds_prim.h"
#include "csr_bt_hf_main.h"
#include "csr_bt_hf_prim.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CSR_BT_AT_COMMAND_MAX_LENGTH
#define CSR_BT_AT_COMMAND_MAX_LENGTH    180
#endif

/* Structure to read valid connection ID for different AT data requests from Application*/
typedef struct
{
    CsrBtHfPrim                 type;                           /* Primitive/message identity */
    CsrBtHfConnectionId         connectionId;                   /* Connection indentifier */
} CsrBtHfDataPrim;

void CsrBtHfMessagePut(CsrSchedQid phandle, void *msg);

void CsrBtHfSaveMessage(HfMainInstanceData_t *instData);
void CsrBtHfSaveCmMessage(HfMainInstanceData_t *instData);
void CsrBtHfSendHfHouseCleaning(HfMainInstanceData_t *instData);
void CsrBtHfInitInstanceData(HfInstanceData_t *linkPtr);
void CsrBtHfSaveQueueCleanUp(HfMainInstanceData_t *instData);
void CsrBtHfSendSdsRegisterReq(HfMainInstanceData_t *instData);
void HsSendSdsRegisterReq(HfMainInstanceData_t *instData);

void CsrBtHfSendConfirmMessage(HfMainInstanceData_t *instData, CsrBtHfPrim type);
void CsrBtHfSendHfMicGainInd(HfMainInstanceData_t *instData, CsrUint8 returnValue);
void CsrBtHfSendHfSpeakerGainInd(HfMainInstanceData_t *instData, CsrUint8 returnValue);
void CsrBtHfSendHfRingInd(HfMainInstanceData_t *instData);
void CsrBtHfSendHfServiceConnectInd(HfMainInstanceData_t *instData,
                                    CsrBtResultCode resultCode, CsrBtSupplier resultSupplier);
void CsrBtHfSendHfFailedServiceConnectCfm(HfMainInstanceData_t *instData, CsrBtDeviceAddr deviceAddr,
                                    CsrBtResultCode resultCode, CsrBtSupplier resultSupplier);
void CsrBtHfSendHfAudioDisconnectCfm(HfMainInstanceData_t *instData,
                           CsrUint16 scoHandle,
                           CsrBtResultCode      resultCode,
                           CsrBtSupplier  resultSupplier);
void CsrBtHfSendHfAudioDisconnectInd(HfMainInstanceData_t *instData,
                           CsrUint16 scoHandle,
                           CsrBtReasonCode      reasonCode,
                           CsrBtSupplier  resultSupplier);
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
                           CsrBtHfPrim           primType);
void CsrBtHfSendIndicatorsUpdateCfm(HfMainInstanceData_t *instData, CsrUint16 result);
void CsrBtHfSendHfDisconnectInd(HfMainInstanceData_t *instData,
                                CsrBtResultCode reasonCode, CsrBtSupplier reasonSupplier);

void CsrBtHfSendHfActivateCfm(HfMainInstanceData_t *instData,
                              CsrBtResultCode resultCode, CsrBtSupplier resultSupplier);
void CsrBtHfSendHfDeactivateCfm(HfMainInstanceData_t *instData,
                                CsrBtResultCode resultCode, CsrBtSupplier resultSupplier);
void CsrBtHfSendHfGeneralCfmMsg(HfMainInstanceData_t *instData, CsrUint16 result, CsrBtHfPrim type);

void CsrBtHfSendHfCmeeInd(HfMainInstanceData_t *instData, CsrUint8 *atTextString);
void HfSendHfCopsCfm(HfMainInstanceData_t *instData, CsrUint8 *atTextString);
void CsrBtHfSendHfCallHandlingInd(HfMainInstanceData_t *instData, CsrUint8 *atTextString);

void sendBrsf(HfMainInstanceData_t * instData);
void sendCodecSupport(HfMainInstanceData_t * instData);
void sendCindSupport(HfMainInstanceData_t * instData);
void sendCindStatus(HfMainInstanceData_t * instData);
void sendSetCmer(HfMainInstanceData_t * instData,CsrBool enable);
void sendCallHoldStatus(HfMainInstanceData_t * instData);
void sendHfSupportedHfInd(HfMainInstanceData_t * instData);
void queryAgSupportedHfInd(HfMainInstanceData_t * instData);
void queryAgEnabledHfInd(HfMainInstanceData_t * instData);
void sendCkpd(HfMainInstanceData_t * instData);
void startSdcFeatureSearch(HfMainInstanceData_t * instData, CsrBool outgoing);
void startAtFeatureSearch(HfMainInstanceData_t * instData);
void sendBia(HfMainInstanceData_t * instData);
void CsrBtHfSendCmScoConnectReq(HfInstanceData_t *linkPtr, CsrUint8 default_setting);

void CsrBtHfSendHfInBandRingToneInd(HfMainInstanceData_t *instData, CsrBool returnValue);

void HfBrsfTimeout(CsrUint16 mi, void *mv);
void CsrBtHfAtResponseTimeout(CsrUint16 mi, void *mv);
void CsrBtHfAcceptIncomingSco(HfInstanceData_t *linkPtr);
void CsrBtHfSetIncomingScoAudioParams(HfInstanceData_t *linkPtr, CsrBtCmScoCommonParms *scoParms);

void CsrBtHfAtCopsSetCommandSend(HfMainInstanceData_t *instData,CsrUint8 mode, CsrUint8 format);
void CsrBtHfAtCopsQuerySend(HfMainInstanceData_t *instData);
void CsrBtHfAtClccSend(HfMainInstanceData_t *instData);
void CsrBtHfAtCnumSend(HfMainInstanceData_t *instData);
void CsrBtHfAtCmeeSetCommandSend(HfMainInstanceData_t *instData,CsrBool mode);
void CsrBtHfAtCcwaSend(HfMainInstanceData_t *instData, CsrBool enable);
void CsrBtHfAtClipSend(HfMainInstanceData_t *instData, CsrBool enable);
void CsrBtHfAtNrecSend(HfMainInstanceData_t *instData, CsrBool enable);
void CsrBtHfAtBvraSend(HfMainInstanceData_t *instData, CsrUint8 value);
void CsrBtHfAtVtsSend(HfMainInstanceData_t *instData, CsrUint8 dtmf);
void CsrBtHfAtStatusIndValueSend(HfMainInstanceData_t *instData);
void CsrBtHfAtBinpSend(HfMainInstanceData_t *instData, CsrUint32 dataRequest);
void CsrBtHfAtDialSend(HfMainInstanceData_t *instData, CsrBtHfDialCommand  command, CsrCharString *number);
void CsrBtHfSendAtBcs(HfMainInstanceData_t *instData);
void CsrBtHfSendAtBcc(HfMainInstanceData_t *instData);
void CsrBtHfSendUpdatedHfIndValue(HfMainInstanceData_t *instData, CsrUint16 indicator, CsrUint16 value);

CsrUint8 CsrBtHfGetNumberOfRecordsInUse(HfMainInstanceData_t *instData,CsrUint8 *nrActiveHf,CsrUint8 *nrActiveHs);
void CsrBtHfCancelAcceptCheck(HfMainInstanceData_t *instData);
void CsrBtHfAllowConnectCheck(HfMainInstanceData_t *instData);
void CsrBtHfCancelAcceptOnConnectingChannel(HfInstanceData_t *linkPtr);

void CsrBtHfSendUpdateCodecSupportedCfm(HfMainInstanceData_t *instData);
void HfSendUpdateOptionalCodecCfm(HfMainInstanceData_t *instData, CsrBtResultCode resultCode);

void CsrBtHfSendSelectedCodecInd(HfMainInstanceData_t *instData);
CsrBool HfIsNewConnectionAllowed(HfMainInstanceData_t *instData, CsrUint8 lServerChannel);

HfInstanceData_t *CsrBtHfGetInstFromBdAddr(HfMainInstanceData_t *hfMainInst,
                                     const CsrBtDeviceAddr *addr);
HfInstanceData_t *CsrBtHfGetConnectedInstFromBdAddr(HfMainInstanceData_t *hfMainInst,
                                     const CsrBtDeviceAddr *addr);

CsrBool CsrBtHfSetCurrentConnIndexFromBdAddr(HfMainInstanceData_t * instData, CsrBtDeviceAddr deviceAddr);
CsrBool CsrBtHfSetCurrentConnIndexFromBdAddrSdc(HfMainInstanceData_t * instData,
                                                CsrBtDeviceAddr deviceAddr);
CsrBool CsrBtHfSetCurrentConnIndexFromConnId(HfMainInstanceData_t * instData, CsrBtHfConnectionId connId);
CsrBool CsrBtHfSetCurrentConnIndexFromInstId(HfMainInstanceData_t * instData, CsrUint8 instId);
CsrBtConnId CsrBtHfGetBtConnIdFromInstId(void *inst,
                                         CsrUint32 instId);

void CsrBtHfSetAddrInvalid(CsrBtDeviceAddr *pktBdAddr);
void CsrBtHfSendHfIndicatorStatusInd(HfMainInstanceData_t *instData, CsrUint16 indId, CsrBtHfpHfIndicatorStatus status);

#ifdef CSR_TARGET_PRODUCT_VM
/* Caller can provide either a valid Connection ID or a valid Device Address. */
void CsrBtHfSetScoHandle(HfMainInstanceData_t * instData, CsrBtConnId connId, CsrBtDeviceAddr addr, hci_connection_handle_t scoHandle);

CsrBtResultCode HfUtilGetBdAddrFromConnectionId(HfMainInstanceData_t *instData,
                                                CsrBtHfConnectionId connectionId,
                                                CsrBtDeviceAddr *deviceAddr);
#endif /* CSR_TARGET_PRODUCT_VM */

#ifdef CSR_BT_HF_ENABLE_SWB_SUPPORT
void sendQac(HfMainInstanceData_t *instData);
void sendQcs(HfMainInstanceData_t *instData);
#endif /* CSR_BT_HF_ENABLE_SWB_SUPPORT */

void CsrBtHfEncodeCindString(CsrUint8 *cindString, CsrUint32 cindLen, hfAgCindSupportInd_t *cindData);
CsrUint8 * CsrBtHfDecodeCindString(hfAgCindSupportInd_t *cindData);

#ifdef __cplusplus
}
#endif

#endif
