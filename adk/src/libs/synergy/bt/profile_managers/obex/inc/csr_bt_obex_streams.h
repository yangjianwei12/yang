#ifndef CSR_BT_OBEX_STREAMS_H
#define CSR_BT_OBEX_STREAMS_H
/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_profiles.h"
#include "csr_bt_cm_private_lib.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CSR_STREAMS_ENABLE
#include "csr_streams.h"

/* This interface uses the indication/confirmation as a reference to a pointer as the pointer 
 * gets modified by the called function, i.e. the pointer may get freed inside the function based  
 * on whether the message is consumed immediately or queued */ 
typedef void(* CmStreamDataCfmHandler)(void * inst, CsrBtCmDataCfm **cfm);
typedef void(* CmStreamDataIndHandler)(void * inst, CsrBtCmDataInd **cfm);

void CsrBtObexMessageMoreSpaceHandler(void * inst, CmStreamDataCfmHandler hndlr, MessageMoreSpace *mms, CsrUint8 protocol);
void CsrBtObexMessageMoreDataHandler(void * inst, CmStreamDataIndHandler hndlr, MessageMoreData *mmd, CsrUint8 protocol);
void CsrBtObexStreamsRegister(void * inst, CsrBtConnId btConnId, CsrUint8 protocol);


#endif /* CSR_STREAMS_ENABLE */

#ifdef __cplusplus
}
#endif
#endif // CSR_BT_OBEX_STREAMS_H
