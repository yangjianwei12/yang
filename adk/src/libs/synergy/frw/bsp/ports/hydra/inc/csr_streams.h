#ifndef CSR_STREAMS__
#define CSR_STREAMS__

/*****************************************************************************
Copyright (c) 2019-2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

REVISION:      $Revision: #57 $
*****************************************************************************/

#include "csr_types.h"
#include "csr_sched.h"
#include "csr_synergy.h"

#include "platform/csr_hydra_streams.h"

#ifdef __cplusplus
extern "C" {
#endif

CsrBool CsrStreamsRegister(CsrUint16 connId, CsrUint8 protocol, CsrSchedQid qId);
void CsrStreamsUnregister(CsrUint16 connId, CsrUint8 protocol);
void CsrStreamsMessageMoreSpaceHandler(Sink sink);
CsrUint16 CsrStreamsMessageMoreDataHandler(Source source,
                                           CsrUint8 **payload);
CsrBool CsrStreamsDataSend(CsrUint16 connId,
                           CsrUint8 protocol,
                           CsrUint16 payloadLen,
                           CsrUint8 *payload);

void CsrStreamsSourceHandoverPolicyConfigure(CsrUint16 connId,
                                             CsrUint8 protocol,
                                             CsrUint8 handoverPolicy);


#ifdef __cplusplus
}
#endif

#endif /* CSR_STREAMS__ */

