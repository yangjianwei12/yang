#ifndef CSR_LOG_GSP_H__
#define CSR_LOG_GSP_H__

#include "csr_synergy.h"
/*****************************************************************************

            Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.


            All Rights Reserved. 
            
*****************************************************************************/

#include "csr_types.h"
#include "csr_log.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CSR_LOG_ENABLE
#define CSR_LOG_GSP

#if defined(CSR_TARGET_PRODUCT_VM) 
#undef CSR_LOG_GSP
#endif

#endif

#define BTSS_ENH_LOG_CONN_HANDLE (0x2EDC)

/*---------------------------------*/
/*  Non scheduler logging */
/*---------------------------------*/
void CsrLogBci(CsrUint8 channel,
    CsrBool received,
    CsrSize payload_length,
    const void *payload);
#if defined(CSR_LOG_GSP)
#define CSR_LOG_BCI(channel, received, payload_length, payload) \
    do { \
        CsrLogBci((channel), (received), (payload_length), (payload)); \
    } while (0)
#else
#define CSR_LOG_BCI(channel, received, payload_length, payload)
#endif


#define CSR_LOG_TRANSPORT_INCOMING  0
#define CSR_LOG_TRANSPORT_OUTGOING  1

void CsrLogTransport(
    CsrUint8 transport_type,
    CsrUint8 direction,
    CsrSize consumed_length,
    CsrSize payload_length,
    const void *payload);
#if defined(CSR_LOG_GSP)
#define CSR_LOG_TRANSPORT(transport_type, direction, consumed_length, \
                          payload_length, payload) \
    do { \
        if (!CsrLogEnvironmentIsFiltered(CSR_LOG_LEVEL_ENVIRONMENT_TRANSPORTS)) \
        { \
            CsrLogTransport((transport_type), (direction), (consumed_length), \
                (payload_length), (payload)); \
        } \
    } while (0)
#else
#define CSR_LOG_TRANSPORT(transport_type, direction, consumed_length, \
                          payload_length, payload)
#endif

void CsrLogMessageQueuePush(CsrUint16 prim_type,
    const void *ptr);
#if defined(CSR_LOG_GSP)
#define CSR_LOG_MESSAGE_QUEUE_PUSH(prim_type, ptr) \
    do { \
        if (!CsrLogTaskIsFiltered(CsrSchedTaskQueueGet(), \
                CSR_LOG_LEVEL_TASK_MESSAGE_QUEUE_PUSH)) \
        { \
            CsrLogMessageQueuePush(prim_type, ptr); \
        } \
    } while (0)
#else
#define CSR_LOG_MESSAGE_QUEUE_PUSH(prim_type, ptr)
#endif

void CsrLogMessageQueuePop(CsrUint16 prim_type,
    const void *ptr);
#if defined(CSR_LOG_GSP)
#define CSR_LOG_MESSAGE_QUEUE_POP(prim_type, ptr) \
    do { \
        if (!CsrLogTaskIsFiltered(CsrSchedTaskQueueGet(), \
                CSR_LOG_LEVEL_TASK_MESSAGE_QUEUE_POP)) \
        { \
            CsrLogMessageQueuePop(prim_type, ptr); \
        } \
    } while (0)
#else
#define CSR_LOG_MESSAGE_QUEUE_POP(prim_type, ptr)
#endif

#ifdef __cplusplus
}
#endif

#endif
