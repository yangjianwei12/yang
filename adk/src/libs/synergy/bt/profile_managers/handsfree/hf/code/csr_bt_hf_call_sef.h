#ifndef CSR_BT_HF_CALL_SEF_H__
#define CSR_BT_HF_CALL_SEF_H__
/******************************************************************************
 Copyright (c) 2008-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/


#include "csr_synergy.h"
#include "csr_bt_hf_main.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Define Hf handlers */
void CsrBtHfConnectedStateHfAtCmdReqHandler(HfMainInstanceData_t *instData);
void CsrBtHfConnectedStateHfChldReqHandler(HfMainInstanceData_t *instData);
void CsrBtHfXStateMapScoPcmIndHandler(HfMainInstanceData_t *instData);
void CsrBtHfConnectedStateHfAudioReqHandler(HfMainInstanceData_t *instData);
void CsrBtHfConnectedStateHfSpeakerGainStatusReqHandler(HfMainInstanceData_t *instData);
void CsrBtHfConnectedStateHfMicGainStatusReqHandler(HfMainInstanceData_t *instData);
void CsrBtHfConnectedStateHfAnswerReqHandler(HfMainInstanceData_t *instData);
void CsrBtHfConnectedStateHfCallEndReqHandler(HfMainInstanceData_t *instData);

/* Define CM handlers */
void HfXStateCmDataIndHandler(HfMainInstanceData_t *instData);
void HfXStateCmDataCfmHandler(HfMainInstanceData_t *instData);
void CsrBtHfConnectedStateCmScoConnectCfmHandler(HfMainInstanceData_t *instData);
void CsrBtHfConnectedStateCmScoDisconnectIndHandler(HfMainInstanceData_t *instData);
void CsrBtHfConnectedStateCmScoAcceptConnectCfmHandler(HfMainInstanceData_t *instData);

#ifdef __cplusplus
}
#endif

#endif

